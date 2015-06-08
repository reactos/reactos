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



/*************************************************************************
*
* Function: UDFFlush()
*
* Description:
*   The I/O Manager will invoke this routine to handle a flush buffers
*   request
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL (invocation at higher IRQL will cause execution
*   to be deferred to a worker thread context)
*
* Return Value: STATUS_SUCCESS/Error
*
*************************************************************************/
NTSTATUS
NTAPI
UDFFlush(
    PDEVICE_OBJECT      DeviceObject,       // the logical volume device object
    PIRP                Irp)                // I/O Request Packet
{
    NTSTATUS            RC = STATUS_SUCCESS;
    PtrUDFIrpContext    PtrIrpContext = NULL;
    BOOLEAN             AreWeTopLevel = FALSE;

    KdPrint(("UDFFlush: \n"));

    FsRtlEnterFileSystem();
    ASSERT(DeviceObject);
    ASSERT(Irp);

    // set the top level context
    AreWeTopLevel = UDFIsIrpTopLevel(Irp);
    ASSERT(!UDFIsFSDevObj(DeviceObject));

    _SEH2_TRY {

        // get an IRP context structure and issue the request
        PtrIrpContext = UDFAllocateIrpContext(Irp, DeviceObject);
        if(PtrIrpContext) {
            RC = UDFCommonFlush(PtrIrpContext, Irp);
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
} // end UDFFlush()



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
    PtrUDFIrpContext PtrIrpContext,
    PIRP             Irp
    )
{
    NTSTATUS            RC = STATUS_SUCCESS;
    PIO_STACK_LOCATION  IrpSp = NULL;
    PFILE_OBJECT        FileObject = NULL;
    PtrUDFFCB           Fcb = NULL;
    PtrUDFCCB           Ccb = NULL;
    PVCB                Vcb = NULL;
    PtrUDFNTRequiredFCB NtReqFcb = NULL;
    BOOLEAN             AcquiredVCB = FALSE;
    BOOLEAN             AcquiredFCB = FALSE;
    BOOLEAN             PostRequest = FALSE;
    BOOLEAN             CanWait = TRUE;

    KdPrint(("UDFCommonFlush: \n"));

    _SEH2_TRY {

        // Get some of the parameters supplied to us
        CanWait = ((PtrIrpContext->IrpContextFlags & UDF_IRP_CONTEXT_CAN_BLOCK) ? TRUE : FALSE);
        // If we cannot wait, post the request immediately since a flush is inherently blocking/synchronous.
        if (!CanWait) {
            PostRequest = TRUE;
            try_return(RC);
        }

        // First, get a pointer to the current I/O stack location
        IrpSp = IoGetCurrentIrpStackLocation(Irp);
        ASSERT(IrpSp);

        FileObject = IrpSp->FileObject;
        ASSERT(FileObject);

        // Get the FCB and CCB pointers
        Ccb = (PtrUDFCCB)(FileObject->FsContext2);
        ASSERT(Ccb);
        Fcb = Ccb->Fcb;
        ASSERT(Fcb);
        NtReqFcb = Fcb->NTRequiredFCB;

        // Check the type of object passed-in. That will determine the course of
        // action we take.
        if ((Fcb->NodeIdentifier.NodeType == UDF_NODE_TYPE_VCB) || (Fcb->FCBFlags & UDF_FCB_ROOT_DIRECTORY)) {

            if (Fcb->NodeIdentifier.NodeType == UDF_NODE_TYPE_VCB) {
                Vcb = (PVCB)(Fcb);
            } else {
                Vcb = Fcb->Vcb;
            }
            Vcb->VCBFlags |= UDF_VCB_SKIP_EJECT_CHECK;

#ifdef UDF_DELAYED_CLOSE
            UDFCloseAllDelayed(Vcb);
#endif //UDF_DELAYED_CLOSE

            UDFAcquireResourceExclusive(&(Vcb->VCBResource), TRUE);
            AcquiredVCB = TRUE;
            // The caller wishes to flush all files for the mounted
            // logical volume. The flush volume routine below should simply
            // walk through all of the open file streams, acquire the
            // VCB resource, and request the flush operation from the Cache
            // Manager. Basically, the sequence of operations listed below
            // for a single file should be executed on all open files.

            UDFFlushLogicalVolume(PtrIrpContext, Irp, Vcb, 0);

            UDFReleaseResource(&(Vcb->VCBResource));
            AcquiredVCB = FALSE;

            try_return(RC);
        } else
        if (!(Fcb->FCBFlags & UDF_FCB_DIRECTORY)) {
            // This is a regular file.
            Vcb = Fcb->Vcb;
            ASSERT(Vcb);
            if(!ExIsResourceAcquiredExclusiveLite(&(Vcb->VCBResource)) &&
               !ExIsResourceAcquiredSharedLite(&(Vcb->VCBResource))) {
                UDFAcquireResourceShared(&(Vcb->VCBResource), TRUE);
                AcquiredVCB = TRUE;
            }
            UDF_CHECK_PAGING_IO_RESOURCE(NtReqFcb);
            UDFAcquireResourceExclusive(&(NtReqFcb->MainResource), TRUE);
            AcquiredFCB = TRUE;

            // Request the Cache Manager to perform a flush operation.
            // Further, instruct the Cache Manager that we wish to flush the
            // entire file stream.
            UDFFlushAFile(Fcb, Ccb, &(Irp->IoStatus), 0);
            RC = Irp->IoStatus.Status;

            // Some log-based FSD implementations may wish to flush their
            // log files at this time. Finally, we should update the time-stamp
            // values for the file stream appropriately. This would involve
            // obtaining the current time and modifying the appropriate directory
            // entry fields.
        } else {
            Vcb = Fcb->Vcb;
        }

try_exit:   NOTHING;

    } _SEH2_FINALLY {

        if (AcquiredFCB) {
            UDF_CHECK_PAGING_IO_RESOURCE(NtReqFcb);
            UDFReleaseResource(&(NtReqFcb->MainResource));
            AcquiredFCB = FALSE;
        }
        if (AcquiredVCB) {
            UDFReleaseResource(&(Vcb->VCBResource));
            AcquiredVCB = FALSE;
        }

        if(!_SEH2_AbnormalTermination()) {
            if (PostRequest) {
                // Nothing to lock now.
                BrutePoint();
                RC = UDFPostRequest(PtrIrpContext, Irp);
            } else {
                // Some applications like this request very much
                // (ex. WinWord). But it's not a good idea for CD-R/RW media
                if(Vcb->FlushMedia) {
                    PIO_STACK_LOCATION      PtrNextIoStackLocation = NULL;
                    NTSTATUS                RC1 = STATUS_SUCCESS;

                    // Send the request down at this point.
                    // To do this, we must set the next IRP stack location, and
                    // maybe set a completion routine.
                    // Be careful about marking the IRP pending if the lower level
                    // driver returned pending and we do have a completion routine!
                    PtrNextIoStackLocation = IoGetNextIrpStackLocation(Irp);
                    *PtrNextIoStackLocation = *IrpSp;

                    // Set the completion routine to "eat-up" any
                    // STATUS_INVALID_DEVICE_REQUEST error code returned by the lower
                    // level driver.
                    IoSetCompletionRoutine(Irp, UDFFlushCompletion, NULL, TRUE, TRUE, TRUE);

                    RC1 = IoCallDriver(Vcb->TargetDeviceObject, Irp);

                    RC = ((RC1 == STATUS_INVALID_DEVICE_REQUEST) ? RC : RC1);

                    // Release the IRP context at this time.
                    UDFReleaseIrpContext(PtrIrpContext);
                } else {
                    Irp->IoStatus.Status = RC;
                    Irp->IoStatus.Information = 0;
                    // Free up the Irp Context
                    UDFReleaseIrpContext(PtrIrpContext);
                    // complete the IRP
                    IoCompleteRequest(Irp, IO_DISK_INCREMENT);
                }
            }
        }
    } _SEH2_END;

    return(RC);
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
    IN PtrUDFFCB           Fcb,
    IN PtrUDFCCB           Ccb,
    OUT PIO_STATUS_BLOCK   PtrIoStatus,
    IN ULONG               FlushFlags
    )
{
    BOOLEAN SetArchive = FALSE;
    BOOLEAN PurgeCache = FALSE;
    ULONG ret_val = 0;

    KdPrint(("UDFFlushAFile: \n"));
    if(!Fcb)
        return 0;

    _SEH2_TRY {
        if(Fcb->Vcb->VCBFlags & UDF_VCB_FLAGS_RAW_DISK)
            return 0;
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        BrutePoint();
    } _SEH2_END;
#ifndef UDF_READ_ONLY_BUILD
    // Flush Security if required
    _SEH2_TRY {
        UDFWriteSecurity(Fcb->Vcb, Fcb, &(Fcb->NTRequiredFCB->SecurityDesc));
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        BrutePoint();
    } _SEH2_END;
#endif //UDF_READ_ONLY_BUILD
    // Flush SDir if any
    _SEH2_TRY {
        if(UDFHasAStreamDir(Fcb->FileInfo) &&
           Fcb->FileInfo->Dloc->SDirInfo &&
           !UDFIsSDirDeleted(Fcb->FileInfo->Dloc->SDirInfo) ) {
            ret_val |=
                UDFFlushADirectory(Fcb->Vcb, Fcb->FileInfo->Dloc->SDirInfo, PtrIoStatus, FlushFlags);
        }
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        BrutePoint();
    } _SEH2_END;
    // Flush File
    _SEH2_TRY {
        if((Fcb->CachedOpenHandleCount || !Fcb->OpenHandleCount) &&
            Fcb->NTRequiredFCB->SectionObject.DataSectionObject) {
            if(!(Fcb->NTRequiredFCB->NtReqFCBFlags & UDF_NTREQ_FCB_DELETED)
                                         &&
                ((Fcb->NTRequiredFCB->NtReqFCBFlags & UDF_NTREQ_FCB_MODIFIED) ||
                 (Ccb && !(Ccb->CCBFlags & UDF_CCB_FLUSHED)) )) {
                MmPrint(("    CcFlushCache()\n"));
                CcFlushCache(&(Fcb->NTRequiredFCB->SectionObject), NULL, 0, PtrIoStatus);
            }
            // notice, that we should purge cache
            // we can't do it now, because it may cause last Close
            // request & thus, structure deallocation
            PurgeCache = TRUE;

#ifndef UDF_READ_ONLY_BUILD
            if(Ccb) {
                if( (Ccb->FileObject->Flags & FO_FILE_MODIFIED) &&
                   !(Ccb->CCBFlags & UDF_CCB_WRITE_TIME_SET)) {
                    if(Fcb->Vcb->CompatFlags & UDF_VCB_IC_UPDATE_MODIFY_TIME) {
                        LONGLONG NtTime;
                        KeQuerySystemTime((PLARGE_INTEGER)&NtTime);
                        UDFSetFileXTime(Fcb->FileInfo, NULL, NULL, NULL, &NtTime);
                        Fcb->NTRequiredFCB->LastWriteTime.QuadPart = NtTime;
                    }
                    SetArchive = TRUE;
                    Ccb->FileObject->Flags &= ~FO_FILE_MODIFIED;
                }
                if(Ccb->FileObject->Flags & FO_FILE_SIZE_CHANGED) {
                    LONGLONG ASize = UDFGetFileAllocationSize(Fcb->Vcb, Fcb->FileInfo);
                    UDFSetFileSizeInDirNdx(Fcb->Vcb, Fcb->FileInfo, &ASize);
                    Ccb->FileObject->Flags &= ~FO_FILE_SIZE_CHANGED;
                }
            }
#endif //UDF_READ_ONLY_BUILD
        }
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        BrutePoint();
    } _SEH2_END;

    _SEH2_TRY {
#ifndef UDF_READ_ONLY_BUILD
        if(SetArchive &&
           (Fcb->Vcb->CompatFlags & UDF_VCB_IC_UPDATE_ARCH_BIT)) {
            ULONG Attr;
            PDIR_INDEX_ITEM DirNdx;
            DirNdx = UDFDirIndex(UDFGetDirIndexByFileInfo(Fcb->FileInfo), Fcb->FileInfo->Index);
            // Archive bit
            Attr = UDFAttributesToNT(DirNdx, Fcb->FileInfo->Dloc->FileEntry);
            if(!(Attr & FILE_ATTRIBUTE_ARCHIVE))
                UDFAttributesToUDF(DirNdx, Fcb->FileInfo->Dloc->FileEntry, Attr | FILE_ATTRIBUTE_ARCHIVE);
        }
#endif //UDF_READ_ONLY_BUILD
        UDFFlushFile__( Fcb->Vcb, Fcb->FileInfo, FlushFlags);
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        BrutePoint();
    } _SEH2_END;

/*    if(PurgeCache) {
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
    IN PVCB                Vcb,
    IN PUDF_FILE_INFO      FI,
    OUT PIO_STATUS_BLOCK   PtrIoStatus,
    IN ULONG               FlushFlags
    )
{
    KdPrint(("UDFFlushADirectory: \n"));
    PDIR_INDEX_HDR hDI;
    PDIR_INDEX_ITEM DI;
//    BOOLEAN Referenced = FALSE;
    ULONG ret_val = 0;

    if(Vcb->VCBFlags & UDF_VCB_FLAGS_RAW_DISK)
        return 0;

    if(!FI || !FI->Dloc || !FI->Dloc->DirIndex) goto SkipFlushDir;
    hDI = FI->Dloc->DirIndex;

    // Flush Security if required
    _SEH2_TRY {
        UDFWriteSecurity(Vcb, FI->Fcb, &(FI->Fcb->NTRequiredFCB->SecurityDesc));
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        BrutePoint();
    } _SEH2_END;
    // Flush SDir if any
    _SEH2_TRY {
        if(UDFHasAStreamDir(FI) &&
           FI->Dloc->SDirInfo &&
           !UDFIsSDirDeleted(FI->Dloc->SDirInfo) ) {
            ret_val |=
                UDFFlushADirectory(Vcb, FI->Dloc->SDirInfo, PtrIoStatus, FlushFlags);
        }
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        BrutePoint();
    } _SEH2_END;

    // Flush Dir Tree
    _SEH2_TRY {
        UDF_DIR_SCAN_CONTEXT ScanContext;
        PUDF_FILE_INFO      tempFI;

        if(UDFDirIndexInitScan(FI, &ScanContext, 2)) {
            while(DI = UDFDirIndexScan(&ScanContext, &tempFI)) {
                // Flush Dir entry
                _SEH2_TRY {
                    if(!tempFI) continue;
                    if(UDFIsADirectory(tempFI)) {
                        UDFFlushADirectory(Vcb, tempFI, PtrIoStatus, FlushFlags);
                    } else {
                        UDFFlushAFile(tempFI->Fcb, NULL, PtrIoStatus, FlushFlags);
                    }
                } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
                    BrutePoint();
                } _SEH2_END;
                if(UDFFlushIsBreaking(Vcb, FlushFlags)) {
                    ret_val |= UDF_FLUSH_FLAGS_INTERRUPTED;
                    break;
                }
            }
        }
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        BrutePoint();
    } _SEH2_END;
SkipFlushDir:
    // Flush Dir
    _SEH2_TRY {
        UDFFlushFile__( Vcb, FI, FlushFlags );
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        BrutePoint();
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
ULONG
UDFFlushLogicalVolume(
    IN PtrUDFIrpContext PtrIrpContext,
    IN PIRP             Irp,
    IN PVCB             Vcb,
    IN ULONG            FlushFlags
    )
{
    ULONG ret_val = 0;
#ifndef UDF_READ_ONLY_BUILD
    IO_STATUS_BLOCK IoStatus;

    KdPrint(("UDFFlushLogicalVolume: \n"));

    _SEH2_TRY {
        if(Vcb->VCBFlags & (UDF_VCB_FLAGS_RAW_DISK/* |
                            UDF_VCB_FLAGS_MEDIA_READ_ONLY*/))
            return 0;
        if(Vcb->VCBFlags & UDF_VCB_FLAGS_VOLUME_READ_ONLY)
            return 0;
        if(!(Vcb->VCBFlags & UDF_VCB_FLAGS_VOLUME_MOUNTED))
            return 0;

        // NOTE: This function may also be invoked internally as part of
        // processing a shutdown request.
        ASSERT(Vcb->RootDirFCB);
        ret_val |= UDFFlushADirectory(Vcb, Vcb->RootDirFCB->FileInfo, &IoStatus, FlushFlags);

