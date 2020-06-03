#include "queue.h"
#include "defines.h"
#include <stdlib.h>
#include <assert.h>
/*
 * Il file queue.c implementa l'interfaccia verso la api definita nell'header
 * queue.h. La coda implementata è unidirezionale e implementa le funzionalità
 * di push, pop, top, size e empty, ognuna descritta dalle rispettive
 * implementazioni.
 * queue_t è thread-safe nelle sue operazioni di base, che garantiscono
 * l'esecuzione atomica delle istruzioni contenute. Per questo motivo la coda
 * è sempre passata alle funzioni come puntatore (a dati non costanti) poichè
 * il lock è contenuto all'interno di queue_t ed è unico per ogni coda.
 */

/*
 * Crea una coda vuota.
 * Restituisce: un puntatore alla coda creata.
 *
 * POST: queue_empty(queue_create()) == TRUE
 */
queue_t *queue_create() {
  queue_t *queue = (queue_t *) malloc(sizeof(queue_t));
  if (queue == NULL) {
    handle_error("malloc queue_create");
  }

  queue->head = NULL;
  queue->tail = NULL;
  pthread_mutex_init(&queue->mtx, NULL);
  return queue;
}

/*
 * Determina se la coda in ingresso è vuota.
 * Restituisce: un valore != 0 se la coda è vuota, 0 altrimenti.
 */
int queue_empty(queue_t *queue) {
  if (queue == NULL) {
    handle_error("queue_empty: queue is NULL");
  }

  pthread_mutex_lock(&queue->mtx);
  int empty = queue->head == NULL;
  pthread_mutex_unlock(&queue->mtx);

  return empty;
}

/*
 * Inserisce un nuovo elemento in fondo alla coda.
 */
void queue_push(queue_t *queue, void *value) {
  assert(queue != NULL);
  if (queue == NULL) {
    handle_error("queue_push: queue is NULL");
  }

  queue_node_t *node = (queue_node_t *) malloc(sizeof(queue_node_t));
  if (node == NULL) {
    handle_error("malloc queue_push");
  }

  node->value = value;
  node->next = NULL;

  /* Per verificare che la coda sia vuota non si può usare la funzione
   * queue_empty() poichè internamente questa ottiene il lock associato alla
   * coda, quindi per ottenere l'accesso esclusivo, la funzione corrente
   * dovrebbe ottenere il lock all'interno del corpo dell'if (per evitare
   * situazioni di deadlock). In questo caso però il thread corrente potrebbe
   * essere stato prerilasciato dallo scheduler e la condizione di coda vuota
   * potrebbe non essere più verificata. 
   */
  /* Sezione critica */
  pthread_mutex_lock(&queue->mtx); /* lock */
  if (queue->head == NULL) { /* coda vuota */
    queue->head = node;
    queue->tail = node;
  }
  else {
    assert(queue->head != NULL);
    assert(queue->tail != NULL);
    queue->tail->next = node;
    queue->tail = node;
  }
  pthread_mutex_unlock(&queue->mtx); /* unlock */
}

/*
 * Restituisce il valore del primo elemento della coda, estraendolo.
 * Se la coda è vuota restituisce NULL;
 */
void* queue_pop(queue_t *queue) {
  assert(queue != NULL);
  if (queue == NULL) {
    handle_error("queue_pop: queue is NULL");
  }

  /* sezione critica */
  pthread_mutex_lock(&queue->mtx); /* lock */

  if (queue->head == NULL) { /* coda vuota */
    pthread_mutex_unlock(&queue->mtx); /* unlock */
    return NULL;
  }

  assert(queue->head != NULL);

  queue_node_t *node = queue->head;
  void* value = node->value;
  queue->head = node->next;

  pthread_mutex_unlock(&queue->mtx); /* unlock */

  free(node);

  return value;
}

/*
 * Restituisce il valore del primo elemento della coda, senza estrarlo.
 * Se la coda è vuota restituisce NULL;
 */
void* queue_top(queue_t *queue) {
  assert(queue != NULL);
  if (queue == NULL) {
    handle_error("queue_top: queue is NULL");
  }

  /* sezione critica */
  pthread_mutex_lock(&queue->mtx); /* lock */

  if (queue->head == NULL) { /* coda vuota */
    pthread_mutex_unlock(&queue->mtx); /* unlock */
    return NULL;
  }

  void *value = queue->head->value;
  pthread_mutex_unlock(&queue->mtx); /* unlock */

  return assert(value != NULL), value;
}

/*
 * Restituisce il numero di elementi contenuti nella coda.
 */
size_t queue_size(queue_t *queue) {
  assert(queue != NULL);
  if (queue == NULL) {
    handle_error("queue_size: queue is NULL");
  }

  /* sezione critica */
  pthread_mutex_lock(&queue->mtx); /* lock */

  if (queue->head == NULL) {
    pthread_mutex_unlock(&queue->mtx); /* unlock */
    return 0;
  }

  size_t n = 0;
  queue_node_t *node = queue->head;
  while (++n, (node = node->next) != NULL);

  pthread_mutex_unlock(&queue->mtx); /* unlock */
  return n;
}

/*
 * Libera la memoria allocata dalla coda chiamando ripetutamente queue_pop().
 */
void queue_free(queue_t *queue) {
  while (!queue_empty(queue)) {
    queue_pop(queue);
  }
  assert(queue->head == NULL);
  free(queue);
}

/*
 * Applica una funzione a tutti gli elementi della coda.
 */
void queue_map(void (*f)(void*), queue_t *queue) {
  assert(queue != NULL);

  pthread_mutex_lock(&queue->mtx); /* lock */

  if (queue->head == NULL) { /* coda vuota */
    pthread_mutex_unlock(&queue->mtx); /* unlock */
    return;
  }
  queue_node_t *node = queue->head;

  /* itera gli elementi della coda */
  while(node != NULL) {
    (*f)(node->value); /* applica la funzione all'elemento corrente della coda */
    node = node->next;
  }

  pthread_mutex_unlock(&queue->mtx); /* unlock */
}
