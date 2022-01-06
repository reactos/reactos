/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         PCI IDE bus driver extension
 * FILE:            drivers/storage/pciidex/misc.c
 * PURPOSE:         Miscellaneous operations
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.org)
 */

#include "pciidex.h"

#define NDEBUG
#include <debug.h>


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

		DestinationString->Buffer = ExAllocatePool(PagedPool, DestMaxLength);
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
