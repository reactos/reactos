/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test for SHSetUnreadMailCountW
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "shelltest.h"

START_TEST(SHSetUnreadMailCountW)
{
    HKEY hKey;
    LSTATUS error;
    DWORD dwDisposition;
    error = RegCreateKeyExW(HKEY_CURRENT_USER,
                            L"Software\\Microsoft\\Windows\\CurrentVersion\\UnreadMail\\example.com",
                            0, NULL, 0, KEY_WRITE, NULL, &hKey, &dwDisposition);
    ok_long(error, ERROR_SUCCESS);
    RegCloseKey(hKey);

    HRESULT hr = SHSetUnreadMailCountW(L"example.com", 1, L"MyMailerApp");
    ok_hex(hr, S_OK);

    error = RegOpenKeyExW(HKEY_CURRENT_USER,
                          L"Software\\Microsoft\\Windows\\CurrentVersion\\UnreadMail\\example.com",
                          0,
                          KEY_READ,
                          &hKey);
    ok_long(error, ERROR_SUCCESS);

    DWORD dwValue, cbValue = sizeof(dwValue);
    error = RegQueryValueExW(hKey, L"MessageCount", NULL, NULL, (PBYTE)&dwValue, &cbValue);
    ok_long(error, ERROR_SUCCESS);
    ok_long(dwValue, 1);

    FILETIME FileTime;
    cbValue = sizeof(FileTime);
    error = RegQueryValueExW(hKey, L"TimeStamp", NULL, NULL, (PBYTE)&FileTime, &cbValue);
    ok_long(error, ERROR_SUCCESS);
    ok(FileTime.dwHighDateTime != 0, "FileTime.dwHighDateTime was zero\n");

    WCHAR szValue[MAX_PATH];
    cbValue = sizeof(szValue);
    error = RegQueryValueExW(hKey, L"Application", NULL, NULL, (PBYTE)szValue, &cbValue);
    ok_long(error, ERROR_SUCCESS);
    ok_wstr(szValue, L"MyMailerApp");

    RegCloseKey(hKey);

    hr = SHSetUnreadMailCountW(L"example.com", 0, L"MyMailerApp");
    ok_hex(hr, S_OK);

    if (dwDisposition == REG_CREATED_NEW_KEY)
    {
        RegDeleteKeyW(HKEY_CURRENT_USER,
                      L"Software\\Microsoft\\Windows\\CurrentVersion\\UnreadMail\\example.com");
    }
}
