/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    UdfData.c

Abstract:

    This module declares the global data used by the Udfs file system.

    This module also handles the dispath routines in the Fsd threads as well as
    handling the IrpContext and Irp through the exception path.

Author:

    Dan Lovinger    [DanLo]   24-May-1996

Revision History:

--*/

#include "UdfProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (UDFS_BUG_CHECK_UDFDATA)

//
//  The local debug trace level
//

#define Dbg                              (UDFS_DEBUG_LEVEL_UDFDATA)

//
//  Global data structures
//

UDF_DATA UdfData;
FAST_IO_DISPATCH UdfFastIoDispatch;

//
//  Debug trace levels
//

#ifdef UDF_SANITY

//
//  For UdfDebugTrace (only live in checked builds) to be able to swing
//  variable argument lists and varargs printfs.
//

#include <stdarg.h>
#include <stdio.h>

BOOLEAN UdfTestTopLevel = TRUE;
BOOLEAN UdfTestRaisedStatus = TRUE;

LONG UdfDebugTraceLevel = 0;
LONG UdfDebugTraceIndent = 0;

//
//  Control whether UdfVerifyDescriptor will only emit info on failure (FALSE) or
//  all of the time (TRUE).
//

BOOLEAN UdfNoisyVerifyDescriptor = FALSE;

#endif

//
//  Reserved directory strings.
//

WCHAR UdfUnicodeSelfArray[] = { L'.' };
WCHAR UdfUnicodeParentArray[] = { L'.', L'.' };

UNICODE_STRING UdfUnicodeDirectoryNames[] = {
    { sizeof(UdfUnicodeSelfArray), sizeof(UdfUnicodeSelfArray), UdfUnicodeSelfArray},
    { sizeof(UdfUnicodeParentArray), sizeof(UdfUnicodeParentArray), UdfUnicodeParentArray}
    };

//
//  Identifier strings defined by UDF.
//

CHAR UdfCS0IdentifierArray[] = { 'O', 'S', 'T', 'A', ' ',
                                 'C', 'o', 'm', 'p', 'r', 'e', 's', 's', 'e', 'd', ' ',
                                 'U', 'n', 'i', 'c', 'o', 'd', 'e' };

STRING UdfCS0Identifier = {
    sizeof(UdfCS0IdentifierArray),
    sizeof(UdfCS0IdentifierArray),
    UdfCS0IdentifierArray
    };

CHAR UdfDomainIdentifierArray[] = { '*', 'O', 'S', 'T', 'A', ' ',
                                    'U', 'D', 'F', ' ',
                                    'C', 'o', 'm', 'p', 'l', 'i', 'a', 'n', 't' };

STRING UdfDomainIdentifier = {
    sizeof(UdfDomainIdentifierArray),
    sizeof(UdfDomainIdentifierArray),
    UdfDomainIdentifierArray
    };

CHAR UdfVirtualPartitionDomainIdentifierArray[] = { '*', 'U', 'D', 'F', ' ',
                                                    'V', 'i', 'r', 't', 'u', 'a', 'l', ' ',
                                                    'P', 'a', 'r', 't', 'i', 't', 'i', 'o', 'n' };

STRING UdfVirtualPartitionDomainIdentifier = {
    sizeof(UdfVirtualPartitionDomainIdentifierArray),
    sizeof(UdfVirtualPartitionDomainIdentifierArray),
    UdfVirtualPartitionDomainIdentifierArray 
    };

CHAR UdfVatTableIdentifierArray[] = { '*', 'U', 'D', 'F', ' ',
                                      'V', 'i', 'r', 't', 'u', 'a', 'l', ' ',
                                      'A', 'l', 'l', 'o', 'c', ' ',
                                      'T', 'b', 'l' };

STRING UdfVatTableIdentifier = {
    sizeof(UdfVatTableIdentifierArray),
    sizeof(UdfVatTableIdentifierArray),
    UdfVatTableIdentifierArray
    };
                                    
CHAR UdfSparablePartitionDomainIdentifierArray[] = { '*', 'U', 'D', 'F', ' ',
                                                     'S', 'p', 'a', 'r', 'a', 'b', 'l', 'e', ' ',
                                                     'P', 'a', 'r', 't', 'i', 't', 'i', 'o', 'n' };

