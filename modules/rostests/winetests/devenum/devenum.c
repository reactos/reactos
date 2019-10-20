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

#include <stdio.h>

#include "wine/test.h"
#include "initguid.h"
#include "ole2.h"
#include "strmif.h"
#include "uuids.h"
#include "vfwmsgs.h"
#include "mmsystem.h"
#include "dsound.h"
#include "mmddk.h"
#include "vfw.h"
#include "dmoreg.h"
#include "setupapi.h"

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);

static const WCHAR friendly_name[] = {'F','r','i','e','n','d','l','y','N','a','m','e',0};
static const WCHAR deviceW[] = {'@','d','e','v','i','c','e',':',0};
static const WCHAR clsidW[] = {'C','L','S','I','D',0};
static const WCHAR waveW[] = {'w','a','v','e',':',0};
static const WCHAR dmoW[] = {'d','m','o',':',0};
static const WCHAR swW[] = {'s','w',':',0};
static const WCHAR cmW[] = {'c','m',':',0};
static const WCHAR backslashW[] = {'\\',0};

static inline WCHAR *strchrW( const WCHAR *str, WCHAR ch )
{
    do { if (*str == ch) return (WCHAR *)str; } while (*str++);
    return NULL;
}

static inline int strncmpW( const WCHAR *str1, const WCHAR *str2, int n )
{
    if (n <= 0) return 0;
    while ((--n > 0) && *str1 && (*str1 == *str2)) { str1++; str2++; }
    return *str1 - *str2;
}

