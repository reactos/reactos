/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/filesystems/cdfs/devctrl.c
 * PURPOSE:          CDROM (ISO 9660) filesystem driver
 * PROGRAMMER:       Pierre Schweitzer
 *
 */

/* INCLUDES *****************************************************************/

#include "cdfs.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

NTSTATUS NTAPI
CdfsDeviceControl(
    PCDFS_IRP_CONTEXT IrpContext)
{
    PIRP Irp;
    NTSTATUS Status;
    PVCB Vcb = NULL;
    PFILE_OBJECT FileObject;
    PIO_STACK_LOCATION Stack;

    DPRINT("CdfsDeviceControl()\n");

    ASSERT(IrpContext);

    Irp = IrpContext->Irp;
    Stack = IrpContext->Stack;
    FileObject = Stack->FileObject;
    Irp->IoStatus.Information = 0;

    /* FIXME: HACK, it means that CD has changed */
    if (!FileObject)
    {
        DPRINT1("FIXME: CdfsDeviceControl called without FileObject!\n");
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    /* Only support such operations on volume */
    if (!(FileObject->RelatedFileObject == NULL || FileObject->RelatedFileObject->FsContext2 != NULL))
    {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        return STATUS_INVALID_PARAMETER;
    }

    if (Stack->Parameters.DeviceIoControl.IoControlCode == IOCTL_CDROM_DISK_TYPE)
    {
        /* We should handle this one, but we don't! */
        Status = STATUS_NOT_IMPLEMENTED;
        Irp->IoStatus.Status = Status;
    }
    else
    {
        /* Pass it to storage driver */
        IoSkipCurrentIrpStackLocation(Irp);
        Vcb = (PVCB)Stack->DeviceObject->DeviceExtension;

        /* Lower driver will complete - we don't have to */
        IrpContext->Flags &= ~IRPCONTEXT_COMPLETE;

        Status = IoCallDriver(Vcb->StorageDevice, Irp);
    }

    return Status;
}
