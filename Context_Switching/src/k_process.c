/**
 * @file:   k_process.c  
 * @brief:  process management C file
 * @author: Yiqing Huang
 * @author: Thomas Reidemeister
 * @date:   2014/01/17
 * NOTE: The example code shows one way of implementing context switching.
 *       The code only has minimal sanity check. There is no stack overflow check.
 *       The implementation assumes only two simple user processes and NO HARDWARE INTERRUPTS. 
 *       The purpose is to show how context switch could be done under stated assumptions. 
 *       These assumptions are not true in the required RTX Project!!!
 *       If you decide to use this piece of code, you need to understand the assumptions and
 *       the limitations. 
 */

#include <LPC17xx.h>
#include <system_LPC17xx.h>
#include "uart_polling.h"
#include "k_memory.h"
#include "k_process.h"
#include "k_queue.h"

#ifdef DEBUG_0
#include "printf.h"
#endif /* DEBUG_0 */

/* ----- Global Variables ----- */
PCB **gp_pcbs;                  /* array of pcbs */
PCB *gp_current_process = NULL; /* always point to the current RUN process */
k_queue_t *gp_ready_queue[NUM_PRIORITIES];
k_queue_t *gp_blocked_queue;

/* Process Initialization Table */
PROC_INIT g_proc_table[NUM_TEST_PROCS];
extern PROC_INIT g_test_procs[NUM_TEST_PROCS];

/**
 * @biref: initialize all processes in the system
 * NOTE: We assume there are only two user processes in the system in this example.
 */
void process_init() 
{
    int i;
    U32 *sp;
    
    /* create ready queue */
    for (i = 0; i < NUM_PRIORITIES; i++) {
        // TODO: Ask about how this should be allocated.
        gp_ready_queue[i] = (k_queue_t *)k_request_memory_block();
        gp_ready_queue[i]->first = NULL;
        gp_ready_queue[i]->last = NULL;
    }
    
    /* create blocked queue */
    gp_blocked_queue = (k_queue_t *)k_request_memory_block();
    gp_blocked_queue->first = NULL;
    gp_blocked_queue->last = NULL;
  
    /* fill out the initialization table */
    set_test_procs();
    for ( i = 0; i < NUM_TEST_PROCS; i++ ) {
        k_queue_t *queue = gp_ready_queue[g_test_procs[i].m_priority];
        k_ready_queue_node_t *node;
        
        g_proc_table[i].m_pid = g_test_procs[i].m_pid;
        g_proc_table[i].m_priority = g_test_procs[i].m_priority;
        g_proc_table[i].m_stack_size = g_test_procs[i].m_stack_size;
        g_proc_table[i].mpf_start_pc = g_test_procs[i].mpf_start_pc;
        
        node = (k_ready_queue_node_t *)k_request_memory_block();
        node->pcb = gp_pcbs[i];
        
        enqueue_node(queue, (k_node_t *)node);
    }
  
    /* initilize exception stack frame (i.e. initial context) for each process */
    for ( i = 0; i < NUM_TEST_PROCS; i++ ) {
        int j;
        (gp_pcbs[i])->m_pid = (g_proc_table[i]).m_pid;
        (gp_pcbs[i])->m_priority = (g_proc_table[i]).m_priority;
        (gp_pcbs[i])->m_state = NEW;
        
        sp = alloc_stack((g_proc_table[i]).m_stack_size);
        *(--sp)  = INITIAL_xPSR;      // user process initial xPSR  
        *(--sp)  = (U32)((g_proc_table[i]).mpf_start_pc); // PC contains the entry point of the process
        for ( j = 0; j < 6; j++ ) { // R0-R3, R12 are cleared with 0
            *(--sp) = 0x0;
        }
        (gp_pcbs[i])->mp_sp = sp;
    }
}

/*@brief: scheduler, pick the pid of the next to run process
 *@return: pointer to the next node containing the PCB pointer of the next to run process
 *         NULL if error happens
 *POST: if gp_current_process was NULL, then it gets set to pcbs[0].
 *      No other effect on other global variables.
 */

k_ready_queue_node_t *scheduler(void)
{
    int i;
    k_ready_queue_node_t *node = NULL;
    
    for (i = 0; i < NUM_PRIORITIES; i++) {
        if (!is_queue_empty(gp_ready_queue[i])) {
            node = (k_ready_queue_node_t *)dequeue_node(gp_ready_queue[i]);
            break;
        }
    }
    
    return node;
}

