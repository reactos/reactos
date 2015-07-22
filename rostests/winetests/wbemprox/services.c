/*
 * Copyright 2012 Hans Leidekker for CodeWeavers
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
#include "windows.h"
#include "objidl.h"
#include "wbemcli.h"
#include "wine/test.h"

static void test_IClientSecurity(void)
{
    static const WCHAR rootW[] = {'R','O','O','T','\\','C','I','M','V','2',0};
    HRESULT hr;
    IWbemLocator *locator;
    IWbemServices *services;
    IClientSecurity *security;
    BSTR path = SysAllocString( rootW );
    ULONG refs;

    hr = CoCreateInstance( &CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, &IID_IWbemLocator, (void **)&locator );
    if (hr != S_OK)
    {
        win_skip("can't create instance of WbemLocator\n");
        return;
    }
    ok( hr == S_OK, "failed to create IWbemLocator interface %08x\n", hr );

    refs = IWbemLocator_Release( locator );
    ok( refs == 0, "unexpected refcount %u\n", refs );

    hr = CoCreateInstance( &CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, &IID_IWbemLocator, (void **)&locator );
    ok( hr == S_OK, "failed to create IWbemLocator interface %08x\n", hr );

    hr = IWbemLocator_ConnectServer( locator, path, NULL, NULL, NULL, 0, NULL, NULL, &services );
    ok( hr == S_OK, "failed to get IWbemServices interface %08x\n", hr );

    refs = IWbemServices_Release( services );
    ok( refs == 0, "unexpected refcount %u\n", refs );

    hr = IWbemLocator_ConnectServer( locator, path, NULL, NULL, NULL, 0, NULL, NULL, &services );
    ok( hr == S_OK, "failed to get IWbemServices interface %08x\n", hr );

    hr = IWbemServices_QueryInterface( services, &IID_IClientSecurity, (void **)&security );
    ok( hr == S_OK, "failed to query IClientSecurity interface %08x\n", hr );
    ok( (void *)services != (void *)security, "expected pointers to be different\n" );

    refs = IClientSecurity_Release( security );
    ok( refs == 1, "unexpected refcount %u\n", refs );

    refs = IWbemServices_Release( services );
    ok( refs == 0, "unexpected refcount %u\n", refs );

    hr = IWbemLocator_ConnectServer( locator, path, NULL, NULL, NULL, 0, NULL, NULL, &services );
    ok( hr == S_OK, "failed to get IWbemServices interface %08x\n", hr );

    hr = IWbemServices_QueryInterface( services, &IID_IClientSecurity, (void **)&security );
    ok( hr == S_OK, "failed to query IClientSecurity interface %08x\n", hr );
    ok( (void *)services != (void *)security, "expected pointers to be different\n" );

    refs = IWbemServices_Release( services );
    todo_wine ok( refs == 1, "unexpected refcount %u\n", refs );

    refs = IClientSecurity_Release( security );
    todo_wine ok( refs == 0, "unexpected refcount %u\n", refs );

    IWbemLocator_Release( locator );
    SysFreeString( path );
}

static void test_IWbemLocator(void)
{
    static const WCHAR path0W[] = {0};
    static const WCHAR path1W[] = {'\\',0};
    static const WCHAR path2W[] = {'\\','\\',0};
    static const WCHAR path3W[] = {'\\','\\','.',0};
    static const WCHAR path4W[] = {'\\','\\','.','\\',0};
    static const WCHAR path5W[] = {'\\','R','O','O','T',0};
    static const WCHAR path6W[] = {'\\','\\','R','O','O','T',0};
    static const WCHAR path7W[] = {'\\','\\','.','R','O','O','T',0};
    static const WCHAR path8W[] = {'\\','\\','.','\\','N','O','N','E',0};
    static const WCHAR path9W[] = {'\\','\\','.','\\','R','O','O','T',0};
    static const WCHAR path10W[] = {'\\','\\','\\','.','\\','R','O','O','T',0};
    static const WCHAR path11W[] = {'\\','/','.','\\','R','O','O','T',0};
    static const WCHAR path12W[] = {'/','/','.','\\','R','O','O','T',0};
    static const WCHAR path13W[] = {'\\','\\','.','/','R','O','O','T',0};
    static const WCHAR path14W[] = {'/','/','.','/','R','O','O','T',0};
    static const WCHAR path15W[] = {'N','O','N','E',0};
    static const WCHAR path16W[] = {'R','O','O','T',0};
    static const WCHAR path17W[] = {'R','O','O','T','\\','N','O','N','E',0};
    static const WCHAR path18W[] = {'R','O','O','T','\\','C','I','M','V','2',0};
    static const WCHAR path19W[] = {'R','O','O','T','\\','\\','C','I','M','V','2',0};
    static const WCHAR path20W[] = {'R','O','O','T','\\','C','I','M','V','2','\\',0};
    static const WCHAR path21W[] = {'R','O','O','T','/','C','I','M','V','2',0};
    static const WCHAR path22W[] = {'r','o','o','t','\\','d','e','f','a','u','l','t',0};
    static const WCHAR path23W[] = {'r','o','o','t','\\','c','i','m','v','0',0};
    static const WCHAR path24W[] = {'r','o','o','t','\\','c','i','m','v','1',0};
    static const WCHAR path25W[] = {'\\','\\','l','o','c','a','l','h','o','s','t','\\','R','O','O','T',0};
    static const WCHAR path26W[] = {'\\','\\','L','O','C','A','L','H','O','S','T','\\','R','O','O','T',0};
    static const struct
    {
        const WCHAR *path;
        HRESULT      result;
        BOOL         todo;
        HRESULT      result_broken;
    }
    test[] =
    {
        { path0W, WBEM_E_INVALID_NAMESPACE },
        { path1W, WBEM_E_INVALID_NAMESPACE },
        { path2W, WBEM_E_INVALID_NAMESPACE },
        { path3W, WBEM_E_INVALID_NAMESPACE },
        { path4W, WBEM_E_INVALID_NAMESPACE, FALSE, WBEM_E_INVALID_PARAMETER },
        { path5W, WBEM_E_INVALID_NAMESPACE },
        { path6W, 0x800706ba, TRUE },
        { path7W, 0x800706ba, TRUE },
        { path8W, WBEM_E_INVALID_NAMESPACE },
        { path9W, S_OK },
        { path10W, WBEM_E_INVALID_PARAMETER },
        { path11W, S_OK, FALSE, WBEM_E_INVALID_PARAMETER },
        { path12W, S_OK },
        { path13W, S_OK },
        { path14W, S_OK },
        { path15W, WBEM_E_INVALID_NAMESPACE },
        { path16W, S_OK },
        { path17W, WBEM_E_INVALID_NAMESPACE },
        { path18W, S_OK },
        { path19W, WBEM_E_INVALID_NAMESPACE, FALSE, WBEM_E_INVALID_PARAMETER },
        { path20W, WBEM_E_INVALID_NAMESPACE, FALSE, WBEM_E_INVALID_PARAMETER },
        { path21W, S_OK },
        { path22W, S_OK },
        { path23W, WBEM_E_INVALID_NAMESPACE },
        { path24W, WBEM_E_INVALID_NAMESPACE },
        { path25W, S_OK },
        { path26W, S_OK }
    };
    IWbemLocator *locator;
    IWbemServices *services;
    unsigned int i;
    HRESULT hr;
    BSTR resource;

    hr = CoCreateInstance( &CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, &IID_IWbemLocator, (void **)&locator );
    if (hr != S_OK)
    {
        win_skip("can't create instance of WbemLocator\n");
        return;
    }
    ok( hr == S_OK, "failed to create IWbemLocator interface %08x\n", hr );

    for (i = 0; i < sizeof(test) / sizeof(test[0]); i++)
    {
        resource = SysAllocString( test[i].path );
        hr = IWbemLocator_ConnectServer( locator, resource, NULL, NULL, NULL, 0, NULL, NULL, &services );
        if (test[i].todo) todo_wine
            ok( hr == test[i].result || broken(hr == test[i].result_broken),
                "%u: expected %08x got %08x\n", i, test[i].result, hr );
        else
            ok( hr == test[i].result || broken(hr == test[i].result_broken),
                "%u: expected %08x got %08x\n", i, test[i].result, hr );
        SysFreeString( resource );
        if (hr == S_OK) IWbemServices_Release( services );
    }
    IWbemLocator_Release( locator );
}

START_TEST(services)
{
    CoInitialize( NULL );
    test_IClientSecurity();
    test_IWbemLocator();
    CoUninitialize();
}
