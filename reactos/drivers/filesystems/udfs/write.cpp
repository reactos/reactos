////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
////////////////////////////////////////////////////////////////////
/*************************************************************************
*
* File: Write.cpp
*
* Module: UDF File System Driver (Kernel mode execution only)
*
* Description:
*   Contains code to handle the "Write" dispatch entry point.
*
*************************************************************************/

#include            "udffs.h"

// define the file specific bug-check id
#define         UDF_BUG_CHECK_ID                UDF_FILE_WRITE

#ifndef UDF_READ_ONLY_BUILD

/*************************************************************************
*
* Function: UDFWrite()
*
* Description:
*   The I/O Manager will invoke this routine to handle a write
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
UDFWrite(
    PDEVICE_OBJECT DeviceObject,       // the logical volume device object
    PIRP           Irp                 // I/O Request Packet
    )
{
    NTSTATUS                RC = STATUS_SUCCESS;
    PtrUDFIrpContext        PtrIrpContext = NULL;
    BOOLEAN                 AreWeTopLevel = FALSE;

    TmPrint(("UDFWrite: , thrd:%8.8x\n",PsGetCurrentThread()));

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

            RC = UDFCommonWrite(PtrIrpContext, Irp);

        } else {
            RC = STATUS_INSUFFICIENT_RESOURCES;
            Irp->IoStatus.Status = RC;
            Irp->IoStatus.Information = 0;
            // complete the IRP
            IoCompleteRequest(Irp, IO_DISK_INCREMENT);
        }

    } _SEH2_EXCEPT (UDFExceptionFilter(PtrIrpContext, _SEH2_GetExceptionInformation())) {

        RC = UDFExceptionHandler(PtrIrpContext, Irp);

        UDFLogEvent(UDF_ERROR_INTERNAL_ERROR, RC);
    } _SEH2_END;

    if (AreWeTopLevel) {
        IoSetTopLevelIrp(NULL);
    }

    FsRtlExitFileSystem();

    return(RC);
} // end UDFWrite()


/*************************************************************************
*
* Function: UDFCommonWrite()
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
UDFCommonWrite(
    PtrUDFIrpContext PtrIrpContext,
    PIRP             Irp)
{
    NTSTATUS                RC = STATUS_SUCCESS;
    PIO_STACK_LOCATION      IrpSp = NULL;
    LARGE_INTEGER           ByteOffset;
    ULONG                   WriteLength = 0, TruncatedLength = 0;
    ULONG                   NumberBytesWritten = 0;
    PFILE_OBJECT            FileObject = NULL;
    PtrUDFFCB               Fcb = NULL;
    PtrUDFCCB               Ccb = NULL;
    PVCB                    Vcb = NULL;
    PtrUDFNTRequiredFCB     NtReqFcb = NULL;
    PERESOURCE              PtrResourceAcquired = NULL;
    PERESOURCE              PtrResourceAcquired2 = NULL;
    PVOID                   SystemBuffer = NULL;
//    PVOID                   TmpBuffer = NULL;
//    uint32                  KeyValue = 0;
    PIRP                    TopIrp;

    LONGLONG                ASize;
    LONGLONG                OldVDL;

    ULONG                   Res1Acq = 0;
    ULONG                   Res2Acq = 0;

    BOOLEAN                 CacheLocked = FALSE;

    BOOLEAN                 CanWait = FALSE;
    BOOLEAN                 PagingIo = FALSE;
    BOOLEAN                 NonBufferedIo = FALSE;
    BOOLEAN                 SynchronousIo = FALSE;
    BOOLEAN                 IsThisADeferredWrite = FALSE;
    BOOLEAN                 WriteToEOF = FALSE;
    BOOLEAN                 Resized = FALSE;
    BOOLEAN                 RecursiveWriteThrough = FALSE;
    BOOLEAN                 WriteFileSizeToDirNdx = FALSE;
    BOOLEAN                 ZeroBlock = FALSE;
    BOOLEAN                 VcbAcquired = FALSE;
    BOOLEAN                 ZeroBlockDone = FALSE;

    TmPrint(("UDFCommonWrite: irp %x\n", Irp));

    _SEH2_TRY {


        TopIrp = IoGetTopLevelIrp();

        switch((ULONG)TopIrp) {
        case FSRTL_FSP_TOP_LEVEL_IRP:
            KdPrint(("  FSRTL_FSP_TOP_LEVEL_IRP\n"));
            break;
        case FSRTL_CACHE_TOP_LEVEL_IRP:
            KdPrint(("  FSRTL_CACHE_TOP_LEVEL_IRP\n"));
            break;
        case FSRTL_MOD_WRITE_TOP_LEVEL_IRP:
            KdPrint(("  FSRTL_MOD_WRITE_TOP_LEVEL_IRP\n"));
            break;
        case FSRTL_FAST_IO_TOP_LEVEL_IRP:
            KdPrint(("  FSRTL_FAST_IO_TOP_LEVEL_IRP\n"));
            BrutePoint();
            break;
        case NULL:
            KdPrint(("  NULL TOP_LEVEL_IRP\n"));
            break;
        default:
            if(TopIrp == Irp) {
                KdPrint(("  TOP_LEVEL_IRP\n"));
            } else {
                KdPrint(("  RECURSIVE_IRP, TOP = %x\n", TopIrp));
            }
            break;
        }

        // First, get a pointer to the current I/O stack location
        IrpSp = IoGetCurrentIrpStackLocation(Irp);
        ASSERT(IrpSp);
        MmPrint(("    Enter Irp, MDL=%x\n", Irp->MdlAddress));
        if(Irp->MdlAddress) {
            UDFTouch(Irp->MdlAddress);
        }

        FileObject = IrpSp->FileObject;
        ASSERT(FileObject);

        // If this happens to be a MDL write complete request, then
        // allocated MDL can be freed. This may cause a recursive write
        // back into the FSD.
        if (IrpSp->MinorFunction & IRP_MN_COMPLETE) {
            // Caller wants to tell the Cache Manager that a previously
            // allocated MDL can be freed.
            UDFMdlComplete(PtrIrpContext, Irp, IrpSp, FALSE);
            // The IRP has been completed.
            try_return(RC = STATUS_SUCCESS);
        }

        // If this is a request at IRQL DISPATCH_LEVEL, then post the request
        if (IrpSp->MinorFunction & IRP_MN_DPC) {
            try_return(RC = STATUS_PENDING);
        }

        // Get the FCB and CCB pointers
        Ccb = (PtrUDFCCB)(FileObject->FsContext2);
        ASSERT(Ccb);
        Fcb = Ccb->Fcb;
        ASSERT(Fcb);
        Vcb = Fcb->Vcb;

        if(Fcb->FCBFlags & UDF_FCB_DELETED) {
            ASSERT(FALSE);
            try_return(RC = STATUS_TOO_LATE);
        }

        // is this operation allowed ?
        if(Vcb->VCBFlags & UDF_VCB_FLAGS_MEDIA_READ_ONLY) {
            try_return(RC = STATUS_ACCESS_DENIED);
        }
        Vcb->VCBFlags |= UDF_VCB_SKIP_EJECT_CHECK;

#ifdef EVALUATION_TIME_LIMIT
        // License Key check
        if(UDFGlobalData.UDFFlags & UDF_DATA_FLAGS_UNREGISTERED) {
            Vcb->VCBFlags |= UDF_VCB_FLAGS_VOLUME_READ_ONLY;
            try_return(RC = STATUS_ACCESS_DENIED);
        }
#endif //EVALUATION_TIME_LIMIT

        // Disk based file systems might decide to verify the logical volume
        //  (if required and only if removable media are supported) at this time
        // As soon as Tray is locked, we needn't call UDFVerifyVcb()

        ByteOffset = IrpSp->Parameters.Write.ByteOffset;

        CanWait = (PtrIrpContext->IrpContextFlags & UDF_IRP_CONTEXT_CAN_BLOCK) ? TRUE : FALSE;
        PagingIo = (Irp->Flags & IRP_PAGING_IO) ? TRUE : FALSE;
        NonBufferedIo = (Irp->Flags & IRP_NOCACHE) ? TRUE : FALSE;
        SynchronousIo = (FileObject->Flags & FO_SYNCHRONOUS_IO) ? TRUE : FALSE;
        KdPrint(("    Flags: %s; %s; %s; %s; Irp(W): %8.8x\n",
                      CanWait ? "Wt" : "nw", PagingIo ? "Pg" : "np",
                      NonBufferedIo ? "NBuf" : "buff", SynchronousIo ? "Snc" : "Asc",
                      Irp->Flags));

        NtReqFcb = Fcb->NTRequiredFCB;

        Res1Acq = UDFIsResourceAcquired(&(NtReqFcb->MainResource));
        if(!Res1Acq) {
            Res1Acq = PtrIrpContext->IrpContextFlags & UDF_IRP_CONTEXT_RES1_ACQ;
        }
        Res2Acq = UDFIsResourceAcquired(&(NtReqFcb->PagingIoResource));
        if(!Res2Acq) {
            Res2Acq = PtrIrpContext->IrpContextFlags & UDF_IRP_CONTEXT_RES2_ACQ;
        }

        if(!NonBufferedIo &&
           (Fcb->NodeIdentifier.NodeType != UDF_NODE_TYPE_VCB)) {
            if((Fcb->NodeIdentifier.NodeType != UDF_NODE_TYPE_VCB) &&
                UDFIsAStream(Fcb->FileInfo)) {
                UDFNotifyFullReportChange( Vcb, Fcb->FileInfo,
                                           FILE_NOTIFY_CHANGE_STREAM_WRITE,
                                           FILE_ACTION_MODIFIED_STREAM);
            } else {
                UDFNotifyFullReportChange( Vcb, Fcb->FileInfo,
                                           FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_LAST_ACCESS,
                                           FILE_ACTION_MODIFIED);
            }
        }

        // Get some of the parameters supplied to us
        WriteLength = IrpSp->Parameters.Write.Length;
        if (WriteLength == 0) {
            // a 0 byte write can be immediately succeeded
            if (SynchronousIo && !PagingIo && NT_SUCCESS(RC)) {
                // NT expects changing CurrentByteOffset to zero in this case
                FileObject->CurrentByteOffset.QuadPart = 0;
            }
            try_return(RC);
        }

        // If this is the normal file we have to check for
        // write access according to the current state of the file locks.
        if (!PagingIo &&
            !FsRtlCheckLockForWriteAccess( &(NtReqFcb->FileLock), Irp) ) {
                try_return( RC = STATUS_FILE_LOCK_CONFLICT );
        }

        // **********
        // Is this a write of the volume itself ?
        // **********
        if (Fcb->NodeIdentifier.NodeType == UDF_NODE_TYPE_VCB) {
            // Yup, we need to send this on to the disk driver after
            //  validation of the offset and length.
            Vcb = (PVCB)(Fcb);
            if(!CanWait)
                try_return(RC = STATUS_PENDING);
            // I dislike the idea of writing to not locked media
            if(!(Vcb->VCBFlags & UDF_VCB_FLAGS_VOLUME_LOCKED)) {
                try_return(RC = STATUS_ACCESS_DENIED);
            }

            if(PtrIrpContext->IrpContextFlags & UDF_IRP_CONTEXT_FLUSH2_REQUIRED) {

                KdPrint(("  UDF_IRP_CONTEXT_FLUSH2_REQUIRED\n"));
                PtrIrpContext->IrpContextFlags &= ~UDF_IRP_CONTEXT_FLUSH2_REQUIRED;

                if(!(Vcb->VCBFlags & UDF_VCB_FLAGS_RAW_DISK)) {
                    UDFCloseAllSystemDelayedInDir(Vcb, Vcb->RootDirFCB->FileInfo);
                }
#ifdef UDF_DELAYED_CLOSE
                UDFCloseAllDelayed(Vcb);
#endif //UDF_DELAYED_CLOSE

            }

            // Acquire the volume resource exclusive
            UDFAcquireResourceExclusive(&(Vcb->VCBResource), TRUE);
            PtrResourceAcquired = &(Vcb->VCBResource);

            // I dislike the idea of writing to mounted media too, but M$ has another point of view...
            if(Vcb->VCBFlags & UDF_VCB_FLAGS_VOLUME_MOUNTED) {
                // flush system cache
                UDFFlushLogicalVolume(NULL, NULL, Vcb, 0);
            }
#ifdef _MSC_VER
/* FIXME */
            if(PagingIo) {
                CollectStatistics(Vcb, MetaDataWrites);
                CollectStatisticsEx(Vcb, MetaDataWriteBytes, NumberBytesWritten);
            }
