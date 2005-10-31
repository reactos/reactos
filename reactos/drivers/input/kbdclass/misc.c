/* 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Keyboard class driver
 * FILE:            drivers/input/kbdclass/misc.c
 * PURPOSE:         Misceallenous operations
 * 
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.org)
 */

#define NDEBUG
#include <debug.h>

#include "kbdclass.h"

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
	PDEVICE_OBJECT LowerDevice = ((PKBDCLASS_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->LowerDevice;
	KEVENT Event;
	NTSTATUS Status;
	
	KeInitializeEvent(&Event, NotificationEvent, FALSE);
	IoCopyCurrentIrpStackLocationToNext(Irp);
	
	DPRINT("Calling lower device %p [%wZ]\n", LowerDevice, &LowerDevice->DriverObject->DriverName);
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
	PDEVICE_OBJECT LowerDevice = ((PKBDCLASS_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->LowerDevice;
	
	IoSkipCurrentIrpStackLocation(Irp);
	return IoCallDriver(LowerDevice, Irp);
}
