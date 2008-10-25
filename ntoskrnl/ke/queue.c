/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/queue.c
 * PURPOSE:         Implements kernel queues
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Gunnar Dalsnes
 *                  Eric Kohl (ekohl@rz-online.de)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

/*
 * Called when a thread which has a queue entry is entering a wait state
 */
VOID
FASTCALL
KiActivateWaiterQueue(IN PKQUEUE Queue)
{
    PLIST_ENTRY QueueEntry;
    PLIST_ENTRY WaitEntry;
    PKWAIT_BLOCK WaitBlock;
    PKTHREAD Thread;
    ASSERT_QUEUE(Queue);

    /* Decrement the number of active threads */
    Queue->CurrentCount--;

    /* Make sure the counts are OK */
    if (Queue->CurrentCount < Queue->MaximumCount)
    {
        /* Get the Queue Entry */
        QueueEntry = Queue->EntryListHead.Flink;

        /* Get the Wait Entry */
        WaitEntry = Queue->Header.WaitListHead.Blink;

        /* Make sure that the Queue entries are not part of empty lists */
        if ((WaitEntry != &Queue->Header.WaitListHead) &&
            (QueueEntry != &Queue->EntryListHead))
        {
            /* Remove this entry */
            RemoveEntryList(QueueEntry);
            QueueEntry->Flink = NULL;

            /* Decrease the Signal State */
            Queue->Header.SignalState--;

            /* Unwait the Thread */
            WaitBlock = CONTAINING_RECORD(WaitEntry,
                                          KWAIT_BLOCK,
                                          WaitListEntry);
            Thread = WaitBlock->Thread;
            KiUnwaitThread(Thread, (NTSTATUS)QueueEntry, IO_NO_INCREMENT);
        }
    }
}

/*
 * Returns the previous number of entries in the queue
 */
