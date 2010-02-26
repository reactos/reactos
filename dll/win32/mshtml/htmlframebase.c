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

static const WCHAR autoW[] = {'a','u','t','o',0};
static const WCHAR yesW[] = {'y','e','s',0};
static const WCHAR noW[] = {'n','o',0};

HRESULT set_frame_doc(HTMLFrameBase *frame, nsIDOMDocument *nsdoc)
{
    nsIDOMWindow *nswindow;
    HTMLWindow *window;
    HRESULT hres = S_OK;

    if(frame->content_window)
        return S_OK;

    nswindow = get_nsdoc_window(nsdoc);
    if(!nswindow)
        return E_FAIL;

    window = nswindow_to_window(nswindow);
    if(!window)
        hres = HTMLWindow_Create(frame->element.node.doc->basedoc.doc_obj, nswindow,
                frame->element.node.doc->basedoc.window, &window);
    nsIDOMWindow_Release(nswindow);
    if(FAILED(hres))
        return hres;

    frame->content_window = window;
    window->frame_element = frame;
    return S_OK;
}

#define HTMLFRAMEBASE_THIS(iface) DEFINE_THIS(HTMLFrameBase, IHTMLFrameBase, iface)

static HRESULT WINAPI HTMLFrameBase_QueryInterface(IHTMLFrameBase *iface, REFIID riid, void **ppv)
{
    HTMLFrameBase *This = HTMLFRAMEBASE_THIS(iface);

    return IHTMLDOMNode_QueryInterface(HTMLDOMNODE(&This->element.node), riid, ppv);
}

static ULONG WINAPI HTMLFrameBase_AddRef(IHTMLFrameBase *iface)
{
    HTMLFrameBase *This = HTMLFRAMEBASE_THIS(iface);

    return IHTMLDOMNode_AddRef(HTMLDOMNODE(&This->element.node));
}

static ULONG WINAPI HTMLFrameBase_Release(IHTMLFrameBase *iface)
{
    HTMLFrameBase *This = HTMLFRAMEBASE_THIS(iface);

    return IHTMLDOMNode_Release(HTMLDOMNODE(&This->element.node));
}

static HRESULT WINAPI HTMLFrameBase_GetTypeInfoCount(IHTMLFrameBase *iface, UINT *pctinfo)
{
    HTMLFrameBase *This = HTMLFRAMEBASE_THIS(iface);

    return IDispatchEx_GetTypeInfoCount(DISPATCHEX(&This->element.node.dispex), pctinfo);
}

