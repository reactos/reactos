/*
 *  Notepad (settings.c)
 *
 *  Copyright 1998,99 Marcel Baur <mbaur@g26.ethz.ch>
 *  Copyright 2002 Sylvain Petreolle <spetreolle@yahoo.fr>
 *  Copyright 2002 Andriy Palamarchuk
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "notepad.h"

#include <winreg.h>

static LPCTSTR s_szRegistryKey = _T("Software\\Microsoft\\Notepad");


static LONG HeightFromPointSize(DWORD dwPointSize)
{
    LONG lHeight;
    HDC hDC;

    hDC = GetDC(NULL);
    lHeight = -MulDiv(dwPointSize, GetDeviceCaps(hDC, LOGPIXELSY), 720);
    ReleaseDC(NULL, hDC);

    return lHeight;
}

static DWORD PointSizeFromHeight(LONG lHeight)
{
    DWORD dwPointSize;
    HDC hDC;

    hDC = GetDC(NULL);
    dwPointSize = -MulDiv(lHeight, 720, GetDeviceCaps(hDC, LOGPIXELSY));
    ReleaseDC(NULL, hDC);

    /* round to nearest multiple of 10 */
    dwPointSize += 5;
    dwPointSize -= dwPointSize % 10;

    return dwPointSize;
}

static BOOL
QueryGeneric(HKEY hKey, LPCTSTR pszValueNameT, DWORD dwExpectedType,
             LPVOID pvResult, DWORD dwResultSize)
{
    DWORD dwType, cbData;
    LPVOID *pTemp = _alloca(dwResultSize);

    ZeroMemory(pTemp, dwResultSize);

    cbData = dwResultSize;
    if (RegQueryValueEx(hKey, pszValueNameT, NULL, &dwType, (LPBYTE) pTemp, &cbData) != ERROR_SUCCESS)
        return FALSE;

    if (dwType != dwExpectedType)
        return FALSE;

    memcpy(pvResult, pTemp, cbData);
    return TRUE;
}

static BOOL QueryDword(HKEY hKey, LPCTSTR pszValueName, DWORD *pdwResult)
{
    return QueryGeneric(hKey, pszValueName, REG_DWORD, pdwResult, sizeof(*pdwResult));
}

static BOOL QueryByte(HKEY hKey, LPCTSTR pszValueName, BYTE *pbResult)
{
    DWORD dwResult;
    if (!QueryGeneric(hKey, pszValueName, REG_DWORD, &dwResult, sizeof(dwResult)))
        return FALSE;
    if (dwResult >= 0x100)
        return FALSE;
    *pbResult = (BYTE) dwResult;
    return TRUE;
}

static BOOL QueryBool(HKEY hKey, LPCTSTR pszValueName, BOOL *pbResult)
{
    DWORD dwResult;
    if (!QueryDword(hKey, pszValueName, &dwResult))
        return FALSE;
    *pbResult = dwResult ? TRUE : FALSE;
    return TRUE;
}

static BOOL QueryString(HKEY hKey, LPCTSTR pszValueName, LPTSTR pszResult, DWORD dwResultLength)
{
    if (dwResultLength == 0)
        return FALSE;
    if (!QueryGeneric(hKey, pszValueName, REG_SZ, pszResult, dwResultLength * sizeof(TCHAR)))
        return FALSE;
    pszResult[dwResultLength - 1] = 0; /* Avoid buffer overrun */
    return TRUE;
}

/***********************************************************************
 *
 *           NOTEPAD_LoadSettingsFromRegistry
 *
 *  Load settings from registry HKCU\Software\Microsoft\Notepad.
 */
