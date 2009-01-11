/*
 * PROJECT:         ReactOS FAT file system driver
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/filesystems/fastfat/fsctl.c
 * PURPOSE:         Filesystem control
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include "fastfat.h"

/* FUNCTIONS ****************************************************************/

NTSTATUS
VfatFileSystemControl(PVFAT_IRP_CONTEXT IrpContext)
{
    DPRINT("VfatFileSystemControl(IrpContext %p)\n", IrpContext);

    return STATUS_NOT_IMPLEMENTED;
}
