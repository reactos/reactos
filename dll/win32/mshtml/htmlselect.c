/*
 * Copyright 2006,2007 Jacek Caban for CodeWeavers
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

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

struct HTMLOptionElement {
    HTMLElement element;

    IHTMLOptionElement IHTMLOptionElement_iface;

    nsIDOMHTMLOptionElement *nsoption;
};

static inline HTMLOptionElement *impl_from_IHTMLOptionElement(IHTMLOptionElement *iface)
{
    return CONTAINING_RECORD(iface, HTMLOptionElement, IHTMLOptionElement_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLOptionElement, IHTMLOptionElement,
                      impl_from_IHTMLOptionElement(iface)->element.node.event_target.dispex)

static HRESULT WINAPI HTMLOptionElement_put_selected(IHTMLOptionElement *iface, VARIANT_BOOL v)
{
    HTMLOptionElement *This = impl_from_IHTMLOptionElement(iface);
    nsresult nsres;

    TRACE("(%p)->(%x)\n", This, v);

    nsres = nsIDOMHTMLOptionElement_SetSelected(This->nsoption, v != VARIANT_FALSE);
    if(NS_FAILED(nsres)) {
        ERR("SetSelected failed: %08lx\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLOptionElement_get_selected(IHTMLOptionElement *iface, VARIANT_BOOL *p)
{
    HTMLOptionElement *This = impl_from_IHTMLOptionElement(iface);
    cpp_bool selected;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLOptionElement_GetSelected(This->nsoption, &selected);
    if(NS_FAILED(nsres)) {
        ERR("GetSelected failed: %08lx\n", nsres);
        return E_FAIL;
    }

    *p = variant_bool(selected);
    return S_OK;
}

static HRESULT WINAPI HTMLOptionElement_put_value(IHTMLOptionElement *iface, BSTR v)
{
    HTMLOptionElement *This = impl_from_IHTMLOptionElement(iface);
    nsAString value_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&value_str, v);
    nsres = nsIDOMHTMLOptionElement_SetValue(This->nsoption, &value_str);
    nsAString_Finish(&value_str);
    if(NS_FAILED(nsres))
        ERR("SetValue failed: %08lx\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLOptionElement_get_value(IHTMLOptionElement *iface, BSTR *p)
{
    HTMLOptionElement *This = impl_from_IHTMLOptionElement(iface);
    nsAString value_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&value_str, NULL);
    nsres = nsIDOMHTMLOptionElement_GetValue(This->nsoption, &value_str);
    return return_nsstr(nsres, &value_str, p);
}

static HRESULT WINAPI HTMLOptionElement_put_defaultSelected(IHTMLOptionElement *iface, VARIANT_BOOL v)
{
    HTMLOptionElement *This = impl_from_IHTMLOptionElement(iface);
    cpp_bool val, selected;
    nsresult nsres;

    TRACE("(%p)->(%x)\n", This, v);

    val = (v == VARIANT_TRUE);

    nsres = nsIDOMHTMLOptionElement_GetSelected(This->nsoption, &selected);
    if(NS_FAILED(nsres)) {
        ERR("GetSelected failed: %08lx\n", nsres);
        return E_FAIL;
    }

    nsres = nsIDOMHTMLOptionElement_SetDefaultSelected(This->nsoption, val);
    if(NS_FAILED(nsres)) {
        ERR("SetDefaultSelected failed: %08lx\n", nsres);
        return E_FAIL;
    }

    if(val != selected) {
        nsres = nsIDOMHTMLOptionElement_SetSelected(This->nsoption, selected); /* WinAPI will reserve selected property */
        if(NS_FAILED(nsres)) {
            ERR("SetSelected failed: %08lx\n", nsres);
            return E_FAIL;
        }
    }

    return S_OK;
}

static HRESULT WINAPI HTMLOptionElement_get_defaultSelected(IHTMLOptionElement *iface, VARIANT_BOOL *p)
{
    HTMLOptionElement *This = impl_from_IHTMLOptionElement(iface);
    cpp_bool val;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);
    if(!p)
        return E_POINTER;
    nsres = nsIDOMHTMLOptionElement_GetDefaultSelected(This->nsoption, &val);
    if(NS_FAILED(nsres)) {
        ERR("GetDefaultSelected failed: %08lx\n", nsres);
        return E_FAIL;
    }

    *p = variant_bool(val);
    return S_OK;
}

