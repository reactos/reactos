/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    dpcsup.c

Abstract:

    This module contains the support routines for the system DPC objects.
    Functions are provided to process quantum end, the power notification
    queue, and timer expiration.

Author:

    David N. Cutler (davec) 22-Apr-1989

Environment:

    Kernel mode only, IRQL DISPATCH_LEVEL.

Revision History:

--*/

#include "ki.h"

//
// Define DPC entry structure and maximum DPC List size.
//

#define MAXIMUM_DPC_LIST_SIZE 16

typedef struct _DPC_ENTRY {
    PRKDPC Dpc;
    PKDEFERRED_ROUTINE Routine;
    PVOID Context;
} DPC_ENTRY, *PDPC_ENTRY;


PRKTHREAD
KiQuantumEnd (
    VOID
    )

/*++

Routine Description:

    This function is called when a quantum end event occurs on the current
    processor. Its function is to determine whether the thread priority should
    be decremented and whether a redispatch of the processor should occur.

Arguments:

    None.

Return Value:

    The next thread to be schedule on the current processor is returned as
    the function value. If this value is not NULL, then the return is with
    the dispatcher database locked. Otherwise, the dispatcher database is
    unlocked.

--*/

{

    KPRIORITY NewPriority;
    KIRQL OldIrql;
    PKPRCB Prcb;
    KPRIORITY Priority;
    PKPROCESS Process;
    PRKTHREAD Thread;
    PRKTHREAD NextThread;

    //
    // Acquire the dispatcher database lock.
    //

    Prcb = KeGetCurrentPrcb();
    Thread = KeGetCurrentThread();
    KiLockDispatcherDatabase(&OldIrql);

    //
    // If the quantum has expired for the current thread, then update its
    // quantum and priority.
    //

    if (Thread->Quantum <= 0) {

        //
        // If quantum runout is disabled for the thread's process and
        // the thread is running at a realtime priority, then set the
        // thread quantum to the highest value and do not round robin
        // at the thread's priority level. Otherwise, reset the thread
        // quantum and decay the thread's priority as appropriate.
        //

        Process = Thread->ApcState.Process;
        if ((Process->DisableQuantum != FALSE) &&
            (Thread->Priority >= LOW_REALTIME_PRIORITY)) {
            Thread->Quantum = MAXCHAR;

        } else {
            Thread->Quantum = Process->ThreadQuantum;

            //
            // Decrement the thread's current priority if the thread is not
            // running in a realtime priority class and check to determine
            // if the processor should be redispatched.
            //

            Priority = Thread->Priority;
            if (Priority < LOW_REALTIME_PRIORITY) {
                NewPriority = Priority - Thread->PriorityDecrement - 1;
                if (NewPriority < Thread->BasePriority) {
                    NewPriority = Thread->BasePriority;
                }

                Thread->PriorityDecrement = 0;

            } else {
                NewPriority = Priority;
            }

            //
            // If the new thread priority is different that the current thread
            // priority, then the thread does not run at a realtime level and
            // its priority should be set. Otherwise, attempt to round robin
            // at the current level.
            //

            if (Priority != NewPriority) {
                KiSetPriorityThread(Thread, NewPriority);

            } else {
                if (Prcb->NextThread == NULL) {
                    NextThread = KiFindReadyThread(Thread->NextProcessor, Priority);

                    if (NextThread != NULL) {
                        NextThread->State = Standby;
                        Prcb->NextThread = NextThread;
                    }

                } else {
                    Thread->Preempted = FALSE;
                }
            }
        }
    }

    //
    // If a thread was scheduled for execution on the current processor,
    // then return the address of the thread with the dispatcher database
    // locked. Otherwise, return NULL with the dispatcher data unlocked.
    //

    NextThread = Prcb->NextThread;
    if (NextThread == NULL) {
        KiUnlockDispatcherDatabase(OldIrql);
    }

    return NextThread;
}

#if DBG


VOID
KiCheckTimerTable (
    IN ULARGE_INTEGER CurrentTime
    )

