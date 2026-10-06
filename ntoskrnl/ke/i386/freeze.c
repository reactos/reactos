/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Processor freeze support for i386
 * COPYRIGHT:
 */

/* Same state machine as amd64/freeze.c, but the freeze is delivered as a regular IPI instead of an NMI */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

PKPRCB KiFreezeOwner;

/* FUNCTIONS ******************************************************************/

#ifdef CONFIG_SMP
static
VOID
KiFreezeWaitForThaw(
    _In_ PKPRCB Prcb)
{
    KCONTINUE_STATUS ContinueStatus;

    while (Prcb->IpiFrozen != IPI_FROZEN_STATE_THAW)
    {
        /* Check for Kd processor switch */
        if (Prcb->IpiFrozen & IPI_FROZEN_FLAG_ACTIVE)
        {
            ContinueStatus = KdReportProcessorChange();
            Prcb->IpiFrozen = IPI_FROZEN_STATE_FROZEN;

            /* Let the freeze owner leave the debugger */
            if (ContinueStatus == ContinueSuccess)
                KiFreezeOwner->IpiFrozen = IPI_FROZEN_STATE_THAW;
        }

        YieldProcessor();
        KeMemoryBarrier();
    }
}
#endif /* CONFIG_SMP */

BOOLEAN
KiProcessorFreezeHandler(
    _In_ PKTRAP_FRAME TrapFrame,
    _In_opt_ PKEXCEPTION_FRAME ExceptionFrame)
{
#ifdef CONFIG_SMP
    PKPRCB CurrentPrcb = KeGetCurrentPrcb();

    if (CurrentPrcb->IpiFrozen != IPI_FROZEN_STATE_TARGET_FREEZE)
        return FALSE;

    CurrentPrcb->IpiFrozen = IPI_FROZEN_STATE_FROZEN;

    /* Save the processor state so the debugger can show this processor */
    KiSaveProcessorState(TrapFrame, ExceptionFrame);

    KiFreezeWaitForThaw(CurrentPrcb);

    KiRestoreProcessorState(TrapFrame, ExceptionFrame);
    KeFlushCurrentTb();

    CurrentPrcb->IpiFrozen = IPI_FROZEN_STATE_RUNNING;
    return TRUE;
#else
    UNREFERENCED_PARAMETER(TrapFrame);
    UNREFERENCED_PARAMETER(ExceptionFrame);
    return FALSE;
#endif
}

VOID
NTAPI
KxFreezeExecution(
    VOID)
{
#ifdef CONFIG_SMP
    PKPRCB CurrentPrcb = KeGetCurrentPrcb();
    ULONG i;

    /* Avoid blocking on recursive debug action */
    if (KiFreezeOwner == CurrentPrcb)
        return;

    /* Try to become the freeze owner */
    while (InterlockedCompareExchangePointer((PVOID*)&KiFreezeOwner,
                                             CurrentPrcb,
                                             NULL) != NULL)
    {
        while (KiFreezeOwner != NULL)
        {
            /* Interrupts are disabled here, so answer the owner's freeze request directly */
            if (CurrentPrcb->IpiFrozen == IPI_FROZEN_STATE_TARGET_FREEZE)
            {
                CurrentPrcb->IpiFrozen = IPI_FROZEN_STATE_FROZEN;
                KiFreezeWaitForThaw(CurrentPrcb);
                KeFlushCurrentTb();
                CurrentPrcb->IpiFrozen = IPI_FROZEN_STATE_RUNNING;
            }

            YieldProcessor();
            KeMemoryBarrier();
        }
    }

    /* We are the owner now and active */
    CurrentPrcb->IpiFrozen = IPI_FROZEN_STATE_OWNER | IPI_FROZEN_FLAG_ACTIVE;

    /* Loop all processors */
    for (i = 0; i < (ULONG)KeNumberProcessors; i++)
    {
        PKPRCB TargetPrcb = KiProcessorBlock[i];
        if (TargetPrcb != CurrentPrcb)
        {
            /* Only the active processor is allowed to change IpiFrozen */
            ASSERT(TargetPrcb->IpiFrozen == IPI_FROZEN_STATE_RUNNING);

            /* Request target to freeze */
            TargetPrcb->IpiFrozen = IPI_FROZEN_STATE_TARGET_FREEZE;
        }
    }

    /* Send the freeze IPI */
    KiIpiSend(KeActiveProcessors & ~CurrentPrcb->SetMember, IPI_FREEZE);

    /* Wait for all targets to be frozen */
    for (i = 0; i < (ULONG)KeNumberProcessors; i++)
    {
        PKPRCB TargetPrcb = KiProcessorBlock[i];
        if (TargetPrcb != CurrentPrcb)
        {
            /* Wait for the target to be frozen */
            while (TargetPrcb->IpiFrozen != IPI_FROZEN_STATE_FROZEN)
            {
                YieldProcessor();
                KeMemoryBarrier();
            }
        }
    }

    /* All targets are frozen, we can continue */
#endif
}

