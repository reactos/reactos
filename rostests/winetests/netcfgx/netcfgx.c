/*
 * Copyright 2014 Alistair Leslie-Hughes
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

#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#define COBJMACROS
#include <wine/test.h>
#include <objbase.h>
#include <netcfgx.h>

static void create_configuration(void)
{
    static const WCHAR tcpipW[] = {'M','S','_','T','C','P','I','P',0};
    static const WCHAR myclient[] = {'M','Y',' ','C','L','I','E','N','T',0};
    HRESULT hr;
    INetCfg *config = NULL;
    INetCfgLock *netlock = NULL;
    INetCfgComponent *component = NULL;
    LPWSTR client = NULL;

    hr = CoCreateInstance( &CLSID_CNetCfg, NULL, CLSCTX_ALL, &IID_INetCfg, (LPVOID*)&config);
    ok(hr == S_OK, "Failed to create object\n");
    if(SUCCEEDED(hr))
    {
        hr = INetCfg_QueryInterface(config, &IID_INetCfgLock, (LPVOID*)&netlock);
        ok(hr == S_OK, "got 0x%08x\n", hr);

        hr = INetCfgLock_AcquireWriteLock(netlock, 5000, myclient, &client);
        ok(hr == S_OK ||
           hr == E_ACCESSDENIED /* Not run as admin */, "got 0x%08x\n", hr);
        if(hr == S_OK)
        {
            trace("Lock value: %s\n", wine_dbgstr_w(client));
            CoTaskMemFree(client);
        }
        else if(hr == E_ACCESSDENIED)
            trace("Not run with Admin permissions\n");

        hr = INetCfg_Initialize(config, NULL);
        ok(hr == S_OK, "got 0x%08x\n", hr);

        /* AcquireWriteLock needs to be run before Initialize */
        hr = INetCfgLock_AcquireWriteLock(netlock, 5000, myclient, &client);
        todo_wine ok(hr == NETCFG_E_ALREADY_INITIALIZED || hr == E_ACCESSDENIED, "got 0x%08x\n", hr);

        hr =  INetCfg_FindComponent(config, tcpipW, &component);
        todo_wine ok(hr == S_OK, "got 0x%08x\n", hr);
        if(hr == S_OK)
        {
            INetCfgComponent_Release(component);
        }

        hr = INetCfg_Apply(config);
        todo_wine ok(hr == S_OK || hr == NETCFG_E_NO_WRITE_LOCK, "got 0x%08x\n", hr);

        hr = INetCfg_Uninitialize(config);
        ok(hr == S_OK, "got 0x%08x\n", hr);

        hr = INetCfgLock_ReleaseWriteLock(netlock);
        ok(hr == S_OK, "got 0x%08x\n", hr);

        INetCfgLock_Release(netlock);
        INetCfg_Release(config);
    }
}

START_TEST(netcfgx)
{
    HRESULT hr;

    hr = CoInitialize(0);
    ok( hr == S_OK, "failed to init com\n");
    if (hr != S_OK)
        return;

    create_configuration();

    CoUninitialize();
}
