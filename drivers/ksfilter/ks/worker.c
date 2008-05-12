/* ===============================================================
    Worker Management Functions
*/

#include <ntddk.h>
#include <debug.h>
#include <ks.h>

/*
    @unimplemented
*/
KSDDKAPI NTSTATUS NTAPI
KsRegisterWorker(
    IN  WORK_QUEUE_TYPE WorkQueueType,
    OUT PKSWORKER* Worker)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
KSDDKAPI VOID NTAPI
KsUnregisterWorker(
    IN  PKSWORKER Worker)
{
    UNIMPLEMENTED;
}

/*
    @unimplemented
*/
KSDDKAPI NTSTATUS NTAPI
KsRegisterCountedWorker(
    IN  WORK_QUEUE_TYPE WorkQueueType,
    IN  PWORK_QUEUE_ITEM CountedWorkItem,
    OUT PKSWORKER* Worker)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
KSDDKAPI ULONG NTAPI
KsDecrementCountedWorker(
    IN  PKSWORKER Worker)
{
    UNIMPLEMENTED;
    return 0;
}

/*
    @unimplemented
*/
KSDDKAPI ULONG NTAPI
KsIncrementCountedWorker(
    IN  PKSWORKER Worker)
{
    UNIMPLEMENTED;
    return 0;
}

/*
    @unimplemented
*/
KSDDKAPI NTSTATUS NTAPI
KsQueueWorkItem(
    IN  PKSWORKER Worker,
    IN  PWORK_QUEUE_ITEM WorkItem)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}
