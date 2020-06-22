#include "cassiere.h"
#include "cliente.h"
#include "defines.h"
#include "utils.h" /* safe_seed() */
#include "direttore.h"
#include <pthread.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#define INTERVAL 1000

static void set_active(cassiere_t *cassiere, int active) {
  pthread_mutex_lock_safe(&cassiere->mtx);
  cassiere->active = active;
  pthread_cond_signal(&cassiere->not_empty_cond); /* risveglia il thread cassiere */
  pthread_mutex_unlock_safe(&cassiere->mtx);
}

static void set_closing(cassiere_t *cassiere, int closing) {
  pthread_mutex_lock_safe(&cassiere->mtx);
  cassiere->closing = closing;
  pthread_cond_signal(&cassiere->not_empty_cond); /* risveglia il thread cassiere */
  pthread_mutex_unlock_safe(&cassiere->mtx);
}

static void set_allocated(cassiere_t *cassiere, int allocated) {
  pthread_mutex_lock_safe(&cassiere->mtx);
  cassiere->allocated = allocated;
  pthread_cond_signal(&cassiere->not_empty_cond); /* risveglia il thread cassiere */
  pthread_mutex_unlock_safe(&cassiere->mtx);
}

static int diff_ms(struct timespec start, struct timespec end) {
  return (end.tv_sec - start.tv_sec)*1000 + (end.tv_nsec - start.tv_nsec)/(1000*1000);
}

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
  // printf("SEED GENERATO DA CASSIERE: %d\n", seed);


  struct timespec ts, timer_start, timer_end, twait, client_start, client_end;
  /* generazione tempo di servizio casuale nel range 20-80 ms*/
  int service_time = 20 + rand_r(&seed) % (80-20);
  int waiting_time;
  int remaining_time = INTERVAL; /* TODO: parametro di configurazione */

  pthread_mutex_lock_safe(&cassiere->mtx);
  clock_gettime(CLOCK_REALTIME, &timer_start); /* Inizializza il timer */
  /* thread loop */
  while(!cassiere->closing && cassiere->active) {
    
    //TODO: debug
    printf("CASSA %d: remaining_time = %.5f (s)\n", 
        cassa_id(cassiere),
        (double)remaining_time/1000);
    /*
     * Nota: la coda clienti è acceduta concorrentemente dal cassiere e dai
     * thread che gestiscono la creazione e la rilocazione dei clienti.
     * Pertanto l'unico thread che elimina elementi dalla coda è il cassiere e
     * non c'è rischio che la condizione !queue_empty(cassiere->clienti) cambi
     * senza che il cassiere processi un nuovo cliente. L'utilizzo di chiamate
     * "atomiche" (sincronizzate dalla coda stessa), è quindi sicuro e non è
     * necessario mantenere il lock sulla coda.
     * Il mutex cassiere->mtx è utilizzato per sincronizzare soltanto l'accesso
     * ai campi active e closing del cassiere.
     */
    while(queue_empty(cassiere->clienti) 
        && !cassiere->closing 
        && cassiere->active) {

      if (remaining_time <= 0) {
        /* Comunica il numero di clienti in coda al direttore */
        comunica_numero_clienti(cassiere, queue_size(cassiere->clienti));
        clock_gettime(CLOCK_REALTIME, &timer_start);
        remaining_time = INTERVAL;
      }

      /* Rimane in attesa di nuovi clienti da servire al più 
       * remaining_time millisecondi, tempo dopo il quale è necessario
       * contattare il direttore.
       */
      clock_gettime(CLOCK_REALTIME, &twait);
      twait.tv_sec += (remaining_time / 1000);
      twait.tv_nsec += (remaining_time % 1000)*1000*1000;

      int res = pthread_cond_timedwait(&cassiere->not_empty_cond, &cassiere->mtx, &twait);
      if (res == ETIMEDOUT) {
        /* Comunica il numero di clienti in coda al direttore */
        comunica_numero_clienti(cassiere, queue_size(cassiere->clienti));
        clock_gettime(CLOCK_REALTIME, &timer_start);
        remaining_time = INTERVAL;
      }
    }

    /* Controlla che nel frattempo la cassa non sia stata chiusa.
     * Nota: Il lock è stato acquisito dalla wait 
     */
    if (cassiere->closing || !cassiere->active) {
      break;
    }

    /* Sottrae dal tempo rimanente il tempo impiegato in attesa di nuovi clienti */
    clock_gettime(CLOCK_REALTIME, &timer_end);
    remaining_time -= diff_ms(timer_start, timer_end);
    clock_gettime(CLOCK_REALTIME, &timer_start);

    /* Comunica con il direttore se il tempo è scaduto */
    if (remaining_time <= 0) {
      /* Comunica il numero di clienti in coda al direttore */
      comunica_numero_clienti(cassiere, queue_size(cassiere->clienti));

      /* Resetta il timer per la comunicazione con il direttore */
      clock_gettime(CLOCK_REALTIME, &timer_start);
      remaining_time = INTERVAL; //TODO: parametro di configurazione
    }


    assert(queue_size(cassiere->clienti) > 0);
    cliente_t *cliente = (cliente_t*) queue_top(cassiere->clienti);

    printf("CASSA %d: Servendo cliente con %d prodotti\n",
        cassa_id(cassiere),
        cliente->products);

    /* Rilascia il mutex prima che il thread si blocchi */
    pthread_mutex_unlock_safe(&cassiere->mtx);
    clock_gettime(CLOCK_REALTIME, &client_start);

    /* Inizializzazione timespec e calcolo tempo di servizio del cassiere:
     * il tempo di servizio (in nanosecondi) è calcolato moltiplicando il numero
     * di prodotti selezionati dal cliente con il tempo di latenza di un singolo
     * prodotto. Il totale è poi sommato al tempo di servizio costante del
     * cassiere.
     */
    waiting_time = (cliente->products*cassiere->tp + service_time); // ms
    ts.tv_sec = waiting_time / 1000; // secondi
    ts.tv_nsec = (waiting_time % 1000)*1000*1000; // nanosecondi

    /*
     * Se il tempo per servire il cliente è maggiore del tempo rimanente
     * dell'intervallo di comunicazione con il direttore, inizia a servire il
     * cliente, poi il cassiere si ferma, comunica con il direttore e termina
     * di servire il cliente.
     */
    if (waiting_time > remaining_time) {
      assert(remaining_time > 0);
      /* inizia a servire il cliente */ 
      ts.tv_sec = (waiting_time - remaining_time) / 1000; // secondi
      ts.tv_nsec = ((waiting_time - remaining_time) % 1000)*1000*1000; // nanosecondi
      nanosleep(&ts, &ts); 

      /* Comunica il numero di clienti in coda al direttore */
      pthread_mutex_lock_safe(&cassiere->mtx);
      comunica_numero_clienti(cassiere, queue_size(cassiere->clienti));
      pthread_mutex_unlock_safe(&cassiere->mtx);

      /* Imposta il tempo di servizio rimanente per processare il cliente */
      ts.tv_sec = remaining_time / 1000; // secondi
      ts.tv_nsec = (remaining_time % 1000)*1000*1000; // nanosecondi

      /* Resetta il timer per la comunicazione con il direttore */
      clock_gettime(CLOCK_REALTIME, &timer_start);
      remaining_time = INTERVAL; //TODO: parametro di configurazione
    }

    nanosleep(&ts, &ts); /* servi il cliente */

    printf("CASSA %d: Cliente servito.\n", cassa_id(cassiere));
    cliente_t *servito = queue_pop(cassiere->clienti);
    assert(cliente == servito);

    /* Informa il thread cliente che è stato servito */
    set_servito(servito, 1);
    clock_gettime(CLOCK_REALTIME, &client_end);
    clock_gettime(CLOCK_REALTIME, &timer_end);

    //TODO: debug
    // int t = diff_ms(client_start, client_end);
    // printf("CASSA %d: TSR = %.5f (s)  TST = %.5f (s)\n",
    //     cassa_id(cassiere),
    //     (double)t/1000,
    //     (double)waiting_time/1000);

    /* Verifica che lo scarto tra il tempo impiegato e il tempo teorico di servizio 
     * sia minore del 10% del tempo teorico di servizio
     */
    // assert(abs(t - waiting_time) <= (int)(waiting_time*0.05));

    /* Sottrae dal tempo rimanente il tempo impiegato per processare il cliente */
    remaining_time -= diff_ms(timer_start, timer_end);
    clock_gettime(CLOCK_REALTIME, &timer_start);

    /* Riacquisisce il lock sul cassiere in modo da poter verificare la
     * condizione del ciclo While in modo sicuro
     */
    pthread_mutex_lock_safe(&cassiere->mtx);
  }


  printf("CASSA %d: chiusa\n", cassa_id(cassiere));
  printf("CASSA %d: clienti in coda dopo la chiusura: %zu \n", 
      cassa_id(cassiere), queue_size(cassiere->clienti));

  /* segnalazione chiusura cassa ai clienti */
  while (!queue_empty(cassiere->clienti)) {
    cliente_t *c = queue_pop(cassiere->clienti);

    pthread_mutex_lock_safe(&c->mtx);
    pthread_cond_signal(&c->servito_cond);
    pthread_mutex_unlock_safe(&c->mtx);
  }

  cassiere->active = 0;
  cassiere->closing = 0;

  /* Rilascia il lock sul cassiere */
  pthread_mutex_unlock_safe(&cassiere->mtx);
  return (void*)0;
}

