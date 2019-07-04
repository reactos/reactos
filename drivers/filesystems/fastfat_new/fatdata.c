/*++

Copyright (c) 1989-2000 Microsoft Corporation

Module Name:

    FatData.c

Abstract:

    This module declares the global data used by the Fat file system.


--*/

#include "fatprocs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (FAT_BUG_CHECK_FATDATA)

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_CATCH_EXCEPTIONS)

#ifdef ALLOC_PRAGMA

#if DBG
#ifdef _MSC_VER
#pragma alloc_text(PAGE, FatBugCheckExceptionFilter)
#endif
#endif

#pragma alloc_text(PAGE, FatCompleteRequest_Real)
#pragma alloc_text(PAGE, FatFastIoCheckIfPossible)
#pragma alloc_text(PAGE, FatFastQueryBasicInfo)
#pragma alloc_text(PAGE, FatFastQueryNetworkOpenInfo)
#pragma alloc_text(PAGE, FatFastQueryStdInfo)
#pragma alloc_text(PAGE, FatIsIrpTopLevel)
#pragma alloc_text(PAGE, FatPopUpFileCorrupt)
#pragma alloc_text(PAGE, FatProcessException)
#endif


//
//  The global fsd data record, and zero large integer
//

#ifdef _MSC_VER
#pragma prefast( suppress:22112, "only applies to user mode processes" )
#endif
FAT_DATA FatData;

PDEVICE_OBJECT FatDiskFileSystemDeviceObject;
PDEVICE_OBJECT FatCdromFileSystemDeviceObject;

#ifndef __REACTOS__
LARGE_INTEGER FatLargeZero = {0,0};
LARGE_INTEGER FatMaxLarge = {MAXULONG,MAXLONG};
#else
LARGE_INTEGER FatLargeZero = {{0,0}};
LARGE_INTEGER FatMaxLarge = {{MAXULONG,MAXLONG}};
#endif

#ifndef __REACTOS__
LARGE_INTEGER Fat30Milliseconds = {(ULONG)(-30 * 1000 * 10), -1};
LARGE_INTEGER Fat100Milliseconds = {(ULONG)(-30 * 1000 * 10), -1};
LARGE_INTEGER FatOneDay = {0x2a69c000, 0xc9};
LARGE_INTEGER FatJanOne1980 = {0xe1d58000,0x01a8e79f};
LARGE_INTEGER FatDecThirtyOne1979 = {0xb76bc000,0x01a8e6d6};
#else
LARGE_INTEGER Fat30Milliseconds = {{(ULONG)(-30 * 1000 * 10), -1}};
LARGE_INTEGER Fat100Milliseconds = {{(ULONG)(-30 * 1000 * 10), -1}};
LARGE_INTEGER FatOneDay = {{0x2a69c000, 0xc9}};
LARGE_INTEGER FatJanOne1980 = {{0xe1d58000,0x01a8e79f}};
LARGE_INTEGER FatDecThirtyOne1979 = {{0xb76bc000,0x01a8e6d6}};
#endif

FAT_TIME_STAMP FatTimeJanOne1980 = {{0,0,0},{1,1,0}};

#ifndef __REACTOS__
LARGE_INTEGER FatMagic10000    = {0xe219652c, 0xd1b71758};
LARGE_INTEGER FatMagic86400000 = {0xfa67b90e, 0xc6d750eb};
#else
LARGE_INTEGER FatMagic10000    = {{0xe219652c, 0xd1b71758}};
LARGE_INTEGER FatMagic86400000 = {{0xfa67b90e, 0xc6d750eb}};
#endif

#ifdef _MSC_VER
#pragma prefast( suppress:22112, "only applies to user mode processes" )
#endif
FAST_IO_DISPATCH FatFastIoDispatch;

//
//  Our lookaside lists.
//

NPAGED_LOOKASIDE_LIST FatIrpContextLookasideList;
NPAGED_LOOKASIDE_LIST FatNonPagedFcbLookasideList;
NPAGED_LOOKASIDE_LIST FatEResourceLookasideList;

SLIST_HEADER FatCloseContextSList;

//
//  Synchronization for the close queue
//

FAST_MUTEX FatCloseQueueMutex;

//
//  Reserve MDL for paging file operations.
//

#ifndef __REACTOS__
PMDL FatReserveMdl = NULL;
#else
volatile PMDL FatReserveMdl = NULL;
#endif
KEVENT FatReserveEvent;

//
//  Global disk accounting state, enabled or disabled
//

LOGICAL FatDiskAccountingEnabled = FALSE;


#ifdef FASTFATDBG

LONG FatDebugTraceLevel = 0x00000009;
LONG FatDebugTraceIndent = 0;

ULONG FatFsdEntryCount = 0;
ULONG FatFspEntryCount = 0;
ULONG FatIoCallDriverCount = 0;

LONG FatPerformanceTimerLevel = 0x00000000;

ULONG FatTotalTicks[32] = { 0 };

//
//  I need this because C can't support conditional compilation within
//  a macro.
//

