/*
 * PROJECT:     ReactOS Zip Shell Extension
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Zip extraction
 * COPYRIGHT:   Copyright 2017 Mark Jansen (mark.jansen@reactos.org)
 */

#include "precomp.h"

static bool GetInterfaceName(const WCHAR* InterfaceString, WCHAR* buf, size_t size)
{
    WCHAR LocalBuf[100];
    DWORD dwType = 0, dwDataSize = size * sizeof(WCHAR);

    if (!SUCCEEDED(StringCchPrintfW(LocalBuf, _countof(LocalBuf), L"Interface\\%s", InterfaceString)))
        return false;

    return SHRegGetValueW(HKEY_CLASSES_ROOT, LocalBuf, NULL, RRF_RT_REG_SZ, &dwType, buf, &dwDataSize) == ERROR_SUCCESS;
}

WCHAR* guid2string(REFCLSID iid)
{
    static WCHAR buf[2][300];
    static int idx = 0;

    idx ^= 1;

    LPOLESTR tmp;
    HRESULT hr = ProgIDFromCLSID(iid, &tmp);
    if (SUCCEEDED(hr))
    {
        wcscpy(buf[idx], tmp);
        CoTaskMemFree(tmp);
        return buf[idx];
    }
    StringFromGUID2(iid, buf[idx], _countof(buf[idx]));
    if (GetInterfaceName(buf[idx], buf[idx], _countof(buf[idx])))
    {
        return buf[idx];
    }
    StringFromGUID2(iid, buf[idx], _countof(buf[idx]));

    return buf[idx];
}
