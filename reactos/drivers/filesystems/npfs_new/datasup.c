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
    InitializeListHead(&DataQueue->Queue);
    return STATUS_SUCCESS;
}

VOID
NTAPI
NpCompleteStalledWrites(IN PNP_DATA_QUEUE DataQueue,
                        IN PLIST_ENTRY List)
{
    ULONG QuotaLeft, ByteOffset, DataLeft, NewQuotaLeft;
    PNP_DATA_QUEUE_ENTRY DataQueueEntry;
    PIRP Irp;
    PLIST_ENTRY NextEntry;

    QuotaLeft = DataQueue->Quota - DataQueue->QuotaUsed;
    ByteOffset = DataQueue->ByteOffset;

    NextEntry = DataQueue->Queue.Flink;
    while (NextEntry != &DataQueue->Queue)
    {
        if ( !QuotaLeft ) break;

        DataQueueEntry = CONTAINING_RECORD(NextEntry, NP_DATA_QUEUE_ENTRY, Irp);

        Irp = DataQueueEntry->Irp;

        if ((DataQueueEntry->DataEntryType == 0) && (Irp))
        {
            DataLeft = DataQueueEntry->DataSize - ByteOffset;

            if ( DataQueueEntry->QuotaInEntry < DataLeft )
            {
                NewQuotaLeft = DataLeft - DataQueueEntry->QuotaInEntry;
                if ( NewQuotaLeft > QuotaLeft ) NewQuotaLeft = QuotaLeft;

                QuotaLeft -= NewQuotaLeft;
                DataQueueEntry->QuotaInEntry += NewQuotaLeft;

                if (DataQueueEntry->QuotaInEntry == DataLeft &&
                    IoSetCancelRoutine(Irp, NULL))
                {
                    DataQueueEntry->Irp = NULL;

                    Irp->IoStatus.Status = 0;
                    Irp->IoStatus.Information = DataQueueEntry->DataSize;

                    InsertTailList(List, &Irp->Tail.Overlay.ListEntry);
                }
            }
        }

        NextEntry = NextEntry->Flink;
        ByteOffset = 0;
    }

    DataQueue->QuotaUsed = DataQueue->Quota - QuotaLeft;
}

PIRP
NTAPI
NpRemoveDataQueueEntry(IN PNP_DATA_QUEUE DataQueue,
                       IN BOOLEAN Flag,
                       IN PLIST_ENTRY List)
{
    PIRP Irp;
    PNP_DATA_QUEUE_ENTRY QueueEntry;
    BOOLEAN HasWrites;

    if ( DataQueue->QueueState == Empty)
    {
        Irp = NULL;
        ASSERT(IsListEmpty(&DataQueue->Queue));
        ASSERT(DataQueue->EntriesInQueue == 0);
        ASSERT(DataQueue->BytesInQueue == 0);
        ASSERT(DataQueue->QuotaUsed == 0);
    }
    else
    {
        QueueEntry = CONTAINING_RECORD(RemoveHeadList(&DataQueue->Queue),
                                       NP_DATA_QUEUE_ENTRY,
                                       QueueEntry);

        DataQueue->BytesInQueue -= QueueEntry->DataSize;
        --DataQueue->EntriesInQueue;

        HasWrites = 1;
        if ( !DataQueue->QueueState != WriteEntries || DataQueue->QuotaUsed < DataQueue->Quota || !QueueEntry->QuotaInEntry )
        {
            HasWrites = 0;
        }

        DataQueue->QuotaUsed -= QueueEntry->QuotaInEntry;

        if (DataQueue->Queue.Flink == &DataQueue->Queue)
        {
            DataQueue->QueueState = Empty;
            HasWrites = 0;
        }

        Irp = QueueEntry->Irp;
        NpFreeClientSecurityContext(QueueEntry->ClientSecurityContext);

        if (Irp && IoSetCancelRoutine(Irp, NULL))
        {
            Irp->Tail.Overlay.DriverContext[3] = 0;
        }

        ExFreePool(QueueEntry);

        if ( Flag )
        {
            NpGetNextRealDataQueueEntry(DataQueue, List);
        }

        if ( HasWrites )
        {
            NpCompleteStalledWrites(DataQueue, List);
        }
    }

    DataQueue->ByteOffset = 0;
    return Irp;
}

