/*
 * PROJECT:     ReactOS VT100 emulator
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/base/green/green.c
 * PURPOSE:     Driver entry point
 * PROGRAMMERS: Copyright 2005-2006 Hervé Poussineau (hpoussin@reactos.org)
 */

#include "green.h"

#define NDEBUG
#include <debug.h>

VOID NTAPI
DriverUnload(IN PDRIVER_OBJECT DriverObject)
{
	// nothing to do here yet
}

/*
 * Standard DriverEntry method.
 */
NTSTATUS NTAPI
DriverEntry(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath)
{
	PGREEN_DRIVER_EXTENSION DriverExtension;
	ULONG i;
	NTSTATUS Status;

	Status = IoAllocateDriverObjectExtension(
		DriverObject,
		DriverObject,
		sizeof(GREEN_DRIVER_EXTENSION),
		(PVOID*)&DriverExtension);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("IoAllocateDriverObjectExtension() failed with status 0x%08lx\n", Status);
		return Status;
	}
	RtlZeroMemory(DriverExtension, sizeof(GREEN_DRIVER_EXTENSION));

	Status = GreenDuplicateUnicodeString(
		0,
		RegistryPath,
		&DriverExtension->RegistryPath);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("GreenDuplicateUnicodeString() failed with status 0x%08lx\n", Status);
		return Status;
	}

	Status = ReadRegistryEntries(RegistryPath, DriverExtension);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("ReadRegistryEntries() failed with status 0x%08lx\n", Status);
		return Status;
	}

	DriverObject->DriverUnload = DriverUnload;
	DriverObject->DriverExtension->AddDevice = GreenAddDevice;

	for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
		DriverObject->MajorFunction[i] = GreenDispatch;

	return STATUS_SUCCESS;
}

NTSTATUS
GreenDuplicateUnicodeString(
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
