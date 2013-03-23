/*
 * ITypeLib and ITypeInfo test
 *
 * Copyright 2004 Jacek Caban
 * Copyright 2006 Dmitry Timoshkov
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
#define COM_NO_WINDOWS_H

#define COBJMACROS
#define CONST_VTABLE

#include <wine/test.h>
//#include <stdarg.h>
#include <stdio.h>

//#include "windef.h"
//#include "winbase.h"
#include <winnls.h>
#include <winreg.h>
#include <objbase.h>
#include <oleauto.h>
//#include "ocidl.h"
//#include "shlwapi.h"
#include <tmarshal.h>

#include <test_reg.h>

#define expect_eq(expr, value, type, format) { type _ret = (expr); ok((value) == _ret, #expr " expected " format " got " format "\n", value, _ret); }
#define expect_int(expr, value) expect_eq(expr, (int)(value), int, "%d")
#define expect_hex(expr, value) expect_eq(expr, (int)(value), int, "0x%x")
#define expect_null(expr) expect_eq(expr, NULL, const void *, "%p")

#define expect_wstr_acpval(expr, value) \
    { \
        CHAR buf[260]; \
        expect_eq(!WideCharToMultiByte(CP_ACP, 0, (expr), -1, buf, 260, NULL, NULL), 0, int, "%d"); \
        ok(lstrcmp(value, buf) == 0, #expr " expected \"%s\" got \"%s\"\n", value, buf); \
    }

#define ole_expect(expr, expect) { \
    HRESULT r = expr; \
    ok(r == (expect), #expr " returned %x, expected %s (%x)\n", r, #expect, expect); \
}

#define ole_check(expr) ole_expect(expr, S_OK);

#define ok_ole_success(hr, func) ok(hr == S_OK, #func " failed with error 0x%08x\n", hr)

static HRESULT (WINAPI *pRegisterTypeLibForUser)(ITypeLib*,OLECHAR*,OLECHAR*);
static HRESULT (WINAPI *pUnRegisterTypeLibForUser)(REFGUID,WORD,WORD,LCID,SYSKIND);

static const WCHAR wszStdOle2[] = {'s','t','d','o','l','e','2','.','t','l','b',0};
static WCHAR wszGUID[] = {'G','U','I','D',0};
static WCHAR wszguid[] = {'g','u','i','d',0};

static const int is_win64 = sizeof(void *) > sizeof(int);

static HRESULT WINAPI invoketest_QueryInterface(IInvokeTest *iface, REFIID riid, void **ret)
{
    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IDispatch) ||
        IsEqualIID(riid, &IID_IInvokeTest))
    {
        *ret = iface;
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI invoketest_AddRef(IInvokeTest *iface)
{
    return 2;
}

static ULONG WINAPI invoketest_Release(IInvokeTest *iface)
{
    return 1;
}

static HRESULT WINAPI invoketest_GetTypeInfoCount(IInvokeTest *iface, UINT *cnt)
{
    ok(0, "unexpected call\n");
    *cnt = 0;
    return E_NOTIMPL;
}

static HRESULT WINAPI invoketest_GetTypeInfo(IInvokeTest *iface, UINT index, LCID lcid, ITypeInfo **ti)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI invoketest_GetIDsOfNames(IInvokeTest *iface, REFIID riid, LPOLESTR *names,
    UINT cnt, LCID lcid, DISPID *dispid)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI invoketest_Invoke(IInvokeTest *iface, DISPID dispid, REFIID riid,
    LCID lcid, WORD flags, DISPPARAMS *dispparams, VARIANT *res, EXCEPINFO *ei, UINT *argerr)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static LONG WINAPI invoketest_get_test(IInvokeTest *iface, LONG i)
{
    return i+1;
}

static const IInvokeTestVtbl invoketestvtbl = {
    invoketest_QueryInterface,
    invoketest_AddRef,
    invoketest_Release,
    invoketest_GetTypeInfoCount,
    invoketest_GetTypeInfo,
    invoketest_GetIDsOfNames,
    invoketest_Invoke,
    invoketest_get_test
};

static IInvokeTest invoketest = { &invoketestvtbl };

static void init_function_pointers(void)
{
    HMODULE hmod = GetModuleHandleA("oleaut32.dll");

    pRegisterTypeLibForUser = (void *)GetProcAddress(hmod, "RegisterTypeLibForUser");
    pUnRegisterTypeLibForUser = (void *)GetProcAddress(hmod, "UnRegisterTypeLibForUser");
}

static void ref_count_test(LPCWSTR type_lib)
{
    ITypeLib *iface;
    ITypeInfo *iti1, *iti2;
    HRESULT hRes;
    int ref_count;

    trace("Loading type library\n");
    hRes = LoadTypeLib(type_lib, &iface);
    ok(hRes == S_OK, "Could not load type library\n");
    if(hRes != S_OK)
        return;

    hRes = ITypeLib_GetTypeInfo(iface, 1, &iti1);
    ok(hRes == S_OK, "ITypeLib_GetTypeInfo failed on index = 1\n");
    ok(ref_count=ITypeLib_Release(iface) > 0, "ITypeLib destroyed while ITypeInfo has back pointer\n");
    if(!ref_count)
        return;

    hRes = ITypeLib_GetTypeInfo(iface, 1, &iti2);
    ok(hRes == S_OK, "ITypeLib_GetTypeInfo failed on index = 1\n");
    ok(iti1 == iti2, "ITypeLib_GetTypeInfo returned different pointers for same indexes\n");

    ITypeLib_AddRef(iface);
    ITypeInfo_Release(iti2);
    ITypeInfo_Release(iti1);
    ok(ITypeLib_Release(iface) == 0, "ITypeLib should be destroyed here.\n");
}

static void test_TypeComp(void)
{
    ITypeLib *pTypeLib;
    ITypeComp *pTypeComp;
    HRESULT hr;
    ULONG ulHash;
    DESCKIND desckind;
    BINDPTR bindptr;
    ITypeInfo *pTypeInfo;
    ITypeInfo *pFontTypeInfo;
    ITypeComp *pTypeComp_tmp;
    static WCHAR wszStdFunctions[] = {'S','t','d','F','u','n','c','t','i','o','n','s',0};
    static WCHAR wszSavePicture[] = {'S','a','v','e','P','i','c','t','u','r','e',0};
    static WCHAR wszOLE_TRISTATE[] = {'O','L','E','_','T','R','I','S','T','A','T','E',0};
    static WCHAR wszUnchecked[] = {'U','n','c','h','e','c','k','e','d',0};
    static WCHAR wszIUnknown[] = {'I','U','n','k','n','o','w','n',0};
    static WCHAR wszFont[] = {'F','o','n','t',0};
    static WCHAR wszStdPicture[] = {'S','t','d','P','i','c','t','u','r','e',0};
    static WCHAR wszOLE_COLOR[] = {'O','L','E','_','C','O','L','O','R',0};
    static WCHAR wszClone[] = {'C','l','o','n','e',0};
    static WCHAR wszclone[] = {'c','l','o','n','e',0};
    static WCHAR wszJunk[] = {'J','u','n','k',0};
    static WCHAR wszAddRef[] = {'A','d','d','R','e','f',0};

    hr = LoadTypeLib(wszStdOle2, &pTypeLib);
    ok_ole_success(hr, LoadTypeLib);

    hr = ITypeLib_GetTypeComp(pTypeLib, &pTypeComp);
    ok_ole_success(hr, ITypeLib_GetTypeComp);

    /* test getting a TKIND_MODULE */
    ulHash = LHashValOfNameSys(SYS_WIN32, LOCALE_NEUTRAL, wszStdFunctions);
    hr = ITypeComp_Bind(pTypeComp, wszStdFunctions, ulHash, 0, &pTypeInfo, &desckind, &bindptr);
    ok_ole_success(hr, ITypeComp_Bind);

    ok(desckind == DESCKIND_TYPECOMP,
        "desckind should have been DESCKIND_TYPECOMP instead of %d\n",
        desckind);
    ok(!pTypeInfo, "pTypeInfo should have been set to NULL\n");

    ITypeComp_Release(bindptr.lptcomp);

    /* test getting a TKIND_MODULE with INVOKE_PROPERTYGET */
    ulHash = LHashValOfNameSys(SYS_WIN32, LOCALE_NEUTRAL, wszStdFunctions);
    hr = ITypeComp_Bind(pTypeComp, wszStdFunctions, ulHash, INVOKE_PROPERTYGET, &pTypeInfo, &desckind, &bindptr);
    ok_ole_success(hr, ITypeComp_Bind);

    ok(desckind == DESCKIND_TYPECOMP,
        "desckind should have been DESCKIND_TYPECOMP instead of %d\n",
        desckind);
    ok(!pTypeInfo, "pTypeInfo should have been set to NULL\n");
    ITypeComp_Release(bindptr.lptcomp);

    /* test getting a function within a TKIND_MODULE */
    ulHash = LHashValOfNameSys(SYS_WIN32, LOCALE_NEUTRAL, wszSavePicture);
    hr = ITypeComp_Bind(pTypeComp, wszSavePicture, ulHash, 0, &pTypeInfo, &desckind, &bindptr);
    ok_ole_success(hr, ITypeComp_Bind);

    ok(desckind == DESCKIND_FUNCDESC,
        "desckind should have been DESCKIND_FUNCDESC instead of %d\n",
        desckind);
    ok(bindptr.lpfuncdesc != NULL, "bindptr.lpfuncdesc should not have been set to NULL\n");
    ITypeInfo_ReleaseFuncDesc(pTypeInfo, bindptr.lpfuncdesc);
    ITypeInfo_Release(pTypeInfo);

    /* test getting a function within a TKIND_MODULE with INVOKE_PROPERTYGET */
    ulHash = LHashValOfNameSys(SYS_WIN32, LOCALE_NEUTRAL, wszSavePicture);
    hr = ITypeComp_Bind(pTypeComp, wszSavePicture, ulHash, INVOKE_PROPERTYGET, &pTypeInfo, &desckind, &bindptr);
    ok(hr == TYPE_E_TYPEMISMATCH,
        "ITypeComp_Bind should have failed with TYPE_E_TYPEMISMATCH instead of 0x%08x\n",
        hr);

    ok(desckind == DESCKIND_NONE,
        "desckind should have been DESCKIND_NONE instead of %d\n",
        desckind);
    ok(!pTypeInfo, "pTypeInfo should have been set to NULL\n");
    ok(!bindptr.lptcomp, "bindptr should have been set to NULL\n");

    /* test getting a TKIND_ENUM */
    ulHash = LHashValOfNameSys(SYS_WIN32, LOCALE_NEUTRAL, wszOLE_TRISTATE);
    hr = ITypeComp_Bind(pTypeComp, wszOLE_TRISTATE, ulHash, 0, &pTypeInfo, &desckind, &bindptr);
    ok_ole_success(hr, ITypeComp_Bind);

    ok(desckind == DESCKIND_TYPECOMP,
        "desckind should have been DESCKIND_TYPECOMP instead of %d\n",
        desckind);
    ok(!pTypeInfo, "pTypeInfo should have been set to NULL\n");

    ITypeComp_Release(bindptr.lptcomp);

    /* test getting a value within a TKIND_ENUM */
    ulHash = LHashValOfNameSys(SYS_WIN32, LOCALE_NEUTRAL, wszUnchecked);
    hr = ITypeComp_Bind(pTypeComp, wszUnchecked, ulHash, 0, &pTypeInfo, &desckind, &bindptr);
    ok_ole_success(hr, ITypeComp_Bind);

    ok(desckind == DESCKIND_VARDESC,
        "desckind should have been DESCKIND_VARDESC instead of %d\n",
        desckind);
    ITypeInfo_ReleaseVarDesc(pTypeInfo, bindptr.lpvardesc);
    ITypeInfo_Release(pTypeInfo);

    /* test getting a TKIND_INTERFACE */
    ulHash = LHashValOfNameSys(SYS_WIN32, LOCALE_NEUTRAL, wszIUnknown);
    hr = ITypeComp_Bind(pTypeComp, wszIUnknown, ulHash, 0, &pTypeInfo, &desckind, &bindptr);
    ok_ole_success(hr, ITypeComp_Bind);

    ok(desckind == DESCKIND_NONE,
        "desckind should have been DESCKIND_NONE instead of %d\n",
        desckind);
    ok(!pTypeInfo, "pTypeInfo should have been set to NULL\n");
    ok(!bindptr.lptcomp, "bindptr should have been set to NULL\n");

    /* test getting a TKIND_DISPATCH */
    ulHash = LHashValOfNameSys(SYS_WIN32, LOCALE_NEUTRAL, wszFont);
    hr = ITypeComp_Bind(pTypeComp, wszFont, ulHash, 0, &pTypeInfo, &desckind, &bindptr);
    ok_ole_success(hr, ITypeComp_Bind);

    ok(desckind == DESCKIND_NONE,
        "desckind should have been DESCKIND_NONE instead of %d\n",
        desckind);
    ok(!pTypeInfo, "pTypeInfo should have been set to NULL\n");
    ok(!bindptr.lptcomp, "bindptr should have been set to NULL\n");

    /* test getting a TKIND_RECORD/TKIND_ALIAS */
    ulHash = LHashValOfNameSys(SYS_WIN32, LOCALE_NEUTRAL, wszGUID);
    hr = ITypeComp_Bind(pTypeComp, wszGUID, ulHash, 0, &pTypeInfo, &desckind, &bindptr);
    ok_ole_success(hr, ITypeComp_Bind);

    ok(desckind == DESCKIND_NONE,
        "desckind should have been DESCKIND_NONE instead of %d\n",
        desckind);
    ok(!pTypeInfo, "pTypeInfo should have been set to NULL\n");
    ok(!bindptr.lptcomp, "bindptr should have been set to NULL\n");

    /* test getting a TKIND_ALIAS */
    ulHash = LHashValOfNameSys(SYS_WIN32, LOCALE_NEUTRAL, wszOLE_COLOR);
    hr = ITypeComp_Bind(pTypeComp, wszOLE_COLOR, ulHash, 0, &pTypeInfo, &desckind, &bindptr);
    ok_ole_success(hr, ITypeComp_Bind);

    ok(desckind == DESCKIND_NONE,
        "desckind should have been DESCKIND_NONE instead of %d\n",
        desckind);
    ok(!pTypeInfo, "pTypeInfo should have been set to NULL\n");
    ok(!bindptr.lptcomp, "bindptr should have been set to NULL\n");

    /* test getting a TKIND_COCLASS */
    ulHash = LHashValOfNameSys(SYS_WIN32, LOCALE_NEUTRAL, wszStdPicture);
    hr = ITypeComp_Bind(pTypeComp, wszStdPicture, ulHash, 0, &pTypeInfo, &desckind, &bindptr);
    ok_ole_success(hr, ITypeComp_Bind);

    ok(desckind == DESCKIND_NONE,
        "desckind should have been DESCKIND_NONE instead of %d\n",
        desckind);
    ok(!pTypeInfo, "pTypeInfo should have been set to NULL\n");
    ok(!bindptr.lptcomp, "bindptr should have been set to NULL\n");

    /* test basic BindType argument handling */
    ulHash = LHashValOfNameSys(SYS_WIN32, LOCALE_NEUTRAL, wszGUID);
    hr = ITypeComp_BindType(pTypeComp, wszGUID, ulHash, NULL, NULL);
    ok(hr == E_INVALIDARG, "Got %08x\n", hr);

    ulHash = LHashValOfNameSys(SYS_WIN32, LOCALE_NEUTRAL, wszGUID);
    pTypeInfo = (void*)0xdeadbeef;
    hr = ITypeComp_BindType(pTypeComp, wszGUID, ulHash, &pTypeInfo, NULL);
    ok(hr == E_INVALIDARG, "Got %08x\n", hr);
    ok(pTypeInfo == (void*)0xdeadbeef, "Got %p\n", pTypeInfo);

    ulHash = LHashValOfNameSys(SYS_WIN32, LOCALE_NEUTRAL, wszGUID);
    pTypeComp_tmp = (void*)0xdeadbeef;
    hr = ITypeComp_BindType(pTypeComp, wszGUID, ulHash, NULL, &pTypeComp_tmp);
    ok(hr == E_INVALIDARG, "Got %08x\n", hr);
    ok(pTypeComp_tmp == (void*)0xdeadbeef, "Got %p\n", pTypeComp_tmp);

    ulHash = LHashValOfNameSys(SYS_WIN32, LOCALE_NEUTRAL, wszGUID);
    pTypeComp_tmp = (void*)0xdeadbeef;
    pTypeInfo = (void*)0xdeadbeef;
    hr = ITypeComp_BindType(pTypeComp, NULL, ulHash, &pTypeInfo, &pTypeComp_tmp);
    ok(hr == E_INVALIDARG, "Got %08x\n", hr);
    ok(pTypeInfo == (void*)0xdeadbeef, "Got %p\n", pTypeInfo);
    ok(pTypeComp_tmp == (void*)0xdeadbeef, "Got %p\n", pTypeComp_tmp);

    ulHash = LHashValOfNameSys(SYS_WIN32, LOCALE_NEUTRAL, wszGUID);
    pTypeComp_tmp = (void*)0xdeadbeef;
    pTypeInfo = (void*)0xdeadbeef;
    hr = ITypeComp_BindType(pTypeComp, wszGUID, ulHash, &pTypeInfo, &pTypeComp_tmp);
    ok_ole_success(hr, ITypeComp_BindType);
    ok(pTypeInfo != NULL, "Got NULL pTypeInfo\n");
    todo_wine ok(pTypeComp_tmp == NULL, "Got pTypeComp_tmp %p\n", pTypeComp_tmp);
    ITypeInfo_Release(pTypeInfo);
    if(pTypeComp_tmp) ITypeComp_Release(pTypeComp_tmp); /* fixme */

    /* test BindType case-insensitivity */
    ulHash = LHashValOfNameSys(SYS_WIN32, LOCALE_NEUTRAL, wszguid);
    pTypeComp_tmp = (void*)0xdeadbeef;
    pTypeInfo = (void*)0xdeadbeef;
    hr = ITypeComp_BindType(pTypeComp, wszguid, ulHash, &pTypeInfo, &pTypeComp_tmp);
    ok_ole_success(hr, ITypeComp_BindType);
    ok(pTypeInfo != NULL, "Got NULL pTypeInfo\n");
    todo_wine ok(pTypeComp_tmp == NULL, "Got pTypeComp_tmp %p\n", pTypeComp_tmp);
    ITypeInfo_Release(pTypeInfo);
    if(pTypeComp_tmp) ITypeComp_Release(pTypeComp_tmp); /* fixme */

    ITypeComp_Release(pTypeComp);

    /* tests for ITypeComp on an interface */
    hr = ITypeLib_GetTypeInfoOfGuid(pTypeLib, &IID_IFont, &pFontTypeInfo);
    ok_ole_success(hr, ITypeLib_GetTypeInfoOfGuid);

    hr = ITypeInfo_GetTypeComp(pFontTypeInfo, &pTypeComp);
    ok_ole_success(hr, ITypeLib_GetTypeComp);

    ulHash = LHashValOfNameSys(SYS_WIN32, LOCALE_NEUTRAL, wszClone);
    hr = ITypeComp_Bind(pTypeComp, wszClone, ulHash, 0, &pTypeInfo, &desckind, &bindptr);
    ok_ole_success(hr, ITypeComp_Bind);

    ok(desckind == DESCKIND_FUNCDESC,
        "desckind should have been DESCKIND_FUNCDESC instead of %d\n",
        desckind);
    ok(bindptr.lpfuncdesc != NULL, "bindptr.lpfuncdesc should not have been set to NULL\n");
    ITypeInfo_ReleaseFuncDesc(pTypeInfo, bindptr.lpfuncdesc);
    ITypeInfo_Release(pTypeInfo);

    ulHash = LHashValOfNameSys(SYS_WIN32, LOCALE_NEUTRAL, wszClone);
    hr = ITypeComp_Bind(pTypeComp, wszClone, ulHash, INVOKE_PROPERTYGET, &pTypeInfo, &desckind, &bindptr);
    ok(hr == TYPE_E_TYPEMISMATCH, "ITypeComp_Bind should have failed with TYPE_E_TYPEMISMATCH instead of 0x%08x\n", hr);

    ok(desckind == DESCKIND_NONE,
        "desckind should have been DESCKIND_NONE instead of %d\n",
        desckind);
    ok(!pTypeInfo, "pTypeInfo should have been set to NULL\n");
    ok(!bindptr.lptcomp, "bindptr should have been set to NULL\n");

    /* tests that the compare is case-insensitive */
    ulHash = LHashValOfNameSys(SYS_WIN32, LOCALE_NEUTRAL, wszclone);
    hr = ITypeComp_Bind(pTypeComp, wszclone, ulHash, 0, &pTypeInfo, &desckind, &bindptr);
    ok_ole_success(hr, ITypeComp_Bind);

    ok(desckind == DESCKIND_FUNCDESC,
        "desckind should have been DESCKIND_FUNCDESC instead of %d\n",
        desckind);
    ok(bindptr.lpfuncdesc != NULL, "bindptr.lpfuncdesc should not have been set to NULL\n");
    ITypeInfo_ReleaseFuncDesc(pTypeInfo, bindptr.lpfuncdesc);
    ITypeInfo_Release(pTypeInfo);

    /* tests nonexistent members */
    desckind = 0xdeadbeef;
    bindptr.lptcomp = (ITypeComp*)0xdeadbeef;
    pTypeInfo = (ITypeInfo*)0xdeadbeef;
    ulHash = LHashValOfNameSys(SYS_WIN32, LOCALE_NEUTRAL, wszJunk);
    hr = ITypeComp_Bind(pTypeComp, wszJunk, ulHash, 0, &pTypeInfo, &desckind, &bindptr);
    ok_ole_success(hr, ITypeComp_Bind);
    ok(desckind == DESCKIND_NONE, "desckind should have been DESCKIND_NONE, was: %d\n", desckind);
    ok(pTypeInfo == NULL, "pTypeInfo should have been NULL, was: %p\n", pTypeInfo);
    ok(bindptr.lptcomp == NULL, "bindptr should have been NULL, was: %p\n", bindptr.lptcomp);

    /* tests inherited members */
    desckind = 0xdeadbeef;
    bindptr.lpfuncdesc = NULL;
    pTypeInfo = NULL;
    ulHash = LHashValOfNameSys(SYS_WIN32, LOCALE_NEUTRAL, wszAddRef);
    hr = ITypeComp_Bind(pTypeComp, wszAddRef, ulHash, 0, &pTypeInfo, &desckind, &bindptr);
    ok_ole_success(hr, ITypeComp_Bind);
    ok(desckind == DESCKIND_FUNCDESC, "desckind should have been DESCKIND_FUNCDESC, was: %d\n", desckind);
    ok(pTypeInfo != NULL, "pTypeInfo should not have been NULL, was: %p\n", pTypeInfo);
    ok(bindptr.lpfuncdesc != NULL, "bindptr should not have been NULL, was: %p\n", bindptr.lpfuncdesc);
    ITypeInfo_ReleaseFuncDesc(pTypeInfo, bindptr.lpfuncdesc);
    ITypeInfo_Release(pTypeInfo);

    ITypeComp_Release(pTypeComp);
    ITypeInfo_Release(pFontTypeInfo);
    ITypeLib_Release(pTypeLib);
}

