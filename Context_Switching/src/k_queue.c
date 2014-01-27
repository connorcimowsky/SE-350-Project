#include "rtx.h"
#ifdef DEBUG_0
#include "printf.h"
#endif
#include "k_queue.h"

int enqueue_node(k_queue_t *queue, k_node_t *node)
{
    if (queue == NULL || node == NULL) {
    
#ifdef DEBUG_0
        printf("Node insertion error.\n");
#endif
    
        return RTX_ERR;
    }
    
    node->next = NULL;
    
    if (is_queue_empty(queue)) {
        queue->first = node;
        queue->last = node;
    } else {
        queue->last->next = node;
        queue->last = queue->last->next;
    }
    
    return RTX_OK;
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

int remove_node_from_queue(k_queue_t *queue, k_node_t *node)
{
    k_node_t *current_iter = NULL;
    k_node_t *previous_iter = NULL;
    
    if (is_queue_empty(queue)) {
    
#ifdef DEBUG_0
        printf("Attempted to remove a node from an empty queue.\n");
#endif
    
        return RTX_ERR;
    }
    
    if (node == queue->first) {
        if (queue->first == queue->last) {
            queue->first = NULL;
            queue->last = NULL;
        } else {
            queue->first = queue->first->next;
        }
        
        return RTX_OK;
    }
    
    current_iter = queue->first->next;
    previous_iter = queue->first;
    while (current_iter != NULL && previous_iter != NULL) {
        if (node == current_iter) {
            if (current_iter == queue->last) {
                queue->last = previous_iter;
            }
            previous_iter->next = current_iter->next;
            return RTX_OK;
        }
        previous_iter = current_iter;
        current_iter = current_iter->next;
    }
    
#ifdef DEBUG_0
        printf("Node not found.\n");
#endif
    
    return RTX_ERR;
}

int queue_contains_node(k_queue_t *queue, k_node_t *node)
{
    k_node_t *iter = NULL;
    
    if (is_queue_empty(queue)) {
    
#ifdef DEBUG_0
        printf("Attempted to check node membership for an empty queue.\n");
#endif
    
        return RTX_ERR;
    }
    
    iter = queue->first;
    while (iter != NULL) {
        if (iter == node) {
            return 1;
        }
        iter = iter->next;
    }
    
    return 0;
}

int is_queue_empty(k_queue_t *queue) {
    if (queue == NULL) {
    
#ifdef DEBUG_0
        printf("Attempted to check if a NULL queue was empty.\n");
#endif
    
        return RTX_ERR;
    } else {
        return (queue->first == NULL);
    }
}

#ifdef DEBUG_0
int print_queue(k_queue_t *queue) {
    k_node_t *iter = NULL;
    if (queue == NULL) {
        return RTX_ERR;
    }
    
    iter = queue->first;
    printf("queue: 0x%x\n\tfirst:0x%x\n\tlast:0x%x\n", queue, queue->first, queue->last);
    printf("nodes:\n");
    while (iter != NULL) {
        printf("\t0x%x, next: 0x%x\n", iter, iter->next);
        iter = iter->next;
    }
    
    return RTX_OK;
}
#endif
