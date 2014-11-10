/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Serial driver
 * FILE:            drivers/dd/serial/serial.c
 * PURPOSE:         Serial driver loading/unloading
 *
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.org)
 */

#include "serial.h"

static DRIVER_UNLOAD DriverUnload;
DRIVER_INITIALIZE DriverEntry;

static VOID NTAPI
DriverUnload(IN PDRIVER_OBJECT DriverObject)
{
	// nothing to do here yet
}

/*
 * Standard DriverEntry method.
 */
NTSTATUS NTAPI
DriverEntry(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegPath)
{
	ULONG i;

	DriverObject->DriverUnload = DriverUnload;
	DriverObject->DriverExtension->AddDevice = SerialAddDevice;

	for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
		DriverObject->MajorFunction[i] = ForwardIrpAndForget;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = SerialCreate;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = SerialClose;
	DriverObject->MajorFunction[IRP_MJ_CLEANUP] = SerialCleanup;
	DriverObject->MajorFunction[IRP_MJ_READ] = SerialRead;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = SerialWrite;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = SerialDeviceControl;
	DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] = SerialQueryInformation;
	DriverObject->MajorFunction[IRP_MJ_PNP] = SerialPnp;
	DriverObject->MajorFunction[IRP_MJ_POWER] = SerialPower;

	return STATUS_SUCCESS;
}
