/*
 * PROJECT:     ReactOS Serial mouse driver
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/input/sermouse/fdo.c
 * PURPOSE:     IRP_MJ_PNP operations for FDOs
 * PROGRAMMERS: Copyright 2005-2006 Hervé Poussineau (hpoussin@reactos.org)
 */

#include "sermouse.h"

#include <debug.h>

NTSTATUS NTAPI
SermouseAddDevice(
	IN PDRIVER_OBJECT DriverObject,
	IN PDEVICE_OBJECT Pdo)
{
	PSERMOUSE_DRIVER_EXTENSION DriverExtension;
	PDEVICE_OBJECT Fdo;
	PSERMOUSE_DEVICE_EXTENSION DeviceExtension = NULL;
	NTSTATUS Status;

	TRACE_(SERMOUSE, "SermouseAddDevice called. Pdo = 0x%p\n", Pdo);

	if (Pdo == NULL)
		return STATUS_SUCCESS;

	/* Create new device object */
	DriverExtension = IoGetDriverObjectExtension(DriverObject, DriverObject);
	Status = IoCreateDevice(
		DriverObject,
		sizeof(SERMOUSE_DEVICE_EXTENSION),
		NULL,
		FILE_DEVICE_SERIAL_MOUSE_PORT,
		FILE_DEVICE_SECURE_OPEN,
		TRUE,
		&Fdo);
	if (!NT_SUCCESS(Status))
	{
		WARN_(SERMOUSE, "IoCreateDevice() failed with status 0x%08lx\n", Status);
		goto cleanup;
	}

	DeviceExtension = (PSERMOUSE_DEVICE_EXTENSION)Fdo->DeviceExtension;
	RtlZeroMemory(DeviceExtension, sizeof(SERMOUSE_DEVICE_EXTENSION));
	DeviceExtension->MouseType = mtNone;
	DeviceExtension->PnpState = dsStopped;
	DeviceExtension->DriverExtension = DriverExtension;
	KeInitializeEvent(&DeviceExtension->StopWorkerThreadEvent, NotificationEvent, FALSE);
	Status = IoAttachDeviceToDeviceStackSafe(Fdo, Pdo, &DeviceExtension->LowerDevice);
	if (!NT_SUCCESS(Status))
	{
		WARN_(SERMOUSE, "IoAttachDeviceToDeviceStackSafe() failed with status 0x%08lx\n", Status);
		goto cleanup;
	}
	if (DeviceExtension->LowerDevice->Flags & DO_POWER_PAGABLE)
		Fdo->Flags |= DO_POWER_PAGABLE;
	if (DeviceExtension->LowerDevice->Flags & DO_BUFFERED_IO)
		Fdo->Flags |= DO_BUFFERED_IO;
	if (DeviceExtension->LowerDevice->Flags & DO_DIRECT_IO)
		Fdo->Flags |= DO_DIRECT_IO;
	Fdo->Flags &= ~DO_DEVICE_INITIALIZING;

	return STATUS_SUCCESS;

cleanup:
	if (DeviceExtension)
	{
		if (DeviceExtension->LowerDevice)
			IoDetachDevice(DeviceExtension->LowerDevice);
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
	NTSTATUS Status;

	DeviceExtension = (PSERMOUSE_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	ASSERT(DeviceExtension->PnpState == dsStopped);
	ASSERT(DeviceExtension->LowerDevice);
	MouseType = SermouseDetectLegacyDevice(DeviceExtension->LowerDevice);
	if (MouseType == mtNone)
	{
		WARN_(SERMOUSE, "No mouse connected to Fdo %p\n",
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
			WARN_(SERMOUSE, "Unknown mouse type 0x%lx\n", MouseType);
			ASSERT(FALSE);
			return STATUS_UNSUCCESSFUL;
	}

	if (DeviceExtension->DriverExtension->NumberOfButtons != 0)
		/* Override the number of buttons */
		DeviceExtension->AttributesInformation.NumberOfButtons = DeviceExtension->DriverExtension->NumberOfButtons;

	DeviceExtension->AttributesInformation.SampleRate = 1200 / 8;
	DeviceExtension->AttributesInformation.InputDataQueueLength = 1;
	DeviceExtension->MouseType = MouseType;
	DeviceExtension->PnpState = dsStarted;

	/* Start read loop */
	Status = PsCreateSystemThread(
		&DeviceExtension->WorkerThreadHandle,
		(ACCESS_MASK)0L,
		NULL,
		NULL,
		NULL,
		SermouseDeviceWorker,
		DeviceObject);

	return Status;
}

NTSTATUS NTAPI
SermousePnp(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	ULONG MinorFunction;
	PIO_STACK_LOCATION Stack;
	ULONG_PTR Information = 0;
	NTSTATUS Status;

	Stack = IoGetCurrentIrpStackLocation(Irp);
	MinorFunction = Stack->MinorFunction;
	Information = Irp->IoStatus.Information;

	switch (MinorFunction)
	{
		/* FIXME: do all these minor functions
		IRP_MN_QUERY_REMOVE_DEVICE 0x1
		IRP_MN_REMOVE_DEVICE 0x2
		IRP_MN_CANCEL_REMOVE_DEVICE 0x3
		IRP_MN_STOP_DEVICE 0x4
		IRP_MN_QUERY_STOP_DEVICE 0x5
		IRP_MN_CANCEL_STOP_DEVICE 0x6
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
			PSERMOUSE_DEVICE_EXTENSION DeviceExtension;

			TRACE_(SERMOUSE, "IRP_MJ_PNP / IRP_MN_START_DEVICE\n");
			Status = STATUS_UNSUCCESSFUL;
			DeviceExtension = DeviceObject->DeviceExtension;

			/* Call lower driver */
			if (IoForwardIrpSynchronously(DeviceExtension->LowerDevice, Irp))
			{
				Status = Irp->IoStatus.Status;
				if (NT_SUCCESS(Status))
				{
					Status = SermouseStartDevice(DeviceObject, Irp);
				}
			}
			break;
		}
		case IRP_MN_QUERY_DEVICE_RELATIONS: /* 0x7 */
		{
			switch (Stack->Parameters.QueryDeviceRelations.Type)
			{
				case BusRelations:
				{
					PDEVICE_RELATIONS DeviceRelations = NULL;
					TRACE_(SERMOUSE, "IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / TargetDeviceRelation\n");

					DeviceRelations = ExAllocatePoolWithTag(PagedPool, FIELD_OFFSET(DEVICE_RELATIONS, Objects), SERMOUSE_TAG);
					if (!DeviceRelations)
					{
						WARN_(SERMOUSE, "ExAllocatePoolWithTag() failed\n");
						Status = STATUS_NO_MEMORY;
					}
					else
					{
						DeviceRelations->Count = 0;
						Status = STATUS_SUCCESS;
						Information = (ULONG_PTR)DeviceRelations;
					}
					break;
				}
				default:
				{
					TRACE_(SERMOUSE, "IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / Unknown type 0x%lx\n",
						Stack->Parameters.QueryDeviceRelations.Type);
					return ForwardIrpAndForget(DeviceObject, Irp);
				}
			}
			break;
		}
		default:
		{
			TRACE_(SERMOUSE, "IRP_MJ_PNP / unknown minor function 0x%lx\n", MinorFunction);
			return ForwardIrpAndForget(DeviceObject, Irp);
		}
	}

	Irp->IoStatus.Information = Information;
	Irp->IoStatus.Status = Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}
