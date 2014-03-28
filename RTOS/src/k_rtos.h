#ifndef K_RTOS_H
#define K_RTOS_H

#include "rtos.h"
#include "list.h"
#include "queue.h"


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

/* the number of logged messages stored in the circular message logs */
#define MSG_LOG_SIZE 4

/* the number of characters stored by message log entries */
#define MSG_LOG_LEN 16


/* process state */
typedef enum {
    NEW = 0,
    READY,
    EXECUTING,
    BLOCKED_ON_MEMORY,
    BLOCKED_ON_RECEIVE
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
    queue_t m_msg_queue;
} k_pcb_t;


/* message envelope */
typedef struct k_msg_t {
    struct k_msg_t *mp_next;
    U32 m_expiry;
    U32 m_sender_pid;
    U32 m_recipient_pid;
    MSG_TYPE_E m_type;
    char m_data[1];
} k_msg_t;


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


typedef struct k_msg_log_t {
    U32 m_sender_pid;
    U32 m_recipient_pid;
    MSG_TYPE_E m_type;
    char m_text[MSG_LOG_LEN];
    U32 m_time_stamp;
} k_msg_log_t;


/* external variables */

/* end address of the memory image */
extern unsigned int Image$$RW_IRAM1$$ZI$$Limit;

/* current bottom of (incrementing) heap */
extern list_t *gp_heap;

/* current top of (decrementing) stack, 8-byte aligned */
extern U32 *gp_stack;

/* process initialization table */
extern PROC_INIT g_proc_table[NUM_PROCS];

/* array of PCBs */
extern k_pcb_t *g_pcbs[NUM_PROCS];

/* the process whose state is EXECUTING */
extern k_pcb_t *gp_current_process;

/* array of queues for processes that are READY, one for each priority */
extern queue_t g_ready_queue[NUM_PRIORITIES];

/* array of queues for processes that are BLOCKED_ON_MEMORY, one for each priority */
extern queue_t g_blocked_on_memory_queue[NUM_PRIORITIES];

/* array of queues for processes that are BLOCKED_ON_RECEIVE, one for each priority */
extern queue_t g_blocked_on_receive_queue[NUM_PRIORITIES];

/* registry of keyboard command entries */
extern list_t g_kcd_reg;

/* circular buffer of recently sent messages */
extern k_msg_log_t g_sent_msg_log[MSG_LOG_SIZE];

/* circular buffer of recently received messages */
extern k_msg_log_t g_received_msg_log[MSG_LOG_SIZE];


#endif /* K_RTOS_H */
