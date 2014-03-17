#ifndef K_LIST_H
#define K_LIST_H

#include "k_node.h"


typedef struct k_list_t {
    node_t *mp_first;
} k_list_t;


int insert_node(k_list_t *p_list, node_t *p_node);

node_t *get_node(k_list_t *p_list);

int list_contains_node(k_list_t *p_list, node_t *p_node);

int is_list_empty(k_list_t *p_list);


#endif /* K_LIST_H */
