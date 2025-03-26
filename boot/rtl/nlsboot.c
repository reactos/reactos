/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     "Poor-man" boot-time National Language Support (NLS) functions.
 * COPYRIGHT:   Copyright 2022 Hermès Bélusca-Maïto
 *
 * NOTE: This code is used at boot-time when no NLS tables are loaded.
 * Adapted from lib/rtl/nls.c
 */

/* INCLUDES ******************************************************************/

#include <rtl.h>

/* GLOBALS *******************************************************************/

BOOLEAN NlsMbCodePageTag = FALSE;

BOOLEAN NlsMbOemCodePageTag = FALSE;
PUSHORT NlsOemToUnicodeTable = NULL;
PCHAR NlsUnicodeToOemTable = NULL;
PUSHORT NlsUnicodeToMbOemTable = NULL;
PUSHORT NlsOemLeadByteInfo = NULL;

USHORT NlsOemDefaultChar = '\0';
USHORT NlsUnicodeDefaultChar = 0;

/* FUNCTIONS *****************************************************************/

WCHAR
NTAPI
RtlpDowncaseUnicodeChar(
    _In_ WCHAR Source)
{
    USHORT Offset = 0;

    if (Source < L'A')
        return Source;

    if (Source <= L'Z')
        return Source + (L'a' - L'A');

#if 0
    if (Source < 0x80)
        return Source;
#endif

    return Source + (SHORT)Offset;
}

WCHAR
NTAPI
RtlDowncaseUnicodeChar(
    _In_ WCHAR Source)
{
    return RtlpDowncaseUnicodeChar(Source);
}

