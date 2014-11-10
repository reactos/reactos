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

struct HTMLDOMTextNode {
    HTMLDOMNode node;
    IHTMLDOMTextNode IHTMLDOMTextNode_iface;

    nsIDOMText *nstext;
};

static inline HTMLDOMTextNode *impl_from_IHTMLDOMTextNode(IHTMLDOMTextNode *iface)
{
    return CONTAINING_RECORD(iface, HTMLDOMTextNode, IHTMLDOMTextNode_iface);
}

static HRESULT WINAPI HTMLDOMTextNode_QueryInterface(IHTMLDOMTextNode *iface,
                                                 REFIID riid, void **ppv)
{
    HTMLDOMTextNode *This = impl_from_IHTMLDOMTextNode(iface);

    return IHTMLDOMNode_QueryInterface(&This->node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI HTMLDOMTextNode_AddRef(IHTMLDOMTextNode *iface)
{
    HTMLDOMTextNode *This = impl_from_IHTMLDOMTextNode(iface);

    return IHTMLDOMNode_AddRef(&This->node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLDOMTextNode_Release(IHTMLDOMTextNode *iface)
{
    HTMLDOMTextNode *This = impl_from_IHTMLDOMTextNode(iface);

    return IHTMLDOMNode_Release(&This->node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLDOMTextNode_GetTypeInfoCount(IHTMLDOMTextNode *iface, UINT *pctinfo)
{
    HTMLDOMTextNode *This = impl_from_IHTMLDOMTextNode(iface);
    return IDispatchEx_GetTypeInfoCount(&This->node.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLDOMTextNode_GetTypeInfo(IHTMLDOMTextNode *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLDOMTextNode *This = impl_from_IHTMLDOMTextNode(iface);
    return IDispatchEx_GetTypeInfo(&This->node.dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLDOMTextNode_GetIDsOfNames(IHTMLDOMTextNode *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLDOMTextNode *This = impl_from_IHTMLDOMTextNode(iface);
    return IDispatchEx_GetIDsOfNames(&This->node.dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI HTMLDOMTextNode_Invoke(IHTMLDOMTextNode *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLDOMTextNode *This = impl_from_IHTMLDOMTextNode(iface);
    return IDispatchEx_Invoke(&This->node.dispex.IDispatchEx_iface, dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLDOMTextNode_put_data(IHTMLDOMTextNode *iface, BSTR v)
{
    HTMLDOMTextNode *This = impl_from_IHTMLDOMTextNode(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&nsstr, v);
    nsres = nsIDOMText_SetData(This->nstext, &nsstr);
    nsAString_Finish(&nsstr);
    return NS_SUCCEEDED(nsres) ? S_OK : E_FAIL;
}

static HRESULT WINAPI HTMLDOMTextNode_get_data(IHTMLDOMTextNode *iface, BSTR *p)
{
    HTMLDOMTextNode *This = impl_from_IHTMLDOMTextNode(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&nsstr, NULL);
    nsres = nsIDOMText_GetData(This->nstext, &nsstr);
    return return_nsstr(nsres, &nsstr, p);
}

static HRESULT WINAPI HTMLDOMTextNode_toString(IHTMLDOMTextNode *iface, BSTR *String)
{
    HTMLDOMTextNode *This = impl_from_IHTMLDOMTextNode(iface);
    FIXME("(%p)->(%p)\n", This, String);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMTextNode_get_length(IHTMLDOMTextNode *iface, LONG *p)
{
    HTMLDOMTextNode *This = impl_from_IHTMLDOMTextNode(iface);
    UINT32 length = 0;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMText_GetLength(This->nstext, &length);
    if(NS_FAILED(nsres))
        ERR("GetLength failed: %08x\n", nsres);

    *p = length;
    return S_OK;
}

static HRESULT WINAPI HTMLDOMTextNode_splitText(IHTMLDOMTextNode *iface, LONG offset, IHTMLDOMNode **pRetNode)
{
    HTMLDOMTextNode *This = impl_from_IHTMLDOMTextNode(iface);
    FIXME("(%p)->(%d %p)\n", This, offset, pRetNode);
    return E_NOTIMPL;
}

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

static inline HTMLDOMTextNode *impl_from_HTMLDOMNode(HTMLDOMNode *iface)
{
    return CONTAINING_RECORD(iface, HTMLDOMTextNode, node);
}

static HRESULT HTMLDOMTextNode_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLDOMTextNode *This = impl_from_HTMLDOMNode(iface);

    *ppv =  NULL;

    if(IsEqualGUID(&IID_IHTMLDOMTextNode, riid)) {
        TRACE("(%p)->(IID_IHTMLDOMTextNode %p)\n", This, ppv);
        *ppv = &This->IHTMLDOMTextNode_iface;
    }else {
        return HTMLDOMNode_QI(&This->node, riid, ppv);
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static HRESULT HTMLDOMTextNode_clone(HTMLDOMNode *iface, nsIDOMNode *nsnode, HTMLDOMNode **ret)
{
    HTMLDOMTextNode *This = impl_from_HTMLDOMNode(iface);

    return HTMLDOMTextNode_Create(This->node.doc, nsnode, ret);
}

static const cpc_entry_t HTMLDOMTextNode_cpc[] = {{NULL}};

static const NodeImplVtbl HTMLDOMTextNodeImplVtbl = {
    HTMLDOMTextNode_QI,
    HTMLDOMNode_destructor,
    HTMLDOMTextNode_cpc,
    HTMLDOMTextNode_clone
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

HRESULT HTMLDOMTextNode_Create(HTMLDocumentNode *doc, nsIDOMNode *nsnode, HTMLDOMNode **node)
{
    HTMLDOMTextNode *ret;
    nsresult nsres;

    ret = heap_alloc_zero(sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->node.vtbl = &HTMLDOMTextNodeImplVtbl;
    ret->IHTMLDOMTextNode_iface.lpVtbl = &HTMLDOMTextNodeVtbl;

    init_dispex(&ret->node.dispex, (IUnknown*)&ret->IHTMLDOMTextNode_iface,
            &HTMLDOMTextNode_dispex);

    HTMLDOMNode_Init(doc, &ret->node, nsnode);

    nsres = nsIDOMNode_QueryInterface(nsnode, &IID_nsIDOMText, (void**)&ret->nstext);
    assert(nsres == NS_OK && (nsIDOMNode*)ret->nstext == ret->node.nsnode);

    /* Share reference with nsnode */
    nsIDOMNode_Release(ret->node.nsnode);

    *node = &ret->node;
    return S_OK;
}
