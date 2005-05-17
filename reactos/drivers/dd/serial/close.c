/* $Id:
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            drivers/dd/serial/close.c
 * PURPOSE:         Serial IRP_MJ_CLOSE operations
 *
 * PROGRAMMERS:     Hervé Poussineau (poussine@freesurf.fr)
 */

#define NDEBUG
#include "serial.h"

NTSTATUS STDCALL
SerialClose(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	PSERIAL_DEVICE_EXTENSION pDeviceExtension;

	DPRINT("Serial: IRP_MJ_CLOSE\n");
	pDeviceExtension = (PSERIAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	pDeviceExtension->IsOpened = FALSE;

	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = STATUS_SUCCESS;
 	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}
