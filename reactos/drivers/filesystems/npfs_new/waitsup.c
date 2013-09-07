#include "npfs.h"

VOID
NTAPI
NpInitializeWaitQueue(IN PNP_WAIT_QUEUE WaitQueue)
{
    InitializeListHead(&WaitQueue->WaitList);
    KeInitializeSpinLock(&WaitQueue->WaitLock);
}

NTSTATUS
NTAPI
NpCancelWaiter(IN PNP_WAIT_QUEUE WaitQueue,
               IN PUNICODE_STRING PipeName,
               IN NTSTATUS Status,
               IN PLIST_ENTRY ListEntry)
{
    UNICODE_STRING DestinationString;
    KIRQL OldIrql;
    PWCHAR Buffer;

    Buffer = ExAllocatePoolWithTag(NonPagedPool, PipeName->Length,'tFpN');
    if (!Buffer) return STATUS_INSUFFICIENT_RESOURCES;

    RtlInitEmptyUnicodeString(&DestinationString, Buffer, PipeName->Length);
    RtlUpcaseUnicodeString(&DestinationString, PipeName, FALSE);

    OldIrql = KfAcquireSpinLock(&WaitQueue->WaitLock);

    ASSERT(IsListEmpty(&WaitQueue->WaitList) == TRUE);

    KfReleaseSpinLock(&WaitQueue->WaitLock, OldIrql);
    ExFreePool(DestinationString.Buffer);
    return STATUS_SUCCESS;
}
