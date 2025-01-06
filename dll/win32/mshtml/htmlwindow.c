/*
 * Copyright 2006-2012 Jacek Caban for CodeWeavers
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
#include <assert.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "mshtmdid.h"
#include "shlguid.h"
#include "shobjidl.h"
#include "exdispid.h"

#define NO_SHLWAPI_REG
#include "shlwapi.h"

#include "wine/debug.h"

#include "mshtml_private.h"
#include "htmlevent.h"
#include "htmlscript.h"
#include "pluginhost.h"
#include "binding.h"
#include "resource.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

static struct list window_list = LIST_INIT(window_list);

static inline BOOL is_outer_window(HTMLWindow *window)
{
    return &window->outer_window->base == window;
}

static void release_children(HTMLOuterWindow *This)
{
    HTMLOuterWindow *child;

    while(!list_empty(&This->children)) {
        child = LIST_ENTRY(list_tail(&This->children), HTMLOuterWindow, sibling_entry);

        list_remove(&child->sibling_entry);
        child->parent = NULL;
        IHTMLWindow2_Release(&child->base.IHTMLWindow2_iface);
    }
}

static HRESULT get_location(HTMLInnerWindow *This, HTMLLocation **ret)
{
    if(This->location) {
        IHTMLLocation_AddRef(&This->location->IHTMLLocation_iface);
    }else {
        HRESULT hres;

        hres = HTMLLocation_Create(This, &This->location);
        if(FAILED(hres))
            return hres;
    }

    *ret = This->location;
    return S_OK;
}

void get_top_window(HTMLOuterWindow *window, HTMLOuterWindow **ret)
{
    HTMLOuterWindow *iter;

    for(iter = window; iter->parent; iter = iter->parent);
    *ret = iter;
}

static inline HRESULT set_window_event(HTMLWindow *window, eventid_t eid, VARIANT *var)
{
    if(!window->inner_window->doc) {
        FIXME("No document\n");
        return E_FAIL;
    }

    return set_event_handler(&window->inner_window->event_target, eid, var);
}

static inline HRESULT get_window_event(HTMLWindow *window, eventid_t eid, VARIANT *var)
{
    if(!window->inner_window->doc) {
        FIXME("No document\n");
        return E_FAIL;
    }

    return get_event_handler(&window->inner_window->event_target, eid, var);
}

static void detach_inner_window(HTMLInnerWindow *window)
{
    HTMLOuterWindow *outer_window = window->base.outer_window;

    if(outer_window && outer_window->doc_obj && outer_window == outer_window->doc_obj->basedoc.window)
        window->doc->basedoc.cp_container.forward_container = NULL;

    if(window->doc) {
        detach_events(window->doc);
        while(!list_empty(&window->doc->plugin_hosts))
            detach_plugin_host(LIST_ENTRY(list_head(&window->doc->plugin_hosts), PluginHost, entry));
    }

    abort_window_bindings(window);
    remove_target_tasks(window->task_magic);
    release_script_hosts(window);
    window->base.outer_window = NULL;

    if(outer_window && outer_window->base.inner_window == window) {
        outer_window->base.inner_window = NULL;
        IHTMLWindow2_Release(&window->base.IHTMLWindow2_iface);
    }
}

static inline HTMLWindow *impl_from_IHTMLWindow2(IHTMLWindow2 *iface)
{
    return CONTAINING_RECORD(iface, HTMLWindow, IHTMLWindow2_iface);
}

static HRESULT WINAPI HTMLWindow2_QueryInterface(IHTMLWindow2 *iface, REFIID riid, void **ppv)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = &This->IHTMLWindow2_iface;
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        *ppv = &This->IHTMLWindow2_iface;
    }else if(IsEqualGUID(&IID_IDispatchEx, riid)) {
        *ppv = &This->IDispatchEx_iface;
    }else if(IsEqualGUID(&IID_IHTMLFramesCollection2, riid)) {
        *ppv = &This->IHTMLWindow2_iface;
    }else if(IsEqualGUID(&IID_IHTMLWindow2, riid)) {
        *ppv = &This->IHTMLWindow2_iface;
    }else if(IsEqualGUID(&IID_IHTMLWindow3, riid)) {
        *ppv = &This->IHTMLWindow3_iface;
    }else if(IsEqualGUID(&IID_IHTMLWindow4, riid)) {
        *ppv = &This->IHTMLWindow4_iface;
    }else if(IsEqualGUID(&IID_IHTMLWindow5, riid)) {
        *ppv = &This->IHTMLWindow5_iface;
    }else if(IsEqualGUID(&IID_IHTMLWindow6, riid)) {
        *ppv = &This->IHTMLWindow6_iface;
    }else if(IsEqualGUID(&IID_IHTMLPrivateWindow, riid)) {
        *ppv = &This->IHTMLPrivateWindow_iface;
    }else if(IsEqualGUID(&IID_IServiceProvider, riid)) {
        *ppv = &This->IServiceProvider_iface;
    }else if(IsEqualGUID(&IID_ITravelLogClient, riid)) {
        *ppv = &This->ITravelLogClient_iface;
    }else if(IsEqualGUID(&IID_IObjectIdentity, riid)) {
        *ppv = &This->IObjectIdentity_iface;
    }else if(dispex_query_interface(&This->inner_window->event_target.dispex, riid, ppv)) {
        assert(!*ppv);
        return E_NOINTERFACE;
    }else {
        *ppv = NULL;
        WARN("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI HTMLWindow2_AddRef(IHTMLWindow2 *iface)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static void release_outer_window(HTMLOuterWindow *This)
{
    if(This->pending_window) {
        abort_window_bindings(This->pending_window);
        This->pending_window->base.outer_window = NULL;
        IHTMLWindow2_Release(&This->pending_window->base.IHTMLWindow2_iface);
    }

    remove_target_tasks(This->task_magic);
    set_current_mon(This, NULL, 0);
    if(This->base.inner_window)
        detach_inner_window(This->base.inner_window);
    release_children(This);

    if(This->secmgr)
        IInternetSecurityManager_Release(This->secmgr);

    if(This->frame_element)
        This->frame_element->content_window = NULL;

    This->window_ref->window = NULL;
    windowref_release(This->window_ref);

    if(This->nswindow)
        nsIDOMWindow_Release(This->nswindow);
    if(This->window_proxy)
        mozIDOMWindowProxy_Release(This->window_proxy);

    list_remove(&This->entry);
    heap_free(This);
}

static void release_inner_window(HTMLInnerWindow *This)
{
    unsigned i;

    TRACE("%p\n", This);

    detach_inner_window(This);

    if(This->doc) {
        This->doc->window = NULL;
        htmldoc_release(&This->doc->basedoc);
    }

    release_dispex(&This->event_target.dispex);

    for(i=0; i < This->global_prop_cnt; i++)
        heap_free(This->global_props[i].name);
    heap_free(This->global_props);

    if(This->location) {
        This->location->window = NULL;
        IHTMLLocation_Release(&This->location->IHTMLLocation_iface);
    }

    if(This->image_factory) {
        This->image_factory->window = NULL;
        IHTMLImageElementFactory_Release(&This->image_factory->IHTMLImageElementFactory_iface);
    }

    if(This->option_factory) {
        This->option_factory->window = NULL;
        IHTMLOptionElementFactory_Release(&This->option_factory->IHTMLOptionElementFactory_iface);
    }

    if(This->xhr_factory) {
        This->xhr_factory->window = NULL;
        IHTMLXMLHttpRequestFactory_Release(&This->xhr_factory->IHTMLXMLHttpRequestFactory_iface);
    }

    if(This->screen)
        IHTMLScreen_Release(This->screen);

    if(This->history) {
        This->history->window = NULL;
        IOmHistory_Release(&This->history->IOmHistory_iface);
    }

    if(This->session_storage)
        IHTMLStorage_Release(This->session_storage);

    if(This->mon)
        IMoniker_Release(This->mon);

    heap_free(This);
}

static ULONG WINAPI HTMLWindow2_Release(IHTMLWindow2 *iface)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        if(is_outer_window(This))
            release_outer_window(This->outer_window);
        else
            release_inner_window(This->inner_window);
    }

    return ref;
}

static HRESULT WINAPI HTMLWindow2_GetTypeInfoCount(IHTMLWindow2 *iface, UINT *pctinfo)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);

    return IDispatchEx_GetTypeInfoCount(&This->IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLWindow2_GetTypeInfo(IHTMLWindow2 *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);

    return IDispatchEx_GetTypeInfo(&This->IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLWindow2_GetIDsOfNames(IHTMLWindow2 *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);

    return IDispatchEx_GetIDsOfNames(&This->IDispatchEx_iface, riid, rgszNames, cNames, lcid,
            rgDispId);
}

static HRESULT WINAPI HTMLWindow2_Invoke(IHTMLWindow2 *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);

    return IDispatchEx_Invoke(&This->IDispatchEx_iface, dispIdMember, riid, lcid, wFlags,
            pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT get_frame_by_index(HTMLOuterWindow *This, UINT32 index, HTMLOuterWindow **ret)
{
    nsIDOMWindowCollection *nsframes;
    mozIDOMWindowProxy *mozwindow;
    UINT32 length;
    nsresult nsres;

    nsres = nsIDOMWindow_GetFrames(This->nswindow, &nsframes);
    if(NS_FAILED(nsres)) {
        FIXME("nsIDOMWindow_GetFrames failed: 0x%08x\n", nsres);
        return E_FAIL;
    }

    nsres = nsIDOMWindowCollection_GetLength(nsframes, &length);
    assert(nsres == NS_OK);

    if(index >= length) {
        nsIDOMWindowCollection_Release(nsframes);
        return DISP_E_MEMBERNOTFOUND;
    }

    nsres = nsIDOMWindowCollection_Item(nsframes, index, &mozwindow);
    nsIDOMWindowCollection_Release(nsframes);
    if(NS_FAILED(nsres)) {
        FIXME("nsIDOMWindowCollection_Item failed: 0x%08x\n", nsres);
        return E_FAIL;
    }

    *ret = mozwindow_to_window(mozwindow);

    mozIDOMWindowProxy_Release(mozwindow);
    return S_OK;
}

HRESULT get_frame_by_name(HTMLOuterWindow *This, const WCHAR *name, BOOL deep, HTMLOuterWindow **ret)
{
    nsIDOMWindowCollection *nsframes;
    HTMLOuterWindow *window = NULL;
    mozIDOMWindowProxy *mozwindow;
    nsAString name_str;
    UINT32 length, i;
    nsresult nsres;
    HRESULT hres = S_OK;

    nsres = nsIDOMWindow_GetFrames(This->nswindow, &nsframes);
    if(NS_FAILED(nsres)) {
        FIXME("nsIDOMWindow_GetFrames failed: 0x%08x\n", nsres);
        return E_FAIL;
    }

    if(!nsframes) {
        WARN("nsIDOMWindow_GetFrames returned NULL nsframes: %p\n", This->nswindow);
        return DISP_E_MEMBERNOTFOUND;
    }

    nsAString_InitDepend(&name_str, name);
    nsres = nsIDOMWindowCollection_NamedItem(nsframes, &name_str, &mozwindow);
    nsAString_Finish(&name_str);
    if(NS_FAILED(nsres)) {
        nsIDOMWindowCollection_Release(nsframes);
        return E_FAIL;
    }

    if(mozwindow) {
        *ret = mozwindow_to_window(mozwindow);
        return S_OK;
    }

    nsres = nsIDOMWindowCollection_GetLength(nsframes, &length);
    assert(nsres == NS_OK);

    for(i = 0; i < length && !window; ++i) {
        HTMLOuterWindow *window_iter;
        BSTR id;

        nsres = nsIDOMWindowCollection_Item(nsframes, i, &mozwindow);
        if(NS_FAILED(nsres)) {
            FIXME("nsIDOMWindowCollection_Item failed: 0x%08x\n", nsres);
            hres = E_FAIL;
            break;
        }

        window_iter = mozwindow_to_window(mozwindow);

        mozIDOMWindowProxy_Release(mozwindow);

        if(!window_iter) {
            WARN("nsIDOMWindow without HTMLOuterWindow: %p\n", mozwindow);
            continue;
        }

        hres = IHTMLElement_get_id(&window_iter->frame_element->element.IHTMLElement_iface, &id);
        if(FAILED(hres)) {
            FIXME("IHTMLElement_get_id failed: 0x%08x\n", hres);
            break;
        }

        if(id && !strcmpiW(id, name))
            window = window_iter;

        SysFreeString(id);

        if(!window && deep)
            get_frame_by_name(window_iter, name, TRUE, &window);
    }

    nsIDOMWindowCollection_Release(nsframes);
    if(FAILED(hres))
        return hres;

    *ret = window;
    return NS_OK;
}

static HRESULT WINAPI HTMLWindow2_item(IHTMLWindow2 *iface, VARIANT *pvarIndex, VARIANT *pvarResult)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);
    HTMLOuterWindow *window = NULL;
    HRESULT hres;

    TRACE("(%p)->(%p %p)\n", This, pvarIndex, pvarResult);

    switch(V_VT(pvarIndex)) {
    case VT_I4: {
        int index = V_I4(pvarIndex);
        TRACE("Getting index %d\n", index);
        if(index < 0)
            return DISP_E_MEMBERNOTFOUND;
        hres = get_frame_by_index(This->outer_window, index, &window);
        break;
    }
    case VT_UINT: {
        unsigned int index = V_UINT(pvarIndex);
        TRACE("Getting index %u\n", index);
        hres = get_frame_by_index(This->outer_window, index, &window);
        break;
    }
    case VT_BSTR: {
        BSTR str = V_BSTR(pvarIndex);
        TRACE("Getting name %s\n", wine_dbgstr_w(str));
        hres = get_frame_by_name(This->outer_window, str, FALSE, &window);
        break;
    }
    default:
        WARN("Invalid index %s\n", debugstr_variant(pvarIndex));
        return E_INVALIDARG;
    }

    if(FAILED(hres))
        return hres;
    if(!window)
        return DISP_E_MEMBERNOTFOUND;

    IHTMLWindow2_AddRef(&window->base.IHTMLWindow2_iface);
    V_VT(pvarResult) = VT_DISPATCH;
    V_DISPATCH(pvarResult) = (IDispatch*)window;
    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_get_length(IHTMLWindow2 *iface, LONG *p)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);
    nsIDOMWindowCollection *nscollection;
    UINT32 length;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMWindow_GetFrames(This->outer_window->nswindow, &nscollection);
    if(NS_FAILED(nsres)) {
        ERR("GetFrames failed: %08x\n", nsres);
        return E_FAIL;
    }

    nsres = nsIDOMWindowCollection_GetLength(nscollection, &length);
    nsIDOMWindowCollection_Release(nscollection);
    if(NS_FAILED(nsres)) {
        ERR("GetLength failed: %08x\n", nsres);
        return E_FAIL;
    }

    *p = length;
    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_get_frames(IHTMLWindow2 *iface, IHTMLFramesCollection2 **p)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%p): semi-stub\n", This, p);

    /* FIXME: Should return a separate Window object */
    *p = (IHTMLFramesCollection2*)&This->IHTMLWindow2_iface;
    HTMLWindow2_AddRef(iface);
    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_put_defaultStatus(IHTMLWindow2 *iface, BSTR v)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_get_defaultStatus(IHTMLWindow2 *iface, BSTR *p)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_put_status(IHTMLWindow2 *iface, BSTR v)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);

    WARN("(%p)->(%s)\n", This, debugstr_w(v));

    /*
     * FIXME: Since IE7, setting status is blocked, but still possible in certain circumstances.
     * Ignoring the call should be enough for us.
     */
    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_get_status(IHTMLWindow2 *iface, BSTR *p)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    /* See put_status */
    *p = NULL;
    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_setTimeout(IHTMLWindow2 *iface, BSTR expression,
        LONG msec, VARIANT *language, LONG *timerID)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);
    VARIANT expr_var;

    TRACE("(%p)->(%s %d %p %p)\n", This, debugstr_w(expression), msec, language, timerID);

    V_VT(&expr_var) = VT_BSTR;
    V_BSTR(&expr_var) = expression;

    return IHTMLWindow3_setTimeout(&This->IHTMLWindow3_iface, &expr_var, msec, language, timerID);
}

