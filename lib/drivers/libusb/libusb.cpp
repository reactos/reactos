/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Driver Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/drivers/libusb/libusb.cpp
 * PURPOSE:     USB Common Driver Library.
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "libusb.h"

#define NDEBUG
#include <debug.h>

//
// driver verifier
//
DRIVER_ADD_DEVICE USBLIB_AddDevice;

extern
"C"
{
NTSTATUS
NTAPI
USBLIB_AddDevice(
    PDRIVER_OBJECT DriverObject,
    PDEVICE_OBJECT PhysicalDeviceObject)
{
    NTSTATUS Status;
    PHCDCONTROLLER HcdController;

    DPRINT("USBLIB_AddDevice\n");

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
}

extern
"C"
{
NTSTATUS
NTAPI
USBLIB_Dispatch(
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
            // dispatch power
            //
            return DeviceExtension->Dispatcher->HandlePower(DeviceObject, Irp);
        }
        case IRP_MJ_INTERNAL_DEVICE_CONTROL:
        case IRP_MJ_DEVICE_CONTROL:
        {
            //
            // dispatch io control
            //
            return DeviceExtension->Dispatcher->HandleDeviceControl(DeviceObject, Irp);
        }
        case IRP_MJ_SYSTEM_CONTROL:
        {
            //
            // dispatch system control
            //
            return DeviceExtension->Dispatcher->HandleSystemControl(DeviceObject, Irp);
        }
        default:
        {
            DPRINT1("USBLIB_Dispatch> Major %lu Minor %lu unhandeled\n", IoStack->MajorFunction, IoStack->MinorFunction);
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
}
