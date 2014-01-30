#ifndef K_MEM_H
#define K_MEM_H

#include "k_rtos.h"


/* initialize process control structures, align the stack, and divide the heap into blocks */
void memory_init(void);

/* allocate the stack for a process, maintaining 8-byte alignment of gp_stack */
U32 *alloc_stack(U32 size_b);

/* request a pointer to the next available memory block */
void *k_request_memory_block(void);

/* return p_mem_blk to the heap */
int k_release_memory_block(void *p_mem_blk);


#endif /* K_MEM_H */
