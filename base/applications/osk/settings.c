/*
 * PROJECT:         ReactOS On-Screen Keyboard
 * LICENSE:         GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:         Configuration settings of the application
 * COPYRIGHT:       Copyright 2018-2019 Bișoc George (fraizeraust99 at gmail dot com)
 */

/* INCLUDES *******************************************************************/

#include "precomp.h"

/* FUNCTIONS *******************************************************************/

BOOL LoadDataFromRegistry(VOID)
{
    HKEY hKey;
    LONG lResult;
    DWORD dwValue;
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
                               (BYTE *)&dwValue,
                               &cbData);

    if (lResult != ERROR_SUCCESS)
    {
        /* Bail out and return FALSE if we fail */
        RegCloseKey(hKey);
        return FALSE;
    }

    /* Load the data value (it can be either FALSE or TRUE depending on the data itself) */
    Globals.bShowWarning = (dwValue != 0);

    /* Query the key */
    lResult = RegQueryValueExW(hKey,
                               L"IsEnhancedKeyboard",
                               0,
                               0,
                               (BYTE *)&dwValue,
                               &cbData);

    if (lResult != ERROR_SUCCESS)
    {
        /* Bail out and return FALSE if we fail */
        RegCloseKey(hKey);
        return FALSE;
    }

    /* Load the dialog layout value */
    Globals.bIsEnhancedKeyboard = (dwValue != 0);

    /* Query the key */
    lResult = RegQueryValueExW(hKey,
                               L"ClickSound",
                               0,
                               0,
                               (BYTE *)&dwValue,
                               &cbData);

    if (lResult != ERROR_SUCCESS)
    {
        /* Bail out and return FALSE if we fail */
        RegCloseKey(hKey);
        return FALSE;
    }

    /* Load the sound on click value event */
    Globals.bSoundClick = (dwValue != 0);

    /* Query the key */
    lResult = RegQueryValueExW(hKey,
                               L"WindowLeft",
                               0,
                               0,
                               (BYTE *)&dwValue,
                               &cbData);

    if (lResult != ERROR_SUCCESS)
    {
        /* Bail out and return FALSE if we fail */
        RegCloseKey(hKey);
        return FALSE;
    }

    /* Load the X value data of the dialog's coordinate */
    Globals.PosX = dwValue;

    lResult = RegQueryValueExW(hKey,
                               L"WindowTop",
                               0,
                               0,
                               (BYTE *)&dwValue,
                               &cbData);

    if (lResult != ERROR_SUCCESS)
    {
        /* Bail out and return FALSE if we fail */
        RegCloseKey(hKey);
        return FALSE;
    }

    /* Load the Y value data of the dialog's coordinate */
    Globals.PosY = dwValue;

    lResult = RegQueryValueExW(hKey,
                               L"AlwaysOnTop",
                               0,
                               0,
                               (BYTE *)&dwValue,
                               &cbData);

    if (lResult != ERROR_SUCCESS)
    {
        /* Bail out and return FALSE if we fail */
        RegCloseKey(hKey);
        return FALSE;
    }

    /* Load the window state value data */
    Globals.bAlwaysOnTop = (dwValue != 0);
    
    /* If we're here then we succeed, close the key and return TRUE */
    RegCloseKey(hKey);
    return TRUE;
}

BOOL SaveDataToRegistry(VOID)
{
    HKEY hKey;
    LONG lResult;
    DWORD dwValue;
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
    dwValue = Globals.bShowWarning;

    /* Welcome warning box value key */
    lResult = RegSetValueExW(hKey,
                             L"ShowWarning",
                             0,
                             REG_DWORD,
                             (BYTE *)&dwValue,
                             sizeof(dwValue));

    if (lResult != ERROR_SUCCESS)
    {
        /* Bail out and return FALSE if we fail */
        RegCloseKey(hKey);
        return FALSE;
    }

    /* The value will be appended to the layout dialog */
    dwValue = Globals.bIsEnhancedKeyboard;

    /* Keyboard dialog switcher */
    lResult = RegSetValueExW(hKey,
                             L"IsEnhancedKeyboard",
                             0,
                             REG_DWORD,
                             (BYTE *)&dwValue,
                             sizeof(dwValue));

    if (lResult != ERROR_SUCCESS)
    {
        /* Bail out and return FALSE if we fail */
        RegCloseKey(hKey);
        return FALSE;
    }

    /* The value will be appended to the sound on click event */
    dwValue = Globals.bSoundClick;

    /* "Sound on Click" switcher value key */
    lResult = RegSetValueExW(hKey,
                             L"ClickSound",
                             0,
                             REG_DWORD,
                             (BYTE *)&dwValue,
                             sizeof(dwValue));

    if (lResult != ERROR_SUCCESS)
    {
        /* Bail out and return FALSE if we fail */
        RegCloseKey(hKey);
        return FALSE;
    }

    /* The value will be appended to the X coordination dialog's placement */
    dwValue = wp.rcNormalPosition.left;

    /* Position X coordination of dialog's placement value key */
    lResult = RegSetValueExW(hKey,
                             L"WindowLeft",
                             0,
                             REG_DWORD,
                             (BYTE *)&dwValue,
                             sizeof(dwValue));

    if (lResult != ERROR_SUCCESS)
    {
        /* Bail out and return FALSE if we fail */
        RegCloseKey(hKey);
        return FALSE;
    }

    /* The value will be appended to the Y coordination dialog's placement */
    dwValue = wp.rcNormalPosition.top;

    /* Position Y coordination of dialog's placement value key */
    lResult = RegSetValueExW(hKey,
                             L"WindowTop",
                             0,
                             REG_DWORD,
                             (BYTE *)&dwValue,
                             sizeof(dwValue));

    if (lResult != ERROR_SUCCESS)
    {
        /* Bail out and return FALSE if we fail */
        RegCloseKey(hKey);
        return FALSE;
    }

    /* Window top state value */
    dwValue = Globals.bAlwaysOnTop;

    /* "Always on Top" state value key */
    lResult = RegSetValueExW(hKey,
                             L"AlwaysOnTop",
                             0,
                             REG_DWORD,
                             (BYTE *)&dwValue,
                             sizeof(dwValue));

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
