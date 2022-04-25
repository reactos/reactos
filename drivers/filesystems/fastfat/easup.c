/*++

Copyright (c) 1990-2000 Microsoft Corporation

Module Name:

    EaSup.c

Abstract:

    This module implements the cluster operations on the EA file for Fat.


--*/

#include "fatprocs.h"

//
//  Local debug trace level
//

#define Dbg                              (DEBUG_TRACE_EA)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FatAddEaSet)
#pragma alloc_text(PAGE, FatAppendPackedEa)
#pragma alloc_text(PAGE, FatCreateEa)
#pragma alloc_text(PAGE, FatDeleteEa)
#pragma alloc_text(PAGE, FatDeleteEaSet)
#pragma alloc_text(PAGE, FatDeletePackedEa)
#pragma alloc_text(PAGE, FatGetEaFile)
#pragma alloc_text(PAGE, FatGetEaLength)
#pragma alloc_text(PAGE, FatGetNeedEaCount)
#pragma alloc_text(PAGE, FatIsEaNameValid)
#pragma alloc_text(PAGE, FatLocateEaByName)
#pragma alloc_text(PAGE, FatLocateNextEa)
#pragma alloc_text(PAGE, FatReadEaSet)
#pragma alloc_text(PAGE, FatPinEaRange)
#pragma alloc_text(PAGE, FatMarkEaRangeDirty)
#pragma alloc_text(PAGE, FatUnpinEaRange)
#endif


//
//  Any access to the Ea file must recognize when a section boundary is being
//  crossed.
//

#define EA_SECTION_SIZE             (0x00040000)


_Requires_lock_held_(_Global_critical_region_)
VOID
FatGetEaLength (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PDIRENT Dirent,
    OUT PULONG EaLength
    )

/*++

Routine Description:

    This routine looks up the Ea length for the Eas of the file.  This
    length is the  length of the packed eas, including the 4 bytes which
    contain the Ea length.

    This routine pins down the Ea set for the desired file and copies
    this field from the Ea set header.

Arguments:

    Vcb - Vcb for the volume containing the Eas.

    Dirent - Supplies a pointer to the dirent for the file in question.

    EaLength - Supplies the address to store the length of the Eas.

Return Value:

    None

--*/

{
    PBCB EaBcb = NULL;
    BOOLEAN LockedEaFcb = FALSE;
    EA_RANGE EaSetRange;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "FatGetEaLength ...\n", 0);

    //
    //  If this is Fat32 volume, or if the handle is 0 then the Ea length is 0.
    //

    if (FatIsFat32( Vcb ) ||
        Dirent->ExtendedAttributes == 0) {

        *EaLength = 0;
        DebugTrace(-1, Dbg, "FatGetEaLength -> %08lx\n", TRUE);
        return;
    }

    RtlZeroMemory( &EaSetRange, sizeof( EA_RANGE ));

    //
    //  Use a try to facilitate cleanup.
    //

    _SEH2_TRY {

        PDIRENT EaDirent;
        OEM_STRING ThisFilename;
        UCHAR Buffer[12];
        PEA_SET_HEADER EaSetHeader;

        //
        //  Initial the local values.
        //

        EaBcb = NULL;
        LockedEaFcb = FALSE;

        //
        //  Try to get the Ea file object.  Return FALSE on failure.
        //

        FatGetEaFile( IrpContext,
                      Vcb,
                      &EaDirent,
                      &EaBcb,
                      FALSE,
                      FALSE );

        LockedEaFcb = TRUE;

        //
        //  If we didn't get the file because it doesn't exist, then the
        //  disk is corrupted.
        //

        if (Vcb->VirtualEaFile == NULL) {

            DebugTrace(0, Dbg, "FatGetEaLength:  Ea file doesn't exist\n", 0);
            FatRaiseStatus( IrpContext, STATUS_NO_EAS_ON_FILE );
        }

        //
        //  Try to pin down the Ea set header for the index in the
        //  dirent.  If the operation doesn't complete, return FALSE
        //  from this routine.
        //

        ThisFilename.Buffer = (PCHAR)Buffer;
        Fat8dot3ToString( IrpContext, Dirent, FALSE, &ThisFilename );

        FatReadEaSet( IrpContext,
                      Vcb,
                      Dirent->ExtendedAttributes,
                      &ThisFilename,
                      FALSE,
                      &EaSetRange );

        EaSetHeader = (PEA_SET_HEADER) EaSetRange.Data;

        //
        //  We now have the Ea set header for this file.  We simply copy
        //  the Ea length field.
        //

        CopyUchar4( EaLength, EaSetHeader->cbList );
        DebugTrace(0, Dbg, "FatGetEaLength:  Length of Ea is -> %08lx\n",
                   *EaLength);

    } _SEH2_FINALLY {

        DebugUnwind( FatGetEaLength );

        //
        //  Unpin the EaDirent and the EaSetHeader if pinned.
        //

        FatUnpinBcb( IrpContext, EaBcb );

        FatUnpinEaRange( IrpContext, &EaSetRange );

        //
        //  Release the Fcb for the Ea file if locked.
        //

        if (LockedEaFcb) {

            FatReleaseFcb( IrpContext, Vcb->EaFcb );
        }

        DebugTrace(-1, Dbg, "FatGetEaLength:  Ea length -> %08lx\n", *EaLength);
    } _SEH2_END;

    return;
}


_Requires_lock_held_(_Global_critical_region_)
VOID
FatGetNeedEaCount (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PDIRENT Dirent,
    OUT PULONG NeedEaCount
    )

/*++

Routine Description:

    This routine looks up the Need Ea count for the file.  The value is the
    in the ea header for the file.

Arguments:

    Vcb - Vcb for the volume containing the Eas.

    Dirent - Supplies a pointer to the dirent for the file in question.

    NeedEaCount - Supplies the address to store the Need Ea count.

Return Value:

    None

--*/

{
    PBCB EaBcb = NULL;
    BOOLEAN LockedEaFcb = FALSE;
    EA_RANGE EaSetRange;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "FatGetNeedEaCount ...\n", 0);

    //
    //  If the handle is 0 then the Need Ea count is 0.
    //

    if (Dirent->ExtendedAttributes == 0) {

        *NeedEaCount = 0;
        DebugTrace(-1, Dbg, "FatGetNeedEaCount -> %08lx\n", TRUE);
        return;
    }

    RtlZeroMemory( &EaSetRange, sizeof( EA_RANGE ));

    //
    //  Use a try to facilitate cleanup.
    //

    _SEH2_TRY {

        PDIRENT EaDirent;
        OEM_STRING ThisFilename;
        UCHAR Buffer[12];
        PEA_SET_HEADER EaSetHeader;

        //
        //  Initial the local values.
        //

        EaBcb = NULL;
        LockedEaFcb = FALSE;

        //
        //  Try to get the Ea file object.  Return FALSE on failure.
        //

        FatGetEaFile( IrpContext,
                      Vcb,
                      &EaDirent,
                      &EaBcb,
                      FALSE,
                      FALSE );

        LockedEaFcb = TRUE;

        //
        //  If we didn't get the file because it doesn't exist, then the
        //  disk is corrupted.
        //

        if (Vcb->VirtualEaFile == NULL) {

            DebugTrace(0, Dbg, "FatGetNeedEaCount:  Ea file doesn't exist\n", 0);
            FatRaiseStatus( IrpContext, STATUS_NO_EAS_ON_FILE );
        }

        //
        //  Try to pin down the Ea set header for the index in the
        //  dirent.  If the operation doesn't complete, return FALSE
        //  from this routine.
        //

        ThisFilename.Buffer = (PCHAR)Buffer;
        Fat8dot3ToString( IrpContext, Dirent, FALSE, &ThisFilename );

        FatReadEaSet( IrpContext,
                      Vcb,
                      Dirent->ExtendedAttributes,
                      &ThisFilename,
                      FALSE,
                      &EaSetRange );

        EaSetHeader = (PEA_SET_HEADER) EaSetRange.Data;

        //
        //  We now have the Ea set header for this file.  We simply copy
        //  the Need Ea field.
        //

        *NeedEaCount = EaSetHeader->NeedEaCount;

    } _SEH2_FINALLY {

        DebugUnwind( FatGetNeedEaCount );

        //
        //  Unpin the EaDirent and the EaSetHeader if pinned.
        //

        FatUnpinBcb( IrpContext, EaBcb );

        FatUnpinEaRange( IrpContext, &EaSetRange );

        //
        //  Release the Fcb for the Ea file if locked.
        //

        if (LockedEaFcb) {

            FatReleaseFcb( IrpContext, Vcb->EaFcb );
        }

        DebugTrace(-1, Dbg, "FatGetNeedEaCount:  NeedEaCount -> %08lx\n", *NeedEaCount);
    } _SEH2_END;

    return;
}


_Requires_lock_held_(_Global_critical_region_)
VOID
FatCreateEa (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PUCHAR Buffer,
    IN ULONG Length,
    IN POEM_STRING FileName,
    OUT PUSHORT EaHandle
    )

/*++

Routine Description:

    This routine adds an entire ea set to the Ea file.  The owning file
    is specified in 'FileName'.  This is used to replace the Ea set attached
    to an existing file during a supersede operation.

    NOTE: This routine may block, it should not be called unless the
    thread is waitable.

Arguments:

    Vcb - Supplies the Vcb for the volume.

    Buffer - Buffer with the Ea list to add.

    Length - Length of the buffer.

    FileName - The Ea's will be attached to this file.

    EaHandle - The new ea handle will be assigned to this address.

Return Value:

    None

--*/

