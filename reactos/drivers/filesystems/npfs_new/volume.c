/*
 * PROJECT:         ReactOS Drivers
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/filesystems/npfs/volume.c
 * PURPOSE:         Named pipe filesystem
 * PROGRAMMERS:     
 */

/* INCLUDES *****************************************************************/

#include "npfs.h"

//#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

NTSTATUS NTAPI
NpfsQueryVolumeInformation(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	FsRtlEnterFileSystem();

	NpfsDbgPrint(NPFS_DEBUG_HIGHEST, "NpfsQueryVolumeInformation()\n");

	/* TODO: Implement */
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	FsRtlExitFileSystem();

	return STATUS_SUCCESS;
}

/* EOF */