static void test_devenum(IBindCtx *bind_ctx)
{
    IEnumMoniker *enum_cat, *enum_moniker;
    ICreateDevEnum* create_devenum;
    IPropertyBag *prop_bag;
    IMoniker *moniker;
    GUID cat_guid, clsid;
    WCHAR *displayname;
    VARIANT var;
    HRESULT hr;
    int count;

    hr = CoCreateInstance(&CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC,
                           &IID_ICreateDevEnum, (LPVOID*)&create_devenum);
    ok(hr == S_OK, "Failed to create devenum: %#x\n", hr);

    hr = ICreateDevEnum_CreateClassEnumerator(create_devenum, &CLSID_ActiveMovieCategories, &enum_cat, 0);
    ok(hr == S_OK, "Failed to enum categories: %#x\n", hr);

    while (IEnumMoniker_Next(enum_cat, 1, &moniker, NULL) == S_OK)
    {
        hr = IMoniker_BindToStorage(moniker, bind_ctx, NULL, &IID_IPropertyBag, (void **)&prop_bag);
        ok(hr == S_OK, "IMoniker_BindToStorage failed: %#x\n", hr);

        VariantInit(&var);
        hr = IPropertyBag_Read(prop_bag, clsidW, &var, NULL);
        ok(hr == S_OK, "Failed to read CLSID: %#x\n", hr);

        hr = CLSIDFromString(V_BSTR(&var), &cat_guid);
        ok(hr == S_OK, "got %#x\n", hr);

        VariantClear(&var);
        hr = IPropertyBag_Read(prop_bag, friendly_name, &var, NULL);
        ok(hr == S_OK, "Failed to read FriendlyName: %#x\n", hr);

        if (winetest_debug > 1)
            trace("%s %s:\n", wine_dbgstr_guid(&cat_guid), wine_dbgstr_w(V_BSTR(&var)));

        VariantClear(&var);
        IPropertyBag_Release(prop_bag);
        IMoniker_Release(moniker);

        hr = ICreateDevEnum_CreateClassEnumerator(create_devenum, &cat_guid, &enum_moniker, 0);
        ok(SUCCEEDED(hr), "Failed to enum devices: %#x\n", hr);

        if (hr == S_OK)
        {
            count = 0;

            while (IEnumMoniker_Next(enum_moniker, 1, &moniker, NULL) == S_OK)
            {
                hr = IMoniker_GetDisplayName(moniker, NULL, NULL, &displayname);
                ok(hr == S_OK, "got %#x\n", hr);

                hr = IMoniker_GetClassID(moniker, NULL);
                ok(hr == E_INVALIDARG, "IMoniker_GetClassID should failed %x\n", hr);

                hr = IMoniker_GetClassID(moniker, &clsid);
                ok(hr == S_OK, "IMoniker_GetClassID failed with error %x\n", hr);
                ok(IsEqualGUID(&clsid, &CLSID_CDeviceMoniker),
                   "Expected CLSID_CDeviceMoniker got %s\n", wine_dbgstr_guid(&clsid));

                VariantInit(&var);
                hr = IMoniker_BindToStorage(moniker, bind_ctx, NULL, &IID_IPropertyBag, (LPVOID*)&prop_bag);
                ok(hr == S_OK, "IMoniker_BindToStorage failed with error %x\n", hr);

                hr = IPropertyBag_Read(prop_bag, friendly_name, &var, NULL);
                ok((hr == S_OK) | (hr == ERROR_KEY_DOES_NOT_EXIST),
					"IPropertyBag_Read failed: %#x\n", hr);

                if (winetest_debug > 1)
                    trace("  %s %s\n", wine_dbgstr_w(displayname), wine_dbgstr_w(V_BSTR(&var)));

                hr = IMoniker_BindToObject(moniker, bind_ctx, NULL, &IID_IUnknown, NULL);
                ok(hr == E_POINTER, "got %#x\n", hr);

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
    HRESULT res;
    ICreateDevEnum *create_devenum = NULL;
    IEnumMoniker *enum_moniker0 = NULL, *enum_moniker1 = NULL;
    IMoniker *moniker0 = NULL, *moniker1 = NULL;

    res = CoCreateInstance(&CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC,
                           &IID_ICreateDevEnum, (LPVOID*)&create_devenum);
    if (FAILED(res))
    {
         skip("Cannot create SystemDeviceEnum object (%x)\n", res);
         return;
    }

    res = ICreateDevEnum_CreateClassEnumerator(create_devenum, &CLSID_LegacyAmFilterCategory, &enum_moniker0, 0);
    ok(SUCCEEDED(res), "Cannot create enum moniker (res = %x)\n", res);
    if (SUCCEEDED(res))
    {
        if (IEnumMoniker_Next(enum_moniker0, 1, &moniker0, NULL) == S_OK &&
            IEnumMoniker_Next(enum_moniker0, 1, &moniker1, NULL) == S_OK)
        {
            res = IMoniker_IsEqual(moniker0, moniker1);
            ok(res == S_FALSE, "IMoniker_IsEqual should fail (res = %x)\n", res);

            res = IMoniker_IsEqual(moniker1, moniker0);
            ok(res == S_FALSE, "IMoniker_IsEqual should fail (res = %x)\n", res);

            IMoniker_Release(moniker0);
            IMoniker_Release(moniker1);
        }
        else
            skip("Cannot get moniker for testing.\n");
    }
    IEnumMoniker_Release(enum_moniker0);

    res = ICreateDevEnum_CreateClassEnumerator(create_devenum, &CLSID_LegacyAmFilterCategory, &enum_moniker0, 0);
    ok(SUCCEEDED(res), "Cannot create enum moniker (res = %x)\n", res);
    res = ICreateDevEnum_CreateClassEnumerator(create_devenum, &CLSID_AudioRendererCategory, &enum_moniker1, 0);
    ok(SUCCEEDED(res), "Cannot create enum moniker (res = %x)\n", res);
    if (SUCCEEDED(res))
    {
        if (IEnumMoniker_Next(enum_moniker0, 1, &moniker0, NULL) == S_OK &&
            IEnumMoniker_Next(enum_moniker1, 1, &moniker1, NULL) == S_OK)
        {
            res = IMoniker_IsEqual(moniker0, moniker1);
            ok(res == S_FALSE, "IMoniker_IsEqual should failed (res = %x)\n", res);

            res = IMoniker_IsEqual(moniker1, moniker0);
            ok(res == S_FALSE, "IMoniker_IsEqual should failed (res = %x)\n", res);

            IMoniker_Release(moniker0);
            IMoniker_Release(moniker1);
        }
        else
            skip("Cannot get moniker for testing.\n");
    }
    IEnumMoniker_Release(enum_moniker0);
    IEnumMoniker_Release(enum_moniker1);

    res = ICreateDevEnum_CreateClassEnumerator(create_devenum, &CLSID_LegacyAmFilterCategory, &enum_moniker0, 0);
    ok(SUCCEEDED(res), "Cannot create enum moniker (res = %x)\n", res);
    res = ICreateDevEnum_CreateClassEnumerator(create_devenum, &CLSID_LegacyAmFilterCategory, &enum_moniker1, 0);
    ok(SUCCEEDED(res), "Cannot create enum moniker (res = %x)\n", res);
    if (SUCCEEDED(res))
    {
        if (IEnumMoniker_Next(enum_moniker0, 1, &moniker0, NULL) == S_OK &&
            IEnumMoniker_Next(enum_moniker1, 1, &moniker1, NULL) == S_OK)
        {
            res = IMoniker_IsEqual(moniker0, moniker1);
            ok(res == S_OK, "IMoniker_IsEqual failed (res = %x)\n", res);

            res = IMoniker_IsEqual(moniker1, moniker0);
            ok(res == S_OK, "IMoniker_IsEqual failed (res = %x)\n", res);

            IMoniker_Release(moniker0);
            IMoniker_Release(moniker1);
        }
        else
            skip("Cannot get moniker for testing.\n");
    }
    IEnumMoniker_Release(enum_moniker0);
    IEnumMoniker_Release(enum_moniker1);

    ICreateDevEnum_Release(create_devenum);

    return;
}

static BOOL find_moniker(const GUID *class, IMoniker *needle)
{
    ICreateDevEnum *devenum;
    IEnumMoniker *enum_mon;
    IMoniker *mon;
    BOOL found = FALSE;

    CoCreateInstance(&CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC, &IID_ICreateDevEnum, (void **)&devenum);
    ICreateDevEnum_CreateClassEnumerator(devenum, class, &enum_mon, 0);
    while (!found && IEnumMoniker_Next(enum_mon, 1, &mon, NULL) == S_OK)
    {
        if (IMoniker_IsEqual(mon, needle) == S_OK)
            found = TRUE;

        IMoniker_Release(mon);
    }

    IEnumMoniker_Release(enum_mon);
    ICreateDevEnum_Release(devenum);
    return found;
}

DEFINE_GUID(CLSID_TestFilter,  0xdeadbeef,0xcf51,0x43e6,0xb6,0xc5,0x29,0x9e,0xa8,0xb6,0xb5,0x91);

static void test_register_filter(void)
{
    static const WCHAR name[] = {'d','e','v','e','n','u','m',' ','t','e','s','t',0};
    IFilterMapper2 *mapper2;
    IMoniker *mon = NULL;
    REGFILTER2 rgf2 = {0};
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_FilterMapper2, NULL, CLSCTX_INPROC, &IID_IFilterMapper2, (void **)&mapper2);
    ok(hr == S_OK, "Failed to create FilterMapper2: %#x\n", hr);

    rgf2.dwVersion = 2;
    rgf2.dwMerit = MERIT_UNLIKELY;
    S2(U(rgf2)).cPins2 = 0;

    hr = IFilterMapper2_RegisterFilter(mapper2, &CLSID_TestFilter, name, &mon, NULL, NULL, &rgf2);
    if (hr == E_ACCESSDENIED)
    {
        skip("Not enough permissions to register filters\n");
        IFilterMapper2_Release(mapper2);
        return;
    }
    ok(hr == S_OK, "RegisterFilter failed: %#x\n", hr);

    ok(find_moniker(&CLSID_LegacyAmFilterCategory, mon), "filter should be registered\n");

    hr = IFilterMapper2_UnregisterFilter(mapper2, NULL, NULL, &CLSID_TestFilter);
    ok(hr == S_OK, "UnregisterFilter failed: %#x\n", hr);

    ok(!find_moniker(&CLSID_LegacyAmFilterCategory, mon), "filter should not be registered\n");
    IMoniker_Release(mon);

    mon = NULL;
    hr = IFilterMapper2_RegisterFilter(mapper2, &CLSID_TestFilter, name, &mon, &CLSID_AudioRendererCategory, NULL, &rgf2);
    ok(hr == S_OK, "RegisterFilter failed: %#x\n", hr);

    ok(find_moniker(&CLSID_AudioRendererCategory, mon), "filter should be registered\n");

    hr = IFilterMapper2_UnregisterFilter(mapper2, &CLSID_AudioRendererCategory, NULL, &CLSID_TestFilter);
    ok(hr == S_OK, "UnregisterFilter failed: %#x\n", hr);

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
    ok_(__FILE__, line)(hr == S_OK, "ParseDisplayName failed: %#x\n", hr);

    hr = IMoniker_GetDisplayName(mon, NULL, NULL, &str);
    ok_(__FILE__, line)(hr == S_OK, "GetDisplayName failed: %#x\n", hr);
    ok_(__FILE__, line)(!lstrcmpW(str, buffer), "got %s\n", wine_dbgstr_w(str));

    CoTaskMemFree(str);

    return mon;
}
#define check_display_name(parser, buffer) check_display_name_(__LINE__, parser, buffer)

static void test_directshow_filter(void)
{
    static const WCHAR instanceW[] = {'\\','I','n','s','t','a','n','c','e',0};
    static const WCHAR clsidW[] = {'C','L','S','I','D','\\',0};
    static WCHAR testW[] = {'\\','t','e','s','t',0};
    IParseDisplayName *parser;
    IPropertyBag *prop_bag;
    IMoniker *mon;
    WCHAR buffer[200];
    LRESULT res;
    VARIANT var;
    HRESULT hr;

    /* Test ParseDisplayName and GetDisplayName */
    hr = CoCreateInstance(&CLSID_CDeviceMoniker, NULL, CLSCTX_INPROC, &IID_IParseDisplayName, (void **)&parser);
    ok(hr == S_OK, "Failed to create ParseDisplayName: %#x\n", hr);

    lstrcpyW(buffer, deviceW);
    lstrcatW(buffer, swW);
    StringFromGUID2(&CLSID_AudioRendererCategory, buffer + lstrlenW(buffer), CHARS_IN_GUID);
    lstrcatW(buffer, testW);
    mon = check_display_name(parser, buffer);

    /* Test writing and reading from the property bag */
    ok(!find_moniker(&CLSID_AudioRendererCategory, mon), "filter should not be registered\n");

    hr = IMoniker_BindToStorage(mon, NULL, NULL, &IID_IPropertyBag, (void **)&prop_bag);
    ok(hr == S_OK, "BindToStorage failed: %#x\n", hr);

    VariantInit(&var);
    hr = IPropertyBag_Read(prop_bag, friendly_name, &var, NULL);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), "got %#x\n", hr);

    /* writing causes the key to be created */
    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(testW);
    hr = IPropertyBag_Write(prop_bag, friendly_name, &var);
    if (hr != E_ACCESSDENIED)
    {
        ok(hr == S_OK, "Write failed: %#x\n", hr);

        ok(find_moniker(&CLSID_AudioRendererCategory, mon), "filter should be registered\n");

        VariantClear(&var);
        hr = IPropertyBag_Read(prop_bag, friendly_name, &var, NULL);
        ok(hr == S_OK, "Read failed: %#x\n", hr);
        ok(!lstrcmpW(V_BSTR(&var), testW), "got %s\n", wine_dbgstr_w(V_BSTR(&var)));

        IMoniker_Release(mon);

        /* devenum doesn't give us a way to unregister—we have to do that manually */
        lstrcpyW(buffer, clsidW);
        StringFromGUID2(&CLSID_AudioRendererCategory, buffer + lstrlenW(buffer), CHARS_IN_GUID);
        lstrcatW(buffer, instanceW);
        lstrcatW(buffer, testW);
        res = RegDeleteKeyW(HKEY_CLASSES_ROOT, buffer);
        ok(!res, "RegDeleteKey failed: %lu\n", res);
    }

    VariantClear(&var);
    IPropertyBag_Release(prop_bag);

    /* name can be anything */

    lstrcpyW(buffer, deviceW);
    lstrcatW(buffer, swW);
    lstrcatW(buffer, testW+1);
    mon = check_display_name(parser, buffer);

    hr = IMoniker_BindToStorage(mon, NULL, NULL, &IID_IPropertyBag, (void **)&prop_bag);
    ok(hr == S_OK, "BindToStorage failed: %#x\n", hr);

    VariantClear(&var);
    hr = IPropertyBag_Read(prop_bag, friendly_name, &var, NULL);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), "got %#x\n", hr);

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(testW);
    hr = IPropertyBag_Write(prop_bag, friendly_name, &var);
    if (hr != E_ACCESSDENIED)
    {
        ok(hr == S_OK, "Write failed: %#x\n", hr);

        VariantClear(&var);
        hr = IPropertyBag_Read(prop_bag, friendly_name, &var, NULL);
        ok(hr == S_OK, "Read failed: %#x\n", hr);
        ok(!lstrcmpW(V_BSTR(&var), testW), "got %s\n", wine_dbgstr_w(V_BSTR(&var)));

        IMoniker_Release(mon);

        /* vista+ stores it inside the Instance key */
        RegDeleteKeyA(HKEY_CLASSES_ROOT, "CLSID\\test\\Instance");

        res = RegDeleteKeyA(HKEY_CLASSES_ROOT, "CLSID\\test");
        ok(!res, "RegDeleteKey failed: %lu\n", res);
    }

    VariantClear(&var);
    IPropertyBag_Release(prop_bag);
    IParseDisplayName_Release(parser);
}