{
    PBCB EaBcb;
    BOOLEAN LockedEaFcb;

    PEA_SET_HEADER EaSetHeader = NULL;
    EA_RANGE EaSetRange;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "FatCreateEa...\n", 0);

    EaBcb = NULL;
    LockedEaFcb = FALSE;

    RtlZeroMemory( &EaSetRange, sizeof( EA_RANGE ));

    //
    //  Use 'try' to facilitate cleanup.
    //

    _SEH2_TRY {

        PDIRENT EaDirent;

        ULONG PackedEasLength;
        ULONG AllocationLength;
        ULONG BytesPerCluster;

        PFILE_FULL_EA_INFORMATION FullEa;

        //
        //  We will allocate a buffer and copy the Ea list from the user's
        //  buffer to a FAT packed Ea list.  Initial allocation is one
        //  cluster, our starting offset into the packed Ea list is 0.
        //

        PackedEasLength = 0;

        BytesPerCluster = 1 << Vcb->AllocationSupport.LogOfBytesPerCluster;

        AllocationLength = (PackedEasLength
                            + SIZE_OF_EA_SET_HEADER
                            + BytesPerCluster - 1)
                           & ~(BytesPerCluster - 1);

        //
        //  Allocate the memory and store the file name into it.
        //

        EaSetHeader = FsRtlAllocatePoolWithTag( PagedPool,
                                                AllocationLength,
                                                TAG_EA_SET_HEADER );

        RtlZeroMemory( EaSetHeader, AllocationLength );

        RtlCopyMemory( EaSetHeader->OwnerFileName,
                       FileName->Buffer,
                       FileName->Length );

        AllocationLength -= SIZE_OF_EA_SET_HEADER;

        //
        //  Loop through the user's Ea list.  Catch any error for invalid
        //  name or non-existent Ea value.
        //

        for ( FullEa = (PFILE_FULL_EA_INFORMATION) Buffer;
              FullEa < (PFILE_FULL_EA_INFORMATION) &Buffer[Length];
              FullEa = (PFILE_FULL_EA_INFORMATION) (FullEa->NextEntryOffset == 0 ?
                                   &Buffer[Length] :
                                   (PUCHAR) FullEa + FullEa->NextEntryOffset)) {

            OEM_STRING EaName;
            ULONG EaOffset;

            EaName.Length = FullEa->EaNameLength;
            EaName.Buffer = &FullEa->EaName[0];

            //
            //  Make sure the ea name is valid
            //

            if (!FatIsEaNameValid( IrpContext, EaName )) {

                DebugTrace(0, Dbg,
                           "FatCreateEa:  Invalid Ea Name -> %Z\n",
                           EaName);

                IrpContext->OriginatingIrp->IoStatus.Information = (PUCHAR)FullEa - Buffer;
                IrpContext->OriginatingIrp->IoStatus.Status = STATUS_INVALID_EA_NAME;
                FatRaiseStatus( IrpContext, STATUS_INVALID_EA_NAME );
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

                IrpContext->OriginatingIrp->IoStatus.Information = (PUCHAR)FullEa - Buffer;
                IrpContext->OriginatingIrp->IoStatus.Status = STATUS_INVALID_EA_NAME;
                FatRaiseStatus( IrpContext, STATUS_INVALID_EA_NAME );
            }

            //
            //  If this is a duplicate name then delete the current ea
            //  value.
            //

            if (FatLocateEaByName( IrpContext,
                                   (PPACKED_EA) EaSetHeader->PackedEas,
                                   PackedEasLength,
                                   &EaName,
                                   &EaOffset )) {

                DebugTrace(0, Dbg, "FatCreateEa:  Duplicate name found\n", 0);

                FatDeletePackedEa( IrpContext,
                                   EaSetHeader,
                                   &PackedEasLength,
                                   EaOffset );
            }

            //
            //  We ignore this value if the eavalue length is zero.
            //

            if (FullEa->EaValueLength == 0) {

                DebugTrace(0, Dbg,
                           "FatCreateEa:  Empty ea\n",
                           0);

                continue;
            }

            FatAppendPackedEa( IrpContext,
                               &EaSetHeader,
                               &PackedEasLength,
                               &AllocationLength,
                               FullEa,
                               BytesPerCluster );
        }

        //
        //  If the resulting length isn't zero, then allocate a FAT cluster
        //  to store the data.
        //

        if (PackedEasLength != 0) {

            PEA_SET_HEADER NewEaSetHeader;

            //
            //  If the packed eas length (plus 4 bytes) is greater
            //  than the maximum allowed ea size, we return an error.
            //

            if (PackedEasLength + 4 > MAXIMUM_EA_SIZE) {

                DebugTrace( 0, Dbg, "Ea length is greater than maximum\n", 0 );

                FatRaiseStatus( IrpContext, STATUS_EA_TOO_LARGE );
            }

            //
            //  Get the Ea file.
            //

            FatGetEaFile( IrpContext,
                          Vcb,
                          &EaDirent,
                          &EaBcb,
                          TRUE,
                          TRUE );

            LockedEaFcb = TRUE;

            FatAddEaSet( IrpContext,
                         Vcb,
                         PackedEasLength + SIZE_OF_EA_SET_HEADER,
                         EaBcb,
                         EaDirent,
                         EaHandle,
                         &EaSetRange );

            NewEaSetHeader = (PEA_SET_HEADER) EaSetRange.Data;

            //
            //  Store the length of the new Ea's into the NewEaSetHeader.
            //  This is the PackedEasLength + 4.
            //

            PackedEasLength += 4;

            CopyU4char( EaSetHeader->cbList, &PackedEasLength );

            //
            //  Copy all but the first four bytes of EaSetHeader into
            //  the new ea.  The signature and index fields have
            //  already been filled in.
            //

            RtlCopyMemory( &NewEaSetHeader->NeedEaCount,
                           &EaSetHeader->NeedEaCount,
                           PackedEasLength + SIZE_OF_EA_SET_HEADER - 8 );

            FatMarkEaRangeDirty( IrpContext, Vcb->VirtualEaFile, &EaSetRange );
            FatUnpinEaRange( IrpContext, &EaSetRange );

            CcFlushCache( Vcb->VirtualEaFile->SectionObjectPointer, NULL, 0, NULL );

        //
        //  There was no data added to the Ea file.  Return a handle
        //  of 0.
        //

        } else {

            *EaHandle = 0;
        }

    } _SEH2_FINALLY {

        DebugUnwind( FatCreateEa );

        //
        //  Deallocate the EaSetHeader if present.
        //

        if (EaSetHeader) {

            ExFreePool( EaSetHeader );
        }

        //
        //  Release the EaFcb if held.
        //

        if (LockedEaFcb) {

            FatReleaseFcb( IrpContext, Vcb->EaFcb );
        }

        //
        //  Unpin the dirents for the EaFcb and EaSetFcb if necessary.
        //

        FatUnpinBcb( IrpContext, EaBcb );
        FatUnpinEaRange( IrpContext, &EaSetRange );

        DebugTrace(-1, Dbg, "FatCreateEa -> Exit\n", 0);
    } _SEH2_END;

    return;
}

_Requires_lock_held_(_Global_critical_region_)
VOID
FatDeleteEa (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN USHORT EaHandle,
    IN POEM_STRING FileName
    )

/*++

Routine Description:

    This routine is called to remove an entire ea set.  Most of the work
    is done in the call to 'FatDeleteEaSet'.  This routine opens the
    Ea file and then calls the support routine.

    NOTE: This routine may block, it should not be called unless the
    thread is waitable.

Arguments:

    Vcb - Vcb for the volume

    EaHandle - The handle for the Ea's to remove.  This handle will be
               verified during this operation.

    FileName - The name of the file whose Ea's are being removed.  This
               name is compared against the Ea owner's name in the Ea set.

Return Value:

    None.

--*/

{
    PBCB EaBcb;
    BOOLEAN LockedEaFcb;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "FatDeleteEa...\n", 0);

    //
    //  Initialize local values.
    //

    EaBcb = NULL;
    LockedEaFcb = FALSE;

    //
    //  Use a try statement to facilitate cleanup.
    //

    _SEH2_TRY {

        PDIRENT EaDirent;

        //
        //  Get the Ea stream file.  If the file doesn't exist on the disk
        //  then the disk has been corrupted.
        //

        FatGetEaFile( IrpContext,
                      Vcb,
                      &EaDirent,
                      &EaBcb,
                      FALSE,
                      TRUE );

        LockedEaFcb = TRUE;

        //
        //  If we didn't get the Ea file, then the disk is corrupt.
        //

        if ( EaBcb == NULL ) {


            DebugTrace(0, Dbg,
                       "FatDeleteEa:  No Ea file exists\n",
                       0);

            FatRaiseStatus( IrpContext, STATUS_NO_EAS_ON_FILE );
        }

        //
        //  We now have everything we need to delete the ea set.  Call the
        //  support routine to do this.
        //

        FatDeleteEaSet( IrpContext,
                        Vcb,
                        EaBcb,
                        EaDirent,
                        EaHandle,
                        FileName );

        CcFlushCache( Vcb->VirtualEaFile->SectionObjectPointer, NULL, 0, NULL );

    } _SEH2_FINALLY {

        DebugUnwind( FatDeleteEa );

        //
        //  Release the EaFcb if held.
        //

        if (LockedEaFcb) {

            FatReleaseFcb( IrpContext, Vcb->EaFcb );
        }

        //
        //  Unpin the dirent for the Ea file if pinned.
        //

        FatUnpinBcb( IrpContext, EaBcb );

        DebugTrace(-1, Dbg, "FatDeleteEa -> Exit\n", 0);
    } _SEH2_END;

    return;
}


_Requires_lock_held_(_Global_critical_region_)
VOID
FatGetEaFile (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVCB Vcb,
    OUT PDIRENT *EaDirent,
    OUT PBCB *EaBcb,
    IN BOOLEAN CreateFile,
    IN BOOLEAN ExclusiveFcb
    )

/*++

Routine Description:

    This routine is used to completely initialize the Vcb and
    the Ea file for the Vcb.

    If the Vcb doesn't have the Ea file object, then we first try to
    lookup the Ea data file in the root directory and if that fails
    we try to create the file.  The 'CreateFile' flag is used to check
    whether it is necessary to create the Ea file.

    This routine will lock down the Fcb for exclusive or shared access before
    performing any operations.  If the operation does not complete due
    to blocking, exclusive or shared access will be given up before returning.

    If we are creating the Ea file and marking sections of it dirty,
    we can't use the repin feature through the cache map.  In that case
    we use a local IrpContext and then unpin all of the Bcb's before
    continuing.

    Note: If this routine will be creating the Ea file, we are guaranteed
    to be waitable.

Arguments:

    Vcb - Vcb for the volume

    EaDirent - Location to store the address of the pinned dirent for the
               Ea file.

    EaBcb - Location to store the address of the Bcb for the pinned dirent.

    CreateFile - Boolean indicating whether we should create the Ea file
                 on the disk.

    ExclusiveFcb - Indicates whether shared or exclusive access is desired
                   for the EaFcb.

Return Value:

    None.

--*/

