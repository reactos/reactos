/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    Gen8dot3.c

Abstract:

    This module implements a routine to generate 8.3 names from long names.

Author:

    Gary Kimura     [GaryKi]    26-Mar-1992

Environment:

    Pure Utility Routines

Revision History:

--*/

#include "ntrtlp.h"
#include <stdio.h>

extern PUSHORT  NlsUnicodeToMbOemData;
extern PUSHORT  NlsOemToUnicodeData;
extern PCH      NlsUnicodeToOemData;
extern PUSHORT  NlsMbOemCodePageTables;
extern BOOLEAN  NlsMbOemCodePageTag;
extern PUSHORT  NlsOemLeadByteInfo;
extern USHORT   OemDefaultChar;

//
//  A condensed table of legal fat character values
//

const
ULONG RtlFatIllegalTable[] = { 0xffffffff,
                               0xfc009c04,
                               0x38000000,
                               0x10000000 };

WCHAR
GetNextWchar (
    IN PUNICODE_STRING Name,
    IN PULONG CurrentIndex,
    IN BOOLEAN SkipDots,
    IN BOOLEAN AllowExtendedCharacters
    );

USHORT
RtlComputeLfnChecksum (
    PUNICODE_STRING Name
    );

//
//  BOOLEAN
//  IsDbcsCharacter (
//      IN WCHAR Wc
//  );
//

#define IsDbcsCharacter(WC) (             \
    ((WC) > 127) &&                       \
    (HIBYTE(NlsUnicodeToMbOemData[(WC)])) \
)

#if defined(ALLOC_PRAGMA) && defined(NTOS_KERNEL_RUNTIME)
#pragma alloc_text(PAGE,RtlGenerate8dot3Name)
#pragma alloc_text(PAGE,GetNextWchar)
#pragma alloc_text(PAGE,RtlComputeLfnChecksum)
#pragma alloc_text(PAGE,RtlIsNameLegalDOS8Dot3)
#pragma alloc_text(PAGE,RtlIsValidOemCharacter)
#endif


VOID
RtlGenerate8dot3Name (
    IN PUNICODE_STRING Name,
    IN BOOLEAN AllowExtendedCharacters,
    IN OUT PGENERATE_NAME_CONTEXT Context,
    OUT PUNICODE_STRING Name8dot3
    )

/*++

Routine Description:

    This routine is used to generate an 8.3 name from a long name.  It can
    be called repeatedly to generate different 8.3 name variations for the
    same long name.  This is necessary if the gernerated 8.3 name conflicts
    with an existing 8.3 name.

Arguments:

    Name - Supplies the original long name that is being translated from.

    AllowExtendedCharacters - If TRUE, then extended characters, including
        DBCS characters, are allowed in the basis of the short name if they
        map to an upcased Oem character.

    Context - Supplies a context for the translation.  This is a private structure
        needed by this routine to help enumerate the different long name
        possibilities.  The caller is responsible with providing a "zeroed out"
        context structure on the first call for each given input name.

    Name8dot3 - Receives the new 8.3 name.  Pool for the buffer must be allocated
        by the caller and should be 12 characters wide (i.e., 24 bytes).

Return Value:

    None.

--*/

