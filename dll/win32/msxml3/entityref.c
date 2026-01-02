/*
 *    DOM Entity Reference implementation
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

typedef struct _entityref
{
    xmlnode node;
    IXMLDOMEntityReference IXMLDOMEntityReference_iface;
    LONG ref;
} entityref;

static const tid_t domentityref_se_tids[] = {
    IXMLDOMNode_tid,
    IXMLDOMEntityReference_tid,
    NULL_tid
};

static inline entityref *impl_from_IXMLDOMEntityReference( IXMLDOMEntityReference *iface )
{
    return CONTAINING_RECORD(iface, entityref, IXMLDOMEntityReference_iface);
}

static HRESULT WINAPI entityref_QueryInterface(
    IXMLDOMEntityReference *iface,
    REFIID riid,
    void** ppvObject )
{
    entityref *This = impl_from_IXMLDOMEntityReference( iface );
    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppvObject);

    if ( IsEqualGUID( riid, &IID_IXMLDOMEntityReference ) ||
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
    else if (IsEqualGUID( riid, &IID_ISupportErrorInfo ))
    {
        return node_create_supporterrorinfo(domentityref_se_tids, ppvObject);
    }
    else
    {
        TRACE("Unsupported interface %s\n", debugstr_guid(riid));
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppvObject);
    return S_OK;
}

static ULONG WINAPI entityref_AddRef(
    IXMLDOMEntityReference *iface )
{
    entityref *This = impl_from_IXMLDOMEntityReference( iface );
    ULONG ref = InterlockedIncrement( &This->ref );
    TRACE("%p, refcount %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI entityref_Release(
    IXMLDOMEntityReference *iface )
{
    entityref *This = impl_from_IXMLDOMEntityReference( iface );
    ULONG ref = InterlockedDecrement( &This->ref );

    TRACE("%p, refcount %lu.\n", iface, ref);

    if (!ref)
    {
        destroy_xmlnode(&This->node);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI entityref_GetTypeInfoCount(
    IXMLDOMEntityReference *iface,
    UINT* pctinfo )
{
    entityref *This = impl_from_IXMLDOMEntityReference( iface );
    return IDispatchEx_GetTypeInfoCount(&This->node.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI entityref_GetTypeInfo(
    IXMLDOMEntityReference *iface,
    UINT iTInfo, LCID lcid,
    ITypeInfo** ppTInfo )
{
    entityref *This = impl_from_IXMLDOMEntityReference( iface );
    return IDispatchEx_GetTypeInfo(&This->node.dispex.IDispatchEx_iface,
        iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI entityref_GetIDsOfNames(
    IXMLDOMEntityReference *iface,
    REFIID riid, LPOLESTR* rgszNames,
    UINT cNames, LCID lcid, DISPID* rgDispId )
{
    entityref *This = impl_from_IXMLDOMEntityReference( iface );
    return IDispatchEx_GetIDsOfNames(&This->node.dispex.IDispatchEx_iface,
        riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI entityref_Invoke(
    IXMLDOMEntityReference *iface,
    DISPID dispIdMember, REFIID riid, LCID lcid,
    WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult,
    EXCEPINFO* pExcepInfo, UINT* puArgErr )
{
    entityref *This = impl_from_IXMLDOMEntityReference( iface );
    return IDispatchEx_Invoke(&This->node.dispex.IDispatchEx_iface,
        dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI entityref_get_nodeName(
    IXMLDOMEntityReference *iface,
    BSTR* p )
{
    entityref *This = impl_from_IXMLDOMEntityReference( iface );

    FIXME("(%p)->(%p)\n", This, p);

    return node_get_nodeName(&This->node, p);
}

static HRESULT WINAPI entityref_get_nodeValue(
    IXMLDOMEntityReference *iface,
    VARIANT* value)
{
    entityref *This = impl_from_IXMLDOMEntityReference( iface );
    TRACE("(%p)->(%p)\n", This, value);
    return return_null_var(value);
}

static HRESULT WINAPI entityref_put_nodeValue(
    IXMLDOMEntityReference *iface,
    VARIANT value)
{
    entityref *This = impl_from_IXMLDOMEntityReference( iface );
    TRACE("(%p)->(%s)\n", This, debugstr_variant(&value));
    return E_FAIL;
}

static HRESULT WINAPI entityref_get_nodeType(
    IXMLDOMEntityReference *iface,
    DOMNodeType* domNodeType )
{
    entityref *This = impl_from_IXMLDOMEntityReference( iface );

    TRACE("(%p)->(%p)\n", This, domNodeType);

    *domNodeType = NODE_ENTITY_REFERENCE;
    return S_OK;
}

static HRESULT WINAPI entityref_get_parentNode(
    IXMLDOMEntityReference *iface,
    IXMLDOMNode** parent )
{
    entityref *This = impl_from_IXMLDOMEntityReference( iface );

    TRACE("(%p)->(%p)\n", This, parent);

    return node_get_parent(&This->node, parent);
}

static HRESULT WINAPI entityref_get_childNodes(
    IXMLDOMEntityReference *iface,
    IXMLDOMNodeList** outList)
{
    entityref *This = impl_from_IXMLDOMEntityReference( iface );

    TRACE("(%p)->(%p)\n", This, outList);

    return node_get_child_nodes(&This->node, outList);
}

static HRESULT WINAPI entityref_get_firstChild(
    IXMLDOMEntityReference *iface,
    IXMLDOMNode** domNode)
{
    entityref *This = impl_from_IXMLDOMEntityReference( iface );

    TRACE("(%p)->(%p)\n", This, domNode);

    return node_get_first_child(&This->node, domNode);
}

static HRESULT WINAPI entityref_get_lastChild(
    IXMLDOMEntityReference *iface,
    IXMLDOMNode** domNode)
{
    entityref *This = impl_from_IXMLDOMEntityReference( iface );

    TRACE("(%p)->(%p)\n", This, domNode);

    return node_get_last_child(&This->node, domNode);
}

static HRESULT WINAPI entityref_get_previousSibling(
    IXMLDOMEntityReference *iface,
    IXMLDOMNode** domNode)
{
    entityref *This = impl_from_IXMLDOMEntityReference( iface );

    TRACE("(%p)->(%p)\n", This, domNode);

    return node_get_previous_sibling(&This->node, domNode);
}

static HRESULT WINAPI entityref_get_nextSibling(
    IXMLDOMEntityReference *iface,
    IXMLDOMNode** domNode)
{
    entityref *This = impl_from_IXMLDOMEntityReference( iface );

    TRACE("(%p)->(%p)\n", This, domNode);

    return node_get_next_sibling(&This->node, domNode);
}

static HRESULT WINAPI entityref_get_attributes(
    IXMLDOMEntityReference *iface,
    IXMLDOMNamedNodeMap** attributeMap)
{
    entityref *This = impl_from_IXMLDOMEntityReference( iface );

    TRACE("(%p)->(%p)\n", This, attributeMap);

    return return_null_ptr((void**)attributeMap);
}

static HRESULT WINAPI entityref_insertBefore(
    IXMLDOMEntityReference *iface,
    IXMLDOMNode* newNode, VARIANT refChild,
    IXMLDOMNode** outOldNode)
{
    entityref *This = impl_from_IXMLDOMEntityReference( iface );

    FIXME("(%p)->(%p %s %p) needs test\n", This, newNode, debugstr_variant(&refChild), outOldNode);

    return node_insert_before(&This->node, newNode, &refChild, outOldNode);
}

static HRESULT WINAPI entityref_replaceChild(
    IXMLDOMEntityReference *iface,
    IXMLDOMNode* newNode,
    IXMLDOMNode* oldNode,
    IXMLDOMNode** outOldNode)
{
    entityref *This = impl_from_IXMLDOMEntityReference( iface );

    FIXME("(%p)->(%p %p %p) needs test\n", This, newNode, oldNode, outOldNode);

    return node_replace_child(&This->node, newNode, oldNode, outOldNode);
}

static HRESULT WINAPI entityref_removeChild(
    IXMLDOMEntityReference *iface,
    IXMLDOMNode *child, IXMLDOMNode **oldChild)
{
    entityref *This = impl_from_IXMLDOMEntityReference( iface );
    TRACE("(%p)->(%p %p)\n", This, child, oldChild);
    return node_remove_child(&This->node, child, oldChild);
}

static HRESULT WINAPI entityref_appendChild(
    IXMLDOMEntityReference *iface,
    IXMLDOMNode *child, IXMLDOMNode **outChild)
{
    entityref *This = impl_from_IXMLDOMEntityReference( iface );
    TRACE("(%p)->(%p %p)\n", This, child, outChild);
    return node_append_child(&This->node, child, outChild);
}

static HRESULT WINAPI entityref_hasChildNodes(
    IXMLDOMEntityReference *iface,
    VARIANT_BOOL *ret)
{
    entityref *This = impl_from_IXMLDOMEntityReference( iface );
    TRACE("(%p)->(%p)\n", This, ret);
    return node_has_childnodes(&This->node, ret);
}

static HRESULT WINAPI entityref_get_ownerDocument(
    IXMLDOMEntityReference *iface,
    IXMLDOMDocument **doc)
{
    entityref *This = impl_from_IXMLDOMEntityReference( iface );
    TRACE("(%p)->(%p)\n", This, doc);
    return node_get_owner_doc(&This->node, doc);
}

static HRESULT WINAPI entityref_cloneNode(
    IXMLDOMEntityReference *iface,
    VARIANT_BOOL deep, IXMLDOMNode** outNode)
{
    entityref *This = impl_from_IXMLDOMEntityReference( iface );
    TRACE("(%p)->(%d %p)\n", This, deep, outNode);
    return node_clone( &This->node, deep, outNode );
}

static HRESULT WINAPI entityref_get_nodeTypeString(
    IXMLDOMEntityReference *iface,
    BSTR* p)
{
    entityref *This = impl_from_IXMLDOMEntityReference( iface );
    static const WCHAR entityreferenceW[] = {'e','n','t','i','t','y','r','e','f','e','r','e','n','c','e',0};

    TRACE("(%p)->(%p)\n", This, p);

    return return_bstr(entityreferenceW, p);
}

static HRESULT WINAPI entityref_get_text(
    IXMLDOMEntityReference *iface,
    BSTR* p)
{
    entityref *This = impl_from_IXMLDOMEntityReference( iface );
    TRACE("(%p)->(%p)\n", This, p);
    return node_get_text(&This->node, p);
}

static HRESULT WINAPI entityref_put_text(
    IXMLDOMEntityReference *iface,
    BSTR p)
{
    entityref *This = impl_from_IXMLDOMEntityReference( iface );
    TRACE("(%p)->(%s)\n", This, debugstr_w(p));
    return node_put_text( &This->node, p );
}

static HRESULT WINAPI entityref_get_specified(
    IXMLDOMEntityReference *iface,
    VARIANT_BOOL* isSpecified)
{
    entityref *This = impl_from_IXMLDOMEntityReference( iface );
    FIXME("(%p)->(%p) stub!\n", This, isSpecified);
    *isSpecified = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI entityref_get_definition(
    IXMLDOMEntityReference *iface,
    IXMLDOMNode** definitionNode)
{
    entityref *This = impl_from_IXMLDOMEntityReference( iface );
    FIXME("(%p)->(%p)\n", This, definitionNode);
    return E_NOTIMPL;
}

static HRESULT WINAPI entityref_get_nodeTypedValue(
    IXMLDOMEntityReference *iface,
    VARIANT* var1)
{
    entityref *This = impl_from_IXMLDOMEntityReference( iface );
    FIXME("(%p)->(%p)\n", This, var1);
    return return_null_var(var1);
}

static HRESULT WINAPI entityref_put_nodeTypedValue(
    IXMLDOMEntityReference *iface,
    VARIANT typedValue)
{
    entityref *This = impl_from_IXMLDOMEntityReference( iface );
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&typedValue));
    return E_NOTIMPL;
}

static HRESULT WINAPI entityref_get_dataType(
    IXMLDOMEntityReference *iface,
    VARIANT* typename)
{
    entityref *This = impl_from_IXMLDOMEntityReference( iface );
    FIXME("(%p)->(%p): should return a valid value\n", This, typename);
    return return_null_var( typename );
}

static HRESULT WINAPI entityref_put_dataType(
    IXMLDOMEntityReference *iface,
    BSTR p)
{
    entityref *This = impl_from_IXMLDOMEntityReference( iface );

    TRACE("(%p)->(%s)\n", This, debugstr_w(p));

    if(!p)
        return E_INVALIDARG;

    return E_FAIL;
}

static HRESULT WINAPI entityref_get_xml(
    IXMLDOMEntityReference *iface,
    BSTR* p)
{
    entityref *This = impl_from_IXMLDOMEntityReference( iface );

    TRACE("(%p)->(%p)\n", This, p);

    return node_get_xml(&This->node, FALSE, p);
}

static HRESULT WINAPI entityref_transformNode(
    IXMLDOMEntityReference *iface,
    IXMLDOMNode *node, BSTR *p)
{
    entityref *This = impl_from_IXMLDOMEntityReference( iface );
    TRACE("(%p)->(%p %p)\n", This, node, p);
    return node_transform_node(&This->node, node, p);
}

static HRESULT WINAPI entityref_selectNodes(
    IXMLDOMEntityReference *iface,
    BSTR p, IXMLDOMNodeList** outList)
{
    entityref *This = impl_from_IXMLDOMEntityReference( iface );
    TRACE("(%p)->(%s %p)\n", This, debugstr_w(p), outList);
    return node_select_nodes(&This->node, p, outList);
}

static HRESULT WINAPI entityref_selectSingleNode(
    IXMLDOMEntityReference *iface,
    BSTR p, IXMLDOMNode** outNode)
{
    entityref *This = impl_from_IXMLDOMEntityReference( iface );
    TRACE("(%p)->(%s %p)\n", This, debugstr_w(p), outNode);
    return node_select_singlenode(&This->node, p, outNode);
}

static HRESULT WINAPI entityref_get_parsed(
    IXMLDOMEntityReference *iface,
    VARIANT_BOOL* isParsed)
{
    entityref *This = impl_from_IXMLDOMEntityReference( iface );
    FIXME("(%p)->(%p) stub!\n", This, isParsed);
    *isParsed = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI entityref_get_namespaceURI(
    IXMLDOMEntityReference *iface,
    BSTR* p)
{
    entityref *This = impl_from_IXMLDOMEntityReference( iface );
    TRACE("(%p)->(%p)\n", This, p);
    return node_get_namespaceURI(&This->node, p);
}

static HRESULT WINAPI entityref_get_prefix(
    IXMLDOMEntityReference *iface,
    BSTR* prefix)
{
    entityref *This = impl_from_IXMLDOMEntityReference( iface );
    FIXME("(%p)->(%p): stub\n", This, prefix);
    return return_null_bstr( prefix );
}

static HRESULT WINAPI entityref_get_baseName(
    IXMLDOMEntityReference *iface,
    BSTR* name)
{
    entityref *This = impl_from_IXMLDOMEntityReference( iface );
    FIXME("(%p)->(%p): needs test\n", This, name);
    return return_null_bstr( name );
}

static HRESULT WINAPI entityref_transformNodeToObject(
    IXMLDOMEntityReference *iface,
    IXMLDOMNode* domNode, VARIANT var1)
{
    entityref *This = impl_from_IXMLDOMEntityReference( iface );
    FIXME("(%p)->(%p %s)\n", This, domNode, debugstr_variant(&var1));
    return E_NOTIMPL;
}

static const struct IXMLDOMEntityReferenceVtbl entityref_vtbl =
{
    entityref_QueryInterface,
    entityref_AddRef,
    entityref_Release,
    entityref_GetTypeInfoCount,
    entityref_GetTypeInfo,
    entityref_GetIDsOfNames,
    entityref_Invoke,
    entityref_get_nodeName,
    entityref_get_nodeValue,
    entityref_put_nodeValue,
    entityref_get_nodeType,
    entityref_get_parentNode,
    entityref_get_childNodes,
    entityref_get_firstChild,
    entityref_get_lastChild,
    entityref_get_previousSibling,
    entityref_get_nextSibling,
    entityref_get_attributes,
    entityref_insertBefore,
    entityref_replaceChild,
    entityref_removeChild,
    entityref_appendChild,
    entityref_hasChildNodes,
    entityref_get_ownerDocument,
    entityref_cloneNode,
    entityref_get_nodeTypeString,
    entityref_get_text,
    entityref_put_text,
    entityref_get_specified,
    entityref_get_definition,
    entityref_get_nodeTypedValue,
    entityref_put_nodeTypedValue,
    entityref_get_dataType,
    entityref_put_dataType,
    entityref_get_xml,
    entityref_transformNode,
    entityref_selectNodes,
    entityref_selectSingleNode,
    entityref_get_parsed,
    entityref_get_namespaceURI,
    entityref_get_prefix,
    entityref_get_baseName,
    entityref_transformNodeToObject,
};

static const tid_t domentityref_iface_tids[] = {
    IXMLDOMEntityReference_tid,
    0
};

static dispex_static_data_t domentityref_dispex = {
    NULL,
    IXMLDOMEntityReference_tid,
    NULL,
    domentityref_iface_tids
};

IUnknown* create_doc_entity_ref( xmlNodePtr entity )
{
    entityref *This;

    This = malloc(sizeof(*This));
    if ( !This )
        return NULL;

    This->IXMLDOMEntityReference_iface.lpVtbl = &entityref_vtbl;
    This->ref = 1;

    init_xmlnode(&This->node, entity, (IXMLDOMNode*)&This->IXMLDOMEntityReference_iface, &domentityref_dispex);

    return (IUnknown*)&This->IXMLDOMEntityReference_iface;
}