{
    PFILE_OBJECT EaStreamFile = NULL;
    EA_RANGE EaFileRange;

    BOOLEAN UnwindLockedEaFcb = FALSE;
    BOOLEAN UnwindLockedRootDcb = FALSE;
    BOOLEAN UnwindAllocatedDiskSpace = FALSE;
    BOOLEAN UnwindEaDirentCreated = FALSE;
    BOOLEAN UnwindUpdatedSizes = FALSE;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "FatGetEaFile ...\n", 0);

    RtlZeroMemory( &EaFileRange, sizeof( EA_RANGE ));

    //
    //  Use a try to facilitate cleanup
    //

    _SEH2_TRY {

        OEM_STRING EaFileName;
        LARGE_INTEGER SectionSize;

        //
        //  Check if the Vcb already has the file object.  If it doesn't, then
        //  we need to search the root directory for the Ea data file.
        //

        if (Vcb->VirtualEaFile == NULL) {

            //
            //  Always lock the Ea file exclusively if we have to create the file.
            //

            if ( !FatAcquireExclusiveFcb( IrpContext, Vcb->EaFcb )) {

                DebugTrace(0, Dbg, "FatGetEaFile:  Can't grab exclusive\n", 0);
                FatRaiseStatus( IrpContext, STATUS_CANT_WAIT );
            }

            UnwindLockedEaFcb = TRUE;

        //
        //  Otherwise we acquire the Fcb as the caller requested.
        //

        } else {

            if ((ExclusiveFcb && !FatAcquireExclusiveFcb( IrpContext, Vcb->EaFcb ))
                || (!ExclusiveFcb && !FatAcquireSharedFcb( IrpContext, Vcb->EaFcb))) {

                DebugTrace(0, Dbg, "FatGetEaFile:  Can't grab EaFcb\n", 0);

                FatRaiseStatus( IrpContext, STATUS_CANT_WAIT );
            }

            UnwindLockedEaFcb = TRUE;

            //
            //  If the file now does not exist we need to release the Fcb and
            //  reacquire exclusive if we acquired shared.
            //

            if ((Vcb->VirtualEaFile == NULL) && !ExclusiveFcb) {

                FatReleaseFcb( IrpContext, Vcb->EaFcb );
                UnwindLockedEaFcb = FALSE;

                if (!FatAcquireExclusiveFcb( IrpContext, Vcb->EaFcb )) {

                    DebugTrace(0, Dbg, "FatGetEaFile:  Can't grab EaFcb\n", 0);

                    FatRaiseStatus( IrpContext, STATUS_CANT_WAIT );
                }

                UnwindLockedEaFcb = TRUE;
            }
        }

        //
        //  If the file object is now there we only need to get the
        //  dirent for the Ea file.
        //

        if (Vcb->VirtualEaFile != NULL) {

            FatVerifyFcb( IrpContext, Vcb->EaFcb );

            FatGetDirentFromFcbOrDcb( IrpContext,
                                      Vcb->EaFcb,
                                      FALSE,
                                      EaDirent,
                                      EaBcb );

            try_return( NOTHING );

        } else {

            VBO ByteOffset = 0;

            //
            //  Always mark the ea fcb as good.
            //

            Vcb->EaFcb->FcbCondition = FcbGood;

            //
            //  We try to lookup the dirent for the Ea Fcb.
            //

            EaFileName.Buffer = "EA DATA. SF";
            EaFileName.Length = 11;
            EaFileName.MaximumLength = 12;

            //
            //  Now pick up the root directory to be synchronized with
            //  deletion/creation of entries.  If we may create the file,
            //  get it exclusive right now.
            //
            //  Again, note how we are relying on bottom-up lockorder. We
            //  already got the EaFcb.
            //

            if (CreateFile) {
                ExAcquireResourceExclusiveLite( Vcb->RootDcb->Header.Resource, TRUE );
            } else {
                ExAcquireResourceSharedLite( Vcb->RootDcb->Header.Resource, TRUE );
            }
            UnwindLockedRootDcb = TRUE;

            FatLocateSimpleOemDirent( IrpContext,
                                      Vcb->EaFcb->ParentDcb,
                                      &EaFileName,
                                      EaDirent,
                                      EaBcb,
                                      &ByteOffset );

            //
            //  If the file exists, we need to create the virtual file
            //  object for it.
            //

            if (*EaDirent != NULL) {

                //
                //  Since we may be modifying the dirent, pin the data now.
                //

                FatPinMappedData( IrpContext,
                                  Vcb->EaFcb->ParentDcb,
                                  ByteOffset,
                                  sizeof(DIRENT),
                                  EaBcb );

                //
                //  Update the Fcb with information on the file size
                //  and disk location.  Also increment the open/unclean
                //  counts in the EaFcb and the open count in the
                //  Vcb.
                //

                Vcb->EaFcb->FirstClusterOfFile = (*EaDirent)->FirstClusterOfFile;
                Vcb->EaFcb->DirentOffsetWithinDirectory = ByteOffset;

                //
                //  Find the allocation size.  The purpose here is
                //  really to completely fill in the Mcb for the
                //  file.
                //

                Vcb->EaFcb->Header.AllocationSize.QuadPart = FCB_LOOKUP_ALLOCATIONSIZE_HINT;

                FatLookupFileAllocationSize( IrpContext, Vcb->EaFcb );

                //
                //  Start by computing the section size for the cache
                //  manager.
                //

                SectionSize.QuadPart = (*EaDirent)->FileSize;
                Vcb->EaFcb->Header.AllocationSize = SectionSize;
                Vcb->EaFcb->Header.FileSize = SectionSize;

                //
                //  Create and initialize the file object for the
                //  Ea virtual file.
                //

                EaStreamFile = FatOpenEaFile( IrpContext, Vcb->EaFcb );

                Vcb->VirtualEaFile = EaStreamFile;

            //
            //  Else there was no dirent.  If we were instructed to
            //  create the file object, we will try to create the dirent,
            //  allocate disk space, initialize the Ea file header and
            //  return this information to the user.
            //

            } else if (CreateFile) {

                ULONG BytesPerCluster;
                ULONG OffsetTableSize;
                ULONG AllocationSize;
                PEA_FILE_HEADER FileHeader;
                USHORT AllocatedClusters;
                PUSHORT CurrentIndex;
                ULONG Index;
                NTSTATUS Status = STATUS_SUCCESS;

                DebugTrace(0, Dbg, "FatGetEaFile:  Creating local IrpContext\n", 0);

                BytesPerCluster = 1 << Vcb->AllocationSupport.LogOfBytesPerCluster;

                AllocationSize = (((ULONG) sizeof( EA_FILE_HEADER ) << 1) + BytesPerCluster - 1)
                                 & ~(BytesPerCluster - 1);

                AllocatedClusters = (USHORT) (AllocationSize
                                    >> Vcb->AllocationSupport.LogOfBytesPerCluster);

                OffsetTableSize = AllocationSize - sizeof( EA_FILE_HEADER );

                //
                //  Allocate disk space, the space allocated is 1024 bytes
                //  rounded up to the nearest cluster size.
                //

                FatAllocateDiskSpace( IrpContext,
                                      Vcb,
                                      0,
                                      &AllocationSize,
                                      FALSE,
                                      &Vcb->EaFcb->Mcb );

                UnwindAllocatedDiskSpace = TRUE;

                //
                //  Allocate and initialize a dirent in the root directory
                //  to describe this new file.
                //

                Vcb->EaFcb->DirentOffsetWithinDirectory =
                    FatCreateNewDirent( IrpContext,
                                        Vcb->EaFcb->ParentDcb,
                                        1,
                                        FALSE );

                FatPrepareWriteDirectoryFile( IrpContext,
                                              Vcb->EaFcb->ParentDcb,
                                              Vcb->EaFcb->DirentOffsetWithinDirectory,
                                              sizeof(DIRENT),
                                              EaBcb,
#ifndef __REACTOS__
                                              EaDirent,
#else
                                              (PVOID *)EaDirent,
#endif
                                              FALSE,
                                              TRUE,
                                              &Status );

                NT_ASSERT( NT_SUCCESS( Status ));

                UnwindEaDirentCreated = TRUE;

                FatConstructDirent( IrpContext,
                                    *EaDirent,
                                    &EaFileName,
                                    FALSE,
                                    FALSE,
                                    NULL,
                                    FAT_DIRENT_ATTR_READ_ONLY
                                    | FAT_DIRENT_ATTR_HIDDEN
                                    | FAT_DIRENT_ATTR_SYSTEM
                                    | FAT_DIRENT_ATTR_ARCHIVE,
                                    TRUE,
                                    NULL );

                (*EaDirent)->FileSize = AllocationSize;

                //
                //  Initialize the Fcb for this file and initialize the
                //  cache map as well.
                //

                //
                //  Start by computing the section size for the cache
                //  manager.
                //

                SectionSize.QuadPart = (*EaDirent)->FileSize;
                Vcb->EaFcb->Header.AllocationSize = SectionSize;
                Vcb->EaFcb->Header.FileSize = SectionSize;
                UnwindUpdatedSizes = TRUE;

                //
                //  Create and initialize the file object for the
                //  Ea virtual file.
                //

                EaStreamFile = FatOpenEaFile( IrpContext, Vcb->EaFcb );

                //
                //  Update the Fcb with information on the file size
                //  and disk location.  Also increment the open/unclean
                //  counts in the EaFcb and the open count in the
                //  Vcb.
                //

                {
                    LBO FirstLboOfFile;

                    FatLookupMcbEntry( Vcb, &Vcb->EaFcb->Mcb,
                                       0,
                                       &FirstLboOfFile,
                                       NULL,
                                       NULL );

                    //
                    //  The discerning reader will note that this doesn't take
                    //  FAT32 into account, which is of course intentional.
                    //

                    (*EaDirent)->FirstClusterOfFile =
                        (USHORT) FatGetIndexFromLbo( Vcb, FirstLboOfFile );
                }

                Vcb->EaFcb->FirstClusterOfFile = (*EaDirent)->FirstClusterOfFile;

                //
                //  Initialize the Ea file header and mark the Bcb as dirty.
                //

                FatPinEaRange( IrpContext,
                               EaStreamFile,
                               Vcb->EaFcb,
                               &EaFileRange,
                               0,
                               AllocationSize,
                               STATUS_DATA_ERROR );

                FileHeader = (PEA_FILE_HEADER) EaFileRange.Data;

                RtlZeroMemory( FileHeader, AllocationSize );
                FileHeader->Signature = EA_FILE_SIGNATURE;

                for (Index = MAX_EA_BASE_INDEX, CurrentIndex = FileHeader->EaBaseTable;
                     Index;
                     Index--, CurrentIndex++) {

                    *CurrentIndex = AllocatedClusters;
                }

                //
                //  Initialize the offset table with the offset set to
                //  after the just allocated clusters.
                //

                for (Index = OffsetTableSize >> 1,
                        CurrentIndex = (PUSHORT) ((PUCHAR) FileHeader + sizeof( EA_FILE_HEADER ));
                     Index;
                     Index--, CurrentIndex++) {

                    *CurrentIndex = UNUSED_EA_HANDLE;
                }

                //
                //  Unpin the file header and offset table.
                //

                FatMarkEaRangeDirty( IrpContext, EaStreamFile, &EaFileRange );
                FatUnpinEaRange( IrpContext, &EaFileRange );

                CcFlushCache( EaStreamFile->SectionObjectPointer, NULL, 0, NULL );

                //
                //  Return the Ea file object to the user.
                //

                Vcb->VirtualEaFile = EaStreamFile;
            }
        }
    try_exit:  NOTHING;
    } _SEH2_FINALLY {

        DebugUnwind( FatGetEaFile );

        //
        //  If this is abnormal termination and disk space has been
        //  allocated.  We deallocate it now.
        //

        if (_SEH2_AbnormalTermination()) {

            //
            //  Deallocate the Ea file
            //

            if (UnwindAllocatedDiskSpace) {

                FatDeallocateDiskSpace( IrpContext,
                                        Vcb,
                                        &Vcb->EaFcb->Mcb,
                                        FALSE );
            }

            //
            //  Delete the dirent for the Ea file, if created.
            //

            if (UnwindEaDirentCreated) {

                if (UnwindUpdatedSizes) {

                    Vcb->EaFcb->Header.AllocationSize.QuadPart = 0;
                    Vcb->EaFcb->Header.FileSize.QuadPart = 0;
                }

                FatUnpinBcb( IrpContext, *EaBcb );
                FatDeleteDirent( IrpContext, Vcb->EaFcb, NULL, TRUE );
            }

            //
            //  Release the EA Fcb if held
            //

            if (UnwindLockedEaFcb) {

                FatReleaseFcb( IrpContext, Vcb->EaFcb );
            }

            //
            //  Dereference the Ea stream file if created.
            //

            if (EaStreamFile != NULL) {

                ObDereferenceObject( EaStreamFile );
            }
        }

        //
        //  Always release the root Dcb (our caller releases the EA Fcb if we
        //  do not raise).
        //

        if (UnwindLockedRootDcb) {

            FatReleaseFcb( IrpContext, Vcb->RootDcb );
        }

        //
        //  If the Ea file header is locked down.  We unpin it now.
        //

        FatUnpinEaRange( IrpContext, &EaFileRange );

        DebugTrace(-1, Dbg, "FatGetEaFile:  Exit\n", 0);
    } _SEH2_END;

    return;
}


VOID
FatReadEaSet (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN USHORT EaHandle,
    IN POEM_STRING FileName,
    IN BOOLEAN ReturnEntireSet,
    OUT PEA_RANGE EaSetRange
    )

/*++

Routine Description:

    This routine pins the Ea set for the given ea handle within the
    Ea stream file.  The EaHandle, after first comparing against valid
    index values, is used to compute the cluster offset for this
    this Ea set.  The Ea set is then verified as belonging to this
    index and lying within the Ea data file.

    The caller of this function will have verified that the Ea file
    exists and that the Vcb field points to an initialized cache file.
    The caller will already have gained exclusive access to the
    EaFcb.

Arguments:

    Vcb - Supplies the Vcb for the volume.

    EaHandle - Supplies the handle for the Ea's to read.

    FileName - Name of the file whose Ea's are being read.

    ReturnEntireSet - Indicates if the caller needs the entire set
        as opposed to just the header.

    EaSetRange - Pointer to the EaRange structure which will describe the Ea
        on return.

Return Value:

    None

--*/

{
    ULONG BytesPerCluster = 1 << Vcb->AllocationSupport.LogOfBytesPerCluster;

    ULONG EaOffsetVbo;
    EA_RANGE EaOffsetRange;
    USHORT EaOffsetCluster;

    EA_RANGE EaHeaderRange;
    PEA_FILE_HEADER EaHeader;

    ULONG EaSetVbo;
    PEA_SET_HEADER EaSet;

    ULONG CbList;

    PAGED_CODE();

    UNREFERENCED_PARAMETER( FileName );

    DebugTrace(+1, Dbg, "FatReadEaSet\n", 0);
    DebugTrace( 0, Dbg, "  Vcb      = %p\n", Vcb);

    //
    //  Verify that the Ea index has a legal value.  Raise status
    //  STATUS_NONEXISTENT_EA_ENTRY if illegal.
    //

    if (EaHandle < MIN_EA_HANDLE
        || EaHandle > MAX_EA_HANDLE) {

        DebugTrace(-1, Dbg, "FatReadEaSet: Illegal handle value\n", 0);
        FatRaiseStatus( IrpContext, STATUS_NONEXISTENT_EA_ENTRY );
    }

    //
    //  Verify that the virtual Ea file is large enough for us to read
    //  the EaOffet table for this index.
    //

    EaOffsetVbo = sizeof( EA_FILE_HEADER ) + (((ULONGLONG)EaHandle >> 7) << 8);

    //
    //  Zero the Ea range structures.
    //

    RtlZeroMemory( &EaHeaderRange, sizeof( EA_RANGE ));
    RtlZeroMemory( &EaOffsetRange, sizeof( EA_RANGE ));

    //
    //  Use a try statement to clean up on exit.
    //

    _SEH2_TRY {

        //
        //  Pin down the EA file header.
        //

        FatPinEaRange( IrpContext,
                       Vcb->VirtualEaFile,
                       Vcb->EaFcb,
                       &EaHeaderRange,
                       0,
                       sizeof( EA_FILE_HEADER ),
                       STATUS_NONEXISTENT_EA_ENTRY );

        EaHeader = (PEA_FILE_HEADER) EaHeaderRange.Data;

        //
        //  Pin down the Ea offset table for the particular index.
        //

        FatPinEaRange( IrpContext,
                       Vcb->VirtualEaFile,
                       Vcb->EaFcb,
                       &EaOffsetRange,
                       EaOffsetVbo,
                       sizeof( EA_OFF_TABLE ),
                       STATUS_NONEXISTENT_EA_ENTRY );

        //
        //  Check if the specifific handle is currently being used.
        //

        EaOffsetCluster = *((PUSHORT) EaOffsetRange.Data
                            + (EaHandle & (MAX_EA_OFFSET_INDEX - 1)));

        if (EaOffsetCluster == UNUSED_EA_HANDLE) {

            DebugTrace(0, Dbg, "FatReadEaSet: Ea handle is unused\n", 0);
            FatRaiseStatus( IrpContext, STATUS_NONEXISTENT_EA_ENTRY );
        }

        //
        //  Compute the file offset for the Ea data.
        //

        EaSetVbo = (EaHeader->EaBaseTable[EaHandle >> 7] + EaOffsetCluster)
                   << Vcb->AllocationSupport.LogOfBytesPerCluster;

        //
        //  Unpin the file header and offset table.
        //

        FatUnpinEaRange( IrpContext, &EaHeaderRange );
        FatUnpinEaRange( IrpContext, &EaOffsetRange );

        //
        //  Pin the ea set.
        //

        FatPinEaRange( IrpContext,
                       Vcb->VirtualEaFile,
                       Vcb->EaFcb,
                       EaSetRange,
                       EaSetVbo,
                       BytesPerCluster,
                       STATUS_DATA_ERROR );

        //
        //  Verify that the Ea set is valid and belongs to this index.
        //  Raise STATUS_DATA_ERROR if there is a data conflict.
        //

        EaSet = (PEA_SET_HEADER) EaSetRange->Data;

        if (EaSet->Signature != EA_SET_SIGNATURE
            || EaSet->OwnEaHandle != EaHandle ) {

            DebugTrace(0, Dbg, "FatReadEaSet: Ea set header is corrupt\n", 0);
            FatRaiseStatus( IrpContext, STATUS_DATA_ERROR );
        }

        //
        //  At this point we have pinned a single cluster of Ea data.  If
        //  this represents the entire Ea data for the Ea index, we are
        //  done.  Otherwise we need to check on the entire size of
        //  of the Ea set header and whether it is contained in the allocated
        //  size of the Ea virtual file.  At that point we can unpin
        //  the partial Ea set header and repin the entire header.
        //

        CbList = GetcbList( EaSet );

        if (ReturnEntireSet
            && CbList > BytesPerCluster ) {

            //
            //  Round up to the cluster size.
            //

            CbList = (CbList + EA_CBLIST_OFFSET + BytesPerCluster - 1)
                     & ~(BytesPerCluster - 1);

            FatUnpinEaRange( IrpContext, EaSetRange );

            RtlZeroMemory( EaSetRange, sizeof( EA_RANGE ));

            FatPinEaRange( IrpContext,
                           Vcb->VirtualEaFile,
                           Vcb->EaFcb,
                           EaSetRange,
                           EaSetVbo,
                           CbList,
                           STATUS_DATA_ERROR );
        }

    } _SEH2_FINALLY {

        DebugUnwind( FatReadEaSet );

        //
        //  Unpin the Ea base and offset tables if locked down.
        //

        FatUnpinEaRange( IrpContext, &EaHeaderRange );
        FatUnpinEaRange( IrpContext, &EaOffsetRange );

        DebugTrace(-1, Dbg, "FatReadEaSet:  Exit\n", 0);
    } _SEH2_END;

    return;
}


