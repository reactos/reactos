/*
 * PROJECT:         ReactOS FAT file system driver
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/filesystems/fastfat/dir.c
 * PURPOSE:         Directory Control
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include "fastfat.h"

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
FatDirectoryControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    DPRINT1("FatDirectoryControl()\n");
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