static void test_codec(void)
{
    static WCHAR testW[] = {'\\','t','e','s','t',0};
    IParseDisplayName *parser;
    IPropertyBag *prop_bag;
    IMoniker *mon;
    WCHAR buffer[200];
    VARIANT var;
    HRESULT hr;

    /* Test ParseDisplayName and GetDisplayName */
    hr = CoCreateInstance(&CLSID_CDeviceMoniker, NULL, CLSCTX_INPROC, &IID_IParseDisplayName, (void **)&parser);
    ok(hr == S_OK, "Failed to create ParseDisplayName: %#x\n", hr);

    lstrcpyW(buffer, deviceW);
    lstrcatW(buffer, cmW);
    StringFromGUID2(&CLSID_AudioRendererCategory, buffer + lstrlenW(buffer), CHARS_IN_GUID);
    lstrcatW(buffer, testW);
    mon = check_display_name(parser, buffer);

    /* Test writing and reading from the property bag */
    ok(!find_moniker(&CLSID_AudioRendererCategory, mon), "codec should not be registered\n");

    hr = IMoniker_BindToStorage(mon, NULL, NULL, &IID_IPropertyBag, (void **)&prop_bag);
    ok(hr == S_OK, "BindToStorage failed: %#x\n", hr);

    VariantInit(&var);
    hr = IPropertyBag_Read(prop_bag, friendly_name, &var, NULL);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), "got %#x\n", hr);

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(testW);
    hr = IPropertyBag_Write(prop_bag, friendly_name, &var);
    ok(hr == S_OK, "Write failed: %#x\n", hr);

    VariantClear(&var);
    hr = IPropertyBag_Read(prop_bag, friendly_name, &var, NULL);
    ok(hr == S_OK, "Read failed: %#x\n", hr);
    ok(!lstrcmpW(V_BSTR(&var), testW), "got %s\n", wine_dbgstr_w(V_BSTR(&var)));

    /* unlike DirectShow filters, these are automatically generated, so
     * enumerating them will destroy the key */
    ok(!find_moniker(&CLSID_AudioRendererCategory, mon), "codec should not be registered\n");

    VariantClear(&var);
    hr = IPropertyBag_Read(prop_bag, friendly_name, &var, NULL);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), "got %#x\n", hr);

    IPropertyBag_Release(prop_bag);
    IMoniker_Release(mon);

    IParseDisplayName_Release(parser);
}

