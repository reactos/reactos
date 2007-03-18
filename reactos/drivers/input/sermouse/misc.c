/*
 * PROJECT:     ReactOS Serial mouse driver
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/input/sermouse/fdo.c
 * PURPOSE:     Misceallenous operations
 * PROGRAMMERS: Copyright 2005-2006 Hervé Poussineau (hpoussin@reactos.org)
 */

#include "sermouse.h"

static IO_COMPLETION_ROUTINE ForwardIrpAndWaitCompletion;

static NTSTATUS NTAPI
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
ForwardIrpAndWait(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	PDEVICE_OBJECT LowerDevice = ((PSERMOUSE_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->LowerDevice;
	KEVENT Event;
	NTSTATUS Status;

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

NTSTATUS NTAPI
ForwardIrpAndForget(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	PDEVICE_OBJECT LowerDevice = ((PSERMOUSE_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->LowerDevice;

	IoSkipCurrentIrpStackLocation(Irp);
	return IoCallDriver(LowerDevice, Irp);
}
