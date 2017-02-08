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
    IHTMLLinkElement IHTMLLinkElement_iface;

    nsIDOMHTMLLinkElement *nslink;
} HTMLLinkElement;

static inline HTMLLinkElement *impl_from_IHTMLLinkElement(IHTMLLinkElement *iface)
{
    return CONTAINING_RECORD(iface, HTMLLinkElement, IHTMLLinkElement_iface);
}

static HRESULT WINAPI HTMLLinkElement_QueryInterface(IHTMLLinkElement *iface,
                                                         REFIID riid, void **ppv)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);

    return IHTMLDOMNode_QueryInterface(&This->element.node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI HTMLLinkElement_AddRef(IHTMLLinkElement *iface)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);

    return IHTMLDOMNode_AddRef(&This->element.node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLLinkElement_Release(IHTMLLinkElement *iface)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);

    return IHTMLDOMNode_Release(&This->element.node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLLinkElement_GetTypeInfoCount(IHTMLLinkElement *iface, UINT *pctinfo)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);

    return IDispatchEx_GetTypeInfoCount(&This->element.node.event_target.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLLinkElement_GetTypeInfo(IHTMLLinkElement *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);

    return IDispatchEx_GetTypeInfo(&This->element.node.event_target.dispex.IDispatchEx_iface, iTInfo, lcid,
            ppTInfo);
}

static HRESULT WINAPI HTMLLinkElement_GetIDsOfNames(IHTMLLinkElement *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);

    return IDispatchEx_GetIDsOfNames(&This->element.node.event_target.dispex.IDispatchEx_iface, riid, rgszNames,
            cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLLinkElement_Invoke(IHTMLLinkElement *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);

    return IDispatchEx_Invoke(&This->element.node.event_target.dispex.IDispatchEx_iface, dispIdMember, riid,
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLLinkElement_put_href(IHTMLLinkElement *iface, BSTR v)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);
    nsAString href_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&href_str, v);
    nsres = nsIDOMHTMLLinkElement_SetHref(This->nslink, &href_str);
    nsAString_Finish(&href_str);

    return NS_SUCCEEDED(nsres) ? S_OK : E_FAIL;
}

static HRESULT WINAPI HTMLLinkElement_get_href(IHTMLLinkElement *iface, BSTR *p)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);
    nsAString href_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&href_str, NULL);
    nsres = nsIDOMHTMLLinkElement_GetHref(This->nslink, &href_str);
    return return_nsstr(nsres, &href_str, p);
}

static HRESULT WINAPI HTMLLinkElement_put_rel(IHTMLLinkElement *iface, BSTR v)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);
    nsAString rel_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&rel_str, v);
    nsres = nsIDOMHTMLLinkElement_SetRel(This->nslink, &rel_str);
    nsAString_Finish(&rel_str);

    return NS_SUCCEEDED(nsres) ? S_OK : E_FAIL;
}

static HRESULT WINAPI HTMLLinkElement_get_rel(IHTMLLinkElement *iface, BSTR *p)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);
    nsAString rel_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&rel_str, NULL);
    nsres = nsIDOMHTMLLinkElement_GetRel(This->nslink, &rel_str);
    return return_nsstr(nsres, &rel_str, p);
}

static HRESULT WINAPI HTMLLinkElement_put_rev(IHTMLLinkElement *iface, BSTR v)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&nsstr, v);
    nsres = nsIDOMHTMLLinkElement_SetRev(This->nslink, &nsstr);
    nsAString_Finish(&nsstr);
    if(NS_FAILED(nsres)) {
        ERR("SetRev failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLLinkElement_get_rev(IHTMLLinkElement *iface, BSTR *p)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&nsstr, NULL);
    nsres = nsIDOMHTMLLinkElement_GetRev(This->nslink, &nsstr);
    return return_nsstr(nsres, &nsstr, p);
}

static HRESULT WINAPI HTMLLinkElement_put_type(IHTMLLinkElement *iface, BSTR v)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);
    nsAString type_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&type_str, v);
    nsres = nsIDOMHTMLLinkElement_SetType(This->nslink, &type_str);
    nsAString_Finish(&type_str);

    return NS_SUCCEEDED(nsres) ? S_OK : E_FAIL;
}

