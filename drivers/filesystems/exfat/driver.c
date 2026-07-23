/*
 * PROJECT:     ReactOS exFAT filesystem driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Driver entry point and IRP dispatch
 * COPYRIGHT:   Copyright 2026 Ahmed ARIF <arif.ing@outlook.com>
 */

#include "exfat.h"

#define NDEBUG
#include <debug.h>

PEXFAT_GLOBAL_DATA ExFatGlobalData;

static NTSTATUS
ExFatForwardIrp(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PEXFAT_VCB Vcb;

    if (DeviceObject == ExFatGlobalData->DeviceObject)
        return STATUS_INVALID_DEVICE_REQUEST;

    Vcb = DeviceObject->DeviceExtension;
    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(Vcb->StorageDevice, Irp);
}

NTSTATUS
NTAPI
ExFatFsdDispatch(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION Stack;
    NTSTATUS Status;
    BOOLEAN CompleteIrp = TRUE;
    BOOLEAN TopLevel = FALSE;

    FsRtlEnterFileSystem();
    if (!IoGetTopLevelIrp())
    {
        IoSetTopLevelIrp(Irp);
        TopLevel = TRUE;
    }

    Stack = IoGetCurrentIrpStackLocation(Irp);
    Irp->IoStatus.Information = 0;

    switch (Stack->MajorFunction)
    {
        case IRP_MJ_CREATE:
            Status = ExFatCreate(DeviceObject, Irp);
            break;
        case IRP_MJ_CLOSE:
            Status = ExFatClose(DeviceObject, Irp);
            break;
        case IRP_MJ_CLEANUP:
            Status = ExFatCleanup(DeviceObject, Irp);
            break;
        case IRP_MJ_READ:
            Status = ExFatRead(DeviceObject, Irp);
            break;
        case IRP_MJ_WRITE:
            Status = ExFatWrite(DeviceObject, Irp);
            break;
        case IRP_MJ_QUERY_INFORMATION:
            Status = ExFatQueryInformation(DeviceObject, Irp);
            break;
        case IRP_MJ_SET_INFORMATION:
            Status = ExFatSetInformation(DeviceObject, Irp);
            break;
        case IRP_MJ_QUERY_VOLUME_INFORMATION:
            Status = ExFatQueryVolumeInformation(DeviceObject, Irp);
            break;
        case IRP_MJ_SET_VOLUME_INFORMATION:
            Status = ExFatSetVolumeInformation(DeviceObject, Irp);
            break;
        case IRP_MJ_DIRECTORY_CONTROL:
            Status = ExFatDirectoryControl(DeviceObject, Irp);
            break;
        case IRP_MJ_FILE_SYSTEM_CONTROL:
            Status = ExFatFileSystemControl(DeviceObject, Irp);
            break;
        case IRP_MJ_FLUSH_BUFFERS:
            Status = ExFatFlushBuffers(DeviceObject, Irp);
            if (DeviceObject != ExFatGlobalData->DeviceObject)
                CompleteIrp = FALSE;
            break;
        case IRP_MJ_LOCK_CONTROL:
            Status = ExFatLockControl(DeviceObject, Irp);
            CompleteIrp = FALSE;
            break;
        case IRP_MJ_SHUTDOWN:
            Status = ExFatShutdown(DeviceObject, Irp);
            break;
        case IRP_MJ_DEVICE_CONTROL:
        case IRP_MJ_PNP:
            if (DeviceObject == ExFatGlobalData->DeviceObject)
            {
                Status = STATUS_INVALID_DEVICE_REQUEST;
            }
            else
            {
                Status = ExFatForwardIrp(DeviceObject, Irp);
                CompleteIrp = FALSE;
            }
            break;
        default:
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }

    if (CompleteIrp)
    {
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, NT_SUCCESS(Status) ? IO_DISK_INCREMENT : IO_NO_INCREMENT);
    }

    if (TopLevel)
        IoSetTopLevelIrp(NULL);
    FsRtlExitFileSystem();
    return Status;
}

