/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/timer.c
 * PURPOSE:         Handle Kernel Timers (Kernel-part of Executive Timers)
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

KTIMER_TABLE_ENTRY KiTimerTableListHead[TIMER_TABLE_SIZE];
LARGE_INTEGER KiTimeIncrementReciprocal;
UCHAR KiTimeIncrementShiftCount;
BOOLEAN KiEnableTimerWatchdog;

/* PRIVATE FUNCTIONS *********************************************************/

BOOLEAN
FASTCALL
KiInsertTreeTimer(IN PKTIMER Timer,
                  IN LARGE_INTEGER Interval)
{
    BOOLEAN Inserted = FALSE;
    ULONG Hand = 0;
    PKSPIN_LOCK_QUEUE LockQueue;
    DPRINT("KiInsertTreeTimer(): Timer %p, Interval: %I64d\n", Timer, Interval.QuadPart);

    /* Setup the timer's due time */
    if (KiComputeDueTime(Timer, Interval, &Hand))
    {
        /* Acquire the lock */
        LockQueue = KiAcquireTimerLock(Hand);

        /* Insert the timer */
        if (KiInsertTimerTable(Timer, Hand))
        {
            /* It was already there, remove it */
            KiRemoveEntryTimer(Timer);
            Timer->Header.Inserted = FALSE;
        }
        else
        {
            /* Otherwise, we're now inserted */
            Inserted = TRUE;
        }
        
        /* Release the lock */
        KiReleaseTimerLock(LockQueue);
    }

    /* Release the lock and return insert status */
    return Inserted;
}

BOOLEAN
FASTCALL
KiInsertTimerTable(IN PKTIMER Timer,
                   IN ULONG Hand)
{
    LARGE_INTEGER InterruptTime;
    LONGLONG DueTime = Timer->DueTime.QuadPart;
    BOOLEAN Expired = FALSE;
    PLIST_ENTRY ListHead, NextEntry;
    PKTIMER CurrentTimer;
    DPRINT("KiInsertTimerTable(): Timer %p, Hand: %d\n", Timer, Hand);

    /* Check if the period is zero */
    if (!Timer->Period) Timer->Header.SignalState = FALSE;

    /* Sanity check */
    ASSERT(Hand == KiComputeTimerTableIndex(DueTime));

    /* Loop the timer list backwards */
    ListHead = &KiTimerTableListHead[Hand].Entry;
    NextEntry = ListHead->Blink;
    while (NextEntry != ListHead)
    {
        /* Get the timer */
        CurrentTimer = CONTAINING_RECORD(NextEntry, KTIMER, TimerListEntry);

        /* Now check if we can fit it before */
        if ((ULONGLONG)DueTime >= CurrentTimer->DueTime.QuadPart) break;

        /* Keep looping */
        NextEntry = NextEntry->Blink;
    }

    /* Looped all the list, insert it here and get the interrupt time again */
    InsertHeadList(NextEntry, &Timer->TimerListEntry);

    /* Check if we didn't find it in the list */
    if (NextEntry == ListHead)
    {
        /* Set the time */
        KiTimerTableListHead[Hand].Time.QuadPart = DueTime;

        /* Make sure it hasn't expired already */
        InterruptTime.QuadPart = KeQueryInterruptTime();
        if (DueTime <= InterruptTime.QuadPart) Expired = TRUE;
    }

    /* Return expired state */
    return Expired;
}

BOOLEAN
FASTCALL
KiSignalTimer(IN PKTIMER Timer)
{
    BOOLEAN RequestInterrupt = FALSE;
    PKDPC Dpc = Timer->Dpc;
    ULONG Period = Timer->Period;
    LARGE_INTEGER Interval, SystemTime;
    DPRINT("KiSignalTimer(): Timer %p\n", Timer);

    /* Set default values */
    Timer->Header.Inserted = FALSE;
    Timer->Header.SignalState = TRUE;

    /* Check if the timer has waiters */
    if (!IsListEmpty(&Timer->Header.WaitListHead))
    {
        /* Check the type of event */
        if (Timer->Header.Type == TimerNotificationObject)
        {
            /* Unwait the thread */
            KxUnwaitThread(&Timer->Header, IO_NO_INCREMENT);
        }
        else
        {
            /* Otherwise unwait the thread and signal the timer */
            KxUnwaitThreadForEvent((PKEVENT)Timer, IO_NO_INCREMENT);
        }
    }

    /* Check if we have a period */
    if (Period)
    {
        /* Calculate the interval and insert the timer */
        Interval.QuadPart = Int32x32To64(Period, -10000);
        while (!KiInsertTreeTimer(Timer, Interval));
    }

    /* Check if we have a DPC */
    if (Dpc)
    {
        /* Insert it in the queue */
        KeQuerySystemTime(&SystemTime);
        KeInsertQueueDpc(Dpc,
                         ULongToPtr(SystemTime.LowPart),
                         ULongToPtr(SystemTime.HighPart));
        RequestInterrupt = TRUE;
    }

    /* Return whether we need to request a DPC interrupt or not */
    return RequestInterrupt;
}

