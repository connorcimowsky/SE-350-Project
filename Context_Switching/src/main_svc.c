#include <LPC17xx.h>
#include <system_LPC17xx.h>

#include "rtos.h"

#ifdef DEBUG_0
#include "uart_polling.h"
#include "printf.h"
#endif


int main(void) 
{
    /* CMSIS system initialization */
    SystemInit();
    
#ifdef DEBUG_0
    init_printf(NULL, putc);
#endif
    
    /* initialize the RTOS */
    rtos_init();  
    
    /* we should never reach this */
    return RTOS_ERR;  
}
