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
    const IHTMLFrameBase2Vtbl  *lpIHTMLFrameBase2Vtbl;

    LONG ref;

    nsIDOMHTMLIFrameElement *nsiframe;
} HTMLIFrame;

#define HTMLFRAMEBASE2(x)  (&(x)->lpIHTMLFrameBase2Vtbl)

#define HTMLFRAMEBASE2_THIS(iface) DEFINE_THIS(HTMLIFrame, IHTMLFrameBase2, iface)

static HRESULT WINAPI HTMLIFrameBase2_QueryInterface(IHTMLFrameBase2 *iface, REFIID riid, void **ppv)
{
    HTMLIFrame *This = HTMLFRAMEBASE2_THIS(iface);

    return IHTMLDOMNode_QueryInterface(HTMLDOMNODE(&This->framebase.element.node), riid, ppv);
}

static ULONG WINAPI HTMLIFrameBase2_AddRef(IHTMLFrameBase2 *iface)
{
    HTMLIFrame *This = HTMLFRAMEBASE2_THIS(iface);

    return IHTMLDOMNode_AddRef(HTMLDOMNODE(&This->framebase.element.node));
}

static ULONG WINAPI HTMLIFrameBase2_Release(IHTMLFrameBase2 *iface)
{
    HTMLIFrame *This = HTMLFRAMEBASE2_THIS(iface);

    return IHTMLDOMNode_Release(HTMLDOMNODE(&This->framebase.element.node));
}

