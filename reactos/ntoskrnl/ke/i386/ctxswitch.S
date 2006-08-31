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

/* FUNCTIONS ****************************************************************/

.globl _KiIsNpxPresent@0
.func KiIsNpxPresent@0
_KiIsNpxPresent@0:

    /* Save stack */
    push ebp

    /* Get CR0 and mask out FPU flags */
    mov eax, cr0
    and eax, ~(CR0_MP + CR0_TS + CR0_EM + CR0_ET)

    /* Initialize the FPU and assume FALSE for return */
    xor edx, edx
    fninit

    /* Save magic value on stack */
    mov ecx, 0x42424242
    push ecx

    /* Setup stack for FPU store */
    mov ebp ,esp
    fnstsw [ebp]

    /* Now check if our magic got cleared */
    cmp byte ptr [ebp], 0
    jnz NoFpu

    /* Enable FPU, set return to TRUE */
    or eax, CR0_ET
    mov edx, 1

    /* If this is a 486 or higher, enable INT 16 as well */
    cmp dword ptr fs:KPCR_PRCB_CPU_TYPE, 3
    jbe NoFpu
    or eax, CR0_NE

NoFpu:
    /* Set emulation enabled during the first boot phase and set the CR0 */
    or eax, (CR0_EM + CR0_TS)
    mov cr0, eax

    /* Restore stack */
    pop eax
    pop ebp

    /* Return true or false */
    mov eax, edx
    ret
.endfunc

.globl _KiFlushNPXState@4
.func KiFlushNPXState@4
_KiFlushNPXState@4:

    /* Save volatiles and disable interrupts */
    push esi
    push edi
    push ebx
    pushfd
    cli

    /* Save the PCR and get the current thread */
    mov edi, fs:[KPCR_SELF]
    mov esi, [edi+KPCR_CURRENT_THREAD]

    /* Check if we're already loaded */
    cmp byte ptr [esi+KTHREAD_NPX_STATE], NPX_STATE_LOADED
    je IsValid

    /* Check if we're supposed to get it */
    cmp dword ptr [esp+20], 0
    je Return

#ifdef DBG
    /* Assert Fxsr support */
    test byte ptr _KeI386FxsrPresent, 1
    jnz AssertOk
    int 3
AssertOk:
#endif

    /* Get CR0 and test if it's valid */
    mov ebx, cr0
    test bl, CR0_MP + CR0_TS + CR0_EM
    jz Cr0OK

    /* Enable fnsave to work */
    and ebx, ~(CR0_MP + CR0_TS + CR0_EM)
    mov cr0, ebx

Cr0OK:
    /* Check if we are the NPX Thread */
    mov eax, [edi+KPCR_NPX_THREAD]
    or eax, eax
    jz DontSave

    /* Check if it's not loaded */
    cmp byte ptr [eax+KTHREAD_NPX_STATE], NPX_STATE_NOT_LOADED
    jnz DontSave

#ifdef DBG
    /* We are the NPX Thread with an unloaded NPX State... this isn't normal! */
    int 3
#endif

    /* Save the NPX State */
    mov ecx, [eax+KTHREAD_INITIAL_STACK]
    sub ecx, NPX_FRAME_LENGTH
    fxsave [ecx]
    mov byte ptr [eax+KTHREAD_NPX_STATE], NPX_STATE_NOT_LOADED

DontSave:
    /* Load the NPX State */
    mov ecx, [esi+KTHREAD_INITIAL_STACK]
    sub ecx, NPX_FRAME_LENGTH
    fxrstor [ecx]

    /* Get the CR0 state and destination */
    mov edx, [ecx+FN_CR0_NPX_STATE]
    mov ecx, [esp+20]
    jmp DoneLoad

IsValid:
    /* We already have a valid state, flush it */
    mov ebx, cr0
    test bl, CR0_MP + CR0_TS + CR0_EM
    jz Cr0OK2

    /* Enable fnsave to work */
    and ebx, ~(CR0_MP + CR0_TS + CR0_EM)
    mov cr0, ebx

Cr0OK2:
    /* Get the kernel stack */
    mov ecx, [esi+KTHREAD_INITIAL_STACK]
    test byte ptr _KeI386FxsrPresent, 1
    lea ecx, [ecx-NPX_FRAME_LENGTH]

    /* Set the NPX State */
    mov byte ptr [esi+KTHREAD_NPX_STATE], NPX_STATE_NOT_LOADED

    /* Get Cr0 */
    mov edx, [ecx+FN_CR0_NPX_STATE]
    jz DoneLoad

    /* Save the FX State */
    fxsave [ecx]

    /* Check if we also have to save it in the parameter */
    mov ecx, [esp+20]
    jecxz NoSave

DoneLoad:
    /* Save the Fn state in the parameter we got */
    fnsave [ecx]
    fwait

NoSave:
    /* Clear eax */
    xor eax, eax

    /* Add NPX State */
    or ebx, NPX_STATE_NOT_LOADED

    /* Clear the NPX thread */
    mov [edi+KPCR_NPX_THREAD], eax

    /* Add saved CR0 into NPX State, and set it */
    or ebx, edx
    mov cr0, ebx

    /* Re-enable interrupts and return */
