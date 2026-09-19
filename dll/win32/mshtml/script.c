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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "activscp.h"
#include "activdbg.h"
#include "shlwapi.h"

#include "wine/debug.h"

#include "mshtml_private.h"
#include "htmlscript.h"
#include "pluginhost.h"
#include "htmlevent.h"
#include "binding.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

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

/* See jscript.h in jscript.dll. */
#define SCRIPTLANGUAGEVERSION_HTML 0x400
#define SCRIPTLANGUAGEVERSION_ES5  0x102
#define SCRIPTLANGUAGEVERSION_ES6  0x103

struct ScriptHost {
    IActiveScriptSite              IActiveScriptSite_iface;
    IActiveScriptSiteInterruptPoll IActiveScriptSiteInterruptPoll_iface;
    IActiveScriptSiteWindow        IActiveScriptSiteWindow_iface;
    IActiveScriptSiteUIControl     IActiveScriptSiteUIControl_iface;
    IActiveScriptSiteDebug         IActiveScriptSiteDebug_iface;
    IServiceProvider               IServiceProvider_iface;
    IOleCommandTarget              IOleCommandTarget_iface;

    LONG ref;

    IActiveScript *script;
    IActiveScriptParse *parse;
    IActiveScriptParseProcedure2 *parse_proc;

    SCRIPTSTATE script_state;

    HTMLInnerWindow *window;
    IWineJSDispatch *script_jsdisp;

    GUID guid;
    struct list entry;
};

static ScriptHost *get_elem_script_host(HTMLInnerWindow*,HTMLScriptElement*);

static BOOL set_script_prop(IActiveScript *script, DWORD property, VARIANT *val)
{
    IActiveScriptProperty *script_prop;
    HRESULT hres;

    hres = IActiveScript_QueryInterface(script, &IID_IActiveScriptProperty,
            (void**)&script_prop);
    if(FAILED(hres)) {
        WARN("Could not get IActiveScriptProperty iface: %08lx\n", hres);
        return FALSE;
    }

    hres = IActiveScriptProperty_SetProperty(script_prop, property, NULL, val);
    IActiveScriptProperty_Release(script_prop);
    if(FAILED(hres)) {
        WARN("SetProperty(%lx) failed: %08lx\n", property, hres);
        return FALSE;
    }

    return TRUE;
}

