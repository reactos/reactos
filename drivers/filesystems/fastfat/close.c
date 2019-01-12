/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/filesystems/fastfat/close.c
 * PURPOSE:          VFAT Filesystem
 * PROGRAMMER:       Jason Filby (jasonfilby@yahoo.com)
 *                   Pierre Schweitzer (pierre@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include "vfat.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

VOID
VfatCommonCloseFile(
    PDEVICE_EXTENSION DeviceExt,
    PVFATFCB pFcb)
{
    /* Nothing to do for volumes or for the FAT file object */
    if (BooleanFlagOn(pFcb->Flags, FCB_IS_FAT | FCB_IS_VOLUME))
    {
        return;
    }

    /* If cache is still initialized, release it
     * This only affects directories
     */
    if (pFcb->OpenHandleCount == 0 && BooleanFlagOn(pFcb->Flags, FCB_CACHE_INITIALIZED))
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
    }

#ifdef KDBG
    pFcb->Flags |= FCB_CLOSED;
#endif

    /* Release the FCB, we likely cause its deletion */
    vfatReleaseFCB(DeviceExt, pFcb);
}

VOID
NTAPI
VfatCloseWorker(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context)
{
    PLIST_ENTRY Entry;
    PVFATFCB pFcb;
    PDEVICE_EXTENSION Vcb;
    PVFAT_CLOSE_CONTEXT CloseContext;
    BOOLEAN ConcurrentDeletion;

    /* Start removing work items */
    ExAcquireFastMutex(&VfatGlobalData->CloseMutex);
    while (!IsListEmpty(&VfatGlobalData->CloseListHead))
    {
        Entry = RemoveHeadList(&VfatGlobalData->CloseListHead);
        CloseContext = CONTAINING_RECORD(Entry, VFAT_CLOSE_CONTEXT, CloseListEntry);

        /* One less */
        --VfatGlobalData->CloseCount;
        /* Reset its entry to detect concurrent deletions */
        InitializeListHead(&CloseContext->CloseListEntry);
        ExReleaseFastMutex(&VfatGlobalData->CloseMutex);

        /* Get the elements */
        Vcb = CloseContext->Vcb;
        pFcb = CloseContext->Fcb;
        ExAcquireResourceExclusiveLite(&Vcb->DirResource, TRUE);
        /* If it didn't got deleted in between */
        if (BooleanFlagOn(pFcb->Flags, FCB_DELAYED_CLOSE))
        {
            /* Close it! */
            DPRINT("Late closing: %wZ\n", &pFcb->PathNameU);
            ClearFlag(pFcb->Flags, FCB_DELAYED_CLOSE);
            pFcb->CloseContext = NULL;
            VfatCommonCloseFile(Vcb, pFcb);
            ConcurrentDeletion = FALSE;
        }
        else
        {
            /* Otherwise, mark not to delete it */
            ConcurrentDeletion = TRUE;
        }
        ExReleaseResourceLite(&Vcb->DirResource);

        /* If we were the fastest, delete the context */
        if (!ConcurrentDeletion)
        {
            ExFreeToPagedLookasideList(&VfatGlobalData->CloseContextLookasideList, CloseContext);
        }

        /* Lock again the list */
        ExAcquireFastMutex(&VfatGlobalData->CloseMutex);
    }

    /* We're done, bye! */
    VfatGlobalData->CloseWorkerRunning = FALSE;
    ExReleaseFastMutex(&VfatGlobalData->CloseMutex);
}

NTSTATUS
VfatPostCloseFile(
    PDEVICE_EXTENSION DeviceExt,
    PFILE_OBJECT FileObject)
{
    PVFAT_CLOSE_CONTEXT CloseContext;

    /* Allocate a work item */
    CloseContext = ExAllocateFromPagedLookasideList(&VfatGlobalData->CloseContextLookasideList);
    if (CloseContext == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Set relevant fields */
    CloseContext->Vcb = DeviceExt;
    CloseContext->Fcb = FileObject->FsContext;
    CloseContext->Fcb->CloseContext = CloseContext;

    /* Acquire the lock to insert in list */
    ExAcquireFastMutex(&VfatGlobalData->CloseMutex);

    /* One more element */
    InsertTailList(&VfatGlobalData->CloseListHead, &CloseContext->CloseListEntry);
    ++VfatGlobalData->CloseCount;

    /* If we have more than 16 items in list, and no worker thread
     * start a new one
     */
    if (VfatGlobalData->CloseCount > 16 && !VfatGlobalData->CloseWorkerRunning)
    {
        VfatGlobalData->CloseWorkerRunning = TRUE;
        IoQueueWorkItem(VfatGlobalData->CloseWorkItem, VfatCloseWorker, CriticalWorkQueue, NULL);
    }

    /* We're done */
    ExReleaseFastMutex(&VfatGlobalData->CloseMutex);

    return STATUS_SUCCESS;
}

/*
 * FUNCTION: Closes a file
 */
NTSTATUS
VfatCloseFile(
    PDEVICE_EXTENSION DeviceExt,
    PFILE_OBJECT FileObject)
{
    PVFATFCB pFcb;
    PVFATCCB pCcb;
    BOOLEAN IsVolume;
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("VfatCloseFile(DeviceExt %p, FileObject %p)\n",
            DeviceExt, FileObject);

    /* FIXME : update entry in directory? */
    pCcb = (PVFATCCB) (FileObject->FsContext2);
    pFcb = (PVFATFCB) (FileObject->FsContext);

    if (pFcb == NULL)
    {
        return STATUS_SUCCESS;
    }

    IsVolume = BooleanFlagOn(pFcb->Flags, FCB_IS_VOLUME);

    if (pCcb)
    {
        vfatDestroyCCB(pCcb);
    }

    /* If we have to close immediately, or if delaying failed, close */
    if (VfatGlobalData->ShutdownStarted || !BooleanFlagOn(pFcb->Flags, FCB_DELAYED_CLOSE) ||
        !NT_SUCCESS(VfatPostCloseFile(DeviceExt, FileObject)))
    {
        VfatCommonCloseFile(DeviceExt, pFcb);
    }

    FileObject->FsContext2 = NULL;
    FileObject->FsContext = NULL;
    FileObject->SectionObjectPointer = NULL;

#ifdef ENABLE_SWAPOUT
    if (IsVolume && DeviceExt->OpenHandleCount == 0)
    {
        VfatCheckForDismount(DeviceExt, FALSE);
    }
#endif

    return Status;
}

/*
 * FUNCTION: Closes a file
 */
NTSTATUS
VfatClose(
    PVFAT_IRP_CONTEXT IrpContext)
{
    NTSTATUS Status;

    DPRINT("VfatClose(DeviceObject %p, Irp %p)\n", IrpContext->DeviceObject, IrpContext->Irp);

    if (IrpContext->DeviceObject == VfatGlobalData->DeviceObject)
    {
        DPRINT("Closing file system\n");
        IrpContext->Irp->IoStatus.Information = 0;
        return STATUS_SUCCESS;
    }
    if (!ExAcquireResourceExclusiveLite(&IrpContext->DeviceExt->DirResource, BooleanFlagOn(IrpContext->Flags, IRPCONTEXT_CANWAIT)))
    {
        return VfatMarkIrpContextForQueue(IrpContext);
    }

    Status = VfatCloseFile(IrpContext->DeviceExt, IrpContext->FileObject);
    ExReleaseResourceLite(&IrpContext->DeviceExt->DirResource);

    IrpContext->Irp->IoStatus.Information = 0;

    return Status;
}

/* EOF */
