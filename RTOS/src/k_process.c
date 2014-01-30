#include <LPC17xx.h>

#include "k_process.h"
#include "usr_proc.h"


/* global variables */
PROC_INIT g_proc_table[NUM_TEST_PROCS];
k_pcb_node_t **gp_pcb_nodes = NULL;
k_pcb_node_t *gp_current_process = NULL;
k_queue_t *gp_ready_queue[NUM_PRIORITIES];
k_queue_t *gp_blocked_queue[NUM_PRIORITIES];


void process_init(void) 
{
    int i;
    U32 *p_sp;
    
    /* populate the test process table */
    set_test_procs();
    
    /* populate the process initialization table */
    for (i = 0; i < NUM_TEST_PROCS; i++) {
        /* get a pointer to the correct ready queue for this process */
        k_queue_t *p_queue = gp_ready_queue[g_test_procs[i].m_priority];
        
        g_proc_table[i].m_pid = g_test_procs[i].m_pid;
        g_proc_table[i].m_priority = g_test_procs[i].m_priority;
        g_proc_table[i].m_stack_size = g_test_procs[i].m_stack_size;
        g_proc_table[i].mpf_start_pc = g_test_procs[i].mpf_start_pc;
        
        /* add the PCB node corresponding to this process to the ready queue */
        enqueue_node(p_queue, (k_node_t *)gp_pcb_nodes[i]);
    }
  
    /* initialize the exception stack frame (i.e. initial context) for each process */
    for (i = 0; i < NUM_TEST_PROCS; i++) {
        int j;
        
        (gp_pcb_nodes[i])->mp_pcb->m_pid = (g_proc_table[i]).m_pid;
        (gp_pcb_nodes[i])->mp_pcb->m_priority = (g_proc_table[i]).m_priority;
        
        /* processes should begin in the NEW state */
        (gp_pcb_nodes[i])->mp_pcb->m_state = NEW;
        
        p_sp = alloc_stack((g_proc_table[i]).m_stack_size);
        /* save the initial program status register */
        *(--p_sp) = INITIAL_xPSR;
        /* save the entry point of the process */
        *(--p_sp) = (U32)((g_proc_table[i]).mpf_start_pc);
        /* ensure that user registers are initialized as 0 */
        for (j = 0; j < 6; j++) {
            *(--p_sp) = 0x0;
        }
        /* set the PCB's stack pointer */
        (gp_pcb_nodes[i])->mp_pcb->mp_sp = p_sp;
    }
}

int k_release_processor(void)
{
    k_pcb_node_t *p_pcb_node_old = NULL;
    
    /* save a pointer to the currently-executing process */
    p_pcb_node_old = gp_current_process;
    /* dequeue the next available process from the ready queue in order of priority */
    gp_current_process = k_dequeue_ready_process();
    
    /* if there is nothing in the ready queue, we must be executing the null process; revert */
    if (gp_current_process == NULL && p_pcb_node_old == gp_pcb_nodes[NULL_PROC_PID]) {
        gp_current_process = p_pcb_node_old;
        return RTOS_OK;
    }
    
    /* perform a context switch from the previous process to the next process */
    if (context_switch(p_pcb_node_old, gp_current_process) == RTOS_ERR) {
        return RTOS_ERR;
    }
    
    return RTOS_OK;
}

int k_set_process_priority(int pid, int priority)
{
    k_pcb_node_t *p_pcb_node = NULL;
    
    if (pid < 0 || pid > NUM_TEST_PROCS) {
        /* pid is out-of-bounds */
        return RTOS_ERR;
    }
    
    if (priority < 0 || priority >= NUM_PRIORITIES) {
        /* priority is out-of-bounds */
        return RTOS_ERR;
    }
    
    if (pid == 0 || priority == LOWEST) {
        /* these values are reserved for the null process */
        return RTOS_ERR;
    }
    
    p_pcb_node = gp_pcb_nodes[pid];
    
    if (p_pcb_node->mp_pcb->m_priority == priority) {
        /* priority value has not changed; return early */
        return RTOS_OK;
    }
    
    switch(p_pcb_node->mp_pcb->m_state) {
        case NEW:
            /* processes in the ready queue can be NEW or READY; follow the same procedure for both states */
        case READY:
            /* dequeue the process from the ready queue, update its priority, then re-enqueue it in the ready queue */
            if (remove_node_from_queue(gp_ready_queue[p_pcb_node->mp_pcb->m_priority], (k_node_t *)p_pcb_node) == RTOS_OK) {
                p_pcb_node->mp_pcb->m_priority = (PRIORITY_E)priority;
                if (k_enqueue_ready_process(p_pcb_node) == RTOS_ERR) {
                    return RTOS_ERR;
                }
            }
            break;
        case BLOCKED_ON_RESOURCE:
            /* dequeue the process from the blocked queue, update its priority, then re-enqueue it in the blocked queue */
            if (remove_node_from_queue(gp_blocked_queue[p_pcb_node->mp_pcb->m_priority], (k_node_t *)p_pcb_node) == RTOS_OK) {
                p_pcb_node->mp_pcb->m_priority = (PRIORITY_E)priority;
                if (k_enqueue_blocked_process(p_pcb_node) == RTOS_ERR) {
                    return RTOS_ERR;
                }
            }
            break;
        case EXECUTING:
            /* if the process is executing, then it is not in a queue; simply update its priority */
        default: {
            p_pcb_node->mp_pcb->m_priority = (PRIORITY_E)priority;
            break;
        }
    }
    
    /* yield the processor so that the scheduler can run again */
    k_release_processor();
    
    return RTOS_OK;
}

