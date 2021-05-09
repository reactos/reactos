/*
 * PROJECT:         ReactOS Utility Manager Resources DLL (UManDlg.dll)
 * LICENSE:         GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:         Registry functions for Utility Manager settings management
 * COPYRIGHT:       Copyright 2020 George Bișoc (george.bisoc@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include "umandlg.h"

/* GLOBALS ********************************************************************/

REGISTRY_DATA RegData;
REGISTRY_SETTINGS Settings;

/* DEFINES ********************************************************************/

#define ACCESS_UTILMAN_KEY L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Accessibility\\Utility Manager"
#define UTILMAN_KEY L"SOFTWARE\\Microsoft\\Utility Manager"
#define OSK_KEY L"On-Screen Keyboard"
#define MAGNIFIER_KEY L"Magnifier"

/* FUNCTIONS ******************************************************************/

/**
 * @InitAppRegKey
 *
 * Initialize a key. The function may not necessarily create it but open the key
 * if it already exists. The disposition pointed lpdwDisposition determines that.
 * This is a function helper.
 *
 * @param[in]   hPredefinedKey
 *     The predefined key (e.g. HKEY_CLASSES_ROOT).
 *
 * @param[in]   lpszSubKey
 *      The path to the sub key to be created.
 *
 * @param[out]   phKey
 *      A pointer that receives a handle to the key given by the function.
 *
 * @param[out]   lpdwDisposition
 *      A pointer that receives the disposition given by the function.
 *
 * @return
 *      Returns TRUE if the function successfully created a key (or opened it),
 *      FALSE otherwise for failure.
 *
 */
BOOL InitAppRegKey(IN HKEY hPredefinedKey,
                   IN LPCWSTR lpszSubKey,
                   OUT PHKEY phKey,
                   OUT LPDWORD lpdwDisposition)
{
    LONG lResult;

    lResult = RegCreateKeyExW(hPredefinedKey,
                              lpszSubKey,
                              0,
                              NULL,
                              0,
                              KEY_WRITE,
                              NULL,
                              phKey,
                              lpdwDisposition);
    if (lResult != ERROR_SUCCESS)
    {
        DPRINT("InitAppRegKey(): Failed to create the following key (or open the key) of path \"%S\". The error code is \"%li\".\n", lpszSubKey, lResult);
        return FALSE;
    }

    return TRUE;
}

/**
 * @QueryAppSettings
 *
 * Query the setting from the application's key. This is a function helper.
 *
 * @param[in]   hKey
 *     A handle to a key.
 *
 * @param[in]   lpszSubKey
 *      The path to a sub-key.
 *
 * @param[in]   lpszRegValue
 *      The registry value where we need to get the data from.
 *
 * @param[out]   ReturnedData
 *      An arbitrary pointer that receives the returned data. Being arbitrary,
 *      the data can be of any type.
 *
 * @param[inout]   lpdwSizeData
 *      A pointer to the returned data pointed by ReturnedData parameter that
 *      retrieves the size of the aforementioned data, in bytes.
 *
 * @return
 *      Returns TRUE if the function successfully loaded the value we wanted,
 *      FALSE otherwise for failure.
 *
 */
BOOL QueryAppSettings(IN HKEY hKey,
                      IN LPCWSTR lpszSubKey,
                      IN LPCWSTR lpszRegValue,
                      OUT PVOID ReturnedData,
                      IN OUT LPDWORD lpdwSizeData)
{
    LONG lResult;
    HKEY hKeyQueryValue;

    lResult = RegOpenKeyExW(hKey,
                            lpszSubKey,
                            0,
                            KEY_READ,
                            &hKeyQueryValue);
    if (lResult != ERROR_SUCCESS)
    {
        DPRINT("QueryAppSettings(): Failed to open the key of path \"%S\". The error code is \"%li\".\n", lpszSubKey, lResult);
        return FALSE;
    }

    lResult = RegQueryValueExW(hKeyQueryValue,
                               lpszRegValue,
                               NULL,
                               NULL,
                               (LPBYTE)&ReturnedData,
                               lpdwSizeData);
    if (lResult != ERROR_SUCCESS)
    {
        DPRINT("QueryAppSettings(): Failed to query the data from value \"%S\". The error code is \"%li\".\n", lpszRegValue, lResult);
        RegCloseKey(hKeyQueryValue);
        return FALSE;
    }

    RegCloseKey(hKeyQueryValue);
    return TRUE;
}

/**
 * @SaveAppSettings
 *
 * Save an application's setting data to the Registry. This is a function helper.
 *
 * @param[in]   hKey
 *     A handle to a key.
 *
 * @param[in]   lpszRegValue
 *      The path to the sub key where the value needs to be created.
 *
 * @param[out]   dwRegType
 *      The type of registry value to be created (e.g. a REG_DWORD).
 *
 * @param[in]    Data
 *      A pointer to an arbitrary data for the value to be set. Being arbitrary,
 *      the data can be of any type (in conformity with the registry type pointed by
 *      dwRegType) otherwise the function might lead to a undefined behaviour.
 *
 * @param[in]    cbSize
 *      The size of the buffer data pointed by Data parameter, in bytes.
 *
 * @return
 *      Returns TRUE if the function successfully saved the application's setting,
 *      FALSE otherwise for failure.
 *
 */
BOOL SaveAppSettings(IN HKEY hKey,
                     IN LPCWSTR lpszRegValue,
                     IN DWORD dwRegType,
                     IN PVOID Data,
                     IN DWORD cbSize)
{
    LONG lResult;
    HKEY hKeySetValue;

    lResult = RegOpenKeyExW(hKey,
                            NULL,
                            0,
                            KEY_SET_VALUE,
                            &hKeySetValue);
    if (lResult != ERROR_SUCCESS)
    {
        DPRINT("SaveAppSettings(): Failed to open the key, the error code is \"%li\"!\n", lResult);
        return FALSE;
    }

    lResult = RegSetValueExW(hKeySetValue,
                             lpszRegValue,
                             0,
                             dwRegType,
                             (LPBYTE)&Data,
                             cbSize);
    if (lResult != ERROR_SUCCESS)
    {
        DPRINT("SaveAppSettings(): Failed to set the \"%S\" value with data, the error code is \"%li\"!\n", lpszRegValue, lResult);
        RegCloseKey(hKeySetValue);
        return FALSE;
    }

    RegCloseKey(hKeySetValue);
    return TRUE;
}
