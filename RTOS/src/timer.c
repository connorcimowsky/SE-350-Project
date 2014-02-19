#include <LPC17xx.h>
#include "timer.h"


uint32_t timer_init(uint8_t n_timer)
{
    LPC_TIM_TypeDef *pTimer;
    
    if (n_timer == 0) {
        pTimer = (LPC_TIM_TypeDef *)LPC_TIM0;
    } else {
        return 1;
    }
    
    pTimer->PR = 12499;
    pTimer->MR0 = 1;
    pTimer->MCR = (1 << 0) | (1 << 1);
    
    NVIC_EnableIRQ(TIMER0_IRQn);
    
    pTimer->TCR = 1;
    
    return 0;
}