static HRESULT WINAPI HTMLWindow2_clearTimeout(IHTMLWindow2 *iface, LONG timerID)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);

    TRACE("(%p)->(%d)\n", This, timerID);

    return clear_task_timer(This->inner_window, timerID);
}

#define MAX_MESSAGE_LEN 2000

static HRESULT WINAPI HTMLWindow2_alert(IHTMLWindow2 *iface, BSTR message)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);
    WCHAR title[100], *msg = message;
    DWORD len;

    TRACE("(%p)->(%s)\n", This, debugstr_w(message));

    if(!LoadStringW(get_shdoclc(), IDS_MESSAGE_BOX_TITLE, title,
                    sizeof(title)/sizeof(WCHAR))) {
        WARN("Could not load message box title: %d\n", GetLastError());
        return S_OK;
    }

    len = SysStringLen(message);
    if(len > MAX_MESSAGE_LEN) {
        msg = heap_alloc((MAX_MESSAGE_LEN+1)*sizeof(WCHAR));
        if(!msg)
            return E_OUTOFMEMORY;
        memcpy(msg, message, MAX_MESSAGE_LEN*sizeof(WCHAR));
        msg[MAX_MESSAGE_LEN] = 0;
    }

    MessageBoxW(This->outer_window->doc_obj->hwnd, msg, title, MB_ICONWARNING);
    if(msg != message)
        heap_free(msg);
    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_confirm(IHTMLWindow2 *iface, BSTR message,
        VARIANT_BOOL *confirmed)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);
    WCHAR wszTitle[100];

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(message), confirmed);

    if(!confirmed) return E_INVALIDARG;

    if(!LoadStringW(get_shdoclc(), IDS_MESSAGE_BOX_TITLE, wszTitle,
                sizeof(wszTitle)/sizeof(WCHAR))) {
        WARN("Could not load message box title: %d\n", GetLastError());
        *confirmed = VARIANT_TRUE;
        return S_OK;
    }

    if(MessageBoxW(This->outer_window->doc_obj->hwnd, message, wszTitle,
                MB_OKCANCEL|MB_ICONQUESTION)==IDOK)
        *confirmed = VARIANT_TRUE;
    else *confirmed = VARIANT_FALSE;

    return S_OK;
}

typedef struct
{
    BSTR message;
    BSTR dststr;
    VARIANT *textdata;
}prompt_arg;

static INT_PTR CALLBACK prompt_dlgproc(HWND hwnd, UINT msg,
        WPARAM wparam, LPARAM lparam)
{
    switch(msg)
    {
        case WM_INITDIALOG:
        {
            prompt_arg *arg = (prompt_arg*)lparam;
            WCHAR wszTitle[100];

            if(!LoadStringW(get_shdoclc(), IDS_MESSAGE_BOX_TITLE, wszTitle,
                        sizeof(wszTitle)/sizeof(WCHAR))) {
                WARN("Could not load message box title: %d\n", GetLastError());
                EndDialog(hwnd, wparam);
                return FALSE;
            }

            SetWindowLongPtrW(hwnd, DWLP_USER, lparam);
            SetWindowTextW(hwnd, wszTitle);
            SetWindowTextW(GetDlgItem(hwnd, ID_PROMPT_PROMPT), arg->message);
            SetWindowTextW(GetDlgItem(hwnd, ID_PROMPT_EDIT), arg->dststr);
            return FALSE;
        }
        case WM_COMMAND:
            switch(wparam)
            {
                case MAKEWPARAM(IDCANCEL, BN_CLICKED):
                    EndDialog(hwnd, wparam);
                    return TRUE;
                case MAKEWPARAM(IDOK, BN_CLICKED):
                {
                    prompt_arg *arg =
                        (prompt_arg*)GetWindowLongPtrW(hwnd, DWLP_USER);
                    HWND hwndPrompt = GetDlgItem(hwnd, ID_PROMPT_EDIT);
                    INT len = GetWindowTextLengthW(hwndPrompt);

                    if(!arg->textdata)
                    {
                        EndDialog(hwnd, wparam);
                        return TRUE;
                    }

                    V_VT(arg->textdata) = VT_BSTR;
                    if(!len && !arg->dststr)
                        V_BSTR(arg->textdata) = NULL;
                    else
                    {
                        V_BSTR(arg->textdata) = SysAllocStringLen(NULL, len);
                        GetWindowTextW(hwndPrompt, V_BSTR(arg->textdata), len+1);
                    }
                    EndDialog(hwnd, wparam);
                    return TRUE;
                }
            }
            return FALSE;
        case WM_CLOSE:
            EndDialog(hwnd, IDCANCEL);
            return TRUE;
        default:
            return FALSE;
    }
}

static HRESULT WINAPI HTMLWindow2_prompt(IHTMLWindow2 *iface, BSTR message,
        BSTR dststr, VARIANT *textdata)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);
    prompt_arg arg;

    TRACE("(%p)->(%s %s %p)\n", This, debugstr_w(message), debugstr_w(dststr), textdata);

    if(textdata) V_VT(textdata) = VT_NULL;

    arg.message = message;
    arg.dststr = dststr;
    arg.textdata = textdata;

    DialogBoxParamW(hInst, MAKEINTRESOURCEW(ID_PROMPT_DIALOG),
            This->outer_window->doc_obj->hwnd, prompt_dlgproc, (LPARAM)&arg);
    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_get_Image(IHTMLWindow2 *iface, IHTMLImageElementFactory **p)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);
    HTMLInnerWindow *window = This->inner_window;

    TRACE("(%p)->(%p)\n", This, p);

    if(!window->image_factory) {
        HRESULT hres;

        hres = HTMLImageElementFactory_Create(window, &window->image_factory);
        if(FAILED(hres))
            return hres;
    }

    *p = &window->image_factory->IHTMLImageElementFactory_iface;
    IHTMLImageElementFactory_AddRef(*p);

    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_get_location(IHTMLWindow2 *iface, IHTMLLocation **p)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);
    HTMLLocation *location;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    hres = get_location(This->inner_window, &location);
    if(FAILED(hres))
        return hres;

    *p = &location->IHTMLLocation_iface;
    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_get_history(IHTMLWindow2 *iface, IOmHistory **p)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);
    HTMLInnerWindow *window = This->inner_window;

    TRACE("(%p)->(%p)\n", This, p);

    if(!window->history) {
        HRESULT hres;

        hres = create_history(window, &window->history);
        if(FAILED(hres))
            return hres;
    }

    IOmHistory_AddRef(&window->history->IOmHistory_iface);
    *p = &window->history->IOmHistory_iface;
    return S_OK;
}

static BOOL notify_webbrowser_close(HTMLOuterWindow *window, HTMLDocumentObj *doc)
{
    IConnectionPointContainer *cp_container;
    VARIANT_BOOL cancel = VARIANT_FALSE;
    IEnumConnections *enum_conn;
    VARIANT args[2];
    DISPPARAMS dp = {args, NULL, 2, 0};
    CONNECTDATA conn_data;
    IConnectionPoint *cp;
    IDispatch *disp;
    ULONG fetched;
    HRESULT hres;

    if(!doc->webbrowser)
        return TRUE;

    hres = IUnknown_QueryInterface(doc->webbrowser, &IID_IConnectionPointContainer, (void**)&cp_container);
    if(FAILED(hres))
        return TRUE;

    hres = IConnectionPointContainer_FindConnectionPoint(cp_container, &DIID_DWebBrowserEvents2, &cp);
    IConnectionPointContainer_Release(cp_container);
    if(FAILED(hres))
        return TRUE;

    hres = IConnectionPoint_EnumConnections(cp, &enum_conn);
    IConnectionPoint_Release(cp);
    if(FAILED(hres))
        return TRUE;

    while(!cancel) {
        conn_data.pUnk = NULL;
        conn_data.dwCookie = 0;
        fetched = 0;
        hres = IEnumConnections_Next(enum_conn, 1, &conn_data, &fetched);
        if(hres != S_OK)
            break;

        hres = IUnknown_QueryInterface(conn_data.pUnk, &IID_IDispatch, (void**)&disp);
        IUnknown_Release(conn_data.pUnk);
        if(FAILED(hres))
            continue;

        V_VT(args) = VT_BYREF|VT_BOOL;
        V_BOOLREF(args) = &cancel;
        V_VT(args+1) = VT_BOOL;
        V_BOOL(args+1) = window->parent ? VARIANT_TRUE : VARIANT_FALSE;
        hres = IDispatch_Invoke(disp, DISPID_WINDOWCLOSING, &IID_NULL, 0, DISPATCH_METHOD, &dp, NULL, NULL, NULL);
        IDispatch_Release(disp);
        if(FAILED(hres))
            cancel = VARIANT_FALSE;
    }

    IEnumConnections_Release(enum_conn);
    return !cancel;
}

