/*++

Copyright (c) 1989-2000 Microsoft Corporation

Module Name:

    DirSup.c

Abstract:

    This module implements the dirent support routines for Fat.


--*/

#include "fatprocs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (FAT_BUG_CHECK_DIRSUP)

//
//  Local debug trace level
//

#define Dbg                              (DEBUG_TRACE_DIRSUP)

//
//  The following three macro all assume the input dirent has been zeroed.
//

//
//  VOID
//  FatConstructDot (
//      IN PIRP_CONTEXT IrpContext,
//      IN PDCB Directory,
//      IN PDIRENT ParentDirent,
//      IN OUT PDIRENT Dirent
//      );
//
//  The following macro is called to initalize the "." dirent.
//
//  Always setting FirstClusterOfFileHi is OK because it will be zero
//  unless we're working on a FAT 32 disk.
//

#define FatConstructDot(IRPCONTEXT,DCB,PARENT,DIRENT) {                  \
                                                                         \
    RtlCopyMemory( (PUCHAR)(DIRENT), ".          ", 11 );                \
    (DIRENT)->Attributes = FAT_DIRENT_ATTR_DIRECTORY;                    \
    (DIRENT)->LastWriteTime = (PARENT)->LastWriteTime;                   \
    if (FatData.ChicagoMode) {                                           \
        (DIRENT)->CreationTime = (PARENT)->CreationTime;                 \
        (DIRENT)->CreationMSec = (PARENT)->CreationMSec;                 \
        (DIRENT)->LastAccessDate = (PARENT)->LastAccessDate;             \
    }                                                                    \
    (DIRENT)->FirstClusterOfFile =                                       \
            (USHORT)(DCB)->FirstClusterOfFile;                           \
    (DIRENT)->FirstClusterOfFileHi =                                     \
            (USHORT)((DCB)->FirstClusterOfFile/0x10000);                 \
}

//
//  VOID
//  FatConstructDotDot (
//      IN PIRP_CONTEXT IrpContext,
//      IN PDCB Directory,
//      IN PDIRENT ParentDirent,
//      IN OUT PDIRENT Dirent
//      );
//
//  The following macro is called to initalize the ".." dirent.
//
//  Always setting FirstClusterOfFileHi is OK because it will be zero
//  unless we're working on a FAT 32 disk.
//

#define FatConstructDotDot(IRPCONTEXT,DCB,PARENT,DIRENT) {   \
                                                             \
    RtlCopyMemory( (PUCHAR)(DIRENT), "..         ", 11 );    \
    (DIRENT)->Attributes = FAT_DIRENT_ATTR_DIRECTORY;        \
    (DIRENT)->LastWriteTime = (PARENT)->LastWriteTime;       \
    if (FatData.ChicagoMode) {                               \
        (DIRENT)->CreationTime = (PARENT)->CreationTime;     \
        (DIRENT)->CreationMSec = (PARENT)->CreationMSec;     \
        (DIRENT)->LastAccessDate = (PARENT)->LastAccessDate; \
    }                                                        \
    if (NodeType((DCB)->ParentDcb) == FAT_NTC_ROOT_DCB) {    \
        (DIRENT)->FirstClusterOfFile = 0;                    \
        (DIRENT)->FirstClusterOfFileHi = 0;                  \
    } else {                                                 \
        (DIRENT)->FirstClusterOfFile = (USHORT)              \
            ((DCB)->ParentDcb->FirstClusterOfFile);          \
        (DIRENT)->FirstClusterOfFileHi = (USHORT)            \
            ((DCB)->ParentDcb->FirstClusterOfFile/0x10000);  \
    }                                                        \
}

//
//  VOID
//  FatConstructEndDirent (
//      IN PIRP_CONTEXT IrpContext,
//      IN OUT PDIRENT Dirent
//      );
//
//  The following macro created the end dirent.  Note that since the
//  dirent was zeroed, the first byte of the name already contains 0x0,
//  so there is nothing to do.
//

#define FatConstructEndDirent(IRPCONTEXT,DIRENT) NOTHING

//
//  VOID
//  FatReadDirent (
//      IN PIRP_CONTEXT IrpContext,
//      IN PDCB Dcb,
//      IN VBO Vbo,
//      OUT PBCB *Bcb,
//      OUT PVOID *Dirent,
//      OUT PNTSTATUS Status
//      );
//

//
//  This macro reads in a page of dirents when we step onto a new page,
//  or this is the first iteration of a loop and Bcb is NULL.
//

#define FatReadDirent(IRPCONTEXT,DCB,VBO,BCB,DIRENT,STATUS)       \
if ((VBO) >= (DCB)->Header.AllocationSize.LowPart) {              \
    *(STATUS) = STATUS_END_OF_FILE;                               \
    FatUnpinBcb( (IRPCONTEXT), *(BCB) );                          \
} else if ( ((VBO) % PAGE_SIZE == 0) || (*(BCB) == NULL) ) {      \
    FatUnpinBcb( (IRPCONTEXT), *(BCB) );                          \
    FatReadDirectoryFile( (IRPCONTEXT),                           \
                          (DCB),                                  \
                          (VBO) & ~(PAGE_SIZE - 1),               \
                          PAGE_SIZE,                              \
                          FALSE,                                  \
                          (BCB),                                  \
                          (PVOID *)(DIRENT),                      \
                          (STATUS) );                             \
    *(DIRENT) = (PVOID)((PUCHAR)*(DIRENT) + ((VBO) % PAGE_SIZE)); \
}

//
//  Internal support routines
//

UCHAR
FatComputeLfnChecksum (
    PDIRENT Dirent
    );

_Requires_lock_held_(_Global_critical_region_)
VOID
FatRescanDirectory (
    PIRP_CONTEXT IrpContext,
    PDCB Dcb
    );

_Requires_lock_held_(_Global_critical_region_)
ULONG
FatDefragDirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN ULONG DirentsNeeded
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FatComputeLfnChecksum)
#pragma alloc_text(PAGE, FatConstructDirent)
#pragma alloc_text(PAGE, FatConstructLabelDirent)
#pragma alloc_text(PAGE, FatCreateNewDirent)
#pragma alloc_text(PAGE, FatDefragDirectory)
#pragma alloc_text(PAGE, FatDeleteDirent)
#pragma alloc_text(PAGE, FatGetDirentFromFcbOrDcb)
#pragma alloc_text(PAGE, FatInitializeDirectoryDirent)
#pragma alloc_text(PAGE, FatIsDirectoryEmpty)
#pragma alloc_text(PAGE, FatLfnDirentExists)
#pragma alloc_text(PAGE, FatLocateDirent)
#pragma alloc_text(PAGE, FatLocateSimpleOemDirent)
#pragma alloc_text(PAGE, FatLocateVolumeLabel)
#pragma alloc_text(PAGE, FatRescanDirectory)
#pragma alloc_text(PAGE, FatSetFileSizeInDirent)
#pragma alloc_text(PAGE, FatSetFileSizeInDirentNoRaise)
#pragma alloc_text(PAGE, FatTunnelFcbOrDcb)
#pragma alloc_text(PAGE, FatUpdateDirentFromFcb)


#endif


_Requires_lock_held_(_Global_critical_region_)
ULONG
FatCreateNewDirent (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB ParentDirectory,
    IN ULONG DirentsNeeded,
    IN BOOLEAN RescanDir
    )

/*++

Routine Description:

    This routine allocates on the disk a new dirent inside of the
    parent directory.  If a new dirent cannot be allocated (i.e.,
    because the disk is full or the root directory is full) then
    it raises the appropriate status.  The dirent itself is
    neither initialized nor pinned by this procedure.
    
Arguments:

    ParentDirectory - Supplies the DCB for the directory in which
        to create the new dirent

    DirentsNeeded - This is the number of continginous dirents required

Return Value:

    ByteOffset - Returns the VBO within the Parent directory where
        the dirent has been allocated

--*/

{
    VBO UnusedVbo;
    VBO DeletedHint;
    ULONG ByteOffset;

    PBCB Bcb = NULL;
    PDIRENT Dirent = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "FatCreateNewDirent\n", 0);

    DebugTrace( 0, Dbg, "  ParentDirectory = %p\n", ParentDirectory);

    //
    //  If UnusedDirentVbo is within our current file allocation then we
    //  don't have to search through the directory at all; we know just
    //  where to put it.
    //
    //  If UnusedDirentVbo is beyond the current file allocation then
    //  there are no more unused dirents in the current allocation, though
    //  upon adding another cluster of allocation UnusedDirentVbo
    //  will point to an unused dirent.  Haveing found no unused dirents
    //  we use the DeletedDirentHint to try and find a deleted dirent in
    //  the current allocation.  In this also runs off the end of the file,
    //  we finally have to break down and allocate another sector.  Note
    //  that simply writing beyond the current allocation will automatically
    //  do just this.
    //
    //  We also must deal with the special case where UnusedDirentVbo and
    //  DeletedDirentHint have yet to be initialized.  In this case we must
    //  first walk through the directory looking for the first deleted entry
    //  first unused dirent.  After this point we continue as before.
    //  This initial state is denoted by the special value of 0xffffffff.
    //

    UnusedVbo = ParentDirectory->Specific.Dcb.UnusedDirentVbo;
    DeletedHint = ParentDirectory->Specific.Dcb.DeletedDirentHint;

    //
    //  Check for our first call to this routine with this Dcb.  If so
    //  we have to correctly set the two hints in the Dcb.
    //

    if (UnusedVbo == 0xffffffff || RescanDir) {

        FatRescanDirectory( IrpContext, ParentDirectory );

        UnusedVbo = ParentDirectory->Specific.Dcb.UnusedDirentVbo;
        DeletedHint = ParentDirectory->Specific.Dcb.DeletedDirentHint;
    }

    //
    //  Now we know that UnusedDirentVbo and DeletedDirentHint are correctly
    //  set so we check if there is already an unused dirent in the the
    //  current allocation.  This is the easy case.
    //

    DebugTrace( 0, Dbg, "  UnusedVbo   = %08lx\n", UnusedVbo);
    DebugTrace( 0, Dbg, "  DeletedHint = %08lx\n", DeletedHint);

    if (!RescanDir && ( UnusedVbo + (DirentsNeeded * sizeof(DIRENT)) <=
         ParentDirectory->Header.AllocationSize.LowPart )) {

        //
        //  Get this unused dirent for the caller.  We have a
        //  sporting chance that we won't have to wait.
        //

        DebugTrace( 0, Dbg, "There is a never used entry.\n", 0);

        ByteOffset = UnusedVbo;

        UnusedVbo += DirentsNeeded * sizeof(DIRENT);

    } else {

        //
        //  Life is tough.  We have to march from the DeletedDirentHint
        //  looking for a deleted dirent.  If we get to EOF without finding
        //  one, we will have to allocate a new cluster.
        //

        ByteOffset =
            RtlFindClearBits( &ParentDirectory->Specific.Dcb.FreeDirentBitmap,
                              DirentsNeeded,
                              DeletedHint / sizeof(DIRENT) );

        //
        //  Do a quick check for a root directory allocation that failed
        //  simply because of fragmentation.  Also, only attempt to defrag
        //  if the length is less that 0x40000.  This is to avoid
        //  complications arising from crossing a MM view boundary (256kb).
        //  By default on DOS the root directory is only 0x2000 long.
        //
        //  Don't try to defrag fat32 root dirs.
        //

        if (!FatIsFat32(ParentDirectory->Vcb) &&
            (ByteOffset == -1) &&
            (NodeType(ParentDirectory) == FAT_NTC_ROOT_DCB) &&
            (ParentDirectory->Header.AllocationSize.LowPart <= 0x40000)) {

            ByteOffset = FatDefragDirectory( IrpContext, ParentDirectory, DirentsNeeded );
        }

        if (ByteOffset != -1) {

            //
            //  If we consuemed deleted dirents at Deleted Hint, update.
            //  We also may have consumed some un-used dirents as well,
            //  so be sure to check for that as well.
            //

            ByteOffset *= sizeof(DIRENT);

            if (ByteOffset == DeletedHint) {

                DeletedHint += DirentsNeeded * sizeof(DIRENT);
            }

            if (ByteOffset + DirentsNeeded * sizeof(DIRENT) > UnusedVbo) {

                UnusedVbo = ByteOffset + DirentsNeeded * sizeof(DIRENT);
            }

        } else {

            //
            //  We are going to have to allocate another cluster.  Do
            //  so, update both the UnusedVbo and the DeletedHint and bail.
            //

            DebugTrace( 0, Dbg, "We have to allocate another cluster.\n", 0);

            //
            //  A reason why we might fail, unrelated to physical reasons,
            //  is that we constrain to 64k directory entries to match the
            //  restriction on Win95.  There are fundamental reasons to do
            //  this since searching a FAT directory is a linear operation
            //  and to allow FAT32 to toss us over the cliff is not permissable.
            //

            if (ParentDirectory->Header.AllocationSize.LowPart >= (64 * 1024 * sizeof(DIRENT)) ||
                
                //
                //  Make sure we are not trying to expand the root directory on non
                //  FAT32.  FAT16 and FAT12 have fixed size allocations.
                //

                (!FatIsFat32(ParentDirectory->Vcb) &&
                 NodeType(ParentDirectory) == FAT_NTC_ROOT_DCB)) {
                    
                DebugTrace(0, Dbg, "Full root directory or too big on FAT32.  Raise Status.\n", 0);

                FatRaiseStatus( IrpContext, STATUS_CANNOT_MAKE );
            }

            //
            //  Take the last dirent(s) in this cluster.  We will allocate
            //  more clusters below.
            //

            ByteOffset = UnusedVbo;
            UnusedVbo += DirentsNeeded * sizeof(DIRENT);

            //
            //  Touch the directory file to cause space for the new dirents
            //  to be allocated.
            //

            Bcb = NULL;

            _SEH2_TRY {

                PVOID Buffer;

                FatPrepareWriteDirectoryFile( IrpContext,
                                              ParentDirectory,
                                              UnusedVbo,
                                              1,
                                              &Bcb,
                                              &Buffer,
                                              FALSE,
                                              TRUE,
                                              &Status );

            } _SEH2_FINALLY {

                FatUnpinBcb( IrpContext, Bcb );
            } _SEH2_END;
        }
    }

    //
    //  If we are only requesting a single dirent, and we did not get the
    //  first dirent in a directory, then check that the preceding dirent
    //  is not an orphaned LFN.  If it is, then mark it deleted.  Thus
    //  reducing the possibility of an accidental pairing.
    //
    //  Only do this when we are in Chicago Mode.
    //

    Bcb = NULL;

    if (FatData.ChicagoMode &&
        (DirentsNeeded == 1) &&
        (ByteOffset > (NodeType(ParentDirectory) == FAT_NTC_ROOT_DCB ?
                       0 : 2 * sizeof(DIRENT)))) {
        _SEH2_TRY {

            FatReadDirent( IrpContext,
                           ParentDirectory,
                           ByteOffset - sizeof(DIRENT),
                           &Bcb,
                           &Dirent,
                           &Status );

            if ((Status != STATUS_SUCCESS) ||
                (Dirent->FileName[0] == FAT_DIRENT_NEVER_USED)) {

                FatPopUpFileCorrupt( IrpContext, ParentDirectory );

                FatRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
            }

            if ((Dirent->Attributes == FAT_DIRENT_ATTR_LFN) &&
                (Dirent->FileName[0] != FAT_DIRENT_DELETED)) {

                //
                //  Pin it, mark it, and set it dirty.
                //

                FatPinMappedData( IrpContext,
                                  ParentDirectory,
                                  ByteOffset - sizeof(DIRENT),
                                  sizeof(DIRENT),
                                  &Bcb );

                Dirent->FileName[0] = FAT_DIRENT_DELETED;

                FatSetDirtyBcb( IrpContext, Bcb, ParentDirectory->Vcb, TRUE );

                NT_ASSERT( RtlAreBitsSet( &ParentDirectory->Specific.Dcb.FreeDirentBitmap,
                                       (ByteOffset - sizeof(DIRENT))/ sizeof(DIRENT),
                                       DirentsNeeded ) );

                RtlClearBits( &ParentDirectory->Specific.Dcb.FreeDirentBitmap,
                              (ByteOffset - sizeof(DIRENT))/ sizeof(DIRENT),
                              DirentsNeeded );

            }

        } _SEH2_FINALLY {

            FatUnpinBcb( IrpContext, Bcb );
        } _SEH2_END;
    }

    //
    //  Assert that the dirents are in fact unused
    //

    _SEH2_TRY {

        ULONG i;

        Bcb = NULL;

        for (i = 0; i < DirentsNeeded; i++) {

            FatReadDirent( IrpContext,
                           ParentDirectory,
                           ByteOffset + i*sizeof(DIRENT),
                           &Bcb,
                           &Dirent,
                           &Status );

            if ((Status != STATUS_SUCCESS) ||
                ((Dirent->FileName[0] != FAT_DIRENT_NEVER_USED) &&
                 (Dirent->FileName[0] != FAT_DIRENT_DELETED))) {

                FatPopUpFileCorrupt( IrpContext, ParentDirectory );
                FatRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
            }
        }

    } _SEH2_FINALLY {

        FatUnpinBcb( IrpContext, Bcb );
    } _SEH2_END;

    //
    //  Set the Bits in the bitmap and move the Unused Dirent Vbo.
    //

    NT_ASSERT( RtlAreBitsClear( &ParentDirectory->Specific.Dcb.FreeDirentBitmap,
                             ByteOffset / sizeof(DIRENT),
                             DirentsNeeded ) );

    RtlSetBits( &ParentDirectory->Specific.Dcb.FreeDirentBitmap,
                ByteOffset / sizeof(DIRENT),
                DirentsNeeded );

    //
    //  Save the newly computed values in the Parent Directory Fcb
    //

    ParentDirectory->Specific.Dcb.UnusedDirentVbo = UnusedVbo;
    ParentDirectory->Specific.Dcb.DeletedDirentHint = DeletedHint;

    DebugTrace(-1, Dbg, "FatCreateNewDirent -> (VOID)\n", 0);

    return ByteOffset;
}



