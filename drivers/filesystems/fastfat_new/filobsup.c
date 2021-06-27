/*++

Copyright (c) 1989-2000 Microsoft Corporation

Module Name:

    FilObSup.c

Abstract:

    This module implements the Fat File object support routines.


--*/

#include "fatprocs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (FAT_BUG_CHECK_FILOBSUP)

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_FILOBSUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FatForceCacheMiss)
#pragma alloc_text(PAGE, FatPurgeReferencedFileObjects)
#pragma alloc_text(PAGE, FatSetFileObject)
#pragma alloc_text(PAGE, FatDecodeFileObject)
#endif


VOID
FatSetFileObject (
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN TYPE_OF_OPEN TypeOfOpen,
    IN PVOID VcbOrFcbOrDcb,
    IN PCCB Ccb OPTIONAL
    )

/*++

Routine Description:

    This routine sets the file system pointers within the file object

Arguments:

    FileObject - Supplies a pointer to the file object being modified, and
        can optionally be null.

    TypeOfOpen - Supplies the type of open denoted by the file object.
        This is only used by this procedure for sanity checking.

    VcbOrFcbOrDcb - Supplies a pointer to either a vcb, fcb, or dcb

    Ccb - Optionally supplies a pointer to a ccb

Return Value:

    None.

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "FatSetFileObject, FileObject = %p\n", FileObject );

    NT_ASSERT((Ccb == NULL) || (NodeType(Ccb) == FAT_NTC_CCB));


    NT_ASSERT(((TypeOfOpen == UnopenedFileObject))

                ||

           ((TypeOfOpen == UserFileOpen) &&
            (NodeType(VcbOrFcbOrDcb) == FAT_NTC_FCB) &&
            (Ccb != NULL))

                ||

           ((TypeOfOpen == EaFile) &&
            (NodeType(VcbOrFcbOrDcb) == FAT_NTC_FCB) &&
            (Ccb == NULL))

                ||

           ((TypeOfOpen == UserDirectoryOpen) &&
            ((NodeType(VcbOrFcbOrDcb) == FAT_NTC_DCB) || (NodeType(VcbOrFcbOrDcb) == FAT_NTC_ROOT_DCB)) &&
            (Ccb != NULL))

                ||

           ((TypeOfOpen == UserVolumeOpen) &&
            (NodeType(VcbOrFcbOrDcb) == FAT_NTC_VCB) &&
            (Ccb != NULL))

                ||

           ((TypeOfOpen == VirtualVolumeFile) &&
            (NodeType(VcbOrFcbOrDcb) == FAT_NTC_VCB) &&
            (Ccb == NULL))

                ||

           ((TypeOfOpen == DirectoryFile) &&
            ((NodeType(VcbOrFcbOrDcb) == FAT_NTC_DCB) || (NodeType(VcbOrFcbOrDcb) == FAT_NTC_ROOT_DCB)) &&
            (Ccb == NULL))
            );


    UNREFERENCED_PARAMETER( TypeOfOpen );

    //
    //  If we were given an Fcb, Dcb, or Vcb, we have some processing to do.
    //

    NT_ASSERT((Ccb == NULL) || (NodeType(Ccb) == FAT_NTC_CCB));

    if ( VcbOrFcbOrDcb != NULL ) {

        //
        //  Set the Vpb field in the file object, and if we were given an
        //  Fcb or Dcb move the field over to point to the nonpaged Fcb/Dcb
        //

        if (NodeType(VcbOrFcbOrDcb) == FAT_NTC_VCB) {

            FileObject->Vpb = ((PVCB)VcbOrFcbOrDcb)->Vpb;

        } else {

            FileObject->Vpb = ((PFCB)VcbOrFcbOrDcb)->Vcb->Vpb;

            //
            //  If this is a temporary file, note it in the FcbState
            //

            if (FlagOn(((PFCB)VcbOrFcbOrDcb)->FcbState, FCB_STATE_TEMPORARY)) {

                SetFlag(FileObject->Flags, FO_TEMPORARY_FILE);
            }
        }
    }

    NT_ASSERT((Ccb == NULL) || (NodeType(Ccb) == FAT_NTC_CCB));

    //
    //  Now set the fscontext fields of the file object
    //

    if (ARGUMENT_PRESENT( FileObject )) {

        FileObject->FsContext  = VcbOrFcbOrDcb;
        FileObject->FsContext2 = Ccb;
    }

    NT_ASSERT((Ccb == NULL) || (NodeType(Ccb) == FAT_NTC_CCB));

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "FatSetFileObject -> VOID\n", 0);

    return;
}

TYPE_OF_OPEN
FatDecodeFileObject (
    _In_ PFILE_OBJECT FileObject,
    _Outptr_ PVCB *Vcb,
    _Outptr_ PFCB *FcbOrDcb,
    _Outptr_ PCCB *Ccb
    )

/*++

Routine Description:

    This procedure takes a pointer to a file object, that has already been
    opened by the Fat file system and figures out what really is opened.

Arguments:

    FileObject - Supplies the file object pointer being interrogated

    Vcb - Receives a pointer to the Vcb for the file object.

    FcbOrDcb - Receives a pointer to the Fcb/Dcb for the file object, if
        one exists.

    Ccb - Receives a pointer to the Ccb for the file object, if one exists.

Return Value:

    TYPE_OF_OPEN - returns the type of file denoted by the input file object.

        UserFileOpen - The FO represents a user's opened data file.
            Ccb, FcbOrDcb, and Vcb are set.  FcbOrDcb points to an Fcb.

        UserDirectoryOpen - The FO represents a user's opened directory.
            Ccb, FcbOrDcb, and Vcb are set.  FcbOrDcb points to a Dcb/RootDcb

        UserVolumeOpen - The FO represents a user's opened volume.
            Ccb and Vcb are set. FcbOrDcb is null.

        VirtualVolumeFile - The FO represents the special virtual volume file.
            Vcb is set, and Ccb and FcbOrDcb are null.

        DirectoryFile - The FO represents a special directory file.
            Vcb and FcbOrDcb are set. Ccb is null.  FcbOrDcb points to a
            Dcb/RootDcb.

        EaFile - The FO represents an Ea Io stream file.
            FcbOrDcb, and Vcb are set.  FcbOrDcb points to an Fcb, and Ccb is
            null.

--*/