/*
 * Determina l'id univoco di un cassiere.
 * Restituisce un valore intero >= 0.
 */
//TODO: valutare sincronizzazione
int cassa_id(const cassiere_t *cassiere) {
  return assert(cassiere != NULL), cassiere->id;
}

/*
 * Determina se una cassa è aperta.
 * Restituisce 0 se la cassa è chiusa e un valore != 0 altrimenti.
 */
int is_cassa_active(cassiere_t *cassiere) {
  assert(cassiere != NULL);
  pthread_mutex_lock_safe(&cassiere->mtx);
  int active = cassiere->active;
  pthread_mutex_unlock_safe(&cassiere->mtx);

  return active;
}

/*
 * Determina se una cassa è in chiusura.
 * Restituisce un valore != 0 se la cassa è in chiusura, 0 altrimenti.
 * Nota: una cassa chiusa non è in chiusura.
 */
int is_cassa_closing(cassiere_t *cassiere) {
  assert(cassiere != NULL);
  pthread_mutex_lock_safe(&cassiere->mtx);
  int closing = cassiere->closing;
  pthread_mutex_unlock_safe(&cassiere->mtx);

  return closing;
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
  cassiere->clienti_serviti =  0;
  cassiere->numero_chiusure =  0;

  /* crea la coda clienti - inizialmente vuota */
  cassiere->clienti = queue_create();
  pthread_mutex_init_ec(&cassiere->mtx, NULL);
  pthread_cond_init(&cassiere->not_empty_cond, NULL);
}

