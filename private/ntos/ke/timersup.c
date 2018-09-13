/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    timersup.c

Abstract:

    This module contains the support routines for the timer object. It
    contains functions to insert and remove from the timer queue.

Author:

    David N. Cutler (davec) 13-Mar-1989

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

//
// Define forward referenced function prototypes.
//

LOGICAL
FASTCALL
KiInsertTimerTable (
    LARGE_INTEGER Interval,
    LARGE_INTEGER CurrentTime,
    IN PRKTIMER Timer
    );

LOGICAL
FASTCALL
KiInsertTreeTimer (
    IN PRKTIMER Timer,
    IN LARGE_INTEGER Interval
    )

/*++

Routine Description:

    This function inserts a timer object in the timer queue.

    N.B. This routine assumes that the dispatcher data lock has been acquired.

Arguments:

    Timer - Supplies a pointer to a dispatcher object of type timer.

    Interval - Supplies the absolute or relative time at which the time
        is to expire.

Return Value:

    If the timer is inserted in the timer tree, than a value of TRUE is
    returned. Otherwise, a value of FALSE is returned.

--*/

{

    LARGE_INTEGER CurrentTime;
    LARGE_INTEGER SystemTime;
    LARGE_INTEGER TimeDifference;

    //
    // Clear the signal state of timer if the timer period is zero and set
    // the inserted state to TRUE.
    //

    Timer->Header.Inserted = TRUE;
    Timer->Header.Absolute = FALSE;
    if (Timer->Period == 0) {
        Timer->Header.SignalState = FALSE;
    }

    //
    // If the specified interval is not a relative time (i.e., is an absolute
    // time), then convert it to relative time.
    //

    if (Interval.HighPart >= 0) {
        KiQuerySystemTime(&SystemTime);
        TimeDifference.QuadPart = SystemTime.QuadPart - Interval.QuadPart;

        //
        // If the resultant relative time is greater than or equal to zero,
        // then the timer has already expired.
        //

        if (TimeDifference.HighPart >= 0) {
            Timer->Header.SignalState = TRUE;
            Timer->Header.Inserted = FALSE;
            return FALSE;
        }

        Interval = TimeDifference;
        Timer->Header.Absolute = TRUE;
    }

    //
    // Get the current interrupt time, insert the timer in the timer table,
    // and return the inserted state.
    //

    KiQueryInterruptTime(&CurrentTime);
    return KiInsertTimerTable(Interval, CurrentTime, Timer);
}

LOGICAL
FASTCALL
KiReinsertTreeTimer (
    IN PRKTIMER Timer,
    IN ULARGE_INTEGER DueTime
    )

/*++

Routine Description:

    This function reinserts a timer object in the timer queue.

    N.B. This routine assumes that the dispatcher data lock has been acquired.

Arguments:

    Timer - Supplies a pointer to a dispatcher object of type timer.

    DueTime - Supplies the absolute time the timer is to expire.

Return Value:

    If the timer is inserted in the timer tree, than a value of TRUE is
    returned. Otherwise, a value of FALSE is returned.

--*/

{

    LARGE_INTEGER CurrentTime;
    LARGE_INTEGER Interval;

    //
    // Clear the signal state of timer if the timer period is zero and set
    // the inserted state to TRUE.
    //

    Timer->Header.Inserted = TRUE;
    if (Timer->Period == 0) {
        Timer->Header.SignalState = FALSE;
    }

    //
    // Compute the interval between the current time and the due time.
    // If the resultant relative time is greater than or equal to zero,
    // then the timer has already expired.
    //

    KiQueryInterruptTime(&CurrentTime);
    Interval.QuadPart = CurrentTime.QuadPart - DueTime.QuadPart;
    if (Interval.QuadPart >= 0) {
        Timer->Header.SignalState = TRUE;
        Timer->Header.Inserted = FALSE;
        return FALSE;
    }

    //
    // Insert the timer in the timer table and return the inserted state.
    //

    return KiInsertTimerTable(Interval, CurrentTime, Timer);
}

LOGICAL
FASTCALL
KiInsertTimerTable (
    LARGE_INTEGER Interval,
    LARGE_INTEGER CurrentTime,
    IN PRKTIMER Timer
    )

/*++

Routine Description:

    This function inserts a timer object in the timer table.

    N.B. This routine assumes that the dispatcher data lock has been acquired.

Arguments:

    Interval - Supplies the relative timer before the timer is to expire.

    CurrentTime - supplies the current interrupt time.

    Timer - Supplies a pointer to a dispatcher object of type timer.

Return Value:

    If the timer is inserted in the timer tree, than a value of TRUE is
    returned. Otherwise, a value of FALSE is returned.

--*/

{

    ULONG Index;
    PLIST_ENTRY ListHead;
    PLIST_ENTRY NextEntry;
    PRKTIMER NextTimer;
    ULONG SearchCount;

    //
    // Compute the timer table index and set the timer expiration time.
    //

    Index = KiComputeTimerTableIndex(Interval, CurrentTime, Timer);

    //
    // If the timer is due before the first entry in the computed list
    // or the computed list is empty, then insert the timer at the front
    // of the list and check if the timer has already expired. Otherwise,
    // insert then timer in the sorted order of the list searching from
    // the back of the list forward.
    //
    // N.B. The sequence of operations below is critical to avoid the race
    //      condition that exists between this code and the clock interrupt
    //      code that examines the timer table lists to detemine when timers
    //      expire.
    //

    ListHead = &KiTimerTableListHead[Index];
    NextEntry = ListHead->Blink;

#if DBG

    SearchCount = 0;

#endif

    while (NextEntry != ListHead) {

        //
        // Compute the maximum search count.
        //

#if DBG

        SearchCount += 1;
        if (SearchCount > KiMaximumSearchCount) {
            KiMaximumSearchCount = SearchCount;
        }

#endif

        NextTimer = CONTAINING_RECORD(NextEntry, KTIMER, TimerListEntry);
        if (((Timer->DueTime.HighPart == NextTimer->DueTime.HighPart) &&
            (Timer->DueTime.LowPart >= NextTimer->DueTime.LowPart)) ||
            (Timer->DueTime.HighPart > NextTimer->DueTime.HighPart)) {
            InsertHeadList(NextEntry, &Timer->TimerListEntry);
            return TRUE;
        }

        NextEntry = NextEntry->Blink;
    }

    //
    // The computed list is empty or the timer is due to expire before
    // the first entry in the list. Insert the entry in the computed
    // timer table list, then check if the timer has expired.
    //
    // Note that it is critical that the interrupt time not be captured
    // until after the timer has been completely inserted into the list.
    //
    // Otherwise, the clock interrupt code can think the list is empty,
    // and the code here that checks if the timer has expired will use
    // a stale interrupt time.
    //

    InsertHeadList(ListHead, &Timer->TimerListEntry);
    KiQueryInterruptTime(&CurrentTime);
    if (((Timer->DueTime.HighPart == (ULONG)CurrentTime.HighPart) &&
        (Timer->DueTime.LowPart <= CurrentTime.LowPart)) ||
        (Timer->DueTime.HighPart < (ULONG)CurrentTime.HighPart)) {

        //
        // The timer is due to expire before the current time. Remove the
        // timer from the computed list, set its status to Signaled, set
        // its inserted state to FALSE, and
        //

        KiRemoveTreeTimer(Timer);
        Timer->Header.SignalState = TRUE;
    }

    return Timer->Header.Inserted;
}
