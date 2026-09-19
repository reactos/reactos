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

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "mshtmdid.h"
#include "wininet.h"
#include "shlguid.h"
#include "shobjidl.h"
#include "activscp.h"
#include "exdispid.h"

#define NO_SHLWAPI_REG
#include "shlwapi.h"

#include "wine/debug.h"

#include "mshtml_private.h"
#include "htmlevent.h"
#include "htmlscript.h"
#include "htmlstyle.h"
#include "pluginhost.h"
#include "binding.h"
#include "resource.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

static ExternalCycleCollectionParticipant outer_window_ccp;

static int window_map_compare(const void *key, const struct wine_rb_entry *entry)
{
    HTMLOuterWindow *window = WINE_RB_ENTRY_VALUE(entry, HTMLOuterWindow, entry);

    if(window->window_proxy == key)
        return 0;
    return (void*)window->window_proxy > key ? -1 : 1;
}

static struct wine_rb_tree window_map = { window_map_compare };

HTMLOuterWindow *mozwindow_to_window(const mozIDOMWindowProxy *mozwindow)
{
    struct wine_rb_entry *entry = wine_rb_get(&window_map, mozwindow);
    return entry ? WINE_RB_ENTRY_VALUE(entry, HTMLOuterWindow, entry) : NULL;
}

static HRESULT get_location(HTMLOuterWindow *This, HTMLLocation **ret)
{
    if(!This->location) {
        HRESULT hres = create_location(This, &This->location);
        if(FAILED(hres))
            return hres;
    }

    IHTMLLocation_AddRef(&This->location->IHTMLLocation_iface);
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
    HTMLDocumentNode *doc = window->doc, *doc_iter;

    while(!list_empty(&window->children)) {
        HTMLOuterWindow *child = LIST_ENTRY(list_tail(&window->children), HTMLOuterWindow, sibling_entry);

        list_remove(&child->sibling_entry);
        child->parent = NULL;

        if(child->base.inner_window)
            detach_inner_window(child->base.inner_window);

        IHTMLWindow2_Release(&child->base.IHTMLWindow2_iface);
    }

    if(outer_window && is_main_content_window(outer_window))
        window->doc->cp_container.forward_container = NULL;

    LIST_FOR_EACH_ENTRY(doc_iter, &window->documents, HTMLDocumentNode, script_global_entry)
        doc_iter->script_global = NULL;
    if(doc)
        detach_document_node(doc);

    if(outer_window && outer_window->location)
        dispex_props_unlink(&outer_window->location->dispex);

    abort_window_bindings(window);
    release_script_hosts(window);
    unlink_ref(&window->jscript);
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

static inline HTMLOuterWindow *HTMLOuterWindow_from_IHTMLWindow2(IHTMLWindow2 *iface)
{
    return CONTAINING_RECORD(iface, HTMLOuterWindow, base.IHTMLWindow2_iface);
}

static void *base_query_interface(HTMLWindow *This, REFIID riid)
{
    if(IsEqualGUID(&IID_IUnknown, riid))
        return &This->IHTMLWindow2_iface;
    if(IsEqualGUID(&IID_IDispatch, riid))
        return &This->IHTMLWindow2_iface;
    if(IsEqualGUID(&IID_IHTMLFramesCollection2, riid))
        return &This->IHTMLWindow2_iface;
    if(IsEqualGUID(&IID_IHTMLWindow2, riid))
        return &This->IHTMLWindow2_iface;
    if(IsEqualGUID(&IID_IHTMLWindow3, riid))
        return &This->IHTMLWindow3_iface;
    if(IsEqualGUID(&IID_IHTMLWindow4, riid))
        return &This->IHTMLWindow4_iface;
    if(IsEqualGUID(&IID_IHTMLWindow5, riid))
        return &This->IHTMLWindow5_iface;
    if(IsEqualGUID(&IID_IHTMLWindow6, riid))
        return &This->IHTMLWindow6_iface;
    if(IsEqualGUID(&IID_IHTMLWindow7, riid))
        return &This->IHTMLWindow7_iface;
    if(IsEqualGUID(&IID_IHTMLPrivateWindow, riid))
        return &This->IHTMLPrivateWindow_iface;
    if(IsEqualGUID(&IID_IServiceProvider, riid))
        return &This->IServiceProvider_iface;
    if(IsEqualGUID(&IID_ITravelLogClient, riid))
        return &This->ITravelLogClient_iface;
    if(IsEqualGUID(&IID_IObjectIdentity, riid))
        return &This->IObjectIdentity_iface;
    if(IsEqualGUID(&IID_IProvideClassInfo, riid))
        return &This->IProvideMultipleClassInfo_iface;
    if(IsEqualGUID(&IID_IProvideClassInfo2, riid))
        return &This->IProvideMultipleClassInfo_iface;
    if(IsEqualGUID(&IID_IProvideMultipleClassInfo, riid))
        return &This->IProvideMultipleClassInfo_iface;
    if(IsEqualGUID(&IID_IWineHTMLWindowPrivate, riid))
        return &This->IWineHTMLWindowPrivate_iface;
    if(IsEqualGUID(&IID_IWineHTMLWindowCompatPrivate, riid))
        return &This->IWineHTMLWindowCompatPrivate_iface;
    if(IsEqualGUID(&IID_IMarshal, riid)) {
        FIXME("(%p)->(IID_IMarshal)\n", This);
        return NULL;
    }

    return NULL;
}

static HRESULT WINAPI outer_window_QueryInterface(IHTMLWindow2 *iface, REFIID riid, void **ppv)
{
    HTMLOuterWindow *This = HTMLOuterWindow_from_IHTMLWindow2(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(&IID_nsXPCOMCycleCollectionParticipant, riid)) {
        *ppv = &outer_window_ccp;
        return S_OK;
    }else if(IsEqualGUID(&IID_nsCycleCollectionISupports, riid)) {
        *ppv = &This->base.IHTMLWindow2_iface;
        return S_OK;
    }else if(IsEqualGUID(&IID_IDispatchEx, riid) || IsEqualGUID(&IID_IWineJSDispatchHost, riid)) {
        *ppv = &This->IWineJSDispatchHost_iface;
    }else if(IsEqualGUID(&IID_IEventTarget, riid)) {
        if(!This->base.inner_window->doc || This->base.inner_window->doc->document_mode < COMPAT_MODE_IE9)
            return E_NOINTERFACE;
        *ppv = &This->IEventTarget_iface;
    }else {
        *ppv = base_query_interface(&This->base, riid);
    }

    if(!*ppv) {
        WARN("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*ppv);
    return S_OK;
}

static ULONG WINAPI outer_window_AddRef(IHTMLWindow2 *iface)
{
    HTMLOuterWindow *This = HTMLOuterWindow_from_IHTMLWindow2(iface);
    LONG ref = ccref_incr(&This->ccref, (nsISupports*)&This->base.IHTMLWindow2_iface);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI outer_window_Release(IHTMLWindow2 *iface)
{
    HTMLOuterWindow *This = HTMLOuterWindow_from_IHTMLWindow2(iface);
    LONG task_magic = This->task_magic;
    LONG ref = ccref_decr(&This->ccref, (nsISupports*)&This->base.IHTMLWindow2_iface, &outer_window_ccp);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref)
        remove_target_tasks(task_magic);

    return ref;
}

DISPEX_IDISPATCH_IMPL(HTMLWindow2, IHTMLWindow2,
                      impl_from_IHTMLWindow2(iface)->inner_window->event_target.dispex)

static HRESULT get_frame_by_index(HTMLOuterWindow *This, UINT32 index, HTMLOuterWindow **ret)
{
    nsIDOMWindowCollection *nsframes;
    mozIDOMWindowProxy *mozwindow;
    UINT32 length;
    nsresult nsres;

    nsres = nsIDOMWindow_GetFrames(This->nswindow, &nsframes);
    if(NS_FAILED(nsres)) {
        FIXME("nsIDOMWindow_GetFrames failed: 0x%08lx\n", nsres);
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
        FIXME("nsIDOMWindowCollection_Item failed: 0x%08lx\n", nsres);
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
        FIXME("nsIDOMWindow_GetFrames failed: 0x%08lx\n", nsres);
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
            FIXME("nsIDOMWindowCollection_Item failed: 0x%08lx\n", nsres);
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
            FIXME("IHTMLElement_get_id failed: 0x%08lx\n", hres);
            break;
        }

        if(id && !wcsicmp(id, name))
            window = window_iter;

        SysFreeString(id);

        if(!window && deep)
            get_frame_by_name(window_iter, name, TRUE, &window);
    }

    nsIDOMWindowCollection_Release(nsframes);
    if(FAILED(hres))
        return hres;

    *ret = window;
    return S_OK;
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
        ERR("GetFrames failed: %08lx\n", nsres);
        return E_FAIL;
    }

    nsres = nsIDOMWindowCollection_GetLength(nscollection, &length);
    nsIDOMWindowCollection_Release(nscollection);
    if(NS_FAILED(nsres)) {
        ERR("GetLength failed: %08lx\n", nsres);
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
    *p = (IHTMLFramesCollection2*)&This->outer_window->base.IHTMLWindow2_iface;
    IHTMLWindow2_AddRef(&This->outer_window->base.IHTMLWindow2_iface);
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

    TRACE("(%p)->(%s %ld %p %p)\n", This, debugstr_w(expression), msec, language, timerID);

    V_VT(&expr_var) = VT_BSTR;
    V_BSTR(&expr_var) = expression;

    return IHTMLWindow3_setTimeout(&This->IHTMLWindow3_iface, &expr_var, msec, language, timerID);
}

static HRESULT WINAPI HTMLWindow2_clearTimeout(IHTMLWindow2 *iface, LONG timerID)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);

    TRACE("(%p)->(%ld)\n", This, timerID);

    return clear_task_timer(This->inner_window, timerID);
}

#define MAX_MESSAGE_LEN 2000

static HRESULT WINAPI HTMLWindow2_alert(IHTMLWindow2 *iface, BSTR message)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);
    WCHAR title[100], *msg = message;
    DWORD len;

    TRACE("(%p)->(%s)\n", This, debugstr_w(message));

    if(!This->outer_window->browser)
        return E_UNEXPECTED;

    if(!LoadStringW(get_shdoclc(), IDS_MESSAGE_BOX_TITLE, title, ARRAY_SIZE(title))) {
        WARN("Could not load message box title: %ld\n", GetLastError());
        return S_OK;
    }

    len = SysStringLen(message);
    if(len > MAX_MESSAGE_LEN) {
        msg = malloc((MAX_MESSAGE_LEN + 1) * sizeof(WCHAR));
        if(!msg)
            return E_OUTOFMEMORY;
        memcpy(msg, message, MAX_MESSAGE_LEN*sizeof(WCHAR));
        msg[MAX_MESSAGE_LEN] = 0;
    }

    MessageBoxW(This->outer_window->browser->doc->hwnd, msg, title, MB_ICONWARNING);
    if(msg != message)
        free(msg);
    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_confirm(IHTMLWindow2 *iface, BSTR message,
        VARIANT_BOOL *confirmed)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);
    WCHAR wszTitle[100];

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(message), confirmed);

    if(!confirmed)
        return E_INVALIDARG;
    if(!This->outer_window->browser)
        return E_UNEXPECTED;

    if(!LoadStringW(get_shdoclc(), IDS_MESSAGE_BOX_TITLE, wszTitle, ARRAY_SIZE(wszTitle))) {
        WARN("Could not load message box title: %ld\n", GetLastError());
        *confirmed = VARIANT_TRUE;
        return S_OK;
    }

    if(MessageBoxW(This->outer_window->browser->doc->hwnd, message, wszTitle,
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

            if(!LoadStringW(get_shdoclc(), IDS_MESSAGE_BOX_TITLE, wszTitle, ARRAY_SIZE(wszTitle))) {
                WARN("Could not load message box title: %ld\n", GetLastError());
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

    if(!This->outer_window->browser)
        return E_UNEXPECTED;

    if(textdata) V_VT(textdata) = VT_NULL;

    arg.message = message;
    arg.dststr = dststr;
    arg.textdata = textdata;

    DialogBoxParamW(hInst, MAKEINTRESOURCEW(ID_PROMPT_DIALOG),
            This->outer_window->browser->doc->hwnd, prompt_dlgproc, (LPARAM)&arg);
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

    hres = get_location(This->outer_window, &location);
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
        V_BOOL(args+1) = variant_bool(window->parent != NULL);
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

    if(!window->browser) {
        FIXME("No document object\n");
        return E_FAIL;
    }

    if(!notify_webbrowser_close(window, window->browser->doc))
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
    HTMLInnerWindow *window = This->inner_window;

    TRACE("(%p)->(%p)\n", This, p);

    if(!window->navigator) {
        HRESULT hres;
        hres = create_navigator(window, &window->navigator);
        if(FAILED(hres))
            return hres;
    }

    IOmNavigator_AddRef(*p = window->navigator);
    return S_OK;
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
        ERR("SetName failed: %08lx\n", nsres);

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
    IUri *uri;
    HRESULT hres;

    TRACE("(%p)->(%s %s %s %x %p)\n", This, debugstr_w(url), debugstr_w(name),
          debugstr_w(features), replace, pomWindowResult);
    if(features)
        FIXME("unsupported features argument %s\n", debugstr_w(features));
    if(replace)
        FIXME("unsupported relace argument\n");

    if(!window->browser || !window->uri_nofrag)
        return E_UNEXPECTED;

    if(name && *name == '_') {
        if(!wcscmp(name, L"_self")) {
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
    *p = &This->outer_window->base.IHTMLWindow2_iface;
    IHTMLWindow2_AddRef(*p);
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
    *p = &This->outer_window->base.IHTMLWindow2_iface;
    IHTMLWindow2_AddRef(*p);
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
        *p = &This->inner_window->doc->IHTMLDocument2_iface;
        IHTMLDocument2_AddRef(*p);
    }else {
        *p = NULL;
    }

    return S_OK;
}

IHTMLEventObj *default_set_current_event(HTMLInnerWindow *window, IHTMLEventObj *event_obj)
{
    IHTMLEventObj *prev_event = NULL;

    if(window) {
        if(event_obj)
            IHTMLEventObj_AddRef(event_obj);
        prev_event = window->event;
        window->event = event_obj;
    }

    return prev_event;
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

        hres = create_html_screen(window, &window->screen);
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

    if(!This->outer_window->browser)
        return E_UNEXPECTED;

    SetFocus(This->outer_window->browser->doc->hwnd);
    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_get_closed(IHTMLWindow2 *iface, VARIANT_BOOL *p)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);

    TRACE("(%p)->(%p) semi-stub\n", This, p);

    *p = VARIANT_FALSE;
    return S_OK;
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

    TRACE("(%p)->(%ld %ld)\n", This, x, y);

    nsres = nsIDOMWindow_Scroll(This->outer_window->nswindow, x, y);
    if(NS_FAILED(nsres)) {
        ERR("ScrollBy failed: %08lx\n", nsres);
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

    TRACE("(%p)->(%s %ld %p %p)\n", This, debugstr_w(expression), msec, language, timerID);

    V_VT(&expr) = VT_BSTR;
    V_BSTR(&expr) = expression;
    return IHTMLWindow3_setInterval(&This->IHTMLWindow3_iface, &expr, msec, language, timerID);
}

static HRESULT WINAPI HTMLWindow2_clearInterval(IHTMLWindow2 *iface, LONG timerID)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);

    TRACE("(%p)->(%ld)\n", This, timerID);

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

    TRACE("(%p)->(%p)\n", This, String);

    if(!String)
        return E_INVALIDARG;

    *String = SysAllocString(L"[object Window]");
    return *String ? S_OK : E_OUTOFMEMORY;
}

static HRESULT WINAPI HTMLWindow2_scrollBy(IHTMLWindow2 *iface, LONG x, LONG y)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);
    nsresult nsres;

    TRACE("(%p)->(%ld %ld)\n", This, x, y);

    nsres = nsIDOMWindow_ScrollBy(This->outer_window->nswindow, x, y);
    if(NS_FAILED(nsres))
        ERR("ScrollBy failed: %08lx\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_scrollTo(IHTMLWindow2 *iface, LONG x, LONG y)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);
    nsresult nsres;

    TRACE("(%p)->(%ld %ld)\n", This, x, y);

    nsres = nsIDOMWindow_ScrollTo(This->outer_window->nswindow, x, y);
    if(NS_FAILED(nsres))
        ERR("ScrollTo failed: %08lx\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_moveTo(IHTMLWindow2 *iface, LONG x, LONG y)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%ld %ld)\n", This, x, y);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_moveBy(IHTMLWindow2 *iface, LONG x, LONG y)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%ld %ld)\n", This, x, y);
    return S_FALSE;
}

