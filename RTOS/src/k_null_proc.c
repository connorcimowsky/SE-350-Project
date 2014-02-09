#include "k_null_proc.h"

#ifdef DEBUG_0
#include "printf.h"
#endif


void null_process(void)
{   
    while (1) {
        
        int ret_val = release_processor();
        
#ifdef DEBUG_0
        printf("null_process: ret_val = %d\n", ret_val);
#endif
        
    }
}
