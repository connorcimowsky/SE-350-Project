/**
 * @file:   k_memory.c
 * @brief:  kernel memory managment routines
 * @author: Yiqing Huang
 * @date:   2014/01/17
 */

#include "k_memory.h"
#include "k_list.h"

#ifdef DEBUG_0
#include "printf.h"
#endif /* ! DEBUG_0 */

extern int k_release_processor(void);

/* ----- Global Variables ----- */
U32 *gp_stack; /* The last allocated stack low address. 8 bytes aligned */
               /* The first stack starts at the RAM high address */
	       /* stack grows down. Fully decremental stack */
k_list_t *mem_heap;

/**
 * @brief: Initialize RAM as follows:

0x10008000+---------------------------+ High Address
          |    Proc 1 STACK           |  |
          |---------------------------|  |
          |    Proc 2 STACK           |  v
          |---------------------------|<--- gp_stack
          |                           |
          |        HEAP               |
          |                           |
          |---------------------------|
          |        PCB 2              |
          |---------------------------|  ^
          |        PCB 1              |  |
          |---------------------------|  |
          |        PCB pointers       |
          |---------------------------|<--- gp_pcbs
          |        Padding            |
          |---------------------------|  
          |Image$$RW_IRAM1$$ZI$$Limit |
          |...........................|          
          |       RTX  Image          |
          |                           |
0x10000000+---------------------------+ Low Address

*/

void memory_init(void)
{
	U8 *p_end = (U8 *)&Image$$RW_IRAM1$$ZI$$Limit;
	int i;
	k_node_t *iterator = NULL;
	
	/* 4 bytes padding */
	p_end += 4;

	/* allocate memory for pcb pointers   */
	gp_pcbs = (PCB **)p_end;
	p_end += NUM_TEST_PROCS * sizeof(PCB *);
  
	for ( i = 0; i < NUM_TEST_PROCS; i++ ) {
		gp_pcbs[i] = (PCB *)p_end;
		p_end += sizeof(PCB); 
	}
#ifdef DEBUG_0  
	printf("gp_pcbs[0] = 0x%x \n", gp_pcbs[0]);
	printf("gp_pcbs[1] = 0x%x \n", gp_pcbs[1]);
#endif

	/* prepare for alloc_stack() to allocate memory for stacks */
	
	gp_stack = (U32 *)RAM_END_ADDR;
	if ((U32)gp_stack & 0x04) { /* 8 bytes alignment */
		--gp_stack; 
	}
  
	/* allocate memory for heap, not implemented yet*/
	#ifdef DEBUG_0  
	// printf("", );
	#endif
	
	mem_heap = (k_list_t *)p_end;
	mem_heap->first = NULL;
	p_end += sizeof(k_list_t);
	
	for (i = 0; i < NUM_BLOCKS; i++) {
		k_node_t *node = (k_node_t *)p_end;
		if (i == (NUM_BLOCKS - 1)) {
			// last block
			node->next = NULL;
		} else {
			node->next = (k_node_t *)(p_end + sizeof(k_node_t) + BLOCK_SIZE);
		}
		insert_node(mem_heap, (k_node_t *)node);
		p_end += sizeof(k_node_t) + BLOCK_SIZE;
	}
	
	#ifdef DEBUG_0
	iterator = mem_heap->first;
    printf("pointer size: %d\n", sizeof(k_node_t));
	while (iterator != NULL) {
		printf("node address: %d\n", (int)iterator);
		iterator = iterator->next;
	}
	#endif
}

/**
 * @brief: allocate stack for a process, align to 8 bytes boundary
 * @param: size, stack size in bytes
 * @return: The top of the stack (i.e. high address)
 * POST:  gp_stack is updated.
 */

U32 *alloc_stack(U32 size_b) 
{
	U32 *sp;
	sp = gp_stack; /* gp_stack is always 8 bytes aligned */
	
	/* update gp_stack */
	gp_stack = (U32 *)((U8 *)sp - size_b);
	
	/* 8 bytes alignement adjustment to exception stack frame */
	if ((U32)gp_stack & 0x04) {
		--gp_stack; 
	}
	return sp;
}

void *k_request_memory_block(void) {
    // TODO(connor): Ask about atomic operations.
    k_node_t *memory_block = NULL;
#ifdef DEBUG_0 
	printf("k_request_memory_block: entering...\n");
#endif /* ! DEBUG_0 */
    while (is_list_empty(mem_heap)) {
#ifdef DEBUG_0
        printf("k_request_memory_block: no available blocks, releasing processor\n");
#endif
        // TODO(connor): Set process state to blocked.
        k_release_processor();
    }

    memory_block = get_node(mem_heap);

	return (void *) memory_block;
}

int k_release_memory_block(void *p_mem_blk) {
#ifdef DEBUG_0 
	printf("k_release_memory_block: releasing block @ 0x%x\n", p_mem_blk);
#endif /* ! DEBUG_0 */
	return RTX_OK;
}
