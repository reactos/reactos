/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Processor freeze support for x64
 * COPYRIGHT:   Copyright 2023-2024 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

/*

 IpiFrozen state graph (based on Windows behavior):

    +-----------------+     Freeze request      +-----------------+
    | RUNNING         |------------------------>| TARGET_FREEZE   |
    +-----------------+<---------               +-----------------+
            |^                  | Resume                |
     Freeze || Thaw        +-----------+ Thaw request   | Freeze IPI
            v|             | THAW      |<-----------\   v
    +-----------------+    +-----------+        +-----------------+
    | OWNER + ACTIVE  |         ^               | FROZEN          |
    +-----------------+         |               +-----------------+
            ^                   |                       ^
            | Kd proc switch    |                       | Kd proc switch
            v                   |                       v
    +-----------------+         |               +-----------------+
    | OWNER           |---------+               | FROZEN + ACTIVE |
    +-----------------+ Thaw request            +-----------------+

 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* NOT INCLUDES ANYMORE ******************************************************/

PKPRCB KiFreezeOwner;

/* FUNCTIONS *****************************************************************/

BOOLEAN
KiProcessorFreezeHandler(
    _In_ PKTRAP_FRAME TrapFrame,
    _In_ PKEXCEPTION_FRAME ExceptionFrame)
{
    PKPRCB CurrentPrcb = KeGetCurrentPrcb();

    /* Make sure this is a freeze request */
    if (CurrentPrcb->IpiFrozen != IPI_FROZEN_STATE_TARGET_FREEZE)
    {
        /* Not a freeze request, return FALSE to signal it is unhandled */
        return FALSE;
    }

    /* We are frozen now */
    CurrentPrcb->IpiFrozen = IPI_FROZEN_STATE_FROZEN;

    /* Save the processor state */
    KiSaveProcessorState(TrapFrame, ExceptionFrame);

    /* Wait for the freeze owner to release us */
    while (CurrentPrcb->IpiFrozen != IPI_FROZEN_STATE_THAW)
    {
        /* Check for Kd processor switch */
        if (CurrentPrcb->IpiFrozen & IPI_FROZEN_FLAG_ACTIVE)
        {
            KCONTINUE_STATUS ContinueStatus;

            /* Enter the debugger */
            ContinueStatus = KdReportProcessorChange();

            /* Set the state back to frozen */
            CurrentPrcb->IpiFrozen = IPI_FROZEN_STATE_FROZEN;

            /* If the status is ContinueSuccess, we need to release the freeze owner */
            if (ContinueStatus == ContinueSuccess)
            {
                /* Release the freeze owner */
                KiFreezeOwner->IpiFrozen = IPI_FROZEN_STATE_THAW;
            }
        }

        YieldProcessor();
        KeMemoryBarrier();
    }

    /* Restore the processor state */
    KiRestoreProcessorState(TrapFrame, ExceptionFrame);

    /* We are running again now */
    CurrentPrcb->IpiFrozen = IPI_FROZEN_STATE_RUNNING;

    /* Return TRUE to signal that we handled the freeze */
    return TRUE;
}

VOID
NTAPI
KxFreezeExecution(
    VOID)
{
    PKPRCB CurrentPrcb = KeGetCurrentPrcb();

    /* Avoid blocking on recursive debug action */
    if (KiFreezeOwner == CurrentPrcb)
    {
        return;
    }

    /* Try to acquire the freeze owner */
    while (InterlockedCompareExchangePointer(&KiFreezeOwner, CurrentPrcb, NULL))
    {
        /* Someone else was faster. We expect an NMI to freeze any time.
           Spin here until the freeze owner is available. */
        while (KiFreezeOwner != NULL)
        {
            YieldProcessor();
            KeMemoryBarrier();
        }
    }

    /* We are the owner now and active */
    CurrentPrcb->IpiFrozen = IPI_FROZEN_STATE_OWNER | IPI_FROZEN_FLAG_ACTIVE;

    /* Loop all processors */
    for (ULONG i = 0; i < KeNumberProcessors; i++)
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
    for (ULONG i = 0; i < KeNumberProcessors; i++)
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
}

VOID
NTAPI
KxThawExecution(
    VOID)
{
    PKPRCB CurrentPrcb = KeGetCurrentPrcb();
    ASSERT(CurrentPrcb->IpiFrozen & IPI_FROZEN_FLAG_ACTIVE);

    /* Loop all processors */
    for (ULONG i = 0; i < KeNumberProcessors; i++)
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
    for (ULONG i = 0; i < KeNumberProcessors; i++)
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
    InterlockedExchangePointer(&KiFreezeOwner, NULL);
}

KCONTINUE_STATUS
NTAPI
KxSwitchKdProcessor(
    _In_ ULONG ProcessorIndex)
{
    PKPRCB CurrentPrcb = KeGetCurrentPrcb();
    PKPRCB TargetPrcb;

    /* Make sure that the processor index is valid */
    ASSERT(ProcessorIndex < KeNumberProcessors);

    /* We are no longer active */
    ASSERT(CurrentPrcb->IpiFrozen & IPI_FROZEN_FLAG_ACTIVE);
    CurrentPrcb->IpiFrozen &= ~IPI_FROZEN_FLAG_ACTIVE;

    /* Inform the target processor that it's his turn now */
    TargetPrcb = KiProcessorBlock[ProcessorIndex];
    TargetPrcb->IpiFrozen |= IPI_FROZEN_FLAG_ACTIVE;

    /* If we are not the freeze owner, we return back to the freeze loop */
    if (KiFreezeOwner != CurrentPrcb)
    {
        return ContinueNextProcessor;
    }

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
        KdpDprintf("[%u] KxSwitchKdProcessor: ContinueSuccess\n", KeGetCurrentProcessorNumber());
        CurrentPrcb->IpiFrozen = IPI_FROZEN_STATE_OWNER | IPI_FROZEN_FLAG_ACTIVE;
        return ContinueSuccess;
    }

    /* We have been reselected, return to Kd to continue in the debugger */
    ASSERT(CurrentPrcb->IpiFrozen == (IPI_FROZEN_STATE_OWNER | IPI_FROZEN_FLAG_ACTIVE));

    return ContinueProcessorReselected;
}
