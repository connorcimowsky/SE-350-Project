#include <LPC17xx.h>

#include "k_process.h"
#include "k_system_proc.h"
#include "k_usr_proc.h"
#include "usr_test_proc.h"

#ifdef DEBUG_HOTKEYS
#include "printf.h"
#endif


/* global variables */
PROC_INIT g_proc_table[NUM_PROCS];
k_pcb_t **gp_pcbs = NULL;
k_pcb_t *gp_current_process = NULL;
k_queue_t *gp_ready_queue[NUM_PRIORITIES];
k_queue_t *gp_blocked_on_memory_queue[NUM_PRIORITIES];
k_queue_t *gp_blocked_on_receive_queue[NUM_PRIORITIES];

#ifdef DEBUG_HOTKEYS

k_msg_log_t g_sent_msg_log[MSG_LOG_SIZE];
int g_cur_sent_msg_log_index = 0;
k_msg_log_t g_received_msg_log[MSG_LOG_SIZE];
int g_cur_received_msg_log_index = 0;

#endif /* DEBUG_HOTKEYS */


void process_init(void) 
{
    int i;
    U32 *p_sp;
    
    /* configure the null process */
    g_proc_table[PID_NULL].m_pid = PID_NULL;
    g_proc_table[PID_NULL].m_priority = LOWEST;
    g_proc_table[PID_NULL].m_stack_size = USR_SZ_STACK;
    g_proc_table[PID_NULL].mpf_start_pc = &null_process;
    enqueue_node(gp_ready_queue[LOWEST], (k_node_t *)gp_pcbs[PID_NULL]);
    
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
    g_proc_table[PID_TIMER_IPROC].m_pid = PID_TIMER_IPROC;
    g_proc_table[PID_TIMER_IPROC].m_priority = HIGHEST;
    g_proc_table[PID_TIMER_IPROC].m_stack_size = USR_SZ_STACK;
    g_proc_table[PID_TIMER_IPROC].mpf_start_pc = &timer_i_process;
    
    /* configure the UART i-process */
    g_proc_table[PID_UART_IPROC].m_pid = PID_UART_IPROC;
    g_proc_table[PID_UART_IPROC].m_priority = HIGHEST;
    g_proc_table[PID_UART_IPROC].m_stack_size = USR_SZ_STACK;
    g_proc_table[PID_UART_IPROC].mpf_start_pc = &uart_i_process;
    
    /* configure the KCD process */
    g_proc_table[PID_KCD].m_pid = PID_KCD;
    g_proc_table[PID_KCD].m_priority = HIGHEST;
    g_proc_table[PID_KCD].m_stack_size = USR_SZ_STACK;
    g_proc_table[PID_KCD].mpf_start_pc = &kcd_proc;
    enqueue_node(gp_ready_queue[HIGHEST], (k_node_t *)gp_pcbs[PID_KCD]);
    
    /* configure the CRT process */
    g_proc_table[PID_CRT].m_pid = PID_CRT;
    g_proc_table[PID_CRT].m_priority = HIGHEST;
    g_proc_table[PID_CRT].m_stack_size = USR_SZ_STACK;
    g_proc_table[PID_CRT].mpf_start_pc = &crt_proc;
    enqueue_node(gp_ready_queue[HIGHEST], (k_node_t *)gp_pcbs[PID_CRT]);
    
    /* configure the wall clock process */
    g_proc_table[PID_CLOCK].m_pid = PID_CLOCK;
    g_proc_table[PID_CLOCK].m_priority = HIGHEST;
    g_proc_table[PID_CLOCK].m_stack_size = USR_SZ_STACK;
    g_proc_table[PID_CLOCK].mpf_start_pc = &wall_clock_proc;
    enqueue_node(gp_ready_queue[HIGHEST], (k_node_t *)gp_pcbs[PID_CLOCK]);
    
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
    if (gp_current_process->m_state != BLOCKED_ON_MEMORY && gp_current_process->m_state != BLOCKED_ON_RECEIVE) {
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
    
    if (pid == PID_NULL || pid == PID_KCD || pid == PID_CRT) {
        /* disallow changing the priorities of system processes */
        return RTOS_ERR;
    }
    
    if (priority == LOWEST) {
        /* this priority is reserved for the null process */
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
        case BLOCKED_ON_MEMORY:
            /* dequeue the process from the blocked-on-memory queue, update its priority, then re-enqueue it in the blocked-on-memory queue */
            if (remove_node_from_queue(gp_blocked_on_memory_queue[p_pcb->m_priority], (k_node_t *)p_pcb) == RTOS_OK) {
                p_pcb->m_priority = (PRIORITY_E)priority;
                if (k_enqueue_blocked_on_memory_process(p_pcb) == RTOS_ERR) {
                    return RTOS_ERR;
                }
            }
            break;
        case BLOCKED_ON_RECEIVE:
            /* dequeue the process from the blocked-on-receive queue, update its priority, then re-enqueue it in the blocked-on-receive queue */
            if (remove_node_from_queue(gp_blocked_on_receive_queue[p_pcb->m_priority], (k_node_t *)p_pcb) == RTOS_OK) {
                p_pcb->m_priority = (PRIORITY_E)priority;
                if (k_enqueue_blocked_on_receive_process(p_pcb) == RTOS_ERR) {
                    return RTOS_ERR;
                }
            }
            break;
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
    /* call the non-premptive version of k_send_message */
    int ret_val = k_send_message_helper(gp_current_process->m_pid, recipient_pid, p_msg);
    
    if (ret_val == 1) {
        /* only check for preemption if we unblocked the recipient (returned 1) */
        if (gp_pcbs[recipient_pid]->m_priority <= gp_current_process->m_priority) {
            /* don't preempt in the case of the UART i-process */
            if (recipient_pid != PID_UART_IPROC) {
                return k_release_processor();
            }
        }
    } else {
        return ret_val;
    }
    
    return RTOS_OK;
}

void *k_receive_message(int *p_sender_pid)
{
    k_msg_t *p_msg = NULL;
    U8 *p_increment = NULL;
    
    while (is_queue_empty(&(gp_current_process->m_msg_queue))) {
        /* if there are no messages, block ourselves and yield the processor */
        if (k_enqueue_blocked_on_receive_process(gp_current_process) == RTOS_OK) {
            k_release_processor();
        }
    }
    
    p_msg = (k_msg_t *)dequeue_node(&(gp_current_process->m_msg_queue));
    
    if (p_msg == NULL) {
        return NULL;
    }
    
#ifdef DEBUG_HOTKEYS
    k_log_received_message(p_msg);
#endif
    
    if (p_sender_pid != NULL) {
        /* only write into the return address if one was provided */
        *p_sender_pid = p_msg->m_sender_pid;
    }
    
    p_increment = (U8 *)p_msg;
    p_increment += MSG_HEADER_OFFSET;
    
    return (void *)p_increment;
}

int k_delayed_send(int recipient_pid, void *p_msg, int delay)
{
    U8 *p_decrement = NULL;
    k_msg_t *p_msg_envelope = NULL;
    
    p_decrement = (U8 *)p_msg;
    p_decrement -= MSG_HEADER_OFFSET;
    
    p_msg_envelope = (k_msg_t *)p_decrement;
    p_msg_envelope->m_expiry = g_timer_count + delay;
    p_msg_envelope->m_sender_pid = gp_current_process->m_pid;
    p_msg_envelope->m_recipient_pid = recipient_pid;
    
    return enqueue_node(&(gp_pcbs[PID_TIMER_IPROC]->m_msg_queue), (k_node_t *)p_msg_envelope);
}

U32 k_get_system_time(void)
{
    return g_timer_count;
}

void *k_non_blocking_receive_message(int pid)
{
    if (!is_queue_empty(&(gp_pcbs[pid]->m_msg_queue))) {
        k_msg_t *p_msg = (k_msg_t *)dequeue_node(&(gp_pcbs[pid]->m_msg_queue));
        
        U8 *p_increment = (U8 *)p_msg;
        p_increment += MSG_HEADER_OFFSET;
        
        return (void *)p_increment;
    } else {
        return NULL;
    }
}

int k_send_message_helper(int sender_pid, int recipient_pid, void *p_msg)
{
    U8 *p_decrement = NULL;
    k_msg_t *p_msg_envelope = NULL;
    k_pcb_t *p_recipient_pcb = NULL;
    
#ifdef DEBUG_HOTKEYS
    k_log_sent_message(sender_pid, recipient_pid, (msg_t *)p_msg);
#endif
    
    p_decrement = (U8 *)p_msg;
    p_decrement -= MSG_HEADER_OFFSET;
    
    p_msg_envelope = (k_msg_t *)p_decrement;
    p_msg_envelope->m_sender_pid = sender_pid;
    p_msg_envelope->m_recipient_pid = recipient_pid;
    
    p_recipient_pcb = gp_pcbs[recipient_pid];
    
    if (enqueue_node(&(p_recipient_pcb->m_msg_queue), (k_node_t *)p_msg_envelope) == RTOS_OK) {
        if (p_recipient_pcb->m_state != BLOCKED_ON_RECEIVE) {
            /* if the recipient was not blocked, then do not unblock it */
            return RTOS_OK;
        } else {
            if (k_remove_blocked_on_receive_process(p_recipient_pcb) == RTOS_OK) {
                p_recipient_pcb->m_state = READY;
                if (k_enqueue_ready_process(p_recipient_pcb) == RTOS_OK) {
                    /* if the recipient was successfully removed from the blocked queue and added to the ready queue, return 1 */
                    return 1;
                } else {
                    return RTOS_ERR;
                }
            } else {
                return RTOS_ERR;
            }
        }
    } else {
        return RTOS_ERR;
    }
}

#ifdef DEBUG_HOTKEYS

void k_log_sent_message(int sender_pid, int recipient_pid, msg_t *p_msg)
{
    int i;
    
    g_sent_msg_log[g_cur_sent_msg_log_index].m_sender_pid = sender_pid;
    g_sent_msg_log[g_cur_sent_msg_log_index].m_recipient_pid = recipient_pid;
    g_sent_msg_log[g_cur_sent_msg_log_index].m_type = ((msg_t *)p_msg)->m_type;
    
    for (i = 0; i < MSG_LOG_LEN; i++) {
        g_sent_msg_log[g_cur_sent_msg_log_index].m_text[i] = ((msg_t *)p_msg)->m_data[i];
    }
    
    g_sent_msg_log[g_cur_sent_msg_log_index].m_time_stamp = k_get_system_time();
    
    g_cur_sent_msg_log_index = (g_cur_sent_msg_log_index + 1) % MSG_LOG_SIZE;
}

void k_log_received_message(k_msg_t *p_msg)
{
    int i = 0;
    
    g_received_msg_log[g_cur_received_msg_log_index].m_sender_pid = p_msg->m_sender_pid;
    g_received_msg_log[g_cur_received_msg_log_index].m_recipient_pid = p_msg->m_recipient_pid;
    g_received_msg_log[g_cur_received_msg_log_index].m_type = p_msg->m_type;
    
    for (i = 0; i < MSG_LOG_LEN; i++) {
        g_received_msg_log[g_cur_received_msg_log_index].m_text[i] = p_msg->m_data[i];
    }
    
    g_received_msg_log[g_cur_received_msg_log_index].m_time_stamp = k_get_system_time();
    
    g_cur_received_msg_log_index = (g_cur_received_msg_log_index + 1) % MSG_LOG_SIZE;
}

#endif /* DEBUG_HOTKEYS */

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
                } else if (p_pcb_old->m_state == BLOCKED_ON_MEMORY) {
                    /* don't add a process to the ready queue if it is already in the blocked-on-memory queue */
                } else if (p_pcb_old->m_state == BLOCKED_ON_RECEIVE) {
                    /* don't add a process to the ready queue if it is already in the blocked-on-receive queue */
                }
                /* save the main stack pointer of the previous process */
                p_pcb_old->mp_sp = (U32 *)__get_MSP();
            }
            
            p_pcb_new->m_state = EXECUTING;
            
            /* switch to the stack pointer of the new process */
            __set_MSP((U32)p_pcb_new->mp_sp);
            /* pop the exception stack frame to give the new process an initial context */
            __rte();
            
            break;
            
        case READY:
            
            if (p_pcb_old != NULL) {
                /* only dereference members if non-NULL */
                if (p_pcb_old->m_state == EXECUTING) {
                    /* only enqueue in the ready queue if executing */
                    p_pcb_old->m_state = READY;
                    k_enqueue_ready_process(p_pcb_old);
                } else if (p_pcb_old->m_state == BLOCKED_ON_MEMORY) {
                    /* don't add a process to the ready queue if it is already in the blocked-on-memory queue */
                } else if (p_pcb_old->m_state == BLOCKED_ON_RECEIVE) {
                    /* don't add a process to the ready queue if it is already in the blocked-on-memory queue */
                }
                /* save the main stack pointer of the previous process */
                p_pcb_old->mp_sp = (U32 *)__get_MSP();
            }
                
            p_pcb_new->m_state = EXECUTING;
            
            /* switch to the stack pointer of the next-to-run process */
            __set_MSP((U32)p_pcb_new->mp_sp);
            
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

