#include "usr_proc.h"
#include "uart_polling.h"

#ifdef DEBUG_0
#include "printf.h"
#endif


/* global variables */

PROC_INIT g_test_procs[NUM_TEST_PROCS];

/* pointers to memory blocks returned by request_memory_block() */
void *gp_mem_blks[16];

/* the index of the most recently requested memory block */
int g_mem_blk_index = 0;

/* the pid of the most recently executed process */
int g_previous_pid = -1;

/* an array of flags indicating the result of each test case */
int g_success_flags[6] = {1, 1, 1, 1, 1, 1};


void set_test_procs(void)
{
    int i;
    for(i = 0; i < NUM_TEST_PROCS; i++) {
        g_test_procs[i].m_pid = (U32)i;
        g_test_procs[i].m_stack_size = USR_SZ_STACK;
    }
    
    g_test_procs[NULL_PROC_PID].m_priority = LOWEST;
    g_test_procs[NULL_PROC_PID].mpf_start_pc = &null_process;
    
    g_test_procs[1].m_priority = HIGHEST;
    g_test_procs[1].mpf_start_pc = &proc1;
    
    g_test_procs[2].m_priority = MEDIUM;
    g_test_procs[2].mpf_start_pc = &proc2;
    
    g_test_procs[3].m_priority = LOW;
    g_test_procs[3].mpf_start_pc = &proc3;
    
    g_test_procs[4].m_priority = LOW;
    g_test_procs[4].mpf_start_pc = &proc4;
    
    g_test_procs[5].m_priority = LOW;
    g_test_procs[5].mpf_start_pc = &proc5;
    
    g_test_procs[6].m_priority = LOW;
    g_test_procs[6].mpf_start_pc = &proc6;
}

void null_process(void)
{   
    while (1) {
        
        int ret_val = release_processor();
        
#ifdef DEBUG_0
        printf("null_process: ret_val = %d\n", ret_val);
#endif
        
    }
}

void proc1(void)
{
    
#ifdef DEBUG_0
    printf("G019_test: START\n");
    printf("G019_test: total 6 tests\n");
#endif
    
    /**
     * TEST CASE 1: this test exhausts the available memory in the heap
     *
     * TEST CASE 1a: request every available block from the heap
     * PRE: entire heap is available
     * POST: no available memory blocks; proc1 is in the blocked queue; preempt to proc2
     */
    
    g_previous_pid = 1;
    
    // request memory blocks until we are preempted
    // when returning to proc1, g_previous_pid will no longer be 1
    while (g_previous_pid == 1) {
        gp_mem_blks[g_mem_blk_index++] = request_memory_block();
    }
    
    /**
     * TEST CASE 2: this test ensures no preemption to lower-priority processes when releasing memory
     *
     * TEST CASE 2a: set the priority of this process to LOW
     * PRE: previously executing in proc2
     * POST: the priority of this process is LOW; preempt to proc2
     */
    
    g_success_flags[0] &= (g_previous_pid == 2);
    g_previous_pid = 1;
    
#ifdef DEBUG_0
    printf("G019_test: test 1 ");
    printf(g_success_flags[0] == 1 ? "OK\n" : "FAIL\n");
#endif
    
    set_process_priority(1, LOW);
    
    
    while (1) {
        // at the very end, preempt to the null process through memory starvation
        gp_mem_blks[g_mem_blk_index++] = request_memory_block();
    }
}

