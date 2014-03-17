#ifndef K_LIST_H
#define K_LIST_H

#include "node.h"


typedef struct list_t {
    node_t *mp_first;
} list_t;


int insert_node(list_t *p_list, node_t *p_node);

node_t *get_node(list_t *p_list);

int list_contains_node(list_t *p_list, node_t *p_node);

int is_list_empty(list_t *p_list);


#endif /* K_LIST_H */
