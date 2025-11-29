/* entry.S - Assembly entry points for kernel facilities */

#include "common.h"

.globl time_elapsed
.globl disable_count

.data
time_elapsed:
    .long 0
    .long 0

disable_count:
    .long 0

.text

/* Macro to send End Of Interrupt signal */
#define SEND_EOI \
    movb $0x20, %al; \
    outb %al, $0x20

/* Macro to save all registers */
#define SAVE_REGS \
    pushl %eax; \
    pushl %ebx; \
    pushl %ecx; \
    pushl %edx; \
    pushl %esi; \
    pushl %edi; \
    pushl %ebp

/* Macro to restore all registers */
#define RESTORE_REGS \
    popl %ebp; \
    popl %edi; \
    popl %esi; \
    popl %edx; \
    popl %ecx; \
    popl %ebx; \
    popl %eax

/* Macro to save stack pointer into PCB */
#define SAVE_STACK \
    movl current_running, %eax; \
    movl %esp, (%eax)

/* Macro to restore stack pointer from PCB */
#define RESTORE_STACK \
    movl current_running, %eax; \
    movl (%eax), %esp

/* Macro to enter critical section */
#define ENTER_CRITICAL \
    cli; \
    incl disable_count

/* Macro to leave critical section */
#define LEAVE_CRITICAL \
    decl disable_count; \
    cmpl $0, disable_count; \
    jne 1f; \
    sti; \
1:

/* Macro to test nested count and jump if zero */
#define TEST_NESTED_COUNT \
    movl current_running, %eax; \
    cmpl $0, NESTED_COUNT_OFFSET(%eax); \
    je nested_is_zero; \
    jmp nested_not_zero

.globl irq0_entry
irq0_entry:
    /* Interrupts are already disabled by hardware */
    
    /* Increment disable_count since interrupts are off */
    incl disable_count
    
    /* Save all registers */
    SAVE_REGS
    
    /* Increment time_elapsed (64-bit counter) */
    incl time_elapsed
    jnc 1f
    incl time_elapsed+4
1:
    
    /* Send End Of Interrupt signal */
    SEND_EOI
    
    /* Test nested_count and branch accordingly */
    TEST_NESTED_COUNT
    
nested_is_zero:
    /* We're not in a system call, so we can preempt */
    /* Save current stack pointer */
    SAVE_STACK
    
    /* Check for sleeping processes that need to wake up */
    call check_sleeping
    
    /* Put current running process back into ready queue */
    call put_current_running
    
    /* Schedule next process */
    call scheduler_entry
    
    /* Restore stack of new process */
    RESTORE_STACK
    
    /* Restore all registers */
    RESTORE_REGS
    
    /* Decrement disable_count before returning */
    decl disable_count
    
    /* Return from interrupt (re-enables interrupts) */
    iret

nested_not_zero:
    /* We're in a system call or kernel thread */
    /* Check for sleeping processes */
    call check_sleeping
    
    /* Just restore registers and return */
    RESTORE_REGS
    
    /* Decrement disable_count before returning */
    decl disable_count
    
    /* Return from interrupt */
    iret

/* System call entry point (provided as reference) */
.globl sysentry
sysentry:
    ENTER_CRITICAL
    SAVE_REGS
    SAVE_STACK
    
    /* Call system call handler */
    pushl %eax  /* syscall number */
    call system_call_helper
    addl $4, %esp
    
    RESTORE_STACK
    RESTORE_REGS
    LEAVE_CRITICAL
    iret

/* Exception handlers (simplified) */
.globl irq7_entry
irq7_entry:
    ENTER_CRITICAL
    SAVE_REGS
    
    SEND_EOI
    
    call irq7_handler
    
    RESTORE_REGS
    LEAVE_CRITICAL
    iret
