/*++

Copyright (c) 1990-2000 Microsoft Corporation

Module Name:

    AllocSup.c

Abstract:

    This module implements the Allocation support routines for Cdfs.

    The data structure used here is the CD_MCB.  There is an entry in
    the Mcb for each dirent for a file.  The entry will map the offset
    within some file to a starting disk offset and number of bytes.
    The Mcb also contains the interleave information for an extent.
    An interleave consists of a number of blocks with data and a
    (possibly different) number of blocks to skip.  Any number of
    data/skip pairs may exist in an extent but the data and skip sizes
    are the same throughout the extent.

    We store the following information into an Mcb entry for an extent.

        FileOffset          Offset in file for start of extent
        DiskOffset          Offset on disk for start of extent
        ByteCount           Number of file bytes in extent, no skip bytes
        DataBlockByteCount  Number of bytes in each data block
        TotalBlockByteCount Number of bytes is data block and skip block

    The disk offset in the Mcb has already been biased by the size of
    the Xar block if present.  All of the byte count fields are aligned
    on logical block boundaries.  If this is a directory or path table
    then the file offset has been biased to round the initial disk
    offset down to a sector boundary.  The biasing is done when loading
    the values into an Mcb entry.

    An XA file has a header prepended to the file and each sector is 2352
    bytes.  The allocation information ignores the header and only deals
    with 2048 byte sectors.  Callers into the allocation package have
    adjusted the starting offset value to reflect 2048 sectors.  On return
    from this package the caller will have to convert from 2048 sector values
    into raw XA sector values.


--*/

#include "CdProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (CDFS_BUG_CHECK_ALLOCSUP)

//
//  Local support routines
//

ULONG
CdFindMcbEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN LONGLONG FileOffset
    );

VOID
CdDiskOffsetFromMcbEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PCD_MCB_ENTRY McbEntry,
    IN LONGLONG FileOffset,
    IN PLONGLONG DiskOffset,
    IN PULONG ByteCount
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CdAddInitialAllocation)
#pragma alloc_text(PAGE, CdAddAllocationFromDirent)
#pragma alloc_text(PAGE, CdDiskOffsetFromMcbEntry)
#pragma alloc_text(PAGE, CdFindMcbEntry)
#pragma alloc_text(PAGE, CdInitializeMcb)
#pragma alloc_text(PAGE, CdLookupAllocation)
#pragma alloc_text(PAGE, CdTruncateAllocation)
#pragma alloc_text(PAGE, CdUninitializeMcb)
#endif


VOID
CdLookupAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN LONGLONG FileOffset,
    OUT PLONGLONG DiskOffset,
    OUT PULONG ByteCount
    )

/*++

Routine Description:

    This routine looks through the mapping information for the file
    to find the logical diskoffset and number of bytes at that offset.
    We only deal with logical 2048 byte sectors here.

    If the mapping isn't present we will look it up on disk now.
    This routine assumes we are looking up a valid range in the file.  This
    routine raises if it can't find mapping for the file offset.

    The Fcb may not be locked prior to calling this routine.  We will always
    acquire it here.

Arguments:

    Fcb - Fcb representing this stream.

    FileOffset - Lookup the allocation beginning at this point.

    DiskOffset - Address to store the logical disk offset.

    ByteCount - Address to store the number of contiguous bytes beginning
        at DiskOffset above.

Return Value:

    None.

--*/

