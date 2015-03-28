/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/fs/vfat/cleanup.c
 * PURPOSE:          VFAT Filesystem
 * PROGRAMMER:       Jason Filby (jasonfilby@yahoo.com)
 */

/* INCLUDES *****************************************************************/

#include "vfat.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

/*
 * FUNCTION: Cleans up after a file has been closed.
 */
static
NTSTATUS
VfatCleanupFile(
    PVFAT_IRP_CONTEXT IrpContext)
{
    PVFATFCB pFcb;
    PDEVICE_EXTENSION DeviceExt = IrpContext->DeviceExt;
    PFILE_OBJECT FileObject = IrpContext->FileObject;

    DPRINT("VfatCleanupFile(DeviceExt %p, FileObject %p)\n",
           IrpContext->DeviceExt, FileObject);

    /* FIXME: handle file/directory deletion here */
    pFcb = (PVFATFCB)FileObject->FsContext;
    if (!pFcb)
        return STATUS_SUCCESS;

    if (pFcb->Flags & FCB_IS_VOLUME)
    {
        pFcb->OpenHandleCount--;

        if (pFcb->OpenHandleCount != 0)
        {
            IoRemoveShareAccess(FileObject, &pFcb->FCBShareAccess);
        }
    }
    else
    {
        if(!ExAcquireResourceExclusiveLite(&pFcb->MainResource,
                                           (BOOLEAN)(IrpContext->Flags & IRPCONTEXT_CANWAIT)))
        {
            return STATUS_PENDING;
        }
        if(!ExAcquireResourceExclusiveLite(&pFcb->PagingIoResource,
                                           (BOOLEAN)(IrpContext->Flags & IRPCONTEXT_CANWAIT)))
        {
            ExReleaseResourceLite(&pFcb->MainResource);
            return STATUS_PENDING;
        }

        /* Notify about the cleanup */
        FsRtlNotifyCleanup(IrpContext->DeviceExt->NotifySync,
                           &(IrpContext->DeviceExt->NotifyList),
                           FileObject->FsContext2);

        pFcb->OpenHandleCount--;
        DeviceExt->OpenHandleCount--;

        if (!(*pFcb->Attributes & FILE_ATTRIBUTE_DIRECTORY) &&
            FsRtlAreThereCurrentFileLocks(&pFcb->FileLock))
        {
            /* remove all locks this process have on this file */
            FsRtlFastUnlockAll(&pFcb->FileLock,
                               FileObject,
                               IoGetRequestorProcess(IrpContext->Irp),
                               NULL);
        }

        if (pFcb->Flags & FCB_IS_DIRTY)
        {
            VfatUpdateEntry (pFcb);
        }

        if (pFcb->Flags & FCB_DELETE_PENDING &&
            pFcb->OpenHandleCount == 0)
        {
            PFILE_OBJECT tmpFileObject;
            tmpFileObject = pFcb->FileObject;
            if (tmpFileObject != NULL)
            {
                pFcb->FileObject = NULL;
                CcUninitializeCacheMap(tmpFileObject, NULL, NULL);
                ObDereferenceObject(tmpFileObject);
            }

            pFcb->RFCB.ValidDataLength.QuadPart = 0;
            pFcb->RFCB.FileSize.QuadPart = 0;
            pFcb->RFCB.AllocationSize.QuadPart = 0;
        }

        /* Uninitialize the cache (should be done even if caching was never initialized) */
        CcUninitializeCacheMap(FileObject, &pFcb->RFCB.FileSize, NULL);

        if (pFcb->Flags & FCB_DELETE_PENDING &&
            pFcb->OpenHandleCount == 0)
        {
            VfatDelEntry(DeviceExt, pFcb, NULL);

            FsRtlNotifyFullReportChange(DeviceExt->NotifySync,
                                        &(DeviceExt->NotifyList),
                                        (PSTRING)&pFcb->PathNameU,
                                        pFcb->PathNameU.Length - pFcb->LongNameU.Length,
                                        NULL,
                                        NULL,
                                        ((*pFcb->Attributes & FILE_ATTRIBUTE_DIRECTORY) ?
                                        FILE_NOTIFY_CHANGE_DIR_NAME : FILE_NOTIFY_CHANGE_FILE_NAME),
                                        FILE_ACTION_REMOVED,
                                        NULL);
        }

        if (pFcb->OpenHandleCount != 0)
        {
            IoRemoveShareAccess(FileObject, &pFcb->FCBShareAccess);
        }

        FileObject->Flags |= FO_CLEANUP_COMPLETE;

        ExReleaseResourceLite(&pFcb->PagingIoResource);
        ExReleaseResourceLite(&pFcb->MainResource);
    }

#ifdef ENABLE_SWAPOUT
    if (DeviceExt->Flags & VCB_DISMOUNT_PENDING)
    {
        VfatCheckForDismount(DeviceExt, FALSE);
    }
#endif

    return STATUS_SUCCESS;
}

/*
 * FUNCTION: Cleans up after a file has been closed.
 */
NTSTATUS
VfatCleanup(
    PVFAT_IRP_CONTEXT IrpContext)
{
    NTSTATUS Status;

    DPRINT("VfatCleanup(DeviceObject %p, Irp %p)\n", IrpContext->DeviceObject, IrpContext->Irp);

    if (IrpContext->DeviceObject == VfatGlobalData->DeviceObject)
    {
        Status = STATUS_SUCCESS;
        goto ByeBye;
    }

    if (!ExAcquireResourceExclusiveLite(&IrpContext->DeviceExt->DirResource,
                                        (BOOLEAN)(IrpContext->Flags & IRPCONTEXT_CANWAIT)))
    {
        return VfatQueueRequest(IrpContext);
    }

    Status = VfatCleanupFile(IrpContext);

    ExReleaseResourceLite(&IrpContext->DeviceExt->DirResource);

    if (Status == STATUS_PENDING)
    {
        return VfatQueueRequest(IrpContext);
    }

ByeBye:
    IrpContext->Irp->IoStatus.Status = Status;
    IrpContext->Irp->IoStatus.Information = 0;

    IoCompleteRequest(IrpContext->Irp, IO_NO_INCREMENT);
    VfatFreeIrpContext(IrpContext);
    return Status;
}

/* EOF */
