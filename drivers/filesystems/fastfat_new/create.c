/*
 * PROJECT:         ReactOS FAT file system driver
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/filesystems/fastfat/create.c
 * PURPOSE:         Create routines
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include "fastfat.h"

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
FatCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    DPRINT1("FatCreate()\n");
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
