#include <LPC17xx.h>
#include "uart_polling.h"

int main()
{
    SystemInit();
    uart0_init();
    
    return 0;
}
