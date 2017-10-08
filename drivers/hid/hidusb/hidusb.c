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

PUSBD_PIPE_INFORMATION
HidUsb_GetInputInterruptInterfaceHandle(
    PUSBD_INTERFACE_INFORMATION InterfaceInformation)
{
    ULONG Index;

    //
    // sanity check
    //
    ASSERT(InterfaceInformation->NumberOfPipes);

    for (Index = 0; Index < InterfaceInformation->NumberOfPipes; Index++)
    {
        //DPRINT1("[HIDUSB] EndpointAddress %x PipeType %x PipeHandle %x\n", InterfaceInformation->Pipes[Index].EndpointAddress, InterfaceInformation->Pipes[Index].PipeType, InterfaceInformation->Pipes[Index].PipeHandle);
        if (InterfaceInformation->Pipes[Index].PipeType == UsbdPipeTypeInterrupt && (InterfaceInformation->Pipes[Index].EndpointAddress & USB_ENDPOINT_DIRECTION_MASK))
        {
            //
            // found handle
            //
            return &InterfaceInformation->Pipes[Index];
        }
    }

    //
    // not found
    //
    return NULL;
}

NTSTATUS
HidUsb_GetPortStatus(
    IN PDEVICE_OBJECT DeviceObject,
    IN PULONG PortStatus)
{
    PIRP Irp;
    KEVENT Event;
    IO_STATUS_BLOCK IoStatus;
    PHID_DEVICE_EXTENSION DeviceExtension;
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;

    //
    // get device extension
    //
    DeviceExtension = DeviceObject->DeviceExtension;

    //
    // init result
    //
    *PortStatus = 0;

    //
    // init event
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    //
    // build irp
    //
    Irp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_USB_GET_PORT_STATUS,
                                        DeviceExtension->NextDeviceObject,
                                        NULL,
                                        0,
                                        NULL,
                                        0,
                                        TRUE,
                                        &Event,
                                        &IoStatus);
    if (!Irp)
    {
        //
        // no memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // get stack location
    //
    IoStack = IoGetNextIrpStackLocation(Irp);

    //
    // store result buffer
    //
    IoStack->Parameters.Others.Argument1 = PortStatus;

    //
    // call driver
    //
    Status = IoCallDriver(DeviceExtension->NextDeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        //
        // wait for completion
        //
        KeWaitForSingleObject(&Event, Executive, KernelMode, 0, NULL);
        return IoStatus.Status;
    }

    //
    // done
    //
    return Status;
}