PVOID FatNull = NULL;

#endif // FASTFATDBG

#if DBG

BOOLEAN FatTestRaisedStatus = FALSE;

NTSTATUS FatBreakOnInterestingIoCompletion = STATUS_SUCCESS;
NTSTATUS FatBreakOnInterestingExceptionStatus = 0;
NTSTATUS FatBreakOnInterestingIrpCompletion = 0;

#endif


#if DBG
ULONG
FatBugCheckExceptionFilter (
    IN PEXCEPTION_POINTERS ExceptionPointer
    )

/*++

Routine Description:

    An exception filter which acts as an assert that the exception should
    never occur.
    
    This is only valid on debug builds, we don't want the overhead on retail.

Arguments:

    ExceptionPointers - The result of GetExceptionInformation() in the context
        of the exception.

Return Value:

    Bugchecks.

--*/

{
    PAGED_CODE();
    
    FatBugCheck( (ULONG_PTR)ExceptionPointer->ExceptionRecord,
                 (ULONG_PTR)ExceptionPointer->ContextRecord,
                 (ULONG_PTR)ExceptionPointer->ExceptionRecord->ExceptionAddress );

//    return EXCEPTION_EXECUTE_HANDLER; // unreachable code
}
#endif


ULONG
FatExceptionFilter (
    IN PIRP_CONTEXT IrpContext,
    IN PEXCEPTION_POINTERS ExceptionPointer
    )

/*++

Routine Description:

    This routine is used to decide if we should or should not handle
    an exception status that is being raised.  It inserts the status
    into the IrpContext and either indicates that we should handle
    the exception or bug check the system.

Arguments:

    ExceptionPointers - The result of GetExceptionInformation() in the context
        of the exception.

Return Value:

    ULONG - returns EXCEPTION_EXECUTE_HANDLER or bugchecks

--*/