static void test_dmo(void)
{
    static const WCHAR name[] = {'d','e','v','e','n','u','m',' ','t','e','s','t',0};
    IParseDisplayName *parser;
    IPropertyBag *prop_bag;
    WCHAR buffer[200];
    IMoniker *mon;
    VARIANT var;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_CDeviceMoniker, NULL, CLSCTX_INPROC, &IID_IParseDisplayName, (void **)&parser);
    ok(hr == S_OK, "Failed to create ParseDisplayName: %#x\n", hr);

    lstrcpyW(buffer, deviceW);
    lstrcatW(buffer, dmoW);
    StringFromGUID2(&CLSID_TestFilter, buffer + lstrlenW(buffer), CHARS_IN_GUID);
    StringFromGUID2(&CLSID_AudioRendererCategory, buffer + lstrlenW(buffer), CHARS_IN_GUID);
    mon = check_display_name(parser, buffer);

    ok(!find_moniker(&CLSID_AudioRendererCategory, mon), "DMO should not be registered\n");

    hr = IMoniker_BindToStorage(mon, NULL, NULL, &IID_IPropertyBag, (void **)&prop_bag);
    ok(hr == S_OK, "got %#x\n", hr);

    VariantInit(&var);
    hr = IPropertyBag_Read(prop_bag, friendly_name, &var, NULL);
    ok(hr == E_FAIL, "got %#x\n", hr);

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(name);
    hr = IPropertyBag_Write(prop_bag, friendly_name, &var);
    ok(hr == E_ACCESSDENIED, "Write failed: %#x\n", hr);

    hr = DMORegister(name, &CLSID_TestFilter, &CLSID_AudioRendererCategory, 0, 0, NULL, 0, NULL);
    if (hr != E_FAIL)
    {
        ok(hr == S_OK, "got %#x\n", hr);

        ok(find_moniker(&CLSID_AudioRendererCategory, mon), "DMO should be registered\n");

        VariantClear(&var);
        hr = IPropertyBag_Read(prop_bag, friendly_name, &var, NULL);
        ok(hr == S_OK, "got %#x\n", hr);
        ok(!lstrcmpW(V_BSTR(&var), name), "got %s\n", wine_dbgstr_w(V_BSTR(&var)));

        VariantClear(&var);
        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = SysAllocString(name);
        hr = IPropertyBag_Write(prop_bag, friendly_name, &var);
        ok(hr == E_ACCESSDENIED, "Write failed: %#x\n", hr);

        VariantClear(&var);
        hr = IPropertyBag_Read(prop_bag, clsidW, &var, NULL);
        ok(hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND), "got %#x\n", hr);

        hr = DMOUnregister(&CLSID_TestFilter, &CLSID_AudioRendererCategory);
        ok(hr == S_OK, "got %#x\n", hr);
    }
    IPropertyBag_Release(prop_bag);
    IMoniker_Release(mon);
    IParseDisplayName_Release(parser);
}

