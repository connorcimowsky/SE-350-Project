#ifndef RTX_H
#define RTX_H


/* definitions */

#define RTX_ERR -1
#define RTX_OK  0

#define NULL 0
#define NUM_TEST_PROCS 2


/* integer types */
typedef unsigned char U8;
typedef unsigned int U32;


/* process priority */
typedef enum {
    HIGHEST = 0,
    HIGH,
    MEDIUM,
    LOW,
    LOWEST,
    NUM_PRIORITIES
} PRIORITY_E;


/* process initialization table item */
typedef struct proc_init
{	
    /* process id */
	int m_pid;

    /* priority */
	PRIORITY_E m_priority;
    
    /* size of stack in words */
	int m_stack_size;
    
    /* entry point of the process */
	void (*mpf_start_pc) ();
} PROC_INIT;


/* user-facing api */

#define __SVC_0  __svc_indirect(0)

extern void k_rtx_init(void);
#define rtx_init() _rtx_init((U32)k_rtx_init)
extern void __SVC_0 _rtx_init(U32 p_func);

extern int k_release_processor(void);
#define release_processor() _release_processor((U32)k_release_processor)
extern int __SVC_0 _release_processor(U32 p_func);

extern void *k_request_memory_block(void);
#define request_memory_block() _request_memory_block((U32)k_request_memory_block)
extern void *_request_memory_block(U32 p_func) __SVC_0;

extern int k_release_memory_block(void *);
#define release_memory_block(p_mem_blk) _release_memory_block((U32)k_release_memory_block, p_mem_blk)
extern int _release_memory_block(U32 p_func, void *p_mem_blk) __SVC_0;


#endif /* RTX_H */