{
    BOOLEAN DbcsAware;
    BOOLEAN IndexAll9s = TRUE;
    ULONG OemLength;
    ULONG IndexLength;
    WCHAR IndexBuffer[8];
    ULONG i;

#ifdef NTOS_KERNEL_RUNTIME
    extern BOOLEAN FsRtlSafeExtensions;
#else
    BOOLEAN FsRtlSafeExtensions = TRUE;
#endif

    DbcsAware = AllowExtendedCharacters && NlsMbOemCodePageTag;

    //
    //  Check if this is the first time we are being called, and if so then
    //  initialize the context fields.
    //

    if (Context->NameLength == 0) {

        ULONG LastDotIndex;

        ULONG CurrentIndex;
        BOOLEAN SkipDots;
        WCHAR wc;

        //
        //  Skip down the name remembering the index of the last dot we
        //  will skip over the first dot provided the name starts with
        //  a dot.
        //

        LastDotIndex = MAXULONG;

        CurrentIndex = 0;
        SkipDots = ((Name->Length > 0) && (Name->Buffer[0] == L'.'));

        while ((wc = GetNextWchar( Name,
                                   &CurrentIndex,
                                   SkipDots,
                                   AllowExtendedCharacters )) != 0) {

            SkipDots = FALSE;
            if (wc == L'.') { LastDotIndex = CurrentIndex; }
        }

        //
        //  If the LastDotIndex is the last character in the name,
        //  then there really isn't an extension, so reset LastDotIndex.
        //

        if (LastDotIndex == Name->Length/sizeof(WCHAR)) {

            LastDotIndex = MAXULONG;
        }

        //
        //  Build up the name part. This can be at most 6 characters
        //  (because of the ~# appeneded on the end) and we skip over
        //  dots, except the last dot, which terminates the loop.
        //
        //  We exit the loop if:
        //
        //  - The input Name has been exhausted
        //  - We have consumed the input name up to the last dot
        //  - We have filled 6 characters of short name basis
        //

        CurrentIndex = 0;
        OemLength = 0;
        Context->NameLength = 0;

        while ((wc = GetNextWchar( Name, &CurrentIndex, TRUE, AllowExtendedCharacters)) &&
               (CurrentIndex < LastDotIndex) &&
               (Context->NameLength < 6)) {

            //
            //  If we are on a multi-byte code page we have to be careful
            //  here because the short name (when converted to Oem) must
            //  be 8.3 compliant.  Note that if AllowExtendedCharacters
            //  is FALSE, then GetNextWchar will never return a DBCS
            //  character, so we don't care what kind of code page we
            //  are on.
            //

            if (DbcsAware) {

                OemLength += IsDbcsCharacter(wc) ? 2 : 1;

                if (OemLength > 6) { break; }
            }

            //
            //  Copy the UNICODE character into the name buffer
            //

            Context->NameBuffer[Context->NameLength++] = wc;
        }

        //
        //  Now if the name part of the basis is 2 or less bytes (when
        //  represented in Oem) then append a four character checksum
        //  to make the short name space less sparse.
        //

        if ((DbcsAware ? OemLength : Context->NameLength) <= 2) {

            USHORT Checksum;
            WCHAR Nibble;

            Checksum =
            Context->Checksum = RtlComputeLfnChecksum( Name );

            for (i = 0; i < 4; i++, Checksum >>= 4) {

                Nibble = Checksum & 0xf;
                Nibble += Nibble <= 9 ? '0' : 'A' - 10;

                Context->NameBuffer[ Context->NameLength + i ] = Nibble;
            }

            Context->NameLength += 4;
            Context->ChecksumInserted = TRUE;
        }

        //
        //  Now process the last extension (if there is one).
        //  If the last dot index is not MAXULONG then we
        //  have located the last dot in the name
        //

        if (LastDotIndex != MAXULONG) {

            //
            //  Put in the "."
            //

            Context->ExtensionBuffer[0] = L'.';

            //
            //  Process the extension similar to how we processed the name
            //
            //  We exit the loop if:
            //
            //  - The input Name has been exhausted
            //  - We have filled . + 3 characters of extension
            //

            OemLength = 1;
            Context->ExtensionLength = 1;

            while ((wc = GetNextWchar( Name, &LastDotIndex, TRUE, AllowExtendedCharacters)) &&
                   (Context->ExtensionLength < 4)) {

                if (DbcsAware) {

                    OemLength += IsDbcsCharacter(wc) ? 2 : 1;

                    if (OemLength > 4) { break; }
                }

                Context->ExtensionBuffer[Context->ExtensionLength++] = wc;
            }

            //
            //  If we had to truncate the extension (i.e. input name was not
            //  exhausted), change the last char of the truncated extension
            //  to a ~ is user has selected safe extensions.
            //

            if (wc && FsRtlSafeExtensions) {

                Context->ExtensionBuffer[Context->ExtensionLength - 1] = L'~';
            }

        } else {

            Context->ExtensionLength = 0;
        }
    }

    //
    //  In all cases we add one to the index value and this is the value
    //  of the index we are going to generate this time around
    //

    Context->LastIndexValue += 1;

    //
    //  Now if the new index value is greater than 4 then we've had too
    //  many collisions and we should alter our basis if possible
    //

    if ((Context->LastIndexValue > 4) && !Context->ChecksumInserted) {

        USHORT Checksum;
        WCHAR Nibble;

        //
        // 'XX' is represented A DBCS character.
        //
        // LongName       -> ShortName  | DbcsBias  Oem  Unicode
        // -----------------------------+------------------------
        // XXXXThisisapen -> XX1234     |    1       6      5
        // XXThisisapen   -> XX1234     |    1       6      5
        // aXXThisisapen  -> a1234      |    1       5      5
        // aaThisisapen   -> aa1234     |    0       6      6
        //

        ULONG DbcsBias;

        if (DbcsAware) {

              DbcsBias = ((IsDbcsCharacter(Context->NameBuffer[0]) ? 1 : 0) |
                          (IsDbcsCharacter(Context->NameBuffer[1]) ? 1 : 0));

        } else {

              DbcsBias = 0;
        }

        Checksum =
        Context->Checksum = RtlComputeLfnChecksum( Name );

        for (i = (2-DbcsBias); i < (6-DbcsBias); i++, Checksum >>= 4) {

            Nibble = Checksum & 0xf;
            Nibble += Nibble <= 9 ? '0' : 'A' - 10;

            Context->NameBuffer[ i ] = Nibble;
        }

        Context->NameLength = (UCHAR)(6-DbcsBias);
        Context->LastIndexValue = 1;
        Context->ChecksumInserted = TRUE;
    }

    //
    //  Now build the index buffer from high index to low index because we
    //  use a mod & div operation to build the string from the index value.
    //
    //  We also want to remember is we are about to rollover in base 10.
    //

    for (IndexLength = 1, i = Context->LastIndexValue;
         (IndexLength <= 7) && (i > 0);
         IndexLength += 1, i /= 10) {

        if ((IndexBuffer[ 8 - IndexLength] = (WCHAR)(L'0' + (i % 10))) != L'9') {

            IndexAll9s = FALSE;
        }
    }

    //
    //  And tack on the preceding dash
    //

    IndexBuffer[ 8 - IndexLength ] = L'~';

    //
    //  At this point everything is set up to copy to the output buffer.  First
    //  copy over the name and then only copy the index and extension if they exist
    //

    if (Context->NameLength != 0) {

        RtlCopyMemory( &Name8dot3->Buffer[0],
                       &Context->NameBuffer[0],
                       Context->NameLength * 2 );

        Name8dot3->Length = (USHORT)(Context->NameLength * 2);

    } else {

        Name8dot3->Length = 0;
    }

    //
    //  Now do the index.
    //

    RtlCopyMemory( &Name8dot3->Buffer[ Name8dot3->Length/2 ],
                   &IndexBuffer[ 8 - IndexLength ],
                   IndexLength * 2 );

    Name8dot3->Length += (USHORT) (IndexLength * 2);

    //
    //  Now conditionally do the extension
    //

    if (Context->ExtensionLength != 0) {

        RtlCopyMemory( &Name8dot3->Buffer[ Name8dot3->Length/2 ],
                       &Context->ExtensionBuffer[0],
                       Context->ExtensionLength * 2 );

        Name8dot3->Length += (USHORT) (Context->ExtensionLength * 2);
    }

    //
    //  If current index value is all 9s, then the next value will cause the
    //  index string to grow from it's current size.  In this case recompute
    //  Context->NameLength so that is will be correct for next time.
    //

    if (IndexAll9s) {

        if (DbcsAware) {

            for (i = 0, OemLength = 0; i < Context->NameLength; i++) {

                OemLength += IsDbcsCharacter(Context->NameBuffer[i]) ? 2 : 1;

                if (OemLength >= 8 - (IndexLength + 1)) {
                    break;
                }
            }

            Context->NameLength = (UCHAR)i;

        } else {

            Context->NameLength -= 1;
        }
    }

    //
    //  And return to our caller
    //

    return;
}