{
    BOOLEAN FirstPass = TRUE;
    ULONG McbEntryOffset;
    PFCB ParentFcb;
    BOOLEAN CleanupParent = FALSE;

    BOOLEAN UnlockFcb = FALSE;

    LONGLONG CurrentFileOffset;
    ULONG CurrentMcbOffset;
    PCD_MCB_ENTRY CurrentMcbEntry;

    DIRENT_ENUM_CONTEXT DirContext;
    DIRENT Dirent;

    PAGED_CODE();

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( Fcb );

    //
    //  Use a try finally to facilitate cleanup.
    //

    try {

        //
        //  We use a loop to perform the lookup.  If we don't find the mapping in the
        //  first pass then we look up all of the allocation and then look again.

        while (TRUE) {

            //
            //
            //  Lookup the entry containing this file offset.
            //

            CdLockFcb( IrpContext, Fcb );
            UnlockFcb = TRUE;

            McbEntryOffset = CdFindMcbEntry( IrpContext, Fcb, FileOffset );

            //
            //  If within the Mcb then we use the data out of this entry and are
            //  done.
            //

            if (McbEntryOffset < Fcb->Mcb.CurrentEntryCount) {

                CdDiskOffsetFromMcbEntry( IrpContext,
                                          Fcb->Mcb.McbArray + McbEntryOffset,
                                          FileOffset,
                                          DiskOffset,
                                          ByteCount );

                break;

            //
            //  If this is not the first pass then the disk is corrupt.
            //

            } else if (!FirstPass) {

                CdRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR );
            }

            CdUnlockFcb( IrpContext, Fcb );
            UnlockFcb = FALSE;

            //
            //  Initialize the search dirent structures.
            //

            CdInitializeDirContext( IrpContext, &DirContext );
            CdInitializeDirent( IrpContext, &Dirent );

            //
            //  Otherwise we need to walk the dirents for this file until we find
            //  the one containing this entry.  The parent Fcb should always be
            //  present.
            //

            ParentFcb = Fcb->ParentFcb;
            CdAcquireFileShared( IrpContext, ParentFcb );
            CleanupParent = TRUE;

            //
            //  Do an unsafe test to see if we need to create a file object.
            //

            if (ParentFcb->FileObject == NULL) {

                CdCreateInternalStream( IrpContext, ParentFcb->Vcb, ParentFcb );
            }

            //
            //  Initialize the local variables to indicate the first dirent
            //  and lookup the first dirent.
            //

            CurrentFileOffset = 0;
            CurrentMcbOffset = 0;

            CdLookupDirent( IrpContext,
                            ParentFcb,
                            CdQueryFidDirentOffset( Fcb->FileId ),
                            &DirContext );

            //
            //  If we are adding allocation to the Mcb then add all of it.
            //

            while (TRUE ) {

                //
                //  Update the dirent from the on-disk dirent.
                //

                CdUpdateDirentFromRawDirent( IrpContext, ParentFcb, &DirContext, &Dirent );

                //
                //  Add this dirent to the Mcb if not already present.
                //

                CdLockFcb( IrpContext, Fcb );
                UnlockFcb = TRUE;

                if (CurrentMcbOffset >= Fcb->Mcb.CurrentEntryCount) {

                    CdAddAllocationFromDirent( IrpContext, Fcb, CurrentMcbOffset, CurrentFileOffset, &Dirent );
                }

                CdUnlockFcb( IrpContext, Fcb );
                UnlockFcb = FALSE;

                //
                //  If this is the last dirent for the file then exit.
                //

                if (!FlagOn( Dirent.DirentFlags, CD_ATTRIBUTE_MULTI )) {

                    break;
                }

                //
                //  If we couldn't find another entry then the directory is corrupt because
                //  the last dirent for a file doesn't exist.
                //

                if (!CdLookupNextDirent( IrpContext, ParentFcb, &DirContext, &DirContext )) {

                    CdRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR );
                }

                //
                //  Update our loop variables.
                //

                CurrentMcbEntry = Fcb->Mcb.McbArray + CurrentMcbOffset;
                CurrentFileOffset += CurrentMcbEntry->ByteCount;
                CurrentMcbOffset += 1;
            }

            //
            //  All of the allocation is loaded.  Go back and look up the mapping again.
            //  It better be there this time.
            //

            FirstPass = FALSE;
        }

    } finally {

        if (CleanupParent) {

            //
            //  Release the parent and cleanup the dirent structures.
            //

            CdReleaseFile( IrpContext, ParentFcb );

            CdCleanupDirContext( IrpContext, &DirContext );
            CdCleanupDirent( IrpContext, &Dirent );
        }

        if (UnlockFcb) { CdUnlockFcb( IrpContext, Fcb ); }
    }

    return;
}


VOID
CdAddAllocationFromDirent (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ULONG McbEntryOffset,
    IN LONGLONG StartingFileOffset,
    IN PDIRENT Dirent
    )

