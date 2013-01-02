/*
 * PROJECT:         ReactOS FAT file system driver
 * LICENSE:         GNU GPLv3 as published by the Free Software Foundation
 * FILE:            drivers/filesystems/fastfat/close.c
 * PURPOSE:         Closing routines
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include "fastfat.h"

VOID NTAPI
FatQueueClose(IN PCLOSE_CONTEXT CloseContext,
              IN BOOLEAN DelayClose);

PCLOSE_CONTEXT NTAPI
FatRemoveClose(PVCB Vcb OPTIONAL,
               PVCB LastVcbHint OPTIONAL);

const ULONG FatMaxDelayedCloseCount = 16;

/* FUNCTIONS ****************************************************************/

NTSTATUS
NTAPI
FatiCommonClose(IN PVCB Vcb,
                IN PFCB Fcb,
                IN PCCB Ccb,
                IN TYPE_OF_OPEN TypeOfOpen,
                IN BOOLEAN Wait,
                OUT PBOOLEAN VcbDeleted)
{
    NTSTATUS Status;
    PFCB ParentDcb;
    BOOLEAN RecursiveClose, VcbDeletedLv = FALSE;
    FAT_IRP_CONTEXT IrpContext;

    if (VcbDeleted) *VcbDeleted = FALSE;

    if (TypeOfOpen == UnopenedFileObject)
    {
        DPRINT1("Closing unopened file object\n");
        Status = STATUS_SUCCESS;
        return Status;
    }

    RtlZeroMemory(&IrpContext, sizeof(FAT_IRP_CONTEXT));

    IrpContext.NodeTypeCode = FAT_NTC_IRP_CONTEXT;
    IrpContext.NodeByteSize = sizeof(IrpContext);
    IrpContext.MajorFunction = IRP_MJ_CLOSE;

    if (Wait) SetFlag(IrpContext.Flags, IRPCONTEXT_CANWAIT);

    if (!ExAcquireResourceExclusiveLite(&Vcb->Resource, Wait)) return STATUS_PENDING;

    if (Vcb->State & VCB_STATE_FLAG_CLOSE_IN_PROGRESS)
    {
        RecursiveClose = TRUE;
    }
    else
    {
        SetFlag(Vcb->State, VCB_STATE_FLAG_CLOSE_IN_PROGRESS);
        RecursiveClose = FALSE;

        Vcb->OpenFileCount++;
    }

    /* Update on-disk structures */
    switch (TypeOfOpen)
    {
    case VirtualVolumeFile:
        DPRINT1("Close VirtualVolumeFile\n");

        InterlockedDecrement((PLONG)&(Vcb->InternalOpenCount));
        InterlockedDecrement((PLONG)&(Vcb->ResidualOpenCount));

        Status = STATUS_SUCCESS;
        goto close_done;
        break;

    case UserVolumeOpen:
        DPRINT1("Close UserVolumeOpen\n");

        Vcb->DirectAccessOpenCount--;
        Vcb->OpenFileCount--;
        if (FlagOn(Ccb->Flags, CCB_READ_ONLY)) Vcb->ReadOnlyCount--;

        FatDeleteCcb(&IrpContext, Ccb);

        Status = STATUS_SUCCESS;
        goto close_done;
        break;

    case EaFile:
        UNIMPLEMENTED;
        break;

    case DirectoryFile:
        DPRINT1("Close DirectoryFile\n");

        InterlockedDecrement((PLONG)&(Fcb->Dcb.DirectoryFileOpenCount));
        InterlockedDecrement((PLONG)&(Vcb->InternalOpenCount));

        if (FatNodeType(Fcb) == FAT_NTC_ROOT_DCB)
        {
            InterlockedDecrement((PLONG)&(Vcb->ResidualOpenCount));
        }

        if (RecursiveClose)
        {
            Status = STATUS_SUCCESS;
            goto close_done;
        }
        else
        {
            break;
        }

    case UserDirectoryOpen:
    case UserFileOpen:
        DPRINT("Close UserFileOpen/UserDirectoryOpen\n");

        if ((FatNodeType(Fcb) == FAT_NTC_DCB) &&
            IsListEmpty(&Fcb->Dcb.ParentDcbList) &&
            (Fcb->OpenCount == 1) &&
            (Fcb->Dcb.DirectoryFile != NULL))
        {
                PFILE_OBJECT DirectoryFileObject = Fcb->Dcb.DirectoryFile;

                DPRINT1("Uninitialize the stream file object\n");

                CcUninitializeCacheMap(DirectoryFileObject, NULL, NULL);

                Fcb->Dcb.DirectoryFile = NULL;
                ObDereferenceObject(DirectoryFileObject);
        }

        Fcb->OpenCount--;
        Vcb->OpenFileCount--;
        if (FlagOn(Ccb->Flags, CCB_READ_ONLY)) Vcb->ReadOnlyCount --;

        FatDeleteCcb(&IrpContext, Ccb);
        break;

    default:
        KeBugCheckEx(FAT_FILE_SYSTEM, __LINE__, (ULONG_PTR)TypeOfOpen, 0, 0);
    }

    /* Update in-memory structures */
    if (((FatNodeType(Fcb) == FAT_NTC_FCB) &&
        (Fcb->OpenCount == 0))
        ||
        ((FatNodeType(Fcb) == FAT_NTC_DCB) &&
        (IsListEmpty(&Fcb->Dcb.ParentDcbList)) &&
        (Fcb->OpenCount == 0) &&
        (Fcb->Dcb.DirectoryFileOpenCount == 0)))
    {
        ParentDcb = Fcb->ParentFcb;

        SetFlag(Vcb->State, VCB_STATE_FLAG_DELETED_FCB);

        FatDeleteFcb(&IrpContext, Fcb);

        while ((FatNodeType(ParentDcb) == FAT_NTC_DCB) &&
            IsListEmpty(&ParentDcb->Dcb.ParentDcbList) &&
            (ParentDcb->OpenCount == 0) &&
            (ParentDcb->Dcb.DirectoryFile != NULL))
        {
                PFILE_OBJECT DirectoryFileObject;

                DirectoryFileObject = ParentDcb->Dcb.DirectoryFile;

                DPRINT1("Uninitialize parent Stream Cache Map\n");

                CcUninitializeCacheMap(DirectoryFileObject, NULL, NULL);

                ParentDcb->Dcb.DirectoryFile = NULL;

                ObDereferenceObject(DirectoryFileObject);

                if (ParentDcb->Dcb.DirectoryFileOpenCount == 0)
                {
                    PFCB CurrentDcb;

                    CurrentDcb = ParentDcb;
                    ParentDcb = CurrentDcb->ParentFcb;

                    SetFlag(Vcb->State, VCB_STATE_FLAG_DELETED_FCB);

                    FatDeleteFcb(&IrpContext, CurrentDcb);
                }
                else
                {
                    break;
                }
        }
    }

    Status = STATUS_SUCCESS;

close_done:
    /* Closing is done, check if VCB could be closed too */
    if (!RecursiveClose)
    {
        /* One open left - yes, VCB can go away */
        if (Vcb->OpenFileCount == 1 &&
            !FlagOn(Vcb->State, VCB_STATE_FLAG_DISMOUNT_IN_PROGRESS)
            && VcbDeleted)
        {
            FatReleaseVcb(&IrpContext, Vcb );

            SetFlag(IrpContext.Flags, IRPCONTEXT_CANWAIT);

            FatAcquireExclusiveGlobal(&IrpContext);

            FatAcquireExclusiveVcb(&IrpContext, Vcb);

            Vcb->OpenFileCount--;

            VcbDeletedLv = FatCheckForDismount(&IrpContext, Vcb, FALSE);

            FatReleaseGlobal(&IrpContext);

            if (VcbDeleted) *VcbDeleted = VcbDeletedLv;
        }
        else
        {
            /* Remove extra referenec */
            Vcb->OpenFileCount --;
        }

        /* Clear recursion flag if necessary */
        if (!VcbDeletedLv)
        {
            ClearFlag(Vcb->State, VCB_STATE_FLAG_CLOSE_IN_PROGRESS);
        }
    }

    /* Release VCB if it wasn't deleted */
    if (!VcbDeletedLv)
        FatReleaseVcb(&IrpContext, Vcb);

    return Status;
}

