#include "usr_proc.h"
#include "uart_polling.h"

#ifdef DEBUG_0
#include "printf.h"
#endif


/* global variables */

PROC_INIT g_test_procs[NUM_TEST_PROCS];

/* pointers to memory blocks returned by request_memory_block() */
void *gp_mem_blks[16];

/* the index of gp_mem_blks pointing to the most recently requested memory block */
int g_mem_blk_index = 0;

/* the pid of the most recently executed process */
int g_previous_pid = -1;

/* an array of flags indicating the result of each test case */
int g_success_flags[6] = {1, 1, 1, 1, 1, 1};


void set_test_procs(void)
{
    int i;
    for(i = 0; i < NUM_TEST_PROCS; i++) {
        /* ensure that PIDs increase sequentially; start from 0 to account for the null process */
        g_test_procs[i].m_pid = (U32)(i + 1);
        /* give each process an appropriate stack frame size */
        g_test_procs[i].m_stack_size = USR_SZ_STACK;
    }
    
    g_test_procs[0].m_priority = MEDIUM;
    g_test_procs[0].mpf_start_pc = &proc1;
    
    g_test_procs[1].m_priority = HIGH;
    g_test_procs[1].mpf_start_pc = &proc2;
    
    g_test_procs[2].m_priority = HIGHEST;
    g_test_procs[2].mpf_start_pc = &proc3;
    
    g_test_procs[3].m_priority = LOW;
    g_test_procs[3].mpf_start_pc = &proc4;
    
    g_test_procs[4].m_priority = LOW;
    g_test_procs[4].mpf_start_pc = &proc5;
    
    g_test_procs[5].m_priority = LOW;
    g_test_procs[5].mpf_start_pc = &proc6;
}

void proc1(void)
{
    int ret_val;
    msg_t *p_msg = (msg_t *)request_memory_block();
    p_msg->m_type = MSG_TYPE_DEFAULT;
    p_msg->mp_data = "hello";
    
    printf("proc1: sending a message to proc2\n\r");
    
    ret_val = delayed_send(PID_P2, p_msg, 50);
    
#ifdef DEBUG_1
    printf("proc1: send_message returned %d\n\r", ret_val);
#endif
    
    while (1) {
        printf("proc1: releasing processor\n\r");
        release_processor();
    }
}

void proc2(void)
{
    int sender_pid;
    msg_t *p_msg = (msg_t *)receive_message(&sender_pid);
    
#ifdef DEBUG_1
    printf("proc2: received a message from PID %d, message: %s\n", sender_pid, p_msg->mp_data);
#endif
    
    while (1) {
        printf("proc2: releasing processor\n\r");
        release_processor();
    }
}

void proc3(void)
{
    msg_t *p_msg = (msg_t *)request_memory_block();
    p_msg->m_type = MSG_TYPE_CRT_DISP;
    p_msg->mp_data = "test message 1\n\r";
    delayed_send(PID_CRT, p_msg, 100);
    
    p_msg = (msg_t *)request_memory_block();
    p_msg->m_type = MSG_TYPE_CRT_DISP;
    p_msg->mp_data = "test message 2\n\r";
    delayed_send(PID_CRT, p_msg, 200);
    
    p_msg = (msg_t *)request_memory_block();
    p_msg->m_type = MSG_TYPE_CRT_DISP;
    p_msg->mp_data = "test message 3\n\r";
    delayed_send(PID_CRT, p_msg, 300);
    
    while (1) {
        printf("proc3: releasing processor\n\r");
        release_processor();
    }
}

void proc4(void)
{
    while (1) {
        printf("proc4: releasing processor\n\r");
        release_processor();
    }
}

void proc5(void)
{
    while (1) {
        printf("proc5: releasing processor\n\r");
        release_processor();
    }
}

void proc6(void)
{
    while (1) {
        printf("proc6: releasing processor\n\r");
        release_processor();
    }
}
