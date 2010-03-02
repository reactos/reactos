/*
 * Copyright 2006 Jacek Caban for CodeWeavers
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

#include "wine/debug.h"
#include "wine/unicode.h"

#include "mshtml_private.h"
#include "htmlevent.h"
#include "resource.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

static struct list window_list = LIST_INIT(window_list);

static void window_set_docnode(HTMLWindow *window, HTMLDocumentNode *doc_node)
{
    if(window->doc) {
        abort_document_bindings(window->doc);
        window->doc->basedoc.window = NULL;
        htmldoc_release(&window->doc->basedoc);
    }
    window->doc = doc_node;
    if(doc_node)
        htmldoc_addref(&doc_node->basedoc);

    if(window->doc_obj && window->doc_obj->basedoc.window == window) {
        if(window->doc_obj->basedoc.doc_node)
            htmldoc_release(&window->doc_obj->basedoc.doc_node->basedoc);
        window->doc_obj->basedoc.doc_node = doc_node;
        if(doc_node)
            htmldoc_addref(&doc_node->basedoc);
    }
}

nsIDOMWindow *get_nsdoc_window(nsIDOMDocument *nsdoc)
{
    nsIDOMDocumentView *nsdocview;
    nsIDOMAbstractView *nsview;
    nsIDOMWindow *nswindow;
    nsresult nsres;

    nsres = nsIDOMDocument_QueryInterface(nsdoc, &IID_nsIDOMDocumentView, (void**)&nsdocview);
    nsIDOMDocument_Release(nsdoc);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIDOMDocumentView iface: %08x\n", nsres);
        return NULL;
    }

    nsres = nsIDOMDocumentView_GetDefaultView(nsdocview, &nsview);
    nsIDOMDocumentView_Release(nsview);
    if(NS_FAILED(nsres)) {
        ERR("GetDefaultView failed: %08x\n", nsres);
        return NULL;
    }

    nsres = nsIDOMAbstractView_QueryInterface(nsview, &IID_nsIDOMWindow, (void**)&nswindow);
    nsIDOMAbstractView_Release(nsview);
    if(NS_FAILED(nsres)) {
        ERR("Coult not get nsIDOMWindow iface: %08x\n", nsres);
        return NULL;
    }

    return nswindow;
}

static void release_children(HTMLWindow *This)
{
    HTMLWindow *child;

    while(!list_empty(&This->children)) {
        child = LIST_ENTRY(list_tail(&This->children), HTMLWindow, sibling_entry);

        list_remove(&child->sibling_entry);
        child->parent = NULL;
        IHTMLWindow2_Release(HTMLWINDOW2(child));
    }
}

static HRESULT get_location(HTMLWindow *This, HTMLLocation **ret)
{
    if(This->location) {
        IHTMLLocation_AddRef(HTMLLOCATION(This->location));
    }else {
        HRESULT hres;

        hres = HTMLLocation_Create(This, &This->location);
        if(FAILED(hres))
            return hres;
    }

    *ret = This->location;
    return S_OK;
}

static inline HRESULT set_window_event(HTMLWindow *window, eventid_t eid, VARIANT *var)
{
    if(!window->doc) {
        FIXME("No document\n");
        return E_FAIL;
    }

    return set_event_handler(&window->doc->body_event_target, window->doc, eid, var);
}

static inline HRESULT get_window_event(HTMLWindow *window, eventid_t eid, VARIANT *var)
{
    if(!window->doc) {
        FIXME("No document\n");
        return E_FAIL;
    }

    return get_event_handler(&window->doc->body_event_target, eid, var);
}

#define HTMLWINDOW2_THIS(iface) DEFINE_THIS(HTMLWindow, HTMLWindow2, iface)

static HRESULT WINAPI HTMLWindow2_QueryInterface(IHTMLWindow2 *iface, REFIID riid, void **ppv)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = HTMLWINDOW2(This);
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = HTMLWINDOW2(This);
    }else if(IsEqualGUID(&IID_IDispatchEx, riid)) {
        TRACE("(%p)->(IID_IDispatchEx %p)\n", This, ppv);
        *ppv = DISPATCHEX(This);
    }else if(IsEqualGUID(&IID_IHTMLFramesCollection2, riid)) {
        TRACE("(%p)->(IID_IHTMLFramesCollection2 %p)\n", This, ppv);
        *ppv = HTMLWINDOW2(This);
    }else if(IsEqualGUID(&IID_IHTMLWindow2, riid)) {
        TRACE("(%p)->(IID_IHTMLWindow2 %p)\n", This, ppv);
        *ppv = HTMLWINDOW2(This);
    }else if(IsEqualGUID(&IID_IHTMLWindow3, riid)) {
        TRACE("(%p)->(IID_IHTMLWindow3 %p)\n", This, ppv);
        *ppv = HTMLWINDOW3(This);
    }else if(IsEqualGUID(&IID_IHTMLWindow4, riid)) {
        TRACE("(%p)->(IID_IHTMLWindow4 %p)\n", This, ppv);
        *ppv = HTMLWINDOW4(This);
    }else if(dispex_query_interface(&This->dispex, riid, ppv)) {
        return *ppv ? S_OK : E_NOINTERFACE;
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI HTMLWindow2_AddRef(IHTMLWindow2 *iface)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLWindow2_Release(IHTMLWindow2 *iface)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        DWORD i;

        remove_target_tasks(This->task_magic);
        set_window_bscallback(This, NULL);
        set_current_mon(This, NULL);
        window_set_docnode(This, NULL);
        release_children(This);

        if(This->frame_element)
            This->frame_element->content_window = NULL;

        if(This->option_factory) {
            This->option_factory->window = NULL;
            IHTMLOptionElementFactory_Release(HTMLOPTFACTORY(This->option_factory));
        }

        if(This->image_factory) {
            This->image_factory->window = NULL;
            IHTMLImageElementFactory_Release(HTMLIMGFACTORY(This->image_factory));
        }

        if(This->location) {
            This->location->window = NULL;
            IHTMLLocation_Release(HTMLLOCATION(This->location));
        }

        if(This->screen)
            IHTMLScreen_Release(This->screen);

        for(i=0; i < This->global_prop_cnt; i++)
            heap_free(This->global_props[i].name);

        This->window_ref->window = NULL;
        windowref_release(This->window_ref);

        heap_free(This->global_props);
        release_script_hosts(This);

        if(This->nswindow)
            nsIDOMWindow_Release(This->nswindow);

        list_remove(&This->entry);
        release_dispex(&This->dispex);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI HTMLWindow2_GetTypeInfoCount(IHTMLWindow2 *iface, UINT *pctinfo)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);

    return IDispatchEx_GetTypeInfoCount(DISPATCHEX(This), pctinfo);
}

static HRESULT WINAPI HTMLWindow2_GetTypeInfo(IHTMLWindow2 *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);

    return IDispatchEx_GetTypeInfo(DISPATCHEX(This), iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLWindow2_GetIDsOfNames(IHTMLWindow2 *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);

    return IDispatchEx_GetIDsOfNames(DISPATCHEX(This), riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLWindow2_Invoke(IHTMLWindow2 *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);

    return IDispatchEx_Invoke(DISPATCHEX(This), dispIdMember, riid, lcid, wFlags, pDispParams,
            pVarResult, pExcepInfo, puArgErr);
}

static HRESULT get_frame_by_index(nsIDOMWindowCollection *nsFrames, PRUint32 index, HTMLWindow **ret)
{
    PRUint32 length;
    nsIDOMWindow *nsWindow;
    nsresult nsres;

    nsres = nsIDOMWindowCollection_GetLength(nsFrames, &length);
    if(NS_FAILED(nsres)) {
        FIXME("nsIDOMWindowCollection_GetLength failed: 0x%08x\n", nsres);
        return E_FAIL;
    }

    if(index >= length)
        return DISP_E_MEMBERNOTFOUND;

    nsres = nsIDOMWindowCollection_Item(nsFrames, index, &nsWindow);
    if(NS_FAILED(nsres)) {
        FIXME("nsIDOMWindowCollection_Item failed: 0x%08x\n", nsres);
        return E_FAIL;
    }

    *ret = nswindow_to_window(nsWindow);

    nsIDOMWindow_Release(nsWindow);

    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_item(IHTMLWindow2 *iface, VARIANT *pvarIndex, VARIANT *pvarResult)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    nsIDOMWindowCollection *nsFrames;
    HTMLWindow *window;
    HRESULT hres;
    nsresult nsres;

    TRACE("(%p)->(%p %p)\n", This, pvarIndex, pvarResult);

    nsres = nsIDOMWindow_GetFrames(This->nswindow, &nsFrames);
    if(NS_FAILED(nsres)) {
        FIXME("nsIDOMWindow_GetFrames failed: 0x%08x\n", nsres);
        return E_FAIL;
    }

    if(V_VT(pvarIndex) == VT_I4) {
        int index = V_I4(pvarIndex);
        TRACE("Getting index %d\n", index);
        if(index < 0) {
            hres = DISP_E_MEMBERNOTFOUND;
            goto cleanup;
        }
        hres = get_frame_by_index(nsFrames, index, &window);
        if(FAILED(hres))
            goto cleanup;
    }else if(V_VT(pvarIndex) == VT_UINT) {
        unsigned int index = V_UINT(pvarIndex);
        TRACE("Getting index %u\n", index);
        hres = get_frame_by_index(nsFrames, index, &window);
        if(FAILED(hres))
            goto cleanup;
    }else if(V_VT(pvarIndex) == VT_BSTR) {
        BSTR str = V_BSTR(pvarIndex);
        PRUint32 length, i;

        TRACE("Getting name %s\n", wine_dbgstr_w(str));

        nsres = nsIDOMWindowCollection_GetLength(nsFrames, &length);

        window = NULL;
        for(i = 0; i < length && !window; ++i) {
            HTMLWindow *cur_window;
            nsIDOMWindow *nsWindow;
            BSTR id;

            nsres = nsIDOMWindowCollection_Item(nsFrames, i, &nsWindow);
            if(NS_FAILED(nsres)) {
                FIXME("nsIDOMWindowCollection_Item failed: 0x%08x\n", nsres);
                hres = E_FAIL;
                goto cleanup;
            }

            cur_window = nswindow_to_window(nsWindow);

            nsIDOMWindow_Release(nsWindow);

            hres = IHTMLElement_get_id(HTMLELEM(&cur_window->frame_element->element), &id);
            if(FAILED(hres)) {
                FIXME("IHTMLElement_get_id failed: 0x%08x\n", hres);
                goto cleanup;
            }

            if(!strcmpW(id, str))
                window = cur_window;

            SysFreeString(id);
        }

        if(!window) {
            hres = DISP_E_MEMBERNOTFOUND;
            goto cleanup;
        }
    }else {
        hres = E_INVALIDARG;
        goto cleanup;
    }

    IHTMLWindow2_AddRef(HTMLWINDOW2(window));
    V_VT(pvarResult) = VT_DISPATCH;
    V_DISPATCH(pvarResult) = (IDispatch*)window;

    hres = S_OK;

cleanup:
    nsIDOMWindowCollection_Release(nsFrames);

    return hres;
}

static HRESULT WINAPI HTMLWindow2_get_length(IHTMLWindow2 *iface, LONG *p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    nsIDOMWindowCollection *nscollection;
    PRUint32 length;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMWindow_GetFrames(This->nswindow, &nscollection);
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
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%p): semi-stub\n", This, p);

    /* FIXME: Should return a separate Window object */
    *p = (IHTMLFramesCollection2*)HTMLWINDOW2(This);
    HTMLWindow2_AddRef(iface);
    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_put_defaultStatus(IHTMLWindow2 *iface, BSTR v)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_get_defaultStatus(IHTMLWindow2 *iface, BSTR *p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_put_status(IHTMLWindow2 *iface, BSTR v)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_get_status(IHTMLWindow2 *iface, BSTR *p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_setTimeout(IHTMLWindow2 *iface, BSTR expression,
        LONG msec, VARIANT *language, LONG *timerID)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    VARIANT expr_var;

    TRACE("(%p)->(%s %d %p %p)\n", This, debugstr_w(expression), msec, language, timerID);

    V_VT(&expr_var) = VT_BSTR;
    V_BSTR(&expr_var) = expression;

    return IHTMLWindow3_setTimeout(HTMLWINDOW3(This), &expr_var, msec, language, timerID);
}

static HRESULT WINAPI HTMLWindow2_clearTimeout(IHTMLWindow2 *iface, LONG timerID)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);

    TRACE("(%p)->(%d)\n", This, timerID);

    return clear_task_timer(&This->doc->basedoc, FALSE, timerID);
}

