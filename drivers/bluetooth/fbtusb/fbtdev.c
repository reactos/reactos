// Copyright (c) 2004, Antony C. Roberts

// Use of this file is subject to the terms
// described in the LICENSE.TXT file that
// accompanies this file.
//
// Your use of this file indicates your
// acceptance of the terms described in
// LICENSE.TXT.
//
// http://www.freebt.net

#include "fbtusb.h"
#include "fbtpnp.h"
#include "fbtpwr.h"
#include "fbtdev.h"
#include "fbtwmi.h"
#include "fbtrwr.h"

#include "fbtusr.h"

// Dispatch routine for CreateHandle
NTSTATUS NTAPI FreeBT_DispatchCreate(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    //ULONG                       i;
    NTSTATUS                    ntStatus;
    PFILE_OBJECT                fileObject;
    PDEVICE_EXTENSION           deviceExtension;
    PIO_STACK_LOCATION          irpStack;
    //PFREEBT_PIPE_CONTEXT        pipeContext;
    PUSBD_INTERFACE_INFORMATION interface;

    PAGED_CODE();

    FreeBT_DbgPrint(3, ("FreeBT_DispatchCreate: Entered\n"));

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    fileObject = irpStack->FileObject;
    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    if (deviceExtension->DeviceState != Working)
    {
        ntStatus = STATUS_INVALID_DEVICE_STATE;
        goto FreeBT_DispatchCreate_Exit;

    }

    if (deviceExtension->UsbInterface)
    {
        interface = deviceExtension->UsbInterface;

    }

    else
    {
        FreeBT_DbgPrint(1, ("UsbInterface not found\n"));
        ntStatus = STATUS_INVALID_DEVICE_STATE;
        goto FreeBT_DispatchCreate_Exit;

    }

    if (fileObject)
    {
        fileObject->FsContext = NULL;
    }

    else
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        goto FreeBT_DispatchCreate_Exit;

    }

    if (deviceExtension->OpenHandleCount>0)
    {
        ntStatus = STATUS_ACCESS_VIOLATION;
        goto FreeBT_DispatchCreate_Exit;

    }

    // opening a device as opposed to pipe.
    ntStatus = STATUS_SUCCESS;

    InterlockedIncrement(&deviceExtension->OpenHandleCount);

    // the device is idle if it has no open handles or pending PnP Irps
    // since we just received an open handle request, cancel idle req.
    if (deviceExtension->SSEnable)
        CancelSelectSuspend(deviceExtension);

FreeBT_DispatchCreate_Exit:
    Irp->IoStatus.Status = ntStatus;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    FreeBT_DbgPrint(3, ("FreeBT_DispatchCreate: Leaving\n"));
    return ntStatus;

}

// Dispatch routine for CloseHandle
NTSTATUS NTAPI FreeBT_DispatchClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    NTSTATUS               ntStatus;
    PFILE_OBJECT           fileObject;
    PDEVICE_EXTENSION      deviceExtension;
    PIO_STACK_LOCATION     irpStack;
    //PFREEBT_PIPE_CONTEXT  pipeContext;
    //PUSBD_PIPE_INFORMATION pipeInformation;

    PAGED_CODE();

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    fileObject = irpStack->FileObject;
    //pipeContext = NULL;
    //pipeInformation = NULL;
    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    FreeBT_DbgPrint(3, ("FreeBT_DispatchClose: Entered\n"));

    ntStatus = STATUS_SUCCESS;
    Irp->IoStatus.Status = ntStatus;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    InterlockedDecrement(&deviceExtension->OpenHandleCount);

    FreeBT_DbgPrint(3, ("FreeBT_DispatchClose: Leaving\n"));

    return ntStatus;

}

// Called when a HCI Send on the control pipe completes
NTSTATUS NTAPI FreeBT_HCISendCompletion(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context)
{
    //ULONG               stageLength;
    NTSTATUS            ntStatus;

    FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_HCISendCompletion, status=0x%08X\n", Irp->IoStatus.Status));

    if (Irp->PendingReturned)
        IoMarkIrpPending(Irp);

    ExFreePool(Context);
    FreeBT_IoDecrement(DeviceObject->DeviceExtension);
    ntStatus = Irp->IoStatus.Status;
    Irp->IoStatus.Information = 0;

    return ntStatus;

}

