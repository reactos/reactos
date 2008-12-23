/* ===============================================================
    Worker Management Functions
*/

#include <ntddk.h>
#include <debug.h>
#include <ks.h>


typedef struct
{
    KEVENT Event;
    KSPIN_LOCK Lock;
    WORK_QUEUE_TYPE Type;
    LONG Counter;
    PWORK_QUEUE_ITEM WorkItem;
    ULONG WorkItemActive;
    ULONG DeleteInProgress;
}KS_WORKER;

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
    KS_WORKER * KsWorker;
    UNIMPLEMENTED;

    if (WorkQueueType != CriticalWorkQueue && 
        WorkQueueType != DelayedWorkQueue &&
        WorkQueueType != HyperCriticalWorkQueue)
    {
        return STATUS_INVALID_PARAMETER;
    }

    KsWorker = ExAllocatePoolWithTag(NonPagedPool, sizeof(KS_WORKER), 0);
    if (!KsWorker)
        return STATUS_INSUFFICIENT_RESOURCES;

    KsWorker->Type = WorkQueueType;
    KsWorker->Counter = 0;
    KsWorker->WorkItemActive = 0;
    KsWorker->WorkItem = NULL;
    KsWorker->DeleteInProgress = TRUE;
    KeInitializeSpinLock(&KsWorker->Lock);
    KeInitializeEvent(&KsWorker->Event, NotificationEvent, FALSE);

    *Worker = KsWorker;
    return STATUS_SUCCESS;
}

/*
    @implemented
*/
KSDDKAPI VOID NTAPI
KsUnregisterWorker(
    IN  PKSWORKER Worker)
{
    KS_WORKER * KsWorker;
    KIRQL OldIrql = 0; // hack!!! janderwald!!!
    //ULONG bWait = FALSE;

    if (!Worker)
        return;

    KsWorker = (KS_WORKER *)Worker;

    KsWorker->DeleteInProgress = TRUE;

    if (KsWorker->WorkItemActive)
    {
        KeReleaseSpinLock(&KsWorker->Lock, OldIrql);
        KeWaitForSingleObject(&KsWorker->Event, Executive, KernelMode, FALSE, NULL);
    }
    else
    {
        KeReleaseSpinLock(&KsWorker->Lock, OldIrql);
    }

    ExFreePoolWithTag(KsWorker, 0);
}

/*
    @implemented
*/
KSDDKAPI NTSTATUS NTAPI
KsRegisterCountedWorker(
    IN  WORK_QUEUE_TYPE WorkQueueType,
    IN  PWORK_QUEUE_ITEM CountedWorkItem,
    OUT PKSWORKER* Worker)
{
    NTSTATUS Status;
    KS_WORKER * KsWorker;

    Status = KsRegisterWorker(WorkQueueType, Worker);

    if (NT_SUCCESS(Status))
    {
        KsWorker = (KS_WORKER *)Worker;
        KsWorker->WorkItem = CountedWorkItem;
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
    KS_WORKER * KsWorker;
    LONG Counter;

    if (!Worker)
        return STATUS_INVALID_PARAMETER;

    KsWorker = (KS_WORKER *)Worker;
    Counter = InterlockedDecrement(&KsWorker->Counter);

    if (KsWorker->DeleteInProgress)
    {
        /* signal that we are done */
        KeSetEvent(&KsWorker->Event, 0, 0);
    }

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
    KS_WORKER * KsWorker;
    LONG Counter;

    if (!Worker)
        return STATUS_INVALID_PARAMETER;

    KsWorker = (KS_WORKER *)Worker;

    Counter = InterlockedIncrement(&KsWorker->Counter);
    if (Counter == 1)
    {
        KsQueueWorkItem(Worker, KsWorker->WorkItem);
    }
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
    KS_WORKER * KsWorker;
    KIRQL OldIrql;
    NTSTATUS Status = STATUS_SUCCESS;

    if (!Worker || !WorkItem)
        return STATUS_INVALID_PARAMETER;

    KsWorker = (KS_WORKER *)Worker;
    KeAcquireSpinLock(&KsWorker->Lock, &OldIrql);

    if (!KsWorker->DeleteInProgress)
    {
        ExQueueWorkItem(WorkItem, KsWorker->Type);
        Status = STATUS_UNSUCCESSFUL;
    }

    KeReleaseSpinLock(&KsWorker->Lock, OldIrql);
    return Status;
}
