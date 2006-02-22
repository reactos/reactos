/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Serial mouse driver
 * FILE:            drivers/input/sermouse/fdo.c
 * PURPOSE:         IRP_MJ_PNP operations for FDOs
 *
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.org)
 */

#define NDEBUG
#include <debug.h>

#include "sermouse.h"

NTSTATUS NTAPI
SermouseAddDevice(
	IN PDRIVER_OBJECT DriverObject,
	IN PDEVICE_OBJECT Pdo)
{
	PSERMOUSE_DRIVER_EXTENSION DriverExtension;
	ULONG DeviceId = 0;
	ULONG PrefixLength;
	UNICODE_STRING DeviceNameU;
	PWSTR DeviceIdW = NULL; /* Pointer into DeviceNameU.Buffer */
	PDEVICE_OBJECT Fdo;
	PSERMOUSE_DEVICE_EXTENSION DeviceExtension;
	NTSTATUS Status;

	DPRINT("SermouseAddDevice called. Pdo = 0x%p\n", Pdo);

	/* Create new device object */
	DriverExtension = IoGetDriverObjectExtension(DriverObject, DriverObject);
	DeviceNameU.Length = 0;
	DeviceNameU.MaximumLength =
		wcslen(L"\\Device\\") * sizeof(WCHAR)           /* "\Device\" */
		+ DriverExtension->PointerDeviceBaseName.Length /* "PointerPort" */
		+ 4 * sizeof(WCHAR)                             /* Id between 0 and 9999 */
		+ sizeof(UNICODE_NULL);                         /* Final NULL char */
	DeviceNameU.Buffer = ExAllocatePool(PagedPool, DeviceNameU.MaximumLength);
	if (!DeviceNameU.Buffer)
	{
		DPRINT("ExAllocatePool() failed\n");
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	Status = RtlAppendUnicodeToString(&DeviceNameU, L"\\Device\\");
	if (!NT_SUCCESS(Status))
	{
		DPRINT("RtlAppendUnicodeToString() failed with status 0x%08lx\n", Status);
		goto cleanup;
	}
	Status = RtlAppendUnicodeStringToString(&DeviceNameU, &DriverExtension->PointerDeviceBaseName);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("RtlAppendUnicodeStringToString() failed with status 0x%08lx\n", Status);
		goto cleanup;
	}
	PrefixLength = DeviceNameU.MaximumLength - 4 * sizeof(WCHAR) - sizeof(UNICODE_NULL);
	DeviceIdW = &DeviceNameU.Buffer[PrefixLength / sizeof(WCHAR)];
	while (DeviceId < 9999)
	{
		DeviceNameU.Length = PrefixLength + swprintf(DeviceIdW, L"%lu", DeviceId) * sizeof(WCHAR);
		Status = IoCreateDevice(
			DriverObject,
			sizeof(SERMOUSE_DEVICE_EXTENSION),
			&DeviceNameU,
			FILE_DEVICE_SERIAL_MOUSE_PORT,
			FILE_DEVICE_SECURE_OPEN,
			TRUE,
			&Fdo);
		if (NT_SUCCESS(Status))
			goto cleanup;
		else if (Status != STATUS_OBJECT_NAME_COLLISION)
		{
			DPRINT("IoCreateDevice() failed with status 0x%08lx\n", Status);
			goto cleanup;
		}
		DeviceId++;
	}
	DPRINT("Too much devices starting with '\\Device\\%wZ'\n", &DriverExtension->PointerDeviceBaseName);
	Status = STATUS_UNSUCCESSFUL;
cleanup:
	if (!NT_SUCCESS(Status))
	{
		ExFreePool(DeviceNameU.Buffer);
		return Status;
	}

	DeviceExtension = (PSERMOUSE_DEVICE_EXTENSION)Fdo->DeviceExtension;
	RtlZeroMemory(DeviceExtension, sizeof(SERMOUSE_DEVICE_EXTENSION));
	DeviceExtension->MouseType = mtNone;
	DeviceExtension->PnpState = dsStopped;
	DeviceExtension->DriverExtension = DriverExtension;
	KeInitializeEvent(&DeviceExtension->StopWorkerThreadEvent, NotificationEvent, FALSE);
	DeviceExtension->MouseInputData[0] = ExAllocatePool(NonPagedPool, DeviceExtension->DriverExtension->MouseDataQueueSize * sizeof(MOUSE_INPUT_DATA));
	if (!DeviceExtension->MouseInputData[0])
	{
		DPRINT("ExAllocatePool() failed\n");
		Status = STATUS_INSUFFICIENT_RESOURCES;
		goto cleanupFDO;
	}
	DeviceExtension->MouseInputData[1] = ExAllocatePool(NonPagedPool, DeviceExtension->DriverExtension->MouseDataQueueSize * sizeof(MOUSE_INPUT_DATA));
	if (!DeviceExtension->MouseInputData[1])
	{
		DPRINT("ExAllocatePool() failed\n");
		Status = STATUS_INSUFFICIENT_RESOURCES;
		goto cleanupFDO;
	}
	Fdo->Flags |= DO_POWER_PAGABLE;
	Status = IoAttachDeviceToDeviceStackSafe(Fdo, Pdo, &DeviceExtension->LowerDevice);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("IoAttachDeviceToDeviceStackSafe() failed with status 0x%08lx\n", Status);
		goto cleanupFDO;
	}
	Fdo->Flags |= DO_BUFFERED_IO;
	Fdo->Flags &= ~DO_DEVICE_INITIALIZING;

	ExFreePool(DeviceNameU.Buffer);

	return STATUS_SUCCESS;