BOOLEAN
RtlIsValidOemCharacter (
    IN PWCHAR Char
)

/*++

Routine Description:

    This routine determines if the best-fitted and upcased version of the
    input unicode char is a valid Oem character.

Arguments:

    Char - Supplies the Unicode char and receives the best-fitted and
        upcased version if it was indeed valid.

Return Value:

    TRUE if the character was valid.

--*/

{
    WCHAR UniTmp;
    WCHAR OemChar;

    //
    //  First try to make a round trip from Unicode->Oem->Unicode.
    //

    if (!NlsMbOemCodePageTag) {

        UniTmp = (WCHAR)NLS_UPCASE(NlsOemToUnicodeData[(UCHAR)NlsUnicodeToOemData[*Char]]);
        OemChar = NlsUnicodeToOemData[UniTmp];

    } else {

        //
        // Convert to OEM and back to Unicode before upper casing
        // to ensure the visual best fits are converted and
        // upper cased properly.
        //

        OemChar = NlsUnicodeToMbOemData[ *Char ];

        if (NlsOemLeadByteInfo[HIBYTE(OemChar)]) {

            USHORT Entry;

            //
            // Lead byte - translate the trail byte using the table
            // that corresponds to this lead byte.
            //

            Entry = NlsOemLeadByteInfo[HIBYTE(OemChar)];
            UniTmp = (WCHAR)NlsMbOemCodePageTables[ Entry + LOBYTE(OemChar) ];

        } else {

            //
            // Single byte character.
            //

            UniTmp = NlsOemToUnicodeData[LOBYTE(OemChar)];
        }

        //
        //  Now upcase this UNICODE character, and convert it to Oem.
        //

        UniTmp = (WCHAR)NLS_UPCASE(UniTmp);
        OemChar = NlsUnicodeToMbOemData[UniTmp];
    }

    //
    //  Now if the final OemChar is the default one, then there was no
    //  mapping for this UNICODE character.
    //

    if (OemChar == OemDefaultChar) {

        return FALSE;

    } else {

        *Char = UniTmp;
        return TRUE;
    }
}


