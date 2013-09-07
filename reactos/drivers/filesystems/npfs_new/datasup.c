#include "npfs.h"

NTSTATUS
NTAPI
NpUninitializeDataQueue(IN PNP_DATA_QUEUE DataQueue)
{
    PAGED_CODE();

    ASSERT(DataQueue->QueueState == Empty);

    RtlZeroMemory(DataQueue, sizeof(*DataQueue));
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
NpInitializeDataQueue(IN PNP_DATA_QUEUE DataQueue,
                      IN ULONG Quota)
{
    PAGED_CODE();

    DataQueue->BytesInQueue = 0;
    DataQueue->EntriesInQueue = 0;
    DataQueue->QuotaUsed = 0;
    DataQueue->ByteOffset = 0;
    DataQueue->QueueState = Empty;
    DataQueue->Quota = Quota;
    InitializeListHead(&DataQueue->List);
    return STATUS_SUCCESS;
}
