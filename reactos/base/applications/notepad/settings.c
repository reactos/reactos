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

static BOOL QueryString(HKEY hKey, LPCTSTR pszValueName, LPTSTR pszResult, DWORD dwResultSize)
{
    return QueryGeneric(hKey, pszValueName, REG_SZ, pszResult, dwResultSize * sizeof(TCHAR));
}

void LoadSettings(void)
{
    HKEY hKey = NULL;
    HFONT hFont;
    DWORD dwPointSize = 0;
    INT base_length, dx, dy;

    base_length = (GetSystemMetrics(SM_CXSCREEN) > GetSystemMetrics(SM_CYSCREEN)) ?
                  GetSystemMetrics(SM_CYSCREEN) : GetSystemMetrics(SM_CXSCREEN);

    dx = (INT)(base_length * .95);
    dy = dx * 3 / 4;
    SetRect(&Globals.main_rect, 0, 0, dx, dy);

    if (RegOpenKey(HKEY_CURRENT_USER, s_szRegistryKey, &hKey) == ERROR_SUCCESS)
    {
        QueryByte(hKey, _T("lfCharSet"), &Globals.lfFont.lfCharSet);
        QueryByte(hKey, _T("lfClipPrecision"), &Globals.lfFont.lfClipPrecision);
        QueryDword(hKey, _T("lfEscapement"), (DWORD*)&Globals.lfFont.lfEscapement);
        QueryString(hKey, _T("lfFaceName"), Globals.lfFont.lfFaceName, sizeof(Globals.lfFont.lfFaceName) / sizeof(Globals.lfFont.lfFaceName[0]));
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
        QueryString(hKey, _T("szHeader"), Globals.szHeader, sizeof(Globals.szHeader) / sizeof(Globals.szHeader[0]));
        QueryString(hKey, _T("szTrailer"), Globals.szFooter, sizeof(Globals.szFooter) / sizeof(Globals.szFooter[0]));
        QueryDword(hKey, _T("iMarginLeft"), (DWORD*)&Globals.lMarginLeft);
        QueryDword(hKey, _T("iMarginTop"), (DWORD*)&Globals.lMarginTop);
        QueryDword(hKey, _T("iMarginRight"), (DWORD*)&Globals.lMarginRight);
        QueryDword(hKey, _T("iMarginBottom"), (DWORD*)&Globals.lMarginBottom);

        QueryDword(hKey, _T("iWindowPosX"), (DWORD*)&Globals.main_rect.left);
        QueryDword(hKey, _T("iWindowPosY"), (DWORD*)&Globals.main_rect.top);
        QueryDword(hKey, _T("iWindowPosDX"), (DWORD*)&dx);
        QueryDword(hKey, _T("iWindowPosDY"), (DWORD*)&dy);

        Globals.main_rect.right = Globals.main_rect.left + dx;
        Globals.main_rect.bottom = Globals.main_rect.top + dy;

        /* invert value because DIALOG_ViewStatusBar will be called to show it */
        Globals.bShowStatusBar = !Globals.bShowStatusBar;

        if (dwPointSize != 0)
            Globals.lfFont.lfHeight = HeightFromPointSize(dwPointSize);

        RegCloseKey(hKey);
    }

    hFont = CreateFontIndirect(&Globals.lfFont);
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

void SaveSettings(void)
{
    HKEY hKey;
    DWORD dwDisposition;

    GetWindowRect(Globals.hMainWnd, &Globals.main_rect);

    if (RegCreateKeyEx(HKEY_CURRENT_USER, s_szRegistryKey, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition)
        == ERROR_SUCCESS)
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
        SaveDword(hKey, _T("iMarginLeft"), Globals.lMarginLeft);
        SaveDword(hKey, _T("iMarginTop"), Globals.lMarginTop);
        SaveDword(hKey, _T("iMarginRight"), Globals.lMarginRight);
        SaveDword(hKey, _T("iMarginBottom"), Globals.lMarginBottom);
        SaveDword(hKey, _T("iWindowPosX"), Globals.main_rect.left);
        SaveDword(hKey, _T("iWindowPosY"), Globals.main_rect.top);
        SaveDword(hKey, _T("iWindowPosDX"), Globals.main_rect.right - Globals.main_rect.left);
        SaveDword(hKey, _T("iWindowPosDY"), Globals.main_rect.bottom - Globals.main_rect.top);
        RegCloseKey(hKey);
    }
}
