#include <LPC17xx.h>
#include <system_LPC17xx.h>

#include "rtx.h"

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
    
	/* initialize the RTX */
	rtx_init();  
    
	/* we should never reach this */
	return RTX_ERR;  
}
