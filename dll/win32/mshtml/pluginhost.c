/*
 * Copyright 2010 Jacek Caban for CodeWeavers
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
#include <assert.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#ifndef __REACTOS__
#include "shlobj.h"
#endif // __REACTOS__
#include "mshtmdid.h"

#include "mshtml_private.h"
#include "pluginhost.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

static BOOL check_load_safety(PluginHost *host)
{
    DWORD policy_size, policy;
    struct CONFIRMSAFETY cs;
    BYTE *ppolicy;
    HRESULT hres;

    cs.clsid = host->clsid;
    cs.pUnk = host->plugin_unk;
    cs.dwFlags = CONFIRMSAFETYACTION_LOADOBJECT;

    hres = IInternetHostSecurityManager_QueryCustomPolicy(&host->doc->IInternetHostSecurityManager_iface,
            &GUID_CUSTOM_CONFIRMOBJECTSAFETY, &ppolicy, &policy_size, (BYTE*)&cs, sizeof(cs), 0);
    if(FAILED(hres))
        return FALSE;

    policy = *(DWORD*)ppolicy;
    CoTaskMemFree(ppolicy);
    return policy == URLPOLICY_ALLOW;
}

static BOOL check_script_safety(PluginHost *host)
{
    DISPPARAMS params = {NULL,NULL,0,0};
    DWORD policy_size, policy;
    struct CONFIRMSAFETY cs;
    BYTE *ppolicy;
    ULONG err = 0;
    VARIANT v;
    HRESULT hres;

    cs.clsid = host->clsid;
    cs.pUnk = host->plugin_unk;
    cs.dwFlags = 0;

    hres = IInternetHostSecurityManager_QueryCustomPolicy(&host->doc->IInternetHostSecurityManager_iface,
            &GUID_CUSTOM_CONFIRMOBJECTSAFETY, &ppolicy, &policy_size, (BYTE*)&cs, sizeof(cs), 0);
    if(FAILED(hres))
        return FALSE;

    policy = *(DWORD*)ppolicy;
    CoTaskMemFree(ppolicy);

    if(policy != URLPOLICY_ALLOW)
        return FALSE;

    V_VT(&v) = VT_EMPTY;
    hres = IDispatch_Invoke(host->disp, DISPID_SECURITYCTX, &IID_NULL, 0, DISPATCH_PROPERTYGET, &params, &v, NULL, &err);
    if(SUCCEEDED(hres)) {
        FIXME("Handle security ctx %s\n", debugstr_variant(&v));
        return FALSE;
    }

    return TRUE;
}

static void update_readystate(PluginHost *host)
{
    DISPPARAMS params = {NULL,NULL,0,0};
    IDispatchEx *dispex;
    IDispatch *disp;
    ULONG err = 0;
    VARIANT v;
    HRESULT hres;

    hres = IUnknown_QueryInterface(host->plugin_unk, &IID_IDispatchEx, (void**)&dispex);
    if(SUCCEEDED(hres)) {
        FIXME("Use IDispatchEx\n");
        IDispatchEx_Release(dispex);
    }

    hres = IUnknown_QueryInterface(host->plugin_unk, &IID_IDispatch, (void**)&disp);
    if(FAILED(hres))
        return;

    hres = IDispatch_Invoke(disp, DISPID_READYSTATE, &IID_NULL, 0, DISPATCH_PROPERTYGET, &params, &v, NULL, &err);
    IDispatch_Release(disp);
    if(SUCCEEDED(hres)) {
        /* FIXME: make plugin readystate affect document readystate */
        TRACE("readystate = %s\n", debugstr_variant(&v));
        VariantClear(&v);
    }
}

/* FIXME: We shouldn't need this function and we should embed plugin directly in the main document */
static void get_pos_rect(PluginHost *host, RECT *ret)
{
    ret->top = 0;
    ret->left = 0;
    ret->bottom = host->rect.bottom - host->rect.top;
    ret->right = host->rect.right - host->rect.left;
}

static void load_prop_bag(PluginHost *host, IPersistPropertyBag *persist_prop_bag)
{
    IPropertyBag *prop_bag;
    HRESULT hres;

    hres = create_param_prop_bag(host->element->element.nselem, &prop_bag);
    if(FAILED(hres))
        return;

    if(prop_bag && !check_load_safety(host)) {
        IPropertyBag_Release(prop_bag);
        prop_bag = NULL;
    }

    if(prop_bag) {
        hres = IPersistPropertyBag_Load(persist_prop_bag, prop_bag, NULL);
        IPropertyBag_Release(prop_bag);
        if(FAILED(hres))
            WARN("Load failed: %08x\n", hres);
    }else {
        hres = IPersistPropertyBag_InitNew(persist_prop_bag);
        if(FAILED(hres))
            WARN("InitNew failed: %08x\n", hres);
    }
}

static void load_plugin(PluginHost *host)
{
    IPersistPropertyBag2 *persist_prop_bag2;
    IPersistPropertyBag *persist_prop_bag;
    HRESULT hres;

    hres = IUnknown_QueryInterface(host->plugin_unk, &IID_IPersistPropertyBag2, (void**)&persist_prop_bag2);
    if(SUCCEEDED(hres)) {
        FIXME("Use IPersistPropertyBag2 iface\n");
        IPersistPropertyBag2_Release(persist_prop_bag2);
        return;
    }

    hres = IUnknown_QueryInterface(host->plugin_unk, &IID_IPersistPropertyBag, (void**)&persist_prop_bag);
    if(SUCCEEDED(hres)) {
        load_prop_bag(host, persist_prop_bag);
        IPersistPropertyBag_Release(persist_prop_bag);
        return;
    }

    FIXME("No IPersistPropertyBag iface\n");
}

static void initialize_plugin_object(PluginHost *host)
{
    IClientSecurity *client_security;
    IQuickActivate *quick_activate;
    IOleObject *ole_obj = NULL;
    IOleCommandTarget *cmdtrg;
    IViewObjectEx *view_obj;
    IDispatchEx *dispex;
    IDispatch *disp;
    HRESULT hres;

    /* Note native calls QI on plugin for an undocumented IID and CLSID_HTMLDocument */

    /* FIXME: call FreezeEvents(TRUE) */

    hres = IUnknown_QueryInterface(host->plugin_unk, &IID_IClientSecurity, (void**)&client_security);
    if(SUCCEEDED(hres)) {
        FIXME("Handle IClientSecurity\n");
        IClientSecurity_Release(client_security);
        return;
    }

    hres = IUnknown_QueryInterface(host->plugin_unk, &IID_IQuickActivate, (void**)&quick_activate);
    if(SUCCEEDED(hres)) {
        QACONTAINER container = {sizeof(container)};
        QACONTROL control = {sizeof(control)};

        TRACE("Using IQuickActivate\n");

        container.pClientSite = &host->IOleClientSite_iface;
        container.dwAmbientFlags = QACONTAINER_SUPPORTSMNEMONICS|QACONTAINER_MESSAGEREFLECT|QACONTAINER_USERMODE;
        container.pAdviseSink = &host->IAdviseSinkEx_iface;
        container.pPropertyNotifySink = &host->IPropertyNotifySink_iface;

        hres = IQuickActivate_QuickActivate(quick_activate, &container, &control);
        IQuickActivate_Release(quick_activate);
        if(FAILED(hres))
            FIXME("QuickActivate failed: %08x\n", hres);
    }else {
        DWORD status = 0;

        hres = IUnknown_QueryInterface(host->plugin_unk, &IID_IOleObject, (void**)&ole_obj);
        if(SUCCEEDED(hres)) {
            hres = IOleObject_GetMiscStatus(ole_obj, DVASPECT_CONTENT, &status);
            TRACE("GetMiscStatus returned %08x %x\n", hres, status);

            hres = IOleObject_SetClientSite(ole_obj, &host->IOleClientSite_iface);
            IOleObject_Release(ole_obj);
            if(FAILED(hres)) {
                FIXME("SetClientSite failed: %08x\n", hres);
                return;
            }
        }else {
            TRACE("Plugin does not support IOleObject\n");
        }
    }

    load_plugin(host);

    if(ole_obj) {
        hres = IUnknown_QueryInterface(host->plugin_unk, &IID_IViewObjectEx, (void**)&view_obj);
        if(SUCCEEDED(hres)) {
            DWORD view_status = 0;

            hres = IViewObjectEx_SetAdvise(view_obj, DVASPECT_CONTENT, 0, (IAdviseSink*)&host->IAdviseSinkEx_iface);
            if(FAILED(hres))
                WARN("SetAdvise failed: %08x\n", hres);

            hres = IViewObjectEx_GetViewStatus(view_obj, &view_status);
            IViewObjectEx_Release(view_obj);
            TRACE("GetViewStatus returned %08x %x\n", hres, view_status);
        }
    }

    update_readystate(host);

    /* NOTE: Native QIs for IActiveScript, an undocumented IID, IOleControl and IRunnableObject */

    hres = IUnknown_QueryInterface(host->plugin_unk, &IID_IDispatchEx, (void**)&dispex);
    if(SUCCEEDED(hres)) {
        FIXME("Use IDispatchEx\n");
        host->disp = (IDispatch*)dispex;
    }else {
        hres = IUnknown_QueryInterface(host->plugin_unk, &IID_IDispatch, (void**)&disp);
        if(SUCCEEDED(hres))
            host->disp = disp;
        else
            TRACE("no IDispatch iface\n");
    }

    hres = IUnknown_QueryInterface(host->plugin_unk, &IID_IOleCommandTarget, (void**)&cmdtrg);
    if(SUCCEEDED(hres)) {
        FIXME("Use IOleCommandTarget\n");
        IOleCommandTarget_Release(cmdtrg);
    }
}