static void test_CreateDispTypeInfo(void)
{
    ITypeInfo *pTypeInfo, *pTI2;
    HRESULT hr;
    INTERFACEDATA ifdata;
    METHODDATA methdata[4];
    PARAMDATA parms1[2];
    PARAMDATA parms3[1];
    TYPEATTR *pTypeAttr;
    HREFTYPE href;
    FUNCDESC *pFuncDesc;
    MEMBERID memid;

    static WCHAR func1[] = {'f','u','n','c','1',0};
    static const WCHAR func2[] = {'f','u','n','c','2',0};
    static const WCHAR func3[] = {'f','u','n','c','3',0};
    static const WCHAR parm1[] = {'p','a','r','m','1',0};
    static const WCHAR parm2[] = {'p','a','r','m','2',0};
    OLECHAR *name = func1;

    ifdata.pmethdata = methdata;
    ifdata.cMembers = sizeof(methdata) / sizeof(methdata[0]);

    methdata[0].szName = SysAllocString(func1);
    methdata[0].ppdata = parms1;
    methdata[0].dispid = 0x123;
    methdata[0].iMeth = 0;
    methdata[0].cc = CC_STDCALL;
    methdata[0].cArgs = 2;
    methdata[0].wFlags = DISPATCH_METHOD;
    methdata[0].vtReturn = VT_HRESULT;
    parms1[0].szName = SysAllocString(parm1);
    parms1[0].vt = VT_I4;
    parms1[1].szName = SysAllocString(parm2);
    parms1[1].vt = VT_BSTR;

    methdata[1].szName = SysAllocString(func2);
    methdata[1].ppdata = NULL;
    methdata[1].dispid = 0x124;
    methdata[1].iMeth = 1;
    methdata[1].cc = CC_STDCALL;
    methdata[1].cArgs = 0;
    methdata[1].wFlags = DISPATCH_PROPERTYGET;
    methdata[1].vtReturn = VT_I4;

    methdata[2].szName = SysAllocString(func3);
    methdata[2].ppdata = parms3;
    methdata[2].dispid = 0x125;
    methdata[2].iMeth = 3;
    methdata[2].cc = CC_STDCALL;
    methdata[2].cArgs = 1;
    methdata[2].wFlags = DISPATCH_PROPERTYPUT;
    methdata[2].vtReturn = VT_HRESULT;
    parms3[0].szName = SysAllocString(parm1);
    parms3[0].vt = VT_I4;

    methdata[3].szName = SysAllocString(func3);
    methdata[3].ppdata = NULL;
    methdata[3].dispid = 0x125;
    methdata[3].iMeth = 4;
    methdata[3].cc = CC_STDCALL;
    methdata[3].cArgs = 0;
    methdata[3].wFlags = DISPATCH_PROPERTYGET;
    methdata[3].vtReturn = VT_I4;

    hr = CreateDispTypeInfo(&ifdata, LOCALE_NEUTRAL, &pTypeInfo);
    ok(hr == S_OK, "hr %08x\n", hr);

    hr = ITypeInfo_GetTypeAttr(pTypeInfo, &pTypeAttr);
    ok(hr == S_OK, "hr %08x\n", hr);

    ok(pTypeAttr->typekind == TKIND_COCLASS, "typekind %0x\n", pTypeAttr->typekind);
    ok(pTypeAttr->cImplTypes == 1, "cImplTypes %d\n", pTypeAttr->cImplTypes);
    ok(pTypeAttr->cFuncs == 0, "cFuncs %d\n", pTypeAttr->cFuncs);
    ok(pTypeAttr->wTypeFlags == 0, "wTypeFlags %04x\n", pTypeAttr->cFuncs);
    ITypeInfo_ReleaseTypeAttr(pTypeInfo, pTypeAttr);

    hr = ITypeInfo_GetRefTypeOfImplType(pTypeInfo, 0, &href);
    ok(hr == S_OK, "hr %08x\n", hr);
    ok(href == 0, "href = 0x%x\n", href);
    hr = ITypeInfo_GetRefTypeInfo(pTypeInfo, href, &pTI2);
    ok(hr == S_OK, "hr %08x\n", hr);
    hr = ITypeInfo_GetTypeAttr(pTI2, &pTypeAttr);
    ok(hr == S_OK, "hr %08x\n", hr);
    ok(pTypeAttr->typekind == TKIND_INTERFACE, "typekind %0x\n", pTypeAttr->typekind);
    ok(pTypeAttr->cFuncs == 4, "cFuncs %d\n", pTypeAttr->cFuncs);
    ok(IsEqualGUID(&pTypeAttr->guid, &GUID_NULL), "guid {%08x-...}\n", pTypeAttr->guid.Data1);
    ok(pTypeAttr->wTypeFlags == 0, "typeflags %08x\n", pTypeAttr->wTypeFlags);

    ITypeInfo_ReleaseTypeAttr(pTI2, pTypeAttr);

    hr = ITypeInfo_GetFuncDesc(pTI2, 0, &pFuncDesc);
    ok(hr == S_OK, "hr %08x\n", hr);
    ok(pFuncDesc->memid == 0x123, "memid %x\n", pFuncDesc->memid);
    ok(pFuncDesc->funckind == FUNC_VIRTUAL, "funckind %d\n", pFuncDesc->funckind);
    ok(pFuncDesc->invkind == methdata[0].wFlags, "invkind %d\n", pFuncDesc->invkind);
    ok(pFuncDesc->callconv == methdata[0].cc, "callconv %d\n", pFuncDesc->callconv);
    ok(pFuncDesc->cParams == methdata[0].cArgs, "cParams %d\n", pFuncDesc->cParams);
    ok(pFuncDesc->oVft == 0, "oVft %d\n", pFuncDesc->oVft);
    ok(pFuncDesc->wFuncFlags == 0, "oVft %d\n", pFuncDesc->wFuncFlags);
    ok(pFuncDesc->elemdescFunc.tdesc.vt == VT_HRESULT, "ret vt %x\n", pFuncDesc->elemdescFunc.tdesc.vt);
    ok(pFuncDesc->lprgelemdescParam[0].tdesc.vt == VT_I4, "parm 0 vt %x\n", pFuncDesc->lprgelemdescParam[0].tdesc.vt);
    ok(U(pFuncDesc->lprgelemdescParam[0]).paramdesc.wParamFlags == PARAMFLAG_NONE, "parm 0 flags %x\n", U(pFuncDesc->lprgelemdescParam[0]).paramdesc.wParamFlags);

    ok(pFuncDesc->lprgelemdescParam[1].tdesc.vt == VT_BSTR, "parm 1 vt %x\n", pFuncDesc->lprgelemdescParam[1].tdesc.vt);
    ok(U(pFuncDesc->lprgelemdescParam[1]).paramdesc.wParamFlags == PARAMFLAG_NONE, "parm 1 flags %x\n", U(pFuncDesc->lprgelemdescParam[1]).paramdesc.wParamFlags);
    ITypeInfo_ReleaseFuncDesc(pTI2, pFuncDesc);

    hr = ITypeInfo_GetFuncDesc(pTI2, 1, &pFuncDesc);
    ok(hr == S_OK, "hr %08x\n", hr);
    ok(pFuncDesc->funckind == FUNC_VIRTUAL, "funckind %d\n", pFuncDesc->funckind);
    ok(pFuncDesc->invkind == methdata[1].wFlags, "invkind %d\n", pFuncDesc->invkind);
    ok(pFuncDesc->callconv == methdata[1].cc, "callconv %d\n", pFuncDesc->callconv);
    ok(pFuncDesc->cParams == methdata[1].cArgs, "cParams %d\n", pFuncDesc->cParams);
    ok(pFuncDesc->oVft == sizeof(void *), "oVft %d\n", pFuncDesc->oVft);
    ok(pFuncDesc->wFuncFlags == 0, "oVft %d\n", pFuncDesc->wFuncFlags);
    ok(pFuncDesc->elemdescFunc.tdesc.vt == VT_I4, "ret vt %x\n", pFuncDesc->elemdescFunc.tdesc.vt);
    ITypeInfo_ReleaseFuncDesc(pTI2, pFuncDesc);

    hr = ITypeInfo_GetFuncDesc(pTI2, 2, &pFuncDesc);
    ok(hr == S_OK, "hr %08x\n", hr);
    ok(pFuncDesc->funckind == FUNC_VIRTUAL, "funckind %d\n", pFuncDesc->funckind);
    ok(pFuncDesc->invkind == methdata[2].wFlags, "invkind %d\n", pFuncDesc->invkind);
    ok(pFuncDesc->callconv == methdata[2].cc, "callconv %d\n", pFuncDesc->callconv);
    ok(pFuncDesc->cParams == methdata[2].cArgs, "cParams %d\n", pFuncDesc->cParams);
    ok(pFuncDesc->oVft == 3 * sizeof(void *), "oVft %d\n", pFuncDesc->oVft);
    ok(pFuncDesc->wFuncFlags == 0, "oVft %d\n", pFuncDesc->wFuncFlags);
    ok(pFuncDesc->elemdescFunc.tdesc.vt == VT_HRESULT, "ret vt %x\n", pFuncDesc->elemdescFunc.tdesc.vt);
    ok(pFuncDesc->lprgelemdescParam[0].tdesc.vt == VT_I4, "parm 0 vt %x\n", pFuncDesc->lprgelemdescParam[0].tdesc.vt);
    ok(U(pFuncDesc->lprgelemdescParam[0]).paramdesc.wParamFlags == PARAMFLAG_NONE, "parm 0 flags %x\n", U(pFuncDesc->lprgelemdescParam[0]).paramdesc.wParamFlags);
    ITypeInfo_ReleaseFuncDesc(pTI2, pFuncDesc);

    hr = ITypeInfo_GetFuncDesc(pTI2, 3, &pFuncDesc);
    ok(hr == S_OK, "hr %08x\n", hr);
    ok(pFuncDesc->funckind == FUNC_VIRTUAL, "funckind %d\n", pFuncDesc->funckind);
    ok(pFuncDesc->invkind == methdata[3].wFlags, "invkind %d\n", pFuncDesc->invkind);
    ok(pFuncDesc->callconv == methdata[3].cc, "callconv %d\n", pFuncDesc->callconv);
    ok(pFuncDesc->cParams == methdata[3].cArgs, "cParams %d\n", pFuncDesc->cParams);
    ok(pFuncDesc->oVft == 4 * sizeof(void *), "oVft %d\n", pFuncDesc->oVft);
    ok(pFuncDesc->wFuncFlags == 0, "oVft %d\n", pFuncDesc->wFuncFlags);
    ok(pFuncDesc->elemdescFunc.tdesc.vt == VT_I4, "ret vt %x\n", pFuncDesc->elemdescFunc.tdesc.vt);
    ITypeInfo_ReleaseFuncDesc(pTI2, pFuncDesc);

    /* test GetIDsOfNames on a coclass to see if it searches its interfaces */
    hr = ITypeInfo_GetIDsOfNames(pTypeInfo, &name, 1, &memid);
    ok(hr == S_OK, "hr 0x%08x\n", hr);
    ok(memid == 0x123, "memid 0x%08x\n", memid);

    ITypeInfo_Release(pTI2);
    ITypeInfo_Release(pTypeInfo);

    SysFreeString(parms1[0].szName);
    SysFreeString(parms1[1].szName);
    SysFreeString(parms3[0].szName);
    SysFreeString(methdata[0].szName);
    SysFreeString(methdata[1].szName);
    SysFreeString(methdata[2].szName);
    SysFreeString(methdata[3].szName);
}