PNP_DATA_QUEUE_ENTRY
NTAPI
NpGetNextRealDataQueueEntry(IN PNP_DATA_QUEUE DataQueue,
                            IN PLIST_ENTRY List)
{
    PNP_DATA_QUEUE_ENTRY DataEntry;
    ULONG Type;
    PIRP Irp;
    PLIST_ENTRY NextEntry;
    PAGED_CODE();

    DataEntry = NULL;

    NextEntry = DataQueue->Queue.Flink;
    while (NextEntry != &DataQueue->Queue)
    {
        DataEntry = CONTAINING_RECORD(NextEntry, NP_DATA_QUEUE_ENTRY, QueueEntry);

        Type = DataEntry->DataEntryType;
        if ( Type == Buffered || Type == Unbuffered ) break;

        Irp = NpRemoveDataQueueEntry(DataQueue, 0, List);
        if ( Irp )
        {
            Irp->IoStatus.Status = STATUS_SUCCESS;
            InsertTailList(List, &Irp->Tail.Overlay.ListEntry);
        }
    }

    return DataEntry;
}

VOID
NTAPI
NpCancelDataQueueIrp(IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP Irp)
{
    PNP_DATA_QUEUE DataQueue;
    PNP_DATA_QUEUE_ENTRY DataEntry;
    LIST_ENTRY List;
    PSECURITY_CLIENT_CONTEXT ClientSecurityContext;
    BOOLEAN CompleteWrites, FirstEntry;
    PLIST_ENTRY NextEntry, ThisEntry;

    if ( DeviceObject ) IoReleaseCancelSpinLock(Irp->CancelIrql);

    InitializeListHead(&List);

    DataQueue = (PNP_DATA_QUEUE)Irp->Tail.Overlay.DriverContext[2];
    ClientSecurityContext = NULL;

    if ( DeviceObject )
    {
        FsRtlEnterFileSystem();
        ExAcquireResourceExclusiveLite(&NpVcb->Lock, TRUE);
    }

    DataEntry = (PNP_DATA_QUEUE_ENTRY)Irp->Tail.Overlay.DriverContext[3];
    if ( DataEntry )
    {
        if (DataEntry->QueueEntry.Blink == &DataQueue->Queue )
        {
            DataQueue->ByteOffset = 0;
            FirstEntry = 1;
        }
        else
        {
            FirstEntry = 0;
        }

        RemoveEntryList(&DataEntry->QueueEntry);

        ClientSecurityContext = DataEntry->ClientSecurityContext;

        CompleteWrites = 1;
        if ( !DataQueue->QueueState != WriteEntries || DataQueue->QuotaUsed < DataQueue->Quota || !DataEntry->QuotaInEntry )
        {
            CompleteWrites = 0;
        }

        DataQueue->BytesInQueue -= DataEntry->DataSize;
        DataQueue->QuotaUsed -= DataEntry->QuotaInEntry;
        --DataQueue->EntriesInQueue;

        if (DataQueue->Queue.Flink == &DataQueue->Queue )
        {
            DataQueue->QueueState = Empty;
            ASSERT(DataQueue->BytesInQueue == 0);
            ASSERT(DataQueue->EntriesInQueue == 0);
            ASSERT(DataQueue->QuotaUsed == 0);
        }
        else
        {
            if ( FirstEntry )
            {
                NpGetNextRealDataQueueEntry(DataQueue, &List);
            }
            if ( CompleteWrites )
            {
                NpCompleteStalledWrites(DataQueue, &List);
            }
        }
    }

    if ( DeviceObject )
    {
        ExReleaseResourceLite(&NpVcb->Lock);
        FsRtlExitFileSystem();
    }

    if ( DataEntry ) ExFreePool(DataEntry);

    NpFreeClientSecurityContext(ClientSecurityContext);
    Irp->IoStatus.Status = STATUS_CANCELLED;
    IofCompleteRequest(Irp, IO_DISK_INCREMENT);

    NextEntry = List.Flink;
    while (NextEntry != &List)
    {
        ThisEntry = NextEntry;
        NextEntry = NextEntry->Flink;

        Irp = CONTAINING_RECORD(ThisEntry, IRP, Tail.Overlay.ListEntry);
        IoCompleteRequest(Irp, IO_DISK_INCREMENT);
    }
}

