#ifndef K_QUEUE_H
#define K_QUEUE_H

#include "node.h"


typedef struct queue_t {
    node_t *mp_first;
    node_t *mp_last;
} queue_t;


int enqueue_node(queue_t *p_queue, node_t *p_node);

int queue_sorted_insert(queue_t *p_queue, node_t *p_node);

node_t *queue_peek(queue_t *p_queue);

node_t *dequeue_node(queue_t *p_queue);

int remove_node_from_queue(queue_t *p_queue, node_t *p_node);

int queue_contains_node(queue_t *p_queue, node_t *p_node);

int is_queue_empty(queue_t *p_queue);

#ifdef DEBUG_1

int print_queue(queue_t *p_queue);

#endif /* DEBUG_1 */


#endif /* K_QUEUE_H */
