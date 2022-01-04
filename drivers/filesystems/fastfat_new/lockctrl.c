/*++

Copyright (c) 1989-2000 Microsoft Corporation

Module Name:

    LockCtrl.c

Abstract:

    This module implements the Lock Control routines for Fat called
    by the dispatch driver.


--*/

#include "fatprocs.h"

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_LOCKCTRL)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FatCommonLockControl)
#pragma alloc_text(PAGE, FatFastLock)
#pragma alloc_text(PAGE, FatFastUnlockAll)
#pragma alloc_text(PAGE, FatFastUnlockAllByKey)
#pragma alloc_text(PAGE, FatFastUnlockSingle)
#pragma alloc_text(PAGE, FatFsdLockControl)
#endif


_Function_class_(IRP_MJ_LOCK_CONTROL)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS
NTAPI
FatFsdLockControl (
    _In_ PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    _Inout_ PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of Lock control operations

Arguments:

    VolumeDeviceObject - Supplies the volume device object where the
        file exists

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The FSD status for the IRP

--*/

{
    NTSTATUS Status;
    PIRP_CONTEXT IrpContext = NULL;

    BOOLEAN TopLevel;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "FatFsdLockControl\n", 0);

    //
    //  Call the common Lock Control routine, with blocking allowed if
    //  synchronous
    //

    FsRtlEnterFileSystem();

    TopLevel = FatIsIrpTopLevel( Irp );

    _SEH2_TRY {

        IrpContext = FatCreateIrpContext( Irp, CanFsdWait( Irp ) );

        Status = FatCommonLockControl( IrpContext, Irp );

    } _SEH2_EXCEPT(FatExceptionFilter( IrpContext, _SEH2_GetExceptionInformation() )) {

        //
        //  We had some trouble trying to perform the requested
        //  operation, so we'll abort the I/O request with
        //  the error status that we get back from the
        //  execption code
        //

        Status = FatProcessException( IrpContext, Irp, _SEH2_GetExceptionCode() );
    } _SEH2_END;

    if (TopLevel) { IoSetTopLevelIrp( NULL ); }

    FsRtlExitFileSystem();

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "FatFsdLockControl -> %08lx\n", Status);

    UNREFERENCED_PARAMETER( VolumeDeviceObject );

    return Status;
}


