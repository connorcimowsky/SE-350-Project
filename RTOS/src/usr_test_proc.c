#include "usr_test_proc.h"
#include "uart_polling.h"
#include "printf.h"
#include "string.h"


/* global variables */

/* test process initialization table */
PROC_INIT g_test_procs[NUM_TEST_PROCS];

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
    
    g_test_procs[0].m_priority = HIGHEST;
    g_test_procs[0].mpf_start_pc = &proc1;
    
    g_test_procs[1].m_priority = HIGH;
    g_test_procs[1].mpf_start_pc = &proc2;
    
    g_test_procs[2].m_priority = HIGH;
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
    msg_t *p_msg = NULL;
    int sender_pid = -1;
    int ret_val = -1;
    
    
#ifdef DEBUG_0
    printf("G019_test: START\n\r");
    printf("G019_test: total 6 tests\n\r");
#endif
    
    
    g_previous_pid = PID_P1;
    
    
    /**
     * TEST CASE 1: this test ensures correct blocking behavior when receiving messages
     *
     * DESCRIPTION: proc1 requests a message
     * PRE: the state of proc1 is EXECUTING
     * POST: the state of proc1 is BLOCKED_ON_RECEIVE; preempt to proc2
     */
    
    p_msg = (msg_t *)receive_message(&sender_pid);
    
    /* proc1 should be preempted to proc2 at this point */
    
    
    /* END TEST CASE 2 */
    
    g_success_flags[1] &= (g_previous_pid == PID_P2);
    g_success_flags[1] &= (sender_pid == PID_P2);
    g_success_flags[1] &= (str_cmp(p_msg->m_data, "test message 1") == 1);
    g_success_flags[1] &= (release_memory_block(p_msg) == RTOS_OK);
    
    g_previous_pid = PID_P1;
    
#ifdef DEBUG_0
    printf("G019_test: test 2 ");
    printf(g_success_flags[1] == 1 ? "OK\n\r" : "FAIL\n\r");
#endif
    
    
    /**
     * TEST CASE 3: this test ensures correct preemption behavior of timing services
     *
     * DESCRIPTION: proc1 sends a delayed message to proc3
     * PRE: the state of proc1 is EXECUTING
     * POST: the state of proc1 is EXECUTING; no preemption should occur until the message is dispatched
     */
    
    p_msg = (msg_t *)request_memory_block();
    p_msg->m_type = MSG_TYPE_DEFAULT;
    str_cpy("test message 2", p_msg->m_data);
    
    /* this system call should not block */
    ret_val = delayed_send(PID_P3, p_msg, 3000);
    
    
    /* END TEST CASE 3 */
    
    g_success_flags[2] &= (g_previous_pid == PID_P1);
    g_success_flags[2] &= (ret_val == RTOS_OK);
    
    g_previous_pid = PID_P1;
    
#ifdef DEBUG_0
    printf("G019_test: test 3 ");
    printf(g_success_flags[2] == 1 ? "OK\n\r" : "FAIL\n\r");
#endif
    
    
    /* this will cause us to preempt to proc2 */
    set_process_priority(PID_P1, LOW);
    
    
    while (1) {
        
#ifdef DEBUG_1
        printf("proc1: releasing processor\n\r");
#endif
        
        release_processor();
    }
}

void proc2(void)
{
    msg_t *p_msg = NULL;
    
    
    /* END TEST CASE 1 */
    
    g_success_flags[0] &= (g_previous_pid == PID_P1);
    g_previous_pid = PID_P2;
    
#ifdef DEBUG_0
    printf("G019_test: test 1 ");
    printf(g_success_flags[0] == 1 ? "OK\n\r" : "FAIL\n\r");
#endif
    
    
    /**
     * TEST CASE 2: this test ensures correct preemption behavior when sending messages
     *
     * DESCRIPTION: proc2 sends a message to proc1
     * PRE: the state of proc1 is BLOCKED_ON_RECEIVE
     * POST: the state of proc1 is READY; preempt to proc1
     */
    
    p_msg = (msg_t *)request_memory_block();
    p_msg->m_type = MSG_TYPE_DEFAULT;
    str_cpy("test message 1", p_msg->m_data);
    
    send_message(PID_P1, p_msg);
    
    /* proc2 should be preempted to proc1 at this point */
    
    
    while (1) {
        
#ifdef DEBUG_1
        printf("proc2: releasing processor\n\r");
#endif
        
        release_processor();
    }
}

