/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ARM64 thread initialization and context switch support
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

VOID
NTAPI
KiArm64ExitToUser(
    _Inout_ PKTRAP_FRAME TrapFrame,
    _Inout_ PKEXCEPTION_FRAME ExceptionFrame);

/* The first switch to a user thread unwinds this, top down into user mode */
typedef struct _KUINIT_FRAME
{
    KSWITCH_FRAME CtxSwitchFrame;
    KSTART_FRAME StartFrame;
    KEXCEPTION_FRAME ExceptionFrame;
    KARM64_VFP_STATE VfpState;
    KTRAP_FRAME TrapFrame;
} KUINIT_FRAME, *PKUINIT_FRAME;

typedef struct _KKINIT_FRAME
{
    KSWITCH_FRAME CtxSwitchFrame;
    KSTART_FRAME StartFrame;
} KKINIT_FRAME, *PKKINIT_FRAME;

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
KiInitializeContextThread(
    _In_ PKTHREAD Thread,
    _In_ PKSYSTEM_ROUTINE SystemRoutine,
    _In_ PKSTART_ROUTINE StartRoutine,
    _In_ PVOID StartContext,
    _In_opt_ PCONTEXT Context)
{
    PKSWITCH_FRAME CtxSwitchFrame;
    PKSTART_FRAME StartFrame;

    if (Context)
    {
        PKUINIT_FRAME InitFrame = ((PKUINIT_FRAME)Thread->InitialStack) - 1;
        PKEXCEPTION_FRAME ExceptionFrame = &InitFrame->ExceptionFrame;
        PKTRAP_FRAME TrapFrame = &InitFrame->TrapFrame;

        RtlZeroMemory(InitFrame, sizeof(*InitFrame));
        CtxSwitchFrame = &InitFrame->CtxSwitchFrame;
        StartFrame = &InitFrame->StartFrame;

        Thread->KernelStack = InitFrame;
        Thread->PreviousMode = UserMode;

        ASSERT((Context->ContextFlags & CONTEXT_CONTROL) == CONTEXT_CONTROL);
        TrapFrame->VfpState = &InitFrame->VfpState;
        TrapFrame->PreviousMode = UserMode;
        KeContextToTrapFrame(Context,
                             ExceptionFrame,
                             TrapFrame,
                             Context->ContextFlags & ~CONTEXT_DEBUG_REGISTERS,
                             UserMode);

        StartFrame->UserThread = TRUE;
    }
    else
    {
        PKKINIT_FRAME InitFrame = ((PKKINIT_FRAME)Thread->InitialStack) - 1;

        RtlZeroMemory(InitFrame, sizeof(*InitFrame));
        CtxSwitchFrame = &InitFrame->CtxSwitchFrame;
        StartFrame = &InitFrame->StartFrame;

        Thread->KernelStack = InitFrame;
        Thread->PreviousMode = KernelMode;
        StartFrame->UserThread = FALSE;
    }

    CtxSwitchFrame->Return = (ULONG64)KiThreadStartup;
    CtxSwitchFrame->ApcBypass = TRUE;

    StartFrame->SystemRoutine = (ULONG64)SystemRoutine;
    StartFrame->StartRoutine = (ULONG64)StartRoutine;
    StartFrame->StartContext = (ULONG64)StartContext;
}

/**
 * @brief Runs the system routine of a new thread, called from KiThreadStartup.
 *
 * System threads never come back. User threads return once their trap
 * frame is ready for the first switch to user mode.
 */
VOID
NTAPI
KiArm64StartThread(
    _In_ PKSTART_FRAME StartFrame)
{
    PKTHREAD Thread = KeGetCurrentThread();
    PKTRAP_FRAME TrapFrame;

    KeLowerIrql(APC_LEVEL);

    ((PKSYSTEM_ROUTINE)StartFrame->SystemRoutine)((PKSTART_ROUTINE)StartFrame->StartRoutine,
                                                   (PVOID)StartFrame->StartContext);

    if (!StartFrame->UserThread)
        KeBugCheck(NO_USER_MODE_CONTEXT);

    /* User mode finds its TEB in x18 */
    TrapFrame = KeGetTrapFrame(Thread);
    TrapFrame->X18 = (ULONG64)Thread->Teb;

    KiArm64ExitToUser(TrapFrame, (PKEXCEPTION_FRAME)(StartFrame + 1));
}

BOOLEAN
KiSwapContextResume(
    _In_ BOOLEAN ApcBypass,
    _In_ PKTHREAD OldThread,
    _In_ PKTHREAD NewThread)
{
    PKIPCR Pcr = (PKIPCR)KeGetPcr();
    PKPROCESS OldProcess, NewProcess;
    ULONG64 CurrentCycleTime, ElapsedCycles;

    OldProcess = OldThread->ApcState.Process;
    NewProcess = NewThread->ApcState.Process;
    if (OldProcess != NewProcess)
        KeArm64SetTranslationBase(NewProcess->DirectoryTableBase[0]);

    CurrentCycleTime = KxQueryProcessorCycleTime();
    ElapsedCycles = CurrentCycleTime - Pcr->Prcb.StartCycles;
    ((PETHREAD)OldThread)->CycleTime += ElapsedCycles;
    InterlockedAdd64((PLONG64)&((PEPROCESS)OldProcess)->CycleTime, ElapsedCycles);
    Pcr->Prcb.StartCycles = CurrentCycleTime;

    /* NtCurrentTeb reads this in kernel mode */
    Pcr->NtTib.Self = (PVOID)NewThread->Teb;

    Pcr->Prcb.KeContextSwitches++;
    NewThread->ContextSwitches++;

    if (Pcr->Prcb.DpcRoutineActive)
    {
        KeBugCheckEx(ATTEMPTED_SWITCH_FROM_DPC,
                     (ULONG_PTR)OldThread,
                     (ULONG_PTR)NewThread,
                     (ULONG_PTR)OldThread->InitialStack,
                     0);
    }

    /* The old stack is no longer in use, another processor may take the thread */
    KeMemoryBarrier();
    OldThread->SwapBusy = FALSE;

    if (NewThread->ApcState.KernelApcPending)
    {
        if ((NewThread->SpecialApcDisable == 0) && (ApcBypass == 0))
            return TRUE;

        HalRequestSoftwareInterrupt(APC_LEVEL);
    }

    return FALSE;
}

