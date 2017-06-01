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

VOID
NTAPI
USBSTOR_BulkResetPipeWorkItem(
    IN PDEVICE_OBJECT FdoDevice,
    IN PVOID Context)
{
    PFDO_DEVICE_EXTENSION FDODeviceExtension;

    DPRINT("USBSTOR_BulkResetPipeWorkItem: \n");

    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)FdoDevice->DeviceExtension;

    USBSTOR_ResetPipeWithHandle(FDODeviceExtension->LowerDeviceObject,
                                FDODeviceExtension->Urb.PipeHandle);

    USBSTOR_CswTransfer(FDODeviceExtension, FdoDevice->CurrentIrp);

    //FIXME RemoveLock
}

VOID
NTAPI
USBSTOR_BulkQueueResetPipe(
    IN PFDO_DEVICE_EXTENSION FDODeviceExtension)
{
    DPRINT("USBSTOR_BulkQueueResetPipe: \n");

    //FIXME RemoveLock

    IoQueueWorkItem(FDODeviceExtension->ResetDeviceWorkItem,
                    USBSTOR_BulkResetPipeWorkItem,
                    CriticalWorkQueue,
                    0);
}

