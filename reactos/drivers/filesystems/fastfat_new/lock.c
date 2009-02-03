/*
 * PROJECT:         ReactOS FAT file system driver
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/filesystems/fastfat/lock.c
 * PURPOSE:         Lock support routines
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include "fastfat.h"

/* FUNCTIONS ****************************************************************/

NTSTATUS
NTAPI
FatLockControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    DPRINT1("FatLockControl()\n");
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
