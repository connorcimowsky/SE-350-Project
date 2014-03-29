#include <LPC17xx.h>

/* initialize the LEDs */
void led_init(void);

/* turn on all LEDs */
void led_all_on(void);

/* turn off all LEDs */
void led_all_off(void);

/* turn on the LED specified by led_num */
void led_on(int led_num);

/* turn off the LED specified by led_num */
void led_off(int led_num);
