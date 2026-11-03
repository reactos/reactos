////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////

/*************************************************************************
*
* File: Flush.cpp
*
* Module: UDF File System Driver (Kernel mode execution only)
*
* Description:
*   Contains code to handle the "Flush Buffers" dispatch entry point.
*
*************************************************************************/

#include            "udffs.h"

// define the file specific bug-check id
#define         UDF_BUG_CHECK_ID                UDF_FILE_FLUSH

/* Keep the first filesystem flush failure while later files and the lower
 * device flush are processed.  A successful later operation cannot commit
 * metadata whose earlier write failed. */
/* The VCB resource and the target FCB/LCB keep this parent chain alive.
 * Validate it before following arbitrary depth; do not silently stop at
 * the append-buffer count, since clean intermediate ancestors can separate
 * two dirty generations. Caller releases the returned held resource. */
static NTSTATUS
UDFFlushAncestorMetadata(PIRP_CONTEXT IrpContext, PVCB Vcb,
    PUDF_FILE_INFO Directory, PFCB* Held)
{
    PUDF_FILE_INFO slow = Directory, fast = Directory, ancestor;
    NTSTATUS status;
    PFCB previous;
    while (fast && fast->ParentFile) {
        slow = slow->ParentFile;
        fast = fast->ParentFile->ParentFile;
        if (slow && slow == fast)
            return STATUS_FILE_CORRUPT_ERROR;
    }
    if (Directory && !*Held) {
        if (!Directory->Fcb)
            return STATUS_FILE_CORRUPT_ERROR;
        UDF_CHECK_PAGING_IO_RESOURCE(Directory->Fcb);
        UDFAcquireFcbExclusive(IrpContext, Directory->Fcb, FALSE);
        *Held = Directory->Fcb;
    }
    while (Directory) {
        if (!Directory->Dloc || Directory->Fcb != *Held)
            return STATUS_FILE_CORRUPT_ERROR;
        status = UDFFlushFile__(IrpContext, Vcb, Directory, UDF_FLUSH_FLAGS_LITE);
        if (!NT_SUCCESS(status))
            return status;
        ancestor = Directory->ParentFile;
        if (!ancestor)
            break;
        if (!ancestor->Fcb)
            return STATUS_FILE_CORRUPT_ERROR;
        UDF_CHECK_PAGING_IO_RESOURCE(ancestor->Fcb);
        UDFAcquireFcbExclusive(IrpContext, ancestor->Fcb, FALSE);
        previous = *Held;
        *Held = ancestor->Fcb;
        UDFReleaseFcb(IrpContext, previous);
        Directory = ancestor;
    }
    return STATUS_SUCCESS;
}


static VOID
UDFRememberFlushStatus(PIO_STATUS_BLOCK IoStatus, NTSTATUS Status)
{
    if (NT_SUCCESS(IoStatus->Status) && !NT_SUCCESS(Status))
        IoStatus->Status = Status;
}

