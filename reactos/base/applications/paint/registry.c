/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        registry.c
 * PURPOSE:     Offering functions dealing with registry values
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include <windows.h>
#include <tchar.h>

/* FUNCTIONS ********************************************************/

void SetWallpaper(TCHAR *FileName, DWORD dwStyle, DWORD dwTile)
{
    HKEY hDesktop;
    TCHAR szStyle[3], szTile[3];

    if ((dwStyle > 2) || (dwTile > 2))
        return;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, 
        _T("Control Panel\\Desktop"), 0,
        KEY_READ | KEY_SET_VALUE, &hDesktop) == ERROR_SUCCESS)
    {
        RegSetValueEx(hDesktop, _T("Wallpaper"), 0, REG_SZ, (LPBYTE) FileName, _tcslen(FileName) * sizeof(TCHAR));

        _stprintf(szStyle, _T("%i"), dwStyle);
        _stprintf(szTile,  _T("%i"), dwTile);

        RegSetValueEx(hDesktop, _T("WallpaperStyle"), 0, REG_SZ, (LPBYTE) szStyle, _tcslen(szStyle) * sizeof(TCHAR));
        RegSetValueEx(hDesktop, _T("TileWallpaper"), 0, REG_SZ, (LPBYTE) szTile, _tcslen(szTile) * sizeof(TCHAR));

        RegCloseKey(hDesktop);
    }
}