/*++

Routine Description:

    This routine is called to add an entry into the Cd Mcb.  We grow the Mcb
    as necessary and update the new entry.

    NOTE - The Fcb has already been locked prior to makeing this call.

Arguments:

    Fcb - Fcb containing the Mcb to update.

    McbEntryOffset - Offset into the Mcb array to add this data.

    StartingFileOffset - Offset in bytes from the start of the file.

    Dirent - Dirent containing the on-disk data for this entry.

Return Value:

    None

--*/

{
    ULONG NewArraySize;
    PVOID NewMcbArray;
    PCD_MCB_ENTRY McbEntry;

    PAGED_CODE();

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( Fcb );
    ASSERT_LOCKED_FCB( Fcb );

    //
    //  If we need to grow the Mcb then do it now.
    //

    if (McbEntryOffset >= Fcb->Mcb.MaximumEntryCount) {

        //
        //  Allocate a new buffer and copy the old data over.
        //

        NewArraySize = Fcb->Mcb.MaximumEntryCount * 2 * sizeof( CD_MCB_ENTRY );

        NewMcbArray = FsRtlAllocatePoolWithTag( CdPagedPool,
                                                NewArraySize,
                                                TAG_MCB_ARRAY );

        RtlZeroMemory( NewMcbArray, NewArraySize );
        RtlCopyMemory( NewMcbArray,
                       Fcb->Mcb.McbArray,
                       Fcb->Mcb.MaximumEntryCount * sizeof( CD_MCB_ENTRY ));

        //
        //  Deallocate the current array unless it is embedded in the Fcb.
        //

        if (Fcb->Mcb.MaximumEntryCount != 1) {

            CdFreePool( &Fcb->Mcb.McbArray );
        }

        //
        //  Now update the Mcb with the new array.
        //

        Fcb->Mcb.MaximumEntryCount *= 2;
        Fcb->Mcb.McbArray = NewMcbArray;
    }

    //
    //  Update the new entry with the input data.
    //

    McbEntry = Fcb->Mcb.McbArray + McbEntryOffset;

    //
    //  Start with the location and length on disk.
    //

    McbEntry->DiskOffset = LlBytesFromBlocks( Fcb->Vcb, Dirent->StartingOffset );
    McbEntry->ByteCount = Dirent->DataLength;

    //
    //  Round the byte count up to a logical block boundary if this is
    //  the last extent.
    //

    if (!FlagOn( Dirent->DirentFlags, CD_ATTRIBUTE_MULTI )) {

        McbEntry->ByteCount = BlockAlign( Fcb->Vcb, McbEntry->ByteCount );
    }

    //
    //  The file offset is the logical position within this file.
    //  We know this is correct regardless of whether we bias the
    //  file size or disk offset.
    //

    McbEntry->FileOffset = StartingFileOffset;

    //
    //  Convert the interleave information from logical blocks to
    //  bytes.
    //

    if (Dirent->FileUnitSize != 0) {

        McbEntry->DataBlockByteCount = LlBytesFromBlocks( Fcb->Vcb, Dirent->FileUnitSize );
        McbEntry->TotalBlockByteCount = McbEntry->DataBlockByteCount +
                                        LlBytesFromBlocks( Fcb->Vcb, Dirent->InterleaveGapSize );

    //
    //  If the file is not interleaved then the size of the data block
    //  and total block are the same as the byte count.
    //

    } else {

        McbEntry->DataBlockByteCount =
        McbEntry->TotalBlockByteCount = McbEntry->ByteCount;
    }

    //
    //  Update the number of entries in the Mcb.  The Mcb is never sparse
    //  so whenever we add an entry it becomes the last entry in the Mcb.
    //

    Fcb->Mcb.CurrentEntryCount = McbEntryOffset + 1;

    return;
}


VOID
CdAddInitialAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ULONG StartingBlock,
    IN LONGLONG DataLength
    )

/*++

Routine Description:

    This routine is called to set up the initial entry in an Mcb.

    This routine handles the single initial entry for a directory file.  We will 
    round the start block down to a sector boundary.  Our caller has already 
    biased the DataLength with any adjustments.  This is used for the case 
    where there is a single entry and we want to align the data on a sector 
    boundary.

Arguments:

    Fcb - Fcb containing the Mcb to update.

    StartingBlock - Starting logical block for this directory.  This is
        the start of the actual data.  We will bias this by the sector
        offset of the data.

    DataLength - Length of the data.

Return Value:

    None

--*/

