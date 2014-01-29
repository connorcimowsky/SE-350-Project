#ifndef K_PROCESS_H
#define K_PROCESS_H

#include "k_rtx.h"


/* external functions */
extern U32 *alloc_stack(U32 size_b);
extern void __rte(void);


/* populate the process initialization table and set up the initial context of each process */
void process_init(void);

/* invoke the scheduler and switch to the next process */
int k_release_processor(void);

/* set the priority of the process specified by pid */
int k_set_process_priority(int pid, int priority);

/* get the priority of the process specified by pid */
int k_get_process_priority(int pid);

/* perform a context switch from p_pcb_node_old to p_pcb_node_new */
int context_switch(k_pcb_node_t *p_pcb_node_old, k_pcb_node_t *p_pcb_node_new);

/* set the state of p_pcb_node to READY and enqueue it in the ready queue */
int k_enqueue_ready_process(k_pcb_node_t *p_pcb_node);

/* dequeue the highest-priority process from the ready queue */
k_pcb_node_t *k_dequeue_ready_process(void);

/* set the state of p_pcb_node to BLOCKED_ON_RESOURCE and enqueue it in the blocked queue */
int k_enqueue_blocked_process(k_pcb_node_t *p_pcb_node);

/* dequeue the next available process from the blocked queue */
k_pcb_node_t *k_dequeue_blocked_process(void);


#endif /* K_PROCESS_H */
