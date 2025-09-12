/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Storage Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     USB block storage device driver.
 * COPYRIGHT:   2005-2006 James Tabor
 *              2011-2012 Michael Martin (michael.martin@reactos.org)
 *              2011-2013 Johannes Anderwald (johannes.anderwald@reactos.org)
 *              2019 Victor Perevertkin (victor.perevertkin@reactos.org)
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

    DPRINT("Allocating URB\n");
    Urb = (PURB)AllocateItem(NonPagedPool, sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST));
    if (!Urb)
    {
        DPRINT1("OutofMemory!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // build status
    UsbBuildGetStatusRequest(Urb, URB_FUNCTION_GET_STATUS_FROM_ENDPOINT, bEndpointAddress & 0x0F, Value, NULL, NULL);

    // send the request
    DPRINT1("Sending Request DeviceObject %p, Urb %p\n", DeviceObject, Urb);
    Status = USBSTOR_SyncUrbRequest(DeviceObject, Urb);

    FreeItem(Urb);
    return Status;
}

NTSTATUS
USBSTOR_ResetPipeWithHandle(
    IN PDEVICE_OBJECT DeviceObject,
    IN USBD_PIPE_HANDLE PipeHandle)
{
    PURB Urb;
    NTSTATUS Status;

    DPRINT("Allocating URB\n");
    Urb = (PURB)AllocateItem(NonPagedPool, sizeof(struct _URB_PIPE_REQUEST));
    if (!Urb)
    {
        DPRINT1("OutofMemory!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Urb->UrbPipeRequest.Hdr.Length = sizeof(struct _URB_PIPE_REQUEST);
    Urb->UrbPipeRequest.Hdr.Function = URB_FUNCTION_SYNC_RESET_PIPE_AND_CLEAR_STALL;
    Urb->UrbPipeRequest.PipeHandle = PipeHandle;

    // send the request
    DPRINT1("Sending Request DeviceObject %p, Urb %p\n", DeviceObject, Urb);
    Status = USBSTOR_SyncUrbRequest(DeviceObject, Urb);

    FreeItem(Urb);
    return Status;
}

VOID
NTAPI
USBSTOR_ResetPipeWorkItemRoutine(
    IN PDEVICE_OBJECT FdoDevice,
    IN PVOID Ctx)
{
    NTSTATUS Status;
    PFDO_DEVICE_EXTENSION FDODeviceExtension = (PFDO_DEVICE_EXTENSION)Ctx;
    PIRP_CONTEXT Context = &FDODeviceExtension->CurrentIrpContext;

    // clear stall on the corresponding pipe
    Status = USBSTOR_ResetPipeWithHandle(FDODeviceExtension->LowerDeviceObject, Context->Urb.UrbBulkOrInterruptTransfer.PipeHandle);
    DPRINT1("USBSTOR_ResetPipeWithHandle Status %x\n", Status);

    // now resend the csw as the stall got cleared
    USBSTOR_SendCSWRequest(FDODeviceExtension, Context->Irp);
}

VOID
NTAPI
USBSTOR_ResetDeviceWorkItemRoutine(
    IN PDEVICE_OBJECT FdoDevice,
    IN PVOID Context)
{
    PFDO_DEVICE_EXTENSION FDODeviceExtension;
    UINT32 ix;
    NTSTATUS Status;
    KIRQL OldIrql;

    DPRINT("USBSTOR_ResetDeviceWorkItemRoutine\n");

    FDODeviceExtension = FdoDevice->DeviceExtension;

    for (ix = 0; ix < 3; ++ix)
    {
        // first perform a mass storage reset step 1 in 5.3.4 USB Mass Storage Bulk Only Specification
        Status = USBSTOR_ResetDevice(FDODeviceExtension->LowerDeviceObject, FDODeviceExtension);
        if (NT_SUCCESS(Status))
        {
            // step 2 reset bulk in pipe section 5.3.4
            Status = USBSTOR_ResetPipeWithHandle(FDODeviceExtension->LowerDeviceObject, FDODeviceExtension->InterfaceInformation->Pipes[FDODeviceExtension->BulkInPipeIndex].PipeHandle);
            if (NT_SUCCESS(Status))
            {
                // finally reset bulk out pipe
                Status = USBSTOR_ResetPipeWithHandle(FDODeviceExtension->LowerDeviceObject, FDODeviceExtension->InterfaceInformation->Pipes[FDODeviceExtension->BulkOutPipeIndex].PipeHandle);
                if (NT_SUCCESS(Status))
                {
                    break;
                }
            }
        }
    }

    KeAcquireSpinLock(&FDODeviceExtension->CommonLock, &OldIrql);
    FDODeviceExtension->Flags &= ~USBSTOR_FDO_FLAGS_DEVICE_RESETTING;
    KeReleaseSpinLock(&FDODeviceExtension->CommonLock, OldIrql);

    USBSTOR_QueueNextRequest(FdoDevice);
}

VOID
NTAPI
USBSTOR_QueueResetPipe(
    IN PFDO_DEVICE_EXTENSION FDODeviceExtension)
{
    DPRINT("USBSTOR_QueueResetPipe\n");

    IoQueueWorkItem(FDODeviceExtension->ResetDeviceWorkItem,
                    USBSTOR_ResetPipeWorkItemRoutine,
                    CriticalWorkQueue,
                    FDODeviceExtension);
}

VOID
NTAPI
USBSTOR_QueueResetDevice(
    IN PFDO_DEVICE_EXTENSION FDODeviceExtension)
{
    KIRQL OldIrql;

    DPRINT("USBSTOR_QueueResetDevice\n");

    KeAcquireSpinLock(&FDODeviceExtension->CommonLock, &OldIrql);
    FDODeviceExtension->Flags |= USBSTOR_FDO_FLAGS_DEVICE_RESETTING;
    KeReleaseSpinLock(&FDODeviceExtension->CommonLock, OldIrql);

    IoQueueWorkItem(FDODeviceExtension->ResetDeviceWorkItem,
                    USBSTOR_ResetDeviceWorkItemRoutine,
                    CriticalWorkQueue,
                    NULL);
}

VOID
NTAPI
USBSTOR_TimerWorkerRoutine(
    IN PVOID Context)
{
    PFDO_DEVICE_EXTENSION FDODeviceExtension;
    NTSTATUS Status;
    PERRORHANDLER_WORKITEM_DATA WorkItemData = (PERRORHANDLER_WORKITEM_DATA)Context;

    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)WorkItemData->DeviceObject->DeviceExtension;
    ASSERT(FDODeviceExtension->Common.IsFDO);

    // first perform a mass storage reset step 1 in 5.3.4 USB Mass Storage Bulk Only Specification
    Status = USBSTOR_ResetDevice(FDODeviceExtension->LowerDeviceObject, FDODeviceExtension);
    if (NT_SUCCESS(Status))
    {
        // step 2 reset bulk in pipe section 5.3.4
        Status = USBSTOR_ResetPipeWithHandle(FDODeviceExtension->LowerDeviceObject, FDODeviceExtension->InterfaceInformation->Pipes[FDODeviceExtension->BulkInPipeIndex].PipeHandle);
        if (NT_SUCCESS(Status))
        {
            // finally reset bulk out pipe
            Status = USBSTOR_ResetPipeWithHandle(FDODeviceExtension->LowerDeviceObject, FDODeviceExtension->InterfaceInformation->Pipes[FDODeviceExtension->BulkOutPipeIndex].PipeHandle);
        }
    }
    DPRINT1("Status %x\n", Status);

    // clear timer srb
    FDODeviceExtension->LastTimerActiveSrb = NULL;

    // re-schedule request
    //USBSTOR_HandleExecuteSCSI(WorkItemData->Context->PDODeviceExtension->Self, WorkItemData->Context->Irp, Context->RetryCount + 1);

    // do not retry for the same packet again
    FDODeviceExtension->TimerWorkQueueEnabled = FALSE;

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

    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)Context;
    DPRINT1("[USBSTOR] TimerRoutine entered\n");
    // DPRINT1("[USBSTOR] ActiveSrb %p ResetInProgress %x LastTimerActiveSrb %p\n", FDODeviceExtension->ActiveSrb, FDODeviceExtension->ResetInProgress, FDODeviceExtension->LastTimerActiveSrb);

    KeAcquireSpinLockAtDpcLevel(&FDODeviceExtension->IrpListLock);

    // is there an active srb and no global reset is in progress
    if (FDODeviceExtension->ActiveSrb && /* FDODeviceExtension->ResetInProgress == FALSE && */ FDODeviceExtension->TimerWorkQueueEnabled)
    {
        if (FDODeviceExtension->LastTimerActiveSrb != NULL && FDODeviceExtension->LastTimerActiveSrb == FDODeviceExtension->ActiveSrb)
        {
            // check if empty
            DPRINT1("[USBSTOR] ActiveSrb %p hang detected\n", FDODeviceExtension->ActiveSrb);
            ResetDevice = TRUE;
        }
        else
        {
            // update pointer
            FDODeviceExtension->LastTimerActiveSrb = FDODeviceExtension->ActiveSrb;
        }
    }
    else
    {
        // reset srb
        FDODeviceExtension->LastTimerActiveSrb = NULL;
    }

    KeReleaseSpinLockFromDpcLevel(&FDODeviceExtension->IrpListLock);


    if (ResetDevice && FDODeviceExtension->TimerWorkQueueEnabled && FDODeviceExtension->SrbErrorHandlingActive == FALSE)
    {
        WorkItemData = ExAllocatePoolWithTag(NonPagedPool,
                                             sizeof(ERRORHANDLER_WORKITEM_DATA),
                                             USB_STOR_TAG);
        if (WorkItemData)
        {
           // Initialize and queue the work item to handle the error
           ExInitializeWorkItem(&WorkItemData->WorkQueueItem,
                                 USBSTOR_TimerWorkerRoutine,
                                 WorkItemData);

           WorkItemData->DeviceObject = FDODeviceExtension->FunctionalDeviceObject;

           DPRINT1("[USBSTOR] Queing Timer WorkItem\n");
           ExQueueWorkItem(&WorkItemData->WorkQueueItem, DelayedWorkQueue);
        }
     }
}
