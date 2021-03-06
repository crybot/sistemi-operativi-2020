#ifndef _CASSIERE_H_
#define _CASSIERE_H_
#include <pthread.h>
#include <stdlib.h>
#include "queue.h"
#include "cliente.h"

/* Contiene le informazioni relative a un cassiere di un supermercato */
typedef struct cassiere {
  pthread_t thread; /* thread di lavoro del cassiere */
  uint id;          /* id univoco del cassiere */
  int active;       /* indica se la cassa è aperta (0 chiusa, != 0 aperta) */
  int closing;      /* indica se la cassa è in chiusura (!= 0 in chiusura, 0 altrimenti) */
  int allocated;    /* indica se il thread cassiere è stato creato */
  int tp;           /* tempo di gestione del singolo prodotto dal cassiere */
  int s;            /* intervallo di comunicazione con il direttore */
  /* synchronized fields*/
  pthread_mutex_t mtx; /* mutex per la sincronizzazione dello stato */
  pthread_cond_t not_empty_cond;
  queue_t *clienti; /* clienti in coda alla cassa */
  /* statistics */
  int clienti_serviti;  /* numero di clienti serviti */
  int numero_chiusure;  /* numero di chisusure della cassa */
  int prodotti_venduti; /* numero di prodotti venduti dal cassiere */
  int tempo_totale;     /* tempo totale di apertura */
  long tempo_medio;      /* tempo medio di servizio clienti */
}cassiere_t;

int cassa_id(const cassiere_t *cassiere);
int is_cassa_active(cassiere_t *cassiere);
int is_cassa_closing(cassiere_t *cassiere);
void init_cassiere(cassiere_t *cassiere, int tp, int s);
int open_cassa(cassiere_t *cassiere);
int close_cassa(cassiere_t *cassiere);
void wait_cassa(cassiere_t *cassiere);
void add_cliente(cassiere_t *cassiere, cliente_t *cliente);

#endif
