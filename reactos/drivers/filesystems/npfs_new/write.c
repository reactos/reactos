#include "npfs.h"

LONG NpSlowWriteCalls;

BOOLEAN
NTAPI
NpCommonWrite(IN PFILE_OBJECT FileObject,
              IN PVOID Buffer,
              IN ULONG DataSize, 
              IN PETHREAD Thread, 
              IN PIO_STATUS_BLOCK IoStatus, 
              IN PIRP Irp, 
              IN PLIST_ENTRY List)
{
    NODE_TYPE_CODE NodeType;
    BOOLEAN WriteOk, ServerSide;
    PNP_CCB Ccb;
    PNP_NONPAGED_CCB NonPagedCcb;
    PNP_DATA_QUEUE WriteQueue;
    NTSTATUS Status;
    PNP_EVENT_BUFFER EventBuffer;
    ULONG BytesWritten;
    PAGED_CODE();

    IoStatus->Information = 0;
    NodeType = NpDecodeFileObject(FileObject, NULL, &Ccb, &ServerSide);

    if (!NodeType)
    {
        IoStatus->Status = STATUS_PIPE_DISCONNECTED;
        return TRUE;
    }

    if ( NodeType != NPFS_NTC_CCB )
    {
        IoStatus->Status = STATUS_INVALID_PARAMETER;
        return TRUE;
    }

    NonPagedCcb = Ccb->NonPagedCcb;
    ExAcquireResourceExclusiveLite(&NonPagedCcb->Lock, TRUE);

   // ms_exc.registration.TryLevel = 0;

    if ( Ccb->NamedPipeState == FILE_PIPE_DISCONNECTED_STATE )
    {
        IoStatus->Status = STATUS_PIPE_DISCONNECTED;
        WriteOk = TRUE;
        goto Quickie;
    }

    if ( Ccb->NamedPipeState == FILE_PIPE_LISTENING_STATE || Ccb->NamedPipeState == FILE_PIPE_CLOSING_STATE )
    {
        IoStatus->Status = Ccb->NamedPipeState != FILE_PIPE_LISTENING_STATE ? STATUS_PIPE_LISTENING : STATUS_PIPE_CLOSING;
        WriteOk = TRUE;
        goto Quickie;
    }

    ASSERT(Ccb->NamedPipeState == FILE_PIPE_CONNECTED_STATE);

    if ((ServerSide == 1 && Ccb->Fcb->NamedPipeConfiguration == FILE_PIPE_INBOUND) ||
        (ServerSide == 0 && Ccb->Fcb->NamedPipeConfiguration == FILE_PIPE_OUTBOUND))
    {
        IoStatus->Status = STATUS_INVALID_PARAMETER;
        WriteOk = TRUE;
        goto Quickie;
    }

    IoStatus->Status = 0;
    IoStatus->Information = DataSize;

    if ( ServerSide == 1 )
    {
        WriteQueue = &Ccb->OutQueue;
        EventBuffer = NonPagedCcb->EventBufferClient;
    }
    else
    {
        WriteQueue = &Ccb->InQueue;
        EventBuffer = NonPagedCcb->EventBufferServer;
    }

    if (WriteQueue->QueueState != ReadEntries ||
        WriteQueue->BytesInQueue >= DataSize ||
        WriteQueue->Quota >= DataSize - WriteQueue->BytesInQueue )
    {
        if (WriteQueue->QueueState != ReadEntries ||
            WriteQueue->Quota - WriteQueue->QuotaUsed >= DataSize )
        {
            goto DoWrite;
        }
    }

    if (Ccb->Fcb->NamedPipeType == FILE_PIPE_BYTE_STREAM_TYPE &&
        Ccb->ServerCompletionMode & FILE_PIPE_COMPLETE_OPERATION)
    {
        IoStatus->Information = 0;
        IoStatus->Status = 0;
        WriteOk = TRUE;
        goto Quickie;
    }

    if (!Irp )
    {
        WriteOk = 0;
        goto Quickie;
    }

DoWrite:
    Status = NpWriteDataQueue(WriteQueue,
                              ServerSide ? Ccb->ClientReadMode : Ccb->ServerReadMode,
                              Buffer,
                              DataSize,
                              Ccb->Fcb->NamedPipeType,
                              &BytesWritten,
                              Ccb,
                              ServerSide,
                              Thread,
                              List);
    IoStatus->Status = Status;
    if ( Status == STATUS_MORE_PROCESSING_REQUIRED )
    {
        ASSERT(WriteQueue->QueueState != ReadEntries);
        if ( (Ccb->ServerCompletionMode & FILE_PIPE_COMPLETE_OPERATION || !Irp)
            && WriteQueue->Quota - WriteQueue->QuotaUsed < BytesWritten )
        {
            IoStatus->Information = DataSize - BytesWritten;
            IoStatus->Status = 0;
        }
        else
        {
            ASSERT(WriteQueue->QueueState != ReadEntries);

            IoStatus->Status = NpAddDataQueueEntry(ServerSide,
                                                    Ccb,
                                                    WriteQueue,
                                                    WriteEntries,
                                                    0,
                                                    DataSize,
                                                    Irp,
                                                    Buffer,
                                                    DataSize - BytesWritten);
        }
    }

    if ( EventBuffer ) KeSetEvent(EventBuffer->Event, 0, 0);
    WriteOk = 1;

Quickie:
    //ms_exc.registration.TryLevel = -1;
    ExReleaseResourceLite(&Ccb->NonPagedCcb->Lock);
    return WriteOk;
}

NTSTATUS
NTAPI
NpFsdWrite(IN PDEVICE_OBJECT DeviceObject,
           IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    IO_STATUS_BLOCK IoStatus;
    LIST_ENTRY List;
    PLIST_ENTRY NextEntry, ThisEntry;
    PAGED_CODE();
    NpSlowWriteCalls++;

    InitializeListHead(&List);
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    FsRtlEnterFileSystem();
    ExAcquireResourceSharedLite(&NpVcb->Lock, TRUE);

    NpCommonWrite(IoStack->FileObject,
                  Irp->UserBuffer,
                  IoStack->Parameters.Write.Length,
                  Irp->Tail.Overlay.Thread,
                  &IoStatus,
                  Irp,
                  &List);

    ExReleaseResourceLite(&NpVcb->Lock);

    NextEntry = List.Flink;
    while (NextEntry != &List)
    {
        ThisEntry = NextEntry;
        NextEntry = NextEntry->Flink;

        Irp = CONTAINING_RECORD(ThisEntry, IRP, Tail.Overlay.ListEntry);
        IoCompleteRequest(Irp, IO_DISK_INCREMENT);
    }

    FsRtlExitFileSystem();

    if ( IoStatus.Status != STATUS_PENDING )
    {
        Irp->IoStatus.Information = IoStatus.Information;
        Irp->IoStatus.Status = IoStatus.Status;
        IoCompleteRequest(Irp, IO_NAMED_PIPE_INCREMENT);
    }

    return IoStatus.Status;
}