// Called the DeviceIOControl handler to send an HCI command received from the user
// HCI Commands are sent on the (default) control pipe
NTSTATUS NTAPI FreeBT_SendHCICommand(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID IoBuffer, IN ULONG InputBufferLength)
{
    PDEVICE_EXTENSION   deviceExtension;
    //ULONG               urbFlags;
    //ULONG               stageLength;
    //PVOID               pBuffer;
    PURB                urb;
    NTSTATUS            ntStatus;
    PIO_STACK_LOCATION  nextStack;
    //PFBT_HCI_CMD_HEADER pHCICommand;
    //LARGE_INTEGER       delay;

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    if (!deviceExtension)
    {
        ntStatus=STATUS_INVALID_PARAMETER;
        FreeBT_DbgPrint(1, ("FBTUSB: FreeBT_SendHCICommand: Failed to get DeviceExtension\n"));
        Irp->IoStatus.Status = ntStatus;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return ntStatus;

    }

    // The user is doing a reset, reset all the pipes as well, so that any
    // old events or data are removed
    /*pHCICommand=(PFBT_HCI_CMD_HEADER)IoBuffer;
    if (pHCICommand->OpCode==FBT_HCI_CMD_RESET)
    {
        FreeBT_ResetPipe(DeviceObject, deviceExtension->EventPipe.PipeHandle);
        FreeBT_ResetPipe(DeviceObject, deviceExtension->DataInPipe.PipeHandle);
        FreeBT_ResetPipe(DeviceObject, deviceExtension->DataOutPipe.PipeHandle);
        FreeBT_ResetPipe(DeviceObject, deviceExtension->AudioInPipe.PipeHandle);
        FreeBT_ResetPipe(DeviceObject, deviceExtension->AudioOutPipe.PipeHandle);

        // Wait a second for the device to recover
        FreeBT_DbgPrint(1, ("FBTUSB: FreeBT_SendHCICommand: Sleeping\n"));
        delay.QuadPart = -10000 * 5000; // 5s
        KeWaitForSingleObject(&deviceExtension->DelayEvent,
                              Executive,
                              UserMode,
                              FALSE,
                              &delay);

        FreeBT_DbgPrint(1, ("FBTUSB: FreeBT_SendHCICommand: Finished sleeping\n"));


    }*/

    // Create the URB
    urb = (PURB)ExAllocatePool(NonPagedPool, sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST));
    if(urb == NULL)
    {
        FreeBT_DbgPrint(1, ("FBTUSB: FreeBT_SendHCICommand: Failed to alloc mem for urb\n"));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        Irp->IoStatus.Status = ntStatus;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return ntStatus;

    }

    UsbBuildVendorRequest(
        urb,
        URB_FUNCTION_CLASS_DEVICE, // This works, for CSR and Silicon Wave
        sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST),
        0,
        0,
        0,
        0,
        0,
        IoBuffer,
        NULL,
        InputBufferLength,
        NULL);

    // use the original irp as an internal device control irp
    nextStack = IoGetNextIrpStackLocation(Irp);
    nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    nextStack->Parameters.Others.Argument1 = (PVOID) urb;
    nextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;

    IoSetCompletionRoutine(
        Irp,
        (PIO_COMPLETION_ROUTINE)FreeBT_HCISendCompletion,
        urb,
        TRUE,
        TRUE,
        TRUE);

    // We return STATUS_PENDING; call IoMarkIrpPending.
    IoMarkIrpPending(Irp);

    FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_SendHCICommand::"));
    FreeBT_IoIncrement(deviceExtension);

    FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_SendHCICommand: Sending IRP %X to underlying driver\n", Irp));
    ntStatus=IoCallDriver(deviceExtension->TopOfStackDeviceObject, Irp);
    if(!NT_SUCCESS(ntStatus))
    {
        FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_SendHCICommand: IoCallDriver fails with status %X\n", ntStatus));

        FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_SendHCICommand::"));
        FreeBT_IoDecrement(deviceExtension);

        // If the device was surprise removed out, the pipeInformation field is invalid.
        // similarly if the request was cancelled, then we need not reset the device.
        if((ntStatus != STATUS_CANCELLED) && (ntStatus != STATUS_DEVICE_NOT_CONNECTED))
            ntStatus = FreeBT_ResetDevice(DeviceObject);

        else
            FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_SendHCICommand: ntStatus is STATUS_CANCELLED or STATUS_DEVICE_NOT_CONNECTED\n"));

        Irp->IoStatus.Status = ntStatus;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return ntStatus;

    }

    FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_SendHCICommand: Completed successfully\n"));

    return STATUS_PENDING;

}

// Called when a HCI Get on the event pipe completes
NTSTATUS NTAPI FreeBT_HCIEventCompletion(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context)
{
    //ULONG               stageLength;
    NTSTATUS            ntStatus;
    PIO_STACK_LOCATION  nextStack;
    PURB                urb;

    FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_HCIEventCompletion, status=0x%08X\n", Irp->IoStatus.Status));

    if (Irp->PendingReturned)
        IoMarkIrpPending(Irp);

    // initialize variables
    urb=(PURB)Context;
    ntStatus = Irp->IoStatus.Status;
    Irp->IoStatus.Information = urb->UrbBulkOrInterruptTransfer.TransferBufferLength;
    nextStack = IoGetNextIrpStackLocation(Irp);

    ExFreePool(Context);
    FreeBT_IoDecrement(DeviceObject->DeviceExtension);

    return ntStatus;

}