static HRESULT WINAPI HTMLLinkElement_get_type(IHTMLLinkElement *iface, BSTR *p)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);
    nsAString type_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&type_str, NULL);
    nsres = nsIDOMHTMLLinkElement_GetType(This->nslink, &type_str);
    return return_nsstr(nsres, &type_str, p);
}

static HRESULT WINAPI HTMLLinkElement_get_readyState(IHTMLLinkElement *iface, BSTR *p)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLLinkElement_put_onreadystatechange(IHTMLLinkElement *iface, VARIANT v)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLLinkElement_get_onreadystatechange(IHTMLLinkElement *iface, VARIANT *p)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLLinkElement_put_onload(IHTMLLinkElement *iface, VARIANT v)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLLinkElement_get_onload(IHTMLLinkElement *iface, VARIANT *p)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLLinkElement_put_onerror(IHTMLLinkElement *iface, VARIANT v)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLLinkElement_get_onerror(IHTMLLinkElement *iface, VARIANT *p)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLLinkElement_get_styleSheet(IHTMLLinkElement *iface, IHTMLStyleSheet **p)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLLinkElement_put_disabled(IHTMLLinkElement *iface, VARIANT_BOOL v)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);
    nsresult nsres;

    TRACE("(%p)->(%x)\n", This, v);

    nsres = nsIDOMHTMLLinkElement_SetDisabled(This->nslink, !!v);
    return SUCCEEDED(nsres) ? S_OK : E_FAIL;
}

static HRESULT WINAPI HTMLLinkElement_get_disabled(IHTMLLinkElement *iface, VARIANT_BOOL *p)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);
    cpp_bool ret;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLLinkElement_GetDisabled(This->nslink, &ret);
    if(NS_FAILED(nsres))
        return E_FAIL;

    *p = ret ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

static HRESULT WINAPI HTMLLinkElement_put_media(IHTMLLinkElement *iface, BSTR v)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);
    nsresult nsres;
    nsAString str;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&str, v);

    nsres = nsIDOMHTMLLinkElement_SetMedia(This->nslink, &str);
    nsAString_Finish(&str);

    if(NS_FAILED(nsres)) {
        ERR("Set Media(%s) failed: %08x\n", debugstr_w(v), nsres);
        return E_FAIL;
    }
    return S_OK;
}

static HRESULT WINAPI HTMLLinkElement_get_media(IHTMLLinkElement *iface, BSTR *p)
{
    HTMLLinkElement *This = impl_from_IHTMLLinkElement(iface);
    nsresult nsres;
    nsAString str;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&str, NULL);
    nsres = nsIDOMHTMLLinkElement_GetMedia(This->nslink, &str);

    return return_nsstr(nsres, &str, p);
}

static const IHTMLLinkElementVtbl HTMLLinkElementVtbl = {
    HTMLLinkElement_QueryInterface,
    HTMLLinkElement_AddRef,
    HTMLLinkElement_Release,
    HTMLLinkElement_GetTypeInfoCount,
    HTMLLinkElement_GetTypeInfo,
    HTMLLinkElement_GetIDsOfNames,
    HTMLLinkElement_Invoke,
    HTMLLinkElement_put_href,
    HTMLLinkElement_get_href,
    HTMLLinkElement_put_rel,
    HTMLLinkElement_get_rel,
    HTMLLinkElement_put_rev,
    HTMLLinkElement_get_rev,
    HTMLLinkElement_put_type,
    HTMLLinkElement_get_type,
    HTMLLinkElement_get_readyState,
    HTMLLinkElement_put_onreadystatechange,
    HTMLLinkElement_get_onreadystatechange,
    HTMLLinkElement_put_onload,
    HTMLLinkElement_get_onload,
    HTMLLinkElement_put_onerror,
    HTMLLinkElement_get_onerror,
    HTMLLinkElement_get_styleSheet,
    HTMLLinkElement_put_disabled,
    HTMLLinkElement_get_disabled,
    HTMLLinkElement_put_media,
    HTMLLinkElement_get_media
};

