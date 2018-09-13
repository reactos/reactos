/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    nlsboot.c

Abstract:

    This module contains NLS routines for use by the OS Loader.  Before
    the NLS tables are loaded, they convert between ANSI and Unicode by
    zero-extending.

Author:

    John Vert (jvert) 11-Nov-1992

Revision History:

    John Vert (jvert) 11-Nov-1992
        created - mostly copied from old RTL routines

--*/

#include "precomp.h"
#pragma hdrstop

//
// Hack-o-rama string routines to use before tables are loaded
//

#define upcase(C) (WCHAR )(((C) >= 'a' && (C) <= 'z' ? (C) - ('a' - 'A') : (C)))


NTSTATUS
RtlAnsiStringToUnicodeString(
    OUT PUNICODE_STRING DestinationString,
    IN PANSI_STRING SourceString,
    IN BOOLEAN AllocateDestinationString
    )

{
    ULONG UnicodeLength;
    ULONG Index;
    NTSTATUS st;

    UnicodeLength = (SourceString->Length << 1) + sizeof(UNICODE_NULL);

    if ( UnicodeLength > MAXUSHORT ) {
        return STATUS_INVALID_PARAMETER_2;
        }

    DestinationString->Length = (USHORT)(UnicodeLength - sizeof(UNICODE_NULL));
    if ( AllocateDestinationString ) {
        return STATUS_NO_MEMORY;
        }
    else {
        if ( DestinationString->Length >= DestinationString->MaximumLength ) {
            return STATUS_BUFFER_OVERFLOW;
            }
        }

    Index = 0;
    while(Index < DestinationString->Length ) {
        DestinationString->Buffer[Index] = (WCHAR)SourceString->Buffer[Index];
        Index++;
        }
    DestinationString->Buffer[Index] = UNICODE_NULL;

    return STATUS_SUCCESS;
}

LONG
RtlCompareUnicodeString(
    IN PUNICODE_STRING String1,
    IN PUNICODE_STRING String2,
    IN BOOLEAN CaseInSensitive
    )

/*++

Routine Description:

    The RtlCompareUnicodeString function compares two counted strings.  The
    return value indicates if the strings are equal or String1 is less than
    String2 or String1 is greater than String2.

    The CaseInSensitive parameter specifies if case is to be ignored when
    doing the comparison.

Arguments:

    String1 - Pointer to the first string.

    String2 - Pointer to the second string.

    CaseInsensitive - TRUE if case should be ignored when doing the
        comparison.

Return Value:

    Signed value that gives the results of the comparison:

        Zero - String1 equals String2

        < Zero - String1 less than String2

        > Zero - String1 greater than String2


--*/

{

    UNALIGNED WCHAR *s1, *s2;
    USHORT n1, n2;
    WCHAR c1, c2;
    LONG cDiff;

    s1 = String1->Buffer;
    s2 = String2->Buffer;
    n1 = (USHORT )(String1->Length / sizeof(WCHAR));
    n2 = (USHORT )(String2->Length / sizeof(WCHAR));
    while (n1 && n2) {
        c1 = *s1++;
        c2 = *s2++;

        if (CaseInSensitive) {
            //
            // Note that this needs to reference the translation table !
            //
            c1 = upcase(c1);
            c2 = upcase(c2);
        }

        if ((cDiff = ((LONG)c1 - (LONG)c2)) != 0) {
            return( cDiff );
            }

        n1--;
        n2--;
        }

    return( n1 - n2 );
}

BOOLEAN
RtlEqualUnicodeString(
    IN const PUNICODE_STRING String1,
    IN const PUNICODE_STRING String2,
    IN BOOLEAN CaseInSensitive
    )

/*++

Routine Description:

    The RtlEqualUnicodeString function compares two counted unicode strings for
    equality.

    The CaseInSensitive parameter specifies if case is to be ignored when
    doing the comparison.

Arguments:

    String1 - Pointer to the first string.

    String2 - Pointer to the second string.

    CaseInsensitive - TRUE if case should be ignored when doing the
        comparison.

Return Value:

    Boolean value that is TRUE if String1 equals String2 and FALSE otherwise.

--*/

