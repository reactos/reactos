/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/legacy/stream/helper.c
 * PURPOSE:         irp helper routines
 * PROGRAMMER:      Johannes Anderwald
 */


#include "stream.h"

NTSTATUS
NTAPI
CompletionRoutine(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp,
    IN PVOID  Context)
{
    if (Irp->PendingReturned == TRUE)
    {
        KeSetEvent ((PKEVENT) Context, IO_NO_INCREMENT, FALSE);
    }
    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
NTAPI
ForwardIrpSynchronous(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    KEVENT Event;
    PSTREAM_DEVICE_EXTENSION DeviceExt;
    NTSTATUS Status;

    DeviceExt = (PSTREAM_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    /* initialize the notification event */
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    IoCopyCurrentIrpStackLocationToNext(Irp);

    IoSetCompletionRoutine(Irp, CompletionRoutine, (PVOID)&Event, TRUE, TRUE, TRUE);

    /* now call the driver */
    Status = IoCallDriver(DeviceExt->LowerDeviceObject, Irp);
    /* did the request complete yet */
    if (Status == STATUS_PENDING)
    {
        /* not yet, lets wait a bit */
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = Irp->IoStatus.Status;
    }
    return Status;
}
