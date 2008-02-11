/*
 * PROJECT:         ReactOS Drivers
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/filesystems/npfs/create.c
 * PURPOSE:         Named pipe filesystem
 * PROGRAMMERS:
 */

/* INCLUDES ******************************************************************/

#include "npfs.h"

/* FUNCTIONS *****************************************************************/

NTSTATUS NTAPI
NpfsCreate(IN PDEVICE_OBJECT DeviceObject,
           IN PIRP Irp)
{
	TRACE_(NPFS, "IRP_MJ_CREATE\n");

	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS NTAPI
NpfsCreateNamedPipe(IN PDEVICE_OBJECT DeviceObject,
                    IN PIRP Irp)
{
	TRACE_(NPFS, "IRP_MJ_CREATE Named Pipe\n");

	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS NTAPI
NpfsCleanup(IN PDEVICE_OBJECT DeviceObject,
            IN PIRP Irp)
{
	TRACE_(NPFS, "Cleanup\n");

	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS NTAPI
NpfsClose(IN PDEVICE_OBJECT DeviceObject,
          IN PIRP Irp)
{
	TRACE_(NPFS, "IRP_MJ_CLOSE\n");

	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

/* EOF */
