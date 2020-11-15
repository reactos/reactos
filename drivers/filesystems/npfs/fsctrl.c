/*
 * PROJECT:     ReactOS Named Pipe FileSystem
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * FILE:        drivers/filesystems/npfs/fsctrl.c
 * PURPOSE:     Named Pipe FileSystem I/O Controls
 * PROGRAMMERS: ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "npfs.h"

// File ID number for NPFS bugchecking support
#define NPFS_BUGCHECK_FILE_ID   (NPFS_BUGCHECK_FSCTRL)

/* GLOBALS ********************************************************************/

IO_STATUS_BLOCK NpUserIoStatusBlock;

/* FUNCTIONS ******************************************************************/

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
    PIO_STACK_LOCATION IoStackLocation;
    NODE_TYPE_CODE NodeTypeCode;
    PNP_CCB Ccb;
    PNP_CLIENT_PROCESS ClientSession, QueryBuffer;
    ULONG Length;
    PAGED_CODE();

    /* Get the current stack location */
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

    /* Decode the file object and check the node type */
    NodeTypeCode = NpDecodeFileObject(IoStackLocation->FileObject, 0, &Ccb, 0);
    if (NodeTypeCode != NPFS_NTC_CCB)
    {
        return STATUS_PIPE_DISCONNECTED;
    }

    /* Get the length of the query buffer */
    Length = IoStackLocation->Parameters.QueryFile.Length;
    if (Length < 8)
    {
        return STATUS_INVALID_PARAMETER;
    }

    QueryBuffer = Irp->AssociatedIrp.SystemBuffer;

    /* Lock the Ccb */
    ExAcquireResourceExclusiveLite(&Ccb->NonPagedCcb->Lock, TRUE);

    /* Get the CCBs client session and check if it's set */
    ClientSession = Ccb->ClientSession;
    if (ClientSession != NULL)
    {
        /* Copy first 2 fields */
        QueryBuffer->Unknown = ClientSession->Unknown;
        QueryBuffer->Process = ClientSession->Process;
    }
    else
    {
        /* Copy the process from the CCB */
        QueryBuffer->Unknown = NULL;
        QueryBuffer->Process = Ccb->Process;
    }

    /* Does the caller provide a large enough buffer for the full data? */
    if (Length >= sizeof(NP_CLIENT_PROCESS))
    {
        Irp->IoStatus.Information = sizeof(NP_CLIENT_PROCESS);

        /* Do we have a ClientSession structure? */
        if (ClientSession != NULL)
        {
            /* Copy length and the data */
            QueryBuffer->DataLength = ClientSession->DataLength;
            RtlCopyMemory(QueryBuffer->Buffer,
                          ClientSession->Buffer,
                          ClientSession->DataLength);

            /* NULL terminate the buffer */
            NT_ASSERT(QueryBuffer->DataLength <= 30);
            QueryBuffer->Buffer[QueryBuffer->DataLength / sizeof(WCHAR)] = 0;
        }
        else
        {
            /* No data */
            QueryBuffer->DataLength = 0;
            QueryBuffer->Buffer[0] = 0;
        }
    }
    else
    {
        Irp->IoStatus.Information = FIELD_OFFSET(NP_CLIENT_PROCESS, DataLength);
    }

    /* Unlock the Ccb */
    ExReleaseResourceLite(&Ccb->NonPagedCcb->Lock);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
