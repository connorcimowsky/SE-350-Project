#include "k_rtos_init.h"
#include "uart_polling.h"
#include "timer.h"
#include "k_memory.h"
#include "k_process.h"


void k_rtos_init(void)
{
    /* disable interrupt requests */
    __disable_irq();
    
    // timer_init(0);
    
    uart0_init();
    
    memory_init();
    
    process_init();
    
    /* enable interrupt requests */
    __enable_irq();
    
    /* start the first process */
    k_release_processor();
}