LONG
NTAPI
KiInsertQueue(IN PKQUEUE Queue,
              IN PLIST_ENTRY Entry,
              IN BOOLEAN Head)
{
    ULONG InitialState;
    PKTHREAD Thread = KeGetCurrentThread();
    PKWAIT_BLOCK WaitBlock;
    PLIST_ENTRY WaitEntry;
    PKTIMER Timer;
    ASSERT_QUEUE(Queue);

    /* Save the old state */
    InitialState = Queue->Header.SignalState;

    /* Get the Entry */
    WaitEntry = Queue->Header.WaitListHead.Blink;

    /*
     * Why the KeGetCurrentThread()->Queue != Queue?
     * KiInsertQueue might be called from an APC for the current thread.
     * -Gunnar
     */
    if ((Queue->CurrentCount < Queue->MaximumCount) &&
        (WaitEntry != &Queue->Header.WaitListHead) &&
        ((Thread->Queue != Queue) ||
         (Thread->WaitReason != WrQueue)))
    {
        /* Remove the wait entry */
        RemoveEntryList(WaitEntry);

        /* Get the Wait Block and Thread */
        WaitBlock = CONTAINING_RECORD(WaitEntry, KWAIT_BLOCK, WaitListEntry);
        Thread = WaitBlock->Thread;

        /* Remove the queue from the thread's wait list */
        Thread->WaitStatus = (NTSTATUS)Entry;
        if (Thread->WaitListEntry.Flink) RemoveEntryList(&Thread->WaitListEntry);

        /* Increase the active threads and remove any wait reason */
        Queue->CurrentCount++;
        Thread->WaitReason = 0;

        /* Check if there's a Thread Timer */
        Timer = &Thread->Timer;
        if (Timer->Header.Inserted) KxRemoveTreeTimer(Timer);

        /* Reschedule the Thread */
        KiReadyThread(Thread);
    }
    else
    {
        /* Increase the Entries */
        Queue->Header.SignalState++;

        /* Check which mode we're using */
        if (Head)
        {
            /* Insert in the head */
            InsertHeadList(&Queue->EntryListHead, Entry);
        }
        else
        {
            /* Insert at the end */
            InsertTailList(&Queue->EntryListHead, Entry);
        }
    }

    /* Return the previous state */
    return InitialState;
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
VOID
NTAPI
KeInitializeQueue(IN PKQUEUE Queue,
                  IN ULONG Count OPTIONAL)
{
    /* Initialize the Header */
    KeInitializeDispatcherHeader(&Queue->Header,
                                 QueueObject,
                                 sizeof(KQUEUE) / sizeof(ULONG),
                                 0);

    /* Initialize the Lists */
    InitializeListHead(&Queue->EntryListHead);
    InitializeListHead(&Queue->ThreadListHead);

    /* Set the Current and Maximum Count */
    Queue->CurrentCount = 0;
    Queue->MaximumCount = (Count == 0) ? (ULONG) KeNumberProcessors : Count;
}

/*
 * @implemented
 */
LONG
NTAPI
KeInsertHeadQueue(IN PKQUEUE Queue,
                  IN PLIST_ENTRY Entry)
{
    LONG PreviousState;
    KIRQL OldIrql;
    ASSERT_QUEUE(Queue);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Lock the Dispatcher Database */
    OldIrql = KiAcquireDispatcherLock();

    /* Insert the Queue */
    PreviousState = KiInsertQueue(Queue, Entry, TRUE);

    /* Release the Dispatcher Lock */
    KiReleaseDispatcherLock(OldIrql);

    /* Return previous State */
    return PreviousState;
}

/*
 * @implemented
 */
LONG
NTAPI
KeInsertQueue(IN PKQUEUE Queue,
              IN PLIST_ENTRY Entry)
{
    LONG PreviousState;
    KIRQL OldIrql;
    ASSERT_QUEUE(Queue);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Lock the Dispatcher Database */
    OldIrql = KiAcquireDispatcherLock();

    /* Insert the Queue */
    PreviousState = KiInsertQueue(Queue, Entry, FALSE);

    /* Release the Dispatcher Lock */
    KiReleaseDispatcherLock(OldIrql);

    /* Return previous State */
    return PreviousState;
}

/*
 * @implemented
 *
 * Returns number of entries in the queue
 */
LONG
NTAPI
KeReadStateQueue(IN PKQUEUE Queue)
{
    /* Returns the Signal State */
    ASSERT_QUEUE(Queue);
    return Queue->Header.SignalState;
}

/*
 * @implemented
 */
PLIST_ENTRY
NTAPI
KeRemoveQueue(IN PKQUEUE Queue,
              IN KPROCESSOR_MODE WaitMode,
              IN PLARGE_INTEGER Timeout OPTIONAL)
{
    PLIST_ENTRY QueueEntry;
    NTSTATUS Status;
    PKTHREAD Thread = KeGetCurrentThread();
    PKQUEUE PreviousQueue;
    PKWAIT_BLOCK WaitBlock = &Thread->WaitBlock[0];
    PKWAIT_BLOCK TimerBlock = &Thread->WaitBlock[TIMER_WAIT_BLOCK];
    PKTIMER Timer = &Thread->Timer;
    BOOLEAN Swappable;
    PLARGE_INTEGER OriginalDueTime = Timeout;
    LARGE_INTEGER DueTime, NewDueTime, InterruptTime;
    ULONG Hand = 0;
    ASSERT_QUEUE(Queue);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Check if the Lock is already held */
    if (Thread->WaitNext)
    {
        /* It is, so next time don't do expect this */
        Thread->WaitNext = FALSE;
        KxQueueThreadWait();
    }
    else
    {
        /* Raise IRQL to synch, prepare the wait, then lock the database */
        Thread->WaitIrql = KeRaiseIrqlToSynchLevel();
        KxQueueThreadWait();
        KiAcquireDispatcherLockAtDpcLevel();
    }

    /*
     * This is needed so that we can set the new queue right here,
     * before additional processing
     */
    PreviousQueue = Thread->Queue;
    Thread->Queue = Queue;

    /* Check if this is a different queue */
    if (Queue != PreviousQueue)
    {
        /* Get the current entry */
        QueueEntry = &Thread->QueueListEntry;
        if (PreviousQueue)
        {
            /* Remove from this list */
            RemoveEntryList(QueueEntry);

            /* Wake the queue */
            KiActivateWaiterQueue(PreviousQueue);
        }

        /* Insert in this new Queue */
        InsertTailList(&Queue->ThreadListHead, QueueEntry);
    }
    else
    {
        /* Same queue, decrement waiting threads */
        Queue->CurrentCount--;
    }

    /* Loop until the queue is processed */
    while (TRUE)
    {
        /* Check if the counts are valid and if there is still a queued entry */
        QueueEntry = Queue->EntryListHead.Flink;
        if ((Queue->CurrentCount < Queue->MaximumCount) &&
            (QueueEntry != &Queue->EntryListHead))
        {
            /* Decrease the number of entries */
            Queue->Header.SignalState--;

            /* Increase numbef of running threads */
            Queue->CurrentCount++;

            /* Check if the entry is valid. If not, bugcheck */
            if (!(QueueEntry->Flink) || !(QueueEntry->Blink))
            {
                /* Invalid item */
                KeBugCheckEx(INVALID_WORK_QUEUE_ITEM,
                             (ULONG_PTR)QueueEntry,
                             (ULONG_PTR)Queue,
                             (ULONG_PTR)NULL,
                             (ULONG_PTR)((PWORK_QUEUE_ITEM)QueueEntry)->
                                         WorkerRoutine);
            }

            /* Remove the Entry */
            RemoveEntryList(QueueEntry);
            QueueEntry->Flink = NULL;

            /* Nothing to wait on */
            break;
        }
        else
        {
            /* Check if a kernel APC is pending and we're below APC_LEVEL */
            if ((Thread->ApcState.KernelApcPending) &&
                !(Thread->SpecialApcDisable) && (Thread->WaitIrql < APC_LEVEL))
            {
                /* Increment the count and unlock the dispatcher */
                Queue->CurrentCount++;
                KiReleaseDispatcherLockFromDpcLevel();
                KiExitDispatcher(Thread->WaitIrql);
            }
            else
            {
                /* Fail if there's a User APC Pending */
                if ((WaitMode != KernelMode) &&
                    (Thread->ApcState.UserApcPending))
                {
                    /* Return the status and increase the pending threads */
                    QueueEntry = (PLIST_ENTRY)STATUS_USER_APC;
                    Queue->CurrentCount++;
                    break;
                }

                /* Enable the Timeout Timer if there was any specified */
                if (Timeout)
                {
                    /* Check if the timer expired */
                    InterruptTime.QuadPart = KeQueryInterruptTime();
                    if ((ULONG64)InterruptTime.QuadPart >= Timer->DueTime.QuadPart)
                    {
                        /* It did, so we don't need to wait */
                        QueueEntry = (PLIST_ENTRY)STATUS_TIMEOUT;
                        Queue->CurrentCount++;
                        break;
                    }

                    /* It didn't, so activate it */
                    Timer->Header.Inserted = TRUE;
                }

                /* Insert the wait block in the list */
                InsertTailList(&Queue->Header.WaitListHead,
                               &WaitBlock->WaitListEntry);

                /* Setup the wait information */
                Thread->State = Waiting;

                /* Add the thread to the wait list */
                KiAddThreadToWaitList(Thread, Swappable);

                /* Activate thread swap */
                ASSERT(Thread->WaitIrql <= DISPATCH_LEVEL);
                KiSetThreadSwapBusy(Thread);

                /* Check if we have a timer */
                if (Timeout)
                {
                    /* Insert it */
                    KxInsertTimer(Timer, Hand);
                }
                else
                {
                    /* Otherwise, unlock the dispatcher */
                    KiReleaseDispatcherLockFromDpcLevel();
                }

                /* Do the actual swap */
                Status = KiSwapThread(Thread, KeGetCurrentPrcb());

                /* Reset the wait reason */
                Thread->WaitReason = 0;

                /* Check if we were executing an APC */
                if (Status != STATUS_KERNEL_APC) return (PLIST_ENTRY)Status;

                /* Check if we had a timeout */
                if (Timeout)
                {
                    /* Recalculate due times */
                    Timeout = KiRecalculateDueTime(OriginalDueTime,
                                                   &DueTime,
                                                   &NewDueTime);
                }
            }

            /* Start another wait */
            Thread->WaitIrql = KeRaiseIrqlToSynchLevel();
            KxQueueThreadWait();
            KiAcquireDispatcherLockAtDpcLevel();
            Queue->CurrentCount--;
        }
    }

    /* Unlock Database and return */
    KiReleaseDispatcherLockFromDpcLevel();
    KiExitDispatcher(Thread->WaitIrql);
    return QueueEntry;
}

/*
 * @implemented
 */
PLIST_ENTRY
NTAPI
KeRundownQueue(IN PKQUEUE Queue)
{
    PLIST_ENTRY ListHead, NextEntry;
    PLIST_ENTRY FirstEntry = NULL;
    PKTHREAD Thread;
    KIRQL OldIrql;
    ASSERT_QUEUE(Queue);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);
    ASSERT(IsListEmpty(&Queue->Header.WaitListHead));

    /* Get the Dispatcher Lock */
    OldIrql = KiAcquireDispatcherLock();

    /* Make sure the list is not empty */
    if (!IsListEmpty(&Queue->EntryListHead))
    {
        /* Remove it */
        FirstEntry = RemoveHeadList(&Queue->EntryListHead);
    }

    /* Unlink threads and clear their Thread->Queue */
    ListHead = &Queue->ThreadListHead;
    NextEntry = ListHead->Flink;
    while (ListHead != NextEntry)
    {
        /* Get the Entry's Thread */
        Thread = CONTAINING_RECORD(NextEntry, KTHREAD, QueueListEntry);

        /* Kill its Queue */
        Thread->Queue = NULL;

        /* Remove this entry */
        RemoveEntryList(NextEntry);

        /* Get the next entry */
        NextEntry = NextEntry->Flink;
    }

    /* Release the lock and return */
    KiReleaseDispatcherLock(OldIrql);
    return FirstEntry;
}

/* EOF */
