#ifndef K_NODE_H
#define K_NODE_H

#include "rtos.h"


typedef struct k_node_t {
    struct k_node_t *mp_next;
    U32 m_val;
} k_node_t;


#endif /* K_NODE_H */
