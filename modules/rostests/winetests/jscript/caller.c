/*
 * Copyright 2012 Jacek Caban for CodeWeavers
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
#include <objsafe.h>

#include "wine/test.h"

#ifdef _WIN64

#define IActiveScriptParse_QueryInterface IActiveScriptParse64_QueryInterface
#define IActiveScriptParse_Release IActiveScriptParse64_Release
#define IActiveScriptParse_InitNew IActiveScriptParse64_InitNew
#define IActiveScriptParse_ParseScriptText IActiveScriptParse64_ParseScriptText

#else

#define IActiveScriptParse_QueryInterface IActiveScriptParse32_QueryInterface
#define IActiveScriptParse_Release IActiveScriptParse32_Release
#define IActiveScriptParse_InitNew IActiveScriptParse32_InitNew
#define IActiveScriptParse_ParseScriptText IActiveScriptParse32_ParseScriptText

#endif

static const CLSID CLSID_JScript =
    {0xf414c260,0x6ac0,0x11cf,{0xb6,0xd1,0x00,0xaa,0x00,0xbb,0xbb,0x58}};

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    expect_ ## func = TRUE

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

#define CLEAR_CALLED(func) \
    expect_ ## func = called_ ## func = FALSE

DEFINE_EXPECT(sp_caller_QI_NULL);
DEFINE_EXPECT(site_QI_NULL);
DEFINE_EXPECT(testArgConv);
DEFINE_EXPECT(testGetCaller);
DEFINE_EXPECT(testGetCallerJS);
DEFINE_EXPECT(testGetCallerNested);
DEFINE_EXPECT(OnEnterScript);
DEFINE_EXPECT(OnLeaveScript);

static IActiveScriptParse *active_script_parser;
static IVariantChangeType *script_change_type;
static IDispatch *stored_obj;
static IServiceProvider *test_get_caller_sp;

#define DISPID_TEST_TESTARGCONV      0x1000
#define DISPID_TEST_TESTGETCALLER    0x1001
#define DISPID_TEST_TESTGETCALLERJS  0x1002
#define DISPID_TEST_TESTGETCALLERNESTED 0x1003

typedef struct {
    int int_result;
    const WCHAR *str_result;
    VARIANT_BOOL bool_result;
    int test_double;
    double double_result;
} conv_results_t;

#define call_change_type(a,b,c,d) _call_change_type(__LINE__,a,b,c,d)
static void _call_change_type(unsigned line, IVariantChangeType *change_type, VARIANT *dst, VARIANT *src, VARTYPE vt)
{
    HRESULT hres;

    VariantInit(dst);
    if(V_VT(src) != vt && vt != VT_BOOL && (V_VT(src) == VT_DISPATCH || V_VT(src) == VT_UNKNOWN)) {
        SET_EXPECT(OnEnterScript);
        SET_EXPECT(OnLeaveScript);
    }
    hres = IVariantChangeType_ChangeType(change_type, dst, src, 0, vt);
    ok_(__FILE__,line)(hres == S_OK, "ChangeType(%d) failed: %08lx\n", vt, hres);
    ok_(__FILE__,line)(V_VT(dst) == vt, "V_VT(dst) = %d\n", V_VT(dst));
    if(V_VT(src) != vt && vt != VT_BOOL && (V_VT(src) == VT_DISPATCH || V_VT(src) == VT_UNKNOWN)) {
        CHECK_CALLED(OnEnterScript);
        CHECK_CALLED(OnLeaveScript);
    }
}

#define change_type_fail(a,b,c,d) _change_type_fail(__LINE__,a,b,c,d)
static void _change_type_fail(unsigned line, IVariantChangeType *change_type, VARIANT *src, VARTYPE vt, HRESULT exhres)
{
    VARIANT v;
    HRESULT hres;

    V_VT(&v) = VT_EMPTY;
    hres = IVariantChangeType_ChangeType(change_type, &v, src, 0, vt);
    ok_(__FILE__,line)(hres == exhres, "ChangeType failed: %08lx, expected %08lx [%d]\n", hres, exhres, V_VT(src));
}

static void test_change_type(IVariantChangeType *change_type, VARIANT *src, const conv_results_t *ex)
{
    VARIANT v;

    call_change_type(change_type, &v, src, VT_I4);
    ok(V_I4(&v) == ex->int_result, "V_I4(v) = %ld, expected %d\n", V_I4(&v), ex->int_result);

    call_change_type(change_type, &v, src, VT_UI2);
    ok(V_UI2(&v) == (UINT16)ex->int_result, "V_UI2(v) = %u, expected %u\n", V_UI2(&v), (UINT16)ex->int_result);

    call_change_type(change_type, &v, src, VT_BSTR);
    ok(!lstrcmpW(V_BSTR(&v), ex->str_result), "V_BSTR(v) = %s, expected %s\n", wine_dbgstr_w(V_BSTR(&v)), wine_dbgstr_w(ex->str_result));
    VariantClear(&v);

    call_change_type(change_type, &v, src, VT_BOOL);
    ok(V_BOOL(&v) == ex->bool_result, "V_BOOL(v) = %x, expected %x\n", V_BOOL(&v), ex->bool_result);

    if(ex->test_double) {
        call_change_type(change_type, &v, src, VT_R8);
        ok(V_R8(&v) == ex->double_result, "V_R8(v) = %lf, expected %lf\n", V_R8(&v), ex->double_result);

        call_change_type(change_type, &v, src, VT_R4);
        ok(V_R4(&v) == (float)ex->double_result, "V_R4(v) = %f, expected %f\n", V_R4(&v), (float)ex->double_result);
    }

    if(V_VT(src) == VT_NULL)
        call_change_type(change_type, &v, src, VT_NULL);
    else
        change_type_fail(change_type, src, VT_NULL, E_NOTIMPL);

    if(V_VT(src) == VT_EMPTY)
        call_change_type(change_type, &v, src, VT_EMPTY);
    else
        change_type_fail(change_type, src, VT_EMPTY, E_NOTIMPL);

    call_change_type(change_type, &v, src, VT_I2);
    ok(V_I2(&v) == (INT16)ex->int_result, "V_I2(v) = %d, expected %d\n", V_I2(&v), ex->int_result);

    if(V_VT(src) != VT_UNKNOWN)
        change_type_fail(change_type, src, VT_UNKNOWN, E_NOTIMPL);
    else {
        call_change_type(change_type, &v, src, VT_UNKNOWN);
        ok(V_UNKNOWN(&v) == V_UNKNOWN(src), "V_UNKNOWN(v) != V_UNKNOWN(src)\n");
        VariantClear(&v);
    }

    if(V_VT(src) != VT_DISPATCH)
        change_type_fail(change_type, src, VT_DISPATCH, E_NOTIMPL);
    else {
        call_change_type(change_type, &v, src, VT_DISPATCH);
        ok(V_DISPATCH(&v) == V_DISPATCH(src), "V_DISPATCH(v) != V_DISPATCH(src)\n");
        VariantClear(&v);
    }
}

static void test_change_types(IVariantChangeType *change_type, IDispatch *obj_disp)
{
    VARIANT v, dst;
    BSTR str;
    HRESULT hres;

    static const conv_results_t bool_results[] = {
        {0, L"false", VARIANT_FALSE, 1,0.0},
        {1, L"true", VARIANT_TRUE, 1,1.0}};
    static const conv_results_t int_results[] = {
        {0, L"0", VARIANT_FALSE, 1,0.0},
        {-100, L"-100", VARIANT_TRUE, 1,-100.0},
        {0x10010, L"65552", VARIANT_TRUE, 1,65552.0}};
    static const conv_results_t empty_results =
        {0, L"undefined", VARIANT_FALSE, 0,0};
    static const conv_results_t null_results =
        {0, L"null", VARIANT_FALSE, 0,0};
    static const conv_results_t obj_results =
        {10, L"strval", VARIANT_TRUE, 1,10.0};

    V_VT(&v) = VT_BOOL;
    V_BOOL(&v) = VARIANT_FALSE;
    test_change_type(change_type, &v, bool_results);
    V_BOOL(&v) = VARIANT_TRUE;
    test_change_type(change_type, &v, bool_results+1);

    V_VT(&v) = VT_I4;
    V_I4(&v) = 0;
    test_change_type(change_type, &v, int_results);
    V_I4(&v) = -100;
    test_change_type(change_type, &v, int_results+1);
    V_I4(&v) = 0x10010;
    test_change_type(change_type, &v, int_results+2);

    V_VT(&v) = VT_EMPTY;
    test_change_type(change_type, &v, &empty_results);

    V_VT(&v) = VT_NULL;
    test_change_type(change_type, &v, &null_results);

    V_VT(&v) = VT_UNKNOWN;
    V_UNKNOWN(&v) = (IUnknown*)obj_disp;
    test_change_type(change_type, &v, &obj_results);

    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = obj_disp;
    test_change_type(change_type, &v, &obj_results);

    V_VT(&v) = VT_BOOL;
    V_BOOL(&v) = VARIANT_FALSE;
    V_VT(&dst) = 0xdead;
    hres = IVariantChangeType_ChangeType(change_type, &dst, &v, 0, VT_I4);
    ok(hres == DISP_E_BADVARTYPE, "ChangeType failed: %08lx, expected DISP_E_BADVARTYPE\n", hres);
    ok(V_VT(&dst) == 0xdead, "V_VT(dst) = %d\n", V_VT(&dst));

    /* Test conversion in place */
    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = str = SysAllocString(L"test");
    hres = IVariantChangeType_ChangeType(change_type, &v, &v, 0, VT_BSTR);
    ok(hres == S_OK, "ChangeType failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_BSTR(&v) != str, "V_BSTR(v) == str\n");
    ok(!lstrcmpW(V_BSTR(&v), L"test"), "V_BSTR(v) = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
}

static void test_caller(IServiceProvider *caller, IDispatch *arg_obj)
{
    IVariantChangeType *change_type;
    IUnknown *unk;
    HRESULT hres;

    hres = IServiceProvider_QueryService(caller, &SID_VariantConversion, &IID_IVariantChangeType, (void**)&change_type);
    ok(hres == S_OK, "Could not get SID_VariantConversion service: %08lx\n", hres);

    ok(change_type == script_change_type, "change_type != script_change_type\n");
    test_change_types(change_type, arg_obj);

    IVariantChangeType_Release(change_type);

    SET_EXPECT(site_QI_NULL);
    hres = IServiceProvider_QueryService(caller, &IID_IActiveScriptSite, &IID_NULL, (void**)&unk);
    ok(hres == E_NOINTERFACE, "Querying for IActiveScriptSite->NULL returned: %08lx\n", hres);
    ok(!unk, "unk != NULL\n");
    CHECK_CALLED(site_QI_NULL);
}

static IServiceProvider sp_caller_obj;

static HRESULT WINAPI sp_caller_QueryInterface(IServiceProvider *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IServiceProvider, riid))
        *ppv = &sp_caller_obj;
    else {
        ok(IsEqualGUID(&IID_NULL, riid), "unexpected riid %s\n", wine_dbgstr_guid(riid));
        CHECK_EXPECT(sp_caller_QI_NULL);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    return S_OK;
}

