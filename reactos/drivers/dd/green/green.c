/* $Id:
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS VT100 emulator
 * FILE:            drivers/dd/green/green.c
 * PURPOSE:         Driver entry point
 *
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.com)
 */

//#define NDEBUG
#include "green.h"

VOID STDCALL
DriverUnload(IN PDRIVER_OBJECT DriverObject)
{
	// nothing to do here yet
}

/*
 * Standard DriverEntry method.
 */
NTSTATUS STDCALL
DriverEntry(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegPath)
{
	ULONG i;

	DriverObject->DriverUnload = DriverUnload;
	DriverObject->DriverExtension->AddDevice = GreenAddDevice;

	for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
		DriverObject->MajorFunction[i] = GreenDispatch;

	/* keyboard only */
	//DriverObject->DriverStartIo = GreenStartIo;

	/* keyboard and screen */
	DriverObject->MajorFunction[IRP_MJ_CREATE] = GreenCreate;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = GreenClose;

	return STATUS_SUCCESS;
}
