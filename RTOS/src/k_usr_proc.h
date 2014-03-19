#ifndef K_USR_PROC_H
#define K_USR_PROC_H

#include "rtos.h"


/* wall clock process */
void wall_clock_proc(void);

/* set priority process */
void set_priority_proc(void);

/* stress test A */
void stress_test_a(void);

/* stress test B */
void stress_test_b(void);

/* stress test C */
void stress_test_c(void);


#endif /* K_USR_PROC_H */
