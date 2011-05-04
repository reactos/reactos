/*
 * PROJECT:         ReactOS Universal Serial Bus Hub Driver
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/usb/usbhub/fdo.c
 * PURPOSE:         Misc helper functions
 * PROGRAMMERS:
 *                  Michael Martin (michael.martin@reactos.org)
 *                  Johannes Anderwald (johannes.anderwald@reactos.org)
 */
#define NDEBUG
#include "usbhub.h"
#include <stdarg.h>

VOID
DumpDeviceDescriptor(PUSB_DEVICE_DESCRIPTOR DeviceDescriptor)
{
    DPRINT1("Dumping Device Descriptor %x\n", DeviceDescriptor);
    DPRINT1("bLength %x\n", DeviceDescriptor->bLength);
    DPRINT1("bDescriptorType %x\n", DeviceDescriptor->bDescriptorType);
    DPRINT1("bcdUSB %x\n", DeviceDescriptor->bcdUSB);
    DPRINT1("bDeviceClass %x\n", DeviceDescriptor->bDeviceClass);
    DPRINT1("bDeviceSubClass %x\n", DeviceDescriptor->bDeviceSubClass);
    DPRINT1("bDeviceProtocol %x\n", DeviceDescriptor->bDeviceProtocol);
    DPRINT1("bMaxPacketSize0 %x\n", DeviceDescriptor->bMaxPacketSize0);
    DPRINT1("idVendor %x\n", DeviceDescriptor->idVendor);
    DPRINT1("idProduct %x\n", DeviceDescriptor->idProduct);
    DPRINT1("bcdDevice %x\n", DeviceDescriptor->bcdDevice);
    DPRINT1("iManufacturer %x\n", DeviceDescriptor->iManufacturer);
    DPRINT1("iProduct %x\n", DeviceDescriptor->iProduct);
    DPRINT1("iSerialNumber %x\n", DeviceDescriptor->iSerialNumber);
    DPRINT1("bNumConfigurations %x\n", DeviceDescriptor->bNumConfigurations);
}

//----------------------------------------------------------------------------------------
VOID
DumpConfigurationDescriptor(PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor)
{
    DPRINT1("Dumping ConfigurationDescriptor %x\n", ConfigurationDescriptor);
    DPRINT1("bLength %x\n", ConfigurationDescriptor->bLength);
    DPRINT1("bDescriptorType %x\n", ConfigurationDescriptor->bDescriptorType);
    DPRINT1("wTotalLength %x\n", ConfigurationDescriptor->wTotalLength);
    DPRINT1("bNumInterfaces %x\n", ConfigurationDescriptor->bNumInterfaces);
    DPRINT1("bConfigurationValue %x\n", ConfigurationDescriptor->bConfigurationValue);
    DPRINT1("iConfiguration %x\n", ConfigurationDescriptor->iConfiguration);
    DPRINT1("bmAttributes %x\n", ConfigurationDescriptor->bmAttributes);
    DPRINT1("MaxPower %x\n", ConfigurationDescriptor->MaxPower);
}

NTSTATUS
NTAPI
ForwardIrpAndWaitCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context)
{
    if (Irp->PendingReturned)
        KeSetEvent((PKEVENT)Context, IO_NO_INCREMENT, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
ForwardIrpAndWait(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    KEVENT Event;
    NTSTATUS Status;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    IoCopyCurrentIrpStackLocationToNext(Irp);

    IoSetCompletionRoutine(Irp, ForwardIrpAndWaitCompletion, &Event, TRUE, TRUE, TRUE);

    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        Status = KeWaitForSingleObject(&Event, Suspended, KernelMode, FALSE, NULL);
        if (NT_SUCCESS(Status))
            Status = Irp->IoStatus.Status;
    }

    return Status;
}

NTSTATUS
ForwardIrpAndForget(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PDEVICE_OBJECT LowerDevice = ((PHUB_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->LowerDeviceObject;

    ASSERT(LowerDevice);

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(LowerDevice, Irp);
}

