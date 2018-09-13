/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    waitsup.c

Abstract:

    This module contains the support routines necessary to support the
    generic kernel wait functions. Functions are provided to test if a
    wait can be satisfied, to satisfy a wait, and to unwwait a thread.

Author:

    David N. Cutler (davec) 24-Mar-1989

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

VOID
FASTCALL
KiUnwaitThread (
    IN PRKTHREAD Thread,
    IN LONG_PTR WaitStatus,
    IN KPRIORITY Increment
    )

/*++

Routine Description:

    This function unwaits a thread, sets the thread's wait completion status,
    calculates the thread's new priority, and readies the thread for execution.

Arguments:

    Thread - Supplies a pointer to a dispatcher object of type thread.

    WaitStatus - Supplies the wait completion status.

    Increment - Supplies the priority increment that is to be applied to
        the thread's priority.

Return Value:

    None.

--*/

{

    KPRIORITY NewPriority;
    PKPROCESS Process;
    PKQUEUE Queue;
    PKTIMER Timer;
    PRKWAIT_BLOCK WaitBlock;

    //
    // Set wait completion status, remove wait blocks from object wait
    // lists, and remove thread from wait list.
    //

    Thread->WaitStatus |= WaitStatus;
    WaitBlock = Thread->WaitBlockList;
    do {
        RemoveEntryList(&WaitBlock->WaitListEntry);
        WaitBlock = WaitBlock->NextWaitBlock;
    } while (WaitBlock != Thread->WaitBlockList);

    RemoveEntryList(&Thread->WaitListEntry);

    //
    // If thread timer is still active, then cancel thread timer.
    //

    Timer = &Thread->Timer;
    if (Timer->Header.Inserted != FALSE) {
        KiRemoveTreeTimer(Timer);
    }

    //
    // If the thread is processing a queue entry, then increment the
    // count of currently active threads.
    //

    Queue = Thread->Queue;
    if (Queue != NULL) {
        Queue->CurrentCount += 1;
    }

    //
    // If the thread runs at a realtime priority level, then reset the
    // thread quantum. Otherwise, compute the next thread priority and
    // charge the thread for the wait operation.
    //

    Process = Thread->ApcState.Process;
    if (Thread->Priority < LOW_REALTIME_PRIORITY) {
        if ((Thread->PriorityDecrement == 0) &&
            (Thread->DisableBoost == FALSE)) {
            NewPriority = Thread->BasePriority + Increment;
            if (((PEPROCESS)Process)->Vm.MemoryPriority == MEMORY_PRIORITY_FOREGROUND) {
                NewPriority += PsPrioritySeperation;
            }

            if (NewPriority > Thread->Priority) {
                if (NewPriority >= LOW_REALTIME_PRIORITY) {
                    Thread->Priority = LOW_REALTIME_PRIORITY - 1;

                } else {
                    Thread->Priority = (SCHAR)NewPriority;
                }
            }
        }

        if (Thread->BasePriority >= TIME_CRITICAL_PRIORITY_BOUND) {
            Thread->Quantum = Process->ThreadQuantum;

        } else {
            Thread->Quantum -= WAIT_QUANTUM_DECREMENT;
            if (Thread->Quantum <= 0) {
                Thread->Quantum = Process->ThreadQuantum;
                Thread->Priority -= (Thread->PriorityDecrement + 1);
                if (Thread->Priority < Thread->BasePriority) {
                    Thread->Priority = Thread->BasePriority;
                }

                Thread->PriorityDecrement = 0;
            }
        }

    } else {
        Thread->Quantum = Process->ThreadQuantum;
    }

    //
    // Reready the thread for execution.
    //

    KiReadyThread(Thread);
    return;
}

VOID
KeBoostCurrentThread(
    VOID
    )

/*++

Routine Description:

    This function boosts the priority of the current thread for one quantum,
    then reduce the thread priority to the base priority of the thread.

Arguments:

    None.

Return Value:

    None.

--*/

