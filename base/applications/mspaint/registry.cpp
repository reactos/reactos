/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/registry.cpp
 * PURPOSE:     Offering functions dealing with registry values
 * PROGRAMMERS: Benedikt Freisen
 *              Katayama Hirofumi MZ
 */

#include "precomp.h"
#include <winreg.h>
#include <wincon.h>
#include <shlobj.h>

RegistrySettings registrySettings;

/* FUNCTIONS ********************************************************/

static void ReadDWORD(CRegKey &key, LPCTSTR lpName, DWORD &dwValue)
{
    DWORD dwTemp;
    if (key.QueryDWORDValue(lpName, dwTemp) == ERROR_SUCCESS)
        dwValue = dwTemp;
}

static void ReadString(CRegKey &key, LPCTSTR lpName, CString &strValue, LPCTSTR lpDefault = TEXT(""))
{
    CString strTemp;
    ULONG nChars = MAX_PATH;
    LPTSTR psz = strTemp.GetBuffer(nChars);
    LONG error = key.QueryStringValue(lpName, psz, &nChars);
    strTemp.ReleaseBuffer();

    if (error == ERROR_SUCCESS)
        strValue = strTemp;
    else
        strValue = lpDefault;
}

void RegistrySettings::SetWallpaper(LPCTSTR szFileName, RegistrySettings::WallpaperStyle style)
{
    CRegKey desktop;
    if (desktop.Open(HKEY_CURRENT_USER, _T("Control Panel\\Desktop")) == ERROR_SUCCESS)
    {
        desktop.SetStringValue(_T("Wallpaper"), szFileName);

        desktop.SetStringValue(_T("WallpaperStyle"), (style == RegistrySettings::STRETCHED) ? _T("2") : _T("0"));
        desktop.SetStringValue(_T("TileWallpaper"), (style == RegistrySettings::TILED) ? _T("1") : _T("0"));
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

    LOGFONT lf;
    GetObject(GetStockObject(DEFAULT_GUI_FONT), sizeof(lf), &lf);
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
    if (paint.Open(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Applets\\Paint"), KEY_READ) != ERROR_SUCCESS)
        return;

    CRegKey view;
    if (view.Open(paint, _T("View"), KEY_READ) == ERROR_SUCCESS)
    {
        ReadDWORD(view, _T("BMPHeight"),     BMPHeight);
        ReadDWORD(view, _T("BMPWidth"),      BMPWidth);
        ReadDWORD(view, _T("GridExtent"),    GridExtent);
        ReadDWORD(view, _T("NoStretching"),  NoStretching);
        ReadDWORD(view, _T("ShowThumbnail"), ShowThumbnail);
        ReadDWORD(view, _T("SnapToGrid"),    SnapToGrid);
        ReadDWORD(view, _T("ThumbHeight"),   ThumbHeight);
        ReadDWORD(view, _T("ThumbWidth"),    ThumbWidth);
        ReadDWORD(view, _T("ThumbXPos"),     ThumbXPos);
        ReadDWORD(view, _T("ThumbYPos"),     ThumbYPos);
        ReadDWORD(view, _T("UnitSetting"),   UnitSetting);
        ReadDWORD(view, _T("ShowStatusBar"), ShowStatusBar);

        ULONG pnBytes = sizeof(WINDOWPLACEMENT);
        view.QueryBinaryValue(_T("WindowPlacement"), &WindowPlacement, &pnBytes);
    }

    CRegKey files;
    if (files.Open(paint, _T("Recent File List"), KEY_READ) == ERROR_SUCCESS)
    {
        TCHAR szName[64];
        for (INT i = 0; i < MAX_RECENT_FILES; ++i)
        {
            wsprintf(szName, _T("File%u"), i + 1);
            ReadString(files, szName, strFiles[i]);
        }
    }

    CRegKey text;
    if (text.Open(paint, _T("Text"), KEY_READ) == ERROR_SUCCESS)
    {
        ReadDWORD(text, _T("Bold"),         Bold);
        ReadDWORD(text, _T("Italic"),       Italic);
        ReadDWORD(text, _T("Underline"),    Underline);
        ReadDWORD(text, _T("CharSet"),      CharSet);
        ReadDWORD(text, _T("PointSize"),    PointSize);
        ReadDWORD(text, _T("PositionX"),    FontsPositionX);
        ReadDWORD(text, _T("PositionY"),    FontsPositionY);
        ReadDWORD(text, _T("ShowTextTool"), ShowTextTool);
        ReadString(text, _T("TypeFaceName"), strFontName, strFontName);
    }

    CRegKey bar1;
    if (bar1.Open(paint, _T("General-Bar1"), KEY_READ) == ERROR_SUCCESS)
    {
        ReadDWORD(bar1, _T("BarID"), Bar1ID);
    }

    CRegKey bar2;
    if (bar2.Open(paint, _T("General-Bar2"), KEY_READ) == ERROR_SUCCESS)
    {
        ReadDWORD(bar2, _T("BarID"), Bar2ID);
    }

    CRegKey bar3;
    if (bar3.Open(paint, _T("General-Bar3"), KEY_READ) == ERROR_SUCCESS)
    {
        ReadDWORD(bar3, _T("Visible"), ShowToolBox);
    }

    CRegKey bar4;
    if (bar4.Open(paint, _T("General-Bar4"), KEY_READ) == ERROR_SUCCESS)
    {
        ReadDWORD(bar4, _T("Visible"), ShowPalette);
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
    if (paint.Create(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Applets\\Paint")) != ERROR_SUCCESS)
        return;

    CRegKey view;
    if (view.Create(paint, _T("View")) == ERROR_SUCCESS)
    {
        view.SetDWORDValue(_T("BMPHeight"),     BMPHeight);
        view.SetDWORDValue(_T("BMPWidth"),      BMPWidth);
        view.SetDWORDValue(_T("GridExtent"),    GridExtent);
        view.SetDWORDValue(_T("NoStretching"),  NoStretching);
        view.SetDWORDValue(_T("ShowThumbnail"), ShowThumbnail);
        view.SetDWORDValue(_T("SnapToGrid"),    SnapToGrid);
        view.SetDWORDValue(_T("ThumbHeight"),   ThumbHeight);
        view.SetDWORDValue(_T("ThumbWidth"),    ThumbWidth);
        view.SetDWORDValue(_T("ThumbXPos"),     ThumbXPos);
        view.SetDWORDValue(_T("ThumbYPos"),     ThumbYPos);
        view.SetDWORDValue(_T("UnitSetting"),   UnitSetting);
        view.SetDWORDValue(_T("ShowStatusBar"), ShowStatusBar);

        view.SetBinaryValue(_T("WindowPlacement"), &WindowPlacement, sizeof(WINDOWPLACEMENT));
    }

    CRegKey files;
    if (files.Create(paint, _T("Recent File List")) == ERROR_SUCCESS)
    {
        TCHAR szName[64];
        for (INT iFile = 0; iFile < MAX_RECENT_FILES; ++iFile)
        {
            wsprintf(szName, _T("File%u"), iFile + 1);
            files.SetStringValue(szName, strFiles[iFile]);
        }
    }

    CRegKey text;
    if (text.Create(paint, _T("Text")) == ERROR_SUCCESS)
    {
        text.SetDWORDValue(_T("Bold"),          Bold);
        text.SetDWORDValue(_T("Italic"),        Italic);
        text.SetDWORDValue(_T("Underline"),     Underline);
        text.SetDWORDValue(_T("CharSet"),       CharSet);
        text.SetDWORDValue(_T("PointSize"),     PointSize);
        text.SetDWORDValue(_T("PositionX"),     FontsPositionX);
        text.SetDWORDValue(_T("PositionY"),     FontsPositionY);
        text.SetDWORDValue(_T("ShowTextTool"),  ShowTextTool);
        text.SetStringValue(_T("TypeFaceName"), strFontName);
    }

    CRegKey bar1;
    if (bar1.Create(paint, _T("General-Bar1")) == ERROR_SUCCESS)
    {
        bar1.SetDWORDValue(_T("BarID"), Bar1ID);
    }

    CRegKey bar2;
    if (bar2.Create(paint, _T("General-Bar2")) == ERROR_SUCCESS)
    {
        bar2.SetDWORDValue(_T("BarID"), Bar2ID);
    }

    CRegKey bar3;
    if (bar3.Create(paint, _T("General-Bar3")) == ERROR_SUCCESS)
    {
        bar3.SetDWORDValue(_T("Visible"), ShowToolBox);
    }

    CRegKey bar4;
    if (bar4.Create(paint, _T("General-Bar4")) == ERROR_SUCCESS)
    {
        bar4.SetDWORDValue(_T("Visible"), ShowPalette);
    }
}

void RegistrySettings::SetMostRecentFile(LPCTSTR szPathName)
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
        {
            CString tmp = strFiles[i];
            strFiles[i] = strFiles[i - 1];
            strFiles[i - 1] = tmp;
        }
    }

    // If szPathName is not the first item in strFiles, insert it at the top of the list
    if (strFiles[0].CompareNoCase(szPathName) != 0)
    {
        for (INT i = MAX_RECENT_FILES - 1; i > 0; --i)
            strFiles[i] = strFiles[i - 1];

        strFiles[0] = szPathName;
    }
}
