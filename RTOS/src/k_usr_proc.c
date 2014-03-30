#include <LPC17xx.h>
#include "k_usr_proc.h"
#include "queue.h"
#include "printf.h"
#include "string.h"

#define NUM_TRIALS 10
#define TIMER_RESET (1 << 1) | (1 << 0);
#define TIMER_START (0 << 1) | (1 << 0);


/* global variables */

/* the system time at which the wall clock was started */
U32 g_wall_clock_start_time = 0;

/* the start time specified by the '%WS hh:mm:ss' command */
U32 g_wall_clock_start_time_offset = 0;

/* flag to indicate if the wall clock is currently running */
U32 g_wall_clock_running = 0;


void wall_clock_proc(void)
{
    msg_t *p_msg = (msg_t *)request_memory_block();
    p_msg->m_type = MSG_TYPE_KCD_REG;
    str_cpy("%WR", p_msg->m_data);
    
    send_message(PID_KCD, p_msg);
    
    p_msg = (msg_t *)request_memory_block();
    p_msg->m_type = MSG_TYPE_KCD_REG;
    str_cpy("%WS", p_msg->m_data);
    
    send_message(PID_KCD, p_msg);
    
    p_msg = (msg_t *)request_memory_block();
    p_msg->m_type = MSG_TYPE_KCD_REG;
    str_cpy("%WT", p_msg->m_data);
    
    send_message(PID_KCD, p_msg);
    
    while (1) {
        
        int sender = -1;
        
        p_msg = (msg_t *)receive_message(&sender);
        
        if (sender == PID_CLOCK && p_msg->m_type == MSG_TYPE_WALL_CLK_TICK) {
            
            if (g_wall_clock_running == 1) {
                /* used for signalling the CRT process to display the current time */
                msg_t *p_display_msg = (msg_t *)request_memory_block();
                
                /* used for signalling ourselves to update */
                msg_t *p_update_msg = NULL;
                
                /* determine the elapsed hours, minutes, and seconds */
                U32 elapsed_time = g_wall_clock_start_time_offset + (get_system_time() - g_wall_clock_start_time);
                U32 s = (elapsed_time / 1000) % 60;
                U32 m = (elapsed_time / (1000 * 60)) % 60;
                U32 h = (elapsed_time / (1000 * 60 * 60)) % 24;
                
                /* display the current time */
                p_display_msg->m_type = MSG_TYPE_CRT_DISP;
                sprintf(p_display_msg->m_data, "%c[s%c[%dA%c[%dC%c[%dD%c[36;1m%02d%c[37m:%c[36;1m%02d%c[37m:%c[36;1m%02d%c[u%c[0m", 0x1B, 0x1B, 2000, 0x1B, 2000, 0x1B, 7, 0x1B, h, 0x1B, 0x1B, m, 0x1B, 0x1B, s, 0x1B, 0x1B);
                send_message(PID_CRT, p_display_msg);
                
                /* update the clock 1 second from now */
                p_update_msg = (msg_t *)request_memory_block();
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
                
                if (p_msg->m_data[3] == ' '
                 && p_msg->m_data[4] >= '0' && p_msg->m_data[4] <= '9'
                 && p_msg->m_data[5] >= '0' && p_msg->m_data[5] <= '9'
                 && p_msg->m_data[6] == ':'
                 && p_msg->m_data[7] >= '0' && p_msg->m_data[7] <= '9'
                 && p_msg->m_data[8] >= '0' && p_msg->m_data[8] <= '9'
                 && p_msg->m_data[9] == ':'
                 && p_msg->m_data[10] >= '0' && p_msg->m_data[10] <= '9'
                 && p_msg->m_data[11] >= '0' && p_msg->m_data[11] <= '9'
                 && p_msg->m_data[12] == '\0') {
                    int s, m, h;
                    int milliseconds;
                    char buf[3] = {'\0'};
                    
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
                    
                    if (s >= 0 && s < 60
                     && m >= 0 && m < 60
                     && h >= 0 && h < 24) {
                        milliseconds = (h * 3600000) + (m * 60000) + (s * 1000);
                        
                        /* set the correct state of the clock */
                        g_wall_clock_start_time = get_system_time();
                        g_wall_clock_start_time_offset = milliseconds;
                        
                        /* only begin firing updates if the clock is not running */
                        if (g_wall_clock_running == 0) {
                            msg_t *p_update_msg = (msg_t *)request_memory_block();
                            p_update_msg->m_type = MSG_TYPE_WALL_CLK_TICK;
                            p_update_msg->m_data[0] = '\0';
                            
                            g_wall_clock_running = 1;
                            
                            /* start firing updates */
                            send_message(PID_CLOCK, p_update_msg);
                        }
                    } else {
                        msg_t *p_display_msg = (msg_t *)request_memory_block();
                        p_display_msg->m_type = MSG_TYPE_CRT_DISP;
                        str_cpy("Error: invalid time.\n\r", p_display_msg->m_data);
                        
                        send_message(PID_CRT, p_display_msg);
                     }
                } else {
                    msg_t *p_display_msg = (msg_t *)request_memory_block();
                    p_display_msg->m_type = MSG_TYPE_CRT_DISP;
                    str_cpy("Error: illegal input.\n\r", p_display_msg->m_data);
                    
                    send_message(PID_CRT, p_display_msg);
                }
                
            } else if (p_msg->m_data[0] == '%' && p_msg->m_data[1] == 'W' && p_msg->m_data[2] == 'T') {
                
                g_wall_clock_running = 0;
                
            }
            
        }
        
        release_memory_block(p_msg);

    }
}