{

    KIRQL OldIrql;
    PKTHREAD Thread;

    //
    // Get current thread address, raise IRQL to synchronization level, and
    // lock the dispatcher database
    //

    Thread = KeGetCurrentThread();

redoboost:
    KiLockDispatcherDatabase(&OldIrql);

    //
    // If a priority boost is not already active for the current thread
    // and the thread priority is less than 14, then boost the thread
    // priority to 14 and give the thread a large quantum. Otherwise,
    // if a priority boost is active, then decrement the round trip
    // count. If the count goes to zero, then release the dispatcher
    // database lock, lower the thread priority to the base priority,
    // and then attempt to boost the priority again. This will give
    // other threads a chance to run. If the count does not reach zero,
    // then give the thread another large qunatum.
    //
    // If the thread priority is above 14, then no boost is applied.
    //

    if ((Thread->PriorityDecrement == 0) && (Thread->Priority < 14)) {
        Thread->PriorityDecrement = 14 - Thread->BasePriority;
        Thread->DecrementCount = ROUND_TRIP_DECREMENT_COUNT;
        Thread->Priority = 14;
        Thread->Quantum = Thread->ApcState.Process->ThreadQuantum * 2;

    } else if (Thread->PriorityDecrement != 0) {
        Thread->DecrementCount -= 1;
        if (Thread->DecrementCount == 0) {
            KiUnlockDispatcherDatabase(OldIrql);
            KeSetPriorityThread(Thread, Thread->BasePriority);
            goto redoboost;

        } else {
            Thread->Quantum = Thread->ApcState.Process->ThreadQuantum * 2;
        }
    }

    KiUnlockDispatcherDatabase(OldIrql);
    return;
}

VOID
FASTCALL
KiWaitSatisfyAll (
    IN PRKWAIT_BLOCK WaitBlock
    )

/*++

Routine Description:

    This function satisfies a wait all and performs any side effects that
    are necessary.

Arguments:

    WaitBlock - Supplies a pointer to a wait block.

Return Value:

    None.

--*/

{

    PKMUTANT Object;
    PRKTHREAD Thread;
    PRKWAIT_BLOCK WaitBlock1;

    //
    // If the wait type was WaitAny, then perform neccessary side effects on
    // the object specified by the wait block. Else perform necessary side
    // effects on all the objects that were involved in the wait operation.
    //

    WaitBlock1 = WaitBlock;
    Thread = WaitBlock1->Thread;
    do {
        if (WaitBlock1->WaitKey != (CSHORT)STATUS_TIMEOUT) {
            Object = (PKMUTANT)WaitBlock1->Object;
            KiWaitSatisfyAny(Object, Thread);
        }

        WaitBlock1 = WaitBlock1->NextWaitBlock;
    } while (WaitBlock1 != WaitBlock);

    return;
}

VOID
FASTCALL
KiWaitTest (
    IN PVOID Object,
    IN KPRIORITY Increment
    )

/*++

Routine Description:

    This function tests if a wait can be satisfied when an object attains
    a state of signaled. If a wait can be satisfied, then the subject thread
    is unwaited with a completion status that is the WaitKey of the wait
    block from the object wait list. As many waits as possible are satisfied.

Arguments:

    Object - Supplies a pointer to a dispatcher object.

Return Value:

    None.

--*/

{

    PKEVENT Event;
    PLIST_ENTRY ListHead;
    PRKWAIT_BLOCK NextBlock;
    PKMUTANT Mutant;
    PRKTHREAD Thread;
    PRKWAIT_BLOCK WaitBlock;
    PLIST_ENTRY WaitEntry;

    //
    // As long as the signal state of the specified object is Signaled and
    // there are waiters in the object wait list, then try to satisfy a wait.
    //

    Event = (PKEVENT)Object;
    ListHead = &Event->Header.WaitListHead;
    WaitEntry = ListHead->Flink;
    while ((Event->Header.SignalState > 0) &&
           (WaitEntry != ListHead)) {
        WaitBlock = CONTAINING_RECORD(WaitEntry, KWAIT_BLOCK, WaitListEntry);
        Thread = WaitBlock->Thread;
        if (WaitBlock->WaitType != WaitAny) {

            //
            // The wait type is wait all - if all the objects are in
            // a Signaled state, then satisfy the wait.
            //

            NextBlock = WaitBlock->NextWaitBlock;
            while (NextBlock != WaitBlock) {
                if (NextBlock->WaitKey != (CSHORT)(STATUS_TIMEOUT)) {
                    Mutant = (PKMUTANT)NextBlock->Object;
                    if ((Mutant->Header.Type == MutantObject) &&
                        (Mutant->Header.SignalState <= 0) &&
                        (Thread == Mutant->OwnerThread)) {
                        goto next;

                    } else if (Mutant->Header.SignalState <= 0) {
                        goto scan;
                    }
                }

            next:
                NextBlock = NextBlock->NextWaitBlock;
            }

            //
            // All objects associated with the wait are in the Signaled
            // state - satisfy the wait.
            //

            WaitEntry = WaitEntry->Blink;
            KiWaitSatisfyAll(WaitBlock);

        } else {

            //
            // The wait type is wait any - satisfy the wait.
            //

            WaitEntry = WaitEntry->Blink;
            KiWaitSatisfyAny((PKMUTANT)Event, Thread);
        }

        KiUnwaitThread(Thread, (NTSTATUS)WaitBlock->WaitKey, Increment);

    scan:
        WaitEntry = WaitEntry->Flink;
    }

    return;
}
