/*
 * PROJECT:     ReactOS Named Pipe FileSystem
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * FILE:        drivers/filesystems/npfs/fileinfo.c
 * PURPOSE:     Pipes Information
 * PROGRAMMERS: ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "npfs.h"

// File ID number for NPFS bugchecking support
#define NPFS_BUGCHECK_FILE_ID   (NPFS_BUGCHECK_FILEINFO)

/* FUNCTIONS ******************************************************************/

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
              IN ULONG NamedPipeEnd,
              IN PLIST_ENTRY List)
{
    NTSTATUS Status;
    PNP_DATA_QUEUE ReadQueue, WriteQueue;
    PAGED_CODE();

    if (Buffer->ReadMode == FILE_PIPE_MESSAGE_MODE && Fcb->NamedPipeType == FILE_PIPE_BYTE_STREAM_TYPE)
    {
        return STATUS_INVALID_PARAMETER;
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

    if (Buffer->CompletionMode != FILE_PIPE_COMPLETE_OPERATION ||
        Ccb->CompletionMode[NamedPipeEnd] == FILE_PIPE_COMPLETE_OPERATION ||
        (ReadQueue->QueueState != ReadEntries &&
        WriteQueue->QueueState != WriteEntries))
    {
        Ccb->ReadMode[NamedPipeEnd] = Buffer->ReadMode & 0xFF;
        Ccb->CompletionMode[NamedPipeEnd] = Buffer->CompletionMode & 0xFF;

        NpCheckForNotify(Fcb->ParentDcb, FALSE, List);
        Status = STATUS_SUCCESS;
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
    ULONG NamedPipeEnd;
    PAGED_CODE();

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    NodeTypeCode = NpDecodeFileObject(IoStack->FileObject,
                                      (PVOID*)&Fcb,
                                      &Ccb,
                                      &NamedPipeEnd);
    if (!NodeTypeCode) return STATUS_PIPE_DISCONNECTED;
    if (NodeTypeCode != NPFS_NTC_CCB) return STATUS_INVALID_PARAMETER;

    InfoClass = IoStack->Parameters.QueryFile.FileInformationClass;
    Buffer = Irp->AssociatedIrp.SystemBuffer;

    if (InfoClass == FileBasicInformation) return NpSetBasicInfo(Ccb, Buffer);

    if (InfoClass != FilePipeInformation) return STATUS_INVALID_PARAMETER;

    return NpSetPipeInfo(Fcb, Ccb, Buffer, NamedPipeEnd, List);
}

NTSTATUS
NTAPI
NpFsdSetInformation(IN PDEVICE_OBJECT DeviceObject,
                    IN PIRP Irp)
{
    NTSTATUS Status;
    LIST_ENTRY DeferredList;
    PAGED_CODE();

    InitializeListHead(&DeferredList);

    FsRtlEnterFileSystem();
    NpAcquireExclusiveVcb();

    Status = NpCommonSetInformation(DeviceObject, Irp, &DeferredList);

    NpReleaseVcb();
    NpCompleteDeferredIrps(&DeferredList);
    FsRtlExitFileSystem();

    if (Status != STATUS_PENDING)
    {
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NAMED_PIPE_INCREMENT);
    }

    return Status;
}

NTSTATUS
NTAPI
NpQueryBasicInfo(IN PNP_CCB Ccb,
                 IN PVOID Buffer,
                 IN OUT PULONG Length)
{
    PFILE_BASIC_INFORMATION InfoBuffer = Buffer;

    *Length -= sizeof(*InfoBuffer);

    RtlZeroMemory(InfoBuffer, sizeof(*InfoBuffer));
    InfoBuffer->FileAttributes = FILE_ATTRIBUTE_NORMAL;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
NpQueryStandardInfo(IN PNP_CCB Ccb,
                    IN PVOID Buffer,
                    IN OUT PULONG Length,
                    IN ULONG NamedPipeEnd)
{
    PNP_DATA_QUEUE DataQueue;
    PFILE_STANDARD_INFORMATION InfoBuffer = Buffer;

    *Length -= sizeof(*InfoBuffer);

    RtlZeroMemory(InfoBuffer, sizeof(*InfoBuffer));

    if (NamedPipeEnd == FILE_PIPE_SERVER_END)
    {
        DataQueue = &Ccb->DataQueue[FILE_PIPE_INBOUND];
    }
    else
    {
        DataQueue = &Ccb->DataQueue[FILE_PIPE_OUTBOUND];
    }

    InfoBuffer->AllocationSize.LowPart = Ccb->DataQueue[FILE_PIPE_INBOUND].Quota +
                                         Ccb->DataQueue[FILE_PIPE_OUTBOUND].Quota;
    InfoBuffer->AllocationSize.HighPart = 0;

    if (DataQueue->QueueState == WriteEntries)
    {
        InfoBuffer->EndOfFile.HighPart = 0;
        InfoBuffer->EndOfFile.LowPart = DataQueue->BytesInQueue -
                                        DataQueue->ByteOffset;
    }

    InfoBuffer->Directory = FALSE;
    InfoBuffer->NumberOfLinks = 1;
    InfoBuffer->DeletePending = TRUE;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
NpQueryEaInfo(IN PNP_CCB Ccb,
              IN PVOID Buffer,
              IN OUT PULONG Length)
{
    PFILE_EA_INFORMATION InfoBuffer = Buffer;

    *Length -= sizeof(*InfoBuffer);

    RtlZeroMemory(InfoBuffer, sizeof(*InfoBuffer));

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
NpQueryNameInfo(IN PNP_CCB Ccb,
                IN PVOID Buffer,
                IN OUT PULONG Length)
{
    PFILE_NAME_INFORMATION InfoBuffer = Buffer;
    USHORT NameLength;
    NTSTATUS Status;
    PWCHAR Name;

    *Length -= sizeof(*InfoBuffer);

    if (Ccb->NodeType == NPFS_NTC_ROOT_DCB_CCB)
    {
        NameLength = NpVcb->RootDcb->FullName.Length;
        Name = NpVcb->RootDcb->FullName.Buffer;
    }
    else
    {
        NameLength = Ccb->Fcb->FullName.Length;
        Name = Ccb->Fcb->FullName.Buffer;
    }

    if (*Length < NameLength)
    {
        Status = STATUS_BUFFER_OVERFLOW;
        NameLength = (USHORT)*Length;
    }
    else
    {
        Status = STATUS_SUCCESS;
    }

    RtlCopyMemory(InfoBuffer->FileName, Name, NameLength);
    InfoBuffer->FileNameLength = NameLength;

    *Length -= NameLength;
    return Status;
}

NTSTATUS
NTAPI
NpQueryInternalInfo(IN PNP_CCB Ccb,
                    IN PVOID Buffer,
                    IN OUT PULONG Length)
{
    PFILE_INTERNAL_INFORMATION InfoBuffer = Buffer;

    *Length -= sizeof(*InfoBuffer);

    RtlZeroMemory(InfoBuffer, sizeof(*InfoBuffer));

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
NpQueryPositionInfo(IN PNP_CCB Ccb,
                    IN PVOID Buffer,
                    IN OUT PULONG Length,
                    IN ULONG NamedPipeEnd)
{
    PNP_DATA_QUEUE DataQueue;
    PFILE_POSITION_INFORMATION InfoBuffer = Buffer;

    *Length -= sizeof(*InfoBuffer);

    RtlZeroMemory(InfoBuffer, sizeof(*InfoBuffer));

    if (NamedPipeEnd == FILE_PIPE_SERVER_END)
    {
        DataQueue = &Ccb->DataQueue[FILE_PIPE_INBOUND];
    }
    else
    {
        DataQueue = &Ccb->DataQueue[FILE_PIPE_OUTBOUND];
    }

    if (DataQueue->QueueState == WriteEntries)
    {
        InfoBuffer->CurrentByteOffset.QuadPart = DataQueue->BytesInQueue -
                                                 DataQueue->ByteOffset;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
NpQueryPipeLocalInfo(IN PNP_FCB Fcb,
                     IN PNP_CCB Ccb,
                     IN PVOID Buffer,
                     IN OUT PULONG Length,
                     IN ULONG NamedPipeEnd)
{
    PFILE_PIPE_LOCAL_INFORMATION InfoBuffer = Buffer;
    PNP_DATA_QUEUE InQueue, OutQueue;

    *Length -= sizeof(*InfoBuffer);

    RtlZeroMemory(InfoBuffer, sizeof(*InfoBuffer));

    InQueue = &Ccb->DataQueue[FILE_PIPE_INBOUND];
    OutQueue = &Ccb->DataQueue[FILE_PIPE_OUTBOUND];

    InfoBuffer->NamedPipeType = Fcb->NamedPipeType;
    InfoBuffer->NamedPipeConfiguration = Fcb->NamedPipeConfiguration;
    InfoBuffer->MaximumInstances = Fcb->MaximumInstances;
    InfoBuffer->CurrentInstances = Fcb->CurrentInstances;
    InfoBuffer->InboundQuota = InQueue->Quota;
    InfoBuffer->OutboundQuota = OutQueue->Quota;
    InfoBuffer->NamedPipeState = Ccb->NamedPipeState;
    InfoBuffer->NamedPipeEnd = NamedPipeEnd;

    if (NamedPipeEnd == FILE_PIPE_SERVER_END)
    {
        if (InQueue->QueueState == WriteEntries)
        {
            InfoBuffer->ReadDataAvailable = InQueue->BytesInQueue - InQueue->ByteOffset;
        }
        InfoBuffer->WriteQuotaAvailable = OutQueue->Quota - OutQueue->QuotaUsed;
    }
    else
    {
        if (OutQueue->QueueState == WriteEntries)
        {
            InfoBuffer->ReadDataAvailable = OutQueue->BytesInQueue - OutQueue->ByteOffset;
        }
        InfoBuffer->WriteQuotaAvailable = OutQueue->Quota - InQueue->QuotaUsed;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
NpQueryPipeInfo(IN PNP_FCB Fcb,
                IN PNP_CCB Ccb,
                IN PVOID Buffer,
                IN OUT PULONG Length,
                IN ULONG NamedPipeEnd)
{
    PFILE_PIPE_INFORMATION InfoBuffer = Buffer;

    *Length -= sizeof(*InfoBuffer);

    RtlZeroMemory(InfoBuffer, sizeof(*InfoBuffer));

    InfoBuffer->ReadMode = Ccb->ReadMode[NamedPipeEnd];
    InfoBuffer->CompletionMode = Ccb->CompletionMode[NamedPipeEnd];

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
NpCommonQueryInformation(IN PDEVICE_OBJECT DeviceObject,
                         IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    NODE_TYPE_CODE NodeTypeCode;
    ULONG NamedPipeEnd;
    PNP_FCB Fcb;
    PNP_CCB Ccb;
    FILE_INFORMATION_CLASS InfoClass;
    ULONG Length;
    PVOID Buffer;
    PFILE_ALL_INFORMATION AllInfo;
    NTSTATUS Status;
    PAGED_CODE();

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    NodeTypeCode = NpDecodeFileObject(IoStack->FileObject,
                                      (PVOID*)&Fcb,
                                      &Ccb,
                                      &NamedPipeEnd);
    if (!NodeTypeCode) return STATUS_PIPE_DISCONNECTED;

    Buffer = Irp->AssociatedIrp.SystemBuffer;
    Length = IoStack->Parameters.QueryFile.Length;
    InfoClass = IoStack->Parameters.QueryFile.FileInformationClass;

    if (NodeTypeCode != NPFS_NTC_CCB)
    {
        if (NodeTypeCode != NPFS_NTC_ROOT_DCB || InfoClass != FileNameInformation)
        {
            return STATUS_INVALID_PARAMETER;
        }
    }

    switch (InfoClass)
    {
        case FileNameInformation:
            Status = NpQueryNameInfo(Ccb, Buffer, &Length);
            break;

        case FilePositionInformation:
            Status = NpQueryPositionInfo(Ccb, Buffer, &Length, NamedPipeEnd);
            break;

        case FilePipeInformation:
            Status = NpQueryPipeInfo(Fcb, Ccb, Buffer, &Length, NamedPipeEnd);
            break;

        case FilePipeLocalInformation:
            Status = NpQueryPipeLocalInfo(Fcb, Ccb, Buffer, &Length, NamedPipeEnd);
            break;

        case FileBasicInformation:
            Status = NpQueryBasicInfo(Ccb, Buffer, &Length);
            break;

        case FileStandardInformation:
            Status = NpQueryStandardInfo(Ccb, Buffer, &Length, NamedPipeEnd);
            break;

        case FileInternalInformation:
            Status = NpQueryInternalInfo(Ccb, Buffer, &Length);
            break;

        case FileAllInformation:

            Length -= sizeof(FILE_ACCESS_INFORMATION) + sizeof(FILE_MODE_INFORMATION) + sizeof(FILE_ALIGNMENT_INFORMATION);
            AllInfo = (PFILE_ALL_INFORMATION)Buffer;
            NpQueryBasicInfo(Ccb, &AllInfo->BasicInformation, &Length);
            NpQueryStandardInfo(Ccb, &AllInfo->StandardInformation, &Length, NamedPipeEnd);
            NpQueryInternalInfo(Ccb, &AllInfo->InternalInformation, &Length);
            NpQueryEaInfo(Ccb, &AllInfo->EaInformation, &Length);
            NpQueryPositionInfo(Ccb, &AllInfo->PositionInformation, &Length, NamedPipeEnd);
            Status = NpQueryNameInfo(Ccb, &AllInfo->NameInformation, &Length);
            break;

        case FileEaInformation:
            Status = NpQueryEaInfo(Ccb, Buffer, &Length);
            break;

        default:
            Status = STATUS_INVALID_PARAMETER;
            break;
    }

    Irp->IoStatus.Information = IoStack->Parameters.QueryFile.Length - Length;
    return Status;
}

NTSTATUS
NTAPI
NpFsdQueryInformation(IN PDEVICE_OBJECT DeviceObject,
                      IN PIRP Irp)
{
    NTSTATUS Status;
    PAGED_CODE();

    FsRtlEnterFileSystem();
    NpAcquireSharedVcb();

    Status = NpCommonQueryInformation(DeviceObject, Irp);

    NpReleaseVcb();
    FsRtlExitFileSystem();

    if (Status != STATUS_PENDING)
    {
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NAMED_PIPE_INCREMENT);
    }

    return Status;
}

/* EOF */
