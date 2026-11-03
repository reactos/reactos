/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ARM64 processor freeze for the kernel debugger
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/*
 * The states follow the x64 implementation. Without an NMI the freeze
 * request travels as an IPI, so a target running above IPI_LEVEL only
 * freezes once it drops below it.
 */

/* GLOBALS *******************************************************************/

PKPRCB KiFreezeOwner;

/* FUNCTIONS *****************************************************************/

VOID
KiSaveProcessorState(
    _In_ PKTRAP_FRAME TrapFrame,
    _In_ PKEXCEPTION_FRAME ExceptionFrame)
{
    PKPRCB Prcb = KeGetCurrentPrcb();

    Prcb->ProcessorState.ContextFrame.ContextFlags = CONTEXT_ALL;
    KeTrapFrameToContext(TrapFrame, ExceptionFrame, &Prcb->ProcessorState.ContextFrame);
    KiSaveProcessorControlState(&Prcb->ProcessorState);
}

VOID
KiRestoreProcessorState(
    _Out_ PKTRAP_FRAME TrapFrame,
    _Out_ PKEXCEPTION_FRAME ExceptionFrame)
{
    PKPRCB Prcb = KeGetCurrentPrcb();

    KeContextToTrapFrame(&Prcb->ProcessorState.ContextFrame,
                         ExceptionFrame,
                         TrapFrame,
                         CONTEXT_ALL,
                         KiGetPreviousMode(TrapFrame));
}

BOOLEAN
KiProcessorFreezeHandler(
    _In_ PKTRAP_FRAME TrapFrame,
    _In_ PKEXCEPTION_FRAME ExceptionFrame)
{
    PKPRCB CurrentPrcb = KeGetCurrentPrcb();
    KCONTINUE_STATUS ContinueStatus;

    if (CurrentPrcb->IpiFrozen != IPI_FROZEN_STATE_TARGET_FREEZE)
        return FALSE;

    CurrentPrcb->IpiFrozen = IPI_FROZEN_STATE_FROZEN;
    KiSaveProcessorState(TrapFrame, ExceptionFrame);

    while (CurrentPrcb->IpiFrozen != IPI_FROZEN_STATE_THAW)
    {
        /* The debugger switched to this processor */
        if (CurrentPrcb->IpiFrozen & IPI_FROZEN_FLAG_ACTIVE)
        {
            ContinueStatus = KdReportProcessorChange();
            CurrentPrcb->IpiFrozen = IPI_FROZEN_STATE_FROZEN;
            if (ContinueStatus == ContinueSuccess)
                KiFreezeOwner->IpiFrozen = IPI_FROZEN_STATE_THAW;
        }

        YieldProcessor();
        KeMemoryBarrier();
    }

    KiRestoreProcessorState(TrapFrame, ExceptionFrame);
    CurrentPrcb->IpiFrozen = IPI_FROZEN_STATE_RUNNING;
    return TRUE;
}

VOID
NTAPI
KxFreezeExecution(VOID)
{
    PKPRCB CurrentPrcb = KeGetCurrentPrcb();
    PKPRCB TargetPrcb;
    ULONG i;

    if (KiFreezeOwner == CurrentPrcb)
        return;

    while (InterlockedCompareExchangePointer((PVOID *)&KiFreezeOwner, CurrentPrcb, NULL))
    {
        /* Another processor owns the freeze and will stop us shortly */
        while (KiFreezeOwner != NULL)
        {
            YieldProcessor();
            KeMemoryBarrier();
        }
    }

    CurrentPrcb->IpiFrozen = IPI_FROZEN_STATE_OWNER | IPI_FROZEN_FLAG_ACTIVE;

    for (i = 0; i < (ULONG)KeNumberProcessors; i++)
    {
        TargetPrcb = KiProcessorBlock[i];
        if (TargetPrcb != CurrentPrcb)
            TargetPrcb->IpiFrozen = IPI_FROZEN_STATE_TARGET_FREEZE;
    }

    HalRequestIpi(KeActiveProcessors & ~CurrentPrcb->SetMember);

    for (i = 0; i < (ULONG)KeNumberProcessors; i++)
    {
        TargetPrcb = KiProcessorBlock[i];
        if (TargetPrcb == CurrentPrcb)
            continue;

        while (TargetPrcb->IpiFrozen != IPI_FROZEN_STATE_FROZEN)
        {
            YieldProcessor();
            KeMemoryBarrier();
        }
    }
}

VOID
NTAPI
KxThawExecution(VOID)
{
    PKPRCB CurrentPrcb = KeGetCurrentPrcb();
    PKPRCB TargetPrcb;
    ULONG i;

    ASSERT(CurrentPrcb->IpiFrozen & IPI_FROZEN_FLAG_ACTIVE);

    for (i = 0; i < (ULONG)KeNumberProcessors; i++)
    {
        TargetPrcb = KiProcessorBlock[i];
        if (TargetPrcb != CurrentPrcb)
            TargetPrcb->IpiFrozen = IPI_FROZEN_STATE_THAW;
    }

    for (i = 0; i < (ULONG)KeNumberProcessors; i++)
    {
        TargetPrcb = KiProcessorBlock[i];
        if (TargetPrcb == CurrentPrcb)
            continue;

        while (TargetPrcb->IpiFrozen != IPI_FROZEN_STATE_RUNNING)
        {
            YieldProcessor();
            KeMemoryBarrier();
        }
    }

    CurrentPrcb->IpiFrozen = IPI_FROZEN_STATE_RUNNING;
    InterlockedExchangePointer((PVOID *)&KiFreezeOwner, NULL);
}

KCONTINUE_STATUS
NTAPI
KxSwitchKdProcessor(
    _In_ ULONG ProcessorIndex)
{
    PKPRCB CurrentPrcb = KeGetCurrentPrcb();
    PKPRCB TargetPrcb;

    ASSERT(ProcessorIndex < (ULONG)KeNumberProcessors);

    ASSERT(CurrentPrcb->IpiFrozen & IPI_FROZEN_FLAG_ACTIVE);
    CurrentPrcb->IpiFrozen &= ~IPI_FROZEN_FLAG_ACTIVE;

    TargetPrcb = KiProcessorBlock[ProcessorIndex];
    TargetPrcb->IpiFrozen |= IPI_FROZEN_FLAG_ACTIVE;

    /* A frozen processor goes back to its freeze loop */
    if (KiFreezeOwner != CurrentPrcb)
        return ContinueNextProcessor;

    while (CurrentPrcb->IpiFrozen == IPI_FROZEN_STATE_OWNER)
    {
        YieldProcessor();
        KeMemoryBarrier();
    }

    if (CurrentPrcb->IpiFrozen == IPI_FROZEN_STATE_THAW)
    {
        CurrentPrcb->IpiFrozen = IPI_FROZEN_STATE_OWNER | IPI_FROZEN_FLAG_ACTIVE;
        return ContinueSuccess;
    }

    ASSERT(CurrentPrcb->IpiFrozen == (IPI_FROZEN_STATE_OWNER | IPI_FROZEN_FLAG_ACTIVE));
    return ContinueProcessorReselected;
}
