#ifndef LIST_H
#define LIST_H

#include "node.h"

typedef struct list_t {
    node_t *first;
} list_t;

list_t *new_list(void);

int insert_node(list_t *list, node_t *node);

node_t *get_node(list_t *list);

int is_list_empty(list_t *list);

#endif
