#include "cassiere.h"
#include "cliente.h"
#include "defines.h"
#include "utils.h" /* safe_seed() */
#include <pthread.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>

// // TODO: synchronized setter method
// static void set_active(cassiere_t *cassiere, int active) {
//   cassiere->
// }

/*
 * Thread di lavoro dei cassieri.
 * Il thread esegue il loop mentre is_cassa_closing(cassiere) == 0.
 * Quando is_cassa_closing(cassiere) = 0, la funzione termina con successo.
 */
static void *working_thread(void *arg) {
  assert(arg != NULL);
  cassiere_t *cassiere = (cassiere_t*)arg;
  printf("CASSA %d: attivata\n", cassa_id(cassiere));

  /* 
   * Ottiene un intero incrementale in maniera thread-safe da usare per la
   * generazione di numeri casuali.
   */
  unsigned int seed = safe_seed();
  printf("SEED GENERATO DA CASSIERE: %d\n", seed);

  struct timespec ts;
  /* generazione tempo di servizio casuale nel range 20-80 ms*/
  int service_time = 20 + rand_r(&seed) % (80-20);

  /* thread loop */
  //TODO: usare condition variable invece di attesa attiva
  while(!is_cassa_closing(cassiere) && is_cassa_active(cassiere)) {

    //TODO: usare condition variable invece di attesa attiva
    while(queue_empty(cassiere->clienti)) {
      if (is_cassa_closing(cassiere) || !is_cassa_active(cassiere)) {
        cassiere->active = 0;
        cassiere->closing = 0;
        printf("CASSA %d: chiusa\n", cassa_id(cassiere));
        return (void*)0;
      }
    }

    printf("CASSA %d: Clienti in coda %zu \n",
        cassa_id(cassiere),
        queue_size(cassiere->clienti));

    assert(queue_size(cassiere->clienti) > 0);
    cliente_t *cliente = (cliente_t*) queue_top(cassiere->clienti);

    printf("CASSA %d: Servendo cliente con %d prodotti\n",
        cassa_id(cassiere),
        cliente->products);

    /* inizializzazione timespec e calcolo tempo di servizio del cassiere:
     * il tempo di servizio (in nanosecondi) è calcolato moltiplicando il numero
     * di prodotti selezionati dal cliente con il tempo di latenza di un singolo
     * prodotto. Il totale è poi sommato al tempo di servizio costante del
     * cassiere.
     */
    ts.tv_sec = 0;
    ts.tv_nsec = (cliente->products*cassiere->tp + service_time)* 1000 * 1000;
    nanosleep(&ts, &ts); /* attendi per ts.tv_nsec nanosecondi */

    printf("CASSA %d: Cliente servito.\n", cassa_id(cassiere));
    cliente_t *servito = queue_pop(cassiere->clienti);
    assert(cliente == servito);

    assert(!servito->servito); /* il cliente non deve essere già stato servito */
    servito->servito = 1; /* setta il flag servito sul cliente, in modo che possa terminare */
  }

  cassiere->active = 0;
  cassiere->closing = 0;
  printf("CASSA %d: chiusa\n", cassa_id(cassiere));
  printf("CASSA %d: clienti in coda dopo la chiusura: %zu \n", 
      cassa_id(cassiere), queue_size(cassiere->clienti));
  //TODO: forse ha senso che sia il thread cassiere a liberare la sua memoria
  //      e quella della coda clienti:
  //      NO PERCHÈ DALL'ESTERNO DEVE ESSERE POSSIBILE CONTROLLARE SE
  //      IL CASSIERE È STATO CHIUSO (is_cassa_active(cassiere)).
  //
  return (void*)0;
}

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
 * TODO: valutare sincronizzazione accesso al campo cassiere->active;
 */
int is_cassa_active(cassiere_t *cassiere) {
  return assert(cassiere != NULL), cassiere->active;
}

/*
 * Determina se una cassa è in chiusura.
 * Restituisce un valore != 0 se la cassa è in chiusura, 0 altrimenti.
 * Nota: una cassa chiusa non è in chiusura.
 * TODO: valutare sincronizzazione accesso al campo cassiere->closing;
 */
