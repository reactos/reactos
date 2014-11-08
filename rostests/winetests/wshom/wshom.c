/*
 * Copyright 2011 Jacek Caban for CodeWeavers
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

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COBJMACROS
#define CONST_VTABLE

#include <windef.h>
#include <winbase.h>
#include <initguid.h>
#include <dispex.h>
#include <wshom.h>
#include <wine/test.h>

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);

#define EXPECT_HR(hr,hr_exp) \
    ok(hr == hr_exp, "got 0x%08x, expected 0x%08x\n", hr, hr_exp)

static void test_wshshell(void)
{
    static const WCHAR desktopW[] = {'D','e','s','k','t','o','p',0};
    static const WCHAR lnk1W[] = {'f','i','l','e','.','l','n','k',0};
    static const WCHAR pathW[] = {'%','P','A','T','H','%',0};
    static const WCHAR sysW[] = {'S','Y','S','T','E','M',0};
    static const WCHAR path2W[] = {'P','A','T','H',0};
    IWshEnvironment *env;
    IWshShell3 *sh3;
    IDispatchEx *dispex;
    IWshCollection *coll;
    IDispatch *disp, *shortcut;
    IUnknown *shell;
    IFolderCollection *folders;
    IWshShortcut *shcut;
    ITypeInfo *ti;
    HRESULT hr;
    TYPEATTR *tattr;
    DISPPARAMS dp;
    EXCEPINFO ei;
    VARIANT arg, res;
    BSTR str, ret;
    UINT err;

    hr = CoCreateInstance(&CLSID_WshShell, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IDispatch, (void**)&disp);
    if(FAILED(hr)) {
        win_skip("Could not create WshShell object: %08x\n", hr);
        return;
    }

    hr = IDispatch_QueryInterface(disp, &IID_IWshShell3, (void**)&shell);
    EXPECT_HR(hr, S_OK);
    IDispatch_Release(disp);

    hr = IDispatch_QueryInterface(disp, &IID_IDispatchEx, (void**)&dispex);
    EXPECT_HR(hr, E_NOINTERFACE);

    hr = IUnknown_QueryInterface(shell, &IID_IWshShell3, (void**)&sh3);
    EXPECT_HR(hr, S_OK);

    hr = IWshShell3_get_SpecialFolders(sh3, &coll);
    EXPECT_HR(hr, S_OK);

    hr = IWshCollection_QueryInterface(coll, &IID_IFolderCollection, (void**)&folders);
    EXPECT_HR(hr, E_NOINTERFACE);

    hr = IWshCollection_QueryInterface(coll, &IID_IDispatch, (void**)&disp);
    EXPECT_HR(hr, S_OK);

    hr = IDispatch_GetTypeInfo(disp, 0, 0, &ti);
    EXPECT_HR(hr, S_OK);

    hr = ITypeInfo_GetTypeAttr(ti, &tattr);
    EXPECT_HR(hr, S_OK);
    ok(IsEqualIID(&tattr->guid, &IID_IWshCollection), "got wrong type guid\n");
    ITypeInfo_ReleaseTypeAttr(ti, tattr);

    /* try to call Item() with normal IDispatch procedure */
    str = SysAllocString(desktopW);
    V_VT(&arg) = VT_BSTR;
    V_BSTR(&arg) = str;
    dp.rgvarg = &arg;
    dp.rgdispidNamedArgs = NULL;
    dp.cArgs = 1;
    dp.cNamedArgs = 0;
    hr = IDispatch_Invoke(disp, DISPID_VALUE, &IID_NULL, 1033, DISPATCH_PROPERTYGET, &dp, &res, &ei, &err);
    EXPECT_HR(hr, DISP_E_MEMBERNOTFOUND);

    /* try Item() directly, it returns directory path apparently */
    V_VT(&res) = VT_EMPTY;
    hr = IWshCollection_Item(coll, &arg, &res);
    EXPECT_HR(hr, S_OK);
    ok(V_VT(&res) == VT_BSTR, "got res type %d\n", V_VT(&res));
    SysFreeString(str);
    VariantClear(&res);

    /* CreateShortcut() */
    str = SysAllocString(lnk1W);
    hr = IWshShell3_CreateShortcut(sh3, str, &shortcut);
    EXPECT_HR(hr, S_OK);
    SysFreeString(str);
    hr = IDispatch_QueryInterface(shortcut, &IID_IWshShortcut, (void**)&shcut);
    EXPECT_HR(hr, S_OK);

    hr = IWshShortcut_get_Arguments(shcut, NULL);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);

    hr = IWshShortcut_get_IconLocation(shcut, NULL);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);

    IWshShortcut_Release(shcut);
    IDispatch_Release(shortcut);

    /* ExpandEnvironmentStrings */
    hr = IWshShell3_ExpandEnvironmentStrings(sh3, NULL, NULL);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);

    str = SysAllocString(pathW);
    hr = IWshShell3_ExpandEnvironmentStrings(sh3, str, NULL);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);
    SysFreeString(str);

    V_VT(&arg) = VT_BSTR;
    V_BSTR(&arg) = SysAllocString(sysW);
    hr = IWshShell3_get_Environment(sh3, &arg, &env);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    VariantClear(&arg);

    hr = IWshEnvironment_get_Item(env, NULL, NULL);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);

    ret = (BSTR)0x1;
    hr = IWshEnvironment_get_Item(env, NULL, &ret);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(ret && !*ret, "got %p\n", ret);
    SysFreeString(ret);

    /* invalid var name */
    str = SysAllocString(lnk1W);
    hr = IWshEnvironment_get_Item(env, str, NULL);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);

    ret = NULL;
    hr = IWshEnvironment_get_Item(env, str, &ret);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(ret && *ret == 0, "got %s\n", wine_dbgstr_w(ret));
    SysFreeString(ret);
    SysFreeString(str);

    /* valid name */
    str = SysAllocString(path2W);
    hr = IWshEnvironment_get_Item(env, str, &ret);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(ret && *ret != 0, "got %s\n", wine_dbgstr_w(ret));
    SysFreeString(ret);
    SysFreeString(str);

    IWshEnvironment_Release(env);

    IWshCollection_Release(coll);
    IDispatch_Release(disp);
    IWshShell3_Release(sh3);
    IUnknown_Release(shell);
}

START_TEST(wshom)
{
    CoInitialize(NULL);

    test_wshshell();

    CoUninitialize();
}