{
    PCD_MCB_ENTRY McbEntry;

    PAGED_CODE();

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( Fcb );
    ASSERT_LOCKED_FCB( Fcb );
    ASSERT( 0 == Fcb->Mcb.CurrentEntryCount);
    ASSERT( CDFS_NTC_FCB_DATA != Fcb->NodeTypeCode);

    //
    //  Update the new entry with the input data.
    //

    McbEntry = Fcb->Mcb.McbArray;

    //
    //  Start with the location and length on disk.
    //

    McbEntry->DiskOffset = LlBytesFromBlocks( Fcb->Vcb, StartingBlock );
    McbEntry->DiskOffset -= Fcb->StreamOffset;

    McbEntry->ByteCount = DataLength;

    //
    //  The file offset is the logical position within this file.
    //  We know this is correct regardless of whether we bias the
    //  file size or disk offset.
    //

    McbEntry->FileOffset = 0;

    //
    //  If the file is not interleaved then the size of the data block
    //  and total block are the same as the byte count.
    //

    McbEntry->DataBlockByteCount =
    McbEntry->TotalBlockByteCount = McbEntry->ByteCount;

    //
    //  Update the number of entries in the Mcb.  The Mcb is never sparse
    //  so whenever we add an entry it becomes the last entry in the Mcb.
    //

    Fcb->Mcb.CurrentEntryCount = 1;

    return;
}


VOID
CdTruncateAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN LONGLONG StartingFileOffset
    )

/*++

Routine Description:

    This routine truncates the Mcb for a file by eliminating all of the Mcb
    entries from the entry which contains the given offset.

    The Fcb should be locked when this routine is called.

Arguments:

    Fcb - Fcb containing the Mcb to truncate.

    StartingFileOffset - Offset in the file to truncate the Mcb from.

Return Value:

    None

--*/

{
    ULONG McbEntryOffset;

    PAGED_CODE();

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( Fcb );
    ASSERT_LOCKED_FCB( Fcb );

    //
    //  Find the entry containg this starting offset.
    //

    McbEntryOffset = CdFindMcbEntry( IrpContext, Fcb, StartingFileOffset );

    //
    //  Now set the current size of the mcb to this point.
    //

    Fcb->Mcb.CurrentEntryCount = McbEntryOffset;

    return;
}


VOID
CdInitializeMcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routine is called to initialize the Mcb in an Fcb.  We initialize
    this with an entry count of one and point to the entry in the Fcb
    itself.

    Fcb should be acquired exclusively when this is called.

Arguments:

    Fcb - Fcb containing the Mcb to initialize.

Return Value:

    None

--*/

{
    PAGED_CODE();

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( Fcb );

    //
    //  Set the entry counts to show there is one entry in the array and
    //  it is unused.
    //

    Fcb->Mcb.MaximumEntryCount = 1;
    Fcb->Mcb.CurrentEntryCount = 0;

    Fcb->Mcb.McbArray = &Fcb->McbEntry;

    return;
}


VOID
CdUninitializeMcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routine is called to cleanup an Mcb in an Fcb.  We look at the
    maximum run count in the Fcb and if greater than one we will deallocate
    the buffer.

    Fcb should be acquired exclusively when this is called.

Arguments:

    Fcb - Fcb containing the Mcb to uninitialize.

Return Value:

    None

--*/

{
    PAGED_CODE();

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( Fcb );

    //
    //  If the count is greater than one then this is an allocated buffer.
    //

    if (Fcb->Mcb.MaximumEntryCount > 1) {

        CdFreePool( &Fcb->Mcb.McbArray );
    }

    return;
}


//
//  Local suupport routine
//

ULONG
CdFindMcbEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN LONGLONG FileOffset
    )

/*++

Routine Description:

    This routine is called to find the Mcb entry which contains the file
    offset at the given point.  If the file offset is not currently in the
    Mcb then we return the offset of the entry to add.

    Fcb should be locked when this is called.

Arguments:

    Fcb - Fcb containing the Mcb to uninitialize.

    FileOffset - Return the Mcb entry which contains this file offset.

Return Value:

    ULONG - Offset in the Mcb of the entry for this offset.

--*/

