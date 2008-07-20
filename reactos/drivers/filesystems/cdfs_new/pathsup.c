/*++

Copyright (c) 1989-2000 Microsoft Corporation

Module Name:

    PathSup.c

Abstract:

    This module implements the Path Table support routines for Cdfs.

    The path table on a CDROM is a condensed summary of the entire
    directory structure.  It is stored on a number of contiguous sectors
    on the disk.  Each directory on the disk has an entry in the path
    table.  The entries are aligned on USHORT boundaries and MAY span
    sector boundaries.  The entries are stored as a breadth-first search.

    The first entry in the table contains the entry for the root.  The
    next entries will consist of the contents of the root directory.  The
    next entries will consist of the all the directories at the next level
    of the tree.  The children of a given directory will be grouped together.

    The directories are assigned ordinal numbers based on their position in
    the path table.  The root dirctory is assigned ordinal value 1.

    Path table sectors:

      Ordinal     1        2        3             4       5        6
                                         +-----------+
                                         | Spanning  |
                                         | Sectors   |
              +----------------------------+  +------------------------+
              |        |        |        | |  |      |         |       |
      DirName |  \     |   a    |    b   |c|  |   c  |    d    |   e   |
              |        |        |        | |  |      |         |       |
      Parent #|  1     |   1    |    1   | |  |   2  |    2    |   3   |
              +----------------------------+  +------------------------+

    Directory Tree:

                                            \ (root)

                                          /   \
                                         /     \
                                        a       b

                                      /   \       \
                                     /     \       \
                                    c       d       e

    Path Table Entries:

        - Position scan at known offset in the path table.  Path Entry at
            this offset must exist and is known to be valid.  Used when
            scanning for the children of a given directory.

        - Position scan at known offset in the path table.  Path Entry is
            known to start at this location but the bounds must be checked
            for validity.

        - Move to next path entry in the table.

        - Update a common path entry structure with the details of the
            on-disk structure.  This is used to smooth out the differences
            in the on-disk structures.

        - Update the filename in the in-memory path entry with the bytes
            off the disk.  For Joliet disks we will have
            to convert to little endian.  We assume that directories
            don't have version numbers.


--*/

#include "CdProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (CDFS_BUG_CHECK_PATHSUP)

//
//  Local macros
//

//
//  PRAW_PATH_ENTRY
//  CdRawPathEntry (
//      IN PIRP_CONTEXT IrpContext,
//      IN PPATH_ENUM_CONTEXT PathContext
//      );
//

#define CdRawPathEntry(IC, PC)      \
    Add2Ptr( (PC)->Data, (PC)->DataOffset, PRAW_PATH_ENTRY )

//
//  Local support routines
//

VOID
CdMapPathTableBlock (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN LONGLONG BaseOffset,
    IN OUT PPATH_ENUM_CONTEXT PathContext
    );

BOOLEAN
CdUpdatePathEntryFromRawPathEntry (
    IN PIRP_CONTEXT IrpContext,
    IN ULONG Ordinal,
    IN BOOLEAN VerifyBounds,
    IN PPATH_ENUM_CONTEXT PathContext,
    OUT PPATH_ENTRY PathEntry
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CdFindPathEntry)
#pragma alloc_text(PAGE, CdLookupPathEntry)
#pragma alloc_text(PAGE, CdLookupNextPathEntry)
#pragma alloc_text(PAGE, CdMapPathTableBlock)
#pragma alloc_text(PAGE, CdUpdatePathEntryFromRawPathEntry)
#pragma alloc_text(PAGE, CdUpdatePathEntryName)
#endif


VOID
CdLookupPathEntry (
    IN PIRP_CONTEXT IrpContext,
    IN ULONG PathEntryOffset,
    IN ULONG Ordinal,
    IN BOOLEAN VerifyBounds,
    IN OUT PCOMPOUND_PATH_ENTRY CompoundPathEntry
    )

/*++

Routine Description:

    This routine is called to initiate a walk through a path table.  We are
    looking for a path table entry at location PathEntryOffset.

Arguments:

    PathEntryOffset - This is our target point in the Path Table.  We know that
        a path entry must begin at this point although we may have to verify
        the bounds.

    Ordinal - Ordinal number for the directory at the PathEntryOffset above.

    VerifyBounds - Indicates whether we need to check the validity of
        this entry.

    CompoundPathEntry - PathEnumeration context and in-memory path entry.  This
        has been initialized outside of this call.

Return Value:

    None.

--*/

