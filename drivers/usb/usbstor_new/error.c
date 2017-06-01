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

#define NDEBUG
#include <debug.h>

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
    DPRINT1("Sending Request DeviceObject %p, Urb %p\n", DeviceObject, Urb);
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
    DPRINT1("Sending Request DeviceObject %p, Urb %p\n", DeviceObject, Urb);
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
    PSCSI_REQUEST_BLOCK Request;
    PCDB pCDB;

    //
    // sanity checks
    //
    ASSERT(Context);
    ASSERT(Context->PDODeviceExtension);
    ASSERT(Context->PDODeviceExtension->Self);
    ASSERT(Context->Irp);

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

    if (Status != STATUS_SUCCESS || Context->RetryCount >= 1)
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

        //
        // srb handling finished
        //
        Context->FDODeviceExtension->SrbErrorHandlingActive = FALSE;

        //
        // clear timer srb
        //
        Context->FDODeviceExtension->LastTimerActiveSrb = NULL;
    }
    else
    {
        DPRINT1("Retrying Count %lu %p\n", Context->RetryCount, Context->PDODeviceExtension->Self);

        //
        // re-schedule request
        //
        USBSTOR_HandleExecuteSCSI(Context->PDODeviceExtension->Self, Context->Irp, Context->RetryCount + 1);

        //
        // srb error handling finished
        //
        Context->FDODeviceExtension->SrbErrorHandlingActive = FALSE;

        //
        // srb error handling finished
        //
        Context->FDODeviceExtension->TimerWorkQueueEnabled = TRUE;

        //
        // clear timer srb
        //
        Context->FDODeviceExtension->LastTimerActiveSrb = NULL;
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
    PERRORHANDLER_WORKITEM_DATA WorkItemData = (PERRORHANDLER_WORKITEM_DATA)Context;

    //
    // clear stall on BulkIn pipe
    //
    Status = USBSTOR_ResetPipeWithHandle(WorkItemData->Context->FDODeviceExtension->LowerDeviceObject, WorkItemData->Context->FDODeviceExtension->InterfaceInformation->Pipes[WorkItemData->Context->FDODeviceExtension->BulkInPipeIndex].PipeHandle);
    DPRINT1("USBSTOR_ResetPipeWithHandle Status %x\n", Status);

    //
    // now resend the csw as the stall got cleared
    //
    USBSTOR_SendCSW(WorkItemData->Context, WorkItemData->Irp);
}

VOID
NTAPI
ErrorHandlerWorkItemRoutine(
    PVOID Context)
{
    PERRORHANDLER_WORKITEM_DATA WorkItemData = (PERRORHANDLER_WORKITEM_DATA)Context;

    if (WorkItemData->Context->ErrorIndex == 2)
    {
        //
        // reset device
        //
        USBSTOR_HandleTransferError(WorkItemData->DeviceObject, WorkItemData->Context);
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
    ExFreePoolWithTag(WorkItemData, USB_STOR_TAG);
}

VOID
NTAPI
USBSTOR_TimerWorkerRoutine(
    IN PVOID Context)
{
    PFDO_DEVICE_EXTENSION FDODeviceExtension;
    NTSTATUS Status;
    PERRORHANDLER_WORKITEM_DATA WorkItemData = (PERRORHANDLER_WORKITEM_DATA)Context;

    //
    // get device extension
    //
    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)WorkItemData->DeviceObject->DeviceExtension;
    ASSERT(FDODeviceExtension->Common.IsFDO);

    //
    // first perform a mass storage reset step 1 in 5.3.4 USB Mass Storage Bulk Only Specification
    //
    Status = USBSTOR_ResetDevice(FDODeviceExtension->LowerDeviceObject, FDODeviceExtension);
    if (NT_SUCCESS(Status))
    {
        //
        // step 2 reset bulk in pipe section 5.3.4
        //
        Status = USBSTOR_ResetPipeWithHandle(FDODeviceExtension->LowerDeviceObject, FDODeviceExtension->InterfaceInformation->Pipes[FDODeviceExtension->BulkInPipeIndex].PipeHandle);
        if (NT_SUCCESS(Status))
        {
            //
            // finally reset bulk out pipe
            //
            Status = USBSTOR_ResetPipeWithHandle(FDODeviceExtension->LowerDeviceObject, FDODeviceExtension->InterfaceInformation->Pipes[FDODeviceExtension->BulkOutPipeIndex].PipeHandle);
        }
    }
    DPRINT1("Status %x\n", Status);

    //
    // clear timer srb
    //
    FDODeviceExtension->LastTimerActiveSrb = NULL;

    //
    // re-schedule request
    //
    //USBSTOR_HandleExecuteSCSI(WorkItemData->Context->PDODeviceExtension->Self, WorkItemData->Context->Irp, Context->RetryCount + 1);



    //
    // do not retry for the same packet again
    //
    FDODeviceExtension->TimerWorkQueueEnabled = FALSE;

    //
    // Free Work Item Data
    //
    ExFreePoolWithTag(WorkItemData, USB_STOR_TAG);
}


