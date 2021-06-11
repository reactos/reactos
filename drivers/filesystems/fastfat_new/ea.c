/*++

Copyright (c) 1989-2000 Microsoft Corporation

Module Name:

    Ea.c

Abstract:

    This module implements the EA routines for Fat called by
    the dispatch driver.


--*/

#include "fatprocs.h"

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_EA)

//
//  Local procedure prototypes
//

IO_STATUS_BLOCK
FatQueryEaUserEaList (
    IN PIRP_CONTEXT IrpContext,
    OUT PCCB Ccb,
    IN PPACKED_EA FirstPackedEa,
    IN ULONG PackedEasLength,
    OUT PUCHAR UserBuffer,
    IN ULONG  UserBufferLength,
    IN PUCHAR UserEaList,
    IN ULONG  UserEaListLength,
    IN BOOLEAN ReturnSingleEntry
    );

IO_STATUS_BLOCK
FatQueryEaIndexSpecified (
    IN PIRP_CONTEXT IrpContext,
    OUT PCCB Ccb,
    IN PPACKED_EA FirstPackedEa,
    IN ULONG PackedEasLength,
    OUT PUCHAR UserBuffer,
    IN ULONG  UserBufferLength,
    IN ULONG  UserEaIndex,
    IN BOOLEAN ReturnSingleEntry
    );

IO_STATUS_BLOCK
FatQueryEaSimpleScan (
    IN PIRP_CONTEXT IrpContext,
    OUT PCCB Ccb,
    IN PPACKED_EA FirstPackedEa,
    IN ULONG PackedEasLength,
    OUT PUCHAR UserBuffer,
    IN ULONG  UserBufferLength,
    IN BOOLEAN ReturnSingleEntry,
    ULONG StartOffset
    );

BOOLEAN
FatIsDuplicateEaName (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_GET_EA_INFORMATION GetEa,
    IN PUCHAR UserBuffer
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FatCommonQueryEa)
#pragma alloc_text(PAGE, FatCommonSetEa)
#pragma alloc_text(PAGE, FatFsdQueryEa)
#pragma alloc_text(PAGE, FatFsdSetEa)
#if 0
#pragma alloc_text(PAGE, FatIsDuplicateEaName)
#pragma alloc_text(PAGE, FatQueryEaIndexSpecified)
#pragma alloc_text(PAGE, FatQueryEaSimpleScan)
#pragma alloc_text(PAGE, FatQueryEaUserEaList)
#endif
#endif


_Function_class_(IRP_MJ_QUERY_EA)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS
NTAPI
FatFsdQueryEa (
    _In_ PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    _Inout_ PIRP Irp
    )

/*++

Routine Description:

    This routine implements the Fsd part of the NtQueryEa API
    call.

Arguments:

    VolumeDeviceObject - Supplies the volume device object where the file
        being queried exists.

    Irp - Supplies the Irp being processed.

Return Value:

    NTSTATUS - The FSD status for the Irp.

--*/

{
    NTSTATUS Status;
    PIRP_CONTEXT IrpContext = NULL;

    BOOLEAN TopLevel;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "FatFsdQueryEa\n", 0);

    //
    //  Call the common query routine, with blocking allowed if synchronous
    //

    FsRtlEnterFileSystem();

    TopLevel = FatIsIrpTopLevel( Irp );

    _SEH2_TRY {

        IrpContext = FatCreateIrpContext( Irp, CanFsdWait( Irp ) );

        Status = FatCommonQueryEa( IrpContext, Irp );

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

    DebugTrace(-1, Dbg, "FatFsdQueryEa -> %08lx\n", Status);

    UNREFERENCED_PARAMETER( VolumeDeviceObject );

    return Status;
}


