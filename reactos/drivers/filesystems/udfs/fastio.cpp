////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////
/*************************************************************************
*
* File: Fastio.cpp
*
* Module: UDF File System Driver (Kernel mode execution only)
*
* Description:
*   Contains code to handle the various "fast-io" calls.
*
*************************************************************************/

#include            "udffs.h"

// define the file specific bug-check id
#define         UDF_BUG_CHECK_ID                UDF_FILE_FAST_IO



/*************************************************************************
*
* Function: UDFFastIoCheckIfPossible()
*
* Description:
*   To fast-io or not to fast-io, that is the question ...
*   This routine helps the I/O Manager determine whether the FSD wishes
*   to permit fast-io on a specific file stream.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: TRUE/FALSE
*
*************************************************************************/
BOOLEAN
NTAPI
UDFFastIoCheckIfPossible(
    IN PFILE_OBJECT             FileObject,
    IN PLARGE_INTEGER           FileOffset,
    IN ULONG                    Length,
    IN BOOLEAN                  Wait,
    IN ULONG                    LockKey,
    IN BOOLEAN                  CheckForReadOperation,
    OUT PIO_STATUS_BLOCK        IoStatus,
    IN PDEVICE_OBJECT           DeviceObject
    )
{
    BOOLEAN             ReturnedStatus = FALSE;
    PtrUDFFCB           Fcb = NULL;
    PtrUDFCCB           Ccb = NULL;
    LARGE_INTEGER       IoLength;

    // Obtain a pointer to the FCB and CCB for the file stream.
    Ccb = (PtrUDFCCB)(FileObject->FsContext2);
    ASSERT(Ccb);
    Fcb = Ccb->Fcb;
    ASSERT(Fcb);
    
    // Validate that this is a fast-IO request to a regular file.
    // The UDF FSD for example, will not allow fast-IO requests
    // to volume objects, or to directories.
    if ((Fcb->NodeIdentifier.NodeType == UDF_NODE_TYPE_VCB) ||
         (Fcb->FCBFlags & UDF_FCB_DIRECTORY)) {
        // This is not allowed.
        IoStatus->Status = STATUS_INVALID_PARAMETER;
        MmPrint(("    UDFFastIoCheckIfPossible() TRUE, Failed\n"));
        return FALSE;
    }
/*
    // back pressure for very smart and fast system cache ;)
    if(Fcb->Vcb->VerifyCtx.ItemCount >= UDF_MAX_VERIFY_CACHE) {
        AdPrint(("    Verify queue overflow -> UDFFastIoCheckIfPossible() = FALSE\n"));
        return FALSE;
    }
*/
    IoLength.QuadPart = Length;
    
    // The FSD can determine the checks that it needs to perform.
    // Typically, a FSD will check whether there exist any byte-range
    // locks that would prevent a fast-IO operation from proceeding.
    
    // ... (FSD specific checks go here).
    
    if (CheckForReadOperation) {
        // The following routine is exported by the FSRTL
        // package and it returns TRUE if the read operation should be
        // allowed to proceed based on the status of the current byte-range
        // locks on the file stream. If we do not use the FSRTL package
        // for byte-range locking support, then we must substitute our
        // own checks over here.
        ReturnedStatus = FsRtlFastCheckLockForRead(&(Fcb->NTRequiredFCB->FileLock),
                              FileOffset, &IoLength, LockKey, FileObject,
                              PsGetCurrentProcess());
    } else {
//        if(Fcb->Vcb->VCBFlags );
        // This is a write request. Invoke the FSRTL byte-range lock package
        // to see whether the write should be allowed to proceed.
        ReturnedStatus = FsRtlFastCheckLockForWrite(&(Fcb->NTRequiredFCB->FileLock),
                              FileOffset, &IoLength, LockKey, FileObject,
                              PsGetCurrentProcess());
    }
    
    MmPrint(("    UDFFastIoCheckIfPossible() %s\n", ReturnedStatus ? "TRUE" : "FALSE"));
    return(ReturnedStatus);
//    return FALSE;

} // end UDFFastIoCheckIfPossible()

/*
 */
FAST_IO_POSSIBLE
NTAPI
UDFIsFastIoPossible(
    IN PtrUDFFCB Fcb
    )
{
    if( !(Fcb->Vcb->VCBFlags & UDF_VCB_FLAGS_VOLUME_MOUNTED) /*||
        !FsRtlOplockIsFastIoPossible(&(Fcb->Oplock))*/ ) {
        KdPrint(("    FastIoIsNotPossible\n"));
        return FastIoIsNotPossible;
    }
/*
    // back pressure for very smart and fast system cache ;)
    if(Fcb->Vcb->VerifyCtx.ItemCount >= UDF_MAX_VERIFY_CACHE) {
        AdPrint(("    Verify queue overflow -> UDFIsFastIoPossible() = FastIoIsNotPossible\n"));
        return FastIoIsNotPossible;
    }
*/
    if(FsRtlAreThereCurrentFileLocks(&(Fcb->NTRequiredFCB->FileLock)) ) {
        KdPrint(("    FastIoIsQuestionable\n"));
        return FastIoIsQuestionable;
    }
    KdPrint(("    FastIoIsPossible\n"));
    return FastIoIsPossible;
} // end UDFIsFastIoPossible()