static const char *create_test_typelib(int res_no)
{
    static char filename[MAX_PATH];
    HANDLE file;
    HRSRC res;
    void *ptr;
    DWORD written;

    GetTempFileNameA( ".", "tlb", 0, filename );
    file = CreateFile( filename, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( file != INVALID_HANDLE_VALUE, "file creation failed\n" );
    if (file == INVALID_HANDLE_VALUE) return NULL;
    res = FindResource( GetModuleHandle(0), MAKEINTRESOURCE(res_no), "TYPELIB" );
    ok( res != 0, "couldn't find resource\n" );
    ptr = LockResource( LoadResource( GetModuleHandle(0), res ));
    WriteFile( file, ptr, SizeofResource( GetModuleHandle(0), res ), &written, NULL );
    ok( written == SizeofResource( GetModuleHandle(0), res ), "couldn't write resource\n" );
    CloseHandle( file );
    return filename;
}

static void test_TypeInfo(void)
{
    ITypeLib *pTypeLib;
    ITypeInfo *pTypeInfo;
    ITypeInfo2 *pTypeInfo2;
    HRESULT hr;
    static WCHAR wszBogus[] = { 'b','o','g','u','s',0 };
    static WCHAR wszGetTypeInfo[] = { 'G','e','t','T','y','p','e','I','n','f','o',0 };
    static WCHAR wszClone[] = {'C','l','o','n','e',0};
    OLECHAR* bogus = wszBogus;
    OLECHAR* pwszGetTypeInfo = wszGetTypeInfo;
    OLECHAR* pwszClone = wszClone;
    DISPID dispidMember;
    DISPPARAMS dispparams;
    GUID bogusguid = {0x806afb4f,0x13f7,0x42d2,{0x89,0x2c,0x6c,0x97,0xc3,0x6a,0x36,0xc1}};
    VARIANT var, res, args[2];
    UINT count, i;
    TYPEKIND kind;
    const char *filenameA;
    WCHAR filename[MAX_PATH];

    hr = LoadTypeLib(wszStdOle2, &pTypeLib);
    ok_ole_success(hr, LoadTypeLib);

    count = ITypeLib_GetTypeInfoCount(pTypeLib);
    ok(count > 0, "got %d\n", count);

    /* invalid index */
    hr = ITypeLib_GetTypeInfo(pTypeLib, count, &pTypeInfo);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "got 0x%08x\n", hr);

    hr = ITypeLib_GetTypeInfo(pTypeLib, 0, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = ITypeLib_GetLibAttr(pTypeLib, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = ITypeLib_GetTypeInfoType(pTypeLib, count, &kind);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "got 0x%08x\n", hr);

    hr = ITypeLib_GetTypeInfoType(pTypeLib, count, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = ITypeLib_GetTypeInfoType(pTypeLib, 0, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = ITypeLib_GetTypeInfoOfGuid(pTypeLib, &IID_IFont, &pTypeInfo);
    ok_ole_success(hr, ITypeLib_GetTypeInfoOfGuid); 

    /* test nonexistent method name */
    hr = ITypeInfo_GetIDsOfNames(pTypeInfo, &bogus, 1, &dispidMember);
    ok(hr == DISP_E_UNKNOWNNAME,
       "ITypeInfo_GetIDsOfNames should have returned DISP_E_UNKNOWNNAME instead of 0x%08x\n",
       hr);

    dispparams.cArgs = 0;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.rgvarg = NULL;

            /* test dispparams not NULL */

    /* invalid member id -- wrong flags -- cNamedArgs not bigger than cArgs */
    dispparams.cNamedArgs = 0;
    hr = ITypeInfo_Invoke(pTypeInfo, (void *)0xdeadbeef, 0xdeadbeef, DISPATCH_PROPERTYGET, &dispparams, NULL, NULL, NULL);
    ok(hr == DISP_E_MEMBERNOTFOUND, "ITypeInfo_Invoke should have returned DISP_E_MEMBERNOTFOUND instead of 0x%08x\n", hr);
    /* invalid member id -- correct flags -- cNamedArgs not bigger than cArgs */
    hr = ITypeInfo_Invoke(pTypeInfo, (void *)0xdeadbeef, 0xdeadbeef, DISPATCH_METHOD, &dispparams, NULL, NULL, NULL);
    ok(hr == DISP_E_MEMBERNOTFOUND, "ITypeInfo_Invoke should have returned DISP_E_MEMBERNOTFOUND instead of 0x%08x\n", hr);

    /* invalid member id -- wrong flags -- cNamedArgs bigger than cArgs */
    dispparams.cNamedArgs = 1;
    hr = ITypeInfo_Invoke(pTypeInfo, (void *)0xdeadbeef, 0xdeadbeef, DISPATCH_PROPERTYGET, &dispparams, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "ITypeInfo_Invoke should have returned E_INVALIDARG instead of 0x%08x\n", hr);
    /* invalid member id -- correct flags -- cNamedArgs bigger than cArgs */
    hr = ITypeInfo_Invoke(pTypeInfo, (void *)0xdeadbeef, 0xdeadbeef, DISPATCH_METHOD, &dispparams, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "ITypeInfo_Invoke should have returned E_INVALIDARG instead of 0x%08x\n", hr);


    hr = ITypeInfo_GetIDsOfNames(pTypeInfo, &pwszClone, 1, &dispidMember);
    ok_ole_success(hr, ITypeInfo_GetIDsOfNames);

    /* correct member id -- wrong flags -- cNamedArgs not bigger than cArgs */
    dispparams.cNamedArgs = 0;
    hr = ITypeInfo_Invoke(pTypeInfo, (void *)0xdeadbeef, dispidMember, DISPATCH_PROPERTYGET, &dispparams, NULL, NULL, NULL);
    ok(hr == DISP_E_MEMBERNOTFOUND, "ITypeInfo_Invoke should have returned DISP_E_MEMBERNOTFOUND instead of 0x%08x\n", hr);
    /* correct member id -- correct flags -- cNamedArgs not bigger than cArgs */
    hr = ITypeInfo_Invoke(pTypeInfo, (void *)0xdeadbeef, dispidMember, DISPATCH_METHOD, &dispparams, NULL, NULL, NULL);
    ok(hr == DISP_E_BADPARAMCOUNT, "ITypeInfo_Invoke should have returned DISP_E_BADPARAMCOUNT instead of 0x%08x\n", hr);

    /* correct member id -- wrong flags -- cNamedArgs bigger than cArgs */
    dispparams.cNamedArgs = 1;
    hr = ITypeInfo_Invoke(pTypeInfo, (void *)0xdeadbeef, dispidMember, DISPATCH_PROPERTYGET, &dispparams, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "ITypeInfo_Invoke should have returned E_INVALIDARG instead of 0x%08x\n", hr);
    /* correct member id -- correct flags -- cNamedArgs bigger than cArgs */
    hr = ITypeInfo_Invoke(pTypeInfo, (void *)0xdeadbeef, dispidMember, DISPATCH_METHOD, &dispparams, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "ITypeInfo_Invoke should have returned E_INVALIDARG instead of 0x%08x\n", hr);

            /* test NULL dispparams */

    /* correct member id -- wrong flags -- cNamedArgs not bigger than cArgs */
    dispparams.cNamedArgs = 0;
    hr = ITypeInfo_Invoke(pTypeInfo, (void *)0xdeadbeef, dispidMember, DISPATCH_PROPERTYGET, NULL, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "ITypeInfo_Invoke should have returned E_INVALIDARG instead of 0x%08x\n", hr);
    /* correct member id -- correct flags -- cNamedArgs not bigger than cArgs */
    hr = ITypeInfo_Invoke(pTypeInfo, (void *)0xdeadbeef, dispidMember, DISPATCH_METHOD, NULL, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "ITypeInfo_Invoke should have returned E_INVALIDARG instead of 0x%08x\n", hr);

    /* correct member id -- wrong flags -- cNamedArgs bigger than cArgs */
    dispparams.cNamedArgs = 1;
    hr = ITypeInfo_Invoke(pTypeInfo, (void *)0xdeadbeef, dispidMember, DISPATCH_PROPERTYGET, NULL, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "ITypeInfo_Invoke should have returned E_INVALIDARG instead of 0x%08x\n", hr);
    /* correct member id -- correct flags -- cNamedArgs bigger than cArgs */
    hr = ITypeInfo_Invoke(pTypeInfo, (void *)0xdeadbeef, dispidMember, DISPATCH_METHOD, NULL, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "ITypeInfo_Invoke should have returned E_INVALIDARG instead of 0x%08x\n", hr);

    ITypeInfo_Release(pTypeInfo);



    hr = ITypeLib_GetTypeInfoOfGuid(pTypeLib, &IID_IDispatch, &pTypeInfo);
    ok_ole_success(hr, ITypeLib_GetTypeInfoOfGuid); 

    hr = ITypeInfo_GetIDsOfNames(pTypeInfo, &pwszGetTypeInfo, 1, &dispidMember);
    ok_ole_success(hr, ITypeInfo_GetIDsOfNames);

    hr = ITypeInfo_QueryInterface(pTypeInfo, &IID_ITypeInfo2, (void**)&pTypeInfo2);
    ok_ole_success(hr, ITypeInfo_QueryInterface);

    if (SUCCEEDED(hr))
    {
        VariantInit(&var);

        V_VT(&var) = VT_I4;

        /* test unknown guid passed to GetCustData */
        hr = ITypeInfo2_GetCustData(pTypeInfo2, &bogusguid, &var);
        ok_ole_success(hr, ITypeInfo_GetCustData);
        ok(V_VT(&var) == VT_EMPTY, "got %i, expected VT_EMPTY\n", V_VT(&var));

        ITypeInfo2_Release(pTypeInfo2);

        VariantClear(&var);
    }

            /* test invoking a method with a [restricted] keyword  */

    /* correct member id -- wrong flags -- cNamedArgs not bigger than cArgs */
    dispparams.cNamedArgs = 0;
    hr = ITypeInfo_Invoke(pTypeInfo, (void *)0xdeadbeef, dispidMember, DISPATCH_PROPERTYGET, &dispparams, NULL, NULL, NULL);
    ok(hr == DISP_E_MEMBERNOTFOUND, "ITypeInfo_Invoke should have returned DISP_E_MEMBERNOTFOUND instead of 0x%08x\n", hr);
    /* correct member id -- correct flags -- cNamedArgs not bigger than cArgs */
    hr = ITypeInfo_Invoke(pTypeInfo, (void *)0xdeadbeef, dispidMember, DISPATCH_METHOD, &dispparams, NULL, NULL, NULL);
    ok(hr == DISP_E_MEMBERNOTFOUND, "ITypeInfo_Invoke should have returned DISP_E_MEMBERNOTFOUND instead of 0x%08x\n", hr);

    /* correct member id -- wrong flags -- cNamedArgs bigger than cArgs */
    dispparams.cNamedArgs = 1;
    hr = ITypeInfo_Invoke(pTypeInfo, (void *)0xdeadbeef, dispidMember, DISPATCH_PROPERTYGET, &dispparams, NULL, NULL, NULL);
    ok(hr == DISP_E_MEMBERNOTFOUND, "ITypeInfo_Invoke should have returned DISP_E_MEMBERNOTFOUND instead of 0x%08x\n", hr);
    /* correct member id -- correct flags -- cNamedArgs bigger than cArgs */
    hr = ITypeInfo_Invoke(pTypeInfo, (void *)0xdeadbeef, dispidMember, DISPATCH_METHOD, &dispparams, NULL, NULL, NULL);
    ok(hr == DISP_E_MEMBERNOTFOUND, "ITypeInfo_Invoke should have returned DISP_E_MEMBERNOTFOUND instead of 0x%08x\n", hr);

            /* test NULL dispparams */

    /* correct member id -- wrong flags -- cNamedArgs not bigger than cArgs */
    dispparams.cNamedArgs = 0;
    hr = ITypeInfo_Invoke(pTypeInfo, (void *)0xdeadbeef, dispidMember, DISPATCH_PROPERTYGET, NULL, NULL, NULL, NULL);
    ok(hr == DISP_E_MEMBERNOTFOUND, "ITypeInfo_Invoke should have returned DISP_E_MEMBERNOTFOUND instead of 0x%08x\n", hr);
    /* correct member id -- correct flags -- cNamedArgs not bigger than cArgs */
    hr = ITypeInfo_Invoke(pTypeInfo, (void *)0xdeadbeef, dispidMember, DISPATCH_METHOD, NULL, NULL, NULL, NULL);
    ok(hr == DISP_E_MEMBERNOTFOUND, "ITypeInfo_Invoke should have returned DISP_E_MEMBERNOTFOUND instead of 0x%08x\n", hr);

    /* correct member id -- wrong flags -- cNamedArgs bigger than cArgs */
    dispparams.cNamedArgs = 1;
    hr = ITypeInfo_Invoke(pTypeInfo, (void *)0xdeadbeef, dispidMember, DISPATCH_PROPERTYGET, NULL, NULL, NULL, NULL);
    ok(hr == DISP_E_MEMBERNOTFOUND, "ITypeInfo_Invoke should have returned DISP_E_MEMBERNOTFOUND instead of 0x%08x\n", hr);
    /* correct member id -- correct flags -- cNamedArgs bigger than cArgs */
    hr = ITypeInfo_Invoke(pTypeInfo, (void *)0xdeadbeef, dispidMember, DISPATCH_METHOD, NULL, NULL, NULL, NULL);
    ok(hr == DISP_E_MEMBERNOTFOUND, "ITypeInfo_Invoke should have returned DISP_E_MEMBERNOTFOUND instead of 0x%08x\n", hr);

    ITypeInfo_Release(pTypeInfo);
    ITypeLib_Release(pTypeLib);

    filenameA = create_test_typelib(3);
    MultiByteToWideChar(CP_ACP, 0, filenameA, -1, filename, MAX_PATH);
    hr = LoadTypeLib(filename, &pTypeLib);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = ITypeLib_GetTypeInfoOfGuid(pTypeLib, &IID_IInvokeTest, &pTypeInfo);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    dispparams.cArgs = 1;
    dispparams.cNamedArgs = 0;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.rgvarg = args;

    V_VT(&args[0]) = VT_I4;
    V_I4(&args[0]) = 0;

    V_VT(&res) = VT_EMPTY;

    i = 0;
    V_VT(&res) = VT_EMPTY;
    V_I4(&res) = 0;
    /* call propget with DISPATCH_METHOD|DISPATCH_PROPERTYGET flags */
    hr = ITypeInfo_Invoke(pTypeInfo, &invoketest, DISPID_VALUE, DISPATCH_METHOD|DISPATCH_PROPERTYGET,
        &dispparams, &res, NULL, &i);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(V_VT(&res) == VT_I4, "got %d\n", V_VT(&res));
    ok(V_I4(&res) == 1, "got %d\n", V_I4(&res));

    i = 0;
    /* call propget with DISPATCH_METHOD flags */
    hr = ITypeInfo_Invoke(pTypeInfo, &invoketest, DISPID_VALUE, DISPATCH_METHOD,
        &dispparams, &res, NULL, &i);
    ok(hr == DISP_E_MEMBERNOTFOUND, "got 0x%08x, %d\n", hr, i);

    i = 0;
    V_VT(&res) = VT_EMPTY;
    V_I4(&res) = 0;
    hr = ITypeInfo_Invoke(pTypeInfo, &invoketest, DISPID_VALUE, DISPATCH_PROPERTYGET,
        &dispparams, &res, NULL, &i);
    ok(hr == S_OK, "got 0x%08x, %d\n", hr, i);
    ok(V_VT(&res) == VT_I4, "got %d\n", V_VT(&res));
    ok(V_I4(&res) == 1, "got %d\n", V_I4(&res));

    ITypeInfo_Release(pTypeInfo);
    ITypeLib_Release(pTypeLib);
    DeleteFileA(filenameA);
}

static int WINAPI int_func( int a0, int a1, int a2, int a3, int a4 )
{
    ok( a0 == 1, "wrong arg0 %x\n", a0 );
    ok( a1 == -1, "wrong arg1 %x\n", a1 );
    ok( a2 == (0x55550000 | 1234), "wrong arg2 %x\n", a2 );
    ok( a3 == 0xdeadbeef, "wrong arg3 %x\n", a3 );
    ok( a4 == 0x555555fd, "wrong arg4 %x\n", a4 );
    return 4321;
}

static double WINAPI double_func( double a0, float a1, double a2, int a3 )
{
    ok( a0 == 1.2, "wrong arg0 %f\n", (double)a0 );
    ok( a1 == 3.25, "wrong arg1 %f\n", (double)a1 );
    ok( a2 == 1.2e12, "wrong arg2 %f\n", (double)a2);
    ok( a3 == -4433.0, "wrong arg3 %f\n", (double)a3 );
    return 4321;
}

static LONGLONG WINAPI longlong_func( LONGLONG a0, CY a1 )
{
    ok( a0 == (((ULONGLONG)0xdead << 32) | 0xbeef), "wrong arg0 %08x%08x\n", (DWORD)(a0 >> 32), (DWORD)a0);
    ok( a1.int64 == ((ULONGLONG)10000 * 12345678), "wrong arg1 %08x%08x\n",
        (DWORD)(a1.int64 >> 32), (DWORD)a1.int64 );
    return ((ULONGLONG)4321 << 32) | 8765;
}

static VARIANT WINAPI variant_func( int a0, BOOL a1, DECIMAL a2, VARIANT a3 )
{
    VARIANT var;
    ok( a0 == 2233, "wrong arg0 %x\n", a0 );
    ok( a1 == 1 || broken(a1 == 0x55550001), "wrong arg1 %x\n", a1 );
    V_VT(&var) = VT_LPWSTR;
    V_UI4(&var) = 0xbabe;
    ok( a2.Hi32 == 1122, "wrong arg2.Hi32 %x\n", a2.Hi32 );
    ok( U1(a2).Lo64 == 3344, "wrong arg2.Lo64 %08x%08x\n", (DWORD)(U1(a2).Lo64 >> 32), (DWORD)U1(a2).Lo64 );
    ok( V_VT(&a3) == VT_EMPTY, "wrong arg3 type %x\n", V_VT(&a3) );
    ok( V_UI4(&a3) == 0xdeadbeef, "wrong arg3 value %x\n", V_UI4(&a3) );
    return var;
}

static int CDECL void_func( int a0, int a1 )
{
    if (is_win64)  /* VT_EMPTY is passed as real arg on win64 */
    {
        ok( a0 == 0x55555555, "wrong arg0 %x\n", a0 );
        ok( a1 == 1111, "wrong arg1 %x\n", a1 );
    }
    else
    {
        ok( a0 == 1111, "wrong arg0 %x\n", a0 );
        ok( a1 == 0, "wrong arg1 %x\n", a1 );
    }
    return 12;
}

static int WINAPI stdcall_func( int a )
{
    return 0;
}

static int WINAPI inst_func( void *inst, int a )
{
    ok( (*(void ***)inst)[3] == inst_func, "wrong ptr %p\n", inst );
    ok( a == 3, "wrong arg %x\n", a );
    return a * 2;
}

static HRESULT WINAPI ret_false_func(void)
{
    return S_FALSE;
}

static const void *vtable[] = { NULL, NULL, NULL, inst_func };

static void test_DispCallFunc(void)
{
    const void **inst = vtable;
    HRESULT res;
    VARIANT result, args[5];
    VARIANTARG *pargs[5];
    VARTYPE types[5];
    int i;

    for (i = 0; i < 5; i++) pargs[i] = &args[i];

    memset( args, 0x55, sizeof(args) );
    types[0] = VT_UI4;
    V_UI4(&args[0]) = 1;
    types[1] = VT_I4;
    V_I4(&args[1]) = -1;
    types[2] = VT_I2;
    V_I2(&args[2]) = 1234;
    types[3] = VT_UI4;
    V_UI4(&args[3]) = 0xdeadbeef;
    types[4] = VT_UI4;
    V_I1(&args[4]) = -3;
    memset( &result, 0xcc, sizeof(result) );
    res = DispCallFunc( NULL, (ULONG_PTR)int_func, CC_STDCALL, VT_UI4, 5, types, pargs, &result );
    ok( res == S_OK, "DispCallFunc failed %x\n", res );
    ok( V_VT(&result) == VT_UI4, "wrong result type %d\n", V_VT(&result) );
    ok( V_UI4(&result) == 4321, "wrong result %u\n", V_UI4(&result) );

    /* the function checks the argument sizes for stdcall */
    if (!is_win64)  /* no stdcall on 64-bit */
    {
        res = DispCallFunc( NULL, (ULONG_PTR)stdcall_func, CC_STDCALL, VT_UI4, 0, types, pargs, &result );
        ok( res == DISP_E_BADCALLEE, "DispCallFunc wrong error %x\n", res );
        res = DispCallFunc( NULL, (ULONG_PTR)stdcall_func, CC_STDCALL, VT_UI4, 1, types, pargs, &result );
        ok( res == S_OK, "DispCallFunc failed %x\n", res );
        res = DispCallFunc( NULL, (ULONG_PTR)stdcall_func, CC_STDCALL, VT_UI4, 2, types, pargs, &result );
        ok( res == DISP_E_BADCALLEE, "DispCallFunc wrong error %x\n", res );
    }

    memset( args, 0x55, sizeof(args) );
    types[0] = VT_R8;
    V_R8(&args[0]) = 1.2;
    types[1] = VT_R4;
    V_R4(&args[1]) = 3.25;
    types[2] = VT_R8;
    V_R8(&args[2]) = 1.2e12;
    types[3] = VT_I4;
    V_I4(&args[3]) = -4433;
    memset( &result, 0xcc, sizeof(result) );
    res = DispCallFunc( NULL, (ULONG_PTR)double_func, CC_STDCALL, VT_R8, 4, types, pargs, &result );
    ok( res == S_OK, "DispCallFunc failed %x\n", res );
    ok( V_VT(&result) == VT_R8, "wrong result type %d\n", V_VT(&result) );
    ok( V_R8(&result) == 4321, "wrong result %f\n", V_R8(&result) );

    memset( args, 0x55, sizeof(args) );
    types[0] = VT_I8;
    V_I8(&args[0]) = ((ULONGLONG)0xdead << 32) | 0xbeef;
    types[1] = VT_CY;
    V_CY(&args[1]).int64 = (ULONGLONG)10000 * 12345678;
    memset( &result, 0xcc, sizeof(result) );
    res = DispCallFunc( NULL, (ULONG_PTR)longlong_func, CC_STDCALL, VT_I8, 2, types, pargs, &result );
    ok( res == S_OK || broken(res == E_INVALIDARG),  /* longlong not supported on <= win2k */
        "DispCallFunc failed %x\n", res );
    if (res == S_OK)
    {
        ok( V_VT(&result) == VT_I8, "wrong result type %d\n", V_VT(&result) );
        ok( V_I8(&result) == (((ULONGLONG)4321 << 32) | 8765), "wrong result %08x%08x\n",
            (DWORD)(V_I8(&result) >> 32), (DWORD)V_I8(&result) );
    }

    memset( args, 0x55, sizeof(args) );
    types[0] = VT_I4;
    V_I4(&args[0]) = 2233;
    types[1] = VT_BOOL;
    V_BOOL(&args[1]) = 1;
    types[2] = VT_DECIMAL;
    V_DECIMAL(&args[2]).Hi32 = 1122;
    U1(V_DECIMAL(&args[2])).Lo64 = 3344;
    types[3] = VT_VARIANT;
    V_VT(&args[3]) = VT_EMPTY;
    V_UI4(&args[3]) = 0xdeadbeef;
    types[4] = VT_EMPTY;
    memset( &result, 0xcc, sizeof(result) );
    res = DispCallFunc( NULL, (ULONG_PTR)variant_func, CC_STDCALL, VT_VARIANT, 5, types, pargs, &result );
    ok( res == S_OK, "DispCallFunc failed %x\n", res );
    ok( V_VT(&result) == VT_LPWSTR, "wrong result type %d\n", V_VT(&result) );
    ok( V_UI4(&result) == 0xbabe, "wrong result %08x\n", V_UI4(&result) );

    memset( args, 0x55, sizeof(args) );
    types[0] = VT_EMPTY;
    types[1] = VT_I4;
    V_I4(&args[1]) = 1111;
    types[2] = VT_EMPTY;
    types[3] = VT_I4;
    V_I4(&args[3]) = 0;
    types[4] = VT_EMPTY;
    memset( &result, 0xcc, sizeof(result) );
    res = DispCallFunc( NULL, (ULONG_PTR)void_func, CC_CDECL, VT_EMPTY, 5, types, pargs, &result );
    ok( res == S_OK, "DispCallFunc failed %x\n", res );
    ok( V_VT(&result) == VT_EMPTY, "wrong result type %d\n", V_VT(&result) );
    if (is_win64)
        ok( V_UI4(&result) == 12, "wrong result %08x\n", V_UI4(&result) );
    else
        ok( V_UI4(&result) == 0xcccccccc, "wrong result %08x\n", V_UI4(&result) );

    memset( args, 0x55, sizeof(args) );
    types[0] = VT_I4;
    V_I4(&args[0]) = 3;
    memset( &result, 0xcc, sizeof(result) );
    res = DispCallFunc( &inst, 3 * sizeof(void*), CC_STDCALL, VT_I4, 1, types, pargs, &result );
    ok( res == S_OK, "DispCallFunc failed %x\n", res );
    ok( V_VT(&result) == VT_I4, "wrong result type %d\n", V_VT(&result) );
    ok( V_I4(&result) == 6, "wrong result %08x\n", V_I4(&result) );

    memset( &result, 0xcc, sizeof(result) );
    res = DispCallFunc(NULL, (ULONG_PTR)ret_false_func, CC_STDCALL, VT_ERROR, 0, NULL, NULL, &result);
    ok(res == S_OK, "DispCallFunc failed: %08x\n", res);
    ok(V_VT(&result) == VT_ERROR, "V_VT(result) = %u\n", V_VT(&result));
    ok(V_ERROR(&result) == S_FALSE, "V_ERROR(result) = %08x\n", V_ERROR(&result));

    memset( &result, 0xcc, sizeof(result) );
    res = DispCallFunc(NULL, (ULONG_PTR)ret_false_func, CC_STDCALL, VT_HRESULT, 0, NULL, NULL, &result);
    ok(res == E_INVALIDARG, "DispCallFunc failed: %08x\n", res);
    ok(V_VT(&result) == 0xcccc, "V_VT(result) = %u\n", V_VT(&result));
}

/* RegDeleteTreeW from dlls/advapi32/registry.c */
static LSTATUS myRegDeleteTreeW(HKEY hKey, LPCWSTR lpszSubKey)
{
    LONG ret;
    DWORD dwMaxSubkeyLen, dwMaxValueLen;
    DWORD dwMaxLen, dwSize;
    WCHAR szNameBuf[MAX_PATH], *lpszName = szNameBuf;
    HKEY hSubKey = hKey;

    if(lpszSubKey)
    {
        ret = RegOpenKeyExW(hKey, lpszSubKey, 0, KEY_READ, &hSubKey);
        if (ret) return ret;
    }

    ret = RegQueryInfoKeyW(hSubKey, NULL, NULL, NULL, NULL,
            &dwMaxSubkeyLen, NULL, NULL, &dwMaxValueLen, NULL, NULL, NULL);
    if (ret) goto cleanup;

    dwMaxSubkeyLen++;
    dwMaxValueLen++;
    dwMaxLen = max(dwMaxSubkeyLen, dwMaxValueLen);
    if (dwMaxLen > sizeof(szNameBuf)/sizeof(WCHAR))
    {
        /* Name too big: alloc a buffer for it */
        if (!(lpszName = HeapAlloc( GetProcessHeap(), 0, dwMaxLen*sizeof(WCHAR))))
        {
            ret = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
    }

    /* Recursively delete all the subkeys */
    while (TRUE)
    {
        dwSize = dwMaxLen;
        if (RegEnumKeyExW(hSubKey, 0, lpszName, &dwSize, NULL,
                          NULL, NULL, NULL)) break;

        ret = myRegDeleteTreeW(hSubKey, lpszName);
        if (ret) goto cleanup;
    }

    if (lpszSubKey)
        ret = RegDeleteKeyW(hKey, lpszSubKey);
    else
        while (TRUE)
        {
            dwSize = dwMaxLen;
            if (RegEnumValueW(hKey, 0, lpszName, &dwSize,
                  NULL, NULL, NULL, NULL)) break;

            ret = RegDeleteValueW(hKey, lpszName);
            if (ret) goto cleanup;
        }

cleanup:
    if (lpszName != szNameBuf)
        HeapFree(GetProcessHeap(), 0, lpszName);
    if(lpszSubKey)
        RegCloseKey(hSubKey);
    return ret;
}

static BOOL do_typelib_reg_key(GUID *uid, WORD maj, WORD min, DWORD arch, LPCWSTR base, BOOL remove)
{
    static const WCHAR typelibW[] = {'T','y','p','e','l','i','b','\\',0};
    static const WCHAR formatW[] = {'\\','%','u','.','%','u','\\','0','\\','w','i','n','%','u',0};
    static const WCHAR format2W[] = {'%','s','_','%','u','_','%','u','.','d','l','l',0};
    WCHAR buf[128];
    HKEY hkey;
    BOOL ret = TRUE;
    DWORD res;

    memcpy(buf, typelibW, sizeof(typelibW));
    StringFromGUID2(uid, buf + lstrlenW(buf), 40);

    if (remove)
    {
        ok(myRegDeleteTreeW(HKEY_CLASSES_ROOT, buf) == ERROR_SUCCESS, "SHDeleteKey failed\n");
        return TRUE;
    }

    wsprintfW(buf + lstrlenW(buf), formatW, maj, min, arch);

    SetLastError(0xdeadbeef);
    res = RegCreateKeyExW(HKEY_CLASSES_ROOT, buf, 0, NULL, 0,
                          KEY_WRITE, NULL, &hkey, NULL);
    if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("W-calls are not implemented\n");
        return FALSE;
    }

    if (res != ERROR_SUCCESS)
    {
        trace("RegCreateKeyExW failed: %u\n", res);
        return FALSE;
    }

    wsprintfW(buf, format2W, base, maj, min);
    if (RegSetValueExW(hkey, NULL, 0, REG_SZ,
                       (BYTE *)buf, (lstrlenW(buf) + 1) * sizeof(WCHAR)) != ERROR_SUCCESS)
    {
        trace("RegSetValueExW failed\n");
        ret = FALSE;
    }
    RegCloseKey(hkey);
    return ret;
}

static void test_QueryPathOfRegTypeLib(DWORD arch)
{
    static const struct test_data
    {
        WORD maj, min;
        HRESULT ret;
        const WCHAR path[16];
    } td[] = {
        { 1, 0, TYPE_E_LIBNOTREGISTERED, { 0 } },
        { 3, 0, S_OK, {'f','a','k','e','_','3','_','0','.','d','l','l',0 } },
        { 3, 1, S_OK, {'f','a','k','e','_','3','_','1','.','d','l','l',0 } },
        { 3, 22, S_OK, {'f','a','k','e','_','3','_','3','7','.','d','l','l',0 } },
        { 3, 37, S_OK, {'f','a','k','e','_','3','_','3','7','.','d','l','l',0 } },
        { 3, 40, S_OK, {'f','a','k','e','_','3','_','3','7','.','d','l','l',0 } },
        { 0xffff, 0xffff, S_OK, {'f','a','k','e','_','5','_','3','7','.','d','l','l',0 } },
        { 0xffff, 0, TYPE_E_LIBNOTREGISTERED, { 0 } },
        { 3, 0xffff, TYPE_E_LIBNOTREGISTERED, { 0 } },
        { 5, 0xffff, TYPE_E_LIBNOTREGISTERED, { 0 } },
        { 4, 0, TYPE_E_LIBNOTREGISTERED, { 0 } }
    };
    static const WCHAR base[] = {'f','a','k','e',0};
    static const WCHAR wrongW[] = {'w','r','o','n','g',0};
    UINT i;
    RPC_STATUS status;
    GUID uid;
    WCHAR uid_str[40];
    HRESULT ret;
    BSTR path;

    status = UuidCreate(&uid);
    ok(!status || status == RPC_S_UUID_LOCAL_ONLY, "UuidCreate error %08x\n", status);

    StringFromGUID2(&uid, uid_str, 40);
    /*trace("GUID: %s\n", wine_dbgstr_w(uid_str));*/

    if (!do_typelib_reg_key(&uid, 3, 0, arch, base, 0)) return;
    if (!do_typelib_reg_key(&uid, 3, 1, arch, base, 0)) return;
    if (!do_typelib_reg_key(&uid, 3, 37, arch, base, 0)) return;
    if (!do_typelib_reg_key(&uid, 5, 37, arch, base, 0)) return;
    if (arch == 64 && !do_typelib_reg_key(&uid, 5, 37, 32, wrongW, 0)) return;

    for (i = 0; i < sizeof(td)/sizeof(td[0]); i++)
    {
        ret = QueryPathOfRegTypeLib(&uid, td[i].maj, td[i].min, 0, &path);
        ok(ret == td[i].ret, "QueryPathOfRegTypeLib(%u.%u) returned %08x\n", td[i].maj, td[i].min, ret);
        if (ret == S_OK)
        {
            ok(!lstrcmpW(td[i].path, path), "typelib %u.%u path doesn't match\n", td[i].maj, td[i].min);
            SysFreeString(path);
        }
    }

    do_typelib_reg_key(&uid, 0, 0, arch, NULL, 1);
}

static void test_inheritance(void)
{
    HRESULT hr;
    ITypeLib *pTL;
    ITypeInfo *pTI, *pTI_p;
    TYPEATTR *pTA;
    HREFTYPE href;
    FUNCDESC *pFD;
    WCHAR path[MAX_PATH];
    CHAR pathA[MAX_PATH];
    static const WCHAR tl_path[] = {'.','\\','m','i','d','l','_','t','m','a','r','s','h','a','l','.','t','l','b',0};

    BOOL use_midl_tlb = 0;

    GetModuleFileNameA(NULL, pathA, MAX_PATH);
    MultiByteToWideChar(CP_ACP, 0, pathA, -1, path, MAX_PATH);

    if(use_midl_tlb)
        memcpy(path, tl_path, sizeof(tl_path));

    hr = LoadTypeLib(path, &pTL);
    if(FAILED(hr)) return;


    /* ItestIF3 is a syntax 2 dispinterface */
    hr = ITypeLib_GetTypeInfoOfGuid(pTL, &DIID_ItestIF3, &pTI);
    ok(hr == S_OK, "hr %08x\n", hr);

    hr = ITypeInfo_GetTypeAttr(pTI, &pTA);
    ok(hr == S_OK, "hr %08x\n", hr);
    ok(pTA->typekind == TKIND_DISPATCH, "kind %04x\n", pTA->typekind);
    ok(pTA->cbSizeVft == 7 * sizeof(void *), "sizevft %d\n", pTA->cbSizeVft);
    ok(pTA->wTypeFlags == TYPEFLAG_FDISPATCHABLE, "typeflags %x\n", pTA->wTypeFlags);
if(use_midl_tlb) {
    ok(pTA->cFuncs == 6, "cfuncs %d\n", pTA->cFuncs);
    ok(pTA->cImplTypes == 1, "cimpltypes %d\n", pTA->cImplTypes);
}
    ITypeInfo_ReleaseTypeAttr(pTI, pTA);

if(use_midl_tlb) {
    hr = ITypeInfo_GetRefTypeOfImplType(pTI, 0, &href);
    ok(hr == S_OK, "hr %08x\n", hr);
    hr = ITypeInfo_GetRefTypeInfo(pTI, href, &pTI_p);
    ok(hr == S_OK, "hr %08x\n", hr);
    hr = ITypeInfo_GetTypeAttr(pTI_p, &pTA);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(IsEqualGUID(&pTA->guid, &IID_IDispatch), "guid {%08x-....\n", pTA->guid.Data1);
    ITypeInfo_ReleaseTypeAttr(pTI_p, pTA);
    ITypeInfo_Release(pTI_p);

    /* Should have six methods */
    hr = ITypeInfo_GetFuncDesc(pTI, 6, &pFD);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "hr %08x\n", hr);
    hr = ITypeInfo_GetFuncDesc(pTI, 5, &pFD);
    ok(hr == S_OK, "hr %08x\n", hr);
    ok(pFD->memid == 0x60020000, "memid %08x\n", pFD->memid);
    ok(pFD->oVft == 5 * sizeof(void *), "oVft %d\n", pFD->oVft);
    ITypeInfo_ReleaseFuncDesc(pTI, pFD);
}
    ITypeInfo_Release(pTI);


    /* ItestIF4 is a syntax 1 dispinterface */
    hr = ITypeLib_GetTypeInfoOfGuid(pTL, &DIID_ItestIF4, &pTI);
    ok(hr == S_OK, "hr %08x\n", hr);

    hr = ITypeInfo_GetTypeAttr(pTI, &pTA);
    ok(hr == S_OK, "hr %08x\n", hr);
    ok(pTA->typekind == TKIND_DISPATCH, "kind %04x\n", pTA->typekind);
    ok(pTA->cbSizeVft == 7 * sizeof(void *), "sizevft %d\n", pTA->cbSizeVft);
    ok(pTA->wTypeFlags == TYPEFLAG_FDISPATCHABLE, "typeflags %x\n", pTA->wTypeFlags);
    ok(pTA->cFuncs == 1, "cfuncs %d\n", pTA->cFuncs);
    ok(pTA->cImplTypes == 1, "cimpltypes %d\n", pTA->cImplTypes);
    ITypeInfo_ReleaseTypeAttr(pTI, pTA);

    hr = ITypeInfo_GetRefTypeOfImplType(pTI, 0, &href);
    ok(hr == S_OK, "hr %08x\n", hr);
    hr = ITypeInfo_GetRefTypeInfo(pTI, href, &pTI_p);
    ok(hr == S_OK, "hr %08x\n", hr);
    hr = ITypeInfo_GetTypeAttr(pTI_p, &pTA);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(IsEqualGUID(&pTA->guid, &IID_IDispatch), "guid {%08x-....\n", pTA->guid.Data1);
    ITypeInfo_ReleaseTypeAttr(pTI_p, pTA);
    ITypeInfo_Release(pTI_p);
    hr = ITypeInfo_GetFuncDesc(pTI, 1, &pFD);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "hr %08x\n", hr);
    hr = ITypeInfo_GetFuncDesc(pTI, 0, &pFD);
    ok(hr == S_OK, "hr %08x\n", hr);
    ok(pFD->memid == 0x1c, "memid %08x\n", pFD->memid);
    ITypeInfo_ReleaseFuncDesc(pTI, pFD);
    ITypeInfo_Release(pTI);


    /* ItestIF5 is dual with inherited ifaces which derive from IUnknown but not IDispatch */
    hr = ITypeLib_GetTypeInfoOfGuid(pTL, &IID_ItestIF5, &pTI);
    ok(hr == S_OK, "hr %08x\n", hr);

    hr = ITypeInfo_GetTypeAttr(pTI, &pTA);
    ok(hr == S_OK, "hr %08x\n", hr);
    if (hr == S_OK)
    {
        ok(pTA->typekind == TKIND_DISPATCH, "kind %04x\n", pTA->typekind);
        ok(pTA->cbSizeVft == 7 * sizeof(void *), "sizevft %d\n", pTA->cbSizeVft);
        if(use_midl_tlb) {
            ok(pTA->wTypeFlags == TYPEFLAG_FDUAL, "typeflags %x\n", pTA->wTypeFlags);
        }
        ok(pTA->cFuncs == 8, "cfuncs %d\n", pTA->cFuncs);
        ok(pTA->cImplTypes == 1, "cimpltypes %d\n", pTA->cImplTypes);
        ITypeInfo_ReleaseTypeAttr(pTI, pTA);
    }
    hr = ITypeInfo_GetRefTypeOfImplType(pTI, 0, &href);
    ok(hr == S_OK, "hr %08x\n", hr);
    hr = ITypeInfo_GetRefTypeInfo(pTI, href, &pTI_p);
    ok(hr == S_OK, "hr %08x\n", hr);
    hr = ITypeInfo_GetTypeAttr(pTI_p, &pTA);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(IsEqualGUID(&pTA->guid, &IID_IDispatch), "guid {%08x-....\n", pTA->guid.Data1);
    ITypeInfo_ReleaseTypeAttr(pTI_p, pTA);
    ITypeInfo_Release(pTI_p);
if(use_midl_tlb) {
    hr = ITypeInfo_GetFuncDesc(pTI, 6, &pFD);
    ok(hr == S_OK, "hr %08x\n", hr);
    ok(pFD->memid == 0x1234, "memid %08x\n", pFD->memid);
    ITypeInfo_ReleaseFuncDesc(pTI, pFD);
}
    ITypeInfo_Release(pTI);

    /* ItestIF7 is dual with inherited ifaces which derive from Dispatch */
    hr = ITypeLib_GetTypeInfoOfGuid(pTL, &IID_ItestIF7, &pTI);
    ok(hr == S_OK, "hr %08x\n", hr);

    hr = ITypeInfo_GetTypeAttr(pTI, &pTA);
    ok(hr == S_OK, "hr %08x\n", hr);
    ok(pTA->typekind == TKIND_DISPATCH, "kind %04x\n", pTA->typekind);
    ok(pTA->cbSizeVft == 7 * sizeof(void *), "sizevft %d\n", pTA->cbSizeVft);
    ok(pTA->wTypeFlags == (TYPEFLAG_FDISPATCHABLE|TYPEFLAG_FDUAL), "typeflags %x\n", pTA->wTypeFlags);
    ok(pTA->cFuncs == 10, "cfuncs %d\n", pTA->cFuncs);
    ok(pTA->cImplTypes == 1, "cimpltypes %d\n", pTA->cImplTypes);
    ITypeInfo_ReleaseTypeAttr(pTI, pTA);

    hr = ITypeInfo_GetRefTypeOfImplType(pTI, 0, &href);
    ok(hr == S_OK, "hr %08x\n", hr);
    hr = ITypeInfo_GetRefTypeInfo(pTI, href, &pTI_p);
    ok(hr == S_OK, "hr %08x\n", hr);
    hr = ITypeInfo_GetTypeAttr(pTI_p, &pTA);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(IsEqualGUID(&pTA->guid, &IID_IDispatch), "guid {%08x-....\n", pTA->guid.Data1);
    ITypeInfo_ReleaseTypeAttr(pTI_p, pTA);
    ITypeInfo_Release(pTI_p);

    hr = ITypeInfo_GetFuncDesc(pTI, 9, &pFD);
    ok(hr == S_OK, "hr %08x\n", hr);
    ok(pFD->memid == 0x1236, "memid %08x\n", pFD->memid);
    ITypeInfo_ReleaseFuncDesc(pTI, pFD);
    ITypeInfo_Release(pTI);

    /* ItestIF10 is a syntax 2 dispinterface which doesn't derive from IUnknown */
    hr = ITypeLib_GetTypeInfoOfGuid(pTL, &DIID_ItestIF10, &pTI);
    ok(hr == S_OK, "hr %08x\n", hr);

    hr = ITypeInfo_GetTypeAttr(pTI, &pTA);
    ok(hr == S_OK, "hr %08x\n", hr);
    ok(pTA->typekind == TKIND_DISPATCH, "kind %04x\n", pTA->typekind);
    ok(pTA->cbSizeVft == 7 * sizeof(void *), "sizevft %d\n", pTA->cbSizeVft);
    ok(pTA->wTypeFlags == TYPEFLAG_FDISPATCHABLE, "typeflags %x\n", pTA->wTypeFlags);
if(use_midl_tlb) {
    ok(pTA->cFuncs == 3, "cfuncs %d\n", pTA->cFuncs);
    ok(pTA->cImplTypes == 1, "cimpltypes %d\n", pTA->cImplTypes);
}
    ITypeInfo_ReleaseTypeAttr(pTI, pTA);

if(use_midl_tlb) {
    hr = ITypeInfo_GetRefTypeOfImplType(pTI, -1, &href);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "hr %08x\n", hr);
    hr = ITypeInfo_GetRefTypeOfImplType(pTI, 0, &href);
    ok(hr == S_OK, "hr %08x\n", hr);
    hr = ITypeInfo_GetRefTypeInfo(pTI, href, &pTI_p);
    ok(hr == S_OK, "hr %08x\n", hr);
    hr = ITypeInfo_GetTypeAttr(pTI_p, &pTA);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(IsEqualGUID(&pTA->guid, &IID_IDispatch), "guid {%08x-....\n", pTA->guid.Data1);
    ITypeInfo_ReleaseTypeAttr(pTI_p, pTA);
    ITypeInfo_Release(pTI_p);

    /* Should have three methods */
    hr = ITypeInfo_GetFuncDesc(pTI, 3, &pFD);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "hr %08x\n", hr);
    hr = ITypeInfo_GetFuncDesc(pTI, 2, &pFD);
    ok(hr == S_OK, "hr %08x\n", hr);
    ok(pFD->memid == 0x60010000, "memid %08x\n", pFD->memid);
    ok(pFD->oVft == 2 * sizeof(void *), "oVft %d\n", pFD->oVft);
    ITypeInfo_ReleaseFuncDesc(pTI, pFD);
}
    ITypeInfo_Release(pTI);

    /* ItestIF11 is a syntax 2 dispinterface which derives from IDispatch */
    hr = ITypeLib_GetTypeInfoOfGuid(pTL, &DIID_ItestIF11, &pTI);
    ok(hr == S_OK, "hr %08x\n", hr);

    hr = ITypeInfo_GetTypeAttr(pTI, &pTA);
    ok(hr == S_OK, "hr %08x\n", hr);
    ok(pTA->typekind == TKIND_DISPATCH, "kind %04x\n", pTA->typekind);
    ok(pTA->cbSizeVft == 7 * sizeof(void *), "sizevft %d\n", pTA->cbSizeVft);
    ok(pTA->wTypeFlags == TYPEFLAG_FDISPATCHABLE, "typeflags %x\n", pTA->wTypeFlags);
