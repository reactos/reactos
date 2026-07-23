/*
 * PROJECT:     ReactOS exFAT filesystem driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Volume mounting and filesystem controls
 * COPYRIGHT:   Copyright 2026 Ahmed ARIF <arif.ing@outlook.com>
 */

#include "exfat.h"

#define NDEBUG
#include <debug.h>

static BOOLEAN
ExFatIsBootSector(
    const UCHAR* BootSector)
{
    return RtlCompareMemory(&BootSector[EXFAT_NAME_OFFSET],
                            "EXFAT   ",
                            EXFAT_NAME_LENGTH) == EXFAT_NAME_LENGTH &&
           BootSector[EXFAT_BOOT_SIGNATURE_OFFSET] == 0x55 &&
           BootSector[EXFAT_BOOT_SIGNATURE_OFFSET + 1] == 0xAA;
}

static NTSTATUS
ExFatMountVolume(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_OBJECT StorageDevice = Stack->Parameters.MountVolume.DeviceObject;
    PVPB Vpb = Stack->Parameters.MountVolume.Vpb;
    PDEVICE_OBJECT VolumeDevice = NULL;
    PEXFAT_VCB Vcb = NULL;
    DISK_GEOMETRY Geometry;
    GET_LENGTH_INFORMATION LengthInformation;
    ULONG OutputLength;
    PUCHAR BootSector = NULL;
    ULONG BootSectorLength;
    ULONG DriveNumber;
    BYTE SectorShift;
    TCHAR DrivePath[3];
    TCHAR Label[FF_LFN_BUF + 1];
    UNICODE_STRING UnicodeLabel;
    UNICODE_STRING VolumePath = RTL_CONSTANT_STRING(L"");
    FRESULT Result;
    NTSTATUS Status;
    BOOLEAN Registered = FALSE;

    if (DeviceObject != ExFatGlobalData->DeviceObject)
        return STATUS_INVALID_DEVICE_REQUEST;

    OutputLength = sizeof(Geometry);
    Status = ExFatDeviceIoControl(StorageDevice,
                                  IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                  NULL,
                                  0,
                                  &Geometry,
                                  &OutputLength);
    if (!NT_SUCCESS(Status) || Geometry.BytesPerSector < 512)
        return STATUS_UNRECOGNIZED_VOLUME;

    BootSectorLength = max(Geometry.BytesPerSector, 512UL);
    BootSector = ExAllocatePoolWithTag(NonPagedPool,
                                       BootSectorLength,
                                       TAG_EXFAT_IO);
    if (!BootSector)
        return STATUS_INSUFFICIENT_RESOURCES;

    {
        LARGE_INTEGER Offset;
        Offset.QuadPart = 0;
        Status = ExFatReadWriteDevice(StorageDevice,
                                      IRP_MJ_READ,
                                      BootSector,
                                      BootSectorLength,
                                      &Offset,
                                      TRUE);
    }
    if (!NT_SUCCESS(Status) || !ExFatIsBootSector(BootSector))
    {
        Status = STATUS_UNRECOGNIZED_VOLUME;
        goto Failure;
    }

    SectorShift = BootSector[EXFAT_SECTOR_SHIFT_OFFSET];
    if (SectorShift < EXFAT_MIN_SECTOR_SHIFT || SectorShift > EXFAT_MAX_SECTOR_SHIFT)
    {
        Status = STATUS_UNRECOGNIZED_VOLUME;
        goto Failure;
    }

    OutputLength = sizeof(LengthInformation);
    Status = ExFatDeviceIoControl(StorageDevice,
                                  IOCTL_DISK_GET_LENGTH_INFO,
                                  NULL,
                                  0,
                                  &LengthInformation,
                                  &OutputLength);
    if (!NT_SUCCESS(Status) || LengthInformation.Length.QuadPart <= 0)
        goto Failure;

    Status = IoCreateDevice(ExFatGlobalData->DriverObject,
                            sizeof(EXFAT_VCB),
                            NULL,
                            FILE_DEVICE_DISK_FILE_SYSTEM,
                            0,
                            FALSE,
                            &VolumeDevice);
    if (!NT_SUCCESS(Status))
        goto Failure;

    Vcb = VolumeDevice->DeviceExtension;
    RtlZeroMemory(Vcb, sizeof(*Vcb));
    Vcb->DeviceObject = VolumeDevice;
    Vcb->StorageDevice = StorageDevice;
    Vcb->Vpb = Vpb;
    ExInitializeResourceLite(&Vcb->Resource);
    ExInitializeResourceLite(&Vcb->FatFsResource);
    InitializeListHead(&Vcb->FcbListHead);

    Vcb->BytesPerSector = 1UL << SectorShift;
    Vcb->SectorCount = LengthInformation.Length.QuadPart / Vcb->BytesPerSector;
    if (!Vcb->SectorCount ||
        LengthInformation.Length.QuadPart % Vcb->BytesPerSector)
    {
        Status = STATUS_UNRECOGNIZED_VOLUME;
        goto Failure;
    }

    Status = ExFatDeviceIoControl(StorageDevice,
                                  IOCTL_DISK_IS_WRITABLE,
                                  NULL,
                                  0,
                                  NULL,
                                  NULL);
    Vcb->ReadOnly = (Status == STATUS_MEDIA_WRITE_PROTECTED);

    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&ExFatGlobalData->VolumeListResource, TRUE);
    for (DriveNumber = 0; DriveNumber < FF_VOLUMES; ++DriveNumber)
    {
        if (!ExFatGlobalData->Volumes[DriveNumber])
            break;
    }
    if (DriveNumber == FF_VOLUMES)
    {
        ExReleaseResourceLite(&ExFatGlobalData->VolumeListResource);
        KeLeaveCriticalRegion();
        Status = STATUS_TOO_MANY_OPENED_FILES;
        goto Failure;
    }

    Vcb->DriveNumber = (BYTE)DriveNumber;
    Vcb->Mounted = TRUE;
    ExFatGlobalData->Volumes[DriveNumber] = Vcb;
    VolToPart[DriveNumber].pd = (BYTE)DriveNumber;
    VolToPart[DriveNumber].pt = 0;
    Registered = TRUE;

    ExFatBuildDrivePath(Vcb, DrivePath);
    ExFatAcquireFatFs(Vcb);
    Result = f_mount(&Vcb->FileSystem, DrivePath, 1);
    if (Result != FR_OK || Vcb->FileSystem.fs_type != FS_EXFAT)
    {
        Status = (Result == FR_OK) ? STATUS_UNRECOGNIZED_VOLUME : ExFatMapResult(Result);
        f_mount(NULL, DrivePath, 0);
        ExFatReleaseFatFs(Vcb);
        ExFatGlobalData->Volumes[DriveNumber] = NULL;
        Registered = FALSE;
        ExReleaseResourceLite(&ExFatGlobalData->VolumeListResource);
        KeLeaveCriticalRegion();
        goto Failure;
    }

    Vcb->BytesPerCluster = Vcb->FileSystem.csize * Vcb->BytesPerSector;
    RtlZeroMemory(Label, sizeof(Label));
    Result = f_getlabel(DrivePath, Label, &Vcb->SerialNumber);
    ExFatReleaseFatFs(Vcb);
    ExReleaseResourceLite(&ExFatGlobalData->VolumeListResource);
    KeLeaveCriticalRegion();
    if (Result != FR_OK)
    {
        Status = ExFatMapResult(Result);
        goto Failure;
    }

    Vcb->VolumeFcb = ExFatCreateFcb(Vcb, &VolumePath, NULL, TRUE);
    if (!Vcb->VolumeFcb)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Failure;
    }

    VolumeDevice->Flags |= DO_DIRECT_IO;
    VolumeDevice->AlignmentRequirement = StorageDevice->AlignmentRequirement;
    VolumeDevice->SectorSize = (USHORT)Vcb->BytesPerSector;
    VolumeDevice->StackSize = StorageDevice->StackSize + 1;
    VolumeDevice->Vpb = Vpb;
    StorageDevice->Vpb = Vpb;

    Vcb->StreamFileObject = IoCreateStreamFileObject(NULL, StorageDevice);
    if (!Vcb->StreamFileObject)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Failure;
    }
    Vcb->StreamFileObject->FsContext = Vcb->VolumeFcb;
    Vcb->StreamFileObject->SectionObjectPointer = &Vcb->VolumeFcb->SectionObjectPointers;
    Vcb->StreamFileObject->PrivateCacheMap = NULL;
    Vcb->StreamFileObject->Vpb = Vpb;

    StorageDevice->Vpb->DeviceObject = VolumeDevice;
    StorageDevice->Vpb->RealDevice = StorageDevice;
    Vpb->SerialNumber = Vcb->SerialNumber;
    Vpb->VolumeLabelLength = 0;
    if (Label[0])
    {
        RtlInitUnicodeString(&UnicodeLabel, Label);
        Vpb->VolumeLabelLength = min(UnicodeLabel.Length,
                                     (USHORT)sizeof(Vpb->VolumeLabel));
        RtlCopyMemory(Vpb->VolumeLabel,
                      UnicodeLabel.Buffer,
                      Vpb->VolumeLabelLength);
    }
    StorageDevice->Vpb->Flags |= VPB_MOUNTED;

    VolumeDevice->Flags &= ~DO_DEVICE_INITIALIZING;

    ExFreePoolWithTag(BootSector, TAG_EXFAT_IO);
    return STATUS_SUCCESS;

