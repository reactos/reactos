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

//#include <roscfg.h>
#include <ndk/asm.h>
.intel_syntax noprefix

#define Running 2
#define WrDispatchInt 0x1F

Dividend: .float 4195835.0
Divisor:  .float 3145727.0
Result1:  .float 0
Result2:  .float 0

/* FUNCTIONS ****************************************************************/

.globl _KiIsNpxErrataPresent@0
.func KiIsNpxErrataPresent@0
_KiIsNpxErrataPresent@0:

    /* Disable interrupts */
    cli

    /* Get CR0 and mask out FPU flags */
    mov eax, cr0
    mov ecx, eax
    and eax, ~(CR0_MP + CR0_TS + CR0_EM)
    mov cr0, eax

    /* Initialize the FPU */
    fninit

    /* Do the divison and inverse multiplication */
    fld qword ptr Dividend
    fstp qword ptr Result1
    fld qword ptr Divisor
    fstp qword ptr Result2
    fld qword ptr Result1
    fdiv qword ptr Result2
    fmul qword ptr Result2

    /* Do the compare and check flags */
    fcomp qword ptr Result1
    fstsw ax
    sahf

    /* Restore CR0 and interrupts */
    mov cr0, ecx
    sti

    /* Return errata status */
    xor eax, eax
    jz NoErrata
    inc eax

NoErrata:
    ret
.endfunc

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
 .func KiThreadStartup@156
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
.globl @KiSwapContextInternal@0
.func @KiSwapContextInternal@0, @KiSwapContextInternal@0
@KiSwapContextInternal@0:

    /* Save the IRQL */
    push ecx

#ifdef CONFIG_SMP
GetSwapLock:
    /* Acquire the swap lock */
    cmp [esi+KTHREAD_SWAP_BUSY], 0
    jz NotBusy
    pause
    jmp GetSwapLock
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
#ifdef DBG
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
#endif

SetStack:
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
#ifdef DBG
    test [ebp+KPROCESS_ACTIVE_PROCESSORS], ecx
    jz WrongActiveCpu
    test [eax+KPROCESS_ACTIVE_PROCESSORS], ecx
    jz WrongActiveCpu
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

#ifdef DBG
    /* Assert NPX State */
    test byte ptr [esi+KTHREAD_NPX_STATE], ~(NPX_STATE_NOT_LOADED)
    jnz InvalidNpx
    test dword ptr [eax - (NPX_FRAME_LENGTH - FN_CR0_NPX_STATE)], ~(CR0_PE + CR0_MP + CR0_EM + CR0_TS)
    jnz InvalidNpx
#endif

    /* Update CR0 */
    mov cr0, ecx
    jmp StackOk

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

#ifdef DBG
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
#ifndef NEW_SCHEDULER
    /* FIXME: ROS HACK */
    call _NtYieldExecution@0
#endif

    /* Check if a next thread is queued */
    cmp dword ptr [ebx+KPCR_PRCB_NEXT_THREAD], 0
#ifdef CONFIG_SMP
    jz NoNextThread
#else
    jz CpuIdle
#endif

#ifdef CONFIG_SMP
    /* There is, raise IRQL to synch level */
    mov ecx, SYNCH_LEVEL
    call @KfRaiseIrql@4
#endif
    sti

    /* Set the current thread to ready */
    mov edi, [ebx+KPCR_CURRENT_THREAD]
#ifdef CONFIG_SMP
    mov byte ptr [edi+KTHREAD_SWAP_BUSY], 1

    /* Acquire the PRCB Lock */
    lock bts [ebx+KPCR_PRCB_PRCB_LOCK], 0
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
    mov byte ptr [esi+KTHREAD_STATE], Running

#ifdef CONFIG_SMP
    /* Disable the idle scheduler and release the PRCB lock */
    and byte ptr [ebx+KPCR_PRCB_IDLE_SCHEDULE], 0
    and [ebx+KPCR_PRCB_PRCB_LOCK], 0
#endif

SwapContext:
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
    /* Clear the next thread, and put the thready as ready after lock release */
    and dword ptr [ebx+KPCR_PRCB_NEXT_THREAD], 0
    and dword ptr [ebx+KPCR_PRCB_PRCB_LOCK], 0
    and byte ptr [edi+KTHREAD_STATE], Ready
    jmp MainLoop

NoNextThread:
    /* Check if the idle scheduler is enabled */
    cmp byte ptr [ebx+KPCR_PRCB_IDLE_SCHEDULE], 0
    jz CpuIdle

    /* It is, so call the scheduler */
    lea ecx, [ebx+KPCR_PRCBDATA]
    call @KiIdleSchedule@4
    test eax, eax

    /* Get new thread pointers and either swap or idle loop again */
    mov esi, eax
    mov edi, [ebx+KPCR_PRCB_IDLE_THREAD]
    jnz SwapContext
    jmp MainLoop
#endif
.endfunc

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

.globl _KiSwapProcess@8
.func KiSwapProcess@8
_KiSwapProcess@8:

    /* Get process pointers */
    mov edx, [esp+4]
    mov eax, [esp+8]

#ifdef CONFIG_SMP
    /* Update active processors */
    mov ecx, fs:[KPCR_SET_MEMBER]
    lock xor [edx+KPROCESS_ACTIVE_PROCESSORS], ecx
    lock xor [eax+KPROCESS_ACTIVE_PROCESSORS], ecx

    /* Sanity check */
#ifdef DBG
    test dword ptr [edx+KPROCESS_ACTIVE_PROCESSORS], 0
    jz WrongCpu1
    test dword ptr [eax+KPROCESS_ACTIVE_PROCESSORS], 0
    jnz WrongCpu2
#endif
#endif

    /* Check if their LDTs changed */
    mov ecx, [edx+KPROCESS_LDT_DESCRIPTOR0]
    or ecx, [eax+KPROCESS_LDT_DESCRIPTOR0]
    jnz NewLdt

    /* Update CR3 */
    mov eax, [edx+KPROCESS_DIRECTORY_TABLE_BASE]
    mov cr3, eax

    /* Get the KTSS */
    mov ecx, fs:[KPCR_TSS]

    /* Clear GS on process swap */
    xor eax, eax
    mov gs, ax

    /* Update IOPM offset */
    mov ax, [edx+KPROCESS_IOPM_OFFSET]
    mov [ecx+KTSS_IOMAPBASE], ax

    /* Return */
    ret 8

NewLdt:
    /* FIXME: TODO */
    int 3

#ifdef DBG
WrongCpu1:
    int 3
WrongCpu2:
    int 3
#endif
.endfunc