static HRESULT WINAPI HTMLWindow2_alert(IHTMLWindow2 *iface, BSTR message)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    WCHAR wszTitle[100];

    TRACE("(%p)->(%s)\n", This, debugstr_w(message));

    if(!LoadStringW(get_shdoclc(), IDS_MESSAGE_BOX_TITLE, wszTitle,
                    sizeof(wszTitle)/sizeof(WCHAR))) {
        WARN("Could not load message box title: %d\n", GetLastError());
        return S_OK;
    }

    MessageBoxW(This->doc_obj->hwnd, message, wszTitle, MB_ICONWARNING);
    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_confirm(IHTMLWindow2 *iface, BSTR message,
        VARIANT_BOOL *confirmed)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    WCHAR wszTitle[100];

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(message), confirmed);

    if(!confirmed) return E_INVALIDARG;

    if(!LoadStringW(get_shdoclc(), IDS_MESSAGE_BOX_TITLE, wszTitle,
                sizeof(wszTitle)/sizeof(WCHAR))) {
        WARN("Could not load message box title: %d\n", GetLastError());
        *confirmed = VARIANT_TRUE;
        return S_OK;
    }

    if(MessageBoxW(This->doc_obj->hwnd, message, wszTitle,
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
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    prompt_arg arg;

    TRACE("(%p)->(%s %s %p)\n", This, debugstr_w(message), debugstr_w(dststr), textdata);

    if(textdata) V_VT(textdata) = VT_NULL;

    arg.message = message;
    arg.dststr = dststr;
    arg.textdata = textdata;

    DialogBoxParamW(hInst, MAKEINTRESOURCEW(ID_PROMPT_DIALOG),
            This->doc_obj->hwnd, prompt_dlgproc, (LPARAM)&arg);
    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_get_Image(IHTMLWindow2 *iface, IHTMLImageElementFactory **p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->image_factory)
        This->image_factory = HTMLImageElementFactory_Create(This);

    *p = HTMLIMGFACTORY(This->image_factory);
    IHTMLImageElementFactory_AddRef(*p);

    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_get_location(IHTMLWindow2 *iface, IHTMLLocation **p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    HTMLLocation *location;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    hres = get_location(This, &location);
    if(FAILED(hres))
        return hres;

    *p = HTMLLOCATION(location);
    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_get_history(IHTMLWindow2 *iface, IOmHistory **p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_close(IHTMLWindow2 *iface)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_put_opener(IHTMLWindow2 *iface, VARIANT v)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(v(%d))\n", This, V_VT(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_get_opener(IHTMLWindow2 *iface, VARIANT *p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_get_navigator(IHTMLWindow2 *iface, IOmNavigator **p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = OmNavigator_Create();
    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_put_name(IHTMLWindow2 *iface, BSTR v)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    nsAString name_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&name_str, v);
    nsres = nsIDOMWindow_SetName(This->nswindow, &name_str);
    nsAString_Finish(&name_str);
    if(NS_FAILED(nsres))
        ERR("SetName failed: %08x\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_get_name(IHTMLWindow2 *iface, BSTR *p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    nsAString name_str;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&name_str, NULL);
    nsres = nsIDOMWindow_GetName(This->nswindow, &name_str);
    if(NS_SUCCEEDED(nsres)) {
        const PRUnichar *name;

        nsAString_GetData(&name_str, &name);
        if(*name) {
            *p = SysAllocString(name);
            hres = *p ? S_OK : E_OUTOFMEMORY;
        }else {
            *p = NULL;
            hres = S_OK;
        }
    }else {
        ERR("GetName failed: %08x\n", nsres);
        hres = E_FAIL;
    }
    nsAString_Finish(&name_str);

    return hres;
}

static HRESULT WINAPI HTMLWindow2_get_parent(IHTMLWindow2 *iface, IHTMLWindow2 **p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    TRACE("(%p)->(%p)\n", This, p);

    if(This->parent) {
        *p = HTMLWINDOW2(This->parent);
        IHTMLWindow2_AddRef(*p);
    }else
        *p = NULL;

    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_open(IHTMLWindow2 *iface, BSTR url, BSTR name,
         BSTR features, VARIANT_BOOL replace, IHTMLWindow2 **pomWindowResult)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%s %s %s %x %p)\n", This, debugstr_w(url), debugstr_w(name),
          debugstr_w(features), replace, pomWindowResult);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_get_self(IHTMLWindow2 *iface, IHTMLWindow2 **p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    /* FIXME: We should return kind of proxy window here. */
    IHTMLWindow2_AddRef(HTMLWINDOW2(This));
    *p = HTMLWINDOW2(This);
    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_get_top(IHTMLWindow2 *iface, IHTMLWindow2 **p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface), *curr;
    TRACE("(%p)->(%p)\n", This, p);

    curr = This;
    while(curr->parent)
        curr = curr->parent;
    *p = HTMLWINDOW2(curr);
    IHTMLWindow2_AddRef(*p);

    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_get_window(IHTMLWindow2 *iface, IHTMLWindow2 **p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    /* FIXME: We should return kind of proxy window here. */
    IHTMLWindow2_AddRef(HTMLWINDOW2(This));
    *p = HTMLWINDOW2(This);
    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_navigate(IHTMLWindow2 *iface, BSTR url)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(url));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_put_onfocus(IHTMLWindow2 *iface, VARIANT v)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(v(%d))\n", This, V_VT(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_get_onfocus(IHTMLWindow2 *iface, VARIANT *p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_put_onblur(IHTMLWindow2 *iface, VARIANT v)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(v(%d))\n", This, V_VT(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_get_onblur(IHTMLWindow2 *iface, VARIANT *p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_put_onload(IHTMLWindow2 *iface, VARIANT v)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_window_event(This, EVENTID_LOAD, &v);
}

static HRESULT WINAPI HTMLWindow2_get_onload(IHTMLWindow2 *iface, VARIANT *p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_window_event(This, EVENTID_LOAD, p);
}

static HRESULT WINAPI HTMLWindow2_put_onbeforeunload(IHTMLWindow2 *iface, VARIANT v)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);

    TRACE("(%p)->(v(%d))\n", This, V_VT(&v));

    return set_window_event(This, EVENTID_BEFOREUNLOAD, &v);
}

static HRESULT WINAPI HTMLWindow2_get_onbeforeunload(IHTMLWindow2 *iface, VARIANT *p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_window_event(This, EVENTID_BEFOREUNLOAD, p);
}

static HRESULT WINAPI HTMLWindow2_put_onunload(IHTMLWindow2 *iface, VARIANT v)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(v(%d))\n", This, V_VT(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_get_onunload(IHTMLWindow2 *iface, VARIANT *p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_put_onhelp(IHTMLWindow2 *iface, VARIANT v)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(v(%d))\n", This, V_VT(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_get_onhelp(IHTMLWindow2 *iface, VARIANT *p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_put_onerror(IHTMLWindow2 *iface, VARIANT v)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(v(%d))\n", This, V_VT(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_get_onerror(IHTMLWindow2 *iface, VARIANT *p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_put_onresize(IHTMLWindow2 *iface, VARIANT v)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_window_event(This, EVENTID_RESIZE, &v);
}

static HRESULT WINAPI HTMLWindow2_get_onresize(IHTMLWindow2 *iface, VARIANT *p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_window_event(This, EVENTID_RESIZE, p);
}

static HRESULT WINAPI HTMLWindow2_put_onscroll(IHTMLWindow2 *iface, VARIANT v)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(v(%d))\n", This, V_VT(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_get_onscroll(IHTMLWindow2 *iface, VARIANT *p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_get_document(IHTMLWindow2 *iface, IHTMLDocument2 **p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(This->doc) {
        /* FIXME: We should return a wrapper object here */
        *p = HTMLDOC(&This->doc->basedoc);
        IHTMLDocument2_AddRef(*p);
    }else {
        *p = NULL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_get_event(IHTMLWindow2 *iface, IHTMLEventObj **p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(This->event)
        IHTMLEventObj_AddRef(This->event);
    *p = This->event;
    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_get__newEnum(IHTMLWindow2 *iface, IUnknown **p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_showModalDialog(IHTMLWindow2 *iface, BSTR dialog,
        VARIANT *varArgIn, VARIANT *varOptions, VARIANT *varArgOut)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%s %p %p %p)\n", This, debugstr_w(dialog), varArgIn, varOptions, varArgOut);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_showHelp(IHTMLWindow2 *iface, BSTR helpURL, VARIANT helpArg,
        BSTR features)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%s v(%d) %s)\n", This, debugstr_w(helpURL), V_VT(&helpArg), debugstr_w(features));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_get_screen(IHTMLWindow2 *iface, IHTMLScreen **p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->screen) {
        HRESULT hres;

        hres = HTMLScreen_Create(&This->screen);
        if(FAILED(hres))
            return hres;
    }

    *p = This->screen;
    IHTMLScreen_AddRef(This->screen);
    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_get_Option(IHTMLWindow2 *iface, IHTMLOptionElementFactory **p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->option_factory)
        This->option_factory = HTMLOptionElementFactory_Create(This);

    *p = HTMLOPTFACTORY(This->option_factory);
    IHTMLOptionElementFactory_AddRef(*p);

    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_focus(IHTMLWindow2 *iface)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_get_closed(IHTMLWindow2 *iface, VARIANT_BOOL *p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_blur(IHTMLWindow2 *iface)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_scroll(IHTMLWindow2 *iface, LONG x, LONG y)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%d %d)\n", This, x, y);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_get_clientInformation(IHTMLWindow2 *iface, IOmNavigator **p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_setInterval(IHTMLWindow2 *iface, BSTR expression,
        LONG msec, VARIANT *language, LONG *timerID)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    VARIANT expr;

    TRACE("(%p)->(%s %d %p %p)\n", This, debugstr_w(expression), msec, language, timerID);

    V_VT(&expr) = VT_BSTR;
    V_BSTR(&expr) = expression;
    return IHTMLWindow3_setInterval(HTMLWINDOW3(This), &expr, msec, language, timerID);
}