Failure:
    if (Vcb && Vcb->StreamFileObject)
    {
        ObDereferenceObject(Vcb->StreamFileObject);
        Vcb->StreamFileObject = NULL;
    }
    if (Registered)
    {
        KeEnterCriticalRegion();
        ExAcquireResourceExclusiveLite(&ExFatGlobalData->VolumeListResource, TRUE);
        ExFatAcquireFatFs(Vcb);
        f_mount(NULL, DrivePath, 0);
        Vcb->Mounted = FALSE;
        ExFatReleaseFatFs(Vcb);
        ExFatGlobalData->Volumes[Vcb->DriveNumber] = NULL;
        ExReleaseResourceLite(&ExFatGlobalData->VolumeListResource);
        KeLeaveCriticalRegion();
    }
    if (Vcb && Vcb->VolumeFcb)
        ExFatDereferenceFcb(Vcb->VolumeFcb);
    if (Vcb)
    {
        ExFatFreeSectorCache(Vcb);
        ExDeleteResourceLite(&Vcb->FatFsResource);
        ExDeleteResourceLite(&Vcb->Resource);
    }
    if (VolumeDevice)
        IoDeleteDevice(VolumeDevice);
    if (BootSector)
        ExFreePoolWithTag(BootSector, TAG_EXFAT_IO);
    return Status;
}

