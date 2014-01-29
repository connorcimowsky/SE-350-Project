#include "k_memory.h"
#include "k_process.h"

#ifdef DEBUG_0
#include "printf.h"
#endif


/* global variables */
k_list_t *gp_heap;
U32 *gp_heap_begin_addr;
U32 *gp_heap_end_addr;
U32 *gp_stack;


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
          |---------------------------|<--- gp_pcb_nodes
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
    U32 *p_end = (U32 *)&Image$$RW_IRAM1$$ZI$$Limit;
    int i;
    k_node_t *iterator = NULL;
    
    /* 4 bytes padding */
    p_end += 1;

    /* allocate memory for pcb node pointers   */
    gp_pcb_nodes = (k_pcb_node_t **)p_end;
    p_end += NUM_TEST_PROCS * sizeof(k_pcb_node_t *);
  
    /* allocate memory for pcb nodes */
    for ( i = 0; i < NUM_TEST_PROCS; i++ ) {
        gp_pcb_nodes[i] = (k_pcb_node_t *)p_end;
        p_end += sizeof(k_pcb_node_t); 
    }
    
    /* allocate memory for pcbs */
    for ( i = 0; i < NUM_TEST_PROCS; i++ ) {
        gp_pcb_nodes[i]->pcb = (PCB *)p_end;
        p_end += sizeof(PCB);
    }
    
    /* create ready queue */
    for (i = 0; i < NUM_PRIORITIES; i++) {
        gp_ready_queue[i] = (k_queue_t *)p_end;
        gp_ready_queue[i]->first = NULL;
        gp_ready_queue[i]->last = NULL;
        
        p_end += sizeof(k_queue_t);
    }
    
    /* create blocked queue */
    gp_blocked_queue = (k_queue_t *)p_end;
    gp_blocked_queue->first = NULL;
    gp_blocked_queue->last = NULL;
    
    p_end += sizeof(k_queue_t);
    
    /* prepare for alloc_stack() to allocate memory for stacks */
    
    gp_stack = (U32 *)RAM_END_ADDR;
    if ((U32)gp_stack & 0x04) { /* 8 bytes alignment */
        --gp_stack; 
    }
    
    gp_heap = (k_list_t *)p_end;
    gp_heap->first = NULL;
    p_end += sizeof(k_list_t);
    
    gp_heap_begin_addr = p_end;
    
    for (i = 0; i < NUM_BLOCKS; i++) {
        k_node_t *node = (k_node_t *)p_end;
        if (i == (NUM_BLOCKS - 1)) {
            // last block
            node->next = NULL;
        } else {
            node->next = (k_node_t *)(p_end + sizeof(k_node_t) + BLOCK_SIZE);
        }
        insert_node(gp_heap, (k_node_t *)node);
        p_end += sizeof(k_node_t) + BLOCK_SIZE;
    }
    
    gp_heap_end_addr = p_end;
    
#ifdef DEBUG_0
    iterator = gp_heap->first;
    printf("Memory blocks:\n");
    while (iterator != NULL) {
        printf("\tnode: 0x%x\n", iterator);
        iterator = iterator->next;
    }
#endif
}

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

void *k_request_memory_block(void)
{
    k_node_t *memory_block = NULL;
    while (is_list_empty(gp_heap)) {
    
#ifdef DEBUG_0
        printf("k_request_memory_block: no available blocks, releasing processor\n");
#endif
    
        if (k_enqueue_blocked_process(gp_current_process) == RTX_OK) {
            k_release_processor();
        }
    }

    memory_block = get_node(gp_heap);
    memory_block += 1;
    
#ifdef DEBUG_0
        // printf("k_request_memory_block: node address: 0x%x, block address: 0x%x\n", (memory_block - 1), memory_block);
#endif
    
    return (void *)memory_block;
}

int k_release_memory_block(void *p_mem_blk)
{
    k_node_t *block_ptr = NULL;
    
#ifdef DEBUG_0
        // printf("k_release_memory_block: node address: 0x%x, block address: 0x%x\n", block_ptr, (block_ptr + 1));
#endif
    
    if (p_mem_blk == NULL ) {
        
#ifdef DEBUG_0
        printf("k_release_memory_block: cannot release NULL\n");
#endif
        
        return RTX_ERR;
    }
    
    block_ptr = p_mem_blk;
    block_ptr -= 1;
    
    // make sure the pointer is not out of bounds
    if ((U32 *)block_ptr < gp_heap_begin_addr || (U32 *)block_ptr > gp_heap_end_addr) {
        
#ifdef DEBUG_0
        printf("k_release_memory_block: 0x%x is out of bounds\n", p_mem_blk);
#endif
        
        return RTX_ERR;
    }
    
    // make sure the pointer is block-aligned
    if ((gp_heap_end_addr - gp_heap_begin_addr) % (BLOCK_SIZE + sizeof(k_node_t)) != 0) {
        
#ifdef DEBUG_0
        printf("k_release_memory_block: 0x%x is not a block-aligned address\n", p_mem_blk);
#endif
        
        return RTX_ERR;
    }
    
    // make sure we aren't trying to release a duplicate block
    if (list_contains_node(gp_heap, block_ptr)) {
        
#ifdef DEBUG_0
        printf("k_release_memory_block: 0x%x has already been returned to the heap\n", p_mem_blk);
#endif
        
        return RTX_ERR;
    }
    
    if (insert_node(gp_heap, block_ptr) == RTX_ERR) {
        return RTX_ERR;
    }
    
    if (!is_queue_empty(gp_blocked_queue)) {
        k_pcb_node_t* p_blocked_pcb_node = k_dequeue_blocked_process();
        if (k_enqueue_ready_process(p_blocked_pcb_node) == RTX_OK) {
            k_release_processor();
        }
    }
    
    return RTX_OK;
}