NTSTATUS
NTAPI
NpAddDataQueueEntry(IN BOOLEAN ServerSide,
                    IN PNP_CCB Ccb,
                    IN PNP_DATA_QUEUE DataQueue,
                    IN ULONG Who, 
                    IN ULONG Type,
                    IN ULONG DataSize,
                    IN PIRP Irp,
                    IN PVOID Buffer,
                    IN ULONG ByteOffset)
{
    NTSTATUS Status;
    PNP_DATA_QUEUE_ENTRY DataEntry;
    SIZE_T EntrySize;
    ULONG QuotaInEntry;
    PSECURITY_CLIENT_CONTEXT ClientContext;
    BOOLEAN HasSpace;

    ClientContext = NULL;
    ASSERT((DataQueue->QueueState == Empty) || (DataQueue->QueueState == Who));

    Status = STATUS_SUCCESS;

    if ((Type != 2) && (Who == WriteEntries))
    {
        Status = NpGetClientSecurityContext(ServerSide,
                                            Ccb,
                                            Irp ? Irp->Tail.Overlay.Thread :
                                            PsGetCurrentThread(),
                                            &ClientContext);
        if (!NT_SUCCESS(Status)) return Status;
    }

    switch (Type)
    {
        case Unbuffered:
        case 2:
        case 3:

            ASSERT(Irp != NULL);
            DataEntry = ExAllocatePoolWithQuotaTag(NonPagedPool, sizeof(*DataEntry), 'rFpN');
            if ( DataEntry )
            {
                DataEntry->DataEntryType = Type;
                DataEntry->QuotaInEntry = 0;
                DataEntry->Irp = Irp;
                DataEntry->DataSize = DataSize;
                DataEntry->ClientSecurityContext = ClientContext;
                ASSERT((DataQueue->QueueState == Empty) || (DataQueue->QueueState == Who));
                Status = STATUS_PENDING;
                break;
            }

            NpFreeClientSecurityContext(ClientContext);
            return STATUS_INSUFFICIENT_RESOURCES;
    
        case Buffered:

            EntrySize = sizeof(*DataEntry);
            if ( Who != Empty)
            {
                EntrySize = DataSize + sizeof(*DataEntry);
                if ((DataSize + sizeof(*DataEntry)) < DataSize )
                {
                    NpFreeClientSecurityContext(ClientContext);
                    return STATUS_INVALID_PARAMETER;
                }
            }

            QuotaInEntry = DataSize - ByteOffset;
            if ( DataQueue->Quota - DataQueue->QuotaUsed < QuotaInEntry )
            {
                QuotaInEntry = DataQueue->Quota - DataQueue->QuotaUsed;
                HasSpace = 1;
            }
            else
            {
                HasSpace = 0;
            }

            DataEntry = ExAllocatePoolWithQuotaTag(NonPagedPool, EntrySize, 'rFpN');
            if ( !DataEntry )
            {
                NpFreeClientSecurityContext(ClientContext);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            DataEntry->QuotaInEntry = QuotaInEntry;
            DataEntry->Irp = Irp;
            DataEntry->DataEntryType = Buffered;
            DataEntry->ClientSecurityContext = ClientContext;
            DataEntry->DataSize = DataSize;

            if ( Who == ReadEntries)
            {
                ASSERT(Irp);

                Status = STATUS_PENDING;
                ASSERT((DataQueue->QueueState == Empty) ||
                       (DataQueue->QueueState == Who));
            }
            else
            {
                _SEH2_TRY
                {
                RtlCopyMemory(DataEntry + 1,
                              Irp ? Irp->UserBuffer: Buffer,
                              DataSize);
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    NpFreeClientSecurityContext(ClientContext);
                    return _SEH2_GetExceptionCode();
                }
                _SEH2_END;

                if ( HasSpace && Irp )
                {
                    Status = STATUS_PENDING;
                }
                else
                {
                    DataEntry->Irp = 0;
                    Status = STATUS_SUCCESS;
                }
                
                ASSERT((DataQueue->QueueState == Empty) || (DataQueue->QueueState == Who));
            }

        default:
            ASSERT(FALSE);
            NpFreeClientSecurityContext(ClientContext);
            return STATUS_INVALID_PARAMETER;
    }

    ASSERT((DataQueue->QueueState == Empty) || (DataQueue->QueueState == Who));
    if ( DataQueue->QueueState == Empty )
    {
        ASSERT(DataQueue->BytesInQueue == 0);
        ASSERT(DataQueue->EntriesInQueue == 0);
        ASSERT(IsListEmpty (&DataQueue->Queue));
    }
    else
    {
        ASSERT(DataQueue->QueueState == Who);
        ASSERT(DataQueue->QueueState != Empty);
        ASSERT(DataQueue->EntriesInQueue != 0);
    }

    DataQueue->QuotaUsed += DataEntry->QuotaInEntry;
    DataQueue->QueueState = Who;
    DataQueue->BytesInQueue += DataEntry->DataSize;
    ++DataQueue->EntriesInQueue;
    if ( ByteOffset )
    {
        DataQueue->ByteOffset = ByteOffset;
        ASSERT(Who == WriteEntries);
        ASSERT(ByteOffset < DataEntry->DataSize);
        ASSERT(DataQueue->EntriesInQueue == 1);
    }
                    
    InsertTailList(&DataQueue->Queue, &DataEntry->QueueEntry);

    if ( Status == STATUS_PENDING )
    {
        IoMarkIrpPending(Irp);
        Irp->Tail.Overlay.DriverContext[2] = DataQueue;
        Irp->Tail.Overlay.DriverContext[3] = DataEntry;

        IoSetCancelRoutine(Irp, NpCancelDataQueueIrp);

        if ( Irp->Cancel )
        {
            IoSetCancelRoutine(Irp, NULL);
            NpCancelDataQueueIrp(0, Irp);
        }
    }

    return Status;
}
