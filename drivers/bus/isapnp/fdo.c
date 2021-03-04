/*
 * PROJECT:         ReactOS ISA PnP Bus driver
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         FDO-specific code
 * COPYRIGHT:       Copyright 2010 Cameron Gutman <cameron.gutman@reactos.org>
 *                  Copyright 2020 Hervé Poussineau <hpoussin@reactos.org>
 */

#include "isapnp.h"

#define NDEBUG
#include <debug.h>

static
CODE_SEG("PAGE")
NTSTATUS
IsaFdoStartDevice(
    _In_ PISAPNP_FDO_EXTENSION FdoExt,
    _Inout_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IrpSp)
{
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(IrpSp);

    PAGED_CODE();

    FdoExt->Common.State = dsStarted;

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
IsaFdoQueryDeviceRelations(
    _In_ PISAPNP_FDO_EXTENSION FdoExt,
    _Inout_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IrpSp)
{
    PAGED_CODE();

    if (IrpSp->Parameters.QueryDeviceRelations.Type != BusRelations)
        return Irp->IoStatus.Status;

    return IsaPnpFillDeviceRelations(FdoExt, Irp, TRUE);
}

CODE_SEG("PAGE")
NTSTATUS
IsaFdoPnp(
    _In_ PISAPNP_FDO_EXTENSION FdoExt,
    _Inout_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IrpSp)
{
    NTSTATUS Status = Irp->IoStatus.Status;

    PAGED_CODE();

    switch (IrpSp->MinorFunction)
    {
        case IRP_MN_START_DEVICE:
            Status = IsaForwardIrpSynchronous(FdoExt, Irp);

            if (NT_SUCCESS(Status))
                Status = IsaFdoStartDevice(FdoExt, Irp, IrpSp);

            Irp->IoStatus.Status = Status;

            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return Status;

        case IRP_MN_STOP_DEVICE:
            FdoExt->Common.State = dsStopped;

            Status = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_DEVICE_RELATIONS:
            Status = IsaFdoQueryDeviceRelations(FdoExt, Irp, IrpSp);

            Irp->IoStatus.Status = Status;

            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return Status;

        case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
            DPRINT("IRP_MN_FILTER_RESOURCE_REQUIREMENTS\n");
            break;

        default:
            DPRINT1("Unknown PnP code: %x\n", IrpSp->MinorFunction);
            break;
    }

    IoSkipCurrentIrpStackLocation(Irp);

    return IoCallDriver(FdoExt->Ldo, Irp);
}
