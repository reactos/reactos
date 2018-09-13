/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    strings.c

Abstract:

    This module defines functions for manipulating counted strings (STRING).
    A counted string is a data structure containing three fields.  The Buffer
    field is a pointer to the string itself.  The MaximumLength field contains
    the maximum number of bytes that can be stored in the memory pointed to
    by the Buffer field.  The Length field contains the current length, in
    bytes, of the string pointed to by the Buffer field.  Users of counted
    strings should not make any assumptions about the existence of a null
    byte at the end of the string, unless the null byte is explicitly
    included in the Length of the string.

Author:

    Steve Wood (stevewo) 31-Mar-1989

Revision History:

    22-Sep-1993    JulieB    Fixed TO_UPPER macro for chars above 0x7f.


--*/

#include "string.h"
#include "nt.h"
#include "ntrtlp.h"


#if defined(ALLOC_PRAGMA) && defined(NTOS_KERNEL_RUNTIME)
#pragma alloc_text(PAGE,RtlUpperChar)
#pragma alloc_text(PAGE,RtlUpperString)
#endif

//
// Global data used for translations.
//
extern PUSHORT  NlsAnsiToUnicodeData;    // Ansi CP to Unicode translation table
extern PCH      NlsUnicodeToAnsiData;    // Unicode to Ansi CP translation table
extern PUSHORT  NlsLeadByteInfo;         // Lead byte info for ACP
extern PUSHORT  NlsUnicodeToMbAnsiData;  // Unicode to Multibyte Ansi CP translation table
extern BOOLEAN  NlsMbCodePageTag;        // TRUE -> Multibyte ACP, FALSE -> Singlebyte ACP



#if !defined(_M_IX86)

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
    ULONG Length;

    DestinationString->Buffer = (PCHAR)SourceString;
    if (ARGUMENT_PRESENT( SourceString )) {
        Length = strlen(SourceString);
        DestinationString->Length = (USHORT)Length;
        DestinationString->MaximumLength = (USHORT)(Length+1);
        }
    else {
        DestinationString->Length = 0;
        DestinationString->MaximumLength = 0;
        }
}


VOID
RtlInitAnsiString(
    OUT PANSI_STRING DestinationString,
    IN PCSZ SourceString OPTIONAL
    )

/*++

Routine Description:

    The RtlInitAnsiString function initializes an NT counted string.
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
    ULONG Length;

    DestinationString->Buffer = (PCHAR)SourceString;
    if (ARGUMENT_PRESENT( SourceString )) {
        Length = strlen(SourceString);
        DestinationString->Length = (USHORT)Length;
        DestinationString->MaximumLength = (USHORT)(Length+1);
        }
    else {
        DestinationString->Length = 0;
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
    ULONG Length;

    DestinationString->Buffer = (PWSTR)SourceString;
    if (ARGUMENT_PRESENT( SourceString )) {
        Length = wcslen( SourceString ) * sizeof( WCHAR );
        DestinationString->Length = (USHORT)Length;
        DestinationString->MaximumLength = (USHORT)(Length + sizeof(UNICODE_NULL));
        }
    else {
        DestinationString->MaximumLength = 0;
        DestinationString->Length = 0;
        }
}

#endif // !defined(_M_IX86)


VOID
RtlCopyString(
    OUT PSTRING DestinationString,
    IN PSTRING SourceString OPTIONAL
    )

/*++

Routine Description:

    The RtlCopyString function copies the SourceString to the
    DestinationString.  If SourceString is not specified, then
    the Length field of DestinationString is set to zero.  The
    MaximumLength and Buffer fields of DestinationString are not
    modified by this function.

    The number of bytes copied from the SourceString is either the
    Length of SourceString or the MaximumLength of DestinationString,
    whichever is smaller.

Arguments:

    DestinationString - Pointer to the destination string.

    SourceString - Optional pointer to the source string.

Return Value:

    None.

--*/

{
    PSZ src, dst;
    ULONG n;

    if (ARGUMENT_PRESENT( SourceString )) {
        dst = DestinationString->Buffer;
        src = SourceString->Buffer;
        n = SourceString->Length;
        if ((USHORT)n > DestinationString->MaximumLength) {
            n = DestinationString->MaximumLength;
            }
        DestinationString->Length = (USHORT)n;
        while (n) {
            *dst++ = *src++;
            n--;
            }
        }
    else {
        DestinationString->Length = 0;
        }
}

CHAR
RtlUpperChar (
    register IN CHAR Character
    )

/*++

Routine Description:

    This routine returns a character uppercased
.
Arguments:

    IN CHAR Character - Supplies the character to upper case

Return Value:

    CHAR - Uppercased version of the charac
ter
--*/