static HRESULT WINAPI HTMLWindow2_clearInterval(IHTMLWindow2 *iface, LONG timerID)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);

    TRACE("(%p)->(%d)\n", This, timerID);

    return clear_task_timer(&This->doc->basedoc, TRUE, timerID);
}

static HRESULT WINAPI HTMLWindow2_put_offscreenBuffering(IHTMLWindow2 *iface, VARIANT v)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(v(%d))\n", This, V_VT(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_get_offscreenBuffering(IHTMLWindow2 *iface, VARIANT *p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_execScript(IHTMLWindow2 *iface, BSTR scode, BSTR language,
        VARIANT *pvarRet)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%s %s %p)\n", This, debugstr_w(scode), debugstr_w(language), pvarRet);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_toString(IHTMLWindow2 *iface, BSTR *String)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);

    static const WCHAR objectW[] = {'[','o','b','j','e','c','t',']',0};

    TRACE("(%p)->(%p)\n", This, String);

    if(!String)
        return E_INVALIDARG;

    *String = SysAllocString(objectW);
    return *String ? S_OK : E_OUTOFMEMORY;
}

static HRESULT WINAPI HTMLWindow2_scrollBy(IHTMLWindow2 *iface, LONG x, LONG y)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    nsresult nsres;

    TRACE("(%p)->(%d %d)\n", This, x, y);

    nsres = nsIDOMWindow_ScrollBy(This->nswindow, x, y);
    if(NS_FAILED(nsres))
        ERR("ScrollBy failed: %08x\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_scrollTo(IHTMLWindow2 *iface, LONG x, LONG y)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    nsresult nsres;

    TRACE("(%p)->(%d %d)\n", This, x, y);

    nsres = nsIDOMWindow_ScrollTo(This->nswindow, x, y);
    if(NS_FAILED(nsres))
        ERR("ScrollTo failed: %08x\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLWindow2_moveTo(IHTMLWindow2 *iface, LONG x, LONG y)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%d %d)\n", This, x, y);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_moveBy(IHTMLWindow2 *iface, LONG x, LONG y)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%d %d)\n", This, x, y);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_resizeTo(IHTMLWindow2 *iface, LONG x, LONG y)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%d %d)\n", This, x, y);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_resizeBy(IHTMLWindow2 *iface, LONG x, LONG y)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    FIXME("(%p)->(%d %d)\n", This, x, y);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow2_get_external(IHTMLWindow2 *iface, IDispatch **p)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = NULL;

    if(!This->doc_obj->hostui)
        return S_OK;

    return IDocHostUIHandler_GetExternal(This->doc_obj->hostui, p);
}

