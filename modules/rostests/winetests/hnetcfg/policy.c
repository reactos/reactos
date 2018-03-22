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

static void test_policy2_rules(INetFwPolicy2 *policy2)
{
    HRESULT hr;
    INetFwRules *rules, *rules2;
    INetFwServiceRestriction *restriction;

    hr = INetFwPolicy2_QueryInterface(policy2, &IID_INetFwRules, (void**)&rules);
    ok(hr == E_NOINTERFACE, "got 0x%08x\n", hr);

    hr = INetFwPolicy2_get_Rules(policy2, &rules);
    ok(hr == S_OK, "got %08x\n", hr);

    hr = INetFwPolicy2_get_Rules(policy2, &rules2);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(rules == rules2, "Different pointers\n");

    hr = INetFwPolicy2_get_ServiceRestriction(policy2, &restriction);
    todo_wine ok(hr == S_OK, "got %08x\n", hr);
    if(hr == S_OK)
    {
        INetFwRules *rules3;

        hr = INetFwServiceRestriction_get_Rules(restriction, &rules3);
        ok(hr == S_OK, "got %08x\n", hr);
        ok(rules != rules3, "same pointers\n");

        if(rules3)
            INetFwRules_Release(rules3);
        INetFwServiceRestriction_Release(restriction);
    }

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
    ok(hr == S_OK, "NetFwMgr create failed: %08x\n", hr);

    hr = INetFwMgr_QueryInterface(manager, &IID_INetFwPolicy, (void**)&policy);
    ok(hr == E_NOINTERFACE, "got 0x%08x\n", hr);

    hr = INetFwMgr_QueryInterface(manager, &IID_INetFwPolicy2, (void**)&policy2);
    ok(hr == E_NOINTERFACE, "got 0x%08x\n", hr);

    hr = INetFwMgr_get_LocalPolicy(manager, &policy);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = INetFwPolicy_QueryInterface(policy, &IID_INetFwPolicy2, (void**)&policy2);
    ok(hr == E_NOINTERFACE, "got 0x%08x\n", hr);

    INetFwPolicy_Release(policy);

    hr = CoCreateInstance(&CLSID_NetFwPolicy2, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_INetFwPolicy2, (void**)&policy2);
    if(hr == S_OK)
    {
        test_policy2_rules(policy2);

        INetFwPolicy2_Release(policy2);
    }
    else
        win_skip("NetFwPolicy2 object is not supported: %08x\n", hr);

    INetFwMgr_Release(manager);
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
        win_skip("NetFwMgr object is not supported: %08x\n", hr);
        CoUninitialize();
        return;
    }

    INetFwMgr_Release(manager);

    test_interfaces();


    CoUninitialize();
}
