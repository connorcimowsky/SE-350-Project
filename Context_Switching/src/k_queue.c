#include <stdlib.h>

#ifdef DEBUG_0
#include "printf.h"
#endif
#include "k_queue.h"

k_queue_t *new_queue(void)
{
    // TODO(ConnorCimowsky): Use request_memory_block instead of malloc.
    k_queue_t *queue = (k_queue_t *)malloc(sizeof(k_queue_t));
    queue->first = NULL;
    queue->last = NULL;
    
    return queue;
}

int enqueue_node(k_queue_t *queue, k_node_t *node)
{
    if (queue == NULL || node == NULL) {
				#ifdef DEBUG_0
				printf("Node insertion error.\n");
				#endif
        return 1;
    }
    
    node->next = NULL;
    
    if (is_queue_empty(queue)) {
        queue->first = node;
        queue->last = node;
    } else {
        queue->last->next = node;
        queue->last = queue->last->next;
    }
    
    return 0;
}

k_node_t *dequeue_node(k_queue_t *queue)
{
    k_node_t *first = NULL;
    
    if (is_queue_empty(queue)) {
				#ifdef DEBUG_0
				printf("Attempted to dequeue a node from an empty queue.\n");
				#endif
        return NULL;
    }
    
    first = queue->first;
    
    if (queue->first == queue->last) {
        queue->first = NULL;
        queue->last = NULL;
    } else {
        queue->first = queue->first->next;
    }
    
    return first;
}

int is_queue_empty(k_queue_t *queue) {
    if (queue == NULL) {
				#ifdef DEBUG_0
				printf("Attempted to check if a NULL queue was empty.\n");
				#endif
        return -1;
    } else {
        return (queue->first == NULL);
    }
}
