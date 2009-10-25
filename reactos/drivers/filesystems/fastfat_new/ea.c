/*
 * PROJECT:         ReactOS FAT file system driver
 * LICENSE:         GNU GPLv3 as published by the Free Software Foundation
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