_Function_class_(IRP_MJ_SET_EA)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS
NTAPI
FatFsdSetEa (
    _In_ PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    _Inout_ PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of the NtSetEa API
    call.

Arguments:

    VolumeDeviceObject - Supplies the volume device object where the file
        being set exists.

    Irp - Supplies the Irp being processed.

Return Value:

    NTSTATUS - The FSD status for the Irp.

--*/

{
    NTSTATUS Status;
    PIRP_CONTEXT IrpContext = NULL;

    BOOLEAN TopLevel;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "FatFsdSetEa\n", 0);

    //
    //  Call the common set routine, with blocking allowed if synchronous
    //

    FsRtlEnterFileSystem();

    TopLevel = FatIsIrpTopLevel( Irp );

    _SEH2_TRY {

        IrpContext = FatCreateIrpContext( Irp, CanFsdWait( Irp ) );

        Status = FatCommonSetEa( IrpContext, Irp );

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

    DebugTrace(-1, Dbg, "FatFsdSetEa -> %08lx\n", Status);

    UNREFERENCED_PARAMETER( VolumeDeviceObject );

    return Status;
}


NTSTATUS
FatCommonQueryEa (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for querying File ea called by both
    the fsd and fsp threads.

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
#if 0
    PIO_STACK_LOCATION IrpSp;

    NTSTATUS Status;

    PUCHAR  Buffer;
    ULONG   UserBufferLength;

    PUCHAR  UserEaList;
    ULONG   UserEaListLength;
    ULONG   UserEaIndex;
    BOOLEAN RestartScan;
    BOOLEAN ReturnSingleEntry;
    BOOLEAN IndexSpecified;

    PVCB Vcb;
    PCCB Ccb;

    PFCB Fcb;
    PDIRENT Dirent;
    PBCB Bcb;

    PDIRENT EaDirent;
    PBCB EaBcb;
    BOOLEAN LockedEaFcb;

    PEA_SET_HEADER EaSetHeader;
    EA_RANGE EaSetRange;

    USHORT ExtendedAttributes;
#endif

    PAGED_CODE();

    FatCompleteRequest( IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST);
    return STATUS_INVALID_DEVICE_REQUEST;

#if 0
    //
    //  Get the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "FatCommonQueryEa...\n", 0);
    DebugTrace( 0, Dbg, " Wait                = %08lx\n", FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT));
    DebugTrace( 0, Dbg, " Irp                 = %p\n", Irp );
    DebugTrace( 0, Dbg, " ->SystemBuffer      = %p\n", Irp->AssociatedIrp.SystemBuffer );
    DebugTrace( 0, Dbg, " ->Length            = %08lx\n", IrpSp->Parameters.QueryEa.Length );
    DebugTrace( 0, Dbg, " ->EaList            = %08lx\n", IrpSp->Parameters.QueryEa.EaList );
    DebugTrace( 0, Dbg, " ->EaListLength      = %08lx\n", IrpSp->Parameters.QueryEa.EaListLength );
    DebugTrace( 0, Dbg, " ->EaIndex           = %08lx\n", IrpSp->Parameters.QueryEa.EaIndex );
    DebugTrace( 0, Dbg, " ->RestartScan       = %08lx\n", FlagOn(IrpSp->Flags, SL_RESTART_SCAN));
    DebugTrace( 0, Dbg, " ->ReturnSingleEntry = %08lx\n", FlagOn(IrpSp->Flags, SL_RETURN_SINGLE_ENTRY));
    DebugTrace( 0, Dbg, " ->IndexSpecified    = %08lx\n", FlagOn(IrpSp->Flags, SL_INDEX_SPECIFIED));

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    //
    //  Check that the file object is associated with either a user file
    //  or directory open.  We don't allow Ea operations on the root
    //  directory.
    //

    {
        TYPE_OF_OPEN OpenType;

        if (((OpenType = FatDecodeFileObject( IrpSp->FileObject,
                                             &Vcb,
                                             &Fcb,
                                             &Ccb )) != UserFileOpen
             && OpenType != UserDirectoryOpen) ||

            (NodeType( Fcb )) == FAT_NTC_ROOT_DCB) {

            FatCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );

            DebugTrace(-1, Dbg,
                       "FatCommonQueryEa -> %08lx\n",
                       STATUS_INVALID_PARAMETER);

            return STATUS_INVALID_PARAMETER;
        }
    }

    //
    //  Fat32 does not support ea's.
    //

    if (FatIsFat32(Vcb)) {

        FatCompleteRequest( IrpContext, Irp, STATUS_EAS_NOT_SUPPORTED );
        DebugTrace(-1, Dbg,
                   "FatCommonQueryEa -> %08lx\n",
                   STATUS_EAS_NOT_SUPPORTED);
        return STATUS_EAS_NOT_SUPPORTED;
    }

    //
    //  Acquire shared access to the Fcb and enqueue the Irp if we didn't
    //  get access.
    //

    if (!FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT )) {

        DebugTrace(0, Dbg, "FatCommonQueryEa:  Thread can't wait\n", 0);

        Status = FatFsdPostRequest( IrpContext, Irp );

        DebugTrace(-1, Dbg, "FatCommonQueryEa -> %08lx\n", Status );

        return Status;
    }

    FatAcquireSharedFcb( IrpContext, Fcb );

    //
    //  Reference our input parameters to make things easier
    //

    UserBufferLength  = IrpSp->Parameters.QueryEa.Length;
    UserEaList        = IrpSp->Parameters.QueryEa.EaList;
    UserEaListLength  = IrpSp->Parameters.QueryEa.EaListLength;
    UserEaIndex       = IrpSp->Parameters.QueryEa.EaIndex;
    RestartScan       = BooleanFlagOn(IrpSp->Flags, SL_RESTART_SCAN);
    ReturnSingleEntry = BooleanFlagOn(IrpSp->Flags, SL_RETURN_SINGLE_ENTRY);
    IndexSpecified    = BooleanFlagOn(IrpSp->Flags, SL_INDEX_SPECIFIED);

    //
    //  Initialize our local values.
    //

    LockedEaFcb = FALSE;
    Bcb = NULL;
    EaBcb = NULL;

    Status = STATUS_SUCCESS;

    RtlZeroMemory( &EaSetRange, sizeof( EA_RANGE ));

    try {

        PPACKED_EA FirstPackedEa;
        ULONG PackedEasLength;

        Buffer = FatMapUserBuffer( IrpContext, Irp );

        //
        //  We verify that the Fcb is still valid.
        //

        FatVerifyFcb( IrpContext, Fcb );

        //
        //  We need to get the dirent for the Fcb to recover the Ea handle.
        //

        FatGetDirentFromFcbOrDcb( IrpContext, Fcb, &Dirent, &Bcb );

        //
        //  Verify that the Ea file is in a consistant state.  If the
        //  Ea modification count in the Fcb doesn't match that in
        //  the CCB, then the Ea file has been changed from under
        //  us.  If we are not starting the search from the beginning
        //  of the Ea set, we return an error.
        //

        if (UserEaList == NULL
            && Ccb->OffsetOfNextEaToReturn != 0
            && !IndexSpecified
            && !RestartScan
            && Fcb->EaModificationCount != Ccb->EaModificationCount) {

            DebugTrace(0, Dbg,
                      "FatCommonQueryEa:  Ea file in unknown state\n", 0);

            Status = STATUS_EA_CORRUPT_ERROR;

            try_return( Status );
        }

        //
        //  Show that the Ea's for this file are consistant for this
        //  file handle.
        //

        Ccb->EaModificationCount = Fcb->EaModificationCount;

        //
        //  If the handle value is 0, then the file has no Eas.  We dummy up
        //  an ea list to use below.
        //

        ExtendedAttributes = Dirent->ExtendedAttributes;

        FatUnpinBcb( IrpContext, Bcb );

        if (ExtendedAttributes == 0) {

            DebugTrace(0, Dbg,
                      "FatCommonQueryEa:  Zero handle, no Ea's for this file\n", 0);

            FirstPackedEa = (PPACKED_EA) NULL;

            PackedEasLength = 0;

        } else {

            //
            //  We need to get the Ea file for this volume.  If the
            //  operation doesn't complete due to blocking, then queue the
            //  Irp to the Fsp.
            //

            FatGetEaFile( IrpContext,
                          Vcb,
                          &EaDirent,
                          &EaBcb,
                          FALSE,
                          FALSE );

            LockedEaFcb = TRUE;

            //
            //  If the above operation completed and the Ea file did not exist,
            //  the disk has been corrupted.  There is an existing Ea handle
            //  without any Ea data.
            //

            if (Vcb->VirtualEaFile == NULL) {

                DebugTrace(0, Dbg,
                          "FatCommonQueryEa:  No Ea file found when expected\n", 0);

                Status = STATUS_NO_EAS_ON_FILE;

                try_return( Status );
            }

            //
            //  We need to try to get the Ea set for the desired file.  If
            //  blocking is necessary then we'll post the request to the Fsp.
            //

            FatReadEaSet( IrpContext,
                          Vcb,
                          ExtendedAttributes,
                          &Fcb->ShortName.Name.Oem,
                          TRUE,
                          &EaSetRange );

            EaSetHeader = (PEA_SET_HEADER) EaSetRange.Data;

            //
            //  Find the start and length of the Eas.
            //

            FirstPackedEa = (PPACKED_EA) EaSetHeader->PackedEas;

            PackedEasLength = GetcbList( EaSetHeader ) - 4;
        }

        //
        //  Protect our access to the user buffer since IO dosn't do this
        //  for us in this path unless we had specified that our driver
        //  requires buffering for these large requests.  We don't, so ...
        //

        try {

            //
            //  Let's clear the output buffer.
            //

            RtlZeroMemory( Buffer, UserBufferLength );

            //
            //  We now satisfy the user's request depending on whether he
            //  specified an Ea name list, an Ea index or restarting the
            //  search.
            //

            //
            //  The user has supplied a list of Ea names.
            //

            if (UserEaList != NULL) {

                Irp->IoStatus = FatQueryEaUserEaList( IrpContext,
                                                      Ccb,
                                                      FirstPackedEa,
                                                      PackedEasLength,
                                                      Buffer,
                                                      UserBufferLength,
                                                      UserEaList,
                                                      UserEaListLength,
                                                      ReturnSingleEntry );

            //
            //  The user supplied an index into the Ea list.
            //

            } else if (IndexSpecified) {

                Irp->IoStatus = FatQueryEaIndexSpecified( IrpContext,
                                                          Ccb,
                                                          FirstPackedEa,
                                                          PackedEasLength,
                                                          Buffer,
                                                          UserBufferLength,
                                                          UserEaIndex,
                                                          ReturnSingleEntry );

            //
            //  Else perform a simple scan, taking into account the restart
            //  flag and the position of the next Ea stored in the Ccb.
            //

            } else {

                Irp->IoStatus = FatQueryEaSimpleScan( IrpContext,
                                                      Ccb,
                                                      FirstPackedEa,
                                                      PackedEasLength,
                                                      Buffer,
                                                      UserBufferLength,
                                                      ReturnSingleEntry,
                                                      RestartScan
                                                      ? 0
                                                      : Ccb->OffsetOfNextEaToReturn );
            }

        }  except (!FsRtlIsNtstatusExpected(GetExceptionCode()) ?
                   EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {

               //
               //  We must have had a problem filling in the user's buffer, so fail.
               //

               Irp->IoStatus.Status = GetExceptionCode();
               Irp->IoStatus.Information = 0;
        }

        Status = Irp->IoStatus.Status;

    try_exit: NOTHING;
    } finally {

        DebugUnwind( FatCommonQueryEa );

        //
        //  Release the Fcb for the file object, and the Ea Fcb if
        //  successfully locked.
        //

        FatReleaseFcb( IrpContext, Fcb );

        if (LockedEaFcb) {

            FatReleaseFcb( IrpContext, Vcb->EaFcb );
        }

        //
        //  Unpin the dirents for the Fcb, EaFcb and EaSetFcb if necessary.
        //

        FatUnpinBcb( IrpContext, Bcb );
        FatUnpinBcb( IrpContext, EaBcb );

        FatUnpinEaRange( IrpContext, &EaSetRange );

        if (!AbnormalTermination()) {

            FatCompleteRequest( IrpContext, Irp, Status );
        }

        DebugTrace(-1, Dbg, "FatCommonQueryEa -> %08lx\n", Status);
    }

    return Status;
#endif
}


