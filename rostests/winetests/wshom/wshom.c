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
#include <winreg.h>
#include <initguid.h>
#include <dispex.h>
#include <wshom.h>
#include <wine/test.h>

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);

#define EXPECT_HR(hr,hr_exp) \
    ok(hr == hr_exp, "got 0x%08x, expected 0x%08x\n", hr, hr_exp)

static void test_wshshell(void)
{
    static const WCHAR notepadW[] = {'n','o','t','e','p','a','d','.','e','x','e',0};
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
    IUnknown *shell, *unk;
    IFolderCollection *folders;
    IWshShortcut *shcut;
    ITypeInfo *ti;
    HRESULT hr;
    TYPEATTR *tattr;
    DISPPARAMS dp;
    EXCEPINFO ei;
    VARIANT arg, res, arg2;
    BSTR str, ret;
    DWORD retval;
    UINT err;

    hr = CoCreateInstance(&CLSID_WshShell, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IDispatch, (void**)&disp);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IDispatch_QueryInterface(disp, &IID_IWshShell3, (void**)&shell);
    EXPECT_HR(hr, S_OK);
    IDispatch_Release(disp);

    hr = IDispatch_QueryInterface(disp, &IID_IDispatchEx, (void**)&dispex);
    EXPECT_HR(hr, E_NOINTERFACE);

    hr = IUnknown_QueryInterface(shell, &IID_IWshShell3, (void**)&sh3);
    EXPECT_HR(hr, S_OK);

    hr = IWshShell3_QueryInterface(sh3, &IID_IObjectWithSite, (void**)&unk);
    ok(hr == E_NOINTERFACE, "got 0x%08x\n", hr);

    hr = IWshShell3_QueryInterface(sh3, &IID_IWshShell, (void**)&unk);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IUnknown_Release(unk);

    hr = IWshShell3_QueryInterface(sh3, &IID_IWshShell2, (void**)&unk);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IUnknown_Release(unk);

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

    V_VT(&arg) = VT_I2;
    V_I2(&arg) = 0;
    V_VT(&arg2) = VT_ERROR;
    V_ERROR(&arg2) = DISP_E_PARAMNOTFOUND;

    str = SysAllocString(notepadW);
    hr = IWshShell3_Run(sh3, str, &arg, &arg2, NULL);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);

    retval = 10;
    hr = IWshShell3_Run(sh3, str, NULL, &arg2, &retval);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);
    ok(retval == 10, "got %u\n", retval);

    retval = 10;
    hr = IWshShell3_Run(sh3, str, &arg, NULL, &retval);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);
    ok(retval == 10, "got %u\n", retval);

    retval = 10;
    V_VT(&arg2) = VT_ERROR;
    V_ERROR(&arg2) = 0;
    hr = IWshShell3_Run(sh3, str, &arg, &arg2, &retval);
    ok(hr == DISP_E_TYPEMISMATCH, "got 0x%08x\n", hr);
    ok(retval == 10, "got %u\n", retval);

    SysFreeString(str);

    IWshCollection_Release(coll);
    IDispatch_Release(disp);
    IWshShell3_Release(sh3);
    IUnknown_Release(shell);
}

/* delete key and all its subkeys */
static DWORD delete_key(HKEY hkey)
{
    char name[MAX_PATH];
    DWORD ret;

    while (!(ret = RegEnumKeyA(hkey, 0, name, sizeof(name)))) {
        HKEY tmp;
        if (!(ret = RegOpenKeyExA(hkey, name, 0, KEY_ENUMERATE_SUB_KEYS, &tmp))) {
            ret = delete_key(tmp);
            RegCloseKey(tmp);
        }
        if (ret) break;
    }
    if (ret != ERROR_NO_MORE_ITEMS) return ret;
    RegDeleteKeyA(hkey, "");
    return 0;
}

