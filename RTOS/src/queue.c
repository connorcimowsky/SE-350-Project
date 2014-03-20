#include "queue.h"
#include "rtos.h"

#ifdef DEBUG_1
#include "printf.h"
#endif


int enqueue(node_t *p_node, queue_t *p_queue)
{
    if (p_queue == NULL || p_node == NULL) {
    
#ifdef DEBUG_1
        printf("Node insertion error.\n\r");
#endif
    
        return RTOS_ERR;
    }
    
    p_node->mp_next = NULL;
    
    if (is_queue_empty(p_queue)) {
        p_queue->mp_first = p_node;
        p_queue->mp_last = p_node;
    } else {
        p_queue->mp_last->mp_next = p_node;
        p_queue->mp_last = p_queue->mp_last->mp_next;
    }
    
    return RTOS_OK;
}

int sorted_enqueue(node_t *p_node, queue_t *p_queue)
{
    node_t *p_current_iter = NULL;
    node_t *p_previous_iter = NULL;
    
    if (p_queue == NULL || p_node == NULL) {
        
#ifdef DEBUG_1
        printf("Sorted node insertion error.\n\r");
#endif
        
        return RTOS_ERR;
    }
    
    if (is_queue_empty(p_queue)) {
        p_queue->mp_first = p_node;
        p_queue->mp_last = p_node;
        
        p_node->mp_next = NULL;
    } else {
        p_previous_iter = NULL;
        p_current_iter = p_queue->mp_first;
        
        /* find the correct position in the queue */
        while (p_current_iter != NULL && p_current_iter->m_val <= p_node->m_val) {
            p_previous_iter = p_current_iter;
            p_current_iter = p_current_iter->mp_next;
        }
        
        if (p_previous_iter == NULL) {
            /* we did not iterate at all; insert at the front */
            p_node->mp_next = p_current_iter;
            p_queue->mp_first = p_node;
        } else {
            /* the node is either inserted in the middle or at the back; this handles both cases */
            p_previous_iter->mp_next = p_node;
            p_node->mp_next = p_current_iter;
            
            if (p_current_iter == NULL) {
                p_queue->mp_last = p_node;
            }
        }
    }
    
    return RTOS_OK;
}

node_t *dequeue(queue_t *p_queue)
{
    node_t *p_first = NULL;
    
    if (is_queue_empty(p_queue)) {
    
#ifdef DEBUG_1
        printf("Attempted to dequeue a node from an empty queue.\n\r");
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

int remove_from_queue(node_t *p_node, queue_t *p_queue)
{
    node_t *p_current_iter = NULL;
    node_t *p_previous_iter = NULL;
    
    if (is_queue_empty(p_queue)) {
    
#ifdef DEBUG_1
        printf("Attempted to remove a node from an empty queue.\n\r");
#endif
    
        return RTOS_ERR;
    }
    
    if (p_node == p_queue->mp_first) {
        if (p_queue->mp_first == p_queue->mp_last) {
            p_queue->mp_first = NULL;
            p_queue->mp_last = NULL;
        } else {
            p_queue->mp_first = p_queue->mp_first->mp_next;
        }
        
        return RTOS_OK;
    }
    
    p_current_iter = p_queue->mp_first->mp_next;
    p_previous_iter = p_queue->mp_first;
    while (p_current_iter != NULL && p_previous_iter != NULL) {
        if (p_node == p_current_iter) {
            if (p_current_iter == p_queue->mp_last) {
                p_queue->mp_last = p_previous_iter;
            }
            p_previous_iter->mp_next = p_current_iter->mp_next;
            return RTOS_OK;
        }
        p_previous_iter = p_current_iter;
        p_current_iter = p_current_iter->mp_next;
    }
    
#ifdef DEBUG_1
        printf("Node not found.\n\r");
#endif
    
    return RTOS_ERR;
}

int queue_contains_node(queue_t *p_queue, node_t *p_node)
{
    node_t *p_iter = NULL;
    
    if (is_queue_empty(p_queue)) {
    
#ifdef DEBUG_1
        printf("Attempted to check node membership for an empty queue.\n\r");
#endif
    
        return RTOS_ERR;
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

int is_queue_empty(queue_t *p_queue)
{
    if (p_queue == NULL) {
    
#ifdef DEBUG_1
        printf("Attempted to check if a NULL queue was empty.\n\r");
#endif
    
        return RTOS_ERR;
    } else {
        return (p_queue->mp_first == NULL);
    }
}

#ifdef DEBUG_1

int print_queue(queue_t *p_queue)
{
    node_t *p_iter = NULL;
    
    if (p_queue == NULL) {
        return RTOS_ERR;
    }
    
    p_iter = p_queue->mp_first;
    printf("queue: 0x%x\n\r\tfirst:0x%x\n\r\tlast:0x%x\n\r", p_queue, p_queue->mp_first, p_queue->mp_last);
    printf("nodes:\n\r");
    while (p_iter != NULL) {
        printf("\t0x%x, next: 0x%x\n\r", p_iter, p_iter->mp_next);
        p_iter = p_iter->mp_next;
    }
    
    return RTOS_OK;
}

#endif /* DEBUG_1 */
