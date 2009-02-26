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

#include "config.h"

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "activscp.h"
#include "activdbg.h"
#include "objsafe.h"

#include "wine/debug.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

static const WCHAR windowW[] = {'w','i','n','d','o','w',0};
static const WCHAR emptyW[] = {0};

static const CLSID CLSID_JScript =
    {0xf414c260,0x6ac0,0x11cf,{0xb6,0xd1,0x00,0xaa,0x00,0xbb,0xbb,0x58}};

typedef struct {
    const IActiveScriptSiteVtbl               *lpIActiveScriptSiteVtbl;
    const IActiveScriptSiteInterruptPollVtbl  *lpIActiveScriptSiteInterruptPollVtbl;
    const IActiveScriptSiteWindowVtbl         *lpIActiveScriptSiteWindowVtbl;
    const IActiveScriptSiteDebug32Vtbl        *lpIActiveScriptSiteDebug32Vtbl;

    LONG ref;

    IActiveScript *script;
    IActiveScriptParse *parse;
    IActiveScriptParseProcedure *parse_proc;

    SCRIPTSTATE script_state;

    HTMLDocument *doc;

    GUID guid;
    struct list entry;
} ScriptHost;

#define ACTSCPSITE(x)  ((IActiveScriptSite*)               &(x)->lpIActiveScriptSiteVtbl)
#define ACTSCPPOLL(x)  (&(x)->lpIActiveScriptSiteInterruptPollVtbl)
#define ACTSCPWIN(x)   (&(x)->lpIActiveScriptSiteWindowVtbl)
#define ACTSCPDBG32(x) (&(x)->lpIActiveScriptSiteDebug32Vtbl)

static BOOL init_script_engine(ScriptHost *script_host)
{
    IActiveScriptProperty *property;
    IObjectSafety *safety;
    SCRIPTSTATE state;
    DWORD supported_opts=0, enabled_opts=0;
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

    hres = IActiveScript_QueryInterface(script_host->script, &IID_IActiveScriptProperty, (void**)&property);
    if(SUCCEEDED(hres)) {
        VARIANT var;

        V_VT(&var) = VT_BOOL;
        V_BOOL(&var) = VARIANT_TRUE;
        hres = IActiveScriptProperty_SetProperty(property, SCRIPTPROP_HACK_TRIDENTEVENTSINK, NULL, &var);
        if(FAILED(hres))
            WARN("SetProperty failed: %08x\n", hres);

        IActiveScriptProperty_Release(property);
    }else {
        WARN("Could not get IActiveScriptProperty: %08x\n", hres);
    }

    hres = IActiveScriptParse64_InitNew(script_host->parse);
    if(FAILED(hres)) {
        WARN("InitNew failed: %08x\n", hres);
        return FALSE;
    }

    hres = IActiveScript_SetScriptSite(script_host->script, ACTSCPSITE(script_host));
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
    if(FAILED(hres))
       WARN("AddNamedItem failed: %08x\n", hres);

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
            IUnknown_Release(This->parse_proc);
            This->parse_proc = NULL;
        }

        if(This->parse) {
            IUnknown_Release(This->parse);
            This->parse = NULL;
        }
    }

    IActiveScript_Release(This->script);
    This->script = NULL;
    This->script_state = SCRIPTSTATE_UNINITIALIZED;
}

void connect_scripts(HTMLDocument *doc)
{
    ScriptHost *iter;

    LIST_FOR_EACH_ENTRY(iter, &doc->script_hosts, ScriptHost, entry) {
        if(iter->script_state == SCRIPTSTATE_STARTED)
            IActiveScript_SetScriptState(iter->script, SCRIPTSTATE_CONNECTED);
    }
}

#define ACTSCPSITE_THIS(iface) DEFINE_THIS(ScriptHost, IActiveScriptSite, iface)

