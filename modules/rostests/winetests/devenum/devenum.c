/*
 * Some unit tests for devenum
 *
 * Copyright (C) 2012 Christian Costa
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

#include "initguid.h"
#include "dshow.h"
#include "dmo.h"
#include "dmodshow.h"
#include "dsound.h"
#include "mmddk.h"
#include "vfw.h"
#include "setupapi.h"
#include "wine/test.h"

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);

static void test_devenum(void)
{
    IEnumMoniker *enum_cat, *enum_moniker;
    ICreateDevEnum* create_devenum;
    IPropertyBag *prop_bag;
    IMoniker *moniker;
    GUID cat_guid, clsid;
    WCHAR *displayname;
    IBindCtx *bindctx;
    HRESULT hr, hr2;
    IUnknown *unk;
    VARIANT var;
    int count;

    hr = CoCreateInstance(&CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC,
                           &IID_ICreateDevEnum, (LPVOID*)&create_devenum);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = ICreateDevEnum_CreateClassEnumerator(create_devenum, &CLSID_ActiveMovieCategories, &enum_cat, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    while (IEnumMoniker_Next(enum_cat, 1, &moniker, NULL) == S_OK)
    {
        hr = IMoniker_BindToStorage(moniker, NULL, NULL, &IID_IPropertyBag, (void **)&prop_bag);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        VariantInit(&var);
        hr = IPropertyBag_Read(prop_bag, L"CLSID", &var, NULL);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        hr = CLSIDFromString(V_BSTR(&var), &cat_guid);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        VariantClear(&var);
        hr = IPropertyBag_Read(prop_bag, L"FriendlyName", &var, NULL);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        if (winetest_debug > 1)
            trace("%s %s:\n", wine_dbgstr_guid(&cat_guid), wine_dbgstr_w(V_BSTR(&var)));

        VariantClear(&var);
        IPropertyBag_Release(prop_bag);
        IMoniker_Release(moniker);

        hr = ICreateDevEnum_CreateClassEnumerator(create_devenum, &cat_guid, &enum_moniker, 0);
        ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);

        if (hr == S_OK)
        {
            count = 0;

            while (IEnumMoniker_Next(enum_moniker, 1, &moniker, NULL) == S_OK)
            {
                hr = IMoniker_GetDisplayName(moniker, NULL, NULL, &displayname);
                ok(hr == S_OK, "Got hr %#lx.\n", hr);

                hr = IMoniker_GetClassID(moniker, NULL);
                ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

                hr = IMoniker_GetClassID(moniker, &clsid);
                ok(hr == S_OK, "Got hr %#lx.\n", hr);
                ok(IsEqualGUID(&clsid, &CLSID_CDeviceMoniker),
                   "Expected CLSID_CDeviceMoniker got %s\n", wine_dbgstr_guid(&clsid));

                VariantInit(&var);
                hr = IMoniker_BindToStorage(moniker, NULL, NULL, &IID_IPropertyBag, (LPVOID*)&prop_bag);
                ok(hr == S_OK, "Got hr %#lx.\n", hr);

                hr = IPropertyBag_Read(prop_bag, L"FriendlyName", &var, NULL);
                ok((hr == S_OK) | (hr == ERROR_KEY_DOES_NOT_EXIST),
					"Got hr %#lx.\n", hr);

                if (winetest_debug > 1)
                    trace("  %s %s\n", wine_dbgstr_w(displayname), wine_dbgstr_w(V_BSTR(&var)));

                hr = IMoniker_BindToObject(moniker, NULL, NULL, &IID_IUnknown, NULL);
                ok(hr == E_POINTER, "Got hr %#lx.\n", hr);

                VariantClear(&var);
                hr = IPropertyBag_Read(prop_bag, L"CLSID", &var, NULL);
                /* Instantiating the WMT Screen Capture Filter crashes on Windows XP. */
                if (hr != S_OK || wcscmp(V_BSTR(&var), L"{31087270-D348-432C-899E-2D2F38FF29A0}"))
                {
                    hr = IMoniker_BindToObject(moniker, NULL, NULL, &IID_IUnknown, (void **)&unk);
                    if (hr == S_OK)
                        IUnknown_Release(unk);
                    hr2 = IMoniker_BindToObject(moniker, NULL, (IMoniker *)0xdeadbeef,
                            &IID_IUnknown, (void **)&unk);
                    if (hr2 == S_OK)
                        IUnknown_Release(unk);
                    ok(hr2 == hr, "Expected hr %#lx, got %#lx.\n", hr, hr2);
                }

                hr = CreateBindCtx(0, &bindctx);
                ok(hr == S_OK, "Got hr %#lx.\n", hr);
                hr = IMoniker_BindToStorage(moniker, bindctx, NULL, &IID_IPropertyBag, (LPVOID*)&prop_bag);
                ok(hr == S_OK, "Got hr %#lx.\n", hr);
                IPropertyBag_Release(prop_bag);
                IBindCtx_Release(bindctx);

                VariantClear(&var);
                CoTaskMemFree(displayname);
                IPropertyBag_Release(prop_bag);
                IMoniker_Release(moniker);
                count++;
            }
            IEnumMoniker_Release(enum_moniker);

            ok(count > 0, "CreateClassEnumerator() returned S_OK but no devices were enumerated.\n");
        }
    }

    IEnumMoniker_Release(enum_cat);
    ICreateDevEnum_Release(create_devenum);
}

