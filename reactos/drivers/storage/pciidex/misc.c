/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         PCI IDE bus driver extension
 * FILE:            drivers/storage/pciidex/misc.c
 * PURPOSE:         Misceallenous operations
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.org)
 */

#define NDEBUG
#include <debug.h>

#include "pciidex.h"

NTSTATUS NTAPI
PciIdeXGenericCompletion(
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
	PDEVICE_OBJECT LowerDevice;
	KEVENT Event;
	NTSTATUS Status;

	ASSERT(((PCOMMON_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->IsFDO);
	LowerDevice = ((PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->LowerDevice;
	ASSERT(LowerDevice);

	KeInitializeEvent(&Event, NotificationEvent, FALSE);
	IoCopyCurrentIrpStackLocationToNext(Irp);

	DPRINT("Calling lower device %p [%wZ]\n", LowerDevice, &LowerDevice->DriverObject->DriverName);
	IoSetCompletionRoutine(Irp, PciIdeXGenericCompletion, &Event, TRUE, TRUE, TRUE);

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
	PDEVICE_OBJECT LowerDevice;

	ASSERT(((PCOMMON_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->IsFDO);
	LowerDevice = ((PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->LowerDevice;
	ASSERT(LowerDevice);

	IoSkipCurrentIrpStackLocation(Irp);
	return IoCallDriver(LowerDevice, Irp);
}
