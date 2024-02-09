/*
 * PROJECT:    PAINT for ReactOS
 * LICENSE:    LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:    Offering functions dealing with registry values
 * COPYRIGHT:  Copyright 2015 Benedikt Freisen <b.freisen@gmx.net>
 *             Copyright 2020 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"
#include <winreg.h>
#include <wincon.h>
#include <shlobj.h>

RegistrySettings registrySettings;

/* FUNCTIONS ********************************************************/

static void ReadDWORD(CRegKey &key, LPCWSTR lpName, DWORD &dwValue)
{
    DWORD dwTemp;
    if (key.QueryDWORDValue(lpName, dwTemp) == ERROR_SUCCESS)
        dwValue = dwTemp;
}

static void ReadString(CRegKey &key, LPCWSTR lpName, CStringW &strValue, LPCWSTR lpDefault = L"")
{
    CStringW strTemp;
    ULONG nChars = MAX_PATH;
    LPWSTR psz = strTemp.GetBuffer(nChars);
    LONG error = key.QueryStringValue(lpName, psz, &nChars);
    strTemp.ReleaseBuffer();

    if (error == ERROR_SUCCESS)
        strValue = strTemp;
    else
        strValue = lpDefault;
}

void RegistrySettings::SetWallpaper(LPCWSTR szFileName, RegistrySettings::WallpaperStyle style)
{
    CRegKey desktop;
    if (desktop.Open(HKEY_CURRENT_USER, L"Control Panel\\Desktop") == ERROR_SUCCESS)
    {
        desktop.SetStringValue(L"Wallpaper", szFileName);

        desktop.SetStringValue(L"WallpaperStyle", (style == RegistrySettings::STRETCHED) ? L"2" : L"0");
        desktop.SetStringValue(L"TileWallpaper", (style == RegistrySettings::TILED) ? L"1" : L"0");
    }

    SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, (PVOID) szFileName, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
}

void RegistrySettings::LoadPresets(INT nCmdShow)
{
    BMPHeight = GetSystemMetrics(SM_CYSCREEN) / 2;
    BMPWidth = GetSystemMetrics(SM_CXSCREEN) / 2;
    GridExtent = 1;
    NoStretching = 0;
    ShowThumbnail = 0;
    SnapToGrid = 0;
    ThumbHeight = 100;
    ThumbWidth = 120;
    ThumbXPos = 180;
    ThumbYPos = 200;
    UnitSetting = 0;
    Bold = FALSE;
    Italic = FALSE;
    Underline = FALSE;
    CharSet = DEFAULT_CHARSET;
    PointSize = 14;
    FontsPositionX = 0;
    FontsPositionY = 0;
    ShowTextTool = TRUE;
    ShowStatusBar = TRUE;
    ShowPalette = TRUE;
    ShowToolBox = TRUE;
    Bar1ID = BAR1ID_TOP;
    Bar2ID = BAR2ID_LEFT;

    LOGFONTW lf;
    ::GetObjectW(GetStockObject(DEFAULT_GUI_FONT), sizeof(lf), &lf);
    strFontName = lf.lfFaceName;

    ZeroMemory(&WindowPlacement, sizeof(WindowPlacement));
    RECT& rc = WindowPlacement.rcNormalPosition;
    rc.left = rc.top = CW_USEDEFAULT;
    rc.right = rc.left + 544;
    rc.bottom = rc.top + 375;
    WindowPlacement.showCmd = nCmdShow;
}

