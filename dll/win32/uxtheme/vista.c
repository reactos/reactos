/*
 * Vista+ functions copied and pasted from draw.c and system.c
 *
 * Copyright (C) 2003 Kevin Koltzau
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "uxthemep.h"

#include <stdlib.h>

typedef int (WINAPI * DRAWSHADOWTEXT)(HDC hdc, LPCWSTR pszText, UINT cch, RECT *prc, DWORD dwFlags,
                          COLORREF crText, COLORREF crShadow, int ixOffset, int iyOffset);

/***********************************************************************
 *      DrawThemeTextEx                                     (UXTHEME.@)
 */
HRESULT
WINAPI
DrawThemeTextEx(
    _In_ HTHEME hTheme,
    _In_ HDC hdc,
    _In_ int iPartId,
    _In_ int iStateId,
    _In_ LPCWSTR pszText,
    _In_ int iCharCount,
    _In_ DWORD dwTextFlags,
    _Inout_ LPRECT pRect,
    _In_ const DTTOPTS *options
)
{
    HRESULT hr;
    HFONT hFont = NULL;
    HGDIOBJ oldFont = NULL;
    LOGFONTW logfont;
    COLORREF textColor;
    COLORREF oldTextColor;
    COLORREF shadowColor;
    POINT ptShadowOffset;
    int oldBkMode;
    RECT rt;
    int iShadowType;
    DWORD optFlags;

    if(!hTheme)
        return E_HANDLE;
    if (!options)
        return E_NOTIMPL;

    optFlags = options->dwFlags;

    hr = GetThemeFont(hTheme, hdc, iPartId, iStateId, TMT_FONT, &logfont);
    if(SUCCEEDED(hr)) 
    {
        hFont = CreateFontIndirectW(&logfont);
        if(!hFont)
        {
            ERR("Failed to create font\n");
        }
    }

    CopyRect(&rt, pRect);
    if(hFont)
        oldFont = SelectObject(hdc, hFont);

    oldBkMode = SetBkMode(hdc, TRANSPARENT);

    if (optFlags & DTT_TEXTCOLOR)
    {
        textColor = options->crText;
    }
    else
    {
        int textColorProp = TMT_TEXTCOLOR;
        if (optFlags & DTT_COLORPROP)
            textColorProp = options->iColorPropId;

        if (FAILED(GetThemeColor(hTheme, iPartId, iStateId, textColorProp, &textColor)))
            textColor = GetTextColor(hdc);
    }

    hr = GetThemeEnumValue(hTheme, iPartId, iStateId, TMT_TEXTSHADOWTYPE, &iShadowType);
    if (SUCCEEDED(hr))
    {
        hr = GetThemeColor(hTheme, iPartId, iStateId, TMT_TEXTSHADOWCOLOR, &shadowColor);
        if (FAILED(hr))
        {
            ERR("GetThemeColor failed\n");
        }

        hr = GetThemePosition(hTheme, iPartId, iStateId, TMT_TEXTSHADOWOFFSET, &ptShadowOffset);
        if (FAILED(hr))
        {
            ERR("GetThemePosition failed\n");
        }

        if (iShadowType == TST_SINGLE)
        {
            oldTextColor = SetTextColor(hdc, shadowColor);
            OffsetRect(&rt, ptShadowOffset.x, ptShadowOffset.y);
            DrawTextW(hdc, pszText, iCharCount, &rt, dwTextFlags);
            OffsetRect(&rt, -ptShadowOffset.x, -ptShadowOffset.y);
            SetTextColor(hdc, oldTextColor);
        }
        else if (iShadowType == TST_CONTINUOUS)
        {
            HANDLE hcomctl32 = GetModuleHandleW(L"comctl32.dll");
            DRAWSHADOWTEXT pDrawShadowText;
            if (!hcomctl32)
            {
                hcomctl32 = LoadLibraryW(L"comctl32.dll");
                if (!hcomctl32)
                    ERR("Failed to load comctl32\n");
            }

            pDrawShadowText = (DRAWSHADOWTEXT)GetProcAddress(hcomctl32, "DrawShadowText");
            if (pDrawShadowText)
            {
                pDrawShadowText(hdc, pszText, iCharCount, &rt, dwTextFlags, textColor, shadowColor, ptShadowOffset.x, ptShadowOffset.y);
                goto cleanup;
            }
        }
    }

    oldTextColor = SetTextColor(hdc, textColor);
    DrawTextW(hdc, pszText, iCharCount, &rt, dwTextFlags);
    SetTextColor(hdc, oldTextColor);
cleanup:
    SetBkMode(hdc, oldBkMode);

    if(hFont) {
        SelectObject(hdc, oldFont);
        DeleteObject(hFont);
    }
    return S_OK;
}
/***********************************************************************
 *      GetThemeTransitionDuration                          (UXTHEME.@)
 */
HRESULT WINAPI GetThemeTransitionDuration(HTHEME hTheme, int iPartId, int iStateIdFrom,
                                          int iStateIdTo, int iPropId, DWORD *pdwDuration)
{
    INTLIST intlist;
    HRESULT hr;

    TRACE("(%p, %d, %d, %d, %d, %p)\n", hTheme, iPartId, iStateIdFrom, iStateIdTo, iPropId,
          pdwDuration);

    if (!pdwDuration || iStateIdFrom < 1 || iStateIdTo < 1)
        return E_INVALIDARG;

    hr = GetThemeIntList(hTheme, iPartId, 0, iPropId, &intlist);
    if (FAILED(hr))
    {
        if (hr == E_PROP_ID_UNSUPPORTED)
            *pdwDuration = 0;

        return hr;
    }

    if (intlist.iValueCount < 1 || iStateIdFrom > intlist.iValues[0]
        || iStateIdTo > intlist.iValues[0]
        || intlist.iValueCount != 1 + intlist.iValues[0] * intlist.iValues[0])
    {
        *pdwDuration = 0;
        return E_INVALIDARG;
    }

    *pdwDuration = intlist.iValues[1 + intlist.iValues[0] * (iStateIdFrom - 1) + (iStateIdTo - 1)];
    return S_OK;
}

BOOL WINAPI IsCompositionActive(void)
{
    // No DWM until we move to Vista+
    return FALSE;
}

/***********************************************************************
 *      SetWindowThemeAttribute                             (UXTHEME.@)
 */
HRESULT
WINAPI
SetWindowThemeAttribute(
    _In_ HWND hwnd,
    _In_ enum WINDOWTHEMEATTRIBUTETYPE eAttribute,
    _In_ PVOID pvAttribute,
    _In_ DWORD cbAttribute)
{
   FIXME("(%p,%d,%p,%ld): stub\n", hwnd, eAttribute, pvAttribute, cbAttribute);
   return E_NOTIMPL;
}

/***********************************************************************
 *      OpenThemeDataForDpi                                 (UXTHEME.@)
 */
HTHEME
WINAPI
OpenThemeDataForDpi(
    _In_ HWND hwnd,
    _In_ LPCWSTR pszClassList,
    _In_ UINT dpi)
{
    FIXME("dpi (%x) is currently ignored", dpi);
    return OpenThemeData(hwnd, pszClassList);
}