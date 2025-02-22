/*
 * PROJECT:     ReactOS Serial mouse driver
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/input/sermouse/fdo.c
 * PURPOSE:     Serial mouse driver entry point
 * PROGRAMMERS: Copyright 2005-2006 Hervé Poussineau (hpoussin@reactos.org)
 */

#include "sermouse.h"

#include <debug.h>

static DRIVER_UNLOAD DriverUnload;
static DRIVER_DISPATCH IrpStub;
DRIVER_INITIALIZE DriverEntry;

static VOID NTAPI
DriverUnload(IN PDRIVER_OBJECT DriverObject)
{
	// nothing to do here yet
}

static NTSTATUS NTAPI
IrpStub(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	ERR_(SERMOUSE, "Irp stub for major function 0x%lx\n",
		IoGetCurrentIrpStackLocation(Irp)->MajorFunction);
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_NOT_SUPPORTED;
}

static NTSTATUS
ReadRegistryEntries(
	IN PUNICODE_STRING RegistryPath,
	IN PSERMOUSE_DRIVER_EXTENSION DriverExtension)
{
	UNICODE_STRING ParametersRegistryKey;
	RTL_QUERY_REGISTRY_TABLE Parameters[2];
	NTSTATUS Status;

	ULONG DefaultNumberOfButtons = 2;

	ParametersRegistryKey.Length = 0;
	ParametersRegistryKey.MaximumLength = RegistryPath->Length + sizeof(L"\\Parameters") + sizeof(UNICODE_NULL);
	ParametersRegistryKey.Buffer = ExAllocatePoolWithTag(PagedPool, ParametersRegistryKey.MaximumLength, SERMOUSE_TAG);
	if (!ParametersRegistryKey.Buffer)
	{
		WARN_(SERMOUSE, "ExAllocatePoolWithTag() failed\n");
		return STATUS_NO_MEMORY;
	}
	RtlCopyUnicodeString(&ParametersRegistryKey, RegistryPath);
	RtlAppendUnicodeToString(&ParametersRegistryKey, L"\\Parameters");
	ParametersRegistryKey.Buffer[ParametersRegistryKey.Length / sizeof(WCHAR)] = UNICODE_NULL;

	RtlZeroMemory(Parameters, sizeof(Parameters));

	Parameters[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
	Parameters[0].Name = L"NumberOfButtons";
	Parameters[0].EntryContext = &DriverExtension->NumberOfButtons;
	Parameters[0].DefaultType = REG_DWORD;
	Parameters[0].DefaultData = &DefaultNumberOfButtons;
	Parameters[0].DefaultLength = sizeof(ULONG);

	Status = RtlQueryRegistryValues(
		RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
		ParametersRegistryKey.Buffer,
		Parameters,
		NULL,
		NULL);

	if (NT_SUCCESS(Status))
	{
		/* Check values */
	}
	else if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
	{
		/* Registry path doesn't exist. Set defaults */
		DriverExtension->NumberOfButtons = (USHORT)DefaultNumberOfButtons;
		Status = STATUS_SUCCESS;
	}

	ExFreePoolWithTag(ParametersRegistryKey.Buffer, SERMOUSE_TAG);
	return Status;
}

/*
 * Standard DriverEntry method.
 */
NTSTATUS NTAPI
DriverEntry(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath)
{
	PSERMOUSE_DRIVER_EXTENSION DriverExtension;
	ULONG i;
	NTSTATUS Status;

	Status = IoAllocateDriverObjectExtension(
		DriverObject,
		DriverObject,
		sizeof(SERMOUSE_DRIVER_EXTENSION),
		(PVOID*)&DriverExtension);
	if (!NT_SUCCESS(Status))
	{
		WARN_(SERMOUSE, "IoAllocateDriverObjectExtension() failed with status 0x%08lx\n", Status);
		return Status;
	}
	RtlZeroMemory(DriverExtension, sizeof(SERMOUSE_DRIVER_EXTENSION));

	Status = ReadRegistryEntries(RegistryPath, DriverExtension);
	if (!NT_SUCCESS(Status))
	{
		WARN_(SERMOUSE, "ReadRegistryEntries() failed with status 0x%08lx\n", Status);
		return Status;
	}

	DriverObject->DriverUnload = DriverUnload;
	DriverObject->DriverExtension->AddDevice = SermouseAddDevice;

	for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
		DriverObject->MajorFunction[i] = IrpStub;

	DriverObject->MajorFunction[IRP_MJ_CREATE] = SermouseCreate;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = SermouseClose;
	DriverObject->MajorFunction[IRP_MJ_CLEANUP] = SermouseCleanup;
	//DriverObject->MajorFunction[IRP_MJ_READ] = SermouseRead;
	//DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = SermouseDeviceControl;
	DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = SermouseInternalDeviceControl;
	//DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] = SermouseQueryInformation;
	DriverObject->MajorFunction[IRP_MJ_PNP] = SermousePnp;
	//DriverObject->MajorFunction[IRP_MJ_POWER] = SermousePower;

	return STATUS_SUCCESS;
}
