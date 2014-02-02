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
    HTMLFrameBase framebase;
    IHTMLFrameElement3 IHTMLFrameElement3_iface;
} HTMLFrameElement;

static inline HTMLFrameElement *impl_from_IHTMLFrameElement3(IHTMLFrameElement3 *iface)
{
    return CONTAINING_RECORD(iface, HTMLFrameElement, IHTMLFrameElement3_iface);
}

static HRESULT WINAPI HTMLFrameElement3_QueryInterface(IHTMLFrameElement3 *iface,
        REFIID riid, void **ppv)
{
    HTMLFrameElement *This = impl_from_IHTMLFrameElement3(iface);

    return IHTMLDOMNode_QueryInterface(&This->framebase.element.node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI HTMLFrameElement3_AddRef(IHTMLFrameElement3 *iface)
{
    HTMLFrameElement *This = impl_from_IHTMLFrameElement3(iface);

    return IHTMLDOMNode_AddRef(&This->framebase.element.node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLFrameElement3_Release(IHTMLFrameElement3 *iface)
{
    HTMLFrameElement *This = impl_from_IHTMLFrameElement3(iface);

    return IHTMLDOMNode_Release(&This->framebase.element.node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLFrameElement3_GetTypeInfoCount(IHTMLFrameElement3 *iface, UINT *pctinfo)
{
    HTMLFrameElement *This = impl_from_IHTMLFrameElement3(iface);
    return IDispatchEx_GetTypeInfoCount(&This->framebase.element.node.dispex.IDispatchEx_iface,
            pctinfo);
}

static HRESULT WINAPI HTMLFrameElement3_GetTypeInfo(IHTMLFrameElement3 *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLFrameElement *This = impl_from_IHTMLFrameElement3(iface);
    return IDispatchEx_GetTypeInfo(&This->framebase.element.node.dispex.IDispatchEx_iface, iTInfo,
            lcid, ppTInfo);
}

static HRESULT WINAPI HTMLFrameElement3_GetIDsOfNames(IHTMLFrameElement3 *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLFrameElement *This = impl_from_IHTMLFrameElement3(iface);
    return IDispatchEx_GetIDsOfNames(&This->framebase.element.node.dispex.IDispatchEx_iface, riid,
            rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLFrameElement3_Invoke(IHTMLFrameElement3 *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLFrameElement *This = impl_from_IHTMLFrameElement3(iface);
    return IDispatchEx_Invoke(&This->framebase.element.node.dispex.IDispatchEx_iface, dispIdMember,
            riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLFrameElement3_get_contentDocument(IHTMLFrameElement3 *iface, IDispatch **p)
{
    HTMLFrameElement *This = impl_from_IHTMLFrameElement3(iface);
    IHTMLDocument2 *doc;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->framebase.content_window) {
        FIXME("NULL window\n");
        return E_FAIL;
    }

    hres = IHTMLWindow2_get_document(&This->framebase.content_window->base.IHTMLWindow2_iface, &doc);
    if(FAILED(hres))
        return hres;

    *p = doc ? (IDispatch*)doc : NULL;
    return S_OK;
}

static HRESULT WINAPI HTMLFrameElement3_put_src(IHTMLFrameElement3 *iface, BSTR v)
{
    HTMLFrameElement *This = impl_from_IHTMLFrameElement3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameElement3_get_src(IHTMLFrameElement3 *iface, BSTR *p)
{
    HTMLFrameElement *This = impl_from_IHTMLFrameElement3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameElement3_put_longDesc(IHTMLFrameElement3 *iface, BSTR v)
{
    HTMLFrameElement *This = impl_from_IHTMLFrameElement3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameElement3_get_longDesc(IHTMLFrameElement3 *iface, BSTR *p)
{
    HTMLFrameElement *This = impl_from_IHTMLFrameElement3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameElement3_put_frameBorder(IHTMLFrameElement3 *iface, BSTR v)
{
    HTMLFrameElement *This = impl_from_IHTMLFrameElement3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameElement3_get_frameBorder(IHTMLFrameElement3 *iface, BSTR *p)
{
    HTMLFrameElement *This = impl_from_IHTMLFrameElement3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLFrameElement3Vtbl HTMLFrameElement3Vtbl = {
    HTMLFrameElement3_QueryInterface,
    HTMLFrameElement3_AddRef,
    HTMLFrameElement3_Release,
    HTMLFrameElement3_GetTypeInfoCount,
    HTMLFrameElement3_GetTypeInfo,
    HTMLFrameElement3_GetIDsOfNames,
    HTMLFrameElement3_Invoke,
    HTMLFrameElement3_get_contentDocument,
    HTMLFrameElement3_put_src,
    HTMLFrameElement3_get_src,
    HTMLFrameElement3_put_longDesc,
    HTMLFrameElement3_get_longDesc,
    HTMLFrameElement3_put_frameBorder,
    HTMLFrameElement3_get_frameBorder
};

static inline HTMLFrameElement *impl_from_HTMLDOMNode(HTMLDOMNode *iface)
{
    return CONTAINING_RECORD(iface, HTMLFrameElement, framebase.element.node);
}

static HRESULT HTMLFrameElement_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLFrameElement *This = impl_from_HTMLDOMNode(iface);

    if(IsEqualGUID(&IID_IHTMLFrameElement3, riid)) {
        TRACE("(%p)->(IID_IHTMLFrameElement3 %p)\n", This, ppv);
        *ppv = &This->IHTMLFrameElement3_iface;
    }else {
        return HTMLFrameBase_QI(&This->framebase, riid, ppv);
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static void HTMLFrameElement_destructor(HTMLDOMNode *iface)
{
    HTMLFrameElement *This = impl_from_HTMLDOMNode(iface);

    HTMLFrameBase_destructor(&This->framebase);
}

static HRESULT HTMLFrameElement_get_document(HTMLDOMNode *iface, IDispatch **p)
{
    HTMLFrameElement *This = impl_from_HTMLDOMNode(iface);

    if(!This->framebase.content_window || !This->framebase.content_window->base.inner_window->doc) {
        *p = NULL;
        return S_OK;
    }

    *p = (IDispatch*)&This->framebase.content_window->base.inner_window->doc->basedoc.IHTMLDocument2_iface;
    IDispatch_AddRef(*p);
    return S_OK;
}

static HRESULT HTMLFrameElement_get_readystate(HTMLDOMNode *iface, BSTR *p)
{
    HTMLFrameElement *This = impl_from_HTMLDOMNode(iface);

    return IHTMLFrameBase2_get_readyState(&This->framebase.IHTMLFrameBase2_iface, p);
}

static HRESULT HTMLFrameElement_get_dispid(HTMLDOMNode *iface, BSTR name,
        DWORD grfdex, DISPID *pid)
{
    HTMLFrameElement *This = impl_from_HTMLDOMNode(iface);

    if(!This->framebase.content_window)
        return DISP_E_UNKNOWNNAME;

    return search_window_props(This->framebase.content_window->base.inner_window, name, grfdex, pid);
}

static HRESULT HTMLFrameElement_invoke(HTMLDOMNode *iface, DISPID id, LCID lcid,
        WORD flags, DISPPARAMS *params, VARIANT *res, EXCEPINFO *ei, IServiceProvider *caller)
{
    HTMLFrameElement *This = impl_from_HTMLDOMNode(iface);

    if(!This->framebase.content_window) {
        ERR("no content window to invoke on\n");
        return E_FAIL;
    }

    return IDispatchEx_InvokeEx(&This->framebase.content_window->base.IDispatchEx_iface, id, lcid,
            flags, params, res, ei, caller);
}

static HRESULT HTMLFrameElement_bind_to_tree(HTMLDOMNode *iface)
{
    HTMLFrameElement *This = impl_from_HTMLDOMNode(iface);
    nsIDOMDocument *nsdoc;
    nsresult nsres;
    HRESULT hres;

    nsres = nsIDOMHTMLFrameElement_GetContentDocument(This->framebase.nsframe, &nsdoc);
    if(NS_FAILED(nsres) || !nsdoc) {
        ERR("GetContentDocument failed: %08x\n", nsres);
        return E_FAIL;
    }

    hres = set_frame_doc(&This->framebase, nsdoc);
    nsIDOMDocument_Release(nsdoc);
    return hres;
}

static const NodeImplVtbl HTMLFrameElementImplVtbl = {
    HTMLFrameElement_QI,
    HTMLFrameElement_destructor,
    HTMLElement_cpc,
    HTMLElement_clone,
    HTMLElement_handle_event,
    HTMLElement_get_attr_col,
    NULL,
    NULL,
    NULL,
    NULL,
    HTMLFrameElement_get_document,
    HTMLFrameElement_get_readystate,
    HTMLFrameElement_get_dispid,
    HTMLFrameElement_invoke,
    HTMLFrameElement_bind_to_tree
};

static const tid_t HTMLFrameElement_iface_tids[] = {
    HTMLELEMENT_TIDS,
    IHTMLFrameBase_tid,
    IHTMLFrameBase2_tid,
    IHTMLFrameElement3_tid,
    0
};

static dispex_static_data_t HTMLFrameElement_dispex = {
    NULL,
    DispHTMLFrameElement_tid,
    NULL,
    HTMLFrameElement_iface_tids
};

HRESULT HTMLFrameElement_Create(HTMLDocumentNode *doc, nsIDOMHTMLElement *nselem, HTMLElement **elem)
{
    HTMLFrameElement *ret;

    ret = heap_alloc_zero(sizeof(HTMLFrameElement));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->framebase.element.node.vtbl = &HTMLFrameElementImplVtbl;
    ret->IHTMLFrameElement3_iface.lpVtbl = &HTMLFrameElement3Vtbl;

    HTMLFrameBase_Init(&ret->framebase, doc, nselem, &HTMLFrameElement_dispex);

    *elem = &ret->framebase.element;
    return S_OK;
}
