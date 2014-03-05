#include <LPC17xx.h>
#include "uart_irq.h"
#include "uart_polling.h"

#ifdef DEBUG_0
#include "printf.h"
#endif


int uart_irq_init(int n_uart) {
    LPC_UART_TypeDef *pUart;
    
    if (n_uart == 0) {
        LPC_PINCON->PINSEL0 |= (1 << 4);
        LPC_PINCON->PINSEL0 |= (1 << 6);
        
        pUart = (LPC_UART_TypeDef *)LPC_UART0;
    } else if (n_uart == 1) {
        LPC_PINCON->PINSEL4 |= (2 << 0);
        LPC_PINCON->PINSEL4 |= (2 << 2);
        
        pUart = (LPC_UART_TypeDef *)LPC_UART1;
    } else {
        return 1;
    }
    
    pUart->LCR = UART_8N1;
    pUart->DLM = 0;
    pUart->DLL = 9;
    pUart->FDR = 0x21;
    pUart->FCR = 0x07;
    pUart->LCR &= ~(1 << 7);
    pUart->IER = IER_RBR | IER_RLS;
    
    if (n_uart == 0) {
        NVIC_EnableIRQ(UART0_IRQn);
    } else if (n_uart == 1) {
        NVIC_EnableIRQ(UART1_IRQn);
    } else {
        return 1;
    }
    
    pUart->THR = '\0';
    
    return 0;
}