/*
 * Apre una cassa attivando il working thread di un cassiere.
 * Successivamente alla chiamata is_cassa_active(cassiere) != 0, e il thread
 * cassiere->thread è attivo.
 *
 * Restituisce: 0 se la cassa è stata aperta correttamente e un valore != 0
 * altrimenti.
 */
int open_cassa(cassiere_t *cassiere) {
  assert(cassiere != NULL);
  assert(!is_cassa_active(cassiere));
  assert(!is_cassa_closing(cassiere));

  if (cassiere == NULL || is_cassa_active(cassiere)) {
    return -1;
  }

  set_active(cassiere, 1);
  set_closing(cassiere, 0);
  cassiere->allocated = 1; //TODO: valutare sincronizzazione
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
 * Se il cassiere non è attivo, la funzione termina con successo.
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
  set_closing(cassiere, 1);
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
  // assert(is_cassa_closing(cassiere) || !is_cassa_active(cassiere));

  /* se il thread cassiere non è stato creato, non fare niente */
  if (!cassiere->allocated) {
    assert(!is_cassa_closing(cassiere));
    assert(!is_cassa_active(cassiere));
    return;
  }

  printf("CASSA %d: joining thread\n", cassa_id(cassiere));

  int s = pthread_join(cassiere->thread, NULL);
  if (s == ESRCH) { /* (s == 3) no thread with the specified ID could be found */
    /* il thread del cassiere richiesto ha già terminato */
    // printf("Thread already closed\n"); //TODO: used for debugging
    return;
  }
  if (s != 0) { /* altri tipi di errore */
    handle_error("pthread_join cassiere");
  }

  printf("CASSA %d: thread joined\n", cassa_id(cassiere));

  set_allocated(cassiere, 0);
}

/*
 * Aggiunge un cliente alla coda clienti di 'cassiere'.
 * -- L'accesso alla coda è sincronizzato dalla struttura dati queue_t. --
 */
void add_cliente(cassiere_t *cassiere, cliente_t *cliente) {
  assert(cassiere != NULL && cliente != NULL);
  pthread_mutex_lock_safe(&cassiere->mtx);

  queue_push(cassiere->clienti, (void*)cliente);
  cliente->cassiere = cassiere;
  /* segnala il cassiere che ci sono nuovi clienti da servire */
  pthread_cond_signal(&cassiere->not_empty_cond); 

  pthread_mutex_unlock_safe(&cassiere->mtx);
}



