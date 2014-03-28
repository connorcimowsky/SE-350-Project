#include <LPC17xx.h>
#include "uart_irq.h"
#include "string.h"
#include "k_system_proc.h"
#include "k_rtos.h"
#include "k_memory.h"
#include "k_process.h"

#ifdef DEBUG_0
#include "printf.h"
#endif

#ifdef DEBUG_HOTKEYS

#include "printf.h"

#define DEBUG_HOTKEY_1 '!'
#define DEBUG_HOTKEY_2 '@'
#define DEBUG_HOTKEY_3 '#'
#define DEBUG_HOTKEY_4 '$'
#define DEBUG_HOTKEY_5 '^'

#endif /* DEBUG_HOTKEYS */


/* global variables */

/* the number of times time timer has ticked, measured in milliseconds */
volatile U32 g_timer_count = 0;

/* the queue containing messages which are scheduled for later dispatching */
queue_t g_timeout_queue;

/* used by TIMER0_IRQHandler to determine whether or not we should yield the processor */
U32 g_timer_preemption_flag = 0;

/* used by UART0_IRQHandler to determine whether or not we should yield the processor */
U32 g_uart_preemption_flag = 0;

msg_t *gp_cur_msg = NULL;
uint8_t g_char_in;
list_t g_kcd_reg;
char g_input_buffer[INPUT_BUFFER_SIZE];
int g_input_buffer_index = 0;
int g_output_buffer_index = 0;


void null_process(void)
{   
    while (1) {
        
        int ret_val = release_processor();
        
#ifdef DEBUG_0
        printf("null_process: ret_val = %d\n\r", ret_val);
#endif
        
    }
}

__asm void UART0_IRQHandler(void)
{
    PRESERVE8
    IMPORT uart_i_process
    IMPORT k_release_processor
    PUSH {R4-R11, LR}
    BL uart_i_process
    LDR R4, =__cpp(&g_uart_preemption_flag);
    LDR R4, [R4]
    MOV R5, #0
    CMP R4, R5
    BEQ UART_RESTORE
    BL k_release_processor
UART_RESTORE
    POP {r4-r11, pc}
}

