/*
 * PROJECT:     ReactOS USB miniport driver (Cromwell type)
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/miniport/common/pdo.c
 * PURPOSE:     Operations on PDOs
 * PROGRAMMERS: Copyright 2005-2006 Hervé Poussineau (hpoussin@reactos.org)
 *              Copyright James Tabor (jimtabor@adsl-64-217-116-74.dsl.hstntx.swbell.net)
 */

#define NDEBUG
#include <debug.h>

#include "usbcommon.h"
#include <wdmguid.h>

extern struct usb_driver hub_driver;

#define IO_METHOD_FROM_CTL_CODE(ctlCode) (ctlCode&0x00000003)

NTSTATUS
UsbMpPdoCreate(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	DPRINT("IRP_MJ_CREATE\n");

	/* Nothing to do */
	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS
UsbMpPdoClose(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	DPRINT("IRP_MJ_CLOSE\n");

	/* Nothing to do */
	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS
UsbMpPdoCleanup(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	DPRINT("IRP_MJ_CLEANUP\n");

	/* Nothing to do */
	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS
UsbMpPdoDeviceControl(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	PIO_STACK_LOCATION Stack;
	ULONG_PTR Information = 0;
	NTSTATUS Status;

	DPRINT("UsbMpDeviceControlPdo() called\n");

	Stack = IoGetCurrentIrpStackLocation(Irp);
	Status = Irp->IoStatus.Status;

	switch (Stack->Parameters.DeviceIoControl.IoControlCode)
	{
		case IOCTL_INTERNAL_USB_GET_ROOT_USB_DEVICE:
		{
			PUSBMP_DEVICE_EXTENSION DeviceExtension;

			DPRINT("IOCTL_INTERNAL_USB_GET_ROOT_USB_DEVICE\n");
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
			DPRINT1("Unknown IOCTL code 0x%lx\n", Stack->Parameters.DeviceIoControl.IoControlCode);
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
QueryId(
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
			DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryDeviceID\n");
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

			DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryHardwareIDs\n");

			Pdo = DeviceExtension->PhysicalDeviceObject;
			Status = IoGetDeviceProperty(
				Pdo,
				DevicePropertyBusNumber,
				sizeof(ULONG),
				&BusNumber,
				&ret);
			if (!NT_SUCCESS(Status))
			{
				DPRINT("IoGetDeviceProperty() failed with status 0x%08lx\n", Status);
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
				DPRINT("IoGetDeviceProperty() failed with status 0x%08lx\n", Status);
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
				DPRINT("HalGetBusDataByOffset() failed (ret = %ld)\n", ret);
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
			DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryCompatibleIDs\n");
			/* No compatible ID */
			*Information = 0;
			return STATUS_NOT_SUPPORTED;
		case BusQueryInstanceID:
		{
			DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryInstanceID\n");
			*Information = 0;
			return Status;
		}
		default:
			DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_ID / unknown query id type 0x%lx\n", IdType);
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
QueryBusInformation(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp,
	OUT ULONG_PTR* Information)
{
	PPNP_BUS_INFORMATION BusInformation;

	DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_BUS_INFORMATION\n");

	BusInformation = ExAllocatePool(PagedPool, sizeof(PNP_BUS_INFORMATION));
	if (!BusInformation)
		return STATUS_INSUFFICIENT_RESOURCES;
	BusInformation->BusTypeGuid = GUID_BUS_TYPE_USB;
	BusInformation->LegacyBusType = PNPBus;
	BusInformation->BusNumber = 0; /* FIXME */
	return STATUS_SUCCESS;
}

static NTSTATUS
StartDevice(
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
		DPRINT("IoRegisterDeviceInterface() failed with status 0x%08lx\n", Status);
		return Status;
	}

	return Status;
}

NTSTATUS
UsbMpPdoPnp(
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
			DPRINT("IRP_MJ_PNP / IRP_MN_START_DEVICE\n");
			Status = StartDevice(DeviceObject);
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
			DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_RESOURCES\n");
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
			DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_RESOURCE_REQUIREMENTS\n");
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

					DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_TEXT / DeviceTextDescription\n");

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
					DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_TEXT / unknown type 0x%lx\n",
						Stack->Parameters.QueryDeviceText.DeviceTextType);
					ASSERT(FALSE);
					Status = STATUS_NOT_SUPPORTED;
				}
			}
			break;
		}
		case IRP_MN_QUERY_ID: /* 0x13 */
		{
			Status = QueryId(DeviceObject, Irp, &Information);
			break;
		}
		case IRP_MN_QUERY_BUS_INFORMATION: /* 0x15 */
		{
			Status = QueryBusInformation(DeviceObject, Irp, &Information);
			break;
		}
		default:
		{
			/* We can't forward request to the lower driver, because
			 * we are a Pdo, so we don't have lower driver...
			 */
			DPRINT1("IRP_MJ_PNP / unknown minor function 0x%lx\n", MinorFunction);
			Information = Irp->IoStatus.Information;
			Status = Irp->IoStatus.Status;
			ASSERT(FALSE);
		}
	}

	Irp->IoStatus.Information = Information;
	Irp->IoStatus.Status = Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}

NTSTATUS
UsbMpPdoInternalDeviceControl(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	NTSTATUS Status = STATUS_INVALID_DEVICE_REQUEST;

	DPRINT("UsbMpDeviceInternalControlPdo(DO %p, code 0x%lx) called\n",
		DeviceObject,
		IoGetCurrentIrpStackLocation(Irp)->Parameters.DeviceIoControl.IoControlCode);

	if (DeviceObject == KeyboardFdo)
	{
		// it's keyboard's IOCTL
		PIO_STACK_LOCATION Stk;

		Irp->IoStatus.Information = 0;
		Stk = IoGetCurrentIrpStackLocation(Irp);

		switch (Stk->Parameters.DeviceIoControl.IoControlCode)
		{
			case IOCTL_INTERNAL_KEYBOARD_CONNECT:
				DPRINT("IOCTL_INTERNAL_KEYBOARD_CONNECT\n");
				if (Stk->Parameters.DeviceIoControl.InputBufferLength <	sizeof(CONNECT_DATA)) {
					DPRINT1("Keyboard IOCTL_INTERNAL_KEYBOARD_CONNECT "
						"invalid buffer size\n");
					Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
					goto intcontfailure;
				}

				RtlCopyMemory(&KbdClassInformation,
					Stk->Parameters.DeviceIoControl.Type3InputBuffer,
					sizeof(CONNECT_DATA));

				Irp->IoStatus.Status = STATUS_SUCCESS;
				break;

		case IOCTL_INTERNAL_I8042_KEYBOARD_WRITE_BUFFER:
			DPRINT("IOCTL_INTERNAL_I8042_KEYBOARD_WRITE_BUFFER\n");
			if (Stk->Parameters.DeviceIoControl.InputBufferLength <	1) {
				Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
				goto intcontfailure;
			}
/*			if (!DevExt->KeyboardInterruptObject) {
				Irp->IoStatus.Status = STATUS_DEVICE_NOT_READY;
				goto intcontfailure;
			}*/

			Irp->IoStatus.Status = STATUS_SUCCESS;
			break;
		case IOCTL_KEYBOARD_QUERY_ATTRIBUTES:
			DPRINT("IOCTL_KEYBOARD_QUERY_ATTRIBUTES\n");
			if (Stk->Parameters.DeviceIoControl.OutputBufferLength <
				sizeof(KEYBOARD_ATTRIBUTES)) {
					DPRINT("Keyboard IOCTL_KEYBOARD_QUERY_ATTRIBUTES: "
						"invalid buffer size\n");
					Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
					goto intcontfailure;
				}
				/*RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,
					&DevExt->KeyboardAttributes,
					sizeof(KEYBOARD_ATTRIBUTES));*/

				Irp->IoStatus.Status = STATUS_SUCCESS;
				break;
		case IOCTL_KEYBOARD_QUERY_INDICATORS:
			DPRINT("IOCTL_KEYBOARD_QUERY_INDICATORS\n");
			if (Stk->Parameters.DeviceIoControl.OutputBufferLength <
				sizeof(KEYBOARD_INDICATOR_PARAMETERS)) {
					DPRINT("Keyboard IOCTL_KEYBOARD_QUERY_INDICATORS: "
						"invalid buffer size\n");
					Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
					goto intcontfailure;
				}
				/*RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,
					&DevExt->KeyboardIndicators,
					sizeof(KEYBOARD_INDICATOR_PARAMETERS));*/

				Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
				break;
		case IOCTL_KEYBOARD_QUERY_TYPEMATIC:
			DPRINT("IOCTL_KEYBOARD_QUERY_TYPEMATIC\n");
			if (Stk->Parameters.DeviceIoControl.OutputBufferLength <
				sizeof(KEYBOARD_TYPEMATIC_PARAMETERS)) {
					DPRINT("Keyboard IOCTL_KEYBOARD_QUERY_TYPEMATIC: "
						"invalid buffer size\n");
					Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
					goto intcontfailure;
				}
				/*RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,
					&DevExt->KeyboardTypematic,
					sizeof(KEYBOARD_TYPEMATIC_PARAMETERS));*/

				Irp->IoStatus.Status = STATUS_SUCCESS;
				break;
		case IOCTL_KEYBOARD_SET_INDICATORS:
			DPRINT("IOCTL_KEYBOARD_SET_INDICATORS\n");
			if (Stk->Parameters.DeviceIoControl.InputBufferLength <
				sizeof(KEYBOARD_INDICATOR_PARAMETERS)) {
					DPRINT("Keyboard IOCTL_KEYBOARD_SET_INDICTATORS: "
						"invalid buffer size\n");
					Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
					goto intcontfailure;
				}

				/*RtlCopyMemory(&DevExt->KeyboardIndicators,
					Irp->AssociatedIrp.SystemBuffer,
					sizeof(KEYBOARD_INDICATOR_PARAMETERS));*/

				//DPRINT("%x\n", DevExt->KeyboardIndicators.LedFlags);

				Irp->IoStatus.Status = STATUS_SUCCESS;
				break;
		case IOCTL_KEYBOARD_SET_TYPEMATIC:
			DPRINT("IOCTL_KEYBOARD_SET_TYPEMATIC\n");
			if (Stk->Parameters.DeviceIoControl.InputBufferLength <
				sizeof(KEYBOARD_TYPEMATIC_PARAMETERS)) {
					DPRINT("Keyboard IOCTL_KEYBOARD_SET_TYPEMATIC "
						"invalid buffer size\n");
					Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
					goto intcontfailure;
				}

				/*RtlCopyMemory(&DevExt->KeyboardTypematic,
					Irp->AssociatedIrp.SystemBuffer,
					sizeof(KEYBOARD_TYPEMATIC_PARAMETERS));*/

				Irp->IoStatus.Status = STATUS_SUCCESS;
				break;
		case IOCTL_KEYBOARD_QUERY_INDICATOR_TRANSLATION:
			/* We should check the UnitID, but it's	kind of	pointless as
			* all keyboards are supposed to have the same one
			*/
#if 0
			DPRINT("IOCTL_KEYBOARD_QUERY_INDICATOR_TRANSLATION\n");
			if (Stk->Parameters.DeviceIoControl.OutputBufferLength <
				sizeof(LOCAL_KEYBOARD_INDICATOR_TRANSLATION)) {
					DPRINT("IOCTL_KEYBOARD_QUERY_INDICATOR_TRANSLATION: "
						"invalid buffer size (expected)\n");
					/* It's to query the buffer size */
					Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
					goto intcontfailure;
				}
				Irp->IoStatus.Information =
					sizeof(LOCAL_KEYBOARD_INDICATOR_TRANSLATION);
#endif
				/*RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,
					&IndicatorTranslation,
					sizeof(LOCAL_KEYBOARD_INDICATOR_TRANSLATION));*/

				Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
				break;
		case IOCTL_INTERNAL_I8042_HOOK_KEYBOARD:
			/* Nothing to do here */
			Irp->IoStatus.Status = STATUS_SUCCESS;
			break;
		default:
			Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
			break;
		}

	intcontfailure:
			Status = Irp->IoStatus.Status;
	}
	else if (DeviceObject == MouseFdo)
	{
		// it's mouse's IOCTL
		PIO_STACK_LOCATION Stk;

		Irp->IoStatus.Information = 0;
		Stk = IoGetCurrentIrpStackLocation(Irp);

		switch (Stk->Parameters.DeviceIoControl.IoControlCode)
		{
			case IOCTL_INTERNAL_MOUSE_CONNECT:
				DPRINT("IOCTL_INTERNAL_MOUSE_CONNECT\n");
				if (Stk->Parameters.DeviceIoControl.InputBufferLength <	sizeof(CONNECT_DATA)) {
					DPRINT1("IOCTL_INTERNAL_MOUSE_CONNECT: "
						"invalid buffer size\n");
					Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
					goto intcontfailure2;
				}

				RtlCopyMemory(&MouseClassInformation,
					Stk->Parameters.DeviceIoControl.Type3InputBuffer,
					sizeof(CONNECT_DATA));

				Irp->IoStatus.Status = STATUS_SUCCESS;
				break;

		default:
			Irp->IoStatus.Status = STATUS_SUCCESS;//STATUS_INVALID_DEVICE_REQUEST;
			break;
		}
	intcontfailure2:
		Status = Irp->IoStatus.Status;
	}
	else
	{
		DPRINT("We got IOCTL for UsbCore\n");
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}


	if (Status == STATUS_INVALID_DEVICE_REQUEST) {
		DPRINT1("Invalid internal device request!\n");
	}

	if (Status != STATUS_PENDING)
		IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return Status;
}
