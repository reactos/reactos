/*
 *    Node implementation
 *
 * Copyright 2005 Mike McCormack
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

#include "config.h"

#define COBJMACROS

#include <stdarg.h>
#include <assert.h>
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winnls.h"
#include "ole2.h"
#include "msxml2.h"

#include "msxml_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

#ifdef HAVE_LIBXML2

typedef struct _xmlnode
{
    const struct IXMLDOMNodeVtbl *lpVtbl;
    const struct IUnknownVtbl *lpInternalUnkVtbl;
    IUnknown *pUnkOuter;
    LONG ref;
    xmlNodePtr node;
} xmlnode;

static inline xmlnode *impl_from_IXMLDOMNode( IXMLDOMNode *iface )
{
    return (xmlnode *)((char*)iface - FIELD_OFFSET(xmlnode, lpVtbl));
}

static inline xmlnode *impl_from_InternalUnknown( IUnknown *iface )
{
    return (xmlnode *)((char*)iface - FIELD_OFFSET(xmlnode, lpInternalUnkVtbl));
}

xmlNodePtr xmlNodePtr_from_domnode( IXMLDOMNode *iface, xmlElementType type )
{
    xmlnode *This;

    if ( !iface )
        return NULL;
    This = impl_from_IXMLDOMNode( iface );
    if ( !This->node )
        return NULL;
    if ( type && This->node->type != type )
        return NULL;
    return This->node;
}

void attach_xmlnode( IXMLDOMNode *node, xmlNodePtr xml )
{
    xmlnode *This = impl_from_IXMLDOMNode( node );

    if(This->node)
        xmldoc_release(This->node->doc);

    This->node = xml;
    if(This->node)
        xmldoc_add_ref(This->node->doc);

    return;
}

static HRESULT WINAPI xmlnode_QueryInterface(
    IXMLDOMNode *iface,
    REFIID riid,
    void** ppvObject )
{
    xmlnode *This = impl_from_IXMLDOMNode( iface );
    TRACE("%p %s %p\n", This, debugstr_guid(riid), ppvObject);

    return IUnknown_QueryInterface(This->pUnkOuter, riid, ppvObject);
}

static ULONG WINAPI xmlnode_AddRef(
    IXMLDOMNode *iface )
{
    xmlnode *This = impl_from_IXMLDOMNode( iface );
    return IUnknown_AddRef(This->pUnkOuter);
}

static ULONG WINAPI xmlnode_Release(
    IXMLDOMNode *iface )
{
    xmlnode *This = impl_from_IXMLDOMNode( iface );
    return IUnknown_Release(This->pUnkOuter);
}

static HRESULT WINAPI xmlnode_GetTypeInfoCount(
    IXMLDOMNode *iface,
    UINT* pctinfo )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_GetTypeInfo(
    IXMLDOMNode *iface,
    UINT iTInfo,
    LCID lcid,
    ITypeInfo** ppTInfo )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_GetIDsOfNames(
    IXMLDOMNode *iface,
    REFIID riid,
    LPOLESTR* rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID* rgDispId )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_Invoke(
    IXMLDOMNode *iface,
    DISPID dispIdMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS* pDispParams,
    VARIANT* pVarResult,
    EXCEPINFO* pExcepInfo,
    UINT* puArgErr )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_get_nodeName(
    IXMLDOMNode *iface,
    BSTR* name)
{
    xmlnode *This = impl_from_IXMLDOMNode( iface );
    const xmlChar *str;

    TRACE("%p\n", This );

    if (!name)
        return E_INVALIDARG;

    if ( !This->node )
        return E_FAIL;

    switch( This->node->type )
    {
    case XML_TEXT_NODE:
        str = (const xmlChar*) "#text";
        break;
    case XML_DOCUMENT_NODE:
        str = (const xmlChar*) "#document";
	break;
    default:
        str = This->node->name;
        break;
    }

    *name = bstr_from_xmlChar( str );
    if (!*name)
        return S_FALSE;

    return S_OK;
}

BSTR bstr_from_xmlChar( const xmlChar *buf )
{
    DWORD len;
    LPWSTR str;
    BSTR bstr;

    if ( !buf )
        return NULL;

    len = MultiByteToWideChar( CP_UTF8, 0, (LPCSTR) buf, -1, NULL, 0 );
    str = (LPWSTR) HeapAlloc( GetProcessHeap(), 0, len * sizeof (WCHAR) );
    if ( !str )
        return NULL;
    MultiByteToWideChar( CP_UTF8, 0, (LPCSTR) buf, -1, str, len );
    bstr = SysAllocString( str );
    HeapFree( GetProcessHeap(), 0, str );
    return bstr;
}

static HRESULT WINAPI xmlnode_get_nodeValue(
    IXMLDOMNode *iface,
    VARIANT* value)
{
    xmlnode *This = impl_from_IXMLDOMNode( iface );
    HRESULT r = S_FALSE;

    TRACE("%p %p\n", This, value);

    V_BSTR(value) = NULL;
    V_VT(value) = VT_NULL;

    switch ( This->node->type )
    {
    case XML_ATTRIBUTE_NODE:
      {
        xmlChar *content = xmlNodeGetContent(This->node);
        V_VT(value) = VT_BSTR;
        V_BSTR(value) = bstr_from_xmlChar( content );
        xmlFree(content);
        r = S_OK;
        break;
      }
    case XML_TEXT_NODE:
        V_VT(value) = VT_BSTR;
        V_BSTR(value) = bstr_from_xmlChar( This->node->content );
        r = S_OK;
        break;
    case XML_ELEMENT_NODE:
    case XML_DOCUMENT_NODE:
        /* these seem to return NULL */
        break;
    case XML_PI_NODE:
    default:
        FIXME("node %p type %d\n", This, This->node->type);
    }
 
    TRACE("%p returned %s\n", This, debugstr_w( V_BSTR(value) ) );

    return r;
}

