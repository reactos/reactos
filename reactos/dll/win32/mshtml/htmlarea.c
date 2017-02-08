/*
 * Copyright 2015 Alex Henrie
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

    IHTMLAreaElement IHTMLAreaElement_iface;

    nsIDOMHTMLAreaElement *nsarea;
} HTMLAreaElement;

static inline HTMLAreaElement *impl_from_IHTMLAreaElement(IHTMLAreaElement *iface)
{
    return CONTAINING_RECORD(iface, HTMLAreaElement, IHTMLAreaElement_iface);
}

static HRESULT WINAPI HTMLAreaElement_QueryInterface(IHTMLAreaElement *iface, REFIID riid, void **ppv)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);

    return IHTMLDOMNode_QueryInterface(&This->element.node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI HTMLAreaElement_AddRef(IHTMLAreaElement *iface)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);

    return IHTMLDOMNode_AddRef(&This->element.node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLAreaElement_Release(IHTMLAreaElement *iface)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);

    return IHTMLDOMNode_Release(&This->element.node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLAreaElement_GetTypeInfoCount(IHTMLAreaElement *iface, UINT *pctinfo)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    return IDispatchEx_GetTypeInfoCount(&This->element.node.event_target.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLAreaElement_GetTypeInfo(IHTMLAreaElement *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    return IDispatchEx_GetTypeInfo(&This->element.node.event_target.dispex.IDispatchEx_iface, iTInfo, lcid,
            ppTInfo);
}

static HRESULT WINAPI HTMLAreaElement_GetIDsOfNames(IHTMLAreaElement *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    return IDispatchEx_GetIDsOfNames(&This->element.node.event_target.dispex.IDispatchEx_iface, riid, rgszNames,
            cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLAreaElement_Invoke(IHTMLAreaElement *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    return IDispatchEx_Invoke(&This->element.node.event_target.dispex.IDispatchEx_iface, dispIdMember, riid,
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLAreaElement_put_shape(IHTMLAreaElement *iface, BSTR v)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_get_shape(IHTMLAreaElement *iface, BSTR *p)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_put_coords(IHTMLAreaElement *iface, BSTR v)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_get_coords(IHTMLAreaElement *iface, BSTR *p)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_put_href(IHTMLAreaElement *iface, BSTR v)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_get_href(IHTMLAreaElement *iface, BSTR *p)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_put_target(IHTMLAreaElement *iface, BSTR v)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_get_target(IHTMLAreaElement *iface, BSTR *p)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_put_alt(IHTMLAreaElement *iface, BSTR v)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_get_alt(IHTMLAreaElement *iface, BSTR *p)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_put_noHref(IHTMLAreaElement *iface, VARIANT_BOOL v)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%i)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_get_noHref(IHTMLAreaElement *iface, VARIANT_BOOL *p)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_put_host(IHTMLAreaElement *iface, BSTR v)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_get_host(IHTMLAreaElement *iface, BSTR *p)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_put_hostname(IHTMLAreaElement *iface, BSTR v)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_get_hostname(IHTMLAreaElement *iface, BSTR *p)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_put_pathname(IHTMLAreaElement *iface, BSTR v)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_get_pathname(IHTMLAreaElement *iface, BSTR *p)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_put_port(IHTMLAreaElement *iface, BSTR v)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_get_port(IHTMLAreaElement *iface, BSTR *p)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_put_protocol(IHTMLAreaElement *iface, BSTR v)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_get_protocol(IHTMLAreaElement *iface, BSTR *p)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_put_search(IHTMLAreaElement *iface, BSTR v)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_get_search(IHTMLAreaElement *iface, BSTR *p)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_put_hash(IHTMLAreaElement *iface, BSTR v)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_get_hash(IHTMLAreaElement *iface, BSTR *p)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_put_onblur(IHTMLAreaElement *iface, VARIANT v)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%p)\n", This, &v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_get_onblur(IHTMLAreaElement *iface, VARIANT *p)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_put_onfocus(IHTMLAreaElement *iface, VARIANT v)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%p)\n", This, &v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_get_onfocus(IHTMLAreaElement *iface, VARIANT *p)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_put_tabIndex(IHTMLAreaElement *iface, short v)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%i)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_get_tabIndex(IHTMLAreaElement *iface, short *p)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_focus(IHTMLAreaElement *iface)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_blur(IHTMLAreaElement *iface)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static const IHTMLAreaElementVtbl HTMLAreaElementVtbl = {
    HTMLAreaElement_QueryInterface,
    HTMLAreaElement_AddRef,
    HTMLAreaElement_Release,
    HTMLAreaElement_GetTypeInfoCount,
    HTMLAreaElement_GetTypeInfo,
    HTMLAreaElement_GetIDsOfNames,
    HTMLAreaElement_Invoke,
    HTMLAreaElement_put_shape,
    HTMLAreaElement_get_shape,
    HTMLAreaElement_put_coords,
    HTMLAreaElement_get_coords,
    HTMLAreaElement_put_href,
    HTMLAreaElement_get_href,
    HTMLAreaElement_put_target,
    HTMLAreaElement_get_target,
    HTMLAreaElement_put_alt,
    HTMLAreaElement_get_alt,
    HTMLAreaElement_put_noHref,
    HTMLAreaElement_get_noHref,
    HTMLAreaElement_put_host,
    HTMLAreaElement_get_host,
    HTMLAreaElement_put_hostname,
    HTMLAreaElement_get_hostname,
    HTMLAreaElement_put_pathname,
    HTMLAreaElement_get_pathname,
    HTMLAreaElement_put_port,
    HTMLAreaElement_get_port,
    HTMLAreaElement_put_protocol,
    HTMLAreaElement_get_protocol,
    HTMLAreaElement_put_search,
    HTMLAreaElement_get_search,
    HTMLAreaElement_put_hash,
    HTMLAreaElement_get_hash,
    HTMLAreaElement_put_onblur,
    HTMLAreaElement_get_onblur,
    HTMLAreaElement_put_onfocus,
    HTMLAreaElement_get_onfocus,
    HTMLAreaElement_put_tabIndex,
    HTMLAreaElement_get_tabIndex,
    HTMLAreaElement_focus,
    HTMLAreaElement_blur
};

static inline HTMLAreaElement *impl_from_HTMLDOMNode(HTMLDOMNode *iface)
{
    return CONTAINING_RECORD(iface, HTMLAreaElement, element.node);
}

static HRESULT HTMLAreaElement_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLAreaElement *This = impl_from_HTMLDOMNode(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IHTMLAreaElement, riid)) {
        TRACE("(%p)->(IID_IHTMLAreaElement %p)\n", This, ppv);
        *ppv = &This->IHTMLAreaElement_iface;
    }else {
        return HTMLElement_QI(&This->element.node, riid, ppv);
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static HRESULT HTMLAreaElement_handle_event(HTMLDOMNode *iface, eventid_t eid, nsIDOMEvent *event, BOOL *prevent_default)
{
    HTMLAreaElement *This = impl_from_HTMLDOMNode(iface);
    nsAString href_str, target_str;
    nsresult nsres;

    if(eid == EVENTID_CLICK) {
        nsAString_Init(&href_str, NULL);
        nsres = nsIDOMHTMLAreaElement_GetHref(This->nsarea, &href_str);
        if (NS_FAILED(nsres)) {
            ERR("Could not get area href: %08x\n", nsres);
            goto fallback;
        }

        nsAString_Init(&target_str, NULL);
        nsres = nsIDOMHTMLAreaElement_GetTarget(This->nsarea, &target_str);
        if (NS_FAILED(nsres)) {
            ERR("Could not get area target: %08x\n", nsres);
            goto fallback;
        }

        return handle_link_click_event(&This->element, &href_str, &target_str, event, prevent_default);

fallback:
        nsAString_Finish(&href_str);
        nsAString_Finish(&target_str);
    }

    return HTMLElement_handle_event(&This->element.node, eid, event, prevent_default);
}

static const NodeImplVtbl HTMLAreaElementImplVtbl = {
    HTMLAreaElement_QI,
    HTMLElement_destructor,
    HTMLElement_cpc,
    HTMLElement_clone,
    HTMLAreaElement_handle_event,
    HTMLElement_get_attr_col
};

static const tid_t HTMLAreaElement_iface_tids[] = {
    HTMLELEMENT_TIDS,
    IHTMLAreaElement_tid,
    0
};
static dispex_static_data_t HTMLAreaElement_dispex = {
    NULL,
    DispHTMLAreaElement_tid,
    NULL,
    HTMLAreaElement_iface_tids
};

HRESULT HTMLAreaElement_Create(HTMLDocumentNode *doc, nsIDOMHTMLElement *nselem, HTMLElement **elem)
{
    HTMLAreaElement *ret;
    nsresult nsres;

    ret = heap_alloc_zero(sizeof(HTMLAreaElement));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IHTMLAreaElement_iface.lpVtbl = &HTMLAreaElementVtbl;
    ret->element.node.vtbl = &HTMLAreaElementImplVtbl;

    HTMLElement_Init(&ret->element, doc, nselem, &HTMLAreaElement_dispex);

    nsres = nsIDOMHTMLElement_QueryInterface(nselem, &IID_nsIDOMHTMLAreaElement, (void**)&ret->nsarea);
    assert(nsres == NS_OK);

    *elem = &ret->element;
    return S_OK;
}
