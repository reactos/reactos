/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test for SHGetUserDisplayName
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "shelltest.h"
#include <undocshell.h>

START_TEST(SHGetUserDisplayName)
{
    HRESULT hr;
    WCHAR szBuf[MAX_PATH], szName[MAX_PATH];
    ULONG cchBuf;

    hr = SHGetUserDisplayName(NULL, NULL);
    ok_hex(hr, E_INVALIDARG);

    hr = SHGetUserDisplayName(szBuf, NULL);
    ok_hex(hr, E_INVALIDARG);

    cchBuf = _countof(szBuf);
    hr = SHGetUserDisplayName(NULL, &cchBuf);
    ok_hex(hr, E_INVALIDARG);

    cchBuf = _countof(szBuf);
    hr = SHGetUserDisplayName(szBuf, &cchBuf);
    ok_hex(hr, S_OK);

    cchBuf = _countof(szName);
    GetUserNameW(szName, &cchBuf);

    ok_wstr(szBuf, szName);
}