void NOTEPAD_LoadSettingsFromRegistry(void)
{
    HKEY hKey;
    HFONT hFont;
    DWORD dwPointSize, cx, cy;
    DWORD cxScreen = GetSystemMetrics(SM_CXSCREEN), cyScreen = GetSystemMetrics(SM_CYSCREEN);

    /* Set the default values */
    Globals.bShowStatusBar = TRUE;
    Globals.bWrapLongLines = FALSE;
    SetRect(&Globals.lMargins, 750, 1000, 750, 1000);
    ZeroMemory(&Globals.lfFont, sizeof(Globals.lfFont));
    Globals.lfFont.lfCharSet = DEFAULT_CHARSET;
    dwPointSize = 100;
    Globals.lfFont.lfWeight = FW_NORMAL;
    Globals.lfFont.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
    Globals.main_rect.left = CW_USEDEFAULT;
    Globals.main_rect.top = CW_USEDEFAULT;
    cx = min((cxScreen * 3) / 4, 640);
    cy = min((cyScreen * 3) / 4, 480);

    /* FIXME: Globals.fSaveWindowPositions = FALSE; */
    /* FIXME: Globals.fMLE_is_broken = FALSE; */

    /* Open the target registry key */
    if (RegOpenKey(HKEY_CURRENT_USER, s_szRegistryKey, &hKey) != ERROR_SUCCESS)
        hKey = NULL;

    /* Load the values from registry */
    if (hKey)
    {
        QueryByte(hKey, _T("lfCharSet"), &Globals.lfFont.lfCharSet);
        QueryByte(hKey, _T("lfClipPrecision"), &Globals.lfFont.lfClipPrecision);
        QueryDword(hKey, _T("lfEscapement"), (DWORD*)&Globals.lfFont.lfEscapement);
        QueryByte(hKey, _T("lfItalic"), &Globals.lfFont.lfItalic);
        QueryDword(hKey, _T("lfOrientation"), (DWORD*)&Globals.lfFont.lfOrientation);
        QueryByte(hKey, _T("lfOutPrecision"), &Globals.lfFont.lfOutPrecision);
        QueryByte(hKey, _T("lfPitchAndFamily"), &Globals.lfFont.lfPitchAndFamily);
        QueryByte(hKey, _T("lfQuality"), &Globals.lfFont.lfQuality);
        QueryByte(hKey, _T("lfStrikeOut"), &Globals.lfFont.lfStrikeOut);
        QueryByte(hKey, _T("lfUnderline"), &Globals.lfFont.lfUnderline);
        QueryDword(hKey, _T("lfWeight"), (DWORD*)&Globals.lfFont.lfWeight);
        QueryDword(hKey, _T("iPointSize"), &dwPointSize);

        QueryBool(hKey, _T("fWrap"), &Globals.bWrapLongLines);
        QueryBool(hKey, _T("fStatusBar"), &Globals.bShowStatusBar);

        QueryDword(hKey, _T("iMarginLeft"), (DWORD*)&Globals.lMargins.left);
        QueryDword(hKey, _T("iMarginTop"), (DWORD*)&Globals.lMargins.top);
        QueryDword(hKey, _T("iMarginRight"), (DWORD*)&Globals.lMargins.right);
        QueryDword(hKey, _T("iMarginBottom"), (DWORD*)&Globals.lMargins.bottom);

        QueryDword(hKey, _T("iWindowPosX"), (DWORD*)&Globals.main_rect.left);
        QueryDword(hKey, _T("iWindowPosY"), (DWORD*)&Globals.main_rect.top);
        QueryDword(hKey, _T("iWindowPosDX"), &cx);
        QueryDword(hKey, _T("iWindowPosDY"), &cy);
    }

    Globals.lfFont.lfHeight = HeightFromPointSize(dwPointSize);
    Globals.main_rect.right = Globals.main_rect.left + cx;
    Globals.main_rect.bottom = Globals.main_rect.top + cy;

    if (!hKey || !QueryString(hKey, _T("lfFaceName"),
                              Globals.lfFont.lfFaceName, ARRAY_SIZE(Globals.lfFont.lfFaceName)))
    {
        LoadString(Globals.hInstance, STRING_DEFAULTFONT, Globals.lfFont.lfFaceName,
                   ARRAY_SIZE(Globals.lfFont.lfFaceName));
    }

    if (!hKey || !QueryString(hKey, _T("szHeader"), Globals.szHeader, ARRAY_SIZE(Globals.szHeader)))
    {
        LoadString(Globals.hInstance, STRING_PAGESETUP_HEADERVALUE, Globals.szHeader,
                   ARRAY_SIZE(Globals.szHeader));
    }

    if (!hKey || !QueryString(hKey, _T("szTrailer"), Globals.szFooter, ARRAY_SIZE(Globals.szFooter)))
    {
        LoadString(Globals.hInstance, STRING_PAGESETUP_FOOTERVALUE, Globals.szFooter,
                   ARRAY_SIZE(Globals.szFooter));
    }

    if (hKey)
        RegCloseKey(hKey);

    /* WORKAROUND: Far East Asian users may not have suitable fixed-pitch fonts. */
    switch (PRIMARYLANGID(GetUserDefaultLangID()))
    {
        case LANG_CHINESE:
        case LANG_JAPANESE:
        case LANG_KOREAN:
            Globals.lfFont.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
            break;
    }

    hFont = CreateFontIndirect(&Globals.lfFont);
    SendMessage(Globals.hEdit, WM_SETFONT, (WPARAM)hFont, (LPARAM)TRUE);
    if (hFont)
    {
        if (Globals.hFont)
            DeleteObject(Globals.hFont);
        Globals.hFont = hFont;
    }
}

