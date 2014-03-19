#ifndef K_NODE_H
#define K_NODE_H

#include "rtos.h"


typedef struct node_t {
    struct node_t *mp_next;
    U32 m_val;
} node_t;


#endif /* K_NODE_H */
