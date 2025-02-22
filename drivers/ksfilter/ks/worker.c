/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/ksfilter/ks/worker.c
 * PURPOSE:         KS Allocator functions
 * PROGRAMMER:      Johannes Anderwald
 */

#include "precomp.h"

#define NDEBUG
#include <debug.h>

/* ===============================================================
    Worker Management Functions
*/

typedef struct
{
    WORK_QUEUE_ITEM WorkItem;

    KEVENT Event;
    KSPIN_LOCK Lock;
    WORK_QUEUE_TYPE Type;
    LONG Counter;
    LONG QueuedWorkItemCount;
    LIST_ENTRY QueuedWorkItems;

    PWORK_QUEUE_ITEM CountedWorkItem;
}KSIWORKER, *PKSIWORKER;

VOID
NTAPI
WorkItemRoutine(
    IN PVOID Context)
{
    PKSIWORKER KsWorker;
    KIRQL OldLevel;
    PWORK_QUEUE_ITEM WorkItem;
    PLIST_ENTRY Entry;


    /* get ks worker implementation */
    KsWorker = (PKSIWORKER)Context;

    /* acquire back the lock */
    KeAcquireSpinLock(&KsWorker->Lock, &OldLevel);

    do
    {
        /* sanity check */
        ASSERT(!IsListEmpty(&KsWorker->QueuedWorkItems));

        /* remove first entry */
        Entry = RemoveHeadList(&KsWorker->QueuedWorkItems);
        /* get offset to work item */
        WorkItem = (PWORK_QUEUE_ITEM)CONTAINING_RECORD(Entry, WORK_QUEUE_ITEM, List);

        /* release lock as the callback might call one KsWorker functions */
        KeReleaseSpinLock(&KsWorker->Lock, OldLevel);

        /*  now dispatch the work */
        WorkItem->WorkerRoutine(WorkItem->Parameter);

        /* acquire back the lock */
        KeAcquireSpinLock(&KsWorker->Lock, &OldLevel);

        /* decrement queued work item count */
        InterlockedDecrement(&KsWorker->QueuedWorkItemCount);

    }while(KsWorker->QueuedWorkItemCount);

    /* release the lock */
    KeReleaseSpinLock(&KsWorker->Lock, OldLevel);

    /* signal completion event */
    KeSetEvent(&KsWorker->Event, IO_NO_INCREMENT, FALSE);

}


/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsRegisterWorker(
    IN  WORK_QUEUE_TYPE WorkQueueType,
    OUT PKSWORKER* Worker)
{
    PKSIWORKER KsWorker;


    if (WorkQueueType != CriticalWorkQueue &&
        WorkQueueType != DelayedWorkQueue &&
        WorkQueueType != HyperCriticalWorkQueue)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* allocate worker context */
    KsWorker = AllocateItem(NonPagedPool, sizeof(KSIWORKER));
    if (!KsWorker)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* initialize the work ctx */
    ExInitializeWorkItem(&KsWorker->WorkItem, WorkItemRoutine, (PVOID)KsWorker);
    /* setup type */
    KsWorker->Type = WorkQueueType;
    /* Initialize work item queue */
    InitializeListHead(&KsWorker->QueuedWorkItems);
    /* initialize work item lock */
    KeInitializeSpinLock(&KsWorker->Lock);
    /* initialize event */
    KeInitializeEvent(&KsWorker->Event, NotificationEvent, FALSE);

    *Worker = KsWorker;
    return STATUS_SUCCESS;
}

