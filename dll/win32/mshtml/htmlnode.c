/*
 * Copyright 2006 Jacek Caban for CodeWeavers
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

#include "wine/debug.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

static HTMLDOMNode *get_node_obj(HTMLDocument*,IUnknown*);

typedef struct {
    DispatchEx dispex;
    const IHTMLDOMChildrenCollectionVtbl  *lpIHTMLDOMChildrenCollectionVtbl;

    LONG ref;

    /* FIXME: implement weak reference */
    HTMLDocument *doc;

    nsIDOMNodeList *nslist;
} HTMLDOMChildrenCollection;

#define HTMLCHILDCOL(x)  ((IHTMLDOMChildrenCollection*)  &(x)->lpIHTMLDOMChildrenCollectionVtbl)

#define HTMLCHILDCOL_THIS(iface) DEFINE_THIS(HTMLDOMChildrenCollection, IHTMLDOMChildrenCollection, iface)

static HRESULT WINAPI HTMLDOMChildrenCollection_QueryInterface(IHTMLDOMChildrenCollection *iface, REFIID riid, void **ppv)
{
    HTMLDOMChildrenCollection *This = HTMLCHILDCOL_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = HTMLCHILDCOL(This);
    }else if(IsEqualGUID(&IID_IHTMLDOMChildrenCollection, riid)) {
        TRACE("(%p)->(IID_IHTMLDOMChildrenCollection %p)\n", This, ppv);
        *ppv = HTMLCHILDCOL(This);
    }else if(dispex_query_interface(&This->dispex, riid, ppv)) {
        return *ppv ? S_OK : E_NOINTERFACE;
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI HTMLDOMChildrenCollection_AddRef(IHTMLDOMChildrenCollection *iface)
{
    HTMLDOMChildrenCollection *This = HTMLCHILDCOL_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLDOMChildrenCollection_Release(IHTMLDOMChildrenCollection *iface)
{
    HTMLDOMChildrenCollection *This = HTMLCHILDCOL_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        nsIDOMNodeList_Release(This->nslist);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI HTMLDOMChildrenCollection_GetTypeInfoCount(IHTMLDOMChildrenCollection *iface, UINT *pctinfo)
{
    HTMLDOMChildrenCollection *This = HTMLCHILDCOL_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pctinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMChildrenCollection_GetTypeInfo(IHTMLDOMChildrenCollection *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLDOMChildrenCollection *This = HTMLCHILDCOL_THIS(iface);
    FIXME("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMChildrenCollection_GetIDsOfNames(IHTMLDOMChildrenCollection *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLDOMChildrenCollection *This = HTMLCHILDCOL_THIS(iface);
    FIXME("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMChildrenCollection_Invoke(IHTMLDOMChildrenCollection *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLDOMChildrenCollection *This = HTMLCHILDCOL_THIS(iface);
    FIXME("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMChildrenCollection_get_length(IHTMLDOMChildrenCollection *iface, long *p)
{
    HTMLDOMChildrenCollection *This = HTMLCHILDCOL_THIS(iface);
    PRUint32 length=0;

    TRACE("(%p)->(%p)\n", This, p);

    nsIDOMNodeList_GetLength(This->nslist, &length);
    *p = length;
    return S_OK;
}

static HRESULT WINAPI HTMLDOMChildrenCollection__newEnum(IHTMLDOMChildrenCollection *iface, IUnknown **p)
{
    HTMLDOMChildrenCollection *This = HTMLCHILDCOL_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMChildrenCollection_item(IHTMLDOMChildrenCollection *iface, long index, IDispatch **ppItem)
{
    HTMLDOMChildrenCollection *This = HTMLCHILDCOL_THIS(iface);
    nsIDOMNode *nsnode = NULL;
    PRUint32 length=0;
    nsresult nsres;

    TRACE("(%p)->(%ld %p)\n", This, index, ppItem);

    nsIDOMNodeList_GetLength(This->nslist, &length);
    if(index < 0 || index >= length)
        return E_INVALIDARG;

    nsres = nsIDOMNodeList_Item(This->nslist, index, &nsnode);
    if(NS_FAILED(nsres) || !nsnode) {
        ERR("Item failed: %08x\n", nsres);
        return E_FAIL;
    }

    *ppItem = (IDispatch*)get_node(This->doc, nsnode, TRUE);
    IDispatch_AddRef(*ppItem);
    return S_OK;
}

#define DISPID_CHILDCOL_0 MSHTML_DISPID_CUSTOM_MIN

static HRESULT HTMLDOMChildrenCollection_get_dispid(IUnknown *iface, BSTR name, DWORD flags, DISPID *dispid)
{
    HTMLDOMChildrenCollection *This = HTMLCHILDCOL_THIS(iface);
    WCHAR *ptr;
    DWORD idx=0;
    PRUint32 len = 0;

    for(ptr = name; *ptr && isdigitW(*ptr); ptr++)
        idx = idx*10 + (*ptr-'0');
    if(*ptr)
        return DISP_E_UNKNOWNNAME;

    nsIDOMNodeList_GetLength(This->nslist, &len);
    if(idx >= len)
        return DISP_E_UNKNOWNNAME;

    *dispid = DISPID_CHILDCOL_0 + idx;
    TRACE("ret %x\n", *dispid);
    return S_OK;
}

static HRESULT HTMLDOMChildrenCollection_invoke(IUnknown *iface, DISPID id, LCID lcid, WORD flags, DISPPARAMS *params,
        VARIANT *res, EXCEPINFO *ei, IServiceProvider *caller)
{
    HTMLDOMChildrenCollection *This = HTMLCHILDCOL_THIS(iface);

    TRACE("(%p)->(%x %x %x %p %p %p %p)\n", This, id, lcid, flags, params, res, ei, caller);

    switch(flags) {
    case INVOKE_PROPERTYGET: {
        IDispatch *disp = NULL;
        HRESULT hres;

        hres = IHTMLDOMChildrenCollection_item(HTMLCHILDCOL(This), id - DISPID_CHILDCOL_0, &disp);
        if(0&&FAILED(hres))
            return hres;

        V_VT(res) = VT_DISPATCH;
        V_DISPATCH(res) = disp;
        break;
    }

    default:
        FIXME("unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

#undef HTMLCHILDCOL_THIS

static const IHTMLDOMChildrenCollectionVtbl HTMLDOMChildrenCollectionVtbl = {
    HTMLDOMChildrenCollection_QueryInterface,
    HTMLDOMChildrenCollection_AddRef,
    HTMLDOMChildrenCollection_Release,
    HTMLDOMChildrenCollection_GetTypeInfoCount,
    HTMLDOMChildrenCollection_GetTypeInfo,
    HTMLDOMChildrenCollection_GetIDsOfNames,
    HTMLDOMChildrenCollection_Invoke,
    HTMLDOMChildrenCollection_get_length,
    HTMLDOMChildrenCollection__newEnum,
    HTMLDOMChildrenCollection_item
};

static const tid_t HTMLDOMChildrenCollection_iface_tids[] = {
    IHTMLDOMChildrenCollection_tid,
    0
};

static const dispex_static_data_vtbl_t HTMLDOMChildrenCollection_dispex_vtbl = {
    HTMLDOMChildrenCollection_get_dispid,
    HTMLDOMChildrenCollection_invoke
};

static dispex_static_data_t HTMLDOMChildrenCollection_dispex = {
    &HTMLDOMChildrenCollection_dispex_vtbl,
    DispDOMChildrenCollection_tid,
    NULL,
    HTMLDOMChildrenCollection_iface_tids
};

static IHTMLDOMChildrenCollection *create_child_collection(HTMLDocument *doc, nsIDOMNodeList *nslist)
{
    HTMLDOMChildrenCollection *ret;

    ret = heap_alloc_zero(sizeof(*ret));
    ret->lpIHTMLDOMChildrenCollectionVtbl = &HTMLDOMChildrenCollectionVtbl;
    ret->ref = 1;

    nsIDOMNodeList_AddRef(nslist);
    ret->nslist = nslist;
    ret->doc = doc;

    init_dispex(&ret->dispex, (IUnknown*)HTMLCHILDCOL(ret), &HTMLDOMChildrenCollection_dispex);

    return HTMLCHILDCOL(ret);
}

#define HTMLDOMNODE_THIS(iface) DEFINE_THIS(HTMLDOMNode, HTMLDOMNode, iface)

static HRESULT WINAPI HTMLDOMNode_QueryInterface(IHTMLDOMNode *iface,
                                                 REFIID riid, void **ppv)
{
    HTMLDOMNode *This = HTMLDOMNODE_THIS(iface);

    return This->vtbl->qi(This, riid, ppv);
}

static ULONG WINAPI HTMLDOMNode_AddRef(IHTMLDOMNode *iface)
{
    HTMLDOMNode *This = HTMLDOMNODE_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLDOMNode_Release(IHTMLDOMNode *iface)
{
    HTMLDOMNode *This = HTMLDOMNODE_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        This->vtbl->destructor(This);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI HTMLDOMNode_GetTypeInfoCount(IHTMLDOMNode *iface, UINT *pctinfo)
{
    HTMLDOMNode *This = HTMLDOMNODE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pctinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMNode_GetTypeInfo(IHTMLDOMNode *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLDOMNode *This = HTMLDOMNODE_THIS(iface);
    FIXME("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMNode_GetIDsOfNames(IHTMLDOMNode *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLDOMNode *This = HTMLDOMNODE_THIS(iface);
    FIXME("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
                                        lcid, rgDispId);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMNode_Invoke(IHTMLDOMNode *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLDOMNode *This = HTMLDOMNODE_THIS(iface);
    FIXME("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMNode_get_nodeType(IHTMLDOMNode *iface, long *p)
{
    HTMLDOMNode *This = HTMLDOMNODE_THIS(iface);
    PRUint16 type = -1;

    TRACE("(%p)->(%p)\n", This, p);

    nsIDOMNode_GetNodeType(This->nsnode, &type);

    switch(type) {
    case ELEMENT_NODE:
        *p = 1;
        break;
    case TEXT_NODE:
        *p = 3;
        break;
    case COMMENT_NODE:
        *p = 8;
        break;
    case DOCUMENT_NODE:
        *p = 9;
        break;
    default:
        /*
         * FIXME:
         * According to MSDN only ELEMENT_NODE and TEXT_NODE are supported.
         * It needs more tests.
         */
        FIXME("type %u\n", type);
        *p = 0;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLDOMNode_get_parentNode(IHTMLDOMNode *iface, IHTMLDOMNode **p)
{
    HTMLDOMNode *This = HTMLDOMNODE_THIS(iface);
    HTMLDOMNode *node;
    nsIDOMNode *nsnode;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMNode_GetParentNode(This->nsnode, &nsnode);
    if(NS_FAILED(nsres)) {
        ERR("GetParentNode failed: %08x\n", nsres);
        return E_FAIL;
    }

    if(!nsnode) {
        *p = NULL;
        return S_OK;
    }

    node = get_node(This->doc, nsnode, TRUE);
    *p = HTMLDOMNODE(node);
    IHTMLDOMNode_AddRef(*p);
    return S_OK;
}

static HRESULT WINAPI HTMLDOMNode_hasChildNodes(IHTMLDOMNode *iface, VARIANT_BOOL *fChildren)
{
    HTMLDOMNode *This = HTMLDOMNODE_THIS(iface);
    PRBool has_child = FALSE;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, fChildren);

    nsres = nsIDOMNode_HasChildNodes(This->nsnode, &has_child);
    if(NS_FAILED(nsres))
        ERR("HasChildNodes failed: %08x\n", nsres);

    *fChildren = has_child ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

static HRESULT WINAPI HTMLDOMNode_get_childNodes(IHTMLDOMNode *iface, IDispatch **p)
{
    HTMLDOMNode *This = HTMLDOMNODE_THIS(iface);
    nsIDOMNodeList *nslist;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMNode_GetChildNodes(This->nsnode, &nslist);
    if(NS_FAILED(nsres)) {
        ERR("GetChildNodes failed: %08x\n", nsres);
        return E_FAIL;
    }

    *p = (IDispatch*)create_child_collection(This->doc, nslist);
    nsIDOMNodeList_Release(nslist);

    return S_OK;
}

static HRESULT WINAPI HTMLDOMNode_get_attributes(IHTMLDOMNode *iface, IDispatch **p)
{
    HTMLDOMNode *This = HTMLDOMNODE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMNode_insertBefore(IHTMLDOMNode *iface, IHTMLDOMNode *newChild,
                                               VARIANT refChild, IHTMLDOMNode **node)
{
    HTMLDOMNode *This = HTMLDOMNODE_THIS(iface);
    FIXME("(%p)->(%p v %p)\n", This, newChild, node);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMNode_removeChild(IHTMLDOMNode *iface, IHTMLDOMNode *oldChild,
                                              IHTMLDOMNode **node)
{
    HTMLDOMNode *This = HTMLDOMNODE_THIS(iface);
    HTMLDOMNode *node_obj;
    nsIDOMNode *nsnode;
    nsresult nsres;

    TRACE("(%p)->(%p %p)\n", This, oldChild, node);

    node_obj = get_node_obj(This->doc, (IUnknown*)oldChild);
    if(!node_obj)
        return E_FAIL;

    nsres = nsIDOMNode_RemoveChild(This->nsnode, node_obj->nsnode, &nsnode);
    if(NS_FAILED(nsres)) {
        ERR("RemoveChild failed: %08x\n", nsres);
        return E_FAIL;
    }

    /* FIXME: Make sure that node != newChild */
    *node = HTMLDOMNODE(get_node(This->doc, nsnode, TRUE));
    IHTMLDOMNode_AddRef(*node);
    return S_OK;
}

static HRESULT WINAPI HTMLDOMNode_replaceChild(IHTMLDOMNode *iface, IHTMLDOMNode *newChild,
                                               IHTMLDOMNode *oldChild, IHTMLDOMNode **node)
{
    HTMLDOMNode *This = HTMLDOMNODE_THIS(iface);
    FIXME("(%p)->(%p %p %p)\n", This, newChild, oldChild, node);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMNode_cloneNode(IHTMLDOMNode *iface, VARIANT_BOOL fDeep,
                                            IHTMLDOMNode **clonedNode)
{
    HTMLDOMNode *This = HTMLDOMNODE_THIS(iface);
    FIXME("(%p)->(%x %p)\n", This, fDeep, clonedNode);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMNode_removeNode(IHTMLDOMNode *iface, VARIANT_BOOL fDeep,
                                             IHTMLDOMNode **removed)
{
    HTMLDOMNode *This = HTMLDOMNODE_THIS(iface);
    FIXME("(%p)->(%x %p)\n", This, fDeep, removed);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMNode_swapNode(IHTMLDOMNode *iface, IHTMLDOMNode *otherNode,
                                           IHTMLDOMNode **swappedNode)
{
    HTMLDOMNode *This = HTMLDOMNODE_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, otherNode, swappedNode);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMNode_replaceNode(IHTMLDOMNode *iface, IHTMLDOMNode *replacement,
                                              IHTMLDOMNode **replaced)
{
    HTMLDOMNode *This = HTMLDOMNODE_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, replacement, replaced);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMNode_appendChild(IHTMLDOMNode *iface, IHTMLDOMNode *newChild,
                                              IHTMLDOMNode **node)
{
    HTMLDOMNode *This = HTMLDOMNODE_THIS(iface);
    HTMLDOMNode *node_obj;
    nsIDOMNode *nsnode;
    nsresult nsres;

    TRACE("(%p)->(%p %p)\n", This, newChild, node);

    node_obj = get_node_obj(This->doc, (IUnknown*)newChild);
    if(!node_obj)
        return E_FAIL;

    nsres = nsIDOMNode_AppendChild(This->nsnode, node_obj->nsnode, &nsnode);
    if(NS_FAILED(nsres)) {
        ERR("AppendChild failed: %08x\n", nsres);
        return E_FAIL;
    }

    /* FIXME: Make sure that node != newChild */
    *node = HTMLDOMNODE(get_node(This->doc, nsnode, TRUE));
    IHTMLDOMNode_AddRef(*node);
    return S_OK;
}

static HRESULT WINAPI HTMLDOMNode_get_nodeName(IHTMLDOMNode *iface, BSTR *p)
{
    HTMLDOMNode *This = HTMLDOMNODE_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = NULL;

    if(This->nsnode) {
        nsAString name_str;
        const PRUnichar *name;
        nsresult nsres;

        nsAString_Init(&name_str, NULL);
        nsres = nsIDOMNode_GetNodeName(This->nsnode, &name_str);

        if(NS_SUCCEEDED(nsres)) {
            nsAString_GetData(&name_str, &name);
            *p = SysAllocString(name);
        }else {
            ERR("GetNodeName failed: %08x\n", nsres);
        }

        nsAString_Finish(&name_str);
    }

    return S_OK;
}

static HRESULT WINAPI HTMLDOMNode_put_nodeValue(IHTMLDOMNode *iface, VARIANT v)
{
    HTMLDOMNode *This = HTMLDOMNODE_THIS(iface);

    TRACE("(%p)->()\n", This);

    switch(V_VT(&v)) {
    case VT_BSTR: {
        nsAString val_str;

        TRACE("bstr %s\n", debugstr_w(V_BSTR(&v)));

        nsAString_Init(&val_str, V_BSTR(&v));
        nsIDOMNode_SetNodeValue(This->nsnode, &val_str);
        nsAString_Finish(&val_str);

        return S_OK;
    }

    default:
        FIXME("unsupported vt %d\n", V_VT(&v));
    }

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMNode_get_nodeValue(IHTMLDOMNode *iface, VARIANT *p)
{
    HTMLDOMNode *This = HTMLDOMNODE_THIS(iface);
    const PRUnichar *val;
    nsAString val_str;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&val_str, NULL);
    nsIDOMNode_GetNodeValue(This->nsnode, &val_str);
    nsAString_GetData(&val_str, &val);

    if(*val) {
        V_VT(p) = VT_BSTR;
        V_BSTR(p) = SysAllocString(val);
    }else {
        V_VT(p) = VT_NULL;
    }

    nsAString_Finish(&val_str);

    return S_OK;
}

static HRESULT WINAPI HTMLDOMNode_get_firstChild(IHTMLDOMNode *iface, IHTMLDOMNode **p)
{
    HTMLDOMNode *This = HTMLDOMNODE_THIS(iface);
    nsIDOMNode *nschild = NULL;

    TRACE("(%p)->(%p)\n", This, p);

    nsIDOMNode_GetFirstChild(This->nsnode, &nschild);
    if(nschild) {
        *p = HTMLDOMNODE(get_node(This->doc, nschild, TRUE));
        IHTMLDOMNode_AddRef(*p);
    }else {
        *p = NULL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLDOMNode_get_lastChild(IHTMLDOMNode *iface, IHTMLDOMNode **p)
{
    HTMLDOMNode *This = HTMLDOMNODE_THIS(iface);
    nsIDOMNode *nschild = NULL;

    TRACE("(%p)->(%p)\n", This, p);

    nsIDOMNode_GetLastChild(This->nsnode, &nschild);
    if(nschild) {
        *p = HTMLDOMNODE(get_node(This->doc, nschild, TRUE));
        IHTMLDOMNode_AddRef(*p);
    }else {
        *p = NULL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLDOMNode_get_previousSibling(IHTMLDOMNode *iface, IHTMLDOMNode **p)
{
    HTMLDOMNode *This = HTMLDOMNODE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMNode_get_nextSibling(IHTMLDOMNode *iface, IHTMLDOMNode **p)
{
    HTMLDOMNode *This = HTMLDOMNODE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

#undef HTMLDOMNODE_THIS

static const IHTMLDOMNodeVtbl HTMLDOMNodeVtbl = {
    HTMLDOMNode_QueryInterface,
    HTMLDOMNode_AddRef,
    HTMLDOMNode_Release,
    HTMLDOMNode_GetTypeInfoCount,
    HTMLDOMNode_GetTypeInfo,
    HTMLDOMNode_GetIDsOfNames,
    HTMLDOMNode_Invoke,
    HTMLDOMNode_get_nodeType,
    HTMLDOMNode_get_parentNode,
    HTMLDOMNode_hasChildNodes,
    HTMLDOMNode_get_childNodes,
    HTMLDOMNode_get_attributes,
    HTMLDOMNode_insertBefore,
    HTMLDOMNode_removeChild,
    HTMLDOMNode_replaceChild,
    HTMLDOMNode_cloneNode,
    HTMLDOMNode_removeNode,
    HTMLDOMNode_swapNode,
    HTMLDOMNode_replaceNode,
    HTMLDOMNode_appendChild,
    HTMLDOMNode_get_nodeName,
    HTMLDOMNode_put_nodeValue,
    HTMLDOMNode_get_nodeValue,
    HTMLDOMNode_get_firstChild,
    HTMLDOMNode_get_lastChild,
    HTMLDOMNode_get_previousSibling,
    HTMLDOMNode_get_nextSibling
};

#define HTMLDOMNODE2_THIS(iface) DEFINE_THIS(HTMLDOMNode, HTMLDOMNode2, iface)

static HRESULT WINAPI HTMLDOMNode2_QueryInterface(IHTMLDOMNode2 *iface,
        REFIID riid, void **ppv)
{
    HTMLDOMNode *This = HTMLDOMNODE2_THIS(iface);

    return IHTMLDOMNode_QueryInterface(HTMLDOMNODE(This), riid, ppv);
}

static ULONG WINAPI HTMLDOMNode2_AddRef(IHTMLDOMNode2 *iface)
{
    HTMLDOMNode *This = HTMLDOMNODE2_THIS(iface);

    return IHTMLDOMNode_AddRef(HTMLDOMNODE(This));
}

static ULONG WINAPI HTMLDOMNode2_Release(IHTMLDOMNode2 *iface)
{
    HTMLDOMNode *This = HTMLDOMNODE2_THIS(iface);

    return IHTMLDOMNode_Release(HTMLDOMNODE(This));
}

static HRESULT WINAPI HTMLDOMNode2_GetTypeInfoCount(IHTMLDOMNode2 *iface, UINT *pctinfo)
{
    HTMLDOMNode *This = HTMLDOMNODE2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pctinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMNode2_GetTypeInfo(IHTMLDOMNode2 *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLDOMNode *This = HTMLDOMNODE2_THIS(iface);
    FIXME("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMNode2_GetIDsOfNames(IHTMLDOMNode2 *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLDOMNode *This = HTMLDOMNODE2_THIS(iface);
    FIXME("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMNode2_Invoke(IHTMLDOMNode2 *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLDOMNode *This = HTMLDOMNODE2_THIS(iface);
    FIXME("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMNode2_get_ownerDocument(IHTMLDOMNode2 *iface, IDispatch **p)
{
    HTMLDOMNode *This = HTMLDOMNODE2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

#undef HTMLDOMNODE2_THIS

static const IHTMLDOMNode2Vtbl HTMLDOMNode2Vtbl = {
    HTMLDOMNode2_QueryInterface,
    HTMLDOMNode2_AddRef,
    HTMLDOMNode2_Release,
    HTMLDOMNode2_GetTypeInfoCount,
    HTMLDOMNode2_GetTypeInfo,
    HTMLDOMNode2_GetIDsOfNames,
    HTMLDOMNode2_Invoke,
    HTMLDOMNode2_get_ownerDocument
};

HRESULT HTMLDOMNode_QI(HTMLDOMNode *This, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = HTMLDOMNODE(This);
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = HTMLDOMNODE(This);
    }else if(IsEqualGUID(&IID_IDispatchEx, riid)) {
        if(This->dispex.data) {
            TRACE("(%p)->(IID_IDispatchEx %p)\n", This, ppv);
            *ppv = DISPATCHEX(&This->dispex);
        }else {
            FIXME("(%p)->(IID_IDispatchEx %p)\n", This, ppv);
            return E_NOINTERFACE;
        }
    }else if(IsEqualGUID(&IID_IHTMLDOMNode, riid)) {
        TRACE("(%p)->(IID_IHTMLDOMNode %p)\n", This, ppv);
        *ppv = HTMLDOMNODE(This);
    }else if(IsEqualGUID(&IID_IHTMLDOMNode2, riid)) {
        TRACE("(%p)->(IID_IHTMLDOMNode2 %p)\n", This, ppv);
        *ppv = HTMLDOMNODE2(This);
    }else if(dispex_query_interface(&This->dispex, riid, ppv)) {
        return *ppv ? S_OK : E_NOINTERFACE;
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

void HTMLDOMNode_destructor(HTMLDOMNode *This)
{
    if(This->nsnode)
        nsIDOMNode_Release(This->nsnode);
    if(This->event_target)
        release_event_target(This->event_target);
}

static const NodeImplVtbl HTMLDOMNodeImplVtbl = {
    HTMLDOMNode_QI,
    HTMLDOMNode_destructor
};

void HTMLDOMNode_Init(HTMLDocument *doc, HTMLDOMNode *node, nsIDOMNode *nsnode)
{
    node->lpHTMLDOMNodeVtbl = &HTMLDOMNodeVtbl;
    node->lpHTMLDOMNode2Vtbl = &HTMLDOMNode2Vtbl;
    node->ref = 1;
    node->doc = doc;

    nsIDOMNode_AddRef(nsnode);
    node->nsnode = nsnode;

    node->next = doc->nodes;
    doc->nodes = node;
}

static HTMLDOMNode *create_node(HTMLDocument *doc, nsIDOMNode *nsnode)
{
    HTMLDOMNode *ret;
    PRUint16 node_type;

    nsIDOMNode_GetNodeType(nsnode, &node_type);

    switch(node_type) {
    case ELEMENT_NODE:
        ret = &HTMLElement_Create(doc, nsnode, FALSE)->node;
        break;
    case TEXT_NODE:
        ret = HTMLDOMTextNode_Create(doc, nsnode);
        break;
    case COMMENT_NODE:
        ret = &HTMLCommentElement_Create(doc, nsnode)->node;
        break;
    default:
        ret = heap_alloc_zero(sizeof(HTMLDOMNode));
        ret->vtbl = &HTMLDOMNodeImplVtbl;
        HTMLDOMNode_Init(doc, ret, nsnode);
    }

    TRACE("type %d ret %p\n", node_type, ret);

    return ret;
}

/*
 * FIXME
 * List looks really ugly here. We should use a better data structure or
 * (better) find a way to store HTMLDOMelement pointer in nsIDOMNode.
 */

HTMLDOMNode *get_node(HTMLDocument *This, nsIDOMNode *nsnode, BOOL create)
{
    HTMLDOMNode *iter = This->nodes;

    while(iter) {
        if(iter->nsnode == nsnode)
            break;
        iter = iter->next;
    }

    if(iter || !create)
        return iter;

    return create_node(This, nsnode);
}

/*
 * FIXME
 * We should use better way for getting node object (like private interface)
 * or avoid it at all.
 */
static HTMLDOMNode *get_node_obj(HTMLDocument *This, IUnknown *iface)
{
    HTMLDOMNode *iter = This->nodes;
    IHTMLDOMNode *node;

    IUnknown_QueryInterface(iface, &IID_IHTMLDOMNode, (void**)&node);
    IHTMLDOMNode_Release(node);

    while(iter) {
        if(HTMLDOMNODE(iter) == node)
            return iter;
        iter = iter->next;
    }

    FIXME("Not found %p\n", iface);
    return NULL;
}

void release_nodes(HTMLDocument *This)
{
    HTMLDOMNode *iter, *next;

    if(!This->nodes)
        return;

    for(iter = This->nodes; iter; iter = next) {
        next = iter->next;
        iter->doc = NULL;
        IHTMLDOMNode_Release(HTMLDOMNODE(iter));
    }
}
