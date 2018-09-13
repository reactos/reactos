/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    wait.c

Abstract:

    This module implements the generic kernel wait routines. Functions
    are provided to wait for a single object, wait for multiple objects,
    wait for event pair low, wait for event pair high, release and wait
    for semaphore, and to delay thread execution.

    N.B. This module is written to be a fast as possible and not as small
        as possible. Therefore some code sequences are duplicated to avoid
        procedure calls. It would also be possible to combine wait for
        single object into wait for multiple objects at the cost of some
        speed. Since wait for single object is the most common case, the
        two routines have been separated.

Author:

    David N. Cutler (davec) 23-Mar-89

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

//
// Test for alertable condition.
//
// If alertable is TRUE and the thread is alerted for a processor
// mode that is equal to the wait mode, then return immediately
// with a wait completion status of ALERTED.
//
// Else if alertable is TRUE, the wait mode is user, and the user APC
// queue is not empty, then set user APC pending, and return immediately
// with a wait completion status of USER_APC.
//
// Else if alertable is TRUE and the thread is alerted for kernel
// mode, then return immediately with a wait completion status of
// ALERTED.
//
// Else if alertable is FALSE and the wait mode is user and there is a
// user APC pending, then return immediately with a wait completion
// status of USER_APC.
//

#define TestForAlertPending(Alertable) \
    if (Alertable) { \
        if (Thread->Alerted[WaitMode] != FALSE) { \
            Thread->Alerted[WaitMode] = FALSE; \
            WaitStatus = STATUS_ALERTED; \
            break; \
        } else if ((WaitMode != KernelMode) && \
                  (IsListEmpty(&Thread->ApcState.ApcListHead[UserMode])) == FALSE) { \
            Thread->ApcState.UserApcPending = TRUE; \
            WaitStatus = STATUS_USER_APC; \
            break; \
        } else if (Thread->Alerted[KernelMode] != FALSE) { \
            Thread->Alerted[KernelMode] = FALSE; \
            WaitStatus = STATUS_ALERTED; \
            break; \
        } \
    } else if ((WaitMode != KernelMode) && (Thread->ApcState.UserApcPending)) { \
        WaitStatus = STATUS_USER_APC; \
        break; \
    }

VOID
KiAdjustQuantumThread (
    IN PKTHREAD Thread
    )

/*++

Routine Description:

    If the current thread is not a time critical or realtime thread, then
    adjust its quantum in accordance with the adjustment that would have
    occured if the thread had actually waited.

Arguments:

    Thread - Supplies a pointer to the current thread.

Return Value:

    None.

--*/

{
    PKPRCB Prcb;
    PKPROCESS Process;
    PKTHREAD NewThread;
    SCHAR ThreadPriority;

    if ((Thread->Priority < LOW_REALTIME_PRIORITY) &&
        (Thread->BasePriority < TIME_CRITICAL_PRIORITY_BOUND)) {
        Thread->Quantum -= WAIT_QUANTUM_DECREMENT;
        if (Thread->Quantum <= 0) {
            Process = Thread->ApcState.Process;
            Thread->Quantum = Process->ThreadQuantum;
            ThreadPriority = Thread->Priority - (Thread->PriorityDecrement + 1);
            if (ThreadPriority < Thread->BasePriority) {
                ThreadPriority = Thread->BasePriority;
            }

            Thread->PriorityDecrement = 0;
            if (ThreadPriority != Thread->Priority) {
                KiSetPriorityThread(Thread, ThreadPriority);

            } else {
                Prcb = KeGetCurrentPrcb();
                if (Prcb->NextThread == NULL) {
                    NewThread = KiFindReadyThread(Thread->NextProcessor,
                                                  ThreadPriority);
                    if (NewThread != NULL) {
                        Prcb->NextThread = NewThread;
                        NewThread->State = Standby;
                    }
                }
            }
        }
    }

    return;
}

NTSTATUS
KeDelayExecutionThread (
    IN KPROCESSOR_MODE WaitMode,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Interval
    )

/*++

Routine Description:

    This function delays the execution of the current thread for the specified
    interval of time.

Arguments:

    WaitMode  - Supplies the processor mode in which the delay is to occur.

    Alertable - Supplies a boolean value that specifies whether the delay
        is alertable.

    Interval - Supplies a pointer to the absolute or relative time over which
        the delay is to occur.

Return Value:

    The wait completion status. A value of STATUS_SUCCESS is returned if
    the delay occurred. A value of STATUS_ALERTED is returned if the wait
    was aborted to deliver an alert to the current thread. A value of
    STATUS_USER_APC is returned if the wait was aborted to deliver a user
    APC to the current thread.

--*/