/*************************************************************************
*
* Function: UDFCommonFlush()
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
* Return Value: STATUS_SUCCESS/Error
*
*************************************************************************/
NTSTATUS
UDFCommonFlush(
    PIRP_CONTEXT IrpContext,
    PIRP             Irp
    )
{
    NTSTATUS            Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION  IrpSp = NULL;
    TYPE_OF_OPEN        TypeOfOpen;
    PFCB                Fcb = NULL;
    PCCB                Ccb = NULL;
    PVCB                Vcb = NULL;
    BOOLEAN             AcquiredVCB = FALSE;
    BOOLEAN             AcquiredFCB = FALSE;
    BOOLEAN             AcquiredParentFcb = FALSE;
    PFCB                HeldParentFcb = NULL;
    IO_STATUS_BLOCK     FsIoStatus;

    PAGED_CODE();

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    // Decode the file object

    TypeOfOpen = UDFDecodeFileObject(IrpSp->FileObject, &Fcb, &Ccb);

    Vcb = IrpContext->Vcb;

    ASSERT_CCB(Ccb);
    ASSERT_FCB(Fcb);
    ASSERT_VCB(Vcb);

    if (Vcb->VcbState & VCB_STATE_VOLUME_READ_ONLY) {

        if (Vcb->VcbState & VCB_STATE_MEDIA_WRITE_PROTECT) {

            Status = STATUS_MEDIA_WRITE_PROTECTED;
        }
        else if (Vcb->VcbState & VCB_STATE_MOUNTED_DIRTY) {

            Status = STATUS_VOLUME_DIRTY;
        }
        else {

            Status = STATUS_ACCESS_DENIED;
        }

        UDFCompleteRequest(IrpContext, Irp, Status);
        return Status;
    }

    // Check for invalid call from user mode

    if (IrpSp->MinorFunction == IRP_MN_MOUNT_VOLUME && Irp->RequestorMode == UserMode) {

        Status = STATUS_INVALID_PARAMETER;
        UDFCompleteRequest(IrpContext, Irp, Status);
        return Status;
    }

    //  CcFlushCache is always synchronous, so if we can't wait enqueue
    //  the irp to the Fsp.

    if (!FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT)) {

        Status = UDFFsdPostRequest(IrpContext, Irp);

        return Status;
    }

    Status = STATUS_SUCCESS;
    FsIoStatus.Status = STATUS_SUCCESS;
    FsIoStatus.Information = 0;

    _SEH2_TRY {

        // Check the type of object passed-in. That will determine the course of
        // action we take.
        if ((Fcb == Fcb->Vcb->VolumeDasdFcb) || (Fcb->FcbState & UDF_FCB_ROOT_DIRECTORY)) {

            Status = UDFFspCloseForFlush(Vcb);
            if (!NT_SUCCESS(Status))
                try_return(Status);

            UDFAcquireVcbExclusive(IrpContext, Vcb, FALSE);
            AcquiredVCB = TRUE;
            // The caller wishes to flush all files for the mounted
            // logical volume. The flush volume routine below should simply
            // walk through all of the open file streams, acquire the
            // VCB resource, and request the flush operation from the Cache
            // Manager. Basically, the sequence of operations listed below
            // for a single file should be executed on all open files.

            UDFVerifyVcb(IrpContext, Vcb);

            Status = UDFFlushVolume(IrpContext, Vcb, 0);

            UDFReleaseVcb(IrpContext, Vcb);
            AcquiredVCB = FALSE;

            try_return(Status);
        } else
        {
            // Flush this file or directory and its parent FID.
            Vcb = Fcb->Vcb;
            ASSERT(Vcb);

            // A file flush also updates its parent FID. Cleanup and volume
            // flush serialize those sector read-modify-writes with the Vcb
            // resource, so join that protocol before taking either Fcb lock.
            if (Fcb->FcbState & UDF_FCB_DIRECTORY) {
                UDFAcquireVcbExclusive(IrpContext, Vcb, FALSE);
                AcquiredVCB = TRUE;
                UDFRememberFlushStatus(&FsIoStatus, UDFTakeMetadataWritebackError(Vcb));
                UDFFlushADirectory(IrpContext, Vcb, Fcb->FileInfo, &FsIoStatus, 0);
                if (NT_SUCCESS(FsIoStatus.Status))
                    UDFRememberFlushStatus(&FsIoStatus,
                        UDFFlushAncestorMetadata(IrpContext, Vcb,
                            Fcb->FileInfo->ParentFile, &HeldParentFcb));
                AcquiredParentFcb = HeldParentFcb != NULL;
                Status = FsIoStatus.Status;
                try_return(Status);
            }
            UDFAcquireVcbShared(IrpContext, Vcb, FALSE);
            AcquiredVCB = TRUE;
            UDFRememberFlushStatus(&FsIoStatus, UDFTakeMetadataWritebackError(Vcb));

            // Child-first lock ordering
            UDF_CHECK_PAGING_IO_RESOURCE(Fcb);
            UDFAcquireFcbExclusive(IrpContext, Fcb, FALSE);
            AcquiredFCB = TRUE;

            // Parent second (needed for DirIndex modification in UDFSetFileSizeInDirNdx)
            if (Fcb->FileInfo->ParentFile && Fcb->FileInfo->ParentFile->Fcb) {
                UDF_CHECK_PAGING_IO_RESOURCE(Fcb->FileInfo->ParentFile->Fcb);
                UDFAcquireFcbExclusive(IrpContext, Fcb->FileInfo->ParentFile->Fcb, FALSE);
                AcquiredParentFcb = TRUE;
                HeldParentFcb = Fcb->FileInfo->ParentFile->Fcb;
            }

            // Request the Cache Manager to perform a flush operation.
            // Further, instruct the Cache Manager that we wish to flush the
            // entire file stream.
            UDFFlushAFile(IrpContext, Fcb, Ccb, &FsIoStatus, 0);
            Status = FsIoStatus.Status;

            /* Commit the namespace path too, including dirty ancestors
             * above an otherwise clean intermediate directory. */
            if (NT_SUCCESS(Status) && AcquiredParentFcb) {
                UDFRememberFlushStatus(&FsIoStatus,
                    UDFFlushAncestorMetadata(IrpContext, Vcb,
                        Fcb->FileInfo->ParentFile, &HeldParentFcb));
                Status = FsIoStatus.Status;
            }

            // Some log-based FSD implementations may wish to flush their
            // log files at this time. Finally, we should update the time-stamp
            // values for the file stream appropriately. This would involve
            // obtaining the current time and modifying the appropriate directory
            // entry fields.
        }

try_exit:   NOTHING;

    } _SEH2_FINALLY {

        // Release in reverse order of acquisition (parent first, then child)
        if (AcquiredParentFcb) {
            UDFReleaseFcb(IrpContext, HeldParentFcb);
            AcquiredParentFcb = FALSE;
        }

        if (AcquiredFCB) {
            UDFReleaseFcb(IrpContext, Fcb);
            AcquiredFCB = FALSE;
        }

        if (AcquiredVCB) {
            UDFReleaseVcb(IrpContext, Vcb);
            AcquiredVCB = FALSE;
        }

        if (!_SEH2_AbnormalTermination()) {

            NTSTATUS DriverStatus;

            /* With copy-on-write, a flush is only durable once the buffered
             * metadata is committed. Done after the locks above are released
             * so the checkpoint can wait for other operations to finish. */
            if (NT_SUCCESS(Status) && Vcb && Vcb->Cow) {
                NTSTATUS cowStatus = UDFCowSync(IrpContext, Vcb);
                if (!NT_SUCCESS(cowStatus))
                    Status = cowStatus;
            }

            // Get the next stack location, and copy over the stack location

            IoCopyCurrentIrpStackLocationToNext(Irp);

            // Set up the completion routine

            IoSetCompletionRoutine(Irp,
                                   UDFFlushCompletion,
                                   ULongToPtr(Status),
                                   TRUE,
                                   TRUE,
                                   TRUE);

            // Send the request.

            DriverStatus = IoCallDriver(Vcb->TargetDeviceObject, Irp);

            if (DriverStatus == STATUS_PENDING ||
                (NT_SUCCESS(Status) && !NT_SUCCESS(DriverStatus) &&
                 DriverStatus != STATUS_INVALID_DEVICE_REQUEST)) {
                Status = DriverStatus;
            }

            // Release the IRP context at this time.
            UDFCompleteRequest(IrpContext, NULL, STATUS_SUCCESS);
        }
    } _SEH2_END;

    return Status;
} // end UDFCommonFlush()