static void test_registry(void)
{
    static const WCHAR keypathW[] = {'H','K','E','Y','_','C','U','R','R','E','N','T','_','U','S','E','R','\\',
        'S','o','f','t','w','a','r','e','\\','W','i','n','e','\\','T','e','s','t','\\',0};

    static const WCHAR regszW[] = {'r','e','g','s','z',0};
    static const WCHAR regdwordW[] = {'r','e','g','d','w','o','r','d',0};
    static const WCHAR regbinaryW[] = {'r','e','g','b','i','n','a','r','y',0};
    static const WCHAR regmultiszW[] = {'r','e','g','m','u','l','t','i','s','z',0};

    static const WCHAR regsz1W[] = {'H','K','E','Y','_','C','U','R','R','E','N','T','_','U','S','E','R','\\',
        'S','o','f','t','w','a','r','e','\\','W','i','n','e','\\','T','e','s','t','\\','r','e','g','s','z','1',0};
    static const WCHAR foobarW[] = {'f','o','o','b','a','r',0};
    static const WCHAR fooW[] = {'f','o','o',0};
    static const WCHAR brokenW[] = {'H','K','E','Y','_','b','r','o','k','e','n','_','k','e','y',0};
    static const WCHAR broken2W[] = {'H','K','E','Y','_','C','U','R','R','E','N','T','_','U','S','E','R','a',0};
    WCHAR pathW[MAX_PATH];
    DWORD dwvalue, type;
    VARIANT value, v;
    IWshShell3 *sh3;
    VARTYPE vartype;
    LONG bound;
    HRESULT hr;
    BSTR name;
    HKEY root;
    LONG ret;
    UINT dim;

    hr = CoCreateInstance(&CLSID_WshShell, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IWshShell3, (void**)&sh3);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    /* RegRead() */
    V_VT(&value) = VT_I2;
    hr = IWshShell3_RegRead(sh3, NULL, &value);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);
    ok(V_VT(&value) == VT_I2, "got %d\n", V_VT(&value));

    name = SysAllocString(brokenW);
    hr = IWshShell3_RegRead(sh3, name, NULL);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);
    V_VT(&value) = VT_I2;
    hr = IWshShell3_RegRead(sh3, name, &value);
    ok(hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND), "got 0x%08x\n", hr);
    ok(V_VT(&value) == VT_I2, "got %d\n", V_VT(&value));
    SysFreeString(name);

    name = SysAllocString(broken2W);
    V_VT(&value) = VT_I2;
    hr = IWshShell3_RegRead(sh3, name, &value);
    ok(hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND), "got 0x%08x\n", hr);
    ok(V_VT(&value) == VT_I2, "got %d\n", V_VT(&value));
    SysFreeString(name);

    ret = RegCreateKeyA(HKEY_CURRENT_USER, "Software\\Wine\\Test", &root);
    ok(ret == 0, "got %d\n", ret);

    ret = RegSetValueExA(root, "regsz", 0, REG_SZ, (const BYTE*)"foobar", 7);
    ok(ret == 0, "got %d\n", ret);

    ret = RegSetValueExA(root, "regmultisz", 0, REG_MULTI_SZ, (const BYTE*)"foo\0bar\0", 9);
    ok(ret == 0, "got %d\n", ret);

    dwvalue = 10;
    ret = RegSetValueExA(root, "regdword", 0, REG_DWORD, (const BYTE*)&dwvalue, sizeof(dwvalue));
    ok(ret == 0, "got %d\n", ret);

    dwvalue = 11;
    ret = RegSetValueExA(root, "regbinary", 0, REG_BINARY, (const BYTE*)&dwvalue, sizeof(dwvalue));
    ok(ret == 0, "got %d\n", ret);

    /* REG_SZ */
    lstrcpyW(pathW, keypathW);
    lstrcatW(pathW, regszW);
    name = SysAllocString(pathW);
    VariantInit(&value);
    hr = IWshShell3_RegRead(sh3, name, &value);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(V_VT(&value) == VT_BSTR, "got %d\n", V_VT(&value));
    ok(!lstrcmpW(V_BSTR(&value), foobarW), "got %s\n", wine_dbgstr_w(V_BSTR(&value)));
    VariantClear(&value);
    SysFreeString(name);

    /* REG_DWORD */
    lstrcpyW(pathW, keypathW);
    lstrcatW(pathW, regdwordW);
    name = SysAllocString(pathW);
    VariantInit(&value);
    hr = IWshShell3_RegRead(sh3, name, &value);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(V_VT(&value) == VT_I4, "got %d\n", V_VT(&value));
    ok(V_I4(&value) == 10, "got %d\n", V_I4(&value));
    SysFreeString(name);

    /* REG_BINARY */
    lstrcpyW(pathW, keypathW);
    lstrcatW(pathW, regbinaryW);
    name = SysAllocString(pathW);
    VariantInit(&value);
    hr = IWshShell3_RegRead(sh3, name, &value);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(V_VT(&value) == (VT_ARRAY|VT_VARIANT), "got 0x%x\n", V_VT(&value));
    dim = SafeArrayGetDim(V_ARRAY(&value));
    ok(dim == 1, "got %u\n", dim);

    hr = SafeArrayGetLBound(V_ARRAY(&value), 1, &bound);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(bound == 0, "got %u\n", bound);

    hr = SafeArrayGetUBound(V_ARRAY(&value), 1, &bound);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(bound == 3, "got %u\n", bound);

    hr = SafeArrayGetVartype(V_ARRAY(&value), &vartype);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(vartype == VT_VARIANT, "got %d\n", vartype);

    bound = 0;
    hr = SafeArrayGetElement(V_ARRAY(&value), &bound, &v);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(V_VT(&v) == VT_UI1, "got %d\n", V_VT(&v));
    ok(V_UI1(&v) == 11, "got %u\n", V_UI1(&v));
    VariantClear(&v);
    VariantClear(&value);
    SysFreeString(name);

    /* REG_MULTI_SZ */
    lstrcpyW(pathW, keypathW);
    lstrcatW(pathW, regmultiszW);
    name = SysAllocString(pathW);
    VariantInit(&value);
    hr = IWshShell3_RegRead(sh3, name, &value);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(V_VT(&value) == (VT_ARRAY|VT_VARIANT), "got 0x%x\n", V_VT(&value));

    dim = SafeArrayGetDim(V_ARRAY(&value));
    ok(dim == 1, "got %u\n", dim);

    hr = SafeArrayGetLBound(V_ARRAY(&value), 1, &bound);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(bound == 0, "got %u\n", bound);

    hr = SafeArrayGetUBound(V_ARRAY(&value), 1, &bound);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(bound == 1, "got %u\n", bound);

    hr = SafeArrayGetVartype(V_ARRAY(&value), &vartype);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(vartype == VT_VARIANT, "got %d\n", vartype);

    bound = 0;
    hr = SafeArrayGetElement(V_ARRAY(&value), &bound, &v);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(V_VT(&v) == VT_BSTR, "got %d\n", V_VT(&v));
    ok(!lstrcmpW(V_BSTR(&v), fooW), "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    VariantClear(&value);

    name = SysAllocString(regsz1W);
    V_VT(&value) = VT_I2;
    hr = IWshShell3_RegRead(sh3, name, &value);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), "got 0x%08x\n", hr);
    ok(V_VT(&value) == VT_I2, "got %d\n", V_VT(&value));
    VariantClear(&value);
    SysFreeString(name);

    delete_key(root);

    /* RegWrite() */
    ret = RegCreateKeyA(HKEY_CURRENT_USER, "Software\\Wine\\Test", &root);
    ok(ret == 0, "got %d\n", ret);

    hr = IWshShell3_RegWrite(sh3, NULL, NULL, NULL);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);

    lstrcpyW(pathW, keypathW);
    lstrcatW(pathW, regszW);
    name = SysAllocString(pathW);

    hr = IWshShell3_RegWrite(sh3, name, NULL, NULL);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);

    VariantInit(&value);
    hr = IWshShell3_RegWrite(sh3, name, &value, NULL);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);

    hr = IWshShell3_RegWrite(sh3, name, &value, &value);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    /* type is optional */
    V_VT(&v) = VT_ERROR;
    V_ERROR(&v) = DISP_E_PARAMNOTFOUND;
    hr = IWshShell3_RegWrite(sh3, name, &value, &v);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    /* default type is REG_SZ */
    V_VT(&value) = VT_I4;
    V_I4(&value) = 12;
    hr = IWshShell3_RegWrite(sh3, name, &value, &v);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    type = REG_NONE;
    ret = RegGetValueA(root, NULL, "regsz", RRF_RT_ANY, &type, NULL, NULL);
    ok(ret == ERROR_SUCCESS, "got %d\n", ret);
    ok(type == REG_SZ, "got %d\n", type);

    ret = RegDeleteValueA(root, "regsz");
    ok(ret == ERROR_SUCCESS, "got %d\n", ret);
    V_VT(&value) = VT_BSTR;
    V_BSTR(&value) = SysAllocString(regszW);
    hr = IWshShell3_RegWrite(sh3, name, &value, &v);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    VariantClear(&value);

    type = REG_NONE;
    ret = RegGetValueA(root, NULL, "regsz", RRF_RT_ANY, &type, NULL, NULL);
    ok(ret == ERROR_SUCCESS, "got %d\n", ret);
    ok(type == REG_SZ, "got %d\n", type);

    ret = RegDeleteValueA(root, "regsz");
    ok(ret == ERROR_SUCCESS, "got %d\n", ret);
    V_VT(&value) = VT_R4;
    V_R4(&value) = 1.2;
    hr = IWshShell3_RegWrite(sh3, name, &value, &v);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    VariantClear(&value);

    type = REG_NONE;
    ret = RegGetValueA(root, NULL, "regsz", RRF_RT_ANY, &type, NULL, NULL);
    ok(ret == ERROR_SUCCESS, "got %d\n", ret);
    ok(type == REG_SZ, "got %d\n", type);

    ret = RegDeleteValueA(root, "regsz");
    ok(ret == ERROR_SUCCESS, "got %d\n", ret);
    V_VT(&value) = VT_R4;
    V_R4(&value) = 1.2;
    V_VT(&v) = VT_I2;
    V_I2(&v) = 1;
    hr = IWshShell3_RegWrite(sh3, name, &value, &v);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);
    VariantClear(&value);

    SysFreeString(name);

    delete_key(root);
    IWshShell3_Release(sh3);
}

START_TEST(wshom)
{
    IUnknown *unk;
    HRESULT hr;

    CoInitialize(NULL);

    hr = CoCreateInstance(&CLSID_WshShell, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IUnknown, (void**)&unk);
    if (FAILED(hr)) {
        win_skip("Could not create WshShell object: %08x\n", hr);
        return;
    }
    IUnknown_Release(unk);

    test_wshshell();
    test_registry();

    CoUninitialize();
}
