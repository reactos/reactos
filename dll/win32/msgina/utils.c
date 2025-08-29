/*
 * PROJECT:     ReactOS Logon GINA DLL msgina.dll
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Miscellaneous utility functions.
 * COPYRIGHT:   Copyright 2006 Hervé Poussineau <hpoussin@reactos.org>
 *              Copyright 2014 Eric Kohl <eric.kohl@reactos.org>
 *              Copyright 2025 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#include "msgina.h"

/**
 * @brief
 * Opens and retrieves a handle to the HKEY_CURRENT_USER
 * corresponding to the specified logged-on user.
 *
 * @param[in]   hUserToken
 * Optional handle to a primary or impersonation access token that represents
 * a logged-on user. See @b ImpersonateLoggedOnUser() for more information.
 * If NULL, opens the SYSTEM's HKEY_USERS\.Default (i.e. HKEY_USERS\S-1-5-18).
 *
 * @param[in]   samDesired
 * A mask (type: REGSAM or ACCESS_MASK) that specifies the desired access
 * rights to the key. See @b RegOpenCurrentUser() for more information.
 *
 * @param[out]  phkResult
 * A pointer to a variable that receives a handle to the opened key.
 * When the handle is no longer needed, close it with @b RegCloseKey().
 **/
LONG
RegOpenLoggedOnHKCU(
    _In_opt_ HANDLE hUserToken,
    _In_ REGSAM samDesired,
    _Out_ PHKEY phkResult)
{
    LONG rc;

    /* Impersonate the logged-on user if necessary */
    if (hUserToken && !ImpersonateLoggedOnUser(hUserToken))
    {
        rc = GetLastError();
        ERR("ImpersonateLoggedOnUser() failed with error %ld\n", rc);
        return rc;
    }

    /* Open the logged-on user HKCU key */
    rc = RegOpenCurrentUser(samDesired, phkResult);

    /* Revert the impersonation */
    if (hUserToken)
        RevertToSelf();

    return rc;
}

LONG
ReadRegSzValue(
    _In_ HKEY hKey,
    _In_ PCWSTR pszValue,
    _Out_ PWSTR* pValue)
{
    LONG rc;
    DWORD dwType;
    DWORD cbData = 0;
    PWSTR Value;

    *pValue = NULL;
    rc = RegQueryValueExW(hKey, pszValue, NULL, &dwType, NULL, &cbData);
    if (rc != ERROR_SUCCESS)
        return rc;
    if (dwType != REG_SZ)
        return ERROR_UNSUPPORTED_TYPE;
    Value = HeapAlloc(GetProcessHeap(), 0, cbData + sizeof(WCHAR));
    if (!Value)
        return ERROR_NOT_ENOUGH_MEMORY;
    rc = RegQueryValueExW(hKey, pszValue, NULL, NULL, (PBYTE)Value, &cbData);
    if (rc != ERROR_SUCCESS)
    {
        HeapFree(GetProcessHeap(), 0, Value);
        return rc;
    }
    /* NULL-terminate the string */
    Value[cbData / sizeof(WCHAR)] = UNICODE_NULL;

    *pValue = Value;
    return ERROR_SUCCESS;
}

LONG
ReadRegDwordValue(
    _In_ HKEY hKey,
    _In_ PCWSTR pszValue,
    _Out_ PDWORD pValue)
{
    LONG rc;
    DWORD dwValue, dwType, cbData;

    cbData = sizeof(dwValue);
    rc = RegQueryValueExW(hKey, pszValue, NULL, &dwType, (PBYTE)&dwValue, &cbData);
    if (rc != ERROR_SUCCESS)
        return rc;
    if (dwType != REG_DWORD)
        return ERROR_UNSUPPORTED_TYPE;
    if (cbData != sizeof(dwValue))
        return ERROR_INVALID_DATA; // ERROR_DATATYPE_MISMATCH;

    *pValue = dwValue;
    return ERROR_SUCCESS;
}

PWSTR
DuplicateString(
    _In_opt_ PCWSTR Str)
{
    PWSTR NewStr;
    SIZE_T cb;

    if (!Str)
        return NULL;

    cb = (wcslen(Str) + 1) * sizeof(WCHAR);
    if ((NewStr = LocalAlloc(LMEM_FIXED, cb)))
        memcpy(NewStr, Str, cb);
    return NewStr;
}
