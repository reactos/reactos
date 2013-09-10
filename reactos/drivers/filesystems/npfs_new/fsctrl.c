#include "npfs.h"

IO_STATUS_BLOCK NpUserIoStatusBlock;

NTSTATUS
NTAPI
NpInternalTransceive(IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP Irp,
                     IN PLIST_ENTRY List)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NpInternalRead(IN PDEVICE_OBJECT DeviceObject,
               IN PIRP Irp,
               IN BOOLEAN Overflow,
               IN PLIST_ENTRY List)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NpInternalWrite(IN PDEVICE_OBJECT DeviceObject,
                IN PIRP Irp,
                IN PLIST_ENTRY List)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NpQueryClientProcess(IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP Irp)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NpSetClientProcess(IN PDEVICE_OBJECT DeviceObject,
                   IN PIRP Irp)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NpAssignEvent(IN PDEVICE_OBJECT DeviceObject,
              IN PIRP Irp)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NpQueryEvent(IN PDEVICE_OBJECT DeviceObject,
             IN PIRP Irp)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NpImpersonate(IN PDEVICE_OBJECT DeviceObject,
              IN PIRP Irp)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NpDisconnect(IN PDEVICE_OBJECT DeviceObject,
             IN PIRP Irp,
             IN PLIST_ENTRY List)
{
    BOOLEAN ServerSide;
    PNP_CCB Ccb;
    NTSTATUS Status;
    NODE_TYPE_CODE NodeTypeCode;
    PIO_STACK_LOCATION IoStack;
    PAGED_CODE();

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    NodeTypeCode = NpDecodeFileObject(IoStack->FileObject, NULL, &Ccb, &ServerSide);
    if (NodeTypeCode == NPFS_NTC_CCB)
    {
        if ( ServerSide == 1 )
        {
            ExAcquireResourceExclusiveLite(&Ccb->NonPagedCcb->Lock, TRUE);

            Status = NpSetDisconnectedPipeState(Ccb, List);

            NpUninitializeSecurity(Ccb);

            ExReleaseResourceLite(&Ccb->NonPagedCcb->Lock);
        }
        else
        {
            Status = STATUS_ILLEGAL_FUNCTION;
        }
    }
    else
    {
        Status = STATUS_PIPE_DISCONNECTED;
    }

    return Status;
}

NTSTATUS
NTAPI
NpListen(IN PDEVICE_OBJECT DeviceObject,
         IN PIRP Irp,
         IN PLIST_ENTRY List)
{
    BOOLEAN ServerSide;
    PNP_CCB Ccb;
    NTSTATUS Status;
    NODE_TYPE_CODE NodeTypeCode;
    PIO_STACK_LOCATION IoStack;
    PAGED_CODE();

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    NodeTypeCode = NpDecodeFileObject(IoStack->FileObject, NULL, &Ccb, &ServerSide);
    if (NodeTypeCode == NPFS_NTC_CCB)
    {
        if ( ServerSide == 1 )
        {
            ExAcquireResourceExclusiveLite(&Ccb->NonPagedCcb->Lock, TRUE);

            Status = NpSetListeningPipeState(Ccb, Irp, List);

            NpUninitializeSecurity(Ccb);

            ExReleaseResourceLite(&Ccb->NonPagedCcb->Lock);
        }
        else
        {
            Status = STATUS_ILLEGAL_FUNCTION;
        }
    }
    else
    {
        Status = STATUS_ILLEGAL_FUNCTION;
    }

    return Status;
}

