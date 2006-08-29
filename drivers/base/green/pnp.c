/*
 * PROJECT:     ReactOS VT100 emulator
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/base/green/pnp.c
 * PURPOSE:     IRP_MJ_PNP operations
 * PROGRAMMERS: Copyright 2005-2006 Hervé Poussineau (hpoussin@reactos.org)
 */

#include "green.h"

#define NDEBUG
#include <debug.h>

static NTSTATUS
CreateGreenFdo(
	IN PDRIVER_OBJECT DriverObject,
	IN PDEVICE_OBJECT GreenPdo)
{
	PGREEN_DRIVER_EXTENSION DriverExtension = NULL;
	PGREEN_DEVICE_EXTENSION DeviceExtension = NULL;
	OBJECT_ATTRIBUTES ObjectAttributes;
	ULONG Fcr;
	HANDLE LocalHandle = 0;
	ACCESS_MASK DesiredAccess = FILE_ANY_ACCESS;
	NTSTATUS Status;

	DriverExtension = IoGetDriverObjectExtension(DriverObject, DriverObject);

	Status = IoCreateDevice(
		DriverObject,
		sizeof(GREEN_DEVICE_EXTENSION),
		NULL,
		FILE_DEVICE_TERMSRV,
		FILE_DEVICE_SECURE_OPEN,
		FALSE,
		&DriverExtension->GreenMainDO);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("IoCreateDevice() failed with status %08lx\n", Status);
		goto cleanup;
	}

	DeviceExtension = (PGREEN_DEVICE_EXTENSION)DriverExtension->GreenMainDO->DeviceExtension;
	RtlZeroMemory(DeviceExtension, sizeof(GREEN_DEVICE_EXTENSION));
	DeviceExtension->Common.Type = GreenFDO;
	DriverExtension->GreenMainDO->Flags |= DO_POWER_PAGABLE;
	DriverExtension->LowerDevice = IoAttachDeviceToDeviceStack(DriverExtension->GreenMainDO, GreenPdo);

	/* Initialize serial port */
	InitializeObjectAttributes(&ObjectAttributes, &DriverExtension->AttachedDeviceName, OBJ_KERNEL_HANDLE, NULL, NULL);
	Status = ObOpenObjectByName(
		&ObjectAttributes,
		IoFileObjectType,
		KernelMode,
		NULL,
		DesiredAccess,
		NULL,
		&LocalHandle);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("ObOpenObjectByName() failed with status %08lx\n", Status);
		goto cleanup;
	}
	Status = ObReferenceObjectByHandle(
		LocalHandle,
		DesiredAccess,
		NULL,
		KernelMode,
		(PVOID*)&DeviceExtension->Serial,
		NULL);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("ObReferenceObjectByHandle() failed with status %08lx\n", Status);
		goto cleanup;
	}
	Fcr = 0;
	Status = GreenDeviceIoControl(DeviceExtension->Serial, IOCTL_SERIAL_SET_FIFO_CONTROL,
		&Fcr, sizeof(Fcr), NULL, NULL);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("GreenDeviceIoControl() failed with status %08lx\n", Status);
		goto cleanup;
	}
	Status = GreenDeviceIoControl(DeviceExtension->Serial, IOCTL_SERIAL_SET_BAUD_RATE,
		&DriverExtension->SampleRate, sizeof(DriverExtension->SampleRate), NULL, NULL);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("GreenDeviceIoControl() failed with status %08lx\n", Status);
		goto cleanup;
	}
	DeviceExtension->LineControl.WordLength = 8;
	DeviceExtension->LineControl.Parity = NO_PARITY;
	DeviceExtension->LineControl.StopBits = STOP_BIT_1;
	Status = GreenDeviceIoControl(DeviceExtension->Serial, IOCTL_SERIAL_SET_LINE_CONTROL,
		&DeviceExtension->LineControl, sizeof(SERIAL_LINE_CONTROL), NULL, NULL);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("GreenDeviceIoControl() failed with status %08lx\n", Status);
		goto cleanup;
	}
	RtlZeroMemory(&DeviceExtension->Timeouts, sizeof(SERIAL_TIMEOUTS));
	DeviceExtension->Timeouts.ReadIntervalTimeout = 100;
	Status = GreenDeviceIoControl(DeviceExtension->Serial, IOCTL_SERIAL_SET_TIMEOUTS,
		&DeviceExtension->Timeouts, sizeof(SERIAL_TIMEOUTS), NULL, NULL);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("GreenDeviceIoControl() failed with status %08lx\n", Status);
		goto cleanup;
	}

	DriverExtension->GreenMainDO->Flags |= DO_BUFFERED_IO;
	DriverExtension->GreenMainDO->Flags &= ~DO_DEVICE_INITIALIZING;

	Status = STATUS_SUCCESS;

