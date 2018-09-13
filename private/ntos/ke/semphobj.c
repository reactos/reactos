/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    semphobj.c

Abstract:

    This module implements the kernel semaphore object. Functions
    are provided to initialize, read, and release semaphore objects.

Author:

    David N. Cutler (davec) 28-Feb-1989

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

//
// The following assert macro is used to check that an input semaphore is
// really a ksemaphore and not something else, like deallocated pool.
//

#define ASSERT_SEMAPHORE(E) {                    \
    ASSERT((E)->Header.Type == SemaphoreObject); \
}


VOID
KeInitializeSemaphore (
    IN PRKSEMAPHORE Semaphore,
    IN LONG Count,
    IN LONG Limit
    )

/*++

Routine Description:

    This function initializes a kernel semaphore object. The initial
    count and limit of the object are set to the specified values.

Arguments:

    Semaphore - Supplies a pointer to a dispatcher object of type
        semaphore.

    Count - Supplies the initial count value to be assigned to the
        semaphore.

    Limit - Supplies the maximum count value that the semaphore
        can attain.

Return Value:

    None.

--*/

{

    //
    // Initialize standard dispatcher object header and set initial
    // count and maximum count values.
    //

    Semaphore->Header.Type = SemaphoreObject;
    Semaphore->Header.Size = sizeof(KSEMAPHORE) / sizeof(LONG);
    Semaphore->Header.SignalState = Count;
    InitializeListHead(&Semaphore->Header.WaitListHead);
    Semaphore->Limit = Limit;
    return;
}

LONG
KeReadStateSemaphore (
    IN PRKSEMAPHORE Semaphore
    )

/*++

Routine Description:

    This function reads the current signal state of a semaphore object.

Arguments:

    Semaphore - Supplies a pointer to a dispatcher object of type
        semaphore.

Return Value:

    The current signal state of the semaphore object.

--*/

{

    ASSERT_SEMAPHORE( Semaphore );

    //
    // Return current signal state of semaphore object.
    //

    return Semaphore->Header.SignalState;
}

LONG
KeReleaseSemaphore (
    IN PRKSEMAPHORE Semaphore,
    IN KPRIORITY Increment,
    IN LONG Adjustment,
    IN BOOLEAN Wait
    )

/*++

Routine Description:

    This function releases a semaphore by adding the specified adjustment
    value to the current semaphore count and attempts to satisfy as many
    Waits as possible. The previous signal state of the semaphore object
    is returned as the function value.

Arguments:

    Semaphore - Supplies a pointer to a dispatcher object of type
        semaphore.

    Increment - Supplies the priority increment that is to be applied
        if releasing the semaphore causes a Wait to be satisfied.

    Adjustment - Supplies value that is to be added to the current
        semaphore count.

    Wait - Supplies a boolean value that signifies whether the call to
        KeReleaseSemaphore will be immediately followed by a call to one
        of the kernel Wait functions.

Return Value:

    The previous signal state of the semaphore object.

--*/

{

    LONG NewState;
    KIRQL OldIrql;
    LONG OldState;
    PRKTHREAD Thread;

    ASSERT_SEMAPHORE( Semaphore );
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    //
    // Raise IRQL to dispatcher level and lock dispatcher database.
    //

    KiLockDispatcherDatabase(&OldIrql);

    //
    // Capture the current signal state of the semaphore object and
    // compute the new count value.
    //

    OldState = Semaphore->Header.SignalState;
    NewState = OldState + Adjustment;

    //
    // If the new state value is greater than the limit or a carry occurs,
    // then unlock the dispatcher database, and raise an exception.
    //

    if ((NewState > Semaphore->Limit) || (NewState < OldState)) {
        KiUnlockDispatcherDatabase(OldIrql);
        ExRaiseStatus(STATUS_SEMAPHORE_LIMIT_EXCEEDED);
    }

    //
    // Set the new signal state of the semaphore object and set the wait
    // next value. If the previous signal state was Not-Signaled (i.e.
    // the count was zero), and the wait queue is not empty, then attempt
    // to satisfy as many Waits as possible.
    //

    Semaphore->Header.SignalState = NewState;
    if ((OldState == 0) && (IsListEmpty(&Semaphore->Header.WaitListHead) == FALSE)) {
        KiWaitTest(Semaphore, Increment);
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
    // Return previous signal state of sempahore object.
    //

    return OldState;
}
