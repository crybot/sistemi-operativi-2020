#ifndef _SUPERMERCATO_H_
#define _SUPERMERCATO_H_
#include "cassiere.h"
#include <pthread.h>

//TODO: change to forward declaration for cassiere_t

// Contiene i dati relativi al supermercato.
typedef struct supermercato {
  unsigned int max_casse; /* massimo numero di casse attive */
  unsigned int num_casse; /* numero di casse attive */
  /* TODO: l'accesso all'insieme dei cassieri DEVE essere sincronizzato */
  pthread_mutex_t cassieri_mtx;
  cassiere_t *cassieri;   /* riferimenti ai cassieri del supermercato */
}supermercato_t;

extern supermercato_t *create_supermercato(int max_casse, int num_casse, int tempo);
extern void close_supermercato(supermercato_t *supermercato);
extern void free_supermercato(supermercato_t *supermercato);
extern cassiere_t *place_cliente(cliente_t *cliente, supermercato_t *supermercato, unsigned int *seed);
extern cassiere_t *open_cassa_supermercato(supermercato_t *supermercato);
extern cassiere_t *close_cassa_supermercato(supermercato_t *supermercato);

#endif