_Requires_lock_held_(_Global_critical_region_)
VOID
FatDeleteEaSet (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PBCB EaBcb,
    OUT PDIRENT EaDirent,
    IN USHORT EaHandle,
    IN POEM_STRING FileName
    )

/*++

Routine Description:

    This routines clips the Ea set for a particular index out of the
    Ea file for a volume.  The index is verified as belonging to a valid
    handle.  The clusters are removed and the Ea stream file along with
    the Ea base and offset files are updated.

    The caller of this function will have verified that the Ea file
    exists and that the Vcb field points to an initialized cache file.
    The caller will already have gained exclusive access to the
    EaFcb.

Arguments:

    Vcb - Supplies the Vcb for the volume.

    VirtualEeFile - Pointer to the file object for the virtual Ea file.

    EaFcb - Supplies the pointer to the Fcb for the Ea file.

    EaBcb - Supplies a pointer to the Bcb for the Ea dirent.

    EaDirent - Supplies a pointer to the dirent for the Ea file.

    EaHandle - Supplies the handle for the Ea's to read.

    FileName - Name of the file whose Ea's are being read.

Return Value:

    None.

--*/

{
    ULONG BytesPerCluster = 1 << Vcb->AllocationSupport.LogOfBytesPerCluster;
    ULONG CbList;
    LARGE_INTEGER FileOffset;

    LARGE_MCB DataMcb;
    BOOLEAN UnwindInitializeDataMcb = FALSE;
    BOOLEAN UnwindSplitData = FALSE;

    LARGE_MCB TailMcb;
    BOOLEAN UnwindInitializeTailMcb = FALSE;
    BOOLEAN UnwindSplitTail = FALSE;
    BOOLEAN UnwindMergeTail = FALSE;

    BOOLEAN UnwindModifiedEaHeader = FALSE;
    BOOLEAN UnwindCacheValues = FALSE;
    ULONG UnwindPrevFileSize = 0;

    ULONG EaOffsetVbo;
    USHORT EaOffsetIndex;
    EA_RANGE EaOffsetRange;
    USHORT EaOffsetCluster;

    PFILE_OBJECT VirtualEaFile = Vcb->VirtualEaFile;
    PFCB EaFcb = Vcb->EaFcb;

    EA_RANGE EaHeaderRange;
    PEA_FILE_HEADER EaHeader;
    USHORT EaHeaderBaseIndex;

    ULONG EaSetVbo = 0;
    ULONG EaSetLength;
    EA_RANGE EaSetRange;
    PEA_SET_HEADER EaSet;
    USHORT EaSetClusterCount;

    PAGED_CODE();

    UNREFERENCED_PARAMETER( FileName );

    //
    //  Verify that the Ea index has a legal value.  Raise status
    //  STATUS_INVALID_HANDLE if illegal.
    //

    if (EaHandle < MIN_EA_HANDLE
        || EaHandle > MAX_EA_HANDLE) {

        DebugTrace(-1, Dbg, "FatDeleteEaSet: Illegal handle value\n", 0);
        FatRaiseStatus( IrpContext, STATUS_NONEXISTENT_EA_ENTRY );
    }

    //
    //  Verify that the virtual Ea file is large enough for us to read
    //  the EaOffet table for this index.
    //

    EaOffsetVbo = sizeof( EA_FILE_HEADER ) + (((ULONGLONG)EaHandle >> 7) << 8);

    //
    //  Zero the Ea range structures.
    //

    RtlZeroMemory( &EaHeaderRange, sizeof( EA_RANGE ));
    RtlZeroMemory( &EaOffsetRange, sizeof( EA_RANGE ));
    RtlZeroMemory( &EaSetRange, sizeof( EA_RANGE ));

    //
    //  Use a try to facilitate cleanup.
    //

    _SEH2_TRY {

        //
        //  Pin down the EA file header.
        //

        FatPinEaRange( IrpContext,
                       VirtualEaFile,
                       EaFcb,
                       &EaHeaderRange,
                       0,
                       sizeof( EA_FILE_HEADER ),
                       STATUS_NONEXISTENT_EA_ENTRY );

        EaHeader = (PEA_FILE_HEADER) EaHeaderRange.Data;

        //
        //  Pin down the Ea offset table for the particular index.
        //

        FatPinEaRange( IrpContext,
                       VirtualEaFile,
                       EaFcb,
                       &EaOffsetRange,
                       EaOffsetVbo,
                       sizeof( EA_OFF_TABLE ),
                       STATUS_NONEXISTENT_EA_ENTRY );

        //
        //  Check if the specifific handle is currently being used.
        //

        EaOffsetIndex = EaHandle & (MAX_EA_OFFSET_INDEX - 1);
        EaOffsetCluster = *((PUSHORT) EaOffsetRange.Data + EaOffsetIndex);

        if (EaOffsetCluster == UNUSED_EA_HANDLE) {

            DebugTrace(0, Dbg, "FatReadEaSet: Ea handle is unused\n", 0);
            FatRaiseStatus( IrpContext, STATUS_NONEXISTENT_EA_ENTRY );
        }

        //
        //  Compute the file offset for the Ea data.
        //

        EaHeaderBaseIndex = EaHandle >> 7;
        EaSetVbo = (EaHeader->EaBaseTable[EaHeaderBaseIndex] + EaOffsetCluster)
                   << Vcb->AllocationSupport.LogOfBytesPerCluster;

        //
        //  Unpin the file header and offset table.
        //

        FatUnpinEaRange( IrpContext, &EaHeaderRange );
        FatUnpinEaRange( IrpContext, &EaOffsetRange );

        //
        //  Try to pin the requested Ea set.
        //

        FatPinEaRange( IrpContext,
                       VirtualEaFile,
                       EaFcb,
                       &EaSetRange,
                       EaSetVbo,
                       BytesPerCluster,
                       STATUS_DATA_ERROR );

        EaSet = (PEA_SET_HEADER) EaSetRange.Data;

        if (EaSet->Signature != EA_SET_SIGNATURE
            || EaSet->OwnEaHandle != EaHandle ) {

            DebugTrace(0, Dbg, "FatReadEaSet: Ea set header is corrupt\n", 0);
            FatRaiseStatus( IrpContext, STATUS_DATA_ERROR );
        }

        //
        //  At this point we have pinned a single cluster of Ea data.  If
        //  this represents the entire Ea data for the Ea index, we know
        //  the number of clusters to remove.  Otherwise we need to check
        //  on the entire size of the Ea set header and whether it is
        //  contained in the allocated size of the Ea virtual file.  At
        //  that point we unpin the partial Ea set header and remember the
        //  starting cluster offset and number of clusters in both cluster
        //  and Vbo formats.
        //
        //  At that point the following variables have the described
        //  values.
        //
        //      EaSetVbo - Vbo to start splice at.
        //      EaSetLength - Number of bytes to splice.
        //      EaSetClusterCount - Number of clusters to splice.
        //

        CbList = GetcbList( EaSet );

        EaSetClusterCount = (USHORT) ((CbList + EA_CBLIST_OFFSET + BytesPerCluster - 1)
                                      >> Vcb->AllocationSupport.LogOfBytesPerCluster);

        EaSetLength = EaSetClusterCount << Vcb->AllocationSupport.LogOfBytesPerCluster;

        if (EaSetLength > BytesPerCluster) {

            if (EaFcb->Header.FileSize.LowPart - EaSetVbo < EaSetLength) {

                DebugTrace(0, Dbg, "FatDeleteEaSet: Full Ea set not contained in file\n", 0);

                FatRaiseStatus( IrpContext, STATUS_DATA_ERROR );
            }
        }

        FatUnpinEaRange( IrpContext, &EaSetRange );

        //
        //  Update the cache manager for this file.  This is done by
        //  truncating to the point where the data was spliced and
        //  reinitializing with the modified size of the file.
        //
        //  NOTE: Even if the all the EA's are removed the Ea file will
        //  always exist and the header area will never shrink.
        //

        FileOffset.LowPart = EaSetVbo;
        FileOffset.HighPart = 0;

        //
        //  Round the cache map down to a system page boundary.
        //

        FileOffset.LowPart &= ~(PAGE_SIZE - 1);

        //
        //  Make sure all the data gets out to the disk.
        //

        {
            IO_STATUS_BLOCK Iosb;
            ULONG PurgeCount = 5;

            while (--PurgeCount) {

                Iosb.Status = STATUS_SUCCESS;

                CcFlushCache( VirtualEaFile->SectionObjectPointer,
                              NULL,
                              0,
                              &Iosb );

                NT_ASSERT( Iosb.Status == STATUS_SUCCESS );

                //
                //  We do not have to worry about a lazy writer firing in parallel
                //  with our CcFlushCache since we have the EaFcb exclusive.  Thus
                //  we know all data is out.
                //

                //
                //  We throw the unwanted pages out of the cache and then
                //  truncate the Ea File for the new size.
                //

                if (CcPurgeCacheSection( VirtualEaFile->SectionObjectPointer,
                                         &FileOffset,
                                         0,
                                         FALSE )) {

                    break;
                }
            }

            if (!PurgeCount) {

                FatRaiseStatus( IrpContext, STATUS_UNABLE_TO_DELETE_SECTION );
            }
        }

        FileOffset.LowPart = EaFcb->Header.FileSize.LowPart - EaSetLength;

        //
        //  Perform the splice operation on the FAT chain.  This is done
        //  by splitting the target clusters out and merging the remaining
        //  clusters around them.  We can ignore the return value from
        //  the merge and splice functions because we are guaranteed
        //  to be able to block.
        //

        {
            FsRtlInitializeLargeMcb( &DataMcb, PagedPool );

            UnwindInitializeDataMcb = TRUE;

            FatSplitAllocation( IrpContext,
                                Vcb,
                                &EaFcb->Mcb,
                                EaSetVbo,
                                &DataMcb );

            UnwindSplitData = TRUE;

            if (EaSetLength + EaSetVbo != EaFcb->Header.FileSize.LowPart) {

                FsRtlInitializeLargeMcb( &TailMcb, PagedPool );

                UnwindInitializeTailMcb = TRUE;

                FatSplitAllocation( IrpContext,
                                    Vcb,
                                    &DataMcb,
                                    EaSetLength,
                                    &TailMcb );

                UnwindSplitTail = TRUE;

                FatMergeAllocation( IrpContext,
                                    Vcb,
                                    &EaFcb->Mcb,
                                    &TailMcb );

                UnwindMergeTail = TRUE;
            }
        }

        //
        //  Update the Fcb for the Ea file
        //

        UnwindPrevFileSize = EaFcb->Header.FileSize.LowPart;

        (VOID)ExAcquireResourceExclusiveLite( EaFcb->Header.PagingIoResource,
                                          TRUE );

        EaFcb->Header.FileSize.LowPart = EaFcb->Header.FileSize.LowPart - EaSetLength;
        EaFcb->Header.AllocationSize = EaFcb->Header.FileSize;


        CcSetFileSizes( VirtualEaFile,
                        (PCC_FILE_SIZES)&EaFcb->Header.AllocationSize );

        ExReleaseResourceLite( EaFcb->Header.PagingIoResource );

        UnwindCacheValues = TRUE;

        EaDirent->FileSize = EaFcb->Header.FileSize.LowPart;

        FatSetDirtyBcb( IrpContext, EaBcb, Vcb, TRUE );

        //
        //  Update the Ea base and offset tables.  For the Ea base table,
        //  all subsequent index values must be decremented by the number
        //  of clusters removed.
        //
        //  For the entries in the relevant Ea offset table, all entries
        //  after this index must also be decreased by the number of
        //  clusters removed.
        //

        //
        //  Pin down the EA file header.
        //

        RtlZeroMemory( &EaHeaderRange,
                       sizeof( EA_RANGE ));

        FatPinEaRange( IrpContext,
                       VirtualEaFile,
                       EaFcb,
                       &EaHeaderRange,
                       0,
                       sizeof( EA_FILE_HEADER ),
                       STATUS_NONEXISTENT_EA_ENTRY );

        EaHeader = (PEA_FILE_HEADER) EaHeaderRange.Data;

        //
        //  Pin down the Ea offset table for the particular index.
        //

        RtlZeroMemory( &EaOffsetRange,
                       sizeof( EA_RANGE ));

        FatPinEaRange( IrpContext,
                       VirtualEaFile,
                       EaFcb,
                       &EaOffsetRange,
                       EaOffsetVbo,
                       sizeof( EA_OFF_TABLE ),
                       STATUS_NONEXISTENT_EA_ENTRY );

        {
            ULONG Count;
            PUSHORT NextEaIndex;

            Count = MAX_EA_BASE_INDEX - EaHeaderBaseIndex - 1;

            NextEaIndex = &EaHeader->EaBaseTable[EaHeaderBaseIndex + 1];

            while (Count--) {

                *(NextEaIndex++) -= EaSetClusterCount;
            }

            FatMarkEaRangeDirty( IrpContext, VirtualEaFile, &EaHeaderRange );

            Count = MAX_EA_OFFSET_INDEX - EaOffsetIndex - 1;
            NextEaIndex = (PUSHORT) EaOffsetRange.Data + EaOffsetIndex;

            *(NextEaIndex++) = UNUSED_EA_HANDLE;

            while (Count--) {

                if (*NextEaIndex != UNUSED_EA_HANDLE) {

                    *NextEaIndex -= EaSetClusterCount;
                }

                NextEaIndex++;
            }

            FatMarkEaRangeDirty( IrpContext, VirtualEaFile, &EaOffsetRange );
        }

        UnwindModifiedEaHeader = TRUE;

        //
        //  Deallocate the ea set removed
        //

        FatDeallocateDiskSpace( IrpContext,
                                Vcb,
                                &DataMcb,
                                FALSE );

    } _SEH2_FINALLY {

        DebugUnwind( FatDeleteEaSet );

        //
        //  Restore file if abnormal termination.
        //
        //  If we have modified the ea file header we ignore this
        //  error.  Otherwise we walk through the state variables.
        //

        if (_SEH2_AbnormalTermination()
            && !UnwindModifiedEaHeader) {

            //
            //  If we modified the Ea dirent or Fcb, recover the previous
            //  values.
            //

            if (UnwindPrevFileSize) {

                EaFcb->Header.FileSize.LowPart = UnwindPrevFileSize;
                EaFcb->Header.AllocationSize.LowPart = UnwindPrevFileSize;
                EaDirent->FileSize = UnwindPrevFileSize;

                if (UnwindCacheValues) {

                    CcSetFileSizes( VirtualEaFile,
                                    (PCC_FILE_SIZES)&EaFcb->Header.AllocationSize );
                }
            }

            //
            //  If we merged the tail with the
            //  ea file header.  We split it out
            //  again.
            //

            if (UnwindMergeTail) {

                FatSplitAllocation( IrpContext,
                                    Vcb,
                                    &EaFcb->Mcb,
                                    EaSetVbo,
                                    &TailMcb );
            }

            //
            //  If we split the tail off we merge the tail back
            //  with the ea data to remove.
            //

            if (UnwindSplitTail) {

                FatMergeAllocation( IrpContext,
                                    Vcb,
                                    &DataMcb,
                                    &TailMcb );
            }

            //
            //  If the ea set has been split out, we merge that
            //  cluster string back in the file.  Otherwise we
            //  simply uninitialize the local Mcb.
            //

            if (UnwindSplitData) {

                FatMergeAllocation( IrpContext,
                                    Vcb,
                                    &EaFcb->Mcb,
                                    &DataMcb );
            }
        }

        //
        //  Unpin any Bcb's still active.
        //

        FatUnpinEaRange( IrpContext, &EaHeaderRange );
        FatUnpinEaRange( IrpContext, &EaOffsetRange );
        FatUnpinEaRange( IrpContext, &EaSetRange );

        //
        //  Uninitialize any initialized Mcbs
        //

        if (UnwindInitializeDataMcb) {

            FsRtlUninitializeLargeMcb( &DataMcb );
        }

        if (UnwindInitializeTailMcb) {

            FsRtlUninitializeLargeMcb( &TailMcb );
        }

        DebugTrace(-1, Dbg, "FatDeleteEaSet -> Exit\n", 0);
    } _SEH2_END;

    return;
}



