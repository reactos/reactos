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

VOID
USBSTOR_QueueInitialize(
    PFDO_DEVICE_EXTENSION FDODeviceExtension)
{

    //
    // initialize queue lock
    //
    KeInitializeSpinLock(&FDODeviceExtension->IrpListLock);

    //
    // initialize irp list head
    //
    InitializeListHead(&FDODeviceExtension->IrpListHead);
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
    // acquire irp list lock
    //
    KeAcquireSpinLockAtDpcLevel(&FDODeviceExtension->IrpListLock);

    //
    // now release the cancel lock
    //
    IoReleaseCancelSpinLock(Irp->CancelIrql);

    //
    // remove the irp from the list
    //
    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);

    //
    // release irp list lock
    //
    KeReleaseSpinLockFromDpcLevel(&FDODeviceExtension->IrpListLock);

    //
    // set cancel status
    //
    Irp->IoStatus.Status = STATUS_CANCELLED;

    //
    // now cancel the irp
    //
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}


BOOLEAN
USBSTOR_QueueAddIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PDRIVER_CANCEL OldDriverCancel;
    KIRQL OldLevel;
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    PFDO_DEVICE_EXTENSION FDODeviceExtension;
    BOOLEAN IrpListFreeze;

    //
    // get pdo device extension
    //
    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // get FDO device extension
    //
    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)PDODeviceExtension->LowerDeviceObject->DeviceExtension;

    //
    // mark irp pending
    //
    IoMarkIrpPending(Irp);

    //
    // acquire lock
    //
    KeAcquireSpinLock(&FDODeviceExtension->IrpListLock, &OldLevel);

    //
    // add irp to queue
    //
    InsertTailList(&FDODeviceExtension->IrpListHead, &Irp->Tail.Overlay.ListEntry);

    //
    // now set the driver cancel routine
    //
    OldDriverCancel = IoSetCancelRoutine(Irp, USBSTOR_CancelIo);

    //
    // check if the irp has already been cancelled
    //
    if (Irp->Cancel && OldDriverCancel == NULL)
    {
        //
        // the irp has already been cancelled
        //
        KeReleaseSpinLock(&FDODeviceExtension->IrpListLock, OldLevel);

        //
        // cancel routine requires that cancel spinlock is held
        //
        IoAcquireCancelSpinLock(&Irp->CancelIrql);

        //
        // cancel irp
        //
        USBSTOR_CancelIo(DeviceObject, Irp);

        //
        // irp was cancelled
        //
        return FALSE;
    }

    //
    // check if queue is freezed
    //
    IrpListFreeze = FDODeviceExtension->IrpListFreeze;

    //
    // release list lock
    //
    KeReleaseSpinLock(&FDODeviceExtension->IrpListLock, OldLevel);

    //
    // if list is freezed, dont start this packet
    //
    return IrpListFreeze;
}

