/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS UHCI controller driver (Cromwell type)
 * FILE:            drivers/usb/cromwell/uhci/create.c
 * PURPOSE:         IRP_MJ_CREATE operations
 *
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.com)
 */

#define NDEBUG
#include "uhci.h"

NTSTATUS STDCALL
UhciCreate(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	PIO_STACK_LOCATION Stack;
	POHCI_DEVICE_EXTENSION DeviceExtension;
	NTSTATUS Status;

	DPRINT("UHCI: IRP_MJ_CREATE\n");
	Stack = IoGetCurrentIrpStackLocation(Irp);
	DeviceExtension = (POHCI_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	if (Stack->Parameters.Create.Options & FILE_DIRECTORY_FILE)
	{
		CHECKPOINT;
		Status = STATUS_NOT_A_DIRECTORY;
		goto ByeBye;
	}

	InterlockedIncrement((PLONG)&DeviceExtension->DeviceOpened);
	Status = STATUS_SUCCESS;

ByeBye:
	Irp->IoStatus.Status = Status;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}