_Requires_lock_held_(_Global_critical_region_)
VOID
FatAddEaSet (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG EaSetLength,
    IN PBCB EaBcb,
    OUT PDIRENT EaDirent,
    OUT PUSHORT EaHandle,
    OUT PEA_RANGE EaSetRange
    )

/*++

Routine Description:

    This routine will add the necessary clusters to support a new
    Ea set of the given size.  This is done by splicing a chain of
    clusters into the existing Ea file.  An Ea index is assigned to
    this new chain and the Ea base and offset tables are updated to
    include this new handle.  This routine also pins the added
    clusters and returns their address and a Bcb.

    The caller of this function will have verified that the Ea file
    exists and that the Vcb field points to an initialized cache file.
    The caller will already have gained exclusive access to the
    EaFcb.

Arguments:

    Vcb - Supplies the Vcb to fill in.

    EaSetLength - The number of bytes needed to contain the Ea set.  This
        routine will round this up the next cluster size.

    EaBcb - Supplies a pointer to the Bcb for the Ea dirent.

    EaDirent - Supplies a pointer to the dirent for the Ea file.

    EaHandle - Supplies the address to store the ea index generated here.

    EaSetRange - This is the structure that describes new range in the Ea file.

Return Value:

    None.

--*/

{
    ULONG BytesPerCluster = 1 << Vcb->AllocationSupport.LogOfBytesPerCluster;

    EA_RANGE EaHeaderRange;
    USHORT EaHeaderIndex;
    PEA_FILE_HEADER EaHeader;

    EA_RANGE EaOffsetRange;
    ULONG EaNewOffsetVbo = 0;
    USHORT EaOffsetIndex;
    ULONG EaOffsetTableSize;
    PUSHORT EaOffsetTable;

    ULONG EaSetClusterOffset;
    ULONG EaSetVbo = 0;
    USHORT EaSetClusterCount;
    PEA_SET_HEADER EaSet;

    PFILE_OBJECT VirtualEaFile = Vcb->VirtualEaFile;
    PFCB EaFcb = Vcb->EaFcb;

    LARGE_MCB EaSetMcb;
    BOOLEAN UnwindInitializedEaSetMcb = FALSE;
    BOOLEAN UnwindAllocatedNewAllocation = FALSE;
    BOOLEAN UnwindMergedNewEaSet = FALSE;

    LARGE_MCB EaOffsetMcb;
    BOOLEAN UnwindInitializedOffsetMcb = FALSE;
    BOOLEAN UnwindSplitNewAllocation = FALSE;
    BOOLEAN UnwindMergedNewOffset = FALSE;

    LARGE_MCB EaTailMcb;
    BOOLEAN UnwindInitializedTailMcb = FALSE;
    BOOLEAN UnwindSplitTail = FALSE;
    BOOLEAN UnwindMergedTail = FALSE;

    LARGE_MCB EaInitialEaMcb;
    BOOLEAN UnwindInitializedInitialEaMcb = FALSE;
    BOOLEAN UnwindSplitInitialEa = FALSE;
    BOOLEAN UnwindMergedInitialEa = FALSE;

    USHORT NewEaIndex;
    PUSHORT NextEaOffset;

    ULONG NewAllocation;
    LARGE_INTEGER FileOffset;
    ULONG Count;

    ULONG UnwindPrevFileSize = 0;
    BOOLEAN UnwindCacheValues = FALSE;

    BOOLEAN TailExists = FALSE;
    BOOLEAN AddedOffsetTableCluster = FALSE;
    BOOLEAN UnwindPurgeCacheMap = FALSE;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "FatAddEaSet\n", 0);
    DebugTrace( 0, Dbg, "  Vcb         = %p\n", Vcb);
    DebugTrace( 0, Dbg, "  EaSetLength = %ul\n", EaSetLength );

    //
    //  Zero the Ea range structures.
    //

    RtlZeroMemory( &EaHeaderRange, sizeof( EA_RANGE ));
    RtlZeroMemory( &EaOffsetRange, sizeof( EA_RANGE ));

    //
    //  Use a try statement to facilitate cleanup.
    //

    _SEH2_TRY {

        //
        //  Pin down the file header.
        //

        FatPinEaRange( IrpContext,
                       VirtualEaFile,
                       EaFcb,
                       &EaHeaderRange,
                       0,
                       sizeof( EA_FILE_HEADER ),
                       STATUS_DATA_ERROR );

        EaHeader = (PEA_FILE_HEADER) EaHeaderRange.Data;

        //
        //  Compute the size of the offset table.
        //

        EaNewOffsetVbo = EaHeader->EaBaseTable[0] << Vcb->AllocationSupport.LogOfBytesPerCluster;
        EaOffsetTableSize = EaNewOffsetVbo - sizeof( EA_FILE_HEADER );

        //
        //  Pin down the entire offset table.
        //

        FatPinEaRange( IrpContext,
                       VirtualEaFile,
                       EaFcb,
                       &EaOffsetRange,
                       sizeof( EA_FILE_HEADER ),
                       EaOffsetTableSize,
                       STATUS_DATA_ERROR );

        //
        //  We now look for a valid handle out of the existing offset table.
        //  We start at the last entry and walk backwards.  We stop at the
        //  first unused handle which is preceded by a used handle (or handle
        //  1).
        //
        //  As we walk backwards, we need to remember the file offset of the
        //  cluster which will follow the clusters we add.  We initially
        //  remember the end of the file.  If the end of the offset table
        //  consists of a string of used handles, we remember the offset of
        //  the handle prior to the transition from used to unused handles.
        //

        EaSetClusterOffset = EaFcb->Header.FileSize.LowPart
                             >> Vcb->AllocationSupport.LogOfBytesPerCluster;

        NewEaIndex = (USHORT) ((EaOffsetTableSize >> 1) - 1);

        NextEaOffset = (PUSHORT) EaOffsetRange.Data + NewEaIndex;

        //
        //  Walk through the used handles at the end of the offset table.
        //

        if (*NextEaOffset != UNUSED_EA_HANDLE) {

            while (NewEaIndex != 0) {

                if (*(NextEaOffset - 1) == UNUSED_EA_HANDLE) {

                    //
                    //  If the handle is 1, we take no action.  Otherwise
                    //  we save the cluster offset of the current handle
                    //  knowing we will use a previous handle and insert
                    //  a chain of clusters.
                    //

                    if (NewEaIndex != 1) {

                        EaSetClusterOffset = *NextEaOffset
                                             + EaHeader->EaBaseTable[NewEaIndex >> 7];

                        TailExists = TRUE;
                    }

                    NewEaIndex--;
                    NextEaOffset--;

                    break;
                }

                NewEaIndex--;
                NextEaOffset--;
            }
        }

        //
        //  Walk through looking for the first unused handle in a string
        //  of unused handles.
        //

        while (NewEaIndex) {

            if (*(NextEaOffset - 1) != UNUSED_EA_HANDLE) {

                break;
            }

            NextEaOffset--;
            NewEaIndex--;
        }

        //
        //  If the handle is zero, we do a special test to see if handle 1
        //  is available.  Otherwise we will use the first handle of a new
        //  cluster.  A non-zero handle now indicates that a handle was found
        //  in an existing offset table cluster.
        //

        if (NewEaIndex == 0) {

            if (*(NextEaOffset + 1) == UNUSED_EA_HANDLE) {

                NewEaIndex = 1;

            } else {

                NewEaIndex = (USHORT) EaOffsetTableSize >> 1;
                AddedOffsetTableCluster = TRUE;
            }
        }

        //
        //  If the Ea index is outside the legal range then raise an
        //  exception.
        //

        if (NewEaIndex > MAX_EA_HANDLE) {

            DebugTrace(-1, Dbg,
                       "FatAddEaSet: Illegal handle value for new handle\n", 0);

            FatRaiseStatus( IrpContext, STATUS_INSUFFICIENT_RESOURCES );
        }

        //
        //  Compute the base and offset indexes.
        //

        EaHeaderIndex = NewEaIndex >> 7;
        EaOffsetIndex = NewEaIndex & (MAX_EA_OFFSET_INDEX - 1);

        //
        //  Compute the byte offset of the new ea data in the file.
        //

        EaSetVbo = EaSetClusterOffset << Vcb->AllocationSupport.LogOfBytesPerCluster;

        //
        //  Allocate all the required disk space together to insure this
        //  operation is atomic.  We don't want to allocate one block
        //  of disk space and then fail on a second allocation.
        //

        EaSetLength = (EaSetLength + BytesPerCluster - 1)
                      & ~(BytesPerCluster - 1);

        NewAllocation = EaSetLength
                        + (AddedOffsetTableCluster ? BytesPerCluster : 0);

        //
        //  Verify that adding these clusters will not grow the Ea file
        //  beyond its legal value.  The maximum number of clusters is
        //  2^16 since the Ea sets are referenced by a 16 bit cluster
        //  offset value.
        //

        if ((ULONG) ((0x0000FFFF << Vcb->AllocationSupport.LogOfBytesPerCluster)
                     - EaFcb->Header.FileSize.LowPart)
            < NewAllocation) {

            DebugTrace(-1, Dbg,
                       "FatAddEaSet: New Ea file size is too large\n", 0);

            FatRaiseStatus( IrpContext, STATUS_INSUFFICIENT_RESOURCES );
        }

        FsRtlInitializeLargeMcb( &EaSetMcb, PagedPool );

        UnwindInitializedEaSetMcb = TRUE;

        FatAllocateDiskSpace( IrpContext,
                              Vcb,
                              0,
                              &NewAllocation,
                              FALSE,
                              &EaSetMcb );

        UnwindAllocatedNewAllocation = TRUE;

        EaSetClusterCount = (USHORT) (EaSetLength >> Vcb->AllocationSupport.LogOfBytesPerCluster);

        if (AddedOffsetTableCluster) {

            FsRtlInitializeLargeMcb( &EaOffsetMcb, PagedPool );

            UnwindInitializedOffsetMcb = TRUE;

            FatSplitAllocation( IrpContext,
                                Vcb,
                                &EaSetMcb,
                                EaSetLength,
                                &EaOffsetMcb );

            UnwindSplitNewAllocation = TRUE;
        }

        FatUnpinEaRange( IrpContext, &EaHeaderRange );
        FatUnpinEaRange( IrpContext, &EaOffsetRange );

        if (AddedOffsetTableCluster) {

            FileOffset.LowPart = EaNewOffsetVbo;

        } else {

            FileOffset.LowPart = EaSetVbo;
        }

        FileOffset.HighPart = 0;

        //
        //  Round the cache map down to a system page boundary.
        //

        FileOffset.LowPart &= ~(PAGE_SIZE - 1);

        {
            IO_STATUS_BLOCK Iosb;
            ULONG PurgeCount = 5;

            while (--PurgeCount) {

                Iosb.Status = STATUS_SUCCESS;

                CcFlushCache( VirtualEaFile->SectionObjectPointer,
                              NULL,
                              0,
                              &Iosb );

                NT_ASSERT( Iosb.Status == STATUS_SUCCESS );

                //
                //  We do not have to worry about a lazy writer firing in parallel
                //  with our CcFlushCache since we have the EaFcb exclusive.  Thus
                //  we know all data is out.
                //

                //
                //  We throw the unwanted pages out of the cache and then
                //  truncate the Ea File for the new size.
                //
                //

                if (CcPurgeCacheSection( VirtualEaFile->SectionObjectPointer,
                                         &FileOffset,
                                         0,
                                         FALSE )) {

                    break;
                }
            }

            if (!PurgeCount) {

                FatRaiseStatus( IrpContext, STATUS_UNABLE_TO_DELETE_SECTION );
            }
        }

        UnwindPurgeCacheMap = TRUE;

        FileOffset.LowPart = EaFcb->Header.FileSize.LowPart + NewAllocation;

        //
        //  If there is a tail to the file, then we initialize an Mcb
        //  for the file section and split the tail from the file.
        //

        if (TailExists) {

            FsRtlInitializeLargeMcb( &EaTailMcb, PagedPool );

            UnwindInitializedTailMcb = TRUE;

            FatSplitAllocation( IrpContext,
                                Vcb,
                                &EaFcb->Mcb,
                                EaSetVbo,
                                &EaTailMcb );

            UnwindSplitTail = TRUE;
        }

        //
        //  If there is an initial section of ea data, we initialize an
        //  Mcb for that section.
        //

        if (AddedOffsetTableCluster
            && EaSetVbo != EaNewOffsetVbo) {

            FsRtlInitializeLargeMcb( &EaInitialEaMcb, PagedPool );

            UnwindInitializedInitialEaMcb = TRUE;

            FatSplitAllocation( IrpContext,
                                Vcb,
                                &EaFcb->Mcb,
                                EaNewOffsetVbo,
                                &EaInitialEaMcb );

            UnwindSplitInitialEa = TRUE;
        }

        //
        //  We have now split the new file allocation into the new
        //  ea set and possibly a new offset table.
        //
        //  We have also split the existing file data into a file
        //  header, an initial section of ea data and the tail of the
        //  file.  These last 2 may not exist.
        //
        //  Each section is described by an Mcb.
        //

        //
        //  Merge the new offset information if it exists.
        //

        if (AddedOffsetTableCluster) {

            FatMergeAllocation( IrpContext,
                                Vcb,
                                &EaFcb->Mcb,
                                &EaOffsetMcb );

            FsRtlUninitializeLargeMcb( &EaOffsetMcb );
            FsRtlInitializeLargeMcb( &EaOffsetMcb, PagedPool );

            UnwindMergedNewOffset = TRUE;
        }

        //
        //  Merge the existing initial ea data if it exists.
        //

        if (UnwindInitializedInitialEaMcb) {

            FatMergeAllocation( IrpContext,
                                Vcb,
                                &EaFcb->Mcb,
                                &EaInitialEaMcb );

            FsRtlUninitializeLargeMcb( &EaInitialEaMcb );
            FsRtlInitializeLargeMcb( &EaInitialEaMcb, PagedPool );

            UnwindMergedInitialEa = TRUE;
        }

        //
        //  We modify the offset of the new ea set by one cluster if
        //  we added one to the offset table.
        //

        if (AddedOffsetTableCluster) {

            EaSetClusterOffset += 1;
            EaSetVbo += BytesPerCluster;
        }

        //
        //  Merge the new ea set.
        //

        FatMergeAllocation( IrpContext,
                            Vcb,
                            &EaFcb->Mcb,
                            &EaSetMcb );

        FsRtlUninitializeLargeMcb( &EaSetMcb );
        FsRtlInitializeLargeMcb( &EaSetMcb, PagedPool );

        UnwindMergedNewEaSet = TRUE;

        //
        //  Merge the tail if it exists.
        //

        if (UnwindInitializedTailMcb) {

            FatMergeAllocation( IrpContext,
                                Vcb,
                                &EaFcb->Mcb,
                                &EaTailMcb );

            FsRtlUninitializeLargeMcb( &EaTailMcb );
            FsRtlInitializeLargeMcb( &EaTailMcb, PagedPool );

            UnwindMergedTail = TRUE;
        }

        //
        //  If we added a new cluster for the offset table, we need to
        //  lock the entire cluster down and initialize all the handles to
        //  the unused state except the first one.
        //

        //
        //  Update the Fcb information.
        //

        UnwindPrevFileSize = EaFcb->Header.FileSize.LowPart;

        EaFcb->Header.FileSize.LowPart += NewAllocation;
        EaFcb->Header.AllocationSize = EaFcb->Header.FileSize;
        EaDirent->FileSize = EaFcb->Header.FileSize.LowPart;

        FatSetDirtyBcb( IrpContext, EaBcb, Vcb, TRUE );

        //
        //  Let Mm and Cc know the new file sizes.
        //

        CcSetFileSizes( VirtualEaFile,
                        (PCC_FILE_SIZES)&EaFcb->Header.AllocationSize );

        UnwindCacheValues = TRUE;

        //
        //  Pin down the file header.
        //

        RtlZeroMemory( &EaHeaderRange, sizeof( EA_RANGE ));

        FatPinEaRange( IrpContext,
                       VirtualEaFile,
                       EaFcb,
                       &EaHeaderRange,
                       0,
                       sizeof( EA_FILE_HEADER ),
                       STATUS_DATA_ERROR );

        EaHeader = (PEA_FILE_HEADER) EaHeaderRange.Data;

        //
        //  Pin down the entire offset table.
        //


        RtlZeroMemory( &EaOffsetRange, sizeof( EA_RANGE ));

        FatPinEaRange( IrpContext,
                       VirtualEaFile,
                       EaFcb,
                       &EaOffsetRange,
                       sizeof( EA_FILE_HEADER ) + (((ULONGLONG)NewEaIndex >> 7) << 8),
                       sizeof( EA_OFF_TABLE ),
                       STATUS_DATA_ERROR );

        EaOffsetTable = (PUSHORT) EaOffsetRange.Data;

        //
        //  Pin the Ea set header for the added clusters and initialize
        //  the fields of interest.  These are the signature field, the
        //  owning handle field, the need Ea field and the cbList field.
        //  Also mark the data as dirty.
        //

        //
        //  Pin the ea set.
        //

        FatPinEaRange( IrpContext,
                       VirtualEaFile,
                       EaFcb,
                       EaSetRange,
                       EaSetVbo,
                       EaSetLength,
                       STATUS_DATA_ERROR );

        EaSet = (PEA_SET_HEADER) EaSetRange->Data;

        EaSet->Signature = EA_SET_SIGNATURE;
        EaSet->OwnEaHandle = NewEaIndex;

        FatMarkEaRangeDirty( IrpContext, VirtualEaFile, EaSetRange );

        //
        //  Update the Ea base and offset tables.  For the Ea base table,
        //  all subsequent index values must be incremented by the number
        //  of clusters added.
        //
        //  For the entries in the relevant Ea offset table, all entries
        //  after this index must also be increased by the number of
        //  clusters added.
        //
        //  If we added another cluster to the offset table, then we increment
        //  all the base table values by 1.
        //

        Count = MAX_EA_BASE_INDEX - EaHeaderIndex - 1;

        NextEaOffset = &EaHeader->EaBaseTable[EaHeaderIndex + 1];

        while (Count--) {

            *(NextEaOffset++) += EaSetClusterCount;
        }

        if (AddedOffsetTableCluster) {

            Count = MAX_EA_BASE_INDEX;

            NextEaOffset = &EaHeader->EaBaseTable[0];

            while (Count--) {

                *(NextEaOffset++) += 1;
            }
        }

        FatMarkEaRangeDirty( IrpContext, VirtualEaFile, &EaHeaderRange );

        //
        //  If we added an offset table cluster, we need to initialize
        //  the handles to unused.
        //

        if (AddedOffsetTableCluster) {

            Count = (BytesPerCluster >> 1) - 1;
            NextEaOffset = EaOffsetTable;

            *NextEaOffset++ = 0;

            while (Count--) {

                *NextEaOffset++ = UNUSED_EA_HANDLE;
            }
        }

        //
        //  We need to compute the offset of the added Ea set clusters
        //  from their base.
        //

        NextEaOffset = EaOffsetTable + EaOffsetIndex;

        *NextEaOffset++ = (USHORT) (EaSetClusterOffset
                                    - EaHeader->EaBaseTable[EaHeaderIndex]);

        Count = MAX_EA_OFFSET_INDEX - EaOffsetIndex - 1;

        while (Count--) {

            if (*NextEaOffset != UNUSED_EA_HANDLE) {

                *NextEaOffset += EaSetClusterCount;
            }

            NextEaOffset++;
        }

        FatMarkEaRangeDirty( IrpContext, VirtualEaFile, &EaOffsetRange );

        //
        //  Update the callers parameters.
        //

        *EaHandle = NewEaIndex;

        DebugTrace(0, Dbg, "FatAddEaSet: Return values\n", 0);

        DebugTrace(0, Dbg, "FatAddEaSet: New Handle -> %x\n",
                   *EaHandle);

    } _SEH2_FINALLY {

        DebugUnwind( FatAddEaSet );

        //
        //  Handle cleanup for abnormal termination only if we allocated
        //  disk space for the new ea set.
        //

        if (_SEH2_AbnormalTermination() && UnwindAllocatedNewAllocation) {

            //
            //  If we modified the Ea dirent or Fcb, recover the previous
            //  values.  Even though we are decreasing FileSize here, we
            //  don't need to synchronize to synchronize with paging Io
            //  because there was no dirty data generated in the new allocation.
            //

            if (UnwindPrevFileSize) {

                EaFcb->Header.FileSize.LowPart = UnwindPrevFileSize;
                EaFcb->Header.AllocationSize.LowPart = UnwindPrevFileSize;
                EaDirent->FileSize = UnwindPrevFileSize;

                if (UnwindCacheValues) {

                    CcSetFileSizes( VirtualEaFile,
                                    (PCC_FILE_SIZES)&EaFcb->Header.AllocationSize );
                }
            }

            //
            //  If we merged the tail then split it off.
            //

            if (UnwindMergedTail) {

                VBO NewTailPosition;

                NewTailPosition = EaSetVbo + EaSetLength;

                FatSplitAllocation( IrpContext,
                                    Vcb,
                                    &EaFcb->Mcb,
                                    NewTailPosition,
                                    &EaTailMcb );
            }

            //
            //  If we merged the new ea data then split it out.
            //

            if (UnwindMergedNewEaSet) {

                FatSplitAllocation( IrpContext,
                                    Vcb,
                                    &EaFcb->Mcb,
                                    EaSetVbo,
                                    &EaSetMcb );
            }

            //
            //  If we merged the initial ea data then split it out.
            //

            if (UnwindMergedInitialEa) {

                FatSplitAllocation( IrpContext,
                                    Vcb,
                                    &EaFcb->Mcb,
                                    EaNewOffsetVbo + BytesPerCluster,
                                    &EaInitialEaMcb );
            }

            //
            //  If we added a new offset cluster, then split it out.
            //

            if (UnwindMergedNewOffset) {

                FatSplitAllocation( IrpContext,
                                    Vcb,
                                    &EaFcb->Mcb,
                                    EaNewOffsetVbo,
                                    &EaOffsetMcb );
            }

            //
            //  If there is an initial ea section prior to the new section, merge
            //  it with the rest of the file.
            //

            if (UnwindSplitInitialEa) {

                FatMergeAllocation( IrpContext, Vcb, &EaFcb->Mcb, &EaInitialEaMcb );
            }

            //
            //  If there is a file tail split off, merge it with the
            //  rest of the file.
            //

            if (UnwindSplitTail) {

                FatMergeAllocation( IrpContext, Vcb, &EaFcb->Mcb, &EaTailMcb );
            }

            //
            //  If we modified the cache initialization for the ea file,
            //  then throw away the ea file object.
            //

            if (UnwindPurgeCacheMap) {

                Vcb->VirtualEaFile = NULL;
                ObDereferenceObject( VirtualEaFile );
            }

            //
            //  If we split the allocation, then deallocate the block for
            //  the new offset information.
            //

            if (UnwindSplitNewAllocation) {

                FatDeallocateDiskSpace( IrpContext, Vcb, &EaOffsetMcb, FALSE );
            }

            //
            //  Deallocate the disk space.
            //

            FatDeallocateDiskSpace( IrpContext, Vcb, &EaSetMcb, FALSE );
        }

        //
        //  Unpin the Ea ranges.
        //

        FatUnpinEaRange( IrpContext, &EaHeaderRange );
        FatUnpinEaRange( IrpContext, &EaOffsetRange );

        //
        //  Uninitialize any local Mcbs
        //

        if (UnwindInitializedEaSetMcb) {

            FsRtlUninitializeLargeMcb( &EaSetMcb );
        }

        if (UnwindInitializedOffsetMcb) {

            FsRtlUninitializeLargeMcb( &EaOffsetMcb );
        }

        if (UnwindInitializedTailMcb) {

            FsRtlUninitializeLargeMcb( &EaTailMcb );
        }

        if (UnwindInitializedInitialEaMcb) {

            FsRtlUninitializeLargeMcb( &EaInitialEaMcb );
        }

        DebugTrace(-1, Dbg, "FatAddEaSet ->  Exit\n", 0);
    } _SEH2_END;

    return;
}