#endif
            // Forward the request to the lower level driver
            // Lock the callers buffer
            if (!NT_SUCCESS(RC = UDFLockCallersBuffer(PtrIrpContext, Irp, TRUE, WriteLength))) {
                try_return(RC);
            }
            SystemBuffer = UDFGetCallersBuffer(PtrIrpContext, Irp);
            if(!SystemBuffer)
                try_return(RC = STATUS_INVALID_USER_BUFFER);
            // Indicate, that volume contents can change after this operation
            // This flag will force VerifyVolume in future
            KdPrint(("  set UnsafeIoctl\n"));
            Vcb->VCBFlags |= UDF_VCB_FLAGS_UNSAFE_IOCTL;
            // Make sure, that volume will never be quick-remounted
            // It is very important for ChkUdf utility.
            Vcb->SerialNumber--;
            // Perform actual Write
            RC = UDFTWrite(Vcb, SystemBuffer, WriteLength,
                           (ULONG)(ByteOffset.QuadPart >> Vcb->BlockSizeBits),
                           &NumberBytesWritten);
            UDFUnlockCallersBuffer(PtrIrpContext, Irp, SystemBuffer);
            try_return(RC);
        }

        if(Vcb->VCBFlags & UDF_VCB_FLAGS_VOLUME_READ_ONLY) {
            try_return(RC = STATUS_ACCESS_DENIED);
        }

        // back pressure for very smart and fast system cache ;)
        if(!NonBufferedIo) {
            // cached IO
            if(Vcb->VerifyCtx.QueuedCount ||
               Vcb->VerifyCtx.ItemCount >= UDF_MAX_VERIFY_CACHE) {
                UDFVVerify(Vcb, UFD_VERIFY_FLAG_WAIT);
            }
        } else {
            if(Vcb->VerifyCtx.ItemCount > UDF_SYS_CACHE_STOP_THR) {
                UDFVVerify(Vcb, UFD_VERIFY_FLAG_WAIT);
            }
        }

        // The FSD (if it is a "nice" FSD) should check whether it is
        // convenient to allow the write to proceed by utilizing the
        // CcCanIWrite() function call. If it is not convenient to perform
        // the write at this time, we should defer the request for a while.
        // The check should not however be performed for non-cached write
        // operations. To determine whether we are retrying the operation
        // or now, use Flags in the IrpContext structure we have created

        IsThisADeferredWrite = (PtrIrpContext->IrpContextFlags & UDF_IRP_CONTEXT_DEFERRED_WRITE) ? TRUE : FALSE;

        if (!NonBufferedIo) {
            MmPrint(("    CcCanIWrite()\n"));
            if (!CcCanIWrite(FileObject, WriteLength, CanWait, IsThisADeferredWrite)) {
                // Cache Manager and/or the VMM does not want us to perform
                // the write at this time. Post the request.
                PtrIrpContext->IrpContextFlags |= UDF_IRP_CONTEXT_DEFERRED_WRITE;
                KdPrint(("UDFCommonWrite: Defer write\n"));
                MmPrint(("    CcDeferWrite()\n"));
                CcDeferWrite(FileObject, UDFDeferredWriteCallBack, PtrIrpContext, Irp, WriteLength, IsThisADeferredWrite);
                try_return(RC = STATUS_PENDING);
            }
        }

        // If the write request is directed to a page file,
        // send the request directly to the disk
        if (Fcb->FCBFlags & UDF_FCB_PAGE_FILE) {
            NonBufferedIo = TRUE;
        }

        // We can continue. Check whether this write operation is targeted
        // to a directory object in which case the UDF FSD will disallow
        // the write request.
        if (Fcb->FCBFlags & UDF_FCB_DIRECTORY) {
            RC = STATUS_INVALID_DEVICE_REQUEST;
            try_return(RC);
        }

        // Validate start offset and length supplied.
        // Here is a special check that determines whether the caller wishes to
        // begin the write at current end-of-file (whatever the value of that
        // offset might be)
        if(ByteOffset.HighPart == (LONG)0xFFFFFFFF) {
            if(ByteOffset.LowPart == FILE_WRITE_TO_END_OF_FILE) {
                WriteToEOF = TRUE;
                ByteOffset = NtReqFcb->CommonFCBHeader.FileSize;
            } else
            if(ByteOffset.LowPart == FILE_USE_FILE_POINTER_POSITION) {
                ByteOffset = FileObject->CurrentByteOffset;
            }
        }

        // Check if this volume has already been shut down.  If it has, fail
        // this write request.
        if (Vcb->VCBFlags & UDF_VCB_FLAGS_SHUTDOWN) {
            try_return(RC = STATUS_TOO_LATE);
        }

