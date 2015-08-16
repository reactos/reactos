/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint_new/registry.cpp
 * PURPOSE:     Offering functions dealing with registry values
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include "precomp.h"

#include <winreg.h>

/* FUNCTIONS ********************************************************/

void RegistrySettings::SetWallpaper(TCHAR * FileName, DWORD dwStyle, DWORD dwTile) //FIXME: Has to be called 2x to apply the pattern (tiled/stretched) too
{
    HKEY hDesktop;
    TCHAR szStyle[3], szTile[3];

    SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, (PVOID) FileName, SPIF_UPDATEINIFILE);

    if ((dwStyle > 2) || (dwTile > 2))
        return;

    if (RegOpenKeyEx(HKEY_CURRENT_USER,
                     _T("Control Panel\\Desktop"), 0, KEY_READ | KEY_SET_VALUE, &hDesktop) == ERROR_SUCCESS)
    {
        RegSetValueEx(hDesktop, _T("Wallpaper"), 0, REG_SZ, (LPBYTE) FileName,
                      _tcslen(FileName) * sizeof(TCHAR));

        _stprintf(szStyle, _T("%lu"), dwStyle);
        _stprintf(szTile, _T("%lu"), dwTile);

        RegSetValueEx(hDesktop, _T("WallpaperStyle"), 0, REG_SZ, (LPBYTE) szStyle,
                      _tcslen(szStyle) * sizeof(TCHAR));
        RegSetValueEx(hDesktop, _T("TileWallpaper"), 0, REG_SZ, (LPBYTE) szTile,
                      _tcslen(szTile) * sizeof(TCHAR));

        RegCloseKey(hDesktop);
    }
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
    HKEY hView;
    LoadPresets();
    if (RegOpenKeyEx(HKEY_CURRENT_USER,
                     _T("Software\\Microsoft\\Windows\\CurrentVersion\\Applets\\Paint\\View"),
                     0, KEY_READ | KEY_SET_VALUE, &hView) == ERROR_SUCCESS)
    {
        DWORD cbData;
        cbData = sizeof(DWORD);
        RegQueryValueEx(hView, _T("BMPHeight"), 0, NULL, (LPBYTE) &BMPHeight, &cbData);
        cbData = sizeof(DWORD);
        RegQueryValueEx(hView, _T("BMPWidth"), 0, NULL, (LPBYTE) &BMPWidth, &cbData);
        cbData = sizeof(DWORD);
        RegQueryValueEx(hView, _T("GridExtent"), 0, NULL, (LPBYTE) &GridExtent, &cbData);
        cbData = sizeof(DWORD);
        RegQueryValueEx(hView, _T("NoStretching"), 0, NULL, (LPBYTE) &NoStretching, &cbData);
        cbData = sizeof(DWORD);
        RegQueryValueEx(hView, _T("ShowThumbnail"), 0, NULL, (LPBYTE) &ShowThumbnail, &cbData);
        cbData = sizeof(DWORD);
        RegQueryValueEx(hView, _T("SnapToGrid"), 0, NULL, (LPBYTE) &SnapToGrid, &cbData);
        cbData = sizeof(DWORD);
        RegQueryValueEx(hView, _T("ThumbHeight"), 0, NULL, (LPBYTE) &ThumbHeight, &cbData);
        cbData = sizeof(DWORD);
        RegQueryValueEx(hView, _T("ThumbWidth"), 0, NULL, (LPBYTE) &ThumbWidth, &cbData);
        cbData = sizeof(DWORD);
        RegQueryValueEx(hView, _T("ThumbXPos"), 0, NULL, (LPBYTE) &ThumbXPos, &cbData);
        cbData = sizeof(DWORD);
        RegQueryValueEx(hView, _T("ThumbYPos"), 0, NULL, (LPBYTE) &ThumbYPos, &cbData);
        cbData = sizeof(DWORD);
        RegQueryValueEx(hView, _T("UnitSetting"), 0, NULL, (LPBYTE) &UnitSetting, &cbData);
        cbData = sizeof(WINDOWPLACEMENT);
        RegQueryValueEx(hView, _T("WindowPlacement"), 0, NULL, (LPBYTE) &WindowPlacement, &cbData);
    }
}

void RegistrySettings::Store()
{
    HKEY hView;
    if (RegCreateKeyEx(HKEY_CURRENT_USER,
                       _T("Software\\Microsoft\\Windows\\CurrentVersion\\Applets\\Paint\\View"),
                       0, NULL, 0, KEY_READ | KEY_SET_VALUE, NULL, &hView, NULL) == ERROR_SUCCESS)
    {
        RegSetValueEx(hView, _T("BMPHeight"), 0, REG_DWORD, (LPBYTE) &BMPHeight, sizeof(DWORD));
        RegSetValueEx(hView, _T("BMPWidth"), 0, REG_DWORD, (LPBYTE) &BMPWidth, sizeof(DWORD));
        RegSetValueEx(hView, _T("GridExtent"), 0, REG_DWORD, (LPBYTE) &GridExtent, sizeof(DWORD));
        RegSetValueEx(hView, _T("NoStretching"), 0, REG_DWORD, (LPBYTE) &NoStretching, sizeof(DWORD));
        RegSetValueEx(hView, _T("ShowThumbnail"), 0, REG_DWORD, (LPBYTE) &ShowThumbnail, sizeof(DWORD));
        RegSetValueEx(hView, _T("SnapToGrid"), 0, REG_DWORD, (LPBYTE) &SnapToGrid, sizeof(DWORD));
        RegSetValueEx(hView, _T("ThumbHeight"), 0, REG_DWORD, (LPBYTE) &ThumbHeight, sizeof(DWORD));
        RegSetValueEx(hView, _T("ThumbWidth"), 0, REG_DWORD, (LPBYTE) &ThumbWidth, sizeof(DWORD));
        RegSetValueEx(hView, _T("ThumbXPos"), 0, REG_DWORD, (LPBYTE) &ThumbXPos, sizeof(DWORD));
        RegSetValueEx(hView, _T("ThumbYPos"), 0, REG_DWORD, (LPBYTE) &ThumbYPos, sizeof(DWORD));
        RegSetValueEx(hView, _T("UnitSetting"), 0, REG_DWORD, (LPBYTE) &UnitSetting, sizeof(DWORD));
        RegSetValueEx(hView, _T("WindowPlacement"), 0, REG_BINARY, (LPBYTE) &WindowPlacement, sizeof(WINDOWPLACEMENT));
    }
}
