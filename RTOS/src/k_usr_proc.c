#include "k_usr_proc.h"
#include "k_queue.h"
#include "printf.h"
#include "string.h"


/* global variables */
U32 g_wall_clock_start_time = 0;
U32 g_wall_clock_start_time_offset = 0;
U32 g_wall_clock_running = 0;


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
                sprintf(p_display_msg->m_data, "%c[s%c[%dA%c[%dC%c[%dD%c[36;1m%02d%c[37m:%c[36;1m%02d%c[37m:%c[36;1m%02d%c[u%c[0m", 0x1B, 0x1B, 2000, 0x1B, 2000, 0x1B, 7, 0x1B, h, 0x1B, 0x1B, m, 0x1B, 0x1B, s, 0x1B, 0x1B);
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
    U8* p_arithmetic = NULL;
    
    queue_t hibernate_queue;
    hibernate_queue.mp_first = NULL;
    hibernate_queue.mp_last = NULL;
    
    while (1) {
        if (is_queue_empty(&hibernate_queue)) {
            p_msg = receive_message(NULL);
        } else {
            p_arithmetic = (U8 *)dequeue_node(&hibernate_queue);
            p_arithmetic += MSG_HEADER_OFFSET;
            p_msg = (msg_t *)p_arithmetic;
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
                        p_arithmetic = (U8 *)p_msg;
                        p_arithmetic -= MSG_HEADER_OFFSET;
                        enqueue_node(&hibernate_queue, (node_t *)p_arithmetic);
                    }
                }
            }
        }
        
        release_memory_block(p_msg);
        
        release_processor();
    }
}
