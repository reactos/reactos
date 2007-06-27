/*
 * PROJECT:         ReactOS Drivers
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/filesystems/npfs/create.c
 * PURPOSE:         Named pipe filesystem
 * PROGRAMMERS:     
 */

/* INCLUDES ******************************************************************/

#include "npfs.h"

//#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS NTAPI
NpfsCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	NpfsDbgPrint(NPFS_DL_API_TRACE, "IRP_MJ_CREATE\n");

	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS NTAPI
NpfsCreateNamedPipe(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	NpfsDbgPrint(NPFS_DL_API_TRACE, "IRP_MJ_CREATE Named Pipe\n");

	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS NTAPI
NpfsCleanup(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	NpfsDbgPrint(NPFS_DL_API_TRACE, "Cleanup\n");

	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS NTAPI
NpfsClose(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	NpfsDbgPrint(NPFS_DL_API_TRACE, "IRP_MJ_CLOSE\n");

	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

/* EOF */