//        if(UDFFlushIsBreaking(Vcb, FlushFlags))
//            return;
        // flush internal cache
        if(FlushFlags & UDF_FLUSH_FLAGS_LITE) {
            KdPrint(("  Lite flush, keep Modified=%d.\n", Vcb->Modified));
        } else {
            if(Vcb->VerifyOnWrite) {
                KdPrint(("UDF: Flushing cache for verify\n"));
                //WCacheFlushAll__(&(Vcb->FastCache), Vcb);
                WCacheFlushBlocks__(&(Vcb->FastCache), Vcb, 0, Vcb->LastLBA);
                UDFVFlush(Vcb);
            }
            // umount (this is internal operation, NT will "dismount" volume later)
            UDFUmount__(Vcb);

            UDFPreClrModified(Vcb);
            WCacheFlushAll__(&(Vcb->FastCache), Vcb);
            UDFClrModified(Vcb);
        }

    } _SEH2_FINALLY {
        ;
    } _SEH2_END;
#endif //UDF_READ_ONLY_BUILD

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
//    NTSTATUS        RC = STATUS_SUCCESS;

    KdPrint(("UDFFlushCompletion: \n"));

    if (Irp->PendingReturned) {
        IoMarkIrpPending(Irp);
    }

    if (Irp->IoStatus.Status == STATUS_INVALID_DEVICE_REQUEST) {
        // cannot do much here, can we?
        Irp->IoStatus.Status = STATUS_SUCCESS;
    }

    return(STATUS_SUCCESS);
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
//    if(!(FlushFlags & UDF_FLUSH_FLAGS_BREAKABLE))
        return FALSE;
    UDFAcquireResourceExclusive(&(Vcb->FlushResource),TRUE);
    ret_val = (Vcb->VCBFlags & UDF_VCB_FLAGS_FLUSH_BREAK_REQ) ? TRUE : FALSE;
    Vcb->VCBFlags &= ~UDF_VCB_FLAGS_FLUSH_BREAK_REQ;
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
    Vcb->VCBFlags |= UDF_VCB_FLAGS_FLUSH_BREAK_REQ;
    UDFReleaseResource(&(Vcb->FlushResource));
} // end UDFFlushTryBreak()
