/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Enhanced Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbccgp/misc.c
 * PURPOSE:     USB  device driver.
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 *              Cameron Gutman
 */

#include "usbccgp.h"

//
// driver verifier
//
IO_COMPLETION_ROUTINE SyncForwardIrpCompletionRoutine;

NTSTATUS
NTAPI
USBSTOR_SyncForwardIrpCompletionRoutine(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp, 
    PVOID Context)
{
    if (Irp->PendingReturned)
    {
        KeSetEvent((PKEVENT)Context, IO_NO_INCREMENT, FALSE);
    }
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
NTAPI
USBCCGP_SyncForwardIrp(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    KEVENT Event;
    NTSTATUS Status;

    //
    // initialize event
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    //
    // copy irp stack location
    //
    IoCopyCurrentIrpStackLocationToNext(Irp);

    //
    // set completion routine
    //
    IoSetCompletionRoutine(Irp, USBSTOR_SyncForwardIrpCompletionRoutine, &Event, TRUE, TRUE, TRUE);


    //
    // call driver
    //
    Status = IoCallDriver(DeviceObject, Irp);

    //
    // check if pending
    //
    if (Status == STATUS_PENDING)
    {
        //
        // wait for the request to finish
        //
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);

        //
        // copy status code
        //
        Status = Irp->IoStatus.Status;
    }

    //
    // done
    //
    return Status;
}

NTSTATUS
USBCCGP_SyncUrbRequest(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PURB UrbRequest)
{
    PIRP Irp;
    PIO_STACK_LOCATION IoStack;
    KEVENT Event;
    NTSTATUS Status;

    //
    // allocate irp
    //
    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    if (!Irp)
    {
        //
        // no memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // initialize event
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);


    //
    // get next stack location
    //
    IoStack = IoGetNextIrpStackLocation(Irp);

    //
    // initialize stack location
    //
    IoStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    IoStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;
    IoStack->Parameters.Others.Argument1 = (PVOID)UrbRequest;
    IoStack->Parameters.DeviceIoControl.InputBufferLength = UrbRequest->UrbHeader.Length;
    Irp->IoStatus.Status = STATUS_SUCCESS;

    //
    // setup completion routine
    //
    IoSetCompletionRoutine(Irp, USBSTOR_SyncForwardIrpCompletionRoutine, &Event, TRUE, TRUE, TRUE);

    //
    // call driver
    //
    Status = IoCallDriver(DeviceObject, Irp);

    //
    // check if request is pending
    //
    if (Status == STATUS_PENDING)
    {
        //
        // wait for completion
        //
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);

        //
        // update status
        //
        Status = Irp->IoStatus.Status;
    }

    //
    // free irp
    //
    IoFreeIrp(Irp);

    //
    // done
    //
    return Status;
}

PVOID
AllocateItem(
    IN POOL_TYPE PoolType,
    IN ULONG ItemSize)
{
    //
    // allocate item
    //
    PVOID Item = ExAllocatePoolWithTag(PoolType, ItemSize, USBCCPG_TAG);

    if (Item)
    {
        //
        // zero item
        //
        RtlZeroMemory(Item, ItemSize);
    }

    //
    // return element
    //
    return Item;
}

VOID
FreeItem(
    IN PVOID Item)
{
    //
    // free item
    //
    ExFreePoolWithTag(Item, USBCCPG_TAG);
}

