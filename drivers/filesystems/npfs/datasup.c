/*
 * PROJECT:     ReactOS Named Pipe FileSystem
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * FILE:        drivers/filesystems/npfs/datasup.c
 * PURPOSE:     Data Queues Support
 * PROGRAMMERS: ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "npfs.h"

// File ID number for NPFS bugchecking support
#define NPFS_BUGCHECK_FILE_ID   (NPFS_BUGCHECK_DATASUP)

/* FUNCTIONS ******************************************************************/

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
        if (!QuotaLeft) break;

        DataQueueEntry = CONTAINING_RECORD(NextEntry,
                                           NP_DATA_QUEUE_ENTRY,
                                           QueueEntry);

        Irp = DataQueueEntry->Irp;

        if ((DataQueueEntry->DataEntryType == Buffered) && (Irp))
        {
            DataLeft = DataQueueEntry->DataSize - ByteOffset;

            if (DataQueueEntry->QuotaInEntry < DataLeft)
            {
                NewQuotaLeft = DataLeft - DataQueueEntry->QuotaInEntry;
                if (NewQuotaLeft > QuotaLeft) NewQuotaLeft = QuotaLeft;

                QuotaLeft -= NewQuotaLeft;
                DataQueueEntry->QuotaInEntry += NewQuotaLeft;

                if (DataQueueEntry->QuotaInEntry == DataLeft &&
                    IoSetCancelRoutine(Irp, NULL))
                {
                    DataQueueEntry->Irp = NULL;

                    Irp->IoStatus.Status = STATUS_SUCCESS;
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

    if (DataQueue->QueueState == Empty)
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

        HasWrites = TRUE;
        if (DataQueue->QueueState != WriteEntries ||
            DataQueue->QuotaUsed < DataQueue->Quota ||
            !QueueEntry->QuotaInEntry)
        {
            HasWrites = FALSE;
        }

        DataQueue->QuotaUsed -= QueueEntry->QuotaInEntry;

        if (IsListEmpty(&DataQueue->Queue))
        {
            DataQueue->QueueState = Empty;
            HasWrites = FALSE;
        }

        Irp = QueueEntry->Irp;
        NpFreeClientSecurityContext(QueueEntry->ClientSecurityContext);

        if (Irp && !IoSetCancelRoutine(Irp, NULL))
        {
            Irp->Tail.Overlay.DriverContext[3] = NULL;
            Irp = NULL;
        }

        ExFreePool(QueueEntry);

        if (Flag)
        {
            NpGetNextRealDataQueueEntry(DataQueue, List);
        }

        if (HasWrites)
        {
            NpCompleteStalledWrites(DataQueue, List);
        }
    }

    DataQueue->ByteOffset = 0;
    return Irp;
}

PLIST_ENTRY
NTAPI
NpGetNextRealDataQueueEntry(IN PNP_DATA_QUEUE DataQueue,
                            IN PLIST_ENTRY List)
{
    PNP_DATA_QUEUE_ENTRY DataEntry;
    ULONG Type;
    PIRP Irp;
    PLIST_ENTRY NextEntry;
    PAGED_CODE();

    for (NextEntry = DataQueue->Queue.Flink;
         NextEntry != &DataQueue->Queue;
         NextEntry = DataQueue->Queue.Flink)
    {
        DataEntry = CONTAINING_RECORD(NextEntry,
                                      NP_DATA_QUEUE_ENTRY,
                                      QueueEntry);

        Type = DataEntry->DataEntryType;
        if (Type == Buffered || Type == Unbuffered) break;

        Irp = NpRemoveDataQueueEntry(DataQueue, FALSE, List);
        if (Irp)
        {
            Irp->IoStatus.Status = STATUS_SUCCESS;
            InsertTailList(List, &Irp->Tail.Overlay.ListEntry);
        }
    }

    return NextEntry;
}

VOID
NTAPI
NpCancelDataQueueIrp(IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP Irp)
{
    PNP_DATA_QUEUE DataQueue;
    PNP_DATA_QUEUE_ENTRY DataEntry;
    LIST_ENTRY DeferredList;
    PSECURITY_CLIENT_CONTEXT ClientSecurityContext;
    BOOLEAN CompleteWrites, FirstEntry;

    if (DeviceObject) IoReleaseCancelSpinLock(Irp->CancelIrql);

    InitializeListHead(&DeferredList);

    DataQueue = Irp->Tail.Overlay.DriverContext[2];
    ClientSecurityContext = NULL;

    if (DeviceObject)
    {
        FsRtlEnterFileSystem();
        NpAcquireExclusiveVcb();
    }

    DataEntry = Irp->Tail.Overlay.DriverContext[3];
    if (DataEntry)
    {
        if (DataEntry->QueueEntry.Blink == &DataQueue->Queue)
        {
            DataQueue->ByteOffset = 0;
            FirstEntry = TRUE;
        }
        else
        {
            FirstEntry = FALSE;
        }

        RemoveEntryList(&DataEntry->QueueEntry);

        ClientSecurityContext = DataEntry->ClientSecurityContext;

        CompleteWrites = TRUE;
        if (DataQueue->QueueState != WriteEntries ||
            DataQueue->QuotaUsed < DataQueue->Quota ||
            !DataEntry->QuotaInEntry)
        {
            CompleteWrites = FALSE;
        }

        DataQueue->BytesInQueue -= DataEntry->DataSize;
        DataQueue->QuotaUsed -= DataEntry->QuotaInEntry;
        --DataQueue->EntriesInQueue;

        if (IsListEmpty(&DataQueue->Queue))
        {
            DataQueue->QueueState = Empty;
            ASSERT(DataQueue->BytesInQueue == 0);
            ASSERT(DataQueue->EntriesInQueue == 0);
            ASSERT(DataQueue->QuotaUsed == 0);
        }
        else
        {
            if (FirstEntry)
            {
                NpGetNextRealDataQueueEntry(DataQueue, &DeferredList);
            }
            if (CompleteWrites)
            {
                NpCompleteStalledWrites(DataQueue, &DeferredList);
            }
        }
    }

    if (DeviceObject)
    {
        NpReleaseVcb();
        FsRtlExitFileSystem();
    }

    if (DataEntry) ExFreePool(DataEntry);

    NpFreeClientSecurityContext(ClientSecurityContext);
    Irp->IoStatus.Status = STATUS_CANCELLED;
    IoCompleteRequest(Irp, IO_NAMED_PIPE_INCREMENT);

    NpCompleteDeferredIrps(&DeferredList);
}

NTSTATUS
NTAPI
NpAddDataQueueEntry(IN ULONG NamedPipeEnd,
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
        Status = NpGetClientSecurityContext(NamedPipeEnd,
                                            Ccb,
                                            Irp ? Irp->Tail.Overlay.Thread :
                                            PsGetCurrentThread(),
                                            &ClientContext);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
    }

    switch (Type)
    {
        case Unbuffered:
        case 2:
        case 3:

            ASSERT(Irp != NULL);
            DataEntry = ExAllocatePoolWithQuotaTag(NonPagedPool | POOL_QUOTA_FAIL_INSTEAD_OF_RAISE,
                                                   sizeof(*DataEntry),
                                                   NPFS_DATA_ENTRY_TAG);
            if (!DataEntry)
            {
                NpFreeClientSecurityContext(ClientContext);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            DataEntry->DataEntryType = Type;
            DataEntry->QuotaInEntry = 0;
            DataEntry->Irp = Irp;
            DataEntry->DataSize = DataSize;
            DataEntry->ClientSecurityContext = ClientContext;
            ASSERT((DataQueue->QueueState == Empty) || (DataQueue->QueueState == Who));
            Status = STATUS_PENDING;
            break;

        case Buffered:

            EntrySize = sizeof(*DataEntry);
            if (Who != ReadEntries)
            {
                EntrySize += DataSize;
                if (EntrySize < DataSize)
                {
                    NpFreeClientSecurityContext(ClientContext);
                    return STATUS_INVALID_PARAMETER;
                }
            }

            QuotaInEntry = DataSize - ByteOffset;
            if (DataQueue->Quota - DataQueue->QuotaUsed < QuotaInEntry)
            {
                QuotaInEntry = DataQueue->Quota - DataQueue->QuotaUsed;
                HasSpace = TRUE;
            }
            else
            {
                HasSpace = FALSE;
            }

            DataEntry = ExAllocatePoolWithQuotaTag(NonPagedPool | POOL_QUOTA_FAIL_INSTEAD_OF_RAISE,
                                                   EntrySize,
                                                   NPFS_DATA_ENTRY_TAG);
            if (!DataEntry)
            {
                NpFreeClientSecurityContext(ClientContext);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            DataEntry->QuotaInEntry = QuotaInEntry;
            DataEntry->Irp = Irp;
            DataEntry->DataEntryType = Buffered;
            DataEntry->ClientSecurityContext = ClientContext;
            DataEntry->DataSize = DataSize;

            if (Who == ReadEntries)
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
                    _SEH2_YIELD(return _SEH2_GetExceptionCode());
                }
                _SEH2_END;

                if (HasSpace && Irp)
                {
                    Status = STATUS_PENDING;
                }
                else
                {
                    DataEntry->Irp = NULL;
                    Status = STATUS_SUCCESS;
                }

                ASSERT((DataQueue->QueueState == Empty) ||
                       (DataQueue->QueueState == Who));
            }
            break;

        default:
            ASSERT(FALSE);
            NpFreeClientSecurityContext(ClientContext);
            return STATUS_INVALID_PARAMETER;
    }

    ASSERT((DataQueue->QueueState == Empty) || (DataQueue->QueueState == Who));
    if (DataQueue->QueueState == Empty)
    {
        ASSERT(DataQueue->BytesInQueue == 0);
        ASSERT(DataQueue->EntriesInQueue == 0);
        ASSERT(IsListEmpty(&DataQueue->Queue));
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
    DataQueue->EntriesInQueue++;

    if (ByteOffset)
    {
        DataQueue->ByteOffset = ByteOffset;
        ASSERT(Who == WriteEntries);
        ASSERT(ByteOffset < DataEntry->DataSize);
        ASSERT(DataQueue->EntriesInQueue == 1);
    }

    InsertTailList(&DataQueue->Queue, &DataEntry->QueueEntry);

    if (Status == STATUS_PENDING)
    {
        IoMarkIrpPending(Irp);
        Irp->Tail.Overlay.DriverContext[2] = DataQueue;
        Irp->Tail.Overlay.DriverContext[3] = DataEntry;

        IoSetCancelRoutine(Irp, NpCancelDataQueueIrp);

        if ((Irp->Cancel) && (IoSetCancelRoutine(Irp, NULL)))
        {
            NpCancelDataQueueIrp(NULL, Irp);
        }
    }

    return Status;
}

/* EOF */