#ifdef EVALUATION_TIME_LIMIT
        // License Key check
        if(UDFGlobalData.UDFFlags & UDF_DATA_FLAGS_UNREGISTERED) {
            Vcb->VCBFlags |= UDF_VCB_FLAGS_VOLUME_READ_ONLY;
            try_return(RC = STATUS_ACCESS_DENIED);
        }
#endif //EVALUATION_TIME_LIMIT

        // Paging I/O write operations are special. If paging i/o write
        // requests begin beyond end-of-file, the request should be no-oped
        // If paging i/o
        // requests extend beyond current end of file, they should be truncated
        // to current end-of-file.
        if(PagingIo && (WriteToEOF || ((ByteOffset.QuadPart + WriteLength) > NtReqFcb->CommonFCBHeader.FileSize.QuadPart))) {
            if (ByteOffset.QuadPart > NtReqFcb->CommonFCBHeader.FileSize.QuadPart) {
                TruncatedLength = 0;
            } else {
                TruncatedLength = (ULONG)(NtReqFcb->CommonFCBHeader.FileSize.QuadPart - ByteOffset.QuadPart);
            }
            if(!TruncatedLength) try_return(RC = STATUS_SUCCESS);
        } else {
            TruncatedLength = WriteLength;
        }

#ifdef _MSC_VER
/* FIXME */
        if(PagingIo) {
            CollectStatistics(Vcb, UserFileWrites);
            CollectStatisticsEx(Vcb, UserFileWriteBytes, NumberBytesWritten);
        }
