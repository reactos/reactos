/*
 * PROJECT:     ReactOS VT100 emulator
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/base/green/dispatch.c
 * PURPOSE:     Dispatch routines
 * PROGRAMMERS: Copyright 2005-2006 Hervé Poussineau (hpoussin@reactos.org)
 */

#include "green.h"

#define NDEBUG
#include <debug.h>

NTSTATUS NTAPI
GreenDispatch(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	ULONG MajorFunction;
	GREEN_DEVICE_TYPE DeviceType;
	ULONG_PTR Information;
	NTSTATUS Status;

	MajorFunction = IoGetCurrentIrpStackLocation(Irp)->MajorFunction;
	DeviceType = ((PCOMMON_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->Type;

	Information = Irp->IoStatus.Information;
	Status = Irp->IoStatus.Status;

	DPRINT("Dispatching major function 0x%lx, DeviceType %u\n",
		MajorFunction, DeviceType);

	if (DeviceType == PassThroughFDO)
	{
		IoSkipCurrentIrpStackLocation(Irp);
		return IoCallDriver(((PCOMMON_FDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->LowerDevice, Irp);
	}
	else if (MajorFunction == IRP_MJ_CREATE && (DeviceType == GreenFDO || DeviceType == KeyboardPDO || DeviceType == ScreenPDO))
		return GreenCreate(DeviceObject, Irp);
	else if (MajorFunction == IRP_MJ_CLOSE && (DeviceType == GreenFDO || DeviceType == KeyboardPDO || DeviceType == ScreenPDO))
		return GreenClose(DeviceObject, Irp);
	else if ((MajorFunction == IRP_MJ_CREATE || MajorFunction == IRP_MJ_CLOSE || MajorFunction == IRP_MJ_CLEANUP)
		&& (DeviceType == KeyboardFDO || DeviceType == ScreenFDO))
	{
		IoSkipCurrentIrpStackLocation(Irp);
		return IoCallDriver(((PCOMMON_FDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->LowerDevice, Irp);
	}
	else if (MajorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL && DeviceType == GreenFDO)
	{
		return KeyboardInternalDeviceControl(
			((PGREEN_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->KeyboardFdo,
			Irp);
	}
	else if (MajorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL && DeviceType == KeyboardFDO)
		return KeyboardInternalDeviceControl(DeviceObject, Irp);
	else if (MajorFunction == IRP_MJ_DEVICE_CONTROL && DeviceType == GreenFDO)
	{
		return ScreenDeviceControl(
			((PGREEN_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->ScreenFdo,
			Irp);
	}
	else if (MajorFunction == IRP_MJ_DEVICE_CONTROL && DeviceType == ScreenFDO)
		return ScreenDeviceControl(DeviceObject, Irp);
	else if (MajorFunction == IRP_MJ_WRITE && DeviceType == ScreenFDO)
		return ScreenWrite(DeviceObject, Irp);
	else if (MajorFunction == IRP_MJ_PNP && (DeviceType == KeyboardFDO || DeviceType == ScreenFDO))
	{
		IoSkipCurrentIrpStackLocation(Irp);
		return IoCallDriver(((PCOMMON_FDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->LowerDevice, Irp);
	}
	else if (MajorFunction == IRP_MJ_PNP && (DeviceType == GreenFDO || DeviceType == KeyboardPDO || DeviceType == ScreenPDO))
		return GreenPnp(DeviceObject, Irp);
	else if (MajorFunction == IRP_MJ_POWER && DeviceType == GreenFDO)
		return GreenPower(DeviceObject, Irp);
	else
	{
		DPRINT1("Unknown combination: MajorFunction 0x%lx, DeviceType %d\n",
			MajorFunction, DeviceType);
		switch (DeviceType)
		{
			case KeyboardFDO:
			case ScreenFDO:
			{
				IoSkipCurrentIrpStackLocation(Irp);
				return IoCallDriver(((PCOMMON_FDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->LowerDevice, Irp);
			}
			case GreenFDO:
			{
				PDRIVER_OBJECT DriverObject;
				PGREEN_DRIVER_EXTENSION DriverExtension;
				DriverObject = DeviceObject->DriverObject;
				DriverExtension = IoGetDriverObjectExtension(DriverObject, DriverObject);
				IoSkipCurrentIrpStackLocation(Irp);
				return IoCallDriver(DriverExtension->LowerDevice, Irp);
			}
			default:
				ASSERT(FALSE);
		}
	}

	Irp->IoStatus.Information = Information;
	Irp->IoStatus.Status = Status;
	IoCompleteRequest (Irp, IO_NO_INCREMENT);
	return Status;
}
