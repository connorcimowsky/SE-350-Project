#include "usr_proc.h"
#include "uart_polling.h"
#include "printf.h"
#include "string.h"


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

U32 g_wall_clock_start_time = 0;
U32 g_wall_clock_start_time_offset = 0;
U32 g_wall_clock_running = 0;


void set_test_procs(void)
{
    int i;
    for(i = 0; i < NUM_TEST_PROCS; i++) {
        /* ensure that PIDs increase sequentially; start from 0 to account for the null process */
        g_test_procs[i].m_pid = (U32)(i + 1);
        /* give each process an appropriate stack frame size */
        g_test_procs[i].m_stack_size = USR_SZ_STACK;
    }
    
    g_test_procs[0].m_priority = HIGH;
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
    msg_t *p_msg = (msg_t *)request_memory_block();
    p_msg->m_type = MSG_TYPE_DEFAULT;
    str_cpy("hello", p_msg->m_data);
    
    delayed_send(PID_P2, p_msg, 50);
    
#ifdef DEBUG_1
    printf("proc1: sent a message to proc2\n\r");
#endif
    
    while (1) {
        
#ifdef DEBUG_1
        printf("proc1: releasing processor\n\r");
#endif
        
        release_processor();
    }
}

void proc2(void)
{
    int sender_pid;
    msg_t *p_msg = (msg_t *)receive_message(&sender_pid);
    
#ifdef DEBUG_1
    printf("proc2: received a message from PID %d, message: %s\n\r", sender_pid, p_msg->m_data);
#endif
    
    while (1) {
        
#ifdef DEBUG_1
        printf("proc2: releasing processor\n\r");
#endif
        
        release_processor();
    }
}

void proc3(void)
{
    msg_t *p_msg = (msg_t *)request_memory_block();
    p_msg->m_type = MSG_TYPE_CRT_DISP;
    str_cpy("test message 1\n\r", p_msg->m_data);
    delayed_send(PID_CRT, p_msg, 100);
    
    p_msg = (msg_t *)request_memory_block();
    p_msg->m_type = MSG_TYPE_CRT_DISP;
    str_cpy("test message 2\n\r", p_msg->m_data);
    delayed_send(PID_CRT, p_msg, 200);
    
    p_msg = (msg_t *)request_memory_block();
    p_msg->m_type = MSG_TYPE_CRT_DISP;
    str_cpy("test message 3\n\r", p_msg->m_data);
    delayed_send(PID_CRT, p_msg, 300);
    
    set_process_priority(PID_P3, LOW);
    
    while (1) {
        
#ifdef DEBUG_1
        printf("proc3: releasing processor\n\r");
#endif
        
        release_processor();
    }
}

void proc4(void)
{
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

void wall_clock_proc(void)
{
    msg_t *p_msg = (msg_t *)request_memory_block();
    p_msg->m_type = MSG_TYPE_KCD_REG;
    p_msg->m_data[0] = '%';
    p_msg->m_data[1] = 'W';
    p_msg->m_data[2] = 'R';
    p_msg->m_data[3] = '\0';
    
    send_message(PID_KCD, p_msg);
    
    p_msg = (msg_t *)request_memory_block();
    p_msg->m_type = MSG_TYPE_KCD_REG;
    p_msg->m_data[0] = '%';
    p_msg->m_data[1] = 'W';
    p_msg->m_data[2] = 'S';
    p_msg->m_data[3] = '\0';
    
    send_message(PID_KCD, p_msg);
    
    p_msg = (msg_t *)request_memory_block();
    p_msg->m_type = MSG_TYPE_KCD_REG;
    p_msg->m_data[0] = '%';
    p_msg->m_data[1] = 'W';
    p_msg->m_data[2] = 'T';
    p_msg->m_data[3] = '\0';
    
    send_message(PID_KCD, p_msg);
    
    while (1) {
        
        int sender = -1;
        
        p_msg = (msg_t *)receive_message(&sender);
        
        if (sender == PID_CLOCK && p_msg->m_type == MSG_TYPE_WALL_CLK_TICK) {
            
            if (g_wall_clock_running == 1) {
                /* used for signalling ourselves to update */
                msg_t *p_update_msg = (msg_t *)request_memory_block();
                
                /* used for signalling the CRT process to display the current time */
                msg_t *p_display_msg = (msg_t *)request_memory_block();
                
                /* determine the elapsed hours, minutes, and seconds */
                U32 elapsed_time = g_wall_clock_start_time_offset + (get_system_time() - g_wall_clock_start_time);
                U32 s = (elapsed_time / 1000) % 60;
                U32 m = (elapsed_time / (1000 * 60)) % 60;
                U32 h = (elapsed_time / (1000 * 60 * 60)) % 24;
                
                /* display the current time */
                p_display_msg->m_type = MSG_TYPE_CRT_DISP;
                sprintf(p_display_msg->m_data, "%02d:%02d:%02d\n\r", h, m, s);
                send_message(PID_CRT, p_display_msg);
                
                /* update the clock 1 second from now */
                p_update_msg->m_type = MSG_TYPE_WALL_CLK_TICK;
                p_update_msg->m_data[0] = '\0';
                delayed_send(PID_CLOCK, p_update_msg, 1000);
            }
            
        } else {
            
            if (p_msg->m_data[0] == '%' && p_msg->m_data[1] == 'W' && p_msg->m_data[2] == 'R') {
            
                msg_t *p_update_msg = (msg_t *)request_memory_block();
                p_update_msg->m_type = MSG_TYPE_WALL_CLK_TICK;
                p_update_msg->m_data[0] = '\0';
                
                /* reset the start time of the clock */
                g_wall_clock_start_time = get_system_time();
                g_wall_clock_start_time_offset = 0;
                
                /* only begin firing updates if the clock is not running */
                if (g_wall_clock_running == 0) {
                    g_wall_clock_running = 1;
                    
                    /* start firing updates */
                    send_message(PID_CLOCK, p_update_msg);
                }
                
            } else if (p_msg->m_data[0] == '%' && p_msg->m_data[1] == 'W' && p_msg->m_data[2] == 'S') {
                
                int s, m, h;
                int milliseconds;
                char buf[3] = {'\0'};
                
                msg_t *p_update_msg = (msg_t *)request_memory_block();
                p_update_msg->m_type = MSG_TYPE_WALL_CLK_TICK;
                p_update_msg->m_data[0] = '\0';
                
                buf[0] = p_msg->m_data[10];
                buf[1] = p_msg->m_data[11];
                buf[2] = '\0';
                
                s = a_to_i(buf);
                
                buf[0] = p_msg->m_data[7];
                buf[1] = p_msg->m_data[8];
                buf[2] = '\0';
                
                m = a_to_i(buf);
                
                buf[0] = p_msg->m_data[4];
                buf[1] = p_msg->m_data[5];
                buf[2] = '\0';
                
                h = a_to_i(buf);
                
                milliseconds = (h * 3600000) + (m * 60000) + (s * 1000);
                
                /* set the correct state of the clock */
                g_wall_clock_start_time = get_system_time();
                g_wall_clock_start_time_offset = milliseconds;
                
                /* only begin firing updates if the clock is not running */
                if (g_wall_clock_running == 0) {
                    g_wall_clock_running = 1;
                    
                    /* start firing updates */
                    send_message(PID_CLOCK, p_update_msg);
                }
                
            } else if (p_msg->m_data[0] == '%' && p_msg->m_data[1] == 'W' && p_msg->m_data[2] == 'T') {
                
                g_wall_clock_running = 0;
                
            }
            
        }
        
        release_memory_block(p_msg);

    }
}