static void test_moniker_isequal(void)
{
    ICreateDevEnum *create_devenum = NULL;
    IEnumMoniker *enum_moniker0 = NULL, *enum_moniker1 = NULL;
    IMoniker *moniker0 = NULL, *moniker1 = NULL;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC,
                           &IID_ICreateDevEnum, (LPVOID*)&create_devenum);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = ICreateDevEnum_CreateClassEnumerator(create_devenum, &CLSID_LegacyAmFilterCategory, &enum_moniker0, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    if (IEnumMoniker_Next(enum_moniker0, 1, &moniker0, NULL) == S_OK
            && IEnumMoniker_Next(enum_moniker0, 1, &moniker1, NULL) == S_OK)
    {
        hr = IMoniker_IsEqual(moniker0, moniker1);
        ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

        hr = IMoniker_IsEqual(moniker1, moniker0);
        ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

        IMoniker_Release(moniker0);
        IMoniker_Release(moniker1);
    }
    else
    {
        skip("Cannot get moniker for testing.\n");
    }
    IEnumMoniker_Release(enum_moniker0);

    hr = ICreateDevEnum_CreateClassEnumerator(create_devenum, &CLSID_LegacyAmFilterCategory, &enum_moniker0, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = ICreateDevEnum_CreateClassEnumerator(create_devenum, &CLSID_AudioRendererCategory, &enum_moniker1, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    if (IEnumMoniker_Next(enum_moniker0, 1, &moniker0, NULL) == S_OK
            && IEnumMoniker_Next(enum_moniker1, 1, &moniker1, NULL) == S_OK)
    {
        hr = IMoniker_IsEqual(moniker0, moniker1);
        ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

        hr = IMoniker_IsEqual(moniker1, moniker0);
        ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

        IMoniker_Release(moniker0);
        IMoniker_Release(moniker1);
    }
    else
    {
        skip("Cannot get moniker for testing.\n");
    }
    IEnumMoniker_Release(enum_moniker0);
    IEnumMoniker_Release(enum_moniker1);

    hr = ICreateDevEnum_CreateClassEnumerator(create_devenum, &CLSID_LegacyAmFilterCategory, &enum_moniker0, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = ICreateDevEnum_CreateClassEnumerator(create_devenum, &CLSID_LegacyAmFilterCategory, &enum_moniker1, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    if (IEnumMoniker_Next(enum_moniker0, 1, &moniker0, NULL) == S_OK
            && IEnumMoniker_Next(enum_moniker1, 1, &moniker1, NULL) == S_OK)
    {
        hr = IMoniker_IsEqual(moniker0, moniker1);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        hr = IMoniker_IsEqual(moniker1, moniker0);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        IMoniker_Release(moniker0);
        IMoniker_Release(moniker1);
    }
    else
    {
        skip("Cannot get moniker for testing.\n");
    }
    IEnumMoniker_Release(enum_moniker0);
    IEnumMoniker_Release(enum_moniker1);

    ICreateDevEnum_Release(create_devenum);
}

static BOOL find_moniker(const GUID *class, IMoniker *needle)
{
    ICreateDevEnum *devenum;
    IEnumMoniker *enum_mon;
    IMoniker *mon;
    BOOL found = FALSE;

    CoCreateInstance(&CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC, &IID_ICreateDevEnum, (void **)&devenum);
    if (ICreateDevEnum_CreateClassEnumerator(devenum, class, &enum_mon, 0) == S_OK)
    {
        while (!found && IEnumMoniker_Next(enum_mon, 1, &mon, NULL) == S_OK)
        {
            if (IMoniker_IsEqual(mon, needle) == S_OK)
                found = TRUE;

            IMoniker_Release(mon);
        }

        IEnumMoniker_Release(enum_mon);
    }
    ICreateDevEnum_Release(devenum);
    return found;
}

DEFINE_GUID(CLSID_TestFilter,  0xdeadbeef,0xcf51,0x43e6,0xb6,0xc5,0x29,0x9e,0xa8,0xb6,0xb5,0x91);

static void test_register_filter(void)
{
    IFilterMapper2 *mapper2;
    IMoniker *mon = NULL;
    REGFILTER2 rgf2 = {0};
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_FilterMapper2, NULL, CLSCTX_INPROC, &IID_IFilterMapper2, (void **)&mapper2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    rgf2.dwVersion = 2;
    rgf2.dwMerit = MERIT_UNLIKELY;
    rgf2.cPins2 = 0;


    hr = IFilterMapper2_RegisterFilter(mapper2, &CLSID_TestFilter, L"devenum test", &mon, NULL, NULL, &rgf2);
    if (hr == E_ACCESSDENIED)
    {
        skip("Not enough permissions to register filters\n");
        IFilterMapper2_Release(mapper2);
        return;
    }
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ok(find_moniker(&CLSID_LegacyAmFilterCategory, mon), "filter should be registered\n");

    hr = IFilterMapper2_UnregisterFilter(mapper2, NULL, NULL, &CLSID_TestFilter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ok(!find_moniker(&CLSID_LegacyAmFilterCategory, mon), "filter should not be registered\n");
    IMoniker_Release(mon);

    mon = NULL;
    hr = IFilterMapper2_RegisterFilter(mapper2, &CLSID_TestFilter, L"devenum test", &mon, &CLSID_AudioRendererCategory, NULL, &rgf2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ok(find_moniker(&CLSID_AudioRendererCategory, mon), "filter should be registered\n");

    hr = IFilterMapper2_UnregisterFilter(mapper2, &CLSID_AudioRendererCategory, NULL, &CLSID_TestFilter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ok(!find_moniker(&CLSID_AudioRendererCategory, mon), "filter should not be registered\n");
    IMoniker_Release(mon);

    IFilterMapper2_Release(mapper2);
}

static IMoniker *check_display_name_(int line, IParseDisplayName *parser, WCHAR *buffer)
{
    IMoniker *mon;
    ULONG eaten;
    HRESULT hr;
    WCHAR *str;

    hr = IParseDisplayName_ParseDisplayName(parser, NULL, buffer, &eaten, &mon);
    ok_(__FILE__, line)(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IMoniker_GetDisplayName(mon, NULL, NULL, &str);
    ok_(__FILE__, line)(hr == S_OK, "Got hr %#lx.\n", hr);
    ok_(__FILE__, line)(!wcscmp(str, buffer), "got %s\n", wine_dbgstr_w(str));

    CoTaskMemFree(str);

    return mon;
}
#define check_display_name(parser, buffer) check_display_name_(__LINE__, parser, buffer)

static void test_directshow_filter(void)
{
    SAFEARRAYBOUND bound = {.cElements = 10};
    IParseDisplayName *parser;
    IPropertyBag *prop_bag;
    void *array_data;
    IMoniker *mon;
    WCHAR buffer[200];
    LRESULT res;
    VARIANT var;
    HRESULT hr;

    /* Test ParseDisplayName and GetDisplayName */
    hr = CoCreateInstance(&CLSID_CDeviceMoniker, NULL, CLSCTX_INPROC, &IID_IParseDisplayName, (void **)&parser);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    wcscpy(buffer, L"@device:sw:");
    StringFromGUID2(&CLSID_AudioRendererCategory, buffer + wcslen(buffer), CHARS_IN_GUID);
    wcscat(buffer, L"\\test");
    mon = check_display_name(parser, buffer);

    /* Test writing and reading from the property bag */
    ok(!find_moniker(&CLSID_AudioRendererCategory, mon), "filter should not be registered\n");

    hr = IMoniker_BindToStorage(mon, NULL, NULL, &IID_IPropertyBag, (void **)&prop_bag);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    VariantInit(&var);
    hr = IPropertyBag_Read(prop_bag, L"FriendlyName", &var, NULL);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), "Got hr %#lx.\n", hr);

    /* writing causes the key to be created */
    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(L"test");
    hr = IPropertyBag_Write(prop_bag, L"FriendlyName", &var);
    if (hr != E_ACCESSDENIED)
    {
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        ok(find_moniker(&CLSID_AudioRendererCategory, mon), "filter should be registered\n");

        VariantClear(&var);
        V_VT(&var) = VT_EMPTY;
        hr = IPropertyBag_Read(prop_bag, L"FriendlyName", &var, NULL);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(V_VT(&var) == VT_BSTR, "Got type %#x.\n", V_VT(&var));
        ok(!wcscmp(V_BSTR(&var), L"test"), "Got name %s.\n", wine_dbgstr_w(V_BSTR(&var)));

        VariantClear(&var);
        V_VT(&var) = VT_LPWSTR;
        hr = IPropertyBag_Read(prop_bag, L"FriendlyName", &var, NULL);
        ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

        V_VT(&var) = VT_BSTR;
        hr = IPropertyBag_Read(prop_bag, L"FriendlyName", &var, NULL);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(V_VT(&var) == VT_BSTR, "Got type %#x.\n", V_VT(&var));
        ok(!wcscmp(V_BSTR(&var), L"test"), "Got name %s.\n", wine_dbgstr_w(V_BSTR(&var)));

        V_VT(&var) = VT_LPWSTR;
        hr = IPropertyBag_Write(prop_bag, L"FriendlyName", &var);
        ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
        VariantClear(&var);

        V_VT(&var) = VT_I4;
        V_I4(&var) = 0xdeadbeef;
        hr = IPropertyBag_Write(prop_bag, L"foobar", &var);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        V_VT(&var) = VT_EMPTY;
        hr = IPropertyBag_Read(prop_bag, L"foobar", &var, NULL);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(V_VT(&var) == VT_I4, "Got type %#x.\n", V_VT(&var));
        ok(V_I4(&var) == 0xdeadbeef, "Got value %#lx.\n", V_I4(&var));

        V_VT(&var) = VT_UI4;
        hr = IPropertyBag_Read(prop_bag, L"foobar", &var, NULL);
        ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
        V_VT(&var) = VT_BSTR;
        hr = IPropertyBag_Read(prop_bag, L"foobar", &var, NULL);
        ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

        V_VT(&var) = VT_I4;
        hr = IPropertyBag_Read(prop_bag, L"foobar", &var, NULL);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(V_VT(&var) == VT_I4, "Got type %#x.\n", V_VT(&var));
        ok(V_I4(&var) == 0xdeadbeef, "Got value %#lx.\n", V_I4(&var));

        V_VT(&var) = VT_UI4;
        hr = IPropertyBag_Write(prop_bag, L"foobar", &var);
        ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

        V_VT(&var) = VT_ARRAY | VT_UI1;
        V_ARRAY(&var) = SafeArrayCreate(VT_UI1, 1, &bound);
        SafeArrayAccessData(V_ARRAY(&var), &array_data);
        memcpy(array_data, "test data", 10);
        SafeArrayUnaccessData(V_ARRAY(&var));
        hr = IPropertyBag_Write(prop_bag, L"foobar", &var);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        VariantClear(&var);
        V_VT(&var) = VT_EMPTY;
        hr = IPropertyBag_Read(prop_bag, L"foobar", &var, NULL);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(V_VT(&var) == (VT_ARRAY | VT_UI1), "Got type %#x.\n", V_VT(&var));
        SafeArrayAccessData(V_ARRAY(&var), &array_data);
        ok(!memcmp(array_data, "test data", 10), "Got wrong data.\n");
        SafeArrayUnaccessData(V_ARRAY(&var));

        IMoniker_Release(mon);

        /* devenum doesn't give us a way to unregister—we have to do that manually */
        wcscpy(buffer, L"CLSID\\");
        StringFromGUID2(&CLSID_AudioRendererCategory, buffer + wcslen(buffer), CHARS_IN_GUID);
        wcscat(buffer, L"\\Instance\\test");
        res = RegDeleteKeyW(HKEY_CLASSES_ROOT, buffer);
        ok(!res, "Failed to delete key, error %Iu.\n", res);
    }

    VariantClear(&var);
    IPropertyBag_Release(prop_bag);

    /* name can be anything */

    wcscpy(buffer, L"@device:sw:test");
    mon = check_display_name(parser, buffer);

    hr = IMoniker_BindToStorage(mon, NULL, NULL, &IID_IPropertyBag, (void **)&prop_bag);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    VariantClear(&var);
    hr = IPropertyBag_Read(prop_bag, L"FriendlyName", &var, NULL);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), "Got hr %#lx.\n", hr);

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(L"test");
    hr = IPropertyBag_Write(prop_bag, L"FriendlyName", &var);
    if (hr != E_ACCESSDENIED)
    {
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        VariantClear(&var);
        hr = IPropertyBag_Read(prop_bag, L"FriendlyName", &var, NULL);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(!wcscmp(V_BSTR(&var), L"test"), "got %s\n", wine_dbgstr_w(V_BSTR(&var)));

        IMoniker_Release(mon);

        /* vista+ stores it inside the Instance key */
        RegDeleteKeyA(HKEY_CLASSES_ROOT, "CLSID\\test\\Instance");

        res = RegDeleteKeyA(HKEY_CLASSES_ROOT, "CLSID\\test");
        ok(!res, "Failed to delete key, error %Iu.\n", res);
    }

    VariantClear(&var);
    IPropertyBag_Release(prop_bag);
    IParseDisplayName_Release(parser);
}

static void test_codec(void)
{
    SAFEARRAYBOUND bound = {.cElements = 10};
    IParseDisplayName *parser;
    IPropertyBag *prop_bag;
    void *array_data;
    IMoniker *mon;
    WCHAR buffer[200];
    VARIANT var;
    HRESULT hr;

    /* Test ParseDisplayName and GetDisplayName */
    hr = CoCreateInstance(&CLSID_CDeviceMoniker, NULL, CLSCTX_INPROC, &IID_IParseDisplayName, (void **)&parser);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    wcscpy(buffer, L"@device:cm:");
    StringFromGUID2(&CLSID_AudioRendererCategory, buffer + wcslen(buffer), CHARS_IN_GUID);
    wcscat(buffer, L"\\test");
    mon = check_display_name(parser, buffer);

    /* Test writing and reading from the property bag */
    ok(!find_moniker(&CLSID_AudioRendererCategory, mon), "codec should not be registered\n");

    hr = IMoniker_BindToStorage(mon, NULL, NULL, &IID_IPropertyBag, (void **)&prop_bag);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    VariantInit(&var);
    hr = IPropertyBag_Read(prop_bag, L"FriendlyName", &var, NULL);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), "Got hr %#lx.\n", hr);

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(L"test");
    hr = IPropertyBag_Write(prop_bag, L"FriendlyName", &var);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    V_VT(&var) = VT_LPWSTR;
    hr = IPropertyBag_Write(prop_bag, L"FriendlyName", &var);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    VariantClear(&var);
    V_VT(&var) = VT_EMPTY;
    hr = IPropertyBag_Read(prop_bag, L"FriendlyName", &var, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(V_VT(&var) == VT_BSTR, "Got type %#x.\n", V_VT(&var));
    ok(!wcscmp(V_BSTR(&var), L"test"), "Got name %s.\n", wine_dbgstr_w(V_BSTR(&var)));

    VariantClear(&var);
    V_VT(&var) = VT_LPWSTR;
    hr = IPropertyBag_Read(prop_bag, L"FriendlyName", &var, NULL);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    V_VT(&var) = VT_BSTR;
    hr = IPropertyBag_Read(prop_bag, L"FriendlyName", &var, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(V_VT(&var) == VT_BSTR, "Got type %#x.\n", V_VT(&var));
    ok(!wcscmp(V_BSTR(&var), L"test"), "Got name %s.\n", wine_dbgstr_w(V_BSTR(&var)));

    V_VT(&var) = VT_I4;
    V_I4(&var) = 0xdeadbeef;
    hr = IPropertyBag_Write(prop_bag, L"foobar", &var);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    V_VT(&var) = VT_EMPTY;
    hr = IPropertyBag_Read(prop_bag, L"foobar", &var, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(V_VT(&var) == VT_I4, "Got type %#x.\n", V_VT(&var));
    ok(V_I4(&var) == 0xdeadbeef, "Got value %#lx.\n", V_I4(&var));

    V_VT(&var) = VT_UI4;
    hr = IPropertyBag_Read(prop_bag, L"foobar", &var, NULL);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    V_VT(&var) = VT_BSTR;
    hr = IPropertyBag_Read(prop_bag, L"foobar", &var, NULL);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    V_VT(&var) = VT_I4;
    hr = IPropertyBag_Read(prop_bag, L"foobar", &var, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(V_VT(&var) == VT_I4, "Got type %#x.\n", V_VT(&var));
    ok(V_I4(&var) == 0xdeadbeef, "Got value %#lx.\n", V_I4(&var));

    V_VT(&var) = VT_UI4;
    hr = IPropertyBag_Write(prop_bag, L"foobar", &var);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    V_VT(&var) = VT_ARRAY | VT_UI1;
    V_ARRAY(&var) = SafeArrayCreate(VT_UI1, 1, &bound);
    SafeArrayAccessData(V_ARRAY(&var), &array_data);
    memcpy(array_data, "test data", 10);
    SafeArrayUnaccessData(V_ARRAY(&var));
    hr = IPropertyBag_Write(prop_bag, L"foobar", &var);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    /* unlike DirectShow filters, these are automatically generated, so
     * enumerating them will destroy the key */
    ok(!find_moniker(&CLSID_AudioRendererCategory, mon), "codec should not be registered\n");

    VariantClear(&var);
    hr = IPropertyBag_Read(prop_bag, L"FriendlyName", &var, NULL);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), "Got hr %#lx.\n", hr);

    IPropertyBag_Release(prop_bag);
    IMoniker_Release(mon);

    IParseDisplayName_Release(parser);
}

static void test_dmo(const GUID *dmo_category, const GUID *enum_category)
{
    IParseDisplayName *parser;
    IPropertyBag *prop_bag;
    IBaseFilter *filter;
    IMediaObject *dmo;
    IEnumDMO *enumdmo;
    WCHAR buffer[200];
    IMoniker *mon;
    VARIANT var;
    WCHAR *name;
    HRESULT hr;
    GUID clsid;

    hr = CoCreateInstance(&CLSID_CDeviceMoniker, NULL, CLSCTX_INPROC, &IID_IParseDisplayName, (void **)&parser);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    wcscpy(buffer, L"@device:dmo:");
    StringFromGUID2(&CLSID_TestFilter, buffer + wcslen(buffer), CHARS_IN_GUID);
    StringFromGUID2(dmo_category, buffer + wcslen(buffer), CHARS_IN_GUID);
    mon = check_display_name(parser, buffer);

    ok(!find_moniker(enum_category, mon), "DMO should not be registered\n");

    hr = IMoniker_BindToStorage(mon, NULL, NULL, &IID_IPropertyBag, (void **)&prop_bag);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    VariantInit(&var);
    hr = IPropertyBag_Read(prop_bag, L"FriendlyName", &var, NULL);
    ok(hr == E_FAIL, "Got hr %#lx.\n", hr);

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(L"devenum test");
    hr = IPropertyBag_Write(prop_bag, L"FriendlyName", &var);
    ok(hr == E_ACCESSDENIED, "Got hr %#lx.\n", hr);

    hr = DMORegister(L"devenum test", &CLSID_TestFilter, dmo_category, 0, 0, NULL, 0, NULL);
    if (hr != E_FAIL)
    {
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        ok(find_moniker(enum_category, mon), "DMO should be registered\n");

        VariantClear(&var);
        hr = IPropertyBag_Read(prop_bag, L"FriendlyName", &var, NULL);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(!wcscmp(V_BSTR(&var), L"devenum test"), "got %s\n", wine_dbgstr_w(V_BSTR(&var)));

        VariantClear(&var);
        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = SysAllocString(L"devenum test");
        hr = IPropertyBag_Write(prop_bag, L"FriendlyName", &var);
        ok(hr == E_ACCESSDENIED, "Got hr %#lx.\n", hr);

        VariantClear(&var);
        hr = IPropertyBag_Read(prop_bag, L"CLSID", &var, NULL);
        ok(hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND), "Got hr %#lx.\n", hr);

        hr = DMOUnregister(&CLSID_TestFilter, dmo_category);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
    }
    IPropertyBag_Release(prop_bag);
    IMoniker_Release(mon);

    hr = DMOEnum(&DMOCATEGORY_AUDIO_DECODER, 0, 0, NULL, 0, NULL, &enumdmo);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    while (IEnumDMO_Next(enumdmo, 1, &clsid, &name, NULL) == S_OK)
    {
        wcscpy(buffer, L"@device:dmo:");
        StringFromGUID2(&clsid, buffer + wcslen(buffer), CHARS_IN_GUID);
        StringFromGUID2(&DMOCATEGORY_AUDIO_DECODER, buffer + wcslen(buffer), CHARS_IN_GUID);
        mon = check_display_name(parser, buffer);
        ok(find_moniker(&DMOCATEGORY_AUDIO_DECODER, mon), "DMO was not found.\n");

        hr = IMoniker_BindToStorage(mon, NULL, NULL, &IID_IPropertyBag, (void **)&prop_bag);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        VariantClear(&var);
        hr = IPropertyBag_Read(prop_bag, L"FriendlyName", &var, NULL);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(!wcscmp(V_BSTR(&var), name), "got %s\n", wine_dbgstr_w(V_BSTR(&var)));

        VariantClear(&var);
        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = SysAllocString(L"devenum test");
        hr = IPropertyBag_Write(prop_bag, L"FriendlyName", &var);
        ok(hr == E_ACCESSDENIED, "Got hr %#lx.\n", hr);

        VariantClear(&var);
        hr = IPropertyBag_Read(prop_bag, L"CLSID", &var, NULL);
        ok(hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND), "Got hr %#lx.\n", hr);

        IPropertyBag_Release(prop_bag);
        CoTaskMemFree(name);

        hr = IMoniker_BindToObject(mon, NULL, NULL, &IID_IBaseFilter, (void **)&filter);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        hr = IBaseFilter_GetClassID(filter, &clsid);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(IsEqualGUID(&clsid, &CLSID_DMOWrapperFilter), "Got CLSID %s.\n", debugstr_guid(&clsid));

        hr = IBaseFilter_QueryInterface(filter, &IID_IMediaObject, (void **)&dmo);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        IMediaObject_Release(dmo);

        IBaseFilter_Release(filter);
    }
    IEnumDMO_Release(enumdmo);

    IParseDisplayName_Release(parser);
}

static void test_legacy_filter(void)
{
    IParseDisplayName *parser;
    IPropertyBag *prop_bag;
    IFilterMapper *mapper;
    IMoniker *mon;
    WCHAR buffer[200];
    VARIANT var;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_CDeviceMoniker, NULL, CLSCTX_INPROC, &IID_IParseDisplayName, (void **)&parser);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = CoCreateInstance(&CLSID_FilterMapper2, NULL, CLSCTX_INPROC, &IID_IFilterMapper, (void **)&mapper);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IFilterMapper_RegisterFilter(mapper, CLSID_TestFilter, L"test", 0xdeadbeef);
    if (hr == VFW_E_BAD_KEY)
    {
        win_skip("not enough permissions to register filters\n");
        goto end;
    }
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    wcscpy(buffer, L"@device:cm:");
    StringFromGUID2(&CLSID_LegacyAmFilterCategory, buffer + wcslen(buffer), CHARS_IN_GUID);
    wcscat(buffer, L"\\");
    StringFromGUID2(&CLSID_TestFilter, buffer + wcslen(buffer), CHARS_IN_GUID);

    mon = check_display_name(parser, buffer);
    ok(find_moniker(&CLSID_LegacyAmFilterCategory, mon), "filter should be registered\n");

    hr = IMoniker_BindToStorage(mon, NULL, NULL, &IID_IPropertyBag, (void **)&prop_bag);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    VariantInit(&var);
    hr = IPropertyBag_Read(prop_bag, L"FriendlyName", &var, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    StringFromGUID2(&CLSID_TestFilter, buffer, CHARS_IN_GUID);
    ok(!wcscmp(buffer, V_BSTR(&var)), "expected %s, got %s\n",
        wine_dbgstr_w(buffer), wine_dbgstr_w(V_BSTR(&var)));

    VariantClear(&var);
    hr = IPropertyBag_Read(prop_bag, L"CLSID", &var, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!wcscmp(buffer, V_BSTR(&var)), "expected %s, got %s\n",
        wine_dbgstr_w(buffer), wine_dbgstr_w(V_BSTR(&var)));

    VariantClear(&var);
    IPropertyBag_Release(prop_bag);

    hr = IFilterMapper_UnregisterFilter(mapper, CLSID_TestFilter);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ok(!find_moniker(&CLSID_LegacyAmFilterCategory, mon), "filter should not be registered\n");
    IMoniker_Release(mon);

end:
    IFilterMapper_Release(mapper);
    IParseDisplayName_Release(parser);
}

static BOOL CALLBACK test_dsound(GUID *guid, const WCHAR *desc, const WCHAR *module, void *context)
{
    IParseDisplayName *parser;
    IPropertyBag *prop_bag;
    IMoniker *mon;
    WCHAR buffer[200];
    WCHAR name[200];
    VARIANT var;
    HRESULT hr;

    if (guid)
    {
        wcscpy(name, L"DirectSound: ");
        wcscat(name, desc);
    }
    else
    {
        wcscpy(name, L"Default DirectSound Device");
        guid = (GUID *)&GUID_NULL;
    }

    hr = CoCreateInstance(&CLSID_CDeviceMoniker, NULL, CLSCTX_INPROC, &IID_IParseDisplayName, (void **)&parser);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    wcscpy(buffer, L"@device:cm:");
    StringFromGUID2(&CLSID_AudioRendererCategory, buffer + wcslen(buffer), CHARS_IN_GUID);
    wcscat(buffer, L"\\");
    wcscat(buffer, name);

    mon = check_display_name(parser, buffer);

    hr = IMoniker_BindToStorage(mon, NULL, NULL, &IID_IPropertyBag, (void **)&prop_bag);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    VariantInit(&var);
    hr = IPropertyBag_Read(prop_bag, L"FriendlyName", &var, NULL);
    if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
    {
        /* Win8+ uses the GUID instead of the device name */
        IPropertyBag_Release(prop_bag);
        IMoniker_Release(mon);

        wcscpy(buffer, L"@device:cm:");
        StringFromGUID2(&CLSID_AudioRendererCategory, buffer + wcslen(buffer), CHARS_IN_GUID);
        wcscat(buffer, L"\\DirectSound: ");
        StringFromGUID2(guid, buffer + wcslen(buffer) - 1, CHARS_IN_GUID);

        mon = check_display_name(parser, buffer);

        hr = IMoniker_BindToStorage(mon, NULL, NULL, &IID_IPropertyBag, (void **)&prop_bag);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        VariantInit(&var);
        hr = IPropertyBag_Read(prop_bag, L"FriendlyName", &var, NULL);
    }
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ok(!wcscmp(name, V_BSTR(&var)), "expected %s, got %s\n",
        wine_dbgstr_w(name), wine_dbgstr_w(V_BSTR(&var)));

    VariantClear(&var);
    hr = IPropertyBag_Read(prop_bag, L"CLSID", &var, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    StringFromGUID2(&CLSID_DSoundRender, buffer, CHARS_IN_GUID);
    ok(!wcscmp(buffer, V_BSTR(&var)), "expected %s, got %s\n",
        wine_dbgstr_w(buffer), wine_dbgstr_w(V_BSTR(&var)));

    VariantClear(&var);
    hr = IPropertyBag_Read(prop_bag, L"DSGuid", &var, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    StringFromGUID2(guid, buffer, CHARS_IN_GUID);
    ok(!wcscmp(buffer, V_BSTR(&var)), "expected %s, got %s\n",
        wine_dbgstr_w(buffer), wine_dbgstr_w(V_BSTR(&var)));

    VariantClear(&var);
    IPropertyBag_Release(prop_bag);
    IMoniker_Release(mon);
    IParseDisplayName_Release(parser);
    return TRUE;
}

static void test_waveout(void)
{
    IParseDisplayName *parser;
    IPropertyBag *prop_bag;
    IMoniker *mon;
    WCHAR endpoint[200];
    WAVEOUTCAPSW caps;
    WCHAR buffer[200];
    const WCHAR *name;
    MMRESULT mmr;
    int count, i;
    VARIANT var;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_CDeviceMoniker, NULL, CLSCTX_INPROC, &IID_IParseDisplayName, (void **)&parser);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    count = waveOutGetNumDevs();

    for (i = -1; i < count; i++)
    {
        waveOutGetDevCapsW(i, &caps, sizeof(caps));

        if (i == -1)    /* WAVE_MAPPER */
            name = L"Default WaveOut Device";
        else
            name = caps.szPname;

        wcscpy(buffer, L"@device:cm:");
        StringFromGUID2(&CLSID_AudioRendererCategory, buffer + wcslen(buffer), CHARS_IN_GUID);
        wcscat(buffer, L"\\");
        wcscat(buffer, name);

        mon = check_display_name(parser, buffer);

        hr = IMoniker_BindToStorage(mon, NULL, NULL, &IID_IPropertyBag, (void **)&prop_bag);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        VariantInit(&var);
        hr = IPropertyBag_Read(prop_bag, L"FriendlyName", &var, NULL);
        if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
        {
            IPropertyBag_Release(prop_bag);
            IMoniker_Release(mon);

            /* Win8+ uses the endpoint GUID instead of the device name */
            mmr = waveOutMessage((HWAVEOUT)(DWORD_PTR) i, DRV_QUERYFUNCTIONINSTANCEID,
                                 (DWORD_PTR) endpoint, sizeof(endpoint));
            ok(!mmr, "waveOutMessage failed: %u\n", mmr);

            wcscpy(buffer, L"@device:cm:");
            StringFromGUID2(&CLSID_AudioRendererCategory, buffer + wcslen(buffer), CHARS_IN_GUID);
            wcscat(buffer, L"\\wave:");
            wcscat(buffer, wcschr(endpoint, '}') + 2);

            mon = check_display_name(parser, buffer);

            hr = IMoniker_BindToStorage(mon, NULL, NULL, &IID_IPropertyBag, (void **)&prop_bag);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);

            hr = IPropertyBag_Read(prop_bag, L"FriendlyName", &var, NULL);
        }
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        ok(!wcsncmp(name, V_BSTR(&var), wcslen(name)), "expected %s, got %s\n",
            wine_dbgstr_w(name), wine_dbgstr_w(V_BSTR(&var)));

        VariantClear(&var);
        hr = IPropertyBag_Read(prop_bag, L"CLSID", &var, NULL);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        StringFromGUID2(&CLSID_AudioRender, buffer, CHARS_IN_GUID);
        ok(!wcscmp(buffer, V_BSTR(&var)), "expected %s, got %s\n",
            wine_dbgstr_w(buffer), wine_dbgstr_w(V_BSTR(&var)));

        VariantClear(&var);
        hr = IPropertyBag_Read(prop_bag, L"WaveOutId", &var, NULL);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        ok(V_I4(&var) == i, "Expected id %d, got %ld.\n", i, V_I4(&var));

        IPropertyBag_Release(prop_bag);
        IMoniker_Release(mon);
    }

    IParseDisplayName_Release(parser);
}

static void test_wavein(void)
{
    IParseDisplayName *parser;
    IPropertyBag *prop_bag;
    IMoniker *mon;
    WCHAR endpoint[200];
    WCHAR buffer[200];
    WAVEINCAPSW caps;
    MMRESULT mmr;
    int count, i;
    VARIANT var;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_CDeviceMoniker, NULL, CLSCTX_INPROC, &IID_IParseDisplayName, (void **)&parser);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    count = waveInGetNumDevs();

    for (i = 0; i < count; i++)
    {
        waveInGetDevCapsW(i, &caps, sizeof(caps));

        wcscpy(buffer, L"@device:cm:");
        StringFromGUID2(&CLSID_AudioInputDeviceCategory, buffer + wcslen(buffer), CHARS_IN_GUID);
        wcscat(buffer, L"\\");
        wcscat(buffer, caps.szPname);

        mon = check_display_name(parser, buffer);

        hr = IMoniker_BindToStorage(mon, NULL, NULL, &IID_IPropertyBag, (void **)&prop_bag);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        VariantInit(&var);
        hr = IPropertyBag_Read(prop_bag, L"FriendlyName", &var, NULL);
        if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
        {
            IPropertyBag_Release(prop_bag);
            IMoniker_Release(mon);

            /* Win8+ uses the endpoint GUID instead of the device name */
            mmr = waveInMessage((HWAVEIN)(DWORD_PTR) i, DRV_QUERYFUNCTIONINSTANCEID,
                                (DWORD_PTR) endpoint, sizeof(endpoint));
            ok(!mmr, "waveInMessage failed: %u\n", mmr);

            wcscpy(buffer, L"@device:cm:");
            StringFromGUID2(&CLSID_AudioInputDeviceCategory, buffer + wcslen(buffer), CHARS_IN_GUID);
            wcscat(buffer, L"\\wave:");
            wcscat(buffer, wcschr(endpoint, '}') + 2);

            mon = check_display_name(parser, buffer);

            hr = IMoniker_BindToStorage(mon, NULL, NULL, &IID_IPropertyBag, (void **)&prop_bag);
            ok(hr == S_OK, "Got hr %#lx.\n", hr);

            hr = IPropertyBag_Read(prop_bag, L"FriendlyName", &var, NULL);
        }
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        ok(!wcsncmp(caps.szPname, V_BSTR(&var), wcslen(caps.szPname)), "expected %s, got %s\n",
            wine_dbgstr_w(caps.szPname), wine_dbgstr_w(V_BSTR(&var)));

        VariantClear(&var);
        hr = IPropertyBag_Read(prop_bag, L"CLSID", &var, NULL);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        StringFromGUID2(&CLSID_AudioRecord, buffer, CHARS_IN_GUID);
        ok(!wcscmp(buffer, V_BSTR(&var)), "expected %s, got %s\n",
            wine_dbgstr_w(buffer), wine_dbgstr_w(V_BSTR(&var)));

        VariantClear(&var);
        hr = IPropertyBag_Read(prop_bag, L"WaveInId", &var, NULL);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        ok(V_I4(&var) == i, "Expected id %d, got %ld.\n", i, V_I4(&var));

        IPropertyBag_Release(prop_bag);
        IMoniker_Release(mon);
    }

    IParseDisplayName_Release(parser);
}

static void test_midiout(void)
{
    IParseDisplayName *parser;
    IPropertyBag *prop_bag;
    IMoniker *mon;
    MIDIOUTCAPSW caps;
    WCHAR buffer[200];
    const WCHAR *name;
    int count, i;
    VARIANT var;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_CDeviceMoniker, NULL, CLSCTX_INPROC, &IID_IParseDisplayName, (void **)&parser);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    count = midiOutGetNumDevs();

    for (i = -1; i < count; i++)
    {
        midiOutGetDevCapsW(i, &caps, sizeof(caps));

        if (i == -1)    /* MIDI_MAPPER */
            name = L"Default MidiOut Device";
        else
            name = caps.szPname;

        wcscpy(buffer, L"@device:cm:");
        StringFromGUID2(&CLSID_MidiRendererCategory, buffer + wcslen(buffer), CHARS_IN_GUID);
        wcscat(buffer, L"\\");
        wcscat(buffer, name);

        mon = check_display_name(parser, buffer);

        hr = IMoniker_BindToStorage(mon, NULL, NULL, &IID_IPropertyBag, (void **)&prop_bag);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        VariantInit(&var);
        hr = IPropertyBag_Read(prop_bag, L"FriendlyName", &var, NULL);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        ok(!wcscmp(name, V_BSTR(&var)), "expected %s, got %s\n",
            wine_dbgstr_w(name), wine_dbgstr_w(V_BSTR(&var)));

        VariantClear(&var);
        hr = IPropertyBag_Read(prop_bag, L"CLSID", &var, NULL);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        StringFromGUID2(&CLSID_AVIMIDIRender, buffer, CHARS_IN_GUID);
        ok(!wcscmp(buffer, V_BSTR(&var)), "expected %s, got %s\n",
            wine_dbgstr_w(buffer), wine_dbgstr_w(V_BSTR(&var)));

        VariantClear(&var);
        hr = IPropertyBag_Read(prop_bag, L"MidiOutId", &var, NULL);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        ok(V_I4(&var) == i, "Expected id %d, got %ld.\n", i, V_I4(&var));

        IPropertyBag_Release(prop_bag);
        IMoniker_Release(mon);
    }

    IParseDisplayName_Release(parser);
}

