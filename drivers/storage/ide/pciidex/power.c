/*
 * PROJECT:     PCI IDE bus driver extension
 * LICENSE:     See COPYING in the top level directory
 * PURPOSE:     Power support functions
 * COPYRIGHT:   Copyright 2023 Dmitry Borisov <di.sean@protonmail.com>
 */

#include "pciidex.h"

#define NDEBUG
#include <debug.h>

static
NTSTATUS
PciIdeXPdoDispatchPower(
    _In_ PPDO_DEVICE_EXTENSION PdoExtension,
    _In_ PIRP Irp)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IoStack;

    UNREFERENCED_PARAMETER(PdoExtension);

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    switch (IoStack->MinorFunction)
    {
        case IRP_MN_SET_POWER:
        case IRP_MN_QUERY_POWER:
            Status = STATUS_SUCCESS;
            Irp->IoStatus.Status = Status;
            break;

        default:
            Status = Irp->IoStatus.Status;
            break;
    }

    PoStartNextPowerIrp(Irp);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

static
NTSTATUS
PciIdeXFdoDispatchPower(
    _In_ PFDO_DEVICE_EXTENSION FdoExtension,
    _In_ PIRP Irp)
{
    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);
    return PoCallDriver(FdoExtension->Ldo, Irp);
}

NTSTATUS
NTAPI
PciIdeXDispatchPower(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp)
{
    PVOID DeviceExtension = DeviceObject->DeviceExtension;

    if (IS_FDO(DeviceExtension))
        return PciIdeXFdoDispatchPower(DeviceExtension, Irp);
    else
        return PciIdeXPdoDispatchPower(DeviceExtension, Irp);
}
