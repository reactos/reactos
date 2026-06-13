/*
 * Copyright 2020 Dmitry Timoshkov
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

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "initguid.h"
#include "iads.h"
#include "adshlp.h"
#include "adserr.h"

#include "wine/test.h"

DEFINE_GUID(CLSID_Pathname,0x080d0d78,0xf421,0x11d0,0xa3,0x6e,0x00,0xc0,0x4f,0xb9,0x50,0xdc);

static void test_ADsBuildVarArrayStr(void)
{
    const WCHAR *props[] = { L"prop1", L"prop2" };
    HRESULT hr;
    VARIANT var, item, *data;
    LONG start, end, idx;

    hr = ADsBuildVarArrayStr(NULL, 0, NULL);
    ok(hr == E_ADS_BAD_PARAMETER || hr == E_FAIL /* XP */, "got %#lx\n", hr);

    hr = ADsBuildVarArrayStr(NULL, 0, &var);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(V_VT(&var) == (VT_ARRAY | VT_VARIANT), "got %d\n", V_VT(&var));
    start = 0xdeadbeef;
    hr = SafeArrayGetLBound(V_ARRAY(&var), 1, &start);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(start == 0, "got %ld\n", start);
    end = 0xdeadbeef;
    hr = SafeArrayGetUBound(V_ARRAY(&var), 1, &end);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(end == -1, "got %ld\n", end);
    VariantClear(&var);

    hr = ADsBuildVarArrayStr((LPWSTR *)props, ARRAY_SIZE(props), &var);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(V_VT(&var) == (VT_ARRAY | VT_VARIANT), "got %d\n", V_VT(&var));
    start = 0xdeadbeef;
    hr = SafeArrayGetLBound(V_ARRAY(&var), 1, &start);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(start == 0, "got %ld\n", start);
    end = 0xdeadbeef;
    hr = SafeArrayGetUBound(V_ARRAY(&var), 1, &end);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(end == 1, "got %ld\n", end);
    idx = 0;
    hr = SafeArrayGetElement(V_ARRAY(&var), &idx, &item);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(V_VT(&item) == VT_BSTR, "got %d\n", V_VT(&item));
    ok(!lstrcmpW(V_BSTR(&item), L"prop1"), "got %s\n", wine_dbgstr_w(V_BSTR(&item)));
    VariantClear(&item);
    hr = SafeArrayAccessData(V_ARRAY(&var), (void *)&data);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(V_VT(&data[0]) == VT_BSTR, "got %d\n", V_VT(&data[0]));
    ok(!lstrcmpW(V_BSTR(&data[0]), L"prop1"), "got %s\n", wine_dbgstr_w(V_BSTR(&data[0])));
    ok(V_VT(&data[0]) == VT_BSTR, "got %d\n", V_VT(&data[0]));
    ok(!lstrcmpW(V_BSTR(&data[1]), L"prop2"), "got %s\n", wine_dbgstr_w(V_BSTR(&data[1])));
    hr = SafeArrayUnaccessData(V_ARRAY(&var));
    ok(hr == S_OK, "got %#lx\n", hr);
    VariantClear(&var);
}

static void test_Pathname(void)
{
    static const WCHAR * const elem[3] = { L"a=b",L"c=d",L"e=f" };
    HRESULT hr;
    IADsPathname *path;
    BSTR bstr;
    LONG count, i;

    hr = CoCreateInstance(&CLSID_Pathname, 0, CLSCTX_INPROC_SERVER, &IID_IADsPathname, (void **)&path);
    ok(hr == S_OK, "got %#lx\n", hr);

    count = 0xdeadbeef;
    hr = IADsPathname_GetNumElements(path, &count);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(count == 0, "got %ld\n", count);

    bstr = NULL;
    hr = IADsPathname_Retrieve(path, ADS_FORMAT_X500, &bstr);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(bstr && !wcscmp(bstr, L"LDAP://"), "got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);

    bstr = SysAllocString(L"LDAP://sample");
    hr = IADsPathname_Set(path, bstr, ADS_SETTYPE_FULL);
    ok(hr == S_OK, "got %#lx\n", hr);
    SysFreeString(bstr);

    count = 0xdeadbeef;
    hr = IADsPathname_GetNumElements(path, &count);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(count == 0, "got %ld\n", count);

    hr = IADsPathname_GetElement(path, 0, &bstr);
    ok(hr == HRESULT_FROM_WIN32(ERROR_INVALID_INDEX), "got %#lx\n", hr);
    SysFreeString(bstr);

    bstr = SysAllocString(L"LDAP://sample:123/a=b,c=d,e=f");
    hr = IADsPathname_Set(path, bstr, ADS_SETTYPE_FULL);
    ok(hr == S_OK, "got %#lx\n", hr);
    SysFreeString(bstr);

    count = 0xdeadbeef;
    hr = IADsPathname_GetNumElements(path, &count);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(count == 3, "got %ld\n", count);

    for (i = 0; i < count; i++)
    {
        hr = IADsPathname_GetElement(path, i, &bstr);
        ok(hr == S_OK, "got %#lx\n", hr);
        ok(!wcscmp(bstr, elem[i]), "%lu: %s\n", i, wine_dbgstr_w(bstr));
        SysFreeString(bstr);
    }

    hr = IADsPathname_Retrieve(path, ADS_FORMAT_X500, &bstr);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(!wcscmp(bstr, L"LDAP://sample:123/a=b,c=d,e=f"), "got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);

    hr = IADsPathname_Retrieve(path, ADS_FORMAT_PROVIDER, &bstr);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(!wcscmp(bstr, L"LDAP"), "got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);

    hr = IADsPathname_Retrieve(path, ADS_FORMAT_SERVER, &bstr);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(!wcscmp(bstr, L"sample:123"), "got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);

    hr = IADsPathname_Retrieve(path, ADS_FORMAT_LEAF, &bstr);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(!wcscmp(bstr, L"a=b"), "got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);

    IADsPathname_Release(path);
}

START_TEST(activeds)
{
    CoInitialize(NULL);

    test_Pathname();
    test_ADsBuildVarArrayStr();

    CoUninitialize();
}