{
    NTSTATUS ExceptionCode;

    ExceptionCode = ExceptionPointer->ExceptionRecord->ExceptionCode;
    DebugTrace(0, DEBUG_TRACE_UNWIND, "FatExceptionFilter %X\n", ExceptionCode);
    DebugDump("FatExceptionFilter\n", Dbg, NULL );

#ifdef DBG

    if( FatBreakOnInterestingExceptionStatus != 0 && ExceptionCode == FatBreakOnInterestingExceptionStatus ) {
        DbgBreakPoint();
    }
    
#endif

    //
    // If the exception is STATUS_IN_PAGE_ERROR, get the I/O error code
    // from the exception record.
    //

    if (ExceptionCode == STATUS_IN_PAGE_ERROR) {
        if (ExceptionPointer->ExceptionRecord->NumberParameters >= 3) {
            ExceptionCode = (NTSTATUS)ExceptionPointer->ExceptionRecord->ExceptionInformation[2];
        }
    }

    //
    //  If there is not an irp context, we must have had insufficient resources.
    //

    if ( !ARGUMENT_PRESENT( IrpContext ) ) {

        if (!FsRtlIsNtstatusExpected( ExceptionCode )) {

#ifdef _MSC_VER
#pragma prefast( suppress:28159, "things are seriously wrong if we get here" )
#endif
            FatBugCheck( (ULONG_PTR)ExceptionPointer->ExceptionRecord,
                         (ULONG_PTR)ExceptionPointer->ContextRecord,
                         (ULONG_PTR)ExceptionPointer->ExceptionRecord->ExceptionAddress );
        }

        return EXCEPTION_EXECUTE_HANDLER;
    }

    //
    //  For the purposes of processing this exception, let's mark this
    //  request as being able to wait and disable  write through if we
    //  aren't posting it.
    //

    SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);

    if ( (ExceptionCode != STATUS_CANT_WAIT) &&
         (ExceptionCode != STATUS_VERIFY_REQUIRED) ) {

        SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_DISABLE_WRITE_THROUGH);
    }

    if ( IrpContext->ExceptionStatus == 0 ) {

        if (FsRtlIsNtstatusExpected( ExceptionCode )) {

            IrpContext->ExceptionStatus = ExceptionCode;

            return EXCEPTION_EXECUTE_HANDLER;

        } else {

#ifdef _MSC_VER
#pragma prefast( suppress:28159, "things are seriously wrong if we get here" )
#endif
            FatBugCheck( (ULONG_PTR)ExceptionPointer->ExceptionRecord,
                         (ULONG_PTR)ExceptionPointer->ContextRecord,
                         (ULONG_PTR)ExceptionPointer->ExceptionRecord->ExceptionAddress );
        }

    } else {

        //
        //  We raised this code explicitly ourselves, so it had better be
        //  expected.
        //

        NT_ASSERT( IrpContext->ExceptionStatus == ExceptionCode );
        NT_ASSERT( FsRtlIsNtstatusExpected( ExceptionCode ) );
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

_Requires_lock_held_(_Global_critical_region_)    
NTSTATUS
FatProcessException (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN NTSTATUS ExceptionCode
    )

/*++

Routine Description:

    This routine process an exception.  It either completes the request
    with the saved exception status or it sends it off to IoRaiseHardError()

Arguments:

    Irp - Supplies the Irp being processed

    ExceptionCode - Supplies the normalized exception status being handled

Return Value:

    NTSTATUS - Returns the results of either posting the Irp or the
        saved completion status.

--*/

{
    PVCB Vcb;
    PIO_STACK_LOCATION IrpSp;
    FAT_VOLUME_STATE TransitionState = VolumeDirty;
    ULONG SavedFlags = 0;

    PAGED_CODE();

    DebugTrace(0, Dbg, "FatProcessException\n", 0);

    //
    //  If there is not an irp context, we must have had insufficient resources.
    //

    if ( !ARGUMENT_PRESENT( IrpContext ) ) {

        FatCompleteRequest( FatNull, Irp, ExceptionCode );

        return ExceptionCode;
    }

    //
    //  Get the real exception status from IrpContext->ExceptionStatus, and
    //  reset it.
    //

    ExceptionCode = IrpContext->ExceptionStatus;
    FatResetExceptionState( IrpContext );
    
    //
    //  If this is an Mdl write request, then take care of the Mdl
    //  here so that things get cleaned up properly.  Cc now leaves
    //  the MDL in place so a filesystem can retry after clearing an
    //  internal condition (FAT does not).
    //

    if ((IrpContext->MajorFunction == IRP_MJ_WRITE) &&
        (FlagOn( IrpContext->MinorFunction, IRP_MN_COMPLETE_MDL ) == IRP_MN_COMPLETE_MDL) &&
        (Irp->MdlAddress != NULL)) {

        PIO_STACK_LOCATION LocalIrpSp = IoGetCurrentIrpStackLocation(Irp);

        CcMdlWriteAbort( LocalIrpSp->FileObject, Irp->MdlAddress );
        Irp->MdlAddress = NULL;
    }

    //
    //  If we are going to post the request, we may have to lock down the
    //  user's buffer, so do it here in a try except so that we failed the
    //  request if the LockPages fails.
    //
    //  Also unpin any repinned Bcbs, protected by the try {} except {} filter.
    //

    _SEH2_TRY {

        SavedFlags = IrpContext->Flags;

        //
        //  Make sure we don't try to write through Bcbs
        //

        SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_DISABLE_WRITE_THROUGH);

        FatUnpinRepinnedBcbs( IrpContext );

        IrpContext->Flags = SavedFlags;

        //
        //  If we will have to post the request, do it here.  Note
        //  that the last thing FatPrePostIrp() does is mark the Irp pending,
        //  so it is critical that we actually return PENDING.  Nothing
        //  from this point to return can fail, so we are OK.
        //
        //  We cannot do a verify operations at APC level because we
        //  have to wait for Io operations to complete.
        //

        if (!FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_RECURSIVE_CALL) &&
#if (NTDDI_VERSION >= NTDDI_VISTA)
            (((ExceptionCode == STATUS_VERIFY_REQUIRED) && KeAreAllApcsDisabled()) ||
#else
            (((ExceptionCode == STATUS_VERIFY_REQUIRED) && (KeGetCurrentIrql() >= APC_LEVEL)) ||
#endif
             (ExceptionCode == STATUS_CANT_WAIT))) {

            ExceptionCode = FatFsdPostRequest( IrpContext, Irp );
        }

    } _SEH2_EXCEPT( FatExceptionFilter( IrpContext, _SEH2_GetExceptionInformation() ) ) {

        ExceptionCode = IrpContext->ExceptionStatus;
        IrpContext->ExceptionStatus = 0;

        IrpContext->Flags = SavedFlags;
    } _SEH2_END;

    //
    //  If we posted the request, just return here.
    //

    if (ExceptionCode == STATUS_PENDING) {

        return ExceptionCode;
    }

    Irp->IoStatus.Status = ExceptionCode;


    //
    //  If this request is not a "top-level" irp, just complete it.
    //

    if (FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_RECURSIVE_CALL)) {

        //
        //  If there is a cache operation above us, commute verify
        //  to a lock conflict.  This will cause retries so that
        //  we have a chance of getting through without needing
        //  to return an unaesthetic error for the operation.
        //

        if (IoGetTopLevelIrp() == (PIRP)FSRTL_CACHE_TOP_LEVEL_IRP &&
            ExceptionCode == STATUS_VERIFY_REQUIRED) {

            ExceptionCode = STATUS_FILE_LOCK_CONFLICT;
        }
        
        FatCompleteRequest( IrpContext, Irp, ExceptionCode );

        return ExceptionCode;
    }

    if (IoIsErrorUserInduced(ExceptionCode)) {

        //
        //  Check for the various error conditions that can be caused by,
        //  and possibly resolved by the user.
        //

        if (ExceptionCode == STATUS_VERIFY_REQUIRED) {

            PDEVICE_OBJECT Device = NULL;

            DebugTrace(0, Dbg, "Perform Verify Operation\n", 0);

            //
            //  Now we are at the top level file system entry point.
            //
            //  Grab the device to verify from the thread local storage
            //  and stick it in the information field for transportation
            //  to the fsp.  We also clear the field at this time.
            //

            Device = IoGetDeviceToVerify( Irp->Tail.Overlay.Thread );
            IoSetDeviceToVerify( Irp->Tail.Overlay.Thread, NULL );

            if ( Device == NULL ) {

                Device = IoGetDeviceToVerify( PsGetCurrentThread() );
                IoSetDeviceToVerify( PsGetCurrentThread(), NULL );

                NT_ASSERT( Device != NULL );
            }

            //
            //  It turns out some storage drivers really do set invalid non-NULL device 
            //  objects to verify.
            //
            //  To work around this, completely ignore the device to verify in the thread, 
            //  and just use our real device object instead.
            //

            if (IrpContext->Vcb) {

                Device = IrpContext->Vcb->Vpb->RealDevice;

            } else {

                //
                // For FSCTLs, IrpContext->Vcb may not be populated, so get the IrpContext->RealDevice instead
                //
            
                Device = IrpContext->RealDevice;
            }

            //
            //  Let's not BugCheck just because the device to verify is somehow still NULL.
            //

            if (Device == NULL) {

                ExceptionCode = STATUS_DRIVER_INTERNAL_ERROR;

                FatCompleteRequest( IrpContext, Irp, ExceptionCode );

                return ExceptionCode;
            }

            //
            //  FatPerformVerify() will do the right thing with the Irp.

            return FatPerformVerify( IrpContext, Irp, Device );
        }

        //
        //  The other user induced conditions generate an error unless
        //  they have been disabled for this request.
        //

        if (FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_DISABLE_POPUPS)) {

            FatCompleteRequest( IrpContext, Irp, ExceptionCode );

            return ExceptionCode;

        } else {

            //
            //  Generate a pop-up
            //

            PDEVICE_OBJECT RealDevice = NULL;
            PVPB Vpb;
            PETHREAD Thread;

            if (IoGetCurrentIrpStackLocation(Irp)->FileObject != NULL) {

                Vpb = IoGetCurrentIrpStackLocation(Irp)->FileObject->Vpb;

            } else {

                Vpb = NULL;
            }

            //
            //  The device to verify is either in my thread local storage
            //  or that of the thread that owns the Irp.
            //

            Thread = Irp->Tail.Overlay.Thread;
            RealDevice = IoGetDeviceToVerify( Thread );

            if ( RealDevice == NULL ) {

                Thread = PsGetCurrentThread();
                RealDevice = IoGetDeviceToVerify( Thread );

                NT_ASSERT( RealDevice != NULL );
            }

            //
            //  It turns out some storage drivers really do set invalid non-NULL device 
            //  objects to verify.
            //
            //  To work around this, completely ignore the device to verify in the thread, 
            //  and just use our real device object instead.
            //

            if (IrpContext->Vcb) {

                RealDevice = IrpContext->Vcb->Vpb->RealDevice;

            } else {

                //
                // For FSCTLs, IrpContext->Vcb may not be populated, so get the IrpContext->RealDevice instead
                //
            
                RealDevice = IrpContext->RealDevice;
            }

            //
            //  Let's not BugCheck just because the device to verify is somehow still NULL.
            //

            if (RealDevice == NULL) {

                FatCompleteRequest( IrpContext, Irp, ExceptionCode );

                return ExceptionCode;
            }

            //
            //  This routine actually causes the pop-up.  It usually
            //  does this by queuing an APC to the callers thread,
            //  but in some cases it will complete the request immediately,
            //  so it is very important to IoMarkIrpPending() first.
            //

            IoMarkIrpPending( Irp );
            IoRaiseHardError( Irp, Vpb, RealDevice );

            //
            //  We will be handing control back to the caller here, so
            //  reset the saved device object.
            //

            IoSetDeviceToVerify( Thread, NULL );

            //
            //  The Irp will be completed by Io or resubmitted.  In either
            //  case we must clean up the IrpContext here.
            //

            FatDeleteIrpContext( IrpContext );
            return STATUS_PENDING;
        }
    }

    //
    //  This is just a run of the mill error.  If is a STATUS that we
    //  raised ourselves, and the information would be use for the
    //  user, raise an informational pop-up.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );
    Vcb = IrpContext->Vcb;

    //
    //  Now, if the Vcb is unknown to us this means that the error was raised
    //  in the process of a mount and before we even had a chance to build
    //  a full Vcb - and was really handled there.
    //

    if (Vcb != NULL) {

        if ( !FatDeviceIsFatFsdo( IrpSp->DeviceObject) &&
             !NT_SUCCESS(ExceptionCode) &&
             !FsRtlIsTotalDeviceFailure(ExceptionCode) ) {

            TransitionState = VolumeDirtyWithSurfaceTest;
        }

        //
        //  If this was a STATUS_FILE_CORRUPT or similar error indicating some
        //  nastiness out on the media, then mark the volume permanently dirty.
        //

        if (!FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_DISABLE_POPUPS) &&
            ( TransitionState == VolumeDirtyWithSurfaceTest ||
              (ExceptionCode == STATUS_FILE_CORRUPT_ERROR) ||
              (ExceptionCode == STATUS_DISK_CORRUPT_ERROR) ||
              (ExceptionCode == STATUS_EA_CORRUPT_ERROR) ||
              (ExceptionCode == STATUS_INVALID_EA_NAME) ||
              (ExceptionCode == STATUS_EA_LIST_INCONSISTENT) ||
              (ExceptionCode == STATUS_NO_EAS_ON_FILE) )) {

            NT_ASSERT( NodeType(Vcb) == FAT_NTC_VCB );
            NT_ASSERT( !FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_RECURSIVE_CALL));

            SetFlag( Vcb->VcbState, VCB_STATE_FLAG_MOUNTED_DIRTY );
            SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);

            //
            //  Do the "dirty" work, ignoring any error.  We need to take the Vcb here
            //  to synchronize against the verify path tearing things down, since
            //  we dropped all synchronization when backing up due to the exception.
            //

            FatAcquireExclusiveVcbNoOpCheck( IrpContext, Vcb);
            
            _SEH2_TRY {

                if (VcbGood == Vcb->VcbCondition) {

                    FatMarkVolume( IrpContext, Vcb, TransitionState );
                }
            } 
            _SEH2_EXCEPT( FatExceptionFilter( IrpContext, _SEH2_GetExceptionInformation() ) ) {

                NOTHING;
            } _SEH2_END;

            FatReleaseVcb( IrpContext, Vcb);
        }
    }

    FatCompleteRequest( IrpContext, Irp, ExceptionCode );

    return ExceptionCode;
}