VOID
NTAPI
USBSTOR_TimerRoutine(
    PDEVICE_OBJECT DeviceObject,
     PVOID Context)
{
    PFDO_DEVICE_EXTENSION FDODeviceExtension;
    BOOLEAN ResetDevice = FALSE;
    PERRORHANDLER_WORKITEM_DATA WorkItemData;

    //
    // get device extension
    //
    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)Context;
    DPRINT1("[USBSTOR] TimerRoutine entered\n");
    DPRINT1("[USBSTOR] CurrentSrb %p ResetInProgress %x LastTimerActiveSrb %p\n", FDODeviceExtension->CurrentSrb, FDODeviceExtension->ResetInProgress, FDODeviceExtension->LastTimerActiveSrb);

    //
    // acquire spinlock
    //
    KeAcquireSpinLockAtDpcLevel(&FDODeviceExtension->IrpListLock);

    //
    // is there an active srb and no global reset is in progress
    //
    if (FDODeviceExtension->CurrentSrb && FDODeviceExtension->ResetInProgress == FALSE && FDODeviceExtension->TimerWorkQueueEnabled)
    {
        if (FDODeviceExtension->LastTimerActiveSrb != NULL && FDODeviceExtension->LastTimerActiveSrb == FDODeviceExtension->CurrentSrb)
        {
            //
            // check if empty
            //
            DPRINT1("[USBSTOR] CurrentSrb %p hang detected\n", FDODeviceExtension->CurrentSrb);
            ResetDevice = TRUE;
        }
        else
        {
            //
            // update pointer
            //
            FDODeviceExtension->LastTimerActiveSrb = FDODeviceExtension->CurrentSrb;
        }
    }
    else
    {
        //
        // reset srb
        //
        FDODeviceExtension->LastTimerActiveSrb = NULL;
    }

    //
    // release lock
    //
    KeReleaseSpinLockFromDpcLevel(&FDODeviceExtension->IrpListLock);


    if (ResetDevice && FDODeviceExtension->TimerWorkQueueEnabled && FDODeviceExtension->SrbErrorHandlingActive == FALSE)
    {
        WorkItemData = ExAllocatePoolWithTag(NonPagedPool,
                                             sizeof(ERRORHANDLER_WORKITEM_DATA),
                                             USB_STOR_TAG);
        if (WorkItemData)
        {
           //
           // Initialize and queue the work item to handle the error
           //
           ExInitializeWorkItem(&WorkItemData->WorkQueueItem,
                                 USBSTOR_TimerWorkerRoutine,
                                 WorkItemData);

           WorkItemData->DeviceObject = FDODeviceExtension->FunctionalDeviceObject;

           DPRINT1("[USBSTOR] Queing Timer WorkItem\n");
           ExQueueWorkItem(&WorkItemData->WorkQueueItem, DelayedWorkQueue);
        }
     }
}
