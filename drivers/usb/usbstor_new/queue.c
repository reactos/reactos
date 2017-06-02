/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Storage Driver
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbstor/queue.c
 * PURPOSE:     USB block storage device driver.
 * PROGRAMMERS:
 *              James Tabor
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "usbstor.h"

#define NDEBUG
#include <debug.h>

VOID
USBSTOR_QueueInitialize(
    PFDO_DEVICE_EXTENSION FDODeviceExtension)
{
    //
    // sanity check
    //
    ASSERT(FDODeviceExtension->Common.IsFDO);

    //
    // initialize queue lock
    //
    KeInitializeSpinLock(&FDODeviceExtension->IrpListLock);

    //
    // initialize irp list head
    //
    InitializeListHead(&FDODeviceExtension->IrpListHead);

    //
    // initialize event
    //
    KeInitializeEvent(&FDODeviceExtension->NoPendingRequests, NotificationEvent, TRUE);
}

VOID
NTAPI
USBSTOR_CancelIo(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    PFDO_DEVICE_EXTENSION FDODeviceExtension;

    //
    // get FDO device extension
    //
    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // sanity check
    //
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    ASSERT(FDODeviceExtension->Common.IsFDO);

    //
    // this IRP isn't in our list here
    //  

    //
    // now release the cancel lock
    //
    IoReleaseCancelSpinLock(Irp->CancelIrql);

    //
    // set cancel status
    //
    Irp->IoStatus.Status = STATUS_CANCELLED;

    //
    // now cancel the irp
    //
    USBSTOR_QueueTerminateRequest(DeviceObject, Irp);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    //
    // start the next one
    //
    USBSTOR_QueueNextRequest(DeviceObject);
}

VOID
NTAPI
USBSTOR_Cancel(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    PFDO_DEVICE_EXTENSION FDODeviceExtension;

    //
    // get FDO device extension
    //
    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // sanity check
    //
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    ASSERT(FDODeviceExtension->Common.IsFDO);

    //
    // acquire irp list lock
    //
    KeAcquireSpinLockAtDpcLevel(&FDODeviceExtension->IrpListLock);

    //
    // remove the irp from the list
    //
    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);

    //
    // release irp list lock
    //
    KeReleaseSpinLockFromDpcLevel(&FDODeviceExtension->IrpListLock);    

    //
    // now release the cancel lock
    //
    IoReleaseCancelSpinLock(Irp->CancelIrql);

    //
    // set cancel status
    //
    Irp->IoStatus.Status = STATUS_CANCELLED;

    //
    // now cancel the irp
    //
    USBSTOR_QueueTerminateRequest(DeviceObject, Irp);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    //
    // start the next one
    //
    USBSTOR_QueueNextRequest(DeviceObject);
}

BOOLEAN
USBSTOR_QueueAddIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PDRIVER_CANCEL OldDriverCancel;
    KIRQL OldLevel;
    PFDO_DEVICE_EXTENSION FDODeviceExtension;
    BOOLEAN IrpListFreeze;
    BOOLEAN SrbProcessing;
    PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation(Irp);
    PSCSI_REQUEST_BLOCK Request = IoStack->Parameters.Scsi.Srb;

    //
    // get FDO device extension
    //
    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // sanity check
    //
    ASSERT(FDODeviceExtension->Common.IsFDO);

    //
    // acquire lock
    //
    KeAcquireSpinLock(&FDODeviceExtension->IrpListLock, &OldLevel);

    //
    // check if there are irp pending
    //
    SrbProcessing = FDODeviceExtension->IrpPendingCount != 0;

    if (SrbProcessing)
    {
        //
        // add irp to queue
        //
        InsertTailList(&FDODeviceExtension->IrpListHead, &Irp->Tail.Overlay.ListEntry);
    }

    //
    // increment pending count
    //
    FDODeviceExtension->IrpPendingCount++;


    //
    // clear the no requests pending event
    //
    KeClearEvent(&FDODeviceExtension->NoPendingRequests);

    //
    // check if queue is freezed
    //
    IrpListFreeze = FDODeviceExtension->IrpListFreeze;

    //
    // release list lock
    //
    KeReleaseSpinLock(&FDODeviceExtension->IrpListLock, OldLevel);

    //
    // synchronize with cancellations by holding the cancel lock
    //
    IoAcquireCancelSpinLock(&Irp->CancelIrql);

    //
    // now set the driver cancel routine
    //
    if (SrbProcessing)
    {
        ASSERT(FDODeviceExtension->CurrentSrb != NULL);

        OldDriverCancel = IoSetCancelRoutine(Irp, USBSTOR_Cancel);
    }
    else
    {
        ASSERT(FDODeviceExtension->CurrentSrb == NULL);

        FDODeviceExtension->CurrentSrb = Request;
        OldDriverCancel = IoSetCancelRoutine(Irp, USBSTOR_CancelIo);
    }

    //
    // check if the irp has already been cancelled
    //
    if (Irp->Cancel && OldDriverCancel == NULL)
    {
        //
        // cancel irp
        //
        Irp->CancelRoutine(DeviceObject, Irp);

        //
        // irp was cancelled
        //
        return FALSE;
    }

    //
    // release the cancel lock
    //
    IoReleaseCancelSpinLock(Irp->CancelIrql);

    //
    // if list is freezed, dont start this packet
    //
    DPRINT("IrpListFreeze: %lu IrpPendingCount %lu\n", IrpListFreeze, FDODeviceExtension->IrpPendingCount);

    return (IrpListFreeze || SrbProcessing);
}