if(use_midl_tlb) {
    ok(pTA->cFuncs == 10, "cfuncs %d\n", pTA->cFuncs);
    ok(pTA->cImplTypes == 1, "cimpltypes %d\n", pTA->cImplTypes);
}
    ITypeInfo_ReleaseTypeAttr(pTI, pTA);

if(use_midl_tlb) {
    hr = ITypeInfo_GetRefTypeOfImplType(pTI, 0, &href);
    ok(hr == S_OK, "hr %08x\n", hr);
    hr = ITypeInfo_GetRefTypeInfo(pTI, href, &pTI_p);
    ok(hr == S_OK, "hr %08x\n", hr);
    hr = ITypeInfo_GetTypeAttr(pTI_p, &pTA);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(IsEqualGUID(&pTA->guid, &IID_IDispatch), "guid {%08x-....\n", pTA->guid.Data1);
    ITypeInfo_ReleaseTypeAttr(pTI_p, pTA);
    ITypeInfo_Release(pTI_p);

    /* Should have ten methods */
    hr = ITypeInfo_GetFuncDesc(pTI, 10, &pFD);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "hr %08x\n", hr);
    hr = ITypeInfo_GetFuncDesc(pTI, 9, &pFD);
    ok(hr == S_OK, "hr %08x\n", hr);
    ok(pFD->memid == 0x1236, "memid %08x\n", pFD->memid);
    ok(pFD->oVft == 9 * sizeof(void *), "oVft %d\n", pFD->oVft);

    /* first argument to 10th function is an HREFTYPE from the impl type */
    ok(pFD->cParams == 1, "cParams %i\n", pFD->cParams);
    ok(pFD->lprgelemdescParam[0].tdesc.vt == VT_USERDEFINED,
        "vt 0x%x\n", pFD->lprgelemdescParam[0].tdesc.vt);
    href = U(pFD->lprgelemdescParam[0].tdesc).hreftype;
    ok((href & 0xff000000) == 0x04000000, "href 0x%08x\n", href);
    hr = ITypeInfo_GetRefTypeInfo(pTI, href, &pTI_p);
    ok(hr == S_OK, "hr %08x\n", hr);
    if (SUCCEEDED(hr)) ITypeInfo_Release(pTI_p);
    ITypeInfo_ReleaseFuncDesc(pTI, pFD);
}
    ITypeInfo_Release(pTI);


    /* ItestIF2 is an interface which derives from IUnknown */
    hr = ITypeLib_GetTypeInfoOfGuid(pTL, &IID_ItestIF2, &pTI);
    ok(hr == S_OK, "hr %08x\n", hr);

    hr = ITypeInfo_GetTypeAttr(pTI, &pTA);
    ok(hr == S_OK, "hr %08x\n", hr);
    ok(pTA->typekind == TKIND_INTERFACE, "kind %04x\n", pTA->typekind);
    ok(pTA->cbSizeVft == 6 * sizeof(void *), "sizevft %d\n", pTA->cbSizeVft);
    ok(pTA->wTypeFlags == 0, "typeflags %x\n", pTA->wTypeFlags);
if(use_midl_tlb) {
    ok(pTA->cFuncs == 1, "cfuncs %d\n", pTA->cFuncs);
    ok(pTA->cImplTypes == 1, "cimpltypes %d\n", pTA->cImplTypes);
}
    ITypeInfo_ReleaseTypeAttr(pTI, pTA);

if(use_midl_tlb) {
    /* Should have one method */
    hr = ITypeInfo_GetFuncDesc(pTI, 1, &pFD);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "hr %08x\n", hr);
    hr = ITypeInfo_GetFuncDesc(pTI, 0, &pFD);
    ok(hr == S_OK, "hr %08x\n", hr);
    ok(pFD->memid == 0x60020000, "memid %08x\n", pFD->memid);
    ok(pFD->oVft == 5 * sizeof(void *), "oVft %d\n", pFD->oVft);
    ITypeInfo_ReleaseFuncDesc(pTI, pFD);
}
    ITypeInfo_Release(pTI);

    ITypeLib_Release(pTL);

    return;
}

