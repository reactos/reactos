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
    HTMLFrameBase framebase;
    IHTMLIFrameElement IHTMLIFrameElement_iface;
    IHTMLIFrameElement2 IHTMLIFrameElement2_iface;
    IHTMLIFrameElement3 IHTMLIFrameElement3_iface;
} HTMLIFrame;

static inline HTMLIFrame *impl_from_IHTMLIFrameElement(IHTMLIFrameElement *iface)
{
    return CONTAINING_RECORD(iface, HTMLIFrame, IHTMLIFrameElement_iface);
}

static HRESULT WINAPI HTMLIFrameElement_QueryInterface(IHTMLIFrameElement *iface,
        REFIID riid, void **ppv)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement(iface);

    return IHTMLDOMNode_QueryInterface(&This->framebase.element.node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI HTMLIFrameElement_AddRef(IHTMLIFrameElement *iface)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement(iface);

    return IHTMLDOMNode_AddRef(&This->framebase.element.node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLIFrameElement_Release(IHTMLIFrameElement *iface)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement(iface);

    return IHTMLDOMNode_Release(&This->framebase.element.node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLIFrameElement_GetTypeInfoCount(IHTMLIFrameElement *iface, UINT *pctinfo)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement(iface);
    return IDispatchEx_GetTypeInfoCount(&This->framebase.element.node.dispex.IDispatchEx_iface,
            pctinfo);
}

static HRESULT WINAPI HTMLIFrameElement_GetTypeInfo(IHTMLIFrameElement *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement(iface);
    return IDispatchEx_GetTypeInfo(&This->framebase.element.node.dispex.IDispatchEx_iface, iTInfo,
            lcid, ppTInfo);
}

static HRESULT WINAPI HTMLIFrameElement_GetIDsOfNames(IHTMLIFrameElement *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement(iface);
    return IDispatchEx_GetIDsOfNames(&This->framebase.element.node.dispex.IDispatchEx_iface, riid,
            rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLIFrameElement_Invoke(IHTMLIFrameElement *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement(iface);
    return IDispatchEx_Invoke(&This->framebase.element.node.dispex.IDispatchEx_iface, dispIdMember,
            riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLIFrameElement_put_vspace(IHTMLIFrameElement *iface, LONG v)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement(iface);
    FIXME("(%p)->(%d)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameElement_get_vspace(IHTMLIFrameElement *iface, LONG *p)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameElement_put_hspace(IHTMLIFrameElement *iface, LONG v)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement(iface);
    FIXME("(%p)->(%d)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameElement_get_hspace(IHTMLIFrameElement *iface, LONG *p)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameElement_put_align(IHTMLIFrameElement *iface, BSTR v)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameElement_get_align(IHTMLIFrameElement *iface, BSTR *p)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLIFrameElementVtbl HTMLIFrameElementVtbl = {
    HTMLIFrameElement_QueryInterface,
    HTMLIFrameElement_AddRef,
    HTMLIFrameElement_Release,
    HTMLIFrameElement_GetTypeInfoCount,
    HTMLIFrameElement_GetTypeInfo,
    HTMLIFrameElement_GetIDsOfNames,
    HTMLIFrameElement_Invoke,
    HTMLIFrameElement_put_vspace,
    HTMLIFrameElement_get_vspace,
    HTMLIFrameElement_put_hspace,
    HTMLIFrameElement_get_hspace,
    HTMLIFrameElement_put_align,
    HTMLIFrameElement_get_align
};

static inline HTMLIFrame *impl_from_IHTMLIFrameElement2(IHTMLIFrameElement2 *iface)
{
    return CONTAINING_RECORD(iface, HTMLIFrame, IHTMLIFrameElement2_iface);
}

static HRESULT WINAPI HTMLIFrameElement2_QueryInterface(IHTMLIFrameElement2 *iface,
        REFIID riid, void **ppv)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement2(iface);

    return IHTMLDOMNode_QueryInterface(&This->framebase.element.node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI HTMLIFrameElement2_AddRef(IHTMLIFrameElement2 *iface)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement2(iface);

    return IHTMLDOMNode_AddRef(&This->framebase.element.node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLIFrameElement2_Release(IHTMLIFrameElement2 *iface)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement2(iface);

    return IHTMLDOMNode_Release(&This->framebase.element.node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLIFrameElement2_GetTypeInfoCount(IHTMLIFrameElement2 *iface, UINT *pctinfo)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement2(iface);
    return IDispatchEx_GetTypeInfoCount(&This->framebase.element.node.dispex.IDispatchEx_iface,
            pctinfo);
}

static HRESULT WINAPI HTMLIFrameElement2_GetTypeInfo(IHTMLIFrameElement2 *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement2(iface);
    return IDispatchEx_GetTypeInfo(&This->framebase.element.node.dispex.IDispatchEx_iface, iTInfo,
            lcid, ppTInfo);
}

static HRESULT WINAPI HTMLIFrameElement2_GetIDsOfNames(IHTMLIFrameElement2 *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement2(iface);
    return IDispatchEx_GetIDsOfNames(&This->framebase.element.node.dispex.IDispatchEx_iface, riid,
            rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLIFrameElement2_Invoke(IHTMLIFrameElement2 *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement2(iface);
    return IDispatchEx_Invoke(&This->framebase.element.node.dispex.IDispatchEx_iface, dispIdMember,
            riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLIFrameElement2_put_height(IHTMLIFrameElement2 *iface, VARIANT v)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement2(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    if(V_VT(&v) != VT_BSTR) {
        FIXME("Unsupported %s\n", debugstr_variant(&v));
        return E_NOTIMPL;
    }

    nsAString_InitDepend(&nsstr, V_BSTR(&v));
    nsres = nsIDOMHTMLIFrameElement_SetHeight(This->framebase.nsiframe, &nsstr);
    nsAString_Finish(&nsstr);
    if(NS_FAILED(nsres)) {
        ERR("SetHeight failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLIFrameElement2_get_height(IHTMLIFrameElement2 *iface, VARIANT *p)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement2(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&nsstr, NULL);
    nsres = nsIDOMHTMLIFrameElement_GetHeight(This->framebase.nsiframe, &nsstr);

    V_VT(p) = VT_BSTR;
    return return_nsstr(nsres, &nsstr, &V_BSTR(p));
}

static HRESULT WINAPI HTMLIFrameElement2_put_width(IHTMLIFrameElement2 *iface, VARIANT v)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement2(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    if(V_VT(&v) != VT_BSTR) {
        FIXME("Unsupported %s\n", debugstr_variant(&v));
        return E_NOTIMPL;
    }

    nsAString_InitDepend(&nsstr, V_BSTR(&v));
    nsres = nsIDOMHTMLIFrameElement_SetWidth(This->framebase.nsiframe, &nsstr);
    nsAString_Finish(&nsstr);
    if(NS_FAILED(nsres)) {
        ERR("SetWidth failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLIFrameElement2_get_width(IHTMLIFrameElement2 *iface, VARIANT *p)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement2(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&nsstr, NULL);
    nsres = nsIDOMHTMLIFrameElement_GetWidth(This->framebase.nsiframe, &nsstr);

    V_VT(p) = VT_BSTR;
    return return_nsstr(nsres, &nsstr, &V_BSTR(p));
}

static const IHTMLIFrameElement2Vtbl HTMLIFrameElement2Vtbl = {
    HTMLIFrameElement2_QueryInterface,
    HTMLIFrameElement2_AddRef,
    HTMLIFrameElement2_Release,
    HTMLIFrameElement2_GetTypeInfoCount,
    HTMLIFrameElement2_GetTypeInfo,
    HTMLIFrameElement2_GetIDsOfNames,
    HTMLIFrameElement2_Invoke,
    HTMLIFrameElement2_put_height,
    HTMLIFrameElement2_get_height,
    HTMLIFrameElement2_put_width,
    HTMLIFrameElement2_get_width
};

static inline HTMLIFrame *impl_from_IHTMLIFrameElement3(IHTMLIFrameElement3 *iface)
{
    return CONTAINING_RECORD(iface, HTMLIFrame, IHTMLIFrameElement3_iface);
}

static HRESULT WINAPI HTMLIFrameElement3_QueryInterface(IHTMLIFrameElement3 *iface,
        REFIID riid, void **ppv)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement3(iface);

    return IHTMLDOMNode_QueryInterface(&This->framebase.element.node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI HTMLIFrameElement3_AddRef(IHTMLIFrameElement3 *iface)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement3(iface);

    return IHTMLDOMNode_AddRef(&This->framebase.element.node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLIFrameElement3_Release(IHTMLIFrameElement3 *iface)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement3(iface);

    return IHTMLDOMNode_Release(&This->framebase.element.node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLIFrameElement3_GetTypeInfoCount(IHTMLIFrameElement3 *iface, UINT *pctinfo)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement3(iface);
    return IDispatchEx_GetTypeInfoCount(&This->framebase.element.node.dispex.IDispatchEx_iface,
            pctinfo);
}

static HRESULT WINAPI HTMLIFrameElement3_GetTypeInfo(IHTMLIFrameElement3 *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement3(iface);
    return IDispatchEx_GetTypeInfo(&This->framebase.element.node.dispex.IDispatchEx_iface, iTInfo,
            lcid, ppTInfo);
}

static HRESULT WINAPI HTMLIFrameElement3_GetIDsOfNames(IHTMLIFrameElement3 *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement3(iface);
    return IDispatchEx_GetIDsOfNames(&This->framebase.element.node.dispex.IDispatchEx_iface, riid,
            rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLIFrameElement3_Invoke(IHTMLIFrameElement3 *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement3(iface);
    return IDispatchEx_Invoke(&This->framebase.element.node.dispex.IDispatchEx_iface, dispIdMember,
            riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLIFrameElement3_get_contentDocument(IHTMLIFrameElement3 *iface, IDispatch **p)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement3(iface);
    IHTMLDocument2 *doc;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->framebase.content_window) {
        *p = NULL;
        return S_OK;
    }

    hres = IHTMLWindow2_get_document(&This->framebase.content_window->base.IHTMLWindow2_iface, &doc);
    *p = (IDispatch*)doc;
    return hres;
}

static HRESULT WINAPI HTMLIFrameElement3_put_src(IHTMLIFrameElement3 *iface, BSTR v)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameElement3_get_src(IHTMLIFrameElement3 *iface, BSTR *p)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameElement3_put_longDesc(IHTMLIFrameElement3 *iface, BSTR v)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameElement3_get_longDesc(IHTMLIFrameElement3 *iface, BSTR *p)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameElement3_put_frameBorder(IHTMLIFrameElement3 *iface, BSTR v)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameElement3_get_frameBorder(IHTMLIFrameElement3 *iface, BSTR *p)
{
    HTMLIFrame *This = impl_from_IHTMLIFrameElement3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLIFrameElement3Vtbl HTMLIFrameElement3Vtbl = {
    HTMLIFrameElement3_QueryInterface,
    HTMLIFrameElement3_AddRef,
    HTMLIFrameElement3_Release,
    HTMLIFrameElement3_GetTypeInfoCount,
    HTMLIFrameElement3_GetTypeInfo,
    HTMLIFrameElement3_GetIDsOfNames,
    HTMLIFrameElement3_Invoke,
    HTMLIFrameElement3_get_contentDocument,
    HTMLIFrameElement3_put_src,
    HTMLIFrameElement3_get_src,
    HTMLIFrameElement3_put_longDesc,
    HTMLIFrameElement3_get_longDesc,
    HTMLIFrameElement3_put_frameBorder,
    HTMLIFrameElement3_get_frameBorder
};

static inline HTMLIFrame *impl_from_HTMLDOMNode(HTMLDOMNode *iface)
{
    return CONTAINING_RECORD(iface, HTMLIFrame, framebase.element.node);
}

static HRESULT HTMLIFrame_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLIFrame *This = impl_from_HTMLDOMNode(iface);

    if(IsEqualGUID(&IID_IHTMLIFrameElement, riid)) {
        TRACE("(%p)->(IID_IHTMLIFrameElement %p)\n", This, ppv);
        *ppv = &This->IHTMLIFrameElement_iface;
    }else if(IsEqualGUID(&IID_IHTMLIFrameElement2, riid)) {
        TRACE("(%p)->(IID_IHTMLIFrameElement2 %p)\n", This, ppv);
        *ppv = &This->IHTMLIFrameElement2_iface;
    }else if(IsEqualGUID(&IID_IHTMLIFrameElement3, riid)) {
        TRACE("(%p)->(IID_IHTMLIFrameElement3 %p)\n", This, ppv);
        *ppv = &This->IHTMLIFrameElement3_iface;
    }else {
        return HTMLFrameBase_QI(&This->framebase, riid, ppv);
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static void HTMLIFrame_destructor(HTMLDOMNode *iface)
{
    HTMLIFrame *This = impl_from_HTMLDOMNode(iface);

    HTMLFrameBase_destructor(&This->framebase);
}

static HRESULT HTMLIFrame_get_document(HTMLDOMNode *iface, IDispatch **p)
{
    HTMLIFrame *This = impl_from_HTMLDOMNode(iface);

    if(!This->framebase.content_window || !This->framebase.content_window->base.inner_window->doc) {
        *p = NULL;
        return S_OK;
    }

    *p = (IDispatch*)&This->framebase.content_window->base.inner_window->doc->basedoc.IHTMLDocument2_iface;
    IDispatch_AddRef(*p);
    return S_OK;
}

static HRESULT HTMLIFrame_get_dispid(HTMLDOMNode *iface, BSTR name,
        DWORD grfdex, DISPID *pid)
{
    HTMLIFrame *This = impl_from_HTMLDOMNode(iface);

    if(!This->framebase.content_window)
        return DISP_E_UNKNOWNNAME;

    return search_window_props(This->framebase.content_window->base.inner_window, name, grfdex, pid);
}

static HRESULT HTMLIFrame_invoke(HTMLDOMNode *iface, DISPID id, LCID lcid,
        WORD flags, DISPPARAMS *params, VARIANT *res, EXCEPINFO *ei, IServiceProvider *caller)
{
    HTMLIFrame *This = impl_from_HTMLDOMNode(iface);

    if(!This->framebase.content_window) {
        ERR("no content window to invoke on\n");
        return E_FAIL;
    }

    return IDispatchEx_InvokeEx(&This->framebase.content_window->base.IDispatchEx_iface, id, lcid,
            flags, params, res, ei, caller);
}

static HRESULT HTMLIFrame_get_readystate(HTMLDOMNode *iface, BSTR *p)
{
    HTMLIFrame *This = impl_from_HTMLDOMNode(iface);

    return IHTMLFrameBase2_get_readyState(&This->framebase.IHTMLFrameBase2_iface, p);
}

static HRESULT HTMLIFrame_bind_to_tree(HTMLDOMNode *iface)
{
    HTMLIFrame *This = impl_from_HTMLDOMNode(iface);
    nsIDOMDocument *nsdoc;
    nsresult nsres;
    HRESULT hres;

    nsres = nsIDOMHTMLIFrameElement_GetContentDocument(This->framebase.nsiframe, &nsdoc);
    if(NS_FAILED(nsres) || !nsdoc) {
        ERR("GetContentDocument failed: %08x\n", nsres);
        return E_FAIL;
    }

    hres = set_frame_doc(&This->framebase, nsdoc);
    nsIDOMDocument_Release(nsdoc);
    return hres;
}

static const NodeImplVtbl HTMLIFrameImplVtbl = {
    HTMLIFrame_QI,
    HTMLIFrame_destructor,
    HTMLElement_cpc,
    HTMLElement_clone,
    HTMLElement_handle_event,
    HTMLElement_get_attr_col,
    NULL,
    NULL,
    NULL,
    NULL,
    HTMLIFrame_get_document,
    HTMLIFrame_get_readystate,
    HTMLIFrame_get_dispid,
    HTMLIFrame_invoke,
    HTMLIFrame_bind_to_tree
};

static const tid_t HTMLIFrame_iface_tids[] = {
    HTMLELEMENT_TIDS,
    IHTMLFrameBase_tid,
    IHTMLFrameBase2_tid,
    IHTMLIFrameElement_tid,
    IHTMLIFrameElement2_tid,
    IHTMLIFrameElement3_tid,
    0
};

static dispex_static_data_t HTMLIFrame_dispex = {
    NULL,
    DispHTMLIFrame_tid,
    NULL,
    HTMLIFrame_iface_tids
};

HRESULT HTMLIFrame_Create(HTMLDocumentNode *doc, nsIDOMHTMLElement *nselem, HTMLElement **elem)
{
    HTMLIFrame *ret;

    ret = heap_alloc_zero(sizeof(HTMLIFrame));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IHTMLIFrameElement_iface.lpVtbl = &HTMLIFrameElementVtbl;
    ret->IHTMLIFrameElement2_iface.lpVtbl = &HTMLIFrameElement2Vtbl;
    ret->IHTMLIFrameElement3_iface.lpVtbl = &HTMLIFrameElement3Vtbl;
    ret->framebase.element.node.vtbl = &HTMLIFrameImplVtbl;

    HTMLFrameBase_Init(&ret->framebase, doc, nselem, &HTMLIFrame_dispex);

    *elem = &ret->framebase.element;
    return S_OK;
}