static HRESULT WINAPI HTMLWindow2_close(IHTMLWindow2 *iface)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);
    HTMLOuterWindow *window = This->outer_window;

    TRACE("(%p)\n", This);

    if(!window->doc_obj) {
        FIXME("No document object\n");
        return E_FAIL;
    }

    if(!notify_webbrowser_close(window, window->doc_obj))
        return S_OK;

    FIXME("default action not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_put_opener(IHTMLWindow2 *iface, VARIANT v)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_get_opener(IHTMLWindow2 *iface, VARIANT *p)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);

    FIXME("(%p)->(%p) returning empty\n", This, p);

    V_VT(p) = VT_EMPTY;
    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_get_navigator(IHTMLWindow2 *iface, IOmNavigator **p)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = OmNavigator_Create();
    return *p ? S_OK : E_OUTOFMEMORY;
}

static HRESULT WINAPI HTMLWindow2_put_name(IHTMLWindow2 *iface, BSTR v)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);
    nsAString name_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&name_str, v);
    nsres = nsIDOMWindow_SetName(This->outer_window->nswindow, &name_str);
    nsAString_Finish(&name_str);
    if(NS_FAILED(nsres))
        ERR("SetName failed: %08x\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_get_name(IHTMLWindow2 *iface, BSTR *p)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);
    nsAString name_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&name_str, NULL);
    nsres = nsIDOMWindow_GetName(This->outer_window->nswindow, &name_str);
    return return_nsstr(nsres, &name_str, p);
}

static HRESULT WINAPI HTMLWindow2_get_parent(IHTMLWindow2 *iface, IHTMLWindow2 **p)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);
    HTMLOuterWindow *window = This->outer_window;

    TRACE("(%p)->(%p)\n", This, p);

    if(!window->parent)
        return IHTMLWindow2_get_self(&This->IHTMLWindow2_iface, p);

    *p = &window->parent->base.IHTMLWindow2_iface;
    IHTMLWindow2_AddRef(*p);
    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_open(IHTMLWindow2 *iface, BSTR url, BSTR name,
         BSTR features, VARIANT_BOOL replace, IHTMLWindow2 **pomWindowResult)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);
    HTMLOuterWindow *window = This->outer_window;
    INewWindowManager *new_window_mgr;
    BSTR uri_str;
    IUri *uri;
    HRESULT hres;

    static const WCHAR _selfW[] = {'_','s','e','l','f',0};

    TRACE("(%p)->(%s %s %s %x %p)\n", This, debugstr_w(url), debugstr_w(name),
          debugstr_w(features), replace, pomWindowResult);

    if(!window->doc_obj || !window->uri_nofrag)
        return E_UNEXPECTED;

    if(name && *name == '_') {
        if(!strcmpW(name, _selfW)) {
            if((features && *features) || replace)
                FIXME("Unsupported arguments for _self target\n");

            hres = IHTMLWindow2_navigate(&This->IHTMLWindow2_iface, url);
            if(FAILED(hres))
                return hres;

            if(pomWindowResult) {
                FIXME("Returning this window for _self target\n");
                *pomWindowResult = &This->IHTMLWindow2_iface;
                IHTMLWindow2_AddRef(*pomWindowResult);
            }

            return S_OK;
        }

        FIXME("Unsupported name %s\n", debugstr_w(name));
        return E_NOTIMPL;
    }

    hres = do_query_service((IUnknown*)window->doc_obj->client, &SID_SNewWindowManager, &IID_INewWindowManager,
            (void**)&new_window_mgr);
    if(FAILED(hres)) {
        FIXME("No INewWindowManager\n");
        return E_NOTIMPL;
    }

    hres = IUri_GetDisplayUri(window->uri_nofrag, &uri_str);
    if(SUCCEEDED(hres)) {
        hres = INewWindowManager_EvaluateNewWindow(new_window_mgr, url, name, uri_str,
                features, !!replace, window->doc_obj->has_popup ? 0 : NWMF_FIRST, 0);
        window->doc_obj->has_popup = TRUE;
        SysFreeString(uri_str);
    }
    INewWindowManager_Release(new_window_mgr);
    if(FAILED(hres)) {
        *pomWindowResult = NULL;
        return S_OK;
    }

    hres = create_relative_uri(window, url, &uri);
    if(FAILED(hres))
        return hres;

    hres = navigate_new_window(window, uri, name, NULL, pomWindowResult);
    IUri_Release(uri);
    return hres;
}

static HRESULT WINAPI HTMLWindow2_get_self(IHTMLWindow2 *iface, IHTMLWindow2 **p)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    /* FIXME: We should return kind of proxy window here. */
    IHTMLWindow2_AddRef(&This->IHTMLWindow2_iface);
    *p = &This->IHTMLWindow2_iface;
    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_get_top(IHTMLWindow2 *iface, IHTMLWindow2 **p)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);
    HTMLOuterWindow *top;

    TRACE("(%p)->(%p)\n", This, p);

    get_top_window(This->outer_window, &top);
    *p = &top->base.IHTMLWindow2_iface;
    IHTMLWindow2_AddRef(*p);

    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_get_window(IHTMLWindow2 *iface, IHTMLWindow2 **p)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    /* FIXME: We should return kind of proxy window here. */
    IHTMLWindow2_AddRef(&This->IHTMLWindow2_iface);
    *p = &This->IHTMLWindow2_iface;
    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_navigate(IHTMLWindow2 *iface, BSTR url)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(url));

    return navigate_url(This->outer_window, url, This->outer_window->uri, BINDING_NAVIGATED);
}

static HRESULT WINAPI HTMLWindow2_put_onfocus(IHTMLWindow2 *iface, VARIANT v)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_window_event(This, EVENTID_FOCUS, &v);
}

static HRESULT WINAPI HTMLWindow2_get_onfocus(IHTMLWindow2 *iface, VARIANT *p)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_window_event(This, EVENTID_FOCUS, p);
}

static HRESULT WINAPI HTMLWindow2_put_onblur(IHTMLWindow2 *iface, VARIANT v)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_window_event(This, EVENTID_BLUR, &v);
}

static HRESULT WINAPI HTMLWindow2_get_onblur(IHTMLWindow2 *iface, VARIANT *p)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_window_event(This, EVENTID_BLUR, p);
}

static HRESULT WINAPI HTMLWindow2_put_onload(IHTMLWindow2 *iface, VARIANT v)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_window_event(This, EVENTID_LOAD, &v);
}

static HRESULT WINAPI HTMLWindow2_get_onload(IHTMLWindow2 *iface, VARIANT *p)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_window_event(This, EVENTID_LOAD, p);
}

static HRESULT WINAPI HTMLWindow2_put_onbeforeunload(IHTMLWindow2 *iface, VARIANT v)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_window_event(This, EVENTID_BEFOREUNLOAD, &v);
}

static HRESULT WINAPI HTMLWindow2_get_onbeforeunload(IHTMLWindow2 *iface, VARIANT *p)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_window_event(This, EVENTID_BEFOREUNLOAD, p);
}

static HRESULT WINAPI HTMLWindow2_put_onunload(IHTMLWindow2 *iface, VARIANT v)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_window_event(This, EVENTID_UNLOAD, &v);
}

static HRESULT WINAPI HTMLWindow2_get_onunload(IHTMLWindow2 *iface, VARIANT *p)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_window_event(This, EVENTID_UNLOAD, p);
}

static HRESULT WINAPI HTMLWindow2_put_onhelp(IHTMLWindow2 *iface, VARIANT v)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_window_event(This, EVENTID_HELP, &v);
}

static HRESULT WINAPI HTMLWindow2_get_onhelp(IHTMLWindow2 *iface, VARIANT *p)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_window_event(This, EVENTID_HELP, p);
}

static HRESULT WINAPI HTMLWindow2_put_onerror(IHTMLWindow2 *iface, VARIANT v)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);

    FIXME("(%p)->(%s) semi-stub\n", This, debugstr_variant(&v));

    return set_window_event(This, EVENTID_ERROR, &v);
}

static HRESULT WINAPI HTMLWindow2_get_onerror(IHTMLWindow2 *iface, VARIANT *p)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_window_event(This, EVENTID_ERROR, p);
}

static HRESULT WINAPI HTMLWindow2_put_onresize(IHTMLWindow2 *iface, VARIANT v)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_window_event(This, EVENTID_RESIZE, &v);
}

static HRESULT WINAPI HTMLWindow2_get_onresize(IHTMLWindow2 *iface, VARIANT *p)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_window_event(This, EVENTID_RESIZE, p);
}

static HRESULT WINAPI HTMLWindow2_put_onscroll(IHTMLWindow2 *iface, VARIANT v)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_window_event(This, EVENTID_SCROLL, &v);
}

static HRESULT WINAPI HTMLWindow2_get_onscroll(IHTMLWindow2 *iface, VARIANT *p)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_window_event(This, EVENTID_SCROLL, p);
}

static HRESULT WINAPI HTMLWindow2_get_document(IHTMLWindow2 *iface, IHTMLDocument2 **p)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(This->inner_window->doc) {
        /* FIXME: We should return a wrapper object here */
        *p = &This->inner_window->doc->basedoc.IHTMLDocument2_iface;
        IHTMLDocument2_AddRef(*p);
    }else {
        *p = NULL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_get_event(IHTMLWindow2 *iface, IHTMLEventObj **p)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);
    HTMLInnerWindow *window = This->inner_window;

    TRACE("(%p)->(%p)\n", This, p);

    if(window->event)
        IHTMLEventObj_AddRef(window->event);
    *p = window->event;
    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_get__newEnum(IHTMLWindow2 *iface, IUnknown **p)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_showModalDialog(IHTMLWindow2 *iface, BSTR dialog,
        VARIANT *varArgIn, VARIANT *varOptions, VARIANT *varArgOut)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%s %p %p %p)\n", This, debugstr_w(dialog), varArgIn, varOptions, varArgOut);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_showHelp(IHTMLWindow2 *iface, BSTR helpURL, VARIANT helpArg,
        BSTR features)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%s %s %s)\n", This, debugstr_w(helpURL), debugstr_variant(&helpArg), debugstr_w(features));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_get_screen(IHTMLWindow2 *iface, IHTMLScreen **p)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);
    HTMLInnerWindow *window = This->inner_window;

    TRACE("(%p)->(%p)\n", This, p);

    if(!window->screen) {
        HRESULT hres;

        hres = HTMLScreen_Create(&window->screen);
        if(FAILED(hres))
            return hres;
    }

    *p = window->screen;
    IHTMLScreen_AddRef(window->screen);
    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_get_Option(IHTMLWindow2 *iface, IHTMLOptionElementFactory **p)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);
    HTMLInnerWindow *window = This->inner_window;

    TRACE("(%p)->(%p)\n", This, p);

    if(!window->option_factory) {
        HRESULT hres;

        hres = HTMLOptionElementFactory_Create(window, &window->option_factory);
        if(FAILED(hres))
            return hres;
    }

    *p = &window->option_factory->IHTMLOptionElementFactory_iface;
    IHTMLOptionElementFactory_AddRef(*p);

    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_focus(IHTMLWindow2 *iface)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);

    TRACE("(%p)->()\n", This);

    if(This->outer_window->doc_obj)
        SetFocus(This->outer_window->doc_obj->hwnd);
    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_get_closed(IHTMLWindow2 *iface, VARIANT_BOOL *p)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_blur(IHTMLWindow2 *iface)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_scroll(IHTMLWindow2 *iface, LONG x, LONG y)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);
    nsresult nsres;

    TRACE("(%p)->(%d %d)\n", This, x, y);

    nsres = nsIDOMWindow_Scroll(This->outer_window->nswindow, x, y);
    if(NS_FAILED(nsres)) {
        ERR("ScrollBy failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_get_clientInformation(IHTMLWindow2 *iface, IOmNavigator **p)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLWindow2_get_navigator(&This->IHTMLWindow2_iface, p);
}

static HRESULT WINAPI HTMLWindow2_setInterval(IHTMLWindow2 *iface, BSTR expression,
        LONG msec, VARIANT *language, LONG *timerID)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);
    VARIANT expr;

    TRACE("(%p)->(%s %d %p %p)\n", This, debugstr_w(expression), msec, language, timerID);

    V_VT(&expr) = VT_BSTR;
    V_BSTR(&expr) = expression;
    return IHTMLWindow3_setInterval(&This->IHTMLWindow3_iface, &expr, msec, language, timerID);
}

static HRESULT WINAPI HTMLWindow2_clearInterval(IHTMLWindow2 *iface, LONG timerID)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);

    TRACE("(%p)->(%d)\n", This, timerID);

    return clear_task_timer(This->inner_window, timerID);
}

