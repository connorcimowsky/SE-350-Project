#ifndef K_LIST_H
#define K_LIST_H

#include "k_node.h"

typedef struct k_list_t {
    k_node_t *first;
} k_list_t;

int insert_node(k_list_t *list, k_node_t *node);

k_node_t *get_node(k_list_t *list);

int is_list_empty(k_list_t *list);

#endif
