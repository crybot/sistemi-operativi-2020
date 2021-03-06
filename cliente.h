#ifndef _CLIENTE_H
#define _CLIENTE_H

#include <pthread.h>

struct supermercato;
struct cassiere;

typedef struct cliente {
  int id;         /* id univoco associato al cliente */
  int dwell_time; /* tempo impiegato per scegliere i prodotti */
  int products;   /* numero di prodotti comprati */
  int servito;    /* 1 se servito, 0 altrimenti */
  struct cassiere *cassiere;   /* cassa in cui il cliente è in coda */
  struct supermercato *supermercato; /* riferimento al supermercato */
  pthread_mutex_t mtx;
  pthread_cond_t servito_cond;
}cliente_t;

cliente_t* create_cliente(
    int dwell_time,
    int products, 
    struct supermercato *supermercato);

void free_cliente(cliente_t *cliente);
void *cliente_worker(void *arg);
void set_servito(cliente_t *cliente, int servito);

#endif

