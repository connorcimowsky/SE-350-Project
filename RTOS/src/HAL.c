/* @brief: HAL.c Hardware Abstraction Layer
 * @author: Yiqing Huang
 * @date: 2014/01/17
 * NOTE: This file contains embedded assembly. 
 *       The code borrowed some ideas from ARM RL-RTX source code
 */
 
/* pop the exception stack frame and return to thread mode */
__asm void __rte(void)
{
  PRESERVE8            ; 8 bytes alignement of the stack
  MVN  LR, #:NOT:0xFFFFFFF9  ; set EXC_RETURN value, Thread mode, MSP
  BX   LR
}

/* NOTE: assuming MSP is used. Ideally, PSP should be used */
__asm void SVC_Handler (void) 
{
  PRESERVE8            ; 8 bytes alignement of the stack
  MRS  R0, MSP         ; Read MSP
	
  
  LDR  R1, [R0, #24]   ; Read Saved PC from SP
                       ; Loads R1 from a word 24 bytes above the address in R0
                       ; Note that R0 now contains the the SP value after the
                       ; exception stack frame is pushed onto the stack.
             
  LDRH R1, [R1, #-2]   ; Load halfword because SVC number is encoded there
  BICS R1, R1, #0xFF00 ; Extract SVC Number and save it in R1.  
                       ; R1 <= R1 & ~(0xFF00), update flags
                   
  BNE  SVC_EXIT        ; if SVC Number !=0, exit
 
  LDM  R0, {R0-R3, R12}; Read R0-R3, R12 from stack. 
                       ; NOTE R0 contains the sp before this instruction

  PUSH {R4-R11, LR}    ; Save other registers for preemption caused by i-procs
<<<<<<< HEAD

  ; This does the equivalent of gp_current_process->mp_sp = __get_MSP()
  LDR R4, =__cpp(&gp_current_process)   ; Load the address of gp_current_process into R4
  LDR R4, [R4]                          ; Load the value of gp_current_process into R4
  STR SP, [R4, #04]                     ; Store the current SP into gp_current_process->mp_sp

=======
>>>>>>> parent of c8220ed... WIP: First attempt at switching to an assembly-based context switch.
  BLX  R12             ; Call SVC C Function, 
                       ; R12 contains the corresponding 
                       ; C kernel functions entry point
                       ; R0-R3 contains the kernel function input parameter (See AAPCS)
<<<<<<< HEAD

  ; This does the equivalent of __set_MSP(gp_current_process->mp_sp)
  LDR R4, =__cpp(&gp_current_process)   ; Load the address of gp_current_process into R4
  LDR R4, [R4]                          ; Load the value of gp_current_process into R4
  LDM R4, {R5, R6, R7, R8}              ; Load the first four fields of gp_current_process into R5, R6, R7, R8
  MOV SP, R6                            ; Store gp_current_process->mp_sp into the SP register
  
  CMP R7, #1                            ; Check if gp_current_process->m_unblocked is 1
  BNE FINISH                            ; If not, continue as normal at FINISH
  
  MOV R0, R8                            ; Store gp_current_process->m_ret_val into R0
  MOV R9, #0
  STR R7, [R9]                          ; Flip the unblocked flag back to 0

FINISH
  
=======
>>>>>>> parent of c8220ed... WIP: First attempt at switching to an assembly-based context switch.
  POP {R4-R11, LR}     ; Restore other registers for preemption caused by i-procs
  MRS  R12, MSP        ; Read MSP
  STR  R0, [R12]       ; store C kernel function return value in R0
                       ; to R0 on the exception stack frame  
SVC_EXIT  
	
  MVN  LR, #:NOT:0xFFFFFFF9  ; set EXC_RETURN value, Thread mode, MSP
  BX   LR
}