_Function_class_(FAST_IO_LOCK)
BOOLEAN
NTAPI
FatFastLock (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length,
    PEPROCESS ProcessId,
    ULONG Key,
    BOOLEAN FailImmediately,
    BOOLEAN ExclusiveLock,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This is a call back routine for doing the fast lock call.

Arguments:

    FileObject - Supplies the file object used in this operation

    FileOffset - Supplies the file offset used in this operation

    Length - Supplies the length used in this operation

    ProcessId - Supplies the process ID used in this operation

    Key - Supplies the key used in this operation

    FailImmediately - Indicates if the request should fail immediately
        if the lock cannot be granted.

    ExclusiveLock - Indicates if this is a request for an exclusive or
        shared lock

    IoStatus - Receives the Status if this operation is successful

Return Value:

    BOOLEAN - TRUE if this operation completed and FALSE if caller
        needs to take the long route.

--*/

{
    BOOLEAN Results = FALSE;
    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    PAGED_CODE();
    UNREFERENCED_PARAMETER( DeviceObject );

    DebugTrace(+1, Dbg, "FatFastLock\n", 0);

    //
    //  Decode the type of file object we're being asked to process and make
    //  sure it is only a user file open.
    //

    if (FatDecodeFileObject( FileObject, &Vcb, &Fcb, &Ccb ) != UserFileOpen) {

        IoStatus->Status = STATUS_INVALID_PARAMETER;
        IoStatus->Information = 0;

        DebugTrace(-1, Dbg, "FatFastLock -> TRUE (STATUS_INVALID_PARAMETER)\n", 0);
        return TRUE;
    }

    //
    //  Acquire shared access to the Fcb
    //

    FsRtlEnterFileSystem();
    ExAcquireResourceSharedLite( Fcb->Header.Resource, TRUE );

    _SEH2_TRY {

        //
        //  We check whether we can proceed
        //  based on the state of the file oplocks.
        //

        if (!FsRtlOplockIsFastIoPossible( FatGetFcbOplock(Fcb) )) {

            try_return( Results = FALSE );
        }

        //
        //  Now call the FsRtl routine to do the actual processing of the
        //  Lock request
        //

#ifdef _MSC_VER
#pragma prefast( suppress:28159, "prefast indicates this API is obsolete but it is ok for fastfat to continue using it" )
#endif
        Results = FsRtlFastLock( &Fcb->Specific.Fcb.FileLock,
                                 FileObject,
                                 FileOffset,
                                 Length,
                                 ProcessId,
                                 Key,
                                 FailImmediately,
                                 ExclusiveLock,
                                 IoStatus,
                                 NULL,
                                 FALSE );

        if (Results) {

            //
            //  Set the flag indicating if Fast I/O is possible
            //

            Fcb->Header.IsFastIoPossible = FatIsFastIoPossible( Fcb );
        }

    try_exit:  NOTHING;
    } _SEH2_FINALLY {

        DebugUnwind( FatFastLock );

        //
        //  Release the Fcb, and return to our caller
        //

        ExReleaseResourceLite( Fcb->Header.Resource );
        FsRtlExitFileSystem();

        DebugTrace(-1, Dbg, "FatFastLock -> %08lx\n", Results);
    } _SEH2_END;

    return Results;
}


_Function_class_(FAST_IO_UNLOCK_SINGLE)
BOOLEAN
NTAPI
FatFastUnlockSingle (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length,
    PEPROCESS ProcessId,
    ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This is a call back routine for doing the fast unlock single call.

Arguments:

    FileObject - Supplies the file object used in this operation

    FileOffset - Supplies the file offset used in this operation

    Length - Supplies the length used in this operation

    ProcessId - Supplies the process ID used in this operation

    Key - Supplies the key used in this operation

    Status - Receives the Status if this operation is successful

Return Value:

    BOOLEAN - TRUE if this operation completed and FALSE if caller
        needs to take the long route.

--*/

{
    BOOLEAN Results = FALSE;
    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    PAGED_CODE();
    UNREFERENCED_PARAMETER( DeviceObject );

    DebugTrace(+1, Dbg, "FatFastUnlockSingle\n", 0);

    IoStatus->Information = 0;

    //
    //  Decode the type of file object we're being asked to process and make sure
    //  it is only a user file open
    //

    if (FatDecodeFileObject( FileObject, &Vcb, &Fcb, &Ccb ) != UserFileOpen) {

        IoStatus->Status = STATUS_INVALID_PARAMETER;

        DebugTrace(-1, Dbg, "FatFastUnlockSingle -> TRUE (STATUS_INVALID_PARAMETER)\n", 0);
        return TRUE;
    }

    //
    //  Acquire exclusive access to the Fcb this operation can always wait
    //

    FsRtlEnterFileSystem();

    _SEH2_TRY {

        //
        //  We check whether we can proceed based on the state of the file oplocks.
        //

        if (!FsRtlOplockIsFastIoPossible( FatGetFcbOplock(Fcb) )) {

            try_return( Results = FALSE );
        }

        //
        //  Now call the FsRtl routine to do the actual processing of the
        //  Lock request.  The call will always succeed.
        //

        Results = TRUE;
        IoStatus->Status = FsRtlFastUnlockSingle( &Fcb->Specific.Fcb.FileLock,
                                                  FileObject,
                                                  FileOffset,
                                                  Length,
                                                  ProcessId,
                                                  Key,
                                                  NULL,
                                                  FALSE );

        //
        //  Set the flag indicating if Fast I/O is possible
        //

        Fcb->Header.IsFastIoPossible = FatIsFastIoPossible( Fcb );

    try_exit:  NOTHING;
    } _SEH2_FINALLY {

        DebugUnwind( FatFastUnlockSingle );

        //
        //  Release the Fcb, and return to our caller
        //

        FsRtlExitFileSystem();

        DebugTrace(-1, Dbg, "FatFastUnlockSingle -> %08lx\n", Results);
    } _SEH2_END;

    return Results;
}


_Function_class_(FAST_IO_UNLOCK_ALL)
BOOLEAN
NTAPI
FatFastUnlockAll (
    IN PFILE_OBJECT FileObject,
    PEPROCESS ProcessId,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This is a call back routine for doing the fast unlock all call.

Arguments:

    FileObject - Supplies the file object used in this operation

    ProcessId - Supplies the process ID used in this operation

    Status - Receives the Status if this operation is successful

Return Value:

    BOOLEAN - TRUE if this operation completed and FALSE if caller
        needs to take the long route.

--*/

{
    BOOLEAN Results = FALSE;
    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    PAGED_CODE();
    UNREFERENCED_PARAMETER( DeviceObject );

    DebugTrace(+1, Dbg, "FatFastUnlockAll\n", 0);

    IoStatus->Information = 0;

    //
    //  Decode the type of file object we're being asked to process and make sure
    //  it is only a user file open.
    //

    if (FatDecodeFileObject( FileObject, &Vcb, &Fcb, &Ccb ) != UserFileOpen) {

        IoStatus->Status = STATUS_INVALID_PARAMETER;

        DebugTrace(-1, Dbg, "FatFastUnlockAll -> TRUE (STATUS_INVALID_PARAMETER)\n", 0);
        return TRUE;
    }

    //
    //  Acquire exclusive access to the Fcb this operation can always wait
    //

    FsRtlEnterFileSystem();

    (VOID) ExAcquireResourceSharedLite( Fcb->Header.Resource, TRUE );

    _SEH2_TRY {

        //
        //  We check whether we can proceed based on the state of the file oplocks.
        //

        if (!FsRtlOplockIsFastIoPossible( FatGetFcbOplock(Fcb) )) {

            try_return( Results = FALSE );
        }

        //
        //  Now call the FsRtl routine to do the actual processing of the
        //  Lock request.  The call will always succeed.
        //

        Results = TRUE;
        IoStatus->Status = FsRtlFastUnlockAll( &Fcb->Specific.Fcb.FileLock,
                                               FileObject,
                                               ProcessId,
                                               NULL );

        //
        //  Set the flag indicating if Fast I/O is possible
        //

        Fcb->Header.IsFastIoPossible = FatIsFastIoPossible( Fcb );

    try_exit:  NOTHING;
    } _SEH2_FINALLY {

        DebugUnwind( FatFastUnlockAll );

        //
        //  Release the Fcb, and return to our caller
        //

        ExReleaseResourceLite( (Fcb)->Header.Resource );

        FsRtlExitFileSystem();

        DebugTrace(-1, Dbg, "FatFastUnlockAll -> %08lx\n", Results);
    } _SEH2_END;

    return Results;
}


_Function_class_(FAST_IO_UNLOCK_ALL_BY_KEY)
BOOLEAN
NTAPI
FatFastUnlockAllByKey (
    IN PFILE_OBJECT FileObject,
    PVOID ProcessId,
    ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This is a call back routine for doing the fast unlock all by key call.

Arguments:

    FileObject - Supplies the file object used in this operation

    ProcessId - Supplies the process ID used in this operation

    Key - Supplies the key used in this operation

    Status - Receives the Status if this operation is successful

Return Value:

    BOOLEAN - TRUE if this operation completed and FALSE if caller
        needs to take the long route.

--*/

{
    BOOLEAN Results = FALSE;
    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    PAGED_CODE();
    UNREFERENCED_PARAMETER( DeviceObject );

    DebugTrace(+1, Dbg, "FatFastUnlockAllByKey\n", 0);

    IoStatus->Information = 0;

    //
    //  Decode the type of file object we're being asked to process and make sure
    //  it is only a user file open.
    //

    if (FatDecodeFileObject( FileObject, &Vcb, &Fcb, &Ccb ) != UserFileOpen) {

        IoStatus->Status = STATUS_INVALID_PARAMETER;

        DebugTrace(-1, Dbg, "FatFastUnlockAll -> TRUE (STATUS_INVALID_PARAMETER)\n", 0);
        return TRUE;
    }

    //
    //  Acquire exclusive access to the Fcb this operation can always wait
    //

    FsRtlEnterFileSystem();

    (VOID) ExAcquireResourceSharedLite( Fcb->Header.Resource, TRUE );

    _SEH2_TRY {

        //
        //  We check whether we can proceed based on the state of the file oplocks.
        //

        if (!FsRtlOplockIsFastIoPossible( FatGetFcbOplock(Fcb) )) {

            try_return( Results = FALSE );
        }

        //
        //  Now call the FsRtl routine to do the actual processing of the
        //  Lock request.  The call will always succeed.
        //

        Results = TRUE;
        IoStatus->Status = FsRtlFastUnlockAllByKey( &Fcb->Specific.Fcb.FileLock,
                                                    FileObject,
                                                    ProcessId,
                                                    Key,
                                                    NULL );

        //
        //  Set the flag indicating if Fast I/O is possible
        //

        Fcb->Header.IsFastIoPossible = FatIsFastIoPossible( Fcb );

    try_exit:  NOTHING;
    } _SEH2_FINALLY {

        DebugUnwind( FatFastUnlockAllByKey );

        //
        //  Release the Fcb, and return to our caller
        //

        ExReleaseResourceLite( (Fcb)->Header.Resource );

        FsRtlExitFileSystem();

        DebugTrace(-1, Dbg, "FatFastUnlockAllByKey -> %08lx\n", Results);
    } _SEH2_END;

    return Results;
}


_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
FatCommonLockControl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for doing Lock control operations called
    by both the fsd and fsp threads

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IrpSp;

    TYPE_OF_OPEN TypeOfOpen;

    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    BOOLEAN OplockPostIrp = FALSE;

    PAGED_CODE();

    //
    //  Get a pointer to the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "FatCommonLockControl\n", 0);
    DebugTrace( 0, Dbg, "Irp           = %p\n", Irp);
    DebugTrace( 0, Dbg, "MinorFunction = %08lx\n", IrpSp->MinorFunction);

    //
    //  Decode the type of file object we're being asked to process
    //

    TypeOfOpen = FatDecodeFileObject( IrpSp->FileObject, &Vcb, &Fcb, &Ccb );

    //
    //  If the file is not a user file open then we reject the request
    //  as an invalid parameter
    //

    if (TypeOfOpen != UserFileOpen) {

        FatCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );

        DebugTrace(-1, Dbg, "FatCommonLockControl -> STATUS_INVALID_PARAMETER\n", 0);
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Acquire exclusive access to the Fcb and enqueue the Irp if we didn't
    //  get access
    //

    if (!FatAcquireSharedFcb( IrpContext, Fcb )) {

        Status = FatFsdPostRequest( IrpContext, Irp );

        DebugTrace(-1, Dbg, "FatCommonLockControl -> %08lx\n", Status);
        return Status;
    }

    _SEH2_TRY {

        //
        //  We check whether we can proceed
        //  based on the state of the file oplocks.
        //

#if (NTDDI_VERSION >= NTDDI_WIN8)

        if (((IRP_MN_LOCK == IrpSp->MinorFunction) &&
             ((ULONGLONG)IrpSp->Parameters.LockControl.ByteOffset.QuadPart <
              (ULONGLONG)Fcb->Header.AllocationSize.QuadPart)) ||
            ((IRP_MN_LOCK != IrpSp->MinorFunction) &&
             FsRtlAreThereWaitingFileLocks( &Fcb->Specific.Fcb.FileLock ))) {

            //
            //  Check whether we can proceed based on the state of file oplocks if doing
            //  an operation that interferes with oplocks. Those operations are:
            //
            //      1. Lock a range within the file's AllocationSize.
            //      2. Unlock a range when there are waiting locks on the file. This one
            //         is not guaranteed to interfere with oplocks, but it could, as
            //         unlocking this range might cause a waiting lock to be granted
            //         within AllocationSize!
            //

#endif
            Status = FsRtlCheckOplock( FatGetFcbOplock(Fcb),
                                       Irp,
                                       IrpContext,
                                       FatOplockComplete,
                                       NULL );

#if (NTDDI_VERSION >= NTDDI_WIN8)
        }
#endif

        if (Status != STATUS_SUCCESS) {

            OplockPostIrp = TRUE;
            try_return( NOTHING );
        }

        //
        //  Now call the FsRtl routine to do the actual processing of the
        //  Lock request
        //

        Status = FsRtlProcessFileLock( &Fcb->Specific.Fcb.FileLock, Irp, NULL );

        //
        //  Set the flag indicating if Fast I/O is possible
        //

        Fcb->Header.IsFastIoPossible = FatIsFastIoPossible( Fcb );

    try_exit:  NOTHING;
    } _SEH2_FINALLY {

        DebugUnwind( FatCommonLockControl );

        //
        //  Only if this is not an abnormal termination do we delete the
        //  irp context
        //

        if (!_SEH2_AbnormalTermination() && !OplockPostIrp) {

            FatCompleteRequest( IrpContext, FatNull, 0 );
        }

        //
        //  Release the Fcb, and return to our caller
        //

        FatReleaseFcb( IrpContext, Fcb );

        DebugTrace(-1, Dbg, "FatCommonLockControl -> %08lx\n", Status);
    } _SEH2_END;

    return Status;
}