/*
    @implemented
*/
KSDDKAPI
VOID
NTAPI
KsUnregisterWorker(
    IN  PKSWORKER Worker)
{
    PKSIWORKER KsWorker;
    KIRQL OldIrql;

    if (!Worker)
        return;

    /* get ks worker implementation */
    KsWorker = (PKSIWORKER)Worker;
    /* acquire spinlock */
    KeAcquireSpinLock(&KsWorker->Lock, &OldIrql);
    /* fake status running to avoid work items to be queued by the counted worker */
    KsWorker->Counter = 1;
    /* is there currently a work item active */
    if (KsWorker->QueuedWorkItemCount)
    {
        /* release the lock */
        KeReleaseSpinLock(&KsWorker->Lock, OldIrql);
        /* wait for the worker routine to finish */
        KeWaitForSingleObject(&KsWorker->Event, Executive, KernelMode, FALSE, NULL);
    }
    else
    {
        /* no work item active, just release the lock */
        KeReleaseSpinLock(&KsWorker->Lock, OldIrql);
    }
    /* free worker context */
    FreeItem(KsWorker);
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsRegisterCountedWorker(
    IN  WORK_QUEUE_TYPE WorkQueueType,
    IN  PWORK_QUEUE_ITEM CountedWorkItem,
    OUT PKSWORKER* Worker)
{
    NTSTATUS Status;
    PKSIWORKER KsWorker;

    /* check for counted work item parameter */
    if (!CountedWorkItem)
        return STATUS_INVALID_PARAMETER_2;

    /* create the work ctx */
    Status = KsRegisterWorker(WorkQueueType, Worker);
    /* check for success */
    if (NT_SUCCESS(Status))
    {
        /* get ks worker implementation */
        KsWorker = *(PKSIWORKER*)Worker;
        /* store counted work item */
        KsWorker->CountedWorkItem = CountedWorkItem;
    }

    return Status;
}

/*
    @implemented
*/
KSDDKAPI
ULONG
NTAPI
KsDecrementCountedWorker(
    IN  PKSWORKER Worker)
{
    PKSIWORKER KsWorker;
    LONG Counter;

    /* did the caller pass a work ctx */
    if (!Worker)
        return STATUS_INVALID_PARAMETER;

    /* get ks worker implementation */
    KsWorker = (PKSIWORKER)Worker;
    /* decrement counter */
    Counter = InterlockedDecrement(&KsWorker->Counter);
    /* return result */
    return Counter;
}

/*
    @implemented
*/
KSDDKAPI
ULONG
NTAPI
KsIncrementCountedWorker(
    IN  PKSWORKER Worker)
{
    PKSIWORKER KsWorker;
    LONG Counter;

    /* did the caller pass a work ctx */
    if (!Worker)
        return STATUS_INVALID_PARAMETER;

    /* get ks worker implementation */
    KsWorker = (PKSIWORKER)Worker;
    /* increment counter */
    Counter = InterlockedIncrement(&KsWorker->Counter);
    if (Counter == 1)
    {
        /* this is the first work item in list, so queue a real work item */
        KsQueueWorkItem(Worker, KsWorker->CountedWorkItem);
    }

    /* return current counter */
    return Counter;
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsQueueWorkItem(
    IN  PKSWORKER Worker,
    IN  PWORK_QUEUE_ITEM WorkItem)
{
    PKSIWORKER KsWorker;
    KIRQL OldIrql;

    /* check for all parameters */
    if (!Worker || !WorkItem)
        return STATUS_INVALID_PARAMETER;

    /* get ks worker implementation */
    KsWorker = (PKSIWORKER)Worker;
    /* lock the work queue */
    KeAcquireSpinLock(&KsWorker->Lock, &OldIrql);
    /* insert work item to list */
    InsertTailList(&KsWorker->QueuedWorkItems, &WorkItem->List);
    /* increment active count */
    InterlockedIncrement(&KsWorker->QueuedWorkItemCount);
    /* is this the first work item */
    if (KsWorker->QueuedWorkItemCount == 1)
    {
        /* clear event */
        KeClearEvent(&KsWorker->Event);
        /* it is, queue it */
        ExQueueWorkItem(&KsWorker->WorkItem, KsWorker->Type);
    }
    /* release lock */
    KeReleaseSpinLock(&KsWorker->Lock, OldIrql);

    return STATUS_SUCCESS;
}
