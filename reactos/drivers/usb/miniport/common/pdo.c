/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS USB miniport driver (Cromwell type)
 * FILE:            drivers/usb/miniport/common/pdo.c
 * PURPOSE:         IRP_MJ_PNP/IRP_MJ_DEVICE_CONTROL operations for PDOs
 *
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.org),
 *                  James Tabor (jimtabor@adsl-64-217-116-74.dsl.hstntx.swbell.net)
 */

#define NDEBUG
#include <debug.h>

#include "usbcommon.h"

extern struct usb_driver hub_driver;

#define IO_METHOD_FROM_CTL_CODE(ctlCode) (ctlCode&0x00000003)

NTSTATUS
UsbMpDeviceControlPdo(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	PIO_STACK_LOCATION Stack;
	ULONG_PTR Information = 0;
	NTSTATUS Status;
	
	DPRINT("USBMP: UsbMpDeviceControlPdo() called\n");
	
	Stack = IoGetCurrentIrpStackLocation(Irp);
	Status = Irp->IoStatus.Status;
	
	switch (Stack->Parameters.DeviceIoControl.IoControlCode)
	{
		case IOCTL_INTERNAL_USB_GET_ROOT_USB_DEVICE:
		{
			PUSBMP_DEVICE_EXTENSION DeviceExtension;
			
			DPRINT("USBMP: IOCTL_INTERNAL_USB_GET_ROOT_USB_DEVICE\n");
			if (Irp->AssociatedIrp.SystemBuffer == NULL
				|| Stack->Parameters.DeviceIoControl.OutputBufferLength != sizeof(PVOID))
			{
				Status = STATUS_INVALID_PARAMETER;
			}
			else
			{
				PVOID* pRootHubPointer;
				DeviceExtension = (PUSBMP_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
				DeviceExtension = (PUSBMP_DEVICE_EXTENSION)DeviceExtension->FunctionalDeviceObject->DeviceExtension;
				
				pRootHubPointer = (PVOID*)Irp->AssociatedIrp.SystemBuffer;
				*pRootHubPointer = ((struct usb_hcd*)DeviceExtension->pdev->data)->self.root_hub;
				Information = sizeof(PVOID);
				Status = STATUS_SUCCESS;
			}
			break;
		}
		default:
		{
			DPRINT1("USBMP: Unknown IOCTL code 0x%lx\n", Stack->Parameters.DeviceIoControl.IoControlCode);
			Information = Irp->IoStatus.Information;
			Status = Irp->IoStatus.Status;
		}
	}
	
	Irp->IoStatus.Information = Information;
	Irp->IoStatus.Status = Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}

static NTSTATUS
UsbMpPdoQueryId(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp,
	OUT ULONG_PTR* Information)
{
	PUSBMP_DEVICE_EXTENSION DeviceExtension;
	ULONG IdType;
	UNICODE_STRING SourceString;
	UNICODE_STRING String;
	struct usb_device *roothub;
	NTSTATUS Status = STATUS_SUCCESS;

	IdType = IoGetCurrentIrpStackLocation(Irp)->Parameters.QueryId.IdType;
	DeviceExtension = (PUSBMP_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	RtlInitUnicodeString(&String, NULL);
	DeviceExtension = (PUSBMP_DEVICE_EXTENSION)DeviceExtension->FunctionalDeviceObject->DeviceExtension;
	roothub = ((struct usb_hcd*)DeviceExtension->pdev->data)->self.root_hub;

	switch (IdType)
	{
		case BusQueryDeviceID:
		{
			DPRINT("USBMP: IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryDeviceID\n");
			if (roothub->speed == USB_SPEED_LOW || roothub->speed == USB_SPEED_FULL)
				RtlInitUnicodeString(&SourceString, L"USB\\ROOT_HUB"); /* USB 1.1 */
			else
				RtlInitUnicodeString(&SourceString, L"USB\\ROOT_HUB20"); /* USB 2.0 */
			break;
		}
		case BusQueryHardwareIDs:
		{
			CHAR Buffer[2][40];
			PCHAR RootHubName;
			PCI_COMMON_CONFIG PciData;
			ULONG BusNumber, SlotNumber;
			ULONG ret;
			PDEVICE_OBJECT Pdo;

			DPRINT("USBMP: IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryHardwareIDs\n");

			Pdo = DeviceExtension->PhysicalDeviceObject;
			Status = IoGetDeviceProperty(
				Pdo,
				DevicePropertyBusNumber,
				sizeof(ULONG),
				&BusNumber,
				&ret);
			if (!NT_SUCCESS(Status))
			{
				DPRINT("USBMP: IoGetDeviceProperty() failed with status 0x%08lx\n", Status);
				break;
			}

			Status = IoGetDeviceProperty(
				Pdo,
				DevicePropertyAddress,
				sizeof(ULONG),
				&SlotNumber,
				&ret);
			if (!NT_SUCCESS(Status))
			{
				DPRINT("USBMP: IoGetDeviceProperty() failed with status 0x%08lx\n", Status);
				break;
			}

			ret = HalGetBusDataByOffset(PCIConfiguration,
				BusNumber,
				SlotNumber,
				&PciData,
				0,
				PCI_COMMON_HDR_LENGTH);
			if (ret != PCI_COMMON_HDR_LENGTH)
			{
				DPRINT("USBMP: HalGetBusDataByOffset() failed (ret = %ld)\n", ret);
				Status = STATUS_IO_DEVICE_ERROR;
				break;
			}

			sprintf(Buffer[0], "USB\\VID%04X&PID%04X&REV%04X",
				PciData.VendorID, PciData.DeviceID, PciData.RevisionID);
			sprintf(Buffer[1], "USB\\VID%04X&PID%04X",
				PciData.VendorID, PciData.DeviceID);
			if (roothub->speed == USB_SPEED_LOW || roothub->speed == USB_SPEED_FULL)
				RootHubName = "USB\\ROOT_HUB"; /* USB 1.1 */
			else
				RootHubName = "USB\\ROOT_HUB20"; /* USB 2.0 */
			Status = UsbMpInitMultiSzString(
				&SourceString,
				Buffer[0], Buffer[1], RootHubName, NULL);
			break;
		}
		case BusQueryCompatibleIDs:
			DPRINT("USBMP: IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryCompatibleIDs\n");
			/* No compatible ID */
			*Information = 0;
			return STATUS_NOT_SUPPORTED;
		case BusQueryInstanceID:
		{
			DPRINT("USBMP: IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryInstanceID\n");
			*Information = 0;
			return Status;
		}
		default:
			DPRINT1("USBMP: IRP_MJ_PNP / IRP_MN_QUERY_ID / unknown query id type 0x%lx\n", IdType);
			return STATUS_NOT_SUPPORTED;
	}

	if (NT_SUCCESS(Status))
	{
		Status = UsbMpDuplicateUnicodeString(
			&String,
			&SourceString,
			PagedPool);
		*Information = (ULONG_PTR)String.Buffer;
	}
	return Status;
}

static NTSTATUS
UsbMpPnpStartDevice(
	IN PDEVICE_OBJECT DeviceObject)
{
	PUSBMP_DEVICE_EXTENSION DeviceExtension;
	NTSTATUS Status;
	
	DeviceExtension = (PUSBMP_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	
	/* Register device interface for root hub */
	Status = IoRegisterDeviceInterface(
		DeviceObject,
		&GUID_DEVINTERFACE_USB_HUB,
		NULL,
		&DeviceExtension->HcdInterfaceName);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("USBMP: IoRegisterDeviceInterface() failed with status 0x%08lx\n", Status);
		return Status;
	}
	
	return Status;
}

NTSTATUS STDCALL
UsbMpPnpPdo(
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
		case IRP_MN_START_DEVICE: /* 0x00 */
		{
			DPRINT("USBMP: IRP_MJ_PNP / IRP_MN_START_DEVICE\n");
			Status = UsbMpPnpStartDevice(DeviceObject);
			break;
		}
		case IRP_MN_QUERY_CAPABILITIES: /* 0x09 */
		{
			PDEVICE_CAPABILITIES DeviceCapabilities;
			ULONG i;
			DPRINT("USBMP: IRP_MJ_PNP / IRP_MN_QUERY_CAPABILITIES\n");

			DeviceCapabilities = (PDEVICE_CAPABILITIES)Stack->Parameters.DeviceCapabilities.Capabilities;
			/* FIXME: capabilities can change with connected device */
			DeviceCapabilities->LockSupported = FALSE;
			DeviceCapabilities->EjectSupported = FALSE;
			DeviceCapabilities->Removable = FALSE;
			DeviceCapabilities->DockDevice = FALSE;
			DeviceCapabilities->UniqueID = FALSE;
			DeviceCapabilities->SilentInstall = TRUE;
			DeviceCapabilities->RawDeviceOK = FALSE;
			DeviceCapabilities->SurpriseRemovalOK = FALSE;
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
			DPRINT("USBMP: IRP_MJ_PNP / IRP_MN_QUERY_RESOURCES\n");
			/* Root buses don't need resources, except the ones of
			 * the usb controller. This PDO is the root bus PDO, so
			 * report no resource by not changing Information and
			 * Status
			 */
			Information = Irp->IoStatus.Information;
			Status = Irp->IoStatus.Status;
			break;
		}
		case IRP_MN_QUERY_RESOURCE_REQUIREMENTS: /* 0x0b */
		{
			DPRINT("USBMP: IRP_MJ_PNP / IRP_MN_QUERY_RESOURCE_REQUIREMENTS\n");
			/* Root buses don't need resources, except the ones of
			 * the usb controller. This PDO is the root bus PDO, so
			 * report no resource by not changing Information and
			 * Status
			 */
			Information = Irp->IoStatus.Information;
			Status = Irp->IoStatus.Status;
			break;
		}
		case IRP_MN_QUERY_DEVICE_TEXT: /* 0x0c */
		{
			switch (Stack->Parameters.QueryDeviceText.DeviceTextType)
			{
				case DeviceTextDescription:
				{
					UNICODE_STRING SourceString = RTL_CONSTANT_STRING(L"Root USB hub");
					UNICODE_STRING Description;
					
					DPRINT("USBMP: IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_TEXT / DeviceTextDescription\n");
					
					Status = RtlDuplicateUnicodeString(RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE, &SourceString, &Description);
					if (NT_SUCCESS(Status))
						Information = (ULONG_PTR)Description.Buffer;
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
					DPRINT1("USBMP: IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_TEXT / unknown type 0x%lx\n",
						Stack->Parameters.QueryDeviceText.DeviceTextType);
					Status = STATUS_NOT_SUPPORTED;
				}
			}
			break;
		}
		case IRP_MN_QUERY_ID: /* 0x13 */
		{
			Status = UsbMpPdoQueryId(DeviceObject, Irp, &Information);
			break;
		}
		default:
		{
			/* We can't forward request to the lower driver, because
			 * we are a Pdo, so we don't have lower driver...
			 */
			DPRINT1("USBMP: IRP_MJ_PNP / unknown minor function 0x%lx\n", MinorFunction);
			Information = Irp->IoStatus.Information;
			Status = Irp->IoStatus.Status;
		}
	}

	Irp->IoStatus.Information = Information;
	Irp->IoStatus.Status = Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}

