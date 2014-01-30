#include "k_memory.h"
#include "k_process.h"

#ifdef DEBUG_0
#include "printf.h"
#endif


/* global variables */
k_list_t *gp_heap;
U8 *gp_heap_begin_addr;
U8 *gp_heap_end_addr;
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
          |        RTOS Image         |
          |                           |
0x10000000+---------------------------+ Low Address

*/


void memory_init(void)
{
    U8 *p_end = (U8 *)&Image$$RW_IRAM1$$ZI$$Limit;
    int i;
    
#ifdef DEBUG_0
    k_node_t *p_iterator = NULL;
#endif
    
    /* 4 bytes padding */
    p_end += 4;

    /* allocate memory for pcb node pointers */
    gp_pcb_nodes = (k_pcb_node_t **)p_end;
    p_end += NUM_TEST_PROCS * sizeof(k_pcb_node_t *);
  
    /* allocate memory for pcb nodes */
    for ( i = 0; i < NUM_TEST_PROCS; i++ ) {
        gp_pcb_nodes[i] = (k_pcb_node_t *)p_end;
        p_end += sizeof(k_pcb_node_t); 
    }
    
    /* allocate memory for pcbs */
    for ( i = 0; i < NUM_TEST_PROCS; i++ ) {
        gp_pcb_nodes[i]->mp_pcb = (k_pcb_t *)p_end;
        p_end += sizeof(k_pcb_t);
    }
    
    /* create ready queue */
    for (i = 0; i < NUM_PRIORITIES; i++) {
        gp_ready_queue[i] = (k_queue_t *)p_end;
        gp_ready_queue[i]->mp_first = NULL;
        gp_ready_queue[i]->mp_last = NULL;
        
        p_end += sizeof(k_queue_t);
    }
    
    /* create blocked queue */
    for (i = 0; i < NUM_PRIORITIES; i++) {
        gp_blocked_queue[i] = (k_queue_t *)p_end;
        gp_blocked_queue[i]->mp_first = NULL;
        gp_blocked_queue[i]->mp_last = NULL;
        
        p_end += sizeof(k_queue_t);
    }
    
    /* prepare for alloc_stack() to allocate memory for stacks */
    
    gp_stack = (U32 *)RAM_END_ADDR;
    if ((U32)gp_stack & 0x04) { /* 8 bytes alignment */
        --gp_stack; 
    }
    
    gp_heap = (k_list_t *)p_end;
    gp_heap->mp_first = NULL;
    p_end += sizeof(k_list_t);
    
    gp_heap_begin_addr = p_end;
    
    for (i = 0; i < NUM_BLOCKS; i++) {
        k_node_t *p_node = (k_node_t *)p_end;
        if (i == (NUM_BLOCKS - 1)) {
            // last block
            p_node->mp_next = NULL;
        } else {
            p_node->mp_next = (k_node_t *)(p_end + sizeof(k_node_t) + BLOCK_SIZE);
        }
        insert_node(gp_heap, (k_node_t *)p_node);
        p_end += sizeof(k_node_t) + BLOCK_SIZE;
    }
    
    gp_heap_end_addr = p_end;
    
#ifdef DEBUG_0
    p_iterator = gp_heap->mp_first;
    printf("Memory blocks:\n");
    while (p_iterator != NULL) {
        printf("\tnode: 0x%x\n", p_iterator);
        p_iterator = p_iterator->mp_next;
    }
#endif
}

U32 *alloc_stack(U32 size_b) 
{
    U32 *p_sp;
    p_sp = gp_stack; /* gp_stack is always 8 bytes aligned */
    
    /* update gp_stack */
    gp_stack = (U32 *)((U8 *)p_sp - size_b);
    
    /* 8 bytes alignement adjustment to exception stack frame */
    if ((U32)gp_stack & 0x04) {
        --gp_stack; 
    }
    return p_sp;
}

void *k_request_memory_block(void)
{
    k_node_t *p_mem_blk = NULL;
    while (is_list_empty(gp_heap)) {
    
#ifdef DEBUG_0
        printf("k_request_memory_block: no available blocks, releasing processor\n");
#endif
    
        if (k_enqueue_blocked_process(gp_current_process) == RTOS_OK) {
            k_release_processor();
        }
    }

    p_mem_blk = get_node(gp_heap);
    p_mem_blk += 1;
    
#ifdef DEBUG_0
        // printf("k_request_memory_block: node address: 0x%x, block address: 0x%x\n", (memory_block - 1), memory_block);
#endif
    
    return (void *)p_mem_blk;
}

int k_release_memory_block(void *p_mem_blk)
{
    k_node_t *p_node = NULL;
    k_pcb_node_t* p_blocked_pcb_node = NULL;
    
#ifdef DEBUG_0
        // printf("k_release_memory_block: node address: 0x%x, block address: 0x%x\n", block_ptr, (block_ptr + 1));
#endif
    
    if (p_mem_blk == NULL ) {
        
#ifdef DEBUG_0
        printf("k_release_memory_block: cannot release NULL\n");
#endif
        
        return RTOS_ERR;
    }
    
    p_node = p_mem_blk;
    p_node -= 1;
    
    // make sure the pointer is not out of bounds
    if ((U8 *)p_node < gp_heap_begin_addr || (U8 *)p_node > gp_heap_end_addr) {
        
#ifdef DEBUG_0
        printf("k_release_memory_block: 0x%x is out of bounds\n", p_mem_blk);
#endif
        
        return RTOS_ERR;
    }
    
    // make sure the pointer is block-aligned
    if (((U8 *)p_node - gp_heap_begin_addr) % (BLOCK_SIZE + sizeof(k_node_t)) != 0) {
        
#ifdef DEBUG_0
        printf("k_release_memory_block: 0x%x is not a block-aligned address\n", p_mem_blk);
#endif
        
        return RTOS_ERR;
    }
    
    // make sure we aren't trying to release a duplicate block
    if (!is_list_empty(gp_heap) && list_contains_node(gp_heap, p_node)) {
        
#ifdef DEBUG_0
        printf("k_release_memory_block: 0x%x has already been returned to the heap\n", p_mem_blk);
#endif
        
        return RTOS_ERR;
    }
    
    if (insert_node(gp_heap, p_node) == RTOS_ERR) {
        return RTOS_ERR;
    }
    
    p_blocked_pcb_node = k_dequeue_blocked_process();
    
    if (p_blocked_pcb_node != NULL) {
        if (k_enqueue_ready_process(p_blocked_pcb_node) == RTOS_OK) {
            if (p_blocked_pcb_node->mp_pcb->m_priority < gp_current_process->mp_pcb->m_priority) {
                // only preempt the calling process if the newly-unblocked process has a higher priority
                k_release_processor();
            }
        }
    }
    
    return RTOS_OK;
}
