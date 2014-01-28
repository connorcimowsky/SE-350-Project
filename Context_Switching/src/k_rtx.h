#ifndef K_RTX_H
#define K_RTX_H

#include "rtx.h"
#include "k_list.h"
#include "k_queue.h"


/* definitions */

#define NUM_BLOCKS 16
#define BLOCK_SIZE 128
#define RAM_END_ADDR 0x10008000

#define INITIAL_xPSR 0x01000000

#ifdef DEBUG_0
#define USR_SZ_STACK 0x200 /* 512 bytes */
#else
#define USR_SZ_STACK 0x100 /* 256 bytes */
#endif


/* process state */
typedef enum {
    NEW = 0,
    READY,
    BLOCKED_ON_RESOURCE,
    WAITING_FOR_MESSAGE,
    EXECUTING,
    INTERRUPTED
} PROC_STATE_E;


/* process control block */
typedef struct pcb 
{
    /* stack pointer */
	U32 *mp_sp;
    
    /* process id */
	U32 m_pid;
    
    /* priority */
    PRIORITY_E m_priority;
    
    /* state */
	PROC_STATE_E m_state;
} PCB;


/* pcb node */
typedef struct k_pcb_node_t {
    struct k_pcb_node_t *next;
    PCB *pcb;
} k_pcb_node_t;


/* external variables */

/* end address of the memory image */
extern unsigned int Image$$RW_IRAM1$$ZI$$Limit;

/* current bottom of (incrementing) heap */
extern k_list_t *gp_heap;

/* current top of (decrementing) stack, 8-byte aligned */
extern U32 *gp_stack;

/* process initialization table */
extern PROC_INIT g_proc_table[NUM_TEST_PROCS];

/* array of nodes pointing to PCBs */
extern k_pcb_node_t **gp_pcb_nodes;

/* the process whose state is EXECUTING */
extern k_pcb_node_t *gp_current_process;

/* array of priority queues, one for each priority */
extern k_queue_t *gp_ready_queue[NUM_PRIORITIES];

/* queue for processes that are BLOCKED_ON_RESOURCE */
extern k_queue_t *gp_blocked_queue;


#endif /* K_RTX_H */
