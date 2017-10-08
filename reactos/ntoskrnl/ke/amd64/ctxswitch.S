/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/amd64/ctxswitch.S
 * PURPOSE:         Thread Context Switching
 *
 * PROGRAMMER:      Timo kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <asm.inc>
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

    /* Allocate a KEXCEPTION_FRAME on the stack (+8 for proper alignment) */
    sub rsp, KEXCEPTION_FRAME_LENGTH + 8
    .allocstack KEXCEPTION_FRAME_LENGTH + 8

    /* save non-volatiles in KEXCEPTION_FRAME */
    mov [rsp + KEXCEPTION_FRAME_Rbp], rbp
    .savereg rbp, KEXCEPTION_FRAME_Rbp
    mov [rsp + KEXCEPTION_FRAME_Rbx], rbx
    .savereg rbx, KEXCEPTION_FRAME_Rbx
    mov [rsp + KEXCEPTION_FRAME_Rdi], rdi
    .savereg rdi, KEXCEPTION_FRAME_Rdi
    mov [rsp + KEXCEPTION_FRAME_Rsi], rsi
    .savereg rsi, KEXCEPTION_FRAME_Rsi
    mov [rsp + KEXCEPTION_FRAME_R12], r12
    .savereg r12, KEXCEPTION_FRAME_R12
    mov [rsp + KEXCEPTION_FRAME_R13], r13
    .savereg r13, KEXCEPTION_FRAME_R13
    mov [rsp + KEXCEPTION_FRAME_R14], r14
    .savereg r14, KEXCEPTION_FRAME_R14
    mov [rsp + KEXCEPTION_FRAME_R15], r15
    .savereg r15, KEXCEPTION_FRAME_R15
    movdqa [rsp + KEXCEPTION_FRAME_Xmm6], xmm6
    movdqa [rsp + KEXCEPTION_FRAME_Xmm7], xmm7
    movdqa [rsp + KEXCEPTION_FRAME_Xmm8], xmm8
    movdqa [rsp + KEXCEPTION_FRAME_Xmm9], xmm9
    movdqa [rsp + KEXCEPTION_FRAME_Xmm10], xmm10
    movdqa [rsp + KEXCEPTION_FRAME_Xmm11], xmm11
    movdqa [rsp + KEXCEPTION_FRAME_Xmm12], xmm12
    movdqa [rsp + KEXCEPTION_FRAME_Xmm13], xmm13
    movdqa [rsp + KEXCEPTION_FRAME_Xmm14], xmm14
    movdqa [rsp + KEXCEPTION_FRAME_Xmm15], xmm15
    // KEXCEPTION_FRAME_MxCsr
    .endprolog

    /* Do the swap with the registers correctly setup */
    mov rcx, gs:[PcCurrentThread] /* Pointer to the new thread */
    call KiSwapContextInternal

    /* restore non-volatile registers */
    mov rbp, [rsp + KEXCEPTION_FRAME_Rbp]
    mov rbx, [rsp + KEXCEPTION_FRAME_Rbx]
    mov rdi, [rsp + KEXCEPTION_FRAME_Rdi]
    mov rsi, [rsp + KEXCEPTION_FRAME_Rsi]
    mov r12, [rsp + KEXCEPTION_FRAME_R12]
    mov r13, [rsp + KEXCEPTION_FRAME_R13]
    mov r14, [rsp + KEXCEPTION_FRAME_R14]
    mov r15, [rsp + KEXCEPTION_FRAME_R15]
    movdqa xmm6, [rsp + KEXCEPTION_FRAME_Xmm6]
    movdqa xmm7, [rsp + KEXCEPTION_FRAME_Xmm7]
    movdqa xmm8, [rsp + KEXCEPTION_FRAME_Xmm8]
    movdqa xmm9, [rsp + KEXCEPTION_FRAME_Xmm9]
    movdqa xmm10, [rsp + KEXCEPTION_FRAME_Xmm10]
    movdqa xmm11, [rsp + KEXCEPTION_FRAME_Xmm11]
    movdqa xmm12, [rsp + KEXCEPTION_FRAME_Xmm12]
    movdqa xmm13, [rsp + KEXCEPTION_FRAME_Xmm13]
    movdqa xmm14, [rsp + KEXCEPTION_FRAME_Xmm14]
    movdqa xmm15, [rsp + KEXCEPTION_FRAME_Xmm15]

    /* Clean stack and return */
    add rsp, KEXCEPTION_FRAME_LENGTH + 8
    ret
.ENDP

END
