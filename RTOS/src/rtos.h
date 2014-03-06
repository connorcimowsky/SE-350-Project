#ifndef RTOS_H
#define RTOS_H


/* definitions */

#define RTOS_ERR -1
#define RTOS_OK  0

#define NULL 0
#define NUM_TEST_PROCS 6

#ifdef DEBUG_0
#define USR_SZ_STACK 0x200 /* 512 bytes */
#else
#define USR_SZ_STACK 0x100 /* 256 bytes */
#endif


/* integer types */
typedef unsigned char U8;
typedef unsigned int U32;


/* process identifiers */
typedef enum {
    PID_NULL = 0,
    PID_P1,
    PID_P2,
    PID_P3,
    PID_P4,
    PID_P5,
    PID_P6,
    PID_A,
    PID_B,
    PID_C,
    PID_SET_PRIO,
    PID_CLOCK,
    PID_KCD,
    PID_CRT,
    PID_TIMER_IPROC,
    PID_UART_IPROC,
    NUM_PROCS
} PID_E;


/* process priority */
typedef enum {
    HIGHEST = 0,
    HIGH,
    MEDIUM,
    LOW,
    LOWEST,
    NUM_PRIORITIES
} PRIORITY_E;


/* process initialization table item */
typedef struct proc_init
{
    /* process id */
    int m_pid;

    /* priority */
    PRIORITY_E m_priority;
    
    /* size of stack in words */
    int m_stack_size;
    
    /* entry point of the process */
    void (*mpf_start_pc) ();
} PROC_INIT;


/* message type */
typedef enum {
    MSG_TYPE_DEFAULT = 0,
    MSG_TYPE_KCD_REG,
    MSG_TYPE_KCD_DISPATCH,
    MSG_TYPE_CRT_DISP,
    MSG_TYPE_WALL_CLK_TICK
} MSG_TYPE_E;


/* user-facing message envelope */
typedef struct msg_t {
    MSG_TYPE_E m_type;
    char m_data[1];
} msg_t;


/* user-facing api */

#define __SVC_0  __svc_indirect(0)

extern void k_rtos_init(void);
#define rtos_init() _rtos_init((U32)k_rtos_init)
extern void __SVC_0 _rtos_init(U32 p_func);

extern void *k_request_memory_block(void);
#define request_memory_block() _request_memory_block((U32)k_request_memory_block)
extern void *_request_memory_block(U32 p_func) __SVC_0;

extern int k_release_memory_block(void *);
#define release_memory_block(p_mem_blk) _release_memory_block((U32)k_release_memory_block, p_mem_blk)
extern int _release_memory_block(U32 p_func, void *p_mem_blk) __SVC_0;

extern int k_release_processor(void);
#define release_processor() _release_processor((U32)k_release_processor)
extern int __SVC_0 _release_processor(U32 p_func);

extern int k_set_process_priority(int, int);
#define set_process_priority(pid, priority) _set_process_priority((U32)k_set_process_priority, pid, priority)
extern int __SVC_0 _set_process_priority(U32 p_func, int pid, int priority);

extern int k_get_process_priority(int);
#define get_process_priority(pid) _get_process_priority((U32)k_get_process_priority, pid)
extern int __SVC_0 _get_process_priority(U32 p_func, int pid);

extern int k_send_message(int, void *);
#define send_message(recipient_pid, p_msg) _send_message((U32)k_send_message, recipient_pid, p_msg)
extern int __SVC_0 _send_message(U32 p_func, int recipient_pid, void *p_msg);

extern void *k_receive_message(int *);
#define receive_message(p_sender_pid) _receive_message((U32)k_receive_message, p_sender_pid)
extern void *_receive_message(U32 p_func, int *p_sender_pid) __SVC_0;

extern int k_delayed_send(int, void *, int);
#define delayed_send(recipient_pid, p_msg, delay) _delayed_send((U32)k_delayed_send, recipient_pid, p_msg, delay)
extern int __SVC_0 _delayed_send(U32 p_func, int recipient_pid, void *p_msg, int delay);

extern U32 k_get_system_time(void);
#define get_system_time() _get_system_time((U32)k_get_system_time)
extern U32 __SVC_0 _get_system_time(U32 p_func);


#endif /* RTOS_H */
