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

/* maximum number of entries in the keyboard command registry */
#define NUM_KCD_REG 10

/* maximum number of characters in a keyboard command identifier */
#define KCD_REG_LENGTH 10

/* the maximum number of characters we allow as input */
#define INPUT_BUFFER_SIZE (BLOCK_SIZE - MSG_HEADER_OFFSET)


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
    /* pointer to the next PCB */
    struct k_pcb_t *mp_next;
    
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


/* message envelope */
typedef struct k_msg_t {
    struct k_msg_t *mp_next;
    U32 m_expiry;
    U32 m_sender_pid;
    U32 m_recipient_pid;
} k_msg_t;


/* size of message envelope header */
#define MSG_HEADER_OFFSET sizeof(k_msg_t)


/* keyboard command registry entry */
typedef struct k_kcd_reg_t {
    /* pointer to the next keyboard command registry entry */
    struct k_kcd_reg_t *mp_next;
    
    /* the keyboard command identifier */
    char m_id[KCD_REG_LENGTH];
    
    /* the process identifier associated with this entry */
    U32 m_pid;
    
    /* indicates whether this entry is active or not */
    U32 m_active;
} k_kcd_reg_t;


/* external variables */

/* end address of the memory image */
extern unsigned int Image$$RW_IRAM1$$ZI$$Limit;

/* current bottom of (incrementing) heap */
extern k_list_t *gp_heap;

/* current top of (decrementing) stack, 8-byte aligned */
extern U32 *gp_stack;

/* process initialization table */
extern PROC_INIT g_proc_table[NUM_PROCS];

/* array of PCBs */
extern k_pcb_t **gp_pcbs;

/* the process whose state is EXECUTING */
extern k_pcb_t *gp_current_process;

/* array of queues for processes that are READY, one for each priority */
extern k_queue_t *gp_ready_queue[NUM_PRIORITIES];

/* array of queues for processes that are BLOCKED_ON_RESOURCE, one for each priority */
extern k_queue_t *gp_blocked_on_memory_queue[NUM_PRIORITIES];

/* the registry of keyboard command entries */
extern k_list_t g_kcd_reg;


#endif /* K_RTOS_H */
