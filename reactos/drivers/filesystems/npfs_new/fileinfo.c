#include "npfs.h"

NTSTATUS
NTAPI
NpSetBasicInfo(IN PNP_CCB Ccb,
               IN PFILE_BASIC_INFORMATION Buffer)
{
    PAGED_CODE();
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
NpSetPipeInfo(IN PNP_FCB Fcb,
              IN PNP_CCB Ccb, 
              IN PFILE_PIPE_INFORMATION Buffer,
              IN ULONG ServerSide, 
              IN PLIST_ENTRY List)
{
    NTSTATUS Status;
    PNP_DATA_QUEUE ReadQueue, WriteQueue; 
    ULONG CompletionMode;
    PAGED_CODE();

    if ( Buffer->ReadMode == FILE_PIPE_MESSAGE_MODE && Fcb->NamedPipeType == FILE_PIPE_BYTE_STREAM_TYPE )
    {
        return STATUS_INVALID_PARAMETER;
    }

    if ( ServerSide )
    {
        if ( ServerSide != 1 ) KeBugCheckEx(NPFS_FILE_SYSTEM, 0xA04EFu, ServerSide, 0, 0);
        ReadQueue = &Ccb->InQueue;
        WriteQueue = &Ccb->OutQueue;
        CompletionMode = Ccb->ServerCompletionMode;
    }
    else
    {
        ReadQueue = &Ccb->OutQueue;
        WriteQueue = &Ccb->InQueue;
        CompletionMode = Ccb->ClientCompletionMode;
    }

    if ( Buffer->CompletionMode != FILE_PIPE_COMPLETE_OPERATION ||
        CompletionMode == FILE_PIPE_COMPLETE_OPERATION || 
        (ReadQueue->QueueState == ReadEntries &&
        WriteQueue->QueueState != WriteEntries ))
    {
        if (ServerSide)
        {
            Ccb->ServerReadMode = Buffer->ReadMode & 0xFF;
            Ccb->ServerCompletionMode = Buffer->CompletionMode & 0xFF;
        }
        else
        {
            Ccb->ClientReadMode = Buffer->ReadMode & 0xFF;
            Ccb->ClientCompletionMode = Buffer->CompletionMode & 0xFF;
        }

        NpCheckForNotify(Fcb->ParentDcb, 0, List);
        Status = 0;
    }
    else
    {
        Status = STATUS_PIPE_BUSY;
    }

    return Status;
}

NTSTATUS
NTAPI
NpCommonSetInformation(IN PDEVICE_OBJECT DeviceObject,
                       IN PIRP Irp,
                       IN PLIST_ENTRY List)
{
    NODE_TYPE_CODE NodeTypeCode;
    PIO_STACK_LOCATION IoStack;
    ULONG InfoClass;
    PVOID Buffer;
    PNP_FCB Fcb;
    PNP_CCB Ccb;
    BOOLEAN ServerSide;
    PAGED_CODE();

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    NodeTypeCode = NpDecodeFileObject(IoStack->FileObject,
                                      (PVOID*)&Fcb,
                                      &Ccb,
                                      &ServerSide);
    if ( !NodeTypeCode ) return STATUS_PIPE_DISCONNECTED;
    if ( NodeTypeCode != NPFS_NTC_CCB ) return STATUS_INVALID_PARAMETER;

    InfoClass = IoStack->Parameters.QueryFile.FileInformationClass;
    Buffer = Irp->AssociatedIrp.SystemBuffer;

    if ( InfoClass == FileBasicInformation ) return NpSetBasicInfo(Ccb, Buffer);
    
    if ( InfoClass != FilePipeInformation ) return STATUS_INVALID_PARAMETER;

    return NpSetPipeInfo(Fcb, Ccb, Buffer, ServerSide, List);
}

NTSTATUS
NTAPI
NpFsdSetInformation(IN PDEVICE_OBJECT DeviceObject,
                    IN PIRP Irp)
{
    NTSTATUS Status;
    LIST_ENTRY List;
    PLIST_ENTRY NextEntry, ThisEntry;
    PAGED_CODE();

    InitializeListHead(&List);

    FsRtlEnterFileSystem();
    ExAcquireResourceSharedLite(&NpVcb->Lock, TRUE);

    Status = NpCommonSetInformation(DeviceObject, Irp, &List);

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

    if ( Status != STATUS_PENDING )
    {
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NAMED_PIPE_INCREMENT);
    }

    return Status;
}

NTSTATUS
NTAPI
NpCommonQueryInformation(IN PDEVICE_OBJECT DeviceObject,
                         IN PIRP Irp)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NpFsdQueryInformation(IN PDEVICE_OBJECT DeviceObject,
                      IN PIRP Irp)
{
    NTSTATUS Status;
    PAGED_CODE();

    FsRtlEnterFileSystem();
    ExAcquireResourceSharedLite(&NpVcb->Lock, TRUE);

    Status = NpCommonQueryInformation(DeviceObject, Irp);

    ExReleaseResourceLite(&NpVcb->Lock);
    FsRtlExitFileSystem();

    if ( Status != STATUS_PENDING )
    {
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NAMED_PIPE_INCREMENT);
    }

    return Status;
}

