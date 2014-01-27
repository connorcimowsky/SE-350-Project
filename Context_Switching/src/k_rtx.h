/** 
 * @file:   k_rtx.h
 * @brief:  kernel deinitiation and data structure header file
 * @auther: Yiqing Huang
 * @date:   2014/01/17
 */
 
#ifndef K_RTX_H_
#define K_RTX_H_

#include "rtx.h"

#ifdef DEBUG_0
#define USR_SZ_STACK 0x200         /* user proc stack size 512B   */
#else
#define USR_SZ_STACK 0x100         /* user proc stack size 218B  */
#endif /* DEBUG_0 */

/*----- Types -----*/

/* Process States */
typedef enum {
    NEW = 0,
    READY,
    BLOCKED_ON_RESOURCE,
    WAITING_FOR_MESSAGE,
    EXECUTING,
    INTERRUPTED
} PROC_STATE_E;

/*
  PCB data structure definition.
  You may want to add your own member variables
  in order to finish P1 and the entire project 
*/
typedef struct pcb 
{ 
	//struct pcb *mp_next;  /* next pcb, not used in this example */  
	U32 *mp_sp;		/* stack pointer of the process */
	U32 m_pid;		/* process id */
    PRIORITY_E m_priority; /* process priority */
	PROC_STATE_E m_state;   /* state of the process */      
} PCB;

/* PCB-containing node */
typedef struct k_pcb_node_t {
    struct k_pcb_node_t *next;
    PCB *pcb;
} k_pcb_node_t;

#endif // ! K_RTX_H_
