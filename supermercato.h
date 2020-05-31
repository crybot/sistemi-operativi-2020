#ifndef _SUPERMERCATO_H_
#define _SUPERMERCATO_H_
#include "cassiere.h"

// Contiene i dati relativi al supermercato.
typedef struct supermercato {
  unsigned int max_casse; /* massimo numero di casse attive */
  unsigned int num_casse; /* numero di casse attive */
  cassiere_t *cassieri;   /* riferimenti ai cassieri del supermercato */
}supermercato_t;

extern supermercato_t *create_supermercato(int max_casse, int num_casse);
extern void close_supermercato(supermercato_t *supermercato);

#endif