CODE_SEG("INIT")
NTSTATUS
NTAPI
DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath)
{
    UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(EXFAT_DEVICE_NAME);
    PDEVICE_OBJECT DeviceObject;
    ULONG Index;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(RegistryPath);

    Status = IoCreateDevice(DriverObject,
                            sizeof(EXFAT_GLOBAL_DATA),
                            &DeviceName,
                            FILE_DEVICE_DISK_FILE_SYSTEM,
                            0,
                            FALSE,
                            &DeviceObject);
    if (!NT_SUCCESS(Status))
        return Status;

    ExFatGlobalData = DeviceObject->DeviceExtension;
    RtlZeroMemory(ExFatGlobalData, sizeof(*ExFatGlobalData));
    ExFatGlobalData->DriverObject = DriverObject;
    ExFatGlobalData->DeviceObject = DeviceObject;
    ExInitializeResourceLite(&ExFatGlobalData->VolumeListResource);
    ExInitializeNPagedLookasideList(&ExFatGlobalData->CcbLookaside,
                                    NULL,
                                    NULL,
                                    0,
                                    sizeof(EXFAT_CCB),
                                    TAG_EXFAT_CCB,
                                    0);
    ExInitializeNPagedLookasideList(&ExFatGlobalData->FatFsNameBufferLookaside,
                                    NULL,
                                    NULL,
                                    0,
                                    sizeof(EXFAT_FATFS_ALLOCATION_HEADER) + EXFAT_FATFS_NAME_BUFFER_SIZE,
                                    TAG_EXFAT_FATFS,
                                    0);

    ExFatGlobalData->CacheManagerCallbacks.AcquireForLazyWrite = ExFatAcquireForLazyWrite;
    ExFatGlobalData->CacheManagerCallbacks.ReleaseFromLazyWrite = ExFatReleaseFromLazyWrite;
    ExFatGlobalData->CacheManagerCallbacks.AcquireForReadAhead = ExFatAcquireForReadAhead;
    ExFatGlobalData->CacheManagerCallbacks.ReleaseFromReadAhead = ExFatReleaseFromReadAhead;

    for (Index = 0; Index <= IRP_MJ_MAXIMUM_FUNCTION; ++Index)
        DriverObject->MajorFunction[Index] = ExFatFsdDispatch;

    RtlZeroMemory(&ExFatGlobalData->FastIoDispatch,
                  sizeof(ExFatGlobalData->FastIoDispatch));
    ExFatGlobalData->FastIoDispatch.SizeOfFastIoDispatch = sizeof(FAST_IO_DISPATCH);
    ExFatGlobalData->FastIoDispatch.FastIoCheckIfPossible = ExFatFastIoCheckIfPossible;
    ExFatGlobalData->FastIoDispatch.FastIoRead = FsRtlCopyRead;
    ExFatGlobalData->FastIoDispatch.FastIoWrite = FsRtlCopyWrite;
    ExFatGlobalData->FastIoDispatch.MdlRead = FsRtlMdlReadDev;
    ExFatGlobalData->FastIoDispatch.MdlReadComplete = FsRtlMdlReadCompleteDev;
    ExFatGlobalData->FastIoDispatch.PrepareMdlWrite = FsRtlPrepareMdlWriteDev;
    ExFatGlobalData->FastIoDispatch.MdlWriteComplete = FsRtlMdlWriteCompleteDev;
    ExFatGlobalData->FastIoDispatch.AcquireFileForNtCreateSection = ExFatAcquireFileForNtCreateSection;
    ExFatGlobalData->FastIoDispatch.ReleaseFileForNtCreateSection = ExFatReleaseFileForNtCreateSection;
    DriverObject->FastIoDispatch = &ExFatGlobalData->FastIoDispatch;
    DriverObject->DriverUnload = NULL;

    DeviceObject->Flags |= DO_DIRECT_IO;
    IoRegisterFileSystem(DeviceObject);
    IoRegisterShutdownNotification(DeviceObject);
    DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    return STATUS_SUCCESS;
}
