////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////
/*************************************************************************
*
* File: Read.cpp
*
* Module: UDF File System Driver (Kernel mode execution only)
*
* Description:
*   Contains code to handle the "Read" dispatch entry point.
*
*************************************************************************/

#include            "udffs.h"

// define the file specific bug-check id
#define         UDF_BUG_CHECK_ID                UDF_FILE_READ

#ifdef _M_IX86
#if DBG
#define OVERFLOW_READ_THRESHHOLD         (0xE00)
#else
#define OVERFLOW_READ_THRESHHOLD         (0xA00)
#endif // UDF_DBG
#else  // defined(_M_IX86)
#define OVERFLOW_READ_THRESHHOLD         (0x1000)
#endif // defined(_M_IX86)

//#define POST_LOCK_PAGES


/*************************************************************************
*
* Function: UDFRead()
*
* Description:
*   The I/O Manager will invoke this routine to handle a read
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
UDFRead(
    PDEVICE_OBJECT DeviceObject,       // the logical volume device object
    PIRP           Irp)                // I/O Request Packet
{
    NTSTATUS            RC = STATUS_SUCCESS;
    PtrUDFIrpContext    PtrIrpContext = NULL;
    BOOLEAN             AreWeTopLevel = FALSE;

    TmPrint(("UDFRead: \n"));

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
            RC = UDFCommonRead(PtrIrpContext, Irp);
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
} // end UDFRead()


/*************************************************************************
*
* Function: UDFPostStackOverflowRead()
*
* Description:
*    Post a read request that could not be processed by
*    the fsp thread because of stack overflow potential.
*
* Arguments:
*    Irp - Supplies the request to process.
*    Fcb - Supplies the file.
*
* Return Value: STATUS_PENDING.
*
*************************************************************************/
NTSTATUS
UDFPostStackOverflowRead(
    IN PtrUDFIrpContext PtrIrpContext,
    IN PIRP             Irp,
    IN PtrUDFFCB        Fcb
    )
{
    PKEVENT Event;
    PERESOURCE Resource;

    KdPrint(("Getting too close to stack limit pass request to Fsp\n"));

    //  Allocate an event and get shared on the resource we will
    //  be later using the common read.
    Event = (PKEVENT)MyAllocatePool__(NonPagedPool, sizeof(KEVENT));
    if(!Event)
        return STATUS_INSUFFICIENT_RESOURCES;
    KeInitializeEvent( Event, NotificationEvent, FALSE );

    if ((Irp->Flags & IRP_PAGING_IO) && (Fcb->NTRequiredFCB->CommonFCBHeader.PagingIoResource)) {
        Resource = Fcb->NTRequiredFCB->CommonFCBHeader.PagingIoResource;
    } else {
        Resource = Fcb->NTRequiredFCB->CommonFCBHeader.Resource;
    }

    UDFAcquireResourceShared( Resource, TRUE );

    _SEH2_TRY {
        //  If this read is the result of a verify, we have to
        //  tell the overflow read routne to temporarily
        //  hijack the Vcb->VerifyThread field so that reads
        //  can go through.
        FsRtlPostStackOverflow(PtrIrpContext, Event, UDFStackOverflowRead);
        //  And wait for the worker thread to complete the item
        DbgWaitForSingleObject(Event, NULL);

    } _SEH2_FINALLY {

        UDFReleaseResource( Resource );
        MyFreePool__( Event );
    } _SEH2_END;

    return STATUS_PENDING;

} // end UDFPostStackOverflowRead()

/*************************************************************************
*
* Function: UDFStackOverflowRead()
*
* Description:
*    Process a read request that could not be processed by
*    the fsp thread because of stack overflow potential.
*
* Arguments:
*    Context - Supplies the IrpContext being processed
*    Event - Supplies the event to be signaled when we are done processing this
*        request.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: None.
*
*************************************************************************/
VOID
NTAPI
UDFStackOverflowRead(
    IN PVOID Context,
    IN PKEVENT Event
    )
{
    PtrUDFIrpContext PtrIrpContext = (PtrUDFIrpContext)Context;
    NTSTATUS RC;

    KdPrint(("UDFStackOverflowRead: \n"));
    //  Make it now look like we can wait for I/O to complete
    PtrIrpContext->IrpContextFlags |= UDF_IRP_CONTEXT_CAN_BLOCK;

    //  Do the read operation protected by a try-except clause
    _SEH2_TRY {
        UDFCommonRead(PtrIrpContext, PtrIrpContext->Irp);
    } _SEH2_EXCEPT(UDFExceptionFilter(PtrIrpContext, _SEH2_GetExceptionInformation())) {
        RC = UDFExceptionHandler(PtrIrpContext, PtrIrpContext->Irp);
        UDFLogEvent(UDF_ERROR_INTERNAL_ERROR, RC);
    } _SEH2_END;

    //  Set the stack overflow item's event to tell the original
    //  thread that we're done.
    KeSetEvent( Event, 0, FALSE );
} // end UDFStackOverflowRead()


