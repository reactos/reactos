/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/misc.c
 * PURPOSE:         Manages all the partitions of the OS in an interactive way.
 * PROGRAMMERS:     Eric Kohl
 */

#include "diskpart.h"

/* FUNCTIONS ******************************************************************/

BOOL
IsDecString(
    _In_ PWSTR pszDecString)
{
    PWSTR ptr;

    if ((pszDecString == NULL) || (*pszDecString == UNICODE_NULL))
        return FALSE;

    ptr = pszDecString;
    while (*ptr != UNICODE_NULL)
    {
        if (!iswdigit(*ptr))
            return FALSE;

        ptr++;
    }

    return TRUE;
}


BOOL
IsHexString(
    _In_ PWSTR pszHexString)
{
    PWSTR ptr;

    if ((pszHexString == NULL) || (*pszHexString == UNICODE_NULL))
        return FALSE;

    ptr = pszHexString;
    while (*ptr != UNICODE_NULL)
    {
        if (!iswxdigit(*ptr))
            return FALSE;

        ptr++;
    }

    return TRUE;
}


BOOL
HasPrefix(
    _In_ PWSTR pszString,
    _In_ PWSTR pszPrefix,
    _Out_opt_ PWSTR *ppszSuffix)
{
    INT nPrefixLength, ret;

    nPrefixLength = wcslen(pszPrefix);
    ret = _wcsnicmp(pszString, pszPrefix, nPrefixLength);
    if ((ret == 0) && (ppszSuffix != NULL))
        *ppszSuffix = &pszString[nPrefixLength];

    return (ret == 0);
}


ULONGLONG
RoundingDivide(
    _In_ ULONGLONG Dividend,
    _In_ ULONGLONG Divisor)
{
    return (Dividend + Divisor / 2) / Divisor;
}


PWSTR
DuplicateQuotedString(
    _In_ PWSTR pszInString)
{
    PWSTR pszOutString = NULL;
    PWSTR pStart, pEnd;
    INT nLength;

    if ((pszInString == NULL) || (pszInString[0] == UNICODE_NULL))
        return NULL;

    if (pszInString[0] == L'"')
    {
        if (pszInString[1] == UNICODE_NULL)
            return NULL;

        pStart = &pszInString[1];
        pEnd = wcschr(pStart, '"');
        if (pEnd == NULL)
        {
            nLength = wcslen(pStart);
        }
        else
        {
            nLength = (pEnd - pStart);
        }
    }
    else
    {
        pStart = pszInString;
        nLength = wcslen(pStart);
    }

    pszOutString = RtlAllocateHeap(RtlGetProcessHeap(),
                                   HEAP_ZERO_MEMORY,
                                   (nLength + 1) * sizeof(WCHAR));
    if (pszOutString == NULL)
        return NULL;

    wcsncpy(pszOutString, pStart, nLength);

    return pszOutString;
}


PWSTR
DuplicateString(
    _In_ PWSTR pszInString)
{
    PWSTR pszOutString = NULL;
    INT nLength;

    if ((pszInString == NULL) || (pszInString[0] == UNICODE_NULL))
        return NULL;

    nLength = wcslen(pszInString);
    pszOutString = RtlAllocateHeap(RtlGetProcessHeap(),
                                   HEAP_ZERO_MEMORY,
                                   (nLength + 1) * sizeof(WCHAR));
    if (pszOutString == NULL)
        return NULL;

    wcscpy(pszOutString, pszInString);

    return pszOutString;
}


VOID
CreateGUID(
    _Out_ GUID *pGuid)
{
    RtlGenRandom(pGuid, sizeof(*pGuid));
    /* Clear the version bits and set the version (4) */
    pGuid->Data3 &= 0x0fff;
    pGuid->Data3 |= (4 << 12);
    /* Set the topmost bits of Data4 (clock_seq_hi_and_reserved) as
     * specified in RFC 4122, section 4.4.
     */
    pGuid->Data4[0] &= 0x3f;
    pGuid->Data4[0] |= 0x80;
}