{
    TYPE_OF_OPEN TypeOfOpen;
    PVOID FsContext;
    PVOID FsContext2;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "FatDecodeFileObject, FileObject = %p\n", FileObject);

    //
    //  Reference the fs context fields of the file object, and zero out
    //  the out pointer parameters.
    //

    FsContext = FileObject->FsContext;
    FsContext2 = FileObject->FsContext2;

    //
    //  Special case the situation where FsContext is null
    //

    if (FsContext == NULL) {

        *Ccb = NULL;
        *FcbOrDcb = NULL;
        *Vcb = NULL;

        TypeOfOpen = UnopenedFileObject;

    } else {

        //
        //  Now we can case on the node type code of the fscontext pointer
        //  and set the appropriate out pointers
        //

        switch (NodeType(FsContext)) {

        case FAT_NTC_VCB:

            *Ccb = FsContext2;
            *FcbOrDcb = NULL;
            *Vcb = FsContext;

            TypeOfOpen = ( *Ccb == NULL ? VirtualVolumeFile : UserVolumeOpen );

            break;

        case FAT_NTC_ROOT_DCB:
        case FAT_NTC_DCB:

            *Ccb = FsContext2;
            *FcbOrDcb = FsContext;
            *Vcb = (*FcbOrDcb)->Vcb;

            TypeOfOpen = ( *Ccb == NULL ? DirectoryFile : UserDirectoryOpen );

            DebugTrace(0, Dbg, "Referencing directory: %wZ\n", &(*FcbOrDcb)->FullFileName);

            break;

        case FAT_NTC_FCB:

            *Ccb = FsContext2;
            *FcbOrDcb = FsContext;
            *Vcb = (*FcbOrDcb)->Vcb;

            if (*Ccb != NULL ) {

                TypeOfOpen = UserFileOpen;
                DebugTrace(0, Dbg, "Referencing file: %wZ\n", &(*FcbOrDcb)->FullFileName);

            } else {

                //
                // No Ccb means this is a special open.
                //


                if ( *FcbOrDcb == (*Vcb)->EaFcb ) {

                    TypeOfOpen = EaFile;
                    DebugTrace(0, Dbg, "Referencing EA file: %wZ\n", &(*FcbOrDcb)->FullFileName);

                } else {

#ifdef _MSC_VER
#pragma prefast(suppress:28159, "things are seriously wrong if we get here")
#endif
                    FatBugCheck( NodeType(FsContext), 0, 0 );

                }

            }

            break;

        default:

#ifdef _MSC_VER
#pragma prefast( suppress:28159, "things are seriously wrong if we get here" )
#endif
            FatBugCheck( NodeType(FsContext), 0, 0 );
        }
    }

    //
    //  and return to our caller
    //

    DebugTrace(0, Dbg, "FatDecodeFileObject -> VCB(%p)\n", *Vcb);
    DebugTrace(0, Dbg, "FatDecodeFileObject -> FCB(%p)\n", *FcbOrDcb);
    DebugTrace(0, Dbg, "FatDecodeFileObject -> CCB(%p)\n", *Ccb);
    DebugTrace(-1, Dbg, "FatDecodeFileObject -> TypeOfOpen = %08lx\n", TypeOfOpen);

    return TypeOfOpen;
}

