/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    eventobj.c

Abstract:

    This module implements the kernel event objects. Functions are
    provided to initialize, pulse, read, reset, and set event objects.

Author:

    David N. Cutler (davec) 27-Feb-1989

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"
#undef KeClearEvent

//
// The following assert macro is used to check that an input event is
// really a kernel event and not something else, like deallocated pool.
//

#define ASSERT_EVENT(E) {                             \
    ASSERT((E)->Header.Type == NotificationEvent ||   \
           (E)->Header.Type == SynchronizationEvent); \
}

//
// The following assert macro is used to check that an input event is
// really a kernel event pair and not something else, like deallocated
// pool.
//

#define ASSERT_EVENT_PAIR(E) {                        \
    ASSERT((E)->Type == EventPairObject);             \
}


#undef KeInitializeEvent

VOID
KeInitializeEvent (
    IN PRKEVENT Event,
    IN EVENT_TYPE Type,
    IN BOOLEAN State
    )

/*++

Routine Description:

    This function initializes a kernel event object. The initial signal
    state of the object is set to the specified value.

Arguments:

    Event - Supplies a pointer to a dispatcher object of type event.

    Type - Supplies the type of event; NotificationEvent or
        SynchronizationEvent.

    State - Supplies the initial signal state of the event object.

Return Value:

    None.

--*/

{

    //
    // Initialize standard dispatcher object header, set initial signal
    // state of event object, and set the type of event object.
    //

    Event->Header.Type = (UCHAR)Type;
    Event->Header.Size = sizeof(KEVENT) / sizeof(LONG);
    Event->Header.SignalState = State;
    InitializeListHead(&Event->Header.WaitListHead);
    return;
}

VOID
KeInitializeEventPair (
    IN PKEVENT_PAIR EventPair
    )

/*++

Routine Description:

    This function initializes a kernel event pair object. A kernel event
    pair object contains two separate synchronization event objects that
    are used to provide a fast interprocess synchronization capability.

Arguments:

    EventPair - Supplies a pointer to a control object of type event pair.

Return Value:

    None.

--*/

{

    //
    // Initialize the type and size of the event pair object and initialize
    // the two event object as synchronization events with an initial state
    // of FALSE.
    //

    EventPair->Type = (USHORT)EventPairObject;
    EventPair->Size = sizeof(KEVENT_PAIR);
    KeInitializeEvent(&EventPair->EventLow, SynchronizationEvent, FALSE);
    KeInitializeEvent(&EventPair->EventHigh, SynchronizationEvent, FALSE);
    return;
}

VOID
KeClearEvent (
    IN PRKEVENT Event
    )

/*++

Routine Description:

    This function clears the signal state of an event object.

Arguments:

    Event - Supplies a pointer to a dispatcher object of type event.

Return Value:

    None.

--*/

{

    ASSERT_EVENT(Event);

    //
    // Clear signal state of event object.
    //

    Event->Header.SignalState = 0;
    return;
}

LONG
KePulseEvent (
    IN PRKEVENT Event,
    IN KPRIORITY Increment,
    IN BOOLEAN Wait
    )

/*++

Routine Description:

    This function atomically sets the signal state of an event object to
    Signaled, attempts to satisfy as many Waits as possible, and then resets
    the signal state of the event object to Not-Signaled. The previous signal
    state of the event object is returned as the function value.

Arguments:

    Event - Supplies a pointer to a dispatcher object of type event.

    Increment - Supplies the priority increment that is to be applied
       if setting the event causes a Wait to be satisfied.

    Wait - Supplies a boolean value that signifies whether the call to
       KePulseEvent will be immediately followed by a call to one of the
       kernel Wait functions.

Return Value:

    The previous signal state of the event object.

--*/

