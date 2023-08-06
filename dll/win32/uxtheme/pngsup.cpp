/*
 * PROJECT:     ReactOS uxtheme.dll
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Support for PNG images in visual styles
 * COPYRIGHT:   Copyright 2023 Ethan Rodensky <splitwirez@gmail.com>
 */

#include "uxthemep.h"

#include <ole2.h>
#include <objidl.h>
#include <gdiplus.h>
#include <gdipluscolor.h>
#include <winreg.h>
#include <shlwapi.h>

BOOL
MSSTYLES_TryLoadPng(
    _In_ HINSTANCE hTheme,
    _In_ LPCWSTR szFile,
    _In_ LPCWSTR type,
    _Out_ HBITMAP *phBitmap)
{
    BOOL ret = FALSE;

    HRSRC hRes = FindResourceW(hTheme, szFile, type);
    if (!hRes)
        return FALSE;

    HGLOBAL hAlloc = LoadResource(hTheme, hRes);
    if (!hAlloc)
        return FALSE;

    DWORD dwSize = SizeofResource(hTheme, hRes);
    LPVOID pData = LockResource(hAlloc);
    if ((!pData) || (dwSize <= 0))
    {
        FreeResource(hAlloc);
        return FALSE;
    }

    IStream* stream = SHCreateMemStream((BYTE*)pData, dwSize);
    if (stream)
    {
        Gdiplus::Bitmap* gdipBitmap = Gdiplus::Bitmap::FromStream(stream, FALSE);
        stream->Release();
        if (gdipBitmap)
        {
            ret = gdipBitmap->GetHBITMAP(Gdiplus::Color(0, 0, 0, 0), phBitmap) == Gdiplus::Ok;
            delete gdipBitmap;
        }
    }

    UnlockResource(pData);
    FreeResource(hAlloc);
    return ret;
}

BOOL
prepare_png_alpha(
    _In_ HBITMAP png,
    _Out_ BOOL* hasAlpha)
{
    DIBSECTION dib;
    int n;
    BYTE* p;

    *hasAlpha = FALSE;

    if (!png || GetObjectW( png, sizeof(dib), &dib ) != sizeof(dib))
        return FALSE;

    if (dib.dsBm.bmBitsPixel != 32)
        /* nothing to do */
        return TRUE;

    p = (BYTE*)dib.dsBm.bmBits;
    n = dib.dsBmih.biHeight * dib.dsBmih.biWidth;
    while (n-- > 0)
    {
        int a = p[3] + 1;
        if (a < 256)
        {
            p[0] = MulDiv(p[0], 256, a);
            p[1] = MulDiv(p[1], 256, a);
            p[2] = MulDiv(p[2], 256, a);

            *hasAlpha = TRUE;
        }
        p += 4;
    }

    return TRUE;
}
