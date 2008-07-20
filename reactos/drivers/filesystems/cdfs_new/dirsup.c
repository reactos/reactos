/*++

Copyright (c) 1989-2000 Microsoft Corporation

Module Name:

    DirSup.c

Abstract:

    This module implements the dirent support routines for Cdfs.

    Directories on a CD consist of a number of contiguous sectors on
    the disk.  File descriptors consist of one or more directory entries
    (dirents) within a directory.  Files may contain version numbers.  If
    present all like-named files will be ordered contiguously in the
    directory by decreasing version numbers.  We will only return the
    first of these on a directory query unless the user explicitly
    asks for version numbers.  Finally dirents will not span sector
    boundaries.  Unused bytes at the end of a sector will be zero
    filled.

    Directory sector:                                                   Offset
                                                                        2048
        +---------------------------------------------------------------+
        |            |          |          |           |          |     |
        | foo;4      | foo;4    | foo;3    |  hat      |  zebra   | Zero|
        |            |          |          |           |          | Fill|
        |            |  final   |  single  |           |          |     |
        |            |  extent  |   extent |           |          |     |
        +---------------------------------------------------------------+

    Dirent operations:

        - Position scan at known offset in directory.  Dirent at this
            offset must exist and is valid.  Used when scanning a directory
            from the beginning when the self entry is known to be valid.
            Used when positioning at the first dirent for an open
            file to scan the allocation information.  Used when resuming
            a directory enumeration from a valid directory entry.

        - Position scan at known offset in directory.  Dirent is known to
            start at this position but must be checked for validity.
            Used to read the self-directory entry.

        - Move to the next dirent within a directory.

        - Given a known starting dirent, collect all the dirents for
            that file.  Scan will finish positioned at the last dirent
            for the file.  We will accumulate the extent lengths to
            find the size of the file.

        - Given a known starting dirent, position the scan for the first
            dirent of the following file.  Used when not interested in
            all of the details for the current file and are looking for
            the next file.

        - Update a common dirent structure with the details of the on-disk
            structure.  This is used to smooth out the differences

        - Build the filename (name and version strings) out of the stream
            of bytes in the file name on disk.  For Joliet disks we will have
            to convert to little endian.


--*/

#include "CdProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (CDFS_BUG_CHECK_DIRSUP)

//
//  Local macros
//

//
//  PRAW_DIRENT
//  CdRawDirent (
//      IN PIRP_CONTEXT IrpContext,
//      IN PDIR_ENUM_CONTEXT DirContext
//      );
//

#define CdRawDirent(IC,DC)                                      \
    Add2Ptr( (DC)->Sector, (DC)->SectorOffset, PRAW_DIRENT )

//
//  Local support routines
//

ULONG
CdCheckRawDirentBounds (
    IN PIRP_CONTEXT IrpContext,
    IN PDIRENT_ENUM_CONTEXT DirContext
    );

XA_EXTENT_TYPE
CdCheckForXAExtent (
    IN PIRP_CONTEXT IrpContext,
    IN PRAW_DIRENT RawDirent,
    IN OUT PDIRENT Dirent
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CdCheckForXAExtent)
#pragma alloc_text(PAGE, CdCheckRawDirentBounds)
#pragma alloc_text(PAGE, CdCleanupFileContext)
#pragma alloc_text(PAGE, CdFindFile)
#pragma alloc_text(PAGE, CdFindDirectory)
#pragma alloc_text(PAGE, CdFindFileByShortName)
#pragma alloc_text(PAGE, CdLookupDirent)
#pragma alloc_text(PAGE, CdLookupLastFileDirent)
#pragma alloc_text(PAGE, CdLookupNextDirent)
#pragma alloc_text(PAGE, CdLookupNextInitialFileDirent)
#pragma alloc_text(PAGE, CdUpdateDirentFromRawDirent)
#pragma alloc_text(PAGE, CdUpdateDirentName)
#endif


VOID
CdLookupDirent (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ULONG DirentOffset,
    OUT PDIRENT_ENUM_CONTEXT DirContext
    )

/*++

Routine Description:

    This routine is called to initiate a walk through a directory.  We will
    position ourselves in the directory at offset DirentOffset.  We know that
    a dirent begins at this boundary but may have to verify the dirent bounds.
    We will call this routine when looking up the first entry of a known
    file or verifying the self entry of a directory.

Arguments:

    Fcb - Fcb for the directory being traversed.

    DirentOffset - This is our target point in the directory.  We will map the
        page containing this entry and possibly verify the dirent bounds at
        this location.

    DirContext - This is the dirent context for this scan.  We update it with
        the location of the dirent we found.  This structure has been initialized
        outside of this call.

Return Value:

    None.

--*/

{
    LONGLONG BaseOffset;

    PAGED_CODE();

    //
    //  Initialize the offset of the first dirent we want to map.
    //

    DirContext->BaseOffset = SectorTruncate( DirentOffset );
    BaseOffset = DirContext->BaseOffset;

    DirContext->DataLength = SECTOR_SIZE;

    DirContext->SectorOffset = SectorOffset( DirentOffset );

    //
    //  Truncate the data length if we are at the end of the file.
    //

    if (DirContext->DataLength > (Fcb->FileSize.QuadPart - BaseOffset)) {

        DirContext->DataLength = (ULONG) (Fcb->FileSize.QuadPart - BaseOffset);
    }

    //
    //  Now map the data at this offset.
    //

    CcMapData( Fcb->FileObject,
               (PLARGE_INTEGER) &BaseOffset,
               DirContext->DataLength,
               TRUE,
               &DirContext->Bcb,
               &DirContext->Sector );

    //
    //  Verify the dirent bounds.
    //

    DirContext->NextDirentOffset = CdCheckRawDirentBounds( IrpContext,
                                                           DirContext );

    return;
}