{
    UNALIGNED WCHAR *s1, *s2;
    USHORT n1, n2;
    WCHAR c1, c2;

    s1 = String1->Buffer;
    s2 = String2->Buffer;
    n1 = (USHORT )(String1->Length / sizeof(WCHAR));
    n2 = (USHORT )(String2->Length / sizeof(WCHAR));

    if ( n1 != n2 ) {
        return FALSE;
        }

    if (CaseInSensitive) {

        while ( n1 ) {

            if ( *s1++ != *s2++ ) {
                c1 = upcase(*(s1-1));
                c2 = upcase(*(s2-1));
                if (c1 != c2) {
                    return( FALSE );
                    }
                }
            n1--;
            }
        }
    else {

        while ( n1 ) {

            if (*s1++ != *s2++) {
                return( FALSE );
                }

            n1--;
            }
        }

    return TRUE;
}


VOID
RtlInitString(
    OUT PSTRING DestinationString,
    IN PCSZ SourceString OPTIONAL
    )

/*++

Routine Description:

    The RtlInitString function initializes an NT counted string.
    The DestinationString is initialized to point to the SourceString
    and the Length and MaximumLength fields of DestinationString are
    initialized to the length of the SourceString, which is zero if
    SourceString is not specified.

Arguments:

    DestinationString - Pointer to the counted string to initialize

    SourceString - Optional pointer to a null terminated string that
        the counted string is to point to.


Return Value:

    None.

--*/

{
    DestinationString->Length = 0;
    DestinationString->Buffer = (PCHAR)SourceString;
    if (ARGUMENT_PRESENT( SourceString )) {
        while (*SourceString++) {
            DestinationString->Length++;
            }

        DestinationString->MaximumLength = (SHORT)(DestinationString->Length+1);
        }
    else {
        DestinationString->MaximumLength = 0;
        }
}


VOID
RtlInitUnicodeString(
    OUT PUNICODE_STRING DestinationString,
    IN PCWSTR SourceString OPTIONAL
    )

/*++

Routine Description:

    The RtlInitUnicodeString function initializes an NT counted
    unicode string.  The DestinationString is initialized to point to
    the SourceString and the Length and MaximumLength fields of
    DestinationString are initialized to the length of the SourceString,
    which is zero if SourceString is not specified.

Arguments:

    DestinationString - Pointer to the counted string to initialize

    SourceString - Optional pointer to a null terminated unicode string that
        the counted string is to point to.


Return Value:

    None.

--*/

{
    USHORT Length = 0;
    DestinationString->Length = 0;
    DestinationString->Buffer = (PWSTR)SourceString;
    if (ARGUMENT_PRESENT( SourceString )) {
        while (*SourceString++) {
            Length += sizeof(*SourceString);
            }

        DestinationString->Length = Length;

        DestinationString->MaximumLength = Length+(USHORT)sizeof(UNICODE_NULL);
        }
    else {
        DestinationString->MaximumLength = 0;
        }
}


NTSTATUS
RtlAppendUnicodeStringToString (
    IN PUNICODE_STRING Destination,
    IN PUNICODE_STRING Source
    )

/*++

Routine Description:

    This routine will concatinate two PSTRINGs together.  It will copy
    bytes from the source up to the MaximumLength of the destination.

Arguments:

    IN PSTRING Destination, - Supplies the destination string
    IN PSTRING Source - Supplies the source for the string copy

Return Value:

    STATUS_SUCCESS - The source string was successfully appended to the
        destination counted string.

    STATUS_BUFFER_TOO_SMALL - The destination string length was not big
        enough to allow the source string to be appended.  The Destination
        string length is not updated.

--*/

{
    USHORT n = Source->Length;
    UNALIGNED WCHAR *dst;

    if (n) {
        if ((n + Destination->Length) > Destination->MaximumLength) {
            return( STATUS_BUFFER_TOO_SMALL );
            }

        dst = &Destination->Buffer[ (Destination->Length / sizeof( WCHAR )) ];
        RtlMoveMemory( dst, Source->Buffer, n );

        Destination->Length += n;

        if (Destination->Length < Destination->MaximumLength) {
            dst[ n / sizeof( WCHAR ) ] = UNICODE_NULL;
            }
        }

    return( STATUS_SUCCESS );
}


NTSTATUS
RtlAppendUnicodeToString (
    IN PUNICODE_STRING Destination,
    IN PCWSTR Source OPTIONAL
    )

