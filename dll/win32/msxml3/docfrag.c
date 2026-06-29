/*
 *    DOM Document Fragment implementation
 *
 * Copyright 2007 Alistair Leslie-Hughes
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

#define COBJMACROS

#include <stdarg.h>
#include <libxml/parser.h>
#include <libxml/xmlerror.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "msxml6.h"

#include "msxml_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

typedef struct _domfrag
{
    xmlnode node;
    IXMLDOMDocumentFragment IXMLDOMDocumentFragment_iface;
    LONG ref;
} domfrag;

static const tid_t domfrag_se_tids[] = {
    IXMLDOMNode_tid,
    IXMLDOMDocumentFragment_tid,
    NULL_tid
};

static inline domfrag *impl_from_IXMLDOMDocumentFragment( IXMLDOMDocumentFragment *iface )
{
    return CONTAINING_RECORD(iface, domfrag, IXMLDOMDocumentFragment_iface);
}

static HRESULT WINAPI domfrag_QueryInterface(
    IXMLDOMDocumentFragment *iface,
    REFIID riid,
    void** ppvObject )
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppvObject);

    if ( IsEqualGUID( riid, &IID_IXMLDOMDocumentFragment ) ||
         IsEqualGUID( riid, &IID_IXMLDOMNode ) ||
         IsEqualGUID( riid, &IID_IDispatch ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = iface;
    }
    else if(node_query_interface(&This->node, riid, ppvObject))
    {
        return *ppvObject ? S_OK : E_NOINTERFACE;
    }
    else if(IsEqualGUID( riid, &IID_ISupportErrorInfo ))
    {
        return node_create_supporterrorinfo(domfrag_se_tids, ppvObject);
    }
    else
    {
        TRACE("Unsupported interface %s\n", debugstr_guid(riid));
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    IXMLDOMDocumentFragment_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI domfrag_AddRef(IXMLDOMDocumentFragment *iface)
{
    domfrag *domfrag = impl_from_IXMLDOMDocumentFragment(iface);
    ULONG ref = InterlockedIncrement(&domfrag->ref);
    TRACE("%p, refcount %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI domfrag_Release(IXMLDOMDocumentFragment *iface)
{
    domfrag *domfrag = impl_from_IXMLDOMDocumentFragment(iface);
    ULONG ref = InterlockedDecrement(&domfrag->ref);

    TRACE("%p, refcount %lu.\n", iface, ref);

    if (!ref)
    {
        destroy_xmlnode(&domfrag->node);
        free(domfrag);
    }

    return ref;
}

static HRESULT WINAPI domfrag_GetTypeInfoCount(
    IXMLDOMDocumentFragment *iface,
    UINT* pctinfo )
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    return IDispatchEx_GetTypeInfoCount(&This->node.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI domfrag_GetTypeInfo(
    IXMLDOMDocumentFragment *iface,
    UINT iTInfo, LCID lcid,
    ITypeInfo** ppTInfo )
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    return IDispatchEx_GetTypeInfo(&This->node.dispex.IDispatchEx_iface,
        iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI domfrag_GetIDsOfNames(
    IXMLDOMDocumentFragment *iface,
    REFIID riid, LPOLESTR* rgszNames,
    UINT cNames, LCID lcid, DISPID* rgDispId )
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    return IDispatchEx_GetIDsOfNames(&This->node.dispex.IDispatchEx_iface,
        riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI domfrag_Invoke(
    IXMLDOMDocumentFragment *iface,
    DISPID dispIdMember, REFIID riid, LCID lcid,
    WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult,
    EXCEPINFO* pExcepInfo, UINT* puArgErr )
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    return IDispatchEx_Invoke(&This->node.dispex.IDispatchEx_iface,
        dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI domfrag_get_nodeName(
    IXMLDOMDocumentFragment *iface,
    BSTR* p )
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );

    static const WCHAR document_fragmentW[] =
        {'#','d','o','c','u','m','e','n','t','-','f','r','a','g','m','e','n','t',0};

    TRACE("(%p)->(%p)\n", This, p);

    return return_bstr(document_fragmentW, p);
}

static HRESULT WINAPI domfrag_get_nodeValue(
    IXMLDOMDocumentFragment *iface,
    VARIANT* value)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    TRACE("(%p)->(%p)\n", This, value);
    return return_null_var(value);
}

static HRESULT WINAPI domfrag_put_nodeValue(
    IXMLDOMDocumentFragment *iface,
    VARIANT value)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    TRACE("(%p)->(%s)\n", This, debugstr_variant(&value));
    return E_FAIL;
}

static HRESULT WINAPI domfrag_get_nodeType(
    IXMLDOMDocumentFragment *iface,
    DOMNodeType* domNodeType )
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );

    TRACE("(%p)->(%p)\n", This, domNodeType);

    *domNodeType = NODE_DOCUMENT_FRAGMENT;
    return S_OK;
}

static HRESULT WINAPI domfrag_get_parentNode(
    IXMLDOMDocumentFragment *iface,
    IXMLDOMNode** parent )
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );

    TRACE("(%p)->(%p)\n", This, parent);

    return node_get_parent(&This->node, parent);
}

static HRESULT WINAPI domfrag_get_childNodes(
    IXMLDOMDocumentFragment *iface,
    IXMLDOMNodeList** outList)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );

    TRACE("(%p)->(%p)\n", This, outList);

    return node_get_child_nodes(&This->node, outList);
}

static HRESULT WINAPI domfrag_get_firstChild(
    IXMLDOMDocumentFragment *iface,
    IXMLDOMNode** domNode)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );

    TRACE("(%p)->(%p)\n", This, domNode);

    return node_get_first_child(&This->node, domNode);
}

static HRESULT WINAPI domfrag_get_lastChild(
    IXMLDOMDocumentFragment *iface,
    IXMLDOMNode** domNode)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );

    TRACE("(%p)->(%p)\n", This, domNode);

    return node_get_last_child(&This->node, domNode);
}

static HRESULT WINAPI domfrag_get_previousSibling(
    IXMLDOMDocumentFragment *iface,
    IXMLDOMNode** domNode)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );

    TRACE("(%p)->(%p)\n", This, domNode);

    return return_null_node(domNode);
}

static HRESULT WINAPI domfrag_get_nextSibling(
    IXMLDOMDocumentFragment *iface,
    IXMLDOMNode** domNode)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );

    TRACE("(%p)->(%p)\n", This, domNode);

    return return_null_node(domNode);
}

static HRESULT WINAPI domfrag_get_attributes(
    IXMLDOMDocumentFragment *iface,
    IXMLDOMNamedNodeMap** attributeMap)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );

    TRACE("(%p)->(%p)\n", This, attributeMap);

    return return_null_ptr((void**)attributeMap);
}

static HRESULT WINAPI domfrag_insertBefore(
    IXMLDOMDocumentFragment *iface,
    IXMLDOMNode* newNode, VARIANT refChild,
    IXMLDOMNode** outOldNode)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );

    TRACE("(%p)->(%p %s %p)\n", This, newNode, debugstr_variant(&refChild), outOldNode);

    /* TODO: test */
    return node_insert_before(&This->node, newNode, &refChild, outOldNode);
}

