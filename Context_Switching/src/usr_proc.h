/**
 * @file:   usr_proc.h
 * @brief:  Two user processes header file
 * @author: Yiqing Huang
 * @date:   2014/01/17
 */
 
#ifndef USR_PROC_H
#define USR_PROC_H

extern PROC_INIT g_test_procs[NUM_TEST_PROCS];  /* process initialization table */

void set_test_procs(void);
void proc1(void);
void proc2(void);

#endif /* USR_PROC_H */
