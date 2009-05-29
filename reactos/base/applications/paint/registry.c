/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        registry.c
 * PURPOSE:     Offering functions dealing with registry values
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include <windows.h>

/* FUNCTIONS ********************************************************/

void setWallpaper(char fname[], int style)
{
    HKEY hkeycontrolpanel;
    HKEY hkeydesktop;
    RegOpenKeyEx(HKEY_CURRENT_USER, "Control Panel", 0, 0, hkeycontrolpanel);
    RegOpenKeyEx(hkeycontrolpanel, "Desktop", 0, KEY_SET_VALUE, hkeydesktop);
    RegSetValueEx(hkeydesktop, "Wallpaper", 0, REG_SZ, fname, sizeof(fname));
    switch (style)
    {
        case 0:
            RegSetValueEx(hkeydesktop, "WallpaperStyle", 0, REG_SZ, "2", 2);
            RegSetValueEx(hkeydesktop, "TileWallpaper", 0, REG_SZ, "0", 2);
            break;
        case 1:
            RegSetValueEx(hkeydesktop, "WallpaperStyle", 0, REG_SZ, "1", 2);
            RegSetValueEx(hkeydesktop, "TileWallpaper", 0, REG_SZ, "0", 2);
            break;
        case 2:
            RegSetValueEx(hkeydesktop, "WallpaperStyle", 0, REG_SZ, "1", 2);
            RegSetValueEx(hkeydesktop, "TileWallpaper", 0, REG_SZ, "1", 2);
            break;
    }
    RegCloseKey(hkeydesktop);
    RegCloseKey(hkeycontrolpanel);
}