// Called from the DeviceIOControl handler to wait for an event on the interrupt pipe
NTSTATUS NTAPI FreeBT_GetHCIEvent(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID IoBuffer, IN ULONG InputBufferLength)
{
    PDEVICE_EXTENSION   deviceExtension;
    PURB                urb;
    NTSTATUS            ntStatus;
    PIO_STACK_LOCATION  nextStack;

    FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_GetHCIEvent: Entered\n"));

    urb = NULL;
    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    urb = (PURB)ExAllocatePool(NonPagedPool, sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER));
    if (urb==NULL)
    {
        FreeBT_DbgPrint(1, ("FBTUSB: FreeBT_GetHCIEvent: Failed to alloc mem for urb\n"));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto FreeBT_GetHCIEvent_Exit;

    }

    UsbBuildInterruptOrBulkTransferRequest(
                            urb,
                            sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER),
                            deviceExtension->EventPipe.PipeHandle,
                            IoBuffer,
                            NULL,
                            InputBufferLength,
                            USBD_SHORT_TRANSFER_OK|USBD_TRANSFER_DIRECTION_IN,
                            NULL);

    // use the original irp as an internal device control irp, which we send down to the
    // USB class driver in order to get our request out on the wire
    nextStack = IoGetNextIrpStackLocation(Irp);
    nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    nextStack->Parameters.Others.Argument1 = (PVOID) urb;
    nextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;

    IoSetCompletionRoutine(
        Irp,
        (PIO_COMPLETION_ROUTINE)FreeBT_HCIEventCompletion,
        urb,
        TRUE,
        TRUE,
        TRUE);

    // We return STATUS_PENDING; call IoMarkIrpPending.
    IoMarkIrpPending(Irp);

    FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_GetHCIEvent::"));
    FreeBT_IoIncrement(deviceExtension);

    ntStatus = IoCallDriver(deviceExtension->TopOfStackDeviceObject, Irp);
    if (!NT_SUCCESS(ntStatus))
    {
        FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_GetHCIEvent: IoCallDriver fails with status %X\n", ntStatus));

        FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_GetHCIEvent::"));
        FreeBT_IoDecrement(deviceExtension);

        // If the device was surprise removed out, the pipeInformation field is invalid.
        // similarly if the request was cancelled, then we need not reset the pipe.
        if((ntStatus != STATUS_CANCELLED) && (ntStatus != STATUS_DEVICE_NOT_CONNECTED))
        {
            ntStatus = FreeBT_ResetPipe(DeviceObject, deviceExtension->EventPipe.PipeHandle);
            if(!NT_SUCCESS(ntStatus))
            {
                FreeBT_DbgPrint(1, ("FreeBT_ResetPipe failed\n"));
                ntStatus = FreeBT_ResetDevice(DeviceObject);

            }

        }

        else
        {
            FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_GetHCIEvent: ntStatus is STATUS_CANCELLED or STATUS_DEVICE_NOT_CONNECTED\n"));

        }

        goto FreeBT_GetHCIEvent_Exit;

    }

    FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_GetHCIEvent: Leaving\n"));

    // Return STATUS_PENDING, when the lower driver completes the request,
    // the FreeBT_HCIEventCompletion completion routine.
    return STATUS_PENDING;

FreeBT_GetHCIEvent_Exit:
    Irp->IoStatus.Status=ntStatus;
    Irp->IoStatus.Information=0;

    FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_GetHCIEvent: Failure (0x%08x), completing IRP\n", ntStatus));
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return ntStatus;

}

// DeviceIOControl dispatch
NTSTATUS NTAPI FreeBT_DispatchDevCtrl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    ULONG              code;
    PVOID              ioBuffer;
    ULONG              inputBufferLength;
    ULONG              outputBufferLength;
    ULONG              info;
    NTSTATUS           ntStatus;
    PDEVICE_EXTENSION  deviceExtension;
    PIO_STACK_LOCATION irpStack;

    info = 0;
    irpStack = IoGetCurrentIrpStackLocation(Irp);
    code = irpStack->Parameters.DeviceIoControl.IoControlCode;
    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    ioBuffer           = Irp->AssociatedIrp.SystemBuffer;
    inputBufferLength  = irpStack->Parameters.DeviceIoControl.InputBufferLength;
    outputBufferLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    if (deviceExtension->DeviceState != Working)
    {
        FreeBT_DbgPrint(1, ("FBTUSB: Invalid device state\n"));
        ntStatus = STATUS_INVALID_DEVICE_STATE;
        goto FreeBT_DispatchDevCtrlExit;

    }

    FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_DispatchDevCtrl::"));

    // Make sure that any selective suspend request has been completed.
    if (deviceExtension->SSEnable)
    {
        FreeBT_DbgPrint(3, ("Waiting on the IdleReqPendEvent\n"));
        KeWaitForSingleObject(&deviceExtension->NoIdleReqPendEvent,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);

    }

    switch(code)
    {
    case IOCTL_FREEBT_HCI_SEND_CMD:
        FreeBT_DbgPrint(3, ("FBTUSB: IOCTL_FREEBT_HCI_SEND_CMD received\n"));
        if (inputBufferLength<FBT_HCI_CMD_MIN_SIZE)
        {
            ntStatus = STATUS_BUFFER_TOO_SMALL;
            FreeBT_DbgPrint(3, ("FBTUSB: IOCTL_FREEBT_HCI_SEND_CMD: Buffer too small\n"));
            goto FreeBT_DispatchDevCtrlExit;

        }

        if (inputBufferLength>FBT_HCI_CMD_MAX_SIZE)
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            FreeBT_DbgPrint(3, ("FBTUSB: IOCTL_FREEBT_HCI_SEND_CMD: Buffer too long\n"));
            goto FreeBT_DispatchDevCtrlExit;

        }

        return FreeBT_SendHCICommand(DeviceObject, Irp, ioBuffer, inputBufferLength);
        break;

    case IOCTL_FREEBT_HCI_GET_EVENT:
        FreeBT_DbgPrint(3, ("FBTUSB: IOCTL_FREEBT_HCI_GET_EVENT received\n"));
        if (outputBufferLength<FBT_HCI_EVENT_MAX_SIZE)
        {
            ntStatus = STATUS_BUFFER_TOO_SMALL;
            FreeBT_DbgPrint(3, ("FBTUSB: IOCTL_FREEBT_HCI_GET_EVENT: Buffer too small\n"));
            goto FreeBT_DispatchDevCtrlExit;

        }

        return FreeBT_GetHCIEvent(DeviceObject, Irp, ioBuffer, outputBufferLength);
        break;

    default:
        FreeBT_DbgPrint(3, ("FBTUSB: Invalid IOCTL 0x%08x received\n", code));
        ntStatus = STATUS_INVALID_DEVICE_REQUEST;
        break;

    }