_Requires_lock_held_(_Global_critical_region_)
VOID
FatInitializeDirectoryDirent (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN PDIRENT ParentDirent
    )

/*++

Routine Description:

    This routine converts a dirent into a directory on the disk.  It does this
    setting the directory flag in the dirent, and by allocating the necessary
    space for the "." and ".." dirents and initializing them.

    If a new dirent cannot be allocated (i.e., because the disk is full) then
    it raises the appropriate status.

Arguments:

    Dcb - Supplies the Dcb denoting the file that is to be made into a
        directory.  This must be input a completely empty file with
        an allocation size of zero.

    ParentDirent - Provides the parent Dirent for a time-stamp model.

Return Value:

    None.

--*/

{
    PBCB Bcb;
    PVOID Buffer;
    NTSTATUS DontCare = STATUS_SUCCESS;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "FatInitializeDirectoryDirent\n", 0);

    DebugTrace( 0, Dbg, "  Dcb = %p\n", Dcb);

    //
    //  Assert that we are not attempting this on the root directory.
    //

    NT_ASSERT( NodeType(Dcb) != FAT_NTC_ROOT_DCB );

    //
    //  Assert that this is only attempted on newly created directories.
    //

    NT_ASSERT( Dcb->Header.AllocationSize.LowPart == 0 );

    //
    //  Prepare the directory file for writing.  Note that we can use a single
    //  Bcb for these two entries because we know they are the first two in
    //  the directory, and thus together do not span a page boundry.  Also
    //  note that we prepare write 2 entries: one for "." and one for "..".
    //  The end of directory marker is automatically set since the whole
    //  directory is initially zero (DIRENT_NEVER_USED).
    //

    FatPrepareWriteDirectoryFile( IrpContext,
                                  Dcb,
                                  0,
                                  2 * sizeof(DIRENT),
                                  &Bcb,
                                  &Buffer,
                                  FALSE,
                                  TRUE,
                                  &DontCare );

    NT_ASSERT( NT_SUCCESS( DontCare ));

    //
    //  Add the . and .. entries
    //

    _SEH2_TRY {

        FatConstructDot( IrpContext, Dcb, ParentDirent, (PDIRENT)Buffer + 0);

        FatConstructDotDot( IrpContext, Dcb, ParentDirent, (PDIRENT)Buffer + 1);

    //
    //  Unpin the buffer and return to the caller.
    //

    } _SEH2_FINALLY {

        FatUnpinBcb( IrpContext, Bcb );
    } _SEH2_END;

    DebugTrace(-1, Dbg, "FatInitializeDirectoryDirent -> (VOID)\n", 0);
    return;
}


VOID
FatTunnelFcbOrDcb (
    IN PFCB FcbOrDcb,
    IN PCCB Ccb OPTIONAL
    )
/*++

Routine Description:

    This routine handles tunneling of an Fcb or Dcb associated with
    an object whose name is disappearing from a directory.

Arguments:

    FcbOrDcb - Supplies the Fcb/Dcb whose name will be going away

    Ccb - Supplies the Ccb for the Fcb (not reqired for a Dcb) so
        that we know which name the Fcb was opened by

Return Value:

    None.

--*/
{
    UNICODE_STRING ShortNameWithCase = {0};
    UNICODE_STRING DownCaseSeg;
    WCHAR ShortNameBuffer[8+1+3];
    NTSTATUS Status;
    USHORT i;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "FatTunnelFcbOrDcb\n", 0);

    if (NodeType(FcbOrDcb) == FAT_NTC_DCB) {

        //
        //  Directory deletion. Flush all entries from this directory in
        //  the cache for this volume
        //

        FsRtlDeleteKeyFromTunnelCache( &FcbOrDcb->Vcb->Tunnel,
                                       FatDirectoryKey(FcbOrDcb) );

    } else {

        //
        //  Was a file, so throw it into the tunnel cache
        //

        //
        //  Get the short name into UNICODE
        //

        ShortNameWithCase.Length = 0;
        ShortNameWithCase.MaximumLength = sizeof(ShortNameBuffer);
        ShortNameWithCase.Buffer = ShortNameBuffer;

#ifdef _MSC_VER
#pragma prefast( suppress:28931, "needed for debug build" )
#endif
        Status = RtlOemStringToCountedUnicodeString( &ShortNameWithCase,
                                                     &FcbOrDcb->ShortName.Name.Oem,
                                                     FALSE);

        NT_ASSERT(ShortNameWithCase.Length != 0);

        NT_ASSERT(NT_SUCCESS(Status));

        if (FlagOn(FcbOrDcb->FcbState, FCB_STATE_8_LOWER_CASE | FCB_STATE_3_LOWER_CASE)) {

            //
            //  Have to repair the case of the short name
            //

            for (i = 0; i < (ShortNameWithCase.Length/sizeof(WCHAR)) &&
                        ShortNameWithCase.Buffer[i] != L'.'; i++);

            //
            //  Now pointing at the '.', or otherwise the end of name component
            //

            if (FlagOn(FcbOrDcb->FcbState, FCB_STATE_8_LOWER_CASE)) {

                DownCaseSeg.Buffer = ShortNameWithCase.Buffer;
                DownCaseSeg.MaximumLength = DownCaseSeg.Length = i*sizeof(WCHAR);

                RtlDowncaseUnicodeString(&DownCaseSeg, &DownCaseSeg, FALSE);
            }

            i++;

            //
            //  Now pointing at first wchar of the extension.
            //

            if (FlagOn(FcbOrDcb->FcbState, FCB_STATE_3_LOWER_CASE)) {

                //
                //  It is not neccesarily the case that we can rely on the flag
                //  indicating that we really have an extension.
                //

                if ((i*sizeof(WCHAR)) < ShortNameWithCase.Length) {
                    DownCaseSeg.Buffer = &ShortNameWithCase.Buffer[i];
                    DownCaseSeg.MaximumLength = DownCaseSeg.Length = ShortNameWithCase.Length - i*sizeof(WCHAR);
    
                    RtlDowncaseUnicodeString(&DownCaseSeg, &DownCaseSeg, FALSE);
                }
            }
        }

        //
        //  ... and add it in
        //

        FsRtlAddToTunnelCache( &FcbOrDcb->Vcb->Tunnel,
                               FatDirectoryKey(FcbOrDcb->ParentDcb),
                               &ShortNameWithCase,
                               &FcbOrDcb->ExactCaseLongName,
                               BooleanFlagOn(Ccb->Flags, CCB_FLAG_OPENED_BY_SHORTNAME),
                               sizeof(LARGE_INTEGER),
                               &FcbOrDcb->CreationTime );
    }

    DebugTrace(-1, Dbg, "FatTunnelFcbOrDcb -> (VOID)\n", 0);

    return;
}


    
_Requires_lock_held_(_Global_critical_region_)    
VOID
FatDeleteDirent (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB FcbOrDcb,
    IN PDELETE_CONTEXT DeleteContext OPTIONAL,
    IN BOOLEAN DeleteEa
    )

/*++

Routine Description:

    This routine Deletes on the disk the indicated dirent.  It does
    this by marking the dirent as deleted.

Arguments:

    FcbOrDcb - Supplies the FCB/DCB for the file/directory being
        deleted.  For a file the file size and allocation must be zero.
        (Zero allocation is implied by a zero cluster index).
        For a directory the allocation must be zero.

    DeleteContext - This variable, if speicified, may be used to preserve
        the file size and first cluster of file information in the dirent
        fot the benefit of unerase utilities.

    DeleteEa - Tells us whether to delete the EA and whether to check
        for no allocation/  Mainly TRUE.  FALSE passed in from rename.

Return Value:

    None.

--*/

