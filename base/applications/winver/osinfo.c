/*
 * PROJECT:    ReactOS Version Program
 * LICENSE:    LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:    Retrieve OS name and simple compatibility information
 * COPYRIGHT:  Copyright 2025 Thamatip Chitpong <thamatip.chitpong@reactos.org>
 */

#include "winver_p.h"

#define OSINFO_KEY L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"

static
VOID
Winver_GetRegValueString(
    _In_ HKEY hKey,
    _In_ LPCWSTR lpValue,
    _Out_ LPWSTR lpBuffer,
    _In_ DWORD cchSize)
{
    DWORD dwType, dwSize;
    LSTATUS lError;

    /* NOTE: Reserved space for a NULL terminator */
    dwSize = (cchSize - 1) * sizeof(WCHAR);
    lError = RegQueryValueExW(hKey, lpValue, NULL, &dwType, (LPBYTE)lpBuffer, &dwSize);
    if (lError != ERROR_SUCCESS || dwType != REG_SZ)
    {
        /* Return empty string on failure */
        lpBuffer[0] = UNICODE_NULL;
        return;
    }

    /* Ensure the returned string is NULL terminated */
    lpBuffer[cchSize - 1] = UNICODE_NULL;
}

static
VOID
Winver_GetFormattedSpkInfo(
    _In_ HKEY hKey,
    _Out_ LPWSTR lpBuffer,
    _In_ DWORD cchSize)
{
    WCHAR szRegValue[48];
    WCHAR szFormat[16] = L"";

    Winver_GetRegValueString(hKey, L"CSDVersion", szRegValue, _countof(szRegValue));
    if (!szRegValue[0])
    {
        /* Return empty string on failure */
        lpBuffer[0] = UNICODE_NULL;
        return;
    }

    LoadStringW(Winver_hInstance,
                IDS_OSINFO_SPK_FORMAT,
                szFormat,
                _countof(szFormat));

    StringCchPrintfW(lpBuffer, cchSize, szFormat, szRegValue);
}

static
VOID
Winver_FormatCompatInfo(
    _Inout_ PWINVER_OS_INFO OSInfo)
{
    WCHAR szFormat[64] = L"";

    /* Required info must be valid */
    if (!OSInfo->szNtVersion[0] || !OSInfo->szNtBuild[0])
    {
        /* Return empty string on failure */
        OSInfo->szCompatInfo[0] = UNICODE_NULL;
        return;
    }

    LoadStringW(Winver_hInstance,
                IDS_OSINFO_COMPAT_FORMAT,
                szFormat,
                _countof(szFormat));

    /* NOTE: Service pack info is optional */
    StringCchPrintfW(OSInfo->szCompatInfo, _countof(OSInfo->szCompatInfo), szFormat,
                     OSInfo->szNtVersion,
                     OSInfo->szNtBuild,
                     OSInfo->szNtSpk);
}

BOOL
Winver_GetOSInfo(
    _Out_ PWINVER_OS_INFO OSInfo)
{
    HKEY hKey;
    LSTATUS lError;

    lError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                           OSINFO_KEY,
                           0,
                           KEY_QUERY_VALUE,
                           &hKey);
    if (lError != ERROR_SUCCESS)
        return FALSE;

    /* OS name */
    Winver_GetRegValueString(hKey, L"ProductName", OSInfo->szName, _countof(OSInfo->szName));
    if (!OSInfo->szName[0])
    {
        /* This info must be valid */
        RegCloseKey(hKey);
        return FALSE;
    }

    /* Compatibility information */
    Winver_GetRegValueString(hKey, L"CurrentVersion", OSInfo->szNtVersion, _countof(OSInfo->szNtVersion));
    Winver_GetRegValueString(hKey, L"CurrentBuildNumber", OSInfo->szNtBuild, _countof(OSInfo->szNtBuild));
    Winver_GetFormattedSpkInfo(hKey, OSInfo->szNtSpk, _countof(OSInfo->szNtSpk));
    Winver_FormatCompatInfo(OSInfo);

    RegCloseKey(hKey);

    return TRUE;
}
