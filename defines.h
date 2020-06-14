#ifndef DEFINES_H
#define DEFINES_H
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>


#define handle_error(msg) \
  do { perror(msg); exit(EXIT_FAILURE); } while (0)

static inline int pthread_mutex_init_ec(pthread_mutex_t *mutex, void *arg) {
  (void)arg; /* Ignora il warning di inutilizzo del parametro */
  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
  pthread_mutex_init(mutex, &attr);
  return 0;
}

static inline void pthread_mutex_lock_safe(pthread_mutex_t *mutex) {
  if (pthread_mutex_lock(mutex) != 0) {
    handle_error("safe locking: pthread_mutex_lock");
  }
}

static inline void pthread_mutex_unlock_safe(pthread_mutex_t *mutex) {
  if (pthread_mutex_unlock(mutex) != 0) {
    handle_error("safe unlocking: pthread_mutex_unlock");
  }
}

#endif
