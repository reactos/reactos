/*
 * Copyright 2009 Andrew Eikum for CodeWeavers
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

#include "wine/debug.h"

#include "mshtml_private.h"
#include "htmlevent.h"
#include "binding.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

struct HTMLFormElement {
    HTMLElement element;

    IHTMLFormElement IHTMLFormElement_iface;

    nsIDOMHTMLFormElement *nsform;
};

typedef struct {
    IEnumVARIANT IEnumVARIANT_iface;

    LONG ref;

    ULONG iter;
    HTMLFormElement *elem;
} HTMLFormElementEnum;

HRESULT return_nsform(nsresult nsres, nsIDOMHTMLFormElement *form, IHTMLFormElement **p)
{
    nsIDOMNode *form_node;
    HTMLDOMNode *node;
    HRESULT hres;

    if (NS_FAILED(nsres)) {
        ERR("GetForm failed: %08lx\n", nsres);
        return E_FAIL;
    }

    if(!form) {
        *p = NULL;
        TRACE("NULL\n");
        return S_OK;
    }

    nsres = nsIDOMHTMLFormElement_QueryInterface(form, &IID_nsIDOMNode, (void**)&form_node);
    nsIDOMHTMLFormElement_Release(form);
    assert(nsres == NS_OK);

    hres = get_node(form_node, TRUE, &node);
    nsIDOMNode_Release(form_node);
    if (FAILED(hres))
        return hres;

    TRACE("node %p\n", node);
    hres = IHTMLDOMNode_QueryInterface(&node->IHTMLDOMNode_iface, &IID_IHTMLFormElement, (void**)p);
    node_release(node);
    return hres;
}

static HRESULT htmlform_item(HTMLFormElement *This, int i, IDispatch **ret)
{
    nsIDOMHTMLCollection *elements;
    nsIDOMNode *item;
    HTMLDOMNode *node;
    nsresult nsres;
    HRESULT hres;

    nsres = nsIDOMHTMLFormElement_GetElements(This->nsform, &elements);
    if(NS_FAILED(nsres)) {
        FIXME("GetElements failed: 0x%08lx\n", nsres);
        return E_FAIL;
    }

    nsres = nsIDOMHTMLCollection_Item(elements, i, &item);
    nsIDOMHTMLCollection_Release(elements);
    if(NS_FAILED(nsres)) {
        FIXME("Item failed: 0x%08lx\n", nsres);
        return E_FAIL;
    }

    if(item) {
        hres = get_node(item, TRUE, &node);
        if(FAILED(hres))
            return hres;

        nsIDOMNode_Release(item);
        *ret = (IDispatch*)&node->IHTMLDOMNode_iface;
    }else {
        *ret = NULL;
    }

    return S_OK;
}

static inline HTMLFormElementEnum *impl_from_IEnumVARIANT(IEnumVARIANT *iface)
{
    return CONTAINING_RECORD(iface, HTMLFormElementEnum, IEnumVARIANT_iface);
}

static HRESULT WINAPI HTMLFormElementEnum_QueryInterface(IEnumVARIANT *iface, REFIID riid, void **ppv)
{
    HTMLFormElementEnum *This = impl_from_IEnumVARIANT(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(riid, &IID_IUnknown)) {
        *ppv = &This->IEnumVARIANT_iface;
    }else if(IsEqualGUID(riid, &IID_IEnumVARIANT)) {
        *ppv = &This->IEnumVARIANT_iface;
    }else {
        FIXME("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI HTMLFormElementEnum_AddRef(IEnumVARIANT *iface)
{
    HTMLFormElementEnum *This = impl_from_IEnumVARIANT(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLFormElementEnum_Release(IEnumVARIANT *iface)
{
    HTMLFormElementEnum *This = impl_from_IEnumVARIANT(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref) {
        IHTMLFormElement_Release(&This->elem->IHTMLFormElement_iface);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI HTMLFormElementEnum_Next(IEnumVARIANT *iface, ULONG celt, VARIANT *rgVar, ULONG *pCeltFetched)
{
    HTMLFormElementEnum *This = impl_from_IEnumVARIANT(iface);
    nsresult nsres;
    HRESULT hres;
    ULONG num, i;
    LONG len;

    TRACE("(%p)->(%lu %p %p)\n", This, celt, rgVar, pCeltFetched);

    nsres = nsIDOMHTMLFormElement_GetLength(This->elem->nsform, &len);
    if(NS_FAILED(nsres))
        return E_FAIL;
    num = min(len - This->iter, celt);

    for(i = 0; i < num; i++) {
        hres = htmlform_item(This->elem, This->iter + i, &V_DISPATCH(&rgVar[i]));
        if(FAILED(hres)) {
            while(i--)
                VariantClear(&rgVar[i]);
            return hres;
        }
        V_VT(&rgVar[i]) = VT_DISPATCH;
    }

    This->iter += num;
    if(pCeltFetched)
        *pCeltFetched = num;
    return num == celt ? S_OK : S_FALSE;
}

static HRESULT WINAPI HTMLFormElementEnum_Skip(IEnumVARIANT *iface, ULONG celt)
{
    HTMLFormElementEnum *This = impl_from_IEnumVARIANT(iface);
    nsresult nsres;
    LONG len;

    TRACE("(%p)->(%lu)\n", This, celt);

    nsres = nsIDOMHTMLFormElement_GetLength(This->elem->nsform, &len);
    if(NS_FAILED(nsres))
        return E_FAIL;

    if(This->iter + celt > len) {
        This->iter = len;
        return S_FALSE;
    }

    This->iter += celt;
    return S_OK;
}

static HRESULT WINAPI HTMLFormElementEnum_Reset(IEnumVARIANT *iface)
{
    HTMLFormElementEnum *This = impl_from_IEnumVARIANT(iface);

    TRACE("(%p)->()\n", This);

    This->iter = 0;
    return S_OK;
}

static HRESULT WINAPI HTMLFormElementEnum_Clone(IEnumVARIANT *iface, IEnumVARIANT **ppEnum)
{
    HTMLFormElementEnum *This = impl_from_IEnumVARIANT(iface);
    FIXME("(%p)->(%p)\n", This, ppEnum);
    return E_NOTIMPL;
}

static const IEnumVARIANTVtbl HTMLFormElementEnumVtbl = {
    HTMLFormElementEnum_QueryInterface,
    HTMLFormElementEnum_AddRef,
    HTMLFormElementEnum_Release,
    HTMLFormElementEnum_Next,
    HTMLFormElementEnum_Skip,
    HTMLFormElementEnum_Reset,
    HTMLFormElementEnum_Clone
};

static inline HTMLFormElement *impl_from_IHTMLFormElement(IHTMLFormElement *iface)
{
    return CONTAINING_RECORD(iface, HTMLFormElement, IHTMLFormElement_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLFormElement, IHTMLFormElement,
                      impl_from_IHTMLFormElement(iface)->element.node.event_target.dispex)

static HRESULT WINAPI HTMLFormElement_put_action(IHTMLFormElement *iface, BSTR v)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);
    nsAString action_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, wine_dbgstr_w(v));

    nsAString_InitDepend(&action_str, v);
    nsres = nsIDOMHTMLFormElement_SetAction(This->nsform, &action_str);
    nsAString_Finish(&action_str);
    if(NS_FAILED(nsres)) {
        ERR("SetAction failed: %08lx\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLFormElement_get_action(IHTMLFormElement *iface, BSTR *p)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);
    nsAString action_str;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&action_str, NULL);
    nsres = nsIDOMHTMLFormElement_GetAction(This->nsform, &action_str);
    if(NS_SUCCEEDED(nsres)) {
        const PRUnichar *action;
        nsAString_GetData(&action_str, &action);
        hres = nsuri_to_url(action, FALSE, p);
    }else {
        ERR("GetAction failed: %08lx\n", nsres);
        hres = E_FAIL;
    }

    nsAString_Finish(&action_str);
    return hres;
}

static HRESULT WINAPI HTMLFormElement_put_dir(IHTMLFormElement *iface, BSTR v)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);
    FIXME("(%p)->(%s)\n", This, wine_dbgstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFormElement_get_dir(IHTMLFormElement *iface, BSTR *p)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFormElement_put_encoding(IHTMLFormElement *iface, BSTR v)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);
    nsAString encoding_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, wine_dbgstr_w(v));

    if(lstrcmpiW(v, L"application/x-www-form-urlencoded") && lstrcmpiW(v, L"multipart/form-data")
            && lstrcmpiW(v, L"text/plain")) {
        WARN("incorrect enctype\n");
        return E_INVALIDARG;
    }

    nsAString_InitDepend(&encoding_str, v);
    nsres = nsIDOMHTMLFormElement_SetEnctype(This->nsform, &encoding_str);
    nsAString_Finish(&encoding_str);
    if(NS_FAILED(nsres))
        return E_FAIL;

    return S_OK;
}

static HRESULT WINAPI HTMLFormElement_get_encoding(IHTMLFormElement *iface, BSTR *p)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);
    nsAString encoding_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&encoding_str, NULL);
    nsres = nsIDOMHTMLFormElement_GetEnctype(This->nsform, &encoding_str);
    return return_nsstr(nsres, &encoding_str, p);
}

static HRESULT WINAPI HTMLFormElement_put_method(IHTMLFormElement *iface, BSTR v)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);
    nsAString method_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, wine_dbgstr_w(v));

    if(lstrcmpiW(v, L"POST") && lstrcmpiW(v, L"GET")) {
        WARN("unrecognized method\n");
        return E_INVALIDARG;
    }

    nsAString_InitDepend(&method_str, v);
    nsres = nsIDOMHTMLFormElement_SetMethod(This->nsform, &method_str);
    nsAString_Finish(&method_str);
    if(NS_FAILED(nsres))
        return E_FAIL;

    return S_OK;
}

static HRESULT WINAPI HTMLFormElement_get_method(IHTMLFormElement *iface, BSTR *p)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);
    nsAString method_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&method_str, NULL);
    nsres = nsIDOMHTMLFormElement_GetMethod(This->nsform, &method_str);
    return return_nsstr(nsres, &method_str, p);
}

static HRESULT WINAPI HTMLFormElement_get_elements(IHTMLFormElement *iface, IDispatch **p)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);
    nsIDOMHTMLCollection *elements;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(dispex_compat_mode(&This->element.node.event_target.dispex) < COMPAT_MODE_IE9) {
        IDispatch_AddRef(*p = (IDispatch*)&This->IHTMLFormElement_iface);
        return S_OK;
    }

    nsres = nsIDOMHTMLFormElement_GetElements(This->nsform, &elements);
    if(NS_FAILED(nsres)) {
        ERR("GetElements failed: %08lx\n", nsres);
        return E_FAIL;
    }

    *p = (IDispatch*)create_collection_from_htmlcol(elements, &This->element.node.event_target.dispex);
    nsIDOMHTMLCollection_Release(elements);
    return S_OK;
}

static HRESULT WINAPI HTMLFormElement_put_target(IHTMLFormElement *iface, BSTR v)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);
    nsAString str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, wine_dbgstr_w(v));

    nsAString_InitDepend(&str, v);

    nsres = nsIDOMHTMLFormElement_SetTarget(This->nsform, &str);

    nsAString_Finish(&str);
    if (NS_FAILED(nsres)) {
        ERR("Set Target(%s) failed: %08lx\n", wine_dbgstr_w(v), nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLFormElement_get_target(IHTMLFormElement *iface, BSTR *p)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);
    nsAString str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&str, NULL);
    nsres = nsIDOMHTMLFormElement_GetTarget(This->nsform, &str);

    return return_nsstr(nsres, &str, p);
}

static HRESULT WINAPI HTMLFormElement_put_name(IHTMLFormElement *iface, BSTR v)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);
    nsAString name_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, wine_dbgstr_w(v));

    nsAString_InitDepend(&name_str, v);
    nsres = nsIDOMHTMLFormElement_SetName(This->nsform, &name_str);
    nsAString_Finish(&name_str);
    if(NS_FAILED(nsres))
        return E_FAIL;

    return S_OK;
}

static HRESULT WINAPI HTMLFormElement_get_name(IHTMLFormElement *iface, BSTR *p)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);
    nsAString name_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&name_str, NULL);
    nsres = nsIDOMHTMLFormElement_GetName(This->nsform, &name_str);
    return return_nsstr(nsres, &name_str, p);
}

static HRESULT WINAPI HTMLFormElement_put_onsubmit(IHTMLFormElement *iface, VARIANT v)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_node_event(&This->element.node, EVENTID_SUBMIT, &v);
}

static HRESULT WINAPI HTMLFormElement_get_onsubmit(IHTMLFormElement *iface, VARIANT *p)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_node_event(&This->element.node, EVENTID_SUBMIT, p);
}

static HRESULT WINAPI HTMLFormElement_put_onreset(IHTMLFormElement *iface, VARIANT v)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFormElement_get_onreset(IHTMLFormElement *iface, VARIANT *p)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFormElement_submit(IHTMLFormElement *iface)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);
    HTMLOuterWindow *window = NULL, *this_window = NULL;
    nsAString action_uri_str, target_str, method_str;
    nsIInputStream *post_stream;
    BOOL is_post_submit = FALSE;
    IUri *uri;
    nsresult nsres;
    HRESULT hres;
    BOOL use_new_window = FALSE;

    TRACE("(%p)\n", This);

    if(This->element.node.doc) {
        HTMLDocumentNode *doc = This->element.node.doc;
        if(doc->window && doc->window->base.outer_window)
            this_window = doc->window->base.outer_window;
    }
    if(!this_window) {
        TRACE("No outer window\n");
        return S_OK;
    }

    nsAString_Init(&target_str, NULL);
    nsres = nsIDOMHTMLFormElement_GetTarget(This->nsform, &target_str);
    if(NS_SUCCEEDED(nsres))
        window = get_target_window(this_window, &target_str, &use_new_window);

    if(!window && !use_new_window) {
        nsAString_Finish(&target_str);
        return S_OK;
    }

    nsAString_Init(&method_str, NULL);
    nsres = nsIDOMHTMLFormElement_GetMethod(This->nsform, &method_str);
    if(NS_SUCCEEDED(nsres)) {
        const PRUnichar *method;

        nsAString_GetData(&method_str, &method);
        TRACE("method is %s\n", debugstr_w(method));
        is_post_submit = !wcsicmp(method, L"post");
    }
    nsAString_Finish(&method_str);

    /*
     * FIXME: We currently use our submit implementation for POST submit. We should always use it.
     */
    if(window && !is_post_submit) {
        nsres = nsIDOMHTMLFormElement_Submit(This->nsform);
        nsAString_Finish(&target_str);
        IHTMLWindow2_Release(&window->base.IHTMLWindow2_iface);
        if(NS_FAILED(nsres)) {
            ERR("Submit failed: %08lx\n", nsres);
            return E_FAIL;
        }

        return S_OK;
    }

    nsAString_Init(&action_uri_str, NULL);
    nsres = nsIDOMHTMLFormElement_GetFormData(This->nsform, NULL, &action_uri_str, &post_stream);
    if(NS_SUCCEEDED(nsres)) {
        const PRUnichar *action_uri;

        nsAString_GetData(&action_uri_str, &action_uri);
        hres = create_uri(action_uri, 0, &uri);
    }else {
        ERR("GetFormData failed: %08lx\n", nsres);
        hres = E_FAIL;
    }
    nsAString_Finish(&action_uri_str);
    if(SUCCEEDED(hres)) {
        const PRUnichar *target;

        nsAString_GetData(&target_str, &target);
        hres = submit_form(window, target, uri, post_stream);
        IUri_Release(uri);
    }

    nsAString_Finish(&target_str);
    if(window)
        IHTMLWindow2_Release(&window->base.IHTMLWindow2_iface);
    if(post_stream)
        nsIInputStream_Release(post_stream);
    return hres;
}