/*************************************************************************
*
* Function: UDFFlushAFile()
*
* Description:
*   Tell the Cache Manager to perform a flush.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: None
*
*************************************************************************/
ULONG
UDFFlushAFile(
    IN PIRP_CONTEXT IrpContext,
    IN PFCB                Fcb,
    IN PCCB                Ccb,
    OUT PIO_STATUS_BLOCK   PtrIoStatus,
    IN ULONG               FlushFlags
    )
{
    BOOLEAN SetArchive = FALSE;
    BOOLEAN CacheFlushFailed = FALSE;
//    BOOLEAN PurgeCache = FALSE;
    ULONG ret_val = 0;

    UDFPrint(("UDFFlushAFile: \n"));
    if (!Fcb)
        return 0;

    // Flush SDir if any
    _SEH2_TRY {
        if (UDFHasAStreamDir(Fcb->FileInfo) &&
           Fcb->FileInfo->Dloc->SDirInfo &&
           !UDFIsSDirDeleted(Fcb->FileInfo->Dloc->SDirInfo) ) {
            ret_val |=
                UDFFlushADirectory(IrpContext, Fcb->Vcb, Fcb->FileInfo->Dloc->SDirInfo, PtrIoStatus, FlushFlags);
        }
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        BrutePoint();
        UDFRememberFlushStatus(PtrIoStatus, _SEH2_GetExceptionCode());
    } _SEH2_END;
    // Flush File
    _SEH2_TRY {
        if ((Fcb->CachedOpenHandleCount || !Fcb->FcbCleanup) &&
            Fcb->FcbNonpaged->SegmentObject.DataSectionObject) {
            if (!(Fcb->NtReqFCBFlags & UDF_NTREQ_FCB_DELETED)
                                         &&
                (Fcb->NtReqFCBFlags & UDF_NTREQ_FCB_MODIFIED)) {
                MmPrint(("    CcFlushCache()\n"));
                IO_STATUS_BLOCK CacheIoStatus;
                CacheIoStatus.Status = STATUS_SUCCESS;
                CacheIoStatus.Information = 0;
                CcFlushCache(&Fcb->FcbNonpaged->SegmentObject, NULL, 0, &CacheIoStatus);
                UDFRememberFlushStatus(PtrIoStatus, CacheIoStatus.Status);
                CacheFlushFailed = !NT_SUCCESS(CacheIoStatus.Status);
            }
            // notice, that we should purge cache
            // we can't do it now, because it may cause last Close
            // request & thus, structure deallocation
//            PurgeCache = TRUE;

            if (Ccb) {
                if ( (Ccb->FileObject->Flags & FO_FILE_MODIFIED) &&
                   !(Ccb->Flags & UDF_CCB_WRITE_TIME_SET)) {
                    if (Fcb->Vcb->CompatFlags & UDF_VCB_IC_UPDATE_MODIFY_TIME) {
                        LONGLONG NtTime;
                        KeQuerySystemTime((PLARGE_INTEGER)&NtTime);
                        UDFSetFileXTime(Fcb->FileInfo, NULL, NULL, NULL, &NtTime);
                        Fcb->LastWriteTime.QuadPart = NtTime;
                    }
                    SetArchive = TRUE;
                    Ccb->FileObject->Flags &= ~FO_FILE_MODIFIED;
                }
                if (Ccb->FileObject->Flags & FO_FILE_SIZE_CHANGED) {
                    LONGLONG ASize = UDFGetFileAllocationSize(Fcb->Vcb, Fcb->FileInfo);
                    UDFSetFileSizeInDirNdx(Fcb->Vcb, Fcb->FileInfo, &ASize);
                    Ccb->FileObject->Flags &= ~FO_FILE_SIZE_CHANGED;
                }
            }
        }
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        BrutePoint();
        UDFRememberFlushStatus(PtrIoStatus, _SEH2_GetExceptionCode());
        CacheFlushFailed = TRUE;
    } _SEH2_END;

    if (CacheFlushFailed)
        return ret_val;

    _SEH2_TRY {
        if (SetArchive &&
           (Fcb->Vcb->CompatFlags & UDF_VCB_IC_UPDATE_ARCH_BIT)) {
            ULONG Attr;
            PDIR_INDEX_ITEM DirNdx;
            DirNdx = UDFDirIndex(UDFGetDirIndexByFileInfo(Fcb->FileInfo), Fcb->FileInfo->Index);
            // Archive bit
            Attr = UDFAttributesToNT(DirNdx, Fcb->FileInfo->Dloc->FileEntry);
            if (!(Attr & FILE_ATTRIBUTE_ARCHIVE))
                UDFAttributesToUDF(DirNdx, Fcb->FileInfo->Dloc->FileEntry, Attr | FILE_ATTRIBUTE_ARCHIVE);
        }

        UDFRememberFlushStatus(PtrIoStatus,
            UDFFlushFile__(IrpContext, Fcb->Vcb, Fcb->FileInfo, FlushFlags));
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        BrutePoint();
        UDFRememberFlushStatus(PtrIoStatus, _SEH2_GetExceptionCode());
    } _SEH2_END;

/*    if (PurgeCache) {
        _SEH2_TRY {
            MmPrint(("    CcPurgeCacheSection()\n"));
            CcPurgeCacheSection( &(Fcb->NTRequiredFCB->SectionObject), NULL, 0, FALSE );
        } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
            BrutePoint();
        } _SEH2_END;
    }*/

    return ret_val;
} // end UDFFlushAFile()

