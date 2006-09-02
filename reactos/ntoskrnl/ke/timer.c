/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/timer.c
 * PURPOSE:         Handle Kernel Timers (Kernel-part of Executive Timers)
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  David Welch (welch@mcmail.com)
 */

/* INCLUDES ***************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ****************************************************************/

LARGE_INTEGER KiTimeIncrementReciprocal;
UCHAR KiTimeIncrementShiftCount;
LIST_ENTRY KiTimerListHead;
KTIMER_TABLE_ENTRY KiTimerTableListHead[TIMER_TABLE_SIZE];
#define SYSTEM_TIME_UNITS_PER_MSEC (10000)

/* PRIVATE FUNCTIONS ******************************************************/

VOID
NTAPI
KiRemoveTimer(IN PKTIMER Timer)
{
    /* Remove the timer */
    Timer->Header.Inserted = FALSE;
    RemoveEntryList(&Timer->TimerListEntry);
}

/*
 * Note: This function is called with the Dispatcher Lock held.
 */
BOOLEAN
NTAPI
KiInsertTimer(IN PKTIMER Timer,
              IN LARGE_INTEGER DueTime)
{
    LARGE_INTEGER SystemTime;
    LARGE_INTEGER DifferenceTime;
    ULONGLONG InterruptTime;

    /* Set default data */
    Timer->Header.Inserted = TRUE;
    Timer->Header.Absolute = FALSE;
    if (!Timer->Period) Timer->Header.SignalState = FALSE;

    /* Convert to relative time if needed */
    if (DueTime.HighPart >= 0)
    {
        /* Get System Time */
        KeQuerySystemTime(&SystemTime);

        /* Do the conversion */
        DifferenceTime.QuadPart = SystemTime.QuadPart - DueTime.QuadPart;

        /* Make sure it hasn't already expired */
        if (DifferenceTime.HighPart >= 0)
        {
            /* Cancel everything */
            Timer->Header.SignalState = TRUE;
            Timer->Header.Inserted = FALSE;
            return FALSE;
        }

        /* Set the time as Absolute */
        Timer->Header.Absolute = TRUE;
        DueTime = DifferenceTime;
    }

    /* Get the Interrupt Time */
    InterruptTime = KeQueryInterruptTime();

    /* Set the Final Due Time */
    Timer->DueTime.QuadPart = InterruptTime - DueTime.QuadPart;

    /* Now insert it into the Timer List */
    InsertAscendingList(&KiTimerListHead,
                        Timer,
                        KTIMER,
                        TimerListEntry,
                        DueTime.QuadPart);
    return TRUE;
}

/*
 * We enter this function at IRQL DISPATCH_LEVEL, and with the
 * Dispatcher Lock held!
 */
VOID
NTAPI
KiHandleExpiredTimer(IN PKTIMER Timer)
{
    LARGE_INTEGER DueTime;

    /* Set it as Signaled */
    Timer->Header.SignalState = TRUE;

    /* Check if it has any waiters */
    if (!IsListEmpty(&Timer->Header.WaitListHead))
    {
        /* Wake them */
        KiWaitTest(Timer, IO_NO_INCREMENT);
    }

    /* If the Timer is periodic, reinsert the timer with the new due time */
    if (Timer->Period)
    {
        /* Reinsert the Timer */
        DueTime.QuadPart = Timer->Period * -SYSTEM_TIME_UNITS_PER_MSEC;
        while (!KiInsertTimer(Timer, DueTime));
    }

    /* Check if the Timer has a DPC */
    if (Timer->Dpc)
    {
        /* Insert the DPC */
        KeInsertQueueDpc(Timer->Dpc,
                         NULL,
                         NULL);
    }
}