void RegistrySettings::Load(INT nCmdShow)
{
    LoadPresets(nCmdShow);

    CRegKey paint;
    if (paint.Open(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Applets\\Paint", KEY_READ) != ERROR_SUCCESS)
        return;

    CRegKey view;
    if (view.Open(paint, L"View", KEY_READ) == ERROR_SUCCESS)
    {
        ReadDWORD(view, L"BMPHeight",     BMPHeight);
        ReadDWORD(view, L"BMPWidth",      BMPWidth);
        ReadDWORD(view, L"GridExtent",    GridExtent);
        ReadDWORD(view, L"NoStretching",  NoStretching);
        ReadDWORD(view, L"ShowThumbnail", ShowThumbnail);
        ReadDWORD(view, L"SnapToGrid",    SnapToGrid);
        ReadDWORD(view, L"ThumbHeight",   ThumbHeight);
        ReadDWORD(view, L"ThumbWidth",    ThumbWidth);
        ReadDWORD(view, L"ThumbXPos",     ThumbXPos);
        ReadDWORD(view, L"ThumbYPos",     ThumbYPos);
        ReadDWORD(view, L"UnitSetting",   UnitSetting);
        ReadDWORD(view, L"ShowStatusBar", ShowStatusBar);

        ULONG pnBytes = sizeof(WINDOWPLACEMENT);
        view.QueryBinaryValue(L"WindowPlacement", &WindowPlacement, &pnBytes);
    }

    CRegKey files;
    if (files.Open(paint, L"Recent File List", KEY_READ) == ERROR_SUCCESS)
    {
        WCHAR szName[64];
        for (INT i = 0; i < MAX_RECENT_FILES; ++i)
        {
            StringCchPrintfW(szName, _countof(szName), L"File%u", i + 1);
            ReadString(files, szName, strFiles[i]);
        }
    }

    CRegKey text;
    if (text.Open(paint, L"Text", KEY_READ) == ERROR_SUCCESS)
    {
        ReadDWORD(text, L"Bold",         Bold);
        ReadDWORD(text, L"Italic",       Italic);
        ReadDWORD(text, L"Underline",    Underline);
        ReadDWORD(text, L"CharSet",      CharSet);
        ReadDWORD(text, L"PointSize",    PointSize);
        ReadDWORD(text, L"PositionX",    FontsPositionX);
        ReadDWORD(text, L"PositionY",    FontsPositionY);
        ReadDWORD(text, L"ShowTextTool", ShowTextTool);
        ReadString(text, L"TypeFaceName", strFontName, strFontName);
    }

    CRegKey bar1;
    if (bar1.Open(paint, L"General-Bar1", KEY_READ) == ERROR_SUCCESS)
    {
        ReadDWORD(bar1, L"BarID", Bar1ID);
    }

    CRegKey bar2;
    if (bar2.Open(paint, L"General-Bar2", KEY_READ) == ERROR_SUCCESS)
    {
        ReadDWORD(bar2, L"BarID", Bar2ID);
    }

    CRegKey bar3;
    if (bar3.Open(paint, L"General-Bar3", KEY_READ) == ERROR_SUCCESS)
    {
        ReadDWORD(bar3, L"Visible", ShowToolBox);
    }

    CRegKey bar4;
    if (bar4.Open(paint, L"General-Bar4", KEY_READ) == ERROR_SUCCESS)
    {
        ReadDWORD(bar4, L"Visible", ShowPalette);
    }

    // Fix the bitmap size if too large
    if (BMPWidth > 5000)
        BMPWidth = (GetSystemMetrics(SM_CXSCREEN) * 6) / 10;
    if (BMPHeight > 5000)
        BMPHeight = (GetSystemMetrics(SM_CYSCREEN) * 6) / 10;
}

void RegistrySettings::Store()
{
    BMPWidth = imageModel.GetWidth();
    BMPHeight = imageModel.GetHeight();

    CRegKey paint;
    if (paint.Create(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Applets\\Paint") != ERROR_SUCCESS)
        return;

    CRegKey view;
    if (view.Create(paint, L"View") == ERROR_SUCCESS)
    {
        view.SetDWORDValue(L"BMPHeight",     BMPHeight);
        view.SetDWORDValue(L"BMPWidth",      BMPWidth);
        view.SetDWORDValue(L"GridExtent",    GridExtent);
        view.SetDWORDValue(L"NoStretching",  NoStretching);
        view.SetDWORDValue(L"ShowThumbnail", ShowThumbnail);
        view.SetDWORDValue(L"SnapToGrid",    SnapToGrid);
        view.SetDWORDValue(L"ThumbHeight",   ThumbHeight);
        view.SetDWORDValue(L"ThumbWidth",    ThumbWidth);
        view.SetDWORDValue(L"ThumbXPos",     ThumbXPos);
        view.SetDWORDValue(L"ThumbYPos",     ThumbYPos);
        view.SetDWORDValue(L"UnitSetting",   UnitSetting);
        view.SetDWORDValue(L"ShowStatusBar", ShowStatusBar);

        view.SetBinaryValue(L"WindowPlacement", &WindowPlacement, sizeof(WINDOWPLACEMENT));
    }

    CRegKey files;
    if (files.Create(paint, L"Recent File List") == ERROR_SUCCESS)
    {
        WCHAR szName[64];
        for (INT iFile = 0; iFile < MAX_RECENT_FILES; ++iFile)
        {
            StringCchPrintfW(szName, _countof(szName), L"File%u", iFile + 1);
            files.SetStringValue(szName, strFiles[iFile]);
        }
    }

    CRegKey text;
    if (text.Create(paint, L"Text") == ERROR_SUCCESS)
    {
        text.SetDWORDValue(L"Bold",          Bold);
        text.SetDWORDValue(L"Italic",        Italic);
        text.SetDWORDValue(L"Underline",     Underline);
        text.SetDWORDValue(L"CharSet",       CharSet);
        text.SetDWORDValue(L"PointSize",     PointSize);
        text.SetDWORDValue(L"PositionX",     FontsPositionX);
        text.SetDWORDValue(L"PositionY",     FontsPositionY);
        text.SetDWORDValue(L"ShowTextTool",  ShowTextTool);
        text.SetStringValue(L"TypeFaceName", strFontName);
    }

    CRegKey bar1;
    if (bar1.Create(paint, L"General-Bar1") == ERROR_SUCCESS)
    {
        bar1.SetDWORDValue(L"BarID", Bar1ID);
    }

    CRegKey bar2;
    if (bar2.Create(paint, L"General-Bar2") == ERROR_SUCCESS)
    {
        bar2.SetDWORDValue(L"BarID", Bar2ID);
    }

    CRegKey bar3;
    if (bar3.Create(paint, L"General-Bar3") == ERROR_SUCCESS)
    {
        bar3.SetDWORDValue(L"Visible", ShowToolBox);
    }

    CRegKey bar4;
    if (bar4.Create(paint, L"General-Bar4") == ERROR_SUCCESS)
    {
        bar4.SetDWORDValue(L"Visible", ShowPalette);
    }
}

void RegistrySettings::SetMostRecentFile(LPCWSTR szPathName)
{
    // Register the file to the user's 'Recent' folder
    if (szPathName && szPathName[0])
        SHAddToRecentDocs(SHARD_PATHW, szPathName);

    // If szPathName is present in strFiles, move it to the top of the list
    for (INT i = MAX_RECENT_FILES - 1, iFound = -1; i > 0; --i)
    {
        if (iFound < 0 && strFiles[i].CompareNoCase(szPathName) == 0)
            iFound = i;

        if (iFound >= 0)
            Swap(strFiles[i], strFiles[i - 1]);
    }

    // If szPathName is not the first item in strFiles, insert it at the top of the list
    if (strFiles[0].CompareNoCase(szPathName) != 0)
    {
        for (INT i = MAX_RECENT_FILES - 1; i > 0; --i)
            strFiles[i] = strFiles[i - 1];

        strFiles[0] = szPathName;
    }
}