BOOLEAN
CdLookupNextDirent (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PDIRENT_ENUM_CONTEXT CurrentDirContext,
    OUT PDIRENT_ENUM_CONTEXT NextDirContext
    )

/*++

Routine Description:

    This routine is called to find the next dirent in the directory.  The
    current position is given and we look for the next.  We leave the context
    for the starting position untouched and update the context for the
    dirent we found.  The target context may already be initialized so we
    may already have the sector in memory.

    This routine will position the enumeration context for the next dirent and
    verify the dirent bounds.

    NOTE - This routine can be called with CurrentDirContext and NextDirContext
        pointing to the same enumeration context.

Arguments:

    Fcb - Fcb for the directory being traversed.

    CurrentDirContext - This is the dirent context for this scan.  We update
        it with the location of the dirent we found.  This is currently
        pointing to a dirent location.  The dirent bounds at this location
        have already been verified.

    NextDirContext - This is the dirent context to update with the dirent we
        find.  This may already point to a dirent so we need to check if
        we are in the same sector and unmap any buffer as necessary.

        This dirent is left in an indeterminant state if we don't find a dirent.

Return Value:

    BOOLEAN - TRUE if we find a location for the next dirent, FALSE otherwise.
        This routine can cause a raise if the directory is corrupt.

--*/

{
    LONGLONG CurrentBaseOffset = CurrentDirContext->BaseOffset;
    ULONG TempUlong;

    BOOLEAN FoundDirent = FALSE;

    PAGED_CODE();

    //
    //  Check if a different sector is mapped.  If so then move our target
    //  enumeration context to the same sector.
    //

    if ((CurrentDirContext->BaseOffset != NextDirContext->BaseOffset) ||
        (NextDirContext->Bcb == NULL)) {

        //
        //  Unpin the current target Bcb and map the next sector.
        //

        CdUnpinData( IrpContext, &NextDirContext->Bcb );

        CcMapData( Fcb->FileObject,
                   (PLARGE_INTEGER) &CurrentBaseOffset,
                   CurrentDirContext->DataLength,
                   TRUE,
                   &NextDirContext->Bcb,
                   &NextDirContext->Sector );

        //
        //  Copy the data length and sector offset.
        //

        NextDirContext->DataLength = CurrentDirContext->DataLength;
        NextDirContext->BaseOffset = CurrentDirContext->BaseOffset;
    }

    //
    //  Now move to the same offset in the sector.
    //

    NextDirContext->SectorOffset = CurrentDirContext->SectorOffset;

    //
    //  If the value is zero then unmap the current sector and set up
    //  the base offset to the beginning of the next sector.
    //

    if (CurrentDirContext->NextDirentOffset == 0) {

        CurrentBaseOffset = NextDirContext->BaseOffset + NextDirContext->DataLength;

        //
        //  Unmap the current sector.  We test the value of the Bcb in the
        //  loop below to see if we need to read in another sector.
        //

        CdUnpinData( IrpContext, &NextDirContext->Bcb );

    //
    //  There is another possible dirent in the current sector.  Update the
    //  enumeration context to reflect this.
    //

    } else {

        NextDirContext->SectorOffset += CurrentDirContext->NextDirentOffset;
    }

    //
    //  Now loop until we find the next possible dirent or walk off the directory.
    //

    while (TRUE) {

        //
        //  If we don't currently have a sector mapped then map the
        //  directory at the current offset.
        //

        if (NextDirContext->Bcb == NULL) {

            TempUlong = SECTOR_SIZE;

            if (TempUlong > (ULONG) (Fcb->FileSize.QuadPart - CurrentBaseOffset)) {

                TempUlong = (ULONG) (Fcb->FileSize.QuadPart - CurrentBaseOffset);

                //
                //  If the length is zero then there is no dirent.
                //

                if (TempUlong == 0) {

                    break;
                }
            }

            CcMapData( Fcb->FileObject,
                       (PLARGE_INTEGER) &CurrentBaseOffset,
                       TempUlong,
                       TRUE,
                       &NextDirContext->Bcb,
                       &NextDirContext->Sector );

            NextDirContext->BaseOffset = (ULONG) CurrentBaseOffset;
            NextDirContext->SectorOffset = 0;
            NextDirContext->DataLength = TempUlong;
        }

        //
        //  The CDFS spec allows for sectors in a directory to contain all zeroes.
        //  In this case we need to move to the next sector.  So look at the
        //  current potential dirent for a zero length.  Move to the next
        //  dirent if length is zero.
        //

        if (*((PCHAR) CdRawDirent( IrpContext, NextDirContext )) != 0) {

            FoundDirent = TRUE;
            break;
        }

        CurrentBaseOffset = NextDirContext->BaseOffset + NextDirContext->DataLength;
        CdUnpinData( IrpContext, &NextDirContext->Bcb );
    }

    //
    //  Check the dirent bounds if we found a dirent.
    //

    if (FoundDirent) {

        NextDirContext->NextDirentOffset = CdCheckRawDirentBounds( IrpContext,
                                                                   NextDirContext );
    }

    return FoundDirent;
}


VOID
CdUpdateDirentFromRawDirent (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PDIRENT_ENUM_CONTEXT DirContext,
    IN OUT PDIRENT Dirent
    )

/*++

Routine Description:

    This routine is called to safely copy the data from the dirent on disk
    to the in-memory dirent.  The fields on disk are unaligned so we
    need to safely copy them to our structure.

Arguments:

    Fcb - Fcb for the directory being scanned.

    DirContext - Enumeration context for the raw disk dirent.

    Dirent - In-memory dirent to update.

Return Value:

    None.

--*/

