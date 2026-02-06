/*
 * PROJECT:     ReactOS win32 kernel mode subsystem
 * LICENSE:     GPL-2.1-or-later (https://spdx.org/licenses/GPL-2.1-or-later)
 * PURPOSE:     Utility functions
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

extern POINTL g_PointZero;
extern UNICODE_STRING g_FontRegPath;

SIZE_T SZZ_GetSize(_In_ PCZZWSTR pszz);
LONG IntNormalizeAngle(_In_ LONG nTenthsOfDegrees);
LPCWSTR FASTCALL IntNameFromCharSet(_In_ BYTE CharSet);
BYTE IntCharSetFromLangID(_In_ LANGID LangID);
VOID IntSwapEndian(_Inout_ LPVOID pvData, _In_ DWORD Size);
PWSTR PathFindFileNameW(_In_ PCWSTR pszPath);
BOOL PathIsRelativeW(_In_ LPCWSTR lpszPath);

VOID
IntRebaseList(
    _Inout_ PLIST_ENTRY pNewHead,
    _Inout_ PLIST_ENTRY pOldHead);

VOID
IntUnicodeStringToBuffer(
    _Out_ LPWSTR pszBuffer,
    _In_ SIZE_T cbBuffer,
    _In_ const UNICODE_STRING *pString);

NTSTATUS
IntDuplicateUnicodeString(
    _In_ PCUNICODE_STRING Source,
    _Out_ PUNICODE_STRING Destination);

/* The ranges of the surrogate pairs */
#define HIGH_SURROGATE_MIN 0xD800U
#define HIGH_SURROGATE_MAX 0xDBFFU
#define LOW_SURROGATE_MIN  0xDC00U
#define LOW_SURROGATE_MAX  0xDFFFU

#define IS_HIGH_SURROGATE(ch0) (HIGH_SURROGATE_MIN <= (ch0) && (ch0) <= HIGH_SURROGATE_MAX)
#define IS_LOW_SURROGATE(ch1)  (LOW_SURROGATE_MIN  <= (ch1) && (ch1) <=  LOW_SURROGATE_MAX)

static inline DWORD
Utf32FromSurrogatePair(_In_ DWORD ch0, _In_ DWORD ch1)
{
    return ((ch0 - HIGH_SURROGATE_MIN) << 10) + (ch1 - LOW_SURROGATE_MIN) + 0x10000;
}

VOID
FASTCALL
IntEngFillBox(
    IN OUT PDC dc,
    IN INT X,
    IN INT Y,
    IN INT Width,
    IN INT Height,
    IN BRUSHOBJ *BrushObj);

VOID APIENTRY
IntEngFillPolygon(
    IN OUT PDC dc,
    IN POINTL *pPoints,
    IN UINT cPoints,
    IN BRUSHOBJ *BrushObj);

static inline
LONG
ScaleLong(LONG lValue, PFLOATOBJ pef)
{
    FLOATOBJ efTemp;

    /* Check if we have scaling different from 1 */
    if (!FLOATOBJ_Equal(pef, (PFLOATOBJ)&gef1))
    {
        /* Need to multiply */
        FLOATOBJ_SetLong(&efTemp, lValue);
        FLOATOBJ_Mul(&efTemp, pef);
        lValue = FLOATOBJ_GetLong(&efTemp);
    }

    return lValue;
}
