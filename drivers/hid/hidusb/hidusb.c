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
    PIO_STACK_LOCATION IoStack;
    PHID_USB_DEVICE_EXTENSION HidDeviceExtension;
    PHID_DEVICE_EXTENSION DeviceExtension;
    PHID_DEVICE_ATTRIBUTES Attributes;
    ULONG Length;
    NTSTATUS Status;

    //
    // get device extension
    //
    DeviceExtension = (PHID_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    HidDeviceExtension = (PHID_USB_DEVICE_EXTENSION)DeviceExtension->MiniDeviceExtension;

    //
    // get current stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    switch(IoStack->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_HID_GET_DEVICE_ATTRIBUTES:
        {
            if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(HID_DEVICE_ATTRIBUTES))
            {
                //
                // invalid request
                //
                Irp->IoStatus.Status = STATUS_INVALID_BUFFER_SIZE;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_INVALID_BUFFER_SIZE;
            }
            //
            // store result
            //
            ASSERT(HidDeviceExtension->HidDescriptor);
            Irp->IoStatus.Information = sizeof(HID_DESCRIPTOR);
            Attributes = (PHID_DEVICE_ATTRIBUTES)Irp->UserBuffer;
            Attributes->Size = sizeof(HID_DEVICE_ATTRIBUTES);
            Attributes->VendorID = HidDeviceExtension->VendorID;
            Attributes->ProductID = HidDeviceExtension->ProductID;
            Attributes->VersionNumber = HidDeviceExtension->VersionNumber;

            //
            // complete request
            //
            Irp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_SUCCESS;
        }
        case IOCTL_HID_GET_DEVICE_DESCRIPTOR:
        {
            //
            // sanity check
            //
            ASSERT(HidDeviceExtension->HidDescriptor);

            //
            // store length
            //
            Length = min(HidDeviceExtension->HidDescriptor->bLength, IoStack->Parameters.DeviceIoControl.OutputBufferLength);

            //
            // copy descriptor
            //
            RtlCopyMemory(Irp->UserBuffer, HidDeviceExtension->HidDescriptor, Length);

            //
            // store result length
            //
            Irp->IoStatus.Information = HidDeviceExtension->HidDescriptor->bLength;
            Irp->IoStatus.Status = STATUS_SUCCESS;

            /* complete request */
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_SUCCESS;
        }
        case IOCTL_HID_GET_REPORT_DESCRIPTOR:
        case IOCTL_HID_READ_REPORT:
        case IOCTL_HID_WRITE_REPORT:
        case IOCTL_GET_PHYSICAL_DESCRIPTOR:
        case IOCTL_HID_SEND_IDLE_NOTIFICATION_REQUEST:
        case IOCTL_HID_GET_FEATURE:
        case IOCTL_HID_SET_FEATURE:
        case IOCTL_HID_SET_OUTPUT_REPORT:
        case IOCTL_HID_GET_INPUT_REPORT:
        case IOCTL_HID_GET_INDEXED_STRING:
        case IOCTL_HID_GET_MS_GENRE_DESCRIPTOR:
        {
            DPRINT1("UNIMPLEMENTED %x\n", IoStack->Parameters.DeviceIoControl.IoControlCode);
            Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
            ASSERT(FALSE);
        }
        default:
            Status = Irp->IoStatus.Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return Status;
    }
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
Hid_PnpStart(
    IN PDEVICE_OBJECT DeviceObject)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
