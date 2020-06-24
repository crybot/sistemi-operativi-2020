#include "supermercato.h"
#include "defines.h"
#include "logger.h"
#include "parser.h" /* config_t */
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

/*
 * Crea il Supermercato allocando `max_casse` cassieri, di cui `num_casse` 
 * vengono attivati su thread separati.
 * I cassieri creati dal supermercato processano i singoli prodotti ciascuno
 * in `tempo` millisecondi.
 * Poichè un Supermercato è singolarmente gestito da un Direttore,
 * non sono previsti meccanismi di sincronizzazione.
 * Requisiti:
 *  - max_casse >= 1;
 *  - 0 <= `num_casse` <= max_casse
 *  - tempo >= 0
 *
 * Restituisce: il puntatore alla struttura supermercato_t creata.
 */
supermercato_t *create_supermercato(const config_t *config) {
  assert(config != NULL);
  int max_casse = config->params[K], num_casse = config->params[I];
  int tempo = config->params[TP];
  assert(num_casse >= 0 && max_casse >= 0);
  assert(max_casse >= num_casse);
  assert(tempo >= 0);

  /* Inizializzazione logger */
  log_setfile(config->LOG);

  supermercato_t *s = (supermercato_t*) malloc(sizeof(supermercato_t));
  if (s == NULL) {
    handle_error("malloc supermercato");
  }
  s->max_casse = max_casse;
  s->num_casse = num_casse;

  /* Inizializzazione mutex cassieri */
  pthread_mutex_init_ec(&s->cassieri_mtx, NULL);
  
  /* Allocazione cassieri */
  s->cassieri = (cassiere_t*) malloc(sizeof(cassiere_t)*max_casse);
  if (s->cassieri == NULL) {
    handle_error("malloc cassieri");
  }

  /* Inizializzazione e apertura iniziale casse:
   * non è necessario ottenere il lock dei cassieri in quanto al momento
   * nessuno (oltre a supermercato) ne detiene i riferimenti.
   */
  for (int i=0; i<max_casse; i++) {
    init_cassiere(&s->cassieri[i], tempo, config->params[S]);
    if (i < num_casse) {
      open_cassa(&s->cassieri[i]);
    }
  }

  return s;
}

/*
 * Chiude tutte le casse del Supermercato e libera le risorse allocate.
 * La funzione termina con successo quando tutti i thread di lavoro dei
 * cassieri sono stati chiusi.
 */
void close_supermercato(supermercato_t *supermercato) {
  assert(supermercato != NULL);
  assert((int)supermercato->num_casse >= 0);

  pthread_mutex_lock_safe(&supermercato->cassieri_mtx);
  /* Informa tutti i cassieri di chiudere le casse */
  for (uint i=0; i<supermercato->max_casse; i++) {
    if (is_cassa_active(&supermercato->cassieri[i])
        && !is_cassa_closing(&supermercato->cassieri[i])) {
      close_cassa(&supermercato->cassieri[i]);
      supermercato->num_casse--;
      assert((int)supermercato->num_casse >= 0);
    }
  }
  pthread_mutex_unlock_safe(&supermercato->cassieri_mtx);

  /* 
   * Attendi la chiusura effettiva di ogni cassa:
   * L'attesa viene effettuata dopo aver prima segnalato la terminazione a ogni
   * cassa, in modo da non ritardarne la chiusura.
   */
  for (uint i=0; i<supermercato->max_casse; i++) {
    wait_cassa(&supermercato->cassieri[i]);
  }

  /*
   * Logging statistiche supermercato
   */
  int totale_prodotti = 0;
  int totale_serviti = 0;
  for (uint i=0; i<supermercato->max_casse; i++) {
    log_write("CASSA %d: prodotti venduti = %d\n",
        cassa_id(&supermercato->cassieri[i]),
        supermercato->cassieri[i].prodotti_venduti);
    log_write("CASSA %d: clienti serviti = %d\n",
        cassa_id(&supermercato->cassieri[i]),
        supermercato->cassieri[i].clienti_serviti);
    log_write("CASSA %d: numero chiusure = %d\n",
        cassa_id(&supermercato->cassieri[i]),
        supermercato->cassieri[i].numero_chiusure);
    log_write("CASSA %d: tempo totale = %.3f\n",
        cassa_id(&supermercato->cassieri[i]),
        (double)supermercato->cassieri[i].tempo_totale/1000);
    log_write("CASSA %d: tempo medio servizio = %.3f\n",
        cassa_id(&supermercato->cassieri[i]),
        (double)supermercato->cassieri[i].tempo_medio/1000);
    totale_prodotti += supermercato->cassieri[i].prodotti_venduti;
    totale_serviti += supermercato->cassieri[i].clienti_serviti;
  }

  log_write("SUPERMERCATO: prodotti venduti = %d\n", totale_prodotti);
  log_write("SUPERMERCATO: clienti serviti = %d\n", totale_serviti);
}

/*
 * Libera tutte le risorse allocate dal supermercato, 
 * compresi cassieri e code clienti
 */
void free_supermercato(supermercato_t *supermercato) {
  assert(supermercato != NULL);
  for (uint i=0; i<supermercato->max_casse; i++) {
    /* Libera la memoria delle code clienti */
    queue_free(supermercato->cassieri[i].clienti);
  }
  log_close();
  free(supermercato->cassieri);
  free(supermercato);
}