_Use_decl_annotations_
NTSTATUS
NTAPI
RtlMultiByteToUnicodeN(
    _Out_ PWCH UnicodeString,
    _In_ ULONG UnicodeSize,
    _Out_opt_ PULONG ResultSize,
    _In_ PCCH MbString,
    _In_ ULONG MbSize)
{
    ULONG Size = 0;
    ULONG i;

    /* single-byte code page */
    if (MbSize > (UnicodeSize / sizeof(WCHAR)))
        Size = UnicodeSize / sizeof(WCHAR);
    else
        Size = MbSize;

    if (ResultSize)
        *ResultSize = Size * sizeof(WCHAR);

    for (i = 0; i < Size; i++)
    {
        /* Trivially zero-extend */
        UnicodeString[i] = (WCHAR)MbString[i];
    }

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
NTAPI
RtlMultiByteToUnicodeSize(
    _Out_ PULONG UnicodeSize,
    _In_ PCCH MbString,
    _In_ ULONG MbSize)
{
    /* single-byte code page */
    *UnicodeSize = MbSize * sizeof(WCHAR);

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
NTAPI
RtlUnicodeToMultiByteN(
    _Out_ PCHAR MbString,
    _In_ ULONG MbSize,
    _Out_opt_ PULONG ResultSize,
    _In_ PCWCH UnicodeString,
    _In_ ULONG UnicodeSize)
{
    ULONG Size = 0;
    ULONG i;

    /* single-byte code page */
    Size = (UnicodeSize > (MbSize * sizeof(WCHAR)))
            ? MbSize : (UnicodeSize / sizeof(WCHAR));

    if (ResultSize)
        *ResultSize = Size;

    for (i = 0; i < Size; i++)
    {
        /* Check for characters that cannot be trivially demoted to ANSI */
        if (*((PCHAR)UnicodeString + 1) == 0)
        {
            *MbString++ = (CHAR)*UnicodeString++;
        }
        else
        {
            /* Invalid character, use default */
            *MbString++ = NlsOemDefaultChar;
            UnicodeString++;
        }
    }

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
NTAPI
RtlUnicodeToMultiByteSize(
    _Out_ PULONG MbSize,
    _In_ PCWCH UnicodeString,
    _In_ ULONG UnicodeSize)
{
    ULONG UnicodeLength = UnicodeSize / sizeof(WCHAR);

    /* single-byte code page */
    *MbSize = UnicodeLength;

    return STATUS_SUCCESS;
}

WCHAR
NTAPI
RtlpUpcaseUnicodeChar(
    _In_ WCHAR Source)
{
    USHORT Offset = 0;

    if (Source < 'a')
        return Source;

    if (Source <= 'z')
        return (Source - ('a' - 'A'));

    return Source + (SHORT)Offset;
}

WCHAR
NTAPI
RtlUpcaseUnicodeChar(
    _In_ WCHAR Source)
{
    return RtlpUpcaseUnicodeChar(Source);
}

_Use_decl_annotations_
NTSTATUS
NTAPI
RtlUpcaseUnicodeToMultiByteN(
    _Out_ PCHAR MbString,
    _In_ ULONG MbSize,
    _Out_opt_ PULONG ResultSize,
    _In_ PCWCH UnicodeString,
    _In_ ULONG UnicodeSize)
{
    WCHAR UpcaseChar;
    ULONG Size = 0;
    ULONG i;

    /* single-byte code page */
    if (UnicodeSize > (MbSize * sizeof(WCHAR)))
        Size = MbSize;
    else
        Size = UnicodeSize / sizeof(WCHAR);

    if (ResultSize)
        *ResultSize = Size;

    for (i = 0; i < Size; i++)
    {
        UpcaseChar = RtlpUpcaseUnicodeChar(*UnicodeString);

        /* Check for characters that cannot be trivially demoted to ANSI */
        if (*((PCHAR)&UpcaseChar + 1) == 0)
        {
            *MbString = (CHAR)UpcaseChar;
        }
        else
        {
            /* Invalid character, use default */
            *MbString = NlsOemDefaultChar;
        }

        MbString++;
        UnicodeString++;
    }

    return STATUS_SUCCESS;
}

CHAR
NTAPI
RtlUpperChar(
    _In_ CHAR Source)
{
    /* Check for simple ANSI case */
    if (Source <= 'z')
    {
        /* Check for simple downcase a-z case */
        if (Source >= 'a')
        {
            /* Just XOR with the difference */
            return Source ^ ('a' - 'A');
        }
        else
        {
            /* Otherwise return the same char, it's already upcase */
            return Source;
        }
    }
    else
    {
        /* single-byte code page */
        return (CHAR)RtlpUpcaseUnicodeChar((WCHAR)Source);
    }
}


/**
 * Stubbed OEM helpers that should not be used in the OS boot loader,
 * but are necessary for linking with the rest of the RTL unicode.c.
 **/

_Use_decl_annotations_
NTSTATUS
NTAPI
RtlUnicodeToOemN(
    _Out_ PCHAR OemString,
    _In_ ULONG OemSize,
    _Out_opt_ PULONG ResultSize,
    _In_ PCWCH UnicodeString,
    _In_ ULONG UnicodeSize)
{
    if (OemSize)
        *OemString = ANSI_NULL;

    if (ResultSize)
        *ResultSize = 0;

    return STATUS_NOT_IMPLEMENTED;
}

_Use_decl_annotations_
NTSTATUS
NTAPI
RtlOemToUnicodeN(
    _Out_ PWCHAR UnicodeString,
    _In_ ULONG UnicodeSize,
    _Out_opt_ PULONG ResultSize,
    _In_ PCCH OemString,
    _In_ ULONG OemSize)
{
    if (UnicodeString)
        *UnicodeString = UNICODE_NULL;

    if (ResultSize)
        *ResultSize = 0;

    return STATUS_NOT_IMPLEMENTED;
}

_Use_decl_annotations_
NTSTATUS
NTAPI
RtlUpcaseUnicodeToOemN(
    _Out_ PCHAR OemString,
    _In_ ULONG OemSize,
    _Out_opt_ PULONG ResultSize,
    _In_ PCWCH UnicodeString,
    _In_ ULONG UnicodeSize)
{
    if (OemSize)
        *OemString = ANSI_NULL;

    if (ResultSize)
        *ResultSize = 0;

    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
