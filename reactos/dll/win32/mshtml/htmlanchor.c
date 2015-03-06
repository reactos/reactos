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

#include "mshtml_private.h"

typedef struct {
    HTMLElement element;

    IHTMLAnchorElement IHTMLAnchorElement_iface;

    nsIDOMHTMLAnchorElement *nsanchor;
} HTMLAnchorElement;

static HRESULT navigate_anchor_window(HTMLAnchorElement *This, const WCHAR *target)
{
    nsAString href_str;
    IUri *uri;
    nsresult nsres;
    HRESULT hres;

    nsAString_Init(&href_str, NULL);
    nsres = nsIDOMHTMLAnchorElement_GetHref(This->nsanchor, &href_str);
    if(NS_SUCCEEDED(nsres)) {
        const PRUnichar *href;

        nsAString_GetData(&href_str, &href);
        hres = create_relative_uri(This->element.node.doc->basedoc.window, href, &uri);
    }else {
        ERR("Could not get anchor href: %08x\n", nsres);
        hres = E_FAIL;
    }
    nsAString_Finish(&href_str);
    if(FAILED(hres))
        return hres;

    hres = navigate_new_window(This->element.node.doc->basedoc.window, uri, target, NULL);
    IUri_Release(uri);
    return hres;
}

HTMLOuterWindow *get_target_window(HTMLOuterWindow *window, nsAString *target_str, BOOL *use_new_window)
{
    HTMLOuterWindow *top_window, *ret_window;
    const PRUnichar *target;
    HRESULT hres;

    static const WCHAR _parentW[] = {'_','p','a','r','e','n','t',0};
    static const WCHAR _selfW[] = {'_','s','e','l','f',0};
    static const WCHAR _topW[] = {'_','t','o','p',0};

    *use_new_window = FALSE;

    nsAString_GetData(target_str, &target);
    TRACE("%s\n", debugstr_w(target));

    if(!*target || !strcmpiW(target, _selfW)) {
        IHTMLWindow2_AddRef(&window->base.IHTMLWindow2_iface);
        return window;
    }

    if(!strcmpiW(target, _topW)) {
        get_top_window(window, &top_window);
        IHTMLWindow2_AddRef(&top_window->base.IHTMLWindow2_iface);
        return top_window;
    }

    if(!strcmpiW(target, _parentW)) {
        if(!window->parent) {
            WARN("Window has no parent, treat as self\n");
            IHTMLWindow2_AddRef(&window->base.IHTMLWindow2_iface);
            return window;
        }

        IHTMLWindow2_AddRef(&window->parent->base.IHTMLWindow2_iface);
        return window->parent;
    }

    get_top_window(window, &top_window);

    hres = get_frame_by_name(top_window, target, TRUE, &ret_window);
    if(FAILED(hres) || !ret_window) {
        *use_new_window = TRUE;
        return NULL;
    }

    IHTMLWindow2_AddRef(&ret_window->base.IHTMLWindow2_iface);
    return ret_window;
}

static HRESULT navigate_anchor(HTMLAnchorElement *This)
{
    nsAString href_str, target_str;
    HTMLOuterWindow *window;
    BOOL use_new_window;
    nsresult nsres;
    HRESULT hres = E_FAIL;


    nsAString_Init(&target_str, NULL);
    nsres = nsIDOMHTMLAnchorElement_GetTarget(This->nsanchor, &target_str);
    if(NS_FAILED(nsres))
        return E_FAIL;

    window = get_target_window(This->element.node.doc->basedoc.window, &target_str, &use_new_window);
    if(!window && use_new_window) {
        const PRUnichar *target;

        nsAString_GetData(&target_str, &target);
        hres = navigate_anchor_window(This, target);
        nsAString_Finish(&target_str);
        return hres;
    }

    nsAString_Finish(&target_str);
    if(!window)
        return S_OK;

    nsAString_Init(&href_str, NULL);
    nsres = nsIDOMHTMLAnchorElement_GetHref(This->nsanchor, &href_str);
    if(NS_SUCCEEDED(nsres)) {
        const PRUnichar *href;

        nsAString_GetData(&href_str, &href);
        if(*href) {
            hres = navigate_url(window, href, window->uri_nofrag, BINDING_NAVIGATED);
        }else {
            TRACE("empty href\n");
            hres = S_OK;
        }
    }
    nsAString_Finish(&href_str);
    IHTMLWindow2_Release(&window->base.IHTMLWindow2_iface);
    return hres;
}