PIRP
USBSTOR_RemoveIrp(
    IN PDEVICE_OBJECT DeviceObject)
{
    KIRQL OldLevel;
    PFDO_DEVICE_EXTENSION FDODeviceExtension;
    PLIST_ENTRY Entry;
    PIRP Irp = NULL;

    //
    // get FDO device extension
    //
    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // sanity check
    //
    ASSERT(FDODeviceExtension->Common.IsFDO);

    //
    // acquire lock
    //
    KeAcquireSpinLock(&FDODeviceExtension->IrpListLock, &OldLevel);

    //
    // check if list is empty
    //
    if (!IsListEmpty(&FDODeviceExtension->IrpListHead))
    {
        //
        // remove entry
        //
        Entry = RemoveHeadList(&FDODeviceExtension->IrpListHead);

        //
        // get offset to start of irp
        //
        Irp = (PIRP)CONTAINING_RECORD(Entry, IRP, Tail.Overlay.ListEntry);
    }

    //
    // release list lock
    //
    KeReleaseSpinLock(&FDODeviceExtension->IrpListLock, OldLevel);

    //
    // return result
    //
    return Irp;
}

VOID
USBSTOR_QueueWaitForPendingRequests(
    IN PDEVICE_OBJECT DeviceObject)
{
    PFDO_DEVICE_EXTENSION FDODeviceExtension;

    //
    // get FDO device extension
    //
    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // perform the wait
    //
    KeWaitForSingleObject(&FDODeviceExtension->NoPendingRequests,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);
}

VOID
USBSTOR_QueueTerminateRequest(
    IN PDEVICE_OBJECT FDODeviceObject,
    IN PIRP Irp)
{
    KIRQL OldLevel;
    PFDO_DEVICE_EXTENSION FDODeviceExtension;
    PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation(Irp);
    PSCSI_REQUEST_BLOCK Request = IoStack->Parameters.Scsi.Srb;

    //
    // get FDO device extension
    //
    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)FDODeviceObject->DeviceExtension;

    //
    // sanity check
    //
    ASSERT(FDODeviceExtension->Common.IsFDO);

    //
    // acquire lock
    //
    KeAcquireSpinLock(&FDODeviceExtension->IrpListLock, &OldLevel);

    //
    // decrement pending irp count
    //
    FDODeviceExtension->IrpPendingCount--;

    //
    // check if this was our current active SRB
    //
    if (FDODeviceExtension->CurrentSrb == Request)
    {
        //
        // indicate processing is completed
        //
        FDODeviceExtension->CurrentSrb = NULL;
    }

    //
    // Set the event if nothing else is pending
    //
    if (FDODeviceExtension->IrpPendingCount == 0 &&
        FDODeviceExtension->CurrentSrb == NULL)
    {
        KeSetEvent(&FDODeviceExtension->NoPendingRequests, IO_NO_INCREMENT, FALSE);
    }

    //
    // release lock
    //
    KeReleaseSpinLock(&FDODeviceExtension->IrpListLock, OldLevel);

}

VOID
USBSTOR_QueueNextRequest(
    IN PDEVICE_OBJECT DeviceObject)
{
    PFDO_DEVICE_EXTENSION FDODeviceExtension;
    PIRP Irp;
    PIO_STACK_LOCATION IoStack;
    PSCSI_REQUEST_BLOCK Request;

    //
    // get pdo device extension
    //
    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // sanity check
    //
    ASSERT(FDODeviceExtension->Common.IsFDO);

    //
    // check first if there's already a request pending or the queue is frozen
    //
    if (FDODeviceExtension->CurrentSrb != NULL ||
        FDODeviceExtension->IrpListFreeze)
    {
        //
        // no work to do yet
        //
        return;
    }

    //
    // remove first irp from list
    //
    Irp = USBSTOR_RemoveIrp(DeviceObject);

    //
    // is there an irp pending
    //
    if (!Irp)
    {
        //
        // no work to do
        //
        IoStartNextPacket(DeviceObject, TRUE);
        return;
    }

    //
    // get current stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // get srb
    //
    Request = IoStack->Parameters.Scsi.Srb;

    //
    // sanity check
    //
    ASSERT(Request);

    //
    // set the active SRB
    //
    FDODeviceExtension->CurrentSrb = Request;

    //
    // start next packet
    //
    IoStartPacket(DeviceObject, Irp, &Request->QueueSortKey, USBSTOR_CancelIo);

    //
    // start next request
    //
    IoStartNextPacket(DeviceObject, TRUE);
}

