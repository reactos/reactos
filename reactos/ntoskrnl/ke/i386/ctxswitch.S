/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/i386/ctxswitch.S
 * PURPOSE:         Thread Context Switching
 * 
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  Gregor Anich (FPU Code)
 */

/* INCLUDES ******************************************************************/

#include <ndk/asm.h>
.intel_syntax noprefix

#define Ready 1
#define Running 2
#define WrDispatchInt 0x1F

/* FUNCTIONS ****************************************************************/

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
.globl @KiSwapContextInternal@0
.func @KiSwapContextInternal@0, @KiSwapContextInternal@0
@KiSwapContextInternal@0:
    /* Set APC Bypass Disable and old thread pointer */
    mov edx, edi
    or dl, cl

    /* Build switch frame */
    sub esp, 2 * 4
    mov ecx, esp
    call @KiSwapContextEntry@8
    mov ecx, 0xB00BFACA
    jmp $
.endfunc

/*++
 * KiSwapContext 
 *
 *     The KiSwapContext routine switches context to another thread.
 *
 * Params:
 *     TargetThread - Pointer to the KTHREAD to which the caller wishes to
 *                    switch to.
 *
 * Returns:
 *     The WaitStatus of the Target Thread.
 *
 * Remarks:
 *     This is a wrapper around KiSwapContextInternal which will save all the
 *     non-volatile registers so that the Internal function can use all of
 *     them. It will also save the old current thread and set the new one.
 *
 *     The calling thread does not return after KiSwapContextInternal until 
 *     another thread switches to IT.
 *
 *--*/
.globl @KiSwapContext@8
.func @KiSwapContext@8, @KiSwapContext@8
@KiSwapContext@8:

    /* Save 4 registers */
    sub esp, 4 * 4

    /* Save all the non-volatile ones */
    mov [esp+12], ebx
    mov [esp+8], esi
    mov [esp+4], edi
    mov [esp+0], ebp

    /* Get the current KPCR */
    mov ebx, fs:[KPCR_SELF]

    /* Get the Current Thread */
    mov edi, ecx

    /* Get the New Thread */
    mov esi, edx

    /* Get the wait IRQL */
    movzx ecx, byte ptr [edi+KTHREAD_WAIT_IRQL]

    /* Do the swap with the registers correctly setup */
    call @KiSwapContextInternal@0

    /* Return the registers */
    mov ebp, [esp+0]
    mov edi, [esp+4]
    mov esi, [esp+8]
    mov ebx, [esp+12]

    /* Clean stack */
    add esp, 4 * 4
    ret
.endfunc

/* DPC INTERRUPT HANDLER ******************************************************/

.globl _KiDispatchInterrupt@0
.func KiDispatchInterrupt@0
_KiDispatchInterrupt@0:

    /* Preserve EBX */
    push ebx

    /* Get the PCR  and disable interrupts */
    mov ebx, PCR[KPCR_SELF]
    cli

    /* Check if we have to deliver DPCs, timers, or deferred threads */
    mov eax, [ebx+KPCR_PRCB_DPC_QUEUE_DEPTH]
    or eax, [ebx+KPCR_PRCB_TIMER_REQUEST]
    or eax, [ebx+KPCR_PRCB_DEFERRED_READY_LIST_HEAD]
    jz CheckQuantum

    /* Save stack pointer and exception list, then clear it */
    push ebp
    push dword ptr [ebx+KPCR_EXCEPTION_LIST]
    mov dword ptr [ebx+KPCR_EXCEPTION_LIST], -1

    /* Save the stack and switch to the DPC Stack */
    mov edx, esp
    mov esp, [ebx+KPCR_PRCB_DPC_STACK]
    push edx

    /* Deliver DPCs */
    mov ecx, [ebx+KPCR_PRCB]
    call @KiRetireDpcList@4

    /* Restore stack and exception list */
    pop esp
    pop dword ptr [ebx+KPCR_EXCEPTION_LIST]
    pop ebp