PIRP
USBSTOR_RemoveIrp(
    IN PDEVICE_OBJECT DeviceObject)
{
    KIRQL OldLevel;
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    PFDO_DEVICE_EXTENSION FDODeviceExtension;
    PLIST_ENTRY Entry;
    PIRP Irp = NULL;

    //
    // get pdo device extension
    //
    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // sanity check
    //
    ASSERT(PDODeviceExtension->Common.IsFDO == FALSE);

    //
    // get FDO device extension
    //
    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)PDODeviceExtension->LowerDeviceObject->DeviceExtension;

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
USBSTOR_QueueFlushIrps(
    IN PDEVICE_OBJECT DeviceObject)
{
    KIRQL OldLevel;
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    PFDO_DEVICE_EXTENSION FDODeviceExtension;
    PLIST_ENTRY Entry;
    PIRP Irp;
    PIO_STACK_LOCATION IoStack;
    PSCSI_REQUEST_BLOCK Request;

    //
    // get pdo device extension
    //
    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // sanity check
    //
    ASSERT(PDODeviceExtension->Common.IsFDO == FALSE);

    //
    // get FDO device extension
    //
    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)PDODeviceExtension->LowerDeviceObject->DeviceExtension;

    //
    // acquire lock
    //
    KeAcquireSpinLock(&FDODeviceExtension->IrpListLock, &OldLevel);

    //
    // complete all irps with status cancelled
    // 
    while(!IsListEmpty(&FDODeviceExtension->IrpListHead))
    {
        //
        // remove irp
        //
        Entry = RemoveHeadList(&FDODeviceExtension->IrpListHead);

        //
        // get start of irp structure
        //
        Irp = (PIRP)CONTAINING_RECORD(Entry, IRP, Tail.Overlay.ListEntry);

        //
        // get current stack location
        //
        IoStack = IoGetCurrentIrpStackLocation(Irp);

        //
        // get request block
        //
        Request = (PSCSI_REQUEST_BLOCK)IoStack->Parameters.Others.Argument1;

        //
        // sanity check
        //
        ASSERT(Request);

        //
        // set srb status to flushed
        //
        Request->SrbStatus = SRB_STATUS_REQUEST_FLUSHED;

        //
        // set unsuccessful status
        //
        Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;

        //
        // complete request
        //
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
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
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    PIRP Irp;
    PIO_STACK_LOCATION IoStack;
    PSCSI_REQUEST_BLOCK Request;

    //
    // get pdo device extension
    //
    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // sanity check
    //
    ASSERT(PDODeviceExtension->Common.IsFDO == FALSE);

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
        return;
    }

    //
    // get current stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // get srb
    //
    Request = (PSCSI_REQUEST_BLOCK)IoStack->Parameters.Others.Argument1;

    //
    // start next packet
    //
    IoStartPacket(PDODeviceExtension->LowerDeviceObject, Irp, &Request->QueueSortKey, USBSTOR_CancelIo);
}

VOID
USBSTOR_QueueRelease(
    IN PDEVICE_OBJECT DeviceObject)
{
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    PFDO_DEVICE_EXTENSION FDODeviceExtension;
    PIRP Irp;
    KIRQL OldLevel;
    PIO_STACK_LOCATION IoStack;
    PSCSI_REQUEST_BLOCK Request;

    //
    // get pdo device extension
    //
    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // sanity check
    //
    ASSERT(PDODeviceExtension->Common.IsFDO == FALSE);

    //
    // get FDO device extension
    //
    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)PDODeviceExtension->LowerDeviceObject->DeviceExtension;

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
    Request = (PSCSI_REQUEST_BLOCK)IoStack->Parameters.Others.Argument1;

    //
    // start new packet
    //
    IoStartPacket(PDODeviceExtension->LowerDeviceObject, // FDO
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
    NTSTATUS Status;

    DPRINT1("USBSTOR_StartIo\n");

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
        // complete request
        //
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        //
        // check if the queue has been frozen
        //
        if (FDODeviceExtension->IrpListFreeze == FALSE)
        {
            //
            // queue next request
            //
            USBSTOR_QueueNextRequest(DeviceObject);

            //
            // start next request
            //
            IoStartNextPacket(DeviceObject, TRUE);
        }

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
    // remove irp from list
    //
    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);

    //
    // release lock
    //
    KeReleaseSpinLock(&FDODeviceExtension->IrpListLock, OldLevel);

    //
    // get current irp stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // get pdo device extension
    //
    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)IoStack->DeviceObject->DeviceExtension;

    //
    // sanity check
    //
    ASSERT(PDODeviceExtension->Common.IsFDO == FALSE);

    //
    // execute scsi
    //
    Status = USBSTOR_HandleExecuteSCSI(IoStack->DeviceObject, Irp);

    //
    // FIXME: synchronize action with error handling
    //
    USBSTOR_QueueNextRequest(IoStack->DeviceObject);

    //
    // start next request
    //
    IoStartNextPacket(DeviceObject, TRUE);

}