/*************************************************************************
*
* Function: UDFCommonRead()
*
* Description:
*   The actual work is performed here. This routine may be invoked in one
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
UDFCommonRead(
    PtrUDFIrpContext PtrIrpContext,
    PIRP             Irp
    )
{
    NTSTATUS                RC = STATUS_SUCCESS;
    PIO_STACK_LOCATION      IrpSp = NULL;
    LARGE_INTEGER           ByteOffset;
    ULONG                   ReadLength = 0, TruncatedLength = 0;
    ULONG                   NumberBytesRead = 0;
    PFILE_OBJECT            FileObject = NULL;
    PtrUDFFCB               Fcb = NULL;
    PtrUDFCCB               Ccb = NULL;
    PVCB                    Vcb = NULL;
    PtrUDFNTRequiredFCB     NtReqFcb = NULL;
    PERESOURCE              PtrResourceAcquired = NULL;
    PERESOURCE              PtrResourceAcquired2 = NULL;
    PVOID                   SystemBuffer = NULL;
    PIRP                    TopIrp;
//    uint32                  KeyValue = 0;

    ULONG                   Res1Acq = 0;
    ULONG                   Res2Acq = 0;

    BOOLEAN                 CacheLocked = FALSE;

    BOOLEAN                 CanWait = FALSE;
    BOOLEAN                 PagingIo = FALSE;
    BOOLEAN                 NonBufferedIo = FALSE;
    BOOLEAN                 SynchronousIo = FALSE;

    TmPrint(("UDFCommonRead: irp %x\n", Irp));

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
//            BrutePoint()
            break;
        case FSRTL_FAST_IO_TOP_LEVEL_IRP:
            KdPrint(("  FSRTL_FAST_IO_TOP_LEVEL_IRP\n"));
//            BrutePoint()
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

        // If this happens to be a MDL read complete request, then
        // there is not much processing that the FSD has to do.
        if (IrpSp->MinorFunction & IRP_MN_COMPLETE) {
            // Caller wants to tell the Cache Manager that a previously
            // allocated MDL can be freed.
            UDFMdlComplete(PtrIrpContext, Irp, IrpSp, TRUE);
            // The IRP has been completed.
            try_return(RC = STATUS_SUCCESS);
        }

        // If this is a request at IRQL DISPATCH_LEVEL, then post
        // the request (your FSD may choose to process it synchronously
        // if you implement the support correctly; obviously you will be
        // quite constrained in what you can do at such IRQL).
        if (IrpSp->MinorFunction & IRP_MN_DPC) {
            try_return(RC = STATUS_PENDING);
        }

        FileObject = IrpSp->FileObject;
        ASSERT(FileObject);

        // Get the FCB and CCB pointers
        Ccb = (PtrUDFCCB)(FileObject->FsContext2);
        ASSERT(Ccb);
        Fcb = Ccb->Fcb;
        ASSERT(Fcb);
        Vcb = Fcb->Vcb;

        if(Fcb->FCBFlags & UDF_FCB_DELETED) {
            ASSERT(FALSE);
            try_return(RC = STATUS_ACCESS_DENIED);
        }

        // check for stack overflow
        if (IoGetRemainingStackSize() < OVERFLOW_READ_THRESHHOLD) {
            RC = UDFPostStackOverflowRead( PtrIrpContext, Irp, Fcb );
            try_return(RC);
        }

        // Disk based file systems might decide to verify the logical volume
        //  (if required and only if removable media are supported) at this time
        // As soon as Tray is locked, we needn't call UDFVerifyVcb()

        ByteOffset = IrpSp->Parameters.Read.ByteOffset;

        CanWait = (PtrIrpContext->IrpContextFlags & UDF_IRP_CONTEXT_CAN_BLOCK) ? TRUE : FALSE;
        PagingIo = (Irp->Flags & IRP_PAGING_IO) ? TRUE : FALSE;
        NonBufferedIo = (Irp->Flags & IRP_NOCACHE) ? TRUE : FALSE;
        SynchronousIo = (FileObject->Flags & FO_SYNCHRONOUS_IO) ? TRUE : FALSE;
        KdPrint(("    Flags: %s %s %s %s\n",
                      CanWait ? "W" : "w", PagingIo ? "Pg" : "pg",
                      NonBufferedIo ? "NBuf" : "buff", SynchronousIo ? "Snc" : "Asc"));

        if(!NonBufferedIo &&
           (Fcb->NodeIdentifier.NodeType != UDF_NODE_TYPE_VCB)) {
            if(UDFIsAStream(Fcb->FileInfo)) {
                UDFNotifyFullReportChange( Vcb, Fcb->FileInfo,
                                           FILE_NOTIFY_CHANGE_LAST_ACCESS,
                                           FILE_ACTION_MODIFIED_STREAM);
            } else {
                UDFNotifyFullReportChange( Vcb, Fcb->FileInfo,
                                           FILE_NOTIFY_CHANGE_LAST_ACCESS,
                                           FILE_ACTION_MODIFIED);
            }
        }

        // Get some of the parameters supplied to us
        ReadLength = IrpSp->Parameters.Read.Length;
        if (ReadLength == 0) {
            // a 0 byte read can be immediately succeeded
            try_return(RC);
        }
        KdPrint(("    ByteOffset = %I64x, ReadLength = %x\n", ByteOffset.QuadPart, ReadLength));

        // Is this a read of the volume itself ?
        if (Fcb->NodeIdentifier.NodeType == UDF_NODE_TYPE_VCB) {
            // Yup, we need to send this on to the disk driver after
            //  validation of the offset and length.
            Vcb = (PVCB)Fcb;
            Vcb->VCBFlags |= UDF_VCB_SKIP_EJECT_CHECK;
            if(!CanWait)
                try_return(RC = STATUS_PENDING);


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

            if(PtrIrpContext->IrpContextFlags & UDF_IRP_CONTEXT_FLUSH_REQUIRED) {

                KdPrint(("  UDF_IRP_CONTEXT_FLUSH_REQUIRED\n"));
                PtrIrpContext->IrpContextFlags &= ~UDF_IRP_CONTEXT_FLUSH_REQUIRED;

                // Acquire the volume resource exclusive
                UDFAcquireResourceExclusive(&(Vcb->VCBResource), TRUE);
                PtrResourceAcquired = &(Vcb->VCBResource);

                UDFFlushLogicalVolume(NULL, NULL, Vcb, 0);

                UDFReleaseResource(PtrResourceAcquired);
                PtrResourceAcquired = NULL;
            }

            // Acquire the volume resource shared ...
            UDFAcquireResourceShared(&(Vcb->VCBResource), TRUE);
            PtrResourceAcquired = &(Vcb->VCBResource);

#if 0
            if(PagingIo) {
                CollectStatistics(Vcb, MetaDataReads);
                CollectStatisticsEx(Vcb, MetaDataReadBytes, NumberBytesRead);
            }
#endif

            // Forward the request to the lower level driver
            // Lock the callers buffer
            if (!NT_SUCCESS(RC = UDFLockCallersBuffer(PtrIrpContext, Irp, TRUE, ReadLength))) {
                try_return(RC);
            }
            SystemBuffer = UDFGetCallersBuffer(PtrIrpContext, Irp);
            if(!SystemBuffer) {
                try_return(RC = STATUS_INVALID_USER_BUFFER);
            }
            if(Vcb->VCBFlags & UDF_VCB_FLAGS_VOLUME_MOUNTED) {
                 RC = UDFReadData(Vcb, TRUE, ByteOffset.QuadPart,
                                ReadLength, FALSE, (PCHAR)SystemBuffer,
                                &NumberBytesRead);
            } else {
                 RC = UDFTRead(Vcb, SystemBuffer, ReadLength,
                                (ULONG)(ByteOffset.QuadPart >> Vcb->BlockSizeBits),
                                &NumberBytesRead);
            }
            UDFUnlockCallersBuffer(PtrIrpContext, Irp, SystemBuffer);
            try_return(RC);
        }
        Vcb->VCBFlags |= UDF_VCB_SKIP_EJECT_CHECK;

        // If the read request is directed to a page file (if your FSD
        // supports paging files), send the request directly to the disk
        // driver. For requests directed to a page file, you have to trust
        // that the offsets will be set correctly by the VMM. You should not
        // attempt to acquire any FSD resources either.
        if(Fcb->FCBFlags & UDF_FCB_PAGE_FILE) {
            NonBufferedIo = TRUE;
        }

        if(ByteOffset.HighPart == -1) {
            if(ByteOffset.LowPart == FILE_USE_FILE_POINTER_POSITION) {
                ByteOffset = FileObject->CurrentByteOffset;
            }
        }

        // If this read is directed to a directory, it is not allowed
        //  by the UDF FSD.
        if(Fcb->FCBFlags & UDF_FCB_DIRECTORY) {
            RC = STATUS_INVALID_DEVICE_REQUEST;
            try_return(RC);
        }

        NtReqFcb = Fcb->NTRequiredFCB;

        Res1Acq = UDFIsResourceAcquired(&(NtReqFcb->MainResource));
        if(!Res1Acq) {
            Res1Acq = PtrIrpContext->IrpContextFlags & UDF_IRP_CONTEXT_RES1_ACQ;
        }
        Res2Acq = UDFIsResourceAcquired(&(NtReqFcb->PagingIoResource));
        if(!Res2Acq) {
            Res2Acq = PtrIrpContext->IrpContextFlags & UDF_IRP_CONTEXT_RES2_ACQ;
        }

#if 0
        if(PagingIo) {
            CollectStatistics(Vcb, UserFileReads);
            CollectStatisticsEx(Vcb, UserFileReadBytes, NumberBytesRead);
        }
#endif

        // This is a good place for oplock related processing.

        // If this is the normal file we have to check for
        // write access according to the current state of the file locks.
        if (!PagingIo &&
            !FsRtlCheckLockForReadAccess( &(NtReqFcb->FileLock), Irp )) {
                try_return( RC = STATUS_FILE_LOCK_CONFLICT );
        }

        // Validate start offset and length supplied.
        //  If start offset is > end-of-file, return an appropriate error. Note
        // that since a FCB resource has already been acquired, and since all
        // file size changes require acquisition of both FCB resources,
        // the contents of the FCB and associated data structures
        // can safely be examined.

        //  Also note that we are using the file size in the "Common FCB Header"
        // to perform the check. However, your FSD might decide to keep a
        // separate copy in the FCB (or some other representation of the
        //  file associated with the FCB).

        TruncatedLength = ReadLength;
        if (ByteOffset.QuadPart >= NtReqFcb->CommonFCBHeader.FileSize.QuadPart) {
            // Starting offset is >= file size
            try_return(RC = STATUS_END_OF_FILE);
        }
        // We can also go ahead and truncate the read length here
        //  such that it is contained within the file size
        if( NtReqFcb->CommonFCBHeader.FileSize.QuadPart < (ByteOffset.QuadPart + ReadLength) ) {
            TruncatedLength = (ULONG)(NtReqFcb->CommonFCBHeader.FileSize.QuadPart - ByteOffset.QuadPart);
            // we can't get ZERO here
        }
        KdPrint(("    TruncatedLength = %x\n", TruncatedLength));

        // There are certain complications that arise when the same file stream
        // has been opened for cached and non-cached access. The FSD is then
        // responsible for maintaining a consistent view of the data seen by
        // the caller.
        // Also, it is possible for file streams to be mapped in both as data files
        // and as an executable. This could also lead to consistency problems since
        // there now exist two separate sections (and pages) containing file
        // information.

        // The test below flushes the data cached in system memory if the current
        // request madates non-cached access (file stream must be cached) and
        // (a) the current request is not paging-io which indicates it is not
        //       a recursive I/O operation OR originating in the Cache Manager
        // (b) OR the current request is paging-io BUT it did not originate via
        //       the Cache Manager (or is a recursive I/O operation) and we do
        //       have an image section that has been initialized.
#define UDF_REQ_NOT_VIA_CACHE_MGR(ptr)  (!MmIsRecursiveIoFault() && ((ptr)->ImageSectionObject != NULL))

        if(NonBufferedIo &&
           (NtReqFcb->SectionObject.DataSectionObject != NULL)) {
            if(!PagingIo) {

/*                // We hold the main resource exclusive here because the flush
                // may generate a recursive write in this thread.  The PagingIo
                // resource is held shared so the drop-and-release serialization
                // below will work.
                if(!UDFAcquireResourceExclusive(&(NtReqFcb->MainResource), CanWait)) {
                    try_return(RC = STATUS_PENDING);
                }
                PtrResourceAcquired = &(NtReqFcb->MainResource);

                // We hold PagingIo shared around the flush to fix a
                // cache coherency problem.
                UDFAcquireResourceShared(&(NtReqFcb->PagingIoResource), TRUE );*/

                MmPrint(("    CcFlushCache()\n"));
                CcFlushCache(&(NtReqFcb->SectionObject), &ByteOffset, TruncatedLength, &(Irp->IoStatus));

