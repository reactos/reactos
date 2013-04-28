/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Serial enumerator driver
 * FILE:            drivers/bus/serenum/pdo.c
 * PURPOSE:         IRP_MJ_PNP operations for PDOs
 *
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.org)
 */

#include "serenum.h"

static NTSTATUS
SerenumPdoStartDevice(
	IN PDEVICE_OBJECT DeviceObject)
{
	PPDO_DEVICE_EXTENSION DeviceExtension;

	DeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	ASSERT(DeviceExtension->Common.PnpState == dsStopped);

	DeviceExtension->Common.PnpState = dsStarted;
	return STATUS_SUCCESS;
}

static NTSTATUS
SerenumPdoQueryId(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp,
	OUT ULONG_PTR* Information)
{
	PPDO_DEVICE_EXTENSION DeviceExtension;
	ULONG IdType;
	PUNICODE_STRING SourceString;
	UNICODE_STRING String;
	NTSTATUS Status;

	IdType = IoGetCurrentIrpStackLocation(Irp)->Parameters.QueryId.IdType;
	DeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	RtlInitUnicodeString(&String, NULL);

	switch (IdType)
	{
		case BusQueryDeviceID:
		{
			TRACE_(SERENUM, "IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryDeviceID\n");
			SourceString = &DeviceExtension->DeviceId;
			break;
		}
		case BusQueryHardwareIDs:
		{
			TRACE_(SERENUM, "IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryHardwareIDs\n");
			SourceString = &DeviceExtension->HardwareIds;
			break;
		}
		case BusQueryCompatibleIDs:
			TRACE_(SERENUM, "IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryCompatibleIDs\n");
			SourceString = &DeviceExtension->CompatibleIds;
			break;
		case BusQueryInstanceID:
		{
			TRACE_(SERENUM, "IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryInstanceID\n");
			SourceString = &DeviceExtension->InstanceId;
			break;
		}
		default:
			WARN_(SERENUM, "IRP_MJ_PNP / IRP_MN_QUERY_ID / unknown query id type 0x%lx\n", IdType);
			ASSERT(FALSE);
			return STATUS_NOT_SUPPORTED;
	}

	Status = DuplicateUnicodeString(
		RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE,
		SourceString,
		&String);
	*Information = (ULONG_PTR)String.Buffer;
	return Status;
}

static NTSTATUS
SerenumPdoQueryDeviceRelations(
	IN PDEVICE_OBJECT DeviceObject,
	OUT PDEVICE_RELATIONS* pDeviceRelations)
{
	PFDO_DEVICE_EXTENSION DeviceExtension;
	PDEVICE_RELATIONS DeviceRelations;

	DeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	ASSERT(DeviceExtension->Common.IsFDO);

	DeviceRelations = (PDEVICE_RELATIONS)ExAllocatePoolWithTag(
		PagedPool,
		sizeof(DEVICE_RELATIONS),
		SERENUM_TAG);
	if (!DeviceRelations)
		return STATUS_INSUFFICIENT_RESOURCES;

	ObReferenceObject(DeviceObject);
	DeviceRelations->Count = 1;
	DeviceRelations->Objects[0] = DeviceObject;

	*pDeviceRelations = DeviceRelations;
	return STATUS_SUCCESS;
}

