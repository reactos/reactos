/*
 * Error Info Tests
 *
 * Copyright 2007 Robert Shearman
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
#define CONST_VTABLE

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"

#include "wine/test.h"

#define ok_ole_success(hr, func) ok(hr == S_OK, func " failed with error %#08lx\n", hr)

static const CLSID CLSID_WineTest =
{ /* 9474ba1a-258b-490b-bc13-516e9239ace0 */
    0x9474ba1a,
    0x258b,
    0x490b,
    {0xbc, 0x13, 0x51, 0x6e, 0x92, 0x39, 0xac, 0xe0}
};

static void test_error_info(void)
{
    HRESULT hr;
    ICreateErrorInfo *pCreateErrorInfo;
    IErrorInfo *pErrorInfo;
    static WCHAR wszDescription[] = {'F','a','i','l','e','d',' ','S','p','r','o','c','k','e','t',0};
    static WCHAR wszHelpFile[] = {'s','p','r','o','c','k','e','t','.','h','l','p',0};
    static WCHAR wszSource[] = {'s','p','r','o','c','k','e','t',0};
    IUnknown *unk;

    hr = CreateErrorInfo(&pCreateErrorInfo);
    ok_ole_success(hr, "CreateErrorInfo");

    hr = ICreateErrorInfo_QueryInterface(pCreateErrorInfo, &IID_IUnknown, (void**)&unk);
    ok_ole_success(hr, "QI");
    IUnknown_Release(unk);

    hr = ICreateErrorInfo_SetDescription(pCreateErrorInfo, NULL);
    ok_ole_success(hr, "ICreateErrorInfo_SetDescription");

    hr = ICreateErrorInfo_SetDescription(pCreateErrorInfo, wszDescription);
    ok_ole_success(hr, "ICreateErrorInfo_SetDescription");

    hr = ICreateErrorInfo_SetGUID(pCreateErrorInfo, &CLSID_WineTest);
    ok_ole_success(hr, "ICreateErrorInfo_SetGUID");

    hr = ICreateErrorInfo_SetHelpContext(pCreateErrorInfo, 0xdeadbeef);
    ok_ole_success(hr, "ICreateErrorInfo_SetHelpContext");

    hr = ICreateErrorInfo_SetHelpFile(pCreateErrorInfo, NULL);
    ok_ole_success(hr, "ICreateErrorInfo_SetHelpFile");

    hr = ICreateErrorInfo_SetHelpFile(pCreateErrorInfo, wszHelpFile);
    ok_ole_success(hr, "ICreateErrorInfo_SetHelpFile");

    hr = ICreateErrorInfo_SetSource(pCreateErrorInfo, NULL);
    ok_ole_success(hr, "ICreateErrorInfo_SetSource");

    hr = ICreateErrorInfo_SetSource(pCreateErrorInfo, wszSource);
    ok_ole_success(hr, "ICreateErrorInfo_SetSource");

    hr = ICreateErrorInfo_QueryInterface(pCreateErrorInfo, &IID_IErrorInfo, (void **)&pErrorInfo);
    ok_ole_success(hr, "ICreateErrorInfo_QueryInterface");

    hr = IErrorInfo_QueryInterface(pErrorInfo, &IID_IUnknown, (void**)&unk);
    ok_ole_success(hr, "QI");
    IUnknown_Release(unk);

    ICreateErrorInfo_Release(pCreateErrorInfo);

    hr = SetErrorInfo(0, pErrorInfo);
    ok_ole_success(hr, "SetErrorInfo");

    IErrorInfo_Release(pErrorInfo);
    pErrorInfo = NULL;

    hr = GetErrorInfo(0, &pErrorInfo);
    ok_ole_success(hr, "GetErrorInfo");

    IErrorInfo_Release(pErrorInfo);

    hr = GetErrorInfo(0, &pErrorInfo);
    ok(hr == S_FALSE, "GetErrorInfo should have returned S_FALSE instead of 0x%08lx\n", hr);
    ok(!pErrorInfo, "pErrorInfo should be set to NULL\n");

    hr = SetErrorInfo(0, NULL);
    ok_ole_success(hr, "SetErrorInfo");

    hr = GetErrorInfo(0xdeadbeef, &pErrorInfo);
    ok(hr == E_INVALIDARG, "GetErrorInfo should have returned E_INVALIDARG instead of 0x%08lx\n", hr);

    hr = SetErrorInfo(0xdeadbeef, NULL);
    ok(hr == E_INVALIDARG, "SetErrorInfo should have returned E_INVALIDARG instead of 0x%08lx\n", hr);
}

START_TEST(errorinfo)
{
    test_error_info();
}
