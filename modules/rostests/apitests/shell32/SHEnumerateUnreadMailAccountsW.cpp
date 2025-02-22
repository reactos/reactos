/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test for SHEnumerateUnreadMailAccountsW
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "shelltest.h"

START_TEST(SHEnumerateUnreadMailAccountsW)
{
    HRESULT hr;
    WCHAR szMailAddress[MAX_PATH];
    HKEY hKey;
    LSTATUS error;
    DWORD dwDisposition;

    error = RegCreateKeyExW(HKEY_CURRENT_USER,
                            L"Software\\Microsoft\\Windows\\CurrentVersion\\UnreadMail\\example.com",
                            0, NULL, 0, KEY_WRITE, NULL, &hKey, &dwDisposition);
    ok_long(error, ERROR_SUCCESS);

    szMailAddress[0] = UNICODE_NULL;
    hr = SHEnumerateUnreadMailAccountsW(NULL, 0, szMailAddress, _countof(szMailAddress));
    ok_hex(hr, S_OK);
    ok(szMailAddress[0] != UNICODE_NULL, "szMailAddress was empty\n");

    if (dwDisposition == REG_CREATED_NEW_KEY)
    {
        RegDeleteKeyW(HKEY_CURRENT_USER,
                      L"Software\\Microsoft\\Windows\\CurrentVersion\\UnreadMail\\example.com");
    }
}