static HRESULT WINAPI HTMLFormElement_reset(IHTMLFormElement *iface)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);
    nsresult nsres;

    TRACE("(%p)->()\n", This);
    nsres = nsIDOMHTMLFormElement_Reset(This->nsform);
    if (NS_FAILED(nsres)) {
        ERR("Reset failed: %08lx\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLFormElement_put_length(IHTMLFormElement *iface, LONG v)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);
    FIXME("(%p)->(%ld)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFormElement_get_length(IHTMLFormElement *iface, LONG *p)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLFormElement_GetLength(This->nsform, p);
    if(NS_FAILED(nsres)) {
        ERR("GetLength failed: %08lx\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLFormElement__newEnum(IHTMLFormElement *iface, IUnknown **p)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);
    HTMLFormElementEnum *ret;

    TRACE("(%p)->(%p)\n", This, p);

    ret = malloc(sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IEnumVARIANT_iface.lpVtbl = &HTMLFormElementEnumVtbl;
    ret->ref = 1;
    ret->iter = 0;

    HTMLFormElement_AddRef(&This->IHTMLFormElement_iface);
    ret->elem = This;

    *p = (IUnknown*)&ret->IEnumVARIANT_iface;
    return S_OK;
}

static HRESULT WINAPI HTMLFormElement_item(IHTMLFormElement *iface, VARIANT name,
        VARIANT index, IDispatch **pdisp)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);

    TRACE("(%p)->(%s %s %p)\n", This, debugstr_variant(&name), debugstr_variant(&index), pdisp);

    if(!pdisp)
        return E_INVALIDARG;
    *pdisp = NULL;

    if(V_VT(&name) == VT_I4) {
        if(V_I4(&name) < 0) {
            *pdisp = NULL;
            return dispex_compat_mode(&This->element.node.event_target.dispex) >= COMPAT_MODE_IE9
                ? S_OK : E_INVALIDARG;
        }
        return htmlform_item(This, V_I4(&name), pdisp);
    }

    FIXME("Unsupported args\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFormElement_tags(IHTMLFormElement *iface, VARIANT tagName,
        IDispatch **pdisp)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);
    FIXME("(%p)->(v %p)\n", This, pdisp);
    return E_NOTIMPL;
}

static const IHTMLFormElementVtbl HTMLFormElementVtbl = {
    HTMLFormElement_QueryInterface,
    HTMLFormElement_AddRef,
    HTMLFormElement_Release,
    HTMLFormElement_GetTypeInfoCount,
    HTMLFormElement_GetTypeInfo,
    HTMLFormElement_GetIDsOfNames,
    HTMLFormElement_Invoke,
    HTMLFormElement_put_action,
    HTMLFormElement_get_action,
    HTMLFormElement_put_dir,
    HTMLFormElement_get_dir,
    HTMLFormElement_put_encoding,
    HTMLFormElement_get_encoding,
    HTMLFormElement_put_method,
    HTMLFormElement_get_method,
    HTMLFormElement_get_elements,
    HTMLFormElement_put_target,
    HTMLFormElement_get_target,
    HTMLFormElement_put_name,
    HTMLFormElement_get_name,
    HTMLFormElement_put_onsubmit,
    HTMLFormElement_get_onsubmit,
    HTMLFormElement_put_onreset,
    HTMLFormElement_get_onreset,
    HTMLFormElement_submit,
    HTMLFormElement_reset,
    HTMLFormElement_put_length,
    HTMLFormElement_get_length,
    HTMLFormElement__newEnum,
    HTMLFormElement_item,
    HTMLFormElement_tags
};

static inline HTMLFormElement *impl_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, HTMLFormElement, element.node.event_target.dispex);
}

