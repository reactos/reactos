/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Serial port driver
 * FILE:            drivers/dd/serial/create.c
 * PURPOSE:         Serial IRP_MJ_CREATE operations
 *
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.org)
 */

#include "serial.h"

NTSTATUS NTAPI
SerialCreate(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	PIO_STACK_LOCATION Stack;
	PSERIAL_DEVICE_EXTENSION DeviceExtension;
	NTSTATUS Status;

	DPRINT("IRP_MJ_CREATE\n");
	Stack = IoGetCurrentIrpStackLocation(Irp);
	DeviceExtension = (PSERIAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	if (Stack->Parameters.Create.Options & FILE_DIRECTORY_FILE)
	{
		CHECKPOINT;
		Status = STATUS_NOT_A_DIRECTORY;
		goto ByeBye;
	}

	if(DeviceExtension->IsOpened)
	{
		DPRINT("COM%lu is already opened\n", DeviceExtension->ComPort);
		Status = STATUS_ACCESS_DENIED;
		goto ByeBye;
	}

	DPRINT("Open COM%lu: successfull\n", DeviceExtension->ComPort);
	DeviceExtension->IsOpened = TRUE;
	Status = STATUS_SUCCESS;

ByeBye:
	Irp->IoStatus.Status = Status;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}
