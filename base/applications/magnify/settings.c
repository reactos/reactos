/*
 * PROJECT:     ReactOS Magnify
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Magnification of parts of the screen.
 * COPYRIGHT:   Copyright 2007-2019 Marc Piulachs <marc.piulachs@codexchange.net>
 *              Copyright 2015-2019 David Quintana <gigaherz@gmail.com>
 */

#include "magnifier.h"

#include <tchar.h>
#include <winreg.h>

UINT uiZoom = 3;

BOOL bShowWarning = TRUE;

BOOL bFollowMouse = TRUE;
BOOL bFollowFocus = TRUE;
BOOL bFollowCaret = TRUE;

BOOL bInvertColors = FALSE;
BOOL bStartMinimized = FALSE;
BOOL bShowMagnifier = TRUE;

struct _AppBarConfig_t AppBarConfig =
{
    sizeof(struct _AppBarConfig_t),
    -2 /* ABE_TOP */,
    0, 1, /* unknown */
    { 101,101,101,101 }, /* edge sizes */
    { 20, 20, 600, 200 }, /* floating window rect */
};

void LoadSettings(void)
{
    HKEY hkey;
    LONG value;
    ULONG len;
    struct _AppBarConfig_t config_temp;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Magnify"), 0, KEY_READ, &hkey) != ERROR_SUCCESS)
        return;

    len = sizeof(AppBarConfig);
    if (RegQueryValueEx(hkey, _T("AppBar"), 0, 0, (BYTE *)&config_temp, &len) == ERROR_SUCCESS)
    {
        if(config_temp.cbSize == sizeof(AppBarConfig))
        {
            AppBarConfig = config_temp;
        }
    }

    len = sizeof(value);
    if (RegQueryValueEx(hkey, _T("StationaryMagLevel"), 0, 0, (BYTE *)&value, &len) == ERROR_SUCCESS)
    {
        if (value >= 0 && value <= 9)
            uiZoom = value;
    }

    len = sizeof(value);
    if (RegQueryValueEx(hkey, _T("ShowWarning"), 0, 0, (BYTE *)&value, &len) == ERROR_SUCCESS)
        bShowWarning = !!value;

    len = sizeof(value);
    if (RegQueryValueEx(hkey, _T("StationaryInvertColors"), 0, 0, (BYTE *)&value, &len) == ERROR_SUCCESS)
        bInvertColors = !!value;

    len = sizeof(value);
    if (RegQueryValueEx(hkey, _T("StationaryStartMinimized"), 0, 0, (BYTE *)&value, &len) == ERROR_SUCCESS)
        bStartMinimized = !!value;

    len = sizeof(value);
    if (RegQueryValueEx(hkey, _T("StationaryTrackCursor"), 0, 0, (BYTE *)&value, &len) == ERROR_SUCCESS)
        bFollowMouse = !!value;

    len = sizeof(value);
    if (RegQueryValueEx(hkey, _T("StationaryTrackFocus"), 0, 0, (BYTE *)&value, &len) == ERROR_SUCCESS)
        bFollowFocus = !!value;

    len = sizeof(value);
    if (RegQueryValueEx(hkey, _T("StationaryTrackSecondaryFocus"), 0, 0, (BYTE *)&value, &len) == ERROR_SUCCESS)
        bFollowFocus = !!value;

    len = sizeof(value);
    if (RegQueryValueEx(hkey, _T("StationaryTrackText"), 0, 0, (BYTE *)&value, &len) == ERROR_SUCCESS)
        bFollowCaret = !!value;

    RegCloseKey(hkey);
}

void SaveSettings(void)
{
    HKEY hkey;
    LONG value;

    if (RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Magnify"), 0, _T(""), 0, KEY_WRITE, NULL, &hkey, NULL) != ERROR_SUCCESS)
        return;

    RegSetValueEx(hkey, _T("AppBar"), 0, REG_BINARY, (BYTE *)&AppBarConfig, sizeof(AppBarConfig));

    value = uiZoom;
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