static NTSTATUS
ExFatVerifyVolume(
    PDEVICE_OBJECT DeviceObject)
{
    PEXFAT_VCB Vcb;
    PUCHAR BootSector;
    LARGE_INTEGER Offset;
    NTSTATUS Status;

    if (DeviceObject == ExFatGlobalData->DeviceObject)
        return STATUS_INVALID_DEVICE_REQUEST;

    Vcb = DeviceObject->DeviceExtension;
    BootSector = ExAllocatePoolWithTag(NonPagedPool,
                                       Vcb->BytesPerSector,
                                       TAG_EXFAT_IO);
    if (!BootSector)
        return STATUS_INSUFFICIENT_RESOURCES;

    Offset.QuadPart = 0;
    Status = ExFatReadWriteDevice(Vcb->StorageDevice,
                                  IRP_MJ_READ,
                                  BootSector,
                                  Vcb->BytesPerSector,
                                  &Offset,
                                  TRUE);
    if (!NT_SUCCESS(Status) || !ExFatIsBootSector(BootSector))
    {
        ExFreePoolWithTag(BootSector, TAG_EXFAT_IO);
        return STATUS_WRONG_VOLUME;
    }

    ExFreePoolWithTag(BootSector, TAG_EXFAT_IO);
    Vcb->StorageDevice->Flags &= ~DO_VERIFY_VOLUME;
    return STATUS_SUCCESS;
}