{
    PPATH_ENUM_CONTEXT PathContext = &CompoundPathEntry->PathContext;
    LONGLONG CurrentBaseOffset;

    PAGED_CODE();

    //
    //  Compute the starting base and starting path table offset.
    //

    CurrentBaseOffset = SectorTruncate( PathEntryOffset );

    //
    //  Map the next block in the Path Table.
    //

    CdMapPathTableBlock( IrpContext,
                         IrpContext->Vcb->PathTableFcb,
                         CurrentBaseOffset,
                         PathContext );

    //
    //  Set up our current offset into the Path Context.
    //

    PathContext->DataOffset = PathEntryOffset - PathContext->BaseOffset;

    //
    //  Update the in-memory structure for this path entry.
    //

    (VOID) CdUpdatePathEntryFromRawPathEntry( IrpContext,
                                              Ordinal,
                                              VerifyBounds,
                                              &CompoundPathEntry->PathContext,
                                              &CompoundPathEntry->PathEntry );
}


BOOLEAN
CdLookupNextPathEntry (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PPATH_ENUM_CONTEXT PathContext,
    IN OUT PPATH_ENTRY PathEntry
    )

/*++

Routine Description:

    This routine is called to move to the next path table entry.  We know
    the offset and the length of the current entry.  We start by computing
    the offset of the next entry and determine if it is contained in the
    table.  Then we check to see if we need to move to the next sector in
    the path table.  We always map two sectors at a time so we don't
    have to deal with any path entries which span sectors.  We move to
    the next sector if we are in the second sector of the current mapped
    data block.

    We look up the next entry and update the path entry structure with
    the values out of the raw sector but don't update the CdName structure.

Arguments:

    PathContext - Enumeration context for this scan of the path table.

    PathEntry - In-memory representation of the on-disk path table entry.

Return Value:

    BOOLEAN - TRUE if another entry is found, FALSE otherwise.
        This routine may raise on error.

--*/

{
    LONGLONG CurrentBaseOffset;

    PAGED_CODE();

    //
    //  Get the offset of the next path entry within the current
    //  data block.
    //

    PathContext->DataOffset += PathEntry->PathEntryLength;

    //
    //  If we are in the last data block then check if we are beyond the
    //  end of the file.
    //

    if (PathContext->LastDataBlock) {

        if (PathContext->DataOffset >= PathContext->DataLength) {

            return FALSE;
        }

    //
    //  If we are not in the last data block of the path table and
    //  this offset is in the second sector then move to the next
    //  data block.
    //

    } else if (PathContext->DataOffset >= SECTOR_SIZE) {

        CurrentBaseOffset = PathContext->BaseOffset + SECTOR_SIZE;

        CdMapPathTableBlock( IrpContext,
                             IrpContext->Vcb->PathTableFcb,
                             CurrentBaseOffset,
                             PathContext );

        //
        //  Set up our current offset into the Path Context.
        //

        PathContext->DataOffset -= SECTOR_SIZE;
    }

    //
    //  Now update the path entry with the values from the on-disk
    //  structure.
    //
        
    return CdUpdatePathEntryFromRawPathEntry( IrpContext,
                                              PathEntry->Ordinal + 1,
                                              TRUE,
                                              PathContext,
                                              PathEntry );
}


BOOLEAN
CdFindPathEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB ParentFcb,
    IN PCD_NAME DirName,
    IN BOOLEAN IgnoreCase,
    IN OUT PCOMPOUND_PATH_ENTRY CompoundPathEntry
    )

/*++

Routine Description:

    This routine will walk through the path table looking for a matching entry for DirName
    among the child directories of the ParentFcb.

Arguments:

    ParentFcb - This is the directory we are examining.  We know the ordinal and path table
        offset for this directory in the path table.  If this is the first scan for this
        Fcb we will update the first child offset for this directory in the path table.

    DirName - This is the name we are searching for.  This name will not contain wildcard
        characters.  The name will also not have a version string.

    IgnoreCase - Indicates if this search is exact or ignore case.

    CompoundPathEntry - Complete path table enumeration structure.  We will have initialized
        it for the search on entry.  This will be positioned at the matching name if found.

Return Value:

    BOOLEAN - TRUE if matching entry found, FALSE otherwise.

--*/

