#ifndef QUEUE_H
#define QUEUE_H

#include "node.h"

typedef struct queue_t {
    node_t *first;
    node_t *last;
} queue_t;

queue_t *new_queue(void);

int enqueue_node(queue_t *queue, node_t *node);

node_t *dequeue_node(queue_t *queue);

int is_queue_empty(queue_t *queue);

#endif