{

    LARGE_INTEGER DueTime;
    LARGE_INTEGER NewTime;
    PLARGE_INTEGER OriginalTime;
    PKPRCB Prcb;
    KPRIORITY Priority;
    PRKQUEUE Queue;
    PRKTHREAD Thread;
    PRKTIMER Timer;
    PKWAIT_BLOCK WaitBlock;
    NTSTATUS WaitStatus;

    //
    // If the dispatcher database lock is not already held, then set the wait
    // IRQL and lock the dispatcher database. Else set boolean wait variable
    // to FALSE.
    //

    Thread = KeGetCurrentThread();
    if (Thread->WaitNext) {
        Thread->WaitNext = FALSE;

    } else {
        KiLockDispatcherDatabase(&Thread->WaitIrql);
    }

    //
    // Start of delay loop.
    //
    // Note this loop is repeated if a kernel APC is delivered in the middle
    // of the delay or a kernel APC is pending on the first attempt through
    // the loop.
    //

    OriginalTime = Interval;
    WaitBlock = &Thread->WaitBlock[TIMER_WAIT_BLOCK];
    do {

        //
        // Test to determine if a kernel APC is pending.
        //
        // If a kernel APC is pending and the previous IRQL was less than
        // APC_LEVEL, then a kernel APC was queued by another processor just
        // after IRQL was raised to DISPATCH_LEVEL, but before the dispatcher
        // database was locked.
        //
        // N.B. that this can only happen in a multiprocessor system.
        //

        if (Thread->ApcState.KernelApcPending && (Thread->WaitIrql < APC_LEVEL)) {

            //
            // Unlock the dispatcher database and lower IRQL to its previous
            // value. An APC interrupt will immediately occur which will result
            // in the delivery of the kernel APC if possible.
            //

            KiUnlockDispatcherDatabase(Thread->WaitIrql);

        } else {

            //
            // Test for alert pending.
            //

            TestForAlertPending(Alertable);

            //
            // Initialize wait block, insert wait block in timer wait list,
            // insert timer in timer queue, put thread in wait state, select
            // next thread to execute, and context switch to next thread.
            //
            // N.B. The timer wait block is initialized when the respective
            //      thread is initialized. Thus the constant fields are not
            //      reinitialized. These include the wait object, wait key,
            //      wait type, and the wait list entry link pointers.
            //

            Thread->WaitBlockList = WaitBlock;
            Thread->WaitStatus = (NTSTATUS)0;
            Timer = &Thread->Timer;
            WaitBlock->NextWaitBlock = WaitBlock;
            Timer->Header.WaitListHead.Flink = &WaitBlock->WaitListEntry;
            Timer->Header.WaitListHead.Blink = &WaitBlock->WaitListEntry;

            //
            // If the timer is inserted in the timer tree, then place the
            // current thread in a wait state. Otherwise, attempt to force
            // the current thread to yield the processor to another thread.
            //

            if (KiInsertTreeTimer(Timer, *Interval) == FALSE) {

                //
                // If the thread is not a realtime thread, then drop the
                // thread priority to the base priority.
                //

                Prcb = KeGetCurrentPrcb();
                Priority = Thread->Priority;
                if (Priority < LOW_REALTIME_PRIORITY) {
                    if (Priority != Thread->BasePriority) {
                        Thread->PriorityDecrement = 0;
                        KiSetPriorityThread(Thread, Thread->BasePriority);
                    }
                }

                //
                // If a new thread has not been selected, the attempt to round
                // robin the thread with other threads at the same priority.
                //

                if (Prcb->NextThread == NULL) {
                    Prcb->NextThread = KiFindReadyThread(Thread->NextProcessor,
                                                         Thread->Priority);
                }

                //
                // If a new thread has been selected for execution, then
                // switch immediately to the selected thread.
                //

                if (Prcb->NextThread != NULL) {

                    //
                    // Give the current thread a new qunatum and switch
                    // context to selected thread.
                    //
                    // N.B. Control is returned at the original IRQL.
                    //

                    Thread->Preempted = FALSE;
                    Thread->Quantum = Thread->ApcState.Process->ThreadQuantum;

                    ASSERT(Thread->WaitIrql <= DISPATCH_LEVEL);

                    KiReadyThread(Thread);
                    WaitStatus = (NTSTATUS)KiSwapThread();
                    goto WaitComplete;

                } else {
                    WaitStatus = (NTSTATUS)STATUS_SUCCESS;
                    break;
                }
            }

            DueTime.QuadPart = Timer->DueTime.QuadPart;

            //
            // If the current thread is processing a queue entry, then attempt
            // to activate another thread that is blocked on the queue object.
            //

            Queue = Thread->Queue;
            if (Queue != NULL) {
                KiActivateWaiterQueue(Queue);
            }

            //
            // Set the thread wait parameters, set the thread dispatcher state
            // to Waiting, and insert the thread in the wait list.
            //

            Thread->Alertable = Alertable;
            Thread->WaitMode = WaitMode;
            Thread->WaitReason = DelayExecution;
            Thread->WaitTime= KiQueryLowTickCount();
            Thread->State = Waiting;
            KiInsertWaitList(WaitMode, Thread);

            //
            // Switch context to selected thread.
            //
            // N.B. Control is returned at the original IRQL.
            //

            ASSERT(Thread->WaitIrql <= DISPATCH_LEVEL);

            WaitStatus = (NTSTATUS)KiSwapThread();

            //
            // If the thread was not awakened to deliver a kernel mode APC,
            // then return the wait status.
            //

        WaitComplete:
            if (WaitStatus != STATUS_KERNEL_APC) {
                if (WaitStatus == STATUS_TIMEOUT) {
                    WaitStatus = STATUS_SUCCESS;
                }

                return WaitStatus;
            }

            //
            // Reduce the time remaining before the time delay expires.
            //

            Interval = KiComputeWaitInterval(OriginalTime,
                                             &DueTime,
                                             &NewTime);
        }

        //
        // Raise IRQL to DISPATCH_LEVEL and lock the dispatcher database.
        //

        KiLockDispatcherDatabase(&Thread->WaitIrql);
    } while (TRUE);

    //
    // The thread is alerted or a user APC should be delivered. Unlock the
    // dispatcher database, lower IRQL to its previous value, and return the
    // wait status.
    //

    KiUnlockDispatcherDatabase(Thread->WaitIrql);
    return WaitStatus;
}

