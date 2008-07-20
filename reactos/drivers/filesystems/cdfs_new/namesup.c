/*++

Copyright (c) 1991-2000 Microsoft Corporation

Module Name:

    NameSup.c

Abstract:

    This module implements the Cdfs Name support routines


--*/

#include "CdProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (CDFS_BUG_CHECK_NAMESUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CdConvertBigToLittleEndian)
#pragma alloc_text(PAGE, CdConvertNameToCdName)
#pragma alloc_text(PAGE, CdDissectName)
#pragma alloc_text(PAGE, CdGenerate8dot3Name)
#pragma alloc_text(PAGE, CdFullCompareNames)
#pragma alloc_text(PAGE, CdIs8dot3Name)
#pragma alloc_text(PAGE, CdIsNameInExpression)
#pragma alloc_text(PAGE, CdShortNameDirentOffset)
#pragma alloc_text(PAGE, CdUpcaseName)
#endif


VOID
CdConvertNameToCdName (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PCD_NAME CdName
    )

/*++

Routine Description:

    This routine is called to convert a string of bytes into a CdName.

    The full name is already in the CdName structure in the FileName field.
    We split this into the filename and version strings.

Arguments:

    CdName - Pointer to CdName structure to update.

Return Value:

    None.

--*/

{
    ULONG NameLength = 0;
    PWCHAR CurrentCharacter = CdName->FileName.Buffer;

    PAGED_CODE();

    //
    //  Look for a separator character.
    //

    while ((NameLength < CdName->FileName.Length) &&
           (*CurrentCharacter != L';')) {

        CurrentCharacter += 1;
        NameLength += 2;
    }

    //
    //  If there is at least one more character after a possible separator then it
    //  and all following characters are part of the version string.
    //

    CdName->VersionString.Length = 0;
    if (NameLength + sizeof( WCHAR ) < CdName->FileName.Length) {

        CdName->VersionString.MaximumLength =
        CdName->VersionString.Length = (USHORT) (CdName->FileName.Length - NameLength - sizeof( WCHAR ));
        CdName->VersionString.Buffer = Add2Ptr( CdName->FileName.Buffer,
                                                NameLength + sizeof( WCHAR ),
                                                PWCHAR );
    }

    //
    //  Now update the filename portion of the name.
    //

    CdName->FileName.Length = (USHORT) NameLength;

    return;
}


VOID
CdConvertBigToLittleEndian (
    IN PIRP_CONTEXT IrpContext,
    IN PCHAR BigEndian,
    IN ULONG ByteCount,
    OUT PCHAR LittleEndian
    )

/*++

Routine Description:

    This routine is called to convert a unicode string in big endian to
    little endian.  We start by copying all of the source bytes except
    the first.  This will put the low order bytes in the correct position.
    We then copy each high order byte in its correct position.

Arguments:

    BigEndian - Pointer to the string of big endian characters.

    ByteCount - Number of unicode characters in this string.

    LittleEndian - Pointer to array to store the little endian characters.

Return Value:

    None.

--*/

{
    ULONG RemainingByteCount = ByteCount;

    PCHAR Source = BigEndian;
    PCHAR Destination = LittleEndian;

    PAGED_CODE();

    //
    //  If the byte count isn't an even number then the disk is corrupt.
    //

    if (FlagOn( ByteCount, 1 )) {

        CdRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR );
    }

    //
    //  Start by copy the low-order bytes into the correct position.  Do
    //  this by skipping the first byte in the BigEndian string.
    //

    RtlCopyMemory( Destination,
                   Source + 1,
                   RemainingByteCount - 1 );

    //
    //  Now move the high-order bytes into position.
    //

    Destination += 1;

    while (RemainingByteCount != 0) {

        *Destination = *Source;

        Source += 2;
        Destination += 2;

        RemainingByteCount -= 2;
    }

    return;
}


VOID
CdUpcaseName (
    IN PIRP_CONTEXT IrpContext,
    IN PCD_NAME Name,
    IN OUT PCD_NAME UpcaseName
    )

