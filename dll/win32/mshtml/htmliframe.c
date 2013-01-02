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
    const IHTMLIFrameElementVtbl *lpIHTMLIFrameElementVtbl;
} HTMLIFrame;

#define HTMLIFRAMEELEM(x)   ((IHTMLIFrameElement*)  &(x)->lpIHTMLIFrameElementVtbl)

#define HTMLIFRAME_THIS(iface) DEFINE_THIS(HTMLIFrame, IHTMLIFrameElement, iface)

static HRESULT WINAPI HTMLIFrameElement_QueryInterface(IHTMLIFrameElement *iface,
        REFIID riid, void **ppv)
{
    HTMLIFrame *This = HTMLIFRAME_THIS(iface);

    return IHTMLDOMNode_QueryInterface(HTMLDOMNODE(&This->framebase.element.node), riid, ppv);
}

static ULONG WINAPI HTMLIFrameElement_AddRef(IHTMLIFrameElement *iface)
{
    HTMLIFrame *This = HTMLIFRAME_THIS(iface);

    return IHTMLDOMNode_AddRef(HTMLDOMNODE(&This->framebase.element.node));
}

static ULONG WINAPI HTMLIFrameElement_Release(IHTMLIFrameElement *iface)
{
    HTMLIFrame *This = HTMLIFRAME_THIS(iface);

    return IHTMLDOMNode_Release(HTMLDOMNODE(&This->framebase.element.node));
}

static HRESULT WINAPI HTMLIFrameElement_GetTypeInfoCount(IHTMLIFrameElement *iface, UINT *pctinfo)
{
    HTMLIFrame *This = HTMLIFRAME_THIS(iface);
    return IDispatchEx_GetTypeInfoCount(DISPATCHEX(&This->framebase.element.node.dispex), pctinfo);
}

static HRESULT WINAPI HTMLIFrameElement_GetTypeInfo(IHTMLIFrameElement *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLIFrame *This = HTMLIFRAME_THIS(iface);
    return IDispatchEx_GetTypeInfo(DISPATCHEX(&This->framebase.element.node.dispex), iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLIFrameElement_GetIDsOfNames(IHTMLIFrameElement *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLIFrame *This = HTMLIFRAME_THIS(iface);
    return IDispatchEx_GetIDsOfNames(DISPATCHEX(&This->framebase.element.node.dispex), riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLIFrameElement_Invoke(IHTMLIFrameElement *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLIFrame *This = HTMLIFRAME_THIS(iface);
    return IDispatchEx_Invoke(DISPATCHEX(&This->framebase.element.node.dispex), dispIdMember, riid,
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLIFrameElement_put_vspace(IHTMLIFrameElement *iface, LONG v)
{
    HTMLIFrame *This = HTMLIFRAME_THIS(iface);
    FIXME("(%p)->(%d)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameElement_get_vspace(IHTMLIFrameElement *iface, LONG *p)
{
    HTMLIFrame *This = HTMLIFRAME_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameElement_put_hspace(IHTMLIFrameElement *iface, LONG v)
{
    HTMLIFrame *This = HTMLIFRAME_THIS(iface);
    FIXME("(%p)->(%d)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameElement_get_hspace(IHTMLIFrameElement *iface, LONG *p)
{
    HTMLIFrame *This = HTMLIFRAME_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameElement_put_align(IHTMLIFrameElement *iface, BSTR v)
{
    HTMLIFrame *This = HTMLIFRAME_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameElement_get_align(IHTMLIFrameElement *iface, BSTR *p)
{
    HTMLIFrame *This = HTMLIFRAME_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

#undef HTMLIFRAME_THIS

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

#define HTMLIFRAME_NODE_THIS(iface) DEFINE_THIS2(HTMLIFrame, framebase.element.node, iface)

static HRESULT HTMLIFrame_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLIFrame *This = HTMLIFRAME_NODE_THIS(iface);

    if(IsEqualGUID(&IID_IHTMLIFrameElement, riid)) {
        TRACE("(%p)->(IID_IHTMLIFrameElement %p)\n", This, ppv);
        *ppv = HTMLIFRAMEELEM(This);
    }else {
        return HTMLFrameBase_QI(&This->framebase, riid, ppv);
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static void HTMLIFrame_destructor(HTMLDOMNode *iface)
{
    HTMLIFrame *This = HTMLIFRAME_NODE_THIS(iface);

    HTMLFrameBase_destructor(&This->framebase);
}

static HRESULT HTMLIFrame_get_document(HTMLDOMNode *iface, IDispatch **p)
{
    HTMLIFrame *This = HTMLIFRAME_NODE_THIS(iface);

    if(!This->framebase.content_window || !This->framebase.content_window->doc) {
        *p = NULL;
        return S_OK;
    }

    *p = (IDispatch*)HTMLDOC(&This->framebase.content_window->doc->basedoc);
    IDispatch_AddRef(*p);
    return S_OK;
}

static HRESULT HTMLIFrame_get_dispid(HTMLDOMNode *iface, BSTR name,
        DWORD grfdex, DISPID *pid)
{
    HTMLIFrame *This = HTMLIFRAME_NODE_THIS(iface);

    if(!This->framebase.content_window)
        return DISP_E_UNKNOWNNAME;

    return search_window_props(This->framebase.content_window, name, grfdex, pid);
}

static HRESULT HTMLIFrame_invoke(HTMLDOMNode *iface, DISPID id, LCID lcid,
        WORD flags, DISPPARAMS *params, VARIANT *res, EXCEPINFO *ei, IServiceProvider *caller)
{
    HTMLIFrame *This = HTMLIFRAME_NODE_THIS(iface);

    if(!This->framebase.content_window) {
        ERR("no content window to invoke on\n");
        return E_FAIL;
    }

    return IDispatchEx_InvokeEx(DISPATCHEX(This->framebase.content_window), id, lcid, flags, params, res, ei, caller);
}

static HRESULT HTMLIFrame_get_readystate(HTMLDOMNode *iface, BSTR *p)
{
    HTMLIFrame *This = HTMLIFRAME_NODE_THIS(iface);

    return IHTMLFrameBase2_get_readyState(HTMLFRAMEBASE2(&This->framebase), p);
}

static HRESULT HTMLIFrame_bind_to_tree(HTMLDOMNode *iface)
{
    HTMLIFrame *This = HTMLIFRAME_NODE_THIS(iface);
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

#undef HTMLIFRAME_NODE_THIS

static const NodeImplVtbl HTMLIFrameImplVtbl = {
    HTMLIFrame_QI,
    HTMLIFrame_destructor,
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
    0
};

static dispex_static_data_t HTMLIFrame_dispex = {
    NULL,
    DispHTMLIFrame_tid,
    NULL,
    HTMLIFrame_iface_tids
};

HTMLElement *HTMLIFrame_Create(HTMLDocumentNode *doc, nsIDOMHTMLElement *nselem)
{
    HTMLIFrame *ret;

    ret = heap_alloc_zero(sizeof(HTMLIFrame));

    ret->lpIHTMLIFrameElementVtbl = &HTMLIFrameElementVtbl;
    ret->framebase.element.node.vtbl = &HTMLIFrameImplVtbl;

    HTMLFrameBase_Init(&ret->framebase, doc, nselem, &HTMLIFrame_dispex);

    return &ret->framebase.element;
}