FreeBT_DispatchDevCtrlExit:
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = ntStatus;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return ntStatus;
}

// Submit URB_FUNCTION_RESET_PIPE
NTSTATUS NTAPI FreeBT_ResetPipe(IN PDEVICE_OBJECT DeviceObject, IN USBD_PIPE_HANDLE PipeHandle)
{
    PURB              urb;
    NTSTATUS          ntStatus;
    PDEVICE_EXTENSION deviceExtension;

    urb = NULL;
    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    urb = (PURB)ExAllocatePool(NonPagedPool, sizeof(struct _URB_PIPE_REQUEST));
    if (urb)
    {
        urb->UrbHeader.Length = (USHORT) sizeof(struct _URB_PIPE_REQUEST);
        urb->UrbHeader.Function = URB_FUNCTION_RESET_PIPE;
        urb->UrbPipeRequest.PipeHandle = PipeHandle;

        ntStatus = CallUSBD(DeviceObject, urb);

        ExFreePool(urb);

    }

    else
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;

    if(NT_SUCCESS(ntStatus))
    {
        FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_ResetPipe - success\n"));
        ntStatus = STATUS_SUCCESS;

    }

    else
        FreeBT_DbgPrint(1, ("FBTUSB: FreeBT_ResetPipe - failed\n"));

    return ntStatus;

}

// Call FreeBT_ResetParentPort to reset the device
NTSTATUS NTAPI FreeBT_ResetDevice(IN PDEVICE_OBJECT DeviceObject)
{
    NTSTATUS ntStatus;
    ULONG    portStatus;

    FreeBT_DbgPrint(3, ("FreeBT_ResetDevice: Entered\n"));

    ntStatus = FreeBT_GetPortStatus(DeviceObject, &portStatus);

    if ( (NT_SUCCESS(ntStatus)) && (!(portStatus & USBD_PORT_ENABLED)) && (portStatus & USBD_PORT_CONNECTED))
        ntStatus=FreeBT_ResetParentPort(DeviceObject);

    FreeBT_DbgPrint(3, ("FreeBT_ResetDevice: Leaving\n"));

    return ntStatus;

}

// Read port status from the lower driver (USB class driver)
NTSTATUS NTAPI FreeBT_GetPortStatus(IN PDEVICE_OBJECT DeviceObject, IN OUT PULONG PortStatus)
{
    NTSTATUS           ntStatus;
    KEVENT             event;
    PIRP               irp;
    IO_STATUS_BLOCK    ioStatus;
    PIO_STACK_LOCATION nextStack;
    PDEVICE_EXTENSION  deviceExtension;

    FreeBT_DbgPrint(3, ("FreeBT_GetPortStatus: Entered\n"));

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    *PortStatus = 0;

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildDeviceIoControlRequest(
                    IOCTL_INTERNAL_USB_GET_PORT_STATUS,
                    deviceExtension->TopOfStackDeviceObject,
                    NULL,
                    0,
                    NULL,
                    0,
                    TRUE,
                    &event,
                    &ioStatus);

    if (NULL == irp)
    {
        FreeBT_DbgPrint(1, ("memory alloc for irp failed\n"));
        return STATUS_INSUFFICIENT_RESOURCES;

    }

    nextStack = IoGetNextIrpStackLocation(irp);
    ASSERT(nextStack != NULL);
    nextStack->Parameters.Others.Argument1 = PortStatus;

    ntStatus = IoCallDriver(deviceExtension->TopOfStackDeviceObject, irp);
    if (STATUS_PENDING==ntStatus)
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

    else
        ioStatus.Status = ntStatus;

    ntStatus = ioStatus.Status;
    FreeBT_DbgPrint(3, ("FreeBT_GetPortStatus: Leaving\n"));

    return ntStatus;

}

