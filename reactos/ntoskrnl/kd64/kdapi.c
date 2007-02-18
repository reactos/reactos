/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/kd64/kdapi.c
 * PURPOSE:         KD64 Public Routines and Internal Support
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

VOID
NTAPI
KdpTimeSlipDpcRoutine(IN PKDPC Dpc,
                      IN PVOID DeferredContext,
                      IN PVOID SystemArgument1,
                      IN PVOID SystemArgument2)
{
    LONG OldSlip, NewSlip, PendingSlip;

    /* Get the current pending slip */
    PendingSlip = KdpTimeSlipPending;
    do
    {
        /* Save the old value and either disable or enable it now. */
        OldSlip = PendingSlip;
        NewSlip = OldSlip > 1 ? 1 : 0;

        /* Try to change the value */
    } while (InterlockedCompareExchange(&KdpTimeSlipPending,
                                        NewSlip,
                                        OldSlip) != OldSlip);

    /* If the New Slip value is 1, then do the Time Slipping */
    if (NewSlip) ExQueueWorkItem(&KdpTimeSlipWorkItem, DelayedWorkQueue);
}

VOID
NTAPI
KdpTimeSlipWork(IN PVOID Context)
{
    KIRQL OldIrql;
    LARGE_INTEGER DueTime;

    /* Update the System time from the CMOS */
    ExAcquireTimeRefreshLock(FALSE);
    ExUpdateSystemTimeFromCmos(FALSE, 0);
    ExReleaseTimeRefreshLock();

    /* Check if we have a registered Time Slip Event and signal it */
    KeAcquireSpinLock(&KdpTimeSlipEventLock, &OldIrql);
    if (KdpTimeSlipEvent) KeSetEvent(KdpTimeSlipEvent, 0, FALSE);
    KeReleaseSpinLock(&KdpTimeSlipEventLock, OldIrql);

    /* Delay the DPC until it runs next time */
    DueTime.QuadPart = -1800000000;
    KeSetTimer(&KdpTimeSlipTimer, DueTime, &KdpTimeSlipDpc);
}

BOOLEAN
NTAPI
KdpSwitchProcessor(IN PEXCEPTION_RECORD ExceptionRecord,
                   IN OUT PCONTEXT ContextRecord,
                   IN BOOLEAN SecondChanceException)
{
    BOOLEAN Status;

    /* Save the port data */
    KdSave(FALSE);

    /* Report a state change */
#if 0
    Status = KdpReportExceptionStateChange(ExceptionRecord,
                                           ContextRecord,
                                           SecondChanceException);
#else
    Status = FALSE;
#endif

    /* Restore the port data and return */
    KdRestore(FALSE);
    return Status;
}

LARGE_INTEGER
NTAPI
KdpQueryPerformanceCounter(IN PKTRAP_FRAME TrapFrame)
{
    LARGE_INTEGER Null = {{0}};

    /* Check if interrupts were disabled */
    if (!(TrapFrame->EFlags & EFLAGS_INTERRUPT_MASK))
    {
        /* Nothing to return */
        return Null;
    }

    /* Otherwise, do the call */
    return KeQueryPerformanceCounter(NULL);
}

BOOLEAN
NTAPI
KdEnterDebugger(IN PKTRAP_FRAME TrapFrame,
                IN PKEXCEPTION_FRAME ExceptionFrame)
{
    BOOLEAN Entered;

    /* Check if we have a trap frame */
    if (TrapFrame)
    {
        /* Calculate the time difference for the enter */
        KdTimerStop = KdpQueryPerformanceCounter(TrapFrame);
        KdTimerDifference.QuadPart = KdTimerStop.QuadPart -
                                     KdTimerStart.QuadPart;
    }
    else
    {
        /* No trap frame, so can't calculate */
        KdTimerStop.QuadPart = 0;
    }

    /* Save the current IRQL */
    KeGetCurrentPrcb()->DebuggerSavedIRQL = KeGetCurrentIrql();

    /* Freeze all CPUs */
    Entered = KeFreezeExecution(TrapFrame, ExceptionFrame);

    /* Lock the port, save the state and set debugger entered */
    KdpPortLocked = KeTryToAcquireSpinLockAtDpcLevel(&KdpDebuggerLock);
    KdSave(FALSE);
    KdEnteredDebugger = TRUE;

    /* Check freeze flag */
    if (KiFreezeFlag & 1)
    {
        /* Print out errror */
        DbgPrint("FreezeLock was jammed!  Backup SpinLock was used!\n");
    }

    /* Check processor state */
    if (KiFreezeFlag & 2)
    {
        /* Print out errror */
        DbgPrint("Some processors not frozen in debugger!\n");
    }

    /* Make sure we acquired the port */
    if (!KdpPortLocked) DbgPrint("Port lock was not acquired!\n");

    /* Return enter state */
    return Entered;
}

VOID
NTAPI
KdExitDebugger(IN BOOLEAN Entered)
{
    ULONG TimeSlip;

    /* Restore the state and unlock the port */
    KdRestore(FALSE);
    if (KdpPortLocked) KdpPortUnlock();

    /* Unfreeze the CPUs */
    KeThawExecution(Entered);

    /* Compare time with the one from KdEnterDebugger */
    if (!KdTimerStop.QuadPart)
    {
        /* We didn't get a trap frame earlier in so never got the time */
        KdTimerStart = KdTimerStop;
    }
    else
    {
        /* Query the timer */
        KdTimerStart = KeQueryPerformanceCounter(NULL);
    }

    /* Check if a Time Slip was on queue */
    TimeSlip = InterlockedIncrement(&KdpTimeSlipPending);
    if (TimeSlip == 1)
    {
        /* Queue a DPC for the time slip */
        InterlockedIncrement(&KdpTimeSlipPending);
        KeInsertQueueDpc(&KdpTimeSlipDpc, NULL, NULL);
    }
}

NTSTATUS
NTAPI
KdEnableDebuggerWithLock(BOOLEAN NeedLock)
{
    KIRQL OldIrql;

    /* Check if we need to acquire the lock */
    if (NeedLock)
    {
        /* Lock the port */
        KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
        KdpPortLock();
    }

    /* Check if we're not disabled */
    if (!KdDisableCount)
    {
        /* Check if we had locked the port before */
        if (NeedLock)
        {
            /* Do the unlock */
            KeLowerIrql(OldIrql);
            KdpPortUnlock();
        }

        /* Fail: We're already enabled */
        return STATUS_INVALID_PARAMETER;
    }

    /* Decrease the disable count */
    if (!(--KdDisableCount))
    {
        /* We're now enabled again! Were we enabled before, too? */
        if (KdPreviouslyEnabled)
        {
            /* Reinitialize the Debugger */
            KdInitSystem(0, NULL) ;
            //KdpRestoreAllBreakpoints();
        }
    }

    /* Check if we had locked the port before */
    if (NeedLock)
    {
        /* Yes, now unlock it */
        KeLowerIrql(OldIrql);
        KdpPortUnlock();
    }

    /* We're done */
    return STATUS_SUCCESS;
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
KdEnableDebugger(VOID)
{
    /* Use the internal routine */
    while (TRUE);
    return KdEnableDebuggerWithLock(TRUE);
}