NTSTATUS
FatCommonSetEa (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the common Set Ea File Api called by the
    the Fsd and Fsp threads

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The appropriate status for the Irp

--*/

{
#if 0
    PIO_STACK_LOCATION IrpSp;

    NTSTATUS Status;

    USHORT ExtendedAttributes;

    PUCHAR Buffer;
    ULONG UserBufferLength;

    PVCB Vcb;
    PCCB Ccb;

    PFCB Fcb;
    PDIRENT Dirent;
    PBCB Bcb = NULL;

    PDIRENT EaDirent = NULL;
    PBCB EaBcb = NULL;

    PEA_SET_HEADER EaSetHeader = NULL;

    PEA_SET_HEADER PrevEaSetHeader;
    PEA_SET_HEADER NewEaSetHeader;
    EA_RANGE EaSetRange;

    BOOLEAN AcquiredVcb = FALSE;
    BOOLEAN AcquiredFcb = FALSE;
    BOOLEAN AcquiredParentDcb = FALSE;
    BOOLEAN AcquiredRootDcb = FALSE;
    BOOLEAN AcquiredEaFcb = FALSE;
#endif

    PAGED_CODE();

    FatCompleteRequest( IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST);
    return STATUS_INVALID_DEVICE_REQUEST;

#if 0

    //
    //  The following booleans are used in the unwind process.
    //

    //
    //  Get the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "FatCommonSetEa...\n", 0);
    DebugTrace( 0, Dbg, " Wait                = %08lx\n", FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT));
    DebugTrace( 0, Dbg, " Irp                 = %p\n", Irp );
    DebugTrace( 0, Dbg, " ->SystemBuffer      = %p\n", Irp->AssociatedIrp.SystemBuffer );
    DebugTrace( 0, Dbg, " ->Length            = %08lx\n", IrpSp->Parameters.SetEa.Length );

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    //
    //  Check that the file object is associated with either a user file
    //  or directory open.
    //

    {
        TYPE_OF_OPEN OpenType;

        if (((OpenType = FatDecodeFileObject( IrpSp->FileObject,
                                             &Vcb,
                                             &Fcb,
                                             &Ccb )) != UserFileOpen
             && OpenType != UserDirectoryOpen) ||

            (NodeType( Fcb )) == FAT_NTC_ROOT_DCB) {

            FatCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );

            DebugTrace(-1, Dbg,
                       "FatCommonSetEa -> %08lx\n",
                       STATUS_INVALID_PARAMETER);

            return STATUS_INVALID_PARAMETER;
        }
    }

    //
    // Fat32 does not support ea's.
    //

    if (FatIsFat32(Vcb)) {

        FatCompleteRequest( IrpContext, Irp, STATUS_EAS_NOT_SUPPORTED );
        DebugTrace(-1, Dbg,
                   "FatCommonSetEa -> %08lx\n",
                   STATUS_EAS_NOT_SUPPORTED);
        return STATUS_EAS_NOT_SUPPORTED;
    }

    //
    //  Reference our input parameters to make things easier
    //

    UserBufferLength  = IrpSp->Parameters.SetEa.Length;

    //
    //  Since we ask for no outside help (direct or buffered IO), it
    //  is our responsibility to insulate ourselves from the
    //  deviousness of the user above.  Now, buffer and validate the
    //  contents.
    //

    Buffer = FatBufferUserBuffer( IrpContext, Irp, UserBufferLength );

    //
    //  Check the validity of the buffer with the new eas.  We really
    //  need to do this always since we don't know, if it was already
    //  buffered, that we buffered and checked it or some overlying
    //  filter buffered without checking.
    //

    Status = IoCheckEaBufferValidity( (PFILE_FULL_EA_INFORMATION) Buffer,
                                      UserBufferLength,
                                      (PULONG)&Irp->IoStatus.Information );

    if (!NT_SUCCESS( Status )) {

        FatCompleteRequest( IrpContext, Irp, Status );
        DebugTrace(-1, Dbg,
                   "FatCommonSetEa -> %08lx\n",
                   Status);
        return Status;
    }

    //
    //  Acquire exclusive access to the Fcb.  If this is a write-through operation
    //  we will need to pick up the other possible streams that can be modified in
    //  this operation so that the locking order is preserved - the root directory
    //  (dirent addition if EA database doesn't already exist) and the parent
    //  directory (addition of the EA handle to the object's dirent).
    //
    //  We are primarily synchronizing with directory enumeration here.
    //
    //  If we cannot wait need to send things off to the fsp.
    //

    if (!FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT)) {

        DebugTrace(0, Dbg, "FatCommonSetEa:  Set Ea must be waitable\n", 0);

        Status = FatFsdPostRequest( IrpContext, Irp );

        DebugTrace(-1, Dbg, "FatCommonSetEa -> %08lx\n", Status );

        return Status;
    }

    //
    //  Set this handle as having modified the file
    //

    IrpSp->FileObject->Flags |= FO_FILE_MODIFIED;

    RtlZeroMemory( &EaSetRange, sizeof( EA_RANGE ));

    try {

        ULONG PackedEasLength;
        BOOLEAN PreviousEas;
        ULONG AllocationLength;
        ULONG BytesPerCluster;
        USHORT EaHandle;

        PFILE_FULL_EA_INFORMATION FullEa;

        //
        //  Now go pick up everything
        //

        FatAcquireSharedVcb( IrpContext, Fcb->Vcb );
        AcquiredVcb = TRUE;
        FatAcquireExclusiveFcb( IrpContext, Fcb );
        AcquiredFcb = TRUE;

        if (FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_WRITE_THROUGH)) {

            if (Fcb->ParentDcb) {

                FatAcquireExclusiveFcb( IrpContext, Fcb->ParentDcb );
                AcquiredParentDcb = TRUE;
            }

            FatAcquireExclusiveFcb( IrpContext, Fcb->Vcb->RootDcb );
            AcquiredRootDcb = TRUE;
        }

        //
        //  We verify that the Fcb is still valid.
        //

        FatVerifyFcb( IrpContext, Fcb );

        //
        //  We need to get the dirent for the Fcb to recover the Ea handle.
        //

        FatGetDirentFromFcbOrDcb( IrpContext, Fcb, &Dirent, &Bcb );

        DebugTrace(0, Dbg, "FatCommonSetEa:  Dirent Address -> %p\n",
                   Dirent );
        DebugTrace(0, Dbg, "FatCommonSetEa:  Dirent Bcb -> %p\n",
                   Bcb);

        //
        //  If the handle value is 0, then the file has no Eas.  In that
        //  case we allocate memory to hold the Eas to be added.  If there
        //  are existing Eas for the file, then we must read from the
        //  file and copy the Eas.
        //

        ExtendedAttributes = Dirent->ExtendedAttributes;

        FatUnpinBcb( IrpContext, Bcb );

        if (ExtendedAttributes == 0) {

            PreviousEas = FALSE;

            DebugTrace(0, Dbg,
                      "FatCommonSetEa:  File has no current Eas\n", 0 );

        } else {

            PreviousEas = TRUE;

            DebugTrace(0, Dbg, "FatCommonSetEa:  File has previous Eas\n", 0 );

            FatGetEaFile( IrpContext,
                          Vcb,
                          &EaDirent,
                          &EaBcb,
                          FALSE,
                          TRUE );

            AcquiredEaFcb = TRUE;

            //
            //  If we didn't get the file then there is an error on
            //  the disk.
            //

            if (Vcb->VirtualEaFile == NULL) {

                Status = STATUS_NO_EAS_ON_FILE;
                try_return( Status );
            }
        }

        DebugTrace(0, Dbg, "FatCommonSetEa:  EaBcb -> %p\n", EaBcb);

        DebugTrace(0, Dbg, "FatCommonSetEa:  EaDirent -> %p\n", EaDirent);

        //
        //  If the file has existing ea's, we need to read them to
        //  determine the size of the buffer allocation.
        //

        if (PreviousEas) {

            //
            //  We need to try to get the Ea set for the desired file.
            //

            FatReadEaSet( IrpContext,
                          Vcb,
                          ExtendedAttributes,
                          &Fcb->ShortName.Name.Oem,
                          TRUE,
                          &EaSetRange );

            PrevEaSetHeader = (PEA_SET_HEADER) EaSetRange.Data;

            //
            //  We now must allocate pool memory for our copy of the
            //  EaSetHeader and then copy the Ea data into it.  At that
            //  time we can unpin the EaSet.
            //

            PackedEasLength = GetcbList( PrevEaSetHeader ) - 4;

        //
        //  Else we will create a dummy EaSetHeader.
        //

        } else {

            PackedEasLength = 0;
        }

        BytesPerCluster = 1 << Vcb->AllocationSupport.LogOfBytesPerCluster;

        AllocationLength = (PackedEasLength
                            + SIZE_OF_EA_SET_HEADER
                            + BytesPerCluster - 1)
                           & ~(BytesPerCluster - 1);

        EaSetHeader = FsRtlAllocatePoolWithTag( PagedPool,
                                                AllocationLength,
                                                TAG_EA_SET_HEADER );

        //
        //  Copy the existing Eas over to pool memory.
        //

        if (PreviousEas) {

            RtlCopyMemory( EaSetHeader, PrevEaSetHeader, AllocationLength );

            FatUnpinEaRange( IrpContext, &EaSetRange );

        } else {

            RtlZeroMemory( EaSetHeader, AllocationLength );

            RtlCopyMemory( EaSetHeader->OwnerFileName,
                           Fcb->ShortName.Name.Oem.Buffer,
                           Fcb->ShortName.Name.Oem.Length );
        }


        AllocationLength -= SIZE_OF_EA_SET_HEADER;

        DebugTrace(0, Dbg, "FatCommonSetEa:  Initial Ea set -> %p\n",
                   EaSetHeader);

        //
        //  At this point we have either read in the current eas for the file
        //  or we have initialized a new empty buffer for the eas.  Now for
        //  each full ea in the input user buffer we do the specified operation
        //  on the ea
        //

        for (FullEa = (PFILE_FULL_EA_INFORMATION) Buffer;
             FullEa < (PFILE_FULL_EA_INFORMATION) &Buffer[UserBufferLength];
             FullEa = (PFILE_FULL_EA_INFORMATION) (FullEa->NextEntryOffset == 0 ?
                                  &Buffer[UserBufferLength] :
                                  (PUCHAR) FullEa + FullEa->NextEntryOffset)) {

            OEM_STRING EaName;
            ULONG Offset;

            EaName.MaximumLength = EaName.Length = FullEa->EaNameLength;
            EaName.Buffer = &FullEa->EaName[0];

            DebugTrace(0, Dbg, "FatCommonSetEa:  Next Ea name -> %Z\n",
                       &EaName);

            //
            //  Make sure the ea name is valid
            //

            if (!FatIsEaNameValid( IrpContext,EaName )) {

                Irp->IoStatus.Information = (PUCHAR)FullEa - Buffer;
                Status = STATUS_INVALID_EA_NAME;
                try_return( Status );
            }

            //
            //  Check that no invalid ea flags are set.
            //

            //
            //  TEMPCODE  We are returning STATUS_INVALID_EA_NAME
            //  until a more appropriate error code exists.
            //

            if (FullEa->Flags != 0
                && FullEa->Flags != FILE_NEED_EA) {

                Irp->IoStatus.Information = (PUCHAR)FullEa - (PUCHAR)Buffer;
                try_return( Status = STATUS_INVALID_EA_NAME );
            }

            //
            //  See if we can locate the ea name in the ea set
            //

            if (FatLocateEaByName( IrpContext,
                                   (PPACKED_EA) EaSetHeader->PackedEas,
                                   PackedEasLength,
                                   &EaName,
                                   &Offset )) {

                DebugTrace(0, Dbg, "FatCommonSetEa:  Found Ea name\n", 0);

                //
                //  We found the ea name so now delete the current entry,
                //  and if the new ea value length is not zero then we
                //  replace if with the new ea
                //

                FatDeletePackedEa( IrpContext,
                                   EaSetHeader,
                                   &PackedEasLength,
                                   Offset );
            }

            if (FullEa->EaValueLength != 0) {

                FatAppendPackedEa( IrpContext,
                                   &EaSetHeader,
                                   &PackedEasLength,
                                   &AllocationLength,
                                   FullEa,
                                   BytesPerCluster );
            }
        }

        //
        //  If there are any ea's not removed, we
        //  call 'AddEaSet' to insert them into the Fat chain.
        //

        if (PackedEasLength != 0) {

            LARGE_INTEGER EaOffset;

            EaOffset.HighPart = 0;

            //
            //  If the packed eas length (plus 4 bytes) is greater
            //  than the maximum allowed ea size, we return an error.
            //

            if (PackedEasLength + 4 > MAXIMUM_EA_SIZE) {

                DebugTrace( 0, Dbg, "Ea length is greater than maximum\n", 0 );

                try_return( Status = STATUS_EA_TOO_LARGE );
            }

            //
            //  We need to now read the ea file if we haven't already.
            //

            if (EaDirent == NULL) {

                FatGetEaFile( IrpContext,
                              Vcb,
                              &EaDirent,
                              &EaBcb,
                              TRUE,
                              TRUE );

                AcquiredEaFcb = TRUE;
            }

            FatGetDirentFromFcbOrDcb( IrpContext, Fcb, &Dirent, &Bcb );

            RtlZeroMemory( &EaSetRange, sizeof( EA_RANGE ));

            FatAddEaSet( IrpContext,
                         Vcb,
                         PackedEasLength + SIZE_OF_EA_SET_HEADER,
                         EaBcb,
                         EaDirent,
                         &EaHandle,
                         &EaSetRange );

            NewEaSetHeader = (PEA_SET_HEADER) EaSetRange.Data;

            DebugTrace(0, Dbg, "FatCommonSetEa:  Adding an ea set\n", 0);

            //
            //  Store the length of the new Ea's into the EaSetHeader.
            //  This is the PackedEasLength + 4.
            //

            PackedEasLength += 4;

            CopyU4char( EaSetHeader->cbList, &PackedEasLength );

            //
            //  Copy all but the first four bytes of EaSetHeader into
            //  NewEaSetHeader.  The signature and index fields have
            //  already been filled in.
            //

            RtlCopyMemory( &NewEaSetHeader->NeedEaCount,
                           &EaSetHeader->NeedEaCount,
                           PackedEasLength + SIZE_OF_EA_SET_HEADER - 8 );

            FatMarkEaRangeDirty( IrpContext, Vcb->VirtualEaFile, &EaSetRange );
            FatUnpinEaRange( IrpContext, &EaSetRange );

            CcFlushCache( Vcb->VirtualEaFile->SectionObjectPointer, NULL, 0, NULL );

        } else {

            FatGetDirentFromFcbOrDcb( IrpContext, Fcb, &Dirent, &Bcb );

            EaHandle = 0;
        }

        //
        //  Now we do a wholesale replacement of the ea for the file
        //

        if (PreviousEas) {

            FatDeleteEaSet( IrpContext,
                            Vcb,
                            EaBcb,
                            EaDirent,
                            ExtendedAttributes,
                            &Fcb->ShortName.Name.Oem );

            CcFlushCache( Vcb->VirtualEaFile->SectionObjectPointer, NULL, 0, NULL );
        }

        if (PackedEasLength != 0 ) {

            Fcb->EaModificationCount++;
        }

        //
        //  Mark the dirent with the new ea's
        //

        Dirent->ExtendedAttributes = EaHandle;

        FatSetDirtyBcb( IrpContext, Bcb, Vcb, TRUE );

        //
        //  We call the notify package to report that the ea's were
        //  modified.
        //

        FatNotifyReportChange( IrpContext,
                               Vcb,
                               Fcb,
                               FILE_NOTIFY_CHANGE_EA,
                               FILE_ACTION_MODIFIED );

        Irp->IoStatus.Information = 0;
        Status = STATUS_SUCCESS;

    try_exit: NOTHING;

        //
        //  Unpin the dirents for the Fcb and EaFcb if necessary.
        //

        FatUnpinBcb( IrpContext, Bcb );
        FatUnpinBcb( IrpContext, EaBcb );

        FatUnpinRepinnedBcbs( IrpContext );

    } finally {

        DebugUnwind( FatCommonSetEa );

        //
        //  If this is an abnormal termination, we need to clean up
        //  any locked resources.
        //

        if (AbnormalTermination()) {

            //
            //  Unpin the dirents for the Fcb, EaFcb and EaSetFcb if necessary.
            //

            FatUnpinBcb( IrpContext, Bcb );
            FatUnpinBcb( IrpContext, EaBcb );

            FatUnpinEaRange( IrpContext, &EaSetRange );
        }

        //
        //  Release the Fcbs/Vcb acquired.
        //

        if (AcquiredEaFcb) {
            FatReleaseFcb( IrpContext, Vcb->EaFcb );
        }

        if (AcquiredFcb) {
            FatReleaseFcb( IrpContext, Fcb );
        }

        if (AcquiredParentDcb) {
            FatReleaseFcb( IrpContext, Fcb->ParentDcb );
        }

        if (AcquiredRootDcb) {
            FatReleaseFcb( IrpContext, Fcb->Vcb->RootDcb );
        }

        if (AcquiredVcb) {
            FatReleaseVcb( IrpContext, Fcb->Vcb );
        }

        //
        //  Deallocate our Ea buffer.
        //

        if (EaSetHeader != NULL) {

            ExFreePool( EaSetHeader );
        }

        //
        //  Complete the irp.
        //

        if (!AbnormalTermination()) {

            FatCompleteRequest( IrpContext, Irp, Status );
        }

        DebugTrace(-1, Dbg, "FatCommonSetEa -> %08lx\n", Status);
    }

    //
    //  And return to our caller
    //

    return Status;
