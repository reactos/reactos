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

#define NSSUPPORTS(x)  ((nsISupports*)  &(x)->lpSupportsVtbl)

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
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMNode_get_parentNode(IHTMLDOMNode *iface, IHTMLDOMNode **p)
{
    HTMLDOMNode *This = HTMLDOMNODE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMNode_hasChildNodes(IHTMLDOMNode *iface, VARIANT_BOOL *fChildren)
{
    HTMLDOMNode *This = HTMLDOMNODE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, fChildren);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMNode_get_childNodes(IHTMLDOMNode *iface, IDispatch **p)
{
    HTMLDOMNode *This = HTMLDOMNODE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
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

static HRESULT WINAPI HTMLDOMNode_removeChild(IHTMLDOMNode *iface, IHTMLDOMNode *newChild,
                                              IHTMLDOMNode **node)
{
    HTMLDOMNode *This = HTMLDOMNODE_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, newChild, node);
    return E_NOTIMPL;
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
    FIXME("(%p)->(%p %p)\n", This, newChild, node);
    return E_NOTIMPL;
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
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMNode_get_nodeValue(IHTMLDOMNode *iface, VARIANT *p)
{
    HTMLDOMNode *This = HTMLDOMNODE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMNode_get_firstChild(IHTMLDOMNode *iface, IHTMLDOMNode **p)
{
    HTMLDOMNode *This = HTMLDOMNODE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMNode_get_lastChild(IHTMLDOMNode *iface, IHTMLDOMNode **p)
{
    HTMLDOMNode *This = HTMLDOMNODE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
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

HRESULT HTMLDOMNode_QI(HTMLDOMNode *This, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = HTMLDOMNODE(This);
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = HTMLDOMNODE(This);
    }else if(IsEqualGUID(&IID_IHTMLDOMNode, riid)) {
        TRACE("(%p)->(IID_IHTMLDOMNode %p)\n", This, ppv);
        *ppv = HTMLDOMNODE(This);
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
}

static const NodeImplVtbl HTMLDOMNodeImplVtbl = {
    HTMLDOMNode_QI,
    HTMLDOMNode_destructor
};

static HTMLDOMNode *create_node(HTMLDocument *doc, nsIDOMNode *nsnode)
{
    HTMLDOMNode *ret;
    PRUint16 node_type;

    nsIDOMNode_GetNodeType(nsnode, &node_type);

    switch(node_type) {
    case ELEMENT_NODE:
        ret = &HTMLElement_Create(nsnode)->node;
        break;
    default:
        ret = heap_alloc(sizeof(HTMLDOMNode));
        ret->vtbl = &HTMLDOMNodeImplVtbl;
    }

    ret->lpHTMLDOMNodeVtbl = &HTMLDOMNodeVtbl;
    ret->ref = 1;
    ret->doc = doc;

    nsIDOMNode_AddRef(nsnode);
    ret->nsnode = nsnode;

    return ret;
}

/*
 * FIXME
 * List looks really ugly here. We should use a better data structure or
 * (better) find a way to store HTMLDOMelement pointer in nsIDOMNode.
 */

HTMLDOMNode *get_node(HTMLDocument *This, nsIDOMNode *nsnode)
{
    HTMLDOMNode *iter = This->nodes, *ret;

    while(iter) {
        if(iter->nsnode == nsnode)
            break;
        iter = iter->next;
    }

    if(iter)
        return iter;

    ret = create_node(This, nsnode);

    ret->next = This->nodes;
    This->nodes = ret;

    return ret;
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
