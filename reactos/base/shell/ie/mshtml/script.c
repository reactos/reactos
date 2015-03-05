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

#include "mshtml_private.h"

#include <activdbg.h>

#ifdef _WIN64

#define CTXARG_T DWORDLONG
#define IActiveScriptSiteDebugVtbl IActiveScriptSiteDebug64Vtbl

#define IActiveScriptParse_Release IActiveScriptParse64_Release
#define IActiveScriptParse_InitNew IActiveScriptParse64_InitNew
#define IActiveScriptParse_ParseScriptText IActiveScriptParse64_ParseScriptText
#define IActiveScriptParseProcedure2_Release IActiveScriptParseProcedure2_64_Release
#define IActiveScriptParseProcedure2_ParseProcedureText IActiveScriptParseProcedure2_64_ParseProcedureText

#else

#define CTXARG_T DWORD
#define IActiveScriptSiteDebugVtbl IActiveScriptSiteDebug32Vtbl

#define IActiveScriptParse_Release IActiveScriptParse32_Release
#define IActiveScriptParse_InitNew IActiveScriptParse32_InitNew
#define IActiveScriptParse_ParseScriptText IActiveScriptParse32_ParseScriptText
#define IActiveScriptParseProcedure2_Release IActiveScriptParseProcedure2_32_Release
#define IActiveScriptParseProcedure2_ParseProcedureText IActiveScriptParseProcedure2_32_ParseProcedureText

#endif

static const WCHAR documentW[] = {'d','o','c','u','m','e','n','t',0};
static const WCHAR windowW[] = {'w','i','n','d','o','w',0};
static const WCHAR script_endW[] = {'<','/','S','C','R','I','P','T','>',0};
static const WCHAR emptyW[] = {0};

struct ScriptHost {
    IActiveScriptSite              IActiveScriptSite_iface;
    IActiveScriptSiteInterruptPoll IActiveScriptSiteInterruptPoll_iface;
    IActiveScriptSiteWindow        IActiveScriptSiteWindow_iface;
    IActiveScriptSiteUIControl     IActiveScriptSiteUIControl_iface;
    IActiveScriptSiteDebug         IActiveScriptSiteDebug_iface;
    IServiceProvider               IServiceProvider_iface;

    LONG ref;

    IActiveScript *script;
    IActiveScriptParse *parse;
    IActiveScriptParseProcedure2 *parse_proc;

    SCRIPTSTATE script_state;

    HTMLInnerWindow *window;

    GUID guid;
    struct list entry;
};

static void set_script_prop(ScriptHost *script_host, DWORD property, VARIANT *val)
{
    IActiveScriptProperty *script_prop;
    HRESULT hres;

    hres = IActiveScript_QueryInterface(script_host->script, &IID_IActiveScriptProperty,
            (void**)&script_prop);
    if(FAILED(hres)) {
        WARN("Could not get IActiveScriptProperty iface: %08x\n", hres);
        return;
    }

    hres = IActiveScriptProperty_SetProperty(script_prop, property, NULL, val);
    IActiveScriptProperty_Release(script_prop);
    if(FAILED(hres))
        WARN("SetProperty(%x) failed: %08x\n", property, hres);
}

static BOOL init_script_engine(ScriptHost *script_host)
{
    IObjectSafety *safety;
    SCRIPTSTATE state;
    DWORD supported_opts=0, enabled_opts=0;
    VARIANT var;
    HRESULT hres;

    hres = IActiveScript_QueryInterface(script_host->script, &IID_IActiveScriptParse, (void**)&script_host->parse);
    if(FAILED(hres)) {
        WARN("Could not get IActiveScriptHost: %08x\n", hres);
        return FALSE;
    }

    hres = IActiveScript_QueryInterface(script_host->script, &IID_IObjectSafety, (void**)&safety);
    if(FAILED(hres)) {
        FIXME("Could not get IObjectSafety: %08x\n", hres);
        return FALSE;
    }

    hres = IObjectSafety_GetInterfaceSafetyOptions(safety, &IID_IActiveScriptParse, &supported_opts, &enabled_opts);
    if(FAILED(hres)) {
        FIXME("GetInterfaceSafetyOptions failed: %08x\n", hres);
    }else if(!(supported_opts & INTERFACE_USES_DISPEX)) {
        FIXME("INTERFACE_USES_DISPEX is not supported\n");
    }else {
        hres = IObjectSafety_SetInterfaceSafetyOptions(safety, &IID_IActiveScriptParse,
                INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER,
                INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER);
        if(FAILED(hres))
            FIXME("SetInterfaceSafetyOptions failed: %08x\n", hres);
    }

    IObjectSafety_Release(safety);
    if(FAILED(hres))
        return FALSE;

    V_VT(&var) = VT_I4;
    V_I4(&var) = 1;
    set_script_prop(script_host, SCRIPTPROP_INVOKEVERSIONING, &var);

    V_VT(&var) = VT_BOOL;
    V_BOOL(&var) = VARIANT_TRUE;
    set_script_prop(script_host, SCRIPTPROP_HACK_TRIDENTEVENTSINK, &var);

    hres = IActiveScriptParse_InitNew(script_host->parse);
    if(FAILED(hres)) {
        WARN("InitNew failed: %08x\n", hres);
        return FALSE;
    }

    hres = IActiveScript_SetScriptSite(script_host->script, &script_host->IActiveScriptSite_iface);
    if(FAILED(hres)) {
        WARN("SetScriptSite failed: %08x\n", hres);
        IActiveScript_Close(script_host->script);
        return FALSE;
    }

    hres = IActiveScript_GetScriptState(script_host->script, &state);
    if(FAILED(hres))
        WARN("GetScriptState failed: %08x\n", hres);
    else if(state != SCRIPTSTATE_INITIALIZED)
        FIXME("state = %x\n", state);

    hres = IActiveScript_SetScriptState(script_host->script, SCRIPTSTATE_STARTED);
    if(FAILED(hres)) {
        WARN("Starting script failed: %08x\n", hres);
        return FALSE;
    }

    hres = IActiveScript_AddNamedItem(script_host->script, windowW,
            SCRIPTITEM_ISVISIBLE|SCRIPTITEM_ISSOURCE|SCRIPTITEM_GLOBALMEMBERS);
    if(SUCCEEDED(hres)) {
        V_VT(&var) = VT_BOOL;
        V_BOOL(&var) = VARIANT_TRUE;
        set_script_prop(script_host, SCRIPTPROP_ABBREVIATE_GLOBALNAME_RESOLUTION, &var);
    }else {
       WARN("AddNamedItem failed: %08x\n", hres);
    }

    hres = IActiveScript_QueryInterface(script_host->script, &IID_IActiveScriptParseProcedure2,
                                        (void**)&script_host->parse_proc);
    if(FAILED(hres)) {
        /* FIXME: QI for IActiveScriptParseProcedure */
        WARN("Could not get IActiveScriptParseProcedure iface: %08x\n", hres);
    }

    return TRUE;
}

