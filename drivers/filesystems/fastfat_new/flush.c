/*
 * PROJECT:         ReactOS FAT file system driver
 * LICENSE:         GNU GPLv3 as published by the Free Software Foundation
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
    DPRINT1("FatFlushBuffers()\n");
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
