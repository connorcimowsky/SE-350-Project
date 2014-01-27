/** 
 * @file:   k_rtx.h
 * @brief:  kernel deinitiation and data structure header file
 * @auther: Yiqing Huang
 * @date:   2014/01/17
 */
 
#ifndef K_RTX_H
#define K_RTX_H

#include "rtx.h"
#include "k_list.h"
#include "k_queue.h"

/* Definitions */

#define RAM_END_ADDR 0x10008000
#define NUM_BLOCKS 16
#define BLOCK_SIZE 128

#define INITIAL_xPSR 0x01000000        /* user process initial xPSR value */

#ifdef DEBUG_0
#define USR_SZ_STACK 0x200         /* user proc stack size 512B   */
#else
#define USR_SZ_STACK 0x100         /* user proc stack size 218B  */
#endif

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

/* Global Variables */

extern unsigned int Image$$RW_IRAM1$$ZI$$Limit;     /* end address of the memory image */
extern k_list_t *gp_heap;                           /* current bottom of (incrementing) heap */
extern U32 *gp_stack;                               /* current top of (decrementing) stack, 8-byte aligned */

extern PROC_INIT g_proc_table[NUM_TEST_PROCS];      /* process initialization table */
extern k_pcb_node_t **gp_pcb_nodes;                 /* array of nodes pointing to PCBs */
extern k_pcb_node_t *gp_current_process;            /* the process whose state is EXECUTING */
extern k_queue_t *gp_ready_queue[NUM_PRIORITIES];   /* array of priority queues, one for each priority */
extern k_queue_t *gp_blocked_queue;                 /* queue for processes that are BLOCKED_ON_RESOURCE */

#endif /* K_RTX_H */
