/*
 * PROJECT:		 ReactOS Boot Loader
 * LICENSE:		 BSD - See COPYING.ARM in the top level directory
 * FILE:		 drivers/sac/driver/dispatch.c
 * PURPOSE:		 Driver for the Server Administration Console (SAC) for EMS
 * PROGRAMMERS:	 ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "sacdrv.h"

/* GLOBALS ********************************************************************/

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
DispatchDeviceControl(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
DispatchShutdownControl(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
DispatchCreate(
	IN PSAC_DEVICE_EXTENSION DeviceExtension,
	IN PIRP Irp
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
DispatchClose(
	IN PSAC_DEVICE_EXTENSION DeviceExtension,
	IN PIRP Irp
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
Dispatch(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
WorkerProcessEvents(
	IN PSAC_CHANNEL Channel
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

VOID
TimerDpcRoutine(
	IN PKDPC Dpc,
	IN PVOID DeferredContext,
	IN PVOID SystemArgument1,
	IN PVOID SystemArgument2
	)
{

}

VOID
UnloadHandler(
	IN PDRIVER_OBJECT DriverObject
	)
{

}