{
    PRAW_DIRENT RawDirent = CdRawDirent( IrpContext, DirContext );

    PAGED_CODE();

    //
    //  Clear all of the current state flags except the flag indicating that
    //  we allocated a name string.
    //

    ClearFlag( Dirent->Flags, DIRENT_FLAG_NOT_PERSISTENT );

    //
    //  The dirent offset is the sum of the start of the sector and the
    //  sector offset.
    //

    Dirent->DirentOffset = DirContext->BaseOffset + DirContext->SectorOffset;

    //
    //  Copy the dirent length from the raw dirent.
    //

    Dirent->DirentLength = RawDirent->DirLen;

    //
    //  The starting offset on disk is computed by finding the starting
    //  logical block and stepping over the Xar block.
    //

    CopyUchar4( &Dirent->StartingOffset, RawDirent->FileLoc );

    Dirent->StartingOffset += RawDirent->XarLen;

    //
    //  Do a safe copy to get the data length.
    //

    CopyUchar4( &Dirent->DataLength, RawDirent->DataLen );

    //
    //  Save a pointer to the time stamps.
    //

    Dirent->CdTime = RawDirent->RecordTime;

    //
    //  Copy the dirent flags.
    //

    Dirent->DirentFlags = CdRawDirentFlags( IrpContext, RawDirent );

    //
    //  For both the file unit and interleave skip we want to take the
    //  logical block count.
    //

    Dirent->FileUnitSize =
    Dirent->InterleaveGapSize = 0;

    if (RawDirent->IntLeaveSize != 0) {

        Dirent->FileUnitSize = RawDirent->IntLeaveSize;
        Dirent->InterleaveGapSize = RawDirent->IntLeaveSkip;
    }

    //
    //  Get the name length and remember a pointer to the start of the
    //  name string.  We don't do any processing on the name at this
    //  point.
    //
    //  Check that the name length is non-zero.
    //

    if (RawDirent->FileIdLen == 0) {

        CdRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
    }

    Dirent->FileNameLen = RawDirent->FileIdLen;
    Dirent->FileName = RawDirent->FileId;

    //
    //  If there are any remaining bytes at the end of the dirent then
    //  there may be a system use area.  We protect ourselves from
    //  disks which don't pad the dirent entries correctly by using
    //  a fudge factor of one.  All system use areas must have a length
    //  greater than one.  Don't bother with the system use area
    //  if this is a directory.
    //

    Dirent->XAAttributes = 0;
    Dirent->XAFileNumber = 0;
    Dirent->ExtentType = Form1Data;
    Dirent->SystemUseOffset = 0;

    if (!FlagOn( Dirent->DirentFlags, CD_ATTRIBUTE_DIRECTORY ) &&
        (Dirent->DirentLength > ((FIELD_OFFSET( RAW_DIRENT, FileId ) + Dirent->FileNameLen) + 1))) {

        Dirent->SystemUseOffset = WordAlign( FIELD_OFFSET( RAW_DIRENT, FileId ) + Dirent->FileNameLen );
    }

    return;
}


VOID
CdUpdateDirentName (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PDIRENT Dirent,
    IN ULONG IgnoreCase
    )

/*++

Routine Description:

    This routine is called to update the name in the dirent with the name
    from the disk.  We will look for the special case of the self and
    parent entries and also construct the Unicode name for the Joliet disk
    in order to work around the BigEndian on-disk structure.

Arguments:

    Dirent - Pointer to the in-memory dirent structure.

    IgnoreCase - TRUE if we should build the upcased version.  Otherwise we
        use the exact case name.

Return Value:

    None.

--*/

