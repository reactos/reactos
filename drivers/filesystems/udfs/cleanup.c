////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////
/*

 Module name: Cleanup.cpp

 Abstract:

    Contains code to handle the "Cleanup" dispatch entry point.

 Environment:

    Kernel mode only
*/

#include            "udffs.h"

// define the file specific bug-check id
#define         UDF_BUG_CHECK_ID                UDF_FILE_CLEANUP

VOID
UDFAutoUnlock (
    IN PVCB Vcb
    );

/* Called under this directory's FCB resource. An indexed, referenced FCB
 * and its Dloc retain the unpublished append generation. Final teardown
 * drains it before removing the FCB/LCB; an explicit flush drains before FE.
 * Ordinary file cleanup and all error/write-through boundaries stay eager.
 * Directory FileObjects do not carry FO_CACHE_SUPPORTED (create.c); the
 * pending generation was already admitted by a cached child cleanup. */
static BOOLEAN
UDFCanDeferDirectoryAppendCleanup(PVCB Vcb, PUDF_FILE_INFO FileInfo,
                                  PFILE_OBJECT FileObject)
{
    PFCB fcb;
    if (!FileInfo || !FileInfo->Dloc || !(fcb = FileInfo->Fcb) ||
        FileInfo->Dloc->CommonFcb != fcb ||
        !(fcb->FcbState & UDF_FCB_DIRECTORY) ||
        (fcb->FcbState & (UDF_FCB_DELETE_ON_CLOSE | UDF_FCB_DELETED)) ||
        !(fcb->FcbState & FCB_STATE_IN_FCB_TABLE) || fcb->FcbReference <= 0 ||
        !FileInfo->Dloc->DirIndex || UDFIsAStream(FileInfo) || UDFIsAStreamDir(FileInfo) ||
        Vcb->VcbCondition != VcbMounted ||
        FlagOn(Vcb->VcbState, VCB_STATE_VOLUME_READ_ONLY | UDF_VCB_FLAGS_DEAD) ||
        !FileObject ||
        FlagOn(FileObject->Flags, FO_WRITE_THROUGH | FO_NO_INTERMEDIATE_BUFFERING))
        return FALSE;
    return UDFDirectoryAppendPending(FileInfo);
}

/* Limit asynchronous cached-data scheduling to the same canonical,
 * single-link unpublished owner as metadata batching. Embedded payload,
 * mixed noncached opens, zero tails, streams and WT retain eager cleanup.
 * CcUninitializeCacheMap is still called at the original cleanup boundary. */
static BOOLEAN
UDFCanDeferNewFileDataCleanup(PIRP_CONTEXT IrpContext, PVCB Vcb,
                              PUDF_FILE_INFO FileInfo)
{
    PFCB fcb;
    if (!UDFDeferNewFileMetadata(IrpContext, Vcb, FileInfo) ||
        !(fcb = FileInfo->Fcb) || fcb->FcbCleanup ||
        fcb->CachedOpenHandleCount ||
        FlagOn(fcb->FcbState, UDF_FCB_DIRECTORY | UDF_FCB_EMBEDDED_DATA) ||
        fcb->Header.FileSize.QuadPart <= 0 ||
        fcb->Header.ValidDataLength.QuadPart < fcb->Header.FileSize.QuadPart ||
        FileInfo->Dloc->DataLoc.Offset ||
        ((((PFILE_ENTRY)FileInfo->Dloc->FileEntry)->icbTag.flags &
          ICB_FLAG_ALLOC_MASK) == ICB_FLAG_AD_IN_ICB))
        return FALSE;
    FileInfo->Dloc->FE_Flags |= UDF_FE_FLAG_CACHED_DATA_PENDING;
    UDF_METADATA_COUNT(Vcb,51);
    return TRUE;
}