NTSTATUS
NTAPI
FatiClose(IN PFAT_IRP_CONTEXT IrpContext,
          IN PIRP Irp)
{
    PIO_STACK_LOCATION IrpSp;
    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;
    BOOLEAN TopLevel, Wait, VcbDeleted = FALSE, DelayedClose = FALSE;
    NTSTATUS Status = STATUS_SUCCESS;
    PCLOSE_CONTEXT CloseContext = NULL;

    TopLevel = FatIsTopLevelIrp(Irp);

    /* Get current IRP stack location */
    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    /* Decode incoming file object */
    TypeOfOpen = FatDecodeFileObject(IrpSp->FileObject, &Vcb, &Fcb, &Ccb);

    /* Set CCB read only flag */
    if (Ccb && IsFileObjectReadOnly(IrpSp->FileObject))
        SetFlag(Ccb->Flags, CCB_READ_ONLY);

    /* It's possible to wait only if we are top level or not a system process */
    Wait = TopLevel && (PsGetCurrentProcess() != FatGlobalData.SystemProcess);

    /* Determine if it's a delayed close, by flags first */
    if ((TypeOfOpen == UserFileOpen || TypeOfOpen == UserDirectoryOpen) &&
        (Fcb->State & FCB_STATE_DELAY_CLOSE) &&
        !FatGlobalData.ShutdownStarted)
    {
        DelayedClose = TRUE;
    }

    /* If close is not delayed, try to perform the close operation */
    if (!DelayedClose)
        Status = FatiCommonClose(Vcb, Fcb, Ccb, TypeOfOpen, Wait, &VcbDeleted);

    /* We have to delay close if either it's defined by a flag or it was not possible
       to perform it synchronously */
    if (DelayedClose || Status == STATUS_PENDING)
    {
        DPRINT1("Queuing a pending close, Vcb %p, Fcb %p, Ccb %p\n", Vcb, Fcb, Ccb);

        /* Check if a close context should be allocated */
        if (TypeOfOpen == VirtualVolumeFile)
        {
            ASSERT(Vcb->CloseContext != NULL);
            CloseContext = Vcb->CloseContext;
            Vcb->CloseContext = NULL;
            CloseContext->Free = TRUE;
        }
        else if (TypeOfOpen == DirectoryFile ||
                 TypeOfOpen == EaFile)
        {
            UNIMPLEMENTED;
            //CloseContext = FatAllocateCloseContext(Vcb);
            //ASSERT(CloseContext != NULL);
            CloseContext->Free = TRUE;
        }
        else
        {
            //TODO: FatDeallocateCcbStrings( Ccb );

            /* Set CloseContext to a buffer inside Ccb */
            CloseContext = &Ccb->CloseContext;
            CloseContext->Free = FALSE;
            SetFlag(Ccb->Flags, CCB_CLOSE_CONTEXT);
        }

        /* Save all info in the close context */
        CloseContext->Vcb = Vcb;
        CloseContext->Fcb = Fcb;
        CloseContext->TypeOfOpen = TypeOfOpen;

        /* Queue the close */
        FatQueueClose(CloseContext, (BOOLEAN)(Fcb && FlagOn(Fcb->State, FCB_STATE_DELAY_CLOSE)));
    }
    else
    {
        /* Close finished right away */
        if (TypeOfOpen == VirtualVolumeFile ||
            TypeOfOpen == DirectoryFile ||
            TypeOfOpen == EaFile)
        {
                if (TypeOfOpen == VirtualVolumeFile)
                {
                    /* Free close context for the not deleted VCB */
                    if (!VcbDeleted)
                    {
                        CloseContext = Vcb->CloseContext;
                        Vcb->CloseContext = NULL;

                        ASSERT(CloseContext != NULL);
                    }
                }
                else
                {
                    //CloseContext = FatAllocateCloseContext(Vcb);
                    DPRINT1("TODO: Allocate close context!\n");
                    ASSERT(CloseContext != NULL);
                }

                /* Free close context */
                if (CloseContext) ExFreePool(CloseContext);
        }
    }

    /* Complete the request */
    FatCompleteRequest(NULL, Irp, Status);

    /* Reset the top level IRP if necessary */
    if (TopLevel) IoSetTopLevelIrp(NULL);

    return Status;
}