{
    ULONG CurrentMcbOffset;
    PCD_MCB_ENTRY CurrentMcbEntry;

    PAGED_CODE();

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( Fcb );
    ASSERT_LOCKED_FCB( Fcb );

    //
    //  We expect a linear search will be sufficient here.
    //

    CurrentMcbOffset = 0;
    CurrentMcbEntry = Fcb->Mcb.McbArray;

    while (CurrentMcbOffset < Fcb->Mcb.CurrentEntryCount) {

        //
        //  Check if the offset lies within the current Mcb position.
        //

        if (FileOffset < CurrentMcbEntry->FileOffset + CurrentMcbEntry->ByteCount) {

            break;
        }

        //
        //  Move to the next entry.
        //

        CurrentMcbOffset += 1;
        CurrentMcbEntry += 1;
    }

    //
    //  This is the offset containing this file offset (or the point
    //  where an entry should be added).
    //

    return CurrentMcbOffset;
}


//
//  Local support routine
//

VOID
CdDiskOffsetFromMcbEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PCD_MCB_ENTRY McbEntry,
    IN LONGLONG FileOffset,
    IN PLONGLONG DiskOffset,
    IN PULONG ByteCount
    )

/*++

Routine Description:

    This routine is called to return the diskoffset and length of the file
    data which begins at offset 'FileOffset'.  We have the Mcb entry which
    contains the mapping and interleave information.

    NOTE - This routine deals with data in 2048 byte logical sectors.  If
        this is an XA file then our caller has already converted from
        'raw' file bytes to 'cooked' file bytes.

Arguments:

    McbEntry - Entry in the Mcb containing the allocation information.

    FileOffset - Starting Offset in the file to find the matching disk
        offsets.

    DiskOffset - Address to store the starting disk offset for this operation.

    ByteCount - Address to store number of contiguous bytes starting at this
        disk offset.

Return Value:

    None

--*/

{
    LONGLONG ExtentOffset;

    LONGLONG CurrentDiskOffset;
    LONGLONG CurrentExtentOffset;

    LONGLONG LocalByteCount;

    PAGED_CODE();
    ASSERT_IRP_CONTEXT( IrpContext );

    //
    //  Extent offset is the difference between the file offset and the start
    //  of the extent.
    //

    ExtentOffset = FileOffset - McbEntry->FileOffset;

    //
    //  Optimize the non-interleave case.
    //

    if (McbEntry->ByteCount == McbEntry->DataBlockByteCount) {

        *DiskOffset = McbEntry->DiskOffset + ExtentOffset;

        LocalByteCount = McbEntry->ByteCount - ExtentOffset;

    } else {

        //
        //  Walk though any interleave until we reach the current offset in
        //  this extent.
        //

        CurrentExtentOffset = McbEntry->DataBlockByteCount;
        CurrentDiskOffset = McbEntry->DiskOffset;

        while (CurrentExtentOffset <= ExtentOffset) {

            CurrentDiskOffset += McbEntry->TotalBlockByteCount;
            CurrentExtentOffset += McbEntry->DataBlockByteCount;
        }

        //
        //  We are now positioned at the data block containing the starting
        //  file offset we were given.  The disk offset is the offset of
        //  the start of this block plus the extent offset into this block.
        //  The byte count is the data block byte count minus our offset into
        //  this block.
        //

        *DiskOffset = CurrentDiskOffset + (ExtentOffset + McbEntry->DataBlockByteCount - CurrentExtentOffset);

        //
        //  Make sure we aren't past the end of the data length.  This is possible
        //  if we only use part of the last data block on an interleaved file.
        //

        if (CurrentExtentOffset > McbEntry->ByteCount) {

            CurrentExtentOffset = McbEntry->ByteCount;
        }

        LocalByteCount = CurrentExtentOffset - ExtentOffset;
    }

    //
    //  If the byte count exceeds our limit then cut it to fit in 32 bits.
    //

    if (LocalByteCount > MAXULONG) {

        *ByteCount = MAXULONG;

    } else {

        *ByteCount = (ULONG) LocalByteCount;
    }

    return;
}