static HRESULT WINAPI HTMLWindow2_put_offscreenBuffering(IHTMLWindow2 *iface, VARIANT v)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_get_offscreenBuffering(IHTMLWindow2 *iface, VARIANT *p)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_execScript(IHTMLWindow2 *iface, BSTR scode, BSTR language,
        VARIANT *pvarRet)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);

    TRACE("(%p)->(%s %s %p)\n", This, debugstr_w(scode), debugstr_w(language), pvarRet);

    return exec_script(This->inner_window, scode, language, pvarRet);
}

static HRESULT WINAPI HTMLWindow2_toString(IHTMLWindow2 *iface, BSTR *String)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);

    static const WCHAR objectW[] = {'[','o','b','j','e','c','t',']',0};

    TRACE("(%p)->(%p)\n", This, String);

    if(!String)
        return E_INVALIDARG;

    *String = SysAllocString(objectW);
    return *String ? S_OK : E_OUTOFMEMORY;
}

static HRESULT WINAPI HTMLWindow2_scrollBy(IHTMLWindow2 *iface, LONG x, LONG y)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);
    nsresult nsres;

    TRACE("(%p)->(%d %d)\n", This, x, y);

    nsres = nsIDOMWindow_ScrollBy(This->outer_window->nswindow, x, y);
    if(NS_FAILED(nsres))
        ERR("ScrollBy failed: %08x\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_scrollTo(IHTMLWindow2 *iface, LONG x, LONG y)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);
    nsresult nsres;

    TRACE("(%p)->(%d %d)\n", This, x, y);

    nsres = nsIDOMWindow_ScrollTo(This->outer_window->nswindow, x, y);
    if(NS_FAILED(nsres))
        ERR("ScrollTo failed: %08x\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_moveTo(IHTMLWindow2 *iface, LONG x, LONG y)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%d %d)\n", This, x, y);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_moveBy(IHTMLWindow2 *iface, LONG x, LONG y)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%d %d)\n", This, x, y);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_resizeTo(IHTMLWindow2 *iface, LONG x, LONG y)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%d %d)\n", This, x, y);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_resizeBy(IHTMLWindow2 *iface, LONG x, LONG y)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%d %d)\n", This, x, y);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_get_external(IHTMLWindow2 *iface, IDispatch **p)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = NULL;

    if(!This->outer_window->doc_obj->hostui)
        return S_OK;

    return IDocHostUIHandler_GetExternal(This->outer_window->doc_obj->hostui, p);
}

static const IHTMLWindow2Vtbl HTMLWindow2Vtbl = {
    HTMLWindow2_QueryInterface,
    HTMLWindow2_AddRef,
    HTMLWindow2_Release,
    HTMLWindow2_GetTypeInfoCount,
    HTMLWindow2_GetTypeInfo,
    HTMLWindow2_GetIDsOfNames,
    HTMLWindow2_Invoke,
    HTMLWindow2_item,
    HTMLWindow2_get_length,
    HTMLWindow2_get_frames,
    HTMLWindow2_put_defaultStatus,
    HTMLWindow2_get_defaultStatus,
    HTMLWindow2_put_status,
    HTMLWindow2_get_status,
    HTMLWindow2_setTimeout,
    HTMLWindow2_clearTimeout,
    HTMLWindow2_alert,
    HTMLWindow2_confirm,
    HTMLWindow2_prompt,
    HTMLWindow2_get_Image,
    HTMLWindow2_get_location,
    HTMLWindow2_get_history,
    HTMLWindow2_close,
    HTMLWindow2_put_opener,
    HTMLWindow2_get_opener,
    HTMLWindow2_get_navigator,
    HTMLWindow2_put_name,
    HTMLWindow2_get_name,
    HTMLWindow2_get_parent,
    HTMLWindow2_open,
    HTMLWindow2_get_self,
    HTMLWindow2_get_top,
    HTMLWindow2_get_window,
    HTMLWindow2_navigate,
    HTMLWindow2_put_onfocus,
    HTMLWindow2_get_onfocus,
    HTMLWindow2_put_onblur,
    HTMLWindow2_get_onblur,
    HTMLWindow2_put_onload,
    HTMLWindow2_get_onload,
    HTMLWindow2_put_onbeforeunload,
    HTMLWindow2_get_onbeforeunload,
    HTMLWindow2_put_onunload,
    HTMLWindow2_get_onunload,
    HTMLWindow2_put_onhelp,
    HTMLWindow2_get_onhelp,
    HTMLWindow2_put_onerror,
    HTMLWindow2_get_onerror,
    HTMLWindow2_put_onresize,
    HTMLWindow2_get_onresize,
    HTMLWindow2_put_onscroll,
    HTMLWindow2_get_onscroll,
    HTMLWindow2_get_document,
    HTMLWindow2_get_event,
    HTMLWindow2_get__newEnum,
    HTMLWindow2_showModalDialog,
    HTMLWindow2_showHelp,
    HTMLWindow2_get_screen,
    HTMLWindow2_get_Option,
    HTMLWindow2_focus,
    HTMLWindow2_get_closed,
    HTMLWindow2_blur,
    HTMLWindow2_scroll,
    HTMLWindow2_get_clientInformation,
    HTMLWindow2_setInterval,
    HTMLWindow2_clearInterval,
    HTMLWindow2_put_offscreenBuffering,
    HTMLWindow2_get_offscreenBuffering,
    HTMLWindow2_execScript,
    HTMLWindow2_toString,
    HTMLWindow2_scrollBy,
    HTMLWindow2_scrollTo,
    HTMLWindow2_moveTo,
    HTMLWindow2_moveBy,
    HTMLWindow2_resizeTo,
    HTMLWindow2_resizeBy,
    HTMLWindow2_get_external
};

static inline HTMLWindow *impl_from_IHTMLWindow3(IHTMLWindow3 *iface)
{
    return CONTAINING_RECORD(iface, HTMLWindow, IHTMLWindow3_iface);
}

static HRESULT WINAPI HTMLWindow3_QueryInterface(IHTMLWindow3 *iface, REFIID riid, void **ppv)
{
    HTMLWindow *This = impl_from_IHTMLWindow3(iface);

    return IHTMLWindow2_QueryInterface(&This->IHTMLWindow2_iface, riid, ppv);
}

static ULONG WINAPI HTMLWindow3_AddRef(IHTMLWindow3 *iface)
{
    HTMLWindow *This = impl_from_IHTMLWindow3(iface);

    return IHTMLWindow2_AddRef(&This->IHTMLWindow2_iface);
}

static ULONG WINAPI HTMLWindow3_Release(IHTMLWindow3 *iface)
{
    HTMLWindow *This = impl_from_IHTMLWindow3(iface);

    return IHTMLWindow2_Release(&This->IHTMLWindow2_iface);
}