static BOOL SaveDword(HKEY hKey, LPCTSTR pszValueNameT, DWORD dwValue)
{
    return RegSetValueEx(hKey, pszValueNameT, 0, REG_DWORD, (LPBYTE) &dwValue, sizeof(dwValue)) == ERROR_SUCCESS;
}

static BOOL SaveString(HKEY hKey, LPCTSTR pszValueNameT, LPCTSTR pszValue)
{
    return RegSetValueEx(hKey, pszValueNameT, 0, REG_SZ, (LPBYTE) pszValue, (DWORD) _tcslen(pszValue) * sizeof(*pszValue)) == ERROR_SUCCESS;
}

/***********************************************************************
 *
 *           NOTEPAD_SaveSettingsToRegistry
 *
 *  Save settings to registry HKCU\Software\Microsoft\Notepad.
 */
void NOTEPAD_SaveSettingsToRegistry(void)
{
    HKEY hKey;
    DWORD dwDisposition;

    GetWindowRect(Globals.hMainWnd, &Globals.main_rect);

    if (RegCreateKeyEx(HKEY_CURRENT_USER, s_szRegistryKey,
                       0, NULL, 0, KEY_SET_VALUE, NULL,
                       &hKey, &dwDisposition) == ERROR_SUCCESS)
    {
        SaveDword(hKey, _T("lfCharSet"), Globals.lfFont.lfCharSet);
        SaveDword(hKey, _T("lfClipPrecision"), Globals.lfFont.lfClipPrecision);
        SaveDword(hKey, _T("lfEscapement"), Globals.lfFont.lfEscapement);
        SaveString(hKey, _T("lfFaceName"), Globals.lfFont.lfFaceName);
        SaveDword(hKey, _T("lfItalic"), Globals.lfFont.lfItalic);
        SaveDword(hKey, _T("lfOrientation"), Globals.lfFont.lfOrientation);
        SaveDword(hKey, _T("lfOutPrecision"), Globals.lfFont.lfOutPrecision);
        SaveDword(hKey, _T("lfPitchAndFamily"), Globals.lfFont.lfPitchAndFamily);
        SaveDword(hKey, _T("lfQuality"), Globals.lfFont.lfQuality);
        SaveDword(hKey, _T("lfStrikeOut"), Globals.lfFont.lfStrikeOut);
        SaveDword(hKey, _T("lfUnderline"), Globals.lfFont.lfUnderline);
        SaveDword(hKey, _T("lfWeight"), Globals.lfFont.lfWeight);
        SaveDword(hKey, _T("iPointSize"), PointSizeFromHeight(Globals.lfFont.lfHeight));
        SaveDword(hKey, _T("fWrap"), Globals.bWrapLongLines ? 1 : 0);
        SaveDword(hKey, _T("fStatusBar"), Globals.bShowStatusBar ? 1 : 0);
        SaveString(hKey, _T("szHeader"), Globals.szHeader);
        SaveString(hKey, _T("szTrailer"), Globals.szFooter);
        SaveDword(hKey, _T("iMarginLeft"), Globals.lMargins.left);
        SaveDword(hKey, _T("iMarginTop"), Globals.lMargins.top);
        SaveDword(hKey, _T("iMarginRight"), Globals.lMargins.right);
        SaveDword(hKey, _T("iMarginBottom"), Globals.lMargins.bottom);
        SaveDword(hKey, _T("iWindowPosX"), Globals.main_rect.left);
        SaveDword(hKey, _T("iWindowPosY"), Globals.main_rect.top);
        SaveDword(hKey, _T("iWindowPosDX"), Globals.main_rect.right - Globals.main_rect.left);
        SaveDword(hKey, _T("iWindowPosDY"), Globals.main_rect.bottom - Globals.main_rect.top);

        RegCloseKey(hKey);
    }
}