/*++

Routine Description:

    This routine is called to upcase a CdName structure.  We will do both
    the filename and version strings.

Arguments:

    Name - This is the mixed case version of the name.

    UpcaseName - This is the destination for the upcase operation.

Return Value:

    None.  This routine will raise all errors.

--*/

{
    NTSTATUS Status;
    PVOID NewBuffer;

    PAGED_CODE();

    //
    //  If the name structures are different then initialize the different components.
    //

    if (Name != UpcaseName) {

        //
        //  Initialize the version string portion of the name.
        //

        UpcaseName->VersionString.Length = 0;

        if (Name->VersionString.Length != 0) {

            UpcaseName->VersionString.MaximumLength =
            UpcaseName->VersionString.Length = Name->VersionString.Length;

            //
            //  Initially set the buffer to point to where we need to insert
            //  the separator.
            //

            UpcaseName->VersionString.Buffer = Add2Ptr( UpcaseName->FileName.Buffer,
                                                        Name->FileName.Length,
                                                        PWCHAR );

            //
            //  We are currently pointing to the location to store the separator.
            //  Store the ';' and then move to the next character to
            //  copy the data.
            //

            *(UpcaseName->VersionString.Buffer) = L';';

            UpcaseName->VersionString.Buffer += 1;
        }
    }

    //
    //  Upcase the string using the correct upcase routine.
    //

    Status = RtlUpcaseUnicodeString( &UpcaseName->FileName,
                                     &Name->FileName,
                                     FALSE );

    //
    //  This should never fail.
    //

    ASSERT( Status == STATUS_SUCCESS );

    if (Name->VersionString.Length != 0) {

        Status = RtlUpcaseUnicodeString( &UpcaseName->VersionString,
                                         &Name->VersionString,
                                         FALSE );

        //
        //  This should never fail.
        //

        ASSERT( Status == STATUS_SUCCESS );
    }

    return;
}


VOID
CdDissectName (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PUNICODE_STRING RemainingName,
    OUT PUNICODE_STRING FinalName
    )

/*++

Routine Description:

    This routine is called to strip off leading components of the name strings.  We search
    for either the end of the string or separating characters.  The input remaining
    name strings should have neither a trailing or leading backslash.

Arguments:

    RemainingName - Remaining name.

    FinalName - Location to store next component of name.

Return Value:

    None.

--*/

{
    ULONG NameLength;
    PWCHAR NextWchar;

    PAGED_CODE();

    //
    //  Find the offset of the next component separators.
    //

    for (NameLength = 0, NextWchar = RemainingName->Buffer;
         (NameLength < RemainingName->Length) && (*NextWchar != L'\\');
         NameLength += sizeof( WCHAR) , NextWchar += 1);

    //
    //  Adjust all the strings by this amount.
    //

    FinalName->Buffer = RemainingName->Buffer;

    FinalName->MaximumLength = FinalName->Length = (USHORT) NameLength;

    //
    //  If this is the last component then set the RemainingName lengths to zero.
    //

    if (NameLength == RemainingName->Length) {

        RemainingName->Length = 0;

    //
    //  Otherwise we adjust the string by this amount plus the separating character.
    //

    } else {

        RemainingName->MaximumLength -= (USHORT) (NameLength + sizeof( WCHAR ));
        RemainingName->Length -= (USHORT) (NameLength + sizeof( WCHAR ));
        RemainingName->Buffer = Add2Ptr( RemainingName->Buffer,
                                         NameLength + sizeof( WCHAR ),
                                         PWCHAR );
    }

    return;
}


BOOLEAN
CdIs8dot3Name (
    IN PIRP_CONTEXT IrpContext,
    IN UNICODE_STRING FileName
    )

/*++

Routine Description:

    This routine checks if the name follows the 8.3 name conventions.  We check for
    the name length and whether the characters are valid.

Arguments:

    FileName - String of bytes containing the name.

Return Value:

    BOOLEAN - TRUE if this name is a legal 8.3 name, FALSE otherwise.

--*/