static void test_CreateTypeLib(void) {
    static const WCHAR stdoleW[] = {'s','t','d','o','l','e','2','.','t','l','b',0};
    static OLECHAR typelibW[] = {'t','y','p','e','l','i','b',0};
    static OLECHAR helpfileW[] = {'C',':','\\','b','o','g','u','s','.','h','l','p',0};
    static OLECHAR interface1W[] = {'i','n','t','e','r','f','a','c','e','1',0};
    static OLECHAR interface2W[] = {'i','n','t','e','r','f','a','c','e','2',0};
    static OLECHAR interface3W[] = {'i','n','t','e','r','f','a','c','e','3',0};
    static OLECHAR dualW[] = {'d','u','a','l',0};
    static OLECHAR coclassW[] = {'c','o','c','l','a','s','s',0};
    static WCHAR defaultW[] = {'d','e','f','a','u','l','t',0x3213,0};
    static WCHAR defaultQW[] = {'d','e','f','a','u','l','t','?',0};
    static OLECHAR func1W[] = {'f','u','n','c','1',0};
    static OLECHAR func2W[] = {'f','u','n','c','2',0};
    static OLECHAR prop1W[] = {'P','r','o','p','1',0};
    static OLECHAR param1W[] = {'p','a','r','a','m','1',0};
    static OLECHAR param2W[] = {'p','a','r','a','m','2',0};
    static OLECHAR asdfW[] = {'A','s','d','f',0};
    static OLECHAR *names1[] = {func1W, param1W, param2W};
    static OLECHAR *names2[] = {func2W, param1W, param2W};
    static OLECHAR *propname[] = {prop1W, param1W};
    static const GUID custguid = {0xbf611abe,0x5b38,0x11df,{0x91,0x5c,0x08,0x02,0x79,0x79,0x94,0x70}};
    static const GUID bogusguid = {0xbf611abe,0x5b38,0x11df,{0x91,0x5c,0x08,0x02,0x79,0x79,0x94,0x71}};

    char filename[MAX_PATH];
    WCHAR filenameW[MAX_PATH];
    ICreateTypeLib2 *createtl;
    ICreateTypeInfo *createti;
    ICreateTypeInfo2 *createti2;
    ITypeLib *tl, *stdole;
    ITypeInfo *interface1, *interface2, *dual, *unknown, *dispatch, *ti;
    ITypeInfo2 *ti2;
    FUNCDESC funcdesc, *pfuncdesc;
    ELEMDESC elemdesc[5], *edesc;
    PARAMDESCEX paramdescex;
    TYPEDESC typedesc1, typedesc2;
    TYPEATTR *typeattr;
    TLIBATTR *libattr;
    HREFTYPE hreftype;
    BSTR name, docstring, helpfile;
    DWORD helpcontext;
    int impltypeflags;
    VARIANT cust_data;
    HRESULT hres;
    TYPEKIND kind;

    trace("CreateTypeLib tests\n");

    hres = LoadTypeLib(stdoleW, &stdole);
    ok(hres == S_OK, "got %08x\n", hres);

    hres = ITypeLib_GetTypeInfoOfGuid(stdole, &IID_IUnknown, &unknown);
    ok(hres == S_OK, "got %08x\n", hres);

    hres = ITypeLib_GetTypeInfoOfGuid(stdole, &IID_IDispatch, &dispatch);
    ok(hres == S_OK, "got %08x\n", hres);

    GetTempFileNameA(".", "tlb", 0, filename);
    MultiByteToWideChar(CP_ACP, 0, filename, -1, filenameW, MAX_PATH);

    hres = CreateTypeLib2(SYS_WIN32, filenameW, &createtl);
    ok(hres == S_OK, "got %08x\n", hres);

    hres = ICreateTypeLib2_QueryInterface(createtl, &IID_ITypeLib, (void**)&tl);
    ok(hres == S_OK, "got %08x\n", hres);

    hres = ITypeLib_GetTypeInfo(tl, 0, NULL);
    ok(hres == E_INVALIDARG, "got 0x%08x\n", hres);

    hres = ITypeLib_GetTypeInfoType(tl, 0, &kind);
    ok(hres == TYPE_E_ELEMENTNOTFOUND, "got 0x%08x\n", hres);

    hres = ITypeLib_GetTypeInfoType(tl, 0, NULL);
    ok(hres == E_INVALIDARG, "got 0x%08x\n", hres);

    hres = ITypeLib_GetTypeInfoType(tl, 0, NULL);
    ok(hres == E_INVALIDARG, "got 0x%08x\n", hres);

    hres = ITypeLib_GetLibAttr(tl, NULL);
    ok(hres == E_INVALIDARG, "got %08x\n", hres);

    hres = ITypeLib_GetLibAttr(tl, &libattr);
    ok(hres == S_OK, "got %08x\n", hres);

    ok(libattr->syskind == SYS_WIN32, "syskind = %d\n", libattr->syskind);
    ok(libattr->wMajorVerNum == 0, "wMajorVer = %d\n", libattr->wMajorVerNum);
    ok(libattr->wMinorVerNum == 0, "wMinorVerNum = %d\n", libattr->wMinorVerNum);
    ok(libattr->wLibFlags == 0, "wLibFlags = %d\n", libattr->wLibFlags);

    ITypeLib_ReleaseTLibAttr(tl, libattr);

    name = (BSTR)0xdeadbeef;
    hres = ITypeLib_GetDocumentation(tl, -1, &name, &docstring, &helpcontext, &helpfile);
    ok(hres == S_OK, "got %08x\n", hres);
    ok(name == NULL, "name != NULL\n");
    ok(docstring == NULL, "docstring != NULL\n");
    ok(helpcontext == 0, "helpcontext != 0\n");
    ok(helpfile == NULL, "helpfile != NULL\n");

    hres = ITypeLib_GetDocumentation(tl, 0, &name, NULL, NULL, NULL);
    ok(hres == TYPE_E_ELEMENTNOTFOUND, "got %08x\n", hres);

    hres = ICreateTypeLib2_SetName(createtl, typelibW);
    ok(hres == S_OK, "got %08x\n", hres);

    hres = ICreateTypeLib2_SetHelpFileName(createtl, helpfileW);
    ok(hres == S_OK, "got %08x\n", hres);

    hres = ITypeLib_GetDocumentation(tl, -1, NULL, NULL, NULL, NULL);
    ok(hres == S_OK, "got %08x\n", hres);

    hres = ITypeLib_GetDocumentation(tl, -1, &name, NULL, NULL, &helpfile);
    ok(hres == S_OK, "got %08x\n", hres);
    ok(!memcmp(name, typelibW, sizeof(typelibW)), "name = %s\n", wine_dbgstr_w(name));
    ok(!memcmp(helpfile, helpfileW, sizeof(helpfileW)), "helpfile = %s\n", wine_dbgstr_w(helpfile));

    SysFreeString(name);
    SysFreeString(helpfile);

    /* invalid parameters */
    hres = ICreateTypeLib2_CreateTypeInfo(createtl, NULL, TKIND_INTERFACE, &createti);
    ok(hres == E_INVALIDARG, "got %08x\n", hres);

    hres = ICreateTypeLib2_CreateTypeInfo(createtl, interface1W, TKIND_INTERFACE, NULL);
    ok(hres == E_INVALIDARG, "got %08x\n", hres);

    hres = ICreateTypeLib2_CreateTypeInfo(createtl, NULL, TKIND_INTERFACE, NULL);
    ok(hres == E_INVALIDARG, "got %08x\n", hres);

    hres = ICreateTypeLib2_CreateTypeInfo(createtl, interface1W, TKIND_INTERFACE, &createti);
    ok(hres == S_OK, "got %08x\n", hres);

    hres = ICreateTypeInfo_QueryInterface(createti, &IID_ITypeInfo, (void**)&interface1);
    ok(hres == S_OK, "got %08x\n", hres);

    hres = ITypeLib_GetDocumentation(tl, 0, &name, NULL, NULL, NULL);
    ok(hres == S_OK, "got %08x\n", hres);
    ok(!memcmp(name, interface1W, sizeof(interface1W)), "name = %s\n", wine_dbgstr_w(name));

    SysFreeString(name);

    ITypeLib_Release(tl);

    name = (BSTR)0xdeadbeef;
    helpfile = (BSTR)0xdeadbeef;
    hres = ITypeInfo_GetDocumentation(interface1, -1, &name, &docstring, &helpcontext, &helpfile);
    ok(hres == S_OK, "got %08x\n", hres);
    ok(!memcmp(name, interface1W, sizeof(interface1W)), "name = %s\n", wine_dbgstr_w(name));
    ok(docstring == NULL, "docstring != NULL\n");
    ok(helpcontext == 0, "helpcontext != 0\n");
    ok(!memcmp(helpfile, helpfileW, sizeof(helpfileW)), "helpfile = %s\n", wine_dbgstr_w(helpfile));

    SysFreeString(name);
    SysFreeString(helpfile);

    hres = ITypeInfo_GetDocumentation(interface1, 0, &name, NULL, NULL, NULL);
    ok(hres == TYPE_E_ELEMENTNOTFOUND, "got %08x\n", hres);

    hres = ITypeInfo_GetRefTypeInfo(interface1, 0, NULL);
    ok(hres == E_INVALIDARG, "got %08x\n", hres);


    hres = ICreateTypeInfo_LayOut(createti);
    ok(hres == S_OK, "got %08x\n", hres);

    hres = ICreateTypeInfo_AddRefTypeInfo(createti, NULL, &hreftype);
    ok(hres == E_INVALIDARG, "got %08x\n", hres);

    hres = ICreateTypeInfo_AddRefTypeInfo(createti, unknown, NULL);
    ok(hres == E_INVALIDARG, "got %08x\n", hres);

    hres = ICreateTypeInfo_AddRefTypeInfo(createti, unknown, &hreftype);
    ok(hres == S_OK, "got %08x\n", hres);
    if(hres != S_OK) {
        skip("Skipping some tests\n");
        return;
    }

    hres = ICreateTypeInfo_AddImplType(createti, 1, hreftype);
    ok(hres == TYPE_E_ELEMENTNOTFOUND, "got %08x\n", hres);

    hres = ICreateTypeInfo_AddImplType(createti, 0, hreftype);
    ok(hres == S_OK, "got %08x\n", hres);

    hres = ITypeInfo_GetRefTypeOfImplType(interface1, 0, &hreftype);
    ok(hres == S_OK, "got %08x\n", hres);
    ok(hreftype == 3, "hreftype = %d\n", hreftype);

    hres = ITypeInfo_GetRefTypeOfImplType(interface1, -1, &hreftype);
    ok(hres == TYPE_E_ELEMENTNOTFOUND, "got %08x\n", hres);

    ICreateTypeInfo_QueryInterface(createti, &IID_ITypeInfo2, (void**)&ti2);

    memset(&funcdesc, 0, sizeof(FUNCDESC));
    funcdesc.funckind = FUNC_PUREVIRTUAL;
    funcdesc.invkind = INVOKE_PROPERTYGET;
    funcdesc.callconv = CC_STDCALL;
    funcdesc.elemdescFunc.tdesc.vt = VT_BSTR;
    U(funcdesc.elemdescFunc).idldesc.wIDLFlags = IDLFLAG_NONE;

    hres = ICreateTypeInfo_AddFuncDesc(createti, 0, NULL);
    ok(hres == E_INVALIDARG, "got %08x\n", hres);

    hres = ICreateTypeInfo_AddFuncDesc(createti, 1, &funcdesc);
    ok(hres == TYPE_E_ELEMENTNOTFOUND, "got %08x\n", hres);

    hres = ICreateTypeInfo_AddFuncDesc(createti, 0, &funcdesc);
    ok(hres == S_OK, "got %08x\n", hres);

    hres = ITypeInfo2_GetFuncDesc(ti2, 0, NULL);
    ok(hres == E_INVALIDARG, "got %08x\n", hres);

    hres = ITypeInfo2_GetFuncDesc(ti2, 1, &pfuncdesc);
    ok(hres == TYPE_E_ELEMENTNOTFOUND, "got %08x\n", hres);

    hres = ITypeInfo2_GetFuncDesc(ti2, 0, &pfuncdesc);
    ok(hres == S_OK, "got %08x\n", hres);

    ok(pfuncdesc->memid == 0, "got %x\n", pfuncdesc->memid);
    ok(pfuncdesc->lprgscode == NULL, "got %p\n", pfuncdesc->lprgscode);
    ok(pfuncdesc->lprgelemdescParam == NULL, "got %p\n", pfuncdesc->lprgelemdescParam);
    ok(pfuncdesc->funckind == FUNC_PUREVIRTUAL, "got 0x%x\n", pfuncdesc->funckind);
    ok(pfuncdesc->invkind == INVOKE_PROPERTYGET, "got 0x%x\n", pfuncdesc->invkind);
    ok(pfuncdesc->callconv == CC_STDCALL, "got 0x%x\n", pfuncdesc->callconv);
    ok(pfuncdesc->cParams == 0, "got %d\n", pfuncdesc->cParams);
    ok(pfuncdesc->cParamsOpt == 0, "got %d\n", pfuncdesc->cParamsOpt);
    todo_wine ok(pfuncdesc->oVft == 12 ||
            broken(pfuncdesc->oVft == 24) /* xp64 */,
            "got %d\n", pfuncdesc->oVft);
    ok(pfuncdesc->cScodes == 0, "got %d\n", pfuncdesc->cScodes);
    ok(pfuncdesc->elemdescFunc.tdesc.vt == VT_BSTR, "got %d\n", pfuncdesc->elemdescFunc.tdesc.vt);
    ok(pfuncdesc->wFuncFlags == 0, "got 0x%x\n", pfuncdesc->wFuncFlags);

    ITypeInfo2_ReleaseFuncDesc(ti2, pfuncdesc);

    hres = ICreateTypeInfo_SetFuncHelpContext(createti, 0, 0xabcdefab);
    ok(hres == S_OK, "got %08x\n", hres);

    funcdesc.invkind = INVOKE_PROPERTYPUT;
    hres = ICreateTypeInfo_AddFuncDesc(createti, 1, &funcdesc);
    ok(hres == TYPE_E_INCONSISTENTPROPFUNCS, "got %08x\n", hres);

    funcdesc.invkind = INVOKE_PROPERTYPUTREF;
    hres = ICreateTypeInfo_AddFuncDesc(createti, 1, &funcdesc);
    ok(hres == TYPE_E_INCONSISTENTPROPFUNCS, "got %08x\n", hres);

    elemdesc[0].tdesc.vt = VT_BSTR;
    U(elemdesc[0]).idldesc.dwReserved = 0;
    U(elemdesc[0]).idldesc.wIDLFlags = IDLFLAG_FIN;

    funcdesc.lprgelemdescParam = elemdesc;
    funcdesc.invkind = INVOKE_PROPERTYPUT;
    funcdesc.cParams = 1;
    funcdesc.elemdescFunc.tdesc.vt = VT_VOID;

    hres = ICreateTypeInfo_AddFuncDesc(createti, 1, &funcdesc);
    ok(hres == S_OK, "got %08x\n", hres);

    hres = ICreateTypeInfo_SetFuncHelpContext(createti, 1, 0xabcdefab);
    ok(hres == S_OK, "got %08x\n", hres);

    hres = ICreateTypeInfo_SetFuncAndParamNames(createti, 0, propname, 0);
    ok(hres == TYPE_E_ELEMENTNOTFOUND, "got %08x\n", hres);

    hres = ICreateTypeInfo_SetFuncAndParamNames(createti, 0, NULL, 1);
    ok(hres == E_INVALIDARG, "got %08x\n", hres);

    hres = ICreateTypeInfo_SetFuncAndParamNames(createti, 0, propname, 1);
    ok(hres == S_OK, "got %08x\n", hres);

    hres = ICreateTypeInfo_SetFuncAndParamNames(createti, 1, propname, 1);
    ok(hres == S_OK, "got %08x\n", hres);

    hres = ICreateTypeInfo_SetFuncAndParamNames(createti, 1, propname, 2);
    ok(hres == TYPE_E_ELEMENTNOTFOUND, "got %08x\n", hres);

    hres = ITypeInfo2_GetFuncDesc(ti2, 1, &pfuncdesc);
    ok(hres == S_OK, "got %08x\n", hres);

    ok(pfuncdesc->memid == 0, "got %x\n", pfuncdesc->memid);
    ok(pfuncdesc->lprgscode == NULL, "got %p\n", pfuncdesc->lprgscode);
    ok(pfuncdesc->lprgelemdescParam != NULL, "got %p\n", pfuncdesc->lprgelemdescParam);
    ok(pfuncdesc->funckind == FUNC_PUREVIRTUAL, "got 0x%x\n", pfuncdesc->funckind);
    ok(pfuncdesc->invkind == INVOKE_PROPERTYPUT, "got 0x%x\n", pfuncdesc->invkind);
    ok(pfuncdesc->callconv == CC_STDCALL, "got 0x%x\n", pfuncdesc->callconv);
    ok(pfuncdesc->cParams == 1, "got %d\n", pfuncdesc->cParams);
    ok(pfuncdesc->cParamsOpt == 0, "got %d\n", pfuncdesc->cParamsOpt);
    todo_wine ok(pfuncdesc->oVft == 16 ||
            broken(pfuncdesc->oVft == 28) /* xp64 */,
            "got %d\n", pfuncdesc->oVft);
    ok(pfuncdesc->cScodes == 0, "got %d\n", pfuncdesc->cScodes);
    ok(pfuncdesc->elemdescFunc.tdesc.vt == VT_VOID, "got %d\n", pfuncdesc->elemdescFunc.tdesc.vt);
    ok(pfuncdesc->wFuncFlags == 0, "got 0x%x\n", pfuncdesc->wFuncFlags);

    edesc = pfuncdesc->lprgelemdescParam;
    ok(edesc->tdesc.vt == VT_BSTR, "got: %d\n", edesc->tdesc.vt);
    ok(U(*edesc).idldesc.wIDLFlags == IDLFLAG_FIN, "got: %x\n", U(*edesc).idldesc.wIDLFlags);

    ITypeInfo2_ReleaseFuncDesc(ti2, pfuncdesc);


    funcdesc.invkind = INVOKE_PROPERTYPUTREF;
    hres = ICreateTypeInfo_AddFuncDesc(createti, 0, &funcdesc);
    ok(hres == S_OK, "got %08x\n", hres);

    hres = ICreateTypeInfo_SetFuncHelpContext(createti, 0, 0xabcdefab);
    ok(hres == S_OK, "got %08x\n", hres);

    hres = ICreateTypeInfo_SetFuncHelpContext(createti, 0, 0x201);
    ok(hres == S_OK, "got %08x\n", hres);

    funcdesc.memid = 1;
    funcdesc.lprgelemdescParam = NULL;
    funcdesc.invkind = INVOKE_FUNC;
    funcdesc.cParams = 0;
    hres = ICreateTypeInfo_AddFuncDesc(createti, 1, &funcdesc);
    ok(hres == S_OK, "got %08x\n", hres);

    hres = ITypeInfo2_GetFuncDesc(ti2, 1, &pfuncdesc);
    ok(hres == S_OK, "got %08x\n", hres);

    ok(pfuncdesc->memid == 1, "got %d\n", pfuncdesc->memid);
    ok(pfuncdesc->lprgscode == NULL, "got %p\n", pfuncdesc->lprgscode);
    ok(pfuncdesc->lprgelemdescParam == NULL, "got %p\n", pfuncdesc->lprgelemdescParam);
    ok(pfuncdesc->funckind == FUNC_PUREVIRTUAL, "got 0x%x\n", pfuncdesc->funckind);
    ok(pfuncdesc->invkind == INVOKE_FUNC, "got 0x%x\n", pfuncdesc->invkind);
    ok(pfuncdesc->callconv == CC_STDCALL, "got 0x%x\n", pfuncdesc->callconv);
    ok(pfuncdesc->cParams == 0, "got %d\n", pfuncdesc->cParams);
    ok(pfuncdesc->cParamsOpt == 0, "got %d\n", pfuncdesc->cParamsOpt);
    todo_wine ok(pfuncdesc->oVft == 16 ||
            broken(pfuncdesc->oVft == 28), /* xp64 */
            "got %d\n", pfuncdesc->oVft);
    ok(pfuncdesc->cScodes == 0, "got %d\n", pfuncdesc->cScodes);
    ok(pfuncdesc->elemdescFunc.tdesc.vt == VT_VOID, "got %d\n", pfuncdesc->elemdescFunc.tdesc.vt);
    ok(pfuncdesc->wFuncFlags == 0, "got 0x%x\n", pfuncdesc->wFuncFlags);

    ITypeInfo2_ReleaseFuncDesc(ti2, pfuncdesc);

    funcdesc.memid = MEMBERID_NIL;
    hres = ICreateTypeInfo_AddFuncDesc(createti, 1, &funcdesc);
    ok(hres == S_OK, "got %08x\n", hres);

    elemdesc[0].tdesc.vt = VT_PTR;
    U(elemdesc[0].tdesc).lptdesc = &typedesc1;
    typedesc1.vt = VT_BSTR;
    funcdesc.cParams = 1;
    funcdesc.lprgelemdescParam = elemdesc;
    hres = ICreateTypeInfo_AddFuncDesc(createti, 4, &funcdesc);
    ok(hres == S_OK, "got %08x\n", hres);

    hres = ITypeInfo2_GetFuncDesc(ti2, 4, &pfuncdesc);
    ok(hres == S_OK, "got %08x\n", hres);

    ok(pfuncdesc->memid == 0x60010004, "got %x\n", pfuncdesc->memid);
    ok(pfuncdesc->lprgscode == NULL, "got %p\n", pfuncdesc->lprgscode);
    ok(pfuncdesc->lprgelemdescParam != NULL, "got %p\n", pfuncdesc->lprgelemdescParam);
    ok(pfuncdesc->funckind == FUNC_PUREVIRTUAL, "got 0x%x\n", pfuncdesc->funckind);
    ok(pfuncdesc->invkind == INVOKE_FUNC, "got 0x%x\n", pfuncdesc->invkind);
    ok(pfuncdesc->callconv == CC_STDCALL, "got 0x%x\n", pfuncdesc->callconv);
    ok(pfuncdesc->cParams == 1, "got %d\n", pfuncdesc->cParams);
    ok(pfuncdesc->cParamsOpt == 0, "got %d\n", pfuncdesc->cParamsOpt);
    todo_wine ok(pfuncdesc->oVft == 28 ||
            broken(pfuncdesc->oVft == 40) /* xp64 */,
            "got %d\n", pfuncdesc->oVft);
    ok(pfuncdesc->cScodes == 0, "got %d\n", pfuncdesc->cScodes);
    ok(pfuncdesc->elemdescFunc.tdesc.vt == VT_VOID, "got %d\n", pfuncdesc->elemdescFunc.tdesc.vt);
    ok(pfuncdesc->wFuncFlags == 0, "got 0x%x\n", pfuncdesc->wFuncFlags);

    edesc = pfuncdesc->lprgelemdescParam;
    ok(edesc->tdesc.vt == VT_PTR, "got: %d\n", edesc->tdesc.vt);
    ok(U(*edesc).paramdesc.wParamFlags == PARAMFLAG_FIN, "got: 0x%x\n", U(*edesc).paramdesc.wParamFlags);
    ok(U(*edesc).paramdesc.pparamdescex == NULL, "got: %p\n", U(*edesc).paramdesc.pparamdescex);
    ok(U(edesc->tdesc).lptdesc != NULL, "got: %p\n", U(edesc->tdesc).lptdesc);
    ok(U(edesc->tdesc).lptdesc->vt == VT_BSTR, "got: %d\n", U(edesc->tdesc).lptdesc->vt);

    ITypeInfo2_ReleaseFuncDesc(ti2, pfuncdesc);

    U(elemdesc[0].tdesc).lptdesc = &typedesc2;
    typedesc2.vt = VT_PTR;
    U(typedesc2).lptdesc = &typedesc1;
    hres = ICreateTypeInfo_AddFuncDesc(createti, 4, &funcdesc);
    ok(hres == S_OK, "got %08x\n", hres);

    hres = ITypeInfo2_GetFuncDesc(ti2, 4, &pfuncdesc);
    ok(hres == S_OK, "got %08x\n", hres);

    ok(pfuncdesc->memid == 0x60010007, "got %x\n", pfuncdesc->memid);
    ok(pfuncdesc->lprgscode == NULL, "got %p\n", pfuncdesc->lprgscode);
    ok(pfuncdesc->lprgelemdescParam != NULL, "got %p\n", pfuncdesc->lprgelemdescParam);
    ok(pfuncdesc->funckind == FUNC_PUREVIRTUAL, "got 0x%x\n", pfuncdesc->funckind);
    ok(pfuncdesc->invkind == INVOKE_FUNC, "got 0x%x\n", pfuncdesc->invkind);
    ok(pfuncdesc->callconv == CC_STDCALL, "got 0x%x\n", pfuncdesc->callconv);
    ok(pfuncdesc->cParams == 1, "got %d\n", pfuncdesc->cParams);
    ok(pfuncdesc->cParamsOpt == 0, "got %d\n", pfuncdesc->cParamsOpt);
    todo_wine ok(pfuncdesc->oVft == 28 ||
            broken(pfuncdesc->oVft == 40) /* xp64 */,
            "got %d\n", pfuncdesc->oVft);
    ok(pfuncdesc->cScodes == 0, "got %d\n", pfuncdesc->cScodes);
    ok(pfuncdesc->elemdescFunc.tdesc.vt == VT_VOID, "got %d\n", pfuncdesc->elemdescFunc.tdesc.vt);
    ok(pfuncdesc->wFuncFlags == 0, "got 0x%x\n", pfuncdesc->wFuncFlags);

    edesc = pfuncdesc->lprgelemdescParam;
    ok(edesc->tdesc.vt == VT_PTR, "got: %d\n", edesc->tdesc.vt);
    ok(U(*edesc).paramdesc.wParamFlags == PARAMFLAG_FIN, "got: 0x%x\n", U(*edesc).paramdesc.wParamFlags);
    ok(U(*edesc).paramdesc.pparamdescex == NULL, "got: %p\n", U(*edesc).paramdesc.pparamdescex);
    ok(U(edesc->tdesc).lptdesc != NULL, "got: %p\n", U(edesc->tdesc).lptdesc);
    ok(U(edesc->tdesc).lptdesc->vt == VT_PTR, "got: %d\n", U(edesc->tdesc).lptdesc->vt);
    ok(U(*U(edesc->tdesc).lptdesc).lptdesc != NULL, "got: %p\n", U(*U(edesc->tdesc).lptdesc).lptdesc);
    ok(U(*U(edesc->tdesc).lptdesc).lptdesc->vt == VT_BSTR, "got: %d\n", U(*U(edesc->tdesc).lptdesc).lptdesc->vt);

    ITypeInfo2_ReleaseFuncDesc(ti2, pfuncdesc);

    elemdesc[0].tdesc.vt = VT_INT;
    U(elemdesc[0]).paramdesc.wParamFlags = PARAMFLAG_FHASDEFAULT;
    U(elemdesc[0]).paramdesc.pparamdescex = &paramdescex;
    V_VT(&paramdescex.varDefaultValue) = VT_INT;
    V_INT(&paramdescex.varDefaultValue) = 0x123;
    hres = ICreateTypeInfo_AddFuncDesc(createti, 3, &funcdesc);
    ok(hres == S_OK, "got %08x\n", hres);

    hres = ITypeInfo2_GetFuncDesc(ti2, 3, &pfuncdesc);
    ok(hres == S_OK, "got %08x\n", hres);

    ok(pfuncdesc->memid == 0x60010003, "got %x\n", pfuncdesc->memid);
    ok(pfuncdesc->lprgscode == NULL, "got %p\n", pfuncdesc->lprgscode);
    ok(pfuncdesc->lprgelemdescParam != NULL, "got %p\n", pfuncdesc->lprgelemdescParam);
    ok(pfuncdesc->funckind == FUNC_PUREVIRTUAL, "got 0x%x\n", pfuncdesc->funckind);
    ok(pfuncdesc->invkind == INVOKE_FUNC, "got 0x%x\n", pfuncdesc->invkind);
    ok(pfuncdesc->callconv == CC_STDCALL, "got 0x%x\n", pfuncdesc->callconv);
    ok(pfuncdesc->cParams == 1, "got %d\n", pfuncdesc->cParams);
    ok(pfuncdesc->cParamsOpt == 0, "got %d\n", pfuncdesc->cParamsOpt);
    todo_wine ok(pfuncdesc->oVft == 24 ||
            broken(pfuncdesc->oVft == 36) /* xp64 */,
            "got %d\n", pfuncdesc->oVft);
    ok(pfuncdesc->cScodes == 0, "got %d\n", pfuncdesc->cScodes);
    ok(pfuncdesc->elemdescFunc.tdesc.vt == VT_VOID, "got %d\n", pfuncdesc->elemdescFunc.tdesc.vt);
    ok(pfuncdesc->wFuncFlags == 0, "got 0x%x\n", pfuncdesc->wFuncFlags);

    edesc = pfuncdesc->lprgelemdescParam;
    ok(edesc->tdesc.vt == VT_INT, "got: %d\n", edesc->tdesc.vt);
    ok(U(*edesc).paramdesc.wParamFlags == PARAMFLAG_FHASDEFAULT, "got: 0x%x\n", U(*edesc).paramdesc.wParamFlags);
    ok(U(*edesc).paramdesc.pparamdescex != NULL, "got: %p\n", U(*edesc).paramdesc.pparamdescex);
    ok(U(*edesc).paramdesc.pparamdescex->cBytes == sizeof(PARAMDESCEX), "got: %d\n",
            U(*edesc).paramdesc.pparamdescex->cBytes);
    ok(V_VT(&U(*edesc).paramdesc.pparamdescex->varDefaultValue) == VT_I4, "got: %d\n",
            V_VT(&U(*edesc).paramdesc.pparamdescex->varDefaultValue));
    ok(V_I4(&U(*edesc).paramdesc.pparamdescex->varDefaultValue) == 0x123, "got: 0x%x\n",
            V_I4(&U(*edesc).paramdesc.pparamdescex->varDefaultValue));

    ITypeInfo2_ReleaseFuncDesc(ti2, pfuncdesc);

    U(elemdesc[0]).idldesc.dwReserved = 0;
    U(elemdesc[0]).idldesc.wIDLFlags = IDLFLAG_FIN;
    elemdesc[1].tdesc.vt = VT_UI2;
    U(elemdesc[1]).paramdesc.wParamFlags = PARAMFLAG_FHASDEFAULT;
    U(elemdesc[1]).paramdesc.pparamdescex = &paramdescex;
    V_VT(&paramdescex.varDefaultValue) = VT_UI2;
    V_UI2(&paramdescex.varDefaultValue) = 0xffff;
    funcdesc.cParams = 2;
    hres = ICreateTypeInfo_AddFuncDesc(createti, 3, &funcdesc);
    ok(hres == S_OK, "got %08x\n", hres);

    hres = ITypeInfo2_GetFuncDesc(ti2, 3, &pfuncdesc);
    ok(hres == S_OK, "got %08x\n", hres);

    ok(pfuncdesc->memid == 0x60010009, "got %x\n", pfuncdesc->memid);
    ok(pfuncdesc->lprgscode == NULL, "got %p\n", pfuncdesc->lprgscode);
    ok(pfuncdesc->lprgelemdescParam != NULL, "got %p\n", pfuncdesc->lprgelemdescParam);
    ok(pfuncdesc->funckind == FUNC_PUREVIRTUAL, "got 0x%x\n", pfuncdesc->funckind);
    ok(pfuncdesc->invkind == INVOKE_FUNC, "got 0x%x\n", pfuncdesc->invkind);
    ok(pfuncdesc->callconv == CC_STDCALL, "got 0x%x\n", pfuncdesc->callconv);
    ok(pfuncdesc->cParams == 2, "got %d\n", pfuncdesc->cParams);
    ok(pfuncdesc->cParamsOpt == 0, "got %d\n", pfuncdesc->cParamsOpt);
    todo_wine ok(pfuncdesc->oVft == 24 ||
            broken(pfuncdesc->oVft == 36) /* xp64 */,
            "got %d\n", pfuncdesc->oVft);
    ok(pfuncdesc->cScodes == 0, "got %d\n", pfuncdesc->cScodes);
    ok(pfuncdesc->elemdescFunc.tdesc.vt == VT_VOID, "got %d\n", pfuncdesc->elemdescFunc.tdesc.vt);
    ok(pfuncdesc->wFuncFlags == 0, "got 0x%x\n", pfuncdesc->wFuncFlags);

    edesc = pfuncdesc->lprgelemdescParam;
    ok(edesc->tdesc.vt == VT_INT, "got: %d\n", edesc->tdesc.vt);
    ok(U(*edesc).paramdesc.wParamFlags == PARAMFLAG_FIN, "got: 0x%x\n", U(*edesc).paramdesc.wParamFlags);
    ok(U(*edesc).paramdesc.pparamdescex == NULL, "got: %p\n", U(*edesc).paramdesc.pparamdescex);

    edesc = pfuncdesc->lprgelemdescParam + 1;
    ok(edesc->tdesc.vt == VT_UI2, "got: %d\n", edesc->tdesc.vt);
    ok(U(*edesc).paramdesc.wParamFlags == PARAMFLAG_FHASDEFAULT, "got: 0x%x\n", U(*edesc).paramdesc.wParamFlags);
    ok(U(*edesc).paramdesc.pparamdescex != NULL, "got: %p\n", U(*edesc).paramdesc.pparamdescex);
    ok(U(*edesc).paramdesc.pparamdescex->cBytes == sizeof(PARAMDESCEX), "got: %d\n",
            U(*edesc).paramdesc.pparamdescex->cBytes);
    ok(V_VT(&U(*edesc).paramdesc.pparamdescex->varDefaultValue) == VT_UI2, "got: %d\n",
            V_VT(&U(*edesc).paramdesc.pparamdescex->varDefaultValue));
    ok(V_UI2(&U(*edesc).paramdesc.pparamdescex->varDefaultValue) == 0xFFFF, "got: 0x%x\n",
            V_UI2(&U(*edesc).paramdesc.pparamdescex->varDefaultValue));

    ITypeInfo2_ReleaseFuncDesc(ti2, pfuncdesc);

    U(elemdesc[0]).paramdesc.wParamFlags = PARAMFLAG_FHASDEFAULT;
    U(elemdesc[0]).paramdesc.pparamdescex = &paramdescex;
    elemdesc[1].tdesc.vt = VT_INT;
    V_VT(&paramdescex.varDefaultValue) = VT_INT;
    V_INT(&paramdescex.varDefaultValue) = 0xffffffff;
    hres = ICreateTypeInfo_AddFuncDesc(createti, 3, &funcdesc);
    ok(hres == S_OK, "got %08x\n", hres);

    elemdesc[0].tdesc.vt = VT_BSTR;
    elemdesc[1].tdesc.vt = VT_BSTR;
    V_VT(&paramdescex.varDefaultValue) = VT_BSTR;
    V_BSTR(&paramdescex.varDefaultValue) = SysAllocString(defaultW);
    hres = ICreateTypeInfo_AddFuncDesc(createti, 3, &funcdesc);
    ok(hres == S_OK, "got %08x\n", hres);
    SysFreeString(V_BSTR(&paramdescex.varDefaultValue));

    hres = ITypeInfo2_GetFuncDesc(ti2, 3, &pfuncdesc);
    ok(hres == S_OK, "got %08x\n", hres);

    ok(pfuncdesc->memid == 0x6001000b, "got %x\n", pfuncdesc->memid);
    ok(pfuncdesc->lprgscode == NULL, "got %p\n", pfuncdesc->lprgscode);
    ok(pfuncdesc->lprgelemdescParam != NULL, "got %p\n", pfuncdesc->lprgelemdescParam);
    ok(pfuncdesc->funckind == FUNC_PUREVIRTUAL, "got 0x%x\n", pfuncdesc->funckind);
    ok(pfuncdesc->invkind == INVOKE_FUNC, "got 0x%x\n", pfuncdesc->invkind);
    ok(pfuncdesc->callconv == CC_STDCALL, "got 0x%x\n", pfuncdesc->callconv);
    ok(pfuncdesc->cParams == 2, "got %d\n", pfuncdesc->cParams);
    ok(pfuncdesc->cParamsOpt == 0, "got %d\n", pfuncdesc->cParamsOpt);
    todo_wine ok(pfuncdesc->oVft == 24 ||
            broken(pfuncdesc->oVft == 36) /* xp64 */,
            "got %d\n", pfuncdesc->oVft);
    ok(pfuncdesc->cScodes == 0, "got %d\n", pfuncdesc->cScodes);
    ok(pfuncdesc->elemdescFunc.tdesc.vt == VT_VOID, "got %d\n", pfuncdesc->elemdescFunc.tdesc.vt);
    ok(pfuncdesc->wFuncFlags == 0, "got 0x%x\n", pfuncdesc->wFuncFlags);

    edesc = pfuncdesc->lprgelemdescParam;
    ok(edesc->tdesc.vt == VT_BSTR, "got: %d\n", edesc->tdesc.vt);
    ok(U(*edesc).paramdesc.wParamFlags == PARAMFLAG_FHASDEFAULT, "got: 0x%x\n", U(*edesc).paramdesc.wParamFlags);
    ok(U(*edesc).paramdesc.pparamdescex != NULL, "got: %p\n", U(*edesc).paramdesc.pparamdescex);
    ok(U(*edesc).paramdesc.pparamdescex->cBytes == sizeof(PARAMDESCEX), "got: %d\n",
            U(*edesc).paramdesc.pparamdescex->cBytes);
    ok(V_VT(&U(*edesc).paramdesc.pparamdescex->varDefaultValue) == VT_BSTR, "got: %d\n",
            V_VT(&U(*edesc).paramdesc.pparamdescex->varDefaultValue));
    ok(!lstrcmpW(V_BSTR(&U(*edesc).paramdesc.pparamdescex->varDefaultValue), defaultQW),
            "got: %s\n",
            wine_dbgstr_w(V_BSTR(&U(*edesc).paramdesc.pparamdescex->varDefaultValue)));

    edesc = pfuncdesc->lprgelemdescParam + 1;
    ok(edesc->tdesc.vt == VT_BSTR, "got: %d\n", edesc->tdesc.vt);
    ok(U(*edesc).paramdesc.wParamFlags == PARAMFLAG_FHASDEFAULT, "got: 0x%x\n", U(*edesc).paramdesc.wParamFlags);
    ok(U(*edesc).paramdesc.pparamdescex != NULL, "got: %p\n", U(*edesc).paramdesc.pparamdescex);
    ok(U(*edesc).paramdesc.pparamdescex->cBytes == sizeof(PARAMDESCEX), "got: %d\n",
            U(*edesc).paramdesc.pparamdescex->cBytes);
    ok(V_VT(&U(*edesc).paramdesc.pparamdescex->varDefaultValue) == VT_BSTR, "got: %d\n",
            V_VT(&U(*edesc).paramdesc.pparamdescex->varDefaultValue));
    ok(!lstrcmpW(V_BSTR(&U(*edesc).paramdesc.pparamdescex->varDefaultValue), defaultQW),
            "got: %s\n",
            wine_dbgstr_w(V_BSTR(&U(*edesc).paramdesc.pparamdescex->varDefaultValue)));

    ITypeInfo2_ReleaseFuncDesc(ti2, pfuncdesc);

    elemdesc[0].tdesc.vt = VT_USERDEFINED;
    U(elemdesc[0].tdesc).hreftype = hreftype;
    U(elemdesc[0]).paramdesc.pparamdescex = NULL;
    U(elemdesc[0]).paramdesc.wParamFlags = 0;

    funcdesc.lprgelemdescParam = elemdesc;
    funcdesc.invkind = INVOKE_FUNC;
    funcdesc.cParams = 1;
    funcdesc.elemdescFunc.tdesc.vt = VT_VOID;

    hres = ICreateTypeInfo_AddFuncDesc(createti, 5, &funcdesc);
    ok(hres == S_OK, "got %08x\n", hres);

    hres = ITypeInfo2_GetFuncDesc(ti2, 5, &pfuncdesc);
    ok(hres == S_OK, "got %08x\n", hres);

    ok(pfuncdesc->memid == 0x60010005, "got %x\n", pfuncdesc->memid);
    ok(pfuncdesc->lprgscode == NULL, "got %p\n", pfuncdesc->lprgscode);
    ok(pfuncdesc->lprgelemdescParam != NULL, "got %p\n", pfuncdesc->lprgelemdescParam);
    ok(pfuncdesc->funckind == FUNC_PUREVIRTUAL, "got 0x%x\n", pfuncdesc->funckind);
    ok(pfuncdesc->invkind == INVOKE_FUNC, "got 0x%x\n", pfuncdesc->invkind);
    ok(pfuncdesc->callconv == CC_STDCALL, "got 0x%x\n", pfuncdesc->callconv);
    ok(pfuncdesc->cParams == 1, "got %d\n", pfuncdesc->cParams);
    ok(pfuncdesc->cParamsOpt == 0, "got %d\n", pfuncdesc->cParamsOpt);
    todo_wine ok(pfuncdesc->oVft == 32 ||
            broken(pfuncdesc->oVft == 44), /* xp64 */
            "got %d\n", pfuncdesc->oVft);
    ok(pfuncdesc->cScodes == 0, "got %d\n", pfuncdesc->cScodes);
    ok(pfuncdesc->elemdescFunc.tdesc.vt == VT_VOID, "got %d\n", pfuncdesc->elemdescFunc.tdesc.vt);
    ok(pfuncdesc->wFuncFlags == 0, "got 0x%x\n", pfuncdesc->wFuncFlags);

    edesc = pfuncdesc->lprgelemdescParam;
    ok(U(*edesc).paramdesc.pparamdescex == NULL, "got: %p\n", U(*edesc).paramdesc.pparamdescex);
    ok(U(*edesc).paramdesc.wParamFlags == 0, "got: 0x%x\n", U(*edesc).paramdesc.wParamFlags);
    ok(edesc->tdesc.vt == VT_USERDEFINED, "got: %d\n", edesc->tdesc.vt);
    ok(U(edesc->tdesc).hreftype == hreftype, "got: 0x%x\n", U(edesc->tdesc).hreftype);

    ITypeInfo2_ReleaseFuncDesc(ti2, pfuncdesc);

    hres = ITypeInfo_GetDocumentation(interface1, 0, &name, &docstring, &helpcontext, &helpfile);
    ok(hres == S_OK, "got %08x\n", hres);
    ok(name == NULL, "name != NULL\n");
    ok(docstring == NULL, "docstring != NULL\n");
    ok(helpcontext == 0x201, "helpcontext != 0x201\n");
    ok(!memcmp(helpfile, helpfileW, sizeof(helpfileW)), "helpfile = %s\n", wine_dbgstr_w(helpfile));

    SysFreeString(helpfile);

    hres = ICreateTypeInfo_SetFuncAndParamNames(createti, 1000, NULL, 1);
    ok(hres == E_INVALIDARG, "got %08x\n", hres);

    hres = ICreateTypeInfo_SetFuncAndParamNames(createti, 1000, names1, 1);
    ok(hres == TYPE_E_ELEMENTNOTFOUND, "got %08x\n", hres);

    hres = ICreateTypeInfo_SetFuncAndParamNames(createti, 0, names1, 2);
    ok(hres == TYPE_E_ELEMENTNOTFOUND, "got %08x\n", hres);

    hres = ICreateTypeInfo_SetFuncAndParamNames(createti, 0, names2, 1);
    ok(hres == S_OK, "got %08x\n", hres);

    hres = ICreateTypeInfo_SetFuncAndParamNames(createti, 0, names1, 1);
    ok(hres == S_OK, "got %08x\n", hres);

    hres = ITypeInfo_GetDocumentation(interface1, 0, &name, NULL, NULL, NULL);
    ok(hres == S_OK, "got %08x\n", hres);
    ok(!memcmp(name, func1W, sizeof(func1W)), "name = %s\n", wine_dbgstr_w(name));

    SysFreeString(name);

    hres = ICreateTypeInfo_SetFuncAndParamNames(createti, 3, names2, 3);
    ok(hres == S_OK, "got %08x\n", hres);

    hres = ICreateTypeInfo_SetFuncAndParamNames(createti, 3, names1, 3);
    ok(hres == TYPE_E_AMBIGUOUSNAME, "got %08x\n", hres);

    ITypeInfo2_Release(ti2);
    ICreateTypeInfo_Release(createti);

    hres = ICreateTypeLib2_CreateTypeInfo(createtl, interface1W, TKIND_INTERFACE, &createti);
    ok(hres == TYPE_E_NAMECONFLICT, "got %08x\n", hres);

    hres = ICreateTypeLib2_CreateTypeInfo(createtl, interface2W, TKIND_INTERFACE, &createti);
    ok(hres == S_OK, "got %08x\n", hres);

    hres = ICreateTypeInfo_QueryInterface(createti, &IID_ITypeInfo, (void**)&interface2);
    ok(hres == S_OK, "got %08x\n", hres);

    hres = ITypeInfo_GetRefTypeOfImplType(interface2, 0, &hreftype);
    ok(hres == TYPE_E_ELEMENTNOTFOUND, "got %08x\n", hres);

    hres = ICreateTypeInfo_AddRefTypeInfo(createti, interface1, &hreftype);
    ok(hres == S_OK, "got %08x\n", hres);

    hres = ITypeInfo_GetRefTypeInfo(interface2, 0, &ti);
    ok(hres == S_OK, "got %08x\n", hres);
    ok(ti == interface1, "Received and added interfaces are different\n");

    ITypeInfo_Release(ti);

    hres = ICreateTypeInfo_AddImplType(createti, 0, hreftype);
    ok(hres == S_OK, "got %08x\n", hres);

    hres = ITypeInfo_GetRefTypeOfImplType(interface2, 0, &hreftype);
    ok(hres == S_OK, "got %08x\n", hres);
    ok(hreftype == 2, "hreftype = %d\n", hreftype);

    hres = ITypeInfo_GetRefTypeOfImplType(interface2, -1, &hreftype);
    ok(hres == TYPE_E_ELEMENTNOTFOUND, "got %08x\n", hres);

    hres = ICreateTypeInfo_SetImplTypeFlags(createti, 0, IMPLTYPEFLAG_FDEFAULT);
    ok(hres == TYPE_E_BADMODULEKIND, "got %08x\n", hres);

    hres = ITypeInfo_GetImplTypeFlags(interface2, 0, &impltypeflags);
    ok(hres == S_OK, "got %08x\n", hres);
    ok(impltypeflags == 0, "impltypeflags = %x\n", impltypeflags);

    hres = ITypeInfo_GetImplTypeFlags(interface2, 1, &impltypeflags);
    ok(hres == TYPE_E_ELEMENTNOTFOUND, "got %08x\n", hres);

    funcdesc.oVft = 0xaaac;
    hres = ICreateTypeInfo_AddFuncDesc(createti, 0, &funcdesc);
    ok(hres == S_OK, "got %08x\n", hres);
    funcdesc.oVft = 0xaaa8;
    hres = ICreateTypeInfo_AddFuncDesc(createti, 0, &funcdesc);
    ok(hres == S_OK, "got %08x\n", hres);
    funcdesc.oVft = 0;

    ICreateTypeInfo_Release(createti);

    VariantInit(&cust_data);

    hres = ICreateTypeLib2_CreateTypeInfo(createtl, interface3W, TKIND_INTERFACE, &createti);
    ok(hres == S_OK, "got %08x\n", hres);

    hres = ICreateTypeInfo_QueryInterface(createti, &IID_ICreateTypeInfo2, (void**)&createti2);
    ok(hres == S_OK, "got %08x\n", hres);

    hres = ICreateTypeInfo2_QueryInterface(createti2, &IID_ITypeInfo2, (void**)&ti2);
    ok(hres == S_OK, "got %08x\n", hres);

    hres = ITypeInfo2_GetCustData(ti2, NULL, NULL);
    ok(hres == E_INVALIDARG, "got %08x\n", hres);

    hres = ITypeInfo2_GetCustData(ti2, &custguid, NULL);
    ok(hres == E_INVALIDARG, "got %08x\n", hres);

    hres = ITypeInfo2_GetCustData(ti2, &custguid, &cust_data);
    ok(hres == S_OK, "got %08x\n", hres);

    hres = ICreateTypeInfo2_SetCustData(createti2, NULL, NULL);
    ok(hres == E_INVALIDARG, "got %08x\n", hres);

    hres = ICreateTypeInfo2_SetCustData(createti2, &custguid, NULL);
    ok(hres == E_INVALIDARG, "got %08x\n", hres);

    hres = ICreateTypeInfo2_SetCustData(createti2, &custguid, &cust_data);
    ok(hres == DISP_E_BADVARTYPE, "got %08x\n", hres);

    V_VT(&cust_data) = VT_UI4;
    V_I4(&cust_data) = 0xdeadbeef;

    hres = ICreateTypeInfo2_SetCustData(createti2, &custguid, &cust_data);
    ok(hres == S_OK, "got %08x\n", hres);

    V_I4(&cust_data) = 0;
    V_VT(&cust_data) = VT_EMPTY;

    hres = ITypeInfo2_GetCustData(ti2, &custguid, &cust_data);
    ok(hres == S_OK, "got %08x\n", hres);

    ok(V_VT(&cust_data) == VT_UI4, "got %d\n", V_VT(&cust_data));
    ok(V_I4(&cust_data) == 0xdeadbeef, "got 0x%08x\n", V_I4(&cust_data));

    V_VT(&cust_data) = VT_UI4;
    V_I4(&cust_data) = 12345678;

    hres = ICreateTypeInfo2_SetCustData(createti2, &custguid, &cust_data);
    ok(hres == S_OK, "got %08x\n", hres);

    V_I4(&cust_data) = 0;
    V_VT(&cust_data) = VT_EMPTY;

    hres = ITypeInfo2_GetCustData(ti2, &custguid, &cust_data);
    ok(hres == S_OK, "got %08x\n", hres);

    ok(V_VT(&cust_data) == VT_UI4, "got %d\n", V_VT(&cust_data));
    ok(V_I4(&cust_data) == 12345678, "got 0x%08x\n", V_I4(&cust_data));

    V_VT(&cust_data) = VT_BSTR;
    V_BSTR(&cust_data) = SysAllocString(asdfW);

    hres = ICreateTypeInfo2_SetCustData(createti2, &custguid, &cust_data);
    ok(hres == S_OK, "got %08x\n", hres);

    SysFreeString(V_BSTR(&cust_data));
    V_I4(&cust_data) = 0;
    V_VT(&cust_data) = VT_EMPTY;

    hres = ITypeInfo2_GetCustData(ti2, &custguid, &cust_data);
    ok(hres == S_OK, "got %08x\n", hres);

    ok(V_VT(&cust_data) == VT_BSTR, "got %d\n", V_VT(&cust_data));
    ok(!lstrcmpW(V_BSTR(&cust_data), asdfW), "got %s\n", wine_dbgstr_w(V_BSTR(&cust_data)));
    SysFreeString(V_BSTR(&cust_data));

    V_VT(&cust_data) = VT_UI4;
    V_UI4(&cust_data) = 17;

    hres = ITypeInfo2_GetCustData(ti2, &bogusguid, &cust_data);
    ok(hres == S_OK, "got %08x\n", hres);

    ok(V_VT(&cust_data) == VT_EMPTY, "got: %d\n", V_VT(&cust_data));

    ITypeInfo2_Release(ti2);
    ICreateTypeInfo2_Release(createti2);
    ICreateTypeInfo_Release(createti);

    hres = ICreateTypeLib2_CreateTypeInfo(createtl, coclassW, TKIND_COCLASS, &createti);
    ok(hres == S_OK, "got %08x\n", hres);

    hres = ICreateTypeInfo_AddRefTypeInfo(createti, interface1, &hreftype);
    ok(hres == S_OK, "got %08x\n", hres);

    hres = ICreateTypeInfo_AddImplType(createti, 0, hreftype);
    ok(hres == S_OK, "got %08x\n", hres);

    hres = ICreateTypeInfo_AddImplType(createti, 0, hreftype);
    ok(hres == TYPE_E_ELEMENTNOTFOUND, "got %08x\n", hres);

    hres = ICreateTypeInfo_AddRefTypeInfo(createti, unknown, &hreftype);
    ok(hres == S_OK, "got %08x\n", hres);

    hres = ICreateTypeInfo_AddImplType(createti, 1, hreftype);
    ok(hres == S_OK, "got %08x\n", hres);

    hres = ICreateTypeInfo_AddImplType(createti, 1, hreftype);
    ok(hres == TYPE_E_ELEMENTNOTFOUND, "got %08x\n", hres);

    hres = ICreateTypeInfo_AddImplType(createti, 2, hreftype);
    ok(hres == S_OK, "got %08x\n", hres);

    hres = ICreateTypeInfo_SetImplTypeFlags(createti, 0, IMPLTYPEFLAG_FDEFAULT);
    ok(hres == S_OK, "got %08x\n", hres);

    hres = ICreateTypeInfo_SetImplTypeFlags(createti, 1, IMPLTYPEFLAG_FRESTRICTED);
    ok(hres == S_OK, "got %08x\n", hres);

    hres = ICreateTypeInfo_QueryInterface(createti, &IID_ITypeInfo, (void**)&ti);
    ok(hres == S_OK, "got %08x\n", hres);

    hres = ITypeInfo_GetImplTypeFlags(ti, 0, NULL);
    ok(hres == E_INVALIDARG, "got %08x\n", hres);

    hres = ITypeInfo_GetImplTypeFlags(ti, 0, &impltypeflags);
    ok(hres == S_OK, "got %08x\n", hres);
    ok(impltypeflags == IMPLTYPEFLAG_FDEFAULT, "impltypeflags = %x\n", impltypeflags);

    hres = ITypeInfo_GetImplTypeFlags(ti, 1, &impltypeflags);
    ok(hres == S_OK, "got %08x\n", hres);
    ok(impltypeflags == IMPLTYPEFLAG_FRESTRICTED, "impltypeflags = %x\n", impltypeflags);

    hres = ITypeInfo_GetImplTypeFlags(ti, 2, &impltypeflags);
    ok(hres == S_OK, "got %08x\n", hres);
    ok(impltypeflags == 0, "impltypeflags = %x\n", impltypeflags);

    hres = ITypeInfo_GetRefTypeOfImplType(ti, 0, &hreftype);
    ok(hres == S_OK, "got %08x\n", hres);
    ok(hreftype == 0, "hreftype = %d\n", hreftype);

    hres = ITypeInfo_GetRefTypeOfImplType(ti, 1, &hreftype);
    ok(hres == S_OK, "got %08x\n", hres);
    ok(hreftype == 1, "hreftype = %d\n", hreftype);

    hres = ITypeInfo_GetRefTypeOfImplType(ti, 2, &hreftype);
    ok(hres == S_OK, "got %08x\n", hres);
    ok(hreftype == 1, "hreftype = %d\n", hreftype);

    hres = ITypeInfo_GetRefTypeOfImplType(ti, -1, &hreftype);
    ok(hres == TYPE_E_ELEMENTNOTFOUND, "got %08x\n", hres);

    ITypeInfo_Release(ti);

    ICreateTypeInfo_Release(createti);

    hres = ICreateTypeLib2_CreateTypeInfo(createtl, dualW, TKIND_INTERFACE, &createti);
    ok(hres == S_OK, "got %08x\n", hres);

    hres = ICreateTypeInfo_SetTypeFlags(createti, TYPEFLAG_FDUAL);
    ok(hres == S_OK, "got %08x\n", hres);

    hres = ICreateTypeInfo_AddFuncDesc(createti, 0, &funcdesc);
    ok(hres == S_OK, "got %08x\n", hres);

    hres = ICreateTypeInfo_AddRefTypeInfo(createti, dispatch, &hreftype);
    ok(hres == S_OK, "got %08x\n", hres);

    hres = ICreateTypeInfo_AddImplType(createti, 0, hreftype);
    ok(hres == S_OK, "got %08x\n", hres);

    hres = ICreateTypeInfo_QueryInterface(createti, &IID_ITypeInfo, (void**)&dual);
    ok(hres == S_OK, "got %08x\n", hres);

    hres = ITypeInfo_GetTypeAttr(dual, &typeattr);
    ok(hres == S_OK, "got %08x\n", hres);
    ok(typeattr->cbSizeInstance == 4, "cbSizeInstance = %d\n", typeattr->cbSizeInstance);
    ok(typeattr->typekind == 3, "typekind = %d\n", typeattr->typekind);
    ok(typeattr->cFuncs == 1, "cFuncs = %d\n", typeattr->cFuncs);
    ok(typeattr->cVars == 0, "cVars = %d\n", typeattr->cVars);
    ok(typeattr->cImplTypes == 1, "cImplTypes = %d\n", typeattr->cImplTypes);
    ok(typeattr->cbSizeVft == 32 || broken(typeattr->cbSizeVft == 7 * sizeof(void *) + 4), /* xp64 */
       "cbSizeVft = %d\n", typeattr->cbSizeVft);
    ok(typeattr->cbAlignment == 4, "cbAlignment = %d\n", typeattr->cbAlignment);
    ok(typeattr->wTypeFlags == (TYPEFLAG_FDISPATCHABLE|TYPEFLAG_FDUAL), "wTypeFlags = %d\n", typeattr->wTypeFlags);
    ok(typeattr->wMajorVerNum == 0, "wMajorVerNum = %d\n", typeattr->wMajorVerNum);
    ok(typeattr->wMinorVerNum == 0, "wMinorVerNum = %d\n", typeattr->wMinorVerNum);

    ITypeInfo_ReleaseTypeAttr(dual, typeattr);

    hres = ITypeInfo_GetRefTypeOfImplType(dual, -1, &hreftype);
    ok(hres == S_OK, "got %08x\n", hres);
    ok(hreftype == -2, "got %08x\n", hreftype);

    hres = ITypeInfo_GetRefTypeInfo(dual, -2, &ti);
    ok(hres == S_OK, "got %08x\n", hres);

    hres = ITypeInfo_GetTypeAttr(ti, &typeattr);
    ok(hres == S_OK, "got %08x\n", hres);
    ok(typeattr->cbSizeInstance == 4, "cbSizeInstance = %d\n", typeattr->cbSizeInstance);
    ok(typeattr->typekind == 4, "typekind = %d\n", typeattr->typekind);
    ok(typeattr->cFuncs == 8, "cFuncs = %d\n", typeattr->cFuncs);
    ok(typeattr->cVars == 0, "cVars = %d\n", typeattr->cVars);
    ok(typeattr->cImplTypes == 1, "cImplTypes = %d\n", typeattr->cImplTypes);
    ok(typeattr->cbSizeVft == 7 * sizeof(void *), "cbSizeVft = %d\n", typeattr->cbSizeVft);
    ok(typeattr->cbAlignment == 4, "cbAlignment = %d\n", typeattr->cbAlignment);
    ok(typeattr->wTypeFlags == (TYPEFLAG_FDISPATCHABLE|TYPEFLAG_FDUAL), "wTypeFlags = %d\n", typeattr->wTypeFlags);
    ok(typeattr->wMajorVerNum == 0, "wMajorVerNum = %d\n", typeattr->wMajorVerNum);
    ok(typeattr->wMinorVerNum == 0, "wMinorVerNum = %d\n", typeattr->wMinorVerNum);

    ITypeInfo_ReleaseTypeAttr(ti, typeattr);

    ITypeInfo_Release(ti);

    ICreateTypeInfo_Release(createti);

    hres = ITypeInfo_GetTypeAttr(interface1, &typeattr);
    ok(hres == S_OK, "got %08x\n", hres);
    ok(typeattr->cbSizeInstance == 4, "cbSizeInstance = %d\n", typeattr->cbSizeInstance);
    ok(typeattr->typekind == 3, "typekind = %d\n", typeattr->typekind);
    ok(typeattr->cFuncs == 12, "cFuncs = %d\n", typeattr->cFuncs);
    ok(typeattr->cVars == 0, "cVars = %d\n", typeattr->cVars);
    ok(typeattr->cImplTypes == 1, "cImplTypes = %d\n", typeattr->cImplTypes);
    ok(typeattr->cbSizeVft == 60 || broken(typeattr->cbSizeVft == 3 * sizeof(void *) + 48), /* xp64 */
       "cbSizeVft = %d\n", typeattr->cbSizeVft);
    ok(typeattr->cbAlignment == 4, "cbAlignment = %d\n", typeattr->cbAlignment);
    ok(typeattr->wTypeFlags == 0, "wTypeFlags = %d\n", typeattr->wTypeFlags);
    ok(typeattr->wMajorVerNum == 0, "wMajorVerNum = %d\n", typeattr->wMajorVerNum);
    ok(typeattr->wMinorVerNum == 0, "wMinorVerNum = %d\n", typeattr->wMinorVerNum);

    ITypeInfo_ReleaseTypeAttr(interface1, typeattr);

    hres = ITypeInfo_GetTypeAttr(interface2, &typeattr);
    ok(hres == S_OK, "got %08x\n", hres);
    ok(typeattr->cbSizeInstance == 4, "cbSizeInstance = %d\n", typeattr->cbSizeInstance);
    ok(typeattr->typekind == 3, "typekind = %d\n", typeattr->typekind);
    ok(typeattr->cFuncs == 2, "cFuncs = %d\n", typeattr->cFuncs);
    ok(typeattr->cVars == 0, "cVars = %d\n", typeattr->cVars);
    ok(typeattr->cImplTypes == 1, "cImplTypes = %d\n", typeattr->cImplTypes);
    ok(typeattr->cbSizeVft == 43696, "cbSizeVft = %d\n", typeattr->cbSizeVft);
    ok(typeattr->cbAlignment == 4, "cbAlignment = %d\n", typeattr->cbAlignment);
    ok(typeattr->wTypeFlags == 0, "wTypeFlags = %d\n", typeattr->wTypeFlags);
    ok(typeattr->wMajorVerNum == 0, "wMajorVerNum = %d\n", typeattr->wMajorVerNum);
    ok(typeattr->wMinorVerNum == 0, "wMinorVerNum = %d\n", typeattr->wMinorVerNum);

    ITypeInfo_ReleaseTypeAttr(interface2, typeattr);

    hres = ICreateTypeLib2_SaveAllChanges(createtl);
    ok(hres == S_OK, "got %08x\n", hres);

    ok(ITypeInfo_Release(interface2)==0, "Object should be freed\n");
    ok(ITypeInfo_Release(interface1)==0, "Object should be freed\n");
    ok(ITypeInfo_Release(dual)==0, "Object should be freed\n");
    ok(ICreateTypeLib2_Release(createtl)==0, "Object should be freed\n");

    ok(ITypeInfo_Release(dispatch)==0, "Object should be freed\n");
    ok(ITypeInfo_Release(unknown)==0, "Object should be freed\n");
    ok(ITypeLib_Release(stdole)==0, "Object should be freed\n");

    hres = LoadTypeLibEx(filenameW, REGKIND_NONE, &tl);
    ok(hres == S_OK, "got %08x\n", hres);
    ok(ITypeLib_Release(tl)==0, "Object should be freed\n");

    DeleteFileA(filename);
}

