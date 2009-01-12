/*
 * PROJECT:         ReactOS FAT file system driver
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/filesystems/fastfat/flush.c
 * PURPOSE:         Flushing routines
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */


/* INCLUDES *****************************************************************/

#define NDEBUG
#include "fastfat.h"

/* FUNCTIONS ****************************************************************/

NTSTATUS
NTAPI
FatFlushBuffers(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
