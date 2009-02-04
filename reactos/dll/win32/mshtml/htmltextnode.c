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

struct HTMLDOMTextNode {
    HTMLDOMNode node;
    const IHTMLDOMTextNodeVtbl   *lpIHTMLDOMTextNodeVtbl;
};

#define HTMLTEXT(x)  ((IHTMLDOMTextNode*)  &(x)->lpIHTMLDOMTextNodeVtbl)

#define HTMLTEXT_THIS(iface) DEFINE_THIS(HTMLDOMTextNode, IHTMLDOMTextNode, iface)

#define HTMLTEXT_NODE_THIS(iface) DEFINE_THIS2(HTMLDOMTextNode, node, iface)

static HRESULT WINAPI HTMLDOMTextNode_QueryInterface(IHTMLDOMTextNode *iface,
                                                 REFIID riid, void **ppv)
{
    HTMLDOMTextNode *This = HTMLTEXT_THIS(iface);

    return IHTMLDOMNode_QueryInterface(HTMLDOMNODE(&This->node), riid, ppv);
}

static ULONG WINAPI HTMLDOMTextNode_AddRef(IHTMLDOMTextNode *iface)
{
    HTMLDOMTextNode *This = HTMLTEXT_THIS(iface);

    return IHTMLDOMNode_AddRef(HTMLDOMNODE(&This->node));
}

static ULONG WINAPI HTMLDOMTextNode_Release(IHTMLDOMTextNode *iface)
{
    HTMLDOMTextNode *This = HTMLTEXT_THIS(iface);

    return IHTMLDOMNode_Release(HTMLDOMNODE(&This->node));
}

static HRESULT WINAPI HTMLDOMTextNode_GetTypeInfoCount(IHTMLDOMTextNode *iface, UINT *pctinfo)
{
    HTMLDOMTextNode *This = HTMLTEXT_THIS(iface);
    return IDispatchEx_GetTypeInfoCount(DISPATCHEX(&This->node.dispex), pctinfo);
}

static HRESULT WINAPI HTMLDOMTextNode_GetTypeInfo(IHTMLDOMTextNode *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLDOMTextNode *This = HTMLTEXT_THIS(iface);
    return IDispatchEx_GetTypeInfo(DISPATCHEX(&This->node.dispex), iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLDOMTextNode_GetIDsOfNames(IHTMLDOMTextNode *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLDOMTextNode *This = HTMLTEXT_THIS(iface);
    return IDispatchEx_GetIDsOfNames(DISPATCHEX(&This->node.dispex), riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLDOMTextNode_Invoke(IHTMLDOMTextNode *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLDOMTextNode *This = HTMLTEXT_THIS(iface);
    return IDispatchEx_Invoke(DISPATCHEX(&This->node.dispex), dispIdMember, riid, lcid, wFlags, pDispParams,
            pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLDOMTextNode_put_data(IHTMLDOMTextNode *iface, BSTR v)
{
    HTMLDOMTextNode *This = HTMLTEXT_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMTextNode_get_data(IHTMLDOMTextNode *iface, BSTR *p)
{
    HTMLDOMTextNode *This = HTMLTEXT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMTextNode_toString(IHTMLDOMTextNode *iface, BSTR *String)
{
    HTMLDOMTextNode *This = HTMLTEXT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, String);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMTextNode_get_length(IHTMLDOMTextNode *iface, long *p)
{
    HTMLDOMTextNode *This = HTMLTEXT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMTextNode_splitText(IHTMLDOMTextNode *iface, long offset, IHTMLDOMNode **pRetNode)
{
    HTMLDOMTextNode *This = HTMLTEXT_THIS(iface);
    FIXME("(%p)->(%ld %p)\n", This, offset, pRetNode);
    return E_NOTIMPL;
}

#undef HTMLTEXT_THIS

static const IHTMLDOMTextNodeVtbl HTMLDOMTextNodeVtbl = {
    HTMLDOMTextNode_QueryInterface,
    HTMLDOMTextNode_AddRef,
    HTMLDOMTextNode_Release,
    HTMLDOMTextNode_GetTypeInfoCount,
    HTMLDOMTextNode_GetTypeInfo,
    HTMLDOMTextNode_GetIDsOfNames,
    HTMLDOMTextNode_Invoke,
    HTMLDOMTextNode_put_data,
    HTMLDOMTextNode_get_data,
    HTMLDOMTextNode_toString,
    HTMLDOMTextNode_get_length,
    HTMLDOMTextNode_splitText
};

#define HTMLTEXT_NODE_THIS(iface) DEFINE_THIS2(HTMLDOMTextNode, node, iface)

static HRESULT HTMLDOMTextNode_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLDOMTextNode *This = HTMLTEXT_NODE_THIS(iface);

    *ppv =  NULL;

    if(IsEqualGUID(&IID_IHTMLDOMTextNode, riid)) {
        TRACE("(%p)->(IID_IHTMLDOMTextNode %p)\n", This, ppv);
        *ppv = HTMLTEXT(This);
    }else {
        return HTMLDOMNode_QI(&This->node, riid, ppv);
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static void HTMLDOMTextNode_destructor(HTMLDOMNode *iface)
{
    HTMLDOMTextNode *This = HTMLTEXT_NODE_THIS(iface);

    HTMLDOMNode_destructor(&This->node);
}

#undef HTMLTEXT_NODE_THIS

static const NodeImplVtbl HTMLDOMTextNodeImplVtbl = {
    HTMLDOMTextNode_QI,
    HTMLDOMTextNode_destructor
};

static const tid_t HTMLDOMTextNode_iface_tids[] = {
    IHTMLDOMNode_tid,
    IHTMLDOMNode2_tid,
    IHTMLDOMTextNode_tid,
    0
};
static dispex_static_data_t HTMLDOMTextNode_dispex = {
    NULL,
    DispHTMLDOMTextNode_tid,
    0,
    HTMLDOMTextNode_iface_tids
};

HTMLDOMNode *HTMLDOMTextNode_Create(HTMLDocument *doc, nsIDOMNode *nsnode)
{
    HTMLDOMTextNode *ret ;

    ret = heap_alloc_zero(sizeof(*ret));
    ret->node.vtbl = &HTMLDOMTextNodeImplVtbl;
    ret->lpIHTMLDOMTextNodeVtbl = &HTMLDOMTextNodeVtbl;

    init_dispex(&ret->node.dispex, (IUnknown*)HTMLTEXT(ret), &HTMLDOMTextNode_dispex);
    HTMLDOMNode_Init(doc, &ret->node, nsnode);

    return &ret->node;
}