_Requires_lock_held_(_Global_critical_region_)
VOID
FatPurgeReferencedFileObjects (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN FAT_FLUSH_TYPE FlushType
    )

/*++

Routine Description:

    This routine non-recursively walks from the given FcbOrDcb and trys
    to force Cc or Mm to close any sections it may be holding on to.

Arguments:

    Fcb - Supplies a pointer to either an fcb or a dcb

    FlushType - Specifies the kind of flushing to perform

Return Value:

    None.

--*/

{
    PFCB OriginalFcb = Fcb;
    PFCB NextFcb;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "FatPurgeReferencedFileObjects, Fcb = %p\n", Fcb );

    NT_ASSERT( FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) );

    //
    //  First, if we have a delayed close, force it closed.
    //

    FatFspClose(Fcb->Vcb);

    //
    //  Walk the directory tree forcing sections closed.
    //
    //  Note that it very important to get the next node to visit before
    //  acting on the current node.  This is because acting on a node may
    //  make it, and an arbitrary number of direct ancestors, vanish.
    //  Since we never visit ancestors in our top-down enumeration scheme, we
    //  can safely continue the enumeration even when the tree is vanishing
    //  beneath us.  This is way cool.
    //

    while ( Fcb != NULL ) {

        NextFcb = FatGetNextFcbTopDown(IrpContext, Fcb, OriginalFcb);

        //
        //  Check for the EA file fcb
        //

        if ( !FlagOn(Fcb->DirentFatFlags, FAT_DIRENT_ATTR_VOLUME_ID) ) {

            FatForceCacheMiss( IrpContext, Fcb, FlushType );
        }

        Fcb = NextFcb;
    }

    DebugTrace(-1, Dbg, "FatPurgeReferencedFileObjects (VOID)\n", 0 );

    return;
}


_Requires_lock_held_(_Global_critical_region_)
VOID
FatForceCacheMiss (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN FAT_FLUSH_TYPE FlushType
    )

/*++

Routine Description:

    The following routine asks either Cc or Mm to get rid of any cached
    pages on a file.  Note that this will fail if a user has mapped a file.

    If there is a shared cache map, purge the cache section.  Otherwise
    we have to go and ask Mm to blow away the section.

    NOTE: This caller MUST own the Vcb exclusive.

Arguments:

    Fcb - Supplies a pointer to an fcb

    FlushType - Specifies the kind of flushing to perform

Return Value:

    None.

--*/

