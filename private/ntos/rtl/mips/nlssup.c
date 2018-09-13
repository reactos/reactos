/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    nlssup.c

Abstract:

    This module defines CPU specific routines for converting NLS strings.

Author:

    Steve Wood (stevewo) 31-Mar-1989

Revision History:


--*/

#include "ntrtlp.h"

WCHAR
RtlAnsiCharToUnicodeChar(
    IN OUT PCHAR *SourceCharacter
    )

/*++

Routine Description:

    This function translates the specified ansi character to unicode and
    returns the unicode value.  The purpose for this routine is to allow
    for character by character ansi to unicode translation.  The
    translation is done with respect to the current system locale
    information.


Arguments:

    SourceCharacter - Supplies a pointer to an ansi character pointer.
        Through two levels of indirection, this supplies an ansi
        character that is to be translated to unicode.  After
        translation, the ansi character pointer is modified to point to
        the next character to be converted.  This is done to allow for
        dbcs ansi characters.

Return Value:

    Returns the unicode equivalent of the specified ansi character.

--*/

{
    WCHAR UnicodeCharacter;

    //
    // Note that this needs to reference the translation table !
    //

    UnicodeCharacter = **SourceCharacter;
    (*SourceCharacter)++;
    return UnicodeCharacter;
}


VOID
RtlpAnsiPszToUnicodePsz(
    IN PCHAR AnsiString,
    IN WCHAR *UnicodeString,
    IN USHORT AnsiStringLength
    )

/*++

Routine Description:

    This function translates the specified ansi character to unicode and
    returns the unicode value.  The purpose for this routine is to allow
    for character by character ansi to unicode translation.  The
    translation is done with respect to the current system locale
    information.


Arguments:

    AnsiString - Supplies a pointer to the ANSI string to convert to unicode.
    UnicodeString - Supplies a pointer to a buffer to hold the unicode string
    AnsiStringLength - Supplies the length of the ANSI string.

Return Value:

    None.

--*/

{
    ULONG Index;
    PCHAR AnsiChar;

    AnsiChar = AnsiString;
    Index = 0;
    while(Index < AnsiStringLength ) {
        UnicodeString[Index] = RtlAnsiCharToUnicodeChar(&AnsiChar);
        Index++;
        }
    UnicodeString[Index] = UNICODE_NULL;
}