NTSTATUS
HidUsb_ResetInterruptPipe(
    IN PDEVICE_OBJECT DeviceObject)
{
    PHID_USB_DEVICE_EXTENSION HidDeviceExtension;
    PHID_DEVICE_EXTENSION DeviceExtension;
    PUSBD_PIPE_INFORMATION PipeInformation;
    PURB Urb;
    NTSTATUS Status;

    //
    // get device extension
    //
    DeviceExtension = DeviceObject->DeviceExtension;
    HidDeviceExtension = DeviceExtension->MiniDeviceExtension;

    //
    // get interrupt pipe handle
    //
    ASSERT(HidDeviceExtension->InterfaceInfo);
    PipeInformation = HidUsb_GetInputInterruptInterfaceHandle(HidDeviceExtension->InterfaceInfo);
    ASSERT(PipeInformation);
    ASSERT(PipeInformation->PipeHandle);

    //
    // allocate urb
    //
    Urb = ExAllocatePoolWithTag(NonPagedPool, sizeof(struct _URB_PIPE_REQUEST), HIDUSB_URB_TAG);
    if (!Urb)
    {
        //
        // no memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // init urb
    //
    RtlZeroMemory(Urb, sizeof(struct _URB_PIPE_REQUEST));
    Urb->UrbHeader.Function = URB_FUNCTION_SYNC_RESET_PIPE_AND_CLEAR_STALL;
    Urb->UrbHeader.Length = sizeof(struct _URB_PIPE_REQUEST);
    Urb->UrbPipeRequest.PipeHandle = PipeInformation->PipeHandle;

    //
    // dispatch request
    //
    Status = Hid_DispatchUrb(DeviceObject, Urb);

    //
    // free urb
    //
    ExFreePoolWithTag(Urb, HIDUSB_URB_TAG);

    //
    // done
    //
    return Status;
}

NTSTATUS
HidUsb_AbortPipe(
    IN PDEVICE_OBJECT DeviceObject)
{
    PHID_USB_DEVICE_EXTENSION HidDeviceExtension;
    PHID_DEVICE_EXTENSION DeviceExtension;
    PURB Urb;
    NTSTATUS Status;
    PUSBD_PIPE_INFORMATION PipeInformation;

    //
    // get device extension
    //
    DeviceExtension = DeviceObject->DeviceExtension;
    HidDeviceExtension = DeviceExtension->MiniDeviceExtension;

    //
    // allocate urb
    //
    Urb = ExAllocatePoolWithTag(NonPagedPool, sizeof(struct _URB_PIPE_REQUEST), HIDUSB_URB_TAG);
    if (!Urb)
    {
        //
        // no memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // get pipe information
    //
    PipeInformation = HidUsb_GetInputInterruptInterfaceHandle(HidDeviceExtension->InterfaceInfo);
    ASSERT(PipeInformation);
    ASSERT(PipeInformation->PipeHandle);

    //
    // init urb
    //
    RtlZeroMemory(Urb, sizeof(struct _URB_PIPE_REQUEST));
    Urb->UrbHeader.Function = URB_FUNCTION_ABORT_PIPE;
    Urb->UrbHeader.Length = sizeof(struct _URB_PIPE_REQUEST);
    Urb->UrbPipeRequest.PipeHandle = PipeInformation->PipeHandle;

    //
    // dispatch request
    //
    Status = Hid_DispatchUrb(DeviceObject, Urb);

    //
    // free urb
    //
    ExFreePoolWithTag(Urb, HIDUSB_URB_TAG);

    //
    // done
    //
    return Status;
}

NTSTATUS
HidUsb_ResetPort(
    IN PDEVICE_OBJECT DeviceObject)
{
    KEVENT Event;
    PIRP Irp;
    PHID_DEVICE_EXTENSION DeviceExtension;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    //
    // get device extension
    //
    DeviceExtension = DeviceObject->DeviceExtension;

    //
    // init event
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    //
    // build irp
    //
    Irp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_USB_RESET_PORT,
                                        DeviceExtension->NextDeviceObject,
                                        NULL,
                                        0,
                                        NULL,
                                        0,
                                        TRUE,
                                        &Event,
                                        &IoStatusBlock);
    if (!Irp)
    {
        //
        // no memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // send the irp
    //
    Status = IoCallDriver(DeviceExtension->NextDeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        //
        // wait for request completion
        //
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }

    //
    // done
    //
    return IoStatusBlock.Status;
}

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
    // informational debug print
    //
    DPRINT("HIDUSB Request: %x\n", IoStack->MajorFunction);

    //
    // complete request
    //
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    //
    // done
    //
    return STATUS_SUCCESS;
}

VOID
NTAPI
HidUsb_ResetWorkerRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Ctx)
{
    NTSTATUS Status;
    ULONG PortStatus;
    PHID_USB_RESET_CONTEXT ResetContext;
    PHID_DEVICE_EXTENSION DeviceExtension;

    DPRINT("[HIDUSB] ResetWorkerRoutine\n");

    //
    // get context
    //
    ResetContext = Ctx;

    //
    // get device extension
    //
    DeviceExtension = ResetContext->DeviceObject->DeviceExtension;

    //
    // get port status
    //
    Status = HidUsb_GetPortStatus(ResetContext->DeviceObject, &PortStatus);
    DPRINT("[HIDUSB] ResetWorkerRoutine GetPortStatus %x PortStatus %x\n", Status, PortStatus);
    if (NT_SUCCESS(Status))
    {
        if (!(PortStatus & USB_PORT_STATUS_ENABLE))
        {
            //
            // port is disabled
            //
            Status = HidUsb_ResetInterruptPipe(ResetContext->DeviceObject);
            DPRINT1("[HIDUSB] ResetWorkerRoutine ResetPipe %x\n", Status);
        }
        else
        {
            //
            // abort pipe
            //
            Status = HidUsb_AbortPipe(ResetContext->DeviceObject);
            DPRINT1("[HIDUSB] ResetWorkerRoutine AbortPipe %x\n", Status);
            if (NT_SUCCESS(Status))
            {
                //
                // reset port
                //
                Status = HidUsb_ResetPort(ResetContext->DeviceObject);
                DPRINT1("[HIDUSB] ResetPort %x\n", Status);
                if (Status == STATUS_DEVICE_DATA_ERROR)
                {
                    //
                    // invalidate device state
                    //
                    IoInvalidateDeviceState(DeviceExtension->PhysicalDeviceObject);
                }

                //
                // reset interrupt pipe
                //
                if (NT_SUCCESS(Status))
                {
                    //
                    // reset pipe
                    //
                    Status = HidUsb_ResetInterruptPipe(ResetContext->DeviceObject);
                    DPRINT1("[HIDUSB] ResetWorkerRoutine ResetPipe %x\n", Status);
                }
            }
        }
    }

    //
    // cleanup
    //
    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
    IoFreeWorkItem(ResetContext->WorkItem);
    IoCompleteRequest(ResetContext->Irp, IO_NO_INCREMENT);
    ExFreePoolWithTag(ResetContext, HIDUSB_TAG);
}


NTSTATUS
NTAPI
HidUsb_ReadReportCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context)
{
    PURB Urb;
    PHID_USB_RESET_CONTEXT ResetContext;

    //
    // get urb
    //
    Urb = Context;
    ASSERT(Urb);

    DPRINT("[HIDUSB] HidUsb_ReadReportCompletion %p Status %x Urb Status %x\n", Irp, Irp->IoStatus, Urb->UrbHeader.Status);

    if (Irp->PendingReturned)
    {
        //
        // mark irp pending
        //
        IoMarkIrpPending(Irp);
    }

    //
    // did the reading report succeed / cancelled
    //
    if (NT_SUCCESS(Irp->IoStatus.Status) || Irp->IoStatus.Status == STATUS_CANCELLED || Irp->IoStatus.Status == STATUS_DEVICE_NOT_CONNECTED)
    {
        //
        // store result length
        //
        Irp->IoStatus.Information = Urb->UrbBulkOrInterruptTransfer.TransferBufferLength;

        //
        // FIXME handle error
        //
        ASSERT(Urb->UrbHeader.Status == USBD_STATUS_SUCCESS);

        //
        // free the urb
        //
        ExFreePoolWithTag(Urb, HIDUSB_URB_TAG);

        //
        // finish completion
        //
        return STATUS_CONTINUE_COMPLETION;
    }

    //
    // allocate reset context
    //
    ResetContext = ExAllocatePoolWithTag(NonPagedPool, sizeof(HID_USB_RESET_CONTEXT), HIDUSB_TAG);
    if (ResetContext)
    {
        //
        // allocate work item
        //
        ResetContext->WorkItem = IoAllocateWorkItem(DeviceObject);
        if (ResetContext->WorkItem)
        {
            //
            // init reset context
            //
            ResetContext->Irp = Irp;
            ResetContext->DeviceObject = DeviceObject;

            //
            // queue the work item
            //
            IoQueueWorkItem(ResetContext->WorkItem, HidUsb_ResetWorkerRoutine, DelayedWorkQueue, ResetContext);

            //
            // free urb
            //
            ExFreePoolWithTag(Urb, HIDUSB_URB_TAG);

            //
            // defer completion
            //
            return STATUS_MORE_PROCESSING_REQUIRED;
        }
        //
        // free context
        //
        ExFreePoolWithTag(ResetContext, HIDUSB_TAG);
    }

    //
    // free urb
    //
    ExFreePoolWithTag(Urb, HIDUSB_URB_TAG);

    //
    // complete request
    //
    return STATUS_CONTINUE_COMPLETION;
}