static void embed_plugin_object(PluginHost *host)
{
    IOleObject *ole_obj;
    RECT rect;
    HRESULT hres;

    hres = IUnknown_QueryInterface(host->plugin_unk, &IID_IOleObject, (void**)&ole_obj);
    if(FAILED(hres)) {
        FIXME("Plugin does not support IOleObject\n");
        return;
    }

    get_pos_rect(host, &rect);
    hres = IOleObject_DoVerb(ole_obj, OLEIVERB_INPLACEACTIVATE, NULL, &host->IOleClientSite_iface, 0, host->hwnd, &rect);
    IOleObject_Release(ole_obj);
    if(FAILED(hres))
        WARN("DoVerb failed: %08x\n", hres);

    if(host->ip_object) {
        HWND hwnd;

        hres = IOleInPlaceObject_GetWindow(host->ip_object, &hwnd);
        if(SUCCEEDED(hres))
            TRACE("hwnd %p\n", hwnd);
    }
}

void update_plugin_window(PluginHost *host, HWND hwnd, const RECT *rect)
{
    BOOL rect_changed = FALSE;

    if(!hwnd || (host->hwnd && host->hwnd != hwnd)) {
        FIXME("unhandled hwnd\n");
        return;
    }

    TRACE("%p %s\n", hwnd, wine_dbgstr_rect(rect));

    if(memcmp(rect, &host->rect, sizeof(RECT))) {
        host->rect = *rect;
        rect_changed = TRUE;
    }

    if(!host->hwnd) {
        host->hwnd = hwnd;
        embed_plugin_object(host);
    }

    if(rect_changed && host->ip_object)
        IOleInPlaceObject_SetObjectRects(host->ip_object, &host->rect, &host->rect);
}

static void notif_enabled(PluginHost *plugin_host)
{
    DISPPARAMS args = {NULL, NULL, 0, 0};
    IDispatch *disp;
    ULONG err = 0;
    VARIANT res;
    HRESULT hres;

    hres = IUnknown_QueryInterface(plugin_host->plugin_unk, &IID_IDispatch, (void**)&disp);
    if(FAILED(hres)) {
        FIXME("Could not get IDispatch iface: %08x\n", hres);
        return;
    }

    V_VT(&res) = VT_EMPTY;
    hres = IDispatch_Invoke(disp, DISPID_ENABLED, &IID_NULL, 0/*FIXME*/, DISPATCH_PROPERTYGET, &args, &res, NULL, &err);
    IDispatch_Release(disp);
    if(SUCCEEDED(hres)) {
        FIXME("Got enabled %s\n", debugstr_variant(&res));
        VariantClear(&res);
    }
}

void notif_container_change(HTMLPluginContainer *plugin_container, DISPID dispid)
{
    IOleControl *ole_control;
    HRESULT hres;

    if(!plugin_container->plugin_host || !plugin_container->plugin_host->plugin_unk)
        return;

    notif_enabled(plugin_container->plugin_host);

    hres = IUnknown_QueryInterface(plugin_container->plugin_host->plugin_unk, &IID_IOleControl, (void**)&ole_control);
    if(SUCCEEDED(hres)) {
        IOleControl_OnAmbientPropertyChange(ole_control, dispid);
        IOleControl_Release(ole_control);
    }
}

HRESULT get_plugin_disp(HTMLPluginContainer *plugin_container, IDispatch **ret)
{
    PluginHost *host;

    host = plugin_container->plugin_host;
    if(!host) {
        ERR("No plugin host\n");
        return E_UNEXPECTED;
    }

    if(!host->disp) {
        *ret = NULL;
        return S_OK;
    }

    if(!check_script_safety(host)) {
        FIXME("Insecure object\n");
        return E_FAIL;
    }

    IDispatch_AddRef(host->disp);
    *ret = host->disp;
    return S_OK;
}

HRESULT get_plugin_dispid(HTMLPluginContainer *plugin_container, WCHAR *name, DISPID *ret)
{
    IDispatch *disp;
    DISPID id;
    DWORD i;
    HRESULT hres;

    if(!plugin_container->plugin_host) {
        WARN("no plugin host\n");
        return DISP_E_UNKNOWNNAME;
    }

    disp = plugin_container->plugin_host->disp;
    if(!disp)
        return DISP_E_UNKNOWNNAME;

    hres = IDispatch_GetIDsOfNames(disp, &IID_NULL, &name, 1, 0, &id);
    if(FAILED(hres)) {
        TRACE("no prop %s\n", debugstr_w(name));
        return DISP_E_UNKNOWNNAME;
    }

    for(i=0; i < plugin_container->props_len; i++) {
        if(id == plugin_container->props[i]) {
            *ret = MSHTML_DISPID_CUSTOM_MIN+i;
            return S_OK;
        }
    }

    if(!plugin_container->props) {
        plugin_container->props = heap_alloc(8*sizeof(DISPID));
        if(!plugin_container->props)
            return E_OUTOFMEMORY;
        plugin_container->props_size = 8;
    }else if(plugin_container->props_len == plugin_container->props_size) {
        DISPID *new_props;

        new_props = heap_realloc(plugin_container->props, plugin_container->props_size*2*sizeof(DISPID));
        if(!new_props)
            return E_OUTOFMEMORY;

        plugin_container->props = new_props;
        plugin_container->props_size *= 2;
    }

    plugin_container->props[plugin_container->props_len] = id;
    *ret = MSHTML_DISPID_CUSTOM_MIN+plugin_container->props_len;
    plugin_container->props_len++;
    return S_OK;
}

HRESULT invoke_plugin_prop(HTMLPluginContainer *plugin_container, DISPID id, LCID lcid, WORD flags, DISPPARAMS *params,
        VARIANT *res, EXCEPINFO *ei)
{
    PluginHost *host;

    host = plugin_container->plugin_host;
    if(!host || !host->disp) {
        FIXME("Called with no disp\n");
        return E_UNEXPECTED;
    }

    if(!check_script_safety(host)) {
        FIXME("Insecure object\n");
        return E_FAIL;
    }

    if(id < MSHTML_DISPID_CUSTOM_MIN || id > MSHTML_DISPID_CUSTOM_MIN + plugin_container->props_len) {
        ERR("Invalid id\n");
        return E_FAIL;
    }

    return IDispatch_Invoke(host->disp, plugin_container->props[id-MSHTML_DISPID_CUSTOM_MIN], &IID_NULL,
            lcid, flags, params, res, ei, NULL);
}

typedef struct {
    DISPID id;
    IDispatch *disp;
} sink_entry_t;

struct PHEventSink {
    IDispatch IDispatch_iface;

    LONG ref;

    PluginHost *host;
    ITypeInfo *typeinfo;
    GUID iid;
    DWORD cookie;
    BOOL is_dispiface;

    sink_entry_t *handlers;
    DWORD handlers_cnt;
    DWORD handlers_size;
};

static sink_entry_t *find_sink_entry(PHEventSink *sink, DISPID id)
{
    sink_entry_t *iter;

    for(iter = sink->handlers; iter < sink->handlers+sink->handlers_cnt; iter++) {
        if(iter->id == id)
            return iter;
    }

    return NULL;
}

static void add_sink_handler(PHEventSink *sink, DISPID id, IDispatch *disp)
{
    sink_entry_t *entry = find_sink_entry(sink, id);

    if(entry) {
        if(entry->disp)
            IDispatch_Release(entry->disp);
    }else {
        if(!sink->handlers_size) {
            sink->handlers = heap_alloc(4*sizeof(*sink->handlers));
            if(!sink->handlers)
                return;
            sink->handlers_size = 4;
        }else if(sink->handlers_cnt == sink->handlers_size) {
            sink_entry_t *new_handlers;

            new_handlers = heap_realloc(sink->handlers, 2*sink->handlers_size*sizeof(*sink->handlers));
            if(!new_handlers)
                return;
            sink->handlers = new_handlers;
            sink->handlers_size *= 2;
        }
        entry = sink->handlers + sink->handlers_cnt++;
        entry->id = id;
    }

    IDispatch_AddRef(disp);
    entry->disp = disp;
}

static inline PHEventSink *PHEventSink_from_IDispatch(IDispatch *iface)
{
    return CONTAINING_RECORD(iface, PHEventSink, IDispatch_iface);
}

