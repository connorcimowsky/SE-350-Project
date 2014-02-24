#include <LPC17xx.h>
#include "k_proc.h"
#include "k_rtos.h"

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
    LPC_TIM0->IR = (1 << 0);
    
    g_timer_count++;
}
