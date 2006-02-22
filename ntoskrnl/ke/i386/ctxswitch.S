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

#include <roscfg.h>
#include <internal/i386/ke.h>
#include <ndk/asm.h>
.intel_syntax noprefix

#define Running 2 
#define SIZEOF_TRAP_FRAME 0x8c
#define APC_LEVEL 1

/* GLOBALS ****************************************************************/  

.extern _DispatcherDatabaseLock

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
.globl _KiThreadStartup@156
_KiThreadStartup@156:

    /*
     * Clear all the non-volatile registers, so the thread won't be tempted to
     * expect any static data (like some badly coded usermode/win9x apps do)
     */
    xor ebx, ebx
    xor esi, esi
    xor edi, edi
    xor ebp, ebp

    /* It's now safe to go to APC */
    mov ecx, APC_LEVEL
    call @KfLowerIrql@4

    /* 
     * Call the System Routine which is right on our stack now.
     * After we pop the pointer, the Start Routine/Context will be on the 
     * stack, as parameters to the System Routine
     */
    pop eax
    call eax

    /* The thread returned... was it a user-thread? */
    pop ecx
    or ecx, ecx
    jz BadThread

    /* Yes it was, set our trapframe for the System Call Exit Dispatcher */
    mov ebp, esp

    /* Exit back to user-mode */
    jmp _KiServiceExit2

BadThread:

    /* A system thread returned...this is very bad! */
    int 3

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
@KiSwapContextInternal@0:

    /* Get the PCR. It's faster to use ebx+offset then fs:offset */
    mov ebx, [fs:KPCR_SELF]
    
    /* Set the Thread to running */
    mov byte ptr [esi+KTHREAD_STATE], Running

    /* Save the Exception list */
    push [ebx+KPCR_EXCEPTION_LIST]
    
    /* Switching, disable interrupts now */
    cli

    /* Save the initial stack in EAX */
    mov eax, [esi+KTHREAD_INITIAL_STACK]

    /* Save the stack limit in ecx */
    mov ecx, [esi+KTHREAD_STACK_LIMIT]

    /* Make space for the NPX Frame */
    sub eax, NPX_FRAME_LENGTH

    /* Set the KPCR stack values */
    mov [ebx+KPCR_INITIAL_STACK], eax
    mov [ebx+KPCR_STACK_LIMIT], ecx

#ifdef CONFIG_SMP
    /* Save FPU state if the thread has used it. */
    mov ecx, [edi+KTHREAD_INITIAL_STACK]
    sub ecx, NPX_FRAME_LENGTH
    mov dword ptr [ebx+KPCR_NPX_THREAD], 0
    test byte ptr [edi+KTHREAD_NPX_STATE], NPX_STATE_DIRTY
    jz 3f
    cmp dword ptr _KeI386FxsrPresent, 0
    je 1f
    fxsave [ecx]
    jmp 2f
1:
    fnsave [ecx]
2:
    mov byte ptr [edi+KTHREAD_NPX_STATE], NPX_STATE_VALID
3:
#endif /* CONFIG_SMP */

    /* Save the stack pointer in this processors TSS */
    mov ebp, [ebx+KPCR_TSS]

    /* Check if this isn't V86 Mode, so we can bias the Esp0 */
    test dword ptr [eax - KTRAP_FRAME_SIZE + KTRAP_FRAME_EFLAGS], X86_EFLAGS_VM
    jnz NoAdjust

    /* Bias esp */
    sub eax, KTRAP_FRAME_V86_GS - KTRAP_FRAME_SS

NoAdjust:

    /* Save it */
    push [ebp+KTSS_ESP0]

    /* Set new ESP0 */
    mov [ebp+KTSS_ESP0], eax

    /* Set TEB pointer */
    mov eax, [esi+KTHREAD_TEB]
    mov [ebx+KPCR_TEB], eax

    /* Check if address space switch is needed */
    mov eax, [esi+KTHREAD_APCSTATE_PROCESS]
    cmp eax, [edi+KTHREAD_APCSTATE_PROCESS]
    mov eax, [eax+KPROCESS_DIRECTORY_TABLE_BASE]

    /* Switch stacks */
    mov [edi+KTHREAD_KERNEL_STACK], esp
    mov esp, [esi+KTHREAD_KERNEL_STACK]

    jz	NoAddressSpaceSwitch

    /* Clear gs */
    xor ecx, ecx
    mov gs, cx

    /* Switch address space */
    mov cr3, eax
    mov [ebp+KTSS_CR3], eax