static HRESULT WINAPI domfrag_replaceChild(
    IXMLDOMDocumentFragment *iface,
    IXMLDOMNode* newNode,
    IXMLDOMNode* oldNode,
    IXMLDOMNode** outOldNode)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );

    TRACE("(%p)->(%p %p %p)\n", This, newNode, oldNode, outOldNode);

    /* TODO: test */
    return node_replace_child(&This->node, newNode, oldNode, outOldNode);
}

static HRESULT WINAPI domfrag_removeChild(
    IXMLDOMDocumentFragment *iface,
    IXMLDOMNode *child, IXMLDOMNode **oldChild)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    TRACE("(%p)->(%p %p)\n", This, child, oldChild);
    return node_remove_child(&This->node, child, oldChild);
}

static HRESULT WINAPI domfrag_appendChild(
    IXMLDOMDocumentFragment *iface,
    IXMLDOMNode *child, IXMLDOMNode **outChild)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    TRACE("(%p)->(%p %p)\n", This, child, outChild);
    return node_append_child(&This->node, child, outChild);
}

static HRESULT WINAPI domfrag_hasChildNodes(
    IXMLDOMDocumentFragment *iface,
    VARIANT_BOOL *ret)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    TRACE("(%p)->(%p)\n", This, ret);
    return node_has_childnodes(&This->node, ret);
}

