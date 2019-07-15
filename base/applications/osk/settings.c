/*
 * PROJECT:         ReactOS On-Screen Keyboard
 * LICENSE:         GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:         Configuration settings of the application
 * COPYRIGHT:       Copyright 2018-2019 Bișoc George (fraizeraust99 at gmail dot com)
 */

/* INCLUDES *******************************************************************/

#include "osk.h"
#include "settings.h"

/* FUNCTIONS *******************************************************************/

BOOL LoadDataFromRegistry()
{
    HKEY hKey;
    LONG lResult;
    DWORD dwShowWarningData, dwLayout, dwSoundOnClick, dwPositionLeft, dwPositionTop, dwAlwaysOnTop;
    DWORD cbData = sizeof(DWORD);

    /* Initialize the registry application settings */
    Globals.bShowWarning = TRUE;
    Globals.bIsEnhancedKeyboard = TRUE;
    Globals.bSoundClick = FALSE;
    Globals.bAlwaysOnTop = TRUE;

    /* Set the coordinate values to default */
    Globals.PosX = CW_USEDEFAULT;
    Globals.PosY = CW_USEDEFAULT;

    /* Open the key, so that we can query it */
    lResult = RegOpenKeyExW(HKEY_CURRENT_USER,
                            L"Software\\Microsoft\\Osk",
                            0,
                            KEY_READ,
                            &hKey);

    if (lResult != ERROR_SUCCESS)
    {
        /* Bail out and return FALSE if we fail */
        return FALSE;
    }

    /* Query the key */
    lResult = RegQueryValueExW(hKey,
                               L"ShowWarning",
                               0,
                               0,
                               (BYTE *)&dwShowWarningData,
                               &cbData);

    if (lResult != ERROR_SUCCESS)
    {
        /* Bail out and return FALSE if we fail */
        RegCloseKey(hKey);
        return FALSE;
    }

    /* Load the data value (it can be either FALSE or TRUE depending on the data itself) */
    Globals.bShowWarning = (dwShowWarningData != 0);

    /* Query the key */
    lResult = RegQueryValueExW(hKey,
                               L"IsEnhancedKeyboard",
                               0,
                               0,
                               (BYTE *)&dwLayout,
                               &cbData);

    if (lResult != ERROR_SUCCESS)
    {
        /* Bail out and return FALSE if we fail */
        RegCloseKey(hKey);
        return FALSE;
    }

    /* Load the dialog layout value */
    Globals.bIsEnhancedKeyboard = (dwLayout != 0);

    /* Query the key */
    lResult = RegQueryValueExW(hKey,
                               L"ClickSound",
                               0,
                               0,
                               (BYTE *)&dwSoundOnClick,
                               &cbData);

    if (lResult != ERROR_SUCCESS)
    {
        /* Bail out and return FALSE if we fail */
        RegCloseKey(hKey);
        return FALSE;
    }

    /* Load the sound on click value event */
    Globals.bSoundClick = (dwSoundOnClick != 0);

    /* Query the key */
    lResult = RegQueryValueExW(hKey,
                               L"WindowLeft",
                               0,
                               0,
                               (BYTE *)&dwPositionLeft,
                               &cbData);

    if (lResult != ERROR_SUCCESS)
    {
        /* Bail out and return FALSE if we fail */
        RegCloseKey(hKey);
        return FALSE;
    }

    /* Load the X value data of the dialog's coordinate */
    Globals.PosX = dwPositionLeft;

    lResult = RegQueryValueExW(hKey,
                               L"WindowTop",
                               0,
                               0,
                               (BYTE *)&dwPositionTop,
                               &cbData);

    if (lResult != ERROR_SUCCESS)
    {
        /* Bail out and return FALSE if we fail */
        RegCloseKey(hKey);
        return FALSE;
    }

    /* Load the Y value data of the dialog's coordinate */
    Globals.PosY = dwPositionTop;

    lResult = RegQueryValueExW(hKey,
                               L"AlwaysOnTop",
                               0,
                               0,
                               (BYTE *)&dwAlwaysOnTop,
                               &cbData);

    if (lResult != ERROR_SUCCESS)
    {
        /* Bail out and return FALSE if we fail */
        RegCloseKey(hKey);
        return FALSE;
    }

    /* Load the window state value data */
    Globals.bAlwaysOnTop = (dwAlwaysOnTop != 0);
    
    /* If we're here then we succeed, close the key and return TRUE */
    RegCloseKey(hKey);
    return TRUE;
}

