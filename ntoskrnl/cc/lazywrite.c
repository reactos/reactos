/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/cc/lazywrite.c
 * PURPOSE:         Cache manager
 *
 * PROGRAMMERS:     Pierre Schweitzer (pierre@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

typedef enum _WORK_QUEUE_FUNCTIONS
{
    ReadAhead = 1,
    WriteBehind = 2,
    LazyWrite = 3,
    SetDone = 4,
} WORK_QUEUE_FUNCTIONS, *PWORK_QUEUE_FUNCTIONS;

/* Counters:
 * - Amount of pages flushed by lazy writer
 * - Number of times lazy writer ran
 */
ULONG CcLazyWritePages = 0;
ULONG CcLazyWriteIos = 0;

/* Internal vars (MS):
 * - Lazy writer status structure
 * - Lookaside list where to allocate work items
 * - Queue for regular work items
 * - Available worker threads
 * - Marker for throttling queues
 * - Number of ongoing workers
 * - Three seconds delay for lazy writer 
 * - One second delay for lazy writer
 * - Zero delay for lazy writer
 * - Number of worker threads
 */
LAZY_WRITER LazyWriter;
NPAGED_LOOKASIDE_LIST CcTwilightLookasideList;
LIST_ENTRY CcRegularWorkQueue;
LIST_ENTRY CcIdleWorkerThreadList;
BOOLEAN CcQueueThrottle = FALSE;
ULONG CcNumberActiveWorkerThreads = 0;
LARGE_INTEGER CcFirstDelay = RTL_CONSTANT_LARGE_INTEGER((LONGLONG)-1*3000*1000*10);
LARGE_INTEGER CcIdleDelay = RTL_CONSTANT_LARGE_INTEGER((LONGLONG)-1*1000*1000*10);
LARGE_INTEGER CcNoDelay = RTL_CONSTANT_LARGE_INTEGER((LONGLONG)0);
ULONG CcNumberWorkerThreads;

/* Internal vars (ROS):
 */
KEVENT iLazyWriterNotify;

/* FUNCTIONS *****************************************************************/

VOID
CcPostWorkQueue(
    IN PWORK_QUEUE_ENTRY WorkItem,
    IN PLIST_ENTRY WorkQueue)
{
    KIRQL OldIrql;
    PWORK_QUEUE_ITEM ThreadToSpawn;

    /* First of all, insert the item in the queue */
    OldIrql = KeAcquireQueuedSpinLock(LockQueueWorkQueueLock);
    InsertTailList(WorkQueue, &WorkItem->WorkQueueLinks);

    /* Now, define whether we have to spawn a new work thread
     * We will spawn a new one if:
     * - There's no throttle in action
     * - There's still at least one idle thread
     */
    ThreadToSpawn = NULL;
    if (!CcQueueThrottle && !IsListEmpty(&CcIdleWorkerThreadList))
    {
        PLIST_ENTRY ListEntry;

        /* Get the idle thread */
        ListEntry = RemoveHeadList(&CcIdleWorkerThreadList);
        ThreadToSpawn = CONTAINING_RECORD(ListEntry, WORK_QUEUE_ITEM, List);

        /* We're going to have one more! */
        CcNumberActiveWorkerThreads += 1;
    }

    KeReleaseQueuedSpinLock(LockQueueWorkQueueLock, OldIrql);

    /* If we have a thread to spawn, do it! */
    if (ThreadToSpawn != NULL)
    {
        /* We NULLify it to be consistent with initialization */
        ThreadToSpawn->List.Flink = NULL;
        ExQueueWorkItem(ThreadToSpawn, CriticalWorkQueue);
    }
}

VOID
NTAPI
CcScanDpc(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2)
{
    PWORK_QUEUE_ENTRY WorkItem;

    /* Allocate a work item */
    WorkItem = ExAllocateFromNPagedLookasideList(&CcTwilightLookasideList);
    if (WorkItem == NULL)
    {
        LazyWriter.ScanActive = FALSE;
        return;
    }

    /* And post it, it will be for lazy write */
    WorkItem->Function = LazyWrite;
    CcPostWorkQueue(WorkItem, &CcRegularWorkQueue);
}

/* FIXME: handle master lock */
VOID
CcLazyWriteScan(VOID)
{
    ULONG Target;
    ULONG Count;
    PLIST_ENTRY ListEntry;

    /* We're not sleeping anymore */
    KeClearEvent(&iLazyWriterNotify);

    /* Our target is one-eighth of the dirty pages */
    Target = CcTotalDirtyPages / 8;
    if (Target != 0)
    {
        /* Flush! */
        DPRINT1("Lazy writer starting (%d)\n", Target);
        CcRosFlushDirtyPages(Target, &Count, FALSE, TRUE);

        /* And update stats */
        CcLazyWritePages += Count;
        ++CcLazyWriteIos;
        DPRINT1("Lazy writer done (%d)\n", Count);
    }

    /* Inform people waiting on us that we're done */
    KeSetEvent(&iLazyWriterNotify, IO_DISK_INCREMENT, FALSE);

    /* Likely not optimal, but let's handle one deferred write now! */
    ListEntry = ExInterlockedRemoveHeadList(&CcDeferredWrites, &CcDeferredWriteSpinLock);
    if (ListEntry != NULL)
    {
        PDEFERRED_WRITE Context;

        /* Extract the context */
        Context = CONTAINING_RECORD(ListEntry, DEFERRED_WRITE, DeferredWriteLinks);
        ASSERT(Context->NodeTypeCode == NODE_TYPE_DEFERRED_WRITE);

        /* Can we write now? */
        if (CcCanIWrite(Context->FileObject, Context->BytesToWrite, FALSE, TRUE))
        {
            /* Yes! Do it, and destroy the associated context */
            Context->PostRoutine(Context->Context1, Context->Context2);
            ExFreePoolWithTag(Context, 'CcDw');
        }
        else
        {
            /* Otherwise, requeue it, but in tail, so that it doesn't block others
             * This is clearly to improve, but given the poor algorithm used now
             * It's better than nothing!
             */
            ExInterlockedInsertTailList(&CcDeferredWrites,
                                        &Context->DeferredWriteLinks,
                                        &CcDeferredWriteSpinLock);
        }
    }

    /* We're no longer active */
    LazyWriter.ScanActive = FALSE;
}

VOID CcScheduleLazyWriteScan(
    IN BOOLEAN NoDelay)
{
    /* If no delay, immediately start lazy writer,
     * no matter it was already started
     */
    if (NoDelay)
    {
        LazyWriter.ScanActive = TRUE;
        KeSetTimer(&LazyWriter.ScanTimer, CcNoDelay, &LazyWriter.ScanDpc);
    }
    /* Otherwise, if it's not running, just wait three seconds to start it */
    else if (!LazyWriter.ScanActive)
    {
        LazyWriter.ScanActive = TRUE;
        KeSetTimer(&LazyWriter.ScanTimer, CcFirstDelay, &LazyWriter.ScanDpc);
    }
    /* Finally, already running, so queue for the next second */
    else
    {
        KeSetTimer(&LazyWriter.ScanTimer, CcIdleDelay, &LazyWriter.ScanDpc);
    }
}

VOID
NTAPI
CcWorkerThread(
    IN PVOID Parameter)
{
    KIRQL OldIrql;
    BOOLEAN DropThrottle;
    PWORK_QUEUE_ITEM Item;

    /* Get back our thread item */
    Item = Parameter;
    /* And by default, don't touch throttle */
    DropThrottle = FALSE;

    /* Loop till we have jobs */
    while (TRUE)
    {
        PWORK_QUEUE_ENTRY WorkItem;

        /* Lock queues */
        OldIrql = KeAcquireQueuedSpinLock(LockQueueWorkQueueLock);

        /* If we have to touch throttle, reset it now! */
        if (DropThrottle)
        {
            CcQueueThrottle = FALSE;
            DropThrottle = FALSE;
        }

        /* If no work to do, we're done */
        if (IsListEmpty(&CcRegularWorkQueue))
        {
            break;
        }

        /* Get our work item, if someone is waiting for us to finish
         * and we're not the only thread in queue
         * then, quit running to let the others do
         * and throttle so that noone starts till current activity is over
         */
        WorkItem = CONTAINING_RECORD(CcRegularWorkQueue.Flink, WORK_QUEUE_ENTRY, WorkQueueLinks);
        if (WorkItem->Function == SetDone && CcNumberActiveWorkerThreads > 1)
        {
            CcQueueThrottle = TRUE;
            break;
        }

        /* Otherwise, remove current entry */
        RemoveEntryList(&WorkItem->WorkQueueLinks);
        KeReleaseQueuedSpinLock(LockQueueWorkQueueLock, OldIrql);

        /* And handle it */
        switch (WorkItem->Function)
        {
            /* We only support lazy write now */
            case LazyWrite:
                CcLazyWriteScan();
                break;

            case SetDone:
                KeSetEvent(WorkItem->Parameters.Event.Event, IO_NO_INCREMENT, FALSE);
                DropThrottle = TRUE;
                break;
        }

        /* And release the item */
        ExFreeToNPagedLookasideList(&CcTwilightLookasideList, WorkItem);
    }

    /* Our thread is available again */
    InsertTailList(&CcIdleWorkerThreadList, &Item->List);
    /* One less worker */
    --CcNumberActiveWorkerThreads;
    KeReleaseQueuedSpinLock(LockQueueWorkQueueLock, OldIrql);
}