static void test_vfw(void)
{
    IParseDisplayName *parser;
    IPropertyBag *prop_bag;
    IMoniker *mon;
    WCHAR buffer[200];
    ICINFO info;
    VARIANT var;
    HRESULT hr;
    int i = 0;
    HIC hic;

    if (broken(sizeof(void *) == 8))
    {
        win_skip("VFW codecs are not enumerated on 64-bit Windows\n");
        return;
    }

    hr = CoCreateInstance(&CLSID_CDeviceMoniker, NULL, CLSCTX_INPROC, &IID_IParseDisplayName, (void **)&parser);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    while (ICInfo(ICTYPE_VIDEO, i++, &info))
    {
        WCHAR name[5] = {LOBYTE(LOWORD(info.fccHandler)), HIBYTE(LOWORD(info.fccHandler)),
                         LOBYTE(HIWORD(info.fccHandler)), HIBYTE(HIWORD(info.fccHandler))};

        hic = ICOpen(ICTYPE_VIDEO, info.fccHandler, ICMODE_QUERY);
        ICGetInfo(hic, &info, sizeof(info));
        ICClose(hic);

        wcscpy(buffer, L"@device:cm:");
        StringFromGUID2(&CLSID_VideoCompressorCategory, buffer + wcslen(buffer), CHARS_IN_GUID);
        wcscat(buffer, L"\\");
        wcscat(buffer, name);

        mon = check_display_name(parser, buffer);

        hr = IMoniker_BindToStorage(mon, NULL, NULL, &IID_IPropertyBag, (void **)&prop_bag);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        VariantInit(&var);
        hr = IPropertyBag_Read(prop_bag, L"FriendlyName", &var, NULL);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        ok(!wcscmp(info.szDescription, V_BSTR(&var)), "expected %s, got %s\n",
            wine_dbgstr_w(info.szDescription), wine_dbgstr_w(V_BSTR(&var)));

        VariantClear(&var);
        hr = IPropertyBag_Read(prop_bag, L"CLSID", &var, NULL);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        StringFromGUID2(&CLSID_AVICo, buffer, CHARS_IN_GUID);
        ok(!wcscmp(buffer, V_BSTR(&var)), "expected %s, got %s\n",
            wine_dbgstr_w(buffer), wine_dbgstr_w(V_BSTR(&var)));

        VariantClear(&var);
        hr = IPropertyBag_Read(prop_bag, L"FccHandler", &var, NULL);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(!wcscmp(name, V_BSTR(&var)), "expected %s, got %s\n",
            wine_dbgstr_w(name), wine_dbgstr_w(V_BSTR(&var)));

        VariantClear(&var);
        IPropertyBag_Release(prop_bag);
        IMoniker_Release(mon);
    }

    IParseDisplayName_Release(parser);
}

START_TEST(devenum)
{
    HRESULT hr;

    CoInitialize(NULL);

    test_devenum();
    test_moniker_isequal();
    test_register_filter();
    test_directshow_filter();
    test_codec();
    test_dmo(&DMOCATEGORY_AUDIO_DECODER, &CLSID_LegacyAmFilterCategory);
    test_dmo(&DMOCATEGORY_VIDEO_DECODER, &CLSID_LegacyAmFilterCategory);
    test_dmo(&DMOCATEGORY_VIDEO_DECODER, &DMOCATEGORY_VIDEO_DECODER);

    test_legacy_filter();
    hr = DirectSoundEnumerateW(test_dsound, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    test_waveout();
    test_wavein();
    test_midiout();
    test_vfw();

    CoUninitialize();
}