{
    BOOLEAN Found = FALSE;
    BOOLEAN UpdateChildOffset = TRUE;

    ULONG StartingOffset;
    ULONG StartingOrdinal;

    PAGED_CODE();

    //
    //  Position ourselves at either the first child or at the directory itself.
    //  Lock the Fcb to get this value and remember whether to update with the first
    //  child.
    //

    StartingOffset = CdQueryFidPathTableOffset( ParentFcb->FileId );
    StartingOrdinal = ParentFcb->Ordinal;

	//
	//  ISO 9660 9.4.4 restricts the backpointer from child to parent in a
	//  pathtable entry to 16bits. Although we internally store ordinals
	//  as 32bit values, it is impossible to search for the children of a
	//  directory whose ordinal value is greater than MAXUSHORT. Media that
	//  could induce such a search is illegal.
	//
	//  Note that it is not illegal to have more than MAXUSHORT directories.
	//

	if (ParentFcb->Ordinal > MAXUSHORT) {

		CdRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR );
	}

    CdLockFcb( IrpContext, ParentFcb );

    if (ParentFcb->ChildPathTableOffset != 0) {

        StartingOffset = ParentFcb->ChildPathTableOffset;
        StartingOrdinal = ParentFcb->ChildOrdinal;
        UpdateChildOffset = FALSE;

    } else if (ParentFcb == ParentFcb->Vcb->RootIndexFcb) {

        UpdateChildOffset = FALSE;
    }

    CdUnlockFcb( IrpContext, ParentFcb );

    CdLookupPathEntry( IrpContext, StartingOffset, StartingOrdinal, FALSE, CompoundPathEntry );

    //
    //  Loop until we find a match or are beyond the children for this directory.
    //

    do {

        //
        //  If we are beyond this directory then return FALSE.
        //

        if (CompoundPathEntry->PathEntry.ParentOrdinal > ParentFcb->Ordinal) {

            //
            //  Update the Fcb with the offsets for the children in the path table.
            //

            if (UpdateChildOffset) {

                CdLockFcb( IrpContext, ParentFcb );

                ParentFcb->ChildPathTableOffset = StartingOffset;
                ParentFcb->ChildOrdinal = StartingOrdinal;

                CdUnlockFcb( IrpContext, ParentFcb );
            }

            break;
        }

        //
        //  If we are within the children of this directory then check for a match.
        //

        if (CompoundPathEntry->PathEntry.ParentOrdinal == ParentFcb->Ordinal) {

            //
            //  Update the child offset if not yet done.
            //

            if (UpdateChildOffset) {

                CdLockFcb( IrpContext, ParentFcb );

                ParentFcb->ChildPathTableOffset = CompoundPathEntry->PathEntry.PathTableOffset;
                ParentFcb->ChildOrdinal = CompoundPathEntry->PathEntry.Ordinal;

                CdUnlockFcb( IrpContext, ParentFcb );

                UpdateChildOffset = FALSE;
            }

            //
            //  Update the name in the path entry.
            //

            CdUpdatePathEntryName( IrpContext, &CompoundPathEntry->PathEntry, IgnoreCase );

            //
            //  Now compare the names for an exact match.
            //

            if (CdIsNameInExpression( IrpContext,
                                      &CompoundPathEntry->PathEntry.CdCaseDirName,
                                      DirName,
                                      0,
                                      FALSE )) {

                //
                //  Let our caller know we have a match.
                //

                Found = TRUE;
                break;
            }
        }

        //
        //  Go to the next entry in the path table.  Remember the current position
        //  in the event we update the Fcb.
        //

        StartingOffset = CompoundPathEntry->PathEntry.PathTableOffset;
        StartingOrdinal = CompoundPathEntry->PathEntry.Ordinal;

    } while (CdLookupNextPathEntry( IrpContext,
                                    &CompoundPathEntry->PathContext,
                                    &CompoundPathEntry->PathEntry ));

    return Found;
}


//
//  Local support routine
//

VOID
CdMapPathTableBlock (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN LONGLONG BaseOffset,
    IN OUT PPATH_ENUM_CONTEXT PathContext
    )

/*++

Routine Description:

    This routine is called to map (or allocate and copy) the next
    data block in the path table.  We check if the next block will
    span a view boundary and allocate an auxilary buffer in that case.

Arguments:

    Fcb - This is the Fcb for the Path Table.

    BaseOffset - Offset of the first sector to map.  This will be on a
        sector boundary.

    PathContext - Enumeration context to update in this routine.

Return Value:

    None.

--*/