{
    PVCB Vcb;
    BOOLEAN ChildrenAcquired = FALSE;

    PAGED_CODE();

    //
    //  If we can't wait, bail.
    //

    NT_ASSERT( FatVcbAcquiredExclusive( IrpContext, Fcb->Vcb ) ||
            FlagOn( Fcb->Vcb->VcbState, VCB_STATE_FLAG_LOCKED ) );

    if (!FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT)) {

        FatRaiseStatus( IrpContext, STATUS_CANT_WAIT );
    }

    //
    //  If we are purging a directory file object, we must acquire all the
    //  FCBs exclusive so that the parent directory is not being pinned.
    //  Careful, we can collide with something acquiring up the tree like
    //  an unpin repinned flush (FsRtlAcquireFileForCcFlush ...) of a parent
    //  dir on extending writethrough of a child file (oops).  So get things
    //  going up the tree, not down.
    //

    if ((NodeType(Fcb) != FAT_NTC_FCB) &&
        !IsListEmpty(&Fcb->Specific.Dcb.ParentDcbQueue)) {

        PLIST_ENTRY Links;
        PFCB TempFcb;

        ChildrenAcquired = TRUE;

        for (Links = Fcb->Specific.Dcb.ParentDcbQueue.Flink;
             Links != &Fcb->Specific.Dcb.ParentDcbQueue;
             Links = Links->Flink) {

            TempFcb = CONTAINING_RECORD( Links, FCB, ParentDcbLinks );

            (VOID)FatAcquireExclusiveFcb( IrpContext, TempFcb );
        }
    }

    (VOID)FatAcquireExclusiveFcb( IrpContext, Fcb );

    //
    //  We use this flag to indicate to a close beneath us that
    //  the Fcb resource should be freed before deleting the Fcb.
    //

    Vcb = Fcb->Vcb;

    SetFlag( Fcb->FcbState, FCB_STATE_FORCE_MISS_IN_PROGRESS );

    ClearFlag( Vcb->VcbState, VCB_STATE_FLAG_DELETED_FCB );

    _SEH2_TRY {

        BOOLEAN DataSectionExists;
        BOOLEAN ImageSectionExists;

        PSECTION_OBJECT_POINTERS Section;

        if ( FlushType ) {

            (VOID)FatFlushFile( IrpContext, Fcb, FlushType );
        }

        //
        //  The Flush may have made the Fcb go away
        //

        if (!FlagOn(Vcb->VcbState, VCB_STATE_FLAG_DELETED_FCB)) {

            Section = &Fcb->NonPaged->SectionObjectPointers;

            DataSectionExists = (BOOLEAN)(Section->DataSectionObject != NULL);
            ImageSectionExists = (BOOLEAN)(Section->ImageSectionObject != NULL);

            //
            //  Note, it is critical to do the Image section first as the
            //  purge of the data section may cause the image section to go
            //  away, but the opposite is not true.
            //

            if (ImageSectionExists) {

                (VOID)MmFlushImageSection( Section, MmFlushForWrite );
            }

            if (DataSectionExists) {

                CcPurgeCacheSection( Section, NULL, 0, FALSE );
            }
        }

    } _SEH2_FINALLY {

        //
        //  If we purging a directory file object, release all the Fcb
        //  resources that we acquired above.  The Dcb cannot have vanished
        //  if there were Fcbs underneath it, and the Fcbs couldn't have gone
        //  away since I own the Vcb.
        //

        if (ChildrenAcquired) {

            PLIST_ENTRY Links;
            PFCB TempFcb;

            for (Links = Fcb->Specific.Dcb.ParentDcbQueue.Flink;
                 Links != &Fcb->Specific.Dcb.ParentDcbQueue;
                 Links = Links->Flink) {

                TempFcb = CONTAINING_RECORD( Links, FCB, ParentDcbLinks );

                FatReleaseFcb( IrpContext, TempFcb );
            }
        }

        //
        //  Since we have the Vcb exclusive we know that if any closes
        //  come in it is because the CcPurgeCacheSection caused the
        //  Fcb to go away.  Also in close, the Fcb was released
        //  before being freed.
        //

        if ( !FlagOn(Vcb->VcbState, VCB_STATE_FLAG_DELETED_FCB) ) {

            ClearFlag( Fcb->FcbState, FCB_STATE_FORCE_MISS_IN_PROGRESS );

            FatReleaseFcb( (IRPCONTEXT), Fcb );
        }
    } _SEH2_END;
}


