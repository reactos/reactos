/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         PCI IDE bus driver extension
 * FILE:            drivers/storage/pciidex/pdo.c
 * PURPOSE:         IRP_MJ_PNP operations for PDOs
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.org)
 */

#include "pciidex.h"

#include <stdio.h>

#define NDEBUG
#include <debug.h>

static NTSTATUS
PciIdeXPdoQueryId(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp,
	OUT ULONG_PTR* Information)
{
	PPDO_DEVICE_EXTENSION DeviceExtension;
	PFDO_DEVICE_EXTENSION FdoDeviceExtension;
	WCHAR Buffer[256];
	ULONG Index = 0;
	ULONG IdType;
	UNICODE_STRING SourceString;
	UNICODE_STRING String;
	NTSTATUS Status;

	IdType = IoGetCurrentIrpStackLocation(Irp)->Parameters.QueryId.IdType;
	DeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	FdoDeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceExtension->ControllerFdo->DeviceExtension;

	switch (IdType)
	{
		case BusQueryDeviceID:
		{
			DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryDeviceID\n");
			RtlInitUnicodeString(&SourceString, L"PCIIDE\\IDEChannel");
			break;
		}
		case BusQueryHardwareIDs:
		{
			DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryHardwareIDs\n");

			switch (FdoDeviceExtension->VendorId)
			{
				case 0x0e11:
					Index += swprintf(&Buffer[Index], L"Compaq-%04x", FdoDeviceExtension->DeviceId) + 1;
					break;
				case 0x1039:
					Index += swprintf(&Buffer[Index], L"SiS-%04x", FdoDeviceExtension->DeviceId) + 1;
					break;
				case 0x1050:
					Index += swprintf(&Buffer[Index], L"WinBond-%04x", FdoDeviceExtension->DeviceId) + 1;
					break;
				case 0x1095:
					Index += swprintf(&Buffer[Index], L"CMD-%04x", FdoDeviceExtension->DeviceId) + 1;
					break;
				case 0x8086:
				{
					switch (FdoDeviceExtension->DeviceId)
					{
						case 0x1230:
							Index += swprintf(&Buffer[Index], L"Intel-PIIX") + 1;
							break;
						case 0x7010:
							Index += swprintf(&Buffer[Index], L"Intel-PIIX3") + 1;
							break;
						case 0x7111:
							Index += swprintf(&Buffer[Index], L"Intel-PIIX4") + 1;
							break;
						default:
							Index += swprintf(&Buffer[Index], L"Intel-%04x", FdoDeviceExtension->DeviceId) + 1;
							break;
					}
					break;
				}
				default:
					break;
			}
			if (DeviceExtension->Channel == 0)
				Index += swprintf(&Buffer[Index], L"Primary_IDE_Channel") + 1;
			else
				Index += swprintf(&Buffer[Index], L"Secondary_IDE_Channel") + 1;
			Index += swprintf(&Buffer[Index], L"*PNP0600") + 1;
			Buffer[Index] = UNICODE_NULL;
			SourceString.Length = SourceString.MaximumLength = Index * sizeof(WCHAR);
			SourceString.Buffer = Buffer;
			break;
		}
		case BusQueryCompatibleIDs:
		{
			DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryCompatibleIDs\n");

			Index += swprintf(&Buffer[Index], L"*PNP0600") + 1;
			Buffer[Index] = UNICODE_NULL;
			SourceString.Length = SourceString.MaximumLength = Index * sizeof(WCHAR);
			SourceString.Buffer = Buffer;
			break;
		}
		case BusQueryInstanceID:
		{
			DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryInstanceID\n");
			swprintf(Buffer, L"%lu", DeviceExtension->Channel);
			RtlInitUnicodeString(&SourceString, Buffer);
			break;
		}
		default:
			DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_ID / unknown query id type 0x%lx\n", IdType);
			ASSERT(FALSE);
			return STATUS_NOT_SUPPORTED;
	}

	Status = DuplicateUnicodeString(
		RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE,
		&SourceString,
		&String);
	*Information = (ULONG_PTR)String.Buffer;
	return Status;
}

