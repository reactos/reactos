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

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <stdio.h>

#define COBJMACROS
#define CONST_VTABLE

#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <ole2.h>
#include <dispex.h>
#include <activscp.h>

#include <wine/test.h>

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
DEFINE_EXPECT(global_testargtypes_i);
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
DEFINE_EXPECT(GetItemInfo_testVal);
DEFINE_EXPECT(ActiveScriptSite_OnScriptError);
DEFINE_EXPECT(invoke_func);
DEFINE_EXPECT(DeleteMemberByDispID);
DEFINE_EXPECT(DeleteMemberByDispID_false);
DEFINE_EXPECT(DeleteMemberByDispID_error);
DEFINE_EXPECT(BindHandler);

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

#define DISPID_GLOBAL_TESTPROPDELETE      0x2000
#define DISPID_GLOBAL_TESTNOPROPDELETE    0x2001
#define DISPID_GLOBAL_TESTPROPDELETEERROR 0x2002

#define DISPID_TESTOBJ_PROP         0x2000
#define DISPID_TESTOBJ_ONLYDISPID   0x2001
#define DISPID_TESTOBJ_WITHPROP     0x2002

#define JS_E_OUT_OF_MEMORY 0x800a03ec
#define JS_E_INVALID_CHAR 0x800a03f6

static const WCHAR testW[] = {'t','e','s','t',0};
static const CHAR testA[] = "test";
static const WCHAR test_valW[] = {'t','e','s','t','V','a','l',0};
static const CHAR test_valA[] = "testVal";
static const WCHAR emptyW[] = {0};

static BOOL strict_dispid_check, testing_expr;
static const char *test_name = "(null)";
static IDispatch *script_disp;
static int invoke_version;
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
    if(!strcmp_wa(bstrName, "withProp")) {
        CHECK_EXPECT(testobj_withprop_d);
        test_grfdex(grfdex, fdexNameCaseSensitive|fdexNameImplicit);
        *pid = DISPID_TESTOBJ_WITHPROP;
        return S_OK;
    }
    if(!strcmp_wa(bstrName, "noprop")) {
        CHECK_EXPECT(testobj_noprop_d);
        test_grfdex(grfdex, fdexNameCaseSensitive);
        return DISP_E_UNKNOWNNAME;
    }
    if(!strcmp_wa(bstrName, "onlyDispID")) {
        if(strict_dispid_check)
            CHECK_EXPECT(testobj_onlydispid_d);
        test_grfdex(grfdex, fdexNameCaseSensitive);
        *pid = DISPID_TESTOBJ_ONLYDISPID;
        return S_OK;
    }
    if(!strcmp_wa(bstrName, "notExists")) {
        CHECK_EXPECT(testobj_notexists_d);
        test_grfdex(grfdex, fdexNameCaseSensitive);
        return DISP_E_UNKNOWNNAME;
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
    }

    ok(0, "unexpected call %x\n", id);
    return DISP_E_MEMBERNOTFOUND;
}

