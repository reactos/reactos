/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    queueobj.c

Abstract:

    This module implements the kernel queue object. Functions are
    provided to initialize, read, insert, and remove queue objects.

Author:

    David N. Cutler (davec) 31-Dec-1993

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

//
// The following assert macro is used to check that an input event is
// really a kernel event and not something else, like deallocated pool.
//

#define ASSERT_QUEUE(Q) ASSERT((Q)->Header.Type == QueueObject);

VOID
KeInitializeQueue (
    IN PRKQUEUE Queue,
    IN ULONG Count OPTIONAL
    )

/*++

Routine Description:

    This function initializes a kernel queue object.

Arguments:

    Queue - Supplies a pointer to a dispatcher object of type event.

    Count - Supplies the target maximum number of threads that should
        be concurrently active. If this parameter is not specified,
        then the number of processors is used.

Return Value:

    None.

--*/

{

    //
    // Initialize standard dispatcher object header and set initial
    // state of queue object.
    //

    Queue->Header.Type = QueueObject;
    Queue->Header.Size = sizeof(KQUEUE) / sizeof(LONG);
    Queue->Header.SignalState = 0;
    InitializeListHead(&Queue->Header.WaitListHead);

    //
    // Initialize queue listhead, the thread list head, the current number
    // of threads, and the target maximum number of threads.
    //

    InitializeListHead(&Queue->EntryListHead);
    InitializeListHead(&Queue->ThreadListHead);
    Queue->CurrentCount = 0;
    if (ARGUMENT_PRESENT((PVOID)(ULONG_PTR)Count)) {
        Queue->MaximumCount = Count;

    } else {
        Queue->MaximumCount = KeNumberProcessors;
    }

    return;
}

LONG
KeReadStateQueue (
    IN PRKQUEUE Queue
    )

/*++

Routine Description:

    This function reads the current signal state of a Queue object.

Arguments:

    Queue - Supplies a pointer to a dispatcher object of type Queue.

Return Value:

    The current signal state of the Queue object.

--*/

{

    ASSERT_QUEUE(Queue);

    //
    // Return current signal state of Queue object.
    //

    return Queue->Header.SignalState;
}

LONG
KeInsertQueue (
    IN PRKQUEUE Queue,
    IN PLIST_ENTRY Entry
    )

/*++

Routine Description:

    This function inserts the specified entry in the queue object entry
    list and attempts to satisfy the wait of a single waiter.

    N.B. The wait discipline for Queue object is LIFO.

Arguments:

    Queue - Supplies a pointer to a dispatcher object of type Queue.

    Entry - Supplies a pointer to a list entry that is inserted in the
        queue object entry list.

Return Value:

    The previous signal state of the Queue object.

--*/

{

    KIRQL OldIrql;
    LONG OldState;

    ASSERT_QUEUE(Queue);
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    //
    // Raise IRQL to dispatcher level and lock dispatcher database.
    //

    KiLockDispatcherDatabase(&OldIrql);

    //
    // Insert the specified entry in the queue object entry list.
    //

    OldState = KiInsertQueue(Queue, Entry, FALSE);

    //
    // Unlock the dispather database, lower IRQL to the previous level, and
    // return signal state of Queue object.
    //

    KiUnlockDispatcherDatabase(OldIrql);
    return OldState;
}

LONG
KeInsertHeadQueue (
    IN PRKQUEUE Queue,
    IN PLIST_ENTRY Entry
    )

/*++

Routine Description:

    This function inserts the specified entry in the queue object entry
    list and attempts to satisfy the wait of a single waiter.

    N.B. The wait discipline for Queue object is LIFO.

Arguments:

    Queue - Supplies a pointer to a dispatcher object of type Queue.

    Entry - Supplies a pointer to a list entry that is inserted in the
        queue object entry list.

Return Value:

    The previous signal state of the Queue object.

--*/