NoAddressSpaceSwitch:

    /* Stack is OK, safe to enable interrupts now */
    sti

    /* Check if address space switch is needed (the result from above is valid) */
    /* If they match, then use the fast-path and skip all this */
    jz SameProcess

    /* Get the new Process. */
    mov edi, [esi+KTHREAD_APCSTATE_PROCESS]

    /* Check if we need an LDT */
    xor eax, eax
    cmp [edi+KPROCESS_LDT_DESCRIPTOR0], eax
    jz NoLdt

    /* Write the LDT Selector */
    mov ecx, [ebx+KPCR_GDT]
    mov eax, [edi+KPROCESS_LDT_DESCRIPTOR0]
    mov [ecx+KGDT_LDT], eax
    mov eax, [edi+KPROCESS_LDT_DESCRIPTOR1]
    mov [ecx+KGDT_LDT+4], eax

    /* Save LDT Selector */
    mov eax, KGDT_LDT

NoLdt:

    /* Load LDT */
    lldt ax

    /* Get the IOPM */
    mov ecx, [edi+KPROCESS_IOPM_OFFSET]

    /* Set current IOPM offset in the TSS */
    mov [ebp+KTSS_IOMAPBASE], cx

SameProcess:

    /* Set the TEB */
    mov eax, [esi+KTHREAD_TEB]
    mov ecx, [ebx+KPCR_GDT]
    mov [ecx+0x3A], ax
    shr eax, 16
    mov [ecx+0x3C], al
    mov [ecx+0x3F], ah

    /* Increase context switches */
    inc dword ptr [esi+KTHREAD_CONTEXT_SWITCHES]

    /* Set TS in cr0 to catch FPU code and load the FPU state when needed */
#ifndef CONFIG_SMP
    cmp [ebx+KPCR_NPX_THREAD], esi
    je 4f
#endif /* !CONFIG_SMP */
    mov eax, cr0
    or eax, X86_CR0_TS
    mov cr0, eax
4:

    /* Restore ESP0 */
    pop [ebp+KTSS_ESP0]

    /* Restore exception list */
    pop [ebx+KPCR_EXCEPTION_LIST]

    /* Return */
#ifdef CONFIG_SMP
    mov ecx, offset _DispatcherDatabaseLock
    call @KefReleaseSpinLockFromDpcLevel@4
#endif
    ret 

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
 *     The WaitStatus of the Target Thread. NOT YET SUPPORTED.
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
.globl @KiSwapContext@4
@KiSwapContext@4:
    /* Note, we CANNOT touch ebp */

    /* Save 4 registers */
    sub esp, 4 * 4
    
    /* Save all the non-volatile ones */
    mov [esp+12], ebx
    mov [esp+8], esi
    mov [esp+4], edi
    mov [esp+0], ebp
    
    /* Get the Current Thread */
    mov edi, fs:[KPCR_CURRENT_THREAD]
    
    /* Get the New Thread */
    mov esi, ecx
    
    /* Save it as Current thread */
    mov fs:[KPCR_CURRENT_THREAD], esi
        
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

.globl _Ki386AdjustEsp0@4
.func Ki386AdjustEsp0@4
_Ki386AdjustEsp0@4:

    /* Get the current thread */
    mov eax, [fs:KPCR_CURRENT_THREAD]

    /* Get trap frame and stack */
    mov edx, [esp+4]
    mov eax, [eax+KTHREAD_INITIAL_STACK]

    /* Check if V86 */
    test dword ptr [edx+KTRAP_FRAME_EFLAGS], EFLAGS_V86_MASK
    jnz 1f

    /* Bias the stack */
    sub eax, KTRAP_FRAME_V86_GS - KTRAP_FRAME_SS

1:
    /* Skip FX Save Area */
    sub eax, SIZEOF_FX_SAVE_AREA

    /* Disable interrupts */
    pushf
    cli

    /* Adjust ESP0 */
    mov edx, [fs:KPCR_TSS]
    mov ss:[edx+KTSS_ESP0], eax

    /* Enable interrupts and return */
    popf
    ret 4
.endfunc
