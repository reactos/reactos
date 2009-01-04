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

#define COBJMACROS

#include <wine/test.h>
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"

#include "oleauto.h"
#include "ocidl.h"
#include "shlwapi.h"
#include "initguid.h"
#include "tmarshal.h"

#define expect_eq(expr, value, type, format) { type _ret = (expr); ok((value) == _ret, #expr " expected " format " got " format "\n", value, _ret); }
#define expect_int(expr, value) expect_eq(expr, (int)value, int, "%d")
#define expect_hex(expr, value) expect_eq(expr, (int)value, int, "0x%x")
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

static const WCHAR wszStdOle2[] = {'s','t','d','o','l','e','2','.','t','l','b',0};

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
    static WCHAR wszStdFunctions[] = {'S','t','d','F','u','n','c','t','i','o','n','s',0};
    static WCHAR wszSavePicture[] = {'S','a','v','e','P','i','c','t','u','r','e',0};
    static WCHAR wszOLE_TRISTATE[] = {'O','L','E','_','T','R','I','S','T','A','T','E',0};
    static WCHAR wszUnchecked[] = {'U','n','c','h','e','c','k','e','d',0};
    static WCHAR wszIUnknown[] = {'I','U','n','k','n','o','w','n',0};
    static WCHAR wszFont[] = {'F','o','n','t',0};
    static WCHAR wszGUID[] = {'G','U','I','D',0};
    static WCHAR wszStdPicture[] = {'S','t','d','P','i','c','t','u','r','e',0};
    static WCHAR wszOLE_COLOR[] = {'O','L','E','_','C','O','L','O','R',0};
    static WCHAR wszClone[] = {'C','l','o','n','e',0};
    static WCHAR wszclone[] = {'c','l','o','n','e',0};

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
    todo_wine ok(hr == TYPE_E_TYPEMISMATCH,
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
    ok(pFuncDesc->oVft == 4, "oVft %d\n", pFuncDesc->oVft);
    ok(pFuncDesc->wFuncFlags == 0, "oVft %d\n", pFuncDesc->wFuncFlags);
    ok(pFuncDesc->elemdescFunc.tdesc.vt == VT_I4, "ret vt %x\n", pFuncDesc->elemdescFunc.tdesc.vt);
    ITypeInfo_ReleaseFuncDesc(pTI2, pFuncDesc);

    hr = ITypeInfo_GetFuncDesc(pTI2, 2, &pFuncDesc);
    ok(hr == S_OK, "hr %08x\n", hr);
    ok(pFuncDesc->funckind == FUNC_VIRTUAL, "funckind %d\n", pFuncDesc->funckind);
    ok(pFuncDesc->invkind == methdata[2].wFlags, "invkind %d\n", pFuncDesc->invkind);
    ok(pFuncDesc->callconv == methdata[2].cc, "callconv %d\n", pFuncDesc->callconv);
    ok(pFuncDesc->cParams == methdata[2].cArgs, "cParams %d\n", pFuncDesc->cParams);
    ok(pFuncDesc->oVft == 12, "oVft %d\n", pFuncDesc->oVft);
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
    ok(pFuncDesc->oVft == 16, "oVft %d\n", pFuncDesc->oVft);
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

static void test_TypeInfo(void)
{
    ITypeLib *pTypeLib;
    ITypeInfo *pTypeInfo;
    HRESULT hr;
    static WCHAR wszBogus[] = { 'b','o','g','u','s',0 };
    static WCHAR wszGetTypeInfo[] = { 'G','e','t','T','y','p','e','I','n','f','o',0 };
    static WCHAR wszClone[] = {'C','l','o','n','e',0};
    OLECHAR* bogus = wszBogus;
    OLECHAR* pwszGetTypeInfo = wszGetTypeInfo;
    OLECHAR* pwszClone = wszClone;
    DISPID dispidMember;
    DISPPARAMS dispparams;

    hr = LoadTypeLib(wszStdOle2, &pTypeLib);
    ok_ole_success(hr, LoadTypeLib);

    hr = ITypeLib_GetTypeInfoOfGuid(pTypeLib, &IID_IFont, &pTypeInfo);
    ok_ole_success(hr, ITypeLib_GetTypeInfoOfGuid); 

    /* test nonexistent method name */
    hr = ITypeInfo_GetIDsOfNames(pTypeInfo, &bogus, 1, &dispidMember);
    ok(hr == DISP_E_UNKNOWNNAME,
       "ITypeInfo_GetIDsOfNames should have returned DISP_E_UNKNOWNNAME instead of 0x%08x\n",
       hr);

    /* test invalid memberid */
    dispparams.cNamedArgs = 0;
    dispparams.cArgs = 0;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.rgvarg = NULL;
    hr = ITypeInfo_Invoke(pTypeInfo, (void *)0xdeadbeef, 0xdeadbeef, DISPATCH_METHOD, &dispparams, NULL, NULL, NULL);
    ok(hr == DISP_E_MEMBERNOTFOUND, "ITypeInfo_Invoke should have returned DISP_E_MEMBERNOTFOUND instead of 0x%08x\n", hr);

    hr = ITypeInfo_GetIDsOfNames(pTypeInfo, &pwszClone, 1, &dispidMember);
    ok_ole_success(hr, ITypeInfo_GetIDsOfNames);

    /* test correct memberid, but wrong flags */
    hr = ITypeInfo_Invoke(pTypeInfo, (void *)0xdeadbeef, dispidMember, DISPATCH_PROPERTYGET, &dispparams, NULL, NULL, NULL);
    ok(hr == DISP_E_MEMBERNOTFOUND, "ITypeInfo_Invoke should have returned DISP_E_MEMBERNOTFOUND instead of 0x%08x\n", hr);

    /* test NULL dispparams */
    hr = ITypeInfo_Invoke(pTypeInfo, (void *)0xdeadbeef, dispidMember, DISPATCH_METHOD, NULL, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "ITypeInfo_Invoke should have returned E_INVALIDARG instead of 0x%08x\n", hr);

    /* test dispparams->cNamedArgs being bigger than dispparams->cArgs */
    dispparams.cNamedArgs = 1;
    hr = ITypeInfo_Invoke(pTypeInfo, (void *)0xdeadbeef, dispidMember, DISPATCH_METHOD, &dispparams, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "ITypeInfo_Invoke should have returned E_INVALIDARG instead of 0x%08x\n", hr);

    ITypeInfo_Release(pTypeInfo);

    hr = ITypeLib_GetTypeInfoOfGuid(pTypeLib, &IID_IDispatch, &pTypeInfo);
    ok_ole_success(hr, ITypeLib_GetTypeInfoOfGuid); 

    hr = ITypeInfo_GetIDsOfNames(pTypeInfo, &pwszGetTypeInfo, 1, &dispidMember);
    ok_ole_success(hr, ITypeInfo_GetIDsOfNames);

    /* test invoking a method with a [restricted] keyword */
    hr = ITypeInfo_Invoke(pTypeInfo, NULL, dispidMember, DISPATCH_METHOD, &dispparams, NULL, NULL, NULL);
    todo_wine {
    ok(hr == DISP_E_MEMBERNOTFOUND, "ITypeInfo_Invoke should have returned DISP_E_MEMBERNOTFOUND instead of 0x%08x\n", hr);
    }

    ITypeInfo_Release(pTypeInfo);
    ITypeLib_Release(pTypeLib);
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

static BOOL do_typelib_reg_key(GUID *uid, WORD maj, WORD min, LPCWSTR base, BOOL remove)
{
    static const WCHAR typelibW[] = {'T','y','p','e','l','i','b','\\',0};
    static const WCHAR formatW[] = {'\\','%','u','.','%','u','\\','0','\\','w','i','n','3','2',0};
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

    wsprintfW(buf + lstrlenW(buf), formatW, maj, min );

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
        trace("RegCreateKeyExW failed\n");
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

static void test_QueryPathOfRegTypeLib(void)
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
        { 4, 0, TYPE_E_LIBNOTREGISTERED, { 0 } }
    };
    static const WCHAR base[] = {'f','a','k','e',0};
    UINT i;
    RPC_STATUS status;
    GUID uid;
    WCHAR uid_str[40];
    HRESULT ret;
    BSTR path;

    status = UuidCreate(&uid);
    ok(!status || status == RPC_S_UUID_LOCAL_ONLY, "UuidCreate error %08lx\n", status);

    StringFromGUID2(&uid, uid_str, 40);
    /*trace("GUID: %s\n", wine_dbgstr_w(uid_str));*/

    if (!do_typelib_reg_key(&uid, 3, 0, base, 0)) return;
    if (!do_typelib_reg_key(&uid, 3, 1, base, 0)) return;
    if (!do_typelib_reg_key(&uid, 3, 37, base, 0)) return;

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

    do_typelib_reg_key(&uid, 0, 0, NULL, 1);
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
    ok(pTA->cbSizeVft == 28, "sizevft %d\n", pTA->cbSizeVft);
    ok(pTA->wTypeFlags == TYPEFLAG_FDISPATCHABLE, "typeflags %x\n", pTA->wTypeFlags);
if(use_midl_tlb) {
    ok(pTA->cFuncs == 6, "cfuncs %d\n", pTA->cFuncs);
    ok(pTA->cImplTypes == 1, "cimpltypes %d\n", pTA->cImplTypes);
    ITypeInfo_ReleaseTypeAttr(pTI, pTA);

    hr = ITypeInfo_GetRefTypeOfImplType(pTI, 0, &href);
    ok(hr == S_OK, "hr %08x\n", hr);
    hr = ITypeInfo_GetRefTypeInfo(pTI, href, &pTI_p);
    ok(hr == S_OK, "hr %08x\n", hr);
    hr = ITypeInfo_GetTypeAttr(pTI_p, &pTA);
    ok(IsEqualGUID(&pTA->guid, &IID_IDispatch), "guid {%08x-....\n", pTA->guid.Data1);
    ITypeInfo_ReleaseTypeAttr(pTI_p, pTA);
    ITypeInfo_Release(pTI_p);

    /* Should have six methods */
    hr = ITypeInfo_GetFuncDesc(pTI, 6, &pFD);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "hr %08x\n", hr);
    hr = ITypeInfo_GetFuncDesc(pTI, 5, &pFD);
    ok(hr == S_OK, "hr %08x\n", hr);
    ok(pFD->memid == 0x60020000, "memid %08x\n", pFD->memid);
    ok(pFD->oVft == 20, "oVft %d\n", pFD->oVft);
    ITypeInfo_ReleaseFuncDesc(pTI, pFD);
}
    ITypeInfo_Release(pTI);


    /* ItestIF4 is a syntax 1 dispinterface */
    hr = ITypeLib_GetTypeInfoOfGuid(pTL, &DIID_ItestIF4, &pTI);
    ok(hr == S_OK, "hr %08x\n", hr);

    hr = ITypeInfo_GetTypeAttr(pTI, &pTA);
    ok(hr == S_OK, "hr %08x\n", hr);
    ok(pTA->typekind == TKIND_DISPATCH, "kind %04x\n", pTA->typekind);
    ok(pTA->cbSizeVft == 28, "sizevft %d\n", pTA->cbSizeVft);
    ok(pTA->wTypeFlags == TYPEFLAG_FDISPATCHABLE, "typeflags %x\n", pTA->wTypeFlags);
    ok(pTA->cFuncs == 1, "cfuncs %d\n", pTA->cFuncs);
    ok(pTA->cImplTypes == 1, "cimpltypes %d\n", pTA->cImplTypes);
    ITypeInfo_ReleaseTypeAttr(pTI, pTA);

    hr = ITypeInfo_GetRefTypeOfImplType(pTI, 0, &href);
    ok(hr == S_OK, "hr %08x\n", hr);
    hr = ITypeInfo_GetRefTypeInfo(pTI, href, &pTI_p);
    ok(hr == S_OK, "hr %08x\n", hr);
    hr = ITypeInfo_GetTypeAttr(pTI_p, &pTA);
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
    ok(pTA->typekind == TKIND_DISPATCH, "kind %04x\n", pTA->typekind);
    ok(pTA->cbSizeVft == 28, "sizevft %d\n", pTA->cbSizeVft);
if(use_midl_tlb) {
    ok(pTA->wTypeFlags == TYPEFLAG_FDUAL, "typeflags %x\n", pTA->wTypeFlags);
 }
    ok(pTA->cFuncs == 8, "cfuncs %d\n", pTA->cFuncs);
    ok(pTA->cImplTypes == 1, "cimpltypes %d\n", pTA->cImplTypes);
    ITypeInfo_ReleaseTypeAttr(pTI, pTA);

    hr = ITypeInfo_GetRefTypeOfImplType(pTI, 0, &href);
    ok(hr == S_OK, "hr %08x\n", hr);
    hr = ITypeInfo_GetRefTypeInfo(pTI, href, &pTI_p);
    ok(hr == S_OK, "hr %08x\n", hr);
    hr = ITypeInfo_GetTypeAttr(pTI_p, &pTA);
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
    ok(pTA->cbSizeVft == 28, "sizevft %d\n", pTA->cbSizeVft);
    ok(pTA->wTypeFlags == (TYPEFLAG_FDISPATCHABLE|TYPEFLAG_FDUAL), "typeflags %x\n", pTA->wTypeFlags);
    ok(pTA->cFuncs == 10, "cfuncs %d\n", pTA->cFuncs);
    ok(pTA->cImplTypes == 1, "cimpltypes %d\n", pTA->cImplTypes);
    ITypeInfo_ReleaseTypeAttr(pTI, pTA);

    hr = ITypeInfo_GetRefTypeOfImplType(pTI, 0, &href);
    ok(hr == S_OK, "hr %08x\n", hr);
    hr = ITypeInfo_GetRefTypeInfo(pTI, href, &pTI_p);
    ok(hr == S_OK, "hr %08x\n", hr);
    hr = ITypeInfo_GetTypeAttr(pTI_p, &pTA);
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
    ok(pTA->cbSizeVft == 28, "sizevft %d\n", pTA->cbSizeVft);
    ok(pTA->wTypeFlags == TYPEFLAG_FDISPATCHABLE, "typeflags %x\n", pTA->wTypeFlags);
if(use_midl_tlb) {
    ok(pTA->cFuncs == 3, "cfuncs %d\n", pTA->cFuncs);
    ok(pTA->cImplTypes == 1, "cimpltypes %d\n", pTA->cImplTypes);
    ITypeInfo_ReleaseTypeAttr(pTI, pTA);

    hr = ITypeInfo_GetRefTypeOfImplType(pTI, -1, &href);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "hr %08x\n", hr);
    hr = ITypeInfo_GetRefTypeOfImplType(pTI, 0, &href);
    ok(hr == S_OK, "hr %08x\n", hr);
    hr = ITypeInfo_GetRefTypeInfo(pTI, href, &pTI_p);
    ok(hr == S_OK, "hr %08x\n", hr);
    hr = ITypeInfo_GetTypeAttr(pTI_p, &pTA);
    ok(IsEqualGUID(&pTA->guid, &IID_IDispatch), "guid {%08x-....\n", pTA->guid.Data1);
    ITypeInfo_ReleaseTypeAttr(pTI_p, pTA);
    ITypeInfo_Release(pTI_p);

    /* Should have three methods */
    hr = ITypeInfo_GetFuncDesc(pTI, 3, &pFD);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "hr %08x\n", hr);
    hr = ITypeInfo_GetFuncDesc(pTI, 2, &pFD);
    ok(hr == S_OK, "hr %08x\n", hr);
    ok(pFD->memid == 0x60010000, "memid %08x\n", pFD->memid);
    ok(pFD->oVft == 8, "oVft %d\n", pFD->oVft);
    ITypeInfo_ReleaseFuncDesc(pTI, pFD);
}
    ITypeInfo_Release(pTI);

    /* ItestIF11 is a syntax 2 dispinterface which derives from IDispatch */
    hr = ITypeLib_GetTypeInfoOfGuid(pTL, &DIID_ItestIF11, &pTI);
    ok(hr == S_OK, "hr %08x\n", hr);

    hr = ITypeInfo_GetTypeAttr(pTI, &pTA);
    ok(hr == S_OK, "hr %08x\n", hr);
    ok(pTA->typekind == TKIND_DISPATCH, "kind %04x\n", pTA->typekind);
    ok(pTA->cbSizeVft == 28, "sizevft %d\n", pTA->cbSizeVft);
    ok(pTA->wTypeFlags == TYPEFLAG_FDISPATCHABLE, "typeflags %x\n", pTA->wTypeFlags);
