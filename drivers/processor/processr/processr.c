/*
 * PROJECT:        ReactOS Generic CPU Driver
 * LICENSE:        GNU GPLv2 only as published by the Free Software Foundation
 * FILE:           drivers/processor/processr/processr.c
 * PURPOSE:        Main Driver Routines
 * PROGRAMMERS:    Eric Kohl <eric.kohl@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include "processr.h"

#define NDEBUG
#include <debug.h>


/* FUNCTIONS ******************************************************************/

static
VOID
NTAPI
ProcessorUnload(
    IN PDRIVER_OBJECT DriverObject)
{
    DPRINT("ProcessorUnload()\n");
}


static
NTSTATUS
NTAPI
ProcessorPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
//    PIO_STACK_LOCATION IrpSp;
//    NTSTATUS Status = Irp->IoStatus.Status;
    PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;

    DPRINT("ProcessorPower()\n");

//    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);
    return PoCallDriver(DeviceExtension->LowerDevice, Irp);
}


NTSTATUS
NTAPI
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath)
{
    DPRINT("Processr: DriverEntry()\n");

    DriverObject->MajorFunction[IRP_MJ_PNP] = ProcessorPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER] = ProcessorPower;

    DriverObject->DriverExtension->AddDevice = ProcessorAddDevice;
    DriverObject->DriverUnload = ProcessorUnload;

    return STATUS_SUCCESS;
}
