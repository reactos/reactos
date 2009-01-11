/*
 * PROJECT:         ReactOS FAT file system driver
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/filesystems/fastfat/volume.c
 * PURPOSE:         Volume information
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include "fastfat.h"

/* FUNCTIONS ****************************************************************/

NTSTATUS
VfatQueryVolumeInformation(PVFAT_IRP_CONTEXT IrpContext)
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
VfatSetVolumeInformation(PVFAT_IRP_CONTEXT IrpContext)
{
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
