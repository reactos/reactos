/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS USB miniport driver (Cromwell type)
 * FILE:            drivers/usb/miniport/common/create.c
 * PURPOSE:         IRP_MJ_CREATE operations
 *
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.org)
 */

#define NDEBUG
#include <debug.h>

#include "usbcommon.h"

NTSTATUS STDCALL
UsbMpCreate(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	PIO_STACK_LOCATION Stack;
	PUSBMP_DEVICE_EXTENSION DeviceExtension;
	NTSTATUS Status;

	DPRINT("USBMP: IRP_MJ_CREATE\n");
	Stack = IoGetCurrentIrpStackLocation(Irp);
	DeviceExtension = (PUSBMP_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

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
