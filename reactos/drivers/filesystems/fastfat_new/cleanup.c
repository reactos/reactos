/*
 * PROJECT:         ReactOS FAT file system driver
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/filesystems/fastfat/cleanup.c
 * PURPOSE:         Cleanup routines
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include "fastfat.h"

/* FUNCTIONS ****************************************************************/

NTSTATUS
VfatCleanup(PVFAT_IRP_CONTEXT IrpContext)
{
    DPRINT("VfatCleanup(DeviceObject %p, Irp %p)\n", IrpContext->DeviceObject, IrpContext->Irp);

    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
