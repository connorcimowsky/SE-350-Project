#ifndef K_QUEUE_H
#define K_QUEUE_H

#include "node.h"


typedef struct queue_t {
    node_t *mp_first;
    node_t *mp_last;
} queue_t;


int queue_init(queue_t *p_queue);

int enqueue(node_t *p_node, queue_t *p_queue);

int sorted_enqueue(node_t *p_node, queue_t *p_queue);

node_t *dequeue(queue_t *p_queue);

int remove_from_queue(node_t *p_node, queue_t *p_queue);

int queue_contains_node(queue_t *p_queue, node_t *p_node);

int is_queue_empty(queue_t *p_queue);

#ifdef DEBUG_1

int print_queue(queue_t *p_queue);

#endif /* DEBUG_1 */


#endif /* K_QUEUE_H */
