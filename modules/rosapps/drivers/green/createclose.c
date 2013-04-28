/*
 * PROJECT:     ReactOS VT100 emulator
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/base/green/createclose.c
 * PURPOSE:     IRP_MJ_CREATE, IRP_MJ_CLOSE and IRP_MJ_CLEANUP operations
 * PROGRAMMERS: Copyright 2005-2006 Hervé Poussineau (hpoussin@reactos.org)
 */

#include "green.h"

#define NDEBUG
#include <debug.h>

NTSTATUS
GreenCreate(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	DPRINT("IRP_MJ_CREATE\n");

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS
GreenClose(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	DPRINT("IRP_MJ_CLOSE\n");

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}
