#include "queue.h"
#include "defines.h"
#include <stdlib.h>
#include <assert.h>

queue_t *queue_create() {
  queue_t *queue = (queue_t *) malloc(sizeof(queue_t));
  if (queue == NULL) {
    handle_error("malloc queue_create");
  }

  queue->head = NULL;
  queue->tail = NULL;
  return queue;
}

int queue_empty(queue_t *queue) {
  if (queue == NULL) {
    handle_error("queue_empty: queue is NULL");
  }

  return queue->head == NULL;
}

/*
 * Restituisce il numero di elementi contenuti nella coda.
 */
size_t queue_size(queue_t *queue) {
  assert(queue != NULL);
  if (queue == NULL) {
    handle_error("queue_size: queue is NULL");
  }

  if (queue_empty(queue)) {
    return 0;
  }

  size_t n = 0;
  queue_node_t *node = queue->head;
  while (++n, (node = node->next) != NULL);
  return n;
}

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

  if (queue_empty(queue)) {
    queue->head = node;
    queue->tail = node;
    node->next = NULL;
  }
  else {
    assert(queue->head != NULL);
    assert(queue->tail != NULL);
    node->next = queue->head;
    queue->head = node;
  }
}

/*
 * Restituisce il valore del primo elemento della coda.
 * Se la coda è vuota restituisce NULL;
 */
void* queue_pop(queue_t *queue) {
  assert(queue != NULL);
  if (queue == NULL) {
    handle_error("queue_pop: queue is NULL");
  }

  if (queue_empty(queue)) {
    return NULL;
  }

  assert(queue->head != NULL);

  queue_node_t *node = queue->head;
  void* value = node->value;
  queue->head = node->next;

  free(node);

  return value;
}
