#include "npfs.h"

LONG NpSlowReadCalls;

BOOLEAN
NTAPI
NpCommonRead(IN PFILE_OBJECT FileObject,
             IN PVOID Buffer,
             IN ULONG BufferSize, 
             OUT PIO_STATUS_BLOCK IoStatus, 
             IN PIRP Irp,
             IN PLIST_ENTRY List)
{
    NODE_TYPE_CODE NodeType;
    PNP_DATA_QUEUE ReadQueue;
    PNP_EVENT_BUFFER EventBuffer;
    NTSTATUS Status;
    ULONG NamedPipeEnd;
    PNP_CCB Ccb;
    PNP_NONPAGED_CCB NonPagedCcb;
    BOOLEAN ReadOk;
    PAGED_CODE();

    IoStatus->Information = 0;
    NodeType = NpDecodeFileObject(FileObject, NULL, &Ccb, &NamedPipeEnd);

    if (!NodeType)
    {
        IoStatus->Status = STATUS_PIPE_DISCONNECTED;
        return TRUE;
    }

    if (NodeType != NPFS_NTC_CCB)
    {
        IoStatus->Status = STATUS_INVALID_PARAMETER;
        return TRUE;
    }

    NonPagedCcb = Ccb->NonPagedCcb;
    ExAcquireResourceExclusiveLite(&NonPagedCcb->Lock, TRUE);

    if (Ccb->NamedPipeState == FILE_PIPE_DISCONNECTED_STATE || Ccb->NamedPipeState == FILE_PIPE_LISTENING_STATE)
    {
        IoStatus->Status = Ccb->NamedPipeState != FILE_PIPE_DISCONNECTED_STATE ? STATUS_PIPE_LISTENING : STATUS_PIPE_DISCONNECTED;
        ReadOk = TRUE;
        goto Quickie;
    }

    ASSERT((Ccb->NamedPipeState == FILE_PIPE_CONNECTED_STATE) || (Ccb->NamedPipeState == FILE_PIPE_CLOSING_STATE));

    if ((NamedPipeEnd == FILE_PIPE_SERVER_END && Ccb->Fcb->NamedPipeConfiguration == FILE_PIPE_OUTBOUND) ||
        (NamedPipeEnd == FILE_PIPE_CLIENT_END && Ccb->Fcb->NamedPipeConfiguration == FILE_PIPE_INBOUND))
    {
        IoStatus->Status = STATUS_INVALID_PARAMETER;
        ReadOk = TRUE;
        goto Quickie;
    }

    if (NamedPipeEnd == FILE_PIPE_SERVER_END)
    {
        ReadQueue = &Ccb->DataQueue[FILE_PIPE_INBOUND];
    }
    else
    {
        ReadQueue = &Ccb->DataQueue[FILE_PIPE_OUTBOUND];
    }

    EventBuffer = NonPagedCcb->EventBuffer[NamedPipeEnd];

    if (ReadQueue->QueueState == WriteEntries)
    {
        *IoStatus = NpReadDataQueue(ReadQueue,
                                    FALSE,
                                    FALSE,
                                    Buffer,
                                    BufferSize,
                                    Ccb->ReadMode[NamedPipeEnd],
                                    Ccb,
                                    List);
        if (!NT_SUCCESS(IoStatus->Status))
        {
            ReadOk = TRUE;
            goto Quickie;
        }

        ReadOk = TRUE;
        if (EventBuffer) KeSetEvent(EventBuffer->Event, IO_NO_INCREMENT, FALSE);
        goto Quickie;
    }

    if (Ccb->NamedPipeState == FILE_PIPE_CLOSING_STATE)
    {
        IoStatus->Status = STATUS_PIPE_BROKEN;
        ReadOk = TRUE;
        if (EventBuffer) KeSetEvent(EventBuffer->Event, IO_NO_INCREMENT, FALSE);
        goto Quickie;
    }

    if (Ccb->CompletionMode[NamedPipeEnd] == FILE_PIPE_COMPLETE_OPERATION)
    {
        IoStatus->Status = STATUS_PIPE_EMPTY;
        ReadOk = TRUE;
        if (EventBuffer) KeSetEvent(EventBuffer->Event, IO_NO_INCREMENT, FALSE);
        goto Quickie;
    }

    if (!Irp)
    {
        ReadOk = FALSE;
        goto Quickie;
    }

    Status = NpAddDataQueueEntry(NamedPipeEnd,
                                 Ccb,
                                 ReadQueue,
                                 ReadEntries,
                                 Buffered,
                                 BufferSize,
                                 Irp,
                                 NULL,
                                 0);
    IoStatus->Status = Status;
    if (!NT_SUCCESS(Status))
    {
        ReadOk = FALSE;
    }
    else
    {
        ReadOk = TRUE;
        if (EventBuffer) KeSetEvent(EventBuffer->Event, IO_NO_INCREMENT, FALSE);
    }

Quickie:
    ExReleaseResourceLite(&Ccb->NonPagedCcb->Lock);
    return ReadOk;
}

NTSTATUS
NTAPI
NpFsdRead(IN PDEVICE_OBJECT DeviceObject,
          IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    IO_STATUS_BLOCK IoStatus;
    LIST_ENTRY List;
    PLIST_ENTRY NextEntry, ThisEntry;
    PIRP LocalIrp;
    PAGED_CODE();
    NpSlowReadCalls++;

    InitializeListHead(&List);
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    FsRtlEnterFileSystem();
    ExAcquireResourceSharedLite(&NpVcb->Lock, TRUE);

    NpCommonRead(IoStack->FileObject,
                 Irp->UserBuffer,
                 IoStack->Parameters.Read.Length,
                 &IoStatus,
                 Irp,
                 &List);

    ExReleaseResourceLite(&NpVcb->Lock);

    NextEntry = List.Flink;
    while (NextEntry != &List)
    {
        ThisEntry = NextEntry;
        NextEntry = NextEntry->Flink;

        LocalIrp = CONTAINING_RECORD(ThisEntry, IRP, Tail.Overlay.ListEntry);
        IoCompleteRequest(LocalIrp, IO_NAMED_PIPE_INCREMENT);
    }

    FsRtlExitFileSystem();

    if (IoStatus.Status != STATUS_PENDING)
    {
        Irp->IoStatus.Information = IoStatus.Information;
        Irp->IoStatus.Status = IoStatus.Status;
        IoCompleteRequest(Irp, IO_NAMED_PIPE_INCREMENT);
    }

    return IoStatus.Status;
}
