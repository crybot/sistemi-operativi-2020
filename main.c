#include "supermercato.h"
#include "cassiere.h"
#include "cliente.h"
#include "parser.h"
#include "direttore.h"
#include "queue.h"
#include "threadpool.h"
#include "defines.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <assert.h>

static pthread_t create_thread;
static pthread_mutex_t deallocate_mtx;
static int quit = 0;

static int quit_thread() {
  pthread_mutex_lock_safe(&deallocate_mtx);
  int q = quit;
  pthread_mutex_unlock_safe(&deallocate_mtx);

  return q;
}

static void set_quit() {
  /* Informa il thread di terminare la propria esecuzione */
  pthread_mutex_lock_safe(&deallocate_mtx);
  quit = 1;
  pthread_mutex_unlock_safe(&deallocate_mtx);
  /* Termina il thread quanto entra in un cancellation point 
   * se non ha già terminato.
   * Serve per evitare che il thread si blocchi dopo una chiamata bloccante e 
   * che non riceva quindi il segnale di chiusura (quit = 1).
   * pthread_cancel() determina l'esecuzione della routine di cleanup impostata
   * dal thread con pthraed_cleanup_push().
   * NOTE: printf() potrebbe essere un cancellation point secondo la
   * documentazione, quindi è necessario disabilitare la cencellazione del
   * thread prima di chiamare tale funzione.
   */
  if (pthread_cancel(create_thread) != 0) {
    handle_error("set_quit: pthread_cancel");
  }
}

struct t_info {
  supermercato_t *supermercato;
  const config_t *config;
};


/*
 * Crea un nuovo cliente con al massimo p prodotti e con tempo di permanenza
 * di al più t millisecondi.
 */
static cliente_t *generate_cliente(int p, int t, supermercato_t *s) {
  int n = rand() % p; /* prodotti 0-20 */
  int dwell = 10 + rand() % (t - 10); /* dwell time 10-t ms */
  return create_cliente(dwell, n, s);
}

static void cleanup(void* arg) {
  assert(arg != NULL);
  printf("ESEGUENDO CLEANUP HANDLER...\n");
  threadpool_t *tpool = (threadpool_t*) arg;
  threadpool_free(tpool);
  printf("CLEANUP ESEGUITO\n");
}

/*
 * Working thread per la creazione clienti:
 * Inizialmente vengono creati C clienti e fatti entrare all'interno del
 * supermercato. Successivamente, quando il numero di clienti nel supermercato
 * scende sotto la soglia C - E, ne vengono fatti entrare altri E.
 */
static void *creazione_clienti(void* arg) {
  /* Disabilita temporaneamente la cancellazione del thread per evitare che
   * qualche cancellation point sfuggito all'analisi venga eseguito prima
   * di inserire le routine di cleaup.
   */
  pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

  struct t_info *info = (struct t_info*) arg;
  const config_t *config = info->config;
  supermercato_t *supermercato = info->supermercato;

  size_t max_clienti = config->params[C];
  int p = config->params[P];
  int t = config->params[T];
  int e = config->params[E];
  assert(max_clienti > 0);

  cliente_t *cliente;
  threadpool_t *tpool = threadpool_create(max_clienti);
  threadpool_job_t *tjob;

  /* Riabilita la cancellazione del thread */
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

  /* Creazione iniziale di C clienti */
  for (uint i=0; i<max_clienti; i++) {
    /* crea un nuovo cliente */
    cliente = generate_cliente(p, t, supermercato);
    /* sottomette il job cliente alla threadpool */
    tjob = threadpool_job_create(
        cliente_worker, /* job */
        (void*) cliente,  /* argomento del job */
        (void (*)(void*)) free_cliente); /* cleanup routine */
    threadpool_add(tpool, tjob);
  }

  while (!quit_thread()) {
    /* Attesa condizionata sul numero di clienti all'interno del supermercato */
    pthread_mutex_lock_safe(&tpool->mtx);
    while (tpool->job_count > max_clienti - e && !quit_thread()) {
      /* Registra la routine di pulizia chiamata a seguito di pthread_cancel().
       * Nota: L'ordine di esecuzione degli handler di cleanup è inverso rispetto
       * all'ordine di inserimento (ordine LIFO). */
      pthread_cleanup_push(cleanup, (void*) tpool);

      /* pthread_cancel() ha effetto solo se il thread al momento si trova in un
       * cancellation point. L'unico cancellation point del thread è la funziona
       * pthread_cond_wait(), la quale a seguito della chiamata a pthread_cancel()
       * risveglia il thread con il mutex tpool->mtx acquisito e passa il controllo
       * all'ultimo cleanup handler inserito. Di conseguenza è necessario
       * rilasciare il mutex inserendo come routine di cleanup il wrapper
       * pthread_mutex_unlock_safe() con argomento tpool->mtx.
       */
      pthread_cleanup_push((void (*)(void*)) pthread_mutex_unlock_safe,
          (void*) &tpool->mtx);

      pthread_cond_wait(&tpool->not_full_cond, &tpool->mtx); /* cancellation point */

      pthread_cleanup_pop(0); /* unlock tpool->mtx: non eseguire */
      pthread_cleanup_pop(0); /* cleanup:           non eseguire*/
    }
    pthread_mutex_unlock_safe(&tpool->mtx);

    /* Termina l'esecuzione se è stata segnalata la chiusura */
    if (quit_thread()) {
      break;
    }

    /* Creazione scaglionata di E clienti per volta */
    for (int i=0; i<e; i++) {
      /* crea un nuovo cliente */
      cliente = generate_cliente(p, t, supermercato);
      /* sottomette il job cliente alla threadpool */
      tjob = threadpool_job_create(
          cliente_worker, /* job */
          (void*) cliente,  /* argomento del job */
          (void (*)(void*)) free_cliente); /* cleanup routine */
      threadpool_add(tpool, tjob);
    }
  }

  /* Disabilita la cancellazione perchè threadpool_free() potrebbe contenere
   * cancellation point.
   */
  pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
  /* Deallocazione risorse usate dalla threadpool */
  threadpool_free(tpool);

  return (void*)0;
}

int main() {
  clock_t start, end;
  //TODO: gestione errori
  //TODO: passaggio path file di configurazione come parametro
  config_t config;
  parse_config("config.txt", &config);

  pthread_mutex_init_ec(&deallocate_mtx, NULL);

  /* Crea il supermercato */
  supermercato_t *s = create_supermercato(config.params[K], config.params[I], config.params[TP]);
  init_direttore(s, config.params[S1], config.params[S2]);
  struct t_info info = { s, &config };

  sleep(1); /* attende un secondo */ 
  start = time(NULL);

  pthread_create(&create_thread, NULL, creazione_clienti, (void*)&info);

  sleep(6);

  printf("Supermercato in chiusura \n");

  /* terminazione */
  set_quit(1); /* quit threads */
  close_supermercato(s); /* chiude il supermercato e i cassieri */
  pthread_join(create_thread, NULL); /* termina il thread di creazione dei clienti */
  printf("THREAD JOINED\n");
  terminate_direttore(); /* termina il thread direttore */
  free_supermercato(s); /* libera la memoria allocata dai cassieri */

  printf("Supermercato chiuso \n");

  end = time(NULL);
  double time = (double)(end - start);
  printf("Tempo impiegato: %f s\n", time);


  exit(EXIT_SUCCESS);
}
