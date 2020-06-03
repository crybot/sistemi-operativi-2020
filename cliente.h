#ifndef _CLIENTE_H
#define _CLIENTE_H

#include <pthread.h>

struct supermercato;
struct cassiere;

//TODO: aggiungere id cliente per logging e analisi
typedef struct cliente {
  pthread_t thread_id; /* thread di lavoro del cliente */
  int dwell_time; /* tempo impiegato per scegliere i prodotti */
  int products;   /* numero di prodotti comprati */
  int servito;    /* 1 se servito, 0 altrimenti */
  struct cassiere *cassiere;   /* cassa in cui il cliente è in coda */
  struct supermercato *supermercato; /* riferimento al supermercato */
}cliente_t;

cliente_t* create_cliente(
    int dwell_time,
    int products, 
    struct supermercato *supermercato);

void free_cliente(cliente_t* cliente);

#endif

