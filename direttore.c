#include "direttore.h"
#include "cassiere.h"
#include "supermercato.h"
#include "defines.h"
#include <pthread.h>
#include <assert.h>
#include <unistd.h>

#define PATIENCE 25

static supermercato_t *s;
static pthread_t thread_id;
static int *in_coda = NULL; /* numero clienti in coda, indicizzato dall'id dei cassieri */
static pthread_mutex_t mtx; /* mutex di sincronizzazione per l'array in_coda */

static pthread_cond_t open_close_cassa_cond = PTHREAD_COND_INITIALIZER;
static int d_s1, d_s2;
static int quit;
static int count = 0; /* numero di comunicazioni ricevute da parte dei cassieri */

/*
 * Restituisce 1 se sono verificate le condizioni per aprire una nuova cassa.
 * La funzione utilizza variabili condivise tra più thread (in_coda), quindi
 * deve essere chiamata con un lock già ottenuto.
 */
static int should_open_cassa() {
  assert(in_coda != NULL);
  for (uint i=0; i<s->max_casse; i++) {
    if (in_coda[i] >= d_s2) {
      return 1; /* è necessario aprire una cassa */
    }
  }

  return 0; /* nessuna cassa da aprire */
}

/*
 * Restituisce 1 se sono verificate le condizioni per chiudere una cassa.
 * La funzione utilizza variabili condivise tra più thread (in_coda), quindi
 * deve essere chiamata con un lock già ottenuto.
 */
static int should_close_cassa() {
  assert(in_coda != NULL);
  int n = 0; /* numero di casse aperte con al più un cliente */
  uint i;

  for (i=0; i<s->max_casse && n < d_s1; i++) {
    if (in_coda[i] <= 1) {
      n++;
    }
  }
  assert(n == d_s1 || i == s->max_casse);
  return n >= d_s1;
}

/*
 * Thread di lavoro del direttore
 */
static void *working_thread(void *arg) {
  cassiere_t *cassa;
  printf("DIRETTORE: Thread creato correttamente.\n");

  /* Thread loop:
   * attende che almeno una delle due condizioni sia verificata per 
   * aprire o chiudere una cassa.
   */
  pthread_mutex_lock_safe(&mtx); 
  while (!quit) {

    /* si mette in attesa che le condizioni per l'apertura/chiusura delle casse
     * siano verificate
     */
    while((!quit && count < PATIENCE) || (!should_open_cassa() && !should_close_cassa() && !quit)) {
      pthread_cond_wait(&open_close_cassa_cond, &mtx);
    }

    /* Ricevuto segnale di terminazione */
    if (quit) {
      pthread_mutex_unlock_safe(&mtx); 
      printf("DIRETTORE: Thread terminato.\n");
      return (void*) 0;
    }

    /* apre/chiude una cassa se sono verificate le condizioni e se sono state
     * ricevute almeno 'PATIENCE' comunicazioni da parte dei cassieri
     * dall'apertura del supermercato o dalla precedente apertura/chiusura
     * di una cassa.
     */
    if (should_open_cassa() && count >= PATIENCE) {
      /* rilascia il lock perchè open_cassa_supermercato() è una funzione bloccante */
      pthread_mutex_unlock_safe(&mtx); 
      /* apri cassa */
      cassa = open_cassa_supermercato(s);
      if (cassa != NULL) {
        printf("DIRETTORE: Aprendo cassa %d.\n", cassa_id(cassa));
      }
      pthread_mutex_lock_safe(&mtx); 
      count = 0;
    }
    if (should_close_cassa() && count >= PATIENCE) {
      /* rilascia il lock perchè close_cassa_supermercato() è una funzione bloccante */
      pthread_mutex_unlock_safe(&mtx); 
      /* chiudi cassa */
      cassa = close_cassa_supermercato(s);
      if (cassa != NULL) {
        printf("DIRETTORE: Chiudendo cassa %d.\n", cassa_id(cassa));
      }
      pthread_mutex_lock_safe(&mtx); 
      in_coda[cassa_id(cassa)] = 0;
      count = 0;
    }
  }

  pthread_mutex_unlock_safe(&mtx); 

  printf("DIRETTORE: Thread terminato.\n");
  return (void*)0;
}

/*
 * Inizializza e fa partire il thread del direttore.
 * I parametri s1 e s2 rappresentano i valori soglia che condizionano
 * l'apertura o la chiusura di una cassa da parte del direttore.
 * Se ci sono almeno s1 casse aperte con al più un cliente in coda, viene chiusa
 * una cassa tra quelle aperte.
 * Se ci sono almeno s2 clienti in coda in almeno una cassa, viene aperta una 
 * nuova cassa, se possibile.
 */
void init_direttore(supermercato_t *supermercato, int s1, int s2) {
  assert(supermercato != NULL);
  assert(supermercato->max_casse > 0);

  s = supermercato;
  in_coda = (int*) calloc(s->max_casse, sizeof(int));
  if (in_coda == NULL) {
    handle_error("init_direttore calloc");
  }

  pthread_mutex_init_ec(&mtx, NULL);
  d_s1 = s1;
  d_s2 = s2;
  quit = 0;
  count = 0;

  int res = pthread_create(&thread_id, NULL, &working_thread, NULL);
  if (res != 0) {
    handle_error("init_direttore pthread_create");
  }

}

/*
 * Comunica al direttore il numero di clienti (n) attualmente in coda alla
 * cassa del cassiere passato come parametro.
 */
void comunica_numero_clienti(const cassiere_t *cassiere, int n) {
  assert(cassiere != NULL);
  assert(cassa_id(cassiere) >= 0);
  assert(cassa_id(cassiere) < (int)s->max_casse);
  assert(in_coda != NULL);

  pthread_mutex_lock_safe(&mtx);

  in_coda[cassa_id(cassiere)] = n;

  assert(count >= 0);
  count++;

  /* Segnala al direttore gli attuali clienti in coda */
  pthread_cond_signal(&open_close_cassa_cond);

  pthread_mutex_unlock_safe(&mtx);
}

/*
 * Termina l'esecuzione del thread direttore e libera le risorse allocate.
 */
void terminate_direttore() {
  pthread_mutex_lock_safe(&mtx);
  quit = 1;

  pthread_cond_signal(&open_close_cassa_cond); /* risveglia il thread */
  pthread_mutex_unlock_safe(&mtx);

  pthread_join(thread_id, NULL);
  free(in_coda);
}

/*
 * Comunica (il cliente) con il direttore la volontà di voler uscire dal
 * supermercato.
 * Il permesso è concesso se la funzione termina con successo.
 */
void get_permesso() {
  pthread_mutex_lock_safe(&mtx);
  /* dummy lock acquire to simulate a request */
  pthread_mutex_unlock_safe(&mtx);
}
