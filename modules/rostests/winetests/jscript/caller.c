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

DEFINE_EXPECT(testArgConv);

static const WCHAR testW[] = {'t','e','s','t',0};

static IVariantChangeType *script_change_type;
static IDispatch *stored_obj;

#define DISPID_TEST_TESTARGCONV      0x1000

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

typedef struct {
    int int_result;
    const char *str_result;
    VARIANT_BOOL bool_result;
    int test_double;
    double double_result;
} conv_results_t;

#define call_change_type(a,b,c,d) _call_change_type(__LINE__,a,b,c,d)
static void _call_change_type(unsigned line, IVariantChangeType *change_type, VARIANT *dst, VARIANT *src, VARTYPE vt)
{
    HRESULT hres;

    VariantInit(dst);
    hres = IVariantChangeType_ChangeType(change_type, dst, src, 0, vt);
    ok_(__FILE__,line)(hres == S_OK, "ChangeType(%d) failed: %08x\n", vt, hres);
    ok_(__FILE__,line)(V_VT(dst) == vt, "V_VT(dst) = %d\n", V_VT(dst));
}

#define change_type_fail(a,b,c,d) _change_type_fail(__LINE__,a,b,c,d)
static void _change_type_fail(unsigned line, IVariantChangeType *change_type, VARIANT *src, VARTYPE vt, HRESULT exhres)
{
    VARIANT v;
    HRESULT hres;

    V_VT(&v) = VT_EMPTY;
    hres = IVariantChangeType_ChangeType(change_type, &v, src, 0, vt);
    ok_(__FILE__,line)(hres == exhres, "ChangeType failed: %08x, expected %08x\n", hres, exhres);
}

static void test_change_type(IVariantChangeType *change_type, VARIANT *src, const conv_results_t *ex)
{
    VARIANT v;

    call_change_type(change_type, &v, src, VT_I4);
    ok(V_I4(&v) == ex->int_result, "V_I4(v) = %d, expected %d\n", V_I4(&v), ex->int_result);

    call_change_type(change_type, &v, src, VT_UI2);
    ok(V_UI2(&v) == (UINT16)ex->int_result, "V_UI2(v) = %u, expected %u\n", V_UI2(&v), (UINT16)ex->int_result);

    call_change_type(change_type, &v, src, VT_BSTR);
    ok(!strcmp_wa(V_BSTR(&v), ex->str_result), "V_BSTR(v) = %s, expected %s\n", wine_dbgstr_w(V_BSTR(&v)), ex->str_result);
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
}

