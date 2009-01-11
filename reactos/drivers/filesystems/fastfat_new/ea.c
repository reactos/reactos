/*
 * PROJECT:         ReactOS FAT file system driver
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/filesystems/fastfat/ea.c
 * PURPOSE:         Extended Attributes support
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include "fastfat.h"

/* FUNCTIONS *****************************************************************/

NTSTATUS
VfatSetExtendedAttributes(PFILE_OBJECT FileObject,
                          PVOID Ea,
                          ULONG EaLength)
{
    return STATUS_EAS_NOT_SUPPORTED;
}