static HRESULT WINAPI HTMLOptionElement_put_index(IHTMLOptionElement *iface, LONG v)
{
    HTMLOptionElement *This = impl_from_IHTMLOptionElement(iface);
    FIXME("(%p)->(%ld)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLOptionElement_get_index(IHTMLOptionElement *iface, LONG *p)
{
    HTMLOptionElement *This = impl_from_IHTMLOptionElement(iface);
    LONG val;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_INVALIDARG;

    nsres = nsIDOMHTMLOptionElement_GetIndex(This->nsoption, &val);
    if(NS_FAILED(nsres)) {
        ERR("GetIndex failed: %08lx\n", nsres);
        return E_FAIL;
    }
    *p = val;
    return S_OK;
}

static HRESULT WINAPI HTMLOptionElement_put_text(IHTMLOptionElement *iface, BSTR v)
{
    HTMLOptionElement *This = impl_from_IHTMLOptionElement(iface);
    nsIDOMText *text_node;
    nsAString text_str;
    nsIDOMNode *tmp;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    if(!This->element.node.doc->dom_document) {
        WARN("NULL dom_document\n");
        return E_UNEXPECTED;
    }

    while(1) {
        nsIDOMNode *child;

        nsres = nsIDOMElement_GetFirstChild(This->element.dom_element, &child);
        if(NS_FAILED(nsres) || !child)
            break;

        nsres = nsIDOMElement_RemoveChild(This->element.dom_element, child, &tmp);
        nsIDOMNode_Release(child);
        if(NS_SUCCEEDED(nsres)) {
            nsIDOMNode_Release(tmp);
        }else {
            ERR("RemoveChild failed: %08lx\n", nsres);
            break;
        }
    }

    nsAString_InitDepend(&text_str, v);
    nsres = nsIDOMDocument_CreateTextNode(This->element.node.doc->dom_document, &text_str, &text_node);
    nsAString_Finish(&text_str);
    if(NS_FAILED(nsres)) {
        ERR("CreateTextNode failed: %08lx\n", nsres);
        return E_FAIL;
    }

    nsres = nsIDOMElement_AppendChild(This->element.dom_element, (nsIDOMNode*)text_node, &tmp);
    nsIDOMText_Release(text_node);
    if(NS_SUCCEEDED(nsres))
        nsIDOMNode_Release(tmp);
    else
        ERR("AppendChild failed: %08lx\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLOptionElement_get_text(IHTMLOptionElement *iface, BSTR *p)
{
    HTMLOptionElement *This = impl_from_IHTMLOptionElement(iface);
    nsAString text_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&text_str, NULL);
    nsres = nsIDOMHTMLOptionElement_GetText(This->nsoption, &text_str);
    return return_nsstr(nsres, &text_str, p);
}

static HRESULT WINAPI HTMLOptionElement_get_form(IHTMLOptionElement *iface, IHTMLFormElement **p)
{
    HTMLOptionElement *This = impl_from_IHTMLOptionElement(iface);
    nsIDOMHTMLFormElement *nsform;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_POINTER;

    nsres = nsIDOMHTMLOptionElement_GetForm(This->nsoption, &nsform);
    return return_nsform(nsres, nsform, p);
}

static const IHTMLOptionElementVtbl HTMLOptionElementVtbl = {
    HTMLOptionElement_QueryInterface,
    HTMLOptionElement_AddRef,
    HTMLOptionElement_Release,
    HTMLOptionElement_GetTypeInfoCount,
    HTMLOptionElement_GetTypeInfo,
    HTMLOptionElement_GetIDsOfNames,
    HTMLOptionElement_Invoke,
    HTMLOptionElement_put_selected,
    HTMLOptionElement_get_selected,
    HTMLOptionElement_put_value,
    HTMLOptionElement_get_value,
    HTMLOptionElement_put_defaultSelected,
    HTMLOptionElement_get_defaultSelected,
    HTMLOptionElement_put_index,
    HTMLOptionElement_get_index,
    HTMLOptionElement_put_text,
    HTMLOptionElement_get_text,
    HTMLOptionElement_get_form
};

static inline HTMLOptionElement *HTMLOptionElement_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, HTMLOptionElement, element.node.event_target.dispex);
}

static void *HTMLOptionElement_query_interface(DispatchEx *dispex, REFIID riid)
{
    HTMLOptionElement *This = HTMLOptionElement_from_DispatchEx(dispex);

    if(IsEqualGUID(&IID_IHTMLOptionElement, riid))
        return &This->IHTMLOptionElement_iface;

    return HTMLElement_query_interface(&This->element.node.event_target.dispex, riid);
}

static void HTMLOptionElement_traverse(DispatchEx *dispex, nsCycleCollectionTraversalCallback *cb)
{
    HTMLOptionElement *This = HTMLOptionElement_from_DispatchEx(dispex);
    HTMLElement_traverse(dispex, cb);

    if(This->nsoption)
        note_cc_edge((nsISupports*)This->nsoption, "nsoption", cb);
}

static void HTMLOptionElement_unlink(DispatchEx *dispex)
{
    HTMLOptionElement *This = HTMLOptionElement_from_DispatchEx(dispex);
    HTMLElement_unlink(dispex);
    unlink_ref(&This->nsoption);
}

static const NodeImplVtbl HTMLOptionElementImplVtbl = {
    .clsid                 = &CLSID_HTMLOptionElement,
    .cpc_entries           = HTMLElement_cpc,
    .clone                 = HTMLElement_clone,
    .get_attr_col          = HTMLElement_get_attr_col,
};

static const event_target_vtbl_t HTMLOptionElement_event_target_vtbl = {
    {
        HTMLELEMENT_DISPEX_VTBL_ENTRIES,
        .query_interface= HTMLOptionElement_query_interface,
        .destructor     = HTMLElement_destructor,
        .traverse       = HTMLOptionElement_traverse,
        .unlink         = HTMLOptionElement_unlink
    },
    HTMLELEMENT_EVENT_TARGET_VTBL_ENTRIES,
    .handle_event       = HTMLElement_handle_event
};

static const tid_t HTMLOptionElement_iface_tids[] = {
    IHTMLOptionElement_tid,
    0
};
dispex_static_data_t HTMLOptionElement_dispex = {
    .id           = PROT_HTMLOptionElement,
    .prototype_id = PROT_HTMLElement,
    .vtbl         = &HTMLOptionElement_event_target_vtbl.dispex_vtbl,
    .disp_tid     = DispHTMLOptionElement_tid,
    .iface_tids   = HTMLOptionElement_iface_tids,
    .init_info    = HTMLElement_init_dispex_info,
};

HRESULT HTMLOptionElement_Create(HTMLDocumentNode *doc, nsIDOMElement *nselem, HTMLElement **elem)
{
    HTMLOptionElement *ret;
    nsresult nsres;

    ret = calloc(1, sizeof(HTMLOptionElement));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IHTMLOptionElement_iface.lpVtbl = &HTMLOptionElementVtbl;
    ret->element.node.vtbl = &HTMLOptionElementImplVtbl;

    HTMLElement_Init(&ret->element, doc, nselem, &HTMLOptionElement_dispex);

    nsres = nsIDOMElement_QueryInterface(nselem, &IID_nsIDOMHTMLOptionElement, (void**)&ret->nsoption);
    assert(nsres == NS_OK);

    *elem = &ret->element;
    return S_OK;
}

static inline HTMLOptionElementFactory *impl_from_IHTMLOptionElementFactory(IHTMLOptionElementFactory *iface)
{
    return CONTAINING_RECORD(iface, HTMLOptionElementFactory, IHTMLOptionElementFactory_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLOptionElementFactory, IHTMLOptionElementFactory,
                      impl_from_IHTMLOptionElementFactory(iface)->dispex)

static HRESULT WINAPI HTMLOptionElementFactory_create(IHTMLOptionElementFactory *iface,
        VARIANT text, VARIANT value, VARIANT defaultselected, VARIANT selected,
        IHTMLOptionElement **optelem)
{
    HTMLOptionElementFactory *This = impl_from_IHTMLOptionElementFactory(iface);
    nsIDOMElement *nselem;
    HTMLDOMNode *node;
    HRESULT hres;

    TRACE("(%p)->(%s %s %s %s %p)\n", This, debugstr_variant(&text), debugstr_variant(&value),
          debugstr_variant(&defaultselected), debugstr_variant(&selected), optelem);

    *optelem = NULL;

    hres = create_nselem(This->window->doc, L"OPTION", &nselem);
    if(FAILED(hres))
        return hres;

    hres = get_node((nsIDOMNode*)nselem, TRUE, &node);
    nsIDOMElement_Release(nselem);
    if(FAILED(hres))
        return hres;

    hres = IHTMLDOMNode_QueryInterface(&node->IHTMLDOMNode_iface,
            &IID_IHTMLOptionElement, (void**)optelem);
    node_release(node);

    if(V_VT(&text) == VT_BSTR)
        IHTMLOptionElement_put_text(*optelem, V_BSTR(&text));
    else if(V_VT(&text) != VT_EMPTY)
        FIXME("Unsupported text %s\n", debugstr_variant(&text));

    if(V_VT(&value) == VT_BSTR)
        IHTMLOptionElement_put_value(*optelem, V_BSTR(&value));
    else if(V_VT(&value) != VT_EMPTY)
        FIXME("Unsupported value %s\n", debugstr_variant(&value));

    if(V_VT(&defaultselected) != VT_EMPTY)
        FIXME("Unsupported defaultselected %s\n", debugstr_variant(&defaultselected));
    if(V_VT(&selected) != VT_EMPTY)
        FIXME("Unsupported selected %s\n", debugstr_variant(&selected));

    return S_OK;
}

static const IHTMLOptionElementFactoryVtbl HTMLOptionElementFactoryVtbl = {
    HTMLOptionElementFactory_QueryInterface,
    HTMLOptionElementFactory_AddRef,
    HTMLOptionElementFactory_Release,
    HTMLOptionElementFactory_GetTypeInfoCount,
    HTMLOptionElementFactory_GetTypeInfo,
    HTMLOptionElementFactory_GetIDsOfNames,
    HTMLOptionElementFactory_Invoke,
    HTMLOptionElementFactory_create
};

static inline HTMLOptionElementFactory *HTMLOptionElementFactory_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, HTMLOptionElementFactory, dispex);
}

