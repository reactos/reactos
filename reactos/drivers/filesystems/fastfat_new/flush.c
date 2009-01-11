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

NTSTATUS VfatFlushVolume(PDEVICE_EXTENSION DeviceExt, PVFATFCB VolumeFcb)
{
    DPRINT("VfatFlushVolume(DeviceExt %p, FatFcb %p)\n", DeviceExt, VolumeFcb);

    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS VfatFlush(PFAT_IRP_CONTEXT IrpContext)
{
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