static BOOL init_script_engine(ScriptHost *script_host, IActiveScript *script)
{
    compat_mode_t compat_mode;
    IObjectSafety *safety;
    SCRIPTSTATE state;
    DWORD supported_opts=0, enabled_opts=0, script_mode;
    VARIANT var;
    HRESULT hres;

    hres = IActiveScript_QueryInterface(script, &IID_IActiveScriptParse, (void**)&script_host->parse);
    if(FAILED(hres)) {
        WARN("Could not get IActiveScriptParse: %08lx\n", hres);
        return FALSE;
    }

    hres = IActiveScript_QueryInterface(script, &IID_IObjectSafety, (void**)&safety);
    if(FAILED(hres)) {
        FIXME("Could not get IObjectSafety: %08lx\n", hres);
        return FALSE;
    }

    hres = IObjectSafety_GetInterfaceSafetyOptions(safety, &IID_IActiveScriptParse, &supported_opts, &enabled_opts);
    if(FAILED(hres)) {
        FIXME("GetInterfaceSafetyOptions failed: %08lx\n", hres);
    }else if(!(supported_opts & INTERFACE_USES_DISPEX)) {
        FIXME("INTERFACE_USES_DISPEX is not supported\n");
    }else {
        hres = IObjectSafety_SetInterfaceSafetyOptions(safety, &IID_IActiveScriptParse,
                INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER,
                INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER);
        if(FAILED(hres))
            FIXME("SetInterfaceSafetyOptions failed: %08lx\n", hres);
    }

    IObjectSafety_Release(safety);
    if(FAILED(hres))
        return FALSE;

    compat_mode = lock_document_mode(script_host->window->doc);
    script_mode = compat_mode < COMPAT_MODE_IE8 ? SCRIPTLANGUAGEVERSION_5_7 : SCRIPTLANGUAGEVERSION_5_8;
    if(IsEqualGUID(&script_host->guid, &CLSID_JScript)) {
        if(compat_mode >= COMPAT_MODE_IE11)
            script_mode = SCRIPTLANGUAGEVERSION_ES6;
        else if(compat_mode >= COMPAT_MODE_IE9)
            script_mode = SCRIPTLANGUAGEVERSION_ES5;
        script_mode |= SCRIPTLANGUAGEVERSION_HTML;
    }
    V_VT(&var) = VT_I4;
    V_I4(&var) = script_mode;
    if(!set_script_prop(script, SCRIPTPROP_INVOKEVERSIONING, &var) && (script_mode & SCRIPTLANGUAGEVERSION_HTML)) {
        /* If this failed, we're most likely using native jscript. */
        WARN("Failed to set script mode to HTML version.\n");
        V_I4(&var) = compat_mode < COMPAT_MODE_IE8 ? SCRIPTLANGUAGEVERSION_5_7 : SCRIPTLANGUAGEVERSION_5_8;
        set_script_prop(script, SCRIPTPROP_INVOKEVERSIONING, &var);
    }

    V_VT(&var) = VT_BOOL;
    V_BOOL(&var) = VARIANT_TRUE;
    set_script_prop(script, SCRIPTPROP_HACK_TRIDENTEVENTSINK, &var);

    hres = IActiveScriptParse_InitNew(script_host->parse);
    if(FAILED(hres)) {
        WARN("InitNew failed: %08lx\n", hres);
        return FALSE;
    }

    hres = IActiveScript_SetScriptSite(script, &script_host->IActiveScriptSite_iface);
    if(FAILED(hres)) {
        WARN("SetScriptSite failed: %08lx\n", hres);
        IActiveScript_Close(script);
        return FALSE;
    }

    hres = IActiveScript_GetScriptState(script, &state);
    if(FAILED(hres))
        WARN("GetScriptState failed: %08lx\n", hres);
    else if(state != SCRIPTSTATE_INITIALIZED)
        FIXME("state = %x\n", state);

    hres = IActiveScript_SetScriptState(script, SCRIPTSTATE_STARTED);
    if(FAILED(hres)) {
        WARN("Starting script failed: %08lx\n", hres);
        return FALSE;
    }

    if(compat_mode >= COMPAT_MODE_IE9 && IsEqualGUID(&CLSID_JScript, &script_host->guid)) {
        IWineJScript *jscript;
        hres = IActiveScript_QueryInterface(script, &IID_IWineJScript, (void **)&jscript);
        if(SUCCEEDED(hres)) {
            DispatchEx *prototype;

            assert(!script_host->window->jscript);
            assert(!script_host->window->event_target.dispex.jsdisp);
            script_host->window->jscript = jscript;

            hres = get_prototype(script_host->window, PROT_Window, &prototype);
            if(SUCCEEDED(hres))
                hres = IWineJScript_InitHostObject(jscript,
                                                   &script_host->window->event_target.dispex.IWineJSDispatchHost_iface,
                                                   prototype->jsdisp, object_descriptors[PROT_Window]->js_flags,
                                                   &script_host->window->event_target.dispex.jsdisp);
            if(FAILED(hres))
                ERR("Could not initialize script global: %08lx\n", hres);

            /* make sure that script global is fully initialized */
            dispex_compat_mode(&script_host->window->event_target.dispex);
        }else {
            ERR("Could not get IWineJScript, don't use native jscript.dll\n");
        }
    }

    hres = IActiveScript_AddNamedItem(script, L"window",
            SCRIPTITEM_ISVISIBLE|SCRIPTITEM_ISSOURCE|SCRIPTITEM_GLOBALMEMBERS);
    if(SUCCEEDED(hres)) {
        ScriptHost *first_host;

        V_VT(&var) = VT_BOOL;
        V_BOOL(&var) = VARIANT_TRUE;

        LIST_FOR_EACH_ENTRY(first_host, &script_host->window->script_hosts, ScriptHost, entry) {
            if(first_host->script) {
                V_BOOL(&var) = VARIANT_FALSE;
                break;
            }
        }
        set_script_prop(script, SCRIPTPROP_ABBREVIATE_GLOBALNAME_RESOLUTION, &var);

        /* if this was second engine initialized, also set it to first engine, since it used to be TRUE */
        if(!V_BOOL(&var)) {
            struct list *iter = &first_host->entry;
            BOOL is_second_init = TRUE;

            while((iter = list_next(&script_host->window->script_hosts, iter))) {
                if(LIST_ENTRY(iter, ScriptHost, entry)->script) {
                    is_second_init = FALSE;
                    break;
                }
            }
            if(is_second_init)
                set_script_prop(first_host->script, SCRIPTPROP_ABBREVIATE_GLOBALNAME_RESOLUTION, &var);
        }

        if(IsEqualGUID(&script_host->guid, &CLSID_JScript)) {
            IDispatch *script_disp;

            hres = IActiveScript_GetScriptDispatch(script, NULL, &script_disp);
            if(FAILED(hres))
                WARN("GetScriptDispatch failed: %08lx\n", hres);
            else {
                IDispatch_QueryInterface(script_disp, &IID_IWineJSDispatch, (void**)&script_host->script_jsdisp);
                IDispatch_Release(script_disp);
            }
        }
    }else {
       WARN("AddNamedItem failed: %08lx\n", hres);
    }

    hres = IActiveScript_QueryInterface(script, &IID_IActiveScriptParseProcedure2,
                                        (void**)&script_host->parse_proc);
    if(FAILED(hres)) {
        /* FIXME: QI for IActiveScriptParseProcedure */
        WARN("Could not get IActiveScriptParseProcedure iface: %08lx\n", hres);
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
        unlink_ref(&This->parse_proc);
        unlink_ref(&This->parse);
    }

    if(This->script_jsdisp)
        IWineJSDispatch_Release(This->script_jsdisp);
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
    }else if(IsEqualGUID(&IID_IOleCommandTarget, riid)) {
        TRACE("(%p)->(IID_IOleCommandTarget %p)\n", This, ppv);
        *ppv = &This->IOleCommandTarget_iface;
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

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI ActiveScriptSite_Release(IActiveScriptSite *iface)
{
    ScriptHost *This = impl_from_IActiveScriptSite(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref) {
        release_script_engine(This);
        if(This->window)
            list_remove(&This->entry);
        free(This);
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

    TRACE("(%p)->(%s %lx %p %p)\n", This, debugstr_w(pstrName), dwReturnMask, ppiunkItem, ppti);

    if(dwReturnMask != SCRIPTINFO_IUNKNOWN) {
        FIXME("Unsupported mask %lx\n", dwReturnMask);
        return E_NOTIMPL;
    }

    *ppiunkItem = NULL;

    if(wcscmp(pstrName, L"window"))
        return DISP_E_MEMBERNOTFOUND;

    if(!This->window || !This->window->base.outer_window)
        return E_FAIL;

    if(dispex_compat_mode(&This->window->event_target.dispex) >= COMPAT_MODE_IE9)
        *ppiunkItem = (IUnknown*)This->window->event_target.dispex.jsdisp;
    else
        *ppiunkItem = (IUnknown*)&This->window->base.outer_window->base.IHTMLWindow2_iface;
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

    if(!This->window || !This->window->base.outer_window)
        return E_UNEXPECTED;

    *phwnd = This->window->base.outer_window->browser->doc->hwnd;
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
    FIXME("(%p)->(%s %lu %lu %p)\n", This, wine_dbgstr_longlong(dwSourceContext), uCharacterOffset,
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

    if(!This->window || !This->window->doc)
        return E_NOINTERFACE;

    return IServiceProvider_QueryService(&This->window->doc->IServiceProvider_iface, guidService, riid, ppv);
}

static const IServiceProviderVtbl ASServiceProviderVtbl = {
    ASServiceProvider_QueryInterface,
    ASServiceProvider_AddRef,
    ASServiceProvider_Release,
    ASServiceProvider_QueryService
};

static inline ScriptHost *impl_from_IOleCommandTarget(IOleCommandTarget *iface)
{
    return CONTAINING_RECORD(iface, ScriptHost, IOleCommandTarget_iface);
}

static HRESULT WINAPI OleCommandTarget_QueryInterface(IOleCommandTarget *iface, REFIID riid, void **ppv)
{
    ScriptHost *This = impl_from_IOleCommandTarget(iface);
    return IActiveScriptSite_QueryInterface(&This->IActiveScriptSite_iface, riid, ppv);
}

static ULONG WINAPI OleCommandTarget_AddRef(IOleCommandTarget *iface)
{
    ScriptHost *This = impl_from_IOleCommandTarget(iface);
    return IActiveScriptSite_AddRef(&This->IActiveScriptSite_iface);
}

static ULONG WINAPI OleCommandTarget_Release(IOleCommandTarget *iface)
{
    ScriptHost *This = impl_from_IOleCommandTarget(iface);
    return IActiveScriptSite_Release(&This->IActiveScriptSite_iface);
}

static HRESULT WINAPI OleCommandTarget_QueryStatus(IOleCommandTarget *iface, const GUID *pguidCmdGroup,
        ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT *pCmdText)
{
    ScriptHost *This = impl_from_IOleCommandTarget(iface);

    FIXME("(%p)->(%s %ld %p %p)\n", This, debugstr_guid(pguidCmdGroup), cCmds, prgCmds, pCmdText);

    return E_NOTIMPL;
}

static HRESULT WINAPI OleCommandTarget_Exec(IOleCommandTarget *iface, const GUID *pguidCmdGroup,
        DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    ScriptHost *This = impl_from_IOleCommandTarget(iface);

    TRACE("(%p)->(%s %ld %ld %s %p)\n", This, debugstr_guid(pguidCmdGroup), nCmdID, nCmdexecopt, wine_dbgstr_variant(pvaIn), pvaOut);

    if(!pguidCmdGroup) {
        FIXME("Unsupported pguidCmdGroup %s\n", debugstr_guid(pguidCmdGroup));
        return OLECMDERR_E_UNKNOWNGROUP;
    }

    if(IsEqualGUID(&CGID_ScriptSite, pguidCmdGroup)) {
        switch(nCmdID) {
        case CMDID_SCRIPTSITE_SECURITY_WINDOW:
            if(!This->window || !This->window->base.outer_window) {
                FIXME("No window\n");
                return E_FAIL;
            }
            V_VT(pvaOut) = VT_DISPATCH;
            V_DISPATCH(pvaOut) = (IDispatch*)&This->window->base.outer_window->base.IHTMLWindow2_iface;
            IDispatch_AddRef(V_DISPATCH(pvaOut));
            break;

        default:
            FIXME("Unsupported nCmdID %ld of CGID_ScriptSite group\n", nCmdID);
            return OLECMDERR_E_NOTSUPPORTED;
        }
        return S_OK;
    }

    FIXME("Unsupported pguidCmdGroup %s\n", debugstr_guid(pguidCmdGroup));
    return OLECMDERR_E_UNKNOWNGROUP;
}

static const IOleCommandTargetVtbl OleCommandTargetVtbl = {
    OleCommandTarget_QueryInterface,
    OleCommandTarget_AddRef,
    OleCommandTarget_Release,
    OleCommandTarget_QueryStatus,
    OleCommandTarget_Exec
};

static ScriptHost *create_script_host(HTMLInnerWindow *window, const GUID *guid)
{
    IActiveScript *script;
    ScriptHost *ret;
    HRESULT hres;

    ret = calloc(1, sizeof(*ret));
    if(!ret)
        return NULL;

    ret->IActiveScriptSite_iface.lpVtbl = &ActiveScriptSiteVtbl;
    ret->IActiveScriptSiteInterruptPoll_iface.lpVtbl = &ActiveScriptSiteInterruptPollVtbl;
    ret->IActiveScriptSiteWindow_iface.lpVtbl = &ActiveScriptSiteWindowVtbl;
    ret->IActiveScriptSiteUIControl_iface.lpVtbl = &ActiveScriptSiteUIControlVtbl;
    ret->IActiveScriptSiteDebug_iface.lpVtbl = &ActiveScriptSiteDebugVtbl;
    ret->IServiceProvider_iface.lpVtbl = &ASServiceProviderVtbl;
    ret->IOleCommandTarget_iface.lpVtbl = &OleCommandTargetVtbl;
    ret->ref = 1;
    ret->window = window;
    ret->script_state = SCRIPTSTATE_UNINITIALIZED;

    ret->guid = *guid;
    list_add_tail(&window->script_hosts, &ret->entry);

    hres = CoCreateInstance(&ret->guid, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IActiveScript, (void**)&script);
    if(FAILED(hres))
        WARN("Could not load script engine: %08lx\n", hres);
    else {
        BOOL succeeded = init_script_engine(ret, script);
        ret->script = script;
        if(!succeeded)
            release_script_engine(ret);
    }

    return ret;
}

static void dispatch_script_readystatechange_event(HTMLScriptElement *script)
{
    DOMEvent *event;
    HRESULT hres;

    if(script->readystate != READYSTATE_LOADED ||
       dispex_compat_mode(&script->element.node.event_target.dispex) < COMPAT_MODE_IE10) {
        hres = create_document_event(script->element.node.doc, EVENTID_READYSTATECHANGE, &event);
        if(SUCCEEDED(hres)) {
            dispatch_event(&script->element.node.event_target, event);
            IDOMEvent_Release(&event->IDOMEvent_iface);
        }
    }

    if(script->readystate == READYSTATE_LOADED) {
        hres = create_document_event(script->element.node.doc, EVENTID_LOAD, &event);
        if(SUCCEEDED(hres)) {
            dispatch_event(&script->element.node.event_target, event);
            IDOMEvent_Release(&event->IDOMEvent_iface);
        }
    }
}

typedef struct {
    event_task_t header;
    HTMLScriptElement *elem;
} fire_readystatechange_task_t;

static void fire_readystatechange_proc(event_task_t *_task)
{
    fire_readystatechange_task_t *task = (fire_readystatechange_task_t*)_task;

    if(!task->elem->pending_readystatechange_event)
        return;

    task->elem->pending_readystatechange_event = FALSE;
    dispatch_script_readystatechange_event(task->elem);
}

static void fire_readystatechange_task_destr(event_task_t *_task)
{
    fire_readystatechange_task_t *task = (fire_readystatechange_task_t*)_task;

    IHTMLScriptElement_Release(&task->elem->IHTMLScriptElement_iface);
}

static void set_script_elem_readystate(HTMLScriptElement *script_elem, READYSTATE readystate)
{
    script_elem->readystate = readystate;

    if(readystate != READYSTATE_LOADED &&
       dispex_compat_mode(&script_elem->element.node.event_target.dispex) >= COMPAT_MODE_IE11)
        return;

    if(readystate != READYSTATE_INTERACTIVE) {
        if(!script_elem->element.node.doc->window->parser_callback_cnt) {
            fire_readystatechange_task_t *task;
            HRESULT hres;

            if(script_elem->pending_readystatechange_event)
                return;

            task = malloc(sizeof(*task));
            if(!task)
                return;

            IHTMLScriptElement_AddRef(&script_elem->IHTMLScriptElement_iface);
            task->elem = script_elem;

            hres = push_event_task(&task->header, script_elem->element.node.doc->window, fire_readystatechange_proc,
                    fire_readystatechange_task_destr, script_elem->element.node.doc->window->task_magic);
            if(SUCCEEDED(hres))
                script_elem->pending_readystatechange_event = TRUE;
        }else {
            script_elem->pending_readystatechange_event = FALSE;
            dispatch_script_readystatechange_event(script_elem);
        }
    }
}

static void parse_elem_text(ScriptHost *script_host, HTMLScriptElement *script_elem, LPCWSTR text)
{
    EXCEPINFO excepinfo;
    VARIANT var;
    HRESULT hres;

    TRACE("%s\n", debugstr_w(text));

    VariantInit(&var);
    memset(&excepinfo, 0, sizeof(excepinfo));
    TRACE(">>>\n");
    hres = IActiveScriptParse_ParseScriptText(script_host->parse, text, L"window", NULL, L"</SCRIPT>",
                                              0, 0, SCRIPTTEXT_ISVISIBLE|SCRIPTTEXT_HOSTMANAGESSOURCE,
                                              &var, &excepinfo);
    if(SUCCEEDED(hres))
        TRACE("<<<\n");
    else
        WARN("<<< %08lx\n", hres);
}

typedef struct {
    BSCallback bsc;

    HTMLScriptElement *script_elem;
    nsILoadGroup *load_group;
    nsIRequest *request;
    DWORD scheme;

    DWORD size;
    char *buf;
} ScriptBSC;

static HRESULT get_binding_text(ScriptBSC *bsc, WCHAR **ret)
{
    UINT cp = CP_UTF8;
    WCHAR *text;

    if(!bsc->bsc.read) {
        text = malloc(sizeof(WCHAR));
        if(!text)
            return E_OUTOFMEMORY;
        *text = 0;
        *ret = text;
        return S_OK;
    }

    switch(bsc->bsc.bom) {
    case BOM_UTF16:
        if(bsc->bsc.read % sizeof(WCHAR)) {
            FIXME("The buffer is not a valid utf16 string\n");
            return E_FAIL;
        }

        text = malloc(bsc->bsc.read + sizeof(WCHAR));
        if(!text)
            return E_OUTOFMEMORY;

        memcpy(text, bsc->buf, bsc->bsc.read);
        text[bsc->bsc.read/sizeof(WCHAR)] = 0;
        break;

    default:
        /* FIXME: Try to use charset from HTTP headers first */
        cp = get_document_charset(bsc->script_elem->element.node.doc);
        /* fall through */
    case BOM_UTF8: {
        DWORD len;

        len = MultiByteToWideChar(cp, 0, bsc->buf, bsc->bsc.read, NULL, 0);
        text = malloc((len + 1) * sizeof(WCHAR));
        if(!text)
            return E_OUTOFMEMORY;

        MultiByteToWideChar(cp, 0, bsc->buf, bsc->bsc.read, text, len);
        text[len] = 0;
    }
    }

    *ret = text;
    return S_OK;
}

static void script_file_available(ScriptBSC *bsc)
{
    HTMLScriptElement *script_elem = bsc->script_elem;
    HTMLInnerWindow *window = bsc->bsc.window;
    ScriptHost *script_host;
    nsIDOMNode *parent;
    nsresult nsres;
    HRESULT hres;

    assert(window != NULL);

    hres = get_binding_text(bsc, &script_elem->src_text);
    if(FAILED(hres))
        return;

    script_host = get_elem_script_host(window, script_elem);
    if(!script_host)
        return;

    if(window->parser_callback_cnt) {
        script_queue_entry_t *queue;

        TRACE("Adding to queue\n");

        queue = malloc(sizeof(*queue));
        if(!queue)
            return;

        IHTMLScriptElement_AddRef(&script_elem->IHTMLScriptElement_iface);
        queue->script = script_elem;

        list_add_tail(&window->script_queue, &queue->entry);
        return;
    }

    nsres = nsIDOMElement_GetParentNode(script_elem->element.dom_element, &parent);
    if(NS_FAILED(nsres) || !parent) {
        TRACE("No parent, not executing\n");
        script_elem->parse_on_bind = TRUE;
        return;
    }

    nsIDOMNode_Release(parent);

    script_host = get_elem_script_host(window, script_elem);
    if(!script_host)
        return;

    script_elem->parsed = TRUE;
    if(script_host->parse)
        parse_elem_text(script_host, script_elem, script_elem->src_text);
}

static inline ScriptBSC *impl_from_BSCallback(BSCallback *iface)
{
    return CONTAINING_RECORD(iface, ScriptBSC, bsc);
}

static void ScriptBSC_destroy(BSCallback *bsc)
{
    ScriptBSC *This = impl_from_BSCallback(bsc);

    if(This->script_elem) {
        IHTMLScriptElement_Release(&This->script_elem->IHTMLScriptElement_iface);
        This->script_elem = NULL;
    }

    if(This->request) {
        ERR("Unfinished request\n");
        nsIRequest_Release(This->request);
    }
    if(This->load_group)
        nsILoadGroup_Release(This->load_group);

    free(This->buf);
    free(This);
}

static HRESULT ScriptBSC_init_bindinfo(BSCallback *bsc)
{
    return S_OK;
}

static HRESULT ScriptBSC_start_binding(BSCallback *bsc)
{
    ScriptBSC *This = impl_from_BSCallback(bsc);
    nsresult nsres;

    This->script_elem->binding = &This->bsc;

    if(This->load_group) {
        nsres = create_onload_blocker_request(&This->request);
        if(NS_SUCCEEDED(nsres)) {
            nsres = nsILoadGroup_AddRequest(This->load_group, This->request, NULL);
            if(NS_FAILED(nsres))
                ERR("AddRequest failed: %08lx\n", nsres);
        }
    }

    /* FIXME: We should find a better to decide if 'loading' state is supposed to be used by the protocol. */
    if(This->scheme == URL_SCHEME_HTTPS || This->scheme == URL_SCHEME_HTTP)
        set_script_elem_readystate(This->script_elem, READYSTATE_LOADING);

    return S_OK;
}

static HRESULT ScriptBSC_stop_binding(BSCallback *bsc, HRESULT result)
{
    ScriptBSC *This = impl_from_BSCallback(bsc);
    nsresult nsres;

    if(SUCCEEDED(result) && !This->script_elem)
        result = E_UNEXPECTED;

    assert(FAILED(result) || This->script_elem->binding == &This->bsc);
    This->script_elem->binding = NULL;

    if(This->script_elem->readystate == READYSTATE_LOADING)
        set_script_elem_readystate(This->script_elem, READYSTATE_LOADED);

    if(SUCCEEDED(result)) {
        script_file_available(This);
    }else {
        FIXME("binding failed %08lx\n", result);
        free(This->buf);
        This->buf = NULL;
        This->size = 0;
    }

    if(This->request) {
        nsres = nsILoadGroup_RemoveRequest(This->load_group, This->request, NULL, NS_OK);
        if(NS_FAILED(nsres))
            ERR("RemoveRequest failed: %08lx\n", nsres);
        nsIRequest_Release(This->request);
        This->request = NULL;
    }

    IHTMLScriptElement_Release(&This->script_elem->IHTMLScriptElement_iface);
    This->script_elem = NULL;
    return S_OK;
}

static HRESULT ScriptBSC_read_data(BSCallback *bsc, IStream *stream)
{
    ScriptBSC *This = impl_from_BSCallback(bsc);
    DWORD read;
    HRESULT hres;

    if(!This->buf) {
        This->buf = malloc(128);
        if(!This->buf)
            return E_OUTOFMEMORY;
        This->size = 128;
    }

    do {
        if(This->bsc.read >= This->size) {
            void *new_buf;
            new_buf = realloc(This->buf, This->size << 1);
            if(!new_buf)
                return E_OUTOFMEMORY;
            This->size <<= 1;
            This->buf = new_buf;
        }

        hres = read_stream(&This->bsc, stream, This->buf+This->bsc.read, This->size-This->bsc.read, &read);
    }while(hres == S_OK);

    return S_OK;
}

static HRESULT ScriptBSC_on_progress(BSCallback *bsc, ULONG progress, ULONG total, ULONG status_code, LPCWSTR status_text)
{
    return S_OK;
}

static HRESULT ScriptBSC_on_response(BSCallback *bsc, DWORD response_code,
        LPCWSTR response_headers)
{
    return S_OK;
}

static HRESULT ScriptBSC_beginning_transaction(BSCallback *bsc, WCHAR **additional_headers)
{
    return S_FALSE;
}

static const BSCallbackVtbl ScriptBSCVtbl = {
    ScriptBSC_destroy,
    ScriptBSC_init_bindinfo,
    ScriptBSC_start_binding,
    ScriptBSC_stop_binding,
    ScriptBSC_read_data,
    ScriptBSC_on_progress,
    ScriptBSC_on_response,
    ScriptBSC_beginning_transaction
};


HRESULT load_script(HTMLScriptElement *script_elem, const WCHAR *src, BOOL async)
{
    HTMLInnerWindow *window;
    ScriptBSC *bsc;
    IMoniker *mon;
    IUri *uri;
    HRESULT hres;

    static const WCHAR wine_schemaW[] = {'w','i','n','e',':'};

    if(lstrlenW(src) > ARRAY_SIZE(wine_schemaW) && !memcmp(src, wine_schemaW, sizeof(wine_schemaW)))
        src += ARRAY_SIZE(wine_schemaW);

    TRACE("(%p %s %x)\n", script_elem, debugstr_w(src), async);

    if(!script_elem->element.node.doc || !(window = script_elem->element.node.doc->window)) {
        ERR("no window\n");
        return E_UNEXPECTED;
    }

    hres = create_uri(src, 0, &uri);
    if(FAILED(hres))
        return hres;

    hres = CreateURLMonikerEx2(NULL, uri, &mon, URL_MK_UNIFORM);
    if(FAILED(hres)) {
        IUri_Release(uri);
        return hres;
    }

    bsc = calloc(1, sizeof(*bsc));
    if(!bsc) {
        IMoniker_Release(mon);
        IUri_Release(uri);
        return E_OUTOFMEMORY;
    }

    init_bscallback(&bsc->bsc, &ScriptBSCVtbl, mon, async ? BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA : 0);
    IMoniker_Release(mon);

    hres = IUri_GetScheme(uri, &bsc->scheme);
    IUri_Release(uri);
    if(hres != S_OK)
        bsc->scheme = URL_SCHEME_UNKNOWN;

    IHTMLScriptElement_AddRef(&script_elem->IHTMLScriptElement_iface);
    bsc->script_elem = script_elem;

    if(window->bscallback && window->bscallback->nschannel &&
       window->bscallback->nschannel->load_group) {
        cpp_bool contains;
        nsresult nsres;

        nsres = nsIDOMNode_Contains(script_elem->element.node.doc->node.nsnode,
                                    script_elem->element.node.nsnode, &contains);
        if(NS_SUCCEEDED(nsres) && contains) {
            TRACE("script %p will block load event\n", script_elem);
            bsc->load_group = window->bscallback->nschannel->load_group;
            nsILoadGroup_AddRef(bsc->load_group);
        }
    }

    hres = start_binding(window, &bsc->bsc, NULL);

    IBindStatusCallback_Release(&bsc->bsc.IBindStatusCallback_iface);
    return hres;
}

static void parse_inline_script(ScriptHost *script_host, HTMLScriptElement *script_elem)
{
    const PRUnichar *text;
    nsAString text_str;
    nsresult nsres;

    nsAString_Init(&text_str, NULL);
    nsres = nsIDOMHTMLScriptElement_GetText(script_elem->nsscript, &text_str);
    nsAString_GetData(&text_str, &text);

    set_script_elem_readystate(script_elem, READYSTATE_INTERACTIVE);

    if(NS_FAILED(nsres)) {
        ERR("GetText failed: %08lx\n", nsres);
    }else if(*text) {
        parse_elem_text(script_host, script_elem, text);
    }

    nsAString_Finish(&text_str);
}

static BOOL parse_script_elem(ScriptHost *script_host, HTMLScriptElement *script_elem)
{
    nsAString src_str, event_str;
    BOOL is_complete = FALSE;
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
            return FALSE;
        }
    }else {
        ERR("GetEvent failed: %08lx\n", nsres);
    }
    nsAString_Finish(&event_str);

    nsAString_Init(&src_str, NULL);
    nsres = nsIDOMHTMLScriptElement_GetSrc(script_elem->nsscript, &src_str);
    nsAString_GetData(&src_str, &src);

    if(NS_FAILED(nsres)) {
        ERR("GetSrc failed: %08lx\n", nsres);
    }else if(*src) {
        load_script(script_elem, src, FALSE);
        is_complete = script_elem->parsed;
    }else {
        parse_inline_script(script_host, script_elem);
        is_complete = TRUE;
    }

    nsAString_Finish(&src_str);

    return is_complete;
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
    /* FIXME: Handle more types */
    if(!wcsicmp(type, L"text/javascript") || !wcsicmp(type, L"text/jscript")) {
        *guid = CLSID_JScript;
    }else if(!wcsicmp(type, L"text/vbscript")) {
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
    nsIDOMElement *nselem;
    const PRUnichar *language;
    nsAString val_str;
    BOOL ret = FALSE;
    nsresult nsres;

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
        ERR("GetType failed: %08lx\n", nsres);
    }

    nsres = nsIDOMHTMLScriptElement_QueryInterface(nsscript, &IID_nsIDOMElement, (void**)&nselem);
    assert(nsres == NS_OK);

    nsres = get_elem_attr_value(nselem, L"language", &val_str, &language);
    nsIDOMElement_Release(nselem);
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

void initialize_script_global(HTMLInnerWindow *script_global)
{
    if(!script_global->base.outer_window)
        return;
    get_script_host(script_global, &CLSID_JScript);
}

static ScriptHost *get_elem_script_host(HTMLInnerWindow *window, HTMLScriptElement *script_elem)
{
    GUID guid;

    if(!get_script_guid(window, script_elem->nsscript, &guid)) {
        WARN("Could not find script GUID\n");
        return NULL;
    }

    if(IsEqualGUID(&CLSID_JScript, &guid) && (!window->doc->browser || window->doc->browser->script_mode != SCRIPTMODE_ACTIVESCRIPT)) {
        TRACE("Ignoring JScript\n");
        return NULL;
    }

    return get_script_host(window, &guid);
}

void doc_insert_script(HTMLInnerWindow *window, HTMLScriptElement *script_elem, BOOL from_parser)
{
    ScriptHost *script_host;
    BOOL is_complete = FALSE;

    script_host = get_elem_script_host(window, script_elem);
    if(!script_host)
        return;

    if(script_host->parse) {
        if(script_elem->src_text) {
            if(from_parser)
                set_script_elem_readystate(script_elem, READYSTATE_INTERACTIVE);
            script_elem->parsed = TRUE;
            parse_elem_text(script_host, script_elem, script_elem->src_text);
            is_complete = TRUE;
        }else if(!script_elem->binding) {
            is_complete = parse_script_elem(script_host, script_elem);
        }
    }

    if(is_complete)
        set_script_elem_readystate(script_elem, READYSTATE_COMPLETE);
}

IDispatch *script_parse_event(HTMLInnerWindow *window, LPCWSTR text)
{
    ScriptHost *script_host;
    GUID guid;
    const WCHAR *ptr;
    IDispatch *disp;
    HRESULT hres;

    TRACE("%s\n", debugstr_w(text));

    for(ptr = text; iswalnum(*ptr); ptr++);
    if(*ptr == ':') {
        LPWSTR language;
        BOOL b;

        language = malloc((ptr - text + 1) * sizeof(WCHAR));
        if(!language)
            return NULL;

        memcpy(language, text, (ptr-text)*sizeof(WCHAR));
        language[ptr-text] = 0;

        b = get_guid_from_language(language, &guid);

        free(language);

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
       && (!window->doc->browser || window->doc->browser->script_mode != SCRIPTMODE_ACTIVESCRIPT)) {
        TRACE("Ignoring JScript\n");
        return NULL;
    }

    script_host = get_script_host(window, &guid);
    if(!script_host || !script_host->parse_proc)
        return NULL;

    hres = IActiveScriptParseProcedure2_ParseProcedureText(script_host->parse_proc, ptr, NULL, L"",
            NULL, NULL, L"\"", 0 /* FIXME */, 0,
            SCRIPTPROC_HOSTMANAGESSOURCE|SCRIPTPROC_IMPLICIT_THIS|SCRIPTPROC_IMPLICIT_PARENTS, &disp);
    if(FAILED(hres)) {
        WARN("ParseProcedureText failed: %08lx\n", hres);
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
    hres = IActiveScriptParse_ParseScriptText(script_host->parse, code, NULL, NULL, L"\"", 0, 0, SCRIPTTEXT_ISVISIBLE, ret, &ei);
    if(SUCCEEDED(hres))
        TRACE("<<<\n");
    else
        WARN("<<< %08lx\n", hres);

    return hres;
}

IDispatch *get_script_disp(ScriptHost *script_host)
{
    IDispatch *disp;
    HRESULT hres;

    if(!script_host->script)
        return NULL;

    hres = IActiveScript_GetScriptDispatch(script_host->script, L"window", &disp);
    if(FAILED(hres))
        return NULL;

    return disp;
}

IWineJSDispatch *get_script_jsdisp(ScriptHost *script_host)
{
    return script_host->script_jsdisp;
}

IActiveScriptSite *get_first_script_site(HTMLInnerWindow *window)
{
    if(list_empty(&window->script_hosts)) {
        ScriptHost *script_host = create_script_host(window, &CLSID_JScript);
        if(!script_host)
            return NULL;
    }
    return &LIST_ENTRY(list_head(&window->script_hosts), ScriptHost, entry)->IActiveScriptSite_iface;
}

static EventTarget *find_event_target(HTMLDocumentNode *doc, HTMLScriptElement *script_elem)
{
    EventTarget *event_target = NULL;
    const PRUnichar *target_id;
    nsAString target_id_str;
    nsresult nsres;
    HRESULT hres;

    nsAString_Init(&target_id_str, NULL);
    nsres = nsIDOMHTMLScriptElement_GetHtmlFor(script_elem->nsscript, &target_id_str);
    if(NS_FAILED(nsres)) {
        ERR("GetScriptFor failed: %08lx\n", nsres);
        nsAString_Finish(&target_id_str);
        return NULL;
    }

    nsAString_GetData(&target_id_str, &target_id);
    if(!*target_id) {
        FIXME("Empty for attribute\n");
    }else if(!wcscmp(target_id, L"document")) {
        event_target = &doc->node.event_target;
        IHTMLDOMNode_AddRef(&doc->node.IHTMLDOMNode_iface);
    }else if(!wcscmp(target_id, L"window")) {
        if(doc->window) {
            event_target = &doc->window->event_target;
            IWineJSDispatchHost_AddRef(&event_target->dispex.IWineJSDispatchHost_iface);
        }
    }else {
        HTMLElement *target_elem;

        hres = get_doc_elem_by_id(doc, target_id, &target_elem);
        if(SUCCEEDED(hres) && target_elem) {
            event_target = &target_elem->node.event_target;
        }
    }

    nsAString_Finish(&target_id_str);
    return event_target;
}

static BOOL parse_event_str(WCHAR *event, const WCHAR **args)
{
    WCHAR *ptr;

    TRACE("%s\n", debugstr_w(event));

    for(ptr = event; iswalnum(*ptr); ptr++);
    if(!*ptr) {
        *args = NULL;
        return TRUE;
    }

    if(*ptr != '(')
        return FALSE;

    *ptr++ = 0;
    *args = ptr;
    while(iswalnum(*ptr) || iswspace(*ptr) || *ptr == ',')
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
        event = wcsdup(event_val);
    }
    nsAString_Finish(&nsstr);
    if(!event)
        return NULL;

    if(!parse_event_str(event, &args)) {
        WARN("parsing %s failed\n", debugstr_w(event));
        free(event);
        return NULL;
    }

    nsAString_Init(&nsstr, NULL);
    nsres = nsIDOMHTMLScriptElement_GetText(script_elem->nsscript, &nsstr);
    if(NS_SUCCEEDED(nsres)) {
        const PRUnichar *text;

        nsAString_GetData(&nsstr, &text);
        hres = IActiveScriptParseProcedure2_ParseProcedureText(script_host->parse_proc, text, args,
                L"", NULL, NULL, L"</SCRIPT>", 0, 0,
                SCRIPTPROC_HOSTMANAGESSOURCE|SCRIPTPROC_IMPLICIT_THIS|SCRIPTPROC_IMPLICIT_PARENTS, &disp);
        if(FAILED(hres))
            disp = NULL;
    }else {
        ERR("GetText failed: %08lx\n", nsres);
        disp = NULL;
    }
    nsAString_Finish(&nsstr);
    if(!disp) {
        free(event);
        return NULL;
    }

    *ret_event = event;
    return disp;
}

void bind_event_scripts(HTMLDocumentNode *doc)
{
    HTMLPluginContainer *plugin_container;
    nsIDOMHTMLScriptElement *nsscript;
    nsIDOMNodeList *node_list = NULL;
    HTMLScriptElement *script_elem;
    EventTarget *event_target;
    nsIDOMNode *script_node;
    nsAString selector_str;
    IDispatch *event_disp;
    UINT32 length, i;
    WCHAR *event;
    nsresult nsres;
    HRESULT hres;

    TRACE("%p\n", doc);

    if(!doc->dom_document)
        return;

    nsAString_InitDepend(&selector_str, L"script[event]");
    nsres = nsIDOMDocument_QuerySelectorAll(doc->dom_document, &selector_str, &node_list);
    nsAString_Finish(&selector_str);
    if(NS_FAILED(nsres)) {
        ERR("QuerySelectorAll failed: %08lx\n", nsres);
        if(node_list)
            nsIDOMNodeList_Release(node_list);
        return;
    }

    if(!node_list)
        return;

    nsres = nsIDOMNodeList_GetLength(node_list, &length);
    assert(nsres == NS_OK);

    for(i=0; i < length; i++) {
        nsres = nsIDOMNodeList_Item(node_list, i, &script_node);
        if(NS_FAILED(nsres) || !script_node) {
            ERR("Item(%d) failed: %08lx\n", i, nsres);
            continue;
        }

        nsres = nsIDOMNode_QueryInterface(script_node, &IID_nsIDOMHTMLScriptElement, (void**)&nsscript);
        assert(nsres == NS_OK);
        nsIDOMNode_Release(script_node);

        hres = script_elem_from_nsscript(nsscript, &script_elem);
        nsIDOMHTMLScriptElement_Release(nsscript);
        if(FAILED(hres))
            continue;

        event_disp = parse_event_elem(doc, script_elem, &event);
        if(event_disp) {
            event_target = find_event_target(doc, script_elem);
            if(event_target) {
                hres = IWineJSDispatchHost_QueryInterface(&event_target->dispex.IWineJSDispatchHost_iface, &IID_HTMLPluginContainer,
                        (void**)&plugin_container);
                if(SUCCEEDED(hres))
                    bind_activex_event(doc, plugin_container, event, event_disp);
                else
                    bind_target_event(doc, event_target, event, event_disp);

                IWineJSDispatchHost_Release(&event_target->dispex.IWineJSDispatchHost_iface);
                if(plugin_container)
                    node_release(&plugin_container->element.node);
            }

            free(event);
            IDispatch_Release(event_disp);
        }

        IHTMLScriptElement_Release(&script_elem->IHTMLScriptElement_iface);
    }

    nsIDOMNodeList_Release(node_list);
}

BOOL find_global_prop(HTMLInnerWindow *window, const WCHAR *name, DWORD flags, ScriptHost **ret_host, DISPID *ret_id)
{
    IDispatchEx *dispex;
    IDispatch *disp;
    ScriptHost *iter;
    BSTR str;
    HRESULT hres;

    if(!(str = SysAllocString(name)))
        return E_OUTOFMEMORY;

    LIST_FOR_EACH_ENTRY(iter, &window->script_hosts, ScriptHost, entry) {
        disp = get_script_disp(iter);
        if(!disp)
            continue;

        hres = IDispatch_QueryInterface(disp, &IID_IDispatchEx, (void**)&dispex);
        if(SUCCEEDED(hres)) {
            hres = IDispatchEx_GetDispID(dispex, str, flags & (~fdexNameEnsure), ret_id);
            IDispatchEx_Release(dispex);
        }else {
            FIXME("No IDispatchEx\n");
            hres = E_NOTIMPL;
        }

        IDispatch_Release(disp);
        if(SUCCEEDED(hres)) {
            SysFreeString(str);
            *ret_host = iter;
            return TRUE;
        }
    }

    SysFreeString(str);
    return FALSE;
}

HRESULT global_prop_still_exists(HTMLInnerWindow *window, global_prop_t *prop)
{
    HRESULT hres;

    switch(prop->type) {
    case GLOBAL_SCRIPTVAR: {
        DWORD properties;

        if(!prop->script_host->script)
            return E_UNEXPECTED;
        if(!prop->script_host->script_jsdisp)
            return S_OK;
        return IWineJSDispatch_GetMemberProperties(prop->script_host->script_jsdisp, prop->id, 0, &properties);
    }
    case GLOBAL_ELEMENTVAR: {
        IHTMLElement *elem;

        hres = IHTMLDocument3_getElementById(&window->doc->IHTMLDocument3_iface, prop->name, &elem);
        if(FAILED(hres))
            return hres;

        if(!elem)
            return DISP_E_MEMBERNOTFOUND;
        IHTMLElement_Release(elem);
        return S_OK;
    }
    case GLOBAL_FRAMEVAR: {
        HTMLOuterWindow *frame;

        hres = get_frame_by_name(window->base.outer_window, prop->name, FALSE, &frame);
        if(FAILED(hres))
            return hres;

        return frame ? S_OK : DISP_E_MEMBERNOTFOUND;
    }
    default:
        break;
    }

    return S_OK;
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

static BOOL use_gecko_script(IUri *uri)
{
    BSTR display_uri;
    DWORD zone;
    HRESULT hres;

    hres = IUri_GetDisplayUri(uri, &display_uri);
    if(FAILED(hres))
        return FALSE;

    hres = IInternetSecurityManager_MapUrlToZone(get_security_manager(), display_uri, &zone, 0);
    if(FAILED(hres)) {
        WARN("Could not map %s to zone: %08lx\n", debugstr_w(display_uri), hres);
        SysFreeString(display_uri);
        return TRUE;
    }

    SysFreeString(display_uri);
    TRACE("zone %ld\n", zone);
    return zone == URLZONE_UNTRUSTED;
}

void update_browser_script_mode(GeckoBrowser *browser, IUri *uri)
{
    nsIWebBrowserSetup *setup;
    nsresult nsres;

    if(!is_jscript_available()) {
        TRACE("jscript.dll not available\n");
        browser->script_mode = SCRIPTMODE_GECKO;
        return;
    }

    browser->script_mode = use_gecko_script(uri) ? SCRIPTMODE_GECKO : SCRIPTMODE_ACTIVESCRIPT;

    nsres = nsIWebBrowser_QueryInterface(browser->webbrowser, &IID_nsIWebBrowserSetup, (void**)&setup);
    if(NS_SUCCEEDED(nsres)) {
        nsres = nsIWebBrowserSetup_SetProperty(setup, SETUP_ALLOW_JAVASCRIPT,
                browser->script_mode == SCRIPTMODE_GECKO);

        if(NS_SUCCEEDED(nsres))
            nsres = nsIWebBrowserSetup_SetProperty(setup, SETUP_DISABLE_NOSCRIPT, TRUE);

        nsIWebBrowserSetup_Release(setup);
    }

    if(NS_FAILED(nsres))
        ERR("JavaScript setup failed: %08lx\n", nsres);
}

void release_script_hosts(HTMLInnerWindow *window)
{
    script_queue_entry_t *queue_iter;
    ScriptHost *iter;

    while(!list_empty(&window->script_queue)) {
        queue_iter = LIST_ENTRY(list_head(&window->script_queue), script_queue_entry_t, entry);

        list_remove(&queue_iter->entry);
        IHTMLScriptElement_Release(&queue_iter->script->IHTMLScriptElement_iface);
        free(queue_iter);
    }

    while(!list_empty(&window->script_hosts)) {
        iter = LIST_ENTRY(list_head(&window->script_hosts), ScriptHost, entry);

        release_script_engine(iter);
        list_remove(&iter->entry);
        iter->window = NULL;
        IActiveScriptSite_Release(&iter->IActiveScriptSite_iface);
    }
}
