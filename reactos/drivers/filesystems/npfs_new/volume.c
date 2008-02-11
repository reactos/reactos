/*
 * PROJECT:         ReactOS Drivers
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/filesystems/npfs/volume.c
 * PURPOSE:         Named pipe filesystem
 * PROGRAMMERS:
 */

/* INCLUDES *****************************************************************/

#include "npfs.h"

/* FUNCTIONS ****************************************************************/

NTSTATUS NTAPI
NpfsQueryVolumeInformation(IN PDEVICE_OBJECT DeviceObject,
                           IN PIRP Irp)
{
	TRACE_(NPFS, "NpfsQueryVolumeInformation()\n");

	FsRtlEnterFileSystem();

	UNIMPLEMENTED;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	FsRtlExitFileSystem();

	return STATUS_SUCCESS;
}

/* EOF */
