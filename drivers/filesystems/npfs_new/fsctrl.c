/*
 * PROJECT:         ReactOS Drivers
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/filesystems/npfs/fsctrl.c
 * PURPOSE:         Named pipe filesystem
 * PROGRAMMERS:
 */

/* INCLUDES ******************************************************************/

#include "npfs.h"

/* FUNCTIONS *****************************************************************/

NTSTATUS NTAPI
NpfsDirectoryControl(IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP Irp)
{
	TRACE_(NPFS, "NpfsDirectoryControl()\n");

	FsRtlEnterFileSystem();

	UNIMPLEMENTED;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	FsRtlExitFileSystem();

	return STATUS_SUCCESS;
}

NTSTATUS NTAPI
NpfsFileSystemControl(IN PDEVICE_OBJECT DeviceObject,
                      IN PIRP Irp)
{
	TRACE_(NPFS, "NpfsFileSystemControl()\n");

	FsRtlEnterFileSystem();

	UNIMPLEMENTED;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	FsRtlExitFileSystem();

	return STATUS_SUCCESS;
}

/* EOF */
