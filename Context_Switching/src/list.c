#include <stdlib.h>

#ifdef DEBUG_0
#include "printf.h"
#endif
#include "list.h"

list_t *new_list(void)
{
    // TODO(ConnorCimowsky): Use request_memory_block instead of malloc.
    list_t *list = (list_t *)malloc(sizeof(list_t));
    list->first = NULL;
    
    return list;
}

int insert_node(list_t *list, node_t *node)
{
    if (list == NULL || node == NULL) {
				#ifdef DEBUG_0
				printf("Node insertion error.\n");
				#endif
        return 1;
    }
    
    node->next = list->first;
    list->first = node;
    
    return 0;
}

node_t *get_node(list_t *list)
{
    node_t *first = NULL;
    
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

int is_list_empty(list_t *list) {
    if (list == NULL) {
				#ifdef DEBUG_0
				printf("Attempted to check if a NULL list was empty.\n");
				#endif
        return -1;
    } else {
        return (list->first == NULL);
    }
}