void proc2(void)
{
    /**
     * TEST CASE 1b: release a single memory block
     * PRE: previously executing in proc1; no available memory in the heap
     * POST: 1 memory block available; proc1 is no longer blocked; preempt to proc1
     *
     * END TEST CASE 1
     */
    
    g_success_flags[0] &= (g_previous_pid == 1);
    g_previous_pid = 2;
    
    release_memory_block(gp_mem_blks[--g_mem_blk_index]);
    
    /**
     * TEST CASE 2b: release a single memory block
     * PRE: previously executing in proc1
     * POST: continue executing without preemption
     *
     * END TEST CASE 2
     */
    
    g_success_flags[1] &= (g_previous_pid == 1);
    g_previous_pid = 2;
    
    release_memory_block(gp_mem_blks[--g_mem_blk_index]);
    
    /**
     * TEST CASE 3: this test ensures proper context switching when setting process priorities and releasing the processor
     *
     * TEST CASE 3a: release all remaining memory; attempt to release an invalid memory block
     * PRE: previously executing in the current process
     * POST: all memory blocks in the heap are available
     */
    
    g_success_flags[1] &= (g_previous_pid == 2);
    g_previous_pid = 2;
    
#ifdef DEBUG_0
    printf("G019_test: test 2 ");
    printf(g_success_flags[1] == 1 ? "OK\n" : "FAIL\n");
#endif
    
    while (g_mem_blk_index > 0) {
        // expect successful release of memory block
        g_success_flags[2] &= (release_memory_block(gp_mem_blks[--g_mem_blk_index]) == RTOS_OK);
    }
    
    // expect RTOS_ERR on attempted release of invalid memory block
    g_success_flags[2] &= (release_memory_block(gp_mem_blks[g_mem_blk_index - 1]) == RTOS_ERR);
    
    /**
     * TEST CASE 3b: set the priority of proc3 to HIGH
     * PRE: previously executing in the current process; proc3 has a priority of LOW
     * POST: the priority of proc3 is HIGH; preempt to proc3
     */
    
    g_success_flags[2] &= (g_previous_pid == 2);
    g_previous_pid = 2;
    
    set_process_priority(3, HIGH);
    
    /**
     * TEST CASE 6d: set the priority of proc6 to HIGHEST
     * PRE: previously executing in proc4; priority of proc6 is LOW
     * POST: priority of proc6 is HIGHEST; preempt to proc6
     */
    
    g_success_flags[5] &= (g_previous_pid == 4);
    g_previous_pid = 2;
    
    set_process_priority(6, HIGHEST);
    
    
    while (1) {
        // at the very end, preempt to the null process through memory starvation
        gp_mem_blks[g_mem_blk_index++] = request_memory_block();
    }
}

void proc3(void)
{
    /**
     * TEST CASE 3c: set the priority of proc4 to HIGH
     * PRE: previously executing in proc2; the priority of proc4 is LOW
     * POST: the priority of proc4 is HIGH; preempt to proc4
     */
    
    g_success_flags[2] &= (g_previous_pid == 2);
    g_previous_pid = 3;
    
    set_process_priority(4, HIGH);
    
    
    /**
     * TEST CASE 4: this test ensures that processes are blocked when memory is unavailable
     *
     * TEST CASE 4a: request every available block from the heap
     * PRE: previously executing in proc4; entire heap is available
     * POST: no available memory blocks; proc3 is in the blocked queue; preempt to proc4
     */
    
    g_success_flags[2] &= (g_previous_pid == 4);
    g_previous_pid = 3;
    
#ifdef DEBUG_0
    printf("G019_test: test 3 ");
    printf(g_success_flags[2] == 1 ? "OK\n" : "FAIL\n");
#endif
    
    // request memory blocks until we are preempted
    // when returning to proc3, g_previous_pid will no longer be 3
    while (g_previous_pid == 3) {
        gp_mem_blks[g_mem_blk_index++] = request_memory_block();
    }
    
    /**
     * TEST CASE 5c: release a single memory block
     * PRE: previously executing in proc5; proc4 is blocked
     * POST: proc4 is no longer blocked
     *
     * END TEST CASE 5
     */
    
    g_success_flags[4] &= (g_previous_pid == 5);
    g_previous_pid = 3;
    
    // this will not result in preemption; the priority of proc4 is not higher than the priority of proc3
    release_memory_block(gp_mem_blks[--g_mem_blk_index]);
    
    /**
     * TEST CASE 6: this test ensures correct scheduling after unblocking processes
     *
     * TEST CASE 6a: release the processor
     * PRE: previously executing in proc3
     * POST: executing in proc5
     */
    
    g_success_flags[4] &= (g_previous_pid == 3);
    g_previous_pid = 3;
    
#ifdef DEBUG_0
    printf("G019_test: test 5 ");
    printf(g_success_flags[4] == 1 ? "OK\n" : "FAIL\n");
#endif
    
    release_processor();
    
    
    while (1) {
        // at the very end, preempt to the null process through memory starvation
        gp_mem_blks[g_mem_blk_index++] = request_memory_block();
    }
}

