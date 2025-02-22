/*
 * PROJECT:     ReactOS VT100 emulator
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/base/green/misc.c
 * PURPOSE:     Misceallenous operations
 * PROGRAMMERS: Copyright 2005-2006 Hervé Poussineau (hpoussin@reactos.org)
 */

#include "green.h"

#define NDEBUG
#include <debug.h>

NTSTATUS
GreenDeviceIoControl(
	IN PDEVICE_OBJECT DeviceObject,
	IN ULONG CtlCode,
	IN PVOID InputBuffer OPTIONAL,
	IN ULONG InputBufferSize,
	IN OUT PVOID OutputBuffer OPTIONAL,
	IN OUT PULONG OutputBufferSize)
{
	KEVENT Event;
	PIRP Irp;
	IO_STATUS_BLOCK IoStatus;
	NTSTATUS Status;

	KeInitializeEvent (&Event, NotificationEvent, FALSE);

	Irp = IoBuildDeviceIoControlRequest(CtlCode,
		DeviceObject,
		InputBuffer,
		InputBufferSize,
		OutputBuffer,
		(OutputBufferSize) ? *OutputBufferSize : 0,
		FALSE,
		&Event,
		&IoStatus);
	if (Irp == NULL)
	{
		DPRINT("IoBuildDeviceIoControlRequest() failed\n");
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	Status = IoCallDriver(DeviceObject, Irp);

	if (Status == STATUS_PENDING)
	{
		DPRINT("Operation pending\n");
		KeWaitForSingleObject(&Event, Suspended, KernelMode, FALSE, NULL);
		Status = IoStatus.Status;
	}

	if (OutputBufferSize)
	{
		*OutputBufferSize = IoStatus.Information;
	}

	return Status;
}

NTSTATUS
ReadRegistryEntries(
	IN PUNICODE_STRING RegistryPath,
	IN PGREEN_DRIVER_EXTENSION DriverExtension)
{
	UNICODE_STRING ParametersRegistryKey;
	RTL_QUERY_REGISTRY_TABLE Parameters[4];
	NTSTATUS Status;

	ULONG DefaultDeviceReported = 0;
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

	Parameters[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
	Parameters[0].Name = L"AttachedDevice";
	Parameters[0].EntryContext = &DriverExtension->AttachedDeviceName;

	Parameters[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
	Parameters[1].Name = L"DeviceReported";
	Parameters[1].EntryContext = &DriverExtension->DeviceReported;
	Parameters[1].DefaultType = REG_DWORD;
	Parameters[1].DefaultData = &DefaultDeviceReported;
	Parameters[1].DefaultLength = sizeof(ULONG);

	Parameters[2].Flags = RTL_QUERY_REGISTRY_DIRECT;
	Parameters[2].Name = L"SampleRate";
	Parameters[2].EntryContext = &DriverExtension->SampleRate;
	Parameters[2].DefaultType = REG_DWORD;
	Parameters[2].DefaultData = &DefaultSampleRate;
	Parameters[2].DefaultLength = sizeof(ULONG);

	Status = RtlQueryRegistryValues(
		RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
		ParametersRegistryKey.Buffer,
		Parameters,
		NULL,
		NULL);

	return Status;
}