static void *HTMLOptionElementFactory_query_interface(DispatchEx *dispex, REFIID riid)
{
    HTMLOptionElementFactory *This = HTMLOptionElementFactory_from_DispatchEx(dispex);

    if(IsEqualGUID(&IID_IHTMLOptionElementFactory, riid))
        return &This->IHTMLOptionElementFactory_iface;

    return NULL;
}

static void HTMLOptionElementFactory_traverse(DispatchEx *dispex, nsCycleCollectionTraversalCallback *cb)
{
    HTMLOptionElementFactory *This = HTMLOptionElementFactory_from_DispatchEx(dispex);

    if(This->window)
        note_cc_edge((nsISupports*)&This->window->base.IHTMLWindow2_iface, "window", cb);
}

static void HTMLOptionElementFactory_unlink(DispatchEx *dispex)
{
    HTMLOptionElementFactory *This = HTMLOptionElementFactory_from_DispatchEx(dispex);

    if(This->window) {
        HTMLInnerWindow *window = This->window;
        This->window = NULL;
        IHTMLWindow2_Release(&window->base.IHTMLWindow2_iface);
    }
}

static void HTMLOptionElementFactory_destructor(DispatchEx *dispex)
{
    HTMLOptionElementFactory *This = HTMLOptionElementFactory_from_DispatchEx(dispex);
    free(This);
}

