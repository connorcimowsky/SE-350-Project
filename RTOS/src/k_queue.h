#ifndef K_QUEUE_H
#define K_QUEUE_H

#include "k_node.h"


typedef struct k_queue_t {
    k_node_t *mp_first;
    k_node_t *mp_last;
} k_queue_t;


int enqueue_node(k_queue_t *p_queue, k_node_t *p_node);

int queue_sorted_insert(k_queue_t *p_queue, k_node_t *p_node);

k_node_t *queue_peek(k_queue_t *p_queue);

k_node_t *dequeue_node(k_queue_t *p_queue);

int remove_node_from_queue(k_queue_t *p_queue, k_node_t *p_node);

int queue_contains_node(k_queue_t *p_queue, k_node_t *p_node);

int is_queue_empty(k_queue_t *p_queue);

#ifdef DEBUG_1

int print_queue(k_queue_t *p_queue);

#endif /* DEBUG_1 */


#endif /* K_QUEUE_H */
