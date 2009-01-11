/*
 * PROJECT:         ReactOS FAT file system driver
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/filesystems/fastfat/cleanup.c
 * PURPOSE:         Closing routines
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include "fastfat.h"

/* FUNCTIONS ****************************************************************/

NTSTATUS
VfatClose(PFAT_IRP_CONTEXT IrpContext)
{
    DPRINT("VfatClose(DeviceObject %p, Irp %p)\n", IrpContext->DeviceObject, IrpContext->Irp);

    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