Return:
    popf
    pop ebx
    pop edi
    pop esi
    ret 4

.endfunc

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

    /* Set the Thread to running */
    mov byte ptr es:[esi+KTHREAD_STATE], Running

    /* Save the IRQL */
    push ecx

    /* Save the Exception list */
    push [ebx+KPCR_EXCEPTION_LIST]

    /* DPC shouldn't be active */
    cmp byte ptr [ebx+KPCR_PRCB_DPC_ROUTINE_ACTIVE], 0
    jnz BugCheckDpc

    /* Switching, disable interrupts now */
    cli

    /* Update kernel stack */
    mov [edi+KTHREAD_KERNEL_STACK], esp

    /* Get stack pointers */
    mov eax, [esi+KTHREAD_INITIAL_STACK]
    mov ecx, [esi+KTHREAD_STACK_LIMIT]

    /* Make space for the NPX Frame */
    sub eax, NPX_FRAME_LENGTH

    /* Set the KPCR stack values */
    mov [ebx+KPCR_INITIAL_STACK], eax
    mov [ebx+KPCR_STACK_LIMIT], ecx

    /* FIXME Check and update CR0 */

    /* Check if this isn't V86 Mode, so we can bias the Esp0 */
    test dword ptr [eax - KTRAP_FRAME_SIZE + KTRAP_FRAME_EFLAGS], X86_EFLAGS_VM
    jnz NoAdjust

    /* Bias esp */
    sub eax, KTRAP_FRAME_V86_GS - KTRAP_FRAME_SS

NoAdjust:

    /* Set new ESP0 */
    mov ecx, [ebx+KPCR_TSS]
    mov [ecx+KTSS_ESP0], eax

    /* Switch to new stack */
    mov esp, [esi+KTHREAD_KERNEL_STACK]

    /* Set TEB pointer */
    mov eax, [esi+KTHREAD_TEB]
    mov [ebx+KPCR_TEB], eax

    /* Stack is OK, safe to enable interrupts now */
    sti

    /* Check if address space switch is needed */
    mov eax, [esi+KTHREAD_APCSTATE_PROCESS]
    cmp eax, [edi+KTHREAD_APCSTATE_PROCESS]
    jz SameProcess

    /* Get the new Process. */
    mov edi, [esi+KTHREAD_APCSTATE_PROCESS]

    /* Check if we need an LDT */
    xor eax, eax
    cmp [edi+KPROCESS_LDT_DESCRIPTOR0], eax
    jnz LdtStuff

LoadLdt:
    /* Load LDT */
    lldt ax

    /* Clear gs */
    xor eax, eax
    mov gs, ax

    /* Get the address space */
    mov eax, [edi+KPROCESS_DIRECTORY_TABLE_BASE]

    /* Get the IOPM and TSS */
    mov ebp, [ebx+KPCR_TSS]
    mov ecx, [edi+KPROCESS_IOPM_OFFSET]

    /* Switch address space */
    mov [ebp+KTSS_CR3], eax
    mov cr3, eax

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
    //inc dword ptr [esi+KPRC_PRCB_CONTEXT_SWITCHES]

    /* Restore exception list */
    pop [ebx+KPCR_EXCEPTION_LIST]

    /* Restore IRQL */
    pop ecx

    /* Check if kernel APCs are pending */
    cmp byte ptr [esi+KTHREAD_PENDING_KERNEL_APC], 0
    jnz CheckApc

    /* No APCs, return */
    xor eax, eax
    ret

CheckApc:

    /* Check if they're disabled */
    cmp word ptr [esi+KTHREAD_SPECIAL_APC_DISABLE], 0
    jnz ApcReturn
    test cl, cl
    jz ApcReturn

    /* Request APC Delivery */
    mov cl, APC_LEVEL
    call @HalRequestSoftwareInterrupt@4
    or eax, esp

ApcReturn:

    /* Return with APC pending */
    setz al
    ret

LdtStuff:

    /* Write the LDT Selector */
    mov ecx, [ebx+KPCR_GDT]
    mov eax, [edi+KPROCESS_LDT_DESCRIPTOR0]
    mov [ecx+KGDT_LDT], eax
    mov eax, [edi+KPROCESS_LDT_DESCRIPTOR1]
    mov [ecx+KGDT_LDT+4], eax

    /* Write the INT21 handler */
    mov ecx, [ebx+KPCR_IDT]
    mov eax, [ebp+KPROCESS_INT21_DESCRIPTOR0]
    mov [ecx+0x108], eax
    mov eax, [ebp+KPROCESS_INT21_DESCRIPTOR1]
    mov [ecx+0x10C], eax

    /* Save LDT Selector */
    mov eax, KGDT_LDT
    jmp LoadLdt

BugCheckDpc:
    push ATTEMPTED_SWITCH_FROM_DPC
    call _KeBugCheck@4

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
.globl @KiSwapContext@8
@KiSwapContext@8:
    /* Note, we CANNOT touch ebp */

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

    /* Save it as Current thread */
    mov fs:[KPCR_CURRENT_THREAD], esi

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
