#include "supermercato.h"
#include "cassiere.h"
#include "cliente.h"
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <assert.h>

// static pthread_cond_t supermercato_not_full_cond;
// 

struct t_info {
  supermercato_t *supermercato;
  const config_t *config;
};


void* creazione_clienti(void* arg) {
  // pthread_cond_init(&supermercato_not_full_cond, NULL);

  struct t_info *info = (struct t_info*) arg;
  const config_t *config = info->config;
  supermercato_t *supermercato = info->supermercato;

  int max_clienti = config->params[C];
  int p = config->params[P];
  int t = config->params[T];
  assert(max_clienti > 0);

  cliente_t **clienti = (cliente_t**) malloc(sizeof(cliente_t*)*max_clienti);

  for (int i=0; i<max_clienti; i++) {
    int n = rand() % p; /* prodotti 0-20 */
    int dwell = 10 + rand() % (t - 10); /* dwell time 10-t ms */
 
    clienti[i] = create_cliente(dwell, n, supermercato);
    printf("Cliente creato...\n");
  }

  for (int i=0; i<max_clienti; i++) {
    free_cliente(clienti[i]);
  }

  free(clienti);

  return (void*)0;
}

/*
 * Note: la deallocazione dei clienti è gestita dal working thread del cliente.
 */
int main() {

  //TODO: gestione errori
  //TODO: passaggio path file di configurazione come parametro
  config_t config;
  parse_config("config.txt", &config);

  supermercato_t *s = create_supermercato(config.params[K], config.params[I], config.params[TP]);
  struct t_info info = { s, &config };

  clock_t start, end;
  start = time(NULL);

  sleep(1); /* attende un secondo */ 

  pthread_t thread_id;
  pthread_create(&thread_id, NULL, &creazione_clienti, (void*)&info);

  //TEST: implementare interfaccia in supermercato
  sleep(1);
  pthread_mutex_lock(&s->cassieri_mtx);
  printf("chiudendo cassa 0...\n");
  close_cassa(&s->cassieri[0]); /* chiudi la cassa 0: reindirizza i clienti nelle altre casse */
  s->num_casse--;
  pthread_mutex_unlock(&s->cassieri_mtx);

  sleep(2);
  pthread_mutex_lock(&s->cassieri_mtx);
  printf("chiudendo cassa 1...\n");
  close_cassa(&s->cassieri[1]); /* chiudi la cassa 0: reindirizza i clienti nelle altre casse */
  s->num_casse--;
  pthread_mutex_unlock(&s->cassieri_mtx);

  sleep(1); /* attende */ 

  printf("Supermercato in chiusura \n");
  close_supermercato(s);
  pthread_join(thread_id, NULL);
  free_supermercato(s);
  printf("Supermercato chiuso \n");

  end = time(NULL);
  double time = (double)(end - start);
  printf("Tempo impiegato: %f s\n", time);


  exit(EXIT_SUCCESS);
}