// Sends an IOCTL_INTERNAL_USB_RESET_PORT via the lower driver
NTSTATUS NTAPI FreeBT_ResetParentPort(IN PDEVICE_OBJECT DeviceObject)
{
    NTSTATUS           ntStatus;
    KEVENT             event;
    PIRP               irp;
    IO_STATUS_BLOCK    ioStatus;
    PIO_STACK_LOCATION nextStack;
    PDEVICE_EXTENSION  deviceExtension;

    FreeBT_DbgPrint(3, ("FreeBT_ResetParentPort: Entered\n"));

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildDeviceIoControlRequest(
                    IOCTL_INTERNAL_USB_RESET_PORT,
                    deviceExtension->TopOfStackDeviceObject,
                    NULL,
                    0,
                    NULL,
                    0,
                    TRUE,
                    &event,
                    &ioStatus);

    if (NULL == irp)
    {
        FreeBT_DbgPrint(1, ("memory alloc for irp failed\n"));
        return STATUS_INSUFFICIENT_RESOURCES;

    }

    nextStack = IoGetNextIrpStackLocation(irp);
    ASSERT(nextStack != NULL);

    ntStatus = IoCallDriver(deviceExtension->TopOfStackDeviceObject, irp);
    if(STATUS_PENDING == ntStatus)
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

    else
        ioStatus.Status = ntStatus;


    ntStatus = ioStatus.Status;

    FreeBT_DbgPrint(3, ("FreeBT_ResetParentPort: Leaving\n"));

    return ntStatus;

}