static HRESULT WINAPI HTMLFrameBase_GetTypeInfo(IHTMLFrameBase *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLFrameBase *This = HTMLFRAMEBASE_THIS(iface);

    return IDispatchEx_GetTypeInfo(DISPATCHEX(&This->element.node.dispex), iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLFrameBase_GetIDsOfNames(IHTMLFrameBase *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLFrameBase *This = HTMLFRAMEBASE_THIS(iface);

    return IDispatchEx_GetIDsOfNames(DISPATCHEX(&This->element.node.dispex), riid, rgszNames,
            cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLFrameBase_Invoke(IHTMLFrameBase *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLFrameBase *This = HTMLFRAMEBASE_THIS(iface);

    return IDispatchEx_Invoke(DISPATCHEX(&This->element.node.dispex), dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLFrameBase_put_src(IHTMLFrameBase *iface, BSTR v)
{
    HTMLFrameBase *This = HTMLFRAMEBASE_THIS(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    if(!This->content_window || !This->element.node.doc || !This->element.node.doc->basedoc.window) {
        FIXME("detached element\n");
        return E_FAIL;
    }

    return navigate_url(This->content_window, v, This->element.node.doc->basedoc.window->url);
}

static HRESULT WINAPI HTMLFrameBase_get_src(IHTMLFrameBase *iface, BSTR *p)
{
    HTMLFrameBase *This = HTMLFRAMEBASE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameBase_put_name(IHTMLFrameBase *iface, BSTR v)
{
    HTMLFrameBase *This = HTMLFRAMEBASE_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameBase_get_name(IHTMLFrameBase *iface, BSTR *p)
{
    HTMLFrameBase *This = HTMLFRAMEBASE_THIS(iface);
    nsAString nsstr;
    const PRUnichar *strdata;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->nsframe) {
        nsAString_Init(&nsstr, NULL);
        nsres = nsIDOMHTMLFrameElement_GetName(This->nsframe, &nsstr);
    }else if(This->nsiframe) {
        nsAString_Init(&nsstr, NULL);
        nsres = nsIDOMHTMLIFrameElement_GetName(This->nsiframe, &nsstr);
    }else {
        ERR("No attached ns frame object\n");
        return E_UNEXPECTED;
    }

    if(NS_FAILED(nsres)) {
        ERR("GetName failed: 0x%08x\n", nsres);
        nsAString_Finish(&nsstr);
        return E_FAIL;
    }

    nsAString_GetData(&nsstr, &strdata);
    if(*strdata) {
        *p = SysAllocString(strdata);
        if(!*p) {
            nsAString_Finish(&nsstr);
            return E_OUTOFMEMORY;
        }
    }else
        *p = NULL;

    nsAString_Finish(&nsstr);

    return S_OK;
}

static HRESULT WINAPI HTMLFrameBase_put_border(IHTMLFrameBase *iface, VARIANT v)
{
    HTMLFrameBase *This = HTMLFRAMEBASE_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameBase_get_border(IHTMLFrameBase *iface, VARIANT *p)
{
    HTMLFrameBase *This = HTMLFRAMEBASE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameBase_put_frameBorder(IHTMLFrameBase *iface, BSTR v)
{
    HTMLFrameBase *This = HTMLFRAMEBASE_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameBase_get_frameBorder(IHTMLFrameBase *iface, BSTR *p)
{
    HTMLFrameBase *This = HTMLFRAMEBASE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameBase_put_frameSpacing(IHTMLFrameBase *iface, VARIANT v)
{
    HTMLFrameBase *This = HTMLFRAMEBASE_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameBase_get_frameSpacing(IHTMLFrameBase *iface, VARIANT *p)
{
    HTMLFrameBase *This = HTMLFRAMEBASE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameBase_put_marginWidth(IHTMLFrameBase *iface, VARIANT v)
{
    HTMLFrameBase *This = HTMLFRAMEBASE_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameBase_get_marginWidth(IHTMLFrameBase *iface, VARIANT *p)
{
    HTMLFrameBase *This = HTMLFRAMEBASE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameBase_put_marginHeight(IHTMLFrameBase *iface, VARIANT v)
{
    HTMLFrameBase *This = HTMLFRAMEBASE_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameBase_get_marginHeight(IHTMLFrameBase *iface, VARIANT *p)
{
    HTMLFrameBase *This = HTMLFRAMEBASE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameBase_put_noResize(IHTMLFrameBase *iface, VARIANT_BOOL v)
{
    HTMLFrameBase *This = HTMLFRAMEBASE_THIS(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameBase_get_noResize(IHTMLFrameBase *iface, VARIANT_BOOL *p)
{
    HTMLFrameBase *This = HTMLFRAMEBASE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameBase_put_scrolling(IHTMLFrameBase *iface, BSTR v)
{
    HTMLFrameBase *This = HTMLFRAMEBASE_THIS(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    if(!(!strcmpiW(v, yesW) || !strcmpiW(v, noW) || !strcmpiW(v, autoW)))
        return E_INVALIDARG;

    if(This->nsframe) {
        nsAString_Init(&nsstr, v);
        nsres = nsIDOMHTMLFrameElement_SetScrolling(This->nsframe, &nsstr);
    }else if(This->nsiframe) {
        nsAString_Init(&nsstr, v);
        nsres = nsIDOMHTMLIFrameElement_SetScrolling(This->nsiframe, &nsstr);
    }else {
        ERR("No attached ns frame object\n");
        return E_UNEXPECTED;
    }
    nsAString_Finish(&nsstr);

    if(NS_FAILED(nsres)) {
        ERR("SetScrolling failed: 0x%08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLFrameBase_get_scrolling(IHTMLFrameBase *iface, BSTR *p)
{
    HTMLFrameBase *This = HTMLFRAMEBASE_THIS(iface);
    nsAString nsstr;
    const PRUnichar *strdata;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->nsframe) {
        nsAString_Init(&nsstr, NULL);
        nsres = nsIDOMHTMLFrameElement_GetScrolling(This->nsframe, &nsstr);
    }else if(This->nsiframe) {
        nsAString_Init(&nsstr, NULL);
        nsres = nsIDOMHTMLIFrameElement_GetScrolling(This->nsiframe, &nsstr);
    }else {
        ERR("No attached ns frame object\n");
        return E_UNEXPECTED;
    }

    if(NS_FAILED(nsres)) {
        ERR("GetScrolling failed: 0x%08x\n", nsres);
        nsAString_Finish(&nsstr);
        return E_FAIL;
    }

    nsAString_GetData(&nsstr, &strdata);

    if(*strdata)
        *p = SysAllocString(strdata);
    else
        *p = SysAllocString(autoW);

    nsAString_Finish(&nsstr);

    return *p ? S_OK : E_OUTOFMEMORY;
}

static const IHTMLFrameBaseVtbl HTMLFrameBaseVtbl = {
    HTMLFrameBase_QueryInterface,
    HTMLFrameBase_AddRef,
    HTMLFrameBase_Release,
    HTMLFrameBase_GetTypeInfoCount,
    HTMLFrameBase_GetTypeInfo,
    HTMLFrameBase_GetIDsOfNames,
    HTMLFrameBase_Invoke,
    HTMLFrameBase_put_src,
    HTMLFrameBase_get_src,
    HTMLFrameBase_put_name,
    HTMLFrameBase_get_name,
    HTMLFrameBase_put_border,
    HTMLFrameBase_get_border,
    HTMLFrameBase_put_frameBorder,
    HTMLFrameBase_get_frameBorder,
    HTMLFrameBase_put_frameSpacing,
    HTMLFrameBase_get_frameSpacing,
    HTMLFrameBase_put_marginWidth,
    HTMLFrameBase_get_marginWidth,
    HTMLFrameBase_put_marginHeight,
    HTMLFrameBase_get_marginHeight,
    HTMLFrameBase_put_noResize,
    HTMLFrameBase_get_noResize,
    HTMLFrameBase_put_scrolling,
    HTMLFrameBase_get_scrolling
};

#define HTMLFRAMEBASE2_THIS(iface) DEFINE_THIS(HTMLFrameBase, IHTMLFrameBase2, iface)

static HRESULT WINAPI HTMLFrameBase2_QueryInterface(IHTMLFrameBase2 *iface, REFIID riid, void **ppv)
{
    HTMLFrameBase *This = HTMLFRAMEBASE2_THIS(iface);

    return IHTMLDOMNode_QueryInterface(HTMLDOMNODE(&This->element.node), riid, ppv);
}

static ULONG WINAPI HTMLFrameBase2_AddRef(IHTMLFrameBase2 *iface)
{
    HTMLFrameBase *This = HTMLFRAMEBASE2_THIS(iface);

    return IHTMLDOMNode_AddRef(HTMLDOMNODE(&This->element.node));
}

static ULONG WINAPI HTMLFrameBase2_Release(IHTMLFrameBase2 *iface)
{
    HTMLFrameBase *This = HTMLFRAMEBASE2_THIS(iface);

    return IHTMLDOMNode_Release(HTMLDOMNODE(&This->element.node));
}

static HRESULT WINAPI HTMLFrameBase2_GetTypeInfoCount(IHTMLFrameBase2 *iface, UINT *pctinfo)
{
    HTMLFrameBase *This = HTMLFRAMEBASE2_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameBase2_GetTypeInfo(IHTMLFrameBase2 *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLFrameBase *This = HTMLFRAMEBASE2_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameBase2_GetIDsOfNames(IHTMLFrameBase2 *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLFrameBase *This = HTMLFRAMEBASE2_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameBase2_Invoke(IHTMLFrameBase2 *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLFrameBase *This = HTMLFRAMEBASE2_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameBase2_get_contentWindow(IHTMLFrameBase2 *iface, IHTMLWindow2 **p)
{
    HTMLFrameBase *This = HTMLFRAMEBASE2_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(This->content_window) {
        IHTMLWindow2_AddRef(HTMLWINDOW2(This->content_window));
        *p = HTMLWINDOW2(This->content_window);
    }else {
        WARN("NULL content window\n");
        *p = NULL;
    }
    return S_OK;
}

static HRESULT WINAPI HTMLFrameBase2_put_onload(IHTMLFrameBase2 *iface, VARIANT v)
{
    HTMLFrameBase *This = HTMLFRAMEBASE2_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameBase2_get_onload(IHTMLFrameBase2 *iface, VARIANT *p)
{
    HTMLFrameBase *This = HTMLFRAMEBASE2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameBase2_put_onreadystatechange(IHTMLFrameBase2 *iface, VARIANT v)
{
    HTMLFrameBase *This = HTMLFRAMEBASE2_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameBase2_get_onreadystatechange(IHTMLFrameBase2 *iface, VARIANT *p)
{
    HTMLFrameBase *This = HTMLFRAMEBASE2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameBase2_get_readyState(IHTMLFrameBase2 *iface, BSTR *p)
{
    HTMLFrameBase *This = HTMLFRAMEBASE2_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->content_window || !This->content_window->doc) {
        FIXME("no document associated\n");
        return E_FAIL;
    }

    return IHTMLDocument2_get_readyState(HTMLDOC(&This->content_window->doc->basedoc), p);
}

static HRESULT WINAPI HTMLFrameBase2_put_allowTransparency(IHTMLFrameBase2 *iface, VARIANT_BOOL v)
{
    HTMLFrameBase *This = HTMLFRAMEBASE2_THIS(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameBase2_get_allowTransparency(IHTMLFrameBase2 *iface, VARIANT_BOOL *p)
{
    HTMLFrameBase *This = HTMLFRAMEBASE2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

#undef HTMLFRAMEBASE2_THIS

static const IHTMLFrameBase2Vtbl HTMLFrameBase2Vtbl = {
    HTMLFrameBase2_QueryInterface,
    HTMLFrameBase2_AddRef,
    HTMLFrameBase2_Release,
    HTMLFrameBase2_GetTypeInfoCount,
    HTMLFrameBase2_GetTypeInfo,
    HTMLFrameBase2_GetIDsOfNames,
    HTMLFrameBase2_Invoke,
    HTMLFrameBase2_get_contentWindow,
    HTMLFrameBase2_put_onload,
    HTMLFrameBase2_get_onload,
    HTMLFrameBase2_put_onreadystatechange,
    HTMLFrameBase2_get_onreadystatechange,
    HTMLFrameBase2_get_readyState,
    HTMLFrameBase2_put_allowTransparency,
    HTMLFrameBase2_get_allowTransparency
};

HRESULT HTMLFrameBase_QI(HTMLFrameBase *This, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IHTMLFrameBase, riid)) {
        TRACE("(%p)->(IID_IHTMLFrameBase %p)\n", This, ppv);
        *ppv = HTMLFRAMEBASE(This);
    }else if(IsEqualGUID(&IID_IHTMLFrameBase2, riid)) {
        TRACE("(%p)->(IID_IHTMLFrameBase2 %p)\n", This, ppv);
        *ppv = HTMLFRAMEBASE2(This);
    }else {
        return HTMLElement_QI(&This->element.node, riid, ppv);
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

void HTMLFrameBase_destructor(HTMLFrameBase *This)
{
    if(This->content_window)
        This->content_window->frame_element = NULL;

    if(This->nsframe)
        nsIDOMHTMLFrameElement_Release(This->nsframe);
    if(This->nsiframe)
        nsIDOMHTMLIFrameElement_Release(This->nsiframe);

    HTMLElement_destructor(&This->element.node);
}

void HTMLFrameBase_Init(HTMLFrameBase *This, HTMLDocumentNode *doc, nsIDOMHTMLElement *nselem,
        dispex_static_data_t *dispex_data)
{
    nsresult nsres;

    This->lpIHTMLFrameBaseVtbl = &HTMLFrameBaseVtbl;
    This->lpIHTMLFrameBase2Vtbl = &HTMLFrameBase2Vtbl;

    HTMLElement_Init(&This->element, doc, nselem, dispex_data);

    nsres = nsIDOMHTMLElement_QueryInterface(nselem, &IID_nsIDOMHTMLFrameElement, (void**)&This->nsframe);
    if(NS_FAILED(nsres)) {
        nsres = nsIDOMHTMLElement_QueryInterface(nselem, &IID_nsIDOMHTMLIFrameElement, (void**)&This->nsiframe);
        if(NS_FAILED(nsres))
            ERR("Could not get nsIDOMHTML[I]Frame interface\n");
    }else
        This->nsiframe = NULL;
}

typedef struct {
    HTMLFrameBase framebase;
} HTMLFrameElement;

#define HTMLFRAME_NODE_THIS(iface) DEFINE_THIS2(HTMLFrameElement, framebase.element.node, iface)

static HRESULT HTMLFrameElement_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLFrameElement *This = HTMLFRAME_NODE_THIS(iface);

    return HTMLFrameBase_QI(&This->framebase, riid, ppv);
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

HTMLElement *HTMLFrameElement_Create(HTMLDocumentNode *doc, nsIDOMHTMLElement *nselem)
{
    HTMLFrameElement *ret;

    ret = heap_alloc_zero(sizeof(HTMLFrameElement));

    ret->framebase.element.node.vtbl = &HTMLFrameElementImplVtbl;

    HTMLFrameBase_Init(&ret->framebase, doc, nselem, NULL);

    return &ret->framebase.element;
}
