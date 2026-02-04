/*
 * PROJECT:     ReactOS win32 kernel mode subsystem
 * LICENSE:     GPL-2.1-or-later (https://spdx.org/licenses/GPL-2.1-or-later)
 * PURPOSE:     Utility functions
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include <win32k.h>

POINTL g_PointZero = { 0, 0 };

UNICODE_STRING g_FontRegPath =
    RTL_CONSTANT_STRING(L"\\REGISTRY\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts");

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

/* Borrowed from shlwapi */
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

/* Delete registry font entries */
VOID IntDeleteRegFontEntries(_In_ PCWSTR pszFileName, _In_ DWORD dwFlags)
{
    NTSTATUS Status;
    HKEY hKey;
    WCHAR szName[MAX_PATH], szValue[MAX_PATH];
    ULONG dwIndex, NameLength, ValueSize, dwType;

    Status = RegOpenKey(g_FontRegPath.Buffer, &hKey);
    if (!NT_SUCCESS(Status))
        return;

    for (dwIndex = 0;;)
    {
        NameLength = RTL_NUMBER_OF(szName);
        ValueSize = sizeof(szValue);
        Status = RegEnumValueW(hKey, dwIndex, szName, &NameLength, &dwType, szValue, &ValueSize);
        if (!NT_SUCCESS(Status))
            break;

        if (dwType != REG_SZ || _wcsicmp(szValue, pszFileName) != 0)
        {
            ++dwIndex;
            continue;
        }

        /* Delete the found value */
        Status = RegDeleteValueW(hKey, szName);
        if (!NT_SUCCESS(Status))
            break;
    }

    ZwClose(hKey);
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