static ULONG WINAPI sp_caller_AddRef(IServiceProvider *iface)
{
    return 2;
}

static ULONG WINAPI sp_caller_Release(IServiceProvider *iface)
{
    return 1;
}

static HRESULT WINAPI sp_caller_QueryService(IServiceProvider *iface, REFGUID guidService,
        REFIID riid, void **ppv)
{
    if(IsEqualGUID(guidService, &SID_GetCaller)) {
        ok(IsEqualGUID(riid, &IID_IServiceProvider), "unexpected riid %s\n", wine_dbgstr_guid(riid));
        *ppv = NULL;
        return S_OK;
    }

    ok(0, "unexpected guidService %s with riid %s\n", wine_dbgstr_guid(guidService), wine_dbgstr_guid(riid));
    *ppv = NULL;
    return E_NOINTERFACE;
}

static const IServiceProviderVtbl sp_caller_vtbl = {
    sp_caller_QueryInterface,
    sp_caller_AddRef,
    sp_caller_Release,
    sp_caller_QueryService
};

static IServiceProvider sp_caller_obj = { &sp_caller_vtbl };

static HRESULT WINAPI DispatchEx_QueryInterface(IDispatchEx *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(riid, &IID_IUnknown)) {
        *ppv = iface;
    }else if(IsEqualGUID(riid, &IID_IDispatch) || IsEqualGUID(riid, &IID_IDispatchEx)) {
        *ppv = iface;
    }else if(IsEqualGUID(&IID_IObjectSafety, riid)) {
        ok(0, "Unexpected IID_IObjectSafety query\n");
    }else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

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