VOID
FatAppendPackedEa (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PEA_SET_HEADER *EaSetHeader,
    IN OUT PULONG PackedEasLength,
    IN OUT PULONG AllocationLength,
    IN PFILE_FULL_EA_INFORMATION FullEa,
    IN ULONG BytesPerCluster
    )

/*++

Routine Description:

    This routine appends a new packed ea onto an existing packed ea list,
    it also will allocate/dealloate pool as necessary to hold the ea list.

Arguments:

    EaSetHeader - Supplies the address to store the pointer to pool memory
                  which contains the Ea list for a file.

    PackedEasLength - Supplies the length of the actual Ea data.  The
                      new Ea data will be appended at this point.

    AllocationLength - Supplies the allocated length available for Ea
                       data.

    FullEa - Supplies a pointer to the new full ea that is to be appended
             (in packed form) to the packed ea list.

    BytesPerCluster - Number of bytes per cluster on this volume.

    NOTE: The EaSetHeader refers to the entire block of Ea data for a
          file.  This includes the Ea's and their values as well as the
          header information.  The PackedEasLength and AllocationLength
          parameters refer to the name/value pairs only.

Return Value:

    None.

--*/

{
    ULONG PackedEaSize;
    PPACKED_EA ThisPackedEa;
    OEM_STRING EaName;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "FatAppendPackedEa...\n", 0);

    //
    //  As a quick check see if the computed packed ea size plus the
    //  current packed ea list size will overflow the buffer.  Full Ea and
    //  packed Ea only differ by 4 in their size
    //

    PackedEaSize = SizeOfFullEa( FullEa ) - 4;

    if ( PackedEaSize + *PackedEasLength > *AllocationLength ) {

        //
        //  We will overflow our current work buffer so allocate a larger
        //  one and copy over the current buffer
        //

        PVOID Temp;
        ULONG NewAllocationSize;
        ULONG OldAllocationSize;

        DebugTrace(0, Dbg, "Allocate a new ea list buffer\n", 0);

        //
        //  Compute a new size and allocate space.  Always increase the
        //  allocation in cluster increments.
        //

        NewAllocationSize = (SIZE_OF_EA_SET_HEADER
                             + PackedEaSize
                             + *PackedEasLength
                             + BytesPerCluster - 1)
                            & ~(BytesPerCluster - 1);

        Temp = FsRtlAllocatePoolWithTag( PagedPool,
                                         NewAllocationSize,
                                         TAG_EA_SET_HEADER );

        //
        //  Move over the existing ea list, and deallocate the old one
        //

        RtlCopyMemory( Temp,
                       *EaSetHeader,
                       OldAllocationSize = *AllocationLength
                                           + SIZE_OF_EA_SET_HEADER );

        ExFreePool( *EaSetHeader );

        //
        //  Set up so we will use the new packed ea list
        //

        *EaSetHeader = Temp;

        //
        //  Zero out the added memory.
        //

        RtlZeroMemory( &(*EaSetHeader)->PackedEas[*AllocationLength],
                       NewAllocationSize - OldAllocationSize );

        *AllocationLength = NewAllocationSize - SIZE_OF_EA_SET_HEADER;
    }

    //
    //  Determine if we need to increment our need ea changes count
    //

    if ( FlagOn(FullEa->Flags, FILE_NEED_EA )) {

        //
        //  The NeedEaCount field is long aligned so we will write
        //  directly to it.
        //

        (*EaSetHeader)->NeedEaCount++;
    }

    //
    //  Now copy over the ea, full ea's and packed ea are identical except
    //  that full ea also have a next ea offset that we skip over
    //
    //  Before:
    //             UsedSize                     Allocated
    //                |                             |
    //                V                             V
    //      +xxxxxxxx+-----------------------------+
    //
    //  After:
    //                              UsedSize    Allocated
    //                                 |            |
    //                                 V            V
    //      +xxxxxxxx+yyyyyyyyyyyyyyyy+------------+
    //

    ThisPackedEa = (PPACKED_EA) (RtlOffsetToPointer( (*EaSetHeader)->PackedEas,
                                                     *PackedEasLength ));

    RtlCopyMemory( ThisPackedEa,
                   (PUCHAR) FullEa + 4,
                   PackedEaSize );

    //
    //  Now convert the name to uppercase.
    //

    EaName.MaximumLength = EaName.Length = FullEa->EaNameLength;
    EaName.Buffer = ThisPackedEa->EaName;

    FatUpcaseEaName( IrpContext, &EaName, &EaName );

    //
    //  Increment the used size in the packed ea list structure
    //

    *PackedEasLength += PackedEaSize;

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "FatAppendPackedEa -> VOID\n", 0);

    UNREFERENCED_PARAMETER( IrpContext );

    return;
}


