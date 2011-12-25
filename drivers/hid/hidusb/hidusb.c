/*
 * PROJECT:     ReactOS Universal Serial Bus Human Interface Device Driver
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/hidusb/hidusb.c
 * PURPOSE:     HID USB Interface Driver
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "hidusb.h"

NTSTATUS
NTAPI
HidCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;

    //
    // get current irp stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // sanity check for hidclass driver
    //
    ASSERT(IoStack->MajorFunction == IRP_MJ_CREATE || IoStack->MajorFunction == IRP_MJ_CLOSE);

    //
    // complete request
    //
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    //
    // informal debug print
    //
    DPRINT1("HIDUSB Request: %x\n", IoStack->MajorFunction);

    //
    // done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
HidInternalDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    UNIMPLEMENTED
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
HidPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    UNIMPLEMENTED
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
HidSystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PHID_DEVICE_EXTENSION DeviceExtension;

    //
    // get hid device extension
    //
    DeviceExtension = (PHID_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // copy stack location
    //
    IoCopyCurrentIrpStackLocationToNext(Irp);

    //
    // submit request
    //
    return IoCallDriver(DeviceExtension->NextDeviceObject, Irp);
}

NTSTATUS
NTAPI
HidPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    UNIMPLEMENTED
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegPath)
{
    HID_MINIDRIVER_REGISTRATION Registration;
    NTSTATUS Status;

    //
    // initialize driver object
    //
    DriverObject->MajorFunction[IRP_MJ_CREATE] = HidCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = HidCreate;
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = HidInternalDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_POWER] = HidPower;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = HidSystemControl;
    DriverObject->MajorFunction[IRP_MJ_PNP] = HidPnp;

    //
    // prepare registration info
    //
    RtlZeroMemory(&Registration, sizeof(HID_MINIDRIVER_REGISTRATION));

    //
    // fill in registration info
    //
    Registration.Revision = HID_REVISION;
    Registration.DriverObject = DriverObject;
    Registration.RegistryPath = RegPath;
    Registration.DeviceExtensionSize = sizeof(HID_USB_DEVICE_EXTENSION);
    Registration.DevicesArePolled = FALSE;

    //
    // register driver
    //
    Status = HidRegisterMinidriver(&Registration);

    //
    // informal debug
    //
    DPRINT1("********* HIDUSB *********\n");
    DPRINT1("HIDUSB Registration Status %x\n", Status);

    return Status;
}
