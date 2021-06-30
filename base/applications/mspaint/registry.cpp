/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/registry.cpp
 * PURPOSE:     Offering functions dealing with registry values
 * PROGRAMMERS: Benedikt Freisen
 *              Katayama Hirofumi MZ
 */

/* INCLUDES *********************************************************/

#include "precomp.h"
#include <winreg.h>
#include <wincon.h>
#include <shlobj.h>

/* FUNCTIONS ********************************************************/
static DWORD ReadDWORD(CRegKey &key, LPCTSTR lpName, DWORD &dwValue, BOOL bCheckForDef)
{
    DWORD dwPrev = dwValue;

    if (key.QueryDWORDValue(lpName, dwValue) != ERROR_SUCCESS || (bCheckForDef && dwValue == 0))
        dwValue = dwPrev;

    return dwPrev;
}

static void ReadFileHistory(CRegKey &key, LPCTSTR lpName, CString &strFile)
{
    ULONG nChars = MAX_PATH;
    LPTSTR szFile = strFile.GetBuffer(nChars);
    if (key.QueryStringValue(lpName, szFile, &nChars) != ERROR_SUCCESS)
        szFile[0] = '\0';
    strFile.ReleaseBuffer();
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

void RegistrySettings::LoadPresets()
{
    BMPHeight = 300;
    BMPWidth = 400;
    GridExtent = 1;
    NoStretching = 0;
    ShowThumbnail = 0;
    SnapToGrid = 0;
    ThumbHeight = 100;
    ThumbWidth = 120;
    ThumbXPos = 180;
    ThumbYPos = 200;
    UnitSetting = 0;
    const WINDOWPLACEMENT DefaultWindowPlacement = {
        sizeof(WINDOWPLACEMENT), 
        0,
        SW_SHOWNORMAL,
        {0, 0},
        {-1, -1},
        {100, 100, 700, 550}
    };
    WindowPlacement = DefaultWindowPlacement;
}

void RegistrySettings::Load()
{
    LoadPresets();

    CRegKey view;
    if (view.Open(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Applets\\Paint\\View"), KEY_READ) == ERROR_SUCCESS)
    {
        ReadDWORD(view, _T("BMPHeight"),     BMPHeight,     TRUE);
        ReadDWORD(view, _T("BMPWidth"),      BMPWidth,      TRUE);
        ReadDWORD(view, _T("GridExtent"),    GridExtent,    FALSE);
        ReadDWORD(view, _T("NoStretching"),  NoStretching,  FALSE);
        ReadDWORD(view, _T("ShowThumbnail"), ShowThumbnail, FALSE);
        ReadDWORD(view, _T("SnapToGrid"),    SnapToGrid,    FALSE);
        ReadDWORD(view, _T("ThumbHeight"),   ThumbHeight,   TRUE);
        ReadDWORD(view, _T("ThumbWidth"),    ThumbWidth,    TRUE);
        ReadDWORD(view, _T("ThumbXPos"),     ThumbXPos,     TRUE);
        ReadDWORD(view, _T("ThumbYPos"),     ThumbYPos,     TRUE);
        ReadDWORD(view, _T("UnitSetting"),   UnitSetting,   FALSE);

        ULONG pnBytes = sizeof(WINDOWPLACEMENT);
        view.QueryBinaryValue(_T("WindowPlacement"), &WindowPlacement, &pnBytes);
    }

    CRegKey files;
    if (files.Open(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Applets\\Paint\\Recent File List"), KEY_READ) == ERROR_SUCCESS)
    {
        ReadFileHistory(files, _T("File1"), strFile1);
        ReadFileHistory(files, _T("File2"), strFile2);
        ReadFileHistory(files, _T("File3"), strFile3);
        ReadFileHistory(files, _T("File4"), strFile4);
    }
}

void RegistrySettings::Store()
{
    CRegKey view;
    if (view.Create(HKEY_CURRENT_USER,
                     _T("Software\\Microsoft\\Windows\\CurrentVersion\\Applets\\Paint\\View")) == ERROR_SUCCESS)
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

        view.SetBinaryValue(_T("WindowPlacement"), &WindowPlacement, sizeof(WINDOWPLACEMENT));
    }

    CRegKey files;
    if (files.Create(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Applets\\Paint\\Recent File List")) == ERROR_SUCCESS)
    {
        if (!strFile1.IsEmpty())
            files.SetStringValue(_T("File1"), strFile1);
        if (!strFile2.IsEmpty())
            files.SetStringValue(_T("File2"), strFile2);
        if (!strFile3.IsEmpty())
            files.SetStringValue(_T("File3"), strFile3);
        if (!strFile4.IsEmpty())
            files.SetStringValue(_T("File4"), strFile4);
    }
}

void RegistrySettings::SetMostRecentFile(LPCTSTR szPathName)
{
    if (szPathName && szPathName[0])
        SHAddToRecentDocs(SHARD_PATHW, szPathName);

    if (strFile1 == szPathName)
    {
        // do nothing
    }
    else if (strFile2 == szPathName)
    {
        CString strTemp = strFile2;
        strFile2 = strFile1;
        strFile1 = strTemp;
    }
    else if (strFile3 == szPathName)
    {
        CString strTemp = strFile3;
        strFile3 = strFile2;
        strFile2 = strFile1;
        strFile1 = strTemp;
    }
    else if (strFile4 == szPathName)
    {
        CString strTemp = strFile4;
        strFile4 = strFile3;
        strFile3 = strFile2;
        strFile2 = strFile1;
        strFile1 = strTemp;
    }
    else
    {
        strFile4 = strFile3;
        strFile3 = strFile2;
        strFile2 = strFile1;
        strFile1 = szPathName;
    }
}
