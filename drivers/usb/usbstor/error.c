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
    DPRINT1("Allocating URB\n");
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
    DPRINT1("Allocating URB\n");
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
    NTSTATUS Status;
    PIO_STACK_LOCATION Stack;
    USBD_PIPE_HANDLE PipeHandle;
    PSCSI_REQUEST_BLOCK Request;
    PCDB pCDB;

    DPRINT1("Entered Handle Transfer Error\n");
    //
    // Determine pipehandle
    //
    if (Context->cbw->CommandBlock[0] == SCSIOP_WRITE)
    {
        //
        // write request used bulk out pipe
        // 
        PipeHandle = Context->FDODeviceExtension->InterfaceInformation->Pipes[Context->FDODeviceExtension->BulkOutPipeIndex].PipeHandle;
    }
    else
    {
        //
        // default bulk in pipe
        //
        PipeHandle = Context->FDODeviceExtension->InterfaceInformation->Pipes[Context->FDODeviceExtension->BulkInPipeIndex].PipeHandle;
    }

    switch (Context->Urb.UrbHeader.Status)
    {
        case USBD_STATUS_STALL_PID:
        {
            //
            // First attempt to reset the pipe
            //
            DPRINT1("Resetting Pipe\n");
            Status = USBSTOR_ResetPipeWithHandle(Context->FDODeviceExtension->LowerDeviceObject, PipeHandle);
            if (NT_SUCCESS(Status))
            {
                Status = STATUS_SUCCESS;
                break;
            }

            DPRINT1("Failed to reset pipe %x\n", Status);

            //
            // FIXME: Reset of pipe failed, attempt to reset port
            //
            
            Status = STATUS_UNSUCCESSFUL;
            break;
        }
        //
        // FIXME: Handle more errors
        //
        default:
        {
            DPRINT1("Error not handled\n");
            Status = STATUS_UNSUCCESSFUL;
        }
    }

    Stack = IoGetCurrentIrpStackLocation(Context->Irp);
    Request = (PSCSI_REQUEST_BLOCK)Stack->Parameters.Others.Argument1;
    pCDB = (PCDB)Request->Cdb;
    if (Status != STATUS_SUCCESS)
    {
        /* Complete the master IRP */
        Context->Irp->IoStatus.Status = Status;
        Context->Irp->IoStatus.Information = 0;
        USBSTOR_QueueTerminateRequest(Context->PDODeviceExtension->LowerDeviceObject, Context->Irp);
        IoCompleteRequest(Context->Irp, IO_NO_INCREMENT);

        /* Start the next request */
        USBSTOR_QueueNextRequest(Context->PDODeviceExtension->LowerDeviceObject);

        /* Signal the context event */
        if (Context->Event)
            KeSetEvent(Context->Event, 0, FALSE);

        /* Cleanup the IRP context */
        if (pCDB->AsByte[0] == SCSIOP_READ_CAPACITY)
            FreeItem(Context->TransferData);
        FreeItem(Context->cbw);
        FreeItem(Context);
    }
    else
    {

        DPRINT1("Retrying\n");
        Status = USBSTOR_HandleExecuteSCSI(*Context->PDODeviceExtension->PDODeviceObject, Context->Irp);

        /* Cleanup the old IRP context */
        if (pCDB->AsByte[0] == SCSIOP_READ_CAPACITY)
            FreeItem(Context->TransferData);
        FreeItem(Context->cbw);
        FreeItem(Context);
    }

    DPRINT1("USBSTOR_HandleTransferError returning with Status %x\n", Status);
    return Status;
}

VOID
NTAPI
ErrorHandlerWorkItemRoutine(
    PVOID Context)
{
    NTSTATUS Status;
    PERRORHANDLER_WORKITEM_DATA WorkItemData = (PERRORHANDLER_WORKITEM_DATA)Context;
    
    Status = USBSTOR_HandleTransferError(WorkItemData->DeviceObject, WorkItemData->Context);

    //
    // Free Work Item Data
    //
    ExFreePool(WorkItemData);
}