#endif

        // There are certain complications that arise when the same file stream
        // has been opened for cached and non-cached access. The FSD is then
        // responsible for maintaining a consistent view of the data seen by
        // the caller.
        // If this happens to be a non-buffered I/O, we should __try to flush the
        // cached data (if some other file object has already initiated caching
        // on the file stream). We should also __try to purge the cached
        // information though the purge will probably fail if the file has been
        // mapped into some process' virtual address space
        // WARNING !!! we should not flush data beyond valid data length
        if ( NonBufferedIo &&
            !PagingIo &&
             NtReqFcb->SectionObject.DataSectionObject && 
             TruncatedLength &&
             (ByteOffset.QuadPart < NtReqFcb->CommonFCBHeader.FileSize.QuadPart)) {

            if(!Res1Acq) {
                // Try to acquire the FCB MainResource exclusively
                if(!UDFAcquireResourceExclusive(&(NtReqFcb->MainResource), CanWait)) {
                    try_return(RC = STATUS_PENDING);
                }
                PtrResourceAcquired = &(NtReqFcb->MainResource);
            }

            if(!Res2Acq) {
                //  We hold PagingIo shared around the flush to fix a
                //  cache coherency problem.
                UDFAcquireSharedStarveExclusive(&(NtReqFcb->PagingIoResource), TRUE );
                PtrResourceAcquired2 = &(NtReqFcb->PagingIoResource);
            }

            // Flush and then attempt to purge the cache
            if((ByteOffset.QuadPart + TruncatedLength) > NtReqFcb->CommonFCBHeader.FileSize.QuadPart) {
                NumberBytesWritten = TruncatedLength;
            } else {
                NumberBytesWritten = (ULONG)(NtReqFcb->CommonFCBHeader.FileSize.QuadPart - ByteOffset.QuadPart);
            }

            MmPrint(("    CcFlushCache()\n"));
            CcFlushCache(&(NtReqFcb->SectionObject), &ByteOffset, NumberBytesWritten, &(Irp->IoStatus));

            if(PtrResourceAcquired2) {
                UDFReleaseResource(&(NtReqFcb->PagingIoResource));
                PtrResourceAcquired2 = NULL;
            }
            // If the flush failed, return error to the caller
            if (!NT_SUCCESS(RC = Irp->IoStatus.Status)) {
                NumberBytesWritten = 0;
                try_return(RC);
            }

            if(!Res2Acq) {
                // Acquiring and immediately dropping the resource serializes
                // us behind any other writes taking place (either from the
                // lazy writer or modified page writer).
                UDFAcquireResourceExclusive(&(NtReqFcb->PagingIoResource), TRUE );
                UDFReleaseResource(&(NtReqFcb->PagingIoResource));
            }

            // Attempt the purge and ignore the return code
            MmPrint(("    CcPurgeCacheSection()\n"));
            CcPurgeCacheSection(&(NtReqFcb->SectionObject), &ByteOffset,
                                        NumberBytesWritten, FALSE);
            NumberBytesWritten = 0;
            // We are finished with our flushing and purging
            if(PtrResourceAcquired) {
                UDFReleaseResource(PtrResourceAcquired);
                PtrResourceAcquired = NULL;
            }
        }

        // Determine if we were called by the lazywriter.
        // We reuse 'IsThisADeferredWrite' here to decrease stack usage
        IsThisADeferredWrite = (NtReqFcb->LazyWriterThreadID == (uint32)PsGetCurrentThread());

        // Acquire the appropriate FCB resource
        if(PagingIo) {
            // PagingIoResource is already acquired exclusive
            // on LazyWrite condition (see UDFAcqLazyWrite())
            ASSERT(NonBufferedIo);
            if(!IsThisADeferredWrite) {
                if(!Res2Acq) {
                    // Try to acquire the FCB PagingIoResource exclusive
                    if(!UDFAcquireResourceExclusive(&(NtReqFcb->PagingIoResource), CanWait)) {
                        try_return(RC = STATUS_PENDING);
                    }
                    // Remember the resource that was acquired
                    PtrResourceAcquired2 = &(NtReqFcb->PagingIoResource);
                }
            }
        } else {
            // Try to acquire the FCB MainResource shared
            if(NonBufferedIo) {
                if(!Res2Acq) {
                    if(!UDFAcquireResourceExclusive(&(NtReqFcb->PagingIoResource), CanWait)) {
                    //if(!UDFAcquireSharedWaitForExclusive(&(NtReqFcb->PagingIoResource), CanWait)) {
                        try_return(RC = STATUS_PENDING);
                    }
                    PtrResourceAcquired2 = &(NtReqFcb->PagingIoResource);
                }
            } else {
                if(!Res1Acq) {
                    UDF_CHECK_PAGING_IO_RESOURCE(NtReqFcb);
                    if(!UDFAcquireResourceExclusive(&(NtReqFcb->MainResource), CanWait)) {
                    //if(!UDFAcquireResourceShared(&(NtReqFcb->MainResource), CanWait)) {
                        try_return(RC = STATUS_PENDING);
                    }
                    PtrResourceAcquired = &(NtReqFcb->MainResource);
                }
            }
            // Remember the resource that was acquired
        }

        //  Set the flag indicating if Fast I/O is possible
        NtReqFcb->CommonFCBHeader.IsFastIoPossible = UDFIsFastIoPossible(Fcb);
