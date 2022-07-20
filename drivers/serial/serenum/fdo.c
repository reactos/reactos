/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Serial enumerator driver
 * FILE:            drivers/bus/serenum/fdo.c
 * PURPOSE:         IRP_MJ_PNP operations for FDOs
 *
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.org)
 */

#include "serenum.h"

#include <debug.h>

NTSTATUS NTAPI
SerenumAddDevice(
	IN PDRIVER_OBJECT DriverObject,
	IN PDEVICE_OBJECT Pdo)
{
	PDEVICE_OBJECT Fdo;
	PFDO_DEVICE_EXTENSION DeviceExtension;
	NTSTATUS Status;

	TRACE_(SERENUM, "SerenumAddDevice called. Pdo = %p\n", Pdo);

	/* Create new device object */
	Status = IoCreateDevice(DriverObject,
	                        sizeof(FDO_DEVICE_EXTENSION),
	                        NULL,
	                        FILE_DEVICE_BUS_EXTENDER,
	                        FILE_DEVICE_SECURE_OPEN,
	                        FALSE,
	                        &Fdo);
	if (!NT_SUCCESS(Status))
	{
		WARN_(SERENUM, "IoCreateDevice() failed with status 0x%08lx\n", Status);
		return Status;
	}
	DeviceExtension = (PFDO_DEVICE_EXTENSION)Fdo->DeviceExtension;
	RtlZeroMemory(DeviceExtension, sizeof(FDO_DEVICE_EXTENSION));

	/* Register device interface */
	Status = IoRegisterDeviceInterface(
		Pdo,
		&GUID_DEVINTERFACE_SERENUM_BUS_ENUMERATOR,
		NULL,
		&DeviceExtension->SerenumInterfaceName);
	if (!NT_SUCCESS(Status))
	{
		WARN_(SERENUM, "IoRegisterDeviceInterface() failed with status 0x%08lx\n", Status);
		IoDeleteDevice(Fdo);
		return Status;
	}

	DeviceExtension->Common.IsFDO = TRUE;
	DeviceExtension->Common.PnpState = dsStopped;
	DeviceExtension->Pdo = Pdo;
	IoInitializeRemoveLock(&DeviceExtension->RemoveLock, SERENUM_TAG, 0, 0);
	Status = IoAttachDeviceToDeviceStackSafe(Fdo, Pdo, &DeviceExtension->LowerDevice);
	if (!NT_SUCCESS(Status))
	{
		WARN_(SERENUM, "IoAttachDeviceToDeviceStackSafe() failed with status 0x%08lx\n", Status);
		IoDeleteDevice(Fdo);
		return Status;
	}
	if (DeviceExtension->LowerDevice->Flags & DO_POWER_PAGABLE)
		Fdo->Flags |= DO_POWER_PAGABLE;
	if (DeviceExtension->LowerDevice->Flags & DO_BUFFERED_IO)
		Fdo->Flags |= DO_BUFFERED_IO;
	if (DeviceExtension->LowerDevice->Flags & DO_DIRECT_IO)
		Fdo->Flags |= DO_DIRECT_IO;
	Fdo->Flags &= ~DO_DEVICE_INITIALIZING;

	return STATUS_SUCCESS;
}

static NTSTATUS NTAPI
SerenumFdoStartDevice(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	PFDO_DEVICE_EXTENSION DeviceExtension;
	NTSTATUS Status;

	TRACE_(SERENUM, "SerenumFdoStartDevice() called\n");
	DeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	ASSERT(DeviceExtension->Common.PnpState == dsStopped);

	Status = IoSetDeviceInterfaceState(&DeviceExtension->SerenumInterfaceName, TRUE);
	if (!NT_SUCCESS(Status))
	{
		WARN_(SERENUM, "IoSetDeviceInterfaceState() failed with status 0x%08lx\n", Status);
		return Status;
	}

	DeviceExtension->Common.PnpState = dsStarted;

	return STATUS_SUCCESS;
}

