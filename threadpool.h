#ifndef THREADPOOL_H
#define THREADPOOL_H
#include <stdlib.h> /* size_t */
#include <pthread.h>

/*
 * Implementazione posix-compliant di una thread pool di dimensione fissa.
 */

struct queue;

typedef struct threadpool {
  size_t size;
  size_t job_count;
  struct queue *jobs;
  pthread_mutex_t mtx;
  pthread_cond_t not_full_cond;
  pthread_cond_t not_empty_cond;
  pthread_t *threads;
  int stopped;
}threadpool_t;

typedef struct threadpool_job {
  void *(*f)(void*);
  void *arg;
  void (*cleanup)(void*);
}threadpool_job_t;

threadpool_t *threadpool_create(size_t size);
threadpool_job_t *threadpool_job_create(void *(*f)(void*), void *arg, void (*cleanup)(void*));
void threadpool_add(threadpool_t *tp, threadpool_job_t *job);
void threadpool_wait(threadpool_t *tp, size_t max_jobs);
void threadpool_free(threadpool_t *tp);



#endif
