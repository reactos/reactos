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

NTSTATUS
NTAPI
USBSTOR_ResetDevice(
    IN PDEVICE_OBJECT FdoDevice)
{
    DPRINT1("USBSTOR_ResetDevice: UNIMPLEMENTED. FIXME.\n");
    DbgBreakPoint();
    return 0;
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

VOID
NTAPI
USBSTOR_ResetDeviceWorkItem(
    IN PDEVICE_OBJECT FdoDevice,
    IN PVOID Context)
{
    PFDO_DEVICE_EXTENSION FDODeviceExtension;
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    ULONG nx;
    NTSTATUS Status;
    KIRQL OldIrql;

    DPRINT("USBSTOR_ResetDeviceWorkItem: \n");

    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)Context;
    FDODeviceExtension = FdoDevice->DeviceExtension;

    if (FDODeviceExtension->CurrentIrp)
    {
        IoCancelIrp(FDODeviceExtension->CurrentIrp);

        KeWaitForSingleObject(&FDODeviceExtension->TimeOutEvent,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);

        if (!Context)
        {
            PIO_STACK_LOCATION IoStack;

            IoStack = FDODeviceExtension->CurrentIrp->Tail.Overlay.CurrentStackLocation;
            PDODeviceExtension = IoStack->Parameters.Others.Argument2;
        }

        KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
        IoCompleteRequest(FDODeviceExtension->CurrentIrp, IO_NO_INCREMENT);
        KeLowerIrql(OldIrql);

        FDODeviceExtension->CurrentIrp = NULL;
    }

    nx = 0;

    do
    {
        Status = USBSTOR_IsDeviceConnected(FdoDevice);

        if (!NT_SUCCESS(Status))
        {
            break;
        }

        Status = USBSTOR_ResetDevice(FdoDevice);

        if (NT_SUCCESS(Status))
        {
            break;
        }

        ++nx;
    }
    while (nx < 3);

    KeAcquireSpinLock(&FDODeviceExtension->StorSpinLock, &OldIrql);

    FDODeviceExtension->Flags &= ~USBSTOR_FDO_FLAGS_DEVICE_RESETTING;

    if (!NT_SUCCESS(Status))
    {
        FDODeviceExtension->Flags |= USBSTOR_FDO_FLAGS_DEVICE_ERROR;
    }

    KeReleaseSpinLock(&FDODeviceExtension->StorSpinLock, OldIrql);

    if (!FDODeviceExtension->DriverFlags)
    {
        FDODeviceExtension->DriverFlags = 1;
    }

    if (PDODeviceExtension)
    {
        USBSTOR_QueueNextRequest(FdoDevice);
    }

    //FIXME RemoveLock
}

VOID
NTAPI
USBSTOR_QueueResetDevice(
    IN PFDO_DEVICE_EXTENSION FDODeviceExtension, 
    IN PPDO_DEVICE_EXTENSION PDODeviceExtension)
{
    KIRQL OldIrql;

    DPRINT("USBSTOR_QueueResetDevice: ... \n");

    KeAcquireSpinLock(&FDODeviceExtension->StorSpinLock, &OldIrql);
    FDODeviceExtension->Flags |= USBSTOR_FDO_FLAGS_DEVICE_RESETTING;
    KeReleaseSpinLock(&FDODeviceExtension->StorSpinLock, OldIrql);

    //FIXME RemoveLock

    IoQueueWorkItem(FDODeviceExtension->ResetDeviceWorkItem,
                    USBSTOR_ResetDeviceWorkItem,
                    CriticalWorkQueue,
                    PDODeviceExtension);
}

VOID
NTAPI
USBSTOR_TimerRoutine(
    IN PDEVICE_OBJECT FdoDevice,
    IN PVOID Context)
{
    PFDO_DEVICE_EXTENSION FDODeviceExtension;
    BOOLEAN IsResetDevice = FALSE;

    DPRINT("USBSTOR_TimerRoutine: ... \n");

    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)FdoDevice->DeviceExtension;

    KefAcquireSpinLockAtDpcLevel(&FDODeviceExtension->StorSpinLock);

    if (!(FDODeviceExtension->Flags & USBSTOR_FDO_FLAGS_DEVICE_RESETTING))
    {
        if (FDODeviceExtension->Flags & USBSTOR_FDO_FLAGS_TRANSFER_FINISHED)
        {
            FDODeviceExtension->SrbTimeOutValue--;

            if (FDODeviceExtension->SrbTimeOutValue == 1)
            {
                FDODeviceExtension->Flags |= USBSTOR_FDO_FLAGS_DEVICE_RESETTING;
                IsResetDevice = TRUE;
            }
        }
    }

    KeReleaseSpinLockFromDpcLevel(&FDODeviceExtension->StorSpinLock);

    if (IsResetDevice)
    {
        //FIXME RemoveLock

        IoQueueWorkItem(FDODeviceExtension->ResetDeviceWorkItem,
                        USBSTOR_ResetDeviceWorkItem,
                        CriticalWorkQueue,
                        0);
    }
}
