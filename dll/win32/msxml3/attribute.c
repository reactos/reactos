/*
 *    DOM Attribute implementation
 *
 * Copyright 2006 Huw Davies
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

#include "config.h"

#include <stdarg.h>
#ifdef HAVE_LIBXML2
# include <libxml/parser.h>
# include <libxml/xmlerror.h>
# include <libxml/HTMLtree.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "msxml6.h"

#include "msxml_private.h"

#include "wine/debug.h"

#ifdef HAVE_LIBXML2

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

static const xmlChar xmlns[] = "xmlns";

typedef struct _domattr
{
    xmlnode node;
    IXMLDOMAttribute IXMLDOMAttribute_iface;
    LONG ref;
    BOOL floating;
} domattr;

static const tid_t domattr_se_tids[] = {
    IXMLDOMNode_tid,
    IXMLDOMAttribute_tid,
    NULL_tid
};

static inline domattr *impl_from_IXMLDOMAttribute( IXMLDOMAttribute *iface )
{
    return CONTAINING_RECORD(iface, domattr, IXMLDOMAttribute_iface);
}

static HRESULT WINAPI domattr_QueryInterface(
    IXMLDOMAttribute *iface,
    REFIID riid,
    void** ppvObject )
{
    domattr *This = impl_from_IXMLDOMAttribute( iface );
    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppvObject);

    if ( IsEqualGUID( riid, &IID_IXMLDOMAttribute ) ||
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
        return node_create_supporterrorinfo(domattr_se_tids, ppvObject);
    }
    else
    {
        TRACE("Unsupported interface %s\n", debugstr_guid(riid));
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    IXMLDOMAttribute_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI domattr_AddRef(
    IXMLDOMAttribute *iface )
{
    domattr *This = impl_from_IXMLDOMAttribute( iface );
    ULONG ref = InterlockedIncrement( &This->ref );
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI domattr_Release(
    IXMLDOMAttribute *iface )
{
    domattr *This = impl_from_IXMLDOMAttribute( iface );
    ULONG ref = InterlockedDecrement( &This->ref );

    TRACE("(%p)->(%d)\n", This, ref);
    if ( ref == 0 )
    {
        destroy_xmlnode(&This->node);
        if ( This->floating )
        {
            xmlFreeNs( This->node.node->ns );
            xmlFreeNode( This->node.node );
        }
        heap_free( This );
    }

    return ref;
}

static HRESULT WINAPI domattr_GetTypeInfoCount(
    IXMLDOMAttribute *iface,
    UINT* pctinfo )
{
    domattr *This = impl_from_IXMLDOMAttribute( iface );
    return IDispatchEx_GetTypeInfoCount(&This->node.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI domattr_GetTypeInfo(
    IXMLDOMAttribute *iface,
    UINT iTInfo, LCID lcid,
    ITypeInfo** ppTInfo )
{
    domattr *This = impl_from_IXMLDOMAttribute( iface );
    return IDispatchEx_GetTypeInfo(&This->node.dispex.IDispatchEx_iface,
        iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI domattr_GetIDsOfNames(
    IXMLDOMAttribute *iface,
    REFIID riid, LPOLESTR* rgszNames,
    UINT cNames, LCID lcid, DISPID* rgDispId )
{
    domattr *This = impl_from_IXMLDOMAttribute( iface );
    return IDispatchEx_GetIDsOfNames(&This->node.dispex.IDispatchEx_iface,
        riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI domattr_Invoke(
    IXMLDOMAttribute *iface,
    DISPID dispIdMember, REFIID riid, LCID lcid,
    WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult,
    EXCEPINFO* pExcepInfo, UINT* puArgErr )
{
    domattr *This = impl_from_IXMLDOMAttribute( iface );
    return IDispatchEx_Invoke(&This->node.dispex.IDispatchEx_iface,
        dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI domattr_get_nodeName(
    IXMLDOMAttribute *iface,
    BSTR* p )
{
    domattr *This = impl_from_IXMLDOMAttribute( iface );

    TRACE("(%p)->(%p)\n", This, p);

    return node_get_nodeName(&This->node, p);
}

static HRESULT WINAPI domattr_get_nodeValue(
    IXMLDOMAttribute *iface,
    VARIANT* value)
{
    domattr *This = impl_from_IXMLDOMAttribute( iface );

    TRACE("(%p)->(%p)\n", This, value);

    return node_get_content(&This->node, value);
}

static HRESULT WINAPI domattr_put_nodeValue(
    IXMLDOMAttribute *iface,
    VARIANT value)
{
    domattr *This = impl_from_IXMLDOMAttribute( iface );

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&value));

    return node_put_value_escaped(&This->node, &value);
}

static HRESULT WINAPI domattr_get_nodeType(
    IXMLDOMAttribute *iface,
    DOMNodeType* domNodeType )
{
    domattr *This = impl_from_IXMLDOMAttribute( iface );

    TRACE("(%p)->(%p)\n", This, domNodeType);

    *domNodeType = NODE_ATTRIBUTE;
    return S_OK;
}

static HRESULT WINAPI domattr_get_parentNode(
    IXMLDOMAttribute *iface,
    IXMLDOMNode** parent )
{
    domattr *This = impl_from_IXMLDOMAttribute( iface );
    TRACE("(%p)->(%p)\n", This, parent);
    if (!parent) return E_INVALIDARG;
    *parent = NULL;
    return S_FALSE;
}

static HRESULT WINAPI domattr_get_childNodes(
    IXMLDOMAttribute *iface,
    IXMLDOMNodeList** outList)
{
    domattr *This = impl_from_IXMLDOMAttribute( iface );

    TRACE("(%p)->(%p)\n", This, outList);

    return node_get_child_nodes(&This->node, outList);
}

static HRESULT WINAPI domattr_get_firstChild(
    IXMLDOMAttribute *iface,
    IXMLDOMNode** domNode)
{
    domattr *This = impl_from_IXMLDOMAttribute( iface );

    TRACE("(%p)->(%p)\n", This, domNode);

    return node_get_first_child(&This->node, domNode);
}

static HRESULT WINAPI domattr_get_lastChild(
    IXMLDOMAttribute *iface,
    IXMLDOMNode** domNode)
{
    domattr *This = impl_from_IXMLDOMAttribute( iface );

    TRACE("(%p)->(%p)\n", This, domNode);

    return node_get_last_child(&This->node, domNode);
}

static HRESULT WINAPI domattr_get_previousSibling(
    IXMLDOMAttribute *iface,
    IXMLDOMNode** domNode)
{
    domattr *This = impl_from_IXMLDOMAttribute( iface );

    TRACE("(%p)->(%p)\n", This, domNode);

    return return_null_node(domNode);
}

static HRESULT WINAPI domattr_get_nextSibling(
    IXMLDOMAttribute *iface,
    IXMLDOMNode** domNode)
{
    domattr *This = impl_from_IXMLDOMAttribute( iface );

    TRACE("(%p)->(%p)\n", This, domNode);

    return return_null_node(domNode);
}

static HRESULT WINAPI domattr_get_attributes(
    IXMLDOMAttribute *iface,
    IXMLDOMNamedNodeMap** attributeMap)
{
    domattr *This = impl_from_IXMLDOMAttribute( iface );

    TRACE("(%p)->(%p)\n", This, attributeMap);

    return return_null_ptr((void**)attributeMap);
}

static HRESULT WINAPI domattr_insertBefore(
    IXMLDOMAttribute *iface,
    IXMLDOMNode* newNode, VARIANT refChild,
    IXMLDOMNode** old_node)
{
    domattr *This = impl_from_IXMLDOMAttribute( iface );
    DOMNodeType type;
    HRESULT hr;

    FIXME("(%p)->(%p %s %p) needs test\n", This, newNode, debugstr_variant(&refChild), old_node);

    if (!newNode) return E_INVALIDARG;

    hr = IXMLDOMNode_get_nodeType(newNode, &type);
    if (hr != S_OK) return hr;

    TRACE("new node type %d\n", type);
    switch (type)
    {
        case NODE_ATTRIBUTE:
        case NODE_CDATA_SECTION:
        case NODE_COMMENT:
        case NODE_ELEMENT:
        case NODE_PROCESSING_INSTRUCTION:
            if (old_node) *old_node = NULL;
            return E_FAIL;
        default:
            return node_insert_before(&This->node, newNode, &refChild, old_node);
    }
}

static HRESULT WINAPI domattr_replaceChild(
    IXMLDOMAttribute *iface,
    IXMLDOMNode* newNode,
    IXMLDOMNode* oldNode,
    IXMLDOMNode** outOldNode)
{
    domattr *This = impl_from_IXMLDOMAttribute( iface );

    FIXME("(%p)->(%p %p %p) needs tests\n", This, newNode, oldNode, outOldNode);

    return node_replace_child(&This->node, newNode, oldNode, outOldNode);
}

static HRESULT WINAPI domattr_removeChild(
    IXMLDOMAttribute *iface,
    IXMLDOMNode *child, IXMLDOMNode **oldChild)
{
    domattr *This = impl_from_IXMLDOMAttribute( iface );
    TRACE("(%p)->(%p %p)\n", This, child, oldChild);
    return node_remove_child(&This->node, child, oldChild);
}

static HRESULT WINAPI domattr_appendChild(
    IXMLDOMAttribute *iface,
    IXMLDOMNode *child, IXMLDOMNode **outChild)
{
    domattr *This = impl_from_IXMLDOMAttribute( iface );
    TRACE("(%p)->(%p %p)\n", This, child, outChild);
    return node_append_child(&This->node, child, outChild);
}

static HRESULT WINAPI domattr_hasChildNodes(
    IXMLDOMAttribute *iface,
    VARIANT_BOOL *ret)
{
    domattr *This = impl_from_IXMLDOMAttribute( iface );
    TRACE("(%p)->(%p)\n", This, ret);
    return node_has_childnodes(&This->node, ret);
}

static HRESULT WINAPI domattr_get_ownerDocument(
    IXMLDOMAttribute *iface,
    IXMLDOMDocument **doc)
{
    domattr *This = impl_from_IXMLDOMAttribute( iface );
    TRACE("(%p)->(%p)\n", This, doc);
    return node_get_owner_doc(&This->node, doc);
}

static HRESULT WINAPI domattr_cloneNode(
    IXMLDOMAttribute *iface,
    VARIANT_BOOL deep, IXMLDOMNode** outNode)
{
    domattr *This = impl_from_IXMLDOMAttribute( iface );
    TRACE("(%p)->(%d %p)\n", This, deep, outNode);
    return node_clone( &This->node, deep, outNode );
}

static HRESULT WINAPI domattr_get_nodeTypeString(
    IXMLDOMAttribute *iface,
    BSTR* p)
{
    domattr *This = impl_from_IXMLDOMAttribute( iface );
    static const WCHAR attributeW[] = {'a','t','t','r','i','b','u','t','e',0};

    TRACE("(%p)->(%p)\n", This, p);

    return return_bstr(attributeW, p);
}

static HRESULT WINAPI domattr_get_text(
    IXMLDOMAttribute *iface,
    BSTR* p)
{
    domattr *This = impl_from_IXMLDOMAttribute( iface );
    TRACE("(%p)->(%p)\n", This, p);
    return node_get_text(&This->node, p);
}

static HRESULT WINAPI domattr_put_text(
    IXMLDOMAttribute *iface,
    BSTR p)
{
    domattr *This = impl_from_IXMLDOMAttribute( iface );
    TRACE("(%p)->(%s)\n", This, debugstr_w(p));
    return node_put_text( &This->node, p );
}

static HRESULT WINAPI domattr_get_specified(
    IXMLDOMAttribute *iface,
    VARIANT_BOOL* isSpecified)
{
    domattr *This = impl_from_IXMLDOMAttribute( iface );
    FIXME("(%p)->(%p) stub!\n", This, isSpecified);
    *isSpecified = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI domattr_get_definition(
    IXMLDOMAttribute *iface,
    IXMLDOMNode** definitionNode)
{
    domattr *This = impl_from_IXMLDOMAttribute( iface );
    FIXME("(%p)->(%p)\n", This, definitionNode);
    return E_NOTIMPL;
}

static HRESULT WINAPI domattr_get_nodeTypedValue(
    IXMLDOMAttribute *iface,
    VARIANT* value)
{
    domattr *This = impl_from_IXMLDOMAttribute( iface );
    IXMLDOMDocument *doc;
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, value);

    hr = IXMLDOMAttribute_get_ownerDocument(iface, &doc);
    if (hr == S_OK)
    {
        IXMLDOMDocument3 *doc3;

        hr = IXMLDOMDocument_QueryInterface(doc, &IID_IXMLDOMDocument3, (void**)&doc3);
        IXMLDOMDocument_Release(doc);

        if (hr == S_OK)
        {
            VARIANT schemas;

            hr = IXMLDOMDocument3_get_schemas(doc3, &schemas);
            IXMLDOMDocument3_Release(doc3);

            if (hr != S_OK)
                return IXMLDOMAttribute_get_value(iface, value);
            else
            {
                FIXME("need to query schema for attribute type\n");
                VariantClear(&schemas);
            }
        }
    }

    return return_null_var(value);
}

static HRESULT WINAPI domattr_put_nodeTypedValue(
    IXMLDOMAttribute *iface,
    VARIANT typedValue)
{
    domattr *This = impl_from_IXMLDOMAttribute( iface );
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&typedValue));
    return E_NOTIMPL;
}

static HRESULT WINAPI domattr_get_dataType(
    IXMLDOMAttribute *iface,
    VARIANT* typename)
{
    domattr *This = impl_from_IXMLDOMAttribute( iface );
    TRACE("(%p)->(%p)\n", This, typename);
    return return_null_var( typename );
}

static HRESULT WINAPI domattr_put_dataType(
    IXMLDOMAttribute *iface,
    BSTR p)
{
    domattr *This = impl_from_IXMLDOMAttribute( iface );

    FIXME("(%p)->(%s)\n", This, debugstr_w(p));

    if(!p)
        return E_INVALIDARG;

    return E_FAIL;
}

static HRESULT WINAPI domattr_get_xml(
    IXMLDOMAttribute *iface,
    BSTR* p)
{
    domattr *This = impl_from_IXMLDOMAttribute( iface );

    TRACE("(%p)->(%p)\n", This, p);

    return node_get_xml(&This->node, FALSE, p);
}

static HRESULT WINAPI domattr_transformNode(
    IXMLDOMAttribute *iface,
    IXMLDOMNode *node, BSTR *p)
{
    domattr *This = impl_from_IXMLDOMAttribute( iface );
    TRACE("(%p)->(%p %p)\n", This, node, p);
    return node_transform_node(&This->node, node, p);
}

static HRESULT WINAPI domattr_selectNodes(
    IXMLDOMAttribute *iface,
    BSTR p, IXMLDOMNodeList** outList)
{
    domattr *This = impl_from_IXMLDOMAttribute( iface );
    TRACE("(%p)->(%s %p)\n", This, debugstr_w(p), outList);
    return node_select_nodes(&This->node, p, outList);
}

static HRESULT WINAPI domattr_selectSingleNode(
    IXMLDOMAttribute *iface,
    BSTR p, IXMLDOMNode** outNode)
{
    domattr *This = impl_from_IXMLDOMAttribute( iface );
    TRACE("(%p)->(%s %p)\n", This, debugstr_w(p), outNode);
    return node_select_singlenode(&This->node, p, outNode);
}

static HRESULT WINAPI domattr_get_parsed(
    IXMLDOMAttribute *iface,
    VARIANT_BOOL* isParsed)
{
    domattr *This = impl_from_IXMLDOMAttribute( iface );
    FIXME("(%p)->(%p) stub!\n", This, isParsed);
    *isParsed = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI domattr_get_namespaceURI(
    IXMLDOMAttribute *iface,
    BSTR* p)
{
    static const WCHAR w3xmlns[] = { 'h','t','t','p',':','/','/', 'w','w','w','.','w','3','.',
        'o','r','g','/','2','0','0','0','/','x','m','l','n','s','/',0 };
    domattr *This = impl_from_IXMLDOMAttribute( iface );
    xmlNsPtr ns = This->node.node->ns;

    TRACE("(%p)->(%p)\n", This, p);

    if (!p)
        return E_INVALIDARG;

    *p = NULL;

    if (ns)
    {
        /* special case for default namespace definition */
        if (xmlStrEqual(This->node.node->name, xmlns))
            *p = bstr_from_xmlChar(xmlns);
        else if (xmlStrEqual(ns->prefix, xmlns))
        {
            if (xmldoc_version(This->node.node->doc) == MSXML6)
                *p = SysAllocString(w3xmlns);
            else
                *p = SysAllocStringLen(NULL, 0);
        }
        else if (ns->href)
            *p = bstr_from_xmlChar(ns->href);
    }

    TRACE("uri: %s\n", debugstr_w(*p));

    return *p ? S_OK : S_FALSE;
}

