#include "k_list.h"
#include "rtos.h"

#ifdef DEBUG_1
#include "printf.h"
#endif


int insert_node(k_list_t *p_list, k_node_t *p_node)
{
    if (p_list == NULL || p_node == NULL) {
        
#ifdef DEBUG_1
        printf("Node insertion error.\n\r");
#endif
    
        return RTOS_ERR;
    }
    
    p_node->mp_next = p_list->mp_first;
    p_list->mp_first = p_node;
    
    return RTOS_OK;
}

k_node_t *get_node(k_list_t *p_list)
{
    k_node_t *p_first = NULL;
    
    if (is_list_empty(p_list)) {
        
#ifdef DEBUG_1
        printf("Attempted to get the top node of an empty list.\n\r");
#endif
    
        return NULL;
    }
    
    p_first = p_list->mp_first;
    p_list->mp_first = p_list->mp_first->mp_next;
    
    return p_first;
}

int list_contains_node(k_list_t *p_list, k_node_t *p_node)
{
    k_node_t *p_iter = NULL;
    
    if (is_list_empty(p_list)) {
    
#ifdef DEBUG_1
        printf("Attempted to check node membership for an empty list.\n\r");
#endif
    
        return RTOS_ERR;
    }
    
    p_iter = p_list->mp_first;
    while (p_iter != NULL) {
        if (p_iter == p_node) {
            return 1;
        }
        p_iter = p_iter->mp_next;
    }
    
    return 0;
}

int is_list_empty(k_list_t *p_list)
{
    if (p_list == NULL) {

#ifdef DEBUG_1
        printf("Attempted to check if a NULL list was empty.\n\r");
#endif
    
        return RTOS_ERR;
    } else {
        return (p_list->mp_first == NULL);
    }
}
