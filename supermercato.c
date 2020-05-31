#include "supermercato.h"
#include "defines.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

/*
 * Crea il Supermercato allocando `max_casse` cassieri, di cui `num_casse` 
 * vengono attivati su thread separati.
 * Poichè un Supermercato è singolarmente gestito da un Direttore,
 * non sono previsti meccanismi di sincronizzazione.
 * Requisiti:
 *  - max_casse >= 1;
 *  - 0 <= `num_casse` <= max_casse
 *
 * Restituisce: il puntatore alla struttura supermercato_t creata.
 */
supermercato_t *create_supermercato(int max_casse, int num_casse) {
  assert(num_casse >= 0 && max_casse >= 0);
  assert(max_casse >= num_casse);

  supermercato_t *s = (supermercato_t*) malloc(sizeof(supermercato_t));
  if (s == NULL) {
    handle_error("malloc supermercato");
  }
  s->max_casse = max_casse;
  s->num_casse = num_casse;


  /* Allocazione cassieri */
  s->cassieri = (cassiere_t*) malloc(sizeof(cassiere_t)*max_casse);
  if (s->cassieri == NULL) {
    handle_error("malloc cassieri");
  }

  /* Inizializzazione e apertura iniziale casse */
  for (int i=0; i<max_casse; i++) {
    init_cassiere(&s->cassieri[i]);
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

  /* Informa tutti i cassieri di chiudere le casse */
  for (uint i=0; i<supermercato->max_casse; i++) {
    close_cassa(&supermercato->cassieri[i]);
  }

  /* 
   * Attendi la chiusura effettiva di ogni cassa:
   * L'attesa viene effettuata dopo aver prima segnalato la terminazione a ogni
   * cassa, in modo da non ritardarne la chiusura.
   */
  for (uint i=0; i<supermercato->max_casse; i++) {
    wait_cassa(&supermercato->cassieri[i]);
  }

  free(supermercato->cassieri);
  free(supermercato);
}