static HRESULT WINAPI HTMLWindow3_GetTypeInfoCount(IHTMLWindow3 *iface, UINT *pctinfo)
{
    HTMLWindow *This = impl_from_IHTMLWindow3(iface);

    return IDispatchEx_GetTypeInfoCount(&This->IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLWindow3_GetTypeInfo(IHTMLWindow3 *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLWindow *This = impl_from_IHTMLWindow3(iface);

    return IDispatchEx_GetTypeInfo(&This->IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLWindow3_GetIDsOfNames(IHTMLWindow3 *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLWindow *This = impl_from_IHTMLWindow3(iface);

    return IDispatchEx_GetIDsOfNames(&This->IDispatchEx_iface, riid, rgszNames, cNames, lcid,
            rgDispId);
}

static HRESULT WINAPI HTMLWindow3_Invoke(IHTMLWindow3 *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLWindow *This = impl_from_IHTMLWindow3(iface);

    return IDispatchEx_Invoke(&This->IDispatchEx_iface, dispIdMember, riid, lcid, wFlags,
            pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLWindow3_get_screenLeft(IHTMLWindow3 *iface, LONG *p)
{
    HTMLWindow *This = impl_from_IHTMLWindow3(iface);
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMWindow_GetScreenX(This->outer_window->nswindow, p);
    if(NS_FAILED(nsres)) {
        ERR("GetScreenX failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLWindow3_get_screenTop(IHTMLWindow3 *iface, LONG *p)
{
    HTMLWindow *This = impl_from_IHTMLWindow3(iface);
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMWindow_GetScreenY(This->outer_window->nswindow, p);
    if(NS_FAILED(nsres)) {
        ERR("GetScreenY failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLWindow3_attachEvent(IHTMLWindow3 *iface, BSTR event, IDispatch *pDisp, VARIANT_BOOL *pfResult)
{
    HTMLWindow *This = impl_from_IHTMLWindow3(iface);
    HTMLInnerWindow *window = This->inner_window;

    TRACE("(%p)->(%s %p %p)\n", This, debugstr_w(event), pDisp, pfResult);

    if(!window->doc) {
        FIXME("No document\n");
        return E_FAIL;
    }

    return attach_event(&window->event_target, event, pDisp, pfResult);
}

static HRESULT WINAPI HTMLWindow3_detachEvent(IHTMLWindow3 *iface, BSTR event, IDispatch *pDisp)
{
    HTMLWindow *This = impl_from_IHTMLWindow3(iface);
    HTMLInnerWindow *window = This->inner_window;

    TRACE("(%p)->()\n", This);

    if(!window->doc) {
        FIXME("No document\n");
        return E_FAIL;
    }

    return detach_event(&window->event_target, event, pDisp);
}

static HRESULT window_set_timer(HTMLInnerWindow *This, VARIANT *expr, LONG msec, VARIANT *language,
        BOOL interval, LONG *timer_id)
{
    IDispatch *disp = NULL;
    HRESULT hres;

    switch(V_VT(expr)) {
    case VT_DISPATCH:
        disp = V_DISPATCH(expr);
        IDispatch_AddRef(disp);
        break;

    case VT_BSTR:
        disp = script_parse_event(This->base.inner_window, V_BSTR(expr));
        break;

    default:
        FIXME("unimplemented expr %s\n", debugstr_variant(expr));
        return E_NOTIMPL;
    }

    if(!disp)
        return E_FAIL;

    hres = set_task_timer(This, msec, interval, disp, timer_id);
    IDispatch_Release(disp);

    return hres;
}

static HRESULT WINAPI HTMLWindow3_setTimeout(IHTMLWindow3 *iface, VARIANT *expression, LONG msec,
        VARIANT *language, LONG *timerID)
{
    HTMLWindow *This = impl_from_IHTMLWindow3(iface);

    TRACE("(%p)->(%s %d %s %p)\n", This, debugstr_variant(expression), msec, debugstr_variant(language), timerID);

    return window_set_timer(This->inner_window, expression, msec, language, FALSE, timerID);
}

static HRESULT WINAPI HTMLWindow3_setInterval(IHTMLWindow3 *iface, VARIANT *expression, LONG msec,
        VARIANT *language, LONG *timerID)
{
    HTMLWindow *This = impl_from_IHTMLWindow3(iface);

    TRACE("(%p)->(%p %d %p %p)\n", This, expression, msec, language, timerID);

    return window_set_timer(This->inner_window, expression, msec, language, TRUE, timerID);
}

static HRESULT WINAPI HTMLWindow3_print(IHTMLWindow3 *iface)
{
    HTMLWindow *This = impl_from_IHTMLWindow3(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow3_put_onbeforeprint(IHTMLWindow3 *iface, VARIANT v)
{
    HTMLWindow *This = impl_from_IHTMLWindow3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow3_get_onbeforeprint(IHTMLWindow3 *iface, VARIANT *p)
{
    HTMLWindow *This = impl_from_IHTMLWindow3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow3_put_onafterprint(IHTMLWindow3 *iface, VARIANT v)
{
    HTMLWindow *This = impl_from_IHTMLWindow3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow3_get_onafterprint(IHTMLWindow3 *iface, VARIANT *p)
{
    HTMLWindow *This = impl_from_IHTMLWindow3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow3_get_clipboardData(IHTMLWindow3 *iface, IHTMLDataTransfer **p)
{
    HTMLWindow *This = impl_from_IHTMLWindow3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow3_showModelessDialog(IHTMLWindow3 *iface, BSTR url,
        VARIANT *varArgIn, VARIANT *options, IHTMLWindow2 **pDialog)
{
    HTMLWindow *This = impl_from_IHTMLWindow3(iface);
    FIXME("(%p)->(%s %p %p %p)\n", This, debugstr_w(url), varArgIn, options, pDialog);
    return E_NOTIMPL;
}

static const IHTMLWindow3Vtbl HTMLWindow3Vtbl = {
    HTMLWindow3_QueryInterface,
    HTMLWindow3_AddRef,
    HTMLWindow3_Release,
    HTMLWindow3_GetTypeInfoCount,
    HTMLWindow3_GetTypeInfo,
    HTMLWindow3_GetIDsOfNames,
    HTMLWindow3_Invoke,
    HTMLWindow3_get_screenLeft,
    HTMLWindow3_get_screenTop,
    HTMLWindow3_attachEvent,
    HTMLWindow3_detachEvent,
    HTMLWindow3_setTimeout,
    HTMLWindow3_setInterval,
    HTMLWindow3_print,
    HTMLWindow3_put_onbeforeprint,
    HTMLWindow3_get_onbeforeprint,
    HTMLWindow3_put_onafterprint,
    HTMLWindow3_get_onafterprint,
    HTMLWindow3_get_clipboardData,
    HTMLWindow3_showModelessDialog
};

static inline HTMLWindow *impl_from_IHTMLWindow4(IHTMLWindow4 *iface)
{
    return CONTAINING_RECORD(iface, HTMLWindow, IHTMLWindow4_iface);
}

static HRESULT WINAPI HTMLWindow4_QueryInterface(IHTMLWindow4 *iface, REFIID riid, void **ppv)
{
    HTMLWindow *This = impl_from_IHTMLWindow4(iface);

    return IHTMLWindow2_QueryInterface(&This->IHTMLWindow2_iface, riid, ppv);
}

static ULONG WINAPI HTMLWindow4_AddRef(IHTMLWindow4 *iface)
{
    HTMLWindow *This = impl_from_IHTMLWindow4(iface);

    return IHTMLWindow2_AddRef(&This->IHTMLWindow2_iface);
}

static ULONG WINAPI HTMLWindow4_Release(IHTMLWindow4 *iface)
{
    HTMLWindow *This = impl_from_IHTMLWindow4(iface);

    return IHTMLWindow2_Release(&This->IHTMLWindow2_iface);
}

static HRESULT WINAPI HTMLWindow4_GetTypeInfoCount(IHTMLWindow4 *iface, UINT *pctinfo)
{
    HTMLWindow *This = impl_from_IHTMLWindow4(iface);

    return IDispatchEx_GetTypeInfoCount(&This->IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLWindow4_GetTypeInfo(IHTMLWindow4 *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLWindow *This = impl_from_IHTMLWindow4(iface);

    return IDispatchEx_GetTypeInfo(&This->IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLWindow4_GetIDsOfNames(IHTMLWindow4 *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLWindow *This = impl_from_IHTMLWindow4(iface);

    return IDispatchEx_GetIDsOfNames(&This->IDispatchEx_iface, riid, rgszNames, cNames, lcid,
            rgDispId);
}

static HRESULT WINAPI HTMLWindow4_Invoke(IHTMLWindow4 *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLWindow *This = impl_from_IHTMLWindow4(iface);

    return IDispatchEx_Invoke(&This->IDispatchEx_iface, dispIdMember, riid, lcid, wFlags,
            pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLWindow4_createPopup(IHTMLWindow4 *iface, VARIANT *varArgIn,
                            IDispatch **ppPopup)
{
    HTMLWindow *This = impl_from_IHTMLWindow4(iface);
    FIXME("(%p)->(%p %p)\n", This, varArgIn, ppPopup);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow4_get_frameElement(IHTMLWindow4 *iface, IHTMLFrameBase **p)
{
    HTMLWindow *This = impl_from_IHTMLWindow4(iface);
    TRACE("(%p)->(%p)\n", This, p);

    if(This->outer_window->frame_element) {
        *p = &This->outer_window->frame_element->IHTMLFrameBase_iface;
        IHTMLFrameBase_AddRef(*p);
    }else
        *p = NULL;

    return S_OK;
}

static const IHTMLWindow4Vtbl HTMLWindow4Vtbl = {
    HTMLWindow4_QueryInterface,
    HTMLWindow4_AddRef,
    HTMLWindow4_Release,
    HTMLWindow4_GetTypeInfoCount,
    HTMLWindow4_GetTypeInfo,
    HTMLWindow4_GetIDsOfNames,
    HTMLWindow4_Invoke,
    HTMLWindow4_createPopup,
    HTMLWindow4_get_frameElement
};

static inline HTMLWindow *impl_from_IHTMLWindow5(IHTMLWindow5 *iface)
{
    return CONTAINING_RECORD(iface, HTMLWindow, IHTMLWindow5_iface);
}

static HRESULT WINAPI HTMLWindow5_QueryInterface(IHTMLWindow5 *iface, REFIID riid, void **ppv)
{
    HTMLWindow *This = impl_from_IHTMLWindow5(iface);

    return IHTMLWindow2_QueryInterface(&This->IHTMLWindow2_iface, riid, ppv);
}

static ULONG WINAPI HTMLWindow5_AddRef(IHTMLWindow5 *iface)
{
    HTMLWindow *This = impl_from_IHTMLWindow5(iface);

    return IHTMLWindow2_AddRef(&This->IHTMLWindow2_iface);
}

static ULONG WINAPI HTMLWindow5_Release(IHTMLWindow5 *iface)
{
    HTMLWindow *This = impl_from_IHTMLWindow5(iface);

    return IHTMLWindow2_Release(&This->IHTMLWindow2_iface);
}

static HRESULT WINAPI HTMLWindow5_GetTypeInfoCount(IHTMLWindow5 *iface, UINT *pctinfo)
{
    HTMLWindow *This = impl_from_IHTMLWindow5(iface);

    return IDispatchEx_GetTypeInfoCount(&This->IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLWindow5_GetTypeInfo(IHTMLWindow5 *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLWindow *This = impl_from_IHTMLWindow5(iface);

    return IDispatchEx_GetTypeInfo(&This->IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLWindow5_GetIDsOfNames(IHTMLWindow5 *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLWindow *This = impl_from_IHTMLWindow5(iface);

    return IDispatchEx_GetIDsOfNames(&This->IDispatchEx_iface, riid, rgszNames, cNames, lcid,
            rgDispId);
}

static HRESULT WINAPI HTMLWindow5_Invoke(IHTMLWindow5 *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLWindow *This = impl_from_IHTMLWindow5(iface);

    return IDispatchEx_Invoke(&This->IDispatchEx_iface, dispIdMember, riid, lcid, wFlags,
            pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLWindow5_put_XMLHttpRequest(IHTMLWindow5 *iface, VARIANT v)
{
    HTMLWindow *This = impl_from_IHTMLWindow5(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow5_get_XMLHttpRequest(IHTMLWindow5 *iface, VARIANT *p)
{
    HTMLWindow *This = impl_from_IHTMLWindow5(iface);
    HTMLInnerWindow *window = This->inner_window;

    TRACE("(%p)->(%p)\n", This, p);

    if(!window->xhr_factory) {
        HRESULT hres;

        hres = HTMLXMLHttpRequestFactory_Create(window, &window->xhr_factory);
        if(FAILED(hres)) {
            return hres;
        }
    }

    V_VT(p) = VT_DISPATCH;
    V_DISPATCH(p) = (IDispatch*)&window->xhr_factory->IHTMLXMLHttpRequestFactory_iface;
    IDispatch_AddRef(V_DISPATCH(p));

    return S_OK;
}

static const IHTMLWindow5Vtbl HTMLWindow5Vtbl = {
    HTMLWindow5_QueryInterface,
    HTMLWindow5_AddRef,
    HTMLWindow5_Release,
    HTMLWindow5_GetTypeInfoCount,
    HTMLWindow5_GetTypeInfo,
    HTMLWindow5_GetIDsOfNames,
    HTMLWindow5_Invoke,
    HTMLWindow5_put_XMLHttpRequest,
    HTMLWindow5_get_XMLHttpRequest
};

static inline HTMLWindow *impl_from_IHTMLWindow6(IHTMLWindow6 *iface)
{
    return CONTAINING_RECORD(iface, HTMLWindow, IHTMLWindow6_iface);
}

static HRESULT WINAPI HTMLWindow6_QueryInterface(IHTMLWindow6 *iface, REFIID riid, void **ppv)
{
    HTMLWindow *This = impl_from_IHTMLWindow6(iface);

    return IHTMLWindow2_QueryInterface(&This->IHTMLWindow2_iface, riid, ppv);
}

static ULONG WINAPI HTMLWindow6_AddRef(IHTMLWindow6 *iface)
{
    HTMLWindow *This = impl_from_IHTMLWindow6(iface);

    return IHTMLWindow2_AddRef(&This->IHTMLWindow2_iface);
}

static ULONG WINAPI HTMLWindow6_Release(IHTMLWindow6 *iface)
{
    HTMLWindow *This = impl_from_IHTMLWindow6(iface);

    return IHTMLWindow2_Release(&This->IHTMLWindow2_iface);
}

static HRESULT WINAPI HTMLWindow6_GetTypeInfoCount(IHTMLWindow6 *iface, UINT *pctinfo)
{
    HTMLWindow *This = impl_from_IHTMLWindow6(iface);

    return IDispatchEx_GetTypeInfoCount(&This->IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLWindow6_GetTypeInfo(IHTMLWindow6 *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLWindow *This = impl_from_IHTMLWindow6(iface);

    return IDispatchEx_GetTypeInfo(&This->IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLWindow6_GetIDsOfNames(IHTMLWindow6 *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLWindow *This = impl_from_IHTMLWindow6(iface);

    return IDispatchEx_GetIDsOfNames(&This->IDispatchEx_iface, riid, rgszNames, cNames, lcid,
            rgDispId);
}

static HRESULT WINAPI HTMLWindow6_Invoke(IHTMLWindow6 *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLWindow *This = impl_from_IHTMLWindow6(iface);

    return IDispatchEx_Invoke(&This->IDispatchEx_iface, dispIdMember, riid, lcid, wFlags,
            pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLWindow6_put_XDomainRequest(IHTMLWindow6 *iface, VARIANT v)
{
    HTMLWindow *This = impl_from_IHTMLWindow6(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow6_get_XDomainRequest(IHTMLWindow6 *iface, VARIANT *p)
{
    HTMLWindow *This = impl_from_IHTMLWindow6(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow6_get_sessionStorage(IHTMLWindow6 *iface, IHTMLStorage **p)
{
    HTMLWindow *This = impl_from_IHTMLWindow6(iface);

    FIXME("(%p)->(%p)\n", This, p);

    if(!This->inner_window->session_storage) {
        HRESULT hres;

        hres = create_storage(&This->inner_window->session_storage);
        if(FAILED(hres))
            return hres;
    }

    IHTMLStorage_AddRef(This->inner_window->session_storage);
    *p = This->inner_window->session_storage;
    return S_OK;
}

static HRESULT WINAPI HTMLWindow6_get_localStorage(IHTMLWindow6 *iface, IHTMLStorage **p)
{
    HTMLWindow *This = impl_from_IHTMLWindow6(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow6_put_onhashchange(IHTMLWindow6 *iface, VARIANT v)
{
    HTMLWindow *This = impl_from_IHTMLWindow6(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow6_get_onhashchange(IHTMLWindow6 *iface, VARIANT *p)
{
    HTMLWindow *This = impl_from_IHTMLWindow6(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow6_get_maxConnectionsPerServer(IHTMLWindow6 *iface, LONG *p)
{
    HTMLWindow *This = impl_from_IHTMLWindow6(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow6_postMessage(IHTMLWindow6 *iface, BSTR msg, VARIANT targetOrigin)
{
    HTMLWindow *This = impl_from_IHTMLWindow6(iface);

    FIXME("(%p)->(%s %s) semi-stub\n", This, debugstr_w(msg), debugstr_variant(&targetOrigin));

    if(!This->inner_window->doc) {
        FIXME("No document\n");
        return E_FAIL;
    }

    fire_event(This->inner_window->doc, EVENTID_MESSAGE, TRUE, &This->inner_window->doc->node, NULL, NULL);
    return S_OK;
}

static HRESULT WINAPI HTMLWindow6_toStaticHTML(IHTMLWindow6 *iface, BSTR bstrHTML, BSTR *pbstrStaticHTML)
{
    HTMLWindow *This = impl_from_IHTMLWindow6(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(bstrHTML), pbstrStaticHTML);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow6_put_onmessage(IHTMLWindow6 *iface, VARIANT v)
{
    HTMLWindow *This = impl_from_IHTMLWindow6(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_window_event(This, EVENTID_MESSAGE, &v);
}

static HRESULT WINAPI HTMLWindow6_get_onmessage(IHTMLWindow6 *iface, VARIANT *p)
{
    HTMLWindow *This = impl_from_IHTMLWindow6(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_window_event(This, EVENTID_MESSAGE, p);
}

static HRESULT WINAPI HTMLWindow6_msWriteProfilerMark(IHTMLWindow6 *iface, BSTR bstrProfilerMark)
{
    HTMLWindow *This = impl_from_IHTMLWindow6(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(bstrProfilerMark));
    return E_NOTIMPL;
}

static const IHTMLWindow6Vtbl HTMLWindow6Vtbl = {
    HTMLWindow6_QueryInterface,
    HTMLWindow6_AddRef,
    HTMLWindow6_Release,
    HTMLWindow6_GetTypeInfoCount,
    HTMLWindow6_GetTypeInfo,
    HTMLWindow6_GetIDsOfNames,
    HTMLWindow6_Invoke,
    HTMLWindow6_put_XDomainRequest,
    HTMLWindow6_get_XDomainRequest,
    HTMLWindow6_get_sessionStorage,
    HTMLWindow6_get_localStorage,
    HTMLWindow6_put_onhashchange,
    HTMLWindow6_get_onhashchange,
    HTMLWindow6_get_maxConnectionsPerServer,
    HTMLWindow6_postMessage,
    HTMLWindow6_toStaticHTML,
    HTMLWindow6_put_onmessage,
    HTMLWindow6_get_onmessage,
    HTMLWindow6_msWriteProfilerMark
};

static inline HTMLWindow *impl_from_IHTMLPrivateWindow(IHTMLPrivateWindow *iface)
{
    return CONTAINING_RECORD(iface, HTMLWindow, IHTMLPrivateWindow_iface);
}

static HRESULT WINAPI HTMLPrivateWindow_QueryInterface(IHTMLPrivateWindow *iface, REFIID riid, void **ppv)
{
    HTMLWindow *This = impl_from_IHTMLPrivateWindow(iface);

    return IHTMLWindow2_QueryInterface(&This->IHTMLWindow2_iface, riid, ppv);
}

static ULONG WINAPI HTMLPrivateWindow_AddRef(IHTMLPrivateWindow *iface)
{
    HTMLWindow *This = impl_from_IHTMLPrivateWindow(iface);

    return IHTMLWindow2_AddRef(&This->IHTMLWindow2_iface);
}

static ULONG WINAPI HTMLPrivateWindow_Release(IHTMLPrivateWindow *iface)
{
    HTMLWindow *This = impl_from_IHTMLPrivateWindow(iface);

    return IHTMLWindow2_Release(&This->IHTMLWindow2_iface);
}

static HRESULT WINAPI HTMLPrivateWindow_SuperNavigate(IHTMLPrivateWindow *iface, BSTR url, BSTR arg2, BSTR arg3,
        BSTR arg4, VARIANT *post_data_var, VARIANT *headers_var, ULONG flags)
{
    HTMLWindow *This = impl_from_IHTMLPrivateWindow(iface);
    HTMLOuterWindow *window = This->outer_window;
    OLECHAR *translated_url = NULL;
    DWORD post_data_size = 0;
    BYTE *post_data = NULL;
    WCHAR *headers = NULL;
    IUri *uri;
    HRESULT hres;

    TRACE("(%p)->(%s %s %s %s %s %s %x)\n", This, debugstr_w(url), debugstr_w(arg2), debugstr_w(arg3), debugstr_w(arg4),
          debugstr_variant(post_data_var), debugstr_variant(headers_var), flags);

    if(window->doc_obj->hostui) {
        hres = IDocHostUIHandler_TranslateUrl(window->doc_obj->hostui, 0, url, &translated_url);
        if(hres != S_OK)
            translated_url = NULL;
    }

    hres = create_uri(translated_url ? translated_url : url, 0, &uri);
    CoTaskMemFree(translated_url);
    if(FAILED(hres))
        return hres;

    if(post_data_var) {
        if(V_VT(post_data_var) == (VT_ARRAY|VT_UI1)) {
            SafeArrayAccessData(V_ARRAY(post_data_var), (void**)&post_data);
            post_data_size = V_ARRAY(post_data_var)->rgsabound[0].cElements;
        }
    }

    if(headers_var && V_VT(headers_var) != VT_EMPTY && V_VT(headers_var) != VT_ERROR) {
        if(V_VT(headers_var) != VT_BSTR)
            return E_INVALIDARG;

        headers = V_BSTR(headers_var);
    }

    hres = super_navigate(window, uri, BINDING_NAVIGATED|BINDING_NOFRAG, headers, post_data, post_data_size);
    IUri_Release(uri);
    if(post_data)
        SafeArrayUnaccessData(V_ARRAY(post_data_var));

    return hres;
}

static HRESULT WINAPI HTMLPrivateWindow_GetPendingUrl(IHTMLPrivateWindow *iface, BSTR *url)
{
    HTMLWindow *This = impl_from_IHTMLPrivateWindow(iface);
    FIXME("(%p)->(%p)\n", This, url);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLPrivateWindow_SetPICSTarget(IHTMLPrivateWindow *iface, IOleCommandTarget *cmdtrg)
{
    HTMLWindow *This = impl_from_IHTMLPrivateWindow(iface);
    FIXME("(%p)->(%p)\n", This, cmdtrg);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLPrivateWindow_PICSComplete(IHTMLPrivateWindow *iface, int arg)
{
    HTMLWindow *This = impl_from_IHTMLPrivateWindow(iface);
    FIXME("(%p)->(%x)\n", This, arg);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLPrivateWindow_FindWindowByName(IHTMLPrivateWindow *iface, LPCWSTR name, IHTMLWindow2 **ret)
{
    HTMLWindow *This = impl_from_IHTMLPrivateWindow(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(name), ret);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLPrivateWindow_GetAddressBarUrl(IHTMLPrivateWindow *iface, BSTR *url)
{
    HTMLWindow *This = impl_from_IHTMLPrivateWindow(iface);
    TRACE("(%p)->(%p)\n", This, url);

    if(!url)
        return E_INVALIDARG;

    *url = SysAllocString(This->outer_window->url);
    return S_OK;
}

static const IHTMLPrivateWindowVtbl HTMLPrivateWindowVtbl = {
    HTMLPrivateWindow_QueryInterface,
    HTMLPrivateWindow_AddRef,
    HTMLPrivateWindow_Release,
    HTMLPrivateWindow_SuperNavigate,
    HTMLPrivateWindow_GetPendingUrl,
    HTMLPrivateWindow_SetPICSTarget,
    HTMLPrivateWindow_PICSComplete,
    HTMLPrivateWindow_FindWindowByName,
    HTMLPrivateWindow_GetAddressBarUrl
};

static inline HTMLWindow *impl_from_ITravelLogClient(ITravelLogClient *iface)
{
    return CONTAINING_RECORD(iface, HTMLWindow, ITravelLogClient_iface);
}

static HRESULT WINAPI TravelLogClient_QueryInterface(ITravelLogClient *iface, REFIID riid, void **ppv)
{
    HTMLWindow *This = impl_from_ITravelLogClient(iface);

    return IHTMLWindow2_QueryInterface(&This->IHTMLWindow2_iface, riid, ppv);
}

static ULONG WINAPI TravelLogClient_AddRef(ITravelLogClient *iface)
{
    HTMLWindow *This = impl_from_ITravelLogClient(iface);

    return IHTMLWindow2_AddRef(&This->IHTMLWindow2_iface);
}

static ULONG WINAPI TravelLogClient_Release(ITravelLogClient *iface)
{
    HTMLWindow *This = impl_from_ITravelLogClient(iface);

    return IHTMLWindow2_Release(&This->IHTMLWindow2_iface);
}

static HRESULT WINAPI TravelLogClient_FindWindowByIndex(ITravelLogClient *iface, DWORD dwID, IUnknown **ppunk)
{
    HTMLWindow *This = impl_from_ITravelLogClient(iface);

    FIXME("(%p)->(%d %p) semi-stub\n", This, dwID, ppunk);

    *ppunk = NULL;
    return E_FAIL;
}

static HRESULT WINAPI TravelLogClient_GetWindowData(ITravelLogClient *iface, IStream *pStream, LPWINDOWDATA pWinData)
{
    HTMLWindow *This = impl_from_ITravelLogClient(iface);
    FIXME("(%p)->(%p %p)\n", This, pStream, pWinData);
    return E_NOTIMPL;
}

static HRESULT WINAPI TravelLogClient_LoadHistoryPosition(ITravelLogClient *iface, LPWSTR pszUrlLocation, DWORD dwPosition)
{
    HTMLWindow *This = impl_from_ITravelLogClient(iface);
    FIXME("(%p)->(%s %d)\n", This, debugstr_w(pszUrlLocation), dwPosition);
    return E_NOTIMPL;
}

static const ITravelLogClientVtbl TravelLogClientVtbl = {
    TravelLogClient_QueryInterface,
    TravelLogClient_AddRef,
    TravelLogClient_Release,
    TravelLogClient_FindWindowByIndex,
    TravelLogClient_GetWindowData,
    TravelLogClient_LoadHistoryPosition
};

static inline HTMLWindow *impl_from_IObjectIdentity(IObjectIdentity *iface)
{
    return CONTAINING_RECORD(iface, HTMLWindow, IObjectIdentity_iface);
}

static HRESULT WINAPI ObjectIdentity_QueryInterface(IObjectIdentity *iface, REFIID riid, void **ppv)
{
    HTMLWindow *This = impl_from_IObjectIdentity(iface);

    return IHTMLWindow2_QueryInterface(&This->IHTMLWindow2_iface, riid, ppv);
}

static ULONG WINAPI ObjectIdentity_AddRef(IObjectIdentity *iface)
{
    HTMLWindow *This = impl_from_IObjectIdentity(iface);

    return IHTMLWindow2_AddRef(&This->IHTMLWindow2_iface);
}

static ULONG WINAPI ObjectIdentity_Release(IObjectIdentity *iface)
{
    HTMLWindow *This = impl_from_IObjectIdentity(iface);

    return IHTMLWindow2_Release(&This->IHTMLWindow2_iface);
}

static HRESULT WINAPI ObjectIdentity_IsEqualObject(IObjectIdentity *iface, IUnknown *unk)
{
    HTMLWindow *This = impl_from_IObjectIdentity(iface);
    IServiceProvider *sp;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, unk);

    hres = IUnknown_QueryInterface(unk, &IID_IServiceProvider, (void**)&sp);
    if(hres != S_OK)
        return hres;

    hres = &This->inner_window->base.IServiceProvider_iface==sp ||
        &This->outer_window->base.IServiceProvider_iface==sp ? S_OK : S_FALSE;
    IServiceProvider_Release(sp);
    return hres;
}

static const IObjectIdentityVtbl ObjectIdentityVtbl = {
    ObjectIdentity_QueryInterface,
    ObjectIdentity_AddRef,
    ObjectIdentity_Release,
    ObjectIdentity_IsEqualObject
};

static inline HTMLWindow *impl_from_IDispatchEx(IDispatchEx *iface)
{
    return CONTAINING_RECORD(iface, HTMLWindow, IDispatchEx_iface);
}

static HRESULT WINAPI WindowDispEx_QueryInterface(IDispatchEx *iface, REFIID riid, void **ppv)
{
    HTMLWindow *This = impl_from_IDispatchEx(iface);

    return IHTMLWindow2_QueryInterface(&This->IHTMLWindow2_iface, riid, ppv);
}

static ULONG WINAPI WindowDispEx_AddRef(IDispatchEx *iface)
{
    HTMLWindow *This = impl_from_IDispatchEx(iface);

    return IHTMLWindow2_AddRef(&This->IHTMLWindow2_iface);
}

static ULONG WINAPI WindowDispEx_Release(IDispatchEx *iface)
{
    HTMLWindow *This = impl_from_IDispatchEx(iface);

    return IHTMLWindow2_Release(&This->IHTMLWindow2_iface);
}

static HRESULT WINAPI WindowDispEx_GetTypeInfoCount(IDispatchEx *iface, UINT *pctinfo)
{
    HTMLWindow *This = impl_from_IDispatchEx(iface);

    TRACE("(%p)->(%p)\n", This, pctinfo);

    return IDispatchEx_GetTypeInfoCount(&This->inner_window->event_target.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI WindowDispEx_GetTypeInfo(IDispatchEx *iface, UINT iTInfo,
                                               LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLWindow *This = impl_from_IDispatchEx(iface);

    TRACE("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);

    return IDispatchEx_GetTypeInfo(&This->inner_window->event_target.dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI WindowDispEx_GetIDsOfNames(IDispatchEx *iface, REFIID riid,
                                                 LPOLESTR *rgszNames, UINT cNames,
                                                 LCID lcid, DISPID *rgDispId)
{
    HTMLWindow *This = impl_from_IDispatchEx(iface);
    UINT i;
    HRESULT hres;

    WARN("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);

    for(i=0; i < cNames; i++) {
        /* We shouldn't use script's IDispatchEx here, so we shouldn't use GetDispID */
        hres = IDispatchEx_GetDispID(&This->IDispatchEx_iface, rgszNames[i], 0, rgDispId+i);
        if(FAILED(hres))
            return hres;
    }

    return S_OK;
}

static HRESULT WINAPI WindowDispEx_Invoke(IDispatchEx *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLWindow *This = impl_from_IDispatchEx(iface);

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    /* FIXME: Use script dispatch */

    return IDispatchEx_Invoke(&This->inner_window->event_target.dispex.IDispatchEx_iface, dispIdMember, riid, lcid, wFlags,
            pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static global_prop_t *alloc_global_prop(HTMLInnerWindow *This, global_prop_type_t type, BSTR name)
{
    if(This->global_prop_cnt == This->global_prop_size) {
        global_prop_t *new_props;
        DWORD new_size;

        if(This->global_props) {
            new_size = This->global_prop_size*2;
            new_props = heap_realloc(This->global_props, new_size*sizeof(global_prop_t));
        }else {
            new_size = 16;
            new_props = heap_alloc(new_size*sizeof(global_prop_t));
        }
        if(!new_props)
            return NULL;
        This->global_props = new_props;
        This->global_prop_size = new_size;
    }

    This->global_props[This->global_prop_cnt].name = heap_strdupW(name);
    if(!This->global_props[This->global_prop_cnt].name)
        return NULL;

    This->global_props[This->global_prop_cnt].type = type;
    return This->global_props + This->global_prop_cnt++;
}

static inline DWORD prop_to_dispid(HTMLInnerWindow *This, global_prop_t *prop)
{
    return MSHTML_DISPID_CUSTOM_MIN + (prop-This->global_props);
}

HRESULT search_window_props(HTMLInnerWindow *This, BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    DWORD i;
    ScriptHost *script_host;
    DISPID id;

    for(i=0; i < This->global_prop_cnt; i++) {
        /* FIXME: case sensitivity */
        if(!strcmpW(This->global_props[i].name, bstrName)) {
            *pid = MSHTML_DISPID_CUSTOM_MIN+i;
            return S_OK;
        }
    }

    if(find_global_prop(This->base.inner_window, bstrName, grfdex, &script_host, &id)) {
        global_prop_t *prop;

        prop = alloc_global_prop(This, GLOBAL_SCRIPTVAR, bstrName);
        if(!prop)
            return E_OUTOFMEMORY;

        prop->script_host = script_host;
        prop->id = id;

        *pid = prop_to_dispid(This, prop);
        return S_OK;
    }

    return DISP_E_UNKNOWNNAME;
}

static HRESULT WINAPI WindowDispEx_GetDispID(IDispatchEx *iface, BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    HTMLWindow *This = impl_from_IDispatchEx(iface);
    HTMLInnerWindow *window = This->inner_window;
    HRESULT hres;

    TRACE("(%p)->(%s %x %p)\n", This, debugstr_w(bstrName), grfdex, pid);

    hres = search_window_props(window, bstrName, grfdex, pid);
    if(hres != DISP_E_UNKNOWNNAME)
        return hres;

    hres = IDispatchEx_GetDispID(&window->base.inner_window->event_target.dispex.IDispatchEx_iface, bstrName, grfdex, pid);
    if(hres != DISP_E_UNKNOWNNAME)
        return hres;

    if(This->outer_window) {
        HTMLOuterWindow *frame;

        hres = get_frame_by_name(This->outer_window, bstrName, FALSE, &frame);
        if(SUCCEEDED(hres) && frame) {
            global_prop_t *prop;

            IHTMLWindow2_Release(&frame->base.IHTMLWindow2_iface);

            prop = alloc_global_prop(window, GLOBAL_FRAMEVAR, bstrName);
            if(!prop)
                return E_OUTOFMEMORY;

            *pid = prop_to_dispid(window, prop);
            return S_OK;
        }
    }

    if(window->doc) {
        global_prop_t *prop;
        IHTMLElement *elem;

        hres = IHTMLDocument3_getElementById(&window->base.inner_window->doc->basedoc.IHTMLDocument3_iface,
                                             bstrName, &elem);
        if(SUCCEEDED(hres) && elem) {
            IHTMLElement_Release(elem);

            prop = alloc_global_prop(window, GLOBAL_ELEMENTVAR, bstrName);
            if(!prop)
                return E_OUTOFMEMORY;

            *pid = prop_to_dispid(window, prop);
            return S_OK;
        }
    }

    return DISP_E_UNKNOWNNAME;
}

static HRESULT WINAPI WindowDispEx_InvokeEx(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    HTMLWindow *This = impl_from_IDispatchEx(iface);
    HTMLInnerWindow *window = This->inner_window;

    TRACE("(%p)->(%x %x %x %p %p %p %p)\n", This, id, lcid, wFlags, pdp, pvarRes, pei, pspCaller);

    switch(id) {
    case DISPID_IHTMLWINDOW2_LOCATION: {
        HTMLLocation *location;
        HRESULT hres;

        if(!(wFlags & DISPATCH_PROPERTYPUT))
            break;

        TRACE("forwarding to location.href\n");

        hres = get_location(window, &location);
        if(FAILED(hres))
            return hres;

        hres = IDispatchEx_InvokeEx(&location->dispex.IDispatchEx_iface, DISPID_VALUE, lcid,
                wFlags, pdp, pvarRes, pei, pspCaller);
        IHTMLLocation_Release(&location->IHTMLLocation_iface);
        return hres;
    }
    case DISPID_IHTMLWINDOW2_SETTIMEOUT:
    case DISPID_IHTMLWINDOW3_SETTIMEOUT: {
        VARIANT args[2];
        DISPPARAMS dp = {args, NULL, 2, 0};

        /*
         * setTimeout calls should use default value 0 for the second argument if only one is provided,
         * but IDL file does not reflect that. We fixup arguments here instead.
         */
        if(!(wFlags & DISPATCH_METHOD) || pdp->cArgs != 1 || pdp->cNamedArgs)
            break;

        TRACE("Fixing args\n");

        V_VT(args) = VT_I4;
        V_I4(args) = 0;
        args[1] = *pdp->rgvarg;
        return IDispatchEx_InvokeEx(&window->event_target.dispex.IDispatchEx_iface, id, lcid,
                wFlags, &dp, pvarRes, pei, pspCaller);
    }
    }

    return IDispatchEx_InvokeEx(&window->event_target.dispex.IDispatchEx_iface, id, lcid, wFlags, pdp, pvarRes, pei, pspCaller);
}

static HRESULT WINAPI WindowDispEx_DeleteMemberByName(IDispatchEx *iface, BSTR bstrName, DWORD grfdex)
{
    HTMLWindow *This = impl_from_IDispatchEx(iface);

    TRACE("(%p)->(%s %x)\n", This, debugstr_w(bstrName), grfdex);

    return IDispatchEx_DeleteMemberByName(&This->inner_window->event_target.dispex.IDispatchEx_iface, bstrName, grfdex);
}

static HRESULT WINAPI WindowDispEx_DeleteMemberByDispID(IDispatchEx *iface, DISPID id)
{
    HTMLWindow *This = impl_from_IDispatchEx(iface);

    TRACE("(%p)->(%x)\n", This, id);

    return IDispatchEx_DeleteMemberByDispID(&This->inner_window->event_target.dispex.IDispatchEx_iface, id);
}

static HRESULT WINAPI WindowDispEx_GetMemberProperties(IDispatchEx *iface, DISPID id, DWORD grfdexFetch, DWORD *pgrfdex)
{
    HTMLWindow *This = impl_from_IDispatchEx(iface);

    TRACE("(%p)->(%x %x %p)\n", This, id, grfdexFetch, pgrfdex);

    return IDispatchEx_GetMemberProperties(&This->inner_window->event_target.dispex.IDispatchEx_iface, id, grfdexFetch,
            pgrfdex);
}

static HRESULT WINAPI WindowDispEx_GetMemberName(IDispatchEx *iface, DISPID id, BSTR *pbstrName)
{
    HTMLWindow *This = impl_from_IDispatchEx(iface);

    TRACE("(%p)->(%x %p)\n", This, id, pbstrName);

    return IDispatchEx_GetMemberName(&This->inner_window->event_target.dispex.IDispatchEx_iface, id, pbstrName);
}

static HRESULT WINAPI WindowDispEx_GetNextDispID(IDispatchEx *iface, DWORD grfdex, DISPID id, DISPID *pid)
{
    HTMLWindow *This = impl_from_IDispatchEx(iface);

    TRACE("(%p)->(%x %x %p)\n", This, grfdex, id, pid);

    return IDispatchEx_GetNextDispID(&This->inner_window->event_target.dispex.IDispatchEx_iface, grfdex, id, pid);
}

static HRESULT WINAPI WindowDispEx_GetNameSpaceParent(IDispatchEx *iface, IUnknown **ppunk)
{
    HTMLWindow *This = impl_from_IDispatchEx(iface);

    TRACE("(%p)->(%p)\n", This, ppunk);

    *ppunk = NULL;
    return S_OK;
}

static const IDispatchExVtbl WindowDispExVtbl = {
    WindowDispEx_QueryInterface,
    WindowDispEx_AddRef,
    WindowDispEx_Release,
    WindowDispEx_GetTypeInfoCount,
    WindowDispEx_GetTypeInfo,
    WindowDispEx_GetIDsOfNames,
    WindowDispEx_Invoke,
    WindowDispEx_GetDispID,
    WindowDispEx_InvokeEx,
    WindowDispEx_DeleteMemberByName,
    WindowDispEx_DeleteMemberByDispID,
    WindowDispEx_GetMemberProperties,
    WindowDispEx_GetMemberName,
    WindowDispEx_GetNextDispID,
    WindowDispEx_GetNameSpaceParent
};

static inline HTMLWindow *impl_from_IServiceProvider(IServiceProvider *iface)
{
    return CONTAINING_RECORD(iface, HTMLWindow, IServiceProvider_iface);
}

static HRESULT WINAPI HTMLWindowSP_QueryInterface(IServiceProvider *iface, REFIID riid, void **ppv)
{
    HTMLWindow *This = impl_from_IServiceProvider(iface);
    return IHTMLWindow2_QueryInterface(&This->IHTMLWindow2_iface, riid, ppv);
}

static ULONG WINAPI HTMLWindowSP_AddRef(IServiceProvider *iface)
{
    HTMLWindow *This = impl_from_IServiceProvider(iface);
    return IHTMLWindow2_AddRef(&This->IHTMLWindow2_iface);
}

static ULONG WINAPI HTMLWindowSP_Release(IServiceProvider *iface)
{
    HTMLWindow *This = impl_from_IServiceProvider(iface);
    return IHTMLWindow2_Release(&This->IHTMLWindow2_iface);
}

static HRESULT WINAPI HTMLWindowSP_QueryService(IServiceProvider *iface, REFGUID guidService, REFIID riid, void **ppv)
{
    HTMLWindow *This = impl_from_IServiceProvider(iface);

    if(IsEqualGUID(guidService, &IID_IHTMLWindow2)) {
        TRACE("IID_IHTMLWindow2\n");
        return IHTMLWindow2_QueryInterface(&This->IHTMLWindow2_iface, riid, ppv);
    }

    TRACE("(%p)->(%s %s %p)\n", This, debugstr_mshtml_guid(guidService), debugstr_mshtml_guid(riid), ppv);

    if(!This->outer_window->doc_obj)
        return E_NOINTERFACE;

    return IServiceProvider_QueryService(&This->outer_window->doc_obj->basedoc.IServiceProvider_iface,
            guidService, riid, ppv);
}

static const IServiceProviderVtbl ServiceProviderVtbl = {
    HTMLWindowSP_QueryInterface,
    HTMLWindowSP_AddRef,
    HTMLWindowSP_Release,
    HTMLWindowSP_QueryService
};

static inline HTMLInnerWindow *impl_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, HTMLInnerWindow, event_target.dispex);
}

static HRESULT HTMLWindow_invoke(DispatchEx *dispex, DISPID id, LCID lcid, WORD flags, DISPPARAMS *params,
        VARIANT *res, EXCEPINFO *ei, IServiceProvider *caller)
{
    HTMLInnerWindow *This = impl_from_DispatchEx(dispex);
    global_prop_t *prop;
    DWORD idx;
    HRESULT hres;

    idx = id - MSHTML_DISPID_CUSTOM_MIN;
    if(idx >= This->global_prop_cnt)
        return DISP_E_MEMBERNOTFOUND;

    prop = This->global_props+idx;

    switch(prop->type) {
    case GLOBAL_SCRIPTVAR: {
        IDispatchEx *iface;
        IDispatch *disp;

        disp = get_script_disp(prop->script_host);
        if(!disp)
            return E_UNEXPECTED;

        hres = IDispatch_QueryInterface(disp, &IID_IDispatchEx, (void**)&iface);
        if(SUCCEEDED(hres)) {
            TRACE("%s >>>\n", debugstr_w(prop->name));
            hres = IDispatchEx_InvokeEx(iface, prop->id, lcid, flags, params, res, ei, caller);
            if(hres == S_OK)
                TRACE("%s <<<\n", debugstr_w(prop->name));
            else
                WARN("%s <<< %08x\n", debugstr_w(prop->name), hres);
            IDispatchEx_Release(iface);
        }else {
            FIXME("No IDispatchEx\n");
        }
        IDispatch_Release(disp);
        break;
    }
    case GLOBAL_ELEMENTVAR:
        switch(flags) {
        case DISPATCH_PROPERTYGET|DISPATCH_METHOD:
        case DISPATCH_PROPERTYGET: {
            IHTMLElement *elem;

            hres = IHTMLDocument3_getElementById(&This->base.inner_window->doc->basedoc.IHTMLDocument3_iface,
                    prop->name, &elem);
            if(FAILED(hres))
                return hres;

            if(!elem)
                return DISP_E_MEMBERNOTFOUND;

            V_VT(res) = VT_DISPATCH;
            V_DISPATCH(res) = (IDispatch*)elem;
            return S_OK;
        }
        case DISPATCH_PROPERTYPUT: {
            DISPID dispex_id;

            hres = dispex_get_dynid(&This->event_target.dispex, prop->name, &dispex_id);
            if(FAILED(hres))
                return hres;

            prop->type = GLOBAL_DISPEXVAR;
            prop->id = dispex_id;
            return IDispatchEx_InvokeEx(&This->event_target.dispex.IDispatchEx_iface, dispex_id, 0, flags, params, res, ei, caller);
        }
        default:
            FIXME("Not supported flags: %x\n", flags);
            return E_NOTIMPL;
        }
    case GLOBAL_FRAMEVAR:
        if(!This->base.outer_window)
            return E_UNEXPECTED;

        switch(flags) {
        case DISPATCH_PROPERTYGET: {
            HTMLOuterWindow *frame;

            hres = get_frame_by_name(This->base.outer_window, prop->name, FALSE, &frame);
            if(FAILED(hres))
                return hres;

            if(!frame)
                return DISP_E_MEMBERNOTFOUND;

            V_VT(res) = VT_DISPATCH;
            V_DISPATCH(res) = (IDispatch*)&frame->base.inner_window->base.IHTMLWindow2_iface;
            IDispatch_AddRef(V_DISPATCH(res));
            return S_OK;
        }
        default:
            FIXME("Not supported flags: %x\n", flags);
            return E_NOTIMPL;
        }
    case GLOBAL_DISPEXVAR:
        return IDispatchEx_InvokeEx(&This->event_target.dispex.IDispatchEx_iface, prop->id, 0, flags, params, res, ei, caller);
    default:
        ERR("invalid type %d\n", prop->type);
        hres = DISP_E_MEMBERNOTFOUND;
    }

    return hres;
}

static event_target_t **HTMLWindow_get_event_target_ptr(DispatchEx *dispex)
{
    HTMLInnerWindow *This = impl_from_DispatchEx(dispex);
    return &This->doc->body_event_target;
}

static void HTMLWindow_bind_event(DispatchEx *dispex, int eid)
{
    HTMLInnerWindow *This = impl_from_DispatchEx(dispex);
    This->doc->node.event_target.dispex.data->vtbl->bind_event(&This->doc->node.event_target.dispex, eid);
}

static const dispex_static_data_vtbl_t HTMLWindow_dispex_vtbl = {
    NULL,
    NULL,
    HTMLWindow_invoke,
    NULL,
    HTMLWindow_get_event_target_ptr,
    HTMLWindow_bind_event
};

static const tid_t HTMLWindow_iface_tids[] = {
    IHTMLWindow2_tid,
    IHTMLWindow3_tid,
    IHTMLWindow4_tid,
    IHTMLWindow6_tid,
    0
};

static dispex_static_data_t HTMLWindow_dispex = {
    &HTMLWindow_dispex_vtbl,
    DispHTMLWindow2_tid,
    NULL,
    HTMLWindow_iface_tids,
    IHTMLWindow5_tid
};

static void *alloc_window(size_t size)
{
    HTMLWindow *window;

    window = heap_alloc_zero(size);
    if(!window)
        return NULL;

    window->IHTMLWindow2_iface.lpVtbl = &HTMLWindow2Vtbl;
    window->IHTMLWindow3_iface.lpVtbl = &HTMLWindow3Vtbl;
    window->IHTMLWindow4_iface.lpVtbl = &HTMLWindow4Vtbl;
    window->IHTMLWindow5_iface.lpVtbl = &HTMLWindow5Vtbl;
    window->IHTMLWindow6_iface.lpVtbl = &HTMLWindow6Vtbl;
    window->IHTMLPrivateWindow_iface.lpVtbl = &HTMLPrivateWindowVtbl;
    window->IDispatchEx_iface.lpVtbl = &WindowDispExVtbl;
    window->IServiceProvider_iface.lpVtbl = &ServiceProviderVtbl;
    window->ITravelLogClient_iface.lpVtbl = &TravelLogClientVtbl;
    window->IObjectIdentity_iface.lpVtbl = &ObjectIdentityVtbl;
    window->ref = 1;

    return window;
}

static HRESULT create_inner_window(HTMLOuterWindow *outer_window, IMoniker *mon, HTMLInnerWindow **ret)
{
    HTMLInnerWindow *window;

    window = alloc_window(sizeof(HTMLInnerWindow));
    if(!window)
        return E_OUTOFMEMORY;

    list_init(&window->script_hosts);
    list_init(&window->bindings);
    list_init(&window->script_queue);

    window->base.outer_window = outer_window;
    window->base.inner_window = window;

    init_dispex(&window->event_target.dispex, (IUnknown*)&window->base.IHTMLWindow2_iface, &HTMLWindow_dispex);

    window->task_magic = get_task_target_magic();

    if(mon) {
        IMoniker_AddRef(mon);
        window->mon = mon;
    }

    *ret = window;
    return S_OK;
}

HRESULT HTMLOuterWindow_Create(HTMLDocumentObj *doc_obj, nsIDOMWindow *nswindow,
        HTMLOuterWindow *parent, HTMLOuterWindow **ret)
{
    HTMLOuterWindow *window;
    HRESULT hres;

    window = alloc_window(sizeof(HTMLOuterWindow));
    if(!window)
        return E_OUTOFMEMORY;

    window->base.outer_window = window;
    window->base.inner_window = NULL;

    window->window_ref = heap_alloc(sizeof(windowref_t));
    if(!window->window_ref) {
        heap_free(window);
        return E_OUTOFMEMORY;
    }

    window->doc_obj = doc_obj;

    window->window_ref->window = window;
    window->window_ref->ref = 1;

    if(nswindow) {
        nsresult nsres;

        nsIDOMWindow_AddRef(nswindow);
        window->nswindow = nswindow;

        nsres = nsIDOMWindow_QueryInterface(nswindow, &IID_mozIDOMWindowProxy, (void**)&window->window_proxy);
        assert(nsres == NS_OK);
    }

    window->scriptmode = parent ? parent->scriptmode : SCRIPTMODE_GECKO;
    window->readystate = READYSTATE_UNINITIALIZED;

    hres = create_pending_window(window, NULL);
    if(SUCCEEDED(hres))
        hres = update_window_doc(window->pending_window);
    if(FAILED(hres)) {
        IHTMLWindow2_Release(&window->base.IHTMLWindow2_iface);
        return hres;
    }

    hres = CoInternetCreateSecurityManager(NULL, &window->secmgr, 0);
    if(FAILED(hres)) {
        IHTMLWindow2_Release(&window->base.IHTMLWindow2_iface);
        return hres;
    }

    window->task_magic = get_task_target_magic();

    list_init(&window->children);
    list_add_head(&window_list, &window->entry);

    if(parent) {
        IHTMLWindow2_AddRef(&window->base.IHTMLWindow2_iface);

        window->parent = parent;
        list_add_tail(&parent->children, &window->sibling_entry);
    }

    TRACE("%p inner_window %p\n", window, window->base.inner_window);

    *ret = window;
    return S_OK;
}

HRESULT create_pending_window(HTMLOuterWindow *outer_window, nsChannelBSC *channelbsc)
{
    HTMLInnerWindow *pending_window;
    HRESULT hres;

    hres = create_inner_window(outer_window, outer_window->mon /* FIXME */, &pending_window);
    if(FAILED(hres))
        return hres;

    if(channelbsc) {
        IBindStatusCallback_AddRef(&channelbsc->bsc.IBindStatusCallback_iface);
        pending_window->bscallback = channelbsc;
    }

    if(outer_window->pending_window) {
        abort_window_bindings(outer_window->pending_window);
        outer_window->pending_window->base.outer_window = NULL;
        IHTMLWindow2_Release(&outer_window->pending_window->base.IHTMLWindow2_iface);
    }

    outer_window->pending_window = pending_window;
    return S_OK;
}

HRESULT update_window_doc(HTMLInnerWindow *window)
{
    HTMLOuterWindow *outer_window = window->base.outer_window;
    nsIDOMHTMLDocument *nshtmldoc;
    nsIDOMDocument *nsdoc;
    nsresult nsres;
    HRESULT hres;

    assert(!window->doc);

    if(!outer_window) {
        ERR("NULL outer window\n");
        return E_UNEXPECTED;
    }

    nsres = nsIDOMWindow_GetDocument(outer_window->nswindow, &nsdoc);
    if(NS_FAILED(nsres) || !nsdoc) {
        ERR("GetDocument failed: %08x\n", nsres);
        return E_FAIL;
    }

    nsres = nsIDOMDocument_QueryInterface(nsdoc, &IID_nsIDOMHTMLDocument, (void**)&nshtmldoc);
    nsIDOMDocument_Release(nsdoc);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIDOMHTMLDocument iface: %08x\n", nsres);
        return E_FAIL;
    }

    hres = create_doc_from_nsdoc(nshtmldoc, outer_window->doc_obj, window, &window->doc);
    nsIDOMHTMLDocument_Release(nshtmldoc);
    if(FAILED(hres))
        return hres;

    if(outer_window->doc_obj->usermode == EDITMODE) {
        nsAString mode_str;
        nsresult nsres;

        static const PRUnichar onW[] = {'o','n',0};

        nsAString_InitDepend(&mode_str, onW);
        nsres = nsIDOMHTMLDocument_SetDesignMode(window->doc->nsdoc, &mode_str);
        nsAString_Finish(&mode_str);
        if(NS_FAILED(nsres))
            ERR("SetDesignMode failed: %08x\n", nsres);
    }

    if(window != outer_window->pending_window) {
        ERR("not current pending window\n");
        return S_OK;
    }

    if(outer_window->base.inner_window)
        detach_inner_window(outer_window->base.inner_window);
    outer_window->base.inner_window = window;
    outer_window->pending_window = NULL;

    if(outer_window->doc_obj->basedoc.window == outer_window || !outer_window->doc_obj->basedoc.window) {
        if(outer_window->doc_obj->basedoc.doc_node)
            htmldoc_release(&outer_window->doc_obj->basedoc.doc_node->basedoc);
        outer_window->doc_obj->basedoc.doc_node = window->doc;
        htmldoc_addref(&window->doc->basedoc);
    }

    return hres;
}

HTMLOuterWindow *nswindow_to_window(const nsIDOMWindow *nswindow)
{
    HTMLOuterWindow *iter;

    LIST_FOR_EACH_ENTRY(iter, &window_list, HTMLOuterWindow, entry) {
        if(iter->nswindow == nswindow)
            return iter;
    }

    return NULL;
}

HTMLOuterWindow *mozwindow_to_window(const mozIDOMWindowProxy *mozwindow)
{
    HTMLOuterWindow *iter;

    LIST_FOR_EACH_ENTRY(iter, &window_list, HTMLOuterWindow, entry) {
        if(iter->window_proxy == mozwindow)
            return iter;
    }

    return NULL;
}