/*                UDFReleaseResource(&(NtReqFcb->PagingIoResource));
                UDFReleaseResource(PtrResourceAcquired);
                PtrResourceAcquired = NULL;
                // If the flush failed, return error to the caller
                if(!NT_SUCCESS(RC = Irp->IoStatus.Status)) {
                    try_return(RC);
                }

                // Acquiring and immediately dropping the resource serializes
                // us behind any other writes taking place (either from the
                // lazy writer or modified page writer).*/
                if(!Res2Acq) {
                    UDFAcquireResourceExclusive(&(NtReqFcb->PagingIoResource), TRUE );
                    UDFReleaseResource(&(NtReqFcb->PagingIoResource));
                }
            }
        }
    
        // Acquire the appropriate FCB resource shared
        if (PagingIo) {
            // Try to acquire the FCB PagingIoResource shared
            if(!Res2Acq) {
                if (!UDFAcquireResourceShared(&(NtReqFcb->PagingIoResource), CanWait)) {
                    try_return(RC = STATUS_PENDING);
                }
                // Remember the resource that was acquired
                PtrResourceAcquired2 = &(NtReqFcb->PagingIoResource);
            }
        } else {
            // Try to acquire the FCB MainResource shared
            if(NonBufferedIo) {
                if(!Res2Acq) {
                    if(!UDFAcquireSharedWaitForExclusive(&(NtReqFcb->PagingIoResource), CanWait)) {
                        try_return(RC = STATUS_PENDING);
                    }
                    PtrResourceAcquired2 = &(NtReqFcb->PagingIoResource);
                }
            } else {
                if(!Res1Acq) {
                    UDF_CHECK_PAGING_IO_RESOURCE(NtReqFcb);
                    if(!UDFAcquireResourceShared(&(NtReqFcb->MainResource), CanWait)) {
                        try_return(RC = STATUS_PENDING);
                    }
                    // Remember the resource that was acquired
                    PtrResourceAcquired = &(NtReqFcb->MainResource);
                }
            }
        }

        // This is also a good place to set whether fast-io can be performed
        // on this particular file or not. Your FSD must make it's own
        // determination on whether or not to allow fast-io operations.
        // Commonly, fast-io is not allowed if any byte range locks exist
        // on the file or if oplocks prevent fast-io. Practically any reason
        // choosen by your FSD could result in your setting FastIoIsNotPossible
        // OR FastIoIsQuestionable instead of FastIoIsPossible.

        NtReqFcb->CommonFCBHeader.IsFastIoPossible = UDFIsFastIoPossible(Fcb);