{
    UCHAR DirectoryValue;
    ULONG Length;

    NTSTATUS Status;

    PAGED_CODE();

    //
    //  Check if this is a self or parent entry.  There is no version number
    //  in these cases.  We use a fixed string for these.
    //
    //      Self-Entry - Length is 1, value is 0.
    //      Parent-Entry - Length is 1, value is 1.
    //

    if ((Dirent->FileNameLen == 1) &&
        FlagOn( Dirent->DirentFlags, CD_ATTRIBUTE_DIRECTORY )) {

        DirectoryValue = *((PCHAR) Dirent->FileName);

        if ((DirectoryValue == 0) || (DirectoryValue == 1)) {

            //
            //  We should not have allocated a name by the time we see these cases.
            //  If we have, this means that the image is in violation of ISO 9660 7.6.2,
            //  which states that the ./.. entries must be the first two in the directory.
            //

            if (FlagOn( Dirent->Flags, DIRENT_FLAG_ALLOC_BUFFER )) {

                CdRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
            }

            //
            //  Now use one of the hard coded directory names.
            //

            Dirent->CdFileName.FileName = CdUnicodeDirectoryNames[DirectoryValue];

            //
            //  Show that there is no version number.
            //

            Dirent->CdFileName.VersionString.Length = 0;

            //
            //  The case name is the same as the exact name.
            //

            Dirent->CdCaseFileName = Dirent->CdFileName;

            //
            //  Mark this as a constant value entry.
            //

            SetFlag( Dirent->Flags, DIRENT_FLAG_CONSTANT_ENTRY );

            //
            //  Return now.
            //

            return;
        }
    }

    //
    //  Mark this as a non-constant value entry.
    //

    ClearFlag( Dirent->Flags, DIRENT_FLAG_CONSTANT_ENTRY );

    //
    //  Compute how large a buffer we will need.  If this is an ignore
    //  case operation then we will want a double size buffer.  If the disk is not
    //  a Joliet disk then we might need two bytes for each byte in the name.
    //

    Length = Dirent->FileNameLen;

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

    if (!FlagOn( Dirent->Flags, DIRENT_FLAG_ALLOC_BUFFER ) &&
        (Length <= sizeof( Dirent->NameBuffer ))) {

        Dirent->CdFileName.FileName.MaximumLength = sizeof( Dirent->NameBuffer );
        Dirent->CdFileName.FileName.Buffer = Dirent->NameBuffer;

    } else {

        //
        //  We need to use an allocated buffer.  Check if the current buffer
        //  is large enough.
        //

        if (Length > Dirent->CdFileName.FileName.MaximumLength) {

            //
            //  Free any allocated buffer.
            //

            if (FlagOn( Dirent->Flags, DIRENT_FLAG_ALLOC_BUFFER )) {

                CdFreePool( &Dirent->CdFileName.FileName.Buffer );
                ClearFlag( Dirent->Flags, DIRENT_FLAG_ALLOC_BUFFER );
            }

            Dirent->CdFileName.FileName.Buffer = FsRtlAllocatePoolWithTag( CdPagedPool,
                                                                            Length,
                                                                            TAG_DIRENT_NAME );

            SetFlag( Dirent->Flags, DIRENT_FLAG_ALLOC_BUFFER );

            Dirent->CdFileName.FileName.MaximumLength = (USHORT) Length;
        }
    }

    //
    //  We now have a buffer for the name.  We need to either convert the on-disk bigendian
    //  to little endian or covert the name to Unicode.
    //

    if (!FlagOn( IrpContext->Vcb->VcbState, VCB_STATE_JOLIET )) {

        Status = RtlOemToUnicodeN( Dirent->CdFileName.FileName.Buffer,
                                   Dirent->CdFileName.FileName.MaximumLength,
                                   &Length,
                                   Dirent->FileName,
                                   Dirent->FileNameLen );

        ASSERT( Status == STATUS_SUCCESS );
        Dirent->CdFileName.FileName.Length = (USHORT) Length;

    } else {

        //
        //  Convert this string to little endian.
        //

        CdConvertBigToLittleEndian( IrpContext,
                                    Dirent->FileName,
                                    Dirent->FileNameLen,
                                    (PCHAR) Dirent->CdFileName.FileName.Buffer );

        Dirent->CdFileName.FileName.Length = (USHORT) Dirent->FileNameLen;
    }

    //
    //  Split the name into name and version strings.
    //

    CdConvertNameToCdName( IrpContext,
                           &Dirent->CdFileName );

    //
    //  The name length better be non-zero.
    //

    if (Dirent->CdFileName.FileName.Length == 0) {

        CdRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
    }

    //
    //  If the filename ends with a period then back up one character.
    //

    if (Dirent->CdFileName.FileName.Buffer[(Dirent->CdFileName.FileName.Length - sizeof( WCHAR )) / 2] == L'.') {

        //
        //  Slide the version string down.
        //

        if (Dirent->CdFileName.VersionString.Length != 0) {

            PWCHAR NewVersion;

            //
            //  Start from the position currently containing the separator.
            //

            NewVersion = Add2Ptr( Dirent->CdFileName.FileName.Buffer,
                                  Dirent->CdFileName.FileName.Length,
                                  PWCHAR );

            //
            //  Now overwrite the period.
            //

            RtlMoveMemory( NewVersion - 1,
                           NewVersion,
                           Dirent->CdFileName.VersionString.Length + sizeof( WCHAR ));

            //
            //  Now point to the new version string.
            //

            Dirent->CdFileName.VersionString.Buffer = NewVersion;
        }

        //
        //  Shrink the filename length.
        //

        Dirent->CdFileName.FileName.Length -= sizeof( WCHAR );
    }

    //
    //  If this an exact case operation then use the filename exactly.
    //

    if (!IgnoreCase) {

        Dirent->CdCaseFileName = Dirent->CdFileName;

    //
    //  Otherwise perform our upcase operation.  We already have guaranteed the buffers are
    //  there.
    //

    } else {

        Dirent->CdCaseFileName.FileName.Buffer = Add2Ptr( Dirent->CdFileName.FileName.Buffer,
                                                          Dirent->CdFileName.FileName.MaximumLength / 2,
                                                          PWCHAR);

        Dirent->CdCaseFileName.FileName.MaximumLength = Dirent->CdFileName.FileName.MaximumLength / 2;

        CdUpcaseName( IrpContext,
                      &Dirent->CdFileName,
                      &Dirent->CdCaseFileName );
    }

    return;
}


BOOLEAN
CdFindFile (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PCD_NAME Name,
    IN BOOLEAN IgnoreCase,
    IN OUT PFILE_ENUM_CONTEXT FileContext,
    OUT PCD_NAME *MatchingName
    )

/*++

Routine Description:

    This routine is called to search a dirctory for a file matching the input
    name.  This name has been upcased at this point if this a case-insensitive
    search.  The name has been separated into separate name and version strings.
    We look for an exact match in the name and only consider the version if
    there is a version specified in the search name.

Arguments:

    Fcb - Fcb for the directory being scanned.

    Name - Name to search for.

    IgnoreCase - Indicates the case of the search.

    FileContext - File context to use for the search.  This has already been
        initialized.

    MatchingName - Pointer to buffer containing matching name.  We need this
        in case we don't match the name in the directory but match the
        short name instead.

Return Value:

    BOOLEAN - TRUE if matching entry is found, FALSE otherwise.

--*/

