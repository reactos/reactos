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

/* Counters:
 * - Amount of pages flushed by lazy writer
 * - Number of times lazy writer ran
 */
ULONG CcLazyWritePages = 0;
ULONG CcLazyWriteIos = 0;

/* Internal vars (MS):
 * - Lazy writer status structure
 * - Lookaside list where to allocate work items
 * - Queue for high priority work items (read ahead)
 * - Queue for regular work items
 * - Available worker threads
 * - Queue for stuff to be queued after lazy writer is done
 * - Marker for throttling queues
 * - Number of ongoing workers
 * - Three seconds delay for lazy writer
 * - One second delay for lazy writer
 * - Zero delay for lazy writer
 * - Number of worker threads
 */
LAZY_WRITER LazyWriter;
NPAGED_LOOKASIDE_LIST CcTwilightLookasideList;
LIST_ENTRY CcExpressWorkQueue;
LIST_ENTRY CcRegularWorkQueue;
LIST_ENTRY CcIdleWorkerThreadList;
LIST_ENTRY CcPostTickWorkQueue;
BOOLEAN CcQueueThrottle = FALSE;
ULONG CcNumberActiveWorkerThreads = 0;
LARGE_INTEGER CcFirstDelay = RTL_CONSTANT_LARGE_INTEGER((LONGLONG)-1*3000*1000*10);
LARGE_INTEGER CcIdleDelay = RTL_CONSTANT_LARGE_INTEGER((LONGLONG)-1*1000*1000*10);
LARGE_INTEGER CcNoDelay = RTL_CONSTANT_LARGE_INTEGER((LONGLONG)0);
ULONG CcNumberWorkerThreads;

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
    WorkItem->Function = LazyScan;
    CcPostWorkQueue(WorkItem, &CcRegularWorkQueue);
}

VOID
CcWriteBehind(VOID)
{
    ULONG Target, Count;

    Target = CcTotalDirtyPages / 8;
    if (Target != 0)
    {
        /* Flush! */
        DPRINT("Lazy writer starting (%d)\n", Target);
        CcRosFlushDirtyPages(Target, &Count, FALSE, TRUE);

        /* And update stats */
        CcLazyWritePages += Count;
        ++CcLazyWriteIos;
        DPRINT("Lazy writer done (%d)\n", Count);
    }
}