STRING UdfSparablePartitionDomainIdentifier = {
    sizeof(UdfSparablePartitionDomainIdentifierArray),
    sizeof(UdfSparablePartitionDomainIdentifierArray),
    UdfSparablePartitionDomainIdentifierArray
    };

CHAR UdfSparingTableIdentifierArray[] = { '*', 'U', 'D', 'F', ' ',
                                          'S', 'p', 'a', 'r', 'i', 'n', 'g', ' ',
                                          'T', 'a', 'b', 'l', 'e' };

STRING UdfSparingTableIdentifier = {
    sizeof(UdfSparingTableIdentifierArray),
    sizeof(UdfSparingTableIdentifierArray),
    UdfSparingTableIdentifierArray
    };

CHAR UdfNSR02IdentifierArray[] = NSR_PART_CONTID_NSR02;

STRING UdfNSR02Identifier = {
    sizeof(UdfNSR02IdentifierArray),
    sizeof(UdfNSR02IdentifierArray),
    UdfNSR02IdentifierArray
    };

//
//  Tables of tokens we have to parse up from mount-time on-disk structures
//

PARSE_KEYVALUE VsdIdentParseTable[] = {
    { VSD_IDENT_BEA01, VsdIdentBEA01 },
    { VSD_IDENT_TEA01, VsdIdentTEA01 },
    { VSD_IDENT_CDROM, VsdIdentCDROM },
    { VSD_IDENT_CD001, VsdIdentCD001 },
    { VSD_IDENT_CDW01, VsdIdentCDW01 },
    { VSD_IDENT_CDW02, VsdIdentCDW02 },
    { VSD_IDENT_NSR01, VsdIdentNSR01 },
    { VSD_IDENT_NSR02, VsdIdentNSR02 },
    { VSD_IDENT_BOOT2, VsdIdentBOOT2 },
    { NULL,            VsdIdentBad }
    };

PARSE_KEYVALUE NsrPartContIdParseTable[] = {
    { NSR_PART_CONTID_FDC01, NsrPartContIdFDC01 },
    { NSR_PART_CONTID_CD001, NsrPartContIdCD001 },
    { NSR_PART_CONTID_CDW01, NsrPartContIdCDW01 },
    { NSR_PART_CONTID_CDW02, NsrPartContIdCDW02 },
    { NSR_PART_CONTID_NSR01, NsrPartContIdNSR01 },
    { NSR_PART_CONTID_NSR02, NsrPartContIdNSR02 },
    { NULL,                  NsrPartContIdBad }
    };

//
//  Lookaside allocation lists for various volatile structures
//

NPAGED_LOOKASIDE_LIST UdfFcbNonPagedLookasideList;
NPAGED_LOOKASIDE_LIST UdfIrpContextLookasideList;

PAGED_LOOKASIDE_LIST UdfCcbLookasideList;
PAGED_LOOKASIDE_LIST UdfFcbIndexLookasideList;
PAGED_LOOKASIDE_LIST UdfFcbDataLookasideList;
PAGED_LOOKASIDE_LIST UdfLcbLookasideList;

//
//  16bit CRC table
//

PUSHORT UdfCrcTable;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, UdfComputeCrc16)
#pragma alloc_text(PAGE, UdfComputeCrc16Uni)
#ifdef UDF_SANITY
#pragma alloc_text(PAGE, UdfDebugTrace)
#endif
#pragma alloc_text(PAGE, UdfFastIoCheckIfPossible)
#pragma alloc_text(PAGE, UdfHighBit)
#pragma alloc_text(PAGE, UdfInitializeCrc16)
#pragma alloc_text(PAGE, UdfSerial32)
#endif


