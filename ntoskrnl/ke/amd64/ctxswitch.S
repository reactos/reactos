/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/amd64/ctxswitch.S
 * PURPOSE:         Thread Context Switching
 *
 * PROGRAMMER:      Timo kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ksamd64.inc>

EXTERN KiSwapContextResume:PROC

/* FUNCTIONS ****************************************************************/

.code64

/*!
 * \name KiThreadStartup
 *
 * \brief
 *     The KiThreadStartup routine is the beginning of any thread.
 *
 * VOID
 * KiThreadStartup(
 *     IN PKSTART_ROUTINE StartRoutine<rcx>,
 *     IN PVOID StartContext<rdx>,
 *     IN PVOID P3<r8>,
 *     IN PVOID P4<r9>,
 *     IN PVOID SystemRoutine);
 *
 * \param StartRoutine
 *     For Kernel Threads only, specifies the starting execution point
 *     of the new thread.
 *
 * \param StartContext
 *     For Kernel Threads only, specifies a pointer to variable
 *     context data to be sent to the StartRoutine above.
 *
 * \param P3, P4 - not used atm
 *
 * \param SystemRoutine
 *     Pointer to the System Startup Routine.
 *     Either PspUserThreadStartup or PspSystemThreadStartup
 *
 * \return
 *     Should never return for a system thread. Returns through the System Call
 *     Exit Dispatcher for a user thread.
 *
 * \remarks
 *     If a return from a system thread is detected, a bug check will occur.
 *
 *--*/
PUBLIC KiThreadStartup
.PROC KiThreadStartup
    /* KSTART_FRAME is already on the stack when we enter here.
     * The virtual prolog looks like this:
     * sub rsp, 5 * 8
     * mov [rsp + SfP1Home], rcx
     * mov [rsp + SfP2Home], rdx
     * mov [rsp + SfP3Home], r8
     * mov [rsp + SfP4Home], r9
     */

    /* Terminate the unwind chain, by setting rbp as frame pointer,
       which contains 0 */
    .setframe rbp, 0
    .endprolog

    /* Clear all the non-volatile registers, so the thread won't be tempted to
     * expect any static data (like some badly coded usermode/win9x apps do) */
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

    /* We have the KSTART_FRAME on the stack, P1Home and P2Home are preloaded
     * with the parameters for the system routine. The address of the system
     * routine is stored in P4Home. */
    mov rcx, [rsp + SfP1Home] /* StartRoutine */
    mov rdx, [rsp + SfP2Home] /* StartContext */
    mov r8, [rsp + SfP3Home]  /* ? */
    call qword ptr [rsp + SfP4Home]     /* SystemRoutine */

    /* The thread returned. If it was a user-thread, we have a return address
       and all is well, otherwise this is very bad. */
    mov rcx, [rsp + SfReturn]
    or rcx, rcx
    jnz .leave

    /* A system thread returned...this is very bad! */
    int 3

.leave:
    /* It was a user thread, set our trapframe for the System Call Exit Dispatcher */
    lea rcx, [rsp + 6 * 8 + KEXCEPTION_FRAME_LENGTH]

    /* Return to the trap exit code */
    add rsp, 5 * 8
    ret
.ENDP

/*!
 * \name KiSwapContextInternal
 *
 * \brief
 *     The KiSwapContextInternal routine switches context to another thread.
 *
 * \param rcx
 *     Pointer to the KTHREAD to which the caller wishes to switch to.
 *
 * \param rdx
 *     Pointer to the KTHREAD to which the caller wishes to switch from.
 *
 * \param r8b
 *     APC bypass
 *
 * \return
 *     None.
 *
 * \remarks
 *     ...
 *
 *--*/
PUBLIC KiSwapContextInternal
.PROC KiSwapContextInternal

    push rbp
    .pushreg rbp
    sub rsp, 6 * 8
    .allocstack 6 * 8
    .endprolog

    /* Save APC bypass */
    mov [rsp + SwApcBypass], r8b

    /* Save kernel stack of old thread */
    mov [rdx + KTHREAD_KernelStack], rsp

    /* Save new thread in rbp */
    mov rbp, rcx

    //call KiSwapContextSuspend

    /* Load stack of new thread */
    mov rsp, [rbp + KTHREAD_KernelStack]

    /* Reload APC bypass */
    mov r8b, [rsp + SwApcBypass]

    call KiSwapContextResume

    /* Cleanup and return */
    add rsp, 6 * 8
    pop rbp
    ret

.ENDP



/*!
 * KiSwapContext
 *
 * \brief
 *     The KiSwapContext routine switches context to another thread.
 *
 * BOOLEAN
 * KiSwapContext(KIRQL WaitIrql, PKTHREAD CurrentThread);
 *
 * \param WaitIrql <rcx>
 *     ...
 *
 * \param CurrentThread <rdx>
 *     Pointer to the KTHREAD of the current thread.
 *
 * \return
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
PUBLIC KiSwapContext
.PROC KiSwapContext

    /* Generate a KEXCEPTION_FRAME on the stack */
    GENERATE_EXCEPTION_FRAME

    /* Do the swap with the registers correctly setup */
    mov rcx, gs:[PcCurrentThread] /* Pointer to the new thread */
    call KiSwapContextInternal

    /* Restore the registers from the KEXCEPTION_FRAME */
    RESTORE_EXCEPTION_STATE

    /* Return */
    ret
.ENDP

END