static void test_legacy_filter(void)
{
    static const WCHAR nameW[] = {'t','e','s','t',0};
    IParseDisplayName *parser;
    IPropertyBag *prop_bag;
    IFilterMapper *mapper;
    IMoniker *mon;
    WCHAR buffer[200];
    VARIANT var;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_CDeviceMoniker, NULL, CLSCTX_INPROC, &IID_IParseDisplayName, (void **)&parser);
    ok(hr == S_OK, "Failed to create ParseDisplayName: %#x\n", hr);

    hr = CoCreateInstance(&CLSID_FilterMapper2, NULL, CLSCTX_INPROC, &IID_IFilterMapper, (void **)&mapper);
    ok(hr == S_OK, "Failed to create FilterMapper: %#x\n", hr);

    hr = IFilterMapper_RegisterFilter(mapper, CLSID_TestFilter, nameW, 0xdeadbeef);
    if (hr == VFW_E_BAD_KEY)
    {
        win_skip("not enough permissions to register filters\n");
        goto end;
    }
    ok(hr == S_OK, "RegisterFilter failed: %#x\n", hr);

    lstrcpyW(buffer, deviceW);
    lstrcatW(buffer, cmW);
    StringFromGUID2(&CLSID_LegacyAmFilterCategory, buffer + lstrlenW(buffer), CHARS_IN_GUID);
    lstrcatW(buffer, backslashW);
    StringFromGUID2(&CLSID_TestFilter, buffer + lstrlenW(buffer), CHARS_IN_GUID);

    mon = check_display_name(parser, buffer);
    ok(find_moniker(&CLSID_LegacyAmFilterCategory, mon), "filter should be registered\n");

    hr = IMoniker_BindToStorage(mon, NULL, NULL, &IID_IPropertyBag, (void **)&prop_bag);
    ok(hr == S_OK, "BindToStorage failed: %#x\n", hr);

    VariantInit(&var);
    hr = IPropertyBag_Read(prop_bag, friendly_name, &var, NULL);
    ok(hr == S_OK, "Read failed: %#x\n", hr);

    StringFromGUID2(&CLSID_TestFilter, buffer, CHARS_IN_GUID);
    ok(!lstrcmpW(buffer, V_BSTR(&var)), "expected %s, got %s\n",
        wine_dbgstr_w(buffer), wine_dbgstr_w(V_BSTR(&var)));

    VariantClear(&var);
    hr = IPropertyBag_Read(prop_bag, clsidW, &var, NULL);
    ok(hr == S_OK, "Read failed: %#x\n", hr);
    ok(!lstrcmpW(buffer, V_BSTR(&var)), "expected %s, got %s\n",
        wine_dbgstr_w(buffer), wine_dbgstr_w(V_BSTR(&var)));

    VariantClear(&var);
    IPropertyBag_Release(prop_bag);

    hr = IFilterMapper_UnregisterFilter(mapper, CLSID_TestFilter);
    ok(hr == S_OK, "UnregisterFilter failed: %#x\n", hr);

    ok(!find_moniker(&CLSID_LegacyAmFilterCategory, mon), "filter should not be registered\n");
    IMoniker_Release(mon);

end:
    IFilterMapper_Release(mapper);
    IParseDisplayName_Release(parser);
}

