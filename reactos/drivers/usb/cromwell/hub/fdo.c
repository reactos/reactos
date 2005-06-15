/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         USB hub driver
 * FILE:            drivers/usb/cromwell/hub/fdo.c
 * PURPOSE:         IRP_MJ_PNP operations for FDOs
 *
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.com)
 */

#define NDEBUG
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
			DPRINT("Usbhub: IOCTL_USB_GET_NODE_INFORMATION\n");
			if (LengthOut < sizeof(USB_NODE_INFORMATION))
				Status = STATUS_BUFFER_TOO_SMALL;
			else if (BufferOut == NULL)
				Status = STATUS_INVALID_PARAMETER;
			else
			{
				NodeInformation = (PUSB_NODE_INFORMATION)BufferOut;
				dev = ((PHUB_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->dev;
				NodeInformation->NodeType = UsbHub;
				RtlCopyMemory(
					&NodeInformation->u.HubInformation.HubDescriptor,
					((struct usb_hub *)usb_get_intfdata(to_usb_interface(hubdev(dev))))->descriptor,
					sizeof(USB_HUB_DESCRIPTOR));
				NodeInformation->u.HubInformation.HubIsBusPowered = TRUE; /* FIXME */
				Information = sizeof(USB_NODE_INFORMATION);
				Status = STATUS_SUCCESS;
			}
			break;
		}
		case IOCTL_USB_GET_NODE_CONNECTION_NAME:
		{
			PUSB_NODE_CONNECTION_NAME ConnectionName;
			
			DPRINT("Usbhub: IOCTL_USB_GET_NODE_CONNECTION_NAME\n");
			if (LengthOut < sizeof(USB_NODE_CONNECTION_NAME))
				Status = STATUS_BUFFER_TOO_SMALL;
			else
			{
				ConnectionName = (PUSB_NODE_CONNECTION_NAME)BufferOut;
				DPRINT1("Usbhub: IOCTL_USB_GET_NODE_CONNECTION_NAME unimplemented\n");
				ConnectionName->ActualLength = 0;
				ConnectionName->NodeName[0] = UNICODE_NULL;
				Information = sizeof(USB_NODE_CONNECTION_NAME);
				Status = STATUS_SUCCESS;
			}
			break;
		}
		case IOCTL_USB_GET_NODE_CONNECTION_INFORMATION:
		{
			PUSB_NODE_CONNECTION_INFORMATION ConnectionInformation;
			struct usb_device* dev;
			//ULONG i;
			
			DPRINT("Usbhub: IOCTL_USB_GET_NODE_CONNECTION_INFORMATION\n");
			if (LengthOut < sizeof(USB_NODE_CONNECTION_INFORMATION))
				Status = STATUS_BUFFER_TOO_SMALL;
			else if (BufferOut == NULL)
				Status = STATUS_INVALID_PARAMETER;
			else
			{
				ConnectionInformation = (PUSB_NODE_CONNECTION_INFORMATION)BufferOut;
				DPRINT1("Usbhub: IOCTL_USB_GET_NODE_CONNECTION_INFORMATION partially implemented\n");
				dev = ((PHUB_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->dev;
				ConnectionInformation->ConnectionIndex = 0; /* FIXME */
				RtlCopyMemory(
					&ConnectionInformation->DeviceDescriptor,
					&dev->descriptor,
					sizeof(USB_DEVICE_DESCRIPTOR));
				ConnectionInformation->CurrentConfigurationValue = 0; /* FIXME */
				ConnectionInformation->LowSpeed = TRUE; /* FIXME */
				ConnectionInformation->DeviceIsHub = TRUE;
				RtlZeroMemory(&ConnectionInformation->DeviceAddress, sizeof(ConnectionInformation->DeviceAddress)); /* FIXME */
				RtlZeroMemory(&ConnectionInformation->NumberOfOpenPipes, sizeof(ConnectionInformation->NumberOfOpenPipes)); /* FIXME */
				RtlZeroMemory(&ConnectionInformation->ConnectionStatus, sizeof(ConnectionInformation->ConnectionStatus)); /* FIXME */
				RtlZeroMemory(&ConnectionInformation->PipeList, sizeof(ConnectionInformation->PipeList)); /* FIXME */
				/*for (i = 0; i < 32; i++)
				{
					RtlCopyMemory(
						&ConnectionInformation->PipeList[i].EndpointDescriptor,
						xxx, // FIXME
						sizeof(USB_ENDPOINT_DESCRIPTOR));
					ConnectionInformation->PipeList[i].ScheduleOffset = 0; // FIXME
				}*/
				Information = sizeof(USB_NODE_CONNECTION_INFORMATION);
				Status = STATUS_SUCCESS;
			}
			break;
		}
		case IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION:
		{
			//PUSB_DESCRIPTOR_REQUEST Descriptor;
			DPRINT("Usbhub: IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION\n");
			DPRINT1("Usbhub: IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION unimplemented\n");
			Information = 0;
			Status = STATUS_NOT_IMPLEMENTED;
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
