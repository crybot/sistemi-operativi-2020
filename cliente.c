#include "cliente.h"
#include "supermercato.h"
#include "defines.h"
#include "utils.h" /* safe_seed() */
#include "logger.h"
#include "stopwatch.h"
#include "direttore.h" /* get_permesso() */
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>

/*
 * Thread di lavoro dei clienti.
 */
void* cliente_worker(void* arg) {
  assert(arg != NULL);
  cliente_t* cliente = (cliente_t*) arg;

  /* 
   * Ottiene un intero incrementale in maniera thread-safe da usare per la
   * generazione di numeri casuali.
   */
  unsigned int queue_changes = 0;
  unsigned int seed = safe_seed();

  /* Creazione record timespec e conversione dwell_time in nanosecondi */
  struct timespec ts;
  ts.tv_sec = cliente->dwell_time / 1000; // secondi
  ts.tv_nsec = (cliente->dwell_time % 1000)*1000*1000; // nanosecondi

  /* Cronometri utilizzati per misurare il tempo impiegato dal cliente nel
   * supermercato e in coda */
  stopwatch_t *total_time = stopwatch_create(STOPWATCH_STARTING);
  stopwatch_t *queue_time = stopwatch_create(STOPWATCH_STOPPED);

  /* Cliente impiega dwell_time millisecondi scegliendo i prodotti */
  nanosleep(&ts, &ts);
  log_write("CLIENTE %d: terminato di scegliere gli acquisti dopo %d ms \n", 
      cliente->id, cliente->dwell_time);

  /* Se il cliente non ha acquistato prodotti, chiede il permesso di uscire
   * al direttore.
   */
  if (cliente->products == 0) {
    get_permesso();
    log_write("CLIENTE %d: terminato senza acquistare prodotti\n", cliente->id);
    log_write("CLIENTE %d: tempo totale = %d ms\n", cliente->id, stopwatch_end(total_time));
    stopwatch_free(total_time);
    stopwatch_free(queue_time);
    return 0;
  }

  /* Thread loop: 
   * attende finchè il cliente non viene servito,
   * la cassa è stata chiusa, oppure il supermercato sta chiudendo.
   */
  cassiere_t *cassa;

  pthread_mutex_lock_safe(&cliente->mtx);
  while(!cliente->servito) {

    /* Se il cliente non è accodato ad alcuna cassa o se la cassa a in cui è in
     * coda è stata chiusa, cerca una cassa aperta e vi si accoda
     * Nota: la cassa è chiusa solo dopo che il cliente corrente è stato
     * servito, cioè la cassa non è più attiva e non è più in chiusura.
     * Nota: per chiarezza la condizione di chiusura e di non nullità è divisa
     * in due branch if-else, al fine di favorire la leggibilità.
     */
    if (cliente->cassiere == NULL) {
      cassa = place_cliente(cliente, cliente->supermercato, &seed);

      /* comincia a misurare il tempo trascorso in coda*/
      if (cassa != NULL) {
        stopwatch_start(queue_time); 
      }
    }
    else if (!is_cassa_active(cliente->cassiere) 
        && !is_cassa_closing(cliente->cassiere)) {
      cassa = place_cliente(cliente, cliente->supermercato, &seed);
      if (cassa != NULL) {
        ++queue_changes;
      }
    }

    if (cassa == NULL) { /* non ci sono casse aperte nel supermercato */
      assert(cliente->supermercato->num_casse == 0);
      assert(!cliente->servito);
      assert(cliente->cassiere == NULL || !is_cassa_closing(cliente->cassiere));
      pthread_mutex_unlock_safe(&cliente->mtx);
      log_write("CLIENTE %d: terminato per mancanza di casse aperte\n", cliente->id);
      log_write("CLIENTE %d: tempo totale = %d ms\n", cliente->id, stopwatch_end(total_time));
      log_write("CLIENTE %d: tempo in coda = %d ms\n", cliente->id, stopwatch_end(queue_time));
      log_write("CLIENTE %d: cambi di coda = %d \n", cliente->id, queue_changes);
      stopwatch_free(total_time);
      stopwatch_free(queue_time);
      return (void*) 1; /* cliente non servito */
    }
    pthread_cond_wait(&cliente->servito_cond, &cliente->mtx);
  }

  pthread_mutex_unlock_safe(&cliente->mtx);
  log_write("CLIENTE %d: prodotti acquistati = %d\n", cliente->id, cliente->products);
  log_write("CLIENTE %d: tempo totale = %d ms\n", cliente->id, stopwatch_end(total_time));
  log_write("CLIENTE %d: tempo in coda = %d ms\n", cliente->id, stopwatch_end(queue_time));
  log_write("CLIENTE %d: cambi di coda = %d \n", cliente->id, queue_changes);
  stopwatch_free(total_time);
  stopwatch_free(queue_time);
  return (void*) 0;
}

/*
 * Alloca un cliente.
 * - dwell_time indica il tempo (in millisecondi) che impiega il cliente per 
 *   scegliere i prodotti da acquistare;
 * - products indica il numero di prodotti che acquisterà il cliente.
 *
 * Restituisce: un puntatore al cliente creato.
 */
cliente_t *create_cliente(int dwell_time, int products, supermercato_t *supermercato) {
  static int cliente_id = 0;
  assert(supermercato != NULL);
  assert(dwell_time >= 0);
  assert(products >= 0);

  cliente_t *cliente = (cliente_t *) malloc(sizeof(cliente_t));
  if (cliente == NULL) {
    handle_error("malloc create_cliente");
  }

  cliente->id = cliente_id++;
  cliente->dwell_time = dwell_time;
  cliente->products = products;
  cliente->servito = 0;
  cliente->supermercato = supermercato;
  cliente->cassiere = NULL;

  pthread_mutex_init_ec(&cliente->mtx, NULL);
  pthread_cond_init(&cliente->servito_cond, NULL);

  return cliente;
}

/*
 * Libera la memoria allocata da un cliente e dal suo thread, dopo averne
 * effettuato il join.
 */
void free_cliente(cliente_t* cliente) {
  log_write("CLIENTE %d: Liberando memoria\n", cliente->id);
  free(cliente);
}

void set_servito(cliente_t *cliente, int servito) {
  pthread_mutex_lock_safe(&cliente->mtx);
  cliente->servito = servito;
  pthread_cond_signal(&cliente->servito_cond);
  pthread_mutex_unlock_safe(&cliente->mtx);
}