{
    PBCB Bcb = NULL;
    PDIRENT Dirent = NULL;
    NTSTATUS DontCare;
    ULONG Offset;
    ULONG DirentsToDelete;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "FatDeleteDirent\n", 0);

    DebugTrace( 0, Dbg, "  FcbOrDcb = %p\n", FcbOrDcb);

    //
    //  We must be holding the vcb exclusive here to deal with the locate dirent
    //  cases where it cannot be holding the parent simply.  This is actually
    //  a true statement from olden daze, lets just wire in our assertion.
    //
    //  Among other reasons, it'd be unfortunate if this raced with the
    //  rename path.
    //
    
    NT_ASSERT( ExIsResourceAcquiredExclusiveLite( &FcbOrDcb->Vcb->Resource ));

    //
    //  Assert that we are not attempting this on the root directory.
    //

    NT_ASSERT( NodeType(FcbOrDcb) != FAT_NTC_ROOT_DCB );

    //
    //  Make sure all requests have zero allocation/file size
    //

    if (DeleteEa &&
        ((FcbOrDcb->Header.AllocationSize.LowPart != 0) ||
         ((NodeType(FcbOrDcb) == FAT_NTC_FCB) &&
          (FcbOrDcb->Header.FileSize.LowPart != 0)))) {

        DebugTrace( 0, Dbg, "Called with non zero allocation/file size.\n", 0);
        
#ifdef _MSC_VER
#pragma prefast( suppress:28159, "things are seriously wrong if we get here" )        
#endif
        FatBugCheck( 0, 0, 0 );
    }

    //
    //  Now, mark the dirents deleted, unpin the Bcb, and return to the caller.
    //  Assert that there isn't any allocation associated with this dirent.
    //
    //  Note that this loop will end with Dirent pointing to the short name.
    //

    _SEH2_TRY {

        //
        //  We must acquire our parent exclusive to synchronize with enumerators
        //  who do not hold the vcb (ex: dirctrl).
        //
        //  This relies on our bottom up lockorder.
        //
    
        ExAcquireResourceExclusiveLite( FcbOrDcb->ParentDcb->Header.Resource, TRUE );
    
        for ( Offset = FcbOrDcb->LfnOffsetWithinDirectory;
              Offset <= FcbOrDcb->DirentOffsetWithinDirectory;
              Offset += sizeof(DIRENT), Dirent += 1 ) {

            //
            //  If we stepped onto a new page, or this is the first iteration,
            //  unpin the old page, and pin the new one.
            //

            if ((Offset == FcbOrDcb->LfnOffsetWithinDirectory) ||
                ((Offset & (PAGE_SIZE - 1)) == 0)) {

                FatUnpinBcb( IrpContext, Bcb );

                FatPrepareWriteDirectoryFile( IrpContext,
                                              FcbOrDcb->ParentDcb,
                                              Offset,
                                              sizeof(DIRENT),
                                              &Bcb,
                                              (PVOID *)&Dirent,
                                              FALSE,
                                              TRUE,
                                              &DontCare );
            }

            NT_ASSERT( (Dirent->FirstClusterOfFile == 0) || !DeleteEa );
            Dirent->FileName[0] = FAT_DIRENT_DELETED;
        }

        //
        //  Back Dirent off by one to point back to the short dirent.
        //

        Dirent -= 1;

        //
        //  If there are extended attributes for this dirent, we will attempt
        //  to remove them.  We ignore any errors in removing Eas.
        //

        if (!FatIsFat32(FcbOrDcb->Vcb) &&
            DeleteEa && (Dirent->ExtendedAttributes != 0)) {

            _SEH2_TRY {

                FatDeleteEa( IrpContext,
                             FcbOrDcb->Vcb,
                             Dirent->ExtendedAttributes,
                             &FcbOrDcb->ShortName.Name.Oem );

            } _SEH2_EXCEPT(FatExceptionFilter( IrpContext, _SEH2_GetExceptionInformation() )) {

                //
                //  We catch all exceptions that Fat catches, but don't do
                //  anything with them.
                //
            } _SEH2_END;
        }

        //
        //  Now clear the bits in the free dirent mask.
        //

        DirentsToDelete = (FcbOrDcb->DirentOffsetWithinDirectory -
                           FcbOrDcb->LfnOffsetWithinDirectory) / sizeof(DIRENT) + 1;


        NT_ASSERT( (FcbOrDcb->ParentDcb->Specific.Dcb.UnusedDirentVbo == 0xffffffff) ||
                RtlAreBitsSet( &FcbOrDcb->ParentDcb->Specific.Dcb.FreeDirentBitmap,
                               FcbOrDcb->LfnOffsetWithinDirectory / sizeof(DIRENT),
                               DirentsToDelete ) );

        RtlClearBits( &FcbOrDcb->ParentDcb->Specific.Dcb.FreeDirentBitmap,
                      FcbOrDcb->LfnOffsetWithinDirectory / sizeof(DIRENT),
                      DirentsToDelete );

        //
        //  Now, if the caller specified a DeleteContext, use it.
        //

        if ( ARGUMENT_PRESENT( DeleteContext ) ) {

            Dirent->FileSize = DeleteContext->FileSize;

            
            Dirent->FirstClusterOfFile = (USHORT)DeleteContext->FirstClusterOfFile;
        }

        //
        //  If this newly deleted dirent is before the DeletedDirentHint, change
        //  the DeletedDirentHint to point here.
        //

        if (FcbOrDcb->DirentOffsetWithinDirectory <
                            FcbOrDcb->ParentDcb->Specific.Dcb.DeletedDirentHint) {

            FcbOrDcb->ParentDcb->Specific.Dcb.DeletedDirentHint =
                                            FcbOrDcb->LfnOffsetWithinDirectory;
        }

    } _SEH2_FINALLY {

        FatUnpinBcb( IrpContext, Bcb );
        
        //
        //  Release our parent.
        //
    
        ExReleaseResourceLite( FcbOrDcb->ParentDcb->Header.Resource );
    } _SEH2_END;

    DebugTrace(-1, Dbg, "FatDeleteDirent -> (VOID)\n", 0);
    return;
}

_Requires_lock_held_(_Global_critical_region_)
BOOLEAN
FatLfnDirentExists (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN PUNICODE_STRING Lfn,
    IN PUNICODE_STRING LfnTmp
    )
/*++

Routine Description:

    This routine looks for a given Lfn in a directory

Arguments:

    Dcb - The directory to search

    Lfn - The Lfn to look for

    Lfn - Temporary buffer to use to search for Lfn with (if < MAX_LFN then this 
        function may cause it to be allocated from pool if not large enough.

Retrn Value:

    BOOLEAN TRUE if it exists, FALSE if not

--*/
{
    CCB Ccb;
    PDIRENT Dirent;
    PBCB DirentBcb = NULL;
    VBO DirentByteOffset;
    BOOLEAN Result = FALSE;
    ULONG Flags = 0;

    PAGED_CODE();

    //
    //  Pay performance penalty by forcing the compares to be case insensitive as
    //  opposed to grabbing more pool for a monocased copy of the Lfn. This is slight.
    //

    Ccb.UnicodeQueryTemplate =  *Lfn;
    Ccb.ContainsWildCards = FALSE;
    Ccb.Flags = CCB_FLAG_SKIP_SHORT_NAME_COMPARE | CCB_FLAG_QUERY_TEMPLATE_MIXED;

    _SEH2_TRY {
        
        FatLocateDirent( IrpContext,
                         Dcb,
                         &Ccb,
                         0,
                         &Flags,
                         &Dirent,
                         &DirentBcb,
                         &DirentByteOffset,
                         NULL,
                         LfnTmp,
                         NULL );
        
    } _SEH2_FINALLY {

        if (DirentBcb) {

            Result = TRUE;
        }
        
        FatUnpinBcb(IrpContext, DirentBcb);
    } _SEH2_END;

    return Result;
}


_Requires_lock_held_(_Global_critical_region_)
VOID
FatLocateDirent (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB ParentDirectory,
    IN PCCB Ccb,
    IN VBO OffsetToStartSearchFrom,
    IN OUT PULONG Flags,
    OUT PDIRENT *Dirent,
    OUT PBCB *Bcb,
    OUT PVBO ByteOffset,
    OUT PBOOLEAN FileNameDos OPTIONAL,
    IN OUT PUNICODE_STRING LongFileName OPTIONAL,
    IN OUT PUNICODE_STRING OrigLongFileName OPTIONAL    
    )

