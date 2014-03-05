#include <LPC17xx.h>
#include "uart_irq.h"
#include "k_proc.h"
#include "k_rtos.h"
#include "k_process.h"

#ifdef DEBUG_0
#include "printf.h"
#endif


/* global variables */
volatile uint32_t g_timer_count = 0;
k_queue_t g_timeout_queue;
U32 g_flag = 0;

uint8_t g_buffer[]= "You Typed a Q\n\r";
uint8_t *gp_buffer = g_buffer;
uint8_t g_send_char = 0;
uint8_t g_char_in;
uint8_t g_char_out;


void null_process(void)
{   
    while (1) {
        
        int ret_val = release_processor();
        
#ifdef DEBUG_0
        printf("null_process: ret_val = %d\n", ret_val);
#endif
        
    }
}

__asm void UART0_IRQHandler(void)
{
    PRESERVE8
    IMPORT uart_i_process
    PUSH {r4-r11, lr}
    BL uart_i_process
    POP {r4-r11, pc}
}

void uart_i_process(void)
{
    uint8_t IIR_IntId;
	LPC_UART_TypeDef *pUart = (LPC_UART_TypeDef *)LPC_UART0;
    
    /* reading automatically acknowledges the interrupt; skip pending bit */
    IIR_IntId = ((pUart->IIR) >> 1);
    
    if (IIR_IntId & IIR_RDA) {
        g_char_in = pUart->RBR;
        
#ifdef DEBUG_0
        printf("UART i-process: read %c\n\r", g_char_in);
#endif
        
        g_buffer[12] = g_char_in;
        g_send_char = 1;
    } else if (IIR_IntId & IIR_THRE) {
        if (*gp_buffer != '\0' ) {
            g_char_out = *gp_buffer;
            
#ifdef DEBUG_0
            printf("UART i-process: writing %c\n\r", g_char_out);
#endif
            
            pUart->THR = g_char_out;
            gp_buffer++;
        } else {
            pUart->IER ^= IER_THRE;
            pUart->THR = '\0';
            g_send_char = 0;
            gp_buffer = g_buffer;
        }
    }
}

__asm void TIMER0_IRQHandler(void)
{
    PRESERVE8
    IMPORT timer_i_process
    IMPORT k_release_processor
    PUSH {R4-R11, LR}
    BL timer_i_process
    LDR R4, =__cpp(&g_flag);
    LDR R4, [R4]
    MOV R5, #0
    CMP R4, R5
    BEQ RESTORE
    BL k_release_processor
RESTORE
    POP {R4-R11, PC}
}

void timer_i_process(void)
{
    msg_t *p_msg = NULL;
    
    __disable_irq();
    
    LPC_TIM0->IR = (1 << 0);
    
    p_msg = (msg_t *)k_non_blocking_receive_message(PID_TIMER_IPROC);
    if (p_msg != NULL) {
        U8 *p_decrement = (U8 *)p_msg;
        p_decrement -= MSG_HEADER_OFFSET;
        
        queue_sorted_insert(&g_timeout_queue, (k_node_t *)p_decrement);
    }
    
    g_flag = 0;
    
    while (!is_queue_empty(&g_timeout_queue) && queue_peek(&g_timeout_queue)->m_val <= g_timer_count) {
        k_msg_t *p_next_message = (k_msg_t *)dequeue_node(&g_timeout_queue);
        k_pcb_t *p_recipient_pcb = gp_pcbs[p_next_message->m_recipient_pid];
    
        if (enqueue_node(&(p_recipient_pcb->m_msg_queue), (k_node_t *)p_next_message) == RTOS_OK) {
            if (p_recipient_pcb->m_priority <= gp_current_process->m_priority) {
                g_flag = 1;
            }
        } else {
#ifdef DEBUG_1
            printf("%s: Could not send message to process %d\n\r", __FUNCTION__, p_recipient_pcb->m_pid);
#endif
        }
    }
    
    g_timer_count++;
    
    __enable_irq();
}

void kcd_proc(void)
{
    msg_t *p_msg = (msg_t *)receive_message(NULL);
    
    if (p_msg->mp_data[0] == '%' && p_msg->mp_data[1] == 'W' && p_msg->mp_data[2] == 'S') {
        // wall clock
    }
    
    return;
}

void crt_proc(void)
{
    msg_t *p_msg = (msg_t *)receive_message(NULL);
    
    send_message(PID_UART_IPROC, p_msg);

    return;
}
