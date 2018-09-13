/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    timerobj.c

Abstract:

    This module implements the kernel timer object. Functions are
    provided to initialize, read, set, and cancel timer objects.

Author:

    David N. Cutler (davec) 2-Mar-1989

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGELK, KeQueryTimerDueTime)
#endif

//
// The following assert macro is used to check that an input timer is
// really a ktimer and not something else, like deallocated pool.
//

#define ASSERT_TIMER(E) {                                     \
    ASSERT(((E)->Header.Type == TimerNotificationObject) ||   \
           ((E)->Header.Type == TimerSynchronizationObject)); \
}

VOID
KeInitializeTimer (
    IN PKTIMER Timer
    )

/*++

Routine Description:

    This function initializes a kernel timer object.

Arguments:

    Timer - Supplies a pointer to a dispatcher object of type timer.

Return Value:

    None.

--*/

{

    //
    // Initialize extended timer object with a type of notification and a
    // period of zero.
    //

    KeInitializeTimerEx(Timer, NotificationTimer);
    return;
}

VOID
KeInitializeTimerEx (
    IN PKTIMER Timer,
    IN TIMER_TYPE Type
    )

/*++

Routine Description:

    This function initializes an extended kernel timer object.

Arguments:

    Timer - Supplies a pointer to a dispatcher object of type timer.

    Type - Supplies the type of timer object; NotificationTimer or
        SynchronizationTimer;

Return Value:

    None.

--*/

{
    //
    // Initialize standard dispatcher object header and set initial
    // state of timer.
    //

    Timer->Header.Type = TimerNotificationObject + Type;
    Timer->Header.Inserted = FALSE;
    Timer->Header.Size = sizeof(KTIMER) / sizeof(LONG);
    Timer->Header.SignalState = FALSE;

#if DBG

    Timer->TimerListEntry.Flink = NULL;
    Timer->TimerListEntry.Blink = NULL;

#endif

    InitializeListHead(&Timer->Header.WaitListHead);
    Timer->DueTime.QuadPart = 0;
    Timer->Period = 0;
    return;
}

VOID
KeClearTimer (
    IN PKTIMER Timer
    )

/*++

Routine Description:

    This function clears the signal state of an timer object.

Arguments:

    Event - Supplies a pointer to a dispatcher object of type timer.

Return Value:

    None.

--*/

{

    ASSERT_TIMER(Timer);

    //
    // Clear signal state of timer object.
    //

    Timer->Header.SignalState = 0;
    return;
}

BOOLEAN
KeCancelTimer (
    IN PKTIMER Timer
    )

/*++

Routine Description:

    This function cancels a timer that was previously set to expire at
    a specified time. If the timer is not currently set, then no operation
    is performed. Canceling a timer does not set the state of the timer to
    Signaled.

Arguments:

    Timer - Supplies a pointer to a dispatcher object of type timer.

Return Value:

    A boolean value of TRUE is returned if the the specified timer was
    currently set. Else a value of FALSE is returned.

--*/

{

    BOOLEAN Inserted;
    KIRQL OldIrql;

    ASSERT_TIMER(Timer);
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    //
    // Raise IRQL to dispatcher level, lock the dispatcher database, and
    // capture the timer inserted status. If the timer is currently set,
    // then remove it from the timer list.
    //

    KiLockDispatcherDatabase(&OldIrql);
    Inserted = Timer->Header.Inserted;
    if (Inserted != FALSE) {
        KiRemoveTreeTimer(Timer);
    }

    //
    // Unlock the dispatcher database, lower IRQL to its previous value, and
    // return boolean value that signifies whether the timer was set of not.
    //

    KiUnlockDispatcherDatabase(OldIrql);
    return Inserted;
}

BOOLEAN
KeReadStateTimer (
    IN PKTIMER Timer
    )

/*++

Routine Description:

    This function reads the current signal state of a timer object.

Arguments:

    Timer - Supplies a pointer to a dispatcher object of type timer.

Return Value:

    The current signal state of the timer object.

--*/

{

    ASSERT_TIMER(Timer);

    //
    // Return current signal state of timer object.
    //

    return (BOOLEAN)Timer->Header.SignalState;
}

