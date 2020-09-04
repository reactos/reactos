/*
 * PROJECT:     ReactOS Named Pipe FileSystem
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * FILE:        drivers/filesystems/npfs/volinfo.c
 * PURPOSE:     Named Pipe FileSystem Volume Information
 * PROGRAMMERS: ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "npfs.h"

// File ID number for NPFS bugchecking support
#define NPFS_BUGCHECK_FILE_ID   (NPFS_BUGCHECK_VOLINFO)

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
NpQueryFsVolumeInfo(IN PVOID Buffer,
                    IN OUT PULONG Length)
{
    PFILE_FS_VOLUME_INFORMATION InfoBuffer = Buffer;
    NTSTATUS Status;
    USHORT NameLength;
    TRACE("Entered\n");

    *Length -= FIELD_OFFSET(FILE_FS_VOLUME_INFORMATION, VolumeLabel);

    InfoBuffer->SupportsObjects = 0;

    NameLength = 18;
    InfoBuffer->VolumeLabelLength = 18;

    if (*Length < 18)
    {
        NameLength = (USHORT)*Length;
        Status = STATUS_BUFFER_OVERFLOW;
    }
    else
    {
        Status = STATUS_SUCCESS;
    }

    RtlCopyMemory(InfoBuffer->VolumeLabel, L"NamedPipe", NameLength);
    *Length -= NameLength;

    TRACE("Leaving, Status = %lx\n", Status);
    return Status;
}

NTSTATUS
NTAPI
NpQueryFsSizeInfo(IN PVOID Buffer,
                  IN OUT PULONG Length)
{
    PFILE_FS_SIZE_INFORMATION InfoBuffer = Buffer;
    TRACE("Entered\n");

    *Length -= sizeof(*InfoBuffer);

    InfoBuffer->SectorsPerAllocationUnit = 1;
    InfoBuffer->BytesPerSector = 1;

    TRACE("Leaving, Status = STATUS_SUCCESS\n");
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
NpQueryFsDeviceInfo(IN PVOID Buffer,
                    IN OUT PULONG Length)
{
    PFILE_FS_DEVICE_INFORMATION InfoBuffer = Buffer;
    TRACE("Entered\n");

    InfoBuffer->DeviceType = FILE_DEVICE_NAMED_PIPE;
    *Length -= sizeof(*InfoBuffer);

    TRACE("Leaving, Status = STATUS_SUCCESS\n");
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
NpQueryFsAttributeInfo(IN PVOID Buffer,
                       IN OUT PULONG Length)
{
    PFILE_FS_ATTRIBUTE_INFORMATION InfoBuffer = Buffer;
    NTSTATUS Status;
    USHORT NameLength;
    TRACE("Entered\n");

    NameLength = (USHORT)(*Length - 12);
    if (NameLength < 8)
    {
        *Length = 0;
        Status = STATUS_BUFFER_OVERFLOW;
    }
    else
    {
        *Length -= 20;
        NameLength = 8;
        Status = STATUS_SUCCESS;
    }

    InfoBuffer->MaximumComponentNameLength = 0xFFFFFFFF;
    InfoBuffer->FileSystemNameLength = 8;
    InfoBuffer->FileSystemAttributes = FILE_CASE_PRESERVED_NAMES;
    RtlCopyMemory(InfoBuffer->FileSystemName, L"NPFS", NameLength);

    TRACE("Leaving, Status = %lx\n", Status);
    return Status;
}

NTSTATUS
NTAPI
NpQueryFsFullSizeInfo(IN PVOID Buffer,
                      IN OUT PULONG Length)
{
    PFILE_FS_FULL_SIZE_INFORMATION InfoBuffer = Buffer;
    TRACE("Entered\n");

    *Length -= sizeof(*InfoBuffer);

    RtlZeroMemory(InfoBuffer, sizeof(*InfoBuffer));

    TRACE("Leaving, Status = STATUS_SUCCESS\n");
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
NpCommonQueryVolumeInformation(IN PDEVICE_OBJECT DeviceObject,
                               IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    FS_INFORMATION_CLASS InfoClass;
    ULONG Length;
    PVOID Buffer;
    NTSTATUS Status;
    PAGED_CODE();
    TRACE("Entered\n");

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    Buffer = Irp->AssociatedIrp.SystemBuffer;
    Length = IoStack->Parameters.QueryVolume.Length;
    InfoClass = IoStack->Parameters.QueryVolume.FsInformationClass;

    RtlZeroMemory(Buffer, Length);

    switch (InfoClass)
    {
        case FileFsVolumeInformation:
            Status = NpQueryFsVolumeInfo(Buffer, &Length);
            break;
        case FileFsSizeInformation:
            Status = NpQueryFsSizeInfo(Buffer, &Length);
            break;
        case FileFsDeviceInformation:
            Status = NpQueryFsDeviceInfo(Buffer, &Length);
            break;
        case FileFsAttributeInformation:
            Status = NpQueryFsAttributeInfo(Buffer, &Length);
            break;
        case FileFsFullSizeInformation:
            Status = NpQueryFsFullSizeInfo(Buffer, &Length);
            break;
        default:
            Status = STATUS_NOT_SUPPORTED;
            break;
    }

    Irp->IoStatus.Information = IoStack->Parameters.QueryVolume.Length - Length;
    TRACE("Leaving, Status = %lx\n", Status);
    return Status;
}

NTSTATUS
NTAPI
NpFsdQueryVolumeInformation(IN PDEVICE_OBJECT DeviceObject,
                            IN PIRP Irp)
{
    NTSTATUS Status;
    PAGED_CODE();
    TRACE("Entered\n");

    FsRtlEnterFileSystem();
    NpAcquireSharedVcb();

    Status = NpCommonQueryVolumeInformation(DeviceObject, Irp);

    NpReleaseVcb();
    FsRtlExitFileSystem();

    if (Status != STATUS_PENDING)
    {
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NAMED_PIPE_INCREMENT);
    }

    TRACE("Leaving, Status = %lx\n", Status);
    return Status;
}

/* EOF */