NTSTATUS
UdfFsdDispatch (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the driver entry to all of the Fsd dispatch points.

    Conceptually the Io routine will call this routine on all requests
    to the file system.  We case on the type of request and invoke the
    correct handler for this type of request.  There is an exception filter
    to catch any exceptions in the UDFS code as well as the UDFS process
    exception routine.

    This routine allocates and initializes the IrpContext for this request as
    well as updating the top-level thread context as necessary.  We may loop
    in this routine if we need to retry the request for any reason.  The
    status code STATUS_CANT_WAIT is used to indicate this.  Suppose the disk
    in the drive has changed.  An Fsd request will proceed normally until it
    recognizes this condition.  STATUS_VERIFY_REQUIRED is raised at that point
    and the exception code will handle the verify and either return
    STATUS_CANT_WAIT or STATUS_PENDING depending on whether the request was
    posted.

Arguments:

    VolumeDeviceObject - Supplies the volume device object for this request

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The FSD status for the IRP

--*/

{
    THREAD_CONTEXT ThreadContext;
    PIRP_CONTEXT IrpContext = NULL;
    BOOLEAN Wait;

#ifdef UDF_SANITY
    PVOID PreviousTopLevel;
#endif

    NTSTATUS Status;

    KIRQL SaveIrql = KeGetCurrentIrql();

    ASSERT_OPTIONAL_IRP( Irp );

    FsRtlEnterFileSystem();

#ifdef UDF_SANITY
    PreviousTopLevel = IoGetTopLevelIrp();
#endif

    //
    //  Loop until this request has been completed or posted.
    //

    do {

        //
        //  Use a try-except to handle the exception cases.
        //

        try {

            //
            //  If the IrpContext is NULL then this is the first pass through
            //  this loop.
            //

            if (IrpContext == NULL) {

                //
                //  Decide if this request is waitable an allocate the IrpContext.
                //  If the file object in the stack location is NULL then this
                //  is a mount which is always waitable.  Otherwise we look at
                //  the file object flags.
                //

                if (IoGetCurrentIrpStackLocation( Irp )->FileObject == NULL) {

                    Wait = TRUE;

                } else {

                    Wait = CanFsdWait( Irp );
                }

                IrpContext = UdfCreateIrpContext( Irp, Wait );

                //
                //  Update the thread context information.
                //

                UdfSetThreadContext( IrpContext, &ThreadContext );

#ifdef UDF_SANITY
                ASSERT( !UdfTestTopLevel ||
                        SafeNodeType( IrpContext->TopLevel ) == UDFS_NTC_IRP_CONTEXT );
#endif

            //
            //  Otherwise cleanup the IrpContext for the retry.
            //

            } else {

                //
                //  Set the MORE_PROCESSING flag to make sure the IrpContext
                //  isn't inadvertently deleted here.  Then cleanup the
                //  IrpContext to perform the retry.
                //

                SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_MORE_PROCESSING );
                UdfCleanupIrpContext( IrpContext, FALSE );
            }

            //
            //  Case on the major irp code.
            //

            switch (IrpContext->MajorFunction) {

                case IRP_MJ_CLEANUP :
                    
                    Status = UdfCommonCleanup( IrpContext, Irp );
                    break;
    
                case IRP_MJ_CLOSE :

                    Status = UdfCommonClose( IrpContext, Irp );
                    break;

                case IRP_MJ_CREATE :
                    
                    Status = UdfCommonCreate( IrpContext, Irp );
                    break;
    
                case IRP_MJ_DEVICE_CONTROL :
    
                    Status = UdfCommonDevControl( IrpContext, Irp );
                    break;
    
                case IRP_MJ_DIRECTORY_CONTROL :

                    Status = UdfCommonDirControl( IrpContext, Irp );
                    break;

                case IRP_MJ_FILE_SYSTEM_CONTROL :
    
                    Status = UdfCommonFsControl( IrpContext, Irp );
                    break;

                case IRP_MJ_LOCK_CONTROL :

                    Status = UdfCommonLockControl( IrpContext, Irp );
                    break;

                case IRP_MJ_PNP :

                    Status = UdfCommonPnp( IrpContext, Irp );
                    break;

                case IRP_MJ_QUERY_INFORMATION :

                    Status = UdfCommonQueryInfo( IrpContext, Irp );
                    break;
                
                case IRP_MJ_QUERY_VOLUME_INFORMATION :

                    Status = UdfCommonQueryVolInfo( IrpContext, Irp );
                    break;
                
                case IRP_MJ_READ :
    
                    //
                    //  If this is an Mdl complete request, don't go through
                    //  common read.
                    //
    
                    if (FlagOn( IrpContext->MinorFunction, IRP_MN_COMPLETE )) {
    
                        Status = UdfCompleteMdl( IrpContext, Irp );
    
                    } else {
    
                        Status = UdfCommonRead( IrpContext, Irp );
                    }
    
                    break;

                case IRP_MJ_SET_INFORMATION :

                    Status = UdfCommonSetInfo( IrpContext, Irp );
                    break;
                
                default :
                            
                    Status = STATUS_INVALID_DEVICE_REQUEST;
                    UdfCompleteRequest( IrpContext, Irp, Status );
            }

        } except( UdfExceptionFilter( IrpContext, GetExceptionInformation() )) {

            Status = UdfProcessException( IrpContext, Irp, GetExceptionCode() );
        }

    } while (Status == STATUS_CANT_WAIT);

#ifdef UDF_SANITY
    ASSERT( !UdfTestTopLevel ||
            (PreviousTopLevel == IoGetTopLevelIrp()) );
#endif

    FsRtlExitFileSystem();

    ASSERT( SaveIrql == KeGetCurrentIrql( ));

    return Status;
}


