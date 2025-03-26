/*
 * PROJECT:         ReactOS On-Screen Keyboard
 * LICENSE:         GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:         Configuration settings of the application
 * COPYRIGHT:       Copyright 2018-2019 George Bișoc (george.bisoc@reactos.org)
 *                  Baruch Rutman (peterooch at gmail dot com)
 */

/* INCLUDES *******************************************************************/

#include "precomp.h"

/* FUNCTIONS *******************************************************************/

LONG LoadDWORDFromRegistry(IN LPCWSTR lpValueDataName,
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
        DPRINT("LoadDWORDFromRegistry(): Failed to open the application's key! (Error - %li)\n", lResult);
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
        DPRINT("LoadDWORDFromRegistry(): Failed to load the following value - \"%S\". (Error - %li)\n", lpValueDataName, lResult);
        RegCloseKey(hKey);
        return lResult;
    }

    /* Is the buffer's size too small to query the required data? */
    if (cbData != sizeof(dwValue))
    {
        /* It is therefore bail out */
        DPRINT("LoadDWORDFromRegistry(): The buffer is too small to hold the data!\n");
        RegCloseKey(hKey);
        return ERROR_MORE_DATA;
    }

    *pdwValueData = dwValue;
    RegCloseKey(hKey);
    return lResult;
}

/* IN: cchCount is how many characters fit in lpValueData,
   OUT: cchCount is how many characters were written into lpValueData */
LONG LoadStringFromRegistry(IN LPCWSTR lpValueDataName,
                            OUT LPWSTR lpValueData,
                            IN OUT LPUINT cchCount)
{
    HKEY hKey;
    LONG lResult;
    UINT cbCount;

    cbCount = (*cchCount) * sizeof(WCHAR);

    /* Open our application's key in order to load its configuration data */
    lResult = RegOpenKeyExW(HKEY_CURRENT_USER,
                            L"Software\\Microsoft\\Osk",
                            0,
                            KEY_READ,
                            &hKey);

    if (lResult != ERROR_SUCCESS)
    {
        /* Bail out */
        DPRINT("LoadStringFromRegistry(): Failed to open the application's key! (Error - %li)\n", lResult);
        return lResult;
    }

    /* Load the specific value based on the parameter caller, lpValueDataName */
    lResult = RegQueryValueExW(hKey,
                               lpValueDataName,
                               0,
                               0,
                               (BYTE *)lpValueData,
                               (LPDWORD)&cbCount);


    if (lResult != ERROR_SUCCESS)
    {

        /* Bail out */
        DPRINT("LoadStringFromRegistry(): Failed to load the following value - \"%S\". (Error - %li)\n", lpValueDataName, lResult);
        RegCloseKey(hKey);
        return lResult;
    }

    *cchCount = cbCount / sizeof(WCHAR);

    RegCloseKey(hKey);
    return lResult;
}

LONG SaveDWORDToRegistry(IN LPCWSTR lpValueDataName,
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
        DPRINT("SaveDWORDToRegistry(): Failed to create the application's key! (Error - %li)\n", lResult);
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
        DPRINT("SaveDWORDToRegistry(): Failed to save the following value - \"%S\". (Error - %li)\n", lpValueDataName, lResult);
        RegCloseKey(hKey);
        return lResult;
    }

    RegCloseKey(hKey);
    return lResult;
}

LONG SaveStringToRegistry(IN LPCWSTR lpValueDataName,
                          IN LPCWSTR lpValueData,
                          IN UINT cchCount)
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
        DPRINT("SaveStringToRegistry(): Failed to create the application's key! (Error - %li)\n", lResult);
        return lResult;
    }

    /* Save the data into the registry value */
    lResult = RegSetValueExW(hKey,
                             lpValueDataName,
                             0,
                             REG_SZ,
                             (BYTE *)lpValueData,
                             cchCount * sizeof(WCHAR));

    if (lResult != ERROR_SUCCESS)
    {
        /* Bail out */
        DPRINT("SaveStringToRegistry(): Failed to save the following value - \"%S\". (Error - %li)\n", lpValueDataName, lResult);
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

    /* Set font value defaults */
    Globals.FontHeight = DEFAULT_FONTSIZE;

    /* Warning dialog registry setting */
    lResult = LoadDWORDFromRegistry(L"ShowWarning", &dwValue);
    if (lResult == NO_ERROR)
        Globals.bShowWarning = (dwValue != 0);

    /* Enhanced keyboard switch dialog registry setting */
    lResult = LoadDWORDFromRegistry(L"IsEnhancedKeyboard", &dwValue);
    if (lResult == NO_ERROR)
        Globals.bIsEnhancedKeyboard = (dwValue != 0);

    /* Sound on click event registry setting */
    lResult = LoadDWORDFromRegistry(L"ClickSound", &dwValue);
    if (lResult == NO_ERROR)
        Globals.bSoundClick = (dwValue != 0);

    /* X coordinate dialog placement registry setting */
    lResult = LoadDWORDFromRegistry(L"WindowLeft", &dwValue);
    if (lResult == NO_ERROR)
        Globals.PosX = dwValue;

    /* Y coordinate dialog placement registry setting */
    lResult = LoadDWORDFromRegistry(L"WindowTop", &dwValue);
    if (lResult == NO_ERROR)
        Globals.PosY = dwValue;

    /* Top window state registry setting */
    lResult = LoadDWORDFromRegistry(L"AlwaysOnTop", &dwValue);
    if (lResult == NO_ERROR)
        Globals.bAlwaysOnTop = (dwValue != 0);

    /* Font information */
    UINT cchCount = _countof(Globals.FontFaceName);
    lResult = LoadStringFromRegistry(L"FontFaceName", Globals.FontFaceName, &cchCount);

    if (lResult != NO_ERROR) /* Copy default on failure */
        StringCchCopyW(Globals.FontFaceName, _countof(Globals.FontFaceName), L"MS Shell Dlg");

    lResult = LoadDWORDFromRegistry(L"FontHeight", &dwValue);
    if (lResult == NO_ERROR)
        Globals.FontHeight = dwValue;
}

VOID SaveSettings(VOID)
{
    WINDOWPLACEMENT wp;

    /* Initialize the window placement structure */
    wp.length = sizeof(WINDOWPLACEMENT);
    GetWindowPlacement(Globals.hMainWnd, &wp);

    /* Warning dialog registry setting */
    SaveDWORDToRegistry(L"ShowWarning", Globals.bShowWarning);

    /* Enhanced keyboard switch dialog registry setting */
    SaveDWORDToRegistry(L"IsEnhancedKeyboard", Globals.bIsEnhancedKeyboard);

    /* Sound on click event registry setting */
    SaveDWORDToRegistry(L"ClickSound", Globals.bSoundClick);

    /* X coordinate dialog placement registry setting */
    SaveDWORDToRegistry(L"WindowLeft", wp.rcNormalPosition.left);

    /* Y coordinate dialog placement registry setting */
    SaveDWORDToRegistry(L"WindowTop", wp.rcNormalPosition.top);

    /* Top window state registry setting */
    SaveDWORDToRegistry(L"AlwaysOnTop", Globals.bAlwaysOnTop);

    /* Font information */
    SaveStringToRegistry(L"FontFaceName", Globals.FontFaceName, _countof(Globals.FontFaceName));
    SaveDWORDToRegistry(L"FontHeight", Globals.FontHeight);
}