static HRESULT WINAPI DispatchEx_GetTypeInfo(IDispatchEx *iface, UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
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
    ok(0, "unexpected call %s %lx\n", wine_dbgstr_w(bstrName), grfdex);
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

static HRESULT WINAPI Test_GetDispID(IDispatchEx *iface, BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    if(!lstrcmpW(bstrName, L"testArgConv")) {
        ok(grfdex == fdexNameCaseSensitive, "grfdex = %lx\n", grfdex);
        *pid = DISPID_TEST_TESTARGCONV;
        return S_OK;
    }
    if(!lstrcmpW(bstrName, L"testGetCaller")) {
        ok(grfdex == fdexNameCaseSensitive, "grfdex = %lx\n", grfdex);
        *pid = DISPID_TEST_TESTGETCALLER;
        return S_OK;
    }
    if(!lstrcmpW(bstrName, L"testGetCallerJS")) {
        ok(grfdex == fdexNameCaseSensitive, "grfdex = %lx\n", grfdex);
        *pid = DISPID_TEST_TESTGETCALLERJS;
        return S_OK;
    }
    if(!lstrcmpW(bstrName, L"testGetCallerNested")) {
        ok(grfdex == fdexNameCaseSensitive, "grfdex = %lx\n", grfdex);
        *pid = DISPID_TEST_TESTGETCALLERNESTED;
        return S_OK;
    }

    return E_NOTIMPL;
}

static HRESULT WINAPI Test_InvokeEx(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    IServiceProvider *caller = (void*)0xdeadbeef;
    HRESULT hres;

    ok(pspCaller != NULL, "pspCaller == NULL\n");

    switch(id) {
    case DISPID_TEST_TESTARGCONV:
        CHECK_EXPECT(testArgConv);

        ok(wFlags == INVOKE_FUNC, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(!pvarRes, "pvarRes != NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        ok(pdp->cArgs == 1, "cArgs = %d\n", pdp->cArgs);
        ok(V_VT(pdp->rgvarg) == VT_DISPATCH, "V_VT(rgvarg) = %d\n", V_VT(pdp->rgvarg));

        CHECK_CALLED(OnEnterScript);
        test_caller(pspCaller, V_DISPATCH(pdp->rgvarg));
        SET_EXPECT(OnLeaveScript);

        stored_obj = V_DISPATCH(pdp->rgvarg);
        IDispatch_AddRef(stored_obj);
        break;

    case DISPID_TEST_TESTGETCALLER: {
        void *iface = (void*)0xdeadbeef;

        CHECK_EXPECT(testGetCaller);
        CHECK_CALLED(OnEnterScript);

        ok(wFlags == DISPATCH_METHOD, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(!pvarRes, "pvarRes != NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        SET_EXPECT(OnEnterScript);
        SET_EXPECT(OnLeaveScript);
        SET_EXPECT(testGetCallerNested);
        hres = IActiveScriptParse_ParseScriptText(active_script_parser, L"testGetCallerNested(1,2)",
                                                  NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
        ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
        CHECK_CALLED(testGetCallerNested);
        CHECK_CALLED(OnLeaveScript);
        CHECK_CALLED(OnEnterScript);
        SET_EXPECT(OnLeaveScript);

        hres = IServiceProvider_QueryService(pspCaller, &SID_GetCaller, &IID_IServiceProvider, (void**)&caller);
        ok(hres == S_OK, "Could not get SID_GetCaller service: %08lx\n", hres);
        ok(caller == test_get_caller_sp, "caller != test_get_caller_sp\n");
        if(caller) IServiceProvider_Release(caller);

        if(test_get_caller_sp)
            SET_EXPECT(sp_caller_QI_NULL);
        hres = IServiceProvider_QueryService(pspCaller, &SID_GetCaller, &IID_NULL, &iface);
        ok(hres == (test_get_caller_sp ? E_NOINTERFACE : S_OK), "Could not query SID_GetCaller with IID_NULL: %08lx\n", hres);
        ok(iface == NULL, "iface != NULL\n");
        if(test_get_caller_sp)
            CHECK_CALLED(sp_caller_QI_NULL);
        break;
    }

    case DISPID_TEST_TESTGETCALLERJS:
        CHECK_EXPECT(testGetCallerJS);

        ok(wFlags == DISPATCH_METHOD, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->cArgs == 1, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(!pvarRes, "pvarRes != NULL\n");
        ok(pei != NULL, "pei == NULL\n");
        ok(V_VT(pdp->rgvarg) == VT_I4, "V_VT(rgvarg) = %d\n", V_VT(pdp->rgvarg));
        ok(V_I4(pdp->rgvarg) == 42, "V_I4(rgvarg) = %ld\n", V_I4(pdp->rgvarg));

        hres = IServiceProvider_QueryService(pspCaller, &SID_GetCaller, &IID_IServiceProvider, (void**)&caller);
        ok(hres == E_NOINTERFACE, "QueryService(SID_GetCaller) returned: %08lx\n", hres);
        ok(caller == NULL, "caller != NULL\n");
        break;

    case DISPID_TEST_TESTGETCALLERNESTED:
        CHECK_EXPECT(testGetCallerNested);

        ok(wFlags == DISPATCH_METHOD, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->cArgs == 2, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(!pvarRes, "pvarRes != NULL\n");
        ok(pei != NULL, "pei == NULL\n");
        ok(V_VT(&pdp->rgvarg[0]) == VT_I4, "V_VT(rgvarg[0]) = %d\n", V_VT(&pdp->rgvarg[0]));
        ok(V_VT(&pdp->rgvarg[1]) == VT_I4, "V_VT(rgvarg[1]) = %d\n", V_VT(&pdp->rgvarg[1]));
        ok(V_I4(&pdp->rgvarg[0]) == 2, "V_I4(rgvarg[0]) = %ld\n", V_I4(&pdp->rgvarg[0]));
        ok(V_I4(&pdp->rgvarg[1]) == 1, "V_I4(rgvarg[1]) = %ld\n", V_I4(&pdp->rgvarg[1]));

        hres = IServiceProvider_QueryService(pspCaller, &SID_GetCaller, &IID_IServiceProvider, (void**)&caller);
        ok(hres == E_NOINTERFACE, "QueryService(SID_GetCaller) returned: %08lx\n", hres);
        ok(caller == NULL, "caller != NULL\n");
        break;

    default:
        ok(0, "unexpected call\n");
        return E_NOTIMPL;
    }

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
    Test_GetDispID,
    Test_InvokeEx,
    DispatchEx_DeleteMemberByName,
    DispatchEx_DeleteMemberByDispID,
    DispatchEx_GetMemberProperties,
    DispatchEx_GetMemberName,
    DispatchEx_GetNextDispID,
    DispatchEx_GetNameSpaceParent
};

static IDispatchEx testObj = { &testObjVtbl };

static HRESULT WINAPI ActiveScriptSite_QueryInterface(IActiveScriptSite *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = iface;
    }else if(IsEqualGUID(&IID_IActiveScriptSite, riid)) {
        *ppv = iface;
    }else {
        if(IsEqualGUID(&IID_NULL, riid))
            CHECK_EXPECT(site_QI_NULL);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

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
    ok(dwReturnMask == SCRIPTINFO_IUNKNOWN, "unexpected dwReturnMask %lx\n", dwReturnMask);
    ok(!ppti, "ppti != NULL\n");
    ok(!lstrcmpW(pstrName, L"test"), "pstrName = %s\n", wine_dbgstr_w(pstrName));

    *ppiunkItem = (IUnknown*)&testObj;
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
    CHECK_EXPECT(OnEnterScript);
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_OnLeaveScript(IActiveScriptSite *iface)
{
    CHECK_EXPECT(OnLeaveScript);
    return E_NOTIMPL;
}

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

#define parse_script(p,s) _parse_script(__LINE__,p,s)
static void _parse_script(unsigned line, IActiveScriptParse *parser, const WCHAR *script)
{
    HRESULT hres;

    hres = IActiveScriptParse_ParseScriptText(parser, script, NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
    ok_(__FILE__,line)(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
}

static IActiveScriptParse *create_script(void)
{
    IActiveScriptParse *parser;
    IActiveScript *script;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_JScript, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IActiveScript, (void**)&script);
    if(FAILED(hres))
        return NULL;

    hres = IActiveScript_QueryInterface(script, &IID_IActiveScriptParse, (void**)&parser);
    ok(hres == S_OK, "Could not get IActiveScriptParse: %08lx\n", hres);

    hres = IActiveScriptParse_InitNew(parser);
    ok(hres == S_OK, "InitNew failed: %08lx\n", hres);

    hres = IActiveScript_SetScriptSite(script, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08lx\n", hres);

    hres = IActiveScript_AddNamedItem(script, L"test",
            SCRIPTITEM_ISVISIBLE|SCRIPTITEM_ISSOURCE|SCRIPTITEM_GLOBALMEMBERS);
    ok(hres == S_OK, "AddNamedItem failed: %08lx\n", hres);

    hres = IActiveScript_SetScriptState(script, SCRIPTSTATE_STARTED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_STARTED) failed: %08lx\n", hres);

    IActiveScript_Release(script);

    return parser;
}

static void run_scripts(void)
{
    IActiveScript *active_script;
    DISPPARAMS dp = { 0 };
    IDispatchEx *dispex;
    IDispatch *disp;
    DISPID dispid;
    HRESULT hres;
    VARIANT var;
    BSTR bstr;

    active_script_parser = create_script();

    hres = IActiveScriptParse_QueryInterface(active_script_parser, &IID_IVariantChangeType, (void**)&script_change_type);
    ok(hres == S_OK, "Could not get IVariantChangeType iface: %08lx\n", hres);

    SET_EXPECT(OnEnterScript); /* checked in callback */
    SET_EXPECT(testArgConv);
    SET_EXPECT(testGetCallerJS);
    parse_script(active_script_parser,
                 L"var obj = {"
                 L"    toString: function() { return 'strval'; },"
                 L"    valueOf: function()  { return 10; }"
                 L"};"
                 L"testArgConv(obj);"
                 L"function testGetCallerFunc() { testGetCaller(); };"
                 L"testGetCallerJS(42);");
    CHECK_CALLED(testGetCallerJS);
    CHECK_CALLED(testArgConv);
    CHECK_CALLED(OnLeaveScript); /* set in callback */

    test_change_types(script_change_type, stored_obj);
    IDispatch_Release(stored_obj);
    IVariantChangeType_Release(script_change_type);

    hres = IActiveScriptParse_QueryInterface(active_script_parser, &IID_IActiveScript, (void**)&active_script);
    ok(hres == S_OK, "Could not get IActiveScript: %08lx\n", hres);
    hres = IActiveScript_GetScriptDispatch(active_script, NULL, &disp);
    ok(hres == S_OK, "GetScriptDispatch failed: %08lx\n", hres);
    IActiveScript_Release(active_script);
    hres = IDispatch_QueryInterface(disp, &IID_IDispatchEx, (void**)&dispex);
    ok(hres == S_OK, "Could not get IDispatchEx: %08lx\n", hres);
    IDispatch_Release(disp);
    bstr = SysAllocString(L"testGetCallerFunc");
    hres = IDispatchEx_GetDispID(dispex, bstr, 0, &dispid);
    ok(hres == S_OK, "GetDispID failed: %08lx\n", hres);
    SysFreeString(bstr);

    hres = IDispatchEx_InvokeEx(dispex, dispid, 0, DISPATCH_PROPERTYGET, &dp, &var, NULL, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    ok(V_VT(&var) == VT_DISPATCH, "V_VT(testGetCallerFunc) = %d\n", V_VT(&var));
    ok(V_DISPATCH(&var) != NULL, "V_DISPATCH(testGetCallerFunc) = NULL\n");
    IDispatchEx_Release(dispex);
    hres = IDispatch_QueryInterface(V_DISPATCH(&var), &IID_IDispatchEx, (void**)&dispex);
    ok(hres == S_OK, "Could not get IDispatchEx: %08lx\n", hres);
    IDispatch_Release(V_DISPATCH(&var));

    SET_EXPECT(OnEnterScript);
    SET_EXPECT(OnLeaveScript);
    SET_EXPECT(testGetCaller);
    hres = IDispatchEx_InvokeEx(dispex, DISPID_VALUE, 0, DISPATCH_METHOD, &dp, NULL, NULL, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    CHECK_CALLED(testGetCaller);
    CHECK_CALLED(OnLeaveScript);
    test_get_caller_sp = &sp_caller_obj;
    SET_EXPECT(OnEnterScript);
    SET_EXPECT(OnLeaveScript);
    SET_EXPECT(testGetCaller);
    hres = IDispatchEx_InvokeEx(dispex, DISPID_VALUE, 0, DISPATCH_METHOD, &dp, NULL, NULL, test_get_caller_sp);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    CHECK_CALLED(testGetCaller);
    CHECK_CALLED(OnLeaveScript);
    IDispatchEx_Release(dispex);

    IActiveScriptParse_Release(active_script_parser);
    active_script_parser = NULL;
}

static BOOL check_jscript(void)
{
    IActiveScriptProperty *script_prop;
    IActiveScriptParse *parser;
    HRESULT hres;

    parser = create_script();
    if(!parser)
        return FALSE;

    SET_EXPECT(OnEnterScript);
    SET_EXPECT(OnLeaveScript);
    hres = IActiveScriptParse_ParseScriptText(parser, L"if(!('localeCompare' in String.prototype)) throw 1;",
                                              NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
    CLEAR_CALLED(OnEnterScript);
    CLEAR_CALLED(OnLeaveScript);
    if(hres == S_OK)
        hres = IActiveScriptParse_QueryInterface(parser, &IID_IActiveScriptProperty, (void**)&script_prop);
    IActiveScriptParse_Release(parser);
    if(hres == S_OK)
        IActiveScriptProperty_Release(script_prop);

    return hres == S_OK;
}

START_TEST(caller)
{
    CoInitialize(NULL);

    if(check_jscript())
        run_scripts();
    else
        win_skip("Broken (too old) jscript\n");

    CoUninitialize();
}