// Send an idle request to the lower driver
NTSTATUS NTAPI SubmitIdleRequestIrp(IN PDEVICE_EXTENSION DeviceExtension)
{
    PIRP                    irp;
    NTSTATUS                ntStatus;
    KIRQL                   oldIrql;
    PUSB_IDLE_CALLBACK_INFO idleCallbackInfo;
    PIO_STACK_LOCATION      nextStack;

    FreeBT_DbgPrint(3, ("SubmitIdleRequest: Entered\n"));

    irp = NULL;
    idleCallbackInfo = NULL;

    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

    if(PowerDeviceD0 != DeviceExtension->DevPower) {

        ntStatus = STATUS_POWER_STATE_INVALID;

        goto SubmitIdleRequestIrp_Exit;
    }

    KeAcquireSpinLock(&DeviceExtension->IdleReqStateLock, &oldIrql);

    if(InterlockedExchange(&DeviceExtension->IdleReqPend, 1)) {

        FreeBT_DbgPrint(1, ("Idle request pending..\n"));

        KeReleaseSpinLock(&DeviceExtension->IdleReqStateLock, oldIrql);

        ntStatus = STATUS_DEVICE_BUSY;

        goto SubmitIdleRequestIrp_Exit;
    }

    //
    // clear the NoIdleReqPendEvent because we are about
    // to submit an idle request. Since we are so early
    // to clear this event, make sure that if we fail this
    // request we set back the event.
    //
    KeClearEvent(&DeviceExtension->NoIdleReqPendEvent);

    idleCallbackInfo = (PUSB_IDLE_CALLBACK_INFO)ExAllocatePool(NonPagedPool, sizeof(struct _USB_IDLE_CALLBACK_INFO));

    if(idleCallbackInfo) {

        idleCallbackInfo->IdleCallback = (USB_IDLE_CALLBACK)IdleNotificationCallback;

        idleCallbackInfo->IdleContext = (PVOID)DeviceExtension;

        ASSERT(DeviceExtension->IdleCallbackInfo == NULL);

        DeviceExtension->IdleCallbackInfo = idleCallbackInfo;

        //
        // we use IoAllocateIrp to create an irp to selectively suspend the
        // device. This irp lies pending with the hub driver. When appropriate
        // the hub driver will invoked callback, where we power down. The completion
        // routine is invoked when we power back.
        //
        irp = IoAllocateIrp(DeviceExtension->TopOfStackDeviceObject->StackSize,
                            FALSE);

        if(irp == NULL) {

            FreeBT_DbgPrint(1, ("cannot build idle request irp\n"));

            KeSetEvent(&DeviceExtension->NoIdleReqPendEvent,
                       IO_NO_INCREMENT,
                       FALSE);

            InterlockedExchange(&DeviceExtension->IdleReqPend, 0);

            KeReleaseSpinLock(&DeviceExtension->IdleReqStateLock, oldIrql);

            ExFreePool(idleCallbackInfo);

            ntStatus = STATUS_INSUFFICIENT_RESOURCES;

            goto SubmitIdleRequestIrp_Exit;
        }

        nextStack = IoGetNextIrpStackLocation(irp);

        nextStack->MajorFunction =
                    IRP_MJ_INTERNAL_DEVICE_CONTROL;

        nextStack->Parameters.DeviceIoControl.IoControlCode =
                    IOCTL_INTERNAL_USB_SUBMIT_IDLE_NOTIFICATION;

        nextStack->Parameters.DeviceIoControl.Type3InputBuffer =
                    idleCallbackInfo;

        nextStack->Parameters.DeviceIoControl.InputBufferLength =
                    sizeof(struct _USB_IDLE_CALLBACK_INFO);


        IoSetCompletionRoutine(irp,
                               (PIO_COMPLETION_ROUTINE)IdleNotificationRequestComplete,
                               DeviceExtension,
                               TRUE,
                               TRUE,
                               TRUE);

        DeviceExtension->PendingIdleIrp = irp;

        //
        // we initialize the count to 2.
        // The reason is, if the CancelSelectSuspend routine manages
        // to grab the irp from the device extension, then the last of the
        // CancelSelectSuspend routine/IdleNotificationRequestComplete routine
        // to execute will free this irp. We need to have this schema so that
        // 1. completion routine does not attempt to touch the irp freed by
        //    CancelSelectSuspend routine.
        // 2. CancelSelectSuspend routine doesnt wait for ever for the completion
        //    routine to complete!
        //
        DeviceExtension->FreeIdleIrpCount = 2;

        KeReleaseSpinLock(&DeviceExtension->IdleReqStateLock, oldIrql);

        //
        // check if the device is idle.
        // A check here ensures that a race condition did not
        // completely reverse the call sequence of SubmitIdleRequestIrp
        // and CancelSelectiveSuspend
        //

        if(!CanDeviceSuspend(DeviceExtension) ||
           PowerDeviceD0 != DeviceExtension->DevPower) {

            //
            // IRPs created using IoBuildDeviceIoControlRequest should be
            // completed by calling IoCompleteRequest and not merely
            // deallocated.
            //

            FreeBT_DbgPrint(1, ("Device is not idle\n"));

            KeAcquireSpinLock(&DeviceExtension->IdleReqStateLock, &oldIrql);

            DeviceExtension->IdleCallbackInfo = NULL;

            DeviceExtension->PendingIdleIrp = NULL;

            KeSetEvent(&DeviceExtension->NoIdleReqPendEvent,
                       IO_NO_INCREMENT,
                       FALSE);

            InterlockedExchange(&DeviceExtension->IdleReqPend, 0);

            KeReleaseSpinLock(&DeviceExtension->IdleReqStateLock, oldIrql);

            if(idleCallbackInfo) {

                ExFreePool(idleCallbackInfo);
            }

            //
            // it is still safe to touch the local variable "irp" here.
            // the irp has not been passed down the stack, the irp has
            // no cancellation routine. The worse position is that the
            // CancelSelectSuspend has run after we released the spin
            // lock above. It is still essential to free the irp.
            //
            if(irp) {

                IoFreeIrp(irp);
            }

            ntStatus = STATUS_UNSUCCESSFUL;

            goto SubmitIdleRequestIrp_Exit;
        }

        FreeBT_DbgPrint(3, ("Cancel the timers\n"));
        //
        // Cancel the timer so that the DPCs are no longer fired.
        // Thus, we are making judicious usage of our resources.
        // we do not need DPCs because we already have an idle irp pending.
        // The timers are re-initialized in the completion routine.
        //
        KeCancelTimer(&DeviceExtension->Timer);

        ntStatus = IoCallDriver(DeviceExtension->TopOfStackDeviceObject, irp);

        if(!NT_SUCCESS(ntStatus)) {

            FreeBT_DbgPrint(1, ("IoCallDriver failed\n"));

            goto SubmitIdleRequestIrp_Exit;
        }
    }
    else {

        FreeBT_DbgPrint(1, ("Memory allocation for idleCallbackInfo failed\n"));

        KeSetEvent(&DeviceExtension->NoIdleReqPendEvent,
                   IO_NO_INCREMENT,
                   FALSE);

        InterlockedExchange(&DeviceExtension->IdleReqPend, 0);

        KeReleaseSpinLock(&DeviceExtension->IdleReqStateLock, oldIrql);

        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

SubmitIdleRequestIrp_Exit:

    FreeBT_DbgPrint(3, ("SubmitIdleRequest: Leaving\n"));

    return ntStatus;
}


VOID NTAPI IdleNotificationCallback(IN PDEVICE_EXTENSION DeviceExtension)
{
    NTSTATUS                ntStatus;
    POWER_STATE             powerState;
    KEVENT                  irpCompletionEvent;
    PIRP_COMPLETION_CONTEXT irpContext;

    FreeBT_DbgPrint(3, ("FBTUSB: IdleNotificationCallback: Entered\n"));

    //
    // Dont idle, if the device was just disconnected or being stopped
    // i.e. return for the following DeviceState(s)
    // NotStarted, Stopped, PendingStop, PendingRemove, SurpriseRemoved, Removed
    //

    if(DeviceExtension->DeviceState != Working) {

        return;
    }

    //
    // If there is not already a WW IRP pending, submit one now
    //
    if(DeviceExtension->WaitWakeEnable) {

        IssueWaitWake(DeviceExtension);
    }


    //
    // power down the device
    //

    irpContext = (PIRP_COMPLETION_CONTEXT)
                 ExAllocatePool(NonPagedPool,
                                sizeof(IRP_COMPLETION_CONTEXT));

    if(!irpContext) {

        FreeBT_DbgPrint(1, ("FBTUSB: IdleNotificationCallback: Failed to alloc memory for irpContext\n"));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    else {

        //
        // increment the count. In the HoldIoRequestWorkerRoutine, the
        // count is decremented twice (one for the system Irp and the
        // other for the device Irp. An increment here compensates for
        // the sytem irp..The decrement corresponding to this increment
        // is in the completion function
        //

        FreeBT_DbgPrint(3, ("FBTUSB: IdleNotificationCallback::"));
        FreeBT_IoIncrement(DeviceExtension);

        powerState.DeviceState = (DEVICE_POWER_STATE) DeviceExtension->PowerDownLevel;

        KeInitializeEvent(&irpCompletionEvent, NotificationEvent, FALSE);

        irpContext->DeviceExtension = DeviceExtension;
        irpContext->Event = &irpCompletionEvent;

        ntStatus = PoRequestPowerIrp(
                          DeviceExtension->PhysicalDeviceObject,
                          IRP_MN_SET_POWER,
                          powerState,
                          (PREQUEST_POWER_COMPLETE) PoIrpCompletionFunc,
                          irpContext,
                          NULL);

        if(STATUS_PENDING == ntStatus) {

            FreeBT_DbgPrint(3, ("FBTUSB: IdleNotificationCallback::"
                           "waiting for the power irp to complete\n"));

            KeWaitForSingleObject(&irpCompletionEvent,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
        }
    }

    if(!NT_SUCCESS(ntStatus)) {

        if(irpContext) {

            ExFreePool(irpContext);
        }
    }

    FreeBT_DbgPrint(3, ("FBTUSB: IdleNotificationCallback: Leaving\n"));
}


NTSTATUS NTAPI IdleNotificationRequestComplete(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PDEVICE_EXTENSION DeviceExtension)
{
    NTSTATUS                ntStatus;
    POWER_STATE             powerState;
    KIRQL                   oldIrql;
    LARGE_INTEGER           dueTime;
    PIRP                    idleIrp;
    PUSB_IDLE_CALLBACK_INFO idleCallbackInfo;

    FreeBT_DbgPrint(3, ("FBTUSB: IdleNotificationRequestCompete: Entered\n"));

    idleIrp = NULL;

    ntStatus = Irp->IoStatus.Status;
    if(!NT_SUCCESS(ntStatus) && ntStatus != STATUS_NOT_SUPPORTED)
    {
        FreeBT_DbgPrint(3, ("FBTUSB: IdleNotificationRequestCompete: Idle irp completes with error::"));
        switch(ntStatus)
        {
        case STATUS_INVALID_DEVICE_REQUEST:
            FreeBT_DbgPrint(3, ("STATUS_INVALID_DEVICE_REQUEST\n"));
            break;

        case STATUS_CANCELLED:
            FreeBT_DbgPrint(3, ("STATUS_CANCELLED\n"));
            break;

        case STATUS_DEVICE_BUSY:
            FreeBT_DbgPrint(3, ("STATUS_DEVICE_BUSY\n"));
            break;

        case STATUS_POWER_STATE_INVALID:
            FreeBT_DbgPrint(3, ("STATUS_POWER_STATE_INVALID\n"));
            goto IdleNotificationRequestComplete_Exit;

        default:
            FreeBT_DbgPrint(3, ("default: status = %X\n", ntStatus));
            break;

        }

        // if in error, issue a SetD0
        FreeBT_DbgPrint(3, ("FBTUSB: IdleNotificationRequestComplete::"));
        FreeBT_IoIncrement(DeviceExtension);

        powerState.DeviceState = PowerDeviceD0;
        ntStatus = PoRequestPowerIrp(
                          DeviceExtension->PhysicalDeviceObject,
                          IRP_MN_SET_POWER,
                          powerState,
                          (PREQUEST_POWER_COMPLETE) PoIrpAsyncCompletionFunc,
                          DeviceExtension,
                          NULL);

        if(!NT_SUCCESS(ntStatus))
            FreeBT_DbgPrint(1, ("PoRequestPowerIrp failed\n"));

    }

IdleNotificationRequestComplete_Exit:
    KeAcquireSpinLock(&DeviceExtension->IdleReqStateLock, &oldIrql);
    idleCallbackInfo = DeviceExtension->IdleCallbackInfo;
    DeviceExtension->IdleCallbackInfo = NULL;

    idleIrp = (PIRP) InterlockedExchangePointer((PVOID*)&DeviceExtension->PendingIdleIrp, NULL);
    InterlockedExchange(&DeviceExtension->IdleReqPend, 0);

    KeReleaseSpinLock(&DeviceExtension->IdleReqStateLock, oldIrql);

    if(idleCallbackInfo)
        ExFreePool(idleCallbackInfo);

    // Since the irp was created using IoAllocateIrp,
    // the Irp needs to be freed using IoFreeIrp.
    // Also return STATUS_MORE_PROCESSING_REQUIRED so that
    // the kernel does not reference this in the near future.
    if(idleIrp)
    {
        FreeBT_DbgPrint(3, ("completion routine has a valid irp and frees it\n"));
        IoFreeIrp(Irp);
        KeSetEvent(&DeviceExtension->NoIdleReqPendEvent, IO_NO_INCREMENT, FALSE);

    }

    else
    {
        // The CancelSelectiveSuspend routine has grabbed the Irp from the device
        // extension. Now the last one to decrement the FreeIdleIrpCount should
        // free the irp.
        if (0 == InterlockedDecrement(&DeviceExtension->FreeIdleIrpCount))
        {
            FreeBT_DbgPrint(3, ("completion routine frees the irp\n"));
            IoFreeIrp(Irp);
            KeSetEvent(&DeviceExtension->NoIdleReqPendEvent, IO_NO_INCREMENT, FALSE);

        }

    }

    if(DeviceExtension->SSEnable)
    {
        FreeBT_DbgPrint(3, ("Set the timer to fire DPCs\n"));
        dueTime.QuadPart = -10000 * IDLE_INTERVAL;               // 5000 ms
        KeSetTimerEx(&DeviceExtension->Timer, dueTime, IDLE_INTERVAL, &DeviceExtension->DeferredProcCall);
        FreeBT_DbgPrint(3, ("IdleNotificationRequestCompete: Leaving\n"));

    }

    return STATUS_MORE_PROCESSING_REQUIRED;

}

VOID NTAPI CancelSelectSuspend(IN PDEVICE_EXTENSION DeviceExtension)
{
    PIRP  irp;
    KIRQL oldIrql;

    FreeBT_DbgPrint(3, ("CancelSelectSuspend: Entered\n"));

    irp = NULL;

    KeAcquireSpinLock(&DeviceExtension->IdleReqStateLock, &oldIrql);

    if(!CanDeviceSuspend(DeviceExtension))
    {
        FreeBT_DbgPrint(3, ("Device is not idle\n"));
        irp = (PIRP) InterlockedExchangePointer((PVOID*)&DeviceExtension->PendingIdleIrp, NULL);

    }

    KeReleaseSpinLock(&DeviceExtension->IdleReqStateLock, oldIrql);

    // since we have a valid Irp ptr,
    // we can call IoCancelIrp on it,
    // without the fear of the irp
    // being freed underneath us.
    if(irp)
    {
        // This routine has the irp pointer.
        // It is safe to call IoCancelIrp because we know that
        // the compleiton routine will not free this irp unless...
        //
        //
        if(IoCancelIrp(irp))
        {
            FreeBT_DbgPrint(3, ("IoCancelIrp returns TRUE\n"));

        }

        else
        {
            FreeBT_DbgPrint(3, ("IoCancelIrp returns FALSE\n"));

        }

        // ....we decrement the FreeIdleIrpCount from 2 to 1.
        // if completion routine runs ahead of us, then this routine
        // decrements the FreeIdleIrpCount from 1 to 0 and hence shall
        // free the irp.
        if(0 == InterlockedDecrement(&DeviceExtension->FreeIdleIrpCount))
        {
            FreeBT_DbgPrint(3, ("CancelSelectSuspend frees the irp\n"));
            IoFreeIrp(irp);
            KeSetEvent(&DeviceExtension->NoIdleReqPendEvent, IO_NO_INCREMENT, FALSE);

        }

    }

    FreeBT_DbgPrint(3, ("CancelSelectSuspend: Leaving\n"));

    return;

}

VOID NTAPI PoIrpCompletionFunc(IN PDEVICE_OBJECT DeviceObject, IN UCHAR MinorFunction, IN POWER_STATE PowerState, IN PVOID Context, IN PIO_STATUS_BLOCK IoStatus)
{
    PIRP_COMPLETION_CONTEXT irpContext;
    irpContext = NULL;

    FreeBT_DbgPrint(3, ("PoIrpCompletionFunc::"));

    if(Context)
        irpContext = (PIRP_COMPLETION_CONTEXT) Context;

    // all we do is set the event and decrement the count
    if(irpContext)
    {
        KeSetEvent(irpContext->Event, 0, FALSE);
        FreeBT_IoDecrement(irpContext->DeviceExtension);
        ExFreePool(irpContext);

    }

    return;

}

VOID NTAPI PoIrpAsyncCompletionFunc(IN PDEVICE_OBJECT DeviceObject, IN UCHAR MinorFunction, IN POWER_STATE PowerState, IN PVOID Context, IN PIO_STATUS_BLOCK IoStatus)
{
    PDEVICE_EXTENSION DeviceExtension = (PDEVICE_EXTENSION) Context;
    FreeBT_DbgPrint(3, ("PoIrpAsyncCompletionFunc::"));
    FreeBT_IoDecrement(DeviceExtension);

    return;

}

VOID NTAPI WWIrpCompletionFunc(IN PDEVICE_OBJECT DeviceObject, IN UCHAR MinorFunction, IN POWER_STATE PowerState, IN PVOID Context, IN PIO_STATUS_BLOCK IoStatus)
{
    PDEVICE_EXTENSION DeviceExtension = (PDEVICE_EXTENSION) Context;

    FreeBT_DbgPrint(3, ("WWIrpCompletionFunc::"));
    FreeBT_IoDecrement(DeviceExtension);

    return;

}
