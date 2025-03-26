/*
 * PROJECT:     VFAT Filesystem
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Plug & Play handlers
 * COPYRIGHT:   Copyright 2010-2015 Pierre Schweitzer <pierre@reactos.org>
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
            break;

        default:
            IoSkipCurrentIrpStackLocation(IrpContext->Irp);
            Vcb = (PVCB)IrpContext->Stack->DeviceObject->DeviceExtension;
            IrpContext->Flags &= ~IRPCONTEXT_COMPLETE;
            Status = IoCallDriver(Vcb->StorageDevice, IrpContext->Irp);
    }

    return Status;
}