static inline HTMLAnchorElement *impl_from_IHTMLAnchorElement(IHTMLAnchorElement *iface)
{
    return CONTAINING_RECORD(iface, HTMLAnchorElement, IHTMLAnchorElement_iface);
}

static HRESULT WINAPI HTMLAnchorElement_QueryInterface(IHTMLAnchorElement *iface,
        REFIID riid, void **ppv)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);

    return IHTMLDOMNode_QueryInterface(&This->element.node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI HTMLAnchorElement_AddRef(IHTMLAnchorElement *iface)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);

    return IHTMLDOMNode_AddRef(&This->element.node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLAnchorElement_Release(IHTMLAnchorElement *iface)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);

    return IHTMLDOMNode_Release(&This->element.node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLAnchorElement_GetTypeInfoCount(IHTMLAnchorElement *iface, UINT *pctinfo)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    return IDispatchEx_GetTypeInfoCount(&This->element.node.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLAnchorElement_GetTypeInfo(IHTMLAnchorElement *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    return IDispatchEx_GetTypeInfo(&This->element.node.dispex.IDispatchEx_iface, iTInfo, lcid,
            ppTInfo);
}

static HRESULT WINAPI HTMLAnchorElement_GetIDsOfNames(IHTMLAnchorElement *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    return IDispatchEx_GetIDsOfNames(&This->element.node.dispex.IDispatchEx_iface, riid, rgszNames,
            cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLAnchorElement_Invoke(IHTMLAnchorElement *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    return IDispatchEx_Invoke(&This->element.node.dispex.IDispatchEx_iface, dispIdMember, riid,
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLAnchorElement_put_href(IHTMLAnchorElement *iface, BSTR v)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&nsstr, v);
    nsres = nsIDOMHTMLAnchorElement_SetHref(This->nsanchor, &nsstr);
    nsAString_Finish(&nsstr);
    if(NS_FAILED(nsres))
        return E_FAIL;

    return S_OK;
}

static HRESULT WINAPI HTMLAnchorElement_get_href(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
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
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&nsstr, v);
    nsres = nsIDOMHTMLAnchorElement_SetTarget(This->nsanchor, &nsstr);
    nsAString_Finish(&nsstr);
    if(NS_FAILED(nsres))
        return E_FAIL;

    return S_OK;
}

static HRESULT WINAPI HTMLAnchorElement_get_target(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    nsAString target_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&target_str, NULL);
    nsres = nsIDOMHTMLAnchorElement_GetTarget(This->nsanchor, &target_str);

    return return_nsstr(nsres, &target_str, p);
}

static HRESULT WINAPI HTMLAnchorElement_put_rel(IHTMLAnchorElement *iface, BSTR v)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&nsstr, v);
    nsres = nsIDOMHTMLAnchorElement_SetRel(This->nsanchor, &nsstr);
    nsAString_Finish(&nsstr);
    if(NS_FAILED(nsres))
        return E_FAIL;

    return S_OK;
}

static HRESULT WINAPI HTMLAnchorElement_get_rel(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&nsstr, NULL);
    nsres = nsIDOMHTMLAnchorElement_GetRel(This->nsanchor, &nsstr);
    return return_nsstr(nsres, &nsstr, p);
}

