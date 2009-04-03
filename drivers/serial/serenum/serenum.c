/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Serial enumerator driver
 * FILE:            drivers/bus/serenum/serenum.c
 * PURPOSE:         Serial enumerator driver entry point
 *
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.org)
 */

#define INITGUID
#include "serenum.h"

static DRIVER_DISPATCH SerenumPnp;
static DRIVER_DISPATCH IrpStub;
static DRIVER_UNLOAD DriverUnload;
DRIVER_INITIALIZE DriverEntry;

static NTSTATUS NTAPI
SerenumPnp(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	if (((PCOMMON_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->IsFDO)
		return SerenumFdoPnp(DeviceObject, Irp);
	else
		return SerenumPdoPnp(DeviceObject, Irp);
}

static VOID NTAPI
DriverUnload(IN PDRIVER_OBJECT DriverObject)
{
	// nothing to do here yet
}

static NTSTATUS NTAPI
IrpStub(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	if (((PCOMMON_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->IsFDO)
	{
		/* Forward IRPs to lower device */
		return ForwardIrpToLowerDeviceAndForget(DeviceObject, Irp);
	}
	else
	{
		/* Forward IRPs to attached FDO */
		return ForwardIrpToAttachedFdoAndForget(DeviceObject, Irp);
	}
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
	DriverObject->DriverExtension->AddDevice = SerenumAddDevice;

	for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
		DriverObject->MajorFunction[i] = IrpStub;

	//DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] = SerialQueryInformation;
	DriverObject->MajorFunction[IRP_MJ_PNP] = SerenumPnp;
	//DriverObject->MajorFunction[IRP_MJ_POWER] = SerialPower;

	return STATUS_SUCCESS;
}
