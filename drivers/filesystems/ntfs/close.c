/*
 *  ReactOS kernel
 *  Copyright (C) 2002 ReactOS Team
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
 * FILE:             drivers/filesystem/ntfs/close.c
 * PURPOSE:          NTFS filesystem driver
 * PROGRAMMER:       Art Yerkes
 * UPDATE HISTORY:
 */

/* INCLUDES *****************************************************************/

#include "ntfs.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

/*
 * FUNCTION: Closes a file
 */
NTSTATUS
NtfsCloseFile(PDEVICE_EXTENSION DeviceExt,
              PFILE_OBJECT FileObject)
{
    PNTFS_CCB Ccb;
    PNTFS_FCB Fcb;

    DPRINT("NtfsCloseFile(DeviceExt %p, FileObject %p)\n",
           DeviceExt,
           FileObject);

    Ccb = (PNTFS_CCB)(FileObject->FsContext2);
    Fcb = (PNTFS_FCB)(FileObject->FsContext);

    DPRINT("Ccb %p\n", Ccb);
    if (Ccb == NULL)
    {
        return STATUS_SUCCESS;
    }

    FileObject->FsContext2 = NULL;
    FileObject->FsContext = NULL;
    FileObject->SectionObjectPointer = NULL;

DPRINT1("DeviceExt->OpenHandleCount = 0x%lx\n", DeviceExt->OpenHandleCount);
DPRINT1("Fcb->OpenHandleCount = 0x%lx\n", Fcb->OpenHandleCount);

    ASSERT(DeviceExt->OpenHandleCount > 0);
    DeviceExt->OpenHandleCount--;

    if (FileObject->FileName.Buffer)
    {
        // This a FO, that was created outside from FSD.
        // Some FO's are created with IoCreateStreamFileObject() inside from FSD.
        // This FO's don't have a FileName.
        NtfsReleaseFCB(DeviceExt, Fcb);
    }

    if (Ccb->DirectorySearchPattern)
    {
        ExFreePool(Ccb->DirectorySearchPattern);
    }

    ExFreePool(Ccb);

    return STATUS_SUCCESS;
}


NTSTATUS
NtfsClose(PNTFS_IRP_CONTEXT IrpContext)
{
    PDEVICE_EXTENSION DeviceExtension;
    PFILE_OBJECT FileObject;
    NTSTATUS Status;
    PDEVICE_OBJECT DeviceObject;

    DPRINT("NtfsClose() called\n");

    DeviceObject = IrpContext->DeviceObject;
    if (DeviceObject == NtfsGlobalData->DeviceObject)
    {
        DPRINT("Closing file system\n");
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

    Status = NtfsCloseFile(DeviceExtension, FileObject);

    ExReleaseResourceLite(&DeviceExtension->DirResource);

    if (Status == STATUS_PENDING)
    {
        return NtfsMarkIrpContextForQueue(IrpContext);
    }

    IrpContext->Irp->IoStatus.Information = 0;
    return Status;
}

/* EOF */
