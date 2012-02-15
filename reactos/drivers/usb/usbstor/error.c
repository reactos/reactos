/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Storage Driver
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbstor/error.c
 * PURPOSE:     USB block storage device driver.
 * PROGRAMMERS:
 *              James Tabor
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "usbstor.h"

NTSTATUS
USBSTOR_GetEndpointStatus(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR bEndpointAddress,
    OUT PUSHORT Value)
{
    PURB Urb;
    NTSTATUS Status;

    //
    // allocate urb
    //
    DPRINT("Allocating URB\n");
    Urb = (PURB)AllocateItem(NonPagedPool, sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST));
    if (!Urb)
    {
        //
        // out of memory
        //
        DPRINT1("OutofMemory!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // build status
    //
   UsbBuildGetStatusRequest(Urb, URB_FUNCTION_GET_STATUS_FROM_ENDPOINT, bEndpointAddress & 0x0F, Value, NULL, NULL);

    //
    // send the request
    //
    DPRINT1("Sending Request DeviceObject %x, Urb %x\n", DeviceObject, Urb);
    Status = USBSTOR_SyncUrbRequest(DeviceObject, Urb);

    //
    // free urb
    //
    FreeItem(Urb);

    //
    // done
    //
    return Status;
}



NTSTATUS
USBSTOR_ResetPipeWithHandle(
    IN PDEVICE_OBJECT DeviceObject,
    IN USBD_PIPE_HANDLE PipeHandle)
{
    PURB Urb;
    NTSTATUS Status;

    //
    // allocate urb
    //
    DPRINT("Allocating URB\n");
    Urb = (PURB)AllocateItem(NonPagedPool, sizeof(struct _URB_PIPE_REQUEST));
    if (!Urb)
    {
        //
        // out of memory
        //
        DPRINT1("OutofMemory!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // initialize the urb
    //
    Urb->UrbPipeRequest.Hdr.Length = sizeof(struct _URB_PIPE_REQUEST);
    Urb->UrbPipeRequest.Hdr.Function = URB_FUNCTION_SYNC_RESET_PIPE_AND_CLEAR_STALL;
    Urb->UrbPipeRequest.PipeHandle = PipeHandle;

    //
    // send the request
    //
    DPRINT1("Sending Request DeviceObject %x, Urb %x\n", DeviceObject, Urb);
    Status = USBSTOR_SyncUrbRequest(DeviceObject, Urb);

    //
    // free urb
    //
    FreeItem(Urb);

    //
    // done
    //
    return Status;
}


NTSTATUS
USBSTOR_HandleTransferError(
    PDEVICE_OBJECT DeviceObject,
    PIRP_CONTEXT Context)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION Stack;
    //USBD_PIPE_HANDLE PipeHandle;
    PSCSI_REQUEST_BLOCK Request;
    PCDB pCDB;


    //
    // first perform a mass storage reset step 1 in 5.3.4 USB Mass Storage Bulk Only Specification
    //
    Status = USBSTOR_ResetDevice(Context->FDODeviceExtension->LowerDeviceObject, Context->FDODeviceExtension);
    if (NT_SUCCESS(Status))
    {
        //
        // step 2 reset bulk in pipe section 5.3.4
        //
        Status = USBSTOR_ResetPipeWithHandle(Context->FDODeviceExtension->LowerDeviceObject, Context->FDODeviceExtension->InterfaceInformation->Pipes[Context->FDODeviceExtension->BulkInPipeIndex].PipeHandle);
        if (NT_SUCCESS(Status))
        {
            //
            // finally reset bulk out pipe
            //
            Status = USBSTOR_ResetPipeWithHandle(Context->FDODeviceExtension->LowerDeviceObject, Context->FDODeviceExtension->InterfaceInformation->Pipes[Context->FDODeviceExtension->BulkOutPipeIndex].PipeHandle);
        }
    }

    if (Context->Irp)
    {
        //
        // get next stack location
        //
        Stack = IoGetCurrentIrpStackLocation(Context->Irp);

        //
        // get request block
        //
        Request = (PSCSI_REQUEST_BLOCK)Stack->Parameters.Others.Argument1;
        ASSERT(Request);

        //
        // obtain request type
        //
        pCDB = (PCDB)Request->Cdb;
        ASSERT(pCDB);

        //
        // Cleanup the IRP context
        if (pCDB->AsByte[0] == SCSIOP_READ_CAPACITY)
        {
            FreeItem(Context->TransferData);
        }

        if (Status != STATUS_SUCCESS)
        {
            //
            // Complete the master IRP
            //
            Context->Irp->IoStatus.Status = Status;
            Context->Irp->IoStatus.Information = 0;
            USBSTOR_QueueTerminateRequest(Context->PDODeviceExtension->LowerDeviceObject, Context->Irp);
            IoCompleteRequest(Context->Irp, IO_NO_INCREMENT);

             //
            // Start the next request
            //
            USBSTOR_QueueNextRequest(Context->PDODeviceExtension->LowerDeviceObject);
        }
    }
    else
    {
        if (Status != STATUS_SUCCESS)
        {
            //
            // Signal the context event
            //
            ASSERT(Context->Event);
            KeSetEvent(Context->Event, 0, FALSE);
        }
    }

    if (NT_SUCCESS(Status))
    {
        DPRINT1("Retrying\n");
        USBSTOR_HandleExecuteSCSI(*Context->PDODeviceExtension->PDODeviceObject, Context->Irp);
    }

    //
    // cleanup irp context
    //
    FreeItem(Context->cbw);
    FreeItem(Context);


    DPRINT1("USBSTOR_HandleTransferError returning with Status %x\n", Status);
    return Status;
}

VOID
NTAPI
USBSTOR_ResetHandlerWorkItemRoutine(
    PVOID Context)
{
    NTSTATUS Status;
    USHORT Value;
    PIO_STACK_LOCATION IoStack;

    PERRORHANDLER_WORKITEM_DATA WorkItemData = (PERRORHANDLER_WORKITEM_DATA)Context;

    //
    // clear stall on BulkIn pipe
    //
    Status = USBSTOR_ResetPipeWithHandle(WorkItemData->Context->FDODeviceExtension->LowerDeviceObject, WorkItemData->Context->FDODeviceExtension->InterfaceInformation->Pipes[WorkItemData->Context->FDODeviceExtension->BulkInPipeIndex].PipeHandle);
    DPRINT1("USBSTOR_ResetPipeWithHandle Status %x\n", Status);

    //
    // get next stack location
    //

    IoStack = IoGetNextIrpStackLocation(WorkItemData->Irp);

    //
    // now initialize the urb for sending the csw
    //
    UsbBuildInterruptOrBulkTransferRequest(&WorkItemData->Context->Urb,
                                           sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER),
                                           WorkItemData->Context->FDODeviceExtension->InterfaceInformation->Pipes[WorkItemData->Context->FDODeviceExtension->BulkInPipeIndex].PipeHandle,
                                           WorkItemData->Context->csw,
                                           NULL,
                                           512, //FIXME
                                           USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK,
                                           NULL);

    //
    // initialize stack location
    //
    IoStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    IoStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;
    IoStack->Parameters.Others.Argument1 = (PVOID)&WorkItemData->Context->Urb;
    IoStack->Parameters.DeviceIoControl.InputBufferLength = WorkItemData->Context->Urb.UrbHeader.Length;
    WorkItemData->Irp->IoStatus.Status = STATUS_SUCCESS;


    //
    // setup completion routine
    //
    IoSetCompletionRoutine(WorkItemData->Irp, USBSTOR_CSWCompletionRoutine, Context, TRUE, TRUE, TRUE);

    //
    // call driver
    //
    IoCallDriver(WorkItemData->Context->FDODeviceExtension->LowerDeviceObject, WorkItemData->Irp);
}

VOID
NTAPI
ErrorHandlerWorkItemRoutine(
    PVOID Context)
{
    NTSTATUS Status;
    PERRORHANDLER_WORKITEM_DATA WorkItemData = (PERRORHANDLER_WORKITEM_DATA)Context;

    if (WorkItemData->Context->ErrorIndex == 2)
    {
        //
        // reset device
        //
        Status = USBSTOR_HandleTransferError(WorkItemData->DeviceObject, WorkItemData->Context);
    }
    else
    {
        //
        // clear stall
        //
        USBSTOR_ResetHandlerWorkItemRoutine(WorkItemData);
    }

    //
    // Free Work Item Data
    //
    ExFreePool(WorkItemData);
}