BOOLEAN
KeSetTimer (
    IN PKTIMER Timer,
    IN LARGE_INTEGER DueTime,
    IN PKDPC Dpc OPTIONAL
    )

/*++

Routine Description:

    This function sets a timer to expire at a specified time. If the timer is
    already set, then it is implicitly canceled before it is set to expire at
    the specified time. Setting a timer causes its due time to be computed,
    its state to be set to Not-Signaled, and the timer object itself to be
    inserted in the timer list.

Arguments:

    Timer - Supplies a pointer to a dispatcher object of type timer.

    DueTime - Supplies an absolute or relative time at which the timer
        is to expire.

    Dpc - Supplies an optional pointer to a control object of type DPC.

Return Value:

    A boolean value of TRUE is returned if the the specified timer was
    currently set. Else a value of FALSE is returned.

--*/

{

    //
    // Set the timer with a period of zero.
    //

    return KeSetTimerEx(Timer, DueTime, 0, Dpc);
}

BOOLEAN
KeSetTimerEx (
    IN PKTIMER Timer,
    IN LARGE_INTEGER DueTime,
    IN LONG Period OPTIONAL,
    IN PKDPC Dpc OPTIONAL
    )

/*++

Routine Description:

    This function sets a timer to expire at a specified time. If the timer is
    already set, then it is implicitly canceled before it is set to expire at
    the specified time. Setting a timer causes its due time to be computed,
    its state to be set to Not-Signaled, and the timer object itself to be
    inserted in the timer list.

Arguments:

    Timer - Supplies a pointer to a dispatcher object of type timer.

    DueTime - Supplies an absolute or relative time at which the timer
        is to expire.

    Period - Supplies an optional period for the timer in milliseconds.

    Dpc - Supplies an optional pointer to a control object of type DPC.

Return Value:

    A boolean value of TRUE is returned if the the specified timer was
    currently set. Else a value of FALSE is returned.

--*/

{

    BOOLEAN Inserted;
    LARGE_INTEGER Interval;
    KIRQL OldIrql;
    LARGE_INTEGER SystemTime;

    ASSERT_TIMER(Timer);
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    //
    // Raise IRQL to dispatcher level and lock dispatcher database.
    //

    KiLockDispatcherDatabase(&OldIrql);

    //
    // Capture the timer inserted status and if the timer is currently
    // set, then remove it from the timer list.
    //

    Inserted = Timer->Header.Inserted;
    if (Inserted != FALSE) {
        KiRemoveTreeTimer(Timer);
    }

    //
    // Clear the signal state, set the period, set the DPC address, and
    // insert the timer in the timer tree. If the timer is not inserted
    // in the timer tree, then it has already expired and as many waiters
    // as possible should be continued, and a DPC, if specified should be
    // queued.
    //
    // N.B. The signal state must be cleared in case the period is not
    //      zero.
    //

    Timer->Header.SignalState = FALSE;
    Timer->Dpc = Dpc;
    Timer->Period = Period;
    if (KiInsertTreeTimer((PRKTIMER)Timer, DueTime) == FALSE) {
        if (IsListEmpty(&Timer->Header.WaitListHead) == FALSE) {
            KiWaitTest(Timer, TIMER_EXPIRE_INCREMENT);
        }

        //
        // If a DPC is specfied, then call the DPC routine.
        //

        if (Dpc != NULL) {
            KiQuerySystemTime(&SystemTime);
            KeInsertQueueDpc(Timer->Dpc,
                             ULongToPtr(SystemTime.LowPart),
                             ULongToPtr(SystemTime.HighPart));
        }

        //
        // If the timer is periodic, then compute the next interval time
        // and reinsert the timer in the timer tree.
        //
        // N.B. Even though the timer insertion is relative, it can still
        //      fail if the period of the timer elapses in between computing
        //      the time and inserting the timer in the table. If this happens,
        //      try again.
        //

        if (Period != 0) {
            Interval.QuadPart = Int32x32To64(Timer->Period, - 10 * 1000);
            while (!KiInsertTreeTimer(Timer, Interval)) {
                ; 
            }
        }
    }

    //
    // Unlock the dispatcher database and lower IRQL to its previous
    // value.
    //

    KiUnlockDispatcherDatabase(OldIrql);

    //
    // Return boolean value that signifies whether the timer was set of
    // not.
    //

    return Inserted;
}

