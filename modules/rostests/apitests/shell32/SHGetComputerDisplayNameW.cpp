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

typedef HRESULT (WINAPI *FN_SHGetComputerDisplayNameW)(PWSTR, DWORD, PWSTR, DWORD);
typedef NET_API_STATUS (WINAPI *FN_NetServerGetInfo)(LPWSTR, DWORD, PBYTE*);
typedef NET_API_STATUS (WINAPI *FN_NetApiBufferFree)(PVOID);

static FN_SHGetComputerDisplayNameW s_pSHGetComputerDisplayNameW = NULL;
static FN_NetServerGetInfo s_pNetServerGetInfo = NULL;
static FN_NetApiBufferFree s_pNetApiBufferFree = NULL;

#define COMPUTER_DESCRIPTIONS_KEY \
    L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ComputerDescriptions"

static PCWSTR
SHELL_SkipServerSlashes(
    _In_ PCWSTR pszPath)
{
    PCWSTR pch;
    for (pch = pszPath; *pch == L'\\'; ++pch)
        ;
    return pch;
}

static VOID
SHELL_CacheComputerDescription(
    _In_ PCWSTR pszServerName,
    _In_ PCWSTR pszDesc)
{
    if (!pszDesc)
        return;

    SIZE_T cbDesc = (wcslen(pszDesc) + 1) * sizeof(WCHAR);
    SHSetValueW(HKEY_CURRENT_USER, COMPUTER_DESCRIPTIONS_KEY,
                SHELL_SkipServerSlashes(pszServerName), REG_SZ, pszDesc, (DWORD)cbDesc);
}

static HRESULT
SHELL_GetCachedComputerDescription(
    _Out_writes_z_(cchDescMax) PWSTR pszDesc,
    _In_ DWORD cchDescMax,
    _In_ PCWSTR pszServerName)
{
    cchDescMax *= sizeof(WCHAR);
    DWORD error = SHGetValueW(HKEY_CURRENT_USER, COMPUTER_DESCRIPTIONS_KEY,
                              SHELL_SkipServerSlashes(pszServerName), NULL, pszDesc, &cchDescMax);
    return HRESULT_FROM_WIN32(error);
}

static HRESULT
SHELL_BuildDisplayMachineName(
    _Out_writes_z_(cchNameMax) PWSTR pszName,
    _In_ DWORD cchNameMax,
    _In_ PCWSTR pszServerName,
    _In_ PCWSTR pszDescription)
{
    if (!pszDescription || !*pszDescription)
        return E_FAIL;

    PCWSTR pszFormat = (SHRestricted(REST_ALLOWCOMMENTTOGGLE) ? L"%2 (%1)" : L"%1 (%2)");
    PCWSTR args[] = { pszDescription , SHELL_SkipServerSlashes(pszServerName) };
    return (FormatMessageW(FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_FROM_STRING,
                           pszFormat, 0, 0, pszName, cchNameMax, (va_list *)args) ? S_OK : E_FAIL);
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

    HRESULT hr = SHELL_GetCachedComputerDescription(szDesc, _countof(szDesc), szServerName);
    if (FAILED(hr))
        szDesc[0] = UNICODE_NULL;
    trace("%s\n", wine_dbgstr_w(szDesc));

    StringCchCopyW(szDisplayName, _countof(szDisplayName), L"@");
    hr = s_pSHGetComputerDisplayNameW(NULL, SHGCDN_NOCACHE, szDisplayName, _countof(szDisplayName));
    ok_hex(hr, S_OK);
    trace("%s\n", wine_dbgstr_w(szDisplayName));
    ok_wstr(szDisplayName, szCompName);

    StringCchCopyW(szDisplayName, _countof(szDisplayName), L"@");
    hr = s_pSHGetComputerDisplayNameW(szServerName, 0, szDisplayName, _countof(szDisplayName));
    ok_hex(hr, S_OK);
    trace("%s\n", wine_dbgstr_w(szServerName));
    ok_wstr(szServerName, L"DummyServerName");

    hr = SHELL_BuildDisplayMachineName(szName, _countof(szName), szServerName, szDesc);
    ok_hex(hr, S_OK);

    trace("%s\n", wine_dbgstr_w(szDisplayName));
    trace("%s\n", wine_dbgstr_w(szName));
    ok_wstr(szDisplayName, szName);

    // Delete registry value
    HKEY hKey;
    LSTATUS error = RegOpenKeyExW(HKEY_CURRENT_USER, COMPUTER_DESCRIPTIONS_KEY, 0, KEY_WRITE, &hKey);
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
        skip("Tests on Vista+ will cause exception\n");
        return;
    }

    HINSTANCE hShell32 = GetModuleHandleW(L"shell32.dll");
    s_pSHGetComputerDisplayNameW =
        (FN_SHGetComputerDisplayNameW)GetProcAddress(hShell32, MAKEINTRESOURCEA(752));
    if (!s_pSHGetComputerDisplayNameW)
    {
        skip("SHGetComputerDisplayNameW not found\n");
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
