/*
 * PROJECT:         ReactOS FAT file system driver
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/filesystems/fastfat/rw.c
 * PURPOSE:         Read/write support
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include "fastfat.h"

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
FatRead(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    DPRINT1("FatRead()\n");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
FatWrite(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    DPRINT1("FatWrite()\n");
    return STATUS_NOT_IMPLEMENTED;
}