LONG
UdfExceptionFilter (
    IN PIRP_CONTEXT IrpContext,
    IN PEXCEPTION_POINTERS ExceptionPointer
    )

/*++

Routine Description:

    This routine is used to decide whether we will handle a raised exception
    status.  If UDFS explicitly raised an error then this status is already
    in the IrpContext.  We choose which is the correct status code and
    either indicate that we will handle the exception or bug-check the system.

Arguments:

    ExceptionCode - Supplies the exception code to being checked.

Return Value:

    ULONG - returns EXCEPTION_EXECUTE_HANDLER or bugchecks

--*/

{
    NTSTATUS ExceptionCode;
    BOOLEAN TestStatus = TRUE;

    ASSERT_OPTIONAL_IRP_CONTEXT( IrpContext );

    ExceptionCode = ExceptionPointer->ExceptionRecord->ExceptionCode;

    DebugTrace(( 0, Dbg,
                 "UdfExceptionFilter: %08x (exr %08x cxr %08x)\n",
                 ExceptionCode,
                 ExceptionPointer->ExceptionRecord,
                 ExceptionPointer->ContextRecord ));


    //
    // If the exception is STATUS_IN_PAGE_ERROR, get the I/O error code
    // from the exception record.
    //

    if ((ExceptionCode == STATUS_IN_PAGE_ERROR) &&
        (ExceptionPointer->ExceptionRecord->NumberParameters >= 3)) {

        ExceptionCode = (NTSTATUS) ExceptionPointer->ExceptionRecord->ExceptionInformation[2];
    }

    //
    //  If there is an Irp context then check which status code to use.
    //

    if (ARGUMENT_PRESENT( IrpContext )) {

        if (IrpContext->ExceptionStatus == STATUS_SUCCESS) {

            //
            //  Store the real status into the IrpContext.
            //

            IrpContext->ExceptionStatus = ExceptionCode;

        } else {

            //
            //  No need to test the status code if we raised it ourselves.
            //

            TestStatus = FALSE;
        }
    }

    //
    //  Bug check if this status is not supported.
    //

    if (TestStatus && !FsRtlIsNtstatusExpected( ExceptionCode )) {

        UdfBugCheck( (ULONG_PTR) ExceptionPointer->ExceptionRecord,
                     (ULONG_PTR) ExceptionPointer->ContextRecord,
                     (ULONG_PTR) ExceptionPointer->ExceptionRecord->ExceptionAddress );

    }

    return EXCEPTION_EXECUTE_HANDLER;
}


NTSTATUS
UdfProcessException (
    IN PIRP_CONTEXT IrpContext OPTIONAL,
    IN PIRP Irp,
    IN NTSTATUS ExceptionCode
    )

/*++

Routine Description:

    This routine processes an exception.  It either completes the request
    with the exception status in the IrpContext, sends this off to the Fsp
    workque or causes it to be retried in the current thread if a verification
    is needed.

    If the volume needs to be verified (STATUS_VERIFY_REQUIRED) and we can
    do the work in the current thread we will translate the status code
    to STATUS_CANT_WAIT to indicate that we need to retry the request.

Arguments:

    Irp - Supplies the Irp being processed

    ExceptionCode - Supplies the normalized exception status being handled

Return Value:

    NTSTATUS - Returns the results of either posting the Irp or the
        saved completion status.

--*/