#endif
}


#if 0

//
//  Local Support Routine
//

IO_STATUS_BLOCK
FatQueryEaUserEaList (
    IN PIRP_CONTEXT IrpContext,
    OUT PCCB Ccb,
    IN PPACKED_EA FirstPackedEa,
    IN ULONG PackedEasLength,
    OUT PUCHAR UserBuffer,
    IN ULONG  UserBufferLength,
    IN PUCHAR UserEaList,
    IN ULONG  UserEaListLength,
    IN BOOLEAN ReturnSingleEntry
    )

/*++

Routine Description:

    This routine is the work routine for querying EAs given an ea index

Arguments:

    Ccb - Supplies the Ccb for the query

    FirstPackedEa - Supplies the first ea for the file being queried

    PackedEasLength - Supplies the length of the ea data

    UserBuffer - Supplies the buffer to receive the full eas

    UserBufferLength - Supplies the length, in bytes, of the user buffer

    UserEaList - Supplies the user specified ea name list

    UserEaListLength - Supplies the length, in bytes, of the user ea list

    ReturnSingleEntry - Indicates if we are to return a single entry or not

Return Value:

    IO_STATUS_BLOCK - Receives the completion status for the operation

--*/

{
    IO_STATUS_BLOCK Iosb;

    ULONG Offset;
    ULONG RemainingUserBufferLength;

    PPACKED_EA PackedEa;
    ULONG PackedEaSize;

    PFILE_FULL_EA_INFORMATION LastFullEa = NULL;
    ULONG LastFullEaSize;
    PFILE_FULL_EA_INFORMATION NextFullEa;

    PFILE_GET_EA_INFORMATION GetEa;

    BOOLEAN Overflow;

    DebugTrace(+1, Dbg, "FatQueryEaUserEaList...\n", 0);

    LastFullEa = NULL;
    NextFullEa = (PFILE_FULL_EA_INFORMATION) UserBuffer;
    RemainingUserBufferLength = UserBufferLength;

    Overflow = FALSE;

    for (GetEa = (PFILE_GET_EA_INFORMATION) &UserEaList[0];
         GetEa < (PFILE_GET_EA_INFORMATION) ((PUCHAR) UserEaList
                                             + UserEaListLength);
         GetEa = (GetEa->NextEntryOffset == 0
                  ? (PFILE_GET_EA_INFORMATION) MAXUINT_PTR
                  : (PFILE_GET_EA_INFORMATION) ((PUCHAR) GetEa
                                                + GetEa->NextEntryOffset))) {

        OEM_STRING Str;
        OEM_STRING OutputEaName;

        DebugTrace(0, Dbg, "Top of loop, GetEa = %p\n", GetEa);
        DebugTrace(0, Dbg, "LastFullEa = %p\n", LastFullEa);
        DebugTrace(0, Dbg, "NextFullEa = %p\n", NextFullEa);
        DebugTrace(0, Dbg, "RemainingUserBufferLength = %08lx\n", RemainingUserBufferLength);

        //
        //  Make a string reference to the GetEa and see if we can
        //  locate the ea by name
        //

        Str.MaximumLength = Str.Length = GetEa->EaNameLength;
        Str.Buffer = &GetEa->EaName[0];

        //
        //  Check for a valid name.
        //

        if (!FatIsEaNameValid( IrpContext, Str )) {

            DebugTrace(-1, Dbg,
                       "FatQueryEaUserEaList:  Invalid Ea Name -> %Z\n",
                       &Str);

            Iosb.Information = (PUCHAR)GetEa - UserEaList;
            Iosb.Status = STATUS_INVALID_EA_NAME;
            return Iosb;
        }

        //
        //  If this is a duplicate name, we skip to the next.
        //

        if (FatIsDuplicateEaName( IrpContext, GetEa, UserEaList )) {

            DebugTrace(0, Dbg, "FatQueryEaUserEaList:  Duplicate name\n", 0);
            continue;
        }

        if (!FatLocateEaByName( IrpContext,
                                FirstPackedEa,
                                PackedEasLength,
                                &Str,
                                &Offset )) {

            Offset = 0xffffffff;

            DebugTrace(0, Dbg, "Need to dummy up an ea\n", 0);

            //
            //  We were not able to locate the name therefore we must
            //  dummy up a entry for the query.  The needed Ea size is
            //  the size of the name + 4 (next entry offset) + 1 (flags)
            //  + 1 (name length) + 2 (value length) + the name length +
            //  1 (null byte).
            //

            if ((ULONG)(4+1+1+2+GetEa->EaNameLength+1)
                > RemainingUserBufferLength) {

                Overflow = TRUE;
                break;
            }

            //
            //  Everything is going to work fine, so copy over the name,
            //  set the name length and zero out the rest of the ea.
            //

            NextFullEa->NextEntryOffset = 0;
            NextFullEa->Flags = 0;
            NextFullEa->EaNameLength = GetEa->EaNameLength;
            NextFullEa->EaValueLength = 0;
            RtlCopyMemory( &NextFullEa->EaName[0],
                           &GetEa->EaName[0],
                           GetEa->EaNameLength );

            //
            //  Upcase the name in the buffer.
            //

            OutputEaName.MaximumLength = OutputEaName.Length = Str.Length;
            OutputEaName.Buffer = NextFullEa->EaName;

            FatUpcaseEaName( IrpContext, &OutputEaName, &OutputEaName );

            NextFullEa->EaName[GetEa->EaNameLength] = 0;

        } else {

            DebugTrace(0, Dbg, "Located the ea, Offset = %08lx\n", Offset);

            //
            //  We were able to locate the packed ea
            //  Reference the packed ea
            //

            PackedEa = (PPACKED_EA) ((PUCHAR) FirstPackedEa + Offset);
            SizeOfPackedEa( PackedEa, &PackedEaSize );

            DebugTrace(0, Dbg, "PackedEaSize = %08lx\n", PackedEaSize);

            //
            //  We know that the packed ea is 4 bytes smaller than its
            //  equivalent full ea so we need to check the remaining
            //  user buffer length against the computed full ea size.
            //

            if (PackedEaSize + 4 > RemainingUserBufferLength) {

                Overflow = TRUE;
                break;
            }

            //
            //  Everything is going to work fine, so copy over the packed
            //  ea to the full ea and zero out the next entry offset field.
            //

            RtlCopyMemory( &NextFullEa->Flags,
                           &PackedEa->Flags,
                           PackedEaSize );

            NextFullEa->NextEntryOffset = 0;
        }

        //
        //  At this point we've copied a new full ea into the next full ea
        //  location.  So now go back and set the set full eas entry offset
        //  field to be the difference between out two pointers.
        //

        if (LastFullEa != NULL) {

            LastFullEa->NextEntryOffset = (ULONG)((PUCHAR) NextFullEa
                                          - (PUCHAR) LastFullEa);
        }

        //
        //  Set the last full ea to the next full ea, compute
        //  where the next full should be, and decrement the remaining user
        //  buffer length appropriately
        //

        LastFullEa = NextFullEa;
        LastFullEaSize = LongAlign( SizeOfFullEa( LastFullEa ));
        RemainingUserBufferLength -= LastFullEaSize;
        NextFullEa = (PFILE_FULL_EA_INFORMATION) ((PUCHAR) NextFullEa
                                                  + LastFullEaSize);

        //
        //  Remember the offset of the next ea in case we're asked to
        //  resume the iteration
        //

        Ccb->OffsetOfNextEaToReturn = FatLocateNextEa( IrpContext,
                                                       FirstPackedEa,
                                                       PackedEasLength,
                                                       Offset );

        //
        //  If we were to return a single entry then break out of our loop
        //  now
        //

        if (ReturnSingleEntry) {

            break;
        }
    }

    //
    //  Now we've iterated all that can and we've exited the preceding loop
    //  with either all, some or no information stored in the return buffer.
    //  We can decide if we got everything to fit by checking the local
    //  Overflow variable
    //

    if (Overflow) {

        Iosb.Information = 0;
        Iosb.Status = STATUS_BUFFER_OVERFLOW;

    } else {

        //
        //  Otherwise we've been successful in returing at least one
        //  ea so we'll compute the number of bytes used to store the
        //  full ea information.  The number of bytes used is the difference
        //  between the LastFullEa and the start of the buffer, and the
        //  non-aligned size of the last full ea.
        //

        Iosb.Information = ((PUCHAR) LastFullEa - UserBuffer)
                            + SizeOfFullEa(LastFullEa);

        Iosb.Status = STATUS_SUCCESS;
    }

    DebugTrace(-1, Dbg, "FatQueryEaUserEaList -> Iosb.Status = %08lx\n",
               Iosb.Status);

    return Iosb;
}