{
    CHAR DbcsNameBuffer[ BYTE_COUNT_8_DOT_3 ];
    STRING DbcsName;

    PWCHAR NextWchar;
    ULONG Count;

    ULONG DotCount = 0;
    BOOLEAN LastCharDot = FALSE;

    PAGED_CODE();

    //
    //  The length must be less than 24 bytes.
    //

    ASSERT( FileName.Length != 0 );
    if (FileName.Length > BYTE_COUNT_8_DOT_3) {

        return FALSE;
    }

    //
    //  Walk though and check for a space character.
    //

    NextWchar = FileName.Buffer;
    Count = 0;

    do {

        //
        //  No spaces allowed.
        //

        if (*NextWchar == L' ') { return FALSE; }

        if (*NextWchar == L'.') {

            //
            //  Not an 8.3 name if more than 1 dot or more than 8 characters
            //  remaining.  (It is legal for the dot to be in the ninth
            //  position)
            //

            if ((DotCount > 0) ||
                (Count > 8 * sizeof( WCHAR ))) {

                return FALSE;
            }

            DotCount += 1;
            LastCharDot = TRUE;

        } else {

            LastCharDot = FALSE;
        }

        Count += 2;
        NextWchar += 1;

    } while (Count < FileName.Length);

    //
    //  Go ahead and truncate the dot if at the end.
    //

    if (LastCharDot) {

        FileName.Length -= sizeof( WCHAR );
    }

    //
    //  Create an Oem name to use to check for a valid short name.
    //

    DbcsName.MaximumLength = BYTE_COUNT_8_DOT_3;
    DbcsName.Buffer = DbcsNameBuffer;

    if (!NT_SUCCESS( RtlUnicodeStringToCountedOemString( &DbcsName,
                                                         &FileName,
                                                         FALSE ))) {

        return FALSE;
    }

    //
    //  We have now initialized the Oem string.  Call the FsRtl package to check for a
    //  valid FAT name.
    //

    return FsRtlIsFatDbcsLegal( DbcsName, FALSE, FALSE, FALSE );
}


VOID
CdGenerate8dot3Name (
    IN PIRP_CONTEXT IrpContext,
    IN PUNICODE_STRING FileName,
    IN ULONG DirentOffset,
    OUT PWCHAR ShortFileName,
    OUT PUSHORT ShortByteCount
    )

/*++

Routine Description:

    This routine is called to generate a short name from the given long name.  We will
    generate a short name from the given long name.

    We go through the following steps to make this conversion.

        1 - Generate the generic short name.  This will also be in unicode format.

        2 - Build the string representation of the dirent offset.

        3 - Build the biased short name string by combining the generic short name with
            the dirent offset string.

        4 - Copy the final unicode string back to our caller's buffer.

Arguments:

    FileName - String of bytes containing the name.

    DirentOffset - Offset in the directory for this filename.  We incorporate the offset into
        the short name by dividing this by 32 and prepending a tilde character to the
        digit character.  We then append this to the base of the generated short name.

    ShortFileName - Pointer to the buffer to store the short name into.

    ShortByteCount - Address to store the number of bytes in the short name.

Return Value:

    None.

--*/