static BOOL CALLBACK test_dsound(GUID *guid, const WCHAR *desc, const WCHAR *module, void *context)
{
    static const WCHAR defaultW[] = {'D','e','f','a','u','l','t',' ','D','i','r','e','c','t','S','o','u','n','d',' ','D','e','v','i','c','e',0};
    static const WCHAR directsoundW[] = {'D','i','r','e','c','t','S','o','u','n','d',':',' ',0};
    static const WCHAR dsguidW[] = {'D','S','G','u','i','d',0};
    IParseDisplayName *parser;
    IPropertyBag *prop_bag;
    IMoniker *mon;
    WCHAR buffer[200];
    WCHAR name[200];
    VARIANT var;
    HRESULT hr;

    if (guid)
    {
        lstrcpyW(name, directsoundW);
        lstrcatW(name, desc);
    }
    else
    {
        lstrcpyW(name, defaultW);
        guid = (GUID *)&GUID_NULL;
    }

    hr = CoCreateInstance(&CLSID_CDeviceMoniker, NULL, CLSCTX_INPROC, &IID_IParseDisplayName, (void **)&parser);
    ok(hr == S_OK, "Failed to create ParseDisplayName: %#x\n", hr);

    lstrcpyW(buffer, deviceW);
    lstrcatW(buffer, cmW);
    StringFromGUID2(&CLSID_AudioRendererCategory, buffer + lstrlenW(buffer), CHARS_IN_GUID);
    lstrcatW(buffer, backslashW);
    lstrcatW(buffer, name);

    mon = check_display_name(parser, buffer);

    hr = IMoniker_BindToStorage(mon, NULL, NULL, &IID_IPropertyBag, (void **)&prop_bag);
    ok(hr == S_OK, "BindToStorage failed: %#x\n", hr);

    VariantInit(&var);
    hr = IPropertyBag_Read(prop_bag, friendly_name, &var, NULL);
    if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
    {
        /* Win8+ uses the GUID instead of the device name */
        IPropertyBag_Release(prop_bag);
        IMoniker_Release(mon);

        lstrcpyW(buffer, deviceW);
        lstrcatW(buffer, cmW);
        StringFromGUID2(&CLSID_AudioRendererCategory, buffer + lstrlenW(buffer), CHARS_IN_GUID);
        lstrcatW(buffer, backslashW);
        lstrcatW(buffer, directsoundW);
        StringFromGUID2(guid, buffer + lstrlenW(buffer) - 1, CHARS_IN_GUID);

        mon = check_display_name(parser, buffer);

        hr = IMoniker_BindToStorage(mon, NULL, NULL, &IID_IPropertyBag, (void **)&prop_bag);
        ok(hr == S_OK, "BindToStorage failed: %#x\n", hr);

        VariantInit(&var);
        hr = IPropertyBag_Read(prop_bag, friendly_name, &var, NULL);
    }
    ok(hr == S_OK, "Read failed: %#x\n", hr);

    ok(!lstrcmpW(name, V_BSTR(&var)), "expected %s, got %s\n",
        wine_dbgstr_w(name), wine_dbgstr_w(V_BSTR(&var)));

    VariantClear(&var);
    hr = IPropertyBag_Read(prop_bag, clsidW, &var, NULL);
    ok(hr == S_OK, "Read failed: %#x\n", hr);

    StringFromGUID2(&CLSID_DSoundRender, buffer, CHARS_IN_GUID);
    ok(!lstrcmpW(buffer, V_BSTR(&var)), "expected %s, got %s\n",
        wine_dbgstr_w(buffer), wine_dbgstr_w(V_BSTR(&var)));

    VariantClear(&var);
    hr = IPropertyBag_Read(prop_bag, dsguidW, &var, NULL);
    ok(hr == S_OK, "Read failed: %#x\n", hr);

    StringFromGUID2(guid, buffer, CHARS_IN_GUID);
    ok(!lstrcmpW(buffer, V_BSTR(&var)), "expected %s, got %s\n",
        wine_dbgstr_w(buffer), wine_dbgstr_w(V_BSTR(&var)));

    VariantClear(&var);
    IPropertyBag_Release(prop_bag);
    IMoniker_Release(mon);
    IParseDisplayName_Release(parser);
    return TRUE;
}

static void test_waveout(void)
{
    static const WCHAR defaultW[] = {'D','e','f','a','u','l','t',' ','W','a','v','e','O','u','t',' ','D','e','v','i','c','e',0};
    static const WCHAR waveoutidW[] = {'W','a','v','e','O','u','t','I','d',0};
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
    ok(hr == S_OK, "Failed to create ParseDisplayName: %#x\n", hr);

    count = waveOutGetNumDevs();

    for (i = -1; i < count; i++)
    {
        waveOutGetDevCapsW(i, &caps, sizeof(caps));

        if (i == -1)    /* WAVE_MAPPER */
            name = defaultW;
        else
            name = caps.szPname;

        lstrcpyW(buffer, deviceW);
        lstrcatW(buffer, cmW);
        StringFromGUID2(&CLSID_AudioRendererCategory, buffer + lstrlenW(buffer), CHARS_IN_GUID);
        lstrcatW(buffer, backslashW);
        lstrcatW(buffer, name);

        mon = check_display_name(parser, buffer);

        hr = IMoniker_BindToStorage(mon, NULL, NULL, &IID_IPropertyBag, (void **)&prop_bag);
        ok(hr == S_OK, "BindToStorage failed: %#x\n", hr);

        VariantInit(&var);
        hr = IPropertyBag_Read(prop_bag, friendly_name, &var, NULL);
        if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
        {
            IPropertyBag_Release(prop_bag);
            IMoniker_Release(mon);

            /* Win8+ uses the endpoint GUID instead of the device name */
            mmr = waveOutMessage((HWAVEOUT)(DWORD_PTR) i, DRV_QUERYFUNCTIONINSTANCEID,
                                 (DWORD_PTR) endpoint, sizeof(endpoint));
            ok(!mmr, "waveOutMessage failed: %u\n", mmr);

            lstrcpyW(buffer, deviceW);
            lstrcatW(buffer, cmW);
            StringFromGUID2(&CLSID_AudioRendererCategory, buffer + lstrlenW(buffer), CHARS_IN_GUID);
            lstrcatW(buffer, backslashW);
            lstrcatW(buffer, waveW);
            lstrcatW(buffer, strchrW(endpoint, '}') + 2);

            mon = check_display_name(parser, buffer);

            hr = IMoniker_BindToStorage(mon, NULL, NULL, &IID_IPropertyBag, (void **)&prop_bag);
            ok(hr == S_OK, "BindToStorage failed: %#x\n", hr);

            hr = IPropertyBag_Read(prop_bag, friendly_name, &var, NULL);
        }
        ok(hr == S_OK, "Read failed: %#x\n", hr);

        ok(!strncmpW(name, V_BSTR(&var), lstrlenW(name)), "expected %s, got %s\n",
            wine_dbgstr_w(name), wine_dbgstr_w(V_BSTR(&var)));

        VariantClear(&var);
        hr = IPropertyBag_Read(prop_bag, clsidW, &var, NULL);
        ok(hr == S_OK, "Read failed: %#x\n", hr);

        StringFromGUID2(&CLSID_AudioRender, buffer, CHARS_IN_GUID);
        ok(!lstrcmpW(buffer, V_BSTR(&var)), "expected %s, got %s\n",
            wine_dbgstr_w(buffer), wine_dbgstr_w(V_BSTR(&var)));

        VariantClear(&var);
        hr = IPropertyBag_Read(prop_bag, waveoutidW, &var, NULL);
        ok(hr == S_OK, "Read failed: %#x\n", hr);

        ok(V_I4(&var) == i, "expected %d, got %d\n", i, V_I4(&var));

        IPropertyBag_Release(prop_bag);
        IMoniker_Release(mon);
    }

    IParseDisplayName_Release(parser);
}