static void release_script_engine(ScriptHost *This)
{
    if(!This->script)
        return;

    switch(This->script_state) {
    case SCRIPTSTATE_CONNECTED:
        IActiveScript_SetScriptState(This->script, SCRIPTSTATE_DISCONNECTED);

    case SCRIPTSTATE_STARTED:
    case SCRIPTSTATE_DISCONNECTED:
    case SCRIPTSTATE_INITIALIZED:
        IActiveScript_Close(This->script);

    default:
        if(This->parse_proc) {
            IActiveScriptParseProcedure2_Release(This->parse_proc);
            This->parse_proc = NULL;
        }

        if(This->parse) {
            IActiveScriptParse_Release(This->parse);
            This->parse = NULL;
        }
    }

    IActiveScript_Release(This->script);
    This->script = NULL;
    This->script_state = SCRIPTSTATE_UNINITIALIZED;
}

void connect_scripts(HTMLInnerWindow *window)
{
    ScriptHost *iter;

    LIST_FOR_EACH_ENTRY(iter, &window->script_hosts, ScriptHost, entry) {
        if(iter->script_state == SCRIPTSTATE_STARTED)
            IActiveScript_SetScriptState(iter->script, SCRIPTSTATE_CONNECTED);
    }
}

static inline ScriptHost *impl_from_IActiveScriptSite(IActiveScriptSite *iface)
{
    return CONTAINING_RECORD(iface, ScriptHost, IActiveScriptSite_iface);
}

