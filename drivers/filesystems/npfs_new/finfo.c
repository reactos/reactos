/*
 * PROJECT:         ReactOS Drivers
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/filesystems/npfs/finfo.c
 * PURPOSE:         Named pipe filesystem
 * PROGRAMMERS:
 */

/* INCLUDES ******************************************************************/

#include "npfs.h"

/* FUNCTIONS *****************************************************************/

NTSTATUS NTAPI
NpfsQueryInformation(IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP Irp)
{
	TRACE_(NPFS, "NpfsQueryInformation()\n");

	FsRtlEnterFileSystem();

	UNIMPLEMENTED;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	FsRtlExitFileSystem();

	return STATUS_SUCCESS;
}

NTSTATUS NTAPI
NpfsSetInformation(IN PDEVICE_OBJECT DeviceObject,
                   IN PIRP Irp)
{
	TRACE_(NPFS, "NpfsSetInformation()\n");

	FsRtlEnterFileSystem();

	UNIMPLEMENTED;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	FsRtlExitFileSystem();

	return STATUS_SUCCESS;
}

/* EOF */