#if 0       /* use this to generate more tests */

#define OLE_CHECK(x) { HRESULT hr = x; if (FAILED(hr)) { printf(#x "failed - %x\n", hr); return; } }

static char *dump_string(LPWSTR wstr)
{
    int size = lstrlenW(wstr)+3;
    char *out = CoTaskMemAlloc(size);
    WideCharToMultiByte(20127, 0, wstr, -1, out+1, size, NULL, NULL);
    out[0] = '\"';
    strcat(out, "\"");
    return out;
}

struct map_entry
{
    DWORD value;
    const char *name;
};

#define MAP_ENTRY(x) { x, #x }
static const struct map_entry tkind_map[] = {
    MAP_ENTRY(TKIND_ENUM),
    MAP_ENTRY(TKIND_RECORD),
    MAP_ENTRY(TKIND_MODULE),
    MAP_ENTRY(TKIND_INTERFACE),
    MAP_ENTRY(TKIND_DISPATCH),
    MAP_ENTRY(TKIND_COCLASS),
    MAP_ENTRY(TKIND_ALIAS),
    MAP_ENTRY(TKIND_UNION),
    MAP_ENTRY(TKIND_MAX),
    {0, NULL}
};

static const struct map_entry funckind_map[] = {
    MAP_ENTRY(FUNC_VIRTUAL),
    MAP_ENTRY(FUNC_PUREVIRTUAL),
    MAP_ENTRY(FUNC_NONVIRTUAL),
    MAP_ENTRY(FUNC_STATIC),
    MAP_ENTRY(FUNC_DISPATCH),
    {0, NULL}
};

