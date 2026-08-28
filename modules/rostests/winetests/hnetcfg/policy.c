/*
 * Copyright 2016 Alistair Leslie-Hughes
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
#include "ole2.h"
#include "oleauto.h"
#include "olectl.h"
#include "dispex.h"

#include "wine/test.h"

#include "netfw.h"
#include "natupnp.h"

static ULONG get_refcount(IUnknown *unk)
{
    IUnknown_AddRef(unk);
    return IUnknown_Release(unk);
}

static void test_policy2_rules(INetFwPolicy2 *policy2)
{
    HRESULT hr;
    INetFwRules *rules, *rules2;
    INetFwServiceRestriction *restriction;

    hr = INetFwPolicy2_QueryInterface(policy2, &IID_INetFwRules, (void**)&rules);
    ok(hr == E_NOINTERFACE, "got 0x%08lx\n", hr);

    hr = INetFwPolicy2_get_Rules(policy2, &rules);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = INetFwPolicy2_get_Rules(policy2, &rules2);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(rules == rules2, "Different pointers\n");

    hr = INetFwPolicy2_get_ServiceRestriction(policy2, &restriction);
    todo_wine ok(hr == S_OK, "got %08lx\n", hr);
    if(hr == S_OK)
    {
        INetFwRules *rules3;

        hr = INetFwServiceRestriction_get_Rules(restriction, &rules3);
        ok(hr == S_OK, "got %08lx\n", hr);
        ok(rules != rules3, "same pointers\n");

        if(rules3)
            INetFwRules_Release(rules3);
        INetFwServiceRestriction_Release(restriction);
    }

    hr = INetFwRules_get__NewEnum(rules, NULL);
    ok(hr == E_POINTER, "got %08lx\n", hr);

    INetFwRules_Release(rules);
    INetFwRules_Release(rules2);
}

static void test_interfaces(void)
{
    INetFwMgr *manager;
    INetFwPolicy *policy;
    INetFwPolicy2 *policy2;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_NetFwMgr, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_INetFwMgr, (void**)&manager);
    ok(hr == S_OK, "NetFwMgr create failed: %08lx\n", hr);

    hr = INetFwMgr_QueryInterface(manager, &IID_INetFwPolicy, (void**)&policy);
    ok(hr == E_NOINTERFACE, "got 0x%08lx\n", hr);

    hr = INetFwMgr_QueryInterface(manager, &IID_INetFwPolicy2, (void**)&policy2);
    ok(hr == E_NOINTERFACE, "got 0x%08lx\n", hr);

    hr = INetFwMgr_get_LocalPolicy(manager, &policy);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = INetFwPolicy_QueryInterface(policy, &IID_INetFwPolicy2, (void**)&policy2);
    ok(hr == E_NOINTERFACE, "got 0x%08lx\n", hr);

    INetFwPolicy_Release(policy);

    hr = CoCreateInstance(&CLSID_NetFwPolicy2, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_INetFwPolicy2, (void**)&policy2);
    if(hr == S_OK)
    {
        test_policy2_rules(policy2);

        INetFwPolicy2_Release(policy2);
    }
    else
        win_skip("NetFwPolicy2 object is not supported: %08lx\n", hr);

    INetFwMgr_Release(manager);
}

static void test_NetFwAuthorizedApplication(void)
{
    INetFwAuthorizedApplication *app;
    static WCHAR empty[] = L"";
    UNIVERSAL_NAME_INFOW *info;
    WCHAR fullpath[MAX_PATH];
    WCHAR netpath[MAX_PATH];
    WCHAR image[MAX_PATH];
    HRESULT hr;
    BSTR bstr;
    DWORD sz;

    hr = CoCreateInstance(&CLSID_NetFwAuthorizedApplication, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_INetFwAuthorizedApplication, (void**)&app);
    ok(hr == S_OK, "got: %08lx\n", hr);

    hr = GetModuleFileNameW(NULL, image, ARRAY_SIZE(image));
    ok(hr, "GetModuleFileName failed: %lu\n", GetLastError());

    hr = INetFwAuthorizedApplication_get_ProcessImageFileName(app, NULL);
    ok(hr == E_POINTER, "got: %08lx\n", hr);

    hr = INetFwAuthorizedApplication_get_ProcessImageFileName(app, &bstr);
    ok(hr == S_OK || hr == HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY), "got: %08lx\n", hr);
    ok(!bstr, "got: %s\n",  wine_dbgstr_w(bstr));

    hr = INetFwAuthorizedApplication_put_ProcessImageFileName(app, NULL);
    ok(hr == E_INVALIDARG || hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND), "got: %08lx\n", hr);

    hr = INetFwAuthorizedApplication_put_ProcessImageFileName(app, empty);
    ok(hr == E_INVALIDARG || hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND), "got: %08lx\n", hr);

    bstr = SysAllocString(image);
    hr = INetFwAuthorizedApplication_put_ProcessImageFileName(app, bstr);
    ok(hr == S_OK, "got: %08lx\n", hr);
    SysFreeString(bstr);

    GetFullPathNameW(image, ARRAY_SIZE(fullpath), fullpath, NULL);
    GetLongPathNameW(fullpath, fullpath, ARRAY_SIZE(fullpath));

    info = (UNIVERSAL_NAME_INFOW *)&netpath;
    sz = sizeof(netpath);
    hr = WNetGetUniversalNameW(image, UNIVERSAL_NAME_INFO_LEVEL, info, &sz);
    if (hr != NO_ERROR)
    {
        info->lpUniversalName = netpath + sizeof(*info)/sizeof(WCHAR);
        lstrcpyW(info->lpUniversalName, fullpath);
    }

    hr = INetFwAuthorizedApplication_get_ProcessImageFileName(app, &bstr);
    ok(hr == S_OK, "got: %08lx\n", hr);
    ok(!lstrcmpW(bstr,info->lpUniversalName), "expected %s, got %s\n",
        wine_dbgstr_w(info->lpUniversalName), wine_dbgstr_w(bstr));
    SysFreeString(bstr);

    INetFwAuthorizedApplication_Release(app);
}

static void test_static_port_mapping_collection( IStaticPortMappingCollection *ports )
{
    LONG i, count, count2, expected_count, external_port;
    IStaticPortMapping *pm, *pm2;
    ULONG refcount, refcount2;
    IEnumVARIANT *enum_ports;
    IUnknown *unk;
    ULONG fetched;
    BSTR protocol;
    VARIANT var;
    HRESULT hr;

    refcount = get_refcount((IUnknown *)ports);
    hr = IStaticPortMappingCollection_get__NewEnum(ports, &unk);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IUnknown_QueryInterface(unk, &IID_IEnumVARIANT, (void **)&enum_ports);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    IUnknown_Release( unk );

    refcount2 = get_refcount((IUnknown *)ports);
    ok(refcount2 == refcount, "Got unexpected refcount %lu, refcount2 %lu.\n", refcount, refcount2);

    hr = IEnumVARIANT_Reset(enum_ports);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    count = 0xdeadbeef;
    hr = IStaticPortMappingCollection_get_Count(ports, &count);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IStaticPortMappingCollection_get_Item(ports, 12345, (BSTR)L"UDP", &pm);
    if (SUCCEEDED(hr))
    {
        expected_count = count;
        IStaticPortMapping_Release(pm);
    }
    else
    {
        expected_count = count + 1;
    }

    hr = IStaticPortMappingCollection_Add(ports, 12345, (BSTR)L"udp", 12345, (BSTR)L"1.2.3.4",
            VARIANT_TRUE, (BSTR)L"wine_test", &pm);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    hr = IStaticPortMappingCollection_Add(ports, 12345, (BSTR)L"UDP", 12345, (BSTR)L"1.2.3.4",
            VARIANT_TRUE, (BSTR)L"wine_test", &pm);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IStaticPortMappingCollection_get_Count(ports, &count2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(count2 == expected_count, "Got unexpected count2 %lu, expected %lu.\n", count2, expected_count);

    hr = IStaticPortMappingCollection_get_Item(ports, 12345, NULL, &pm);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    hr = IStaticPortMappingCollection_get_Item(ports, 12345, (BSTR)L"UDP", NULL);
    ok(hr == E_POINTER, "Got unexpected hr %#lx.\n", hr);

    hr = IStaticPortMappingCollection_get_Item(ports, 12345, (BSTR)L"udp", &pm);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    hr = IStaticPortMappingCollection_get_Item(ports, -1, (BSTR)L"UDP", &pm);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    hr = IStaticPortMappingCollection_get_Item(ports, 65536, (BSTR)L"UDP", &pm);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    hr = IStaticPortMappingCollection_get_Item(ports, 12346, (BSTR)L"UDP", &pm);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), "Got unexpected hr %#lx.\n", hr);

    hr = IEnumVARIANT_Reset(enum_ports);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    for (i = 0; i < count2; ++i)
    {
        VariantInit(&var);

        fetched = 0xdeadbeef;
        hr = IEnumVARIANT_Next(enum_ports, 1, &var, &fetched);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(fetched == 1, "Got unexpected fetched %lu.\n", fetched);
        ok(V_VT(&var) == VT_DISPATCH, "Got unexpected variant type %u.\n", V_VT(&var));

        hr = IDispatch_QueryInterface(V_DISPATCH(&var), &IID_IStaticPortMapping, (void **)&pm);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        hr = IStaticPortMapping_get_Protocol(pm, &protocol);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        external_port = 0xdeadbeef;
        hr = IStaticPortMapping_get_ExternalPort(pm, &external_port);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        ok(!wcscmp(protocol, L"UDP") || !wcscmp(protocol, L"TCP"), "Got unexpected protocol %s.\n",
                debugstr_w(protocol));
        hr = IStaticPortMappingCollection_get_Item(ports, external_port, protocol, &pm2);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(pm2 != pm, "Got same interface.\n");

        IStaticPortMapping_Release(pm);
        IStaticPortMapping_Release(pm2);

        SysFreeString(protocol);

        VariantClear(&var);
    }
    hr = IEnumVARIANT_Next(enum_ports, 1, &var, &fetched);
    ok(hr == S_FALSE, "Got unexpected hr %#lx.\n", hr);

    hr = IStaticPortMappingCollection_Remove(ports, 12345, (BSTR)L"UDP");
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    IEnumVARIANT_Release(enum_ports);
}

static void test_IUPnPNAT(void)
{
    IUPnPNAT *nat;
    IStaticPortMappingCollection *static_ports;
    IDynamicPortMappingCollection *dync_ports;
    INATEventManager *manager;
    IProvideClassInfo *provider;
    ULONG refcount, refcount2;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_UPnPNAT, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER, &IID_IUPnPNAT, (void**)&nat);
    ok(hr == S_OK, "got: %08lx\n", hr);

    hr = IUPnPNAT_QueryInterface(nat, &IID_IProvideClassInfo, (void**)&provider);
    ok(hr == E_NOINTERFACE, "got: %08lx\n", hr);

    refcount = get_refcount((IUnknown *)nat);
    hr = IUPnPNAT_get_StaticPortMappingCollection(nat, &static_ports);

    ok(hr == S_OK, "got: %08lx\n", hr);
    if(hr == S_OK && static_ports)
    {
        refcount2 = get_refcount((IUnknown *)nat);
        ok(refcount2 == refcount, "Got unexpected refcount %lu, refcount2 %lu.\n", refcount, refcount2);
        test_static_port_mapping_collection( static_ports );
        IStaticPortMappingCollection_Release(static_ports);
    }
    else if (hr == S_OK)
    {
        skip( "UPNP gateway not found.\n" );
    }
    hr = IUPnPNAT_get_DynamicPortMappingCollection(nat, &dync_ports);
    ok(hr == S_OK || hr == E_NOTIMPL /* Windows 8.1 */, "got: %08lx\n", hr);
    if(hr == S_OK && dync_ports)
        IDynamicPortMappingCollection_Release(dync_ports);

    hr = IUPnPNAT_get_NATEventManager(nat, &manager);
    todo_wine ok(hr == S_OK, "got: %08lx\n", hr);
    if(hr == S_OK && manager)
        INATEventManager_Release(manager);

    IUPnPNAT_Release(nat);
}

START_TEST(policy)
{
    INetFwMgr *manager;
    HRESULT hr;

    CoInitialize(NULL);

    hr = CoCreateInstance(&CLSID_NetFwMgr, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_INetFwMgr, (void**)&manager);
    if(FAILED(hr))
    {
        win_skip("NetFwMgr object is not supported: %08lx\n", hr);
        CoUninitialize();
        return;
    }

    INetFwMgr_Release(manager);

    test_interfaces();
    test_NetFwAuthorizedApplication();
    test_IUPnPNAT();

    CoUninitialize();
}