int k_enqueue_blocked_on_memory_process(k_pcb_t *p_pcb)
{
    k_queue_t *p_blocked_on_memory_queue = NULL;
    
    if (p_pcb == NULL) {
        return RTOS_ERR;
    }
    
    p_pcb->m_state = BLOCKED_ON_MEMORY;
    
    /* retrieve a pointer to the blocked-on-memory queue corresponding to the priority of the process */
    p_blocked_on_memory_queue = gp_blocked_on_memory_queue[p_pcb->m_priority];
    
    if (!is_queue_empty(p_blocked_on_memory_queue) && queue_contains_node(p_blocked_on_memory_queue, (k_node_t *)p_pcb)) {
        /* the node is already contained in the blocked-on-memory queue, so do not add it again */
        return RTOS_OK;
    }
    
    /* enqueue the PCB in the blocked-on-memory queue */
    return (enqueue_node(p_blocked_on_memory_queue, (k_node_t *)p_pcb));
}

k_pcb_t* k_dequeue_blocked_on_memory_process(void)
{
    int i;
    k_pcb_t *p_pcb = NULL;
    
    /* iterate through the blocked-on-memory queues in priority sequence, using FIFO ordering within each queue */
    for (i = 0; i < NUM_PRIORITIES; i++) {
        if (!is_queue_empty(gp_blocked_on_memory_queue[i])) {
            p_pcb = (k_pcb_t *)dequeue_node(gp_blocked_on_memory_queue[i]);
            break;
        }
    }
    
    return p_pcb;
}

