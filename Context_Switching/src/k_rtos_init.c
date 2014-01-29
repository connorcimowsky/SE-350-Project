#include "k_rtos_init.h"
#include "uart_polling.h"
#include "k_memory.h"
#include "k_process.h"


void k_rtos_init(void)
{
    __disable_irq();
    
    uart0_init();
    
    memory_init();
    
    process_init();
    
    __enable_irq();
    
    /* start the first process */
    k_release_processor();
}