{
    //
    // NOTE:  This assumes an ANSI string and it does NOT upper case
    //        DOUBLE BYTE characters properly.
    //

    //
    //  Handle a - z separately.
    //
    if (Character <= 'z') {
        if (Character >= 'a') {
            return Character ^ 0x20;
            }
        else {
            return Character;
            }
        }
    else {
        WCHAR wCh;

        /*
         *  Handle extended characters.
         */
        if (!NlsMbCodePageTag) {
            //
            //  Single byte code page.
            //
            wCh = NlsAnsiToUnicodeData[(UCHAR)Character];
            wCh = NLS_UPCASE(wCh);
            return NlsUnicodeToAnsiData[(USHORT)wCh];
            }
        else {
            //
            //  Multi byte code page.  Do nothing to the character
            //  if it's a lead byte or if the translation of the
            //  upper case Unicode character is a DBCS character.
            //
            if (!NlsLeadByteInfo[Character]) {
                wCh = NlsAnsiToUnicodeData[(UCHAR)Character];
                wCh = NLS_UPCASE(wCh);
                wCh = NlsUnicodeToMbAnsiData[(USHORT)wCh];
                if (!HIBYTE(wCh)) {
                    return LOBYTE(wCh);
                    }
                }
            }
        }

    return Character;
}

LONG
RtlCompareString(
    IN PSTRING String1,
    IN PSTRING String2,
    IN BOOLEAN CaseInSensitive
    )

/*++

Routine Description:

    The RtlCompareString function compares two counted strings.  The return
    value indicates if the strings are equal or String1 is less than String2
    or String1 is greater than String2.

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

    PUCHAR s1, s2, Limit;
    LONG n1, n2;
    UCHAR c1, c2;

    s1 = String1->Buffer;
    s2 = String2->Buffer;
    n1 = String1->Length;
    n2 = String2->Length;
    Limit = s1 + (n1 <= n2 ? n1 : n2);
    if (CaseInSensitive) {
        while (s1 < Limit) {
            c1 = *s1++;
            c2 = *s2++;
            if (c1 !=c2) {
                c1 = RtlUpperChar(c1);
                c2 = RtlUpperChar(c2);
                if (c1 != c2) {
                    return (LONG)c1 - (LONG)c2;
                }
            }
        }

    } else {
        while (s1 < Limit) {
            c1 = *s1++;
            c2 = *s2++;
            if (c1 != c2) {
                return (LONG)c1 - (LONG)c2;
            }
        }
    }

    return n1 - n2;
}

BOOLEAN
RtlEqualString(
    IN PSTRING String1,
    IN PSTRING String2,
    IN BOOLEAN CaseInSensitive
    )

/*++

Routine Description:

    The RtlEqualString function compares two counted strings for equality.

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

    PUCHAR s1, s2, Limit;
    LONG n1, n2;
    UCHAR c1, c2;

    n1 = String1->Length;
    n2 = String2->Length;
    if (n1 == n2) {
        s1 = String1->Buffer;
        s2 = String2->Buffer;
        Limit = s1 + n1;
        if (CaseInSensitive) {
            while (s1 < Limit) {
                c1 = *s1++;
                c2 = *s2++;
                if (c1 != c2) {
                    c1 = RtlUpperChar(c1);
                    c2 = RtlUpperChar(c2);
                    if (c1 != c2) {
                        return FALSE;
                    }
                }
            }

            return TRUE;

        } else {
            while (s1 < Limit) {
                c1 = *s1++;
                c2 = *s2++;
                if (c1 != c2) {
                    return FALSE;
                }
            }

            return TRUE;
        }

    } else {
        return FALSE;
    }
}

BOOLEAN
RtlPrefixString(
    IN PSTRING String1,
    IN PSTRING String2,
    IN BOOLEAN CaseInSensitive
    )

/*++

Routine Description:

    The RtlPrefixString function determines if the String1 counted string
    parameter is a prefix of the String2 counted string parameter.

    The CaseInSensitive parameter specifies if case is to be ignored when
    doing the comparison.

Arguments:

    String1 - Pointer to the first string.

    String2 - Pointer to the second string.

    CaseInsensitive - TRUE if case should be ignored when doing the
        comparison.

Return Value:

    Boolean value that is TRUE if String1 equals a prefix of String2 and
    FALSE otherwise.

--*/

{
    PSZ s1, s2;
    USHORT n;
    UCHAR c1, c2;

    s1 = String1->Buffer;
    s2 = String2->Buffer;
    n = String1->Length;
    if (String2->Length < n) {
        return( FALSE );
        }

    if (CaseInSensitive) {
        while (n) {
            c1 = *s1++;
            c2 = *s2++;

            if (c1 != c2 && RtlUpperChar(c1) != RtlUpperChar(c2)) {
                return( FALSE );
                }

            n--;
            }
        }
    else {
        while (n) {
            if (*s1++ != *s2++) {
                return( FALSE );
                }

            n--;
            }
        }

    return TRUE;
}

