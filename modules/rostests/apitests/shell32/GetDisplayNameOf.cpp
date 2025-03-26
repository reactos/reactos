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
        STRRET ret, expected;

        memset(&ret, 'a', sizeof(ret));
        memset(&expected, 'a', sizeof(expected));
        hr = spPanel->GetDisplayNameOf(NULL, SHGDN_NORMAL, &ret);

        /* Return value is expected to be 'S_FALSE', which is out-of-spec behavior.
         * The data after function call is expected to be unchanged. */
        ok_hex(hr, S_FALSE);
        ok(memcmp(&ret, &expected, sizeof(ret)) == 0, "Data was changed!\n");

        /* Repeat the same test with SHGDN_FORPARSING */
        memset(&ret, 'a', sizeof(ret));
        memset(&expected, 'a', sizeof(expected));
        hr = spPanel->GetDisplayNameOf(NULL, SHGDN_FORPARSING, &ret);

        ok_hex(hr, S_FALSE);
        ok(memcmp(&ret, &expected, sizeof(ret)) == 0, "Data was changed!\n");
    }
}
