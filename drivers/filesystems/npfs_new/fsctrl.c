/*
 * PROJECT:         ReactOS Drivers
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/filesystems/npfs/fsctrl.c
 * PURPOSE:         Named pipe filesystem
 * PROGRAMMERS:     
 */

/* INCLUDES ******************************************************************/

#include "npfs.h"

//#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS NTAPI
NpfsDirectoryControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	FsRtlEnterFileSystem();

	NpfsDbgPrint(NPFS_DL_API_TRACE, "NpfsDirectoryControl()\n");

	/* TODO: Implement */
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	FsRtlExitFileSystem();

	return STATUS_SUCCESS;
}

NTSTATUS NTAPI
NpfsFileSystemControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	FsRtlEnterFileSystem();

	NpfsDbgPrint(NPFS_DL_API_TRACE, "NpfsFileSystemControl()\n");

	/* TODO: Implement */
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	FsRtlExitFileSystem();

	return STATUS_SUCCESS;
}

/* EOF */
