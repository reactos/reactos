/*
 * Copyright 2007 Jacek Caban for CodeWeavers
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
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"

#include "wine/debug.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

typedef struct {
    HTMLElement element;

    const IHTMLAnchorElementVtbl *lpHTMLAnchorElementVtbl;

    nsIDOMHTMLAnchorElement *nsanchor;
} HTMLAnchorElement;

#define HTMLANCHOR(x)  (&(x)->lpHTMLAnchorElementVtbl)

#define HTMLANCHOR_THIS(iface) DEFINE_THIS(HTMLAnchorElement, HTMLAnchorElement, iface)

static HRESULT WINAPI HTMLAnchorElement_QueryInterface(IHTMLAnchorElement *iface,
        REFIID riid, void **ppv)
{
    HTMLAnchorElement *This = HTMLANCHOR_THIS(iface);

    return IHTMLDOMNode_QueryInterface(HTMLDOMNODE(&This->element.node), riid, ppv);
}

static ULONG WINAPI HTMLAnchorElement_AddRef(IHTMLAnchorElement *iface)
{
    HTMLAnchorElement *This = HTMLANCHOR_THIS(iface);

    return IHTMLDOMNode_AddRef(HTMLDOMNODE(&This->element.node));
}

static ULONG WINAPI HTMLAnchorElement_Release(IHTMLAnchorElement *iface)
{
    HTMLAnchorElement *This = HTMLANCHOR_THIS(iface);

    return IHTMLDOMNode_Release(HTMLDOMNODE(&This->element.node));
}

static HRESULT WINAPI HTMLAnchorElement_GetTypeInfoCount(IHTMLAnchorElement *iface, UINT *pctinfo)
{
    HTMLAnchorElement *This = HTMLANCHOR_THIS(iface);
    return IDispatchEx_GetTypeInfoCount(DISPATCHEX(&This->element.node.dispex), pctinfo);
}

static HRESULT WINAPI HTMLAnchorElement_GetTypeInfo(IHTMLAnchorElement *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLAnchorElement *This = HTMLANCHOR_THIS(iface);
    return IDispatchEx_GetTypeInfo(DISPATCHEX(&This->element.node.dispex), iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLAnchorElement_GetIDsOfNames(IHTMLAnchorElement *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLAnchorElement *This = HTMLANCHOR_THIS(iface);
    return IDispatchEx_GetIDsOfNames(DISPATCHEX(&This->element.node.dispex), riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLAnchorElement_Invoke(IHTMLAnchorElement *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLAnchorElement *This = HTMLANCHOR_THIS(iface);
    return IDispatchEx_Invoke(DISPATCHEX(&This->element.node.dispex), dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLAnchorElement_put_href(IHTMLAnchorElement *iface, BSTR v)
{
    HTMLAnchorElement *This = HTMLANCHOR_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_get_href(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = HTMLANCHOR_THIS(iface);
    nsAString href_str;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&href_str, NULL);
    nsres = nsIDOMHTMLAnchorElement_GetHref(This->nsanchor, &href_str);
    if(NS_SUCCEEDED(nsres)) {
        const PRUnichar *href;

        nsAString_GetData(&href_str, &href);
        hres = nsuri_to_url(href, TRUE, p);
    }else {
        ERR("GetHref failed: %08x\n", nsres);
        hres = E_FAIL;
    }

    nsAString_Finish(&href_str);
    return hres;
}

static HRESULT WINAPI HTMLAnchorElement_put_target(IHTMLAnchorElement *iface, BSTR v)
{
    HTMLAnchorElement *This = HTMLANCHOR_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_get_target(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = HTMLANCHOR_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_put_rel(IHTMLAnchorElement *iface, BSTR v)
{
    HTMLAnchorElement *This = HTMLANCHOR_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_get_rel(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = HTMLANCHOR_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_put_rev(IHTMLAnchorElement *iface, BSTR v)
{
    HTMLAnchorElement *This = HTMLANCHOR_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_get_rev(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = HTMLANCHOR_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_put_urn(IHTMLAnchorElement *iface, BSTR v)
{
    HTMLAnchorElement *This = HTMLANCHOR_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_get_urn(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = HTMLANCHOR_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_put_Methods(IHTMLAnchorElement *iface, BSTR v)
{
    HTMLAnchorElement *This = HTMLANCHOR_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_get_Methods(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = HTMLANCHOR_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_put_name(IHTMLAnchorElement *iface, BSTR v)
{
    HTMLAnchorElement *This = HTMLANCHOR_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_get_name(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = HTMLANCHOR_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_put_host(IHTMLAnchorElement *iface, BSTR v)
{
    HTMLAnchorElement *This = HTMLANCHOR_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_get_host(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = HTMLANCHOR_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_put_hostname(IHTMLAnchorElement *iface, BSTR v)
{
    HTMLAnchorElement *This = HTMLANCHOR_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_get_hostname(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = HTMLANCHOR_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_put_pathname(IHTMLAnchorElement *iface, BSTR v)
{
    HTMLAnchorElement *This = HTMLANCHOR_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_get_pathname(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = HTMLANCHOR_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_put_port(IHTMLAnchorElement *iface, BSTR v)
{
    HTMLAnchorElement *This = HTMLANCHOR_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_get_port(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = HTMLANCHOR_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_put_protocol(IHTMLAnchorElement *iface, BSTR v)
{
    HTMLAnchorElement *This = HTMLANCHOR_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_get_protocol(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = HTMLANCHOR_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_put_search(IHTMLAnchorElement *iface, BSTR v)
{
    HTMLAnchorElement *This = HTMLANCHOR_THIS(iface);
    FIXME("(%p)->(%p)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_get_search(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = HTMLANCHOR_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_put_hash(IHTMLAnchorElement *iface, BSTR v)
{
    HTMLAnchorElement *This = HTMLANCHOR_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_get_hash(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = HTMLANCHOR_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_put_onblur(IHTMLAnchorElement *iface, VARIANT v)
{
    HTMLAnchorElement *This = HTMLANCHOR_THIS(iface);

    TRACE("(%p)->()\n", This);

    return IHTMLElement2_put_onblur(HTMLELEM2(&This->element), v);
}

static HRESULT WINAPI HTMLAnchorElement_get_onblur(IHTMLAnchorElement *iface, VARIANT *p)
{
    HTMLAnchorElement *This = HTMLANCHOR_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLElement2_get_onblur(HTMLELEM2(&This->element), p);
}

static HRESULT WINAPI HTMLAnchorElement_put_onfocus(IHTMLAnchorElement *iface, VARIANT v)
{
    HTMLAnchorElement *This = HTMLANCHOR_THIS(iface);

    TRACE("(%p)->()\n", This);

    return IHTMLElement2_put_onfocus(HTMLELEM2(&This->element), v);
}

static HRESULT WINAPI HTMLAnchorElement_get_onfocus(IHTMLAnchorElement *iface, VARIANT *p)
{
    HTMLAnchorElement *This = HTMLANCHOR_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLElement2_get_onfocus(HTMLELEM2(&This->element), p);
}

static HRESULT WINAPI HTMLAnchorElement_put_accessKey(IHTMLAnchorElement *iface, BSTR v)
{
    HTMLAnchorElement *This = HTMLANCHOR_THIS(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return IHTMLElement2_put_accessKey(HTMLELEM2(&This->element), v);
}

static HRESULT WINAPI HTMLAnchorElement_get_accessKey(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = HTMLANCHOR_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLElement2_get_accessKey(HTMLELEM2(&This->element), p);
}

static HRESULT WINAPI HTMLAnchorElement_get_protocolLong(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = HTMLANCHOR_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_get_mimeType(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = HTMLANCHOR_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_get_nameProp(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = HTMLANCHOR_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_put_tabIndex(IHTMLAnchorElement *iface, short v)
{
    HTMLAnchorElement *This = HTMLANCHOR_THIS(iface);

    TRACE("(%p)->()\n", This);

    return IHTMLElement2_put_tabIndex(HTMLELEM2(&This->element), v);
}

static HRESULT WINAPI HTMLAnchorElement_get_tabIndex(IHTMLAnchorElement *iface, short *p)
{
    HTMLAnchorElement *This = HTMLANCHOR_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLElement2_get_tabIndex(HTMLELEM2(&This->element), p);
}

static HRESULT WINAPI HTMLAnchorElement_focus(IHTMLAnchorElement *iface)
{
    HTMLAnchorElement *This = HTMLANCHOR_THIS(iface);

    TRACE("(%p)\n", This);

    return IHTMLElement2_focus(HTMLELEM2(&This->element));
}

static HRESULT WINAPI HTMLAnchorElement_blur(IHTMLAnchorElement *iface)
{
    HTMLAnchorElement *This = HTMLANCHOR_THIS(iface);

    TRACE("(%p)\n", This);

    return IHTMLElement2_blur(HTMLELEM2(&This->element));
}

#undef HTMLANCHOR_THIS

static const IHTMLAnchorElementVtbl HTMLAnchorElementVtbl = {
    HTMLAnchorElement_QueryInterface,
    HTMLAnchorElement_AddRef,
    HTMLAnchorElement_Release,
    HTMLAnchorElement_GetTypeInfoCount,
    HTMLAnchorElement_GetTypeInfo,
    HTMLAnchorElement_GetIDsOfNames,
    HTMLAnchorElement_Invoke,
    HTMLAnchorElement_put_href,
    HTMLAnchorElement_get_href,
    HTMLAnchorElement_put_target,
    HTMLAnchorElement_get_target,
    HTMLAnchorElement_put_rel,
    HTMLAnchorElement_get_rel,
    HTMLAnchorElement_put_rev,
    HTMLAnchorElement_get_rev,
    HTMLAnchorElement_put_urn,
    HTMLAnchorElement_get_urn,
    HTMLAnchorElement_put_Methods,
    HTMLAnchorElement_get_Methods,
    HTMLAnchorElement_put_name,
    HTMLAnchorElement_get_name,
    HTMLAnchorElement_put_host,
    HTMLAnchorElement_get_host,
    HTMLAnchorElement_put_hostname,
    HTMLAnchorElement_get_hostname,
    HTMLAnchorElement_put_pathname,
    HTMLAnchorElement_get_pathname,
    HTMLAnchorElement_put_port,
    HTMLAnchorElement_get_port,
    HTMLAnchorElement_put_protocol,
    HTMLAnchorElement_get_protocol,
    HTMLAnchorElement_put_search,
    HTMLAnchorElement_get_search,
    HTMLAnchorElement_put_hash,
    HTMLAnchorElement_get_hash,
    HTMLAnchorElement_put_onblur,
    HTMLAnchorElement_get_onblur,
    HTMLAnchorElement_put_onfocus,
    HTMLAnchorElement_get_onfocus,
    HTMLAnchorElement_put_accessKey,
    HTMLAnchorElement_get_accessKey,
    HTMLAnchorElement_get_protocolLong,
    HTMLAnchorElement_get_mimeType,
    HTMLAnchorElement_get_nameProp,
    HTMLAnchorElement_put_tabIndex,
    HTMLAnchorElement_get_tabIndex,
    HTMLAnchorElement_focus,
    HTMLAnchorElement_blur
};

#define HTMLANCHOR_NODE_THIS(iface) DEFINE_THIS2(HTMLAnchorElement, element.node, iface)

static HRESULT HTMLAnchorElement_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLAnchorElement *This = HTMLANCHOR_NODE_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = HTMLANCHOR(This);
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = HTMLANCHOR(This);
    }else if(IsEqualGUID(&IID_IHTMLAnchorElement, riid)) {
        TRACE("(%p)->(IID_IHTMLAnchorElement %p)\n", This, ppv);
        *ppv = HTMLANCHOR(This);
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    return HTMLElement_QI(&This->element.node, riid, ppv);
}

static void HTMLAnchorElement_destructor(HTMLDOMNode *iface)
{
    HTMLAnchorElement *This = HTMLANCHOR_NODE_THIS(iface);

    if(This->nsanchor)
        nsIDOMHTMLAnchorElement_Release(This->nsanchor);

    HTMLElement_destructor(&This->element.node);
}

#undef HTMLANCHOR_NODE_THIS

static const NodeImplVtbl HTMLAnchorElementImplVtbl = {
    HTMLAnchorElement_QI,
    HTMLAnchorElement_destructor
};

static const tid_t HTMLAnchorElement_iface_tids[] = {
    IHTMLAnchorElement_tid,
    HTMLELEMENT_TIDS,
    IHTMLTextContainer_tid,
    IHTMLUniqueName_tid,
    0
};

static dispex_static_data_t HTMLAnchorElement_dispex = {
    NULL,
    DispHTMLAnchorElement_tid,
    NULL,
    HTMLAnchorElement_iface_tids
};

HTMLElement *HTMLAnchorElement_Create(HTMLDocumentNode *doc, nsIDOMHTMLElement *nselem)
{
    HTMLAnchorElement *ret = heap_alloc_zero(sizeof(HTMLAnchorElement));
    nsresult nsres;

    ret->lpHTMLAnchorElementVtbl = &HTMLAnchorElementVtbl;
    ret->element.node.vtbl = &HTMLAnchorElementImplVtbl;

    HTMLElement_Init(&ret->element, doc, nselem, &HTMLAnchorElement_dispex);

    nsres = nsIDOMHTMLElement_QueryInterface(nselem, &IID_nsIDOMHTMLAnchorElement, (void**)&ret->nsanchor);
    if(NS_FAILED(nsres))
        ERR("Could not get nsIDOMHTMLAnchorElement iface: %08x\n", nsres);

    return &ret->element;
}
