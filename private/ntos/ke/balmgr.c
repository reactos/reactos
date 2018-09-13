/*++

Copyright (c) 1991-1994  Microsoft Corporation

Module Name:

    balmgr.c

Abstract:

    This module implements the NT balance set manager. Normally the kernel
    does not contain "policy" code. However, the balance set manager needs
    to be able to traverse the kernel data structures and, therefore, the
    code has been located as logically part of the kernel.

    The balance set manager performs the following operations:

        1. Makes the kernel stack of threads that have been waiting for a
           certain amount of time, nonresident.

        2. Removes processes from the balance set when memory gets tight
           and brings processes back into the balance set when there is
           more memory available.

        3. Makes the kernel stack resident for threads whose wait has been
           completed, but whose stack is nonresident.

        4. Arbitrarily boosts the priority of a selected set of threads
           to prevent priority inversion in variable priority levels.

    In general, the balance set manager only is active during periods when
    memory is tight.

Author:

    David N. Cutler (davec) 13-Jul-1991

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

//
// Define balance set wait object types.
//

typedef enum _BALANCE_OBJECT {
    TimerExpiration,
    WorkingSetManagerEvent,
    MaximumObject
    } BALANCE_OBJECT;

//
// Define maximum number of thread stacks that can be out swapped in
// a single time period.
//

#define MAXIMUM_THREAD_STACKS 20

//
// Define periodic wait interval value.
//

#define PERIODIC_INTERVAL (1 * 1000 * 1000 * 10)

//
// Define amount of time a thread can be in the ready state without having
// is priority boosted (approximately 4 seconds).
//

#define READY_WITHOUT_RUNNING  (4 * 75)

//
// Define kernel stack protect time. For small systems the protect time
// is 3 seconds. For all other systems, the protect time is 7 seconds.
//

#define SMALL_SYSTEM_STACK_PROTECT_TIME (3 * 75)
#define STACK_PROTECT_TIME              (7 * 75)
#define STACK_SCAN_PERIOD 4
ULONG KiStackProtectTime;

//
// Define number of threads to scan each period and the priority boost bias.
//

#define THREAD_BOOST_BIAS 1
#define THREAD_BOOST_PRIORITY (LOW_REALTIME_PRIORITY - THREAD_BOOST_BIAS)
#define THREAD_SCAN_PRIORITY (THREAD_BOOST_PRIORITY - 1)
#define THREAD_READY_COUNT 10
#define THREAD_SCAN_COUNT 16

#define EXECUTION_TIME_LIMITS_PERIOD 7

//
// Define local procedure prototypes.
//

VOID
KiInSwapKernelStacks (
    IN KIRQL PreviousIrql
    );

VOID
KiInSwapProcesses (
    IN KIRQL PreviousIrql
    );

VOID
KiOutSwapKernelStacks (
    IN KIRQL PreviousIrql
    );

VOID
KiOutSwapProcesses (
    IN KIRQL PreviousIrql
    );

VOID
KiScanReadyQueues (
    VOID
    );

//
// Define thread table index static data.
//

ULONG KiReadyQueueIndex = 1;

//
// Define swap request flag.
//

BOOLEAN KiStackOutSwapRequest = FALSE;

VOID
KeBalanceSetManager (
    IN PVOID Context
    )

/*++

Routine Description:

    This function is the startup code for the balance set manager. The
    balance set manager thread is created during system initialization
    and begins execution in this function.

Arguments:

    Context - Supplies a pointer to an arbitrary data structure (NULL).

Return Value:

    None.

--*/