void set_priority_proc(void)
{
    /* register ourselves for the %C command */
    
    msg_t *p_msg = (msg_t *)request_memory_block();
    p_msg->m_type = MSG_TYPE_KCD_REG;
    str_cpy("%C", p_msg->m_data);
    
    send_message(PID_KCD, p_msg);
    
    /* start receiving and parsing messages */
    
    while (1) {
        char pid_buf[3] = {'\0'};
        char priority_buf[2] = {'\0'};
        
        int pid = 0;
        int priority = 0;
        
        int ret_val = 0;
        
        p_msg = receive_message(NULL);
        
        if (p_msg->m_data[2] == ' '
         && p_msg->m_data[3] >= '0' && p_msg->m_data[3] <= '9'
         && p_msg->m_data[4] == ' '
         && p_msg->m_data[5] >= '0' && p_msg->m_data[5] <= '9'
         && p_msg->m_data[6] == '\0') {
            pid_buf[0] = p_msg->m_data[3];
            priority_buf[0] = p_msg->m_data[5];
         } else if (p_msg->m_data[2] == ' '
         && p_msg->m_data[3] >= '0' && p_msg->m_data[3] <= '9'
         && p_msg->m_data[4] >= '0' && p_msg->m_data[4] <= '9'
         && p_msg->m_data[5] == ' '
         && p_msg->m_data[6] >= '0' && p_msg->m_data[5] <= '9'
         && p_msg->m_data[7] == '\0') {
            pid_buf[0] = p_msg->m_data[3];
            pid_buf[1] = p_msg->m_data[4];
            priority_buf[0] = p_msg->m_data[6];
         } else {
            msg_t *p_display_msg = (msg_t *)request_memory_block();
            p_display_msg->m_type = MSG_TYPE_CRT_DISP;
            str_cpy("Error: illegal parameters.\n\r", p_display_msg->m_data);
            
            send_message(PID_CRT, p_display_msg);
            
            release_memory_block(p_msg);
            
            continue;
        }
        
        pid = a_to_i(pid_buf);
        priority = a_to_i(priority_buf);
        
        ret_val = set_process_priority(pid, priority);
        
        if (ret_val != RTOS_OK) {
            msg_t *p_display_msg = (msg_t *)request_memory_block();
            p_display_msg->m_type = MSG_TYPE_CRT_DISP;
            str_cpy("Error: illegal PID or priority.\n\r", p_display_msg->m_data);
            
            send_message(PID_CRT, p_display_msg);
        }
        
        release_memory_block(p_msg);
    }
}

void stress_test_a(void)
{
    int counter = 0;
    
    /* register ourselves for the %Z command */
    
    msg_t *p_msg = (msg_t *)request_memory_block();
    p_msg->m_type = MSG_TYPE_KCD_REG;
    str_cpy("%Z", p_msg->m_data);
    
    send_message(PID_KCD, p_msg);
    
    while (1) {
        p_msg = receive_message(NULL);
        
        if (p_msg->m_data[0] == '%' && p_msg->m_data[1] == 'Z') {
            release_memory_block(p_msg);
            break;
        } else {
            release_memory_block(p_msg);
        }
    }
    
    while (1) {
        p_msg = (msg_t *)request_memory_block();
        p_msg->m_type = MSG_TYPE_COUNT_REPORT;
        p_msg->m_data[0] = counter;
        
        send_message(PID_B, p_msg);
        
        counter += 1;
        
        release_processor();
    }
}

