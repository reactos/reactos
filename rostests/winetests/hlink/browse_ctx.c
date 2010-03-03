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

/* Win9x and WinMe don't have lstrcmpW */
static int strcmp_ww(const WCHAR *str1, const WCHAR *str2)
{
    DWORD len1 = lstrlenW(str1);
    DWORD len2 = lstrlenW(str2);

    if (len1 != len2) return 1;
    return memcmp(str1, str2, len1 * sizeof(WCHAR));
}

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
    ok(hres == S_OK, "CreateBindCtx failed: 0x%08x\n", hres);

    hres = CreateItemMoniker(one, five, &dummy);
    ok(hres == S_OK, "CreateItemMoniker failed: 0x%08x\n", hres);

    hres = IMoniker_GetDisplayName(dummy, bindctx, NULL, &exp_name);
    ok(hres == S_OK, "GetDisplayName failed: 0x%08x\n", hres);

    hres = HlinkCreateBrowseContext(NULL, &IID_IHlinkBrowseContext, (void**)&bc);
    ok(hres == S_OK, "HlinkCreateBrowseContext failed: 0x%08x\n", hres);

    hres = IHlinkBrowseContext_SetInitialHlink(bc, dummy, one, NULL);
    ok(hres == S_OK, "SetInitialHlink failed: 0x%08x\n", hres);

    hres = IHlinkBrowseContext_GetHlink(bc, HLID_CURRENT, &found_hlink);
    ok(hres == S_OK, "GetHlink failed: 0x%08x\n", hres);

    hres = IHlink_GetMonikerReference(found_hlink, HLINKGETREF_DEFAULT, &found_moniker, NULL);
    ok(hres == S_OK, "GetMonikerReference failed: 0x%08x\n", hres);

    hres = IMoniker_GetDisplayName(found_moniker, bindctx, NULL, &found_name);
    ok(hres == S_OK, "GetDisplayName failed: 0x%08x\n", hres);
    ok(!strcmp_ww(found_name, exp_name), "Found display name should have been %s, was: %s\n", wine_dbgstr_w(exp_name), wine_dbgstr_w(found_name));

    CoTaskMemFree(exp_name);
    CoTaskMemFree(found_name);

    IBindCtx_Release(bindctx);
    IMoniker_Release(found_moniker);
    IHlink_Release(found_hlink);
    IHlinkBrowseContext_Release(bc);
    IMoniker_Release(dummy);
}

START_TEST(browse_ctx)
{
    CoInitialize(NULL);

    test_SetInitialHlink();

    CoUninitialize();
}