static void test_wavein(void)
{
    static const WCHAR waveinidW[] = {'W','a','v','e','I','n','I','d',0};
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
    ok(hr == S_OK, "Failed to create ParseDisplayName: %#x\n", hr);

    count = waveInGetNumDevs();

    for (i = 0; i < count; i++)
    {
        waveInGetDevCapsW(i, &caps, sizeof(caps));

        lstrcpyW(buffer, deviceW);
        lstrcatW(buffer, cmW);
        StringFromGUID2(&CLSID_AudioInputDeviceCategory, buffer + lstrlenW(buffer), CHARS_IN_GUID);
        lstrcatW(buffer, backslashW);
        lstrcatW(buffer, caps.szPname);

        mon = check_display_name(parser, buffer);

        hr = IMoniker_BindToStorage(mon, NULL, NULL, &IID_IPropertyBag, (void **)&prop_bag);
        ok(hr == S_OK, "BindToStorage failed: %#x\n", hr);

        VariantInit(&var);
        hr = IPropertyBag_Read(prop_bag, friendly_name, &var, NULL);
        if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
        {
            IPropertyBag_Release(prop_bag);
            IMoniker_Release(mon);

            /* Win8+ uses the endpoint GUID instead of the device name */
            mmr = waveInMessage((HWAVEIN)(DWORD_PTR) i, DRV_QUERYFUNCTIONINSTANCEID,
                                (DWORD_PTR) endpoint, sizeof(endpoint));
            ok(!mmr, "waveInMessage failed: %u\n", mmr);

            lstrcpyW(buffer, deviceW);
            lstrcatW(buffer, cmW);
            StringFromGUID2(&CLSID_AudioInputDeviceCategory, buffer + lstrlenW(buffer), CHARS_IN_GUID);
            lstrcatW(buffer, backslashW);
            lstrcatW(buffer, waveW);
            lstrcatW(buffer, strchrW(endpoint, '}') + 2);

            mon = check_display_name(parser, buffer);

            hr = IMoniker_BindToStorage(mon, NULL, NULL, &IID_IPropertyBag, (void **)&prop_bag);
            ok(hr == S_OK, "BindToStorage failed: %#x\n", hr);

            hr = IPropertyBag_Read(prop_bag, friendly_name, &var, NULL);
        }
        ok(hr == S_OK, "Read failed: %#x\n", hr);

        ok(!strncmpW(caps.szPname, V_BSTR(&var), lstrlenW(caps.szPname)), "expected %s, got %s\n",
            wine_dbgstr_w(caps.szPname), wine_dbgstr_w(V_BSTR(&var)));

        VariantClear(&var);
        hr = IPropertyBag_Read(prop_bag, clsidW, &var, NULL);
        ok(hr == S_OK, "Read failed: %#x\n", hr);

        StringFromGUID2(&CLSID_AudioRecord, buffer, CHARS_IN_GUID);
        ok(!lstrcmpW(buffer, V_BSTR(&var)), "expected %s, got %s\n",
            wine_dbgstr_w(buffer), wine_dbgstr_w(V_BSTR(&var)));

        VariantClear(&var);
        hr = IPropertyBag_Read(prop_bag, waveinidW, &var, NULL);
        ok(hr == S_OK, "Read failed: %#x\n", hr);

        ok(V_I4(&var) == i, "expected %d, got %d\n", i, V_I4(&var));

        IPropertyBag_Release(prop_bag);
        IMoniker_Release(mon);
    }

    IParseDisplayName_Release(parser);
}