static HRESULT WINAPI ActiveScriptSite_QueryInterface(IActiveScriptSite *iface, REFIID riid, void **ppv)
{
    ScriptHost *This = impl_from_IActiveScriptSite(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IActiveScriptSite_iface;
    }else if(IsEqualGUID(&IID_IActiveScriptSite, riid)) {
        TRACE("(%p)->(IID_IActiveScriptSite %p)\n", This, ppv);
        *ppv = &This->IActiveScriptSite_iface;
    }else if(IsEqualGUID(&IID_IActiveScriptSiteInterruptPoll, riid)) {
        TRACE("(%p)->(IID_IActiveScriptSiteInterruprtPoll %p)\n", This, ppv);
        *ppv = &This->IActiveScriptSiteInterruptPoll_iface;
    }else if(IsEqualGUID(&IID_IActiveScriptSiteWindow, riid)) {
        TRACE("(%p)->(IID_IActiveScriptSiteWindow %p)\n", This, ppv);
        *ppv = &This->IActiveScriptSiteWindow_iface;
    }else if(IsEqualGUID(&IID_IActiveScriptSiteUIControl, riid)) {
        TRACE("(%p)->(IID_IActiveScriptSiteUIControl %p)\n", This, ppv);
        *ppv = &This->IActiveScriptSiteUIControl_iface;
    }else if(IsEqualGUID(&IID_IActiveScriptSiteDebug, riid)) {
        TRACE("(%p)->(IID_IActiveScriptSiteDebug %p)\n", This, ppv);
        *ppv = &This->IActiveScriptSiteDebug_iface;
    }else if(IsEqualGUID(&IID_IServiceProvider, riid)) {
        TRACE("(%p)->(IID_IServiceProvider %p)\n", This, ppv);
        *ppv = &This->IServiceProvider_iface;
    }else if(IsEqualGUID(&IID_ICanHandleException, riid)) {
        TRACE("(%p)->(IID_ICanHandleException not supported %p)\n", This, ppv);
        return E_NOINTERFACE;
    }else {
        FIXME("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI ActiveScriptSite_AddRef(IActiveScriptSite *iface)
{
    ScriptHost *This = impl_from_IActiveScriptSite(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI ActiveScriptSite_Release(IActiveScriptSite *iface)
{
    ScriptHost *This = impl_from_IActiveScriptSite(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        release_script_engine(This);
        if(This->window)
            list_remove(&This->entry);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI ActiveScriptSite_GetLCID(IActiveScriptSite *iface, LCID *plcid)
{
    ScriptHost *This = impl_from_IActiveScriptSite(iface);

    TRACE("(%p)->(%p)\n", This, plcid);

    *plcid = GetUserDefaultLCID();
    return S_OK;
}

static HRESULT WINAPI ActiveScriptSite_GetItemInfo(IActiveScriptSite *iface, LPCOLESTR pstrName,
        DWORD dwReturnMask, IUnknown **ppiunkItem, ITypeInfo **ppti)
{
    ScriptHost *This = impl_from_IActiveScriptSite(iface);

    TRACE("(%p)->(%s %x %p %p)\n", This, debugstr_w(pstrName), dwReturnMask, ppiunkItem, ppti);

    if(dwReturnMask != SCRIPTINFO_IUNKNOWN) {
        FIXME("Unsupported mask %x\n", dwReturnMask);
        return E_NOTIMPL;
    }

    *ppiunkItem = NULL;

    if(strcmpW(pstrName, windowW))
        return DISP_E_MEMBERNOTFOUND;

    if(!This->window)
        return E_FAIL;

    /* FIXME: Return proxy object */
    *ppiunkItem = (IUnknown*)&This->window->base.IHTMLWindow2_iface;
    IUnknown_AddRef(*ppiunkItem);

    return S_OK;
}

static HRESULT WINAPI ActiveScriptSite_GetDocVersionString(IActiveScriptSite *iface, BSTR *pbstrVersion)
{
    ScriptHost *This = impl_from_IActiveScriptSite(iface);
    FIXME("(%p)->(%p)\n", This, pbstrVersion);
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_OnScriptTerminate(IActiveScriptSite *iface,
        const VARIANT *pvarResult, const EXCEPINFO *pexcepinfo)
{
    ScriptHost *This = impl_from_IActiveScriptSite(iface);
    FIXME("(%p)->(%p %p)\n", This, pvarResult, pexcepinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_OnStateChange(IActiveScriptSite *iface, SCRIPTSTATE ssScriptState)
{
    ScriptHost *This = impl_from_IActiveScriptSite(iface);

    TRACE("(%p)->(%x)\n", This, ssScriptState);

    This->script_state = ssScriptState;
    return S_OK;
}

static HRESULT WINAPI ActiveScriptSite_OnScriptError(IActiveScriptSite *iface, IActiveScriptError *pscripterror)
{
    ScriptHost *This = impl_from_IActiveScriptSite(iface);
    FIXME("(%p)->(%p)\n", This, pscripterror);
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_OnEnterScript(IActiveScriptSite *iface)
{
    ScriptHost *This = impl_from_IActiveScriptSite(iface);

    TRACE("(%p)->()\n", This);

    return S_OK;
}

static HRESULT WINAPI ActiveScriptSite_OnLeaveScript(IActiveScriptSite *iface)
{
    ScriptHost *This = impl_from_IActiveScriptSite(iface);

    TRACE("(%p)->()\n", This);

    return S_OK;
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

static inline ScriptHost *impl_from_IActiveScriptSiteInterruptPoll(IActiveScriptSiteInterruptPoll *iface)
{
    return CONTAINING_RECORD(iface, ScriptHost, IActiveScriptSiteInterruptPoll_iface);
}

static HRESULT WINAPI ActiveScriptSiteInterruptPoll_QueryInterface(IActiveScriptSiteInterruptPoll *iface,
        REFIID riid, void **ppv)
{
    ScriptHost *This = impl_from_IActiveScriptSiteInterruptPoll(iface);
    return IActiveScriptSite_QueryInterface(&This->IActiveScriptSite_iface, riid, ppv);
}

static ULONG WINAPI ActiveScriptSiteInterruptPoll_AddRef(IActiveScriptSiteInterruptPoll *iface)
{
    ScriptHost *This = impl_from_IActiveScriptSiteInterruptPoll(iface);
    return IActiveScriptSite_AddRef(&This->IActiveScriptSite_iface);
}

static ULONG WINAPI ActiveScriptSiteInterruptPoll_Release(IActiveScriptSiteInterruptPoll *iface)
{
    ScriptHost *This = impl_from_IActiveScriptSiteInterruptPoll(iface);
    return IActiveScriptSite_Release(&This->IActiveScriptSite_iface);
}

static HRESULT WINAPI ActiveScriptSiteInterruptPoll_QueryContinue(IActiveScriptSiteInterruptPoll *iface)
{
    ScriptHost *This = impl_from_IActiveScriptSiteInterruptPoll(iface);

    TRACE("(%p)\n", This);

    return S_OK;
}

static const IActiveScriptSiteInterruptPollVtbl ActiveScriptSiteInterruptPollVtbl = {
    ActiveScriptSiteInterruptPoll_QueryInterface,
    ActiveScriptSiteInterruptPoll_AddRef,
    ActiveScriptSiteInterruptPoll_Release,
    ActiveScriptSiteInterruptPoll_QueryContinue
};

static inline ScriptHost *impl_from_IActiveScriptSiteWindow(IActiveScriptSiteWindow *iface)
{
    return CONTAINING_RECORD(iface, ScriptHost, IActiveScriptSiteWindow_iface);
}

static HRESULT WINAPI ActiveScriptSiteWindow_QueryInterface(IActiveScriptSiteWindow *iface,
        REFIID riid, void **ppv)
{
    ScriptHost *This = impl_from_IActiveScriptSiteWindow(iface);
    return IActiveScriptSite_QueryInterface(&This->IActiveScriptSite_iface, riid, ppv);
}

static ULONG WINAPI ActiveScriptSiteWindow_AddRef(IActiveScriptSiteWindow *iface)
{
    ScriptHost *This = impl_from_IActiveScriptSiteWindow(iface);
    return IActiveScriptSite_AddRef(&This->IActiveScriptSite_iface);
}

static ULONG WINAPI ActiveScriptSiteWindow_Release(IActiveScriptSiteWindow *iface)
{
    ScriptHost *This = impl_from_IActiveScriptSiteWindow(iface);
    return IActiveScriptSite_Release(&This->IActiveScriptSite_iface);
}

static HRESULT WINAPI ActiveScriptSiteWindow_GetWindow(IActiveScriptSiteWindow *iface, HWND *phwnd)
{
    ScriptHost *This = impl_from_IActiveScriptSiteWindow(iface);

    TRACE("(%p)->(%p)\n", This, phwnd);

    if(!This->window || !This->window->base.outer_window || !This->window->base.outer_window->doc_obj)
        return E_UNEXPECTED;

    *phwnd = This->window->base.outer_window->doc_obj->hwnd;
    return S_OK;
}

static HRESULT WINAPI ActiveScriptSiteWindow_EnableModeless(IActiveScriptSiteWindow *iface, BOOL fEnable)
{
    ScriptHost *This = impl_from_IActiveScriptSiteWindow(iface);
    FIXME("(%p)->(%x)\n", This, fEnable);
    return S_OK;
}

static const IActiveScriptSiteWindowVtbl ActiveScriptSiteWindowVtbl = {
    ActiveScriptSiteWindow_QueryInterface,
    ActiveScriptSiteWindow_AddRef,
    ActiveScriptSiteWindow_Release,
    ActiveScriptSiteWindow_GetWindow,
    ActiveScriptSiteWindow_EnableModeless
};

static inline ScriptHost *impl_from_IActiveScriptSiteUIControl(IActiveScriptSiteUIControl *iface)
{
    return CONTAINING_RECORD(iface, ScriptHost, IActiveScriptSiteUIControl_iface);
}

static HRESULT WINAPI ActiveScriptSiteUIControl_QueryInterface(IActiveScriptSiteUIControl *iface, REFIID riid, void **ppv)
{
    ScriptHost *This = impl_from_IActiveScriptSiteUIControl(iface);
    return IActiveScriptSite_QueryInterface(&This->IActiveScriptSite_iface, riid, ppv);
}

static ULONG WINAPI ActiveScriptSiteUIControl_AddRef(IActiveScriptSiteUIControl *iface)
{
    ScriptHost *This = impl_from_IActiveScriptSiteUIControl(iface);
    return IActiveScriptSite_AddRef(&This->IActiveScriptSite_iface);
}

static ULONG WINAPI ActiveScriptSiteUIControl_Release(IActiveScriptSiteUIControl *iface)
{
    ScriptHost *This = impl_from_IActiveScriptSiteUIControl(iface);
    return IActiveScriptSite_Release(&This->IActiveScriptSite_iface);
}

static HRESULT WINAPI ActiveScriptSiteUIControl_GetUIBehavior(IActiveScriptSiteUIControl *iface, SCRIPTUICITEM UicItem,
        SCRIPTUICHANDLING *pUicHandling)
{
    ScriptHost *This = impl_from_IActiveScriptSiteUIControl(iface);

    WARN("(%p)->(%d %p) semi-stub\n", This, UicItem, pUicHandling);

    *pUicHandling = SCRIPTUICHANDLING_ALLOW;
    return S_OK;
}

static const IActiveScriptSiteUIControlVtbl ActiveScriptSiteUIControlVtbl = {
    ActiveScriptSiteUIControl_QueryInterface,
    ActiveScriptSiteUIControl_AddRef,
    ActiveScriptSiteUIControl_Release,
    ActiveScriptSiteUIControl_GetUIBehavior
};

static inline ScriptHost *impl_from_IActiveScriptSiteDebug(IActiveScriptSiteDebug *iface)
{
    return CONTAINING_RECORD(iface, ScriptHost, IActiveScriptSiteDebug_iface);
}

static HRESULT WINAPI ActiveScriptSiteDebug_QueryInterface(IActiveScriptSiteDebug *iface,
        REFIID riid, void **ppv)
{
    ScriptHost *This = impl_from_IActiveScriptSiteDebug(iface);
    return IActiveScriptSite_QueryInterface(&This->IActiveScriptSite_iface, riid, ppv);
}

static ULONG WINAPI ActiveScriptSiteDebug_AddRef(IActiveScriptSiteDebug *iface)
{
    ScriptHost *This = impl_from_IActiveScriptSiteDebug(iface);
    return IActiveScriptSite_AddRef(&This->IActiveScriptSite_iface);
}

static ULONG WINAPI ActiveScriptSiteDebug_Release(IActiveScriptSiteDebug *iface)
{
    ScriptHost *This = impl_from_IActiveScriptSiteDebug(iface);
    return IActiveScriptSite_Release(&This->IActiveScriptSite_iface);
}

static HRESULT WINAPI ActiveScriptSiteDebug_GetDocumentContextFromPosition(IActiveScriptSiteDebug *iface,
            CTXARG_T dwSourceContext, ULONG uCharacterOffset, ULONG uNumChars, IDebugDocumentContext **ppsc)
{
    ScriptHost *This = impl_from_IActiveScriptSiteDebug(iface);
    FIXME("(%p)->(%s %u %u %p)\n", This, wine_dbgstr_longlong(dwSourceContext), uCharacterOffset,
          uNumChars, ppsc);
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSiteDebug_GetApplication(IActiveScriptSiteDebug *iface, IDebugApplication **ppda)
{
    ScriptHost *This = impl_from_IActiveScriptSiteDebug(iface);
    FIXME("(%p)->(%p)\n", This, ppda);
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSiteDebug_GetRootApplicationNode(IActiveScriptSiteDebug *iface,
            IDebugApplicationNode **ppdanRoot)
{
    ScriptHost *This = impl_from_IActiveScriptSiteDebug(iface);
    FIXME("(%p)->(%p)\n", This, ppdanRoot);
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSiteDebug_OnScriptErrorDebug(IActiveScriptSiteDebug *iface,
            IActiveScriptErrorDebug *pErrorDebug, BOOL *pfEnterDebugger, BOOL *pfCallOnScriptErrorWhenContinuing)
{
    ScriptHost *This = impl_from_IActiveScriptSiteDebug(iface);
    FIXME("(%p)->(%p %p %p)\n", This, pErrorDebug, pfEnterDebugger, pfCallOnScriptErrorWhenContinuing);
    return E_NOTIMPL;
}

static const IActiveScriptSiteDebugVtbl ActiveScriptSiteDebugVtbl = {
    ActiveScriptSiteDebug_QueryInterface,
    ActiveScriptSiteDebug_AddRef,
    ActiveScriptSiteDebug_Release,
    ActiveScriptSiteDebug_GetDocumentContextFromPosition,
    ActiveScriptSiteDebug_GetApplication,
    ActiveScriptSiteDebug_GetRootApplicationNode,
    ActiveScriptSiteDebug_OnScriptErrorDebug
};

static inline ScriptHost *impl_from_IServiceProvider(IServiceProvider *iface)
{
    return CONTAINING_RECORD(iface, ScriptHost, IServiceProvider_iface);
}

static HRESULT WINAPI ASServiceProvider_QueryInterface(IServiceProvider *iface, REFIID riid, void **ppv)
{
    ScriptHost *This = impl_from_IServiceProvider(iface);
    return IActiveScriptSite_QueryInterface(&This->IActiveScriptSite_iface, riid, ppv);
}

static ULONG WINAPI ASServiceProvider_AddRef(IServiceProvider *iface)
{
    ScriptHost *This = impl_from_IServiceProvider(iface);
    return IActiveScriptSite_AddRef(&This->IActiveScriptSite_iface);
}

static ULONG WINAPI ASServiceProvider_Release(IServiceProvider *iface)
{
    ScriptHost *This = impl_from_IServiceProvider(iface);
    return IActiveScriptSite_Release(&This->IActiveScriptSite_iface);
}

static HRESULT WINAPI ASServiceProvider_QueryService(IServiceProvider *iface, REFGUID guidService,
        REFIID riid, void **ppv)
{
    ScriptHost *This = impl_from_IServiceProvider(iface);

    if(IsEqualGUID(&SID_SInternetHostSecurityManager, guidService)) {
        TRACE("(%p)->(SID_SInternetHostSecurityManager)\n", This);

        if(!This->window || !This->window->doc)
            return E_NOINTERFACE;

        return IInternetHostSecurityManager_QueryInterface(&This->window->doc->IInternetHostSecurityManager_iface,
                riid, ppv);
    }

    if(IsEqualGUID(&SID_SContainerDispatch, guidService)) {
        TRACE("(%p)->(SID_SContainerDispatch)\n", This);

        if(!This->window || !This->window->doc)
            return E_NOINTERFACE;

        return IHTMLDocument2_QueryInterface(&This->window->doc->basedoc.IHTMLDocument2_iface, riid, ppv);
    }

    FIXME("(%p)->(%s %s %p)\n", This, debugstr_guid(guidService), debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static const IServiceProviderVtbl ASServiceProviderVtbl = {
    ASServiceProvider_QueryInterface,
    ASServiceProvider_AddRef,
    ASServiceProvider_Release,
    ASServiceProvider_QueryService
};

static ScriptHost *create_script_host(HTMLInnerWindow *window, const GUID *guid)
{
    ScriptHost *ret;
    HRESULT hres;

    ret = heap_alloc_zero(sizeof(*ret));
    if(!ret)
        return NULL;

    ret->IActiveScriptSite_iface.lpVtbl = &ActiveScriptSiteVtbl;
    ret->IActiveScriptSiteInterruptPoll_iface.lpVtbl = &ActiveScriptSiteInterruptPollVtbl;
    ret->IActiveScriptSiteWindow_iface.lpVtbl = &ActiveScriptSiteWindowVtbl;
    ret->IActiveScriptSiteUIControl_iface.lpVtbl = &ActiveScriptSiteUIControlVtbl;
    ret->IActiveScriptSiteDebug_iface.lpVtbl = &ActiveScriptSiteDebugVtbl;
    ret->IServiceProvider_iface.lpVtbl = &ASServiceProviderVtbl;
    ret->ref = 1;
    ret->window = window;
    ret->script_state = SCRIPTSTATE_UNINITIALIZED;

    ret->guid = *guid;
    list_add_tail(&window->script_hosts, &ret->entry);

    hres = CoCreateInstance(&ret->guid, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IActiveScript, (void**)&ret->script);
    if(FAILED(hres))
        WARN("Could not load script engine: %08x\n", hres);
    else if(!init_script_engine(ret))
        release_script_engine(ret);

    return ret;
}

static void parse_text(ScriptHost *script_host, LPCWSTR text)
{
    EXCEPINFO excepinfo;
    VARIANT var;
    HRESULT hres;

    TRACE("%s\n", debugstr_w(text));

    VariantInit(&var);
    memset(&excepinfo, 0, sizeof(excepinfo));
    TRACE(">>>\n");
    hres = IActiveScriptParse_ParseScriptText(script_host->parse, text, windowW, NULL, script_endW,
                                              0, 0, SCRIPTTEXT_ISVISIBLE|SCRIPTTEXT_HOSTMANAGESSOURCE,
                                              &var, &excepinfo);
    if(SUCCEEDED(hres))
        TRACE("<<<\n");
    else
        WARN("<<< %08x\n", hres);

}

static void parse_extern_script(ScriptHost *script_host, LPCWSTR src)
{
    IMoniker *mon;
    WCHAR *text;
    HRESULT hres;

    static const WCHAR wine_schemaW[] = {'w','i','n','e',':'};

    if(strlenW(src) > sizeof(wine_schemaW)/sizeof(WCHAR) && !memcmp(src, wine_schemaW, sizeof(wine_schemaW)))
        src += sizeof(wine_schemaW)/sizeof(WCHAR);

    hres = CreateURLMoniker(NULL, src, &mon);
    if(FAILED(hres))
        return;

    hres = bind_mon_to_wstr(script_host->window, mon, &text);
    IMoniker_Release(mon);
    if(FAILED(hres))
        return;

    parse_text(script_host, text);

    heap_free(text);
}

static void parse_inline_script(ScriptHost *script_host, HTMLScriptElement *script_elem)
{
    const PRUnichar *text;
    nsAString text_str;
    nsresult nsres;

    nsAString_Init(&text_str, NULL);
    nsres = nsIDOMHTMLScriptElement_GetText(script_elem->nsscript, &text_str);
    nsAString_GetData(&text_str, &text);

    if(NS_FAILED(nsres)) {
        ERR("GetText failed: %08x\n", nsres);
    }else if(*text) {
        parse_text(script_host, text);
    }

    nsAString_Finish(&text_str);
}

static void parse_script_elem(ScriptHost *script_host, HTMLScriptElement *script_elem)
{
    nsAString src_str, event_str;
    const PRUnichar *src;
    nsresult nsres;

    nsAString_Init(&event_str, NULL);
    nsres = nsIDOMHTMLScriptElement_GetEvent(script_elem->nsscript, &event_str);
    if(NS_SUCCEEDED(nsres)) {
        const PRUnichar *event;

        nsAString_GetData(&event_str, &event);
        if(*event) {
            TRACE("deferring event %s script evaluation\n", debugstr_w(event));
            nsAString_Finish(&event_str);
            return;
        }
    }else {
        ERR("GetEvent failed: %08x\n", nsres);
    }
    nsAString_Finish(&event_str);

    nsAString_Init(&src_str, NULL);
    nsres = nsIDOMHTMLScriptElement_GetSrc(script_elem->nsscript, &src_str);
    nsAString_GetData(&src_str, &src);

    if(NS_FAILED(nsres)) {
        ERR("GetSrc failed: %08x\n", nsres);
    }else if(*src) {
        script_elem->parsed = TRUE;
        parse_extern_script(script_host, src);
    }else {
        parse_inline_script(script_host, script_elem);
    }

    nsAString_Finish(&src_str);
}

static GUID get_default_script_guid(HTMLInnerWindow *window)
{
    /* If not specified, we should use very first script host that was created for the page (or JScript if none) */
    return list_empty(&window->script_hosts)
        ? CLSID_JScript
        : LIST_ENTRY(list_head(&window->script_hosts), ScriptHost, entry)->guid;
}

static BOOL get_guid_from_type(LPCWSTR type, GUID *guid)
{
    const WCHAR text_javascriptW[] =
        {'t','e','x','t','/','j','a','v','a','s','c','r','i','p','t',0};
    const WCHAR text_jscriptW[] =
        {'t','e','x','t','/','j','s','c','r','i','p','t',0};
    const WCHAR text_vbscriptW[] =
        {'t','e','x','t','/','v','b','s','c','r','i','p','t',0};

    /* FIXME: Handle more types */
    if(!strcmpiW(type, text_javascriptW) || !strcmpiW(type, text_jscriptW)) {
        *guid = CLSID_JScript;
    }else if(!strcmpiW(type, text_vbscriptW)) {
        *guid = CLSID_VBScript;
    }else {
        FIXME("Unknown type %s\n", debugstr_w(type));
        return FALSE;
    }

    return TRUE;
}

static BOOL get_guid_from_language(LPCWSTR type, GUID *guid)
{
    HRESULT hres;

    hres = CLSIDFromProgID(type, guid);
    if(FAILED(hres))
        return FALSE;

    /* FIXME: Check CATID_ActiveScriptParse */

    return TRUE;
}

static BOOL get_script_guid(HTMLInnerWindow *window, nsIDOMHTMLScriptElement *nsscript, GUID *guid)
{
    const PRUnichar *language;
    nsAString val_str;
    BOOL ret = FALSE;
    nsresult nsres;

    static const PRUnichar languageW[] = {'l','a','n','g','u','a','g','e',0};

    nsAString_Init(&val_str, NULL);

    nsres = nsIDOMHTMLScriptElement_GetType(nsscript, &val_str);
    if(NS_SUCCEEDED(nsres)) {
        const PRUnichar *type;

        nsAString_GetData(&val_str, &type);
        if(*type) {
            ret = get_guid_from_type(type, guid);
            nsAString_Finish(&val_str);
            return ret;
        }
    }else {
        ERR("GetType failed: %08x\n", nsres);
    }

    nsres = get_elem_attr_value((nsIDOMHTMLElement*)nsscript, languageW, &val_str, &language);
    if(NS_SUCCEEDED(nsres)) {
        if(*language) {
            ret = get_guid_from_language(language, guid);
        }else {
            *guid = get_default_script_guid(window);
            ret = TRUE;
        }
        nsAString_Finish(&val_str);
    }

    return ret;
}

static ScriptHost *get_script_host(HTMLInnerWindow *window, const GUID *guid)
{
    ScriptHost *iter;

    LIST_FOR_EACH_ENTRY(iter, &window->script_hosts, ScriptHost, entry) {
        if(IsEqualGUID(guid, &iter->guid))
            return iter;
    }

    return create_script_host(window, guid);
}

static ScriptHost *get_elem_script_host(HTMLInnerWindow *window, HTMLScriptElement *script_elem)
{
    GUID guid;

    if(!get_script_guid(window, script_elem->nsscript, &guid)) {
        WARN("Could not find script GUID\n");
        return NULL;
    }

    if(IsEqualGUID(&CLSID_JScript, &guid)
       && (!window->base.outer_window || window->base.outer_window->scriptmode != SCRIPTMODE_ACTIVESCRIPT)) {
        TRACE("Ignoring JScript\n");
        return NULL;
    }

    return get_script_host(window, &guid);
}

void doc_insert_script(HTMLInnerWindow *window, HTMLScriptElement *script_elem)
{
    ScriptHost *script_host;

    script_host = get_elem_script_host(window, script_elem);
    if(!script_host)
        return;

    if(script_host->parse)
        parse_script_elem(script_host, script_elem);
}

IDispatch *script_parse_event(HTMLInnerWindow *window, LPCWSTR text)
{
    ScriptHost *script_host;
    GUID guid;
    const WCHAR *ptr;
    IDispatch *disp;
    HRESULT hres;

    static const WCHAR delimiterW[] = {'\"',0};

    TRACE("%s\n", debugstr_w(text));

    for(ptr = text; isalnumW(*ptr); ptr++);
    if(*ptr == ':') {
        LPWSTR language;
        BOOL b;

        language = heap_alloc((ptr-text+1)*sizeof(WCHAR));
        if(!language)
            return NULL;

        memcpy(language, text, (ptr-text)*sizeof(WCHAR));
        language[ptr-text] = 0;

        b = get_guid_from_language(language, &guid);

        heap_free(language);

        if(!b) {
            WARN("Could not find language\n");
            return NULL;
        }

        ptr++;
    }else {
        ptr = text;
        guid = get_default_script_guid(window);
    }

    if(IsEqualGUID(&CLSID_JScript, &guid)
       && (!window->base.outer_window || window->base.outer_window->scriptmode != SCRIPTMODE_ACTIVESCRIPT)) {
        TRACE("Ignoring JScript\n");
        return NULL;
    }

    script_host = get_script_host(window, &guid);
    if(!script_host || !script_host->parse_proc)
        return NULL;

    hres = IActiveScriptParseProcedure2_ParseProcedureText(script_host->parse_proc, ptr, NULL, emptyW,
            NULL, NULL, delimiterW, 0 /* FIXME */, 0,
            SCRIPTPROC_HOSTMANAGESSOURCE|SCRIPTPROC_IMPLICIT_THIS|SCRIPTPROC_IMPLICIT_PARENTS, &disp);
    if(FAILED(hres)) {
        WARN("ParseProcedureText failed: %08x\n", hres);
        return NULL;
    }

    TRACE("ret %p\n", disp);
    return disp;
}

HRESULT exec_script(HTMLInnerWindow *window, const WCHAR *code, const WCHAR *lang, VARIANT *ret)
{
    ScriptHost *script_host;
    EXCEPINFO ei;
    GUID guid;
    HRESULT hres;

    static const WCHAR delimW[] = {'"',0};

    if(!get_guid_from_language(lang, &guid)) {
        WARN("Could not find script GUID\n");
        return CO_E_CLASSSTRING;
    }

    script_host = get_script_host(window, &guid);
    if(!script_host) {
        FIXME("No script host\n");
        return E_FAIL;
    }

    if(!script_host->parse) {
        FIXME("script_host->parse == NULL\n");
        return E_FAIL;
    }

    memset(&ei, 0, sizeof(ei));
    TRACE(">>>\n");
    hres = IActiveScriptParse_ParseScriptText(script_host->parse, code, NULL, NULL, delimW, 0, 0, SCRIPTTEXT_ISVISIBLE, ret, &ei);
    if(SUCCEEDED(hres))
        TRACE("<<<\n");
    else
        WARN("<<< %08x\n", hres);

    return hres;
}

IDispatch *get_script_disp(ScriptHost *script_host)
{
    IDispatch *disp;
    HRESULT hres;

    if(!script_host->script)
        return NULL;

    hres = IActiveScript_GetScriptDispatch(script_host->script, windowW, &disp);
    if(FAILED(hres))
        return NULL;

    return disp;
}

static event_target_t **find_event_target(HTMLDocumentNode *doc, HTMLScriptElement *script_elem, HTMLDOMNode **ret_target_node)
{
    HTMLDOMNode *target_node = NULL;
    event_target_t **target = NULL;
    const PRUnichar *target_id;
    nsAString target_id_str;
    nsresult nsres;
    HRESULT hres;

    nsAString_Init(&target_id_str, NULL);
    nsres = nsIDOMHTMLScriptElement_GetHtmlFor(script_elem->nsscript, &target_id_str);
    if(NS_FAILED(nsres)) {
        ERR("GetScriptFor failed: %08x\n", nsres);
        nsAString_Finish(&target_id_str);
        return NULL;
    }

    nsAString_GetData(&target_id_str, &target_id);
    if(!*target_id) {
        FIXME("Empty for attribute\n");
    }else if(!strcmpW(target_id, documentW)) {
        target = &doc->node.event_target;
        target_node = &doc->node;
        IHTMLDOMNode_AddRef(&target_node->IHTMLDOMNode_iface);
    }else if(!strcmpW(target_id, windowW)) {
        target = &doc->body_event_target;
    }else {
        HTMLElement *target_elem;

        hres = get_doc_elem_by_id(doc, target_id, &target_elem);
        if(SUCCEEDED(hres) && target_elem) {
            target_node = &target_elem->node;
            target = &target_elem->node.event_target;
        }
    }
    nsAString_Finish(&target_id_str);

    *ret_target_node = target_node;
    return target;
}

static BOOL parse_event_str(WCHAR *event, const WCHAR **args)
{
    WCHAR *ptr;

    TRACE("%s\n", debugstr_w(event));

    for(ptr = event; isalnumW(*ptr); ptr++);
    if(!*ptr) {
        *args = NULL;
        return TRUE;
    }

    if(*ptr != '(')
        return FALSE;

    *ptr++ = 0;
    *args = ptr;
    while(isalnumW(*ptr) || isspaceW(*ptr) || *ptr == ',')
        ptr++;

    if(*ptr != ')')
        return FALSE;

    *ptr++ = 0;
    return !*ptr;
}

static IDispatch *parse_event_elem(HTMLDocumentNode *doc, HTMLScriptElement *script_elem, WCHAR **ret_event)
{
    ScriptHost *script_host;
    WCHAR *event = NULL;
    const WCHAR *args;
    nsAString nsstr;
    IDispatch *disp;
    nsresult nsres;
    HRESULT hres;

    if(script_elem->parsed)
        return NULL;

    script_host = get_elem_script_host(doc->window, script_elem);
    if(!script_host || !script_host->parse_proc)
        return NULL;

    nsAString_Init(&nsstr, NULL);
    nsres = nsIDOMHTMLScriptElement_GetEvent(script_elem->nsscript, &nsstr);
    if(NS_SUCCEEDED(nsres)) {
        const PRUnichar *event_val;

        nsAString_GetData(&nsstr, &event_val);
        event = heap_strdupW(event_val);
    }
    nsAString_Finish(&nsstr);
    if(!event)
        return NULL;

    if(!parse_event_str(event, &args)) {
        WARN("parsing %s failed\n", debugstr_w(event));
        heap_free(event);
        return NULL;
    }

    nsAString_Init(&nsstr, NULL);
    nsres = nsIDOMHTMLScriptElement_GetText(script_elem->nsscript, &nsstr);
    if(NS_SUCCEEDED(nsres)) {
        const PRUnichar *text;

        nsAString_GetData(&nsstr, &text);
        hres = IActiveScriptParseProcedure2_ParseProcedureText(script_host->parse_proc, text, args,
                emptyW, NULL, NULL, script_endW, 0, 0,
                SCRIPTPROC_HOSTMANAGESSOURCE|SCRIPTPROC_IMPLICIT_THIS|SCRIPTPROC_IMPLICIT_PARENTS, &disp);
        if(FAILED(hres))
            disp = NULL;
    }else {
        ERR("GetText failed: %08x\n", nsres);
        disp = NULL;
    }
    nsAString_Finish(&nsstr);
    if(!disp) {
        heap_free(event);
        return NULL;
    }

    *ret_event = event;
    return disp;
}

void bind_event_scripts(HTMLDocumentNode *doc)
{
    HTMLPluginContainer *plugin_container;
    nsIDOMHTMLScriptElement *nsscript;
    HTMLScriptElement *script_elem;
    event_target_t **event_target;
    nsIDOMNodeList *node_list;
    HTMLDOMNode *target_node;
    nsIDOMNode *script_node;
    nsAString selector_str;
    IDispatch *event_disp;
    UINT32 length, i;
    WCHAR *event;
    nsresult nsres;
    HRESULT hres;

    static const PRUnichar selectorW[] = {'s','c','r','i','p','t','[','e','v','e','n','t',']',0};

    TRACE("%p\n", doc);

    if(!doc->nsdoc)
        return;

    nsAString_InitDepend(&selector_str, selectorW);
    nsres = nsIDOMNodeSelector_QuerySelectorAll(doc->nsnode_selector, &selector_str, &node_list);
    nsAString_Finish(&selector_str);
    if(NS_FAILED(nsres)) {
        ERR("QuerySelectorAll failed: %08x\n", nsres);
        return;
    }

    if(!node_list)
        return;

    nsres = nsIDOMNodeList_GetLength(node_list, &length);
    assert(nsres == NS_OK);

    for(i=0; i < length; i++) {
        nsres = nsIDOMNodeList_Item(node_list, i, &script_node);
        if(NS_FAILED(nsres) || !script_node) {
            ERR("Item(%d) failed: %08x\n", i, nsres);
            continue;
        }

        nsres = nsIDOMNode_QueryInterface(script_node, &IID_nsIDOMHTMLScriptElement, (void**)&nsscript);
        assert(nsres == NS_OK);
        nsIDOMNode_Release(script_node);

        hres = script_elem_from_nsscript(doc, nsscript, &script_elem);
        if(FAILED(hres))
            continue;

        event_disp = parse_event_elem(doc, script_elem, &event);
        if(event_disp) {
            event_target = find_event_target(doc, script_elem, &target_node);
            if(event_target) {
                if(target_node)
                    hres = IHTMLDOMNode_QueryInterface(&target_node->IHTMLDOMNode_iface, &IID_HTMLPluginContainer,
                            (void**)&plugin_container);
                else
                    hres = E_NOINTERFACE;

                if(SUCCEEDED(hres))
                    bind_activex_event(doc, plugin_container, event, event_disp);
                else
                    bind_node_event(doc, event_target, target_node, event, event_disp);

                if(target_node)
                    IHTMLDOMNode_Release(&target_node->IHTMLDOMNode_iface);
            }

            heap_free(event);
            IDispatch_Release(event_disp);
        }

        IHTMLScriptElement_Release(&script_elem->IHTMLScriptElement_iface);
    }

    nsIDOMNodeList_Release(node_list);
}

BOOL find_global_prop(HTMLInnerWindow *window, BSTR name, DWORD flags, ScriptHost **ret_host, DISPID *ret_id)
{
    IDispatchEx *dispex;
    IDispatch *disp;
    ScriptHost *iter;
    HRESULT hres;

    LIST_FOR_EACH_ENTRY(iter, &window->script_hosts, ScriptHost, entry) {
        disp = get_script_disp(iter);
        if(!disp)
            continue;

        hres = IDispatch_QueryInterface(disp, &IID_IDispatchEx, (void**)&dispex);
        if(SUCCEEDED(hres)) {
            hres = IDispatchEx_GetDispID(dispex, name, flags & (~fdexNameEnsure), ret_id);
            IDispatchEx_Release(dispex);
        }else {
            FIXME("No IDispatchEx\n");
            hres = E_NOTIMPL;
        }

        IDispatch_Release(disp);
        if(SUCCEEDED(hres)) {
            *ret_host = iter;
            return TRUE;
        }
    }

    return FALSE;
}

static BOOL is_jscript_available(void)
{
    static BOOL available, checked;

    if(!checked) {
        IUnknown *unk;
        HRESULT hres = CoGetClassObject(&CLSID_JScript, CLSCTX_INPROC_SERVER, NULL, &IID_IUnknown, (void**)&unk);

        if(SUCCEEDED(hres)) {
            available = TRUE;
            IUnknown_Release(unk);
        }else {
            available = FALSE;
        }
        checked = TRUE;
    }

    return available;
}

void set_script_mode(HTMLOuterWindow *window, SCRIPTMODE mode)
{
    nsIWebBrowserSetup *setup;
    nsresult nsres;

    if(mode == SCRIPTMODE_ACTIVESCRIPT && !is_jscript_available()) {
        TRACE("jscript.dll not available\n");
        window->scriptmode = SCRIPTMODE_GECKO;
        return;
    }

    window->scriptmode = mode;

    if(!window->doc_obj->nscontainer || !window->doc_obj->nscontainer->webbrowser)
        return;

    nsres = nsIWebBrowser_QueryInterface(window->doc_obj->nscontainer->webbrowser,
            &IID_nsIWebBrowserSetup, (void**)&setup);
    if(NS_SUCCEEDED(nsres)) {
        nsres = nsIWebBrowserSetup_SetProperty(setup, SETUP_ALLOW_JAVASCRIPT,
                window->scriptmode == SCRIPTMODE_GECKO);

        if(NS_SUCCEEDED(nsres))
            nsres = nsIWebBrowserSetup_SetProperty(setup, SETUP_DISABLE_NOSCRIPT, TRUE);

        nsIWebBrowserSetup_Release(setup);
    }

    if(NS_FAILED(nsres))
        ERR("JavaScript setup failed: %08x\n", nsres);
}

void release_script_hosts(HTMLInnerWindow *window)
{
    script_queue_entry_t *queue_iter;
    ScriptHost *iter;

    while(!list_empty(&window->script_queue)) {
        queue_iter = LIST_ENTRY(list_head(&window->script_queue), script_queue_entry_t, entry);

        list_remove(&queue_iter->entry);
        IHTMLScriptElement_Release(&queue_iter->script->IHTMLScriptElement_iface);
        heap_free(queue_iter);
    }

    while(!list_empty(&window->script_hosts)) {
        iter = LIST_ENTRY(list_head(&window->script_hosts), ScriptHost, entry);

        release_script_engine(iter);
        list_remove(&iter->entry);
        iter->window = NULL;
        IActiveScriptSite_Release(&iter->IActiveScriptSite_iface);
    }
}