int k_get_process_priority(int pid)
{
    if (pid < 0 || pid > NUM_TEST_PROCS) {
        /* pid is out-of-bounds */
        return RTOS_ERR;
    }
    
    return (int)gp_pcb_nodes[pid]->mp_pcb->m_priority;
}

int context_switch(k_pcb_node_t *p_pcb_node_old, k_pcb_node_t *p_pcb_node_new) 
{
    PROC_STATE_E new_state = p_pcb_node_new->mp_pcb->m_state;
    
    switch (new_state) {
        case NEW:
            
            if (p_pcb_node_old != NULL && p_pcb_node_old->mp_pcb != NULL) {
                /* only dereference members if non-NULL */
                if (p_pcb_node_old->mp_pcb->m_state != BLOCKED_ON_RESOURCE) {
                    /* only enqueue in the ready queue if not already in the blocked queue */
                    p_pcb_node_old->mp_pcb->m_state = READY;
                    k_enqueue_ready_process(p_pcb_node_old);
                }
                /* save the main stack pointer of the previous process */
                p_pcb_node_old->mp_pcb->mp_sp = (U32 *)__get_MSP();
            }
            
            p_pcb_node_new->mp_pcb->m_state = EXECUTING;
            
            /* switch to the stack pointer of the new process */
            __set_MSP((U32)p_pcb_node_new->mp_pcb->mp_sp);
            /* pop the exception stack frame to give the new process an initial context */
            __rte();
            
            break;
            
        case READY:
            
            if (p_pcb_node_old != NULL && p_pcb_node_old->mp_pcb != NULL) {
                /* only dereference members if non-NULL */
                if (p_pcb_node_old->mp_pcb->m_state != BLOCKED_ON_RESOURCE) {
                    /* only enqueue in the ready queue if not already in the blocked queue */
                    p_pcb_node_old->mp_pcb->m_state = READY;
                    k_enqueue_ready_process(p_pcb_node_old);
                }
                /* save the main stack pointer of the previous process */
                p_pcb_node_old->mp_pcb->mp_sp = (U32 *)__get_MSP();
            }
                
            p_pcb_node_new->mp_pcb->m_state = EXECUTING;
            
            /* switch to the stack pointer of the next-to-run process */
            __set_MSP((U32)p_pcb_node_new->mp_pcb->mp_sp);
            
            break;
            
        default:
            return RTOS_ERR;
    }
    
    return RTOS_OK;
}

int k_enqueue_ready_process(k_pcb_node_t *p_pcb_node)
{
    if (p_pcb_node == NULL || p_pcb_node->mp_pcb == NULL) {
        return RTOS_ERR;
    }
    
    /* enqueue the PCB node in the ready queue corresponding to its priority */
    return (enqueue_node(gp_ready_queue[p_pcb_node->mp_pcb->m_priority], (k_node_t *)p_pcb_node));
}

k_pcb_node_t *k_dequeue_ready_process(void)
{
    int i;
    k_pcb_node_t *p_node = NULL;
    
    /* iterate through the ready queues in priority sequence, using FIFO ordering within each queue */
    for (i = 0; i < NUM_PRIORITIES; i++) {
        if (!is_queue_empty(gp_ready_queue[i])) {
            p_node = (k_pcb_node_t *)dequeue_node(gp_ready_queue[i]);
            break;
        }
    }
    
    return p_node;
}

int k_enqueue_blocked_process(k_pcb_node_t *p_pcb_node)
{
    k_queue_t *p_blocked_queue = NULL;
    
    if (p_pcb_node == NULL) {
        return RTOS_ERR;
    }
    
    p_pcb_node->mp_pcb->m_state = BLOCKED_ON_RESOURCE;
    
    /* retrieve a pointer to the blocked queue corresponding to the priority of the process */
    p_blocked_queue = gp_blocked_queue[p_pcb_node->mp_pcb->m_priority];
    
    if (!is_queue_empty(p_blocked_queue) && queue_contains_node(p_blocked_queue, (k_node_t *)p_pcb_node)) {
        /* the node is already contained in the blocked queue, so do not add it again */
        return RTOS_OK;
    }
    
    /* enqueue the PCB node in the blocked queue */
    return (enqueue_node(p_blocked_queue, (k_node_t *)p_pcb_node));
}

k_pcb_node_t* k_dequeue_blocked_process(void)
{
    int i;
    k_pcb_node_t *p_node = NULL;
    
    /* iterate through the blocked queues in priority sequence, using FIFO ordering within each queue */
    for (i = 0; i < NUM_PRIORITIES; i++) {
        if (!is_queue_empty(gp_blocked_queue[i])) {
            p_node = (k_pcb_node_t *)dequeue_node(gp_blocked_queue[i]);
            break;
        }
    }
    
    return p_node;
}
