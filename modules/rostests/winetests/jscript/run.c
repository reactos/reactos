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

#ifdef _WIN64

#define IActiveScriptParse_QueryInterface IActiveScriptParse64_QueryInterface
#define IActiveScriptParse_Release IActiveScriptParse64_Release
#define IActiveScriptParse_InitNew IActiveScriptParse64_InitNew
#define IActiveScriptParse_ParseScriptText IActiveScriptParse64_ParseScriptText
#define IActiveScriptParseProcedure2_Release IActiveScriptParseProcedure2_64_Release
#define IActiveScriptParseProcedure2_ParseProcedureText IActiveScriptParseProcedure2_64_ParseProcedureText

#else

#define IActiveScriptParse_QueryInterface IActiveScriptParse32_QueryInterface
#define IActiveScriptParse_Release IActiveScriptParse32_Release
#define IActiveScriptParse_InitNew IActiveScriptParse32_InitNew
#define IActiveScriptParse_ParseScriptText IActiveScriptParse32_ParseScriptText
#define IActiveScriptParseProcedure2_Release IActiveScriptParseProcedure2_32_Release
#define IActiveScriptParseProcedure2_ParseProcedureText IActiveScriptParseProcedure2_32_ParseProcedureText

#endif

static const CLSID CLSID_JScript =
    {0xf414c260,0x6ac0,0x11cf,{0xb6,0xd1,0x00,0xaa,0x00,0xbb,0xbb,0x58}};
static const CLSID CLSID_JScriptEncode =
    {0xf414c262,0x6ac0,0x11cf,{0xb6,0xd1,0x00,0xaa,0x00,0xbb,0xbb,0x58}};

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

DEFINE_EXPECT(global_propget_d);
DEFINE_EXPECT(global_propget_i);
DEFINE_EXPECT(global_propput_d);
DEFINE_EXPECT(global_propput_i);
DEFINE_EXPECT(global_propputref_d);
DEFINE_EXPECT(global_propputref_i);
DEFINE_EXPECT(global_propdelete_d);
DEFINE_EXPECT(global_nopropdelete_d);
DEFINE_EXPECT(global_propdeleteerror_d);
DEFINE_EXPECT(global_success_d);
DEFINE_EXPECT(global_success_i);
DEFINE_EXPECT(global_notexists_d);
DEFINE_EXPECT(global_propargput_d);
DEFINE_EXPECT(global_propargput_i);
DEFINE_EXPECT(global_propargputop_d);
DEFINE_EXPECT(global_propargputop_get_i);
DEFINE_EXPECT(global_propargputop_put_i);
DEFINE_EXPECT(global_testargtypes_i);
DEFINE_EXPECT(global_calleval_i);
DEFINE_EXPECT(puredisp_prop_d);
DEFINE_EXPECT(puredisp_noprop_d);
DEFINE_EXPECT(puredisp_value);
DEFINE_EXPECT(dispexfunc_value);
DEFINE_EXPECT(testobj_delete_test);
DEFINE_EXPECT(testobj_delete_nodelete);
DEFINE_EXPECT(testobj_value);
DEFINE_EXPECT(testobj_construct);
DEFINE_EXPECT(testobj_prop_d);
DEFINE_EXPECT(testobj_withprop_d);
DEFINE_EXPECT(testobj_withprop_i);
DEFINE_EXPECT(testobj_noprop_d);
DEFINE_EXPECT(testobj_onlydispid_d);
DEFINE_EXPECT(testobj_onlydispid_i);
DEFINE_EXPECT(testobj_notexists_d);
DEFINE_EXPECT(testobj_newenum);
DEFINE_EXPECT(testobj_getidfail_d);
DEFINE_EXPECT(testobj_tolocalestr_d);
DEFINE_EXPECT(testobj_tolocalestr_i);
DEFINE_EXPECT(test_caller_get);
DEFINE_EXPECT(test_caller_null);
DEFINE_EXPECT(test_caller_obj);
DEFINE_EXPECT(testdestrobj);
DEFINE_EXPECT(enumvariant_next_0);
DEFINE_EXPECT(enumvariant_next_1);
DEFINE_EXPECT(enumvariant_reset);
DEFINE_EXPECT(GetItemInfo_testVal);
DEFINE_EXPECT(ActiveScriptSite_OnScriptError);
DEFINE_EXPECT(invoke_func);
DEFINE_EXPECT(DeleteMemberByDispID);
DEFINE_EXPECT(DeleteMemberByDispID_false);
DEFINE_EXPECT(DeleteMemberByDispID_error);
DEFINE_EXPECT(BindHandler);

#define JS_E_SUBSCRIPT_OUT_OF_RANGE  0x800a0009
#define JS_E_INVALID_ACTION          0x800a01bd
#define JS_E_OBJECT_EXPECTED         0x800a138f
#define JS_E_UNDEFINED_VARIABLE      0x800a1391
#define JS_E_EXCEPTION_THROWN        0x800a139e
#define JS_E_SYNTAX                  0x800a03ea
#define JS_E_MISSING_RBRACKET        0x800a03ee
#define JS_E_MISPLACED_RETURN        0x800a03fa

#define DISPID_GLOBAL_TESTPROPGET   0x1000
#define DISPID_GLOBAL_TESTPROPPUT   0x1001
#define DISPID_GLOBAL_REPORTSUCCESS 0x1002
#define DISPID_GLOBAL_TRACE         0x1003
#define DISPID_GLOBAL_OK            0x1004
#define DISPID_GLOBAL_GETVT         0x1005
#define DISPID_GLOBAL_TESTOBJ       0x1006
#define DISPID_GLOBAL_GETNULLBSTR   0x1007
#define DISPID_GLOBAL_NULL_DISP     0x1008
#define DISPID_GLOBAL_TESTTHIS      0x1009
#define DISPID_GLOBAL_TESTTHIS2     0x100a
#define DISPID_GLOBAL_INVOKEVERSION 0x100b
#define DISPID_GLOBAL_CREATEARRAY   0x100c
#define DISPID_GLOBAL_PROPGETFUNC   0x100d
#define DISPID_GLOBAL_OBJECT_FLAG   0x100e
#define DISPID_GLOBAL_ISWIN64       0x100f
#define DISPID_GLOBAL_PUREDISP      0x1010
#define DISPID_GLOBAL_ISNULLBSTR    0x1011
#define DISPID_GLOBAL_PROPARGPUT    0x1012
#define DISPID_GLOBAL_SHORTPROP     0x1013
#define DISPID_GLOBAL_GETSHORT      0x1014
#define DISPID_GLOBAL_TESTARGTYPES  0x1015
#define DISPID_GLOBAL_INTPROP       0x1016
#define DISPID_GLOBAL_DISPUNK       0x1017
#define DISPID_GLOBAL_TESTRES       0x1018
#define DISPID_GLOBAL_TESTNORES     0x1019
#define DISPID_GLOBAL_DISPEXFUNC    0x101a
#define DISPID_GLOBAL_TESTPROPPUTREF 0x101b
#define DISPID_GLOBAL_GETSCRIPTSTATE 0x101c
#define DISPID_GLOBAL_BINDEVENTHANDLER 0x101d
#define DISPID_GLOBAL_TESTENUMOBJ   0x101e
#define DISPID_GLOBAL_CALLEVAL      0x101f
#define DISPID_GLOBAL_PROPARGPUTOP  0x1020
#define DISPID_GLOBAL_THROWINT      0x1021
#define DISPID_GLOBAL_THROWEI       0x1022
#define DISPID_GLOBAL_VDATE         0x1023
#define DISPID_GLOBAL_VCY           0x1024
#define DISPID_GLOBAL_TODOWINE      0x1025
#define DISPID_GLOBAL_TESTDESTROBJ  0x1026

#define DISPID_GLOBAL_TESTPROPDELETE      0x2000
#define DISPID_GLOBAL_TESTNOPROPDELETE    0x2001
#define DISPID_GLOBAL_TESTPROPDELETEERROR 0x2002

#define DISPID_TESTOBJ_PROP         0x2000
#define DISPID_TESTOBJ_ONLYDISPID   0x2001
#define DISPID_TESTOBJ_WITHPROP     0x2002
#define DISPID_TESTOBJ_TOLOCALESTR  0x2003

#define JS_E_OUT_OF_MEMORY 0x800a03ec
#define JS_E_INVALID_CHAR 0x800a03f6

static BOOL strict_dispid_check, testing_expr;
static const char *test_name = "(null)";
static IDispatch *script_disp;
static int invoke_version;
static BOOL use_english;
static IActiveScriptError *script_error;
static IActiveScript *script_engine;
static const CLSID *engine_clsid = &CLSID_JScript;

/* Returns true if the user interface is in English. Note that this does not
 * presume of the formatting of dates, numbers, etc.
 */
static BOOL is_lang_english(void)
{
    static HMODULE hkernel32 = NULL;
    static LANGID (WINAPI *pGetThreadUILanguage)(void) = NULL;
    static LANGID (WINAPI *pGetUserDefaultUILanguage)(void) = NULL;

    if (!hkernel32)
    {
        hkernel32 = GetModuleHandleA("kernel32.dll");
        pGetThreadUILanguage = (void*)GetProcAddress(hkernel32, "GetThreadUILanguage");
        pGetUserDefaultUILanguage = (void*)GetProcAddress(hkernel32, "GetUserDefaultUILanguage");
    }
    if (pGetThreadUILanguage)
        return PRIMARYLANGID(pGetThreadUILanguage()) == LANG_ENGLISH;
    if (pGetUserDefaultUILanguage)
        return PRIMARYLANGID(pGetUserDefaultUILanguage()) == LANG_ENGLISH;

    return PRIMARYLANGID(GetUserDefaultLangID()) == LANG_ENGLISH;
}

#define test_grfdex(a,b) _test_grfdex(__LINE__,a,b)
static void _test_grfdex(unsigned line, DWORD grfdex, DWORD expect)
{
    expect |= invoke_version << 28;
    ok_(__FILE__,line)(grfdex == expect, "grfdex = %lx, expected %lx\n", grfdex, expect);
}

static void close_script(IActiveScript *script)
{
    HRESULT hres;
    ULONG ref;

    hres = IActiveScript_Close(script);
    ok(hres == S_OK, "Close failed: %08lx\n", hres);

    ref = IActiveScript_Release(script);
    ok(!ref, "ref=%lu\n", ref);
}

static HRESULT WINAPI EnumVARIANT_QueryInterface(IEnumVARIANT *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if (IsEqualGUID(riid, &IID_IEnumVARIANT))
        *ppv = iface;
    else
        return E_NOINTERFACE;

    return S_OK;
}

static ULONG WINAPI EnumVARIANT_AddRef(IEnumVARIANT *iface)
{
    return 2;
}

static ULONG WINAPI EnumVARIANT_Release(IEnumVARIANT *iface)
{
    return 1;
}

static int EnumVARIANT_index = 0;
static int EnumVARIANT_next_0_count = 0;
static HRESULT WINAPI EnumVARIANT_Next(
    IEnumVARIANT *This,
    ULONG celt,
    VARIANT *rgVar,
    ULONG *pCeltFetched)
{
    ok(rgVar != NULL, "rgVar is NULL\n");
    ok(celt == 1, "celt = %ld\n", celt);
    ok(pCeltFetched == NULL, "pCeltFetched is not NULL\n");

    if (!rgVar)
        return S_FALSE;

    if (EnumVARIANT_index == 0)
    {
        EnumVARIANT_next_0_count--;
        if (EnumVARIANT_next_0_count <= 0)
            CHECK_EXPECT(enumvariant_next_0);

        V_VT(rgVar) = VT_I4;
        V_I4(rgVar) = 123;

        if (pCeltFetched)
            *pCeltFetched = 1;
        EnumVARIANT_index++;
        return S_OK;
    }

    CHECK_EXPECT(enumvariant_next_1);

    if (pCeltFetched)
        *pCeltFetched = 0;
    return S_FALSE;

}

static HRESULT WINAPI EnumVARIANT_Skip(
    IEnumVARIANT *This,
    ULONG celt)
{
    ok(0, "EnumVariant_Skip: unexpected call\n");
    return E_NOTIMPL;
}
static HRESULT WINAPI EnumVARIANT_Reset(
    IEnumVARIANT *This)
{
    CHECK_EXPECT(enumvariant_reset);
    EnumVARIANT_index = 0;
    return S_OK;
}

static HRESULT WINAPI EnumVARIANT_Clone(
    IEnumVARIANT *This,
    IEnumVARIANT **ppEnum)
{
    ok(0, "EnumVariant_Clone: unexpected call\n");
    return E_NOTIMPL;
}

static IEnumVARIANTVtbl testEnumVARIANTVtbl = {
    EnumVARIANT_QueryInterface,
    EnumVARIANT_AddRef,
    EnumVARIANT_Release,
    EnumVARIANT_Next,
    EnumVARIANT_Skip,
    EnumVARIANT_Reset,
    EnumVARIANT_Clone
};

static IEnumVARIANT testEnumVARIANT = { &testEnumVARIANTVtbl };

static HRESULT WINAPI sp_QueryInterface(IServiceProvider *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IServiceProvider, riid)) {
        *ppv = iface;
        return S_OK;
    }

    ok(0, "unexpected call %s\n", wine_dbgstr_guid(riid));
    *ppv = NULL;
    return E_NOTIMPL;
}

static ULONG WINAPI sp_AddRef(IServiceProvider *iface)
{
    return 2;
}

static ULONG WINAPI sp_Release(IServiceProvider *iface)
{
    return 1;
}

static HRESULT WINAPI sp_QueryService(IServiceProvider *iface, REFGUID guidService, REFIID riid, void **ppv)
{
    ok(0, "unexpected call %s\n", wine_dbgstr_guid(guidService));
    *ppv = NULL;
    return E_NOTIMPL;
}

static const IServiceProviderVtbl sp_vtbl = {
    sp_QueryInterface,
    sp_AddRef,
    sp_Release,
    sp_QueryService
};

static IServiceProvider sp_obj = { &sp_vtbl };

static HRESULT WINAPI DispatchEx_QueryInterface(IDispatchEx *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(riid, &IID_IUnknown)
       || IsEqualGUID(riid, &IID_IDispatch)
       || IsEqualGUID(riid, &IID_IDispatchEx))
        *ppv = iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*ppv);
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