/*************************************************************************
*
* Function: UDFCommonCleanup()
*
* Description:
*   The actual work is performed here. This routine may be invoked in one'
*   of the two possible contexts:
*   (a) in the context of a system worker thread
*   (b) in the context of the original caller
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: Does not matter!
*
*************************************************************************/
NTSTATUS
UDFCommonCleanup(
    PIRP_CONTEXT IrpContext,
    PIRP Irp)
{
    IO_STATUS_BLOCK         IoStatus;
    NTSTATUS                RC = STATUS_SUCCESS;
    NTSTATUS                UnlinkError = STATUS_SUCCESS;
    PFILE_OBJECT            FileObject = NULL;
    PFCB                    Fcb = NULL;
    PCCB                    Ccb = NULL;
    PVCB                    Vcb = NULL;
    ULONG                   lc = 0;
    BOOLEAN                 AcquiredVcb = FALSE;
    BOOLEAN                 AcquiredFCB = FALSE;
    BOOLEAN                 AcquiredParentFCB = FALSE;
    BOOLEAN SendUnlockNotification = FALSE;
    BOOLEAN                 ChangeTime = FALSE;
    BOOLEAN                 ForcedCleanUp = FALSE;

    PUDF_FILE_INFO          NextFileInfo = NULL;
    TYPE_OF_OPEN            TypeOfOpen;

    ASSERT_IRP_CONTEXT(IrpContext);
    ASSERT_IRP(Irp);

    // If we were called with our file system device object instead of a
    // volume device object, just complete this request with STATUS_SUCCESS.

    if (IrpContext->Vcb == NULL) {

        UDFCompleteRequest(IrpContext, Irp, STATUS_SUCCESS);
        return STATUS_SUCCESS;
    }

    // Get the file object out of the Irp and decode the type of open.

    FileObject = IoGetCurrentIrpStackLocation(Irp)->FileObject;

    TypeOfOpen = UDFDecodeFileObject(FileObject, &Fcb, &Ccb);

    // No work here for either an UnopenedFile object or a StreamFileObject.

    if (TypeOfOpen <= StreamFileOpen) {

        UDFCompleteRequest(IrpContext, Irp, STATUS_SUCCESS);

        return STATUS_SUCCESS;
    }

    //  Keep a local pointer to the Vcb.
    Vcb = Fcb->Vcb;

    ASSERT_CCB(Ccb);
    ASSERT_FCB(Fcb);
    ASSERT_VCB(Vcb);

    _SEH2_TRY {

        // Steps we shall take at this point are:
        // (a) Acquire the file (FCB) exclusively
        // (b) Flush file data to disk
        // (c) Talk to the FSRTL package (if we use it) about pending oplocks.
        // (d) Notify the FSRTL package for use with pending notification IRPs
        // (e) Unlock byte-range locks (if any were acquired by process)
        // (f) Update time stamp values (e.g. fast-IO had been performed)
        // (g) Inform the Cache Manager to uninitialize Cache Maps ...
        // and other similar stuff.
        //  BrutePoint();

        if (TypeOfOpen == UserVolumeOpen) {

            // For a force dismount, physically disconnect this Vcb from the device so 
            // a new mount can occur.  Vcb deletion cannot happen at this time since 
            // there is a reference on it associated with this very request,  but we'll 
            // call check for dismount again later after we process this close.

            if (FlagOn(Ccb->Flags, CCB_FLAG_DISMOUNT_ON_CLOSE)) {
        
                UDFAcquireUdfData(IrpContext);
                UDFCheckForDismount(IrpContext, Vcb, TRUE);
                UDFReleaseUdfData(IrpContext);

            // If this handle actually wrote something, flush the device buffers,
            // and then set the verify bit now just to be safe (in case there is no
            // dismount).
        
            } else if (FlagOn(FileObject->Flags, FO_FILE_MODIFIED)) {
        
                UDFHijackIrpAndFlushDevice(IrpContext, Irp, Vcb->TargetDeviceObject);
                UDFUpdateMediaChangeCount(Vcb, 0);
                UDFMarkDevForVerifyIfVcbMounted(Vcb);
            }

            //  If the volume is locked by this file object then release
            //  the volume and send notification.

            if (FlagOn(Vcb->VcbState, VCB_STATE_LOCKED) &&
                FileObject == Vcb->VolumeLockFileObject) {

                UDFAutoUnlock(Vcb);
                SendUnlockNotification = TRUE;
            }

            UDFLockVcb(IrpContext, Vcb);
            UDFDecrementCleanupCounts(IrpContext, Fcb);
            UDFUnlockVcb(IrpContext, Vcb);

            if (FileObject->Flags & FO_CACHE_SUPPORTED) {
                // we've cached close
                InterlockedDecrement((PLONG)&Fcb->CachedOpenHandleCount);
            }

            MmPrint(("    CcUninitializeCacheMap()\n"));
            CcUninitializeCacheMap(FileObject, NULL, NULL);

            //  We must clean up the share access at this time, since we may not
            //  get a Close call for awhile if the file was mapped through this
            //  File Object.
            IoRemoveShareAccess( FileObject, &Fcb->ShareAccess);

            try_return(RC = STATUS_SUCCESS);
        }
        else if ((TypeOfOpen == UserFileOpen) || (TypeOfOpen == UserDirectoryOpen)) {

            UDFAcquireVcbExclusive(IrpContext, Vcb, FALSE);
            AcquiredVcb = TRUE;
        }

        // Acquire current object only
        // Parent is acquired later only for delete operations (Child → Parent order)
        UDF_CHECK_PAGING_IO_RESOURCE(Fcb);
        UDFAcquireFcbExclusive(IrpContext, Fcb, FALSE);
        AcquiredFCB = TRUE;

        // Decrement the cleanup counts in the Vcb and Fcb.
        // Also decrement LCB reference count.

        UDFLockVcb(IrpContext, Vcb);
        UDFDecrementCleanupCounts(IrpContext, Fcb);
        if (Ccb->Lcb) {
            ASSERT(Ccb->Lcb->Reference > 0);
            Ccb->Lcb->Reference--;
        }
        UDFUnlockVcb(IrpContext, Vcb);

        if (FileObject->Flags & FO_CACHE_SUPPORTED) {
            // we've cached close
            InterlockedDecrement((PLONG)&Fcb->CachedOpenHandleCount);
        }
        // No ASSERT on FcbCleanup vs FcbReference here - FcbCleanup
        // can be temporarily bumped by try-lock reordering in create.cpp

        // check if Ccb being cleaned up has DeleteOnClose flag set
        if (Ccb->Flags & UDF_CCB_DELETE_ON_CLOSE) {
            AdPrint(("    DeleteOnClose\n"));
            // Ok, now we'll become 'delete on close'...
            ASSERT(!(Fcb->FcbState & UDF_FCB_ROOT_DIRECTORY));
            Fcb->FcbState |= UDF_FCB_DELETE_ON_CLOSE;
            FileObject->DeletePending = TRUE;
            //  Report this to the dir notify package for a directory.
            if (Fcb->FcbState & UDF_FCB_DIRECTORY) {
                FsRtlNotifyFullChangeDirectory( Vcb->NotifySync, &(Vcb->NextNotifyIRP),
                                                (PVOID)Ccb, NULL, FALSE, FALSE,
                                                0, NULL, NULL, NULL );
            }
        }

        if (!(Fcb->FcbState & UDF_FCB_DIRECTORY)) {

            //  Unlock all outstanding file locks.
            if (Fcb->FileLock != NULL) {

                FsRtlFastUnlockAll(Fcb->FileLock,
                                   FileObject,
                                   IoGetRequestorProcess(Irp),
                                   NULL);
            }
        }
        // get Link count
        lc = UDFGetFileNameLinkCount(Fcb->FileInfo);

        NextFileInfo = Fcb->FileInfo;

        // Attempt delete if this is the last cleanup and DELETE_ON_CLOSE is set
        if ((Fcb->FcbState & UDF_FCB_DELETE_ON_CLOSE) &&
           !(Fcb->FcbCleanup)) {

            BOOLEAN DeleteAttempted = FALSE;
            BOOLEAN HadFreeDeferred = (NextFileInfo->Dloc &&
                (NextFileInfo->Dloc->FE_Flags & UDF_FE_FLAG_FREE_DEFERRED));

            // This can be useful for Streams, those were brutally deleted
            // (together with parent object)
            ASSERT(!(Fcb->FcbState & UDF_FCB_ROOT_DIRECTORY));
            FileObject->DeletePending = TRUE;

            // Check if directory is non-empty — if so, discard delete
            if ((Fcb->FcbState & UDF_FCB_DIRECTORY) &&
                !UDFIsDirEmpty__(NextFileInfo)) {

                Fcb->FcbState &= ~UDF_FCB_DELETE_ON_CLOSE;

            } else {

                DeleteAttempted = TRUE;

                // Acquire parent for delete operation (after current — child first order)
                if (Fcb->FileInfo->ParentFile) {
                    UDF_CHECK_PAGING_IO_RESOURCE(Fcb->ParentFcb);
                    UDFAcquireFcbExclusive(IrpContext, Fcb->ParentFcb, FALSE);
                    AcquiredParentFCB = TRUE;
                }

                // Note: do NOT set file sizes to zero here before unlink.
                // If unlink fails (STATUS_CANNOT_DELETE), the file stays visible
                // with FSize=0 — other threads see truncated data.

                // Flush file. It is required by UDFUnlinkFile__()
                RC = UDFFlushFile__(IrpContext, Vcb, NextFileInfo, 0);
                if (!NT_SUCCESS(RC)) {
                    /* Unlink rejects dirty metadata. Do not turn that refusal
                     * into an in-memory deletion or deferred block release. */
                    UnlinkError = RC;
                    DeleteAttempted = FALSE;
                    Fcb->FcbState &= ~(UDF_FCB_DELETE_ON_CLOSE |
                                       UDF_FCB_DELETE_PARENT);
                    FileObject->DeletePending = FALSE;
                    goto delete_attempt_done;
                }

                if ((lc <= 1) &&
                   !UDFIsSDirDeleted(Fcb->FileInfo->Dloc->SDirInfo)) {
                    RC = UDFMarkStreamsForDeletion(IrpContext, Vcb, Fcb, TRUE);
                    if (!NT_SUCCESS(RC)) {
                        /* The streams must remain attached if preparing their
                         * deletion failed.  Do not unlink the last base FID
                         * or schedule its allocation for release. */
                        UnlinkError = RC;
                        DeleteAttempted = FALSE;
                        Fcb->FcbState &= ~(UDF_FCB_DELETE_ON_CLOSE |
                                           UDF_FCB_DELETE_PARENT);
                        FileObject->DeletePending = FALSE;
                        goto delete_attempt_done;
                    }
                }

                // Defer block freeing to teardown when this is the last link.
                // Blocks stay allocated until FcbReference drops to 0,
                // preventing block reuse while FCB is still in the table.
                if (lc <= 1 && NextFileInfo->Dloc) {
                    NextFileInfo->Dloc->FE_Flags |= UDF_FE_FLAG_FREE_DEFERRED;
                }

                // Try to unlink
                RC = UDFUnlinkFile__(IrpContext, Vcb, NextFileInfo, TRUE);

                if (!NT_SUCCESS(RC) && RC != STATUS_CANNOT_DELETE) {
                    /* An I/O failure did not commit the unlink. Teardown must
                     * retain the FE allocation and must not report removal. */
                    UnlinkError = RC;
                    DeleteAttempted = FALSE;
                    Fcb->FcbState &= ~(UDF_FCB_DELETE_ON_CLOSE |
                                       UDF_FCB_DELETE_PARENT);
                    FileObject->DeletePending = FALSE;
                    if (!HadFreeDeferred && NextFileInfo->Dloc)
                        NextFileInfo->Dloc->FE_Flags &= ~UDF_FE_FLAG_FREE_DEFERRED;
                } else if (RC == STATUS_CANNOT_DELETE) {

                    if (NextFileInfo->Dloc &&
                       NextFileInfo->Dloc->SDirInfo &&
                       NextFileInfo->Dloc->SDirInfo->Fcb) {

                        // Can't delete file with open streams — pretend deleted.
                        // Streams will trigger parent deletion on their cleanup.
                        BrutePoint();
                        if (!UDFIsSDirDeleted(NextFileInfo->Dloc->SDirInfo)) {
                            UDFPretendFileDeleted__(Vcb, Fcb->FileInfo);
                        }

                    } else {

                        /* No namespace commit occurred. Keep the FID, FE,
                         * allocation and prefix available for a later open.
                         * In particular, a reference refusal is not a
                         * successful delete of the last ordinary link. */
                        UnlinkError = RC;
                        DeleteAttempted = FALSE;
                        Fcb->FcbState &= ~(UDF_FCB_DELETE_ON_CLOSE |
                                           UDF_FCB_DELETE_PARENT);
                        FileObject->DeletePending = FALSE;
                        if (!HadFreeDeferred && NextFileInfo->Dloc)
                            NextFileInfo->Dloc->FE_Flags &= ~UDF_FE_FLAG_FREE_DEFERRED;
                    }

                } else {

                    // Unlink completed — mark as deleted
                    ASSERT(!(Fcb->FcbState & UDF_FCB_ROOT_DIRECTORY));
                    ForcedCleanUp = TRUE;
                    if (NT_SUCCESS(RC))
                        Fcb->FcbState &= ~UDF_FCB_DELETE_ON_CLOSE;
                    Fcb->FcbState |= UDF_FCB_DELETED;
                    // Remove LCB from parent's splay trees immediately.
                    // Parent is held exclusive (AcquiredParentFCB).
                    if (Ccb->Lcb && Ccb->Lcb->ParentFcb) {
                        UdfRemoveNameLinks(Ccb->Lcb->ParentFcb, Ccb->Lcb);
                    }
                    // Note: do NOT call CcSetFileSizes(0) here.
                    // CcUninitializeCacheMap with TruncateSize=0 below (ForcedCleanUp path)
                    // already purges the cache. Setting Fcb->Header.FileSize=0 here would
                    // leave a stale FCB with FSize=0 in the prefix table — if the FCB is
                    // reused (e.g., by rename), the renamed file appears as 0-byte.
                    RC = STATUS_SUCCESS;
                }
            }

delete_attempt_done:
            if (DeleteAttempted) {
                /* A failed unlink must not schedule deletion of its parent. */
                if ((Fcb->FcbState & UDF_FCB_DELETE_PARENT) && Fcb->ParentFcb) {
                    ASSERT(!(Fcb->ParentFcb->FcbState & UDF_FCB_ROOT_DIRECTORY));
                    Fcb->ParentFcb->FcbState |= UDF_FCB_DELETE_ON_CLOSE;
                }
                // Prevent SetEOF operations on completely deleted data streams
                if (lc < 1) {
                    Fcb->NtReqFCBFlags |= UDF_NTREQ_FCB_DELETED;
                }
                // Report that we have removed an entry.
                if (UDFIsAStream(NextFileInfo)) {
                    UDFNotifyReportChange( IrpContext, Vcb, NextFileInfo->Fcb,
                                           FILE_NOTIFY_CHANGE_STREAM_NAME,
                                           FILE_ACTION_REMOVED_STREAM,
                                           Ccb->Lcb, FileObject);
                } else {
                    UDFNotifyReportChange( IrpContext, Vcb, NextFileInfo->Fcb,
                                           UDFIsADirectory(NextFileInfo) ? FILE_NOTIFY_CHANGE_DIR_NAME : FILE_NOTIFY_CHANGE_FILE_NAME,
                                           FILE_ACTION_REMOVED,
                                           Ccb->Lcb, FileObject);
                }
            } else {
                // Delete discarded (e.g. non-empty directory) — notify modification
                UDFNotifyReportChange( IrpContext, Vcb, NextFileInfo->Fcb,
                                         ((Ccb->Flags & UDF_CCB_ACCESS_TIME_SET) ? FILE_NOTIFY_CHANGE_LAST_ACCESS : 0) |
                                         ((Ccb->Flags & UDF_CCB_WRITE_TIME_SET) ? (FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_LAST_WRITE) : 0) |
                                         0,
                                         UDFIsAStream(NextFileInfo) ? FILE_ACTION_MODIFIED_STREAM : FILE_ACTION_MODIFIED,
                                         Ccb->Lcb, FileObject);
            }

        } else if (Fcb->FcbState & UDF_FCB_DELETE_ON_CLOSE) {

            // DELETE_ON_CLOSE is set but FcbCleanup > 0 (other handles still open)
            UDFNotifyReportChange( IrpContext, Vcb, NextFileInfo->Fcb,
                                     ((Ccb->Flags & UDF_CCB_ACCESS_TIME_SET) ? FILE_NOTIFY_CHANGE_LAST_ACCESS : 0) |
                                     ((Ccb->Flags & UDF_CCB_WRITE_TIME_SET) ? (FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_LAST_WRITE) : 0) |
                                     0,
                                     UDFIsAStream(NextFileInfo) ? FILE_ACTION_MODIFIED_STREAM : FILE_ACTION_MODIFIED,
                                     Ccb->Lcb, FileObject);
        }

        if (Fcb->FcbState & UDF_FCB_DIRECTORY) {
            //  Report to the dir notify package for a directory.
            FsRtlNotifyCleanup( Vcb->NotifySync, &(Vcb->NextNotifyIRP), (PVOID)Ccb );
        }

        // we can't purge Cache when more than one link exists
        if (lc > 1) {
            ForcedCleanUp = FALSE;
        }

        if (FileObject->Flags & FO_CACHE_SUPPORTED &&
             Fcb->FcbNonpaged->SegmentObject.DataSectionObject) {
            BOOLEAN LastNonCached = (!Fcb->CachedOpenHandleCount &&
                                      Fcb->FcbCleanup);
            // If this was the last cached open, and there are open
            // non-cached handles, attempt a flush and purge operation
            // to avoid cache coherency overhead from these non-cached
            // handles later.  We ignore any I/O errors from the flush.
            // We shall not flush deleted files
            RC = STATUS_SUCCESS;
            // On an ordinary final cleanup, avoid flushing clean cache state.
            // Preserve writeback for modified or pending data, VDL gaps, and
            // cached/non-cached coherence; the cache map is still uninitialized below.
            if (LastNonCached ||
                (!Fcb->FcbCleanup &&
                 !ForcedCleanUp &&
                 !UDFCanDeferNewFileDataCleanup(IrpContext, Vcb, NextFileInfo) &&
                 ((FileObject->Flags & FO_FILE_MODIFIED) ||
                  (Fcb->NtReqFCBFlags & UDF_NTREQ_FCB_MODIFIED) ||
                  (Fcb->Header.ValidDataLength.QuadPart <
                   Fcb->Header.FileSize.QuadPart) ||
                  (NextFileInfo && NextFileInfo->Dloc &&
                   (NextFileInfo->Dloc->FE_Flags &
                    UDF_FE_FLAG_CACHED_DATA_PENDING)))) ) {

                LONGLONG OldFileSize, NewFileSize;

                if ((OldFileSize = Fcb->Header.ValidDataLength.QuadPart) <
                    (NewFileSize = Fcb->Header.FileSize.QuadPart)) {
                    UDFZeroData(Vcb,
                                FileObject,
                                OldFileSize,
                                NewFileSize - OldFileSize,
                                TRUE);

                    Fcb->Header.ValidDataLength.QuadPart = NewFileSize;
                }

                MmPrint(("    CcFlushCache()\n"));
                CcFlushCache(&Fcb->FcbNonpaged->SegmentObject, NULL, 0, &IoStatus);
                if (!NT_SUCCESS(IoStatus.Status)) {
                    MmPrint(("    CcFlushCache() error: %x\n", IoStatus.Status));
                    RC = IoStatus.Status;
                } else if (NextFileInfo && NextFileInfo->Dloc) {
                    NextFileInfo->Dloc->FE_Flags &= ~UDF_FE_FLAG_CACHED_DATA_PENDING;
                }
            }
            // If file is deleted or it is last cached open, but there are
            // some non-cached handles we should purge cache section
            if (ForcedCleanUp || LastNonCached) {
                if (Fcb->FcbNonpaged->SegmentObject.DataSectionObject) {
                    MmPrint(("    CcPurgeCacheSection()\n"));
                    CcPurgeCacheSection(&Fcb->FcbNonpaged->SegmentObject, NULL, 0, FALSE);
                }
/*                MmPrint(("    CcPurgeCacheSection()\n"));
                CcPurgeCacheSection(&Fcb->SectionObject, NULL, 0, FALSE);*/
            }
            // we needn't Flush here. It will be done in UDFCloseFile__
        }

        // Update FileTimes & Attrs
        if (!(Vcb->VcbState & VCB_STATE_VOLUME_READ_ONLY) &&
           !(Fcb->FcbState & (UDF_FCB_DELETE_ON_CLOSE |
                              UDF_FCB_DELETED /*|
                              UDF_FCB_DIRECTORY |
                              UDF_FCB_READ_ONLY*/)) &&
           !UDFIsAStreamDir(NextFileInfo)) {
            LONGLONG NtTime;
            LONGLONG ASize;
            KeQuerySystemTime((PLARGE_INTEGER)&NtTime);
            // Check if we should set ARCHIVE bit & LastWriteTime
            if (FileObject->Flags & FO_FILE_MODIFIED) {
                ULONG Attr;
                PDIR_INDEX_ITEM DirNdx;
                DirNdx = UDFDirIndex(UDFGetDirIndexByFileInfo(NextFileInfo), NextFileInfo->Index);
                ASSERT(DirNdx);
                // Archive bit
                if (!(Ccb->Flags & UDF_CCB_ATTRIBUTES_SET) &&
                    (Vcb->CompatFlags & UDF_VCB_IC_UPDATE_ARCH_BIT)) {
                    Attr = UDFAttributesToNT(DirNdx, NextFileInfo->Dloc->FileEntry);
                    if (!(Attr & FILE_ATTRIBUTE_ARCHIVE))
                        UDFAttributesToUDF(DirNdx, NextFileInfo->Dloc->FileEntry, Attr | FILE_ATTRIBUTE_ARCHIVE);
                }
                // WriteTime
                if (!(Ccb->Flags & UDF_CCB_WRITE_TIME_SET) &&
                    (Vcb->CompatFlags & UDF_VCB_IC_UPDATE_MODIFY_TIME)) {
                    UDFSetFileXTime(NextFileInfo, NULL, &NtTime, NULL, &NtTime);
                    Fcb->LastWriteTime.QuadPart =
                    Fcb->LastAccessTime.QuadPart = NtTime;
                    ChangeTime = TRUE;
                }
            }
            if (!(Fcb->FcbState & UDF_FCB_DIRECTORY)) {
                // Update sizes in DirIndex
                if (!Fcb->FcbCleanup) {
                    ASize = UDFGetFileAllocationSize(Vcb, NextFileInfo);
//                        Fcb->CommonFCBHeader.AllocationSize.QuadPart;
                    UDFSetFileSizeInDirNdx(Vcb, NextFileInfo, &ASize);

                } else if (FileObject->Flags & FO_FILE_SIZE_CHANGED) {

                    ASize = //UDFGetFileAllocationSize(Vcb, NextFileInfo);
                    Fcb->Header.AllocationSize.QuadPart;
                    UDFSetFileSizeInDirNdx(Vcb, NextFileInfo, &ASize);

                    if (UDFIsAStream(Fcb->FileInfo)) {

                        UDFNotifyReportChange(IrpContext, Vcb,
                            Fcb,
                            FILE_NOTIFY_CHANGE_STREAM_SIZE,
                            FILE_ACTION_MODIFIED_STREAM,
                            Ccb->Lcb, FileObject);
                    }
                    else {

                        UDFNotifyReportChange(IrpContext, Vcb,
                            Fcb,
                            FILE_NOTIFY_CHANGE_SIZE,
                            FILE_ACTION_MODIFIED,
                            Ccb->Lcb, FileObject);
                    }

                }
            }
            // AccessTime
            if ((FileObject->Flags & FO_FILE_FAST_IO_READ) &&
               !(Ccb->Flags & UDF_CCB_ACCESS_TIME_SET) &&
                (Vcb->CompatFlags & UDF_VCB_IC_UPDATE_ACCESS_TIME)) {
                UDFSetFileXTime(NextFileInfo, NULL, &NtTime, NULL, NULL);
                Fcb->LastAccessTime.QuadPart = NtTime;
//                ChangeTime = TRUE;
            }
            // ChangeTime (AttrTime)
            if (!(Ccb->Flags & UDF_CCB_MODIFY_TIME_SET) &&
                (Vcb->CompatFlags & UDF_VCB_IC_UPDATE_ATTR_TIME) &&
                (ChangeTime || (Ccb->Flags & (UDF_CCB_ATTRIBUTES_SET |
                                                 UDF_CCB_CREATE_TIME_SET |
                                                 UDF_CCB_ACCESS_TIME_SET |
                                                 UDF_CCB_WRITE_TIME_SET))) ) {
                UDFSetFileXTime(NextFileInfo, NULL, NULL, &NtTime, NULL);
                Fcb->ChangeTime.QuadPart = NtTime;
            }
        }

        // Keep structural updates eager. Only an owned unpublished append or
        // a proved attribute-only directory generation may remain resident.
        // Final teardown flushes before removing the canonical FCB/LCB;
        // explicit flush never takes either cleanup-only exception.
        if (!Fcb->FcbCleanup &&
            !ForcedCleanUp &&
            !(Fcb->FcbState & UDF_FCB_DELETED) &&
            !(Vcb->VcbState & VCB_STATE_VOLUME_READ_ONLY) &&
            NextFileInfo &&
            // Regular files are always flushed here; only directories may
            // keep a pending append or attribute-only update in memory.
            (!(Fcb->FcbState & UDF_FCB_DIRECTORY) ||
             (!UDFCanDeferDirectoryAppendCleanup(Vcb, NextFileInfo, FileObject) &&
              !UDFCanDeferDirectoryAttributesCleanup(IrpContext, Vcb,
                                                    NextFileInfo, FileObject) &&
              !UDFDeferNewFileMetadata(IrpContext, Vcb, NextFileInfo)))) {
            NTSTATUS FlushStatus = UDFFlushFile__(IrpContext, Vcb,
                                                  NextFileInfo, 0);
            if (NT_SUCCESS(UnlinkError) && !NT_SUCCESS(FlushStatus))
                UnlinkError = FlushStatus;
        }

        if (!(Fcb->FcbState & UDF_FCB_DIRECTORY) &&
            ForcedCleanUp) {
            // flush system cache
            MmPrint(("    CcUninitializeCacheMap()\n"));
            CcUninitializeCacheMap(FileObject, &(UdfData.UDFLargeZero), NULL);
            if ((Fcb->FcbState & UDF_FCB_DELETED) && NextFileInfo && NextFileInfo->Dloc)
                NextFileInfo->Dloc->FE_Flags &= ~UDF_FE_FLAG_CACHED_DATA_PENDING;
        } else {
            MmPrint(("    CcUninitializeCacheMap()\n"));
            CcUninitializeCacheMap(FileObject, NULL, NULL);
        }

        if (
            !Fcb->FcbCleanup &&
            !(Fcb->FcbState & (UDF_FCB_DELETE_ON_CLOSE | UDF_FCB_DELETED)) &&
            !UDFIsAStream(NextFileInfo) &&
            !UDFIsAStreamDir(NextFileInfo)
        )
            Fcb->FcbState |= UDF_FCB_DELAY_CLOSE;

        // release resources now.
        UDFReleaseFcb(IrpContext, Fcb);
        AcquiredFCB = FALSE;

        if (AcquiredParentFCB && Fcb->FileInfo->ParentFile) {
            UDFReleaseFcb(IrpContext, Fcb->FileInfo->ParentFile->Fcb);
            AcquiredParentFCB = FALSE;
        }

        // Close the target file's FileInfo - this decrements FileInfo->RefCount
        // Parent FileInfo references are now handled by LCB mechanism in UDFTeardownStructures
        ASSERT(AcquiredVcb);
        if (NextFileInfo) {
            PUDF_FILE_INFO DirInfo = NextFileInfo->ParentFile;
            NTSTATUS CloseStatus = UDFCloseFile__(IrpContext, Vcb,
                                                  NextFileInfo);
            if (NT_SUCCESS(UnlinkError) && !NT_SUCCESS(CloseStatus))
                UnlinkError = CloseStatus;
            if (DirInfo && 
                DirInfo->Dloc && 
                DirInfo->Fcb &&
                !(Vcb->VcbState & VCB_STATE_VOLUME_READ_ONLY) &&
                (
                    (DirInfo->Dloc->FE_Flags & UDF_FE_FLAG_FE_MODIFIED) ||
                    DirInfo->Dloc->AllocLoc.Modified ||
                    DirInfo->Dloc->DataLoc.Modified ||
                    DirInfo->Dloc->FELoc.Modified
                )
                ) {
                UDF_CHECK_PAGING_IO_RESOURCE(DirInfo->Fcb);
                UDFAcquireFcbExclusive(IrpContext, DirInfo->Fcb, FALSE);
                /* A regular file's new FID may only be in its parent's
                 * append buffer. Flush the parent so the name reaches the
                 * disk together with the file entry flushed above. Only a
                 * directory's own cleanup may keep its parent deferred. */
                BOOLEAN DeferAppend =
                    FlagOn(Fcb->FcbState, UDF_FCB_DIRECTORY) &&
                    FlagOn(FileObject->Flags, FO_CACHE_SUPPORTED) &&
                    UDFCanDeferDirectoryAppendCleanup(Vcb, DirInfo, FileObject);
                if (!DeferAppend) {
                    NTSTATUS FlushStatus = UDFFlushFile__(IrpContext, Vcb,
                                                           DirInfo,
                                                           UDF_FLUSH_FLAGS_LITE);
                    if (NT_SUCCESS(UnlinkError) && !NT_SUCCESS(FlushStatus))
                        UnlinkError = FlushStatus;
                }
                UDFReleaseFcb(IrpContext, DirInfo->Fcb);
            }
        }

        if (!NT_SUCCESS(UnlinkError))
            Fcb->FcbState &= ~UDF_FCB_DELAY_CLOSE;

        Ccb->Flags |= UDF_CCB_CLEANED;

        //  We must clean up the share access at this time, since we may not
        //  get a Close call for awhile if the file was mapped through this
        //  File Object.
        IoRemoveShareAccess(FileObject, &Fcb->ShareAccess);

        Fcb->Header.IsFastIoPossible = UDFIsFastIoPossible(Fcb);

        FileObject->Flags |= FO_CLEANUP_COMPLETE;

try_exit: NOTHING;

    } _SEH2_FINALLY {

        if (AcquiredFCB) {
            UDFReleaseFcb(IrpContext, Fcb);
        }

        if (AcquiredParentFCB && Fcb->FileInfo->ParentFile) {
            UDFReleaseFcb(IrpContext, Fcb->FileInfo->ParentFile->Fcb);
        }

        if (AcquiredVcb) {
            UDFReleaseVcb(IrpContext, Vcb);
            AcquiredVcb = FALSE;
        }

        if (SendUnlockNotification) {

            FsRtlNotifyVolumeEvent(FileObject, FSRTL_VOLUME_UNLOCK);
        }

        if (!_SEH2_AbnormalTermination()) {
                if (!NT_SUCCESS(UnlinkError))
                    RC = UnlinkError;
                UDFCompleteRequest(IrpContext, Irp, RC);
        }

    } _SEH2_END; // end of "__finally" processing
    return(RC);
} // end UDFCommonCleanup()

VOID
UDFAutoUnlock (
    IN PVCB Vcb
    )
{
    KIRQL SavedIrql;

    //  Unlock the volume.
 
    IoAcquireVpbSpinLock( &SavedIrql );

    ClearFlag(Vcb->Vpb->Flags, VPB_LOCKED | VPB_DIRECT_WRITES_ALLOWED);
    ClearFlag(Vcb->VcbState, VCB_STATE_LOCKED);
    Vcb->VolumeLockFileObject = NULL;

    IoReleaseVpbSpinLock( SavedIrql );
}