static HRESULT WINAPI testObj_DeleteMemberByName(IDispatchEx *iface, BSTR bstrName, DWORD grfdex)
{
    if(!strcmp_wa(bstrName, "deleteTest")) {
        CHECK_EXPECT(testobj_delete_test);
        test_grfdex(grfdex, fdexNameCaseSensitive);
        return S_OK;
    }
    if(!strcmp_wa(bstrName, "noDeleteTest")) {
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
        ok(*pdp->rgdispidNamedArgs == DISPID_THIS, "*rgdispidNamedArgs = %d\n", *pdp->rgdispidNamedArgs);
        ok(pdp->cArgs == 2, "cArgs = %d\n", pdp->cArgs);
        ok(pdp->cNamedArgs == 1, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(res != NULL, "res == NULL\n");
        ok(wFlags == (DISPATCH_PROPERTYGET|DISPATCH_METHOD), "wFlags = %x\n", wFlags);
        ok(pei != NULL, "pei == NULL\n");

        ok(V_VT(pdp->rgvarg+1) == VT_BOOL, "V_VT(pdp->rgvarg+1) = %d\n", V_VT(pdp->rgvarg+1));
        ok(!V_BOOL(pdp->rgvarg+1), "V_BOOL(pdp->rgvarg+1) = %x\n", V_BOOL(pdp->rgvarg+1));

        ok(V_VT(pdp->rgvarg) == VT_DISPATCH, "V_VT(pdp->rgvarg) = %d\n", V_VT(pdp->rgvarg));
        ok(V_DISPATCH(pdp->rgvarg) != NULL, "V_DISPATCH(pdp->rgvarg) == NULL\n");

        if(res)
            V_VT(res) = VT_NULL;
        return S_OK;
    default:
        ok(0, "unexpected call %x\n", id);
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

    if(!strcmp_wa(*rgszNames, "prop")) {
        CHECK_EXPECT(puredisp_prop_d);
        *rgDispId = DISPID_TESTOBJ_PROP;
        return S_OK;
    } else if(!strcmp_wa(*rgszNames, "noprop")) {
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
        ok(wFlags == (DISPATCH_PROPERTYGET|DISPATCH_METHOD), "wFlags = %x\n", wFlags);
        ok(ei != NULL, "ei == NULL\n");
        ok(puArgErr != NULL, "puArgErr == NULL\n");

        ok(V_VT(pdp->rgvarg) == VT_BOOL, "V_VT(pdp->rgvarg) = %d\n", V_VT(pdp->rgvarg));
        ok(!V_BOOL(pdp->rgvarg), "V_BOOL(pdp->rgvarg) = %x\n", V_BOOL(pdp->rgvarg));

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
    ok(!strcmp_wa(event, "eventName"), "event = %s\n", wine_dbgstr_w(event));
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
    if(!strcmp_wa(bstrName, "testPropPutRef")) {
        CHECK_EXPECT(global_propputref_d);
        test_grfdex(grfdex, fdexNameCaseSensitive);
        *pid = DISPID_GLOBAL_TESTPROPPUTREF;
        return S_OK;
    }
    if(!strcmp_wa(bstrName, "testPropDelete")) {
        CHECK_EXPECT(global_propdelete_d);
        test_grfdex(grfdex, fdexNameCaseSensitive);
        *pid = DISPID_GLOBAL_TESTPROPDELETE;
        return S_OK;
    }
    if(!strcmp_wa(bstrName, "testNoPropDelete")) {
        CHECK_EXPECT(global_nopropdelete_d);
        test_grfdex(grfdex, fdexNameCaseSensitive);
        *pid = DISPID_GLOBAL_TESTNOPROPDELETE;
        return S_OK;
    }
    if(!strcmp_wa(bstrName, "testPropDeleteError")) {
        CHECK_EXPECT(global_propdeleteerror_d);
        test_grfdex(grfdex, fdexNameCaseSensitive);
        *pid = DISPID_GLOBAL_TESTPROPDELETEERROR;
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
    if(!strcmp_wa(bstrName, "getNullBSTR")) {
        *pid = DISPID_GLOBAL_GETNULLBSTR;
        return S_OK;
    }
    if(!strcmp_wa(bstrName, "isNullBSTR")) {
        *pid = DISPID_GLOBAL_ISNULLBSTR;
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
    if(!strcmp_wa(bstrName, "createArray")) {
        test_grfdex(grfdex, fdexNameCaseSensitive);
        *pid = DISPID_GLOBAL_CREATEARRAY;
        return S_OK;
    }
    if(!strcmp_wa(bstrName, "propGetFunc")) {
        test_grfdex(grfdex, fdexNameCaseSensitive);
        *pid = DISPID_GLOBAL_PROPGETFUNC;
        return S_OK;
    }
    if(!strcmp_wa(bstrName, "objectFlag")) {
        test_grfdex(grfdex, fdexNameCaseSensitive);
        *pid = DISPID_GLOBAL_OBJECT_FLAG;
        return S_OK;
    }

    if(!strcmp_wa(bstrName, "isWin64")) {
        test_grfdex(grfdex, fdexNameCaseSensitive);
        *pid = DISPID_GLOBAL_ISWIN64;
        return S_OK;
    }

    if(!strcmp_wa(bstrName, "pureDisp")) {
        test_grfdex(grfdex, fdexNameCaseSensitive);
        *pid = DISPID_GLOBAL_PUREDISP;
        return S_OK;
    }

    if(!strcmp_wa(bstrName, "propArgPutG")) {
        CHECK_EXPECT(global_propargput_d);
        test_grfdex(grfdex, fdexNameCaseSensitive);
        *pid = DISPID_GLOBAL_PROPARGPUT;
        return S_OK;
    }

    if(!strcmp_wa(bstrName, "propArgPutO")) {
        CHECK_EXPECT(global_propargput_d);
        test_grfdex(grfdex, fdexNameEnsure|fdexNameCaseSensitive);
        *pid = DISPID_GLOBAL_PROPARGPUT;
        return S_OK;
    }

    if(!strcmp_wa(bstrName, "shortProp")) {
        *pid = DISPID_GLOBAL_SHORTPROP;
        return S_OK;
    }

    if(!strcmp_wa(bstrName, "getShort")) {
        *pid = DISPID_GLOBAL_GETSHORT;
        return S_OK;
    }

    if(!strcmp_wa(bstrName, "testArgTypes")) {
        *pid = DISPID_GLOBAL_TESTARGTYPES;
        return S_OK;
    }

    if(!strcmp_wa(bstrName, "intProp")) {
        *pid = DISPID_GLOBAL_INTPROP;
        return S_OK;
    }

    if(!strcmp_wa(bstrName, "dispUnk")) {
        *pid = DISPID_GLOBAL_DISPUNK;
        return S_OK;
    }

    if(!strcmp_wa(bstrName, "testRes")) {
        *pid = DISPID_GLOBAL_TESTRES;
        return S_OK;
    }

    if(!strcmp_wa(bstrName, "testNoRes")) {
        *pid = DISPID_GLOBAL_TESTNORES;
        return S_OK;
    }

    if(!strcmp_wa(bstrName, "dispexFunc")) {
        *pid = DISPID_GLOBAL_DISPEXFUNC;
        return S_OK;
    }

    if(!strcmp_wa(bstrName, "getScriptState")) {
        *pid = DISPID_GLOBAL_GETSCRIPTSTATE;
        return S_OK;
    }

    if(!strcmp_wa(bstrName, "bindEventHandler")) {
        *pid = DISPID_GLOBAL_BINDEVENTHANDLER;
        return S_OK;
    }

    if(strict_dispid_check && strcmp_wa(bstrName, "t"))
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
        ok(pdp->rgdispidNamedArgs[0] == DISPID_PROPERTYPUT, "pdp->rgdispidNamedArgs[0] = %d\n", pdp->rgdispidNamedArgs[0]);
        ok(!pvarRes, "pvarRes != NULL\n");

        ok(V_VT(pdp->rgvarg) == VT_I4, "V_VT(pdp->rgvarg)=%d\n", V_VT(pdp->rgvarg));
        ok(V_I4(pdp->rgvarg) == 1, "V_I4(pdp->rgvarg)=%d\n", V_I4(pdp->rgvarg));
        return S_OK;

    case DISPID_GLOBAL_TESTPROPPUTREF:
        CHECK_EXPECT(global_propputref_i);

        ok(wFlags == (INVOKE_PROPERTYPUT|INVOKE_PROPERTYPUTREF), "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(pdp->rgdispidNamedArgs != NULL, "rgdispidNamedArgs == NULL\n");
        ok(pdp->cArgs == 1, "cArgs = %d\n", pdp->cArgs);
        ok(pdp->cNamedArgs == 1, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pdp->rgdispidNamedArgs[0] == DISPID_PROPERTYPUT, "pdp->rgdispidNamedArgs[0] = %d\n", pdp->rgdispidNamedArgs[0]);
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
        case VT_ARRAY|VT_VARIANT:
            V_BSTR(pvarRes) = a2bstr("VT_ARRAY|VT_VARIANT");
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
        ok(hres == S_OK, "GetScriptState failed: %08x\n", hres);

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
        ok(pdp->rgdispidNamedArgs[0] == DISPID_PROPERTYPUT, "pdp->rgdispidNamedArgs[0] = %d\n", pdp->rgdispidNamedArgs[0]);
        ok(!pvarRes, "pvarRes != NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        ok(V_VT(pdp->rgvarg) == VT_I4, "V_VT(pdp->rgvarg) = %d\n", V_VT(pdp->rgvarg));
        ok(V_I4(pdp->rgvarg) == 2, "V_I4(pdp->rgvarg) = %d\n", V_I4(pdp->rgvarg));

        ok(V_VT(pdp->rgvarg+1) == VT_I4, "V_VT(pdp->rgvarg+1) = %d\n", V_VT(pdp->rgvarg+1));
        ok(V_I4(pdp->rgvarg+1) == 1, "V_I4(pdp->rgvarg+1) = %d\n", V_I4(pdp->rgvarg+1));

        ok(V_VT(pdp->rgvarg+2) == VT_I4, "V_VT(pdp->rgvarg+2) = %d\n", V_VT(pdp->rgvarg+2));
        ok(V_I4(pdp->rgvarg+2) == 0, "V_I4(pdp->rgvarg+2) = %d\n", V_I4(pdp->rgvarg+2));
        return S_OK;

    case DISPID_GLOBAL_OBJECT_FLAG: {
        IDispatchEx *dispex;
        BSTR str;
        HRESULT hres;

        hres = IDispatch_QueryInterface(script_disp, &IID_IDispatchEx, (void**)&dispex);
        ok(hres == S_OK, "hres = %x\n", hres);

        str = a2bstr("Object");
        hres = IDispatchEx_GetDispID(dispex, str, fdexNameCaseSensitive, &id);
        SysFreeString(str);
        ok(hres == S_OK, "hres = %x\n", hres);

        hres = IDispatchEx_InvokeEx(dispex, id, lcid, DISPATCH_METHOD, pdp, NULL, pei, pspCaller);
        ok(hres == S_OK, "hres = %x\n", hres);

        V_VT(pvarRes) = VT_EMPTY;
        hres = IDispatchEx_InvokeEx(dispex, id, lcid, DISPATCH_METHOD, pdp, pvarRes, pei, pspCaller);
        ok(hres == S_OK, "hres = %x\n", hres);
        ok(V_VT(pvarRes) == VT_DISPATCH, "V_VT(pvarRes) = %d\n", V_VT(pvarRes));
        VariantClear(pvarRes);

        hres = IDispatchEx_InvokeEx(dispex, id, lcid, DISPATCH_METHOD|DISPATCH_PROPERTYGET, pdp, NULL, pei, pspCaller);
        ok(hres == S_OK, "hres = %x\n", hres);

        V_VT(pvarRes) = VT_EMPTY;
        hres = IDispatchEx_InvokeEx(dispex, id, lcid, DISPATCH_CONSTRUCT, pdp, pvarRes, pei, pspCaller);
        ok(hres == S_OK, "hres = %x\n", hres);
        ok(V_VT(pvarRes) == VT_DISPATCH, "V_VT(pvarRes) = %d\n", V_VT(pvarRes));
        VariantClear(pvarRes);

        hres = IDispatchEx_InvokeEx(dispex, id, lcid, DISPATCH_CONSTRUCT, pdp, NULL, pei, pspCaller);
        ok(hres == S_OK, "hres = %x\n", hres);

        V_VT(pvarRes) = VT_EMPTY;
        hres = IDispatchEx_InvokeEx(dispex, id, lcid, DISPATCH_CONSTRUCT|DISPATCH_PROPERTYGET, pdp, pvarRes, pei, pspCaller);
        ok(hres == E_INVALIDARG, "hres = %x\n", hres);

        V_VT(pvarRes) = VT_EMPTY;
        hres = IDispatchEx_InvokeEx(dispex, id, lcid, wFlags, pdp, pvarRes, pei, pspCaller);
        ok(hres == S_OK, "hres = %x\n", hres);
        ok(V_VT(pvarRes) == VT_DISPATCH, "V_VT(pvarRes) = %d\n", V_VT(pvarRes));
        IDispatchEx_Release(dispex);
        return S_OK;
    }
    case DISPID_GLOBAL_SHORTPROP:
    case DISPID_GLOBAL_GETSHORT:
        V_VT(pvarRes) = VT_I2;
        V_I2(pvarRes) = 10;
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
        VARIANT args[6], v;
        DISPPARAMS dp = {args, NULL, sizeof(args)/sizeof(*args), 0};
        HRESULT hres;

        CHECK_EXPECT(global_testargtypes_i);
        ok(wFlags == DISPATCH_METHOD, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg != NULL\n");
        ok(pdp->cArgs == 6, "cArgs = %d\n", pdp->cArgs);
        ok(!pvarRes, "pvarRes != NULL\n");

        ok(V_VT(pdp->rgvarg+1) == VT_I4, "V_VT(pdp->rgvarg+1) = %d\n", V_VT(pdp->rgvarg+1));
        ok(V_I4(pdp->rgvarg+1) == 10, "V_I4(pdp->rgvarg+1) = %d\n", V_I4(pdp->rgvarg+1));

        ok(V_VT(pdp->rgvarg+2) == VT_I4, "V_VT(pdp->rgvarg+2) = %d\n", V_VT(pdp->rgvarg+2));
        ok(V_I4(pdp->rgvarg+2) == 10, "V_I4(pdp->rgvarg+2) = %d\n", V_I4(pdp->rgvarg+2));

        ok(V_VT(pdp->rgvarg+3) == VT_I4, "V_VT(pdp->rgvarg+3) = %d\n", V_VT(pdp->rgvarg+3));
        ok(V_I4(pdp->rgvarg+3) == 22, "V_I4(pdp->rgvarg+3) = %d\n", V_I4(pdp->rgvarg+3));

        ok(V_VT(pdp->rgvarg+4) == VT_I4, "V_VT(pdp->rgvarg+4) = %d\n", V_VT(pdp->rgvarg+4));
        ok(V_I4(pdp->rgvarg+4) == 22, "V_I4(pdp->rgvarg+4) = %d\n", V_I4(pdp->rgvarg+4));

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
        V_VT(&v) = VT_I4;
        V_I4(&v) = 2;
        hres = IDispatch_Invoke(V_DISPATCH(pdp->rgvarg), DISPID_VALUE, &IID_NULL, 0, DISPATCH_METHOD, &dp, NULL, NULL, NULL);
        ok(hres == S_OK, "Invoke failed: %08x\n", hres);

        return S_OK;
    }
    }

    ok(0, "unexpected call %x\n", id);
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
        ok(0, "id = %d\n", id);
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

    hres = CoCreateInstance(engine_clsid, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
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

    hres = IActiveScriptParse_InitNew(parser);
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

    hres = IActiveScriptParse_ParseScriptText(parser, script_str, NULL, NULL, NULL, 0, 0, 0, NULL, NULL);

    IDispatch_Release(script_disp);
    IActiveScript_Release(engine);
    IActiveScriptParse_Release(parser);

    return hres;
}

static HRESULT invoke_procedure(const char *argsa, const char *sourcea, DISPPARAMS *dp)
{
    IActiveScriptParseProcedure2 *parse_proc;
    IActiveScriptParse *parser;
    IActiveScript *engine;
    IDispatchEx *dispex;
    EXCEPINFO ei = {0};
    BSTR source, args;
    IDispatch *disp;
    VARIANT res;
    HRESULT hres;

    engine = create_script();
    if(!engine)
        return S_OK;

    hres = IActiveScript_QueryInterface(engine, &IID_IActiveScriptParse, (void**)&parser);
    ok(hres == S_OK, "Could not get IActiveScriptParse: %08x\n", hres);

    hres = IActiveScriptParse_InitNew(parser);
    ok(hres == S_OK, "InitNew failed: %08x\n", hres);

    hres = IActiveScript_SetScriptSite(engine, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08x\n", hres);

    hres = IActiveScript_SetScriptState(engine, SCRIPTSTATE_STARTED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_STARTED) failed: %08x\n", hres);

    hres = IActiveScript_QueryInterface(engine, &IID_IActiveScriptParseProcedure2, (void**)&parse_proc);
    ok(hres == S_OK, "Could not get IActiveScriptParse: %08x\n", hres);

    source = a2bstr(sourcea);
    args = argsa ? a2bstr(argsa) : NULL;
    hres = IActiveScriptParseProcedure2_ParseProcedureText(parse_proc, source, args, emptyW, NULL, NULL, NULL, 0, 0,
        SCRIPTPROC_HOSTMANAGESSOURCE|SCRIPTPROC_IMPLICIT_THIS|SCRIPTPROC_IMPLICIT_PARENTS, &disp);
    ok(hres == S_OK, "ParseProcedureText failed: %08x\n", hres);
    SysFreeString(source);
    SysFreeString(args);

    hres = IDispatch_QueryInterface(disp, &IID_IDispatchEx, (void**)&dispex);
    ok(hres == S_OK, "Could not get IDispatchEx iface: %08x\n", hres);
    IDispatch_Release(disp);

    V_VT(&res) = VT_EMPTY;
    hres = IDispatchEx_InvokeEx(dispex, DISPID_VALUE, 0, DISPATCH_METHOD, dp, &res, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08x\n", hres);
    ok(V_VT(&res) == VT_BOOL && V_BOOL(&res), "InvokeEx returned vt %d (%x)\n", V_VT(&res), V_I4(&res));
    IDispatchEx_Release(dispex);

    IActiveScriptParseProcedure2_Release(parse_proc);
    IActiveScript_Release(engine);
    IActiveScriptParse_Release(parser);

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

    hres = IActiveScriptParse_InitNew(parser);
    ok(hres == S_OK, "InitNew failed: %08x\n", hres);

    hres = IActiveScript_SetScriptSite(engine, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08x\n", hres);

    hres = IActiveScript_AddNamedItem(engine, testW,
            SCRIPTITEM_ISVISIBLE|SCRIPTITEM_ISSOURCE|SCRIPTITEM_GLOBALMEMBERS);
    ok(hres == S_OK, "AddNamedItem failed: %08x\n", hres);

    hres = IActiveScript_SetScriptState(engine, SCRIPTSTATE_STARTED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_STARTED) failed: %08x\n", hres);

    hres = IActiveScriptParse_ParseScriptText(parser, script_str, NULL, NULL, tmp, 0, 0, 0, NULL, NULL);

    IActiveScript_Release(engine);
    IActiveScriptParse_Release(parser);
    SysFreeString(tmp);

    return hres;
}

static void test_IActiveScriptError(IActiveScriptError *error, SCODE errorcode, ULONG line, LONG pos, BSTR script_source, BSTR description, BSTR line_text)
{
    HRESULT hres;
    DWORD source_context;
    ULONG line_number;
    LONG char_position;
    BSTR linetext;
    EXCEPINFO excep;

    /* IActiveScriptError_GetSourcePosition */

    hres = IActiveScriptError_GetSourcePosition(error, NULL, NULL, NULL);
    ok(hres == S_OK, "IActiveScriptError_GetSourcePosition -- hres: expected S_OK, got 0x%08x\n", hres);

    source_context = 0xdeadbeef;
    hres = IActiveScriptError_GetSourcePosition(error, &source_context, NULL, NULL);
    ok(hres == S_OK, "IActiveScriptError_GetSourcePosition -- hres: expected S_OK, got 0x%08x\n", hres);
    ok(source_context == 0, "IActiveScriptError_GetSourcePosition -- source_context: expected 0, got 0x%08x\n", source_context);

    line_number = 0xdeadbeef;
    hres = IActiveScriptError_GetSourcePosition(error, NULL, &line_number, NULL);
    ok(hres == S_OK, "IActiveScriptError_GetSourcePosition -- hres: expected S_OK, got 0x%08x\n", hres);
    ok(line_number == line, "IActiveScriptError_GetSourcePosition -- line_number: expected %d, got %d\n", line, line_number);

    char_position = 0xdeadbeef;
    hres = IActiveScriptError_GetSourcePosition(error, NULL, NULL, &char_position);
    ok(hres == S_OK, "IActiveScriptError_GetSourcePosition -- hres: expected S_OK, got 0x%08x\n", hres);
    ok(char_position == pos, "IActiveScriptError_GetSourcePosition -- char_position: expected %d, got %d\n", pos, char_position);

    /* IActiveScriptError_GetSourceLineText */

    hres = IActiveScriptError_GetSourceLineText(error, NULL);
    ok(hres == E_POINTER, "IActiveScriptError_GetSourceLineText -- hres: expected E_POINTER, got 0x%08x\n", hres);

    linetext = NULL;
    hres = IActiveScriptError_GetSourceLineText(error, &linetext);
    if (line_text) {
        ok(hres == S_OK, "IActiveScriptError_GetSourceLineText -- hres: expected S_OK, got 0x%08x\n", hres);
        ok(linetext != NULL && !lstrcmpW(linetext, line_text),
           "IActiveScriptError_GetSourceLineText -- expected %s, got %s\n", wine_dbgstr_w(line_text), wine_dbgstr_w(linetext));
    } else {
        ok(hres == E_FAIL, "IActiveScriptError_GetSourceLineText -- hres: expected S_OK, got 0x%08x\n", hres);
        ok(linetext == NULL,
           "IActiveScriptError_GetSourceLineText -- expected NULL, got %s\n", wine_dbgstr_w(linetext));
    }
    SysFreeString(linetext);

    /* IActiveScriptError_GetExceptionInfo */

    hres = IActiveScriptError_GetExceptionInfo(error, NULL);
    ok(hres == E_POINTER, "IActiveScriptError_GetExceptionInfo -- hres: expected E_POINTER, got 0x%08x\n", hres);

    excep.wCode = 0xdead;
    excep.wReserved = 0xdead;
    excep.bstrSource = (BSTR)0xdeadbeef;
    excep.bstrDescription = (BSTR)0xdeadbeef;
    excep.bstrHelpFile = (BSTR)0xdeadbeef;
    excep.dwHelpContext = 0xdeadbeef;
    excep.pvReserved = (void *)0xdeadbeef;
    excep.pfnDeferredFillIn = (void *)0xdeadbeef;
    excep.scode = 0xdeadbeef;

    hres = IActiveScriptError_GetExceptionInfo(error, &excep);
    ok(hres == S_OK, "IActiveScriptError_GetExceptionInfo -- hres: expected S_OK, got 0x%08x\n", hres);

    ok(excep.wCode == 0, "IActiveScriptError_GetExceptionInfo -- excep.wCode: expected 0, got 0x%08x\n", excep.wCode);
    ok(excep.wReserved == 0, "IActiveScriptError_GetExceptionInfo -- excep.wReserved: expected 0, got %d\n", excep.wReserved);
    if (!is_lang_english())
        skip("Non-english UI (test with hardcoded strings)\n");
    else {
        ok(excep.bstrSource != NULL && !lstrcmpW(excep.bstrSource, script_source),
           "IActiveScriptError_GetExceptionInfo -- excep.bstrSource is not valid: expected %s, got %s\n",
           wine_dbgstr_w(script_source), wine_dbgstr_w(excep.bstrSource));
        ok(excep.bstrDescription != NULL && !lstrcmpW(excep.bstrDescription, description),
           "IActiveScriptError_GetExceptionInfo -- excep.bstrDescription is not valid: got %s\n", wine_dbgstr_w(excep.bstrDescription));
    }
    ok(excep.bstrHelpFile == NULL,
       "IActiveScriptError_GetExceptionInfo -- excep.bstrHelpFile: expected NULL, got %s\n", wine_dbgstr_w(excep.bstrHelpFile));
    ok(excep.dwHelpContext == 0, "IActiveScriptError_GetExceptionInfo -- excep.dwHelpContext: expected 0, got %d\n", excep.dwHelpContext);
    ok(excep.pvReserved == NULL, "IActiveScriptError_GetExceptionInfo -- excep.pvReserved: expected NULL, got %p\n", excep.pvReserved);
    ok(excep.pfnDeferredFillIn == NULL, "IActiveScriptError_GetExceptionInfo -- excep.pfnDeferredFillIn: expected NULL, got %p\n", excep.pfnDeferredFillIn);
    ok(excep.scode == errorcode, "IActiveScriptError_GetExceptionInfo -- excep.scode: expected 0x%08x, got 0x%08x\n", errorcode, excep.scode);

    SysFreeString(excep.bstrSource);
    SysFreeString(excep.bstrDescription);
    SysFreeString(excep.bstrHelpFile);
}

static void parse_script_with_error(DWORD flags, BSTR script_str, SCODE errorcode, ULONG line, LONG pos, BSTR script_source, BSTR description, BSTR line_text)
{
    IActiveScriptParse *parser;
    IActiveScript *engine;
    HRESULT hres;

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

    hres = IActiveScriptParse_InitNew(parser);
    ok(hres == S_OK, "InitNew failed: %08x\n", hres);

    hres = IActiveScript_SetScriptSite(engine, &ActiveScriptSite_CheckError);
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

    script_error = NULL;
    SET_EXPECT(ActiveScriptSite_OnScriptError);
    hres = IActiveScriptParse_ParseScriptText(parser, script_str, NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
    todo_wine ok(hres == 0x80020101, "parse_script_with_error should have returned 0x80020101, got: 0x%08x\n", hres);
    todo_wine CHECK_CALLED(ActiveScriptSite_OnScriptError);

    if (script_error)
    {
        test_IActiveScriptError(script_error, errorcode, line, pos, script_source, description, line_text);

        IActiveScriptError_Release(script_error);
    }

    IDispatch_Release(script_disp);
    IActiveScript_Release(engine);
    IActiveScriptParse_Release(parser);
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

static void parse_script_ae(const char *src, HRESULT exhres)
{
    BSTR tmp;
    HRESULT hres;

    tmp = a2bstr(src);
    hres = parse_script(SCRIPTITEM_GLOBALMEMBERS, tmp);
    SysFreeString(tmp);
    ok(hres == exhres, "parse_script failed: %08x, expected %08x\n", hres, exhres);
}

static void parse_script_with_error_a(const char *src, SCODE errorcode, ULONG line, LONG pos, LPCSTR source, LPCSTR desc, LPCSTR linetext)
{
    BSTR tmp, script_source, description, line_text;

    tmp = a2bstr(src);
    script_source = a2bstr(source);
    description = a2bstr(desc);
    line_text = a2bstr(linetext);

    parse_script_with_error(SCRIPTITEM_GLOBALMEMBERS, tmp, errorcode, line, pos, script_source, description, line_text);

    SysFreeString(line_text);
    SysFreeString(description);
    SysFreeString(script_source);
    SysFreeString(tmp);
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

    map = CreateFileMappingW(file, NULL, PAGE_READONLY, 0, 0, NULL);
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

    hres = IActiveScriptParse_InitNew(parser);
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
    hres = IActiveScriptParse_ParseScriptText(parser, script_textW, NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08x\n", hres);
    if(!global_members)
        CHECK_CALLED(GetItemInfo_testVal);

    hres = IActiveScriptParse_ParseScriptText(parser, script_textW, NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08x\n", hres);

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
    ok(hres == S_OK, "Could not get IActiveScriptParse: %08x\n", hres);

    hres = IActiveScriptParse_InitNew(parser);
    ok(hres == S_OK, "InitNew failed: %08x\n", hres);

    hres = IActiveScript_SetScriptSite(engine, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08x\n", hres);

    hres = IActiveScript_AddNamedItem(engine, testW, SCRIPTITEM_ISVISIBLE|SCRIPTITEM_ISSOURCE|SCRIPTITEM_GLOBALMEMBERS);
    ok(hres == S_OK, "AddNamedItem failed: %08x\n", hres);

    str = a2bstr("ok(getScriptState() === 5, \"getScriptState = \" + getScriptState());\n"
                 "reportSuccess();");
    hres = IActiveScriptParse_ParseScriptText(parser, str, NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08x\n", hres);
    SysFreeString(str);

    SET_EXPECT(global_success_d);
    SET_EXPECT(global_success_i);
    hres = IActiveScript_SetScriptState(engine, SCRIPTSTATE_STARTED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_STARTED) failed: %08x\n", hres);
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
    ok(hres == S_OK, "Could not get IActiveScriptParse: %08x\n", hres);

    hres = IActiveScriptParse_InitNew(parser);
    ok(hres == S_OK, "InitNew failed: %08x\n", hres);

    hres = IActiveScript_SetScriptSite(engine, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08x\n", hres);

    hres = IActiveScript_AddNamedItem(engine, testW, SCRIPTITEM_ISVISIBLE|SCRIPTITEM_ISSOURCE|SCRIPTITEM_GLOBALMEMBERS);
    ok(hres == S_OK, "AddNamedItem failed: %08x\n", hres);

    str = a2bstr("function bindEventHandler::eventName() {}\n"
                 "reportSuccess();");
    hres = IActiveScriptParse_ParseScriptText(parser, str, NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08x\n", hres);
    SysFreeString(str);

    SET_EXPECT(BindHandler);
    SET_EXPECT(global_success_d);
    SET_EXPECT(global_success_i);
    hres = IActiveScript_SetScriptState(engine, SCRIPTSTATE_STARTED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_STARTED) failed: %08x\n", hres);
    CHECK_CALLED(BindHandler);
    CHECK_CALLED(global_success_d);
    CHECK_CALLED(global_success_i);

    IActiveScript_Release(engine);
    IActiveScriptParse_Release(parser);
    script_engine = NULL;
}

static HRESULT parse_script_expr(const char *expr, VARIANT *res, IActiveScript **engine_ret)
{
    IActiveScriptParse *parser;
    IActiveScript *engine;
    BSTR str;
    HRESULT hres;

    engine = create_script();

    hres = IActiveScript_QueryInterface(engine, &IID_IActiveScriptParse, (void**)&parser);
    ok(hres == S_OK, "Could not get IActiveScriptParse: %08x\n", hres);

    hres = IActiveScriptParse_InitNew(parser);
    ok(hres == S_OK, "InitNew failed: %08x\n", hres);

    hres = IActiveScript_SetScriptSite(engine, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08x\n", hres);

    SET_EXPECT(GetItemInfo_testVal);
    hres = IActiveScript_AddNamedItem(engine, test_valW,
            SCRIPTITEM_ISVISIBLE|SCRIPTITEM_ISSOURCE|SCRIPTITEM_GLOBALMEMBERS);
    ok(hres == S_OK, "AddNamedItem failed: %08x\n", hres);
    CHECK_CALLED(GetItemInfo_testVal);

    hres = IActiveScript_SetScriptState(engine, SCRIPTSTATE_STARTED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_STARTED) failed: %08x\n", hres);

    str = a2bstr(expr);
    hres = IActiveScriptParse_ParseScriptText(parser, str, NULL, NULL, NULL, 0, 0, SCRIPTTEXT_ISEXPRESSION, res, NULL);
    SysFreeString(str);

    IActiveScriptParse_Release(parser);

    if(engine_ret) {
        *engine_ret = engine;
    }else {
        IActiveScript_Close(engine);
        IActiveScript_Release(engine);
    }
    return hres;
}

static void test_retval(void)
{
    BSTR str = a2bstr("reportSuccess(), true");
    IActiveScriptParse *parser;
    IActiveScript *engine;
    SCRIPTSTATE state;
    VARIANT res;
    HRESULT hres;

    engine = create_script();

    hres = IActiveScript_QueryInterface(engine, &IID_IActiveScriptParse, (void**)&parser);
    ok(hres == S_OK, "Could not get IActiveScriptParse: %08x\n", hres);

    hres = IActiveScriptParse_InitNew(parser);
    ok(hres == S_OK, "InitNew failed: %08x\n", hres);

    hres = IActiveScript_SetScriptSite(engine, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08x\n", hres);

    SET_EXPECT(GetItemInfo_testVal);
    hres = IActiveScript_AddNamedItem(engine, test_valW,
            SCRIPTITEM_ISVISIBLE|SCRIPTITEM_ISSOURCE|SCRIPTITEM_GLOBALMEMBERS);
    ok(hres == S_OK, "AddNamedItem failed: %08x\n", hres);
    CHECK_CALLED(GetItemInfo_testVal);

    V_VT(&res) = VT_NULL;
    SET_EXPECT(global_success_d);
    SET_EXPECT(global_success_i);
    hres = IActiveScriptParse_ParseScriptText(parser, str, NULL, NULL, NULL, 0, 0, 0, &res, NULL);
    CHECK_CALLED(global_success_d);
    CHECK_CALLED(global_success_i);
    ok(hres == S_OK, "ParseScriptText failed: %08x\n", hres);
    ok(V_VT(&res) == VT_EMPTY, "V_VT(&res) = %d\n", V_VT(&res));

    hres = IActiveScript_GetScriptState(engine, &state);
    ok(hres == S_OK, "GetScriptState failed: %08x\n", hres);
    ok(state == SCRIPTSTATE_INITIALIZED, "state = %d\n", state);

    hres = IActiveScript_SetScriptState(engine, SCRIPTSTATE_STARTED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_STARTED) failed: %08x\n", hres);

    hres = IActiveScript_Close(engine);
    ok(hres == S_OK, "Close failed: %08x\n", hres);

    IActiveScriptParse_Release(parser);
    IActiveScript_Release(engine);
    SysFreeString(str);
}

static void test_default_value(void)
{
    DISPPARAMS dp = {0};
    IDispatch *disp;
    VARIANT v;
    HRESULT hres;

    hres = parse_script_expr("new Date()", &v, NULL);
    ok(hres == S_OK, "parse_script_expr failed: %08x\n", hres);
    ok(V_VT(&v) == VT_DISPATCH, "V_VT(v) = %d\n", V_VT(&v));
    disp = V_DISPATCH(&v);

    V_VT(&v) = VT_EMPTY;
    hres = IDispatch_Invoke(disp, DISPID_VALUE, &IID_NULL, 0, DISPATCH_PROPERTYGET, &dp, &v, NULL, NULL);
    ok(hres == S_OK || broken(hres == 0x8000ffff), "Invoke failed: %08x\n", hres);
    if(hres == S_OK)
    {
        ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    }

    VariantClear(&v);
    IDispatch_Release(disp);
}

static void test_script_exprs(void)
{
    VARIANT v;
    HRESULT hres;

    testing_expr = TRUE;

    hres = parse_script_expr("true", &v, NULL);
    ok(hres == S_OK, "parse_script_expr failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BOOL, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_BOOL(&v) == VARIANT_TRUE, "V_BOOL(v) = %x\n", V_BOOL(&v));

    hres = parse_script_expr("false, true", &v, NULL);
    ok(hres == S_OK, "parse_script_expr failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BOOL, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_BOOL(&v) == VARIANT_TRUE, "V_BOOL(v) = %x\n", V_BOOL(&v));

    SET_EXPECT(global_success_d);
    SET_EXPECT(global_success_i);
    hres = parse_script_expr("reportSuccess(); true", &v, NULL);
    ok(hres == S_OK, "parse_script_expr failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BOOL, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_BOOL(&v) == VARIANT_TRUE, "V_BOOL(v) = %x\n", V_BOOL(&v));
    CHECK_CALLED(global_success_d);
    CHECK_CALLED(global_success_i);

    hres = parse_script_expr("if(false) true", &v, NULL);
    ok(hres == S_OK, "parse_script_expr failed: %08x\n", hres);
    ok(V_VT(&v) == VT_EMPTY, "V_VT(v) = %d\n", V_VT(&v));

    hres = parse_script_expr("return testPropGet", &v, NULL);
    ok(hres == 0x800a03fa, "parse_script_expr failed: %08x\n", hres);

    hres = parse_script_expr("reportSuccess(); return true", &v, NULL);
    ok(hres == 0x800a03fa, "parse_script_expr failed: %08x\n", hres);

    SET_EXPECT(global_success_d);
    SET_EXPECT(global_success_i);
    hres = parse_script_expr("reportSuccess(); true", NULL, NULL);
    ok(hres == S_OK, "parse_script_expr failed: %08x\n", hres);
    CHECK_CALLED(global_success_d);
    CHECK_CALLED(global_success_i);

    test_default_value();
    test_retval();

    testing_expr = FALSE;
}

static void test_invokeex(void)
{
    DISPID func_id, prop_id;
    DISPPARAMS dp = {NULL};
    IActiveScript *script;
    IDispatchEx *dispex;
    VARIANT v;
    BSTR str;
    HRESULT hres;

    hres = parse_script_expr("var o = {func: function() {return 3;}, prop: 6}; o", &v, &script);
    ok(hres == S_OK, "parse_script_expr failed: %08x\n", hres);
    ok(V_VT(&v) == VT_DISPATCH, "V_VT(v) = %d\n", V_VT(&v));

    hres = IDispatch_QueryInterface(V_DISPATCH(&v), &IID_IDispatchEx, (void**)&dispex);
    ok(hres == S_OK, "Could not get IDispatchEx iface: %08x\n", hres);
    VariantClear(&v);

    str = a2bstr("func");
    hres = IDispatchEx_GetDispID(dispex, str, 0, &func_id);
    SysFreeString(str);
    ok(hres == S_OK, "GetDispID failed: %08x\n", hres);

    str = a2bstr("prop");
    hres = IDispatchEx_GetDispID(dispex, str, 0, &prop_id);
    SysFreeString(str);
    ok(hres == S_OK, "GetDispID failed: %08x\n", hres);

    hres = IDispatchEx_InvokeEx(dispex, func_id, 0, DISPATCH_METHOD, &dp, &v, NULL, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08x\n", hres);
    ok(V_VT(&v) == VT_I4, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_I4(&v) == 3, "V_I4(v) = %d\n", V_I4(&v));

    hres = IDispatchEx_InvokeEx(dispex, prop_id, 0, DISPATCH_PROPERTYGET, &dp, &v, NULL, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08x\n", hres);
    ok(V_VT(&v) == VT_I4, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_I4(&v) == 6, "V_I4(v) = %d\n", V_I4(&v));

    hres = IActiveScript_SetScriptState(script, SCRIPTSTATE_UNINITIALIZED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_STARTED) failed: %08x\n", hres);

    str = a2bstr("func");
    hres = IDispatchEx_GetDispID(dispex, str, 0, &func_id);
    SysFreeString(str);
    ok(hres == S_OK, "GetDispID failed: %08x\n", hres);

    hres = IDispatchEx_InvokeEx(dispex, func_id, 0, DISPATCH_METHOD, &dp, &v, NULL, NULL);
    ok(hres == E_UNEXPECTED || broken(hres == 0x800a1393), "InvokeEx failed: %08x\n", hres);

    hres = IDispatchEx_InvokeEx(dispex, prop_id, 0, DISPATCH_PROPERTYGET, &dp, &v, NULL, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08x\n", hres);
    ok(V_VT(&v) == VT_I4, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_I4(&v) == 6, "V_I4(v) = %d\n", V_I4(&v));

    IDispatchEx_Release(dispex);
    IActiveScript_Release(script);
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
        {{'v','a','r',' ','a',' ','=',' ','1',';',' ','r','e','p','o','r','t','S','u','c','c','e','s','s','(',')',';','\0'}, S_OK},
        {{0xFEFF,'v','a','r',' ','a',' ','=',' ','1',';',' ','r','e','p','o','r','t','S','u','c','c','e','s','s','(',')',';','\0'}, S_OK},
        {{'v',0xFEFF,'a','r',' ','a',' ','=',' ','1',';',' ','r','e','p','o','r','t','S','u','c','c','e','s','s','(',')',';','\0'}, JS_E_OUT_OF_MEMORY},
        {{'v','a','r',0xFEFF,' ','a',' ','=',' ','1',';',' ','r','e','p','o','r','t','S','u','c','c','e','s','s','(',')',';','\0'}, S_OK},
        {{'v','a','r',' ','a',' ','=',' ','1',';',' ',0xFEFF,'r','e','p','o','r','t','S','u','c','c','e','s','s','(',')',';','\0'}, S_OK},
        {{'v','a','r',' ','a',' ','=',' ','1',';',' ','r','e','p','o','r','t',0xFEFF,'S','u','c','c','e','s','s','(',')',';','\0'}, JS_E_OUT_OF_MEMORY},
        {{'v','a','r',' ','a',' ','=',' ','1',';',' ','r','e','p','o','r','t','S','u','c','c','e','s','s',0xFEFF,'(',')',';','\0'}, S_OK},
        {{'v','a','r',' ','a',' ','=',' ','1',';',' ','r','e','p','o','r','t','S','u','c','c','e','s','s','(',0xFEFF,')',';','\0'}, S_OK},
        {{'v','a','r',' ','a',' ','=',0xFEFF,' ','1',';',' ','r','e','p','o','r','t','S','u','c','c','e','s','s','(',0xFEFF,')',';','\0'}, S_OK},
        {{0xFEFF,'v','a','r',' ','a',' ','=',0xFEFF,0xFEFF,' ','1',';',' ','r','e','p','o','r','t','S','u','c','c','e','s','s','(',0xFEFF,')',';','\0'}, S_OK},
        {{0}}
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
             ok(hres == S_OK, "test %s failed with %08x\n", wine_dbgstr_w(src), hres);
             SysFreeString(src);
             CHECK_CALLED(global_success_d);
             CHECK_CALLED(global_success_i);
        }
        else
        {
             src = SysAllocString(bom_tests[i].str);
             hres = parse_script(SCRIPTITEM_GLOBALMEMBERS, src);
             todo_wine ok(hres == bom_tests[i].hres, "test %s returned with %08x\n", wine_dbgstr_w(src), hres);
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

    SET_EXPECT(global_propputref_d);
    SET_EXPECT(global_propputref_i);
    parse_script_a("testPropPutRef = new Object();");
    CHECK_CALLED(global_propputref_d);
    CHECK_CALLED(global_propputref_i);

    SET_EXPECT(global_propputref_d);
    SET_EXPECT(global_propputref_i);
    parse_script_a("testPropPutRef = testObj;");
    CHECK_CALLED(global_propputref_d);
    CHECK_CALLED(global_propputref_i);

    SET_EXPECT(global_success_d);
    SET_EXPECT(global_success_i);
    parse_script_a("reportSuccess();");
    CHECK_CALLED(global_success_d);
    CHECK_CALLED(global_success_i);

    SET_EXPECT(testobj_delete_test);
    parse_script_a("ok((delete testObj.deleteTest) === true, 'delete testObj.deleteTest did not return true');");
    CHECK_CALLED(testobj_delete_test);

    SET_EXPECT(testobj_delete_nodelete);
    parse_script_a("ok((delete testObj.noDeleteTest) === false, 'delete testObj.noDeleteTest did not return false');");
    CHECK_CALLED(testobj_delete_nodelete);

    SET_EXPECT(global_propdelete_d);
    SET_EXPECT(DeleteMemberByDispID);
    parse_script_a("ok((delete testPropDelete) === true, 'delete testPropDelete did not return true');");
    CHECK_CALLED(global_propdelete_d);
    CHECK_CALLED(DeleteMemberByDispID);

    SET_EXPECT(global_nopropdelete_d);
    SET_EXPECT(DeleteMemberByDispID_false);
    parse_script_a("ok((delete testNoPropDelete) === false, 'delete testPropDelete did not return false');");
    CHECK_CALLED(global_nopropdelete_d);
    CHECK_CALLED(DeleteMemberByDispID_false);

    SET_EXPECT(global_propdeleteerror_d);
    SET_EXPECT(DeleteMemberByDispID_error);
    parse_script_ae("delete testPropDeleteError;", E_FAIL);
    CHECK_CALLED(global_propdeleteerror_d);
    CHECK_CALLED(DeleteMemberByDispID_error);

    SET_EXPECT(puredisp_prop_d);
    parse_script_a("ok((delete pureDisp.prop) === false, 'delete pureDisp.prop did not return true');");
    CHECK_CALLED(puredisp_prop_d);

    SET_EXPECT(puredisp_noprop_d);
    parse_script_a("ok((delete pureDisp.noprop) === true, 'delete pureDisp.noprop did not return false');");
    CHECK_CALLED(puredisp_noprop_d);

    SET_EXPECT(puredisp_value);
    parse_script_a("var t=pureDisp; t=t(false);");
    CHECK_CALLED(puredisp_value);

    SET_EXPECT(puredisp_value);
    parse_script_a("var t = {func: pureDisp}; t = t.func(false);");
    CHECK_CALLED(puredisp_value);

    SET_EXPECT(dispexfunc_value);
    parse_script_a("var t = dispexFunc; t = t(false);");
    CHECK_CALLED(dispexfunc_value);

    SET_EXPECT(dispexfunc_value);
    parse_script_a("var t = {func: dispexFunc}; t = t.func(false);");
    CHECK_CALLED(dispexfunc_value);

    parse_script_a("(function reportSuccess() {})()");

    parse_script_a("ok(typeof(test) === 'object', \"typeof(test) != 'object'\");");

    parse_script_a("function reportSuccess() {}; reportSuccess();");

    SET_EXPECT(global_propget_d);
    parse_script_a("var testPropGet");
    CHECK_CALLED(global_propget_d);

    SET_EXPECT(global_propget_d);
    parse_script_a("eval('var testPropGet;');");
    CHECK_CALLED(global_propget_d);

    SET_EXPECT(global_notexists_d);
    parse_script_a("var notExists; notExists = 1;");
    CHECK_CALLED(global_notexists_d);

    SET_EXPECT(testobj_notexists_d);
    parse_script_a("testObj.notExists;");
    CHECK_CALLED(testobj_notexists_d);

    parse_script_a("function f() { var testPropGet; }");
    parse_script_a("(function () { var testPropGet; })();");
    parse_script_a("(function () { eval('var testPropGet;'); })();");

    SET_EXPECT(invoke_func);
    parse_script_a("ok(propGetFunc() == 0, \"Incorrect propGetFunc value\");");
    CHECK_CALLED(invoke_func);
    parse_script_a("ok(propGetFunc(1) == 1, \"Incorrect propGetFunc value\");");
    parse_script_a("ok(propGetFunc(1, 2) == 2, \"Incorrect propGetFunc value\");");
    SET_EXPECT(invoke_func);
    parse_script_a("ok(propGetFunc().toString() == 0, \"Incorrect propGetFunc value\");");
    CHECK_CALLED(invoke_func);
    parse_script_a("ok(propGetFunc(1).toString() == 1, \"Incorrect propGetFunc value\");");
    SET_EXPECT(invoke_func);
    parse_script_a("propGetFunc(1);");
    CHECK_CALLED(invoke_func);

    parse_script_a("objectFlag(1).toString();");

    parse_script_a("(function() { var tmp = (function () { return testObj; })()(1);})();");
    parse_script_a("(function() { var tmp = (function () { return testObj; })()();})();");

    parse_script_a("ok((testObj instanceof Object) === false, 'testObj is instance of Object');");

    SET_EXPECT(testobj_prop_d);
    parse_script_a("ok(('prop' in testObj) === true, 'prop is not in testObj');");
    CHECK_CALLED(testobj_prop_d);

    SET_EXPECT(testobj_noprop_d);
    parse_script_a("ok(('noprop' in testObj) === false, 'noprop is in testObj');");
    CHECK_CALLED(testobj_noprop_d);

    SET_EXPECT(testobj_prop_d);
    parse_script_a("ok(Object.prototype.hasOwnProperty.call(testObj, 'prop') === true, 'hasOwnProperty(\\\"prop\\\") returned false');");
    CHECK_CALLED(testobj_prop_d);

    SET_EXPECT(testobj_noprop_d);
    parse_script_a("ok(Object.prototype.hasOwnProperty.call(testObj, 'noprop') === false, 'hasOwnProperty(\\\"noprop\\\") returned true');");
    CHECK_CALLED(testobj_noprop_d);

    SET_EXPECT(puredisp_prop_d);
    parse_script_a("ok(Object.prototype.hasOwnProperty.call(pureDisp, 'prop') === true, 'hasOwnProperty(\\\"noprop\\\") returned false');");
    CHECK_CALLED(puredisp_prop_d);

    SET_EXPECT(puredisp_noprop_d);
    parse_script_a("ok(Object.prototype.hasOwnProperty.call(pureDisp, 'noprop') === false, 'hasOwnProperty(\\\"noprop\\\") returned true');");
    CHECK_CALLED(puredisp_noprop_d);

    SET_EXPECT(testobj_value);
    parse_script_a("ok(String(testObj) === '1', 'wrong testObj value');");
    CHECK_CALLED(testobj_value);

    SET_EXPECT(testobj_value);
    parse_script_a("ok(String.prototype.concat.call(testObj, ' OK') === '1 OK', 'wrong concat result');");
    CHECK_CALLED(testobj_value);

    SET_EXPECT(testobj_construct);
    parse_script_a("var t = new testObj(1);");
    CHECK_CALLED(testobj_construct);

    SET_EXPECT(global_propget_d);
    SET_EXPECT(global_propget_i);
    parse_script_a("this.testPropGet;");
    CHECK_CALLED(global_propget_d);
    CHECK_CALLED(global_propget_i);

    SET_EXPECT(global_propputref_d);
    SET_EXPECT(global_propputref_i);
    parse_script_a("testPropPutRef = nullDisp;");
    CHECK_CALLED(global_propputref_d);
    CHECK_CALLED(global_propputref_i);

    SET_EXPECT(global_propget_d);
    SET_EXPECT(global_propget_i);
    parse_script_a("(function () { this.testPropGet; })();");
    CHECK_CALLED(global_propget_d);
    CHECK_CALLED(global_propget_i);

    parse_script_a("testThis(this);");
    parse_script_a("(function () { testThis(this); })();");
    parse_script_a("function x() { testThis(this); }; x();");
    parse_script_a("var t = {func: function () { ok(this === t, 'this !== t'); }}; with(t) { func(); }");
    parse_script_a("function x() { testThis(this); }; with({y: 1}) { x(); }");
    parse_script_a("(function () { function x() { testThis(this);} x(); })();");

    SET_EXPECT(testobj_onlydispid_d);
    SET_EXPECT(testobj_onlydispid_i);
    parse_script_a("ok(typeof(testObj.onlyDispID) === 'unknown', 'unexpected typeof(testObj.onlyDispID)');");
    CHECK_CALLED(testobj_onlydispid_d);
    CHECK_CALLED(testobj_onlydispid_i);

    SET_EXPECT(global_propargput_d);
    SET_EXPECT(global_propargput_i);
    parse_script_a("var t=0; propArgPutG(t++, t++) = t++;");
    CHECK_CALLED(global_propargput_d);
    CHECK_CALLED(global_propargput_i);

    SET_EXPECT(global_propargput_d);
    SET_EXPECT(global_propargput_i);
    parse_script_a("var t=0; test.propArgPutO(t++, t++) = t++;");
    CHECK_CALLED(global_propargput_d);
    CHECK_CALLED(global_propargput_i);

    SET_EXPECT(global_testargtypes_i);
    parse_script_a("testArgTypes(dispUnk, intProp(), intProp, getShort(), shortProp, function(i4ref,ui4,nullunk,d,i,s) {"
                   "    ok(getVT(i) === 'VT_I4', 'getVT(i) = ' + getVT(i));"
                   "    ok(getVT(s) === 'VT_I4', 'getVT(s) = ' + getVT(s));"
                   "    ok(getVT(d) === 'VT_DISPATCH', 'getVT(d) = ' + getVT(d));"
                   "    ok(getVT(nullunk) === 'VT_DISPATCH', 'getVT(nullunk) = ' + getVT(nullunk));"
                   "    ok(nullunk === null, 'nullunk !== null');"
                   "    ok(getVT(ui4) === 'VT_R8', 'getVT(ui4) = ' + getVT(ui4));"
                   "    ok(ui4 > 0, 'ui4 = ' + ui4);"
                   "    ok(getVT(i4ref) === 'VT_I4', 'getVT(i4ref) = ' + getVT(i4ref));"
                   "    ok(i4ref === 2, 'i4ref = ' + i4ref);"
                   "});");
    CHECK_CALLED(global_testargtypes_i);

    SET_EXPECT(testobj_withprop_d);
    SET_EXPECT(testobj_withprop_i);
    parse_script_a("var t = (function () { with(testObj) { return withProp; }})(); ok(t === 1, 't = ' + t);");
    CHECK_CALLED(testobj_withprop_d);
    CHECK_CALLED(testobj_withprop_i);

    parse_script_a("@set @t=2\nok(@t === 2, '@t = ' + @t);");

    SET_EXPECT(global_success_d);
    SET_EXPECT(global_success_i);
    parse_script_a("@if(true)\nif(@_jscript) reportSuccess();\n@end");
    CHECK_CALLED(global_success_d);
    CHECK_CALLED(global_success_i);

    run_from_res("lang.js");
    run_from_res("api.js");
    run_from_res("regexp.js");
    run_from_res("cc.js");

    test_isvisible(FALSE);
    test_isvisible(TRUE);
    test_start();
    test_automagic();

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

    test_script_exprs();
    test_invokeex();

    parse_script_with_error_a(
        "?",
        0x800a03ea, 0, 0,
        "Microsoft JScript compilation error",
        "Syntax error",
        "?");

    parse_script_with_error_a(
        "var a=1;\nif(a\n-->0) a=5;\n",
        0x800a03ee, 2, 0,
        "Microsoft JScript compilation error",
        "Expected ')'",
        "-->0) a=5;");

    parse_script_with_error_a(
        "new 3;",
        0x800a01bd, 0, 0,
        "Microsoft JScript runtime error",
        "Object doesn't support this action",
        NULL);

    parse_script_with_error_a(
        "new null;",
        0x800a138f, 0, 0,
        "Microsoft JScript runtime error",
        "Object expected",
        NULL);

    parse_script_with_error_a(
        "var a;\nnew null;",
        0x800a138f, 1, 0,
        "Microsoft JScript runtime error",
        "Object expected",
        NULL);

    parse_script_with_error_a(
        "var a; new null;",
        0x800a138f, 0, 7,
        "Microsoft JScript runtime error",
        "Object expected",
        NULL);

    run_bom_tests();

    return TRUE;
}

static void test_parse_proc(void)
{
    VARIANT args[2];
    DISPPARAMS dp = {args};

    dp.cArgs = 0;
    invoke_procedure(NULL, "return true;", &dp);

    dp.cArgs = 1;
    V_VT(args) = VT_EMPTY;
    invoke_procedure(NULL, "return arguments.length == 1;", &dp);

    V_VT(args) = VT_BOOL;
    V_BOOL(args) = VARIANT_TRUE;
    invoke_procedure(" x ", "return x;", &dp);

    dp.cArgs = 2;
    V_VT(args) = VT_I4;
    V_I4(args) = 2;
    V_VT(args+1) = VT_I4;
    V_I4(args+1) = 1;
    invoke_procedure(" _x1 , y_2", "return _x1 === 1 && y_2 === 2;", &dp);
}

static void run_encoded_tests(void)
{
    BSTR src;
    HRESULT hres;

    engine_clsid = &CLSID_JScriptEncode;

    SET_EXPECT(global_success_d);
    SET_EXPECT(global_success_i);
    /*             |reportSuccess();                           | */
    parse_script_a("#@~^EAAAAA==.\x7fwGMYUEm1+kd`*iAQYAAA==^#~@");
    CHECK_CALLED(global_success_d);
    CHECK_CALLED(global_success_i);

    SET_EXPECT(global_success_d);
    SET_EXPECT(global_success_i);
    parse_script_a("reportSuccess();");
    CHECK_CALLED(global_success_d);
    CHECK_CALLED(global_success_i);

    SET_EXPECT(global_success_d);
    SET_EXPECT(global_success_i);
    /*                   |Success                         | */
    parse_script_a("report#@~^BwAAAA==j!m^\x7f/k2QIAAA==^#~@();");
    CHECK_CALLED(global_success_d);
    CHECK_CALLED(global_success_i);

    SET_EXPECT(global_success_d);
    SET_EXPECT(global_success_i);
    /*             |\r\n\treportSuccess();\r\n                        | */
    parse_script_a("#@~^GQAAAA==@#@&d.\x7fwKDYUE1^+k/c#p@#@&OAYAAA==^#~@");
    CHECK_CALLED(global_success_d);
    CHECK_CALLED(global_success_i);

    /*                   v                                   */
    src = a2bstr("#@~^EAA*AA==.\x7fwGMYUEm1+kd`*iAQYAAA==^#~@");
    hres = parse_script(SCRIPTITEM_GLOBALMEMBERS, src);
    SysFreeString(src);
    ok(hres == JS_E_INVALID_CHAR, "parse_script failed %08x\n", hres);

    /*                      vv                                 */
    src = a2bstr("#@~^EAAAAAAA==.\x7fwGMYUEm1+kd`*iAQYAAA==^#~@");
    hres = parse_script(SCRIPTITEM_GLOBALMEMBERS, src);
    SysFreeString(src);
    ok(hres == JS_E_INVALID_CHAR, "parse_script failed %08x\n", hres);

    /*                      v                                */
    src = a2bstr("#@~^EAAAAA^=.\x7fwGMYUEm1+kd`*iAQYAAA==^#~@");
    hres = parse_script(SCRIPTITEM_GLOBALMEMBERS, src);
    SysFreeString(src);
    ok(hres == JS_E_INVALID_CHAR, "parse_script failed %08x\n", hres);

    /*                                     v                 */
    src = a2bstr("#@~^EAAAAA==.\x7fwGMYUEm1ekd`*iAQYAAA==^#~@");
    hres = parse_script(SCRIPTITEM_GLOBALMEMBERS, src);
    SysFreeString(src);
    ok(hres == JS_E_INVALID_CHAR, "parse_script failed %08x\n", hres);

    /*                                                    vv  */
    src = a2bstr("#@~^EAAAAA==.\x7fwGMYUEm1+kd`*iAQYAAA==^~#@");
    hres = parse_script(SCRIPTITEM_GLOBALMEMBERS, src);
    SysFreeString(src);
    ok(hres == JS_E_INVALID_CHAR, "parse_script failed %08x\n", hres);
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
    ok(hres == S_OK, "Could not get IActiveScriptParse: %08x\n", hres);
    if (FAILED(hres)) {
        IActiveScript_Release(engine);
        return;
    }

    hres = IActiveScriptParse_InitNew(parser);
    ok(hres == S_OK, "InitNew failed: %08x\n", hres);

    hres = IActiveScript_SetScriptSite(engine, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08x\n", hres);

    hres = IActiveScript_AddNamedItem(engine, testW,
            SCRIPTITEM_ISVISIBLE|SCRIPTITEM_ISSOURCE);
    ok(hres == S_OK, "AddNamedItem failed: %08x\n", hres);

    hres = IActiveScript_SetScriptState(engine, SCRIPTSTATE_STARTED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_STARTED) failed: %08x\n", hres);

    src = load_res(script_name);

    start = GetTickCount();
    hres = IActiveScriptParse_ParseScriptText(parser, src, NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
    end = GetTickCount();
    ok(hres == S_OK, "%s: ParseScriptText failed: %08x\n", script_name, hres);

    trace("%s ran in %u ms\n", script_name, end-start);

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
    BSTR str;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_JScript, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IActiveScriptProperty, (void**)&script_prop);
    if(FAILED(hres))
        return FALSE;
    IActiveScriptProperty_Release(script_prop);

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
