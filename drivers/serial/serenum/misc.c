/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Serial enumerator driver
 * FILE:            drivers/dd/serenum/misc.c
 * PURPOSE:         Miscellaneous operations
 *
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.com)
 */

#include "serenum.h"

#include <debug.h>

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

NTSTATUS NTAPI
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
	TRACE_(SERENUM, "Calling lower device 0x%p\n", LowerDevice);
	IoSkipCurrentIrpStackLocation(Irp);
	return IoCallDriver(LowerDevice, Irp);
}

NTSTATUS NTAPI
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
	TRACE_(SERENUM, "Calling attached Fdo 0x%p\n", Fdo);
	IoSkipCurrentIrpStackLocation(Irp);
	return IoCallDriver(Fdo, Irp);
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

NTSTATUS
DuplicateUnicodeString(
	IN ULONG Flags,
	IN PCUNICODE_STRING SourceString,
	OUT PUNICODE_STRING DestinationString)
{
	if (SourceString == NULL || DestinationString == NULL
	 || SourceString->Length > SourceString->MaximumLength
	 || (SourceString->Length == 0 && SourceString->MaximumLength > 0 && SourceString->Buffer == NULL)
	 || Flags == RTL_DUPLICATE_UNICODE_STRING_ALLOCATE_NULL_STRING || Flags >= 4)
	{
		return STATUS_INVALID_PARAMETER;
	}


	if ((SourceString->Length == 0)
	 && (Flags != (RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE |
	               RTL_DUPLICATE_UNICODE_STRING_ALLOCATE_NULL_STRING)))
	{
		DestinationString->Length = 0;
		DestinationString->MaximumLength = 0;
		DestinationString->Buffer = NULL;
	}
	else
	{
		USHORT DestMaxLength = SourceString->Length;

		if (Flags & RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE)
			DestMaxLength += sizeof(UNICODE_NULL);

		DestinationString->Buffer = ExAllocatePoolWithTag(PagedPool, DestMaxLength, SERENUM_TAG);
		if (DestinationString->Buffer == NULL)
			return STATUS_NO_MEMORY;

		RtlCopyMemory(DestinationString->Buffer, SourceString->Buffer, SourceString->Length);
		DestinationString->Length = SourceString->Length;
		DestinationString->MaximumLength = DestMaxLength;

		if (Flags & RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE)
			DestinationString->Buffer[DestinationString->Length / sizeof(WCHAR)] = 0;
	}

	return STATUS_SUCCESS;
}