/*        if(NtReqFcb->CommonFCBHeader.IsFastIoPossible == FastIoIsPossible) {
            NtReqFcb->CommonFCBHeader.IsFastIoPossible = FastIoIsQuestionable;
        }*/

        if ( (Irp->Flags & IRP_SYNCHRONOUS_PAGING_IO) &&
             (PtrIrpContext->IrpContextFlags & UDF_IRP_CONTEXT_NOT_TOP_LEVEL)) {

            //  This clause determines if the top level request was
            //  in the FastIo path.
            if ((ULONG)TopIrp > FSRTL_MAX_TOP_LEVEL_IRP_FLAG) {

                PIO_STACK_LOCATION IrpStack;
                ASSERT( TopIrp->Type == IO_TYPE_IRP );
                IrpStack = IoGetCurrentIrpStackLocation(TopIrp);

                //  Finally this routine detects if the Top irp was a
                //  write to this file and thus we are the writethrough.
                if ((IrpStack->MajorFunction == IRP_MJ_WRITE) &&
                    (IrpStack->FileObject->FsContext == FileObject->FsContext)) {

                    RecursiveWriteThrough = TRUE;
                    PtrIrpContext->IrpContextFlags |= UDF_IRP_CONTEXT_WRITE_THROUGH;
                }
            }
        }

        //  Here is the deal with ValidDataLength and FileSize:
        //
        //  Rule 1: PagingIo is never allowed to extend file size.
        //
        //  Rule 2: Only the top level requestor may extend Valid
        //          Data Length.  This may be paging IO, as when a
        //          a user maps a file, but will never be as a result
        //          of cache lazy writer writes since they are not the
        //          top level request.
        //
        //  Rule 3: If, using Rules 1 and 2, we decide we must extend
        //          file size or valid data, we take the Fcb exclusive.

        // Check whether the current request will extend the file size,
        // or the valid data length (if the FSD supports the concept of a
        // valid data length associated with the file stream). In either case,
        // inform the Cache Manager at this time using CcSetFileSizes() about
        // the new file length. Note that real FSD implementations will have to
        // first allocate enough on-disk space at this point (before they
        // inform the Cache Manager about the new size) to ensure that the write
        // will subsequently not fail due to lack of disk space.

        OldVDL = NtReqFcb->CommonFCBHeader.ValidDataLength.QuadPart;
        ZeroBlock = (ByteOffset.QuadPart > OldVDL);

        if (!PagingIo &&
            !RecursiveWriteThrough &&
            !IsThisADeferredWrite) {

            BOOLEAN ExtendFS;

            ExtendFS = (ByteOffset.QuadPart + TruncatedLength > NtReqFcb->CommonFCBHeader.FileSize.QuadPart);

            if( WriteToEOF || ZeroBlock || ExtendFS) {
                // we are extending the file;

                if(!CanWait)
                    try_return(RC = STATUS_PENDING);
//                CanWait = TRUE;
                // Release any resources acquired above ...
                if (PtrResourceAcquired2) {
                    UDFReleaseResource(PtrResourceAcquired2);
                    PtrResourceAcquired2 = NULL;
                }
                if (PtrResourceAcquired) {
                    UDFReleaseResource(PtrResourceAcquired);
                    PtrResourceAcquired = NULL;
                }
                if(!UDFAcquireResourceShared(&(Vcb->VCBResource), CanWait)) {
                    try_return(RC = STATUS_PENDING);
                }
                VcbAcquired = TRUE;
                if(!Res1Acq) {
                    // Try to acquire the FCB MainResource exclusively
                    UDF_CHECK_PAGING_IO_RESOURCE(NtReqFcb);
                    if(!UDFAcquireResourceExclusive(&(NtReqFcb->MainResource), CanWait)) {
                        try_return(RC = STATUS_PENDING);
                    }
                    // Remember the resource that was acquired
                    PtrResourceAcquired = &(NtReqFcb->MainResource);
                }

                if(!Res2Acq) {
                    // allocate space...
                    AdPrint(("      Try to acquire PagingIoRes\n"));
                    UDFAcquireResourceExclusive(&(NtReqFcb->PagingIoResource), TRUE );
                    PtrResourceAcquired2 = &(NtReqFcb->PagingIoResource);
                }
                AdPrint(("      PagingIoRes Ok, Resizing...\n"));

                if(ExtendFS) {
                    RC = UDFResizeFile__(Vcb, Fcb->FileInfo, ByteOffset.QuadPart + TruncatedLength);

                    if(!NT_SUCCESS(RC)) {
                        if(PtrResourceAcquired2) {
                            UDFReleaseResource(&(NtReqFcb->PagingIoResource));
                            PtrResourceAcquired2 = NULL;
                        }
                        try_return(RC);
                    }
                    Resized = TRUE;
                    // ... and inform the Cache Manager about it
                    NtReqFcb->CommonFCBHeader.FileSize.QuadPart = ByteOffset.QuadPart + TruncatedLength;
                    NtReqFcb->CommonFCBHeader.AllocationSize.QuadPart = UDFGetFileAllocationSize(Vcb, Fcb->FileInfo);
                    if(!Vcb->LowFreeSpace) {
                         NtReqFcb->CommonFCBHeader.AllocationSize.QuadPart += (PAGE_SIZE*9-1);
                    } else {
                         NtReqFcb->CommonFCBHeader.AllocationSize.QuadPart += (PAGE_SIZE-1);
                    }
                    NtReqFcb->CommonFCBHeader.AllocationSize.LowPart &= ~(PAGE_SIZE-1);
                }

                KdPrint(("UDFCommonWrite: Set size %x (alloc size %x)\n", ByteOffset.LowPart + TruncatedLength, NtReqFcb->CommonFCBHeader.AllocationSize.LowPart));
                if (CcIsFileCached(FileObject)) {
                    if(ExtendFS) {
                        MmPrint(("    CcSetFileSizes()\n"));
                        CcSetFileSizes(FileObject, (PCC_FILE_SIZES)&(NtReqFcb->CommonFCBHeader.AllocationSize));
                        NtReqFcb->NtReqFCBFlags |= UDF_NTREQ_FCB_MODIFIED;
                    }
                    // Attempt to Zero newly added fragment
                    // and ignore the return code
                    // This should be done to inform cache manager
                    // that given extent has no cached data
                    // (Otherwise, CM sometimes thinks that it has)
                    if(ZeroBlock) {
                        NtReqFcb->NtReqFCBFlags |= UDF_NTREQ_FCB_MODIFIED;
                        ThPrint(("    UDFZeroDataEx(1)\n"));
                        UDFZeroDataEx(NtReqFcb,
                                      OldVDL,
                                      /*ByteOffset.QuadPart*/ NtReqFcb->CommonFCBHeader.FileSize.QuadPart - OldVDL,
                                      CanWait, Vcb, FileObject);
#ifdef UDF_DBG
                        ZeroBlockDone = TRUE;
#endif //UDF_DBG
                    }
                }
                if (PtrResourceAcquired2) {
                    UDFReleaseResource(PtrResourceAcquired2);
                    PtrResourceAcquired2 = NULL;
                }

                // Inform any pending IRPs (notify change directory).
                if(UDFIsAStream(Fcb->FileInfo)) {
                    UDFNotifyFullReportChange( Vcb, Fcb->FileInfo,
                                               FILE_NOTIFY_CHANGE_STREAM_SIZE,
                                               FILE_ACTION_MODIFIED_STREAM);
                } else {
                    UDFNotifyFullReportChange( Vcb, Fcb->FileInfo,
                                               FILE_NOTIFY_CHANGE_SIZE,
                                               FILE_ACTION_MODIFIED);
                }
            }

        }

#ifdef UDF_DISABLE_SYSTEM_CACHE_MANAGER
        NonBufferedIo = TRUE;
