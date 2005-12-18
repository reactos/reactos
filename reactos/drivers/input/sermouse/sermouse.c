/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Serial mouse driver
 * FILE:            drivers/input/sermouse/sermouse.c
 * PURPOSE:         Serial mouse driver entry point
 *
  * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.org)
 */

#define NDEBUG
#include <debug.h>

#define INITGUID
#include "sermouse.h"

VOID NTAPI
DriverUnload(IN PDRIVER_OBJECT DriverObject)
{
	// nothing to do here yet
}

NTSTATUS NTAPI
IrpStub(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	DPRINT1("Irp stub for major function 0x%lx\n",
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
	RTL_QUERY_REGISTRY_TABLE Parameters[5];
	NTSTATUS Status;

	ULONG DefaultMouseDataQueueSize = 0x64;
	ULONG DefaultNumberOfButtons = 0;
	UNICODE_STRING DefaultPointerDeviceBaseName = RTL_CONSTANT_STRING(L"PointerPort");
	ULONG DefaultSampleRate = 1200;

	ParametersRegistryKey.Length = 0;
	ParametersRegistryKey.MaximumLength = RegistryPath->Length + sizeof(L"\\Parameters") + sizeof(UNICODE_NULL);
	ParametersRegistryKey.Buffer = ExAllocatePool(PagedPool, ParametersRegistryKey.MaximumLength);
	if (!ParametersRegistryKey.Buffer)
	{
		DPRINT("ExAllocatePool() failed\n");
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	RtlCopyUnicodeString(&ParametersRegistryKey, RegistryPath);
	RtlAppendUnicodeToString(&ParametersRegistryKey, L"\\Parameters");
	ParametersRegistryKey.Buffer[ParametersRegistryKey.Length / sizeof(WCHAR)] = UNICODE_NULL;

	RtlZeroMemory(Parameters, sizeof(Parameters));

	Parameters[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_REGISTRY_OPTIONAL;
	Parameters[0].Name = L"MouseDataQueueSize";
	Parameters[0].EntryContext = &DriverExtension->MouseDataQueueSize;
	Parameters[0].DefaultType = REG_DWORD;
	Parameters[0].DefaultData = &DefaultMouseDataQueueSize;
	Parameters[0].DefaultLength = sizeof(ULONG);

	Parameters[1].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_REGISTRY_OPTIONAL;
	Parameters[1].Name = L"NumberOfButtons";
	Parameters[1].EntryContext = &DriverExtension->NumberOfButtons;
	Parameters[1].DefaultType = REG_DWORD;
	Parameters[1].DefaultData = &DefaultNumberOfButtons;
	Parameters[1].DefaultLength = sizeof(ULONG);

	Parameters[2].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_REGISTRY_OPTIONAL;
	Parameters[2].Name = L"PointerDeviceBaseName";
	Parameters[2].EntryContext = &DriverExtension->PointerDeviceBaseName;
	Parameters[2].DefaultType = REG_SZ;
	Parameters[2].DefaultData = &DefaultPointerDeviceBaseName;
	Parameters[2].DefaultLength = 0;

	Parameters[3].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_REGISTRY_OPTIONAL;
	Parameters[3].Name = L"SampleRate";
	Parameters[3].EntryContext = &DriverExtension->SampleRate;
	Parameters[3].DefaultType = REG_DWORD;
	Parameters[3].DefaultData = &DefaultSampleRate;
	Parameters[3].DefaultLength = sizeof(ULONG);

	Status = RtlQueryRegistryValues(
		RTL_REGISTRY_ABSOLUTE,
		ParametersRegistryKey.Buffer,
		Parameters,
		NULL,
		NULL);

	if (NT_SUCCESS(Status))
	{
		/* Check values */
		if (DriverExtension->MouseDataQueueSize == 0)
		{
			DriverExtension->MouseDataQueueSize = DefaultMouseDataQueueSize;
		}
	}
	else if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
	{
		/* Registry path doesn't exist. Set defaults */
		DriverExtension->MouseDataQueueSize = DefaultMouseDataQueueSize;
		DriverExtension->NumberOfButtons = DefaultNumberOfButtons;
		Status = RtlDuplicateUnicodeString(
			RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE,
			&DefaultPointerDeviceBaseName,
			&DriverExtension->PointerDeviceBaseName);
		DriverExtension->SampleRate = DefaultSampleRate;
	}

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
		DPRINT("IoAllocateDriverObjectExtension() failed with status 0x%08lx\n", Status);
		return Status;
	}
	RtlZeroMemory(DriverExtension, sizeof(SERMOUSE_DRIVER_EXTENSION));

	Status = ReadRegistryEntries(RegistryPath, DriverExtension);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("ReadRegistryEntries() failed with status 0x%08lx\n", Status);
		return Status;
	}

	DriverObject->DriverUnload = DriverUnload;
	DriverObject->DriverExtension->AddDevice = SermouseAddDevice;

	for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
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
