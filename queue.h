#ifndef QUEUE_H
#define QUEUE_H
#include <stdlib.h> /* incluso per size_t */

typedef struct queue_node {
  struct queue_node *next;
  void *value;
}queue_node_t;

typedef struct queue {
  queue_node_t *head;
  queue_node_t *tail;
}queue_t;


extern queue_t *queue_create();
extern int queue_empty(queue_t *queue);
extern void queue_push(queue_t *queue, void *value);
extern void *queue_pop(queue_t *queue);
extern size_t queue_size(queue_t *queue);

#endif
