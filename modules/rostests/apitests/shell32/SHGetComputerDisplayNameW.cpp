/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Tests for SHGetComputerDisplayNameW
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "shelltest.h"
#include <lmserver.h>
#include <undocshell.h>
#include <strsafe.h>
#include <versionhelpers.h>

typedef NET_API_STATUS (WINAPI *FN_NetServerGetInfo)(LPWSTR, DWORD, PBYTE*);
typedef NET_API_STATUS (WINAPI *FN_NetApiBufferFree)(PVOID);

static FN_NetServerGetInfo s_pNetServerGetInfo = NULL;
static FN_NetApiBufferFree s_pNetApiBufferFree = NULL;

static PWSTR
SHELL_SkipServerSlashes(
    _In_ PCWSTR pszPath)
{
    PCWSTR pch;
    for (pch = pszPath; *pch == L'\\'; ++pch)
        ;
    return const_cast<PWSTR>(pch);
}

static VOID
SHELL_CacheComputerDescription(
    _In_ PCWSTR pszServerName,
    _In_ PCWSTR pszDesc)
{
    if (!pszDesc)
        return;

    DWORD cbDesc = (lstrlenW(pszDesc) + 1) * sizeof(WCHAR);
    SHSetValueW(
        HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ComputerDescriptions",
        SHELL_SkipServerSlashes(pszServerName), REG_SZ, pszDesc, cbDesc);
}

static HRESULT
SHELL_GetCachedComputerDescription(
    _In_ PCWSTR pszServerName,
    _Out_ PWSTR pszDesc,
    _In_ DWORD cchDescMax)
{
    cchDescMax *= sizeof(WCHAR);

    LSTATUS error = SHGetValueW(HKEY_CURRENT_USER,
                                L"Software\\Microsoft\\Windows\\CurrentVersion\\"
                                L"Explorer\\ComputerDescriptions",
                                SHELL_SkipServerSlashes(pszServerName), NULL, pszDesc, &cchDescMax);
    return HRESULT_FROM_WIN32(error);
}

static HRESULT
SHELL_BuildDisplayMachineName(
    _In_ PCWSTR pszServerName,
    _In_ PCWSTR pszDescription,
    _Out_ PWSTR pszName,
    _In_ DWORD cchNameMax)
{
    if (!pszDescription || !*pszDescription)
        return E_FAIL;

    PCWSTR pszFormat = SHRestricted(REST_ALLOWCOMMENTTOGGLE) ? L"%1 (%2)" : L"%2 (%1)";
    PCWSTR args[] = { SHELL_SkipServerSlashes(pszServerName), pszDescription };
    if (!FormatMessageW(FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_FROM_STRING,
                        pszFormat, 0, 0, pszName, cchNameMax, (va_list *)args))
    {
        return E_FAIL;
    }

    return S_OK;
}

static VOID
TEST_SHGetComputerDisplayNameW(VOID)
{
    WCHAR szCompName[MAX_COMPUTERNAME_LENGTH + 1], szDesc[256], szDisplayName[MAX_PATH];
    WCHAR szName[MAX_PATH], szServerName[] = L"DummyServerName";

    DWORD cchCompName = _countof(szCompName);
    BOOL ret = GetComputerNameW(szCompName, &cchCompName);
    ok_int(ret, TRUE);
    trace("%s\n", wine_dbgstr_w(szCompName));

    SHELL_CacheComputerDescription(szServerName, L"DummyDescription");

    HRESULT hr = SHELL_GetCachedComputerDescription(szServerName, szDesc, _countof(szDesc));
    if (FAILED(hr))
        szDesc[0] = UNICODE_NULL;
    trace("%s\n", wine_dbgstr_w(szDesc));

    StringCchCopyW(szDisplayName, _countof(szDisplayName), L"@");
    hr = SHGetComputerDisplayNameW(NULL, SHGCDN_NOCACHE, szDisplayName, _countof(szDisplayName));
    ok_hex(hr, S_OK);
    trace("%s\n", wine_dbgstr_w(szDisplayName));
    ok_wstr(szDisplayName, szCompName);

    StringCchCopyW(szDisplayName, _countof(szDisplayName), L"@");
    hr = SHGetComputerDisplayNameW(szServerName, 0, szDisplayName, _countof(szDisplayName));
    ok_hex(hr, S_OK);
    trace("%s\n", wine_dbgstr_w(szServerName));
    trace("%s\n", wine_dbgstr_w(szDisplayName));
    ok_wstr(szServerName, L"DummyServerName");

    hr = SHELL_BuildDisplayMachineName(szServerName, szDesc, szName, _countof(szName));
    ok_hex(hr, S_OK);

    trace("%s\n", wine_dbgstr_w(szDisplayName));
    ok_wstr(szDisplayName, szName);

    // Delete registry value
    HKEY hKey;
    LSTATUS error = RegOpenKeyExW(HKEY_CURRENT_USER,
                                  L"Software\\Microsoft\\Windows\\CurrentVersion\\"
                                  L"Explorer\\ComputerDescriptions", 0, KEY_WRITE, &hKey);
    if (error == ERROR_SUCCESS)
    {
        RegDeleteValueW(hKey, L"DummyServerName");
        RegCloseKey(hKey);
    }
}

START_TEST(SHGetComputerDisplayNameW)
{
    if (IsWindowsVistaOrGreater())
    {
        skip("Vista+\n"); // Tests on Vista+ will cause exception
        return;
    }

    HINSTANCE hNetApi32 = LoadLibraryW(L"netapi32.dll");
    if (!hNetApi32)
    {
        skip("netapi32.dll not found\n");
        return;
    }

    s_pNetServerGetInfo = (FN_NetServerGetInfo)GetProcAddress(hNetApi32, "NetServerGetInfo");
    s_pNetApiBufferFree = (FN_NetApiBufferFree)GetProcAddress(hNetApi32, "NetApiBufferFree");
    if (!s_pNetServerGetInfo || !s_pNetApiBufferFree)
    {
        skip("NetServerGetInfo or NetApiBufferFree not found\n");
        FreeLibrary(hNetApi32);
        return;
    }

    TEST_SHGetComputerDisplayNameW();

    FreeLibrary(hNetApi32);
}
