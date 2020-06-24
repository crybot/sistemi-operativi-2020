#ifndef UTILS_H
#define UTILS_H
#include <pthread.h>

static pthread_mutex_t seed_mtx = PTHREAD_MUTEX_INITIALIZER;

/*
 * Genera un numero intero incrementale tale che, per due chiamate successive
 * s1 = safe_seed() e s2 = safe_seed(), s2 == s1 + 1.
 * L'accesso alla funzione safe_seed è thread-safe in quanto è sincronizzato da
 * un mutex.
 * La funzione è dichiarata statica e definita nell'header in modo da permettere
 * ad ogni file che includono quest'ultimo di ottenere la propria copia dello 
 * stato interno di utils.h. In questo modo ogni thread generato da una singola
 * translation unit riceverà seed univoci, ma non influenzeranno la generazione
 * di seed effettuata dai thread di altre translatio unit.
 */
static inline unsigned int safe_seed(void) {
  static unsigned int seed = 0;
  pthread_mutex_lock(&seed_mtx);
  int s = seed++;
  pthread_mutex_unlock(&seed_mtx);
  return s;
}



#endif