static void *HTMLFormElement_query_interface(DispatchEx *dispex, REFIID riid)
{
    HTMLFormElement *This = impl_from_DispatchEx(dispex);

    if(IsEqualGUID(&IID_IHTMLFormElement, riid))
        return &This->IHTMLFormElement_iface;
    if(IsEqualGUID(&DIID_DispHTMLFormElement, riid))
        return &This->IHTMLFormElement_iface;

    return HTMLElement_query_interface(&This->element.node.event_target.dispex, riid);
}

static void HTMLFormElement_traverse(DispatchEx *dispex, nsCycleCollectionTraversalCallback *cb)
{
    HTMLFormElement *This = impl_from_DispatchEx(dispex);
    HTMLElement_traverse(dispex, cb);

    if(This->nsform)
        note_cc_edge((nsISupports*)This->nsform, "nsform", cb);
}

static void HTMLFormElement_unlink(DispatchEx *dispex)
{
    HTMLFormElement *This = impl_from_DispatchEx(dispex);
    HTMLElement_unlink(dispex);
    unlink_ref(&This->nsform);
}

static HRESULT HTMLFormElement_get_dispid(DispatchEx *dispex, const WCHAR *name, DWORD grfdex, DISPID *dispid)
{
    HTMLFormElement *This = impl_from_DispatchEx(dispex);
    nsIDOMHTMLCollection *elements;
    nsAString nsstr, name_str;
    UINT32 len, i;
    nsresult nsres;
    HRESULT hres = DISP_E_UNKNOWNNAME;

    TRACE("(%p)->(%s %lx %p)\n", This, wine_dbgstr_w(name), grfdex, dispid);

    nsres = nsIDOMHTMLFormElement_GetElements(This->nsform, &elements);
    if(NS_FAILED(nsres)) {
        FIXME("GetElements failed: 0x%08lx\n", nsres);
        return E_FAIL;
    }

    nsres = nsIDOMHTMLCollection_GetLength(elements, &len);
    if(NS_FAILED(nsres)) {
        FIXME("GetLength failed: 0x%08lx\n", nsres);
        nsIDOMHTMLCollection_Release(elements);
        return E_FAIL;
    }

    if(len > MSHTML_CUSTOM_DISPID_CNT)
        len = MSHTML_CUSTOM_DISPID_CNT;

    /* FIXME: Implement in more generic way */
    if('0' <= *name && *name <= '9') {
        WCHAR *end_ptr;

        i = wcstoul(name, &end_ptr, 10);
        if(!*end_ptr && i < len) {
            *dispid = MSHTML_DISPID_CUSTOM_MIN + i;
            return S_OK;
        }
    }

    nsAString_Init(&nsstr, NULL);
    for(i = 0; i < len; ++i) {
        nsIDOMNode *nsitem;
        nsIDOMElement *elem;
        const PRUnichar *str;

        nsres = nsIDOMHTMLCollection_Item(elements, i, &nsitem);
        if(NS_FAILED(nsres)) {
            FIXME("Item failed: 0x%08lx\n", nsres);
            hres = E_FAIL;
            break;
        }

        nsres = nsIDOMNode_QueryInterface(nsitem, &IID_nsIDOMElement, (void**)&elem);
        nsIDOMNode_Release(nsitem);
        if(NS_FAILED(nsres)) {
            FIXME("Failed to get nsIDOMHTMLNode interface: 0x%08lx\n", nsres);
            hres = E_FAIL;
            break;
        }

        /* compare by id attr */
        nsres = nsIDOMElement_GetId(elem, &nsstr);
        if(NS_FAILED(nsres)) {
            FIXME("GetId failed: 0x%08lx\n", nsres);
            nsIDOMElement_Release(elem);
            hres = E_FAIL;
            break;
        }
        nsAString_GetData(&nsstr, &str);
        if(!wcsicmp(str, name)) {
            nsIDOMElement_Release(elem);
            /* FIXME: using index for dispid */
            *dispid = MSHTML_DISPID_CUSTOM_MIN + i;
            hres = S_OK;
            break;
        }

        /* compare by name attr */
        nsres = get_elem_attr_value(elem, L"name", &name_str, &str);
        nsIDOMElement_Release(elem);
        if(NS_SUCCEEDED(nsres)) {
            if(!wcsicmp(str, name)) {
                nsAString_Finish(&name_str);
                /* FIXME: using index for dispid */
                *dispid = MSHTML_DISPID_CUSTOM_MIN + i;
                hres = S_OK;
                break;
            }
            nsAString_Finish(&name_str);
        }
    }

    nsAString_Finish(&nsstr);
    nsIDOMHTMLCollection_Release(elements);
    return hres;
}