Hid_PnpCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context)
{
    //
    // signal event
    //
    KeSetEvent((PRKEVENT)Context, 0, FALSE);

    //
    // done
    //
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
NTAPI
HidPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IoStack;
    PHID_USB_DEVICE_EXTENSION HidDeviceExtension;
    PHID_DEVICE_EXTENSION DeviceExtension;
    KEVENT Event;

    //
    // get device extension
    //
    DeviceExtension = (PHID_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    HidDeviceExtension = (PHID_USB_DEVICE_EXTENSION)DeviceExtension->MiniDeviceExtension;

    //
    // get current stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // handle requests based on request type
    //
    switch(IoStack->MinorFunction)
    {
        case IRP_MN_REMOVE_DEVICE:
        {
            //
            // pass request onto lower driver
            //
            IoSkipCurrentIrpStackLocation(Irp);
            Status = IoCallDriver(DeviceExtension->NextDeviceObject, Irp);
 
            //
            // free resources
            //
            if (HidDeviceExtension->HidDescriptor)
            {
                ExFreePool(HidDeviceExtension->HidDescriptor);
                HidDeviceExtension->HidDescriptor = NULL;
            }

            //
            // done
            //
            return Status;
        }
        case IRP_MN_QUERY_PNP_DEVICE_STATE:
        {
            //
            // device can not be disabled
            //
            Irp->IoStatus.Information |= PNP_DEVICE_NOT_DISABLEABLE;

            //
            // pass request to next request
            //
            IoSkipCurrentIrpStackLocation(Irp);
            Status = IoCallDriver(DeviceExtension->NextDeviceObject, Irp);

            //
            // done
            //
            return Status;
        }
        case IRP_MN_STOP_DEVICE:
        {
            //
            // FIXME: unconfigure the device
            //

            //
            // prepare irp
            //
            KeInitializeEvent(&Event, NotificationEvent, FALSE);
            IoCopyCurrentIrpStackLocationToNext(Irp);
            IoSetCompletionRoutine(Irp, Hid_PnpCompletion, (PVOID)&Event, TRUE, TRUE, TRUE);

            //
            // send irp and wait for completion
            //
            Status = IoCallDriver(DeviceExtension->NextDeviceObject, Irp);
            if (Status == STATUS_PENDING)
            {
                KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
                Status = Irp->IoStatus.Status;
            }

            //
            // free resources
            //
            if (HidDeviceExtension->HidDescriptor)
            {
                ExFreePool(HidDeviceExtension->HidDescriptor);
                HidDeviceExtension->HidDescriptor = NULL;
            }

            //
            // done
            //
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return Status;
        }
        case IRP_MN_QUERY_CAPABILITIES:
        {
            //
            // prepare irp
            //
            KeInitializeEvent(&Event, NotificationEvent, FALSE);
            IoCopyCurrentIrpStackLocationToNext(Irp);
            IoSetCompletionRoutine(Irp, Hid_PnpCompletion, (PVOID)&Event, TRUE, TRUE, TRUE);

            //
            // send irp and wait for completion
            //
            Status = IoCallDriver(DeviceExtension->NextDeviceObject, Irp);
            if (Status == STATUS_PENDING)
            {
                KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
                Status = Irp->IoStatus.Status;
            }

            if (NT_SUCCESS(Status))
            {
                //
                // driver supports D1 & D2
                //
                IoStack->Parameters.DeviceCapabilities.Capabilities->DeviceD1 = TRUE;
                IoStack->Parameters.DeviceCapabilities.Capabilities->DeviceD2 = TRUE;
            }

            //
            // done
            //
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return Status;
        }
        case IRP_MN_START_DEVICE:
        {
            //
            // prepare irp
            //
            KeInitializeEvent(&Event, NotificationEvent, FALSE);
            IoCopyCurrentIrpStackLocationToNext(Irp);
            IoSetCompletionRoutine(Irp, Hid_PnpCompletion, (PVOID)&Event, TRUE, TRUE, TRUE);

            //
            // send irp and wait for completion
            //
            Status = IoCallDriver(DeviceExtension->NextDeviceObject, Irp);
            if (Status == STATUS_PENDING)
            {
                KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
                Status = Irp->IoStatus.Status;
            }

            //
            // did the device successfully start
            //
            if (!NT_SUCCESS(Status))
            {
                //
                // failed
                //
                DPRINT1("HIDUSB: IRP_MN_START_DEVICE failed with %x\n", Status);
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return Status;
            }

            //
            // start device
            //
            Status = Hid_PnpStart(DeviceObject);

            //
            // complete request
            //
            Irp->IoStatus.Status = Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return Status;
        }
        default:
        {
             //
             // forward and forget request
             //
             IoSkipCurrentIrpStackLocation(Irp);
             return IoCallDriver(DeviceExtension->NextDeviceObject, Irp);
        }
    }
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