{
    PDIRENT Dirent;
    ULONG ShortNameDirentOffset;

    BOOLEAN Found = FALSE;

    PAGED_CODE();

    //
    //  Make sure there is a stream file for this Fcb.
    //

    if (Fcb->FileObject == NULL) {

        CdCreateInternalStream( IrpContext, Fcb->Vcb, Fcb );
    }

    //
    //  Check to see whether we need to check for a possible short name.
    //

    ShortNameDirentOffset = CdShortNameDirentOffset( IrpContext, &Name->FileName );

    //
    //  Position ourselves at the first entry.
    //

    CdLookupInitialFileDirent( IrpContext, Fcb, FileContext, Fcb->StreamOffset );

    //
    //  Loop while there are more entries in this directory.
    //

    do {

        Dirent = &FileContext->InitialDirent->Dirent;

        //
        //  We only consider files which don't have the associated bit set.
        //  We also only look for files.  All directories would already
        //  have been found.
        //

        if (!FlagOn( Dirent->DirentFlags, CD_ATTRIBUTE_ASSOC | CD_ATTRIBUTE_DIRECTORY )) {

            //
            //  Update the name in the current dirent.
            //

            CdUpdateDirentName( IrpContext, Dirent, IgnoreCase );

            //
            //  Don't bother with constant entries.
            //

            if (FlagOn( Dirent->Flags, DIRENT_FLAG_CONSTANT_ENTRY )) {

                continue;
            }

            //
            //  Now check whether we have a name match.
            //  We exit the loop if we have a match.
            //

            if (CdIsNameInExpression( IrpContext,
                                      &Dirent->CdCaseFileName,
                                      Name,
                                      0,
                                      TRUE )) {

                *MatchingName = &Dirent->CdCaseFileName;
                Found = TRUE;
                break;
            }

            //
            //  The names didn't match.  If the input name is a possible short
            //  name and we are at the correct offset in the directory then
            //  check if the short names match.
            //

            if (((Dirent->DirentOffset >> SHORT_NAME_SHIFT) == ShortNameDirentOffset) &&
                (Name->VersionString.Length == 0) &&
                !CdIs8dot3Name( IrpContext,
                                Dirent->CdFileName.FileName )) {

                //
                //  Create the short name and check for a match.
                //

                CdGenerate8dot3Name( IrpContext,
                                     &Dirent->CdCaseFileName.FileName,
                                     Dirent->DirentOffset,
                                     FileContext->ShortName.FileName.Buffer,
                                     &FileContext->ShortName.FileName.Length );

                //
                //  Now check whether we have a name match.
                //  We exit the loop if we have a match.
                //

                if (CdIsNameInExpression( IrpContext,
                                          &FileContext->ShortName,
                                          Name,
                                          0,
                                          FALSE )) {

                    *MatchingName = &FileContext->ShortName,
                    Found = TRUE;
                    break;
                }
            }
        }

        //
        //  Go to the next initial dirent for a file.
        //

    } while (CdLookupNextInitialFileDirent( IrpContext, Fcb, FileContext ));

    //
    //  If we find the file then collect all of the dirents.
    //

    if (Found) {

        CdLookupLastFileDirent( IrpContext, Fcb, FileContext );

    }

    return Found;
}


BOOLEAN
CdFindDirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PCD_NAME Name,
    IN BOOLEAN IgnoreCase,
    IN OUT PFILE_ENUM_CONTEXT FileContext
    )

/*++

Routine Description:

    This routine is called to search a dirctory for a directory matching the input
    name.  This name has been upcased at this point if this a case-insensitive
    search.  We look for an exact match in the name and do not look for shortname
    equivalents.

Arguments:

    Fcb - Fcb for the directory being scanned.

    Name - Name to search for.

    IgnoreCase - Indicates the case of the search.

    FileContext - File context to use for the search.  This has already been
        initialized.

Return Value:

    BOOLEAN - TRUE if matching entry is found, FALSE otherwise.

--*/

{
    PDIRENT Dirent;

    BOOLEAN Found = FALSE;

    PAGED_CODE();

    //
    //  Make sure there is a stream file for this Fcb.
    //

    if (Fcb->FileObject == NULL) {

        CdCreateInternalStream( IrpContext, Fcb->Vcb, Fcb );
    }

    //
    //  Position ourselves at the first entry.
    //

    CdLookupInitialFileDirent( IrpContext, Fcb, FileContext, Fcb->StreamOffset );

    //
    //  Loop while there are more entries in this directory.
    //

    do {

        Dirent = &FileContext->InitialDirent->Dirent;

        //
        //  We only look for directories.  Directories cannot have the
        //  associated bit set.
        //

        if (FlagOn( Dirent->DirentFlags, CD_ATTRIBUTE_DIRECTORY )) {

            //
            //  Update the name in the current dirent.
            //

            CdUpdateDirentName( IrpContext, Dirent, IgnoreCase );

            //
            //  Don't bother with constant entries.
            //

            if (FlagOn( Dirent->Flags, DIRENT_FLAG_CONSTANT_ENTRY )) {

                continue;
            }

            //
            //  Now check whether we have a name match.
            //  We exit the loop if we have a match.
            //

            if (CdIsNameInExpression( IrpContext,
                                      &Dirent->CdCaseFileName,
                                      Name,
                                      0,
                                      TRUE )) {

                Found = TRUE;
                break;
            }
        }

        //
        //  Go to the next initial dirent.
        //

    } while (CdLookupNextInitialFileDirent( IrpContext, Fcb, FileContext ));

    return Found;
}


BOOLEAN
CdFindFileByShortName (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PCD_NAME Name,
    IN BOOLEAN IgnoreCase,
    IN ULONG ShortNameDirentOffset,
    IN OUT PFILE_ENUM_CONTEXT FileContext
    )

