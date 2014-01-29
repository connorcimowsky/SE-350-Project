#include "k_queue.h"
#include "rtx.h"

#ifdef DEBUG_0
#include "printf.h"
#endif


int enqueue_node(k_queue_t *p_queue, k_node_t *p_node)
{
    if (p_queue == NULL || p_node == NULL) {
    
#ifdef DEBUG_0
        printf("Node insertion error.\n");
#endif
    
        return RTX_ERR;
    }
    
    p_node->mp_next = NULL;
    
    if (is_queue_empty(p_queue)) {
        p_queue->mp_first = p_node;
        p_queue->mp_last = p_node;
    } else {
        p_queue->mp_last->mp_next = p_node;
        p_queue->mp_last = p_queue->mp_last->mp_next;
    }
    
    return RTX_OK;
}

k_node_t *dequeue_node(k_queue_t *p_queue)
{
    k_node_t *p_first = NULL;
    
    if (is_queue_empty(p_queue)) {
    
#ifdef DEBUG_0
        printf("Attempted to dequeue a node from an empty queue.\n");
#endif
    
        return NULL;
    }
    
    p_first = p_queue->mp_first;
    
    if (p_queue->mp_first == p_queue->mp_last) {
        p_queue->mp_first = NULL;
        p_queue->mp_last = NULL;
    } else {
        p_queue->mp_first = p_queue->mp_first->mp_next;
    }
    
    return p_first;
}

int remove_node_from_queue(k_queue_t *p_queue, k_node_t *p_node)
{
    k_node_t *p_current_iter = NULL;
    k_node_t *p_previous_iter = NULL;
    
    if (is_queue_empty(p_queue)) {
    
#ifdef DEBUG_0
        printf("Attempted to remove a node from an empty queue.\n");
#endif
    
        return RTX_ERR;
    }
    
    if (p_node == p_queue->mp_first) {
        if (p_queue->mp_first == p_queue->mp_last) {
            p_queue->mp_first = NULL;
            p_queue->mp_last = NULL;
        } else {
            p_queue->mp_first = p_queue->mp_first->mp_next;
        }
        
        return RTX_OK;
    }
    
    p_current_iter = p_queue->mp_first->mp_next;
    p_previous_iter = p_queue->mp_first;
    while (p_current_iter != NULL && p_previous_iter != NULL) {
        if (p_node == p_current_iter) {
            if (p_current_iter == p_queue->mp_last) {
                p_queue->mp_last = p_previous_iter;
            }
            p_previous_iter->mp_next = p_current_iter->mp_next;
            return RTX_OK;
        }
        p_previous_iter = p_current_iter;
        p_current_iter = p_current_iter->mp_next;
    }
    
#ifdef DEBUG_0
        printf("Node not found.\n");
#endif
    
    return RTX_ERR;
}

int queue_contains_node(k_queue_t *p_queue, k_node_t *p_node)
{
    k_node_t *p_iter = NULL;
    
    if (is_queue_empty(p_queue)) {
    
#ifdef DEBUG_0
        printf("Attempted to check node membership for an empty queue.\n");
#endif
    
        return RTX_ERR;
    }
    
    p_iter = p_queue->mp_first;
    while (p_iter != NULL) {
        if (p_iter == p_node) {
            return 1;
        }
        p_iter = p_iter->mp_next;
    }
    
    return 0;
}

int is_queue_empty(k_queue_t *p_queue)
{
    if (p_queue == NULL) {
    
#ifdef DEBUG_0
        printf("Attempted to check if a NULL queue was empty.\n");
#endif
    
        return RTX_ERR;
    } else {
        return (p_queue->mp_first == NULL);
    }
}

#ifdef DEBUG_0

int print_queue(k_queue_t *p_queue)
{
    k_node_t *p_iter = NULL;
    
    if (p_queue == NULL) {
        return RTX_ERR;
    }
    
    p_iter = p_queue->mp_first;
    printf("queue: 0x%x\n\tfirst:0x%x\n\tlast:0x%x\n", p_queue, p_queue->mp_first, p_queue->mp_last);
    printf("nodes:\n");
    while (p_iter != NULL) {
        printf("\t0x%x, next: 0x%x\n", p_iter, p_iter->mp_next);
        p_iter = p_iter->mp_next;
    }
    
    return RTX_OK;
}

#endif /* DEBUG_0 */