NTSTATUS
NTAPI
HidUsb_ReadReport(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PHID_USB_DEVICE_EXTENSION HidDeviceExtension;
    PHID_DEVICE_EXTENSION DeviceExtension;
    PIO_STACK_LOCATION IoStack;
    PURB Urb;
    PUSBD_PIPE_INFORMATION PipeInformation;

    //
    // get device extension
    //
    DeviceExtension = DeviceObject->DeviceExtension;
    HidDeviceExtension = DeviceExtension->MiniDeviceExtension;

    //
    // get current stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // sanity checks
    //
    ASSERT(IoStack->Parameters.DeviceIoControl.OutputBufferLength);
    ASSERT(Irp->UserBuffer);
    ASSERT(HidDeviceExtension->InterfaceInfo);

    //
    // get interrupt input pipe
    //
    PipeInformation = HidUsb_GetInputInterruptInterfaceHandle(HidDeviceExtension->InterfaceInfo);
    ASSERT(PipeInformation);

    //
    // lets allocate urb
    //
    Urb = ExAllocatePoolWithTag(NonPagedPool, sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER), HIDUSB_URB_TAG);
    if (!Urb)
    {
        //
        // no memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // init urb
    //
    RtlZeroMemory(Urb, sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER));

    //
    // sanity check
    //
    ASSERT(Irp->UserBuffer);
    ASSERT(IoStack->Parameters.DeviceIoControl.OutputBufferLength);
    ASSERT(PipeInformation->PipeHandle);

    //
    // build the urb
    //
    UsbBuildInterruptOrBulkTransferRequest(Urb,
                                           sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER),
                                           PipeInformation->PipeHandle,
                                           Irp->UserBuffer,
                                           NULL,
                                           IoStack->Parameters.DeviceIoControl.OutputBufferLength,
                                           USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK,
                                           NULL);

    //
    // store configuration handle
    //
    Urb->UrbHeader.UsbdDeviceHandle = HidDeviceExtension->ConfigurationHandle;

    //
    // get next location to setup irp
    //
    IoStack = IoGetNextIrpStackLocation(Irp);

    //
    // init irp for lower driver
    //
    IoStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    IoStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;
    IoStack->Parameters.DeviceIoControl.InputBufferLength = 0;
    IoStack->Parameters.DeviceIoControl.OutputBufferLength = 0;
    IoStack->Parameters.DeviceIoControl.Type3InputBuffer = NULL;
    IoStack->Parameters.Others.Argument1 = Urb;


    //
    // set completion routine
    //
    IoSetCompletionRoutine(Irp, HidUsb_ReadReportCompletion, Urb, TRUE, TRUE, TRUE);

    //
    // call driver
    //
    return IoCallDriver(DeviceExtension->NextDeviceObject, Irp);
}


