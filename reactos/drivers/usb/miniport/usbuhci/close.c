/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS UHCI controller driver (Cromwell type)
 * FILE:            drivers/usb/cromwell/uhci/close.c
 * PURPOSE:         IRP_MJ_CLOSE operations
 *
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.com)
 */

#define NDEBUG
#include "uhci.h"

NTSTATUS STDCALL
UhciClose(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	POHCI_DEVICE_EXTENSION pDeviceExtension;

	DPRINT("UHCI: IRP_MJ_CLOSE\n");
	pDeviceExtension = (POHCI_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	InterlockedDecrement((PLONG)&pDeviceExtension->DeviceOpened);

	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}