/*
 * Seleziona una cassa in modo casuale tra quelle aperte e vi accoda il cliente.
 * Il parametro seed viene utilizzato per determinare in modo casuale la cassa.
 *
 * Restituisce: un puntatore al cassiere a cui il cliente viene accodato.
 *              Se non ci sono casse aperte, restituisce NULL.
 */
cassiere_t* place_cliente(cliente_t *cliente, supermercato_t *supermercato, unsigned int *seed) {
  assert(cliente != NULL && supermercato != NULL);
  assert(seed != NULL);

  pthread_mutex_lock_safe(&supermercato->cassieri_mtx);

  if (supermercato->num_casse == 0) { /* Non ci sono casse aperte */
    pthread_mutex_unlock_safe(&supermercato->cassieri_mtx);
    return NULL;
  }

  cassiere_t *cassieri = supermercato->cassieri;
  cassiere_t *scelta = NULL;

  /* numero casuale tra 0 e num_casse - 1: se c'è solo una cassa attiva r = 0 */
  unsigned int attive = 0;
  unsigned int r = 0;
  if (supermercato->num_casse > 1) {
    r = rand_r(seed) % supermercato->num_casse; 
  }

  /* trova la r-esima cassa aperta */
  for (uint i=0; i<supermercato->max_casse && attive <= r; i++) {
    assert(attive < supermercato->num_casse);
    assert(scelta == NULL);
    if (is_cassa_active(&cassieri[i]) 
        && !is_cassa_closing(&cassieri[i])
        && attive++ == r) {
      scelta = &cassieri[i];
    }
  }

  assert(scelta != NULL); /* deve esserci almeno una cassa aperta */
  assert(attive == r + 1); /* le casse attive visitate in sequenza sono r - 1 */

  /* dal file cassiere.c */
  add_cliente(scelta, cliente);

  pthread_mutex_unlock_safe(&supermercato->cassieri_mtx);

  return scelta;
}


/*
 * Apre la prima cassa disponibile del supermercato.
 * Se non ce ne sono, restituisce NULL, altrimenti restituisce il puntatore
 * alla cassa aperta.
 */
cassiere_t *open_cassa_supermercato(supermercato_t *supermercato) {
  assert(supermercato != NULL);
  assert((int)supermercato->num_casse >= 0);

  pthread_mutex_lock_safe(&supermercato->cassieri_mtx);

  cassiere_t *cassa = NULL;
  /* cerca la prima cassa che rispetta le condizioni per essere aperta */
  for (uint i=0; i<supermercato->max_casse && cassa == NULL; i++) {

    /* se la cassa non è attiva e non è in chiusura */
    if (!is_cassa_active(&supermercato->cassieri[i]) 
        && !is_cassa_closing(&supermercato->cassieri[i])) {
      /* mi assicuro che il thread del cassiere sia già stato terminato */
      pthread_mutex_unlock_safe(&supermercato->cassieri_mtx);
      wait_cassa(&supermercato->cassieri[i]);
      pthread_mutex_lock_safe(&supermercato->cassieri_mtx);

      /* quindi apro la cassa */
      if (open_cassa(&supermercato->cassieri[i]) == 0) {
        cassa = &supermercato->cassieri[i]; /* cassa aperta correttamente */
        supermercato->num_casse++; /* incrementa il numero di casse aperte */
      }
    }

  }
  pthread_mutex_unlock_safe(&supermercato->cassieri_mtx);

  return cassa;
}


/*
 * Chiude la prima cassa tra quelle aperte.
 * Se non ce ne sono, restituisce NULL, altrimenti restituisce il puntatore
 * alla cassa chiusa.
 */
cassiere_t *close_cassa_supermercato(supermercato_t *supermercato) {
  assert(supermercato != NULL);
  assert((int)supermercato->num_casse >= 0);

  pthread_mutex_lock_safe(&supermercato->cassieri_mtx);

  if (supermercato->num_casse == 0) {
    pthread_mutex_unlock_safe(&supermercato->cassieri_mtx);
    return NULL;
  }

  cassiere_t *cassa = NULL;
  /* cerca la prima cassa che rispetta le condizioni per essere chiusa */
  for (uint i=0; i<supermercato->max_casse && cassa == NULL; i++) {

    /* se la cassa è attiva e non è in chiusura */
    if (is_cassa_active(&supermercato->cassieri[i]) 
        && !is_cassa_closing(&supermercato->cassieri[i])) {

      /* chiudo la cassa */
      if (close_cassa(&supermercato->cassieri[i]) == 0) {
        cassa = &supermercato->cassieri[i]; /* cassa correttamente in chiusura */
        supermercato->num_casse--; /* decrementa il numero di casse aperte */

        /* rilascio il lock per attendere la cassa e non causare deadlock */
        pthread_mutex_unlock_safe(&supermercato->cassieri_mtx);
        wait_cassa(&supermercato->cassieri[i]); /* aspetta che la cassa chiuda */
        pthread_mutex_lock_safe(&supermercato->cassieri_mtx);
      }
    }

  }
  pthread_mutex_unlock_safe(&supermercato->cassieri_mtx);

  return cassa;
}

