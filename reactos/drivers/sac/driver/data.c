/*
 * PROJECT:		 ReactOS Boot Loader
 * LICENSE:		 BSD - See COPYING.ARM in the top level directory
 * FILE:		 drivers/sac/driver/data.c
 * PURPOSE:		 Driver for the Server Administration Console (SAC) for EMS
 * PROGRAMMERS:	 ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "sacdrv.h"

/* GLOBALS ********************************************************************/

ULONG SACDebug;

/* FUNCTIONS ******************************************************************/

VOID
FreeGlobalData(
	VOID
	)
{

}

VOID
FreeDeviceData(
	IN PDEVICE_OBJECT DeviceObject
	)
{

}

NTSTATUS
WorkerThreadStartUp(
	IN PVOID Context
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

BOOLEAN
InitializeDeviceData(
	IN PDEVICE_OBJECT DeviceObject
	)
{
	return FALSE;
}

BOOLEAN
InitializeGlobalData(
	IN PUNICODE_STRING RegistryPath,
	IN PDRIVER_OBJECT DriverObject
	)
{
	return FALSE;
}
