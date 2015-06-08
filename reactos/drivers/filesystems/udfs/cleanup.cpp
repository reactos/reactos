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


/*************************************************************************
*
* Function: UDFCleanup()
*
* Description:
*   The I/O Manager will invoke this routine to handle a cleanup
*   request
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL (invocation at higher IRQL will cause execution
*   to be deferred to a worker thread context)
*
* Return Value: STATUS_SUCCESS
*
*************************************************************************/
NTSTATUS
NTAPI
UDFCleanup(
    PDEVICE_OBJECT  DeviceObject,  // the logical volume device object
    PIRP            Irp            // I/O Request Packet
    )
{
    NTSTATUS                RC = STATUS_SUCCESS;
    PtrUDFIrpContext        PtrIrpContext = NULL;
    BOOLEAN                 AreWeTopLevel = FALSE;

    TmPrint(("UDFCleanup\n"));

    FsRtlEnterFileSystem();
    ASSERT(DeviceObject);
    ASSERT(Irp);

    //  If we were called with our file system device object instead of a
    //  volume device object, just complete this request with STATUS_SUCCESS
    if (UDFIsFSDevObj(DeviceObject)) {
        // this is a cleanup of the FSD itself
        Irp->IoStatus.Status = RC;
        Irp->IoStatus.Information = 0;

        if(UDFGlobalData.AutoFormatCount == IoGetCurrentIrpStackLocation(Irp)->FileObject) {
            KdPrint(("Deregister Autoformat\n"));
            UDFGlobalData.AutoFormatCount = NULL;
        }

        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        FsRtlExitFileSystem();
        return(RC);
    }

    // set the top level context
    AreWeTopLevel = UDFIsIrpTopLevel(Irp);

    _SEH2_TRY {

        // get an IRP context structure and issue the request
        PtrIrpContext = UDFAllocateIrpContext(Irp, DeviceObject);
        if(PtrIrpContext) {
            RC = UDFCommonCleanup(PtrIrpContext, Irp);
        } else {
            RC = STATUS_INSUFFICIENT_RESOURCES;
            Irp->IoStatus.Status = RC;
            Irp->IoStatus.Information = 0;
            // complete the IRP
            IoCompleteRequest(Irp, IO_DISK_INCREMENT);
        }

    } _SEH2_EXCEPT(UDFExceptionFilter(PtrIrpContext, _SEH2_GetExceptionInformation())) {

        RC = UDFExceptionHandler(PtrIrpContext, Irp);

        UDFLogEvent(UDF_ERROR_INTERNAL_ERROR, RC);
    } _SEH2_END;

    if (AreWeTopLevel) {
        IoSetTopLevelIrp(NULL);
    }

    FsRtlExitFileSystem();

    return(RC);
} // end UDFCleanup()

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
    PtrUDFIrpContext PtrIrpContext,
    PIRP             Irp)
{
    IO_STATUS_BLOCK         IoStatus;
    NTSTATUS                RC = STATUS_SUCCESS;
    NTSTATUS                RC2;
    PIO_STACK_LOCATION      IrpSp = NULL;
    PFILE_OBJECT            FileObject = NULL;
    PtrUDFFCB               Fcb = NULL;
    PtrUDFCCB               Ccb = NULL;
    PVCB                    Vcb = NULL;
    PtrUDFNTRequiredFCB     NtReqFcb = NULL;
    ULONG                   lc = 0;
    BOOLEAN                 AcquiredVcb = FALSE;
    BOOLEAN                 AcquiredFCB = FALSE;
    BOOLEAN                 AcquiredParentFCB = FALSE;

//    BOOLEAN                 CompleteIrp = TRUE;
//    BOOLEAN                 PostRequest = FALSE;
    BOOLEAN                 ChangeTime = FALSE;
#ifdef UDF_DBG
    BOOLEAN                 CanWait = FALSE;
#endif // UDF_DBG
    BOOLEAN                 ForcedCleanUp = FALSE;

    PUDF_FILE_INFO          NextFileInfo = NULL;
#ifdef UDF_DBG
    UNICODE_STRING          CurName;
    PDIR_INDEX_HDR          DirNdx;
#endif // UDF_DBG
//    PUDF_DATALOC_INFO       Dloc;

    TmPrint(("UDFCommonCleanup\n"));

//    BrutePoint();

    _SEH2_TRY {
        // First, get a pointer to the current I/O stack location
        IrpSp = IoGetCurrentIrpStackLocation(Irp);
        if(!IrpSp) try_return(RC = STATUS_INVALID_PARAMETER);

        FileObject = IrpSp->FileObject;

        // Get the FCB and CCB pointers
        Ccb = (PtrUDFCCB)(FileObject->FsContext2);
        ASSERT(Ccb);
        Fcb = Ccb->Fcb;
        ASSERT(Fcb);

        Vcb = (PVCB)(PtrIrpContext->TargetDeviceObject->DeviceExtension);
        ASSERT(Vcb);
        ASSERT(Vcb->NodeIdentifier.NodeType == UDF_NODE_TYPE_VCB);
//        Vcb->VCBFlags |= UDF_VCB_SKIP_EJECT_CHECK;
#ifdef UDF_DBG
        CanWait = (PtrIrpContext->IrpContextFlags & UDF_IRP_CONTEXT_CAN_BLOCK) ? TRUE : FALSE;
        AdPrint(("   %s\n", CanWait ? "Wt" : "nw"));
        ASSERT(CanWait);
#endif // UDF_DBG
        UDFAcquireResourceShared(&(Vcb->VCBResource), TRUE);
        AcquiredVcb = TRUE;
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
        NtReqFcb = Fcb->NTRequiredFCB;

        if (Fcb->NodeIdentifier.NodeType == UDF_NODE_TYPE_VCB) {
            AdPrint(("Cleaning up Volume\n"));
            AdPrint(("UDF: OpenHandleCount: %x\n",Fcb->OpenHandleCount));

            UDFInterlockedDecrement((PLONG)&(Fcb->OpenHandleCount));
            UDFInterlockedDecrement((PLONG)&(Vcb->VCBHandleCount));
            if(FileObject->Flags & FO_CACHE_SUPPORTED) {
                // we've cached close
                UDFInterlockedDecrement((PLONG)&(Fcb->CachedOpenHandleCount));
            }
            ASSERT(Fcb->OpenHandleCount <= (Fcb->ReferenceCount-1));

            //  If this handle had write access, and actually wrote something,
            //  flush the device buffers, and then set the verify bit now
            //  just to be safe (in case there is no dismount).
            if( FileObject->WriteAccess &&
               (FileObject->Flags & FO_FILE_MODIFIED)) {

                Vcb->Vpb->RealDevice->Flags |= DO_VERIFY_VOLUME;
            }
            // User may decide to close locked volume without call to unlock proc
            // So, handle this situation properly & unlock it now...
            if (FileObject == Vcb->VolumeLockFileObject) {
                Vcb->VolumeLockFileObject = NULL;
                Vcb->VolumeLockPID = -1;
                Vcb->VCBFlags &= ~UDF_VCB_FLAGS_VOLUME_LOCKED;
                Vcb->Vpb->Flags &= ~VPB_LOCKED;
                UDFNotifyVolumeEvent(FileObject, FSRTL_VOLUME_UNLOCK);
            }

            MmPrint(("    CcUninitializeCacheMap()\n"));
            CcUninitializeCacheMap(FileObject, NULL, NULL);
            // reset device
            if(!(Vcb->VCBFlags & UDF_VCB_FLAGS_VOLUME_MOUNTED) &&
                (Vcb->VCBFlags & UDF_VCB_FLAGS_OUR_DEVICE_DRIVER)) {
                // this call doesn't modify data buffer
                // it just requires its presence
                UDFResetDeviceDriver(Vcb, Vcb->TargetDeviceObject, TRUE);
            }
            //  We must clean up the share access at this time, since we may not
            //  get a Close call for awhile if the file was mapped through this
            //  File Object.
            IoRemoveShareAccess( FileObject, &(NtReqFcb->FCBShareAccess) );

            try_return(RC = STATUS_SUCCESS);
        }
//        BrutePoint();
#ifdef UDF_DBG
        DirNdx = UDFGetDirIndexByFileInfo(Fcb->FileInfo);
        if(DirNdx) {
            CurName.Buffer = UDFDirIndex(DirNdx, Fcb->FileInfo->Index)->FName.Buffer;
            if(CurName.Buffer) {
                AdPrint(("Cleaning up file: %ws %8.8x\n", CurName.Buffer, FileObject));
            } else {
                AdPrint(("Cleaning up file: ??? \n"));
            }
        }
#endif //UDF_DBG
        AdPrint(("UDF: OpenHandleCount: %x\n",Fcb->OpenHandleCount));
        // Acquire parent object
        if(Fcb->FileInfo->ParentFile) {
            UDF_CHECK_PAGING_IO_RESOURCE(Fcb->FileInfo->ParentFile->Fcb->NTRequiredFCB);
            UDFAcquireResourceExclusive(&(Fcb->FileInfo->ParentFile->Fcb->NTRequiredFCB->MainResource),TRUE);
        } else {
            UDFAcquireResourceShared(&(Vcb->VCBResource),TRUE);
        }
        AcquiredParentFCB = TRUE;
        // Acquire current object
        UDF_CHECK_PAGING_IO_RESOURCE(NtReqFcb);
        UDFAcquireResourceExclusive(&(NtReqFcb->MainResource),TRUE);
        AcquiredFCB = TRUE;
        // dereference object
        UDFInterlockedDecrement((PLONG)&(Fcb->OpenHandleCount));
        UDFInterlockedDecrement((PLONG)&(Vcb->VCBHandleCount));
        if(FileObject->Flags & FO_CACHE_SUPPORTED) {
            // we've cached close
            UDFInterlockedDecrement((PLONG)&(Fcb->CachedOpenHandleCount));
        }
        ASSERT(Fcb->OpenHandleCount <= (Fcb->ReferenceCount-1));
        // check if Ccb being cleaned up has DeleteOnClose flag set
#ifndef UDF_READ_ONLY_BUILD
        if(Ccb->CCBFlags & UDF_CCB_DELETE_ON_CLOSE) {
            AdPrint(("    DeleteOnClose\n"));
            // Ok, now we'll become 'delete on close'...
            ASSERT(!(Fcb->FCBFlags & UDF_FCB_ROOT_DIRECTORY));
            Fcb->FCBFlags |= UDF_FCB_DELETE_ON_CLOSE;
            FileObject->DeletePending = TRUE;
            //  Report this to the dir notify package for a directory.
            if(Fcb->FCBFlags & UDF_FCB_DIRECTORY) {
                FsRtlNotifyFullChangeDirectory( Vcb->NotifyIRPMutex, &(Vcb->NextNotifyIRP),
                                                (PVOID)Ccb, NULL, FALSE, FALSE,
                                                0, NULL, NULL, NULL );
            }
        }
#endif //UDF_READ_ONLY_BUILD

        if(!(Fcb->FCBFlags & UDF_FCB_DIRECTORY)) {
            //  Unlock all outstanding file locks.
            FsRtlFastUnlockAll(&(NtReqFcb->FileLock),
                               FileObject,
                               IoGetRequestorProcess(Irp),
                               NULL);
        }
        // get Link count
        lc = UDFGetFileLinkCount(Fcb->FileInfo);

#ifndef UDF_READ_ONLY_BUILD
        if( (Fcb->FCBFlags & UDF_FCB_DELETE_ON_CLOSE) &&
           !(Fcb->OpenHandleCount)) {
            // This can be useful for Streams, those were brutally deleted
            // (together with parent object)
            ASSERT(!(Fcb->FCBFlags & UDF_FCB_ROOT_DIRECTORY));
            FileObject->DeletePending = TRUE;

            // we should mark all streams of the file being deleted
            // for deletion too, if there are no more Links to
            // main data stream
            if((lc <= 1) &&
               !UDFIsSDirDeleted(Fcb->FileInfo->Dloc->SDirInfo)) {
                RC = UDFMarkStreamsForDeletion(Vcb, Fcb, TRUE); // Delete
            }
            // we can release these resources 'cause UDF_FCB_DELETE_ON_CLOSE
            // flag is already set & the file can't be opened
            UDF_CHECK_PAGING_IO_RESOURCE(NtReqFcb);
            UDFReleaseResource(&(NtReqFcb->MainResource));
            AcquiredFCB = FALSE;
            if(Fcb->FileInfo->ParentFile) {
                UDF_CHECK_PAGING_IO_RESOURCE(Fcb->ParentFcb->NTRequiredFCB);
                UDFReleaseResource(&(Fcb->ParentFcb->NTRequiredFCB->MainResource));
            } else {
                UDFReleaseResource(&(Vcb->VCBResource));
            }
            AcquiredParentFCB = FALSE;
            UDFReleaseResource(&(Vcb->VCBResource));
            AcquiredVcb = FALSE;

            // Make system to issue last Close request
            // for our Target ...
            UDFRemoveFromSystemDelayedQueue(Fcb);

#ifdef UDF_DELAYED_CLOSE
            // remove file from our DelayedClose queue
            UDFRemoveFromDelayedQueue(Fcb);
            ASSERT(!Fcb->IrpContextLite);
#endif //UDF_DELAYED_CLOSE

            UDFAcquireResourceShared(&(Vcb->VCBResource), TRUE);
            AcquiredVcb = TRUE;
            if(Fcb->FileInfo->ParentFile) {
                UDF_CHECK_PAGING_IO_RESOURCE(Fcb->ParentFcb->NTRequiredFCB);
                UDFAcquireResourceExclusive(&(Fcb->ParentFcb->NTRequiredFCB->MainResource),TRUE);
            } else {
                UDFAcquireResourceShared(&(Vcb->VCBResource),TRUE);
            }
            AcquiredParentFCB = TRUE;
            UDF_CHECK_PAGING_IO_RESOURCE(NtReqFcb);
            UDFAcquireResourceExclusive(&(NtReqFcb->MainResource),TRUE);
            AcquiredFCB = TRUE;

            // we should set file sizes to zero if there are no more
            // links to this file
            if(lc <= 1) {
                // Synchronize here with paging IO
                UDFAcquireResourceExclusive(&(NtReqFcb->PagingIoResource),TRUE);
                // set file size to zero (for system cache manager)
//                NtReqFcb->CommonFCBHeader.ValidDataLength.QuadPart =
                NtReqFcb->CommonFCBHeader.FileSize.QuadPart =
                NtReqFcb->CommonFCBHeader.ValidDataLength.QuadPart = 0;
                CcSetFileSizes(FileObject, (PCC_FILE_SIZES)&(NtReqFcb->CommonFCBHeader.AllocationSize));

                UDFReleaseResource(&(NtReqFcb->PagingIoResource));
            }
        }
#endif //UDF_READ_ONLY_BUILD

#ifdef UDF_DELAYED_CLOSE
        if ((Fcb->ReferenceCount == 1) &&
         /*(Fcb->NodeIdentifier.NodeType != UDF_NODE_TYPE_VCB) &&*/ // see above
            (!(Fcb->FCBFlags & UDF_FCB_DELETE_ON_CLOSE)) ) {
            Fcb->FCBFlags |= UDF_FCB_DELAY_CLOSE;
        }
#endif //UDF_DELAYED_CLOSE

        NextFileInfo = Fcb->FileInfo;

#ifndef UDF_READ_ONLY_BUILD
        // do we need to delete it now ?
        if( (Fcb->FCBFlags & UDF_FCB_DELETE_ON_CLOSE) &&
           !(Fcb->OpenHandleCount)) {

            // can we do it ?
            if(Fcb->FCBFlags & UDF_FCB_DIRECTORY) {
                ASSERT(!(Fcb->FCBFlags & UDF_FCB_ROOT_DIRECTORY));
                if(!UDFIsDirEmpty__(NextFileInfo)) {
                    // forget about it
                    Fcb->FCBFlags &= ~UDF_FCB_DELETE_ON_CLOSE;
                    goto DiscardDelete;
                }
            } else
            if (lc <= 1) {
                // Synchronize here with paging IO
                BOOLEAN AcquiredPagingIo;
                AcquiredPagingIo = UDFAcquireResourceExclusiveWithCheck(&(NtReqFcb->PagingIoResource));
                // set file size to zero (for UdfInfo package)
                // we should not do this for directories and linked files
                UDFResizeFile__(Vcb, NextFileInfo, 0);
                if(AcquiredPagingIo) {
                    UDFReleaseResource(&(NtReqFcb->PagingIoResource));
                }
            }
            // mark parent object for deletion if requested
            if((Fcb->FCBFlags & UDF_FCB_DELETE_PARENT) &&
                Fcb->ParentFcb) {
                ASSERT(!(Fcb->ParentFcb->FCBFlags & UDF_FCB_ROOT_DIRECTORY));
                Fcb->ParentFcb->FCBFlags |= UDF_FCB_DELETE_ON_CLOSE;
            }
            // flush file. It is required by UDFUnlinkFile__()
            RC = UDFFlushFile__(Vcb, NextFileInfo);
            if(!NT_SUCCESS(RC)) {
                AdPrint(("Error flushing file !!!\n"));
            }
            // try to unlink
            if((RC = UDFUnlinkFile__(Vcb, NextFileInfo, TRUE)) == STATUS_CANNOT_DELETE) {
                // If we can't delete file with Streams due to references,
                // mark SDir & Streams
                // for Deletion. We shall also set DELETE_PARENT flag to
                // force Deletion of the current file later... when curently
                // opened Streams would be cleaned up.

                // WARNING! We should keep SDir & Streams if there is a
                // link to this file
                if(NextFileInfo->Dloc &&
                   NextFileInfo->Dloc->SDirInfo &&
                   NextFileInfo->Dloc->SDirInfo->Fcb) {

                    BrutePoint();
                    if(!UDFIsSDirDeleted(NextFileInfo->Dloc->SDirInfo)) {
//                        RC = UDFMarkStreamsForDeletion(Vcb, Fcb, TRUE); // Delete
//#ifdef UDF_ALLOW_PRETEND_DELETED
                        UDFPretendFileDeleted__(Vcb, Fcb->FileInfo);
//#endif //UDF_ALLOW_PRETEND_DELETED
                    }
                    goto NotifyDelete;

                } else {
                    // Getting here means that we can't delete file because of
                    // References/PemissionsDenied/Smth.Else,
                    // but not Linked+OpenedStream
                    BrutePoint();
//                    RC = STATUS_SUCCESS;
                    goto DiscardDelete_1;
                }
            } else {
DiscardDelete_1:
                // We have got an ugly ERROR, or
                // file is deleted, so forget about it
                ASSERT(!(Fcb->FCBFlags & UDF_FCB_ROOT_DIRECTORY));
                ForcedCleanUp = TRUE;
                if(NT_SUCCESS(RC))
                    Fcb->FCBFlags &= ~UDF_FCB_DELETE_ON_CLOSE;
                Fcb->FCBFlags |= UDF_FCB_DELETED;
                RC = STATUS_SUCCESS;
            }
NotifyDelete:
            // We should prevent SetEOF operations on completly
            // deleted data streams
            if(lc < 1) {
                NtReqFcb->NtReqFCBFlags |= UDF_NTREQ_FCB_DELETED;
            }
            // Report that we have removed an entry.
            if(UDFIsAStream(NextFileInfo)) {
                UDFNotifyFullReportChange( Vcb, NextFileInfo,
                                       FILE_NOTIFY_CHANGE_STREAM_NAME,
                                       FILE_ACTION_REMOVED_STREAM);
            } else {
                UDFNotifyFullReportChange( Vcb, NextFileInfo,
                                       UDFIsADirectory(NextFileInfo) ? FILE_NOTIFY_CHANGE_DIR_NAME : FILE_NOTIFY_CHANGE_FILE_NAME,
                                       FILE_ACTION_REMOVED);
            }
        } else
        if(Fcb->FCBFlags & UDF_FCB_DELETE_ON_CLOSE) {
DiscardDelete:
            UDFNotifyFullReportChange( Vcb, NextFileInfo,
                                     ((Ccb->CCBFlags & UDF_CCB_ACCESS_TIME_SET) ? FILE_NOTIFY_CHANGE_LAST_ACCESS : 0) |
                                     ((Ccb->CCBFlags & UDF_CCB_WRITE_TIME_SET) ? (FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_LAST_WRITE) : 0) |
                                     0,
                                     UDFIsAStream(NextFileInfo) ? FILE_ACTION_MODIFIED_STREAM : FILE_ACTION_MODIFIED);
        }
#endif //UDF_READ_ONLY_BUILD

        if(Fcb->FCBFlags & UDF_FCB_DIRECTORY) {
            //  Report to the dir notify package for a directory.
            FsRtlNotifyCleanup( Vcb->NotifyIRPMutex, &(Vcb->NextNotifyIRP), (PVOID)Ccb );
        }

        // we can't purge Cache when more than one link exists
        if(lc > 1) {
            ForcedCleanUp = FALSE;
        }

        if ( (FileObject->Flags & FO_CACHE_SUPPORTED) &&
             (NtReqFcb->SectionObject.DataSectionObject) ) {
            BOOLEAN LastNonCached = (!Fcb->CachedOpenHandleCount &&
                                      Fcb->OpenHandleCount);
            // If this was the last cached open, and there are open
            // non-cached handles, attempt a flush and purge operation
            // to avoid cache coherency overhead from these non-cached
            // handles later.  We ignore any I/O errors from the flush.
            // We shall not flush deleted files
            RC = STATUS_SUCCESS;
            if(  LastNonCached
                      ||
                (!Fcb->OpenHandleCount &&
                 !ForcedCleanUp) ) {

#ifndef UDF_READ_ONLY_BUILD
                LONGLONG OldFileSize, NewFileSize;

                if( (OldFileSize = NtReqFcb->CommonFCBHeader.ValidDataLength.QuadPart) < 
                    (NewFileSize = NtReqFcb->CommonFCBHeader.FileSize.QuadPart)) {
/*                    UDFZeroDataEx(NtReqFcb,
                                  OldFileSize,
                                  NewFileSize - OldFileSize,
                                  TRUE, Vcb, FileObject);*/
                    
                    NtReqFcb->CommonFCBHeader.ValidDataLength.QuadPart = NewFileSize;
                }
#endif //UDF_READ_ONLY_BUILD
                MmPrint(("    CcFlushCache()\n"));
                CcFlushCache( &(NtReqFcb->SectionObject), NULL, 0, &IoStatus );
                if(!NT_SUCCESS(IoStatus.Status)) {
                    MmPrint(("    CcFlushCache() error: %x\n", IoStatus.Status));
                    RC = IoStatus.Status;
                }
            }
            // If file is deleted or it is last cached open, but there are
            // some non-cached handles we should purge cache section
            if(ForcedCleanUp || LastNonCached) {
                if(NtReqFcb->SectionObject.DataSectionObject) {
                    MmPrint(("    CcPurgeCacheSection()\n"));
                    CcPurgeCacheSection( &(NtReqFcb->SectionObject), NULL, 0, FALSE );
                }
/*                MmPrint(("    CcPurgeCacheSection()\n"));
                CcPurgeCacheSection( &(NtReqFcb->SectionObject), NULL, 0, FALSE );*/
            }
            // we needn't Flush here. It will be done in UDFCloseFileInfoChain()
        }

#ifndef UDF_READ_ONLY_BUILD
        // Update FileTimes & Attrs
        if(!(Vcb->VCBFlags & UDF_VCB_FLAGS_VOLUME_READ_ONLY) &&
           !(Fcb->FCBFlags & (UDF_FCB_DELETE_ON_CLOSE |
                              UDF_FCB_DELETED /*|
                              UDF_FCB_DIRECTORY /*|
                              UDF_FCB_READ_ONLY*/)) &&
           !UDFIsAStreamDir(NextFileInfo)) {
            LONGLONG NtTime;
            LONGLONG ASize;
            KeQuerySystemTime((PLARGE_INTEGER)&NtTime);
            // Check if we should set ARCHIVE bit & LastWriteTime
            if(FileObject->Flags & FO_FILE_MODIFIED) {
                ULONG Attr;
                PDIR_INDEX_ITEM DirNdx;
                DirNdx = UDFDirIndex(UDFGetDirIndexByFileInfo(NextFileInfo), NextFileInfo->Index);
                ASSERT(DirNdx);
                // Archive bit
                if(!(Ccb->CCBFlags & UDF_CCB_ATTRIBUTES_SET) &&
                    (Vcb->CompatFlags & UDF_VCB_IC_UPDATE_ARCH_BIT)) {
                    Attr = UDFAttributesToNT(DirNdx, NextFileInfo->Dloc->FileEntry);
                    if(!(Attr & FILE_ATTRIBUTE_ARCHIVE))
                        UDFAttributesToUDF(DirNdx, NextFileInfo->Dloc->FileEntry, Attr | FILE_ATTRIBUTE_ARCHIVE);
                }
                // WriteTime
                if(!(Ccb->CCBFlags & UDF_CCB_WRITE_TIME_SET) && 
                    (Vcb->CompatFlags & UDF_VCB_IC_UPDATE_MODIFY_TIME)) {
                    UDFSetFileXTime(NextFileInfo, NULL, &NtTime, NULL, &NtTime);
                    NtReqFcb->LastWriteTime.QuadPart =
                    NtReqFcb->LastAccessTime.QuadPart = NtTime;
                    ChangeTime = TRUE;
                }
            }
            if(!(Fcb->FCBFlags & UDF_FCB_DIRECTORY)) {
                // Update sizes in DirIndex
                if(!Fcb->OpenHandleCount) {
                    ASize = UDFGetFileAllocationSize(Vcb, NextFileInfo);
//                        NtReqFcb->CommonFCBHeader.AllocationSize.QuadPart;
                    UDFSetFileSizeInDirNdx(Vcb, NextFileInfo, &ASize);
                } else
                if(FileObject->Flags & FO_FILE_SIZE_CHANGED) {
                    ASize = //UDFGetFileAllocationSize(Vcb, NextFileInfo);
                        NtReqFcb->CommonFCBHeader.AllocationSize.QuadPart;
                    UDFSetFileSizeInDirNdx(Vcb, NextFileInfo, &ASize);
                }
            }
            // AccessTime
            if((FileObject->Flags & FO_FILE_FAST_IO_READ) &&
               !(Ccb->CCBFlags & UDF_CCB_ACCESS_TIME_SET) &&
                (Vcb->CompatFlags & UDF_VCB_IC_UPDATE_ACCESS_TIME)) {
                UDFSetFileXTime(NextFileInfo, NULL, &NtTime, NULL, NULL);
                NtReqFcb->LastAccessTime.QuadPart = NtTime;
//                ChangeTime = TRUE;
            }
            // ChangeTime (AttrTime)
            if(!(Ccb->CCBFlags & UDF_CCB_MODIFY_TIME_SET) &&
                (Vcb->CompatFlags & UDF_VCB_IC_UPDATE_ATTR_TIME) &&
                (ChangeTime || (Ccb->CCBFlags & (UDF_CCB_ATTRIBUTES_SET |
                                                 UDF_CCB_CREATE_TIME_SET |
                                                 UDF_CCB_ACCESS_TIME_SET |
                                                 UDF_CCB_WRITE_TIME_SET))) ) {
                UDFSetFileXTime(NextFileInfo, NULL, NULL, &NtTime, NULL);
                NtReqFcb->ChangeTime.QuadPart = NtTime;
            }
        }
#endif //UDF_READ_ONLY_BUILD

        if(!(Fcb->FCBFlags & UDF_FCB_DIRECTORY) &&
            ForcedCleanUp) {
            // flush system cache
            MmPrint(("    CcUninitializeCacheMap()\n"));
            CcUninitializeCacheMap(FileObject, &(UDFGlobalData.UDFLargeZero), NULL);
        } else {
            MmPrint(("    CcUninitializeCacheMap()\n"));
            CcUninitializeCacheMap(FileObject, NULL, NULL);
        }

        // release resources now.
        // they'll be acquired in UDFCloseFileInfoChain()
        UDF_CHECK_PAGING_IO_RESOURCE(NtReqFcb);
        UDFReleaseResource(&(NtReqFcb->MainResource));
        AcquiredFCB = FALSE;

        if(Fcb->FileInfo->ParentFile) {
            UDF_CHECK_PAGING_IO_RESOURCE(Fcb->FileInfo->ParentFile->Fcb->NTRequiredFCB);
            UDFReleaseResource(&(Fcb->FileInfo->ParentFile->Fcb->NTRequiredFCB->MainResource));
        } else {
            UDFReleaseResource(&(Vcb->VCBResource));
        }
        AcquiredParentFCB = FALSE;
        // close the chain
        ASSERT(AcquiredVcb);
        RC2 = UDFCloseFileInfoChain(Vcb, NextFileInfo, Ccb->TreeLength, TRUE);
        if(NT_SUCCESS(RC))
            RC = RC2;

        Ccb->CCBFlags |= UDF_CCB_CLEANED;

        //  We must clean up the share access at this time, since we may not
        //  get a Close call for awhile if the file was mapped through this
        //  File Object.
        IoRemoveShareAccess( FileObject, &(NtReqFcb->FCBShareAccess) );

        NtReqFcb->CommonFCBHeader.IsFastIoPossible = UDFIsFastIoPossible(Fcb);

        FileObject->Flags |= FO_CLEANUP_COMPLETE;

try_exit: NOTHING;

    } _SEH2_FINALLY {

        if(AcquiredFCB) {
            UDF_CHECK_PAGING_IO_RESOURCE(NtReqFcb);
            UDFReleaseResource(&(NtReqFcb->MainResource));
        }

        if(AcquiredParentFCB) {
            if(Fcb->FileInfo->ParentFile) {
                UDF_CHECK_PAGING_IO_RESOURCE(Fcb->FileInfo->ParentFile->Fcb->NTRequiredFCB);
                UDFReleaseResource(&(Fcb->FileInfo->ParentFile->Fcb->NTRequiredFCB->MainResource));
            } else {
                UDFReleaseResource(&(Vcb->VCBResource));
            }
        }

        if(AcquiredVcb) {
            UDFReleaseResource(&(Vcb->VCBResource));
            AcquiredVcb = FALSE;
        }

        if (!_SEH2_AbnormalTermination()) {
            // complete the IRP
            Irp->IoStatus.Status = RC;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest(Irp, IO_DISK_INCREMENT);
            // Free up the Irp Context
            UDFReleaseIrpContext(PtrIrpContext);
        }

    } _SEH2_END; // end of "__finally" processing
    return(RC);
} // end UDFCommonCleanup()