{
    PDEVICE_OBJECT Device;
    PVPB Vpb;
    PETHREAD Thread;

    ASSERT_OPTIONAL_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );

    //
    //  If there is not an irp context, then complete the request with the
    //  current status code.
    //

    if (!ARGUMENT_PRESENT( IrpContext )) {

        UdfCompleteRequest( NULL, Irp, ExceptionCode );
        return ExceptionCode;
    }

    //
    //  Get the real exception status from the IrpContext.
    //

    ExceptionCode = IrpContext->ExceptionStatus;

    //
    //  If we are not a top level request then we just complete the request
    //  with the current status code.
    //

    if (!FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_TOP_LEVEL )) {

        UdfCompleteRequest( IrpContext, Irp, ExceptionCode );

        return ExceptionCode;
    }

    //
    //  Check if we are posting this request.  One of the following must be true
    //  if we are to post a request.
    //
    //      - Status code is STATUS_CANT_WAIT and the request is asynchronous
    //          or we are forcing this to be posted.
    //
    //      - Status code is STATUS_VERIFY_REQUIRED and we are at APC level
    //          or higher.  Can't wait for IO in the verify path in this case.
    //
    //  Set the MORE_PROCESSING flag in the IrpContext to keep if from being
    //  deleted if this is a retryable condition.
    //
    //  Note:  Children of UdfFsdPostRequest() can raise.
    //

    try {
    
        if (ExceptionCode == STATUS_CANT_WAIT) {

            if (FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_FORCE_POST )) {

                ExceptionCode = UdfFsdPostRequest( IrpContext, Irp );
            }

        } else if (ExceptionCode == STATUS_VERIFY_REQUIRED) {

            if (KeGetCurrentIrql() >= APC_LEVEL) {

                ExceptionCode = UdfFsdPostRequest( IrpContext, Irp );
            }
        }
    }
    except (UdfExceptionFilter( IrpContext, GetExceptionInformation()))  {
    
        ExceptionCode = GetExceptionCode(); 
    }

    //
    //  If we posted the request or our caller will retry then just return here.
    //

    if ((ExceptionCode == STATUS_PENDING) ||
        (ExceptionCode == STATUS_CANT_WAIT)) {

        return ExceptionCode;
    }

    ClearFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_MORE_PROCESSING );

    //
    //  Store this error into the Irp for posting back to the Io system.
    //

    Irp->IoStatus.Status = ExceptionCode;

    if (IoIsErrorUserInduced( ExceptionCode )) {

        //
        //  Check for the various error conditions that can be caused by,
        //  and possibly resolved my the user.
        //

        if (ExceptionCode == STATUS_VERIFY_REQUIRED) {

            //
            //  Now we are at the top level file system entry point.
            //
            //  If we have already posted this request then the device to
            //  verify is in the original thread.  Find this via the Irp.
            //

            Device = IoGetDeviceToVerify( Irp->Tail.Overlay.Thread );
            IoSetDeviceToVerify( Irp->Tail.Overlay.Thread, NULL );

            //
            //  If there is no device in that location then check in the
            //  current thread.
            //

            if (Device == NULL) {

                Device = IoGetDeviceToVerify( PsGetCurrentThread() );
                IoSetDeviceToVerify( PsGetCurrentThread(), NULL );

                ASSERT( Device != NULL );

                //
                //  Let's not BugCheck just because the driver screwed up.
                //

                if (Device == NULL) {

                    ExceptionCode = STATUS_DRIVER_INTERNAL_ERROR;

                    UdfCompleteRequest( IrpContext, Irp, ExceptionCode );

                    return ExceptionCode;
                }
            }

            //
            //  CdPerformVerify() will do the right thing with the Irp.
            //  If we return STATUS_CANT_WAIT then the current thread
            //  can retry the request.
            //

            return UdfPerformVerify( IrpContext, Irp, Device );
        }

        //
        //  The other user induced conditions generate an error unless
        //  they have been disabled for this request.
        //

        if (FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_DISABLE_POPUPS )) {

            UdfCompleteRequest( IrpContext, Irp, ExceptionCode );

            return ExceptionCode;

        } else {

            //
            //  Generate a pop-up
            //

            if (IoGetCurrentIrpStackLocation( Irp )->FileObject != NULL) {

                Vpb = IoGetCurrentIrpStackLocation( Irp )->FileObject->Vpb;

            } else {

                Vpb = NULL;
            }

            //
            //  The device to verify is either in my thread local storage
            //  or that of the thread that owns the Irp.
            //

            Thread = Irp->Tail.Overlay.Thread;
            Device = IoGetDeviceToVerify( Thread );

            if (Device == NULL) {

                Thread = PsGetCurrentThread();
                Device = IoGetDeviceToVerify( Thread );

                ASSERT( Device != NULL );

                //
                //  Let's not BugCheck just because the driver screwed up.
                //

                if (Device == NULL) {

                    UdfCompleteRequest( IrpContext, Irp, ExceptionCode );

                    return ExceptionCode;
                }
            }

            //
            //  This routine actually causes the pop-up.  It usually
            //  does this by queuing an APC to the callers thread,
            //  but in some cases it will complete the request immediately,
            //  so it is very important to IoMarkIrpPending() first.
            //

            IoMarkIrpPending( Irp );
            IoRaiseHardError( Irp, Vpb, Device );

            //
            //  We will be handing control back to the caller here, so
            //  reset the saved device object.
            //

            IoSetDeviceToVerify( Thread, NULL );

            //
            //  The Irp will be completed by Io or resubmitted.  In either
            //  case we must clean up the IrpContext here.
            //

            UdfCompleteRequest( IrpContext, NULL, STATUS_SUCCESS );
            return STATUS_PENDING;
        }
    }

    //
    //  This is just a run of the mill error.
    //

    UdfCompleteRequest( IrpContext, Irp, ExceptionCode );

    return ExceptionCode;
}


