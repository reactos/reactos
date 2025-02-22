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
NTSTATUS
NTAPI
FdcAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT Pdo)
{
    PFDO_DEVICE_EXTENSION DeviceExtension = NULL;
    PDEVICE_OBJECT Fdo = NULL;
    NTSTATUS Status;

    DPRINT("FdcAddDevice()\n");

    ASSERT(DriverObject);
    ASSERT(Pdo);

    /* Create functional device object */
    Status = IoCreateDevice(DriverObject,
                            sizeof(FDO_DEVICE_EXTENSION),
                            NULL,
                            FILE_DEVICE_CONTROLLER,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &Fdo);
    if (NT_SUCCESS(Status))
    {
        DeviceExtension = (PFDO_DEVICE_EXTENSION)Fdo->DeviceExtension;
        RtlZeroMemory(DeviceExtension, sizeof(FDO_DEVICE_EXTENSION));

        DeviceExtension->Common.IsFDO = TRUE;
        DeviceExtension->Common.DeviceObject = Fdo;

        DeviceExtension->Pdo = Pdo;

        Status = IoAttachDeviceToDeviceStackSafe(Fdo, Pdo, &DeviceExtension->LowerDevice);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("IoAttachDeviceToDeviceStackSafe() failed with status 0x%08lx\n", Status);
            IoDeleteDevice(Fdo);
            return Status;
        }


        Fdo->Flags |= DO_DIRECT_IO;
        Fdo->Flags |= DO_POWER_PAGABLE;

        Fdo->Flags &= ~DO_DEVICE_INITIALIZING;
    }

    return Status;
}


static
VOID
NTAPI
FdcDriverUnload(IN PDRIVER_OBJECT DriverObject)
{
    DPRINT("FdcDriverUnload()\n");
}


static
NTSTATUS
NTAPI
FdcCreate(IN PDEVICE_OBJECT DeviceObject,
          IN PIRP Irp)
{
    DPRINT("FdcCreate()\n");

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
    DPRINT("FdcClose()\n");

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

    DPRINT("FdcPnP()\n");
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
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status = Irp->IoStatus.Status;
    PFDO_DEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;

    DPRINT("FdcPower()\n");

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    if (DeviceExtension->Common.IsFDO)
    {
        PoStartNextPowerIrp(Irp);
        IoSkipCurrentIrpStackLocation(Irp);
        return PoCallDriver(DeviceExtension->LowerDevice, Irp);
    }
    else
    {
        switch (IrpSp->MinorFunction)
        {
            case IRP_MN_QUERY_POWER:
            case IRP_MN_SET_POWER:
                Status = STATUS_SUCCESS;
                break;
        }
        PoStartNextPowerIrp(Irp);
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }
}


NTSTATUS
NTAPI
DriverEntry(IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegistryPath)
{
    DPRINT("FDC: DriverEntry()\n");

    DriverObject->MajorFunction[IRP_MJ_CREATE] = FdcCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = FdcClose;
//    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = FdcDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_PNP] = FdcPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER] = FdcPower;

    DriverObject->DriverExtension->AddDevice = FdcAddDevice;
    DriverObject->DriverUnload = FdcDriverUnload;

    return STATUS_SUCCESS;
}