static HRESULT WINAPI HTMLWindow2_resizeTo(IHTMLWindow2 *iface, LONG x, LONG y)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%ld %ld)\n", This, x, y);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_resizeBy(IHTMLWindow2 *iface, LONG x, LONG y)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);
    FIXME("(%p)->(%ld %ld)\n", This, x, y);
    return S_FALSE;
}

static HRESULT WINAPI HTMLWindow2_get_external(IHTMLWindow2 *iface, IDispatch **p)
{
    HTMLWindow *This = impl_from_IHTMLWindow2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->outer_window->browser)
        return E_UNEXPECTED;

    *p = NULL;

    if(!This->outer_window->browser->doc->hostui)
        return S_OK;

    return IDocHostUIHandler_GetExternal(This->outer_window->browser->doc->hostui, p);
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

static const IHTMLWindow2Vtbl outer_window_HTMLWindow2Vtbl = {
    outer_window_QueryInterface,
    outer_window_AddRef,
    outer_window_Release,
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

DISPEX_IDISPATCH_NOUNK_IMPL(HTMLWindow3, IHTMLWindow3,
                            impl_from_IHTMLWindow3(iface)->inner_window->event_target.dispex)

static HRESULT WINAPI HTMLWindow3_get_screenLeft(IHTMLWindow3 *iface, LONG *p)
{
    HTMLWindow *This = impl_from_IHTMLWindow3(iface);
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMWindow_GetScreenX(This->outer_window->nswindow, p);
    if(NS_FAILED(nsres)) {
        ERR("GetScreenX failed: %08lx\n", nsres);
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
        ERR("GetScreenY failed: %08lx\n", nsres);
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
        enum timer_type timer_type, LONG *timer_id)
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

    hres = set_task_timer(This, msec, timer_type, disp, timer_id);
    IDispatch_Release(disp);

    return hres;
}

static HRESULT WINAPI HTMLWindow3_setTimeout(IHTMLWindow3 *iface, VARIANT *expression, LONG msec,
        VARIANT *language, LONG *timerID)
{
    HTMLWindow *This = impl_from_IHTMLWindow3(iface);

    TRACE("(%p)->(%s %ld %s %p)\n", This, debugstr_variant(expression), msec, debugstr_variant(language), timerID);

    return window_set_timer(This->inner_window, expression, msec, language, TIMER_TIMEOUT, timerID);
}

static HRESULT WINAPI HTMLWindow3_setInterval(IHTMLWindow3 *iface, VARIANT *expression, LONG msec,
        VARIANT *language, LONG *timerID)
{
    HTMLWindow *This = impl_from_IHTMLWindow3(iface);

    TRACE("(%p)->(%p %ld %p %p)\n", This, expression, msec, language, timerID);

    return window_set_timer(This->inner_window, expression, msec, language, TIMER_INTERVAL, timerID);
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
    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));
    return set_window_event(This, EVENTID_BEFOREPRINT, &v);
}

static HRESULT WINAPI HTMLWindow3_get_onbeforeprint(IHTMLWindow3 *iface, VARIANT *p)
{
    HTMLWindow *This = impl_from_IHTMLWindow3(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_window_event(This, EVENTID_BEFOREPRINT, p);
}

static HRESULT WINAPI HTMLWindow3_put_onafterprint(IHTMLWindow3 *iface, VARIANT v)
{
    HTMLWindow *This = impl_from_IHTMLWindow3(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));
    return set_window_event(This, EVENTID_AFTERPRINT, &v);
}

static HRESULT WINAPI HTMLWindow3_get_onafterprint(IHTMLWindow3 *iface, VARIANT *p)
{
    HTMLWindow *This = impl_from_IHTMLWindow3(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_window_event(This, EVENTID_AFTERPRINT, p);
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

DISPEX_IDISPATCH_NOUNK_IMPL(HTMLWindow4, IHTMLWindow4,
                            impl_from_IHTMLWindow4(iface)->inner_window->event_target.dispex)

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

DISPEX_IDISPATCH_NOUNK_IMPL(HTMLWindow5, IHTMLWindow5,
                            impl_from_IHTMLWindow5(iface)->inner_window->event_target.dispex)

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

    if(This->outer_window->readystate == READYSTATE_UNINITIALIZED) {
        V_VT(p) = VT_EMPTY;
        return S_OK;
    }

    if(!window->constructors[PROT_XMLHttpRequest]) {
        HRESULT hres;

        hres = HTMLXMLHttpRequestFactory_Create(window, &window->constructors[PROT_XMLHttpRequest]);
        if(FAILED(hres))
            return hres;
    }

    V_VT(p) = VT_DISPATCH;
    V_DISPATCH(p) = (IDispatch*)&window->constructors[PROT_XMLHttpRequest]->IWineJSDispatchHost_iface;
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

DISPEX_IDISPATCH_NOUNK_IMPL(HTMLWindow6, IHTMLWindow6,
                            impl_from_IHTMLWindow6(iface)->inner_window->event_target.dispex)

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

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->inner_window->session_storage) {
        HRESULT hres;

        hres = create_html_storage(This->inner_window, FALSE, &This->inner_window->session_storage);
        if(hres != S_OK) {
            *p = NULL;
            return hres;
        }
    }

    IHTMLStorage_AddRef(This->inner_window->session_storage);
    *p = This->inner_window->session_storage;
    return S_OK;
}