VOID
FatDeletePackedEa (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PEA_SET_HEADER EaSetHeader,
    IN OUT PULONG PackedEasLength,
    IN ULONG Offset
    )

/*++

Routine Description:

    This routine deletes an individual packed ea from the supplied
    packed ea list.

Arguments:

    EaSetHeader - Supplies the address to store the pointer to pool memory
                  which contains the Ea list for a file.

    PackedEasLength - Supplies the length of the actual Ea data.  The
                      new Ea data will be appended at this point.

    Offset - Supplies the offset to the individual ea in the list to delete

    NOTE: The EaSetHeader refers to the entire block of Ea data for a
          file.  This includes the Ea's and their values as well as the
          header information.  The PackedEasLength parameter refer to the
          name/value pairs only.

Return Value:

    None.

--*/

{
    PPACKED_EA PackedEa;
    ULONG PackedEaSize;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "FatDeletePackedEa, Offset = %08lx\n", Offset);

    //
    //  Get a reference to the packed ea and figure out its size
    //

    PackedEa = (PPACKED_EA) (&EaSetHeader->PackedEas[Offset]);

    SizeOfPackedEa( PackedEa, &PackedEaSize );

    //
    //  Determine if we need to decrement our need ea changes count
    //

    if (FlagOn(PackedEa->Flags, EA_NEED_EA_FLAG)) {

        EaSetHeader->NeedEaCount--;
    }

    //
    //  Shrink the ea list over the deleted ea.  The amount to copy is the
    //  total size of the ea list minus the offset to the end of the ea
    //  we're deleting.
    //
    //  Before:
    //              Offset    Offset+PackedEaSize      UsedSize    Allocated
    //                |                |                  |            |
    //                V                V                  V            V
    //      +xxxxxxxx+yyyyyyyyyyyyyyyy+zzzzzzzzzzzzzzzzzz+------------+
    //
    //  After
    //              Offset            UsedSize                     Allocated
    //                |                  |                             |
    //                V                  V                             V
    //      +xxxxxxxx+zzzzzzzzzzzzzzzzzz+-----------------------------+
    //

    RtlCopyMemory( PackedEa,
                   (PUCHAR) PackedEa + PackedEaSize,
                   *PackedEasLength - (Offset + PackedEaSize) );

    //
    //  And zero out the remaing part of the ea list, to make things
    //  nice and more robust
    //

    RtlZeroMemory( &EaSetHeader->PackedEas[*PackedEasLength - PackedEaSize],
                   PackedEaSize );

    //
    //  Decrement the used size by the amount we just removed
    //

    *PackedEasLength -= PackedEaSize;

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "FatDeletePackedEa -> VOID\n", 0);

    UNREFERENCED_PARAMETER( IrpContext );

    return;
}


ULONG
FatLocateNextEa (
    IN PIRP_CONTEXT IrpContext,
    IN PPACKED_EA FirstPackedEa,
    IN ULONG PackedEasLength,
    IN ULONG PreviousOffset
    )

/*++

Routine Description:

    This routine locates the offset for the next individual packed ea
    inside of a packed ea list, given the offset to a previous Ea.
    Instead of returing boolean to indicate if we've found the next one
    we let the return offset be so large that it overuns the used size
    of the packed ea list, and that way it's an easy construct to use
    in a for loop.

Arguments:

    FirstPackedEa - Supplies a pointer to the packed ea list structure

    PackedEasLength - Supplies the length of the packed ea list

    PreviousOffset - Supplies the offset to a individual packed ea in the
        list

Return Value:

    ULONG - The offset to the next ea in the list or 0xffffffff of one
        does not exist.

--*/