CheckQuantum:

    /* Re-enable interrupts */
    sti

    /* Check if we have quantum end */
    cmp byte ptr [ebx+KPCR_PRCB_QUANTUM_END], 0
    jnz QuantumEnd

    /* Check if we have a thread to swap to */
    cmp byte ptr [ebx+KPCR_PRCB_NEXT_THREAD], 0
    je Return

    /* Make space on the stack to save registers */
    sub esp, 3 * 4
    mov [esp+8], esi
    mov [esp+4], edi
    mov [esp+0], ebp

    /* Get the current thread */
    mov edi, [ebx+KPCR_CURRENT_THREAD]

#ifdef CONFIG_SMP
    /* Raise to synch level */
    call _KeRaiseIrqlToSynchLevel@0

    /* Set context swap busy */
    mov byte ptr [edi+KTHREAD_SWAP_BUSY], 1

    /* Acquire the PRCB Lock */
    lock bts dword ptr [ebx+KPCR_PRCB_PRCB_LOCK], 0
    jnb GetNext
    lea ecx, [ebx+KPCR_PRCB_PRCB_LOCK]
    call @KefAcquireSpinLockAtDpcLevel@4
#endif

GetNext:
    /* Get the next thread and clear it */
    mov esi, [ebx+KPCR_PRCB_NEXT_THREAD]
    and dword ptr [ebx+KPCR_PRCB_NEXT_THREAD], 0

    /* Set us as the current running thread */
    mov [ebx+KPCR_CURRENT_THREAD], esi
    mov byte ptr [esi+KTHREAD_STATE_], Running
    mov byte ptr [edi+KTHREAD_WAIT_REASON], WrDispatchInt

    /* Put thread in ECX and get the PRCB in EDX */
    mov ecx, edi
    lea edx, [ebx+KPCR_PRCB_DATA]
    call @KiQueueReadyThread@8

    /* Set APC_LEVEL and do the swap */
    mov cl, APC_LEVEL
    call @KiSwapContextInternal@0

#ifdef CONFIG_SMP
    /* Lower IRQL back to dispatch */
    mov cl, DISPATCH_LEVEL
    call @KfLowerIrql@4
#endif

    /* Restore registers */
    mov ebp, [esp+0]
    mov edi, [esp+4]
    mov esi, [esp+8]
    add esp, 3*4

Return:
    /* All done */
    pop ebx
    ret

QuantumEnd:
    /* Disable quantum end and process it */
    mov byte ptr [ebx+KPCR_PRCB_QUANTUM_END], 0
    call _KiQuantumEnd@0
    pop ebx
    ret
.endfunc

/* FIXFIX: Move to C code ****/
.globl _Ki386SetupAndExitToV86Mode@4
.func Ki386SetupAndExitToV86Mode@4
_Ki386SetupAndExitToV86Mode@4:

    /* Enter V8086 mode */
    pushad
    sub esp, (12 + KTRAP_FRAME_LENGTH + NPX_FRAME_LENGTH)
    mov ecx, esp
    call @KiEnterV86Mode@4
    jmp $
.endfunc

.globl @KiSwitchThreads@8
.func @KiSwitchThreads@8, @KiSwitchThreads@8
@KiSwitchThreads@8:

    /* Load the new kernel stack and switch OS to new thread */
    mov esp, [edx+KTHREAD_KERNEL_STACK]
    mov edx, esp
    call @KiSwapContextExit@8
    
    /* Now we're on the new thread. Return to the caller to restore registers */
    add esp, 2 * 4
    ret
.endfunc

.globl @Ki386BiosCallReturnAddress@4
@Ki386BiosCallReturnAddress@4:

    /* Exit V8086 mode */
    call @KiExitV86Mode@4
    mov esp, eax
    add esp, (12 + KTRAP_FRAME_LENGTH + NPX_FRAME_LENGTH)
    popad
    ret 4

