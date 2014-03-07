#ifndef USR_PROC_H
#define USR_PROC_H

#include "rtos.h"


/* external variables */
extern PROC_INIT g_test_procs[NUM_TEST_PROCS];


/* populate the table of test processes */
void set_test_procs(void);

/* test processes */
void proc1(void);
void proc2(void);
void proc3(void);
void proc4(void);
void proc5(void);
void proc6(void);

/* wall clock process */
void wall_clock_proc(void);


#endif /* USR_PROC_H */