/*++

Routine Description:

    This routine is called to find the file name entry whose short name
    is defined by the input DirentOffset.  The dirent offset here is
    multiplied by 32 and we look for the dirent begins in this 32 byte offset in
    directory.  The minimum dirent length is 34 so we are guaranteed that only
    one dirent can begin in each 32 byte block in the directory.

Arguments:

    Fcb - Fcb for the directory being scanned.

    Name - Name we are trying to match.  We know this contains the tilde
        character followed by decimal characters.

    IgnoreCase - Indicates whether we need to upcase the long name and
        generated short name.

    ShortNameDirentOffset - This is the shifted value for the offset of the
        name in the directory.

    FileContext - This is the initialized file context to use for the search.

Return Value:

    BOOLEAN - TRUE if a matching name was found, FALSE otherwise.

--*/

{
    BOOLEAN Found = FALSE;
    PDIRENT Dirent;

    ULONG ThisShortNameDirentOffset;

    PAGED_CODE();

    //
    //  Make sure there is a stream file for this Fcb.
    //

    if (Fcb->FileObject == NULL) {

        CdCreateInternalStream( IrpContext, Fcb->Vcb, Fcb );
    }

    //
    //  Position ourselves at the start of the directory and update
    //
    //

    CdLookupInitialFileDirent( IrpContext, Fcb, FileContext, Fcb->StreamOffset );

    //
    //  Loop until we have found the entry or are beyond this dirent.
    //

    do {

        //
        //  Compute the short name dirent offset for the current dirent.
        //

        Dirent = &FileContext->InitialDirent->Dirent;
        ThisShortNameDirentOffset = Dirent->DirentOffset >> SHORT_NAME_SHIFT;

        //
        //  If beyond the target then exit.
        //

        if (ThisShortNameDirentOffset > ShortNameDirentOffset) {

            break;
        }

        //
        //  If equal to the target then check if we have a name match.
        //  We will either match or fail here.
        //

        if (ThisShortNameDirentOffset == ShortNameDirentOffset) {

            //
            //  If this is an associated file then get out.
            //

            if (FlagOn( Dirent->DirentFlags, CD_ATTRIBUTE_ASSOC )) {

                break;
            }

            //
            //  Update the name in the dirent and check if it is not
            //  an 8.3 name.
            //

            CdUpdateDirentName( IrpContext, Dirent, IgnoreCase );

            if (CdIs8dot3Name( IrpContext,
                               Dirent->CdFileName.FileName )) {

                break;
            }

            //
            //  Generate the 8.3 name see if it matches our input name.
            //

            CdGenerate8dot3Name( IrpContext,
                                 &Dirent->CdCaseFileName.FileName,
                                 Dirent->DirentOffset,
                                 FileContext->ShortName.FileName.Buffer,
                                 &FileContext->ShortName.FileName.Length );

            //
            //  Check if this name matches.
            //

            if (CdIsNameInExpression( IrpContext,
                                      Name,
                                      &FileContext->ShortName,
                                      0,
                                      FALSE )) {

                //
                //  Let our caller know we found an entry.
                //

                Found = TRUE;
            }

            //
            //  Break out of the loop.
            //

            break;
        }

        //
        //  Continue until there are no more entries.
        //

    } while (CdLookupNextInitialFileDirent( IrpContext, Fcb, FileContext ));

    //
    //  If we find the file then collect all of the dirents.
    //

    if (Found) {

        CdLookupLastFileDirent( IrpContext, Fcb, FileContext );

    }

    return Found;
}


BOOLEAN
CdLookupNextInitialFileDirent (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PFILE_ENUM_CONTEXT FileContext
    )

/*++

Routine Description:

    This routine is called to walk through the directory until we find the
    first possible dirent for file.  We are positioned at some point described
    by the FileContext.  We will walk through any remaing dirents for the
    current file until we find the first dirent for some subsequent file.

    We can be called when we have found just one dirent for a file or all
    of them.  We first check the CurrentDirContext.  In the typical
    single-extent case this is unused.  Then we look to the InitialDirContext
    which must be initialized.

    This routine will save the initial DirContext to the PriorDirContext and
    clean up any existing DirContext for the Prior or Current positions in
    the enumeration context.

Arguments:

    Fcb - This is the directory to scan.

    FileContext - This is the file enumeration context.  It is currently pointing
        at some file in the directory.

Return Value:

--*/

{
    PRAW_DIRENT RawDirent;

    PDIRENT_ENUM_CONTEXT CurrentDirContext;
    PDIRENT_ENUM_CONTEXT TargetDirContext;
    PCOMPOUND_DIRENT TempDirent;

    BOOLEAN FoundDirent = FALSE;
    BOOLEAN FoundLastDirent;

    PAGED_CODE();

    //
    //  Start by saving the initial dirent of the current file as the
    //  previous file.
    //

    TempDirent = FileContext->PriorDirent;
    FileContext->PriorDirent = FileContext->InitialDirent;
    FileContext->InitialDirent = TempDirent;

    //
    //  We will use the initial dirent of the prior file unless the
    //  previous search returned multiple extents.
    //

    CurrentDirContext = &FileContext->PriorDirent->DirContext;

    if (FlagOn( FileContext->Flags, FILE_CONTEXT_MULTIPLE_DIRENTS )) {

        CurrentDirContext = &FileContext->CurrentDirent->DirContext;
    }

    //
    //  Clear all of the flags and file size for the next file.
    //

    FileContext->Flags = 0;
    FileContext->FileSize = 0;

    FileContext->ShortName.FileName.Length = 0;

    //
    //  We always want to store the result into the updated initial dirent
    //  context.
    //

    TargetDirContext = &FileContext->InitialDirent->DirContext;

    //
    //  Loop until we find the first dirent after the last dirent of the
    //  current file.  We may not be at the last dirent for the current file yet
    //  so we may walk forward looking for the last and then find the
    //  initial dirent for the next file after that.
    //

    while (TRUE) {

        //
        //  Remember if the last dirent we visited was the last dirent for
        //  a file.
        //

        RawDirent = CdRawDirent( IrpContext, CurrentDirContext );

        FoundLastDirent = !FlagOn( CdRawDirentFlags( IrpContext, RawDirent ), CD_ATTRIBUTE_MULTI );

        //
        //  Try to find another dirent.
        //

        FoundDirent = CdLookupNextDirent( IrpContext,
                                          Fcb,
                                          CurrentDirContext,
                                          TargetDirContext );

        //
        //  Exit the loop if no entry found.
        //

        if (!FoundDirent) {

            break;

        }

        //
        //  Update the in-memory dirent.
        //

        CdUpdateDirentFromRawDirent( IrpContext,
                                     Fcb,
                                     TargetDirContext,
                                     &FileContext->InitialDirent->Dirent );

        //
        //  Exit the loop if we had the end for the previous file.
        //

        if (FoundLastDirent) {

            break;
        }

        //
        //  Always use a single dirent from this point on.
        //

        CurrentDirContext = TargetDirContext;
    }

    return FoundDirent;
}