VOID
FatCompleteRequest_Real (
    IN PIRP_CONTEXT IrpContext OPTIONAL,
    IN PIRP Irp OPTIONAL,
    IN NTSTATUS Status
    )

/*++

Routine Description:

    This routine completes a Irp

Arguments:

    Irp - Supplies the Irp being processed

    Status - Supplies the status to complete the Irp with

Return Value:

    None.

--*/

{
    PAGED_CODE();

#if DBG
    if ( (FatBreakOnInterestingIrpCompletion != 0) && (Status == FatBreakOnInterestingIrpCompletion) ) {
        DbgBreakPoint();
    }
    
#endif

    //
    //  If we have an Irp Context then unpin all of the repinned bcbs
    //  we might have collected.
    //

    if (IrpContext != NULL) {

        NT_ASSERT( IrpContext->Repinned.Bcb[0] == NULL );

        FatUnpinRepinnedBcbs( IrpContext );
    }

    //
    //  Delete the Irp context before completing the IRP so if
    //  we run into some of the asserts, we can still backtrack
    //  through the IRP.
    //

    if (IrpContext != NULL) {

        FatDeleteIrpContext( IrpContext );
    }

    //
    //  If we have an Irp then complete the irp.
    //

    if (Irp != NULL) {

        //
        //  We got an error, so zero out the information field before
        //  completing the request if this was an input operation.
        //  Otherwise IopCompleteRequest will try to copy to the user's buffer.
        //

        if ( NT_ERROR(Status) &&
             FlagOn(Irp->Flags, IRP_INPUT_OPERATION) ) {

            Irp->IoStatus.Information = 0;
        }

        Irp->IoStatus.Status = Status;

        IoCompleteRequest( Irp, IO_DISK_INCREMENT );
    }

    return;
}

