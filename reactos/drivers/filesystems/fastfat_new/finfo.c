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

NTSTATUS
NTAPI
FatQueryInformation(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
FatSetInformation(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