/*************************************************************************
*
* Function: UDFFlushADirectory()
*
* Description:
*   Tell the Cache Manager to perform a flush for all files
*   in current directory & all subdirectories and flush all metadata
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: None
*
*************************************************************************/
ULONG
UDFFlushADirectory(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB                Vcb,
    IN PUDF_FILE_INFO      FI,
    OUT PIO_STATUS_BLOCK   PtrIoStatus,
    IN ULONG               FlushFlags
    )
{
    UDFPrint(("UDFFlushADirectory: \n"));
//    PDIR_INDEX_HDR hDI;
    PDIR_INDEX_ITEM DI;
//    BOOLEAN Referenced = FALSE;
    ULONG ret_val = 0;

    if (!FI || !FI->Dloc || !FI->Dloc->DirIndex) goto SkipFlushDir;
//    hDI = FI->Dloc->DirIndex;

    // Flush SDir if any
    _SEH2_TRY {
        if (UDFHasAStreamDir(FI) &&
           FI->Dloc->SDirInfo &&
           !UDFIsSDirDeleted(FI->Dloc->SDirInfo) ) {
            ret_val |=
                UDFFlushADirectory(IrpContext, Vcb, FI->Dloc->SDirInfo, PtrIoStatus, FlushFlags);
        }
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        BrutePoint();
        UDFRememberFlushStatus(PtrIoStatus, _SEH2_GetExceptionCode());
    } _SEH2_END;

    // Flush Dir Tree
    _SEH2_TRY {
        UDF_DIR_SCAN_CONTEXT ScanContext;
        PUDF_FILE_INFO      tempFI;

        if (UDFDirIndexInitScan(FI, &ScanContext, 2)) {
            while((DI = UDFDirIndexScan(&ScanContext, &tempFI))) {
                // Flush Dir entry
                _SEH2_TRY {
                    if (!tempFI) continue;
                    if (UDFIsADirectory(tempFI)) {
                        UDFFlushADirectory(IrpContext, Vcb, tempFI, PtrIoStatus, FlushFlags);
                    } else if (!tempFI->Fcb && tempFI->Dloc) {
                        /* A retained FileInfo may no longer have an FCB.
                         * Passing NULL to UDFFlushAFile silently skips its
                         * dirty FE/FID, so flush that metadata directly. */
                        if ((tempFI->Dloc->FE_Flags & UDF_FE_FLAG_FE_MODIFIED) ||
                            tempFI->Dloc->DataLoc.Modified ||
                            tempFI->Dloc->AllocLoc.Modified ||
                            tempFI->Dloc->FELoc.Modified ||
                            (DI->FI_Flags & UDF_FI_FLAG_FI_MODIFIED)) {
                            UDFRememberFlushStatus(PtrIoStatus,
                                UDFFlushFile__(IrpContext, Vcb, tempFI,
                                               FlushFlags));
                        }
                    } else {
                        UDFFlushAFile(IrpContext, tempFI->Fcb, NULL, PtrIoStatus, FlushFlags);
                    }
                } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
                    BrutePoint();
                    UDFRememberFlushStatus(PtrIoStatus, _SEH2_GetExceptionCode());
                } _SEH2_END;
                if (UDFFlushIsBreaking(Vcb, FlushFlags)) {
                    ret_val |= UDF_FLUSH_FLAGS_INTERRUPTED;
                    break;
                }
            }
        }
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        BrutePoint();
        UDFRememberFlushStatus(PtrIoStatus, _SEH2_GetExceptionCode());
    } _SEH2_END;