{

    LARGE_INTEGER DueTime;
    KTIMER PeriodTimer;
    KIRQL OldIrql;
    ULONG StackScanPeriod;
    ULONG ExecutionTimeLimitPeriod;
    NTSTATUS Status;
    KWAIT_BLOCK WaitBlockArray[MaximumObject];
    PVOID WaitObjects[MaximumObject];

    //
    // Raise the thread priority to the lowest realtime level.
    //

    KeSetPriorityThread(KeGetCurrentThread(), LOW_REALTIME_PRIORITY);

    //
    // Initialize the periodic timer, set it to expire one period from
    // now, and set the stack scan period.
    //

    KeInitializeTimer(&PeriodTimer);
    DueTime.QuadPart = - PERIODIC_INTERVAL;
    KeSetTimer(&PeriodTimer, DueTime, NULL);
    StackScanPeriod = STACK_SCAN_PERIOD;
    ExecutionTimeLimitPeriod = EXECUTION_TIME_LIMITS_PERIOD;
    //
    // Compute the stack protect time based on the system size.
    //

    if (MmQuerySystemSize() == MmSmallSystem) {
        KiStackProtectTime = SMALL_SYSTEM_STACK_PROTECT_TIME;

    } else {
        KiStackProtectTime = STACK_PROTECT_TIME;
    }

    //
    // Initialize the wait objects array.
    //

    WaitObjects[TimerExpiration] = (PVOID)&PeriodTimer;
    WaitObjects[WorkingSetManagerEvent] = (PVOID)&MmWorkingSetManagerEvent;

    //
    // Loop forever processing balance set manager events.
    //

    do {

        //
        // Wait for a memory management memory low event, a swap event,
        // or the expiration of the period timout rate that the balance
        // set manager runs at.
        //

        Status = KeWaitForMultipleObjects(MaximumObject,
                                          &WaitObjects[0],
                                          WaitAny,
                                          Executive,
                                          KernelMode,
                                          FALSE,
                                          NULL,
                                          &WaitBlockArray[0]);

        //
        // Switch on the wait status.
        //

        switch (Status) {

            //
            // Periodic timer expiration.
            //

        case TimerExpiration:

            //
            // Attempt to initiate outswaping of kernel stacks.
            //

            StackScanPeriod -= 1;
            if (StackScanPeriod == 0) {
                StackScanPeriod = STACK_SCAN_PERIOD;
                KiLockDispatcherDatabase(&OldIrql);
                if (KiStackOutSwapRequest == FALSE) {
                    KiStackOutSwapRequest = TRUE;
                    KiUnlockDispatcherDatabase(OldIrql);
                    KeSetEvent(&KiSwapEvent, 0, FALSE);

                } else {
                    KiUnlockDispatcherDatabase(OldIrql);
                }
            }

            //
            // Adjust the depth of lookaside lists.
            //

            ExAdjustLookasideDepth();

            //
            // Scan ready queues and boost thread priorities as appropriate.
            //

            KiScanReadyQueues();

            //
            // Execute the virtual memory working set manager.
            //

            MmWorkingSetManager();

            //
            // Enforce execution time limits
            //

            ExecutionTimeLimitPeriod -= 1;
            if (ExecutionTimeLimitPeriod == 0) {
                ExecutionTimeLimitPeriod = EXECUTION_TIME_LIMITS_PERIOD;
                PsEnforceExecutionTimeLimits();
                }

            //
            // Set the timer to expire at the next periodic interval.
            //

            KeSetTimer(&PeriodTimer, DueTime, NULL);
            break;

            //
            // Working set manager event.
            //

        case WorkingSetManagerEvent:

            //
            // Call the working set manager to trim working sets.
            //

            MmWorkingSetManager();
            break;

            //
            // Illegal return status.
            //

        default:
            KdPrint(("BALMGR: Illegal wait status, %lx =\n", Status));
            break;
        }

    } while (TRUE);
    return;
}

VOID
KeSwapProcessOrStack (
    IN PVOID Context
    )

/*++

Routine Description:

    This thread controls the swapping of processes and kernel stacks. The
    order of evaluation is:

        Outswap kernel stacks
        Outswap processes
        Inswap processes
        Inswap kernel stacks

Arguments:

    Context - Supplies a pointer to the routine context - not used.

Return Value:

    None.

--*/

