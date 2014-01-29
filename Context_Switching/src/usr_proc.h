#ifndef USR_PROC_H
#define USR_PROC_H

#include "rtx.h"


/* external functions */
extern PROC_INIT g_test_procs[NUM_TEST_PROCS];  /* process initialization table */


/* populate the table of test processes */
void set_test_procs(void);

/* indefinitely releases the processor */
void null_process(void);

/* prints five uppercase letters and then releases the processor */
void proc1(void);

/* prints five numbers and then releases the processor */
void proc2(void);

/* indefinitely requests memory blocks */
void proc3(void);


#endif /* USR_PROC_H */
