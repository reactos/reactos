/*
 * ReactOS Calc (HtmlHelp support)
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
    calc_##name = (type_##name)GetProcAddress(hHtmlHelp, #name); \
    if (calc_##name == NULL) calc_##name = dummy_##name;

static HWND WINAPI
dummy_HtmlHelpA(HWND hWnd, LPCSTR pszFile, UINT uCommand, DWORD dwData);

static HWND WINAPI
dummy_HtmlHelpW(HWND hWnd, LPCWSTR pszFile, UINT uCommand, DWORD dwData);

type_HtmlHelpA calc_HtmlHelpA = dummy_HtmlHelpA;
type_HtmlHelpW calc_HtmlHelpW = dummy_HtmlHelpW;

static HMODULE hHtmlHelp;

static HWND WINAPI
dummy_HtmlHelpA(HWND hWnd, LPCSTR pszFile, UINT uCommand, DWORD dwData)
{
    return NULL;
}

static HWND WINAPI
dummy_HtmlHelpW(HWND hWnd, LPCWSTR pszFile, UINT uCommand, DWORD dwData)
{
    return NULL;
}

void HtmlHelp_Start(HINSTANCE hInstance)
{
    hHtmlHelp = LoadLibrary(_T("HHCTRL.OCX"));
    if (hHtmlHelp == NULL)
        return;

    GET_CB(HtmlHelpW)
    GET_CB(HtmlHelpA)
}

void HtmlHelp_Stop(void)
{
    if(hHtmlHelp == NULL)
        return;

    FreeLibrary(hHtmlHelp);
    hHtmlHelp = NULL;
}
