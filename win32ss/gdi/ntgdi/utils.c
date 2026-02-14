/*
 * PROJECT:     ReactOS win32 kernel mode subsystem
 * LICENSE:     GPL-2.1-or-later (https://spdx.org/licenses/GPL-2.1-or-later)
 * PURPOSE:     Utility functions
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include <win32k.h>

extern UNICODE_STRING g_FontRegPath;

POINTL g_PointZero = { 0, 0 };

SIZE_T
SZZ_GetSize(_In_ PCZZWSTR pszz)
{
    SIZE_T ret = 0, cch;
    const WCHAR *pch = pszz;
    while (*pch)
    {
        cch = wcslen(pch) + 1;
        ret += cch;
        pch += cch;
    }
    ++ret;
    return ret * sizeof(WCHAR);
}

VOID
IntRebaseList(
    _Inout_ PLIST_ENTRY pNewHead,
    _Inout_ PLIST_ENTRY pOldHead)
{
    PLIST_ENTRY Entry;

    ASSERT(pNewHead != pOldHead);

    InitializeListHead(pNewHead);
    while (!IsListEmpty(pOldHead))
    {
        Entry = RemoveTailList(pOldHead);
        InsertHeadList(pNewHead, Entry);
    }
}

LONG IntNormalizeAngle(_In_ LONG nTenthsOfDegrees)
{
    nTenthsOfDegrees %= 360 * 10;
    if (nTenthsOfDegrees >= 0)
        return nTenthsOfDegrees;
    return nTenthsOfDegrees + 360 * 10;
}

LPCWSTR FASTCALL IntNameFromCharSet(_In_ BYTE CharSet)
{
    switch (CharSet)
    {
        case ANSI_CHARSET: return L"ANSI";
        case DEFAULT_CHARSET: return L"Default";
        case SYMBOL_CHARSET: return L"Symbol";
        case SHIFTJIS_CHARSET: return L"Shift_JIS";
        case HANGUL_CHARSET: return L"Hangul";
        case GB2312_CHARSET: return L"GB 2312";
        case CHINESEBIG5_CHARSET: return L"Chinese Big5";
        case OEM_CHARSET: return L"OEM";
        case JOHAB_CHARSET: return L"Johab";
        case HEBREW_CHARSET: return L"Hebrew";
        case ARABIC_CHARSET: return L"Arabic";
        case GREEK_CHARSET: return L"Greek";
        case TURKISH_CHARSET: return L"Turkish";
        case VIETNAMESE_CHARSET: return L"Vietnamese";
        case THAI_CHARSET: return L"Thai";
        case EASTEUROPE_CHARSET: return L"Eastern European";
        case RUSSIAN_CHARSET: return L"Russian";
        case MAC_CHARSET: return L"Mac";
        case BALTIC_CHARSET: return L"Baltic";
        default: return L"Unknown";
    }
}

/* See https://learn.microsoft.com/en-us/previous-versions/visualstudio/visual-studio-2008/bb165625(v=vs.90) */
BYTE IntCharSetFromLangID(_In_ LANGID LangID)
{
    /* FIXME: Add more and fix if wrong */
    switch (PRIMARYLANGID(LangID))
    {
        case LANG_CHINESE:
            switch (SUBLANGID(LangID))
            {
                case SUBLANG_CHINESE_TRADITIONAL:
                    return CHINESEBIG5_CHARSET;
                case SUBLANG_CHINESE_SIMPLIFIED:
                default:
                    break;
            }
            return GB2312_CHARSET;

        case LANG_CZECH: case LANG_HUNGARIAN: case LANG_POLISH:
        case LANG_SLOVAK: case LANG_SLOVENIAN: case LANG_ROMANIAN:
            return EASTEUROPE_CHARSET;

        case LANG_RUSSIAN: case LANG_BULGARIAN: case LANG_MACEDONIAN:
        case LANG_SERBIAN: case LANG_UKRAINIAN:
            return RUSSIAN_CHARSET;

        case LANG_ARABIC:       return ARABIC_CHARSET;
        case LANG_GREEK:        return GREEK_CHARSET;
        case LANG_HEBREW:       return HEBREW_CHARSET;
        case LANG_JAPANESE:     return SHIFTJIS_CHARSET;
        case LANG_KOREAN:       return JOHAB_CHARSET;
        case LANG_TURKISH:      return TURKISH_CHARSET;
        case LANG_THAI:         return THAI_CHARSET;
        case LANG_LATVIAN:      return BALTIC_CHARSET;
        case LANG_VIETNAMESE:   return VIETNAMESE_CHARSET;

        case LANG_ENGLISH: case LANG_BASQUE: case LANG_CATALAN:
        case LANG_DANISH: case LANG_DUTCH: case LANG_FINNISH:
        case LANG_FRENCH: case LANG_GERMAN: case LANG_ITALIAN:
        case LANG_NORWEGIAN: case LANG_PORTUGUESE: case LANG_SPANISH:
        case LANG_SWEDISH: default:
            return ANSI_CHARSET;
    }
}

