/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/event.c
 * PURPOSE:         Implements the Event Dispatcher Object
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
VOID
NTAPI
KeClearEvent(IN PKEVENT Event)
{
    ASSERT_EVENT(Event);

    /* Reset Signal State */
    Event->Header.SignalState = FALSE;
}

/*
 * @implemented
 */
VOID
NTAPI
KeInitializeEvent(IN PKEVENT Event,
                  IN EVENT_TYPE Type,
                  IN BOOLEAN State)
{
    /* Initialize the Dispatcher Header */
    KeInitializeDispatcherHeader(&Event->Header,
                                 Type,
                                 sizeof(*Event) / sizeof(ULONG),
                                 State);
}

/*
 * @implemented
 */
VOID
NTAPI
KeInitializeEventPair(IN PKEVENT_PAIR EventPair)
{
    /* Initialize the Event Pair Type and Size */
    EventPair->Type = EventPairObject;
    EventPair->Size = sizeof(KEVENT_PAIR);

    /* Initialize the two Events */
    KeInitializeEvent(&EventPair->LowEvent, SynchronizationEvent, FALSE);
    KeInitializeEvent(&EventPair->HighEvent, SynchronizationEvent, FALSE);
}

/*
 * @implemented
 */
LONG
NTAPI
KePulseEvent(IN PKEVENT Event,
             IN KPRIORITY Increment,
             IN BOOLEAN Wait)
{
    KIRQL OldIrql;
    LONG PreviousState;
    PKTHREAD Thread;
    ASSERT_EVENT(Event);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Lock the Dispatcher Database */
    OldIrql = KiAcquireDispatcherLock();

    /* Save the Old State */
    PreviousState = Event->Header.SignalState;

    /* Check if we are non-signaled and we have stuff in the Wait Queue */
    if (!PreviousState && !IsListEmpty(&Event->Header.WaitListHead))
    {
        /* Set the Event to Signaled */
        Event->Header.SignalState = 1;

        /* Wake the Event */
        KiWaitTest(&Event->Header, Increment);
    }

    /* Unsignal it */
    Event->Header.SignalState = 0;

    /* Check what wait state was requested */
    if (Wait == FALSE)
    {
        /* Wait not requested, release Dispatcher Database and return */
        KiReleaseDispatcherLock(OldIrql);
    }
    else
    {
        /* Return Locked and with a Wait */
        Thread = KeGetCurrentThread();
        Thread->WaitNext = TRUE;
        Thread->WaitIrql = OldIrql;
    }

    /* Return the previous State */
    return PreviousState;
}

/*
 * @implemented
 */
LONG
NTAPI
KeReadStateEvent(IN PKEVENT Event)
{
    ASSERT_EVENT(Event);

    /* Return the Signal State */
    return Event->Header.SignalState;
}

/*
 * @implemented
 */
LONG
NTAPI
KeResetEvent(IN PKEVENT Event)
{
    KIRQL OldIrql;
    LONG PreviousState;
    ASSERT_EVENT(Event);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Lock the Dispatcher Database */
    OldIrql = KiAcquireDispatcherLock();

    /* Save the Previous State */
    PreviousState = Event->Header.SignalState;

    /* Set it to zero */
    Event->Header.SignalState = 0;

    /* Release Dispatcher Database and return previous state */
    KiReleaseDispatcherLock(OldIrql);
    return PreviousState;
}

/*
 * @implemented
 */
LONG
NTAPI
KeSetEvent(IN PKEVENT Event,
           IN KPRIORITY Increment,
           IN BOOLEAN Wait)
{
    KIRQL OldIrql;
    LONG PreviousState;
    PKWAIT_BLOCK WaitBlock;
    PKTHREAD Thread;
    ASSERT_EVENT(Event);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /*
     * Check if this is an signaled notification event without an upcoming wait.
     * In this case, we can immediately return TRUE, without locking.
     */
    if ((Event->Header.Type == NotificationEvent) &&
        (Event->Header.SignalState == 1) &&
        !(Wait))
    {
        /* Return the signal state (TRUE/Signalled) */
        return TRUE;
    }

    /* Lock the Dispathcer Database */
    OldIrql = KiAcquireDispatcherLock();

    /* Save the Previous State */
    PreviousState = Event->Header.SignalState;

    /* Set the Event to Signaled */
    Event->Header.SignalState = 1;

    /* Check if the event just became signaled now, and it has waiters */
    if (!(PreviousState) && !(IsListEmpty(&Event->Header.WaitListHead)))
    {
        /* Get the Wait Block */
        WaitBlock = CONTAINING_RECORD(Event->Header.WaitListHead.Flink,
                                      KWAIT_BLOCK,
                                      WaitListEntry);

        /* Check the type of event */
        if (Event->Header.Type == NotificationEvent)
        {
            /* Unwait the thread */
            KxUnwaitThread(&Event->Header, Increment);
        }
        else
        {
            /* Otherwise unwait the thread and unsignal the event */
            KxUnwaitThreadForEvent(Event, Increment);
        }
    }

    /* Check what wait state was requested */
    if (!Wait)
    {
        /* Wait not requested, release Dispatcher Database and return */
        KiReleaseDispatcherLock(OldIrql);
    }
    else
    {
        /* Return Locked and with a Wait */
        Thread = KeGetCurrentThread();
        Thread->WaitNext = TRUE;
        Thread->WaitIrql = OldIrql;
    }

    /* Return the previous State */
    return PreviousState;
}

/*
 * @implemented
 */
VOID
NTAPI
KeSetEventBoostPriority(IN PKEVENT Event,
                        IN PKTHREAD *WaitingThread OPTIONAL)
{
    KIRQL OldIrql;
    PKWAIT_BLOCK WaitBlock;
    PKTHREAD Thread = KeGetCurrentThread(), WaitThread;
    ASSERT(Event->Header.Type == SynchronizationEvent);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Acquire Dispatcher Database Lock */
    OldIrql = KiAcquireDispatcherLock();

    /* Check if the list is empty */
    if (IsListEmpty(&Event->Header.WaitListHead))
    {
        /* Set the Event to Signaled */
        Event->Header.SignalState = 1;

        /* Return */
        KiReleaseDispatcherLock(OldIrql);
        return;
    }

    /* Get the Wait Block */
    WaitBlock = CONTAINING_RECORD(Event->Header.WaitListHead.Flink,
                                  KWAIT_BLOCK,
                                  WaitListEntry);

    /* Check if this is a WaitAll */
    if (WaitBlock->WaitType == WaitAll)
    {
        /* Set the Event to Signaled */
        Event->Header.SignalState = 1;

        /* Unwait the thread and unsignal the event */
        KxUnwaitThreadForEvent(Event, EVENT_INCREMENT);
    }
    else
    {
        /* Return waiting thread to caller */
        WaitThread = WaitBlock->Thread;
        if (WaitingThread) *WaitingThread = WaitThread;

        /* Calculate new priority */
        Thread->Priority = KiComputeNewPriority(Thread, 0);

        /* Unlink the waiting thread */
        KiUnlinkThread(WaitThread, STATUS_SUCCESS);

        /* Request priority boosting */
        WaitThread->AdjustIncrement = Thread->Priority;
        WaitThread->AdjustReason = AdjustBoost;

        /* Ready the thread */
        KiReadyThread(WaitThread);
    }

    /* Release the Dispatcher Database Lock */
    KiReleaseDispatcherLock(OldIrql);
}

/* EOF */