static HRESULT WINAPI domfrag_get_ownerDocument(
    IXMLDOMDocumentFragment *iface,
    IXMLDOMDocument **doc)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    TRACE("(%p)->(%p)\n", This, doc);
    return node_get_owner_doc(&This->node, doc);
}

static HRESULT WINAPI domfrag_cloneNode(
    IXMLDOMDocumentFragment *iface,
    VARIANT_BOOL deep, IXMLDOMNode** outNode)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    TRACE("(%p)->(%d %p)\n", This, deep, outNode);
    return node_clone( &This->node, deep, outNode );
}

static HRESULT WINAPI domfrag_get_nodeTypeString(
    IXMLDOMDocumentFragment *iface,
    BSTR* p)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    static const WCHAR documentfragmentW[] = {'d','o','c','u','m','e','n','t','f','r','a','g','m','e','n','t',0};

    TRACE("(%p)->(%p)\n", This, p);

    return return_bstr(documentfragmentW, p);
}

static HRESULT WINAPI domfrag_get_text(
    IXMLDOMDocumentFragment *iface,
    BSTR* p)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    TRACE("(%p)->(%p)\n", This, p);
    return node_get_text(&This->node, p);
}

static HRESULT WINAPI domfrag_put_text(
    IXMLDOMDocumentFragment *iface,
    BSTR p)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    TRACE("(%p)->(%s)\n", This, debugstr_w(p));
    return node_put_text( &This->node, p );
}

static HRESULT WINAPI domfrag_get_specified(
    IXMLDOMDocumentFragment *iface,
    VARIANT_BOOL* isSpecified)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    FIXME("(%p)->(%p) stub!\n", This, isSpecified);
    *isSpecified = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI domfrag_get_definition(
    IXMLDOMDocumentFragment *iface,
    IXMLDOMNode** definitionNode)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    FIXME("(%p)->(%p)\n", This, definitionNode);
    return E_NOTIMPL;
}

static HRESULT WINAPI domfrag_get_nodeTypedValue(
    IXMLDOMDocumentFragment *iface,
    VARIANT *v)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    TRACE("(%p)->(%p)\n", This, v);
    return return_null_var(v);
}

static HRESULT WINAPI domfrag_put_nodeTypedValue(
    IXMLDOMDocumentFragment *iface,
    VARIANT typedValue)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&typedValue));
    return E_NOTIMPL;
}

static HRESULT WINAPI domfrag_get_dataType(
    IXMLDOMDocumentFragment *iface,
    VARIANT* typename)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    TRACE("(%p)->(%p)\n", This, typename);
    return return_null_var( typename );
}

static HRESULT WINAPI domfrag_put_dataType(
    IXMLDOMDocumentFragment *iface,
    BSTR p)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );

    TRACE("(%p)->(%s)\n", This, debugstr_w(p));

    if(!p)
        return E_INVALIDARG;

    return E_FAIL;
}

static HRESULT WINAPI domfrag_get_xml(
    IXMLDOMDocumentFragment *iface,
    BSTR* p)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );

    TRACE("(%p)->(%p)\n", This, p);

    return node_get_xml(&This->node, FALSE, p);
}

static HRESULT WINAPI domfrag_transformNode(
    IXMLDOMDocumentFragment *iface,
    IXMLDOMNode *node, BSTR *p)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    TRACE("(%p)->(%p %p)\n", This, node, p);
    return node_transform_node(&This->node, node, p);
}

static HRESULT WINAPI domfrag_selectNodes(
    IXMLDOMDocumentFragment *iface,
    BSTR p, IXMLDOMNodeList** outList)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    TRACE("(%p)->(%s %p)\n", This, debugstr_w(p), outList);
    return node_select_nodes(&This->node, p, outList);
}