static void test_change_types(IVariantChangeType *change_type, IDispatch *obj_disp)
{
    VARIANT v, dst;
    BSTR str;
    HRESULT hres;

    static const conv_results_t bool_results[] = {
        {0, "false", VARIANT_FALSE, 1,0.0},
        {1, "true", VARIANT_TRUE, 1,1.0}};
    static const conv_results_t int_results[] = {
        {0, "0", VARIANT_FALSE, 1,0.0},
        {-100, "-100", VARIANT_TRUE, 1,-100.0},
        {0x10010, "65552", VARIANT_TRUE, 1,65552.0}};
    static const conv_results_t empty_results =
        {0, "undefined", VARIANT_FALSE, 0,0};
    static const conv_results_t null_results =
        {0, "null", VARIANT_FALSE, 0,0};
    static const conv_results_t obj_results =
        {10, "strval", VARIANT_TRUE, 1,10.0};

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

    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = obj_disp;
    test_change_type(change_type, &v, &obj_results);

    V_VT(&v) = VT_BOOL;
    V_BOOL(&v) = VARIANT_FALSE;
    V_VT(&dst) = 0xdead;
    hres = IVariantChangeType_ChangeType(change_type, &dst, &v, 0, VT_I4);
    ok(hres == DISP_E_BADVARTYPE, "ChangeType failed: %08x, expected DISP_E_BADVARTYPE\n", hres);
    ok(V_VT(&dst) == 0xdead, "V_VT(dst) = %d\n", V_VT(&dst));

    /* Test conversion in place */
    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = str = a2bstr("test");
    hres = IVariantChangeType_ChangeType(change_type, &v, &v, 0, VT_BSTR);
    ok(hres == S_OK, "ChangeType failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_BSTR(&v) != str, "V_BSTR(v) == str\n");
    ok(!strcmp_wa(V_BSTR(&v), "test"), "V_BSTR(v) = %s\n", wine_dbgstr_w(V_BSTR(&v)));
}

static void test_caller(IServiceProvider *caller, IDispatch *arg_obj)
{
    IVariantChangeType *change_type;
    HRESULT hres;

    hres = IServiceProvider_QueryService(caller, &SID_VariantConversion, &IID_IVariantChangeType, (void**)&change_type);
    ok(hres == S_OK, "Could not get SID_VariantConversion service: %08x\n", hres);

    ok(change_type == script_change_type, "change_type != script_change_type\n");
    test_change_types(change_type, arg_obj);

    IVariantChangeType_Release(change_type);
}

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

static HRESULT WINAPI Test_GetDispID(IDispatchEx *iface, BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    if(!strcmp_wa(bstrName, "testArgConv")) {
        ok(grfdex == fdexNameCaseSensitive, "grfdex = %x\n", grfdex);
        *pid = DISPID_TEST_TESTARGCONV;
        return S_OK;
    }

    return E_NOTIMPL;
}

static HRESULT WINAPI Test_InvokeEx(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
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

        test_caller(pspCaller, V_DISPATCH(pdp->rgvarg));

        stored_obj = V_DISPATCH(pdp->rgvarg);
        IDispatch_AddRef(stored_obj);
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
    ok(dwReturnMask == SCRIPTINFO_IUNKNOWN, "unexpected dwReturnMask %x\n", dwReturnMask);
    ok(!ppti, "ppti != NULL\n");
    ok(!strcmp_wa(pstrName, "test"), "pstrName = %s\n", wine_dbgstr_w(pstrName));

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
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_OnLeaveScript(IActiveScriptSite *iface)
{
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

#define parse_script_a(p,s) _parse_script_a(__LINE__,p,s)
static void _parse_script_a(unsigned line, IActiveScriptParse *parser, const char *script)
{
    BSTR str;
    HRESULT hres;

    str = a2bstr(script);
    hres = IActiveScriptParse_ParseScriptText(parser, str, NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
    SysFreeString(str);
    ok_(__FILE__,line)(hres == S_OK, "ParseScriptText failed: %08x\n", hres);
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
    ok(hres == S_OK, "Could not get IActiveScriptParse: %08x\n", hres);

    hres = IActiveScriptParse_InitNew(parser);
    ok(hres == S_OK, "InitNew failed: %08x\n", hres);

    hres = IActiveScript_SetScriptSite(script, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08x\n", hres);

    hres = IActiveScript_AddNamedItem(script, testW,
            SCRIPTITEM_ISVISIBLE|SCRIPTITEM_ISSOURCE|SCRIPTITEM_GLOBALMEMBERS);
    ok(hres == S_OK, "AddNamedItem failed: %08x\n", hres);

    hres = IActiveScript_SetScriptState(script, SCRIPTSTATE_STARTED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_STARTED) failed: %08x\n", hres);

    IActiveScript_Release(script);

    return parser;
}

static void run_scripts(void)
{
    IActiveScriptParse *parser;
    HRESULT hres;

    parser = create_script();

    hres = IActiveScriptParse_QueryInterface(parser, &IID_IVariantChangeType, (void**)&script_change_type);
    ok(hres == S_OK, "Could not get IVariantChangeType iface: %08x\n", hres);

    SET_EXPECT(testArgConv);
    parse_script_a(parser,
                   "var obj = {"
                   "    toString: function() { return 'strval'; },"
                   "    valueOf: function()  { return 10; }"
                   "};"
                   "testArgConv(obj);");
    CHECK_CALLED(testArgConv);

    test_change_types(script_change_type, stored_obj);
    IDispatch_Release(stored_obj);
    IVariantChangeType_Release(script_change_type);

    IActiveScriptParse_Release(parser);
}

static BOOL check_jscript(void)
{
    IActiveScriptProperty *script_prop;
    IActiveScriptParse *parser;
    BSTR str;
    HRESULT hres;

    parser = create_script();
    if(!parser)
        return FALSE;

    str = a2bstr("if(!('localeCompare' in String.prototype)) throw 1;");
    hres = IActiveScriptParse_ParseScriptText(parser, str, NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
    SysFreeString(str);

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
