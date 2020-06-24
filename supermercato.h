#ifndef _SUPERMERCATO_H_
#define _SUPERMERCATO_H_
#include "cassiere.h"
#include <pthread.h>

/* Contiene i dati relativi al supermercato. */
typedef struct supermercato {
  unsigned int max_casse; /* massimo numero di casse attive */
  unsigned int num_casse; /* numero di casse attive */
  pthread_mutex_t cassieri_mtx;
  cassiere_t *cassieri;   /* riferimenti ai cassieri del supermercato */
}supermercato_t;

typedef struct config config_t;

supermercato_t *create_supermercato(const config_t *config);
void close_supermercato(supermercato_t *supermercato);
void free_supermercato(supermercato_t *supermercato);
cassiere_t *place_cliente(cliente_t *cliente, supermercato_t *supermercato, unsigned int *seed);
cassiere_t *open_cassa_supermercato(supermercato_t *supermercato);
cassiere_t *close_cassa_supermercato(supermercato_t *supermercato);

#endif