static HRESULT WINAPI HTMLWindow6_get_localStorage(IHTMLWindow6 *iface, IHTMLStorage **p)
{
    HTMLWindow *This = impl_from_IHTMLWindow6(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->inner_window->local_storage) {
        HRESULT hres;

        hres = create_html_storage(This->inner_window, TRUE, &This->inner_window->local_storage);
        if(hres != S_OK) {
            *p = NULL;
            return hres;
        }
    }

    IHTMLStorage_AddRef(This->inner_window->local_storage);
    *p = This->inner_window->local_storage;
    return S_OK;
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

static HRESULT check_target_origin(HTMLInnerWindow *window, const WCHAR *target_origin)
{
    BOOL no_port = FALSE;
    IUri *uri, *target;
    DWORD port, port2;
    BSTR bstr, bstr2;
    HRESULT hres;

    if(!target_origin)
        return E_INVALIDARG;

    if(!wcscmp(target_origin, L"*"))
        return S_OK;

    hres = create_uri(target_origin, Uri_CREATE_NOFRAG | Uri_CREATE_NO_DECODE_EXTRA_INFO, &target);
    if(FAILED(hres))
        return hres;

    if(!(uri = window->base.outer_window->uri)) {
        FIXME("window with no URI\n");
        hres = S_FALSE;
        goto done;
    }

    bstr = NULL;
    hres = IUri_GetSchemeName(uri, &bstr);
    if(hres != S_OK) {
        SysFreeString(bstr);
        goto done;
    }
    hres = IUri_GetSchemeName(target, &bstr2);
    if(SUCCEEDED(hres)) {
        if(hres == S_OK && wcsicmp(bstr, bstr2))
            hres = S_FALSE;
        SysFreeString(bstr2);
    }
    SysFreeString(bstr);
    if(hres != S_OK)
        goto done;

    bstr = NULL;
    hres = IUri_GetHost(uri, &bstr);
    if(hres != S_OK) {
        SysFreeString(bstr);
        goto done;
    }
    hres = IUri_GetHost(target, &bstr2);
    if(SUCCEEDED(hres)) {
        if(hres == S_OK && wcsicmp(bstr, bstr2))
            hres = S_FALSE;
        SysFreeString(bstr2);
    }
    SysFreeString(bstr);
    if(hres != S_OK)
        goto done;

    /* Legacy modes ignore port */
    if(dispex_compat_mode(&window->event_target.dispex) < COMPAT_MODE_IE9)
        goto done;

    hres = IUri_GetPort(uri, &port);
    if(hres != S_OK) {
        if(FAILED(hres))
            goto done;
        no_port = TRUE;    /* some protocols don't have ports (e.g. res) */
    }
    hres = IUri_GetPort(target, &port2);
    if(hres != S_OK) {
        if(FAILED(hres))
            goto done;
        if(no_port)
            hres = S_OK;
    }else if(no_port || port != port2) {
        hres = S_FALSE;
    }

done:
    IUri_Release(target);
    return hres;
}

static IHTMLWindow2 *get_source_window(IServiceProvider *caller, compat_mode_t compat_mode)
{
    IOleCommandTarget *cmdtarget, *parent_cmdtarget;
    IServiceProvider *parent;
    IHTMLWindow2 *source;
    HRESULT hres;
    VARIANT var;

    if(!caller)
        return NULL;

    hres = IServiceProvider_QueryService(caller, &IID_IActiveScriptSite, &IID_IOleCommandTarget, (void**)&cmdtarget);
    if(hres != S_OK)
        cmdtarget = NULL;

    hres = IServiceProvider_QueryService(caller, &SID_GetCaller, &IID_IServiceProvider, (void**)&parent);
    if(hres == S_OK && parent) {
        hres = IServiceProvider_QueryService(parent, &IID_IActiveScriptSite, &IID_IOleCommandTarget, (void**)&parent_cmdtarget);
        IServiceProvider_Release(parent);
        if(hres == S_OK && parent_cmdtarget) {
            if(cmdtarget)
                IOleCommandTarget_Release(cmdtarget);
            cmdtarget = parent_cmdtarget;
        }
    }

    if(!cmdtarget)
        return NULL;

    V_VT(&var) = VT_EMPTY;
    hres = IOleCommandTarget_Exec(cmdtarget, &CGID_ScriptSite, CMDID_SCRIPTSITE_SECURITY_WINDOW, 0, NULL, &var);
    IOleCommandTarget_Release(cmdtarget);
    if(hres != S_OK)
        return NULL;

    /* Native assumes it's VT_DISPATCH and doesn't check it */
    hres = IDispatch_QueryInterface(V_DISPATCH(&var), &IID_IHTMLWindow2, (void**)&source);
    IDispatch_Release(V_DISPATCH(&var));
    if(hres != S_OK)
        return NULL;

    if(compat_mode < COMPAT_MODE_IE9) {
        IHTMLWindow2 *tmp;
        hres = IHTMLWindow2_get_self(source, &tmp);
        if(hres == S_OK) {
            IHTMLWindow2_Release(source);
            source = tmp;
        }
    }
    return source;
}

struct post_message_task {
    event_task_t header;
    DOMEvent *event;
};

static void post_message_proc(event_task_t *_task)
{
    struct post_message_task *task = (struct post_message_task *)_task;
    dispatch_event(&task->header.window->event_target, task->event);
}

static void post_message_destr(event_task_t *_task)
{
    struct post_message_task *task = (struct post_message_task *)_task;
    IDOMEvent_Release(&task->event->IDOMEvent_iface);
}

static HRESULT post_message(HTMLInnerWindow *window, VARIANT msg, BSTR targetOrigin, VARIANT transfer,
        IServiceProvider *caller, compat_mode_t compat_mode)
{
    IHTMLWindow2 *source;
    DOMEvent *event;
    HRESULT hres;

    if(V_VT(&transfer) != VT_EMPTY && V_VT(&transfer) != VT_ERROR)
        FIXME("transfer not implemented, ignoring\n");

    hres = check_target_origin(window, targetOrigin);
    if(hres != S_OK)
        return SUCCEEDED(hres) ? S_OK : hres;

    source = get_source_window(caller, compat_mode);
    if(!source) {
        if(compat_mode < COMPAT_MODE_IE9)
            return E_ABORT;
        IHTMLWindow2_AddRef(source = &window->base.outer_window->base.IHTMLWindow2_iface);
    }

    switch(V_VT(&msg)) {
        case VT_EMPTY:
        case VT_NULL:
        case VT_VOID:
        case VT_I1:
        case VT_I2:
        case VT_I4:
        case VT_I8:
        case VT_UI1:
        case VT_UI2:
        case VT_UI4:
        case VT_UI8:
        case VT_INT:
        case VT_UINT:
        case VT_R4:
        case VT_R8:
        case VT_BOOL:
        case VT_BSTR:
        case VT_CY:
        case VT_DATE:
        case VT_DECIMAL:
        case VT_HRESULT:
            break;
        case VT_ERROR:
            V_VT(&msg) = VT_EMPTY;
            break;
        default:
            FIXME("Unsupported vt %d\n", V_VT(&msg));
            IHTMLWindow2_Release(source);
            return E_NOTIMPL;
    }

    if(!window->doc) {
        FIXME("No document\n");
        IHTMLWindow2_Release(source);
        return E_FAIL;
    }

    hres = create_message_event(window->doc, source, &msg, &event);
    IHTMLWindow2_Release(source);
    if(FAILED(hres))
        return hres;

    if(compat_mode >= COMPAT_MODE_IE9) {
        struct post_message_task *task;
        if(!(task = malloc(sizeof(*task)))) {
            IDOMEvent_Release(&event->IDOMEvent_iface);
            return E_OUTOFMEMORY;
        }

        /* Because message events can be sent to different windows, they get blocked by any context */
        task->header.thread_blocked = TRUE;
        task->event = event;
        return push_event_task(&task->header, window, post_message_proc, post_message_destr, window->task_magic);
    }

    dispatch_event(&window->event_target, event);
    IDOMEvent_Release(&event->IDOMEvent_iface);
    return S_OK;
}

static HRESULT WINAPI HTMLWindow6_postMessage(IHTMLWindow6 *iface, BSTR msg, VARIANT targetOrigin)
{
    HTMLWindow *This = impl_from_IHTMLWindow6(iface);

    TRACE("(%p)->(%s %s)\n", This, debugstr_w(msg), debugstr_variant(&targetOrigin));

    if(V_VT(&targetOrigin) != VT_BSTR)
        return E_INVALIDARG;

    /* This can't obtain the source, and never works even in IE9+ modes... */
    return E_ABORT;
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

static inline HTMLWindow *impl_from_IHTMLWindow7(IHTMLWindow7 *iface)
{
    return CONTAINING_RECORD(iface, HTMLWindow, IHTMLWindow7_iface);
}

static HRESULT WINAPI HTMLWindow7_QueryInterface(IHTMLWindow7 *iface, REFIID riid, void **ppv)
{
    HTMLWindow *This = impl_from_IHTMLWindow7(iface);

    return IHTMLWindow2_QueryInterface(&This->IHTMLWindow2_iface, riid, ppv);
}

static ULONG WINAPI HTMLWindow7_AddRef(IHTMLWindow7 *iface)
{
    HTMLWindow *This = impl_from_IHTMLWindow7(iface);
    return IHTMLWindow2_AddRef(&This->IHTMLWindow2_iface);
}

static ULONG WINAPI HTMLWindow7_Release(IHTMLWindow7 *iface)
{
    HTMLWindow *This = impl_from_IHTMLWindow7(iface);
    return IHTMLWindow2_Release(&This->IHTMLWindow2_iface);
}

DISPEX_IDISPATCH_NOUNK_IMPL(HTMLWindow7, IHTMLWindow7,
                            impl_from_IHTMLWindow7(iface)->inner_window->event_target.dispex)

static HRESULT WINAPI HTMLWindow7_getSelection(IHTMLWindow7 *iface, IHTMLSelection **p)
{
    HTMLWindow *This = impl_from_IHTMLWindow7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow7_getComputedStyle(IHTMLWindow7 *iface, IHTMLDOMNode *node,
                                                   BSTR pseudo_elt, IHTMLCSSStyleDeclaration **p)
{
    HTMLWindow *This = impl_from_IHTMLWindow7(iface);
    nsIDOMCSSStyleDeclaration *nsstyle;
    nsAString pseudo_elt_str;
    HTMLElement *element;
    IHTMLElement *elem;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%p %s %p)\n", This, node, debugstr_w(pseudo_elt), p);

    hres = IHTMLDOMNode_QueryInterface(node, &IID_IHTMLElement, (void**)&elem);
    if(FAILED(hres))
        return hres;

    element = unsafe_impl_from_IHTMLElement(elem);
    if(!element) {
        WARN("Not our element\n");
        IHTMLElement_Release(elem);
        return E_INVALIDARG;
    }

    nsAString_Init(&pseudo_elt_str, NULL);
    nsres = nsIDOMWindow_GetComputedStyle(This->outer_window->nswindow, element->dom_element,
                                          &pseudo_elt_str, &nsstyle);
    IHTMLElement_Release(elem);
    nsAString_Finish(&pseudo_elt_str);
    if(NS_FAILED(nsres)) {
        FIXME("GetComputedStyle failed: %08lx\n", nsres);
        return E_FAIL;
    }

    if (!nsstyle)
    {
        FIXME("nsIDOMWindow_GetComputedStyle returned NULL nsstyle.\n");
        *p = NULL;
        return S_OK;
    }

    hres = create_computed_style(nsstyle, &This->inner_window->event_target.dispex, p);
    nsIDOMCSSStyleDeclaration_Release(nsstyle);
    return hres;
}

static HRESULT WINAPI HTMLWindow7_get_styleMedia(IHTMLWindow7 *iface, IHTMLStyleMedia **p)
{
    HTMLWindow *This = impl_from_IHTMLWindow7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow7_put_performance(IHTMLWindow7 *iface, VARIANT v)
{
    HTMLInnerWindow *This = impl_from_IHTMLWindow7(iface)->inner_window;

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    if(!This->performance_initialized) {
        V_VT(&This->performance) = VT_EMPTY;
        This->performance_initialized = TRUE;
    }

    return VariantCopy(&This->performance, &v);
}

static HRESULT WINAPI HTMLWindow7_get_performance(IHTMLWindow7 *iface, VARIANT *p)
{
    HTMLInnerWindow *This = impl_from_IHTMLWindow7(iface)->inner_window;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->performance_initialized) {
        IHTMLPerformance *performance;

        hres = create_performance(This, &performance);
        if(FAILED(hres))
            return hres;

        V_VT(&This->performance) = VT_DISPATCH;
        V_DISPATCH(&This->performance) = (IDispatch*)performance;
        This->performance_initialized = TRUE;
    }

    V_VT(p) = VT_NULL;
    return VariantCopy(p, &This->performance);
}

static HRESULT WINAPI HTMLWindow7_get_innerWidth(IHTMLWindow7 *iface, LONG *p)
{
    HTMLWindow *This = impl_from_IHTMLWindow7(iface);
    LONG ret;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMWindow_GetInnerWidth(This->outer_window->nswindow, &ret);
    if(NS_FAILED(nsres)) {
        ERR("GetInnerWidth failed: %08lx\n", nsres);
        return E_FAIL;
    }

    *p = ret;
    return S_OK;
}

static HRESULT WINAPI HTMLWindow7_get_innerHeight(IHTMLWindow7 *iface, LONG *p)
{
    HTMLWindow *This = impl_from_IHTMLWindow7(iface);
    LONG ret;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMWindow_GetInnerHeight(This->outer_window->nswindow, &ret);
    if(NS_FAILED(nsres)) {
        ERR("GetInnerWidth failed: %08lx\n", nsres);
        return E_FAIL;
    }

    *p = ret;
    return S_OK;
}

static HRESULT WINAPI HTMLWindow7_get_pageXOffset(IHTMLWindow7 *iface, LONG *p)
{
    HTMLWindow *This = impl_from_IHTMLWindow7(iface);
    nsresult nsres;
    LONG ret;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMWindow_GetPageXOffset(This->outer_window->nswindow, &ret);
    if(NS_FAILED(nsres)) {
        ERR("GetPageXOffset failed: %08lx\n", nsres);
        return E_FAIL;
    }

    *p = ret;
    return S_OK;
}

static HRESULT WINAPI HTMLWindow7_get_pageYOffset(IHTMLWindow7 *iface, LONG *p)
{
    HTMLWindow *This = impl_from_IHTMLWindow7(iface);
    nsresult nsres;
    LONG ret;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMWindow_GetPageYOffset(This->outer_window->nswindow, &ret);
    if(NS_FAILED(nsres)) {
        ERR("GetPageYOffset failed: %08lx\n", nsres);
        return E_FAIL;
    }

    *p = ret;
    return S_OK;
}

static HRESULT WINAPI HTMLWindow7_get_screenX(IHTMLWindow7 *iface, LONG *p)
{
    HTMLWindow *This = impl_from_IHTMLWindow7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow7_get_screenY(IHTMLWindow7 *iface, LONG *p)
{
    HTMLWindow *This = impl_from_IHTMLWindow7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow7_get_outerWidth(IHTMLWindow7 *iface, LONG *p)
{
    HTMLWindow *This = impl_from_IHTMLWindow7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow7_get_outerHeight(IHTMLWindow7 *iface, LONG *p)
{
    HTMLWindow *This = impl_from_IHTMLWindow7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

#define HTMLWINDOW7_ONEVENT_PROPERTY_IMPL(name, event_id) \
    static HRESULT WINAPI HTMLWindow7_put_on##name(IHTMLWindow7 *iface, VARIANT v) \
    { \
        HTMLWindow *This = impl_from_IHTMLWindow7(iface); \
        TRACE("(%p)->(%s)\n", This, debugstr_variant(&v)); \
        return set_window_event(This, event_id, &v); \
    } \
    static HRESULT WINAPI HTMLWindow7_get_on##name(IHTMLWindow7 *iface, VARIANT *p) \
    { \
        HTMLWindow *This = impl_from_IHTMLWindow7(iface); \
        TRACE("(%p)->(%p)\n", This, p); \
        return get_window_event(This, event_id, p); \
    }

#define HTMLWINDOW7_ONEVENT_PROPERTY_STUB(name)                         \
    static HRESULT WINAPI HTMLWindow7_put_on##name(IHTMLWindow7 *iface, VARIANT v) \
    { \
        HTMLWindow *This = impl_from_IHTMLWindow7(iface); \
        FIXME("(%p)->(%s)\n", This, debugstr_variant(&v)); \
        return E_NOTIMPL; \
    } \
    static HRESULT WINAPI HTMLWindow7_get_on##name(IHTMLWindow7 *iface, VARIANT *p) \
    { \
        HTMLWindow *This = impl_from_IHTMLWindow7(iface); \
        FIXME("(%p)->(%p)\n", This, p); \
        return E_NOTIMPL; \
    }

HTMLWINDOW7_ONEVENT_PROPERTY_IMPL(abort,            EVENTID_ABORT)
HTMLWINDOW7_ONEVENT_PROPERTY_STUB(canplay)
HTMLWINDOW7_ONEVENT_PROPERTY_STUB(canplaythrough)
HTMLWINDOW7_ONEVENT_PROPERTY_IMPL(change,           EVENTID_CHANGE)
HTMLWINDOW7_ONEVENT_PROPERTY_IMPL(click,            EVENTID_CLICK)
HTMLWINDOW7_ONEVENT_PROPERTY_IMPL(contextmenu,      EVENTID_CONTEXTMENU)
HTMLWINDOW7_ONEVENT_PROPERTY_IMPL(dblclick,         EVENTID_DBLCLICK)
HTMLWINDOW7_ONEVENT_PROPERTY_IMPL(drag,             EVENTID_DRAG)
HTMLWINDOW7_ONEVENT_PROPERTY_STUB(dragend)
HTMLWINDOW7_ONEVENT_PROPERTY_STUB(dragenter)
HTMLWINDOW7_ONEVENT_PROPERTY_STUB(dragleave)
HTMLWINDOW7_ONEVENT_PROPERTY_STUB(dragover)
HTMLWINDOW7_ONEVENT_PROPERTY_IMPL(dragstart,        EVENTID_DRAGSTART)
HTMLWINDOW7_ONEVENT_PROPERTY_STUB(drop)
HTMLWINDOW7_ONEVENT_PROPERTY_STUB(durationchange)
HTMLWINDOW7_ONEVENT_PROPERTY_IMPL(focusin,          EVENTID_FOCUSIN)
HTMLWINDOW7_ONEVENT_PROPERTY_IMPL(focusout,         EVENTID_FOCUSOUT)
HTMLWINDOW7_ONEVENT_PROPERTY_IMPL(input,            EVENTID_INPUT)
HTMLWINDOW7_ONEVENT_PROPERTY_STUB(emptied)
HTMLWINDOW7_ONEVENT_PROPERTY_STUB(ended)
HTMLWINDOW7_ONEVENT_PROPERTY_IMPL(keydown,          EVENTID_KEYDOWN)
HTMLWINDOW7_ONEVENT_PROPERTY_IMPL(keypress,         EVENTID_KEYPRESS)
HTMLWINDOW7_ONEVENT_PROPERTY_IMPL(keyup,            EVENTID_KEYUP)
HTMLWINDOW7_ONEVENT_PROPERTY_STUB(loadeddata)
HTMLWINDOW7_ONEVENT_PROPERTY_STUB(loadedmetadata)
HTMLWINDOW7_ONEVENT_PROPERTY_IMPL(loadstart,        EVENTID_LOADSTART)
HTMLWINDOW7_ONEVENT_PROPERTY_IMPL(mousedown,        EVENTID_MOUSEDOWN)
HTMLWINDOW7_ONEVENT_PROPERTY_STUB(mouseenter)
HTMLWINDOW7_ONEVENT_PROPERTY_STUB(mouseleave)
HTMLWINDOW7_ONEVENT_PROPERTY_IMPL(mousemove,        EVENTID_MOUSEMOVE)
HTMLWINDOW7_ONEVENT_PROPERTY_IMPL(mouseout,         EVENTID_MOUSEOUT)
HTMLWINDOW7_ONEVENT_PROPERTY_IMPL(mouseover,        EVENTID_MOUSEOVER)
HTMLWINDOW7_ONEVENT_PROPERTY_IMPL(mouseup,          EVENTID_MOUSEUP)
HTMLWINDOW7_ONEVENT_PROPERTY_IMPL(mousewheel,       EVENTID_MOUSEWHEEL)
HTMLWINDOW7_ONEVENT_PROPERTY_STUB(offline)
HTMLWINDOW7_ONEVENT_PROPERTY_STUB(online)
HTMLWINDOW7_ONEVENT_PROPERTY_IMPL(progress,         EVENTID_PROGRESS)
HTMLWINDOW7_ONEVENT_PROPERTY_STUB(ratechange)
HTMLWINDOW7_ONEVENT_PROPERTY_IMPL(readystatechange, EVENTID_READYSTATECHANGE)
HTMLWINDOW7_ONEVENT_PROPERTY_STUB(reset)
HTMLWINDOW7_ONEVENT_PROPERTY_STUB(seeked)
HTMLWINDOW7_ONEVENT_PROPERTY_STUB(seeking)
HTMLWINDOW7_ONEVENT_PROPERTY_STUB(select)
HTMLWINDOW7_ONEVENT_PROPERTY_STUB(stalled)
HTMLWINDOW7_ONEVENT_PROPERTY_IMPL(storage,          EVENTID_STORAGE)
HTMLWINDOW7_ONEVENT_PROPERTY_IMPL(submit,           EVENTID_SUBMIT)
HTMLWINDOW7_ONEVENT_PROPERTY_STUB(suspend)
HTMLWINDOW7_ONEVENT_PROPERTY_STUB(timeupdate)
HTMLWINDOW7_ONEVENT_PROPERTY_STUB(pause)
HTMLWINDOW7_ONEVENT_PROPERTY_STUB(play)
HTMLWINDOW7_ONEVENT_PROPERTY_STUB(playing)
HTMLWINDOW7_ONEVENT_PROPERTY_STUB(volumechange)
HTMLWINDOW7_ONEVENT_PROPERTY_STUB(waiting)

static const IHTMLWindow7Vtbl HTMLWindow7Vtbl = {
    HTMLWindow7_QueryInterface,
    HTMLWindow7_AddRef,
    HTMLWindow7_Release,
    HTMLWindow7_GetTypeInfoCount,
    HTMLWindow7_GetTypeInfo,
    HTMLWindow7_GetIDsOfNames,
    HTMLWindow7_Invoke,
    HTMLWindow7_getSelection,
    HTMLWindow7_getComputedStyle,
    HTMLWindow7_get_styleMedia,
    HTMLWindow7_put_performance,
    HTMLWindow7_get_performance,
    HTMLWindow7_get_innerWidth,
    HTMLWindow7_get_innerHeight,
    HTMLWindow7_get_pageXOffset,
    HTMLWindow7_get_pageYOffset,
    HTMLWindow7_get_screenX,
    HTMLWindow7_get_screenY,
    HTMLWindow7_get_outerWidth,
    HTMLWindow7_get_outerHeight,
    HTMLWindow7_put_onabort,
    HTMLWindow7_get_onabort,
    HTMLWindow7_put_oncanplay,
    HTMLWindow7_get_oncanplay,
    HTMLWindow7_put_oncanplaythrough,
    HTMLWindow7_get_oncanplaythrough,
    HTMLWindow7_put_onchange,
    HTMLWindow7_get_onchange,
    HTMLWindow7_put_onclick,
    HTMLWindow7_get_onclick,
    HTMLWindow7_put_oncontextmenu,
    HTMLWindow7_get_oncontextmenu,
    HTMLWindow7_put_ondblclick,
    HTMLWindow7_get_ondblclick,
    HTMLWindow7_put_ondrag,
    HTMLWindow7_get_ondrag,
    HTMLWindow7_put_ondragend,
    HTMLWindow7_get_ondragend,
    HTMLWindow7_put_ondragenter,
    HTMLWindow7_get_ondragenter,
    HTMLWindow7_put_ondragleave,
    HTMLWindow7_get_ondragleave,
    HTMLWindow7_put_ondragover,
    HTMLWindow7_get_ondragover,
    HTMLWindow7_put_ondragstart,
    HTMLWindow7_get_ondragstart,
    HTMLWindow7_put_ondrop,
    HTMLWindow7_get_ondrop,
    HTMLWindow7_put_ondurationchange,
    HTMLWindow7_get_ondurationchange,
    HTMLWindow7_put_onfocusin,
    HTMLWindow7_get_onfocusin,
    HTMLWindow7_put_onfocusout,
    HTMLWindow7_get_onfocusout,
    HTMLWindow7_put_oninput,
    HTMLWindow7_get_oninput,
    HTMLWindow7_put_onemptied,
    HTMLWindow7_get_onemptied,
    HTMLWindow7_put_onended,
    HTMLWindow7_get_onended,
    HTMLWindow7_put_onkeydown,
    HTMLWindow7_get_onkeydown,
    HTMLWindow7_put_onkeypress,
    HTMLWindow7_get_onkeypress,
    HTMLWindow7_put_onkeyup,
    HTMLWindow7_get_onkeyup,
    HTMLWindow7_put_onloadeddata,
    HTMLWindow7_get_onloadeddata,
    HTMLWindow7_put_onloadedmetadata,
    HTMLWindow7_get_onloadedmetadata,
    HTMLWindow7_put_onloadstart,
    HTMLWindow7_get_onloadstart,
    HTMLWindow7_put_onmousedown,
    HTMLWindow7_get_onmousedown,
    HTMLWindow7_put_onmouseenter,
    HTMLWindow7_get_onmouseenter,
    HTMLWindow7_put_onmouseleave,
    HTMLWindow7_get_onmouseleave,
    HTMLWindow7_put_onmousemove,
    HTMLWindow7_get_onmousemove,
    HTMLWindow7_put_onmouseout,
    HTMLWindow7_get_onmouseout,
    HTMLWindow7_put_onmouseover,
    HTMLWindow7_get_onmouseover,
    HTMLWindow7_put_onmouseup,
    HTMLWindow7_get_onmouseup,
    HTMLWindow7_put_onmousewheel,
    HTMLWindow7_get_onmousewheel,
    HTMLWindow7_put_onoffline,
    HTMLWindow7_get_onoffline,
    HTMLWindow7_put_ononline,
    HTMLWindow7_get_ononline,
    HTMLWindow7_put_onprogress,
    HTMLWindow7_get_onprogress,
    HTMLWindow7_put_onratechange,
    HTMLWindow7_get_onratechange,
    HTMLWindow7_put_onreadystatechange,
    HTMLWindow7_get_onreadystatechange,
    HTMLWindow7_put_onreset,
    HTMLWindow7_get_onreset,
    HTMLWindow7_put_onseeked,
    HTMLWindow7_get_onseeked,
    HTMLWindow7_put_onseeking,
    HTMLWindow7_get_onseeking,
    HTMLWindow7_put_onselect,
    HTMLWindow7_get_onselect,
    HTMLWindow7_put_onstalled,
    HTMLWindow7_get_onstalled,
    HTMLWindow7_put_onstorage,
    HTMLWindow7_get_onstorage,
    HTMLWindow7_put_onsubmit,
    HTMLWindow7_get_onsubmit,
    HTMLWindow7_put_onsuspend,
    HTMLWindow7_get_onsuspend,
    HTMLWindow7_put_ontimeupdate,
    HTMLWindow7_get_ontimeupdate,
    HTMLWindow7_put_onpause,
    HTMLWindow7_get_onpause,
    HTMLWindow7_put_onplay,
    HTMLWindow7_get_onplay,
    HTMLWindow7_put_onplaying,
    HTMLWindow7_get_onplaying,
    HTMLWindow7_put_onvolumechange,
    HTMLWindow7_get_onvolumechange,
    HTMLWindow7_put_onwaiting,
    HTMLWindow7_get_onwaiting
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
    DWORD binding_flags = BINDING_NAVIGATED|BINDING_NOFRAG;
    HTMLOuterWindow *window = This->outer_window;
    OLECHAR *translated_url = NULL;
    DWORD post_data_size = 0;
    BYTE *post_data = NULL;
    WCHAR *headers = NULL;
    IUri *uri;
    HRESULT hres;

    TRACE("(%p)->(%s %s %s %s %s %s %lx)\n", This, debugstr_w(url), debugstr_w(arg2), debugstr_w(arg3), debugstr_w(arg4),
          debugstr_variant(post_data_var), debugstr_variant(headers_var), flags);

    if(flags & ~2)
        FIXME("unimplemented flags %lx\n", flags & ~2);

    if(!window->browser)
        return E_FAIL;

    if(window->browser->doc->hostui) {
        hres = IDocHostUIHandler_TranslateUrl(window->browser->doc->hostui, 0, url, &translated_url);
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

    if(flags & 2)
        binding_flags |= BINDING_REPLACE;

    hres = super_navigate(window, uri, binding_flags, headers, post_data, post_data_size);
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

    *url = SysAllocString(This->outer_window->url ? This->outer_window->url : L"about:blank");
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

    FIXME("(%p)->(%ld %p) semi-stub\n", This, dwID, ppunk);

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
    FIXME("(%p)->(%s %ld)\n", This, debugstr_w(pszUrlLocation), dwPosition);
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

static inline HTMLWindow *impl_from_IProvideMultipleClassInfo(IProvideMultipleClassInfo *iface)
{
    return CONTAINING_RECORD(iface, HTMLWindow, IProvideMultipleClassInfo_iface);
}

static HRESULT WINAPI ProvideClassInfo_QueryInterface(IProvideMultipleClassInfo *iface,
        REFIID riid, void **ppv)
{
    HTMLWindow *This = impl_from_IProvideMultipleClassInfo(iface);
    return IHTMLWindow2_QueryInterface(&This->IHTMLWindow2_iface, riid, ppv);
}

static ULONG WINAPI ProvideClassInfo_AddRef(IProvideMultipleClassInfo *iface)
{
    HTMLWindow *This = impl_from_IProvideMultipleClassInfo(iface);
    return IHTMLWindow2_AddRef(&This->IHTMLWindow2_iface);
}

static ULONG WINAPI ProvideClassInfo_Release(IProvideMultipleClassInfo *iface)
{
    HTMLWindow *This = impl_from_IProvideMultipleClassInfo(iface);
    return IHTMLWindow2_Release(&This->IHTMLWindow2_iface);
}

static HRESULT WINAPI ProvideClassInfo_GetClassInfo(IProvideMultipleClassInfo *iface, ITypeInfo **ppTI)
{
    HTMLWindow *This = impl_from_IProvideMultipleClassInfo(iface);
    TRACE("(%p)->(%p)\n", This, ppTI);
    return get_class_typeinfo(&CLSID_HTMLWindow2, ppTI);
}

static HRESULT WINAPI ProvideClassInfo2_GetGUID(IProvideMultipleClassInfo *iface, DWORD dwGuidKind, GUID *pGUID)
{
    HTMLWindow *This = impl_from_IProvideMultipleClassInfo(iface);
    FIXME("(%p)->(%lu %p)\n", This, dwGuidKind, pGUID);
    return E_NOTIMPL;
}

static HRESULT WINAPI ProvideMultipleClassInfo_GetMultiTypeInfoCount(IProvideMultipleClassInfo *iface, ULONG *pcti)
{
    HTMLWindow *This = impl_from_IProvideMultipleClassInfo(iface);
    FIXME("(%p)->(%p)\n", This, pcti);
    *pcti = 1;
    return S_OK;
}

static HRESULT WINAPI ProvideMultipleClassInfo_GetInfoOfIndex(IProvideMultipleClassInfo *iface, ULONG iti,
        DWORD dwFlags, ITypeInfo **pptiCoClass, DWORD *pdwTIFlags, ULONG *pcdispidReserved, IID *piidPrimary, IID *piidSource)
{
    HTMLWindow *This = impl_from_IProvideMultipleClassInfo(iface);
    FIXME("(%p)->(%lu %lx %p %p %p %p %p)\n", This, iti, dwFlags, pptiCoClass, pdwTIFlags, pcdispidReserved, piidPrimary, piidSource);
    return E_NOTIMPL;
}

static const IProvideMultipleClassInfoVtbl ProvideMultipleClassInfoVtbl = {
    ProvideClassInfo_QueryInterface,
    ProvideClassInfo_AddRef,
    ProvideClassInfo_Release,
    ProvideClassInfo_GetClassInfo,
    ProvideClassInfo2_GetGUID,
    ProvideMultipleClassInfo_GetMultiTypeInfoCount,
    ProvideMultipleClassInfo_GetInfoOfIndex
};

static inline HTMLWindow *impl_from_IWineHTMLWindowPrivateVtbl(IWineHTMLWindowPrivate *iface)
{
    return CONTAINING_RECORD(iface, HTMLWindow, IWineHTMLWindowPrivate_iface);
}

static HRESULT WINAPI window_private_QueryInterface(IWineHTMLWindowPrivate *iface,
        REFIID riid, void **ppv)
{
    HTMLWindow *This = impl_from_IWineHTMLWindowPrivateVtbl(iface);

    return IHTMLWindow2_QueryInterface(&This->IHTMLWindow2_iface, riid, ppv);
}

static ULONG WINAPI window_private_AddRef(IWineHTMLWindowPrivate *iface)
{
    HTMLWindow *This = impl_from_IWineHTMLWindowPrivateVtbl(iface);

    return IHTMLWindow2_AddRef(&This->IHTMLWindow2_iface);
}

static ULONG WINAPI window_private_Release(IWineHTMLWindowPrivate *iface)
{
    HTMLWindow *This = impl_from_IWineHTMLWindowPrivateVtbl(iface);

    return IHTMLWindow2_Release(&This->IHTMLWindow2_iface);
}

DISPEX_IDISPATCH_NOUNK_IMPL(window_private, IWineHTMLWindowPrivate,
                            impl_from_IWineHTMLWindowPrivateVtbl(iface)->inner_window->event_target.dispex)

static HRESULT WINAPI window_private_requestAnimationFrame(IWineHTMLWindowPrivate *iface,
        VARIANT *expr, VARIANT *timer_id)
{
    HTMLWindow *This = impl_from_IWineHTMLWindowPrivateVtbl(iface);
    HRESULT hres;
    LONG r;

    FIXME("iface %p, expr %p, timer_id %p semi-stub.\n", iface, expr, timer_id);

    hres = window_set_timer(This->inner_window, expr, 50, NULL, TIMER_ANIMATION_FRAME, &r);
    if(SUCCEEDED(hres) && timer_id) {
        V_VT(timer_id) = VT_I4;
        V_I4(timer_id) = r;
    }

    return hres;
}

static HRESULT WINAPI window_private_cancelAnimationFrame(IWineHTMLWindowPrivate *iface, VARIANT timer_id)
{
    HTMLWindow *This = impl_from_IWineHTMLWindowPrivateVtbl(iface);
    HRESULT hres;

    TRACE("iface %p, timer_id %s\n", iface, debugstr_variant(&timer_id));

    hres = VariantChangeType(&timer_id, &timer_id, 0, VT_I4);
    if(SUCCEEDED(hres))
        clear_animation_timer(This->inner_window, V_I4(&timer_id));

    return S_OK;
}

static HRESULT WINAPI window_private_matchMedia(IWineHTMLWindowPrivate *iface, BSTR media_query, IDispatch **media_query_list)
{
    HTMLWindow *This = impl_from_IWineHTMLWindowPrivateVtbl(iface);

    TRACE("iface %p, media_query %s\n", iface, debugstr_w(media_query));

    return create_media_query_list(This->inner_window, media_query, media_query_list);
}

static HRESULT WINAPI window_private_get_console(IWineHTMLWindowPrivate *iface, IDispatch **console)
{
    HTMLWindow *This = impl_from_IWineHTMLWindowPrivateVtbl(iface);
    HTMLInnerWindow *window = This->inner_window;

    TRACE("iface %p, console %p.\n", iface, console);

    if (!window->console)
        create_console(window, &window->console);

    *console = (IDispatch *)window->console;
    if (window->console)
        IWineMSHTMLConsole_AddRef(window->console);
    return S_OK;
}

static const IWineHTMLWindowPrivateVtbl WineHTMLWindowPrivateVtbl = {
    window_private_QueryInterface,
    window_private_AddRef,
    window_private_Release,
    window_private_GetTypeInfoCount,
    window_private_GetTypeInfo,
    window_private_GetIDsOfNames,
    window_private_Invoke,
    window_private_requestAnimationFrame,
    window_private_cancelAnimationFrame,
    window_private_get_console,
    window_private_matchMedia,
};

static inline HTMLWindow *impl_from_IWineHTMLWindowCompatPrivateVtbl(IWineHTMLWindowCompatPrivate *iface)
{
    return CONTAINING_RECORD(iface, HTMLWindow, IWineHTMLWindowCompatPrivate_iface);
}

static HRESULT WINAPI window_compat_private_QueryInterface(IWineHTMLWindowCompatPrivate *iface,
        REFIID riid, void **ppv)
{
    HTMLWindow *This = impl_from_IWineHTMLWindowCompatPrivateVtbl(iface);

    return IHTMLWindow2_QueryInterface(&This->IHTMLWindow2_iface, riid, ppv);
}

static ULONG WINAPI window_compat_private_AddRef(IWineHTMLWindowCompatPrivate *iface)
{
    HTMLWindow *This = impl_from_IWineHTMLWindowCompatPrivateVtbl(iface);

    return IHTMLWindow2_AddRef(&This->IHTMLWindow2_iface);
}

static ULONG WINAPI window_compat_private_Release(IWineHTMLWindowCompatPrivate *iface)
{
    HTMLWindow *This = impl_from_IWineHTMLWindowCompatPrivateVtbl(iface);

    return IHTMLWindow2_Release(&This->IHTMLWindow2_iface);
}

DISPEX_IDISPATCH_NOUNK_IMPL(window_compat_private, IWineHTMLWindowCompatPrivate,
                            impl_from_IWineHTMLWindowCompatPrivateVtbl(iface)->inner_window->event_target.dispex)

static HRESULT WINAPI window_compat_private_put_performance(IWineHTMLWindowCompatPrivate *iface, VARIANT v)
{
    HTMLWindow *This = impl_from_IWineHTMLWindowCompatPrivateVtbl(iface);

    return IHTMLWindow7_put_performance(&This->IHTMLWindow7_iface, v);
}

static HRESULT WINAPI window_compat_private_get_performance(IWineHTMLWindowCompatPrivate *iface, VARIANT *p)
{
    HTMLWindow *This = impl_from_IWineHTMLWindowCompatPrivateVtbl(iface);

    return IHTMLWindow7_get_performance(&This->IHTMLWindow7_iface, p);
}

static const IWineHTMLWindowCompatPrivateVtbl WineHTMLWindowCompatPrivateVtbl = {
    window_compat_private_QueryInterface,
    window_compat_private_AddRef,
    window_compat_private_Release,
    window_compat_private_GetTypeInfoCount,
    window_compat_private_GetTypeInfo,
    window_compat_private_GetIDsOfNames,
    window_compat_private_Invoke,
    window_compat_private_put_performance,
    window_compat_private_get_performance,
};

static inline HTMLOuterWindow *impl_from_IWineJSDispatchHost(IWineJSDispatchHost *iface)
{
    return CONTAINING_RECORD(iface, HTMLOuterWindow, IWineJSDispatchHost_iface);
}

static HRESULT WINAPI WindowDispEx_QueryInterface(IWineJSDispatchHost *iface, REFIID riid, void **ppv)
{
    HTMLOuterWindow *This = impl_from_IWineJSDispatchHost(iface);

    return IHTMLWindow2_QueryInterface(&This->base.IHTMLWindow2_iface, riid, ppv);
}

static ULONG WINAPI WindowDispEx_AddRef(IWineJSDispatchHost *iface)
{
    HTMLOuterWindow *This = impl_from_IWineJSDispatchHost(iface);

    return IHTMLWindow2_AddRef(&This->base.IHTMLWindow2_iface);
}

static ULONG WINAPI WindowDispEx_Release(IWineJSDispatchHost *iface)
{
    HTMLOuterWindow *This = impl_from_IWineJSDispatchHost(iface);

    return IHTMLWindow2_Release(&This->base.IHTMLWindow2_iface);
}

DISPEX_IDISPATCH_NOUNK_IMPL(WindowDispEx, IWineJSDispatchHost,
                            impl_from_IWineJSDispatchHost(iface)->base.inner_window->event_target.dispex)

static global_prop_t *alloc_global_prop(HTMLInnerWindow *This, global_prop_type_t type, const WCHAR *name)
{
    if(This->global_prop_cnt == This->global_prop_size) {
        global_prop_t *new_props;
        DWORD new_size;

        if(This->global_props) {
            new_size = This->global_prop_size*2;
            new_props = realloc(This->global_props, new_size * sizeof(global_prop_t));
        }else {
            new_size = 16;
            new_props = malloc(new_size * sizeof(global_prop_t));
        }
        if(!new_props)
            return NULL;
        This->global_props = new_props;
        This->global_prop_size = new_size;
    }

    This->global_props[This->global_prop_cnt].name = SysAllocString(name);
    if(!This->global_props[This->global_prop_cnt].name)
        return NULL;

    This->global_props[This->global_prop_cnt].type = type;
    return This->global_props + This->global_prop_cnt++;
}

static inline DWORD prop_to_dispid(HTMLInnerWindow *This, global_prop_t *prop)
{
    return MSHTML_DISPID_CUSTOM_MIN + (prop-This->global_props);
}

HRESULT search_window_props(HTMLInnerWindow *This, const WCHAR *name, DWORD grfdex, DISPID *pid)
{
    DWORD i;
    ScriptHost *script_host;
    DISPID id;

    for(i=0; i < This->global_prop_cnt; i++) {
        /* FIXME: case sensitivity */
        if(!wcscmp(This->global_props[i].name, name)) {
            HRESULT hres = global_prop_still_exists(This, &This->global_props[i]);
            if(hres == S_OK)
                *pid = MSHTML_DISPID_CUSTOM_MIN + i;
            return (hres == DISP_E_MEMBERNOTFOUND) ? DISP_E_UNKNOWNNAME : hres;
        }
    }

    if(find_global_prop(This->base.inner_window, name, grfdex, &script_host, &id)) {
        global_prop_t *prop;

        prop = alloc_global_prop(This, GLOBAL_SCRIPTVAR, name);
        if(!prop)
            return E_OUTOFMEMORY;

        prop->script_host = script_host;
        prop->id = id;

        *pid = prop_to_dispid(This, prop);
        return S_OK;
    }

    return DISP_E_UNKNOWNNAME;
}

static HRESULT WINAPI WindowDispEx_GetDispID(IWineJSDispatchHost *iface, BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    HTMLOuterWindow *This = impl_from_IWineJSDispatchHost(iface);

    return IWineJSDispatchHost_GetDispID(&This->base.inner_window->event_target.dispex.IWineJSDispatchHost_iface, bstrName, grfdex, pid);
}

static HRESULT WINAPI WindowDispEx_InvokeEx(IWineJSDispatchHost *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    HTMLOuterWindow *This = impl_from_IWineJSDispatchHost(iface);
    return IWineJSDispatchHost_InvokeEx(&This->base.inner_window->event_target.dispex.IWineJSDispatchHost_iface, id, lcid, wFlags,
                                    pdp, pvarRes, pei, pspCaller);
}

static HRESULT WINAPI WindowDispEx_DeleteMemberByName(IWineJSDispatchHost *iface, BSTR bstrName, DWORD grfdex)
{
    HTMLOuterWindow *This = impl_from_IWineJSDispatchHost(iface);
    compat_mode_t compat_mode = dispex_compat_mode(&This->base.inner_window->event_target.dispex);

    TRACE("(%p)->(%s %lx)\n", This, debugstr_w(bstrName), grfdex);

    if(compat_mode < COMPAT_MODE_IE8) {
        /* Not implemented by IE */
        return E_NOTIMPL;
    }
    if(compat_mode == COMPAT_MODE_IE8)
        return MSHTML_E_INVALID_ACTION;

    return IWineJSDispatchHost_DeleteMemberByName(&This->base.inner_window->event_target.dispex.IWineJSDispatchHost_iface, bstrName, grfdex);
}

static HRESULT WINAPI WindowDispEx_DeleteMemberByDispID(IWineJSDispatchHost *iface, DISPID id)
{
    HTMLOuterWindow *This = impl_from_IWineJSDispatchHost(iface);
    compat_mode_t compat_mode = dispex_compat_mode(&This->base.inner_window->event_target.dispex);

    TRACE("(%p)->(%lx)\n", This, id);

    if(compat_mode < COMPAT_MODE_IE8) {
        /* Not implemented by IE */
        return E_NOTIMPL;
    }
    if(compat_mode == COMPAT_MODE_IE8)
        return MSHTML_E_INVALID_ACTION;

    return IWineJSDispatchHost_DeleteMemberByDispID(&This->base.inner_window->event_target.dispex.IWineJSDispatchHost_iface, id);
}

static HRESULT WINAPI WindowDispEx_GetMemberProperties(IWineJSDispatchHost *iface, DISPID id, DWORD grfdexFetch, DWORD *pgrfdex)
{
    HTMLOuterWindow *This = impl_from_IWineJSDispatchHost(iface);

    TRACE("(%p)->(%lx %lx %p)\n", This, id, grfdexFetch, pgrfdex);

    return IWineJSDispatchHost_GetMemberProperties(&This->base.inner_window->event_target.dispex.IWineJSDispatchHost_iface, id, grfdexFetch,
            pgrfdex);
}

static HRESULT WINAPI WindowDispEx_GetMemberName(IWineJSDispatchHost *iface, DISPID id, BSTR *pbstrName)
{
    HTMLOuterWindow *This = impl_from_IWineJSDispatchHost(iface);

    TRACE("(%p)->(%lx %p)\n", This, id, pbstrName);

    return IWineJSDispatchHost_GetMemberName(&This->base.inner_window->event_target.dispex.IWineJSDispatchHost_iface, id, pbstrName);
}

static HRESULT WINAPI WindowDispEx_GetNextDispID(IWineJSDispatchHost *iface, DWORD grfdex, DISPID id, DISPID *pid)
{
    HTMLOuterWindow *This = impl_from_IWineJSDispatchHost(iface);

    TRACE("(%p)->(%lx %lx %p)\n", This, grfdex, id, pid);

    return IWineJSDispatchHost_GetNextDispID(&This->base.inner_window->event_target.dispex.IWineJSDispatchHost_iface, grfdex, id, pid);
}

static HRESULT WINAPI WindowDispEx_GetNameSpaceParent(IWineJSDispatchHost *iface, IUnknown **ppunk)
{
    HTMLOuterWindow *This = impl_from_IWineJSDispatchHost(iface);

    TRACE("(%p)->(%p)\n", This, ppunk);

    *ppunk = NULL;
    return S_OK;
}

static HRESULT WINAPI WindowDispEx_GetJSDispatch(IWineJSDispatchHost *iface, IWineJSDispatch **ret)
{
    HTMLOuterWindow *This = impl_from_IWineJSDispatchHost(iface);

    return IWineJSDispatchHost_GetJSDispatch(&This->base.inner_window->event_target.dispex.IWineJSDispatchHost_iface, ret);
}

static HRESULT WINAPI WindowDispEx_LookupProperty(IWineJSDispatchHost *iface, const WCHAR *name, DWORD flags,
                                                  struct property_info *desc)
{
    HTMLOuterWindow *This = impl_from_IWineJSDispatchHost(iface);

    return IWineJSDispatchHost_LookupProperty(&This->base.inner_window->event_target.dispex.IWineJSDispatchHost_iface,
                                              name, flags, desc);
}

static HRESULT WINAPI WindowDispEx_NextProperty(IWineJSDispatchHost *iface, DISPID id, struct property_info *desc)
{
    HTMLOuterWindow *This = impl_from_IWineJSDispatchHost(iface);

    return IWineJSDispatchHost_NextProperty(&This->base.inner_window->event_target.dispex.IWineJSDispatchHost_iface,
                                            id, desc);
}

static HRESULT WINAPI WindowDispEx_GetProperty(IWineJSDispatchHost *iface, DISPID id, LCID lcid, VARIANT *r,
                                               EXCEPINFO *ei, IServiceProvider *caller)
{
    HTMLOuterWindow *This = impl_from_IWineJSDispatchHost(iface);

    return IWineJSDispatchHost_GetProperty(&This->base.inner_window->event_target.dispex.IWineJSDispatchHost_iface,
                                           id, lcid, r, ei, caller);
}

static HRESULT WINAPI WindowDispEx_SetProperty(IWineJSDispatchHost *iface, DISPID id, LCID lcid, VARIANT *v,
                                               EXCEPINFO *ei, IServiceProvider *caller)
{
    HTMLOuterWindow *This = impl_from_IWineJSDispatchHost(iface);

    return IWineJSDispatchHost_SetProperty(&This->base.inner_window->event_target.dispex.IWineJSDispatchHost_iface,
                                           id, lcid, v, ei, caller);
}

static HRESULT WINAPI WindowDispEx_DeleteProperty(IWineJSDispatchHost *iface, DISPID id)
{
    HTMLOuterWindow *This = impl_from_IWineJSDispatchHost(iface);

    return IWineJSDispatchHost_DeleteProperty(&This->base.inner_window->event_target.dispex.IWineJSDispatchHost_iface, id);
}

static HRESULT WINAPI WindowDispEx_ConfigureProperty(IWineJSDispatchHost *iface, DISPID id, UINT32 flags)
{
    HTMLOuterWindow *This = impl_from_IWineJSDispatchHost(iface);

    return IWineJSDispatchHost_ConfigureProperty(&This->base.inner_window->event_target.dispex.IWineJSDispatchHost_iface, id, flags);
}

static HRESULT WINAPI WindowDispEx_CallFunction(IWineJSDispatchHost *iface, DISPID id, UINT32 iid, DWORD flags, DISPPARAMS *dp,
                                                VARIANT *ret, EXCEPINFO *ei, IServiceProvider *caller)
{
    HTMLOuterWindow *This = impl_from_IWineJSDispatchHost(iface);

    return IWineJSDispatchHost_CallFunction(&This->base.inner_window->event_target.dispex.IWineJSDispatchHost_iface,
                                            id, iid, flags, dp, ret, ei, caller);
}

static HRESULT WINAPI WindowDispEx_Construct(IWineJSDispatchHost *iface, LCID lcid, DWORD flags, DISPPARAMS *dp, VARIANT *ret,
                                             EXCEPINFO *ei, IServiceProvider *caller)
{
    HTMLOuterWindow *This = impl_from_IWineJSDispatchHost(iface);

    return IWineJSDispatchHost_Construct(&This->base.inner_window->event_target.dispex.IWineJSDispatchHost_iface,
                                         lcid, flags, dp, ret, ei, caller);
}

static HRESULT WINAPI WindowDispEx_GetOuterDispatch(IWineJSDispatchHost *iface, IWineJSDispatchHost **ret)
{
    HTMLOuterWindow *This = impl_from_IWineJSDispatchHost(iface);

    *ret = &This->IWineJSDispatchHost_iface;
    IWineJSDispatchHost_AddRef(*ret);
    return S_OK;
}

static HRESULT WINAPI WindowDispEx_ToString(IWineJSDispatchHost *iface, BSTR *str)
{
    HTMLOuterWindow *This = impl_from_IWineJSDispatchHost(iface);

    return IWineJSDispatchHost_ToString(&This->base.inner_window->event_target.dispex.IWineJSDispatchHost_iface, str);
}

static const IWineJSDispatchHostVtbl WindowDispExVtbl = {
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
    WindowDispEx_GetNameSpaceParent,
    WindowDispEx_GetJSDispatch,
    WindowDispEx_LookupProperty,
    WindowDispEx_NextProperty,
    WindowDispEx_GetProperty,
    WindowDispEx_SetProperty,
    WindowDispEx_DeleteProperty,
    WindowDispEx_ConfigureProperty,
    WindowDispEx_CallFunction,
    WindowDispEx_Construct,
    WindowDispEx_GetOuterDispatch,
    WindowDispEx_ToString,
};

static inline HTMLOuterWindow *impl_from_IEventTarget(IEventTarget *iface)
{
    return CONTAINING_RECORD(iface, HTMLOuterWindow, IEventTarget_iface);
}

static HRESULT WINAPI WindowEventTarget_QueryInterface(IEventTarget *iface, REFIID riid, void **ppv)
{
    HTMLOuterWindow *This = impl_from_IEventTarget(iface);

    return IHTMLWindow2_QueryInterface(&This->base.IHTMLWindow2_iface, riid, ppv);
}

static ULONG WINAPI WindowEventTarget_AddRef(IEventTarget *iface)
{
    HTMLOuterWindow *This = impl_from_IEventTarget(iface);

    return IHTMLWindow2_AddRef(&This->base.IHTMLWindow2_iface);
}

static ULONG WINAPI WindowEventTarget_Release(IEventTarget *iface)
{
    HTMLOuterWindow *This = impl_from_IEventTarget(iface);

    return IHTMLWindow2_Release(&This->base.IHTMLWindow2_iface);
}

DISPEX_IDISPATCH_NOUNK_IMPL(WindowEventTarget, IEventTarget,
                            impl_from_IEventTarget(iface)->base.inner_window->event_target.dispex)

static HRESULT WINAPI WindowEventTarget_addEventListener(IEventTarget *iface, BSTR type, IDispatch *listener,
        VARIANT_BOOL capture)
{
    HTMLOuterWindow *This = impl_from_IEventTarget(iface);

    return IEventTarget_addEventListener(&This->base.inner_window->event_target.IEventTarget_iface, type, listener, capture);
}

static HRESULT WINAPI WindowEventTarget_removeEventListener(IEventTarget *iface, BSTR type, IDispatch *listener,
        VARIANT_BOOL capture)
{
    HTMLOuterWindow *This = impl_from_IEventTarget(iface);

    return IEventTarget_removeEventListener(&This->base.inner_window->event_target.IEventTarget_iface, type, listener, capture);
}

static HRESULT WINAPI WindowEventTarget_dispatchEvent(IEventTarget *iface, IDOMEvent *event_iface, VARIANT_BOOL *result)
{
    HTMLOuterWindow *This = impl_from_IEventTarget(iface);

    return IEventTarget_dispatchEvent(&This->base.inner_window->event_target.IEventTarget_iface, event_iface, result);
}

static const IEventTargetVtbl EventTargetVtbl = {
    WindowEventTarget_QueryInterface,
    WindowEventTarget_AddRef,
    WindowEventTarget_Release,
    WindowEventTarget_GetTypeInfoCount,
    WindowEventTarget_GetTypeInfo,
    WindowEventTarget_GetIDsOfNames,
    WindowEventTarget_Invoke,
    WindowEventTarget_addEventListener,
    WindowEventTarget_removeEventListener,
    WindowEventTarget_dispatchEvent
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

    if(!This->outer_window || !This->outer_window->browser)
        return E_NOINTERFACE;

    return IServiceProvider_QueryService(&This->outer_window->browser->doc->IServiceProvider_iface,
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

static void *HTMLWindow_query_interface(DispatchEx *dispex, REFIID riid)
{
    HTMLInnerWindow *This = impl_from_DispatchEx(dispex);
    void *iface = base_query_interface(&This->base, riid);
    return iface ? iface : EventTarget_query_interface(&This->event_target, riid);
}

static void HTMLWindow_traverse(DispatchEx *dispex, nsCycleCollectionTraversalCallback *cb)
{
    HTMLInnerWindow *This = impl_from_DispatchEx(dispex);
    HTMLOuterWindow *child;
    unsigned int i;

    traverse_event_target(&This->event_target, cb);
    LIST_FOR_EACH_ENTRY(child, &This->children, HTMLOuterWindow, sibling_entry)
        note_cc_edge((nsISupports*)&child->base.IHTMLWindow2_iface, "child", cb);
    for(i = 0; i < ARRAYSIZE(This->prototypes); i++) {
        if(This->prototypes[i])
            note_cc_edge((nsISupports*)&This->prototypes[i]->IWineJSDispatchHost_iface, "prototypes", cb);
    }
    for(i = 0; i < ARRAYSIZE(This->constructors); i++) {
        if(This->constructors[i])
            note_cc_edge((nsISupports*)&This->constructors[i]->IWineJSDispatchHost_iface, "constructors", cb);
    }
    if(This->doc)
        note_cc_edge((nsISupports*)&This->doc->node.IHTMLDOMNode_iface, "doc", cb);
    if(This->console)
        note_cc_edge((nsISupports*)This->console, "console", cb);
    if(This->image_factory)
        note_cc_edge((nsISupports*)&This->image_factory->IHTMLImageElementFactory_iface, "image_factory", cb);
    if(This->option_factory)
        note_cc_edge((nsISupports*)&This->option_factory->IHTMLOptionElementFactory_iface, "option_factory", cb);
    if(This->screen)
        note_cc_edge((nsISupports*)This->screen, "screen", cb);
    if(This->history)
        note_cc_edge((nsISupports*)&This->history->IOmHistory_iface, "history", cb);
    if(This->navigator)
        note_cc_edge((nsISupports*)This->navigator, "navigator", cb);
    if(This->session_storage)
        note_cc_edge((nsISupports*)This->session_storage, "session_storage", cb);
    if(This->local_storage)
        note_cc_edge((nsISupports*)This->local_storage, "local_storage", cb);
    if(This->dom_window)
        note_cc_edge((nsISupports*)This->dom_window, "dom_window", cb);
    traverse_variant(&This->performance, "performance", cb);
}

static void HTMLWindow_unlink(DispatchEx *dispex)
{
    HTMLInnerWindow *This = impl_from_DispatchEx(dispex);
    unsigned int i;

    TRACE("%p\n", This);

    unlink_ref(&This->console);
    detach_inner_window(This);

    for(i = 0; i < ARRAYSIZE(This->prototypes); i++)
        unlink_ref(&This->prototypes[i]);
    for(i = 0; i < ARRAYSIZE(This->constructors); i++)
        unlink_ref(&This->constructors[i]);

    if(This->doc) {
        HTMLDocumentNode *doc = This->doc;
        This->doc = NULL;
        IHTMLDOMNode_Release(&doc->node.IHTMLDOMNode_iface);
    }

    release_event_target(&This->event_target);

    if(This->image_factory) {
        HTMLImageElementFactory *image_factory = This->image_factory;
        This->image_factory = NULL;
        IHTMLImageElementFactory_Release(&image_factory->IHTMLImageElementFactory_iface);
    }
    if(This->option_factory) {
        HTMLOptionElementFactory *option_factory = This->option_factory;
        This->option_factory = NULL;
        IHTMLOptionElementFactory_Release(&option_factory->IHTMLOptionElementFactory_iface);
    }
    unlink_ref(&This->screen);
    if(This->history) {
        OmHistory *history = This->history;
        This->history = NULL;
        IOmHistory_Release(&history->IOmHistory_iface);
    }
    unlink_ref(&This->navigator);
    if(This->session_storage) {
        IHTMLStorage *session_storage = This->session_storage;
        This->session_storage = NULL;
        IHTMLStorage_Release(session_storage);
    }
    if(This->local_storage) {
        IHTMLStorage *local_storage = This->local_storage;
        This->local_storage = NULL;
        IHTMLStorage_Release(local_storage);
    }
    unlink_variant(&This->performance);
    unlink_ref(&This->dom_window);
}

static void HTMLWindow_destructor(DispatchEx *dispex)
{
    HTMLInnerWindow *This = impl_from_DispatchEx(dispex);
    unsigned i;

    VariantClear(&This->performance);

    for(i = 0; i < This->global_prop_cnt; i++)
        SysFreeString(This->global_props[i].name);
    free(This->global_props);

    if(This->mon)
        IMoniker_Release(This->mon);

    free(This);
}

static void HTMLWindow_last_release(DispatchEx *dispex)
{
    HTMLInnerWindow *This = impl_from_DispatchEx(dispex);
    remove_target_tasks(This->task_magic);
}

static HRESULT HTMLWindow_lookup_dispid(DispatchEx *dispex, const WCHAR *name, DWORD grfdex, DISPID *dispid)
{
    HTMLInnerWindow *This = impl_from_DispatchEx(dispex);

    return search_window_props(This, name, grfdex, dispid);
}

static const WCHAR *constructor_names[] = {
#define X(name) L ## #name,
    ALL_PROTOTYPES
#undef X
};

static int CDECL cmp_name(const void *x, const void *y)
{
    return wcscmp(*(const WCHAR **)x, *(const WCHAR **)y);
}

static HRESULT HTMLWindow_find_dispid(DispatchEx *dispex, const WCHAR *name, DWORD grfdex, DISPID *dispid)
{
    HTMLInnerWindow *This = impl_from_DispatchEx(dispex);
    HTMLOuterWindow *frame;
    global_prop_t *prop;
    HTMLElement *elem;
    HRESULT hres;

    if(dispex_compat_mode(dispex) >= COMPAT_MODE_IE9) {
        const WCHAR **constr_name = bsearch(&name, constructor_names, ARRAYSIZE(constructor_names) ,
                                            sizeof(constructor_names[0]), cmp_name);
        if(constr_name) {
            prototype_id_t id = constr_name - constructor_names + 1;
            compat_mode_t compat_mode = dispex_compat_mode(dispex);
            DispatchEx *constr;
            VARIANT v;

            if(compat_mode >= object_descriptors[id]->min_compat_mode &&
               (!object_descriptors[id]->max_compat_mode || compat_mode <= object_descriptors[id]->max_compat_mode)) {
                hres = get_constructor(This, id, &constr);
                if(FAILED(hres))
                    return hres;

                V_VT(&v) = VT_DISPATCH;
                V_DISPATCH(&v) = (IDispatch *)&constr->IWineJSDispatchHost_iface;
                return dispex_define_property(&This->event_target.dispex, name, PROPF_WRITABLE | PROPF_CONFIGURABLE, &v, dispid);
            }
        }
    }

    hres = get_frame_by_name(This->base.outer_window, name, FALSE, &frame);
    if(SUCCEEDED(hres) && frame) {
        prop = alloc_global_prop(This, GLOBAL_FRAMEVAR, name);
        if(!prop)
            return E_OUTOFMEMORY;

        *dispid = prop_to_dispid(This, prop);
        return S_OK;
    }

    hres = get_doc_elem_by_id(This->doc, name, &elem);
    if(SUCCEEDED(hres) && elem) {
        IHTMLElement_Release(&elem->IHTMLElement_iface);

        prop = alloc_global_prop(This, GLOBAL_ELEMENTVAR, name);
        if(!prop)
            return E_OUTOFMEMORY;

        *dispid = prop_to_dispid(This, prop);
        return S_OK;
    }

    return DISP_E_UNKNOWNNAME;
}

HRESULT HTMLWindow_invoke(DispatchEx *dispex, DISPID id, LCID lcid, WORD flags, DISPPARAMS *params,
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
                WARN("%s <<< %08lx\n", debugstr_w(prop->name), hres);
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

            hres = IHTMLDocument3_getElementById(&This->base.inner_window->doc->IHTMLDocument3_iface,
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

            if(This->event_target.dispex.jsdisp)
                return S_FALSE;

            hres = dispex_get_dynid(&This->event_target.dispex, prop->name, TRUE, &dispex_id);
            if(FAILED(hres))
                return hres;

            prop->type = GLOBAL_DISPEXVAR;
            prop->id = dispex_id;
            return dispex_prop_put(&This->event_target.dispex, dispex_id, 0, params->rgvarg, ei, caller);
        }
        default:
            FIXME("Not supported flags: %x\n", flags);
            return E_NOTIMPL;
        }
    case GLOBAL_FRAMEVAR:
        switch(flags) {
        case DISPATCH_PROPERTYGET: {
            HTMLOuterWindow *frame;

            hres = get_frame_by_name(This->base.outer_window, prop->name, FALSE, &frame);
            if(FAILED(hres))
                return hres;

            if(!frame)
                return DISP_E_MEMBERNOTFOUND;

            V_VT(res) = VT_DISPATCH;
            V_DISPATCH(res) = (IDispatch*)&frame->base.IHTMLWindow2_iface;
            IDispatch_AddRef(V_DISPATCH(res));
            return S_OK;
        }
        default:
            FIXME("Not supported flags: %x\n", flags);
            return E_NOTIMPL;
        }
    case GLOBAL_DISPEXVAR:
        return IWineJSDispatchHost_InvokeEx(&This->event_target.dispex.IWineJSDispatchHost_iface, prop->id, 0, flags, params, res, ei, caller);
    default:
        ERR("invalid type %d\n", prop->type);
        hres = DISP_E_MEMBERNOTFOUND;
    }

    return hres;
}

static HRESULT HTMLWindow_delete(DispatchEx *dispex, DISPID id)
{
    HTMLInnerWindow *This = impl_from_DispatchEx(dispex);
    DWORD idx = id - MSHTML_DISPID_CUSTOM_MIN;
    global_prop_t *prop;
    HRESULT hres = S_OK;

    if(idx >= This->global_prop_cnt)
        return DISP_E_MEMBERNOTFOUND;

    prop = This->global_props + idx;
    switch(prop->type) {
    case GLOBAL_SCRIPTVAR: {
        IDispatchEx *iface;
        IDispatch *disp;

        disp = get_script_disp(prop->script_host);
        if(!disp)
            return E_UNEXPECTED;

        hres = IDispatch_QueryInterface(disp, &IID_IDispatchEx, (void**)&iface);
        if(SUCCEEDED(hres)) {
            hres = IDispatchEx_DeleteMemberByDispID(iface, prop->id);
            IDispatchEx_Release(iface);
        }else {
            WARN("No IDispatchEx, so can't delete\n");
            hres = S_OK;
        }
        IDispatch_Release(disp);
        break;
    }
    default:
        break;
    }

    return hres;
}

static HRESULT HTMLWindow_next_dispid(DispatchEx *dispex, DISPID id, DISPID *pid)
{
    DWORD idx = (id == DISPID_STARTENUM) ? 0 : id - MSHTML_DISPID_CUSTOM_MIN + 1;
    HTMLInnerWindow *This = impl_from_DispatchEx(dispex);

    while(idx < This->global_prop_cnt && This->global_props[idx].type != GLOBAL_DISPEXVAR)
        idx++;
    if(idx >= This->global_prop_cnt)
        return S_FALSE;

    *pid = idx + MSHTML_DISPID_CUSTOM_MIN;
    return S_OK;
}

HRESULT HTMLWindow_get_prop_desc(DispatchEx *dispex, DISPID id, struct property_info *desc)
{
    HTMLInnerWindow *This = impl_from_DispatchEx(dispex);
    IWineJSDispatch *jsdisp;
    global_prop_t *prop;
    HRESULT hres = S_OK;

    if(id - MSHTML_DISPID_CUSTOM_MIN >= This->global_prop_cnt)
        return DISP_E_MEMBERNOTFOUND;

    prop = &This->global_props[id - MSHTML_DISPID_CUSTOM_MIN];
    desc->name = prop->name;
    desc->id = id;
    desc->flags = PROPF_WRITABLE | PROPF_CONFIGURABLE;
    desc->iid = 0;

    switch(prop->type) {
    case GLOBAL_SCRIPTVAR: {
        if((jsdisp = get_script_jsdisp(prop->script_host)))
            hres = IWineJSDispatch_GetPropertyFlags(jsdisp, prop->id, &desc->flags);
        break;
    }
    case GLOBAL_DISPEXVAR:
        desc->flags |= PROPF_ENUMERABLE;
        break;
    default:
        break;
    }

    return hres;
}

static HTMLInnerWindow *HTMLWindow_get_script_global(DispatchEx *dispex, dispex_static_data_t **dispex_data)
{
    HTMLInnerWindow *This = impl_from_DispatchEx(dispex);
    lock_document_mode(This->doc);
    *dispex_data = &Window_dispex;
    return This;
}

static IWineJSDispatchHost *HTMLWindow_get_outer_iface(DispatchEx *dispex)
{
    HTMLInnerWindow *This = impl_from_DispatchEx(dispex);
    IWineJSDispatchHost *ret;

    if(This->base.outer_window)
        ret = &This->base.outer_window->IWineJSDispatchHost_iface;
    else
        ret = &This->event_target.dispex.IWineJSDispatchHost_iface;
    IWineJSDispatchHost_AddRef(ret);
    return ret;
}

static nsISupports *HTMLWindow_get_gecko_target(DispatchEx *dispex)
{
    HTMLInnerWindow *This = impl_from_DispatchEx(dispex);
    return (nsISupports*)This->base.outer_window->nswindow;
}

static void HTMLWindow_bind_event(DispatchEx *dispex, eventid_t eid)
{
    HTMLInnerWindow *This = impl_from_DispatchEx(dispex);
    ensure_doc_nsevent_handler(This->doc, NULL, eid);
}

static HRESULT IHTMLWindow2_location_hook(DispatchEx *dispex, WORD flags, DISPPARAMS *dp, VARIANT *res,
        EXCEPINFO *ei, IServiceProvider *caller)
{
    HTMLInnerWindow *This = impl_from_DispatchEx(dispex);
    HTMLLocation *location;
    HRESULT hres;

    if(!(flags & DISPATCH_PROPERTYPUT))
        return S_FALSE;

    TRACE("forwarding to location.href\n");

    hres = get_location(This->base.outer_window, &location);
    if(FAILED(hres))
        return hres;

    hres = IWineJSDispatchHost_InvokeEx(&location->dispex.IWineJSDispatchHost_iface, DISPID_VALUE, 0, flags, dp, res, ei, caller);
    IHTMLLocation_Release(&location->IHTMLLocation_iface);
    return hres;
}

static HRESULT IHTMLWindow3_setTimeout_hook(DispatchEx *dispex, WORD flags, DISPPARAMS *dp, VARIANT *res,
        EXCEPINFO *ei, IServiceProvider *caller)
{
    VARIANT args[2];
    DISPPARAMS new_dp = { args, NULL, 2, 0 };

    /*
     * setTimeout calls should use default value 0 for the second argument if only one is provided,
     * but IDL file does not reflect that. We fixup arguments here instead.
     */
    if(!(flags & DISPATCH_METHOD) || dp->cArgs != 1 || dp->cNamedArgs)
        return S_FALSE;

    TRACE("Fixing args\n");

    V_VT(args) = VT_I4;
    V_I4(args) = 0;
    args[1] = dp->rgvarg[0];
    return dispex_call_builtin(dispex, DISPID_IHTMLWINDOW3_SETTIMEOUT, &new_dp, res, ei, caller);
}

static HRESULT IHTMLWindow6_postMessage_hook(DispatchEx *dispex, WORD flags, DISPPARAMS *dp, VARIANT *res,
        EXCEPINFO *ei, IServiceProvider *caller)
{
    HTMLInnerWindow *This = impl_from_DispatchEx(dispex);
    BSTR targetOrigin, converted_msg = NULL;
    VARIANT msg, transfer, converted;
    compat_mode_t compat_mode;
    HRESULT hres;

    if(!(flags & DISPATCH_METHOD) || dp->cArgs < 2 || dp->cNamedArgs)
        return S_FALSE;
    compat_mode = dispex_compat_mode(&This->event_target.dispex);

    msg = dp->rgvarg[dp->cArgs - 1];
    V_VT(&transfer) = VT_EMPTY;
    if(compat_mode >= COMPAT_MODE_IE10 && dp->cArgs > 2)
        transfer = dp->rgvarg[dp->cArgs - 3];

    TRACE("(%p)->(msg %s, targetOrigin %s, transfer %s)\n", This, debugstr_variant(&msg),
          debugstr_variant(&dp->rgvarg[dp->cArgs - 2]), debugstr_variant(&transfer));

    if(compat_mode < COMPAT_MODE_IE10 && V_VT(&msg) != VT_BSTR) {
        hres = change_type(&msg, &dp->rgvarg[dp->cArgs - 1], VT_BSTR, caller);
        if(FAILED(hres))
            return hres;
        converted_msg = V_BSTR(&msg);
    }

    if(V_VT(&dp->rgvarg[dp->cArgs - 2]) == VT_BSTR) {
        targetOrigin = V_BSTR(&dp->rgvarg[dp->cArgs - 2]);
        V_BSTR(&converted) = NULL;
    }else {
        if(compat_mode < COMPAT_MODE_IE10) {
            SysFreeString(converted_msg);
            return E_INVALIDARG;
        }
        hres = change_type(&converted, &dp->rgvarg[dp->cArgs - 2], VT_BSTR, caller);
        if(FAILED(hres)) {
            SysFreeString(converted_msg);
            return hres;
        }
        targetOrigin = V_BSTR(&converted);
    }

    hres = post_message(This, msg, targetOrigin, transfer, caller, compat_mode);

    SysFreeString(V_BSTR(&converted));
    SysFreeString(converted_msg);
    return hres;
}

static void HTMLWindow_init_dispex_info(dispex_data_t *info, compat_mode_t compat_mode)
{
    static const dispex_hook_t window2_hooks[] = {
        {DISPID_IHTMLWINDOW2_LOCATION, IHTMLWindow2_location_hook},
        {DISPID_UNKNOWN}
    };
    static const dispex_hook_t window2_ie11_hooks[] = {
        {DISPID_IHTMLWINDOW2_LOCATION,   IHTMLWindow2_location_hook},
        {DISPID_IHTMLWINDOW2_EXECSCRIPT, NULL},
        {DISPID_UNKNOWN}
    };
    static const dispex_hook_t window3_hooks[] = {
        {DISPID_IHTMLWINDOW3_SETTIMEOUT, IHTMLWindow3_setTimeout_hook},
        {DISPID_UNKNOWN}
    };
    static const dispex_hook_t window3_ie11_hooks[] = {
        {DISPID_IHTMLWINDOW3_SETTIMEOUT,  IHTMLWindow3_setTimeout_hook},
        {DISPID_IHTMLWINDOW3_ATTACHEVENT, NULL},
        {DISPID_IHTMLWINDOW3_DETACHEVENT, NULL},
        {DISPID_UNKNOWN}
    };
    static const dispex_hook_t window4_ie11_hooks[] = {
        {DISPID_IHTMLWINDOW4_CREATEPOPUP, NULL},
        {DISPID_UNKNOWN}
    };
    static const dispex_hook_t window6_hooks[] = {
        {DISPID_IHTMLWINDOW6_POSTMESSAGE, IHTMLWindow6_postMessage_hook},
        {DISPID_UNKNOWN}
    };

    if(compat_mode >= COMPAT_MODE_IE9)
        dispex_info_add_interface(info, IHTMLWindow7_tid, NULL);
    else
        dispex_info_add_interface(info, IWineHTMLWindowCompatPrivate_tid, NULL);
    if(compat_mode >= COMPAT_MODE_IE10)
        dispex_info_add_interface(info, IWineHTMLWindowPrivate_tid, NULL);

    dispex_info_add_interface(info, IHTMLWindow6_tid, window6_hooks);
    if(compat_mode < COMPAT_MODE_IE9)
        dispex_info_add_interface(info, IHTMLWindow5_tid, NULL);
    dispex_info_add_interface(info, IHTMLWindow4_tid, compat_mode >= COMPAT_MODE_IE11 ? window4_ie11_hooks : NULL);
    dispex_info_add_interface(info, IHTMLWindow3_tid, compat_mode >= COMPAT_MODE_IE11 ? window3_ie11_hooks : window3_hooks);
    dispex_info_add_interface(info, IHTMLWindow2_tid, compat_mode >= COMPAT_MODE_IE11 ? window2_ie11_hooks : window2_hooks);
    EventTarget_init_dispex_info(info, compat_mode);
}

static IHTMLEventObj *HTMLWindow_set_current_event(DispatchEx *dispex, IHTMLEventObj *event)
{
    HTMLInnerWindow *This = impl_from_DispatchEx(dispex);
    return default_set_current_event(This, event);
}

static const event_target_vtbl_t HTMLWindow_event_target_vtbl = {
    {
        .query_interface     = HTMLWindow_query_interface,
        .destructor          = HTMLWindow_destructor,
        .traverse            = HTMLWindow_traverse,
        .unlink              = HTMLWindow_unlink,
        .last_release        = HTMLWindow_last_release,
        .lookup_dispid       = HTMLWindow_lookup_dispid,
        .find_dispid         = HTMLWindow_find_dispid,
        .invoke              = HTMLWindow_invoke,
        .delete              = HTMLWindow_delete,
        .next_dispid         = HTMLWindow_next_dispid,
        .get_prop_desc       = HTMLWindow_get_prop_desc,
        .get_script_global   = HTMLWindow_get_script_global,
        .get_outer_iface     = HTMLWindow_get_outer_iface,
    },
    .get_gecko_target        = HTMLWindow_get_gecko_target,
    .bind_event              = HTMLWindow_bind_event,
    .set_current_event       = HTMLWindow_set_current_event
};

dispex_static_data_t Window_dispex = {
    .id         = PROT_Window,
    .vtbl       = &HTMLWindow_event_target_vtbl.dispex_vtbl,
    .disp_tid   = DispHTMLWindow2_tid,
    .init_info  = HTMLWindow_init_dispex_info,
};

static nsresult NSAPI outer_window_traverse(void *ccp, void *p, nsCycleCollectionTraversalCallback *cb)
{
    HTMLOuterWindow *window = HTMLOuterWindow_from_IHTMLWindow2(p);

    describe_cc_node(&window->ccref, "OuterWindow", cb);

    if(window->pending_window)
        note_cc_edge((nsISupports*)&window->pending_window->base.IHTMLWindow2_iface, "pending_window", cb);
    if(window->base.inner_window)
        note_cc_edge((nsISupports*)&window->base.inner_window->base.IHTMLWindow2_iface, "inner_window", cb);
    if(window->location)
        note_cc_edge((nsISupports*)&window->location->IHTMLLocation_iface, "location", cb);
    if(window->nswindow)
        note_cc_edge((nsISupports*)window->nswindow, "nswindow", cb);
    if(window->window_proxy)
        note_cc_edge((nsISupports*)window->window_proxy, "window_proxy", cb);
    return NS_OK;
}

static nsresult NSAPI outer_window_unlink(void *p)
{
    HTMLOuterWindow *window = HTMLOuterWindow_from_IHTMLWindow2(p);

    if(window->browser) {
        list_remove(&window->browser_entry);
        window->browser = NULL;
    }
    if(window->pending_window) {
        HTMLInnerWindow *pending_window = window->pending_window;
        abort_window_bindings(pending_window);
        pending_window->base.outer_window = NULL;
        window->pending_window = NULL;
        IHTMLWindow2_Release(&pending_window->base.IHTMLWindow2_iface);
    }

    set_current_mon(window, NULL, 0);
    set_current_uri(window, NULL);
    if(window->base.inner_window)
        detach_inner_window(window->base.inner_window);
    if(window->location) {
        HTMLLocation *location = window->location;
        window->location = NULL;
        IHTMLLocation_Release(&location->IHTMLLocation_iface);
    }
    if(window->frame_element) {
        window->frame_element->content_window = NULL;
        window->frame_element = NULL;
    }
    unlink_ref(&window->nswindow);
    if(window->window_proxy) {
        unlink_ref(&window->window_proxy);
        wine_rb_remove(&window_map, &window->entry);
    }
    return NS_OK;
}

static void NSAPI outer_window_delete_cycle_collectable(void *p)
{
    HTMLOuterWindow *window = HTMLOuterWindow_from_IHTMLWindow2(p);
    outer_window_unlink(p);
    free(window);
}

void init_window_cc(void)
{
    static const CCObjCallback ccp_callback = {
        outer_window_traverse,
        outer_window_unlink,
        outer_window_delete_cycle_collectable
    };
    ccp_init(&outer_window_ccp, &ccp_callback);
}

HTMLInnerWindow *get_script_global(DispatchEx *dispex)
{
    IWineJSDispatchHost *disp;
    HTMLInnerWindow *ret;
    HRESULT hres;

    if(!dispex->jsdisp)
        return NULL;
    hres = IWineJSDispatch_GetScriptGlobal(dispex->jsdisp, &disp);
    if(FAILED(hres))
        return NULL;

    assert(disp->lpVtbl == &WindowDispExVtbl);
    ret = impl_from_IWineJSDispatchHost(disp)->base.inner_window;
    if(ret)
        IHTMLWindow2_AddRef(&ret->base.IHTMLWindow2_iface);
    IWineJSDispatchHost_Release(disp);
    return ret;
}

static void *alloc_window(size_t size)
{
    HTMLWindow *window;

    window = calloc(1, size);
    if(!window)
        return NULL;

    window->IHTMLWindow3_iface.lpVtbl = &HTMLWindow3Vtbl;
    window->IHTMLWindow4_iface.lpVtbl = &HTMLWindow4Vtbl;
    window->IHTMLWindow5_iface.lpVtbl = &HTMLWindow5Vtbl;
    window->IHTMLWindow6_iface.lpVtbl = &HTMLWindow6Vtbl;
    window->IHTMLWindow7_iface.lpVtbl = &HTMLWindow7Vtbl;
    window->IHTMLPrivateWindow_iface.lpVtbl = &HTMLPrivateWindowVtbl;
    window->IServiceProvider_iface.lpVtbl = &ServiceProviderVtbl;
    window->ITravelLogClient_iface.lpVtbl = &TravelLogClientVtbl;
    window->IObjectIdentity_iface.lpVtbl = &ObjectIdentityVtbl;
    window->IProvideMultipleClassInfo_iface.lpVtbl = &ProvideMultipleClassInfoVtbl;
    window->IWineHTMLWindowPrivate_iface.lpVtbl = &WineHTMLWindowPrivateVtbl;
    window->IWineHTMLWindowCompatPrivate_iface.lpVtbl = &WineHTMLWindowCompatPrivateVtbl;

    return window;
}

static HRESULT create_inner_window(HTMLOuterWindow *outer_window, IMoniker *mon, HTMLInnerWindow **ret)
{
    HTMLInnerWindow *window;

    window = alloc_window(sizeof(HTMLInnerWindow));
    if(!window)
        return E_OUTOFMEMORY;
    window->base.IHTMLWindow2_iface.lpVtbl = &HTMLWindow2Vtbl;

    list_init(&window->documents);
    list_init(&window->children);
    list_init(&window->script_hosts);
    list_init(&window->bindings);
    list_init(&window->script_queue);

    window->base.outer_window = outer_window;
    window->base.inner_window = window;

    init_event_target(&window->event_target, &Window_dispex, NULL);

    window->task_magic = get_task_target_magic();

    if(mon) {
        IMoniker_AddRef(mon);
        window->mon = mon;
    }

    *ret = window;
    return S_OK;
}

HRESULT create_outer_window(GeckoBrowser *browser, mozIDOMWindowProxy *mozwindow,
        HTMLOuterWindow *parent, HTMLOuterWindow **ret)
{
    HTMLOuterWindow *window;
    nsresult nsres;
    HRESULT hres;

    window = alloc_window(sizeof(HTMLOuterWindow));
    if(!window)
        return E_OUTOFMEMORY;
    window->base.IHTMLWindow2_iface.lpVtbl = &outer_window_HTMLWindow2Vtbl;
    window->IWineJSDispatchHost_iface.lpVtbl = &WindowDispExVtbl;
    window->IEventTarget_iface.lpVtbl = &EventTargetVtbl;

    window->base.outer_window = window;
    window->base.inner_window = NULL;
    window->browser = browser;
    list_add_head(&browser->outer_windows, &window->browser_entry);
    ccref_init(&window->ccref, 1);

    mozIDOMWindowProxy_AddRef(mozwindow);
    window->window_proxy = mozwindow;
    nsres = mozIDOMWindowProxy_QueryInterface(mozwindow, &IID_nsIDOMWindow, (void**)&window->nswindow);
    assert(nsres == NS_OK);

    window->readystate = READYSTATE_UNINITIALIZED;
    window->task_magic = get_task_target_magic();

    wine_rb_put(&window_map, window->window_proxy, &window->entry);

    hres = create_pending_window(window, NULL);
    if(SUCCEEDED(hres))
        hres = update_window_doc(window->pending_window);
    if(FAILED(hres)) {
        IHTMLWindow2_Release(&window->base.IHTMLWindow2_iface);
        return hres;
    }

    /* Initial empty doc does not have unload events or timings */
    window->base.inner_window->doc->unload_sent = TRUE;

    if(parent) {
        IHTMLWindow2_AddRef(&window->base.IHTMLWindow2_iface);

        window->parent = parent;
        list_add_tail(&parent->base.inner_window->children, &window->sibling_entry);
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
    compat_mode_t parent_mode = COMPAT_MODE_QUIRKS;
    mozIDOMWindow *gecko_inner_window;
    nsIDOMDocument *nsdoc;
    nsresult nsres;
    HRESULT hres;

    assert(!window->doc && !window->dom_window);

    if(!outer_window)
        return E_UNEXPECTED;

    nsres = nsIDOMWindow_GetDocument(outer_window->nswindow, &nsdoc);
    if(NS_FAILED(nsres) || !nsdoc) {
        ERR("GetDocument failed: %08lx\n", nsres);
        return E_FAIL;
    }

    nsres = nsIDOMWindow_GetInnerWindow(outer_window->nswindow, &gecko_inner_window);
    assert(nsres == NS_OK);
    nsres = mozIDOMWindow_QueryInterface(gecko_inner_window, &IID_nsIDOMWindow, (void **)&window->dom_window);
    assert(nsres == NS_OK);
    mozIDOMWindow_Release(gecko_inner_window);

    if(outer_window->parent)
        parent_mode = outer_window->parent->base.inner_window->doc->document_mode;

    hres = create_document_node(nsdoc, outer_window->browser, window, window, parent_mode, &window->doc);
    nsIDOMDocument_Release(nsdoc);
    if(FAILED(hres))
        return hres;

    if(window != outer_window->pending_window) {
        ERR("not current pending window\n");
        return S_OK;
    }

    if(outer_window->base.inner_window)
        detach_inner_window(outer_window->base.inner_window);
    outer_window->base.inner_window = window;
    outer_window->pending_window = NULL;

    if(is_main_content_window(outer_window) || !outer_window->browser->content_window) {
        HTMLDocumentObj *doc_obj = outer_window->browser->doc;
        if(doc_obj) {
            if(doc_obj->doc_node)
                IHTMLDOMNode_Release(&doc_obj->doc_node->node.IHTMLDOMNode_iface);
            doc_obj->doc_node = window->doc;
            IHTMLDOMNode_AddRef(&window->doc->node.IHTMLDOMNode_iface);
        }
    }

    return hres;
}