NTSTATUS
NTAPI
HidUsb_GetReportDescriptor(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PHID_USB_DEVICE_EXTENSION HidDeviceExtension;
    PHID_DEVICE_EXTENSION DeviceExtension;
    PVOID Report = NULL;
    ULONG BufferLength, Length;
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;

    //
    // get device extension
    //
    DeviceExtension = DeviceObject->DeviceExtension;
    HidDeviceExtension = DeviceExtension->MiniDeviceExtension;

    //
    // sanity checks
    //
    ASSERT(HidDeviceExtension);
    ASSERT(HidDeviceExtension->HidDescriptor);
    ASSERT(HidDeviceExtension->HidDescriptor->bNumDescriptors >= 1);
    ASSERT(HidDeviceExtension->HidDescriptor->DescriptorList[0].bReportType == HID_REPORT_DESCRIPTOR_TYPE);
    ASSERT(HidDeviceExtension->HidDescriptor->DescriptorList[0].wReportLength > 0);

    //
    // FIXME: support old hid version
    //
    BufferLength = HidDeviceExtension->HidDescriptor->DescriptorList[0].wReportLength;
    Status = Hid_GetDescriptor(DeviceObject,
                               URB_FUNCTION_GET_DESCRIPTOR_FROM_INTERFACE,
                               sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                               &Report,
                               &BufferLength,
                               HidDeviceExtension->HidDescriptor->DescriptorList[0].bReportType,
                               0,
                               HidDeviceExtension->InterfaceInfo->InterfaceNumber);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to get descriptor
        // try with old hid version
        //
        BufferLength = HidDeviceExtension->HidDescriptor->DescriptorList[0].wReportLength;
        Status = Hid_GetDescriptor(DeviceObject,
                                   URB_FUNCTION_GET_DESCRIPTOR_FROM_ENDPOINT,
                                   sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                   &Report,
                                   &BufferLength,
                                   HidDeviceExtension->HidDescriptor->DescriptorList[0].bReportType,
                                   0,
                                   0 /* FIXME*/);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("[HIDUSB] failed to get report descriptor with %x\n", Status);
            return Status;
        }
    }

    //
    // get current stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);
    DPRINT("[HIDUSB] GetReportDescriptor: Status %x ReportLength %lu OutputBufferLength %lu TransferredLength %lu\n", Status, HidDeviceExtension->HidDescriptor->DescriptorList[0].wReportLength, IoStack->Parameters.DeviceIoControl.OutputBufferLength, BufferLength);

    //
    // get length to copy
    //
    Length = min(IoStack->Parameters.DeviceIoControl.OutputBufferLength, BufferLength);
    ASSERT(Length);

    //
    // copy result
    //
    RtlCopyMemory(Irp->UserBuffer, Report, Length);

    //
    // store result length
    //
    Irp->IoStatus.Information = Length;

    //
    // free the report buffer
    //
    ExFreePoolWithTag(Report, HIDUSB_TAG);

    //
    // done
    //
    return Status;

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
    DeviceExtension = DeviceObject->DeviceExtension;
    HidDeviceExtension = DeviceExtension->MiniDeviceExtension;

    //
    // get current stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    switch (IoStack->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_HID_GET_DEVICE_ATTRIBUTES:
        {
            if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(HID_DEVICE_ATTRIBUTES))
            {
                //
                // invalid request
                //
                Irp->IoStatus.Status = STATUS_INVALID_BUFFER_SIZE;
                DPRINT1("[HIDUSB] IOCTL_HID_GET_DEVICE_ATTRIBUTES invalid buffer\n");
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_INVALID_BUFFER_SIZE;
            }
            //
            // store result
            //
            DPRINT("[HIDUSB] IOCTL_HID_GET_DEVICE_ATTRIBUTES\n");
            ASSERT(HidDeviceExtension->DeviceDescriptor);
            Irp->IoStatus.Information = sizeof(HID_DESCRIPTOR);
            Attributes = Irp->UserBuffer;
            Attributes->Size = sizeof(HID_DEVICE_ATTRIBUTES);
            Attributes->VendorID = HidDeviceExtension->DeviceDescriptor->idVendor;
            Attributes->ProductID = HidDeviceExtension->DeviceDescriptor->idProduct;
            Attributes->VersionNumber = HidDeviceExtension->DeviceDescriptor->bcdDevice;

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
            DPRINT("[HIDUSB] IOCTL_HID_GET_DEVICE_DESCRIPTOR DescriptorLength %lu OutputBufferLength %lu\n", HidDeviceExtension->HidDescriptor->bLength, IoStack->Parameters.DeviceIoControl.OutputBufferLength);

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
        {
            Status = HidUsb_GetReportDescriptor(DeviceObject, Irp);
            DPRINT("[HIDUSB] IOCTL_HID_GET_REPORT_DESCRIPTOR Status %x\n", Status);
            Irp->IoStatus.Status = Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return Status;
        }
        case IOCTL_HID_READ_REPORT:
        {
            DPRINT("[HIDUSB] IOCTL_HID_READ_REPORT\n");
            Status = HidUsb_ReadReport(DeviceObject, Irp);
            return Status;
        }
        case IOCTL_HID_WRITE_REPORT:
        {
            DPRINT1("[HIDUSB] IOCTL_HID_WRITE_REPORT not implemented \n");
            ASSERT(FALSE);
            Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_NOT_IMPLEMENTED;
        }
        case IOCTL_GET_PHYSICAL_DESCRIPTOR:
        {
            DPRINT1("[HIDUSB] IOCTL_GET_PHYSICAL_DESCRIPTOR not implemented \n");
            ASSERT(FALSE);
            Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_NOT_IMPLEMENTED;
        }
        case IOCTL_HID_SEND_IDLE_NOTIFICATION_REQUEST:
        {
            DPRINT1("[HIDUSB] IOCTL_HID_SEND_IDLE_NOTIFICATION_REQUEST not implemented \n");
            ASSERT(FALSE);
            Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_NOT_IMPLEMENTED;
        }
        case IOCTL_HID_GET_FEATURE:
        {
            DPRINT1("[HIDUSB] IOCTL_HID_GET_FEATURE not implemented \n");
            ASSERT(FALSE);
            Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_NOT_IMPLEMENTED;
        }
        case IOCTL_HID_SET_FEATURE:
        {
            DPRINT1("[HIDUSB] IOCTL_HID_SET_FEATURE not implemented \n");
            ASSERT(FALSE);
            Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_NOT_IMPLEMENTED;
        }
        case IOCTL_HID_SET_OUTPUT_REPORT:
        {
            DPRINT1("[HIDUSB] IOCTL_HID_SET_OUTPUT_REPORT not implemented \n");
            ASSERT(FALSE);
            Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_NOT_IMPLEMENTED;
        }
        case IOCTL_HID_GET_INPUT_REPORT:
        {
            DPRINT1("[HIDUSB] IOCTL_HID_GET_INPUT_REPORT not implemented \n");
            ASSERT(FALSE);
            Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_NOT_IMPLEMENTED;
        }
        case IOCTL_HID_GET_INDEXED_STRING:
        {
            DPRINT1("[HIDUSB] IOCTL_HID_GET_INDEXED_STRING not implemented \n");
            ASSERT(FALSE);
            Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_NOT_IMPLEMENTED;
        }
        case IOCTL_HID_GET_MS_GENRE_DESCRIPTOR:
        {
            DPRINT1("[HIDUSB] IOCTL_HID_GET_MS_GENRE_DESCRIPTOR not implemented \n");
            ASSERT(FALSE);
            Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_NOT_IMPLEMENTED;
        }
        default:
        {
            UNIMPLEMENTED;
            ASSERT(FALSE);
            Status = Irp->IoStatus.Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return Status;
        }
    }
}