VOID
CreateSignature(
    _Out_ PDWORD pSignature)
{
    LARGE_INTEGER SystemTime;
    TIME_FIELDS TimeFields;
    PUCHAR Buffer;

    NtQuerySystemTime(&SystemTime);
    RtlTimeToTimeFields(&SystemTime, &TimeFields);

    Buffer = (PUCHAR)pSignature;
    Buffer[0] = (UCHAR)(TimeFields.Year & 0xFF) + (UCHAR)(TimeFields.Hour & 0xFF);
    Buffer[1] = (UCHAR)(TimeFields.Year >> 8) + (UCHAR)(TimeFields.Minute & 0xFF);
    Buffer[2] = (UCHAR)(TimeFields.Month & 0xFF) + (UCHAR)(TimeFields.Second & 0xFF);
    Buffer[3] = (UCHAR)(TimeFields.Day & 0xFF) + (UCHAR)(TimeFields.Milliseconds & 0xFF);
}


VOID
PrintGUID(
    _Out_ PWSTR pszBuffer,
    _In_ GUID *pGuid)
{
    swprintf(pszBuffer,
             L"%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
             pGuid->Data1,
             pGuid->Data2,
             pGuid->Data3,
             pGuid->Data4[0],
             pGuid->Data4[1],
             pGuid->Data4[2],
             pGuid->Data4[3],
             pGuid->Data4[4],
             pGuid->Data4[5],
             pGuid->Data4[6],
             pGuid->Data4[7]);
}

static
BYTE
CharToByte(
    WCHAR Char)
{
    if (iswdigit(Char))
    {
        return (BYTE)((INT)Char - (INT)'0');
    }
    else if (iswalpha(Char))
    {
        if (iswupper(Char))
            return (INT)Char - ((INT)'A' - 10);
        else
            return (INT)Char - ((INT)'a' - 10);
    }
    return 0;
}


BOOL
StringToGUID(
    _Out_ GUID *pGuid,
    _In_ PWSTR pszString)
{
    INT i;

    if (pszString == NULL)
        return FALSE;

    pGuid->Data1 = 0;
    for (i = 0; i < 8; i++)
    {
        if (!iswxdigit(pszString[i]))
            return FALSE;

        pGuid->Data1 = (pGuid->Data1 << 4) | CharToByte(pszString[i]);
    }

    if (pszString[8] != L'-')
        return FALSE;

    pGuid->Data2 = 0;
    for (i = 9; i < 13; i++)
    {
        if (!iswxdigit(pszString[i]))
            return FALSE;

        pGuid->Data2 = (pGuid->Data2 << 4) | CharToByte(pszString[i]);
    }

    if (pszString[13] != L'-')
        return FALSE;

    pGuid->Data3 = 0;
    for (i = 14; i < 18; i++)
    {
        if (!iswxdigit(pszString[i]))
            return FALSE;

        pGuid->Data3 = (pGuid->Data3 << 4) | CharToByte(pszString[i]);
    }

    if (pszString[18] != L'-')
        return FALSE;

    for (i = 19; i < 36; i += 2)
    {
        if (i == 23)
        {
            if (pszString[i] != L'-')
                return FALSE;
            i++;
        }

        if (!iswxdigit(pszString[i]) || !iswxdigit(pszString[i + 1]))
            return FALSE;

        pGuid->Data4[(i - 19) / 2] = CharToByte(pszString[i]) << 4 | CharToByte(pszString[i + 1]);
    }

    if (pszString[36] == L'\0')
        return TRUE;

    return FALSE;
}


VOID
PrintBusType(
    _Out_ PWSTR pszBuffer,
    _In_ INT cchBufferMax,
    _In_ STORAGE_BUS_TYPE BusType)
{
    if (BusType <= BusTypeMax)
        LoadStringW(GetModuleHandle(NULL),
                    IDS_BUSTYPE_UNKNOWN + BusType,
                    pszBuffer,
                    cchBufferMax);
    else
        LoadStringW(GetModuleHandle(NULL),
                    IDS_BUSTYPE_OTHER,
                    pszBuffer,
                    cchBufferMax);
}