int k_enqueue_blocked_on_receive_process(k_pcb_t *p_pcb)
{
    k_queue_t *p_blocked_on_receive_queue = NULL;
    
    if (p_pcb == NULL) {
        return RTOS_ERR;
    }
    
    p_pcb->m_state = BLOCKED_ON_RECEIVE;
    
    /* retrieve a pointer to the blocked-on-receive queue corresponding to the priority of the process */
    p_blocked_on_receive_queue = gp_blocked_on_receive_queue[p_pcb->m_priority];
    
    if (!is_queue_empty(p_blocked_on_receive_queue) && queue_contains_node(p_blocked_on_receive_queue, (k_node_t *)p_pcb)) {
        /* the node is already contained in the blocked-on-receive queue, so do not add it again */
        return RTOS_OK;
    }
    
    /* enqueue the PCB in the blocked-on-receive queue */
    return (enqueue_node(p_blocked_on_receive_queue, (k_node_t *)p_pcb));
}

int k_remove_blocked_on_receive_process(k_pcb_t *p_pcb)
{
    return (remove_node_from_queue(gp_blocked_on_receive_queue[p_pcb->m_priority], (k_node_t *)p_pcb));
}

#ifdef DEBUG_HOTKEYS

void k_print_ready_queue(void)
{
    int i;
    
    printf("\n\r\n\r*** READY QUEUE ***\n\r\n\r");
    
    printf("Executing:\n\r\tPID %d\n\r\n\r", gp_current_process->m_pid);
    
    /* iterate through the ready queue */
    for (i = 0; i < NUM_PRIORITIES; i++) {
        k_queue_t *p_cur_queue = gp_ready_queue[i];
        k_pcb_t *p_cur_pcb = (k_pcb_t *)p_cur_queue->mp_first;
        
        printf("Priority %d:\n\r", i);
        
        while (p_cur_pcb != NULL) {
            printf("\tPID %d\n\r", p_cur_pcb->m_pid);
            p_cur_pcb = p_cur_pcb->mp_next;
        }
    }
    
    printf("\n\r");
}