static HRESULT WINAPI DispatchEx_GetDispID(IDispatchEx *iface, BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_InvokeEx(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *res, EXCEPINFO *pei, IServiceProvider *pspCaller)
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

static HRESULT WINAPI testObj_Invoke(IDispatchEx *iface, DISPID id,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
                            VARIANT *pvarRes, EXCEPINFO *pei, UINT *puArgErr)
{
    switch(id) {
    case DISPID_NEWENUM:
        ok(wFlags == (DISPATCH_METHOD | DISPATCH_PROPERTYGET), "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pvarRes != NULL, "pvarRes == NULL\n");
        ok(pei == NULL, "pei != NULL\n");

        CHECK_EXPECT(testobj_newenum);
        V_VT(pvarRes) = VT_DISPATCH;
        V_DISPATCH(pvarRes) = (IDispatch*)&testEnumVARIANT;
        return S_OK;
    }

    ok(0, "unexpected call %lx\n", id);
    return DISP_E_MEMBERNOTFOUND;
}

static HRESULT WINAPI testObj_GetDispID(IDispatchEx *iface, BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    if(!lstrcmpW(bstrName, L"prop")) {
        CHECK_EXPECT(testobj_prop_d);
        test_grfdex(grfdex, fdexNameCaseSensitive);
        *pid = DISPID_TESTOBJ_PROP;
        return S_OK;
    }
    if(!lstrcmpW(bstrName, L"withProp")) {
        CHECK_EXPECT(testobj_withprop_d);
        test_grfdex(grfdex, fdexNameCaseSensitive|fdexNameImplicit);
        *pid = DISPID_TESTOBJ_WITHPROP;
        return S_OK;
    }
    if(!lstrcmpW(bstrName, L"noprop")) {
        CHECK_EXPECT(testobj_noprop_d);
        test_grfdex(grfdex, fdexNameCaseSensitive);
        return DISP_E_UNKNOWNNAME;
    }
    if(!lstrcmpW(bstrName, L"onlyDispID")) {
        if(strict_dispid_check)
            CHECK_EXPECT(testobj_onlydispid_d);
        test_grfdex(grfdex, fdexNameCaseSensitive);
        *pid = DISPID_TESTOBJ_ONLYDISPID;
        return S_OK;
    }
    if(!lstrcmpW(bstrName, L"toLocaleString")) {
        CHECK_EXPECT(testobj_tolocalestr_d);
        test_grfdex(grfdex, fdexNameCaseSensitive);
        *pid = DISPID_TESTOBJ_TOLOCALESTR;
        return S_OK;
    }
    if(!lstrcmpW(bstrName, L"notExists")) {
        CHECK_EXPECT(testobj_notexists_d);
        test_grfdex(grfdex, fdexNameCaseSensitive);
        return DISP_E_UNKNOWNNAME;
    }
    if(!lstrcmpW(bstrName, L"getIDFail")) {
        CHECK_EXPECT(testobj_getidfail_d);
        return E_FAIL;
    }

    ok(0, "unexpected name %s\n", wine_dbgstr_w(bstrName));
    return E_NOTIMPL;
}

static HRESULT WINAPI testObj_InvokeEx(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    ok(pspCaller != NULL, "pspCaller = NULL\n");

    switch(id) {
    case DISPID_VALUE:
        ok(pdp != NULL, "pdp == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pvarRes != NULL, "pvarRes == NULL\n");
        ok(V_VT(pvarRes) ==  VT_EMPTY, "V_VT(pvarRes) = %d\n", V_VT(pvarRes));
        ok(pei != NULL, "pei == NULL\n");

        switch(wFlags) {
        case INVOKE_PROPERTYGET:
            CHECK_EXPECT(testobj_value);
            ok(!pdp->rgvarg, "rgvarg != NULL\n");
            ok(!pdp->cArgs, "cArgs = %d\n", pdp->cArgs);
            break;
        case INVOKE_FUNC:
            ok(!pdp->cArgs, "cArgs = %d\n", pdp->cArgs);
            break;
        case INVOKE_FUNC|INVOKE_PROPERTYGET:
            ok(pdp->cArgs == 1, "cArgs = %d\n", pdp->cArgs);
            break;
        case DISPATCH_CONSTRUCT:
            CHECK_EXPECT(testobj_construct);
            ok(pdp->cArgs == 1, "cArgs = %d\n", pdp->cArgs);
            break;
        default:
            ok(0, "invalid flag (%x)\n", wFlags);
        }

        V_VT(pvarRes) = VT_I4;
        V_I4(pvarRes) = 1;
        return S_OK;
    case DISPID_TESTOBJ_ONLYDISPID:
        if(strict_dispid_check)
            CHECK_EXPECT(testobj_onlydispid_i);
        ok(wFlags == INVOKE_PROPERTYGET, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(!pdp->rgvarg, "rgvarg != NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(!pdp->cArgs, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pvarRes != NULL, "pvarRes == NULL\n");
        ok(V_VT(pvarRes) ==  VT_EMPTY, "V_VT(pvarRes) = %d\n", V_VT(pvarRes));
        ok(pei != NULL, "pei == NULL\n");
        return DISP_E_MEMBERNOTFOUND;
     case DISPID_TESTOBJ_WITHPROP:
        CHECK_EXPECT(testobj_withprop_i);

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
     case DISPID_TESTOBJ_TOLOCALESTR:
        CHECK_EXPECT(testobj_tolocalestr_i);

        ok(wFlags == DISPATCH_METHOD, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(!pdp->cArgs, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pvarRes != NULL, "pvarRes == NULL\n");
        ok(V_VT(pvarRes) == VT_EMPTY, "V_VT(pvarRes) = %d\n", V_VT(pvarRes));
        ok(pei != NULL, "pei == NULL\n");

        V_VT(pvarRes) = VT_I4;
        V_I4(pvarRes) = 1234;

        return S_OK;
    }

    ok(0, "unexpected call %lx\n", id);
    return DISP_E_MEMBERNOTFOUND;
}

static HRESULT WINAPI testObj_DeleteMemberByName(IDispatchEx *iface, BSTR bstrName, DWORD grfdex)
{
    if(!lstrcmpW(bstrName, L"deleteTest")) {
        CHECK_EXPECT(testobj_delete_test);
        test_grfdex(grfdex, fdexNameCaseSensitive);
        return S_OK;
    }
    if(!lstrcmpW(bstrName, L"noDeleteTest")) {
        CHECK_EXPECT(testobj_delete_nodelete);
        test_grfdex(grfdex, fdexNameCaseSensitive);
        return S_FALSE;
    }

    ok(0, "unexpected name %s\n", wine_dbgstr_w(bstrName));
    return E_FAIL;
}

static IDispatchExVtbl testObjVtbl = {
    DispatchEx_QueryInterface,
    DispatchEx_AddRef,
    DispatchEx_Release,
    DispatchEx_GetTypeInfoCount,
    DispatchEx_GetTypeInfo,
    DispatchEx_GetIDsOfNames,
    testObj_Invoke,
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

static HRESULT WINAPI testcallerobj_InvokeEx(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    ok(id == DISPID_VALUE, "id = %ld\n", id);
    ok(pdp != NULL, "pdp == NULL\n");
    ok(pvarRes != NULL, "pvarRes == NULL\n");
    ok(V_VT(pvarRes) == VT_EMPTY, "V_VT(pvarRes) = %d\n", V_VT(pvarRes));
    if(wFlags == DISPATCH_PROPERTYGET) {
        CHECK_EXPECT(test_caller_get);
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pei != NULL, "pei == NULL\n");
        ok(pspCaller != NULL, "pspCaller == NULL\n");
        ok(pspCaller != &sp_obj, "pspCaller == sp_obj\n");
        V_VT(pvarRes) = VT_DISPATCH;
        V_DISPATCH(pvarRes) = (IDispatch*)iface;
    }else if(pspCaller) {
        CHECK_EXPECT(test_caller_obj);
        ok(wFlags == DISPATCH_METHOD, "wFlags = %04x\n", wFlags);
        ok(pdp->rgdispidNamedArgs != NULL, "rgdispidNamedArgs == NULL\n");
        ok(pdp->cNamedArgs == 1, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pspCaller == &sp_obj, "pspCaller != sp_obj\n");
        V_VT(pvarRes) = VT_I4;
        V_I4(pvarRes) = 137;
    }else {
        CHECK_EXPECT(test_caller_null);
        ok(wFlags == DISPATCH_METHOD, "wFlags = %04x\n", wFlags);
        ok(pdp->rgdispidNamedArgs != NULL, "rgdispidNamedArgs == NULL\n");
        ok(pdp->cNamedArgs == 1, "cNamedArgs = %d\n", pdp->cNamedArgs);
        V_VT(pvarRes) = VT_I4;
        V_I4(pvarRes) = 42;
    }
    return S_OK;
}

static IDispatchExVtbl testcallerobj_vtbl = {
    DispatchEx_QueryInterface,
    DispatchEx_AddRef,
    DispatchEx_Release,
    DispatchEx_GetTypeInfoCount,
    DispatchEx_GetTypeInfo,
    DispatchEx_GetIDsOfNames,
    DispatchEx_Invoke,
    DispatchEx_GetDispID,
    testcallerobj_InvokeEx,
    DispatchEx_DeleteMemberByName,
    DispatchEx_DeleteMemberByDispID,
    DispatchEx_GetMemberProperties,
    DispatchEx_GetMemberName,
    DispatchEx_GetNextDispID,
    DispatchEx_GetNameSpaceParent
};

static IDispatchEx testcallerobj = { &testcallerobj_vtbl };

static LONG test_destr_ref;

static ULONG WINAPI testDestrObj_AddRef(IDispatchEx *iface)
{
    return ++test_destr_ref;
}

static ULONG WINAPI testDestrObj_Release(IDispatchEx *iface)
{
    if (!--test_destr_ref)
        CHECK_EXPECT(testdestrobj);
    return test_destr_ref;
}

static IDispatchExVtbl testDestrObjVtbl = {
    DispatchEx_QueryInterface,
    testDestrObj_AddRef,
    testDestrObj_Release,
    DispatchEx_GetTypeInfoCount,
    DispatchEx_GetTypeInfo,
    DispatchEx_GetIDsOfNames,
    DispatchEx_Invoke,
    DispatchEx_GetDispID,
    DispatchEx_InvokeEx,
    DispatchEx_DeleteMemberByName,
    DispatchEx_DeleteMemberByDispID,
    DispatchEx_GetMemberProperties,
    DispatchEx_GetMemberName,
    DispatchEx_GetNextDispID,
    DispatchEx_GetNameSpaceParent
};

static IDispatchEx testDestrObj = { &testDestrObjVtbl };

static HRESULT WINAPI dispexFunc_InvokeEx(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *res, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    ok(pspCaller != NULL, "pspCaller = NULL\n");

    switch(id) {
    case DISPID_VALUE:
        CHECK_EXPECT(dispexfunc_value);

        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(pdp->rgdispidNamedArgs != NULL, "rgdispidNamedArgs != NULL\n");
        ok(*pdp->rgdispidNamedArgs == DISPID_THIS, "*rgdispidNamedArgs = %ld\n", *pdp->rgdispidNamedArgs);
        ok(pdp->cArgs == 2, "cArgs = %d\n", pdp->cArgs);
        ok(pdp->cNamedArgs == 1, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(res != NULL, "res == NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        ok(V_VT(pdp->rgvarg+1) == VT_BOOL, "V_VT(pdp->rgvarg+1) = %d\n", V_VT(pdp->rgvarg+1));

        if(V_BOOL(pdp->rgvarg+1))
            /* NOTE: If called by Function.apply(), native doesn't set DISPATCH_PROPERTYGET flag. */
            todo_wine ok(wFlags == DISPATCH_METHOD, "wFlags = %x\n", wFlags);
        else
            ok(wFlags == (DISPATCH_PROPERTYGET|DISPATCH_METHOD), "wFlags = %x\n", wFlags);

        ok(V_VT(pdp->rgvarg) == VT_DISPATCH, "V_VT(pdp->rgvarg) = %d\n", V_VT(pdp->rgvarg));
        ok(V_DISPATCH(pdp->rgvarg) != NULL, "V_DISPATCH(pdp->rgvarg) == NULL\n");

        if(res)
            V_VT(res) = VT_NULL;
        return S_OK;
    default:
        ok(0, "unexpected call %lx\n", id);
        return DISP_E_MEMBERNOTFOUND;
    }
}

static IDispatchExVtbl dispexFuncVtbl = {
    DispatchEx_QueryInterface,
    DispatchEx_AddRef,
    DispatchEx_Release,
    DispatchEx_GetTypeInfoCount,
    DispatchEx_GetTypeInfo,
    DispatchEx_GetIDsOfNames,
    DispatchEx_Invoke,
    DispatchEx_GetDispID,
    dispexFunc_InvokeEx,
    DispatchEx_DeleteMemberByName,
    DispatchEx_DeleteMemberByDispID,
    DispatchEx_GetMemberProperties,
    DispatchEx_GetMemberName,
    DispatchEx_GetNextDispID,
    DispatchEx_GetNameSpaceParent
};

static IDispatchEx dispexFunc = { &dispexFuncVtbl };

static HRESULT WINAPI pureDisp_QueryInterface(IDispatchEx *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDispatch)) {
        *ppv = iface;
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

static HRESULT WINAPI pureDisp_GetIDsOfNames(IDispatchEx *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    ok(IsEqualGUID(riid, &IID_NULL), "Expected IID_NULL\n");
    ok(cNames==1, "cNames = %d\n", cNames);

    if(!lstrcmpW(*rgszNames, L"prop")) {
        CHECK_EXPECT(puredisp_prop_d);
        *rgDispId = DISPID_TESTOBJ_PROP;
        return S_OK;
    } else if(!lstrcmpW(*rgszNames, L"noprop")) {
        CHECK_EXPECT(puredisp_noprop_d);
        return DISP_E_UNKNOWNNAME;
    }

    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI pureDisp_Invoke(IDispatchEx *iface, DISPID dispIdMember, REFIID riid, LCID lcid,
        WORD wFlags, DISPPARAMS *pdp, VARIANT *res, EXCEPINFO *ei, UINT *puArgErr)
{
    ok(IsEqualGUID(&IID_NULL, riid), "unexpected riid\n");

    switch(dispIdMember) {
    case DISPID_VALUE:
        CHECK_EXPECT(puredisp_value);

        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(pdp->cArgs == 1, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(res != NULL, "res == NULL\n");
        ok(ei != NULL, "ei == NULL\n");
        ok(puArgErr != NULL, "puArgErr == NULL\n");

        ok(V_VT(pdp->rgvarg) == VT_BOOL, "V_VT(pdp->rgvarg) = %d\n", V_VT(pdp->rgvarg));

        if(V_BOOL(pdp->rgvarg))
            todo_wine ok(wFlags == DISPATCH_METHOD, "wFlags = %x\n", wFlags);
        else
            ok(wFlags == (DISPATCH_PROPERTYGET|DISPATCH_METHOD), "wFlags = %x\n", wFlags);

        if(res)
            V_VT(res) = VT_NULL;
        return S_OK;
    default:
        ok(0, "unexpected call\n");
        return E_NOTIMPL;
    }
}

static IDispatchExVtbl pureDispVtbl = {
    pureDisp_QueryInterface,
    DispatchEx_AddRef,
    DispatchEx_Release,
    DispatchEx_GetTypeInfoCount,
    DispatchEx_GetTypeInfo,
    pureDisp_GetIDsOfNames,
    pureDisp_Invoke
};

static IDispatchEx pureDisp = { &pureDispVtbl };

static HRESULT WINAPI BindEventHandler_QueryInterface(IBindEventHandler *iface, REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI BindEventHandler_AddRef(IBindEventHandler *iface)
{
    return 2;
}

static ULONG WINAPI BindEventHandler_Release(IBindEventHandler *iface)
{
    return 1;
}

static HRESULT WINAPI BindEventHandler_BindHandler(IBindEventHandler *iface, const WCHAR *event, IDispatch *disp)
{
    CHECK_EXPECT(BindHandler);
    ok(!lstrcmpW(event, L"eventName"), "event = %s\n", wine_dbgstr_w(event));
    ok(disp != NULL, "disp = NULL\n");
    return S_OK;
}

static const IBindEventHandlerVtbl BindEventHandlerVtbl = {
    BindEventHandler_QueryInterface,
    BindEventHandler_AddRef,
    BindEventHandler_Release,
    BindEventHandler_BindHandler
};

static IBindEventHandler BindEventHandler = { &BindEventHandlerVtbl };

static HRESULT WINAPI bindEventHandlerDisp_QueryInterface(IDispatchEx *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDispatch) || IsEqualGUID(riid, &IID_IDispatchEx)) {
        *ppv = iface;
        return S_OK;
    }

    if(IsEqualGUID(riid, &IID_IBindEventHandler)) {
        *ppv = &BindEventHandler;
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

static IDispatchExVtbl bindEventHandlerDispVtbl = {
    bindEventHandlerDisp_QueryInterface,
    DispatchEx_AddRef,
    DispatchEx_Release,
    DispatchEx_GetTypeInfoCount,
    DispatchEx_GetTypeInfo,
    DispatchEx_GetIDsOfNames,
    DispatchEx_Invoke,
    DispatchEx_GetDispID,
    DispatchEx_InvokeEx,
    DispatchEx_DeleteMemberByName,
    DispatchEx_DeleteMemberByDispID,
    DispatchEx_GetMemberProperties,
    DispatchEx_GetMemberName,
    DispatchEx_GetNextDispID,
    DispatchEx_GetNameSpaceParent
};

static IDispatchEx bindEventHandlerDisp = { &bindEventHandlerDispVtbl };

static HRESULT CALLBACK test_deferred_fill_in(struct tagEXCEPINFO *ei)
{
    ok(ei->pfnDeferredFillIn == test_deferred_fill_in, "pfnDeferredFillIn != test_deferred_fill_in\n");
    ok(!wcscmp(ei->bstrSource, L"source before defer"), "bstrSource = %s\n", wine_dbgstr_w(ei->bstrSource));
    ok(!wcscmp(ei->bstrDescription, L"desc before defer"), "bstrDescription = %s\n", wine_dbgstr_w(ei->bstrDescription));
    ok(!wcscmp(ei->bstrHelpFile, L"help before defer"), "bstrHelpFile = %s\n", wine_dbgstr_w(ei->bstrHelpFile));
    ok(ei->dwHelpContext == 1337, "dwHelpContext = %lu\n", ei->dwHelpContext);

    SysFreeString(ei->bstrSource);
    SysFreeString(ei->bstrDescription);
    SysFreeString(ei->bstrHelpFile);
    ei->pfnDeferredFillIn = NULL;
    ei->bstrSource = SysAllocString(L"source after defer");
    ei->bstrDescription = SysAllocString(L"desc after defer");
    ei->bstrHelpFile = SysAllocString(L"help after defer");
    ei->dwHelpContext = 1234567890;

    return E_FAIL;  /* return code ignored */
}

static HRESULT WINAPI Global_GetDispID(IDispatchEx *iface, BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    if(!lstrcmpW(bstrName, L"ok")) {
        test_grfdex(grfdex, fdexNameCaseSensitive);
        *pid = DISPID_GLOBAL_OK;
        return S_OK;
    }
    if(!lstrcmpW(bstrName, L"trace")) {
        test_grfdex(grfdex, fdexNameCaseSensitive);
        *pid = DISPID_GLOBAL_TRACE;
        return S_OK;
    }
    if(!lstrcmpW(bstrName, L"todo_wine_ok")) {
        test_grfdex(grfdex, fdexNameCaseSensitive);
        *pid = DISPID_GLOBAL_TODOWINE;
        return S_OK;
    }
    if(!lstrcmpW(bstrName, L"reportSuccess")) {
        CHECK_EXPECT(global_success_d);
        test_grfdex(grfdex, fdexNameCaseSensitive);
        *pid = DISPID_GLOBAL_REPORTSUCCESS;
        return S_OK;
    }
    if(!lstrcmpW(bstrName, L"testPropGet")) {
        CHECK_EXPECT(global_propget_d);
        test_grfdex(grfdex, fdexNameCaseSensitive);
        *pid = DISPID_GLOBAL_TESTPROPGET;
        return S_OK;
    }
    if(!lstrcmpW(bstrName, L"testPropPut")) {
        CHECK_EXPECT(global_propput_d);
        test_grfdex(grfdex, fdexNameCaseSensitive);
        *pid = DISPID_GLOBAL_TESTPROPPUT;
        return S_OK;
    }
    if(!lstrcmpW(bstrName, L"testPropPutRef")) {
        CHECK_EXPECT(global_propputref_d);
        test_grfdex(grfdex, fdexNameCaseSensitive);
        *pid = DISPID_GLOBAL_TESTPROPPUTREF;
        return S_OK;
    }
    if(!lstrcmpW(bstrName, L"testPropDelete")) {
        CHECK_EXPECT(global_propdelete_d);
        test_grfdex(grfdex, fdexNameCaseSensitive);
        *pid = DISPID_GLOBAL_TESTPROPDELETE;
        return S_OK;
    }
    if(!lstrcmpW(bstrName, L"testNoPropDelete")) {
        CHECK_EXPECT(global_nopropdelete_d);
        test_grfdex(grfdex, fdexNameCaseSensitive);
        *pid = DISPID_GLOBAL_TESTNOPROPDELETE;
        return S_OK;
    }
    if(!lstrcmpW(bstrName, L"testPropDeleteError")) {
        CHECK_EXPECT(global_propdeleteerror_d);
        test_grfdex(grfdex, fdexNameCaseSensitive);
        *pid = DISPID_GLOBAL_TESTPROPDELETEERROR;
        return S_OK;
    }
    if(!lstrcmpW(bstrName, L"getVT")) {
        test_grfdex(grfdex, fdexNameCaseSensitive);
        *pid = DISPID_GLOBAL_GETVT;
        return S_OK;
    }
    if(!lstrcmpW(bstrName, L"testObj")) {
        test_grfdex(grfdex, fdexNameCaseSensitive);
        *pid = DISPID_GLOBAL_TESTOBJ;
        return S_OK;
    }
    if(!lstrcmpW(bstrName, L"getNullBSTR")) {
        *pid = DISPID_GLOBAL_GETNULLBSTR;
        return S_OK;
    }
    if(!lstrcmpW(bstrName, L"isNullBSTR")) {
        *pid = DISPID_GLOBAL_ISNULLBSTR;
        return S_OK;
    }
    if(!lstrcmpW(bstrName, L"nullDisp")) {
        *pid = DISPID_GLOBAL_NULL_DISP;
        return S_OK;
    }
    if(!lstrcmpW(bstrName, L"notExists")) {
        CHECK_EXPECT(global_notexists_d);
        test_grfdex(grfdex, fdexNameCaseSensitive);
        return DISP_E_UNKNOWNNAME;
    }

    if(!lstrcmpW(bstrName, L"testThis")) {
        test_grfdex(grfdex, fdexNameCaseSensitive);
        *pid = DISPID_GLOBAL_TESTTHIS;
        return S_OK;
    }

    if(!lstrcmpW(bstrName, L"testThis2")) {
        test_grfdex(grfdex, fdexNameCaseSensitive);
        *pid = DISPID_GLOBAL_TESTTHIS2;
        return S_OK;
    }

    if(!lstrcmpW(bstrName, L"invokeVersion")) {
        test_grfdex(grfdex, fdexNameCaseSensitive);
        *pid = DISPID_GLOBAL_INVOKEVERSION;
        return S_OK;
    }
    if(!lstrcmpW(bstrName, L"createArray")) {
        test_grfdex(grfdex, fdexNameCaseSensitive);
        *pid = DISPID_GLOBAL_CREATEARRAY;
        return S_OK;
    }
    if(!lstrcmpW(bstrName, L"propGetFunc")) {
        test_grfdex(grfdex, fdexNameCaseSensitive);
        *pid = DISPID_GLOBAL_PROPGETFUNC;
        return S_OK;
    }
    if(!lstrcmpW(bstrName, L"objectFlag")) {
        test_grfdex(grfdex, fdexNameCaseSensitive);
        *pid = DISPID_GLOBAL_OBJECT_FLAG;
        return S_OK;
    }

    if(!lstrcmpW(bstrName, L"isWin64")) {
        test_grfdex(grfdex, fdexNameCaseSensitive);
        *pid = DISPID_GLOBAL_ISWIN64;
        return S_OK;
    }

    if(!lstrcmpW(bstrName, L"pureDisp")) {
        test_grfdex(grfdex, fdexNameCaseSensitive);
        *pid = DISPID_GLOBAL_PUREDISP;
        return S_OK;
    }

    if(!lstrcmpW(bstrName, L"propArgPutG")) {
        CHECK_EXPECT(global_propargput_d);
        test_grfdex(grfdex, fdexNameCaseSensitive);
        *pid = DISPID_GLOBAL_PROPARGPUT;
        return S_OK;
    }

    if(!lstrcmpW(bstrName, L"propArgPutOp")) {
        CHECK_EXPECT(global_propargputop_d);
        test_grfdex(grfdex, fdexNameCaseSensitive);
        *pid = DISPID_GLOBAL_PROPARGPUTOP;
        return S_OK;
    }

    if(!lstrcmpW(bstrName, L"throwInt")) {
        test_grfdex(grfdex, fdexNameCaseSensitive);
        *pid = DISPID_GLOBAL_THROWINT;
        return S_OK;
    }

    if(!lstrcmpW(bstrName, L"throwEI")) {
        test_grfdex(grfdex, fdexNameCaseSensitive);
        *pid = DISPID_GLOBAL_THROWEI;
        return S_OK;
    }

    if(!lstrcmpW(bstrName, L"propArgPutO")) {
        CHECK_EXPECT(global_propargput_d);
        test_grfdex(grfdex, fdexNameEnsure|fdexNameCaseSensitive);
        *pid = DISPID_GLOBAL_PROPARGPUT;
        return S_OK;
    }

    if(!lstrcmpW(bstrName, L"shortProp")) {
        *pid = DISPID_GLOBAL_SHORTPROP;
        return S_OK;
    }

    if(!lstrcmpW(bstrName, L"getShort")) {
        *pid = DISPID_GLOBAL_GETSHORT;
        return S_OK;
    }

    if(!lstrcmpW(bstrName, L"v_date")) {
        *pid = DISPID_GLOBAL_VDATE;
        return S_OK;
    }

    if(!lstrcmpW(bstrName, L"v_cy")) {
        *pid = DISPID_GLOBAL_VCY;
        return S_OK;
    }

    if(!lstrcmpW(bstrName, L"testArgTypes")) {
        *pid = DISPID_GLOBAL_TESTARGTYPES;
        return S_OK;
    }

    if(!lstrcmpW(bstrName, L"intProp")) {
        *pid = DISPID_GLOBAL_INTPROP;
        return S_OK;
    }

    if(!lstrcmpW(bstrName, L"dispUnk")) {
        *pid = DISPID_GLOBAL_DISPUNK;
        return S_OK;
    }

    if(!lstrcmpW(bstrName, L"testRes")) {
        *pid = DISPID_GLOBAL_TESTRES;
        return S_OK;
    }

    if(!lstrcmpW(bstrName, L"testNoRes")) {
        *pid = DISPID_GLOBAL_TESTNORES;
        return S_OK;
    }

    if(!lstrcmpW(bstrName, L"dispexFunc")) {
        *pid = DISPID_GLOBAL_DISPEXFUNC;
        return S_OK;
    }

    if(!lstrcmpW(bstrName, L"getScriptState")) {
        *pid = DISPID_GLOBAL_GETSCRIPTSTATE;
        return S_OK;
    }

    if(!lstrcmpW(bstrName, L"bindEventHandler")) {
        *pid = DISPID_GLOBAL_BINDEVENTHANDLER;
        return S_OK;
    }

    if(!lstrcmpW(bstrName, L"testEnumObj")) {
        *pid = DISPID_GLOBAL_TESTENUMOBJ;
        return S_OK;
    }

    if(!lstrcmpW(bstrName, L"callEval")) {
        *pid = DISPID_GLOBAL_CALLEVAL;
        return S_OK;
    }

    if(!lstrcmpW(bstrName, L"testDestrObj")) {
        *pid = DISPID_GLOBAL_TESTDESTROBJ;
        return S_OK;
    }

    if(strict_dispid_check && lstrcmpW(bstrName, L"t"))
        ok(0, "unexpected call %s\n", wine_dbgstr_w(bstrName));
    return DISP_E_UNKNOWNNAME;
}

static HRESULT WINAPI Global_InvokeEx(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    ok(pspCaller != NULL, "pspCaller = NULL\n");

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

        ok(V_VT(pdp->rgvarg) == VT_BSTR, "V_VT(pdp->rgvarg) = %d\n", V_VT(pdp->rgvarg));
        ok(V_VT(pdp->rgvarg+1) == VT_BOOL, "V_VT(pdp->rgvarg+1) = %d\n", V_VT(pdp->rgvarg+1));
#ifndef __REACTOS__ // Fails on Windows 2003
        ok(V_BOOL(pdp->rgvarg+1), "%s: %s\n", test_name, wine_dbgstr_w(V_BSTR(pdp->rgvarg)));
#endif

        return S_OK;

    case DISPID_GLOBAL_TODOWINE:
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

        ok(V_VT(pdp->rgvarg) == VT_BSTR, "V_VT(pdp->rgvarg) = %d\n", V_VT(pdp->rgvarg));
        ok(V_VT(pdp->rgvarg+1) == VT_BOOL, "V_VT(pdp->rgvarg+1) = %d\n", V_VT(pdp->rgvarg+1));
        todo_wine ok(V_BOOL(pdp->rgvarg+1), "%s: %s\n", test_name, wine_dbgstr_w(V_BSTR(pdp->rgvarg)));

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

        ok(V_VT(pdp->rgvarg) == VT_BSTR, "V_VT(pdp->rgvarg) = %d\n", V_VT(pdp->rgvarg));
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
        if(!testing_expr)
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
        ok(pdp->rgdispidNamedArgs[0] == DISPID_PROPERTYPUT, "pdp->rgdispidNamedArgs[0] = %ld\n", pdp->rgdispidNamedArgs[0]);
        ok(!pvarRes, "pvarRes != NULL\n");

        ok(V_VT(pdp->rgvarg) == VT_I4, "V_VT(pdp->rgvarg)=%d\n", V_VT(pdp->rgvarg));
        ok(V_I4(pdp->rgvarg) == 1, "V_I4(pdp->rgvarg)=%ld\n", V_I4(pdp->rgvarg));
        return S_OK;

    case DISPID_GLOBAL_TESTPROPPUTREF:
        CHECK_EXPECT(global_propputref_i);

        ok(wFlags == (INVOKE_PROPERTYPUT|INVOKE_PROPERTYPUTREF), "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(pdp->rgdispidNamedArgs != NULL, "rgdispidNamedArgs == NULL\n");
        ok(pdp->cArgs == 1, "cArgs = %d\n", pdp->cArgs);
        ok(pdp->cNamedArgs == 1, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pdp->rgdispidNamedArgs[0] == DISPID_PROPERTYPUT, "pdp->rgdispidNamedArgs[0] = %ld\n", pdp->rgdispidNamedArgs[0]);
        ok(!pvarRes, "pvarRes != NULL\n");

        ok(V_VT(pdp->rgvarg) == VT_DISPATCH, "V_VT(pdp->rgvarg)=%d\n", V_VT(pdp->rgvarg));
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
            V_BSTR(pvarRes) = SysAllocString(L"VT_EMPTY");
            break;
        case VT_NULL:
            V_BSTR(pvarRes) = SysAllocString(L"VT_NULL");
            break;
        case VT_I4:
            V_BSTR(pvarRes) = SysAllocString(L"VT_I4");
            break;
        case VT_R8:
            V_BSTR(pvarRes) = SysAllocString(L"VT_R8");
            break;
        case VT_BSTR:
            V_BSTR(pvarRes) = SysAllocString(L"VT_BSTR");
            break;
        case VT_DISPATCH:
            V_BSTR(pvarRes) = SysAllocString(L"VT_DISPATCH");
            break;
        case VT_BOOL:
            V_BSTR(pvarRes) = SysAllocString(L"VT_BOOL");
            break;
        case VT_ARRAY|VT_VARIANT:
            V_BSTR(pvarRes) = SysAllocString(L"VT_ARRAY|VT_VARIANT");
            break;
        case VT_DATE:
            V_BSTR(pvarRes) = SysAllocString(L"VT_DATE");
            break;
        default:
            ok(0, "unknown vt %d\n", V_VT(pdp->rgvarg));
            return E_FAIL;
        }

        return S_OK;

    case DISPID_GLOBAL_TESTRES:
        ok(pvarRes != NULL, "pvarRes = NULL\n");
        if(pvarRes) {
            V_VT(pvarRes) = VT_BOOL;
            V_BOOL(pvarRes) = VARIANT_TRUE;
        }
        return S_OK;

    case DISPID_GLOBAL_TESTNORES:
        ok(!pvarRes, "pvarRes != NULL\n");
        if(pvarRes)
            V_VT(pvarRes) = VT_NULL;
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

    case DISPID_GLOBAL_TESTDESTROBJ:
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
        V_DISPATCH(pvarRes) = (IDispatch*)&testDestrObj;
        IDispatch_AddRef(V_DISPATCH(pvarRes));
        return S_OK;

    case DISPID_GLOBAL_PUREDISP:
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
        V_DISPATCH(pvarRes) = (IDispatch*)&pureDisp;
        return S_OK;

    case DISPID_GLOBAL_DISPEXFUNC:
        V_VT(pvarRes) = VT_DISPATCH;
        V_DISPATCH(pvarRes) = (IDispatch*)&dispexFunc;
        return S_OK;

    case DISPID_GLOBAL_GETNULLBSTR:
        if(pvarRes) {
            V_VT(pvarRes) = VT_BSTR;
            V_BSTR(pvarRes) = NULL;
        }
        return S_OK;

    case DISPID_GLOBAL_ISNULLBSTR:
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(pdp->cArgs == 1, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pvarRes != NULL, "pvarRes == NULL\n");
        ok(V_VT(pvarRes) ==  VT_EMPTY, "V_VT(pvarRes) = %d\n", V_VT(pvarRes));
        ok(pei != NULL, "pei == NULL\n");
        ok(V_VT(pdp->rgvarg) == VT_BSTR, "V_VT(pdp->rgvarg) = %d\n", V_VT(pdp->rgvarg));

        V_VT(pvarRes) = VT_BOOL;
        V_BOOL(pvarRes) = V_BSTR(pdp->rgvarg) ? VARIANT_FALSE : VARIANT_TRUE;
        return S_OK;

    case DISPID_GLOBAL_ISWIN64:
        if(pvarRes) {
            V_VT(pvarRes) = VT_BOOL;
            V_BOOL(pvarRes) = sizeof(void*) == 8 ? VARIANT_TRUE : VARIANT_FALSE;
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

        ok(V_VT(pdp->rgvarg) == VT_DISPATCH, "V_VT(pdp->rgvarg) = %d\n", V_VT(pdp->rgvarg));
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

    case DISPID_GLOBAL_CREATEARRAY: {
        SAFEARRAYBOUND bound[2];
        VARIANT *data;
        int i,j;

        ok(wFlags == INVOKE_FUNC, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(!pdp->cArgs, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pvarRes != NULL, "pvarRes == NULL\n");
        ok(V_VT(pvarRes) ==  VT_EMPTY, "V_VT(pvarRes) = %d\n", V_VT(pvarRes));
        ok(pei != NULL, "pei == NULL\n");

        bound[0].lLbound = 0;
        bound[0].cElements = 5;
        bound[1].lLbound = 2;
        bound[1].cElements = 2;

        V_VT(pvarRes) = VT_ARRAY|VT_VARIANT;
        V_ARRAY(pvarRes) = SafeArrayCreate(VT_VARIANT, 2, bound);

        SafeArrayAccessData(V_ARRAY(pvarRes), (void**)&data);
        for(i=0; i<5; i++) {
            for(j=2; j<4; j++) {
                V_VT(data) = VT_I4;
                V_I4(data) = i*10+j;
                data++;
            }
        }
        SafeArrayUnaccessData(V_ARRAY(pvarRes));

        return S_OK;
    }

    case DISPID_GLOBAL_PROPGETFUNC:
        switch(wFlags) {
        case INVOKE_FUNC:
            CHECK_EXPECT(invoke_func);
            break;
        case INVOKE_FUNC|INVOKE_PROPERTYGET:
            ok(pdp->cArgs != 0, "pdp->cArgs = %d\n", pdp->cArgs);
            ok(pvarRes != NULL, "pdp->pvarRes == NULL\n");
            break;
        default:
            ok(0, "invalid flag (%x)\n", wFlags);
        }

        ok(pdp != NULL, "pdp == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pei != NULL, "pei == NULL\n");

        if(pvarRes) {
            ok(V_VT(pvarRes) ==  VT_EMPTY, "V_VT(pvarRes) = %d\n", V_VT(pvarRes));
            V_VT(pvarRes) = VT_I4;
            V_I4(pvarRes) = pdp->cArgs;
        }

        return S_OK;

    case DISPID_GLOBAL_GETSCRIPTSTATE: {
        SCRIPTSTATE state;
        HRESULT hres;

        hres = IActiveScript_GetScriptState(script_engine, &state);
        ok(hres == S_OK, "GetScriptState failed: %08lx\n", hres);

        V_VT(pvarRes) = VT_I4;
        V_I4(pvarRes) = state;
        return S_OK;
    }

    case DISPID_GLOBAL_BINDEVENTHANDLER:
        V_VT(pvarRes) = VT_DISPATCH;
        V_DISPATCH(pvarRes) = (IDispatch*)&bindEventHandlerDisp;
        return S_OK;

    case DISPID_GLOBAL_PROPARGPUT:
        CHECK_EXPECT(global_propargput_i);
        ok(wFlags == INVOKE_PROPERTYPUT, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg != NULL\n");
        ok(pdp->rgdispidNamedArgs != NULL, "rgdispidNamedArgs == NULL\n");
        ok(pdp->cArgs == 3, "cArgs = %d\n", pdp->cArgs);
        ok(pdp->cNamedArgs == 1, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pdp->rgdispidNamedArgs[0] == DISPID_PROPERTYPUT, "pdp->rgdispidNamedArgs[0] = %ld\n", pdp->rgdispidNamedArgs[0]);
        ok(!pvarRes, "pvarRes != NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        ok(V_VT(pdp->rgvarg) == VT_I4, "V_VT(pdp->rgvarg) = %d\n", V_VT(pdp->rgvarg));
        ok(V_I4(pdp->rgvarg) == 2, "V_I4(pdp->rgvarg) = %ld\n", V_I4(pdp->rgvarg));

        ok(V_VT(pdp->rgvarg+1) == VT_I4, "V_VT(pdp->rgvarg+1) = %d\n", V_VT(pdp->rgvarg+1));
        ok(V_I4(pdp->rgvarg+1) == 1, "V_I4(pdp->rgvarg+1) = %ld\n", V_I4(pdp->rgvarg+1));

        ok(V_VT(pdp->rgvarg+2) == VT_I4, "V_VT(pdp->rgvarg+2) = %d\n", V_VT(pdp->rgvarg+2));
        ok(V_I4(pdp->rgvarg+2) == 0, "V_I4(pdp->rgvarg+2) = %ld\n", V_I4(pdp->rgvarg+2));
        return S_OK;

    case DISPID_GLOBAL_PROPARGPUTOP:
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg != NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        switch(wFlags) {
        case INVOKE_PROPERTYGET | INVOKE_FUNC:
            CHECK_EXPECT(global_propargputop_get_i);

            ok(pdp->cNamedArgs == 0, "cNamedArgs = %d\n", pdp->cNamedArgs);
            ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
            ok(pdp->cArgs == 2, "cArgs = %d\n", pdp->cArgs);
            ok(pdp->cNamedArgs == 0, "cNamedArgs = %d\n", pdp->cNamedArgs);
            ok(pvarRes != NULL, "pvarRes = NULL\n");

            ok(V_VT(pdp->rgvarg) == VT_I4, "V_VT(pdp->rgvarg) = %d\n", V_VT(pdp->rgvarg));
            ok(V_I4(pdp->rgvarg) == 1, "V_I4(pdp->rgvarg) = %ld\n", V_I4(pdp->rgvarg));

            ok(V_VT(pdp->rgvarg+1) == VT_I4, "V_VT(pdp->rgvarg+1) = %d\n", V_VT(pdp->rgvarg+1));
            ok(V_I4(pdp->rgvarg+1) == 0, "V_I4(pdp->rgvarg+1) = %ld\n", V_I4(pdp->rgvarg+1));

            V_VT(pvarRes) = VT_I4;
            V_I4(pvarRes) = 6;
            break;
        case INVOKE_PROPERTYPUT:
            CHECK_EXPECT(global_propargputop_put_i);

            ok(pdp->cNamedArgs == 1, "cNamedArgs = %d\n", pdp->cNamedArgs);
            ok(pdp->rgdispidNamedArgs[0] == DISPID_PROPERTYPUT, "pdp->rgdispidNamedArgs[0] = %ld\n", pdp->rgdispidNamedArgs[0]);
            ok(pdp->rgdispidNamedArgs != NULL, "rgdispidNamedArgs == NULL\n");
            ok(pdp->cArgs == 3, "cArgs = %d\n", pdp->cArgs);
            ok(pdp->cNamedArgs == 1, "cNamedArgs = %d\n", pdp->cNamedArgs);
            ok(!pvarRes, "pvarRes != NULL\n");

            ok(V_VT(pdp->rgvarg) == VT_I4, "V_VT(pdp->rgvarg) = %d\n", V_VT(pdp->rgvarg));
            ok(V_I4(pdp->rgvarg) == 8, "V_I4(pdp->rgvarg) = %ld\n", V_I4(pdp->rgvarg));

            ok(V_VT(pdp->rgvarg+1) == VT_I4, "V_VT(pdp->rgvarg+1) = %d\n", V_VT(pdp->rgvarg+1));
            ok(V_I4(pdp->rgvarg+1) == 1, "V_I4(pdp->rgvarg+1) = %ld\n", V_I4(pdp->rgvarg+1));

            ok(V_VT(pdp->rgvarg+2) == VT_I4, "V_VT(pdp->rgvarg+2) = %d\n", V_VT(pdp->rgvarg+2));
            ok(V_I4(pdp->rgvarg+2) == 0, "V_I4(pdp->rgvarg+2) = %ld\n", V_I4(pdp->rgvarg+2));
            break;
        default:
            ok(0, "wFlags = %x\n", wFlags);
        }

        return S_OK;

    case DISPID_GLOBAL_OBJECT_FLAG: {
        IDispatchEx *dispex;
        BSTR str;
        HRESULT hres;

        hres = IDispatch_QueryInterface(script_disp, &IID_IDispatchEx, (void**)&dispex);
        ok(hres == S_OK, "hres = %lx\n", hres);

        str = SysAllocString(L"Object");
        hres = IDispatchEx_GetDispID(dispex, str, fdexNameCaseSensitive, &id);
        SysFreeString(str);
        ok(hres == S_OK, "hres = %lx\n", hres);

        hres = IDispatchEx_InvokeEx(dispex, id, lcid, DISPATCH_METHOD, pdp, NULL, pei, pspCaller);
        ok(hres == S_OK, "hres = %lx\n", hres);

        V_VT(pvarRes) = VT_EMPTY;
        hres = IDispatchEx_InvokeEx(dispex, id, lcid, DISPATCH_METHOD, pdp, pvarRes, pei, pspCaller);
        ok(hres == S_OK, "hres = %lx\n", hres);
        ok(V_VT(pvarRes) == VT_DISPATCH, "V_VT(pvarRes) = %d\n", V_VT(pvarRes));
        VariantClear(pvarRes);

        hres = IDispatchEx_InvokeEx(dispex, id, lcid, DISPATCH_METHOD|DISPATCH_PROPERTYGET, pdp, NULL, pei, pspCaller);
        ok(hres == S_OK, "hres = %lx\n", hres);

        V_VT(pvarRes) = VT_EMPTY;
        hres = IDispatchEx_InvokeEx(dispex, id, lcid, DISPATCH_CONSTRUCT, pdp, pvarRes, pei, pspCaller);
        ok(hres == S_OK, "hres = %lx\n", hres);
        ok(V_VT(pvarRes) == VT_DISPATCH, "V_VT(pvarRes) = %d\n", V_VT(pvarRes));
        VariantClear(pvarRes);

        hres = IDispatchEx_InvokeEx(dispex, id, lcid, DISPATCH_CONSTRUCT, pdp, NULL, pei, pspCaller);
        ok(hres == S_OK, "hres = %lx\n", hres);

        V_VT(pvarRes) = VT_EMPTY;
        hres = IDispatchEx_InvokeEx(dispex, id, lcid, DISPATCH_CONSTRUCT|DISPATCH_PROPERTYGET, pdp, pvarRes, pei, pspCaller);
        ok(hres == E_INVALIDARG, "hres = %lx\n", hres);

        V_VT(pvarRes) = VT_EMPTY;
        hres = IDispatchEx_InvokeEx(dispex, id, lcid, wFlags, pdp, pvarRes, pei, pspCaller);
        ok(hres == S_OK, "hres = %lx\n", hres);
        ok(V_VT(pvarRes) == VT_DISPATCH, "V_VT(pvarRes) = %d\n", V_VT(pvarRes));
        IDispatchEx_Release(dispex);
        return S_OK;
    }
    case DISPID_GLOBAL_SHORTPROP:
    case DISPID_GLOBAL_GETSHORT:
        V_VT(pvarRes) = VT_I2;
        V_I2(pvarRes) = 10;
        return S_OK;

    case DISPID_GLOBAL_VDATE:
        ok(wFlags == (DISPATCH_METHOD|DISPATCH_PROPERTYGET), "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->cArgs == 1, "cArgs = %d\n", pdp->cArgs);
        ok(pvarRes != NULL, "pvarRes != NULL\n");
        V_VT(pvarRes) = VT_DATE;
        switch(V_VT(pdp->rgvarg))
        {
        case VT_I4:
            V_DATE(pvarRes) = V_I4(pdp->rgvarg);
            break;
        case VT_R8:
            V_DATE(pvarRes) = V_R8(pdp->rgvarg);
            break;
        default:
            ok(0, "vt = %u\n", V_VT(pdp->rgvarg));
            return E_INVALIDARG;
        }
        return S_OK;

    case DISPID_GLOBAL_VCY:
        ok(wFlags == (DISPATCH_METHOD|DISPATCH_PROPERTYGET), "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->cArgs == 1, "cArgs = %d\n", pdp->cArgs);
        ok(pvarRes != NULL, "pvarRes != NULL\n");
        V_VT(pvarRes) = VT_CY;
        switch(V_VT(pdp->rgvarg))
        {
        case VT_I4:
            V_CY(pvarRes).int64 = V_I4(pdp->rgvarg);
            break;
        case VT_R8:
            V_CY(pvarRes).int64 = V_R8(pdp->rgvarg);
            break;
        default:
            ok(0, "vt = %u\n", V_VT(pdp->rgvarg));
            return E_INVALIDARG;
        }
        return S_OK;

    case DISPID_GLOBAL_INTPROP:
        V_VT(pvarRes) = VT_INT;
        V_INT(pvarRes) = 22;
        return S_OK;

    case DISPID_GLOBAL_DISPUNK:
        V_VT(pvarRes) = VT_UNKNOWN;
        V_UNKNOWN(pvarRes) = (IUnknown*)&testObj;
        return S_OK;

    case DISPID_GLOBAL_TESTARGTYPES: {
        VARIANT args[10], v;
        DISPPARAMS dp = {args, NULL, ARRAY_SIZE(args), 0};
        HRESULT hres;

        CHECK_EXPECT(global_testargtypes_i);
        ok(wFlags == DISPATCH_METHOD, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg != NULL\n");
        ok(pdp->cArgs == 6, "cArgs = %d\n", pdp->cArgs);
        ok(!pvarRes, "pvarRes != NULL\n");

        ok(V_VT(pdp->rgvarg+1) == VT_I4, "V_VT(pdp->rgvarg+1) = %d\n", V_VT(pdp->rgvarg+1));
        ok(V_I4(pdp->rgvarg+1) == 10, "V_I4(pdp->rgvarg+1) = %ld\n", V_I4(pdp->rgvarg+1));

        ok(V_VT(pdp->rgvarg+2) == VT_I4, "V_VT(pdp->rgvarg+2) = %d\n", V_VT(pdp->rgvarg+2));
        ok(V_I4(pdp->rgvarg+2) == 10, "V_I4(pdp->rgvarg+2) = %ld\n", V_I4(pdp->rgvarg+2));

        ok(V_VT(pdp->rgvarg+3) == VT_I4, "V_VT(pdp->rgvarg+3) = %d\n", V_VT(pdp->rgvarg+3));
        ok(V_I4(pdp->rgvarg+3) == 22, "V_I4(pdp->rgvarg+3) = %ld\n", V_I4(pdp->rgvarg+3));

        ok(V_VT(pdp->rgvarg+4) == VT_I4, "V_VT(pdp->rgvarg+4) = %d\n", V_VT(pdp->rgvarg+4));
        ok(V_I4(pdp->rgvarg+4) == 22, "V_I4(pdp->rgvarg+4) = %ld\n", V_I4(pdp->rgvarg+4));

        ok(V_VT(pdp->rgvarg+5) == VT_DISPATCH, "V_VT(pdp->rgvarg+5) = %d\n", V_VT(pdp->rgvarg+5));
        ok(V_DISPATCH(pdp->rgvarg+5) == (IDispatch*)&testObj, "V_DISPATCH(pdp->rgvarg+5) != testObj\n");

        ok(V_VT(pdp->rgvarg) == VT_DISPATCH, "V_VT(pdp->rgvarg) = %d\n", V_VT(pdp->rgvarg));

        V_VT(args) = VT_I2;
        V_I2(args) = 2;
        V_VT(args+1) = VT_INT;
        V_INT(args+1) = 22;
        V_VT(args+2) = VT_UNKNOWN;
        V_UNKNOWN(args+2) = (IUnknown*)&testObj;
        V_VT(args+3) = VT_UNKNOWN;
        V_UNKNOWN(args+3) = NULL;
        V_VT(args+4) = VT_UI4;
        V_UI4(args+4) = 0xffffffff;
        V_VT(args+5) = VT_BYREF|VT_VARIANT;
        V_VARIANTREF(args+5) = &v;
        V_VT(args+6) = VT_R4;
        V_R4(args+6) = 0.5;
        V_VT(args+7) = VT_UI2;
        V_UI2(args+7) = 3;
        V_VT(args+8) = VT_UI1;
        V_UI1(args+8) = 4;
        V_VT(args+9) = VT_I1;
        V_I1(args+9) = 5;
        V_VT(&v) = VT_I4;
        V_I4(&v) = 2;
        hres = IDispatch_Invoke(V_DISPATCH(pdp->rgvarg), DISPID_VALUE, &IID_NULL, 0, DISPATCH_METHOD, &dp, NULL, NULL, NULL);
        ok(hres == S_OK, "Invoke failed: %08lx\n", hres);

        return S_OK;
    }

    case DISPID_GLOBAL_CALLEVAL: {
        IDispatchEx *eval_func;
        DISPPARAMS params;
        VARIANT arg, res;
        HRESULT hres;

        CHECK_EXPECT(global_calleval_i);

        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(pdp->cArgs == 1, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pvarRes == NULL, "pvarRes != NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        ok(V_VT(pdp->rgvarg) == VT_DISPATCH, "V_VT(arg) = %d\n", V_VT(pdp->rgvarg));
        hres = IDispatch_QueryInterface(V_DISPATCH(pdp->rgvarg), &IID_IDispatchEx, (void**)&eval_func);
        ok(hres == S_OK, "Could not get IDispatchEx iface: %08lx\n", hres);

        params.rgvarg = &arg;
        params.rgdispidNamedArgs = NULL;
        params.cArgs = 1;
        params.cNamedArgs = 0;
        V_VT(&arg) = VT_BSTR;

        V_BSTR(&arg) = SysAllocString(L"var x = 5; v");
        V_VT(&res) = VT_ERROR;
        hres = IDispatchEx_InvokeEx(eval_func, DISPID_VALUE, 0, DISPATCH_METHOD, &params, &res, NULL, NULL);
        ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
        ok(V_VT(&res) == VT_I4, "eval returned type %u\n", V_VT(&res));
        ok(V_I4(&res) == 2, "eval returned %ld\n", V_I4(&res));
        SysFreeString(V_BSTR(&arg));
        IDispatchEx_Release(eval_func);
        return S_OK;
    }
    case DISPID_GLOBAL_THROWINT: {
        VARIANT *v = pdp->rgvarg;
        HRESULT hres;

        ok(wFlags == INVOKE_FUNC, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(pdp->cArgs == 1, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pei != NULL, "pei == NULL\n");
        if(pvarRes) {
            ok(V_VT(pvarRes) == VT_EMPTY, "V_VT(pvarRes) = %d\n", V_VT(pvarRes));
            V_VT(pvarRes) = VT_BOOL;
            V_BOOL(pvarRes) = VARIANT_FALSE;
        }

        switch(V_VT(v)) {
        case VT_I4:
            hres = V_I4(v);
            break;
        case VT_R8:
            hres = (HRESULT)V_R8(v);
            break;
        default:
            ok(0, "unexpected vt %d\n", V_VT(v));
            return E_INVALIDARG;
        }
        return hres;
    }

    case DISPID_GLOBAL_THROWEI: {
        VARIANT *v = pdp->rgvarg + pdp->cArgs - 1;
        HRESULT hres;

        ok(wFlags == INVOKE_FUNC, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(pdp->cArgs == 1 || pdp->cArgs == 2, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pei != NULL, "pei == NULL\n");
        if(pvarRes) {
            ok(V_VT(pvarRes) == VT_EMPTY, "V_VT(pvarRes) = %d\n", V_VT(pvarRes));
            V_VT(pvarRes) = VT_BOOL;
            V_BOOL(pvarRes) = VARIANT_FALSE;
        }

        switch(V_VT(v)) {
        case VT_I4:
            hres = V_I4(v);
            break;
        case VT_R8:
            hres = (HRESULT)V_R8(v);
            break;
        default:
            ok(0, "unexpected vt %d\n", V_VT(v));
            return E_INVALIDARG;
        }

        pei->scode = hres;
        if(pdp->cArgs == 1) {
            pei->bstrSource = SysAllocString(L"test source");
            pei->bstrDescription = SysAllocString(L"test description");
        }else if(V_VT(pdp->rgvarg) == VT_BOOL && V_BOOL(pdp->rgvarg)) {
            pei->pfnDeferredFillIn = test_deferred_fill_in;
            pei->bstrSource = SysAllocString(L"source before defer");
            pei->bstrDescription = SysAllocString(L"desc before defer");
            pei->bstrHelpFile = SysAllocString(L"help before defer");
            pei->dwHelpContext = 1337;
        }
        return DISP_E_EXCEPTION;
    }
    }

    ok(0, "unexpected call %lx\n", id);
    return DISP_E_MEMBERNOTFOUND;
}

static HRESULT WINAPI Global_DeleteMemberByDispID(IDispatchEx *iface, DISPID id)
{
    switch(id) {
    case DISPID_GLOBAL_TESTPROPDELETE:
        CHECK_EXPECT(DeleteMemberByDispID);
        return S_OK;
    case DISPID_GLOBAL_TESTNOPROPDELETE:
        CHECK_EXPECT(DeleteMemberByDispID_false);
        return S_FALSE;
    case DISPID_GLOBAL_TESTPROPDELETEERROR:
        CHECK_EXPECT(DeleteMemberByDispID_error);
        return E_FAIL;
    default:
        ok(0, "id = %ld\n", id);
    }

    return E_FAIL;
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
    Global_DeleteMemberByDispID,
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
    *plcid = use_english ? MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT) : GetUserDefaultLCID();
    return S_OK;
}

static HRESULT WINAPI ActiveScriptSite_GetItemInfo(IActiveScriptSite *iface, LPCOLESTR pstrName,
        DWORD dwReturnMask, IUnknown **ppiunkItem, ITypeInfo **ppti)
{
    ok(dwReturnMask == SCRIPTINFO_IUNKNOWN, "unexpected dwReturnMask %lx\n", dwReturnMask);
    ok(!ppti, "ppti != NULL\n");

    if(!lstrcmpW(pstrName, L"testVal"))
        CHECK_EXPECT(GetItemInfo_testVal);
    else if(lstrcmpW(pstrName, L"test"))
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

static HRESULT WINAPI ActiveScriptSite_OnScriptError_CheckError(IActiveScriptSite *iface, IActiveScriptError *pscripterror)
{
    ok(pscripterror != NULL, "ActiveScriptSite_OnScriptError -- expected pscripterror to be set, got NULL\n");

    script_error = pscripterror;
    IActiveScriptError_AddRef(script_error);

    CHECK_EXPECT(ActiveScriptSite_OnScriptError);

    return S_OK;
}

static const IActiveScriptSiteVtbl ActiveScriptSite_CheckErrorVtbl = {
    ActiveScriptSite_QueryInterface,
    ActiveScriptSite_AddRef,
    ActiveScriptSite_Release,
    ActiveScriptSite_GetLCID,
    ActiveScriptSite_GetItemInfo,
    ActiveScriptSite_GetDocVersionString,
    ActiveScriptSite_OnScriptTerminate,
    ActiveScriptSite_OnStateChange,
    ActiveScriptSite_OnScriptError_CheckError,
    ActiveScriptSite_OnEnterScript,
    ActiveScriptSite_OnLeaveScript
};

static IActiveScriptSite ActiveScriptSite_CheckError = { &ActiveScriptSite_CheckErrorVtbl };

static HRESULT set_script_prop(IActiveScript *engine, DWORD property, VARIANT *val)
{
    IActiveScriptProperty *script_prop;
    HRESULT hres;

    hres = IActiveScript_QueryInterface(engine, &IID_IActiveScriptProperty,
            (void**)&script_prop);
    ok(hres == S_OK, "Could not get IActiveScriptProperty iface: %08lx\n", hres);

    hres = IActiveScriptProperty_SetProperty(script_prop, property, NULL, val);
    IActiveScriptProperty_Release(script_prop);

    return hres;
}

static IActiveScript *create_script(void)
{
    IActiveScript *script;
    VARIANT v;
    HRESULT hres;

    hres = CoCreateInstance(engine_clsid, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IActiveScript, (void**)&script);
    ok(hres == S_OK, "CoCreateInstance failed: %08lx\n", hres);

    V_VT(&v) = VT_I4;
    V_I4(&v) = invoke_version;
    hres = set_script_prop(script, SCRIPTPROP_INVOKEVERSIONING, &v);
    ok(hres == S_OK || broken(hres == E_NOTIMPL), "SetProperty(SCRIPTPROP_INVOKEVERSIONING) failed: %08lx\n", hres);
    if(invoke_version && FAILED(hres)) {
        IActiveScript_Release(script);
        return NULL;
    }

    return script;
}

static HRESULT parse_script(DWORD flags, const WCHAR *script_str)
{
    IActiveScriptParse *parser;
    IActiveScript *engine;
    HRESULT hres;

    engine = create_script();
    if(!engine)
        return S_OK;

    hres = IActiveScript_QueryInterface(engine, &IID_IActiveScriptParse, (void**)&parser);
    ok(hres == S_OK, "Could not get IActiveScriptParse: %08lx\n", hres);
    if (FAILED(hres))
    {
        IActiveScript_Release(engine);
        return hres;
    }

    hres = IActiveScriptParse_InitNew(parser);
    ok(hres == S_OK, "InitNew failed: %08lx\n", hres);

    hres = IActiveScript_SetScriptSite(engine, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08lx\n", hres);

    hres = IActiveScript_AddNamedItem(engine, L"test",
            SCRIPTITEM_ISVISIBLE|SCRIPTITEM_ISSOURCE|flags);
    ok(hres == S_OK, "AddNamedItem failed: %08lx\n", hres);

    hres = IActiveScript_SetScriptState(engine, SCRIPTSTATE_STARTED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_STARTED) failed: %08lx\n", hres);

    hres = IActiveScript_GetScriptDispatch(engine, NULL, &script_disp);
    ok(hres == S_OK, "GetScriptDispatch failed: %08lx\n", hres);
    ok(script_disp != NULL, "script_disp == NULL\n");
    ok(script_disp != (IDispatch*)&Global, "script_disp == Global\n");

    hres = IActiveScriptParse_ParseScriptText(parser, script_str, NULL, NULL, NULL, 0, 0, 0, NULL, NULL);

    IDispatch_Release(script_disp);
    IActiveScript_Release(engine);
    IActiveScriptParse_Release(parser);

    return hres;
}

static HRESULT invoke_procedure(const WCHAR *args, const WCHAR *source, DISPPARAMS *dp)
{
    IActiveScriptParseProcedure2 *parse_proc;
    IActiveScriptParse *parser;
    IActiveScript *engine;
    IDispatchEx *dispex;
    EXCEPINFO ei = {0};
    IDispatch *disp;
    VARIANT res;
    HRESULT hres;

    engine = create_script();
    if(!engine)
        return S_OK;

    hres = IActiveScript_QueryInterface(engine, &IID_IActiveScriptParse, (void**)&parser);
    ok(hres == S_OK, "Could not get IActiveScriptParse: %08lx\n", hres);

    hres = IActiveScriptParse_InitNew(parser);
    ok(hres == S_OK, "InitNew failed: %08lx\n", hres);

    hres = IActiveScript_SetScriptSite(engine, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08lx\n", hres);

    hres = IActiveScript_SetScriptState(engine, SCRIPTSTATE_STARTED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_STARTED) failed: %08lx\n", hres);

    hres = IActiveScript_QueryInterface(engine, &IID_IActiveScriptParseProcedure2, (void**)&parse_proc);
    ok(hres == S_OK, "Could not get IActiveScriptParse: %08lx\n", hres);

    hres = IActiveScriptParseProcedure2_ParseProcedureText(parse_proc, source, args, L"", NULL, NULL, NULL, 0, 0,
        SCRIPTPROC_HOSTMANAGESSOURCE|SCRIPTPROC_IMPLICIT_THIS|SCRIPTPROC_IMPLICIT_PARENTS, &disp);
    ok(hres == S_OK, "ParseProcedureText failed: %08lx\n", hres);

    hres = IDispatch_QueryInterface(disp, &IID_IDispatchEx, (void**)&dispex);
    ok(hres == S_OK, "Could not get IDispatchEx iface: %08lx\n", hres);
    IDispatch_Release(disp);

    V_VT(&res) = VT_EMPTY;
    hres = IDispatchEx_InvokeEx(dispex, DISPID_VALUE, 0, DISPATCH_METHOD, dp, &res, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    ok(V_VT(&res) == VT_BOOL && V_BOOL(&res), "InvokeEx returned vt %d (%lx)\n", V_VT(&res), V_I4(&res));
    IDispatchEx_Release(dispex);

    IActiveScriptParseProcedure2_Release(parse_proc);
    IActiveScript_Release(engine);
    IActiveScriptParse_Release(parser);

    return hres;
}

static HRESULT parse_htmlscript(const WCHAR *script_str)
{
    IActiveScriptParse *parser;
    IActiveScript *engine;
    HRESULT hres;

    engine = create_script();
    if(!engine)
        return E_FAIL;

    hres = IActiveScript_QueryInterface(engine, &IID_IActiveScriptParse, (void**)&parser);
    ok(hres == S_OK, "Could not get IActiveScriptParse: %08lx\n", hres);
    if (FAILED(hres))
    {
        IActiveScript_Release(engine);
        return E_FAIL;
    }

    hres = IActiveScriptParse_InitNew(parser);
    ok(hres == S_OK, "InitNew failed: %08lx\n", hres);

    hres = IActiveScript_SetScriptSite(engine, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08lx\n", hres);

    hres = IActiveScript_AddNamedItem(engine, L"test",
            SCRIPTITEM_ISVISIBLE|SCRIPTITEM_ISSOURCE|SCRIPTITEM_GLOBALMEMBERS);
    ok(hres == S_OK, "AddNamedItem failed: %08lx\n", hres);

    hres = IActiveScript_SetScriptState(engine, SCRIPTSTATE_STARTED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_STARTED) failed: %08lx\n", hres);

    hres = IActiveScriptParse_ParseScriptText(parser, script_str, NULL, NULL, L"</SCRIPT>", 0, 0, 0, NULL, NULL);

    IActiveScript_Release(engine);
    IActiveScriptParse_Release(parser);
    return hres;
}

#define ERROR_TODO_PARSE        0x0001
#define ERROR_TODO_SCODE        0x0002
#define ERROR_TODO_DESCRIPTION  0x0004
#define ERROR_TODO_HELPFILE     0x0008

static void test_error_reports(void)
{
    IActiveScriptParse *parser;
    IActiveScript *engine;
    unsigned i;
    HRESULT hres;

    static const struct {
        const WCHAR *script;
        HRESULT error;
        unsigned line;
        unsigned character;
        const WCHAR *error_source;
        const WCHAR *description;
        const WCHAR *help_file;
        DWORD help_context;
        const WCHAR *line_text;
        BOOL todo_flags;
        BOOL reserved_lcid;
    } tests[] = {
        {
            L"?",
            JS_E_SYNTAX, 0, 0,
            L"Microsoft JScript compilation error",
            L"Syntax error",
            NULL, 0,
            L"?"
        },
        {
            L"var a=1;\nif(a\n-->0) a=5;\n",
            JS_E_MISSING_RBRACKET, 2, 0,
            L"Microsoft JScript compilation error",
            L"Expected ')'",
            NULL, 0,
            L"-->0) a=5;",
            ERROR_TODO_PARSE
        },
        {
            L"new 3;",
            JS_E_INVALID_ACTION, 0, 0,
            L"Microsoft JScript runtime error",
            L"Object doesn't support this action"
        },
        {
            L"new null;",
            JS_E_OBJECT_EXPECTED, 0, 0,
            L"Microsoft JScript runtime error",
            L"Object expected"
        },
        {
            L"var a;\nnew null;",
            JS_E_OBJECT_EXPECTED, 1, 0,
            L"Microsoft JScript runtime error",
            L"Object expected"
        },
        {
            L"var a; new null;",
            JS_E_OBJECT_EXPECTED, 0, 7,
            L"Microsoft JScript runtime error",
            L"Object expected"
        },
        {
            L"var a;\na=\n  new null;",
            JS_E_OBJECT_EXPECTED, 1, 0,
            L"Microsoft JScript runtime error",
            L"Object expected"
        },
        {
            L"var a;\nif(na=\n  new null) {}",
            JS_E_OBJECT_EXPECTED, 1, 0,
            L"Microsoft JScript runtime error",
            L"Object expected"
        },
        {
            L"not_existing_variable.something();",
            JS_E_UNDEFINED_VARIABLE, 0, 0,
            L"Microsoft JScript runtime error",
            L"'not_existing_variable' is undefined"
        },
        {
            L" throw 1;",
            JS_E_EXCEPTION_THROWN, 0, 1,
            L"Microsoft JScript runtime error",
            L"Exception thrown and not caught"
        },
        {
            L"var f = function() { throw 1; };\n"
            L"f();\n",
            JS_E_EXCEPTION_THROWN, 0, 21,
            L"Microsoft JScript runtime error",
            L"Exception thrown and not caught"
        },
        {
            L"var f = function() { throw 1; };\n"
            L"try { f(); } finally { 2; }\n",
            JS_E_EXCEPTION_THROWN, 1, 21,
            L"Microsoft JScript runtime error",
            L"Exception thrown and not caught"
        },
        {
            L" throwInt(-2146827270);",
            JS_E_MISPLACED_RETURN, 0, 1,
            L"Microsoft JScript runtime error",
            L"'return' statement outside of function"
        },
        {
            L" throwEI(-2146827270);",
            JS_E_MISPLACED_RETURN, 0, 1,
            L"test source",
            L"test description"
        },
        {
            L" throwEI(-2146827270, false);",
            JS_E_MISPLACED_RETURN, 0, 1,
            L"Microsoft JScript runtime error",
            L"'return' statement outside of function"
        },
        {
            L" throwEI(-2147467259 /* E_FAIL */, false);",
            E_FAIL, 0, 1
        },
        {
            L" throwInt(-2147467259 /* E_FAIL */);",
            E_FAIL, 0, 1,
            NULL,
            NULL,
            NULL, 0,
            NULL,
            FALSE,
            0x409
        },
        {
            L" throwEI(-2147467259 /* E_FAIL */);",
            E_FAIL, 0, 1,
            L"test source",
            L"test description"
        },
        {
            L" throwEI(-2147467259 /* E_FAIL */, true);",
            E_FAIL, 0, 1,
            L"source after defer",
            L"desc after defer",
            L"help after defer", 1234567890,
            NULL,
            ERROR_TODO_HELPFILE
        },
        {
            L"switch(2) {\n"
            L"    case 1: break;\n"
            L"    case 0: break;\n"
            L"    case new null: break;\n"
            L"    default: throw 1;\n"
            L"}\n",
            JS_E_OBJECT_EXPECTED, 3, 4,
            L"Microsoft JScript runtime error",
            L"Object expected"
        },
        {
            L"do {\n"
            L"    1;\n"
            L"} while ( new null );\n",
            JS_E_OBJECT_EXPECTED, 2, 2,
            L"Microsoft JScript runtime error",
            L"Object expected"
        },
        {
            L"for (var i = 0; i < 100; new null) { i++ }",
            JS_E_OBJECT_EXPECTED, 0, 25,
            L"Microsoft JScript runtime error",
            L"Object expected"
        },
        {
            L"for (var i = 0; new null; i++) { i++ }",
            JS_E_OBJECT_EXPECTED, 0, 16,
            L"Microsoft JScript runtime error",
            L"Object expected"
        },
        {
            L"for (new null; i < 100; i++) { i++ }",
            JS_E_OBJECT_EXPECTED, 0, 5,
            L"Microsoft JScript runtime error",
            L"Object expected"
        },
        {
            L"var e = new Error();\n"
            L"e.number = -2146828279;\n"
            L"e.description = 'test';\n"
            L"throw e;",
            JS_E_SUBSCRIPT_OUT_OF_RANGE, 3, 0,
            L"Microsoft JScript runtime error",
            L"test",
            NULL, 0,
            NULL,
            FALSE,
            TRUE
        },
        {
            L"var e = new Error();\n"
            L"e.number = -2146828279;\n"
            L"e.message = 'test';\n"
            L"throw e;",
            JS_E_SUBSCRIPT_OUT_OF_RANGE, 3, 0,
            L"Microsoft JScript runtime error",
            L"",
            NULL, 0,
            NULL,
            FALSE,
            TRUE
        },
        {
            L"var e = new Error();\n"
            L"throw e;",
            E_FAIL, 1, 0,
            NULL,
            L"",
            NULL, 0,
            NULL,
            FALSE,
            TRUE
        },
        {
            L"var e = new Object();\n"
            L"e.number = -2146828279;\n"
            L"e.description = 'test';\n"
            L"throw e;",
            JS_E_EXCEPTION_THROWN, 3, 0,
            L"Microsoft JScript runtime error",
            L"Exception thrown and not caught",
            NULL, 0,
            NULL,
            ERROR_TODO_SCODE | ERROR_TODO_DESCRIPTION
        },
        {
            L"f(1\n,\n2,\n ,,3\n);\n",
            JS_E_SYNTAX, 3, 1,
            L"Microsoft JScript compilation error",
            L"Syntax error",
            NULL, 0,
            L" ,,3"
        },
    };

    if (!is_lang_english())
        skip("Non-english UI (test with hardcoded strings)\n");

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        engine = create_script();

        hres = IActiveScript_QueryInterface(engine, &IID_IActiveScriptParse, (void**)&parser);
        ok(hres == S_OK, "Could not get IActiveScriptParse: %08lx\n", hres);

        hres = IActiveScriptParse_InitNew(parser);
        ok(hres == S_OK, "InitNew failed: %08lx\n", hres);

        hres = IActiveScript_SetScriptSite(engine, &ActiveScriptSite_CheckError);
        ok(hres == S_OK, "SetScriptSite failed: %08lx\n", hres);

        hres = IActiveScript_AddNamedItem(engine, L"test",
                SCRIPTITEM_ISVISIBLE|SCRIPTITEM_ISSOURCE|SCRIPTITEM_GLOBALMEMBERS);
        ok(hres == S_OK, "AddNamedItem failed: %08lx\n", hres);

        hres = IActiveScript_SetScriptState(engine, SCRIPTSTATE_STARTED);
        ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_STARTED) failed: %08lx\n", hres);

        hres = IActiveScript_GetScriptDispatch(engine, NULL, &script_disp);
        ok(hres == S_OK, "GetScriptDispatch failed: %08lx\n", hres);
        ok(script_disp != NULL, "script_disp == NULL\n");
        ok(script_disp != (IDispatch*)&Global, "script_disp == Global\n");

        script_error = NULL;
        SET_EXPECT(ActiveScriptSite_OnScriptError);
        hres = IActiveScriptParse_ParseScriptText(parser, tests[i].script, NULL, NULL, NULL, 10, 0, 0, NULL, NULL);
        todo_wine_if(tests[i].todo_flags & ERROR_TODO_PARSE)
        ok(hres == SCRIPT_E_REPORTED || (tests[i].error == JS_E_EXCEPTION_THROWN && hres == SCRIPT_E_PROPAGATE),
           "[%u] got: 0x%08lx for %s\n", i, hres, wine_dbgstr_w(tests[i].script));
        todo_wine_if(tests[i].todo_flags & ERROR_TODO_PARSE)
        CHECK_CALLED(ActiveScriptSite_OnScriptError);

        if (script_error)
        {
            DWORD source_context;
            ULONG line_number;
            LONG character;
            BSTR line_text;
            EXCEPINFO ei;

            hres = IActiveScriptError_GetSourcePosition(script_error, NULL, NULL, NULL);
            ok(hres == S_OK, "GetSourcePosition failed %08lx\n", hres);

            source_context = 0xdeadbeef;
            hres = IActiveScriptError_GetSourcePosition(script_error, &source_context, NULL, NULL);
            ok(hres == S_OK, "GetSourcePosition failed0x%08lx\n", hres);
            ok(source_context == 10, "source_context = %lx\n", source_context);

            line_number = 0xdeadbeef;
            hres = IActiveScriptError_GetSourcePosition(script_error, NULL, &line_number, NULL);
            ok(hres == S_OK, "GetSourcePosition failed%08lx\n", hres);
            ok(line_number == tests[i].line, "[%u] line = %lu expected %u\n", i, line_number, tests[i].line);

            character = 0xdeadbeef;
            hres = IActiveScriptError_GetSourcePosition(script_error, NULL, NULL, &character);
            ok(hres == S_OK, "GetSourcePosition failed: %08lx\n", hres);
            ok(character == tests[i].character, "[%u] character = %lu expected %u\n", i, character, tests[i].character);

            hres = IActiveScriptError_GetSourceLineText(script_error, NULL);
            ok(hres == E_POINTER, "GetSourceLineText returned %08lx\n", hres);

            line_text = (BSTR)0xdeadbeef;
            hres = IActiveScriptError_GetSourceLineText(script_error, &line_text);
            if (tests[i].line_text)
            {
                ok(hres == S_OK, "GetSourceLineText failed: %08lx\n", hres);
                ok(line_text != NULL && !lstrcmpW(line_text, tests[i].line_text), "[%u] GetSourceLineText returned %s expected %s\n",
                   i, wine_dbgstr_w(line_text), wine_dbgstr_w(tests[i].line_text));
            }
            else
            {
                ok(hres == E_FAIL, "GetSourceLineText failed: %08lx\n", hres);
            }
            if (SUCCEEDED(hres))
                SysFreeString(line_text);

            hres = IActiveScriptError_GetExceptionInfo(script_error, NULL);
            ok(hres == E_POINTER, "GetExceptionInfo failed: %08lx\n", hres);

            ei.wCode = 0xdead;
            ei.wReserved = 0xdead;
            ei.bstrSource = (BSTR)0xdeadbeef;
            ei.bstrDescription = (BSTR)0xdeadbeef;
            ei.bstrHelpFile = (BSTR)0xdeadbeef;
            ei.dwHelpContext = 0xdeadbeef;
            ei.pvReserved = (void *)0xdeadbeef;
            ei.pfnDeferredFillIn = (void *)0xdeadbeef;
            ei.scode = 0xdeadbeef;

            hres = IActiveScriptError_GetExceptionInfo(script_error, &ei);
            ok(hres == S_OK, "GetExceptionInfo failed: %08lx\n", hres);

            todo_wine_if(tests[i].todo_flags & ERROR_TODO_SCODE)
            ok(ei.scode == tests[i].error, "[%u] scode = %08lx, expected %08lx\n", i, ei.scode, tests[i].error);
            ok(ei.wCode == 0, "wCode = %x\n", ei.wCode);
            todo_wine_if(tests[i].reserved_lcid)
            ok(ei.wReserved == (tests[i].reserved_lcid ? GetUserDefaultLCID() : 0), "[%u] wReserved = %x expected %lx\n",
               i, ei.wReserved, (tests[i].reserved_lcid ? GetUserDefaultLCID() : 0));
            if (is_lang_english())
            {
                if(tests[i].error_source)
                    ok(ei.bstrSource && !lstrcmpW(ei.bstrSource, tests[i].error_source), "[%u] bstrSource = %s expected %s\n",
                       i, wine_dbgstr_w(ei.bstrSource), wine_dbgstr_w(tests[i].error_source));
                else
                    ok(!ei.bstrSource, "[%u] bstrSource = %s expected NULL\n", i, wine_dbgstr_w(ei.bstrSource));
                if(tests[i].description)
                    todo_wine_if(tests[i].todo_flags & ERROR_TODO_DESCRIPTION)
                    ok(ei.bstrDescription && !lstrcmpW(ei.bstrDescription, tests[i].description),
                       "[%u] bstrDescription = %s expected %s\n", i, wine_dbgstr_w(ei.bstrDescription), wine_dbgstr_w(tests[i].description));
                else
                    ok(!ei.bstrDescription, "[%u] bstrDescription = %s expected NULL\n", i, wine_dbgstr_w(ei.bstrDescription));
            }
            if(tests[i].help_file)
                todo_wine_if(tests[i].todo_flags & ERROR_TODO_HELPFILE)
                ok(ei.bstrHelpFile && !lstrcmpW(ei.bstrHelpFile, tests[i].help_file),
                   "[%u] bstrHelpFile = %s expected %s\n", i, wine_dbgstr_w(ei.bstrHelpFile), wine_dbgstr_w(tests[i].help_file));
            else
                ok(!ei.bstrHelpFile, "[%u] bstrHelpFile = %s expected NULL\n", i, wine_dbgstr_w(ei.bstrHelpFile));
            todo_wine_if(tests[i].todo_flags & ERROR_TODO_HELPFILE)
            ok(ei.dwHelpContext == tests[i].help_context, "dwHelpContext = %lu, expected %lu\n", ei.dwHelpContext, tests[i].help_context);
            ok(!ei.pvReserved, "pvReserved = %p\n", ei.pvReserved);
            ok(!ei.pfnDeferredFillIn, "pfnDeferredFillIn = %p\n", ei.pfnDeferredFillIn);

            SysFreeString(ei.bstrSource);
            SysFreeString(ei.bstrDescription);
            SysFreeString(ei.bstrHelpFile);

            IActiveScriptError_Release(script_error);
        }

        IDispatch_Release(script_disp);
        IActiveScript_Release(engine);
        IActiveScriptParse_Release(parser);
    }
}

#define run_script(a) _run_script(__LINE__,a)
static void _run_script(unsigned line, const WCHAR *src)
{
    HRESULT hres;

    hres = parse_script(SCRIPTITEM_GLOBALMEMBERS, src);
    ok_(__FILE__,line)(hres == S_OK, "script %s failed: %08lx\n", wine_dbgstr_w(src), hres);
}

static BSTR get_script_from_file(const char *filename)
{
    DWORD size, len;
    HANDLE file, map;
    const char *file_map;
    BSTR ret;

    file = CreateFileA(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
    if(file == INVALID_HANDLE_VALUE) {
        trace("Could not open file: %lu\n", GetLastError());
        return NULL;
    }

    size = GetFileSize(file, NULL);

    map = CreateFileMappingW(file, NULL, PAGE_READONLY, 0, 0, NULL);
    CloseHandle(file);
    if(map == INVALID_HANDLE_VALUE) {
        trace("Could not create file mapping: %lu\n", GetLastError());
        return NULL;
    }

    file_map = MapViewOfFile(map, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(map);
    if(!file_map) {
        trace("MapViewOfFile failed: %lu\n", GetLastError());
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
    ok(hres == S_OK, "parse_script failed: %08lx\n", hres);
}

static BSTR load_res(const char *name)
{
    const char *data;
    DWORD size, len;
    BSTR str;
    HRSRC src;

    strict_dispid_check = FALSE;
    test_name = name;

    src = FindResourceA(NULL, name, (LPCSTR)40);
    ok(src != NULL, "Could not find resource %s\n", name);

    size = SizeofResource(NULL, src);
    data = LoadResource(NULL, src);

    len = MultiByteToWideChar(CP_ACP, 0, data, size, NULL, 0);
    str = SysAllocStringLen(NULL, len);
    MultiByteToWideChar(CP_ACP, 0, data, size, str, len);

    return str;
}

static void run_from_res(const char *name)
{
    BSTR str;
    HRESULT hres;

    str = load_res(name);

    SET_EXPECT(global_success_d);
    SET_EXPECT(global_success_i);
    hres = parse_script(SCRIPTITEM_GLOBALMEMBERS, str);
    CHECK_CALLED(global_success_d);
    CHECK_CALLED(global_success_i);

    ok(hres == S_OK, "parse_script failed: %08lx\n", hres);
    SysFreeString(str);
}

static void test_isvisible(BOOL global_members)
{
    IActiveScriptParse *parser;
    IActiveScript *engine;
    HRESULT hres;

    engine = create_script();
    if(!engine)
        return;

    hres = IActiveScript_QueryInterface(engine, &IID_IActiveScriptParse, (void**)&parser);
    ok(hres == S_OK, "Could not get IActiveScriptParse: %08lx\n", hres);
    if (FAILED(hres))
    {
        IActiveScript_Release(engine);
        return;
    }

    hres = IActiveScriptParse_InitNew(parser);
    ok(hres == S_OK, "InitNew failed: %08lx\n", hres);

    hres = IActiveScript_SetScriptSite(engine, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08lx\n", hres);

    if(global_members)
        SET_EXPECT(GetItemInfo_testVal);
    hres = IActiveScript_AddNamedItem(engine, L"testVal",
            SCRIPTITEM_ISVISIBLE|SCRIPTITEM_ISSOURCE|
            (global_members ? SCRIPTITEM_GLOBALMEMBERS : 0));
    ok(hres == S_OK, "AddNamedItem failed: %08lx\n", hres);
    if(global_members)
        CHECK_CALLED(GetItemInfo_testVal);

    hres = IActiveScript_SetScriptState(engine, SCRIPTSTATE_STARTED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_STARTED) failed: %08lx\n", hres);

    if(!global_members)
        SET_EXPECT(GetItemInfo_testVal);
    hres = IActiveScriptParse_ParseScriptText(parser, L"var v = testVal;", NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    if(!global_members)
        CHECK_CALLED(GetItemInfo_testVal);

    hres = IActiveScriptParse_ParseScriptText(parser, L"var v = testVal;", NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);

    IActiveScript_Release(engine);
    IActiveScriptParse_Release(parser);
}

static void test_start(void)
{
    IActiveScriptParse *parser;
    IActiveScript *engine;
    BSTR str;
    HRESULT hres;

    script_engine = engine = create_script();
    if(!engine)
        return;

    hres = IActiveScript_QueryInterface(engine, &IID_IActiveScriptParse, (void**)&parser);
    ok(hres == S_OK, "Could not get IActiveScriptParse: %08lx\n", hres);

    hres = IActiveScriptParse_InitNew(parser);
    ok(hres == S_OK, "InitNew failed: %08lx\n", hres);

    hres = IActiveScript_SetScriptSite(engine, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08lx\n", hres);

    hres = IActiveScript_AddNamedItem(engine, L"test", SCRIPTITEM_ISVISIBLE|SCRIPTITEM_ISSOURCE|SCRIPTITEM_GLOBALMEMBERS);
    ok(hres == S_OK, "AddNamedItem failed: %08lx\n", hres);

    str = SysAllocString(L"ok(getScriptState() === 5, \"getScriptState = \" + getScriptState());\n"
                         L"reportSuccess();");
    hres = IActiveScriptParse_ParseScriptText(parser, str, NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    SysFreeString(str);

    SET_EXPECT(global_success_d);
    SET_EXPECT(global_success_i);
    hres = IActiveScript_SetScriptState(engine, SCRIPTSTATE_STARTED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_STARTED) failed: %08lx\n", hres);
    CHECK_CALLED(global_success_d);
    CHECK_CALLED(global_success_i);

    IActiveScript_Release(engine);
    IActiveScriptParse_Release(parser);
    script_engine = NULL;
}

static void test_automagic(void)
{
    IActiveScriptParse *parser;
    IActiveScript *engine;
    BSTR str;
    HRESULT hres;

    script_engine = engine = create_script();
    if(!engine)
        return;

    hres = IActiveScript_QueryInterface(engine, &IID_IActiveScriptParse, (void**)&parser);
    ok(hres == S_OK, "Could not get IActiveScriptParse: %08lx\n", hres);

    hres = IActiveScriptParse_InitNew(parser);
    ok(hres == S_OK, "InitNew failed: %08lx\n", hres);

    hres = IActiveScript_SetScriptSite(engine, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08lx\n", hres);

    hres = IActiveScript_AddNamedItem(engine, L"test", SCRIPTITEM_ISVISIBLE|SCRIPTITEM_ISSOURCE|SCRIPTITEM_GLOBALMEMBERS);
    ok(hres == S_OK, "AddNamedItem failed: %08lx\n", hres);

    str = SysAllocString(L"function bindEventHandler::eventName() {}\n"
                         L"reportSuccess();");
    hres = IActiveScriptParse_ParseScriptText(parser, str, NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    SysFreeString(str);

    SET_EXPECT(BindHandler);
    SET_EXPECT(global_success_d);
    SET_EXPECT(global_success_i);
    hres = IActiveScript_SetScriptState(engine, SCRIPTSTATE_STARTED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_STARTED) failed: %08lx\n", hres);
    CHECK_CALLED(BindHandler);
    CHECK_CALLED(global_success_d);
    CHECK_CALLED(global_success_i);

    IActiveScript_Release(engine);
    IActiveScriptParse_Release(parser);
    script_engine = NULL;
}

static HRESULT parse_script_expr(const WCHAR *expr, VARIANT *res, IActiveScript **engine_ret)
{
    IActiveScriptParse *parser;
    IActiveScript *engine;
    HRESULT hres;

    engine = create_script();
    if(!engine)
        return E_FAIL;

    hres = IActiveScript_QueryInterface(engine, &IID_IActiveScriptParse, (void**)&parser);
    ok(hres == S_OK, "Could not get IActiveScriptParse: %08lx\n", hres);

    hres = IActiveScriptParse_InitNew(parser);
    ok(hres == S_OK, "InitNew failed: %08lx\n", hres);

    hres = IActiveScript_SetScriptSite(engine, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08lx\n", hres);

    SET_EXPECT(GetItemInfo_testVal);
    hres = IActiveScript_AddNamedItem(engine, L"testVal",
            SCRIPTITEM_ISVISIBLE|SCRIPTITEM_ISSOURCE|SCRIPTITEM_GLOBALMEMBERS);
    ok(hres == S_OK, "AddNamedItem failed: %08lx\n", hres);
    CHECK_CALLED(GetItemInfo_testVal);

    hres = IActiveScript_SetScriptState(engine, SCRIPTSTATE_STARTED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_STARTED) failed: %08lx\n", hres);

    hres = IActiveScriptParse_ParseScriptText(parser, expr, NULL, NULL, NULL, 0, 0, SCRIPTTEXT_ISEXPRESSION, res, NULL);
    IActiveScriptParse_Release(parser);

    if(engine_ret)
        *engine_ret = engine;
    else
        close_script(engine);

    return hres;
}

static void test_retval(void)
{
    IActiveScriptParse *parser;
    IActiveScript *engine;
    SCRIPTSTATE state;
    VARIANT res;
    HRESULT hres;
    BSTR str;

    engine = create_script();
    if(!engine)
        return;

    hres = IActiveScript_QueryInterface(engine, &IID_IActiveScriptParse, (void**)&parser);
    ok(hres == S_OK, "Could not get IActiveScriptParse: %08lx\n", hres);

    hres = IActiveScriptParse_InitNew(parser);
    ok(hres == S_OK, "InitNew failed: %08lx\n", hres);

    hres = IActiveScript_SetScriptSite(engine, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08lx\n", hres);

    SET_EXPECT(GetItemInfo_testVal);
    hres = IActiveScript_AddNamedItem(engine, L"testVal",
            SCRIPTITEM_ISVISIBLE|SCRIPTITEM_ISSOURCE|SCRIPTITEM_GLOBALMEMBERS);
    ok(hres == S_OK, "AddNamedItem failed: %08lx\n", hres);
    CHECK_CALLED(GetItemInfo_testVal);

    str = SysAllocString(L"reportSuccess(), true");
    V_VT(&res) = VT_NULL;
    SET_EXPECT(global_success_d);
    SET_EXPECT(global_success_i);
    hres = IActiveScriptParse_ParseScriptText(parser, str, NULL, NULL, NULL, 0, 0, 0, &res, NULL);
    CHECK_CALLED(global_success_d);
    CHECK_CALLED(global_success_i);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    ok(V_VT(&res) == VT_EMPTY, "V_VT(&res) = %d\n", V_VT(&res));
    SysFreeString(str);

    hres = IActiveScript_GetScriptState(engine, &state);
    ok(hres == S_OK, "GetScriptState failed: %08lx\n", hres);
    ok(state == SCRIPTSTATE_INITIALIZED, "state = %d\n", state);

    hres = IActiveScript_SetScriptState(engine, SCRIPTSTATE_STARTED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_STARTED) failed: %08lx\n", hres);

    IActiveScriptParse_Release(parser);

    close_script(engine);
}

static void test_propputref(void)
{
    static DISPID propput_dispid = DISPID_PROPERTYPUT;
    DISPPARAMS dp = {0}, dp_get = {0};
    IActiveScript *script, *script2;
    IDispatch *disp, *obj;
    HRESULT hres;
    VARIANT v;
    DISPID id;
    BSTR str;

    hres = parse_script_expr(L"new Object()", &v, &script2);
    ok(hres == S_OK, "parse_script_expr failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_DISPATCH, "V_VT(v) = %d\n", V_VT(&v));
    obj = V_DISPATCH(&v);

    hres = parse_script_expr(L"var disp = new Object(); disp.a = disp; disp", &v, &script);
    ok(hres == S_OK, "parse_script_expr failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_DISPATCH, "V_VT(v) = %d\n", V_VT(&v));
    disp = V_DISPATCH(&v);

    str = SysAllocString(L"a");
    hres = IDispatch_GetIDsOfNames(disp, &IID_NULL, &str, 1, 0, &id);
    ok(hres == S_OK, "GetIDsOfNames failed: %08lx\n", hres);
    SysFreeString(str);

    dp.cArgs = dp.cNamedArgs = 1;
    dp.rgdispidNamedArgs = &propput_dispid;
    dp.rgvarg = &v;
    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = obj;
    hres = IDispatch_Invoke(disp, id, &IID_NULL, 0, DISPATCH_PROPERTYPUTREF, &dp, NULL, NULL, NULL);
    ok(hres == S_OK, "Invoke failed: %08lx\n", hres);
    V_VT(&v) = VT_EMPTY;
    hres = IDispatch_Invoke(disp, id, &IID_NULL, 0, DISPATCH_PROPERTYGET, &dp_get, &v, NULL, NULL);
    ok(hres == S_OK, "Invoke failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_DISPATCH, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_DISPATCH(&v) == obj, "V_DISPATCH(v) = %p\n", V_DISPATCH(&v));
    VariantClear(&v);

    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = obj;
    hres = IDispatch_Invoke(disp, id, &IID_NULL, 0, DISPATCH_PROPERTYPUTREF | DISPATCH_PROPERTYPUT, &dp, NULL, NULL, NULL);
    ok(hres == S_OK, "Invoke failed: %08lx\n", hres);
    V_VT(&v) = VT_EMPTY;
    hres = IDispatch_Invoke(disp, id, &IID_NULL, 0, DISPATCH_PROPERTYGET, &dp_get, &v, NULL, NULL);
    ok(hres == S_OK, "Invoke failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_DISPATCH, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_DISPATCH(&v) == obj, "V_DISPATCH(v) = %p\n", V_DISPATCH(&v));
    IDispatch_Release(obj);
    close_script(script2);
    VariantClear(&v);

    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = NULL;
    hres = IDispatch_Invoke(disp, id, &IID_NULL, 0, DISPATCH_PROPERTYPUTREF, &dp, NULL, NULL, NULL);
    ok(hres == S_OK, "Invoke failed: %08lx\n", hres);
    V_VT(&v) = VT_EMPTY;
    hres = IDispatch_Invoke(disp, id, &IID_NULL, 0, DISPATCH_PROPERTYGET, &dp_get, &v, NULL, NULL);
    ok(hres == S_OK, "Invoke failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_DISPATCH, "V_VT(v) = %d\n", V_VT(&v));
    ok(!V_DISPATCH(&v), "V_DISPATCH(v) = %p\n", V_DISPATCH(&v));

    V_VT(&v) = VT_EMPTY;
    hres = IDispatch_Invoke(disp, id, &IID_NULL, 0, DISPATCH_PROPERTYPUTREF, &dp, NULL, NULL, NULL);
    ok(hres == S_OK, "Invoke failed: %08lx\n", hres);
    V_VT(&v) = VT_EMPTY;
    hres = IDispatch_Invoke(disp, id, &IID_NULL, 0, DISPATCH_PROPERTYGET, &dp_get, &v, NULL, NULL);
    ok(hres == S_OK, "Invoke failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_EMPTY, "V_VT(v) = %d\n", V_VT(&v));

    V_VT(&v) = VT_I4;
    V_I4(&v) = 42;
    hres = IDispatch_Invoke(disp, id, &IID_NULL, 0, DISPATCH_PROPERTYPUTREF, &dp, NULL, NULL, NULL);
    ok(hres == S_OK, "Invoke failed: %08lx\n", hres);
    V_VT(&v) = VT_EMPTY;
    hres = IDispatch_Invoke(disp, id, &IID_NULL, 0, DISPATCH_PROPERTYGET, &dp_get, &v, NULL, NULL);
    ok(hres == S_OK, "Invoke failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_I4, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_I4(&v) == 42, "V_I4(v) = %ld\n", V_I4(&v));

    IDispatch_Release(disp);
    close_script(script);
}

static void test_default_value(void)
{
    static DISPID propput_dispid = DISPID_PROPERTYPUT;
    IActiveScript *script;
    DISPPARAMS dp = {0};
    IDispatch *disp;
    VARIANT v;
    HRESULT hres;

    hres = parse_script_expr(L"new Date()", &v, NULL);
    ok(hres == S_OK, "parse_script_expr failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_DISPATCH, "V_VT(v) = %d\n", V_VT(&v));
    disp = V_DISPATCH(&v);

    V_VT(&v) = VT_EMPTY;
    hres = IDispatch_Invoke(disp, DISPID_VALUE, &IID_NULL, 0, DISPATCH_PROPERTYGET, &dp, &v, NULL, NULL);
    ok(hres == E_UNEXPECTED, "Invoke failed: %08lx\n", hres);
    IDispatch_Release(disp);

    hres = parse_script_expr(L"new Date()", &v, &script);
    ok(hres == S_OK, "parse_script_expr failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_DISPATCH, "V_VT(v) = %d\n", V_VT(&v));
    disp = V_DISPATCH(&v);

    V_VT(&v) = VT_EMPTY;
    hres = IDispatch_Invoke(disp, DISPID_VALUE, &IID_NULL, 0, DISPATCH_PROPERTYGET, &dp, &v, NULL, NULL);
    ok(hres == S_OK, "Invoke failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    VariantClear(&v);
    IDispatch_Release(disp);
    close_script(script);

    hres = parse_script_expr(L"var arr = [5]; arr.toString = function() {return \"foo\";}; arr.valueOf = function() {return 42;}; arr", &v, &script);
    ok(hres == S_OK, "parse_script_expr failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_DISPATCH, "V_VT(v) = %d\n", V_VT(&v));
    disp = V_DISPATCH(&v);

    V_VT(&v) = VT_EMPTY;
    hres = IDispatch_Invoke(disp, DISPID_VALUE, &IID_NULL, 0, DISPATCH_PROPERTYGET, &dp, &v, NULL, NULL);
    ok(hres == S_OK, "Invoke failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_I4, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_I4(&v) == 42, "V_I4(v) = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    IDispatch_Release(disp);
    close_script(script);

    hres = parse_script_expr(L"var arr = [5]; arr.toString = function() {return \"foo\";}; arr", &v, &script);
    ok(hres == S_OK, "parse_script_expr failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_DISPATCH, "V_VT(v) = %d\n", V_VT(&v));
    disp = V_DISPATCH(&v);

    V_VT(&v) = VT_EMPTY;
    hres = IDispatch_Invoke(disp, DISPID_VALUE, &IID_NULL, 0, DISPATCH_PROPERTYGET, &dp, &v, NULL, NULL);
    ok(hres == S_OK, "Invoke failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    ok(!lstrcmpW(V_BSTR(&v), L"foo"), "V_BSTR(v) = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    IDispatch_Release(disp);
    close_script(script);

    hres = parse_script_expr(L"var arr = [5]; arr", &v, &script);
    ok(hres == S_OK, "parse_script_expr failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_DISPATCH, "V_VT(v) = %d\n", V_VT(&v));
    disp = V_DISPATCH(&v);

    V_VT(&v) = VT_EMPTY;
    hres = IDispatch_Invoke(disp, DISPID_VALUE, &IID_NULL, 0, DISPATCH_PROPERTYGET, &dp, &v, NULL, NULL);
    ok(hres == S_OK, "Invoke failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    ok(!lstrcmpW(V_BSTR(&v), L"5"), "V_BSTR(v) = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    IDispatch_Release(disp);
    close_script(script);

    hres = parse_script_expr(L"var obj = Object.prototype; delete obj.valueOf; obj", &v, &script);
    ok(hres == S_OK, "parse_script_expr failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_DISPATCH, "V_VT(v) = %d\n", V_VT(&v));
    disp = V_DISPATCH(&v);

    V_VT(&v) = VT_EMPTY;
    hres = IDispatch_Invoke(disp, DISPID_VALUE, &IID_NULL, 0, DISPATCH_PROPERTYGET, &dp, &v, NULL, NULL);
    ok(hres == S_OK, "Invoke failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    ok(!lstrcmpW(V_BSTR(&v), L"[object Object]"), "V_BSTR(v) = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    IDispatch_Release(disp);
    close_script(script);

    hres = parse_script_expr(L"var obj = Object.prototype; delete obj.toString; obj", &v, &script);
    ok(hres == S_OK, "parse_script_expr failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_DISPATCH, "V_VT(v) = %d\n", V_VT(&v));
    disp = V_DISPATCH(&v);

    V_VT(&v) = VT_EMPTY;
    hres = IDispatch_Invoke(disp, DISPID_VALUE, &IID_NULL, 0, DISPATCH_PROPERTYGET, &dp, &v, NULL, NULL);
    ok(hres == DISP_E_MEMBERNOTFOUND, "Invoke failed: %08lx\n", hres);
    IDispatch_Release(disp);
    close_script(script);

    hres = parse_script_expr(L"Object.prototype", &v, &script);
    ok(hres == S_OK, "parse_script_expr failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_DISPATCH, "V_VT(v) = %d\n", V_VT(&v));
    disp = V_DISPATCH(&v);

    dp.cArgs = dp.cNamedArgs = 1;
    dp.rgdispidNamedArgs = &propput_dispid;
    dp.rgvarg = &v;
    V_VT(&v) = VT_EMPTY;
    hres = IDispatch_Invoke(disp, DISPID_VALUE, &IID_NULL, 0, DISPATCH_PROPERTYPUT, &dp, NULL, NULL, NULL);
    ok(hres == DISP_E_MEMBERNOTFOUND, "Invoke failed: %08lx\n", hres);
    hres = IDispatch_Invoke(disp, DISPID_VALUE, &IID_NULL, 0, DISPATCH_PROPERTYPUTREF, &dp, NULL, NULL, NULL);
    ok(hres == DISP_E_MEMBERNOTFOUND, "Invoke failed: %08lx\n", hres);
    hres = IDispatch_Invoke(disp, DISPID_VALUE, &IID_NULL, 0, DISPATCH_PROPERTYPUTREF | DISPATCH_PROPERTYPUT, &dp, NULL, NULL, NULL);
    ok(hres == DISP_E_MEMBERNOTFOUND, "Invoke failed: %08lx\n", hres);
    IDispatch_Release(disp);
    close_script(script);

    hres = parse_script_expr(L"var f = function() {return 42;}; f", &v, &script);
    ok(hres == S_OK, "parse_script_expr failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_DISPATCH, "V_VT(v) = %d\n", V_VT(&v));
    disp = V_DISPATCH(&v);

    V_VT(&v) = VT_EMPTY;
    hres = IDispatch_Invoke(disp, DISPID_VALUE, &IID_NULL, 0, DISPATCH_PROPERTYPUT, &dp, NULL, NULL, NULL);
    ok(hres == DISP_E_MEMBERNOTFOUND, "Invoke failed: %08lx\n", hres);
    hres = IDispatch_Invoke(disp, DISPID_VALUE, &IID_NULL, 0, DISPATCH_PROPERTYPUTREF, &dp, NULL, NULL, NULL);
    ok(hres == DISP_E_MEMBERNOTFOUND, "Invoke failed: %08lx\n", hres);
    hres = IDispatch_Invoke(disp, DISPID_VALUE, &IID_NULL, 0, DISPATCH_PROPERTYPUTREF | DISPATCH_PROPERTYPUT, &dp, NULL, NULL, NULL);
    ok(hres == DISP_E_MEMBERNOTFOUND, "Invoke failed: %08lx\n", hres);
    IDispatch_Release(disp);
    close_script(script);
}

static void test_number_localization(void)
{
    static struct {
        const WCHAR *num;
        const WCHAR *expect;
    } tests[] = {
        { L"0",                 L"0.00" },
        { L"+1234.5",           L"1,234.50" },
        { L"-1337.7331",        L"-1,337.73" },
        { L"-0.0123",           L"-0.01" },
        { L"-0.0198",           L"-0.02" },
        { L"0.004",             L"0.00" },
        { L"65536.5",           L"65,536.50" },
        { L"NaN",               L"NaN" }
    };
    static const WCHAR fmt[] = L"Number.prototype.toLocaleString.call(%s)";
    WCHAR script_buf[ARRAY_SIZE(fmt) + 32];
    HRESULT hres;
    unsigned i;
    VARIANT v;

    use_english = TRUE;
    for(i = 0; i < ARRAY_SIZE(tests); i++) {
        swprintf(script_buf, ARRAY_SIZE(script_buf), fmt, tests[i].num);
        hres = parse_script_expr(script_buf, &v, NULL);
        ok(hres == S_OK, "[%u] parse_script_expr failed: %08lx\n", i, hres);
        ok(V_VT(&v) == VT_BSTR, "[%u] V_VT(v) = %d\n", i, V_VT(&v));
        ok(!lstrcmpW(V_BSTR(&v), tests[i].expect), "[%u] got %s\n", i, wine_dbgstr_w(V_BSTR(&v)));
        VariantClear(&v);
    }
    use_english = FALSE;
}

static void test_script_exprs(void)
{
    WCHAR buf[64], sep[4];
    VARIANT v;
    HRESULT hres;

    testing_expr = TRUE;

    hres = parse_script_expr(L"true", &v, NULL);
    ok(hres == S_OK, "parse_script_expr failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_BOOL, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_BOOL(&v) == VARIANT_TRUE, "V_BOOL(v) = %x\n", V_BOOL(&v));

    hres = parse_script_expr(L"false, true", &v, NULL);
    ok(hres == S_OK, "parse_script_expr failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_BOOL, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_BOOL(&v) == VARIANT_TRUE, "V_BOOL(v) = %x\n", V_BOOL(&v));

    SET_EXPECT(global_success_d);
    SET_EXPECT(global_success_i);
    hres = parse_script_expr(L"reportSuccess(); true", &v, NULL);
    ok(hres == S_OK, "parse_script_expr failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_BOOL, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_BOOL(&v) == VARIANT_TRUE, "V_BOOL(v) = %x\n", V_BOOL(&v));
    CHECK_CALLED(global_success_d);
    CHECK_CALLED(global_success_i);

    hres = parse_script_expr(L"if(false) true", &v, NULL);
    ok(hres == S_OK, "parse_script_expr failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_EMPTY, "V_VT(v) = %d\n", V_VT(&v));

    hres = parse_script_expr(L"return testPropGet", &v, NULL);
    ok(hres == 0x800a03fa, "parse_script_expr failed: %08lx\n", hres);

    hres = parse_script_expr(L"reportSuccess(); return true", &v, NULL);
    ok(hres == 0x800a03fa, "parse_script_expr failed: %08lx\n", hres);

    SET_EXPECT(global_success_d);
    SET_EXPECT(global_success_i);
    hres = parse_script_expr(L"reportSuccess(); true", NULL, NULL);
    ok(hres == S_OK, "parse_script_expr failed: %08lx\n", hres);
    CHECK_CALLED(global_success_d);
    CHECK_CALLED(global_success_i);

    hres = parse_script_expr(L"var o=new Object(); Object.prototype.toLocaleString.call(o)", &v, NULL);
    ok(hres == S_OK, "parse_script_expr failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    ok(!lstrcmpW(V_BSTR(&v), L"[object Object]"), "V_BSTR(v) = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    hres = parse_script_expr(L"var o=new Object(); Object.prototype.toString = function() {return \"wine\";}; Object.prototype.toLocaleString.call(o)", &v, NULL);
    ok(hres == S_OK, "parse_script_expr failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    ok(!lstrcmpW(V_BSTR(&v), L"wine"), "V_BSTR(v) = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    hres = parse_script_expr(L"var o=new Object(); delete Object.prototype.toString; Object.prototype.toLocaleString.call(o)", &v, NULL);
    ok(hres == 0x800a01b6, "parse_script_expr failed: %08lx\n", hres);

    hres = parse_script_expr(L"var o=new Object(); o.toString = function() {return \"wine\";}; Object.prototype.toLocaleString.call(o)", &v, NULL);
    ok(hres == S_OK, "parse_script_expr failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    ok(!lstrcmpW(V_BSTR(&v), L"wine"), "V_BSTR(v) = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    if(!GetLocaleInfoW(GetUserDefaultLCID(), LOCALE_SLIST, sep, ARRAY_SIZE(sep))) wcscpy(sep, L",");
    swprintf(buf, ARRAY_SIZE(buf), L"12%s 12%s undefined undefined undefined%s 12", sep, sep, sep);
    hres = parse_script_expr(L"var arr = [5]; arr.toLocaleString = function(a,b,c) {return a+' '+b+' '+c;};"
                             L"Number.prototype.toLocaleString = function() {return 12;};"
                             L"[1,2,arr,3].toLocaleString('foo','bar','baz')", &v, NULL);
    ok(hres == S_OK, "parse_script_expr failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    ok(!lstrcmpW(V_BSTR(&v), buf), "V_BSTR(v) = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    hres = parse_script_expr(L"delete Object.prototype.toLocaleString; Array.prototype.toLocaleString.call([])", &v, NULL);
    ok(hres == S_OK, "parse_script_expr failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    ok(!lstrcmpW(V_BSTR(&v), L""), "V_BSTR(v) = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    hres = parse_script_expr(L"delete Object.prototype.toLocaleString; Array.prototype.toLocaleString.call(['a'])", &v, NULL);
    ok(hres == 0x800a01b6, "parse_script_expr failed: %08lx\n", hres);

    test_number_localization();
    test_default_value();
    test_propputref();
    test_retval();

    testing_expr = FALSE;
}

static void test_invokeex(void)
{
    static DISPID propput_dispid = DISPID_PROPERTYPUT;
    DISPPARAMS dp = {NULL}, dp_max = {NULL};
    DISPID func_id, max_id, prop_id;
    IActiveScript *script;
    IDispatchEx *dispex;
    VARIANT v, arg;
    BSTR str;
    HRESULT hres;

    hres = parse_script_expr(L"var o = {func: function() {return 3;}, max: Math.max, prop: 6}; o", &v, &script);
    ok(hres == S_OK, "parse_script_expr failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_DISPATCH, "V_VT(v) = %d\n", V_VT(&v));

    hres = IDispatch_QueryInterface(V_DISPATCH(&v), &IID_IDispatchEx, (void**)&dispex);
    ok(hres == S_OK, "Could not get IDispatchEx iface: %08lx\n", hres);
    VariantClear(&v);

    str = SysAllocString(L"func");
    hres = IDispatchEx_GetDispID(dispex, str, 0, &func_id);
    SysFreeString(str);
    ok(hres == S_OK, "GetDispID failed: %08lx\n", hres);

    str = SysAllocString(L"max");
    hres = IDispatchEx_GetDispID(dispex, str, 0, &max_id);
    SysFreeString(str);
    ok(hres == S_OK, "GetDispID failed: %08lx\n", hres);

    str = SysAllocString(L"prop");
    hres = IDispatchEx_GetDispID(dispex, str, 0, &prop_id);
    SysFreeString(str);
    ok(hres == S_OK, "GetDispID failed: %08lx\n", hres);

    dp_max.rgvarg = &arg;
    dp_max.cArgs = 1;
    V_VT(&arg) = VT_I4;
    V_I4(&arg) = 42;

    hres = IDispatchEx_InvokeEx(dispex, func_id, 0, DISPATCH_METHOD, &dp, &v, NULL, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_I4, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_I4(&v) == 3, "V_I4(v) = %ld\n", V_I4(&v));

    hres = IDispatchEx_InvokeEx(dispex, max_id, 0, DISPATCH_METHOD, &dp_max, &v, NULL, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_I4, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_I4(&v) == 42, "V_I4(v) = %ld\n", V_I4(&v));

    hres = IDispatchEx_InvokeEx(dispex, prop_id, 0, DISPATCH_PROPERTYGET, &dp, &v, NULL, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_I4, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_I4(&v) == 6, "V_I4(v) = %ld\n", V_I4(&v));

    hres = IActiveScript_SetScriptState(script, SCRIPTSTATE_UNINITIALIZED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_STARTED) failed: %08lx\n", hres);

    str = SysAllocString(L"func");
    hres = IDispatchEx_GetDispID(dispex, str, 0, &func_id);
    SysFreeString(str);
    ok(hres == S_OK, "GetDispID failed: %08lx\n", hres);

    V_VT(&v) = VT_EMPTY;
    hres = IDispatchEx_InvokeEx(dispex, func_id, 0, DISPATCH_METHOD, &dp, &v, NULL, NULL);
    ok(hres == E_UNEXPECTED || broken(hres == 0x800a1393), "InvokeEx failed: %08lx\n", hres);

    V_VT(&v) = VT_EMPTY;
    hres = IDispatchEx_InvokeEx(dispex, max_id, 0, DISPATCH_METHOD, &dp_max, &v, NULL, NULL);
    ok(hres == E_UNEXPECTED || broken(hres == 0x800a1393), "InvokeEx failed: %08lx\n", hres);

    V_VT(&v) = VT_EMPTY;
    hres = IDispatchEx_InvokeEx(dispex, prop_id, 0, DISPATCH_PROPERTYGET, &dp, &v, NULL, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_I4, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_I4(&v) == 6, "V_I4(v) = %ld\n", V_I4(&v));

    IActiveScript_Close(script);

    V_VT(&v) = VT_EMPTY;
    hres = IDispatchEx_InvokeEx(dispex, prop_id, 0, DISPATCH_PROPERTYGET, &dp, &v, NULL, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_I4, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_I4(&v) == 6, "V_I4(v) = %ld\n", V_I4(&v));

    IDispatchEx_Release(dispex);
    IActiveScript_Release(script);

    hres = parse_script_expr(L"Math.max", &v, &script);
    ok(hres == S_OK, "parse_script_expr failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_DISPATCH, "V_VT(v) = %d\n", V_VT(&v));

    hres = IDispatch_QueryInterface(V_DISPATCH(&v), &IID_IDispatchEx, (void**)&dispex);
    ok(hres == S_OK, "Could not get IDispatchEx iface: %08lx\n", hres);
    VariantClear(&v);

    str = SysAllocString(L"call");
    hres = IDispatchEx_GetDispID(dispex, str, 0, &func_id);
    SysFreeString(str);
    ok(hres == S_OK, "GetDispID failed: %08lx\n", hres);

    hres = IActiveScript_SetScriptState(script, SCRIPTSTATE_UNINITIALIZED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_STARTED) failed: %08lx\n", hres);

    str = SysAllocString(L"call");
    hres = IDispatchEx_GetDispID(dispex, str, 0, &func_id);
    SysFreeString(str);
    ok(hres == S_OK, "GetDispID failed: %08lx\n", hres);

    str = SysAllocString(L"length");
    hres = IDispatchEx_GetDispID(dispex, str, 0, &prop_id);
    SysFreeString(str);
    ok(hres == S_OK, "GetDispID failed: %08lx\n", hres);

    hres = IDispatchEx_InvokeEx(dispex, func_id, 0, DISPATCH_PROPERTYGET, &dp, &v, NULL, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_DISPATCH, "V_VT(v) = %d\n", V_VT(&v));
    VariantClear(&v);

    hres = IDispatchEx_InvokeEx(dispex, prop_id, 0, DISPATCH_PROPERTYGET, &dp, &v, NULL, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_I4, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_I4(&v) == 2, "V_I4(v) = %ld\n", V_I4(&v));

    IDispatchEx_Release(dispex);
    IActiveScript_Release(script);

    hres = parse_script_expr(L"Math.max", &v, &script);
    ok(hres == S_OK, "parse_script_expr failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_DISPATCH, "V_VT(v) = %d\n", V_VT(&v));

    hres = IDispatch_QueryInterface(V_DISPATCH(&v), &IID_IDispatchEx, (void**)&dispex);
    ok(hres == S_OK, "Could not get IDispatchEx iface: %08lx\n", hres);
    VariantClear(&v);

    hres = IActiveScript_SetScriptState(script, SCRIPTSTATE_UNINITIALIZED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_STARTED) failed: %08lx\n", hres);

    str = SysAllocString(L"call");
    hres = IDispatchEx_GetDispID(dispex, str, 0, &func_id);
    SysFreeString(str);
    ok(hres == E_UNEXPECTED, "GetDispID failed: %08lx\n", hres);

    IDispatchEx_Release(dispex);
    IActiveScript_Release(script);

    /* test InvokeEx following prototype chain of builtin object (PROP_PROTREF) */
    hres = parse_script_expr(L"o = new Array(); o.push(\"foo\"); o", &v, &script);
    ok(hres == S_OK, "parse_script_expr failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_DISPATCH, "V_VT(v) = %d\n", V_VT(&v));

    hres = IDispatch_QueryInterface(V_DISPATCH(&v), &IID_IDispatchEx, (void**)&dispex);
    ok(hres == S_OK, "Could not get IDispatchEx iface: %08lx\n", hres);
    VariantClear(&v);

    str = SysAllocString(L"push");
    hres = IDispatchEx_GetDispID(dispex, str, 0, &func_id);
    SysFreeString(str);
    ok(hres == S_OK, "GetDispID failed: %08lx\n", hres);

    dp.rgvarg = &arg;
    dp.cArgs = 1;
    V_VT(&arg) = VT_BSTR;
    V_BSTR(&arg) = SysAllocString(L"bar");

    hres = IDispatchEx_InvokeEx(dispex, func_id, 0, DISPATCH_METHOD, &dp, &v, NULL, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    SysFreeString(V_BSTR(&arg));

    str = SysAllocString(L"join");
    hres = IDispatchEx_GetDispID(dispex, str, 0, &func_id);
    SysFreeString(str);
    ok(hres == S_OK, "GetDispID failed: %08lx\n", hres);

    V_BSTR(&arg) = SysAllocString(L";");
    hres = IDispatchEx_InvokeEx(dispex, func_id, 0, DISPATCH_METHOD, &dp, &v, NULL, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    SysFreeString(V_BSTR(&arg));
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    ok(!lstrcmpW(V_BSTR(&v), L"foo;bar"), "V_BSTR(v) = %s\n", wine_dbgstr_w(V_BSTR(&v)));

    VariantClear(&v);

    dp.rgvarg = NULL;
    dp.cArgs = 0;

    IDispatchEx_Release(dispex);
    IActiveScript_Release(script);

    /* test InvokeEx following prototype chain of JScript objects (PROP_JSVAL) */
    hres = parse_script_expr(L"function c() { this.func = function() { return this.prop1 * this.prop2 };"
                             L"this.prop1 = 6; this.prop2 = 9; }; var o = new c(); o.prop2 = 7; o",
                             &v, &script);
    ok(hres == S_OK, "parse_script_expr failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_DISPATCH, "V_VT(v) = %d\n", V_VT(&v));

    hres = IDispatch_QueryInterface(V_DISPATCH(&v), &IID_IDispatchEx, (void**)&dispex);
    ok(hres == S_OK, "Could not get IDispatchEx iface: %08lx\n", hres);
    VariantClear(&v);

    str = SysAllocString(L"prop1");
    hres = IDispatchEx_GetDispID(dispex, str, 0, &prop_id);
    SysFreeString(str);
    ok(hres == S_OK, "GetDispID failed: %08lx\n", hres);

    hres = IDispatchEx_InvokeEx(dispex, prop_id, 0, DISPATCH_PROPERTYGET, &dp, &v, NULL, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_I4, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_I4(&v) == 6, "V_I4(v) = %ld\n", V_I4(&v));

    str = SysAllocString(L"prop2");
    hres = IDispatchEx_GetDispID(dispex, str, 0, &prop_id);
    SysFreeString(str);
    ok(hres == S_OK, "GetDispID failed: %08lx\n", hres);

    hres = IDispatchEx_InvokeEx(dispex, prop_id, 0, DISPATCH_PROPERTYGET, &dp, &v, NULL, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_I4, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_I4(&v) == 7, "V_I4(v) = %ld\n", V_I4(&v));

    str = SysAllocString(L"func");
    hres = IDispatchEx_GetDispID(dispex, str, 0, &func_id);
    SysFreeString(str);
    ok(hres == S_OK, "GetDispID failed: %08lx\n", hres);

    hres = IDispatchEx_InvokeEx(dispex, func_id, 0, DISPATCH_METHOD, &dp, &v, NULL, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_I4, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_I4(&v) == 42, "V_I4(v) = %s\n", wine_dbgstr_variant(&v));

    IDispatchEx_Release(dispex);
    IActiveScript_Release(script);

    /* test InvokeEx with host prop and custom caller */
    hres = parse_script_expr(L"var o = {}; o", &v, &script);
    ok(hres == S_OK, "parse_script_expr failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_DISPATCH, "V_VT(v) = %d\n", V_VT(&v));

    hres = IDispatch_QueryInterface(V_DISPATCH(&v), &IID_IDispatchEx, (void**)&dispex);
    ok(hres == S_OK, "Could not get IDispatchEx iface: %08lx\n", hres);
    VariantClear(&v);

    str = SysAllocString(L"caller");
    hres = IDispatchEx_GetDispID(dispex, str, fdexNameEnsure, &func_id);
    SysFreeString(str);
    ok(hres == S_OK, "GetDispID failed: %08lx\n", hres);

    SET_EXPECT(test_caller_get);
    dp.cArgs = dp.cNamedArgs = 1;
    dp.rgvarg = &arg;
    dp.rgdispidNamedArgs = &propput_dispid;
    V_VT(&arg) = VT_DISPATCH;
    V_DISPATCH(&arg) = (IDispatch*)&testcallerobj;
    hres = IDispatchEx_InvokeEx(dispex, func_id, 0, DISPATCH_PROPERTYPUT, &dp, NULL, NULL, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    todo_wine
    CHECK_CALLED(test_caller_get);

    SET_EXPECT(test_caller_null);
    dp.cArgs = dp.cNamedArgs = 0;
    dp.rgvarg = NULL;
    dp.rgdispidNamedArgs = NULL;
    hres = IDispatchEx_InvokeEx(dispex, func_id, 0, DISPATCH_METHOD, &dp, &v, NULL, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_I4, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_I4(&v) == 42, "V_I4(v) = %s\n", wine_dbgstr_variant(&v));
    CHECK_CALLED(test_caller_null);
    V_VT(&v) = VT_EMPTY;

    SET_EXPECT(test_caller_obj);
    hres = IDispatchEx_InvokeEx(dispex, func_id, 0, DISPATCH_METHOD, &dp, &v, NULL, &sp_obj);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_I4, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_I4(&v) == 137, "V_I4(v) = %s\n", wine_dbgstr_variant(&v));
    CHECK_CALLED(test_caller_obj);

    IDispatchEx_Release(dispex);
    IActiveScript_Release(script);
}

static void test_members(void)
{
    DISPID func_id, prop_id;
    IActiveScript *script;
    IDispatchEx *dispex;
    DWORD propflags;
    HRESULT hres;
    VARIANT v;
    BSTR str;

    hres = parse_script_expr(L"var o = { func: function() {}, prop: 1 }; o", &v, &script);
    ok(hres == S_OK, "parse_script_expr failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_DISPATCH, "V_VT(v) = %d\n", V_VT(&v));

    hres = IDispatch_QueryInterface(V_DISPATCH(&v), &IID_IDispatchEx, (void**)&dispex);
    ok(hres == S_OK, "Could not get IDispatchEx iface: %08lx\n", hres);
    VariantClear(&v);

    str = SysAllocString(L"func");
    hres = IDispatchEx_GetDispID(dispex, str, 0, &func_id);
    SysFreeString(str);
    ok(hres == S_OK, "GetDispID failed: %08lx\n", hres);

    str = SysAllocString(L"prop");
    hres = IDispatchEx_GetDispID(dispex, str, 0, &prop_id);
    SysFreeString(str);
    ok(hres == S_OK, "GetDispID failed: %08lx\n", hres);

    hres = IDispatchEx_GetMemberName(dispex, func_id, &str);
    ok(hres == S_OK, "GetMemberName failed: %08lx\n", hres);
    ok(!wcscmp(str, L"func"), "GetMemberName returned %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    hres = IDispatchEx_GetMemberName(dispex, prop_id, &str);
    ok(hres == S_OK, "GetMemberName failed: %08lx\n", hres);
    ok(!wcscmp(str, L"prop"), "GetMemberName returned %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    propflags = 0xdeadbeef;
    hres = IDispatchEx_GetMemberProperties(dispex, func_id, 0, &propflags);
    ok(hres == S_OK, "GetMemberProperties failed: %08lx\n", hres);
    ok(propflags == 0, "propflags = %08lx", propflags);

    propflags = 0xdeadbeef;
    hres = IDispatchEx_GetMemberProperties(dispex, prop_id, 0, &propflags);
    ok(hres == S_OK, "GetMemberProperties failed: %08lx\n", hres);
    ok(propflags == 0, "propflags = %08lx", propflags);

    hres = IDispatchEx_DeleteMemberByDispID(dispex, func_id);
    ok(hres == S_OK, "DeleteMemberByDispID failed: %08lx\n", hres);

    hres = IDispatchEx_GetMemberName(dispex, func_id, &str);
    ok(hres == DISP_E_MEMBERNOTFOUND, "GetMemberName failed: %08lx\n", hres);
    hres = IDispatchEx_GetMemberProperties(dispex, func_id, 0, &propflags);
    ok(hres == DISP_E_MEMBERNOTFOUND, "GetMemberProperties failed: %08lx\n", hres);

    hres = IDispatchEx_GetMemberName(dispex, prop_id, &str);
    ok(hres == S_OK, "GetMemberName failed: %08lx\n", hres);
    ok(!wcscmp(str, L"prop"), "GetMemberName returned %s\n", wine_dbgstr_w(str));
    SysFreeString(str);
    propflags = 0xdeadbeef;
    hres = IDispatchEx_GetMemberProperties(dispex, prop_id, 0, &propflags);
    ok(hres == S_OK, "GetMemberProperties failed: %08lx\n", hres);
    ok(propflags == 0, "propflags = %08lx", propflags);

    str = SysAllocString(L"prop");
    hres = IDispatchEx_DeleteMemberByName(dispex, str, 0);
    ok(hres == S_OK, "DeleteMemberByName failed: %08lx\n", hres);
    SysFreeString(str);

    hres = IDispatchEx_GetMemberName(dispex, prop_id, &str);
    ok(hres == DISP_E_MEMBERNOTFOUND, "GetMemberName failed: %08lx\n", hres);
    hres = IDispatchEx_GetMemberProperties(dispex, prop_id, 0, &propflags);
    ok(hres == DISP_E_MEMBERNOTFOUND, "GetMemberProperties failed: %08lx\n", hres);

    IDispatchEx_Release(dispex);
    IActiveScript_Release(script);
}

static void test_destructors(void)
{
    static const WCHAR cyclic_refs[] = L"(function() {\n"
        "var a = function() {}, c = { 'a': a, 'ref': Math }, b = { 'a': a, 'c': c };\n"
        "Math.ref = { 'obj': testDestrObj, 'ref': Math, 'a': a, 'b': b };\n"
        "a.ref = { 'ref': Math, 'a': a }; b.ref = Math.ref;\n"
        "a.self = a; b.self = b; c.self = c;\n"
    "})(), true";
    static DISPID propput_dispid = DISPID_PROPERTYPUT;
    IActiveScript *script, *script2;
    IDispatchEx *dispex, *dispex2;
    IActiveScriptParse *parser;
    DISPPARAMS dp = { 0 };
    VARIANT v;
    DISPID id;
    BSTR str;
    HRESULT hres;

    V_VT(&v) = VT_EMPTY;
    hres = parse_script_expr(L"Math.ref = testDestrObj, isNaN.ref = testDestrObj, true", &v, &script);
    ok(hres == S_OK, "parse_script_expr failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_BOOL, "V_VT(v) = %d\n", V_VT(&v));

    SET_EXPECT(testdestrobj);
    hres = IActiveScript_SetScriptState(script, SCRIPTSTATE_UNINITIALIZED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_UNINITIALIZED) failed: %08lx\n", hres);
    CHECK_CALLED(testdestrobj);

    IActiveScript_Release(script);

    V_VT(&v) = VT_EMPTY;
    hres = parse_script_expr(cyclic_refs, &v, &script);
    ok(hres == S_OK, "parse_script_expr failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_BOOL, "V_VT(v) = %d\n", V_VT(&v));

    SET_EXPECT(testdestrobj);
    hres = IActiveScript_SetScriptState(script, SCRIPTSTATE_UNINITIALIZED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_UNINITIALIZED) failed: %08lx\n", hres);
    CHECK_CALLED(testdestrobj);

    IActiveScript_Release(script);

    V_VT(&v) = VT_EMPTY;
    hres = parse_script_expr(cyclic_refs, &v, &script);
    ok(hres == S_OK, "parse_script_expr failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_BOOL, "V_VT(v) = %d\n", V_VT(&v));

    hres = IActiveScript_QueryInterface(script, &IID_IActiveScriptParse, (void**)&parser);
    ok(hres == S_OK, "Could not get IActiveScriptParse: %08lx\n", hres);

    SET_EXPECT(testdestrobj);
    V_VT(&v) = VT_EMPTY;
    hres = IActiveScriptParse_ParseScriptText(parser, L"Math.ref = undefined, CollectGarbage(), true",
                                              NULL, NULL, NULL, 0, 0, SCRIPTTEXT_ISEXPRESSION, &v, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_BOOL, "V_VT(v) = %d\n", V_VT(&v));
    IActiveScriptParse_Release(parser);
    CHECK_CALLED(testdestrobj);

    IActiveScript_Release(script);

    /* Create a cyclic ref across two jscript engines */
    V_VT(&v) = VT_EMPTY;
    hres = parse_script_expr(cyclic_refs, &v, &script);
    ok(hres == S_OK, "parse_script_expr failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_BOOL, "V_VT(v) = %d\n", V_VT(&v));

    hres = IActiveScript_QueryInterface(script, &IID_IActiveScriptParse, (void**)&parser);
    ok(hres == S_OK, "Could not get IActiveScriptParse: %08lx\n", hres);

    V_VT(&v) = VT_EMPTY;
    hres = IActiveScriptParse_ParseScriptText(parser, L"Math.ref", NULL, NULL, NULL, 0, 0, SCRIPTTEXT_ISEXPRESSION, &v, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_DISPATCH, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_DISPATCH(&v) != NULL, "V_DISPATCH(v) = NULL\n");

    hres = IDispatch_QueryInterface(V_DISPATCH(&v), &IID_IDispatchEx, (void**)&dispex);
    ok(hres == S_OK, "Could not get IDispatchEx iface: %08lx\n", hres);
    VariantClear(&v);

    V_VT(&v) = VT_EMPTY;
    hres = parse_script_expr(L"new Object()", &v, &script2);
    ok(hres == S_OK, "parse_script_expr failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_DISPATCH, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_DISPATCH(&v) != NULL, "V_DISPATCH(v) = NULL\n");

    hres = IDispatch_QueryInterface(V_DISPATCH(&v), &IID_IDispatchEx, (void**)&dispex2);
    ok(hres == S_OK, "Could not get IDispatchEx iface: %08lx\n", hres);
    VariantClear(&v);

    dp.cArgs = dp.cNamedArgs = 1;
    dp.rgdispidNamedArgs = &propput_dispid;
    dp.rgvarg = &v;

    str = SysAllocString(L"diff_ctx");
    hres = IDispatchEx_GetDispID(dispex, str, fdexNameEnsure, &id);
    ok(hres == S_OK, "GetDispID failed: %08lx\n", hres);
    SysFreeString(str);

    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = (IDispatch*)dispex2;
    hres = IDispatchEx_Invoke(dispex, id, &IID_NULL, 0, DISPATCH_PROPERTYPUT, &dp, NULL, NULL, NULL);
    ok(hres == S_OK, "Invoke failed: %08lx\n", hres);

    str = SysAllocString(L"ref");
    hres = IDispatchEx_GetDispID(dispex2, str, fdexNameEnsure, &id);
    ok(hres == S_OK, "GetDispID failed: %08lx\n", hres);
    SysFreeString(str);

    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = (IDispatch*)dispex;
    hres = IDispatchEx_Invoke(dispex2, id, &IID_NULL, 0, DISPATCH_PROPERTYPUT, &dp, NULL, NULL, NULL);
    ok(hres == S_OK, "Invoke failed: %08lx\n", hres);

    IDispatchEx_Release(dispex2);
    IDispatchEx_Release(dispex);

    SET_EXPECT(testdestrobj);
    V_VT(&v) = VT_EMPTY;
    hres = IActiveScriptParse_ParseScriptText(parser, L"Math.ref = undefined, CollectGarbage(), true",
                                              NULL, NULL, NULL, 0, 0, SCRIPTTEXT_ISEXPRESSION, &v, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_BOOL, "V_VT(v) = %d\n", V_VT(&v));
    IActiveScriptParse_Release(parser);
    CHECK_CALLED(testdestrobj);

    IActiveScript_Release(script2);
    IActiveScript_Release(script);
}

static void test_eval(void)
{
    IActiveScriptParse *parser;
    IDispatchEx *script_dispex;
    IDispatch *script_disp;
    IActiveScript *engine;
    VARIANT arg, res;
    DISPPARAMS params;
    DISPID id, v_id;
    BSTR str;
    HRESULT hres;

    engine = create_script();

    hres = IActiveScript_QueryInterface(engine, &IID_IActiveScriptParse, (void**)&parser);
    ok(hres == S_OK, "Could not get IActiveScriptParse: %08lx\n", hres);

    hres = IActiveScriptParse_InitNew(parser);
    ok(hres == S_OK, "InitNew failed: %08lx\n", hres);

    hres = IActiveScript_SetScriptSite(engine, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08lx\n", hres);

    SET_EXPECT(GetItemInfo_testVal);
    hres = IActiveScript_AddNamedItem(engine, L"testVal",
            SCRIPTITEM_ISVISIBLE|SCRIPTITEM_ISSOURCE|SCRIPTITEM_GLOBALMEMBERS);
    ok(hres == S_OK, "AddNamedItem failed: %08lx\n", hres);
    CHECK_CALLED(GetItemInfo_testVal);

    hres = IActiveScript_SetScriptState(engine, SCRIPTSTATE_STARTED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_STARTED) failed: %08lx\n", hres);

    hres = IActiveScript_GetScriptDispatch(engine, NULL, &script_disp);
    ok(hres == S_OK, "GetScriptDispatch failed: %08lx\n", hres);
    ok(script_disp != NULL, "script_disp == NULL\n");

    hres = IDispatch_QueryInterface(script_disp, &IID_IDispatchEx, (void**)&script_dispex);
    ok(hres == S_OK, "Could not get IDispatchEx iface: %08lx\n", hres);
    IDispatch_Release(script_disp);

    str = SysAllocString(L"eval");
    hres = IDispatchEx_GetDispID(script_dispex, str, 0, &id);
    ok(hres == S_OK, "Could not get eval dispid: %08lx\n", hres);
    SysFreeString(str);

    params.rgvarg = &arg;
    params.rgdispidNamedArgs = NULL;
    params.cArgs = 1;
    params.cNamedArgs = 0;
    V_VT(&arg) = VT_BSTR;

    V_BSTR(&arg) = SysAllocString(L"var v = 1;");
    V_VT(&res) = VT_ERROR;
    hres = IDispatchEx_InvokeEx(script_dispex, id, 0, DISPATCH_METHOD, &params, &res, NULL, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    ok(V_VT(&res) == VT_EMPTY, "eval returned type %u\n", V_VT(&res));
    SysFreeString(V_BSTR(&arg));

    V_BSTR(&arg) = SysAllocString(L"v");
    V_VT(&res) = VT_ERROR;
    hres = IDispatchEx_InvokeEx(script_dispex, id, 0, DISPATCH_METHOD, &params, &res, NULL, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    ok(V_VT(&res) == VT_I4, "eval returned type %u\n", V_VT(&res));
    ok(V_I4(&res) == 1, "eval returned %ld\n", V_I4(&res));
    SysFreeString(V_BSTR(&arg));

    str = SysAllocString(L"v");
    hres = IDispatchEx_GetDispID(script_dispex, str, 0, &v_id);
    ok(hres == S_OK, "Could not get v dispid: %08lx\n", hres);
    SysFreeString(str);

    params.rgvarg = NULL;
    params.cArgs = 0;
    V_VT(&res) = VT_ERROR;
    hres = IDispatchEx_InvokeEx(script_dispex, v_id, 0, DISPATCH_PROPERTYGET, &params, &res, NULL, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    ok(V_VT(&res) == VT_I4, "eval returned type %u\n", V_VT(&res));
    ok(V_I4(&res) == 1, "eval returned %ld\n", V_I4(&res));

    SET_EXPECT(global_calleval_i);
    hres = IActiveScriptParse_ParseScriptText(parser,
                                              L"(function(){"
                                              L"    var v = 2;"
                                              L"    callEval(eval);"
                                              L"    ok(x === 5, 'x = ' + x);"
                                              L"})();",
                                              NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    CHECK_CALLED(global_calleval_i);

    str = SysAllocString(L"x");
    hres = IDispatchEx_GetDispID(script_dispex, str, 0, &id);
    ok(hres == DISP_E_UNKNOWNNAME, "GetDispID(x) returned %08lx\n", hres);
    SysFreeString(str);

    IDispatchEx_Release(script_dispex);
    IActiveScriptParse_Release(parser);
    close_script(engine);
}

struct bom_test
{
    WCHAR str[1024];
    HRESULT hres;
};

static void run_bom_tests(void)
{
    BSTR src;
    int i;
    HRESULT hres;
    struct bom_test bom_tests[] = {
        {L"var a = 1; reportSuccess();", S_OK},
        {L"\xfeffvar a = 1; reportSuccess();", S_OK},
        {L"v\xfeff" "ar a = 1; reportSuccess();", JS_E_OUT_OF_MEMORY},
        {L"var\xfeff a = 1; reportSuccess();", S_OK},
        {L"var a = 1; \xfeffreportSuccess();", S_OK},
        {L"var a = 1; report\xfeffSuccess();", JS_E_OUT_OF_MEMORY},
        {L"var a = 1; reportSuccess\xfeff();", S_OK},
        {L"var a = 1; reportSuccess(\xfeff);", S_OK},
        {L"var a =\xfeff 1; reportSuccess(\xfeff);", S_OK},
        {L"\xfeffvar a =\xfeff\xfeff 1; reportSuccess(\xfeff);", S_OK},
        {L""}
    };

    engine_clsid = &CLSID_JScript;

    for (i = 0; bom_tests[i].str[0]; i++)
    {
        if(bom_tests[i].hres == S_OK)
        {
             SET_EXPECT(global_success_d);
             SET_EXPECT(global_success_i);
             src = SysAllocString(bom_tests[i].str);
             hres = parse_script(SCRIPTITEM_GLOBALMEMBERS, src);
             ok(hres == S_OK, "test %s failed with %08lx\n", wine_dbgstr_w(src), hres);
             SysFreeString(src);
             CHECK_CALLED(global_success_d);
             CHECK_CALLED(global_success_i);
        }
        else
        {
             src = SysAllocString(bom_tests[i].str);
             hres = parse_script(SCRIPTITEM_GLOBALMEMBERS, src);
             todo_wine ok(hres == bom_tests[i].hres, "test %s returned with %08lx\n", wine_dbgstr_w(src), hres);
             SysFreeString(src);
        }
    }
}

static BOOL run_tests(void)
{
    HRESULT hres;

    if(invoke_version) {
        IActiveScript *script;

        script = create_script();
        if(!script) {
            win_skip("Could not create script\n");
            return FALSE;
        }
        IActiveScript_Release(script);
    }

    strict_dispid_check = TRUE;

    run_script(L"");
    run_script(L"/* empty */ ;");

    SET_EXPECT(global_propget_d);
    SET_EXPECT(global_propget_i);
    run_script(L"testPropGet;");
    CHECK_CALLED(global_propget_d);
    CHECK_CALLED(global_propget_i);

    SET_EXPECT(global_propput_d);
    SET_EXPECT(global_propput_i);
    run_script(L"testPropPut = 1;");
    CHECK_CALLED(global_propput_d);
    CHECK_CALLED(global_propput_i);

    SET_EXPECT(global_propputref_d);
    SET_EXPECT(global_propputref_i);
    run_script(L"testPropPutRef = new Object();");
    CHECK_CALLED(global_propputref_d);
    CHECK_CALLED(global_propputref_i);

    SET_EXPECT(global_propputref_d);
    SET_EXPECT(global_propputref_i);
    run_script(L"testPropPutRef = testObj;");
    CHECK_CALLED(global_propputref_d);
    CHECK_CALLED(global_propputref_i);

    SET_EXPECT(global_success_d);
    SET_EXPECT(global_success_i);
    run_script(L"reportSuccess();");
    CHECK_CALLED(global_success_d);
    CHECK_CALLED(global_success_i);

    SET_EXPECT(testobj_delete_test);
    run_script(L"ok((delete testObj.deleteTest) === true, 'delete testObj.deleteTest did not return true');");
    CHECK_CALLED(testobj_delete_test);

    SET_EXPECT(testobj_delete_nodelete);
    run_script(L"ok((delete testObj.noDeleteTest) === false, 'delete testObj.noDeleteTest did not return false');");
    CHECK_CALLED(testobj_delete_nodelete);

    SET_EXPECT(global_propdelete_d);
    SET_EXPECT(DeleteMemberByDispID);
    run_script(L"ok((delete testPropDelete) === true, 'delete testPropDelete did not return true');");
    CHECK_CALLED(global_propdelete_d);
    CHECK_CALLED(DeleteMemberByDispID);

    SET_EXPECT(global_nopropdelete_d);
    SET_EXPECT(DeleteMemberByDispID_false);
    run_script(L"ok((delete testNoPropDelete) === false, 'delete testPropDelete did not return false');");
    CHECK_CALLED(global_nopropdelete_d);
    CHECK_CALLED(DeleteMemberByDispID_false);

    SET_EXPECT(global_propdeleteerror_d);
    SET_EXPECT(DeleteMemberByDispID_error);
    hres = parse_script(SCRIPTITEM_GLOBALMEMBERS, L"delete testPropDeleteError;");
    ok(hres == E_FAIL, "unexpected result %08lx\n", hres);
    CHECK_CALLED(global_propdeleteerror_d);
    CHECK_CALLED(DeleteMemberByDispID_error);

    SET_EXPECT(puredisp_prop_d);
    run_script(L"ok((delete pureDisp.prop) === false, 'delete pureDisp.prop did not return true');");
    CHECK_CALLED(puredisp_prop_d);

    SET_EXPECT(puredisp_noprop_d);
    run_script(L"ok((delete pureDisp.noprop) === true, 'delete pureDisp.noprop did not return false');");
    CHECK_CALLED(puredisp_noprop_d);

    SET_EXPECT(puredisp_value);
    run_script(L"var t=pureDisp; t=t(false);");
    CHECK_CALLED(puredisp_value);

    SET_EXPECT(puredisp_value);
    run_script(L"var t = {func: pureDisp}; t = t.func(false);");
    CHECK_CALLED(puredisp_value);

    SET_EXPECT(dispexfunc_value);
    run_script(L"var t = dispexFunc; t = t(false);");
    CHECK_CALLED(dispexfunc_value);

    SET_EXPECT(dispexfunc_value);
    run_script(L"var t = {func: dispexFunc}; t = t.func(false);");
    CHECK_CALLED(dispexfunc_value);

    SET_EXPECT(dispexfunc_value);
    run_script(L"Function.prototype.apply.call(dispexFunc, testObj, [true]);");
    CHECK_CALLED(dispexfunc_value);

    SET_EXPECT(puredisp_value);
    run_script(L"Function.prototype.apply.call(pureDisp, testObj, [true]);");
    CHECK_CALLED(puredisp_value);

    run_script(L"(function reportSuccess() {})()");

    run_script(L"ok(typeof(test) === 'object', \"typeof(test) != 'object'\");");

    run_script(L"function reportSuccess() {}; reportSuccess();");

    SET_EXPECT(global_propget_d);
    run_script(L"var testPropGet");
    CHECK_CALLED(global_propget_d);

    SET_EXPECT(global_propget_d);
    run_script(L"eval('var testPropGet;');");
    CHECK_CALLED(global_propget_d);

    run_script(L"var testPropGet; function testPropGet() {}");

    SET_EXPECT(global_notexists_d);
    run_script(L"var notExists; notExists = 1;");
    CHECK_CALLED(global_notexists_d);

    SET_EXPECT(testobj_notexists_d);
    run_script(L"testObj.notExists;");
    CHECK_CALLED(testobj_notexists_d);

    run_script(L"function f() { var testPropGet; }");
    run_script(L"(function () { var testPropGet; })();");
    run_script(L"(function () { eval('var testPropGet;'); })();");

    SET_EXPECT(invoke_func);
    run_script(L"ok(propGetFunc() == 0, \"Incorrect propGetFunc value\");");
    CHECK_CALLED(invoke_func);
    run_script(L"ok(propGetFunc(1) == 1, \"Incorrect propGetFunc value\");");
    run_script(L"ok(propGetFunc(1, 2) == 2, \"Incorrect propGetFunc value\");");
    SET_EXPECT(invoke_func);
    run_script(L"ok(propGetFunc().toString() == 0, \"Incorrect propGetFunc value\");");
    CHECK_CALLED(invoke_func);
    run_script(L"ok(propGetFunc(1).toString() == 1, \"Incorrect propGetFunc value\");");
    SET_EXPECT(invoke_func);
    run_script(L"propGetFunc(1);");
    CHECK_CALLED(invoke_func);

    run_script(L"objectFlag(1).toString();");

    run_script(L"(function() { var tmp = (function () { return testObj; })()(1);})();");
    run_script(L"(function() { var tmp = (function () { return testObj; })()();})();");

    run_script(L"ok((testObj instanceof Object) === false, 'testObj is instance of Object');");

    SET_EXPECT(testobj_prop_d);
    run_script(L"ok(('prop' in testObj) === true, 'prop is not in testObj');");
    CHECK_CALLED(testobj_prop_d);

    SET_EXPECT(testobj_noprop_d);
    run_script(L"ok(('noprop' in testObj) === false, 'noprop is in testObj');");
    CHECK_CALLED(testobj_noprop_d);

    SET_EXPECT(testobj_prop_d);
    run_script(L"ok(Object.prototype.hasOwnProperty.call(testObj, 'prop') === true, 'hasOwnProperty(\\\"prop\\\") returned false');");
    CHECK_CALLED(testobj_prop_d);

    SET_EXPECT(testobj_noprop_d);
    run_script(L"ok(Object.prototype.hasOwnProperty.call(testObj, 'noprop') === false, 'hasOwnProperty(\\\"noprop\\\") returned true');");
    CHECK_CALLED(testobj_noprop_d);

    SET_EXPECT(puredisp_prop_d);
    run_script(L"ok(Object.prototype.hasOwnProperty.call(pureDisp, 'prop') === true, 'hasOwnProperty(\\\"noprop\\\") returned false');");
    CHECK_CALLED(puredisp_prop_d);

    SET_EXPECT(puredisp_noprop_d);
    run_script(L"ok(Object.prototype.hasOwnProperty.call(pureDisp, 'noprop') === false, 'hasOwnProperty(\\\"noprop\\\") returned true');");
    CHECK_CALLED(puredisp_noprop_d);

    SET_EXPECT(testobj_value);
    run_script(L"ok(String(testObj) === '1', 'wrong testObj value');");
    CHECK_CALLED(testobj_value);

    SET_EXPECT(testobj_value);
    run_script(L"ok(String.prototype.concat.call(testObj, ' OK') === '1 OK', 'wrong concat result');");
    CHECK_CALLED(testobj_value);

    SET_EXPECT(testobj_construct);
    run_script(L"var t = new testObj(1);");
    CHECK_CALLED(testobj_construct);

    SET_EXPECT(global_propget_d);
    SET_EXPECT(global_propget_i);
    run_script(L"this.testPropGet;");
    CHECK_CALLED(global_propget_d);
    CHECK_CALLED(global_propget_i);

    SET_EXPECT(global_propputref_d);
    SET_EXPECT(global_propputref_i);
    run_script(L"testPropPutRef = nullDisp;");
    CHECK_CALLED(global_propputref_d);
    CHECK_CALLED(global_propputref_i);

    SET_EXPECT(global_propget_d);
    SET_EXPECT(global_propget_i);
    run_script(L"(function () { this.testPropGet; })();");
    CHECK_CALLED(global_propget_d);
    CHECK_CALLED(global_propget_i);

    run_script(L"testThis(this);");
    run_script(L"(function () { testThis(this); })();");
    run_script(L"function x() { testThis(this); }; x();");
    run_script(L"var t = {func: function () { ok(this === t, 'this !== t'); }}; with(t) { func(); }");
    run_script(L"function x() { testThis(this); }; with({y: 1}) { x(); }");
    run_script(L"(function () { function x() { testThis(this);} x(); })();");

    SET_EXPECT(testobj_onlydispid_d);
    SET_EXPECT(testobj_onlydispid_i);
    run_script(L"ok(typeof(testObj.onlyDispID) === 'unknown', 'unexpected typeof(testObj.onlyDispID)');");
    CHECK_CALLED(testobj_onlydispid_d);
    CHECK_CALLED(testobj_onlydispid_i);

    SET_EXPECT(testobj_getidfail_d);
    hres = parse_script(SCRIPTITEM_GLOBALMEMBERS, L"testObj.notExists = testObj.getIDFail;");
    ok(hres == E_FAIL, "parse_script returned %08lx\n", hres);
    CHECK_CALLED(testobj_getidfail_d);

    SET_EXPECT(global_propget_d);
    SET_EXPECT(global_propget_i);
    SET_EXPECT(testobj_getidfail_d);
    hres = parse_script(SCRIPTITEM_GLOBALMEMBERS, L"testObj.getIDFail = testPropGet;");
    ok(hres == E_FAIL, "parse_script returned %08lx\n", hres);
    CHECK_CALLED(global_propget_d);
    CHECK_CALLED(global_propget_i);
    CHECK_CALLED(testobj_getidfail_d);

    SET_EXPECT(global_propargput_d);
    SET_EXPECT(global_propargput_i);
    run_script(L"var t=0; propArgPutG(t++, t++) = t++;");
    CHECK_CALLED(global_propargput_d);
    CHECK_CALLED(global_propargput_i);

    SET_EXPECT(global_propargput_d);
    SET_EXPECT(global_propargput_i);
    run_script(L"var t=0; test.propArgPutO(t++, t++) = t++;");
    CHECK_CALLED(global_propargput_d);
    CHECK_CALLED(global_propargput_i);

    SET_EXPECT(global_propargputop_d);
    SET_EXPECT(global_propargputop_get_i);
    SET_EXPECT(global_propargputop_put_i);
    run_script(L"var t=0; propArgPutOp(t++, t++) += t++;");
    CHECK_CALLED(global_propargputop_d);
    CHECK_CALLED(global_propargputop_get_i);
    CHECK_CALLED(global_propargputop_put_i);

    SET_EXPECT(global_propargputop_d);
    SET_EXPECT(global_propargputop_get_i);
    SET_EXPECT(global_propargputop_put_i);
    run_script(L"var t=0; propArgPutOp(t++, t++) ^= 14;");
    CHECK_CALLED(global_propargputop_d);
    CHECK_CALLED(global_propargputop_get_i);
    CHECK_CALLED(global_propargputop_put_i);

    SET_EXPECT(global_testargtypes_i);
    run_script(L"testArgTypes(dispUnk, intProp(), intProp, getShort(), shortProp,"
               L"function(i1,ui1,ui2,r4,i4ref,ui4,nullunk,d,i,s) {"
               L"    ok(getVT(i) === 'VT_I4', 'getVT(i) = ' + getVT(i));"
               L"    ok(getVT(s) === 'VT_I4', 'getVT(s) = ' + getVT(s));"
               L"    ok(getVT(d) === 'VT_DISPATCH', 'getVT(d) = ' + getVT(d));"
               L"    ok(getVT(nullunk) === 'VT_DISPATCH', 'getVT(nullunk) = ' + getVT(nullunk));"
               L"    ok(nullunk === null, 'nullunk !== null');"
               L"    ok(getVT(ui4) === 'VT_R8', 'getVT(ui4) = ' + getVT(ui4));"
               L"    ok(ui4 > 0, 'ui4 = ' + ui4);"
               L"    ok(getVT(i4ref) === 'VT_I4', 'getVT(i4ref) = ' + getVT(i4ref));"
               L"    ok(i4ref === 2, 'i4ref = ' + i4ref);"
               L"    ok(r4 === 0.5, 'r4 = ' + r4);"
               L"    ok(getVT(r4) === 'VT_R8', 'getVT(r4) = ' + getVT(r4));"
               L"    ok(getVT(ui2) === 'VT_I4', 'getVT(ui2) = ' + getVT(ui2));"
               L"    ok(getVT(ui1) === 'VT_I4', 'getVT(ui1) = ' + getVT(ui1));"
               L"    ok(ui1 === 4, 'ui1 = ' + ui1);"
               L"    ok(getVT(i1) === 'VT_I4', 'getVT(i1) = ' + getVT(i1));"
               L"    ok(i1 === 5, 'i1 = ' + i1);"
               L"});");
    CHECK_CALLED(global_testargtypes_i);

    SET_EXPECT(testobj_withprop_d);
    SET_EXPECT(testobj_withprop_i);
    run_script(L"var t = (function () { with(testObj) { return withProp; }})(); ok(t === 1, 't = ' + t);");
    CHECK_CALLED(testobj_withprop_d);
    CHECK_CALLED(testobj_withprop_i);

    SET_EXPECT(testobj_tolocalestr_d);
    SET_EXPECT(testobj_tolocalestr_i);
    run_script(L"var t = [testObj].toLocaleString(); ok(t === '1234', 't = ' + t);");
    CHECK_CALLED(testobj_tolocalestr_d);
    CHECK_CALLED(testobj_tolocalestr_i);

    run_script(L"@set @t=2\nok(@t === 2, '@t = ' + @t);");

    SET_EXPECT(global_success_d);
    SET_EXPECT(global_success_i);
    run_script(L"@if(true)\nif(@_jscript) reportSuccess();\n@end");
    CHECK_CALLED(global_success_d);
    CHECK_CALLED(global_success_i);

    EnumVARIANT_index = 0;
    EnumVARIANT_next_0_count = 1;
    SET_EXPECT(testobj_newenum);
    SET_EXPECT(enumvariant_next_0);
    run_script(L"new Enumerator(testObj);");
    CHECK_CALLED(testobj_newenum);
    CHECK_CALLED(enumvariant_next_0);

    EnumVARIANT_index = 0;
    EnumVARIANT_next_0_count = 2;
    SET_EXPECT(testobj_newenum);
    SET_EXPECT(enumvariant_next_0);
    SET_EXPECT(enumvariant_reset);
    run_script(L"(function () {"
               L"    var testEnumObj = new Enumerator(testObj);"
               L"    var tmp = testEnumObj.moveFirst();"
               L"    ok(tmp == undefined, \"testEnumObj.moveFirst() = \" + tmp);"
               L"})()");
    CHECK_CALLED(testobj_newenum);
    CHECK_CALLED(enumvariant_next_0);
    CHECK_CALLED(enumvariant_reset);

    EnumVARIANT_index = 0;
    EnumVARIANT_next_0_count = 1;
    SET_EXPECT(testobj_newenum);
    SET_EXPECT(enumvariant_next_0);
    SET_EXPECT(enumvariant_next_1);
    run_script(L"(function () {"
               L"    var testEnumObj = new Enumerator(testObj);"
               L"    while (!testEnumObj.atEnd())"
               L"    {"
               L"        ok(testEnumObj.item() == 123, "
               L"         \"testEnumObj.item() = \"+testEnumObj.item());"
               L"        testEnumObj.moveNext();"
               L"    }"
               L"})()");
    CHECK_CALLED(testobj_newenum);
    CHECK_CALLED(enumvariant_next_0);
    CHECK_CALLED(enumvariant_next_1);

    run_from_res("lang.js");
    run_from_res("api.js");
    run_from_res("regexp.js");
    run_from_res("cc.js");

    test_isvisible(FALSE);
    test_isvisible(TRUE);
    test_start();
    test_automagic();

    hres = parse_script(0, L"test.testThis2(this);");
    ok(hres == S_OK, "unexpected result %08lx\n", hres);
    hres = parse_script(0, L"(function () { test.testThis2(this); })();");
    ok(hres == S_OK, "unexpected result %08lx\n", hres);

    hres = parse_htmlscript(L"<!--");
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    hres = parse_htmlscript(L"-->");
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    hres = parse_htmlscript(L"<!--\nvar a=1;\n-->\n");
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    hres = parse_htmlscript(L"<!--\n<!-- ignore this\n-->\n");
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    hres = parse_htmlscript(L"var a=1;\nif(a-->0) a=5;\n");
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    hres = parse_htmlscript(L"var a=1;\nif(a\n-->0) a=5;\n");
    ok(hres != S_OK, "ParseScriptText have not failed\n");

    test_script_exprs();
    test_invokeex();
    test_members();
    test_destructors();
    test_eval();
    test_error_reports();

    run_bom_tests();

    return TRUE;
}

static void test_parse_proc(void)
{
    VARIANT args[2];
    DISPPARAMS dp = {args};

    dp.cArgs = 0;
    invoke_procedure(NULL, L"return true;", &dp);

    dp.cArgs = 1;
    V_VT(args) = VT_EMPTY;
    invoke_procedure(NULL, L"return arguments.length == 1;", &dp);

    V_VT(args) = VT_BOOL;
    V_BOOL(args) = VARIANT_TRUE;
    invoke_procedure(L" x ", L"return x;", &dp);

    dp.cArgs = 2;
    V_VT(args) = VT_I4;
    V_I4(args) = 2;
    V_VT(args+1) = VT_I4;
    V_I4(args+1) = 1;
    invoke_procedure(L" _x1 , y_2", L"return _x1 === 1 && y_2 === 2;", &dp);
}

static void run_encoded_tests(void)
{
    BSTR src;
    HRESULT hres;

    engine_clsid = &CLSID_JScriptEncode;

    SET_EXPECT(global_success_d);
    SET_EXPECT(global_success_i);
    /*             |reportSuccess();                           | */
    run_script(L"#@~^EAAAAA==.\x7fwGMYUEm1+kd`*iAQYAAA==^#~@");
    CHECK_CALLED(global_success_d);
    CHECK_CALLED(global_success_i);

    SET_EXPECT(global_success_d);
    SET_EXPECT(global_success_i);
    run_script(L"reportSuccess();");
    CHECK_CALLED(global_success_d);
    CHECK_CALLED(global_success_i);

    SET_EXPECT(global_success_d);
    SET_EXPECT(global_success_i);
    /*                   |Success                         | */
    run_script(L"report#@~^BwAAAA==j!m^\x7f/k2QIAAA==^#~@();");
    CHECK_CALLED(global_success_d);
    CHECK_CALLED(global_success_i);

    SET_EXPECT(global_success_d);
    SET_EXPECT(global_success_i);
    /*             |\r\n\treportSuccess();\r\n                        | */
    run_script(L"#@~^GQAAAA==@#@&d.\x7fwKDYUE1^+k/c#p@#@&OAYAAA==^#~@");
    CHECK_CALLED(global_success_d);
    CHECK_CALLED(global_success_i);

    /*                   v                                   */
    src = SysAllocString(L"#@~^EAA*AA==.\x7fwGMYUEm1+kd`*iAQYAAA==^#~@");
    hres = parse_script(SCRIPTITEM_GLOBALMEMBERS, src);
    SysFreeString(src);
    ok(hres == JS_E_INVALID_CHAR, "parse_script failed %08lx\n", hres);

    /*                      vv                                 */
    src = SysAllocString(L"#@~^EAAAAAAA==.\x7fwGMYUEm1+kd`*iAQYAAA==^#~@");
    hres = parse_script(SCRIPTITEM_GLOBALMEMBERS, src);
    SysFreeString(src);
    ok(hres == JS_E_INVALID_CHAR, "parse_script failed %08lx\n", hres);

    /*                      v                                */
    src = SysAllocString(L"#@~^EAAAAA^=.\x7fwGMYUEm1+kd`*iAQYAAA==^#~@");
    hres = parse_script(SCRIPTITEM_GLOBALMEMBERS, src);
    SysFreeString(src);
    ok(hres == JS_E_INVALID_CHAR, "parse_script failed %08lx\n", hres);

    /*                                     v                 */
    src = SysAllocString(L"#@~^EAAAAA==.\x7fwGMYUEm1ekd`*iAQYAAA==^#~@");
    hres = parse_script(SCRIPTITEM_GLOBALMEMBERS, src);
    SysFreeString(src);
    ok(hres == JS_E_INVALID_CHAR, "parse_script failed %08lx\n", hres);

    /*                                                    vv  */
    src = SysAllocString(L"#@~^EAAAAA==.\x7fwGMYUEm1+kd`*iAQYAAA==^~#@");
    hres = parse_script(SCRIPTITEM_GLOBALMEMBERS, src);
    SysFreeString(src);
    ok(hres == JS_E_INVALID_CHAR, "parse_script failed %08lx\n", hres);
}

static void run_benchmark(const char *script_name)
{
    IActiveScriptParse *parser;
    IActiveScript *engine;
    ULONG start, end;
    BSTR src;
    HRESULT hres;

    engine = create_script();
    if(!engine)
        return;

    hres = IActiveScript_QueryInterface(engine, &IID_IActiveScriptParse, (void**)&parser);
    ok(hres == S_OK, "Could not get IActiveScriptParse: %08lx\n", hres);
    if (FAILED(hres)) {
        IActiveScript_Release(engine);
        return;
    }

    hres = IActiveScriptParse_InitNew(parser);
    ok(hres == S_OK, "InitNew failed: %08lx\n", hres);

    hres = IActiveScript_SetScriptSite(engine, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08lx\n", hres);

    hres = IActiveScript_AddNamedItem(engine, L"test",
            SCRIPTITEM_ISVISIBLE|SCRIPTITEM_ISSOURCE);
    ok(hres == S_OK, "AddNamedItem failed: %08lx\n", hres);

    hres = IActiveScript_SetScriptState(engine, SCRIPTSTATE_STARTED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_STARTED) failed: %08lx\n", hres);

    src = load_res(script_name);

    start = GetTickCount();
    hres = IActiveScriptParse_ParseScriptText(parser, src, NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
    end = GetTickCount();
    ok(hres == S_OK, "%s: ParseScriptText failed: %08lx\n", script_name, hres);

    trace("%s ran in %lu ms\n", script_name, end-start);

    IActiveScript_Release(engine);
    IActiveScriptParse_Release(parser);
    SysFreeString(src);
}

static void run_benchmarks(void)
{
    trace("Running benchmarks...\n");

    run_benchmark("dna.js");
    run_benchmark("base64.js");
    run_benchmark("validateinput.js");
}

static BOOL check_jscript(void)
{
    IActiveScriptProperty *script_prop;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_JScript, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IActiveScriptProperty, (void**)&script_prop);
    if(FAILED(hres))
        return FALSE;
    IActiveScriptProperty_Release(script_prop);

    return parse_script(0, L"if(!('localeCompare' in String.prototype)) throw 1;") == S_OK;
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
        invoke_version = 2;
        run_from_file(argv[2]);
    }else {
        trace("invoke version 0\n");
        invoke_version = 0;
        run_tests();

        trace("invoke version 2\n");
        invoke_version = 2;
        if(run_tests()) {
            trace("JSctipt.Encode tests...\n");
            run_encoded_tests();
            trace("ParseProcedureText tests...\n");
            test_parse_proc();
        }

        if(winetest_interactive)
            run_benchmarks();
    }

    CoUninitialize();
}
