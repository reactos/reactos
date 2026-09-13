/*
 * Copyright 2009,2011 Jacek Caban for CodeWeavers
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
#include <urlmon.h>
#include <mshtmhst.h>

#include "wine/test.h"

#ifdef _WIN64

#define IActiveScriptParse_QueryInterface IActiveScriptParse64_QueryInterface
#define IActiveScriptParse_Release IActiveScriptParse64_Release
#define IActiveScriptParse_InitNew IActiveScriptParse64_InitNew
#define IActiveScriptParse_ParseScriptText IActiveScriptParse64_ParseScriptText
#define IActiveScriptParseProcedure2_Release IActiveScriptParseProcedure2_64_Release

#else

#define IActiveScriptParse_QueryInterface IActiveScriptParse32_QueryInterface
#define IActiveScriptParse_Release IActiveScriptParse32_Release
#define IActiveScriptParse_InitNew IActiveScriptParse32_InitNew
#define IActiveScriptParse_ParseScriptText IActiveScriptParse32_ParseScriptText
#define IActiveScriptParseProcedure2_Release IActiveScriptParseProcedure2_32_Release

#endif

extern const CLSID CLSID_VBScript;

#define VB_E_ACCESS_DENIED      0x800a0046

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

DEFINE_EXPECT(CreateInstance);
DEFINE_EXPECT(ProcessUrlAction);
DEFINE_EXPECT(QueryCustomPolicy);
DEFINE_EXPECT(reportSuccess);
DEFINE_EXPECT(Host_QS_SecMgr);
DEFINE_EXPECT(Caller_QS_SecMgr);
DEFINE_EXPECT(QI_IObjectWithSite);
DEFINE_EXPECT(SetSite);

static HRESULT QS_SecMgr_hres;
static HRESULT ProcessUrlAction_hres;
static DWORD ProcessUrlAction_policy;
static HRESULT CreateInstance_hres;
static HRESULT QueryCustomPolicy_hres;
static DWORD QueryCustomPolicy_psize;
static DWORD QueryCustomPolicy_policy;
static HRESULT QI_IDispatch_hres;
static HRESULT QI_IObjectWithSite_hres;
static HRESULT SetSite_hres;

#define TESTOBJ_CLSID     "{178fc163-f585-4e24-9c13-4bb7faf80646}"
#define TESTOBJINST_CLSID "{178fc163-f585-4e24-9c13-4bb7faf80647}"

static const GUID CLSID_TestObj =
    {0x178fc163,0xf585,0x4e24,{0x9c,0x13,0x4b,0xb7,0xfa,0xf8,0x06,0x46}};
static const GUID CLSID_TestObjInst =
    {0x178fc163,0xf585,0x4e24,{0x9c,0x13,0x4b,0xb7,0xfa,0xf8,0x06,0x47}};

/* Defined as extern in urlmon.idl, but not exported by uuid.lib */
const GUID GUID_CUSTOM_CONFIRMOBJECTSAFETY =
    {0x10200490,0xfa38,0x11d0,{0xac,0x0e,0x00,0xa0,0xc9,0xf,0xff,0xc0}};

#define DISPID_TEST_REPORTSUCCESS    0x1000

#define DISPID_GLOBAL_OK             0x2000

#define VB_E_CANNOT_CREATE_OBJ 0x800a01ad