int is_cassa_closing(cassiere_t *cassiere) {
  return assert(cassiere != NULL), cassiere->closing;
}

/*
 * Inizializza una struttura cassiere_t con i valori di default.
 * Il campo cassiere._id è un numero intero incrementale.
 * Poichè la funzione non è rientrate e non è sincronizzata, questa non risulta
 * thread-safe.
 */
void init_cassiere(cassiere_t *cassiere, int tp) {
  static int cassiere_id = 0;
  printf("Inizializzando cassiere %d\n", cassiere_id);
  assert(cassiere != NULL);
  cassiere->id = cassiere_id++;
  cassiere->active = 0; /* cassiere inizialmente non attivo */
  cassiere->closing = 0; /* cassiere inizialmente non in chiusura */
  cassiere->allocated = 0; /* thread cassiere ancora non inizializzato */
  cassiere->tp = tp;

  /* crea la coda clienti - inizialmente vuota */
  cassiere->clienti = queue_create();
}

/*
 * Apre una cassa attivando il working thread di un cassiere.
 * Successivamente alla chiamata is_cassa_active(cassiere) != 0, e il thread
 * cassiere->thread è attivo.
 */
int open_cassa(cassiere_t *cassiere) {
  assert(cassiere != NULL);
  assert(!is_cassa_active(cassiere));
  assert(!is_cassa_closing(cassiere));

  if (cassiere == NULL || is_cassa_active(cassiere)) {
    return -1;
  }

  cassiere->active = 1;
  cassiere->closing = 0;
  cassiere->allocated = 1;
  int s = pthread_create(&cassiere->thread, (void*)NULL, &working_thread, (void*)cassiere);
  if (s != 0) {
    handle_error("pthread_create cassiere");
  }

  return 0;
}

/*
 * Chiude una cassa causando la terminazione del working thread del cassiere.
 * La terminazione istantanea non è garantita, ma avviene dopo aver servito il
 * cliente corrente.
 * Se cassiere non è attivo, la funzione termina con successo.
 * TODO: valutare sincronizzazione accesso al campo cassiere->active;
 */
int close_cassa(cassiere_t *cassiere) {
  assert(cassiere != NULL);
  assert(is_cassa_active(cassiere));
  assert(!is_cassa_closing(cassiere));

  if (cassiere == NULL) {
    return -1;
  }

  /* Informa il thread del cassiere di fermarsi */
  // cassiere->active = 0;
  cassiere->closing = 1;
  return 0;
}

/*
 * Attende la chiusura di una cassa invocando pthread_join sul thread di lavoro
 * del cassiere.
 * Se il thread è già terminato, la funzione termina con successo.
 * La funzione wait_cassa() non chiude la cassa, quindi per non aspettare
 * indefinitivamente è necessario chiamare prima close_cassa().
 */
void wait_cassa(cassiere_t *cassiere) {
  assert(cassiere != NULL);
  /* la cassa deve essere chiusa oppure in chiusura */
  assert(is_cassa_closing(cassiere) || !is_cassa_active(cassiere));

  /* se il thread cassiere non è stato creato, non fare niente */
  if (!cassiere->allocated) {
    assert(!is_cassa_closing(cassiere));
    assert(!is_cassa_active(cassiere));
    return;
  }

  int s = pthread_join(cassiere->thread, NULL);
  if (s == ESRCH) { /* (s == 3) no thread with the specified ID could be found */
    /* il thread del cassiere richiesto ha già terminato */
    printf("Thread already closed\n");
    return;
  }
  if (s != 0) { /* altri tipi di errore */
    handle_error("pthread_join cassiere");
  }
}

/*
 * Aggiunge un cliente alla coda clienti di 'cassiere'.
 * L'accesso alla coda è sincronizzato dalla struttura dati queue_t.
 */
void add_cliente(cassiere_t *cassiere, cliente_t *cliente) {
  assert(cassiere != NULL && cliente != NULL);
  queue_push(cassiere->clienti, (void*)cliente);
  cliente->cassiere = cassiere;
}



