#include <LPC17xx.h>

#include "k_process.h"
#include "usr_proc.h"

#ifdef DEBUG_0
#include "printf.h"
#endif


/* global variables */
PROC_INIT g_proc_table[NUM_TEST_PROCS];
k_pcb_node_t **gp_pcb_nodes = NULL;
k_pcb_node_t *gp_current_process = NULL;
k_queue_t *gp_ready_queue[NUM_PRIORITIES];
k_queue_t *gp_blocked_queue[NUM_PRIORITIES];


void process_init(void) 
{
    int i;
    U32 *sp;
    
    /* fill out the initialization table */
    set_test_procs();
    for ( i = 0; i < NUM_TEST_PROCS; i++ ) {
        k_queue_t *queue = gp_ready_queue[g_test_procs[i].m_priority];
        
        g_proc_table[i].m_pid = g_test_procs[i].m_pid;
        g_proc_table[i].m_priority = g_test_procs[i].m_priority;
        g_proc_table[i].m_stack_size = g_test_procs[i].m_stack_size;
        g_proc_table[i].mpf_start_pc = g_test_procs[i].mpf_start_pc;
        
        enqueue_node(queue, (k_node_t *)gp_pcb_nodes[i]);
    }
  
    /* initilize exception stack frame (i.e. initial context) for each process */
    for ( i = 0; i < NUM_TEST_PROCS; i++ ) {
        int j;
        (gp_pcb_nodes[i])->pcb->m_pid = (g_proc_table[i]).m_pid;
        (gp_pcb_nodes[i])->pcb->m_priority = (g_proc_table[i]).m_priority;
        (gp_pcb_nodes[i])->pcb->m_state = NEW;
        
        sp = alloc_stack((g_proc_table[i]).m_stack_size);
        *(--sp)  = INITIAL_xPSR;      // user process initial xPSR  
        *(--sp)  = (U32)((g_proc_table[i]).mpf_start_pc); // PC contains the entry point of the process
        for ( j = 0; j < 6; j++ ) { // R0-R3, R12 are cleared with 0
            *(--sp) = 0x0;
        }
        (gp_pcb_nodes[i])->pcb->mp_sp = sp;
    }
}

int k_release_processor(void)
{
    k_pcb_node_t *p_pcb_node_old = NULL;
    k_pcb_node_t *next_ready_queue_node = NULL;
    
    p_pcb_node_old = gp_current_process;
    next_ready_queue_node = k_dequeue_ready_process();
    
    if (next_ready_queue_node != NULL) {
        gp_current_process = next_ready_queue_node;
    } else {
        gp_current_process = NULL;
    }
    
    if ( gp_current_process == NULL ) {
        // We want to resume execution of the process without adding it back to the
        // ready queue, i.e. 'pretend k_release_processor() was never called.
        
        // This handles the case of the null process (priority 4) and other error cases.
        
        gp_current_process = p_pcb_node_old; // revert back to the old process
        return RTX_ERR;
    }
    
    context_switch(p_pcb_node_old, gp_current_process);
    return RTX_OK;
}

int context_switch(k_pcb_node_t *p_pcb_node_old, k_pcb_node_t *p_pcb_node_new) 
{
    PROC_STATE_E new_state = p_pcb_node_new->pcb->m_state;
    
    switch (new_state) {
        case NEW:
            
            if (p_pcb_node_old != NULL && p_pcb_node_old->pcb != NULL) {
                if (p_pcb_node_old->pcb->m_state != BLOCKED_ON_RESOURCE) {
                    k_enqueue_ready_process(p_pcb_node_old);
                }
                p_pcb_node_old->pcb->mp_sp = (U32 *)__get_MSP();
            }
            
            p_pcb_node_new->pcb->m_state = EXECUTING;
            
            __set_MSP((U32)p_pcb_node_new->pcb->mp_sp);
            __rte(); // pop exception stack frame from the stack for a new processes
            
            break;
            
        case READY:
            
            if (p_pcb_node_old != NULL && p_pcb_node_old->pcb != NULL) {
                if (p_pcb_node_old->pcb->m_state != BLOCKED_ON_RESOURCE) {
                    k_enqueue_ready_process(p_pcb_node_old);
                }
                p_pcb_node_old->pcb->mp_sp = (U32 *)__get_MSP(); // save the old process's sp
                
                p_pcb_node_new->pcb->m_state = EXECUTING;
                
                __set_MSP((U32)p_pcb_node_new->pcb->mp_sp); //switch to the new proc's stack
            }
            
            break;
            
        default:
            p_pcb_node_new = p_pcb_node_old; // revert back to the old proc on error
            return RTX_ERR;
    }
    
    return RTX_OK;
}

int k_enqueue_ready_process(k_pcb_node_t *p_pcb_node)
{
    PRIORITY_E priority;
    
    if (p_pcb_node == NULL || p_pcb_node->pcb == NULL) {
        return RTX_ERR;
    }

    p_pcb_node->pcb->m_state = READY;
    priority = p_pcb_node->pcb->m_priority;
    
    return (enqueue_node(gp_ready_queue[priority], (k_node_t *)p_pcb_node));
}

k_pcb_node_t *k_dequeue_ready_process(void)
{
    int i;
    k_pcb_node_t *node = NULL;
    
    for (i = 0; i < NUM_PRIORITIES; i++) {
        if (!is_queue_empty(gp_ready_queue[i])) {
            node = (k_pcb_node_t *)dequeue_node(gp_ready_queue[i]);
            break;
        }
    }
    
    return node;
}

int k_enqueue_blocked_process(k_pcb_node_t *p_pcb_node)
{
    k_queue_t *p_blocked_queue = NULL;
    
    if (p_pcb_node == NULL) {
        return RTX_ERR;
    }
    
    p_pcb_node->pcb->m_state = BLOCKED_ON_RESOURCE;
    
    p_blocked_queue = gp_blocked_queue[p_pcb_node->pcb->m_priority];
    
    if (!is_queue_empty(p_blocked_queue) && queue_contains_node(p_blocked_queue, (k_node_t *)p_pcb_node)) {
        // the node is already in the blocked queue, so don't enqueue it again
        return RTX_OK;
    }
    
    return (enqueue_node(p_blocked_queue, (k_node_t *)p_pcb_node));
}

k_pcb_node_t* k_dequeue_blocked_process(void)
{
    int i;
    k_pcb_node_t *node = NULL;
    
    for (i = 0; i < NUM_PRIORITIES; i++) {
        if (!is_queue_empty(gp_blocked_queue[i])) {
            node = (k_pcb_node_t *)dequeue_node(gp_blocked_queue[i]);
            break;
        }
    }
    
    return node;
}