SkipFlushDir:
    // Flush Dir
    _SEH2_TRY {
        UDFRememberFlushStatus(PtrIoStatus,
            UDFFlushFile__(IrpContext, Vcb, FI, FlushFlags));
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        BrutePoint();
        UDFRememberFlushStatus(PtrIoStatus, _SEH2_GetExceptionCode());
    } _SEH2_END;

    return ret_val;
} // end UDFFlushADirectory()

/*************************************************************************
*
* Function: UDFFlushLogicalVolume()
*
* Description:
*   Flush everything beginning from root directory.
*   Vcb must be previously acquired exclusively.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: None
*
*************************************************************************/
NTSTATUS
UDFFlushVolume(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG FlushFlags
    )
{
    ULONG ret_val = 0;
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS Status;
    BOOLEAN CloseDeferred = FALSE;

    UDFPrint(("UDFFlushVolume: \n"));

    ASSERT_EXCLUSIVE_VCB(Vcb);
    IoStatus.Status = STATUS_SUCCESS;
    IoStatus.Information = 0;

    Status = UDFTakeMetadataWritebackError(Vcb);
    UDFFspClose(Vcb);
    {
        NTSTATUS current = UDFTakeMetadataWritebackError(Vcb);
        if (NT_SUCCESS(Status))
            Status = current;
    }
    if (!NT_SUCCESS(Status))
        return Status;

    _SEH2_TRY {
        if (Vcb->VcbState & VCB_STATE_VOLUME_READ_ONLY)
            return 0;
        if (Vcb->VcbCondition != VcbMounted)
            return 0;

        /* Closed handles can retain a live cache/section FILE_OBJECT and
         * never reach delayed close. Prepare their owned FE generations in
         * bounded cohorts; still drain Cc/data/tree before bitmap and LVID. */
        Status = UDFWritebackNewVolumeFiles(IrpContext,Vcb);
        if (!NT_SUCCESS(Status))
            return Status;

        // NOTE: This function may also be invoked internally as part of
        // processing a shutdown request.
        ASSERT(Vcb->RootIndexFcb);
        ret_val |= UDFFlushADirectory(IrpContext, Vcb, Vcb->RootIndexFcb->FileInfo, &IoStatus, FlushFlags);
        if (!NT_SUCCESS(IoStatus.Status))
            return IoStatus.Status;

        if (Vcb->MetadataBitmapModified) {
            NTSTATUS metadataStatus = UDFFlushMetadataBitmap(IrpContext, Vcb);
            if (!NT_SUCCESS(metadataStatus))
                return metadataStatus;
        }

//        if (UDFFlushIsBreaking(Vcb, FlushFlags))
//            return;
        // flush internal cache
        if (FlushFlags & UDF_FLUSH_FLAGS_LITE) {
            UDFPrint(("  Lite flush, keep Modified=%d.\n", Vcb->Modified));
            if (Vcb->Cow) {
                Status = UDFCowCheckpoint(IrpContext, Vcb, FALSE);
                if (!NT_SUCCESS(Status))
                    return Status;
            }
        } else {
            /* Commit buffered metadata and return the disk to its home
             * layout before the volume is closed. */
            Status = UDFCowStop(IrpContext, Vcb);
            if (!NT_SUCCESS(Status))
                return Status;

            // umount (this is internal operation, NT will "dismount" volume later)
            Status = UDFUmount__(IrpContext, Vcb, &CloseDeferred);
            if (!NT_SUCCESS(Status))
                return Status;

            /* UDFUmount__ only issues its device barriers when the LVID or
             * bitmap changed. FE/FID/data sectors written above can still sit
             * in the lower device's volatile cache at shutdown/dismount, so
             * always finish a full flush with a barrier. */
            Status = UDFBarrierFlushDevice(Vcb->TargetDeviceObject);
            if (!NT_SUCCESS(Status))
                return Status;

            if (!CloseDeferred) {
                UDFPreClrModified(Vcb);
                UDFClrModified(Vcb);
            }

            /* A user flush keeps the volume in use: start again. Shutdown,
             * lock and dismount leave it off. */
            if (IrpContext->MajorFunction == IRP_MJ_FLUSH_BUFFERS)
                (VOID)UDFCowStart(IrpContext, Vcb);
        }

    } _SEH2_FINALLY {
        ;
    } _SEH2_END;

    return ret_val;
} // end UDFFlushLogicalVolume()