VOID
UdfCompleteRequest (
    IN PIRP_CONTEXT IrpContext OPTIONAL,
    IN PIRP Irp OPTIONAL,
    IN NTSTATUS Status
    )

/*++

Routine Description:

    This routine completes a Irp and cleans up the IrpContext.  Either or
    both of these may not be specified.

Arguments:

    Irp - Supplies the Irp being processed.

    Status - Supplies the status to complete the Irp with

Return Value:

    None.

--*/

{
    ASSERT_OPTIONAL_IRP_CONTEXT( IrpContext );
    ASSERT_OPTIONAL_IRP( Irp );

    //
    //  Cleanup the IrpContext if passed in here.
    //

    if (ARGUMENT_PRESENT( IrpContext )) {

        UdfCleanupIrpContext( IrpContext, FALSE );
    }

    //
    //  If we have an Irp then complete the irp.
    //

    if (ARGUMENT_PRESENT( Irp )) {

        //
        //  Clear the information field in case we have used this Irp
        //  internally.
        //

        if (NT_ERROR( Status ) &&
            FlagOn( Irp->Flags, IRP_INPUT_OPERATION )) {

            Irp->IoStatus.Information = 0;
        }

        Irp->IoStatus.Status = Status;
        IoCompleteRequest( Irp, IO_CD_ROM_INCREMENT );
    }

    return;
}


VOID
UdfSetThreadContext (
    IN PIRP_CONTEXT IrpContext,
    IN PTHREAD_CONTEXT ThreadContext
    )

/*++

Routine Description:

    This routine is called at each Fsd/Fsp entry point set up the IrpContext
    and thread local storage to track top level requests.  If there is
    not a Udfs context in the thread local storage then we use the input one.
    Otherwise we use the one already there.  This routine also updates the
    IrpContext based on the state of the top-level context.

    If the TOP_LEVEL flag in the IrpContext is already set when we are called
    then we force this request to appear top level.

Arguments:

    ThreadContext - Address on stack for local storage if not already present.

    ForceTopLevel - We force this request to appear top level regardless of
        any previous stack value.

Return Value:

    None

--*/

{
    PTHREAD_CONTEXT CurrentThreadContext;
    ULONG_PTR StackTop;
    ULONG_PTR StackBottom;

    PAGED_CODE();

    ASSERT_IRP_CONTEXT( IrpContext );

    //
    //  Get the current top-level irp out of the thread storage.
    //  If NULL then this is the top-level request.
    //

    CurrentThreadContext = (PTHREAD_CONTEXT) IoGetTopLevelIrp();

    if (CurrentThreadContext == NULL) {

        SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_TOP_LEVEL );
    }

    //
    //  Initialize the input context unless we are using the current
    //  thread context block.  We use the new block if our caller
    //  specified this or the existing block is invalid.
    //
    //  The following must be true for the current to be a valid Udfs context.
    //
    //      Structure must lie within current stack.
    //      Address must be ULONG aligned.
    //      Udfs signature must be present.
    //
    //  If this is not a valid Udfs context then use the input thread
    //  context and store it in the top level context.
    //

    IoGetStackLimits( &StackTop, &StackBottom);

    if (FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_TOP_LEVEL ) ||
        (((ULONG_PTR) CurrentThreadContext > StackBottom - sizeof( THREAD_CONTEXT )) ||
         ((ULONG_PTR) CurrentThreadContext <= StackTop) ||
         LongOffsetPtr( CurrentThreadContext ) ||
         (CurrentThreadContext->Udfs != UDFS_SIGNATURE))) {

        ThreadContext->Udfs = UDFS_SIGNATURE;
        ThreadContext->SavedTopLevelIrp = (PIRP) CurrentThreadContext;
        ThreadContext->TopLevelIrpContext = IrpContext;
        IoSetTopLevelIrp( (PIRP) ThreadContext );

        IrpContext->TopLevel = IrpContext;
        IrpContext->ThreadContext = ThreadContext;

        SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_TOP_LEVEL_UDFS );

    //
    //  Otherwise use the IrpContext in the thread context.
    //

    } else {

        IrpContext->TopLevel = CurrentThreadContext->TopLevelIrpContext;
    }

    return;
}