VOID
USBSTOR_QueueRelease(
    IN PDEVICE_OBJECT DeviceObject)
{
    PFDO_DEVICE_EXTENSION FDODeviceExtension;
    PIRP Irp;
    KIRQL OldLevel;
    PIO_STACK_LOCATION IoStack;
    PSCSI_REQUEST_BLOCK Request;

    //
    // get FDO device extension
    //
    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // sanity check
    //
    ASSERT(FDODeviceExtension->Common.IsFDO);

    //
    // acquire lock
    //
    KeAcquireSpinLock(&FDODeviceExtension->IrpListLock, &OldLevel);

    //
    // clear freezed status
    //
    FDODeviceExtension->IrpListFreeze = FALSE;

    //
    // release irp list lock
    //
    KeReleaseSpinLock(&FDODeviceExtension->IrpListLock, OldLevel);

    //
    // grab newest irp
    //
    Irp = USBSTOR_RemoveIrp(DeviceObject);

    //
    // is there an irp
    //
    if (!Irp)
    {
        //
        // no irp
        //
        return;
    }

    //
    // get current irp stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // get srb
    //
    Request = IoStack->Parameters.Scsi.Srb;

    //
    // start new packet
    //
    IoStartPacket(DeviceObject,
                  Irp,
                  &Request->QueueSortKey, 
                  USBSTOR_CancelIo);
}

VOID
NTAPI
USBSTOR_StartIo(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PFDO_DEVICE_EXTENSION FDODeviceExtension;
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    KIRQL OldLevel;
    BOOLEAN ResetInProgress;
    PSCSI_REQUEST_BLOCK Srb;

    DPRINT("USBSTOR_StartIo\n");

    //
    // get FDO device extension
    //
    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // sanity check
    //
    ASSERT(FDODeviceExtension->Common.IsFDO);

    //
    // acquire cancel spinlock
    //
    IoAcquireCancelSpinLock(&OldLevel);

    //
    // set cancel routine to zero
    //
    IoSetCancelRoutine(Irp, NULL);

    //
    // check if the irp has been cancelled
    //
    if (Irp->Cancel)
    {
        //
        // irp has been cancelled, release cancel spinlock
        //
        IoReleaseCancelSpinLock(OldLevel);

        //
        // irp is cancelled
        //
        Irp->IoStatus.Status = STATUS_CANCELLED;
        Irp->IoStatus.Information = 0;

        //
        // terminate request
        //
        USBSTOR_QueueTerminateRequest(DeviceObject, Irp);

        //
        // complete request
        //
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        //
        // queue next request
        //
        USBSTOR_QueueNextRequest(DeviceObject);

        //
        // done
        //
        return;
    }

    //
    // release cancel spinlock
    //
    IoReleaseCancelSpinLock(OldLevel);

    //
    // acquire lock
    //
    KeAcquireSpinLock(&FDODeviceExtension->IrpListLock, &OldLevel);

    //
    // check reset is in progress
    //
    ResetInProgress = FDODeviceExtension->ResetInProgress;
    ASSERT(ResetInProgress == FALSE);

    //
    // release lock
    //
    KeReleaseSpinLock(&FDODeviceExtension->IrpListLock, OldLevel);

    //
    // get current irp stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    Srb = IoStack->Parameters.Scsi.Srb;
    FDODeviceExtension->CurrentSrb = Srb;
    DPRINT("USBSTOR_StartIo: Srb - %p\n", Srb);

    //
    // get pdo device extension
    //
    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)IoStack->DeviceObject->DeviceExtension;

    //
    // sanity check
    //
    ASSERT(PDODeviceExtension->Common.IsFDO == FALSE);

    //
    // is a reset in progress
    //
    if (ResetInProgress)
    {
        //
        // hard reset is in progress
        //
        Srb->SrbStatus = SRB_STATUS_NO_DEVICE;
        Srb->DataTransferLength = 0;

        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_DEVICE_DOES_NOT_EXIST;

        USBSTOR_QueueTerminateRequest(DeviceObject, Irp);
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        USBSTOR_QueueNextRequest(DeviceObject);

        return;
    }

    //
    // execute scsi
    //
    USBSTOR_HandleExecuteSCSI(IoStack->DeviceObject, Irp, 0);

    //
    // FIXME: handle error
    //
}