static NTSTATUS
SerenumFdoQueryBusRelations(
	IN PDEVICE_OBJECT DeviceObject,
	OUT PDEVICE_RELATIONS* pDeviceRelations)
{
	PFDO_DEVICE_EXTENSION DeviceExtension;
	PDEVICE_RELATIONS DeviceRelations;
	ULONG NumPDO;
	ULONG i;
	NTSTATUS Status = STATUS_SUCCESS;

	DeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	ASSERT(DeviceExtension->Common.IsFDO);

	/* Do enumeration if needed */
	if (!(DeviceExtension->Flags & FLAG_ENUMERATION_DONE))
	{
		ASSERT(DeviceExtension->AttachedPdo == NULL);
		/* Detect plug-and-play devices */
		Status = SerenumDetectPnpDevice(DeviceObject, DeviceExtension->LowerDevice);
		if (Status == STATUS_DEVICE_NOT_CONNECTED)
		{
			/* Detect legacy devices */
			Status = SerenumDetectLegacyDevice(DeviceObject, DeviceExtension->LowerDevice);
			if (Status == STATUS_DEVICE_NOT_CONNECTED)
				Status = STATUS_SUCCESS;
		}
		DeviceExtension->Flags |= FLAG_ENUMERATION_DONE;
	}
	NumPDO = (DeviceExtension->AttachedPdo != NULL ? 1 : 0);

	DeviceRelations = (PDEVICE_RELATIONS)ExAllocatePoolWithTag(
		PagedPool,
		sizeof(DEVICE_RELATIONS) + sizeof(PDEVICE_OBJECT) * (NumPDO - 1),
		SERENUM_TAG);
	if (!DeviceRelations)
		return STATUS_INSUFFICIENT_RESOURCES;

	/* Fill returned structure */
	DeviceRelations->Count = NumPDO;
	for (i = 0; i < NumPDO; i++)
	{
		ObReferenceObject(DeviceExtension->AttachedPdo);
		DeviceRelations->Objects[i] = DeviceExtension->AttachedPdo;
	}

	*pDeviceRelations = DeviceRelations;
	return Status;
}

NTSTATUS
SerenumFdoPnp(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	PFDO_DEVICE_EXTENSION FdoExtension;
	ULONG MinorFunction;
	PIO_STACK_LOCATION Stack;
	ULONG_PTR Information = 0;
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
		IRP_MN_QUERY_DEVICE_RELATIONS / RemovalRelations (optional) 0x7
		IRP_MN_QUERY_INTERFACE (optional) 0x8
		IRP_MN_QUERY_CAPABILITIES (optional) 0x9
		IRP_MN_QUERY_PNP_DEVICE_STATE (optional) 0x14
		IRP_MN_DEVICE_USAGE_NOTIFICATION (required or optional) 0x16
		IRP_MN_SURPRISE_REMOVAL 0x17
		*/
		case IRP_MN_START_DEVICE: /* 0x0 */
		{
			TRACE_(SERENUM, "IRP_MJ_PNP / IRP_MN_START_DEVICE\n");
			/* Call lower driver */
			FdoExtension = DeviceObject->DeviceExtension;
			Status = STATUS_UNSUCCESSFUL;

			if (IoForwardIrpSynchronously(FdoExtension->LowerDevice, Irp))
			{
				Status = Irp->IoStatus.Status;
				if (NT_SUCCESS(Status))
				{
					Status = SerenumFdoStartDevice(DeviceObject, Irp);
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
					TRACE_(SERENUM, "IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / BusRelations\n");
					Status = SerenumFdoQueryBusRelations(DeviceObject, &DeviceRelations);
					Information = (ULONG_PTR)DeviceRelations;
					break;
				}
				default:
					TRACE_(SERENUM, "IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / Unknown type 0x%lx\n",
						Stack->Parameters.QueryDeviceRelations.Type);
					return ForwardIrpAndForget(DeviceObject, Irp);
			}
			break;
		}
		case IRP_MN_FILTER_RESOURCE_REQUIREMENTS: /* 0xd */
		{
			TRACE_(SERENUM, "IRP_MJ_PNP / IRP_MN_FILTER_RESOURCE_REQUIREMENTS\n");
			return ForwardIrpAndForget(DeviceObject, Irp);
		}
		default:
		{
			TRACE_(SERENUM, "IRP_MJ_PNP / unknown minor function 0x%lx\n", MinorFunction);
			return ForwardIrpAndForget(DeviceObject, Irp);
		}
	}

	Irp->IoStatus.Information = Information;
	Irp->IoStatus.Status = Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}