cleanup:
	if (LocalHandle != 0)
		ZwClose(LocalHandle);
	if (!NT_SUCCESS(Status))
	{
		if (DeviceExtension && DeviceExtension->Serial)
			ObDereferenceObject(DeviceExtension->Serial);
		if (DriverExtension)
		{
			if (DriverExtension->LowerDevice)
			{
				IoDetachDevice(DriverExtension->LowerDevice);
				DriverExtension->LowerDevice = NULL;
			}
			if (DriverExtension->GreenMainDO)
			{
				IoDeleteDevice(DriverExtension->GreenMainDO);
				DriverExtension->GreenMainDO = NULL;
			}
		}
	}
	return Status;
}

static NTSTATUS
ReportGreenPdo(
	IN PDRIVER_OBJECT DriverObject,
	IN PGREEN_DRIVER_EXTENSION DriverExtension)
{
	PDEVICE_OBJECT GreenPdo = NULL;
	NTSTATUS Status;

	/* Create green PDO */
	Status = IoReportDetectedDevice(
		DriverObject,
		InterfaceTypeUndefined, -1, -1,
		NULL, NULL, TRUE,
		&GreenPdo);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("IoReportDetectedDevice() failed with status 0x%lx\n", Status);
		goto cleanup;
	}

	/* Create green FDO */
	Status = CreateGreenFdo(DriverObject, GreenPdo);

	IoInvalidateDeviceRelations(GreenPdo, BusRelations);

	/* FIXME: Update registry, set "DeviceReported" to 1 */

	Status = STATUS_SUCCESS;

cleanup:
	if (!NT_SUCCESS(Status))
	{
		if (DriverExtension->GreenMainDO)
			IoDeleteDevice(DriverExtension->GreenMainDO);
	}
	return Status;
}

NTSTATUS NTAPI
GreenAddDevice(
	IN PDRIVER_OBJECT DriverObject,
	IN PDEVICE_OBJECT Pdo)
{
	PGREEN_DRIVER_EXTENSION DriverExtension;

	DPRINT("AddDevice(DriverObject %p, Pdo %p)\n", DriverObject, Pdo);

	DriverExtension = IoGetDriverObjectExtension(DriverObject, DriverObject);

	if (Pdo == NULL)
	{
		if (DriverExtension->DeviceReported)
			/* Green Pdo has already been reported during a previous boot.
			 * We will get another AddDevice call soon.
			 */
			return STATUS_SUCCESS;
		else
			return ReportGreenPdo(DriverObject, DriverExtension);
	}
	else if (DriverExtension->GreenMainDO == NULL)
	{
		return CreateGreenFdo(DriverObject, Pdo);
	}
	else
	{
		PGREEN_DEVICE_EXTENSION GreenDeviceExtension;

		GreenDeviceExtension = (PGREEN_DEVICE_EXTENSION)DriverExtension->GreenMainDO->DeviceExtension;
		if (Pdo == GreenDeviceExtension->KeyboardPdo)
			return KeyboardAddDevice(DriverObject, Pdo);
		else if (Pdo == GreenDeviceExtension->ScreenPdo)
			return ScreenAddDevice(DriverObject, Pdo);
		else
			/* Strange PDO. We don't know it */
			ASSERT(FALSE);
			return STATUS_UNSUCCESSFUL;
	}
}

