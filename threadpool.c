#include "threadpool.h"
#include "defines.h"
#include "queue.h"
#include <assert.h>

/*
 * Rilascia la memoria allocata da un job e chiama la routine di cleanup
 * ad esso associato.
 */
static void threadpool_job_free(threadpool_job_t *job) {
  assert(job != NULL);
  if (job->cleanup != NULL) {
    job->cleanup(job->arg);
  }
  free(job);
}

/*
 * Thread di lavoro dei job sottomessi alla thread pool.
 */
static void *job_worker(void *arg) {
  threadpool_t *tp = (threadpool_t*)arg;

  pthread_mutex_lock_safe(&tp->mtx);

  while (!tp->stopped) {

    while (!tp->stopped && queue_empty(tp->jobs)) {
      pthread_cond_wait(&tp->not_empty_cond, &tp->mtx);
    }

    /* mutex locked */
    if (tp->stopped) {
      break;
    }

    assert(!queue_empty(tp->jobs));
    threadpool_job_t *job = (threadpool_job_t*) queue_pop(tp->jobs);

    /* mutex unlocked */
    pthread_mutex_unlock_safe(&tp->mtx);
    /* esegui job */
    job->f(job->arg); /* chiamata possibilmente bloccante: necessario rilasciare lock */
    /* rilascia la memoria allocata dal job */
    threadpool_job_free(job);

    /* mutex locked */
    pthread_mutex_lock_safe(&tp->mtx);
    tp->job_count--;
    pthread_cond_signal(&tp->not_full_cond);
  }

  assert(tp->stopped);
  pthread_cond_signal(&tp->not_full_cond);
  pthread_mutex_unlock_safe(&tp->mtx);
  pthread_exit(0);
}


/*
 * Alloca un elemento threadpool_job_t e vi associa il funzionale 'f' con
 * argomento 'arg'.
 */
threadpool_job_t *threadpool_job_create(
    void *(*f)(void*), 
    void *arg,
    void (*cleanup)(void*)) {
  threadpool_job_t *tjob = (threadpool_job_t*) malloc(sizeof(threadpool_job_t));
  if (tjob == NULL) {
    handle_error("thredpool_job_create malloc");
  }

  assert (f != NULL);

  tjob->f = f;
  tjob->arg = arg;
  tjob->cleanup = cleanup;
  return tjob;
}

/*
 * Crea una threadpool di dimensione fissa pari a 'size'.
 */
threadpool_t *threadpool_create(size_t size) {
  threadpool_t *tp = (threadpool_t*) malloc(sizeof(threadpool_t));
  pthread_t *threads = (pthread_t*) malloc(sizeof(pthread_t)*size);

  if (tp == NULL || threads == NULL) {
    handle_error("threadpool_create malloc");
  }
  if (size <= 0) {
    return NULL;
  }

  tp->size = size;
  tp->job_count = 0;
  tp->jobs = queue_create();
  tp->threads = threads;
  tp->stopped = 0;
  pthread_mutex_init_ec(&tp->mtx, NULL);
  pthread_cond_init(&tp->not_full_cond, NULL);
  pthread_cond_init(&tp->not_empty_cond, NULL);

  for (uint i=0; i<size; i++) {
    pthread_create(&threads[i], NULL, &job_worker, (void*)tp);
  }

  return tp;
}

/*
 * Aggiunge un job alla coda della threadpool e notifica i thread attivi in
 * ascolto della presenza di un nuovo lavoro.
 */
void threadpool_add(threadpool_t *tp, threadpool_job_t *job) {
  pthread_mutex_lock_safe(&tp->mtx);

  queue_push(tp->jobs, (void*)job);
  tp->job_count++;
  pthread_cond_broadcast(&tp->not_empty_cond);

  pthread_mutex_unlock_safe(&tp->mtx);
}

/*
 * Attende che vi siano al più max_jobs task sottomessi o in svolgimento.
 */
void threadpool_wait(threadpool_t *tp, size_t max_jobs) {
  pthread_mutex_lock_safe(&tp->mtx);

  while (tp->job_count > max_jobs) {
    pthread_cond_wait(&tp->not_full_cond, &tp->mtx);
  }

  pthread_mutex_unlock_safe(&tp->mtx);
}

/*
 * Termina e disalloca i thread e le risorse della pool.
 * La chiamata a threadpool_free() è bloccante, in quanto attende la
 * terminazione di tutti i thread creati.
 */
void threadpool_free(threadpool_t *tp) {
  pthread_mutex_lock_safe(&tp->mtx);
  tp->stopped = 1;
  pthread_cond_broadcast(&tp->not_empty_cond);
  pthread_mutex_unlock_safe(&tp->mtx);

  for(uint i=0; i<tp->size; i++) {
    if (pthread_join(tp->threads[i], NULL) != 0) {
      handle_error("threadpool_free: pthread_join");
    }
  }
  queue_map((void (*)(void*))threadpool_job_free, tp->jobs);
  queue_free(tp->jobs);
  free(tp->threads);
  free(tp);
}