static HRESULT HTMLOptionElementFactory_value(DispatchEx *dispex, LCID lcid,
        WORD flags, DISPPARAMS *params, VARIANT *res, EXCEPINFO *ei,
        IServiceProvider *caller)
{
    HTMLOptionElementFactory *This = HTMLOptionElementFactory_from_DispatchEx(dispex);
    unsigned int i, argc = params->cArgs - params->cNamedArgs;
    IHTMLOptionElement *opt;
    VARIANT empty, *arg[4];
    HRESULT hres;

    if(flags != DISPATCH_CONSTRUCT) {
        FIXME("flags %x not supported\n", flags);
        return E_NOTIMPL;
    }

    V_VT(res) = VT_NULL;
    V_VT(&empty) = VT_EMPTY;

    for(i = 0; i < ARRAY_SIZE(arg); i++)
        arg[i] = argc > i ? &params->rgvarg[params->cArgs - 1 - i] : &empty;

    hres = IHTMLOptionElementFactory_create(&This->IHTMLOptionElementFactory_iface,
                                            *arg[0], *arg[1], *arg[2], *arg[3], &opt);
    if(FAILED(hres))
        return hres;

    V_VT(res) = VT_DISPATCH;
    V_DISPATCH(res) = (IDispatch*)opt;

    return S_OK;
}

static const tid_t HTMLOptionElementFactory_iface_tids[] = {
    IHTMLOptionElementFactory_tid,
    0
};

static const dispex_static_data_vtbl_t HTMLOptionElementFactory_dispex_vtbl = {
    .query_interface  = HTMLOptionElementFactory_query_interface,
    .destructor       = HTMLOptionElementFactory_destructor,
    .traverse         = HTMLOptionElementFactory_traverse,
    .unlink           = HTMLOptionElementFactory_unlink,
    .value            = HTMLOptionElementFactory_value,
};

static dispex_static_data_t HTMLOptionElementFactory_dispex = {
    .name           = "Function",
    .constructor_id = PROT_HTMLOptionElement,
    .vtbl           = &HTMLOptionElementFactory_dispex_vtbl,
    .disp_tid       = IHTMLOptionElementFactory_tid,
    .iface_tids     = HTMLOptionElementFactory_iface_tids,
};