static HRESULT HTMLWindow_invoke(IUnknown *iface, DISPID id, LCID lcid, WORD flags, DISPPARAMS *params,
        VARIANT *res, EXCEPINFO *ei, IServiceProvider *caller)
{
    HTMLWindow *This = HTMLWINDOW2_THIS(iface);
    global_prop_t *prop;
    DWORD idx;
    HRESULT hres;

    idx = id - MSHTML_DISPID_CUSTOM_MIN;
    if(idx >= This->global_prop_cnt)
        return DISP_E_MEMBERNOTFOUND;

    prop = This->global_props+idx;

    switch(prop->type) {
    case GLOBAL_SCRIPTVAR: {
        IDispatchEx *dispex;
        IDispatch *disp;

        disp = get_script_disp(prop->script_host);
        if(!disp)
            return E_UNEXPECTED;

        hres = IDispatch_QueryInterface(disp, &IID_IDispatchEx, (void**)&dispex);
        if(SUCCEEDED(hres)) {
            TRACE("%s >>>\n", debugstr_w(prop->name));
            hres = IDispatchEx_InvokeEx(dispex, prop->id, lcid, flags, params, res, ei, caller);
            if(hres == S_OK)
                TRACE("%s <<<\n", debugstr_w(prop->name));
            else
                WARN("%s <<< %08x\n", debugstr_w(prop->name), hres);
            IDispatchEx_Release(dispex);
        }else {
            FIXME("No IDispatchEx\n");
        }
        IDispatch_Release(disp);
        break;
    }
    case GLOBAL_ELEMENTVAR: {
        IHTMLElement *elem;

        hres = IHTMLDocument3_getElementById(HTMLDOC3(&This->doc->basedoc), prop->name, &elem);
        if(FAILED(hres))
            return hres;

        if(!elem)
            return DISP_E_MEMBERNOTFOUND;

        V_VT(res) = VT_DISPATCH;
        V_DISPATCH(res) = (IDispatch*)elem;
        break;
    }
    default:
        ERR("invalid type %d\n", prop->type);
        hres = DISP_E_MEMBERNOTFOUND;
    }

    return hres;
}

