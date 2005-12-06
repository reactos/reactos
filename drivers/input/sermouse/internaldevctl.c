/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Serial mouse driver
 * FILE:            drivers/input/sermouse/internaldevctl.c
 * PURPOSE:         IRP_MJ_INTERNAL_DEVICE_CONTROL operations
 *
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.org)
 */

#define NDEBUG
#include <debug.h>

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
			DPRINT("IRP_MJ_INTERNAL_DEVICE_CONTROL / IOCTL_INTERNAL_MOUSE_CONNECT\n");
			DeviceExtension->ConnectData =
				*((PCONNECT_DATA)Stack->Parameters.DeviceIoControl.Type3InputBuffer);

			/* Start read loop */
			Status = PsCreateSystemThread(
				&DeviceExtension->WorkerThreadHandle,
				(ACCESS_MASK)0L,
				NULL,
				NULL,
				NULL,
				SermouseDeviceWorker,
				DeviceObject);
			break;
		}
		case IOCTL_INTERNAL_MOUSE_DISCONNECT:
		{
			DPRINT("IRP_MJ_INTERNAL_DEVICE_CONTROL / IOCTL_INTERNAL_MOUSE_DISCONNECT\n");

			/* Ask read loop to end */
			KeSetEvent(&DeviceExtension->StopWorkerThreadEvent, (KPRIORITY)0, FALSE);
			Status = STATUS_SUCCESS;
			break;
		}
		case IOCTL_MOUSE_QUERY_ATTRIBUTES:
		{
			DPRINT("IRP_MJ_INTERNAL_DEVICE_CONTROL / IOCTL_MOUSE_QUERY_ATTRIBUTES\n");
			if (Stack->Parameters.DeviceIoControl.OutputBufferLength >= sizeof(MOUSE_ATTRIBUTES))
			{
				*(PMOUSE_ATTRIBUTES)Irp->AssociatedIrp.SystemBuffer =
					DeviceExtension->AttributesInformation;
				Irp->IoStatus.Information = sizeof(MOUSE_ATTRIBUTES);
				Status = STATUS_SUCCESS;
			} else {
				Status = STATUS_BUFFER_TOO_SMALL;
			}
			break;
		}
		default:
		{
			DPRINT1("IRP_MJ_INTERNAL_DEVICE_CONTROL / unknown ioctl code 0x%lx\n",
				Stack->Parameters.DeviceIoControl.IoControlCode);
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
