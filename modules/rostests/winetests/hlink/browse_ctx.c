/*
 * Copyright 2009 Andrew Eikum for CodeWeavers
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

#include <hlink.h>

#include "wine/test.h"

static void test_SetInitialHlink(void)
{
    IHlinkBrowseContext *bc;
    IHlink *found_hlink;
    IMoniker *dummy, *found_moniker;
    IBindCtx *bindctx;
    WCHAR one[] = {'1',0};
    WCHAR five[] = {'5',0};
    WCHAR *found_name, *exp_name;
    HRESULT hres;

    hres = CreateBindCtx(0, &bindctx);
    ok(hres == S_OK, "CreateBindCtx failed: 0x%08lx\n", hres);

    hres = CreateItemMoniker(one, five, &dummy);
    ok(hres == S_OK, "CreateItemMoniker failed: 0x%08lx\n", hres);

    hres = IMoniker_GetDisplayName(dummy, bindctx, NULL, &exp_name);
    ok(hres == S_OK, "GetDisplayName failed: 0x%08lx\n", hres);

    hres = HlinkCreateBrowseContext(NULL, &IID_IHlinkBrowseContext, (void**)&bc);
    ok(hres == S_OK, "HlinkCreateBrowseContext failed: 0x%08lx\n", hres);

    hres = IHlinkBrowseContext_SetInitialHlink(bc, dummy, one, NULL);
    ok(hres == S_OK, "SetInitialHlink failed: 0x%08lx\n", hres);

    hres = IHlinkBrowseContext_SetInitialHlink(bc, dummy, one, NULL);
    ok(hres == CO_E_ALREADYINITIALIZED, "got 0x%08lx\n", hres);

    hres = IHlinkBrowseContext_SetInitialHlink(bc, dummy, five, NULL);
    ok(hres == CO_E_ALREADYINITIALIZED, "got 0x%08lx\n", hres);

    /* there's only one */
    hres = IHlinkBrowseContext_GetHlink(bc, HLID_PREVIOUS, &found_hlink);
    ok(hres == E_FAIL, "got 0x%08lx\n", hres);

    hres = IHlinkBrowseContext_GetHlink(bc, HLID_NEXT, &found_hlink);
    ok(hres == E_FAIL, "got 0x%08lx\n", hres);

    hres = IHlinkBrowseContext_GetHlink(bc, HLID_CURRENT, &found_hlink);
    ok(hres == S_OK, "GetHlink failed: 0x%08lx\n", hres);

    hres = IHlink_GetMonikerReference(found_hlink, HLINKGETREF_DEFAULT, &found_moniker, NULL);
    ok(hres == S_OK, "GetMonikerReference failed: 0x%08lx\n", hres);

    hres = IMoniker_GetDisplayName(found_moniker, bindctx, NULL, &found_name);
    ok(hres == S_OK, "GetDisplayName failed: 0x%08lx\n", hres);
    ok(!lstrcmpW(found_name, exp_name), "Found display name should have been %s, was: %s\n", wine_dbgstr_w(exp_name), wine_dbgstr_w(found_name));

    CoTaskMemFree(exp_name);
    CoTaskMemFree(found_name);

    IBindCtx_Release(bindctx);
    IMoniker_Release(found_moniker);
    IHlink_Release(found_hlink);
    IHlinkBrowseContext_Release(bc);
    IMoniker_Release(dummy);
}