NTSTATUS
KeWaitForMultipleObjects (
    IN ULONG Count,
    IN PVOID Object[],
    IN WAIT_TYPE WaitType,
    IN KWAIT_REASON WaitReason,
    IN KPROCESSOR_MODE WaitMode,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL,
    IN PKWAIT_BLOCK WaitBlockArray OPTIONAL
    )

/*++

Routine Description:

    This function waits until the specified objects attain a state of
    Signaled. The wait can be specified to wait until all of the objects
    attain a state of Signaled or until one of the objects attains a state
    of Signaled. An optional timeout can also be specified. If a timeout
    is not specified, then the wait will not be satisfied until the objects
    attain a state of Signaled. If a timeout is specified, and the objects
    have not attained a state of Signaled when the timeout expires, then
    the wait is automatically satisfied. If an explicit timeout value of
    zero is specified, then no wait will occur if the wait cannot be satisfied
    immediately. The wait can also be specified as alertable.

Arguments:

    Count - Supplies a count of the number of objects that are to be waited
        on.

    Object[] - Supplies an array of pointers to dispatcher objects.

    WaitType - Supplies the type of wait to perform (WaitAll, WaitAny).

    WaitReason - Supplies the reason for the wait.

    WaitMode  - Supplies the processor mode in which the wait is to occur.

    Alertable - Supplies a boolean value that specifies whether the wait is
        alertable.

    Timeout - Supplies a pointer to an optional absolute of relative time over
        which the wait is to occur.

    WaitBlockArray - Supplies an optional pointer to an array of wait blocks
        that are to used to describe the wait operation.

Return Value:

    The wait completion status. A value of STATUS_TIMEOUT is returned if a
    timeout occurred. The index of the object (zero based) in the object
    pointer array is returned if an object satisfied the wait. A value of
    STATUS_ALERTED is returned if the wait was aborted to deliver an alert
    to the current thread. A value of STATUS_USER_APC is returned if the
    wait was aborted to deliver a user APC to the current thread.

--*/

