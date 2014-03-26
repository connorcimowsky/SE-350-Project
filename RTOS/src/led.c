#include "led.h"

#define NUM_LEDS 8


static const int g_led_bits[NUM_LEDS] = {28, 29, 31, 2, 3, 4, 5, 6};


void led_init(void)
{
    LPC_GPIO1->FIODIR |= 0xB0000000;
    LPC_GPIO2->FIODIR |= 0x0000007C;
}

void led_all_on(void)
{
    int i;
    
    for (i = 0; i < NUM_LEDS; i++) {
        led_on(i);
    }
}


void led_all_off(void)
{
    int i;
    
    for (i = 0; i < NUM_LEDS; i++) {
        led_off(i);
    }
}

void led_on(int led_num)
{
    int led_mask = 1 << g_led_bits[led_num];
    
    if (led_num < 3) {
        LPC_GPIO1->FIOSET = led_mask;
    } else {
        LPC_GPIO2->FIOSET = led_mask;
    }
}

void led_off(int led_num)
{
    int led_mask = 1 << g_led_bits[led_num];
    
    if (led_num < 3) {
        LPC_GPIO1->FIOCLR = led_mask;
    } else {
        LPC_GPIO2->FIOCLR = led_mask;
    }
}
