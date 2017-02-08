/*
 * PROJECT:     ReactOS Magnify
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/magnify/settings.c
 * PURPOSE:     Magnification of parts of the screen.
 * COPYRIGHT:   Copyright 2007 Marc Piulachs <marc.piulachs@codexchange.net>
 *
 */

#include "magnifier.h"

#include <tchar.h>
#include <winreg.h>

int iZoom = 3;

BOOL bShowWarning = TRUE;

BOOL bFollowMouse = TRUE;
BOOL bFollowFocus = TRUE;
BOOL bFollowCaret = TRUE;

BOOL bInvertColors = FALSE;
BOOL bStartMinimized = FALSE;
BOOL bShowMagnifier = TRUE;

struct _AppBarConfig_t AppBarConfig = {
    sizeof(struct _AppBarConfig_t),
    -2 /* ABE_TOP */,
    0, 1, /* unknown */
    { 101,101,101,101 }, /* edge sizes */
    { 20, 20, 600, 200 }, /* floating window rect */
};

void LoadSettings()
{
    HKEY hkey;
    LONG value;
    ULONG len;
    struct _AppBarConfig_t config_temp;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Magnify"), 0, KEY_READ, &hkey) == ERROR_SUCCESS)
    {
        len = sizeof(AppBarConfig);
        if (RegQueryValueEx(hkey, _T("AppBar"), 0, 0, (BYTE *)&config_temp, &len) == ERROR_SUCCESS)
        {
            if(config_temp.cbSize == sizeof(AppBarConfig))
            {
                AppBarConfig = config_temp;
            }
        }

        len = 4;
        if (RegQueryValueEx(hkey, _T("StationaryMagLevel"),  0, 0, (BYTE *)&value, &len) == ERROR_SUCCESS)
        {
            if (value >= 0 && value <= 9)
                iZoom  = value;
        }

        len = 4;
        if (RegQueryValueEx(hkey, _T("ShowWarning"), 0, 0, (BYTE *)&value, &len) == ERROR_SUCCESS)
            bShowWarning = (value == 0 ? FALSE : TRUE);

        len = 4;
        if (RegQueryValueEx(hkey, _T("StationaryInvertColors"), 0, 0, (BYTE *)&value, &len) == ERROR_SUCCESS)
            bInvertColors = (value == 0 ? FALSE : TRUE);

        len = 4;
        if (RegQueryValueEx(hkey, _T("StationaryStartMinimized"), 0, 0, (BYTE *)&value, &len) == ERROR_SUCCESS)
            bStartMinimized = (value == 0 ? FALSE : TRUE);

        len = 4;
        if (RegQueryValueEx(hkey, _T("StationaryTrackCursor"), 0, 0, (BYTE *)&value, &len) == ERROR_SUCCESS)
            bFollowMouse = (value == 0 ? FALSE : TRUE);

        len = 4;
        if (RegQueryValueEx(hkey, _T("StationaryTrackFocus"), 0, 0, (BYTE *)&value, &len) == ERROR_SUCCESS)
            bFollowFocus = (value == 0 ? FALSE : TRUE);

        len = 4;
        if (RegQueryValueEx(hkey, _T("StationaryTrackSecondaryFocus"), 0, 0, (BYTE *)&value, &len) == ERROR_SUCCESS)
            bFollowFocus = (value == 0 ? FALSE : TRUE);

        len = 4;
        if (RegQueryValueEx(hkey, _T("StationaryTrackText"), 0, 0, (BYTE *)&value, &len) == ERROR_SUCCESS)
            bFollowCaret = (value == 0 ? FALSE : TRUE);

        RegCloseKey(hkey);
    }
}

void SaveSettings()
{
    HKEY hkey;
    LONG value;

    if (RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Magnify"), 0, _T(""), 0, KEY_WRITE, NULL, &hkey, NULL) == ERROR_SUCCESS)
    {
        RegSetValueEx(hkey, _T("AppBar"), 0, REG_BINARY, (BYTE *)&AppBarConfig, sizeof(AppBarConfig));

        value = iZoom;
        RegSetValueEx(hkey, _T("StationaryMagLevel"), 0, REG_DWORD, (BYTE *)&value, sizeof(value));

        value = bShowWarning;
        RegSetValueEx(hkey, _T("ShowWarning"), 0, REG_DWORD, (BYTE *)&value, sizeof(value));

        value = bInvertColors;
        RegSetValueEx(hkey, _T("StationaryInvertColors"), 0, REG_DWORD, (BYTE *)&value, sizeof(value));

        value = bStartMinimized;
        RegSetValueEx(hkey, _T("StationaryStartMinimized"), 0, REG_DWORD, (BYTE *)&value, sizeof(value));

        value = bFollowMouse;
        RegSetValueEx(hkey, _T("StationaryTrackCursor"), 0, REG_DWORD, (BYTE *)&value, sizeof(value));

        value = bFollowFocus;
        RegSetValueEx(hkey, _T("StationaryTrackFocus"), 0, REG_DWORD, (BYTE *)&value, sizeof(value));

        value = bFollowFocus;
        RegSetValueEx(hkey, _T("StationaryTrackSecondaryFocus"), 0, REG_DWORD, (BYTE *)&value, sizeof(value));

        value = bFollowCaret;
        RegSetValueEx(hkey, _T("StationaryTrackText"), 0, REG_DWORD, (BYTE *)&value, sizeof(value));

        RegCloseKey(hkey);
    }
}
