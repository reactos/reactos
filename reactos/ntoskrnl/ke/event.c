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
#include <internal/debug.h>

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

    /* Lock the Dispathcer Database */
    OldIrql = KiAcquireDispatcherLock();

    /* Save the Previous State */
    PreviousState = Event->Header.SignalState;

    /* Check if we have stuff in the Wait Queue */
    if (IsListEmpty(&Event->Header.WaitListHead))
    {
        /* Set the Event to Signaled */
        Event->Header.SignalState = 1;
    }
    else
    {
        /* Get the Wait Block */
        WaitBlock = CONTAINING_RECORD(Event->Header.WaitListHead.Flink,
                                      KWAIT_BLOCK,
                                      WaitListEntry);

        /* Check the type of event */
        if ((Event->Header.Type == NotificationEvent) || (WaitBlock->WaitType == WaitAll))
        {
            /* Check if it wasn't signaled */
            if (!PreviousState)
            {
                /* We must do a full wait satisfaction */
                Event->Header.SignalState = 1;
                KiWaitTest(&Event->Header, Increment);
            }
        }
        else
        {
            /* We can satisfy wait simply by waking the thread */
            KiAbortWaitThread(WaitBlock->Thread,
                              WaitBlock->WaitKey,
                              Increment);
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
                        IN PKTHREAD *Thread OPTIONAL)
{
    PKTHREAD WaitingThread;
    KIRQL OldIrql;
    ASSERT_EVENT(Event);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    //
    // FIXME: This is half-broken, there's no boosting!
    //

    /* Acquire Dispatcher Database Lock */
    OldIrql = KiAcquireDispatcherLock();

    /* If our wait list is empty, then signal the event and return */
    if (IsListEmpty(&Event->Header.WaitListHead))
    {
        Event->Header.SignalState = 1;
    }
    else
    {
        /* Get the waiting thread */
        WaitingThread = CONTAINING_RECORD(Event->Header.WaitListHead.Flink,
                                          KWAIT_BLOCK,
                                          WaitListEntry)->Thread;

        /* Return it to caller if requested */
        if (Thread) *Thread = WaitingThread;

        /* Reset the Quantum and Unwait the Thread */
        WaitingThread->Quantum = WaitingThread->QuantumReset;
        KiAbortWaitThread(WaitingThread, STATUS_SUCCESS, EVENT_INCREMENT);
    }

    /* Release the Dispatcher Database Lock */
    KiReleaseDispatcherLock(OldIrql);
}

/* EOF */