NTSTATUS
NTAPI
FatClose(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PFAT_IRP_CONTEXT IrpContext;
    NTSTATUS Status;

    DPRINT("FatClose(DeviceObject %p, Irp %p)\n", DeviceObject, Irp);

    /* FatClose works only with a volume device object */
    if (DeviceObject == FatGlobalData.DiskDeviceObject)
    {
        /* Complete the request and return success */
        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = FILE_OPENED;

        IoCompleteRequest(Irp, IO_DISK_INCREMENT);

        return STATUS_SUCCESS;
    }

    /* Enter FsRtl critical region */
    FsRtlEnterFileSystem();

    /* Build an irp context */
    IrpContext = FatBuildIrpContext(Irp, TRUE);

    /* Call internal function */
    Status = FatiClose(IrpContext, Irp);

    /* Leave FsRtl critical region */
    FsRtlExitFileSystem();

    return Status;
}

VOID
NTAPI
FatPendingClose(IN PVCB Vcb OPTIONAL)
{
    PCLOSE_CONTEXT CloseContext;
    PVCB CurrentVcb = NULL;
    PVCB LastVcb = NULL;
    BOOLEAN FreeContext;
    ULONG Loops = 0;

    /* Do the top-level IRP trick */
    if (!Vcb) IoSetTopLevelIrp((PIRP)FSRTL_FSP_TOP_LEVEL_IRP);

    while ((CloseContext = FatRemoveClose(Vcb, LastVcb)))
    {
        if (!Vcb)
        {
            if (!FatGlobalData.ShutdownStarted)
            {
                if (CloseContext->Vcb != CurrentVcb)
                {
                    Loops = 0;

                    /* Release previous VCB */
                    if (CurrentVcb)
                        ExReleaseResourceLite(&CurrentVcb->Resource);

                    /* Lock the new VCB */
                    CurrentVcb = CloseContext->Vcb;
                    (VOID)ExAcquireResourceExclusiveLite(&CurrentVcb->Resource, TRUE);
                }
                else
                {
                    /* Try to lock */
                    if (++Loops >= 20)
                    {
                        if (ExGetSharedWaiterCount(&CurrentVcb->Resource) +
                            ExGetExclusiveWaiterCount(&CurrentVcb->Resource))
                        {
                            ExReleaseResourceLite(&CurrentVcb->Resource);
                            (VOID)ExAcquireResourceExclusiveLite(&CurrentVcb->Resource, TRUE);
                        }

                        Loops = 0;
                    }
                }

                /* Check open count */
                if (CurrentVcb->OpenFileCount <= 1)
                {
                    ExReleaseResourceLite(&CurrentVcb->Resource);
                    CurrentVcb = NULL;
                }
            }
            else if (CurrentVcb)
            {
                ExReleaseResourceLite(&CurrentVcb->Resource);
                CurrentVcb = NULL;
            }
        }

        LastVcb = CurrentVcb;

        /* Remember if we should free the context */
        FreeContext = CloseContext->Free;

        FatiCommonClose(CloseContext->Vcb,
                        CloseContext->Fcb,
                        (FreeContext ? NULL : CONTAINING_RECORD(CloseContext, CCB, CloseContext)),
                        CloseContext->TypeOfOpen,
                        TRUE,
                        NULL);

        /* Free context if necessary */
        if (FreeContext) ExFreePool(CloseContext);
    }

    /* Release VCB if necessary */
    if (CurrentVcb) ExReleaseResourceLite(&CurrentVcb->Resource);

    /* Reset top level IRP */
    if (!Vcb) IoSetTopLevelIrp( NULL );
}