VOID
CdLookupLastFileDirent (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PFILE_ENUM_CONTEXT FileContext
    )

/*++

Routine Description:

    This routine is called when we've found the matching initial dirent for
    a file.  Now we want to find all of the dirents for a file as well as
    compute the running total for the file size.

    We also go out to the system use area and check whether this is an
    XA sector.  In that case we will compute the real file size.

    The dirent in the initial compound dirent has been updated from the
    raw dirent when this routine is called.

Arguments:

    Fcb - Directory containing the entries for the file.

    FileContext - Enumeration context for this search.  It currently points
        to the first dirent of the file and the in-memory dirent has been
        updated.

Return Value:

    None.  This routine may raise STATUS_FILE_CORRUPT.

--*/

{
    XA_EXTENT_TYPE ExtentType;
    PCOMPOUND_DIRENT CurrentCompoundDirent;
    PDIRENT CurrentDirent;

    BOOLEAN FirstPass = TRUE;
    BOOLEAN FoundDirent;

    PAGED_CODE();

    //
    //  The current dirent to look at is the initial dirent for the file.
    //

    CurrentCompoundDirent = FileContext->InitialDirent;

    //
    //  Loop until we reach the last dirent for the file.
    //

    while (TRUE) {

        CurrentDirent = &CurrentCompoundDirent->Dirent;

        //
        //  Check if this extent has XA sectors.
        //

        if ((CurrentDirent->SystemUseOffset != 0) &&
            FlagOn( Fcb->Vcb->VcbState, VCB_STATE_CDXA ) &&
            CdCheckForXAExtent( IrpContext,
                                CdRawDirent( IrpContext, &CurrentCompoundDirent->DirContext ),
                                CurrentDirent )) {

            //
            //  Any previous dirent must describe XA sectors as well.
            //

            if (!FirstPass && (ExtentType != CurrentDirent->ExtentType)) {

                CdRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
            }

            //
            //  If there are XA sectors then the data on the disk must
            //  be correctly aligned on sectors and be an integral number of
            //  sectors.  Only an issue if the logical block size is not
            //  2048.
            //

            if (Fcb->Vcb->BlockSize != SECTOR_SIZE) {

                //
                //  We will do the following checks.
                //
                //      Data must start on a sector boundary.
                //      Data length must be integral number of sectors.
                //

                if ((SectorBlockOffset( Fcb->Vcb, CurrentDirent->StartingOffset ) != 0) ||
                    (SectorBlockOffset( Fcb->Vcb, CurrentDirent->DataLength ) != 0)) {

                    CdRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
                }

                //
                //  If interleaved then both the file unit and interleave
                //  gap must be integral number of sectors.
                //

                if ((CurrentDirent->FileUnitSize != 0) &&
                    ((SectorBlockOffset( Fcb->Vcb, CurrentDirent->FileUnitSize ) != 0) ||
                     (SectorBlockOffset( Fcb->Vcb, CurrentDirent->InterleaveGapSize ) != 0))) {

                    CdRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
                }
            }

            //
            //  If this is the first dirent then add the bytes for the RIFF
            //  header.
            //

            if (FirstPass) {

                FileContext->FileSize = sizeof( RIFF_HEADER );
            }

            //
            //  Add the size of the mode2-form2 sector for each sector
            //  we have here.
            //

            FileContext->FileSize += Int32x32To64( CurrentDirent->DataLength >> SECTOR_SHIFT,
                                                   XA_SECTOR_SIZE);

        } else {

            //
            //  This extent does not have XA sectors.  Any previous dirent
            //  better not have XA sectors.
            //

            if (!FirstPass && (ExtentType != CurrentDirent->ExtentType)) {

                CdRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
            }

            //
            //  Add these bytes to the file size.
            //

            FileContext->FileSize += CurrentDirent->DataLength;
        }

        //
        //  If we are at the last dirent then exit.
        //

        if (!FlagOn( CurrentDirent->DirentFlags, CD_ATTRIBUTE_MULTI )) {

            break;
        }

        //
        //  Remember the extent type of the current extent.
        //

        ExtentType = CurrentDirent->ExtentType;

        //
        //  Look for the next dirent of the file.
        //

        FoundDirent = CdLookupNextDirent( IrpContext,
                                          Fcb,
                                          &CurrentCompoundDirent->DirContext,
                                          &FileContext->CurrentDirent->DirContext );

        //
        //  If we didn't find the entry then this is a corrupt directory.
        //

        if (!FoundDirent) {

            CdRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
        }

        //
        //  Remember the dirent we just found.
        //

        CurrentCompoundDirent = FileContext->CurrentDirent;
        FirstPass = FALSE;

        //
        //  Look up all of the dirent information for the given dirent.
        //

        CdUpdateDirentFromRawDirent( IrpContext,
                                     Fcb,
                                     &CurrentCompoundDirent->DirContext,
                                     &CurrentCompoundDirent->Dirent );

        //
        //  Set flag to show there were multiple extents.
        //

        SetFlag( FileContext->Flags, FILE_CONTEXT_MULTIPLE_DIRENTS );
    }

    return;
}