ULONGLONG
KeQueryTimerDueTime (
    IN PKTIMER Timer
    )

/*++

Routine Description:

    This function returns the InterruptTime at which the timer is
    pending.   0 is returned if the timer is not pending.

    N.B. This function may only be called by the system sleep code.

Arguments:

    Timer - Supplies a pointer to a dispatcher object of type timer.

Return Value:

    Returns the amount of time remaining on the timer, or 0 if the
    timer is not pending.

--*/

{

    KIRQL OldIrql;
    LARGE_INTEGER InterruptTime;
    ULONGLONG DueTime;

    ASSERT_TIMER(Timer);

    //
    // Raise IRQL to dispatcher level and lock dispatcher database.
    //

    KiLockDispatcherDatabase(&OldIrql);

    //
    // If the timer is currently pending, compute its due time
    //

    DueTime = 0;
    if (Timer->Header.Inserted) {
        DueTime = Timer->DueTime.QuadPart;
    }

    //
    // Unlock the dispatcher database and lower IRQL to its previous
    // value, and return the due time
    //

    KiUnlockDispatcherDatabase(OldIrql);
    return DueTime;
}

PVOID
KeCheckForTimer(
    IN PVOID BlockStart,
    IN ULONG BlockSize
    )
/*++

Routine Description:

    This function is used for debugging by checking all timers
    to see if any is in the memory block passed.  If so, the
    system stops at a debug breakpoint.

Arguments:

    MemoryBlock - Base address to check for timer

    BlockSize - Size (in bytes) to check in memory block

Return Value:

    The address of the currently active timer.

--*/
{
    ULONG Index;
    PLIST_ENTRY ListHead;
    PLIST_ENTRY NextEntry;
    KIRQL OldIrql;
    PKTIMER Timer;
    PUCHAR Address;
    PUCHAR Start;
    PUCHAR End;

    //
    // Compute the ending memory location.
    //

    Start = (PUCHAR)BlockStart;
    End = Start + BlockSize;

    //
    // Raise IRQL to dispatcher level and lock dispatcher database.
    //

    KiLockDispatcherDatabase(&OldIrql);

    //
    // Run the entire timer database and check for any timers in
    // the memory block
    //

    Index = 0;
    do {
        ListHead = &KiTimerTableListHead[Index];
        NextEntry = ListHead->Flink;
        while (NextEntry != ListHead) {
            Timer = CONTAINING_RECORD(NextEntry, KTIMER, TimerListEntry);
            Address = (PUCHAR)Timer;
            NextEntry = NextEntry->Flink;

            //
            // Check this timer object is not in the range.
            // In each of the following, we check that the object
            // does not overlap the range, for example, if the timer
            // object (in this first check), starts one dword before
            // the range being checked, we have an overlap and should
            // stop.
            //

            if ((Address > (Start - sizeof(KTIMER))) &&
                (Address < End)) {
                KeBugCheckEx(TIMER_OR_DPC_INVALID,
                             0x0,
                             (ULONG_PTR)Address,
                             (ULONG_PTR)Start,
                             (ULONG_PTR)End);
            }

            if (Timer->Dpc) {

                //
                // Check the timer's DPC object isn't in the range.
                //

                Address = (PUCHAR)Timer->Dpc;
                if ((Address > (Start - sizeof(KDPC))) &&
                    (Address < End)) {
                    KeBugCheckEx(TIMER_OR_DPC_INVALID,
                                 0x1,
                                 (ULONG_PTR)Address,
                                 (ULONG_PTR)Start,
                                 (ULONG_PTR)End);
                }

                //
                // Check the timer's DPC routine is not in the range.
                //

                Address = (PUCHAR)Timer->Dpc->DeferredRoutine;
                if (Address >= Start && Address < End) {
                    KeBugCheckEx(TIMER_OR_DPC_INVALID,
                                 0x2,
                                 (ULONG_PTR)Address,
                                 (ULONG_PTR)Start,
                                 (ULONG_PTR)End);
                }
            }
        }

        Index += 1;
    } while(Index < TIMER_TABLE_SIZE);


    //
    // Unlock the dispatcher database and lower IRQL to its previous value
    //

    KiUnlockDispatcherDatabase(OldIrql);
    return NULL;
}
