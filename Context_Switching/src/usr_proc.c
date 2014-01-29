#include "usr_proc.h"
#include "uart_polling.h"

#ifdef DEBUG_0
#include "printf.h"
#endif


/* global variables */
PROC_INIT g_test_procs[NUM_TEST_PROCS];


void set_test_procs(void)
{
    int i;
    for( i = 0; i < NUM_TEST_PROCS; i++ ) {
        g_test_procs[i].m_pid = (U32)(i + 1);
    }
    
    g_test_procs[0].m_priority = LOWEST;
    g_test_procs[0].m_stack_size = 0x100;
    g_test_procs[0].mpf_start_pc = &null_process;
    
    g_test_procs[1].m_priority = LOW;
    g_test_procs[1].m_stack_size = 0x100;
    g_test_procs[1].mpf_start_pc = &proc1;
    
    g_test_procs[1].m_priority = MEDIUM;
    g_test_procs[2].m_stack_size = 0x100;
    g_test_procs[2].mpf_start_pc = &proc2;
    
    g_test_procs[1].m_priority = HIGHEST;
    g_test_procs[3].m_stack_size = 0x100;
    g_test_procs[3].mpf_start_pc = &proc3;
}

void null_process(void)
{
    int ret_val;
    
    while (1) {
        
        ret_val = release_processor();
        
#ifdef DEBUG_0
        printf("null_process: ret_val = %d\n", ret_val);
#endif
        
    }
}

void proc1(void)
{
    int i = 0;
    int ret_val = 10;
    while ( 1) {
        if ( i != 0 && i%5 == 0 ) {
            uart0_put_string("\n\r");
            ret_val = release_processor();
#ifdef DEBUG_0
            printf("proc1: ret_val=%d\n", ret_val);
#endif /* DEBUG_0 */
        }
        uart0_put_char('A' + i%26);
        i++;
    }
}

void proc2(void)
{
    int i = 0;
    int ret_val = 20;
    while ( 1) {
        if ( i != 0 && i%5 == 0 ) {
            uart0_put_string("\n\r");
            ret_val = release_processor();
#ifdef DEBUG_0
            printf("proc2: ret_val=%d\n", ret_val);
#endif /* DEBUG_0 */
        }
        uart0_put_char('0' + i%10);
        i++;
    }
}

void proc3(void)
{
    while(1) {
        request_memory_block();
    }
}
