/*
 *  ReactOS kernel
 *  Copyright (C) 2016 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/filesystem/ntfs/cleanup.c
 * PURPOSE:          NTFS filesystem driver
 * PROGRAMMER:       Pierre Schweitzer (pierre@reactos.org)
 * UPDATE HISTORY:
 */

/* INCLUDES *****************************************************************/

#include "ntfs.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

/*
 * FUNCTION: Cleans up a file
 */
NTSTATUS
NtfsCleanupFile(PDEVICE_EXTENSION DeviceExt,
                PFILE_OBJECT FileObject,
                BOOLEAN CanWait)
{
    PNTFS_FCB Fcb;

    DPRINT("NtfsCleanupFile(DeviceExt %p, FileObject %p, CanWait %u)\n",
           DeviceExt,
           FileObject,
           CanWait);

    Fcb = (PNTFS_FCB)(FileObject->FsContext);
    if (!Fcb)
        return STATUS_SUCCESS;

DPRINT1("DeviceExt->OpenHandleCount = 0x%lx\n", DeviceExt->OpenHandleCount);
DPRINT1("Fcb->OpenHandleCount = 0x%lx\n", Fcb->OpenHandleCount);

    if (Fcb->Flags & FCB_IS_VOLUME)
    {
        ASSERT(Fcb->OpenHandleCount > 0);
        Fcb->OpenHandleCount--;

        if (Fcb->OpenHandleCount != 0)
        {
            // Remove share access when handled
        }
    }
    else
    {
        if (!ExAcquireResourceExclusiveLite(&Fcb->MainResource, CanWait))
        {
            return STATUS_PENDING;
        }

        ASSERT(Fcb->OpenHandleCount > 0);
        Fcb->OpenHandleCount--;

        CcUninitializeCacheMap(FileObject, &Fcb->RFCB.FileSize, NULL);

        if (Fcb->OpenHandleCount != 0)
        {
            // Remove share access when handled
        }

        FileObject->Flags |= FO_CLEANUP_COMPLETE;

        ExReleaseResourceLite(&Fcb->MainResource);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NtfsCleanup(PNTFS_IRP_CONTEXT IrpContext)
{
    PDEVICE_EXTENSION DeviceExtension;
    PFILE_OBJECT FileObject;
    NTSTATUS Status;
    PDEVICE_OBJECT DeviceObject;

    DPRINT("NtfsCleanup() called\n");

    DeviceObject = IrpContext->DeviceObject;
    if (DeviceObject == NtfsGlobalData->DeviceObject)
    {
        DPRINT("Cleaning up file system\n");
        IrpContext->Irp->IoStatus.Information = 0;
        return STATUS_SUCCESS;
    }

    FileObject = IrpContext->FileObject;
    DeviceExtension = DeviceObject->DeviceExtension;

    if (!ExAcquireResourceExclusiveLite(&DeviceExtension->DirResource,
                                        BooleanFlagOn(IrpContext->Flags, IRPCONTEXT_CANWAIT)))
    {
        return NtfsMarkIrpContextForQueue(IrpContext);
    }

    Status = NtfsCleanupFile(DeviceExtension, FileObject, BooleanFlagOn(IrpContext->Flags, IRPCONTEXT_CANWAIT));

    ExReleaseResourceLite(&DeviceExtension->DirResource);

    if (Status == STATUS_PENDING)
    {
        return NtfsMarkIrpContextForQueue(IrpContext);
    }

    IrpContext->Irp->IoStatus.Information = 0;
    return Status;
}
