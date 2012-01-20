/*
 * PROJECT:     ReactOS Serial mouse driver
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/input/sermouse/fdo.c
 * PURPOSE:     IRP_MJ_INTERNAL_DEVICE_CONTROL operations
 * PROGRAMMERS: Copyright 2005-2006 Hervé Poussineau (hpoussin@reactos.org)
 */

#include "sermouse.h"

NTSTATUS NTAPI
SermouseInternalDeviceControl(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	PSERMOUSE_DEVICE_EXTENSION DeviceExtension;
	PIO_STACK_LOCATION Stack;
	NTSTATUS Status;

	DeviceExtension = (PSERMOUSE_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	Stack = IoGetCurrentIrpStackLocation(Irp);

	switch (Stack->Parameters.DeviceIoControl.IoControlCode)
	{
		case IOCTL_INTERNAL_MOUSE_CONNECT:
		{
			TRACE_(SERMOUSE, "IRP_MJ_INTERNAL_DEVICE_CONTROL / IOCTL_INTERNAL_MOUSE_CONNECT\n");
			DeviceExtension->ConnectData =
				*((PCONNECT_DATA)Stack->Parameters.DeviceIoControl.Type3InputBuffer);
			Status = STATUS_SUCCESS;
			break;
		}
		case IOCTL_INTERNAL_MOUSE_DISCONNECT:
		{
			TRACE_(SERMOUSE, "IRP_MJ_INTERNAL_DEVICE_CONTROL / IOCTL_INTERNAL_MOUSE_DISCONNECT\n");

			/* Ask read loop to end */
			KeSetEvent(&DeviceExtension->StopWorkerThreadEvent, (KPRIORITY)0, FALSE);
			Status = STATUS_SUCCESS;
			break;
		}
		case IOCTL_MOUSE_QUERY_ATTRIBUTES:
		{
			TRACE_(SERMOUSE, "IRP_MJ_INTERNAL_DEVICE_CONTROL / IOCTL_MOUSE_QUERY_ATTRIBUTES\n");
			if (Stack->Parameters.DeviceIoControl.OutputBufferLength >= sizeof(MOUSE_ATTRIBUTES))
			{
				*(PMOUSE_ATTRIBUTES)Irp->AssociatedIrp.SystemBuffer =
					DeviceExtension->AttributesInformation;
				Irp->IoStatus.Information = sizeof(MOUSE_ATTRIBUTES);
				Status = STATUS_SUCCESS;
			}
			else
			{
				Status = STATUS_BUFFER_TOO_SMALL;
			}
			break;
		}
		default:
		{
			WARN_(SERMOUSE, "IRP_MJ_INTERNAL_DEVICE_CONTROL / unknown ioctl code 0x%lx\n",
				Stack->Parameters.DeviceIoControl.IoControlCode);
			ASSERT(FALSE);
			Status = STATUS_INVALID_DEVICE_REQUEST;
			break;
		}
	}

	Irp->IoStatus.Status = Status;
	if (Status == STATUS_PENDING)
	{
		IoMarkIrpPending(Irp);
		IoStartPacket(DeviceObject, Irp, NULL, NULL);
	}
	else
	{
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
	}

	return Status;
}