cleanupFDO:
	if (DeviceExtension)
	{
		if (DeviceExtension->LowerDevice)
			IoDetachDevice(DeviceExtension->LowerDevice);
		ExFreePool(DeviceExtension->MouseInputData[0]);
		ExFreePool(DeviceExtension->MouseInputData[1]);
	}
	if (Fdo)
	{
		IoDeleteDevice(Fdo);
	}
	return Status;
}

NTSTATUS NTAPI
SermouseStartDevice(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	PSERMOUSE_DEVICE_EXTENSION DeviceExtension;
	SERMOUSE_MOUSE_TYPE MouseType;

	DeviceExtension = (PSERMOUSE_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	ASSERT(DeviceExtension->PnpState == dsStopped);
	ASSERT(DeviceExtension->LowerDevice);
	MouseType = SermouseDetectLegacyDevice(DeviceExtension->LowerDevice);
	if (MouseType == mtNone)
	{
		DPRINT("No mouse connected to Fdo %p\n",
			DeviceExtension->LowerDevice);
		return STATUS_DEVICE_NOT_CONNECTED;
	}

	switch (MouseType)
	{
		case mtMicrosoft:
			DeviceExtension->AttributesInformation.MouseIdentifier = MOUSE_SERIAL_HARDWARE;
			DeviceExtension->AttributesInformation.NumberOfButtons = 2;
			break;
		case mtLogitech:
			DeviceExtension->AttributesInformation.MouseIdentifier = MOUSE_SERIAL_HARDWARE;
			DeviceExtension->AttributesInformation.NumberOfButtons = 3;
			break;
		case mtWheelZ:
			DeviceExtension->AttributesInformation.MouseIdentifier = WHEELMOUSE_SERIAL_HARDWARE;
			DeviceExtension->AttributesInformation.NumberOfButtons = 3;
			break;
		default:
			CHECKPOINT;
			return STATUS_UNSUCCESSFUL;
	}

	if (DeviceExtension->DriverExtension->NumberOfButtons != 0)
		/* Override the number of buttons */
		DeviceExtension->AttributesInformation.NumberOfButtons = DeviceExtension->DriverExtension->NumberOfButtons;

	DeviceExtension->AttributesInformation.SampleRate = DeviceExtension->DriverExtension->SampleRate / 8;
	DeviceExtension->AttributesInformation.InputDataQueueLength = DeviceExtension->DriverExtension->MouseDataQueueSize;
	DeviceExtension->MouseType = MouseType;
	DeviceExtension->PnpState = dsStarted;

	return STATUS_SUCCESS;
}

NTSTATUS NTAPI
SermousePnp(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	ULONG MinorFunction;
	PIO_STACK_LOCATION Stack;
	ULONG Information = 0;
	NTSTATUS Status;

	Stack = IoGetCurrentIrpStackLocation(Irp);
	MinorFunction = Stack->MinorFunction;

	switch (MinorFunction)
	{
		/* FIXME: do all these minor functions
		IRP_MN_QUERY_REMOVE_DEVICE 0x1
		IRP_MN_REMOVE_DEVICE 0x2
		IRP_MN_CANCEL_REMOVE_DEVICE 0x3
		IRP_MN_STOP_DEVICE 0x4
		IRP_MN_QUERY_STOP_DEVICE 0x5
		IRP_MN_CANCEL_STOP_DEVICE 0x6
		IRP_MN_QUERY_DEVICE_RELATIONS / BusRelations (optional) 0x7
		IRP_MN_QUERY_DEVICE_RELATIONS / RemovalRelations (optional) 0x7
		IRP_MN_QUERY_INTERFACE (optional) 0x8
		IRP_MN_QUERY_CAPABILITIES (optional) 0x9
		IRP_MN_FILTER_RESOURCE_REQUIREMENTS (optional or required) 0xd
		IRP_MN_QUERY_PNP_DEVICE_STATE (optional) 0x14
		IRP_MN_DEVICE_USAGE_NOTIFICATION (required or optional) 0x16
		IRP_MN_SURPRISE_REMOVAL 0x17
		*/
		case IRP_MN_START_DEVICE: /* 0x0 */
		{
			DPRINT("IRP_MJ_PNP / IRP_MN_START_DEVICE\n");
			/* Call lower driver */
			Status = ForwardIrpAndWait(DeviceObject, Irp);
			if (NT_SUCCESS(Status))
				Status = SermouseStartDevice(DeviceObject, Irp);
			break;
		}
		default:
		{
			DPRINT1("IRP_MJ_PNP / unknown minor function 0x%lx\n", MinorFunction);
			return ForwardIrpAndForget(DeviceObject, Irp);
		}
	}

	Irp->IoStatus.Information = Information;
	Irp->IoStatus.Status = Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}