/*++

Routine Description:

    This routine locates on the disk an undeleted dirent matching a given name.

Arguments:

    ParentDirectory - Supplies the DCB for the directory to search

    Ccb - Contains a context control block with all matching information.

    OffsetToStartSearchFrom - Supplies the VBO within the parent directory
        from which to start looking for another real dirent.

    Dirent - Receives a pointer to the located dirent if one was found
        or NULL otherwise.

    Bcb - Receives the Bcb for the located dirent if one was found or
        NULL otherwise.

    ByteOffset - Receives the VBO within the Parent directory for
        the located dirent if one was found, or 0 otherwise.

    FileNameDos - Receives TRUE if the element of the dirent we hit on
        was the short (non LFN) side

    LongFileName - If specified, this parameter returns the long file name
        associated with the returned dirent.  Note that it is the caller's
        responsibility to provide the buffer (and set MaximumLength
        accordingly) for this unicode string.  The Length field is reset
        to 0 by this routine on invocation.  If the supplied buffer is not
        large enough,  a new one will be allocated from pool.

Return Value:

    None.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    OEM_STRING Name;
    UCHAR NameBuffer[12];

    BOOLEAN UpcasedLfnValid = FALSE;
    UNICODE_STRING UpcasedLfn = {0};
    WCHAR LocalLfnBuffer[32];

        
    BOOLEAN LfnInProgress = FALSE;
    UCHAR LfnChecksum = 0;
    ULONG LfnSize = 0;
    ULONG LfnIndex = 0;
    UCHAR Ordinal = 0;
    VBO LfnByteOffset = 0;

    TimerStart(Dbg);

    PAGED_CODE();

    DebugTrace(+1, Dbg, "FatLocateDirent\n", 0);

    DebugTrace( 0, Dbg, "  ParentDirectory         = %p\n", ParentDirectory);
    DebugTrace( 0, Dbg, "  OffsetToStartSearchFrom = %08lx\n", OffsetToStartSearchFrom);
    DebugTrace( 0, Dbg, "  Dirent                  = %p\n", Dirent);
    DebugTrace( 0, Dbg, "  Bcb                     = %p\n", Bcb);
    DebugTrace( 0, Dbg, "  ByteOffset              = %08lx\n", *ByteOffset);

    //
    //  We must have acquired the parent or the vcb to synchronize with deletion.  This
    //  is important since we can't survive racing a thread marking a series of lfn
    //  dirents deleted - we'd get a bogus ordinal, and otherwise get really messed up.
    //  
    //  This routine cannot do the acquire since it would be out-of-order with respect
    //  to the Bcb resources on iterative calls.  Our order has Bcbs as the inferior resource.
    //
    //  Deletion always grabs the parent (safely - this used to not be possible until the
    //  multiple fcb lockorder was fixed to be bottom up!).  Deletion always occurs with
    //  the vcb held exclusive as well, and this will cover the cases where we can't easily
    //  hold the parent here, see above.
    //

    NT_ASSERT( ExIsResourceAcquiredSharedLite( ParentDirectory->Header.Resource ) ||
            ExIsResourceAcquiredExclusiveLite( ParentDirectory->Header.Resource ) ||
            ExIsResourceAcquiredSharedLite( &ParentDirectory->Vcb->Resource ) ||
            ExIsResourceAcquiredExclusiveLite( &ParentDirectory->Vcb->Resource ));
    
    //
    //  The algorithm here is pretty simple.  We just walk through the
    //  parent directory until we:
    //
    //      A)  Find a matching entry.
    //      B)  Can't Wait
    //      C)  Hit the End of Directory
    //      D)  Hit Eof
    //
    //  In the first case we found it, in the latter three cases we did not.
    //
 
    UNREFERENCED_PARAMETER( Flags ); // future use


    Name.MaximumLength = 12;
    Name.Buffer = (PCHAR)NameBuffer;

    UpcasedLfn.Length = 0;
    UpcasedLfn.MaximumLength = sizeof( LocalLfnBuffer);
    UpcasedLfn.Buffer = LocalLfnBuffer;


    //
    //  If we were given a non-NULL Bcb, compute the new Dirent address
    //  from the prior one, or unpin the Bcb if the new Dirent is not pinned.
    //

    if (*Bcb != NULL) {

        if ((OffsetToStartSearchFrom / PAGE_SIZE) == (*ByteOffset / PAGE_SIZE)) {

            *Dirent += (OffsetToStartSearchFrom - *ByteOffset) / sizeof(DIRENT);

        } else {

            FatUnpinBcb( IrpContext, *Bcb );
        }
    }

    //
    //  Init the Lfn if we were given one.
    //

    if (ARGUMENT_PRESENT(LongFileName)) {

        LongFileName->Length = 0;
    }

    if (ARGUMENT_PRESENT(OrigLongFileName)) {

        OrigLongFileName->Length = 0;
    }

    //
    //  Init the FileNameDos flag
    //

    if (FileNameDos) {

        *FileNameDos = FALSE;
    }

    //
    //  Round up OffsetToStartSearchFrom to the nearest Dirent, and store
    //  in ByteOffset.  Note that this wipes out the prior value.
    //

    *ByteOffset = (OffsetToStartSearchFrom +  (sizeof(DIRENT) - 1))
                                           & ~(sizeof(DIRENT) - 1);

    _SEH2_TRY {

        while ( TRUE ) {

            BOOLEAN FoundValidLfn;

            UpcasedLfnValid = FALSE;


            //
            //  Try to read in the dirent
            //

            FatReadDirent( IrpContext,
                           ParentDirectory,
                           *ByteOffset,
                           Bcb,
                           Dirent,
                           &Status );

            //
            //  If End Directory dirent or EOF, set all out parameters to
            //  indicate entry not found and, like, bail.
            //
            //  Note that the order of evaluation here is important since we
            //  cannot check the first character of the dirent until after we
            //  know we are not beyond EOF
            //

            if ((Status == STATUS_END_OF_FILE) ||
                ((*Dirent)->FileName[0] == FAT_DIRENT_NEVER_USED)) {

                DebugTrace( 0, Dbg, "End of directory: entry not found.\n", 0);

                //
                //  If there is a Bcb, unpin it and set it to null
                //

                FatUnpinBcb( IrpContext, *Bcb );

                *Dirent = NULL;
                *ByteOffset = 0;
                break;
            }

            //
            //  If the entry is marked deleted, skip.  If there was an Lfn in
            //  progress we throw it out at this point.
            //
            
            if ((*Dirent)->FileName[0] == FAT_DIRENT_DELETED) {

                LfnInProgress = FALSE;
                goto GetNextDirent;
            }

            //
            //  If we have wandered onto an LFN entry, try to interpret it.
            //

            if (FatData.ChicagoMode &&
                ARGUMENT_PRESENT(LongFileName) &&
                ((*Dirent)->Attributes == FAT_DIRENT_ATTR_LFN)) {

                PLFN_DIRENT Lfn;

                Lfn = (PLFN_DIRENT)*Dirent;

                if (LfnInProgress) {

                    //
                    //  Check for a proper continuation of the Lfn in progress.
                    //

                    if ((Lfn->Ordinal & FAT_LAST_LONG_ENTRY) ||
                        (Lfn->Ordinal == 0) ||
                        (Lfn->Ordinal != Ordinal - 1) ||
                        (Lfn->Checksum != LfnChecksum) ||
                        (Lfn->MustBeZero != 0)) {

                        //
                        //  The Lfn is not proper, stop constructing it.
                        //

                        LfnInProgress = FALSE;

                    } else {

                        NT_ASSERT( ((LfnIndex % 13) == 0) && LfnIndex );

                        LfnIndex -= 13;

                        RtlCopyMemory( &LongFileName->Buffer[LfnIndex+0],
                                       &Lfn->Name1[0],
                                       5*sizeof(WCHAR) );

                        RtlCopyMemory( &LongFileName->Buffer[LfnIndex+5],
                                       &Lfn->Name2[0],
                                       6 * sizeof(WCHAR) );

                        RtlCopyMemory( &LongFileName->Buffer[LfnIndex+11],
                                       &Lfn->Name3[0],
                                       2 * sizeof(WCHAR) );

                        Ordinal = Lfn->Ordinal;
                        LfnByteOffset = *ByteOffset;
                    }
                }

                //
                //  Now check (maybe again) if we should analyze this entry
                //  for a possible last entry.
                //

                if ((!LfnInProgress) &&
                    (Lfn->Ordinal & FAT_LAST_LONG_ENTRY) &&
                    ((Lfn->Ordinal & ~FAT_LAST_LONG_ENTRY) <= MAX_LFN_DIRENTS) &&
                    (Lfn->MustBeZero == 0)) {

                    BOOLEAN CheckTail = FALSE;

                    Ordinal = Lfn->Ordinal & ~FAT_LAST_LONG_ENTRY;

                    //
                    //  We're usually permissive (following the lead of Win9x) when we find
                    //  malformation of the LFN dirent pile.  I'm not sure this is a good idea,
                    //  so I'm going to trigger corruption on this particularly ugly one.  Perhaps
                    //  we should come back and redo the original code here with this in mind in the
                    //  future.
                    //

                    if (Ordinal == 0) {

                        //
                        //  First LFN in the pile was zero marked as the last. This is never
                        //  possible since oridinals are 1-based.
                        //

                        FatPopUpFileCorrupt( IrpContext, ParentDirectory );
                        FatRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
                    }

                    LfnIndex = (Ordinal - 1) * 13;

                    FatEnsureStringBufferEnough( LongFileName, 
                                                 (USHORT)((LfnIndex + 13) << 1));

                    RtlCopyMemory( &LongFileName->Buffer[LfnIndex+0],
                                   &Lfn->Name1[0],
                                   5*sizeof(WCHAR));

                    RtlCopyMemory( &LongFileName->Buffer[LfnIndex+5],
                                   &Lfn->Name2[0],
                                   6 * sizeof(WCHAR) );

                    RtlCopyMemory( &LongFileName->Buffer[LfnIndex+11],
                                   &Lfn->Name3[0],
                                   2 * sizeof(WCHAR) );

                    //
                    //  Now compute the Lfn size and make sure that the tail
                    //  bytes are correct.
                    //

                    while (LfnIndex != (ULONG)Ordinal * 13) {

                        if (!CheckTail) {

                            if (LongFileName->Buffer[LfnIndex] == 0x0000) {

                                LfnSize = LfnIndex;
                                CheckTail = TRUE;
                            }

                        } else {

                            if (LongFileName->Buffer[LfnIndex] != 0xffff) {

                                break;
                            }
                        }

                        LfnIndex += 1;
                    }

                    //
                    //  If we exited this loop prematurely, the LFN is not valid.
                    //

                    if (LfnIndex == (ULONG)Ordinal * 13) {

                        //
                        //  If we didn't find the NULL terminator, then the size
                        //  is LfnIndex.
                        //

                        if (!CheckTail) {

                            LfnSize = LfnIndex;
                        }

                        LfnIndex -= 13;
                        LfnInProgress = TRUE;
                        LfnChecksum = Lfn->Checksum;
                        LfnByteOffset = *ByteOffset;
                    }
                }

                //
                //  Move on to the next dirent.
                //

                goto GetNextDirent;
            }

            //
            //  If this is the volume label, skip.  Note that we never arrive here
            //  while building the LFN.  If we did, we weren't asked to find LFNs
            //  and that is another good reason to skip this LFN fragment.
            //

            if (FlagOn((*Dirent)->Attributes, FAT_DIRENT_ATTR_VOLUME_ID)) {

                //
                //  If we actually were asked to hand back volume labels,
                //  do it.
                //
                
                if (FlagOn(Ccb->Flags, CCB_FLAG_MATCH_VOLUME_ID)) {

                    break;
                }

                goto GetNextDirent;
            }

            //
            //  We may have just stepped off a valid Lfn run.  Check to see if
            //  it is indeed valid for the following dirent.
            //

            if (LfnInProgress &&
                (*ByteOffset == LfnByteOffset + sizeof(DIRENT)) &&
                (LfnIndex == 0) &&
                (FatComputeLfnChecksum(*Dirent) == LfnChecksum)) {

                NT_ASSERT( Ordinal == 1);

                FoundValidLfn = TRUE;
                LongFileName->Length = (USHORT)(LfnSize * sizeof(WCHAR));


                if (ARGUMENT_PRESENT(OrigLongFileName)) {
                    *OrigLongFileName = *LongFileName;
                }

            } else {

                FoundValidLfn = FALSE;
            }


            
            //
            //  If we are supposed to match all entries, then match this entry.
            //

            if (FlagOn(Ccb->Flags, CCB_FLAG_MATCH_ALL)) {

                break;
            }

            //
            //  Check against the short name given if one was.
            //

            if (!FlagOn( Ccb->Flags, CCB_FLAG_SKIP_SHORT_NAME_COMPARE )) {

                if (Ccb->ContainsWildCards) {

                    //
                    //  If we get one, note that all out parameters are already set.
                    //

                    (VOID)Fat8dot3ToString( IrpContext, (*Dirent), FALSE, &Name );

                    //
                    //  For fat we special case the ".." dirent because we want it to
                    //  match ????????.??? and to do that we change ".." to "." before
                    //  calling the Fsrtl routine.  But only do this if the expression
                    //  is greater than one character long.
                    //

                    if ((Name.Length == 2) &&
                        (Name.Buffer[0] == '.') &&
                        (Name.Buffer[1] == '.') &&
                        (Ccb->OemQueryTemplate.Wild.Length > 1)) {

                        Name.Length = 1;
                    }

                    if (FatIsNameInExpression( IrpContext,
                                               Ccb->OemQueryTemplate.Wild,
                                               Name)) {

                        DebugTrace( 0, Dbg, "Entry found: Name = \"%Z\"\n", &Name);
                        DebugTrace( 0, Dbg, "             VBO  = %08lx\n", *ByteOffset);

                        if (FileNameDos) {

                            *FileNameDos = TRUE;
                        }

                        SetFlag( Ccb->Flags, CCB_FLAG_OPENED_BY_SHORTNAME );

                        break;
                    }

                } else {

                    //
                    //  Do the quickest 8.3 equivalency check possible
                    //

                    if (!FlagOn((*Dirent)->Attributes, FAT_DIRENT_ATTR_VOLUME_ID) &&
                        (*(PULONG)&(Ccb->OemQueryTemplate.Constant[0]) == *(PULONG)&((*Dirent)->FileName[0])) &&
                        (*(PULONG)&(Ccb->OemQueryTemplate.Constant[4]) == *(PULONG)&((*Dirent)->FileName[4])) &&
                        (*(PUSHORT)&(Ccb->OemQueryTemplate.Constant[8]) == *(PUSHORT)&((*Dirent)->FileName[8])) &&
                        (*(PUCHAR)&(Ccb->OemQueryTemplate.Constant[10]) == *(PUCHAR)&((*Dirent)->FileName[10]))) {

                        DebugTrace( 0, Dbg, "Entry found.\n", 0);

                        if (FileNameDos) {

                            *FileNameDos = TRUE;
                        }

                        SetFlag( Ccb->Flags, CCB_FLAG_OPENED_BY_SHORTNAME );

                        break;
                    }
                }
            }

            //
            //  No matches were found with the short name.  If an LFN exists,
            //  use it for the search.
            //

            if (FoundValidLfn) {


                //
                //  First do a quick check here for different sized constant
                //  name and expression before upcasing.
                //

                if (!Ccb->ContainsWildCards &&
                    (Ccb->UnicodeQueryTemplate.Length != (USHORT)(LfnSize * sizeof(WCHAR)))) {

                    //
                    //  Move on to the next dirent.
                    //

                    FoundValidLfn = FALSE;
                    LongFileName->Length = 0;
                    if (OrigLongFileName) {
                        OrigLongFileName->Length = 0;
                    }

                    goto GetNextDirent;
                }



                if (!UpcasedLfnValid) {

                    //
                    //  We need to upcase the name we found on disk.
                    //  We need a buffer.  Try to avoid doing an allocation.
                    //

                    FatEnsureStringBufferEnough( &UpcasedLfn, 
                                                 LongFileName->Length);

                    Status = RtlUpcaseUnicodeString( &UpcasedLfn,
                                                     LongFileName,
                                                     FALSE );

                    if (!NT_SUCCESS(Status)) {

                        FatNormalizeAndRaiseStatus( IrpContext, Status );
                    }


                    UpcasedLfnValid = TRUE;
                    
                }

                //
                //  Do the compare
                //

                if (Ccb->ContainsWildCards) {

                    if (FsRtlIsNameInExpression( &Ccb->UnicodeQueryTemplate,
                                                 &UpcasedLfn,
                                                 TRUE,
                                                 NULL )) {

                        break;
                    }

                } else {

                    if (FsRtlAreNamesEqual( &Ccb->UnicodeQueryTemplate,
                                            &UpcasedLfn,
                                            BooleanFlagOn( Ccb->Flags, CCB_FLAG_QUERY_TEMPLATE_MIXED ),
                                            NULL )) {

                        break;
                    }

                    
                }

            }

            //
            //  This long name was not a match.  Zero out the Length field.
            //

            if (FoundValidLfn) {

                FoundValidLfn = FALSE;
                LongFileName->Length = 0;


                if (OrigLongFileName) {
                    OrigLongFileName->Length = 0;
                }
            }

GetNextDirent:

            //
            //  Move on to the next dirent.
            //

            *ByteOffset += sizeof(DIRENT);
            *Dirent += 1;
        }

    } _SEH2_FINALLY {

        FatFreeStringBuffer( &UpcasedLfn );

        
    } _SEH2_END;

    DebugTrace(-1, Dbg, "FatLocateDirent -> (VOID)\n", 0);

    TimerStop(Dbg,"FatLocateDirent");

    return;
}


_Requires_lock_held_(_Global_critical_region_)    
VOID
FatLocateSimpleOemDirent (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB ParentDirectory,
    IN POEM_STRING FileName,
    OUT PDIRENT *Dirent,
    OUT PBCB *Bcb,
    OUT PVBO ByteOffset
    )

/*++

Routine Description:

    This routine locates on the disk an undelted simple Oem dirent.  By simple
    I mean that FileName cannot contain any extended characters, and we do
    not search LFNs or return them.

Arguments:

    ParentDirectory - Supplies the DCB for the directory in which
        to search

    FileName - Supplies the filename to search for.  The name may contain
        wild cards

    OffsetToStartSearchFrom - Supplies the VBO within the parent directory
        from which to start looking for another real dirent.

    Dirent - Receives a pointer to the located dirent if one was found
        or NULL otherwise.

    Bcb - Receives the Bcb for the located dirent if one was found or
        NULL otherwise.

    ByteOffset - Receives the VBO within the Parent directory for
        the located dirent if one was found, or 0 otherwise.

Return Value:

    None.

--*/