{

    KIRQL OldIrql;
    LONG OldState;
    PRKTHREAD Thread;

    ASSERT_EVENT(Event);
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    //
    // Raise IRQL to dispatcher level and lock dispatcher database.
    //

    KiLockDispatcherDatabase(&OldIrql);

    //
    // If the current state of the event object is Not-Signaled and
    // the wait queue is not empty, then set the state of the event
    // to Signaled, satisfy as many Waits as possible, and then reset
    // the state of the event to Not-Signaled.
    //

    OldState = Event->Header.SignalState;
    if ((OldState == 0) && (IsListEmpty(&Event->Header.WaitListHead) == FALSE)) {
        Event->Header.SignalState = 1;
        KiWaitTest(Event, Increment);
    }

    Event->Header.SignalState = 0;

    //
    // If the value of the Wait argument is TRUE, then return to the
    // caller with IRQL raised and the dispatcher database locked. Else
    // release the dispatcher database lock and lower IRQL to the
    // previous value.
    //

    if (Wait != FALSE) {
        Thread = KeGetCurrentThread();
        Thread->WaitIrql = OldIrql;
        Thread->WaitNext = Wait;

    } else {
       KiUnlockDispatcherDatabase(OldIrql);
    }

    //
    // Return previous signal state of event object.
    //

    return OldState;
}

LONG
KeReadStateEvent (
    IN PRKEVENT Event
    )

/*++

Routine Description:

    This function reads the current signal state of an event object.

Arguments:

    Event - Supplies a pointer to a dispatcher object of type event.

Return Value:

    The current signal state of the event object.

--*/

{

    ASSERT_EVENT(Event);

    //
    // Return current signal state of event object.
    //

    return Event->Header.SignalState;
}

LONG
KeResetEvent (
    IN PRKEVENT Event
    )

/*++

Routine Description:

    This function resets the signal state of an event object to
    Not-Signaled. The previous state of the event object is returned
    as the function value.

Arguments:

    Event - Supplies a pointer to a dispatcher object of type event.

Return Value:

    The previous signal state of the event object.

--*/

{

    KIRQL OldIrql;
    LONG OldState;

    ASSERT_EVENT(Event);
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    //
    // Raise IRQL to dispatcher level and lock dispatcher database.
    //

    KiLockDispatcherDatabase(&OldIrql);

    //
    // Capture the current signal state of event object and then reset
    // the state of the event object to Not-Signaled.
    //

    OldState = Event->Header.SignalState;
    Event->Header.SignalState = 0;

    //
    // Unlock the dispatcher database and lower IRQL to its previous
    // value.

    KiUnlockDispatcherDatabase(OldIrql);

    //
    // Return previous signal state of event object.
    //

    return OldState;
}

LONG
KeSetEvent (
    IN PRKEVENT Event,
    IN KPRIORITY Increment,
    IN BOOLEAN Wait
    )

/*++

Routine Description:

    This function sets the signal state of an event object to Signaled
    and attempts to satisfy as many Waits as possible. The previous
    signal state of the event object is returned as the function value.

Arguments:

    Event - Supplies a pointer to a dispatcher object of type event.

    Increment - Supplies the priority increment that is to be applied
       if setting the event causes a Wait to be satisfied.

    Wait - Supplies a boolean value that signifies whether the call to
       KePulseEvent will be immediately followed by a call to one of the
       kernel Wait functions.

Return Value:

    The previous signal state of the event object.

--*/

