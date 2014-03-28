#ifndef K_SYSTEM_PROC_H
#define K_SYSTEM_PROC_H

#include "queue.h"


/* external variables */

/* the total number of timer ticks, measured in milliseconds */
extern volatile U32 g_timer_count;

/* the queue containing delayed messages that have not yet expired */
extern queue_t g_timeout_queue;


/* indefinitely releases the processor */
void null_process(void);

/* uart i-process */
void uart_i_process(void);

/* timing service i-process */
void timer_i_process(void);

/* KCD process */
void kcd_proc(void);

/* CRT process */
void crt_proc(void);


#endif /* K_SYSTEM_PROC_H */