{
    CCB LocalCcb;

    PAGED_CODE();

    //
    //  Note, this routine is called rarely, so performance is not critical.
    //  Just fill in a Ccb structure on my stack with the values that are
    //  required.
    //

    FatStringTo8dot3( IrpContext,
                      *FileName,
                      &LocalCcb.OemQueryTemplate.Constant );
    LocalCcb.ContainsWildCards = FALSE;
    LocalCcb.Flags = 0;

    FatLocateDirent( IrpContext,
                     ParentDirectory,
                     &LocalCcb,
                     0,
                     NULL,
                     Dirent,
                     Bcb,
                     ByteOffset,
                     NULL,
                     NULL,
                     NULL );

    return;
}



_Requires_lock_held_(_Global_critical_region_)
VOID
FatLocateVolumeLabel (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    OUT PDIRENT *Dirent,
    OUT PBCB *Bcb,
    OUT PVBO ByteOffset
    )

/*++

Routine Description:

    This routine locates on the disk a dirent representing the volume
    label.  It does this by searching the root directory for a special
    volume label dirent.

Arguments:

    Vcb - Supplies the VCB for the volume to search

    Dirent - Receives a pointer to the located dirent if one was found
        or NULL otherwise.

    Bcb - Receives the Bcb for the located dirent if one was found or
        NULL otherwise.

    ByteOffset - Receives the VBO within the Parent directory for
        the located dirent if one was found, or 0 otherwise.

Return Value:

    None.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "FatLocateVolumeLabel\n", 0);

    DebugTrace( 0, Dbg, "  Vcb        = %p\n", Vcb);
    DebugTrace( 0, Dbg, "  Dirent     = %p\n", Dirent);
    DebugTrace( 0, Dbg, "  Bcb        = %p\n", Bcb);
    DebugTrace( 0, Dbg, "  ByteOffset = %08lx\n", *ByteOffset);

    //
    //  The algorithm here is really simple.  We just walk through the
    //  root directory until we:
    //
    //      A)  Find the non-deleted volume label
    //      B)  Can't Wait
    //      C)  Hit the End of Directory
    //      D)  Hit Eof
    //
    //  In the first case we found it, in the latter three cases we did not.
    //

    *Bcb = NULL;
    *ByteOffset = 0;

    while ( TRUE ) {

        //
        //  Try to read in the dirent
        //

        FatReadDirent( IrpContext,
                       Vcb->RootDcb,
                       *ByteOffset,
                       Bcb,
                       Dirent,
                       &Status );

        //
        //  If End Directory dirent or EOF, set all out parameters to
        //  indicate volume label not found and, like, bail.
        //
        //  Note that the order of evaluation here is important since we cannot
        //  check the first character of the dirent until after we know we
        //  are not beyond EOF
        //

        if ((Status == STATUS_END_OF_FILE) ||
            ((*Dirent)->FileName[0] == FAT_DIRENT_NEVER_USED)) {

            DebugTrace( 0, Dbg, "Volume label not found.\n", 0);

            //
            //  If there is a Bcb, unpin it and set it to null
            //

            FatUnpinBcb( IrpContext, *Bcb );

            *Dirent = NULL;
            *ByteOffset = 0;
            break;
        }

        //
        //  If the entry is the non-deleted volume label break from the loop.
        //
        //  Note that all out parameters are already correctly set.
        //

        if ((((*Dirent)->Attributes & ~FAT_DIRENT_ATTR_ARCHIVE) == FAT_DIRENT_ATTR_VOLUME_ID) &&
            ((*Dirent)->FileName[0] != FAT_DIRENT_DELETED)) {

            DebugTrace( 0, Dbg, "Volume label found at VBO = %08lx\n", *ByteOffset);

            //
            //  We may set this dirty, so pin it.
            //

            FatPinMappedData( IrpContext,
                              Vcb->RootDcb,
                              *ByteOffset,
                              sizeof(DIRENT),
                              Bcb );

            break;
        }

        //
        //  Move on to the next dirent.
        //

        *ByteOffset += sizeof(DIRENT);
        *Dirent += 1;
    }


    DebugTrace(-1, Dbg, "FatLocateVolumeLabel -> (VOID)\n", 0);

    return;
}



_Requires_lock_held_(_Global_critical_region_)
VOID
FatGetDirentFromFcbOrDcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB FcbOrDcb,
    IN BOOLEAN ReturnOnFailure,
    OUT PDIRENT *Dirent,
    OUT PBCB *Bcb
    )

/*++

Routine Description:

    This routine reads locates on the disk the dirent denoted by the
    specified Fcb/Dcb.

Arguments:

    FcbOrDcb - Supplies the FCB/DCB for the file/directory whose dirent
        we are trying to read in.  This must not be the root dcb.

    Dirent - Receives a pointer to the dirent

    Bcb - Receives the Bcb for the dirent

Return Value:

    None.

--*/

{
    NTSTATUS DontCare = STATUS_SUCCESS;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "FatGetDirentFromFcbOrDcb\n", 0);

    DebugTrace( 0, Dbg, "  FcbOrDcb = %p\n", FcbOrDcb);
    DebugTrace( 0, Dbg, "  Dirent   = %p\n", Dirent);
    DebugTrace( 0, Dbg, "  Bcb      = %p\n", Bcb);

    //
    //  Assert that we are not attempting this on the root directory.
    //

    NT_ASSERT( NodeType(FcbOrDcb) != FAT_NTC_ROOT_DCB );

    //
    //  We know the offset of the dirent within the directory file,
    //  so we just read it (with pinning).
    //

    FatReadDirectoryFile( IrpContext,
                          FcbOrDcb->ParentDcb,
                          FcbOrDcb->DirentOffsetWithinDirectory,
                          sizeof(DIRENT),
                          TRUE,
                          Bcb,
                          (PVOID *)Dirent,
                          &DontCare );

    //
    //  Previous call can fail.  We used to assert success, but we use this
    //  as part of volume verification (DetermineAndMarkFcbCondition) after
    //  media has been removed.  Clearly the directory could shrink and we
    //  would try to read beyond filesize.
    //
    //  The caller will note this via NULL pointers for Bcb/Buffer.  Note that
    //  both asserts below are OK since this should never happen fixed media.
    //
    //  This was a Prefix catch.
    //

    NT_ASSERT( FlagOn( FcbOrDcb->Vcb->VcbState, VCB_STATE_FLAG_REMOVABLE_MEDIA) ||
            NT_SUCCESS( DontCare ));

    //
    //  Note also that the only way this could fail is if the Fcb was being
    //  verified.  This can't happen if the Fcb is in good condition.
    //
    //  Also a Prefix catch.
    //

    NT_ASSERT( NT_SUCCESS( DontCare ) || FcbOrDcb->FcbCondition == FcbNeedsToBeVerified );

    //
    //  This should never happen except in very specific cases (during volume
    //  verify) but we'll handle and raise here to save all callers checking the
    //  pointers.
    //
    
    if ((NULL == *Dirent) && !ReturnOnFailure) {

        NT_ASSERT( FALSE);
        FatRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR);
    }
    
    DebugTrace(-1, Dbg, "FatGetDirentFromFcbOrDcb -> (VOID)\n", 0);
}



_Requires_lock_held_(_Global_critical_region_)
BOOLEAN
FatIsDirectoryEmpty (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb
    )

/*++

Routine Description:

    This routine indicates to the caller if the specified directory
    is empty.  (i.e., it is not the root dcb and it only contains
    the "." and ".." entries, or deleted files).

Arguments:

    Dcb - Supplies the DCB for the directory being queried.

Return Value:

    BOOLEAN - Returns TRUE if the directory is empty and
        FALSE if the directory and is not empty.

--*/

{
    PBCB Bcb;
    ULONG ByteOffset;
    PDIRENT Dirent = NULL;

    BOOLEAN IsDirectoryEmpty = FALSE;

    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "FatIsDirectoryEmpty\n", 0);

    DebugTrace( 0, Dbg, "  Dcb              = %p\n", Dcb);
    DebugTrace( 0, Dbg, "  IsDirectoryEmpty = %08lx\n", IsDirectoryEmpty);

    //
    //  Check to see if the first entry is an and of directory marker.
    //  For the root directory we check at Vbo = 0, for normal directories
    //  we check after the "." and ".." entries.
    //

    ByteOffset = (NodeType(Dcb) == FAT_NTC_ROOT_DCB) ? 0 : 2*sizeof(DIRENT);

    //
    //  We just march through the directory looking for anything other
    //  than deleted files, LFNs, an EOF, or end of directory marker.
    //

    Bcb = NULL;

    _SEH2_TRY {

        while ( TRUE ) {

            //
            //  Try to read in the dirent
            //

            FatReadDirent( IrpContext,
                           Dcb,
                           ByteOffset,
                           &Bcb,
                           &Dirent,
                           &Status );

            //
            //  If End Directory dirent or EOF, set IsDirectoryEmpty to TRUE and,
            //  like, bail.
            //
            //  Note that the order of evaluation here is important since we cannot
            //  check the first character of the dirent until after we know we
            //  are not beyond EOF
            //

            if ((Status == STATUS_END_OF_FILE) ||
                (Dirent->FileName[0] == FAT_DIRENT_NEVER_USED)) {

                DebugTrace( 0, Dbg, "Empty.  Last exempt entry at VBO = %08lx\n", ByteOffset);

                IsDirectoryEmpty = TRUE;
                break;
            }

            //
            //  If this dirent is NOT deleted or an LFN set IsDirectoryEmpty to
            //  FALSE and, like, bail.
            //

            if ((Dirent->FileName[0] != FAT_DIRENT_DELETED) &&
                (Dirent->Attributes != FAT_DIRENT_ATTR_LFN)) {

                
                    break;

                
            }

            //
            //  Move on to the next dirent.
            //

            ByteOffset += sizeof(DIRENT);
            Dirent += 1;
        }

    } _SEH2_FINALLY {

        FatUnpinBcb( IrpContext, Bcb );
    } _SEH2_END;

    DebugTrace(-1, Dbg, "FatIsDirectoryEmpty -> %ld\n", IsDirectoryEmpty);

    return IsDirectoryEmpty;
}





VOID
FatConstructDirent (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PDIRENT Dirent,
    IN POEM_STRING FileName,
    IN BOOLEAN ComponentReallyLowercase,
    IN BOOLEAN ExtensionReallyLowercase,
    IN PUNICODE_STRING Lfn OPTIONAL,
    IN USHORT Attributes,
    IN BOOLEAN ZeroAndSetTimeFields,
    IN PLARGE_INTEGER SetCreationTime OPTIONAL
    )