void k_print_blocked_on_memory_queue(void)
{
    int i;
    
    printf("\n\r\n\r*** BLOCKED-ON-MEMORY QUEUE ***\n\r\n\r");
    
    /* iterate through the blocked-on-memory queue */
    for (i = 0; i < NUM_PRIORITIES; i++) {
        k_queue_t *p_cur_queue = gp_blocked_on_memory_queue[i];
        k_pcb_t *p_cur_pcb = (k_pcb_t *)p_cur_queue->mp_first;
        
        printf("Priority %d:\n\r", i);
        
        while (p_cur_pcb != NULL) {
            printf("\tPID %d\n\r", p_cur_pcb->m_pid);
            p_cur_pcb = p_cur_pcb->mp_next;
        }
    }
    
    printf("\n\r");
}

void k_print_blocked_on_receive_queue(void)
{
    int i;
    
    printf("\n\r\n\r*** BLOCKED-ON-RECEIVE QUEUE ***\n\r\n\r");
    
    /* iterate through the blocked-on-receive queue */
    for (i = 0; i < NUM_PRIORITIES; i++) {
        k_queue_t *p_cur_queue = gp_blocked_on_receive_queue[i];
        k_pcb_t *p_cur_pcb = (k_pcb_t *)p_cur_queue->mp_first;
        
        printf("Priority %d:\n\r", i);
        
        while (p_cur_pcb != NULL) {
            printf("\tPID %d\n\r", p_cur_pcb->m_pid);
            p_cur_pcb = p_cur_pcb->mp_next;
        }
    }
    
    printf("\n\r");
}

