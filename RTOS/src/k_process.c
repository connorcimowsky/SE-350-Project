#include <LPC17xx.h>

#include "k_process.h"
#include "k_proc.h"
#include "usr_proc.h"


/* global variables */
PROC_INIT g_proc_table[NUM_PROCS];
k_pcb_t **gp_pcbs = NULL;
k_pcb_t *gp_current_process = NULL;
k_queue_t *gp_ready_queue[NUM_PRIORITIES];
k_queue_t *gp_blocked_queue[NUM_PRIORITIES];


void process_init(void) 
{
    int i;
    U32 *p_sp;
    
    /* configure the null process */
    g_proc_table[NULL_PROC_PID].m_pid = NULL_PROC_PID;
    g_proc_table[NULL_PROC_PID].m_priority = LOWEST;
    g_proc_table[NULL_PROC_PID].m_stack_size = USR_SZ_STACK;
    g_proc_table[NULL_PROC_PID].mpf_start_pc = &null_process;
    enqueue_node(gp_ready_queue[LOWEST], (k_node_t *)gp_pcbs[NULL_PROC_PID]);
    
    /* populate the test process table */
    set_test_procs();
    
    /* add the test processes to the initialization table */
    for (i = 0; i < NUM_TEST_PROCS; i++) {
        /* get a pointer to the correct ready queue for this process */
        k_queue_t *p_queue = gp_ready_queue[g_test_procs[i].m_priority];
        
        g_proc_table[i + 1].m_pid = g_test_procs[i].m_pid;
        g_proc_table[i + 1].m_priority = g_test_procs[i].m_priority;
        g_proc_table[i + 1].m_stack_size = g_test_procs[i].m_stack_size;
        g_proc_table[i + 1].mpf_start_pc = g_test_procs[i].mpf_start_pc;
        
        /* add the PCB corresponding to this process to the ready queue */
        enqueue_node(p_queue, (k_node_t *)gp_pcbs[i + 1]);
    }
    
    /* configure the timer i-process */
    g_proc_table[TIMER_I_PROC_PID].m_pid = TIMER_I_PROC_PID;
    g_proc_table[TIMER_I_PROC_PID].m_priority = HIGHEST;
    g_proc_table[TIMER_I_PROC_PID].m_stack_size = USR_SZ_STACK;
    g_proc_table[TIMER_I_PROC_PID].mpf_start_pc = &timer_i_process;
  
    /* initialize the exception stack frame (i.e. initial context) for each process */
    for (i = 0; i < NUM_PROCS; i++) {
        int j;
        
        (gp_pcbs[i])->m_pid = (g_proc_table[i]).m_pid;
        (gp_pcbs[i])->m_priority = (g_proc_table[i]).m_priority;
        
        /* processes should begin in the NEW state */
        (gp_pcbs[i])->m_state = NEW;
        
        p_sp = alloc_stack((g_proc_table[i]).m_stack_size);
        /* save the initial program status register */
        *(--p_sp) = INITIAL_xPSR;
        /* save the entry point of the process */
        *(--p_sp) = (U32)((g_proc_table[i]).mpf_start_pc);
        /* ensure that user registers are initialized as 0 */
        for (j = 0; j < 6; j++) {
            *(--p_sp) = 0x0;
        }
        /* ensure that there is space for R4-R11 and LR/PC */
        for (j = 0; j < 9; j++) {
            *(--p_sp) = 0x0;
        }
        /* set the PCB's stack pointer */
        (gp_pcbs[i])->mp_sp = p_sp;
    }
}

int k_release_processor(void)
{
    k_pcb_t *p_previous_pcb = NULL;
    k_pcb_t *p_next_pcb = k_ready_queue_peek();
    
    /* if there is nothing in the ready queue, do nothing */
    if (p_next_pcb == NULL) {
        return RTOS_OK;
    }
    
    /* only check the priority of the next process if the current process is not blocked */
    if (gp_current_process->m_state != BLOCKED_ON_RESOURCE && gp_current_process->m_state != WAITING_FOR_MESSAGE) {
        /* if the next process is of lesser importance, do nothing */
        if (p_next_pcb->m_priority > gp_current_process->m_priority) {
            return RTOS_OK;
        }
    }
    
    /* save a pointer to the currently-executing process */
    p_previous_pcb = gp_current_process;
    /* dequeue the next available process from the ready queue */
    gp_current_process = k_dequeue_ready_process();
    
    /* perform a context switch from the previous process to the next process */
    if (context_switch(p_previous_pcb, gp_current_process) == RTOS_ERR) {
        return RTOS_ERR;
    }
    
    return RTOS_OK;
}