BOOLEAN
UdfFastIoCheckIfPossible (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN BOOLEAN CheckForReadOperation,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine checks if fast i/o is possible for a read/write operation

Arguments:

    FileObject - Supplies the file object used in the query

    FileOffset - Supplies the starting byte offset for the read/write operation

    Length - Supplies the length, in bytes, of the read/write operation

    Wait - Indicates if we can wait

    LockKey - Supplies the lock key

    CheckForReadOperation - Indicates if this is a check for a read or write
        operation

    IoStatus - Receives the status of the operation if our return value is
        FastIoReturnError

Return Value:

    BOOLEAN - TRUE if fast I/O is possible and FALSE if the caller needs
        to take the long route.

--*/

{
    PAGED_CODE();

    return TRUE;
}


ULONG
UdfSerial32 (
    IN PCHAR Buffer,
    IN ULONG ByteCount
    )

/*++

Routine Description:

    This routine is called to generate a 32 bit serial number.  This is
    done by doing four separate checksums into an array of bytes and
    then treating the bytes as a ULONG.

Arguments:

    Buffer - Pointer to the buffer to generate the ID for.

    ByteCount - Number of bytes in the buffer.

Return Value:

    ULONG - The 32 bit serial number.

--*/

{
    union {
        UCHAR   Bytes[4];
        ULONG   SerialId;
    } Checksum;

    PAGED_CODE();

    //
    //  Initialize the serial number.
    //

    Checksum.SerialId = 0;

    //
    //  Continue while there are more bytes to use.
    //

    while (ByteCount--) {

        //
        //  Increment this sub-checksum.
        //

        Checksum.Bytes[ByteCount & 0x3] += *(Buffer++);
    }

    //
    //  Return the checksums as a ULONG.
    //

    return Checksum.SerialId;
}


VOID
UdfInitializeCrc16 (
    ULONG Polynomial
    )

/*++

Routine Description:

    This routine generates the 16bit CRC Table to be used in CRC calculation.

Arguments:

    Polynomial - Starting seed for the generation

Return Value:

    None

--*/

{
    ULONG n, i, Crc;

    //
    //  All CRC code was devised by Don P. Mitchell of AT&T Bell Laboratories
    //  and Ned W. Rhodes of Software Systems Group.  It has been published in
    //  "Design and Validation of Computer Protocols", Prentice Hall, Englewood
    //  Cliffs, NJ, 1991, Chapter 3, ISBN 0-13-539925-4.
    //
    //  Copyright is held by AT&T.
    //
    //  AT&T gives permission for the free use of the source code.
    //

    UdfCrcTable = (PUSHORT) FsRtlAllocatePoolWithTag( UdfPagedPool,
                                                      256 * sizeof(USHORT),
                                                      TAG_CRC_TABLE );

    for (n = 0; n < 256; n++) {

        Crc = n << 8;

        for (i = 0; i < 8; i++) {

            if(Crc & 0x8000) {

                Crc = (Crc << 1) ^ Polynomial;

            } else {

                Crc <<= 1;
            }

            Crc &= 0xffff;
        }

        UdfCrcTable[n] = (USHORT) Crc;
    }
}



USHORT
UdfComputeCrc16 (
	PUCHAR Buffer,
	ULONG ByteCount
    )

/*++

Routine Description:

    This routine generates a 16 bit CRC of the input buffer in accordance
    with the precomputed CRC table.

Arguments:

    Buffer - Pointer to the buffer to generate the CRC for.

    ByteCount - Number of bytes in the buffer.

Return Value:

    USHORT - The 16bit CRC

--*/

{
	USHORT Crc = 0;

    //
    //  All CRC code was devised by Don P. Mitchell of AT&T Bell Laboratories
    //  and Ned W. Rhodes of Software Systems Group.  It has been published in
    //  "Design and Validation of Computer Protocols", Prentice Hall, Englewood
    //  Cliffs, NJ, 1991, Chapter 3, ISBN 0-13-539925-4.
    //
    //  Copyright is held by AT&T.
    //
    //  AT&T gives permission for the free use of the source code.
    //

    while (ByteCount-- > 0) {

        Crc = UdfCrcTable[((Crc >> 8) ^ *Buffer++) & 0xff] ^ (Crc << 8);
    }

	return Crc;
}


USHORT
UdfComputeCrc16Uni (
    PWCHAR Buffer,
    ULONG CharCount
    )

/*++

Routine Description:

    This routine generates a 16 bit CRC of the input buffer in accordance
    with the precomputed CRC table.
    
    It performs a byte-order independent crc (hi then lo). This is a bit
    suspect, but is called for in the specification. 

Arguments:

    Buffer - Pointer to the buffer to generate the CRC for.

    ShortCount - Number of wide characters in the buffer.

Return Value:

    USHORT - The 16bit CRC

--*/

{
    USHORT Crc = 0;

    //
    //  Byte order independent CRC, hi byte to low byte per character.
    //

    while (CharCount-- > 0) {

        Crc = UdfCrcTable[((Crc >> 8) ^ (*Buffer >> 8)) & 0xff] ^ (Crc << 8);
        Crc = UdfCrcTable[((Crc >> 8) ^ (*Buffer++ & 0xff)) & 0xff] ^ (Crc << 8);
    }

    return Crc;
}


ULONG
UdfHighBit (
    ULONG Word
    )

/*++

Routine Description:

    This routine discovers the highest set bit of the input word.  It is
    equivalent to the integer logarithim base 2.

Arguments:

    Word - word to check

Return Value:

    Bit offset of highest set bit. If no bit is set, return is zero.

--*/

{
    ULONG Offset = 31;
    ULONG Mask = (ULONG)(1 << 31);

    if (Word == 0) {

        return 0;
    }

    while ((Word & Mask) == 0) {

        Offset--;
        Mask >>= 1;
    }

    return Offset;
}


#ifdef UDF_SANITY
BOOLEAN
UdfDebugTrace (
    LONG IndentIncrement,
    ULONG TraceMask,
    PCHAR Format,
    ...
    )

/*++

Routine Description:

    This routine is a simple debug info printer that returns a constant boolean value.  This
    makes it possible to splice it into the middle of boolean expressions to discover which
    elements are firing.
    
    We will use this as our general debug printer.  See udfdata.h for how we use the DebugTrace
    macro to accomplish the effect.
    
Arguments:

    IndentIncrement - amount to change the indentation by.
    
    TraceMask - specification of what debug trace level this call should be noisy at.

Return Value:

    USHORT - The 16bit CRC

--*/

{
    va_list Arglist;
    LONG i;
    UCHAR Buffer[128];
    int Bytes;

    if (TraceMask == 0 || (UdfDebugTraceLevel & TraceMask) != 0) {

        if (IndentIncrement < 0) {
            
            UdfDebugTraceIndent += IndentIncrement;
        }

        if (UdfDebugTraceIndent < 0) {
            
            UdfDebugTraceIndent = 0;
        }

        //
        //  Build the indent in big chunks since calling DbgPrint repeatedly is expensive.
        //
        
        for (i = UdfDebugTraceIndent; i > 0; i -= (sizeof(Buffer) - 1)) {

            RtlFillMemory( Buffer, Min( i, (sizeof(Buffer) - 1 )), ' ');
            *(Buffer + Min( i, (sizeof(Buffer) - 1 ))) = '\0';
            
            DbgPrint( Buffer );
        }

        //
        // Format the output into a buffer and then print it.
        //

        va_start( Arglist, Format );
        Bytes = _vsnprintf( Buffer, sizeof(Buffer), Format, Arglist );
        va_end( Arglist );

        //
        // detect buffer overflow
        //

        if (Bytes == -1) {

            Buffer[sizeof(Buffer) - 1] = '\n';
        }

        DbgPrint( Buffer );

        if (IndentIncrement > 0) {

            UdfDebugTraceIndent += IndentIncrement;
        }
    }

    return TRUE;
}
#endif

