/*
 * PROJECT:         ReactOS FAT file system driver
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/filesystems/fastfat/blockdev.c
 * PURPOSE:         Temporary sector reading support
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include "fastfat.h"

/* FUNCTIONS ***************************************************************/

NTSTATUS
VfatReadDisk (IN PDEVICE_OBJECT pDeviceObject,
	      IN PLARGE_INTEGER ReadOffset,
	      IN ULONG ReadLength,
	      IN OUT PUCHAR Buffer,
	      IN BOOLEAN Override)
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
VfatReadDiskPartial (IN PFAT_IRP_CONTEXT IrpContext,
		     IN PLARGE_INTEGER ReadOffset,
		     IN ULONG ReadLength,
		     ULONG BufferOffset,
		     IN BOOLEAN Wait)
{
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
VfatWriteDiskPartial (IN PFAT_IRP_CONTEXT IrpContext,
		      IN PLARGE_INTEGER WriteOffset,
		      IN ULONG WriteLength,
		      IN ULONG BufferOffset,
		      IN BOOLEAN Wait)
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
VfatBlockDeviceIoControl (IN PDEVICE_OBJECT DeviceObject,
			  IN ULONG CtlCode,
			  IN PVOID InputBuffer OPTIONAL,
			  IN ULONG InputBufferSize,
			  IN OUT PVOID OutputBuffer OPTIONAL,
			  IN OUT PULONG OutputBufferSize,
			  IN BOOLEAN Override)
{
    return STATUS_NOT_IMPLEMENTED;
}