#endif
        if(Fcb && Fcb->FileInfo && Fcb->FileInfo->Dloc) {
            AdPrint(("UDFCommonWrite: DataLoc %x, Mapping %x\n", Fcb->FileInfo->Dloc->DataLoc, Fcb->FileInfo->Dloc->DataLoc.Mapping));
        }

        //  Branch here for cached vs non-cached I/O
        if (!NonBufferedIo) {

            // The caller wishes to perform cached I/O. Initiate caching if
            // this is the first cached I/O operation using this file object
            if (!FileObject->PrivateCacheMap) {
                // This is the first cached I/O operation. You must ensure
                // that the FCB Common FCB Header contains valid sizes at this time
                KdPrint(("UDFCommonWrite: Init system cache\n"));
                MmPrint(("    CcInitializeCacheMap()\n"));
                CcInitializeCacheMap(FileObject, (PCC_FILE_SIZES)(&(NtReqFcb->CommonFCBHeader.AllocationSize)),
                    FALSE,      // We will not utilize pin access for this file
                    &(UDFGlobalData.CacheMgrCallBacks), // callbacks
                    NtReqFcb);       // The context used in callbacks
                MmPrint(("    CcSetReadAheadGranularity()\n"));
                CcSetReadAheadGranularity(FileObject, Vcb->SystemCacheGran);

            }

            if(ZeroBlock && !ZeroBlockDone) {
                ThPrint(("    UDFZeroDataEx(2)\n"));
                UDFZeroDataEx(NtReqFcb,
                              OldVDL,
                              /*ByteOffset.QuadPart*/ ByteOffset.QuadPart + TruncatedLength - OldVDL,
                              CanWait, Vcb, FileObject);
                if(ByteOffset.LowPart & (PAGE_SIZE-1)) {
                }
            }

            WriteFileSizeToDirNdx = (PtrIrpContext->IrpContextFlags & UDF_IRP_CONTEXT_WRITE_THROUGH) ?
                                    TRUE : FALSE;
            // Check and see if this request requires a MDL returned to the caller
            if (IrpSp->MinorFunction & IRP_MN_MDL) {
                // Caller does want a MDL returned. Note that this mode
                // implies that the caller is prepared to block
                MmPrint(("    CcPrepareMdlWrite()\n"));
//                CcPrepareMdlWrite(FileObject, &ByteOffset, TruncatedLength, &(Irp->MdlAddress), &(Irp->IoStatus));
//                NumberBytesWritten = Irp->IoStatus.Information;
//                RC = Irp->IoStatus.Status;

                NumberBytesWritten = 0;
                RC = STATUS_INVALID_PARAMETER;

                try_return(RC);
            }

            if(NtReqFcb->SectionObject.DataSectionObject &&
               TruncatedLength >= 0x10000 &&
               ByteOffset.LowPart &&
               !(ByteOffset.LowPart & 0x00ffffff)) {

                //if(WinVer_Id() < WinVer_2k) {
                    //LARGE_INTEGER flush_offs;
                    //flush_offs.QuadPart = ByteOffset.QuadPart - 0x100*0x10000;
                    MmPrint(("    CcFlushCache() 16Mb\n"));
                    //CcFlushCache(&(NtReqFcb->SectionObject), &ByteOffset, 0x100*0x10000, &(Irp->IoStatus));

                    // there was a nice idea: flush just previous part. But it doesn't work
                    CcFlushCache(&(NtReqFcb->SectionObject), NULL, 0, &(Irp->IoStatus));
                //}
            }

            // This is a regular run-of-the-mill cached I/O request. Let the
            // Cache Manager worry about it!
            // First though, we need a buffer pointer (address) that is valid

            // We needn't call CcZeroData 'cause udf_info.cpp will care about it
            SystemBuffer = UDFGetCallersBuffer(PtrIrpContext, Irp);
            if(!SystemBuffer)
                try_return(RC = STATUS_INVALID_USER_BUFFER);
            ASSERT(SystemBuffer);
            NtReqFcb->NtReqFCBFlags |= UDF_NTREQ_FCB_MODIFIED;
            PerfPrint(("UDFCommonWrite: CcCopyWrite %x bytes at %x\n", TruncatedLength, ByteOffset.LowPart));
            MmPrint(("    CcCopyWrite()\n"));
            if(!CcCopyWrite(FileObject, &(ByteOffset), TruncatedLength, CanWait, SystemBuffer)) {
                // The caller was not prepared to block and data is not immediately
                // available in the system cache
                // Mark Irp Pending ...
                try_return(RC = STATUS_PENDING);
            }

            UDFUnlockCallersBuffer(PtrIrpContext, Irp, SystemBuffer);
            // We have the data
            RC = STATUS_SUCCESS;
            NumberBytesWritten = TruncatedLength;

            try_return(RC);

        } else {

            MmPrint(("    Write NonBufferedIo\n"));

            // We needn't call CcZeroData here (like in Fat driver)
            // 'cause we've already done it above
            // (see call to UDFZeroDataEx() )
            if (!RecursiveWriteThrough &&
                !IsThisADeferredWrite &&
                (OldVDL < ByteOffset.QuadPart)) {
#ifdef UDF_DBG
                    ASSERT(!ZeroBlockDone);
#endif //UDF_DBG
                    UDFZeroDataEx(NtReqFcb,
                                 OldVDL,
                                 /*ByteOffset.QuadPart*/ ByteOffset.QuadPart - OldVDL,
                                 CanWait, Vcb, FileObject);
            }
            if(OldVDL < (ByteOffset.QuadPart + TruncatedLength)) {
                NtReqFcb->CommonFCBHeader.ValidDataLength.QuadPart = ByteOffset.QuadPart + TruncatedLength;
            }

#if 1
            if((ULONG)TopIrp == FSRTL_MOD_WRITE_TOP_LEVEL_IRP) {
                KdPrint(("FSRTL_MOD_WRITE_TOP_LEVEL_IRP => CanWait\n"));
                CanWait = TRUE;
            } else
            if((ULONG)TopIrp == FSRTL_CACHE_TOP_LEVEL_IRP) {
                KdPrint(("FSRTL_CACHE_TOP_LEVEL_IRP => CanWait\n"));
                CanWait = TRUE;
            }

            if(NtReqFcb->AcqSectionCount || NtReqFcb->AcqFlushCount) {
                MmPrint(("    AcqCount (%d/%d)=> CanWait ?\n", NtReqFcb->AcqSectionCount, NtReqFcb->AcqFlushCount));
                CanWait = TRUE;
            } else
            {}
/*            if((TopIrp != Irp)) {
                KdPrint(("(TopIrp != Irp) => CanWait\n"));
                CanWait = TRUE;
            } else*/
#endif
            if(KeGetCurrentIrql() > PASSIVE_LEVEL) {
                MmPrint(("    !PASSIVE_LEVEL\n"));
                CanWait = FALSE;
                PtrIrpContext->IrpContextFlags |= UDF_IRP_CONTEXT_FORCED_POST;
            }
            // Successful check will cause WCache lock
            if(!CanWait && UDFIsFileCached__(Vcb, Fcb->FileInfo, ByteOffset.QuadPart, TruncatedLength, TRUE)) {
                KdPrint(("UDFCommonWrite: Cached => CanWait\n"));
                CacheLocked = TRUE;
                CanWait = TRUE;
            }
            // Send the request to lower level drivers
            if(!CanWait) {
                KdPrint(("UDFCommonWrite: Post physical write %x bytes at %x\n", TruncatedLength, ByteOffset.LowPart));

                try_return(RC = STATUS_PENDING);
            }

            if(!Res2Acq) {
                if(UDFAcquireResourceExclusiveWithCheck(&(NtReqFcb->PagingIoResource))) {
                    PtrResourceAcquired2 = &(NtReqFcb->PagingIoResource);
                }
            }

            PerfPrint(("UDFCommonWrite: Physical write %x bytes at %x\n", TruncatedLength, ByteOffset.LowPart));

            // Lock the callers buffer
            if (!NT_SUCCESS(RC = UDFLockCallersBuffer(PtrIrpContext, Irp, TRUE, TruncatedLength))) {
                try_return(RC);
            }

            SystemBuffer = UDFGetCallersBuffer(PtrIrpContext, Irp);
            if(!SystemBuffer) {
                try_return(RC = STATUS_INVALID_USER_BUFFER);
            }
            NtReqFcb->NtReqFCBFlags |= UDF_NTREQ_FCB_MODIFIED;
            RC = UDFWriteFile__(Vcb, Fcb->FileInfo, ByteOffset.QuadPart, TruncatedLength,
                           CacheLocked, (PCHAR)SystemBuffer, &NumberBytesWritten);

            UDFUnlockCallersBuffer(PtrIrpContext, Irp, SystemBuffer);

#ifdef _MSC_VER
/* FIXME */
            if(PagingIo) {
                CollectStatistics(Vcb, UserDiskWrites);
            } else {
                CollectStatistics2(Vcb, NonCachedDiskWrites);
            }
#endif
            WriteFileSizeToDirNdx = TRUE;

            try_return(RC);
        }

