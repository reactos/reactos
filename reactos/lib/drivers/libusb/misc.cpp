/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Driver Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/drivers/libusb/misc.cpp
 * PURPOSE:     USB Common Driver Library.
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "libusb.h"

#define NDEBUG
#include <debug.h>

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

NTSTATUS
NTAPI
GetBusInterface(
    PDEVICE_OBJECT DeviceObject, 
    PBUS_INTERFACE_STANDARD busInterface)
{
    KEVENT Event;
    NTSTATUS Status;
    PIRP Irp;
    IO_STATUS_BLOCK IoStatus;
    PIO_STACK_LOCATION Stack;

    if ((!DeviceObject) || (!busInterface))
        return STATUS_UNSUCCESSFUL;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP,
                                       DeviceObject,
                                       NULL,
                                       0,
                                       NULL,
                                       &Event,
                                       &IoStatus);

    if (Irp == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Stack=IoGetNextIrpStackLocation(Irp);
    Stack->MajorFunction = IRP_MJ_PNP;
    Stack->MinorFunction = IRP_MN_QUERY_INTERFACE;
    Stack->Parameters.QueryInterface.Size = sizeof(BUS_INTERFACE_STANDARD);
    Stack->Parameters.QueryInterface.InterfaceType = (LPGUID)&GUID_BUS_INTERFACE_STANDARD;
    Stack->Parameters.QueryInterface.Version = 1;
    Stack->Parameters.QueryInterface.Interface = (PINTERFACE)busInterface;
    Stack->Parameters.QueryInterface.InterfaceSpecificData = NULL;
    Irp->IoStatus.Status=STATUS_NOT_SUPPORTED ;

    Status=IoCallDriver(DeviceObject, Irp);

    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);

        Status=IoStatus.Status;
    }

    return Status;
}