/*++

Routine Description:

    This routine modifies the fields of a dirent.

Arguments:

    Dirent - Supplies the dirent being modified.

    FileName - Supplies the name to store in the Dirent.  This
        name must not contain wildcards.

    ComponentReallyLowercase - This boolean indicates that the User Specified
        compoent name was really all a-z and < 0x80 characters.  We set the
        magic bit in this case.

    ExtensionReallyLowercase - Same as above, but for the extension.

    Lfn - May supply a long file name.

    Attributes - Supplies the attributes to store in the dirent

    ZeroAndSetTimeFields - Tells whether or not to initially zero the dirent
        and update the time fields.

    SetCreationTime - If specified, contains a timestamp to use as the creation
        time of this dirent

Return Value:

    None.

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "FatConstructDirent\n", 0);

    DebugTrace( 0, Dbg, "  Dirent             = %p\n", Dirent);
    DebugTrace( 0, Dbg, "  FileName           = %Z\n", FileName);
    DebugTrace( 0, Dbg, "  Attributes         = %08lx\n", Attributes);

    if (ZeroAndSetTimeFields) {

        RtlZeroMemory( Dirent, sizeof(DIRENT) );
    }

    //
    //  We just merrily go and fill up the dirent with the fields given.
    //

    FatStringTo8dot3( IrpContext, *FileName, (PFAT8DOT3)&Dirent->FileName[0] );

    if (ZeroAndSetTimeFields || SetCreationTime) {

        LARGE_INTEGER Time, SaveTime;

        KeQuerySystemTime( &Time );

        if (FatData.ChicagoMode) {

            if (!SetCreationTime || !FatNtTimeToFatTime( IrpContext,
                                                         SetCreationTime,
                                                         FALSE,
                                                         &Dirent->CreationTime,
                                                         &Dirent->CreationMSec )) {
        
                //
                //  No tunneled time or the tunneled time was bogus. Since we aren't
                //  responsible for initializing the to-be-created Fcb with creation
                //  time, we can't do the usual thing and let NtTimeToFatTime perform
                //  rounding on the timestamp - this would mess up converting to the
                //  LastWriteTime below.
                //

                SaveTime = Time;

                if (!FatNtTimeToFatTime( IrpContext,
                                         &SaveTime,
                                         FALSE,
                                         &Dirent->CreationTime,
                                         &Dirent->CreationMSec )) {

                    //
                    //  Failed again. Wow.
                    //

                    RtlZeroMemory( &Dirent->CreationTime, sizeof(FAT_TIME_STAMP));
                    Dirent->CreationMSec = 0;
                }
            }
        }

        if (ZeroAndSetTimeFields) {

            //
            //  We only touch the other timestamps if we are initializing the dirent
            //

            if (!FatNtTimeToFatTime( IrpContext,
                                     &Time,
                                     TRUE,
                                     &Dirent->LastWriteTime,
                                     NULL )) {

                DebugTrace( 0, Dbg, "Current time invalid.\n", 0);

                RtlZeroMemory( &Dirent->LastWriteTime, sizeof(FAT_TIME_STAMP) );
            }

            if (FatData.ChicagoMode) {

                Dirent->LastAccessDate = Dirent->LastWriteTime.Date;
            }
        }
    }

    //
    //  Copy the attributes
    //

    Dirent->Attributes = (UCHAR)Attributes;

    //
    //  Set the magic bit here, to tell dirctrl.c that this name is really
    //  lowercase.
    //

    Dirent->NtByte = 0;

    if (ComponentReallyLowercase) {

        SetFlag( Dirent->NtByte, FAT_DIRENT_NT_BYTE_8_LOWER_CASE );
    }

    if (ExtensionReallyLowercase) {

        SetFlag( Dirent->NtByte, FAT_DIRENT_NT_BYTE_3_LOWER_CASE );
    }

    //
    //  See if we have to create an Lfn entry
    //

    if (ARGUMENT_PRESENT(Lfn)) {

        UCHAR DirentChecksum;
        UCHAR DirentsInLfn;
        UCHAR LfnOrdinal;
        PWCHAR LfnBuffer;
        PLFN_DIRENT LfnDirent;

        NT_ASSERT( FatData.ChicagoMode );

        DirentChecksum = FatComputeLfnChecksum( Dirent );

        LfnOrdinal =
        DirentsInLfn = (UCHAR)FAT_LFN_DIRENTS_NEEDED(Lfn);

        LfnBuffer = &Lfn->Buffer[(DirentsInLfn - 1) * 13];

        NT_ASSERT( DirentsInLfn <= MAX_LFN_DIRENTS );

        for (LfnDirent = (PLFN_DIRENT)Dirent - DirentsInLfn;
             LfnDirent < (PLFN_DIRENT)Dirent;
             LfnDirent += 1, LfnOrdinal -= 1, LfnBuffer -= 13) {

            WCHAR FinalLfnBuffer[13];
            PWCHAR Buffer;

            //
            //  We need to special case the "final" dirent.
            //

            if (LfnOrdinal == DirentsInLfn) {

                ULONG i;
                ULONG RemainderChars;

                RemainderChars = (Lfn->Length / sizeof(WCHAR)) % 13;

                LfnDirent->Ordinal = LfnOrdinal | FAT_LAST_LONG_ENTRY;

                if (RemainderChars != 0) {

                    RtlCopyMemory( FinalLfnBuffer,
                                   LfnBuffer,
                                   RemainderChars * sizeof(WCHAR) );

                    for (i = RemainderChars; i < 13; i++) {

                        //
                        //  Figure out which character to use.
                        //

                        if (i == RemainderChars) {

                            FinalLfnBuffer[i] = 0x0000;

                        } else {

                            FinalLfnBuffer[i] = 0xffff;
                        }
                    }

                    Buffer = FinalLfnBuffer;

                } else {

                    Buffer = LfnBuffer;
                }

            } else {

                LfnDirent->Ordinal = LfnOrdinal;

                Buffer = LfnBuffer;
            }

            //
            //  Now fill in the name.
            //

            RtlCopyMemory( &LfnDirent->Name1[0],
                           &Buffer[0],
                           5 * sizeof(WCHAR) );

            RtlCopyMemory( &LfnDirent->Name2[0],
                           &Buffer[5],
                           6 * sizeof(WCHAR) );

            RtlCopyMemory( &LfnDirent->Name3[0],
                           &Buffer[11],
                           2 * sizeof(WCHAR) );

            //
            //  And the other fields
            //

            LfnDirent->Attributes = FAT_DIRENT_ATTR_LFN;

            LfnDirent->Type = 0;

            LfnDirent->Checksum = DirentChecksum;

            LfnDirent->MustBeZero = 0;
        }
    }

    DebugTrace(-1, Dbg, "FatConstructDirent -> (VOID)\n", 0);
    return;
}


VOID
FatConstructLabelDirent (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PDIRENT Dirent,
    IN POEM_STRING Label
    )

/*++

Routine Description:

    This routine modifies the fields of a dirent to be used for a label.

Arguments:

    Dirent - Supplies the dirent being modified.

    Label - Supplies the name to store in the Dirent.  This
            name must not contain wildcards.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "FatConstructLabelDirent\n", 0);

    DebugTrace( 0, Dbg, "  Dirent             = %p\n", Dirent);
    DebugTrace( 0, Dbg, "  Label              = %Z\n", Label);

    RtlZeroMemory( Dirent, sizeof(DIRENT) );

    //
    //  We just merrily go and fill up the dirent with the fields given.
    //

    RtlCopyMemory( Dirent->FileName, Label->Buffer, Label->Length );

    //
    // Pad the label with spaces, not nulls.
    //

    RtlFillMemory( &Dirent->FileName[Label->Length], 11 - Label->Length, ' ');

    Dirent->LastWriteTime = FatGetCurrentFatTime( IrpContext );

    Dirent->Attributes = FAT_DIRENT_ATTR_VOLUME_ID;
    Dirent->ExtendedAttributes = 0;
    Dirent->FileSize = 0;

    DebugTrace(-1, Dbg, "FatConstructLabelDirent -> (VOID)\n", 0);
    return;
}


_Requires_lock_held_(_Global_critical_region_)    
VOID
FatSetFileSizeInDirent (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PULONG AlternativeFileSize OPTIONAL
    )

/*++

Routine Description:

    This routine saves the file size in an fcb into its dirent.

Arguments:

    Fcb - Supplies the Fcb being referenced

    AlternativeFileSize - If non-null we use the ULONG it points to as
        the new file size.  Otherwise we use the one in the Fcb.

Return Value:

    None.

--*/

{
    PDIRENT Dirent;
    PBCB DirentBcb;

    PAGED_CODE();

    NT_ASSERT( Fcb->FcbCondition == FcbGood );

    FatGetDirentFromFcbOrDcb( IrpContext,
                              Fcb,
                              FALSE,
                              &Dirent,
                              &DirentBcb );
    _SEH2_TRY {

        Dirent->FileSize = ARGUMENT_PRESENT( AlternativeFileSize ) ?
                           *AlternativeFileSize : Fcb->Header.FileSize.LowPart;


        FatSetDirtyBcb( IrpContext, DirentBcb, Fcb->Vcb, TRUE );

    } _SEH2_FINALLY {

        FatUnpinBcb( IrpContext, DirentBcb );
    } _SEH2_END;
}

_Requires_lock_held_(_Global_critical_region_)    
VOID
FatSetFileSizeInDirentNoRaise (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PULONG AlternativeFileSize OPTIONAL
    )

/*++

Routine Description:

    This routine saves the file size in an fcb into its dirent.
    All exceptions thrown from FatSetFileSizeInDirent are
    silently swallowed.

Arguments:

    Fcb - Supplies the Fcb being referenced

    AlternativeFileSize - If non-null we use the ULONG it points to as
        the new file size.  Otherwise we use the one in the Fcb.

Return Value:

    None.

--*/

{
    _SEH2_TRY {
    
        FatSetFileSizeInDirent( IrpContext, Fcb, AlternativeFileSize );
        
    } _SEH2_EXCEPT(FatExceptionFilter( IrpContext, _SEH2_GetExceptionInformation() )) {

        NOTHING;
    } _SEH2_END;
}


_Requires_lock_held_(_Global_critical_region_)    
VOID
FatUpdateDirentFromFcb (
   IN PIRP_CONTEXT IrpContext,
   IN PFILE_OBJECT FileObject,
   IN PFCB FcbOrDcb,
   IN PCCB Ccb
   )


/*++

Routine Description:

    This routine modifies an objects directory entry based on the hints
    that have been built up over previous operations on a handle.  Notify
    change filters are built and fired as a result of these updates.

Arguments:

    FileObject - Fileobject representing the handle involved
    
    FcbOrDcb - File/Dir involved
    
    Ccb - User context involved

Return Value:

    None.

--*/

{
    BOOLEAN SetArchiveBit;

    BOOLEAN UpdateFileSize;
    BOOLEAN UpdateLastWriteTime;
    BOOLEAN UpdateLastAccessTime;
    BOOLEAN UpdateDirent = FALSE;

    PDIRENT Dirent;
    PBCB DirentBcb = NULL;
    ULONG NotifyFilter = 0;
    FAT_TIME_STAMP CurrentFatTime = {0};

    LARGE_INTEGER CurrentTime;
    LARGE_INTEGER CurrentDay = {0};
    LARGE_INTEGER LastAccessDay;

    PAGED_CODE();

    //
    //  Nothing to do if the fcb is bad, volume is readonly or we got the
    //  root dir.
    //
    
    if (FcbOrDcb->FcbCondition != FcbGood ||
        NodeType(FcbOrDcb) == FAT_NTC_ROOT_DCB ||
        FlagOn(FcbOrDcb->Vcb->VcbState, VCB_STATE_FLAG_WRITE_PROTECTED)) {

        return;
    }

    //
    //  Check if we should be changing the time or file size and set
    //  the archive bit on the file.
    //

    KeQuerySystemTime( &CurrentTime );

    //
    //  Note that we HAVE to use BooleanFlagOn() here because
    //  FO_FILE_SIZE_CHANGED > 0x80 (i.e., not in the first byte).
    //

    SetArchiveBit = BooleanFlagOn(FileObject->Flags, FO_FILE_MODIFIED);

    UpdateLastWriteTime = FlagOn(FileObject->Flags, FO_FILE_MODIFIED) &&
                          !FlagOn(Ccb->Flags, CCB_FLAG_USER_SET_LAST_WRITE);

    UpdateFileSize = NodeType(FcbOrDcb) == FAT_NTC_FCB &&
                     BooleanFlagOn(FileObject->Flags, FO_FILE_SIZE_CHANGED);

    //
    //  Do one further check here of access time.  Only update it if
    //  the current version is at least one day old.  We know that
    //  the current FcbOrDcb->LastAccessTime corresponds to 12 midnight local
    //  time, so just see if the current time is on the same day.
    //

    if (FatData.ChicagoMode &&
        (UpdateLastWriteTime ||
         FlagOn(FileObject->Flags, FO_FILE_FAST_IO_READ)) &&
        !FlagOn(Ccb->Flags, CCB_FLAG_USER_SET_LAST_ACCESS)) {

        ExSystemTimeToLocalTime( &FcbOrDcb->LastAccessTime, &LastAccessDay );
        ExSystemTimeToLocalTime( &CurrentTime, &CurrentDay );

        LastAccessDay.QuadPart /= FatOneDay.QuadPart;
        CurrentDay.QuadPart /= FatOneDay.QuadPart;

        if (LastAccessDay.LowPart != CurrentDay.LowPart) {

            UpdateLastAccessTime = TRUE;

        } else {

            UpdateLastAccessTime = FALSE;
        }

    } else {

        UpdateLastAccessTime = FALSE;
    }

    if (SetArchiveBit ||
        UpdateFileSize ||
        UpdateLastWriteTime ||
        UpdateLastAccessTime
        ) {

        DebugTrace(0, Dbg, "Update Time and/or file size on File/Dir\n", 0);

        _SEH2_TRY {

            _SEH2_TRY {

#if (NTDDI_VERSION >= NTDDI_WIN8)
                //
                //  Break parent directory oplock.  Directory oplock breaks are
                //  always advisory, so we will never block/get STATUS_PENDING here.
                //
                
                if (FcbOrDcb->ParentDcb != NULL) {

                    FsRtlCheckOplockEx( FatGetFcbOplock(FcbOrDcb->ParentDcb),
                                        IrpContext->OriginatingIrp,
                                        OPLOCK_FLAG_PARENT_OBJECT,
                                        NULL,
                                        NULL,
                                        NULL );
                }
#endif
                
                //
                //  Get the dirent
                //

                FatGetDirentFromFcbOrDcb( IrpContext,
                                          FcbOrDcb,
                                          FALSE,
                                          &Dirent,
                                          &DirentBcb );

                if (UpdateLastWriteTime || UpdateLastAccessTime) {

                    (VOID)FatNtTimeToFatTime( IrpContext,
                                              &CurrentTime,
                                              TRUE,
                                              &CurrentFatTime,
                                              NULL );
                }

                if (SetArchiveBit) {

                    Dirent->Attributes |= FILE_ATTRIBUTE_ARCHIVE;
                    FcbOrDcb->DirentFatFlags |= FILE_ATTRIBUTE_ARCHIVE;
                    
                    NotifyFilter |= FILE_NOTIFY_CHANGE_ATTRIBUTES;
                    UpdateDirent = TRUE;
                }

                if (UpdateLastWriteTime) {

                    //
                    //  Update its time of last write
                    //

                    FcbOrDcb->LastWriteTime = CurrentTime;
                    Dirent->LastWriteTime = CurrentFatTime;

                    //
                    //  We call the notify package to report that the
                    //  last modification time has changed.
                    //

                    NotifyFilter |= FILE_NOTIFY_CHANGE_LAST_WRITE;
                    UpdateDirent = TRUE;
                }

                if (UpdateLastAccessTime) {

                    //
                    //  Now we have to truncate the local time down
                    //  to the current day, then convert back to UTC.
                    //

                    FcbOrDcb->LastAccessTime.QuadPart =
                        CurrentDay.QuadPart * FatOneDay.QuadPart;

                    ExLocalTimeToSystemTime( &FcbOrDcb->LastAccessTime,
                                             &FcbOrDcb->LastAccessTime );

                    Dirent->LastAccessDate = CurrentFatTime.Date;

                    //
                    //  We call the notify package to report that the
                    //  last access time has changed.
                    //

                    NotifyFilter |= FILE_NOTIFY_CHANGE_LAST_ACCESS;
                    UpdateDirent = TRUE;
                }

                if (UpdateFileSize) {

                    //
                    //  Perhaps we were called to make certain that the
                    //  filesize on disc was updated - don't bother updating
                    //  and firing the filter if nothing changed.
                    //

                    NT_ASSERT( NodeType(FcbOrDcb) == FAT_NTC_FCB );
                    
                    if (Dirent->FileSize != FcbOrDcb->Header.FileSize.LowPart) {
                    
                        //
                        //  Update the dirent file size
                        //
                        
                        Dirent->FileSize = FcbOrDcb->Header.FileSize.LowPart;
                        
                        //
                        //  We call the notify package to report that the
                        //  size has changed.
                        //

                        NotifyFilter |= FILE_NOTIFY_CHANGE_SIZE;
                        UpdateDirent = TRUE;
                    }                  

            
                }


                FatNotifyReportChange( IrpContext,
                                       FcbOrDcb->Vcb,
                                       FcbOrDcb,
                                       NotifyFilter,
                                       FILE_ACTION_MODIFIED );

                if (UpdateDirent) {

                    //
                    //  If all we did was update last access time,
                    //  don't mark the volume dirty.
                    //

                    FatSetDirtyBcb( IrpContext,
                                    DirentBcb,
                                    NotifyFilter == FILE_NOTIFY_CHANGE_LAST_ACCESS ?
                                    NULL : FcbOrDcb->Vcb,
                                    TRUE );
                }

            } _SEH2_EXCEPT( FsRtlIsNtstatusExpected(_SEH2_GetExceptionCode()) ?
                      EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH ) {

                  FatResetExceptionState( IrpContext );
            } _SEH2_END;

        } _SEH2_FINALLY {

            FatUnpinBcb( IrpContext, DirentBcb );
        } _SEH2_END;
    }

}