try_exit:   NOTHING;

    } _SEH2_FINALLY {

        if(CacheLocked) {
            WCacheEODirect__(&(Vcb->FastCache), Vcb);
        }

        // Release any resources acquired here ...
        if(PtrResourceAcquired2) {
            UDFReleaseResource(PtrResourceAcquired2);
        }
        if(PtrResourceAcquired) {
            if(NtReqFcb &&
               (PtrResourceAcquired ==
                &(NtReqFcb->MainResource))) {
                UDF_CHECK_PAGING_IO_RESOURCE(NtReqFcb);
            }
            UDFReleaseResource(PtrResourceAcquired);
        }
        if(VcbAcquired) {
            UDFReleaseResource(&(Vcb->VCBResource));
        }

        // Post IRP if required
        if(RC == STATUS_PENDING) {

            // Lock the callers buffer here. Then invoke a common routine to
            // perform the post operation.
            if (!(IrpSp->MinorFunction & IRP_MN_MDL)) {
                RC = UDFLockCallersBuffer(PtrIrpContext, Irp, FALSE, WriteLength);
                ASSERT(NT_SUCCESS(RC));
            }
            if(PagingIo) {
                if(Res1Acq) {
                    PtrIrpContext->IrpContextFlags |= UDF_IRP_CONTEXT_RES1_ACQ;
                }
                if(Res2Acq) {
                    PtrIrpContext->IrpContextFlags |= UDF_IRP_CONTEXT_RES2_ACQ;
                }
            }

            // Perform the post operation which will mark the IRP pending
            // and will return STATUS_PENDING back to us
            RC = UDFPostRequest(PtrIrpContext, Irp);

        } else {
            // For synchronous I/O, the FSD must maintain the current byte offset
            // Do not do this however, if I/O is marked as paging-io
            if (SynchronousIo && !PagingIo && NT_SUCCESS(RC)) {
                FileObject->CurrentByteOffset.QuadPart = ByteOffset.QuadPart + NumberBytesWritten;
            }
            // If the write completed successfully and this was not a paging-io
            // operation, set a flag in the CCB that indicates that a write was
            // performed and that the file time should be updated at cleanup
            if (NT_SUCCESS(RC) && !PagingIo) {
                Ccb->CCBFlags |= UDF_CCB_MODIFIED;
                // If the file size was changed, set a flag in the FCB indicating that
                // this occurred.
                FileObject->Flags |= FO_FILE_MODIFIED;
                if(Resized) {
                    if(!WriteFileSizeToDirNdx) {
                        FileObject->Flags |= FO_FILE_SIZE_CHANGED;
                    } else {
                        ASize = UDFGetFileAllocationSize(Vcb, Fcb->FileInfo);
                        UDFSetFileSizeInDirNdx(Vcb, Fcb->FileInfo, &ASize);
                    }
                }
                // Update ValidDataLength
                if(!IsThisADeferredWrite &&
                   NtReqFcb) {
                    if(NtReqFcb->CommonFCBHeader.ValidDataLength.QuadPart < (ByteOffset.QuadPart + NumberBytesWritten)) {

                        NtReqFcb->CommonFCBHeader.ValidDataLength.QuadPart =
                            min(NtReqFcb->CommonFCBHeader.FileSize.QuadPart,
                                ByteOffset.QuadPart + NumberBytesWritten);
                    }
                }
            }

            // If the request failed, and we had done some nasty stuff like
            // extending the file size (including informing the Cache Manager
            // about the new file size), and allocating on-disk space etc., undo
            // it at this time.

            // Can complete the IRP here if no exception was encountered
            if(!_SEH2_AbnormalTermination() &&
               Irp) {
                Irp->IoStatus.Status = RC;
                Irp->IoStatus.Information = NumberBytesWritten;
                // complete the IRP
                MmPrint(("    Complete Irp, MDL=%x\n", Irp->MdlAddress));
                if(Irp->MdlAddress) {
                    UDFTouch(Irp->MdlAddress);
                }
                IoCompleteRequest(Irp, IO_DISK_INCREMENT);
            }
            // Free up the Irp Context
            UDFReleaseIrpContext(PtrIrpContext);

        } // can we complete the IRP ?
    } _SEH2_END; // end of "__finally" processing

    KdPrint(("\n"));
    return(RC);
} // end UDFCommonWrite()

/*************************************************************************
*
* Function: UDFDeferredWriteCallBack()
*
* Description:
*   Invoked by the cache manager in the context of a worker thread.
*   Typically, you can simply post the request at this point (just
*   as you would have if the original request could not block) to
*   perform the write in the context of a system worker thread.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: None
*
*************************************************************************/
VOID
NTAPI
UDFDeferredWriteCallBack(
    IN PVOID Context1,          // Should be PtrIrpContext
    IN PVOID Context2           // Should be Irp
    )          
{
    KdPrint(("UDFDeferredWriteCallBack\n"));
    // We should typically simply post the request to our internal
    // queue of posted requests (just as we would if the original write
    // could not be completed because the caller could not block).
    // Once we post the request, return from this routine. The write
    // will then be retried in the context of a system worker thread
    UDFPostRequest((PtrUDFIrpContext)Context1, (PIRP)Context2);

} // end UDFDeferredWriteCallBack()

/*************************************************************************
*
*************************************************************************/

#define USE_CcCopyWrite_TO_ZERO

