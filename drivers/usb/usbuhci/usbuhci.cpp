/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Enhanced Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbohci/usbohci.cpp
 * PURPOSE:     USB OHCI device driver.
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "usbuhci.h"

//
// driver verifier
//
DRIVER_ADD_DEVICE UHCI_AddDevice;

NTSTATUS
NTAPI
UHCI_AddDevice(
    PDRIVER_OBJECT DriverObject,
    PDEVICE_OBJECT PhysicalDeviceObject)
{
    NTSTATUS Status;
    PHCDCONTROLLER HcdController;

    DPRINT1("UHCI_AddDevice\n");

    /* first create the controller object */
    Status = CreateHCDController(&HcdController);
    if (!NT_SUCCESS(Status))
    {
        /* failed to create hcd */
        DPRINT1("AddDevice: Failed to create hcd with %x\n", Status);
        return Status;
    }

    /* initialize the hcd */
    Status = HcdController->Initialize(NULL, // FIXME
                                       DriverObject,
                                       PhysicalDeviceObject);

    /* check for success */
    if (!NT_SUCCESS(Status))
    {
        /* failed to initialize device */
        DPRINT1("AddDevice: failed to initialize\n");

        /* release object */
        HcdController->Release();
    }

    return Status;

}

NTSTATUS
NTAPI
UHCI_Dispatch(
    PDEVICE_OBJECT DeviceObject, 
    PIRP Irp)
{
    PCOMMON_DEVICE_EXTENSION DeviceExtension;
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;

    //
    // get common device extension
    //
    DeviceExtension = (PCOMMON_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // get current stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // sanity checks
    //
    PC_ASSERT(DeviceExtension->Dispatcher);

    switch(IoStack->MajorFunction)
    {
        case IRP_MJ_PNP:
        {
            //
            // dispatch pnp
            //
            return DeviceExtension->Dispatcher->HandlePnp(DeviceObject, Irp);
        }

        case IRP_MJ_POWER:
        {
            //
            // dispatch pnp
            //
            return DeviceExtension->Dispatcher->HandlePower(DeviceObject, Irp);
        }
        case IRP_MJ_INTERNAL_DEVICE_CONTROL:
        case IRP_MJ_DEVICE_CONTROL:
        {
            //
            // dispatch pnp
            //
            return DeviceExtension->Dispatcher->HandleDeviceControl(DeviceObject, Irp);
        }
        default:
        {
            DPRINT1("UHCI_Dispatch> Major %lu Minor %lu unhandeled\n", IoStack->MajorFunction, IoStack->MinorFunction);
            Status = STATUS_SUCCESS;
        }
    }

    //
    // complete request
    //
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

extern
"C"
NTSTATUS
NTAPI
DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath)
{
    DPRINT1("[UHCI] Driver Entry\n");
    /* initialize driver object*/
    DriverObject->DriverExtension->AddDevice = UHCI_AddDevice;

    DriverObject->MajorFunction[IRP_MJ_CREATE] = UHCI_Dispatch;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = UHCI_Dispatch;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = UHCI_Dispatch;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = UHCI_Dispatch;
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = UHCI_Dispatch;
    DriverObject->MajorFunction[IRP_MJ_PNP] = UHCI_Dispatch;

    return STATUS_SUCCESS;
}
