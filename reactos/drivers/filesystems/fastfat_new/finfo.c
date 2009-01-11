/*
 * PROJECT:         ReactOS FAT file system driver
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/filesystems/fastfat/finfo.c
 * PURPOSE:         File Information support routines
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include "fastfat.h"

/* FUNCTIONS ****************************************************************/

NTSTATUS VfatQueryInformation(PVFAT_IRP_CONTEXT IrpContext)
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS VfatSetInformation(PVFAT_IRP_CONTEXT IrpContext)
{
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