NTSTATUS
NTAPI
HidPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PHID_DEVICE_EXTENSION DeviceExtension;

    DeviceExtension = DeviceObject->DeviceExtension;
    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);
    return PoCallDriver(DeviceExtension->NextDeviceObject, Irp);
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
    DeviceExtension = DeviceObject->DeviceExtension;

    //
    // skip stack location
    //
    IoSkipCurrentIrpStackLocation(Irp);

    //
    // submit request
    //
    return IoCallDriver(DeviceExtension->NextDeviceObject, Irp);
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
    KeSetEvent(Context, 0, FALSE);

    //
    // done
    //
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
Hid_DispatchUrb(
    IN PDEVICE_OBJECT DeviceObject,
    IN PURB Urb)
{
    PIRP Irp;
    KEVENT Event;
    PHID_DEVICE_EXTENSION DeviceExtension;
    IO_STATUS_BLOCK IoStatus;
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;

    //
    // init event
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    //
    // get device extension
    //
    DeviceExtension = DeviceObject->DeviceExtension;

    //
    // build irp
    //
    Irp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_USB_SUBMIT_URB,
                                        DeviceExtension->NextDeviceObject,
                                        NULL,
                                        0,
                                        NULL,
                                        0,
                                        TRUE,
                                        &Event,
                                        &IoStatus);
    if (!Irp)
    {
        //
        // no memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // get next stack location
    //
    IoStack = IoGetNextIrpStackLocation(Irp);

    //
    // store urb
    //
    IoStack->Parameters.Others.Argument1 = Urb;

    //
    // set completion routine
    //
    IoSetCompletionRoutine(Irp, Hid_PnpCompletion, &Event, TRUE, TRUE, TRUE);

    //
    // call driver
    //
    Status = IoCallDriver(DeviceExtension->NextDeviceObject, Irp);

    //
    // wait for the request to finish
    //
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
    }

    //
    // complete request
    //
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    if (Status == STATUS_PENDING)
    {
        //
        // get final status
        //
        Status = IoStatus.Status;
    }

    DPRINT("[HIDUSB] DispatchUrb %x\n", Status);


    //
    // done
    //
    return Status;
}