/*************************************************************************
*
* Function: UDFFlushCompletion()
*
* Description:
*   Eat up any bad errors.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: None
*
*************************************************************************/
NTSTATUS
NTAPI
UDFFlushCompletion(
    PDEVICE_OBJECT  PtrDeviceObject,
    PIRP            Irp,
    PVOID           Context
    )
{
    NTSTATUS Status = (NTSTATUS)(ULONG_PTR)Context;

    // Add the hack-o-ramma to fix formats.

    if (Irp->PendingReturned) {

        IoMarkIrpPending(Irp);
    }

    //  If the Irp got STATUS_INVALID_DEVICE_REQUEST or a warning status,
    //  normalize it to the original status value.

    if (!NT_SUCCESS(Status) || NT_WARNING(Irp->IoStatus.Status) ||
        Irp->IoStatus.Status == STATUS_INVALID_DEVICE_REQUEST) {

        Irp->IoStatus.Status = Status;
    }

    return STATUS_SUCCESS;
} // end UDFFlushCompletion()


/*
  Check if we should break FlushTree process
 */
BOOLEAN
UDFFlushIsBreaking(
    IN PVCB         Vcb,
    IN ULONG        FlushFlags
    )
{
    BOOLEAN ret_val = FALSE;
//    if (!(FlushFlags & UDF_FLUSH_FLAGS_BREAKABLE))
        return FALSE;
    UDFAcquireResourceExclusive(&(Vcb->FlushResource),TRUE);
    ret_val = (Vcb->VcbState & UDF_VCB_FLAGS_FLUSH_BREAK_REQ) ? TRUE : FALSE;
    Vcb->VcbState &= ~UDF_VCB_FLAGS_FLUSH_BREAK_REQ;
    UDFReleaseResource(&(Vcb->FlushResource));
    return ret_val;
} // end UDFFlushIsBreaking()

