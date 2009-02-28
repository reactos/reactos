/*
 * PROJECT:         ReactOS FAT file system driver
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/filesystems/fastfat/fsctl.c
 * PURPOSE:         Filesystem control
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include "fastfat.h"

/* FUNCTIONS ****************************************************************/

NTSTATUS
NTAPI
FatUserFsCtrl(PFAT_IRP_CONTEXT IrpContext, PIRP Irp)
{
    DPRINT1("FatUserFsCtrl()\n");
    FatCompleteRequest(IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST);
    return STATUS_INVALID_DEVICE_REQUEST;
}

NTSTATUS
NTAPI
FatVerifyVolume(PFAT_IRP_CONTEXT IrpContext, PIRP Irp)
{
    DPRINT1("FatVerifyVolume()\n");
    FatCompleteRequest(IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST);
    return STATUS_INVALID_DEVICE_REQUEST;
}

VOID
NTAPI
FatiCleanVcbs(PFAT_IRP_CONTEXT IrpContext)
{
    /* Make sure this IRP is waitable */
    ASSERT(IrpContext->Flags & IRPCONTEXT_CANWAIT);

    /* Acquire global resource */
    ExAcquireResourceExclusiveLite(&FatGlobalData.Resource, TRUE);

    /* TODO: Go through all VCBs and delete unmounted ones */

    /* Release global resource */
    ExReleaseResourceLite(&FatGlobalData.Resource);
}

NTSTATUS
NTAPI
FatMountVolume(PFAT_IRP_CONTEXT IrpContext,
               PDEVICE_OBJECT TargetDeviceObject,
               PVPB Vpb,
               PDEVICE_OBJECT FsDeviceObject)
{
    NTSTATUS Status;
    DISK_GEOMETRY DiskGeometry;
    ULONG MediaChangeCount = 0;
    PVOLUME_DEVICE_OBJECT VolumeDevice;

    DPRINT1("FatMountVolume()\n");

    /* Make sure this IRP is waitable */
    ASSERT(IrpContext->Flags & IRPCONTEXT_CANWAIT);

    /* Request media changes count, mostly usefull for removable devices */
    Status = FatPerformDevIoCtrl(TargetDeviceObject,
                                 IOCTL_STORAGE_CHECK_VERIFY,
                                 NULL,
                                 0,
                                 &MediaChangeCount,
                                 sizeof(ULONG),
                                 TRUE);

    if (!NT_SUCCESS(Status)) return Status;

    /* TODO: Check if data-track present in case of a CD drive */
    /* TODO: IOCTL_DISK_GET_PARTITION_INFO_EX */

    /* Remove unmounted VCBs */
    FatiCleanVcbs(IrpContext);

    /* Create a new volume device object */
    Status = IoCreateDevice(FatGlobalData.DriverObject,
                            sizeof(VOLUME_DEVICE_OBJECT) - sizeof(DEVICE_OBJECT),
                            NULL,
                            FILE_DEVICE_DISK_FILE_SYSTEM,
                            0,
                            FALSE,
                            (PDEVICE_OBJECT *)&VolumeDevice);

    if (!NT_SUCCESS(Status)) return Status;

    /* Match alignment requirements */
    if (TargetDeviceObject->AlignmentRequirement > VolumeDevice->DeviceObject.AlignmentRequirement)
    {
        VolumeDevice->DeviceObject.AlignmentRequirement = TargetDeviceObject->AlignmentRequirement;
    }

    /* Init stack size */
    VolumeDevice->DeviceObject.StackSize = TargetDeviceObject->StackSize + 1;

    /* Get sector size */
    Status = FatPerformDevIoCtrl(TargetDeviceObject,
                                 IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                 NULL,
                                 0,
                                 &DiskGeometry,
                                 sizeof(DISK_GEOMETRY),
                                 TRUE);

    if (!NT_SUCCESS(Status)) goto FatMountVolumeCleanup;

    VolumeDevice->DeviceObject.SectorSize = (USHORT) DiskGeometry.BytesPerSector;

    /* Signal we're done with initializing */
    VolumeDevice->DeviceObject.Flags &= ~DO_DEVICE_INITIALIZING;

    /* Save device object in a VPB */
    Vpb->DeviceObject = (PDEVICE_OBJECT)VolumeDevice;

    /* Initialize VCB for this volume */
    Status = FatInitializeVcb(&VolumeDevice->Vcb, TargetDeviceObject, Vpb);
    if (!NT_SUCCESS(Status)) goto FatMountVolumeCleanup;

    /* Keep trace of media changes */
    VolumeDevice->Vcb.MediaChangeCount = MediaChangeCount;

    /* Notify about volume mount */
    FsRtlNotifyVolumeEvent(VolumeDevice->Vcb.StreamFileObject, FSRTL_VOLUME_MOUNT);

    /* Return success */
    return STATUS_SUCCESS;


FatMountVolumeCleanup:

    /* Unwind the routine actions */
    IoDeleteDevice((PDEVICE_OBJECT)VolumeDevice);
    return Status;
}



NTSTATUS
NTAPI
FatiFileSystemControl(PFAT_IRP_CONTEXT IrpContext, PIRP Irp)
{
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status;

    /* Get current IRP stack location */
    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    /* Dispatch depending on the minor function */
    switch (IrpSp->MinorFunction)
    {
    case IRP_MN_USER_FS_REQUEST:
        Status = FatUserFsCtrl(IrpContext, Irp);
        break;

    case IRP_MN_MOUNT_VOLUME:
        Status = FatMountVolume(IrpContext,
                                IrpSp->Parameters.MountVolume.DeviceObject,
                                IrpSp->Parameters.MountVolume.Vpb,
                                IrpSp->DeviceObject);

        FatCompleteRequest(IrpContext, Irp, Status);

        break;

    case IRP_MN_VERIFY_VOLUME:
        Status = FatVerifyVolume(IrpContext, Irp);
        break;

    default:
        DPRINT1("Unhandled FSCTL minor 0x%x\n", IrpSp->MinorFunction);
        FatCompleteRequest(IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST);
        Status = STATUS_INVALID_DEVICE_REQUEST;
    }

    return Status;
}


NTSTATUS
NTAPI
FatFileSystemControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PFAT_IRP_CONTEXT IrpContext;
    BOOLEAN CanWait = TRUE;

    DPRINT1("FatFileSystemControl(DeviceObject %p, Irp %p)\n", DeviceObject, Irp);

    /* Get CanWait flag */
    if (IoGetCurrentIrpStackLocation(Irp)->FileObject)
    {
        CanWait = IoIsOperationSynchronous(Irp);
    }

    /* Enter FsRtl critical region */
    FsRtlEnterFileSystem();

    /* Build an irp context */
    IrpContext = FatBuildIrpContext(Irp, CanWait);

    /* Call internal function */
    Status = FatiFileSystemControl(IrpContext, Irp);

    /* Leave FsRtl critical region */
    FsRtlExitFileSystem();

    return Status;
}
