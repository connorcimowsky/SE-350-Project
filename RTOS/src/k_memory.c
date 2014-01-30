#include "k_memory.h"
#include "k_process.h"

#ifdef DEBUG_1
#include "printf.h"
#endif


/* global variables */
k_list_t *gp_heap;
U8 *gp_heap_begin_addr;
U8 *gp_heap_end_addr;
U32 *gp_stack;


void memory_init(void)
{
    /* get the address of the end of the memory image */
    U8 *p_end = (U8 *)&Image$$RW_IRAM1$$ZI$$Limit;
    int i;
    
    /* add 4 bytes of padding */
    p_end += 4;

    /* reserve memory for pointers to PCB nodes */
    gp_pcb_nodes = (k_pcb_node_t **)p_end;
    p_end += NUM_TEST_PROCS * sizeof(k_pcb_node_t *);
  
    /* reserve memory for the PCB nodes themselves */
    for ( i = 0; i < NUM_TEST_PROCS; i++ ) {
        gp_pcb_nodes[i] = (k_pcb_node_t *)p_end;
        p_end += sizeof(k_pcb_node_t); 
    }
    
    /* reserve memory for the PCBs themselves */
    for ( i = 0; i < NUM_TEST_PROCS; i++ ) {
        gp_pcb_nodes[i]->mp_pcb = (k_pcb_t *)p_end;
        p_end += sizeof(k_pcb_t);
    }
    
    /* create the ready queue */
    for (i = 0; i < NUM_PRIORITIES; i++) {
        gp_ready_queue[i] = (k_queue_t *)p_end;
        gp_ready_queue[i]->mp_first = NULL;
        gp_ready_queue[i]->mp_last = NULL;
        
        p_end += sizeof(k_queue_t);
    }
    
    /* create the blocked queue */
    for (i = 0; i < NUM_PRIORITIES; i++) {
        gp_blocked_queue[i] = (k_queue_t *)p_end;
        gp_blocked_queue[i]->mp_first = NULL;
        gp_blocked_queue[i]->mp_last = NULL;
        
        p_end += sizeof(k_queue_t);
    }
    
    /* prepare for alloc_stack() by ensuring 8-byte alignment */
    gp_stack = (U32 *)RAM_END_ADDR;
    if ((U32)gp_stack & 0x04) {
        --gp_stack; 
    }
    
    /* create the memory heap */
    
    gp_heap = (k_list_t *)p_end;
    gp_heap->mp_first = NULL;
    p_end += sizeof(k_list_t);
    
    /* save the beginning address of the heap for error-checking later on */
    gp_heap_begin_addr = p_end;
    
    for (i = 0; i < NUM_BLOCKS; i++) {
        
        /* create a node to represent a memory block */
        k_node_t *p_node = (k_node_t *)p_end;
        if (i == (NUM_BLOCKS - 1)) {
            /* the list should be NULL-terminated */
            p_node->mp_next = NULL;
        } else {
            /* block headers should not use any of the block's storage */
            p_node->mp_next = (k_node_t *)(p_end + sizeof(k_node_t) + BLOCK_SIZE);
        }
        
        /* insert the node into the memory heap structure */
        insert_node(gp_heap, (k_node_t *)p_node);
        p_end += sizeof(k_node_t) + BLOCK_SIZE;
    }
    
    /* save the end address of the heap for error-checking later on */
    gp_heap_end_addr = p_end;
}

U32 *alloc_stack(U32 size_b) 
{
    U32 *p_sp;
    p_sp = gp_stack; /* gp_stack is always 8-byte aligned */
    
    /* update gp_stack */
    gp_stack = (U32 *)((U8 *)p_sp - size_b);
    
    /* the exception stack frame should be 8-byte aligned */
    if ((U32)gp_stack & 0x04) {
        --gp_stack; 
    }
    
    return p_sp;
}

void *k_request_memory_block(void)
{
    k_node_t *p_mem_blk = NULL;
    
    while (is_list_empty(gp_heap)) {
        /* if the heap is empty, loop until a block becomes available */
        
#ifdef DEBUG_1
        printf("k_request_memory_block: no available blocks, releasing processor\n");
#endif
        
        /* add the calling process to the blocked queue and yield the processor */
        if (k_enqueue_blocked_process(gp_current_process) == RTOS_OK) {
            k_release_processor();
        }
    }

    /* retrieve the next available node from the heap */
    p_mem_blk = get_node(gp_heap);

    /* increment the address of the node by 4 bytes to get the start address of the block itself */
    p_mem_blk += 1;
    
#ifdef DEBUG_1
        printf("k_request_memory_block: node address: 0x%x, block address: 0x%x\n", (memory_block - 1), memory_block);
#endif
    
    return (void *)p_mem_blk;
}

int k_release_memory_block(void *p_mem_blk)
{
    k_node_t *p_node = NULL;
    k_pcb_node_t* p_blocked_pcb_node = NULL;
    
#ifdef DEBUG_1
        printf("k_release_memory_block: node address: 0x%x, block address: 0x%x\n", block_ptr, (block_ptr + 1));
#endif
    
    if (p_mem_blk == NULL ) {
        
#ifdef DEBUG_1
        printf("k_release_memory_block: cannot release NULL\n");
#endif
        
        return RTOS_ERR;
    }
    
    /* cast the start address of the memory block to a k_node_t for correct pointer arithmetic */
    p_node = p_mem_blk;
    
    /* decrement the address of the block by 4 bytes to get the start address of the node */
    p_node -= 1;
    
    /* make sure the pointer is not out of bounds */
    if ((U8 *)p_node < gp_heap_begin_addr || (U8 *)p_node > gp_heap_end_addr) {
        
#ifdef DEBUG_1
        printf("k_release_memory_block: 0x%x is out of bounds\n", p_mem_blk);
#endif
        
        return RTOS_ERR;
    }
    
    /* make sure the pointer is block-aligned */
    if (((U8 *)p_node - gp_heap_begin_addr) % (BLOCK_SIZE + sizeof(k_node_t)) != 0) {
        
#ifdef DEBUG_1
        printf("k_release_memory_block: 0x%x is not a block-aligned address\n", p_mem_blk);
#endif
        
        return RTOS_ERR;
    }
    
    /* make sure we are not trying to release a duplicate block */
    if (!is_list_empty(gp_heap) && list_contains_node(gp_heap, p_node)) {
        
#ifdef DEBUG_1
        printf("k_release_memory_block: 0x%x is already contained in the heap\n", p_mem_blk);
#endif
        
        return RTOS_ERR;
    }
    
    /* if none of the above tests failed, insert the node into the memory heap */
    if (insert_node(gp_heap, p_node) == RTOS_ERR) {
        return RTOS_ERR;
    }
    
    /* attempt to dequeue the next available process from the blocked queue */
    p_blocked_pcb_node = k_dequeue_blocked_process();
    
    /* if there is a blocked process, set its state to READY and enqueue it in the ready queue */
    if (p_blocked_pcb_node != NULL) {
        p_blocked_pcb_node->mp_pcb->m_state = READY;
        if (k_enqueue_ready_process(p_blocked_pcb_node) == RTOS_OK) {
            if (p_blocked_pcb_node->mp_pcb->m_priority < gp_current_process->mp_pcb->m_priority) {
                /* only preempt the calling process if the newly-unblocked process has a higher priority */
                k_release_processor();
            }
        }
    }
    
    return RTOS_OK;
}