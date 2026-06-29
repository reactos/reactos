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
    /* Buffer big enough to hold the NULL-terminated string L"4294967295",
     * corresponding to the literal 0xFFFFFFFF (MAXULONG) in decimal. */
    WCHAR Buffer[sizeof("4294967295")];
    C_ASSERT(sizeof(Buffer) >= sizeof(DWORD));

    cbData = sizeof(Buffer);
    rc = RegQueryValueExW(hKey, pszValue, NULL, &dwType, (PBYTE)&Buffer, &cbData);
    if (rc != ERROR_SUCCESS)
        return rc;

    if (dwType == REG_DWORD)
    {
        if (cbData != sizeof(dwValue))
            return ERROR_INVALID_DATA; // ERROR_DATATYPE_MISMATCH;
        dwValue = *(PDWORD)Buffer;
    }
    else if (dwType == REG_SZ)
    {
        PWCHAR pEnd = NULL;
        Buffer[cbData / sizeof(WCHAR) - 1] = UNICODE_NULL;
        dwValue = wcstoul(Buffer, &pEnd, 0);
        if (*pEnd) // Don't consider REG_SZ to be supported in this case!
            return ERROR_UNSUPPORTED_TYPE;
    }
    else
    {
        return ERROR_UNSUPPORTED_TYPE;
    }

    *pValue = dwValue;
    return ERROR_SUCCESS;
}

/**
 * @brief
 * Verifies whether the specified token has the given privilege.
 *
 * @see
 * shell32!SHTestTokenPrivilegeW(),
 * http://undoc.airesoft.co.uk/shell32.dll/SHTestTokenPrivilegeW.php
 * and setupapi!DoesUserHavePrivilege().
 */
BOOL
TestTokenPrivilege(
    _In_opt_ HANDLE hToken,
    _In_ ULONG Privilege)
{
    LUID PrivilegeLuid = {Privilege, 0};
    HANDLE hNewToken = NULL;
    PTOKEN_PRIVILEGES pTokenPriv;
    DWORD dwLength;
    BOOL ret = FALSE;

    if (!hToken)
    {
        /* Open effective token */
        ret = OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, FALSE, &hNewToken);
        if (!ret && (GetLastError() == ERROR_NO_TOKEN))
            ret = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hNewToken);
        if (!ret || !hNewToken)
            return FALSE;
        hToken = hNewToken;
    }

    dwLength = 0;
    ret = GetTokenInformation(hToken, TokenPrivileges, NULL, 0, &dwLength);
    if (!ret && (GetLastError() != ERROR_INSUFFICIENT_BUFFER))
        goto Quit;

    ret = FALSE;
    pTokenPriv = (PTOKEN_PRIVILEGES)LocalAlloc(LPTR, dwLength);
    if (!pTokenPriv)
        goto Quit;

    if (GetTokenInformation(hToken, TokenPrivileges, pTokenPriv, dwLength, &dwLength))
    {
        DWORD i, cPrivs = pTokenPriv->PrivilegeCount;
        for (i = 0; !ret && i < cPrivs; ++i)
        {
            ret = RtlEqualLuid(&PrivilegeLuid, &pTokenPriv->Privileges[i].Luid);
        }
    }

    LocalFree(pTokenPriv);

Quit:
    if (hToken == hNewToken)
        CloseHandle(hNewToken);
    return ret;
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