//
//  Local Support Routine
//

IO_STATUS_BLOCK
FatQueryEaIndexSpecified (
    IN PIRP_CONTEXT IrpContext,
    OUT PCCB Ccb,
    IN PPACKED_EA FirstPackedEa,
    IN ULONG PackedEasLength,
    OUT PUCHAR UserBuffer,
    IN ULONG  UserBufferLength,
    IN ULONG  UserEaIndex,
    IN BOOLEAN ReturnSingleEntry
    )

/*++

Routine Description:

    This routine is the work routine for querying EAs given an ea index

Arguments:

    Ccb - Supplies the Ccb for the query

    FirstPackedEa - Supplies the first ea for the file being queried

    PackedEasLength - Supplies the length of the ea data

    UserBuffer - Supplies the buffer to receive the full eas

    UserBufferLength - Supplies the length, in bytes, of the user buffer

    UserEaIndex - Supplies the index of the first ea to return.

    RestartScan - Indicates if the first item to return is at the
                  beginning of the packed ea list or if we should resume our
                  previous iteration

Return Value:

    IO_STATUS_BLOCK - Receives the completion status for the operation

--*/

{
    IO_STATUS_BLOCK Iosb;

    ULONG i;
    ULONG Offset;

    DebugTrace(+1, Dbg, "FatQueryEaIndexSpecified...\n", 0);

    //
    //  Zero out the information field of the iosb
    //

    Iosb.Information = 0;

    //
    //  If the index value is zero or there are no Eas on the file, then
    //  the specified index can't be returned.
    //

    if (UserEaIndex == 0
        || PackedEasLength == 0) {

        DebugTrace( -1, Dbg, "FatQueryEaIndexSpecified: Non-existant entry\n", 0 );

        Iosb.Status = STATUS_NONEXISTENT_EA_ENTRY;

        return Iosb;
    }

    //
    //  Iterate the eas until we find the index we're after.
    //

    for (i = 1, Offset = 0;
         (i < UserEaIndex) && (Offset < PackedEasLength);
         i += 1, Offset = FatLocateNextEa( IrpContext,
                                           FirstPackedEa,
                                           PackedEasLength, Offset )) {

        NOTHING;
    }

    //
    //  Make sure the offset we're given to the ea is a real offset otherwise
    //  the ea doesn't exist
    //

    if (Offset >= PackedEasLength) {

        //
        //  If we just passed the last Ea, we will return STATUS_NO_MORE_EAS.
        //  This is for the caller who may be enumerating the Eas.
        //

        if (i == UserEaIndex) {

            Iosb.Status = STATUS_NO_MORE_EAS;

        //
        //  Otherwise we report that this is a bad ea index.
        //

        } else {

            Iosb.Status = STATUS_NONEXISTENT_EA_ENTRY;
        }

        DebugTrace(-1, Dbg, "FatQueryEaIndexSpecified -> %08lx\n", Iosb.Status);
        return Iosb;
    }

    //
    //  We now have the offset of the first Ea to return to the user.
    //  We simply call our EaSimpleScan routine to do the actual work.
    //

    Iosb = FatQueryEaSimpleScan( IrpContext,
                                 Ccb,
                                 FirstPackedEa,
                                 PackedEasLength,
                                 UserBuffer,
                                 UserBufferLength,
                                 ReturnSingleEntry,
                                 Offset );

    DebugTrace(-1, Dbg, "FatQueryEaIndexSpecified -> %08lx\n", Iosb.Status);

    return Iosb;

}


