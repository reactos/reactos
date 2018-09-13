/****************************** Module Header ******************************\
* Module Name: nlsxlat.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* This modules contains the private routines for character translation:
* 8-bit <=> Unicode.
*
* History:
* 03-Jan-1992    gregoryw
* 16-Feb-1993    JulieB      Added Upcase Routines & Macros.
* 17-Feb-1993    JulieB      Fixed Tables; Fixed DBCS Code.
* 08-Mar-1993    JulieB      Moved Upcase Macro to ntrtlp.h.
\***************************************************************************/

#include "ntrtlp.h"


NTSTATUS
RtlConsoleMultiByteToUnicodeN(
    OUT PWCH UnicodeString,
    IN ULONG MaxBytesInUnicodeString,
    OUT PULONG BytesInUnicodeString OPTIONAL,
    IN PCH MultiByteString,
    IN ULONG BytesInMultiByteString,
    OUT PULONG pdwSpecialChar );

VOID
RtlpInitUpcaseTable(
    IN PUSHORT TableBase,
    OUT PNLSTABLEINFO CodePageTable
    );

#if defined(ALLOC_PRAGMA) && defined(NTOS_KERNEL_RUNTIME)
#pragma alloc_text(PAGE,RtlConsoleMultiByteToUnicodeN)
#pragma alloc_text(PAGE,RtlMultiByteToUnicodeN)
#pragma alloc_text(PAGE,RtlOemToUnicodeN)
#pragma alloc_text(PAGE,RtlUnicodeToMultiByteN)
#pragma alloc_text(PAGE,RtlUpcaseUnicodeToMultiByteN)
#pragma alloc_text(PAGE,RtlUnicodeToOemN)
#pragma alloc_text(PAGE,RtlUpcaseUnicodeToOemN)
#pragma alloc_text(PAGE,RtlpDidUnicodeToOemWork)
#pragma alloc_text(PAGE,RtlCustomCPToUnicodeN)
#pragma alloc_text(PAGE,RtlUnicodeToCustomCPN)
#pragma alloc_text(PAGE,RtlUpcaseUnicodeToCustomCPN)
#pragma alloc_text(PAGE,RtlInitCodePageTable)
#pragma alloc_text(PAGE,RtlpInitUpcaseTable)
#pragma alloc_text(PAGE,RtlInitNlsTables)
#pragma alloc_text(PAGE,RtlResetRtlTranslations)
#pragma alloc_text(PAGE,RtlMultiByteToUnicodeSize)
#pragma alloc_text(PAGE,RtlUnicodeToMultiByteSize)
#pragma alloc_text(PAGE,RtlGetDefaultCodePage)
#endif



//
// Various defines and convenient macros for data access
//

#define DBCS_TABLE_SIZE 256


/*
 * Global data used by the translation routines.
 *
 */

//
// Upcase and Lowercase data
//
PUSHORT Nls844UnicodeUpcaseTable;
PUSHORT Nls844UnicodeLowercaseTable;

//
// ACP related data
//
USHORT   NlsLeadByteInfoTable[DBCS_TABLE_SIZE]; // Lead byte info. for ACP
USHORT   NlsAnsiCodePage;                  // Default ANSI code page
USHORT   NlsOemCodePage;                   // Default OEM code page
PUSHORT  NlsLeadByteInfo = NlsLeadByteInfoTable;
PUSHORT  NlsMbAnsiCodePageTables;          // Multibyte to Unicode translation tables
PUSHORT  NlsAnsiToUnicodeData;             // Ansi CP to Unicode translation table
PCH      NlsUnicodeToAnsiData;             // Unicode to Ansi CP translation table
PUSHORT  NlsUnicodeToMbAnsiData;           // Unicode to Multibyte Ansi CP translation table
BOOLEAN  NlsMbCodePageTag = FALSE;         // TRUE -> Multibyte ACP, FALSE -> Singlebyte ACP

//
// OEM related data
//
USHORT   NlsOemLeadByteInfoTable[DBCS_TABLE_SIZE]; // Lead byte info. for 0CP
PUSHORT  NlsOemLeadByteInfo = NlsOemLeadByteInfoTable;
PUSHORT  NlsMbOemCodePageTables;              // OEM Multibyte to Unicode translation tables
PUSHORT  NlsOemToUnicodeData;                 // Oem CP to Unicode translation table
PCH      NlsUnicodeToOemData;                 // Unicode to Oem CP translation table
PUSHORT  NlsUnicodeToMbOemData;               // Unicode to Multibyte Oem CP translation table
BOOLEAN  NlsMbOemCodePageTag = FALSE;         // TRUE -> Multibyte OCP, FALSE -> Singlebyte OCP

//
// Default info taken from data files
//
USHORT   UnicodeDefaultChar;

USHORT   OemDefaultChar;
USHORT   OemTransUniDefaultChar;

//
// Default info NOT taken from data files
//
USHORT   UnicodeNull  = 0x0000;



NTSTATUS
RtlConsoleMultiByteToUnicodeN(
    OUT PWCH UnicodeString,
    IN ULONG MaxBytesInUnicodeString,
    OUT PULONG BytesInUnicodeString OPTIONAL,
    IN PCH MultiByteString,
    IN ULONG BytesInMultiByteString,
    OUT PULONG pdwSpecialChar )

/*++

Routine Description:

    This function is a superset of MultiByteToUnicode for the
    console.  It works just like the other, except it will detect
    if any characters were under 0x20.

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

    pdwSpecialChar - will be zero if non detected, else it will contain the
       approximate index (can be off by 32).

Return Value:

    SUCCESS - The conversion was successful.


--*/

