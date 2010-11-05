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

    /* Save the IRQL */
    push ecx

#ifdef CONFIG_SMP
GetSwapLock:
    /* Acquire the swap lock */
    cmp byte ptr [esi+KTHREAD_SWAP_BUSY], 0
    jz NotBusy
    pause
    jmp GetSwapLock
NotBusy:
#endif
    /* Increase context switches (use ES for lazy load) */
    inc dword ptr es:[ebx+KPCR_CONTEXT_SWITCHES]

    /* Save the Exception list */
    push [ebx+KPCR_EXCEPTION_LIST]

    /* Check for WMI */
    cmp dword ptr [ebx+KPCR_PERF_GLOBAL_GROUP_MASK], 0
    jnz WmiTrace

AfterTrace:
#ifdef CONFIG_SMP
#if DBG
    /* Assert that we're on the right CPU */
    mov cl, [esi+KTHREAD_NEXT_PROCESSOR]
    cmp cl, [ebx+KPCR_PROCESSOR_NUMBER]
    jnz WrongCpu
#endif
#endif

    /* Get CR0 and save it */
    mov ebp, cr0
    mov edx, ebp

#ifdef CONFIG_SMP
    /* Check NPX State */
    cmp byte ptr [edi+KTHREAD_NPX_STATE], NPX_STATE_LOADED
    jz NpxLoaded
SetStack:
#endif

    /* Set new stack */
    mov [edi+KTHREAD_KERNEL_STACK], esp

    /* Checking NPX, disable interrupts now */
    mov eax, [esi+KTHREAD_INITIAL_STACK]
    cli

    /* Get the NPX State */
    movzx ecx, byte ptr [esi+KTHREAD_NPX_STATE]

    /* Clear the other bits, merge in CR0, merge in FPU CR0 bits and compare */
    and edx, ~(CR0_MP + CR0_EM + CR0_TS)
    or ecx, edx
    or ecx, [eax - (NPX_FRAME_LENGTH - FN_CR0_NPX_STATE)]
    cmp ebp, ecx
    jnz NewCr0

StackOk:
    /* Enable interrupts and set the current stack */
    sti
    mov esp, [esi+KTHREAD_KERNEL_STACK]

    /* Check if address space switch is needed */
    mov ebp, [esi+KTHREAD_APCSTATE_PROCESS]
    mov eax, [edi+KTHREAD_APCSTATE_PROCESS]
    cmp ebp, eax
    jz SameProcess

#ifdef CONFIG_SMP
    /* Get the active processors and XOR with the process' */
    mov ecx, [ebx+KPCR_SET_MEMBER_COPY]
    lock xor [ebp+KPROCESS_ACTIVE_PROCESSORS], ecx
    lock xor [eax+KPROCESS_ACTIVE_PROCESSORS], ecx

    /* Assert change went ok */
#if DBG
    test [ebp+KPROCESS_ACTIVE_PROCESSORS], ecx
    jz WrongActiveCpu
    test [eax+KPROCESS_ACTIVE_PROCESSORS], ecx
    jnz WrongActiveCpu
#endif
#endif

    /* Check if we need an LDT */
    mov ecx, [ebp+KPROCESS_LDT_DESCRIPTOR0]
    or ecx, [eax+KPROCESS_LDT_DESCRIPTOR0]
    jnz LdtReload

UpdateCr3:
    /* Switch address space */
    mov eax, [ebp+KPROCESS_DIRECTORY_TABLE_BASE]
    mov cr3, eax

SameProcess:

#ifdef CONFIG_SMP
    /* Release swap lock */
    and byte ptr [edi+KTHREAD_SWAP_BUSY], 0
#endif

    /* Clear gs */
    xor eax, eax
    mov gs, ax

    /* Set the TEB */
    mov eax, [esi+KTHREAD_TEB]
    mov [ebx+KPCR_TEB], eax
    mov ecx, [ebx+KPCR_GDT]
    mov [ecx+0x3A], ax
    shr eax, 16
    mov [ecx+0x3C], al
    mov [ecx+0x3F], ah

    /* Get stack pointer */
    mov eax, [esi+KTHREAD_INITIAL_STACK]

    /* Make space for the NPX Frame */
    sub eax, NPX_FRAME_LENGTH

    /* Check if this isn't V86 Mode, so we can bias the Esp0 */
    test dword ptr [eax - KTRAP_FRAME_SIZE + KTRAP_FRAME_EFLAGS], EFLAGS_V86_MASK
    jnz NoAdjust

    /* Bias esp */
    sub eax, KTRAP_FRAME_V86_GS - KTRAP_FRAME_SS