/*
  Signal FlushTree break request. Note, this is
  treated as recommendation only
 */
VOID
UDFFlushTryBreak(
    IN PVCB         Vcb
    )
{
    UDFAcquireResourceExclusive(&(Vcb->FlushResource),TRUE);
    Vcb->VcbState |= UDF_VCB_FLAGS_FLUSH_BREAK_REQ;
    UDFReleaseResource(&(Vcb->FlushResource));
} // end UDFFlushTryBreak()

//  Tell prefast this is a completion routine.
IO_COMPLETION_ROUTINE UDFHijackCompletionRoutine;

NTSTATUS
NTAPI
UDFHijackCompletionRoutine (
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Contxt
    )

/*++

Routine Description:

    Completion routine for synchronizing back to dispatch.

Arguments:

    Contxt - pointer to KEVENT.

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED

--*/

{
    PKEVENT Event = (PKEVENT)Contxt;
    _Analysis_assume_(Contxt != NULL);

    UNREFERENCED_PARAMETER( Irp );
    UNREFERENCED_PARAMETER( DeviceObject );

    KeSetEvent( Event, 0, FALSE );

    //  We don't want IO to get our IRP and free it.
    
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
UDFHijackIrpAndFlushDevice (
    _In_ PIRP_CONTEXT IrpContext,
    _Inout_ PIRP Irp,
    _In_ PDEVICE_OBJECT TargetDeviceObject
    )

/*++

Routine Description:

    This routine is called when we need to send a flush to a device but
    we don't have a flush Irp.  What this routine does is make a copy
    of its current Irp stack location, but changes the Irp Major code
    to a IRP_MJ_FLUSH_BUFFERS amd then send it down, but cut it off at
    the knees in the completion routine, fix it up and return to the
    user as if nothing had happened.

Arguments:

    Irp - The Irp to hijack

    TargetDeviceObject - The device to send the request to.

Return Value:

    NTSTATUS - The Status from the flush in case anybody cares.

--*/

{
    KEVENT Event;
    NTSTATUS Status;
    PIO_STACK_LOCATION NextIrpSp;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(IrpContext);
    
    // Get the next stack location, and copy over the stack location

    NextIrpSp = IoGetNextIrpStackLocation( Irp );

    *NextIrpSp = *IoGetCurrentIrpStackLocation( Irp );

    NextIrpSp->MajorFunction = IRP_MJ_FLUSH_BUFFERS;
    NextIrpSp->MinorFunction = 0;

    //  Set up the completion routine

    KeInitializeEvent( &Event, NotificationEvent, FALSE );

    IoSetCompletionRoutine(Irp,
                           &UDFHijackCompletionRoutine,
                           &Event,
                           TRUE,
                           TRUE,
                           TRUE);

    //  Send the request.

    Status = IoCallDriver(TargetDeviceObject, Irp);

    if (Status == STATUS_PENDING) {

        (VOID)KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);

        Status = Irp->IoStatus.Status;
    }

    // If the driver doesn't support flushes, return SUCCESS.

    if (Status == STATUS_INVALID_DEVICE_REQUEST) {

        Status = STATUS_SUCCESS;
    }

    Irp->IoStatus.Status = 0;
    Irp->IoStatus.Information = 0;

    return Status;
}
