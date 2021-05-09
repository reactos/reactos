/*
 * PROJECT:     ReactOS Storage Stack / SCSIPORT storage port library
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     PnP power handlers
 * COPYRIGHT:   Copyright 2016 Thomas Faber <thomas.faber@reactos.org>
 */

#include "scsiport.h"


NTSTATUS
NTAPI
ScsiPortDispatchPower(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp)
{
    PSCSI_PORT_COMMON_EXTENSION comExt = DeviceObject->DeviceExtension;

    if (comExt->IsFDO)
    {
        PoStartNextPowerIrp(Irp);
        IoSkipCurrentIrpStackLocation(Irp);
        return PoCallDriver(comExt->LowerDevice, Irp);
    }
    else
    {
        PIO_STACK_LOCATION ioStack = IoGetCurrentIrpStackLocation(Irp);
        switch (ioStack->MinorFunction)
        {
            case IRP_MN_SET_POWER:
            case IRP_MN_QUERY_POWER:
                Irp->IoStatus.Status = STATUS_SUCCESS;
                break;
        }

        NTSTATUS status = Irp->IoStatus.Status;
        PoStartNextPowerIrp(Irp);
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }
}