{

    KIRQL OldIrql;
    NTSTATUS Status;

    //
    // Raise the thread priority to the lowest realtime level + 7 (i.e.,
    // priority 23).
    //

    KeSetPriorityThread(KeGetCurrentThread(), LOW_REALTIME_PRIORITY + 7);

    //
    // Loop for ever processing swap events.
    //

    do {

        //
        // Wait for a swap event to occur.
        //

        Status = KeWaitForSingleObject(&KiSwapEvent,
                                       Executive,
                                       KernelMode,
                                       FALSE,
                                       NULL);

        //
        // Raise IRQL to dispatcher level and lock dispatcher database.
        //

        KiLockDispatcherDatabase(&OldIrql);

        //
        // Loop until all of the four possible actions cannot be initiated.
        //

        do {

            //
            // If a request has been made to out swap kernel stacks, then
            // attempt to outswap kernel stacks. Otherwise, if the process
            // out swap list is not empty, then initiate process outswapping.
            // Otherwise, if the process inswap list is not empty, then start
            // process inswapping. Otherwise, if the kernal stack inswap list
            // is not active, then initiate kernel stack inswapping. Otherwise,
            // no work is available.
            //

            if (KiStackOutSwapRequest != FALSE) {
                KiStackOutSwapRequest = FALSE;
                KiOutSwapKernelStacks(OldIrql);
                continue;

            } else if (IsListEmpty(&KiProcessOutSwapListHead) == FALSE) {
                KiOutSwapProcesses(OldIrql);
                continue;

            } else if (IsListEmpty(&KiProcessInSwapListHead) == FALSE) {
                KiInSwapProcesses(OldIrql);
                continue;

            } else if (IsListEmpty(&KiStackInSwapListHead) == FALSE) {
                KiInSwapKernelStacks(OldIrql);
                continue;

            } else {
                break;
            }
        } while (TRUE);

        //
        // Unlock the dispatcher database and lower IRQL to its previous
        // value.
        //

        KiUnlockDispatcherDatabase(OldIrql);
    } while (TRUE);
    return;
}

VOID
KiInSwapKernelStacks (
    IN KIRQL PreviousIrql
    )

/*++

Routine Description:

    This function in swaps the kernel stack for threads whose wait has been
    completed and whose kernel stack is nonresident.

    N.B. The dispatcher data lock is held on entry to this routine and must
         be help on exit to this routine.

Arguments:

    PreviousIrql - Supplies the previous IRQL.

Return Value:

    None.

--*/

{

    PLIST_ENTRY NextEntry;
    KIRQL OldIrql;
    PKTHREAD Thread;

    //
    // Process the stack in swap list and for each thread removed from the
    // list, make its kernel stack resident, and ready it for execution.
    //

    OldIrql = PreviousIrql;
    NextEntry = KiStackInSwapListHead.Flink;
    while (NextEntry != &KiStackInSwapListHead) {
        Thread = CONTAINING_RECORD(NextEntry, KTHREAD, WaitListEntry);
        RemoveEntryList(NextEntry);
        KiUnlockDispatcherDatabase(OldIrql);
        MmInPageKernelStack(Thread);
        KiLockDispatcherDatabase(&OldIrql);
        Thread->KernelStackResident = TRUE;
        KiReadyThread(Thread);
        NextEntry = KiStackInSwapListHead.Flink;
    }

    return;
}

VOID
KiInSwapProcesses (
    IN KIRQL PreviousIrql
    )

/*++

Routine Description:

    This function in swaps processes.

    N.B. The dispatcher data lock is held on entry to this routine and must
         be help on exit to this routine.

Arguments:

    PreviousIrql - Supplies the previous IRQL.

Return Value:

    None.

--*/

{

    PLIST_ENTRY NextEntry;
    KIRQL OldIrql;
    PKPROCESS Process;
    PKTHREAD Thread;

    //
    // Process the process in swap list and for each process removed from
    // the list, make the process resident, and process its ready list.
    //

    OldIrql = PreviousIrql;
    NextEntry = KiProcessInSwapListHead.Flink;
    while (NextEntry != &KiProcessInSwapListHead) {
        Process = CONTAINING_RECORD(NextEntry, KPROCESS, SwapListEntry);
        RemoveEntryList(NextEntry);
        Process->State = ProcessInSwap;
        KiUnlockDispatcherDatabase(OldIrql);
        MmInSwapProcess(Process);
        KiLockDispatcherDatabase(&OldIrql);
        Process->State = ProcessInMemory;
        NextEntry = Process->ReadyListHead.Flink;
        while (NextEntry != &Process->ReadyListHead) {
            Thread = CONTAINING_RECORD(NextEntry, KTHREAD, WaitListEntry);
            RemoveEntryList(NextEntry);
            Thread->ProcessReadyQueue = FALSE;
            KiReadyThread(Thread);
            NextEntry = Process->ReadyListHead.Flink;
        }

        NextEntry = KiProcessInSwapListHead.Flink;
    }

    return;
}

VOID
KiOutSwapKernelStacks (
    IN KIRQL PreviousIrql
    )

