#include "k_rtx.h"
#ifdef DEBUG_0
#include "printf.h"
#endif
#include "k_list.h"

int insert_node(k_list_t *list, k_node_t *node)
{
    if (list == NULL || node == NULL) {
				
#ifdef DEBUG_0
        printf("Node insertion error.\n");
#endif
    
        return RTX_ERR;
    }
    
    node->next = list->first;
    list->first = node;
    
    return RTX_OK;
}

k_node_t *get_node(k_list_t *list)
{
    k_node_t *first = NULL;
    
    if (is_list_empty(list)) {
        
#ifdef DEBUG_0
        printf("Attempted to get the top node of an empty list.\n");
#endif
    
        return NULL;
    }
    
    first = list->first;
    list->first = list->first->next;
    
    return first;
}

int is_list_empty(k_list_t *list) {
    if (list == NULL) {

#ifdef DEBUG_0
        printf("Attempted to check if a NULL list was empty.\n");
#endif
    
        return RTX_ERR;
    } else {
        return (list->first == NULL);
    }
}