#undef HTMLWINDOW2_THIS

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

#define HTMLWINDOW3_THIS(iface) DEFINE_THIS(HTMLWindow, HTMLWindow3, iface)

static HRESULT WINAPI HTMLWindow3_QueryInterface(IHTMLWindow3 *iface, REFIID riid, void **ppv)
{
    HTMLWindow *This = HTMLWINDOW3_THIS(iface);

    return IHTMLWindow2_QueryInterface(HTMLWINDOW2(This), riid, ppv);
}

static ULONG WINAPI HTMLWindow3_AddRef(IHTMLWindow3 *iface)
{
    HTMLWindow *This = HTMLWINDOW3_THIS(iface);

    return IHTMLWindow2_AddRef(HTMLWINDOW2(This));
}

static ULONG WINAPI HTMLWindow3_Release(IHTMLWindow3 *iface)
{
    HTMLWindow *This = HTMLWINDOW3_THIS(iface);

    return IHTMLWindow2_Release(HTMLWINDOW2(This));
}

static HRESULT WINAPI HTMLWindow3_GetTypeInfoCount(IHTMLWindow3 *iface, UINT *pctinfo)
{
    HTMLWindow *This = HTMLWINDOW3_THIS(iface);

    return IDispatchEx_GetTypeInfoCount(DISPATCHEX(This), pctinfo);
}

