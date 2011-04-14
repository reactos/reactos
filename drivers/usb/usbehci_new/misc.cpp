/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Enhanced Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbehci/misc.cpp
 * PURPOSE:     USB EHCI device driver.
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "usbehci.h"

//
// driver verifier
//
IO_COMPLETION_ROUTINE SyncForwardIrpCompletionRoutine;

NTSTATUS
NTAPI
SyncForwardIrpCompletionRoutine(
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
SyncForwardIrp(PDEVICE_OBJECT DeviceObject, PIRP Irp)
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
    IoSetCompletionRoutine(Irp, SyncForwardIrpCompletionRoutine, &Event, TRUE, TRUE, TRUE);


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