HRESULT HTMLOptionElementFactory_Create(HTMLInnerWindow *window, HTMLOptionElementFactory **ret_ptr)
{
    HTMLOptionElementFactory *ret;

    ret = malloc(sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IHTMLOptionElementFactory_iface.lpVtbl = &HTMLOptionElementFactoryVtbl;
    ret->window = window;
    IHTMLWindow2_AddRef(&window->base.IHTMLWindow2_iface);

    init_dispatch(&ret->dispex, &HTMLOptionElementFactory_dispex, window,
                  dispex_compat_mode(&window->event_target.dispex));

    *ret_ptr = ret;
    return S_OK;
}

struct HTMLSelectElement {
    HTMLElement element;

    IHTMLSelectElement IHTMLSelectElement_iface;

    nsIDOMHTMLSelectElement *nsselect;
};

typedef struct {
    IEnumVARIANT IEnumVARIANT_iface;

    LONG ref;

    ULONG iter;
    HTMLSelectElement *elem;
} HTMLSelectElementEnum;

static inline HTMLSelectElement *impl_from_IHTMLSelectElement(IHTMLSelectElement *iface)
{
    return CONTAINING_RECORD(iface, HTMLSelectElement, IHTMLSelectElement_iface);
}

static HRESULT htmlselect_item(HTMLSelectElement *This, int i, IDispatch **ret)
{
    nsIDOMHTMLOptionsCollection *nscol;
    nsIDOMNode *nsnode;
    nsresult nsres;
    HRESULT hres;

    nsres = nsIDOMHTMLSelectElement_GetOptions(This->nsselect, &nscol);
    if(NS_FAILED(nsres)) {
        ERR("GetOptions failed: %08lx\n", nsres);
        return E_FAIL;
    }

    nsres = nsIDOMHTMLOptionsCollection_Item(nscol, i, &nsnode);
    nsIDOMHTMLOptionsCollection_Release(nscol);
    if(NS_FAILED(nsres)) {
        ERR("Item failed: %08lx\n", nsres);
        return E_FAIL;
    }

    if(nsnode) {
        HTMLDOMNode *node;

        hres = get_node(nsnode, TRUE, &node);
        nsIDOMNode_Release(nsnode);
        if(FAILED(hres))
            return hres;

        *ret = (IDispatch*)&node->IHTMLDOMNode_iface;
    }else {
        *ret = NULL;
    }
    return S_OK;
}

static inline HTMLSelectElementEnum *impl_from_IEnumVARIANT(IEnumVARIANT *iface)
{
    return CONTAINING_RECORD(iface, HTMLSelectElementEnum, IEnumVARIANT_iface);
}

static HRESULT WINAPI HTMLSelectElementEnum_QueryInterface(IEnumVARIANT *iface, REFIID riid, void **ppv)
{
    HTMLSelectElementEnum *This = impl_from_IEnumVARIANT(iface);

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

static ULONG WINAPI HTMLSelectElementEnum_AddRef(IEnumVARIANT *iface)
{
    HTMLSelectElementEnum *This = impl_from_IEnumVARIANT(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLSelectElementEnum_Release(IEnumVARIANT *iface)
{
    HTMLSelectElementEnum *This = impl_from_IEnumVARIANT(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref) {
        IHTMLSelectElement_Release(&This->elem->IHTMLSelectElement_iface);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI HTMLSelectElementEnum_Next(IEnumVARIANT *iface, ULONG celt, VARIANT *rgVar, ULONG *pCeltFetched)
{
    HTMLSelectElementEnum *This = impl_from_IEnumVARIANT(iface);
    nsresult nsres;
    HRESULT hres;
    ULONG num, i;
    UINT32 len;

    TRACE("(%p)->(%lu %p %p)\n", This, celt, rgVar, pCeltFetched);

    nsres = nsIDOMHTMLSelectElement_GetLength(This->elem->nsselect, &len);
    if(NS_FAILED(nsres))
        return E_FAIL;
    num = min(len - This->iter, celt);

    for(i = 0; i < num; i++) {
        hres = htmlselect_item(This->elem, This->iter + i, &V_DISPATCH(&rgVar[i]));
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

static HRESULT WINAPI HTMLSelectElementEnum_Skip(IEnumVARIANT *iface, ULONG celt)
{
    HTMLSelectElementEnum *This = impl_from_IEnumVARIANT(iface);
    nsresult nsres;
    UINT32 len;

    TRACE("(%p)->(%lu)\n", This, celt);

    nsres = nsIDOMHTMLSelectElement_GetLength(This->elem->nsselect, &len);
    if(NS_FAILED(nsres))
        return E_FAIL;

    if(This->iter + celt > len) {
        This->iter = len;
        return S_FALSE;
    }

    This->iter += celt;
    return S_OK;
}

static HRESULT WINAPI HTMLSelectElementEnum_Reset(IEnumVARIANT *iface)
{
    HTMLSelectElementEnum *This = impl_from_IEnumVARIANT(iface);

    TRACE("(%p)->()\n", This);

    This->iter = 0;
    return S_OK;
}

static HRESULT WINAPI HTMLSelectElementEnum_Clone(IEnumVARIANT *iface, IEnumVARIANT **ppEnum)
{
    HTMLSelectElementEnum *This = impl_from_IEnumVARIANT(iface);
    FIXME("(%p)->(%p)\n", This, ppEnum);
    return E_NOTIMPL;
}

static const IEnumVARIANTVtbl HTMLSelectElementEnumVtbl = {
    HTMLSelectElementEnum_QueryInterface,
    HTMLSelectElementEnum_AddRef,
    HTMLSelectElementEnum_Release,
    HTMLSelectElementEnum_Next,
    HTMLSelectElementEnum_Skip,
    HTMLSelectElementEnum_Reset,
    HTMLSelectElementEnum_Clone
};

DISPEX_IDISPATCH_IMPL(HTMLSelectElement, IHTMLSelectElement,
                      impl_from_IHTMLSelectElement(iface)->element.node.event_target.dispex)

static HRESULT WINAPI HTMLSelectElement_put_size(IHTMLSelectElement *iface, LONG v)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    nsresult nsres;

    TRACE("(%p)->(%ld)\n", This, v);
    if(v < 0)
        return CTL_E_INVALIDPROPERTYVALUE;

    nsres = nsIDOMHTMLSelectElement_SetSize(This->nsselect, v);
    if(NS_FAILED(nsres)) {
        ERR("SetSize failed: %08lx\n", nsres);
        return E_FAIL;
    }
    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_get_size(IHTMLSelectElement *iface, LONG *p)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    UINT32 val;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);
    if(!p)
        return E_INVALIDARG;

    nsres = nsIDOMHTMLSelectElement_GetSize(This->nsselect, &val);
    if(NS_FAILED(nsres)) {
        ERR("GetSize failed: %08lx\n", nsres);
        return E_FAIL;
    }
    *p = val;
    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_put_multiple(IHTMLSelectElement *iface, VARIANT_BOOL v)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    nsresult nsres;

    TRACE("(%p)->(%x)\n", This, v);

    nsres = nsIDOMHTMLSelectElement_SetMultiple(This->nsselect, !!v);
    assert(nsres == NS_OK);
    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_get_multiple(IHTMLSelectElement *iface, VARIANT_BOOL *p)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    cpp_bool val;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLSelectElement_GetMultiple(This->nsselect, &val);
    assert(nsres == NS_OK);

    *p = variant_bool(val);
    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_put_name(IHTMLSelectElement *iface, BSTR v)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    nsAString str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    nsAString_InitDepend(&str, v);
    nsres = nsIDOMHTMLSelectElement_SetName(This->nsselect, &str);
    nsAString_Finish(&str);

    if(NS_FAILED(nsres)) {
        ERR("SetName failed: %08lx\n", nsres);
        return E_FAIL;
    }
    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_get_name(IHTMLSelectElement *iface, BSTR *p)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    nsAString name_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&name_str, NULL);
    nsres = nsIDOMHTMLSelectElement_GetName(This->nsselect, &name_str);

    return return_nsstr(nsres, &name_str, p);
}

static HRESULT WINAPI HTMLSelectElement_get_options(IHTMLSelectElement *iface, IDispatch **p)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = (IDispatch*)&This->IHTMLSelectElement_iface;
    IDispatch_AddRef(*p);
    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_put_onchange(IHTMLSelectElement *iface, VARIANT v)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);

    TRACE("(%p)->()\n", This);

    return set_node_event(&This->element.node, EVENTID_CHANGE, &v);
}

static HRESULT WINAPI HTMLSelectElement_get_onchange(IHTMLSelectElement *iface, VARIANT *p)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLSelectElement_put_selectedIndex(IHTMLSelectElement *iface, LONG v)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    nsresult nsres;

    TRACE("(%p)->(%ld)\n", This, v);

    nsres = nsIDOMHTMLSelectElement_SetSelectedIndex(This->nsselect, v);
    if(NS_FAILED(nsres))
        ERR("SetSelectedIndex failed: %08lx\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_get_selectedIndex(IHTMLSelectElement *iface, LONG *p)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLSelectElement_GetSelectedIndex(This->nsselect, p);
    if(NS_FAILED(nsres)) {
        ERR("GetSelectedIndex failed: %08lx\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_get_type(IHTMLSelectElement *iface, BSTR *p)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    nsAString type_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&type_str, NULL);
    nsres = nsIDOMHTMLSelectElement_GetType(This->nsselect, &type_str);
    return return_nsstr(nsres, &type_str, p);
}

static HRESULT WINAPI HTMLSelectElement_put_value(IHTMLSelectElement *iface, BSTR v)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    nsAString value_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&value_str, v);
    nsres = nsIDOMHTMLSelectElement_SetValue(This->nsselect, &value_str);
    nsAString_Finish(&value_str);
    if(NS_FAILED(nsres))
        ERR("SetValue failed: %08lx\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_get_value(IHTMLSelectElement *iface, BSTR *p)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    nsAString value_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&value_str, NULL);
    nsres = nsIDOMHTMLSelectElement_GetValue(This->nsselect, &value_str);
    return return_nsstr(nsres, &value_str, p);
}

static HRESULT WINAPI HTMLSelectElement_put_disabled(IHTMLSelectElement *iface, VARIANT_BOOL v)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    nsresult nsres;

    TRACE("(%p)->(%x)\n", This, v);

    nsres = nsIDOMHTMLSelectElement_SetDisabled(This->nsselect, v != VARIANT_FALSE);
    if(NS_FAILED(nsres)) {
        ERR("SetDisabled failed: %08lx\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_get_disabled(IHTMLSelectElement *iface, VARIANT_BOOL *p)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    cpp_bool disabled = FALSE;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLSelectElement_GetDisabled(This->nsselect, &disabled);
    if(NS_FAILED(nsres)) {
        ERR("GetDisabled failed: %08lx\n", nsres);
        return E_FAIL;
    }

    *p = variant_bool(disabled);
    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_get_form(IHTMLSelectElement *iface, IHTMLFormElement **p)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    nsIDOMHTMLFormElement *nsform;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_POINTER;

    nsres = nsIDOMHTMLSelectElement_GetForm(This->nsselect, &nsform);
    return return_nsform(nsres, nsform, p);
}

static HRESULT WINAPI HTMLSelectElement_add(IHTMLSelectElement *iface, IHTMLElement *element,
                                            VARIANT before)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    nsIWritableVariant *nsvariant;
    HTMLElement *element_obj;
    nsresult nsres;

    TRACE("(%p)->(%p %s)\n", This, element, debugstr_variant(&before));

    element_obj = unsafe_impl_from_IHTMLElement(element);
    if(!element_obj) {
        FIXME("External IHTMLElement implementation?\n");
        return E_INVALIDARG;
    }

    if(!element_obj->html_element) {
        FIXME("Not HTML element\n");
        return E_NOTIMPL;
    }

    nsvariant = create_nsvariant();
    if(!nsvariant)
        return E_FAIL;

    switch(V_VT(&before)) {
    case VT_EMPTY:
    case VT_ERROR:
        nsres = nsIWritableVariant_SetAsEmpty(nsvariant);
        break;
    case VT_I2:
        nsres = nsIWritableVariant_SetAsInt16(nsvariant, V_I2(&before));
        break;
    default:
        FIXME("unhandled before %s\n", debugstr_variant(&before));
        nsIWritableVariant_Release(nsvariant);
        return E_NOTIMPL;
    }

    if(NS_SUCCEEDED(nsres))
        nsres = nsIDOMHTMLSelectElement_Add(This->nsselect, element_obj->html_element, (nsIVariant*)nsvariant);
    nsIWritableVariant_Release(nsvariant);
    if(NS_FAILED(nsres)) {
        ERR("Add failed: %08lx\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_remove(IHTMLSelectElement *iface, LONG index)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    nsresult nsres;
    TRACE("(%p)->(%ld)\n", This, index);
    if(index < 0)
        return E_INVALIDARG;

    nsres = nsIDOMHTMLSelectElement_select_Remove(This->nsselect, index);
    if(NS_FAILED(nsres)) {
        ERR("Remove failed: %08lx\n", nsres);
        return E_FAIL;
    }
    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_put_length(IHTMLSelectElement *iface, LONG v)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    nsresult nsres;

    TRACE("(%p)->(%ld)\n", This, v);

    nsres = nsIDOMHTMLSelectElement_SetLength(This->nsselect, v);
    if(NS_FAILED(nsres))
        ERR("SetLength failed: %08lx\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_get_length(IHTMLSelectElement *iface, LONG *p)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    UINT32 length = 0;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLSelectElement_GetLength(This->nsselect, &length);
    if(NS_FAILED(nsres))
        ERR("GetLength failed: %08lx\n", nsres);

    *p = length;

    TRACE("ret %ld\n", *p);
    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_get__newEnum(IHTMLSelectElement *iface, IUnknown **p)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    HTMLSelectElementEnum *ret;

    TRACE("(%p)->(%p)\n", This, p);

    ret = malloc(sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IEnumVARIANT_iface.lpVtbl = &HTMLSelectElementEnumVtbl;
    ret->ref = 1;
    ret->iter = 0;

    HTMLSelectElement_AddRef(&This->IHTMLSelectElement_iface);
    ret->elem = This;

    *p = (IUnknown*)&ret->IEnumVARIANT_iface;
    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_item(IHTMLSelectElement *iface, VARIANT name,
                                             VARIANT index, IDispatch **pdisp)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);

    TRACE("(%p)->(%s %s %p)\n", This, debugstr_variant(&name), debugstr_variant(&index), pdisp);

    if(!pdisp)
        return E_POINTER;
    *pdisp = NULL;

    if(V_VT(&name) == VT_I4) {
        if(V_I4(&name) < 0)
            return E_INVALIDARG;
        return htmlselect_item(This, V_I4(&name), pdisp);
    }

    FIXME("Unsupported args\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLSelectElement_tags(IHTMLSelectElement *iface, VARIANT tagName,
                                             IDispatch **pdisp)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    FIXME("(%p)->(v %p)\n", This, pdisp);
    return E_NOTIMPL;
}

static const IHTMLSelectElementVtbl HTMLSelectElementVtbl = {
    HTMLSelectElement_QueryInterface,
    HTMLSelectElement_AddRef,
    HTMLSelectElement_Release,
    HTMLSelectElement_GetTypeInfoCount,
    HTMLSelectElement_GetTypeInfo,
    HTMLSelectElement_GetIDsOfNames,
    HTMLSelectElement_Invoke,
    HTMLSelectElement_put_size,
    HTMLSelectElement_get_size,
    HTMLSelectElement_put_multiple,
    HTMLSelectElement_get_multiple,
    HTMLSelectElement_put_name,
    HTMLSelectElement_get_name,
    HTMLSelectElement_get_options,
    HTMLSelectElement_put_onchange,
    HTMLSelectElement_get_onchange,
    HTMLSelectElement_put_selectedIndex,
    HTMLSelectElement_get_selectedIndex,
    HTMLSelectElement_get_type,
    HTMLSelectElement_put_value,
    HTMLSelectElement_get_value,
    HTMLSelectElement_put_disabled,
    HTMLSelectElement_get_disabled,
    HTMLSelectElement_get_form,
    HTMLSelectElement_add,
    HTMLSelectElement_remove,
    HTMLSelectElement_put_length,
    HTMLSelectElement_get_length,
    HTMLSelectElement_get__newEnum,
    HTMLSelectElement_item,
    HTMLSelectElement_tags
};

static inline HTMLSelectElement *impl_from_HTMLDOMNode(HTMLDOMNode *iface)
{
    return CONTAINING_RECORD(iface, HTMLSelectElement, element.node);
}

static HRESULT HTMLSelectElementImpl_put_disabled(HTMLDOMNode *iface, VARIANT_BOOL v)
{
    HTMLSelectElement *This = impl_from_HTMLDOMNode(iface);
    return IHTMLSelectElement_put_disabled(&This->IHTMLSelectElement_iface, v);
}

static HRESULT HTMLSelectElementImpl_get_disabled(HTMLDOMNode *iface, VARIANT_BOOL *p)
{
    HTMLSelectElement *This = impl_from_HTMLDOMNode(iface);
    return IHTMLSelectElement_get_disabled(&This->IHTMLSelectElement_iface, p);
}

static inline HTMLSelectElement *impl_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, HTMLSelectElement, element.node.event_target.dispex);
}

static void *HTMLSelectElement_query_interface(DispatchEx *dispex, REFIID riid)
{
    HTMLSelectElement *This = impl_from_DispatchEx(dispex);

    if(IsEqualGUID(&IID_IHTMLSelectElement, riid))
        return &This->IHTMLSelectElement_iface;

    return HTMLElement_query_interface(&This->element.node.event_target.dispex, riid);
}

static void HTMLSelectElement_traverse(DispatchEx *dispex, nsCycleCollectionTraversalCallback *cb)
{
    HTMLSelectElement *This = impl_from_DispatchEx(dispex);
    HTMLElement_traverse(dispex, cb);

    if(This->nsselect)
        note_cc_edge((nsISupports*)This->nsselect, "nsselect", cb);
}

static void HTMLSelectElement_unlink(DispatchEx *dispex)
{
    HTMLSelectElement *This = impl_from_DispatchEx(dispex);
    HTMLElement_unlink(dispex);
    unlink_ref(&This->nsselect);
}

#define DISPID_OPTIONCOL_0 MSHTML_DISPID_CUSTOM_MIN

static HRESULT HTMLSelectElement_get_dispid(DispatchEx *dispex, const WCHAR *name, DWORD flags, DISPID *dispid)
{
    const WCHAR *ptr;
    DWORD idx = 0;

    for(ptr = name; *ptr && is_digit(*ptr); ptr++) {
        idx = idx*10 + (*ptr-'0');
        if(idx > MSHTML_CUSTOM_DISPID_CNT) {
            WARN("too big idx\n");
            return DISP_E_UNKNOWNNAME;
        }
    }
    if(*ptr)
        return DISP_E_UNKNOWNNAME;

    *dispid = DISPID_OPTIONCOL_0 + idx;
    return S_OK;
}

static HRESULT HTMLSelectElement_invoke(DispatchEx *dispex, DISPID id, LCID lcid, WORD flags, DISPPARAMS *params,
        VARIANT *res, EXCEPINFO *ei, IServiceProvider *caller)
{
    HTMLSelectElement *This = impl_from_DispatchEx(dispex);

    TRACE("(%p)->(%lx %lx %x %p %p %p %p)\n", This, id, lcid, flags, params, res, ei, caller);

    switch(flags) {
    case DISPATCH_PROPERTYGET: {
        IDispatch *ret;
        HRESULT hres;

        hres = htmlselect_item(This, id-DISPID_OPTIONCOL_0, &ret);
        if(FAILED(hres))
            return hres;

        if(ret) {
            V_VT(res) = VT_DISPATCH;
            V_DISPATCH(res) = ret;
        }else {
            V_VT(res) = VT_NULL;
        }
        break;
    }

    default:
        FIXME("unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

static const NodeImplVtbl HTMLSelectElementImplVtbl = {
    .clsid                 = &CLSID_HTMLSelectElement,
    .cpc_entries           = HTMLElement_cpc,
    .clone                 = HTMLElement_clone,
    .get_attr_col          = HTMLElement_get_attr_col,
    .put_disabled          = HTMLSelectElementImpl_put_disabled,
    .get_disabled          = HTMLSelectElementImpl_get_disabled,
};

static const event_target_vtbl_t HTMLSelectElement_event_target_vtbl = {
    {
        HTMLELEMENT_DISPEX_VTBL_ENTRIES,
        .query_interface= HTMLSelectElement_query_interface,
        .destructor     = HTMLElement_destructor,
        .traverse       = HTMLSelectElement_traverse,
        .unlink         = HTMLSelectElement_unlink,
        .get_dispid     = HTMLSelectElement_get_dispid,
        .get_prop_desc  = dispex_index_prop_desc,
        .invoke         = HTMLSelectElement_invoke
    },
    HTMLELEMENT_EVENT_TARGET_VTBL_ENTRIES,
    .handle_event       = HTMLElement_handle_event
};

static const tid_t HTMLSelectElement_tids[] = {
    IHTMLSelectElement_tid,
    0
};

dispex_static_data_t HTMLSelectElement_dispex = {
    .id           = PROT_HTMLSelectElement,
    .prototype_id = PROT_HTMLElement,
    .vtbl         = &HTMLSelectElement_event_target_vtbl.dispex_vtbl,
    .disp_tid     = DispHTMLSelectElement_tid,
    .iface_tids   = HTMLSelectElement_tids,
    .init_info    = HTMLElement_init_dispex_info,
};

HRESULT HTMLSelectElement_Create(HTMLDocumentNode *doc, nsIDOMElement *nselem, HTMLElement **elem)
{
    HTMLSelectElement *ret;
    nsresult nsres;

    ret = calloc(1, sizeof(HTMLSelectElement));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IHTMLSelectElement_iface.lpVtbl = &HTMLSelectElementVtbl;
    ret->element.node.vtbl = &HTMLSelectElementImplVtbl;

    HTMLElement_Init(&ret->element, doc, nselem, &HTMLSelectElement_dispex);

    nsres = nsIDOMElement_QueryInterface(nselem, &IID_nsIDOMHTMLSelectElement, (void**)&ret->nsselect);
    assert(nsres == NS_OK);

    *elem = &ret->element;
    return S_OK;
}