/*        if(NtReqFcb->CommonFCBHeader.IsFastIoPossible == FastIoIsPossible)
            NtReqFcb->CommonFCBHeader.IsFastIoPossible = FastIoIsQuestionable;*/

#ifdef UDF_DISABLE_SYSTEM_CACHE_MANAGER
        NonBufferedIo = TRUE;
#endif

        if(Fcb && Fcb->FileInfo && Fcb->FileInfo->Dloc) {
            AdPrint(("UDFCommonRead: DataLoc %x, Mapping %x\n", &Fcb->FileInfo->Dloc->DataLoc, Fcb->FileInfo->Dloc->DataLoc.Mapping));
        }

        //  Branch here for cached vs non-cached I/O
        if (!NonBufferedIo) {

            if(FileObject->Flags & FO_WRITE_THROUGH) {
                CanWait = TRUE;
            }
            // The caller wishes to perform cached I/O. Initiate caching if
            // this is the first cached I/O operation using this file object
            if (!(FileObject->PrivateCacheMap)) {
                // This is the first cached I/O operation. You must ensure
                // that the FCB Common FCB Header contains valid sizes at this time
                MmPrint(("    CcInitializeCacheMap()\n"));
                CcInitializeCacheMap(FileObject, (PCC_FILE_SIZES)(&(NtReqFcb->CommonFCBHeader.AllocationSize)),
                    FALSE,      // We will not utilize pin access for this file
                    &(UDFGlobalData.CacheMgrCallBacks), // callbacks
                    NtReqFcb);        // The context used in callbacks
                MmPrint(("    CcSetReadAheadGranularity()\n"));
                CcSetReadAheadGranularity(FileObject, Vcb->SystemCacheGran);
            }

            // Check and see if this request requires a MDL returned to the caller
            if (IrpSp->MinorFunction & IRP_MN_MDL) {
                // Caller does want a MDL returned. Note that this mode
                // implies that the caller is prepared to block
                MmPrint(("    CcMdlRead()\n"));
//                CcMdlRead(FileObject, &ByteOffset, TruncatedLength, &(Irp->MdlAddress), &(Irp->IoStatus));
//                NumberBytesRead = Irp->IoStatus.Information;
//                RC = Irp->IoStatus.Status;
                NumberBytesRead = 0;
                RC = STATUS_INVALID_PARAMETER;

                try_return(RC);
            }

            // This is a regular run-of-the-mill cached I/O request. Let the
            // Cache Manager worry about it!
            // First though, we need a buffer pointer (address) that is valid
            SystemBuffer = UDFGetCallersBuffer(PtrIrpContext, Irp);
            if(!SystemBuffer)
                try_return(RC = STATUS_INVALID_USER_BUFFER);
            ASSERT(SystemBuffer);
            MmPrint(("    CcCopyRead()\n"));
            if (!CcCopyRead(FileObject, &(ByteOffset), TruncatedLength, CanWait, SystemBuffer, &(Irp->IoStatus))) {
                // The caller was not prepared to block and data is not immediately
                // available in the system cache
                try_return(RC = STATUS_PENDING);
            }

            UDFUnlockCallersBuffer(PtrIrpContext, Irp, SystemBuffer);
            // We have the data
            RC = Irp->IoStatus.Status;
            NumberBytesRead = Irp->IoStatus.Information;

            try_return(RC);

        } else {

            MmPrint(("    Read NonBufferedIo\n"));

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
            if(!CanWait && UDFIsFileCached__(Vcb, Fcb->FileInfo, ByteOffset.QuadPart, TruncatedLength, FALSE)) {
                MmPrint(("    Locked => CanWait\n"));
                CacheLocked = TRUE;
                CanWait = TRUE;
            }

            // Send the request to lower level drivers
            if(!CanWait) {
                try_return(RC = STATUS_PENDING);
            }

//                ASSERT(NT_SUCCESS(RC));
            if(!Res2Acq) {
                if(UDFAcquireResourceSharedWithCheck(&(NtReqFcb->PagingIoResource)))
                    PtrResourceAcquired2 = &(NtReqFcb->PagingIoResource);
            }

            RC = UDFLockCallersBuffer(PtrIrpContext, Irp, TRUE, TruncatedLength);
            if(!NT_SUCCESS(RC)) {
                try_return(RC);
            }

            SystemBuffer = UDFGetCallersBuffer(PtrIrpContext, Irp);
            if(!SystemBuffer) {
                try_return(RC = STATUS_INVALID_USER_BUFFER);
            }

            RC = UDFReadFile__(Vcb, Fcb->FileInfo, ByteOffset.QuadPart, TruncatedLength,
                           CacheLocked, (PCHAR)SystemBuffer, &NumberBytesRead);
/*                // AFAIU, CacheManager wants this:
            if(!NT_SUCCESS(RC)) {
                NumberBytesRead = 0;
            }*/

            UDFUnlockCallersBuffer(PtrIrpContext, Irp, SystemBuffer);

#if 0
            if(PagingIo) {
                CollectStatistics(Vcb, UserDiskReads);
            } else {
                CollectStatistics2(Vcb, NonCachedDiskReads);
            }
#endif

            try_return(RC);

            // For paging-io, the FSD has to trust the VMM to do the right thing

            // Here is a common method used by Windows NT native file systems
            // that are in the process of sending a request to the disk driver.
            // First, mark the IRP as pending, then invoke the lower level driver
            // after setting a completion routine.
            // Meanwhile, this particular thread can immediately return a
            // STATUS_PENDING return code.
            // The completion routine is then responsible for completing the IRP
            // and unlocking appropriate resources

            // Also, at this point, the FSD might choose to utilize the
            // information contained in the ValidDataLength field to simply
            // return zeroes to the caller for reads extending beyond current
            // valid data length.

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

        // Post IRP if required
        if(RC == STATUS_PENDING) {

            // Lock the callers buffer here. Then invoke a common routine to
            // perform the post operation.
            if (!(IrpSp->MinorFunction & IRP_MN_MDL)) {
                RC = UDFLockCallersBuffer(PtrIrpContext, Irp, TRUE, ReadLength);
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
                FileObject->CurrentByteOffset.QuadPart = ByteOffset.QuadPart + NumberBytesRead;
            }
            // If the read completed successfully and this was not a paging-io
            // operation, set a flag in the CCB that indicates that a read was
            // performed and that the file time should be updated at cleanup
            if (NT_SUCCESS(RC) && !PagingIo) {
                FileObject->Flags |= FO_FILE_FAST_IO_READ;
                Ccb->CCBFlags |= UDF_CCB_ACCESSED;
            }

            if(!_SEH2_AbnormalTermination()) {
                Irp->IoStatus.Status = RC;
                Irp->IoStatus.Information = NumberBytesRead;
                KdPrint(("    NumberBytesRead = %x\n", NumberBytesRead));
                // Free up the Irp Context
                UDFReleaseIrpContext(PtrIrpContext);
                // complete the IRP
                MmPrint(("    Complete Irp, MDL=%x\n", Irp->MdlAddress));
                if(Irp->MdlAddress) {
                    UDFTouch(Irp->MdlAddress);
                }
                IoCompleteRequest(Irp, IO_DISK_INCREMENT);
            }
        } // can we complete the IRP ?
    } _SEH2_END; // end of "__finally" processing

    return(RC);
} // end UDFCommonRead()


