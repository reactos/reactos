/*
 * PROJECT:         ReactOS ISA PnP Bus driver
 * FILE:            fdo.c
 * PURPOSE:         FDO-specific code
 * PROGRAMMERS:     Cameron Gutman (cameron.gutman@reactos.org)
 *                  Hervé Poussineau
 */

#include <isapnp.h>

#define NDEBUG
#include <debug.h>

NTSTATUS
NTAPI
IsaFdoStartDevice(
    IN PISAPNP_FDO_EXTENSION FdoExt,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp)
{
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(IrpSp);

    FdoExt->Common.State = dsStarted;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IsaFdoQueryDeviceRelations(
    IN PISAPNP_FDO_EXTENSION FdoExt,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp)
{
    if (IrpSp->Parameters.QueryDeviceRelations.Type != BusRelations)
        return Irp->IoStatus.Status;

    return IsaPnpFillDeviceRelations(FdoExt, Irp, TRUE);
}

NTSTATUS
NTAPI
IsaFdoPnp(
    IN PISAPNP_FDO_EXTENSION FdoExt,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp)
{
    NTSTATUS Status = Irp->IoStatus.Status;

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