static HRESULT WINAPI xmlnode_put_nodeValue(
    IXMLDOMNode *iface,
    VARIANT value)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_get_nodeType(
    IXMLDOMNode *iface,
    DOMNodeType* type)
{
    xmlnode *This = impl_from_IXMLDOMNode( iface );

    TRACE("%p %p\n", This, type);

    assert( NODE_ELEMENT == XML_ELEMENT_NODE );
    assert( NODE_NOTATION == XML_NOTATION_NODE );

    *type = This->node->type;

    return S_OK;
}

static HRESULT get_node(
    xmlnode *This,
    const char *name,
    xmlNodePtr node,
    IXMLDOMNode **out )
{
    TRACE("%p->%s %p\n", This, name, node );

    if ( !out )
        return E_INVALIDARG;
    *out = create_node( node );
    if (!*out)
        return S_FALSE;
    return S_OK;
}

static HRESULT WINAPI xmlnode_get_parentNode(
    IXMLDOMNode *iface,
    IXMLDOMNode** parent)
{
    xmlnode *This = impl_from_IXMLDOMNode( iface );
    return get_node( This, "parent", This->node->parent, parent );
}

static HRESULT WINAPI xmlnode_get_childNodes(
    IXMLDOMNode *iface,
    IXMLDOMNodeList** childList)
{
    xmlnode *This = impl_from_IXMLDOMNode( iface );

    TRACE("%p %p\n", This, childList );

    if ( !childList )
        return E_INVALIDARG;

    *childList = create_children_nodelist(This->node);
    if (*childList == NULL)
        return E_OUTOFMEMORY;

    return S_OK;
}

static HRESULT WINAPI xmlnode_get_firstChild(
    IXMLDOMNode *iface,
    IXMLDOMNode** firstChild)
{
    xmlnode *This = impl_from_IXMLDOMNode( iface );
    return get_node( This, "firstChild", This->node->children, firstChild );
}

static HRESULT WINAPI xmlnode_get_lastChild(
    IXMLDOMNode *iface,
    IXMLDOMNode** lastChild)
{
    xmlnode *This = impl_from_IXMLDOMNode( iface );
    return get_node( This, "lastChild", This->node->last, lastChild );
}