/*************************************************************************
*
* Function: UDFFastIoQueryBasicInfo()
*
* Description:
*   Bypass the traditional IRP method to perform a query basic
*   information operation.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: TRUE/FALSE
*
*************************************************************************/
BOOLEAN
NTAPI
UDFFastIoQueryBasicInfo(
    IN PFILE_OBJECT             FileObject,
    IN BOOLEAN                  Wait,
    OUT PFILE_BASIC_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK        IoStatus,
    IN PDEVICE_OBJECT           DeviceObject
    )
{
    BOOLEAN          ReturnedStatus = FALSE;     // fast i/o failed/not allowed
    NTSTATUS         RC = STATUS_SUCCESS;
    PtrUDFIrpContext PtrIrpContext = NULL;
    LONG             Length = sizeof(FILE_BASIC_INFORMATION);
    PtrUDFFCB        Fcb;
    PtrUDFCCB        Ccb;
    PtrUDFNTRequiredFCB NtReqFcb = NULL;
    BOOLEAN          MainResourceAcquired = FALSE;

    FsRtlEnterFileSystem();

    KdPrint(("UDFFastIo  \n"));
    // if the file is already opended we can satisfy this request
    // immediately 'cause all the data we need must be cached
    _SEH2_TRY {

        _SEH2_TRY {

            // Get the FCB and CCB pointers.
            Ccb = (PtrUDFCCB)(FileObject->FsContext2);
            ASSERT(Ccb);
            Fcb = Ccb->Fcb;
            ASSERT(Fcb);
            NtReqFcb = Fcb->NTRequiredFCB;
            //Fcb->Vcb->VCBFlags |= UDF_VCB_SKIP_EJECT_CHECK;

            if (!(Fcb->FCBFlags & UDF_FCB_PAGE_FILE)) {
                // Acquire the MainResource shared.
                UDF_CHECK_PAGING_IO_RESOURCE(NtReqFcb);
                if (!UDFAcquireResourceShared(&(NtReqFcb->MainResource), Wait)) {
                    try_return(RC = STATUS_CANT_WAIT);
                }
                MainResourceAcquired = TRUE;
            }

            ReturnedStatus =
                ((RC = UDFGetBasicInformation(FileObject, Fcb, Buffer, &Length)) == STATUS_SUCCESS);

        } _SEH2_EXCEPT(UDFExceptionFilter(PtrIrpContext, _SEH2_GetExceptionInformation())) {

            RC = UDFExceptionHandler(PtrIrpContext, NULL);

            UDFLogEvent(UDF_ERROR_INTERNAL_ERROR, RC);

        } _SEH2_END;
try_exit: NOTHING;
    } _SEH2_FINALLY {
        if (MainResourceAcquired) {
            UDF_CHECK_PAGING_IO_RESOURCE(NtReqFcb);
            UDFReleaseResource(&(NtReqFcb->MainResource));
            MainResourceAcquired = FALSE;
        }
        IoStatus->Status = RC;
        if(ReturnedStatus) {
            IoStatus->Information = sizeof(FILE_BASIC_INFORMATION);
        } else {
            IoStatus->Information = 0;
        }
    } _SEH2_END;
    
    FsRtlExitFileSystem();

    return(ReturnedStatus);
} // end UDFFastIoQueryBasicInfo()


