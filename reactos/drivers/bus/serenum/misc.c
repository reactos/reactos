/* $Id:
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Serial enumerator driver
 * FILE:            drivers/dd/serenum/misc.c
 * PURPOSE:         Misceallenous operations
 *
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.com)
 */

#define NDEBUG
#include "serenum.h"
#include <stdarg.h>

NTSTATUS
SerenumDuplicateUnicodeString(
	OUT PUNICODE_STRING Destination,
	IN PUNICODE_STRING Source,
	IN POOL_TYPE PoolType)
{
	ASSERT(Destination);

	if (Source == NULL)
	{
		RtlInitUnicodeString(Destination, NULL);
		return STATUS_SUCCESS;
	}

	Destination->Buffer = ExAllocatePool(PoolType, Source->MaximumLength);
	if (Destination->Buffer == NULL)
	{
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	Destination->MaximumLength = Source->MaximumLength;
	Destination->Length = Source->Length;
	RtlCopyMemory(Destination->Buffer, Source->Buffer, Source->MaximumLength);

	return STATUS_SUCCESS;
}

/* I really want PCSZ strings as last arguments because
 * PnP ids are ANSI-encoded in PnP device string
 * identification */
NTSTATUS
SerenumInitMultiSzString(
	OUT PUNICODE_STRING Destination,
	... /* list of PCSZ */)
{
	va_list args;
	PCSZ Source;
	ANSI_STRING AnsiString;
	UNICODE_STRING UnicodeString;
	ULONG DestinationSize = 0;
	NTSTATUS Status = STATUS_SUCCESS;

	ASSERT(Destination);

	/* Calculate length needed for destination unicode string */
	va_start(args, Destination);
	Source = va_arg(args, PCSZ);
	while (Source != NULL)
	{
		RtlInitAnsiString(&AnsiString, Source);
		DestinationSize += RtlAnsiStringToUnicodeSize(&AnsiString)
			+ sizeof(WCHAR) /* final NULL */;
		Source = va_arg(args, PCSZ);
	}
	va_end(args);
	if (DestinationSize == 0)
	{
		RtlInitUnicodeString(Destination, NULL);
		return STATUS_SUCCESS;
	}

	/* Initialize destination string */
	DestinationSize += sizeof(WCHAR); // final NULL
	Destination->Buffer = (PWSTR)ExAllocatePoolWithTag(PagedPool, DestinationSize, SERENUM_TAG);
	if (!Destination->Buffer)
		return STATUS_INSUFFICIENT_RESOURCES;
	Destination->Length = 0;
	Destination->MaximumLength = (USHORT)DestinationSize;

	/* Copy arguments to destination string */
	/* Use a temporary unicode string, which buffer is shared with
	 * destination string, to copy arguments */
	UnicodeString.Length = Destination->Length;
	UnicodeString.MaximumLength = Destination->MaximumLength;
	UnicodeString.Buffer = Destination->Buffer;
	va_start(args, Destination);
	Source = va_arg(args, PCSZ);
	while (Source != NULL)
	{
		RtlInitAnsiString(&AnsiString, Source);
		Status = RtlAnsiStringToUnicodeString(&UnicodeString, &AnsiString, FALSE);
		if (!NT_SUCCESS(Status))
		{
			ExFreePoolWithTag(Destination->Buffer, SERENUM_TAG);
			break;
		}
		Destination->Length += UnicodeString.Length + sizeof(WCHAR);
		UnicodeString.MaximumLength -= UnicodeString.Length + sizeof(WCHAR);
		UnicodeString.Buffer += UnicodeString.Length / sizeof(WCHAR) + 1;
		UnicodeString.Length = 0;
		Source = va_arg(args, PCSZ);
	}
	va_end(args);
	if (NT_SUCCESS(Status))
	{
		/* Finish multi-sz string */
		Destination->Buffer[Destination->Length / sizeof(WCHAR)] = L'\0';
		Destination->Length += sizeof(WCHAR);
	}
	return Status;
}

NTSTATUS STDCALL
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
	PDEVICE_OBJECT LowerDevice;
	KEVENT Event;
	NTSTATUS Status;

	ASSERT(((PCOMMON_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->IsFDO);
	LowerDevice = ((PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->LowerDevice;

	ASSERT(LowerDevice);

	KeInitializeEvent(&Event, NotificationEvent, FALSE);
	IoCopyCurrentIrpStackLocationToNext(Irp);

	DPRINT("Serenum: Calling lower device %p [%wZ]\n", LowerDevice, &LowerDevice->DriverObject->DriverName);
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

NTSTATUS STDCALL
ForwardIrpToLowerDeviceAndForget(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	PFDO_DEVICE_EXTENSION DeviceExtension;
	PDEVICE_OBJECT LowerDevice;

	DeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	ASSERT(DeviceExtension->Common.IsFDO);

	LowerDevice = DeviceExtension->LowerDevice;
	ASSERT(LowerDevice);
	DPRINT("Serenum: calling lower device 0x%p [%wZ]\n",
		LowerDevice, &LowerDevice->DriverObject->DriverName);
	IoSkipCurrentIrpStackLocation(Irp);
	return IoCallDriver(LowerDevice, Irp);
}

NTSTATUS STDCALL
ForwardIrpToAttachedFdoAndForget(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	PPDO_DEVICE_EXTENSION DeviceExtension;
	PDEVICE_OBJECT Fdo;

	DeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	ASSERT(!DeviceExtension->Common.IsFDO);

	Fdo = DeviceExtension->AttachedFdo;
	ASSERT(Fdo);
	DPRINT("Serenum: calling attached Fdo 0x%p [%wZ]\n",
		Fdo, &Fdo->DriverObject->DriverName);
	IoSkipCurrentIrpStackLocation(Irp);
	return IoCallDriver(Fdo, Irp);
}

NTSTATUS STDCALL
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