{
    NTSTATUS Status;
    UNICODE_STRING ShortName;
    UNICODE_STRING BiasedShortName;
    WCHAR ShortNameBuffer[ BYTE_COUNT_8_DOT_3 / sizeof( WCHAR ) ];
    WCHAR BiasedShortNameBuffer[ BYTE_COUNT_8_DOT_3 / sizeof( WCHAR ) ];

    GENERATE_NAME_CONTEXT NameContext;

    ULONG BiasedDirentOffset;

    ULONG MaximumBaseBytes;
    ULONG BaseNameOffset;

    PWCHAR NextWchar;
    WCHAR ThisWchar;
    USHORT Length;

    BOOLEAN FoundTilde = FALSE;

    OEM_STRING OemName;
    USHORT OemNameOffset = 0;
    BOOLEAN OverflowBuffer = FALSE;

    PAGED_CODE();

    //
    //  Initialize the short string to use the input buffer.
    //

    ShortName.Buffer = ShortNameBuffer;
    ShortName.MaximumLength = BYTE_COUNT_8_DOT_3;

    //
    //  Initialize the name context.
    //

    RtlZeroMemory( &NameContext, sizeof( GENERATE_NAME_CONTEXT ));

    //
    //  We now have the unicode name for the input string.  Go ahead and generate
    //  the short name.
    //

    RtlGenerate8dot3Name( FileName, TRUE, &NameContext, &ShortName );

    //
    //  We now have the generic short name.  We want incorporate the dirent offset
    //  into the name in order to reduce the chance of name conflicts.  We will use
    //  a tilde character followed by a character representation of the dirent offset.
    //  This will be the hexadecimal representation of the dirent offset in the directory.
    //  It is actuall this offset divided by 32 since we don't need the full
    //  granularity.
    //

    BiasedDirentOffset = DirentOffset >> SHORT_NAME_SHIFT;

    //
    //  Point to a local buffer to store the offset string.  We start
    //  at the end of the buffer and work backwards.
    //

    NextWchar = Add2Ptr( BiasedShortNameBuffer,
                         BYTE_COUNT_8_DOT_3,
                         PWCHAR );

    BiasedShortName.MaximumLength = BYTE_COUNT_8_DOT_3;

    //
    //  Generate an OEM version of the string so that we can check for double
    //  byte characters.
    //
    
    Status = RtlUnicodeStringToOemString(&OemName, &ShortName, TRUE);

    if (!NT_SUCCESS( Status )) {

        CdRaiseStatus( IrpContext, Status );
    }
    
    Length = 0;

    //
    //  Now add the characters for the dirent offset.  We need to start
    //  from the least significant digit and work backwards.
    //

    do {

        NextWchar -= 1;

        ThisWchar = (WCHAR) (BiasedDirentOffset & 0x0000000f);

        //
        //  Store in the next character.  Bias against either '0' or 'A'
        //

        if (ThisWchar <= 9) {

            *NextWchar = ThisWchar + L'0';

        } else {

            *NextWchar = ThisWchar + L'A' - 0xA;
        }

        Length += sizeof( WCHAR );

        //
        //  Shift out the low 4 bits of the offset.
        //

        BiasedDirentOffset >>= 4;

    } while (BiasedDirentOffset != 0);

    //
    //  Now store in the tilde character.
    //

    NextWchar -= 1;
    *NextWchar = L'~';
    Length += sizeof( WCHAR );

    //
    //  Set the length of this string.
    //

    BiasedShortName.Length = Length;
    BiasedShortName.Buffer = NextWchar;

    //
    //  Figure out the maximum number of characters we can copy of the base
    //  name.  We subract the number of characters in the dirent string from 8.
    //  We will copy this many characters or stop when we reach a '.' character
    //  or a '~' character in the name.
    //

    MaximumBaseBytes = 16 - Length;

    BaseNameOffset = 0;

    //
    //  Keep copying from the base name until we hit a '.', '~'  or the end of
    //  the short name.
    //

    NextWchar = ShortFileName;
    Length = 0;

    while ((BaseNameOffset < ShortName.Length) &&
           (ShortName.Buffer[BaseNameOffset / 2] != L'.')) {

        //
        //  Remember if we found a tilde character in the short name,
        //  so we don't copy it or anything following it.
        //

        if (ShortName.Buffer[BaseNameOffset / 2] == L'~') {

            FoundTilde = TRUE;
        }

        //
        // We need to consider the DBCS code page,  because Unicode characters
        // may use 2 bytes as DBCS characters.
        //

        if (FsRtlIsLeadDbcsCharacter(OemName.Buffer[OemNameOffset])) {

            OemNameOffset += 2;
        }
        else  {
        
            OemNameOffset++;
        }

        if ((OemNameOffset + (BiasedShortName.Length / sizeof(WCHAR))) > 8)  {
        
            OverflowBuffer = TRUE;
        }

        //
        //  Only copy the bytes if we still have space for the dirent string.
        //

        if (!FoundTilde && !OverflowBuffer && (BaseNameOffset < MaximumBaseBytes)) {

            *NextWchar = ShortName.Buffer[BaseNameOffset / 2];
            Length += sizeof( WCHAR );
            NextWchar += 1;
        }

        BaseNameOffset += 2;
    }

    RtlFreeOemString(&OemName);

    //
    //  Now copy the dirent string into the biased name buffer.
    //

    RtlCopyMemory( NextWchar,
                   BiasedShortName.Buffer,
                   BiasedShortName.Length );

    Length += BiasedShortName.Length;
    NextWchar += (BiasedShortName.Length / sizeof( WCHAR ));

    //
    //  Now copy any remaining bytes over to the biased short name.
    //

    if (BaseNameOffset != ShortName.Length) {

        RtlCopyMemory( NextWchar,
                       &ShortName.Buffer[BaseNameOffset / 2],
                       ShortName.Length - BaseNameOffset );

        Length += (ShortName.Length - (USHORT) BaseNameOffset);
    }

    //
    //  The final short name is stored in the user's buffer.
    //

    *ShortByteCount = Length;

    return;
}