void uart_i_process(void)
{
    uint8_t IIR_IntId;
    LPC_UART_TypeDef *pUart = (LPC_UART_TypeDef *)LPC_UART0;
    
    __disable_irq();
    
    g_uart_preemption_flag = 0;
    
    /* reading IIR automatically acknowledges the interrupt; skip pending bit */
    IIR_IntId = ((pUart->IIR) >> 1);
    
    if (IIR_IntId & IIR_RDA) {
        /* a character has been entered */
        
        /* read the character from the receiver buffer register and acknowledge the interrupt */
        g_char_in = pUart->RBR;
        
#ifdef DEBUG_1
        printf("UART i-process: read %c\n\r", g_char_in);
#endif
        
        /* echo the entered character to the CRT process; only request memory if we will not block */
        if (!is_list_empty(gp_heap)) {
            msg_t *p_msg = (msg_t *)k_request_memory_block();
            p_msg->m_type = MSG_TYPE_CRT_DISP;
            
            if (g_char_in != '\r') {
                p_msg->m_data[0] = g_char_in;
                p_msg->m_data[1] = '\0';
            } else {
                p_msg->m_data[0] = '\n';
                p_msg->m_data[1] = g_char_in;
                p_msg->m_data[2] = '\0';
            }
            
            k_send_message_helper(PID_UART_IPROC, PID_CRT, p_msg);
            
            /* we should preempt to the CRT process at this point */
            g_uart_preemption_flag = 1;
        }
        
#ifdef DEBUG_HOTKEYS
        
        if (g_char_in == DEBUG_HOTKEY_1) {
            k_print_ready_queue();
        } else if (g_char_in == DEBUG_HOTKEY_2) {
            k_print_blocked_on_memory_queue();
        } else if (g_char_in == DEBUG_HOTKEY_3) {
            k_print_blocked_on_receive_queue();
        } else if (g_char_in == DEBUG_HOTKEY_4) {
            k_print_msg_logs();
        } else if (g_char_in == DEBUG_HOTKEY_5) {
            k_print_memory_heap();
        }
        
#endif
        
        if (g_char_in != '\r') {
            
#ifdef DEBUG_HOTKEYS
            
            /* only filter the debug hotkeys if DEBUG_HOTKEYS is defined */
            if ((g_char_in != DEBUG_HOTKEY_1) && (g_char_in != DEBUG_HOTKEY_2) && (g_char_in != DEBUG_HOTKEY_3) && (g_char_in != DEBUG_HOTKEY_4) && (g_char_in != DEBUG_HOTKEY_5)) {
                g_input_buffer[g_input_buffer_index++] = g_char_in;
            }
            
#else
            
            g_input_buffer[g_input_buffer_index++] = g_char_in;
            
#endif
            
        } else {
            /* copy the input buffer into a message envelope and send it to the KCD */
            
            g_input_buffer[g_input_buffer_index++] = '\0';
            
            /* only request memory if we will not block */
            if (!is_list_empty(gp_heap)) {
                msg_t *p_msg = (msg_t *)k_request_memory_block();
                p_msg->m_type = MSG_TYPE_DEFAULT;
                str_cpy(g_input_buffer, p_msg->m_data);
                
                g_input_buffer_index = 0;
                
                k_send_message_helper(PID_UART_IPROC, PID_KCD, p_msg);
                
                /* we should preempt to the KCD process at this point */
                g_uart_preemption_flag = 1;
            }
        }
        
    } else if (IIR_IntId & IIR_THRE) {
        /* a character is ready to be transmitted */
        
        if (gp_cur_msg == NULL) {
            gp_cur_msg = k_non_blocking_receive_message(PID_UART_IPROC);
        }
        
        if (gp_cur_msg->m_data[g_output_buffer_index] != '\0' ) {
            /* write the character to the transmit holding register if non-null */
        
#ifdef DEBUG_1
            printf("UART i-process: writing %c\n\r", gp_cur_msg->m_data[g_output_buffer_index]);
#endif
            
            pUart->THR = gp_cur_msg->m_data[g_output_buffer_index];
            
            g_output_buffer_index++;
            
        } else {
            /* otherwise, clear the current message being displayed and toggle the transmit holding register empty bit */
            
            k_pcb_t *p_blocked_pcb;
            
            if (is_queue_empty(&(g_pcbs[PID_UART_IPROC]->m_msg_queue))) {
                pUart->IER &= (~IER_THRE);
            }
            
            pUart->THR = '\0';
            
            k_release_memory_block_helper(gp_cur_msg);
            gp_cur_msg = NULL;
            
            p_blocked_pcb = k_dequeue_blocked_on_memory_process();
            
            /* if there is a blocked-on-memory process, set its state to READY and enqueue it in the ready queue */
            if (p_blocked_pcb != NULL) {
                p_blocked_pcb->m_state = READY;
                k_enqueue_ready_process(p_blocked_pcb);
                g_uart_preemption_flag = 1;
            }
            
            g_output_buffer_index = 0;
        }
        
    } 
    
    __enable_irq();
}

__asm void TIMER0_IRQHandler(void)
{
    PRESERVE8
    IMPORT timer_i_process
    IMPORT k_release_processor
    PUSH {R4-R11, LR}
    BL timer_i_process
    LDR R4, =__cpp(&g_timer_preemption_flag);
    LDR R4, [R4]
    MOV R5, #0
    CMP R4, R5
    BEQ TIMER_RESTORE
    BL k_release_processor
TIMER_RESTORE
    POP {R4-R11, PC}
}