{
    ULONG LoopCount;
    PUSHORT TranslateTable;
    ULONG MaxCharsInUnicodeString;

    RTL_PAGED_CODE();

    *pdwSpecialChar = 0;

    if (!NlsMbCodePageTag) {
        MaxCharsInUnicodeString = MaxBytesInUnicodeString / sizeof(WCHAR);

        LoopCount = (MaxCharsInUnicodeString < BytesInMultiByteString) ?
                     MaxCharsInUnicodeString : BytesInMultiByteString;

        if (ARGUMENT_PRESENT(BytesInUnicodeString))
            *BytesInUnicodeString = LoopCount * sizeof(WCHAR);

        TranslateTable = NlsAnsiToUnicodeData;  // used to help the mips compiler

        quick_copy:
            switch( LoopCount ) {
            default:
                if ((UCHAR)MultiByteString[0x1F] < 0x20)    goto  bad_case;
                UnicodeString[0x1F] = TranslateTable[(UCHAR)MultiByteString[0x1F]];
            case 0x1F:
                if ((UCHAR)MultiByteString[0x1E] < 0x20)    goto  bad_case;
                UnicodeString[0x1E] = TranslateTable[(UCHAR)MultiByteString[0x1E]];
            case 0x1E:
                if ((UCHAR)MultiByteString[0x1D] < 0x20)    goto  bad_case;
                UnicodeString[0x1D] = TranslateTable[(UCHAR)MultiByteString[0x1D]];
            case 0x1D:
                if ((UCHAR)MultiByteString[0x1C] < 0x20)    goto  bad_case;
                UnicodeString[0x1C] = TranslateTable[(UCHAR)MultiByteString[0x1C]];
            case 0x1C:
                if ((UCHAR)MultiByteString[0x1B] < 0x20)    goto  bad_case;
                UnicodeString[0x1B] = TranslateTable[(UCHAR)MultiByteString[0x1B]];
            case 0x1B:
                if ((UCHAR)MultiByteString[0x1A] < 0x20)    goto  bad_case;
                UnicodeString[0x1A] = TranslateTable[(UCHAR)MultiByteString[0x1A]];
            case 0x1A:
                if ((UCHAR)MultiByteString[0x19] < 0x20)    goto  bad_case;
                UnicodeString[0x19] = TranslateTable[(UCHAR)MultiByteString[0x19]];
            case 0x19:
                if ((UCHAR)MultiByteString[0x18] < 0x20)    goto  bad_case;
                UnicodeString[0x18] = TranslateTable[(UCHAR)MultiByteString[0x18]];
            case 0x18:
                if ((UCHAR)MultiByteString[0x17] < 0x20)    goto  bad_case;
                UnicodeString[0x17] = TranslateTable[(UCHAR)MultiByteString[0x17]];
            case 0x17:
                if ((UCHAR)MultiByteString[0x16] < 0x20)    goto  bad_case;
                UnicodeString[0x16] = TranslateTable[(UCHAR)MultiByteString[0x16]];
            case 0x16:
                if ((UCHAR)MultiByteString[0x15] < 0x20)    goto  bad_case;
                UnicodeString[0x15] = TranslateTable[(UCHAR)MultiByteString[0x15]];
            case 0x15:
                if ((UCHAR)MultiByteString[0x14] < 0x20)    goto  bad_case;
                UnicodeString[0x14] = TranslateTable[(UCHAR)MultiByteString[0x14]];
            case 0x14:
                if ((UCHAR)MultiByteString[0x13] < 0x20)    goto  bad_case;
                UnicodeString[0x13] = TranslateTable[(UCHAR)MultiByteString[0x13]];
            case 0x13:
                if ((UCHAR)MultiByteString[0x12] < 0x20)    goto  bad_case;
                UnicodeString[0x12] = TranslateTable[(UCHAR)MultiByteString[0x12]];
            case 0x12:
                if ((UCHAR)MultiByteString[0x11] < 0x20)    goto  bad_case;
                UnicodeString[0x11] = TranslateTable[(UCHAR)MultiByteString[0x11]];
            case 0x11:
                if ((UCHAR)MultiByteString[0x10] < 0x20)    goto  bad_case;
                UnicodeString[0x10] = TranslateTable[(UCHAR)MultiByteString[0x10]];
            case 0x10:
                if ((UCHAR)MultiByteString[0x0F] < 0x20)    goto  bad_case;
                UnicodeString[0x0F] = TranslateTable[(UCHAR)MultiByteString[0x0F]];
            case 0x0F:
                if ((UCHAR)MultiByteString[0x0E] < 0x20)    goto  bad_case;
                UnicodeString[0x0E] = TranslateTable[(UCHAR)MultiByteString[0x0E]];
            case 0x0E:
                if ((UCHAR)MultiByteString[0x0D] < 0x20)    goto  bad_case;
                UnicodeString[0x0D] = TranslateTable[(UCHAR)MultiByteString[0x0D]];
            case 0x0D:
                if ((UCHAR)MultiByteString[0x0C] < 0x20)    goto  bad_case;
                UnicodeString[0x0C] = TranslateTable[(UCHAR)MultiByteString[0x0C]];
            case 0x0C:
                if ((UCHAR)MultiByteString[0x0B] < 0x20)    goto  bad_case;
                UnicodeString[0x0B] = TranslateTable[(UCHAR)MultiByteString[0x0B]];
            case 0x0B:
                if ((UCHAR)MultiByteString[0x0A] < 0x20)    goto  bad_case;
                UnicodeString[0x0A] = TranslateTable[(UCHAR)MultiByteString[0x0A]];
            case 0x0A:
                if ((UCHAR)MultiByteString[0x09] < 0x20)    goto  bad_case;
                UnicodeString[0x09] = TranslateTable[(UCHAR)MultiByteString[0x09]];
            case 0x09:
                if ((UCHAR)MultiByteString[0x08] < 0x20)    goto  bad_case;
                UnicodeString[0x08] = TranslateTable[(UCHAR)MultiByteString[0x08]];
            case 0x08:
                if ((UCHAR)MultiByteString[0x07] < 0x20)    goto  bad_case;
                UnicodeString[0x07] = TranslateTable[(UCHAR)MultiByteString[0x07]];
            case 0x07:
                if ((UCHAR)MultiByteString[0x06] < 0x20)    goto  bad_case;
                UnicodeString[0x06] = TranslateTable[(UCHAR)MultiByteString[0x06]];
            case 0x06:
                if ((UCHAR)MultiByteString[0x05] < 0x20)    goto  bad_case;
                UnicodeString[0x05] = TranslateTable[(UCHAR)MultiByteString[0x05]];
            case 0x05:
                if ((UCHAR)MultiByteString[0x04] < 0x20)    goto  bad_case;
                UnicodeString[0x04] = TranslateTable[(UCHAR)MultiByteString[0x04]];
            case 0x04:
                if ((UCHAR)MultiByteString[0x03] < 0x20)    goto  bad_case;
                UnicodeString[0x03] = TranslateTable[(UCHAR)MultiByteString[0x03]];
            case 0x03:
                if ((UCHAR)MultiByteString[0x02] < 0x20)    goto  bad_case;
                UnicodeString[0x02] = TranslateTable[(UCHAR)MultiByteString[0x02]];
            case 0x02:
                if ((UCHAR)MultiByteString[0x01] < 0x20)    goto  bad_case;
                UnicodeString[0x01] = TranslateTable[(UCHAR)MultiByteString[0x01]];
            case 0x01:
                if ((UCHAR)MultiByteString[0x00] < 0x20)    goto  bad_case;
                UnicodeString[0x00] = TranslateTable[(UCHAR)MultiByteString[0x00]];
            case 0x00:
                ;
            }

            if ( LoopCount > 0x20 ) {
                LoopCount -= 0x20;
                UnicodeString += 0x20;
                MultiByteString += 0x20;

                goto  quick_copy;
            }
        /* end of copy... */
    } else {
        register USHORT Entry;

        PWCH UnicodeStringAnchor = UnicodeString;
        TranslateTable = (PUSHORT)NlsMbAnsiCodePageTables;

        // yea, this line is duplicated, but the compiler can handle opt
        // better by moving it around
        MaxCharsInUnicodeString = MaxBytesInUnicodeString / sizeof(WCHAR);

        //
        // The ACP is a multibyte code page.  Check each character
        // to see if it is a lead byte before doing the translation.
        //
        while (MaxCharsInUnicodeString && BytesInMultiByteString) {
            MaxCharsInUnicodeString--;
            BytesInMultiByteString--;
            if (NlsLeadByteInfo[*(PUCHAR)MultiByteString]) {
                //
                // Lead byte - Make sure there is a trail byte.  If not,
                // pass back a space rather than an error.  Some 3.x
                // applications pass incorrect strings and don't expect
                // to get an error.
                //
                if (BytesInMultiByteString == 0)
                {
                    *UnicodeString++ = UnicodeNull;
                    break;
                }

                //
                // Get the unicode character.
                //
                Entry = NlsLeadByteInfo[*(PUCHAR)MultiByteString++];
                *UnicodeString = (WCHAR)TranslateTable[ Entry + *(PUCHAR)MultiByteString++ ];
                UnicodeString++;

                //
                // Decrement count of bytes in multibyte string to account
                // for the double byte character.
                //
                BytesInMultiByteString--;
            } else {
                //
                // Single byte character.
                //
                if ((UCHAR)MultiByteString[0x00] < 0x20)
                    *pdwSpecialChar = 1;
                *UnicodeString++ = NlsAnsiToUnicodeData[*(PUCHAR)MultiByteString++];
            }
        }

        if (ARGUMENT_PRESENT(BytesInUnicodeString))
            *BytesInUnicodeString = (ULONG)((PCH)UnicodeString - (PCH)UnicodeStringAnchor);
    }

    return STATUS_SUCCESS;

    bad_case:
        //
        // this is a low probability case, so we optimized the loop.  If have a
        // special char, finish trans and notify caller.
        //
        *pdwSpecialChar = 1;
        return RtlMultiByteToUnicodeN(UnicodeString, MaxBytesInUnicodeString,
                NULL, MultiByteString, LoopCount);
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
        converted to unicode.  For single-byte character sets, this address
        CAN be the same as UnicodeString.

    BytesInMultiByteString - The number of bytes in the string pointed to
        by MultiByteString.

Return Value:

    SUCCESS - The conversion was successful.


--*/

{
    ULONG LoopCount;
    ULONG TmpCount;
    PUSHORT TranslateTable;
    ULONG MaxCharsInUnicodeString;

    RTL_PAGED_CODE();

    if (!NlsMbCodePageTag) {
        MaxCharsInUnicodeString = MaxBytesInUnicodeString / sizeof(WCHAR);

        LoopCount = (MaxCharsInUnicodeString < BytesInMultiByteString) ?
                     MaxCharsInUnicodeString : BytesInMultiByteString;

        if (ARGUMENT_PRESENT(BytesInUnicodeString))
            *BytesInUnicodeString = LoopCount * sizeof(WCHAR);

        TranslateTable = NlsAnsiToUnicodeData;  // used to help the mips compiler

        TmpCount = LoopCount & 0x1F;
        UnicodeString += (LoopCount - TmpCount);
        MultiByteString += (LoopCount - TmpCount);
        quick_copy:
            switch( TmpCount ) {
            default:
                UnicodeString[0x1F] = TranslateTable[(UCHAR)MultiByteString[0x1F]];
            case 0x1F:
                UnicodeString[0x1E] = TranslateTable[(UCHAR)MultiByteString[0x1E]];
            case 0x1E:
                UnicodeString[0x1D] = TranslateTable[(UCHAR)MultiByteString[0x1D]];
            case 0x1D:
                UnicodeString[0x1C] = TranslateTable[(UCHAR)MultiByteString[0x1C]];
            case 0x1C:
                UnicodeString[0x1B] = TranslateTable[(UCHAR)MultiByteString[0x1B]];
            case 0x1B:
                UnicodeString[0x1A] = TranslateTable[(UCHAR)MultiByteString[0x1A]];
            case 0x1A:
                UnicodeString[0x19] = TranslateTable[(UCHAR)MultiByteString[0x19]];
            case 0x19:
                UnicodeString[0x18] = TranslateTable[(UCHAR)MultiByteString[0x18]];
            case 0x18:
                UnicodeString[0x17] = TranslateTable[(UCHAR)MultiByteString[0x17]];
            case 0x17:
                UnicodeString[0x16] = TranslateTable[(UCHAR)MultiByteString[0x16]];
            case 0x16:
                UnicodeString[0x15] = TranslateTable[(UCHAR)MultiByteString[0x15]];
            case 0x15:
                UnicodeString[0x14] = TranslateTable[(UCHAR)MultiByteString[0x14]];
            case 0x14:
                UnicodeString[0x13] = TranslateTable[(UCHAR)MultiByteString[0x13]];
            case 0x13:
                UnicodeString[0x12] = TranslateTable[(UCHAR)MultiByteString[0x12]];
            case 0x12:
                UnicodeString[0x11] = TranslateTable[(UCHAR)MultiByteString[0x11]];
            case 0x11:
                UnicodeString[0x10] = TranslateTable[(UCHAR)MultiByteString[0x10]];
            case 0x10:
                UnicodeString[0x0F] = TranslateTable[(UCHAR)MultiByteString[0x0F]];
            case 0x0F:
                UnicodeString[0x0E] = TranslateTable[(UCHAR)MultiByteString[0x0E]];
            case 0x0E:
                UnicodeString[0x0D] = TranslateTable[(UCHAR)MultiByteString[0x0D]];
            case 0x0D:
                UnicodeString[0x0C] = TranslateTable[(UCHAR)MultiByteString[0x0C]];
            case 0x0C:
                UnicodeString[0x0B] = TranslateTable[(UCHAR)MultiByteString[0x0B]];
            case 0x0B:
                UnicodeString[0x0A] = TranslateTable[(UCHAR)MultiByteString[0x0A]];
            case 0x0A:
                UnicodeString[0x09] = TranslateTable[(UCHAR)MultiByteString[0x09]];
            case 0x09:
                UnicodeString[0x08] = TranslateTable[(UCHAR)MultiByteString[0x08]];
            case 0x08:
                UnicodeString[0x07] = TranslateTable[(UCHAR)MultiByteString[0x07]];
            case 0x07:
                UnicodeString[0x06] = TranslateTable[(UCHAR)MultiByteString[0x06]];
            case 0x06:
                UnicodeString[0x05] = TranslateTable[(UCHAR)MultiByteString[0x05]];
            case 0x05:
                UnicodeString[0x04] = TranslateTable[(UCHAR)MultiByteString[0x04]];
            case 0x04:
                UnicodeString[0x03] = TranslateTable[(UCHAR)MultiByteString[0x03]];
            case 0x03:
                UnicodeString[0x02] = TranslateTable[(UCHAR)MultiByteString[0x02]];
            case 0x02:
                UnicodeString[0x01] = TranslateTable[(UCHAR)MultiByteString[0x01]];
            case 0x01:
                UnicodeString[0x00] = TranslateTable[(UCHAR)MultiByteString[0x00]];
            case 0x00:
                ;
            }

            if ( LoopCount >= 0x20 ) {
                TmpCount = 0x20;
                LoopCount -= 0x20;
                UnicodeString -= 0x20;
                MultiByteString -= 0x20;

                goto  quick_copy;
            }
        /* end of copy... */
    } else {
        register USHORT Entry;
        PWCH UnicodeStringAnchor = UnicodeString;
        TranslateTable = (PUSHORT)NlsMbAnsiCodePageTables;

        // yea, this line is duplicated, but the compiler can handle opt
        // better buy moving it around
        MaxCharsInUnicodeString = MaxBytesInUnicodeString / sizeof(WCHAR);

        //
        // The ACP is a multibyte code page.  Check each character
        // to see if it is a lead byte before doing the translation.
        //
        while (MaxCharsInUnicodeString && BytesInMultiByteString) {
            MaxCharsInUnicodeString--;
            BytesInMultiByteString--;
            if (NlsLeadByteInfo[*(PUCHAR)MultiByteString]) {
                //
                // Lead byte - Make sure there is a trail byte.  If not,
                // pass back a space rather than an error.  Some 3.x
                // applications pass incorrect strings and don't expect
                // to get an error.
                //
                if (BytesInMultiByteString == 0)
                {
                    *UnicodeString++ = UnicodeNull;
                    break;
                }

                //
                // Get the unicode character.
                //
                Entry = NlsLeadByteInfo[*(PUCHAR)MultiByteString++];
                *UnicodeString = (WCHAR)TranslateTable[ Entry + *(PUCHAR)MultiByteString++ ];
                UnicodeString++;

                //
                // Decrement count of bytes in multibyte string to account
                // for the double byte character.
                //
                BytesInMultiByteString--;
            } else {
                //
                // Single byte character.
                //
                *UnicodeString++ = NlsAnsiToUnicodeData[*(PUCHAR)MultiByteString++];
            }
        }

        if (ARGUMENT_PRESENT(BytesInUnicodeString))
            *BytesInUnicodeString = (ULONG)((PCH)UnicodeString - (PCH)UnicodeStringAnchor);
    }

    return STATUS_SUCCESS;

}


NTSTATUS
RtlOemToUnicodeN(
    OUT PWCH UnicodeString,
    IN ULONG MaxBytesInUnicodeString,
    OUT PULONG BytesInUnicodeString OPTIONAL,
    IN PCH OemString,
    IN ULONG BytesInOemString)

/*++

Routine Description:

    This functions converts the specified oem source string into a
    Unicode string. The translation is done with respect to the
    OEM Code Page (OCP) installed at boot time.  Single byte characters
    in the range 0x00 - 0x7f are simply zero extended as a performance
    enhancement.  In some far eastern code pages 0x5c is defined as the
    Yen sign.  For system translation we always want to consider 0x5c
    to be the backslash character.  We get this for free by zero extending.

    NOTE: This routine only supports precomposed Unicode characters.

Arguments:

    UnicodeString - Returns a unicode string that is equivalent to
        the oem source string.

    MaxBytesInUnicodeString - Supplies the maximum number of bytes to be
        written to UnicodeString.  If this causes UnicodeString to be a
        truncated equivalent of OemString, no error condition results.

    BytesInUnicodeString - Returns the number of bytes in the returned
        unicode string pointed to by UnicodeString.

    OemString - Supplies the oem source string that is to be
        converted to unicode.

    BytesInOemString - The number of bytes in the string pointed to
        by OemString.

Return Value:

    SUCCESS - The conversion was successful

    STATUS_ILLEGAL_CHARACTER - The final Oem character was illegal

    STATUS_BUFFER_OVERFLOW - MaxBytesInUnicodeString was not enough to hold
        the whole Oem string.  It was converted correct to the point though.

--*/

{
    ULONG LoopCount;
    PUSHORT TranslateTable;
    ULONG MaxCharsInUnicodeString;

    RTL_PAGED_CODE();

    if (!NlsMbOemCodePageTag) {
        //
        // The OCP is a single byte code page.
        //
        MaxCharsInUnicodeString = MaxBytesInUnicodeString / sizeof(WCHAR);

        LoopCount = (MaxCharsInUnicodeString < BytesInOemString) ?
                     MaxCharsInUnicodeString : BytesInOemString;

        if (ARGUMENT_PRESENT(BytesInUnicodeString))
            *BytesInUnicodeString = LoopCount * sizeof(WCHAR);


        TranslateTable = NlsOemToUnicodeData;  // used to help the mips compiler

        quick_copy:
            switch( LoopCount ) {
            default:
                UnicodeString[0x0F] = TranslateTable[(UCHAR)OemString[0x0F]];
            case 0x0F:
                UnicodeString[0x0E] = TranslateTable[(UCHAR)OemString[0x0E]];
            case 0x0E:
                UnicodeString[0x0D] = TranslateTable[(UCHAR)OemString[0x0D]];
            case 0x0D:
                UnicodeString[0x0C] = TranslateTable[(UCHAR)OemString[0x0C]];
            case 0x0C:
                UnicodeString[0x0B] = TranslateTable[(UCHAR)OemString[0x0B]];
            case 0x0B:
                UnicodeString[0x0A] = TranslateTable[(UCHAR)OemString[0x0A]];
            case 0x0A:
                UnicodeString[0x09] = TranslateTable[(UCHAR)OemString[0x09]];
            case 0x09:
                UnicodeString[0x08] = TranslateTable[(UCHAR)OemString[0x08]];
            case 0x08:
                UnicodeString[0x07] = TranslateTable[(UCHAR)OemString[0x07]];
            case 0x07:
                UnicodeString[0x06] = TranslateTable[(UCHAR)OemString[0x06]];
            case 0x06:
                UnicodeString[0x05] = TranslateTable[(UCHAR)OemString[0x05]];
            case 0x05:
                UnicodeString[0x04] = TranslateTable[(UCHAR)OemString[0x04]];
            case 0x04:
                UnicodeString[0x03] = TranslateTable[(UCHAR)OemString[0x03]];
            case 0x03:
                UnicodeString[0x02] = TranslateTable[(UCHAR)OemString[0x02]];
            case 0x02:
                UnicodeString[0x01] = TranslateTable[(UCHAR)OemString[0x01]];
            case 0x01:
                UnicodeString[0x00] = TranslateTable[(UCHAR)OemString[0x00]];
            case 0x00:
                ;
            }

            if ( LoopCount > 0x10 ) {
                LoopCount -= 0x10;
                OemString += 0x10;
                UnicodeString += 0x10;

                goto  quick_copy;
            }
        /* end of copy... */
    } else {
        register USHORT Entry;
        PWCH UnicodeStringAnchor = UnicodeString;

        //
        // The OCP is a multibyte code page.  Check each character
        // to see if it is a lead byte before doing the translation.
        //
        MaxCharsInUnicodeString = MaxBytesInUnicodeString / sizeof(WCHAR);
        TranslateTable = (PUSHORT)NlsMbOemCodePageTables;

        while (MaxCharsInUnicodeString && BytesInOemString) {
            MaxCharsInUnicodeString--;
            BytesInOemString--;
            if (NlsOemLeadByteInfo[*(PUCHAR)OemString]) {
                //
                // Lead byte - Make sure there is a trail byte.  If not,
                // pass back a space rather than an error.  Some 3.x
                // applications pass incorrect strings and don't expect
                // to get an error.
                //
                if (BytesInOemString == 0)
                {
                    *UnicodeString++ = UnicodeNull;
                    break;
                }

                //
                // Get the unicode character.
                //
                Entry = NlsOemLeadByteInfo[*(PUCHAR)OemString++];
                *UnicodeString = TranslateTable[ Entry + *(PUCHAR)OemString++ ];
                UnicodeString++;

                //
                // Decrement count of bytes in oem string to account
                // for the double byte character.
                //
                BytesInOemString--;
            } else {
                //
                // Single byte character.
                //
                *UnicodeString++ = NlsOemToUnicodeData[*(PUCHAR)OemString++];
            }
        }

        if (ARGUMENT_PRESENT(BytesInUnicodeString))
            *BytesInUnicodeString = (ULONG)((PCH)UnicodeString - (PCH)UnicodeStringAnchor);
    }

    //
    //  Check if we were able to use all of the source Oem String
    //
    return (BytesInOemString <= MaxCharsInUnicodeString) ?
           STATUS_SUCCESS :
           STATUS_BUFFER_OVERFLOW;
}


NTSTATUS
RtlMultiByteToUnicodeSize(
    OUT PULONG BytesInUnicodeString,
    IN PCH MultiByteString,
    IN ULONG BytesInMultiByteString)

/*++

Routine Description:

    This functions determines how many bytes would be needed to represent
    the specified ANSI source string in Unicode string (not counting the
    null terminator)
    The translation is done with respect to the ANSI Code Page (ACP) installed
    at boot time.  Single byte characters in the range 0x00 - 0x7f are simply
    zero extended as a performance enhancement.  In some far eastern code pages
    0x5c is defined as the Yen sign.  For system translation we always want to
    consider 0x5c to be the backslash character.  We get this for free by zero
    extending.

    NOTE: This routine only supports precomposed Unicode characters.

Arguments:

    BytesInUnicodeString - Returns the number of bytes a Unicode translation
        of the ANSI string pointed to by MultiByteString would contain.

    MultiByteString - Supplies the ansi source string whose Unicode length
        is to be calculated.

    BytesInMultiByteString - The number of bytes in the string pointed to
        by MultiByteString.

Return Value:

    SUCCESS - The conversion was successful


--*/

{
    ULONG cbUnicode = 0;

    RTL_PAGED_CODE();

    if (NlsMbCodePageTag) {
        //
        // The ACP is a multibyte code page.  Check each character
        // to see if it is a lead byte before doing the translation.
        //
        while (BytesInMultiByteString--) {
            if (NlsLeadByteInfo[*(PUCHAR)MultiByteString++]) {
                //
                // Lead byte - translate the trail byte using the table
                // that corresponds to this lead byte.  NOTE: make sure
                // we have a trail byte to convert.
                //
                if (BytesInMultiByteString == 0) {
                    //
                    // RtlMultibyteToUnicodeN() uses the unicode
                    // default character if the last multibyte
                    // character is a lead byte.
                    //
                    cbUnicode += sizeof(WCHAR);
                    break;
                } else {
                    BytesInMultiByteString--;
                    MultiByteString++;
                }
            }
            cbUnicode += sizeof(WCHAR);
        }
        *BytesInUnicodeString = cbUnicode;
    } else {
        //
        // The ACP is a single byte code page.
        //
        *BytesInUnicodeString = BytesInMultiByteString * sizeof(WCHAR);
    }

    return STATUS_SUCCESS;
}


NTSTATUS
RtlUnicodeToMultiByteSize(
    OUT PULONG BytesInMultiByteString,
    IN PWCH UnicodeString,
    IN ULONG BytesInUnicodeString)

/*++

Routine Description:

    This functions determines how many bytes would be needed to represent
    the specified Unicode source string as an ANSI string (not counting the
    null terminator)

Arguments:

    BytesInMultiByteString - Returns the number of bytes an ANSI translation
        of the Unicode string pointed to by UnicodeString would contain.

    UnicodeString - Supplies the unicode source string whose ANSI length
        is to be calculated.

    BytesInUnicodeString - The number of bytes in the the string pointed to by
        UnicodeString.

Return Value:

    SUCCESS - The conversion was successful

    !SUCCESS - The conversion failed.  A unicode character was encountered
        that has no translation for the current ANSI Code Page (ACP).

--*/

{
    ULONG cbMultiByte = 0;
    ULONG CharsInUnicodeString;

    RTL_PAGED_CODE();

    /*
     * convert from bytes to chars for easier loop handling.
     */
    CharsInUnicodeString = BytesInUnicodeString / sizeof(WCHAR);

    if (NlsMbCodePageTag) {
        USHORT MbChar;

        while (CharsInUnicodeString--) {
            MbChar = NlsUnicodeToMbAnsiData[ *UnicodeString++ ];
            if (HIBYTE(MbChar) == 0) {
                cbMultiByte++ ;
            } else {
                cbMultiByte += 2;
            }
        }
        *BytesInMultiByteString = cbMultiByte;
    }
    else {
        *BytesInMultiByteString = CharsInUnicodeString;
    }

    return STATUS_SUCCESS;
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
    PCH TranslateTable;
    ULONG CharsInUnicodeString;

    RTL_PAGED_CODE();

    //
    // Convert Unicode byte count to character count. Byte count of
    // multibyte string is equivalent to character count.
    //
    if (!NlsMbCodePageTag) {
        CharsInUnicodeString = BytesInUnicodeString / sizeof(WCHAR);

        LoopCount = (CharsInUnicodeString < MaxBytesInMultiByteString) ?
                     CharsInUnicodeString : MaxBytesInMultiByteString;

        if (ARGUMENT_PRESENT(BytesInMultiByteString))
            *BytesInMultiByteString = LoopCount;

        TranslateTable = NlsUnicodeToAnsiData;  // used to help the mips compiler

        TmpCount = LoopCount & 0x0F;
        UnicodeString += TmpCount;
        MultiByteString += TmpCount;

        do
        {
            switch( TmpCount ) {
            default:
                UnicodeString += 0x10;
                MultiByteString += 0x10;

                MultiByteString[-0x10] = TranslateTable[UnicodeString[-0x10]];
            case 0x0F:
                MultiByteString[-0x0F] = TranslateTable[UnicodeString[-0x0F]];
            case 0x0E:
                MultiByteString[-0x0E] = TranslateTable[UnicodeString[-0x0E]];
            case 0x0D:
                MultiByteString[-0x0D] = TranslateTable[UnicodeString[-0x0D]];
            case 0x0C:
                MultiByteString[-0x0C] = TranslateTable[UnicodeString[-0x0C]];
            case 0x0B:
                MultiByteString[-0x0B] = TranslateTable[UnicodeString[-0x0B]];
            case 0x0A:
                MultiByteString[-0x0A] = TranslateTable[UnicodeString[-0x0A]];
            case 0x09:
                MultiByteString[-0x09] = TranslateTable[UnicodeString[-0x09]];
            case 0x08:
                MultiByteString[-0x08] = TranslateTable[UnicodeString[-0x08]];
            case 0x07:
                MultiByteString[-0x07] = TranslateTable[UnicodeString[-0x07]];
            case 0x06:
                MultiByteString[-0x06] = TranslateTable[UnicodeString[-0x06]];
            case 0x05:
                MultiByteString[-0x05] = TranslateTable[UnicodeString[-0x05]];
            case 0x04:
                MultiByteString[-0x04] = TranslateTable[UnicodeString[-0x04]];
            case 0x03:
                MultiByteString[-0x03] = TranslateTable[UnicodeString[-0x03]];
            case 0x02:
                MultiByteString[-0x02] = TranslateTable[UnicodeString[-0x02]];
            case 0x01:
                MultiByteString[-0x01] = TranslateTable[UnicodeString[-0x01]];
            case 0x00:
                ;
            }

            LoopCount -= TmpCount;
            TmpCount = 0x10;
        } while ( LoopCount > 0 );

        /* end of copy... */
    } else {
        USHORT MbChar;
        PCH MultiByteStringAnchor = MultiByteString;

        CharsInUnicodeString = BytesInUnicodeString / sizeof(WCHAR);

        while ( CharsInUnicodeString && MaxBytesInMultiByteString ) {

            MbChar = NlsUnicodeToMbAnsiData[ *UnicodeString++ ];
            if (HIBYTE(MbChar) != 0) {
                //
                // Need at least 2 bytes to copy a double byte char.
                // Don't want to truncate in the middle of a DBCS char.
                //
                if (MaxBytesInMultiByteString-- < 2) {
                    break;
                }
                *MultiByteString++ = HIBYTE(MbChar);  // lead byte
            }
            *MultiByteString++ = LOBYTE(MbChar);
            MaxBytesInMultiByteString--;

            CharsInUnicodeString--;
        }

        if (ARGUMENT_PRESENT(BytesInMultiByteString))
            *BytesInMultiByteString = (ULONG)(MultiByteString - MultiByteStringAnchor);
    }

    return STATUS_SUCCESS;
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
    PCH TranslateTable;
    ULONG CharsInUnicodeString;
    UCHAR SbChar;
    WCHAR UnicodeChar;

    RTL_PAGED_CODE();

    //
    // Convert Unicode byte count to character count. Byte count of
    // multibyte string is equivalent to character count.
    //
    if (!NlsMbCodePageTag) {
        CharsInUnicodeString = BytesInUnicodeString / sizeof(WCHAR);

        LoopCount = (CharsInUnicodeString < MaxBytesInMultiByteString) ?
                     CharsInUnicodeString : MaxBytesInMultiByteString;

        if (ARGUMENT_PRESENT(BytesInMultiByteString))
            *BytesInMultiByteString = LoopCount;

        TranslateTable = NlsUnicodeToAnsiData;  // used to help the mips compiler

        TmpCount = LoopCount & 0x0F;
        UnicodeString += TmpCount;
        MultiByteString += TmpCount;

        do
        {
            //
            // Convert to ANSI and back to Unicode before upper casing
            // to ensure the visual best fits are converted and
            // upper cased properly.
            //
            switch( TmpCount ) {
            default:
                UnicodeString += 0x10;
                MultiByteString += 0x10;

                SbChar = TranslateTable[UnicodeString[-0x10]];
                UnicodeChar = NlsAnsiToUnicodeData[SbChar];
                UnicodeChar = (WCHAR)NLS_UPCASE(UnicodeChar);
                MultiByteString[-0x10] = TranslateTable[UnicodeChar];
            case 0x0F:
                SbChar = TranslateTable[UnicodeString[-0x0F]];
                UnicodeChar = NlsAnsiToUnicodeData[SbChar];
                UnicodeChar = (WCHAR)NLS_UPCASE(UnicodeChar);
                MultiByteString[-0x0F] = TranslateTable[UnicodeChar];
            case 0x0E:
                SbChar = TranslateTable[UnicodeString[-0x0E]];
                UnicodeChar = NlsAnsiToUnicodeData[SbChar];
                UnicodeChar = (WCHAR)NLS_UPCASE(UnicodeChar);
                MultiByteString[-0x0E] = TranslateTable[UnicodeChar];
            case 0x0D:
                SbChar = TranslateTable[UnicodeString[-0x0D]];
                UnicodeChar = NlsAnsiToUnicodeData[SbChar];
                UnicodeChar = (WCHAR)NLS_UPCASE(UnicodeChar);
                MultiByteString[-0x0D] = TranslateTable[UnicodeChar];
            case 0x0C:
                SbChar = TranslateTable[UnicodeString[-0x0C]];
                UnicodeChar = NlsAnsiToUnicodeData[SbChar];
                UnicodeChar = (WCHAR)NLS_UPCASE(UnicodeChar);
                MultiByteString[-0x0C] = TranslateTable[UnicodeChar];
            case 0x0B:
                SbChar = TranslateTable[UnicodeString[-0x0B]];
                UnicodeChar = NlsAnsiToUnicodeData[SbChar];
                UnicodeChar = (WCHAR)NLS_UPCASE(UnicodeChar);
                MultiByteString[-0x0B] = TranslateTable[UnicodeChar];
            case 0x0A:
                SbChar = TranslateTable[UnicodeString[-0x0A]];
                UnicodeChar = NlsAnsiToUnicodeData[SbChar];
                UnicodeChar = (WCHAR)NLS_UPCASE(UnicodeChar);
                MultiByteString[-0x0A] = TranslateTable[UnicodeChar];
            case 0x09:
                SbChar = TranslateTable[UnicodeString[-0x09]];
                UnicodeChar = NlsAnsiToUnicodeData[SbChar];
                UnicodeChar = (WCHAR)NLS_UPCASE(UnicodeChar);
                MultiByteString[-0x09] = TranslateTable[UnicodeChar];
            case 0x08:
                SbChar = TranslateTable[UnicodeString[-0x08]];
                UnicodeChar = NlsAnsiToUnicodeData[SbChar];
                UnicodeChar = (WCHAR)NLS_UPCASE(UnicodeChar);
                MultiByteString[-0x08] = TranslateTable[UnicodeChar];
            case 0x07:
                SbChar = TranslateTable[UnicodeString[-0x07]];
                UnicodeChar = NlsAnsiToUnicodeData[SbChar];
                UnicodeChar = (WCHAR)NLS_UPCASE(UnicodeChar);
                MultiByteString[-0x07] = TranslateTable[UnicodeChar];
            case 0x06:
                SbChar = TranslateTable[UnicodeString[-0x06]];
                UnicodeChar = NlsAnsiToUnicodeData[SbChar];
                UnicodeChar = (WCHAR)NLS_UPCASE(UnicodeChar);
                MultiByteString[-0x06] = TranslateTable[UnicodeChar];
            case 0x05:
                SbChar = TranslateTable[UnicodeString[-0x05]];
                UnicodeChar = NlsAnsiToUnicodeData[SbChar];
                UnicodeChar = (WCHAR)NLS_UPCASE(UnicodeChar);
                MultiByteString[-0x05] = TranslateTable[UnicodeChar];
            case 0x04:
                SbChar = TranslateTable[UnicodeString[-0x04]];
                UnicodeChar = NlsAnsiToUnicodeData[SbChar];
                UnicodeChar = (WCHAR)NLS_UPCASE(UnicodeChar);
                MultiByteString[-0x04] = TranslateTable[UnicodeChar];
            case 0x03:
                SbChar = TranslateTable[UnicodeString[-0x03]];
                UnicodeChar = NlsAnsiToUnicodeData[SbChar];
                UnicodeChar = (WCHAR)NLS_UPCASE(UnicodeChar);
                MultiByteString[-0x03] = TranslateTable[UnicodeChar];
            case 0x02:
                SbChar = TranslateTable[UnicodeString[-0x02]];
                UnicodeChar = NlsAnsiToUnicodeData[SbChar];
                UnicodeChar = (WCHAR)NLS_UPCASE(UnicodeChar);
                MultiByteString[-0x02] = TranslateTable[UnicodeChar];
            case 0x01:
                SbChar = TranslateTable[UnicodeString[-0x01]];
                UnicodeChar = NlsAnsiToUnicodeData[SbChar];
                UnicodeChar = (WCHAR)NLS_UPCASE(UnicodeChar);
                MultiByteString[-0x01] = TranslateTable[UnicodeChar];
            case 0x00:
                ;
            }

            LoopCount -= TmpCount;
            TmpCount = 0x10;
        } while ( LoopCount > 0 );

        /* end of copy... */
    } else {
        USHORT MbChar;
        register USHORT Entry;
        PCH MultiByteStringAnchor = MultiByteString;

        CharsInUnicodeString = BytesInUnicodeString / sizeof(WCHAR);

        while ( CharsInUnicodeString && MaxBytesInMultiByteString ) {
            //
            // Convert to ANSI and back to Unicode before upper casing
            // to ensure the visual best fits are converted and
            // upper cased properly.
            //
            MbChar = NlsUnicodeToMbAnsiData[ *UnicodeString++ ];
            if ( NlsLeadByteInfo[HIBYTE(MbChar)] ) {
                //
                // Lead byte - translate the trail byte using the table
                // that corresponds to this lead byte.
                //
                Entry = NlsLeadByteInfo[HIBYTE(MbChar)];
                UnicodeChar = (WCHAR)NlsMbAnsiCodePageTables[ Entry + LOBYTE(MbChar) ];
            } else {
                //
                // Single byte character.
                //
                UnicodeChar = NlsAnsiToUnicodeData[LOBYTE(MbChar)];
            }
            UnicodeChar = (WCHAR)NLS_UPCASE(UnicodeChar);
            MbChar = NlsUnicodeToMbAnsiData[UnicodeChar];

            if (HIBYTE(MbChar) != 0) {
                //
                // Need at least 2 bytes to copy a double byte char.
                // Don't want to truncate in the middle of a DBCS char.
                //
                if (MaxBytesInMultiByteString-- < 2) {
                    break;
                }
                *MultiByteString++ = HIBYTE(MbChar);  // lead byte
            }
            *MultiByteString++ = LOBYTE(MbChar);
            MaxBytesInMultiByteString--;

            CharsInUnicodeString--;
        }

        if (ARGUMENT_PRESENT(BytesInMultiByteString))
            *BytesInMultiByteString = (ULONG)(MultiByteString - MultiByteStringAnchor);
    }

    return STATUS_SUCCESS;
}


NTSTATUS
RtlUnicodeToOemN(
    OUT PCH OemString,
    IN ULONG MaxBytesInOemString,
    OUT PULONG BytesInOemString OPTIONAL,
    IN PWCH UnicodeString,
    IN ULONG BytesInUnicodeString)

/*++

Routine Description:

    This functions converts the specified unicode source string into an
    oem string. The translation is done with respect to the OEM Code
    Page (OCP) loaded at boot time.

Arguments:

    OemString - Returns an oem string that is equivalent to the
        unicode source string.  If the translation can not be done,
        an error is returned.

    MaxBytesInOemString - Supplies the maximum number of bytes to be
        written to OemString.  If this causes OemString to be a
        truncated equivalent of UnicodeString, no error condition results.

    BytesInOemString - Returns the number of bytes in the returned
        oem string pointed to by OemString.

    UnicodeString - Supplies the unicode source string that is to be
        converted to oem.

    BytesInUnicodeString - The number of bytes in the the string pointed to by
        UnicodeString.

Return Value:

    SUCCESS - The conversion was successful

    STATUS_BUFFER_OVERFLOW - MaxBytesInUnicodeString was not enough to hold
        the whole Oem string.  It was converted correct to the point though.

--*/

{
    ULONG TmpCount;
    ULONG LoopCount;
    PCH TranslateTable;
    ULONG CharsInUnicodeString;

    RTL_PAGED_CODE();

    //
    // Convert Unicode byte count to character count. Byte count of
    // multibyte string is equivalent to character count.
    //
    if (!NlsMbOemCodePageTag) {
        CharsInUnicodeString = BytesInUnicodeString / sizeof(WCHAR);

        LoopCount = (CharsInUnicodeString < MaxBytesInOemString) ?
                     CharsInUnicodeString : MaxBytesInOemString;

        if (ARGUMENT_PRESENT(BytesInOemString))
            *BytesInOemString = LoopCount;

        TranslateTable = NlsUnicodeToOemData;  // used to help the mips compiler

        TmpCount = LoopCount & 0x0F;
        UnicodeString += TmpCount;
        OemString += TmpCount;

        do
        {
            switch( TmpCount ) {
            default:
                UnicodeString += 0x10;
                OemString += 0x10;

                OemString[-0x10] = TranslateTable[UnicodeString[-0x10]];
            case 0x0F:
                OemString[-0x0F] = TranslateTable[UnicodeString[-0x0F]];
            case 0x0E:
                OemString[-0x0E] = TranslateTable[UnicodeString[-0x0E]];
            case 0x0D:
                OemString[-0x0D] = TranslateTable[UnicodeString[-0x0D]];
            case 0x0C:
                OemString[-0x0C] = TranslateTable[UnicodeString[-0x0C]];
            case 0x0B:
                OemString[-0x0B] = TranslateTable[UnicodeString[-0x0B]];
            case 0x0A:
                OemString[-0x0A] = TranslateTable[UnicodeString[-0x0A]];
            case 0x09:
                OemString[-0x09] = TranslateTable[UnicodeString[-0x09]];
            case 0x08:
                OemString[-0x08] = TranslateTable[UnicodeString[-0x08]];
            case 0x07:
                OemString[-0x07] = TranslateTable[UnicodeString[-0x07]];
            case 0x06:
                OemString[-0x06] = TranslateTable[UnicodeString[-0x06]];
            case 0x05:
                OemString[-0x05] = TranslateTable[UnicodeString[-0x05]];
            case 0x04:
                OemString[-0x04] = TranslateTable[UnicodeString[-0x04]];
            case 0x03:
                OemString[-0x03] = TranslateTable[UnicodeString[-0x03]];
            case 0x02:
                OemString[-0x02] = TranslateTable[UnicodeString[-0x02]];
            case 0x01:
                OemString[-0x01] = TranslateTable[UnicodeString[-0x01]];
            case 0x00:
                ;
            }

            LoopCount -= TmpCount;
            TmpCount = 0x10;
        } while ( LoopCount > 0 );

        /* end of copy... */
    } else {
        register USHORT MbChar;
        PCH OemStringAnchor = OemString;

        CharsInUnicodeString = BytesInUnicodeString / sizeof(WCHAR);

        while ( CharsInUnicodeString && MaxBytesInOemString ) {

            MbChar = NlsUnicodeToMbOemData[ *UnicodeString++ ];
            if (HIBYTE(MbChar) != 0) {
                //
                // Need at least 2 bytes to copy a double byte char.
                // Don't want to truncate in the middle of a DBCS char.
                //
                if (MaxBytesInOemString-- < 2) {
                    break;
                }
                *OemString++ = HIBYTE(MbChar);  // lead byte
            }
            *OemString++ = LOBYTE(MbChar);
            MaxBytesInOemString--;

            CharsInUnicodeString--;
        }

        if (ARGUMENT_PRESENT(BytesInOemString))
            *BytesInOemString = (ULONG)(OemString - OemStringAnchor);
    }

    //
    //  Check if we were able to use all of the source Unicode String
    //
    return ( CharsInUnicodeString <= MaxBytesInOemString ) ?
           STATUS_SUCCESS :
           STATUS_BUFFER_OVERFLOW;
}


NTSTATUS
RtlUpcaseUnicodeToOemN(
    OUT PCH OemString,
    IN ULONG MaxBytesInOemString,
    OUT PULONG BytesInOemString OPTIONAL,
    IN PWCH UnicodeString,
    IN ULONG BytesInUnicodeString)

/*++

Routine Description:

    This functions upper cases the specified unicode source string and
    converts it into an oem string. The translation is done with respect
    to the OEM Code Page (OCP) loaded at boot time.

Arguments:

    OemString - Returns an oem string that is equivalent to the upper
        case of the unicode source string.  If the translation can not
        be done, an error is returned.

    MaxBytesInOemString - Supplies the maximum number of bytes to be
        written to OemString.  If this causes OemString to be a
        truncated equivalent of UnicodeString, no error condition results.

    BytesInOemString - Returns the number of bytes in the returned
        oem string pointed to by OemString.

    UnicodeString - Supplies the unicode source string that is to be
        converted to oem.

    BytesInUnicodeString - The number of bytes in the the string pointed
        to by UnicodeString.

Return Value:

    SUCCESS - The conversion was successful

    STATUS_BUFFER_OVERFLOW - MaxBytesInUnicodeString was not enough to
        hold the whole Oem string.  It was converted correctly to that
        point, though.

--*/

{
    ULONG TmpCount;
    ULONG LoopCount;
    PCH TranslateTable;
    ULONG CharsInUnicodeString;
    UCHAR SbChar;
    WCHAR UnicodeChar;

    RTL_PAGED_CODE();

    //
    // Convert Unicode byte count to character count. Byte count of
    // multibyte string is equivalent to character count.
    //
    if (!NlsMbOemCodePageTag) {
        CharsInUnicodeString = BytesInUnicodeString / sizeof(WCHAR);

        LoopCount = (CharsInUnicodeString < MaxBytesInOemString) ?
                     CharsInUnicodeString : MaxBytesInOemString;

        if (ARGUMENT_PRESENT(BytesInOemString))
            *BytesInOemString = LoopCount;

        TranslateTable = NlsUnicodeToOemData;  // used to help the mips compiler

        TmpCount = LoopCount & 0x0F;
        UnicodeString += TmpCount;
        OemString += TmpCount;

        do
        {
            //
            // Convert to OEM and back to Unicode before upper casing
            // to ensure the visual best fits are converted and
            // upper cased properly.
            //
            switch( TmpCount ) {
            default:
                UnicodeString += 0x10;
                OemString += 0x10;

                SbChar = TranslateTable[UnicodeString[-0x10]];
                UnicodeChar = NlsOemToUnicodeData[SbChar];
                UnicodeChar = (WCHAR)NLS_UPCASE(UnicodeChar);
                OemString[-0x10] = TranslateTable[UnicodeChar];
            case 0x0F:
                SbChar = TranslateTable[UnicodeString[-0x0F]];
                UnicodeChar = NlsOemToUnicodeData[SbChar];
                UnicodeChar = (WCHAR)NLS_UPCASE(UnicodeChar);
                OemString[-0x0F] = TranslateTable[UnicodeChar];
            case 0x0E:
                SbChar = TranslateTable[UnicodeString[-0x0E]];
                UnicodeChar = NlsOemToUnicodeData[SbChar];
                UnicodeChar = (WCHAR)NLS_UPCASE(UnicodeChar);
                OemString[-0x0E] = TranslateTable[UnicodeChar];
            case 0x0D:
                SbChar = TranslateTable[UnicodeString[-0x0D]];
                UnicodeChar = NlsOemToUnicodeData[SbChar];
                UnicodeChar = (WCHAR)NLS_UPCASE(UnicodeChar);
                OemString[-0x0D] = TranslateTable[UnicodeChar];
            case 0x0C:
                SbChar = TranslateTable[UnicodeString[-0x0C]];
                UnicodeChar = NlsOemToUnicodeData[SbChar];
                UnicodeChar = (WCHAR)NLS_UPCASE(UnicodeChar);
                OemString[-0x0C] = TranslateTable[UnicodeChar];
            case 0x0B:
                SbChar = TranslateTable[UnicodeString[-0x0B]];
                UnicodeChar = NlsOemToUnicodeData[SbChar];
                UnicodeChar = (WCHAR)NLS_UPCASE(UnicodeChar);
                OemString[-0x0B] = TranslateTable[UnicodeChar];
            case 0x0A:
                SbChar = TranslateTable[UnicodeString[-0x0A]];
                UnicodeChar = NlsOemToUnicodeData[SbChar];
                UnicodeChar = (WCHAR)NLS_UPCASE(UnicodeChar);
                OemString[-0x0A] = TranslateTable[UnicodeChar];
            case 0x09:
                SbChar = TranslateTable[UnicodeString[-0x09]];
                UnicodeChar = NlsOemToUnicodeData[SbChar];
                UnicodeChar = (WCHAR)NLS_UPCASE(UnicodeChar);
                OemString[-0x09] = TranslateTable[UnicodeChar];
            case 0x08:
                SbChar = TranslateTable[UnicodeString[-0x08]];
                UnicodeChar = NlsOemToUnicodeData[SbChar];
                UnicodeChar = (WCHAR)NLS_UPCASE(UnicodeChar);
                OemString[-0x08] = TranslateTable[UnicodeChar];
            case 0x07:
                SbChar = TranslateTable[UnicodeString[-0x07]];
                UnicodeChar = NlsOemToUnicodeData[SbChar];
                UnicodeChar = (WCHAR)NLS_UPCASE(UnicodeChar);
                OemString[-0x07] = TranslateTable[UnicodeChar];
            case 0x06:
                SbChar = TranslateTable[UnicodeString[-0x06]];
                UnicodeChar = NlsOemToUnicodeData[SbChar];
                UnicodeChar = (WCHAR)NLS_UPCASE(UnicodeChar);
                OemString[-0x06] = TranslateTable[UnicodeChar];
            case 0x05:
                SbChar = TranslateTable[UnicodeString[-0x05]];
                UnicodeChar = NlsOemToUnicodeData[SbChar];
                UnicodeChar = (WCHAR)NLS_UPCASE(UnicodeChar);
                OemString[-0x05] = TranslateTable[UnicodeChar];
            case 0x04:
                SbChar = TranslateTable[UnicodeString[-0x04]];
                UnicodeChar = NlsOemToUnicodeData[SbChar];
                UnicodeChar = (WCHAR)NLS_UPCASE(UnicodeChar);
                OemString[-0x04] = TranslateTable[UnicodeChar];
            case 0x03:
                SbChar = TranslateTable[UnicodeString[-0x03]];
                UnicodeChar = NlsOemToUnicodeData[SbChar];
                UnicodeChar = (WCHAR)NLS_UPCASE(UnicodeChar);
                OemString[-0x03] = TranslateTable[UnicodeChar];
            case 0x02:
                SbChar = TranslateTable[UnicodeString[-0x02]];
                UnicodeChar = NlsOemToUnicodeData[SbChar];
                UnicodeChar = (WCHAR)NLS_UPCASE(UnicodeChar);
                OemString[-0x02] = TranslateTable[UnicodeChar];
            case 0x01:
                SbChar = TranslateTable[UnicodeString[-0x01]];
                UnicodeChar = NlsOemToUnicodeData[SbChar];
                UnicodeChar = (WCHAR)NLS_UPCASE(UnicodeChar);
                OemString[-0x01] = TranslateTable[UnicodeChar];
            case 0x00:
                ;
            }

            LoopCount -= TmpCount;
            TmpCount = 0x10;
        } while ( LoopCount > 0 );

        /* end of copy... */
    } else {
        USHORT MbChar;
        register USHORT Entry;
        PCH OemStringAnchor = OemString;

        CharsInUnicodeString = BytesInUnicodeString / sizeof(WCHAR);

        while ( CharsInUnicodeString && MaxBytesInOemString ) {
            //
            // Convert to OEM and back to Unicode before upper casing
            // to ensure the visual best fits are converted and
            // upper cased properly.
            //
            MbChar = NlsUnicodeToMbOemData[ *UnicodeString++ ];
            if (NlsOemLeadByteInfo[HIBYTE(MbChar)]) {
                //
                // Lead byte - translate the trail byte using the table
                // that corresponds to this lead byte.
                //
                Entry = NlsOemLeadByteInfo[HIBYTE(MbChar)];
                UnicodeChar = (WCHAR)NlsMbOemCodePageTables[ Entry + LOBYTE(MbChar) ];
            } else {
                //
                // Single byte character.
                //
                UnicodeChar = NlsOemToUnicodeData[LOBYTE(MbChar)];
            }
            UnicodeChar = (WCHAR)NLS_UPCASE(UnicodeChar);
            MbChar = NlsUnicodeToMbOemData[UnicodeChar];

            if (HIBYTE(MbChar) != 0) {
                //
                // Need at least 2 bytes to copy a double byte char.
                // Don't want to truncate in the middle of a DBCS char.
                //
                if (MaxBytesInOemString-- < 2) {
                    break;
                }
                *OemString++ = HIBYTE(MbChar);  // lead byte
            }
            *OemString++ = LOBYTE(MbChar);
            MaxBytesInOemString--;

            CharsInUnicodeString--;
        }

        if (ARGUMENT_PRESENT(BytesInOemString))
            *BytesInOemString = (ULONG)(OemString - OemStringAnchor);
    }

    //
    //  Check if we were able to use all of the source Unicode String
    //
    return ( CharsInUnicodeString <= MaxBytesInOemString ) ?
           STATUS_SUCCESS :
           STATUS_BUFFER_OVERFLOW;
}

BOOLEAN
RtlpDidUnicodeToOemWork(
    IN POEM_STRING OemString,
    IN PUNICODE_STRING UnicodeString
    )

/*++

Routine Description:

    This function looks for the default character in the Oem string, making
    sure it was not a correct translation from the Unicode source string.

    This allows us to test whether or not a translation was really successful.

Arguments:

    OemString - The result of conversion from the unicode string.

    UnicodeString - The source of the Oem string.

Return Value:

    TRUE if the Unicode to Oem translation caused no default characters to be
        inserted.  FALSE otherwise.

--*/

{
    ULONG OemOffset;
    BOOLEAN Result = TRUE;

    RTL_PAGED_CODE();

    if (!NlsMbOemCodePageTag) {

        for (OemOffset = 0;
             OemOffset < OemString->Length;
             OemOffset += 1) {

            if ((OemString->Buffer[OemOffset] == (UCHAR)OemDefaultChar) &&
                (UnicodeString->Buffer[OemOffset] != OemTransUniDefaultChar)) {

                Result = FALSE;
                break;
            }
        }

    } else {

        ULONG UnicodeOffset;

        for (OemOffset = 0, UnicodeOffset = 0;
             OemOffset < OemString->Length;
             OemOffset += 1, UnicodeOffset += 1) {

            //
            //  If we landed on a DBCS character handle it accordingly
            //

            if (NlsOemLeadByteInfo[(UCHAR)OemString->Buffer[OemOffset]]) {

                USHORT DbcsChar;

                ASSERT( OemOffset + 1 < OemString->Length );

                DbcsChar = (OemString->Buffer[OemOffset] << 8) + (UCHAR)OemString->Buffer[OemOffset+1];
                OemOffset++;

                if ((DbcsChar == OemDefaultChar) &&
                    (UnicodeString->Buffer[UnicodeOffset] != OemTransUniDefaultChar)) {

                    Result = FALSE;
                    break;
                }

                continue;
            }

            if ((OemString->Buffer[OemOffset] == (UCHAR)OemDefaultChar) &&
                (UnicodeString->Buffer[UnicodeOffset] != OemTransUniDefaultChar)) {

                Result = FALSE;
                break;
            }
        }
    }

    return Result;
}


NTSTATUS
RtlCustomCPToUnicodeN(
    IN PCPTABLEINFO CustomCP,
    OUT PWCH UnicodeString,
    IN ULONG MaxBytesInUnicodeString,
    OUT PULONG BytesInUnicodeString OPTIONAL,
    IN PCH CustomCPString,
    IN ULONG BytesInCustomCPString)

/*++

Routine Description:

    This functions converts the specified CustomCP source string into a
    Unicode string. The translation is done with respect to the
    CustomCP Code Page specified.  Single byte characters
    in the range 0x00 - 0x7f are simply zero extended as a performance
    enhancement.  In some far eastern code pages 0x5c is defined as the
    Yen sign.  For system translation we always want to consider 0x5c
    to be the backslash character.  We get this for free by zero extending.

    NOTE: This routine only supports precomposed Unicode characters.

Arguments:

    CustomCP - Supplies the address of the code page that translations
        are done relative to

    UnicodeString - Returns a unicode string that is equivalent to
        the CustomCP source string.

    MaxBytesInUnicodeString - Supplies the maximum number of bytes to be
        written to UnicodeString.  If this causes UnicodeString to be a
        truncated equivalent of CustomCPString, no error condition results.

    BytesInUnicodeString - Returns the number of bytes in the returned
        unicode string pointed to by UnicodeString.

    CustomCPString - Supplies the CustomCP source string that is to be
        converted to unicode.

    BytesInCustomCPString - The number of bytes in the string pointed to
        by CustomCPString.

Return Value:

    SUCCESS - The conversion was successful

    STATUS_ILLEGAL_CHARACTER - The final CustomCP character was illegal

    STATUS_BUFFER_OVERFLOW - MaxBytesInUnicodeString was not enough to hold
        the whole CustomCP string.  It was converted correct to the point though.

--*/

{
    ULONG LoopCount;
    PUSHORT TranslateTable;
    ULONG MaxCharsInUnicodeString;

    RTL_PAGED_CODE();

    if (!(CustomCP->DBCSCodePage)) {
        //
        // The Custom CP is a single byte code page.
        //
        MaxCharsInUnicodeString = MaxBytesInUnicodeString / sizeof(WCHAR);

        LoopCount = (MaxCharsInUnicodeString < BytesInCustomCPString) ?
                     MaxCharsInUnicodeString : BytesInCustomCPString;

        if (ARGUMENT_PRESENT(BytesInUnicodeString))
            *BytesInUnicodeString = LoopCount * sizeof(WCHAR);


        TranslateTable = CustomCP->MultiByteTable;

        quick_copy:
            switch( LoopCount ) {
            default:
                UnicodeString[0x0F] = TranslateTable[(UCHAR)CustomCPString[0x0F]];
            case 0x0F:
                UnicodeString[0x0E] = TranslateTable[(UCHAR)CustomCPString[0x0E]];
            case 0x0E:
                UnicodeString[0x0D] = TranslateTable[(UCHAR)CustomCPString[0x0D]];
            case 0x0D:
                UnicodeString[0x0C] = TranslateTable[(UCHAR)CustomCPString[0x0C]];
            case 0x0C:
                UnicodeString[0x0B] = TranslateTable[(UCHAR)CustomCPString[0x0B]];
            case 0x0B:
                UnicodeString[0x0A] = TranslateTable[(UCHAR)CustomCPString[0x0A]];
            case 0x0A:
                UnicodeString[0x09] = TranslateTable[(UCHAR)CustomCPString[0x09]];
            case 0x09:
                UnicodeString[0x08] = TranslateTable[(UCHAR)CustomCPString[0x08]];
            case 0x08:
                UnicodeString[0x07] = TranslateTable[(UCHAR)CustomCPString[0x07]];
            case 0x07:
                UnicodeString[0x06] = TranslateTable[(UCHAR)CustomCPString[0x06]];
            case 0x06:
                UnicodeString[0x05] = TranslateTable[(UCHAR)CustomCPString[0x05]];
            case 0x05:
                UnicodeString[0x04] = TranslateTable[(UCHAR)CustomCPString[0x04]];
            case 0x04:
                UnicodeString[0x03] = TranslateTable[(UCHAR)CustomCPString[0x03]];
            case 0x03:
                UnicodeString[0x02] = TranslateTable[(UCHAR)CustomCPString[0x02]];
            case 0x02:
                UnicodeString[0x01] = TranslateTable[(UCHAR)CustomCPString[0x01]];
            case 0x01:
                UnicodeString[0x00] = TranslateTable[(UCHAR)CustomCPString[0x00]];
            case 0x00:
                ;
            }

            if ( LoopCount > 0x10 ) {
                LoopCount -= 0x10;
                CustomCPString += 0x10;
                UnicodeString += 0x10;

                goto  quick_copy;
            }
        /* end of copy... */
    } else {
        register USHORT Entry;
        PWCH UnicodeStringAnchor = UnicodeString;
        PUSHORT NlsCustomLeadByteInfo = CustomCP->DBCSOffsets;

        //
        // The CP is a multibyte code page.  Check each character
        // to see if it is a lead byte before doing the translation.
        //
        MaxCharsInUnicodeString = MaxBytesInUnicodeString / sizeof(WCHAR);
        TranslateTable = (PUSHORT)(CustomCP->DBCSOffsets);

        while (MaxCharsInUnicodeString && BytesInCustomCPString) {
            MaxCharsInUnicodeString--;
            BytesInCustomCPString--;
            if (NlsCustomLeadByteInfo[*(PUCHAR)CustomCPString]) {
                //
                // Lead byte - Make sure there is a trail byte.  If not,
                // pass back a space rather than an error.  Some 3.x
                // applications pass incorrect strings and don't expect
                // to get an error.
                //
                if (BytesInCustomCPString == 0)
                {
                    *UnicodeString++ = UnicodeNull;
                    break;
                }

                //
                // Get the unicode character.
                //
                Entry = NlsCustomLeadByteInfo[*(PUCHAR)CustomCPString++];
                *UnicodeString = TranslateTable[ Entry + *(PUCHAR)CustomCPString++ ];
                UnicodeString++;

                //
                // Decrement count of bytes in multibyte string to account
                // for the double byte character.
                //
                BytesInCustomCPString--;
            } else {
                //
                // Single byte character.
                //
                *UnicodeString++ = (CustomCP->MultiByteTable)[*(PUCHAR)CustomCPString++];
            }
        }

        if (ARGUMENT_PRESENT(BytesInUnicodeString))
            *BytesInUnicodeString = (ULONG)((PCH)UnicodeString - (PCH)UnicodeStringAnchor);
    }

    //
    //  Check if we were able to use all of the source CustomCP String
    //
    return ( BytesInCustomCPString <= MaxCharsInUnicodeString ) ?
           STATUS_SUCCESS :
           STATUS_BUFFER_OVERFLOW;
}


NTSTATUS
RtlUnicodeToCustomCPN(
    IN PCPTABLEINFO CustomCP,
    OUT PCH CustomCPString,
    IN ULONG MaxBytesInCustomCPString,
    OUT PULONG BytesInCustomCPString OPTIONAL,
    IN PWCH UnicodeString,
    IN ULONG BytesInUnicodeString)

/*++

Routine Description:

    This functions converts the specified unicode source string into an
    CustomCP string.  The translation is done with respect to the
    CustomCP Code Page specified by CustomCp.

Arguments:

    CustomCP - Supplies the address of the code page that translations
        are done relative to

    CustomCPString - Returns an CustomCP string that is equivalent to the
        unicode source string.  If the translation can not be done,
        an error is returned.

    MaxBytesInCustomCPString - Supplies the maximum number of bytes to be
        written to CustomCPString.  If this causes CustomCPString to be a
        truncated equivalent of UnicodeString, no error condition results.

    BytesInCustomCPString - Returns the number of bytes in the returned
        CustomCP string pointed to by CustomCPString.

    UnicodeString - Supplies the unicode source string that is to be
        converted to CustomCP.

    BytesInUnicodeString - The number of bytes in the the string pointed to by
        UnicodeString.

Return Value:

    SUCCESS - The conversion was successful

    STATUS_BUFFER_OVERFLOW - MaxBytesInUnicodeString was not enough to hold
        the whole CustomCP string.  It was converted correct to the point though.

--*/

{
    ULONG TmpCount;
    ULONG LoopCount;
    PCH TranslateTable;
    PUSHORT WideTranslateTable;
    ULONG CharsInUnicodeString;

    RTL_PAGED_CODE();

    //
    // Convert Unicode byte count to character count. Byte count of
    // multibyte string is equivalent to character count.
    //
    if (!(CustomCP->DBCSCodePage)) {
        CharsInUnicodeString = BytesInUnicodeString / sizeof(WCHAR);

        LoopCount = (CharsInUnicodeString < MaxBytesInCustomCPString) ?
                     CharsInUnicodeString : MaxBytesInCustomCPString;

        if (ARGUMENT_PRESENT(BytesInCustomCPString))
            *BytesInCustomCPString = LoopCount;

        TranslateTable = CustomCP->WideCharTable;

        TmpCount = LoopCount & 0x0F;
        UnicodeString += TmpCount;
        CustomCPString += TmpCount;

        do
        {
            switch( TmpCount ) {
            default:
                UnicodeString += 0x10;
                CustomCPString += 0x10;

                CustomCPString[-0x10] = TranslateTable[UnicodeString[-0x10]];
            case 0x0F:
                CustomCPString[-0x0F] = TranslateTable[UnicodeString[-0x0F]];
            case 0x0E:
                CustomCPString[-0x0E] = TranslateTable[UnicodeString[-0x0E]];
            case 0x0D:
                CustomCPString[-0x0D] = TranslateTable[UnicodeString[-0x0D]];
            case 0x0C:
                CustomCPString[-0x0C] = TranslateTable[UnicodeString[-0x0C]];
            case 0x0B:
                CustomCPString[-0x0B] = TranslateTable[UnicodeString[-0x0B]];
            case 0x0A:
                CustomCPString[-0x0A] = TranslateTable[UnicodeString[-0x0A]];
            case 0x09:
                CustomCPString[-0x09] = TranslateTable[UnicodeString[-0x09]];
            case 0x08:
                CustomCPString[-0x08] = TranslateTable[UnicodeString[-0x08]];
            case 0x07:
                CustomCPString[-0x07] = TranslateTable[UnicodeString[-0x07]];
            case 0x06:
                CustomCPString[-0x06] = TranslateTable[UnicodeString[-0x06]];
            case 0x05:
                CustomCPString[-0x05] = TranslateTable[UnicodeString[-0x05]];
            case 0x04:
                CustomCPString[-0x04] = TranslateTable[UnicodeString[-0x04]];
            case 0x03:
                CustomCPString[-0x03] = TranslateTable[UnicodeString[-0x03]];
            case 0x02:
                CustomCPString[-0x02] = TranslateTable[UnicodeString[-0x02]];
            case 0x01:
                CustomCPString[-0x01] = TranslateTable[UnicodeString[-0x01]];
            case 0x00:
                ;
            }

            LoopCount -= TmpCount;
            TmpCount = 0x10;
        } while ( LoopCount > 0 );

        /* end of copy... */
    } else {
        USHORT MbChar;
        PCH CustomCPStringAnchor = CustomCPString;

        WideTranslateTable = CustomCP->WideCharTable;

        CharsInUnicodeString = BytesInUnicodeString / sizeof(WCHAR);

        while (CharsInUnicodeString && MaxBytesInCustomCPString) {

            MbChar = WideTranslateTable[ *UnicodeString++ ];
            if (HIBYTE(MbChar) != 0) {
                //
                // Need at least 2 bytes to copy a double byte char.
                // Don't want to truncate in the middle of a DBCS char.
                //
                if (MaxBytesInCustomCPString-- < 2) {
                    break;
                }
                *CustomCPString++ = HIBYTE(MbChar);  // lead byte
            }
            *CustomCPString++ = LOBYTE(MbChar);
            MaxBytesInCustomCPString--;

            CharsInUnicodeString--;
        }

        if (ARGUMENT_PRESENT(BytesInCustomCPString))
            *BytesInCustomCPString = (ULONG)(CustomCPString - CustomCPStringAnchor);
    }

    //
    //  Check if we were able to use all of the source Unicode String
    //
    return ( CharsInUnicodeString <= MaxBytesInCustomCPString ) ?
           STATUS_SUCCESS :
           STATUS_BUFFER_OVERFLOW;
}


NTSTATUS
RtlUpcaseUnicodeToCustomCPN(
    IN PCPTABLEINFO CustomCP,
    OUT PCH CustomCPString,
    IN ULONG MaxBytesInCustomCPString,
    OUT PULONG BytesInCustomCPString OPTIONAL,
    IN PWCH UnicodeString,
    IN ULONG BytesInUnicodeString)

/*++

Routine Description:

    This functions upper cases the specified unicode source string and
    converts it into a CustomCP string.  The translation is done with
    respect to the CustomCP Code Page specified by CustomCp.

Arguments:

    CustomCP - Supplies the address of the code page that translations
        are done relative to

    CustomCPString - Returns an CustomCP string that is equivalent to the
        unicode source string.  If the translation can not be done,
        an error is returned.

    MaxBytesInCustomCPString - Supplies the maximum number of bytes to be
        written to CustomCPString.  If this causes CustomCPString to be a
        truncated equivalent of UnicodeString, no error condition results.

    BytesInCustomCPString - Returns the number of bytes in the returned
        CustomCP string pointed to by CustomCPString.

    UnicodeString - Supplies the unicode source string that is to be
        converted to CustomCP.

    BytesInUnicodeString - The number of bytes in the the string pointed
        to by UnicodeString.

Return Value:

    SUCCESS - The conversion was successful

    STATUS_BUFFER_OVERFLOW - MaxBytesInUnicodeString was not enough to
        hold the whole CustomCP string.  It was converted correctly to
        that point, though.

--*/

{
    ULONG TmpCount;
    ULONG LoopCount;
    PCH TranslateTable;
    PUSHORT WideTranslateTable;
    ULONG CharsInUnicodeString;
    UCHAR SbChar;
    WCHAR UnicodeChar;

    RTL_PAGED_CODE();

    //
    // Convert Unicode byte count to character count. Byte count of
    // multibyte string is equivalent to character count.
    //
    if (!(CustomCP->DBCSCodePage)) {
        CharsInUnicodeString = BytesInUnicodeString / sizeof(WCHAR);

        LoopCount = (CharsInUnicodeString < MaxBytesInCustomCPString) ?
                     CharsInUnicodeString : MaxBytesInCustomCPString;

        if (ARGUMENT_PRESENT(BytesInCustomCPString))
            *BytesInCustomCPString = LoopCount;

        TranslateTable = CustomCP->WideCharTable;

        TmpCount = LoopCount & 0x0F;
        UnicodeString += TmpCount;
        CustomCPString += TmpCount;

        do
        {
            //
            // Convert to Single Byte and back to Unicode before upper
            // casing to ensure the visual best fits are converted and
            // upper cased properly.
            //
            switch( TmpCount ) {
            default:
                UnicodeString += 0x10;
                CustomCPString += 0x10;

                SbChar = TranslateTable[UnicodeString[-0x10]];
                UnicodeChar = (CustomCP->MultiByteTable)[SbChar];
                UnicodeChar = (WCHAR)NLS_UPCASE(UnicodeChar);
                CustomCPString[-0x10] = TranslateTable[UnicodeChar];
            case 0x0F:
                SbChar = TranslateTable[UnicodeString[-0x0F]];
                UnicodeChar = (CustomCP->MultiByteTable)[SbChar];
                UnicodeChar = (WCHAR)NLS_UPCASE(UnicodeChar);
                CustomCPString[-0x0F] = TranslateTable[UnicodeChar];
            case 0x0E:
                SbChar = TranslateTable[UnicodeString[-0x0E]];
                UnicodeChar = (CustomCP->MultiByteTable)[SbChar];
                UnicodeChar = (WCHAR)NLS_UPCASE(UnicodeChar);
                CustomCPString[-0x0E] = TranslateTable[UnicodeChar];
            case 0x0D:
                SbChar = TranslateTable[UnicodeString[-0x0D]];
                UnicodeChar = (CustomCP->MultiByteTable)[SbChar];
                UnicodeChar = (WCHAR)NLS_UPCASE(UnicodeChar);
                CustomCPString[-0x0D] = TranslateTable[UnicodeChar];
            case 0x0C:
                SbChar = TranslateTable[UnicodeString[-0x0C]];
                UnicodeChar = (CustomCP->MultiByteTable)[SbChar];
                UnicodeChar = (WCHAR)NLS_UPCASE(UnicodeChar);
                CustomCPString[-0x0C] = TranslateTable[UnicodeChar];
            case 0x0B:
                SbChar = TranslateTable[UnicodeString[-0x0B]];
                UnicodeChar = (CustomCP->MultiByteTable)[SbChar];
                UnicodeChar = (WCHAR)NLS_UPCASE(UnicodeChar);
                CustomCPString[-0x0B] = TranslateTable[UnicodeChar];
            case 0x0A:
                SbChar = TranslateTable[UnicodeString[-0x0A]];
                UnicodeChar = (CustomCP->MultiByteTable)[SbChar];
                UnicodeChar = (WCHAR)NLS_UPCASE(UnicodeChar);
                CustomCPString[-0x0A] = TranslateTable[UnicodeChar];
            case 0x09:
                SbChar = TranslateTable[UnicodeString[-0x09]];
                UnicodeChar = (CustomCP->MultiByteTable)[SbChar];
                UnicodeChar = (WCHAR)NLS_UPCASE(UnicodeChar);
                CustomCPString[-0x09] = TranslateTable[UnicodeChar];
            case 0x08:
                SbChar = TranslateTable[UnicodeString[-0x08]];
                UnicodeChar = (CustomCP->MultiByteTable)[SbChar];
                UnicodeChar = (WCHAR)NLS_UPCASE(UnicodeChar);
                CustomCPString[-0x08] = TranslateTable[UnicodeChar];
            case 0x07:
                SbChar = TranslateTable[UnicodeString[-0x07]];
                UnicodeChar = (CustomCP->MultiByteTable)[SbChar];
                UnicodeChar = (WCHAR)NLS_UPCASE(UnicodeChar);
                CustomCPString[-0x07] = TranslateTable[UnicodeChar];
            case 0x06:
                SbChar = TranslateTable[UnicodeString[-0x06]];
                UnicodeChar = (CustomCP->MultiByteTable)[SbChar];
                UnicodeChar = (WCHAR)NLS_UPCASE(UnicodeChar);
                CustomCPString[-0x06] = TranslateTable[UnicodeChar];
            case 0x05:
                SbChar = TranslateTable[UnicodeString[-0x05]];
                UnicodeChar = (CustomCP->MultiByteTable)[SbChar];
                UnicodeChar = (WCHAR)NLS_UPCASE(UnicodeChar);
                CustomCPString[-0x05] = TranslateTable[UnicodeChar];
            case 0x04:
                SbChar = TranslateTable[UnicodeString[-0x04]];
                UnicodeChar = (CustomCP->MultiByteTable)[SbChar];
                UnicodeChar = (WCHAR)NLS_UPCASE(UnicodeChar);
                CustomCPString[-0x04] = TranslateTable[UnicodeChar];
            case 0x03:
                SbChar = TranslateTable[UnicodeString[-0x03]];
                UnicodeChar = (CustomCP->MultiByteTable)[SbChar];
                UnicodeChar = (WCHAR)NLS_UPCASE(UnicodeChar);
                CustomCPString[-0x03] = TranslateTable[UnicodeChar];
            case 0x02:
                SbChar = TranslateTable[UnicodeString[-0x02]];
                UnicodeChar = (CustomCP->MultiByteTable)[SbChar];
                UnicodeChar = (WCHAR)NLS_UPCASE(UnicodeChar);
                CustomCPString[-0x02] = TranslateTable[UnicodeChar];
            case 0x01:
                SbChar = TranslateTable[UnicodeString[-0x01]];
                UnicodeChar = (CustomCP->MultiByteTable)[SbChar];
                UnicodeChar = (WCHAR)NLS_UPCASE(UnicodeChar);
                CustomCPString[-0x01] = TranslateTable[UnicodeChar];
            case 0x00:
                ;
            }

            LoopCount -= TmpCount;
            TmpCount = 0x10;
        } while ( LoopCount > 0 );

        /* end of copy... */
    } else {
        USHORT MbChar;
        register USHORT Entry;
        PCH CustomCPStringAnchor = CustomCPString;
        PUSHORT NlsCustomLeadByteInfo = CustomCP->DBCSOffsets;

        WideTranslateTable = CustomCP->WideCharTable;

        CharsInUnicodeString = BytesInUnicodeString / sizeof(WCHAR);

        while ( CharsInUnicodeString && MaxBytesInCustomCPString ) {
            //
            // Convert to Single Byte and back to Unicode before upper
            // casing to ensure the visual best fits are converted and
            // upper cased properly.
            //
            MbChar = WideTranslateTable[ *UnicodeString++ ];
            if (NlsCustomLeadByteInfo[HIBYTE(MbChar)]) {
                //
                // Lead byte - translate the trail byte using the table
                // that corresponds to this lead byte.
                //
                Entry = NlsCustomLeadByteInfo[HIBYTE(MbChar)];
                UnicodeChar = NlsCustomLeadByteInfo[ Entry + LOBYTE(MbChar) ];
            } else {
                //
                // Single byte character.
                //
                UnicodeChar = (CustomCP->MultiByteTable)[LOBYTE(MbChar)];
            }
            UnicodeChar = (WCHAR)NLS_UPCASE(UnicodeChar);
            MbChar = WideTranslateTable[UnicodeChar];

            if (HIBYTE(MbChar) != 0) {
                //
                // Need at least 2 bytes to copy a double byte char.
                // Don't want to truncate in the middle of a DBCS char.
                //
                if (MaxBytesInCustomCPString-- < 2) {
                    break;
                }
                *CustomCPString++ = HIBYTE(MbChar);  // lead byte
            }
            *CustomCPString++ = LOBYTE(MbChar);
            MaxBytesInCustomCPString--;

            CharsInUnicodeString--;
        }

        if (ARGUMENT_PRESENT(BytesInCustomCPString))
            *BytesInCustomCPString = (ULONG)(CustomCPString - CustomCPStringAnchor);
    }

    //
    //  Check if we were able to use all of the source Unicode String
    //
    return ( CharsInUnicodeString <= MaxBytesInCustomCPString ) ?
           STATUS_SUCCESS :
           STATUS_BUFFER_OVERFLOW;
}

#define MB_TBL_SIZE      256             /* size of MB tables */
#define GLYPH_TBL_SIZE   MB_TBL_SIZE     /* size of GLYPH tables */
#define DBCS_TBL_SIZE    256             /* size of DBCS tables */
#define GLYPH_HEADER     1               /* size of GLYPH table header */
#define DBCS_HEADER      1               /* size of DBCS table header */
#define LANG_HEADER      1               /* size of LANGUAGE file header */
#define UP_HEADER        1               /* size of UPPERCASE table header */
#define LO_HEADER        1               /* size of LOWERCASE table header */

VOID
RtlInitCodePageTable(
    IN PUSHORT TableBase,
    OUT PCPTABLEINFO CodePageTable
    )
{
    USHORT offMB;
    USHORT offWC;
    PUSHORT pGlyph;
    PUSHORT pRange;

    RTL_PAGED_CODE();

    //
    // Get the offsets.
    //

    offMB = TableBase[0];
    offWC = offMB + TableBase[offMB];


    //
    // Attach Code Page Info to CP hash node.
    //

    CodePageTable->CodePage = TableBase[1];
    CodePageTable->MaximumCharacterSize = TableBase[2];
    CodePageTable->DefaultChar = TableBase[3];           // default character (MB)
    CodePageTable->UniDefaultChar = TableBase[4];        // default character (Unicode)
    CodePageTable->TransDefaultChar = TableBase[5];      // trans of default char (Unicode)
    CodePageTable->TransUniDefaultChar = TableBase[6];   // trans of Uni default char (MB)
    RtlMoveMemory(
        &CodePageTable->LeadByte,
        &TableBase[7],
        MAXIMUM_LEADBYTES
        );
    CodePageTable->MultiByteTable = (TableBase + offMB + 1);

    pGlyph = CodePageTable->MultiByteTable + MB_TBL_SIZE;

    if (pGlyph[0] != 0) {
        pRange = CodePageTable->DBCSRanges = pGlyph + GLYPH_HEADER + GLYPH_TBL_SIZE;
        }
    else {
        pRange = CodePageTable->DBCSRanges = pGlyph + GLYPH_HEADER;
        }

    //
    //  Attach DBCS information to CP hash node.
    //

    if (pRange[0] > 0) {
        CodePageTable->DBCSOffsets = pRange + DBCS_HEADER;
        CodePageTable->DBCSCodePage = 1;
        }
    else {
        CodePageTable->DBCSCodePage = 0;
        CodePageTable->DBCSOffsets = NULL;
        }

    CodePageTable->WideCharTable = (TableBase + offWC + 1);
}


VOID
RtlpInitUpcaseTable(
    IN PUSHORT TableBase,
    OUT PNLSTABLEINFO CodePageTable
    )
{
    USHORT offUP;
    USHORT offLO;

    //
    // Get the offsets.
    //

    offUP = LANG_HEADER;
    offLO = offUP + TableBase[offUP];

    CodePageTable->UpperCaseTable = TableBase + offUP + UP_HEADER;
    CodePageTable->LowerCaseTable = TableBase + offLO + LO_HEADER;
}


VOID
RtlInitNlsTables(
    IN PUSHORT AnsiNlsBase,
    IN PUSHORT OemNlsBase,
    IN PUSHORT LanguageNlsBase,
    OUT PNLSTABLEINFO TableInfo
    )
{
    RTL_PAGED_CODE();

    RtlInitCodePageTable(AnsiNlsBase,&TableInfo->AnsiTableInfo);
    RtlInitCodePageTable(OemNlsBase,&TableInfo->OemTableInfo);
    RtlpInitUpcaseTable(LanguageNlsBase,TableInfo);
}


VOID
RtlResetRtlTranslations(
    PNLSTABLEINFO TableInfo
    )
{
    RTL_PAGED_CODE();

    if ( TableInfo->AnsiTableInfo.DBCSCodePage ) {
        RtlMoveMemory(NlsLeadByteInfo,TableInfo->AnsiTableInfo.DBCSOffsets,DBCS_TBL_SIZE*sizeof(USHORT));
        }
    else {
        RtlZeroMemory(NlsLeadByteInfo,DBCS_TBL_SIZE*sizeof(USHORT));
        }

    NlsMbAnsiCodePageTables = (PUSHORT)TableInfo->AnsiTableInfo.DBCSOffsets;

    NlsAnsiToUnicodeData = TableInfo->AnsiTableInfo.MultiByteTable;
    NlsUnicodeToAnsiData = (PCH)TableInfo->AnsiTableInfo.WideCharTable;
    NlsUnicodeToMbAnsiData = (PUSHORT)TableInfo->AnsiTableInfo.WideCharTable;
    NlsMbCodePageTag = TableInfo->AnsiTableInfo.DBCSCodePage ? TRUE : FALSE;
    NlsAnsiCodePage = TableInfo->AnsiTableInfo.CodePage;

    if ( TableInfo->OemTableInfo.DBCSCodePage ) {
        RtlMoveMemory(NlsOemLeadByteInfo,TableInfo->OemTableInfo.DBCSOffsets,DBCS_TBL_SIZE*sizeof(USHORT));
        }
    else {
        RtlZeroMemory(NlsOemLeadByteInfo,DBCS_TBL_SIZE*sizeof(USHORT));
        }

    NlsMbOemCodePageTables = (PUSHORT)TableInfo->OemTableInfo.DBCSOffsets;

    NlsOemToUnicodeData = TableInfo->OemTableInfo.MultiByteTable;
    NlsUnicodeToOemData = (PCH)TableInfo->OemTableInfo.WideCharTable;
    NlsUnicodeToMbOemData = (PUSHORT)TableInfo->OemTableInfo.WideCharTable;
    NlsMbOemCodePageTag = TableInfo->OemTableInfo.DBCSCodePage ? TRUE : FALSE;
    NlsOemCodePage = TableInfo->OemTableInfo.CodePage;
    OemDefaultChar = TableInfo->OemTableInfo.DefaultChar;
    OemTransUniDefaultChar = TableInfo->OemTableInfo.TransDefaultChar;

    Nls844UnicodeUpcaseTable = TableInfo->UpperCaseTable;
    Nls844UnicodeLowercaseTable = TableInfo->LowerCaseTable;
    UnicodeDefaultChar = TableInfo->AnsiTableInfo.UniDefaultChar;
}

void
RtlGetDefaultCodePage(
    OUT PUSHORT AnsiCodePage,
    OUT PUSHORT OemCodePage
    )
{
    RTL_PAGED_CODE();
    *AnsiCodePage = NlsAnsiCodePage;
    *OemCodePage = NlsOemCodePage;
}





