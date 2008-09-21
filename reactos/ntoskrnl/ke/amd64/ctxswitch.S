/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/amd64/ctxswitch.S
 * PURPOSE:         Thread Context Switching
 * 
 * PROGRAMMER:      Timo kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ndk/asm.h>

.intel_syntax noprefix

.altmacro
.macro UNIMPLEMENTED func
    jmp 2f
1:
    .ascii "Sorry, asm function "
    .ascii func
    .ascii " is unimplemented!\n\0"
2:
    movabs rcx, offset 1b
    call _DbgPrint
    ret
.endm


/* FUNCTIONS ****************************************************************/

/*++
 * KiThreadStartup
 *
 *     The KiThreadStartup routine is the beginning of any thread.
 *
 * Params:
 *     SystemRoutine - Pointer to the System Startup Routine. Either 
 *                     PspUserThreadStartup or PspSystemThreadStartup
 *
 *     StartRoutine - For Kernel Threads only, specifies the starting execution
 *                    point of the new thread.
 *
 *     StartContext - For Kernel Threads only, specifies a pointer to variable
 *                    context data to be sent to the StartRoutine above.
 *
 *     UserThread - Indicates whether or not this is a user thread. This tells
 *                  us if the thread has a context or not.
 *
 *     TrapFrame - Pointer to the KTHREAD to which the caller wishes to
 *           switch from.
 *
 * Returns:
 *     Should never return for a system thread. Returns through the System Call
 *     Exit Dispatcher for a user thread.
 *
 * Remarks:
 *     If a return from a system thread is detected, a bug check will occur.
 *
 *--*/
 .func KiThreadStartup
.globl _KiThreadStartup
_KiThreadStartup:

    /*
     * Clear all the non-volatile registers, so the thread won't be tempted to
     * expect any static data (like some badly coded usermode/win9x apps do)
     */
    xor rbx, rbx
    xor rsi, rsi
    xor rdi, rdi
    xor rbp, rbp
    xor r10, r10
    xor r11, r11
    xor r12, r12
    xor r13, r13
    xor r14, r14
    xor r15, r15

    /* It's now safe to go to APC */
    mov rax, APC_LEVEL
    mov cr8, rax

    /* 
     * Call the System Routine which is right on our stack now.
     * After we pop the pointer, the Start Routine/Context is on the 
     * stack, we pop it as parameters to the System Routine into rcx
     */
    pop rax
    pop rcx
    call rax

    /* The thread returned... was it a user-thread? */
    pop rcx
    or rcx, rcx
    jz BadThread

    /* Yes it was, set our trapframe for the System Call Exit Dispatcher */
    mov ebp, esp

    /* Exit back to user-mode */
//    jmp _KiServiceExit2
UNIMPLEMENTED "KiThreadStartup->KiServiceExit2"

BadThread:

    /* A system thread returned...this is very bad! */
    int 3
.endfunc


/*++
 * KiSwapContextInternal 
 *
 *     The KiSwapContextInternal routine switches context to another thread.
 *
 * Params:
 *     ESI - Pointer to the KTHREAD to which the caller wishes to
 *           switch to.
 *     EDI - Pointer to the KTHREAD to which the caller wishes to
 *           switch from.
 *
 * Returns:
 *     None.
 *
 * Remarks:
 *     Absolutely all registers except ESP can be trampled here for maximum code flexibility.
 *
 *--*/
.globl _KiSwapContextInternal
.func _KiSwapContextInternal, _KiSwapContextInternal
_KiSwapContextInternal:
    UNIMPLEMENTED "KiSwapContextInternal"
    ret

.endfunc

/**
 * KiSwapContext
 *
 * \brief
 *     The KiSwapContext routine switches context to another thread.
 *
 * BOOLEAN
 * KiSwapContext(PKTHREAD CurrentThread, PKTHREAD TargetThread);
 *
 * \param CurrentThread
 *     Pointer to the KTHREAD of the current thread.
 *                    
 * \param TargetThread
 *     Pointer to the KTHREAD to which the caller wishes to switch to.
 *
 * \returns
 *     The WaitStatus of the Target Thread.
 *
 * \remarks
 *     This is a wrapper around KiSwapContextInternal which will save all the
 *     non-volatile registers so that the Internal function can use all of
 *     them. It will also save the old current thread and set the new one.
 *
 *     The calling thread does not return after KiSwapContextInternal until 
 *     another thread switches to IT.
 *
 *--*/
.globl _KiSwapContext
.func _KiSwapContext, _KiSwapContext
_KiSwapContext:

    /* Save 10 registers */
    sub rsp, 10 * 8

    /* Save all the non-volatile ones */
    mov [rsp+72], r15
    mov [rsp+64], r14
    mov [rsp+56], r13
    mov [rsp+48], r12
    mov [rsp+40], r11
    mov [rsp+32], r10

    mov [rsp+24], rbx
    mov [rsp+16], rsi
    mov [rsp+8], rdi
    mov [rsp+0], rbp

    /* Get the PCR */
    mov rbx, gs:[KPCR_SELF]

    /* Get the current thread */
    mov rdi, rcx

    /* Get the New Thread */
    mov rsi, rdx

    /* Get the wait IRQL */
    movzx ecx, byte ptr [edi+KTHREAD_WAIT_IRQL]

    /* Do the swap with the registers correctly setup */
    call _KiSwapContextInternal

    /* Restore the registers */
    mov rbp, [rsp+0]
    mov rdi, [rsp+8]
    mov rsi, [rsp+16]
    mov rbx, [rsp+24]

    mov r10, [rsp+32]
    mov r11, [rsp+40]
    mov r12, [rsp+48]
    mov r13, [rsp+56]
    mov r14, [rsp+64]
    mov r15, [rsp+72]

    /* Clean stack */
    add esp, 10 * 8
    ret
.endfunc