NTSTATUS
SerenumPdoPnp(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
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
		IRP_MN_QUERY_DEVICE_RELATIONS / EjectionRelations (optional) 0x7
		IRP_MN_QUERY_INTERFACE (required or optional) 0x8
		IRP_MN_READ_CONFIG (required or optional) 0xf
		IRP_MN_WRITE_CONFIG (required or optional) 0x10
		IRP_MN_EJECT (required or optional) 0x11
		IRP_MN_SET_LOCK (required or optional) 0x12
		IRP_MN_QUERY_ID / BusQueryDeviceID 0x13
		IRP_MN_QUERY_ID / BusQueryCompatibleIDs (optional) 0x13
		IRP_MN_QUERY_ID / BusQueryInstanceID (optional) 0x13
		IRP_MN_QUERY_PNP_DEVICE_STATE (optional) 0x14
		IRP_MN_DEVICE_USAGE_NOTIFICATION (required or optional) 0x16
		IRP_MN_SURPRISE_REMOVAL 0x17
		*/
		case IRP_MN_START_DEVICE: /* 0x0 */
		{
			TRACE_(SERENUM, "IRP_MJ_PNP / IRP_MN_START_DEVICE\n");
			Status = SerenumPdoStartDevice(DeviceObject);
			break;
		}
		case IRP_MN_QUERY_DEVICE_RELATIONS: /* 0x7 */
		{
			switch (Stack->Parameters.QueryDeviceRelations.Type)
			{
				case RemovalRelations:
				{
					return ForwardIrpToAttachedFdoAndForget(DeviceObject, Irp);
				}
				case TargetDeviceRelation:
				{
					PDEVICE_RELATIONS DeviceRelations = NULL;
					TRACE_(SERENUM, "IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / TargetDeviceRelation\n");
					Status = SerenumPdoQueryDeviceRelations(DeviceObject, &DeviceRelations);
					Information = (ULONG_PTR)DeviceRelations;
					break;
				}
				default:
				{
					WARN_(SERENUM, "IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / Unknown type 0x%lx\n",
						Stack->Parameters.QueryDeviceRelations.Type);
					ASSERT(FALSE);
					Status = STATUS_NOT_IMPLEMENTED;
					break;
				}
			}
			break;
		}
		case IRP_MN_QUERY_CAPABILITIES: /* 0x9 */
		{
			PDEVICE_CAPABILITIES DeviceCapabilities;
			ULONG i;
			TRACE_(SERENUM, "IRP_MJ_PNP / IRP_MN_QUERY_CAPABILITIES\n");

			DeviceCapabilities = (PDEVICE_CAPABILITIES)Stack->Parameters.DeviceCapabilities.Capabilities;
			/* FIXME: capabilities can change with connected device */
			DeviceCapabilities->LockSupported = FALSE;
			DeviceCapabilities->EjectSupported = FALSE;
			DeviceCapabilities->Removable = TRUE;
			DeviceCapabilities->DockDevice = FALSE;
			DeviceCapabilities->UniqueID = FALSE;
			DeviceCapabilities->SilentInstall = FALSE;
			DeviceCapabilities->RawDeviceOK = TRUE;
			DeviceCapabilities->SurpriseRemovalOK = TRUE;
			DeviceCapabilities->HardwareDisabled = FALSE; /* FIXME */
			//DeviceCapabilities->NoDisplayInUI = FALSE; /* FIXME */
			DeviceCapabilities->DeviceState[0] = PowerDeviceD0; /* FIXME */
			for (i = 0; i < PowerSystemMaximum; i++)
				DeviceCapabilities->DeviceState[i] = PowerDeviceD3; /* FIXME */
			//DeviceCapabilities->DeviceWake = PowerDeviceUndefined; /* FIXME */
			DeviceCapabilities->D1Latency = 0; /* FIXME */
			DeviceCapabilities->D2Latency = 0; /* FIXME */
			DeviceCapabilities->D3Latency = 0; /* FIXME */
			Status = STATUS_SUCCESS;
			break;
		}
		case IRP_MN_QUERY_RESOURCES: /* 0xa */
		{
			TRACE_(SERENUM, "IRP_MJ_PNP / IRP_MN_QUERY_RESOURCES\n");
			/* Serial devices don't need resources, except the ones of
			 * the serial port. This PDO is the serial device PDO, so
			 * report no resource by not changing Information and
			 * Status
			 */
			Information = Irp->IoStatus.Information;
			Status = Irp->IoStatus.Status;
			break;
		}
		case IRP_MN_QUERY_RESOURCE_REQUIREMENTS: /* 0xb */
		{
			TRACE_(SERENUM, "IRP_MJ_PNP / IRP_MN_QUERY_RESOURCE_REQUIREMENTS\n");
			/* Serial devices don't need resources, except the ones of
			 * the serial port. This PDO is the serial device PDO, so
			 * report no resource by not changing Information and
			 * Status
			 */
			Information = Irp->IoStatus.Information;
			Status = Irp->IoStatus.Status;
			break;
		}
		case IRP_MN_QUERY_DEVICE_TEXT: /* 0xc */
		{
			switch (Stack->Parameters.QueryDeviceText.DeviceTextType)
			{
				case DeviceTextDescription:
				{
					PUNICODE_STRING Source;
					PWSTR Description;
					TRACE_(SERENUM, "IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_TEXT / DeviceTextDescription\n");

					Source = &((PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->DeviceDescription;
					Description = ExAllocatePoolWithTag(PagedPool, Source->Length + sizeof(WCHAR), SERENUM_TAG);
					if (!Description)
						Status = STATUS_INSUFFICIENT_RESOURCES;
					else
					{
						RtlCopyMemory(Description, Source->Buffer, Source->Length);
						Description[Source->Length / sizeof(WCHAR)] = L'\0';
						Information = (ULONG_PTR)Description;
						Status = STATUS_SUCCESS;
					}
					break;
				}
				case DeviceTextLocationInformation:
				{
					/* We don't have any text location to report,
					 * and this query is optional, so ignore it.
					 */
					Information = Irp->IoStatus.Information;
					Status = Irp->IoStatus.Status;
					break;
				}
				default:
				{
					WARN_(SERENUM, "IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_TEXT / unknown type 0x%lx\n",
						Stack->Parameters.QueryDeviceText.DeviceTextType);
					ASSERT(FALSE);
					Status = STATUS_NOT_SUPPORTED;
				}
			}
			break;
		}
		case IRP_MN_FILTER_RESOURCE_REQUIREMENTS: /* 0xd */
		{
			return ForwardIrpToAttachedFdoAndForget(DeviceObject, Irp);
		}
		case IRP_MN_QUERY_ID: /* 0x13 */
		{
			Status = SerenumPdoQueryId(DeviceObject, Irp, &Information);
			break;
		}
		case IRP_MN_QUERY_BUS_INFORMATION: /* 0x15 */
		{
			PPNP_BUS_INFORMATION BusInfo;
			TRACE_(SERENUM, "IRP_MJ_PNP / IRP_MN_QUERY_BUS_INFORMATION\n");

			BusInfo = (PPNP_BUS_INFORMATION)ExAllocatePoolWithTag(PagedPool, sizeof(PNP_BUS_INFORMATION), SERENUM_TAG);
			if (!BusInfo)
				Status = STATUS_INSUFFICIENT_RESOURCES;
			else
			{
				memcpy(
					&BusInfo->BusTypeGuid,
					&GUID_DEVINTERFACE_SERENUM_BUS_ENUMERATOR,
					sizeof(BusInfo->BusTypeGuid));
				BusInfo->LegacyBusType = PNPBus;
				/* We're the only serial bus enumerator on the computer */
				BusInfo->BusNumber = 0;
				Information = (ULONG_PTR)BusInfo;
				Status = STATUS_SUCCESS;
			}
			break;
		}
		default:
		{
			/* We can't forward request to the lower driver, because
			 * we are a Pdo, so we don't have lower driver... */
			WARN_(SERENUM, "IRP_MJ_PNP / unknown minor function 0x%lx\n", MinorFunction);
			ASSERT(FALSE);
			Information = Irp->IoStatus.Information;
			Status = Irp->IoStatus.Status;
		}
	}

	Irp->IoStatus.Information = Information;
	Irp->IoStatus.Status = Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}