{
    ULONG CurrentLength;
    ULONG SectorSize;
    ULONG DataOffset;
    ULONG PassCount;
    PVOID Sector;

    PAGED_CODE();

    //
    //  Map the new block and set the enumeration context to this
    //  point.  Allocate an auxilary buffer if necessary.
    //

    CurrentLength = 2 * SECTOR_SIZE;

    if (CurrentLength >= (ULONG) (Fcb->FileSize.QuadPart - BaseOffset)) {

        CurrentLength = (ULONG) (Fcb->FileSize.QuadPart - BaseOffset);

        //
        //  We know this is the last data block for this
        //  path table.
        //

        PathContext->LastDataBlock = TRUE;
    }

    //
    //  Set context values.
    //

    PathContext->BaseOffset = (ULONG) BaseOffset;
    PathContext->DataLength = CurrentLength;

    //
    //  Drop the previous sector's mapping
    //

    CdUnpinData( IrpContext, &PathContext->Bcb );

    //
    //  Check if spanning a view section.  The following must
    //  be true before we take this step.
    //
    //      Data length is more than one sector.
    //      Starting offset must be one sector before the
    //          cache manager VACB boundary.
    //

    if ((CurrentLength > SECTOR_SIZE) &&
        (FlagOn( ((ULONG) BaseOffset), VACB_MAPPING_MASK ) == LAST_VACB_SECTOR_OFFSET )) {

        //
        //  Map each sector individually and store into an auxilary
        //  buffer.
        //

        SectorSize = SECTOR_SIZE;
        DataOffset = 0;
        PassCount = 2;

        PathContext->Data = FsRtlAllocatePoolWithTag( CdPagedPool,
                                                      CurrentLength,
                                                      TAG_SPANNING_PATH_TABLE );
        PathContext->AllocatedData = TRUE;

        while (PassCount--) {

            CcMapData( Fcb->FileObject,
                       (PLARGE_INTEGER) &BaseOffset,
                       SectorSize,
                       TRUE,
                       &PathContext->Bcb,
                       &Sector );

            RtlCopyMemory( Add2Ptr( PathContext->Data, DataOffset, PVOID ),
                           Sector,
                           SectorSize );

            CdUnpinData( IrpContext, &PathContext->Bcb );

            BaseOffset += SECTOR_SIZE;
            SectorSize = CurrentLength - SECTOR_SIZE;
            DataOffset = SECTOR_SIZE;
        }

    //
    //  Otherwise we can just map the data into the cache.
    //

    } else {

        //
        //  There is a slight chance that we have allocated an
        //  auxilary buffer on the previous sector.
        //

        if (PathContext->AllocatedData) {

            CdFreePool( &PathContext->Data );
            PathContext->AllocatedData = FALSE;
        }

        CcMapData( Fcb->FileObject,
                   (PLARGE_INTEGER) &BaseOffset,
                   CurrentLength,
                   TRUE,
                   &PathContext->Bcb,
                   &PathContext->Data );
    }

    return;
}


//
//  Local support routine
//

BOOLEAN
CdUpdatePathEntryFromRawPathEntry (
    IN PIRP_CONTEXT IrpContext,
    IN ULONG Ordinal,
    IN BOOLEAN VerifyBounds,
    IN PPATH_ENUM_CONTEXT PathContext,
    OUT PPATH_ENTRY PathEntry
    )

/*++

Routine Description:

    This routine is called to update the in-memory Path Entry from the on-disk
    path entry.  We also do a careful check of the bounds if requested and we
    are in the last data block of the path table.

Arguments:

    Ordinal - Ordinal number for this directory.

    VerifyBounds - Check that the current raw Path Entry actually fits
        within the data block.

    PathContext - Current path table enumeration context.

    PathEntry - Pointer to the in-memory path entry structure.

Return Value:

    TRUE  if updated ok,  
    FALSE if we've hit the end of the pathtable - zero name length && PT size is a multiple
          of blocksize.  This is a workaround for some Video CDs.  Win 9x works around this.

    This routine may raise.

--*/