NpSetClientProcess(IN PDEVICE_OBJECT DeviceObject,
                   IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStackLocation;
    NODE_TYPE_CODE NodeTypeCode;
    PNP_CCB Ccb;
    ULONG Length;
    PNP_CLIENT_PROCESS InputBuffer, ClientSession, OldClientSession;
    PAGED_CODE();

    /* Get the current stack location */
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

    /* Only kernel calls are allowed! */
    if (IoStackLocation->MinorFunction != IRP_MN_KERNEL_CALL)
    {
        return STATUS_ACCESS_DENIED;
    }

    /* Decode the file object and check the node type */
    NodeTypeCode = NpDecodeFileObject(IoStackLocation->FileObject, 0, &Ccb, 0);
    if (NodeTypeCode != NPFS_NTC_CCB)
    {
        return STATUS_PIPE_DISCONNECTED;
    }

    /* Get the length of the query buffer and check if it's valid */
    Length = IoStackLocation->Parameters.QueryFile.Length;
    if (Length != sizeof(NP_CLIENT_PROCESS))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Get the buffer and check if the data Length is valid */
    InputBuffer = Irp->AssociatedIrp.SystemBuffer;
    if (InputBuffer->DataLength > 30)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Allocate a new structure */
    ClientSession = ExAllocatePoolWithQuotaTag(PagedPool,
                                               sizeof(NP_CLIENT_PROCESS),
                                               'iFpN');

    /* Copy the full input buffer */
    RtlCopyMemory(ClientSession, InputBuffer, sizeof(NP_CLIENT_PROCESS));

    /* Lock the Ccb */
    ExAcquireResourceExclusiveLite(&Ccb->NonPagedCcb->Lock, TRUE);

    /* Get the old ClientSession and set the new */
    OldClientSession = Ccb->ClientSession;
    Ccb->ClientSession = ClientSession;

    /* Copy the process to the CCB */
    Ccb->Process = ClientSession->Process;

    /* Unlock the Ccb */
    ExReleaseResourceLite(&Ccb->NonPagedCcb->Lock);

    /* Check if there was already a ClientSession */
    if (OldClientSession != NULL)
    {
        /* Free it */
        ExFreePoolWithTag(OldClientSession, 'iFpN');
    }

    return STATUS_SUCCESS;
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
    ULONG NamedPipeEnd;
    PNP_CCB Ccb;
    NTSTATUS Status;
    NODE_TYPE_CODE NodeTypeCode;
    PIO_STACK_LOCATION IoStack;
    PAGED_CODE();

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    NodeTypeCode = NpDecodeFileObject(IoStack->FileObject, NULL, &Ccb, &NamedPipeEnd);
    if (NodeTypeCode == NPFS_NTC_CCB && NamedPipeEnd == FILE_PIPE_SERVER_END)
    {
        Status = NpImpersonateClientContext(Ccb);
    }
    else
    {
        Status = STATUS_ILLEGAL_FUNCTION;
    }

    return Status;
}