VOID
FASTCALL
KiCompleteTimer(IN PKTIMER Timer,
                IN PKSPIN_LOCK_QUEUE LockQueue)
{
    LIST_ENTRY ListHead;
    BOOLEAN RequestInterrupt = FALSE;
    DPRINT("KiCompleteTimer(): Timer %p, LockQueue: %p\n", Timer, LockQueue);

    /* Remove it from the timer list */
    KiRemoveEntryTimer(Timer);

    /* Link the timer list to our stack */
    ListHead.Flink = &Timer->TimerListEntry;
    ListHead.Blink = &Timer->TimerListEntry;
    Timer->TimerListEntry.Flink = &ListHead;
    Timer->TimerListEntry.Blink = &ListHead;

    /* Release the timer lock */
    KiReleaseTimerLock(LockQueue);

    /* Acquire dispatcher lock */
    KiAcquireDispatcherLockAtDpcLevel();

    /* Signal the timer if it's still on our list */
    if (!IsListEmpty(&ListHead)) RequestInterrupt = KiSignalTimer(Timer);

    /* Release the dispatcher lock */
    KiReleaseDispatcherLockFromDpcLevel();

    /* Request a DPC if needed */
    if (RequestInterrupt) HalRequestSoftwareInterrupt(DISPATCH_LEVEL);
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
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
    DPRINT("KeCancelTimer(): Timer %p\n", Timer);

    /* Lock the Database and Raise IRQL */
    OldIrql = KiAcquireDispatcherLock();

    /* Check if it's inserted, and remove it if it is */
    Inserted = Timer->Header.Inserted;
    if (Inserted) KxRemoveTreeTimer(Timer);

    /* Release Dispatcher Lock */
    KiReleaseDispatcherLock(OldIrql);

    /* Return the old state */
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
    DPRINT("KeInitializeTimerEx(): Timer %p, Type %s\n",
            Timer, (Type == NotificationTimer) ?
           "NotificationTimer" : "SynchronizationTimer");

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
    ULONG Hand = 0;
    BOOLEAN RequestInterrupt = FALSE;
    ASSERT_TIMER(Timer);
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
    DPRINT("KeSetTimerEx(): Timer %p, DueTime %I64d, Period %d, Dpc %p\n",
           Timer, DueTime.QuadPart, Period, Dpc);

    /* Lock the Database and Raise IRQL */
    OldIrql = KiAcquireDispatcherLock();

    /* Check if it's inserted, and remove it if it is */
    Inserted = Timer->Header.Inserted;
    if (Inserted) KxRemoveTreeTimer(Timer);

    /* Set Default Timer Data */
    Timer->Dpc = Dpc;
    Timer->Period = Period;
    if (!KiComputeDueTime(Timer, DueTime, &Hand))
    {
        /* Signal the timer */
        RequestInterrupt = KiSignalTimer(Timer);
        
        /* Release the dispatcher lock */
        KiReleaseDispatcherLockFromDpcLevel();
        
        /* Check if we need to do an interrupt */
        if (RequestInterrupt) HalRequestSoftwareInterrupt(DISPATCH_LEVEL);        
    }
    else
    {
        /* Insert the timer */
        Timer->Header.SignalState = FALSE;
        KxInsertTimer(Timer, Hand);        
    }
    
    /* Exit the dispatcher */
    KiExitDispatcher(OldIrql);

    /* Return old state */
    return Inserted;
}