{

    LARGE_INTEGER DueTime;
    ULONG Index;
    LARGE_INTEGER NewTime;
    PRKTHREAD NextThread;
    PKMUTANT Objectx;
    PLARGE_INTEGER OriginalTime;
    PRKQUEUE Queue;
    PRKTHREAD Thread;
    PRKTIMER Timer;
    PRKWAIT_BLOCK WaitBlock;
    BOOLEAN WaitSatisfied;
    NTSTATUS WaitStatus;
    PKWAIT_BLOCK WaitTimer;

    //
    // If the dispatcher database lock is not already held, then set the wait
    // IRQL and lock the dispatcher database. Else set boolean wait variable
    // to FALSE.
    //

    Thread = KeGetCurrentThread();
    if (Thread->WaitNext) {
        Thread->WaitNext = FALSE;

    } else {
        KiLockDispatcherDatabase(&Thread->WaitIrql);
    }

    //
    // If a wait block array has been specified, then the maximum number of
    // objects that can be waited on is specified by MAXIMUM_WAIT_OBJECTS.
    // Otherwise the builtin wait blocks in the thread object are used and
    // the maximum number of objects that can be waited on is specified by
    // THREAD_WAIT_OBJECTS. If the specified number of objects is not within
    // limits, then bug check.
    //

    if (ARGUMENT_PRESENT(WaitBlockArray)) {
        if (Count > MAXIMUM_WAIT_OBJECTS) {
            KeBugCheck(MAXIMUM_WAIT_OBJECTS_EXCEEDED);
        }

    } else {
        if (Count > THREAD_WAIT_OBJECTS) {
            KeBugCheck(MAXIMUM_WAIT_OBJECTS_EXCEEDED);
        }

        WaitBlockArray = &Thread->WaitBlock[0];
    }

    //
    // Start of wait loop.
    //
    // Note this loop is repeated if a kernel APC is delivered in the middle
    // of the wait or a kernel APC is pending on the first attempt through
    // the loop.
    //

    OriginalTime = Timeout;
    do {

        //
        // Set address of wait block list in thread object.
        //

        Thread->WaitBlockList = WaitBlockArray;

        //
        // Test to determine if a kernel APC is pending.
        //
        // If a kernel APC is pending and the previous IRQL was less than
        // APC_LEVEL, then a kernel APC was queued by another processor just
        // after IRQL was raised to DISPATCH_LEVEL, but before the dispatcher
        // database was locked.
        //
        // N.B. that this can only happen in a multiprocessor system.
        //

        if (Thread->ApcState.KernelApcPending && (Thread->WaitIrql < APC_LEVEL)) {

            //
            // Unlock the dispatcher database and lower IRQL to its previous
            // value. An APC interrupt will immediately occur which will result
            // in the delivery of the kernel APC if possible.
            //

            KiUnlockDispatcherDatabase(Thread->WaitIrql);

        } else {

            //
            // Construct wait blocks and check to determine if the wait is
            // already satisfied. If the wait is satisfied, then perform
            // wait completion and return. Else put current thread in a wait
            // state if an explicit timeout value of zero is not specified.
            //

            Thread->WaitStatus = (NTSTATUS)0;
            WaitSatisfied = TRUE;
            for (Index = 0; Index < Count; Index += 1) {

                //
                // Test if wait can be satisfied immediately.
                //

                Objectx = (PKMUTANT)Object[Index];

                ASSERT(Objectx->Header.Type != QueueObject);

                if (WaitType == WaitAny) {

                    //
                    // If the object is a mutant object and the mutant object
                    // has been recursively acquired MINLONG times, then raise
                    // an exception. Otherwise if the signal state of the mutant
                    // object is greater than zero, or the current thread is
                    // the owner of the mutant object, then satisfy the wait.
                    //

                    if (Objectx->Header.Type == MutantObject) {
                        if ((Objectx->Header.SignalState > 0) ||
                            (Thread == Objectx->OwnerThread)) {
                            if (Objectx->Header.SignalState != MINLONG) {
                                KiWaitSatisfyMutant(Objectx, Thread);
                                WaitStatus = (NTSTATUS)(Index | Thread->WaitStatus);
                                goto NoWait;

                            } else {
                                KiUnlockDispatcherDatabase(Thread->WaitIrql);
                                ExRaiseStatus(STATUS_MUTANT_LIMIT_EXCEEDED);
                            }
                        }

                    //
                    // If the signal state is greater than zero, then satisfy
                    // the wait.
                    //

                    } else if (Objectx->Header.SignalState > 0) {
                        KiWaitSatisfyOther(Objectx);
                        WaitStatus = (NTSTATUS)(Index);
                        goto NoWait;
                    }

                } else {

                    //
                    // If the object is a mutant object and the mutant object
                    // has been recursively acquired MAXLONG times, then raise
                    // an exception. Otherwise if the signal state of the mutant
                    // object is less than or equal to zero and the current
                    // thread is not the  owner of the mutant object, then the
                    // wait cannot be satisfied.
                    //

                    if (Objectx->Header.Type == MutantObject) {
                        if ((Thread == Objectx->OwnerThread) &&
                            (Objectx->Header.SignalState == MINLONG)) {
                            KiUnlockDispatcherDatabase(Thread->WaitIrql);
                            ExRaiseStatus(STATUS_MUTANT_LIMIT_EXCEEDED);

                        } else if ((Objectx->Header.SignalState <= 0) &&
                                  (Thread != Objectx->OwnerThread)) {
                            WaitSatisfied = FALSE;
                        }

                    //
                    // If the signal state is less than or equal to zero, then
                    // the wait cannot be satisfied.
                    //

                    } else if (Objectx->Header.SignalState <= 0) {
                        WaitSatisfied = FALSE;
                    }
                }

                //
                // Construct wait block for the current object.
                //

                WaitBlock = &WaitBlockArray[Index];
                WaitBlock->Object = (PVOID)Objectx;
                WaitBlock->WaitKey = (CSHORT)(Index);
                WaitBlock->WaitType = (USHORT)WaitType;
                WaitBlock->Thread = Thread;
                WaitBlock->NextWaitBlock = &WaitBlockArray[Index + 1];
            }

            //
            // If the wait type is wait all, then check to determine if the
            // wait can be satisfied immediately.
            //

            if ((WaitType == WaitAll) && (WaitSatisfied)) {
                WaitBlock->NextWaitBlock = &WaitBlockArray[0];
                KiWaitSatisfyAll(WaitBlock);
                WaitStatus = (NTSTATUS)Thread->WaitStatus;
                goto NoWait;
            }

            //
            // Test for alert pending.
            //

            TestForAlertPending(Alertable);

            //
            // The wait cannot be satisifed immediately. Check to determine if
            // a timeout value is specified.
            //

            if (ARGUMENT_PRESENT(Timeout)) {

                //
                // If the timeout value is zero, then return immediately without
                // waiting.
                //

                if (!(Timeout->LowPart | Timeout->HighPart)) {
                    WaitStatus = (NTSTATUS)(STATUS_TIMEOUT);
                    goto NoWait;
                }

                //
                // Initialize a wait block for the thread specific timer,
                // initialize timer wait list head, insert the timer in the
                // timer tree, and increment the number of wait objects.
                //
                // N.B. The timer wait block is initialized when the respective
                //      thread is initialized. Thus the constant fields are not
                //      reinitialized. These include the wait object, wait key,
                //      wait type, and the wait list entry link pointers.
                //

                WaitTimer = &Thread->WaitBlock[TIMER_WAIT_BLOCK];
                WaitBlock->NextWaitBlock = WaitTimer;
                WaitBlock = WaitTimer;
                Timer = &Thread->Timer;
                InitializeListHead(&Timer->Header.WaitListHead);
                if (KiInsertTreeTimer(Timer, *Timeout) == FALSE) {
                    WaitStatus = (NTSTATUS)STATUS_TIMEOUT;
                    goto NoWait;
                }

                DueTime.QuadPart = Timer->DueTime.QuadPart;
            }

            //
            // Close up the circular list of wait control blocks.
            //

            WaitBlock->NextWaitBlock = &WaitBlockArray[0];

            //
            // Insert wait blocks in object wait lists.
            //

            WaitBlock = &WaitBlockArray[0];
            do {
                Objectx = (PKMUTANT)WaitBlock->Object;
                InsertTailList(&Objectx->Header.WaitListHead, &WaitBlock->WaitListEntry);
                WaitBlock = WaitBlock->NextWaitBlock;
            } while (WaitBlock != &WaitBlockArray[0]);

            //
            // If the current thread is processing a queue entry, then attempt
            // to activate another thread that is blocked on the queue object.
            //

            Queue = Thread->Queue;
            if (Queue != NULL) {
                KiActivateWaiterQueue(Queue);
            }

            //
            // Set the thread wait parameters, set the thread dispatcher state
            // to Waiting, and insert the thread in the wait list.
            //

            Thread->Alertable = Alertable;
            Thread->WaitMode = WaitMode;
            Thread->WaitReason = (UCHAR)WaitReason;
            Thread->WaitTime= KiQueryLowTickCount();
            Thread->State = Waiting;
            KiInsertWaitList(WaitMode, Thread);

            //
            // Switch context to selected thread.
            //
            // Control is returned at the original IRQL.
            //

            ASSERT(Thread->WaitIrql <= DISPATCH_LEVEL);

            WaitStatus = (NTSTATUS)KiSwapThread();

            //
            // If the thread was not awakened to deliver a kernel mode APC,
            // then the wait status.
            //

            if (WaitStatus != STATUS_KERNEL_APC) {
                return WaitStatus;
            }

            if (ARGUMENT_PRESENT(Timeout)) {

                //
                // Reduce the amount of time remaining before timeout occurs.
                //

                Timeout = KiComputeWaitInterval(OriginalTime,
                                                &DueTime,
                                                &NewTime);
            }
        }

        //
        // Raise IRQL to DISPATCH_LEVEL and lock the dispatcher database.
        //

        KiLockDispatcherDatabase(&Thread->WaitIrql);
    } while (TRUE);

    //
    // The thread is alerted or a user APC should be delivered. Unlock the
    // dispatcher database, lower IRQL to its previous value, and return
    // the wait status.
    //

    KiUnlockDispatcherDatabase(Thread->WaitIrql);
    return WaitStatus;

    //
    // The wait has been satisfied without actually waiting.
    //
    // If the thread priority that is less than time critical, then reduce
    // the thread quantum. If a quantum end occurs, then reduce the thread
    // priority.
    //

NoWait:
    KiAdjustQuantumThread(Thread);

    //
    // Unlock the dispatcher database, lower IRQL to its previous value, and
    // return the wait status.
    //

    KiUnlockDispatcherDatabase(Thread->WaitIrql);
    return WaitStatus;
}