static HRESULT WINAPI ObjectWithSite_QueryInterface(IObjectWithSite *iface, REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static ULONG WINAPI ObjectWithSite_AddRef(IObjectWithSite *iface)
{
    return 2;
}

static ULONG WINAPI ObjectWithSite_Release(IObjectWithSite *iface)
{
    return 1;
}

static HRESULT WINAPI ObjectWithSite_SetSite(IObjectWithSite *iface, IUnknown *pUnkSite)
{
    IServiceProvider *sp;
    HRESULT hres;


    CHECK_EXPECT(SetSite);
    ok(pUnkSite != NULL, "pUnkSite == NULL\n");

    hres = IUnknown_QueryInterface(pUnkSite, &IID_IServiceProvider, (void**)&sp);
    ok(hres == S_OK, "Could not get IServiceProvider iface: %08lx\n", hres);
    IServiceProvider_Release(sp);

    return SetSite_hres;
}

static HRESULT WINAPI ObjectWithSite_GetSite(IObjectWithSite *iface, REFIID riid, void **ppvSite)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IObjectWithSiteVtbl ObjectWithSiteVtbl = {
    ObjectWithSite_QueryInterface,
    ObjectWithSite_AddRef,
    ObjectWithSite_Release,
    ObjectWithSite_SetSite,
    ObjectWithSite_GetSite
};

static IObjectWithSite ObjectWithSite = { &ObjectWithSiteVtbl };

static IObjectWithSite *object_with_site;

static HRESULT WINAPI DispatchEx_QueryInterface(IDispatchEx *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(riid, &IID_IUnknown)) {
       *ppv = iface;
    }else if(IsEqualGUID(riid, &IID_IDispatch) || IsEqualGUID(riid, &IID_IDispatchEx)) {
        if(FAILED(QI_IDispatch_hres))
            return QI_IDispatch_hres;
        *ppv = iface;
    }else if(IsEqualGUID(&IID_IObjectWithSite, riid)) {
        CHECK_EXPECT(QI_IObjectWithSite);
        if(FAILED(QI_IObjectWithSite_hres))
            return QI_IObjectWithSite_hres;
        *ppv = object_with_site;
    }else if(IsEqualGUID(&IID_IObjectSafety, riid)) {
        ok(0, "Unexpected IID_IObjectSafety query\n");
    }

    return *ppv ? S_OK : E_NOINTERFACE;
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
    if(!lstrcmpW(bstrName, L"reportSuccess")) {
        ok(grfdex == fdexNameCaseInsensitive, "grfdex = %lx\n", grfdex);
        *pid = DISPID_TEST_REPORTSUCCESS;
        return S_OK;
    }

    ok(0, "unexpected name %s\n", wine_dbgstr_w(bstrName));
    return E_NOTIMPL;
}