if(use_midl_tlb) {
    ok(pTA->cFuncs == 10, "cfuncs %d\n", pTA->cFuncs);
    ok(pTA->cImplTypes == 1, "cimpltypes %d\n", pTA->cImplTypes);
    ITypeInfo_ReleaseTypeAttr(pTI, pTA);

    hr = ITypeInfo_GetRefTypeOfImplType(pTI, 0, &href);
    ok(hr == S_OK, "hr %08x\n", hr);
    hr = ITypeInfo_GetRefTypeInfo(pTI, href, &pTI_p);
    ok(hr == S_OK, "hr %08x\n", hr);
    hr = ITypeInfo_GetTypeAttr(pTI_p, &pTA);
    ok(IsEqualGUID(&pTA->guid, &IID_IDispatch), "guid {%08x-....\n", pTA->guid.Data1);
    ITypeInfo_ReleaseTypeAttr(pTI_p, pTA);
    ITypeInfo_Release(pTI_p);

    /* Should have ten methods */
    hr = ITypeInfo_GetFuncDesc(pTI, 10, &pFD);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "hr %08x\n", hr);
    hr = ITypeInfo_GetFuncDesc(pTI, 9, &pFD);
    ok(hr == S_OK, "hr %08x\n", hr);
    ok(pFD->memid == 0x1236, "memid %08x\n", pFD->memid);
    ok(pFD->oVft == 36, "oVft %d\n", pFD->oVft);
    ITypeInfo_ReleaseFuncDesc(pTI, pFD);
}
    ITypeInfo_Release(pTI);


    /* ItestIF2 is an interface which derives from IUnknown */
    hr = ITypeLib_GetTypeInfoOfGuid(pTL, &IID_ItestIF2, &pTI);
    ok(hr == S_OK, "hr %08x\n", hr);

    hr = ITypeInfo_GetTypeAttr(pTI, &pTA);
    ok(hr == S_OK, "hr %08x\n", hr);
    ok(pTA->typekind == TKIND_INTERFACE, "kind %04x\n", pTA->typekind);
    ok(pTA->cbSizeVft == 24, "sizevft %d\n", pTA->cbSizeVft);
    ok(pTA->wTypeFlags == 0, "typeflags %x\n", pTA->wTypeFlags);