k_pcb_t *k_ready_queue_peek(void)
{
    int i;
    k_pcb_t *p_pcb = NULL;
    
    /* iterate through the ready queues in priority sequence, using FIFO ordering within each queue */
    for (i = 0; i < NUM_PRIORITIES; i++) {
        if (!is_queue_empty(gp_ready_queue[i])) {
            p_pcb = (k_pcb_t *)queue_peek(gp_ready_queue[i]);
            break;
        }
    }
    
    return p_pcb;
}

int k_set_process_priority(int pid, int priority)
{
    k_pcb_t *p_pcb = NULL;
    
    if (pid < 0 || pid >= NUM_PROCS) {
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
    
    p_pcb = gp_pcbs[pid];
    
    if (p_pcb->m_priority == priority) {
        /* priority value has not changed; return early */
        return RTOS_OK;
    }
    
    switch(p_pcb->m_state) {
        case NEW:
            /* processes in the ready queue can be NEW or READY; follow the same procedure for both states */
        case READY:
            /* dequeue the process from the ready queue, update its priority, then re-enqueue it in the ready queue */
            if (remove_node_from_queue(gp_ready_queue[p_pcb->m_priority], (k_node_t *)p_pcb) == RTOS_OK) {
                p_pcb->m_priority = (PRIORITY_E)priority;
                if (k_enqueue_ready_process(p_pcb) == RTOS_ERR) {
                    return RTOS_ERR;
                }
            }
            break;
        case BLOCKED_ON_RESOURCE:
            /* dequeue the process from the blocked queue, update its priority, then re-enqueue it in the blocked queue */
            if (remove_node_from_queue(gp_blocked_queue[p_pcb->m_priority], (k_node_t *)p_pcb) == RTOS_OK) {
                p_pcb->m_priority = (PRIORITY_E)priority;
                if (k_enqueue_blocked_process(p_pcb) == RTOS_ERR) {
                    return RTOS_ERR;
                }
            }
            break;
        case WAITING_FOR_MESSAGE:
            /* if the process is waiting for a message, then it is not in a queue; simply update its priority */
        case EXECUTING:
            /* if the process is executing, then it is not in a queue; simply update its priority */
        default:
            p_pcb->m_priority = (PRIORITY_E)priority;
            break;
    }
    
    /* yield the processor so that the scheduler can run again */
    k_release_processor();
    
    return RTOS_OK;
}

int k_get_process_priority(int pid)
{
    if (pid < 0 || pid >= NUM_PROCS) {
        /* pid is out-of-bounds */
        return RTOS_ERR;
    }
    
    return (int)gp_pcbs[pid]->m_priority;
}

int k_send_message(int recipient_pid, void *p_msg)
{
    U8 *p_decrement = NULL;
    k_msg_t *p_msg_envelope = NULL;
    k_pcb_t *p_recipient_pcb = NULL;
    int ret_val;
    
    /* disable interrupt requests */
    __disable_irq();
    
    /* get the address of the actual message envelope */
    p_decrement = (U8 *)p_msg;
    p_decrement -= MSG_HEADER_OFFSET;
    
    p_msg_envelope = (k_msg_t *)p_decrement;
    p_msg_envelope->m_sender_pid = gp_current_process->m_pid;
    p_msg_envelope->m_recipient_pid = recipient_pid;
    
    p_recipient_pcb = gp_pcbs[recipient_pid];
    
    ret_val = enqueue_node(&(p_recipient_pcb->m_msg_queue), (k_node_t *)p_msg_envelope);
    
    if (ret_val == RTOS_OK) {
        if (p_recipient_pcb->m_state == WAITING_FOR_MESSAGE) {
            p_recipient_pcb->m_state = READY;
            ret_val = k_enqueue_ready_process(p_recipient_pcb);
        }
    }
    
    /* enable interrupt requests */
    __enable_irq();
    
    return ret_val;
}

void *k_receive_message(int *p_sender_pid)
{
    k_msg_t *p_msg = NULL;
    
    /* disable interrupt requests */
    __disable_irq();
    
    while (is_queue_empty(&(gp_current_process->m_msg_queue))) {
        gp_current_process->m_state = WAITING_FOR_MESSAGE;
        release_processor();
    }
    
    p_msg = (k_msg_t *)dequeue_node(&(gp_current_process->m_msg_queue));

    if (p_msg != NULL) {
        *p_sender_pid = p_msg->m_sender_pid;
    }
    
    /* enable interrupt requests */
    __enable_irq();
    
    if (p_msg == NULL) {
        return NULL;
    }
    
    return (void *)((U8 *)p_msg + MSG_HEADER_OFFSET);
}

int k_delayed_send(int recipient_pid, void *p_msg, int delay)
{
    return RTOS_OK;
}

void *k_non_blocking_receive_message(void)
{
    k_msg_t *p_msg = NULL;
    
    if (!is_queue_empty(&(gp_current_process->m_msg_queue))) {
        p_msg = (k_msg_t *)dequeue_node(&(gp_current_process->m_msg_queue));
    } else {
        return NULL;
    }
    
    return (void *)((U8 *)p_msg + MSG_HEADER_OFFSET);
}

int context_switch(k_pcb_t *p_pcb_old, k_pcb_t *p_pcb_new) 
{
    PROC_STATE_E new_state = p_pcb_new->m_state;
    
    switch (new_state) {
        case NEW:
            
            if (p_pcb_old != NULL) {
                /* only dereference members if non-NULL */
                if (p_pcb_old->m_state == EXECUTING) {
                    /* only enqueue in the ready queue if executing */
                    p_pcb_old->m_state = READY;
                    k_enqueue_ready_process(p_pcb_old);
                } else if (p_pcb_old->m_state == BLOCKED_ON_RESOURCE) {
                    /* don't add a process to the ready queue if it is already in the blocked queue */
                } else if (p_pcb_old->m_state == WAITING_FOR_MESSAGE) {
                    /* don't add a process to the ready queue if it is waiting for a message */
                }
            }
            
            p_pcb_new->m_state = EXECUTING;
            
            break;
            
        case READY:
            
            if (p_pcb_old != NULL) {
                /* only dereference members if non-NULL */
                if (p_pcb_old->m_state == EXECUTING) {
                    /* only enqueue in the ready queue if executing */
                    p_pcb_old->m_state = READY;
                    k_enqueue_ready_process(p_pcb_old);
                } else if (p_pcb_old->m_state == BLOCKED_ON_RESOURCE) {
                    /* don't add a process to the ready queue if it is already in the blocked queue */
                } else if (p_pcb_old->m_state == WAITING_FOR_MESSAGE) {
                    /* don't add a process to the ready queue if it is waiting for a message */
                }
            }
                
            p_pcb_new->m_state = EXECUTING;
            
            break;
            
        default:
            return RTOS_ERR;
    }
    
    return RTOS_OK;
}

int k_enqueue_ready_process(k_pcb_t *p_pcb)
{
    if (p_pcb == NULL) {
        return RTOS_ERR;
    }
    
    /* enqueue the PCB in the ready queue corresponding to its priority */
    return (enqueue_node(gp_ready_queue[p_pcb->m_priority], (k_node_t *)p_pcb));
}

k_pcb_t *k_dequeue_ready_process(void)
{
    int i;
    k_pcb_t *p_pcb = NULL;
    
    /* iterate through the ready queues in priority sequence, using FIFO ordering within each queue */
    for (i = 0; i < NUM_PRIORITIES; i++) {
        if (!is_queue_empty(gp_ready_queue[i])) {
            p_pcb = (k_pcb_t *)dequeue_node(gp_ready_queue[i]);
            break;
        }
    }
    
    return p_pcb;
}

int k_enqueue_blocked_process(k_pcb_t *p_pcb)
{
    k_queue_t *p_blocked_queue = NULL;
    
    if (p_pcb == NULL) {
        return RTOS_ERR;
    }
    
    p_pcb->m_state = BLOCKED_ON_RESOURCE;
    
    /* retrieve a pointer to the blocked queue corresponding to the priority of the process */
    p_blocked_queue = gp_blocked_queue[p_pcb->m_priority];
    
    if (!is_queue_empty(p_blocked_queue) && queue_contains_node(p_blocked_queue, (k_node_t *)p_pcb)) {
        /* the node is already contained in the blocked queue, so do not add it again */
        return RTOS_OK;
    }
    
    /* enqueue the PCB in the blocked queue */
    return (enqueue_node(p_blocked_queue, (k_node_t *)p_pcb));
}

k_pcb_t* k_dequeue_blocked_process(void)
{
    int i;
    k_pcb_t *p_pcb = NULL;
    
    /* iterate through the blocked queues in priority sequence, using FIFO ordering within each queue */
    for (i = 0; i < NUM_PRIORITIES; i++) {
        if (!is_queue_empty(gp_blocked_queue[i])) {
            p_pcb = (k_pcb_t *)dequeue_node(gp_blocked_queue[i]);
            break;
        }
    }
    
    return p_pcb;
}