NTSTATUS
NTAPI
NpPeek(IN PDEVICE_OBJECT DeviceObject,
       IN PIRP Irp, 
       IN PLIST_ENTRY List)
{
    PIO_STACK_LOCATION IoStack;
    NODE_TYPE_CODE Type;
    ULONG InputLength;
    BOOLEAN ServerSide;
    PNP_CCB Ccb;
    PFILE_PIPE_PEEK_BUFFER PeekBuffer;
    PNP_DATA_QUEUE DataQueue;
    ULONG BytesPeeked;
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS Status;
    PNP_DATA_QUEUE_ENTRY DataEntry;
    PAGED_CODE();

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    InputLength = IoStack->Parameters.FileSystemControl.OutputBufferLength;
    Type = NpDecodeFileObject(IoStack->FileObject, NULL, &Ccb, &ServerSide);
    if (!Type) return STATUS_PIPE_DISCONNECTED;

    if ((Type != NPFS_NTC_CCB) && (InputLength < sizeof(*PeekBuffer)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    PeekBuffer = (PFILE_PIPE_PEEK_BUFFER)Irp->AssociatedIrp.SystemBuffer;
    if ( ServerSide )
    {
        if ( ServerSide != 1 )
        {
            KeBugCheckEx(NPFS_FILE_SYSTEM, 0xD02E5, ServerSide, 0, 0);
        }
        DataQueue = &Ccb->InQueue;
    }
    else
    {
        DataQueue = &Ccb->OutQueue;
    }

    if ( Ccb->NamedPipeState != FILE_PIPE_CONNECTED_STATE )
    {
        if ( Ccb->NamedPipeState != FILE_PIPE_CLOSING_STATE )
        {
            return STATUS_INVALID_PIPE_STATE;
        }
        if ( DataQueue->QueueState != WriteEntries)
        {
            return STATUS_PIPE_BROKEN;
        }
    }

    PeekBuffer->NamedPipeState = 0;
    PeekBuffer->ReadDataAvailable = 0;
    PeekBuffer->NumberOfMessages = 0;
    PeekBuffer->MessageLength = 0;
    PeekBuffer->NamedPipeState = Ccb->NamedPipeState;
    BytesPeeked = sizeof(*PeekBuffer);

    if ( DataQueue->QueueState == WriteEntries)
    {
        DataEntry = CONTAINING_RECORD(DataQueue->Queue.Flink,
                                      NP_DATA_QUEUE_ENTRY,
                                      QueueEntry);
        ASSERT((DataEntry->DataEntryType == Buffered) || (DataEntry->DataEntryType == Unbuffered));

        PeekBuffer->ReadDataAvailable = DataQueue->BytesInQueue - DataQueue->ByteOffset;
        if ( Ccb->Fcb->NamedPipeType == FILE_PIPE_MESSAGE_TYPE)
        {
            PeekBuffer->NumberOfMessages = DataQueue->EntriesInQueue;
            PeekBuffer->MessageLength = DataEntry->DataSize - DataQueue->ByteOffset;
        }
        if ( InputLength == sizeof(*PeekBuffer) )
        {
            Status = PeekBuffer->ReadDataAvailable != 0 ? STATUS_BUFFER_OVERFLOW : STATUS_SUCCESS;
        }
        else
        {
            IoStatus = NpReadDataQueue(DataQueue,
                                       TRUE,
                                       FALSE,
                                       PeekBuffer->Data,
                                       InputLength - sizeof(*PeekBuffer),
                                       Ccb->Fcb->NamedPipeType == FILE_PIPE_MESSAGE_TYPE,
                                       Ccb,
                                       List);
            Status = IoStatus.Status;
            BytesPeeked = IoStatus.Information + sizeof(*PeekBuffer);
        }
    }
    else
    {
        Status = STATUS_SUCCESS;
    }

    Irp->IoStatus.Information = BytesPeeked;
    return Status;
}

NTSTATUS
NTAPI
NpCompleteTransceiveIrp(IN PDEVICE_OBJECT DeviceObject,
                        IN PIRP Irp,
                        IN PVOID Context)
{
    PAGED_CODE();

    if ( Irp->AssociatedIrp.SystemBuffer )
    {
        ExFreePool(Irp->AssociatedIrp.SystemBuffer);
    }

    IoFreeIrp(Irp);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
NTAPI
NpTransceive(IN PDEVICE_OBJECT DeviceObject,
             IN PIRP Irp,
             IN PLIST_ENTRY List)
{
    PIO_STACK_LOCATION IoStack;
    PVOID InBuffer, OutBuffer;
    ULONG InLength, OutLength, ReadMode, BytesWritten;
    NODE_TYPE_CODE NodeTypeCode;
    PNP_CCB Ccb;
    BOOLEAN ServerSide;
    PNP_NONPAGED_CCB NonPagedCcb;
    PNP_DATA_QUEUE ReadQueue, WriteQueue;
    PNP_EVENT_BUFFER EventBuffer;
    NTSTATUS Status;
    PIRP NewIrp;
    PAGED_CODE();

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    InLength = IoStack->Parameters.FileSystemControl.InputBufferLength;
    InBuffer = IoStack->Parameters.FileSystemControl.Type3InputBuffer;
    OutLength = IoStack->Parameters.FileSystemControl.OutputBufferLength;
    OutBuffer = Irp->UserBuffer;

    if ( Irp->RequestorMode  == UserMode)
    {
        _SEH2_TRY
        {
            ProbeForRead(InBuffer, InLength, sizeof(CHAR));
            ProbeForWrite(OutBuffer, OutLength, sizeof(CHAR));
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            ASSERT(FALSE);
        }
        _SEH2_END;
    }

    NodeTypeCode = NpDecodeFileObject(IoStack->FileObject, NULL, &Ccb, &ServerSide);
    if (NodeTypeCode != NPFS_NTC_CCB )
    {
        return STATUS_PIPE_DISCONNECTED;
    }

    NonPagedCcb = Ccb->NonPagedCcb;
    ExAcquireResourceExclusiveLite(&NonPagedCcb->Lock, TRUE);

    //ms_exc.registration.TryLevel = 1;
    if ( Ccb->NamedPipeState != FILE_PIPE_CONNECTED_STATE )
    {
        Status = STATUS_INVALID_PIPE_STATE;
        goto Quickie;
    }

    if ( ServerSide )
    {
        if ( ServerSide != 1 ) KeBugCheckEx(NPFS_FILE_SYSTEM, 0xD0538, ServerSide, 0, 0);
        ReadQueue = &Ccb->InQueue;
        WriteQueue = &Ccb->OutQueue;
        WriteQueue = &Ccb->OutQueue;
        EventBuffer = NonPagedCcb->EventBufferServer;
        ReadMode = Ccb->ServerReadMode;
    }
    else
    {
        ReadQueue = &Ccb->OutQueue;
        WriteQueue = &Ccb->InQueue;
        EventBuffer = NonPagedCcb->EventBufferClient;
        ReadMode = Ccb->ClientReadMode;
        WriteQueue = &Ccb->InQueue;
    }

    if (Ccb->Fcb->NamedPipeConfiguration != FILE_PIPE_FULL_DUPLEX ||
        ReadMode != FILE_PIPE_MESSAGE_MODE )
    {
        Status = STATUS_INVALID_PIPE_STATE;
        goto Quickie;
    }

    if ( ReadQueue->QueueState != Empty)
    {
        Status = STATUS_PIPE_BUSY;
        goto Quickie;
    }

    Status = NpWriteDataQueue(WriteQueue,
                              1,
                              InBuffer,
                              InLength,
                              Ccb->Fcb->NamedPipeType,
                              &BytesWritten,
                              Ccb,
                              ServerSide,
                              Irp->Tail.Overlay.Thread,
                              List);
    if ( Status == STATUS_MORE_PROCESSING_REQUIRED )
    {
        ASSERT(WriteQueue->QueueState != ReadEntries);
        NewIrp = IoAllocateIrp(DeviceObject->StackSize, TRUE);
        if ( !NewIrp )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Quickie;
        }

        IoSetCompletionRoutine(Irp, NpCompleteTransceiveIrp, NULL, TRUE, TRUE, TRUE);

        if ( BytesWritten )
        {
            NewIrp->AssociatedIrp.SystemBuffer = ExAllocatePoolWithQuotaTag(PagedPool, BytesWritten, 'wFpN');
            if ( !NewIrp->AssociatedIrp.SystemBuffer )
            {
                IoFreeIrp(NewIrp);
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Quickie;
            }

            RtlCopyMemory(NewIrp->AssociatedIrp.SystemBuffer,
                          (PVOID)((ULONG_PTR)InBuffer + InLength - BytesWritten),
                          BytesWritten);
            //ms_exc.registration.TryLevel = 1;
        }
        else
        {
            NewIrp->AssociatedIrp.SystemBuffer = NULL;
        }

        IoStack = IoGetNextIrpStackLocation(NewIrp);
        IoSetNextIrpStackLocation(NewIrp);

        NewIrp->Tail.Overlay.Thread = Irp->Tail.Overlay.Thread;
        NewIrp->IoStatus.Information = BytesWritten;

        IoStack->Parameters.Read.Length = BytesWritten;
        IoStack->MajorFunction = IRP_MJ_WRITE;

        if ( BytesWritten > 0 ) NewIrp->Flags = IRP_DEALLOCATE_BUFFER | IRP_BUFFERED_IO;
        NewIrp->UserIosb = &NpUserIoStatusBlock;

        Status = NpAddDataQueueEntry(ServerSide,
                                     Ccb,
                                     WriteQueue,
                                     WriteEntries,
                                     1,
                                     BytesWritten,
                                     NewIrp,
                                     NULL,
                                     0);
        if ( Status != STATUS_PENDING )
        {
            NewIrp->IoStatus.Status = Status;
            InsertTailList(List, &NewIrp->Tail.Overlay.ListEntry);
        }
    }

    if (!NT_SUCCESS(Status)) goto Quickie;
 
    if ( EventBuffer ) KeSetEvent(EventBuffer->Event, 0, 0);
    ASSERT(ReadQueue->QueueState == Empty);
    Status = NpAddDataQueueEntry(ServerSide,
                                 Ccb,
                                 ReadQueue,
                                 ReadEntries,
                                 0,
                                 OutLength,
                                 Irp,
                                 NULL,
                                 0);
    if (NT_SUCCESS(Status))
    {
        if ( EventBuffer ) KeSetEvent(EventBuffer->Event, 0, 0);
    }

Quickie:
    //ms_exc.registration.TryLevel = -1;
    ExReleaseResourceLite(&Ccb->NonPagedCcb->Lock);
    return Status;
}

NTSTATUS
NTAPI
NpWaitForNamedPipe(IN PDEVICE_OBJECT DeviceObject,
                   IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    ULONG InLength, NameLength;
    UNICODE_STRING SourceString, Prefix;
    BOOLEAN ServerSide;
    PNP_CCB Ccb;
    PFILE_PIPE_WAIT_FOR_BUFFER Buffer;
    NTSTATUS Status;
    NODE_TYPE_CODE NodeTypeCode;
    PLIST_ENTRY NextEntry;
    PNP_FCB Fcb;
    PAGED_CODE();

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    InLength = IoStack->Parameters.DeviceIoControl.InputBufferLength;

    SourceString.Buffer = NULL;

    //ms_exc.registration.TryLevel = 0;

    if (NpDecodeFileObject(IoStack->FileObject, NULL, &Ccb, &ServerSide) != NPFS_NTC_ROOT_DCB )
    {
        Status = STATUS_ILLEGAL_FUNCTION;
        goto Quickie;
    }

    Buffer = (PFILE_PIPE_WAIT_FOR_BUFFER)Irp->AssociatedIrp.SystemBuffer;
    if (InLength < sizeof(*Buffer))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Quickie;
    }

    NameLength = Buffer->NameLength;
    if ((NameLength > 0xFFFD) || ((NameLength + sizeof(*Buffer)) > InLength ))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Quickie;
    }

    SourceString.Length = (USHORT)NameLength + sizeof(OBJ_NAME_PATH_SEPARATOR);
    SourceString.Buffer = ExAllocatePoolWithTag(PagedPool, SourceString.Length, 'WFpN');
    if (!SourceString.Buffer )
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Quickie;
    }

    SourceString.Buffer[0] = OBJ_NAME_PATH_SEPARATOR;
    RtlCopyMemory(&SourceString.Buffer[1], Buffer->Name, Buffer->NameLength);

    Status = STATUS_SUCCESS;
    //Status = NpTranslateAlias(&SourceString);
    if (!NT_SUCCESS(Status)) goto Quickie;

    Fcb = NpFindPrefix(&SourceString, TRUE, &Prefix);
    Fcb = (PNP_FCB)((ULONG_PTR)Fcb & ~1);

    NodeTypeCode = Fcb ? Fcb->NodeType : 0;
    if ( NodeTypeCode != NPFS_NTC_FCB )
    {
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
        goto Quickie;
    }

    for (NextEntry = Fcb->CcbList.Flink;
         NextEntry != &Fcb->CcbList;
         NextEntry = NextEntry->Flink)
    {
        Ccb = CONTAINING_RECORD(NextEntry, NP_CCB, CcbEntry);
        if ( Ccb->NamedPipeState == FILE_PIPE_LISTENING_STATE ) break;
    }

    if ( NextEntry == &Fcb->CcbList )
    {
        Status = STATUS_SUCCESS;
    }
    else
    {
        Status = NpAddWaiter(&NpVcb->WaitQueue,
                             Fcb->Timeout,
                             Irp,
                             &SourceString);
    }

