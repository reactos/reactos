/*
 * PROJECT:         ReactOS FAT file system driver
 * LICENSE:         GNU GPLv3 as published by the Free Software Foundation
 * FILE:            drivers/filesystems/fastfat/cleanup.c
 * PURPOSE:         Cleanup routines
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include "fastfat.h"

/* FUNCTIONS ****************************************************************/

/* Last handle to a file object is closed */
NTSTATUS
NTAPI
FatiCleanup(PFAT_IRP_CONTEXT IrpContext, PIRP Irp)
{
    PIO_STACK_LOCATION IrpSp;
    PFILE_OBJECT FileObject;
    TYPE_OF_OPEN TypeOfOpen;
    PSHARE_ACCESS ShareAccess;
    BOOLEAN SendUnlockNotification = FALSE;
    PLARGE_INTEGER TruncateSize = NULL;
    //LARGE_INTEGER LocalTruncateSize;
    BOOLEAN AcquiredVcb = FALSE, AcquiredFcb = FALSE;
    NTSTATUS Status;
    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DPRINT("FatiCleanup\n");
    DPRINT("\tIrp           = %p\n", Irp);
    DPRINT("\t->FileObject  = %p\n", IrpSp->FileObject);

    FileObject = IrpSp->FileObject;
    TypeOfOpen = FatDecodeFileObject(FileObject, &Vcb, &Fcb, &Ccb);

    if (TypeOfOpen == UnopenedFileObject)
    {
        DPRINT1("Unopened File Object\n");

        FatCompleteRequest(IrpContext, Irp, STATUS_SUCCESS);
        return STATUS_SUCCESS;
    }

    if (FlagOn( FileObject->Flags, FO_CLEANUP_COMPLETE ))
    {
        /* Just flush the file */

        if (FlagOn(Vcb->State, VCB_STATE_FLAG_DEFERRED_FLUSH) &&
            FlagOn(FileObject->Flags, FO_FILE_MODIFIED) &&
            !FlagOn(Vcb->State, VCB_STATE_FLAG_WRITE_PROTECTED) &&
            (TypeOfOpen == UserFileOpen))
        {
            //Status = FatFlushFile(IrpContext, Fcb, Flush);
            //if (!NT_SUCCESS(Status)) FatNormalizeAndRaiseStatus(IrpContext, Status);
            UNIMPLEMENTED;
        }

        FatCompleteRequest(IrpContext, Irp, STATUS_SUCCESS);
        return STATUS_SUCCESS;
    }

    if (TypeOfOpen == UserFileOpen ||
        TypeOfOpen == UserDirectoryOpen)
    {
        ASSERT(Fcb != NULL);

        (VOID)FatAcquireExclusiveFcb(IrpContext, Fcb);

        AcquiredFcb = TRUE;

        /* Set FCB flags according to DELETE_ON_CLOSE */
        if (FlagOn(Ccb->Flags, CCB_DELETE_ON_CLOSE))
        {
            ASSERT(FatNodeType(Fcb) != FAT_NTC_ROOT_DCB);

            SetFlag(Fcb->State, FCB_STATE_DELETE_ON_CLOSE);

            /* Issue a notification */
            if (TypeOfOpen == UserDirectoryOpen)
            {
                FsRtlNotifyFullChangeDirectory(Vcb->NotifySync,
                                               &Vcb->NotifyList,
                                               FileObject->FsContext,
                                               NULL,
                                               FALSE,
                                               FALSE,
                                               0,
                                               NULL,
                                               NULL,
                                               NULL);
            }
        }

        /* If file should be deleted, acquire locks */
        if ((Fcb->UncleanCount == 1) &&
            FlagOn(Fcb->State, FCB_STATE_DELETE_ON_CLOSE) &&
            (Fcb->Condition != FcbBad) &&
            !FlagOn(Vcb->State, VCB_STATE_FLAG_WRITE_PROTECTED))
        {
            FatReleaseFcb(IrpContext, Fcb);
            AcquiredFcb = FALSE;

            (VOID)FatAcquireExclusiveVcb(IrpContext, Vcb);
            AcquiredVcb = TRUE;

            (VOID)FatAcquireExclusiveFcb(IrpContext, Fcb);
            AcquiredFcb = TRUE;
        }
    }

    /* Acquire VCB lock if it was a volume open */
    if (TypeOfOpen == UserVolumeOpen)
    {
        (VOID)FatAcquireExclusiveVcb(IrpContext, Vcb);
        AcquiredVcb = TRUE;
    }

    /* Cleanup all notifications */
    if (TypeOfOpen == UserDirectoryOpen)
    {
        FsRtlNotifyCleanup(Vcb->NotifySync,
                           &Vcb->NotifyList,
                           Ccb);
    }

    if (Fcb)
    {
        //TODO: FatVerifyFcb
    }

    switch (TypeOfOpen)
    {
    case DirectoryFile:
    case VirtualVolumeFile:
        DPRINT1("Cleanup VirtualVolumeFile/DirectoryFile\n");
        ShareAccess = NULL;
        break;

    case UserVolumeOpen:
        DPRINT("Cleanup UserVolumeOpen\n");

        if (FlagOn(Ccb->Flags, CCB_COMPLETE_DISMOUNT))
        {
            FatCheckForDismount( IrpContext, Vcb, TRUE );
        } else if (FileObject->WriteAccess &&
            FlagOn(FileObject->Flags, FO_FILE_MODIFIED))
        {
            UNIMPLEMENTED;
        }

        /* Release the volume and send notification */
        if (FlagOn(Vcb->State, VCB_STATE_FLAG_LOCKED) &&
            (Vcb->FileObjectWithVcbLocked == FileObject))
        {
            UNIMPLEMENTED;
            SendUnlockNotification = TRUE;
        }

        ShareAccess = &Vcb->ShareAccess;
        break;

    case EaFile:
        DPRINT1("Cleanup EaFileObject\n");
        ShareAccess = NULL;
        break;

    case UserDirectoryOpen:
        DPRINT("Cleanup UserDirectoryOpen\n");

        ShareAccess = &Fcb->ShareAccess;

        /* Should it be a delayed close? */
        if ((Fcb->UncleanCount == 1) &&
            (Fcb->OpenCount == 1) &&
            (Fcb->Dcb.DirectoryFileOpenCount == 0) &&
            !FlagOn(Fcb->State, FCB_STATE_DELETE_ON_CLOSE) &&
            Fcb->Condition == FcbGood)
        {
            /* Yes, a delayed one */
            SetFlag(Fcb->State, FCB_STATE_DELAY_CLOSE);
        }

        if (VcbGood == Vcb->Condition)
        {
            //FatUpdateDirentFromFcb( IrpContext, FileObject, Fcb, Ccb );
            //TODO: Actually update dirent
        }

        if ((Fcb->UncleanCount == 1) &&
            (FatNodeType(Fcb) == FAT_NTC_DCB) &&
            (FlagOn(Fcb->State, FCB_STATE_DELETE_ON_CLOSE)) &&
            (Fcb->Condition != FcbBad) &&
            !FlagOn(Vcb->State, VCB_STATE_FLAG_WRITE_PROTECTED))
        {
            UNIMPLEMENTED;
        }

        /*  Decrement unclean counter */
        ASSERT(Fcb->UncleanCount != 0);
        Fcb->UncleanCount--;
        break;

    case UserFileOpen:
        DPRINT("Cleanup UserFileOpen\n");

        ShareAccess = &Fcb->ShareAccess;

        /* Should it be a delayed close? */
        if ((FileObject->SectionObjectPointer->DataSectionObject == NULL) &&
            (FileObject->SectionObjectPointer->ImageSectionObject == NULL) &&
            (Fcb->UncleanCount == 1) &&
            (Fcb->OpenCount == 1) &&
            !FlagOn(Fcb->State, FCB_STATE_DELETE_ON_CLOSE) &&
            Fcb->Condition == FcbGood)
        {
            /* Yes, a delayed one */
            //SetFlag(Fcb->State, FCB_STATE_DELAY_CLOSE);
            DPRINT1("Setting a delay on close for some reason for FCB %p, FF handle %p, file name '%wZ'\n", Fcb, Fcb->FatHandle, &Fcb->FullFileName);
        }

        /* Unlock all file locks */
        FsRtlFastUnlockAll(&Fcb->Fcb.Lock,
                           FileObject,
                           IoGetRequestorProcess(Irp),
                           NULL);

        if (Vcb->Condition == VcbGood)
        {
            if (Fcb->Condition != FcbBad)
            {
                //FatUpdateDirentFromFcb( IrpContext, FileObject, Fcb, Ccb );
                // TODO: Update on-disk structures
            }

            if (Fcb->UncleanCount == 1 &&
                Fcb->Condition != FcbBad)
            {
                //DELETE_CONTEXT DeleteContext;

                /* Should this file be deleted on close? */
                if (FlagOn(Fcb->State, FCB_STATE_DELETE_ON_CLOSE) &&
                    !FlagOn(Vcb->State, VCB_STATE_FLAG_WRITE_PROTECTED))
                {
                    UNIMPLEMENTED;
                }
                else
                {
                    if (!FlagOn(Fcb->State, FCB_STATE_PAGEFILE) &&
                        (Fcb->Header.ValidDataLength.LowPart < Fcb->Header.FileSize.LowPart))
                    {
#if 0
                        ULONG ValidDataLength;

                        ValidDataLength = Fcb->Header.ValidDataLength.LowPart;

                        if (ValidDataLength < Fcb->ValidDataToDisk) {
                            ValidDataLength = Fcb->ValidDataToDisk;
                        }

                        if (ValidDataLength < Fcb->Header.FileSize.LowPart)
                        {
                            FatZeroData( IrpContext,
                                Vcb,
                                FileObject,
                                ValidDataLength,
                                Fcb->Header.FileSize.LowPart -
                                ValidDataLength );

                            Fcb->ValidDataToDisk =
                                Fcb->Header.ValidDataLength.LowPart =
                                Fcb->Header.FileSize.LowPart;

                            if (CcIsFileCached(FileObject))
                            {
                                CcSetFileSizes(FileObject, (PCC_FILE_SIZES)&Fcb->Header.AllocationSize);
                            }
                        }
#endif
                        DPRINT1("Zeroing out data is not implemented\n");
                    }
                }

                /* Should the file be truncated on close? */
                if (FlagOn(Fcb->State, FCB_STATE_TRUNCATE_ON_CLOSE))
                {
                    if (Vcb->Condition == VcbGood)
                    {
                        // TODO: Actually truncate the file allocation
                        UNIMPLEMENTED;
                    }

                    /* Remove truncation flag */
                    Fcb->State &= ~FCB_STATE_TRUNCATE_ON_CLOSE;
                }

                /* Check again if it should be deleted */
                if (FlagOn(Fcb->State, FCB_STATE_DELETE_ON_CLOSE) &&
                    Fcb->Header.AllocationSize.LowPart == 0)
                {
                    FatNotifyReportChange(IrpContext,
                                          Vcb,
                                          Fcb,
                                          FILE_NOTIFY_CHANGE_FILE_NAME,
                                          FILE_ACTION_REMOVED);
                }

                /* Remove the entry from the splay table if the file was deleted */
                if (FlagOn(Fcb->State, FCB_STATE_DELETE_ON_CLOSE))
                {
                    FatRemoveNames(IrpContext, Fcb);
                }
            }
        }

        ASSERT(Fcb->UncleanCount != 0);
        Fcb->UncleanCount--;
        if (!FlagOn(FileObject->Flags, FO_CACHE_SUPPORTED))
        {
            ASSERT(Fcb->NonCachedUncleanCount != 0);
            Fcb->NonCachedUncleanCount--;
        }

        if (FlagOn(FileObject->Flags, FO_CACHE_SUPPORTED) &&
            (Fcb->NonCachedUncleanCount != 0) &&
            (Fcb->NonCachedUncleanCount == Fcb->UncleanCount) &&
            (Fcb->SectionObjectPointers.DataSectionObject != NULL))
        {
            CcFlushCache(&Fcb->SectionObjectPointers, NULL, 0, NULL);

            /* Acquire and release PagingIo to get in sync with lazy writer */
            ExAcquireResourceExclusiveLite(Fcb->Header.PagingIoResource, TRUE);
            ExReleaseResourceLite(Fcb->Header.PagingIoResource);

            CcPurgeCacheSection(&Fcb->SectionObjectPointers,
                                NULL,
                                0,
                                FALSE);
        }

        if (Fcb->Condition == FcbBad)
        {
            //TruncateSize = &FatLargeZero;
            UNIMPLEMENTED;
        }

        /*  Cleanup the cache map */
        CcUninitializeCacheMap(FileObject, TruncateSize, NULL);
        break;

    default:
        KeBugCheckEx(FAT_FILE_SYSTEM, __LINE__, (ULONG_PTR)TypeOfOpen, 0, 0);
    }

    /* Cleanup the share access */

    if (ShareAccess)
    {
        DPRINT("Cleaning up the share access\n");
        IoRemoveShareAccess(FileObject, ShareAccess);
    }

    if (TypeOfOpen == UserFileOpen)
    {
        /* Update oplocks */
        FsRtlCheckOplock(&Fcb->Fcb.Oplock,
                         Irp,
                         IrpContext,
                         NULL,
                         NULL);

        Fcb->Header.IsFastIoPossible = FatIsFastIoPossible(Fcb);
    }

    /* Set the FO_CLEANUP_COMPLETE flag */
    SetFlag(FileObject->Flags, FO_CLEANUP_COMPLETE);

    Status = STATUS_SUCCESS;

    // TODO: Unpin repinned BCBs
    //FatUnpinRepinnedBcbs(IrpContext);

    /* Flush the volume if necessary */
    if (FlagOn(Vcb->State, VCB_STATE_FLAG_DEFERRED_FLUSH) &&
        !FlagOn(Vcb->State, VCB_STATE_FLAG_WRITE_PROTECTED))
    {
        UNIMPLEMENTED;
    }

    /* Cleanup */
    if (AcquiredFcb) FatReleaseFcb(IrpContext, Fcb);
    if (AcquiredVcb) FatReleaseVcb(IrpContext, Vcb);

    /* Send volume notification */
    if (SendUnlockNotification)
        FsRtlNotifyVolumeEvent(FileObject, FSRTL_VOLUME_UNLOCK);

    return Status;
}

NTSTATUS
NTAPI
FatCleanup(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PFAT_IRP_CONTEXT IrpContext;
    NTSTATUS Status;

    DPRINT("FatCleanup(DeviceObject %p, Irp %p)\n", DeviceObject, Irp);

    /* FatCleanup works only with a volume device object */
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
    Status = FatiCleanup(IrpContext, Irp);

    /* Leave FsRtl critical region */
    FsRtlExitFileSystem();

    return Status;
}

/* EOF */
