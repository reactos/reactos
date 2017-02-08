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

#include "mshtml_private.h"

typedef struct {
    HTMLElement element;

    IHTMLStyleElement IHTMLStyleElement_iface;

    nsIDOMHTMLStyleElement *nsstyle;
    IHTMLStyleSheet *style_sheet;
} HTMLStyleElement;

static inline HTMLStyleElement *impl_from_IHTMLStyleElement(IHTMLStyleElement *iface)
{
    return CONTAINING_RECORD(iface, HTMLStyleElement, IHTMLStyleElement_iface);
}

static HRESULT WINAPI HTMLStyleElement_QueryInterface(IHTMLStyleElement *iface,
        REFIID riid, void **ppv)
{
    HTMLStyleElement *This = impl_from_IHTMLStyleElement(iface);

    return IHTMLDOMNode_QueryInterface(&This->element.node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI HTMLStyleElement_AddRef(IHTMLStyleElement *iface)
{
    HTMLStyleElement *This = impl_from_IHTMLStyleElement(iface);

    return IHTMLDOMNode_AddRef(&This->element.node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLStyleElement_Release(IHTMLStyleElement *iface)
{
    HTMLStyleElement *This = impl_from_IHTMLStyleElement(iface);

    return IHTMLDOMNode_Release(&This->element.node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLStyleElement_GetTypeInfoCount(IHTMLStyleElement *iface, UINT *pctinfo)
{
    HTMLStyleElement *This = impl_from_IHTMLStyleElement(iface);
    return IDispatchEx_GetTypeInfoCount(&This->element.node.event_target.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLStyleElement_GetTypeInfo(IHTMLStyleElement *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLStyleElement *This = impl_from_IHTMLStyleElement(iface);
    return IDispatchEx_GetTypeInfo(&This->element.node.event_target.dispex.IDispatchEx_iface, iTInfo, lcid,
            ppTInfo);
}

static HRESULT WINAPI HTMLStyleElement_GetIDsOfNames(IHTMLStyleElement *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLStyleElement *This = impl_from_IHTMLStyleElement(iface);
    return IDispatchEx_GetIDsOfNames(&This->element.node.event_target.dispex.IDispatchEx_iface, riid, rgszNames,
            cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLStyleElement_Invoke(IHTMLStyleElement *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLStyleElement *This = impl_from_IHTMLStyleElement(iface);
    return IDispatchEx_Invoke(&This->element.node.event_target.dispex.IDispatchEx_iface, dispIdMember, riid,
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLStyleElement_put_type(IHTMLStyleElement *iface, BSTR v)
{
    HTMLStyleElement *This = impl_from_IHTMLStyleElement(iface);
    nsAString type_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&type_str, v);
    nsres = nsIDOMHTMLStyleElement_SetType(This->nsstyle, &type_str);
    nsAString_Finish(&type_str);
    if(NS_FAILED(nsres)) {
        ERR("SetType failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLStyleElement_get_type(IHTMLStyleElement *iface, BSTR *p)
{
    HTMLStyleElement *This = impl_from_IHTMLStyleElement(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&nsstr, NULL);
    nsres = nsIDOMHTMLStyleElement_GetType(This->nsstyle, &nsstr);
    return return_nsstr(nsres, &nsstr, p);
}

static HRESULT WINAPI HTMLStyleElement_get_readyState(IHTMLStyleElement *iface, BSTR *p)
{
    HTMLStyleElement *This = impl_from_IHTMLStyleElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleElement_put_onreadystatechange(IHTMLStyleElement *iface, VARIANT v)
{
    HTMLStyleElement *This = impl_from_IHTMLStyleElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleElement_get_onreadystatechange(IHTMLStyleElement *iface, VARIANT *p)
{
    HTMLStyleElement *This = impl_from_IHTMLStyleElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleElement_put_onload(IHTMLStyleElement *iface, VARIANT v)
{
    HTMLStyleElement *This = impl_from_IHTMLStyleElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleElement_get_onload(IHTMLStyleElement *iface, VARIANT *p)
{
    HTMLStyleElement *This = impl_from_IHTMLStyleElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleElement_put_onerror(IHTMLStyleElement *iface, VARIANT v)
{
    HTMLStyleElement *This = impl_from_IHTMLStyleElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleElement_get_onerror(IHTMLStyleElement *iface, VARIANT *p)
{
    HTMLStyleElement *This = impl_from_IHTMLStyleElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleElement_get_styleSheet(IHTMLStyleElement *iface, IHTMLStyleSheet **p)
{
    HTMLStyleElement *This = impl_from_IHTMLStyleElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->nsstyle)
        return E_FAIL;

    if(!This->style_sheet) {
        nsIDOMStyleSheet *ss;
        nsresult nsres;

        nsres = nsIDOMHTMLStyleElement_GetDOMStyleSheet(This->nsstyle, &ss);
        assert(nsres == NS_OK);

        if(ss) {
            This->style_sheet = HTMLStyleSheet_Create(ss);
            nsIDOMStyleSheet_Release(ss);
            if(!This->style_sheet)
                return E_OUTOFMEMORY;
        }
    }

    if(This->style_sheet)
        IHTMLStyleSheet_AddRef(This->style_sheet);
    *p = This->style_sheet;
    return S_OK;
}

static HRESULT WINAPI HTMLStyleElement_put_disabled(IHTMLStyleElement *iface, VARIANT_BOOL v)
{
    HTMLStyleElement *This = impl_from_IHTMLStyleElement(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleElement_get_disabled(IHTMLStyleElement *iface, VARIANT_BOOL *p)
{
    HTMLStyleElement *This = impl_from_IHTMLStyleElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyleElement_put_media(IHTMLStyleElement *iface, BSTR v)
{
    HTMLStyleElement *This = impl_from_IHTMLStyleElement(iface);
    nsAString media_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&media_str, v);
    nsres = nsIDOMHTMLStyleElement_SetMedia(This->nsstyle, &media_str);
    nsAString_Finish(&media_str);
    if(NS_FAILED(nsres)) {
        ERR("SetMedia failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLStyleElement_get_media(IHTMLStyleElement *iface, BSTR *p)
{
    HTMLStyleElement *This = impl_from_IHTMLStyleElement(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&nsstr, NULL);
    nsres = nsIDOMHTMLStyleElement_GetMedia(This->nsstyle, &nsstr);
    return return_nsstr(nsres, &nsstr, p);
}

static const IHTMLStyleElementVtbl HTMLStyleElementVtbl = {
    HTMLStyleElement_QueryInterface,
    HTMLStyleElement_AddRef,
    HTMLStyleElement_Release,
    HTMLStyleElement_GetTypeInfoCount,
    HTMLStyleElement_GetTypeInfo,
    HTMLStyleElement_GetIDsOfNames,
    HTMLStyleElement_Invoke,
    HTMLStyleElement_put_type,
    HTMLStyleElement_get_type,
    HTMLStyleElement_get_readyState,
    HTMLStyleElement_put_onreadystatechange,
    HTMLStyleElement_get_onreadystatechange,
    HTMLStyleElement_put_onload,
    HTMLStyleElement_get_onload,
    HTMLStyleElement_put_onerror,
    HTMLStyleElement_get_onerror,
    HTMLStyleElement_get_styleSheet,
    HTMLStyleElement_put_disabled,
    HTMLStyleElement_get_disabled,
    HTMLStyleElement_put_media,
    HTMLStyleElement_get_media
};

static inline HTMLStyleElement *impl_from_HTMLDOMNode(HTMLDOMNode *iface)
{
    return CONTAINING_RECORD(iface, HTMLStyleElement, element.node);
}

static HRESULT HTMLStyleElement_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLStyleElement *This = impl_from_HTMLDOMNode(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IHTMLStyleElement_iface;
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = &This->IHTMLStyleElement_iface;
    }else if(IsEqualGUID(&IID_IHTMLStyleElement, riid)) {
        TRACE("(%p)->(IID_IHTMLStyleElement %p)\n", This, ppv);
        *ppv = &This->IHTMLStyleElement_iface;
    }else {
        return HTMLElement_QI(&This->element.node, riid, ppv);
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static void HTMLStyleElement_destructor(HTMLDOMNode *iface)
{
    HTMLStyleElement *This = impl_from_HTMLDOMNode(iface);

    if(This->style_sheet) {
        IHTMLStyleSheet_Release(This->style_sheet);
        This->style_sheet = NULL;
    }

    HTMLElement_destructor(iface);
}

static void HTMLStyleElement_traverse(HTMLDOMNode *iface, nsCycleCollectionTraversalCallback *cb)
{
    HTMLStyleElement *This = impl_from_HTMLDOMNode(iface);

    if(This->nsstyle)
        note_cc_edge((nsISupports*)This->nsstyle, "This->nsstyle", cb);
}

static void HTMLStyleElement_unlink(HTMLDOMNode *iface)
{
    HTMLStyleElement *This = impl_from_HTMLDOMNode(iface);

    if(This->nsstyle) {
        nsIDOMHTMLStyleElement *nsstyle = This->nsstyle;

        This->nsstyle = NULL;
        nsIDOMHTMLStyleElement_Release(nsstyle);
    }
}

static const NodeImplVtbl HTMLStyleElementImplVtbl = {
    HTMLStyleElement_QI,
    HTMLStyleElement_destructor,
    HTMLElement_cpc,
    HTMLElement_clone,
    HTMLElement_handle_event,
    HTMLElement_get_attr_col,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    HTMLStyleElement_traverse,
    HTMLStyleElement_unlink
};

static const tid_t HTMLStyleElement_iface_tids[] = {
    HTMLELEMENT_TIDS,
    IHTMLStyleElement_tid,
    0
};
static dispex_static_data_t HTMLStyleElement_dispex = {
    NULL,
    DispHTMLStyleElement_tid,
    NULL,
    HTMLStyleElement_iface_tids
};

HRESULT HTMLStyleElement_Create(HTMLDocumentNode *doc, nsIDOMHTMLElement *nselem, HTMLElement **elem)
{
    HTMLStyleElement *ret;
    nsresult nsres;

    ret = heap_alloc_zero(sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IHTMLStyleElement_iface.lpVtbl = &HTMLStyleElementVtbl;
    ret->element.node.vtbl = &HTMLStyleElementImplVtbl;

    HTMLElement_Init(&ret->element, doc, nselem, &HTMLStyleElement_dispex);

    nsres = nsIDOMHTMLElement_QueryInterface(nselem, &IID_nsIDOMHTMLStyleElement, (void**)&ret->nsstyle);
    assert(nsres == NS_OK);

    *elem = &ret->element;
    return S_OK;
}
