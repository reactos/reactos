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
NTAPI
FatQueryVolumeInfo(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    DPRINT1("FatQueryVolumeInfo()\n");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
FatSetVolumeInfo(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    DPRINT1("FatSetVolumeInfo()\n");
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
