/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/queue.c
 * PURPOSE:         Implements kernel queues
 * 
 * PROGRAMMERS:     Eric Kohl (ekohl@rz-online.de)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

LONG STDCALL KiInsertQueue(IN PKQUEUE Queue, IN PLIST_ENTRY Entry, BOOLEAN Head);
              
/*
 * @implemented
 */
VOID 
STDCALL
KeInitializeQueue(IN PKQUEUE Queue,
                  IN ULONG Count OPTIONAL)
{
    DPRINT("KeInitializeQueue %x\n", Queue);
    
    /* Initialize the Header */
    KeInitializeDispatcherHeader(&Queue->Header,
                                 QueueObject,
                                 sizeof(KQUEUE)/sizeof(ULONG),
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
STDCALL
KeInsertHeadQueue(IN PKQUEUE Queue,
                  IN PLIST_ENTRY Entry)
{
    LONG PreviousState;
    KIRQL OldIrql;
    
    DPRINT("KeInsertHeadQueue %x\n", Queue);
    
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
LONG STDCALL
KeInsertQueue(IN PKQUEUE Queue,
              IN PLIST_ENTRY Entry)
{
    LONG PreviousState;
    KIRQL OldIrql;
    
    DPRINT("KeInsertQueue %x\n", Queue);
    
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
STDCALL
KeReadStateQueue(IN PKQUEUE Queue)
{
    /* Returns the Signal State */
    return(Queue->Header.SignalState);
}

/*
 * @implemented
 */
PLIST_ENTRY 
STDCALL
KeRemoveQueue(IN PKQUEUE Queue,
              IN KPROCESSOR_MODE WaitMode,
              IN PLARGE_INTEGER Timeout OPTIONAL)
{
   
    PLIST_ENTRY ListEntry;
    NTSTATUS Status;
    PKTHREAD Thread = KeGetCurrentThread();
    KIRQL OldIrql;
    PKQUEUE PreviousQueue;
    PKWAIT_BLOCK WaitBlock;
    PKWAIT_BLOCK TimerWaitBlock;
    PKTIMER Timer;

    DPRINT("KeRemoveQueue %x\n", Queue);
    
    /* Check if the Lock is already held */
    if (Thread->WaitNext) {
    
        DPRINT("Lock is already held\n");
    
    } else {
        
        /* Lock the Dispatcher Database */
        DPRINT("Lock not held, acquiring\n");
        OldIrql = KeAcquireDispatcherDatabaseLock();
        Thread->WaitIrql = OldIrql;
    }

    /* This is needed so that we can set the new queue right here, before additional processing */
    PreviousQueue = Thread->Queue;
    Thread->Queue = Queue;

    /* Check if this is a different queue */    
    if (Queue != PreviousQueue) {
        
        /*
         * INVESTIGATE: What is the Thread->QueueListEntry used for? It's linked it into the
         * Queue->ThreadListHead when the thread registers with the queue and unlinked when
         * the thread registers with a new queue. The Thread->Queue already tells us what
         * queue the thread is registered with.
         * -Gunnar
         */
        DPRINT("Different Queue\n");
        if (PreviousQueue)  {
          
            /* Remove from this list */
            DPRINT("Removing Old Queue\n");
            RemoveEntryList(&Thread->QueueListEntry);
            
            /* Wake the queue */
            DPRINT("Activating new thread\n");
            KiWakeQueue(PreviousQueue);
      }

        /* Insert in this new Queue */
        DPRINT("Inserting new Queue!\n");
        InsertTailList(&Queue->ThreadListHead, &Thread->QueueListEntry);
   
    } else {
      
        /* Same queue, decrement waiting threads */
        DPRINT("Same Queue!\n");
        Queue->CurrentCount--;
    }
    
    /* Loop until the queue is processed */
    while (TRUE) {
      
        /* Get the Entry */
        ListEntry = Queue->EntryListHead.Flink;
        
        /* Check if the counts are valid and if there is still a queued entry */
        if ((Queue->CurrentCount < Queue->MaximumCount) && 
             (ListEntry != &Queue->EntryListHead)) {
          
            /* Remove the Entry and Save it */
            DPRINT("Removing Queue Entry. CurrentCount: %d, Maximum Count: %d\n", 
                    Queue->CurrentCount, Queue->MaximumCount);
            ListEntry = RemoveHeadList(&Queue->EntryListHead);
            
            /* Decrease the number of entries */
            Queue->Header.SignalState--;
            
            /* Increase numbef of running threads */
            Queue->CurrentCount++;
            
            /* Check if the entry is valid. If not, bugcheck */
            if (!ListEntry->Flink || !ListEntry->Blink) {
            
                KEBUGCHECK(INVALID_WORK_QUEUE_ITEM);
            }
            
            /* Remove the Entry */
            RemoveEntryList(ListEntry);
            ListEntry->Flink = NULL;
            
            /* Nothing to wait on */
            break;
            
        } else {
                       
            /* Do the wait */
            DPRINT("Waiting on Queue Entry. CurrentCount: %d, Maximum Count: %d\n", 
                    Queue->CurrentCount, Queue->MaximumCount);
            
            /* Use the Thread's Wait Block, it's big enough */
            Thread->WaitBlockList = &Thread->WaitBlock[0];
            
            /* Fail if there's an APC Pending */
            if (WaitMode == UserMode && Thread->ApcState.UserApcPending) {
            
                /* Return the status and increase the pending threads */
                ListEntry = (PLIST_ENTRY)STATUS_USER_APC;
                Queue->CurrentCount++;
                
                /* Nothing to wait on */
                break;
            }
            
            /* Build the Wait Block */
            WaitBlock = &Thread->WaitBlock[0];
            WaitBlock->Object = (PVOID)Queue;
            WaitBlock->WaitKey = STATUS_SUCCESS;
            WaitBlock->WaitType = WaitAny;
            WaitBlock->Thread = Thread;
            WaitBlock->NextWaitBlock = NULL;
            
            Thread->WaitStatus = STATUS_SUCCESS;
            
            /* We need to wait for the object... check if we have a timeout */
            if (Timeout) {
        
                /* If it's zero, then don't do any waiting */
                if (!Timeout->QuadPart) {
            
                    /* Instant Timeout, return the status and increase the pending threads */
                    DPRINT("Queue Wait has timed out\n");
                    ListEntry = (PLIST_ENTRY)STATUS_TIMEOUT;
                    Queue->CurrentCount++;
                    
                    /* Nothing to wait on */
                    break;
                }
                
                /* 
                 * Set up the Timer. We'll use the internal function so that we can
                 * hold on to the dispatcher lock.
                 */
                Timer = &Thread->Timer;
                TimerWaitBlock = &Thread->WaitBlock[1];

                /* Set up the Timer Wait Block */
                TimerWaitBlock->Object = (PVOID)Timer;
                TimerWaitBlock->Thread = Thread;
                TimerWaitBlock->WaitKey = STATUS_TIMEOUT;
                TimerWaitBlock->WaitType = WaitAny;
                TimerWaitBlock->NextWaitBlock = NULL;
            
                /* Link the timer to this Wait Block */
                InitializeListHead(&Timer->Header.WaitListHead);
                InsertTailList(&Timer->Header.WaitListHead, &TimerWaitBlock->WaitListEntry);
            
                /* Create Timer */
                DPRINT("Creating Timer with timeout %I64d\n", *Timeout);
                KiInsertTimer(Timer, *Timeout);
            }
                            
            /* Insert the wait block into the Queues's wait list */
            WaitBlock = Thread->WaitBlockList;
            InsertTailList(&Queue->Header.WaitListHead, &WaitBlock->WaitListEntry);
            
            /* Block the Thread */
            DPRINT("Blocking the Thread: %x %x!\n", KeGetCurrentThread(), Thread);
            PsBlockThread(&Status, 
                          FALSE, 
                          WaitMode,
                          WrQueue);
    
            /* Reset the wait reason */
            Thread->WaitReason = 0;
            
            /* Check if we were executing an APC */
            if (Status != STATUS_KERNEL_APC) {
             
                /* Done Waiting  */
                DPRINT("Done waking queue. Thread: %x %x!\n", KeGetCurrentThread(), Thread);
                return (PLIST_ENTRY)Status;
            }
        
            /* Acquire again the lock */
            DPRINT("Looping again\n");
            OldIrql = KeAcquireDispatcherDatabaseLock();
            
            /* Save the new IRQL and decrease number of waiting threads */
            Thread->WaitIrql = OldIrql;
            Queue->CurrentCount--;
        }
    }
    
    /* Unlock Database and return */
    KeReleaseDispatcherDatabaseLock(Thread->WaitIrql);
    DPRINT("Returning. CurrentCount: %d, Maximum Count: %d\n", 
            Queue->CurrentCount, Queue->MaximumCount);
    return ListEntry;
}

/*
 * @implemented
 */
PLIST_ENTRY 
STDCALL
KeRundownQueue(IN PKQUEUE Queue)
{
    PLIST_ENTRY EnumEntry;
    PLIST_ENTRY FirstEntry;
    PKTHREAD Thread;
    KIRQL OldIrql;

    DPRINT("KeRundownQueue(Queue %x)\n", Queue);

    /* Get the Dispatcher Lock */
    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* Get the First Empty Entry */
    FirstEntry = Queue->EntryListHead.Flink;
            
    /* Make sure the list is not empty */
    if (FirstEntry == &Queue->EntryListHead) {
    
        /* It is, so don't return anything */
        EnumEntry = NULL;
   
    } else {
       
        /* Remove it */
        RemoveEntryList(&Queue->EntryListHead);
    }
    
    /* Unlink threads and clear their Thread->Queue */
    while (!IsListEmpty(&Queue->ThreadListHead)) {
        
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
    
    /* Decrement the number of active threads */
    DPRINT("KiWakeQueue: %x. Thread: %x\n", Queue, KeGetCurrentThread());
    Queue->CurrentCount--;
    
    /* Make sure the counts are OK */
    if (Queue->CurrentCount < Queue->MaximumCount) {
    
        /* Get the Queue Entry */
        QueueEntry = Queue->EntryListHead.Flink;
        
        /* Get the Wait Entry */
        WaitEntry = Queue->Header.WaitListHead.Blink;
        DPRINT("Queue Count is ok, Queue entries: %x, %x\n", QueueEntry, WaitEntry);
        
        /* Make sure that the Queue List isn't empty and that this entry is valid */
        if (!IsListEmpty(&Queue->Header.WaitListHead) && 
            (QueueEntry != &Queue->EntryListHead)) {
            
            /* Remove this entry */
            DPRINT("Queue in List, removing it\n");
            RemoveEntryList(QueueEntry);
            QueueEntry->Flink = NULL;
            
            /* Decrease the Signal State */
            Queue->Header.SignalState--;
            
            /* Unwait the Thread */
            DPRINT("Unwaiting Thread\n");
            WaitBlock = CONTAINING_RECORD(WaitEntry, KWAIT_BLOCK, WaitListEntry);
            KiAbortWaitThread(WaitBlock->Thread, (NTSTATUS)QueueEntry);
        }
    }    
}

/*
 * Returns the previous number of entries in the queue
 */
LONG 
STDCALL
KiInsertQueue(IN PKQUEUE Queue,
              IN PLIST_ENTRY Entry,
              BOOLEAN Head)
{
    ULONG InitialState;
    PKTHREAD Thread = KeGetCurrentThread();
    PKWAIT_BLOCK WaitBlock;
    PLIST_ENTRY WaitEntry;
  
    DPRINT("KiInsertQueue(Queue %x, Entry %x)\n", Queue, Entry);
   
    /* Save the old state */
    InitialState = Queue->Header.SignalState;
     
    /* Get the Entry */
    WaitEntry = Queue->Header.WaitListHead.Blink;
    DPRINT("Initial State, WaitEntry: %d, %x\n", InitialState, WaitEntry);
    
    /*
     * Why the KeGetCurrentThread()->Queue != Queue?
     * KiInsertQueue might be called from an APC for the current thread. 
     * -Gunnar
     */
    if ((Queue->CurrentCount < Queue->MaximumCount) && 
        (WaitEntry != &Queue->Header.WaitListHead) &&
        ((Thread->Queue != Queue) || (Thread->WaitReason != WrQueue))) {
       
        /* Remove the wait entry */
        DPRINT("Removing Entry\n");
        RemoveEntryList(WaitEntry);
        
        /* Get the Wait Block and Thread */
        WaitBlock = CONTAINING_RECORD(WaitEntry, KWAIT_BLOCK, WaitListEntry);
        DPRINT("Got wait block: %x\n", WaitBlock);
        Thread = WaitBlock->Thread;
        
        /* Reset the wait reason */
        Thread->WaitReason = 0;
        
        /* Increase the waiting threads */
        Queue->CurrentCount++;
        
        /* Check if there's a Thread Timer */
        if (Thread->Timer.Header.Inserted) {
    
            /* Cancel the Thread Timer with the no-lock fastpath */
            DPRINT("Removing the Thread's Timer\n");
            Thread->Timer.Header.Inserted = FALSE;
            RemoveEntryList(&Thread->Timer.TimerListEntry);
        }
        
        /* Reschedule the Thread */
        DPRINT("Unblocking the Thread\n");
        PsUnblockThread((PETHREAD)Thread, (PNTSTATUS)&Entry, 0);
    
    } else {
    
        /* Increase the Entries */
        DPRINT("Adding new Queue Entry: %d %d\n", Head, Queue->Header.SignalState);
        Queue->Header.SignalState++;
        
        if (Head) {
        
            InsertHeadList(&Queue->EntryListHead, Entry);
        
        } else {
        
            InsertTailList(&Queue->EntryListHead, Entry);
        }
    }

    /* Return the previous state */
    DPRINT("Returning\n");
    return InitialState;
}

/* EOF */