//
//  Local Support Routine
//

IO_STATUS_BLOCK
FatQueryEaSimpleScan (
    IN PIRP_CONTEXT IrpContext,
    OUT PCCB Ccb,
    IN PPACKED_EA FirstPackedEa,
    IN ULONG PackedEasLength,
    OUT PUCHAR UserBuffer,
    IN ULONG  UserBufferLength,
    IN BOOLEAN ReturnSingleEntry,
    ULONG StartOffset
    )

/*++

Routine Description:

    This routine is the work routine for querying EAs from the beginning of
    the ea list.

Arguments:

    Ccb - Supplies the Ccb for the query

    FirstPackedEa - Supplies the first ea for the file being queried

    PackedEasLength - Supplies the length of the ea data

    UserBuffer - Supplies the buffer to receive the full eas

    UserBufferLength - Supplies the length, in bytes, of the user buffer

    ReturnSingleEntry - Indicates if we are to return a single entry or not

    StartOffset - Indicates the offset within the Ea data to return the
                  first block of data.

Return Value:

    IO_STATUS_BLOCK - Receives the completion status for the operation

--*/

{
    IO_STATUS_BLOCK Iosb;

    ULONG RemainingUserBufferLength;

    PPACKED_EA PackedEa;
    ULONG PackedEaSize;

    PFILE_FULL_EA_INFORMATION LastFullEa;
    ULONG LastFullEaSize;
    PFILE_FULL_EA_INFORMATION NextFullEa;
    BOOLEAN BufferOverflow = FALSE;


    DebugTrace(+1, Dbg, "FatQueryEaSimpleScan...\n", 0);

    //
    //  Zero out the information field in the Iosb
    //

    Iosb.Information = 0;

    LastFullEa = NULL;
    NextFullEa = (PFILE_FULL_EA_INFORMATION) UserBuffer;
    RemainingUserBufferLength = UserBufferLength;

    while (StartOffset < PackedEasLength) {

        DebugTrace(0, Dbg, "Top of loop, Offset = %08lx\n", StartOffset);
        DebugTrace(0, Dbg, "LastFullEa = %p\n", LastFullEa);
        DebugTrace(0, Dbg, "NextFullEa = %p\n", NextFullEa);
        DebugTrace(0, Dbg, "RemainingUserBufferLength = %08lx\n", RemainingUserBufferLength);

        //
        //  Reference the packed ea of interest.
        //

        PackedEa = (PPACKED_EA) ((PUCHAR) FirstPackedEa + StartOffset);

        SizeOfPackedEa( PackedEa, &PackedEaSize );

        DebugTrace(0, Dbg, "PackedEaSize = %08lx\n", PackedEaSize);

        //
        //  We know that the packed ea is 4 bytes smaller than its
        //  equivalent full ea so we need to check the remaining
        //  user buffer length against the computed full ea size.
        //

        if (PackedEaSize + 4 > RemainingUserBufferLength) {

            BufferOverflow = TRUE;
            break;
        }

        //
        //  Everything is going to work fine, so copy over the packed
        //  ea to the full ea and zero out the next entry offset field.
        //  Then go back and set the last full eas entry offset field
        //  to be the difference between the two pointers.
        //

        RtlCopyMemory( &NextFullEa->Flags, &PackedEa->Flags, PackedEaSize );
        NextFullEa->NextEntryOffset = 0;

        if (LastFullEa != NULL) {

            LastFullEa->NextEntryOffset = (ULONG)((PUCHAR) NextFullEa
                                          - (PUCHAR) LastFullEa);
        }

        //
        //  Set the last full ea to the next full ea, compute
        //  where the next full should be, and decrement the remaining user
        //  buffer length appropriately
        //

        LastFullEa = NextFullEa;
        LastFullEaSize = LongAlign( SizeOfFullEa( LastFullEa ));
        RemainingUserBufferLength -= LastFullEaSize;
        NextFullEa = (PFILE_FULL_EA_INFORMATION) ((PUCHAR) NextFullEa
                                                  + LastFullEaSize);

        //
        //  Remember the offset of the next ea in case we're asked to
        //  resume the teration
        //

        StartOffset = FatLocateNextEa( IrpContext,
                                       FirstPackedEa,
                                       PackedEasLength,
                                       StartOffset );

        Ccb->OffsetOfNextEaToReturn = StartOffset;

        //
        //  If we were to return a single entry then break out of our loop
        //  now
        //

        if (ReturnSingleEntry) {

            break;
        }
    }

    //
    //  Now we've iterated all that can and we've exited the preceding loop
    //  with either some or no information stored in the return buffer.
    //  We can decide which it is by checking if the last full ea is null
    //

    if (LastFullEa == NULL) {

        Iosb.Information = 0;

        //
        //  We were not able to return a single ea entry, now we need to find
        //  out if it is because we didn't have an entry to return or the
        //  buffer is too small.  If the Offset variable is less than
        //  PackedEaList->UsedSize then the user buffer is too small
        //

        if (PackedEasLength == 0) {

            Iosb.Status = STATUS_NO_EAS_ON_FILE;

        } else if (StartOffset >= PackedEasLength) {

            Iosb.Status = STATUS_NO_MORE_EAS;

        } else {

            Iosb.Status = STATUS_BUFFER_TOO_SMALL;
        }

    } else {

        //
        //  Otherwise we've been successful in returing at least one
        //  ea so we'll compute the number of bytes used to store the
        //  full ea information.  The number of bytes used is the difference
        //  between the LastFullEa and the start of the buffer, and the
        //  non-aligned size of the last full ea.
        //

        Iosb.Information = ((PUCHAR) LastFullEa - UserBuffer)
                            + SizeOfFullEa( LastFullEa );

        //
        //  If there are more to return, report the buffer was too small.
        //  Otherwise return STATUS_SUCCESS.
        //

        if (BufferOverflow) {

            Iosb.Status = STATUS_BUFFER_OVERFLOW;

        } else {

            Iosb.Status = STATUS_SUCCESS;
        }
    }

    DebugTrace(-1, Dbg, "FatQueryEaSimpleScan -> Iosb.Status = %08lx\n",
               Iosb.Status);

    return Iosb;

}