NoAdjust:

    /* Set new ESP0 */
    mov ecx, [ebx+KPCR_TSS]
    mov [ecx+KTSS_ESP0], eax

    /* Set current IOPM offset in the TSS */
    mov ax, [ebp+KPROCESS_IOPM_OFFSET]
    mov [ecx+KTSS_IOMAPBASE], ax

    /* Increase context switches */
    inc dword ptr [esi+KTHREAD_CONTEXT_SWITCHES]

    /* Restore exception list */
    pop [ebx+KPCR_EXCEPTION_LIST]

    /* Restore IRQL */
    pop ecx

    /* DPC shouldn't be active */
    cmp byte ptr [ebx+KPCR_PRCB_DPC_ROUTINE_ACTIVE], 0
    jnz BugCheckDpc

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

LdtReload:
    /* Check if it's empty */
    mov eax, [ebp+KPROCESS_LDT_DESCRIPTOR0]
    test eax, eax
    jz LoadLdt

    /* Write the LDT Selector */
    mov ecx, [ebx+KPCR_GDT]
    mov [ecx+KGDT_LDT], eax
    mov eax, [ebp+KPROCESS_LDT_DESCRIPTOR1]
    mov [ecx+KGDT_LDT+4], eax

    /* Write the INT21 handler */
    mov ecx, [ebx+KPCR_IDT]
    mov eax, [ebp+KPROCESS_INT21_DESCRIPTOR0]
    mov [ecx+0x108], eax
    mov eax, [ebp+KPROCESS_INT21_DESCRIPTOR1]
    mov [ecx+0x10C], eax

    /* Save LDT Selector */
    mov eax, KGDT_LDT

LoadLdt:
    lldt ax
    jmp UpdateCr3

NewCr0:

#if DBG
    /* Assert NPX State */
    test byte ptr [esi+KTHREAD_NPX_STATE], ~(NPX_STATE_NOT_LOADED)
    jnz InvalidNpx
    test dword ptr [eax - (NPX_FRAME_LENGTH - FN_CR0_NPX_STATE)], ~(CR0_PE + CR0_MP + CR0_EM + CR0_TS)
    jnz InvalidNpx
#endif

    /* Update CR0 */
    mov cr0, ecx
    jmp StackOk

#ifdef CONFIG_SMP
NpxLoaded:

    /* Mask out FPU flags */
    and edx, ~(CR0_MP + CR0_EM + CR0_TS)

    /* Get the NPX Frame */
    mov ecx, [edi+KTHREAD_INITIAL_STACK]
    sub ecx, NPX_FRAME_LENGTH

    /* Check if we have a new CR0 */
    cmp ebp, edx
    jz Cr0Equal

    /* We do, update it */
    mov cr0, edx
    mov ebp, edx

Cr0Equal:

    /* Save the NPX State */
    fxsave [ecx]
    mov byte ptr [edi+KTHREAD_NPX_STATE], NPX_STATE_NOT_LOADED

    /* Clear the NPX Thread */
    mov dword ptr [ebx+KPCR_NPX_THREAD], 0

    /* Jump back */
    jmp SetStack
#endif

WmiTrace:

    /* No WMI support yet */
    int 3

    /* Jump back */
    jmp AfterTrace

BugCheckDpc:

    /* Bugcheck the machine, printing out the threads being switched */
    mov eax, [edi+KTHREAD_INITIAL_STACK]
    push 0
    push eax
    push esi
    push edi
    push ATTEMPTED_SWITCH_FROM_DPC
    call _KeBugCheckEx@20

#if DBG
InvalidNpx:
    int 3
WrongActiveCpu:
    int 3
WrongCpu:
    int 3
#endif
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

.globl @KiIdleLoop@0
.func @KiIdleLoop@0, @KiIdleLoop@0
@KiIdleLoop@0:

    /* Set EBX */
    mov ebx, fs:[KPCR_SELF]

    /* Jump into mainline code */
    jmp MainLoop

CpuIdle:
    /* Call the CPU's idle function */
    lea ecx, [ebx+KPCR_PRCB_POWER_STATE_IDLE_FUNCTION]
    call [ecx]

