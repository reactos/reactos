/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Enhanced Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbehci/usbehci.c
 * PURPOSE:     USB EHCI device driver.
 * PROGRAMMERS:
 *              Michael Martin
 */

/* DEFINES *******************************************************************/
#include "usbehci.h"
#define NDEBUG

/* INCLUDES *******************************************************************/
#include <debug.h>


static NTSTATUS NTAPI
IrpStub(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    NTSTATUS Status;

    if (((PCOMMON_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->IsFdo)
    {
        DPRINT1("ehci: FDO stub for major function 0x%lx\n",
            IoGetCurrentIrpStackLocation(Irp)->MajorFunction);
        return ForwardIrpAndForget(DeviceObject, Irp);
    }

    /* We are lower driver, So complete */
    DPRINT1("ehci: PDO stub for major function 0x%lx\n",
    IoGetCurrentIrpStackLocation(Irp)->MajorFunction);

    Status = Irp->IoStatus.Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

NTSTATUS NTAPI
DispatchDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    DPRINT("DispatchDeviceControl\n");
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS NTAPI
DispatchInternalDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    DPRINT("DispatchInternalDeviceControl\n");
    if (((PCOMMON_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->IsFdo)
        return IrpStub(DeviceObject, Irp);
    else
        return PdoDispatchInternalDeviceControl(DeviceObject, Irp);
}

NTSTATUS NTAPI
UsbEhciCleanup(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    DPRINT1("UsbEhciCleanup\n");
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
UsbEhciCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    DPRINT1("UsbEhciCreate\n");
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
UsbEhciClose(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    DPRINT1("Close\n");
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

VOID NTAPI
DriverUnload(PDRIVER_OBJECT DriverObject)
{
    DPRINT1("Unloading Driver\n");
}

NTSTATUS NTAPI
DispatchPnp(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    if (((PCOMMON_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->IsFdo)
        return FdoDispatchPnp(DeviceObject, Irp);
    else
        return PdoDispatchPnp(DeviceObject, Irp);
}

NTSTATUS NTAPI
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    DPRINT1("Driver Entry %wZ!\n", RegistryPath);

    DriverObject->DriverExtension->AddDevice = AddDevice;

    DriverObject->MajorFunction[IRP_MJ_CREATE] = UsbEhciCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = UsbEhciClose;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = UsbEhciCleanup;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = DispatchInternalDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_PNP] = DispatchPnp;

    DriverObject->DriverUnload = DriverUnload;
    DPRINT1("Driver entry done\n");

    return STATUS_SUCCESS;
}

