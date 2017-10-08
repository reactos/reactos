/*
 * PROJECT:        ReactOS Generic CPU Driver
 * LICENSE:        GNU GPLv2 only as published by the Free Software Foundation
 * FILE:           drivers/processor/processr/misc.c
 * PURPOSE:        Misc routines
 * PROGRAMMERS:    Eric Kohl <eric.kohl@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include "processr.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

static IO_COMPLETION_ROUTINE ForwardIrpAndWaitCompletion;

static
NTSTATUS
NTAPI
ForwardIrpAndWaitCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context)
{
    if (Irp->PendingReturned)
        KeSetEvent((PKEVENT)Context, IO_NO_INCREMENT, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
NTAPI
ForwardIrpAndWait(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PDEVICE_OBJECT LowerDevice;
    KEVENT Event;
    NTSTATUS Status;

    LowerDevice = ((PDEVICE_EXTENSION)DeviceObject->DeviceExtension)->LowerDevice;
    ASSERT(LowerDevice);

    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    IoCopyCurrentIrpStackLocationToNext(Irp);

    DPRINT("Calling lower device %p\n", LowerDevice);
    IoSetCompletionRoutine(Irp, ForwardIrpAndWaitCompletion, &Event, TRUE, TRUE, TRUE);

    Status = IoCallDriver(LowerDevice, Irp);
    if (Status == STATUS_PENDING)
    {
        Status = KeWaitForSingleObject(&Event, Suspended, KernelMode, FALSE, NULL);
        if (NT_SUCCESS(Status))
            Status = Irp->IoStatus.Status;
    }

    return Status;
}


NTSTATUS
NTAPI
ForwardIrpAndForget(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PDEVICE_OBJECT LowerDevice;

    LowerDevice = ((PDEVICE_EXTENSION)DeviceObject->DeviceExtension)->LowerDevice;
    ASSERT(LowerDevice);

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(LowerDevice, Irp);
}

/* EOF */