void k_print_msg_logs(void)
{
    int i;
    
    printf("\n\r\n\r*** SENT MESSAGE LOG ***\n\r");
    
    /* iterate through the sent message log */
    for (i = 0; i < MSG_LOG_SIZE; i++) {
        int j;
        
        printf("\n\r");
        printf("Message #%d:\n\r", (i + 1));
        printf("\tSender PID: %d\n\r", g_sent_msg_log[i].m_sender_pid);
        printf("\tRecipient PID: %d\n\r", g_sent_msg_log[i].m_recipient_pid);
        printf("\tType: %d\n\r", g_sent_msg_log[i].m_type);
        printf("\tText: '");
        
        for (j = 0; j < MSG_LOG_LEN; j++) {
            if (g_sent_msg_log[i].m_text[j] == '\0') {
                break;
            }
            
            if ((g_sent_msg_log[i].m_text[j] != '\n') && (g_sent_msg_log[i].m_text[j] != '\r')) {
                printf("%c", g_sent_msg_log[i].m_text[j]);
            }
        }
        
        printf("'\n\r");
        printf("\tTime Stamp: %d\n\r", g_sent_msg_log[i].m_time_stamp);
    }
    
    printf("\n\r\n\r*** RECEIVED MESSAGE LOG ***\n\r");
    
    /* iterate through the received message log */
    for (i = 0; i < MSG_LOG_SIZE; i++) {
        int j;
        
        printf("\n\r");
        printf("Message #%d:\n\r", (i + 1));
        printf("\tSender PID: %d\n\r", g_received_msg_log[i].m_sender_pid);
        printf("\tRecipient PID: %d\n\r", g_received_msg_log[i].m_recipient_pid);
        printf("\tType: %d\n\r", g_received_msg_log[i].m_type);
        printf("\tText: '");
        
        for (j = 0; j < MSG_LOG_LEN; j++) {
            if (g_received_msg_log[i].m_text[j] == '\0') {
                break;
            }
            
            if ((g_received_msg_log[i].m_text[j] != '\n') && (g_received_msg_log[i].m_text[j] != '\r')) {
                printf("%c", g_received_msg_log[i].m_text[j]);
            }
        }
        
        printf("'\n\r");
        printf("\tTime Stamp: %d\n\r", g_received_msg_log[i].m_time_stamp);
    }
    
    printf("\n\r");
}

#endif /* DEBUG_HOTKEYS */
