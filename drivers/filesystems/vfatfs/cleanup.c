/*
 * PROJECT:     VFAT Filesystem
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Cleanup routines
 * COPYRIGHT:   Copyright 1998 Jason Filby <jasonfilby@yahoo.com>
 *              Copyright 2014-2018 Pierre Schweitzer <pierre@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include "vfat.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

/*
 * FUNCTION: Cleans up after a file has been closed.
 * Returns whether the device was deleted
 */
static
BOOLEAN
VfatCleanupFile(
    PVFAT_IRP_CONTEXT IrpContext)
{
    PVFATFCB pFcb;
    PVFATCCB pCcb;
    BOOLEAN IsVolume;
    PDEVICE_EXTENSION DeviceExt = IrpContext->DeviceExt;
    PFILE_OBJECT FileObject = IrpContext->FileObject;
    BOOLEAN Deleted = FALSE;

    DPRINT("VfatCleanupFile(DeviceExt %p, FileObject %p)\n",
           IrpContext->DeviceExt, FileObject);

    /* FIXME: handle file/directory deletion here */
    pFcb = (PVFATFCB)FileObject->FsContext;
    if (!pFcb)
        return FALSE;

    IsVolume = BooleanFlagOn(pFcb->Flags, FCB_IS_VOLUME);
    if (IsVolume)
    {
        pFcb->OpenHandleCount--;
        DeviceExt->OpenHandleCount--;

        if (pFcb->OpenHandleCount != 0)
        {
            IoRemoveShareAccess(FileObject, &pFcb->FCBShareAccess);
        }
    }
    else
    {
        ExAcquireResourceExclusiveLite(&pFcb->MainResource, TRUE);
        ExAcquireResourceExclusiveLite(&pFcb->PagingIoResource, TRUE);

        pCcb = FileObject->FsContext2;
        if (BooleanFlagOn(pCcb->Flags, CCB_DELETE_ON_CLOSE))
        {
            pFcb->Flags |= FCB_DELETE_PENDING;
        }

        /* Notify about the cleanup */
        FsRtlNotifyCleanup(IrpContext->DeviceExt->NotifySync,
                           &(IrpContext->DeviceExt->NotifyList),
                           FileObject->FsContext2);

        pFcb->OpenHandleCount--;
        DeviceExt->OpenHandleCount--;

        if (!vfatFCBIsDirectory(pFcb) &&
            FsRtlAreThereCurrentFileLocks(&pFcb->FileLock))
        {
            /* remove all locks this process have on this file */
            FsRtlFastUnlockAll(&pFcb->FileLock,
                               FileObject,
                               IoGetRequestorProcess(IrpContext->Irp),
                               NULL);
        }

        if (BooleanFlagOn(pFcb->Flags, FCB_IS_DIRTY))
        {
            VfatUpdateEntry (DeviceExt, pFcb);
        }

        if (BooleanFlagOn(pFcb->Flags, FCB_DELETE_PENDING) &&
            pFcb->OpenHandleCount == 0)
        {
            if (vfatFCBIsDirectory(pFcb) &&
                !VfatIsDirectoryEmpty(DeviceExt, pFcb))
            {
                pFcb->Flags &= ~FCB_DELETE_PENDING;
            }
            else
            {
                PFILE_OBJECT tmpFileObject;
                tmpFileObject = pFcb->FileObject;
                if (tmpFileObject != NULL)
                {
                    pFcb->FileObject = NULL;
                    CcUninitializeCacheMap(tmpFileObject, NULL, NULL);
                    ClearFlag(pFcb->Flags, FCB_CACHE_INITIALIZED);
                    ObDereferenceObject(tmpFileObject);
                }

                pFcb->RFCB.ValidDataLength.QuadPart = 0;
                pFcb->RFCB.FileSize.QuadPart = 0;
                pFcb->RFCB.AllocationSize.QuadPart = 0;
            }
        }

        /* Uninitialize the cache (should be done even if caching was never initialized) */
        CcUninitializeCacheMap(FileObject, &pFcb->RFCB.FileSize, NULL);

        if (BooleanFlagOn(pFcb->Flags, FCB_DELETE_PENDING) &&
            pFcb->OpenHandleCount == 0)
        {
            VfatDelEntry(DeviceExt, pFcb, NULL);

            vfatReportChange(DeviceExt,
                             pFcb,
                             (vfatFCBIsDirectory(pFcb) ?
                              FILE_NOTIFY_CHANGE_DIR_NAME : FILE_NOTIFY_CHANGE_FILE_NAME),
                             FILE_ACTION_REMOVED);
        }

        if (pFcb->OpenHandleCount != 0)
        {
            IoRemoveShareAccess(FileObject, &pFcb->FCBShareAccess);
        }
/* FIXME: causes FS corruption and breaks selfhosting/testbots and so on */
#if 0
        /* If that's the last open handle we just closed, try to see whether
         * we can delay close operation
         */
        else if (!BooleanFlagOn(pFcb->Flags, FCB_DELETE_PENDING) && !BooleanFlagOn(pFcb->Flags, FCB_IS_PAGE_FILE) &&
                 !BooleanFlagOn(pFcb->Flags, FCB_IS_FAT) && !BooleanFlagOn(pFcb->Flags, FCB_IS_VOLUME))
        {
            /* This is only allowed if that's a directory with no open files
             * OR if it's a file with no section opened
             */
            if ((vfatFCBIsDirectory(pFcb) && IsListEmpty(&pFcb->ParentListHead)) ||
                (!vfatFCBIsDirectory(pFcb) && FileObject->SectionObjectPointer->DataSectionObject == NULL &&
                 FileObject->SectionObjectPointer->ImageSectionObject == NULL))
            {
                DPRINT("Delaying close of: %wZ\n", &pFcb->PathNameU);
                SetFlag(pFcb->Flags, FCB_DELAYED_CLOSE);
            }
        }
#endif

        FileObject->Flags |= FO_CLEANUP_COMPLETE;
#ifdef KDBG
        pFcb->Flags |= FCB_CLEANED_UP;
#endif

        ExReleaseResourceLite(&pFcb->PagingIoResource);
        ExReleaseResourceLite(&pFcb->MainResource);
    }

#ifdef ENABLE_SWAPOUT
    if (IsVolume && BooleanFlagOn(DeviceExt->Flags, VCB_DISMOUNT_PENDING))
    {
        Deleted = VfatCheckForDismount(DeviceExt, TRUE);
    }
#endif

    return Deleted;
}

/*
 * FUNCTION: Cleans up after a file has been closed.
 */
NTSTATUS
VfatCleanup(
    PVFAT_IRP_CONTEXT IrpContext)
{
    BOOLEAN Deleted;

    DPRINT("VfatCleanup(DeviceObject %p, Irp %p)\n", IrpContext->DeviceObject, IrpContext->Irp);

    if (IrpContext->DeviceObject == VfatGlobalData->DeviceObject)
    {
        IrpContext->Irp->IoStatus.Information = 0;
        return STATUS_SUCCESS;
    }

    ExAcquireResourceExclusiveLite(&IrpContext->DeviceExt->DirResource, TRUE);
    Deleted = VfatCleanupFile(IrpContext);
    if (!Deleted) ExReleaseResourceLite(&IrpContext->DeviceExt->DirResource);

    IrpContext->Irp->IoStatus.Information = 0;
    return STATUS_SUCCESS;
}

/* EOF */
