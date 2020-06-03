#include "../queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int main(int argc, char *argv[]) {
  assert(argc > 1);

  queue_t *queue = queue_create();

  assert(queue_empty(queue));

  for (int i=1; i<argc; i++) {
    queue_push(queue, (void*)argv[i]);
    assert(!queue_empty(queue));
    assert(queue->head != NULL);
    assert(queue->tail != NULL);
    assert(queue->tail->value == (void*)argv[i]);
    assert(queue_size(queue) == (size_t) i);
  }

  for (int i=1; i<argc; i++) {
    assert(!queue_empty(queue));
    assert(queue->head != NULL);
    assert(queue->tail != NULL);
    assert(queue_top(queue) == (void*)argv[i]);
    assert(queue_pop(queue) == (void*)argv[i]);
    assert(queue_size(queue) == (size_t)(argc - i - 1));
  }

  assert(queue_empty(queue));
  exit(EXIT_SUCCESS);
}