static inline HTMLLinkElement *impl_from_HTMLDOMNode(HTMLDOMNode *iface)
{
    return CONTAINING_RECORD(iface, HTMLLinkElement, element.node);
}

static HRESULT HTMLLinkElement_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLLinkElement *This = impl_from_HTMLDOMNode(iface);

    if(IsEqualGUID(&IID_IHTMLLinkElement, riid)) {
        TRACE("(%p)->(IID_IHTMLLinkElement %p)\n", This, ppv);
        *ppv = &This->IHTMLLinkElement_iface;
    }else {
        return HTMLElement_QI(&This->element.node, riid, ppv);
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static HRESULT HTMLLinkElementImpl_put_disabled(HTMLDOMNode *iface, VARIANT_BOOL v)
{
    HTMLLinkElement *This = impl_from_HTMLDOMNode(iface);
    return IHTMLLinkElement_put_disabled(&This->IHTMLLinkElement_iface, v);
}

static HRESULT HTMLLinkElementImpl_get_disabled(HTMLDOMNode *iface, VARIANT_BOOL *p)
{
    HTMLLinkElement *This = impl_from_HTMLDOMNode(iface);
    return IHTMLLinkElement_get_disabled(&This->IHTMLLinkElement_iface, p);
}

static void HTMLLinkElement_traverse(HTMLDOMNode *iface, nsCycleCollectionTraversalCallback *cb)
{
    HTMLLinkElement *This = impl_from_HTMLDOMNode(iface);

    if(This->nslink)
        note_cc_edge((nsISupports*)This->nslink, "This->nslink", cb);
}

static void HTMLLinkElement_unlink(HTMLDOMNode *iface)
{
    HTMLLinkElement *This = impl_from_HTMLDOMNode(iface);

    if(This->nslink) {
        nsIDOMHTMLLinkElement *nslink = This->nslink;

        This->nslink = NULL;
        nsIDOMHTMLLinkElement_Release(nslink);
    }
}
static const NodeImplVtbl HTMLLinkElementImplVtbl = {
    HTMLLinkElement_QI,
    HTMLElement_destructor,
    HTMLElement_cpc,
    HTMLElement_clone,
    HTMLElement_handle_event,
    HTMLElement_get_attr_col,
    NULL,
    NULL,
    HTMLLinkElementImpl_put_disabled,
    HTMLLinkElementImpl_get_disabled,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    HTMLLinkElement_traverse,
    HTMLLinkElement_unlink
};

static const tid_t HTMLLinkElement_iface_tids[] = {
    HTMLELEMENT_TIDS,
    IHTMLLinkElement_tid,
    0
};
static dispex_static_data_t HTMLLinkElement_dispex = {
    NULL,
    DispHTMLLinkElement_tid,
    NULL,
    HTMLLinkElement_iface_tids
};

HRESULT HTMLLinkElement_Create(HTMLDocumentNode *doc, nsIDOMHTMLElement *nselem, HTMLElement **elem)
{
    HTMLLinkElement *ret;
    nsresult nsres;

    ret = heap_alloc_zero(sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IHTMLLinkElement_iface.lpVtbl = &HTMLLinkElementVtbl;
    ret->element.node.vtbl = &HTMLLinkElementImplVtbl;

    HTMLElement_Init(&ret->element, doc, nselem, &HTMLLinkElement_dispex);

    nsres = nsIDOMHTMLElement_QueryInterface(nselem, &IID_nsIDOMHTMLLinkElement, (void**)&ret->nslink);
    assert(nsres == NS_OK);

    *elem = &ret->element;
    return S_OK;
}