static HRESULT WINAPI HTMLWindow3_GetTypeInfo(IHTMLWindow3 *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLWindow *This = HTMLWINDOW3_THIS(iface);

    return IDispatchEx_GetTypeInfo(DISPATCHEX(This), iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLWindow3_GetIDsOfNames(IHTMLWindow3 *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLWindow *This = HTMLWINDOW3_THIS(iface);

    return IDispatchEx_GetIDsOfNames(DISPATCHEX(This), riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLWindow3_Invoke(IHTMLWindow3 *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLWindow *This = HTMLWINDOW3_THIS(iface);

    return IDispatchEx_Invoke(DISPATCHEX(This), dispIdMember, riid, lcid, wFlags, pDispParams,
            pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLWindow3_get_screenLeft(IHTMLWindow3 *iface, LONG *p)
{
    HTMLWindow *This = HTMLWINDOW3_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow3_get_screenTop(IHTMLWindow3 *iface, LONG *p)
{
    HTMLWindow *This = HTMLWINDOW3_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow3_attachEvent(IHTMLWindow3 *iface, BSTR event, IDispatch *pDisp, VARIANT_BOOL *pfResult)
{
    HTMLWindow *This = HTMLWINDOW3_THIS(iface);

    TRACE("(%p)->(%s %p %p)\n", This, debugstr_w(event), pDisp, pfResult);

    if(!This->doc) {
        FIXME("No document\n");
        return E_FAIL;
    }

    return attach_event(&This->doc->body_event_target, &This->doc->basedoc, event, pDisp, pfResult);
}

static HRESULT WINAPI HTMLWindow3_detachEvent(IHTMLWindow3 *iface, BSTR event, IDispatch *pDisp)
{
    HTMLWindow *This = HTMLWINDOW3_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT window_set_timer(HTMLWindow *This, VARIANT *expr, LONG msec, VARIANT *language,
        BOOL interval, LONG *timer_id)
{
    IDispatch *disp = NULL;

    switch(V_VT(expr)) {
    case VT_DISPATCH:
        disp = V_DISPATCH(expr);
        IDispatch_AddRef(disp);
        break;

    case VT_BSTR:
        disp = script_parse_event(This, V_BSTR(expr));
        break;

    default:
        FIXME("unimplemented vt=%d\n", V_VT(expr));
        return E_NOTIMPL;
    }

    if(!disp)
        return E_FAIL;

    *timer_id = set_task_timer(&This->doc->basedoc, msec, interval, disp);
    IDispatch_Release(disp);

    return S_OK;
}

static HRESULT WINAPI HTMLWindow3_setTimeout(IHTMLWindow3 *iface, VARIANT *expression, LONG msec,
        VARIANT *language, LONG *timerID)
{
    HTMLWindow *This = HTMLWINDOW3_THIS(iface);

    TRACE("(%p)->(%p(%d) %d %p %p)\n", This, expression, V_VT(expression), msec, language, timerID);

    return window_set_timer(This, expression, msec, language, FALSE, timerID);
}

static HRESULT WINAPI HTMLWindow3_setInterval(IHTMLWindow3 *iface, VARIANT *expression, LONG msec,
        VARIANT *language, LONG *timerID)
{
    HTMLWindow *This = HTMLWINDOW3_THIS(iface);

    TRACE("(%p)->(%p %d %p %p)\n", This, expression, msec, language, timerID);

    return window_set_timer(This, expression, msec, language, TRUE, timerID);
}

static HRESULT WINAPI HTMLWindow3_print(IHTMLWindow3 *iface)
{
    HTMLWindow *This = HTMLWINDOW3_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow3_put_onbeforeprint(IHTMLWindow3 *iface, VARIANT v)
{
    HTMLWindow *This = HTMLWINDOW3_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow3_get_onbeforeprint(IHTMLWindow3 *iface, VARIANT *p)
{
    HTMLWindow *This = HTMLWINDOW3_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow3_put_onafterprint(IHTMLWindow3 *iface, VARIANT v)
{
    HTMLWindow *This = HTMLWINDOW3_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow3_get_onafterprint(IHTMLWindow3 *iface, VARIANT *p)
{
    HTMLWindow *This = HTMLWINDOW3_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow3_get_clipboardData(IHTMLWindow3 *iface, IHTMLDataTransfer **p)
{
    HTMLWindow *This = HTMLWINDOW3_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow3_showModelessDialog(IHTMLWindow3 *iface, BSTR url,
        VARIANT *varArgIn, VARIANT *options, IHTMLWindow2 **pDialog)
{
    HTMLWindow *This = HTMLWINDOW3_THIS(iface);
    FIXME("(%p)->(%s %p %p %p)\n", This, debugstr_w(url), varArgIn, options, pDialog);
    return E_NOTIMPL;
}

#undef HTMLWINDOW3_THIS

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

#define HTMLWINDOW4_THIS(iface) DEFINE_THIS(HTMLWindow, HTMLWindow4, iface)

static HRESULT WINAPI HTMLWindow4_QueryInterface(IHTMLWindow4 *iface, REFIID riid, void **ppv)
{
    HTMLWindow *This = HTMLWINDOW4_THIS(iface);

    return IHTMLWindow2_QueryInterface(HTMLWINDOW2(This), riid, ppv);
}

static ULONG WINAPI HTMLWindow4_AddRef(IHTMLWindow4 *iface)
{
    HTMLWindow *This = HTMLWINDOW4_THIS(iface);

    return IHTMLWindow2_AddRef(HTMLWINDOW2(This));
}

static ULONG WINAPI HTMLWindow4_Release(IHTMLWindow4 *iface)
{
    HTMLWindow *This = HTMLWINDOW4_THIS(iface);

    return IHTMLWindow2_Release(HTMLWINDOW2(This));
}

static HRESULT WINAPI HTMLWindow4_GetTypeInfoCount(IHTMLWindow4 *iface, UINT *pctinfo)
{
    HTMLWindow *This = HTMLWINDOW4_THIS(iface);

    return IDispatchEx_GetTypeInfoCount(DISPATCHEX(This), pctinfo);
}

static HRESULT WINAPI HTMLWindow4_GetTypeInfo(IHTMLWindow4 *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLWindow *This = HTMLWINDOW4_THIS(iface);

    return IDispatchEx_GetTypeInfo(DISPATCHEX(This), iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLWindow4_GetIDsOfNames(IHTMLWindow4 *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLWindow *This = HTMLWINDOW4_THIS(iface);

    return IDispatchEx_GetIDsOfNames(DISPATCHEX(This), riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLWindow4_Invoke(IHTMLWindow4 *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLWindow *This = HTMLWINDOW4_THIS(iface);

    return IDispatchEx_Invoke(DISPATCHEX(This), dispIdMember, riid, lcid, wFlags, pDispParams,
            pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLWindow4_createPopup(IHTMLWindow4 *iface, VARIANT *varArgIn,
                            IDispatch **ppPopup)
{
    HTMLWindow *This = HTMLWINDOW4_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, varArgIn, ppPopup);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLWindow4_get_frameElement(IHTMLWindow4 *iface, IHTMLFrameBase **p)
{
    HTMLWindow *This = HTMLWINDOW4_THIS(iface);
    TRACE("(%p)->(%p)\n", This, p);

    if(This->frame_element) {
        *p = HTMLFRAMEBASE(This->frame_element);
        IHTMLFrameBase_AddRef(*p);
    }else
        *p = NULL;

    return S_OK;
}

#undef HTMLWINDOW4_THIS

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

#define DISPEX_THIS(iface) DEFINE_THIS(HTMLWindow, IDispatchEx, iface)

static HRESULT WINAPI WindowDispEx_QueryInterface(IDispatchEx *iface, REFIID riid, void **ppv)
{
    HTMLWindow *This = DISPEX_THIS(iface);

    return IHTMLWindow2_QueryInterface(HTMLWINDOW2(This), riid, ppv);
}

static ULONG WINAPI WindowDispEx_AddRef(IDispatchEx *iface)
{
    HTMLWindow *This = DISPEX_THIS(iface);

    return IHTMLWindow2_AddRef(HTMLWINDOW2(This));
}

static ULONG WINAPI WindowDispEx_Release(IDispatchEx *iface)
{
    HTMLWindow *This = DISPEX_THIS(iface);

    return IHTMLWindow2_Release(HTMLWINDOW2(This));
}

static HRESULT WINAPI WindowDispEx_GetTypeInfoCount(IDispatchEx *iface, UINT *pctinfo)
{
    HTMLWindow *This = DISPEX_THIS(iface);

    TRACE("(%p)->(%p)\n", This, pctinfo);

    return IDispatchEx_GetTypeInfoCount(DISPATCHEX(&This->dispex), pctinfo);
}

static HRESULT WINAPI WindowDispEx_GetTypeInfo(IDispatchEx *iface, UINT iTInfo,
                                               LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLWindow *This = DISPEX_THIS(iface);

    TRACE("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);

    return IDispatchEx_GetTypeInfo(DISPATCHEX(&This->dispex), iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI WindowDispEx_GetIDsOfNames(IDispatchEx *iface, REFIID riid,
                                                 LPOLESTR *rgszNames, UINT cNames,
                                                 LCID lcid, DISPID *rgDispId)
{
    HTMLWindow *This = DISPEX_THIS(iface);
    UINT i;
    HRESULT hres;

    WARN("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);

    for(i=0; i < cNames; i++) {
        /* We shouldn't use script's IDispatchEx here, so we shouldn't use GetDispID */
        hres = IDispatchEx_GetDispID(DISPATCHEX(This), rgszNames[i], 0, rgDispId+i);
        if(FAILED(hres))
            return hres;
    }

    return S_OK;
}

static HRESULT WINAPI WindowDispEx_Invoke(IDispatchEx *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLWindow *This = DISPEX_THIS(iface);

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    /* FIXME: Use script dispatch */

    return IDispatchEx_Invoke(DISPATCHEX(&This->dispex), dispIdMember, riid, lcid, wFlags, pDispParams,
                              pVarResult, pExcepInfo, puArgErr);
}

static global_prop_t *alloc_global_prop(HTMLWindow *This, global_prop_type_t type, BSTR name)
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

static inline DWORD prop_to_dispid(HTMLWindow *This, global_prop_t *prop)
{
    return MSHTML_DISPID_CUSTOM_MIN + (prop-This->global_props);
}

HRESULT search_window_props(HTMLWindow *This, BSTR bstrName, DWORD grfdex, DISPID *pid)
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

    if(find_global_prop(This, bstrName, grfdex, &script_host, &id)) {
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
    HTMLWindow *This = DISPEX_THIS(iface);
    HRESULT hres;

    TRACE("(%p)->(%s %x %p)\n", This, debugstr_w(bstrName), grfdex, pid);

    hres = search_window_props(This, bstrName, grfdex, pid);
    if(hres != DISP_E_UNKNOWNNAME)
        return hres;

    hres = IDispatchEx_GetDispID(DISPATCHEX(&This->dispex), bstrName, grfdex, pid);
    if(hres != DISP_E_UNKNOWNNAME)
        return hres;

    if(This->doc) {
        global_prop_t *prop;
        IHTMLElement *elem;

        hres = IHTMLDocument3_getElementById(HTMLDOC3(&This->doc->basedoc), bstrName, &elem);
        if(SUCCEEDED(hres) && elem) {
            IHTMLElement_Release(elem);

            prop = alloc_global_prop(This, GLOBAL_ELEMENTVAR, bstrName);
            if(!prop)
                return E_OUTOFMEMORY;

            *pid = prop_to_dispid(This, prop);
            return S_OK;
        }
    }

    return DISP_E_UNKNOWNNAME;
}

static HRESULT WINAPI WindowDispEx_InvokeEx(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    HTMLWindow *This = DISPEX_THIS(iface);

    TRACE("(%p)->(%x %x %x %p %p %p %p)\n", This, id, lcid, wFlags, pdp, pvarRes, pei, pspCaller);

    if(id == DISPID_IHTMLWINDOW2_LOCATION && (wFlags & DISPATCH_PROPERTYPUT)) {
        HTMLLocation *location;
        HRESULT hres;

        TRACE("forwarding to location.href\n");

        hres = get_location(This, &location);
        if(FAILED(hres))
            return hres;

        hres = IDispatchEx_InvokeEx(DISPATCHEX(&location->dispex), DISPID_VALUE, lcid, wFlags, pdp, pvarRes, pei, pspCaller);
        IHTMLLocation_Release(HTMLLOCATION(location));
        return hres;
    }

    return IDispatchEx_InvokeEx(DISPATCHEX(&This->dispex), id, lcid, wFlags, pdp, pvarRes, pei, pspCaller);
}

static HRESULT WINAPI WindowDispEx_DeleteMemberByName(IDispatchEx *iface, BSTR bstrName, DWORD grfdex)
{
    HTMLWindow *This = DISPEX_THIS(iface);

    TRACE("(%p)->(%s %x)\n", This, debugstr_w(bstrName), grfdex);

    return IDispatchEx_DeleteMemberByName(DISPATCHEX(&This->dispex), bstrName, grfdex);
}

static HRESULT WINAPI WindowDispEx_DeleteMemberByDispID(IDispatchEx *iface, DISPID id)
{
    HTMLWindow *This = DISPEX_THIS(iface);

    TRACE("(%p)->(%x)\n", This, id);

    return IDispatchEx_DeleteMemberByDispID(DISPATCHEX(&This->dispex), id);
}

static HRESULT WINAPI WindowDispEx_GetMemberProperties(IDispatchEx *iface, DISPID id, DWORD grfdexFetch, DWORD *pgrfdex)
{
    HTMLWindow *This = DISPEX_THIS(iface);

    TRACE("(%p)->(%x %x %p)\n", This, id, grfdexFetch, pgrfdex);

    return IDispatchEx_GetMemberProperties(DISPATCHEX(&This->dispex), id, grfdexFetch, pgrfdex);
}

static HRESULT WINAPI WindowDispEx_GetMemberName(IDispatchEx *iface, DISPID id, BSTR *pbstrName)
{
    HTMLWindow *This = DISPEX_THIS(iface);

    TRACE("(%p)->(%x %p)\n", This, id, pbstrName);

    return IDispatchEx_GetMemberName(DISPATCHEX(&This->dispex), id, pbstrName);
}

static HRESULT WINAPI WindowDispEx_GetNextDispID(IDispatchEx *iface, DWORD grfdex, DISPID id, DISPID *pid)
{
    HTMLWindow *This = DISPEX_THIS(iface);

    TRACE("(%p)->(%x %x %p)\n", This, grfdex, id, pid);

    return IDispatchEx_GetNextDispID(DISPATCHEX(&This->dispex), grfdex, id, pid);
}

static HRESULT WINAPI WindowDispEx_GetNameSpaceParent(IDispatchEx *iface, IUnknown **ppunk)
{
    HTMLWindow *This = DISPEX_THIS(iface);

    TRACE("(%p)->(%p)\n", This, ppunk);

    *ppunk = NULL;
    return S_OK;
}

#undef DISPEX_THIS

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

static const tid_t HTMLWindow_iface_tids[] = {
    IHTMLWindow2_tid,
    IHTMLWindow3_tid,
    IHTMLWindow4_tid,
    0
};

static const dispex_static_data_vtbl_t HTMLWindow_dispex_vtbl = {
    NULL,
    NULL,
    HTMLWindow_invoke
};

static dispex_static_data_t HTMLWindow_dispex = {
    &HTMLWindow_dispex_vtbl,
    DispHTMLWindow2_tid,
    NULL,
    HTMLWindow_iface_tids
};

HRESULT HTMLWindow_Create(HTMLDocumentObj *doc_obj, nsIDOMWindow *nswindow, HTMLWindow *parent, HTMLWindow **ret)
{
    HTMLWindow *window;

    window = heap_alloc_zero(sizeof(HTMLWindow));
    if(!window)
        return E_OUTOFMEMORY;

    window->window_ref = heap_alloc(sizeof(windowref_t));
    if(!window->window_ref) {
        heap_free(window);
        return E_OUTOFMEMORY;
    }

    window->lpHTMLWindow2Vtbl = &HTMLWindow2Vtbl;
    window->lpHTMLWindow3Vtbl = &HTMLWindow3Vtbl;
    window->lpHTMLWindow4Vtbl = &HTMLWindow4Vtbl;
    window->lpIDispatchExVtbl = &WindowDispExVtbl;
    window->ref = 1;
    window->doc_obj = doc_obj;

    window->window_ref->window = window;
    window->window_ref->ref = 1;

    init_dispex(&window->dispex, (IUnknown*)HTMLWINDOW2(window), &HTMLWindow_dispex);

    if(nswindow) {
        nsIDOMWindow_AddRef(nswindow);
        window->nswindow = nswindow;
    }

    window->scriptmode = parent ? parent->scriptmode : SCRIPTMODE_GECKO;
    window->readystate = READYSTATE_UNINITIALIZED;
    list_init(&window->script_hosts);

    window->task_magic = get_task_target_magic();
    update_window_doc(window);

    list_init(&window->children);
    list_add_head(&window_list, &window->entry);

    if(parent) {
        IHTMLWindow2_AddRef(HTMLWINDOW2(window));

        window->parent = parent;
        list_add_tail(&parent->children, &window->sibling_entry);
    }

    *ret = window;
    return S_OK;
}

void update_window_doc(HTMLWindow *window)
{
    nsIDOMHTMLDocument *nshtmldoc;
    nsIDOMDocument *nsdoc;
    nsresult nsres;

    nsres = nsIDOMWindow_GetDocument(window->nswindow, &nsdoc);
    if(NS_FAILED(nsres) || !nsdoc) {
        ERR("GetDocument failed: %08x\n", nsres);
        return;
    }

    nsres = nsIDOMDocument_QueryInterface(nsdoc, &IID_nsIDOMHTMLDocument, (void**)&nshtmldoc);
    nsIDOMDocument_Release(nsdoc);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIDOMHTMLDocument iface: %08x\n", nsres);
        return;
    }

    if(!window->doc || window->doc->nsdoc != nshtmldoc) {
        HTMLDocumentNode *doc;
        HRESULT hres;

        hres = create_doc_from_nsdoc(nshtmldoc, window->doc_obj, window, &doc);
        if(SUCCEEDED(hres)) {
            window_set_docnode(window, doc);
            htmldoc_release(&doc->basedoc);
        }else {
            ERR("create_doc_from_nsdoc failed: %08x\n", hres);
        }
    }

    nsIDOMHTMLDocument_Release(nshtmldoc);
}

HTMLWindow *nswindow_to_window(const nsIDOMWindow *nswindow)
{
    HTMLWindow *iter;

    LIST_FOR_EACH_ENTRY(iter, &window_list, HTMLWindow, entry) {
        if(iter->nswindow == nswindow)
            return iter;
    }

    return NULL;
}