static HRESULT WINAPI xmlnode_get_previousSibling(
    IXMLDOMNode *iface,
    IXMLDOMNode** previousSibling)
{
    xmlnode *This = impl_from_IXMLDOMNode( iface );
    return get_node( This, "previous", This->node->prev, previousSibling );
}

static HRESULT WINAPI xmlnode_get_nextSibling(
    IXMLDOMNode *iface,
    IXMLDOMNode** nextSibling)
{
    xmlnode *This = impl_from_IXMLDOMNode( iface );
    return get_node( This, "next", This->node->next, nextSibling );
}

static HRESULT WINAPI xmlnode_get_attributes(
    IXMLDOMNode *iface,
    IXMLDOMNamedNodeMap** attributeMap)
{
    xmlnode *This = impl_from_IXMLDOMNode( iface );
    TRACE("%p\n", This);
    *attributeMap = create_nodemap( iface );
    return S_OK;
}

static HRESULT WINAPI xmlnode_insertBefore(
    IXMLDOMNode *iface,
    IXMLDOMNode* newChild,
    VARIANT refChild,
    IXMLDOMNode** outNewChild)
{
    xmlnode *This = impl_from_IXMLDOMNode( iface );
    xmlNodePtr before_node, new_child_node;
    IXMLDOMNode *before = NULL, *new;
    HRESULT hr;

    TRACE("(%p)->(%p,var,%p)\n",This,newChild,outNewChild);

    if (!newChild)
        return E_INVALIDARG;

    switch(V_VT(&refChild))
    {
    case VT_EMPTY:
    case VT_NULL:
        break;

    case VT_UNKNOWN:
        hr = IUnknown_QueryInterface(V_UNKNOWN(&refChild), &IID_IXMLDOMNode, (LPVOID)&before);
        if(FAILED(hr)) return hr;
        break;

    case VT_DISPATCH:
        hr = IDispatch_QueryInterface(V_DISPATCH(&refChild), &IID_IXMLDOMNode, (LPVOID)&before);
        if(FAILED(hr)) return hr;
        break;

    default:
        FIXME("refChild var type %x\n", V_VT(&refChild));
        return E_FAIL;
    }

    IXMLDOMNode_QueryInterface(newChild, &IID_IXMLDOMNode, (LPVOID)&new);
    new_child_node = impl_from_IXMLDOMNode(new)->node;
    TRACE("new_child_node %p This->node %p\n", new_child_node, This->node);

    if(before)
    {
        before_node = impl_from_IXMLDOMNode(before)->node;
        xmlAddPrevSibling(before_node, new_child_node);
        IXMLDOMNode_Release(before);
    }
    else
    {
        xmlAddChild(This->node, new_child_node);
    }

    IXMLDOMNode_Release(new);
    IXMLDOMNode_AddRef(newChild);
    if(outNewChild)
        *outNewChild = newChild;

    TRACE("ret S_OK\n");
    return S_OK;
}

static HRESULT WINAPI xmlnode_replaceChild(
    IXMLDOMNode *iface,
    IXMLDOMNode* newChild,
    IXMLDOMNode* oldChild,
    IXMLDOMNode** outOldChild)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_removeChild(
    IXMLDOMNode *iface,
    IXMLDOMNode* childNode,
    IXMLDOMNode** oldChild)
{
    xmlnode *This = impl_from_IXMLDOMNode( iface );
    xmlNode *ancestor, *child_node_ptr;
    HRESULT hr;
    IXMLDOMNode *child;

    TRACE("%p->(%p, %p)\n", This, childNode, oldChild);

    *oldChild = NULL;

    if(!childNode) return E_INVALIDARG;

    hr = IXMLDOMNode_QueryInterface(childNode, &IID_IXMLDOMNode, (LPVOID)&child);
    if(FAILED(hr))
        return hr;

    child_node_ptr = ancestor = impl_from_IXMLDOMNode(child)->node;
    while(ancestor->parent)
    {
        if(ancestor->parent == This->node)
            break;
        ancestor = ancestor->parent;
    }
    if(!ancestor->parent)
    {
        WARN("childNode %p is not a child of %p\n", childNode, iface);
        IXMLDOMNode_Release(child);
        return E_INVALIDARG;
    }

    xmlUnlinkNode(child_node_ptr);

    IXMLDOMNode_Release(child);
    IXMLDOMNode_AddRef(childNode);
    *oldChild = childNode;
    return S_OK;
}