BOOL SaveDataToRegistry()
{
    HKEY hKey;
    LONG lResult;
    DWORD dwShowWarningData, dwLayout, dwSoundOnClick, dwPositionLeft, dwPositionTop, dwAlwaysOnTop;
    WINDOWPLACEMENT wp;

    /* Set the structure length and retrieve the dialog's placement */
    wp.length = sizeof(WINDOWPLACEMENT);
    GetWindowPlacement(Globals.hMainWnd, &wp);

    /* If no key has been made, create one */
    lResult = RegCreateKeyExW(HKEY_CURRENT_USER,
                              L"Software\\Microsoft\\Osk",
                              0,
                              NULL,
                              0,
                              KEY_WRITE,
                              NULL,
                              &hKey,
                              NULL);

    if (lResult != ERROR_SUCCESS)
    {
        /* Bail out and return FALSE if we fail */
        return FALSE;
    }

    /* The data value of the subkey will be appended to the warning dialog switch */
    dwShowWarningData = Globals.bShowWarning;

    /* Welcome warning box value key */
    lResult = RegSetValueExW(hKey,
                             L"ShowWarning",
                             0,
                             REG_DWORD,
                             (BYTE *)&dwShowWarningData,
                             sizeof(dwShowWarningData));

    if (lResult != ERROR_SUCCESS)
    {
        /* Bail out and return FALSE if we fail */
        RegCloseKey(hKey);
        return FALSE;
    }

    /* The value will be appended to the layout dialog */
    dwLayout = Globals.bIsEnhancedKeyboard;

    /* Keyboard dialog switcher */
    lResult = RegSetValueExW(hKey,
                             L"IsEnhancedKeyboard",
                             0,
                             REG_DWORD,
                             (BYTE *)&dwLayout,
                             sizeof(dwLayout));

    if (lResult != ERROR_SUCCESS)
    {
        /* Bail out and return FALSE if we fail */
        RegCloseKey(hKey);
        return FALSE;
    }

    /* The value will be appended to the sound on click event */
    dwSoundOnClick = Globals.bSoundClick;

    /* "Sound on Click" switcher value key */
    lResult = RegSetValueExW(hKey,
                             L"ClickSound",
                             0,
                             REG_DWORD,
                             (BYTE *)&dwSoundOnClick,
                             sizeof(dwSoundOnClick));

    if (lResult != ERROR_SUCCESS)
    {
        /* Bail out and return FALSE if we fail */
        RegCloseKey(hKey);
        return FALSE;
    }

    /* The value will be appended to the X coordination dialog's placement */
    dwPositionLeft = wp.rcNormalPosition.left;

    /* Position X coordination of dialog's placement value key */
    lResult = RegSetValueExW(hKey,
                             L"WindowLeft",
                             0,
                             REG_DWORD,
                             (BYTE *)&dwPositionLeft,
                             sizeof(dwPositionLeft));

    if (lResult != ERROR_SUCCESS)
    {
        /* Bail out and return FALSE if we fail */
        RegCloseKey(hKey);
        return FALSE;
    }

    /* The value will be appended to the Y coordination dialog's placement */
    dwPositionTop = wp.rcNormalPosition.top;

    /* Position Y coordination of dialog's placement value key */
    lResult = RegSetValueExW(hKey,
                             L"WindowTop",
                             0,
                             REG_DWORD,
                             (BYTE *)&dwPositionTop,
                             sizeof(dwPositionTop));

    if (lResult != ERROR_SUCCESS)
    {
        /* Bail out and return FALSE if we fail */
        RegCloseKey(hKey);
        return FALSE;
    }

    /* Window top state value */
    dwAlwaysOnTop = Globals.bAlwaysOnTop;

    /* "Always on Top" state value key */
    lResult = RegSetValueExW(hKey,
                             L"AlwaysOnTop",
                             0,
                             REG_DWORD,
                             (BYTE *)&dwAlwaysOnTop,
                             sizeof(dwAlwaysOnTop));

    if (lResult != ERROR_SUCCESS)
    {
        /* Bail out and return FALSE if we fail */
        RegCloseKey(hKey);
        return FALSE;
    }

    /* If we're here then we succeed, close the key and return TRUE */
    RegCloseKey(hKey);
    return TRUE;
}