VOID
NTAPI
FatCloseWorker(IN PDEVICE_OBJECT DeviceObject,
               IN PVOID Context)
{
    FsRtlEnterFileSystem();

    FatPendingClose((PVCB)Context);

    FsRtlExitFileSystem();
}

VOID
NTAPI
FatQueueClose(IN PCLOSE_CONTEXT CloseContext,
              IN BOOLEAN DelayClose)
{
    BOOLEAN RunWorker = FALSE;

    /* Acquire the close lists mutex */
    ExAcquireFastMutexUnsafe(&FatCloseQueueMutex);

    /* Add it to the desired list */
    if (DelayClose)
    {
        InsertTailList(&FatGlobalData.DelayedCloseList,
                       &CloseContext->GlobalLinks);
        InsertTailList(&CloseContext->Vcb->DelayedCloseList,
                       &CloseContext->VcbLinks);

        FatGlobalData.DelayedCloseCount++;

        if (FatGlobalData.DelayedCloseCount > FatMaxDelayedCloseCount &&
            !FatGlobalData.AsyncCloseActive)
        {
            FatGlobalData.AsyncCloseActive = TRUE;
            RunWorker = TRUE;
        }
    }
    else
    {
        InsertTailList(&FatGlobalData.AsyncCloseList,
                       &CloseContext->GlobalLinks);
        InsertTailList(&CloseContext->Vcb->AsyncCloseList,
                       &CloseContext->VcbLinks);

        FatGlobalData.AsyncCloseCount++;

        if (!FatGlobalData.AsyncCloseActive)
        {
            FatGlobalData.AsyncCloseActive = TRUE;
            RunWorker = TRUE;
        }
    }

    /* Release the close lists mutex */
    ExReleaseFastMutexUnsafe(&FatCloseQueueMutex);

    if (RunWorker)
        IoQueueWorkItem(FatGlobalData.FatCloseItem, FatCloseWorker, CriticalWorkQueue, NULL);
}