NTSTATUS
KeWaitForSingleObject (
    IN PVOID Object,
    IN KWAIT_REASON WaitReason,
    IN KPROCESSOR_MODE WaitMode,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
    )

/*++

Routine Description:

    This function waits until the specified object attains a state of
    Signaled. An optional timeout can also be specified. If a timeout
    is not specified, then the wait will not be satisfied until the object
    attains a state of Signaled. If a timeout is specified, and the object
    has not attained a state of Signaled when the timeout expires, then
    the wait is automatically satisfied. If an explicit timeout value of
    zero is specified, then no wait will occur if the wait cannot be satisfied
    immediately. The wait can also be specified as alertable.

Arguments:

    Object - Supplies a pointer to a dispatcher object.

    WaitReason - Supplies the reason for the wait.

    WaitMode  - Supplies the processor mode in which the wait is to occur.

    Alertable - Supplies a boolean value that specifies whether the wait is
        alertable.

    Timeout - Supplies a pointer to an optional absolute of relative time over
        which the wait is to occur.

Return Value:

    The wait completion status. A value of STATUS_TIMEOUT is returned if a
    timeout occurred. A value of STATUS_SUCCESS is returned if the specified
    object satisfied the wait. A value of STATUS_ALERTED is returned if the
    wait was aborted to deliver an alert to the current thread. A value of
    STATUS_USER_APC is returned if the wait was aborted to deliver a user
    APC to the current thread.

--*/