//
//  Local support routine
//

WCHAR
GetNextWchar (
    IN PUNICODE_STRING Name,
    IN PULONG CurrentIndex,
    IN BOOLEAN SkipDots,
    IN BOOLEAN AllowExtendedCharacters
    )

/*++

Routine Description:

    This routine scans the input name starting at the current index and
    returns the next valid character for the long name to 8.3 generation
    algorithm.  It also updates the current index to point to the
    next character to examine.

    The user can specify if dots are skipped over or passed back.  The
    filtering done by the procedure is:

    1. Skip characters less then blanks, and larger than 127 if
       AllowExtendedCharacters is FALSE
    2. Optionally skip over dots
    3. translate the special 7 characters : + , ; = [ ] into underscores

Arguments:

    Name - Supplies the name being examined

    CurrentIndex - Supplies the index to start our examination and also
        receives the index of one beyond the character we return.

    SkipDots - Indicates whether this routine will also skip over periods

    AllowExtendedCharacters - Tell whether charaacters >= 127 are valid.

Return Value:

    WCHAR - returns the next wchar in the name string

--*/

{
    WCHAR wc;

    //
    //  Until we find out otherwise the character we are going to return
    //  is 0
    //

    wc = 0;

    //
    //  Now loop through updating the current index until we either have a character to
    //  return or until we exhaust the name buffer
    //

    while (*CurrentIndex < (ULONG)(Name->Length/2)) {

        //
        //  Get the next character in the buffer
        //

        wc = Name->Buffer[*CurrentIndex];
        *CurrentIndex += 1;

        //
        //  If the character is to be skipped over then reset wc to 0
        //

        if ((wc <= L' ') ||
            ((wc >= 127) && (!AllowExtendedCharacters || !RtlIsValidOemCharacter(&wc))) ||
            ((wc == L'.') && SkipDots)) {

            wc = 0;

        } else {

            //
            //  We have a character to return, but first translate the character is necessary
            //

            if ((wc < 0x80) && (RtlFatIllegalTable[wc/32] & (1 << (wc%32)))) {

                wc = L'_';
            }

            //
            //  Do an a-z upcase.
            //

            if ((wc >= L'a') && (wc <= L'z')) {

                wc -= L'a' - L'A';
            }

            //
            //  And break out of the loop to return to our caller
            //

            break;
        }
    }

    //DebugTrace( 0, Dbg, "GetNextWchar -> %08x\n", wc);

    return wc;
}


//
//  Internal support routine
//

USHORT
RtlComputeLfnChecksum (
    PUNICODE_STRING Name
    )

/*++

Routine Description:

    This routine computes the Chicago long file name checksum.

Arguments:

    Name - Supplies the name to compute the checksum on.  Note that one
        character names don't have interesting checksums.

Return Value:

    The checksum.

--*/

{
    ULONG i;
    USHORT Checksum;

    RTL_PAGED_CODE();

    if (Name->Length == sizeof(WCHAR)) {

        return Name->Buffer[0];
    }

    Checksum = ((Name->Buffer[0] << 8) + Name->Buffer[1]) & 0xffff;

    //
    //  This checksum is kinda strange because we want to still have
    //  a good range even if all the characters are < 0x00ff.
    //

    for (i=2; i < Name->Length / sizeof(WCHAR); i+=2) {

        Checksum = (Checksum & 1 ? 0x8000 : 0) +
                   (Checksum >> 1) +
                   (Name->Buffer[i] << 8);

        //
        //  Be carefull to not walk off the end of the string.
        //

        if (i+1 < Name->Length / sizeof(WCHAR)) {

            Checksum += Name->Buffer[i+1] & 0xffff;
        }
    }

    return Checksum;
}


BOOLEAN
RtlIsNameLegalDOS8Dot3 (
    IN PUNICODE_STRING Name,
    IN OUT POEM_STRING OemName OPTIONAL,
    OUT PBOOLEAN NameContainsSpaces OPTIONAL
    )