{

    KIRQL OldIrql;
    LONG OldState;

    ASSERT_QUEUE(Queue);
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    //
    // Raise IRQL to dispatcher level and lock dispatcher database.
    //

    KiLockDispatcherDatabase(&OldIrql);

    //
    // Insert the specified entry in the queue object entry list.
    //

    OldState = KiInsertQueue(Queue, Entry, TRUE);

    //
    // Unlock the dispather database, lower IRQL to the previous level, and
    // return signal state of Queue object.
    //

    KiUnlockDispatcherDatabase(OldIrql);
    return OldState;
}

PLIST_ENTRY
KeRemoveQueue (
    IN PRKQUEUE Queue,
    IN KPROCESSOR_MODE WaitMode,
    IN PLARGE_INTEGER Timeout OPTIONAL
    )

/*++

Routine Description:

    This function removes the next entry from the Queue object entry
    list. If no list entry is available, then the calling thread is
    put in a wait state.

    N.B. The wait discipline for Queue object LIFO.

Arguments:

    Queue - Supplies a pointer to a dispatcher object of type Queue.

    WaitMode  - Supplies the processor mode in which the wait is to occur.

    Timeout - Supplies a pointer to an optional absolute of relative time over
        which the wait is to occur.

Return Value:

    The address of the entry removed from the Queue object entry list or
    STATUS_TIMEOUT.

    N.B. These values can easily be distinguished by the fact that all
         addresses in kernel mode have the high order bit set.

--*/

