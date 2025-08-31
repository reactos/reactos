/*
 * PROJECT:     ReactOS Logon GINA DLL msgina.dll
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Miscellaneous utility functions.
 * COPYRIGHT:   Copyright 2006 Hervé Poussineau <hpoussin@reactos.org>
 *              Copyright 2014 Eric Kohl <eric.kohl@reactos.org>
 *              Copyright 2025 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#include "msgina.h"

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
    if ((rc == ERROR_SUCCESS) && (dwType == REG_DWORD) && (cbData == sizeof(dwValue)))
    {
        *pValue = dwValue;
        return ERROR_SUCCESS;
    }

    return rc;
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
