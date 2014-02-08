#ifndef K_RTOS_H
#define K_RTOS_H

#include "rtos.h"
#include "k_list.h"
#include "k_queue.h"


/* definitions */

#define NUM_BLOCKS 16
#define BLOCK_SIZE 128
#define RAM_END_ADDR 0x10008000

#define INITIAL_xPSR 0x01000000


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
typedef struct k_pcb_t
{
    /* stack pointer */
    U32 *mp_sp;
    
    /* process id */
    U32 m_pid;
    
    /* priority */
    PRIORITY_E m_priority;
    
    /* state */
    PROC_STATE_E m_state;
    
    /* message queue */
    k_queue_t m_msg_queue;
} k_pcb_t;


/* pcb node */
typedef struct k_pcb_node_t {
    struct k_pcb_node_t *mp_next;
    k_pcb_t *mp_pcb;
} k_pcb_node_t;


/* message type */
typedef enum {
	DEFAULT = 0,
	KCD_REG
} MSG_TYPE_E;


/* message envelope */
typedef struct k_msg_t {
	struct k_msg_t *mp_next;
	U32 m_sender_pid;
	U32 m_destination_pid;
	MSG_TYPE_E m_type;
	char *mp_data;
} k_msg_t;


/* external variables */

/* end address of the memory image */
extern unsigned int Image$$RW_IRAM1$$ZI$$Limit;

/* current bottom of (incrementing) heap */
extern k_list_t *gp_heap;

/* current top of (decrementing) stack, 8-byte aligned */
extern U32 *gp_stack;

/* process initialization table */
extern PROC_INIT g_proc_table[NUM_PROCS];

/* array of nodes pointing to PCBs */
extern k_pcb_node_t **gp_pcb_nodes;

/* the process whose state is EXECUTING */
extern k_pcb_node_t *gp_current_process;

/* array of queues for processes that are READY, one for each priority */
extern k_queue_t *gp_ready_queue[NUM_PRIORITIES];

/* array of queues for processes that are BLOCKED_ON_RESOURCE, one for each priority */
extern k_queue_t *gp_blocked_queue[NUM_PRIORITIES];


#endif /* K_RTOS_H */