/*++

Routine Description:

    This routine appends the supplied UNICODE string to an existing
    PUNICODE_STRING.

    It will copy bytes from the Source PSZ to the destination PSTRING up to
    the destinations PUNICODE_STRING->MaximumLength field.

Arguments:

    IN PUNICODE_STRING Destination, - Supplies a pointer to the destination
                            string
    IN PWSTR Source - Supplies the string to append to the destination

Return Value:

    STATUS_SUCCESS - The source string was successfully appended to the
        destination counted string.

    STATUS_BUFFER_TOO_SMALL - The destination string length was not big
        enough to allow the source string to be appended.  The Destination
        string length is not updated.

--*/

{
    USHORT n;
    UNALIGNED WCHAR *dst;

    if (ARGUMENT_PRESENT( Source )) {
        UNICODE_STRING UniSource;

        RtlInitUnicodeString(&UniSource, Source);

        n = UniSource.Length;

        if ((n + Destination->Length) > Destination->MaximumLength) {
            return( STATUS_BUFFER_TOO_SMALL );
            }

        dst = &Destination->Buffer[ (Destination->Length / sizeof( WCHAR )) ];
        RtlMoveMemory( dst, Source, n );

        Destination->Length += n;

        if (Destination->Length < Destination->MaximumLength) {
            dst[ n / sizeof( WCHAR ) ] = UNICODE_NULL;
            }
        }

    return( STATUS_SUCCESS );
}

WCHAR
RtlUpcaseUnicodeChar(
    IN WCHAR SourceCharacter
    )

/*++

Routine Description:

    This function translates the specified unicode character to its
    equivalent upcased unicode chararacter.  The purpose for this routine
    is to allow for character by character upcase translation.  The
    translation is done with respect to the current system locale
    information.


Arguments:

    SourceCharacter - Supplies the unicode character to be upcased.

Return Value:

    Returns the upcased unicode equivalent of the specified input character.

--*/

{

    return (upcase(SourceCharacter));
}

WCHAR
RtlAnsiCharToUnicodeChar(
    IN OUT PUCHAR *SourceCharacter
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
    ULONG cbCharSize;
    NTSTATUS st;


    UnicodeCharacter = (WCHAR)**SourceCharacter;
    (*SourceCharacter)++;
    return(UnicodeCharacter);
}

NTSTATUS
RtlUpcaseUnicodeToMultiByteN(
    OUT PCH MultiByteString,
    IN ULONG MaxBytesInMultiByteString,
    OUT PULONG BytesInMultiByteString OPTIONAL,
    IN PWCH UnicodeString,
    IN ULONG BytesInUnicodeString)

/*++

Routine Description:

    This functions upper cases the specified unicode source string and
    converts it into an ansi string. The translation is done with respect
    to the ANSI Code Page (ACP) loaded at boot time.

Arguments:

    MultiByteString - Returns an ansi string that is equivalent to the
        upper case of the unicode source string.  If the translation can
        not be done, an error is returned.

    MaxBytesInMultiByteString - Supplies the maximum number of bytes to be
        written to MultiByteString.  If this causes MultiByteString to be a
        truncated equivalent of UnicodeString, no error condition results.

    BytesInMultiByteString - Returns the number of bytes in the returned
        ansi string pointed to by MultiByteString.

    UnicodeString - Supplies the unicode source string that is to be
        converted to ansi.

    BytesInUnicodeString - The number of bytes in the the string pointed to by
        UnicodeString.

Return Value:

    SUCCESS - The conversion was successful

--*/

{
    ULONG TmpCount;
    ULONG LoopCount;
    ULONG CharsInUnicodeString;
    UCHAR SbChar;
    WCHAR UnicodeChar;
    ULONG i;

    //
    // Convert Unicode byte count to character count. Byte count of
    // multibyte string is equivalent to character count.
    //
    CharsInUnicodeString = BytesInUnicodeString / sizeof(WCHAR);

    LoopCount = (CharsInUnicodeString < MaxBytesInMultiByteString) ?
                 CharsInUnicodeString : MaxBytesInMultiByteString;

    if (ARGUMENT_PRESENT(BytesInMultiByteString))
        *BytesInMultiByteString = LoopCount;


    for (i=0;i<LoopCount;i++) {

        MultiByteString[i] = (UCHAR)RtlUpcaseUnicodeChar((UCHAR)(UnicodeString[i]));
    }

    return STATUS_SUCCESS;
}

