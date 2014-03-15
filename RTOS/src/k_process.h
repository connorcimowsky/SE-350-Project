#ifndef K_PROCESS_H
#define K_PROCESS_H

#include "k_rtos.h"


/* external functions */
extern U32 *alloc_stack(U32 size_b);
extern void __rte(void);


/* populate the process initialization table and set up the initial context of each process */
void process_init(void);

/* invoke the scheduler and switch to the next process */
int k_release_processor(void);

/* return the next-to-run process from the ready queue without dequeueing it, or NULL if empty */
k_pcb_t *k_ready_queue_peek(void);

/* set the priority of the process specified by pid */
int k_set_process_priority(int pid, int priority);

/* get the priority of the process specified by pid */
int k_get_process_priority(int pid);

/* send a message to the process specified by recipient_pid */
int k_send_message(int recipient_pid, void *p_msg);

/* send a message to the process specified by recipient_pid after a delay */
int k_delayed_send(int recipient_pid, void *p_msg, int delay);

/* dequeue the first message from the message queue of the calling process */
void *k_receive_message(int *p_sender_pid);

/* non-blocking version of k_receive_message() */
void *k_non_blocking_receive_message(int pid);

/* enqueue p_msg in recipient_pid's message queue without preempting; return 1 if the recipient became unblocked */
int k_send_message_helper(int sender_pid, int recipient_pid, void *p_msg);

/* perform a context switch from p_pcb_old to p_pcb_new */
void context_switch(k_pcb_t *p_pcb_old, k_pcb_t *p_pcb_new);

/* enqueue p_pcb in the ready queue */
void k_enqueue_ready_process(k_pcb_t *p_pcb);

/* dequeue the highest-priority process from the ready queue */
k_pcb_t *k_dequeue_ready_process(void);

/* set the state of p_pcb to BLOCKED_ON_MEMORY and enqueue it in the blocked-on-memory queue */
void k_enqueue_blocked_on_memory_process(k_pcb_t *p_pcb);

/* dequeue the next available process from the blocked-on-memory queue */
k_pcb_t *k_dequeue_blocked_on_memory_process(void);

/* set the state of p_pcb to BLOCKED_ON_RECEIVE and enqueue it in the blocked-on-receive queue */
void k_enqueue_blocked_on_receive_process(k_pcb_t *p_pcb);

/* remove 'p_pcb' from the blocked-on-receive queue */
int k_remove_blocked_on_receive_process(k_pcb_t *p_pcb);

#ifdef DEBUG_HOTKEYS

/* print the ready queue */
void k_print_ready_queue(void);

/* print the blocked-on-memory queue */
void k_print_blocked_on_memory_queue(void);

/* print the blocked-on-receive queue */
void k_print_blocked_on_receive_queue(void);

/* print the sent and received message logs */
void k_print_msg_logs(void);

#endif /* DEBUG_HOTKEYS */

#endif /* K_PROCESS_H */
