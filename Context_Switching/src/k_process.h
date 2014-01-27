/**
 * @file:   k_process.h
 * @brief:  process management hearder file
 * @author: Yiqing Huang
 * @author: Thomas Reidemeister
 * @date:   2014/01/17
 * NOTE: Assuming there are only two user processes in the system
 */

#ifndef K_PROCESS_H
#define K_PROCESS_H

#include "k_rtx.h"

void process_init(void);                        /* initialize all procs in the system */
k_pcb_node_t *scheduler(void);                  /* pick the pid of the next to run process */
int k_release_processor(void);                  /* kernel release_process function */
int k_enqueue_blocked_process(void);            /* set state of current process to BLOCKED_ON_RESOURCE and add it to the blocked queue */
k_pcb_node_t* k_dequeue_blocked_process(void);  /* dequeue the first available blocked process and return it */
int k_enqueue_ready_process(PCB* p_pcb_old);    /* set state of current process to READY and add it to the ready queue */
int k_enqueue_ready_node(k_pcb_node_t* node);   /* set state of current process to READY and add it to the ready queue */
extern U32 *alloc_stack(U32 size_b);            /* allocate stack for a process */
extern void __rte(void);                        /* pop exception stack frame */
extern void set_test_procs(void);               /* test process initial set up */

#endif /* K_PROCESS_H */
