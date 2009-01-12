/*
 * PROJECT:         ReactOS FAT file system driver
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/filesystems/fastfat/device.c
 * PURPOSE:         Device control
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include "fastfat.h"

/* FUNCTIONS ****************************************************************/

NTSTATUS
NTAPI
FatDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