/*
    This routine walks through the tree to RootDir &
    calls UDFCloseFile__() for each file instance
    imho, Useful feature
 */
NTSTATUS
UDFCloseFileInfoChain(
    IN PVCB Vcb,
    IN PUDF_FILE_INFO fi,
    IN ULONG TreeLength,
    IN BOOLEAN VcbAcquired
    )
{
    PUDF_FILE_INFO ParentFI;
    PtrUDFFCB Fcb;
    PtrUDFFCB ParentFcb = NULL;
    NTSTATUS RC = STATUS_SUCCESS;
    NTSTATUS RC2;

    // we can't process Tree until we can acquire Vcb
    if(!VcbAcquired)
        UDFAcquireResourceShared(&(Vcb->VCBResource),TRUE);

    AdPrint(("UDFCloseFileInfoChain\n"));
    for(; TreeLength && fi; TreeLength--) {

        // close parent chain (if any)
        // if we started path parsing not from RootDir on Create,
        // we would never get RootDir here
        ValidateFileInfo(fi);

        // acquire parent
        if(ParentFI = fi->ParentFile) {
            ParentFcb = fi->Fcb->ParentFcb;
            ASSERT(ParentFcb);
            ASSERT(ParentFcb->NTRequiredFCB);
            UDF_CHECK_PAGING_IO_RESOURCE(ParentFcb->NTRequiredFCB);
            UDFAcquireResourceExclusive(&(ParentFcb->NTRequiredFCB->MainResource),TRUE);
            ASSERT(ParentFcb->NodeIdentifier.NodeType == UDF_NODE_TYPE_FCB);
            ASSERT(ParentFcb->NTRequiredFCB->CommonFCBHeader.NodeTypeCode == UDF_NODE_TYPE_NT_REQ_FCB);
        } else {
            AdPrint(("Acquiring VCB...\n"));
            UDFAcquireResourceShared(&(Vcb->VCBResource),TRUE);
            AdPrint(("Done\n"));
        }
        // acquire current file/dir
        // we must assure that no more threads try to reuse this object
        if(Fcb = fi->Fcb) {
            UDF_CHECK_PAGING_IO_RESOURCE(Fcb->NTRequiredFCB);
            UDFAcquireResourceExclusive(&(Fcb->NTRequiredFCB->MainResource),TRUE);
            ASSERT_REF(Fcb->ReferenceCount >= fi->RefCount);
            if(!(Fcb->FCBFlags & UDF_FCB_DELETED) &&
                (Fcb->FCBFlags & UDF_FCB_VALID))
                UDFWriteSecurity(Vcb, Fcb, &(Fcb->NTRequiredFCB->SecurityDesc));
            RC2 = UDFCloseFile__(Vcb,fi);
            if(!NT_SUCCESS(RC2))
                RC = RC2;
            ASSERT_REF(Fcb->ReferenceCount > fi->RefCount);
            UDF_CHECK_PAGING_IO_RESOURCE(Fcb->NTRequiredFCB);
            UDFReleaseResource(&(Fcb->NTRequiredFCB->MainResource));
        } else {
            BrutePoint();
            RC2 = UDFCloseFile__(Vcb,fi);
            if(!NT_SUCCESS(RC2))
                RC = RC2;
        }

        if(ParentFI) {
            UDF_CHECK_PAGING_IO_RESOURCE(ParentFcb->NTRequiredFCB);
            UDFReleaseResource(&(ParentFcb->NTRequiredFCB->MainResource));
        } else {
            UDFReleaseResource(&(Vcb->VCBResource));
        }
        fi = ParentFI;
    }

    if(!VcbAcquired)
        UDFReleaseResource(&(Vcb->VCBResource));

    return RC;

} // end UDFCloseFileInfoChain()