/*************************************************************************
*
* Function: UDFFastIoQueryStdInfo()
*
* Description:
*   Bypass the traditional IRP method to perform a query standard
*   information operation.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: TRUE/FALSE
*
*************************************************************************/
BOOLEAN
NTAPI
UDFFastIoQueryStdInfo(
    IN PFILE_OBJECT                 FileObject,
    IN BOOLEAN                      Wait,
    OUT PFILE_STANDARD_INFORMATION  Buffer,
    OUT PIO_STATUS_BLOCK            IoStatus,
    IN PDEVICE_OBJECT               DeviceObject)
{
    BOOLEAN          ReturnedStatus = FALSE;     // fast i/o failed/not allowed
    NTSTATUS         RC = STATUS_SUCCESS;
    PtrUDFIrpContext PtrIrpContext = NULL;
    LONG             Length = sizeof(FILE_STANDARD_INFORMATION);
    PtrUDFFCB        Fcb;
    PtrUDFCCB        Ccb;
    PtrUDFNTRequiredFCB NtReqFcb = NULL;
//    BOOLEAN          MainResourceAcquired = FALSE;

    FsRtlEnterFileSystem();

    KdPrint(("UDFFastIo  \n"));
    // if the file is already opended we can satisfy this request
    // immediately 'cause all the data we need must be cached
    _SEH2_TRY {

        _SEH2_TRY {

            // Get the FCB and CCB pointers.
            Ccb = (PtrUDFCCB)(FileObject->FsContext2);
            ASSERT(Ccb);
            Fcb = Ccb->Fcb;
            ASSERT(Fcb);
            NtReqFcb = Fcb->NTRequiredFCB;
            //Fcb->Vcb->VCBFlags |= UDF_VCB_SKIP_EJECT_CHECK;

/*                
            if (!(Fcb->FCBFlags & UDF_FCB_PAGE_FILE)) {
                // Acquire the MainResource shared.
                UDF_CHECK_PAGING_IO_RESOURCE(NtReqFcb);
                if (!UDFAcquireResourceShared(&(NtReqFcb->MainResource), Wait)) {
                    try_return(RC = STATUS_CANT_WAIT);
                }
                MainResourceAcquired = TRUE;
            }
*/
            ReturnedStatus =
                ((RC = UDFGetStandardInformation(Fcb, Buffer, &Length)) == STATUS_SUCCESS);

        } _SEH2_EXCEPT(UDFExceptionFilter(PtrIrpContext, _SEH2_GetExceptionInformation())) {

            RC = UDFExceptionHandler(PtrIrpContext, NULL);

            UDFLogEvent(UDF_ERROR_INTERNAL_ERROR, RC);

        } _SEH2_END;
//try_exit: NOTHING;
    } _SEH2_FINALLY {
/*        
        if (MainResourceAcquired) {
            UDFReleaseResource(&(NtReqFcb->MainResource));
            MainResourceAcquired = FALSE;
        }
*/
        IoStatus->Status = RC;
        if(ReturnedStatus) {
            IoStatus->Information = sizeof(FILE_STANDARD_INFORMATION);
        } else {
            IoStatus->Information = 0;
        }
    } _SEH2_END;
    
    FsRtlExitFileSystem();

    return(ReturnedStatus);
} // end UDFFastIoQueryStdInfo()


/*************************************************************************
*
* Function: UDFFastIoAcqCreateSec()
*
* Description:
*   Not really a fast-io operation. Used by the VMM to acquire FSD resources
*   before processing a file map (create section object) request.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: None (we must be prepared to handle VMM initiated calls)
*
*************************************************************************/
VOID
NTAPI
UDFFastIoAcqCreateSec(
    IN PFILE_OBJECT FileObject
    )
{
    PtrUDFNTRequiredFCB NtReqFcb = (PtrUDFNTRequiredFCB)(FileObject->FsContext);
    
    MmPrint(("  AcqForCreateSection()\n"));
    // Acquire the MainResource exclusively for the file stream
    if(!ExIsResourceAcquiredExclusiveLite(&(NtReqFcb->MainResource)) ||
       !ExIsResourceAcquiredExclusiveLite(&(NtReqFcb->PagingIoResource)) ) {
        UDF_CHECK_PAGING_IO_RESOURCE(NtReqFcb);
    } else {
        MmPrint(("    already acquired\n"));
    }
    UDFAcquireResourceExclusive(&(NtReqFcb->MainResource), TRUE);

    // Although this is typically not required, the UDF FSD will
    // also acquire the PagingIoResource exclusively at this time
    // to conform with the resource acquisition described in the set
    // file information routine. Once again though, we will probably
    // not need to do this.
    UDFAcquireResourceExclusive(&(NtReqFcb->PagingIoResource), TRUE);
    NtReqFcb->AcqSectionCount++;

    return;
} // end UDFFastIoAcqCreateSec()


