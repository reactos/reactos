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
#include <internal/debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

/*
 * Called when a thread which has a queue entry is entering a wait state
 */
VOID
FASTCALL
KiWakeQueue(IN PKQUEUE Queue)
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
            KiAbortWaitThread(Thread, (NTSTATUS)QueueEntry, IO_NO_INCREMENT);
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
              BOOLEAN Head)
{
    ULONG InitialState;
    PKTHREAD Thread = KeGetCurrentThread();
    PKWAIT_BLOCK WaitBlock;
    PLIST_ENTRY WaitEntry;
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
        RemoveEntryList(&Thread->WaitListEntry);
        Thread->WaitReason = 0;

        /* Increase the active threads and set the status*/
        Queue->CurrentCount++;

        /* Check if there's a Thread Timer */
        if (Thread->Timer.Header.Inserted)
        {
            /* Cancel the Thread Timer with the no-lock fastpath */
            Thread->Timer.Header.Inserted = FALSE;
            RemoveEntryList(&Thread->Timer.TimerListEntry);
        }

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
    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* Insert the Queue */
    PreviousState = KiInsertQueue(Queue, Entry, TRUE);

    /* Release the Dispatcher Lock */
    KeReleaseDispatcherDatabaseLock(OldIrql);

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
    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* Insert the Queue */
    PreviousState = KiInsertQueue(Queue, Entry, FALSE);

    /* Release the Dispatcher Lock */
    KeReleaseDispatcherDatabaseLock(OldIrql);

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
    KIRQL OldIrql;
    PKQUEUE PreviousQueue;
    PKWAIT_BLOCK WaitBlock;
    PKTIMER Timer;
    ASSERT_QUEUE(Queue);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Check if the Lock is already held */
    if (Thread->WaitNext)
    {
        /* It is, so next time don't do expect this */
        Thread->WaitNext = FALSE;
    }
    else
    {
        /* Lock the Dispatcher Database */
        OldIrql = KeAcquireDispatcherDatabaseLock();
        Thread->WaitIrql = OldIrql;
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
            KiWakeQueue(PreviousQueue);
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
                KEBUGCHECK(INVALID_WORK_QUEUE_ITEM);
            }

            /* Remove the Entry */
            RemoveEntryList(QueueEntry);
            QueueEntry->Flink = NULL;

            /* Nothing to wait on */
            break;
        }
        else
        {
            /* Use the Thread's Wait Block, it's big enough */
            Thread->WaitBlockList = &Thread->WaitBlock[0];

            /* Check if a kernel APC is pending and we're below APC_LEVEL */
            if ((Thread->ApcState.KernelApcPending) &&
                !(Thread->SpecialApcDisable) && (Thread->WaitIrql < APC_LEVEL))
            {
                /* Increment the count and unlock the dispatcher */
                Queue->CurrentCount++;
                KeReleaseDispatcherDatabaseLock(Thread->WaitIrql);
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

                /* Build the Wait Block */
                WaitBlock = &Thread->WaitBlock[0];
                WaitBlock->Object = (PVOID)Queue;
                WaitBlock->WaitKey = STATUS_SUCCESS;
                WaitBlock->WaitType = WaitAny;
                WaitBlock->Thread = Thread;
                Thread->WaitStatus = STATUS_WAIT_0;

                /* We need to wait for the object... check for a timeout */
                if (Timeout)
                {
                    /* Check if it's zero */
                    if (!Timeout->QuadPart)
                    {
                        /* Don't wait. Return and increase pending threads */
                        QueueEntry = (PLIST_ENTRY)STATUS_TIMEOUT;
                        Queue->CurrentCount++;
                        break;
                    }

                    /*
                     * Set up the Timer. We'll use the internal function so
                     * that we can hold on to the dispatcher lock.
                     */
                    Timer = &Thread->Timer;
                    WaitBlock->NextWaitBlock = &Thread->WaitBlock[1];
                    WaitBlock = &Thread->WaitBlock[1];

                    /* Set up the Timer Wait Block */
                    WaitBlock->Object = (PVOID)Timer;
                    WaitBlock->Thread = Thread;
                    WaitBlock->WaitKey = STATUS_TIMEOUT;
                    WaitBlock->WaitType = WaitAny;

                    /* Link the timer to this Wait Block */
                    Timer->Header.WaitListHead.Flink =
                        &WaitBlock->WaitListEntry;
                    Timer->Header.WaitListHead.Blink =
                        &WaitBlock->WaitListEntry;

                    /* Create Timer */
                    KiInsertTimer(Timer, *Timeout);
                }

                /* Close the loop */
                WaitBlock->NextWaitBlock = &Thread->WaitBlock[0];

                /* Insert the wait block into the Queues's wait list */
                WaitBlock = &Thread->WaitBlock[0];
                InsertTailList(&Queue->Header.WaitListHead,
                               &WaitBlock->WaitListEntry);

                /* Setup the wait information */
                Thread->WaitMode = WaitMode;
                Thread->WaitReason = WrQueue;
                Thread->Alertable = FALSE;
                Thread->WaitTime = ((PLARGE_INTEGER)&KeTickCount)->LowPart;
                Thread->State = Waiting;

                /* Find a new thread to run */
                Status = KiSwapThread();

                /* Reset the wait reason */
                Thread->WaitReason = 0;

                /* Check if we were executing an APC */
                if (Status != STATUS_KERNEL_APC)
                {
                    /* Done Waiting  */
                    return (PLIST_ENTRY)Status;
                }

                /* Check if we had a timeout */
                if (Timeout)
                {
                    /* FIXME: Fixup interval */
                    DPRINT1("FIXME!!!\n");
                }
            }

            /* Reacquire the lock */
            OldIrql = KeAcquireDispatcherDatabaseLock();

            /* Save the new IRQL and decrease number of waiting threads */
            Thread->WaitIrql = OldIrql;
            Queue->CurrentCount--;
        }
    }

    /* Unlock Database and return */
    KeReleaseDispatcherDatabaseLock(Thread->WaitIrql);
    return QueueEntry;
}

/*
 * @implemented
 */
PLIST_ENTRY
NTAPI
KeRundownQueue(IN PKQUEUE Queue)
{
    PLIST_ENTRY EnumEntry;
    PLIST_ENTRY FirstEntry = NULL;
    PKTHREAD Thread;
    KIRQL OldIrql;
    ASSERT_QUEUE(Queue);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);
    ASSERT(IsListEmpty(&Queue->Header.WaitListHead));

    /* Get the Dispatcher Lock */
    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* Make sure the list is not empty */
    if (!IsListEmpty(&Queue->EntryListHead))
    {
        /* Remove it */
        FirstEntry = RemoveHeadList(&Queue->EntryListHead);
    }

    /* Unlink threads and clear their Thread->Queue */
    while (!IsListEmpty(&Queue->ThreadListHead))
    {
        /* Get the Entry and Remove it */
        EnumEntry = RemoveHeadList(&Queue->ThreadListHead);

        /* Get the Entry's Thread */
        Thread = CONTAINING_RECORD(EnumEntry, KTHREAD, QueueListEntry);

        /* Kill its Queue */
        Thread->Queue = NULL;
    }

    /* Release the lock and return */
    KeReleaseDispatcherDatabaseLock(OldIrql);
    return FirstEntry;
}

/* EOF */
