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

struct HTMLCommentElement {
    HTMLElement element;
    const IHTMLCommentElementVtbl   *lpIHTMLCommentElementVtbl;
};

#define HTMLCOMMENT(x)  (&(x)->lpIHTMLCommentElementVtbl)

#define HTMLCOMMENT_THIS(iface) DEFINE_THIS(HTMLCommentElement, IHTMLCommentElement, iface)

static HRESULT WINAPI HTMLCommentElement_QueryInterface(IHTMLCommentElement *iface,
        REFIID riid, void **ppv)
{
    HTMLCommentElement *This = HTMLCOMMENT_THIS(iface);

    return IHTMLDOMNode_QueryInterface(HTMLDOMNODE(&This->element.node), riid, ppv);
}

static ULONG WINAPI HTMLCommentElement_AddRef(IHTMLCommentElement *iface)
{
    HTMLCommentElement *This = HTMLCOMMENT_THIS(iface);

    return IHTMLDOMNode_AddRef(HTMLDOMNODE(&This->element.node));
}

static ULONG WINAPI HTMLCommentElement_Release(IHTMLCommentElement *iface)
{
    HTMLCommentElement *This = HTMLCOMMENT_THIS(iface);

    return IHTMLDOMNode_Release(HTMLDOMNODE(&This->element.node));
}

static HRESULT WINAPI HTMLCommentElement_GetTypeInfoCount(IHTMLCommentElement *iface, UINT *pctinfo)
{
    HTMLCommentElement *This = HTMLCOMMENT_THIS(iface);
    return IDispatchEx_GetTypeInfoCount(DISPATCHEX(&This->element.node.dispex), pctinfo);
}

static HRESULT WINAPI HTMLCommentElement_GetTypeInfo(IHTMLCommentElement *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLCommentElement *This = HTMLCOMMENT_THIS(iface);
    return IDispatchEx_GetTypeInfo(DISPATCHEX(&This->element.node.dispex), iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLCommentElement_GetIDsOfNames(IHTMLCommentElement *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLCommentElement *This = HTMLCOMMENT_THIS(iface);
    return IDispatchEx_GetIDsOfNames(DISPATCHEX(&This->element.node.dispex), riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLCommentElement_Invoke(IHTMLCommentElement *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLCommentElement *This = HTMLCOMMENT_THIS(iface);
    return IDispatchEx_Invoke(DISPATCHEX(&This->element.node.dispex), dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLCommentElement_put_text(IHTMLCommentElement *iface, BSTR v)
{
    HTMLCommentElement *This = HTMLCOMMENT_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCommentElement_get_text(IHTMLCommentElement *iface, BSTR *p)
{
    HTMLCommentElement *This = HTMLCOMMENT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCommentElement_put_atomic(IHTMLCommentElement *iface, long v)
{
    HTMLCommentElement *This = HTMLCOMMENT_THIS(iface);
    FIXME("(%p)->(%ld)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCommentElement_get_atomic(IHTMLCommentElement *iface, long *p)
{
    HTMLCommentElement *This = HTMLCOMMENT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

#undef HTMLCOMMENT_THIS

static const IHTMLCommentElementVtbl HTMLCommentElementVtbl = {
    HTMLCommentElement_QueryInterface,
    HTMLCommentElement_AddRef,
    HTMLCommentElement_Release,
    HTMLCommentElement_GetTypeInfoCount,
    HTMLCommentElement_GetTypeInfo,
    HTMLCommentElement_GetIDsOfNames,
    HTMLCommentElement_Invoke,
    HTMLCommentElement_put_text,
    HTMLCommentElement_get_text,
    HTMLCommentElement_put_atomic,
    HTMLCommentElement_get_atomic
};

#define HTMLCOMMENT_NODE_THIS(iface) DEFINE_THIS2(HTMLCommentElement, element.node, iface)

static HRESULT HTMLCommentElement_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLCommentElement *This = HTMLCOMMENT_NODE_THIS(iface);

    *ppv =  NULL;

    if(IsEqualGUID(&IID_IHTMLCommentElement, riid)) {
        TRACE("(%p)->(IID_IHTMLCommentElement %p)\n", This, ppv);
        *ppv = HTMLCOMMENT(This);
    }else {
        return HTMLElement_QI(&This->element.node, riid, ppv);
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static void HTMLCommentElement_destructor(HTMLDOMNode *iface)
{
    HTMLCommentElement *This = HTMLCOMMENT_NODE_THIS(iface);

    HTMLElement_destructor(&This->element.node);
}

#undef HTMLCOMMENT_NODE_THIS

static const NodeImplVtbl HTMLCommentElementImplVtbl = {
    HTMLCommentElement_QI,
    HTMLCommentElement_destructor
};

static const tid_t HTMLCommentElement_iface_tids[] = {
    IHTMLDOMNode_tid,
    IHTMLDOMNode2_tid,
    IHTMLElement_tid,
    IHTMLElement2_tid,
    IHTMLElement3_tid,
    IHTMLCommentElement_tid,
    0
};
static dispex_static_data_t HTMLCommentElement_dispex = {
    NULL,
    DispHTMLCommentElement_tid,
    NULL,
    HTMLCommentElement_iface_tids
};

HTMLElement *HTMLCommentElement_Create(HTMLDocument *doc, nsIDOMNode *nsnode)
{
    HTMLCommentElement *ret = heap_alloc_zero(sizeof(*ret));

    ret->element.node.vtbl = &HTMLCommentElementImplVtbl;
    ret->lpIHTMLCommentElementVtbl = &HTMLCommentElementVtbl;

    init_dispex(&ret->element.node.dispex, (IUnknown*)HTMLCOMMENT(ret), &HTMLCommentElement_dispex);
    HTMLElement_Init(&ret->element);
    HTMLDOMNode_Init(doc, &ret->element.node, nsnode);

    return &ret->element;
}