MainLoop:
    /* Cycle interrupts for 1 cycle */
    sti
    nop
    nop
    cli

    /* Check if we have to deliver DPCs, timers, or deferred threads */
    mov eax, [ebx+KPCR_PRCB_DPC_QUEUE_DEPTH]
    or eax, [ebx+KPCR_PRCB_TIMER_REQUEST]
#ifdef CONFIG_SMP
    or eax, [ebx+KPCR_PRCB_DEFERRED_READY_LIST_HEAD]
#endif
    jz CheckSchedule

    mov cl, DISPATCH_LEVEL
    call @HalClearSoftwareInterrupt@4
    
    /* Handle the above */
    lea ecx, [ebx+KPCR_PRCB_DATA]
    call @KiRetireDpcList@4

CheckSchedule:
    /* Check if a next thread is queued */
    cmp dword ptr [ebx+KPCR_PRCB_NEXT_THREAD], 0
#ifdef CONFIG_SMP
    jz NoNextThread
#else
    jz CpuIdle
#endif

#ifdef CONFIG_SMP
    /* There is, raise IRQL to synch level */
    call _KeRaiseIrqlToSynchLevel@0
#endif
    sti

    /* Set the current thread to ready */
    mov edi, [ebx+KPCR_CURRENT_THREAD]
#ifdef CONFIG_SMP
    mov byte ptr [edi+KTHREAD_SWAP_BUSY], 1

    /* Acquire the PRCB Lock */
    lock bts dword ptr [ebx+KPCR_PRCB_PRCB_LOCK], 0
    jnb CheckNext
    lea ecx, [ebx+KPCR_PRCB_PRCB_LOCK]
    call @KefAcquireSpinLockAtDpcLevel@4
#endif

CheckNext:
    /* Check if the next thread is the current */
    mov esi, [ebx+KPCR_PRCB_NEXT_THREAD]
#ifdef CONFIG_SMP
    cmp esi, edi
    jz SameThread
#endif

    /* Clear the next thread and set this one instead */
    and dword ptr [ebx+KPCR_PRCB_NEXT_THREAD], 0
    mov [ebx+KPCR_CURRENT_THREAD], esi

    /* Set the thread as running */
    mov byte ptr [esi+KTHREAD_STATE_], Running

#ifdef CONFIG_SMP
    /* Disable the idle scheduler and release the PRCB lock */
    and byte ptr [ebx+KPCR_PRCB_IDLE_SCHEDULE], 0
    and dword ptr [ebx+KPCR_PRCB_PRCB_LOCK], 0
#endif

SwapContext:
    /* ReactOS Mm Hack */
    mov ecx, esi
    call @MiSyncForContextSwitch@4

    /* Swap context at APC_LEVEL */
    mov ecx, APC_LEVEL
    call @KiSwapContextInternal@0

#ifdef CONFIG_SMP
    /* Lower to DPC level */
    mov ecx, DISPATCH_LEVEL
    call @KfLowerIrql@4
#endif
    jmp MainLoop

#ifdef CONFIG_SMP
SameThread:
    /* Clear the next thread, and put the thread as ready after lock release */
    and dword ptr [ebx+KPCR_PRCB_NEXT_THREAD], 0
    and dword ptr [ebx+KPCR_PRCB_PRCB_LOCK], 0
    and byte ptr [edi+KTHREAD_STATE_], Ready
    jmp MainLoop

NoNextThread:
    /* Check if the idle scheduler is enabled */
    cmp byte ptr [ebx+KPCR_PRCB_IDLE_SCHEDULE], 0
    jz CpuIdle

    /* It is, so call the scheduler */
    lea ecx, [ebx+KPCR_PRCB_DATA]
    call @KiIdleSchedule@4
    test eax, eax

    /* Get new thread pointers and either swap or idle loop again */
    mov esi, eax
    mov edi, [ebx+KPCR_PRCB_IDLE_THREAD]
    jnz SwapContext
    jmp MainLoop
#endif
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

.globl @Ki386BiosCallReturnAddress@4
@Ki386BiosCallReturnAddress@4:

    /* Exit V8086 mode */
    call @KiExitV86Mode@4
    mov esp, eax
    add esp, (12 + KTRAP_FRAME_LENGTH + NPX_FRAME_LENGTH)
    popad
    ret 4

