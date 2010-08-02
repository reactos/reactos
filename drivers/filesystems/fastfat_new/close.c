/*
 * PROJECT:         ReactOS FAT file system driver
 * LICENSE:         GNU GPLv3 as published by the Free Software Foundation
 * FILE:            drivers/filesystems/fastfat/close.c
 * PURPOSE:         Closing routines
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include "fastfat.h"

/* FUNCTIONS ****************************************************************/

NTSTATUS
NTAPI
FatClose(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    DPRINT1("FatClose(DeviceObject %p, Irp %p)\n", DeviceObject, Irp);

    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