{

    ULONG Index;
    PLIST_ENTRY ListHead;
    PLIST_ENTRY NextEntry;
    KIRQL OldIrql;
    PKTIMER Timer;

    //
    // Raise IRQL to highest level and scan timer table for timers that
    // have expired.
    //

    KeRaiseIrql(HIGH_LEVEL, &OldIrql);
    Index = 0;
    do {
        ListHead = &KiTimerTableListHead[Index];
        NextEntry = ListHead->Flink;
        while (NextEntry != ListHead) {
            Timer = CONTAINING_RECORD(NextEntry, KTIMER, TimerListEntry);
            NextEntry = NextEntry->Flink;
            if (Timer->DueTime.QuadPart <= CurrentTime.QuadPart) {
                DbgBreakPoint();
            }
        }

        Index += 1;
    } while(Index < TIMER_TABLE_SIZE);

    //
    // Lower IRQL to the previous level.
    //

    KeLowerIrql(OldIrql);
    return;
}

#endif


VOID
KiTimerExpiration (
    IN PKDPC TimerDpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

    This function is called when the clock interupt routine discovers that
    a timer has expired.

Arguments:

    TimerDpc - Supplies a pointer to a control object of type DPC.

    DeferredContext - Not used.

    SystemArgument1 - Supplies the starting timer table index value to
        use for the timer table scan.

    SystemArgument2 - Not used.

Return Value:

    None.

--*/

{
    ULARGE_INTEGER CurrentTime;
    LIST_ENTRY ExpiredListHead;
    LONG HandLimit;
    LONG Index;
    PLIST_ENTRY ListHead;
    PLIST_ENTRY NextEntry;
    KIRQL OldIrql;
    PKTIMER Timer;

    //
    // Acquire the dispatcher database lock and read the current interrupt
    // time to determine which timers have expired.
    //

    KiLockDispatcherDatabase(&OldIrql);
    KiQueryInterruptTime((PLARGE_INTEGER)&CurrentTime);

    //
    // If the timer table has not wrapped, then start with the specified
    // timer table index value, and scan for timer entries that have expired.
    // Otherwise, start with the first entry in the timer table and scan the
    // entire table for timer entries that have expired.
    //
    // N.B. This later condition exists when DPC processing is blocked for a
    //      period longer than one round trip throught the timer table.
    //

    HandLimit = (LONG)KiQueryLowTickCount();
    if (((ULONG)(HandLimit - PtrToLong(SystemArgument1))) >= TIMER_TABLE_SIZE) {
        Index = - 1;
        HandLimit = TIMER_TABLE_SIZE - 1;

    } else {
        Index = (PtrToLong(SystemArgument1) - 1) & (TIMER_TABLE_SIZE - 1);
        HandLimit &= (TIMER_TABLE_SIZE - 1);
    }

    InitializeListHead(&ExpiredListHead);
    do {
        Index = (Index + 1) & (TIMER_TABLE_SIZE - 1);
        ListHead = &KiTimerTableListHead[Index];
        NextEntry = ListHead->Flink;
        while (NextEntry != ListHead) {
            Timer = CONTAINING_RECORD(NextEntry, KTIMER, TimerListEntry);
            if (Timer->DueTime.QuadPart <= CurrentTime.QuadPart) {

                //
                // The next timer in the current timer list has expired.
                // Remove the entry from the timer list and insert the
                // timer in the expired list.
                //

                RemoveEntryList(&Timer->TimerListEntry);
                InsertTailList(&ExpiredListHead, &Timer->TimerListEntry);
                NextEntry = ListHead->Flink;

            } else {
                break;
            }
        }

    } while(Index != HandLimit);

#if DBG

    if ((PtrToUlong(SystemArgument2) == 0) && (KeNumberProcessors == 1)) {
        KiCheckTimerTable(CurrentTime);
    }

#endif

    //
    // Process the expired timer list.
    //
    // N.B. The following function returns with the dispatcher database
    //      unlocked.
    //

    KiTimerListExpire(&ExpiredListHead, OldIrql);
    return;
}

VOID
FASTCALL
KiTimerListExpire (
    IN PLIST_ENTRY ExpiredListHead,
    IN KIRQL OldIrql
    )

/*++

Routine Description:

    This function is called to process a list of timers that have expired.

    N.B. This function is called with the dispatcher database locked and
        returns with the dispatcher database unlocked.

Arguments:

    ExpiredListHead - Supplies a pointer to a list of timers that have
        expired.

    OldIrql - Supplies the previous IRQL.

Return Value:

    None.

--*/

{

    LONG Count;
    PKDPC Dpc;
    DPC_ENTRY DpcList[MAXIMUM_DPC_LIST_SIZE];
    LONG Index;
    LARGE_INTEGER Interval;
    KIRQL OldIrql1;
    LARGE_INTEGER SystemTime;
    PKTIMER Timer;

    //
    // Capture the timer expiration time.
    //

    KiQuerySystemTime(&SystemTime);

    //
    // Remove the next timer from the expired timer list, set the state of
    // the timer to signaled, reinsert the timer in the timer tree if it is
    // periodic, and optionally call the DPC routine if one is specified.
    //

RestartScan:
    Count = 0;
    while (ExpiredListHead->Flink != ExpiredListHead) {
        Timer = CONTAINING_RECORD(ExpiredListHead->Flink, KTIMER, TimerListEntry);
        KiRemoveTreeTimer(Timer);
        Timer->Header.SignalState = 1;
        if (IsListEmpty(&Timer->Header.WaitListHead) == FALSE) {
            KiWaitTest(Timer, TIMER_EXPIRE_INCREMENT);
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
        if (Timer->Period != 0) {
            Interval.QuadPart = Int32x32To64(Timer->Period, - 10 * 1000);
            while (!KiInsertTreeTimer(Timer, Interval)) {
                ;
            }
        }

        if (Timer->Dpc != NULL) {
            Dpc = Timer->Dpc;

            //
            // If the DPC is explicitly targeted to another processor, then
            // queue the DPC to the target processor. Otherwise, capture the
            // DPC parameters for execution on the current processor.
            //

#if defined(NT_UP)

            DpcList[Count].Dpc = Dpc;
            DpcList[Count].Routine = Dpc->DeferredRoutine;
            DpcList[Count].Context = Dpc->DeferredContext;
            Count += 1;
            if (Count == MAXIMUM_DPC_LIST_SIZE) {
                break;
            }

#else

            if ((Dpc->Number >= MAXIMUM_PROCESSORS) &&
                (((ULONG)Dpc->Number - MAXIMUM_PROCESSORS) != (ULONG)KeGetCurrentProcessorNumber())) {
                KeInsertQueueDpc(Dpc,
                                 ULongToPtr(SystemTime.LowPart),
                                 ULongToPtr(SystemTime.HighPart));

            } else {
                DpcList[Count].Dpc = Dpc;
                DpcList[Count].Routine = Dpc->DeferredRoutine;
                DpcList[Count].Context = Dpc->DeferredContext;
                Count += 1;
                if (Count == MAXIMUM_DPC_LIST_SIZE) {
                    break;
                }
            }

#endif

        }
    }

    //
    // Unlock the dispacher database and process DPC list entries.
    //

    if (Count != 0) {
        KiUnlockDispatcherDatabase(DISPATCH_LEVEL);
        Index = 0;
        do {

#if DBG && (defined(i386) || defined(ALPHA))

            //
            // Reset the dpc tick count. If the tick count handler,
            // which increments this value, detects that it has crossed
            // a certain threshold, a breakpoint will be generated.
            //

            KeGetCurrentPrcb()->DebugDpcTime = 0;
#endif

            (DpcList[Index].Routine)(DpcList[Index].Dpc,
                                     DpcList[Index].Context,
                                     ULongToPtr(SystemTime.LowPart),
                                     ULongToPtr(SystemTime.HighPart));


            Index += 1;
        } while (Index < Count);

        //
        // If processing of the expired timer list was terminated because
        // the DPC List was full, then process any remaining entries.
        //

        if (Count == MAXIMUM_DPC_LIST_SIZE) {
            KiLockDispatcherDatabase(&OldIrql1);
            goto RestartScan;
        }

        KeLowerIrql(OldIrql);

    } else {
        KiUnlockDispatcherDatabase(OldIrql);
    }

    return;
}