static const struct map_entry invkind_map[] = {
    MAP_ENTRY(INVOKE_FUNC),
    MAP_ENTRY(INVOKE_PROPERTYGET),
    MAP_ENTRY(INVOKE_PROPERTYPUT),
    MAP_ENTRY(INVOKE_PROPERTYPUTREF),
    {0, NULL}
};

#undef MAP_ENTRY

static const char *map_value(DWORD val, const struct map_entry *map)
{
    static int map_id;
    static char bufs[16][256];
    char *buf;

    while (map->name)
    {
        if (map->value == val)
            return map->name;
        map++;
    }

    buf = bufs[(map_id++)%16];
    sprintf(buf, "0x%x", val);
    return buf;
}

static void test_dump_typelib(const char *name)
{
    WCHAR wszString[260];
    ITypeInfo *info;
    ITypeLib *lib;
    int count;
    int i;

    MultiByteToWideChar(CP_ACP, 0, name, -1, wszString, 260);
    OLE_CHECK(LoadTypeLib(wszString, &lib));
    count = ITypeLib_GetTypeInfoCount(lib);
    printf("/* interfaces count: %d */\n", count);
    for (i = 0; i < count; i++)
    {
        TYPEATTR *attr;
        BSTR name;
        int f = 0;

        OLE_CHECK(ITypeLib_GetDocumentation(lib, i, &name, NULL, NULL, NULL));
        printf("{\n"
               "  %s,\n", dump_string(name));
        SysFreeString(name);

        OLE_CHECK(ITypeLib_GetTypeInfo(lib, i, &info));
        ITypeInfo_GetTypeAttr(info, &attr);
        printf("  /*kind*/ %s, /*flags*/ 0x%x, /*align*/ %d, /*size*/ %d,\n"
               "  /*#vtbl*/ %d, /*#func*/ %d,\n"
               "  {\n",
            map_value(attr->typekind, tkind_map), attr->wTypeFlags, attr->cbAlignment, attr->cbSizeInstance, attr->cbSizeVft,
            attr->cFuncs);
        ITypeInfo_ReleaseTypeAttr(info, attr);
        while (1)
        {
            FUNCDESC *desc;
            BSTR tab[256];
            UINT cNames;
            int p;

            if (FAILED(ITypeInfo_GetFuncDesc(info, f, &desc)))
                break;
            printf("    {\n"
                   "      0x%x, /*func*/ %s, /*inv*/ %s, /*call*/ 0x%x,\n",
                desc->memid, map_value(desc->funckind, funckind_map), map_value(desc->invkind, invkind_map),
                desc->callconv);
            printf("      /*#param*/ %d, /*#opt*/ %d, /*vtbl*/ %d, /*#scodes*/ %d, /*flags*/ 0x%x,\n",
                desc->cParams, desc->cParamsOpt, desc->oVft, desc->cScodes, desc->wFuncFlags);
            printf("      {%d, %x}, /* ret */\n", desc->elemdescFunc.tdesc.vt, desc->elemdescFunc.paramdesc.wParamFlags);
            printf("      { /* params */\n");
            for (p = 0; p < desc->cParams; p++)
            {
                ELEMDESC e = desc->lprgelemdescParam[p];
                printf("        {%d, %x},\n", e.tdesc.vt, e.paramdesc.wParamFlags);
            }
            printf("        {-1, -1}\n");
            printf("      },\n");
            printf("      { /* names */\n");
            OLE_CHECK(ITypeInfo_GetNames(info, desc->memid, tab, 256, &cNames));
            for (p = 0; p < cNames; p++)
            {
                printf("        %s,\n", dump_string(tab[p]));
                SysFreeString(tab[p]);
            }
            printf("        NULL,\n");
            printf("      },\n");
            printf("    },\n");
            ITypeInfo_ReleaseFuncDesc(info, desc);
            f++;
        }
        printf("  }\n");
        printf("},\n");
        ITypeInfo_Release(info);
    }
    ITypeLib_Release(lib);
}

#else

typedef struct _element_info
{
    VARTYPE vt;
    USHORT wParamFlags;
} element_info;

typedef struct _function_info
{
    MEMBERID memid;
    FUNCKIND funckind;
    INVOKEKIND invkind;
    CALLCONV callconv;
    short cParams;
    short cParamsOpt;
    short vtbl_index;
    short cScodes;
    WORD wFuncFlags;
    element_info ret_type;
    element_info params[15];
    LPCSTR names[15];
} function_info;

typedef struct _interface_info
{
    LPCSTR name;
    TYPEKIND type;
    WORD wTypeFlags;
    USHORT cbAlignment;
    USHORT cbSizeInstance;
    USHORT cbSizeVft;
    USHORT cFuncs;
    function_info funcs[20];
} interface_info;

static const interface_info info[] = {
/* interfaces count: 2 */
{
  "IDualIface",
  /*kind*/ TKIND_DISPATCH, /*flags*/ 0x1040, /*align*/ 4, /*size*/ 4,
  /*#vtbl*/ 7, /*#func*/ 8,
  {
    {
      0x60000000, /*func*/ FUNC_DISPATCH, /*inv*/ INVOKE_FUNC, /*call*/ 0x4,
      /*#param*/ 2, /*#opt*/ 0, /*vtbl*/ 0, /*#scodes*/ 0, /*flags*/ 0x1,
      {24, 0}, /* ret */
      { /* params */
        {26, 1},
        {26, 2},
        {-1, -1}
      },
      { /* names */
        "QueryInterface",
        "riid",
        "ppvObj",
        NULL,
      },
    },
    {
      0x60000001, /*func*/ FUNC_DISPATCH, /*inv*/ INVOKE_FUNC, /*call*/ 0x4,
      /*#param*/ 0, /*#opt*/ 0, /*vtbl*/ 1, /*#scodes*/ 0, /*flags*/ 0x1,
      {19, 0}, /* ret */
      { /* params */
        {-1, -1}
      },
      { /* names */
        "AddRef",
        NULL,
      },
    },
    {
      0x60000002, /*func*/ FUNC_DISPATCH, /*inv*/ INVOKE_FUNC, /*call*/ 0x4,
      /*#param*/ 0, /*#opt*/ 0, /*vtbl*/ 2, /*#scodes*/ 0, /*flags*/ 0x1,
      {19, 0}, /* ret */
      { /* params */
        {-1, -1}
      },
      { /* names */
        "Release",
        NULL,
      },
    },
    {
      0x60010000, /*func*/ FUNC_DISPATCH, /*inv*/ INVOKE_FUNC, /*call*/ 0x4,
      /*#param*/ 1, /*#opt*/ 0, /*vtbl*/ 3, /*#scodes*/ 0, /*flags*/ 0x1,
      {24, 0}, /* ret */
      { /* params */
        {26, 2},
        {-1, -1}
      },
      { /* names */
        "GetTypeInfoCount",
        "pctinfo",
        NULL,
      },
    },
    {
      0x60010001, /*func*/ FUNC_DISPATCH, /*inv*/ INVOKE_FUNC, /*call*/ 0x4,
      /*#param*/ 3, /*#opt*/ 0, /*vtbl*/ 4, /*#scodes*/ 0, /*flags*/ 0x1,
      {24, 0}, /* ret */
      { /* params */
        {23, 1},
        {19, 1},
        {26, 2},
        {-1, -1}
      },
      { /* names */
        "GetTypeInfo",
        "itinfo",
        "lcid",
        "pptinfo",
        NULL,
      },
    },
    {
      0x60010002, /*func*/ FUNC_DISPATCH, /*inv*/ INVOKE_FUNC, /*call*/ 0x4,
      /*#param*/ 5, /*#opt*/ 0, /*vtbl*/ 5, /*#scodes*/ 0, /*flags*/ 0x1,
      {24, 0}, /* ret */
      { /* params */
        {26, 1},
        {26, 1},
        {23, 1},
        {19, 1},
        {26, 2},
        {-1, -1}
      },
      { /* names */
        "GetIDsOfNames",
        "riid",
        "rgszNames",
        "cNames",
        "lcid",
        "rgdispid",
        NULL,
      },
    },
    {
      0x60010003, /*func*/ FUNC_DISPATCH, /*inv*/ INVOKE_FUNC, /*call*/ 0x4,
      /*#param*/ 8, /*#opt*/ 0, /*vtbl*/ 6, /*#scodes*/ 0, /*flags*/ 0x1,
      {24, 0}, /* ret */
      { /* params */
        {3, 1},
        {26, 1},
        {19, 1},
        {18, 1},
        {26, 1},
        {26, 2},
        {26, 2},
        {26, 2},
        {-1, -1}
      },
      { /* names */
        "Invoke",
        "dispidMember",
        "riid",
        "lcid",
        "wFlags",
        "pdispparams",
        "pvarResult",
        "pexcepinfo",
        "puArgErr",
        NULL,
      },
    },
    {
      0x60020000, /*func*/ FUNC_DISPATCH, /*inv*/ INVOKE_FUNC, /*call*/ 0x4,
      /*#param*/ 0, /*#opt*/ 0, /*vtbl*/ 7, /*#scodes*/ 0, /*flags*/ 0x0,
      {24, 0}, /* ret */
      { /* params */
        {-1, -1}
      },
      { /* names */
        "Test",
        NULL,
      },
    },
  }
},
{
  "ISimpleIface",
  /*kind*/ TKIND_INTERFACE, /*flags*/ 0x1000, /*align*/ 4, /*size*/ 4,
  /*#vtbl*/ 8, /*#func*/ 1,
  {
    {
      0x60020000, /*func*/ FUNC_PUREVIRTUAL, /*inv*/ INVOKE_FUNC, /*call*/ 0x4,
      /*#param*/ 0, /*#opt*/ 0, /*vtbl*/ 7, /*#scodes*/ 0, /*flags*/ 0x0,
      {25, 0}, /* ret */
      { /* params */
        {-1, -1}
      },
      { /* names */
        "Test",
        NULL,
      },
    },
  }
},
};

#define check_type(elem, info) { \
      expect_int((elem)->tdesc.vt, (info)->vt);                     \
      expect_hex(U(*(elem)).paramdesc.wParamFlags, (info)->wParamFlags); \
  }

