#include "k_usr_proc.h"
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