static NTSTATUS
ExFatUserFsRequest(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
    PEXFAT_VCB Vcb;
    PFILE_OBJECT FileObject = Stack->FileObject;
    NTSTATUS Status;

    if (DeviceObject == ExFatGlobalData->DeviceObject)
        return STATUS_INVALID_DEVICE_REQUEST;
    Vcb = DeviceObject->DeviceExtension;

    ExAcquireResourceExclusiveLite(&Vcb->Resource, TRUE);
    switch (Stack->Parameters.FileSystemControl.FsControlCode)
    {
        case FSCTL_LOCK_VOLUME:
            if (Vcb->Locked && Vcb->LockOwner == FileObject)
            {
                Status = STATUS_SUCCESS;
            }
            else if (Vcb->Locked || Vcb->OpenHandleCount > 1)
            {
                Status = STATUS_ACCESS_DENIED;
            }
            else
            {
                /* Raw access follows; drop cached namespace state. */
                ExFatPurgeCachedFcbs(Vcb);
                ExFatAcquireFatFs(Vcb);
                Status = ExFatFlushSectorCache(Vcb);
                ExFatReleaseFatFs(Vcb);
                if (!NT_SUCCESS(Status))
                    break;
                ExFatDropDirIndexes(Vcb);
                Vcb->Locked = TRUE;
                Vcb->LockOwner = FileObject;
                Status = STATUS_SUCCESS;
            }
            break;

        case FSCTL_UNLOCK_VOLUME:
            if (!Vcb->Locked || Vcb->LockOwner != FileObject)
            {
                Status = STATUS_ACCESS_DENIED;
            }
            else
            {
                Vcb->NamespaceGeneration++;
                ExFatDropDirIndexes(Vcb);
                Vcb->Locked = FALSE;
                Vcb->LockOwner = NULL;
                Status = STATUS_SUCCESS;
            }
            break;

        case FSCTL_IS_VOLUME_MOUNTED:
            Status = Vcb->Mounted ? STATUS_SUCCESS : STATUS_VOLUME_DISMOUNTED;
            break;

        case FSCTL_IS_VOLUME_DIRTY:
            if (Stack->Parameters.FileSystemControl.OutputBufferLength < sizeof(ULONG) ||
                !Irp->AssociatedIrp.SystemBuffer)
            {
                Status = STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
                *(PULONG)Irp->AssociatedIrp.SystemBuffer = 0;
                Irp->IoStatus.Information = sizeof(ULONG);
                Status = STATUS_SUCCESS;
            }
            break;

        case FSCTL_ALLOW_EXTENDED_DASD_IO:
            Status = STATUS_SUCCESS;
            break;

        default:
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }
    ExReleaseResourceLite(&Vcb->Resource);
    return Status;
}

NTSTATUS
ExFatFileSystemControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);

    switch (Stack->MinorFunction)
    {
        case IRP_MN_MOUNT_VOLUME:
            return ExFatMountVolume(DeviceObject, Irp);
        case IRP_MN_VERIFY_VOLUME:
            return ExFatVerifyVolume(DeviceObject);
        case IRP_MN_USER_FS_REQUEST:
            return ExFatUserFsRequest(DeviceObject, Irp);
        default:
            return STATUS_INVALID_DEVICE_REQUEST;
    }
}

static NTSTATUS
NTAPI
ExFatFlushCompletion(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context)
{
    NTSTATUS FileSystemStatus = (NTSTATUS)(ULONG_PTR)Context;

    UNREFERENCED_PARAMETER(DeviceObject);

    if (Irp->PendingReturned)
        IoMarkIrpPending(Irp);

    if (NT_SUCCESS(Irp->IoStatus.Status) ||
        Irp->IoStatus.Status == STATUS_INVALID_DEVICE_REQUEST)
    {
        Irp->IoStatus.Status = FileSystemStatus;
    }
    return STATUS_SUCCESS;
}

