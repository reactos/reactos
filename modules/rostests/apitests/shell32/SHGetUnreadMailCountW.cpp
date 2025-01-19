/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test for SHGetUnreadMailCountW
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "shelltest.h"

static VOID SetUnreadMailInfo(PDWORD pdwDisposition)
{
    HKEY hKey;
    LSTATUS error = RegCreateKeyExW(
        HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\UnreadMail\\example.com",
        0, NULL, 0, KEY_WRITE, NULL, &hKey, pdwDisposition);
    ok_long(error, ERROR_SUCCESS);

    DWORD dwCount = 1;
    error = SHSetValueW(hKey, NULL, L"MessageCount", REG_DWORD, &dwCount, sizeof(dwCount));
    ok_long(error, ERROR_SUCCESS);

    FILETIME FileTime;
    GetSystemTimeAsFileTime(&FileTime);
    error = SHSetValueW(hKey, NULL, L"TimeStamp", REG_BINARY, &FileTime, sizeof(FileTime));
    ok_long(error, ERROR_SUCCESS);

    LPCWSTR pszApp = L"MyMailerApp";
    DWORD cbValue = (lstrlenW(pszApp) + 1) * sizeof(WCHAR);
    error = SHSetValueW(hKey, NULL, L"Application", REG_SZ, pszApp, cbValue);
    ok_long(error, ERROR_SUCCESS);

    RegCloseKey(hKey);
}

START_TEST(SHGetUnreadMailCountW)
{
    HRESULT hr;

    DWORD dwDisposition;
    SetUnreadMailInfo(&dwDisposition);

    hr = SHGetUnreadMailCountW(NULL, L"example.com", NULL, NULL, NULL, 0);
    ok_hex(hr, S_OK);

    FILETIME FileTime;
    ZeroMemory(&FileTime, sizeof(FileTime));
    hr = SHGetUnreadMailCountW(HKEY_CURRENT_USER, L"example.com", NULL, &FileTime, NULL, 0);
    ok_hex(hr, S_OK);
    ok(FileTime.dwHighDateTime != 0, "FileTime.dwHighDateTime was zero\n");

    DWORD dwCount = 0;
    ZeroMemory(&FileTime, sizeof(FileTime));
    hr = SHGetUnreadMailCountW(NULL, NULL, &dwCount, &FileTime, NULL, 0);
    ok_hex(hr, S_OK);
    ok_long(dwCount, 1);
    ok_long(FileTime.dwHighDateTime, 0);

    dwCount = 0;
    hr = SHGetUnreadMailCountW(NULL, L"example.com", &dwCount, NULL, NULL, 0);
    ok_hex(hr, S_OK);
    ok_long(dwCount, 1);

    hr = SHGetUnreadMailCountW(NULL, NULL, &dwCount, NULL, NULL, 0);
    ok_hex(hr, S_OK);

    WCHAR szAppName[MAX_PATH];
    dwCount = 0;
    hr = SHGetUnreadMailCountW(NULL, NULL, &dwCount, NULL, szAppName, _countof(szAppName));
    ok_hex(hr, E_INVALIDARG);
    ok_long(dwCount, 0);

    hr = SHGetUnreadMailCountW(NULL, L"example.com", NULL, NULL, szAppName, _countof(szAppName));
    ok_hex(hr, S_OK);
    ok_wstr(szAppName, L"MyMailerApp");

    if (dwDisposition == REG_CREATED_NEW_KEY)
    {
        RegDeleteKeyW(HKEY_CURRENT_USER,
                      L"Software\\Microsoft\\Windows\\CurrentVersion\\UnreadMail\\example.com");
    }
}