void proc3(void)
{
    msg_t *p_msg = NULL;
    int sender_pid = -1;
    
    
    /**
     * TEST CASE 4: this test ensures correct non-blocking behavior when receiving messages
     *
     * DESCRIPTION: proc3 receives the delayed message from proc1; since the message should be available, it should not get blocked
     * PRE: the state of proc3 is EXECUTING
     * POST: the state of proc3 is EXECUTING
     */
    
    g_success_flags[3] &= (g_previous_pid == PID_P1);
    g_previous_pid = PID_P3;
    
    /* this system call should not block, since we should only have been preempted if the message was delivered */
    p_msg = (msg_t *)receive_message(&sender_pid);
    
    
    /* END TEST CASE 4 */
    
    g_success_flags[3] &= (g_previous_pid == PID_P3);
    g_success_flags[3] &= (sender_pid == PID_P1);
    g_success_flags[3] &= (str_cmp(p_msg->m_data, "test message 2") == 1);
    g_success_flags[3] &= (release_memory_block(p_msg) == RTOS_OK);
    
    g_previous_pid = PID_P3;
    
#ifdef DEBUG_0
    printf("G019_test: test 4 ");
    printf(g_success_flags[3] == 1 ? "OK\n\r" : "FAIL\n\r");
#endif
    
    
    /* this will cause us to preempt to proc4 */
    set_process_priority(PID_P4, HIGHEST);
    
    
    while (1) {
        
#ifdef DEBUG_1
        printf("proc3: releasing processor\n\r");
#endif
        
        release_processor();
    }
}

void proc4(void)
{
    msg_t *p_msg = NULL;
    int ret_val = -1;
    int sender_pid = -1;
    int i;
    int num_successes = 0;
    
    
    /* register proc4 for the '%YES' command */
    
    p_msg = (msg_t *)request_memory_block();
    p_msg->m_type = MSG_TYPE_KCD_REG;
    p_msg->m_data[0] = '%';
    p_msg->m_data[1] = 'Y';
    p_msg->m_data[2] = 'E';
    p_msg->m_data[3] = 'S';
    p_msg->m_data[4] = '\0';
    
    send_message(PID_KCD, p_msg);
    
    
    /**
     * TEST CASE 5: this test ensures correct behaviour of the CRT process
     *
     * DESCRIPTION: proc4 sends a message to the CRT process containing a string to be displayed on UART0
     * PRE: the state of proc4 is EXECUTING
     * POST: the state of proc4 is EXECUTING
     */
    
    g_success_flags[4] &= (g_previous_pid == PID_P3);
    g_previous_pid = PID_P4;
    
    p_msg = (msg_t *)request_memory_block();
    p_msg->m_type = MSG_TYPE_CRT_DISP;
    str_cpy("If you can see this, please enter '%YES' and then press the return key.\n\r", p_msg->m_data);
    
    ret_val = send_message(PID_CRT, p_msg);
    
    g_success_flags[4] &= (g_previous_pid == PID_P4);
    g_success_flags[4] &= (ret_val == RTOS_OK);
    g_previous_pid = PID_P4;
    
    
    /**
     * TEST CASE 6: this test ensures correct behaviour of the KCD process
     *
     * DESCRIPTION: a message should be received from the KCD process; if it matches '%YES', mark test 5 as OK too
     * PRE: the state of proc4 is EXECUTING
     * POST: the state of proc4 is EXECUTING and a message has been received from the KCD process
     */
    
    p_msg = receive_message(&sender_pid);
    
    
    /* END TEST CASE 5 */
    
    g_success_flags[4] &= (str_cmp(p_msg->m_data, "%YES") == 1);
    g_success_flags[4] &= (release_memory_block(p_msg) == RTOS_OK);
    
#ifdef DEBUG_0
    printf("G019_test: test 5 ");
    printf(g_success_flags[4] == 1 ? "OK\n\r" : "FAIL\n\r");
#endif
    
    
    /* END TEST CASE 6 */
    
    g_success_flags[5] &= (g_previous_pid == PID_P4);
    g_success_flags[5] &= (sender_pid == PID_KCD);
    g_previous_pid = PID_P5;
    
#ifdef DEBUG_0
    printf("G019_test: test 6 ");
    printf(g_success_flags[5] == 1 ? "OK\n\r" : "FAIL\n\r");
#endif
    
    
    /* END TESTS */
    
    for (i = 0; i < 6; i++) {
        num_successes += g_success_flags[i];
    }
    
#ifdef DEBUG_0
    printf("G019_test: %d/%d tests OK\n\r", num_successes, 6);
    printf("G019_test: %d/%d tests FAIL\n\r", (6 - num_successes), 6);
    
    printf("G019_test: END\n\r");
#endif
    
    
    while (1) {
        
#ifdef DEBUG_1
        printf("proc4: releasing processor\n\r");
#endif
        
        release_processor();
    }
}

void proc5(void)
{
    while (1) {
        
#ifdef DEBUG_1
        printf("proc5: releasing processor\n\r");
#endif
        
        release_processor();
    }
}

void proc6(void)
{
    while (1) {
        
#ifdef DEBUG_1
        printf("proc6: releasing processor\n\r");
#endif
        
        release_processor();
    }
}