#ifdef UDF_DBG
ULONG LockBufferCounter = 0;
ULONG BuildMdlCounter = 0;
#endif //UDF_DBG

/*************************************************************************
*
* Function: UDFGetCallersBuffer()
*
* Description:
*   Obtain a pointer to the caller's buffer.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: STATUS_SUCCESS/Error
*
*************************************************************************/
PVOID
UDFGetCallersBuffer(
    PtrUDFIrpContext PtrIrpContext,
    PIRP             Irp
    )
{
    VOID            *ReturnedBuffer = NULL;

    KdPrint(("UDFGetCallersBuffer: \n"));

    // If an MDL is supplied, use it.
    if(Irp->MdlAddress) {
        MmPrint(("    UDFGetCallersBuffer: MmGetSystemAddressForMdl(Irp->MdlAddress) MDL=%x\n", Irp->MdlAddress));
//        ReturnedBuffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
        ReturnedBuffer = MmGetSystemAddressForMdlSafer(Irp->MdlAddress);
    } else
    if (PtrIrpContext->IrpContextFlags & UDF_IRP_CONTEXT_BUFFER_LOCKED) {
        // Free buffer
#ifndef POST_LOCK_PAGES
        MmPrint(("    UDFGetCallersBuffer: MmGetSystemAddressForMdl(PtrIrpContext->PtrMdl) MDL=%x\n", PtrIrpContext->PtrMdl));
        ReturnedBuffer = MmGetSystemAddressForMdlSafe(PtrIrpContext->PtrMdl, NormalPagePriority);
#else //POST_LOCK_PAGES
            if(PtrIrpContext->TransitionBuffer) {
                MmPrint(("    UDFGetCallersBuffer: TransitionBuffer\n"));
                return PtrIrpContext->TransitionBuffer;
            }

            _SEH2_TRY {
                MmPrint(("    MmProbeAndLockPages()\n"));
                MmProbeAndLockPages(PtrIrpContext->PtrMdl, Irp->RequestorMode,
                                    ((PtrIrpContext->MajorFunction == IRP_MJ_READ) ? IoWriteAccess:IoReadAccess));
#ifdef UDF_DBG
                LockBufferCounter++;
#endif //UDF_DBG
            } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
                //RC = STATUS_INVALID_USER_BUFFER;
                BrutePoint();
                return NULL;
            } _SEH2_END;

            MmPrint(("    MmGetSystemAddressForMdlSafer()\n"));
        ReturnedBuffer = MmGetSystemAddressForMdlSafer(PtrIrpContext->PtrMdl);
#endif //POST_LOCK_PAGES
    } else {
        MmPrint(("    UDFGetCallersBuffer: Irp->UserBuffer\n"));
        ReturnedBuffer = Irp->UserBuffer;
    }

    return(ReturnedBuffer);
} // end UDFGetCallersBuffer()