NTSTATUS
NTAPI
NpDisconnect(IN PDEVICE_OBJECT DeviceObject,
             IN PIRP Irp,
             IN PLIST_ENTRY List)
{
    ULONG NamedPipeEnd;
    PNP_CCB Ccb;
    NTSTATUS Status;
    NODE_TYPE_CODE NodeTypeCode;
    PIO_STACK_LOCATION IoStack;
    PAGED_CODE();

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    NodeTypeCode = NpDecodeFileObject(IoStack->FileObject, NULL, &Ccb, &NamedPipeEnd);
    if (NodeTypeCode == NPFS_NTC_CCB)
    {
        if (NamedPipeEnd == FILE_PIPE_SERVER_END)
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
    ULONG NamedPipeEnd;
    PNP_CCB Ccb;
    NTSTATUS Status;
    NODE_TYPE_CODE NodeTypeCode;
    PIO_STACK_LOCATION IoStack;
    PAGED_CODE();

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    NodeTypeCode = NpDecodeFileObject(IoStack->FileObject, NULL, &Ccb, &NamedPipeEnd);
    if (NodeTypeCode == NPFS_NTC_CCB)
    {
        if (NamedPipeEnd == FILE_PIPE_SERVER_END)
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
    ULONG OutputLength;
    ULONG NamedPipeEnd;
    PNP_CCB Ccb;
    PFILE_PIPE_PEEK_BUFFER PeekBuffer;
    PNP_DATA_QUEUE DataQueue;
    ULONG_PTR BytesPeeked;
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS Status;
    PNP_DATA_QUEUE_ENTRY DataEntry;
    PAGED_CODE();

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    OutputLength = IoStack->Parameters.FileSystemControl.OutputBufferLength;
    Type = NpDecodeFileObject(IoStack->FileObject, NULL, &Ccb, &NamedPipeEnd);

    if (!Type)
    {
        return STATUS_PIPE_DISCONNECTED;
    }

    if ((Type != NPFS_NTC_CCB) &&
        (OutputLength < FIELD_OFFSET(FILE_PIPE_PEEK_BUFFER, Data)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    PeekBuffer = Irp->AssociatedIrp.SystemBuffer;
    if (NamedPipeEnd != FILE_PIPE_CLIENT_END)
    {
        if (NamedPipeEnd != FILE_PIPE_SERVER_END)
        {
            NpBugCheck(NamedPipeEnd, 0, 0);
        }

        DataQueue = &Ccb->DataQueue[FILE_PIPE_INBOUND];
    }
    else
    {
        DataQueue = &Ccb->DataQueue[FILE_PIPE_OUTBOUND];
    }

    if (Ccb->NamedPipeState != FILE_PIPE_CONNECTED_STATE)
    {
        if (Ccb->NamedPipeState != FILE_PIPE_CLOSING_STATE)
        {
            return STATUS_INVALID_PIPE_STATE;
        }

        if (DataQueue->QueueState != WriteEntries)
        {
            return STATUS_PIPE_BROKEN;
        }
    }

    PeekBuffer->NamedPipeState = 0;
    PeekBuffer->ReadDataAvailable = 0;
    PeekBuffer->NumberOfMessages = 0;
    PeekBuffer->MessageLength = 0;
    PeekBuffer->NamedPipeState = Ccb->NamedPipeState;
    BytesPeeked = FIELD_OFFSET(FILE_PIPE_PEEK_BUFFER, Data);

    if (DataQueue->QueueState == WriteEntries)
    {
        DataEntry = CONTAINING_RECORD(DataQueue->Queue.Flink,
                                      NP_DATA_QUEUE_ENTRY,
                                      QueueEntry);
        ASSERT((DataEntry->DataEntryType == Buffered) || (DataEntry->DataEntryType == Unbuffered));

        PeekBuffer->ReadDataAvailable = DataQueue->BytesInQueue - DataQueue->ByteOffset;
        if (Ccb->Fcb->NamedPipeType == FILE_PIPE_MESSAGE_TYPE)
        {
            PeekBuffer->NumberOfMessages = DataQueue->EntriesInQueue;
            PeekBuffer->MessageLength = DataEntry->DataSize - DataQueue->ByteOffset;
        }

        if (OutputLength == FIELD_OFFSET(FILE_PIPE_PEEK_BUFFER, Data))
        {
            Status = PeekBuffer->ReadDataAvailable ? STATUS_BUFFER_OVERFLOW : STATUS_SUCCESS;
        }
        else
        {
            IoStatus = NpReadDataQueue(DataQueue,
                                       TRUE,
                                       FALSE,
                                       PeekBuffer->Data,
                                       OutputLength - FIELD_OFFSET(FILE_PIPE_PEEK_BUFFER, Data),
                                       Ccb->Fcb->NamedPipeType == FILE_PIPE_MESSAGE_TYPE,
                                       Ccb,
                                       List);
            Status = IoStatus.Status;
            BytesPeeked += IoStatus.Information;
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

    if (Irp->AssociatedIrp.SystemBuffer)
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
    ULONG InLength, OutLength, BytesWritten;
    NODE_TYPE_CODE NodeTypeCode;
    PNP_CCB Ccb;
    ULONG NamedPipeEnd;
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

    if (Irp->RequestorMode == UserMode)
    {
        _SEH2_TRY
        {
            ProbeForRead(InBuffer, InLength, sizeof(CHAR));
            ProbeForWrite(OutBuffer, OutLength, sizeof(CHAR));
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    NodeTypeCode = NpDecodeFileObject(IoStack->FileObject, NULL, &Ccb, &NamedPipeEnd);
    if (NodeTypeCode != NPFS_NTC_CCB)
    {
        return STATUS_PIPE_DISCONNECTED;
    }

    NonPagedCcb = Ccb->NonPagedCcb;
    ExAcquireResourceExclusiveLite(&NonPagedCcb->Lock, TRUE);

    if (Ccb->NamedPipeState != FILE_PIPE_CONNECTED_STATE)
    {
        Status = STATUS_INVALID_PIPE_STATE;
        goto Quickie;
    }

    if (NamedPipeEnd != FILE_PIPE_CLIENT_END)
    {
        if (NamedPipeEnd != FILE_PIPE_SERVER_END)
        {
            NpBugCheck(NamedPipeEnd, 0, 0);
        }
        ReadQueue = &Ccb->DataQueue[FILE_PIPE_INBOUND];
        WriteQueue = &Ccb->DataQueue[FILE_PIPE_OUTBOUND];
    }
    else
    {
        ReadQueue = &Ccb->DataQueue[FILE_PIPE_OUTBOUND];
        WriteQueue = &Ccb->DataQueue[FILE_PIPE_INBOUND];
    }

    EventBuffer = NonPagedCcb->EventBuffer[NamedPipeEnd];

    if (Ccb->Fcb->NamedPipeConfiguration != FILE_PIPE_FULL_DUPLEX ||
        Ccb->ReadMode[NamedPipeEnd] != FILE_PIPE_MESSAGE_MODE)
    {
        Status = STATUS_INVALID_PIPE_STATE;
        goto Quickie;
    }

    if (ReadQueue->QueueState != Empty)
    {
        Status = STATUS_PIPE_BUSY;
        goto Quickie;
    }

    Status = NpWriteDataQueue(WriteQueue,
                              FILE_PIPE_MESSAGE_MODE,
                              InBuffer,
                              InLength,
                              Ccb->Fcb->NamedPipeType,
                              &BytesWritten,
                              Ccb,
                              NamedPipeEnd,
                              Irp->Tail.Overlay.Thread,
                              List);
    if (Status == STATUS_MORE_PROCESSING_REQUIRED)
    {
        ASSERT(WriteQueue->QueueState != ReadEntries);
        NewIrp = IoAllocateIrp(DeviceObject->StackSize, TRUE);
        if (!NewIrp)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Quickie;
        }

        IoSetCompletionRoutine(NewIrp, NpCompleteTransceiveIrp, NULL, TRUE, TRUE, TRUE);

        if (BytesWritten)
        {
            NewIrp->AssociatedIrp.SystemBuffer = ExAllocatePoolWithQuotaTag(PagedPool | POOL_QUOTA_FAIL_INSTEAD_OF_RAISE,
                                                                            BytesWritten,
                                                                            NPFS_WRITE_BLOCK_TAG);
            if (!NewIrp->AssociatedIrp.SystemBuffer)
            {
                IoFreeIrp(NewIrp);
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Quickie;
            }

            _SEH2_TRY
            {
                RtlCopyMemory(NewIrp->AssociatedIrp.SystemBuffer,
                              (PVOID)((ULONG_PTR)InBuffer + InLength - BytesWritten),
                              BytesWritten);
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
                _SEH2_YIELD(goto Quickie);
            }
            _SEH2_END;
        }
        else
        {
            NewIrp->AssociatedIrp.SystemBuffer = NULL;
        }

        IoStack = IoGetNextIrpStackLocation(NewIrp);
        IoSetNextIrpStackLocation(NewIrp);

        NewIrp->Tail.Overlay.Thread = Irp->Tail.Overlay.Thread;
        NewIrp->IoStatus.Information = BytesWritten;

        IoStack->Parameters.Write.Length = BytesWritten;
        IoStack->MajorFunction = IRP_MJ_WRITE;

        if (BytesWritten > 0) NewIrp->Flags = IRP_DEALLOCATE_BUFFER | IRP_BUFFERED_IO;
        NewIrp->UserIosb = &NpUserIoStatusBlock;

        Status = NpAddDataQueueEntry(NamedPipeEnd,
                                     Ccb,
                                     WriteQueue,
                                     WriteEntries,
                                     Unbuffered,
                                     BytesWritten,
                                     NewIrp,
                                     NULL,
                                     0);
        if (Status != STATUS_PENDING)
        {
            NewIrp->IoStatus.Status = Status;
            InsertTailList(List, &NewIrp->Tail.Overlay.ListEntry);
        }
    }

    if (!NT_SUCCESS(Status)) goto Quickie;

    if (EventBuffer) KeSetEvent(EventBuffer->Event, IO_NO_INCREMENT, FALSE);
    ASSERT(ReadQueue->QueueState == Empty);
    Status = NpAddDataQueueEntry(NamedPipeEnd,
                                 Ccb,
                                 ReadQueue,
                                 ReadEntries,
                                 Buffered,
                                 OutLength,
                                 Irp,
                                 NULL,
                                 0);
    if (NT_SUCCESS(Status))
    {
        if (EventBuffer) KeSetEvent(EventBuffer->Event, IO_NO_INCREMENT, FALSE);
    }

Quickie:
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
    ULONG NamedPipeEnd;
    PNP_CCB Ccb;
    PFILE_PIPE_WAIT_FOR_BUFFER Buffer;
    NTSTATUS Status;
    NODE_TYPE_CODE NodeTypeCode;
    PLIST_ENTRY NextEntry;
    PNP_FCB Fcb;
    PWCHAR OriginalBuffer;
    PAGED_CODE();

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    InLength = IoStack->Parameters.FileSystemControl.InputBufferLength;

    SourceString.Buffer = NULL;

    if (NpDecodeFileObject(IoStack->FileObject,
                           NULL,
                           &Ccb,
                           &NamedPipeEnd) != NPFS_NTC_ROOT_DCB)
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
    if ((NameLength > (0xFFFF - sizeof(UNICODE_NULL))) ||
        ((NameLength + FIELD_OFFSET(FILE_PIPE_WAIT_FOR_BUFFER, Name)) > InLength))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Quickie;
    }

    SourceString.Length = (USHORT)NameLength + sizeof(OBJ_NAME_PATH_SEPARATOR);
    SourceString.Buffer = ExAllocatePoolWithTag(PagedPool,
                                                SourceString.Length,
                                                NPFS_WRITE_BLOCK_TAG);
    if (!SourceString.Buffer)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Quickie;
    }

    SourceString.Buffer[0] = OBJ_NAME_PATH_SEPARATOR;
    RtlCopyMemory(&SourceString.Buffer[1], Buffer->Name, Buffer->NameLength);

    Status = STATUS_SUCCESS;
    OriginalBuffer = SourceString.Buffer;
    //Status = NpTranslateAlias(&SourceString);
    if (!NT_SUCCESS(Status)) goto Quickie;

    Fcb = NpFindPrefix(&SourceString, 1, &Prefix);
    Fcb = (PNP_FCB)((ULONG_PTR)Fcb & ~1);

    NodeTypeCode = Fcb ? Fcb->NodeType : 0;
    if (NodeTypeCode != NPFS_NTC_FCB)
    {
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
        goto Quickie;
    }

    for (NextEntry = Fcb->CcbList.Flink;
         NextEntry != &Fcb->CcbList;
         NextEntry = NextEntry->Flink)
    {
        Ccb = CONTAINING_RECORD(NextEntry, NP_CCB, CcbEntry);
        if (Ccb->NamedPipeState == FILE_PIPE_LISTENING_STATE) break;
    }

    if (NextEntry != &Fcb->CcbList)
    {
        Status = STATUS_SUCCESS;
    }
    else
    {
        Status = NpAddWaiter(&NpVcb->WaitQueue,
                             Fcb->Timeout,
                             Irp,
                             OriginalBuffer == SourceString.Buffer ?
                             NULL : &SourceString);
    }

Quickie:
    if (SourceString.Buffer) ExFreePool(SourceString.Buffer);
    return Status;
}

NTSTATUS
NTAPI
NpCommonFileSystemControl(IN PDEVICE_OBJECT DeviceObject,
                          IN PIRP Irp)
{
    ULONG Fsctl;
    BOOLEAN Overflow = FALSE;
    LIST_ENTRY DeferredList;
    NTSTATUS Status;
    PAGED_CODE();

    InitializeListHead(&DeferredList);
    Fsctl = IoGetCurrentIrpStackLocation(Irp)->Parameters.FileSystemControl.FsControlCode;

    switch (Fsctl)
    {
        case FSCTL_PIPE_PEEK:
            NpAcquireExclusiveVcb();
            Status = NpPeek(DeviceObject, Irp, &DeferredList);
            break;

        case FSCTL_PIPE_INTERNAL_WRITE:
            NpAcquireSharedVcb();
            Status = NpInternalWrite(DeviceObject, Irp, &DeferredList);
            break;

        case FSCTL_PIPE_TRANSCEIVE:
            NpAcquireSharedVcb();
            Status = NpTransceive(DeviceObject, Irp, &DeferredList);
            break;

        case FSCTL_PIPE_INTERNAL_TRANSCEIVE:
            NpAcquireSharedVcb();
            Status = NpInternalTransceive(DeviceObject, Irp, &DeferredList);
            break;

        case FSCTL_PIPE_INTERNAL_READ_OVFLOW:
            Overflow = TRUE;
            // on purpose

        case FSCTL_PIPE_INTERNAL_READ:
            NpAcquireSharedVcb();
            Status = NpInternalRead(DeviceObject, Irp, Overflow, &DeferredList);
            break;

        case FSCTL_PIPE_QUERY_CLIENT_PROCESS:

            NpAcquireSharedVcb();
            Status = NpQueryClientProcess(DeviceObject, Irp);
            break;

        case FSCTL_PIPE_ASSIGN_EVENT:

            NpAcquireExclusiveVcb();
            Status = NpAssignEvent(DeviceObject, Irp);
            break;

        case FSCTL_PIPE_DISCONNECT:

            NpAcquireExclusiveVcb();
            Status = NpDisconnect(DeviceObject, Irp, &DeferredList);
            break;

        case FSCTL_PIPE_LISTEN:

            NpAcquireSharedVcb();
            Status = NpListen(DeviceObject, Irp, &DeferredList);
            break;

        case FSCTL_PIPE_QUERY_EVENT:

            NpAcquireExclusiveVcb();
            Status = NpQueryEvent(DeviceObject, Irp);
            break;

        case FSCTL_PIPE_WAIT:

            NpAcquireExclusiveVcb();
            Status = NpWaitForNamedPipe(DeviceObject, Irp);
            break;

        case FSCTL_PIPE_IMPERSONATE:

            NpAcquireExclusiveVcb();
            Status = NpImpersonate(DeviceObject, Irp);
            break;

        case FSCTL_PIPE_SET_CLIENT_PROCESS:
            NpAcquireExclusiveVcb();
            Status = NpSetClientProcess(DeviceObject, Irp);
            break;

        default:
            return STATUS_NOT_SUPPORTED;
    }

    NpReleaseVcb();
    NpCompleteDeferredIrps(&DeferredList);

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

    if (Status != STATUS_PENDING)
    {
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NAMED_PIPE_INCREMENT);
    }

    return Status;
}

/* EOF */