/*@brief: switch out old pcb (p_pcb_old), run the new pcb (gp_current_process)
 *@param: p_pcb_old, the old pcb that was in RUN
 *@return: RTX_OK upon success
 *         RTX_ERR upon failure
 *PRE:  p_pcb_old and gp_current_process are pointing to valid PCBs.
 *POST: if gp_current_process was NULL, then it gets set to pcbs[0].
 *      No other effect on other global variables.
 */
int process_switch(PCB *p_pcb_old) 
{
    PROC_STATE_E new_state = gp_current_process->m_state;
    
    switch (new_state) {
        case NEW:
            
            if (p_pcb_old != NULL) {
                if (p_pcb_old->m_state != BLOCKED_ON_RESOURCE) {
                    k_enqueue_ready_process(p_pcb_old);
                }
                p_pcb_old->mp_sp = (U32 *)__get_MSP();
            }
            
            gp_current_process->m_state = EXECUTING;
            
            __set_MSP((U32)gp_current_process->mp_sp);
            __rte(); // pop exception stack frame from the stack for a new processes
            
            break;
            
        case READY:
            
            if (p_pcb_old != NULL) {
                if (p_pcb_old->m_state != BLOCKED_ON_RESOURCE) {
                    k_enqueue_ready_process(p_pcb_old);
                }
                p_pcb_old->mp_sp = (U32 *)__get_MSP(); // save the old process's sp
                
                gp_current_process->m_state = EXECUTING;
                
                __set_MSP((U32)gp_current_process->mp_sp); //switch to the new proc's stack
            }
            
            break;
            
        default:
            gp_current_process = p_pcb_old; // revert back to the old proc on error
            return RTX_ERR;
    }
    
    return RTX_OK;
}
/**
 * @brief release_processor(). 
 * @return RTX_ERR on error and zero on success
 * POST: gp_current_process gets updated to next to run process
 */
int k_release_processor(void)
{
    PCB *p_pcb_old = NULL;
    k_ready_queue_node_t *next_ready_queue_node = NULL;
    
    p_pcb_old = gp_current_process;
    next_ready_queue_node = scheduler();
    
    if (next_ready_queue_node != NULL) {
        gp_current_process = next_ready_queue_node->pcb;
    } else {
        gp_current_process = NULL;
    }
    
    k_release_memory_block(next_ready_queue_node);
    
    if ( gp_current_process == NULL ) {
        // We want to resume execution of the process without adding it back to the
        // ready queue, i.e. 'pretend k_release_processor() was never called.
        
        // This handles the case of the null process (priority 4) and other error cases.
        
        gp_current_process = p_pcb_old; // revert back to the old process
        return RTX_ERR;
    }
    
    process_switch(p_pcb_old);
    return RTX_OK;
}

int k_enqueue_blocked_process(void)
{
    k_ready_queue_node_t *node = NULL;
    
    if (gp_current_process == NULL) {
        return RTX_ERR;
    }
    
    gp_current_process->m_state = BLOCKED_ON_RESOURCE;
    
    node = (k_ready_queue_node_t *)k_request_memory_block();
    node->pcb = gp_current_process;
    
    return (enqueue_node(gp_blocked_queue, (k_node_t *)node));
}

k_ready_queue_node_t* k_dequeue_blocked_process(void)
{
    if (is_queue_empty(gp_blocked_queue)) {
        return NULL;
    }
    
    return ((k_ready_queue_node_t *)dequeue_node(gp_blocked_queue));
}

int k_enqueue_ready_process(PCB* p_pcb_old)
{
    k_ready_queue_node_t *node = (k_ready_queue_node_t *)k_request_memory_block();
    node->pcb = p_pcb_old;
    node->next = NULL;
    
    return k_enqueue_ready_node(node);
}

int k_enqueue_ready_node(k_ready_queue_node_t* node)
{
    PCB* p_pcb_old;
    PRIORITY_E old_priority;

    if (node == NULL || node->pcb == NULL) {
        return RTX_ERR;
    }

    p_pcb_old = node->pcb;
    p_pcb_old->m_state = READY;
    
    old_priority = p_pcb_old->m_priority;
        
    return (enqueue_node(gp_ready_queue[old_priority], (k_node_t *)node));
}