/*************************************************************************
*
* Function: UDFFastIoRelCreateSec()
*
* Description:
*   Not really a fast-io operation. Used by the VMM to release FSD resources
*   after processing a file map (create section object) request.
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
UDFFastIoRelCreateSec(
    IN PFILE_OBJECT FileObject)
{
    PtrUDFNTRequiredFCB NtReqFcb = (PtrUDFNTRequiredFCB)(FileObject->FsContext);
    
    MmPrint(("  RelFromCreateSection()\n"));

    NtReqFcb->AcqSectionCount--;
    // Release the PagingIoResource for the file stream
    UDFReleaseResource(&(NtReqFcb->PagingIoResource));

    // Release the MainResource for the file stream
    UDFReleaseResource(&(NtReqFcb->MainResource));

    return;
} // end UDFFastIoRelCreateSec()


/*************************************************************************
*
* Function: UDFAcqLazyWrite()
*
* Description:
*   Not really a fast-io operation. Used by the NT Cache Mgr to acquire FSD
*   resources before performing a delayed write (write behind/lazy write)
*   operation.
*   NOTE: this function really must succeed since the Cache Manager will
*           typically ignore failure and continue on ...
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: TRUE/FALSE (Cache Manager does not tolerate FALSE well)
*
*************************************************************************/
BOOLEAN NTAPI UDFAcqLazyWrite(
    IN PVOID   Context,
    IN BOOLEAN Wait)
{
    // The context is whatever we passed to the Cache Manager when invoking
    // the CcInitializeCacheMaps() function. In the case of the UDF FSD
    // implementation, this context is a pointer to the NT_REQ_FCB structure.
    PtrUDFNTRequiredFCB NtReqFcb = (PtrUDFNTRequiredFCB)Context;

    MmPrint(("  UDFAcqLazyWrite()\n"));

    // Acquire the PagingIoResource in the NT_REQ_FCB exclusively. Then, set the
    // lazy-writer thread id in the NT_REQ_FCB structure for identification
    // when an actual write request is received by the FSD.
    // Note: The lazy-writer typically always supplies WAIT set to TRUE.
    if (!UDFAcquireResourceExclusive(&(NtReqFcb->PagingIoResource), Wait))
        return FALSE;

    // Now, set the lazy-writer thread id.
    ASSERT(!(NtReqFcb->LazyWriterThreadID));
    NtReqFcb->LazyWriterThreadID = (unsigned int)(PsGetCurrentThread());

    ASSERT(IoGetTopLevelIrp() == NULL);
    IoSetTopLevelIrp((PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);

    // If our FSD needs to perform some special preparations in anticipation
    // of receving a lazy-writer request, do so now.
    return TRUE;
} // end UDFAcqLazyWrite()


/*************************************************************************
*
* Function: UDFRelLazyWrite()
*
* Description:
*   Not really a fast-io operation. Used by the NT Cache Mgr to release FSD
*   resources after performing a delayed write (write behind/lazy write)
*   operation.
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
UDFRelLazyWrite(
    IN PVOID   Context)
{
    // The context is whatever we passed to the Cache Manager when invoking
    // the CcInitializeCacheMaps() function. In the case of the UDF FSD
    // implementation, this context is a pointer to the NT_REQ_FCB structure.
    PtrUDFNTRequiredFCB NtReqFcb = (PtrUDFNTRequiredFCB)Context;

    MmPrint(("  UDFRelLazyWrite()\n"));

    // Remove the current thread-id from the NT_REQ_FCB
    // and release the MainResource.
    ASSERT((NtReqFcb->LazyWriterThreadID) == (unsigned int)PsGetCurrentThread());
    NtReqFcb->LazyWriterThreadID = 0;

    // Release the acquired resource.
    UDFReleaseResource(&(NtReqFcb->PagingIoResource));

    IoSetTopLevelIrp( NULL );
    return;
} // end UDFRelLazyWrite()


/*************************************************************************
*
* Function: UDFAcqReadAhead()
*
* Description:
*   Not really a fast-io operation. Used by the NT Cache Mgr to acquire FSD
*   resources before performing a read-ahead operation.
*   NOTE: this function really must succeed since the Cache Manager will
*           typically ignore failure and continue on ...
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: TRUE/FALSE (Cache Manager does not tolerate FALSE well)
*
*************************************************************************/
BOOLEAN
NTAPI
UDFAcqReadAhead(
    IN PVOID   Context,
    IN BOOLEAN Wait
    )
{
    // The context is whatever we passed to the Cache Manager when invoking
    // the CcInitializeCacheMaps() function. In the case of the UDF FSD
    // implementation, this context is a pointer to the NT_REQ_FCB structure.
#define NtReqFcb ((PtrUDFNTRequiredFCB)Context)

    MmPrint(("  AcqForReadAhead()\n"));

    // Acquire the MainResource in the NT_REQ_FCB shared.
    // Note: The read-ahead thread typically always supplies WAIT set to TRUE.
    UDF_CHECK_PAGING_IO_RESOURCE(NtReqFcb);
    if (!UDFAcquireResourceShared(&(NtReqFcb->MainResource), Wait))
        return FALSE;

    ASSERT(IoGetTopLevelIrp() == NULL);
    IoSetTopLevelIrp((PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);

    return TRUE;
#undef NtReqFcb

} // end UDFAcqReadAhead()


/*************************************************************************
*
* Function: UDFRelReadAhead()
*
* Description:
*   Not really a fast-io operation. Used by the NT Cache Mgr to release FSD
*   resources after performing a read-ahead operation.
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
UDFRelReadAhead(
    IN PVOID  Context)
{
    // The context is whatever we passed to the Cache Manager when invoking
    // the CcInitializeCacheMaps() function. In the case of the UDF FSD
    // implementation, this context is a pointer to the NT_REQ_FCB structure.
#define NtReqFcb ((PtrUDFNTRequiredFCB)Context)

    MmPrint(("  RelFromReadAhead()\n"));

    // Release the acquired resource.
    UDF_CHECK_PAGING_IO_RESOURCE(NtReqFcb);
    UDFReleaseResource(&(NtReqFcb->MainResource));

    // Of course, the FSD should undo whatever else seems appropriate at this
    // time.
    IoSetTopLevelIrp( NULL );

    return;
#undef NtReqFcb
} // end UDFRelReadAhead()

/* the remaining are only valid under NT Version 4.0 and later */
#if(_WIN32_WINNT >= 0x0400)


/*************************************************************************
*
* Function: UDFFastIoQueryNetInfo()
*
* Description:
*   Get information requested by a redirector across the network. This call
*   will originate from the LAN Manager server.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: TRUE/FALSE
*
*************************************************************************/
BOOLEAN
NTAPI
UDFFastIoQueryNetInfo(
    IN PFILE_OBJECT                                 FileObject,
    IN BOOLEAN                                      Wait,
    OUT PFILE_NETWORK_OPEN_INFORMATION              Buffer,
    OUT PIO_STATUS_BLOCK                            IoStatus,
    IN PDEVICE_OBJECT                               DeviceObject)
{
    BOOLEAN          ReturnedStatus = FALSE;     // fast i/o failed/not allowed
    NTSTATUS         RC = STATUS_SUCCESS;
    PtrUDFIrpContext PtrIrpContext = NULL;
    LONG             Length = sizeof(FILE_NETWORK_OPEN_INFORMATION);
    PtrUDFFCB        Fcb;
    PtrUDFCCB        Ccb;
    PtrUDFNTRequiredFCB NtReqFcb = NULL;
    BOOLEAN          MainResourceAcquired = FALSE;

    FsRtlEnterFileSystem();

    KdPrint(("UDFFastIo  \n"));
    // if the file is already opended we can satisfy this request
    // immediately 'cause all the data we need must be cached
    _SEH2_TRY {

        _SEH2_TRY {

            // Get the FCB and CCB pointers.
            Ccb = (PtrUDFCCB)(FileObject->FsContext2);
            ASSERT(Ccb);
            Fcb = Ccb->Fcb;
            ASSERT(Fcb);
            NtReqFcb = Fcb->NTRequiredFCB;
            //Fcb->Vcb->VCBFlags |= UDF_VCB_SKIP_EJECT_CHECK;

            if (!(Fcb->FCBFlags & UDF_FCB_PAGE_FILE)) {
                // Acquire the MainResource shared.
                UDF_CHECK_PAGING_IO_RESOURCE(NtReqFcb);
                if (!UDFAcquireResourceShared(&(NtReqFcb->MainResource), Wait)) {
                    try_return(RC = STATUS_CANT_WAIT);
                }
                MainResourceAcquired = TRUE;
            }

            ReturnedStatus =
                ((RC = UDFGetNetworkInformation(Fcb, Buffer, &Length)) == STATUS_SUCCESS);

        } _SEH2_EXCEPT(UDFExceptionFilter(PtrIrpContext, _SEH2_GetExceptionInformation())) {

            RC = UDFExceptionHandler(PtrIrpContext, NULL);

            UDFLogEvent(UDF_ERROR_INTERNAL_ERROR, RC);

        } _SEH2_END;
try_exit: NOTHING;
    } _SEH2_FINALLY {
        if (MainResourceAcquired) {
            UDF_CHECK_PAGING_IO_RESOURCE(NtReqFcb);
            UDFReleaseResource(&(NtReqFcb->MainResource));
            MainResourceAcquired = FALSE;
        }
        IoStatus->Status = RC;
        if(ReturnedStatus) {
            IoStatus->Information = sizeof(FILE_NETWORK_OPEN_INFORMATION);
        } else {
            IoStatus->Information = 0;
        }
    } _SEH2_END;
    
    FsRtlExitFileSystem();

    return(ReturnedStatus);

} // end UDFFastIoQueryNetInfo()


/*************************************************************************
*
* Function: UDFFastIoMdlRead()
*
* Description:
*   Bypass the traditional IRP method to perform a MDL read operation.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: TRUE/FALSE
*
*************************************************************************/
/*BOOLEAN UDFFastIoMdlRead(
IN PFILE_OBJECT             FileObject,
IN PLARGE_INTEGER           FileOffset,
IN ULONG                    Length,
IN ULONG                    LockKey,
OUT PMDL*                   MdlChain,
OUT PIO_STATUS_BLOCK        IoStatus,
IN PDEVICE_OBJECT           DeviceObject)
{
    BOOLEAN ReturnedStatus = FALSE;     // fast i/o failed/not allowed
    NTSTATUS RC = STATUS_SUCCESS;
    PtrUDFIrpContext PtrIrpContext = NULL;

    FsRtlEnterFileSystem();

    _SEH2_TRY {

        _SEH2_TRY {
    
            // See description in UDFFastIoRead() before filling-in the
            // stub here.
            NOTHING;
    
    
        } __except (UDFExceptionFilter(PtrIrpContext, GetExceptionInformation())) {
    
            RC = UDFExceptionHandler(PtrIrpContext, NULL);
    
            UDFLogEvent(UDF_ERROR_INTERNAL_ERROR, RC);

        }

        //try_exit: NOTHING;

    } _SEH2_FINALLY {

    }
    
    FsRtlExitFileSystem();

    return(ReturnedStatus);
}*/


/*************************************************************************
*
* Function: UDFFastIoMdlReadComplete()
*
* Description:
*   Bypass the traditional IRP method to inform the NT Cache Manager and the
*   FSD that the caller no longer requires the data locked in the system cache
*   or the MDL to stay around anymore ..
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: TRUE/FALSE
*
*************************************************************************/
/*BOOLEAN UDFFastIoMdlReadComplete(
IN PFILE_OBJECT             FileObject,
OUT PMDL                            MdlChain,
IN PDEVICE_OBJECT               DeviceObject)
{
    BOOLEAN             ReturnedStatus = FALSE;     // fast i/o failed/not allowed
    NTSTATUS                RC = STATUS_SUCCESS;
   PtrUDFIrpContext PtrIrpContext = NULL;

    FsRtlEnterFileSystem();

    _SEH2_TRY {

        _SEH2_TRY {
    
            // See description in UDFFastIoRead() before filling-in the
            // stub here.
            NOTHING;
        
        } __except (UDFExceptionFilter(PtrIrpContext, GetExceptionInformation())) {
    
            RC = UDFExceptionHandler(PtrIrpContext, NULL);
    
            UDFLogEvent(UDF_ERROR_INTERNAL_ERROR, RC);

        }

        //try_exit: NOTHING;

    } _SEH2_FINALLY {

    }
    
    FsRtlExitFileSystem();

    return(ReturnedStatus);
}*/


/*************************************************************************
*
* Function: UDFFastIoPrepareMdlWrite()
*
* Description:
*   Bypass the traditional IRP method to prepare for a MDL write operation.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: TRUE/FALSE
*
*************************************************************************/
/*BOOLEAN
UDFFastIoPrepareMdlWrite(
    IN PFILE_OBJECT      FileObject,
    IN PLARGE_INTEGER    FileOffset,
    IN ULONG             Length,
    IN ULONG             LockKey,
    OUT PMDL             *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT    DeviceObject
    )
{
    BOOLEAN              ReturnedStatus = FALSE; // fast i/o failed/not allowed
    NTSTATUS             RC = STATUS_SUCCESS;
   PtrUDFIrpContext PtrIrpContext = NULL;

    FsRtlEnterFileSystem();

    _SEH2_TRY {

        _SEH2_TRY {
    
            // See description in UDFFastIoRead() before filling-in the
            // stub here.
            NOTHING;
        
        } __except (UDFExceptionFilter(PtrIrpContext, GetExceptionInformation())) {
    
            RC = UDFExceptionHandler(PtrIrpContext, NULL);
    
            UDFLogEvent(UDF_ERROR_INTERNAL_ERROR, RC);

        }

        //try_exit: NOTHING;

    } _SEH2_FINALLY {

    }
    
    FsRtlExitFileSystem();

    return(ReturnedStatus);
}*/


/*************************************************************************
*
* Function: UDFFastIoMdlWriteComplete()
*
* Description:
*   Bypass the traditional IRP method to inform the NT Cache Manager and the
*   FSD that the caller has updated the contents of the MDL. This data can
*   now be asynchronously written out to secondary storage by the Cache Mgr.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: TRUE/FALSE
*
*************************************************************************/
/*BOOLEAN UDFFastIoMdlWriteComplete(
IN PFILE_OBJECT             FileObject,
IN PLARGE_INTEGER               FileOffset,
OUT PMDL                            MdlChain,
IN PDEVICE_OBJECT               DeviceObject)
{
    BOOLEAN             ReturnedStatus = FALSE;     // fast i/o failed/not allowed
    NTSTATUS                RC = STATUS_SUCCESS;
   PtrUDFIrpContext PtrIrpContext = NULL;

    FsRtlEnterFileSystem();

    _SEH2_TRY {

        _SEH2_TRY {
    
            // See description in UDFFastIoRead() before filling-in the
            // stub here.
            NOTHING;
        
        } __except (UDFExceptionFilter(PtrIrpContext, GetExceptionInformation())) {
    
            RC = UDFExceptionHandler(PtrIrpContext, NULL);
    
            UDFLogEvent(UDF_ERROR_INTERNAL_ERROR, RC);

        }

        //try_exit: NOTHING;

    } _SEH2_FINALLY {

    }
    
    FsRtlExitFileSystem();

    return(ReturnedStatus);
}*/


/*************************************************************************
*
* Function: UDFFastIoAcqModWrite()
*
* Description:
*   Not really a fast-io operation. Used by the VMM to acquire FSD resources
*   before initiating a write operation via the Modified Page/Block Writer.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: STATUS_SUCCESS/Error (__try not to return an error, will 'ya ? :-)
*
*************************************************************************/
NTSTATUS
NTAPI
UDFFastIoAcqModWrite(
    IN PFILE_OBJECT   FileObject,
    IN PLARGE_INTEGER EndingOffset,
    OUT PERESOURCE    *ResourceToRelease,
    IN PDEVICE_OBJECT DeviceObject)
{
    NTSTATUS RC = STATUS_SUCCESS;

    FsRtlEnterFileSystem();

    MmPrint(("  AcqModW %I64x\n", EndingOffset->QuadPart));

#define NtReqFcb ((PtrUDFNTRequiredFCB)(FileObject->FsContext))

    // We must determine which resource(s) we would like to
    // acquire at this time. We know that a write is imminent;
    // we will probably therefore acquire appropriate resources
    // exclusively.

    // We must first get the FCB and CCB pointers from the file object
    // that is passed in to this function (as an argument). Note that
    // the ending offset (when examined in conjunction with current valid data
    // length) may help us in determining the appropriate resource(s) to acquire.

    // For example, if the ending offset is beyond current valid data length,
    // We may decide to acquire *both* the MainResource and the PagingIoResource
    // exclusively; otherwise, we may decide simply to acquire the PagingIoResource.

    // Consult the text for more information on synchronization in FSDs.

    // One final note; the VMM expects that we will return a pointer to
    // the resource that we acquired (single return value). This pointer
    // will be returned back to we in the release call (below).

    if(UDFAcquireResourceShared(&(NtReqFcb->PagingIoResource), FALSE)) {
        if(EndingOffset->QuadPart <= NtReqFcb->CommonFCBHeader.ValidDataLength.QuadPart) {
            UDFReleaseResource(&(NtReqFcb->PagingIoResource));
            RC = STATUS_CANT_WAIT;
        } else {
            NtReqFcb->AcqFlushCount++;
            (*ResourceToRelease) = &(NtReqFcb->PagingIoResource);
            MmPrint(("    AcqModW OK\n"));
        }
    } else {
        RC = STATUS_CANT_WAIT;
    }

#undef NtReqFcb

    FsRtlExitFileSystem();

    return RC;
} // end UDFFastIoAcqModWrite()


/*************************************************************************
*
* Function: UDFFastIoRelModWrite()
*
* Description:
*   Not really a fast-io operation. Used by the VMM to release FSD resources
*   after processing a modified page/block write operation.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: STATUS_SUCCESS/Error (an error returned here is really not expected!)
*
*************************************************************************/
NTSTATUS
NTAPI
UDFFastIoRelModWrite(
    IN PFILE_OBJECT   FileObject,
    IN PERESOURCE     ResourceToRelease,
    IN PDEVICE_OBJECT DeviceObject)
{
    FsRtlEnterFileSystem();

    MmPrint(("  RelModW\n"));

#define NtReqFcb ((PtrUDFNTRequiredFCB)(FileObject->FsContext))

    // The MPW has complete the write for modified pages and therefore
    // wants us to release pre-acquired resource(s).

    // We must undo here whatever it is that we did in the
    // UDFFastIoAcqModWrite() call above.

    NtReqFcb->AcqFlushCount--;
    ASSERT(ResourceToRelease == &(NtReqFcb->PagingIoResource));
    UDFReleaseResource(ResourceToRelease);

#undef NtReqFcb

    FsRtlExitFileSystem();

    return(STATUS_SUCCESS);
} // end UDFFastIoRelModWrite()


/*************************************************************************
*
* Function: UDFFastIoAcqCcFlush()
*
* Description:
*   Not really a fast-io operation. Used by the NT Cache Mgr to acquire FSD
*   resources before performing a CcFlush() operation on a specific file
*   stream.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: STATUS_SUCCESS/Error
*
*************************************************************************/
NTSTATUS
NTAPI
UDFFastIoAcqCcFlush(
    IN PFILE_OBJECT         FileObject,
    IN PDEVICE_OBJECT       DeviceObject)
{
//    NTSTATUS                RC = STATUS_SUCCESS;

    FsRtlEnterFileSystem();

    MmPrint(("  AcqCcFlush\n"));

    // Acquire appropriate resources that will allow correct synchronization
    // with a flush call (and avoid deadlock).
    
#define NtReqFcb ((PtrUDFNTRequiredFCB)(FileObject->FsContext))

//    UDFAcquireResourceExclusive(&(NtReqFcb->MainResource), TRUE);
    UDFAcquireResourceExclusive(&(NtReqFcb->PagingIoResource), TRUE);
//    ASSERT(!(NtReqFcb->AcqFlushCount));
    NtReqFcb->AcqFlushCount++;

#undef NtReqFcb

    FsRtlExitFileSystem();

    return(STATUS_SUCCESS);

} // end UDFFastIoAcqCcFlush()

/*************************************************************************
*
* Function: UDFFastIoRelCcFlush()
*
* Description:
*   Not really a fast-io operation. Used by the NT Cache Mgr to acquire FSD
*   resources before performing a CcFlush() operation on a specific file
*   stream.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: STATUS_SUCCESS/Error
*
*************************************************************************/
NTSTATUS
NTAPI
UDFFastIoRelCcFlush(
    IN PFILE_OBJECT         FileObject,
    IN PDEVICE_OBJECT       DeviceObject
    )
{
//    NTSTATUS                RC = STATUS_SUCCESS;

    FsRtlEnterFileSystem();

    MmPrint(("  RelCcFlush\n"));

#define NtReqFcb ((PtrUDFNTRequiredFCB)(FileObject->FsContext))

    // Release resources acquired in UDFFastIoAcqCcFlush() above.

    NtReqFcb->AcqFlushCount--;
    UDFReleaseResource(&(NtReqFcb->PagingIoResource));
//    UDFReleaseResource(&(NtReqFcb->MainResource));

#undef NtReqFcb

    FsRtlExitFileSystem();

    return(STATUS_SUCCESS);

} // end UDFFastIoRelCcFlush()


/*BOOLEAN
UDFFastIoDeviceControl (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )
{
    switch(IoControlCode) {
    case FSCTL_ALLOW_EXTENDED_DASD_IO: {
        IoStatus->Information = 0;
        IoStatus->Status = STATUS_SUCCESS;
        break;
    }
    case FSCTL_IS_VOLUME_MOUNTED: {
        PtrUDFFCB Fcb;
        PtrUDFCCB Ccb;

        Ccb = (PtrUDFCCB)(FileObject->FsContext2);
        Fcb = Ccb->Fcb;

        if(Fcb &&
           !(Fcb->Vcb->VCBFlags & UDF_VCB_FLAGS_RAW_DISK) &&
           !(Fcb->Vcb->VCBFlags & UDF_VCB_FLAGS_VOLUME_LOCKED) ) {
            return FALSE;
        }

        IoStatus->Information = 0;
        IoStatus->Status = STATUS_SUCCESS;

        break;
    }
    default:
        return FALSE;
    }
    return TRUE;
}*/

#endif  //_WIN32_WINNT >= 0x0400

BOOLEAN
NTAPI
UDFFastIoCopyWrite (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )
{
    PtrUDFFCB           Fcb = NULL;
    PtrUDFCCB           Ccb = NULL;

    // Obtain a pointer to the FCB and CCB for the file stream.
    Ccb = (PtrUDFCCB)(FileObject->FsContext2);
    ASSERT(Ccb);
    Fcb = Ccb->Fcb;
    ASSERT(Fcb);

    // back pressure for very smart and fast system cache ;)
    if(Fcb->Vcb->VerifyCtx.QueuedCount ||
       Fcb->Vcb->VerifyCtx.ItemCount >= UDF_MAX_VERIFY_CACHE) {
        AdPrint(("    Verify queue overflow -> UDFFastIoCopyWrite() = FALSE\n"));
        return FALSE;
    }
    if(Fcb->NTRequiredFCB->SectionObject.DataSectionObject &&
       Length >= 0x10000 &&
       FileOffset->LowPart &&
       !(FileOffset->LowPart & 0x00ffffff)) {

        MmPrint(("    no FastIo 16Mb\n"));
        return FALSE;
    }
    return FsRtlCopyWrite(FileObject, FileOffset, Length, Wait, LockKey, Buffer, IoStatus, DeviceObject);

} // end UDFFastIoCopyWrite()
