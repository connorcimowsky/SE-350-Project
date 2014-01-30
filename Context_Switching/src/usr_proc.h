#ifndef USR_PROC_H
#define USR_PROC_H

#include "rtos.h"


/* external functions */
extern PROC_INIT g_test_procs[NUM_TEST_PROCS];  /* process initialization table */


/* populate the table of test processes */
void set_test_procs(void);

/* indefinitely releases the processor */
void null_process(void);

/* test processes */
void proc1(void);
void proc2(void);
void proc3(void);
void proc4(void);
void proc5(void);
void proc6(void);


#endif /* USR_PROC_H */