if(use_midl_tlb) {
    ok(pTA->cFuncs == 1, "cfuncs %d\n", pTA->cFuncs);
    ok(pTA->cImplTypes == 1, "cimpltypes %d\n", pTA->cImplTypes);
    ITypeInfo_ReleaseTypeAttr(pTI, pTA);

    /* Should have one method */
    hr = ITypeInfo_GetFuncDesc(pTI, 1, &pFD);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "hr %08x\n", hr);
    hr = ITypeInfo_GetFuncDesc(pTI, 0, &pFD);
    ok(hr == S_OK, "hr %08x\n", hr);
    ok(pFD->memid == 0x60020000, "memid %08x\n", pFD->memid);
    ok(pFD->oVft == 20, "oVft %d\n", pFD->oVft);
    ITypeInfo_ReleaseFuncDesc(pTI, pFD);
}
    ITypeInfo_Release(pTI);

    ITypeLib_Release(pTL);

    return;
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
    short oVft;
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
  /*#vtbl*/ 28, /*#func*/ 8,
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
      /*#param*/ 0, /*#opt*/ 0, /*vtbl*/ 4, /*#scodes*/ 0, /*flags*/ 0x1,
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
      /*#param*/ 0, /*#opt*/ 0, /*vtbl*/ 8, /*#scodes*/ 0, /*flags*/ 0x1,
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
      /*#param*/ 1, /*#opt*/ 0, /*vtbl*/ 12, /*#scodes*/ 0, /*flags*/ 0x1,
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
      /*#param*/ 3, /*#opt*/ 0, /*vtbl*/ 16, /*#scodes*/ 0, /*flags*/ 0x1,
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
      /*#param*/ 5, /*#opt*/ 0, /*vtbl*/ 20, /*#scodes*/ 0, /*flags*/ 0x1,
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
      /*#param*/ 8, /*#opt*/ 0, /*vtbl*/ 24, /*#scodes*/ 0, /*flags*/ 0x1,
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
      /*#param*/ 0, /*#opt*/ 0, /*vtbl*/ 28, /*#scodes*/ 0, /*flags*/ 0x0,
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
  /*#vtbl*/ 32, /*#func*/ 1,
  {
    {
      0x60020000, /*func*/ FUNC_PUREVIRTUAL, /*inv*/ INVOKE_FUNC, /*call*/ 0x4,
      /*#param*/ 0, /*#opt*/ 0, /*vtbl*/ 28, /*#scodes*/ 0, /*flags*/ 0x0,
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
        expect_int(typeattr->cbSizeVft, if_info->cbSizeVft);
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
            expect_int(desc->oVft, fn_info->oVft);
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

static const char *create_test_typelib(void)
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
    res = FindResource( GetModuleHandle(0), MAKEINTRESOURCE(2), "TYPELIB" );
    ok( res != 0, "couldn't find resource\n" );
    ptr = LockResource( LoadResource( GetModuleHandle(0), res ));
    WriteFile( file, ptr, SizeofResource( GetModuleHandle(0), res ), &written, NULL );
    ok( written == SizeofResource( GetModuleHandle(0), res ), "couldn't write resource\n" );
    CloseHandle( file );
    return filename;
}

START_TEST(typelib)
{
    const char *filename;

    ref_count_test(wszStdOle2);
    test_TypeComp();
    test_CreateDispTypeInfo();
    test_TypeInfo();
    test_QueryPathOfRegTypeLib();
    test_inheritance();

    if ((filename = create_test_typelib()))
    {
        test_dump_typelib( filename );
        DeleteFile( filename );
    }
}