Quickie:
    //ms_exc.registration.TryLevel = -1;
    if ( SourceString.Buffer ) ExFreePool(SourceString.Buffer);
    return Status;
}

NTSTATUS
NTAPI
NpCommonFileSystemControl(IN PDEVICE_OBJECT DeviceObject,
                          IN PIRP Irp)
{
    ULONG Fsctl;
    BOOLEAN Overflow = FALSE;
    LIST_ENTRY List;
    PLIST_ENTRY NextEntry, ThisEntry;
    NTSTATUS Status;
    PAGED_CODE();

    InitializeListHead(&List);
    Fsctl = IoGetCurrentIrpStackLocation(Irp)->Parameters.FileSystemControl.FsControlCode;

    switch (Fsctl)
    {
        case FSCTL_PIPE_PEEK:
            ExAcquireResourceExclusiveLite(&NpVcb->Lock, 1u);
            Status = NpPeek(DeviceObject, Irp, &List);
            break;

        case FSCTL_PIPE_INTERNAL_WRITE:
            ExAcquireResourceSharedLite(&NpVcb->Lock, 1u);
            Status = NpInternalWrite(DeviceObject, Irp, &List);
            break;

        case FSCTL_PIPE_TRANSCEIVE:
            ExAcquireResourceSharedLite(&NpVcb->Lock, 1u);
            Status = NpTransceive(DeviceObject, Irp, &List);
            break;

        case FSCTL_PIPE_INTERNAL_TRANSCEIVE:
            ExAcquireResourceSharedLite(&NpVcb->Lock, 1u);
            Status = NpInternalTransceive(DeviceObject, Irp, &List);
            break;
 
        case FSCTL_PIPE_INTERNAL_READ_OVFLOW:
            Overflow = TRUE;
            // on purpose

        case FSCTL_PIPE_INTERNAL_READ:
            ExAcquireResourceSharedLite(&NpVcb->Lock, 1u);
            Status = NpInternalRead(DeviceObject, Irp, Overflow, &List);
            break;

        case FSCTL_PIPE_QUERY_CLIENT_PROCESS:

            ExAcquireResourceSharedLite(&NpVcb->Lock, 1u);
            Status = NpQueryClientProcess(DeviceObject, Irp);
            break;

        case FSCTL_PIPE_ASSIGN_EVENT:

            ExAcquireResourceExclusiveLite(&NpVcb->Lock, 1u);
            Status = NpAssignEvent(DeviceObject, Irp);
            break;

        case FSCTL_PIPE_DISCONNECT:

            ExAcquireResourceExclusiveLite(&NpVcb->Lock, 1u);
            Status = NpDisconnect(DeviceObject, Irp, &List);
            break;

        case FSCTL_PIPE_LISTEN:

            ExAcquireResourceSharedLite(&NpVcb->Lock, 1u);
            Status = NpListen(DeviceObject, Irp, &List);
            break;

        case FSCTL_PIPE_QUERY_EVENT:

            ExAcquireResourceExclusiveLite(&NpVcb->Lock, 1u);
            Status = NpQueryEvent(DeviceObject, Irp);
            break;

        case FSCTL_PIPE_WAIT:

            ExAcquireResourceExclusiveLite(&NpVcb->Lock, 1u);
            Status = NpWaitForNamedPipe(DeviceObject, Irp);
            break;

        case FSCTL_PIPE_IMPERSONATE:

            ExAcquireResourceExclusiveLite(&NpVcb->Lock, 1u);
            Status = NpImpersonate(DeviceObject, Irp);
            break;

        case FSCTL_PIPE_SET_CLIENT_PROCESS:
            ExAcquireResourceExclusiveLite(&NpVcb->Lock, 1u);
            Status = NpSetClientProcess(DeviceObject, Irp);
            break;

        default:
            return STATUS_NOT_SUPPORTED;
    }

    ExReleaseResourceLite(&NpVcb->Lock);

    NextEntry = List.Flink;
    while (NextEntry != &List)
    {
        ThisEntry = NextEntry;
        NextEntry = NextEntry->Flink;

        Irp = CONTAINING_RECORD(ThisEntry, IRP, Tail.Overlay.ListEntry);
        IoCompleteRequest(Irp, IO_DISK_INCREMENT);
    }

    return Status;
}

NTSTATUS
NTAPI
NpFsdFileSystemControl(IN PDEVICE_OBJECT DeviceObject,
                       IN PIRP Irp)
{
    NTSTATUS Status;
    PAGED_CODE();

    FsRtlEnterFileSystem();

    Status = NpCommonFileSystemControl(DeviceObject, Irp);

    FsRtlExitFileSystem();

    if ( Status != STATUS_PENDING )
    {
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NAMED_PIPE_INCREMENT);
    }

    return Status;
}