static HRESULT WINAPI HTMLAnchorElement_put_rev(IHTMLAnchorElement *iface, BSTR v)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_get_rev(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_put_urn(IHTMLAnchorElement *iface, BSTR v)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_get_urn(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_put_Methods(IHTMLAnchorElement *iface, BSTR v)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_get_Methods(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_put_name(IHTMLAnchorElement *iface, BSTR v)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&nsstr, v);
    nsres = nsIDOMHTMLAnchorElement_SetName(This->nsanchor, &nsstr);
    nsAString_Finish(&nsstr);
    if(NS_FAILED(nsres))
        return E_FAIL;

    return S_OK;
}

static HRESULT WINAPI HTMLAnchorElement_get_name(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    nsAString name_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&name_str, NULL);
    nsres = nsIDOMHTMLAnchorElement_GetName(This->nsanchor, &name_str);

    return return_nsstr(nsres, &name_str, p);
}

static HRESULT WINAPI HTMLAnchorElement_put_host(IHTMLAnchorElement *iface, BSTR v)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_get_host(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_put_hostname(IHTMLAnchorElement *iface, BSTR v)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_get_hostname(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    nsAString hostname_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&hostname_str, NULL);
    nsres = nsIDOMHTMLAnchorElement_GetHostname(This->nsanchor, &hostname_str);
    return return_nsstr(nsres, &hostname_str, p);
}

static HRESULT WINAPI HTMLAnchorElement_put_pathname(IHTMLAnchorElement *iface, BSTR v)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_get_pathname(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_put_port(IHTMLAnchorElement *iface, BSTR v)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_get_port(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_put_protocol(IHTMLAnchorElement *iface, BSTR v)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_get_protocol(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_put_search(IHTMLAnchorElement *iface, BSTR v)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_get_search(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_put_hash(IHTMLAnchorElement *iface, BSTR v)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_get_hash(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    nsAString hash_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&hash_str, NULL);
    nsres = nsIDOMHTMLAnchorElement_GetHash(This->nsanchor, &hash_str);
    return return_nsstr(nsres, &hash_str, p);
}

static HRESULT WINAPI HTMLAnchorElement_put_onblur(IHTMLAnchorElement *iface, VARIANT v)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);

    TRACE("(%p)->()\n", This);

    return IHTMLElement2_put_onblur(&This->element.IHTMLElement2_iface, v);
}

static HRESULT WINAPI HTMLAnchorElement_get_onblur(IHTMLAnchorElement *iface, VARIANT *p)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLElement2_get_onblur(&This->element.IHTMLElement2_iface, p);
}

static HRESULT WINAPI HTMLAnchorElement_put_onfocus(IHTMLAnchorElement *iface, VARIANT v)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);

    TRACE("(%p)->()\n", This);

    return IHTMLElement2_put_onfocus(&This->element.IHTMLElement2_iface, v);
}

static HRESULT WINAPI HTMLAnchorElement_get_onfocus(IHTMLAnchorElement *iface, VARIANT *p)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLElement2_get_onfocus(&This->element.IHTMLElement2_iface, p);
}

static HRESULT WINAPI HTMLAnchorElement_put_accessKey(IHTMLAnchorElement *iface, BSTR v)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return IHTMLElement2_put_accessKey(&This->element.IHTMLElement2_iface, v);
}

static HRESULT WINAPI HTMLAnchorElement_get_accessKey(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLElement2_get_accessKey(&This->element.IHTMLElement2_iface, p);
}

static HRESULT WINAPI HTMLAnchorElement_get_protocolLong(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_get_mimeType(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_get_nameProp(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_put_tabIndex(IHTMLAnchorElement *iface, short v)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);

    TRACE("(%p)->()\n", This);

    return IHTMLElement2_put_tabIndex(&This->element.IHTMLElement2_iface, v);
}

static HRESULT WINAPI HTMLAnchorElement_get_tabIndex(IHTMLAnchorElement *iface, short *p)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLElement2_get_tabIndex(&This->element.IHTMLElement2_iface, p);
}

static HRESULT WINAPI HTMLAnchorElement_focus(IHTMLAnchorElement *iface)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);

    TRACE("(%p)\n", This);

    return IHTMLElement2_focus(&This->element.IHTMLElement2_iface);
}