NTSTATUS
ExFatFlushBuffers(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
    PEXFAT_VCB Vcb;
    PEXFAT_FCB Fcb;
    NTSTATUS Status = STATUS_SUCCESS;
    IO_STATUS_BLOCK IoStatus;
    FRESULT Result = FR_OK;

    if (DeviceObject == ExFatGlobalData->DeviceObject)
        return STATUS_SUCCESS;
    Vcb = DeviceObject->DeviceExtension;
    Fcb = Stack->FileObject ? Stack->FileObject->FsContext : NULL;

    if (Fcb && !Fcb->IsDirectory && !Fcb->IsVolume &&
        Fcb->SectionObjectPointers.DataSectionObject)
    {
        CcFlushCache(&Fcb->SectionObjectPointers, NULL, 0, &IoStatus);
        if (!NT_SUCCESS(IoStatus.Status))
            Status = IoStatus.Status;
    }

    if (NT_SUCCESS(Status))
    {
        ExFatAcquireFatFs(Vcb);
        if (Fcb && Fcb->FatFileOpen)
            Result = f_sync(&Fcb->FatFile);
        if (Result == FR_OK && !NT_SUCCESS(ExFatFlushSectorCache(Vcb)))
            Result = FR_DISK_ERR;
        ExFatReleaseFatFs(Vcb);
        Status = ExFatMapResult(Result);
    }

    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine(Irp,
                           ExFatFlushCompletion,
                           (PVOID)(ULONG_PTR)Status,
                           TRUE,
                           TRUE,
                           TRUE);
    return IoCallDriver(Vcb->StorageDevice, Irp);
}

NTSTATUS
ExFatLockControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
    PEXFAT_FCB Fcb;
    NTSTATUS Status;

    if (DeviceObject == ExFatGlobalData->DeviceObject || !Stack->FileObject)
        Status = STATUS_INVALID_DEVICE_REQUEST;
    else
    {
        Fcb = Stack->FileObject->FsContext;
        if (!Fcb || Fcb->IsDirectory || Fcb->IsVolume)
            Status = STATUS_INVALID_PARAMETER;
        else
            return FsRtlProcessFileLock(&Fcb->FileLock, Irp, NULL);
    }

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

NTSTATUS
ExFatShutdown(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    ULONG Index;

    UNREFERENCED_PARAMETER(Irp);
    if (DeviceObject != ExFatGlobalData->DeviceObject)
        return STATUS_SUCCESS;

    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&ExFatGlobalData->VolumeListResource, TRUE);
    for (Index = 0; Index < FF_VOLUMES; ++Index)
    {
        PEXFAT_VCB Vcb = ExFatGlobalData->Volumes[Index];
        PLIST_ENTRY Entry;
        PEXFAT_FCB Fcb;
        TCHAR DrivePath[3];

        if (!Vcb || !Vcb->Mounted)
            continue;
        ExAcquireResourceExclusiveLite(&Vcb->Resource, TRUE);
        ExFatAcquireFatFs(Vcb);
        for (Entry = Vcb->FcbListHead.Flink;
             Entry != &Vcb->FcbListHead;
             Entry = Entry->Flink)
        {
            Fcb = CONTAINING_RECORD(Entry, EXFAT_FCB, ListEntry);
            if (Fcb->FatFileOpen)
                f_sync(&Fcb->FatFile);
        }
        ExFatFlushSectorCache(Vcb);
        ExFatFlushStorageDevice(Vcb);
        ExFatBuildDrivePath(Vcb, DrivePath);
        f_mount(NULL, DrivePath, 0);
        Vcb->Mounted = FALSE;
        ExFatReleaseFatFs(Vcb);
        ExReleaseResourceLite(&Vcb->Resource);
    }
    ExReleaseResourceLite(&ExFatGlobalData->VolumeListResource);
    KeLeaveCriticalRegion();
    return STATUS_SUCCESS;
}