/*************************************************************************
*
* Function: UDFLockCallersBuffer()
*
* Description:
*   Obtain a MDL that describes the buffer. Lock pages for I/O
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: STATUS_SUCCESS/Error
*
*************************************************************************/
NTSTATUS
UDFLockCallersBuffer(
    PtrUDFIrpContext  PtrIrpContext,
    PIRP              Irp,
    BOOLEAN           IsReadOperation,
    uint32            Length
    )
{
    NTSTATUS            RC = STATUS_SUCCESS;
    PMDL                PtrMdl = NULL;

    KdPrint(("UDFLockCallersBuffer: \n"));

    ASSERT(Irp);
    
    _SEH2_TRY {
        // Is a MDL already present in the IRP
        if (!(Irp->MdlAddress)) {
            // Allocate a MDL
/*
            if(!IsReadOperation) {
                MmPrint(("  Allocate TransitionBuffer\n"));
                PtrIrpContext->TransitionBuffer = (PCHAR)DbgAllocatePool(NonPagedPool, Length);
                if(!PtrIrpContext->TransitionBuffer) {
                    RC = STATUS_INSUFFICIENT_RESOURCES;
                    try_return(RC);
                }
                _SEH2_TRY {
                    RtlCopyMemory(PtrIrpContext->TransitionBuffer, Irp->UserBuffer, Length);
                } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
                    RC = STATUS_INVALID_USER_BUFFER;
                } _SEH2_END;
            } else*/ {

                MmPrint(("  IoAllocateMdl()\n"));
//                if (!(PtrMdl = IoAllocateMdl(Irp->UserBuffer, Length, FALSE, FALSE, NULL))) {

                // This will place allocated Mdl to Irp
                if (!(PtrMdl = IoAllocateMdl(Irp->UserBuffer, Length, FALSE, FALSE, Irp))) {
                    RC = STATUS_INSUFFICIENT_RESOURCES;
                    try_return(RC);
                }
                MmPrint(("    Alloc MDL=%x\n", PtrMdl));
#ifdef UDF_DBG
                BuildMdlCounter++;
#endif //UDF_DBG
            }
            // Probe and lock the pages described by the MDL
            // We could encounter an exception doing so, swallow the exception
            // NOTE: The exception could be due to an unexpected (from our
            // perspective), invalidation of the virtual addresses that comprise
            // the passed in buffer
#ifndef POST_LOCK_PAGES
            _SEH2_TRY {
                MmPrint(("    MmProbeAndLockPages()\n"));
                MmProbeAndLockPages(PtrMdl, Irp->RequestorMode, (IsReadOperation ? IoWriteAccess:IoReadAccess));
            } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
                MmPrint(("    MmProbeAndLockPages() failed\n"));
                Irp->MdlAddress = NULL;
                RC = STATUS_INVALID_USER_BUFFER;
            } _SEH2_END;
#endif //POST_LOCK_PAGES

            if(NT_SUCCESS(RC)) {
                PtrIrpContext->IrpContextFlags |= UDF_IRP_CONTEXT_BUFFER_LOCKED;
                PtrIrpContext->PtrMdl = PtrMdl;
            }
        } else {
            MmPrint(("    UDFLockCallersBuffer: do nothing, MDL=%x\n", Irp->MdlAddress));
            UDFTouch(Irp->MdlAddress);
        }

try_exit:   NOTHING;

    } _SEH2_FINALLY {
        if (!NT_SUCCESS(RC) && PtrMdl) {
            MmPrint(("    Free MDL=%x\n", PtrMdl));
            IoFreeMdl(PtrMdl);
        }
    } _SEH2_END;

    return(RC);
} // end UDFLockCallersBuffer()