{
    PPACKED_EA PackedEa;
    ULONG PackedEaSize;
    ULONG Offset;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "FatLocateNextEa, PreviousOffset = %08lx\n",
               PreviousOffset);

    //
    //  Make sure the previous offset is within the used size range
    //

    if ( PreviousOffset >= PackedEasLength ) {

        DebugTrace(-1, Dbg, "FatLocateNextEa -> 0xffffffff\n", 0);
        return 0xffffffff;
    }

    //
    //  Get a reference to the previous packed ea, and compute its size
    //

    PackedEa = (PPACKED_EA) ((PUCHAR) FirstPackedEa + PreviousOffset );
    SizeOfPackedEa( PackedEa, &PackedEaSize );

    //
    //  Compute to the next ea
    //

    Offset = PreviousOffset + PackedEaSize;

    //
    //  Now, if the new offset is beyond the ea size then we know
    //  that there isn't one so, we return an offset of 0xffffffff.
    //  otherwise we'll leave the new offset alone.
    //

    if ( Offset >= PackedEasLength ) {

        Offset = 0xffffffff;
    }

    DebugTrace(-1, Dbg, "FatLocateNextEa -> %08lx\n", Offset);

    UNREFERENCED_PARAMETER( IrpContext );

    return Offset;
}


BOOLEAN
FatLocateEaByName (
    IN PIRP_CONTEXT IrpContext,
    IN PPACKED_EA FirstPackedEa,
    IN ULONG PackedEasLength,
    IN POEM_STRING EaName,
    OUT PULONG Offset
    )

/*++

Routine Description:

    This routine locates the offset for the next individual packed ea
    inside of a packed ea list, given the name of the ea to locate

Arguments:

    FirstPackedEa - Supplies a pointer to the packed ea list structure

    PackedEasLength - Supplies the length of the packed ea list

    EaName - Supplies the name of the ea search for

    Offset - Receives the offset to the located individual ea in the list
        if one exists.

Return Value:

    BOOLEAN - TRUE if the named packed ea exists in the list and FALSE
        otherwise.

--*/

{
    PPACKED_EA PackedEa;
    OEM_STRING Name;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "FatLocateEaByName, EaName = %Z\n", EaName);

    //
    //  For each packed ea in the list check its name against the
    //  ea name we're searching for
    //

    for ( *Offset = 0;
          *Offset < PackedEasLength;
          *Offset = FatLocateNextEa( IrpContext,
                                     FirstPackedEa,
                                     PackedEasLength,
                                     *Offset )) {

        //
        //  Reference the packed ea and get a string to its name
        //

        PackedEa = (PPACKED_EA) ((PUCHAR) FirstPackedEa + *Offset);

        Name.Buffer = &PackedEa->EaName[0];
        Name.Length = PackedEa->EaNameLength;
        Name.MaximumLength = PackedEa->EaNameLength;

        //
        //  Compare the two strings, if they are equal then we've
        //  found the caller's ea
        //

        if ( RtlCompareString( EaName, &Name, TRUE ) == 0 ) {

            DebugTrace(-1, Dbg, "FatLocateEaByName -> TRUE, *Offset = %08lx\n", *Offset);
            return TRUE;
        }
    }

    //
    //  We've exhausted the ea list without finding a match so return false
    //

    DebugTrace(-1, Dbg, "FatLocateEaByName -> FALSE\n", 0);
    return FALSE;
}


BOOLEAN
FatIsEaNameValid (
    IN PIRP_CONTEXT IrpContext,
    IN OEM_STRING Name
    )

/*++

Routine Description:

    This routine simple returns whether the specified file names conforms
    to the file system specific rules for legal Ea names.

    For Ea names, the following rules apply:

    A. An Ea name may not contain any of the following characters:

       0x0000 - 0x001F  \ / : * ? " < > | , + = [ ] ;

Arguments:

    Name - Supllies the name to check.

Return Value:

    BOOLEAN - TRUE if the name is legal, FALSE otherwise.

--*/

{
    ULONG Index;

    UCHAR Char;

    PAGED_CODE();

    UNREFERENCED_PARAMETER( IrpContext );

    //
    //  Empty names are not valid.
    //

    if ( Name.Length == 0 ) { return FALSE; }

    //
    //  At this point we should only have a single name, which can't have
    //  more than 254 characters
    //

    if ( Name.Length > 254 ) { return FALSE; }

    for ( Index = 0; Index < (ULONG)Name.Length; Index += 1 ) {

        Char = Name.Buffer[ Index ];

        //
        //  Skip over and Dbcs chacters
        //

        if ( FsRtlIsLeadDbcsCharacter( Char ) ) {

            NT_ASSERT( Index != (ULONG)(Name.Length - 1) );

            Index += 1;

            continue;
        }

        //
        //  Make sure this character is legal, and if a wild card, that
        //  wild cards are permissible.
        //

        if ( !FsRtlIsAnsiCharacterLegalFat(Char, FALSE) ) {

            return FALSE;
        }
    }

    return TRUE;
}


VOID
FatPinEaRange (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT VirtualEaFile,
    IN PFCB EaFcb,
    IN OUT PEA_RANGE EaRange,
    IN ULONG StartingVbo,
    IN ULONG Length,
    IN NTSTATUS ErrorStatus
    )

/*++

Routine Description:

    This routine is called to pin a range within the Ea file.  It will follow all the
    rules required by the cache manager so that we don't have overlapping pin operations.
    If the range being pinned spans a section then the desired data will be copied into
    an auxilary buffer.  FatMarkEaRangeDirty will know whether to copy the data back
    into the cache or whether to simply mark the pinned data dirty.

Arguments:

    VirtualEaFile - This is the stream file for the Ea file.

    EaFcb - This is the Fcb for the Ea file.

    EaRange - This is the Ea range structure for this request.

    StartingVbo - This is the starting offset in the Ea file to read from.

    Length - This is the length of the read.

    ErrorStatus - This is the error status to use if we are reading outside
        of the file.

Return Value:

    None.

--*/

{
    LARGE_INTEGER LargeVbo;
    ULONG ByteCount;
    PBCB *NextBcb;
    PVOID Buffer;
    PCHAR DestinationBuffer = NULL;
    BOOLEAN FirstPage = TRUE;

    PAGED_CODE();

    //
    //  Verify that the entire read is contained within the Ea file.
    //

    if (Length == 0
        || StartingVbo >= EaFcb->Header.AllocationSize.LowPart
        || (EaFcb->Header.AllocationSize.LowPart - StartingVbo) < Length) {

        FatRaiseStatus( IrpContext, ErrorStatus );
    }

    //
    //  If the read will span a section, the system addresses may not be contiguous.
    //  Allocate a separate buffer in this case.
    //

    if (((StartingVbo & (EA_SECTION_SIZE - 1)) + Length) > EA_SECTION_SIZE) {

        EaRange->Data = FsRtlAllocatePoolWithTag( PagedPool,
                                                  Length,
                                                  TAG_EA_DATA );
        EaRange->AuxilaryBuffer = TRUE;

        DestinationBuffer = EaRange->Data;

    } else {

        //
        //  PREfix correctly notes that if we don't decide here to have an aux buffer
        //  and the flag is up in the EaRange, we'll party on random memory since
        //  DestinationBuffer won't be set; however, this will never happen due to
        //  initialization of ea ranges and the cleanup in UnpinEaRange.
        //

        NT_ASSERT( EaRange->AuxilaryBuffer == FALSE );
    }


    //
    //  If the read will require more pages than our structure will hold then
    //  allocate an auxilary buffer.  We have to figure the number of pages
    //  being requested so we have to include the page offset of the first page of
    //  the request.
    //

    EaRange->BcbChainLength = (USHORT) (((StartingVbo & (PAGE_SIZE - 1)) + Length + PAGE_SIZE - 1) / PAGE_SIZE);

    if (EaRange->BcbChainLength > EA_BCB_ARRAY_SIZE) {

        EaRange->BcbChain = FsRtlAllocatePoolWithTag( PagedPool,
                                                      sizeof( PBCB ) * EaRange->BcbChainLength,
                                                      TAG_BCB );

        RtlZeroMemory( EaRange->BcbChain, sizeof( PBCB ) * EaRange->BcbChainLength );

    } else {

        EaRange->BcbChain = (PBCB *) &EaRange->BcbArray;
    }

    //
    //  Store the byte range data in the Ea Range structure.
    //

    EaRange->StartingVbo = StartingVbo;
    EaRange->Length = Length;

    //
    //  Compute the initial pin length.
    //

    ByteCount = PAGE_SIZE - (StartingVbo & (PAGE_SIZE - 1));

    //
    //  For each page in the range; pin the page and update the Bcb count, copy to
    //  the auxiliary buffer.
    //

    NextBcb = EaRange->BcbChain;

    while (Length != 0) {

        //
        //  Pin the page and remember the data start.
        //

        LargeVbo.QuadPart = StartingVbo;

        if (ByteCount > Length) {

            ByteCount = Length;
        }

        if (!CcPinRead( VirtualEaFile,
                        &LargeVbo,
                        ByteCount,
                        BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT),
                        NextBcb,
                        &Buffer )) {

            //
            // Could not read the data without waiting (cache miss).
            //

            FatRaiseStatus( IrpContext, STATUS_CANT_WAIT );
        }

        //
        //  Increment the Bcb pointer and copy to the auxilary buffer if necessary.
        //

        NextBcb += 1;

        if (EaRange->AuxilaryBuffer == TRUE) {

            RtlCopyMemory( DestinationBuffer,
                           Buffer,
                           ByteCount );

            DestinationBuffer = (PCHAR) Add2Ptr( DestinationBuffer, ByteCount );
        }

        StartingVbo += ByteCount;
        Length -= ByteCount;

        //
        //  If this is the first page then update the Ea Range structure.
        //

        if (FirstPage) {

            FirstPage = FALSE;
            ByteCount = PAGE_SIZE;

            if (EaRange->AuxilaryBuffer == FALSE) {

                EaRange->Data = Buffer;
            }
        }
    }

    return;
}


VOID
FatMarkEaRangeDirty (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT EaFileObject,
    IN OUT PEA_RANGE EaRange
    )

/*++

Routine Description:

    This routine is called to mark a range of the Ea file as dirty.  If the modified
    data is sitting in an auxilary buffer then we will copy it back into the cache.
    In any case we will go through the list of Bcb's and mark them dirty.

Arguments:

    EaFileObject - This is the file object for the Ea file.

    EaRange - This is the Ea range structure for this request.

Return Value:

    None.

--*/

{
    PBCB *NextBcb;
    ULONG BcbCount;

    PAGED_CODE();

    UNREFERENCED_PARAMETER( IrpContext );

    //
    //  If there is an auxilary buffer we need to copy the data back into the cache.
    //

    if (EaRange->AuxilaryBuffer == TRUE) {

        LARGE_INTEGER LargeVbo;

        LargeVbo.QuadPart = EaRange->StartingVbo;

        CcCopyWrite( EaFileObject,
                     &LargeVbo,
                     EaRange->Length,
                     TRUE,
                     EaRange->Data );
    }

    //
    //  Now walk through the Bcb chain and mark everything dirty.
    //

    BcbCount = EaRange->BcbChainLength;
    NextBcb = EaRange->BcbChain;

    while (BcbCount--) {

        if (*NextBcb != NULL) {

            CcSetDirtyPinnedData( *NextBcb, NULL );
        }

        NextBcb += 1;
    }

    return;
}


VOID
FatUnpinEaRange (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PEA_RANGE EaRange
    )

/*++

Routine Description:

    This routine is called to unpin a range in the Ea file.  Any structures allocated
    will be deallocated here.

Arguments:

    EaRange - This is the Ea range structure for this request.

Return Value:

    None.

--*/

{
    PBCB *NextBcb;
    ULONG BcbCount;

    PAGED_CODE();

    UNREFERENCED_PARAMETER( IrpContext );

    //
    //  If we allocated a auxilary buffer, deallocate it here.
    //

    if (EaRange->AuxilaryBuffer == TRUE) {

        ExFreePool( EaRange->Data );
        EaRange->AuxilaryBuffer = FALSE;
    }

    //
    //  Walk through the Bcb chain and unpin the data.
    //

    if (EaRange->BcbChain != NULL) {

        BcbCount = EaRange->BcbChainLength;
        NextBcb = EaRange->BcbChain;

        while (BcbCount--) {

            if (*NextBcb != NULL) {

                CcUnpinData( *NextBcb );
                *NextBcb = NULL;
            }

            NextBcb += 1;
        }

        //
        //  If we allocated a Bcb chain, deallocate it here.
        //

        if (EaRange->BcbChain != &EaRange->BcbArray[0]) {

            ExFreePool( EaRange->BcbChain );
        }

        EaRange->BcbChain = NULL;
    }

    return;
}