NTSTATUS
Hid_GetDescriptor(
    IN PDEVICE_OBJECT DeviceObject,
    IN USHORT UrbFunction,
    IN USHORT UrbLength,
    IN OUT PVOID *UrbBuffer,
    IN OUT PULONG UrbBufferLength,
    IN UCHAR DescriptorType,
    IN UCHAR Index,
    IN USHORT LanguageIndex)
{
    PURB Urb;
    NTSTATUS Status;
    UCHAR Allocated = FALSE;

    //
    // allocate urb
    //
    Urb = ExAllocatePoolWithTag(NonPagedPool, UrbLength, HIDUSB_URB_TAG);
    if (!Urb)
    {
        //
        // no memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // is there an urb buffer
    //
    if (!*UrbBuffer)
    {
        //
        // allocate buffer
        //
        *UrbBuffer = ExAllocatePoolWithTag(NonPagedPool, *UrbBufferLength, HIDUSB_TAG);
        if (!*UrbBuffer)
        {
            //
            // no memory
            //
            ExFreePoolWithTag(Urb, HIDUSB_URB_TAG);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // zero buffer
        //
        RtlZeroMemory(*UrbBuffer, *UrbBufferLength);
        Allocated = TRUE;
    }

    //
    // zero urb
    //
    RtlZeroMemory(Urb, UrbLength);

    //
    // build descriptor request
    //
    UsbBuildGetDescriptorRequest(Urb, UrbLength, DescriptorType, Index, LanguageIndex, *UrbBuffer, NULL, *UrbBufferLength, NULL);

    //
    // set urb function
    //
    Urb->UrbHeader.Function = UrbFunction;

    //
    // dispatch urb
    //
    Status = Hid_DispatchUrb(DeviceObject, Urb);

    //
    // did the request fail
    //
    if (!NT_SUCCESS(Status))
    {
        if (Allocated)
        {
            //
            // free allocated buffer
            //
            ExFreePoolWithTag(*UrbBuffer, HIDUSB_TAG);
            *UrbBuffer = NULL;
        }

        //
        // free urb
        //
        ExFreePoolWithTag(Urb, HIDUSB_URB_TAG);
        *UrbBufferLength = 0;
        return Status;
    }

    //
    // did urb request fail
    //
    if (!NT_SUCCESS(Urb->UrbHeader.Status))
    {
        if (Allocated)
        {
            //
            // free allocated buffer
            //
            ExFreePoolWithTag(*UrbBuffer, HIDUSB_TAG);
            *UrbBuffer = NULL;
        }

        //
        // free urb
        //
        ExFreePoolWithTag(Urb, HIDUSB_URB_TAG);
        *UrbBufferLength = 0;
        return STATUS_UNSUCCESSFUL;
    }

    //
    // store result length
    //
    *UrbBufferLength = Urb->UrbControlDescriptorRequest.TransferBufferLength;

    //
    // free urb
    //
    ExFreePoolWithTag(Urb, HIDUSB_URB_TAG);

    //
    // completed successfully
    //
    return STATUS_SUCCESS;
}

NTSTATUS
Hid_SelectConfiguration(
    IN PDEVICE_OBJECT DeviceObject)
{
    PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor;
    NTSTATUS Status;
    USBD_INTERFACE_LIST_ENTRY InterfaceList[2];
    PURB Urb;
    PHID_USB_DEVICE_EXTENSION HidDeviceExtension;
    PHID_DEVICE_EXTENSION DeviceExtension;

    //
    // get device extension
    //
    DeviceExtension = DeviceObject->DeviceExtension;
    HidDeviceExtension = DeviceExtension->MiniDeviceExtension;

    //
    // now parse the descriptors
    //
    InterfaceDescriptor = USBD_ParseConfigurationDescriptorEx(HidDeviceExtension->ConfigurationDescriptor,
                                                              HidDeviceExtension->ConfigurationDescriptor,
                                                             -1,
                                                             -1,
                                                              USB_DEVICE_CLASS_HUMAN_INTERFACE,
                                                             -1,
                                                             -1);
    if (!InterfaceDescriptor)
    {
        //
        // bogus configuration descriptor
        //
        return STATUS_INVALID_PARAMETER;
    }

    //
    // sanity check
    //
    ASSERT(InterfaceDescriptor);
    ASSERT(InterfaceDescriptor->bInterfaceClass == USB_DEVICE_CLASS_HUMAN_INTERFACE);
    ASSERT(InterfaceDescriptor->bDescriptorType == USB_INTERFACE_DESCRIPTOR_TYPE);
    ASSERT(InterfaceDescriptor->bLength == sizeof(USB_INTERFACE_DESCRIPTOR));

    //
    // setup interface list
    //
    RtlZeroMemory(InterfaceList, sizeof(InterfaceList));
    InterfaceList[0].InterfaceDescriptor = InterfaceDescriptor;

    //
    // build urb
    //
    Urb = USBD_CreateConfigurationRequestEx(HidDeviceExtension->ConfigurationDescriptor, InterfaceList);
    if (!Urb)
    {
        //
        // no memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // dispatch request
    //
    Status = Hid_DispatchUrb(DeviceObject, Urb);
    if (NT_SUCCESS(Status))
    {
        //
        // store configuration handle
        //
        HidDeviceExtension->ConfigurationHandle = Urb->UrbSelectConfiguration.ConfigurationHandle;

        //
        // copy interface info
        //
        HidDeviceExtension->InterfaceInfo = ExAllocatePoolWithTag(NonPagedPool, Urb->UrbSelectConfiguration.Interface.Length, HIDUSB_TAG);
        if (HidDeviceExtension->InterfaceInfo)
        {
            //
            // copy interface info
            //
            RtlCopyMemory(HidDeviceExtension->InterfaceInfo, &Urb->UrbSelectConfiguration.Interface, Urb->UrbSelectConfiguration.Interface.Length);
        }
    }

    //
    // free urb request
    //
    ExFreePoolWithTag(Urb, 0);

    //
    // done
    //
    return Status;
}

NTSTATUS
Hid_DisableConfiguration(
    IN PDEVICE_OBJECT DeviceObject)
{
    PHID_DEVICE_EXTENSION DeviceExtension;
    PHID_USB_DEVICE_EXTENSION HidDeviceExtension;
    NTSTATUS Status;
    PURB Urb;

    //
    // get device extension
    //
    DeviceExtension = DeviceObject->DeviceExtension;
    HidDeviceExtension = DeviceExtension->MiniDeviceExtension;

    //
    // build urb
    //
    Urb = ExAllocatePoolWithTag(NonPagedPool,
                                sizeof(struct _URB_SELECT_CONFIGURATION),
                                HIDUSB_URB_TAG);
    if (!Urb)
    {
        //
        // no memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // format urb
    //
    UsbBuildSelectConfigurationRequest(Urb,
                                       sizeof(struct _URB_SELECT_CONFIGURATION),
                                       NULL);

    //
    // dispatch request
    //
    Status = Hid_DispatchUrb(DeviceObject, Urb);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("[HIDUSB] Dispatching unconfigure URB failed with %lx\n", Status);
    }
    else if (!USBD_SUCCESS(Urb->UrbHeader.Status))
    {
        DPRINT("[HIDUSB] Unconfigure URB failed with %lx\n", Status);
    }

    //
    // free urb
    //
    ExFreePoolWithTag(Urb, HIDUSB_URB_TAG);

    //
    // free resources
    //
    HidDeviceExtension->ConfigurationHandle = NULL;

    if (HidDeviceExtension->InterfaceInfo)
    {
        ExFreePoolWithTag(HidDeviceExtension->InterfaceInfo, HIDUSB_TAG);
        HidDeviceExtension->InterfaceInfo = NULL;
    }

    if (HidDeviceExtension->ConfigurationDescriptor)
    {
        ExFreePoolWithTag(HidDeviceExtension->ConfigurationDescriptor, HIDUSB_TAG);
        HidDeviceExtension->ConfigurationDescriptor = NULL;
        HidDeviceExtension->HidDescriptor = NULL;
    }

    if (HidDeviceExtension->DeviceDescriptor)
    {
        ExFreePoolWithTag(HidDeviceExtension->DeviceDescriptor, HIDUSB_TAG);
        HidDeviceExtension->DeviceDescriptor = NULL;
    }

    //
    // done
    //
    return Status;
}

NTSTATUS
Hid_SetIdle(
    IN PDEVICE_OBJECT DeviceObject)
{
    PURB Urb;
    NTSTATUS Status;

    //
    // allocate urb
    //
    Urb = ExAllocatePoolWithTag(NonPagedPool, sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST), HIDUSB_URB_TAG);
    if (!Urb)
    {
        //
        // no memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // zero urb
    //
    RtlZeroMemory(Urb, sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST));

    //
    // format urb
    //
    UsbBuildVendorRequest(Urb,
                          URB_FUNCTION_CLASS_INTERFACE,
                          sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST),
                          0,
                          0,
                          USB_SET_IDLE_REQUEST, // HID_SET_IDLE
                          0,
                          0,
                          NULL,
                          NULL,
                          0,
                          NULL);

    //
    // dispatch urb
    //
    Status = Hid_DispatchUrb(DeviceObject, Urb);

    //
    // free urb
    //
    ExFreePoolWithTag(Urb, HIDUSB_URB_TAG);

    //
    // print status
    //
    DPRINT1("Status %x\n", Status);
    return Status;
}


VOID
Hid_GetProtocol(
    IN PDEVICE_OBJECT DeviceObject)
{
    PHID_USB_DEVICE_EXTENSION HidDeviceExtension;
    PHID_DEVICE_EXTENSION DeviceExtension;
    PURB Urb;
    UCHAR Protocol[1];

    //
    // get device extension
    //
    DeviceExtension = DeviceObject->DeviceExtension;
    HidDeviceExtension = DeviceExtension->MiniDeviceExtension;
    ASSERT(HidDeviceExtension->InterfaceInfo);

    if (HidDeviceExtension->InterfaceInfo->SubClass != 0x1)
    {
        //
        // device does not support the boot protocol
        //
        return;
    }

    //
    // allocate urb
    //
    Urb = ExAllocatePoolWithTag(NonPagedPool, sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST), HIDUSB_URB_TAG);
    if (!Urb)
    {
        //
        // no memory
        //
        return;
    }

    //
    // zero urb
    //
    RtlZeroMemory(Urb, sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST));

    //
    // format urb
    //
    UsbBuildVendorRequest(Urb,
                          URB_FUNCTION_CLASS_INTERFACE,
                          sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST),
                          USBD_TRANSFER_DIRECTION_IN,
                          0,
                          USB_GET_PROTOCOL_REQUEST,
                          0,
                          0,
                          Protocol,
                          NULL,
                          1,
                          NULL);
    Protocol[0] = 0xFF;

    //
    // dispatch urb
    //
    Hid_DispatchUrb(DeviceObject, Urb);

    //
    // free urb
    //
    ExFreePoolWithTag(Urb, HIDUSB_URB_TAG);

    //
    // boot protocol active 0x00 disabled 0x1
    //
    if (Protocol[0] != 0x1)
    {
        if (Protocol[0] == 0x00)
        {
            DPRINT1("[HIDUSB] Need to disable boot protocol!\n");
        }
        else
        {
            DPRINT1("[HIDUSB] Unexpected protocol value %x\n", Protocol[0] & 0xFF);
        }
    }
}

