/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         USB hub driver
 * FILE:            drivers/usb/cromwell/usbhub/createclose.c
 * PURPOSE:         IRP_MJ_CREATE and IRP_MJ_CLOSE operations
 *
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.com)
 */

#define NDEBUG
#include "usbhub.h"

NTSTATUS STDCALL
UsbhubCreate(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	DPRINT("Usbhub: IRP_MJ_CREATE\n");
	
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS STDCALL
UsbhubClose(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	DPRINT("Usbhub: IRP_MJ_CLOSE\n");
	
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS STDCALL
UsbhubCleanup(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	DPRINT("Usbhub: IRP_MJ_CLEANUP\n");
	
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}
