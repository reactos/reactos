/*
 * Copyright 2008 Jacek Caban for CodeWeavers
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

#define COBJMACROS
#define CONST_VTABLE

#include <ole2.h>
#include <dispex.h>
#include <activscp.h>

#include "wine/test.h"

static const CLSID CLSID_JScript =
    {0xf414c260,0x6ac0,0x11cf,{0xb6,0xd1,0x00,0xaa,0x00,0xbb,0xbb,0x58}};

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    expect_ ## func = TRUE

#define SET_CALLED(func) \
    called_ ## func = TRUE

#define CHECK_EXPECT2(func) \
    do { \
        ok(expect_ ##func, "unexpected call " #func "\n"); \
        called_ ## func = TRUE; \
    }while(0)

#define CHECK_EXPECT(func) \
    do { \
        CHECK_EXPECT2(func); \
        expect_ ## func = FALSE; \
    }while(0)

#define CHECK_CALLED(func) \
    do { \
        ok(called_ ## func, "expected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

DEFINE_EXPECT(global_propget_d);
DEFINE_EXPECT(global_propget_i);
DEFINE_EXPECT(global_propput_d);
DEFINE_EXPECT(global_propput_i);
DEFINE_EXPECT(global_success_d);
DEFINE_EXPECT(global_success_i);
DEFINE_EXPECT(global_notexists_d);
DEFINE_EXPECT(testobj_delete);
DEFINE_EXPECT(testobj_value);
DEFINE_EXPECT(testobj_prop_d);
DEFINE_EXPECT(testobj_noprop_d);
DEFINE_EXPECT(GetItemInfo_testVal);

#define DISPID_GLOBAL_TESTPROPGET   0x1000
#define DISPID_GLOBAL_TESTPROPPUT   0x1001
#define DISPID_GLOBAL_REPORTSUCCESS 0x1002
#define DISPID_GLOBAL_TRACE         0x1003
#define DISPID_GLOBAL_OK            0x1004
#define DISPID_GLOBAL_GETVT         0x1005
#define DISPID_GLOBAL_TESTOBJ       0x1006
#define DISPID_GLOBAL_NULL_BSTR     0x1007
#define DISPID_GLOBAL_NULL_DISP     0x1008
#define DISPID_GLOBAL_TESTTHIS      0x1009
#define DISPID_GLOBAL_TESTTHIS2     0x100a
#define DISPID_GLOBAL_INVOKEVERSION 0x100b

#define DISPID_TESTOBJ_PROP         0x2000

static const WCHAR testW[] = {'t','e','s','t',0};
static const CHAR testA[] = "test";
static const WCHAR test_valW[] = {'t','e','s','t','V','a','l',0};
static const CHAR test_valA[] = "testVal";

static BOOL strict_dispid_check;
static const char *test_name = "(null)";
static IDispatch *script_disp;
static int invoke_version;

static BSTR a2bstr(const char *str)
{
    BSTR ret;
    int len;

    len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    ret = SysAllocStringLen(NULL, len-1);
    MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len);

    return ret;
}

static int strcmp_wa(LPCWSTR strw, const char *stra)
{
    CHAR buf[512];
    WideCharToMultiByte(CP_ACP, 0, strw, -1, buf, sizeof(buf), 0, 0);
    return lstrcmpA(buf, stra);
}

#define test_grfdex(a,b) _test_grfdex(__LINE__,a,b)
static void _test_grfdex(unsigned line, DWORD grfdex, DWORD expect)
{
    expect |= invoke_version << 28;
    ok_(__FILE__,line)(grfdex == expect, "grfdex = %x, expected %x\n", grfdex, expect);
}

static HRESULT WINAPI DispatchEx_QueryInterface(IDispatchEx *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(riid, &IID_IUnknown)
       || IsEqualGUID(riid, &IID_IDispatch)
       || IsEqualGUID(riid, &IID_IDispatchEx))
        *ppv = iface;
    else
        return E_NOINTERFACE;

    return S_OK;
}

static ULONG WINAPI DispatchEx_AddRef(IDispatchEx *iface)
{
    return 2;
}

static ULONG WINAPI DispatchEx_Release(IDispatchEx *iface)
{
    return 1;
}

static HRESULT WINAPI DispatchEx_GetTypeInfoCount(IDispatchEx *iface, UINT *pctinfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetTypeInfo(IDispatchEx *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetIDsOfNames(IDispatchEx *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_Invoke(IDispatchEx *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_DeleteMemberByName(IDispatchEx *iface, BSTR bstrName, DWORD grfdex)
{
    ok(0, "unexpected call %s %x\n", wine_dbgstr_w(bstrName), grfdex);
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_DeleteMemberByDispID(IDispatchEx *iface, DISPID id)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetMemberProperties(IDispatchEx *iface, DISPID id, DWORD grfdexFetch, DWORD *pgrfdex)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetMemberName(IDispatchEx *iface, DISPID id, BSTR *pbstrName)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetNextDispID(IDispatchEx *iface, DWORD grfdex, DISPID id, DISPID *pid)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetNameSpaceParent(IDispatchEx *iface, IUnknown **ppunk)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testObj_GetDispID(IDispatchEx *iface, BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    if(!strcmp_wa(bstrName, "prop")) {
        CHECK_EXPECT(testobj_prop_d);
        test_grfdex(grfdex, fdexNameCaseSensitive);
        *pid = DISPID_TESTOBJ_PROP;
        return S_OK;
    }
    if(!strcmp_wa(bstrName, "noprop")) {
        CHECK_EXPECT(testobj_noprop_d);
        test_grfdex(grfdex, fdexNameCaseSensitive);
        return DISP_E_UNKNOWNNAME;
    }

    ok(0, "unexpected name %s\n", wine_dbgstr_w(bstrName));
    return E_NOTIMPL;
}

static HRESULT WINAPI testObj_InvokeEx(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    switch(id) {
    case DISPID_VALUE:
        CHECK_EXPECT(testobj_value);

        ok(wFlags == INVOKE_PROPERTYGET, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(!pdp->rgvarg, "rgvarg != NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(!pdp->cArgs, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pvarRes != NULL, "pvarRes == NULL\n");
        ok(V_VT(pvarRes) ==  VT_EMPTY, "V_VT(pvarRes) = %d\n", V_VT(pvarRes));
        ok(pei != NULL, "pei == NULL\n");

        V_VT(pvarRes) = VT_I4;
        V_I4(pvarRes) = 1;
        return S_OK;
    }

    ok(0, "unexpected call %x\n", id);
    return DISP_E_MEMBERNOTFOUND;
}

static HRESULT WINAPI testObj_DeleteMemberByName(IDispatchEx *iface, BSTR bstrName, DWORD grfdex)
{
    CHECK_EXPECT(testobj_delete);

    ok(!strcmp_wa(bstrName, "deleteTest"), "unexpected name %s\n", wine_dbgstr_w(bstrName));
    test_grfdex(grfdex, fdexNameCaseSensitive);
    return S_OK;
}

static IDispatchExVtbl testObjVtbl = {
    DispatchEx_QueryInterface,
    DispatchEx_AddRef,
    DispatchEx_Release,
    DispatchEx_GetTypeInfoCount,
    DispatchEx_GetTypeInfo,
    DispatchEx_GetIDsOfNames,
    DispatchEx_Invoke,
    testObj_GetDispID,
    testObj_InvokeEx,
    testObj_DeleteMemberByName,
    DispatchEx_DeleteMemberByDispID,
    DispatchEx_GetMemberProperties,
    DispatchEx_GetMemberName,
    DispatchEx_GetNextDispID,
    DispatchEx_GetNameSpaceParent
};

static IDispatchEx testObj = { &testObjVtbl };

static HRESULT WINAPI Global_GetDispID(IDispatchEx *iface, BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    if(!strcmp_wa(bstrName, "ok")) {
        test_grfdex(grfdex, fdexNameCaseSensitive);
        *pid = DISPID_GLOBAL_OK;
        return S_OK;
    }
    if(!strcmp_wa(bstrName, "trace")) {
        test_grfdex(grfdex, fdexNameCaseSensitive);
        *pid = DISPID_GLOBAL_TRACE;
        return S_OK;
    }
    if(!strcmp_wa(bstrName, "reportSuccess")) {
        CHECK_EXPECT(global_success_d);
        test_grfdex(grfdex, fdexNameCaseSensitive);
        *pid = DISPID_GLOBAL_REPORTSUCCESS;
        return S_OK;
    }
    if(!strcmp_wa(bstrName, "testPropGet")) {
        CHECK_EXPECT(global_propget_d);
        test_grfdex(grfdex, fdexNameCaseSensitive);
        *pid = DISPID_GLOBAL_TESTPROPGET;
        return S_OK;
    }
    if(!strcmp_wa(bstrName, "testPropPut")) {
        CHECK_EXPECT(global_propput_d);
        test_grfdex(grfdex, fdexNameCaseSensitive);
        *pid = DISPID_GLOBAL_TESTPROPPUT;
        return S_OK;
    }
    if(!strcmp_wa(bstrName, "getVT")) {
        test_grfdex(grfdex, fdexNameCaseSensitive);
        *pid = DISPID_GLOBAL_GETVT;
        return S_OK;
    }
    if(!strcmp_wa(bstrName, "testObj")) {
        test_grfdex(grfdex, fdexNameCaseSensitive);
        *pid = DISPID_GLOBAL_TESTOBJ;
        return S_OK;
    }
    if(!strcmp_wa(bstrName, "createNullBSTR")) {
        *pid = DISPID_GLOBAL_NULL_BSTR;
        return S_OK;
    }
    if(!strcmp_wa(bstrName, "nullDisp")) {
        *pid = DISPID_GLOBAL_NULL_DISP;
        return S_OK;
    }
    if(!strcmp_wa(bstrName, "notExists")) {
        CHECK_EXPECT(global_notexists_d);
        test_grfdex(grfdex, fdexNameCaseSensitive);
        return DISP_E_UNKNOWNNAME;
    }

    if(!strcmp_wa(bstrName, "testThis")) {
        test_grfdex(grfdex, fdexNameCaseSensitive);
        *pid = DISPID_GLOBAL_TESTTHIS;
        return S_OK;
    }

    if(!strcmp_wa(bstrName, "testThis2")) {
        test_grfdex(grfdex, fdexNameCaseSensitive);
        *pid = DISPID_GLOBAL_TESTTHIS2;
        return S_OK;
    }

    if(!strcmp_wa(bstrName, "invokeVersion")) {
        test_grfdex(grfdex, fdexNameCaseSensitive);
        *pid = DISPID_GLOBAL_INVOKEVERSION;
        return S_OK;
    }

    if(strict_dispid_check)
        ok(0, "unexpected call %s\n", wine_dbgstr_w(bstrName));
    return DISP_E_UNKNOWNNAME;
}

static HRESULT WINAPI Global_InvokeEx(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    switch(id) {
    case DISPID_GLOBAL_OK:
        ok(wFlags == INVOKE_FUNC || wFlags == (INVOKE_FUNC|INVOKE_PROPERTYGET), "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(pdp->cArgs == 2, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        if(wFlags & INVOKE_PROPERTYGET)
            ok(pvarRes != NULL, "pvarRes == NULL\n");
        else
            ok(!pvarRes, "pvarRes != NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        ok(V_VT(pdp->rgvarg) == VT_BSTR, "V_VT(psp->rgvargs) = %d\n", V_VT(pdp->rgvarg));
        ok(V_VT(pdp->rgvarg+1) == VT_BOOL, "V_VT(psp->rgvargs+1) = %d\n", V_VT(pdp->rgvarg));
        ok(V_BOOL(pdp->rgvarg+1), "%s: %s\n", test_name, wine_dbgstr_w(V_BSTR(pdp->rgvarg)));

        return S_OK;

     case DISPID_GLOBAL_TRACE:
        ok(wFlags == INVOKE_FUNC, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(pdp->cArgs == 1, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(!pvarRes, "pvarRes != NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        ok(V_VT(pdp->rgvarg) == VT_BSTR, "V_VT(psp->rgvargs) = %d\n", V_VT(pdp->rgvarg));
        if(V_VT(pdp->rgvarg) == VT_BSTR)
            trace("%s: %s\n", test_name, wine_dbgstr_w(V_BSTR(pdp->rgvarg)));

        return S_OK;

    case DISPID_GLOBAL_REPORTSUCCESS:
        CHECK_EXPECT(global_success_i);

        ok(wFlags == INVOKE_FUNC, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(pdp->cArgs == 0, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(!pvarRes, "pvarRes != NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        return S_OK;

     case DISPID_GLOBAL_TESTPROPGET:
        CHECK_EXPECT(global_propget_i);

        ok(wFlags == INVOKE_PROPERTYGET, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(!pdp->rgvarg, "rgvarg != NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(!pdp->cArgs, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pvarRes != NULL, "pvarRes == NULL\n");
        ok(V_VT(pvarRes) ==  VT_EMPTY, "V_VT(pvarRes) = %d\n", V_VT(pvarRes));
        ok(pei != NULL, "pei == NULL\n");

        V_VT(pvarRes) = VT_I4;
        V_I4(pvarRes) = 1;

        return S_OK;

    case DISPID_GLOBAL_TESTPROPPUT:
        CHECK_EXPECT(global_propput_i);

        ok(wFlags == INVOKE_PROPERTYPUT, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(pdp->rgdispidNamedArgs != NULL, "rgdispidNamedArgs == NULL\n");
        ok(pdp->cArgs == 1, "cArgs = %d\n", pdp->cArgs);
        ok(pdp->cNamedArgs == 1, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pdp->rgdispidNamedArgs[0] == DISPID_PROPERTYPUT, "pdp->rgdispidNamedArgs[0] = %d\n", pdp->rgdispidNamedArgs[0]);
        ok(!pvarRes, "pvarRes != NULL\n");

        ok(V_VT(pdp->rgvarg) == VT_I4, "V_VT(pdp->rgvarg)=%d\n", V_VT(pdp->rgvarg));
        ok(V_I4(pdp->rgvarg) == 1, "V_I4(pdp->rgvarg)=%d\n", V_I4(pdp->rgvarg));
        return S_OK;

     case DISPID_GLOBAL_GETVT:
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(pdp->cArgs == 1, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pvarRes != NULL, "pvarRes == NULL\n");
        ok(V_VT(pvarRes) ==  VT_EMPTY, "V_VT(pvarRes) = %d\n", V_VT(pvarRes));
        ok(pei != NULL, "pei == NULL\n");

        V_VT(pvarRes) = VT_BSTR;
        switch(V_VT(pdp->rgvarg)) {
        case VT_EMPTY:
            V_BSTR(pvarRes) = a2bstr("VT_EMPTY");
            break;
        case VT_NULL:
            V_BSTR(pvarRes) = a2bstr("VT_NULL");
            break;
        case VT_I4:
            V_BSTR(pvarRes) = a2bstr("VT_I4");
            break;
        case VT_R8:
            V_BSTR(pvarRes) = a2bstr("VT_R8");
            break;
        case VT_BSTR:
            V_BSTR(pvarRes) = a2bstr("VT_BSTR");
            break;
        case VT_DISPATCH:
            V_BSTR(pvarRes) = a2bstr("VT_DISPATCH");
            break;
        case VT_BOOL:
            V_BSTR(pvarRes) = a2bstr("VT_BOOL");
            break;
        default:
            ok(0, "unknown vt %d\n", V_VT(pdp->rgvarg));
            return E_FAIL;
        }

        return S_OK;

    case DISPID_GLOBAL_TESTOBJ:
        ok(wFlags == INVOKE_PROPERTYGET, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(!pdp->rgvarg, "rgvarg != NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(!pdp->cArgs, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pvarRes != NULL, "pvarRes == NULL\n");
        ok(V_VT(pvarRes) ==  VT_EMPTY, "V_VT(pvarRes) = %d\n", V_VT(pvarRes));
        ok(pei != NULL, "pei == NULL\n");

        V_VT(pvarRes) = VT_DISPATCH;
        V_DISPATCH(pvarRes) = (IDispatch*)&testObj;
        return S_OK;

    case DISPID_GLOBAL_NULL_BSTR:
        if(pvarRes) {
            V_VT(pvarRes) = VT_BSTR;
            V_BSTR(pvarRes) = NULL;
        }
        return S_OK;

    case DISPID_GLOBAL_NULL_DISP:
        ok(wFlags == INVOKE_PROPERTYGET, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(!pdp->rgvarg, "rgvarg != NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(!pdp->cArgs, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pvarRes != NULL, "pvarRes == NULL\n");
        ok(V_VT(pvarRes) ==  VT_EMPTY, "V_VT(pvarRes) = %d\n", V_VT(pvarRes));
        ok(pei != NULL, "pei == NULL\n");

        V_VT(pvarRes) = VT_DISPATCH;
        V_DISPATCH(pvarRes) = NULL;
        return S_OK;

    case DISPID_GLOBAL_TESTTHIS:
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(pdp->cArgs == 1, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pvarRes == NULL, "pvarRes != NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        ok(V_VT(pdp->rgvarg) == VT_DISPATCH, "V_VT(arg) = %d\n", V_VT(pdp->rgvarg));
        ok(V_DISPATCH(pdp->rgvarg) == (IDispatch*)iface, "disp != iface\n");

        return S_OK;

    case DISPID_GLOBAL_TESTTHIS2:
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(pdp->cArgs == 1, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pvarRes == NULL, "pvarRes != NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        ok(V_VT(pdp->rgvarg) == VT_DISPATCH, "V_VT(arg) = %d\n", V_VT(pdp->rgvarg));
        ok(V_DISPATCH(pdp->rgvarg) != (IDispatch*)iface, "disp == iface\n");
        ok(V_DISPATCH(pdp->rgvarg) == script_disp, "disp != script_disp\n");

        return S_OK;

     case DISPID_GLOBAL_INVOKEVERSION:
        ok(wFlags == INVOKE_PROPERTYGET, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(!pdp->rgvarg, "rgvarg != NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(!pdp->cArgs, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pvarRes != NULL, "pvarRes == NULL\n");
        ok(V_VT(pvarRes) ==  VT_EMPTY, "V_VT(pvarRes) = %d\n", V_VT(pvarRes));
        ok(pei != NULL, "pei == NULL\n");

        V_VT(pvarRes) = VT_I4;
        V_I4(pvarRes) = invoke_version;

        return S_OK;

    }

    ok(0, "unexpected call %x\n", id);
    return DISP_E_MEMBERNOTFOUND;
}

static IDispatchExVtbl GlobalVtbl = {
    DispatchEx_QueryInterface,
    DispatchEx_AddRef,
    DispatchEx_Release,
    DispatchEx_GetTypeInfoCount,
    DispatchEx_GetTypeInfo,
    DispatchEx_GetIDsOfNames,
    DispatchEx_Invoke,
    Global_GetDispID,
    Global_InvokeEx,
    DispatchEx_DeleteMemberByName,
    DispatchEx_DeleteMemberByDispID,
    DispatchEx_GetMemberProperties,
    DispatchEx_GetMemberName,
    DispatchEx_GetNextDispID,
    DispatchEx_GetNameSpaceParent
};

static IDispatchEx Global = { &GlobalVtbl };

static HRESULT WINAPI ActiveScriptSite_QueryInterface(IActiveScriptSite *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid))
        *ppv = iface;
    else if(IsEqualGUID(&IID_IActiveScriptSite, riid))
        *ppv = iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI ActiveScriptSite_AddRef(IActiveScriptSite *iface)
{
    return 2;
}

static ULONG WINAPI ActiveScriptSite_Release(IActiveScriptSite *iface)
{
    return 1;
}

static HRESULT WINAPI ActiveScriptSite_GetLCID(IActiveScriptSite *iface, LCID *plcid)
{
    *plcid = GetUserDefaultLCID();
    return S_OK;
}

static HRESULT WINAPI ActiveScriptSite_GetItemInfo(IActiveScriptSite *iface, LPCOLESTR pstrName,
        DWORD dwReturnMask, IUnknown **ppiunkItem, ITypeInfo **ppti)
{
    ok(dwReturnMask == SCRIPTINFO_IUNKNOWN, "unexpected dwReturnMask %x\n", dwReturnMask);
    ok(!ppti, "ppti != NULL\n");

    if(!strcmp_wa(pstrName, test_valA))
        CHECK_EXPECT(GetItemInfo_testVal);
    else if(strcmp_wa(pstrName, testA))
        ok(0, "unexpected pstrName %s\n", wine_dbgstr_w(pstrName));

    *ppiunkItem = (IUnknown*)&Global;
    return S_OK;
}

static HRESULT WINAPI ActiveScriptSite_GetDocVersionString(IActiveScriptSite *iface, BSTR *pbstrVersion)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_OnScriptTerminate(IActiveScriptSite *iface,
        const VARIANT *pvarResult, const EXCEPINFO *pexcepinfo)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_OnStateChange(IActiveScriptSite *iface, SCRIPTSTATE ssScriptState)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_OnScriptError(IActiveScriptSite *iface, IActiveScriptError *pscripterror)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_OnEnterScript(IActiveScriptSite *iface)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_OnLeaveScript(IActiveScriptSite *iface)
{
    return E_NOTIMPL;
}

#undef ACTSCPSITE_THIS

static const IActiveScriptSiteVtbl ActiveScriptSiteVtbl = {
    ActiveScriptSite_QueryInterface,
    ActiveScriptSite_AddRef,
    ActiveScriptSite_Release,
    ActiveScriptSite_GetLCID,
    ActiveScriptSite_GetItemInfo,
    ActiveScriptSite_GetDocVersionString,
    ActiveScriptSite_OnScriptTerminate,
    ActiveScriptSite_OnStateChange,
    ActiveScriptSite_OnScriptError,
    ActiveScriptSite_OnEnterScript,
    ActiveScriptSite_OnLeaveScript
};

static IActiveScriptSite ActiveScriptSite = { &ActiveScriptSiteVtbl };

static HRESULT set_script_prop(IActiveScript *engine, DWORD property, VARIANT *val)
{
    IActiveScriptProperty *script_prop;
    HRESULT hres;

    hres = IActiveScript_QueryInterface(engine, &IID_IActiveScriptProperty,
            (void**)&script_prop);
    ok(hres == S_OK, "Could not get IActiveScriptProperty iface: %08x\n", hres);

    hres = IActiveScriptProperty_SetProperty(script_prop, property, NULL, val);
    IActiveScriptProperty_Release(script_prop);

    return hres;
}

static IActiveScript *create_script(void)
{
    IActiveScript *script;
    VARIANT v;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_JScript, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IActiveScript, (void**)&script);
    ok(hres == S_OK, "CoCreateInstance failed: %08x\n", hres);

    V_VT(&v) = VT_I4;
    V_I4(&v) = invoke_version;
    hres = set_script_prop(script, SCRIPTPROP_INVOKEVERSIONING, &v);
    ok(hres == S_OK || broken(hres == E_NOTIMPL), "SetProperty(SCRIPTPROP_INVOKEVERSIONING) failed: %08x\n", hres);
    if(invoke_version && FAILED(hres)) {
        IActiveScript_Release(script);
        return NULL;
    }

    return script;
}

static HRESULT parse_script(DWORD flags, BSTR script_str)
{
    IActiveScriptParse *parser;
    IActiveScript *engine;
    HRESULT hres;

    engine = create_script();
    if(!engine)
        return S_OK;

    hres = IActiveScript_QueryInterface(engine, &IID_IActiveScriptParse, (void**)&parser);
    ok(hres == S_OK, "Could not get IActiveScriptParse: %08x\n", hres);
    if (FAILED(hres))
    {
        IActiveScript_Release(engine);
        return hres;
    }

    hres = IActiveScriptParse64_InitNew(parser);
    ok(hres == S_OK, "InitNew failed: %08x\n", hres);

    hres = IActiveScript_SetScriptSite(engine, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08x\n", hres);

    hres = IActiveScript_AddNamedItem(engine, testW,
            SCRIPTITEM_ISVISIBLE|SCRIPTITEM_ISSOURCE|flags);
    ok(hres == S_OK, "AddNamedItem failed: %08x\n", hres);

    hres = IActiveScript_SetScriptState(engine, SCRIPTSTATE_STARTED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_STARTED) failed: %08x\n", hres);

    hres = IActiveScript_GetScriptDispatch(engine, NULL, &script_disp);
    ok(hres == S_OK, "GetScriptDispatch failed: %08x\n", hres);
    ok(script_disp != NULL, "script_disp == NULL\n");
    ok(script_disp != (IDispatch*)&Global, "script_disp == Global\n");

    hres = IActiveScriptParse64_ParseScriptText(parser, script_str, NULL, NULL, NULL, 0, 0, 0, NULL, NULL);

    IDispatch_Release(script_disp);
    IActiveScript_Release(engine);
    IUnknown_Release(parser);

    return hres;
}

static HRESULT parse_htmlscript(BSTR script_str)
{
    IActiveScriptParse *parser;
    IActiveScript *engine;
    HRESULT hres;
    BSTR tmp = a2bstr("</SCRIPT>");

    engine = create_script();
    if(!engine)
        return E_FAIL;

    hres = IActiveScript_QueryInterface(engine, &IID_IActiveScriptParse, (void**)&parser);
    ok(hres == S_OK, "Could not get IActiveScriptParse: %08x\n", hres);
    if (FAILED(hres))
    {
        IActiveScript_Release(engine);
        return E_FAIL;
    }

    hres = IActiveScriptParse64_InitNew(parser);
    ok(hres == S_OK, "InitNew failed: %08x\n", hres);

    hres = IActiveScript_SetScriptSite(engine, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08x\n", hres);

    hres = IActiveScript_AddNamedItem(engine, testW,
            SCRIPTITEM_ISVISIBLE|SCRIPTITEM_ISSOURCE|SCRIPTITEM_GLOBALMEMBERS);
    ok(hres == S_OK, "AddNamedItem failed: %08x\n", hres);

    hres = IActiveScript_SetScriptState(engine, SCRIPTSTATE_STARTED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_STARTED) failed: %08x\n", hres);

    hres = IActiveScriptParse64_ParseScriptText(parser, script_str, NULL, NULL, tmp, 0, 0, 0, NULL, NULL);

    IActiveScript_Release(engine);
    IUnknown_Release(parser);
    SysFreeString(tmp);

    return hres;
}

static void parse_script_af(DWORD flags, const char *src)
{
    BSTR tmp;
    HRESULT hres;

    tmp = a2bstr(src);
    hres = parse_script(flags, tmp);
    SysFreeString(tmp);
    ok(hres == S_OK, "parse_script failed: %08x\n", hres);
}

static void parse_script_a(const char *src)
{
    parse_script_af(SCRIPTITEM_GLOBALMEMBERS, src);
}

static HRESULT parse_htmlscript_a(const char *src)
{
    HRESULT hres;
    BSTR tmp = a2bstr(src);
    hres = parse_htmlscript(tmp);
    SysFreeString(tmp);

    return hres;
}

static BSTR get_script_from_file(const char *filename)
{
    DWORD size, len;
    HANDLE file, map;
    const char *file_map;
    BSTR ret;

    file = CreateFileA(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
    if(file == INVALID_HANDLE_VALUE) {
        trace("Could not open file: %u\n", GetLastError());
        return NULL;
    }

    size = GetFileSize(file, NULL);

    map = CreateFileMapping(file, NULL, PAGE_READONLY, 0, 0, NULL);
    CloseHandle(file);
    if(map == INVALID_HANDLE_VALUE) {
        trace("Could not create file mapping: %u\n", GetLastError());
        return NULL;
    }

    file_map = MapViewOfFile(map, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(map);
    if(!file_map) {
        trace("MapViewOfFile failed: %u\n", GetLastError());
        return NULL;
    }

    len = MultiByteToWideChar(CP_ACP, 0, file_map, size, NULL, 0);
    ret = SysAllocStringLen(NULL, len);
    MultiByteToWideChar(CP_ACP, 0, file_map, size, ret, len);

    UnmapViewOfFile(file_map);

    return ret;
}

static void run_from_file(const char *filename)
{
    BSTR script_str;
    HRESULT hres;

    script_str = get_script_from_file(filename);
    if(!script_str)
        return;

    strict_dispid_check = FALSE;
    hres = parse_script(SCRIPTITEM_GLOBALMEMBERS, script_str);
    SysFreeString(script_str);
    ok(hres == S_OK, "parse_script failed: %08x\n", hres);
}

static void run_from_res(const char *name)
{
    const char *data;
    DWORD size, len;
    BSTR str;
    HRSRC src;
    HRESULT hres;

    strict_dispid_check = FALSE;
    test_name = name;

    src = FindResourceA(NULL, name, (LPCSTR)40);
    ok(src != NULL, "Could not find resource %s\n", name);

    size = SizeofResource(NULL, src);
    data = LoadResource(NULL, src);

    len = MultiByteToWideChar(CP_ACP, 0, data, size, NULL, 0);
    str = SysAllocStringLen(NULL, len);
    len = MultiByteToWideChar(CP_ACP, 0, data, size, str, len);

    SET_EXPECT(global_success_d);
    SET_EXPECT(global_success_i);
    hres = parse_script(SCRIPTITEM_GLOBALMEMBERS, str);
    CHECK_CALLED(global_success_d);
    CHECK_CALLED(global_success_i);

    ok(hres == S_OK, "parse_script failed: %08x\n", hres);
    SysFreeString(str);
}

static void test_isvisible(BOOL global_members)
{
    IActiveScriptParse *parser;
    IActiveScript *engine;
    HRESULT hres;

    static const WCHAR script_textW[] =
        {'v','a','r',' ','v',' ','=',' ','t','e','s','t','V','a','l',';',0};

    engine = create_script();
    if(!engine)
        return;

    hres = IActiveScript_QueryInterface(engine, &IID_IActiveScriptParse, (void**)&parser);
    ok(hres == S_OK, "Could not get IActiveScriptParse: %08x\n", hres);
    if (FAILED(hres))
    {
        IActiveScript_Release(engine);
        return;
    }

    hres = IActiveScriptParse64_InitNew(parser);
    ok(hres == S_OK, "InitNew failed: %08x\n", hres);

    hres = IActiveScript_SetScriptSite(engine, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08x\n", hres);

    if(global_members)
        SET_EXPECT(GetItemInfo_testVal);
    hres = IActiveScript_AddNamedItem(engine, test_valW,
            SCRIPTITEM_ISVISIBLE|SCRIPTITEM_ISSOURCE|
            (global_members ? SCRIPTITEM_GLOBALMEMBERS : 0));
    ok(hres == S_OK, "AddNamedItem failed: %08x\n", hres);
    if(global_members)
        CHECK_CALLED(GetItemInfo_testVal);

    hres = IActiveScript_SetScriptState(engine, SCRIPTSTATE_STARTED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_STARTED) failed: %08x\n", hres);

    if(!global_members)
        SET_EXPECT(GetItemInfo_testVal);
    hres = IActiveScriptParse64_ParseScriptText(parser, script_textW, NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08x\n", hres);
    if(!global_members)
        CHECK_CALLED(GetItemInfo_testVal);

    hres = IActiveScriptParse64_ParseScriptText(parser, script_textW, NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08x\n", hres);

    IActiveScript_Release(engine);
    IUnknown_Release(parser);
}

static void run_tests(void)
{
    HRESULT hres;

    if(invoke_version) {
        IActiveScript *script;

        script = create_script();
        if(!script) {
            win_skip("Could not create script\n");
            return;
        }
        IActiveScript_Release(script);
    }

    strict_dispid_check = TRUE;

    parse_script_a("");
    parse_script_a("/* empty */ ;");

    SET_EXPECT(global_propget_d);
    SET_EXPECT(global_propget_i);
    parse_script_a("testPropGet;");
    CHECK_CALLED(global_propget_d);
    CHECK_CALLED(global_propget_i);

    SET_EXPECT(global_propput_d);
    SET_EXPECT(global_propput_i);
    parse_script_a("testPropPut = 1;");
    CHECK_CALLED(global_propput_d);
    CHECK_CALLED(global_propput_i);

    SET_EXPECT(global_success_d);
    SET_EXPECT(global_success_i);
    parse_script_a("reportSuccess();");
    CHECK_CALLED(global_success_d);
    CHECK_CALLED(global_success_i);

    SET_EXPECT(testobj_delete);
    parse_script_a("delete testObj.deleteTest;");
    CHECK_CALLED(testobj_delete);

    parse_script_a("ok(typeof(test) === 'object', \"typeof(test) != 'object'\");");

    parse_script_a("function reportSuccess() {}; reportSuccess();");

    SET_EXPECT(global_propget_d);
    parse_script_a("var testPropGet");
    CHECK_CALLED(global_propget_d);

    SET_EXPECT(global_notexists_d);
    parse_script_a("var notExists; notExists = 1;");
    CHECK_CALLED(global_notexists_d);

    parse_script_a("function f() { var testPropGet; }");

    parse_script_a("ok((testObj instanceof Object) === false, 'testObj is instance of Object');");

    SET_EXPECT(testobj_prop_d);
    parse_script_a("ok(('prop' in testObj) === true, 'prop is not in testObj');");
    CHECK_CALLED(testobj_prop_d);

    SET_EXPECT(testobj_noprop_d);
    parse_script_a("ok(('noprop' in testObj) === false, 'noprop is in testObj');");
    CHECK_CALLED(testobj_noprop_d);

    SET_EXPECT(testobj_value);
    parse_script_a("ok(String(testObj) === '1', 'wrong testObj value');");
    CHECK_CALLED(testobj_value);

    SET_EXPECT(testobj_value);
    parse_script_a("ok(String.prototype.concat.call(testObj, ' OK') === '1 OK', 'wrong concat result');");
    CHECK_CALLED(testobj_value);

    SET_EXPECT(global_propget_d);
    SET_EXPECT(global_propget_i);
    parse_script_a("this.testPropGet;");
    CHECK_CALLED(global_propget_d);
    CHECK_CALLED(global_propget_i);

    SET_EXPECT(global_propget_d);
    SET_EXPECT(global_propget_i);
    parse_script_a("(function () { this.testPropGet; })();");
    CHECK_CALLED(global_propget_d);
    CHECK_CALLED(global_propget_i);

    parse_script_a("testThis(this);");
    parse_script_a("(function () { testThis(this); })();");

    run_from_res("lang.js");
    run_from_res("api.js");
    run_from_res("regexp.js");

    test_isvisible(FALSE);
    test_isvisible(TRUE);

    parse_script_af(0, "test.testThis2(this);");
    parse_script_af(0, "(function () { test.testThis2(this); })();");

    hres = parse_htmlscript_a("<!--");
    ok(hres == S_OK, "ParseScriptText failed: %08x\n", hres);
    hres = parse_htmlscript_a("-->");
    ok(hres == S_OK, "ParseScriptText failed: %08x\n", hres);
    hres = parse_htmlscript_a("<!--\nvar a=1;\n-->\n");
    ok(hres == S_OK, "ParseScriptText failed: %08x\n", hres);
    hres = parse_htmlscript_a("<!--\n<!-- ignore this\n-->\n");
    ok(hres == S_OK, "ParseScriptText failed: %08x\n", hres);
    hres = parse_htmlscript_a("var a=1;\nif(a-->0) a=5;\n");
    ok(hres == S_OK, "ParseScriptText failed: %08x\n", hres);
    hres = parse_htmlscript_a("var a=1;\nif(a\n-->0) a=5;\n");
    ok(hres != S_OK, "ParseScriptText have not failed\n");
}

static BOOL check_jscript(void)
{
    BSTR str;
    HRESULT hres;

    str = a2bstr("if(!('localeCompare' in String.prototype)) throw 1;");
    hres = parse_script(0, str);
    SysFreeString(str);

    return hres == S_OK;
}

START_TEST(run)
{
    int argc;
    char **argv;

    argc = winetest_get_mainargs(&argv);

    CoInitialize(NULL);

    if(!check_jscript()) {
        win_skip("Broken engine, probably too old\n");
    }else if(argc > 2) {
        run_from_file(argv[2]);
    }else {
        trace("invoke version 0\n");
        invoke_version = 0;
        run_tests();

        trace("invoke version 2\n");
        invoke_version = 2;
        run_tests();
    }

    CoUninitialize();
}
