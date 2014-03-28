#include "k_rtos_init.h"
#include "uart_polling.h"
#include "uart_irq.h"
#include "timer.h"
#include "k_memory.h"
#include "k_process.h"

#ifdef DEBUG_LED
#include "led.h"
#endif


void k_rtos_init(void)
{
    /* disable interrupt requests */
    __disable_irq();
    
#ifdef DEBUG_LED
    led_init();
    led_all_on();
#endif
    
    timer_init(0);
    
    uart_irq_init(0);
    uart_init(1);
    
    k_memory_init();
    
    k_process_init();
    
    /* enable interrupt requests */
    __enable_irq();
    
    /* start the first process */
    k_release_processor();
}