static HRESULT WINAPI HTMLAnchorElement_blur(IHTMLAnchorElement *iface)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);

    TRACE("(%p)\n", This);

    return IHTMLElement2_blur(&This->element.IHTMLElement2_iface);
}

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

static inline HTMLAnchorElement *impl_from_HTMLDOMNode(HTMLDOMNode *iface)
{
    return CONTAINING_RECORD(iface, HTMLAnchorElement, element.node);
}

static HRESULT HTMLAnchorElement_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLAnchorElement *This = impl_from_HTMLDOMNode(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IHTMLAnchorElement_iface;
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = &This->IHTMLAnchorElement_iface;
    }else if(IsEqualGUID(&IID_IHTMLAnchorElement, riid)) {
        TRACE("(%p)->(IID_IHTMLAnchorElement %p)\n", This, ppv);
        *ppv = &This->IHTMLAnchorElement_iface;
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    return HTMLElement_QI(&This->element.node, riid, ppv);
}

static HRESULT HTMLAnchorElement_handle_event(HTMLDOMNode *iface, eventid_t eid, nsIDOMEvent *event, BOOL *prevent_default)
{
    HTMLAnchorElement *This = impl_from_HTMLDOMNode(iface);

    if(eid == EVENTID_CLICK) {
        nsIDOMMouseEvent *mouse_event;
        UINT16 button;
        nsresult nsres;

        TRACE("CLICK\n");

        nsres = nsIDOMEvent_QueryInterface(event, &IID_nsIDOMMouseEvent, (void**)&mouse_event);
        assert(nsres == NS_OK);

        nsres = nsIDOMMouseEvent_GetButton(mouse_event, &button);
        assert(nsres == NS_OK);

        nsIDOMMouseEvent_Release(mouse_event);

        switch(button) {
        case 0:
            *prevent_default = TRUE;
            return navigate_anchor(This);
        case 1:
            *prevent_default = TRUE;
            return navigate_anchor_window(This, NULL);
        default:
            *prevent_default = FALSE;
            return S_OK;
        }
    }

    return HTMLElement_handle_event(&This->element.node, eid, event, prevent_default);
}

static const NodeImplVtbl HTMLAnchorElementImplVtbl = {
    HTMLAnchorElement_QI,
    HTMLElement_destructor,
    HTMLElement_cpc,
    HTMLElement_clone,
    HTMLAnchorElement_handle_event,
    HTMLElement_get_attr_col
};

static const tid_t HTMLAnchorElement_iface_tids[] = {
    IHTMLAnchorElement_tid,
    HTMLELEMENT_TIDS,
    IHTMLUniqueName_tid,
    0
};

static dispex_static_data_t HTMLAnchorElement_dispex = {
    NULL,
    DispHTMLAnchorElement_tid,
    NULL,
    HTMLAnchorElement_iface_tids
};

HRESULT HTMLAnchorElement_Create(HTMLDocumentNode *doc, nsIDOMHTMLElement *nselem, HTMLElement **elem)
{
    HTMLAnchorElement *ret;
    nsresult nsres;

    ret = heap_alloc_zero(sizeof(HTMLAnchorElement));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IHTMLAnchorElement_iface.lpVtbl = &HTMLAnchorElementVtbl;
    ret->element.node.vtbl = &HTMLAnchorElementImplVtbl;

    HTMLElement_Init(&ret->element, doc, nselem, &HTMLAnchorElement_dispex);

    nsres = nsIDOMHTMLElement_QueryInterface(nselem, &IID_nsIDOMHTMLAnchorElement, (void**)&ret->nsanchor);

    /* Shere the reference with nsnode */
    assert(nsres == NS_OK && (nsIDOMNode*)ret->nsanchor == ret->element.node.nsnode);
    nsIDOMNode_Release(ret->element.node.nsnode);

    *elem = &ret->element;
    return S_OK;
}
