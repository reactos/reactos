/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS UHCI controller driver (Cromwell type)
 * FILE:            drivers/usb/cromwell/uhci/cleanup.c
 * PURPOSE:         IRP_MJ_CLEANUP operations
 *
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.com)
 */

#define NDEBUG
#include "uhci.h"

NTSTATUS STDCALL
UhciCleanup(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	DPRINT("UHCI: IRP_MJ_CLEANUP\n");

	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}
