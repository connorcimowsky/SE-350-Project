#include "k_memory.h"
#include "k_process.h"
#include "k_system_proc.h"

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

    /* reserve memory for pointers to PCBs */
    gp_pcbs = (k_pcb_t **)p_end;
    p_end += NUM_PROCS * sizeof(k_pcb_t *);
    
    /* reserve memory for the PCBs themselves */
    for (i = 0; i < NUM_PROCS; i++) {
        gp_pcbs[i] = (k_pcb_t *)p_end;
        gp_pcbs[i]->m_msg_queue.mp_first = NULL;
        gp_pcbs[i]->m_msg_queue.mp_last = NULL;
        
        p_end += sizeof(k_pcb_t);
    }
    
    /* create the ready queue */
    for (i = 0; i < NUM_PRIORITIES; i++) {
        gp_ready_queue[i] = (k_queue_t *)p_end;
        gp_ready_queue[i]->mp_first = NULL;
        gp_ready_queue[i]->mp_last = NULL;
        
        p_end += sizeof(k_queue_t);
    }
    
    /* create the blocked-on-memory queue */
    for (i = 0; i < NUM_PRIORITIES; i++) {
        gp_blocked_on_memory_queue[i] = (k_queue_t *)p_end;
        gp_blocked_on_memory_queue[i]->mp_first = NULL;
        gp_blocked_on_memory_queue[i]->mp_last = NULL;
        
        p_end += sizeof(k_queue_t);
    }
    
    /* create the blocked-on-receive queue */
    for (i = 0; i < NUM_PRIORITIES; i++) {
        gp_blocked_on_receive_queue[i] = (k_queue_t *)p_end;
        gp_blocked_on_receive_queue[i]->mp_first = NULL;
        gp_blocked_on_receive_queue[i]->mp_last = NULL;
        
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
        
        /* insert the node into the memory heap structure */
        insert_node(gp_heap, (k_node_t *)p_node);

        /* space each memory block apart using the defined block size */
        p_end += BLOCK_SIZE;
    }
    
    /* save the end address of the heap for error-checking later on */
    gp_heap_end_addr = p_end;
    
    /* initialize the timeout queue for the timer i-process */
    g_timeout_queue.mp_first = NULL;
    g_timeout_queue.mp_last = NULL;
    
    /* initialize the keyboard command registry for the KCD process */
    g_kcd_reg.mp_first = NULL;
    
    /* populate the keyboard command registry with NUM_KCD_REG empty entries */
    for (i = 0; i < NUM_KCD_REG; i++) {
        int j;
        
        k_kcd_reg_t *p_reg = (k_kcd_reg_t *)p_end;
        
        p_reg->mp_next = NULL;
        
        /* make sure the keyboard command identifier is an array of null characters */
        for (j = 0; j < KCD_REG_LENGTH; j++) {
            p_reg->m_id[j] = '\0';
        }
        
        p_reg->m_pid = 0;
        p_reg->m_active = 0;
        
        insert_node(&g_kcd_reg, (k_node_t *)p_reg);
        
        p_end += sizeof(k_kcd_reg_t);
    }
    
#ifdef DEBUG_HOTKEYS
    
    /* initialize the sent message log */
    for (i = 0; i < MSG_LOG_SIZE; i++) {
        int j;
        
        g_sent_msg_log[i].m_sender_pid = 0;
        g_sent_msg_log[i].m_recipient_pid = 0;
        g_sent_msg_log[i].m_type = MSG_TYPE_DEFAULT;
        
        for (j = 0; j < MSG_LOG_LEN; j++) {
            g_sent_msg_log[i].m_text[j] = '\0';
        }
        
        g_sent_msg_log[i].m_time_stamp = 0;
    }
    
    /* initialize the received message log */
    for (i = 0; i < MSG_LOG_SIZE; i++) {
        int j;
        
        g_received_msg_log[i].m_sender_pid = 0;
        g_received_msg_log[i].m_recipient_pid = 0;
        g_received_msg_log[i].m_type = MSG_TYPE_DEFAULT;
        
        for (j = 0; j < MSG_LOG_LEN; j++) {
            g_received_msg_log[i].m_text[j] = '\0';
        }
        
        g_received_msg_log[i].m_time_stamp = 0;
    }
    