/*++

Routine Description:

    This function attempts to out swap the kernel stack for threads whose
    wait mode is user and which have been waiting longer than the stack
    protect time.

    N.B. The dispatcher data lock is held on entry to this routine and must
         be help on exit to this routine.

Arguments:

    PreviousIrql - Supplies the previous IRQL.

Return Value:

    None.

--*/

{

    ULONG CurrentTick;
    PLIST_ENTRY NextEntry;
    ULONG NumberOfThreads;
    KIRQL OldIrql;
    PKPROCESS Process;
    PKTHREAD Thread;
    PKTHREAD ThreadObjects[MAXIMUM_THREAD_STACKS];
    ULONG WaitTime;

    //
    // Scan the waiting in list and check if the wait time exceeds the
    // stack protect time. If the protect time is exceeded, then make
    // the kernel stack of the waiting thread nonresident. If the count
    // of the number of stacks that are resident for the process reaches
    // zero, then insert the process in the outswap list and set its state
    // to transition.
    //

    CurrentTick = KiQueryLowTickCount();
    OldIrql = PreviousIrql;
    NextEntry = KiWaitInListHead.Flink;
    NumberOfThreads = 0;
    while ((NextEntry != &KiWaitInListHead) &&
           (NumberOfThreads < MAXIMUM_THREAD_STACKS)) {
        Thread = CONTAINING_RECORD(NextEntry, KTHREAD, WaitListEntry);

        ASSERT(Thread->WaitMode == UserMode);

        NextEntry = NextEntry->Flink;
        WaitTime = CurrentTick - Thread->WaitTime;
        if ((WaitTime >= KiStackProtectTime) &&
             KiIsThreadNumericStateSaved(Thread)) {
            Thread->KernelStackResident = FALSE;
            ThreadObjects[NumberOfThreads] = Thread;
            NumberOfThreads += 1;
            RemoveEntryList(&Thread->WaitListEntry);
            InsertTailList(&KiWaitOutListHead, &Thread->WaitListEntry);
            Process = Thread->ApcState.Process;
            Process->StackCount -= 1;
            if (Process->StackCount == 0) {
                Process->State = ProcessInTransition;
                InsertTailList(&KiProcessOutSwapListHead,
                               &Process->SwapListEntry);
            }
        }
    }

    //
    // Unlock the dispatcher database and lower IRQL to its previous
    // value.
    //

    KiUnlockDispatcherDatabase(OldIrql);

    //
    // Out swap the kernels stack for the selected set of threads.
    //

    while (NumberOfThreads > 0) {
        NumberOfThreads -= 1;
        Thread = ThreadObjects[NumberOfThreads];
        MmOutPageKernelStack(Thread);
    }

    //
    // Raise IRQL to dispatcher level and lock dispatcher database.
    //

    KiLockDispatcherDatabase(&OldIrql);
    return;
}

VOID
KiOutSwapProcesses (
    IN KIRQL PreviousIrql
    )

/*++

Routine Description:

    This function out swaps processes.

    N.B. The dispatcher data lock is held on entry to this routine and must
         be help on exit to this routine.

Arguments:

    PreviousIrql - Supplies the previous IRQL.

Return Value:

    None.

--*/

{

    PLIST_ENTRY NextEntry;
    KIRQL OldIrql;
    PKPROCESS Process;
    PKTHREAD Thread;

    //
    // Process the process out swap list and for each process removed from
    // the list, make the process nonresident, and process its ready list.
    //

    OldIrql = PreviousIrql;
    NextEntry = KiProcessOutSwapListHead.Flink;
    while (NextEntry != &KiProcessOutSwapListHead) {
        Process = CONTAINING_RECORD(NextEntry, KPROCESS, SwapListEntry);
        RemoveEntryList(NextEntry);

        //
        // If there are any threads in the process ready list, then don't
        // out swap the process and ready all threads in the process ready
        // list. Otherwsie, out swap the process.
        //

        NextEntry = Process->ReadyListHead.Flink;
        if (NextEntry != &Process->ReadyListHead) {
            Process->State = ProcessInMemory;
            while (NextEntry != &Process->ReadyListHead) {
                Thread = CONTAINING_RECORD(NextEntry, KTHREAD, WaitListEntry);
                RemoveEntryList(NextEntry);
                Thread->ProcessReadyQueue = FALSE;
                KiReadyThread(Thread);
                NextEntry = Process->ReadyListHead.Flink;
            }

        } else {
            Process->State = ProcessOutSwap;
            KiUnlockDispatcherDatabase(OldIrql);
            MmOutSwapProcess(Process);
            KiLockDispatcherDatabase(&OldIrql);

            //
            // While the process was being outswapped there may have been one
            // or more threads that attached to the process. If the process
            // ready list is not empty, then in swap the process. Otherwise,
            // mark the process as out of memory.
            //

            NextEntry = Process->ReadyListHead.Flink;
            if (NextEntry != &Process->ReadyListHead) {
                Process->State = ProcessInTransition;
                InsertTailList(&KiProcessInSwapListHead, &Process->SwapListEntry);

            } else {
                Process->State = ProcessOutOfMemory;
            }
        }

        NextEntry = KiProcessOutSwapListHead.Flink;
    }

    return;
}

