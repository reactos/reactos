/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         USB hub driver
 * FILE:            drivers/usb/cromwell/hub/pdo.c
 * PURPOSE:         IRP_MJ_PNP operations for PDOs
 *
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.com)
 */

#define NDEBUG
#include <stdio.h>
#include "usbhub.h"

#define IO_METHOD_FROM_CTL_CODE(ctlCode) (ctlCode&0x00000003)

NTSTATUS
UsbhubDeviceControlPdo(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	PIO_STACK_LOCATION Stack;
	ULONG_PTR Information = 0;
	NTSTATUS Status;
	
	DPRINT("Usbhub: UsbhubDeviceControlPdo() called\n");
	
	Stack = IoGetCurrentIrpStackLocation(Irp);
	Status = Irp->IoStatus.Status;
	
	switch (Stack->Parameters.DeviceIoControl.IoControlCode)
	{
		default:
		{
			DPRINT1("Usbhub: Unknown IOCTL code 0x%lx\n", Stack->Parameters.DeviceIoControl.IoControlCode);
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
UsbhubPdoStartDevice(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	PHUB_DEVICE_EXTENSION DeviceExtension;
	NTSTATUS Status;
	
	DeviceExtension = (PHUB_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	DbgBreakPoint();
	
	/* Register and activate device interface */
	Status = IoRegisterDeviceInterface(
		DeviceObject,
		DeviceExtension->dev->descriptor.bDeviceClass == USB_CLASS_HUB ?
			&GUID_DEVINTERFACE_USB_HUB :
			&GUID_DEVINTERFACE_USB_DEVICE,
		NULL, /* Reference string */
		&DeviceExtension->SymbolicLinkName);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("Usbhub: IoRegisterDeviceInterface() failed with status 0x%08lx\n", Status);
		return Status;
	}
	
	Status = IoSetDeviceInterfaceState(&DeviceExtension->SymbolicLinkName, TRUE);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("Usbhub: IoSetDeviceInterfaceState() failed with status 0x%08lx\n", Status);
		return Status;
	}
	
	return STATUS_SUCCESS;
}

static NTSTATUS
UsbhubPdoQueryId(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp,
	OUT ULONG_PTR* Information)
{
	PHUB_DEVICE_EXTENSION DeviceExtension;
	ULONG IdType;
	PUNICODE_STRING SourceString;
	UNICODE_STRING String;
	NTSTATUS Status;
	
	IdType = IoGetCurrentIrpStackLocation(Irp)->Parameters.QueryId.IdType;
	DeviceExtension = (PHUB_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	RtlInitUnicodeString(&String, NULL);
	
	switch (IdType)
	{
		case BusQueryDeviceID:
		{
			DPRINT("Usbhub: IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryDeviceID\n");
			SourceString = &DeviceExtension->DeviceId;
			break;
		}
		case BusQueryHardwareIDs:
		{
			DPRINT("Usbhub: IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryHardwareIDs\n");
			SourceString = &DeviceExtension->HardwareIds;
			break;
		}
		case BusQueryCompatibleIDs:
		{
			DPRINT("Usbhub: IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryCompatibleIDs\n");
			SourceString = &DeviceExtension->CompatibleIds;
			break;
		}
		case BusQueryInstanceID:
		{
			DPRINT("Usbhub: IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryInstanceID\n");
			SourceString = &DeviceExtension->InstanceId;
			break;
		}
		default:
			DPRINT1("Usbhub: IRP_MJ_PNP / IRP_MN_QUERY_ID / unknown query id type 0x%lx\n", IdType);
			return STATUS_NOT_SUPPORTED;
	}
	
	Status = UsbhubDuplicateUnicodeString(
		&String,
		SourceString,
		PagedPool);
	*Information = (ULONG_PTR)String.Buffer;
	return Status;
}

NTSTATUS STDCALL
UsbhubPnpPdo(
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
		case IRP_MN_START_DEVICE: /* 0x0 */
		{
			DPRINT("Usbhub: IRP_MJ_PNP / IRP_MN_START_DEVICE\n");
			Status = UsbhubPdoStartDevice(DeviceObject, Irp);
			break;
		}
		case IRP_MN_QUERY_ID: /* 0x13 */
		{
			Status = UsbhubPdoQueryId(DeviceObject, Irp, &Information);
			break;
		}
		default:
		{
			/* We can't forward request to the lower driver, because
			 * we are a Pdo, so we don't have lower driver...
			 */
			DPRINT1("Usbhub: IRP_MJ_PNP / unknown minor function 0x%lx\n", MinorFunction);
			Information = Irp->IoStatus.Information;
			Status = Irp->IoStatus.Status;
		}
	}

	Irp->IoStatus.Information = Information;
	Irp->IoStatus.Status = Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}