{

    LARGE_INTEGER DueTime;
    LARGE_INTEGER NewTime;
    PRKTHREAD NextThread;
    PKMUTANT Objectx;
    PLARGE_INTEGER OriginalTime;
    PRKQUEUE Queue;
    PRKTHREAD Thread;
    PRKTIMER Timer;
    PKWAIT_BLOCK WaitBlock;
    NTSTATUS WaitStatus;
    PKWAIT_BLOCK WaitTimer;

    //
    // Collect call data.
    //

#if defined(_COLLECT_WAIT_SINGLE_CALLDATA_)

    RECORD_CALL_DATA(&KiWaitSingleCallData);

#endif

    //
    // If the dispatcher database lock is not already held, then set the wait
    // IRQL and lock the dispatcher database. Else set boolean wait variable
    // to FALSE.
    //

    Thread = KeGetCurrentThread();
    if (Thread->WaitNext) {
        Thread->WaitNext = FALSE;

    } else {
        KiLockDispatcherDatabase(&Thread->WaitIrql);
    }

    //
    // Start of wait loop.
    //
    // Note this loop is repeated if a kernel APC is delivered in the middle
    // of the wait or a kernel APC is pending on the first attempt through
    // the loop.
    //

    OriginalTime = Timeout;
    WaitBlock = &Thread->WaitBlock[0];
    do {

        //
        // Test to determine if a kernel APC is pending.
        //
        // If a kernel APC is pending and the previous IRQL was less than
        // APC_LEVEL, then a kernel APC was queued by another processor just
        // after IRQL was raised to DISPATCH_LEVEL, but before the dispatcher
        // database was locked.
        //
        // N.B. that this can only happen in a multiprocessor system.
        //

        if (Thread->ApcState.KernelApcPending && (Thread->WaitIrql < APC_LEVEL)) {

            //
            // Unlock the dispatcher database and lower IRQL to its previous
            // value. An APC interrupt will immediately occur which will result
            // in the delivery of the kernel APC if possible.
            //

            KiUnlockDispatcherDatabase(Thread->WaitIrql);

        } else {

            //
            // Test if the wait can be immediately satisfied.
            //

            Objectx = (PKMUTANT)Object;
            Thread->WaitStatus = (NTSTATUS)0;

            ASSERT(Objectx->Header.Type != QueueObject);

            //
            // If the object is a mutant object and the mutant object has been
            // recursively acquired MINLONG times, then raise an exception.
            // Otherwise if the signal state of the mutant object is greater
            // than zero, or the current thread is the owner of the mutant
            // object, then satisfy the wait.
            //

            if (Objectx->Header.Type == MutantObject) {
                if ((Objectx->Header.SignalState > 0) ||
                    (Thread == Objectx->OwnerThread)) {
                    if (Objectx->Header.SignalState != MINLONG) {
                        KiWaitSatisfyMutant(Objectx, Thread);
                        WaitStatus = (NTSTATUS)(Thread->WaitStatus);
                        goto NoWait;

                    } else {
                        KiUnlockDispatcherDatabase(Thread->WaitIrql);
                        ExRaiseStatus(STATUS_MUTANT_LIMIT_EXCEEDED);
                    }
                }

            //
            // If the signal state is greater than zero, then satisfy the wait.
            //

            } else if (Objectx->Header.SignalState > 0) {
                KiWaitSatisfyOther(Objectx);
                WaitStatus = (NTSTATUS)(0);
                goto NoWait;
            }

            //
            // Construct a wait block for the object.
            //

            Thread->WaitBlockList = WaitBlock;
            WaitBlock->Object = Object;
            WaitBlock->WaitKey = (CSHORT)(STATUS_SUCCESS);
            WaitBlock->WaitType = WaitAny;

            //
            // Test for alert pending.
            //

            TestForAlertPending(Alertable);

            //
            // The wait cannot be satisifed immediately. Check to determine if
            // a timeout value is specified.
            //

            if (ARGUMENT_PRESENT(Timeout)) {

                //
                // If the timeout value is zero, then return immediately without
                // waiting.
                //

                if (!(Timeout->LowPart | Timeout->HighPart)) {
                    WaitStatus = (NTSTATUS)(STATUS_TIMEOUT);
                    goto NoWait;
                }

                //
                // Initialize a wait block for the thread specific timer, insert
                // wait block in timer wait list, insert the timer in the timer
                // tree.
                //
                // N.B. The timer wait block is initialized when the respective
                //      thread is initialized. Thus the constant fields are not
                //      reinitialized. These include the wait object, wait key,
                //      wait type, and the wait list entry link pointers.
                //

                Timer = &Thread->Timer;
                WaitTimer = &Thread->WaitBlock[TIMER_WAIT_BLOCK];
                WaitBlock->NextWaitBlock = WaitTimer;
                Timer->Header.WaitListHead.Flink = &WaitTimer->WaitListEntry;
                Timer->Header.WaitListHead.Blink = &WaitTimer->WaitListEntry;
                WaitTimer->NextWaitBlock = WaitBlock;
                if (KiInsertTreeTimer(Timer, *Timeout) == FALSE) {
                    WaitStatus = (NTSTATUS)STATUS_TIMEOUT;
                    goto NoWait;
                }

                DueTime.QuadPart = Timer->DueTime.QuadPart;

            } else {
                WaitBlock->NextWaitBlock = WaitBlock;
            }

            //
            // Insert wait block in object wait list.
            //

            InsertTailList(&Objectx->Header.WaitListHead, &WaitBlock->WaitListEntry);

            //
            // If the current thread is processing a queue entry, then attempt
            // to activate another thread that is blocked on the queue object.
            //

            Queue = Thread->Queue;
            if (Queue != NULL) {
                KiActivateWaiterQueue(Queue);
            }

            //
            // Set the thread wait parameters, set the thread dispatcher state
            // to Waiting, and insert the thread in the wait list.
            //

            Thread->Alertable = Alertable;
            Thread->WaitMode = WaitMode;
            Thread->WaitReason = (UCHAR)WaitReason;
            Thread->WaitTime= KiQueryLowTickCount();
            Thread->State = Waiting;
            KiInsertWaitList(WaitMode, Thread);

            //
            // Switch context to selected thread.
            //
            // Control is returned at the original IRQL.
            //

            ASSERT(Thread->WaitIrql <= DISPATCH_LEVEL);

            WaitStatus = (NTSTATUS)KiSwapThread();

            //
            // If the thread was not awakened to deliver a kernel mode APC,
            // then return wait status.
            //

            if (WaitStatus != STATUS_KERNEL_APC) {
                return WaitStatus;
            }

            if (ARGUMENT_PRESENT(Timeout)) {

                //
                // Reduce the amount of time remaining before timeout occurs.
                //

                Timeout = KiComputeWaitInterval(OriginalTime,
                                                &DueTime,
                                                &NewTime);
            }
        }

        //
        // Raise IRQL to DISPATCH_LEVEL and lock the dispatcher database.
        //

        KiLockDispatcherDatabase(&Thread->WaitIrql);
    } while (TRUE);

    //
    // The thread is alerted or a user APC should be delivered. Unlock the
    // dispatcher database, lower IRQL to its previous value, and return
    // the wait status.
    //

    KiUnlockDispatcherDatabase(Thread->WaitIrql);
    return WaitStatus;

    //
    // The wait has been satisfied without actually waiting.
    //
    // If the thread priority that is less than time critical, then reduce
    // the thread quantum. If a quantum end occurs, then reduce the thread
    // priority.
    //

NoWait:
    KiAdjustQuantumThread(Thread);

    //
    // Unlock the dispatcher database, lower IRQL to its previous value, and
    // return the wait status.
    //

    KiUnlockDispatcherDatabase(Thread->WaitIrql);
    return WaitStatus;
}

