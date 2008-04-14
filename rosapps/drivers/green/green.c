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

	Status = RtlDuplicateUnicodeString(
		0,
		RegistryPath,
		&DriverExtension->RegistryPath);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("RtlDuplicateUnicodeString() failed with status 0x%08lx\n", Status);
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