//
//  Internal support routine
//

UCHAR
FatComputeLfnChecksum (
    PDIRENT Dirent
    )

/*++

Routine Description:

    This routine computes the Chicago long file name checksum.

Arguments:

    Dirent - Specifies the dirent that we are to compute a checksum for.

Return Value:

    The checksum.

--*/

{
    ULONG i;
    UCHAR Checksum;

    PAGED_CODE();

    Checksum = Dirent->FileName[0];

    for (i=1; i < 11; i++) {

        Checksum = ((Checksum & 1) ? 0x80 : 0) +
                    (Checksum >> 1) +
                    Dirent->FileName[i];
    }

    return Checksum;
}



#if 0 // It turns out Win95 is still creating short names without a ~

//
//  Internal support routine
//

BOOLEAN
FatIsLfnPairValid (
    PWCHAR Lfn,
    ULONG LfnSize,
    PDIRENT Dirent
    )

/*++

Routine Description:

    This routine does a few more checks to make sure that a LFN/short
    name pairing is legitimate.  Basically this is the test:

        Pairing is valid if:

        DIRENT has a ~ character ||
        (LFN is 8.3 compliant &&
         (LFN has extended character(s) ? TRUE :
          LFN upcases to DIRENT))

    When checking for the presence of a tilda character in the short
    name, note that we purposely do a single byte search instead of
    converting the name to UNICODE and looking there for the tilda.
    This protects us from accidently missing the tilda if the
    preceding byte is a lead byte in the current Oem code page,
    but wasn't in the Oem code page that created the file.

    Also note that if the LFN is longer than 12 characters, then the
    second clause of the OR must be false.

Arguments:

    Lfn - Points to a buffer of UNICODE chars.

    LfnSize - This is the size of the LFN in characters.

    Dirent - Specifies the dirent we are to consider.

Return Value:

    TRUE if the Lfn/DIRENT form a legitimate pair, FALSE otherwise.

--*/

{
    ULONG i;
    BOOLEAN ExtendedChars;
    ULONG DirentBuffer[3];
    PUCHAR DirentName;
    ULONG DirentIndex;
    BOOLEAN DotEncountered;

    //
    //  First, look for a tilda
    //

    for (i=0; i<11; i++) {
        if (Dirent->FileName[i] == '~') {
            return TRUE;
        }
    }

    //
    //  No tilda.  If the LFN is longer than 12 characters, then it can
    //  neither upcase to the DIRENT nor be 8.3 complient.
    //

    if (LfnSize > 12) {
        return FALSE;
    }

    //
    //  Now see if the name is 8.3, and build an upcased DIRENT as well.
    //

    DirentBuffer[0] = 0x20202020;
    DirentBuffer[1] = 0x20202020;
    DirentBuffer[2] = 0x20202020;

    DirentName = (PUCHAR)DirentBuffer;

    ExtendedChars = FALSE;
    DirentIndex = 0;
    DotEncountered = FALSE;

    for (i=0; i < LfnSize; i++) {

        //
        //  Do dot transition work
        //

        if (Lfn[i] == L'.') {
            if (DotEncountered ||
                (i > 8) ||
                ((LfnSize - i) > 4) ||
                (i && Lfn[i-1] == L' ')) {
                return FALSE;
            }
            DotEncountered = TRUE;
            DirentIndex = 8;
            continue;
        }

        //
        //  The character must be legal in order to be 8.3
        //

        if ((Lfn[i] < 0x80) &&
            !FsRtlIsAnsiCharacterLegalFat((UCHAR)Lfn[i], FALSE)) {
            return FALSE;
        }

        //
        //  If the name contains no extended chars, continue building DIRENT
        //

        if (!ExtendedChars) {
            if (Lfn[i] > 0x7f) {
                ExtendedChars = TRUE;
            } else {
                DirentName[DirentIndex++] = (UCHAR) (
                Lfn[i] < 'a' ? Lfn[i] : Lfn[i] <= 'z' ? Lfn[i] - ('a' - 'A') : Lfn[i]);
            }
        }
    }

    //
    //  If the LFN ended in a space, or there was no dot and the name
    //  has more than 8 characters, then it is not 8.3 compliant.
    //

    if ((Lfn[LfnSize - 1] == L' ') ||
        (!DotEncountered && (LfnSize > 8))) {
        return FALSE;
    }

    //
    //  OK, now if we got this far then the LFN is 8dot3.  If there are
    //  no extended characters, then we can also check to make sure that
    //  the LFN is only a case varient of the DIRENT.
    //

    if (!ExtendedChars &&
        !RtlEqualMemory(Dirent->FileName, DirentName, 11)) {

        return FALSE;
    }

    //
    //  We have now verified this pairing the very best we can without
    //  knowledge of the code page that the file was created under.
    //

    return TRUE;
}
#endif //0

//
//  Internal support routine
//

_Requires_lock_held_(_Global_critical_region_)
VOID
FatRescanDirectory (
    PIRP_CONTEXT IrpContext,
    PDCB Dcb
    )

/*++

Routine Description:

    This routine rescans the given directory, finding the first unused
    dirent, first deleted dirent, and setting the free dirent bitmap
    appropriately.

Arguments:

    Dcb - Supplies the directory to rescan.

Return Value:

    None.

--*/

{
    PBCB Bcb = NULL;
    PDIRENT Dirent = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    ULONG UnusedVbo;
    ULONG DeletedHint;
    ULONG DirentIndex;
    ULONG DirentsThisRun;
    ULONG StartIndexOfThisRun;

    enum RunType {
        InitialRun,
        FreeDirents,
        AllocatedDirents,
    } CurrentRun;

    PAGED_CODE();

    DebugTrace( 0, Dbg, "We must scan the whole directory.\n", 0);

    UnusedVbo = 0;
    DeletedHint = 0xffffffff;

    //
    //  To start with, we have to find out if the first dirent is free.
    //

    CurrentRun = InitialRun;
    DirentIndex =
    StartIndexOfThisRun = 0;

    _SEH2_TRY {

        while ( TRUE ) {

            BOOLEAN DirentDeleted;

            //
            //  Read a dirent
            //

            FatReadDirent( IrpContext,
                           Dcb,
                           UnusedVbo,
                           &Bcb,
                           &Dirent,
                           &Status );

            //
            //  If EOF, or we found a NEVER_USED entry, we exit the loop
            //

            if ( (Status == STATUS_END_OF_FILE ) ||
                 (Dirent->FileName[0] == FAT_DIRENT_NEVER_USED)) {

                break;
            }

            //
            //  If the dirent is DELETED, and it is the first one we found, set
            //  it in the deleted hint.
            //

            if (Dirent->FileName[0] == FAT_DIRENT_DELETED) {

                DirentDeleted = TRUE;

                if (DeletedHint == 0xffffffff) {

                    DeletedHint = UnusedVbo;
                }

            } else {

                DirentDeleted = FALSE;
            }

            //
            //  Check for the first time through the loop, and determine
            //  the current run type.
            //

            if (CurrentRun == InitialRun) {

                CurrentRun = DirentDeleted ?
                             FreeDirents : AllocatedDirents;

            } else {

                //
                //  Are we switching from a free run to an allocated run?
                //

                if ((CurrentRun == FreeDirents) && !DirentDeleted) {

                    DirentsThisRun = DirentIndex - StartIndexOfThisRun;

                    RtlClearBits( &Dcb->Specific.Dcb.FreeDirentBitmap,
                                  StartIndexOfThisRun,
                                  DirentsThisRun );

                    CurrentRun = AllocatedDirents;
                    StartIndexOfThisRun = DirentIndex;
                }

                //
                //  Are we switching from an allocated run to a free run?
                //

                if ((CurrentRun == AllocatedDirents) && DirentDeleted) {

                    DirentsThisRun = DirentIndex - StartIndexOfThisRun;

                    RtlSetBits( &Dcb->Specific.Dcb.FreeDirentBitmap,
                                StartIndexOfThisRun,
                                DirentsThisRun );

                    CurrentRun = FreeDirents;
                    StartIndexOfThisRun = DirentIndex;
                }
            }

            //
            //  Move on to the next dirent.
            //

            UnusedVbo += sizeof(DIRENT);
            Dirent += 1;
            DirentIndex += 1;
        }

        //
        //  Now we have to record the final run we encoutered
        //

        DirentsThisRun = DirentIndex - StartIndexOfThisRun;

        if ((CurrentRun == FreeDirents) || (CurrentRun == InitialRun)) {

            RtlClearBits( &Dcb->Specific.Dcb.FreeDirentBitmap,
                          StartIndexOfThisRun,
                          DirentsThisRun );

        } else {

            RtlSetBits( &Dcb->Specific.Dcb.FreeDirentBitmap,
                        StartIndexOfThisRun,
                        DirentsThisRun );
        }

        //
        //  Now if there we bailed prematurely out of the loop because
        //  we hit an unused entry, set all the rest as free.
        //

        if (UnusedVbo < Dcb->Header.AllocationSize.LowPart) {

            StartIndexOfThisRun = UnusedVbo / sizeof(DIRENT);

            DirentsThisRun = (Dcb->Header.AllocationSize.LowPart -
                              UnusedVbo) / sizeof(DIRENT);

            RtlClearBits( &Dcb->Specific.Dcb.FreeDirentBitmap,
                          StartIndexOfThisRun,
                          DirentsThisRun);
        }

    } _SEH2_FINALLY {

        FatUnpinBcb( IrpContext, Bcb );
    } _SEH2_END;

    //
    //  If there weren't any DELETED entries, set the index to our current
    //  position.
    //

    if (DeletedHint == 0xffffffff) { DeletedHint = UnusedVbo; }

    Dcb->Specific.Dcb.UnusedDirentVbo = UnusedVbo;
    Dcb->Specific.Dcb.DeletedDirentHint = DeletedHint;

    return;
}


//
//  Internal support routine
//

_Requires_lock_held_(_Global_critical_region_)
ULONG
FatDefragDirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN ULONG DirentsNeeded
    )

/*++

Routine Description:

    This routine determines if the requested number of dirents can be found
    in the directory, looking for deleted dirents and orphaned LFNs.  If the
    request can be satisifed, orphaned LFNs are marked as deleted, and deleted
    dirents are all grouped together at the end of the directory.

    Note that this routine is currently used only on the root directory, but
    it is completely general and could be used on any directory.

Arguments:

    Dcb - Supplies the directory to defrag.

Return Value:

    The Index of the first dirent available for use, or -1 if the
    request cannot be satisfied.

--*/

