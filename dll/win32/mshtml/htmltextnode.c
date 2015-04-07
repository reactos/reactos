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
    IHTMLDOMTextNode2 IHTMLDOMTextNode2_iface;

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
    HTMLDOMNode *node;
    nsIDOMText *text;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%d %p)\n", This, offset, pRetNode);

    nsres = nsIDOMText_SplitText(This->nstext, offset, &text);
    if(NS_FAILED(nsres)) {
        ERR("SplitText failed: %x08x\n", nsres);
        return E_FAIL;
    }

    if(!text) {
        *pRetNode = NULL;
        return S_OK;
    }

    hres = get_node(This->node.doc, (nsIDOMNode*)text, TRUE, &node);
    nsIDOMText_Release(text);
    if(FAILED(hres))
        return hres;

    *pRetNode = &node->IHTMLDOMNode_iface;
    return S_OK;
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

static inline HTMLDOMTextNode *impl_from_IHTMLDOMTextNode2(IHTMLDOMTextNode2 *iface)
{
    return CONTAINING_RECORD(iface, HTMLDOMTextNode, IHTMLDOMTextNode2_iface);
}

static HRESULT WINAPI HTMLDOMTextNode2_QueryInterface(IHTMLDOMTextNode2 *iface, REFIID riid, void **ppv)
{
    HTMLDOMTextNode *This = impl_from_IHTMLDOMTextNode2(iface);

    return IHTMLDOMNode_QueryInterface(&This->node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI HTMLDOMTextNode2_AddRef(IHTMLDOMTextNode2 *iface)
{
    HTMLDOMTextNode *This = impl_from_IHTMLDOMTextNode2(iface);

    return IHTMLDOMNode_AddRef(&This->node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLDOMTextNode2_Release(IHTMLDOMTextNode2 *iface)
{
    HTMLDOMTextNode *This = impl_from_IHTMLDOMTextNode2(iface);

    return IHTMLDOMNode_Release(&This->node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLDOMTextNode2_GetTypeInfoCount(IHTMLDOMTextNode2 *iface, UINT *pctinfo)
{
    HTMLDOMTextNode *This = impl_from_IHTMLDOMTextNode2(iface);
    return IDispatchEx_GetTypeInfoCount(&This->node.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLDOMTextNode2_GetTypeInfo(IHTMLDOMTextNode2 *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLDOMTextNode *This = impl_from_IHTMLDOMTextNode2(iface);
    return IDispatchEx_GetTypeInfo(&This->node.dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLDOMTextNode2_GetIDsOfNames(IHTMLDOMTextNode2 *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLDOMTextNode *This = impl_from_IHTMLDOMTextNode2(iface);
    return IDispatchEx_GetIDsOfNames(&This->node.dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI HTMLDOMTextNode2_Invoke(IHTMLDOMTextNode2 *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLDOMTextNode *This = impl_from_IHTMLDOMTextNode2(iface);
    return IDispatchEx_Invoke(&This->node.dispex.IDispatchEx_iface, dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLDOMTextNode2_substringData(IHTMLDOMTextNode2 *iface, LONG offset, LONG count, BSTR *string)
{
    HTMLDOMTextNode *This = impl_from_IHTMLDOMTextNode2(iface);
    FIXME("(%p)->(%d %d %p)\n", This, offset, count, string);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMTextNode2_appendData(IHTMLDOMTextNode2 *iface, BSTR string)
{
    HTMLDOMTextNode *This = impl_from_IHTMLDOMTextNode2(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(string));

    nsAString_InitDepend(&nsstr, string);
    nsres = nsIDOMText_AppendData(This->nstext, &nsstr);
    nsAString_Finish(&nsstr);
    if(NS_FAILED(nsres)) {
        ERR("AppendData failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLDOMTextNode2_insertData(IHTMLDOMTextNode2 *iface, LONG offset, BSTR string)
{
    HTMLDOMTextNode *This = impl_from_IHTMLDOMTextNode2(iface);
    FIXME("(%p)->(%d %s)\n", This, offset, debugstr_w(string));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMTextNode2_deleteData(IHTMLDOMTextNode2 *iface, LONG offset, LONG count)
{
    HTMLDOMTextNode *This = impl_from_IHTMLDOMTextNode2(iface);
    FIXME("(%p)->(%d %d)\n", This, offset, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMTextNode2_replaceData(IHTMLDOMTextNode2 *iface, LONG offset, LONG count, BSTR string)
{
    HTMLDOMTextNode *This = impl_from_IHTMLDOMTextNode2(iface);
    FIXME("(%p)->(%d %d %s)\n", This, offset, count, debugstr_w(string));
    return E_NOTIMPL;
}

static const IHTMLDOMTextNode2Vtbl HTMLDOMTextNode2Vtbl = {
    HTMLDOMTextNode2_QueryInterface,
    HTMLDOMTextNode2_AddRef,
    HTMLDOMTextNode2_Release,
    HTMLDOMTextNode2_GetTypeInfoCount,
    HTMLDOMTextNode2_GetTypeInfo,
    HTMLDOMTextNode2_GetIDsOfNames,
    HTMLDOMTextNode2_Invoke,
    HTMLDOMTextNode2_substringData,
    HTMLDOMTextNode2_appendData,
    HTMLDOMTextNode2_insertData,
    HTMLDOMTextNode2_deleteData,
    HTMLDOMTextNode2_replaceData
};

static inline HTMLDOMTextNode *impl_from_HTMLDOMNode(HTMLDOMNode *iface)
{
    return CONTAINING_RECORD(iface, HTMLDOMTextNode, node);
}

static HRESULT HTMLDOMTextNode_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLDOMTextNode *This = impl_from_HTMLDOMNode(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(&IID_IHTMLDOMTextNode, riid))
        *ppv = &This->IHTMLDOMTextNode_iface;
    else if(IsEqualGUID(&IID_IHTMLDOMTextNode2, riid))
        *ppv = &This->IHTMLDOMTextNode2_iface;
    else
        return HTMLDOMNode_QI(&This->node, riid, ppv);

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
    IHTMLDOMTextNode2_tid,
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
    ret->IHTMLDOMTextNode2_iface.lpVtbl = &HTMLDOMTextNode2Vtbl;

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