VOID
UDFPurgeCacheEx_(
    PtrUDFNTRequiredFCB NtReqFcb,
    LONGLONG            Offset,
    LONGLONG            Length,
//#ifndef ALLOW_SPARSE
    BOOLEAN             CanWait,
//#endif //ALLOW_SPARSE
    PVCB                Vcb,
    PFILE_OBJECT        FileObject
    )
{
    ULONG Off_l;
#ifdef USE_CcCopyWrite_TO_ZERO
    ULONG PgLen;
#endif //USE_CcCopyWrite_TO_ZERO

    // We'll just purge cache section here,
    // without call to CcZeroData()
    // 'cause udf_info.cpp will care about it

#define PURGE_BLOCK_SZ 0x10000000

    // NOTE: if FS engine doesn't suport
    // sparse/unrecorded areas, CcZeroData must be called
    // In this case we'll see some recursive WRITE requests

    _SEH2_TRY {
        MmPrint(("    UDFPurgeCacheEx_():  Offs: %I64x, ", Offset));
        MmPrint((" Len: %lx\n", Length));
        SECTION_OBJECT_POINTERS* SectionObject = &(NtReqFcb->SectionObject);
        if(Length) {
            LONGLONG Offset0, OffsetX, VDL;

            Offset0 = Offset;
            if((Off_l = ((ULONG)Offset0 & (PAGE_SIZE-1)))) {
                //                 Offset, Offset0
                //                 v
                // ...|dddddddddddd00000|....
                //    |<- Off_l ->|
#ifndef USE_CcCopyWrite_TO_ZERO
                *((PULONG)&Offset0) &= ~(PAGE_SIZE-1);
                MmPrint(("    CcFlushCache(s) Offs %I64x, Len %x\n", Offset0, Off_l));
                CcFlushCache( SectionObject, (PLARGE_INTEGER)&Offset0, Off_l, NULL ); 
#else //USE_CcCopyWrite_TO_ZERO
                // ...|ddddd000000000000|....
                //          |<- PgLen ->|
                PgLen = PAGE_SIZE - Off_l; /*(*((PULONG)&Offset) & (PAGE_SIZE-1))*/
                // 
                if(PgLen > Length)
                    PgLen = (ULONG)Length;

                MmPrint(("    ZeroCache (CcWrite) Offs %I64x, Len %x\n", Offset, PgLen));
#ifdef DBG
                if(FileObject && Vcb) {
                    
                    ASSERT(CanWait);
#endif //DBG
                    if (PgLen) {
                        if (SectionObject->SharedCacheMap) {
                            CcCopyWrite(FileObject, (PLARGE_INTEGER)&Offset, PgLen, TRUE || CanWait, Vcb->ZBuffer);
                        }
                        Offset += PgLen;
                        Length -= PgLen;
                    }
#ifdef DBG
                } else {
                    MmPrint(("    Can't use CcWrite to zero cache\n"));
                }
#endif //DBG 
#endif //USE_CcCopyWrite_TO_ZERO
            }
            VDL = NtReqFcb->CommonFCBHeader.ValidDataLength.QuadPart;
            OffsetX = Offset+Length;
            if((Off_l = ((ULONG)OffsetX & (PAGE_SIZE-1)))) {

                if(OffsetX < VDL) {
#ifndef USE_CcCopyWrite_TO_ZERO
                    Off_l = ( (ULONG)(VDL-OffsetX) > PAGE_SIZE ) ?
                        (PAGE_SIZE - Off_l) :
                        ((ULONG)(VDL-OffsetX));
                    *((PULONG)&OffsetX) &= ~(PAGE_SIZE-1);
                    MmPrint(("    CcFlushCache(e) Offs %I64x, Len %x\n", OffsetX, Off_l));
                    CcFlushCache( SectionObject, (PLARGE_INTEGER)&OffsetX, Off_l, NULL );
#else //USE_CcCopyWrite_TO_ZERO
                    if(VDL - OffsetX > PAGE_SIZE) {
                        PgLen = (ULONG)OffsetX & ~(PAGE_SIZE-1);
                    } else {
                        PgLen = (ULONG)(VDL - OffsetX) & ~(PAGE_SIZE-1);
                    }
                    // ...|000000000000ddddd|....
                    //    |<- PgLen ->|
                    MmPrint(("    ZeroCache (CcWrite - 2) Offs %I64x, Len %x\n", OffsetX, PgLen));
#ifdef DBG
                    if(FileObject && Vcb) {
                        ASSERT(CanWait);
#endif //DBG
                        if (SectionObject->SharedCacheMap) {
                            CcCopyWrite(FileObject, (PLARGE_INTEGER)&OffsetX, PgLen, TRUE || CanWait, Vcb->ZBuffer);
                        }
                        Length -= PgLen;
#ifdef DBG
                    } else {
                        MmPrint(("    Can't use CcWrite to zero cache (2)\n"));
                    }
#endif //DBG
#endif //USE_CcCopyWrite_TO_ZERO
                }
            }
#ifndef USE_CcCopyWrite_TO_ZERO
            do
#else //USE_CcCopyWrite_TO_ZERO
            while(Length)
#endif //USE_CcCopyWrite_TO_ZERO
            {
                MmPrint(("    CcPurgeCacheSection()\n"));
                if(PURGE_BLOCK_SZ > Length) {
                    CcPurgeCacheSection(SectionObject, (PLARGE_INTEGER)&Offset,
                                                (ULONG)Length, FALSE);
    /*
                    NtReqFcb->CommonFCBHeader.ValidDataLength.QuadPart += Length;
                    ASSERT(NtReqFcb->CommonFCBHeader.ValidDataLength.QuadPart <=
                           NtReqFcb->CommonFCBHeader.FileSize.QuadPart);
                    MmPrint(("    CcFlushCache()\n"));
                    CcFlushCache( SectionObject, (PLARGE_INTEGER)&Offset, (ULONG)Length, NULL );
    */
#ifndef ALLOW_SPARSE
        //            UDFZeroFile__(
#endif //ALLOW_SPARSE
                    break;
                } else {
                    CcPurgeCacheSection(SectionObject, (PLARGE_INTEGER)&Offset,
                                                PURGE_BLOCK_SZ, FALSE);
    /* 
                    NtReqFcb->CommonFCBHeader.ValidDataLength.QuadPart += PURGE_BLOCK_SZ;
                    ASSERT(NtReqFcb->CommonFCBHeader.ValidDataLength.QuadPart <=
                           NtReqFcb->CommonFCBHeader.FileSize.QuadPart);
                    MmPrint(("    CcFlushCache()\n"));
                    CcFlushCache( SectionObject, (PLARGE_INTEGER)&Offset, (ULONG)Length, NULL );
    */
#ifndef ALLOW_SPARSE
        //            UDFZeroFile__(
#endif //ALLOW_SPARSE
                    Length -= PURGE_BLOCK_SZ;
                    Offset += PURGE_BLOCK_SZ;
                }
            }
#ifndef USE_CcCopyWrite_TO_ZERO
            while(Length);
#endif //USE_CcCopyWrite_TO_ZERO
            if(VDL < Offset)
                NtReqFcb->CommonFCBHeader.ValidDataLength.QuadPart = Offset;
        }
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        BrutePoint();
    } _SEH2_END;
} // end UDFPurgeCacheEx_()

#endif //UDF_READ_ONLY_BUILD
