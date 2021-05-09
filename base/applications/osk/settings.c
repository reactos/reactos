/*
 * PROJECT:         ReactOS On-Screen Keyboard
 * LICENSE:         GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:         Configuration settings of the application
 * COPYRIGHT:       Copyright 2018-2019 George Bișoc (george.bisoc@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include "precomp.h"

/* FUNCTIONS *******************************************************************/

LONG LoadDataFromRegistry(IN LPCWSTR lpValueDataName,
                          OUT PDWORD pdwValueData)
{
    HKEY hKey;
    LONG lResult;
    DWORD dwValue;
    DWORD cbData = sizeof(dwValue);

    /* Initialize the pointer parameter to default */
    *pdwValueData = 0;

    /* Open our application's key in order to load its configuration data */
    lResult = RegOpenKeyExW(HKEY_CURRENT_USER,
                            L"Software\\Microsoft\\Osk",
                            0,
                            KEY_READ,
                            &hKey);

    if (lResult != ERROR_SUCCESS)
    {
        /* Bail out */
        DPRINT("LoadDataFromRegistry(): Failed to open the application's key! (Error - %li)\n", lResult);
        return lResult;
    }

    /* Load the specific value based on the parameter caller, lpValueDataName */
    lResult = RegQueryValueExW(hKey,
                               lpValueDataName,
                               0,
                               0,
                               (BYTE *)&dwValue,
                               &cbData);

    if (lResult != ERROR_SUCCESS)
    {

        /* Bail out */
        DPRINT("LoadDataFromRegistry(): Failed to load the following value - \"%S\". (Error - %li)\n", lpValueDataName, lResult);
        RegCloseKey(hKey);
        return lResult;
    }

    /* Is the buffer's size too small to query the required data? */
    if (cbData != sizeof(dwValue))
    {
        /* It is therefore bail out */
        DPRINT("LoadDataFromRegistry(): The buffer is too small to hold the data!\n");
        RegCloseKey(hKey);
        return ERROR_MORE_DATA;
    }

    *pdwValueData = dwValue;
    RegCloseKey(hKey);
    return lResult;
}

LONG SaveDataToRegistry(IN LPCWSTR lpValueDataName,
                        IN DWORD dwValueData)
{
    HKEY hKey;
    LONG lResult;

    /* Set up the application's key in case it has not been made yet */
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
        /* Bail out */
        DPRINT("SaveDataToRegistry(): Failed to create the application's key! (Error - %li)\n", lResult);
        return lResult;
    }

    /* Save the data into the registry value */
    lResult = RegSetValueExW(hKey,
                             lpValueDataName,
                             0,
                             REG_DWORD,
                             (BYTE *)&dwValueData,
                             sizeof(dwValueData));

    if (lResult != ERROR_SUCCESS)
    {
        /* Bail out */
        DPRINT("SaveDataToRegistry(): Failed to save the following value - \"%S\". (Error - %li)\n", lpValueDataName, lResult);
        RegCloseKey(hKey);
        return lResult;
    }

    RegCloseKey(hKey);
    return lResult;
}

VOID LoadSettings(VOID)
{
    DWORD dwValue;
    LONG lResult;

    /* Initialize the registry application settings */
    Globals.bShowWarning = TRUE;
    Globals.bIsEnhancedKeyboard = TRUE;
    Globals.bAlwaysOnTop = TRUE;
    Globals.bSoundClick = FALSE;

    /* Set the coordinate values to default */
    Globals.PosX = CW_USEDEFAULT;
    Globals.PosY = CW_USEDEFAULT;

    /* Warning dialog registry setting */
    lResult = LoadDataFromRegistry(L"ShowWarning", &dwValue);
    if (lResult == NO_ERROR)
        Globals.bShowWarning = (dwValue != 0);

    /* Enhanced keyboard switch dialog registry setting */
    lResult = LoadDataFromRegistry(L"IsEnhancedKeyboard", &dwValue);
    if (lResult == NO_ERROR)
        Globals.bIsEnhancedKeyboard = (dwValue != 0);

    /* Sound on click event registry setting */
    lResult = LoadDataFromRegistry(L"ClickSound", &dwValue);
    if (lResult == NO_ERROR)
        Globals.bSoundClick = (dwValue != 0);

    /* X coordinate dialog placement registry setting */
    lResult = LoadDataFromRegistry(L"WindowLeft", &dwValue);
    if (lResult == NO_ERROR)
        Globals.PosX = dwValue;

    /* Y coordinate dialog placement registry setting */
    lResult = LoadDataFromRegistry(L"WindowTop", &dwValue);
    if (lResult == NO_ERROR)
        Globals.PosY = dwValue;

    /* Top window state registry setting */
    lResult = LoadDataFromRegistry(L"AlwaysOnTop", &dwValue);
    if (lResult == NO_ERROR)
        Globals.bAlwaysOnTop = (dwValue != 0);
}

VOID SaveSettings(VOID)
{
    WINDOWPLACEMENT wp;

    /* Initialize the window placement structure */
    wp.length = sizeof(WINDOWPLACEMENT);
    GetWindowPlacement(Globals.hMainWnd, &wp);

    /* Warning dialog registry setting */
    SaveDataToRegistry(L"ShowWarning", Globals.bShowWarning);

    /* Enhanced keyboard switch dialog registry setting */
    SaveDataToRegistry(L"IsEnhancedKeyboard", Globals.bIsEnhancedKeyboard);

    /* Sound on click event registry setting */
    SaveDataToRegistry(L"ClickSound", Globals.bSoundClick);

    /* X coordinate dialog placement registry setting */
    SaveDataToRegistry(L"WindowLeft", wp.rcNormalPosition.left);

    /* Y coordinate dialog placement registry setting */
    SaveDataToRegistry(L"WindowTop", wp.rcNormalPosition.top);

    /* Top window state registry setting */
    SaveDataToRegistry(L"AlwaysOnTop", Globals.bAlwaysOnTop);
}