static void test_BrowseWindowInfo(void)
{
    IHlinkBrowseContext *bc;
    HLBWINFO bwinfo_set, bwinfo_get;
    HRESULT hres;

    hres = HlinkCreateBrowseContext(NULL, &IID_IHlinkBrowseContext, (void**)&bc);
    ok(hres == S_OK, "HlinkCreateBrowseContext failed: 0x%08lx\n", hres);

    hres = IHlinkBrowseContext_GetBrowseWindowInfo(bc, NULL);
    ok(hres == E_INVALIDARG, "GetBrowseWindow failed with wrong code: 0x%08lx\n", hres);

    hres = IHlinkBrowseContext_SetBrowseWindowInfo(bc, NULL);
    ok(hres == E_INVALIDARG, "SetBrowseWindow failed with wrong code: 0x%08lx\n", hres);

    memset(&bwinfo_get, -1, sizeof(HLBWINFO));

    hres = IHlinkBrowseContext_GetBrowseWindowInfo(bc, &bwinfo_get);
    ok(hres == S_OK, "GetBrowseWindowInfo failed: 0x%08lx\n", hres);
    ok(bwinfo_get.cbSize == 0, "Got wrong size: %lx\n", bwinfo_get.cbSize);

    bwinfo_set.cbSize = sizeof(HLBWINFO);
    bwinfo_set.grfHLBWIF = HLBWIF_WEBTOOLBARHIDDEN;
    bwinfo_set.rcFramePos.left = 1;
    bwinfo_set.rcFramePos.right = 2;
    bwinfo_set.rcFramePos.top = 3;
    bwinfo_set.rcFramePos.bottom = 4;
    bwinfo_set.rcDocPos.left = 5;
    bwinfo_set.rcDocPos.right = 6;
    bwinfo_set.rcDocPos.top = 7;
    bwinfo_set.rcDocPos.bottom = 8;
    bwinfo_set.hltbinfo.uDockType = 4321;
    bwinfo_set.hltbinfo.rcTbPos.left = 9;
    bwinfo_set.hltbinfo.rcTbPos.right = 10;
    bwinfo_set.hltbinfo.rcTbPos.top = 11;
    bwinfo_set.hltbinfo.rcTbPos.bottom = 12;
    hres = IHlinkBrowseContext_SetBrowseWindowInfo(bc, &bwinfo_set);
    ok(hres == S_OK, "SetBrowseWindowInfo failed: 0x%08lx\n", hres);

    memset(&bwinfo_get, 0, sizeof(HLBWINFO));

    hres = IHlinkBrowseContext_GetBrowseWindowInfo(bc, &bwinfo_get);
    ok(hres == S_OK, "GetBrowseWindowInfo failed: 0x%08lx\n", hres);
    ok(!memcmp(&bwinfo_set, &bwinfo_get, sizeof(HLBWINFO)), "Set and Get differ\n");

    IHlinkBrowseContext_Release(bc);
}

static HRESULT WINAPI Unknown_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
    {
        *ppv = iface;
        return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG WINAPI Unknown_AddRef(IUnknown *iface)
{
    return 2;
}

static ULONG WINAPI Unknown_Release(IUnknown *iface)
{
    return 1;
}

static IUnknownVtbl UnknownVtbl = {
    Unknown_QueryInterface,
    Unknown_AddRef,
    Unknown_Release,
};

static IUnknown Unknown = { &UnknownVtbl };

static void test_GetObject(void)
{
    IHlinkBrowseContext *bc;
    IMoniker *dummy;
    IUnknown *unk;
    WCHAR one[] = {'1',0};
    WCHAR five[] = {'5',0};
    DWORD cookie;
    HRESULT hres;

    hres = CreateItemMoniker(one, five, &dummy);
    ok(hres == S_OK, "CreateItemMoniker() failed: 0x%08lx\n", hres);

    hres = HlinkCreateBrowseContext(NULL, &IID_IHlinkBrowseContext, (void **)&bc);
    ok(hres == S_OK, "HlinkCreateBrowseContext() failed: 0x%08lx\n", hres);

    hres = IHlinkBrowseContext_GetObject(bc, dummy, FALSE, &unk);
    ok(hres == MK_E_UNAVAILABLE, "expected MK_E_UNAVAILABLE, got 0x%08lx\n", hres);

    hres = IHlinkBrowseContext_Register(bc, 0, &Unknown, dummy, &cookie);
    ok(hres == S_OK, "Register() failed: 0x%08lx\n", hres);

    hres = IHlinkBrowseContext_GetObject(bc, dummy, FALSE, &unk);
    ok(hres == S_OK, "GetObject() failed: 0x%08lx\n", hres);
    ok(unk == &Unknown, "wrong object returned\n");

    hres = IHlinkBrowseContext_Revoke(bc, cookie);
    ok(hres == S_OK, "Revoke() failed: 0x%08lx\n", hres);

    hres = IHlinkBrowseContext_GetObject(bc, dummy, FALSE, &unk);
    ok(hres == MK_E_UNAVAILABLE, "expected MK_E_UNAVAILABLE, got 0x%08lx\n", hres);

    IHlinkBrowseContext_Release(bc);
    IMoniker_Release(dummy);
}

static void test_interfaces(void)
{
    IHlinkBrowseContext *bc;
    HRESULT hr;

    hr = HlinkCreateBrowseContext(NULL, &IID_IMoniker, (void **)&bc);
    ok(hr == E_NOINTERFACE, "Unexpected hr %#lx.\n", hr);
}

START_TEST(browse_ctx)
{
    CoInitialize(NULL);

    test_interfaces();
    test_SetInitialHlink();
    test_BrowseWindowInfo();
    test_GetObject();

    CoUninitialize();
}