VOID
NTAPI
KxThawExecution(
    VOID)
{
#ifdef CONFIG_SMP
    PKPRCB CurrentPrcb = KeGetCurrentPrcb();
    ULONG i;

    ASSERT(CurrentPrcb->IpiFrozen & IPI_FROZEN_FLAG_ACTIVE);

    /* Loop all processors */
    for (i = 0; i < (ULONG)KeNumberProcessors; i++)
    {
        PKPRCB TargetPrcb = KiProcessorBlock[i];
        if (TargetPrcb != CurrentPrcb)
        {
            /* Make sure they are still frozen */
            ASSERT(TargetPrcb->IpiFrozen == IPI_FROZEN_STATE_FROZEN);

            /* Request target to thaw */
            TargetPrcb->IpiFrozen = IPI_FROZEN_STATE_THAW;
        }
    }

    /* Wait for all targets to be running */
    for (i = 0; i < (ULONG)KeNumberProcessors; i++)
    {
        PKPRCB TargetPrcb = KiProcessorBlock[i];
        if (TargetPrcb != CurrentPrcb)
        {
            /* Wait for the target to be running again */
            while (TargetPrcb->IpiFrozen != IPI_FROZEN_STATE_RUNNING)
            {
                YieldProcessor();
                KeMemoryBarrier();
            }
        }
    }

    /* We are running again now */
    CurrentPrcb->IpiFrozen = IPI_FROZEN_STATE_RUNNING;

    /* Release the freeze owner */
    InterlockedExchangePointer((PVOID*)&KiFreezeOwner, NULL);
#endif
}

KCONTINUE_STATUS
NTAPI
KxSwitchKdProcessor(
    _In_ ULONG ProcessorIndex)
{
#ifdef CONFIG_SMP
    PKPRCB CurrentPrcb = KeGetCurrentPrcb();
    PKPRCB TargetPrcb;

    /* Make sure that the processor index is valid */
    ASSERT(ProcessorIndex < (ULONG)KeNumberProcessors);

    /* We are no longer active */
    ASSERT(CurrentPrcb->IpiFrozen & IPI_FROZEN_FLAG_ACTIVE);
    CurrentPrcb->IpiFrozen &= ~IPI_FROZEN_FLAG_ACTIVE;

    /* Inform the target processor that it is its turn now */
    TargetPrcb = KiProcessorBlock[ProcessorIndex];
    TargetPrcb->IpiFrozen |= IPI_FROZEN_FLAG_ACTIVE;

    /* If we are not the freeze owner, we return back to the freeze loop */
    if (KiFreezeOwner != CurrentPrcb)
        return ContinueNextProcessor;

    /* Loop until it's our turn again */
    while (CurrentPrcb->IpiFrozen == IPI_FROZEN_STATE_OWNER)
    {
        YieldProcessor();
        KeMemoryBarrier();
    }

    /* Check if we have been thawed */
    if (CurrentPrcb->IpiFrozen == IPI_FROZEN_STATE_THAW)
    {
        /* Another CPU has completed, we can leave the debugger now */
        CurrentPrcb->IpiFrozen = IPI_FROZEN_STATE_OWNER | IPI_FROZEN_FLAG_ACTIVE;
        return ContinueSuccess;
    }

    /* We have been reselected, return to Kd to continue in the debugger */
    ASSERT(CurrentPrcb->IpiFrozen == (IPI_FROZEN_STATE_OWNER | IPI_FROZEN_FLAG_ACTIVE));

    return ContinueProcessorReselected;
#else
    UNREFERENCED_PARAMETER(ProcessorIndex);
    return ContinueError;
#endif
}
