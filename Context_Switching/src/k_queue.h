#ifndef K_QUEUE_H
#define K_QUEUE_H

#include "k_node.h"

typedef struct k_queue_t {
    k_node_t *first;
    k_node_t *last;
} k_queue_t;

int enqueue_node(k_queue_t *queue, k_node_t *node);

k_node_t *dequeue_node(k_queue_t *queue);

int is_queue_empty(k_queue_t *queue);

#ifdef DEBUG_0
int print_queue(k_queue_t *queue);
#endif

#endif