static NTSTATUS
GetCurrentResources(
	IN PDEVICE_OBJECT DeviceObject,
	OUT PULONG CommandPortBase,
	OUT PULONG ControlPortBase,
	OUT PULONG BusMasterPortBase,
	OUT PULONG InterruptVector)
{
	PPDO_DEVICE_EXTENSION DeviceExtension;
	PFDO_DEVICE_EXTENSION FdoDeviceExtension;
	ULONG BaseIndex;
	ULONG BytesRead;
	PCI_COMMON_CONFIG PciConfig;
	NTSTATUS ret = STATUS_UNSUCCESSFUL;

	DeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	FdoDeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceExtension->ControllerFdo->DeviceExtension;
	BaseIndex = DeviceExtension->Channel * 2;

	BytesRead = (*FdoDeviceExtension->BusInterface->GetBusData)(
		FdoDeviceExtension->BusInterface->Context,
		PCI_WHICHSPACE_CONFIG,
		&PciConfig,
		0,
		PCI_COMMON_HDR_LENGTH);
	if (BytesRead != PCI_COMMON_HDR_LENGTH)
		return STATUS_IO_DEVICE_ERROR;

	/* We have found a known native pci ide controller */
	if ((PciConfig.ProgIf & 0x80) && (PciConfig.u.type0.BaseAddresses[4] & PCI_ADDRESS_IO_SPACE))
	{
		DPRINT("Found IDE Bus Master controller!\n");
		*BusMasterPortBase = PciConfig.u.type0.BaseAddresses[4] & PCI_ADDRESS_IO_ADDRESS_MASK;
		DPRINT("  IDE Bus Master Registers at IO %lx\n", *BusMasterPortBase);
	}
	else
	{
		*BusMasterPortBase = 0;
	}

	if ((PciConfig.ProgIf >> BaseIndex) & 0x1)
	{
		/* Native mode */
		if ((PciConfig.u.type0.BaseAddresses[BaseIndex + 0] & PCI_ADDRESS_IO_SPACE) &&
		    (PciConfig.u.type0.BaseAddresses[BaseIndex + 1] & PCI_ADDRESS_IO_SPACE))
		{
			/* Channel is enabled */
			*CommandPortBase = PciConfig.u.type0.BaseAddresses[BaseIndex + 0] & PCI_ADDRESS_IO_ADDRESS_MASK;
			*ControlPortBase = PciConfig.u.type0.BaseAddresses[BaseIndex + 1] & PCI_ADDRESS_IO_ADDRESS_MASK;
			*InterruptVector = PciConfig.u.type0.InterruptLine;
			ret = STATUS_SUCCESS;
		}
	}
	else
	{
		/* Compatibility mode */
		switch (DeviceExtension->Channel)
		{
			case 0:
				if (IoGetConfigurationInformation()->AtDiskPrimaryAddressClaimed)
					ret = STATUS_INSUFFICIENT_RESOURCES;
				else
				{
					*CommandPortBase = 0x1F0;
					*ControlPortBase = 0x3F6;
					*InterruptVector = 14;
					ret = STATUS_SUCCESS;
				}
				break;
			case 1:
				if (IoGetConfigurationInformation()->AtDiskSecondaryAddressClaimed)
					ret = STATUS_INSUFFICIENT_RESOURCES;
				else
				{
					*CommandPortBase = 0x170;
					*ControlPortBase = 0x376;
					*InterruptVector = 15;
					ret = STATUS_SUCCESS;
				}
				break;
		}
	}

	return ret;
}

