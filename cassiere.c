#include "cassiere.h"
#include "defines.h"
#include <pthread.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>

/*
 * Determina l'id univoco di un cassiere.
 * Restituisce un valore intero >= 0.
 */
int cassa_id(cassiere_t *cassiere) {
  return assert(cassiere != NULL), cassiere->id;
}

/*
 * Determina se una cassa è aperta.
 * Restituisce 0 se la cassa è chiusa e un valore != 0 altrimenti.
 */
int is_cassa_active(cassiere_t *cassiere) {
  return assert(cassiere != NULL), cassiere->active;
}

/*
 * Thread di lavoro dei cassieri.
 * Il thread esegue il loop finchè is_cassa_active(cassiere) != 0.
 * Quando is_cassa_active(cassiere) = 0, la funzione termina con successo.
 */
static void *working_thread(void *arg) {
  assert(arg != NULL);
  cassiere_t *cassiere = (cassiere_t*)arg;
  printf("Cassa %d attivata\n", cassa_id(cassiere));

  /* thread loop */
  while(is_cassa_active(cassiere)) {
    printf("Cassa %d looping...\n", cassa_id(cassiere));
    sleep(1);
  }

  printf("Cassa %d chiusa\n", cassa_id(cassiere));
  return (void*)0;
}

/*
 * Inizializza una struttura cassiere_t con i valori di default.
 * Il campo cassiere._id è un numero intero incrementale.
 * Poichè la gestione dell'id è a carico della funzione, questa non risulta
 * thread_safe.
 */
void init_cassiere(cassiere_t *cassiere) {
  static int cassiere_id = 0;
  printf("Inizializzando cassiere %d\n", cassiere_id);
  assert(cassiere != NULL);
  cassiere->id = cassiere_id++;
  cassiere->active = 0;
}

/*
 * Apre una cassa attivando il working thread di un cassiere.
 * Successivamente alla chiamata is_cassa_active(cassiere) != 0, e il thread
 * cassiere->thread è attivo.
 */
int open_cassa(cassiere_t *cassiere) {
  assert(cassiere != NULL);
  assert(!is_cassa_active(cassiere));

  if (cassiere == NULL || is_cassa_active(cassiere)) {
    return -1;
  }

  cassiere->active = 1;
  int s = pthread_create(&cassiere->thread, (void*)NULL, &working_thread, (void*)cassiere);
  if (s != 0) {
    handle_error("pthread_create cassiere");
  }

  return 0;
}

/*
 * Chiude una cassa causando la terminazione del working thread del cassiere.
 * La terminazione istantanea non è garantita.
 * Se cassiere non è attivo, la funzione termina con successo.
 * TODO: valutare sincronizzazione accesso al campo cassiere->_active;
 */
int close_cassa(cassiere_t *cassiere) {
  assert(cassiere != NULL);
  if (cassiere == NULL) {
    return -1;
  }

  /* Informa il thread del cassiere di fermarsi */
  cassiere->active = 0;
  return 0;
}

/*
 * Attende la chiusura di una cassa invocando pthread_join sul thread di lavoro
 * del cassiere.
 * La funzione wait_cassa() non chiude la cassa, quindi per non aspettare
 * indefinitivamente è necessario chiamare prima close_cassa().
 */
void wait_cassa(cassiere_t *cassiere) {
  assert(cassiere != NULL);
  assert(!is_cassa_active(cassiere)); /* ha senso aspettare una cassa aperta? */

  int res;
  int s = pthread_join(cassiere->thread, (void*)&res);
  if (s != 0) {
    handle_error("pthread_join cassiere");
  }
}