VOID
CdCleanupFileContext (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_ENUM_CONTEXT FileContext
    )

/*++

Routine Description:

    This routine is called to cleanup the enumeration context for a file
    search in a directory.  We will unpin any remaining Bcbs and free
    any allocated buffers.

Arguments:

    FileContext - Enumeration context for the file search.

Return Value:

    None.

--*/

{
    PCOMPOUND_DIRENT CurrentCompoundDirent;
    ULONG Count = 2;

    PAGED_CODE();

    //
    //  Cleanup the individual compound dirents.
    //

    do {

        CurrentCompoundDirent = &FileContext->Dirents[ Count ];
        CdCleanupDirContext( IrpContext, &CurrentCompoundDirent->DirContext );
        CdCleanupDirent( IrpContext, &CurrentCompoundDirent->Dirent );

    } while (Count--);

    return;
}


//
//  Local support routine
//

ULONG
CdCheckRawDirentBounds (
    IN PIRP_CONTEXT IrpContext,
    IN PDIRENT_ENUM_CONTEXT DirContext
    )

/*++

Routine Description:

    This routine takes a Dirent enumeration context and computes the offset
    to the next dirent.  A non-zero value indicates the offset within this
    sector.  A zero value indicates to move to the next sector.  If the
    current dirent does not fit within the sector then we will raise
    STATUS_CORRUPT.

Arguments:

    DirContext - Enumeration context indicating the current position in
        the sector.

Return Value:

    ULONG - Offset to the next dirent in this sector or zero if the
        next dirent is in the next sector.

    This routine will raise on a dirent which does not fit into the
    described data buffer.

--*/

{
    ULONG NextDirentOffset;
    PRAW_DIRENT RawDirent;

    PAGED_CODE();

    //
    //  We should always have at least a byte still available in the
    //  current buffer.
    //

    ASSERT( (DirContext->DataLength - DirContext->SectorOffset) >= 1 );

    //
    //  Get a pointer to the current dirent.
    //

    RawDirent = CdRawDirent( IrpContext, DirContext );

    //
    //  If the dirent length is non-zero then look at the current dirent.
    //

    if (RawDirent->DirLen != 0) {

        //
        //  Check the following bound for the dirent length.
        //
        //      - Fits in the available bytes in the sector.
        //      - Is at least the minimal dirent size.
        //      - Is large enough to hold the file name.
        //

        if ((RawDirent->DirLen > (DirContext->DataLength - DirContext->SectorOffset)) ||
            (RawDirent->DirLen < MIN_RAW_DIRENT_LEN) ||
            (RawDirent->DirLen < (MIN_RAW_DIRENT_LEN - 1 + RawDirent->FileIdLen))) {

            CdRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
        }

        //
        //  Copy the dirent length field.
        //

        NextDirentOffset = RawDirent->DirLen;

        //
        //  If we are exactly at the next sector then tell our caller by
        //  returning zero.
        //

        if (NextDirentOffset == (DirContext->DataLength - DirContext->SectorOffset)) {

            NextDirentOffset = 0;
        }

    } else {

        NextDirentOffset = 0;
    }

    return NextDirentOffset;
}


//
//  Local support routine
//

XA_EXTENT_TYPE
CdCheckForXAExtent (
    IN PIRP_CONTEXT IrpContext,
    IN PRAW_DIRENT RawDirent,
    IN OUT PDIRENT Dirent
    )

/*++

Routine Description:

    This routine is called to scan through the system use area to test if
    the current dirent has the XA bit set.  The bit in the in-memory
    dirent will be set as appropriate.

Arguments:

    RawDirent - Pointer to the on-disk dirent.

    Dirent - Pointer to the in-memory dirent.  We will update this with the
        appropriate XA flag.

Return Value:

    XA_EXTENT_TYPE - Type of physical extent for this on disk dirent.

--*/

{
    XA_EXTENT_TYPE ExtentType = Form1Data;
    PSYSTEM_USE_XA SystemUseArea;

    PAGED_CODE();

    //
    //  Check if there is enough space for the XA system use area.
    //

    if (Dirent->DirentLength - Dirent->SystemUseOffset >= sizeof( SYSTEM_USE_XA )) {

        SystemUseArea = Add2Ptr( RawDirent, Dirent->SystemUseOffset, PSYSTEM_USE_XA );

        //
        //  Check for a valid signature.
        //

        if (SystemUseArea->Signature == SYSTEM_XA_SIGNATURE) {

            //
            //  Check for an audio track.
            //

            if (FlagOn( SystemUseArea->Attributes, SYSTEM_USE_XA_DA )) {

                ExtentType = CDAudio;

            } else if (FlagOn( SystemUseArea->Attributes, SYSTEM_USE_XA_FORM2 )) {

                //
                //  Check for XA data.  Note that a number of discs (video CDs)
                //  have files marked as type XA Mode 2 Form 1 (2048 bytes of 
                //  user data),  but actually record these sectors as Mode2 Form 2 
                //  (2352). We will fail to read these files,  since for M2F1,  
                //  a normal read CD command is issued (as per SCSI specs).
                //
                
                ExtentType = Mode2Form2Data;
            }

            Dirent->XAAttributes = SystemUseArea->Attributes;
            Dirent->XAFileNumber = SystemUseArea->FileNumber;
        }
    }

    Dirent->ExtentType = ExtentType;
    return ExtentType;
}



