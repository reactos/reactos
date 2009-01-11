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
VfatRead(PVFAT_IRP_CONTEXT IrpContext)
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
VfatWrite(PVFAT_IRP_CONTEXT IrpContext)
{
    return STATUS_NOT_IMPLEMENTED;
}