void proc4(void)
{
    /**
     * TEST CASE 3d: release the processor
     * PRE: previously executing in proc3
     * POST: executing in proc3
     *
     * END TEST CASE 3
     */
    
    g_success_flags[2] &= (g_previous_pid == 3);
    g_previous_pid = 4;
    
    release_processor();
    
    
    /**
     * TEST CASE 4b: set the priority of proc5 to HIGH
     * PRE: previously executing in proc3; proc5 has a priority of LOW
     * POST: priority of proc5 is HIGH; preempt to proc5
     */
    
    g_success_flags[3] &= (g_previous_pid == 3);
    g_previous_pid = 4;
    
    set_process_priority(5, HIGH);
    
    /**
     * TEST CASE 4d: request a single memory block
     * PRE: previously executing in proc5; no memory blocks available in the heap
     * POST: no available memory blocks; proc4 is in the blocked queue; preempt to proc5
     *
     * END TEST CASE 4
     */
    
    g_success_flags[3] &= (g_previous_pid == 5);
    g_previous_pid = 4;
    
    gp_mem_blks[g_mem_blk_index++] = request_memory_block();
    
    /**
     * TEST CASE 6c: set the priority of proc2 to HIGHEST
     * PRE: previously executing in proc5; priority of proc2 is MEDIUM
     * POST: priority of proc2 is HIGHEST; preempt to proc2
     */
    
    g_success_flags[5] &= (g_previous_pid == 5);
    g_previous_pid = 4;
    
    set_process_priority(2, HIGHEST);
    
    
    while (1) {
        // at the very end, preempt to the null process through memory starvation
        gp_mem_blks[g_mem_blk_index++] = request_memory_block();
    }
}

void proc5(void)
{
    /**
     * TEST CASE 4c: release the processor
     * PRE: previously executing in proc4
     * POST: executing in proc4
     */
    
    g_success_flags[3] &= (g_previous_pid == 4);
    g_previous_pid = 5;
    
    release_processor();
    
    /**
     * TEST CASE 5: this test ensures the correct behaviour when memory is released while processes are blocked
     *
     * TEST CASE 5a: release a single memory block
     * PRE: previously executing in proc4; no available memory blocks; proc3 and proc4 are blocked
     * POST: one available memory block; proc3 no longer blocked
     */
    
    g_success_flags[3] &= (g_previous_pid == 4);
    g_previous_pid = 5;
    
#ifdef DEBUG_0
    printf("G019_test: test 4 ");
    printf(g_success_flags[3] == 1 ? "OK\n" : "FAIL\n");
#endif
    
    // this will not result in preemption; the priority of proc3 is not higher than the priority of proc5
    release_memory_block(gp_mem_blks[--g_mem_blk_index]);
    
    /**
     * TEST CASE 5b: release the processor
     * PRE: previously executing in proc5; proc3 is ready to execute
     * POST: executing in proc3
     */
    
    g_success_flags[4] &= (g_previous_pid == 5);
    g_previous_pid = 5;
    
    release_processor();
    
    /**
     * TEST CASE 6b: release the processor
     * PRE: previously executing in proc3; proc4 is ready to execute
     * POST: executing in proc4
     */
    
    g_success_flags[5] &= (g_previous_pid == 3);
    g_previous_pid = 5;
    
    release_processor();
    
    
    while (1) {
        // at the very end, preempt to the null process through memory starvation
        gp_mem_blks[g_mem_blk_index++] = request_memory_block();
    }
}

void proc6(void)
{
    int i;
    int num_successes = 0;
    
    /* update the success flag for TEST CASE 6 */
    g_success_flags[5] &= (g_previous_pid == 2);
    g_previous_pid = 6;
    
#ifdef DEBUG_0
    printf("G019_test: test 6 ");
    printf(g_success_flags[5] == 1 ? "OK\n" : "FAIL\n");
#endif
    
    /* END TEST CASE 6 */
    
    for (i = 0; i < 6; i++) {
        num_successes += g_success_flags[i];
    }
    
#ifdef DEBUG_0
    printf("G019_test: %d/%d tests OK\n", num_successes, 6);
    printf("G019_test: %d/%d tests FAIL\n", (6 - num_successes), 6);
    
    printf("G019_test: END\n");
#endif
    
    
    while (1) {
        // at the very end, preempt to the null process through memory starvation
        gp_mem_blks[g_mem_blk_index++] = request_memory_block();
    }
}