{
    PRAW_PATH_ENTRY RawPathEntry = CdRawPathEntry( IrpContext, PathContext );
    ULONG RemainingDataLength;

    PAGED_CODE();
    
    //
    //  Check for a name length of zero.  This is the first byte of the record,
    //  and there must be at least one byte remaining in the buffer else we 
    //  wouldn't be here (caller would have spotted buffer end).
    //
    
    PathEntry->DirNameLen = CdRawPathIdLen( IrpContext, RawPathEntry );
    
    if (0 == PathEntry->DirNameLen) {

        //
        //  If we are in the last block,  and the path table size (ie last block) is a 
        //  multiple of block size,  then we will consider this the end of the path table
        //  rather than raising an error.  Workaround for NTI Cd Maker video CDs which
        //  round path table length to blocksize multiple.  In all other cases we consider
        //  a zero length name to be corruption.
        //
        
        if ( PathContext->LastDataBlock && 
             (0 == BlockOffset( IrpContext->Vcb, PathContext->DataLength)))  {
        
            return FALSE;
        }
        
        CdRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR );
    }

    //
    //  Check if we should verify the path entry.  If we are not in the last
    //  data block then there is nothing to check.
    //
    
    if (PathContext->LastDataBlock && VerifyBounds) {

        //
        //  Quick check to see if the maximum size is still available.  This
        //  will handle most cases and we don't need to access any of the
        //  fields.
        //

        RemainingDataLength = PathContext->DataLength - PathContext->DataOffset;

        if (RemainingDataLength < sizeof( RAW_PATH_ENTRY )) {

            //
            //  Make sure the remaining bytes hold the path table entries.
            //  Do the following checks.
            //
            //      - A minimal path table entry will fit (and then check)
            //      - This path table entry (with dir name) will fit.
            //

            if ((RemainingDataLength < MIN_RAW_PATH_ENTRY_LEN) ||
                (RemainingDataLength < (ULONG) (CdRawPathIdLen( IrpContext, RawPathEntry ) + MIN_RAW_PATH_ENTRY_LEN - 1))) {

                CdRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR );
            }
        }
    }

    //
    //  The ordinal number of this directory is passed in.
    //  Compute the path table offset of this entry.
    //

    PathEntry->Ordinal = Ordinal;
    PathEntry->PathTableOffset = PathContext->BaseOffset + PathContext->DataOffset;

    //
    //  We know we can safely access all of the fields of the raw path table at
    //  this point.
    
    //
    //  Bias the disk offset by the number of logical blocks
    //

    CopyUchar4( &PathEntry->DiskOffset, CdRawPathLoc( IrpContext, RawPathEntry ));

    PathEntry->DiskOffset += CdRawPathXar( IrpContext, RawPathEntry );

    CopyUchar2( &PathEntry->ParentOrdinal, &RawPathEntry->ParentNum );

    PathEntry->PathEntryLength = PathEntry->DirNameLen + MIN_RAW_PATH_ENTRY_LEN - 1;

    //
    //  Align the path entry length on a ushort boundary.
    //

    PathEntry->PathEntryLength = WordAlign( PathEntry->PathEntryLength );

    PathEntry->DirName = RawPathEntry->DirId;

    return TRUE;
}


//
//  Local support routine
//

VOID
CdUpdatePathEntryName (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PPATH_ENTRY PathEntry,
    IN BOOLEAN IgnoreCase
    )

/*++

Routine Description:

    This routine will store the directory name into the CdName in the
    path entry.  If this is a Joliet name then we will make sure we have
    an allocated buffer and need to convert from big endian to little
    endian.  We also correctly update the case name.  If this operation is ignore
    case then we need an auxilary buffer for the name.

    For an Ansi disk we can use the name from the disk for the exact case.  We only
    need to allocate a buffer for the ignore case name.  The on-disk representation of
    a Unicode name is useless for us.  In this case we will need a name buffer for
    both names.  We store a buffer in the PathEntry which can hold two 8.3 unicode
    names.  This means we will almost never need to allocate a buffer in the Ansi case
    (we only need one buffer and already have 48 characters).

Arguments:

    PathEntry - Pointer to a path entry structure.  We have already updated
        this path entry with the values from the raw path entry.

Return Value:

    None.

--*/