/*************************************************************************
*
* Function: UDFUnlockCallersBuffer()
*
* Description:
*   Obtain a MDL that describes the buffer. Lock pages for I/O
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: STATUS_SUCCESS/Error
*
*************************************************************************/
NTSTATUS
UDFUnlockCallersBuffer(
    PtrUDFIrpContext PtrIrpContext,
    PIRP    Irp,
    PVOID   SystemBuffer
    )
{
    NTSTATUS            RC = STATUS_SUCCESS;

    KdPrint(("UDFUnlockCallersBuffer: \n"));

    ASSERT(Irp);

    _SEH2_TRY {
        // Is a nonPaged buffer already present in the IRP
        if (PtrIrpContext->IrpContextFlags & UDF_IRP_CONTEXT_BUFFER_LOCKED) {

            KdPrint(("  UDF_IRP_CONTEXT_BUFFER_LOCKED MDL=%x, Irp MDL=%x\n", PtrIrpContext->PtrMdl, Irp->MdlAddress));
            if(PtrIrpContext->TransitionBuffer) {
                MmPrint(("    UDFUnlockCallersBuffer: free TransitionBuffer\n"));
                DbgFreePool(PtrIrpContext->TransitionBuffer);
                PtrIrpContext->TransitionBuffer = NULL;
                PtrIrpContext->IrpContextFlags &= ~UDF_IRP_CONTEXT_BUFFER_LOCKED;
                try_return(RC);
            }
            // Free buffer
            KeFlushIoBuffers( PtrIrpContext->PtrMdl, TRUE, FALSE );
//            MmPrint(("    IrpCtx->Mdl, MmUnmapLockedPages()\n"));
//            MmUnmapLockedPages(SystemBuffer, PtrIrpContext->PtrMdl);

            // This will be done in IoCompleteIrp !!!

            //MmPrint(("    MmUnlockPages()\n"));
            //MmUnlockPages(PtrIrpContext->PtrMdl);

#ifdef UDF_DBG
            LockBufferCounter--;
#endif //UDF_DBG

            // This will be done in IoCompleteIrp !!!

            //IoFreeMdl(PtrIrpContext->PtrMdl);

#ifdef UDF_DBG
            BuildMdlCounter--;
#endif //UDF_DBG
            UDFTouch(PtrIrpContext->PtrMdl);
            PtrIrpContext->PtrMdl = NULL;
            PtrIrpContext->IrpContextFlags &= ~UDF_IRP_CONTEXT_BUFFER_LOCKED;
        } else
        if(Irp->MdlAddress) {
//            MmPrint(("    Irp->Mdl, MmUnmapLockedPages()\n"));
//            MmUnmapLockedPages(SystemBuffer, Irp->MdlAddress);
            KdPrint(("  UDF_IRP_CONTEXT_BUFFER_LOCKED MDL=%x, Irp MDL=%x\n", PtrIrpContext->PtrMdl, Irp->MdlAddress));
            UDFTouch(Irp->MdlAddress);
            KeFlushIoBuffers( Irp->MdlAddress,
                              ((IoGetCurrentIrpStackLocation(Irp))->MajorFunction) == IRP_MJ_READ,
                              FALSE );
        } else
        { ; }

try_exit:   NOTHING;

    } _SEH2_FINALLY {
        NOTHING;
    } _SEH2_END;

    return(RC);
} // end UDFUnlockCallersBuffer()

