/* $Id:
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            drivers/dd/serial/power.c
 * PURPOSE:         Serial IRP_MJ_POWER operations
 * 
 * PROGRAMMERS:     Hervé Poussineau (poussine@freesurf.fr)
 */

#define NDEBUG
#include "serial.h"

NTSTATUS STDCALL
SerialPower(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	DPRINT("Serial: IRP_MJ_POWER dispatch\n");
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}