DECLSPEC_NORETURN
VOID
KiIdleLoop(VOID)
{
    PKPRCB Prcb = KeGetCurrentPrcb();
    PKTHREAD OldThread, NewThread;

    for (;;)
    {
        _enable();
        YieldProcessor();
        YieldProcessor();
        _disable();

        if ((Prcb->DpcData[0].DpcQueueDepth) ||
            (Prcb->TimerRequest) ||
            (Prcb->DeferredReadyListHead.Next))
        {
            HalClearSoftwareInterrupt(DISPATCH_LEVEL);
            KiRetireDpcList(Prcb);
        }

        if (Prcb->NextThread)
        {
            _enable();

            OldThread = Prcb->CurrentThread;
            NewThread = Prcb->NextThread;
            Prcb->NextThread = NULL;
            Prcb->CurrentThread = NewThread;
            NewThread->State = Running;

#ifdef CONFIG_SMP
            KfRaiseIrql(SYNCH_LEVEL);
#endif
            KiSwapContext(APC_LEVEL, OldThread);
#ifdef CONFIG_SMP
            KeLowerIrql(DISPATCH_LEVEL);
#endif
        }
        else
        {
            /* The HAL returns with interrupts on */
            Prcb->PowerState.IdleFunction(&Prcb->PowerState);
        }
    }
}

VOID
NTAPI
KiSwapProcess(
    _In_ PKPROCESS NewProcess,
    _In_ PKPROCESS OldProcess)
{
#ifdef CONFIG_SMP
    PKPRCB Prcb = KeGetCurrentPrcb();

    InterlockedXor64((PLONG64)&NewProcess->ActiveProcessors, Prcb->SetMember);
    InterlockedXor64((PLONG64)&OldProcess->ActiveProcessors, Prcb->SetMember);
#else
    UNREFERENCED_PARAMETER(OldProcess);
#endif

    KeArm64SetTranslationBase(NewProcess->DirectoryTableBase[0]);
}

/**
 * @brief Copies the current stack to a new kernel stack.
 *
 * Every frame record saves the caller's frame pointer, the chain in the copy
 * is moved along with the thread's stack fields. KeSwitchKernelStack moves
 * sp and fp by the returned offset.
 */
LONG_PTR
NTAPI
KiSwitchKernelStack(
    _In_ PVOID StackBase,
    _In_ PVOID StackLimit,
    _In_ PULONG_PTR CallerFrame,
    _Out_ PVOID *OldStackBase)
{
    PKTHREAD Thread = KeGetCurrentThread();
    ULONG_PTR OldBase, OldLimit, Saved;
    PKTRAP_FRAME TrapFrame;
    PULONG_PTR Frame;
    LONG_PTR Offset;
    BOOLEAN Enabled;
    SIZE_T Size;

    OldBase = (ULONG_PTR)Thread->StackBase;
    OldLimit = Thread->StackLimit;
    Size = OldBase - OldLimit;
    ASSERT(Size <= (ULONG_PTR)StackBase - (ULONG_PTR)StackLimit);

    Enabled = KeDisableInterrupts();

    RtlCopyMemory((PUCHAR)StackBase - Size, (PVOID)OldLimit, Size);
    Offset = (PUCHAR)StackBase - (PUCHAR)OldBase;

    /* User mode frame pointers end the chain, they never point into the stack */
    Frame = (PULONG_PTR)((ULONG_PTR)CallerFrame + Offset);
    for (;;)
    {
        Saved = Frame[0];
        if ((Saved < OldLimit) || (Saved >= OldBase))
            break;

        Frame[0] = Saved + Offset;
        Frame = (PULONG_PTR)(Saved + Offset);
    }

    if (((ULONG_PTR)Thread->TrapFrame >= OldLimit) && ((ULONG_PTR)Thread->TrapFrame < OldBase))
        Thread->TrapFrame = (PKTRAP_FRAME)((ULONG_PTR)Thread->TrapFrame + Offset);

    Thread->InitialStack = (PVOID)((ULONG_PTR)Thread->InitialStack + Offset);
    Thread->StackBase = StackBase;
    Thread->StackLimit = (ULONG_PTR)StackLimit;
    Thread->LargeStack = TRUE;

    /* The user trap frame keeps a pointer to its floating point state */
    TrapFrame = KeGetTrapFrame(Thread);
    if (((ULONG_PTR)TrapFrame->VfpState >= OldLimit) && ((ULONG_PTR)TrapFrame->VfpState < OldBase))
        TrapFrame->VfpState = (PKARM64_VFP_STATE)((ULONG_PTR)TrapFrame->VfpState + Offset);

    KeRestoreInterrupts(Enabled);

    *OldStackBase = (PVOID)OldBase;
    return Offset;
}
