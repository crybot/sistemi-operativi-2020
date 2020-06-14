#include "../threadpool.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
int count = 0;

void* job() {
  pthread_mutex_lock(&mtx);
  count++;
  pthread_mutex_unlock(&mtx);

  return (void*)0;
}

int main(int argc, char *argv[]) {
  assert(argc > 1);

  int n = atoi(argv[1]);
  assert(n > 1);

  threadpool_t *tp = threadpool_create(5);
  for (int i=0; i<n; i++) {
    threadpool_job_t *pool_job = threadpool_job_create(
        (void* (*)(void*)) job,
        NULL,
        NULL);
    threadpool_add(tp, pool_job);
  }

  threadpool_wait(tp, 0);
  threadpool_free(tp);
  assert(count == n);
  // printf("COUNT: %d", count);

  exit(EXIT_SUCCESS);
}
