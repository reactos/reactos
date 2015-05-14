/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Parallel Port Function Driver
 * FILE:            drivers/parallel/parport/misc.c
 * PURPOSE:         Miscellaneous functions
 */

#include "parport.h"


static IO_COMPLETION_ROUTINE ForwardIrpAndWaitCompletion;

/* FUNCTIONS ****************************************************************/

static
NTSTATUS
NTAPI
ForwardIrpAndWaitCompletion(IN PDEVICE_OBJECT DeviceObject,
                            IN PIRP Irp,
                            IN PVOID Context)
{
    if (Irp->PendingReturned)
        KeSetEvent((PKEVENT)Context, IO_NO_INCREMENT, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
ForwardIrpAndWait(IN PDEVICE_OBJECT DeviceObject,
                  IN PIRP Irp)
{
    PDEVICE_OBJECT LowerDevice;
    KEVENT Event;
    NTSTATUS Status;

    LowerDevice = ((PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->LowerDevice;
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
ForwardIrpAndForget(IN PDEVICE_OBJECT DeviceObject,
                    IN PIRP Irp)
{
    PDEVICE_OBJECT LowerDevice;

    LowerDevice = ((PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->LowerDevice;
    ASSERT(LowerDevice);

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(LowerDevice, Irp);
}


PVOID
GetUserBuffer(IN PIRP Irp)
{
    ASSERT(Irp);

    if (Irp->MdlAddress)
        return Irp->MdlAddress;
    else
        return Irp->AssociatedIrp.SystemBuffer;
}

/* EOF */