//
//  Local Support Routine
//

BOOLEAN
FatIsDuplicateEaName (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_GET_EA_INFORMATION GetEa,
    IN PUCHAR UserBuffer
    )

/*++

Routine Description:

    This routine walks through a list of ea names to find a duplicate name.
    'GetEa' is an actual position in the list.  We are only interested in
    previous matching ea names, as the ea information for that ea name
    would have been returned with the previous instance.

Arguments:

    GetEa - Supplies the Ea name structure for the ea name to match.

    UserBuffer - Supplies a pointer to the user buffer with the list
                 of ea names to search for.

Return Value:

    BOOLEAN - TRUE if a previous match is found, FALSE otherwise.

--*/

{
    PFILE_GET_EA_INFORMATION ThisGetEa;

    BOOLEAN DuplicateFound;
    OEM_STRING EaString;

    DebugTrace(+1, Dbg, "FatIsDuplicateEaName...\n", 0);

    EaString.MaximumLength = EaString.Length = GetEa->EaNameLength;
    EaString.Buffer = &GetEa->EaName[0];

    FatUpcaseEaName( IrpContext, &EaString, &EaString );

    DuplicateFound = FALSE;

    for (ThisGetEa = (PFILE_GET_EA_INFORMATION) &UserBuffer[0];
         ThisGetEa < GetEa
         && ThisGetEa->NextEntryOffset != 0;
         ThisGetEa = (PFILE_GET_EA_INFORMATION) ((PUCHAR) ThisGetEa
                                                 + ThisGetEa->NextEntryOffset)) {

        OEM_STRING Str;

        DebugTrace(0, Dbg, "Top of loop, ThisGetEa = %p\n", ThisGetEa);

        //
        //  Make a string reference to the GetEa and see if we can
        //  locate the ea by name
        //

        Str.MaximumLength = Str.Length = ThisGetEa->EaNameLength;
        Str.Buffer = &ThisGetEa->EaName[0];

        DebugTrace(0, Dbg, "FatIsDuplicateEaName:  Next Name -> %Z\n", &Str);

        if ( FatAreNamesEqual(IrpContext, Str, EaString) ) {

            DebugTrace(0, Dbg, "FatIsDuplicateEaName:  Duplicate found\n", 0);
            DuplicateFound = TRUE;
            break;
        }
    }

    DebugTrace(-1, Dbg, "FatIsDuplicateEaName:  Exit -> %04x\n", DuplicateFound);

    return DuplicateFound;
}
#endif