NTSTATUS
RtlMultiByteToUnicodeN(
    OUT PWCH UnicodeString,
    IN ULONG MaxBytesInUnicodeString,
    OUT PULONG BytesInUnicodeString OPTIONAL,
    IN PCH MultiByteString,
    IN ULONG BytesInMultiByteString)

/*++

Routine Description:

    This functions converts the specified ansi source string into a
    Unicode string. The translation is done with respect to the
    ANSI Code Page (ACP) installed at boot time.  Single byte characters
    in the range 0x00 - 0x7f are simply zero extended as a performance
    enhancement.  In some far eastern code pages 0x5c is defined as the
    Yen sign.  For system translation we always want to consider 0x5c
    to be the backslash character.  We get this for free by zero extending.

    NOTE: This routine only supports precomposed Unicode characters.

Arguments:

    UnicodeString - Returns a unicode string that is equivalent to
        the ansi source string.

    MaxBytesInUnicodeString - Supplies the maximum number of bytes to be
        written to UnicodeString.  If this causes UnicodeString to be a
        truncated equivalent of MultiByteString, no error condition results.

    BytesInUnicodeString - Returns the number of bytes in the returned
        unicode string pointed to by UnicodeString.

    MultiByteString - Supplies the ansi source string that is to be
        converted to unicode.

    BytesInMultiByteString - The number of bytes in the string pointed to
        by MultiByteString.

Return Value:

    SUCCESS - The conversion was successful.


--*/

{
    ULONG LoopCount;
    PUSHORT TranslateTable;
    ULONG MaxCharsInUnicodeString;
    ULONG i;

    MaxCharsInUnicodeString = MaxBytesInUnicodeString / sizeof(WCHAR);

    LoopCount = (MaxCharsInUnicodeString < BytesInMultiByteString) ?
                 MaxCharsInUnicodeString : BytesInMultiByteString;

    if (ARGUMENT_PRESENT(BytesInUnicodeString))
        *BytesInUnicodeString = LoopCount * sizeof(WCHAR);

    for (i=0;i<LoopCount;i++) {
        UnicodeString[i] = (WCHAR)((UCHAR)(MultiByteString[i]));
    }

    return(STATUS_SUCCESS);
}

NTSTATUS
RtlUnicodeToMultiByteN(
    OUT PCH MultiByteString,
    IN ULONG MaxBytesInMultiByteString,
    OUT PULONG BytesInMultiByteString OPTIONAL,
    IN PWCH UnicodeString,
    IN ULONG BytesInUnicodeString)

/*++

Routine Description:

    This functions converts the specified unicode source string into an
    ansi string. The translation is done with respect to the
    ANSI Code Page (ACP) loaded at boot time.

Arguments:

    MultiByteString - Returns an ansi string that is equivalent to the
        unicode source string.  If the translation can not be done,
        an error is returned.

    MaxBytesInMultiByteString - Supplies the maximum number of bytes to be
        written to MultiByteString.  If this causes MultiByteString to be a
        truncated equivalent of UnicodeString, no error condition results.

    BytesInMultiByteString - Returns the number of bytes in the returned
        ansi string pointed to by MultiByteString.

    UnicodeString - Supplies the unicode source string that is to be
        converted to ansi.

    BytesInUnicodeString - The number of bytes in the the string pointed to by
        UnicodeString.

Return Value:

    SUCCESS - The conversion was successful

--*/

{
    ULONG TmpCount;
    ULONG LoopCount;
    ULONG CharsInUnicodeString;
    UCHAR SbChar;
    WCHAR UnicodeChar;
    ULONG i;

    //
    // Convert Unicode byte count to character count. Byte count of
    // multibyte string is equivalent to character count.
    //
    CharsInUnicodeString = BytesInUnicodeString / sizeof(WCHAR);

    LoopCount = (CharsInUnicodeString < MaxBytesInMultiByteString) ?
                 CharsInUnicodeString : MaxBytesInMultiByteString;

    if (ARGUMENT_PRESENT(BytesInMultiByteString))
        *BytesInMultiByteString = LoopCount;


    for (i=0;i<LoopCount;i++) {
        MultiByteString[i] = (CHAR)UnicodeString[i];
    }

    return STATUS_SUCCESS;

}
