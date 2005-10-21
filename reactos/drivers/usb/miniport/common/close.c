/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS USB miniport driver (Cromwell type)
 * FILE:            drivers/usb/miniport/common/close.c
 * PURPOSE:         IRP_MJ_CLOSE operations
 *
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.org)
 */

#define NDEBUG
#include <debug.h>

#include "usbcommon.h"

NTSTATUS STDCALL
UsbMpClose(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	PUSBMP_DEVICE_EXTENSION pDeviceExtension;

	DPRINT("USBMP: IRP_MJ_CLOSE\n");
	pDeviceExtension = (PUSBMP_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	InterlockedDecrement((PLONG)&pDeviceExtension->DeviceOpened);

	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}