static HRESULT WINAPI ActiveScriptSite_QueryInterface(IActiveScriptSite *iface, REFIID riid, void **ppv)
{
    ScriptHost *This = ACTSCPSITE_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = ACTSCPSITE(This);
    }else if(IsEqualGUID(&IID_IActiveScriptSite, riid)) {
        TRACE("(%p)->(IID_IActiveScriptSite %p)\n", This, ppv);
        *ppv = ACTSCPSITE(This);
    }else if(IsEqualGUID(&IID_IActiveScriptSiteInterruptPoll, riid)) {
        TRACE("(%p)->(IID_IActiveScriptSiteInterruprtPoll %p)\n", This, ppv);
        *ppv = ACTSCPPOLL(This);
    }else if(IsEqualGUID(&IID_IActiveScriptSiteWindow, riid)) {
        TRACE("(%p)->(IID_IActiveScriptSiteWindow %p)\n", This, ppv);
        *ppv = ACTSCPWIN(This);
    }else if(IsEqualGUID(&IID_IActiveScriptSiteDebug32, riid)) {
        TRACE("(%p)->(IID_IActiveScriptSiteDebug32 %p)\n", This, ppv);
        *ppv = ACTSCPDBG32(This);
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
    ScriptHost *This = ACTSCPSITE_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI ActiveScriptSite_Release(IActiveScriptSite *iface)
{
    ScriptHost *This = ACTSCPSITE_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        release_script_engine(This);
        if(This->doc)
            list_remove(&This->entry);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI ActiveScriptSite_GetLCID(IActiveScriptSite *iface, LCID *plcid)
{
    ScriptHost *This = ACTSCPSITE_THIS(iface);

    TRACE("(%p)->(%p)\n", This, plcid);

    *plcid = GetUserDefaultLCID();
    return S_OK;
}

static HRESULT WINAPI ActiveScriptSite_GetItemInfo(IActiveScriptSite *iface, LPCOLESTR pstrName,
        DWORD dwReturnMask, IUnknown **ppiunkItem, ITypeInfo **ppti)
{
    ScriptHost *This = ACTSCPSITE_THIS(iface);

    TRACE("(%p)->(%s %x %p %p)\n", This, debugstr_w(pstrName), dwReturnMask, ppiunkItem, ppti);

    if(dwReturnMask != SCRIPTINFO_IUNKNOWN) {
        FIXME("Unsupported mask %x\n", dwReturnMask);
        return E_NOTIMPL;
    }

    *ppiunkItem = NULL;

    if(strcmpW(pstrName, windowW))
        return DISP_E_MEMBERNOTFOUND;

    if(!This->doc)
        return E_FAIL;

    /* FIXME: Return proxy object */
    *ppiunkItem = (IUnknown*)HTMLWINDOW2(This->doc->window);
    IUnknown_AddRef(*ppiunkItem);

    return S_OK;
}

static HRESULT WINAPI ActiveScriptSite_GetDocVersionString(IActiveScriptSite *iface, BSTR *pbstrVersion)
{
    ScriptHost *This = ACTSCPSITE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pbstrVersion);
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_OnScriptTerminate(IActiveScriptSite *iface,
        const VARIANT *pvarResult, const EXCEPINFO *pexcepinfo)
{
    ScriptHost *This = ACTSCPSITE_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, pvarResult, pexcepinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_OnStateChange(IActiveScriptSite *iface, SCRIPTSTATE ssScriptState)
{
    ScriptHost *This = ACTSCPSITE_THIS(iface);

    TRACE("(%p)->(%x)\n", This, ssScriptState);

    This->script_state = ssScriptState;
    return S_OK;
}

static HRESULT WINAPI ActiveScriptSite_OnScriptError(IActiveScriptSite *iface, IActiveScriptError *pscripterror)
{
    ScriptHost *This = ACTSCPSITE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pscripterror);
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_OnEnterScript(IActiveScriptSite *iface)
{
    ScriptHost *This = ACTSCPSITE_THIS(iface);

    TRACE("(%p)->()\n", This);

    return S_OK;
}

static HRESULT WINAPI ActiveScriptSite_OnLeaveScript(IActiveScriptSite *iface)
{
    ScriptHost *This = ACTSCPSITE_THIS(iface);

    TRACE("(%p)->()\n", This);

    return S_OK;
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

#define ACTSCPPOLL_THIS(iface) DEFINE_THIS(ScriptHost, IActiveScriptSiteInterruptPoll, iface)

static HRESULT WINAPI ActiveScriptSiteInterruptPoll_QueryInterface(IActiveScriptSiteInterruptPoll *iface,
        REFIID riid, void **ppv)
{
    ScriptHost *This = ACTSCPPOLL_THIS(iface);
    return IActiveScriptSite_QueryInterface(ACTSCPSITE(This), riid, ppv);
}

static ULONG WINAPI ActiveScriptSiteInterruptPoll_AddRef(IActiveScriptSiteInterruptPoll *iface)
{
    ScriptHost *This = ACTSCPPOLL_THIS(iface);
    return IActiveScriptSite_AddRef(ACTSCPSITE(This));
}

static ULONG WINAPI ActiveScriptSiteInterruptPoll_Release(IActiveScriptSiteInterruptPoll *iface)
{
    ScriptHost *This = ACTSCPPOLL_THIS(iface);
    return IActiveScriptSite_Release(ACTSCPSITE(This));
}

static HRESULT WINAPI ActiveScriptSiteInterruptPoll_QueryContinue(IActiveScriptSiteInterruptPoll *iface)
{
    ScriptHost *This = ACTSCPPOLL_THIS(iface);

    TRACE("(%p)\n", This);

    return S_OK;
}

#undef ACTSCPPOLL_THIS

static const IActiveScriptSiteInterruptPollVtbl ActiveScriptSiteInterruptPollVtbl = {
    ActiveScriptSiteInterruptPoll_QueryInterface,
    ActiveScriptSiteInterruptPoll_AddRef,
    ActiveScriptSiteInterruptPoll_Release,
    ActiveScriptSiteInterruptPoll_QueryContinue
};

#define ACTSCPWIN_THIS(iface) DEFINE_THIS(ScriptHost, IActiveScriptSiteWindow, iface)

static HRESULT WINAPI ActiveScriptSiteWindow_QueryInterface(IActiveScriptSiteWindow *iface,
        REFIID riid, void **ppv)
{
    ScriptHost *This = ACTSCPWIN_THIS(iface);
    return IActiveScriptSite_QueryInterface(ACTSCPSITE(This), riid, ppv);
}

static ULONG WINAPI ActiveScriptSiteWindow_AddRef(IActiveScriptSiteWindow *iface)
{
    ScriptHost *This = ACTSCPWIN_THIS(iface);
    return IActiveScriptSite_AddRef(ACTSCPSITE(This));
}

static ULONG WINAPI ActiveScriptSiteWindow_Release(IActiveScriptSiteWindow *iface)
{
    ScriptHost *This = ACTSCPWIN_THIS(iface);
    return IActiveScriptSite_Release(ACTSCPSITE(This));
}

static HRESULT WINAPI ActiveScriptSiteWindow_GetWindow(IActiveScriptSiteWindow *iface, HWND *phwnd)
{
    ScriptHost *This = ACTSCPWIN_THIS(iface);
    FIXME("(%p)->(%p)\n", This, phwnd);
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSiteWindow_EnableModeless(IActiveScriptSiteWindow *iface, BOOL fEnable)
{
    ScriptHost *This = ACTSCPWIN_THIS(iface);
    FIXME("(%p)->(%x)\n", This, fEnable);
    return E_NOTIMPL;
}

#undef ACTSCPWIN_THIS

static const IActiveScriptSiteWindowVtbl ActiveScriptSiteWindowVtbl = {
    ActiveScriptSiteWindow_QueryInterface,
    ActiveScriptSiteWindow_AddRef,
    ActiveScriptSiteWindow_Release,
    ActiveScriptSiteWindow_GetWindow,
    ActiveScriptSiteWindow_EnableModeless
};

#define ACTSCPDBG32_THIS(iface) DEFINE_THIS(ScriptHost, IActiveScriptSiteDebug32, iface)

static HRESULT WINAPI ActiveScriptSiteDebug32_QueryInterface(IActiveScriptSiteDebug32 *iface,
        REFIID riid, void **ppv)
{
    ScriptHost *This = ACTSCPDBG32_THIS(iface);
    return IActiveScriptSite_QueryInterface(ACTSCPSITE(This), riid, ppv);
}

static ULONG WINAPI ActiveScriptSiteDebug32_AddRef(IActiveScriptSiteDebug32 *iface)
{
    ScriptHost *This = ACTSCPDBG32_THIS(iface);
    return IActiveScriptSite_AddRef(ACTSCPSITE(This));
}

static ULONG WINAPI ActiveScriptSiteDebug32_Release(IActiveScriptSiteDebug32 *iface)
{
    ScriptHost *This = ACTSCPDBG32_THIS(iface);
    return IActiveScriptSite_Release(ACTSCPSITE(This));
}

static HRESULT WINAPI ActiveScriptSiteDebug32_GetDocumentContextFromPosition(IActiveScriptSiteDebug32 *iface,
            DWORD dwSourceContext, ULONG uCharacterOffset, ULONG uNumChars, IDebugDocumentContext **ppsc)
{
    ScriptHost *This = ACTSCPDBG32_THIS(iface);
    FIXME("(%p)->(%x %u %u %p)\n", This, dwSourceContext, uCharacterOffset, uNumChars, ppsc);
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSiteDebug32_GetApplication(IActiveScriptSiteDebug32 *iface, IDebugApplication32 **ppda)
{
    ScriptHost *This = ACTSCPDBG32_THIS(iface);
    FIXME("(%p)->(%p)\n", This, ppda);
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSiteDebug32_GetRootApplicationNode(IActiveScriptSiteDebug32 *iface,
            IDebugApplicationNode **ppdanRoot)
{
    ScriptHost *This = ACTSCPDBG32_THIS(iface);
    FIXME("(%p)->(%p)\n", This, ppdanRoot);
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSiteDebug32_OnScriptErrorDebug(IActiveScriptSiteDebug32 *iface,
            IActiveScriptErrorDebug *pErrorDebug, BOOL *pfEnterDebugger, BOOL *pfCallOnScriptErrorWhenContinuing)
{
    ScriptHost *This = ACTSCPDBG32_THIS(iface);
    FIXME("(%p)->(%p %p %p)\n", This, pErrorDebug, pfEnterDebugger, pfCallOnScriptErrorWhenContinuing);
    return E_NOTIMPL;
}

#undef ACTSCPDBG32_THIS

static const IActiveScriptSiteDebug32Vtbl ActiveScriptSiteDebug32Vtbl = {
    ActiveScriptSiteDebug32_QueryInterface,
    ActiveScriptSiteDebug32_AddRef,
    ActiveScriptSiteDebug32_Release,
    ActiveScriptSiteDebug32_GetDocumentContextFromPosition,
    ActiveScriptSiteDebug32_GetApplication,
    ActiveScriptSiteDebug32_GetRootApplicationNode,
    ActiveScriptSiteDebug32_OnScriptErrorDebug
};

static ScriptHost *create_script_host(HTMLDocument *doc, const GUID *guid)
{
    ScriptHost *ret;
    HRESULT hres;

    ret = heap_alloc_zero(sizeof(*ret));
    ret->lpIActiveScriptSiteVtbl               = &ActiveScriptSiteVtbl;
    ret->lpIActiveScriptSiteInterruptPollVtbl  = &ActiveScriptSiteInterruptPollVtbl;
    ret->lpIActiveScriptSiteWindowVtbl         = &ActiveScriptSiteWindowVtbl;
    ret->lpIActiveScriptSiteDebug32Vtbl        = &ActiveScriptSiteDebug32Vtbl;
    ret->ref = 1;
    ret->doc = doc;
    ret->script_state = SCRIPTSTATE_UNINITIALIZED;

    ret->guid = *guid;
    list_add_tail(&doc->script_hosts, &ret->entry);

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

    static const WCHAR script_endW[] = {'<','/','S','C','R','I','P','T','>',0};

    TRACE("%s\n", debugstr_w(text));

    VariantInit(&var);
    memset(&excepinfo, 0, sizeof(excepinfo));
    hres = IActiveScriptParse64_ParseScriptText(script_host->parse, text, windowW, NULL, script_endW,
                                              0, 0, SCRIPTTEXT_ISVISIBLE|SCRIPTTEXT_HOSTMANAGESSOURCE,
                                              &var, &excepinfo);
    if(FAILED(hres))
        WARN("ParseScriptText failed: %08x\n", hres);

}

static void parse_extern_script(ScriptHost *script_host, LPCWSTR src)
{
    IMoniker *mon;
    char *buf;
    WCHAR *text;
    DWORD len, size=0;
    HRESULT hres;

    static const WCHAR wine_schemaW[] = {'w','i','n','e',':'};

    if(strlenW(src) > sizeof(wine_schemaW)/sizeof(WCHAR) && !memcmp(src, wine_schemaW, sizeof(wine_schemaW)))
        src += sizeof(wine_schemaW)/sizeof(WCHAR);

    hres = CreateURLMoniker(NULL, src, &mon);
    if(FAILED(hres))
        return;

    hres = bind_mon_to_buffer(script_host->doc, mon, (void**)&buf, &size);
    IMoniker_Release(mon);
    if(FAILED(hres))
        return;

    len = MultiByteToWideChar(CP_ACP, 0, buf, size, NULL, 0);
    text = heap_alloc((len+1)*sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, buf, size, text, len);
    heap_free(buf);
    text[len] = 0;

    parse_text(script_host, text);

    heap_free(text);
}

static void parse_inline_script(ScriptHost *script_host, nsIDOMHTMLScriptElement *nsscript)
{
    const PRUnichar *text;
    nsAString text_str;
    nsresult nsres;

    nsAString_Init(&text_str, NULL);

    nsres = nsIDOMHTMLScriptElement_GetText(nsscript, &text_str);

    if(NS_SUCCEEDED(nsres)) {
        nsAString_GetData(&text_str, &text);
        parse_text(script_host, text);
    }else {
        ERR("GetText failed: %08x\n", nsres);
    }

    nsAString_Finish(&text_str);
}

static void parse_script_elem(ScriptHost *script_host, nsIDOMHTMLScriptElement *nsscript)
{
    const PRUnichar *src;
    nsAString src_str;
    nsresult nsres;

    nsAString_Init(&src_str, NULL);

    nsres = nsIDOMHTMLScriptElement_GetSrc(nsscript, &src_str);
    nsAString_GetData(&src_str, &src);

    if(NS_FAILED(nsres))
        ERR("GetSrc failed: %08x\n", nsres);
    else if(*src)
        parse_extern_script(script_host, src);
    else
        parse_inline_script(script_host, nsscript);

    nsAString_Finish(&src_str);
}

static BOOL get_guid_from_type(LPCWSTR type, GUID *guid)
{
    const WCHAR text_javascriptW[] =
        {'t','e','x','t','/','j','a','v','a','s','c','r','i','p','t',0};

    /* FIXME: Handle more types */
    if(!strcmpiW(type, text_javascriptW)) {
        *guid = CLSID_JScript;
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

static BOOL get_script_guid(nsIDOMHTMLScriptElement *nsscript, GUID *guid)
{
    nsAString attr_str, val_str;
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

    nsAString_Init(&attr_str, languageW);

    nsres = nsIDOMHTMLScriptElement_GetAttribute(nsscript, &attr_str, &val_str);
    if(NS_SUCCEEDED(nsres)) {
        const PRUnichar *language;

        nsAString_GetData(&val_str, &language);

        if(*language) {
            ret = get_guid_from_language(language, guid);
        }else {
            *guid = CLSID_JScript;
            ret = TRUE;
        }
    }else {
        ERR("GetAttribute(language) failed: %08x\n", nsres);
    }

    nsAString_Finish(&attr_str);
    nsAString_Finish(&val_str);

    return ret;
}

static ScriptHost *get_script_host(HTMLDocument *doc, const GUID *guid)
{
    ScriptHost *iter;

    if(IsEqualGUID(&CLSID_JScript, guid) && doc->scriptmode != SCRIPTMODE_ACTIVESCRIPT) {
        TRACE("Ignoring JScript\n");
        return NULL;
    }

    LIST_FOR_EACH_ENTRY(iter, &doc->script_hosts, ScriptHost, entry) {
        if(IsEqualGUID(guid, &iter->guid))
            return iter;
    }

    return create_script_host(doc, guid);
}

void doc_insert_script(HTMLDocument *doc, nsIDOMHTMLScriptElement *nsscript)
{
    ScriptHost *script_host;
    GUID guid;

    if(!get_script_guid(nsscript, &guid)) {
        WARN("Could not find script GUID\n");
        return;
    }

    script_host = get_script_host(doc, &guid);
    if(!script_host)
        return;

    if(script_host->parse)
        parse_script_elem(script_host, nsscript);
}

IDispatch *script_parse_event(HTMLDocument *doc, LPCWSTR text)
{
    ScriptHost *script_host;
    GUID guid = CLSID_JScript;
    const WCHAR *ptr;
    IDispatch *disp;
    HRESULT hres;

    static const WCHAR delimiterW[] = {'\"',0};

    for(ptr = text; isalnumW(*ptr); ptr++);
    if(*ptr == ':') {
        LPWSTR language;
        BOOL b;

        language = heap_alloc((ptr-text+1)*sizeof(WCHAR));
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
    }

    script_host = get_script_host(doc, &guid);
    if(!script_host || !script_host->parse_proc)
        return NULL;

    hres = IActiveScriptParseProcedure64_ParseProcedureText(script_host->parse_proc, ptr, NULL, emptyW,
            NULL, NULL, delimiterW, 0 /* FIXME */, 0,
            SCRIPTPROC_HOSTMANAGESSOURCE|SCRIPTPROC_IMPLICIT_THIS|SCRIPTPROC_IMPLICIT_PARENTS, &disp);
    if(FAILED(hres)) {
        WARN("ParseProcedureText failed: %08x\n", hres);
        return NULL;
    }

    TRACE("ret %p\n", disp);
    return disp;
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

void set_script_mode(HTMLDocument *doc, SCRIPTMODE mode)
{
    nsIWebBrowserSetup *setup;
    nsresult nsres;

    if(mode == SCRIPTMODE_ACTIVESCRIPT && !is_jscript_available()) {
        TRACE("jscript.dll not available\n");
        doc->scriptmode = SCRIPTMODE_GECKO;
        return;
    }

    doc->scriptmode = mode;

    if(!doc->nscontainer || !doc->nscontainer->webbrowser)
        return;

    nsres = nsIWebBrowser_QueryInterface(doc->nscontainer->webbrowser,
            &IID_nsIWebBrowserSetup, (void**)&setup);
    if(NS_SUCCEEDED(nsres)) {
        nsres = nsIWebBrowserSetup_SetProperty(setup, SETUP_ALLOW_JAVASCRIPT,
                doc->scriptmode == SCRIPTMODE_GECKO);
        nsIWebBrowserSetup_Release(setup);
    }

    if(NS_FAILED(nsres))
        ERR("JavaScript setup failed: %08x\n", nsres);
}

void release_script_hosts(HTMLDocument *doc)
{
    ScriptHost *iter;

    while(!list_empty(&doc->script_hosts)) {
        iter = LIST_ENTRY(list_head(&doc->script_hosts), ScriptHost, entry);

        release_script_engine(iter);
        list_remove(&iter->entry);
        iter->doc = NULL;
        IActiveScript_Release(ACTSCPSITE(iter));
    }
}