NTSTATUS
Hid_PnpStart(
    IN PDEVICE_OBJECT DeviceObject)
{
    PHID_USB_DEVICE_EXTENSION HidDeviceExtension;
    PHID_DEVICE_EXTENSION DeviceExtension;
    NTSTATUS Status;
    ULONG DescriptorLength;
    PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor;
    PHID_DESCRIPTOR HidDescriptor;

    //
    // get device extension
    //
    DeviceExtension = DeviceObject->DeviceExtension;
    HidDeviceExtension = DeviceExtension->MiniDeviceExtension;

    //
    // get device descriptor
    //
    DescriptorLength = sizeof(USB_DEVICE_DESCRIPTOR);
    Status = Hid_GetDescriptor(DeviceObject,
                               URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE,
                               sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                               (PVOID *)&HidDeviceExtension->DeviceDescriptor,
                               &DescriptorLength,
                               USB_DEVICE_DESCRIPTOR_TYPE,
                               0,
                               0);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to obtain device descriptor
        //
        DPRINT1("[HIDUSB] failed to get device descriptor %x\n", Status);
        return Status;
    }

    //
    // now get the configuration descriptor
    //
    DescriptorLength = sizeof(USB_CONFIGURATION_DESCRIPTOR);
    Status = Hid_GetDescriptor(DeviceObject,
                               URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE,
                               sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                               (PVOID *)&HidDeviceExtension->ConfigurationDescriptor,
                               &DescriptorLength,
                               USB_CONFIGURATION_DESCRIPTOR_TYPE,
                               0,
                               0);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to obtain device descriptor
        //
        DPRINT1("[HIDUSB] failed to get device descriptor %x\n", Status);
        return Status;
    }

    //
    // sanity check
    //
    ASSERT(DescriptorLength);
    ASSERT(HidDeviceExtension->ConfigurationDescriptor);
    ASSERT(HidDeviceExtension->ConfigurationDescriptor->bLength);

    //
    // store full length
    //
    DescriptorLength = HidDeviceExtension->ConfigurationDescriptor->wTotalLength;

    //
    // delete partial configuration descriptor
    //
    ExFreePoolWithTag(HidDeviceExtension->ConfigurationDescriptor, HIDUSB_TAG);
    HidDeviceExtension->ConfigurationDescriptor = NULL;

    //
    // get full configuration descriptor
    //
    Status = Hid_GetDescriptor(DeviceObject,
                               URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE,
                               sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                               (PVOID *)&HidDeviceExtension->ConfigurationDescriptor,
                               &DescriptorLength,
                               USB_CONFIGURATION_DESCRIPTOR_TYPE,
                               0,
                               0);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to obtain device descriptor
        //
        DPRINT1("[HIDUSB] failed to get device descriptor %x\n", Status);
        return Status;
    }

    //
    // now parse the descriptors
    //
    InterfaceDescriptor = USBD_ParseConfigurationDescriptorEx(HidDeviceExtension->ConfigurationDescriptor,
                                                              HidDeviceExtension->ConfigurationDescriptor,
                                                             -1,
                                                             -1,
                                                              USB_DEVICE_CLASS_HUMAN_INTERFACE,
                                                             -1,
                                                             -1);
    if (!InterfaceDescriptor)
    {
        //
        // no interface class
        //
        DPRINT1("[HIDUSB] HID Interface descriptor not found\n");
        return STATUS_UNSUCCESSFUL;
    }

    //
    // sanity check
    //
    ASSERT(InterfaceDescriptor->bInterfaceClass == USB_DEVICE_CLASS_HUMAN_INTERFACE);
    ASSERT(InterfaceDescriptor->bDescriptorType == USB_INTERFACE_DESCRIPTOR_TYPE);
    ASSERT(InterfaceDescriptor->bLength == sizeof(USB_INTERFACE_DESCRIPTOR));

    //
    // move to next descriptor
    //
    HidDescriptor = (PHID_DESCRIPTOR)((ULONG_PTR)InterfaceDescriptor + InterfaceDescriptor->bLength);
    ASSERT(HidDescriptor->bLength >= 2);

    //
    // check if this is the hid descriptor
    //
    if (HidDescriptor->bLength == sizeof(HID_DESCRIPTOR) && HidDescriptor->bDescriptorType == HID_HID_DESCRIPTOR_TYPE)
    {
        //
        // found
        //
        HidDeviceExtension->HidDescriptor = HidDescriptor;

        //
        // select configuration
        //
        Status = Hid_SelectConfiguration(DeviceObject);

        //
        // done
        //
        DPRINT("[HIDUSB] SelectConfiguration %x\n", Status);

        if (NT_SUCCESS(Status))
        {
            //
            // now set the device idle
            //
            Hid_SetIdle(DeviceObject);

            //
            // get protocol
            //
            Hid_GetProtocol(DeviceObject);
            return Status;
        }
    }
    else
    {
        //
        // FIXME parse hid descriptor
        // select configuration
        // set idle
        // and get protocol
        //
        UNIMPLEMENTED;
        ASSERT(FALSE);
    }
    return Status;
}