static HRESULT WINAPI domattr_get_prefix(
    IXMLDOMAttribute *iface,
    BSTR* prefix)
{
    domattr *This = impl_from_IXMLDOMAttribute( iface );
    xmlNsPtr ns = This->node.node->ns;

    TRACE("(%p)->(%p)\n", This, prefix);

    if (!prefix) return E_INVALIDARG;

    *prefix = NULL;

    if (ns)
    {
        /* special case for default namespace definition */
        if (xmlStrEqual(This->node.node->name, xmlns))
            *prefix = bstr_from_xmlChar(xmlns);
        else if (ns->prefix)
            *prefix = bstr_from_xmlChar(ns->prefix);
    }

    TRACE("prefix: %s\n", debugstr_w(*prefix));

    return *prefix ? S_OK : S_FALSE;
}

static HRESULT WINAPI domattr_get_baseName(
    IXMLDOMAttribute *iface,
    BSTR* name)
{
    domattr *This = impl_from_IXMLDOMAttribute( iface );
    TRACE("(%p)->(%p)\n", This, name);
    return node_get_base_name( &This->node, name );
}

static HRESULT WINAPI domattr_transformNodeToObject(
    IXMLDOMAttribute *iface,
    IXMLDOMNode* domNode, VARIANT var1)
{
    domattr *This = impl_from_IXMLDOMAttribute( iface );
    FIXME("(%p)->(%p %s)\n", This, domNode, debugstr_variant(&var1));
    return E_NOTIMPL;
}