PCLOSE_CONTEXT
NTAPI
FatRemoveClose(PVCB Vcb OPTIONAL,
               PVCB LastVcbHint OPTIONAL)
{
    PLIST_ENTRY Entry;
    PCLOSE_CONTEXT CloseContext;
    BOOLEAN IsWorker = FALSE;

    /* Acquire the close lists mutex */
    ExAcquireFastMutexUnsafe(&FatCloseQueueMutex);

    if (!Vcb) IsWorker = TRUE;

    if (Vcb == NULL && LastVcbHint != NULL)
    {
        // TODO: A very special case of overflowing the queue
        UNIMPLEMENTED;
    }

    /* Usual processing from a worker thread */
    if (!Vcb)
    {
TryToCloseAgain:

        /* Is there anything in the async close list */
        if (!IsListEmpty(&FatGlobalData.AsyncCloseList))
        {
            Entry = RemoveHeadList(&FatGlobalData.AsyncCloseList);
            FatGlobalData.AsyncCloseCount--;

            CloseContext = CONTAINING_RECORD(Entry,
                                             CLOSE_CONTEXT,
                                             GlobalLinks);

            RemoveEntryList(&CloseContext->VcbLinks);
        } else if (!IsListEmpty(&FatGlobalData.DelayedCloseList) &&
                   (FatGlobalData.DelayedCloseCount > FatMaxDelayedCloseCount/2 ||
                   FatGlobalData.ShutdownStarted))
        {
            /* In case of a shutdown or when delayed queue is filled at half - perform closing */
            Entry = RemoveHeadList(&FatGlobalData.DelayedCloseList);
            FatGlobalData.DelayedCloseCount--;

            CloseContext = CONTAINING_RECORD(Entry,
                                             CLOSE_CONTEXT,
                                             GlobalLinks);
            RemoveEntryList(&CloseContext->VcbLinks);
        }
        else
        {
            /* Nothing to close */
            CloseContext = NULL;
            if (IsWorker) FatGlobalData.AsyncCloseActive = FALSE;
        }
    }
    else
    {
        if (!IsListEmpty(&Vcb->AsyncCloseList))
        {
            /* Is there anything in the async close list */
            Entry = RemoveHeadList(&Vcb->AsyncCloseList);
            FatGlobalData.AsyncCloseCount--;

            CloseContext = CONTAINING_RECORD(Entry,
                                             CLOSE_CONTEXT,
                                             VcbLinks);

            RemoveEntryList(&CloseContext->GlobalLinks);
        }
        else if (!IsListEmpty(&Vcb->DelayedCloseList))
        {
            /* Process delayed close list */
            Entry = RemoveHeadList(&Vcb->DelayedCloseList);
            FatGlobalData.DelayedCloseCount--;

            CloseContext = CONTAINING_RECORD(Entry,
                                             CLOSE_CONTEXT,
                                             VcbLinks);

            RemoveEntryList(&CloseContext->GlobalLinks);
        }
        else if (LastVcbHint)
        {
            /* Try again */
            goto TryToCloseAgain;
        }
        else
        {
            /* Nothing to close */
            CloseContext = NULL;
        }
    }

    /* Release the close lists mutex */
    ExReleaseFastMutexUnsafe(&FatCloseQueueMutex);

    return CloseContext;
}

/* EOF */