/*************************************************************************
*
* Function: UDFMdlComplete()
*
* Description:
*   Tell Cache Manager to release MDL (and possibly flush).
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: None.
*
*************************************************************************/
VOID UDFMdlComplete(
PtrUDFIrpContext            PtrIrpContext,
PIRP                        Irp,
PIO_STACK_LOCATION          IrpSp,
BOOLEAN                     ReadCompletion)
{
    NTSTATUS                RC = STATUS_SUCCESS;
    PFILE_OBJECT            FileObject = NULL;

    KdPrint(("UDFMdlComplete: \n"));

    FileObject = IrpSp->FileObject;
    ASSERT(FileObject);

    UDFTouch(Irp->MdlAddress);
    // Not much to do here.
    if (ReadCompletion) {
        MmPrint(("    CcMdlReadComplete() MDL=%x\n", Irp->MdlAddress));
        CcMdlReadComplete(FileObject, Irp->MdlAddress);
    } else {
        // The Cache Manager needs the byte offset in the I/O stack location.
        MmPrint(("    CcMdlWriteComplete() MDL=%x\n", Irp->MdlAddress));
        CcMdlWriteComplete(FileObject, &(IrpSp->Parameters.Write.ByteOffset), Irp->MdlAddress);
    }

    // Clear the MDL address field in the IRP so the IoCompleteRequest()
    // does not __try to play around with the MDL.
    Irp->MdlAddress = NULL;

    // Free up the Irp Context.
    UDFReleaseIrpContext(PtrIrpContext);

    // Complete the IRP.
    Irp->IoStatus.Status = RC;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return;
}