static void test_dump_typelib(const char *name)
{
    WCHAR wszName[MAX_PATH];
    ITypeLib *typelib;
    int ifcount = sizeof(info)/sizeof(info[0]);
    int iface, func;

    MultiByteToWideChar(CP_ACP, 0, name, -1, wszName, MAX_PATH);
    ole_check(LoadTypeLibEx(wszName, REGKIND_NONE, &typelib));
    expect_eq(ITypeLib_GetTypeInfoCount(typelib), ifcount, UINT, "%d");
    for (iface = 0; iface < ifcount; iface++)
    {
        const interface_info *if_info = &info[iface];
        ITypeInfo *typeinfo;
        TYPEATTR *typeattr;
        BSTR bstrIfName;

        trace("Interface %s\n", if_info->name);
        ole_check(ITypeLib_GetTypeInfo(typelib, iface, &typeinfo));
        ole_check(ITypeLib_GetDocumentation(typelib, iface, &bstrIfName, NULL, NULL, NULL));
        expect_wstr_acpval(bstrIfName, if_info->name);
        SysFreeString(bstrIfName);

        ole_check(ITypeInfo_GetTypeAttr(typeinfo, &typeattr));
        expect_int(typeattr->typekind, if_info->type);
        expect_hex(typeattr->wTypeFlags, if_info->wTypeFlags);
        expect_int(typeattr->cbAlignment, if_info->cbAlignment);
        expect_int(typeattr->cbSizeInstance, if_info->cbSizeInstance);
        expect_int(typeattr->cbSizeVft, if_info->cbSizeVft * sizeof(void*));
        expect_int(typeattr->cFuncs, if_info->cFuncs);

        for (func = 0; func < typeattr->cFuncs; func++)
        {
            function_info *fn_info = (function_info *)&if_info->funcs[func];
            FUNCDESC *desc;
            BSTR namesTab[256];
            UINT cNames;
            int i;

            trace("Function %s\n", fn_info->names[0]);
            ole_check(ITypeInfo_GetFuncDesc(typeinfo, func, &desc));
            expect_int(desc->memid, fn_info->memid);
            expect_int(desc->funckind, fn_info->funckind);
            expect_int(desc->invkind, fn_info->invkind);
            expect_int(desc->callconv, fn_info->callconv);
            expect_int(desc->cParams, fn_info->cParams);
            expect_int(desc->cParamsOpt, fn_info->cParamsOpt);
            ok( desc->oVft == fn_info->vtbl_index * sizeof(void*) ||
                broken(desc->oVft == fn_info->vtbl_index * 4), /* xp64 */
                "desc->oVft got %u\n", desc->oVft );
            expect_int(desc->cScodes, fn_info->cScodes);
            expect_int(desc->wFuncFlags, fn_info->wFuncFlags);
            ole_check(ITypeInfo_GetNames(typeinfo, desc->memid, namesTab, 256, &cNames));
            for (i = 0; i < cNames; i++)
            {
                expect_wstr_acpval(namesTab[i], fn_info->names[i]);
                SysFreeString(namesTab[i]);
            }
            expect_null(fn_info->names[cNames]);

            check_type(&desc->elemdescFunc, &fn_info->ret_type);
            for (i = 0 ; i < desc->cParams; i++)
            {
                check_type(&desc->lprgelemdescParam[i], &fn_info->params[i]);
            }
            expect_int(fn_info->params[desc->cParams].vt, (VARTYPE)-1);

            ITypeInfo_ReleaseFuncDesc(typeinfo, desc);
        }

        ITypeInfo_ReleaseTypeAttr(typeinfo, typeattr);
        ITypeInfo_Release(typeinfo);
    }
    ITypeLib_Release(typelib);
}

#endif

static void test_create_typelib_lcid(LCID lcid)
{
    char filename[MAX_PATH];
    WCHAR name[MAX_PATH];
    HRESULT hr;
    ICreateTypeLib2 *tl;
    HANDLE file;
    DWORD msft_header[5]; /* five is enough for now */
    DWORD read;

    GetTempFileNameA( ".", "tlb", 0, filename );
    MultiByteToWideChar(CP_ACP, 0, filename, -1, name, MAX_PATH);

    hr = CreateTypeLib2(SYS_WIN32, name, &tl);
    ok(hr == S_OK, "got %08x\n", hr);

    hr = ICreateTypeLib2_SetLcid(tl, lcid);
    ok(hr == S_OK, "got %08x\n", hr);

    hr = ICreateTypeLib2_SaveAllChanges(tl);
    ok(hr == S_OK, "got %08x\n", hr);

    ICreateTypeLib2_Release(tl);

    file = CreateFileA( filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0 );
    ok( file != INVALID_HANDLE_VALUE, "file creation failed\n" );

    ReadFile( file, msft_header, sizeof(msft_header), &read, NULL );
    ok(read == sizeof(msft_header), "read %d\n", read);
    CloseHandle( file );

    ok(msft_header[0] == 0x5446534d, "got %08x\n", msft_header[0]);
    ok(msft_header[1] == 0x00010002, "got %08x\n", msft_header[1]);
    ok(msft_header[2] == 0xffffffff, "got %08x\n", msft_header[2]);
    ok(msft_header[3] == (lcid ? lcid : 0x409), "got %08x (lcid %08x)\n", msft_header[3], lcid);
    ok(msft_header[4] == lcid, "got %08x (lcid %08x)\n", msft_header[4], lcid);

    DeleteFileA(filename);
}

static void test_create_typelibs(void)
{
    test_create_typelib_lcid(LOCALE_SYSTEM_DEFAULT);
    test_create_typelib_lcid(LOCALE_USER_DEFAULT);
    test_create_typelib_lcid(LOCALE_NEUTRAL);

    test_create_typelib_lcid(0x009);
    test_create_typelib_lcid(0x409);
    test_create_typelib_lcid(0x809);

    test_create_typelib_lcid(0x007);
    test_create_typelib_lcid(0x407);
}


static void test_register_typelib(BOOL system_registration)
{
    HRESULT hr;
    WCHAR filename[MAX_PATH];
    const char *filenameA;
    ITypeLib *typelib;
    WCHAR uuidW[40];
    char key_name[MAX_PATH], uuid[40];
    LONG ret, expect_ret;
    UINT count, i;
    HKEY hkey;
    struct
    {
        TYPEKIND kind;
        WORD flags;
    } attrs[12] =
    {
        { TKIND_INTERFACE, 0 },
        { TKIND_INTERFACE, TYPEFLAG_FDISPATCHABLE },
        { TKIND_INTERFACE, TYPEFLAG_FOLEAUTOMATION },
        { TKIND_INTERFACE, TYPEFLAG_FDISPATCHABLE | TYPEFLAG_FOLEAUTOMATION },
        { TKIND_DISPATCH,  0 /* TYPEFLAG_FDUAL - widl clears this flag for non-IDispatch derived interfaces */ },
        { TKIND_DISPATCH,  0 /* TYPEFLAG_FDUAL - widl clears this flag for non-IDispatch derived interfaces */ },
        { TKIND_DISPATCH,  TYPEFLAG_FDISPATCHABLE | TYPEFLAG_FDUAL },
        { TKIND_DISPATCH,  TYPEFLAG_FDISPATCHABLE | TYPEFLAG_FDUAL },
        { TKIND_DISPATCH,  TYPEFLAG_FDISPATCHABLE },
        { TKIND_DISPATCH,  TYPEFLAG_FDISPATCHABLE },
        { TKIND_DISPATCH,  TYPEFLAG_FDISPATCHABLE },
        { TKIND_INTERFACE, TYPEFLAG_FDISPATCHABLE }
    };

    trace("Starting %s typelib registration tests\n",
          system_registration ? "system" : "user");

    if (!system_registration && (!pRegisterTypeLibForUser || !pUnRegisterTypeLibForUser))
    {
        win_skip("User typelib registration functions are not available\n");
        return;
    }

    filenameA = create_test_typelib(3);
    MultiByteToWideChar(CP_ACP, 0, filenameA, -1, filename, MAX_PATH);

    hr = LoadTypeLibEx(filename, REGKIND_NONE, &typelib);
    ok(hr == S_OK, "got %08x\n", hr);

    if (system_registration)
        hr = RegisterTypeLib(typelib, filename, NULL);
    else
        hr = pRegisterTypeLibForUser(typelib, filename, NULL);
    if (hr == TYPE_E_REGISTRYACCESS)
    {
        win_skip("Insufficient privileges to register typelib in the registry\n");
        ITypeLib_Release(typelib);
        DeleteFileA(filenameA);
        return;
    }
    ok(hr == S_OK, "got %08x\n", hr);

    count = ITypeLib_GetTypeInfoCount(typelib);
    ok(count == 12, "got %d\n", count);

    for(i = 0; i < count; i++)
    {
        ITypeInfo *typeinfo;
        TYPEATTR *attr;

        hr = ITypeLib_GetTypeInfo(typelib, i, &typeinfo);
        ok(hr == S_OK, "got %08x\n", hr);

        hr = ITypeInfo_GetTypeAttr(typeinfo, &attr);
        ok(hr == S_OK, "got %08x\n", hr);

        ok(attr->typekind == attrs[i].kind, "%d: got kind %d\n", i, attr->typekind);
        ok(attr->wTypeFlags == attrs[i].flags, "%d: got flags %04x\n", i, attr->wTypeFlags);

        if(attr->typekind == TKIND_DISPATCH && (attr->wTypeFlags & TYPEFLAG_FDUAL))
        {
            HREFTYPE reftype;
            ITypeInfo *dual_info;
            TYPEATTR *dual_attr;

            hr = ITypeInfo_GetRefTypeOfImplType(typeinfo, -1, &reftype);
            ok(hr == S_OK, "got %08x\n", hr);

            hr = ITypeInfo_GetRefTypeInfo(typeinfo, reftype, &dual_info);
            ok(hr == S_OK, "got %08x\n", hr);

            hr = ITypeInfo_GetTypeAttr(dual_info, &dual_attr);
            ok(hr == S_OK, "got %08x\n", hr);

            ok(dual_attr->typekind == TKIND_INTERFACE, "%d: got kind %d\n", i, dual_attr->typekind);
            ok(dual_attr->wTypeFlags == (TYPEFLAG_FDISPATCHABLE | TYPEFLAG_FOLEAUTOMATION | TYPEFLAG_FDUAL), "%d: got flags %04x\n", i, dual_attr->wTypeFlags);

            ITypeInfo_ReleaseTypeAttr(dual_info, dual_attr);
            ITypeInfo_Release(dual_info);

        }

        StringFromGUID2(&attr->guid, uuidW, sizeof(uuidW) / sizeof(uuidW[0]));
        WideCharToMultiByte(CP_ACP, 0, uuidW, -1, uuid, sizeof(uuid), NULL, NULL);
        sprintf(key_name, "Interface\\%s", uuid);

        /* All dispinterfaces will be registered (this includes dual interfaces) as well
           as oleautomation interfaces */
        if((attr->typekind == TKIND_INTERFACE && (attr->wTypeFlags & TYPEFLAG_FOLEAUTOMATION)) ||
           attr->typekind == TKIND_DISPATCH)
            expect_ret = ERROR_SUCCESS;
        else
            expect_ret = ERROR_FILE_NOT_FOUND;

        ret = RegOpenKeyExA(HKEY_CLASSES_ROOT, key_name, 0, KEY_READ, &hkey);
        ok(ret == expect_ret, "%d: got %d\n", i, ret);
        if(ret == ERROR_SUCCESS) RegCloseKey(hkey);

        ITypeInfo_ReleaseTypeAttr(typeinfo, attr);
        ITypeInfo_Release(typeinfo);
    }

    if (system_registration)
        hr = UnRegisterTypeLib(&LIBID_register_test, 1, 0, LOCALE_NEUTRAL, is_win64 ? SYS_WIN64 : SYS_WIN32);
    else
        hr = pUnRegisterTypeLibForUser(&LIBID_register_test, 1, 0, LOCALE_NEUTRAL, is_win64 ? SYS_WIN64 : SYS_WIN32);
    ok(hr == S_OK, "got %08x\n", hr);

    ITypeLib_Release(typelib);
    DeleteFileA( filenameA );
}

static void test_LoadTypeLib(void)
{
    ITypeLib *tl;
    HRESULT hres;

    static const WCHAR kernel32_dllW[] = {'k','e','r','n','e','l','3','2','.','d','l','l',0};

    hres = LoadTypeLib(kernel32_dllW, &tl);
    ok(hres == TYPE_E_CANTLOADLIBRARY, "LoadTypeLib returned: %08x, expected TYPE_E_CANTLOADLIBRARY\n", hres);
}

static void test_SetVarHelpContext(void)
{
    static OLECHAR nameW[] = {'n','a','m','e',0};
    CHAR filenameA[MAX_PATH];
    WCHAR filenameW[MAX_PATH];
    ICreateTypeLib2 *ctl;
    ICreateTypeInfo *cti;
    VARDESC desc;
    HRESULT hr;
    VARIANT v;

    GetTempFileNameA(".", "tlb", 0, filenameA);
    MultiByteToWideChar(CP_ACP, 0, filenameA, -1, filenameW, MAX_PATH);

    hr = CreateTypeLib2(SYS_WIN32, filenameW, &ctl);
    ok(hr == S_OK, "got %08x\n", hr);

    hr = ICreateTypeLib2_CreateTypeInfo(ctl, nameW, TKIND_ENUM, &cti);
    ok(hr == S_OK, "got %08x\n", hr);

    hr = ICreateTypeInfo_SetVarHelpContext(cti, 0, 0);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "got %08x\n", hr);

    memset(&desc, 0, sizeof(desc));
    desc.elemdescVar.tdesc.vt = VT_INT;
    desc.varkind = VAR_CONST;

    V_VT(&v) = VT_INT;
    V_INT(&v) = 1;
    U(desc).lpvarValue = &v;
    hr = ICreateTypeInfo_AddVarDesc(cti, 0, &desc);
    ok(hr == S_OK, "got %08x\n", hr);

    hr = ICreateTypeInfo_SetVarHelpContext(cti, 0, 0);
    ok(hr == S_OK, "got %08x\n", hr);

    /* another time */
    hr = ICreateTypeInfo_SetVarHelpContext(cti, 0, 1);
    ok(hr == S_OK, "got %08x\n", hr);

    /* wrong index now */
    hr = ICreateTypeInfo_SetVarHelpContext(cti, 1, 0);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "got %08x\n", hr);

    ICreateTypeInfo_Release(cti);
    ICreateTypeLib2_Release(ctl);
    DeleteFileA(filenameA);
}

static void test_SetFuncAndParamNames(void)
{
    static OLECHAR nameW[] = {'n','a','m','e',0};
    static OLECHAR prop[] = {'p','r','o','p',0};
    static OLECHAR *propW[] = {prop};
    CHAR filenameA[MAX_PATH];
    WCHAR filenameW[MAX_PATH];
    ICreateTypeLib2 *ctl;
    ICreateTypeInfo *cti;
    FUNCDESC funcdesc;
    ELEMDESC edesc;
    HRESULT hr;

    GetTempFileNameA(".", "tlb", 0, filenameA);
    MultiByteToWideChar(CP_ACP, 0, filenameA, -1, filenameW, MAX_PATH);

    hr = CreateTypeLib2(SYS_WIN32, filenameW, &ctl);
    ok(hr == S_OK, "got %08x\n", hr);

    hr = ICreateTypeLib2_CreateTypeInfo(ctl, nameW, TKIND_DISPATCH, &cti);
    ok(hr == S_OK, "got %08x\n", hr);

    /* get method */
    memset(&funcdesc, 0, sizeof(FUNCDESC));
    funcdesc.funckind = FUNC_DISPATCH;
    funcdesc.callconv = CC_STDCALL;
    funcdesc.elemdescFunc.tdesc.vt = VT_VOID;
    funcdesc.wFuncFlags = FUNCFLAG_FBINDABLE;

    /* put method */
    memset(&edesc, 0, sizeof(edesc));
    edesc.tdesc.vt = VT_BSTR;
    U(edesc).idldesc.dwReserved = 0;
    U(edesc).idldesc.wIDLFlags = IDLFLAG_FIN;

    funcdesc.lprgelemdescParam = &edesc;
    funcdesc.invkind = INVOKE_PROPERTYPUT;
    funcdesc.cParams = 1;

    hr = ICreateTypeInfo_AddFuncDesc(cti, 0, &funcdesc);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    /* setter name */
    hr = ICreateTypeInfo_SetFuncAndParamNames(cti, 0, propW, 1);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    /* putref method */
    funcdesc.invkind = INVOKE_PROPERTYPUTREF;
    hr = ICreateTypeInfo_AddFuncDesc(cti, 1, &funcdesc);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    /* putref name */
    hr = ICreateTypeInfo_SetFuncAndParamNames(cti, 1, propW, 1);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    funcdesc.invkind = INVOKE_PROPERTYGET;
    funcdesc.cParams = 0;
    hr = ICreateTypeInfo_AddFuncDesc(cti, 2, &funcdesc);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    /* getter name */
    hr = ICreateTypeInfo_SetFuncAndParamNames(cti, 2, propW, 1);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    ICreateTypeInfo_Release(cti);
    ICreateTypeLib2_Release(ctl);
    DeleteFileA(filenameA);
}

static void test_SetVarDocString(void)
{
    static OLECHAR nameW[] = {'n','a','m','e',0};
    static OLECHAR doc1W[] = {'d','o','c','1',0};
    static OLECHAR doc2W[] = {'d','o','c','2',0};
    CHAR filenameA[MAX_PATH];
    WCHAR filenameW[MAX_PATH];
    ICreateTypeLib2 *ctl;
    ICreateTypeInfo *cti;
    VARDESC desc;
    HRESULT hr;
    VARIANT v;

    GetTempFileNameA(".", "tlb", 0, filenameA);
    MultiByteToWideChar(CP_ACP, 0, filenameA, -1, filenameW, MAX_PATH);

    hr = CreateTypeLib2(SYS_WIN32, filenameW, &ctl);
    ok(hr == S_OK, "got %08x\n", hr);

    hr = ICreateTypeLib2_CreateTypeInfo(ctl, nameW, TKIND_ENUM, &cti);
    ok(hr == S_OK, "got %08x\n", hr);

    hr = ICreateTypeInfo_SetVarDocString(cti, 0, doc1W);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "got %08x\n", hr);

    hr = ICreateTypeInfo_SetVarDocString(cti, 0, NULL);
    ok(hr == E_INVALIDARG, "got %08x\n", hr);

    memset(&desc, 0, sizeof(desc));
    desc.elemdescVar.tdesc.vt = VT_INT;
    desc.varkind = VAR_CONST;

    V_VT(&v) = VT_INT;
    V_INT(&v) = 1;
    U(desc).lpvarValue = &v;
    hr = ICreateTypeInfo_AddVarDesc(cti, 0, &desc);
    ok(hr == S_OK, "got %08x\n", hr);

    hr = ICreateTypeInfo_SetVarDocString(cti, 0, NULL);
    ok(hr == E_INVALIDARG, "got %08x\n", hr);

    hr = ICreateTypeInfo_SetVarDocString(cti, 0, doc1W);
    ok(hr == S_OK, "got %08x\n", hr);

    /* already set */
    hr = ICreateTypeInfo_SetVarDocString(cti, 0, doc2W);
    ok(hr == S_OK, "got %08x\n", hr);

    /* wrong index now */
    hr = ICreateTypeInfo_SetVarDocString(cti, 1, doc1W);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "got %08x\n", hr);

    ICreateTypeInfo_Release(cti);
    ICreateTypeLib2_Release(ctl);
    DeleteFileA(filenameA);
}

static void test_FindName(void)
{
    static const WCHAR invalidW[] = {'i','n','v','a','l','i','d',0};
    WCHAR buffW[100];
    MEMBERID memid;
    ITypeInfo *ti;
    ITypeLib *tl;
    HRESULT hr;
    UINT16 c;

    hr = LoadTypeLib(wszStdOle2, &tl);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = ITypeLib_FindName(tl, NULL, 0, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    lstrcpyW(buffW, wszGUID);
    hr = ITypeLib_FindName(tl, buffW, 0, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    c = 0;
    ti = (void*)0xdeadbeef;
    hr = ITypeLib_FindName(tl, buffW, 0, &ti, NULL, &c);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);
    ok(c == 0, "got %d\n", c);
    ok(ti == (void*)0xdeadbeef, "got %p\n", ti);

    c = 1;
    ti = (void*)0xdeadbeef;
    hr = ITypeLib_FindName(tl, buffW, 0, &ti, NULL, &c);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);
    ok(c == 1, "got %d\n", c);
    ok(ti == (void*)0xdeadbeef, "got %p\n", ti);

    c = 1;
    memid = 0;
    ti = (void*)0xdeadbeef;
    hr = ITypeLib_FindName(tl, buffW, 0, &ti, &memid, &c);
    ok(hr == S_OK, "got 0x%08x\n", hr);
todo_wine
    ok(memid == -1, "got %d\n", memid);
    ok(!lstrcmpW(buffW, wszGUID), "got %s\n", wine_dbgstr_w(buffW));
    ok(c == 1, "got %d\n", c);
    ITypeInfo_Release(ti);

    c = 1;
    memid = 0;
    lstrcpyW(buffW, wszguid);
    ti = (void*)0xdeadbeef;
    hr = ITypeLib_FindName(tl, buffW, 0, &ti, &memid, &c);
    ok(hr == S_OK, "got 0x%08x\n", hr);
todo_wine {
    ok(memid == -1, "got %d\n", memid);
    ok(!lstrcmpW(buffW, wszGUID), "got %s\n", wine_dbgstr_w(buffW));
    ok(c == 1, "got %d\n", c);
}
    if (c == 1)
        ITypeInfo_Release(ti);

    c = 1;
    memid = -1;
    lstrcpyW(buffW, invalidW);
    ti = (void*)0xdeadbeef;
    hr = ITypeLib_FindName(tl, buffW, 0, &ti, &memid, &c);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(memid == -1, "got %d\n", memid);
    ok(!lstrcmpW(buffW, invalidW), "got %s\n", wine_dbgstr_w(buffW));
    ok(c == 0, "got %d\n", c);
    ok(ti == (void*)0xdeadbeef, "got %p\n", ti);

    ITypeLib_Release(tl);
}

START_TEST(typelib)
{
    const char *filename;

    init_function_pointers();

    ref_count_test(wszStdOle2);
    test_TypeComp();
    test_CreateDispTypeInfo();
    test_TypeInfo();
    test_DispCallFunc();
    test_QueryPathOfRegTypeLib(32);
    if(sizeof(void*) == 8)
        test_QueryPathOfRegTypeLib(64);
    test_inheritance();
    test_CreateTypeLib();
    test_SetVarHelpContext();
    test_SetFuncAndParamNames();
    test_SetVarDocString();
    test_FindName();

    if ((filename = create_test_typelib(2)))
    {
        test_dump_typelib( filename );
        DeleteFile( filename );
    }

    test_register_typelib(TRUE);
    test_register_typelib(FALSE);
    test_create_typelibs();
    test_LoadTypeLib();
}