BOOLEAN
CdIsNameInExpression (
    IN PIRP_CONTEXT IrpContext,
    IN PCD_NAME CurrentName,
    IN PCD_NAME SearchExpression,
    IN ULONG  WildcardFlags,
    IN BOOLEAN CheckVersion
    )

/*++

Routine Description:

    This routine will compare two CdName strings.  We assume that if this
    is to be a case-insensitive search then they are already upcased.

    We compare the filename portions of the name and if they match we
    compare the version strings if requested.

Arguments:

    CurrentName - Filename from the disk.

    SearchExpression - Filename expression to use for match.

    WildcardFlags - Flags field which indicates which parts of the
        search expression might have wildcards.  These flags are the
        same as in the Ccb flags field.

    CheckVersion - Indicates whether we should check both the name and the
        version strings or just the name.

Return Value:

    BOOLEAN - TRUE if the expressions match, FALSE otherwise.

--*/

{
    BOOLEAN Match = TRUE;
    PAGED_CODE();

    //
    //  If there are wildcards in the expression then we call the
    //  appropriate FsRtlRoutine.
    //

    if (FlagOn( WildcardFlags, CCB_FLAG_ENUM_NAME_EXP_HAS_WILD )) {

        Match = FsRtlIsNameInExpression( &SearchExpression->FileName,
                                         &CurrentName->FileName,
                                         FALSE,
                                         NULL );

    //
    //  Otherwise do a direct memory comparison for the name string.
    //

    } else {

        if ((CurrentName->FileName.Length != SearchExpression->FileName.Length) ||
            (!RtlEqualMemory( CurrentName->FileName.Buffer,
                              SearchExpression->FileName.Buffer,
                              CurrentName->FileName.Length ))) {

            Match = FALSE;
        }
    }

    //
    //  Check the version numbers if requested by the user and we have a
    //  match on the name and the version number is present.
    //

    if (Match && CheckVersion && SearchExpression->VersionString.Length &&
        !FlagOn( WildcardFlags, CCB_FLAG_ENUM_VERSION_MATCH_ALL )) {

        //
        //  If there are wildcards in the expression then call the
        //  appropriate search expression.
        //

        if (FlagOn( WildcardFlags, CCB_FLAG_ENUM_VERSION_EXP_HAS_WILD )) {

            Match = FsRtlIsNameInExpression( &SearchExpression->VersionString,
                                             &CurrentName->VersionString,
                                             FALSE,
                                             NULL );

        //
        //  Otherwise do a direct memory comparison for the name string.
        //

        } else {

            if ((CurrentName->VersionString.Length != SearchExpression->VersionString.Length) ||
                (!RtlEqualMemory( CurrentName->VersionString.Buffer,
                                  SearchExpression->VersionString.Buffer,
                                  CurrentName->VersionString.Length ))) {

                Match = FALSE;
            }
        }
    }

    return Match;
}


