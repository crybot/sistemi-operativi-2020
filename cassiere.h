#ifndef _CASSIERE_H_
#define _CASSIERE_H_
#include <pthread.h>
#include <stdlib.h>
#include "queue.h"

/* Contiene le informazioni relative a un cassiere di un supermercato */
typedef struct cassiere {
  pthread_t thread; /* thread di lavoro del cassiere */
  uint id;          /* id univoco del cassiere */
  int active;       /* indica se la cassa è aperta (0 chiusa, != 0 aperta) */
  /* synchronized fields*/
  queue_t *clienti; /* clienti in coda alla cassa */
}cassiere_t;

extern int cassa_id(cassiere_t *cassiere);
extern int is_cassa_active(cassiere_t *cassiere);
extern void init_cassiere(cassiere_t *cassiere);
extern int open_cassa(cassiere_t *cassiere);
extern int close_cassa(cassiere_t *cassiere);
extern void wait_cassa(cassiere_t *cassiere);

#endif
