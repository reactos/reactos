/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/filesystems/fastfat/pnp.c
 * PURPOSE:          VFAT Filesystem
 * PROGRAMMER:       Pierre Schweitzer
 *
 */

/* INCLUDES *****************************************************************/

#include "vfat.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

NTSTATUS
VfatPnp(
    PVFAT_IRP_CONTEXT IrpContext)
{
    PVCB Vcb = NULL;
    NTSTATUS Status;

    /* PRECONDITION */
    ASSERT(IrpContext);

    switch (IrpContext->Stack->MinorFunction)
    {
        case IRP_MN_QUERY_REMOVE_DEVICE:
        case IRP_MN_SURPRISE_REMOVAL:
        case IRP_MN_REMOVE_DEVICE:
        case IRP_MN_CANCEL_REMOVE_DEVICE:
            Status = STATUS_NOT_IMPLEMENTED;
            IrpContext->Irp->IoStatus.Status = Status;
            IoCompleteRequest(IrpContext->Irp, IO_NO_INCREMENT);
            break;

        default:
            IoSkipCurrentIrpStackLocation(IrpContext->Irp);
            Vcb = (PVCB)IrpContext->Stack->DeviceObject->DeviceExtension;
            Status = IoCallDriver(Vcb->StorageDevice, IrpContext->Irp);
    }

    VfatFreeIrpContext(IrpContext);

    return Status;
}
