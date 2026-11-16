/*
 * PROJECT:     VerOS NT Kernel
 * LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
 * FILE:        drivers/filesystems/ntfs/close.c
 * PURPOSE:     NTFS filesystem driver
 * COPYRIGHT:   Copyright (C) 2002 ReactOS Team
 *              See COPYING in the top level directory
 *              Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Art Yerkes
 *              Ryan Wilson <ryan@verosnt.com>
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

    /* SectionObjectPointer is deliberately left alone. Mm reaches the file's section object
     * pointers through it for as long as a section exists, which outlasts the handle.
     * OpenHandleCount was already dropped in cleanup. */

    UNREFERENCED_PARAMETER(DeviceExt);

    if (FileObject->FileName.Buffer)
    {
        // This a FO, that was created outside from FSD.
        // Some FO's are created with IoCreateStreamFileObject() insid from FSD.
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

    Status = NtfsCloseFile(DeviceExtension, FileObject);

    if (Status == STATUS_PENDING)
    {
        return NtfsMarkIrpContextForQueue(IrpContext);
    }

    IrpContext->Irp->IoStatus.Information = 0;
    return Status;
}

/* EOF */