static HRESULT HTMLFormElement_invoke(DispatchEx *dispex, DISPID id, LCID lcid, WORD flags, DISPPARAMS *params,
        VARIANT *res, EXCEPINFO *ei, IServiceProvider *caller)
{
    HTMLFormElement *This = impl_from_DispatchEx(dispex);
    IDispatch *ret;
    HRESULT hres;

    TRACE("(%p)->(%lx %lx %x %p %p %p %p)\n", This, id, lcid, flags, params, res, ei, caller);

    hres = htmlform_item(This, id - MSHTML_DISPID_CUSTOM_MIN, &ret);
    if(FAILED(hres))
        return hres;

    if(ret) {
        V_VT(res) = VT_DISPATCH;
        V_DISPATCH(res) = ret;
    }else {
        V_VT(res) = VT_NULL;
    }
    return S_OK;
}

static HRESULT HTMLFormElement_handle_event(DispatchEx *dispex, DOMEvent *event, BOOL *prevent_default)
{
    HTMLFormElement *This = impl_from_DispatchEx(dispex);

    if(event->event_id == EVENTID_SUBMIT) {
        *prevent_default = TRUE;
        return IHTMLFormElement_submit(&This->IHTMLFormElement_iface);
    }

    return HTMLElement_handle_event(&This->element.node.event_target.dispex, event, prevent_default);
}

