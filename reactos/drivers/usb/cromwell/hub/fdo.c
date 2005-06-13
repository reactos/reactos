/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         USB hub driver
 * FILE:            drivers/usb/cromwell/hub/fdo.c
 * PURPOSE:         IRP_MJ_PNP operations for FDOs
 *
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.com)
 */

//#define NDEBUG
#include "usbhub.h"

extern struct usb_driver hub_driver;

#define IO_METHOD_FROM_CTL_CODE(ctlCode) (ctlCode&0x00000003)

static VOID
UsbhubGetUserBuffers(
	IN PIRP Irp,
	IN ULONG IoControlCode,
	OUT PVOID* BufferIn,
	OUT PVOID* BufferOut)
{
	ASSERT(Irp);
	ASSERT(BufferIn);
	ASSERT(BufferOut);

	switch (IO_METHOD_FROM_CTL_CODE(IoControlCode))
	{
		case METHOD_BUFFERED:
			*BufferIn = *BufferOut = Irp->AssociatedIrp.SystemBuffer;
			break;
		case METHOD_IN_DIRECT:
		case METHOD_OUT_DIRECT:
			*BufferIn = Irp->AssociatedIrp.SystemBuffer;
			*BufferOut = MmGetSystemAddressForMdl(Irp->MdlAddress);
			break;
		case METHOD_NEITHER:
			*BufferIn = IoGetCurrentIrpStackLocation(Irp)->Parameters.DeviceIoControl.Type3InputBuffer;
			*BufferOut = Irp->UserBuffer;
			break;
		default:
			/* Should never happen */
			*BufferIn = NULL;
			*BufferOut = NULL;
			break;
	}
}

NTSTATUS STDCALL
UsbhubPnpFdo(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	PIO_STACK_LOCATION IrpSp;
	NTSTATUS Status;
	ULONG MinorFunction;
	ULONG_PTR Information = 0;
	
	IrpSp = IoGetCurrentIrpStackLocation(Irp);
	MinorFunction = IrpSp->MinorFunction;

	switch (MinorFunction)
	{
		case IRP_MN_START_DEVICE:
		{
			DPRINT("Usbhub: IRP_MJ_PNP / IRP_MN_START_DEVICE\n");
			Status = ForwardIrpAndWait(DeviceObject, Irp);
			//if (NT_SUCCESS(Status) && NT_SUCCESS(Irp->IoStatus.Status))
			//	Status = OHCD_PnPStartDevice(DeviceObject, Irp);
			break;
		}

		case IRP_MN_REMOVE_DEVICE:
		//case IRP_MN_QUERY_REMOVE_DEVICE:
		//case IRP_MN_CANCEL_REMOVE_DEVICE:
		case IRP_MN_SURPRISE_REMOVAL:

		case IRP_MN_STOP_DEVICE:
		{
			DPRINT("Usbhub: IRP_MJ_PNP / IRP_MN_STOP_DEVICE\n");
			Status = ForwardIrpAndWait(DeviceObject, Irp);
			if (NT_SUCCESS(Status) && NT_SUCCESS(Irp->IoStatus.Status))
				Status = STATUS_SUCCESS;
			IoDeleteDevice(DeviceObject); // just delete device for now
			break;
		}
		case IRP_MN_QUERY_STOP_DEVICE:
		case IRP_MN_CANCEL_STOP_DEVICE:
		{
			Status = STATUS_SUCCESS;
			break;
		}

		default:
		{
			DPRINT1("Usbhub: IRP_MJ_PNP / unknown minor function 0x%lx\n", MinorFunction);
			return ForwardIrpAndForget(DeviceObject, Irp);
		}
	}
	Irp->IoStatus.Information = Information;
	Irp->IoStatus.Status = Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}

static inline struct device *hubdev (struct usb_device *dev)
{
	return &dev->actconfig->interface [0].dev;
}

NTSTATUS
UsbhubDeviceControlFdo(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	PIO_STACK_LOCATION Stack;
	ULONG IoControlCode;
	PHUB_DEVICE_EXTENSION DeviceExtension;
	ULONG LengthIn, LengthOut;
	ULONG_PTR Information = 0;
	PVOID BufferIn, BufferOut;
	NTSTATUS Status;

	DPRINT("Usbhub: UsbhubDeviceControlFdo() called\n");

	Stack = IoGetCurrentIrpStackLocation(Irp);
	LengthIn = Stack->Parameters.DeviceIoControl.InputBufferLength;
	LengthOut = Stack->Parameters.DeviceIoControl.OutputBufferLength;
	DeviceExtension = (PHUB_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	IoControlCode = Stack->Parameters.DeviceIoControl.IoControlCode;
	UsbhubGetUserBuffers(Irp, IoControlCode, &BufferIn, &BufferOut);

	switch (IoControlCode)
	{
		case IOCTL_USB_GET_NODE_INFORMATION:
		{
			PUSB_NODE_INFORMATION NodeInformation;
			struct usb_device* dev;
			struct device* device;
			struct usb_interface * intf;
			struct usb_hub *hub;
			struct usb_hub_descriptor *descriptor;
			DPRINT("Usbhub: IOCTL_USB_GET_NODE_INFORMATION\n");
			if (LengthOut < sizeof(USB_NODE_INFORMATION))
				Status = STATUS_BUFFER_TOO_SMALL;
			else if (BufferOut == NULL)
				Status = STATUS_INVALID_PARAMETER;
			else
			{
				NodeInformation = (PUSB_NODE_INFORMATION)BufferOut;
				dev = ((PHUB_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->dev;
				device = hubdev(dev);
				intf = to_usb_interface(device);
				hub = usb_get_intfdata(intf);
				descriptor = hub->descriptor;
				NodeInformation->NodeType = UsbHub;
				RtlCopyMemory(
					&NodeInformation->u.HubInformation.HubDescriptor,
					descriptor,
					sizeof(USB_HUB_DESCRIPTOR));
				NodeInformation->u.HubInformation.HubIsBusPowered = TRUE; /* FIXME */
				Information = sizeof(USB_NODE_INFORMATION);
				Status = STATUS_SUCCESS;
			}
			break;
		}
		default:
		{
			/* Pass Irp to lower driver */
			DPRINT1("Usbhub: Unknown IOCTL code 0x%lx\n", Stack->Parameters.DeviceIoControl.IoControlCode);
			return ForwardIrpAndForget(DeviceObject, Irp);
		}
	}

	Irp->IoStatus.Information = Information;
	Irp->IoStatus.Status = Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}