static NTSTATUS
GreenQueryBusRelations(
	IN PDEVICE_OBJECT DeviceObject,
	OUT PDEVICE_RELATIONS* pDeviceRelations)
{
	PGREEN_DEVICE_EXTENSION DeviceExtension;
	PDEVICE_RELATIONS DeviceRelations = NULL;
	NTSTATUS Status;

	DeviceExtension = (PGREEN_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	/* Create PDOs for keyboard and screen */
	if (DeviceExtension->KeyboardPdo == NULL)
	{
		Status = IoCreateDevice(
			DeviceObject->DriverObject,
			sizeof(COMMON_DEVICE_EXTENSION),
			NULL,
			FILE_DEVICE_KEYBOARD,
			FILE_AUTOGENERATED_DEVICE_NAME | FILE_DEVICE_SECURE_OPEN,
			FALSE,
			&DeviceExtension->KeyboardPdo);
		if (!NT_SUCCESS(Status))
		{
			DPRINT("IoCreateDevice() failed with status 0x%lx\n", Status);
			goto cleanup;
		}
		((PCOMMON_DEVICE_EXTENSION)DeviceExtension->KeyboardPdo->DeviceExtension)->Type = KeyboardPDO;
		DeviceExtension->KeyboardPdo->Flags |= DO_POWER_PAGABLE | DO_BUS_ENUMERATED_DEVICE;
		DeviceExtension->KeyboardPdo->Flags &= ~DO_DEVICE_INITIALIZING;
	}

	if (DeviceExtension->ScreenPdo == NULL)
	{
		Status = IoCreateDevice(
			DeviceObject->DriverObject,
			sizeof(COMMON_DEVICE_EXTENSION),
			NULL,
			FILE_DEVICE_SCREEN,
			FILE_AUTOGENERATED_DEVICE_NAME | FILE_DEVICE_SECURE_OPEN,
			FALSE,
			&DeviceExtension->ScreenPdo);
		if (!NT_SUCCESS(Status))
		{
			DPRINT("IoCreateDevice() failed with status 0x%lx\n", Status);
			goto cleanup;
		}
		((PCOMMON_DEVICE_EXTENSION)DeviceExtension->ScreenPdo->DeviceExtension)->Type = ScreenPDO;
		DeviceExtension->ScreenPdo->Flags |= DO_POWER_PAGABLE | DO_BUS_ENUMERATED_DEVICE;
		DeviceExtension->ScreenPdo->Flags &= ~DO_DEVICE_INITIALIZING;
	}

	/* Allocate return structure */
	DeviceRelations = (PDEVICE_RELATIONS)ExAllocatePool(
		PagedPool,
		FIELD_OFFSET(DEVICE_RELATIONS, Objects) + 2 * sizeof(PDEVICE_OBJECT));
	if (!DeviceRelations)
		return STATUS_INSUFFICIENT_RESOURCES;

	/* Fill return structure */
	DeviceRelations->Count = 2;
	ObReferenceObject(DeviceExtension->KeyboardPdo);
	ObReferenceObject(DeviceExtension->ScreenPdo);
	DeviceRelations->Objects[0] = DeviceExtension->KeyboardPdo;
	DeviceRelations->Objects[1] = DeviceExtension->ScreenPdo;

	*pDeviceRelations = DeviceRelations;
	Status = STATUS_SUCCESS;

cleanup:
	if (!NT_SUCCESS(Status))
	{
		if (DeviceRelations)
		{
			ULONG i;
			for (i = 0; i < DeviceRelations->Count; i++)
				ObDereferenceObject(DeviceRelations->Objects[i]);
			ExFreePool(DeviceRelations);
		}
		if (DeviceExtension->KeyboardPdo)
		{
			IoDeleteDevice(DeviceExtension->KeyboardPdo);
			DeviceExtension->KeyboardPdo = NULL;
		}
		if (DeviceExtension->ScreenPdo)
		{
			IoDeleteDevice(DeviceExtension->ScreenPdo);
			DeviceExtension->ScreenPdo = NULL;
		}
	}
	return Status;
}

static NTSTATUS
GreenQueryId(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp,
	OUT ULONG_PTR* Information)
{
	GREEN_DEVICE_TYPE Type;
	ULONG IdType;
	NTSTATUS Status = Irp->IoStatus.Status;

	Type = ((PCOMMON_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->Type;
	IdType = IoGetCurrentIrpStackLocation(Irp)->Parameters.QueryId.IdType;

	switch (IdType)
	{
		case BusQueryDeviceID:
		{
			LPCWSTR Source = NULL;

			if (Type == ScreenPDO)
				Source = L"GREEN\\SCREEN";
			else if (Type == KeyboardPDO)
				Source = L"GREEN\\KEYBOARD";
			else
			{
				DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryDeviceId / Unknown type 0x%lx\n",
					Type);
				ASSERT(FALSE);
			}

			if (Source)
			{
				UNICODE_STRING SourceU, String;
				RtlInitUnicodeString(&SourceU, Source);
				Status = RtlDuplicateUnicodeString(
					RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE,
					&SourceU,
					&String);
				*Information = (ULONG_PTR)String.Buffer;
			}
			break;
		}
		case BusQueryHardwareIDs:
		{
			UNICODE_STRING SourceU = { 0, };

			if (Type == ScreenPDO)
			{
				RtlInitUnicodeString(&SourceU, L"GREEN\\SCREEN\0");
				/* We can add the two \0 that are at the end of the string */
				SourceU.Length = SourceU.MaximumLength = SourceU.Length + 2 * sizeof(WCHAR);
			}
			else if (Type == KeyboardPDO)
			{
				RtlInitUnicodeString(&SourceU, L"GREEN\\KEYBOARD\0");
				/* We can add the two \0 that are at the end of the string */
				SourceU.Length = SourceU.MaximumLength = SourceU.Length + 2 * sizeof(WCHAR);
			}
			else
			{
				DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryHardwareIDs / Unknown type 0x%lx\n",
					Type);
				ASSERT(FALSE);
			}

			if (SourceU.Length)
			{
				UNICODE_STRING String;
				Status = RtlDuplicateUnicodeString(
					RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE,
					&SourceU,
					&String);
				*Information = (ULONG_PTR)String.Buffer;
			}
			break;
		}
		case BusQueryCompatibleIDs:
		{
			/* We don't have any compatible ID */
			break;
		}
		case BusQueryInstanceID:
		{
			/* We don't have any instance ID */
			break;
		}
		default:
		{
			DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_ID / unknown query id type 0x%lx\n", IdType);
		}
	}

	return Status;
}

NTSTATUS
GreenPnp(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	GREEN_DEVICE_TYPE Type;
	PIO_STACK_LOCATION Stack;
	ULONG_PTR Information = Irp->IoStatus.Information;
	NTSTATUS Status = Irp->IoStatus.Status;

	Type = ((PCOMMON_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->Type;
	Stack = IoGetCurrentIrpStackLocation(Irp);

	switch (Stack->MinorFunction)
	{
		case IRP_MN_START_DEVICE: /* 0x00 */
		{
			DPRINT("IRP_MJ_PNP / IRP_MN_START_DEVICE\n");
			if (Type == GreenFDO || Type == KeyboardPDO || Type == ScreenPDO)
				Status = STATUS_SUCCESS;
			else
			{
				DPRINT1("IRP_MJ_PNP / IRP_MN_START_DEVICE / Unknown type 0x%lx\n",
					Type);
				ASSERT(FALSE);
			}
			break;
		}
		case IRP_MN_QUERY_DEVICE_RELATIONS: /* 0x07 */
		{
			DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS\n");
			switch (Stack->Parameters.QueryDeviceRelations.Type)
			{
				case BusRelations:
				{
					if (Type == GreenFDO)
					{
						PDEVICE_RELATIONS DeviceRelations = NULL;
						Status = GreenQueryBusRelations(DeviceObject, &DeviceRelations);
						Information = (ULONG_PTR)DeviceRelations;
					}
					else if (Type == KeyboardPDO || Type == ScreenPDO)
					{
						PDEVICE_RELATIONS DeviceRelations = NULL;
						DeviceRelations = ExAllocatePool(PagedPool, FIELD_OFFSET(DEVICE_RELATIONS, Objects));
						if (!DeviceRelations)
							Status = STATUS_INSUFFICIENT_RESOURCES;
						else
						{
							DeviceRelations->Count = 0;
							Status = STATUS_SUCCESS;
							Information = (ULONG_PTR)DeviceRelations;
						}
					}
					else
					{
						DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / BusRelations / Unknown type 0x%lx\n",
							Type);
						ASSERT(FALSE);
					}
					break;
				}
				default:
				{
					DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / Unknown type 0x%lx\n",
						Stack->Parameters.QueryDeviceRelations.Type);
					break;
				}
			}
			break;
		}
		case IRP_MN_QUERY_RESOURCES: /* 0x0a */
		{
			DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_RESOURCES\n");
			/* We don't need resources */
			break;
		}
		case IRP_MN_QUERY_RESOURCE_REQUIREMENTS: /* 0x0b */
		{
			DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_RESOURCE_REQUIREMENTS\n");
			/* We don't need resources */
			break;
		}
		case IRP_MN_QUERY_DEVICE_TEXT: /* 0x0c */
		{
			DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_TEXT\n");
			switch (Stack->Parameters.QueryDeviceText.DeviceTextType)
			{
				case DeviceTextDescription:
				{
					LPCWSTR Description = NULL;
					if (Type == GreenFDO)
						Description = L"Green device";
					else if (Type == ScreenPDO)
						Description = L"Green screen";
					else if (Type == KeyboardPDO)
						Description = L"Green keyboard";

					if (Description != NULL)
					{
						LPWSTR Destination = ExAllocatePool(PagedPool, wcslen(Description) * sizeof(WCHAR) + sizeof(UNICODE_NULL));
						if (!Destination)
							Status = STATUS_INSUFFICIENT_RESOURCES;
						else
						{
							wcscpy(Destination, Description);
							Information = (ULONG_PTR)Description;
							Status = STATUS_SUCCESS;
						}
					}
					else
					{
						DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_TEXT / DeviceTextDescription / Unknown type 0x%lx\n",
							Type);
						ASSERT(FALSE);
					}
					break;
				}
				case DeviceTextLocationInformation:
				{
					/* We don't have any text location to report,
					 * and this query is optional, so ignore it.
					 */
					break;
				}
				default:
				{
					DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_TEXT / unknown type 0x%lx\n",
						Stack->Parameters.QueryDeviceText.DeviceTextType);
					ASSERT(FALSE);
					break;
				}
			}
			break;
		}
		case IRP_MN_QUERY_ID: /* 0x13 */
		{
			DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_ID\n");
			Status = GreenQueryId(DeviceObject, Irp, &Information);
			break;
		}
		default:
		{
			DPRINT1("IRP_MJ_PNP / unknown minor function 0x%lx\n", Stack->MinorFunction);
			break;
		}
	}

	Irp->IoStatus.Status = Status;
	Irp->IoStatus.Information = Information;
	if (Status != STATUS_PENDING)
		IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return Status;
}