static HRESULT WINAPI domfrag_selectSingleNode(
    IXMLDOMDocumentFragment *iface,
    BSTR p, IXMLDOMNode** outNode)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    TRACE("(%p)->(%s %p)\n", This, debugstr_w(p), outNode);
    return node_select_singlenode(&This->node, p, outNode);
}

static HRESULT WINAPI domfrag_get_parsed(
    IXMLDOMDocumentFragment *iface,
    VARIANT_BOOL* isParsed)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    FIXME("(%p)->(%p) stub!\n", This, isParsed);
    *isParsed = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI domfrag_get_namespaceURI(
    IXMLDOMDocumentFragment *iface,
    BSTR* p)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    TRACE("(%p)->(%p)\n", This, p);
    return node_get_namespaceURI(&This->node, p);
}

static HRESULT WINAPI domfrag_get_prefix(
    IXMLDOMDocumentFragment *iface,
    BSTR* prefix)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    TRACE("(%p)->(%p)\n", This, prefix);
    return return_null_bstr( prefix );
}

static HRESULT WINAPI domfrag_get_baseName(
    IXMLDOMDocumentFragment *iface,
    BSTR* name)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    FIXME("(%p)->(%p): needs test\n", This, name);
    return return_null_bstr( name );
}

static HRESULT WINAPI domfrag_transformNodeToObject(
    IXMLDOMDocumentFragment *iface,
    IXMLDOMNode* domNode, VARIANT var1)
{
    domfrag *This = impl_from_IXMLDOMDocumentFragment( iface );
    FIXME("(%p)->(%p %s)\n", This, domNode, debugstr_variant(&var1));
    return E_NOTIMPL;
}

static const struct IXMLDOMDocumentFragmentVtbl domfrag_vtbl =
{
    domfrag_QueryInterface,
    domfrag_AddRef,
    domfrag_Release,
    domfrag_GetTypeInfoCount,
    domfrag_GetTypeInfo,
    domfrag_GetIDsOfNames,
    domfrag_Invoke,
    domfrag_get_nodeName,
    domfrag_get_nodeValue,
    domfrag_put_nodeValue,
    domfrag_get_nodeType,
    domfrag_get_parentNode,
    domfrag_get_childNodes,
    domfrag_get_firstChild,
    domfrag_get_lastChild,
    domfrag_get_previousSibling,
    domfrag_get_nextSibling,
    domfrag_get_attributes,
    domfrag_insertBefore,
    domfrag_replaceChild,
    domfrag_removeChild,
    domfrag_appendChild,
    domfrag_hasChildNodes,
    domfrag_get_ownerDocument,
    domfrag_cloneNode,
    domfrag_get_nodeTypeString,
    domfrag_get_text,
    domfrag_put_text,
    domfrag_get_specified,
    domfrag_get_definition,
    domfrag_get_nodeTypedValue,
    domfrag_put_nodeTypedValue,
    domfrag_get_dataType,
    domfrag_put_dataType,
    domfrag_get_xml,
    domfrag_transformNode,
    domfrag_selectNodes,
    domfrag_selectSingleNode,
    domfrag_get_parsed,
    domfrag_get_namespaceURI,
    domfrag_get_prefix,
    domfrag_get_baseName,
    domfrag_transformNodeToObject
};

static const tid_t domfrag_iface_tids[] = {
    IXMLDOMDocumentFragment_tid,
    0
};

static dispex_static_data_t domfrag_dispex = {
    NULL,
    IXMLDOMDocumentFragment_tid,
    NULL,
    domfrag_iface_tids
};

IUnknown* create_doc_fragment( xmlNodePtr fragment )
{
    domfrag *This;

    This = malloc(sizeof(*This));
    if ( !This )
        return NULL;

    This->IXMLDOMDocumentFragment_iface.lpVtbl = &domfrag_vtbl;
    This->ref = 1;

    init_xmlnode(&This->node, fragment, (IXMLDOMNode*)&This->IXMLDOMDocumentFragment_iface, &domfrag_dispex);

    return (IUnknown*)&This->IXMLDOMDocumentFragment_iface;
}