BOOLEAN
FatIsIrpTopLevel (
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine detects if an Irp is the Top level requestor, ie. if it os OK
    to do a verify or pop-up now.  If TRUE is returned, then no file system
    resources are held above us.

Arguments:

    Irp - Supplies the Irp being processed

    Status - Supplies the status to complete the Irp with

Return Value:

    None.

--*/

{
    PAGED_CODE();

    if ( IoGetTopLevelIrp() == NULL ) {

        IoSetTopLevelIrp( Irp );

        return TRUE;

    } else {

        return FALSE;
    }
}


_Function_class_(FAST_IO_CHECK_IF_POSSIBLE)
BOOLEAN
NTAPI
FatFastIoCheckIfPossible (
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
    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    LARGE_INTEGER LargeLength;

    PAGED_CODE();

    UNREFERENCED_PARAMETER( DeviceObject );
    UNREFERENCED_PARAMETER( IoStatus );
    UNREFERENCED_PARAMETER( Wait );
    //
    //  Decode the file object to get our fcb, the only one we want
    //  to deal with is a UserFileOpen
    //

    if (FatDecodeFileObject( FileObject, &Vcb, &Fcb, &Ccb ) != UserFileOpen) {

        return FALSE;
    }

    LargeLength.QuadPart = Length;

    //
    //  Based on whether this is a read or write operation we call
    //  fsrtl check for read/write
    //

    if (CheckForReadOperation) {

        if (FsRtlFastCheckLockForRead( &Fcb->Specific.Fcb.FileLock,
                                       FileOffset,
                                       &LargeLength,
                                       LockKey,
                                       FileObject,
                                       PsGetCurrentProcess() )) {

            return TRUE;
        }

    } else {


        //
        //  Also check for a write-protected volume here.
        //

        if (!FlagOn(Vcb->VcbState, VCB_STATE_FLAG_WRITE_PROTECTED) &&
            FsRtlFastCheckLockForWrite( &Fcb->Specific.Fcb.FileLock,
                                        FileOffset,
                                        &LargeLength,
                                        LockKey,
                                        FileObject,
                                        PsGetCurrentProcess() )) {

            return TRUE;
        }
    }

    return FALSE;
}


_Function_class_(FAST_IO_QUERY_BASIC_INFO)	
BOOLEAN
NTAPI
FatFastQueryBasicInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN OUT PFILE_BASIC_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine is for the fast query call for basic file information.

Arguments:

    FileObject - Supplies the file object used in this operation

    Wait - Indicates if we are allowed to wait for the information

    Buffer - Supplies the output buffer to receive the basic information

    IoStatus - Receives the final status of the operation

Return Value:

    BOOLEAN - TRUE if the operation succeeded and FALSE if the caller
        needs to take the long route.

--*/

{
    BOOLEAN Results = FALSE;
    IRP_CONTEXT IrpContext;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    BOOLEAN FcbAcquired = FALSE;

    PAGED_CODE();
    UNREFERENCED_PARAMETER( DeviceObject );
    
    //
    //  Prepare the dummy irp context
    //

    RtlZeroMemory( &IrpContext, sizeof(IRP_CONTEXT) );
    IrpContext.NodeTypeCode = FAT_NTC_IRP_CONTEXT;
    IrpContext.NodeByteSize = sizeof(IRP_CONTEXT);

    if (Wait) {

        SetFlag(IrpContext.Flags, IRP_CONTEXT_FLAG_WAIT);

    } else {

        ClearFlag(IrpContext.Flags, IRP_CONTEXT_FLAG_WAIT);
    }

    //
    //  Determine the type of open for the input file object and only accept
    //  the user file or directory open
    //

    TypeOfOpen = FatDecodeFileObject( FileObject, &Vcb, &Fcb, &Ccb );

    if ((TypeOfOpen != UserFileOpen) && (TypeOfOpen != UserDirectoryOpen)) {

        return Results;
    }

    FsRtlEnterFileSystem();

    //
    //  Get access to the Fcb but only if it is not the paging file
    //

    if (!FlagOn( Fcb->FcbState, FCB_STATE_PAGING_FILE )) {

        if (!ExAcquireResourceSharedLite( Fcb->Header.Resource, Wait )) {

            FsRtlExitFileSystem();
            return Results;
        }

        FcbAcquired = TRUE;
    }

    _SEH2_TRY {

        //
        //  If the Fcb is not in a good state, return FALSE.
        //

        if (Fcb->FcbCondition != FcbGood) {

            try_return( Results );
        }

        Buffer->FileAttributes = 0;

        //
        //  If the fcb is not the root dcb then we will fill in the
        //  buffer otherwise it is all setup for us.
        //

        if (NodeType(Fcb) != FAT_NTC_ROOT_DCB) {

            //
            //  Extract the data and fill in the non zero fields of the output
            //  buffer
            //

            Buffer->LastWriteTime = Fcb->LastWriteTime;
            Buffer->CreationTime = Fcb->CreationTime;
            Buffer->LastAccessTime = Fcb->LastAccessTime;

            //
            //  Zero out the field we don't support.
            //

            Buffer->ChangeTime.QuadPart = 0;
            Buffer->FileAttributes = Fcb->DirentFatFlags;

        } else {

            Buffer->LastWriteTime.QuadPart = 0;
            Buffer->CreationTime.QuadPart = 0;
            Buffer->LastAccessTime.QuadPart = 0;
            Buffer->ChangeTime.QuadPart = 0;

            Buffer->FileAttributes = FILE_ATTRIBUTE_DIRECTORY;
        }


        //
        //  If the temporary flag is set, then set it in the buffer.
        //

        if (FlagOn( Fcb->FcbState, FCB_STATE_TEMPORARY )) {

            SetFlag( Buffer->FileAttributes, FILE_ATTRIBUTE_TEMPORARY );
        }

        //
        //  If no attributes were set, set the normal bit.
        //

        if (Buffer->FileAttributes == 0) {

            Buffer->FileAttributes = FILE_ATTRIBUTE_NORMAL;
        }

        IoStatus->Status = STATUS_SUCCESS;
        IoStatus->Information = sizeof(FILE_BASIC_INFORMATION);

        Results = TRUE;

    try_exit: NOTHING;
    } _SEH2_FINALLY {

        if (FcbAcquired) { ExReleaseResourceLite( Fcb->Header.Resource ); }

        FsRtlExitFileSystem();
    } _SEH2_END;

    //
    //  And return to our caller
    //

    return Results;
}


_Function_class_(FAST_IO_QUERY_STANDARD_INFO)
BOOLEAN
NTAPI
FatFastQueryStdInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN OUT PFILE_STANDARD_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine is for the fast query call for standard file information.

Arguments:

    FileObject - Supplies the file object used in this operation

    Wait - Indicates if we are allowed to wait for the information

    Buffer - Supplies the output buffer to receive the basic information

    IoStatus - Receives the final status of the operation

Return Value:

    BOOLEAN - TRUE if the operation succeeded and FALSE if the caller
        needs to take the long route.

--*/

{
    BOOLEAN Results = FALSE;
    IRP_CONTEXT IrpContext;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    BOOLEAN FcbAcquired = FALSE;

    PAGED_CODE();

    UNREFERENCED_PARAMETER( DeviceObject );
    
    //
    //  Prepare the dummy irp context
    //

    RtlZeroMemory( &IrpContext, sizeof(IRP_CONTEXT) );
    IrpContext.NodeTypeCode = FAT_NTC_IRP_CONTEXT;
    IrpContext.NodeByteSize = sizeof(IRP_CONTEXT);

    if (Wait) {

        SetFlag(IrpContext.Flags, IRP_CONTEXT_FLAG_WAIT);

    } else {

        ClearFlag(IrpContext.Flags, IRP_CONTEXT_FLAG_WAIT);
    }

    //
    //  Determine the type of open for the input file object and only accept
    //  the user file or directory open
    //

    TypeOfOpen = FatDecodeFileObject( FileObject, &Vcb, &Fcb, &Ccb );

    if ((TypeOfOpen != UserFileOpen) && (TypeOfOpen != UserDirectoryOpen)) {

        return Results;
    }

    //
    //  Get access to the Fcb but only if it is not the paging file
    //

    FsRtlEnterFileSystem();

    if (!FlagOn( Fcb->FcbState, FCB_STATE_PAGING_FILE )) {

        if (!ExAcquireResourceSharedLite( Fcb->Header.Resource, Wait )) {

            FsRtlExitFileSystem();
            return Results;
        }

        FcbAcquired = TRUE;
    }

    _SEH2_TRY {

        //
        //  If the Fcb is not in a good state, return FALSE.
        //

        if (Fcb->FcbCondition != FcbGood) {

            try_return( Results );
        }

        Buffer->NumberOfLinks = 1;
        Buffer->DeletePending = BooleanFlagOn( Fcb->FcbState, FCB_STATE_DELETE_ON_CLOSE );

        //
        //  Case on whether this is a file or a directory, and extract
        //  the information and fill in the fcb/dcb specific parts
        //  of the output buffer.
        //

        if (NodeType(Fcb) == FAT_NTC_FCB) {

            //
            //  If we don't alread know the allocation size, we cannot look
            //  it up in the fast path.
            //

            if (Fcb->Header.AllocationSize.QuadPart == FCB_LOOKUP_ALLOCATIONSIZE_HINT) {

                try_return( Results );
            }

            Buffer->AllocationSize = Fcb->Header.AllocationSize;
            Buffer->EndOfFile = Fcb->Header.FileSize;

            Buffer->Directory = FALSE;

        } else {

            Buffer->AllocationSize = FatLargeZero;
            Buffer->EndOfFile = FatLargeZero;

            Buffer->Directory = TRUE;
        }

        IoStatus->Status = STATUS_SUCCESS;
        IoStatus->Information = sizeof(FILE_STANDARD_INFORMATION);

        Results = TRUE;

    try_exit: NOTHING;
    } _SEH2_FINALLY {

        if (FcbAcquired) { ExReleaseResourceLite( Fcb->Header.Resource ); }

        FsRtlExitFileSystem();
    } _SEH2_END;

    //
    //  And return to our caller
    //

    return Results;
}


_Function_class_(FAST_IO_QUERY_NETWORK_OPEN_INFO)
BOOLEAN
NTAPI
FatFastQueryNetworkOpenInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN OUT PFILE_NETWORK_OPEN_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine is for the fast query call for network open information.

Arguments:

    FileObject - Supplies the file object used in this operation

    Wait - Indicates if we are allowed to wait for the information

    Buffer - Supplies the output buffer to receive the information

    IoStatus - Receives the final status of the operation

Return Value:

    BOOLEAN - TRUE if the operation succeeded and FALSE if the caller
        needs to take the long route.

--*/

{
    BOOLEAN Results = FALSE;
    IRP_CONTEXT IrpContext;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    BOOLEAN FcbAcquired = FALSE;

    PAGED_CODE();
    
    UNREFERENCED_PARAMETER( DeviceObject );
    
    //
    //  Prepare the dummy irp context
    //

    RtlZeroMemory( &IrpContext, sizeof(IRP_CONTEXT) );
    IrpContext.NodeTypeCode = FAT_NTC_IRP_CONTEXT;
    IrpContext.NodeByteSize = sizeof(IRP_CONTEXT);

    if (Wait) {

        SetFlag(IrpContext.Flags, IRP_CONTEXT_FLAG_WAIT);

    } else {

        ClearFlag(IrpContext.Flags, IRP_CONTEXT_FLAG_WAIT);
    }

    //
    //  Determine the type of open for the input file object and only accept
    //  the user file or directory open
    //

    TypeOfOpen = FatDecodeFileObject( FileObject, &Vcb, &Fcb, &Ccb );

    if ((TypeOfOpen != UserFileOpen) && (TypeOfOpen != UserDirectoryOpen)) {

        return Results;
    }

    FsRtlEnterFileSystem();

    //
    //  Get access to the Fcb but only if it is not the paging file
    //

    if (!FlagOn( Fcb->FcbState, FCB_STATE_PAGING_FILE )) {

        if (!ExAcquireResourceSharedLite( Fcb->Header.Resource, Wait )) {

            FsRtlExitFileSystem();
            return Results;
        }

        FcbAcquired = TRUE;
    }

    _SEH2_TRY {

        //
        //  If the Fcb is not in a good state, return FALSE.
        //

        if (Fcb->FcbCondition != FcbGood) {

            try_return( Results );
        }

        //
        //  Extract the data and fill in the non zero fields of the output
        //  buffer
        //

        //
        //  Default the field we don't support to a reasonable value.
        //

        ExLocalTimeToSystemTime( &FatJanOne1980,
                                 &Buffer->ChangeTime );

        Buffer->FileAttributes = Fcb->DirentFatFlags;

        if (Fcb->Header.NodeTypeCode == FAT_NTC_ROOT_DCB) {

            //
            //  Reuse the default for the root dir.
            //

            Buffer->CreationTime =
            Buffer->LastAccessTime =
            Buffer->LastWriteTime = Buffer->ChangeTime;

        } else {

            Buffer->LastWriteTime = Fcb->LastWriteTime;
            Buffer->CreationTime = Fcb->CreationTime;
            Buffer->LastAccessTime = Fcb->LastAccessTime;

        }

        //
        //  If the temporary flag is set, then set it in the buffer.
        //

        if (FlagOn( Fcb->FcbState, FCB_STATE_TEMPORARY )) {

            SetFlag( Buffer->FileAttributes, FILE_ATTRIBUTE_TEMPORARY );
        }



        //
        //  If no attributes were set, set the normal bit.
        //

        if (Buffer->FileAttributes == 0) {

            Buffer->FileAttributes = FILE_ATTRIBUTE_NORMAL;
        }

        if (NodeType(Fcb) == FAT_NTC_FCB) {

            //
            //  If we don't already know the allocation size, we cannot
            //  lock it up in the fast path.
            //

            if (Fcb->Header.AllocationSize.QuadPart == FCB_LOOKUP_ALLOCATIONSIZE_HINT) {

                try_return( Results );
            }

            Buffer->AllocationSize = Fcb->Header.AllocationSize;
            Buffer->EndOfFile = Fcb->Header.FileSize;

        } else {

            Buffer->AllocationSize = FatLargeZero;
            Buffer->EndOfFile = FatLargeZero;
        }

        IoStatus->Status = STATUS_SUCCESS;
        IoStatus->Information = sizeof(FILE_NETWORK_OPEN_INFORMATION);

        Results = TRUE;

    try_exit: NOTHING;
    } _SEH2_FINALLY {

        if (FcbAcquired) { ExReleaseResourceLite( Fcb->Header.Resource ); }

        FsRtlExitFileSystem();
    } _SEH2_END;

    //
    //  And return to our caller
    //

    return Results;
}

_Requires_lock_held_(_Global_critical_region_)    
VOID
FatPopUpFileCorrupt (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    )

/*++

Routine Description:

    The Following routine makes an informational popup that the file
    is corrupt.

Arguments:

    Fcb - The file that is corrupt.

Return Value:

    None.

--*/

{
    PKTHREAD Thread;

    PAGED_CODE();

    //
    //  Disable the popup on the root directory.  It is important not
    //  to generate them on objects which are part of the mount process.
    //

    if (NodeType(Fcb) == FAT_NTC_ROOT_DCB) {

        return;
    }

    //
    //  Got to grab the full filename now.
    //

    if (Fcb->FullFileName.Buffer == NULL) {

        FatSetFullFileNameInFcb( IrpContext, Fcb );
    }

    //
    //  We never want to block a system thread waiting for the user to
    //  press OK.
    //

    if (IoIsSystemThread(IrpContext->OriginatingIrp->Tail.Overlay.Thread)) {

       Thread = NULL;

    } else {

#ifndef __REACTOS__
       Thread = IrpContext->OriginatingIrp->Tail.Overlay.Thread;
#else
       Thread = (PKTHREAD)IrpContext->OriginatingIrp->Tail.Overlay.Thread;
#endif
    }

    IoRaiseInformationalHardError( STATUS_FILE_CORRUPT_ERROR,
                                   &Fcb->FullFileName,
                                   Thread);
}