ULONG
CdShortNameDirentOffset (
    IN PIRP_CONTEXT IrpContext,
    IN PUNICODE_STRING Name
    )

/*++

Routine Description:

    This routine is called to examine a name to see if the dirent offset string is contained.
    This consists of a tilde character followed by the offset represented as a hexadecimal
    characters.  We don't do any other checks to see if this is a short name.  We
    catch that later outside this routine.

Arguments:

    Name - This is the CdName to examine.

Return Value:

    ULONG - MAXULONG if there is no valid dirent offset string embedded, otherwise the
        convert the value to numeric form.

--*/

{
    ULONG ResultOffset = MAXULONG;
    ULONG RemainingByteCount = Name->Length;

    BOOLEAN FoundTilde = FALSE;

    PWCHAR NextWchar;

    PAGED_CODE();

    //
    //  Walk through the name until we either reach the end of the name
    //  or find a tilde character.
    //

    for (NextWchar = Name->Buffer;
         RemainingByteCount != 0;
         NextWchar += 1, RemainingByteCount -= sizeof( WCHAR )) {

        //
        //  Check if this is a dot.  Stop constructing any string if
        //  we found a dot.
        //

        if (*NextWchar == L'.') {

            break;
        }

        //
        //  If we already found a tilde then check this character as a
        //  valid character.  It must be a digit or A to F.
        //

        if (FoundTilde) {

            if ((*NextWchar < L'0') ||
                (*NextWchar > L'F') ||
                ((*NextWchar > L'9') && (*NextWchar < 'A'))) {

                ResultOffset = MAXULONG;
                break;
            }

            //
            //  Shift the result by 4 bits and add in this new character.
            //

            ResultOffset <<= 4;

            if (*NextWchar < L'A') {

                ResultOffset += *NextWchar - L'0';

            } else {

                ResultOffset += (*NextWchar - L'A') + 10;
            }

            continue;
        }

        //
        //  If this is a tilde then start building the dirent string.
        //

        if (*NextWchar == L'~') {

            FoundTilde = TRUE;
            ResultOffset = 0;
        }
    }

    return ResultOffset;
}


//
//  Local support routine
//

FSRTL_COMPARISON_RESULT
CdFullCompareNames (
    IN PIRP_CONTEXT IrpContext,
    IN PUNICODE_STRING NameA,
    IN PUNICODE_STRING NameB
    )

/*++

Routine Description:

    This function compares two names as fast as possible.  Note that since
    this comparison is case sensitive we can do a direct memory comparison.

Arguments:

    NameA & NameB - The names to compare.

Return Value:

    COMPARISON - returns

        LessThan    if NameA < NameB lexicalgraphically,
        GreaterThan if NameA > NameB lexicalgraphically,
        EqualTo     if NameA is equal to NameB

--*/

{
    SIZE_T i;
    ULONG MinLength = NameA->Length;
    FSRTL_COMPARISON_RESULT Result = LessThan;

    PAGED_CODE();

    //
    //  Figure out the minimum of the two lengths
    //

    if (NameA->Length > NameB->Length) {

        MinLength = NameB->Length;
        Result = GreaterThan;

    } else if (NameA->Length == NameB->Length) {

        Result = EqualTo;
    }

    //
    //  Loop through looking at all of the characters in both strings
    //  testing for equalilty, less than, and greater than
    //

    i = RtlCompareMemory( NameA->Buffer, NameB->Buffer, MinLength );

    if (i < MinLength) {

        //
        //  We know the offset of the first character which is different.
        //

        return ((NameA->Buffer[ i / 2 ] < NameB->Buffer[ i / 2 ]) ?
                 LessThan :
                 GreaterThan);
    }

    //
    //  The names match up to the length of the shorter string.
    //  The shorter string lexically appears first.
    //

    return Result;
}