NTSTATUS
KiSetServerWaitClientEvent (
    IN PKEVENT ServerEvent,
    IN PKEVENT ClientEvent,
    IN ULONG WaitMode
    )

/*++

Routine Description:

    This function sets the specified server event and waits on specified
    client event. The wait is performed such that an optimal switch to
    the waiting thread occurs if possible. No timeout is associated with
    the wait, and thus, the issuing thread will wait until the client event
    is signaled or an APC is delivered.

Arguments:

    ServerEvent - Supplies a pointer to a dispatcher object of type event.

    ClientEvent - Supplies a pointer to a dispatcher object of type event.

    WaitMode  - Supplies the processor mode in which the wait is to occur.

Return Value:

    The wait completion status. A value of STATUS_SUCCESS is returned if
    the specified object satisfied the wait. A value of STATUS_USER_APC is
    returned if the wait was aborted to deliver a user APC to the current
    thread.

--*/

{

    //
    // Set sever event and wait on client event atomically.
    //

    KeSetEvent(ServerEvent, EVENT_INCREMENT, TRUE);
    return KeWaitForSingleObject(ClientEvent,
                                 WrEventPair,
                                 (KPROCESSOR_MODE)WaitMode,
                                 FALSE,
                                 NULL);
}

PLARGE_INTEGER
FASTCALL
KiComputeWaitInterval (
    IN PLARGE_INTEGER OriginalTime,
    IN PLARGE_INTEGER DueTime,
    IN OUT PLARGE_INTEGER NewTime
    )

/*++

Routine Description:

    This function recomputes the wait interval after a thread has been
    awakened to deliver a kernel APC.

Arguments:

    OriginalTime - Supplies a pointer to the original timeout value.

    DueTime - Supplies a pointer to the previous due time.

    NewTime - Supplies a pointer to a variable that receives the
        recomputed wait interval.

Return Value:

    A pointer to the new time is returned as the function value.

--*/

{

    //
    // If the original wait time was absolute, then return the same
    // absolute time. Otherwise, reduce the wait time remaining before
    // the time delay expires.
    //

    if (OriginalTime->QuadPart >= 0) {
        return OriginalTime;

    } else {
        KiQueryInterruptTime(NewTime);
        NewTime->QuadPart -= DueTime->QuadPart;
        return NewTime;
    }
}