static HRESULT WINAPI HTMLIFrameBase2_GetTypeInfoCount(IHTMLFrameBase2 *iface, UINT *pctinfo)
{
    HTMLIFrame *This = HTMLFRAMEBASE2_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameBase2_GetTypeInfo(IHTMLFrameBase2 *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLIFrame *This = HTMLFRAMEBASE2_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameBase2_GetIDsOfNames(IHTMLFrameBase2 *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLIFrame *This = HTMLFRAMEBASE2_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameBase2_Invoke(IHTMLFrameBase2 *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLIFrame *This = HTMLFRAMEBASE2_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameBase2_get_contentWindow(IHTMLFrameBase2 *iface, IHTMLWindow2 **p)
{
    HTMLIFrame *This = HTMLFRAMEBASE2_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(This->framebase.content_window) {
        IHTMLWindow2_AddRef(HTMLWINDOW2(This->framebase.content_window));
        *p = HTMLWINDOW2(This->framebase.content_window);
    }else {
        WARN("NULL content window\n");
        *p = NULL;
    }
    return S_OK;
}

static HRESULT WINAPI HTMLIFrameBase2_put_onload(IHTMLFrameBase2 *iface, VARIANT v)
{
    HTMLIFrame *This = HTMLFRAMEBASE2_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameBase2_get_onload(IHTMLFrameBase2 *iface, VARIANT *p)
{
    HTMLIFrame *This = HTMLFRAMEBASE2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameBase2_put_onreadystatechange(IHTMLFrameBase2 *iface, VARIANT v)
{
    HTMLIFrame *This = HTMLFRAMEBASE2_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameBase2_get_onreadystatechange(IHTMLFrameBase2 *iface, VARIANT *p)
{
    HTMLIFrame *This = HTMLFRAMEBASE2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameBase2_get_readyState(IHTMLFrameBase2 *iface, BSTR *p)
{
    HTMLIFrame *This = HTMLFRAMEBASE2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameBase2_put_allowTransparency(IHTMLFrameBase2 *iface, VARIANT_BOOL v)
{
    HTMLIFrame *This = HTMLFRAMEBASE2_THIS(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLIFrameBase2_get_allowTransparency(IHTMLFrameBase2 *iface, VARIANT_BOOL *p)
{
    HTMLIFrame *This = HTMLFRAMEBASE2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

#undef HTMLFRAMEBASE2_THIS

static const IHTMLFrameBase2Vtbl HTMLIFrameBase2Vtbl = {
    HTMLIFrameBase2_QueryInterface,
    HTMLIFrameBase2_AddRef,
    HTMLIFrameBase2_Release,
    HTMLIFrameBase2_GetTypeInfoCount,
    HTMLIFrameBase2_GetTypeInfo,
    HTMLIFrameBase2_GetIDsOfNames,
    HTMLIFrameBase2_Invoke,
    HTMLIFrameBase2_get_contentWindow,
    HTMLIFrameBase2_put_onload,
    HTMLIFrameBase2_get_onload,
    HTMLIFrameBase2_put_onreadystatechange,
    HTMLIFrameBase2_get_onreadystatechange,
    HTMLIFrameBase2_get_readyState,
    HTMLIFrameBase2_put_allowTransparency,
    HTMLIFrameBase2_get_allowTransparency
};

#define HTMLIFRAME_NODE_THIS(iface) DEFINE_THIS2(HTMLIFrame, framebase.element.node, iface)

static HRESULT HTMLIFrame_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLIFrame *This = HTMLIFRAME_NODE_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IHTMLFrameBase2, riid)) {
        TRACE("(%p)->(IID_IHTMLFrameBase2 %p)\n", This, ppv);
        *ppv = HTMLFRAMEBASE2(This);
    }else {
        return HTMLFrameBase_QI(&This->framebase, riid, ppv);
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static void HTMLIFrame_destructor(HTMLDOMNode *iface)
{
    HTMLIFrame *This = HTMLIFRAME_NODE_THIS(iface);

    if(This->nsiframe)
        nsIDOMHTMLIFrameElement_Release(This->nsiframe);

    HTMLFrameBase_destructor(&This->framebase);
}

#undef HTMLIFRAME_NODE_THIS

static const NodeImplVtbl HTMLIFrameImplVtbl = {
    HTMLIFrame_QI,
    HTMLIFrame_destructor
};

static const tid_t HTMLIFrame_iface_tids[] = {
    IHTMLDOMNode_tid,
    IHTMLDOMNode2_tid,
    IHTMLElement_tid,
    IHTMLElement2_tid,
    IHTMLElement3_tid,
    IHTMLFrameBase_tid,
    IHTMLFrameBase2_tid,
    0
};

static dispex_static_data_t HTMLIFrame_dispex = {
    NULL,
    DispHTMLIFrame_tid,
    NULL,
    HTMLIFrame_iface_tids
};

static HTMLWindow *get_content_window(nsIDOMHTMLIFrameElement *nsiframe)
{
    HTMLWindow *ret;
    nsIDOMWindow *nswindow;
    nsIDOMDocument *nsdoc;
    nsresult nsres;

    nsres = nsIDOMHTMLIFrameElement_GetContentDocument(nsiframe, &nsdoc);
    if(NS_FAILED(nsres)) {
        ERR("GetContentDocument failed: %08x\n", nsres);
        return NULL;
    }

    if(!nsdoc) {
        FIXME("NULL contentDocument\n");
        return NULL;
    }

    nswindow = get_nsdoc_window(nsdoc);
    nsIDOMDocument_Release(nsdoc);
    if(!nswindow)
        return NULL;

    ret = nswindow_to_window(nswindow);
    nsIDOMWindow_Release(nswindow);
    if(!ret)
        ERR("Could not get window object\n");

    return ret;
}

HTMLElement *HTMLIFrame_Create(HTMLDocumentNode *doc, nsIDOMHTMLElement *nselem, HTMLWindow *content_window)
{
    HTMLIFrame *ret;
    nsresult nsres;

    ret = heap_alloc_zero(sizeof(HTMLIFrame));

    ret->lpIHTMLFrameBase2Vtbl = &HTMLIFrameBase2Vtbl;
    ret->framebase.element.node.vtbl = &HTMLIFrameImplVtbl;

    nsres = nsIDOMHTMLElement_QueryInterface(nselem, &IID_nsIDOMHTMLIFrameElement, (void**)&ret->nsiframe);
    if(NS_FAILED(nsres))
        ERR("Could not get nsIDOMHTMLIFrameElement iface: %08x\n", nsres);

    if(!content_window)
        content_window = get_content_window(ret->nsiframe);

    HTMLFrameBase_Init(&ret->framebase, doc, nselem, content_window, &HTMLIFrame_dispex);

    return &ret->framebase.element;
}
