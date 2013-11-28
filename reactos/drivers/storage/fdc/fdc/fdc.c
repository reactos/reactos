/*
 * PROJECT:        ReactOS Floppy Disk Controller Driver
 * LICENSE:        GNU GPLv2 only as published by the Free Software Foundation
 * FILE:           drivers/storage/fdc/fdc/fdc.c
 * PURPOSE:        Main Driver Routines
 * PROGRAMMERS:    Eric Kohl
 */

/* INCLUDES *******************************************************************/

#include "fdc.h"

#define NDEBUG
#include <debug.h>



/* GLOBALS ********************************************************************/

ULONG ControllerCount = 0;

/* FUNCTIONS ******************************************************************/

static
VOID
NTAPI
FdcDriverUnload(IN PDRIVER_OBJECT DriverObject)
{
    DPRINT1("FdcDriverUnload()\n");
}


static
NTSTATUS
NTAPI
FdcCreate(IN PDEVICE_OBJECT DeviceObject,
          IN PIRP Irp)
{
    DPRINT1("FdcCreate()\n");

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = FILE_OPENED;

    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return STATUS_SUCCESS;
}


static
NTSTATUS
NTAPI
FdcClose(IN PDEVICE_OBJECT DeviceObject,
         IN PIRP Irp)
{
    DPRINT1("FdcClose()\n");

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}


static
NTSTATUS
NTAPI
FdcPnp(IN PDEVICE_OBJECT DeviceObject,
       IN PIRP Irp)
{
    PCOMMON_DEVICE_EXTENSION Common = DeviceObject->DeviceExtension;

    DPRINT1("FdcPnP()\n");
    if (Common->IsFDO)
    {
        return FdcFdoPnp(DeviceObject,
                         Irp);
    }
    else
    {
        return FdcPdoPnp(DeviceObject,
                         Irp);
    }
}


static
NTSTATUS
NTAPI
FdcPower(IN PDEVICE_OBJECT DeviceObject,
         IN PIRP Irp)
{
    DPRINT1("FdcPower()\n");
    return STATUS_UNSUCCESSFUL;
}


NTSTATUS
NTAPI
DriverEntry(IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegistryPath)
{
    DPRINT1("FDC: DriverEntry()\n");

    DriverObject->MajorFunction[IRP_MJ_CREATE] = FdcCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = FdcClose;
//    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = FdcDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_PNP] = FdcPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER] = FdcPower;

    DriverObject->DriverExtension->AddDevice = FdcAddDevice;
    DriverObject->DriverUnload = FdcDriverUnload;

    return STATUS_SUCCESS;
}