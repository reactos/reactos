/*
 * PROJECT:         ReactOS FAT file system driver
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/filesystems/fastfat/shutdown.c
 * PURPOSE:         Shutdown support routines
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include "fastfat.h"

/* FUNCTIONS ****************************************************************/

NTSTATUS
NTAPI
VfatShutdown(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    DPRINT("VfatShutdown(DeviceObject %p, Irp %p)\n", DeviceObject, Irp);

    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