VOID
KiScanReadyQueues (
    VOID
    )

/*++

Routine Description:

    This function scans a section of the ready queues and attempts to
    boost the priority of threads that run at variable priority levels.

Arguments:

    None.

Return Value:

    None.

--*/

{

    ULONG Count = 0;
    ULONG CurrentTick;
    PLIST_ENTRY Entry;
    ULONG Index;
    PLIST_ENTRY ListHead;
    ULONG Number = 0;
    KIRQL OldIrql;
    PKPROCESS Process;
    ULONG Summary;
    PKTHREAD Thread;
    ULONG WaitTime;

    //
    // Lock the dispatcher database and check if there are any ready threads
    // queued at the scannable priority levels.
    //

    KiLockDispatcherDatabase(&OldIrql);
    Summary = KiReadySummary & ((1 << THREAD_BOOST_PRIORITY) - 2);
    if (Summary != 0) {
        Count = THREAD_READY_COUNT;
        CurrentTick = KiQueryLowTickCount();
        Index = KiReadyQueueIndex;
        Number = THREAD_SCAN_COUNT;
        do {

            //
            // If the current ready queue index is beyond the end of the range
            // of priorities that are scanned, then wrap back to the beginning
            // priority.
            //

            if (Index > THREAD_SCAN_PRIORITY) {
                Index = 1;
            }

            //
            // If there are any ready threads queued at the current priority
            // level, then attempt to boost the thread priority.
            //

            if (((Summary >> Index) & 1) != 0) {
                Summary ^= (1 << Index);
                ListHead = &KiDispatcherReadyListHead[Index];
                Entry = ListHead->Flink;

                ASSERT(Entry != ListHead);

                do {
                    Thread = CONTAINING_RECORD(Entry, KTHREAD, WaitListEntry);

                    //
                    // If the thread has been waiting for an extended period,
                    // then boost the priority of the selected.
                    //

                    WaitTime = CurrentTick - Thread->WaitTime;
                    if (WaitTime >= READY_WITHOUT_RUNNING) {

                        //
                        // Remove the thread from the respective ready queue.
                        //

                        Entry = Entry->Blink;
                        RemoveEntryList(Entry->Flink);
                        if (IsListEmpty(ListHead) != FALSE) {
                            ClearMember(Index, KiReadySummary);
                        }

                        //
                        // Compute the priority decrement value, set the new
                        // thread priority, set the decrement count, set the
                        // thread quantum, and ready the thread for execution.
                        //

                        Thread->PriorityDecrement +=
                                        THREAD_BOOST_PRIORITY - Thread->Priority;

                        Thread->DecrementCount = ROUND_TRIP_DECREMENT_COUNT;
                        Thread->Priority = THREAD_BOOST_PRIORITY;
                        Process = Thread->ApcState.Process;
                        Thread->Quantum = Process->ThreadQuantum * 2;
                        KiReadyThread(Thread);
                        Count -= 1;
                    }

                    Entry = Entry->Flink;
                    Number -= 1;
                } while ((Entry != ListHead) & (Number != 0) & (Count != 0));
            }

            Index += 1;
        } while ((Summary != 0) & (Number != 0) & (Count != 0));
    }

    //
    // Unlock the dispatcher database and save the last read queue index
    // for the next scan.
    //

    KiUnlockDispatcherDatabase(OldIrql);
    if ((Count != 0) && (Number != 0)) {
        KiReadyQueueIndex = 1;

    } else {
        KiReadyQueueIndex = Index;
    }

    return;
}