{

    KIRQL OldIrql;
    LONG OldState;
    PRKTHREAD Thread;
    PRKWAIT_BLOCK WaitBlock;

    ASSERT_EVENT(Event);
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    //
    // Collect call data.
    //

#if defined(_COLLECT_SET_EVENT_CALLDATA_)

    RECORD_CALL_DATA(&KiSetEventCallData);

#endif

    //
    // Raise IRQL to dispatcher level and lock dispatcher database.
    //

    KiLockDispatcherDatabase(&OldIrql);

    //
    // If the wait list is empty, then set the state of the event to signaled.
    // Otherwise, check if the wait can be satisfied immediately.
    //

    OldState = Event->Header.SignalState;
    if (IsListEmpty(&Event->Header.WaitListHead) != FALSE) {
        Event->Header.SignalState = 1;

    } else {

        //
        // If the event is a notification event or the wait is not a wait any,
        // then set the state of the event to signaled and attempt to satisfy
        // as many waits as possible. Otherwise, the wait can be satisfied by
        // directly unwaiting the thread.
        //

        WaitBlock = CONTAINING_RECORD(Event->Header.WaitListHead.Flink,
                                      KWAIT_BLOCK,
                                      WaitListEntry);

        if ((Event->Header.Type == NotificationEvent) ||
            (WaitBlock->WaitType != WaitAny)) {
            if (OldState == 0) {
                Event->Header.SignalState = 1;
                KiWaitTest(Event, Increment);
            }

        } else {
            KiUnwaitThread(WaitBlock->Thread, (NTSTATUS)WaitBlock->WaitKey, Increment);
        }
    }

    //
    // If the value of the Wait argument is TRUE, then return to the
    // caller with IRQL raised and the dispatcher database locked. Else
    // release the dispatcher database lock and lower IRQL to its
    // previous value.
    //

    if (Wait != FALSE) {
       Thread = KeGetCurrentThread();
       Thread->WaitNext = Wait;
       Thread->WaitIrql = OldIrql;

    } else {
       KiUnlockDispatcherDatabase(OldIrql);
    }

    //
    // Return previous signal state of event object.
    //

    return OldState;
}

VOID
KeSetEventBoostPriority (
    IN PRKEVENT Event,
    IN PRKTHREAD *Thread OPTIONAL
    )

/*++

Routine Description:

    This function conditionally sets the signal state of an event object
    to Signaled, and attempts to unwait the first waiter, and optionally
    returns the thread address of the unwatied thread.

    N.B. This function can only be called with synchronization events
         and is primarily for the purpose of implementing fast mutexes.
         It is assumed that the waiter is NEVER waiting on multiple
         objects.

Arguments:

    Event - Supplies a pointer to a dispatcher object of type event.

    Thread - Supplies an optional pointer to a variable that receives
        the address of the thread that is awakened.

Return Value:

    None.

--*/

{

    KPRIORITY Increment;
    KIRQL OldIrql;
    PRKTHREAD WaitThread;

    ASSERT(Event->Header.Type == SynchronizationEvent);
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    //
    // Raise IRQL to dispatcher level and lock dispatcher database.
    //

    KiLockDispatcherDatabase(&OldIrql);

    //
    // If the the wait list is not empty, then satisfy the wait of the
    // first thread in the wait list. Otherwise, set the signal state
    // of the event object.
    //
    // N.B. This function is only called for fast mutexes and exclusive
    //      access to resources. All waits MUST be wait for single object.
    //

    if (IsListEmpty(&Event->Header.WaitListHead) != FALSE) {
        Event->Header.SignalState = 1;

    } else {

        //
        // Get the address of the waiting thread.
        //

        WaitThread = CONTAINING_RECORD(Event->Header.WaitListHead.Flink,
                                       KWAIT_BLOCK,
                                       WaitListEntry)->Thread;

        //
        // If specified, return the address of the thread that is awakened.
        //

        if (ARGUMENT_PRESENT(Thread)) {
            *Thread = WaitThread;
        }

        //
        // Give the new owner of the resource/fast mutex (the only callers) a
        // full quantum, and unwait the thread with a standard event increment
        // unless the system is a server system, in which case no boost if given.
        //

        WaitThread->Quantum = WaitThread->ApcState.Process->ThreadQuantum;
        Increment = 0;
        if (MmProductType == 0) {
            Increment = EVENT_INCREMENT;
        }

        KiUnwaitThread(WaitThread, STATUS_SUCCESS, Increment);
    }

    //
    // Unlock dispatcher database lock and lower IRQL to its previous
    // value.
    //

    KiUnlockDispatcherDatabase(OldIrql);
    return;
}