/*++

Routine Description:

    This routine takes an input string and gives a definitive answer
    on whether this name can successfully be used to create a file
    on the FAT file system.

    This routine can therefore also be used to determine if a name is
    appropriate to be passed back to a Win31 or DOS app, i.e. whether
    the downlevel APP will understand the name.

    Note: an important part of this test is the mapping from UNICODE
    to Oem, which is why it is important that the input parameter be
    received in UNICODE.

Arguments:

    Name - The UNICODE name to test for conformance to 8.3 symantics.

    OemName - If specified, will receive the Oem name corresponding
        to the passed in Name.  Storage must be provided by the caller.
        The name is undefined if the routine returns FALSE.

    NameContainsSpaces - If the function returns TRUE, then this
        parameter will indicate if the names contains spaces.  If
        the function returns FALSE, this parameter is undefined. In
        many instances, the alternate name is more appropriate to
        use if spaces are present in the principle name, even if
        it is 8.3 compliant.

Return Value:

    BOOLEAN - TRUE if the passed in UNICODE name forms a valid 8.3
        FAT name when upcased to the current Oem code page.

--*/

{
    ULONG Index;
    BOOLEAN ExtensionPresent = FALSE;
    BOOLEAN SpacesPresent = FALSE;
    OEM_STRING LocalOemName;
    UCHAR Char;
    UCHAR OemBuffer[12];

    //
    //  If the name is more than 12 chars, bail.
    //

    if (Name->Length > 12*sizeof(WCHAR)) {
        return FALSE;
    }

    //
    //  Now upcase this name to Oem.  If anything goes wrong,
    //  return FALSE.
    //

    if (!ARGUMENT_PRESENT(OemName)) {

        OemName = &LocalOemName;

        OemName->Buffer = &OemBuffer[0];
        OemName->Length = 0;
        OemName->MaximumLength = 12;
    }

    if (!NT_SUCCESS(RtlUpcaseUnicodeStringToCountedOemString(OemName, Name, FALSE))) {
        return FALSE;
    }

    //
    //  Special case . and ..
    //

    if (((OemName->Length == 1) && (OemName->Buffer[0] == '.')) ||
        ((OemName->Length == 2) && (OemName->Buffer[0] == '.') && (OemName->Buffer[1] == '.'))) {

        if (ARGUMENT_PRESENT(NameContainsSpaces)) {
            *NameContainsSpaces = FALSE;
        }
        return TRUE;
    }

    //
    //  Now we are going to walk through the string looking for
    //  illegal characters and/or incorrect syntax.
    //

    for ( Index = 0; Index < OemName->Length; Index += 1 ) {

        Char = OemName->Buffer[ Index ];

        //
        //  Skip over and Dbcs chacters
        //

        if (NlsMbOemCodePageTag && NlsOemLeadByteInfo[Char]) {

            //
            //  1) if we're looking at base part ( !ExtensionPresent ) and the 8th byte
            //     is in the dbcs leading byte range, it's error ( Index == 7 ). If the
            //     length of base part is more than 8 ( Index > 7 ), it's definitely error.
            //
            //  2) if the last byte ( Index == DbcsName.Length - 1 ) is in the dbcs leading
            //     byte range, it's error
            //

            if ((!ExtensionPresent && (Index >= 7)) ||
                (Index == (ULONG)(OemName->Length - 1))) {
                return FALSE;
            }

            Index += 1;

            continue;
        }

        //
        //  Make sure this character is legal.
        //

        if ((Char < 0x80) &&
            (RtlFatIllegalTable[Char/32] & (1 << (Char%32)))) {
            return FALSE;
        }

        //
        //  Remember if there was a space.
        //

        if (Char == ' ') {
            SpacesPresent = TRUE;
        }

        if (Char == '.') {

            //
            //  We stepped onto a period.  We require the following things:
            //
            //      - There can only be one
            //      - It can't be the first character
            //      - The previous character can't be a space.
            //      - There can't be more than 3 bytes following
            //

            if (ExtensionPresent ||
                (Index == 0) ||
                (OemName->Buffer[Index - 1] == ' ') ||
                (OemName->Length - (Index + 1) > 3)) {

                return FALSE;
            }

            ExtensionPresent = TRUE;
        }

        //
        //  The base part of the name can't be more than 8 characters long.
        //

        if ((Index >= 8) && !ExtensionPresent) { return FALSE; }
    }

    //
    //  The name cannot end in a space or a period.
    //

    if ((Char == ' ') || (Char == '.')) { return FALSE; }

    if (ARGUMENT_PRESENT(NameContainsSpaces)) {
        *NameContainsSpaces = SpacesPresent;
    }
    return TRUE;
}