static HRESULT WINAPI xmlnode_appendChild(
    IXMLDOMNode *iface,
    IXMLDOMNode* newChild,
    IXMLDOMNode** outNewChild)
{
    xmlnode *This = impl_from_IXMLDOMNode( iface );
    VARIANT var;

    TRACE("(%p)->(%p,%p)\n", This, newChild, outNewChild);
    VariantInit(&var);
    return IXMLDOMNode_insertBefore(iface, newChild, var, outNewChild);
}

static HRESULT WINAPI xmlnode_hasChildNodes(
    IXMLDOMNode *iface,
    VARIANT_BOOL* hasChild)
{
    xmlnode *This = impl_from_IXMLDOMNode( iface );

    TRACE("%p\n", This);

    if (!hasChild)
        return E_INVALIDARG;
    if (!This->node->children)
    {
        *hasChild = VARIANT_FALSE;
        return S_FALSE;
    }

    *hasChild = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI xmlnode_get_ownerDocument(
    IXMLDOMNode *iface,
    IXMLDOMDocument** DOMDocument)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_cloneNode(
    IXMLDOMNode *iface,
    VARIANT_BOOL deep,
    IXMLDOMNode** cloneRoot)
{
    xmlnode *This = impl_from_IXMLDOMNode( iface );
    xmlNodePtr pClone = NULL;
    IXMLDOMNode *pNode = NULL;

    TRACE("%p (%d)\n", This, deep);

    if(!cloneRoot)
        return E_INVALIDARG;

    pClone = xmlCopyNode(This->node, deep ? 1 : 2);
    if(pClone)
    {
        pClone->doc = This->node->doc;

        pNode = create_node(pClone);
        if(!pNode)
        {
            ERR("Copy failed\n");
            return E_FAIL;
        }

        *cloneRoot = pNode;
    }
    else
    {
        ERR("Copy failed\n");
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI xmlnode_get_nodeTypeString(
    IXMLDOMNode *iface,
    BSTR* xmlnodeType)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_get_text(
    IXMLDOMNode *iface,
    BSTR* text)
{
    xmlnode *This = impl_from_IXMLDOMNode( iface );
    BSTR str = NULL;

    TRACE("%p\n", This);

    if ( !text )
        return E_INVALIDARG;

    switch(This->node->type)
    {
    case XML_ELEMENT_NODE:
    case XML_ATTRIBUTE_NODE:
    {
        xmlNodePtr child = This->node->children;
        if ( child && child->type == XML_TEXT_NODE )
            str = bstr_from_xmlChar( child->content );
        break;
    }

    case XML_TEXT_NODE:
    case XML_CDATA_SECTION_NODE:
    case XML_PI_NODE:
    case XML_COMMENT_NODE:
        str = bstr_from_xmlChar( This->node->content );
        break;

    default:
        FIXME("Unhandled node type %d\n", This->node->type);
    }

    /* Always return a string. */
    if (!str) str = SysAllocStringLen( NULL, 0 );

    TRACE("%p %s\n", This, debugstr_w(str) );
    *text = str;
 
    return S_OK;
}

static HRESULT WINAPI xmlnode_put_text(
    IXMLDOMNode *iface,
    BSTR text)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_get_specified(
    IXMLDOMNode *iface,
    VARIANT_BOOL* isSpecified)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_get_definition(
    IXMLDOMNode *iface,
    IXMLDOMNode** definitionNode)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_get_nodeTypedValue(
    IXMLDOMNode *iface,
    VARIANT* typedValue)
{
    FIXME("ignoring data type\n");
    return xmlnode_get_nodeValue(iface, typedValue);
}

static HRESULT WINAPI xmlnode_put_nodeTypedValue(
    IXMLDOMNode *iface,
    VARIANT typedValue)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_get_dataType(
    IXMLDOMNode *iface,
    VARIANT* dataTypeName)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_put_dataType(
    IXMLDOMNode *iface,
    BSTR dataTypeName)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_get_xml(
    IXMLDOMNode *iface,
    BSTR* xmlString)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_transformNode(
    IXMLDOMNode *iface,
    IXMLDOMNode* styleSheet,
    BSTR* xmlString)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_selectNodes(
    IXMLDOMNode *iface,
    BSTR queryString,
    IXMLDOMNodeList** resultList)
{
    xmlnode *This = impl_from_IXMLDOMNode( iface );

    TRACE("%p %s %p\n", This, debugstr_w(queryString), resultList );

    return queryresult_create( This->node, queryString, resultList );
}

static HRESULT WINAPI xmlnode_selectSingleNode(
    IXMLDOMNode *iface,
    BSTR queryString,
    IXMLDOMNode** resultNode)
{
    xmlnode *This = impl_from_IXMLDOMNode( iface );
    IXMLDOMNodeList *list;
    HRESULT r;

    TRACE("%p %s %p\n", This, debugstr_w(queryString), resultNode );

    *resultNode = NULL;
    r = IXMLDOMNode_selectNodes(iface, queryString, &list);
    if(r == S_OK)
    {
        r = IXMLDOMNodeList_nextNode(list, resultNode);
        IXMLDOMNodeList_Release(list);
    }
    return r;
}

static HRESULT WINAPI xmlnode_get_parsed(
    IXMLDOMNode *iface,
    VARIANT_BOOL* isParsed)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_get_namespaceURI(
    IXMLDOMNode *iface,
    BSTR* namespaceURI)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_get_prefix(
    IXMLDOMNode *iface,
    BSTR* prefixString)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlnode_get_baseName(
    IXMLDOMNode *iface,
    BSTR* nameString)
{
    xmlnode *This = impl_from_IXMLDOMNode( iface );
    BSTR str = NULL;
    HRESULT r = S_FALSE;

    TRACE("%p %p\n", This, nameString );

    if ( !nameString )
        return E_INVALIDARG;

    switch ( This->node->type )
    {
    case XML_ELEMENT_NODE:
    case XML_ATTRIBUTE_NODE:
        str = bstr_from_xmlChar( This->node->name );
        r = S_OK;
        break;
    case XML_TEXT_NODE:
        break;
    default:
        ERR("Unhandled type %d\n", This->node->type );
        break;
    }

    TRACE("returning %08x str = %s\n", r, debugstr_w( str ) );

    *nameString = str;
    return r;
}

static HRESULT WINAPI xmlnode_transformNodeToObject(
    IXMLDOMNode *iface,
    IXMLDOMNode* stylesheet,
    VARIANT outputObject)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static const struct IXMLDOMNodeVtbl xmlnode_vtbl =
{
    xmlnode_QueryInterface,
    xmlnode_AddRef,
    xmlnode_Release,
    xmlnode_GetTypeInfoCount,
    xmlnode_GetTypeInfo,
    xmlnode_GetIDsOfNames,
    xmlnode_Invoke,
    xmlnode_get_nodeName,
    xmlnode_get_nodeValue,
    xmlnode_put_nodeValue,
    xmlnode_get_nodeType,
    xmlnode_get_parentNode,
    xmlnode_get_childNodes,
    xmlnode_get_firstChild,
    xmlnode_get_lastChild,
    xmlnode_get_previousSibling,
    xmlnode_get_nextSibling,
    xmlnode_get_attributes,
    xmlnode_insertBefore,
    xmlnode_replaceChild,
    xmlnode_removeChild,
    xmlnode_appendChild,
    xmlnode_hasChildNodes,
    xmlnode_get_ownerDocument,
    xmlnode_cloneNode,
    xmlnode_get_nodeTypeString,
    xmlnode_get_text,
    xmlnode_put_text,
    xmlnode_get_specified,
    xmlnode_get_definition,
    xmlnode_get_nodeTypedValue,
    xmlnode_put_nodeTypedValue,
    xmlnode_get_dataType,
    xmlnode_put_dataType,
    xmlnode_get_xml,
    xmlnode_transformNode,
    xmlnode_selectNodes,
    xmlnode_selectSingleNode,
    xmlnode_get_parsed,
    xmlnode_get_namespaceURI,
    xmlnode_get_prefix,
    xmlnode_get_baseName,
    xmlnode_transformNodeToObject,
};

static HRESULT WINAPI Internal_QueryInterface(
    IUnknown *iface,
    REFIID riid,
    void** ppvObject )
{
    xmlnode *This = impl_from_InternalUnknown( iface );

    TRACE("%p %s %p\n", iface, debugstr_guid(riid), ppvObject);


    if ( IsEqualGUID( riid, &IID_IUnknown ))
        *ppvObject = iface;
    else if ( IsEqualGUID( riid, &IID_IDispatch ) ||
              IsEqualGUID( riid, &IID_IXMLDOMNode ) )
        *ppvObject = &This->lpVtbl;
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef( (IUnknown*)*ppvObject );

    return S_OK;
}

static ULONG WINAPI Internal_AddRef(
                 IUnknown *iface )
{
    xmlnode *This = impl_from_InternalUnknown( iface );
    return InterlockedIncrement( &This->ref );
}

static ULONG WINAPI Internal_Release(
    IUnknown *iface )
{
    xmlnode *This = impl_from_InternalUnknown( iface );
    ULONG ref;

    ref = InterlockedDecrement( &This->ref );
    if ( ref == 0 )
    {
        if( This->node )
	    xmldoc_release( This->node->doc );
        HeapFree( GetProcessHeap(), 0, This );
    }

    return ref;
}

static const struct IUnknownVtbl internal_unk_vtbl =
{
    Internal_QueryInterface,
    Internal_AddRef,
    Internal_Release
};

IUnknown *create_basic_node( xmlNodePtr node, IUnknown *pUnkOuter )
{
    xmlnode *This;

    This = HeapAlloc( GetProcessHeap(), 0, sizeof *This );
    if ( !This )
        return NULL;

    if(node)
        xmldoc_add_ref( node->doc );

    This->lpVtbl = &xmlnode_vtbl;
    This->lpInternalUnkVtbl = &internal_unk_vtbl;

    if(pUnkOuter)
        This->pUnkOuter = pUnkOuter; /* Don't take a ref on outer Unknown */
    else
        This->pUnkOuter = (IUnknown *)&This->lpInternalUnkVtbl;

    This->ref = 1;
    This->node = node;

    return (IUnknown*)&This->lpInternalUnkVtbl;
}

IXMLDOMNode *create_node( xmlNodePtr node )
{
    IUnknown *pUnk;
    IXMLDOMNode *ret;
    HRESULT hr;

    if ( !node )
        return NULL;

    TRACE("type %d\n", node->type);
    switch(node->type)
    {
    case XML_ELEMENT_NODE:
        pUnk = create_element( node, NULL );
        break;
    case XML_ATTRIBUTE_NODE:
        pUnk = create_attribute( node );
        break;
    case XML_TEXT_NODE:
        pUnk = create_text( node );
        break;
    case XML_COMMENT_NODE:
        pUnk = create_comment( node );
        break;
    case XML_DOCUMENT_NODE:
        ERR("shouldn't be here!\n");
        return NULL;
    default:
        FIXME("only creating basic node for type %d\n", node->type);
        pUnk = create_basic_node( node, NULL );
    }

    hr = IUnknown_QueryInterface(pUnk, &IID_IXMLDOMNode, (LPVOID*)&ret);
    IUnknown_Release(pUnk);
    if(FAILED(hr)) return NULL;
    return ret;
}
#endif
