#include <LPC17xx.h>
#include "k_proc.h"
#include "k_rtos.h"
#include "k_process.h"

#ifdef DEBUG_0
#include "printf.h"
#endif


/* global variables */
volatile uint32_t g_timer_count = 0;
k_queue_t g_timeout_queue;


void null_process(void)
{   
    while (1) {
        
        int ret_val = release_processor();
        
#ifdef DEBUG_0
        printf("null_process: ret_val = %d\n", ret_val);
#endif
        
    }
}

__asm void TIMER0_IRQHandler(void)
{
    PRESERVE8
    IMPORT timer_i_process
    PUSH {r4-r11, lr}
    BL timer_i_process
    POP {r4-r11, pc}
}

void timer_i_process(void)
{   
    msg_t *p_msg = (msg_t *)k_non_blocking_receive_message();
    if (p_msg != NULL) {
        U8 *p_decrement = (U8 *)p_msg;
        p_decrement -= MSG_HEADER_OFFSET;
        
        queue_sorted_insert(&g_timeout_queue, (k_node_t *)p_decrement);
    }
    
    while (!is_queue_empty(&g_timeout_queue) && queue_peek(&g_timeout_queue)->m_val <= g_timer_count) {
        k_msg_t *p_next_message = (k_msg_t *)dequeue_node(&g_timeout_queue);
        k_pcb_t *p_recipient_pcb = gp_pcbs[p_next_message->m_recipient_pid];
    
        if (enqueue_node(&(p_recipient_pcb->m_msg_queue), (k_node_t *)p_next_message) == RTOS_ERR) {
#ifdef DEBUG_1
            printf("%s: Could not send message to process %d\n\r", __FUNCTION__, p_recipient_pcb->m_pid);
#endif
        }
    }
    
    LPC_TIM0->IR = (1 << 0);
    
    g_timer_count++;
}