static HRESULT WINAPI Test_InvokeEx(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    switch(id) {
    case DISPID_TEST_REPORTSUCCESS:
        CHECK_EXPECT(reportSuccess);

        ok(wFlags == INVOKE_FUNC, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(pdp->cArgs == 0, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(!pvarRes, "pvarRes != NULL\n");
        ok(pei != NULL, "pei == NULL\n");
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

static HRESULT WINAPI Global_GetDispID(IDispatchEx *iface, BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    if(!lstrcmpW(bstrName, L"ok")) {
        ok(grfdex == fdexNameCaseSensitive, "grfdex = %lx\n", grfdex);
        *pid = DISPID_GLOBAL_OK;
        return S_OK;
    }

    ok(0, "unexpected name %s\n", wine_dbgstr_w(bstrName));
    return E_NOTIMPL;
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
        ok(pei != NULL, "pei == NULL\n");

        ok(V_VT(pdp->rgvarg) == VT_BSTR, "V_VT(psp->rgvargs) = %d\n", V_VT(pdp->rgvarg));
        ok(V_VT(pdp->rgvarg+1) == VT_BOOL, "V_VT(psp->rgvargs+1) = %d\n", V_VT(pdp->rgvarg));
        ok(V_BOOL(pdp->rgvarg+1), "%s\n", wine_dbgstr_w(V_BSTR(pdp->rgvarg)));
        break;

    default:
        ok(0, "unexpected call\n");
        return E_NOTIMPL;
    }

    return S_OK;
}

static IDispatchExVtbl globalObjVtbl = {
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

static IDispatchEx globalObj = { &globalObjVtbl };

static HRESULT WINAPI ClassFactory_QueryInterface(IClassFactory *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IClassFactory, riid)) {
        *ppv = iface;
        return S_OK;
    }

    /* TODO: IClassFactoryEx */
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI ClassFactory_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI ClassFactory_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI ClassFactory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **ppv)
{
    CHECK_EXPECT(CreateInstance);

    ok(!outer, "outer = %p\n", outer);
    ok(IsEqualGUID(&IID_IUnknown, riid), "unexpected riid %s\n", wine_dbgstr_guid(riid));

    if(SUCCEEDED(CreateInstance_hres))
        *ppv = &testObj;
    return CreateInstance_hres;
}

static HRESULT WINAPI ClassFactory_LockServer(IClassFactory *iface, BOOL dolock)
{
    ok(0, "unexpected call\n");
    return S_OK;
}

static const IClassFactoryVtbl ClassFactoryVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    ClassFactory_CreateInstance,
    ClassFactory_LockServer
};

static IClassFactory activex_cf = { &ClassFactoryVtbl };

static HRESULT WINAPI InternetHostSecurityManager_QueryInterface(IInternetHostSecurityManager *iface, REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI InternetHostSecurityManager_AddRef(IInternetHostSecurityManager *iface)
{
    return 2;
}

static ULONG WINAPI InternetHostSecurityManager_Release(IInternetHostSecurityManager *iface)
{
    return 1;
}

static HRESULT WINAPI InternetHostSecurityManager_GetSecurityId(IInternetHostSecurityManager *iface,  BYTE *pbSecurityId,
        DWORD *pcbSecurityId, DWORD_PTR dwReserved)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetHostSecurityManager_ProcessUrlAction(IInternetHostSecurityManager *iface, DWORD dwAction,
        BYTE *pPolicy, DWORD cbPolicy, BYTE *pContext, DWORD cbContext, DWORD dwFlags, DWORD dwReserved)
{
    CHECK_EXPECT(ProcessUrlAction);

    ok(dwAction == URLACTION_ACTIVEX_RUN, "dwAction = %lx\n", dwAction);
    ok(pPolicy != NULL, "pPolicy == NULL\n");
    ok(cbPolicy == sizeof(DWORD), "cbPolicy = %ld\n", cbPolicy);
    ok(pContext != NULL, "pContext == NULL\n");
    ok(cbContext == sizeof(GUID), "cbContext = %ld\n", cbContext);
    ok(IsEqualGUID(pContext, &CLSID_TestObj), "pContext = %s\n", wine_dbgstr_guid((const IID*)pContext));
    ok(!dwFlags, "dwFlags = %lx\n", dwFlags);
    ok(!dwReserved, "dwReserved = %lx\n", dwReserved);

    if(SUCCEEDED(ProcessUrlAction_hres))
        *(DWORD*)pPolicy = ProcessUrlAction_policy;
    return ProcessUrlAction_hres;
}

static HRESULT WINAPI InternetHostSecurityManager_QueryCustomPolicy(IInternetHostSecurityManager *iface, REFGUID guidKey,
        BYTE **ppPolicy, DWORD *pcbPolicy, BYTE *pContext, DWORD cbContext, DWORD dwReserved)
{
    const struct CONFIRMSAFETY *cs = (const struct CONFIRMSAFETY*)pContext;
    DWORD *ret;

    CHECK_EXPECT(QueryCustomPolicy);

    ok(IsEqualGUID(&GUID_CUSTOM_CONFIRMOBJECTSAFETY, guidKey), "guidKey = %s\n", wine_dbgstr_guid(guidKey));

    ok(ppPolicy != NULL, "ppPolicy == NULL\n");
    ok(pcbPolicy != NULL, "pcbPolicy == NULL\n");
    ok(pContext != NULL, "pContext == NULL\n");
    ok(cbContext == sizeof(struct CONFIRMSAFETY), "cbContext = %ld\n", cbContext);
    ok(!dwReserved, "dwReserved = %lx\n", dwReserved);

    /* TODO: CLSID */
    ok(cs->pUnk != NULL, "cs->pUnk == NULL\n");
    ok(!cs->dwFlags, "dwFlags = %lx\n", cs->dwFlags);

    if(FAILED(QueryCustomPolicy_hres))
        return QueryCustomPolicy_hres;

    ret = CoTaskMemAlloc(QueryCustomPolicy_psize);
    *ppPolicy = (BYTE*)ret;
    *pcbPolicy = QueryCustomPolicy_psize;
    memset(ret, 0, QueryCustomPolicy_psize);
    if(QueryCustomPolicy_psize >= sizeof(DWORD))
        *ret = QueryCustomPolicy_policy;

    return QueryCustomPolicy_hres;
}

static const IInternetHostSecurityManagerVtbl InternetHostSecurityManagerVtbl = {
    InternetHostSecurityManager_QueryInterface,
    InternetHostSecurityManager_AddRef,
    InternetHostSecurityManager_Release,
    InternetHostSecurityManager_GetSecurityId,
    InternetHostSecurityManager_ProcessUrlAction,
    InternetHostSecurityManager_QueryCustomPolicy
};

static IInternetHostSecurityManager InternetHostSecurityManager = { &InternetHostSecurityManagerVtbl };

static IServiceProvider ServiceProvider;

static HRESULT WINAPI ServiceProvider_QueryInterface(IServiceProvider *iface, REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI ServiceProvider_AddRef(IServiceProvider *iface)
{
    return 2;
}

static ULONG WINAPI ServiceProvider_Release(IServiceProvider *iface)
{
    return 1;
}

static HRESULT WINAPI ServiceProvider_QueryService(IServiceProvider *iface,
        REFGUID guidService, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&SID_GetCaller, guidService))
        return E_NOINTERFACE;

    if(IsEqualGUID(&SID_SInternetHostSecurityManager, guidService)) {
        if(iface == &ServiceProvider)
            CHECK_EXPECT(Host_QS_SecMgr);
        else
            CHECK_EXPECT(Caller_QS_SecMgr);
        ok(IsEqualGUID(&IID_IInternetHostSecurityManager, riid), "unexpected riid %s\n", wine_dbgstr_guid(riid));
        if(SUCCEEDED(QS_SecMgr_hres))
            *ppv = &InternetHostSecurityManager;
        return QS_SecMgr_hres;
    }

    ok(0, "unexpected service %s\n", wine_dbgstr_guid(guidService));
    return E_NOINTERFACE;
}

static IServiceProviderVtbl ServiceProviderVtbl = {
    ServiceProvider_QueryInterface,
    ServiceProvider_AddRef,
    ServiceProvider_Release,
    ServiceProvider_QueryService
};

static IServiceProvider ServiceProvider = { &ServiceProviderVtbl };

static HRESULT WINAPI ActiveScriptSite_QueryInterface(IActiveScriptSite *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = iface;
    }else if(IsEqualGUID(&IID_IActiveScriptSite, riid)) {
        *ppv = iface;
    }else if(IsEqualGUID(&IID_IServiceProvider, riid)) {
        *ppv = &ServiceProvider;
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
    ok(dwReturnMask == SCRIPTINFO_IUNKNOWN, "unexpected dwReturnMask %lx\n", dwReturnMask);
    ok(!ppti, "ppti != NULL\n");
    ok(!lstrcmpW(pstrName, L"test"), "pstrName = %s\n", wine_dbgstr_w(pstrName));

    *ppiunkItem = (IUnknown*)&globalObj;
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

static void set_safety_options(IUnknown *unk, BOOL use_sec_mgr)
{
    IObjectSafety *safety;
    DWORD supported, enabled, options_all, options_set;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IObjectSafety, (void**)&safety);
    ok(hres == S_OK, "Could not get IObjectSafety: %08lx\n", hres);
    if(FAILED(hres))
        return;

    options_all = INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER;
    if(use_sec_mgr)
        options_set = options_all;
    else
        options_set = INTERFACE_USES_DISPEX;

    hres = IObjectSafety_SetInterfaceSafetyOptions(safety, &IID_IActiveScriptParse, options_all, options_set);
    ok(hres == S_OK, "SetInterfaceSafetyOptions failed: %08lx\n", hres);

    supported = enabled = 0xdeadbeef;
    hres = IObjectSafety_GetInterfaceSafetyOptions(safety, &IID_IActiveScriptParse, &supported, &enabled);
    ok(hres == S_OK, "GetInterfaceSafetyOptions failed: %08lx\n", hres);
    ok(supported == options_all, "supported=%lx, expected %lx\n", supported, options_all);
    ok(enabled == options_set, "enabled=%lx, expected %lx\n", enabled, options_set);

    IObjectSafety_Release(safety);
}

#define parse_script(p,s) _parse_script(__LINE__,p,s)
static void _parse_script(unsigned line, IActiveScriptParse *parser, const WCHAR *script)
{
    BSTR str;
    HRESULT hres;

    str = SysAllocString(script);
    hres = IActiveScriptParse_ParseScriptText(parser, str, NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
    SysFreeString(str);
    ok_(__FILE__,line)(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
}

static HRESULT parse_script_e(IActiveScriptParse *parser, const WCHAR *script)
{
    BSTR str;
    HRESULT hres;

    str = SysAllocString(script);
    hres = IActiveScriptParse_ParseScriptText(parser, str, NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
    SysFreeString(str);

    return hres;
}

static IActiveScriptParse *create_script(BOOL use_sec_mgr)
{
    IActiveScriptParse *parser;
    IActiveScript *script;
    HRESULT hres;

    QS_SecMgr_hres = S_OK;
    ProcessUrlAction_hres = S_OK;
    ProcessUrlAction_policy = URLPOLICY_ALLOW;
    CreateInstance_hres = S_OK;
    QueryCustomPolicy_hres = S_OK;
    QueryCustomPolicy_psize = sizeof(DWORD);
    QueryCustomPolicy_policy = URLPOLICY_ALLOW;
    QI_IDispatch_hres = S_OK;
    QI_IObjectWithSite_hres = S_OK;
    SetSite_hres = S_OK;

    hres = CoCreateInstance(&CLSID_VBScript, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IActiveScript, (void**)&script);
    ok(hres == S_OK, "CoCreateInstance failed: %08lx\n", hres);
    if(FAILED(hres))
        return NULL;

    set_safety_options((IUnknown*)script, use_sec_mgr);

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

static void test_CreateObject(void)
{
    IActiveScriptParse *parser;
    HRESULT hres;

    parser = create_script(TRUE);

    SET_EXPECT(Host_QS_SecMgr);
    SET_EXPECT(ProcessUrlAction);
    SET_EXPECT(CreateInstance);
    SET_EXPECT(QueryCustomPolicy);
    SET_EXPECT(QI_IObjectWithSite);
    SET_EXPECT(reportSuccess);
    parse_script(parser, L"Call CreateObject(\"Wine.Test\").reportSuccess()");
    CHECK_CALLED(Host_QS_SecMgr);
    CHECK_CALLED(ProcessUrlAction);
    CHECK_CALLED(CreateInstance);
    CHECK_CALLED(QueryCustomPolicy);
    CHECK_CALLED(QI_IObjectWithSite);
    CHECK_CALLED(reportSuccess);

    IActiveScriptParse_Release(parser);

    parser = create_script(TRUE);

    hres = parse_script_e(parser, L"Call CreateObject(\"Wine.TestABC\")");
    ok(hres == VB_E_CANNOT_CREATE_OBJ, "hres = %08lx\n", hres);

    IActiveScriptParse_Release(parser);

    parser = create_script(TRUE);
    QS_SecMgr_hres = E_NOINTERFACE;

    SET_EXPECT(Host_QS_SecMgr);
    hres = parse_script_e(parser, L"Call CreateObject(\"Wine.Test\")");
    ok(hres == VB_E_CANNOT_CREATE_OBJ, "hres = %08lx\n", hres);
    CHECK_CALLED(Host_QS_SecMgr);

    IActiveScriptParse_Release(parser);

    parser = create_script(TRUE);
    ProcessUrlAction_hres = E_FAIL;

    SET_EXPECT(Host_QS_SecMgr);
    SET_EXPECT(ProcessUrlAction);
    hres = parse_script_e(parser, L"Call CreateObject(\"Wine.Test\")");
    ok(hres == VB_E_CANNOT_CREATE_OBJ, "hres = %08lx\n", hres);
    CHECK_CALLED(Host_QS_SecMgr);
    CHECK_CALLED(ProcessUrlAction);

    IActiveScriptParse_Release(parser);

    parser = create_script(TRUE);
    ProcessUrlAction_policy = URLPOLICY_DISALLOW;

    SET_EXPECT(Host_QS_SecMgr);
    SET_EXPECT(ProcessUrlAction);
    hres = parse_script_e(parser, L"Call CreateObject(\"Wine.Test\")");
    ok(hres == VB_E_CANNOT_CREATE_OBJ, "hres = %08lx\n", hres);
    CHECK_CALLED(Host_QS_SecMgr);
    CHECK_CALLED(ProcessUrlAction);

    IActiveScriptParse_Release(parser);

    parser = create_script(TRUE);
    CreateInstance_hres = E_FAIL;

    SET_EXPECT(Host_QS_SecMgr);
    SET_EXPECT(ProcessUrlAction);
    SET_EXPECT(CreateInstance);
    hres = parse_script_e(parser, L"Call CreateObject(\"Wine.Test\")");
    ok(hres == VB_E_CANNOT_CREATE_OBJ, "hres = %08lx\n", hres);
    CHECK_CALLED(Host_QS_SecMgr);
    CHECK_CALLED(ProcessUrlAction);
    CHECK_CALLED(CreateInstance);

    IActiveScriptParse_Release(parser);

    parser = create_script(TRUE);
    QueryCustomPolicy_hres = E_FAIL;

    SET_EXPECT(Host_QS_SecMgr);
    SET_EXPECT(ProcessUrlAction);
    SET_EXPECT(CreateInstance);
    SET_EXPECT(QueryCustomPolicy);
    hres = parse_script_e(parser, L"Call CreateObject(\"Wine.Test\")");
    ok(hres == VB_E_CANNOT_CREATE_OBJ, "hres = %08lx\n", hres);
    CHECK_CALLED(Host_QS_SecMgr);
    CHECK_CALLED(ProcessUrlAction);
    CHECK_CALLED(CreateInstance);
    CHECK_CALLED(QueryCustomPolicy);

    IActiveScriptParse_Release(parser);

    parser = create_script(TRUE);
    QueryCustomPolicy_psize = 6;

    SET_EXPECT(Host_QS_SecMgr);
    SET_EXPECT(ProcessUrlAction);
    SET_EXPECT(CreateInstance);
    SET_EXPECT(QueryCustomPolicy);
    SET_EXPECT(QI_IObjectWithSite);
    SET_EXPECT(reportSuccess);
    parse_script(parser, L"Call CreateObject(\"Wine.Test\").reportSuccess()");
    CHECK_CALLED(Host_QS_SecMgr);
    CHECK_CALLED(ProcessUrlAction);
    CHECK_CALLED(CreateInstance);
    CHECK_CALLED(QueryCustomPolicy);
    CHECK_CALLED(QI_IObjectWithSite);
    CHECK_CALLED(reportSuccess);

    IActiveScriptParse_Release(parser);

    parser = create_script(TRUE);
    QueryCustomPolicy_policy = URLPOLICY_DISALLOW;

    SET_EXPECT(Host_QS_SecMgr);
    SET_EXPECT(ProcessUrlAction);
    SET_EXPECT(CreateInstance);
    SET_EXPECT(QueryCustomPolicy);
    hres = parse_script_e(parser, L"Call CreateObject(\"Wine.Test\")");
    ok(hres == VB_E_CANNOT_CREATE_OBJ, "hres = %08lx\n", hres);
    CHECK_CALLED(Host_QS_SecMgr);
    CHECK_CALLED(ProcessUrlAction);
    CHECK_CALLED(CreateInstance);
    CHECK_CALLED(QueryCustomPolicy);

    QueryCustomPolicy_psize = 6;

    SET_EXPECT(ProcessUrlAction);
    SET_EXPECT(CreateInstance);
    SET_EXPECT(QueryCustomPolicy);
    hres = parse_script_e(parser, L"Call CreateObject(\"Wine.Test\")");
    ok(hres == VB_E_CANNOT_CREATE_OBJ, "hres = %08lx\n", hres);
    CHECK_CALLED(ProcessUrlAction);
    CHECK_CALLED(CreateInstance);
    CHECK_CALLED(QueryCustomPolicy);

    QueryCustomPolicy_policy = URLPOLICY_ALLOW;
    QueryCustomPolicy_psize = 3;

    SET_EXPECT(ProcessUrlAction);
    SET_EXPECT(CreateInstance);
    SET_EXPECT(QueryCustomPolicy);
    hres = parse_script_e(parser, L"Call CreateObject(\"Wine.Test\")");
    ok(hres == VB_E_CANNOT_CREATE_OBJ, "hres = %08lx\n", hres);
    CHECK_CALLED(ProcessUrlAction);
    CHECK_CALLED(CreateInstance);
    CHECK_CALLED(QueryCustomPolicy);

    IActiveScriptParse_Release(parser);

    parser = create_script(FALSE);

    SET_EXPECT(CreateInstance);
    SET_EXPECT(QI_IObjectWithSite);
    SET_EXPECT(reportSuccess);
    parse_script(parser, L"Call CreateObject(\"Wine.Test\").reportSuccess()");
    CHECK_CALLED(CreateInstance);
    CHECK_CALLED(QI_IObjectWithSite);
    CHECK_CALLED(reportSuccess);

    IActiveScriptParse_Release(parser);

    parser = create_script(TRUE);
    object_with_site = &ObjectWithSite;

    SET_EXPECT(Host_QS_SecMgr);
    SET_EXPECT(ProcessUrlAction);
    SET_EXPECT(CreateInstance);
    SET_EXPECT(QueryCustomPolicy);
    SET_EXPECT(QI_IObjectWithSite);
    SET_EXPECT(SetSite);
    SET_EXPECT(reportSuccess);
    parse_script(parser, L"Call CreateObject(\"Wine.Test\").reportSuccess()");
    CHECK_CALLED(Host_QS_SecMgr);
    CHECK_CALLED(ProcessUrlAction);
    CHECK_CALLED(CreateInstance);
    CHECK_CALLED(QueryCustomPolicy);
    CHECK_CALLED(QI_IObjectWithSite);
    CHECK_CALLED(SetSite);
    CHECK_CALLED(reportSuccess);

    SetSite_hres = E_FAIL;
    SET_EXPECT(ProcessUrlAction);
    SET_EXPECT(CreateInstance);
    SET_EXPECT(QueryCustomPolicy);
    SET_EXPECT(QI_IObjectWithSite);
    SET_EXPECT(SetSite);
    hres = parse_script_e(parser, L"Call CreateObject(\"Wine.Test\")");
    ok(hres == VB_E_CANNOT_CREATE_OBJ, "hres = %08lx\n", hres);
    CHECK_CALLED(ProcessUrlAction);
    CHECK_CALLED(CreateInstance);
    CHECK_CALLED(QueryCustomPolicy);
    CHECK_CALLED(QI_IObjectWithSite);
    CHECK_CALLED(SetSite);

    IActiveScriptParse_Release(parser);
}

static void test_GetObject(void)
{
    IActiveScriptParse *parser;
    HRESULT hres;

    /* Never allowed with security manager */
    parser = create_script(TRUE);
    hres = parse_script_e(parser, L"Call GetObject(\"clsid:" TESTOBJINST_CLSID "\").reportSuccess()");
    ok(hres == VB_E_CANNOT_CREATE_OBJ, "hres = %08lx\n", hres);
    IActiveScriptParse_Release(parser);

    parser = create_script(FALSE);

    SET_EXPECT(QI_IObjectWithSite);
    SET_EXPECT(SetSite);
    SET_EXPECT(reportSuccess);
    hres = parse_script_e(parser, L"Call GetObject(\"clsid:" TESTOBJINST_CLSID "\").reportSuccess()");
    if(broken(hres == VB_E_ACCESS_DENIED)) /* 64-bit win8 fails on the first try */
        hres = parse_script_e(parser, L"Call GetObject(\"clsid:" TESTOBJINST_CLSID "\").reportSuccess()");
    if(hres == HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND)) { /* Workaround for broken win2k */
        win_skip("got unexpected error %08lx\n", hres);
        CLEAR_CALLED(QI_IObjectWithSite);
        CLEAR_CALLED(SetSite);
        CLEAR_CALLED(reportSuccess);
        IActiveScriptParse_Release(parser);
        return;
    }
    ok(hres == S_OK, "hres = %08lx\n", hres);
    CHECK_CALLED(QI_IObjectWithSite);
    CHECK_CALLED(SetSite);
    CHECK_CALLED(reportSuccess);

    SetSite_hres = E_FAIL;
    SET_EXPECT(QI_IObjectWithSite);
    SET_EXPECT(SetSite);
    hres = parse_script_e(parser, L"Call GetObject(\"clsid:" TESTOBJINST_CLSID "\").reportSuccess()");
    ok(hres == E_FAIL, "hres = %08lx\n", hres);
    CHECK_CALLED(QI_IObjectWithSite);
    CHECK_CALLED(SetSite);

    QI_IObjectWithSite_hres = E_NOINTERFACE;
    SET_EXPECT(QI_IObjectWithSite);
    SET_EXPECT(reportSuccess);
    parse_script(parser, L"Call GetObject(\"clsid:" TESTOBJINST_CLSID "\").reportSuccess()");
    CHECK_CALLED(QI_IObjectWithSite);
    CHECK_CALLED(reportSuccess);

    IActiveScriptParse_Release(parser);

    /* Invalid moniker */
    parser = create_script(FALSE);
    hres = parse_script_e(parser, L"Call GetObject(\"nonexistent:test\").reportSuccess()");
    ok(hres == MK_E_SYNTAX, "hres = %08lx\n", hres);
    IActiveScriptParse_Release(parser);
}

static BOOL init_key(const char *key_name, const char *def_value, BOOL init)
{
    HKEY hkey;
    DWORD res;

    if(!init) {
        RegDeleteKeyA(HKEY_CLASSES_ROOT, key_name);
        return TRUE;
    }

    res = RegCreateKeyA(HKEY_CLASSES_ROOT, key_name, &hkey);
    if(res != ERROR_SUCCESS)
        return FALSE;

    if(def_value)
        res = RegSetValueA(hkey, NULL, REG_SZ, def_value, strlen(def_value));

    RegCloseKey(hkey);

    return res == ERROR_SUCCESS;
}

static BOOL init_registry(BOOL init)
{
    return init_key("Wine.Test\\CLSID", TESTOBJ_CLSID, init);
}

static BOOL register_activex(void)
{
    DWORD regid;
    HRESULT hres;

    if(!init_registry(TRUE)) {
        init_registry(FALSE);
        return FALSE;
    }

    hres = CoRegisterClassObject(&CLSID_TestObj, (IUnknown *)&activex_cf,
                                 CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE, &regid);
    ok(hres == S_OK, "Could not register script engine: %08lx\n", hres);

    hres = CoRegisterClassObject(&CLSID_TestObjInst, (IUnknown *)&testObj,
                                 CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE, &regid);
    ok(hres == S_OK, "Could not register script engine: %08lx\n", hres);

    return TRUE;
}

static BOOL check_vbscript(void)
{
    IActiveScriptParseProcedure2 *vbscript;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_VBScript, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IActiveScriptParseProcedure2, (void**)&vbscript);
    if(SUCCEEDED(hres))
        IActiveScriptParseProcedure2_Release(vbscript);

    return hres == S_OK;
}


START_TEST(createobj)
{
    CoInitialize(NULL);

    if(check_vbscript()) {
        if(register_activex()) {
            test_CreateObject();
            test_GetObject();
            init_registry(FALSE);
        }else {
            skip("Could not register ActiveX object.\n");
        }
    }else {
        win_skip("Broken engine, probably too old\n");
    }

    CoUninitialize();
}
