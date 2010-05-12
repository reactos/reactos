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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"

#include "mshtml_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

typedef struct {
    HTMLFrameBase framebase;
    const IHTMLFrameElement3Vtbl  *lpIHTMLFrameElement3Vtbl;
} HTMLFrameElement;

#define HTMLFRAMEELEM3(x)   ((IHTMLFrameElement3*)  &(x)->lpIHTMLFrameElement3Vtbl)

#define HTMLFRAME3_THIS(iface) DEFINE_THIS(HTMLFrameElement, IHTMLFrameElement3, iface)

static HRESULT WINAPI HTMLFrameElement3_QueryInterface(IHTMLFrameElement3 *iface,
        REFIID riid, void **ppv)
{
    HTMLFrameElement *This = HTMLFRAME3_THIS(iface);

    return IHTMLDOMNode_QueryInterface(HTMLDOMNODE(&This->framebase.element.node), riid, ppv);
}

static ULONG WINAPI HTMLFrameElement3_AddRef(IHTMLFrameElement3 *iface)
{
    HTMLFrameElement *This = HTMLFRAME3_THIS(iface);

    return IHTMLDOMNode_AddRef(HTMLDOMNODE(&This->framebase.element.node));
}

static ULONG WINAPI HTMLFrameElement3_Release(IHTMLFrameElement3 *iface)
{
    HTMLFrameElement *This = HTMLFRAME3_THIS(iface);

    return IHTMLDOMNode_Release(HTMLDOMNODE(&This->framebase.element.node));
}

static HRESULT WINAPI HTMLFrameElement3_GetTypeInfoCount(IHTMLFrameElement3 *iface, UINT *pctinfo)
{
    HTMLFrameElement *This = HTMLFRAME3_THIS(iface);
    return IDispatchEx_GetTypeInfoCount(DISPATCHEX(&This->framebase.element.node.dispex), pctinfo);
}

static HRESULT WINAPI HTMLFrameElement3_GetTypeInfo(IHTMLFrameElement3 *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLFrameElement *This = HTMLFRAME3_THIS(iface);
    return IDispatchEx_GetTypeInfo(DISPATCHEX(&This->framebase.element.node.dispex), iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLFrameElement3_GetIDsOfNames(IHTMLFrameElement3 *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLFrameElement *This = HTMLFRAME3_THIS(iface);
    return IDispatchEx_GetIDsOfNames(DISPATCHEX(&This->framebase.element.node.dispex), riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLFrameElement3_Invoke(IHTMLFrameElement3 *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLFrameElement *This = HTMLFRAME3_THIS(iface);
    return IDispatchEx_Invoke(DISPATCHEX(&This->framebase.element.node.dispex), dispIdMember, riid,
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLFrameElement3_get_contentDocument(IHTMLFrameElement3 *iface, IDispatch **p)
{
    HTMLFrameElement *This = HTMLFRAME3_THIS(iface);
    IHTMLDocument2 *doc;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->framebase.content_window) {
        FIXME("NULL window\n");
        return E_FAIL;
    }

    hres = IHTMLWindow2_get_document(HTMLWINDOW2(This->framebase.content_window), &doc);
    if(FAILED(hres))
        return hres;

    *p = doc ? (IDispatch*)doc : NULL;
    return S_OK;
}

static HRESULT WINAPI HTMLFrameElement3_put_src(IHTMLFrameElement3 *iface, BSTR v)
{
    HTMLFrameElement *This = HTMLFRAME3_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameElement3_get_src(IHTMLFrameElement3 *iface, BSTR *p)
{
    HTMLFrameElement *This = HTMLFRAME3_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameElement3_put_longDesc(IHTMLFrameElement3 *iface, BSTR v)
{
    HTMLFrameElement *This = HTMLFRAME3_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameElement3_get_longDesc(IHTMLFrameElement3 *iface, BSTR *p)
{
    HTMLFrameElement *This = HTMLFRAME3_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameElement3_put_frameBorder(IHTMLFrameElement3 *iface, BSTR v)
{
    HTMLFrameElement *This = HTMLFRAME3_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameElement3_get_frameBorder(IHTMLFrameElement3 *iface, BSTR *p)
{
    HTMLFrameElement *This = HTMLFRAME3_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

#undef HTMLFRAME3_THIS

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

#define HTMLFRAME_NODE_THIS(iface) DEFINE_THIS2(HTMLFrameElement, framebase.element.node, iface)

static HRESULT HTMLFrameElement_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLFrameElement *This = HTMLFRAME_NODE_THIS(iface);

    if(IsEqualGUID(&IID_IHTMLFrameElement3, riid)) {
        TRACE("(%p)->(IID_IHTMLFrameElement3 %p)\n", This, ppv);
        *ppv = HTMLFRAMEELEM3(This);
    }else {
        return HTMLFrameBase_QI(&This->framebase, riid, ppv);
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static void HTMLFrameElement_destructor(HTMLDOMNode *iface)
{
    HTMLFrameElement *This = HTMLFRAME_NODE_THIS(iface);

    HTMLFrameBase_destructor(&This->framebase);
}

static HRESULT HTMLFrameElement_get_document(HTMLDOMNode *iface, IDispatch **p)
{
    HTMLFrameElement *This = HTMLFRAME_NODE_THIS(iface);

    if(!This->framebase.content_window || !This->framebase.content_window->doc) {
        *p = NULL;
        return S_OK;
    }

    *p = (IDispatch*)HTMLDOC(&This->framebase.content_window->doc->basedoc);
    IDispatch_AddRef(*p);
    return S_OK;
}

static HRESULT HTMLFrameElement_get_dispid(HTMLDOMNode *iface, BSTR name,
        DWORD grfdex, DISPID *pid)
{
    HTMLFrameElement *This = HTMLFRAME_NODE_THIS(iface);

    if(!This->framebase.content_window)
        return DISP_E_UNKNOWNNAME;

    return search_window_props(This->framebase.content_window, name, grfdex, pid);
}

static HRESULT HTMLFrameElement_invoke(HTMLDOMNode *iface, DISPID id, LCID lcid,
        WORD flags, DISPPARAMS *params, VARIANT *res, EXCEPINFO *ei, IServiceProvider *caller)
{
    HTMLFrameElement *This = HTMLFRAME_NODE_THIS(iface);

    if(!This->framebase.content_window) {
        ERR("no content window to invoke on\n");
        return E_FAIL;
    }

    return IDispatchEx_InvokeEx(DISPATCHEX(This->framebase.content_window), id, lcid, flags, params, res, ei, caller);
}

static HRESULT HTMLFrameElement_bind_to_tree(HTMLDOMNode *iface)
{
    HTMLFrameElement *This = HTMLFRAME_NODE_THIS(iface);
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

#undef HTMLFRAME_NODE_THIS

static const NodeImplVtbl HTMLFrameElementImplVtbl = {
    HTMLFrameElement_QI,
    HTMLFrameElement_destructor,
    NULL,
    NULL,
    NULL,
    NULL,
    HTMLFrameElement_get_document,
    NULL,
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

HTMLElement *HTMLFrameElement_Create(HTMLDocumentNode *doc, nsIDOMHTMLElement *nselem)
{
    HTMLFrameElement *ret;

    ret = heap_alloc_zero(sizeof(HTMLFrameElement));

    ret->framebase.element.node.vtbl = &HTMLFrameElementImplVtbl;
    ret->lpIHTMLFrameElement3Vtbl = &HTMLFrameElement3Vtbl;

    HTMLFrameBase_Init(&ret->framebase, doc, nselem, &HTMLFrameElement_dispex);

    return &ret->framebase.element;
}