{

    LARGE_INTEGER DueTime;
    PLIST_ENTRY Entry;
    PRKTHREAD NextThread;
    LARGE_INTEGER NewTime;
    KIRQL OldIrql;
    PRKQUEUE OldQueue;
    PLARGE_INTEGER OriginalTime;
    PRKTHREAD Thread;
    PRKTIMER Timer;
    PRKWAIT_BLOCK WaitBlock;
    LONG_PTR WaitStatus;

    ASSERT_QUEUE(Queue);
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    //
    // If the dispatcher database lock is not already held, then set the wait
    // IRQL and lock the dispatcher database. Else set boolean wait variable
    // to FALSE.
    //

    Thread = KeGetCurrentThread();
    if (Thread->WaitNext) {
        Thread->WaitNext = FALSE;

    } else {
        KiLockDispatcherDatabase(&OldIrql);
        Thread->WaitIrql = OldIrql;
    }

    //
    // Check if the thread is currently processing a queue entry and whether
    // the new queue is the same as the old queue.
    //

    OldQueue = Thread->Queue;
    Thread->Queue = Queue;
    if (Queue != OldQueue) {

        //
        // If the thread was previously associated with a queue, then remove
        // the thread from the old queue object thread list and attempt to
        // activate another thread.
        //

        Entry = &Thread->QueueListEntry;
        if (OldQueue != NULL) {
            RemoveEntryList(Entry);
            KiActivateWaiterQueue(OldQueue);
        }

        //
        // Insert thread in the thread list of the new queue that the thread
        // will be associate with.
        //

        InsertTailList(&Queue->ThreadListHead, Entry);

    } else {

        //
        // The previous and current queue are the same queue - decrement the
        // current number of threads.
        //

        Queue->CurrentCount -= 1;
    }

    //
    //
    // Start of wait loop.
    //
    //
    // Note this loop is repeated if a kernel APC is delivered in the
    // middle of the wait or a kernel APC is pending on the first attempt
    // through the loop.
    //
    // If the Queue object entry list is not empty, then remove the next
    // entry from the Queue object entry list. Otherwise, wait for an entry
    // to be inserted in the queue.
    //

    OriginalTime = Timeout;
    do {

        //
        // Check if there is a queue entry available and the current
        // number of active threads is less than target maximum number
        // of threads.
        //

        Entry = Queue->EntryListHead.Flink;
        if ((Entry != &Queue->EntryListHead) &&
            (Queue->CurrentCount < Queue->MaximumCount)) {

            //
            // Decrement the number of entires in the Queue object entry list,
            // increment the number of active threads, remove the next entry
            // rom the list, and set the forward link to NULL.
            //

            Queue->Header.SignalState -= 1;
            Queue->CurrentCount += 1;
            if ((Entry->Flink == NULL) || (Entry->Blink == NULL)) {
                KeBugCheckEx(INVALID_WORK_QUEUE_ITEM,
                             (ULONG_PTR)Entry,
                             (ULONG_PTR)Queue,
                             (ULONG_PTR)&ExWorkerQueue[0],
                             (ULONG_PTR)((PWORK_QUEUE_ITEM)Entry)->WorkerRoutine);
            }

            RemoveEntryList(Entry);
            Entry->Flink = NULL;
            break;

        } else {

            //
            // Set address of wait block list in thread object.
            //

            Thread->WaitBlockList = &Thread->WaitBlock[0];

            //
            // Test to determine if a kernel APC is pending.
            //
            // If a kernel APC is pending and the previous IRQL was less than
            // APC_LEVEL, then a kernel APC was queued by another processor
            // just after IRQL was raised to DISPATCH_LEVEL, but before the
            // dispatcher database was locked.
            //
            // N.B. that this can only happen in a multiprocessor system.
            //

            if (Thread->ApcState.KernelApcPending && (Thread->WaitIrql < APC_LEVEL)) {

                //
                // Increment the current thread count, unlock the dispatcher
                // database, and lower IRQL to previous value. An APC interrupt
                // will immediately occur which will result in the delivery of
                // the kernel APC if possible.
                //

                Queue->CurrentCount += 1;
                KiUnlockDispatcherDatabase(Thread->WaitIrql);

            } else {

                //
                // Test if a user APC is pending.
                //

                if ((WaitMode != KernelMode) && (Thread->ApcState.UserApcPending)) {
                    Entry = (PLIST_ENTRY)ULongToPtr(STATUS_USER_APC);
                    Queue->CurrentCount += 1;
                    break;
                }

                //
                // Construct a wait block and check to determine if the wait
                // is already satisfied. If the wait is satisfied, then perform
                // wait completion and return. Else put current thread in a
                // wait state if an explicit timeout value of zero is not
                // specified.
                //

                Thread->WaitStatus = (NTSTATUS)0;
                WaitBlock = &Thread->WaitBlock[0];
                WaitBlock->Object = (PVOID)Queue;
                WaitBlock->WaitKey = (CSHORT)(STATUS_SUCCESS);
                WaitBlock->WaitType = WaitAny;
                WaitBlock->Thread = Thread;

                //
                // Check to determine if a timeout value is specified.
                //

                if (ARGUMENT_PRESENT(Timeout)) {

                    //
                    // If the timeout value is zero, then return immediately
                    // without waiting.
                    //

                    if (!(Timeout->LowPart | Timeout->HighPart)) {
                        Entry = (PLIST_ENTRY)ULongToPtr(STATUS_TIMEOUT);
                        Queue->CurrentCount += 1;
                        break;
                    }

                    //
                    // Initialize a wait block for the thread specific timer,
                    // insert wait block in timer wait list, insert the timer
                    // in the timer tree.
                    //

                    Timer = &Thread->Timer;
                    WaitBlock->NextWaitBlock = &Thread->WaitBlock[1];
                    WaitBlock = &Thread->WaitBlock[1];
                    WaitBlock->Object = (PVOID)Timer;
                    WaitBlock->WaitKey = (CSHORT)(STATUS_TIMEOUT);
                    WaitBlock->WaitType = WaitAny;
                    WaitBlock->Thread = Thread;
                    Timer->Header.WaitListHead.Flink = &WaitBlock->WaitListEntry;
                    Timer->Header.WaitListHead.Blink = &WaitBlock->WaitListEntry;
                    WaitBlock->WaitListEntry.Flink = &Timer->Header.WaitListHead;
                    WaitBlock->WaitListEntry.Blink = &Timer->Header.WaitListHead;
                    if (KiInsertTreeTimer(Timer, *Timeout) == FALSE) {
                        Entry = (PLIST_ENTRY)ULongToPtr(STATUS_TIMEOUT);
                        Queue->CurrentCount += 1;
                        break;
                    }

                    DueTime.QuadPart = Timer->DueTime.QuadPart;
                }

                //
                // Close up the circular list of wait control blocks.
                //

                WaitBlock->NextWaitBlock = &Thread->WaitBlock[0];

                //
                // Insert wait block in object wait list.
                //

                WaitBlock = &Thread->WaitBlock[0];
                InsertTailList(&Queue->Header.WaitListHead, &WaitBlock->WaitListEntry);

                //
                // Set the thread wait parameters, set the thread dispatcher
                // state to Waiting, and insert the thread in the wait list.
                //

                Thread->Alertable = FALSE;
                Thread->WaitMode = WaitMode;
                Thread->WaitReason = WrQueue;
                Thread->WaitTime = KiQueryLowTickCount();
                Thread->State = Waiting;
                KiInsertWaitList(WaitMode, Thread);

                //
                // Switch context to selected thread.
                //
                // Control is returned at the original IRQL.
                //

                ASSERT(Thread->WaitIrql <= DISPATCH_LEVEL);

                WaitStatus = KiSwapThread();

                //
                // If the thread was not awakened to deliver a kernel mode APC,
                // then return wait status.
                //

                Thread->WaitReason = 0;
                if (WaitStatus != STATUS_KERNEL_APC) {
                    return (PLIST_ENTRY)WaitStatus;
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
            // Raise IRQL to DISPATCH_LEVEL, lock the dispatcher database,
            // and decrement the count of active threads.
            //

            KiLockDispatcherDatabase(&OldIrql);
            Thread->WaitIrql = OldIrql;
            Queue->CurrentCount -= 1;
        }

    } while (TRUE);

    //
    // Unlock the dispatcher database and return the list entry address or a
    // status of timeout.
    //

    KiUnlockDispatcherDatabase(Thread->WaitIrql);
    return Entry;
}

PLIST_ENTRY
KeRundownQueue (
    IN PRKQUEUE Queue
    )

/*++

Routine Description:

    This function runs down the specified queue by removing the listhead
    from the queue list, removing any associated threads from the thread
    list, and returning the address of the first entry.


Arguments:

    Queue - Supplies a pointer to a dispatcher object of type Queue.

Return Value:

    If the queue list is not empty, then the address of the first entry in
    the queue is returned as the function value. Otherwise, a value of NULL
    is returned.

--*/

{

    PLIST_ENTRY Entry;
    PLIST_ENTRY FirstEntry;
    KIRQL OldIrql;
    PKTHREAD Thread;

    ASSERT_QUEUE(Queue);
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    //
    // Raise IRQL to dispatch level and lock the dispatcher database.
    //

    KiLockDispatcherDatabase(&OldIrql);

    //
    // Get the address of the first entry in the queue and check if the
    // list is empty or contains entries that should be flushed. If there
    // are no entries in the list, then set the return value to NULL.
    // Otherwise, set the return value to the address of the first list
    // entry and remove the listhead from the list.
    //

    FirstEntry = Queue->EntryListHead.Flink;
    if (FirstEntry == &Queue->EntryListHead) {
        FirstEntry = NULL;

    } else {
        RemoveEntryList(&Queue->EntryListHead);
    }

    //
    // Remove all associated threads from the thread list of the queue.
    //

    while (Queue->ThreadListHead.Flink != &Queue->ThreadListHead) {
        Entry = Queue->ThreadListHead.Flink;
        Thread = CONTAINING_RECORD(Entry, KTHREAD, QueueListEntry);
        Thread->Queue = NULL;
        RemoveEntryList(Entry);
    }

    //
    // Unlock the dispatcher database, lower IRQL to its previous level,
    // and return the function value.
    //

    KiUnlockDispatcherDatabase(OldIrql);
    return FirstEntry;
}

VOID
FASTCALL
KiActivateWaiterQueue (
    IN PRKQUEUE Queue
    )

/*++

Routine Description:

    This function is called when the current thread is about to enter a
    wait state and is currently processing a queue entry. The current
    number of threads processign entries for the queue is decrement and
    an attempt is made to activate another thread if the current count
    is less than the maximum count, there is a waiting thread, and the
    queue is not empty.

Arguments:

    Queue - Supplies a pointer to a dispatcher object of type event.

Return Value:

    None.

--*/

{

    PRLIST_ENTRY Entry;
    PRKTHREAD Thread;
    PRKWAIT_BLOCK WaitBlock;
    PRLIST_ENTRY WaitEntry;

    //
    // Decrement the current count of active threads and check if another
    // thread can be activated. If the current number of active threads is
    // less than the target maximum number of threads, there is a entry in
    // in the queue, and a thread is waiting, then remove the entry from the
    // queue, decrement the number of entries in the queue, and unwait the
    // respectiive thread.
    //

    Queue->CurrentCount -= 1;
    if (Queue->CurrentCount < Queue->MaximumCount) {
        Entry = Queue->EntryListHead.Flink;
        WaitEntry = Queue->Header.WaitListHead.Blink;
        if ((Entry != &Queue->EntryListHead) &&
            (WaitEntry != &Queue->Header.WaitListHead)) {
            RemoveEntryList(Entry);
            Entry->Flink = NULL;
            Queue->Header.SignalState -= 1;
            WaitBlock = CONTAINING_RECORD(WaitEntry, KWAIT_BLOCK, WaitListEntry);
            Thread = WaitBlock->Thread;
            KiUnwaitThread(Thread, (LONG_PTR)Entry, 0);
        }
    }

    return;
}

LONG
FASTCALL
KiInsertQueue (
    IN PRKQUEUE Queue,
    IN PLIST_ENTRY Entry,
    IN BOOLEAN Head
    )

/*++

Routine Description:

    This function inserts the specified entry in the queue object entry
    list and attempts to satisfy the wait of a single waiter.

    N.B. The wait discipline for Queue object is LIFO.

Arguments:

    Queue - Supplies a pointer to a dispatcher object of type Queue.

    Entry - Supplies a pointer to a list entry that is inserted in the
        queue object entry list.

    Head - Supplies a boolean value that determines whether the queue
        entry is inserted at the head or tail of the queue if it can
        not be immediately dispatched.

Return Value:

    The previous signal state of the Queue object.

--*/

{

    LONG OldState;
    PRKTHREAD Thread;
    PKTIMER Timer;
    PKWAIT_BLOCK WaitBlock;
    PLIST_ENTRY WaitEntry;

    ASSERT_QUEUE(Queue);

    //
    // Capture the current signal state of queue object and check if there
    // is a thread waiting on the queue object, the current number of active
    // threads is less than the target number of threads, and the wait reason
    // of the current thread is not queue wait or the wait queue is not the
    // same queue as the insertion queue. If these conditions are satisfied,
    // then satisfy the thread wait and pass the thread the address of the
    // queue entry as the wait status. Otherwise, set the state of the queue
    // object to signaled and insert the specified entry in the queue object
    // entry list.
    //

    OldState = Queue->Header.SignalState;
    Thread = KeGetCurrentThread();
    WaitEntry = Queue->Header.WaitListHead.Blink;
    if ((WaitEntry != &Queue->Header.WaitListHead) &&
        (Queue->CurrentCount < Queue->MaximumCount) &&
        ((Thread->Queue != Queue) ||
        (Thread->WaitReason != WrQueue))) {

        //
        // Remove the last wait block from the wait list and get the address
        // of the waiting thread object.
        //

        RemoveEntryList(WaitEntry);
        WaitBlock = CONTAINING_RECORD(WaitEntry, KWAIT_BLOCK, WaitListEntry);
        Thread = WaitBlock->Thread;

        //
        // Set the wait completion status, remove the thread from its wait
        // list, increment the number of active threads, and clear the wait
        // reason.
        //

        Thread->WaitStatus = (LONG_PTR)Entry;
        RemoveEntryList(&Thread->WaitListEntry);
        Queue->CurrentCount += 1;
        Thread->WaitReason = 0;

        //
        // If thread timer is still active, then cancel thread timer.
        //

        Timer = &Thread->Timer;
        if (Timer->Header.Inserted == TRUE) {
            KiRemoveTreeTimer(Timer);
        }

        //
        // Ready the thread for execution.
        //

        KiReadyThread(Thread);

    } else {
        Queue->Header.SignalState += 1;
        if (Head != FALSE) {
            InsertHeadList(&Queue->EntryListHead, Entry);

        } else {
            InsertTailList(&Queue->EntryListHead, Entry);
        }
    }

    return OldState;
}