NTSTATUS
NTAPI
HidPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IoStack;
    PHID_DEVICE_EXTENSION DeviceExtension;
    KEVENT Event;

    //
    // get device extension
    //
    DeviceExtension = DeviceObject->DeviceExtension;

    //
    // get current stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);
    DPRINT("[HIDUSB] Pnp %x\n", IoStack->MinorFunction);

    //
    // handle requests based on request type
    //
    switch (IoStack->MinorFunction)
    {
        case IRP_MN_REMOVE_DEVICE:
        {
            //
            // unconfigure device
            // FIXME: Call this on IRP_MN_SURPRISE_REMOVAL, but don't send URBs
            // FIXME: Don't call this after we've already seen a surprise removal or stop
            //
            Hid_DisableConfiguration(DeviceObject);

            //
            // pass request onto lower driver
            //
            IoSkipCurrentIrpStackLocation(Irp);
            Status = IoCallDriver(DeviceExtension->NextDeviceObject, Irp);

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
        case IRP_MN_QUERY_STOP_DEVICE:
        case IRP_MN_QUERY_REMOVE_DEVICE:
        {
            //
            // we're fine with it
            //
            Irp->IoStatus.Status = STATUS_SUCCESS;

            //
            // pass request to next driver
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
            // unconfigure device
            //
            Hid_DisableConfiguration(DeviceObject);

            //
            // prepare irp
            //
            KeInitializeEvent(&Event, NotificationEvent, FALSE);
            IoCopyCurrentIrpStackLocationToNext(Irp);
            IoSetCompletionRoutine(Irp, Hid_PnpCompletion, &Event, TRUE, TRUE, TRUE);

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
            IoSetCompletionRoutine(Irp, Hid_PnpCompletion, &Event, TRUE, TRUE, TRUE);

            //
            // send irp and wait for completion
            //
            Status = IoCallDriver(DeviceExtension->NextDeviceObject, Irp);
            if (Status == STATUS_PENDING)
            {
                KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
                Status = Irp->IoStatus.Status;
            }

            if (NT_SUCCESS(Status) && IoStack->Parameters.DeviceCapabilities.Capabilities != NULL)
            {
                //
                // don't need to safely remove
                //
                IoStack->Parameters.DeviceCapabilities.Capabilities->SurpriseRemovalOK = TRUE;
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
            IoSetCompletionRoutine(Irp, Hid_PnpCompletion, &Event, TRUE, TRUE, TRUE);

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
            DPRINT("[HIDUSB] IRP_MN_START_DEVICE Status %x\n", Status);
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
HidAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject)
{
    PHID_USB_DEVICE_EXTENSION HidDeviceExtension;
    PHID_DEVICE_EXTENSION DeviceExtension;

    //
    // get device extension
    //
    DeviceExtension = DeviceObject->DeviceExtension;
    HidDeviceExtension = DeviceExtension->MiniDeviceExtension;

    //
    // init event
    //
    KeInitializeEvent(&HidDeviceExtension->Event, NotificationEvent, FALSE);

    //
    // done
    //
    return STATUS_SUCCESS;
}

VOID
NTAPI
Hid_Unload(
    IN PDRIVER_OBJECT DriverObject)
{
    UNIMPLEMENTED;
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
    DriverObject->DriverExtension->AddDevice = HidAddDevice;
    DriverObject->DriverUnload = Hid_Unload;

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
    DPRINT("********* HIDUSB *********\n");
    DPRINT("HIDUSB Registration Status %x\n", Status);

    return Status;
}