{
    ULONG SavedIrpContextFlag;
    PLIST_ENTRY Links;
    ULONG ReturnValue = 0;
    PFCB Fcb;

    PBCB Bcb = NULL;
    PDIRENT Dirent = NULL;
    UNICODE_STRING Lfn = {0,0,NULL};

    LARGE_MCB Mcb;
    BOOLEAN McbInitialized = FALSE;
    BOOLEAN InvalidateFcbs = FALSE;

    PUCHAR Directory = NULL;
    PUCHAR UnusedDirents;
    PUCHAR UnusedDirentBuffer = NULL;
    PUCHAR UsedDirents;
    PUCHAR UsedDirentBuffer = NULL;

    PBCB *Bcbs = NULL;
    ULONG Page;
    ULONG PagesPinned = 0;

    ULONG DcbSize;
    ULONG TotalBytesAllocated = 0;

    PAGED_CODE();

    //
    //  We assume we own the Vcb.
    //

    NT_ASSERT( FatVcbAcquiredExclusive(IrpContext, Dcb->Vcb) );

    //
    //  We will only attempt this on directories less than 0x40000 bytes
    //  long (by default on DOS the root directory is only 0x2000 long).
    //  This is to avoid a cache manager complication.
    //

    DcbSize = Dcb->Header.AllocationSize.LowPart;

    if (DcbSize > 0x40000) {

        return (ULONG)-1;
    }

    //
    //  Force wait to TRUE
    //

    SavedIrpContextFlag = IrpContext->Flags;

    SetFlag( IrpContext->Flags,
             IRP_CONTEXT_FLAG_WAIT | IRP_CONTEXT_FLAG_WRITE_THROUGH );

    //
    //  Now acquire all open Fcbs in the Dcb exclusive.
    //

    for (Links = Dcb->Specific.Dcb.ParentDcbQueue.Flink;
         Links != &Dcb->Specific.Dcb.ParentDcbQueue;
         Links = Links->Flink) {

        Fcb = CONTAINING_RECORD( Links, FCB, ParentDcbLinks );

        (VOID)ExAcquireResourceExclusiveLite( Fcb->Header.Resource, TRUE );
    }

    _SEH2_TRY {

        CCB Ccb;
        ULONG QueryOffset = 0;
        ULONG FoundOffset = 0;
        ULONGLONG BytesUsed = 0;

        NTSTATUS DontCare;
        ULONG Run;
        ULONG TotalRuns;
        BOOLEAN Result;
        PUCHAR Char;

        //
        //  We are going to build a new bitmap that will show all orphaned
        //  LFNs as well as deleted dirents as available.
        //
        //  Initialize our local CCB that will match all files and even
        //  a label if it is here.
        //

        RtlZeroMemory( &Ccb, sizeof(CCB) );
        Ccb.Flags = CCB_FLAG_MATCH_ALL | CCB_FLAG_MATCH_VOLUME_ID;

        //
        //  Init the Long File Name string.
        //

        Lfn.MaximumLength = 260 * sizeof(WCHAR);
        Lfn.Buffer = FsRtlAllocatePoolWithTag( PagedPool,
                                               260*sizeof(WCHAR),
                                               TAG_FILENAME_BUFFER );

        //
        //  Initalize the Mcb.  We use this structure to keep track of runs
        //  of free and allocated dirents.  Runs are identity allocations, and
        //  holes are free dirents.
        //

        FsRtlInitializeLargeMcb( &Mcb, PagedPool );

        McbInitialized = TRUE;

        do {
            
            FatLocateDirent( IrpContext,
                             Dcb,
                             &Ccb,
                             QueryOffset,
                             NULL,
                             &Dirent,
                             &Bcb,
                             (PVBO)&FoundOffset,
                             NULL,
                             &Lfn,
                             NULL );

            if (Dirent != NULL) {

                ULONG LfnByteOffset;

                //
                //  Compute the LfnByteOffset.
                //

                LfnByteOffset = FoundOffset -
                                FAT_LFN_DIRENTS_NEEDED(&Lfn) * sizeof(LFN_DIRENT);

                BytesUsed = FoundOffset - LfnByteOffset + sizeof(DIRENT);

                //
                //  Set a run to represent all the dirents used for this
                //  file in the Dcb dir.
                //

#ifdef _MSC_VER
#pragma prefast( suppress:28931, "needed for debug build" )
#endif
                Result = FsRtlAddLargeMcbEntry( &Mcb,
                                                LfnByteOffset,
                                                LfnByteOffset,
                                                BytesUsed );

                NT_ASSERT( Result );

                //
                //  Move on to the next dirent.
                //

                TotalBytesAllocated += (ULONG) BytesUsed;
                QueryOffset = FoundOffset + sizeof(DIRENT);
            }

        } while ((Dirent != NULL) && (QueryOffset < DcbSize));

        if (Bcb != NULL) {

            FatUnpinBcb( IrpContext, Bcb );
        }

        //
        //  If we need more dirents than are available, bail.
        //

        if (DirentsNeeded > (DcbSize - TotalBytesAllocated)/sizeof(DIRENT)) {

            try_return(ReturnValue = (ULONG)-1);
        }

        //
        //  Now we are going to copy all the used and un-used parts of the
        //  directory to separate pool.
        //
        //  Allocate these buffers and pin the entire directory.
        //

        UnusedDirents =
        UnusedDirentBuffer = FsRtlAllocatePoolWithTag( PagedPool,
                                                       DcbSize - TotalBytesAllocated,
                                                       TAG_DIRENT );

        UsedDirents =
        UsedDirentBuffer = FsRtlAllocatePoolWithTag( PagedPool,
                                                     TotalBytesAllocated,
                                                     TAG_DIRENT );

        PagesPinned = (DcbSize + (PAGE_SIZE - 1 )) / PAGE_SIZE;

        Bcbs = FsRtlAllocatePoolWithTag( PagedPool,
                                         PagesPinned * sizeof(PBCB),
                                         TAG_BCB );

        RtlZeroMemory( Bcbs, PagesPinned * sizeof(PBCB) );

        for (Page = 0; Page < PagesPinned; Page += 1) {

            ULONG PinSize;

            //
            //  Don't try to pin beyond the Dcb size.
            //

            if ((Page + 1) * PAGE_SIZE > DcbSize) {

                PinSize = DcbSize - (Page * PAGE_SIZE);

            } else {

                PinSize = PAGE_SIZE;
            }

            FatPrepareWriteDirectoryFile( IrpContext,
                                          Dcb,
                                          Page * PAGE_SIZE,
                                          PinSize,
                                          &Bcbs[Page],
#ifndef __REACTOS__
                                          &Dirent,
#else
                                          (PVOID *)&Dirent,
#endif
                                          FALSE,
                                          TRUE,
                                          &DontCare );

            if (Page == 0) {
                Directory = (PUCHAR)Dirent;
            }
        }

        TotalRuns = FsRtlNumberOfRunsInLargeMcb( &Mcb );

        for (Run = 0; Run < TotalRuns; Run++) {

            LBO Vbo;
            LBO Lbo;

#ifdef _MSC_VER
#pragma prefast( suppress:28931, "needed for debug build" )
#endif
            Result = FsRtlGetNextLargeMcbEntry( &Mcb,
                                                Run,
                                                &Vbo,
                                                &Lbo,
                                                (PLONGLONG)&BytesUsed );

            NT_ASSERT(Result);

            //
            //  Copy each run to their specific pool.
            //

            if (Lbo != -1) {

                RtlCopyMemory( UsedDirents,
                               Directory + Vbo,
                               (ULONG) BytesUsed );

                UsedDirents += BytesUsed;

            } else {

                RtlCopyMemory( UnusedDirents,
                               Directory + Vbo,
                               (ULONG) BytesUsed );

                UnusedDirents += BytesUsed;
            }
        }

        //
        //  Marking all the un-used dirents as "deleted".  This will reclaim
        //  storage used by orphaned LFNs.
        //

        for (Char = UnusedDirentBuffer; Char < UnusedDirents; Char += sizeof(DIRENT)) {

            *Char = FAT_DIRENT_DELETED;
        }

        //
        //  Now, for the permanent step.  Copy the two pool buffer back to the
        //  real Dcb directory, and flush the Dcb directory
        //

        NT_ASSERT( TotalBytesAllocated == (ULONG)(UsedDirents - UsedDirentBuffer) );

        RtlCopyMemory( Directory, UsedDirentBuffer, TotalBytesAllocated );

        RtlCopyMemory( Directory + TotalBytesAllocated,
                       UnusedDirentBuffer,
                       UnusedDirents - UnusedDirentBuffer );

        //
        //  We need to unpin here so that the UnpinRepinned won't deadlock.
        //

        if (Bcbs) {
            for (Page = 0; Page < PagesPinned; Page += 1) {
                FatUnpinBcb( IrpContext, Bcbs[Page] );
            }
            ExFreePool(Bcbs);
            Bcbs = NULL;
        }

        //
        //  Now make the free dirent bitmap reflect the new state of the Dcb
        //  directory.
        //

        RtlSetBits( &Dcb->Specific.Dcb.FreeDirentBitmap,
                    0,
                    TotalBytesAllocated / sizeof(DIRENT) );

        RtlClearBits( &Dcb->Specific.Dcb.FreeDirentBitmap,
                      TotalBytesAllocated / sizeof(DIRENT),
                      (DcbSize - TotalBytesAllocated) / sizeof(DIRENT) );

        ReturnValue = TotalBytesAllocated / sizeof(DIRENT);

        //
        //  Flush the directory to disk.  If we raise, we will need to invalidate
        //  all of the children.  Sorry, guys, but I can't figure out where you are
        //  now - if this failed I probably can't read the media either.  And we
        //  probably purged the cache to boot.
        //

        _SEH2_TRY {
            
            FatUnpinRepinnedBcbs( IrpContext );

        } _SEH2_EXCEPT(FsRtlIsNtstatusExpected(_SEH2_GetExceptionCode()) ?
                 EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {

              InvalidateFcbs = TRUE;
        } _SEH2_END;

        //
        //  OK, now nothing can go wrong.  We have two more things to do.
        //  First, we have to fix up all the dirent offsets in any open Fcbs.
        //  If we cannot now find the Fcb, the file is marked invalid.  Also,
        //  we skip deleted files.
        //

        for (Links = Dcb->Specific.Dcb.ParentDcbQueue.Flink;
             Links != &Dcb->Specific.Dcb.ParentDcbQueue;
             Links = Links->Flink) {

            PBCB TmpBcb = NULL;
            ULONG TmpOffset = 0;
            PDIRENT TmpDirent = NULL;
            ULONG PreviousLfnSpread;

            Fcb = CONTAINING_RECORD( Links, FCB, ParentDcbLinks );

            if (IsFileDeleted( IrpContext, Fcb )) {

                continue;
            }

            //
            //  If we aren't already giving up, safely try to pick up the dirent
            //  to update the Fcb.  If this raises, we have to give up and blow
            //  evenyone else away too.
            //
            
            if (!InvalidateFcbs) {
                
                _SEH2_TRY {
                    
                    FatLocateSimpleOemDirent( IrpContext,
                                              Dcb,
                                              &Fcb->ShortName.Name.Oem,
                                              &TmpDirent,
                                              &TmpBcb,
                                              (PVBO)&TmpOffset );
                
                } _SEH2_EXCEPT(FsRtlIsNtstatusExpected(_SEH2_GetExceptionCode()) ?
                         EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {

                      InvalidateFcbs = TRUE;
                } _SEH2_END;
            }

            if (TmpBcb == NULL || InvalidateFcbs) {

                FatUnpinBcb( IrpContext, TmpBcb );
                FatMarkFcbCondition( IrpContext, Fcb, FcbBad, TRUE );

            } else {

                FatUnpinBcb( IrpContext, TmpBcb );

                PreviousLfnSpread = Fcb->DirentOffsetWithinDirectory -
                                    Fcb->LfnOffsetWithinDirectory;

                Fcb->DirentOffsetWithinDirectory = TmpOffset;
                Fcb->LfnOffsetWithinDirectory = TmpOffset - PreviousLfnSpread;
            }
        }

    try_exit: NOTHING;
    } _SEH2_FINALLY {

        //
        //  Free all our resources and stuff.
        //

        if (McbInitialized) {
            FsRtlUninitializeLargeMcb( &Mcb );
        }

        if (Lfn.Buffer) {
            ExFreePool( Lfn.Buffer );
        }

        if (UnusedDirentBuffer) {
            ExFreePool( UnusedDirentBuffer );
        }

        if (UsedDirentBuffer) {
            ExFreePool( UsedDirentBuffer );
        }

        if (Bcbs) {
            for (Page = 0; Page < PagesPinned; Page += 1) {
                FatUnpinBcb( IrpContext, Bcbs[Page] );
            }
            ExFreePool(Bcbs);
        }

        FatUnpinBcb( IrpContext, Bcb );

        for (Links = Dcb->Specific.Dcb.ParentDcbQueue.Flink;
             Links != &Dcb->Specific.Dcb.ParentDcbQueue;
             Links = Links->Flink) {

            Fcb = CONTAINING_RECORD( Links, FCB, ParentDcbLinks );

            ExReleaseResourceLite( Fcb->Header.Resource );
        }

        IrpContext->Flags = SavedIrpContextFlag;
    } _SEH2_END;

    //
    //  Now return the offset of the first free dirent to the caller.
    //

    return ReturnValue;
}