static HRESULT WINAPI domattr_get_name(
    IXMLDOMAttribute *iface,
    BSTR *p)
{
    domattr *This = impl_from_IXMLDOMAttribute( iface );

    TRACE("(%p)->(%p)\n", This, p);

    return node_get_nodeName(&This->node, p);
}

static HRESULT WINAPI domattr_get_value(
    IXMLDOMAttribute *iface,
    VARIANT *value)
{
    domattr *This = impl_from_IXMLDOMAttribute( iface );

    TRACE("(%p)->(%p)\n", This, value);

    return node_get_content(&This->node, value);
}

static HRESULT WINAPI domattr_put_value(
    IXMLDOMAttribute *iface,
    VARIANT value)
{
    domattr *This = impl_from_IXMLDOMAttribute( iface );

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&value));

    return node_put_value_escaped(&This->node, &value);
}

static const struct IXMLDOMAttributeVtbl domattr_vtbl =
{
    domattr_QueryInterface,
    domattr_AddRef,
    domattr_Release,
    domattr_GetTypeInfoCount,
    domattr_GetTypeInfo,
    domattr_GetIDsOfNames,
    domattr_Invoke,
    domattr_get_nodeName,
    domattr_get_nodeValue,
    domattr_put_nodeValue,
    domattr_get_nodeType,
    domattr_get_parentNode,
    domattr_get_childNodes,
    domattr_get_firstChild,
    domattr_get_lastChild,
    domattr_get_previousSibling,
    domattr_get_nextSibling,
    domattr_get_attributes,
    domattr_insertBefore,
    domattr_replaceChild,
    domattr_removeChild,
    domattr_appendChild,
    domattr_hasChildNodes,
    domattr_get_ownerDocument,
    domattr_cloneNode,
    domattr_get_nodeTypeString,
    domattr_get_text,
    domattr_put_text,
    domattr_get_specified,
    domattr_get_definition,
    domattr_get_nodeTypedValue,
    domattr_put_nodeTypedValue,
    domattr_get_dataType,
    domattr_put_dataType,
    domattr_get_xml,
    domattr_transformNode,
    domattr_selectNodes,
    domattr_selectSingleNode,
    domattr_get_parsed,
    domattr_get_namespaceURI,
    domattr_get_prefix,
    domattr_get_baseName,
    domattr_transformNodeToObject,
    domattr_get_name,
    domattr_get_value,
    domattr_put_value
};

static const tid_t domattr_iface_tids[] = {
    IXMLDOMAttribute_tid,
    0
};

static dispex_static_data_t domattr_dispex = {
    NULL,
    IXMLDOMAttribute_tid,
    NULL,
    domattr_iface_tids
};

IUnknown* create_attribute( xmlNodePtr attribute, BOOL floating )
{
    domattr *This;

    This = heap_alloc( sizeof *This );
    if ( !This )
        return NULL;

    This->IXMLDOMAttribute_iface.lpVtbl = &domattr_vtbl;
    This->ref = 1;
    This->floating = floating;

    init_xmlnode(&This->node, attribute, (IXMLDOMNode*)&This->IXMLDOMAttribute_iface, &domattr_dispex);

    return (IUnknown*)&This->IXMLDOMAttribute_iface;
}

#endif