#endif /* DEBUG_HOTKEYS */
    
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
    U8 *p_mem_blk = NULL;
    
    while (is_list_empty(gp_heap)) {
        /* if the heap is empty, loop until a block becomes available */
        
#ifdef DEBUG_1
        printf("k_request_memory_block: no available blocks, releasing processor\n\r");
#endif
        
        /* add the calling process to the blocked queue and yield the processor */
        k_enqueue_blocked_on_memory_process(gp_current_process);
        k_release_processor();
    }

    /* retrieve the next available node from the heap */
    p_mem_blk = (U8 *)get_node(gp_heap);
    
#ifdef DEBUG_1
        printf("k_request_memory_block: 0x%x\n\r", p_mem_blk);
#endif
    
    /* increment the address of the node by the size of the header to get the start address of the block itself */
    p_mem_blk += MSG_HEADER_OFFSET;
    
    return (void *)p_mem_blk;
}

int k_release_memory_block(void *p_mem_blk)
{
    if (k_release_memory_block_helper(p_mem_blk) == RTOS_OK) {
        /* attempt to dequeue the next available process from the blocked queue */
        k_pcb_t *p_blocked_pcb = k_dequeue_blocked_on_memory_process();
        
        /* if there is a blocked process, set its state to READY and enqueue it in the ready queue */
        if (p_blocked_pcb != NULL) {
            p_blocked_pcb->m_state = READY;
            k_enqueue_ready_process(p_blocked_pcb);
            return k_release_processor();
        }
    } else {
        return RTOS_ERR;
    }
    
    return RTOS_OK;
}

int k_release_memory_block_helper(void *p_mem_blk)
{
    U8 *p_decrement = NULL;
    k_node_t *p_node = NULL;
    
    if (p_mem_blk == NULL ) {
        
#ifdef DEBUG_1
        printf("k_release_memory_block: cannot release NULL\n\r");
#endif
        
        return RTOS_ERR;
    }
    
    /* cast the memory block for correct pointer arithmetic */
    p_decrement = p_mem_blk;
    
    /* decrement the address of the block by the size of the header to get the start address of the node */
    p_decrement -= MSG_HEADER_OFFSET;
        
    /* cast the start address of the node to a k_node_t */
    p_node = (k_node_t *)p_decrement;
    
#ifdef DEBUG_1
        printf("k_release_memory_block: 0x%x\n\r", p_node);
#endif
    
    /* make sure the pointer is not out of bounds */
    if ((U8 *)p_node < gp_heap_begin_addr || (U8 *)p_node > gp_heap_end_addr) {
        
#ifdef DEBUG_1
        printf("k_release_memory_block: 0x%x is out of bounds\n\r", p_node);
#endif
        
        return RTOS_ERR;
    }
    
    /* make sure the pointer is block-aligned */
    if (((U8 *)p_node - gp_heap_begin_addr) % BLOCK_SIZE != 0) {
        
#ifdef DEBUG_1
        printf("k_release_memory_block: 0x%x is not a block-aligned address\n\r", p_node);
#endif
        
        return RTOS_ERR;
    }
    
    /* make sure we are not trying to release a duplicate block */
    if (!is_list_empty(gp_heap) && list_contains_node(gp_heap, p_node)) {
        
#ifdef DEBUG_1
        printf("k_release_memory_block: 0x%x is already contained in the heap\n\r", p_node);
#endif
        
        return RTOS_ERR;
    }
    
    /* if none of the above tests failed, insert the node into the memory heap */
    insert_node(gp_heap, p_node);
    
    return RTOS_OK;
}
