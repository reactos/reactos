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

#define COBJMACROS
#define CONST_VTABLE

#include <wine/test.h>
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "dispex.h"
#include "mshtml.h"
#include "initguid.h"
#include "activscp.h"
#include "activdbg.h"
#include "objsafe.h"
#include "mshtmdid.h"

DEFINE_GUID(CLSID_IdentityUnmarshal,0x0000001b,0x0000,0x0000,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    do { called_ ## func = FALSE; expect_ ## func = TRUE; } while(0)

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

#define CHECK_NOT_CALLED(func) \
    do { \
        ok(!called_ ## func, "unexpected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

#define CLEAR_CALLED(func) \
    expect_ ## func = called_ ## func = FALSE


DEFINE_EXPECT(CreateInstance);
DEFINE_EXPECT(GetInterfaceSafetyOptions);
DEFINE_EXPECT(SetInterfaceSafetyOptions);
DEFINE_EXPECT(InitNew);
DEFINE_EXPECT(Close);
DEFINE_EXPECT(SetProperty);
DEFINE_EXPECT(SetScriptSite);
DEFINE_EXPECT(GetScriptState);
DEFINE_EXPECT(SetScriptState_STARTED);
DEFINE_EXPECT(SetScriptState_CONNECTED);
DEFINE_EXPECT(SetScriptState_DISCONNECTED);
DEFINE_EXPECT(AddNamedItem);
DEFINE_EXPECT(ParseScriptText);
DEFINE_EXPECT(GetScriptDispatch);
DEFINE_EXPECT(funcDisp);

#define TESTSCRIPT_CLSID "{178fc163-f585-4e24-9c13-4bb7faf80746}"

static const GUID CLSID_TestScript =
    {0x178fc163,0xf585,0x4e24,{0x9c,0x13,0x4b,0xb7,0xfa,0xf8,0x07,0x46}};

static IHTMLDocument2 *notif_doc;
static IDispatchEx *window_dispex;
static BOOL doc_complete;

static const char *debugstr_w(LPCWSTR str)
{
    static char buf[1024];
    WideCharToMultiByte(CP_ACP, 0, str, -1, buf, sizeof(buf), NULL, NULL);
    return buf;
}

static const char *debugstr_guid(REFIID riid)
{
    static char buf[50];

    sprintf(buf, "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
            riid->Data1, riid->Data2, riid->Data3, riid->Data4[0],
            riid->Data4[1], riid->Data4[2], riid->Data4[3], riid->Data4[4],
            riid->Data4[5], riid->Data4[6], riid->Data4[7]);

    return buf;
}

static HRESULT WINAPI PropertyNotifySink_QueryInterface(IPropertyNotifySink *iface,
        REFIID riid, void**ppv)
{
    if(IsEqualGUID(&IID_IPropertyNotifySink, riid)) {
        *ppv = iface;
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI PropertyNotifySink_AddRef(IPropertyNotifySink *iface)
{
    return 2;
}

static ULONG WINAPI PropertyNotifySink_Release(IPropertyNotifySink *iface)
{
    return 1;
}

static HRESULT WINAPI PropertyNotifySink_OnChanged(IPropertyNotifySink *iface, DISPID dispID)
{
    if(dispID == DISPID_READYSTATE){
        BSTR state;
        HRESULT hres;

        static const WCHAR completeW[] = {'c','o','m','p','l','e','t','e',0};

        hres = IHTMLDocument2_get_readyState(notif_doc, &state);
        ok(hres == S_OK, "get_readyState failed: %08x\n", hres);

        if(!lstrcmpW(state, completeW))
            doc_complete = TRUE;

        SysFreeString(state);
    }

    return S_OK;
}

static HRESULT WINAPI PropertyNotifySink_OnRequestEdit(IPropertyNotifySink *iface, DISPID dispID)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static IPropertyNotifySinkVtbl PropertyNotifySinkVtbl = {
    PropertyNotifySink_QueryInterface,
    PropertyNotifySink_AddRef,
    PropertyNotifySink_Release,
    PropertyNotifySink_OnChanged,
    PropertyNotifySink_OnRequestEdit
};

static IPropertyNotifySink PropertyNotifySink = { &PropertyNotifySinkVtbl };

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
    ok(0, "unexpected call %s %x\n", debugstr_w(bstrName), grfdex);
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

static HRESULT WINAPI DispatchEx_GetDispID(IDispatchEx *iface, BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI funcDisp_InvokeEx(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    CHECK_EXPECT(funcDisp);

    ok(id == DISPID_VALUE, "id = %d\n", id);
    ok(lcid == 0, "lcid = %x\n", lcid);
    ok(wFlags == DISPATCH_METHOD, "wFlags = %x\n", wFlags);
    ok(pdp != NULL, "pdp == NULL\n");
    ok(pdp->cArgs == 2, "pdp->cArgs = %d\n", pdp->cArgs);
    ok(pdp->cNamedArgs == 1, "pdp->cNamedArgs = %d\n", pdp->cNamedArgs);
    ok(pdp->rgdispidNamedArgs[0] == DISPID_THIS, "pdp->rgdispidNamedArgs[0] = %d\n", pdp->rgdispidNamedArgs[0]);
    ok(V_VT(pdp->rgvarg) == VT_DISPATCH, "V_VT(rgvarg) = %d\n", V_VT(pdp->rgvarg));
    ok(V_VT(pdp->rgvarg+1) == VT_BOOL, "V_VT(rgvarg[1]) = %d\n", V_VT(pdp->rgvarg));
    ok(V_BOOL(pdp->rgvarg+1) == VARIANT_TRUE, "V_BOOL(rgvarg[1]) = %x\n", V_BOOL(pdp->rgvarg));
    ok(pvarRes != NULL, "pvarRes == NULL\n");
    ok(pei != NULL, "pei == NULL\n");
    ok(!pspCaller, "pspCaller != NULL\n");

    V_VT(pvarRes) = VT_I4;
    V_I4(pvarRes) = 100;
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
    DispatchEx_GetDispID,
    funcDisp_InvokeEx,
    DispatchEx_DeleteMemberByName,
    DispatchEx_DeleteMemberByDispID,
    DispatchEx_GetMemberProperties,
    DispatchEx_GetMemberName,
    DispatchEx_GetNextDispID,
    DispatchEx_GetNameSpaceParent
};

static IDispatchEx funcDisp = { &testObjVtbl };

static IHTMLDocument2 *create_document(void)
{
    IHTMLDocument2 *doc;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_HTMLDocument, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IHTMLDocument2, (void**)&doc);
    ok(hres == S_OK, "CoCreateInstance failed: %08x\n", hres);

    return doc;
}

static IHTMLDocument2 *create_doc_with_string(const char *str)
{
    IPersistStreamInit *init;
    IStream *stream;
    IHTMLDocument2 *doc;
    HGLOBAL mem;
    SIZE_T len;

    notif_doc = doc = create_document();
    if(!doc)
        return NULL;

    doc_complete = FALSE;
    len = strlen(str);
    mem = GlobalAlloc(0, len);
    memcpy(mem, str, len);
    CreateStreamOnHGlobal(mem, TRUE, &stream);

    IHTMLDocument2_QueryInterface(doc, &IID_IPersistStreamInit, (void**)&init);

    IPersistStreamInit_Load(init, stream);
    IPersistStreamInit_Release(init);
    IStream_Release(stream);

    return doc;
}

static void do_advise(IUnknown *unk, REFIID riid, IUnknown *unk_advise)
{
    IConnectionPointContainer *container;
    IConnectionPoint *cp;
    DWORD cookie;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IConnectionPointContainer, (void**)&container);
    ok(hres == S_OK, "QueryInterface(IID_IConnectionPointContainer) failed: %08x\n", hres);

    hres = IConnectionPointContainer_FindConnectionPoint(container, riid, &cp);
    IConnectionPointContainer_Release(container);
    ok(hres == S_OK, "FindConnectionPoint failed: %08x\n", hres);

    hres = IConnectionPoint_Advise(cp, unk_advise, &cookie);
    IConnectionPoint_Release(cp);
    ok(hres == S_OK, "Advise failed: %08x\n", hres);
}

typedef void (*domtest_t)(IHTMLDocument2*);

static IHTMLDocument2 *create_and_load_doc(const char *str)
{
    IHTMLDocument2 *doc;
    IHTMLElement *body = NULL;
    ULONG ref;
    MSG msg;
    HRESULT hres;
    static const WCHAR ucPtr[] = {'b','a','c','k','g','r','o','u','n','d',0};
    DISPID dispID = -1;
    OLECHAR *name;


    doc = create_doc_with_string(str);
    do_advise((IUnknown*)doc, &IID_IPropertyNotifySink, (IUnknown*)&PropertyNotifySink);

    while(!doc_complete && GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    hres = IHTMLDocument2_get_body(doc, &body);
    ok(hres == S_OK, "get_body failed: %08x\n", hres);

    if(!body) {
        skip("Could not get document body. Assuming no Gecko installed.\n");
        ref = IHTMLDocument2_Release(doc);
        ok(!ref, "ref = %d\n", ref);
        return NULL;
    }

    /* Check we can query for function on the IHTMLElementBody interface */
    name = (WCHAR*)ucPtr;
    hres = IHTMLElement_GetIDsOfNames(body, &IID_NULL, &name, 1, LOCALE_USER_DEFAULT, &dispID);
    ok(hres == S_OK, "GetIDsOfNames(background) failed %08x\n", hres);
    ok(dispID == DISPID_IHTMLBODYELEMENT_BACKGROUND, "Incorrect dispID got (%d)\n", dispID);

    IHTMLElement_Release(body);
    return doc;
}

static IActiveScriptSite *site;
static SCRIPTSTATE state;

static HRESULT WINAPI ObjectSafety_QueryInterface(IObjectSafety *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;
    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI ObjectSafety_AddRef(IObjectSafety *iface)
{
    return 2;
}

static ULONG WINAPI ObjectSafety_Release(IObjectSafety *iface)
{
    return 1;
}

static HRESULT WINAPI ObjectSafety_GetInterfaceSafetyOptions(IObjectSafety *iface, REFIID riid,
        DWORD *pdwSupportedOptions, DWORD *pdwEnabledOptions)
{
    CHECK_EXPECT(GetInterfaceSafetyOptions);

    ok(IsEqualGUID(&IID_IActiveScriptParse, riid), "unexpected riid %s\n", debugstr_guid(riid));
    ok(pdwSupportedOptions != NULL, "pdwSupportedOptions == NULL\n");
    ok(pdwEnabledOptions != NULL, "pdwEnabledOptions == NULL\n");

    *pdwSupportedOptions = INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER;
    *pdwEnabledOptions = INTERFACE_USES_DISPEX;

    return S_OK;
}

static HRESULT WINAPI ObjectSafety_SetInterfaceSafetyOptions(IObjectSafety *iface, REFIID riid,
        DWORD dwOptionSetMask, DWORD dwEnabledOptions)
{
    CHECK_EXPECT(SetInterfaceSafetyOptions);

    ok(IsEqualGUID(&IID_IActiveScriptParse, riid), "unexpected riid %s\n", debugstr_guid(riid));

    ok(dwOptionSetMask == (INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER),
       "dwOptionSetMask=%x\n", dwOptionSetMask);
    ok(dwEnabledOptions == (INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER),
       "dwEnabledOptions=%x\n", dwOptionSetMask);

    return S_OK;
}

static const IObjectSafetyVtbl ObjectSafetyVtbl = {
    ObjectSafety_QueryInterface,
    ObjectSafety_AddRef,
    ObjectSafety_Release,
    ObjectSafety_GetInterfaceSafetyOptions,
    ObjectSafety_SetInterfaceSafetyOptions
};

static IObjectSafety ObjectSafety = { &ObjectSafetyVtbl };

static HRESULT WINAPI ActiveScriptProperty_QueryInterface(IActiveScriptProperty *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;
    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI ActiveScriptProperty_AddRef(IActiveScriptProperty *iface)
{
    return 2;
}

static ULONG WINAPI ActiveScriptProperty_Release(IActiveScriptProperty *iface)
{
    return 1;
}

static HRESULT WINAPI ActiveScriptProperty_GetProperty(IActiveScriptProperty *iface, DWORD dwProperty,
        VARIANT *pvarIndex, VARIANT *pvarValue)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptProperty_SetProperty(IActiveScriptProperty *iface, DWORD dwProperty,
        VARIANT *pvarIndex, VARIANT *pvarValue)
{
    CHECK_EXPECT(SetProperty);

    ok(dwProperty == SCRIPTPROP_HACK_TRIDENTEVENTSINK, "unexpected property %d\n", dwProperty);
    ok(!pvarIndex, "pvarIndex != NULL\n");
    ok(pvarValue != NULL, "pvarValue == NULL\n");
    ok(V_VT(pvarValue) == VT_BOOL, "V_VT(pvarValue)=%d\n", V_VT(pvarValue));
    ok(V_BOOL(pvarValue) == VARIANT_TRUE, "V_BOOL(pvarValue)=%x\n", V_BOOL(pvarValue));

    return E_NOTIMPL;
}

static const IActiveScriptPropertyVtbl ActiveScriptPropertyVtbl = {
    ActiveScriptProperty_QueryInterface,
    ActiveScriptProperty_AddRef,
    ActiveScriptProperty_Release,
    ActiveScriptProperty_GetProperty,
    ActiveScriptProperty_SetProperty
};

static IActiveScriptProperty ActiveScriptProperty = { &ActiveScriptPropertyVtbl };

static HRESULT WINAPI ActiveScriptParseProcedure_QueryInterface(IActiveScriptParseProcedure2 *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;
    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI ActiveScriptParseProcedure_AddRef(IActiveScriptParseProcedure2 *iface)
{
    return 2;
}

static ULONG WINAPI ActiveScriptParseProcedure_Release(IActiveScriptParseProcedure2 *iface)
{
    return 1;
}

static HRESULT WINAPI ActiveScriptParseProcedure_ParseProcedureText(IActiveScriptParseProcedure2 *iface,
        LPCOLESTR pstrCode, LPCOLESTR pstrFormalParams, LPCOLESTR pstrProcedureName,
        LPCOLESTR pstrItemName, IUnknown *punkContext, LPCOLESTR pstrDelimiter,
        DWORD dwSourceContextCookie, ULONG ulStartingLineNumber, DWORD dwFlags, IDispatch **ppdisp)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IActiveScriptParseProcedure2Vtbl ActiveScriptParseProcedureVtbl = {
    ActiveScriptParseProcedure_QueryInterface,
    ActiveScriptParseProcedure_AddRef,
    ActiveScriptParseProcedure_Release,
    ActiveScriptParseProcedure_ParseProcedureText
};

static IActiveScriptParseProcedure2 ActiveScriptParseProcedure = { &ActiveScriptParseProcedureVtbl };

static HRESULT WINAPI ActiveScriptParse_QueryInterface(IActiveScriptParse *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;
    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI ActiveScriptParse_AddRef(IActiveScriptParse *iface)
{
    return 2;
}

static ULONG WINAPI ActiveScriptParse_Release(IActiveScriptParse *iface)
{
    return 1;
}

static HRESULT WINAPI ActiveScriptParse_InitNew(IActiveScriptParse *iface)
{
    CHECK_EXPECT(InitNew);
    return S_OK;
}

static HRESULT WINAPI ActiveScriptParse_AddScriptlet(IActiveScriptParse *iface,
        LPCOLESTR pstrDefaultName, LPCOLESTR pstrCode, LPCOLESTR pstrItemName,
        LPCOLESTR pstrSubItemName, LPCOLESTR pstrEventName, LPCOLESTR pstrDelimiter,
        DWORD dwSourceContextCookie, ULONG ulStartingLineNumber, DWORD dwFlags,
        BSTR *pbstrName, EXCEPINFO *pexcepinfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptParse_ParseScriptText(IActiveScriptParse *iface,
        LPCOLESTR pstrCode, LPCOLESTR pstrItemName, IUnknown *punkContext,
        LPCOLESTR pstrDelimiter, DWORD dwSourceContextCookie, ULONG ulStartingLine,
        DWORD dwFlags, VARIANT *pvarResult, EXCEPINFO *pexcepinfo)
{
    IDispatchEx *document;
    IUnknown *unk;
    VARIANT var, arg;
    DISPPARAMS dp;
    EXCEPINFO ei;
    DISPID id, named_arg = DISPID_PROPERTYPUT;
    BSTR tmp;
    HRESULT hres;

    static const WCHAR documentW[] = {'d','o','c','u','m','e','n','t',0};
    static const WCHAR testW[] = {'t','e','s','t',0};
    static const WCHAR funcW[] = {'f','u','n','c',0};

    CHECK_EXPECT(ParseScriptText);

    SET_EXPECT(GetScriptDispatch);

    tmp = SysAllocString(documentW);
    hres = IDispatchEx_GetDispID(window_dispex, tmp, fdexNameCaseSensitive, &id);
    SysFreeString(tmp);
    ok(hres == S_OK, "GetDispID(document) failed: %08x\n", hres);
    ok(id == DISPID_IHTMLWINDOW2_DOCUMENT, "id=%x\n", id);

    todo_wine CHECK_CALLED(GetScriptDispatch);

    VariantInit(&var);
    memset(&dp, 0, sizeof(dp));
    memset(&ei, 0, sizeof(ei));

    hres = IDispatchEx_InvokeEx(window_dispex, id, LOCALE_NEUTRAL, INVOKE_PROPERTYGET, &dp, &var, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08x\n", hres);
    ok(V_VT(&var) == VT_DISPATCH, "V_VT(var)=%d\n", V_VT(&var));
    ok(V_DISPATCH(&var) != NULL, "V_DISPATCH(&var) == NULL\n");

    hres = IDispatch_QueryInterface(V_DISPATCH(&var), &IID_IDispatchEx, (void**)&document);
    VariantClear(&var);
    ok(hres == S_OK, "Could not get DispatchEx: %08x\n", hres);

    tmp = SysAllocString(testW);
    hres = IDispatchEx_GetDispID(document, tmp, fdexNameCaseSensitive, &id);
    ok(hres == DISP_E_UNKNOWNNAME, "GetDispID(document) failed: %08x, expected DISP_E_UNKNOWNNAME\n", hres);
    hres = IDispatchEx_GetDispID(document, tmp, fdexNameCaseSensitive | fdexNameImplicit, &id);
    ok(hres == DISP_E_UNKNOWNNAME, "GetDispID(document) failed: %08x, expected DISP_E_UNKNOWNNAME\n", hres);
    SysFreeString(tmp);

    id = 0;
    tmp = SysAllocString(testW);
    hres = IDispatchEx_GetDispID(document, tmp, fdexNameCaseSensitive|fdexNameEnsure, &id);
    SysFreeString(tmp);
    ok(hres == S_OK, "GetDispID(document) failed: %08x\n", hres);
    ok(id, "id == 0\n");

    dp.cArgs = 1;
    dp.rgvarg = &var;
    dp.cNamedArgs = 1;
    dp.rgdispidNamedArgs = &named_arg;
    V_VT(&var) = VT_I4;
    V_I4(&var) = 100;

    hres = IDispatchEx_InvokeEx(document, id, LOCALE_NEUTRAL, INVOKE_PROPERTYPUT, &dp, NULL, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08x\n", hres);

    tmp = SysAllocString(testW);
    hres = IDispatchEx_GetDispID(document, tmp, fdexNameCaseSensitive, &id);
    SysFreeString(tmp);
    ok(hres == S_OK, "GetDispID(document) failed: %08x\n", hres);

    VariantInit(&var);
    memset(&dp, 0, sizeof(dp));
    memset(&ei, 0, sizeof(ei));
    hres = IDispatchEx_InvokeEx(document, id, LOCALE_NEUTRAL, INVOKE_PROPERTYGET, &dp, &var, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08x\n", hres);
    ok(V_VT(&var) == VT_I4, "V_VT(var)=%d\n", V_VT(&var));
    ok(V_I4(&var) == 100, "V_I4(&var) == NULL\n");

    unk = (void*)0xdeadbeef;
    hres = IDispatchEx_GetNameSpaceParent(window_dispex, &unk);
    ok(hres == S_OK, "GetNameSpaceParent failed: %08x\n", hres);
    ok(!unk, "unk=%p, expected NULL\n", unk);

    id = 0;
    tmp = SysAllocString(funcW);
    hres = IDispatchEx_GetDispID(document, tmp, fdexNameCaseSensitive|fdexNameEnsure, &id);
    SysFreeString(tmp);
    ok(hres == S_OK, "GetDispID(func) failed: %08x\n", hres);
    ok(id, "id == 0\n");

    dp.cArgs = 1;
    dp.rgvarg = &var;
    dp.cNamedArgs = 0;
    dp.rgdispidNamedArgs = NULL;
    V_VT(&var) = VT_DISPATCH;
    V_DISPATCH(&var) = (IDispatch*)&funcDisp;

    hres = IDispatchEx_InvokeEx(document, id, LOCALE_NEUTRAL, INVOKE_PROPERTYPUT, &dp, NULL, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08x\n", hres);

    VariantInit(&var);
    memset(&dp, 0, sizeof(dp));
    memset(&ei, 0, sizeof(ei));
    V_VT(&arg) = VT_BOOL;
    V_BOOL(&arg) = VARIANT_TRUE;
    dp.cArgs = 1;
    dp.rgvarg = &arg;

    SET_EXPECT(funcDisp);
    hres = IDispatchEx_InvokeEx(document, id, LOCALE_NEUTRAL, INVOKE_FUNC, &dp, &var, &ei, NULL);
    CHECK_CALLED(funcDisp);

    ok(hres == S_OK, "InvokeEx(INVOKE_FUNC) failed: %08x\n", hres);
    ok(V_VT(&var) == VT_I4, "V_VT(var)=%d\n", V_VT(&var));
    ok(V_I4(&var) == 100, "V_I4(&var) == NULL\n");

    IDispatchEx_Release(document);

    return S_OK;
}

static const IActiveScriptParseVtbl ActiveScriptParseVtbl = {
    ActiveScriptParse_QueryInterface,
    ActiveScriptParse_AddRef,
    ActiveScriptParse_Release,
    ActiveScriptParse_InitNew,
    ActiveScriptParse_AddScriptlet,
    ActiveScriptParse_ParseScriptText
};

static IActiveScriptParse ActiveScriptParse = { &ActiveScriptParseVtbl };

static HRESULT WINAPI ActiveScript_QueryInterface(IActiveScript *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IActiveScript, riid)) {
        *ppv = iface;
        return S_OK;
    }

    if(IsEqualGUID(&IID_IActiveScriptParse, riid)) {
        *ppv = &ActiveScriptParse;
        return S_OK;
    }

    if(IsEqualGUID(&IID_IActiveScriptParseProcedure2, riid)) {
        *ppv = &ActiveScriptParseProcedure;
        return S_OK;
    }

    if(IsEqualGUID(&IID_IActiveScriptProperty, riid)) {
        *ppv = &ActiveScriptProperty;
        return S_OK;
    }

    if(IsEqualGUID(&IID_IObjectSafety, riid)) {
        *ppv = &ObjectSafety;
        return S_OK;
    }

    ok(0, "unexpected riid %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI ActiveScript_AddRef(IActiveScript *iface)
{
    return 2;
}

static ULONG WINAPI ActiveScript_Release(IActiveScript *iface)
{
    return 1;
}

static HRESULT WINAPI ActiveScript_SetScriptSite(IActiveScript *iface, IActiveScriptSite *pass)
{
    IActiveScriptSiteInterruptPoll *poll;
    IActiveScriptSiteDebug *debug;
    IServiceProvider *service;
    ICanHandleException *canexpection;
    LCID lcid;
    HRESULT hres;

    CHECK_EXPECT(SetScriptSite);

    ok(pass != NULL, "pass == NULL\n");

    hres = IActiveScriptSite_QueryInterface(pass, &IID_IActiveScriptSiteInterruptPoll, (void**)&poll);
    ok(hres == S_OK, "Could not get IActiveScriptSiteInterruptPoll interface: %08x\n", hres);
    if(FAILED(hres))
        IActiveScriptSiteInterruptPoll_Release(poll);

    hres = IActiveScriptSite_GetLCID(pass, &lcid);
    ok(hres == S_OK, "GetLCID failed: %08x\n", hres);

    hres = IActiveScriptSite_OnStateChange(pass, (state = SCRIPTSTATE_INITIALIZED));
    ok(hres == S_OK, "OnStateChange failed: %08x\n", hres);

    hres = IActiveScriptSite_QueryInterface(pass, &IID_IActiveScriptSiteDebug, (void**)&debug);
    ok(hres == S_OK, "Could not get IActiveScriptSiteDebug interface: %08x\n", hres);
    if(SUCCEEDED(hres))
        IActiveScriptSiteDebug32_Release(debug);

    hres = IActiveScriptSite_QueryInterface(pass, &IID_ICanHandleException, (void**)&canexpection);
    ok(hres == E_NOINTERFACE, "Could not get IID_ICanHandleException interface: %08x\n", hres);

    hres = IActiveScriptSite_QueryInterface(pass, &IID_IServiceProvider, (void**)&service);
    todo_wine ok(hres == S_OK, "Could not get IServiceProvider interface: %08x\n", hres);
    if(SUCCEEDED(hres))
        IServiceProvider_Release(service);

    site = pass;
    IActiveScriptSite_AddRef(site);
    return S_OK;
}

static HRESULT WINAPI ActiveScript_GetScriptSite(IActiveScript *iface, REFIID riid,
                                            void **ppvObject)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScript_SetScriptState(IActiveScript *iface, SCRIPTSTATE ss)
{
    HRESULT hres;

    switch(ss) {
    case SCRIPTSTATE_STARTED:
        CHECK_EXPECT(SetScriptState_STARTED);
        break;
    case SCRIPTSTATE_CONNECTED:
        CHECK_EXPECT(SetScriptState_CONNECTED);
        break;
    case SCRIPTSTATE_DISCONNECTED:
        CHECK_EXPECT(SetScriptState_DISCONNECTED);
        break;
    default:
        ok(0, "unexpected state %d\n", ss);
        return E_NOTIMPL;
    }

    hres = IActiveScriptSite_OnStateChange(site, (state = ss));
    return S_OK;
}

static HRESULT WINAPI ActiveScript_GetScriptState(IActiveScript *iface, SCRIPTSTATE *pssState)
{
    CHECK_EXPECT(GetScriptState);

    *pssState = state;
    return S_OK;
}

static HRESULT WINAPI ActiveScript_Close(IActiveScript *iface)
{
    CHECK_EXPECT(Close);
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScript_AddNamedItem(IActiveScript *iface,
        LPCOLESTR pstrName, DWORD dwFlags)
{
    IDispatch *disp;
    IUnknown *unk = NULL, *unk2;
    HRESULT hres;

    static const WCHAR windowW[] = {'w','i','n','d','o','w',0};

    static const IID unknown_iid = {0x719C3050,0xF9D3,0x11CF,{0xA4,0x93,0x00,0x40,0x05,0x23,0xA8,0xA0}};

    CHECK_EXPECT(AddNamedItem);

    ok(!lstrcmpW(pstrName, windowW), "pstrName=%s\n", debugstr_w(pstrName));
    ok(dwFlags == (SCRIPTITEM_ISVISIBLE|SCRIPTITEM_ISSOURCE|SCRIPTITEM_GLOBALMEMBERS), "dwFlags=%x\n", dwFlags);

    hres = IActiveScriptSite_GetItemInfo(site, windowW, SCRIPTINFO_IUNKNOWN, &unk, NULL);
    ok(hres == S_OK, "GetItemInfo failed: %08x\n", hres);
    ok(unk != NULL, "unk == NULL\n");

    hres = IUnknown_QueryInterface(unk, &IID_IDispatch, (void**)&disp);
    ok(hres == S_OK, "Could not get IDispatch interface: %08x\n", hres);
    if(SUCCEEDED(hres))
        IDispatch_Release(disp);

    hres = IUnknown_QueryInterface(unk, &unknown_iid, (void**)&unk2);
    ok(hres == E_NOINTERFACE, "Got ?? interface: %p\n", unk2);
    if(SUCCEEDED(hres))
        IUnknown_Release(unk2);

    hres = IUnknown_QueryInterface(unk, &IID_IDispatchEx, (void**)&window_dispex);
    ok(hres == S_OK, "Could not get IDispatchEx interface: %08x\n", hres);

    IUnknown_Release(unk);
    return S_OK;
}

static HRESULT WINAPI ActiveScript_AddTypeLib(IActiveScript *iface, REFGUID rguidTypeLib,
                                         DWORD dwMajor, DWORD dwMinor, DWORD dwFlags)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScript_GetScriptDispatch(IActiveScript *iface, LPCOLESTR pstrItemName,
                                                IDispatch **ppdisp)
{
    CHECK_EXPECT(GetScriptDispatch);
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScript_GetCurrentScriptThreadID(IActiveScript *iface,
                                                       SCRIPTTHREADID *pstridThread)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScript_GetScriptThreadID(IActiveScript *iface,
                                                DWORD dwWin32ThreadId, SCRIPTTHREADID *pstidThread)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScript_GetScriptThreadState(IActiveScript *iface,
        SCRIPTTHREADID stidThread, SCRIPTTHREADSTATE *pstsState)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScript_InterruptScriptThread(IActiveScript *iface,
        SCRIPTTHREADID stidThread, const EXCEPINFO *pexcepinfo, DWORD dwFlags)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScript_Clone(IActiveScript *iface, IActiveScript **ppscript)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IActiveScriptVtbl ActiveScriptVtbl = {
    ActiveScript_QueryInterface,
    ActiveScript_AddRef,
    ActiveScript_Release,
    ActiveScript_SetScriptSite,
    ActiveScript_GetScriptSite,
    ActiveScript_SetScriptState,
    ActiveScript_GetScriptState,
    ActiveScript_Close,
    ActiveScript_AddNamedItem,
    ActiveScript_AddTypeLib,
    ActiveScript_GetScriptDispatch,
    ActiveScript_GetCurrentScriptThreadID,
    ActiveScript_GetScriptThreadID,
    ActiveScript_GetScriptThreadState,
    ActiveScript_InterruptScriptThread,
    ActiveScript_Clone
};

static IActiveScript ActiveScript = { &ActiveScriptVtbl };

static HRESULT WINAPI ClassFactory_QueryInterface(IClassFactory *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IClassFactory, riid)) {
        *ppv = iface;
        return S_OK;
    }

    if(IsEqualGUID(&IID_IMarshal, riid))
        return E_NOINTERFACE;
    if(IsEqualGUID(&CLSID_IdentityUnmarshal, riid))
        return E_NOINTERFACE;

    ok(0, "unexpected riid %s\n", debugstr_guid(riid));
    return E_NOTIMPL;
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
    ok(IsEqualGUID(&IID_IActiveScript, riid), "unexpected riid %s\n", debugstr_guid(riid));
    *ppv = &ActiveScript;
    return S_OK;
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

static IClassFactory script_cf = { &ClassFactoryVtbl };

static const char simple_script_str[] =
    "<html><head></head><body>"
    "<script language=\"TestScript\">simple script</script>"
    "</body></html>";

static void test_simple_script(void)
{
    IHTMLDocument2 *doc;

    SET_EXPECT(CreateInstance);
    SET_EXPECT(GetInterfaceSafetyOptions);
    SET_EXPECT(SetInterfaceSafetyOptions);
    SET_EXPECT(SetProperty);
    SET_EXPECT(InitNew);
    SET_EXPECT(SetScriptSite);
    SET_EXPECT(GetScriptState);
    SET_EXPECT(SetScriptState_STARTED);
    SET_EXPECT(AddNamedItem);
    SET_EXPECT(ParseScriptText);
    SET_EXPECT(SetScriptState_CONNECTED);

    doc = create_and_load_doc(simple_script_str);
    if(!doc) return;

    CHECK_CALLED(CreateInstance);
    CHECK_CALLED(GetInterfaceSafetyOptions);
    CHECK_CALLED(SetInterfaceSafetyOptions);
    CHECK_CALLED(SetProperty);
    CHECK_CALLED(InitNew);
    CHECK_CALLED(SetScriptSite);
    CHECK_CALLED(GetScriptState);
    CHECK_CALLED(SetScriptState_STARTED);
    CHECK_CALLED(AddNamedItem);
    CHECK_CALLED(ParseScriptText);
    CHECK_CALLED(SetScriptState_CONNECTED);

    if(site)
        IActiveScriptSite_Release(site);
    if(window_dispex)
        IDispatchEx_Release(window_dispex);

    SET_EXPECT(SetScriptState_DISCONNECTED);
    SET_EXPECT(Close);

    IHTMLDocument2_Release(doc);

    CHECK_CALLED(SetScriptState_DISCONNECTED);
    CHECK_CALLED(Close);
}

static BOOL init_key(const char *key_name, const char *def_value, BOOL init)
{
    HKEY hkey;
    DWORD res;

    if(!init) {
        RegDeleteKey(HKEY_CLASSES_ROOT, key_name);
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
    return init_key("TestScript\\CLSID", TESTSCRIPT_CLSID, init)
        && init_key("CLSID\\"TESTSCRIPT_CLSID"\\Implemented Categories\\{F0B7A1A1-9847-11CF-8F20-00805F2CD064}",
                    NULL, init)
        && init_key("CLSID\\"TESTSCRIPT_CLSID"\\Implemented Categories\\{F0B7A1A2-9847-11CF-8F20-00805F2CD064}",
                    NULL, init);
}

static BOOL register_script_engine(void)
{
    DWORD regid;
    HRESULT hres;

    if(!init_registry(TRUE)) {
        init_registry(FALSE);
        return FALSE;
    }

    hres = CoRegisterClassObject(&CLSID_TestScript, (IUnknown *)&script_cf,
                                 CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE, &regid);
    ok(hres == S_OK, "Could not register screipt engine: %08x\n", hres);

    return TRUE;
}

static void gecko_installer_workaround(BOOL disable)
{
    HKEY hkey;
    DWORD res;

    static BOOL has_url = FALSE;
    static char url[2048];

    if(!disable && !has_url)
        return;

    res = RegOpenKey(HKEY_CURRENT_USER, "Software\\Wine\\MSHTML", &hkey);
    if(res != ERROR_SUCCESS)
        return;

    if(disable) {
        DWORD type, size = sizeof(url);

        res = RegQueryValueEx(hkey, "GeckoUrl", NULL, &type, (PVOID)url, &size);
        if(res == ERROR_SUCCESS && type == REG_SZ)
            has_url = TRUE;

        RegDeleteValue(hkey, "GeckoUrl");
    }else {
        RegSetValueEx(hkey, "GeckoUrl", 0, REG_SZ, (PVOID)url, lstrlenA(url)+1);
    }

    RegCloseKey(hkey);
}

/* Check if Internet Explorer is configured to run in "Enhanced Security Configuration" (aka hardened mode) */
/* Note: this code is duplicated in dlls/mshtml/tests/dom.c, dlls/mshtml/tests/script.c and dlls/urlmon/tests/misc.c */
static BOOL is_ie_hardened(void)
{
    HKEY zone_map;
    DWORD ie_harden, type, size;

    ie_harden = 0;
    if(RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\ZoneMap",
                    0, KEY_QUERY_VALUE, &zone_map) == ERROR_SUCCESS) {
        size = sizeof(DWORD);
        if (RegQueryValueEx(zone_map, "IEHarden", NULL, &type, (LPBYTE) &ie_harden, &size) != ERROR_SUCCESS ||
            type != REG_DWORD) {
            ie_harden = 0;
        }
    RegCloseKey(zone_map);
    }

    return ie_harden != 0;
}

START_TEST(script)
{
    gecko_installer_workaround(TRUE);
    CoInitialize(NULL);

    if(winetest_interactive || ! is_ie_hardened()) {
        if(register_script_engine()) {
            test_simple_script();
            init_registry(FALSE);
        }else {
            skip("Could not register TestScript engine\n");
        }
    }else {
        skip("IE running in Enhanced Security Configuration\n");
    }

    CoUninitialize();
    gecko_installer_workaround(FALSE);
}