VOID
CcLazyWriteScan(VOID)
{
    ULONG Target;
    KIRQL OldIrql;
    PLIST_ENTRY ListEntry;
    LIST_ENTRY ToPost;
    PWORK_QUEUE_ENTRY WorkItem;

    /* Do we have entries to queue after we're done? */
    InitializeListHead(&ToPost);
    OldIrql = KeAcquireQueuedSpinLock(LockQueueMasterLock);
    if (LazyWriter.OtherWork)
    {
        while (!IsListEmpty(&CcPostTickWorkQueue))
        {
            ListEntry = RemoveHeadList(&CcPostTickWorkQueue);
            WorkItem = CONTAINING_RECORD(ListEntry, WORK_QUEUE_ENTRY, WorkQueueLinks);
            InsertTailList(&ToPost, &WorkItem->WorkQueueLinks);
        }
        LazyWriter.OtherWork = FALSE;
    }
    KeReleaseQueuedSpinLock(LockQueueMasterLock, OldIrql);

    /* Our target is one-eighth of the dirty pages */
    Target = CcTotalDirtyPages / 8;
    if (Target != 0)
    {
        /* There is stuff to flush, schedule a write-behind operation */

        /* Allocate a work item */
        WorkItem = ExAllocateFromNPagedLookasideList(&CcTwilightLookasideList);
        if (WorkItem != NULL)
        {
            WorkItem->Function = WriteBehind;
            CcPostWorkQueue(WorkItem, &CcRegularWorkQueue);
        }
    }

    /* Post items that were due for end of run */
    while (!IsListEmpty(&ToPost))
    {
        ListEntry = RemoveHeadList(&ToPost);
        WorkItem = CONTAINING_RECORD(ListEntry, WORK_QUEUE_ENTRY, WorkQueueLinks);
        CcPostWorkQueue(WorkItem, &CcRegularWorkQueue);
    }

    /* If we have deferred writes, try them now! */
    if (!IsListEmpty(&CcDeferredWrites))
    {
        CcPostDeferredWrites();
        /* Reschedule immediately a lazy writer run
         * Keep us active to have short idle delay
         */
        CcScheduleLazyWriteScan(FALSE);
    }
    else
    {
        /* We're no longer active */
        OldIrql = KeAcquireQueuedSpinLock(LockQueueMasterLock);
        LazyWriter.ScanActive = FALSE;
        KeReleaseQueuedSpinLock(LockQueueMasterLock, OldIrql);
    }
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
    BOOLEAN DropThrottle, WritePerformed;
    PWORK_QUEUE_ITEM Item;
#if DBG
    PIRP TopLevel;
#endif

    /* Get back our thread item */
    Item = Parameter;
    /* And by default, don't touch throttle */
    DropThrottle = FALSE;
    /* No write performed */
    WritePerformed =  FALSE;

#if DBG
    /* Top level IRP should be clean when started
     * Save it to catch buggy drivers (or bugs!)
     */
    TopLevel = IoGetTopLevelIrp();
    if (TopLevel != NULL)
    {
        DPRINT1("(%p) TopLevel IRP for this thread: %p\n", PsGetCurrentThread(), TopLevel);
    }
#endif

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

        /* Check first if we have read ahead to do */
        if (IsListEmpty(&CcExpressWorkQueue))
        {
            /* If not, check regular queue */
            if (IsListEmpty(&CcRegularWorkQueue))
            {
                break;
            }
            else
            {
                WorkItem = CONTAINING_RECORD(CcRegularWorkQueue.Flink, WORK_QUEUE_ENTRY, WorkQueueLinks);
            }
        }
        else
        {
            WorkItem = CONTAINING_RECORD(CcExpressWorkQueue.Flink, WORK_QUEUE_ENTRY, WorkQueueLinks);
        }

        /* Get our work item, if someone is waiting for us to finish
         * and we're not the only thread in queue
         * then, quit running to let the others do
         * and throttle so that noone starts till current activity is over
         */
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
            case ReadAhead:
                CcPerformReadAhead(WorkItem->Parameters.Read.FileObject);
                break;

            case WriteBehind:
                PsGetCurrentThread()->MemoryMaker = 1;
                CcWriteBehind();
                PsGetCurrentThread()->MemoryMaker = 0;
                WritePerformed = TRUE;
                break;

            case LazyScan:
                CcLazyWriteScan();
                break;

            case SetDone:
                KeSetEvent(WorkItem->Parameters.Event.Event, IO_NO_INCREMENT, FALSE);
                DropThrottle = TRUE;
                break;

            default:
                DPRINT1("Ignored item: %p (%d)\n", WorkItem, WorkItem->Function);
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

    /* If there are pending write openations and we have at least 20 dirty pages */
    if (!IsListEmpty(&CcDeferredWrites) && CcTotalDirtyPages >= 20)
    {
        /* And if we performed a write operation previously, then
         * stress the system a bit and reschedule a scan to find
         * stuff to write
         */
        if (WritePerformed)
        {
            CcLazyWriteScan();
        }
    }

#if DBG
    /* Top level shouldn't have changed */
    if (TopLevel != IoGetTopLevelIrp())
    {
        DPRINT1("(%p) Mismatching TopLevel: %p, %p\n", PsGetCurrentThread(), TopLevel, IoGetTopLevelIrp());
    }
#endif
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
CcWaitForCurrentLazyWriterActivity (
    VOID)
{
    KIRQL OldIrql;
    KEVENT WaitEvent;
    PWORK_QUEUE_ENTRY WorkItem;

    /* Allocate a work item */
    WorkItem = ExAllocateFromNPagedLookasideList(&CcTwilightLookasideList);
    if (WorkItem == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* We want lazy writer to set our event */
    WorkItem->Function = SetDone;
    KeInitializeEvent(&WaitEvent, NotificationEvent, FALSE);
    WorkItem->Parameters.Event.Event = &WaitEvent;

    /* Use the post tick queue */
    OldIrql = KeAcquireQueuedSpinLock(LockQueueMasterLock);
    InsertTailList(&CcPostTickWorkQueue, &WorkItem->WorkQueueLinks);

    /* Inform the lazy writer it will have to handle the post tick queue */
    LazyWriter.OtherWork = TRUE;
    /* And if it's not running, queue a lazy writer run
     * And start it NOW, we want the response now
     */
    if (!LazyWriter.ScanActive)
    {
        CcScheduleLazyWriteScan(TRUE);
    }

    KeReleaseQueuedSpinLock(LockQueueMasterLock, OldIrql);

    /* And now, wait until lazy writer replies */
    return KeWaitForSingleObject(&WaitEvent, Executive, KernelMode, FALSE, NULL);
}
