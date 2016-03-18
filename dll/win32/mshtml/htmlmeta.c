/*
 * Copyright 2012 Jacek Caban for CodeWeavers
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

    IHTMLMetaElement IHTMLMetaElement_iface;
} HTMLMetaElement;

static inline HTMLMetaElement *impl_from_IHTMLMetaElement(IHTMLMetaElement *iface)
{
    return CONTAINING_RECORD(iface, HTMLMetaElement, IHTMLMetaElement_iface);
}

static HRESULT WINAPI HTMLMetaElement_QueryInterface(IHTMLMetaElement *iface, REFIID riid, void **ppv)
{
    HTMLMetaElement *This = impl_from_IHTMLMetaElement(iface);

    return IHTMLDOMNode_QueryInterface(&This->element.node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI HTMLMetaElement_AddRef(IHTMLMetaElement *iface)
{
    HTMLMetaElement *This = impl_from_IHTMLMetaElement(iface);

    return IHTMLDOMNode_AddRef(&This->element.node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLMetaElement_Release(IHTMLMetaElement *iface)
{
    HTMLMetaElement *This = impl_from_IHTMLMetaElement(iface);

    return IHTMLDOMNode_Release(&This->element.node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLMetaElement_GetTypeInfoCount(IHTMLMetaElement *iface, UINT *pctinfo)
{
    HTMLMetaElement *This = impl_from_IHTMLMetaElement(iface);
    return IDispatchEx_GetTypeInfoCount(&This->element.node.event_target.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLMetaElement_GetTypeInfo(IHTMLMetaElement *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLMetaElement *This = impl_from_IHTMLMetaElement(iface);
    return IDispatchEx_GetTypeInfo(&This->element.node.event_target.dispex.IDispatchEx_iface, iTInfo, lcid,
            ppTInfo);
}

static HRESULT WINAPI HTMLMetaElement_GetIDsOfNames(IHTMLMetaElement *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLMetaElement *This = impl_from_IHTMLMetaElement(iface);
    return IDispatchEx_GetIDsOfNames(&This->element.node.event_target.dispex.IDispatchEx_iface, riid, rgszNames,
            cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLMetaElement_Invoke(IHTMLMetaElement *iface, DISPID dispIdMember, REFIID riid,
        LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo,
        UINT *puArgErr)
{
    HTMLMetaElement *This = impl_from_IHTMLMetaElement(iface);
    return IDispatchEx_Invoke(&This->element.node.event_target.dispex.IDispatchEx_iface, dispIdMember, riid,
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLMetaElement_put_httpEquiv(IHTMLMetaElement *iface, BSTR v)
{
    HTMLMetaElement *This = impl_from_IHTMLMetaElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLMetaElement_get_httpEquiv(IHTMLMetaElement *iface, BSTR *p)
{
    HTMLMetaElement *This = impl_from_IHTMLMetaElement(iface);
    static const PRUnichar httpEquivW[] = {'h','t','t','p','-','e','q','u','i','v',0};

    TRACE("(%p)->(%p)\n", This, p);

    return elem_string_attr_getter(&This->element, httpEquivW, TRUE, p);
}

static HRESULT WINAPI HTMLMetaElement_put_content(IHTMLMetaElement *iface, BSTR v)
{
    HTMLMetaElement *This = impl_from_IHTMLMetaElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLMetaElement_get_content(IHTMLMetaElement *iface, BSTR *p)
{
    HTMLMetaElement *This = impl_from_IHTMLMetaElement(iface);
    static const PRUnichar contentW[] = {'c','o','n','t','e','n','t',0};

    TRACE("(%p)->(%p)\n", This, p);

    return elem_string_attr_getter(&This->element, contentW, TRUE, p);
}

static HRESULT WINAPI HTMLMetaElement_put_name(IHTMLMetaElement *iface, BSTR v)
{
    HTMLMetaElement *This = impl_from_IHTMLMetaElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLMetaElement_get_name(IHTMLMetaElement *iface, BSTR *p)
{
    HTMLMetaElement *This = impl_from_IHTMLMetaElement(iface);
    static const PRUnichar nameW[] = {'n','a','m','e',0};

    TRACE("(%p)->(%p)\n", This, p);

    return elem_string_attr_getter(&This->element, nameW, TRUE, p);
}

static HRESULT WINAPI HTMLMetaElement_put_url(IHTMLMetaElement *iface, BSTR v)
{
    HTMLMetaElement *This = impl_from_IHTMLMetaElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLMetaElement_get_url(IHTMLMetaElement *iface, BSTR *p)
{
    HTMLMetaElement *This = impl_from_IHTMLMetaElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const WCHAR charsetW[] = {'c','h','a','r','s','e','t',0};

static HRESULT WINAPI HTMLMetaElement_put_charset(IHTMLMetaElement *iface, BSTR v)
{
    HTMLMetaElement *This = impl_from_IHTMLMetaElement(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return elem_string_attr_setter(&This->element, charsetW, v);
}

static HRESULT WINAPI HTMLMetaElement_get_charset(IHTMLMetaElement *iface, BSTR *p)
{
    HTMLMetaElement *This = impl_from_IHTMLMetaElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return elem_string_attr_getter(&This->element, charsetW, TRUE, p);
}

static const IHTMLMetaElementVtbl HTMLMetaElementVtbl = {
    HTMLMetaElement_QueryInterface,
    HTMLMetaElement_AddRef,
    HTMLMetaElement_Release,
    HTMLMetaElement_GetTypeInfoCount,
    HTMLMetaElement_GetTypeInfo,
    HTMLMetaElement_GetIDsOfNames,
    HTMLMetaElement_Invoke,
    HTMLMetaElement_put_httpEquiv,
    HTMLMetaElement_get_httpEquiv,
    HTMLMetaElement_put_content,
    HTMLMetaElement_get_content,
    HTMLMetaElement_put_name,
    HTMLMetaElement_get_name,
    HTMLMetaElement_put_url,
    HTMLMetaElement_get_url,
    HTMLMetaElement_put_charset,
    HTMLMetaElement_get_charset
};

static inline HTMLMetaElement *impl_from_HTMLDOMNode(HTMLDOMNode *iface)
{
    return CONTAINING_RECORD(iface, HTMLMetaElement, element.node);
}

static HRESULT HTMLMetaElement_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLMetaElement *This = impl_from_HTMLDOMNode(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IHTMLMetaElement_iface;
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = &This->IHTMLMetaElement_iface;
    }else if(IsEqualGUID(&IID_IHTMLMetaElement, riid)) {
        TRACE("(%p)->(IID_IHTMLMetaElement %p)\n", This, ppv);
        *ppv = &This->IHTMLMetaElement_iface;
    }else {
        return HTMLElement_QI(&This->element.node, riid, ppv);
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static void HTMLMetaElement_destructor(HTMLDOMNode *iface)
{
    HTMLMetaElement *This = impl_from_HTMLDOMNode(iface);

    HTMLElement_destructor(&This->element.node);
}

static const NodeImplVtbl HTMLMetaElementImplVtbl = {
    HTMLMetaElement_QI,
    HTMLMetaElement_destructor,
    HTMLElement_cpc,
    HTMLElement_clone,
    HTMLElement_handle_event,
    HTMLElement_get_attr_col
};

static const tid_t HTMLMetaElement_iface_tids[] = {
    HTMLELEMENT_TIDS,
    IHTMLMetaElement_tid,
    0
};

static dispex_static_data_t HTMLMetaElement_dispex = {
    NULL,
    DispHTMLMetaElement_tid,
    NULL,
    HTMLMetaElement_iface_tids
};

HRESULT HTMLMetaElement_Create(HTMLDocumentNode *doc, nsIDOMHTMLElement *nselem, HTMLElement **elem)
{
    HTMLMetaElement *ret;

    ret = heap_alloc_zero(sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IHTMLMetaElement_iface.lpVtbl = &HTMLMetaElementVtbl;
    ret->element.node.vtbl = &HTMLMetaElementImplVtbl;

    HTMLElement_Init(&ret->element, doc, nselem, &HTMLMetaElement_dispex);

    *elem = &ret->element;
    return S_OK;
}