void timer_i_process(void)
{
    msg_t *p_msg = NULL;
    
    __disable_irq();
    
    LPC_TIM0->IR = (1 << 0);
    
    p_msg = (msg_t *)k_non_blocking_receive_message(PID_TIMER_IPROC);
    if (p_msg != NULL) {
        /* if there is a message waiting for us (from delayed_send), add it to our timeout queue */
        sorted_enqueue((node_t *)((U8 *)p_msg - MSG_HEADER_OFFSET), &g_timeout_queue);
    }
    
    g_timer_preemption_flag = 0;
    
    while (!is_queue_empty(&g_timeout_queue) && g_timeout_queue.mp_first->m_val <= g_timer_count) {
        /* while there are expired messages in our timeout queue, place them in the appropriate message queues */
        
        k_msg_t *p_next_message = (k_msg_t *)dequeue(&g_timeout_queue);
        
        k_send_message_helper(p_next_message->m_sender_pid, p_next_message->m_recipient_pid, (msg_t *)((U8 *)p_next_message + MSG_HEADER_OFFSET));
        
        if (g_pcbs[p_next_message->m_recipient_pid]->m_priority <= gp_current_process->m_priority) {
            /* only preempt to the recipient if it is of equal or greater importance */
            g_timer_preemption_flag = 1;
        }
    }
    
#ifdef DEBUG_TIMER
    g_timer_count += 35;
#else
    g_timer_count += 1;
#endif
    
    __enable_irq();
}

void kcd_proc(void)
{
    int sender;
    while (1) {
        msg_t *p_msg = (msg_t *)receive_message(&sender);
        
        if (p_msg->m_type == MSG_TYPE_KCD_REG) {
            
            /* pick the next unused entry from the registry */
            k_kcd_reg_t *p_reg = (k_kcd_reg_t *)g_kcd_reg.mp_first;
            while (p_reg != NULL && p_reg->m_active == 1) {
                p_reg = p_reg->mp_next;
            }
            
            /* if available, populate the fields of the registry entry */
            if (p_reg != NULL && p_reg->m_active == 0) {
                str_cpy(p_msg->m_data, p_reg->m_id);
                p_reg->m_pid = sender;
                p_reg->m_active = 1;
            }
            
        } else if (p_msg->m_type == MSG_TYPE_DEFAULT) {
            
            /* we have received a keyboard command */
            
            int i = 0;
            k_kcd_reg_t *p_kcd_reg_iter = NULL;
            
            /* we will isolate the command identifier in this buffer */
            char keyboard_command_identifier[KCD_REG_LENGTH] = {'\0'};
            
            /* isolate the command identifier, i.e. the portion of the input before the first space */
            while (i < KCD_REG_LENGTH && p_msg->m_data[i] != '\0' && p_msg->m_data[i] != ' ') {
                keyboard_command_identifier[i] = p_msg->m_data[i];
                i++;
            }
            
            /* iterate through the keyboard command registry to see if the entered command has been registered */
            p_kcd_reg_iter = (k_kcd_reg_t *)g_kcd_reg.mp_first;
            while (p_kcd_reg_iter != NULL) {
                
                if (p_kcd_reg_iter->m_active == 1 && str_cmp(p_kcd_reg_iter->m_id, keyboard_command_identifier)) {
                    break;
                }
                
                p_kcd_reg_iter = p_kcd_reg_iter->mp_next;
            }
            
            /* only dispatch the command if a registry entry was found */
            if (p_kcd_reg_iter != NULL) {
                msg_t *p_msg_dispatch = (msg_t *)request_memory_block();
                p_msg_dispatch->m_type = MSG_TYPE_KCD_DISPATCH;
                str_cpy(p_msg->m_data, p_msg_dispatch->m_data);
                
                send_message(p_kcd_reg_iter->m_pid, p_msg_dispatch);
            }
            
        }
        
        release_memory_block(p_msg);
    }
}

void crt_proc(void)
{
    LPC_UART_TypeDef *pUart = (LPC_UART_TypeDef *) LPC_UART0;
    
    while (1) {
        msg_t *p_msg = (msg_t *)receive_message(NULL);
        
        if (p_msg->m_type == MSG_TYPE_CRT_DISP) {
            send_message(PID_UART_IPROC, p_msg);
            pUart->IER |= IER_THRE;
        } else {
            release_memory_block(p_msg);
        }
    }
}
