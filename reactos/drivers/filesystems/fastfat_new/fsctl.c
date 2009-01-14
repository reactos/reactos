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

NTSTATUS
NTAPI
FatMountVolume(PFAT_IRP_CONTEXT IrpContext,
               PDEVICE_OBJECT TargetDeviceObject,
               PVPB Vpb,
               PDEVICE_OBJECT FsDeviceObject)
{
    DPRINT1("FatMountVolume()\n");

    FatCompleteRequest(IrpContext, IrpContext->Irp, STATUS_INVALID_DEVICE_REQUEST);
    return STATUS_INVALID_DEVICE_REQUEST;
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
