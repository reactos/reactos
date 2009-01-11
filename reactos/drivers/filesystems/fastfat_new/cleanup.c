/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/fs/vfat/cleanup.c
 * PURPOSE:          VFAT Filesystem
 * PROGRAMMER:       Jason Filby (jasonfilby@yahoo.com)
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include "fastfat.h"

/* FUNCTIONS ****************************************************************/

/*
 * FUNCTION: Cleans up after a file has been closed.
 */
static NTSTATUS
VfatCleanupFile(PVFAT_IRP_CONTEXT IrpContext)
{
    PVFATFCB pFcb;
    PFILE_OBJECT FileObject = IrpContext->FileObject;

    DPRINT("VfatCleanupFile(DeviceExt %p, FileObject %p)\n",
           IrpContext->DeviceExt, FileObject);

    /* FIXME: handle file/directory deletion here */
    pFcb = (PVFATFCB) FileObject->FsContext;
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
        if(!ExAcquireResourceExclusiveLite (&pFcb->MainResource,
                                            (BOOLEAN)(IrpContext->Flags & IRPCONTEXT_CANWAIT)))
        {
            return STATUS_PENDING;
        }
        if(!ExAcquireResourceExclusiveLite (&pFcb->PagingIoResource,
                                            (BOOLEAN)(IrpContext->Flags & IRPCONTEXT_CANWAIT)))
        {
            ExReleaseResourceLite (&pFcb->MainResource);
            return STATUS_PENDING;
        }

        pFcb->OpenHandleCount--;

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

           CcPurgeCacheSection(FileObject->SectionObjectPointer, NULL, 0, FALSE);
        }

        /* Uninitialize the cache (should be done even if caching was never initialized) */
        CcUninitializeCacheMap(FileObject, &pFcb->RFCB.FileSize, NULL);

        if (pFcb->OpenHandleCount != 0)
        {
            IoRemoveShareAccess(FileObject, &pFcb->FCBShareAccess);
        }

        FileObject->Flags |= FO_CLEANUP_COMPLETE;

        ExReleaseResourceLite (&pFcb->PagingIoResource);
        ExReleaseResourceLite (&pFcb->MainResource);
    }

    return STATUS_SUCCESS;
}

/*
 * FUNCTION: Cleans up after a file has been closed.
 */
NTSTATUS VfatCleanup(PVFAT_IRP_CONTEXT IrpContext)
{
    NTSTATUS Status;

    DPRINT("VfatCleanup(DeviceObject %p, Irp %p)\n", IrpContext->DeviceObject, IrpContext->Irp);

    if (IrpContext->DeviceObject == VfatGlobalData->DeviceObject)
    {
        Status = STATUS_SUCCESS;
        goto ByeBye;
    }

    if (!ExAcquireResourceExclusiveLite (&IrpContext->DeviceExt->DirResource,
                                         (BOOLEAN)(IrpContext->Flags & IRPCONTEXT_CANWAIT)))
    {
        return VfatQueueRequest (IrpContext);
    }

    Status = VfatCleanupFile(IrpContext);

    ExReleaseResourceLite (&IrpContext->DeviceExt->DirResource);

    if (Status == STATUS_PENDING)
    {
        return VfatQueueRequest(IrpContext);
    }

ByeBye:
    IrpContext->Irp->IoStatus.Status = Status;
    IrpContext->Irp->IoStatus.Information = 0;

    IoCompleteRequest (IrpContext->Irp, IO_NO_INCREMENT);
    VfatFreeIrpContext(IrpContext);
    return (Status);
}

/* EOF */
