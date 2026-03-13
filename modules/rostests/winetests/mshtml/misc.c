/*
 * Copyright 2006 Jacek Caban for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define COBJMACROS

#include <wine/test.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "initguid.h"
#include "optary.h"

static void test_HTMLLoadOptions(void)
{
    IHtmlLoadOptions *loadopts;
    BYTE buf[100];
    DWORD size, i, data = 0xdeadbeef;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_HTMLLoadOptions, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IHtmlLoadOptions, (void**)&loadopts);
    ok(hres == S_OK, "creating HTMLLoadOptions failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    for(i=0; i <= HTMLLOADOPTION_FRAMELOAD+3; i++) {
        size = 0xdeadbeef;
        memset(buf, 0xdd, sizeof(buf));
        hres = IHtmlLoadOptions_QueryOption(loadopts, i, NULL, &size);
        ok(hres == S_OK, "QueryOption failed: %08x\n", hres);
        ok(size == 0, "size = %d\n", size);
        ok(buf[0] == 0xdd, "buf changed\n");
    }

    size = 0xdeadbeef;
    hres = IHtmlLoadOptions_QueryOption(loadopts, HTMLLOADOPTION_CODEPAGE, NULL, &size);
    ok(hres == S_OK, "QueryOption failed: %08x\n", hres);
    ok(size == 0, "size = %d\n", size);

    hres = IHtmlLoadOptions_SetOption(loadopts, HTMLLOADOPTION_CODEPAGE, &data, sizeof(data));
    ok(hres == S_OK, "SetOption failed: %08x\n", hres);

    size = sizeof(data);
    memset(buf, 0xdd, sizeof(buf));
    hres = IHtmlLoadOptions_QueryOption(loadopts, HTMLLOADOPTION_CODEPAGE, buf, &size);
    ok(hres == S_OK, "QueryOption failed: %08x\n", hres);
    ok(size == sizeof(data), "size = %d\n", size);
    ok(*(DWORD*)buf == data, "unexpected buf\n");

    size = sizeof(data)-1;
    memset(buf, 0xdd, sizeof(buf));
    hres = IHtmlLoadOptions_QueryOption(loadopts, HTMLLOADOPTION_CODEPAGE, buf, &size);
    ok(hres == E_FAIL, "QueryOption failed: %08x\n", hres);
    ok(size == sizeof(data) || !size, "size = %d\n", size);
    ok(buf[0] == 0xdd, "buf changed\n");

    data = 100;
    hres = IHtmlLoadOptions_SetOption(loadopts, HTMLLOADOPTION_CODEPAGE, &data, 0);
    ok(hres == S_OK, "SetOption failed: %08x\n", hres);

    size = 0xdeadbeef; 
    memset(buf, 0xdd, sizeof(buf));
    hres = IHtmlLoadOptions_QueryOption(loadopts, HTMLLOADOPTION_CODEPAGE, buf, &size);
    ok(hres == S_OK, "QueryOption failed: %08x\n", hres);
    ok(size == 0, "size = %d\n", size);
    ok(buf[0] == 0xdd, "buf changed\n");

    hres = IHtmlLoadOptions_SetOption(loadopts, HTMLLOADOPTION_CODEPAGE, NULL, 0);
    ok(hres == S_OK, "SetOption failed: %08x\n", hres);

    hres = IHtmlLoadOptions_SetOption(loadopts, 1000, &data, sizeof(data));
    ok(hres == S_OK, "SetOption failed: %08x\n", hres);

    size = sizeof(data);
    memset(buf, 0xdd, sizeof(buf));
    hres = IHtmlLoadOptions_QueryOption(loadopts, 1000, buf, &size);
    ok(hres == S_OK, "QueryOption failed: %08x\n", hres);
    ok(size == sizeof(data), "size = %d\n", size);
    ok(*(DWORD*)buf == data, "unexpected buf\n");

    hres = IHtmlLoadOptions_SetOption(loadopts, 1000, buf, sizeof(buf));
    ok(hres == S_OK, "SetOption failed: %08x\n", hres);

    size = 0xdeadbeef;
    hres = IHtmlLoadOptions_QueryOption(loadopts, 1000, buf, &size);
    ok(hres == S_OK, "QueryOption failed: %08x\n", hres);
    ok(size == sizeof(buf), "size = %d\n", size);

    IHtmlLoadOptions_Release(loadopts);
}

START_TEST(misc)
{
    CoInitialize(NULL);

    test_HTMLLoadOptions();

    CoUninitialize();
}
