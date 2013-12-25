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

    IHTMLEmbedElement IHTMLEmbedElement_iface;
} HTMLEmbedElement;

static inline HTMLEmbedElement *impl_from_IHTMLEmbedElement(IHTMLEmbedElement *iface)
{
    return CONTAINING_RECORD(iface, HTMLEmbedElement, IHTMLEmbedElement_iface);
}

static HRESULT WINAPI HTMLEmbedElement_QueryInterface(IHTMLEmbedElement *iface,
        REFIID riid, void **ppv)
{
    HTMLEmbedElement *This = impl_from_IHTMLEmbedElement(iface);

    return IHTMLDOMNode_QueryInterface(&This->element.node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI HTMLEmbedElement_AddRef(IHTMLEmbedElement *iface)
{
    HTMLEmbedElement *This = impl_from_IHTMLEmbedElement(iface);

    return IHTMLDOMNode_AddRef(&This->element.node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLEmbedElement_Release(IHTMLEmbedElement *iface)
{
    HTMLEmbedElement *This = impl_from_IHTMLEmbedElement(iface);

    return IHTMLDOMNode_Release(&This->element.node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLEmbedElement_GetTypeInfoCount(IHTMLEmbedElement *iface, UINT *pctinfo)
{
    HTMLEmbedElement *This = impl_from_IHTMLEmbedElement(iface);
    return IDispatchEx_GetTypeInfoCount(&This->element.node.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLEmbedElement_GetTypeInfo(IHTMLEmbedElement *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLEmbedElement *This = impl_from_IHTMLEmbedElement(iface);
    return IDispatchEx_GetTypeInfo(&This->element.node.dispex.IDispatchEx_iface, iTInfo, lcid,
            ppTInfo);
}

static HRESULT WINAPI HTMLEmbedElement_GetIDsOfNames(IHTMLEmbedElement *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLEmbedElement *This = impl_from_IHTMLEmbedElement(iface);
    return IDispatchEx_GetIDsOfNames(&This->element.node.dispex.IDispatchEx_iface, riid, rgszNames,
            cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLEmbedElement_Invoke(IHTMLEmbedElement *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLEmbedElement *This = impl_from_IHTMLEmbedElement(iface);
    return IDispatchEx_Invoke(&This->element.node.dispex.IDispatchEx_iface, dispIdMember, riid,
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLEmbedElement_put_hidden(IHTMLEmbedElement *iface, BSTR v)
{
    HTMLEmbedElement *This = impl_from_IHTMLEmbedElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEmbedElement_get_hidden(IHTMLEmbedElement *iface, BSTR *p)
{
    HTMLEmbedElement *This = impl_from_IHTMLEmbedElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEmbedElement_get_palete(IHTMLEmbedElement *iface, BSTR *p)
{
    HTMLEmbedElement *This = impl_from_IHTMLEmbedElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEmbedElement_get_pluginspage(IHTMLEmbedElement *iface, BSTR *p)
{
    HTMLEmbedElement *This = impl_from_IHTMLEmbedElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEmbedElement_put_src(IHTMLEmbedElement *iface, BSTR v)
{
    HTMLEmbedElement *This = impl_from_IHTMLEmbedElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEmbedElement_get_src(IHTMLEmbedElement *iface, BSTR *p)
{
    HTMLEmbedElement *This = impl_from_IHTMLEmbedElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEmbedElement_put_units(IHTMLEmbedElement *iface, BSTR v)
{
    HTMLEmbedElement *This = impl_from_IHTMLEmbedElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEmbedElement_get_units(IHTMLEmbedElement *iface, BSTR *p)
{
    HTMLEmbedElement *This = impl_from_IHTMLEmbedElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEmbedElement_put_name(IHTMLEmbedElement *iface, BSTR v)
{
    HTMLEmbedElement *This = impl_from_IHTMLEmbedElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEmbedElement_get_name(IHTMLEmbedElement *iface, BSTR *p)
{
    HTMLEmbedElement *This = impl_from_IHTMLEmbedElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEmbedElement_put_width(IHTMLEmbedElement *iface, VARIANT v)
{
    HTMLEmbedElement *This = impl_from_IHTMLEmbedElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEmbedElement_get_width(IHTMLEmbedElement *iface, VARIANT *p)
{
    HTMLEmbedElement *This = impl_from_IHTMLEmbedElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEmbedElement_put_height(IHTMLEmbedElement *iface, VARIANT v)
{
    HTMLEmbedElement *This = impl_from_IHTMLEmbedElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEmbedElement_get_height(IHTMLEmbedElement *iface, VARIANT *p)
{
    HTMLEmbedElement *This = impl_from_IHTMLEmbedElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLEmbedElementVtbl HTMLEmbedElementVtbl = {
    HTMLEmbedElement_QueryInterface,
    HTMLEmbedElement_AddRef,
    HTMLEmbedElement_Release,
    HTMLEmbedElement_GetTypeInfoCount,
    HTMLEmbedElement_GetTypeInfo,
    HTMLEmbedElement_GetIDsOfNames,
    HTMLEmbedElement_Invoke,
    HTMLEmbedElement_put_hidden,
    HTMLEmbedElement_get_hidden,
    HTMLEmbedElement_get_palete,
    HTMLEmbedElement_get_pluginspage,
    HTMLEmbedElement_put_src,
    HTMLEmbedElement_get_src,
    HTMLEmbedElement_put_units,
    HTMLEmbedElement_get_units,
    HTMLEmbedElement_put_name,
    HTMLEmbedElement_get_name,
    HTMLEmbedElement_put_width,
    HTMLEmbedElement_get_width,
    HTMLEmbedElement_put_height,
    HTMLEmbedElement_get_height
};

static inline HTMLEmbedElement *impl_from_HTMLDOMNode(HTMLDOMNode *iface)
{
    return CONTAINING_RECORD(iface, HTMLEmbedElement, element.node);
}

static HRESULT HTMLEmbedElement_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLEmbedElement *This = impl_from_HTMLDOMNode(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IHTMLEmbedElement_iface;
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = &This->IHTMLEmbedElement_iface;
    }else if(IsEqualGUID(&IID_IHTMLEmbedElement, riid)) {
        TRACE("(%p)->(IID_IHTMLEmbedElement %p)\n", This, ppv);
        *ppv = &This->IHTMLEmbedElement_iface;
    }else {
        return HTMLElement_QI(&This->element.node, riid, ppv);
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static void HTMLEmbedElement_destructor(HTMLDOMNode *iface)
{
    HTMLEmbedElement *This = impl_from_HTMLDOMNode(iface);

    HTMLElement_destructor(&This->element.node);
}

static const NodeImplVtbl HTMLEmbedElementImplVtbl = {
    HTMLEmbedElement_QI,
    HTMLEmbedElement_destructor,
    HTMLElement_cpc,
    HTMLElement_clone,
    HTMLElement_handle_event,
    HTMLElement_get_attr_col
};

static const tid_t HTMLEmbedElement_iface_tids[] = {
    HTMLELEMENT_TIDS,
    IHTMLEmbedElement_tid,
    0
};
static dispex_static_data_t HTMLEmbedElement_dispex = {
    NULL,
    DispHTMLEmbed_tid,
    NULL,
    HTMLEmbedElement_iface_tids
};

HRESULT HTMLEmbedElement_Create(HTMLDocumentNode *doc, nsIDOMHTMLElement *nselem, HTMLElement **elem)
{
    HTMLEmbedElement *ret;

    ret = heap_alloc_zero(sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IHTMLEmbedElement_iface.lpVtbl = &HTMLEmbedElementVtbl;
    ret->element.node.vtbl = &HTMLEmbedElementImplVtbl;

    HTMLElement_Init(&ret->element, doc, nselem, &HTMLEmbedElement_dispex);
    *elem = &ret->element;
    return S_OK;
}
