#include <stdlib.h>

#ifdef DEBUG_0
#include "printf.h"
#endif
#include "queue.h"

queue_t *new_queue(void)
{
    // TODO(ConnorCimowsky): Use request_memory_block instead of malloc.
    queue_t *queue = (queue_t *)malloc(sizeof(queue_t));
    queue->first = NULL;
    queue->last = NULL;
    
    return queue;
}

int enqueue_node(queue_t *queue, node_t *node)
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

node_t *dequeue_node(queue_t *queue)
{
    node_t *first = NULL;
    
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

int is_queue_empty(queue_t *queue) {
    if (queue == NULL) {
				#ifdef DEBUG_0
				printf("Attempted to check if a NULL queue was empty.\n");
				#endif
        return -1;
    } else {
        return (queue->first == NULL);
    }
}