static HRESULT WINAPI PHEventSink_QueryInterface(IDispatch *iface, REFIID riid, void **ppv)
{
    PHEventSink *This = PHEventSink_from_IDispatch(iface);

    if(IsEqualGUID(riid, &IID_IUnknown)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IDispatch_iface;
    }else if(IsEqualGUID(riid, &IID_IDispatch)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = &This->IDispatch_iface;
    }else if(This->is_dispiface && IsEqualGUID(riid, &This->iid)) {
        TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = &This->IDispatch_iface;
    }else {
        WARN("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI PHEventSink_AddRef(IDispatch *iface)
{
    PHEventSink *This = PHEventSink_from_IDispatch(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)\n", This);

    return ref;
}

static ULONG WINAPI PHEventSink_Release(IDispatch *iface)
{
    PHEventSink *This = PHEventSink_from_IDispatch(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)\n", This);

    if(!ref) {
        unsigned i;

        assert(!This->host);

        for(i=0; i < This->handlers_cnt; i++) {
            if(This->handlers[i].disp)
                IDispatch_Release(This->handlers[i].disp);
        }
        heap_free(This->handlers);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI PHEventSink_GetTypeInfoCount(IDispatch *iface, UINT *pctinfo)
{
    PHEventSink *This = PHEventSink_from_IDispatch(iface);
    FIXME("(%p)->(%p)\n", This, pctinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHEventSink_GetTypeInfo(IDispatch *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    PHEventSink *This = PHEventSink_from_IDispatch(iface);
    FIXME("(%p)->(%d %d %p)\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHEventSink_GetIDsOfNames(IDispatch *iface, REFIID riid, LPOLESTR *rgszNames,
        UINT cNames, LCID lcid, DISPID *rgDispId)
{
    PHEventSink *This = PHEventSink_from_IDispatch(iface);
    FIXME("(%p)->(%s %p %u %d %p)\n", This, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHEventSink_Invoke(IDispatch *iface, DISPID dispIdMember, REFIID riid, LCID lcid,
        WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    PHEventSink *This = PHEventSink_from_IDispatch(iface);
    IDispatchEx *dispex;
    sink_entry_t *entry;
    HRESULT hres;

    TRACE("(%p)->(%d %s %d %x %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid), lcid, wFlags,
          pDispParams, pVarResult, pExcepInfo, puArgErr);

    if(!This->host) {
        WARN("No host\n");
        return E_UNEXPECTED;
    }

    entry = find_sink_entry(This, dispIdMember);
    if(!entry || !entry->disp) {
        WARN("No handler %d\n", dispIdMember);
        if(pVarResult)
            V_VT(pVarResult) = VT_EMPTY;
        return S_OK;
    }

    hres = IDispatch_QueryInterface(entry->disp, &IID_IDispatchEx, (void**)&dispex);

    TRACE("(%p) %d >>>\n", This, entry->id);
    if(SUCCEEDED(hres)) {
        hres = IDispatchEx_InvokeEx(dispex, DISPID_VALUE, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, NULL);
        IDispatchEx_Release(dispex);
    }else {
        hres = IDispatch_Invoke(entry->disp, DISPID_VALUE, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    }
    if(SUCCEEDED(hres))
        TRACE("(%p) %d <<<\n", This, entry->id);
    else
        WARN("(%p) %d <<< %08x\n", This, entry->id, hres);
    return hres;
}

static const IDispatchVtbl PHCPDispatchVtbl = {
    PHEventSink_QueryInterface,
    PHEventSink_AddRef,
    PHEventSink_Release,
    PHEventSink_GetTypeInfoCount,
    PHEventSink_GetTypeInfo,
    PHEventSink_GetIDsOfNames,
    PHEventSink_Invoke
};

static PHEventSink *create_event_sink(PluginHost *plugin_host, ITypeInfo *typeinfo)
{
    IConnectionPointContainer *cp_container;
    PHEventSink *ret;
    IConnectionPoint *cp;
    TYPEATTR *typeattr;
    TYPEKIND typekind;
    GUID guid;
    HRESULT hres;

    hres = ITypeInfo_GetTypeAttr(typeinfo, &typeattr);
    if(FAILED(hres))
        return NULL;

    typekind = typeattr->typekind;
    guid = typeattr->guid;
    ITypeInfo_ReleaseTypeAttr(typeinfo, typeattr);

    TRACE("guid %s typekind %d\n", debugstr_guid(&guid), typekind);

    if(typekind != TKIND_INTERFACE && typekind != TKIND_DISPATCH) {
        WARN("invalid typekind %d\n", typekind);
        return NULL;
    }

    hres = IUnknown_QueryInterface(plugin_host->plugin_unk, &IID_IConnectionPointContainer, (void**)&cp_container);
    if(FAILED(hres)) {
        WARN("Could not get IConnectionPointContainer iface: %08x\n", hres);
        return NULL;
    }

    hres = IConnectionPointContainer_FindConnectionPoint(cp_container, &guid, &cp);
    IConnectionPointContainer_Release(cp_container);
    if(FAILED(hres)) {
        WARN("Could not find %s connection point\n", debugstr_guid(&guid));
        return NULL;
    }

    ret = heap_alloc_zero(sizeof(*ret));
    if(ret) {
        ret->IDispatch_iface.lpVtbl = &PHCPDispatchVtbl;
        ret->ref = 1;
        ret->host = plugin_host;
        ret->iid = guid;
        ret->is_dispiface = typekind == TKIND_DISPATCH;

        ITypeInfo_AddRef(typeinfo);
        ret->typeinfo = typeinfo;

        hres = IConnectionPoint_Advise(cp, (IUnknown*)&ret->IDispatch_iface, &ret->cookie);
    }else {
        hres = E_OUTOFMEMORY;
    }

    IConnectionPoint_Release(cp);
    if(FAILED(hres)) {
        WARN("Advise failed: %08x\n", hres);
        return NULL;
    }

    return ret;
}

static ITypeInfo *get_eventiface_info(HTMLPluginContainer *plugin_container, ITypeInfo *class_info)
{
    int impl_types, i, impl_flags;
    ITypeInfo *ret = NULL;
    TYPEATTR *typeattr;
    HREFTYPE ref;
    HRESULT hres;

    hres = ITypeInfo_GetTypeAttr(class_info, &typeattr);
    if(FAILED(hres))
        return NULL;

    if(typeattr->typekind != TKIND_COCLASS) {
        WARN("not coclass\n");
        ITypeInfo_ReleaseTypeAttr(class_info, typeattr);
        return NULL;
    }

    impl_types = typeattr->cImplTypes;
    ITypeInfo_ReleaseTypeAttr(class_info, typeattr);

    for(i=0; i<impl_types; i++) {
        hres = ITypeInfo_GetImplTypeFlags(class_info, i, &impl_flags);
        if(FAILED(hres))
            continue;

        if((impl_flags & IMPLTYPEFLAG_FSOURCE)) {
            if(!(impl_flags & IMPLTYPEFLAG_FDEFAULT)) {
                FIXME("Handle non-default source iface\n");
                continue;
            }

            hres = ITypeInfo_GetRefTypeOfImplType(class_info, i, &ref);
            if(FAILED(hres))
                continue;

            hres = ITypeInfo_GetRefTypeInfo(class_info, ref, &ret);
            if(FAILED(hres))
                ret = NULL;
        }
    }

    return ret;
}

void bind_activex_event(HTMLDocumentNode *doc, HTMLPluginContainer *plugin_container, WCHAR *event, IDispatch *disp)
{
    PluginHost *plugin_host = plugin_container->plugin_host;
    ITypeInfo *class_info, *source_info;
    DISPID id;
    HRESULT hres;

    TRACE("(%p %p %s %p)\n", doc, plugin_host, debugstr_w(event), disp);

    if(!plugin_host || !plugin_host->plugin_unk) {
        WARN("detached element %p\n", plugin_host);
        return;
    }

    if(plugin_host->sink) {
        source_info = plugin_host->sink->typeinfo;
        ITypeInfo_AddRef(source_info);
    }else {
        IProvideClassInfo *provide_ci;

        hres = IUnknown_QueryInterface(plugin_host->plugin_unk, &IID_IProvideClassInfo, (void**)&provide_ci);
        if(FAILED(hres)) {
            FIXME("No IProvideClassInfo, try GetTypeInfo?\n");
            return;
        }

        hres = IProvideClassInfo_GetClassInfo(provide_ci, &class_info);
        IProvideClassInfo_Release(provide_ci);
        if(FAILED(hres) || !class_info) {
            WARN("GetClassInfo failed: %08x\n", hres);
            return;
        }

        source_info = get_eventiface_info(plugin_container, class_info);
        ITypeInfo_Release(class_info);
        if(!source_info)
            return;
    }

    hres = ITypeInfo_GetIDsOfNames(source_info, &event, 1, &id);
    if(FAILED(hres))
        WARN("Could not get disp id: %08x\n", hres);
    else if(!plugin_host->sink)
        plugin_host->sink = create_event_sink(plugin_host, source_info);

    ITypeInfo_Release(source_info);
    if(FAILED(hres) || !plugin_host->sink)
        return;

    add_sink_handler(plugin_host->sink, id, disp);
}

static inline PluginHost *impl_from_IOleClientSite(IOleClientSite *iface)
{
    return CONTAINING_RECORD(iface, PluginHost, IOleClientSite_iface);
}

static HRESULT WINAPI PHClientSite_QueryInterface(IOleClientSite *iface, REFIID riid, void **ppv)
{
    PluginHost *This = impl_from_IOleClientSite(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IOleClientSite_iface;
    }else if(IsEqualGUID(&IID_IOleClientSite, riid)) {
        TRACE("(%p)->(IID_IOleClientSite %p)\n", This, ppv);
        *ppv = &This->IOleClientSite_iface;
    }else if(IsEqualGUID(&IID_IAdviseSink, riid)) {
        TRACE("(%p)->(IID_IAdviseSink %p)\n", This, ppv);
        *ppv = &This->IAdviseSinkEx_iface;
    }else if(IsEqualGUID(&IID_IAdviseSinkEx, riid)) {
        TRACE("(%p)->(IID_IAdviseSinkEx %p)\n", This, ppv);
        *ppv = &This->IAdviseSinkEx_iface;
    }else if(IsEqualGUID(&IID_IPropertyNotifySink, riid)) {
        TRACE("(%p)->(IID_IPropertyNotifySink %p)\n", This, ppv);
        *ppv = &This->IPropertyNotifySink_iface;
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = &This->IDispatch_iface;
    }else if(IsEqualGUID(&IID_IOleWindow, riid)) {
        TRACE("(%p)->(IID_IOleWindow %p)\n", This, ppv);
        *ppv = &This->IOleInPlaceSiteEx_iface;
    }else if(IsEqualGUID(&IID_IOleInPlaceSite, riid)) {
        TRACE("(%p)->(IID_IOleInPlaceSite %p)\n", This, ppv);
        *ppv = &This->IOleInPlaceSiteEx_iface;
    }else if(IsEqualGUID(&IID_IOleInPlaceSiteEx, riid)) {
        TRACE("(%p)->(IID_IOleInPlaceSiteEx %p)\n", This, ppv);
        *ppv = &This->IOleInPlaceSiteEx_iface;
    }else if(IsEqualGUID(&IID_IOleControlSite, riid)) {
        TRACE("(%p)->(IID_IOleControlSite %p)\n", This, ppv);
        *ppv = &This->IOleControlSite_iface;
    }else if(IsEqualGUID(&IID_IBindHost, riid)) {
        TRACE("(%p)->(IID_IBindHost %p)\n", This, ppv);
        *ppv = &This->IBindHost_iface;
    }else if(IsEqualGUID(&IID_IServiceProvider, riid)) {
        TRACE("(%p)->(IID_IServiceProvider %p)\n", This, ppv);
        *ppv = &This->IServiceProvider_iface;
    }else {
        WARN("Unsupported interface %s\n", debugstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI PHClientSite_AddRef(IOleClientSite *iface)
{
    PluginHost *This = impl_from_IOleClientSite(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static void release_plugin_ifaces(PluginHost *This)
{
    if(This->disp) {
        IDispatch_Release(This->disp);
        This->disp = NULL;
    }

    if(This->ip_object) {
        IOleInPlaceObject_Release(This->ip_object);
        This->ip_object = NULL;
    }

    if(This->plugin_unk) {
        IUnknown *unk = This->plugin_unk;
        LONG ref;

        This->plugin_unk = NULL;
        ref = IUnknown_Release(unk);

        TRACE("plugin ref = %d\n", ref);
    }
}

static ULONG WINAPI PHClientSite_Release(IOleClientSite *iface)
{
    PluginHost *This = impl_from_IOleClientSite(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        release_plugin_ifaces(This);
        if(This->sink) {
            This->sink->host = NULL;
            IDispatch_Release(&This->sink->IDispatch_iface);
            This->sink = NULL;
        }
        list_remove(&This->entry);
        if(This->element)
            This->element->plugin_host = NULL;
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI PHClientSite_SaveObject(IOleClientSite *iface)
{
    PluginHost *This = impl_from_IOleClientSite(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHClientSite_GetMoniker(IOleClientSite *iface, DWORD dwAssign,
        DWORD dwWhichMoniker, IMoniker **ppmk)
{
    PluginHost *This = impl_from_IOleClientSite(iface);

    TRACE("(%p)->(%d %d %p)\n", This, dwAssign, dwWhichMoniker, ppmk);

    switch(dwWhichMoniker) {
    case OLEWHICHMK_CONTAINER:
        if(!This->doc || !This->doc->window || !This->doc->window->mon) {
            FIXME("no moniker\n");
            return E_UNEXPECTED;
        }

        *ppmk = This->doc->window->mon;
        IMoniker_AddRef(*ppmk);
        break;
    default:
        FIXME("which %d\n", dwWhichMoniker);
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT WINAPI PHClientSite_GetContainer(IOleClientSite *iface, IOleContainer **ppContainer)
{
    PluginHost *This = impl_from_IOleClientSite(iface);

    TRACE("(%p)->(%p)\n", This, ppContainer);

    if(!This->doc) {
        ERR("Called on detached object\n");
        return E_UNEXPECTED;
    }

    *ppContainer = &This->doc->basedoc.IOleContainer_iface;
    IOleContainer_AddRef(*ppContainer);
    return S_OK;
}

static HRESULT WINAPI PHClientSite_ShowObject(IOleClientSite *iface)
{
    PluginHost *This = impl_from_IOleClientSite(iface);

    TRACE("(%p)\n", This);

    return S_OK;
}

static HRESULT WINAPI PHClientSite_OnShowWindow(IOleClientSite *iface, BOOL fShow)
{
    PluginHost *This = impl_from_IOleClientSite(iface);
    FIXME("(%p)->(%x)\n", This, fShow);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHClientSite_RequestNewObjectLayout(IOleClientSite *iface)
{
    PluginHost *This = impl_from_IOleClientSite(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static const IOleClientSiteVtbl OleClientSiteVtbl = {
    PHClientSite_QueryInterface,
    PHClientSite_AddRef,
    PHClientSite_Release,
    PHClientSite_SaveObject,
    PHClientSite_GetMoniker,
    PHClientSite_GetContainer,
    PHClientSite_ShowObject,
    PHClientSite_OnShowWindow,
    PHClientSite_RequestNewObjectLayout
};

static inline PluginHost *impl_from_IAdviseSinkEx(IAdviseSinkEx *iface)
{
    return CONTAINING_RECORD(iface, PluginHost, IAdviseSinkEx_iface);
}

static HRESULT WINAPI PHAdviseSinkEx_QueryInterface(IAdviseSinkEx *iface, REFIID riid, void **ppv)
{
    PluginHost *This = impl_from_IAdviseSinkEx(iface);
    return IOleClientSite_QueryInterface(&This->IOleClientSite_iface, riid, ppv);
}

static ULONG WINAPI PHAdviseSinkEx_AddRef(IAdviseSinkEx *iface)
{
    PluginHost *This = impl_from_IAdviseSinkEx(iface);
    return IOleClientSite_AddRef(&This->IOleClientSite_iface);
}

static ULONG WINAPI PHAdviseSinkEx_Release(IAdviseSinkEx *iface)
{
    PluginHost *This = impl_from_IAdviseSinkEx(iface);
    return IOleClientSite_Release(&This->IOleClientSite_iface);
}

static void WINAPI PHAdviseSinkEx_OnDataChange(IAdviseSinkEx *iface, FORMATETC *pFormatetc, STGMEDIUM *pStgMedium)
{
    PluginHost *This = impl_from_IAdviseSinkEx(iface);
    FIXME("(%p)->(%p %p)\n", This, pFormatetc, pStgMedium);
}

static void WINAPI PHAdviseSinkEx_OnViewChange(IAdviseSinkEx *iface, DWORD dwAspect, LONG lindex)
{
    PluginHost *This = impl_from_IAdviseSinkEx(iface);
    FIXME("(%p)->(%d %d)\n", This, dwAspect, lindex);
}

static void WINAPI PHAdviseSinkEx_OnRename(IAdviseSinkEx *iface, IMoniker *pmk)
{
    PluginHost *This = impl_from_IAdviseSinkEx(iface);
    FIXME("(%p)->(%p)\n", This, pmk);
}

static void WINAPI PHAdviseSinkEx_OnSave(IAdviseSinkEx *iface)
{
    PluginHost *This = impl_from_IAdviseSinkEx(iface);
    FIXME("(%p)\n", This);
}

static void WINAPI PHAdviseSinkEx_OnClose(IAdviseSinkEx *iface)
{
    PluginHost *This = impl_from_IAdviseSinkEx(iface);
    FIXME("(%p)\n", This);
}

static void WINAPI PHAdviseSinkEx_OnViewStatusChange(IAdviseSinkEx *iface, DWORD dwViewStatus)
{
    PluginHost *This = impl_from_IAdviseSinkEx(iface);
    FIXME("(%p)->(%d)\n", This, dwViewStatus);
}

static const IAdviseSinkExVtbl AdviseSinkExVtbl = {
    PHAdviseSinkEx_QueryInterface,
    PHAdviseSinkEx_AddRef,
    PHAdviseSinkEx_Release,
    PHAdviseSinkEx_OnDataChange,
    PHAdviseSinkEx_OnViewChange,
    PHAdviseSinkEx_OnRename,
    PHAdviseSinkEx_OnSave,
    PHAdviseSinkEx_OnClose,
    PHAdviseSinkEx_OnViewStatusChange
};

static inline PluginHost *impl_from_IPropertyNotifySink(IPropertyNotifySink *iface)
{
    return CONTAINING_RECORD(iface, PluginHost, IPropertyNotifySink_iface);
}

static HRESULT WINAPI PHPropertyNotifySink_QueryInterface(IPropertyNotifySink *iface, REFIID riid, void **ppv)
{
    PluginHost *This = impl_from_IPropertyNotifySink(iface);
    return IOleClientSite_QueryInterface(&This->IOleClientSite_iface, riid, ppv);
}

static ULONG WINAPI PHPropertyNotifySink_AddRef(IPropertyNotifySink *iface)
{
    PluginHost *This = impl_from_IPropertyNotifySink(iface);
    return IOleClientSite_AddRef(&This->IOleClientSite_iface);
}

static ULONG WINAPI PHPropertyNotifySink_Release(IPropertyNotifySink *iface)
{
    PluginHost *This = impl_from_IPropertyNotifySink(iface);
    return IOleClientSite_Release(&This->IOleClientSite_iface);
}

static HRESULT WINAPI PHPropertyNotifySink_OnChanged(IPropertyNotifySink *iface, DISPID dispID)
{
    PluginHost *This = impl_from_IPropertyNotifySink(iface);

    TRACE("(%p)->(%d)\n", This, dispID);

    switch(dispID) {
    case DISPID_READYSTATE:
        update_readystate(This);
        break;
    default :
        FIXME("Unimplemented dispID %d\n", dispID);
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT WINAPI PHPropertyNotifySink_OnRequestEdit(IPropertyNotifySink *iface, DISPID dispID)
{
    PluginHost *This = impl_from_IPropertyNotifySink(iface);
    FIXME("(%p)->(%d)\n", This, dispID);
    return E_NOTIMPL;
}

static const IPropertyNotifySinkVtbl PropertyNotifySinkVtbl = {
    PHPropertyNotifySink_QueryInterface,
    PHPropertyNotifySink_AddRef,
    PHPropertyNotifySink_Release,
    PHPropertyNotifySink_OnChanged,
    PHPropertyNotifySink_OnRequestEdit
};

static inline PluginHost *impl_from_IDispatch(IDispatch *iface)
{
    return CONTAINING_RECORD(iface, PluginHost, IDispatch_iface);
}

static HRESULT WINAPI PHDispatch_QueryInterface(IDispatch *iface, REFIID riid, void **ppv)
{
    PluginHost *This = impl_from_IDispatch(iface);
    return IOleClientSite_QueryInterface(&This->IOleClientSite_iface, riid, ppv);
}

static ULONG WINAPI PHDispatch_AddRef(IDispatch *iface)
{
    PluginHost *This = impl_from_IDispatch(iface);
    return IOleClientSite_AddRef(&This->IOleClientSite_iface);
}

static ULONG WINAPI PHDispatch_Release(IDispatch *iface)
{
    PluginHost *This = impl_from_IDispatch(iface);
    return IOleClientSite_Release(&This->IOleClientSite_iface);
}

static HRESULT WINAPI PHDispatch_GetTypeInfoCount(IDispatch *iface, UINT *pctinfo)
{
    PluginHost *This = impl_from_IDispatch(iface);
    FIXME("(%p)->(%p)\n", This, pctinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHDispatch_GetTypeInfo(IDispatch *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    PluginHost *This = impl_from_IDispatch(iface);
    FIXME("(%p)->(%d %d %p)\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHDispatch_GetIDsOfNames(IDispatch *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    PluginHost *This = impl_from_IDispatch(iface);
    FIXME("(%p)->(%s %p %d %d %p)\n", This, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHDispatch_Invoke(IDispatch *iface, DISPID dispid,  REFIID riid, LCID lcid,
        WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    PluginHost *This = impl_from_IDispatch(iface);
    FIXME("(%p)->(%d %x %p %p)\n", This, dispid, wFlags, pDispParams, pVarResult);
    return E_NOTIMPL;
}

static const IDispatchVtbl DispatchVtbl = {
    PHDispatch_QueryInterface,
    PHDispatch_AddRef,
    PHDispatch_Release,
    PHDispatch_GetTypeInfoCount,
    PHDispatch_GetTypeInfo,
    PHDispatch_GetIDsOfNames,
    PHDispatch_Invoke
};

static inline PluginHost *impl_from_IOleInPlaceSiteEx(IOleInPlaceSiteEx *iface)
{
    return CONTAINING_RECORD(iface, PluginHost, IOleInPlaceSiteEx_iface);
}

static HRESULT WINAPI PHInPlaceSite_QueryInterface(IOleInPlaceSiteEx *iface, REFIID riid, void **ppv)
{
    PluginHost *This = impl_from_IOleInPlaceSiteEx(iface);
    return IOleClientSite_QueryInterface(&This->IOleClientSite_iface, riid, ppv);
}

static ULONG WINAPI PHInPlaceSite_AddRef(IOleInPlaceSiteEx *iface)
{
    PluginHost *This = impl_from_IOleInPlaceSiteEx(iface);
    return IOleClientSite_AddRef(&This->IOleClientSite_iface);
}

static ULONG WINAPI PHInPlaceSite_Release(IOleInPlaceSiteEx *iface)
{
    PluginHost *This = impl_from_IOleInPlaceSiteEx(iface);
    return IOleClientSite_Release(&This->IOleClientSite_iface);
}

static HRESULT WINAPI PHInPlaceSite_GetWindow(IOleInPlaceSiteEx *iface, HWND *phwnd)
{
    PluginHost *This = impl_from_IOleInPlaceSiteEx(iface);

    TRACE("(%p)->(%p)\n", This, phwnd);

    *phwnd = This->hwnd;
    return S_OK;
}

static HRESULT WINAPI PHInPlaceSite_ContextSensitiveHelp(IOleInPlaceSiteEx *iface, BOOL fEnterMode)
{
    PluginHost *This = impl_from_IOleInPlaceSiteEx(iface);
    FIXME("(%p)->(%x)\n", This, fEnterMode);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHInPlaceSite_CanInPlaceActivate(IOleInPlaceSiteEx *iface)
{
    PluginHost *This = impl_from_IOleInPlaceSiteEx(iface);

    TRACE("(%p)\n", This);

    return S_OK;
}

static HRESULT WINAPI PHInPlaceSite_OnInPlaceActivate(IOleInPlaceSiteEx *iface)
{
    PluginHost *This = impl_from_IOleInPlaceSiteEx(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHInPlaceSite_OnUIActivate(IOleInPlaceSiteEx *iface)
{
    PluginHost *This = impl_from_IOleInPlaceSiteEx(iface);

    TRACE("(%p)\n", This);

    if(!This->plugin_unk) {
        ERR("No plugin object\n");
        return E_UNEXPECTED;
    }

    This->ui_active = TRUE;

    notif_enabled(This);
    return S_OK;
}

static HRESULT WINAPI PHInPlaceSite_GetWindowContext(IOleInPlaceSiteEx *iface,
        IOleInPlaceFrame **ppFrame, IOleInPlaceUIWindow **ppDoc, RECT *lprcPosRect,
        RECT *lprcClipRect, OLEINPLACEFRAMEINFO *frame_info)
{
    PluginHost *This = impl_from_IOleInPlaceSiteEx(iface);
    IOleInPlaceUIWindow *ip_window;
    IOleInPlaceFrame *ip_frame;
    RECT pr, cr;
    HRESULT hres;

    TRACE("(%p)->(%p %p %p %p %p)\n", This, ppFrame, ppDoc, lprcPosRect, lprcClipRect, frame_info);

    if(!This->doc || !This->doc->basedoc.doc_obj || !This->doc->basedoc.doc_obj->ipsite) {
        FIXME("No ipsite\n");
        return E_UNEXPECTED;
    }

    hres = IOleInPlaceSite_GetWindowContext(This->doc->basedoc.doc_obj->ipsite, &ip_frame, &ip_window, &pr, &cr, frame_info);
    if(FAILED(hres)) {
        WARN("GetWindowContext failed: %08x\n", hres);
        return hres;
    }

    if(ip_window)
        IOleInPlaceUIWindow_Release(ip_window);
    if(ip_frame)
        IOleInPlaceFrame_Release(ip_frame);

    hres = create_ip_frame(&ip_frame);
    if(FAILED(hres))
        return hres;

    hres = create_ip_window(ppDoc);
    if(FAILED(hres)) {
        IOleInPlaceFrame_Release(ip_frame);
        return hres;
    }

    *ppFrame = ip_frame;
    *lprcPosRect = This->rect;
    *lprcClipRect = This->rect;
    return S_OK;
}

static HRESULT WINAPI PHInPlaceSite_Scroll(IOleInPlaceSiteEx *iface, SIZE scrollExtent)
{
    PluginHost *This = impl_from_IOleInPlaceSiteEx(iface);
    FIXME("(%p)->({%d %d})\n", This, scrollExtent.cx, scrollExtent.cy);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHInPlaceSite_OnUIDeactivate(IOleInPlaceSiteEx *iface, BOOL fUndoable)
{
    PluginHost *This = impl_from_IOleInPlaceSiteEx(iface);
    FIXME("(%p)->(%x)\n", This, fUndoable);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHInPlaceSite_OnInPlaceDeactivate(IOleInPlaceSiteEx *iface)
{
    PluginHost *This = impl_from_IOleInPlaceSiteEx(iface);

    TRACE("(%p)\n", This);

    if(This->ip_object) {
        IOleInPlaceObject_Release(This->ip_object);
        This->ip_object = NULL;
    }

    return S_OK;
}

static HRESULT WINAPI PHInPlaceSite_DiscardUndoState(IOleInPlaceSiteEx *iface)
{
    PluginHost *This = impl_from_IOleInPlaceSiteEx(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHInPlaceSite_DeactivateAndUndo(IOleInPlaceSiteEx *iface)
{
    PluginHost *This = impl_from_IOleInPlaceSiteEx(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHInPlaceSite_OnPosRectChange(IOleInPlaceSiteEx *iface, LPCRECT lprcPosRect)
{
    PluginHost *This = impl_from_IOleInPlaceSiteEx(iface);
    FIXME("(%p)->(%p)\n", This, lprcPosRect);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHInPlaceSiteEx_OnInPlaceActivateEx(IOleInPlaceSiteEx *iface, BOOL *pfNoRedraw, DWORD dwFlags)
{
    PluginHost *This = impl_from_IOleInPlaceSiteEx(iface);
    HWND hwnd;
    HRESULT hres;

    TRACE("(%p)->(%p %x)\n", This, pfNoRedraw, dwFlags);

    if(This->ip_object)
        return S_OK;

    hres = IUnknown_QueryInterface(This->plugin_unk, &IID_IOleInPlaceObject, (void**)&This->ip_object);
    if(FAILED(hres))
        return hres;

    hres = IOleInPlaceObject_GetWindow(This->ip_object, &hwnd);
    if(SUCCEEDED(hres))
        FIXME("Use hwnd %p\n", hwnd);

    *pfNoRedraw = FALSE;
    return S_OK;
}

static HRESULT WINAPI PHInPlaceSiteEx_OnInPlaceDeactivateEx(IOleInPlaceSiteEx *iface, BOOL fNoRedraw)
{
    PluginHost *This = impl_from_IOleInPlaceSiteEx(iface);
    FIXME("(%p)->(%x)\n", This, fNoRedraw);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHInPlaceSiteEx_RequestUIActivate(IOleInPlaceSiteEx *iface)
{
    PluginHost *This = impl_from_IOleInPlaceSiteEx(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static const IOleInPlaceSiteExVtbl OleInPlaceSiteExVtbl = {
    PHInPlaceSite_QueryInterface,
    PHInPlaceSite_AddRef,
    PHInPlaceSite_Release,
    PHInPlaceSite_GetWindow,
    PHInPlaceSite_ContextSensitiveHelp,
    PHInPlaceSite_CanInPlaceActivate,
    PHInPlaceSite_OnInPlaceActivate,
    PHInPlaceSite_OnUIActivate,
    PHInPlaceSite_GetWindowContext,
    PHInPlaceSite_Scroll,
    PHInPlaceSite_OnUIDeactivate,
    PHInPlaceSite_OnInPlaceDeactivate,
    PHInPlaceSite_DiscardUndoState,
    PHInPlaceSite_DeactivateAndUndo,
    PHInPlaceSite_OnPosRectChange,
    PHInPlaceSiteEx_OnInPlaceActivateEx,
    PHInPlaceSiteEx_OnInPlaceDeactivateEx,
    PHInPlaceSiteEx_RequestUIActivate
};

static inline PluginHost *impl_from_IOleControlSite(IOleControlSite *iface)
{
    return CONTAINING_RECORD(iface, PluginHost, IOleControlSite_iface);
}

static HRESULT WINAPI PHControlSite_QueryInterface(IOleControlSite *iface, REFIID riid, void **ppv)
{
    PluginHost *This = impl_from_IOleControlSite(iface);
    return IOleClientSite_QueryInterface(&This->IOleClientSite_iface, riid, ppv);
}

static ULONG WINAPI PHControlSite_AddRef(IOleControlSite *iface)
{
    PluginHost *This = impl_from_IOleControlSite(iface);
    return IOleClientSite_AddRef(&This->IOleClientSite_iface);
}

static ULONG WINAPI PHControlSite_Release(IOleControlSite *iface)
{
    PluginHost *This = impl_from_IOleControlSite(iface);
    return IOleClientSite_Release(&This->IOleClientSite_iface);
}

static HRESULT WINAPI PHControlSite_OnControlInfoChanged(IOleControlSite *iface)
{
    PluginHost *This = impl_from_IOleControlSite(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHControlSite_LockInPlaceActive(IOleControlSite *iface, BOOL fLock)
{
    PluginHost *This = impl_from_IOleControlSite(iface);
    FIXME("(%p)->(%x)\n", This, fLock);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHControlSite_GetExtendedControl(IOleControlSite *iface, IDispatch **ppDisp)
{
    PluginHost *This = impl_from_IOleControlSite(iface);
    FIXME("(%p)->(%p)\n", This, ppDisp);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHControlSite_TransformCoords(IOleControlSite *iface, POINTL *pPtlHimetric, POINTF *pPtfContainer, DWORD dwFlags)
{
    PluginHost *This = impl_from_IOleControlSite(iface);
    FIXME("(%p)->(%p %p %x)\n", This, pPtlHimetric, pPtfContainer, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHControlSite_TranslateAccelerator(IOleControlSite *iface, MSG *pMsg, DWORD grfModifiers)
{
    PluginHost *This = impl_from_IOleControlSite(iface);
    FIXME("(%p)->(%x)\n", This, grfModifiers);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHControlSite_OnFocus(IOleControlSite *iface, BOOL fGotFocus)
{
    PluginHost *This = impl_from_IOleControlSite(iface);
    FIXME("(%p)->(%x)\n", This, fGotFocus);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHControlSite_ShowPropertyFrame(IOleControlSite *iface)
{
    PluginHost *This = impl_from_IOleControlSite(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static const IOleControlSiteVtbl OleControlSiteVtbl = {
    PHControlSite_QueryInterface,
    PHControlSite_AddRef,
    PHControlSite_Release,
    PHControlSite_OnControlInfoChanged,
    PHControlSite_LockInPlaceActive,
    PHControlSite_GetExtendedControl,
    PHControlSite_TransformCoords,
    PHControlSite_TranslateAccelerator,
    PHControlSite_OnFocus,
    PHControlSite_ShowPropertyFrame
};

static inline PluginHost *impl_from_IBindHost(IBindHost *iface)
{
    return CONTAINING_RECORD(iface, PluginHost, IBindHost_iface);
}

static HRESULT WINAPI PHBindHost_QueryInterface(IBindHost *iface, REFIID riid, void **ppv)
{
    PluginHost *This = impl_from_IBindHost(iface);
    return IOleClientSite_QueryInterface(&This->IOleClientSite_iface, riid, ppv);
}

static ULONG WINAPI PHBindHost_AddRef(IBindHost *iface)
{
    PluginHost *This = impl_from_IBindHost(iface);
    return IOleClientSite_AddRef(&This->IOleClientSite_iface);
}

static ULONG WINAPI PHBindHost_Release(IBindHost *iface)
{
    PluginHost *This = impl_from_IBindHost(iface);
    return IOleClientSite_Release(&This->IOleClientSite_iface);
}

static HRESULT WINAPI PHBindHost_CreateMoniker(IBindHost *iface, LPOLESTR szName, IBindCtx *pBC, IMoniker **ppmk, DWORD dwReserved)
{
    PluginHost *This = impl_from_IBindHost(iface);

    TRACE("(%p)->(%s %p %p %x)\n", This, debugstr_w(szName), pBC, ppmk, dwReserved);

    if(!This->doc || !This->doc->window || !This->doc->window->mon) {
        FIXME("no moniker\n");
        return E_UNEXPECTED;
    }

    return CreateURLMoniker(This->doc->window->mon, szName, ppmk);
}

static HRESULT WINAPI PHBindHost_MonikerBindToStorage(IBindHost *iface, IMoniker *pMk, IBindCtx *pBC,
        IBindStatusCallback *pBSC, REFIID riid, void **ppvObj)
{
    PluginHost *This = impl_from_IBindHost(iface);
    FIXME("(%p)->(%p %p %p %s %p)\n", This, pMk, pBC, pBSC, debugstr_guid(riid), ppvObj);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHBindHost_MonikerBindToObject(IBindHost *iface, IMoniker *pMk, IBindCtx *pBC,
        IBindStatusCallback *pBSC, REFIID riid, void **ppvObj)
{
    PluginHost *This = impl_from_IBindHost(iface);
    FIXME("(%p)->(%p %p %p %s %p)\n", This, pMk, pBC, pBSC, debugstr_guid(riid), ppvObj);
    return E_NOTIMPL;
}

static const IBindHostVtbl BindHostVtbl = {
    PHBindHost_QueryInterface,
    PHBindHost_AddRef,
    PHBindHost_Release,
    PHBindHost_CreateMoniker,
    PHBindHost_MonikerBindToStorage,
    PHBindHost_MonikerBindToObject
};

static inline PluginHost *impl_from_IServiceProvider(IServiceProvider *iface)
{
    return CONTAINING_RECORD(iface, PluginHost, IServiceProvider_iface);
}

static HRESULT WINAPI PHServiceProvider_QueryInterface(IServiceProvider *iface, REFIID riid, void **ppv)
{
    PluginHost *This = impl_from_IServiceProvider(iface);
    return IOleClientSite_QueryInterface(&This->IOleClientSite_iface, riid, ppv);
}

static ULONG WINAPI PHServiceProvider_AddRef(IServiceProvider *iface)
{
    PluginHost *This = impl_from_IServiceProvider(iface);
    return IOleClientSite_AddRef(&This->IOleClientSite_iface);
}

static ULONG WINAPI PHServiceProvider_Release(IServiceProvider *iface)
{
    PluginHost *This = impl_from_IServiceProvider(iface);
    return IOleClientSite_Release(&This->IOleClientSite_iface);
}

static HRESULT WINAPI PHServiceProvider_QueryService(IServiceProvider *iface, REFGUID guidService, REFIID riid, void **ppv)
{
    PluginHost *This = impl_from_IServiceProvider(iface);

    if(IsEqualGUID(guidService, &SID_SBindHost)) {
        TRACE("SID_SBindHost service\n");
        return IOleClientSite_QueryInterface(&This->IOleClientSite_iface, riid, ppv);
    }

    TRACE("(%p)->(%s %s %p)\n", This, debugstr_guid(guidService), debugstr_guid(riid), ppv);

    if(!This->doc || !This->doc->basedoc.window) {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    return IServiceProvider_QueryService(&This->doc->basedoc.window->base.IServiceProvider_iface,
            guidService, riid, ppv);
}

static const IServiceProviderVtbl ServiceProviderVtbl = {
    PHServiceProvider_QueryInterface,
    PHServiceProvider_AddRef,
    PHServiceProvider_Release,
    PHServiceProvider_QueryService
};

static BOOL parse_classid(const PRUnichar *classid, CLSID *clsid)
{
    const WCHAR *ptr;
    unsigned len;
    HRESULT hres;

    static const PRUnichar clsidW[] = {'c','l','s','i','d',':'};

    if(strncmpiW(classid, clsidW, sizeof(clsidW)/sizeof(WCHAR)))
        return FALSE;

    ptr = classid + sizeof(clsidW)/sizeof(WCHAR);
    len = strlenW(ptr);

    if(len == 38) {
        hres = CLSIDFromString(ptr, clsid);
    }else if(len == 36) {
        WCHAR buf[39];

        buf[0] = '{';
        memcpy(buf+1, ptr, len*sizeof(WCHAR));
        buf[37] = '}';
        buf[38] = 0;
        hres = CLSIDFromString(buf, clsid);
    }else {
        return FALSE;
    }

    return SUCCEEDED(hres);
}

static BOOL get_elem_clsid(nsIDOMHTMLElement *elem, CLSID *clsid)
{
    const PRUnichar *val;
    nsAString val_str;
    nsresult nsres;
    BOOL ret = FALSE;

    static const PRUnichar classidW[] = {'c','l','a','s','s','i','d',0};

    nsres = get_elem_attr_value(elem, classidW, &val_str, &val);
    if(NS_SUCCEEDED(nsres)) {
        if(*val)
            ret = parse_classid(val, clsid);
        nsAString_Finish(&val_str);
    }

    return ret;
}

typedef struct {
    IBindStatusCallback IBindStatusCallback_iface;
    IWindowForBindingUI IWindowForBindingUI_iface;
    LONG ref;
} InstallCallback;

static inline InstallCallback *impl_from_IBindStatusCallback(IBindStatusCallback *iface)
{
    return CONTAINING_RECORD(iface, InstallCallback, IBindStatusCallback_iface);
}

static HRESULT WINAPI InstallCallback_QueryInterface(IBindStatusCallback *iface,
        REFIID riid, void **ppv)
{
    InstallCallback *This = impl_from_IBindStatusCallback(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IBindStatusCallback_iface;
    }else if(IsEqualGUID(&IID_IBindStatusCallback, riid)) {
        TRACE("(%p)->(IID_IBindStatusCallback %p)\n", This, ppv);
        *ppv = &This->IBindStatusCallback_iface;
    }else if(IsEqualGUID(&IID_IWindowForBindingUI, riid)) {
        TRACE("(%p)->(IID_IWindowForBindingUI %p)\n", This, ppv);
        *ppv = &This->IWindowForBindingUI_iface;
    }else {
        TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI InstallCallback_AddRef(IBindStatusCallback *iface)
{
    InstallCallback *This = impl_from_IBindStatusCallback(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI InstallCallback_Release(IBindStatusCallback *iface)
{
    InstallCallback *This = impl_from_IBindStatusCallback(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref)
        heap_free(This);

    return ref;
}

static HRESULT WINAPI InstallCallback_OnStartBinding(IBindStatusCallback *iface,
        DWORD dwReserved, IBinding *pib)
{
    InstallCallback *This = impl_from_IBindStatusCallback(iface);
    TRACE("(%p)->(%x %p)\n", This, dwReserved, pib);
    return S_OK;
}

static HRESULT WINAPI InstallCallback_GetPriority(IBindStatusCallback *iface, LONG *pnPriority)
{
    InstallCallback *This = impl_from_IBindStatusCallback(iface);
    TRACE("(%p)->(%p)\n", This, pnPriority);
    return E_NOTIMPL;
}

static HRESULT WINAPI InstallCallback_OnLowResource(IBindStatusCallback *iface, DWORD dwReserved)
{
    InstallCallback *This = impl_from_IBindStatusCallback(iface);
    TRACE("(%p)->(%x)\n", This, dwReserved);
    return S_OK;
}

static HRESULT WINAPI InstallCallback_OnProgress(IBindStatusCallback *iface, ULONG ulProgress,
        ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText)
{
    InstallCallback *This = impl_from_IBindStatusCallback(iface);
    TRACE("(%p)->(%u %u %u %s)\n", This, ulProgress, ulProgressMax, ulStatusCode, debugstr_w(szStatusText));
    return S_OK;
}

static HRESULT WINAPI InstallCallback_OnStopBinding(IBindStatusCallback *iface,
        HRESULT hresult, LPCWSTR szError)
{
    InstallCallback *This = impl_from_IBindStatusCallback(iface);
    TRACE("(%p)->(%08x %s)\n", This, hresult, debugstr_w(szError));
    return S_OK;
}

static HRESULT WINAPI InstallCallback_GetBindInfo(IBindStatusCallback *iface,
        DWORD* grfBINDF, BINDINFO* pbindinfo)
{
    InstallCallback *This = impl_from_IBindStatusCallback(iface);

    TRACE("(%p)->(%p %p)\n", This, grfBINDF, pbindinfo);

    *grfBINDF = BINDF_ASYNCHRONOUS;
    return S_OK;
}

static HRESULT WINAPI InstallCallback_OnDataAvailable(IBindStatusCallback *iface, DWORD grfBSCF,
        DWORD dwSize, FORMATETC* pformatetc, STGMEDIUM* pstgmed)
{
    InstallCallback *This = impl_from_IBindStatusCallback(iface);
    ERR("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI InstallCallback_OnObjectAvailable(IBindStatusCallback *iface,
        REFIID riid, IUnknown* punk)
{
    InstallCallback *This = impl_from_IBindStatusCallback(iface);
    ERR("(%p)\n", This);
    return E_NOTIMPL;
}

static IBindStatusCallbackVtbl InstallCallbackVtbl = {
    InstallCallback_QueryInterface,
    InstallCallback_AddRef,
    InstallCallback_Release,
    InstallCallback_OnStartBinding,
    InstallCallback_GetPriority,
    InstallCallback_OnLowResource,
    InstallCallback_OnProgress,
    InstallCallback_OnStopBinding,
    InstallCallback_GetBindInfo,
    InstallCallback_OnDataAvailable,
    InstallCallback_OnObjectAvailable
};

static inline InstallCallback *impl_from_IWindowForBindingUI(IWindowForBindingUI *iface)
{
    return CONTAINING_RECORD(iface, InstallCallback, IWindowForBindingUI_iface);
}

static HRESULT WINAPI WindowForBindingUI_QueryInterface(IWindowForBindingUI *iface, REFIID riid, void **ppv)
{
    InstallCallback *This = impl_from_IWindowForBindingUI(iface);
    return IBindStatusCallback_QueryInterface(&This->IBindStatusCallback_iface, riid, ppv);
}

static ULONG WINAPI WindowForBindingUI_AddRef(IWindowForBindingUI *iface)
{
    InstallCallback *This = impl_from_IWindowForBindingUI(iface);
    return IBindStatusCallback_AddRef(&This->IBindStatusCallback_iface);
}

static ULONG WINAPI WindowForBindingUI_Release(IWindowForBindingUI *iface)
{
    InstallCallback *This = impl_from_IWindowForBindingUI(iface);
    return IBindStatusCallback_Release(&This->IBindStatusCallback_iface);
}

static HRESULT WINAPI WindowForBindingUI_GetWindow(IWindowForBindingUI *iface, REFGUID rguidReason, HWND *phwnd)
{
    InstallCallback *This = impl_from_IWindowForBindingUI(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_guid(rguidReason), phwnd);
    *phwnd = NULL;
    return S_OK;
}

static const IWindowForBindingUIVtbl WindowForBindingUIVtbl = {
    WindowForBindingUI_QueryInterface,
    WindowForBindingUI_AddRef,
    WindowForBindingUI_Release,
    WindowForBindingUI_GetWindow
};

typedef struct {
    struct list entry;
    IUri *uri;
} install_entry_t;

static struct list install_list = LIST_INIT(install_list);

static CRITICAL_SECTION cs_install_list;
static CRITICAL_SECTION_DEBUG cs_install_list_dbg =
{
    0, 0, &cs_install_list,
    { &cs_install_list_dbg.ProcessLocksList, &cs_install_list_dbg.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": install_list") }
};
static CRITICAL_SECTION cs_install_list = { &cs_install_list_dbg, -1, 0, 0, 0, 0 };

static void install_codebase(const WCHAR *url)
{
    InstallCallback *callback;
    IBindCtx *bctx;
    HRESULT hres;

    callback = heap_alloc(sizeof(*callback));
    if(!callback)
        return;

    callback->IBindStatusCallback_iface.lpVtbl = &InstallCallbackVtbl;
    callback->IWindowForBindingUI_iface.lpVtbl = &WindowForBindingUIVtbl;
    callback->ref = 1;

    hres = CreateAsyncBindCtx(0, &callback->IBindStatusCallback_iface, NULL, &bctx);
    IBindStatusCallback_Release(&callback->IBindStatusCallback_iface);
    if(FAILED(hres))
        return;

    hres = AsyncInstallDistributionUnit(NULL, NULL, NULL, 0, 0, url, bctx, NULL, 0);
    IBindCtx_Release(bctx);
    if(FAILED(hres))
        WARN("FAILED: %08x\n", hres);
}

static void check_codebase(HTMLInnerWindow *window, nsIDOMHTMLElement *nselem)
{
    BOOL is_on_list = FALSE;
    install_entry_t *iter;
    const PRUnichar *val;
    nsAString val_str;
    IUri *uri = NULL;
    nsresult nsres;
    HRESULT hres;

    static const PRUnichar codebaseW[] = {'c','o','d','e','b','a','s','e',0};

    nsres = get_elem_attr_value(nselem, codebaseW, &val_str, &val);
    if(NS_SUCCEEDED(nsres)) {
        if(*val) {
            hres = CoInternetCombineUrlEx(window->base.outer_window->uri, val, 0, &uri, 0);
            if(FAILED(hres))
                uri = NULL;
        }
        nsAString_Finish(&val_str);
    }

    if(!uri)
        return;

    EnterCriticalSection(&cs_install_list);

    LIST_FOR_EACH_ENTRY(iter, &install_list, install_entry_t, entry) {
        BOOL eq;

        hres = IUri_IsEqual(uri, iter->uri, &eq);
        if(SUCCEEDED(hres) && eq) {
            TRACE("already proceeded\n");
            is_on_list = TRUE;
            break;
        }
    }

    if(!is_on_list) {
        iter = heap_alloc(sizeof(*iter));
        if(iter) {
            IUri_AddRef(uri);
            iter->uri = uri;

            list_add_tail(&install_list, &iter->entry);
        }
    }

    LeaveCriticalSection(&cs_install_list);

    if(!is_on_list) {
        BSTR display_uri;

        hres = IUri_GetDisplayUri(uri, &display_uri);
        if(SUCCEEDED(hres)) {
            install_codebase(display_uri);
            SysFreeString(display_uri);
        }
    }

    IUri_Release(uri);
}

static IUnknown *create_activex_object(HTMLDocumentNode *doc, nsIDOMHTMLElement *nselem, CLSID *clsid)
{
    IClassFactoryEx *cfex;
    IClassFactory *cf;
    IUnknown *obj;
    DWORD policy;
    HRESULT hres;

    if(!get_elem_clsid(nselem, clsid)) {
        WARN("Could not determine element CLSID\n");
        return NULL;
    }

    TRACE("clsid %s\n", debugstr_guid(clsid));

    policy = 0;
    hres = IInternetHostSecurityManager_ProcessUrlAction(&doc->IInternetHostSecurityManager_iface,
            URLACTION_ACTIVEX_RUN, (BYTE*)&policy, sizeof(policy), (BYTE*)clsid, sizeof(GUID), 0, 0);
    if(FAILED(hres) || policy != URLPOLICY_ALLOW) {
        WARN("ProcessUrlAction returned %08x %x\n", hres, policy);
        return NULL;
    }

    hres = CoGetClassObject(clsid, CLSCTX_INPROC_SERVER|CLSCTX_LOCAL_SERVER, NULL, &IID_IClassFactory, (void**)&cf);
    if(hres == REGDB_E_CLASSNOTREG)
        check_codebase(doc->window, nselem);
    if(FAILED(hres))
        return NULL;

    hres = IClassFactory_QueryInterface(cf, &IID_IClassFactoryEx, (void**)&cfex);
    if(SUCCEEDED(hres)) {
        FIXME("Use IClassFactoryEx\n");
        IClassFactoryEx_Release(cfex);
    }

    hres = IClassFactory_CreateInstance(cf, NULL, &IID_IUnknown, (void**)&obj);
    IClassFactory_Release(cf);
    if(FAILED(hres))
        return NULL;

    return obj;
}

void detach_plugin_host(PluginHost *host)
{
    HRESULT hres;

    TRACE("%p\n", host);

    if(!host->doc)
        return;

    if(host->ip_object) {
        if(host->ui_active)
            IOleInPlaceObject_UIDeactivate(host->ip_object);
        IOleInPlaceObject_InPlaceDeactivate(host->ip_object);
    }

    if(host->plugin_unk) {
        IOleObject *ole_obj;

        hres = IUnknown_QueryInterface(host->plugin_unk, &IID_IOleObject, (void**)&ole_obj);
        if(SUCCEEDED(hres)) {
            if(!host->ip_object)
                IOleObject_Close(ole_obj, OLECLOSE_NOSAVE);
            IOleObject_SetClientSite(ole_obj, NULL);
            IOleObject_Release(ole_obj);
        }
    }

    if(host->sink) {
        IConnectionPointContainer *cp_container;
        IConnectionPoint *cp;

        assert(host->plugin_unk != NULL);

        hres = IUnknown_QueryInterface(host->plugin_unk, &IID_IConnectionPointContainer, (void**)&cp_container);
        if(SUCCEEDED(hres)) {
            hres = IConnectionPointContainer_FindConnectionPoint(cp_container, &host->sink->iid, &cp);
            IConnectionPointContainer_Release(cp_container);
            if(SUCCEEDED(hres)) {
                IConnectionPoint_Unadvise(cp, host->sink->cookie);
                IConnectionPoint_Release(cp);
            }
        }

        host->sink->host = NULL;
        IDispatch_Release(&host->sink->IDispatch_iface);
        host->sink = NULL;
    }

    release_plugin_ifaces(host);

    if(host->element) {
        host->element->plugin_host = NULL;
        host->element = NULL;
    }

    list_remove(&host->entry);
    list_init(&host->entry);
    host->doc = NULL;
}

HRESULT create_plugin_host(HTMLDocumentNode *doc, HTMLPluginContainer *container)
{
    PluginHost *host;
    IUnknown *unk;
    CLSID clsid;

    assert(!container->plugin_host);

    unk = create_activex_object(doc, container->element.nselem, &clsid);
    if(!unk)
        return E_FAIL;

    host = heap_alloc_zero(sizeof(*host));
    if(!host) {
        IUnknown_Release(unk);
        return E_OUTOFMEMORY;
    }

    host->IOleClientSite_iface.lpVtbl      = &OleClientSiteVtbl;
    host->IAdviseSinkEx_iface.lpVtbl       = &AdviseSinkExVtbl;
    host->IPropertyNotifySink_iface.lpVtbl = &PropertyNotifySinkVtbl;
    host->IDispatch_iface.lpVtbl           = &DispatchVtbl;
    host->IOleInPlaceSiteEx_iface.lpVtbl   = &OleInPlaceSiteExVtbl;
    host->IOleControlSite_iface.lpVtbl     = &OleControlSiteVtbl;
    host->IBindHost_iface.lpVtbl           = &BindHostVtbl;
    host->IServiceProvider_iface.lpVtbl    = &ServiceProviderVtbl;

    host->ref = 1;

    host->plugin_unk = unk;
    host->clsid = clsid;

    host->doc = doc;
    list_add_tail(&doc->plugin_hosts, &host->entry);

    container->plugin_host = host;
    host->element = container;

    initialize_plugin_object(host);

    return S_OK;
}