{
    ULONG Length;
    NTSTATUS Status;

    PAGED_CODE();

    //
    //  Check if this is a self entry.  We use a fixed string for this.
    //
    //      Self-Entry - Length is 1, value is 0.
    //

    if ((*PathEntry->DirName == 0) &&
        (PathEntry->DirNameLen == 1)) {

        //
        //  There should be no allocated buffers.
        //

        ASSERT( !FlagOn( PathEntry->Flags, PATH_ENTRY_FLAG_ALLOC_BUFFER ));

        //
        //  Now use one of the hard coded directory names.
        //

        PathEntry->CdDirName.FileName = CdUnicodeDirectoryNames[0];

        //
        //  Show that there is no version number.
        //

        PathEntry->CdDirName.VersionString.Length = 0;

        //
        //  The case name is identical.
        //

        PathEntry->CdCaseDirName = PathEntry->CdDirName;

        //
        //  Return now.
        //

        return;
    }

    //
    //  Compute how large a buffer we will need.  If this is an ignore
    //  case operation then we will want a double size buffer.  If the disk is not
    //  a Joliet disk then we might need two bytes for each byte in the name.
    //

    Length = PathEntry->DirNameLen;

    if (IgnoreCase) {

        Length *= 2;
    }

    if (!FlagOn( IrpContext->Vcb->VcbState, VCB_STATE_JOLIET )) {

        Length *= sizeof( WCHAR );
    }

    //
    //  Now decide if we need to allocate a new buffer.  We will if
    //  this name won't fit in the embedded name buffer and it is
    //  larger than the current allocated buffer.  We always use the
    //  allocated buffer if present.
    //
    //  If we haven't allocated a buffer then use the embedded buffer if the data
    //  will fit.  This is the typical case.
    //

    if (!FlagOn( PathEntry->Flags, PATH_ENTRY_FLAG_ALLOC_BUFFER ) &&
        (Length <= sizeof( PathEntry->NameBuffer ))) {

        PathEntry->CdDirName.FileName.MaximumLength = sizeof( PathEntry->NameBuffer );
        PathEntry->CdDirName.FileName.Buffer = PathEntry->NameBuffer;

    } else {

        //
        //  We need to use an allocated buffer.  Check if the current buffer
        //  is large enough.
        //

        if (Length > PathEntry->CdDirName.FileName.MaximumLength) {

            //
            //  Free any allocated buffer.
            //

            if (FlagOn( PathEntry->Flags, PATH_ENTRY_FLAG_ALLOC_BUFFER )) {

                CdFreePool( &PathEntry->CdDirName.FileName.Buffer );
                ClearFlag( PathEntry->Flags, PATH_ENTRY_FLAG_ALLOC_BUFFER );
            }

            PathEntry->CdDirName.FileName.Buffer = FsRtlAllocatePoolWithTag( CdPagedPool,
                                                                             Length,
                                                                             TAG_PATH_ENTRY_NAME );

            SetFlag( PathEntry->Flags, PATH_ENTRY_FLAG_ALLOC_BUFFER );

            PathEntry->CdDirName.FileName.MaximumLength = (USHORT) Length;
        }
    }

    //
    //  We now have a buffer for the name.  We need to either convert the on-disk bigendian
    //  to little endian or covert the name to Unicode.
    //

    if (!FlagOn( IrpContext->Vcb->VcbState, VCB_STATE_JOLIET )) {

        Status = RtlOemToUnicodeN( PathEntry->CdDirName.FileName.Buffer,
                                   PathEntry->CdDirName.FileName.MaximumLength,
                                   &Length,
                                   PathEntry->DirName,
                                   PathEntry->DirNameLen );

        ASSERT( Status == STATUS_SUCCESS );
        PathEntry->CdDirName.FileName.Length = (USHORT) Length;

    } else {

        //
        //  Convert this string to little endian.
        //

        CdConvertBigToLittleEndian( IrpContext,
                                    PathEntry->DirName,
                                    PathEntry->DirNameLen,
                                    (PCHAR) PathEntry->CdDirName.FileName.Buffer );

        PathEntry->CdDirName.FileName.Length = (USHORT) PathEntry->DirNameLen;
    }

    //
    //  There is no version string.
    //

    PathEntry->CdDirName.VersionString.Length =
    PathEntry->CdCaseDirName.VersionString.Length = 0;

    //
    //  If the name string ends with a period then knock off the last
    //  character.
    //

    if (PathEntry->CdDirName.FileName.Buffer[(PathEntry->CdDirName.FileName.Length - sizeof( WCHAR )) / 2] == L'.') {

        //
        //  Shrink the filename length.
        //

        PathEntry->CdDirName.FileName.Length -= sizeof( WCHAR );
    }

    //
    //  Update the case name buffer if necessary.  If this is an exact case
    //  operation then just copy the exact case string.
    //

    if (IgnoreCase) {

        PathEntry->CdCaseDirName.FileName.Buffer = Add2Ptr( PathEntry->CdDirName.FileName.Buffer,
                                                            PathEntry->CdDirName.FileName.MaximumLength / 2,
                                                            PWCHAR);

        PathEntry->CdCaseDirName.FileName.MaximumLength = PathEntry->CdDirName.FileName.MaximumLength / 2;

        CdUpcaseName( IrpContext,
                      &PathEntry->CdDirName,
                      &PathEntry->CdCaseDirName );

    } else {

        PathEntry->CdCaseDirName = PathEntry->CdDirName;
    }

    return;
}