static NTSTATUS
PciIdeXPdoQueryResourceRequirements(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp,
	OUT ULONG_PTR* Information)
{
	ULONG CommandPortBase;
	ULONG ControlPortBase;
	ULONG BusMasterPortBase;
	ULONG InterruptVector;
	ULONG ListSize;
	PIO_RESOURCE_REQUIREMENTS_LIST RequirementsList;
	PIO_RESOURCE_DESCRIPTOR Descriptor;
	NTSTATUS Status;

	Status = GetCurrentResources(DeviceObject, &CommandPortBase,
		&ControlPortBase, &BusMasterPortBase, &InterruptVector);
	if (!NT_SUCCESS(Status))
		return Status;

	DPRINT("IDE Channel %lu: IO %x and %x, BM %lx, Irq %lu\n",
		((PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->Channel,
		CommandPortBase, ControlPortBase,
		BusMasterPortBase, InterruptVector);

	/* FIXME: what to do with BusMasterPortBase? */

	ListSize = sizeof(IO_RESOURCE_REQUIREMENTS_LIST)
		+ 2 * sizeof(IO_RESOURCE_DESCRIPTOR);
	RequirementsList = ExAllocatePool(PagedPool, ListSize);
	if (!RequirementsList)
		return STATUS_INSUFFICIENT_RESOURCES;

	RtlZeroMemory(RequirementsList, ListSize);
	RequirementsList->ListSize = ListSize;
	RequirementsList->AlternativeLists = 1;

	RequirementsList->List[0].Version = 1;
	RequirementsList->List[0].Revision = 1;
	RequirementsList->List[0].Count = 3;

	Descriptor = &RequirementsList->List[0].Descriptors[0];

	/* Command port base */
	Descriptor->Option = 0; /* Required */
	Descriptor->Type = CmResourceTypePort;
	Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
	Descriptor->Flags = CM_RESOURCE_PORT_IO |
	                    CM_RESOURCE_PORT_16_BIT_DECODE |
	                    CM_RESOURCE_PORT_POSITIVE_DECODE;
	Descriptor->u.Port.Length = 8;
	Descriptor->u.Port.Alignment = 1;
	Descriptor->u.Port.MinimumAddress.QuadPart = (ULONGLONG)CommandPortBase;
	Descriptor->u.Port.MaximumAddress.QuadPart = (ULONGLONG)(CommandPortBase + Descriptor->u.Port.Length - 1);
	Descriptor++;

	/* Control port base */
	Descriptor->Option = 0; /* Required */
	Descriptor->Type = CmResourceTypePort;
	Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
	Descriptor->Flags = CM_RESOURCE_PORT_IO |
	                    CM_RESOURCE_PORT_16_BIT_DECODE |
	                    CM_RESOURCE_PORT_POSITIVE_DECODE;
	Descriptor->u.Port.Length = 1;
	Descriptor->u.Port.Alignment = 1;
	Descriptor->u.Port.MinimumAddress.QuadPart = (ULONGLONG)ControlPortBase;
	Descriptor->u.Port.MaximumAddress.QuadPart = (ULONGLONG)(ControlPortBase + Descriptor->u.Port.Length - 1);
	Descriptor++;

	/* Interrupt */
	Descriptor->Option = 0; /* Required */
	Descriptor->Type = CmResourceTypeInterrupt;
	Descriptor->ShareDisposition = CmResourceShareShared;
	Descriptor->Flags = CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;
	Descriptor->u.Interrupt.MinimumVector = InterruptVector;
	Descriptor->u.Interrupt.MaximumVector = InterruptVector;

	*Information = (ULONG_PTR)RequirementsList;
	return STATUS_SUCCESS;
}

static NTSTATUS
PciIdeXPdoQueryDeviceText(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp,
	OUT ULONG_PTR* Information)
{
	PPDO_DEVICE_EXTENSION DeviceExtension;
	ULONG DeviceTextType;
	PCWSTR SourceString;
	UNICODE_STRING String;

	DeviceTextType = IoGetCurrentIrpStackLocation(Irp)->Parameters.QueryDeviceText.DeviceTextType;
	DeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	switch (DeviceTextType)
	{
		case DeviceTextDescription:
		case DeviceTextLocationInformation:
		{
			DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_TEXT / %S\n",
				DeviceTextType == DeviceTextDescription ? L"DeviceTextDescription" : L"DeviceTextLocationInformation");
			if (DeviceExtension->Channel == 0)
				SourceString = L"Primary channel";
			else
				SourceString = L"Secondary channel";
			break;
		}
		default:
			DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_TEXT / unknown type 0x%lx\n", DeviceTextType);
			ASSERT(FALSE);
			return STATUS_NOT_SUPPORTED;
	}

	if (RtlCreateUnicodeString(&String, SourceString))
	{
		*Information = (ULONG_PTR)String.Buffer;
		return STATUS_SUCCESS;
	}
	else
		return STATUS_INSUFFICIENT_RESOURCES;
}

static NTSTATUS
PciIdeXPdoQueryDeviceRelations(
	IN PDEVICE_OBJECT DeviceObject,
	OUT PDEVICE_RELATIONS* pDeviceRelations)
{
	PFDO_DEVICE_EXTENSION DeviceExtension;
	PDEVICE_RELATIONS DeviceRelations;

	DeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	ASSERT(DeviceExtension->Common.IsFDO);

	DeviceRelations = (PDEVICE_RELATIONS)ExAllocatePool(
		PagedPool,
		sizeof(DEVICE_RELATIONS));
	if (!DeviceRelations)
		return STATUS_INSUFFICIENT_RESOURCES;

	ObReferenceObject(DeviceObject);
	DeviceRelations->Count = 1;
	DeviceRelations->Objects[0] = DeviceObject;

	*pDeviceRelations = DeviceRelations;
	return STATUS_SUCCESS;
}

NTSTATUS NTAPI
PciIdeXPdoPnpDispatch(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	ULONG MinorFunction;
	PIO_STACK_LOCATION Stack;
	ULONG_PTR Information = Irp->IoStatus.Information;
	NTSTATUS Status;

	Stack = IoGetCurrentIrpStackLocation(Irp);
	MinorFunction = Stack->MinorFunction;

	switch (MinorFunction)
	{
		/* FIXME:
		 * Those are required:
		 *    IRP_MN_START_DEVICE (done)
		 *    IRP_MN_QUERY_STOP_DEVICE
		 *    IRP_MN_STOP_DEVICE
		 *    IRP_MN_CANCEL_STOP_DEVICE
		 *    IRP_MN_QUERY_REMOVE_DEVICE
		 *    IRP_MN_REMOVE_DEVICE
		 *    IRP_MN_CANCEL_REMOVE_DEVICE
		 *    IRP_MN_SURPRISE_REMOVAL
		 *    IRP_MN_QUERY_CAPABILITIES (done)
		 *    IRP_MN_QUERY_DEVICE_RELATIONS / TargetDeviceRelations (done)
		 *    IRP_MN_QUERY_ID / BusQueryDeviceID (done)
		 * Those may be required/optional:
		 *    IRP_MN_DEVICE_USAGE_NOTIFICATION
		 *    IRP_MN_QUERY_RESOURCES
		 *    IRP_MN_QUERY_RESOURCE_REQUIREMENTS (done)
		 *    IRP_MN_QUERY_DEVICE_TEXT
		 *    IRP_MN_QUERY_BUS_INFORMATION
		 *    IRP_MN_QUERY_INTERFACE
		 *    IRP_MN_READ_CONFIG
		 *    IRP_MN_WRITE_CONFIG
		 *    IRP_MN_EJECT
		 *    IRP_MN_SET_LOCK
		 * Those are optional:
		 *    IRP_MN_QUERY_DEVICE_RELATIONS / EjectionRelations
		 *    IRP_MN_QUERY_ID / BusQueryHardwareIDs (done)
		 *    IRP_MN_QUERY_ID / BusQueryCompatibleIDs (done)
		 *    IRP_MN_QUERY_ID / BusQueryInstanceID (done)
		 */
		case IRP_MN_START_DEVICE: /* 0x00 */
		{
			DPRINT("IRP_MJ_PNP / IRP_MN_START_DEVICE\n");
			Status = STATUS_SUCCESS;
			break;
		}
                case IRP_MN_QUERY_REMOVE_DEVICE: /* 0x01 */
                {
                        DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_REMOVE_DEVICE\n");
                        Status = STATUS_UNSUCCESSFUL;
                        break;
                }
		case IRP_MN_QUERY_DEVICE_RELATIONS: /* 0x07 */
		{
			switch (Stack->Parameters.QueryDeviceRelations.Type)
			{
				case TargetDeviceRelation:
				{
					PDEVICE_RELATIONS DeviceRelations = NULL;
					DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / TargetDeviceRelation\n");
					Status = PciIdeXPdoQueryDeviceRelations(DeviceObject, &DeviceRelations);
					Information = (ULONG_PTR)DeviceRelations;
					break;
				}
				default:
				{
					DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / Unknown type 0x%lx\n",
						Stack->Parameters.QueryDeviceRelations.Type);
					Status = Irp->IoStatus.Status;
					break;
				}
			}
			break;
		}
		case IRP_MN_QUERY_CAPABILITIES: /* 0x09 */
		{
			PDEVICE_CAPABILITIES DeviceCapabilities;
			ULONG i;
			DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_CAPABILITIES\n");

			DeviceCapabilities = (PDEVICE_CAPABILITIES)Stack->Parameters.DeviceCapabilities.Capabilities;
			/* FIXME: capabilities can change with connected device */
			DeviceCapabilities->LockSupported = FALSE;
			DeviceCapabilities->EjectSupported = FALSE;
			DeviceCapabilities->Removable = TRUE;
			DeviceCapabilities->DockDevice = FALSE;
			DeviceCapabilities->UniqueID = FALSE;
			DeviceCapabilities->SilentInstall = FALSE;
			DeviceCapabilities->RawDeviceOK = FALSE;
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
		case IRP_MN_QUERY_RESOURCES: /* 0x0a */
		{
			/* This IRP is optional; do nothing */
			Information = Irp->IoStatus.Information;
			Status = Irp->IoStatus.Status;
			break;
		}
		case IRP_MN_QUERY_RESOURCE_REQUIREMENTS: /* 0x0b */
		{
			DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_RESOURCE_REQUIREMENTS\n");
			Status = PciIdeXPdoQueryResourceRequirements(DeviceObject, Irp, &Information);
			break;
		}
		case IRP_MN_QUERY_DEVICE_TEXT: /* 0x0c */
		{
			Status = PciIdeXPdoQueryDeviceText(DeviceObject, Irp, &Information);
			break;
		}
		case IRP_MN_FILTER_RESOURCE_REQUIREMENTS: /* 0x0d */
		{
			DPRINT("IRP_MJ_PNP / IRP_MN_FILTER_RESOURCE_REQUIREMENTS\n");
			Information = Irp->IoStatus.Information;
			Status = Irp->IoStatus.Status;
			break;
		}
		case IRP_MN_QUERY_ID: /* 0x13 */
		{
			Status = PciIdeXPdoQueryId(DeviceObject, Irp, &Information);
			break;
		}
                case IRP_MN_QUERY_PNP_DEVICE_STATE: /* 0x14 */
                {
                        DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_PNP_DEVICE_STATE\n");
                        Information |= PNP_DEVICE_NOT_DISABLEABLE;
                        Status = STATUS_SUCCESS;
                        break;
                }
		case IRP_MN_QUERY_BUS_INFORMATION: /* 0x15 */
		{
			PPNP_BUS_INFORMATION BusInfo;
			DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_BUS_INFORMATION\n");

			BusInfo = (PPNP_BUS_INFORMATION)ExAllocatePool(PagedPool, sizeof(PNP_BUS_INFORMATION));
			if (!BusInfo)
				Status = STATUS_INSUFFICIENT_RESOURCES;
			else
			{
				/*RtlCopyMemory(
					&BusInfo->BusTypeGuid,
					&GUID_DEVINTERFACE_XXX,
					sizeof(GUID));*/
				BusInfo->LegacyBusType = PNPBus;
				BusInfo->BusNumber = 0; /* FIXME */
				Information = (ULONG_PTR)BusInfo;
				Status = STATUS_SUCCESS;
			}
			break;
		}
		default:
		{
			/* We can't forward request to the lower driver, because
			 * we are a Pdo, so we don't have lower driver... */
			DPRINT1("IRP_MJ_PNP / Unknown minor function 0x%lx\n", MinorFunction);
			Information = Irp->IoStatus.Information;
			Status = Irp->IoStatus.Status;
		}
	}

	Irp->IoStatus.Information = Information;
	Irp->IoStatus.Status = Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}
