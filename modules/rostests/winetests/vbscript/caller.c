/*
 * Copyright 2023 Gabriel Ivăncescu for CodeWeavers
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

extern const CLSID CLSID_VBScript;

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
DEFINE_EXPECT(testGetCaller);
DEFINE_EXPECT(testGetCallerVBS);
DEFINE_EXPECT(testGetCallerNested);
DEFINE_EXPECT(OnEnterScript);
DEFINE_EXPECT(OnLeaveScript);

static IActiveScript *active_script;
static IServiceProvider *test_get_caller_sp;

#define DISPID_TEST_TESTGETCALLER    0x1000
#define DISPID_TEST_TESTGETCALLERVBS 0x1001
#define DISPID_TEST_TESTGETCALLERNESTED 0x1002

#define parse_script(a,s) _parse_script(__LINE__,a,s)
static void _parse_script(unsigned line, IActiveScript *active_script, const WCHAR *script)
{
    IActiveScriptParse *parser;
    HRESULT hres;

    hres = IActiveScript_QueryInterface(active_script, &IID_IActiveScriptParse, (void**)&parser);
    ok_(__FILE__,line)(hres == S_OK, "Could not get IActiveScriptParse: %08lx\n", hres);

    hres = IActiveScriptParse_ParseScriptText(parser, script, NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
    ok_(__FILE__,line)(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    IActiveScriptParse_Release(parser);
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

static HRESULT WINAPI DispatchEx_GetIDsOfNames(IDispatchEx *iface, REFIID riid, LPOLESTR *rgszNames,
        UINT cNames, LCID lcid, DISPID *rgDispId)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_Invoke(IDispatchEx *iface, DISPID dispIdMember, REFIID riid, LCID lcid,
        WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_DeleteMemberByName(IDispatchEx *iface, BSTR bstrName, DWORD grfdex)
{
    ok(0, "unexpected call\n");
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
    if(!lstrcmpW(bstrName, L"testGetCaller")) {
        ok(grfdex == fdexNameCaseInsensitive, "grfdex = %lx\n", grfdex);
        *pid = DISPID_TEST_TESTGETCALLER;
        return S_OK;
    }
    if(!lstrcmpW(bstrName, L"testGetCallerVBS")) {
        ok(grfdex == fdexNameCaseInsensitive, "grfdex = %lx\n", grfdex);
        *pid = DISPID_TEST_TESTGETCALLERVBS;
        return S_OK;
    }
    if(!lstrcmpW(bstrName, L"testGetCallerNested")) {
        ok(grfdex == fdexNameCaseInsensitive, "grfdex = %lx\n", grfdex);
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
        parse_script(active_script, L"Call testGetCallerNested(1,2)");
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

    case DISPID_TEST_TESTGETCALLERVBS: {
        IUnknown *unk;

        CHECK_EXPECT(testGetCallerVBS);

        ok(wFlags == DISPATCH_METHOD, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->cArgs == 1, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(!pvarRes, "pvarRes != NULL\n");
        ok(pei != NULL, "pei == NULL\n");
        ok(V_VT(pdp->rgvarg) == VT_I2, "V_VT(rgvarg) = %d\n", V_VT(pdp->rgvarg));
        ok(V_I2(pdp->rgvarg) == 42, "V_I2(rgvarg) = %d\n", V_I2(pdp->rgvarg));

        hres = IServiceProvider_QueryService(pspCaller, &SID_GetCaller, &IID_IServiceProvider, (void**)&caller);
        ok(hres == E_NOINTERFACE, "QueryService(SID_GetCaller) returned: %08lx\n", hres);
        ok(caller == NULL, "caller != NULL\n");

        SET_EXPECT(site_QI_NULL);
        hres = IServiceProvider_QueryService(pspCaller, &IID_IActiveScriptSite, &IID_NULL, (void**)&unk);
        ok(hres == E_NOINTERFACE, "QueryService(IActiveScriptSite->NULL) returned: %08lx\n", hres);
        ok(!unk, "unk != NULL\n");
        CHECK_CALLED(site_QI_NULL);
        break;
    }

    case DISPID_TEST_TESTGETCALLERNESTED:
        CHECK_EXPECT(testGetCallerNested);

        ok(wFlags == DISPATCH_METHOD, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->cArgs == 2, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(!pvarRes, "pvarRes != NULL\n");
        ok(pei != NULL, "pei == NULL\n");
        ok(V_VT(&pdp->rgvarg[0]) == VT_I2, "V_VT(rgvarg[0]) = %d\n", V_VT(&pdp->rgvarg[0]));
        ok(V_VT(&pdp->rgvarg[1]) == VT_I2, "V_VT(rgvarg[1]) = %d\n", V_VT(&pdp->rgvarg[1]));
        ok(V_I2(&pdp->rgvarg[0]) == 2, "V_I2(rgvarg[0]) = %d\n", V_I2(&pdp->rgvarg[0]));
        ok(V_I2(&pdp->rgvarg[1]) == 1, "V_I2(rgvarg[1]) = %d\n", V_I2(&pdp->rgvarg[1]));

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

static IActiveScript *create_script(void)
{
    IActiveScriptParse *parser;
    IActiveScript *script;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_VBScript, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IActiveScript, (void**)&script);
    if(FAILED(hres))
        return NULL;

    hres = IActiveScript_QueryInterface(script, &IID_IActiveScriptParse, (void**)&parser);
    ok(hres == S_OK, "Could not get IActiveScriptParse: %08lx\n", hres);

    hres = IActiveScriptParse_InitNew(parser);
    ok(hres == S_OK, "InitNew failed: %08lx\n", hres);
    IActiveScriptParse_Release(parser);

    hres = IActiveScript_SetScriptSite(script, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08lx\n", hres);

    hres = IActiveScript_AddNamedItem(script, L"test",
            SCRIPTITEM_ISVISIBLE|SCRIPTITEM_ISSOURCE|SCRIPTITEM_GLOBALMEMBERS);
    ok(hres == S_OK, "AddNamedItem failed: %08lx\n", hres);

    hres = IActiveScript_SetScriptState(script, SCRIPTSTATE_STARTED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_STARTED) failed: %08lx\n", hres);

    return script;
}

static void run_scripts(void)
{
    DISPPARAMS dp = { 0 };
    IDispatchEx *dispex;
    IDispatch *disp;
    DISPID dispid;
    HRESULT hres;
    BSTR bstr;

    active_script = create_script();

    SET_EXPECT(OnEnterScript);
    SET_EXPECT(OnLeaveScript);
    SET_EXPECT(testGetCallerVBS);
    parse_script(active_script,
                 L"Sub testGetCallerFunc\nCall testGetCaller\nEnd Sub\n"
                 L"Call testGetCallerVBS(42)");
    CHECK_CALLED(testGetCallerVBS);
    CHECK_CALLED(OnLeaveScript);
    CHECK_CALLED(OnEnterScript);

    hres = IActiveScript_GetScriptDispatch(active_script, NULL, &disp);
    ok(hres == S_OK, "GetScriptDispatch failed: %08lx\n", hres);
    hres = IDispatch_QueryInterface(disp, &IID_IDispatchEx, (void**)&dispex);
    ok(hres == S_OK, "Could not get IDispatchEx: %08lx\n", hres);
    IDispatch_Release(disp);
    bstr = SysAllocString(L"testGetCallerFunc");
    hres = IDispatchEx_GetDispID(dispex, bstr, 0, &dispid);
    ok(hres == S_OK, "GetDispID failed: %08lx\n", hres);
    SysFreeString(bstr);

    SET_EXPECT(OnEnterScript);
    SET_EXPECT(OnLeaveScript);
    SET_EXPECT(testGetCaller);
    hres = IDispatchEx_InvokeEx(dispex, dispid, 0, DISPATCH_METHOD, &dp, NULL, NULL, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    CHECK_CALLED(testGetCaller);
    CHECK_CALLED(OnLeaveScript);
    test_get_caller_sp = &sp_caller_obj;
    SET_EXPECT(OnEnterScript);
    SET_EXPECT(OnLeaveScript);
    SET_EXPECT(testGetCaller);
    hres = IDispatchEx_InvokeEx(dispex, dispid, 0, DISPATCH_METHOD, &dp, NULL, NULL, test_get_caller_sp);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    CHECK_CALLED(testGetCaller);
    CHECK_CALLED(OnLeaveScript);
    IDispatchEx_Release(dispex);

    IActiveScript_Release(active_script);
    active_script = NULL;
}

START_TEST(caller)
{
    CoInitialize(NULL);

    run_scripts();

    CoUninitialize();
}
