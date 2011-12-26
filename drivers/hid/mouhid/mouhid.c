/*
 * PROJECT:     ReactOS HID Stack
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/hid/mouhid/mouhid.c
 * PURPOSE:     Mouse HID Driver
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "mouhid.h"

NTSTATUS
NTAPI
MouHid_Create(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    UNIMPLEMENTED
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
NTAPI
MouHid_Close(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    UNIMPLEMENTED
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
MouHid_DeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PMOUSE_ATTRIBUTES Attributes;
    PMOUHID_DEVICE_EXTENSION DeviceExtension;
    PCONNECT_DATA Data;

    /* get current stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* get device extension */
    DeviceExtension = (PMOUHID_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    /* handle requests */
    if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_MOUSE_QUERY_ATTRIBUTES)
    {
         /* verify output buffer length */
         if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(MOUSE_ATTRIBUTES))
         {
             /* invalid request */
             Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
             IoCompleteRequest(Irp, IO_NO_INCREMENT);
             return STATUS_BUFFER_TOO_SMALL;
         }

         /* get output buffer */
         Attributes = (PMOUSE_ATTRIBUTES)Irp->AssociatedIrp.SystemBuffer;

         /* type of mouse */
         Attributes->MouseIdentifier = DeviceExtension->MouseIdentifier;

         /* number of buttons */
         Attributes->NumberOfButtons = DeviceExtension->Buttons;

         /* sample rate not used for usb */
         Attributes->SampleRate = 0;

         /* queue length */
         Attributes->InputDataQueueLength = 2;

         /* complete request */
         Irp->IoStatus.Information = sizeof(MOUSE_ATTRIBUTES);
         Irp->IoStatus.Status = STATUS_SUCCESS;
         IoCompleteRequest(Irp, IO_NO_INCREMENT);
         return STATUS_SUCCESS;
    }
    else if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_INTERNAL_MOUSE_CONNECT)
    {
         /* verify input buffer length */
         if (IoStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(CONNECT_DATA))
         {
             /* invalid request */
             Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
             IoCompleteRequest(Irp, IO_NO_INCREMENT);
             return STATUS_INVALID_PARAMETER;
         }

         /* is it already connected */
         if (DeviceExtension->ClassService)
         {
             /* already connected */
             Irp->IoStatus.Status = STATUS_SHARING_VIOLATION;
             IoCompleteRequest(Irp, IO_NO_INCREMENT);
             return STATUS_SHARING_VIOLATION;
         }

         /* get connect data */
         Data = (PCONNECT_DATA)IoStack->Parameters.DeviceIoControl.Type3InputBuffer;

         /* store connect details */
         DeviceExtension->ClassDeviceObject = Data->ClassDeviceObject;
         DeviceExtension->ClassService = Data->ClassService;

         /* completed successfully */
         Irp->IoStatus.Status = STATUS_SUCCESS;
         IoCompleteRequest(Irp, IO_NO_INCREMENT);
         return STATUS_SUCCESS;
    }
    else if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_INTERNAL_MOUSE_DISCONNECT)
    {
        /* not supported */
        Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_NOT_IMPLEMENTED;
    }
    else if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_INTERNAL_MOUSE_ENABLE)
    {
        /* not supported */
        Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_NOT_SUPPORTED;
    }
    else if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_INTERNAL_MOUSE_DISABLE)
    {
        /* not supported */
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    /* unknown request not supported */
    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
MouHid_InternalDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PMOUHID_DEVICE_EXTENSION DeviceExtension;

    /* get device extension */
    DeviceExtension = (PMOUHID_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    /* skip stack location */
    IoSkipCurrentIrpStackLocation(Irp);

    /* pass and forget */
    return IoCallDriver(DeviceExtension->NextDeviceObject, Irp);
}

NTSTATUS
NTAPI
MouHid_Power(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    UNIMPLEMENTED
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
MouHid_SubmitRequest(
    PDEVICE_OBJECT DeviceObject,
    ULONG IoControlCode,
    ULONG InputBufferSize,
    PVOID InputBuffer,
    ULONG OutputBufferSize,
    PVOID OutputBuffer)
{
    KEVENT Event;
    PMOUHID_DEVICE_EXTENSION DeviceExtension;
    PIRP Irp;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus;

    /* get device extension */
    DeviceExtension = (PMOUHID_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    /* init event */
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    /* build request */
    Irp = IoBuildDeviceIoControlRequest(IoControlCode, DeviceExtension->NextDeviceObject, InputBuffer, InputBufferSize, OutputBuffer, OutputBufferSize, FALSE, &Event, &IoStatus);
    if (!Irp)
    {
        /* no memory */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* send request */
    Status = IoCallDriver(DeviceExtension->NextDeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        /* wait for request to complete */
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatus.Status;
    }

    /* done */
    return Status;
}

NTSTATUS
NTAPI
MouHid_StartDevice(
    IN PDEVICE_OBJECT DeviceObject)
{
    NTSTATUS Status;
    ULONG Buttons;
    HID_COLLECTION_INFORMATION Information;
    PVOID PreparsedData;
    HIDP_CAPS Capabilities;
    ULONG ValueCapsLength;
    HIDP_VALUE_CAPS ValueCaps;
    PMOUHID_DEVICE_EXTENSION DeviceExtension;

    /* get device extension */
    DeviceExtension = (PMOUHID_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    /* query collection information */
    Status = MouHid_SubmitRequest(DeviceObject, IOCTL_HID_GET_COLLECTION_INFORMATION, 0, NULL, sizeof(HID_COLLECTION_INFORMATION), &Information);
    if (!NT_SUCCESS(Status))
    {
        /* failed to query collection information */
        return Status;
    }

    /* lets allocate space for preparsed data */
    PreparsedData = ExAllocatePool(NonPagedPool, Information.DescriptorSize);
    if (PreparsedData)
    {
        /* no memory */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* now obtain the preparsed data */
    Status = MouHid_SubmitRequest(DeviceObject, IOCTL_HID_GET_DRIVER_CONFIG, 0, NULL, Information.DescriptorSize, PreparsedData);
    if (!NT_SUCCESS(Status))
    {
        /* failed to get preparsed data */
        ExFreePool(PreparsedData);
        return Status;
    }

    /* lets get the caps */
    Status = HidP_GetCaps(PreparsedData, &Capabilities);
    if (!NT_SUCCESS(Status))
    {
        /* failed to get capabilities */
        ExFreePool(PreparsedData);
        return Status;
    }

    /* verify capabilities */
    if (Capabilities.Usage != HID_USAGE_GENERIC_POINTER && Capabilities.Usage != HID_USAGE_GENERIC_MOUSE || Capabilities.UsagePage != HID_USAGE_PAGE_GENERIC)
    {
        /* not supported */
        ExFreePool(PreparsedData);
        return STATUS_UNSUCCESSFUL;
    }

    /* get number of buttons */
    Buttons = HidP_MaxUsageListLength(HidP_Input, HID_USAGE_PAGE_BUTTON, PreparsedData);
    ASSERT(Buttons > 0);
    /* store number of buttons */
    DeviceExtension->Buttons = (USHORT)Buttons;

    ValueCapsLength = 1;
    HidP_GetSpecificValueCaps(HidP_Input, HID_USAGE_PAGE_GENERIC, HIDP_LINK_COLLECTION_UNSPECIFIED, HID_USAGE_GENERIC_X, &ValueCaps, &ValueCapsLength, PreparsedData);

    ValueCapsLength = 1;
    HidP_GetSpecificValueCaps(HidP_Input, HID_USAGE_PAGE_GENERIC, HIDP_LINK_COLLECTION_UNSPECIFIED, HID_USAGE_GENERIC_Y, &ValueCaps, &ValueCapsLength, PreparsedData);

    /* now check for wheel mouse support */
    ValueCapsLength = 1;
    Status = HidP_GetSpecificValueCaps(HidP_Input, HID_USAGE_PAGE_GENERIC, HIDP_LINK_COLLECTION_UNSPECIFIED, HID_USAGE_GENERIC_WHEEL, &ValueCaps, &ValueCapsLength, PreparsedData);
    if (Status == HIDP_STATUS_SUCCESS )
    {
        /* mouse has wheel support */
        DeviceExtension->MouseIdentifier = WHEELMOUSE_HID_HARDWARE;
        DeviceExtension->WheelUsagePage = ValueCaps.UsagePage;
    }
    else
    {
        /* check if the mouse has z-axis */
        ValueCapsLength = 1;
        Status = HidP_GetSpecificValueCaps(HidP_Input, HID_USAGE_PAGE_GENERIC, HIDP_LINK_COLLECTION_UNSPECIFIED, HID_USAGE_GENERIC_Z, &ValueCaps, &ValueCapsLength, PreparsedData);
        if (Status == HIDP_STATUS_SUCCESS && ValueCapsLength == 1)
        {
            /* wheel support */
            DeviceExtension->MouseIdentifier = WHEELMOUSE_HID_HARDWARE;
            DeviceExtension->WheelUsagePage = ValueCaps.UsagePage;
        }
    }

    /* completed successfully */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
MouHid_StartDeviceCompletion(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp,
    IN PVOID  Context)
{
    KeSetEvent((PKEVENT)Context, 0, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
NTAPI
MouHid_Pnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    KEVENT Event;
    NTSTATUS Status;
    PMOUHID_DEVICE_EXTENSION DeviceExtension;

    /* get device extension */
    DeviceExtension = (PMOUHID_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    /* get current irp stack */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    if (IoStack->MinorFunction == IRP_MN_STOP_DEVICE || IoStack->MinorFunction == IRP_MN_CANCEL_REMOVE_DEVICE || IoStack->MinorFunction == IRP_MN_QUERY_STOP_DEVICE || IoStack->MinorFunction == IRP_MN_CANCEL_STOP_DEVICE)
    {
        /* indicate success */
        Irp->IoStatus.Status = STATUS_SUCCESS;

        /* skip irp stack location */
        IoSkipCurrentIrpStackLocation(Irp);

        /* dispatch to lower device */
        return IoCallDriver(DeviceExtension->NextDeviceObject, Irp);
    }
    else if (IoStack->MinorFunction == IRP_MN_REMOVE_DEVICE)
    {
        /* FIXME synchronization */

        /* cancel irp */
        IoCancelIrp(DeviceExtension->Irp);

        /* indicate success */
        Irp->IoStatus.Status = STATUS_SUCCESS;

        /* skip irp stack location */
        IoSkipCurrentIrpStackLocation(Irp);

        /* dispatch to lower device */
        Status = IoCallDriver(DeviceExtension->NextDeviceObject, Irp);

        IoFreeIrp(DeviceExtension->Irp);
        IoDetachDevice(DeviceExtension->NextDeviceObject);
        IoDeleteDevice(DeviceObject);
        return Status;
    }
    else if (IoStack->MinorFunction == IRP_MN_START_DEVICE)
    {
        /* init event */
        KeInitializeEvent(&Event, NotificationEvent, FALSE);

        /* copy stack location */
        IoCopyCurrentIrpStackLocationToNext (Irp);

        /* set completion routine */
        IoSetCompletionRoutine(Irp, MouHid_StartDeviceCompletion, &Event, TRUE, TRUE, TRUE);
        Irp->IoStatus.Status = 0;

        /* pass request */
        Status = IoCallDriver(DeviceExtension->NextDeviceObject, Irp);
        if (Status == STATUS_PENDING)
        {
            KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
            Status = Irp->IoStatus.Status;
        }

        if (!NT_SUCCESS(Status))
        {
            /* failed */
            Irp->IoStatus.Status = Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return Status;
        }

        /* lets start the device */
        Status = MouHid_StartDevice(DeviceObject);
        DPRINT1("MouHid_StartDevice %x\n", Status);

        /* complete request */
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        /* done */
        return Status;
    }
    else
    {
        /* skip irp stack location */
        IoSkipCurrentIrpStackLocation(Irp);

        /* dispatch to lower device */
        return IoCallDriver(DeviceExtension->NextDeviceObject, Irp);
    }
}

NTSTATUS
NTAPI
MouHid_AddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject)
{
    NTSTATUS Status;
    PDEVICE_OBJECT DeviceObject, NextDeviceObject;
    PMOUHID_DEVICE_EXTENSION DeviceExtension;
    POWER_STATE State;

    /* create device object */
    Status = IoCreateDevice(DriverObject, sizeof(MOUHID_DEVICE_EXTENSION), NULL, FILE_DEVICE_MOUSE, 0, FALSE, &DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        /* failed to create device object */
        return Status;
    }

    /* now attach it */
    NextDeviceObject = IoAttachDeviceToDeviceStack(DeviceObject, PhysicalDeviceObject);
    if (!NextDeviceObject)
    {
        /* failed to attach */
        IoDeleteDevice(DeviceObject);
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    /* get device extension */
    DeviceExtension = (PMOUHID_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    /* zero extension */
    RtlZeroMemory(DeviceExtension, sizeof(MOUHID_DEVICE_EXTENSION));

    /* init device extension */
    DeviceExtension->MouseIdentifier = MOUSE_HID_HARDWARE;
    DeviceExtension->Buttons = 0;
    DeviceExtension->WheelUsagePage = 0;
    DeviceExtension->NextDeviceObject = NextDeviceObject;
    KeInitializeEvent(&DeviceExtension->Event, NotificationEvent, FALSE);
    DeviceExtension->Irp = IoAllocateIrp(NextDeviceObject->StackSize, FALSE);

    /* FIXME handle allocation error */
    ASSERT(DeviceExtension->Irp);

    /* FIXME query parameter 'FlipFlopWheel', 'WheelScalingFactor' */

    /* set power state to D0 */
    State.DeviceState =  PowerDeviceD0;
    PoSetPowerState(DeviceObject, DevicePowerState, State);

    /* init device object */
    DeviceObject->Flags |= DO_BUFFERED_IO | DO_POWER_PAGABLE;
    DeviceObject->Flags  &= ~DO_DEVICE_INITIALIZING;

    /* completed successfully */
    return STATUS_SUCCESS;
}

VOID
NTAPI
MouHid_Unload(
    IN PDRIVER_OBJECT DriverObject)
{
    UNIMPLEMENTED
    ASSERT(FALSE);
}


NTSTATUS
NTAPI
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegPath)
{
    /* FIXME check for parameters 'UseOnlyMice', 'TreatAbsoluteAsRelative', 'TreatAbsolutePointerAsAbsolute' */

    /* initialize driver object */
    DriverObject->DriverUnload = MouHid_Unload;
    DriverObject->DriverExtension->AddDevice = MouHid_AddDevice;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = MouHid_Create;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = MouHid_Close;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = MouHid_DeviceControl;
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = MouHid_InternalDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_POWER] = MouHid_Power;
    DriverObject->MajorFunction[IRP_MJ_PNP] = MouHid_Pnp;
    DriverObject->DriverUnload = MouHid_Unload;
    DriverObject->DriverExtension->AddDevice = MouHid_AddDevice;

    /* done */
    return STATUS_SUCCESS;
}
