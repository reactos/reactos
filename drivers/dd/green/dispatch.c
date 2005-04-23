/* $Id:
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS VT100 emulator
 * FILE:            drivers/dd/green/dispatch.c
 * PURPOSE:         Dispatch routines
 * 
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.com)
 */

#define NDEBUG
#include "green.h"

NTSTATUS STDCALL
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
	
	DPRINT("Green: Dispatching major function 0x%lx, DeviceType %d\n",
		MajorFunction, DeviceType);
	
	if (MajorFunction == IRP_MJ_CREATE && DeviceType == Green)
		return GreenCreate(DeviceObject, Irp);
	else if (MajorFunction == IRP_MJ_CLOSE && DeviceType == Green)
		return GreenClose(DeviceObject, Irp);
	else if (MajorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL && DeviceType == Green)
	{
		return KeyboardInternalDeviceControl(
			((PGREEN_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->Keyboard,
			Irp);
	}
	else if (MajorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL && DeviceType == Keyboard)
		return KeyboardInternalDeviceControl(DeviceObject, Irp);
	else if (MajorFunction == IRP_MJ_DEVICE_CONTROL && DeviceType == Green)
	{
		return ScreenDeviceControl(
			((PGREEN_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->Screen,
			Irp);
	}
	else if (MajorFunction == IRP_MJ_DEVICE_CONTROL && DeviceType == Screen)
		return ScreenDeviceControl(DeviceObject, Irp);
	else if (MajorFunction == IRP_MJ_WRITE && DeviceType == Screen)
		return ScreenWrite(DeviceObject, Irp);
	else
	{
		DPRINT1("Green: unknown combination: MajorFunction 0x%lx, DeviceType %d\n",
			MajorFunction, DeviceType);
	}
	
	Irp->IoStatus.Information = Information;
	Irp->IoStatus.Status = Status;
	IoCompleteRequest (Irp, IO_NO_INCREMENT);
	return Status;
}
