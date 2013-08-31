/*
* COPYRIGHT:        See COPYING in the top level directory
* PROJECT:          ReactOS kernel
* FILE:             drivers/filesystems/npfs/volume.c
* PURPOSE:          Named pipe filesystem
* PROGRAMMER:       Eric Kohl
*/

/* INCLUDES *****************************************************************/

#include "npfs.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

NTSTATUS
NTAPI
NpQueryFsDeviceInformation(IN PFILE_FS_DEVICE_INFORMATION FsDeviceInfo,
                           OUT PULONG BufferLength)
{
    PAGED_CODE();
    DPRINT("NpfsQueryFsDeviceInformation()\n");
    DPRINT("FsDeviceInfo = %p\n", FsDeviceInfo);

    if (*BufferLength < sizeof(FILE_FS_DEVICE_INFORMATION))
    {
        return STATUS_BUFFER_OVERFLOW;
    }

    FsDeviceInfo->DeviceType = FILE_DEVICE_NAMED_PIPE;
    FsDeviceInfo->Characteristics = 0;

    *BufferLength -= sizeof(FILE_FS_DEVICE_INFORMATION);

    DPRINT("NpfsQueryFsDeviceInformation() finished.\n");

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
NpQueryFsFullSizeInfo(IN PFILE_FS_FULL_SIZE_INFORMATION FsSizeInfo,
                      OUT PULONG BufferSize)
{
    RtlZeroMemory(FsSizeInfo, sizeof(FILE_FS_FULL_SIZE_INFORMATION));
    *BufferSize -= sizeof(FILE_FS_FULL_SIZE_INFORMATION);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
NpQueryFsSizeInfo(IN PFILE_FS_SIZE_INFORMATION FsSizeInfo,
                  OUT PULONG BufferSize)
{
    FsSizeInfo->TotalAllocationUnits.LowPart = 0;
    FsSizeInfo->TotalAllocationUnits.HighPart = 0;
    FsSizeInfo->AvailableAllocationUnits.LowPart = 0;
    FsSizeInfo->AvailableAllocationUnits.HighPart = 0;
    FsSizeInfo->SectorsPerAllocationUnit = 1;
    FsSizeInfo->BytesPerSector = 1;
    *BufferSize -= sizeof(FILE_FS_SIZE_INFORMATION);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
NpQueryFsAttributeInformation(IN PFILE_FS_ATTRIBUTE_INFORMATION FsAttributeInfo,
                              OUT PULONG BufferLength)
{
    PAGED_CODE();
    DPRINT("NpfsQueryFsAttributeInformation() called.\n");
    DPRINT("FsAttributeInfo = %p\n", FsAttributeInfo);

    if (*BufferLength < sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + 8)
    {
        *BufferLength = 0;
        return STATUS_BUFFER_OVERFLOW;
    }

    FsAttributeInfo->FileSystemAttributes = FILE_CASE_PRESERVED_NAMES;
    FsAttributeInfo->MaximumComponentNameLength = -1;
    FsAttributeInfo->FileSystemNameLength = 8;
    wcscpy(FsAttributeInfo->FileSystemName, L"NPFS");

    DPRINT("NpfsQueryFsAttributeInformation() finished.\n");
    *BufferLength -= (sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + 8);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
NpCommonQueryVolumeInformation(IN PIRP Irp)
{
    PIO_STACK_LOCATION Stack;
    FS_INFORMATION_CLASS FsInformationClass;
    PVOID SystemBuffer;
    ULONG BufferLength;
    NTSTATUS Status;
    PAGED_CODE();

    Stack = IoGetCurrentIrpStackLocation(Irp);
    FsInformationClass = Stack->Parameters.QueryVolume.FsInformationClass;
    BufferLength = Stack->Parameters.QueryVolume.Length;
    SystemBuffer = Irp->AssociatedIrp.SystemBuffer;

    DPRINT("FsInformationClass %d\n", FsInformationClass);
    DPRINT("SystemBuffer %p\n", SystemBuffer);

    switch (FsInformationClass)
    {
    case FileFsFullSizeInformation:
        Status = NpQueryFsFullSizeInfo(SystemBuffer, &BufferLength);
        break;
    case FileFsSizeInformation:
        Status = NpQueryFsSizeInfo(SystemBuffer, &BufferLength);
        break;

    case FileFsDeviceInformation:
        Status = NpQueryFsDeviceInformation(SystemBuffer, &BufferLength);
        break;

    case FileFsAttributeInformation:
        Status = NpQueryFsAttributeInformation(SystemBuffer, &BufferLength);
        break;

    default:
        DPRINT1("Query not implemented: %d\n", FsInformationClass);
        Status = STATUS_NOT_SUPPORTED;
    }

    Irp->IoStatus.Information = Stack->Parameters.QueryVolume.Length - BufferLength;
    return Status;
}

NTSTATUS
NTAPI
NpfsQueryVolumeInformation(IN PDEVICE_OBJECT DeviceObject,
                           IN PIRP Irp)
{
    NTSTATUS Status;
    PAGED_CODE();
    DPRINT("NpfsQueryVolumeInformation(DeviceObject %p, Irp %p)\n", DeviceObject, Irp);

    FsRtlEnterFileSystem();
    Status = NpCommonQueryVolumeInformation(Irp);
    FsRtlExitFileSystem();

    if (Status != STATUS_PENDING)
    {
        Irp->IoStatus.Status = Status;

        IoCompleteRequest(Irp, IO_DISK_INCREMENT);
    }

    return Status;
}

/* EOF */