VOID
NTAPI
KiExpireTimers(IN PKDPC Dpc,
               IN PVOID DeferredContext,
               IN PVOID SystemArgument1,
               IN PVOID SystemArgument2)
{
    PKTIMER Timer;
    ULONGLONG InterruptTime;
    LIST_ENTRY ExpiredTimerList;
    PLIST_ENTRY ListHead, NextEntry;
    KIRQL OldIrql;

    /* Initialize the Expired Timer List */
    InitializeListHead(&ExpiredTimerList);

    /* Lock the Database and Raise IRQL */
    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* Query Interrupt Times */
    InterruptTime = KeQueryInterruptTime();

    /* Loop through the Timer List */
    ListHead = &KiTimerListHead;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        /* Get the timer */
        Timer = CONTAINING_RECORD(NextEntry, KTIMER, TimerListEntry);

        /* Check if we have to Expire it */
        if (InterruptTime < Timer->DueTime.QuadPart) break;

        /* Remove it from the Timer List, add it to the Expired List */
        RemoveEntryList(&Timer->TimerListEntry);
        InsertTailList(&ExpiredTimerList, &Timer->TimerListEntry);
        NextEntry = ListHead->Flink;
    }

    /* Expire the Timers */
    while (ExpiredTimerList.Flink != &ExpiredTimerList)
    {
        /* Get the Timer */
        Timer = CONTAINING_RECORD(ExpiredTimerList.Flink,
                                  KTIMER,
                                  TimerListEntry);

        /* Remove it */
        ///
        // GCC IS A BRAINDEAD PIECE OF SHIT. WILL GIVE 5$ FOR EACH DEV KILLED.
        ///
        Timer->Header.Inserted = FALSE;
        RemoveEntryList(&Timer->TimerListEntry);
        //KiRemoveTimer(Timer);

        /* Expire it */
        KiHandleExpiredTimer(Timer);
    }

    /* Release Dispatcher Lock */
    KeReleaseDispatcherDatabaseLock(OldIrql);
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
BOOLEAN
NTAPI
KeCancelTimer(IN OUT PKTIMER Timer)
{
    KIRQL OldIrql;
    BOOLEAN Inserted;
    ASSERT_TIMER(Timer);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Lock the Database and Raise IRQL */
    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* Check if it's inserted, and remove it if it is */
    Inserted = Timer->Header.Inserted;
    if (Inserted)
    {
        ///
        // GCC IS A BRAINDEAD PIECE OF SHIT. WILL GIVE 5$ FOR EACH DEV KILLED.
        ///
        Timer->Header.Inserted = FALSE;
        RemoveEntryList(&Timer->TimerListEntry);
        //KiRemoveTimer(Timer);
    }

    /* Release Dispatcher Lock */
    KeReleaseDispatcherDatabaseLock(OldIrql);
    return Inserted;
}

/*
 * @implemented
 */
VOID
NTAPI
KeInitializeTimer(OUT PKTIMER Timer)
{
    /* Call the New Function */
    KeInitializeTimerEx(Timer, NotificationTimer);
}

/*
 * @implemented
 */
VOID
NTAPI
KeInitializeTimerEx(OUT PKTIMER Timer,
                    IN TIMER_TYPE Type)
{
    /* Initialize the Dispatch Header */
    KeInitializeDispatcherHeader(&Timer->Header,
                                 TimerNotificationObject + Type,
                                 sizeof(KTIMER) / sizeof(ULONG),
                                 FALSE);

    /* Initalize the Other data */
    Timer->DueTime.QuadPart = 0;
    Timer->Period = 0;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
KeReadStateTimer(IN PKTIMER Timer)
{
    /* Return the Signal State */
    ASSERT_TIMER(Timer);
    return (BOOLEAN)Timer->Header.SignalState;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
KeSetTimer(IN OUT PKTIMER Timer,
           IN LARGE_INTEGER DueTime,
           IN PKDPC Dpc OPTIONAL)
{
    /* Call the newer function and supply a period of 0 */
    return KeSetTimerEx(Timer, DueTime, 0, Dpc);
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
KeSetTimerEx(IN OUT PKTIMER Timer,
             IN LARGE_INTEGER DueTime,
             IN LONG Period,
             IN PKDPC Dpc OPTIONAL)
{
    KIRQL OldIrql;
    BOOLEAN Inserted;
    ASSERT_TIMER(Timer);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Lock the Database and Raise IRQL */
    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* Check if it's inserted, and remove it if it is */
    Inserted = Timer->Header.Inserted;
    if (Inserted)
    {
        ///
        // GCC IS A BRAINDEAD PIECE OF SHIT. WILL GIVE 5$ FOR EACH DEV KILLED.
        ///
        Timer->Header.Inserted = FALSE;
        RemoveEntryList(&Timer->TimerListEntry);
        //KiRemoveTimer(Timer);
    }

    /* Set Default Timer Data */
    Timer->Dpc = Dpc;
    Timer->Period = Period;
    Timer->Header.SignalState = FALSE;

    /* Insert it */
    if (!KiInsertTimer(Timer, DueTime))
    {
        /* Check if it has any waiters */
        if (!IsListEmpty(&Timer->Header.WaitListHead))
        {
            /* Wake them */
            KiWaitTest(Timer, IO_NO_INCREMENT);
        }

        /* Check if the Timer has a DPC */
        if (Dpc)
        {
            /* Insert the DPC */
            KeInsertQueueDpc(Timer->Dpc,
                             NULL,
                             NULL);
        }

        /* Check if the Timer is periodic */
        if (Timer->Period)
        {
            /* Reinsert the Timer */
            DueTime.QuadPart = Timer->Period * -SYSTEM_TIME_UNITS_PER_MSEC;
            while (!KiInsertTimer(Timer, DueTime));
        }
    }

    /* Release Dispatcher Lock */
    KeReleaseDispatcherDatabaseLock(OldIrql);

    /* Return old state */
    return Inserted;
}

/* EOF */