static void test_midiout(void)
{
    static const WCHAR defaultW[] = {'D','e','f','a','u','l','t',' ','M','i','d','i','O','u','t',' ','D','e','v','i','c','e',0};
    static const WCHAR midioutidW[] = {'M','i','d','i','O','u','t','I','d',0};
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
    ok(hr == S_OK, "Failed to create ParseDisplayName: %#x\n", hr);

    count = midiOutGetNumDevs();

    for (i = -1; i < count; i++)
    {
        midiOutGetDevCapsW(i, &caps, sizeof(caps));

        if (i == -1)    /* MIDI_MAPPER */
            name = defaultW;
        else
            name = caps.szPname;

        lstrcpyW(buffer, deviceW);
        lstrcatW(buffer, cmW);
        StringFromGUID2(&CLSID_MidiRendererCategory, buffer + lstrlenW(buffer), CHARS_IN_GUID);
        lstrcatW(buffer, backslashW);
        lstrcatW(buffer, name);

        mon = check_display_name(parser, buffer);

        hr = IMoniker_BindToStorage(mon, NULL, NULL, &IID_IPropertyBag, (void **)&prop_bag);
        ok(hr == S_OK, "BindToStorage failed: %#x\n", hr);

        VariantInit(&var);
        hr = IPropertyBag_Read(prop_bag, friendly_name, &var, NULL);
        ok(hr == S_OK, "Read failed: %#x\n", hr);

        ok(!lstrcmpW(name, V_BSTR(&var)), "expected %s, got %s\n",
            wine_dbgstr_w(name), wine_dbgstr_w(V_BSTR(&var)));

        VariantClear(&var);
        hr = IPropertyBag_Read(prop_bag, clsidW, &var, NULL);
        ok(hr == S_OK, "Read failed: %#x\n", hr);

        StringFromGUID2(&CLSID_AVIMIDIRender, buffer, CHARS_IN_GUID);
        ok(!lstrcmpW(buffer, V_BSTR(&var)), "expected %s, got %s\n",
            wine_dbgstr_w(buffer), wine_dbgstr_w(V_BSTR(&var)));

        VariantClear(&var);
        hr = IPropertyBag_Read(prop_bag, midioutidW, &var, NULL);
        ok(hr == S_OK, "Read failed: %#x\n", hr);

        ok(V_I4(&var) == i, "expected %d, got %d\n", i, V_I4(&var));

        IPropertyBag_Release(prop_bag);
        IMoniker_Release(mon);
    }

    IParseDisplayName_Release(parser);
}

static void test_vfw(void)
{
    static const WCHAR fcchandlerW[] = {'F','c','c','H','a','n','d','l','e','r',0};
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
    ok(hr == S_OK, "Failed to create ParseDisplayName: %#x\n", hr);

    while (ICInfo(ICTYPE_VIDEO, i++, &info))
    {
        WCHAR name[5] = {LOBYTE(LOWORD(info.fccHandler)), HIBYTE(LOWORD(info.fccHandler)),
                         LOBYTE(HIWORD(info.fccHandler)), HIBYTE(HIWORD(info.fccHandler))};

        hic = ICOpen(ICTYPE_VIDEO, info.fccHandler, ICMODE_QUERY);
        ICGetInfo(hic, &info, sizeof(info));
        ICClose(hic);

        lstrcpyW(buffer, deviceW);
        lstrcatW(buffer, cmW);
        StringFromGUID2(&CLSID_VideoCompressorCategory, buffer + lstrlenW(buffer), CHARS_IN_GUID);
        lstrcatW(buffer, backslashW);
        lstrcatW(buffer, name);

        mon = check_display_name(parser, buffer);

        hr = IMoniker_BindToStorage(mon, NULL, NULL, &IID_IPropertyBag, (void **)&prop_bag);
        ok(hr == S_OK, "BindToStorage failed: %#x\n", hr);

        VariantInit(&var);
        hr = IPropertyBag_Read(prop_bag, friendly_name, &var, NULL);
        ok(hr == S_OK, "Read failed: %#x\n", hr);

        ok(!lstrcmpW(info.szDescription, V_BSTR(&var)), "expected %s, got %s\n",
            wine_dbgstr_w(info.szDescription), wine_dbgstr_w(V_BSTR(&var)));

        VariantClear(&var);
        hr = IPropertyBag_Read(prop_bag, clsidW, &var, NULL);
        ok(hr == S_OK, "Read failed: %#x\n", hr);

        StringFromGUID2(&CLSID_AVICo, buffer, CHARS_IN_GUID);
        ok(!lstrcmpW(buffer, V_BSTR(&var)), "expected %s, got %s\n",
            wine_dbgstr_w(buffer), wine_dbgstr_w(V_BSTR(&var)));

        VariantClear(&var);
        hr = IPropertyBag_Read(prop_bag, fcchandlerW, &var, NULL);
        ok(hr == S_OK, "Read failed: %#x\n", hr);
        ok(!lstrcmpW(name, V_BSTR(&var)), "expected %s, got %s\n",
            wine_dbgstr_w(name), wine_dbgstr_w(V_BSTR(&var)));

        VariantClear(&var);
        IPropertyBag_Release(prop_bag);
        IMoniker_Release(mon);
    }

    IParseDisplayName_Release(parser);
}

START_TEST(devenum)
{
    IBindCtx *bind_ctx = NULL;
    HRESULT hr;

    CoInitialize(NULL);

    test_devenum(NULL);

    /* IBindCtx is allowed in IMoniker_BindToStorage (IMediaCatMoniker_BindToStorage) */
    hr = CreateBindCtx(0, &bind_ctx);
    ok(hr == S_OK, "Cannot create BindCtx: (res = 0x%x)\n", hr);
    if (bind_ctx) {
        test_devenum(bind_ctx);
        IBindCtx_Release(bind_ctx);
    }

    test_moniker_isequal();
    test_register_filter();
    test_directshow_filter();
    test_codec();
    test_dmo();

    test_legacy_filter();
    hr = DirectSoundEnumerateW(test_dsound, NULL);
    ok(hr == S_OK, "got %#x\n", hr);
    test_waveout();
    test_wavein();
    test_midiout();
    test_vfw();

    CoUninitialize();
}
