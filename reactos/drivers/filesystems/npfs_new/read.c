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
    PNP_DATA_QUEUE Queue;
    PNP_EVENT_BUFFER EventBuffer;
    NTSTATUS Status;
    BOOLEAN ServerSide;
    PNP_CCB Ccb;
    PNP_NONPAGED_CCB NonPagedCcb;
    BOOLEAN ReadOk;
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

    //ms_exc.registration.TryLevel = 0;

    if ( Ccb->NamedPipeState == FILE_PIPE_DISCONNECTED_STATE || Ccb->NamedPipeState == FILE_PIPE_LISTENING_STATE )
    {
        IoStatus->Status = Ccb->NamedPipeState != FILE_PIPE_DISCONNECTED_STATE ? STATUS_PIPE_LISTENING : STATUS_PIPE_DISCONNECTED;
        ReadOk = TRUE;
        goto Quickie;
    }

    ASSERT((Ccb->NamedPipeState == FILE_PIPE_CONNECTED_STATE) || (Ccb->NamedPipeState == FILE_PIPE_CLOSING_STATE));

    if ((ServerSide == 1 && Ccb->Fcb->NamedPipeConfiguration == FILE_PIPE_OUTBOUND) ||
        (ServerSide == 0 && Ccb->Fcb->NamedPipeConfiguration == FILE_PIPE_INBOUND ))
    {
        IoStatus->Status = STATUS_INVALID_PARAMETER;
        ReadOk = TRUE;
        goto Quickie;
    }

    if ( ServerSide == 1 )
    {
        Queue = &Ccb->InQueue;
        EventBuffer = NonPagedCcb->EventBufferClient;
    }
    else
    {
        Queue = &Ccb->OutQueue;
        EventBuffer = NonPagedCcb->EventBufferServer;
    }

    if ( Queue->QueueState == WriteEntries )
    {
        *IoStatus = NpReadDataQueue(Queue,
                                    FALSE,
                                    FALSE,
                                    Buffer,
                                    BufferSize,
                                    ServerSide ? Ccb->ServerReadMode : Ccb->ClientReadMode,
                                    Ccb,
                                    List);
        if (!NT_SUCCESS(IoStatus->Status))
        {
            ReadOk = TRUE;
            goto Quickie;
        }

        ReadOk = TRUE;
        if ( EventBuffer ) KeSetEvent(EventBuffer->Event, IO_NO_INCREMENT, FALSE);
        goto Quickie;
    }

    if ( Ccb->NamedPipeState == FILE_PIPE_CLOSING_STATE )
    {
        IoStatus->Status = STATUS_PIPE_BROKEN;
        ReadOk = TRUE;
        if ( EventBuffer ) KeSetEvent(EventBuffer->Event, IO_NO_INCREMENT, FALSE);
        goto Quickie;
    }

    if ((ServerSide ? Ccb->ServerCompletionMode : Ccb->ServerCompletionMode) == FILE_PIPE_COMPLETE_OPERATION)
    {
        IoStatus->Status = STATUS_PIPE_EMPTY;
        ReadOk = TRUE;
        if ( EventBuffer ) KeSetEvent(EventBuffer->Event, IO_NO_INCREMENT, FALSE);
        goto Quickie;
    }

    if ( !Irp )
    {
        ReadOk = FALSE;
        goto Quickie;

    }
    Status = NpAddDataQueueEntry(ServerSide,
                                    Ccb,
                                    Queue,
                                    0,
                                    0,
                                    BufferSize,
                                    Irp,
                                    0,
                                    0);
    IoStatus->Status = Status;
    if (!NT_SUCCESS(Status))
    {
        ReadOk = FALSE;
    }
    else
    {
        ReadOk = TRUE;
        if ( EventBuffer ) KeSetEvent(EventBuffer->Event, IO_NO_INCREMENT, FALSE);
    }

Quickie:
    //ms_exc.registration.TryLevel = -1;
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