BOOLEAN
RtlCreateUnicodeStringFromAsciiz(
    OUT PUNICODE_STRING DestinationString,
    IN PCSZ SourceString
    )
{
    ANSI_STRING AnsiString;
    NTSTATUS Status;

    RtlInitAnsiString( &AnsiString, SourceString );
    Status = RtlAnsiStringToUnicodeString( DestinationString, &AnsiString, TRUE );
    if (NT_SUCCESS( Status )) {
        return( TRUE );
        }
    else {
        return( FALSE );
        }
}


VOID
RtlUpperString(
    IN PSTRING DestinationString,
    IN PSTRING SourceString
    )

/*++

Routine Description:

    The RtlUpperString function copies the SourceString to the
    DestinationString, converting it to upper case.  The MaximumLength
    and Buffer fields of DestinationString are not modified by this
    function.

    The number of bytes copied from the SourceString is either the
    Length of SourceString or the MaximumLength of DestinationString,
    whichever is smaller.

Arguments:

    DestinationString - Pointer to the destination string.

    SourceString - Pointer to the source string.

Return Value:

    None.

--*/

{
    PSZ src, dst;
    ULONG n;

    dst = DestinationString->Buffer;
    src = SourceString->Buffer;
    n = SourceString->Length;
    if ((USHORT)n > DestinationString->MaximumLength) {
        n = DestinationString->MaximumLength;
        }
    DestinationString->Length = (USHORT)n;
    while (n) {
        *dst++ = RtlUpperChar(*src++);
        n--;
        }
}


NTSTATUS
RtlAppendAsciizToString (
    IN PSTRING Destination,
    IN PCSZ Source OPTIONAL
    )

/*++

Routine Description:

    This routine appends the supplied ASCIIZ string to an existing PSTRING.

    It will copy bytes from the Source PSZ to the destination PSTRING up to
    the destinations PSTRING->MaximumLength field.

Arguments:

    IN PSTRING Destination, - Supplies a pointer to the destination string
    IN PSZ Source - Supplies the string to append to the destination

Return Value:

    STATUS_SUCCESS - The source string was successfully appended to the
        destination counted string.

    STATUS_BUFFER_TOO_SMALL - The destination string length was not big
        enough to allow the source string to be appended.  The Destination
        string length is not updated.

--*/

{
    USHORT n;

    if (ARGUMENT_PRESENT( Source )) {
        n = (USHORT)strlen( Source );

        if ((n + Destination->Length) > Destination->MaximumLength) {
            return( STATUS_BUFFER_TOO_SMALL );
            }

        RtlMoveMemory( &Destination->Buffer[ Destination->Length ], Source, n );
        Destination->Length += n;
        }

    return( STATUS_SUCCESS );
}



NTSTATUS
RtlAppendStringToString (
    IN PSTRING Destination,
    IN PSTRING Source
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

    if (n) {
        if ((n + Destination->Length) > Destination->MaximumLength) {
            return( STATUS_BUFFER_TOO_SMALL );
            }

        RtlMoveMemory( &Destination->Buffer[ Destination->Length ],
                       Source->Buffer,
                       n
                     );
        Destination->Length += n;
        }

    return( STATUS_SUCCESS );
}


#ifndef i386

SIZE_T
NTAPI
RtlCompareMemoryUlong(
    PVOID Source,
    SIZE_T Length,
    ULONG Pattern
    )

/*++

Routine Description:

    This function compares two blocks of memory and returns the number
    of bytes that compared equal.

    N.B. This routine requires that the source address is aligned on a
         longword boundary and that the length is an even multiple of
         longwords.

Arguments:

    Source - Supplies a pointer to the block of memory to compare against.

    Length - Supplies the Length, in bytes, of the memory to be
        compared.

    Pattern - Supplies a 32-bit pattern to compare against the block of
        memory.

Return Value:

   The number of bytes that compared equal is returned as the function
   value.  If all bytes compared equal, then the length of the orginal
   block of memory is returned.  Returns zero if either the Source
   address is not longword aligned or the length is not a multiple of
   longwords.

--*/
{
    SIZE_T CountLongs;
    PULONG p = (PULONG)Source;
    PCHAR p1, p2;

    if (((ULONG_PTR)p & (sizeof( ULONG )-1)) ||
        (Length & (sizeof( ULONG )-1))
       ) {
        return( 0 );
        }

    CountLongs = Length / sizeof( ULONG );
    while (CountLongs--) {
        if (*p++ != Pattern) {
            p1 = (PCHAR)(p - 1);
            p2 = (PCHAR)&Pattern;
            Length = p1 - (PCHAR)Source;
            while (*p1++ == *p2++) {
                if (p1 > (PCHAR)p) {
                    break;
                    }

                Length++;
                }
            }
        }

    return( Length );
}

#endif // ndef i386
