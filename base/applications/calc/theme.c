/*
 * ReactOS Calc (Theming support)
 *
 * Copyright 2007-2017, Carlo Bramini
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "calc.h"

#define GET_CB(name) \
    calc_##name = (type_##name)GetProcAddress(hUxTheme, #name); \
    if (calc_##name == NULL) calc_##name = dummy_##name;

static HTHEME WINAPI
dummy_OpenThemeData(HWND hwnd, const WCHAR *pszClassList);

static HRESULT WINAPI
dummy_CloseThemeData(HTHEME hTheme);

static HRESULT WINAPI
dummy_DrawThemeBackground(HTHEME hTheme, HDC hdc, int iPartId, int iStateId,
    const RECT *prc, const RECT *prcClip);

static BOOL WINAPI
dummy_IsAppThemed(void);

static BOOL WINAPI
dummy_IsThemeActive(void);

static BOOL WINAPI
dummy_IsThemeBackgroundPartiallyTransparent(HTHEME hTheme, int iPartId, int iStateId);

static HRESULT WINAPI
dummy_DrawThemeParentBackground(HWND hWnd, HDC hdc, RECT *prc);


type_OpenThemeData       calc_OpenThemeData       = dummy_OpenThemeData;
type_CloseThemeData      calc_CloseThemeData      = dummy_CloseThemeData;
type_DrawThemeBackground calc_DrawThemeBackground = dummy_DrawThemeBackground;
type_IsAppThemed         calc_IsAppThemed         = dummy_IsAppThemed;
type_IsThemeActive       calc_IsThemeActive       = dummy_IsThemeActive;
type_IsThemeBackgroundPartiallyTransparent calc_IsThemeBackgroundPartiallyTransparent = \
    dummy_IsThemeBackgroundPartiallyTransparent;
type_DrawThemeParentBackground calc_DrawThemeParentBackground = \
    dummy_DrawThemeParentBackground;

static HMODULE hUxTheme;

static HTHEME WINAPI
dummy_OpenThemeData(HWND hwnd, const WCHAR* pszClassList)
{
    return NULL;
}

static HRESULT WINAPI
dummy_CloseThemeData(HTHEME hTheme)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI
dummy_DrawThemeBackground(HTHEME hTheme, HDC hdc, int iPartId, int iStateId,
            const RECT* prc, const RECT* prcClip)
{
    return E_NOTIMPL;
}

static BOOL WINAPI
dummy_IsAppThemed(void)
{
    return FALSE;
}

static BOOL WINAPI
dummy_IsThemeActive(void)
{
    return FALSE;
}

static BOOL WINAPI
dummy_IsThemeBackgroundPartiallyTransparent(HTHEME hTheme, int iPartId, int iStateId)
{
    return FALSE;
}

static HRESULT WINAPI
dummy_DrawThemeParentBackground(HWND hWnd, HDC hdc, RECT *prc)
{
    return E_NOTIMPL;
}

void Theme_Start(HINSTANCE hInstance)
{
    hUxTheme = LoadLibrary(_T("UXTHEME"));
    if (hUxTheme == NULL)
        return;

    GET_CB(OpenThemeData)
    GET_CB(CloseThemeData)
    GET_CB(DrawThemeBackground)
    GET_CB(IsAppThemed)
    GET_CB(IsThemeActive)
    GET_CB(IsThemeBackgroundPartiallyTransparent)
    GET_CB(DrawThemeParentBackground)
}

void Theme_Stop(void)
{
    if(hUxTheme == NULL)
        return;

    FreeLibrary(hUxTheme);
    hUxTheme = NULL;
}