static const NodeImplVtbl HTMLFormElementImplVtbl = {
    .clsid                 = &CLSID_HTMLFormElement,
    .cpc_entries           = HTMLElement_cpc,
    .clone                 = HTMLElement_clone,
    .get_attr_col          = HTMLElement_get_attr_col,
};

static const event_target_vtbl_t HTMLFormElement_event_target_vtbl = {
    {
        HTMLELEMENT_DISPEX_VTBL_ENTRIES,
        .query_interface= HTMLFormElement_query_interface,
        .destructor     = HTMLElement_destructor,
        .traverse       = HTMLFormElement_traverse,
        .unlink         = HTMLFormElement_unlink,
        .get_dispid     = HTMLFormElement_get_dispid,
        .get_prop_desc  = dispex_index_prop_desc,
        .invoke         = HTMLFormElement_invoke
    },
    HTMLELEMENT_EVENT_TARGET_VTBL_ENTRIES,
    .handle_event       = HTMLFormElement_handle_event
};

static const tid_t HTMLFormElement_iface_tids[] = {
    IHTMLFormElement_tid,
    0
};

dispex_static_data_t HTMLFormElement_dispex = {
    .id           = PROT_HTMLFormElement,
    .prototype_id = PROT_HTMLElement,
    .vtbl         = &HTMLFormElement_event_target_vtbl.dispex_vtbl,
    .disp_tid     = DispHTMLFormElement_tid,
    .iface_tids   = HTMLFormElement_iface_tids,
    .init_info    = HTMLElement_init_dispex_info,
};

HRESULT HTMLFormElement_Create(HTMLDocumentNode *doc, nsIDOMElement *nselem, HTMLElement **elem)
{
    HTMLFormElement *ret;
    nsresult nsres;

    ret = calloc(1, sizeof(HTMLFormElement));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IHTMLFormElement_iface.lpVtbl = &HTMLFormElementVtbl;
    ret->element.node.vtbl = &HTMLFormElementImplVtbl;

    HTMLElement_Init(&ret->element, doc, nselem, &HTMLFormElement_dispex);

    nsres = nsIDOMElement_QueryInterface(nselem, &IID_nsIDOMHTMLFormElement, (void**)&ret->nsform);
    assert(nsres == NS_OK);

    *elem = &ret->element;
    return S_OK;
}
