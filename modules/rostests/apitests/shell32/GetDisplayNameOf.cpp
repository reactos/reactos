/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Test for GetDisplayNameOf
 * COPYRIGHT:   Copyright 2023 Mark Jansen <mark.jansen@reactos.org>
 *              Copyright 2023 Doug Lyons <douglyons@douglyons.com>
 */

#include "shelltest.h"
#include <stdio.h>
#include <shellutils.h>

START_TEST(GetDisplayNameOf)
{
    HRESULT hr;
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    CComPtr<IShellFolder> spPanel;

    hr = CoCreateInstance(CLSID_ControlPanel, NULL, CLSCTX_ALL,
                          IID_PPV_ARG(IShellFolder, &spPanel));
    ok_hr(hr, S_OK);
    if (SUCCEEDED(hr))
    {
        /* We want to know if 'STRRET ret' data was changed when the first
         * parameter of GetDisplayNameOf for the Control Panel is NULL */
        STRRET ret, expected;
        memset(&ret, 'a', sizeof(ret));
        memset(&expected, 'a', sizeof(expected));
        hr = spPanel->GetDisplayNameOf(NULL, SHGDN_NORMAL, &ret);
        /* This verifies that the return value is 'S_FALSE' */
        ok_hex(hr, S_FALSE);
        ok(memcmp(&ret, &expected, sizeof(ret)) == 0, "Data was changed!\n");
        memset(&ret, 'a', sizeof(ret));
        memset(&expected, 'a', sizeof(expected));
        hr = spPanel->GetDisplayNameOf(NULL, SHGDN_FORPARSING, &ret);
        ok_hex(hr, S_FALSE);
        ok(memcmp(&ret, &expected, sizeof(ret)) == 0, "Data was changed!\n");
    }
}
