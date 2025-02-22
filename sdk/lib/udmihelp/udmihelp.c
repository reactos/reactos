/*
 * PROJECT:     ReactOS User-mode DMI/SMBIOS Helper Functions
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     SMBIOS table parsing functions
 * COPYRIGHT:   Copyright 2018 Stanislav Motylkov
 */

#include "precomp.h"

static UINT (WINAPI * pGetSystemFirmwareTable)(DWORD, DWORD, PVOID, DWORD);
static BOOL bInitAPI = FALSE;

static
VOID
InitializeAPI()
{
    HANDLE hKernel;

    pGetSystemFirmwareTable = NULL;

    hKernel = GetModuleHandleW(L"kernel32.dll");
    if (!hKernel)
        return;

    pGetSystemFirmwareTable = (void *)GetProcAddress(hKernel, "GetSystemFirmwareTable");
}

/* Load SMBIOS Data */
PVOID
LoadSMBiosData(
    _Inout_updates_(ID_STRINGS_MAX) PCHAR * Strings)
{
    PVOID pBuffer = NULL;
    HKEY hKey;
    DWORD dwType, dwCheck, dwBytes = 0;

    if (!bInitAPI)
    {
        InitializeAPI();
        bInitAPI = TRUE;
    }

    /* Try using GetSystemFirmwareTable (works on NT 5.2 and higher) */
    if (pGetSystemFirmwareTable)
    {
        dwBytes = pGetSystemFirmwareTable('RSMB', 0, NULL, 0);
        if (dwBytes > 0)
        {
            pBuffer = HeapAlloc(GetProcessHeap(), 0, dwBytes);
            if (!pBuffer)
            {
                return NULL;
            }
            dwCheck = pGetSystemFirmwareTable('RSMB', 0, pBuffer, dwBytes);
            if (dwCheck != dwBytes)
            {
                HeapFree(GetProcessHeap(), 0, pBuffer);
                return NULL;
            }
        }
    }
    if (dwBytes == 0)
    {
        /* Try using registry (works on NT 5.1) */
        if (RegOpenKeyW(HKEY_LOCAL_MACHINE,
                        L"SYSTEM\\CurrentControlSet\\Services\\mssmbios\\Data",
                        &hKey) != ERROR_SUCCESS)
        {
            return NULL;
        }

        if (RegQueryValueExW(hKey,
                             L"SMBiosData",
                             NULL,
                             &dwType,
                             NULL,
                             &dwBytes) != ERROR_SUCCESS || dwType != REG_BINARY)
        {
            RegCloseKey(hKey);
            return NULL;
        }

        pBuffer = HeapAlloc(GetProcessHeap(), 0, dwBytes);
        if (!pBuffer)
        {
            RegCloseKey(hKey);
            return NULL;
        }

        if (RegQueryValueExW(hKey,
                             L"SMBiosData",
                             NULL,
                             &dwType,
                             pBuffer,
                             &dwBytes) != ERROR_SUCCESS || dwType != REG_BINARY)
        {
            HeapFree(GetProcessHeap(), 0, pBuffer);
            RegCloseKey(hKey);
            return NULL;
        }

        RegCloseKey(hKey);
    }
    ParseSMBiosTables(pBuffer, dwBytes, Strings);
    return pBuffer;
}

/* Trim converted DMI string */
VOID
TrimDmiStringW(
    _Inout_ PWSTR pStr)
{
    SIZE_T Length;
    UINT i = 0;

    if (!pStr)
        return;

    Length = wcslen(pStr);
    if (Length == 0)
        return;

    /* Trim leading spaces */
    while (i < Length && pStr[i] <= L' ')
    {
        i++;
    }

    if (i > 0)
    {
        Length -= i;
        memmove(pStr, pStr + i, (Length + 1) * sizeof(WCHAR));
    }

    /* Trim trailing spaces */
    while (Length && pStr[Length-1] <= L' ')
    {
        pStr[Length-1] = L'\0';
        --Length;
    }
}

/* Convert string from SMBIOS */
SIZE_T
GetSMBiosStringW(
    _In_ PCSTR DmiString,
    _Out_ PWSTR pBuf,
    _In_ DWORD cchBuf,
    _In_ BOOL bTrim)
{
    SIZE_T cChars;

    if (!DmiString)
    {
        if (cchBuf >= 1)
        {
            *pBuf = 0;
        }
        return 0;
    }

    cChars = MultiByteToWideChar(CP_OEMCP, 0, DmiString, -1, pBuf, cchBuf);

    /* NULL-terminate string */
    pBuf[min(cchBuf-1, cChars)] = L'\0';

    if (bTrim)
    {
        TrimDmiStringW(pBuf);
    }

    return wcslen(pBuf);
}

/* Free SMBIOS Data */
VOID
FreeSMBiosData(
    _In_ PVOID Buffer)
{
    if (!Buffer)
        return;

    HeapFree(GetProcessHeap(), 0, Buffer);
}
