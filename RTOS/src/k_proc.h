#ifndef K_PROC_H
#define K_PROC_H

#define NULL_PROC_PID 0
#define TIMER_I_PROC_PID 7

#include "k_queue.h"


/* external variables */
extern k_queue_t g_timeout_queue;


/* indefinitely releases the processor */
void null_process(void);

/* timing service i-process */
void timer_i_process(void);


#endif /* K_PROC_H */
