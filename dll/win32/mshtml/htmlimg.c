/*
 * Copyright 2008 Jacek Caban for CodeWeavers
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

    IHTMLImgElement IHTMLImgElement_iface;

    nsIDOMHTMLImageElement *nsimg;
} HTMLImgElement;

static inline HTMLImgElement *impl_from_IHTMLImgElement(IHTMLImgElement *iface)
{
    return CONTAINING_RECORD(iface, HTMLImgElement, IHTMLImgElement_iface);
}

static HRESULT WINAPI HTMLImgElement_QueryInterface(IHTMLImgElement *iface, REFIID riid, void **ppv)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);

    return IHTMLDOMNode_QueryInterface(&This->element.node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI HTMLImgElement_AddRef(IHTMLImgElement *iface)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);

    return IHTMLDOMNode_AddRef(&This->element.node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLImgElement_Release(IHTMLImgElement *iface)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);

    return IHTMLDOMNode_Release(&This->element.node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLImgElement_GetTypeInfoCount(IHTMLImgElement *iface, UINT *pctinfo)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);
    return IDispatchEx_GetTypeInfoCount(&This->element.node.event_target.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLImgElement_GetTypeInfo(IHTMLImgElement *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);
    return IDispatchEx_GetTypeInfo(&This->element.node.event_target.dispex.IDispatchEx_iface, iTInfo, lcid,
            ppTInfo);
}

static HRESULT WINAPI HTMLImgElement_GetIDsOfNames(IHTMLImgElement *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);
    return IDispatchEx_GetIDsOfNames(&This->element.node.event_target.dispex.IDispatchEx_iface, riid, rgszNames,
            cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLImgElement_Invoke(IHTMLImgElement *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);
    return IDispatchEx_Invoke(&This->element.node.event_target.dispex.IDispatchEx_iface, dispIdMember, riid,
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLImgElement_put_isMap(IHTMLImgElement *iface, VARIANT_BOOL v)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);
    nsresult nsres;

    TRACE("(%p)->(%x)\n", This, v);

    nsres = nsIDOMHTMLImageElement_SetIsMap(This->nsimg, v != VARIANT_FALSE);
    if (NS_FAILED(nsres)) {
        ERR("Set IsMap failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLImgElement_get_isMap(IHTMLImgElement *iface, VARIANT_BOOL *p)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);
    cpp_bool b;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if (p == NULL)
        return E_INVALIDARG;

    nsres = nsIDOMHTMLImageElement_GetIsMap(This->nsimg, &b);
    if (NS_FAILED(nsres)) {
        ERR("Get IsMap failed: %08x\n", nsres);
        return E_FAIL;
    }
    *p = b ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

static HRESULT WINAPI HTMLImgElement_put_useMap(IHTMLImgElement *iface, BSTR v)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_useMap(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_mimeType(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_fileSize(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_fileCreatedDate(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_fileModifiedDate(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_fileUpdatedDate(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_protocol(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_href(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_nameProp(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_put_border(IHTMLImgElement *iface, VARIANT v)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_border(IHTMLImgElement *iface, VARIANT *p)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_put_vspace(IHTMLImgElement *iface, LONG v)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->(%d)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_vspace(IHTMLImgElement *iface, LONG *p)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_put_hspace(IHTMLImgElement *iface, LONG v)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->(%d)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_hspace(IHTMLImgElement *iface, LONG *p)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_put_alt(IHTMLImgElement *iface, BSTR v)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);
    nsAString alt_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&alt_str, v);
    nsres = nsIDOMHTMLImageElement_SetAlt(This->nsimg, &alt_str);
    nsAString_Finish(&alt_str);
    if(NS_FAILED(nsres))
        ERR("SetAlt failed: %08x\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLImgElement_get_alt(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);
    nsAString alt_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&alt_str, NULL);
    nsres = nsIDOMHTMLImageElement_GetAlt(This->nsimg, &alt_str);
    return return_nsstr(nsres, &alt_str, p);
}

static HRESULT WINAPI HTMLImgElement_put_src(IHTMLImgElement *iface, BSTR v)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);
    nsAString src_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&src_str, v);
    nsres = nsIDOMHTMLImageElement_SetSrc(This->nsimg, &src_str);
    nsAString_Finish(&src_str);
    if(NS_FAILED(nsres))
        ERR("SetSrc failed: %08x\n", nsres);

    return NS_OK;
}

static HRESULT WINAPI HTMLImgElement_get_src(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);
    const PRUnichar *src;
    nsAString src_str;
    nsresult nsres;
    HRESULT hres = S_OK;

    static const WCHAR blockedW[] = {'B','L','O','C','K','E','D',':',':',0};

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&src_str, NULL);
    nsres = nsIDOMHTMLImageElement_GetSrc(This->nsimg, &src_str);
    if(NS_SUCCEEDED(nsres)) {
        nsAString_GetData(&src_str, &src);

        if(!strncmpiW(src, blockedW, sizeof(blockedW)/sizeof(WCHAR)-1)) {
            TRACE("returning BLOCKED::\n");
            *p = SysAllocString(blockedW);
            if(!*p)
                hres = E_OUTOFMEMORY;
        }else {
            hres = nsuri_to_url(src, TRUE, p);
        }
    }else {
        ERR("GetSrc failed: %08x\n", nsres);
        hres = E_FAIL;
    }

    nsAString_Finish(&src_str);
    return hres;
}

static HRESULT WINAPI HTMLImgElement_put_lowsrc(IHTMLImgElement *iface, BSTR v)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_lowsrc(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_put_vrml(IHTMLImgElement *iface, BSTR v)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_vrml(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_put_dynsrc(IHTMLImgElement *iface, BSTR v)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_dynsrc(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_readyState(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_complete(IHTMLImgElement *iface, VARIANT_BOOL *p)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);
    cpp_bool complete;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLImageElement_GetComplete(This->nsimg, &complete);
    if(NS_FAILED(nsres)) {
        ERR("GetComplete failed: %08x\n", nsres);
        return E_FAIL;
    }

    *p = complete ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

static HRESULT WINAPI HTMLImgElement_put_loop(IHTMLImgElement *iface, VARIANT v)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_loop(IHTMLImgElement *iface, VARIANT *p)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_put_align(IHTMLImgElement *iface, BSTR v)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);
    nsAString str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&str, v);

    nsres = nsIDOMHTMLImageElement_SetAlign(This->nsimg, &str);
    nsAString_Finish(&str);
    if (NS_FAILED(nsres)){
        ERR("Set Align(%s) failed: %08x\n", debugstr_w(v), nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLImgElement_get_align(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);
    nsAString str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&str, NULL);
    nsres = nsIDOMHTMLImageElement_GetAlign(This->nsimg, &str);

    return return_nsstr(nsres, &str, p);
}

static HRESULT WINAPI HTMLImgElement_put_onload(IHTMLImgElement *iface, VARIANT v)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_node_event(&This->element.node, EVENTID_LOAD, &v);
}

static HRESULT WINAPI HTMLImgElement_get_onload(IHTMLImgElement *iface, VARIANT *p)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_node_event(&This->element.node, EVENTID_LOAD, p);
}

static HRESULT WINAPI HTMLImgElement_put_onerror(IHTMLImgElement *iface, VARIANT v)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);

    TRACE("(%p)->()\n", This);

    return set_node_event(&This->element.node, EVENTID_ERROR, &v);
}

static HRESULT WINAPI HTMLImgElement_get_onerror(IHTMLImgElement *iface, VARIANT *p)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_node_event(&This->element.node, EVENTID_ERROR, p);
}

static HRESULT WINAPI HTMLImgElement_put_onabort(IHTMLImgElement *iface, VARIANT v)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);

    TRACE("(%p)->()\n", This);

    return set_node_event(&This->element.node, EVENTID_ABORT, &v);
}

static HRESULT WINAPI HTMLImgElement_get_onabort(IHTMLImgElement *iface, VARIANT *p)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_node_event(&This->element.node, EVENTID_ABORT, p);
}

static HRESULT WINAPI HTMLImgElement_put_name(IHTMLImgElement *iface, BSTR v)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_name(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);
    nsAString name;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&name, NULL);
    nsres = nsIDOMHTMLImageElement_GetName(This->nsimg, &name);
    return return_nsstr(nsres, &name, p);
}

static HRESULT WINAPI HTMLImgElement_put_width(IHTMLImgElement *iface, LONG v)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);
    nsresult nsres;

    TRACE("(%p)->(%d)\n", This, v);

    nsres = nsIDOMHTMLImageElement_SetWidth(This->nsimg, v);
    if(NS_FAILED(nsres)) {
        ERR("SetWidth failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLImgElement_get_width(IHTMLImgElement *iface, LONG *p)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);
    UINT32 width;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLImageElement_GetWidth(This->nsimg, &width);
    if(NS_FAILED(nsres)) {
        ERR("GetWidth failed: %08x\n", nsres);
        return E_FAIL;
    }

    *p = width;
    return S_OK;
}

static HRESULT WINAPI HTMLImgElement_put_height(IHTMLImgElement *iface, LONG v)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);
    nsresult nsres;

    TRACE("(%p)->(%d)\n", This, v);

    nsres = nsIDOMHTMLImageElement_SetHeight(This->nsimg, v);
    if(NS_FAILED(nsres)) {
        ERR("SetHeight failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLImgElement_get_height(IHTMLImgElement *iface, LONG *p)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);
    UINT32 height;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLImageElement_GetHeight(This->nsimg, &height);
    if(NS_FAILED(nsres)) {
        ERR("GetHeight failed: %08x\n", nsres);
        return E_FAIL;
    }

    *p = height;
    return S_OK;
}

static HRESULT WINAPI HTMLImgElement_put_start(IHTMLImgElement *iface, BSTR v)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_start(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImgElement *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLImgElementVtbl HTMLImgElementVtbl = {
    HTMLImgElement_QueryInterface,
    HTMLImgElement_AddRef,
    HTMLImgElement_Release,
    HTMLImgElement_GetTypeInfoCount,
    HTMLImgElement_GetTypeInfo,
    HTMLImgElement_GetIDsOfNames,
    HTMLImgElement_Invoke,
    HTMLImgElement_put_isMap,
    HTMLImgElement_get_isMap,
    HTMLImgElement_put_useMap,
    HTMLImgElement_get_useMap,
    HTMLImgElement_get_mimeType,
    HTMLImgElement_get_fileSize,
    HTMLImgElement_get_fileCreatedDate,
    HTMLImgElement_get_fileModifiedDate,
    HTMLImgElement_get_fileUpdatedDate,
    HTMLImgElement_get_protocol,
    HTMLImgElement_get_href,
    HTMLImgElement_get_nameProp,
    HTMLImgElement_put_border,
    HTMLImgElement_get_border,
    HTMLImgElement_put_vspace,
    HTMLImgElement_get_vspace,
    HTMLImgElement_put_hspace,
    HTMLImgElement_get_hspace,
    HTMLImgElement_put_alt,
    HTMLImgElement_get_alt,
    HTMLImgElement_put_src,
    HTMLImgElement_get_src,
    HTMLImgElement_put_lowsrc,
    HTMLImgElement_get_lowsrc,
    HTMLImgElement_put_vrml,
    HTMLImgElement_get_vrml,
    HTMLImgElement_put_dynsrc,
    HTMLImgElement_get_dynsrc,
    HTMLImgElement_get_readyState,
    HTMLImgElement_get_complete,
    HTMLImgElement_put_loop,
    HTMLImgElement_get_loop,
    HTMLImgElement_put_align,
    HTMLImgElement_get_align,
    HTMLImgElement_put_onload,
    HTMLImgElement_get_onload,
    HTMLImgElement_put_onerror,
    HTMLImgElement_get_onerror,
    HTMLImgElement_put_onabort,
    HTMLImgElement_get_onabort,
    HTMLImgElement_put_name,
    HTMLImgElement_get_name,
    HTMLImgElement_put_width,
    HTMLImgElement_get_width,
    HTMLImgElement_put_height,
    HTMLImgElement_get_height,
    HTMLImgElement_put_start,
    HTMLImgElement_get_start
};

static inline HTMLImgElement *impl_from_HTMLDOMNode(HTMLDOMNode *iface)
{
    return CONTAINING_RECORD(iface, HTMLImgElement, element.node);
}

static HRESULT HTMLImgElement_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLImgElement *This = impl_from_HTMLDOMNode(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IHTMLImgElement, riid)) {
        TRACE("(%p)->(IID_IHTMLImgElement %p)\n", This, ppv);
        *ppv = &This->IHTMLImgElement_iface;
    }else {
        return HTMLElement_QI(&This->element.node, riid, ppv);
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static HRESULT HTMLImgElement_get_readystate(HTMLDOMNode *iface, BSTR *p)
{
    HTMLImgElement *This = impl_from_HTMLDOMNode(iface);

    return IHTMLImgElement_get_readyState(&This->IHTMLImgElement_iface, p);
}

static void HTMLImgElement_traverse(HTMLDOMNode *iface, nsCycleCollectionTraversalCallback *cb)
{
    HTMLImgElement *This = impl_from_HTMLDOMNode(iface);

    if(This->nsimg)
        note_cc_edge((nsISupports*)This->nsimg, "This->nsimg", cb);
}

static void HTMLImgElement_unlink(HTMLDOMNode *iface)
{
    HTMLImgElement *This = impl_from_HTMLDOMNode(iface);

    if(This->nsimg) {
        nsIDOMHTMLImageElement *nsimg = This->nsimg;

        This->nsimg = NULL;
        nsIDOMHTMLImageElement_Release(nsimg);
    }
}

static const NodeImplVtbl HTMLImgElementImplVtbl = {
    HTMLImgElement_QI,
    HTMLElement_destructor,
    HTMLElement_cpc,
    HTMLElement_clone,
    HTMLElement_handle_event,
    HTMLElement_get_attr_col,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    HTMLImgElement_get_readystate,
    NULL,
    NULL,
    NULL,
    HTMLImgElement_traverse,
    HTMLImgElement_unlink
};

static const tid_t HTMLImgElement_iface_tids[] = {
    HTMLELEMENT_TIDS,
    IHTMLImgElement_tid,
    0
};
static dispex_static_data_t HTMLImgElement_dispex = {
    NULL,
    DispHTMLImg_tid,
    NULL,
    HTMLImgElement_iface_tids
};

HRESULT HTMLImgElement_Create(HTMLDocumentNode *doc, nsIDOMHTMLElement *nselem, HTMLElement **elem)
{
    HTMLImgElement *ret;
    nsresult nsres;

    ret = heap_alloc_zero(sizeof(HTMLImgElement));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IHTMLImgElement_iface.lpVtbl = &HTMLImgElementVtbl;
    ret->element.node.vtbl = &HTMLImgElementImplVtbl;

    HTMLElement_Init(&ret->element, doc, nselem, &HTMLImgElement_dispex);

    nsres = nsIDOMHTMLElement_QueryInterface(nselem, &IID_nsIDOMHTMLImageElement, (void**)&ret->nsimg);
    assert(nsres == NS_OK);

    *elem = &ret->element;
    return S_OK;
}

static inline HTMLImageElementFactory *impl_from_IHTMLImageElementFactory(IHTMLImageElementFactory *iface)
{
    return CONTAINING_RECORD(iface, HTMLImageElementFactory, IHTMLImageElementFactory_iface);
}

static HRESULT WINAPI HTMLImageElementFactory_QueryInterface(IHTMLImageElementFactory *iface,
        REFIID riid, void **ppv)
{
    HTMLImageElementFactory *This = impl_from_IHTMLImageElementFactory(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = &This->IHTMLImageElementFactory_iface;
    }else if(IsEqualGUID(&IID_IHTMLImageElementFactory, riid)) {
        *ppv = &This->IHTMLImageElementFactory_iface;
    }else if(dispex_query_interface(&This->dispex, riid, ppv)) {
        return *ppv ? S_OK : E_NOINTERFACE;
    }else {
        *ppv = NULL;
        WARN("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI HTMLImageElementFactory_AddRef(IHTMLImageElementFactory *iface)
{
    HTMLImageElementFactory *This = impl_from_IHTMLImageElementFactory(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLImageElementFactory_Release(IHTMLImageElementFactory *iface)
{
    HTMLImageElementFactory *This = impl_from_IHTMLImageElementFactory(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref)
        heap_free(This);

    return ref;
}

static HRESULT WINAPI HTMLImageElementFactory_GetTypeInfoCount(IHTMLImageElementFactory *iface,
        UINT *pctinfo)
{
    HTMLImageElementFactory *This = impl_from_IHTMLImageElementFactory(iface);
    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLImageElementFactory_GetTypeInfo(IHTMLImageElementFactory *iface,
        UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLImageElementFactory *This = impl_from_IHTMLImageElementFactory(iface);
    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLImageElementFactory_GetIDsOfNames(IHTMLImageElementFactory *iface,
        REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid,
        DISPID *rgDispId)
{
    HTMLImageElementFactory *This = impl_from_IHTMLImageElementFactory(iface);
    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLImageElementFactory_Invoke(IHTMLImageElementFactory *iface,
        DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
        DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo,
        UINT *puArgErr)
{
    HTMLImageElementFactory *This = impl_from_IHTMLImageElementFactory(iface);
    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid, lcid, wFlags,
            pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static LONG var_to_size(const VARIANT *v)
{
    switch(V_VT(v)) {
    case VT_EMPTY:
        return 0;
    case VT_I4:
        return V_I4(v);
    case VT_BSTR: {
        LONG ret;
        HRESULT hres;

        hres = VarI4FromStr(V_BSTR(v), 0, 0, &ret);
        if(FAILED(hres)) {
            FIXME("VarI4FromStr failed: %08x\n", hres);
            return 0;
        }
        return ret;
    }
    default:
        FIXME("unsupported size %s\n", debugstr_variant(v));
    }
    return 0;
}

static HRESULT WINAPI HTMLImageElementFactory_create(IHTMLImageElementFactory *iface,
        VARIANT width, VARIANT height, IHTMLImgElement **img_elem)
{
    HTMLImageElementFactory *This = impl_from_IHTMLImageElementFactory(iface);
    HTMLDocumentNode *doc;
    IHTMLImgElement *img;
    HTMLElement *elem;
    nsIDOMHTMLElement *nselem;
    LONG l;
    HRESULT hres;

    static const PRUnichar imgW[] = {'I','M','G',0};

    TRACE("(%p)->(%s %s %p)\n", This, debugstr_variant(&width),
            debugstr_variant(&height), img_elem);

    if(!This->window || !This->window->doc) {
        WARN("NULL doc\n");
        return E_UNEXPECTED;
    }

    doc = This->window->doc;

    *img_elem = NULL;

    hres = create_nselem(doc, imgW, &nselem);
    if(FAILED(hres))
        return hres;

    hres = HTMLElement_Create(doc, (nsIDOMNode*)nselem, FALSE, &elem);
    nsIDOMHTMLElement_Release(nselem);
    if(FAILED(hres)) {
        ERR("HTMLElement_Create failed\n");
        return hres;
    }

    hres = IHTMLElement_QueryInterface(&elem->IHTMLElement_iface, &IID_IHTMLImgElement,
            (void**)&img);
    IHTMLElement_Release(&elem->IHTMLElement_iface);
    if(FAILED(hres)) {
        ERR("IHTMLElement_QueryInterface failed: 0x%08x\n", hres);
        return hres;
    }

    l = var_to_size(&width);
    if(l)
        IHTMLImgElement_put_width(img, l);
    l = var_to_size(&height);
    if(l)
        IHTMLImgElement_put_height(img, l);

    *img_elem = img;
    return S_OK;
}

static const IHTMLImageElementFactoryVtbl HTMLImageElementFactoryVtbl = {
    HTMLImageElementFactory_QueryInterface,
    HTMLImageElementFactory_AddRef,
    HTMLImageElementFactory_Release,
    HTMLImageElementFactory_GetTypeInfoCount,
    HTMLImageElementFactory_GetTypeInfo,
    HTMLImageElementFactory_GetIDsOfNames,
    HTMLImageElementFactory_Invoke,
    HTMLImageElementFactory_create
};

static inline HTMLImageElementFactory *impl_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, HTMLImageElementFactory, dispex);
}

static HRESULT HTMLImageElementFactory_value(DispatchEx *dispex, LCID lcid,
        WORD flags, DISPPARAMS *params, VARIANT *res, EXCEPINFO *ei,
        IServiceProvider *caller)
{
    HTMLImageElementFactory *This = impl_from_DispatchEx(dispex);
    IHTMLImgElement *img;
    VARIANT empty, *width, *height;
    HRESULT hres;
    int argc = params->cArgs - params->cNamedArgs;

    V_VT(res) = VT_NULL;

    V_VT(&empty) = VT_EMPTY;

    width = argc >= 1 ? params->rgvarg + (params->cArgs - 1) : &empty;
    height = argc >= 2 ? params->rgvarg + (params->cArgs - 2) : &empty;

    hres = IHTMLImageElementFactory_create(&This->IHTMLImageElementFactory_iface, *width, *height,
            &img);
    if(FAILED(hres))
        return hres;

    V_VT(res) = VT_DISPATCH;
    V_DISPATCH(res) = (IDispatch*)img;

    return S_OK;
}

static const tid_t HTMLImageElementFactory_iface_tids[] = {
    IHTMLImageElementFactory_tid,
    0
};

static const dispex_static_data_vtbl_t HTMLImageElementFactory_dispex_vtbl = {
    HTMLImageElementFactory_value,
    NULL,
    NULL,
    NULL
};

static dispex_static_data_t HTMLImageElementFactory_dispex = {
    &HTMLImageElementFactory_dispex_vtbl,
    IHTMLImageElementFactory_tid,
    NULL,
    HTMLImageElementFactory_iface_tids
};

HRESULT HTMLImageElementFactory_Create(HTMLInnerWindow *window, HTMLImageElementFactory **ret_val)
{
    HTMLImageElementFactory *ret;

    ret = heap_alloc(sizeof(HTMLImageElementFactory));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IHTMLImageElementFactory_iface.lpVtbl = &HTMLImageElementFactoryVtbl;
    ret->ref = 1;
    ret->window = window;

    init_dispex(&ret->dispex, (IUnknown*)&ret->IHTMLImageElementFactory_iface,
            &HTMLImageElementFactory_dispex);

    *ret_val = ret;
    return S_OK;
}