VOID IntSwapEndian(_Inout_ LPVOID pvData, _In_ DWORD Size)
{
    BYTE b, *pb = pvData;
    Size /= 2;
    while (Size-- > 0)
    {
        b = pb[0];
        pb[0] = pb[1];
        pb[1] = b;
        ++pb; ++pb;
    }
}

/* Borrowed from shlwapi!PathFindFileNameW */
PWSTR PathFindFileNameW(_In_ PCWSTR pszPath)
{
    PCWSTR lastSlash = pszPath;
    while (*pszPath)
    {
        if ((*pszPath == L'\\' || *pszPath == L'/' || *pszPath == L':') &&
            pszPath[1] && pszPath[1] != '\\' && pszPath[1] != L'/')
        {
            lastSlash = pszPath + 1;
        }
        pszPath++;
    }
    return (PWSTR)lastSlash;
}

/* Borrowed from shlwapi!PathIsRelativeW */
BOOL PathIsRelativeW(_In_ LPCWSTR lpszPath)
{
    if (!lpszPath || !*lpszPath)
        return TRUE;
    if (*lpszPath == L'\\' || (*lpszPath && lpszPath[1] == L':'))
        return FALSE;
    return TRUE;
}

VOID
IntUnicodeStringToBuffer(
    _Out_ LPWSTR pszBuffer,
    _In_ SIZE_T cbBuffer,
    _In_ const UNICODE_STRING *pString)
{
    SIZE_T cbLength = pString->Length;

    if (cbBuffer < sizeof(UNICODE_NULL))
        return;

    if (cbLength > cbBuffer - sizeof(UNICODE_NULL))
        cbLength = cbBuffer - sizeof(UNICODE_NULL);

    RtlCopyMemory(pszBuffer, pString->Buffer, cbLength);
    pszBuffer[cbLength / sizeof(WCHAR)] = UNICODE_NULL;
}

NTSTATUS
IntDuplicateUnicodeString(
    _In_ PCUNICODE_STRING Source,
    _Out_ PUNICODE_STRING Destination)
{
    NTSTATUS Status = STATUS_NO_MEMORY;
    UNICODE_STRING Tmp;

    Tmp.Buffer = ExAllocatePoolWithTag(PagedPool, Source->MaximumLength, TAG_USTR);
    if (Tmp.Buffer)
    {
        Tmp.MaximumLength = Source->MaximumLength;
        Tmp.Length = 0;
        RtlCopyUnicodeString(&Tmp, Source);

        Destination->MaximumLength = Tmp.MaximumLength;
        Destination->Length = Tmp.Length;
        Destination->Buffer = Tmp.Buffer;

        Status = STATUS_SUCCESS;
    }

    return Status;
}

VOID
FASTCALL
IntEngFillBox(
    IN OUT PDC dc,
    IN INT X,
    IN INT Y,
    IN INT Width,
    IN INT Height,
    IN BRUSHOBJ *BrushObj)
{
    RECTL DestRect;
    SURFACE *psurf = dc->dclevel.pSurface;

    ASSERT_DC_PREPARED(dc);
    ASSERT(psurf != NULL);

    if (Width < 0)
    {
        X += Width;
        Width = -Width;
    }

    if (Height < 0)
    {
        Y += Height;
        Height = -Height;
    }

    DestRect.left = X;
    DestRect.right = X + Width;
    DestRect.top = Y;
    DestRect.bottom = Y + Height;

    IntEngBitBlt(&psurf->SurfObj,
                 NULL,
                 NULL,
                 (CLIPOBJ *)&dc->co,
                 NULL,
                 &DestRect,
                 NULL,
                 NULL,
                 BrushObj,
                 &g_PointZero,
                 ROP4_FROM_INDEX(R3_OPINDEX_PATCOPY));
}

VOID APIENTRY
IntEngFillPolygon(
    IN OUT PDC dc,
    IN POINTL *pPoints,
    IN UINT cPoints,
    IN BRUSHOBJ *BrushObj)
{
    SURFACE *psurf = dc->dclevel.pSurface;
    RECT Rect;
    UINT i;
    INT x, y;

    ASSERT_DC_PREPARED(dc);
    ASSERT(psurf != NULL);

    Rect.left = Rect.right = pPoints[0].x;
    Rect.top = Rect.bottom = pPoints[0].y;
    for (i = 1; i < cPoints; ++i)
    {
        x = pPoints[i].x;
        if (x < Rect.left)
            Rect.left = x;
        else if (Rect.right < x)
            Rect.right = x;

        y = pPoints[i].y;
        if (y < Rect.top)
            Rect.top = y;
        else if (Rect.bottom < y)
            Rect.bottom = y;
    }

    IntFillPolygon(dc, dc->dclevel.pSurface, BrushObj, pPoints, cPoints, Rect, &g_PointZero);
}
