#ifndef QUEUE_H
#define QUEUE_H
#include <stdlib.h> /* size_t */
#include <pthread.h> /* pthread_mutex_t */

typedef struct queue_node {
  struct queue_node *next;
  void *value;
}queue_node_t;

typedef struct queue {
  queue_node_t *head;
  queue_node_t *tail;
  pthread_mutex_t mtx;
  pthread_cond_t not_empty_cond;
  //TODO: aggiungere cv not_full_cond
}queue_t;


queue_t *queue_create(void);
int queue_empty(queue_t *queue);
void queue_push(queue_t *queue, void *value);
void *queue_pop(queue_t *queue);
void *queue_top(queue_t *queue);
size_t queue_size(queue_t *queue);
void queue_free(queue_t *queue);
void queue_map(void (*f)(void*), queue_t *queue);
void queue_wait(queue_t *queue);

#endif