void stress_test_b(void)
{
    msg_t *p_msg = NULL;
    
    while (1) {
        p_msg = receive_message(NULL);
        send_message(PID_C, p_msg);
    }
}

void stress_test_c(void)
{
    msg_t *p_msg = NULL;
    
    queue_t hibernate_queue;
    queue_init(&hibernate_queue);
    
    while (1) {
        if (is_queue_empty(&hibernate_queue)) {
            p_msg = receive_message(NULL);
        } else {
            p_msg = (msg_t *)((U8 *)dequeue(&hibernate_queue) + MSG_HEADER_OFFSET);
        }
        
        if (p_msg->m_type == MSG_TYPE_COUNT_REPORT) {
            if (p_msg->m_data[0] % 20 == 0) {
                msg_t *p_wakeup_msg = NULL;
                
                p_msg->m_type = MSG_TYPE_CRT_DISP;
                str_cpy("Process C\n\r", p_msg->m_data);
                send_message(PID_CRT, p_msg);
                
                p_wakeup_msg = (msg_t *)request_memory_block();
                p_wakeup_msg->m_type = MSG_TYPE_WAKEUP_10;
                p_wakeup_msg->m_data[0] = '\0';
                
                delayed_send(PID_C, p_wakeup_msg, 10000);
                
                while (1) {
                    p_msg = receive_message(NULL);
                    
                    if (p_msg->m_type == MSG_TYPE_WAKEUP_10) {
                        break;
                    } else {
                        enqueue((node_t *)((U8 *)p_msg - MSG_HEADER_OFFSET), &hibernate_queue);
                    }
                }
            }
        }
        
        release_memory_block(p_msg);
        
        release_processor();
    }
}

void profiler_proc(void)
{
    msg_t *p_msg = (msg_t *)request_memory_block();
    p_msg->m_type = MSG_TYPE_KCD_REG;
    str_cpy("%P", p_msg->m_data);
    
    send_message(PID_KCD, p_msg);
    
    while (1) {
        int i;
        void *mem_blks[NUM_TRIALS];
        U32 finish_time_ns = 0;
        int sender_pid;
        
        /* wait for a message from the KCD */
        msg_t *p_msg = (msg_t *)receive_message(NULL);
        release_memory_block(p_msg);
        
        LPC_TIM1->TCR = TIMER_RESET;
        LPC_TIM1->TCR = TIMER_START;
        
        for (i = 0; i < NUM_TRIALS; i++) {
            mem_blks[i] = request_memory_block();
        }
        
        finish_time_ns = 10 * LPC_TIM1->TC;
        printf("request_memory_block:\n\r\tnumber of trials: %d\n\r\ttotal time (ns): %d\n\r\tapproximate time per trial (ns): %d\n\r", NUM_TRIALS, finish_time_ns, (finish_time_ns / NUM_TRIALS));
        
        LPC_TIM1->TCR = TIMER_RESET;
        LPC_TIM1->TCR = TIMER_START;
        
        for (i = 0; i < NUM_TRIALS; i++) {
            send_message(PID_PROFILER, mem_blks[i]);
        }
        
        finish_time_ns = 10 * LPC_TIM1->TC;
        printf("send_message:\n\r\tnumber of trials: %d\n\r\ttotal time (ns): %d\n\r\tapproximate time per trial (ns): %d\n\r", NUM_TRIALS, finish_time_ns, (finish_time_ns / NUM_TRIALS));
        
        LPC_TIM1->TCR = TIMER_RESET;
        LPC_TIM1->TCR = TIMER_START;
        
        for (i = 0; i < NUM_TRIALS; i++) {
            receive_message(&sender_pid);
        }
        
        finish_time_ns = 10 * LPC_TIM1->TC;
        printf("receive_message:\n\r\tnumber of trials: %d\n\r\ttotal time (ns): %d\n\r\tapproximate time per trial (ns): %d\n\r", NUM_TRIALS, finish_time_ns, (finish_time_ns / NUM_TRIALS));
        
        for (i = 0; i < NUM_TRIALS; i++) {
            release_memory_block(mem_blks[i]);
        }
    }
}
