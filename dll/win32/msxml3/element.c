/*
 *    DOM Document implementation
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

#define COBJMACROS

#include "config.h"

#include <stdarg.h>
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

typedef struct _domelem
{
    xmlnode node;
    const struct IXMLDOMElementVtbl *lpVtbl;
    LONG ref;
} domelem;

static inline domelem *impl_from_IXMLDOMElement( IXMLDOMElement *iface )
{
    return (domelem *)((char*)iface - FIELD_OFFSET(domelem, lpVtbl));
}

static inline xmlNodePtr get_element( domelem *This )
{
    return This->node.node;
}

static HRESULT WINAPI domelem_QueryInterface(
    IXMLDOMElement *iface,
    REFIID riid,
    void** ppvObject )
{
    domelem *This = impl_from_IXMLDOMElement( iface );

    TRACE("%p %s %p\n", This, debugstr_guid(riid), ppvObject);

    if ( IsEqualGUID( riid, &IID_IXMLDOMElement ) ||
         IsEqualGUID( riid, &IID_IDispatch ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = &This->lpVtbl;
    }
    else if ( IsEqualGUID( riid, &IID_IXMLDOMNode ) )
    {
        *ppvObject = IXMLDOMNode_from_impl(&This->node);
    }
    else if(dispex_query_interface(&This->node.dispex, riid, ppvObject))
    {
        return *ppvObject ? S_OK : E_NOINTERFACE;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef( (IUnknown*)*ppvObject );
    return S_OK;
}

static ULONG WINAPI domelem_AddRef(
    IXMLDOMElement *iface )
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI domelem_Release(
    IXMLDOMElement *iface )
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        destroy_xmlnode(&This->node);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI domelem_GetTypeInfoCount(
    IXMLDOMElement *iface,
    UINT* pctinfo )
{
    domelem *This = impl_from_IXMLDOMElement( iface );

    TRACE("(%p)->(%p)\n", This, pctinfo);

    *pctinfo = 1;

    return S_OK;
}

static HRESULT WINAPI domelem_GetTypeInfo(
    IXMLDOMElement *iface,
    UINT iTInfo, LCID lcid,
    ITypeInfo** ppTInfo )
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    HRESULT hr;

    TRACE("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);

    hr = get_typeinfo(IXMLDOMElement_tid, ppTInfo);

    return hr;
}

static HRESULT WINAPI domelem_GetIDsOfNames(
    IXMLDOMElement *iface,
    REFIID riid, LPOLESTR* rgszNames,
    UINT cNames, LCID lcid, DISPID* rgDispId )
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);

    if(!rgszNames || cNames == 0 || !rgDispId)
        return E_INVALIDARG;

    hr = get_typeinfo(IXMLDOMElement_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI domelem_Invoke(
    IXMLDOMElement *iface,
    DISPID dispIdMember, REFIID riid, LCID lcid,
    WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult,
    EXCEPINFO* pExcepInfo, UINT* puArgErr )
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IXMLDOMElement_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, &(This->lpVtbl), dispIdMember, wFlags, pDispParams,
                pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI domelem_get_nodeName(
    IXMLDOMElement *iface,
    BSTR* p )
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    return IXMLDOMNode_get_nodeName( IXMLDOMNode_from_impl(&This->node), p );
}

static HRESULT WINAPI domelem_get_nodeValue(
    IXMLDOMElement *iface,
    VARIANT* var1 )
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    return IXMLDOMNode_get_nodeValue( IXMLDOMNode_from_impl(&This->node), var1 );
}

static HRESULT WINAPI domelem_put_nodeValue(
    IXMLDOMElement *iface,
    VARIANT var1 )
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    return IXMLDOMNode_put_nodeValue( IXMLDOMNode_from_impl(&This->node), var1 );
}

static HRESULT WINAPI domelem_get_nodeType(
    IXMLDOMElement *iface,
    DOMNodeType* domNodeType )
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    return IXMLDOMNode_get_nodeType( IXMLDOMNode_from_impl(&This->node), domNodeType );
}

static HRESULT WINAPI domelem_get_parentNode(
    IXMLDOMElement *iface,
    IXMLDOMNode** parent )
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    return IXMLDOMNode_get_parentNode( IXMLDOMNode_from_impl(&This->node), parent );
}

static HRESULT WINAPI domelem_get_childNodes(
    IXMLDOMElement *iface,
    IXMLDOMNodeList** outList)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    return IXMLDOMNode_get_childNodes( IXMLDOMNode_from_impl(&This->node), outList );
}

static HRESULT WINAPI domelem_get_firstChild(
    IXMLDOMElement *iface,
    IXMLDOMNode** domNode)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    return IXMLDOMNode_get_firstChild( IXMLDOMNode_from_impl(&This->node), domNode );
}

static HRESULT WINAPI domelem_get_lastChild(
    IXMLDOMElement *iface,
    IXMLDOMNode** domNode)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    return IXMLDOMNode_get_lastChild( IXMLDOMNode_from_impl(&This->node), domNode );
}

static HRESULT WINAPI domelem_get_previousSibling(
    IXMLDOMElement *iface,
    IXMLDOMNode** domNode)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    return IXMLDOMNode_get_previousSibling( IXMLDOMNode_from_impl(&This->node), domNode );
}

static HRESULT WINAPI domelem_get_nextSibling(
    IXMLDOMElement *iface,
    IXMLDOMNode** domNode)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    return IXMLDOMNode_get_nextSibling( IXMLDOMNode_from_impl(&This->node), domNode );
}

static HRESULT WINAPI domelem_get_attributes(
    IXMLDOMElement *iface,
    IXMLDOMNamedNodeMap** attributeMap)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    return IXMLDOMNode_get_attributes( IXMLDOMNode_from_impl(&This->node), attributeMap );
}

static HRESULT WINAPI domelem_insertBefore(
    IXMLDOMElement *iface,
    IXMLDOMNode* newNode, VARIANT var1,
    IXMLDOMNode** outOldNode)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    return IXMLDOMNode_insertBefore( IXMLDOMNode_from_impl(&This->node), newNode, var1, outOldNode );
}

static HRESULT WINAPI domelem_replaceChild(
    IXMLDOMElement *iface,
    IXMLDOMNode* newNode,
    IXMLDOMNode* oldNode,
    IXMLDOMNode** outOldNode)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    return IXMLDOMNode_replaceChild( IXMLDOMNode_from_impl(&This->node), newNode, oldNode, outOldNode );
}

static HRESULT WINAPI domelem_removeChild(
    IXMLDOMElement *iface,
    IXMLDOMNode* domNode, IXMLDOMNode** oldNode)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    return IXMLDOMNode_removeChild( IXMLDOMNode_from_impl(&This->node), domNode, oldNode );
}

static HRESULT WINAPI domelem_appendChild(
    IXMLDOMElement *iface,
    IXMLDOMNode* newNode, IXMLDOMNode** outNewNode)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    return IXMLDOMNode_appendChild( IXMLDOMNode_from_impl(&This->node), newNode, outNewNode );
}

static HRESULT WINAPI domelem_hasChildNodes(
    IXMLDOMElement *iface,
    VARIANT_BOOL* pbool)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    return IXMLDOMNode_hasChildNodes( IXMLDOMNode_from_impl(&This->node), pbool );
}

static HRESULT WINAPI domelem_get_ownerDocument(
    IXMLDOMElement *iface,
    IXMLDOMDocument** domDocument)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    return IXMLDOMNode_get_ownerDocument( IXMLDOMNode_from_impl(&This->node), domDocument );
}

static HRESULT WINAPI domelem_cloneNode(
    IXMLDOMElement *iface,
    VARIANT_BOOL pbool, IXMLDOMNode** outNode)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    return IXMLDOMNode_cloneNode( IXMLDOMNode_from_impl(&This->node), pbool, outNode );
}

static HRESULT WINAPI domelem_get_nodeTypeString(
    IXMLDOMElement *iface,
    BSTR* p)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    return IXMLDOMNode_get_nodeTypeString( IXMLDOMNode_from_impl(&This->node), p );
}

static HRESULT WINAPI domelem_get_text(
    IXMLDOMElement *iface,
    BSTR* p)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    return IXMLDOMNode_get_text( IXMLDOMNode_from_impl(&This->node), p );
}

static HRESULT WINAPI domelem_put_text(
    IXMLDOMElement *iface,
    BSTR p)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    return IXMLDOMNode_put_text( IXMLDOMNode_from_impl(&This->node), p );
}

static HRESULT WINAPI domelem_get_specified(
    IXMLDOMElement *iface,
    VARIANT_BOOL* pbool)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    return IXMLDOMNode_get_specified( IXMLDOMNode_from_impl(&This->node), pbool );
}

static HRESULT WINAPI domelem_get_definition(
    IXMLDOMElement *iface,
    IXMLDOMNode** domNode)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    return IXMLDOMNode_get_definition( IXMLDOMNode_from_impl(&This->node), domNode );
}

static HRESULT WINAPI domelem_get_nodeTypedValue(
    IXMLDOMElement *iface,
    VARIANT* var1)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    return IXMLDOMNode_get_nodeTypedValue( IXMLDOMNode_from_impl(&This->node), var1 );
}

static HRESULT WINAPI domelem_put_nodeTypedValue(
    IXMLDOMElement *iface,
    VARIANT var1)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    return IXMLDOMNode_put_nodeTypedValue( IXMLDOMNode_from_impl(&This->node), var1 );
}

static HRESULT WINAPI domelem_get_dataType(
    IXMLDOMElement *iface,
    VARIANT* var1)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    return IXMLDOMNode_get_dataType( IXMLDOMNode_from_impl(&This->node), var1 );
}

static HRESULT WINAPI domelem_put_dataType(
    IXMLDOMElement *iface,
    BSTR p)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    return IXMLDOMNode_put_dataType( IXMLDOMNode_from_impl(&This->node), p );
}

static HRESULT WINAPI domelem_get_xml(
    IXMLDOMElement *iface,
    BSTR* p)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    return IXMLDOMNode_get_xml( IXMLDOMNode_from_impl(&This->node), p );
}

static HRESULT WINAPI domelem_transformNode(
    IXMLDOMElement *iface,
    IXMLDOMNode* domNode, BSTR* p)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    return IXMLDOMNode_transformNode( IXMLDOMNode_from_impl(&This->node), domNode, p );
}

static HRESULT WINAPI domelem_selectNodes(
    IXMLDOMElement *iface,
    BSTR p, IXMLDOMNodeList** outList)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    return IXMLDOMNode_selectNodes( IXMLDOMNode_from_impl(&This->node), p, outList );
}

static HRESULT WINAPI domelem_selectSingleNode(
    IXMLDOMElement *iface,
    BSTR p, IXMLDOMNode** outNode)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    return IXMLDOMNode_selectSingleNode( IXMLDOMNode_from_impl(&This->node), p, outNode );
}

static HRESULT WINAPI domelem_get_parsed(
    IXMLDOMElement *iface,
    VARIANT_BOOL* pbool)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    return IXMLDOMNode_get_parsed( IXMLDOMNode_from_impl(&This->node), pbool );
}

static HRESULT WINAPI domelem_get_namespaceURI(
    IXMLDOMElement *iface,
    BSTR* p)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    return IXMLDOMNode_get_namespaceURI( IXMLDOMNode_from_impl(&This->node), p );
}

static HRESULT WINAPI domelem_get_prefix(
    IXMLDOMElement *iface,
    BSTR* p)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    return IXMLDOMNode_get_prefix( IXMLDOMNode_from_impl(&This->node), p );
}

static HRESULT WINAPI domelem_get_baseName(
    IXMLDOMElement *iface,
    BSTR* p)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    return IXMLDOMNode_get_baseName( IXMLDOMNode_from_impl(&This->node), p );
}

static HRESULT WINAPI domelem_transformNodeToObject(
    IXMLDOMElement *iface,
    IXMLDOMNode* domNode, VARIANT var1)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    return IXMLDOMNode_transformNodeToObject( IXMLDOMNode_from_impl(&This->node), domNode, var1 );
}

static HRESULT WINAPI domelem_get_tagName(
    IXMLDOMElement *iface,
    BSTR* p)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    xmlNodePtr element;
    DWORD len;
    DWORD offset = 0;
    LPWSTR str;

    TRACE("%p\n", This );

    element = get_element( This );
    if ( !element )
        return E_FAIL;

    len = MultiByteToWideChar( CP_UTF8, 0, (LPCSTR) element->name, -1, NULL, 0 );
    if (element->ns)
        len += MultiByteToWideChar( CP_UTF8, 0, (LPCSTR) element->ns->prefix, -1, NULL, 0 );
    str = heap_alloc( len * sizeof (WCHAR) );
    if ( !str )
        return E_OUTOFMEMORY;
    if (element->ns)
    {
        offset = MultiByteToWideChar( CP_UTF8, 0, (LPCSTR) element->ns->prefix, -1, str, len );
        str[offset - 1] = ':';
    }
    MultiByteToWideChar( CP_UTF8, 0, (LPCSTR) element->name, -1, str + offset, len - offset );
    *p = SysAllocString( str );
    heap_free( str );

    return S_OK;
}

static HRESULT WINAPI domelem_getAttribute(
    IXMLDOMElement *iface,
    BSTR name, VARIANT* value)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    xmlNodePtr element;
    xmlChar *xml_name, *xml_value = NULL;
    HRESULT hr = S_FALSE;

    TRACE("(%p)->(%s,%p)\n", This, debugstr_w(name), value);

    if(!value || !name)
        return E_INVALIDARG;

    element = get_element( This );
    if ( !element )
        return E_FAIL;

    V_BSTR(value) = NULL;
    V_VT(value) = VT_NULL;

    xml_name = xmlChar_from_wchar( name );

    if(!xmlValidateNameValue(xml_name))
        hr = E_FAIL;
    else
        xml_value = xmlGetNsProp(element, xml_name, NULL);

    heap_free(xml_name);
    if(xml_value)
    {
        V_VT(value) = VT_BSTR;
        V_BSTR(value) = bstr_from_xmlChar( xml_value );
        xmlFree(xml_value);
        hr = S_OK;
    }

    return hr;
}

static HRESULT WINAPI domelem_setAttribute(
    IXMLDOMElement *iface,
    BSTR name, VARIANT value)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    xmlNodePtr element;
    xmlChar *xml_name, *xml_value;
    HRESULT hr;
    VARIANT var;

    TRACE("(%p)->(%s, var)\n", This, debugstr_w(name));

    element = get_element( This );
    if ( !element )
        return E_FAIL;

    VariantInit(&var);
    hr = VariantChangeType(&var, &value, 0, VT_BSTR);
    if(hr != S_OK)
    {
        FIXME("VariantChangeType failed\n");
        return hr;
    }

    xml_name = xmlChar_from_wchar( name );
    xml_value = xmlChar_from_wchar( V_BSTR(&var) );

    if(!xmlSetNsProp(element, NULL,  xml_name, xml_value))
        hr = E_FAIL;

    heap_free(xml_value);
    heap_free(xml_name);
    VariantClear(&var);

    return hr;
}

static HRESULT WINAPI domelem_removeAttribute(
    IXMLDOMElement *iface,
    BSTR p)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    IXMLDOMNamedNodeMap *attr;
    HRESULT hr;

    TRACE("(%p)->(%s)", This, debugstr_w(p));

    hr = IXMLDOMElement_get_attributes(iface, &attr);
    if (hr != S_OK) return hr;

    hr = IXMLDOMNamedNodeMap_removeNamedItem(attr, p, NULL);
    IXMLDOMNamedNodeMap_Release(attr);

    return hr;
}

static HRESULT WINAPI domelem_getAttributeNode(
    IXMLDOMElement *iface,
    BSTR p, IXMLDOMAttribute** attributeNode )
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    xmlChar *xml_name;
    xmlNodePtr element;
    xmlAttrPtr attr;
    IUnknown *unk;
    HRESULT hr = S_FALSE;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(p), attributeNode);

    if(!attributeNode)
        return E_FAIL;

    *attributeNode = NULL;

    element = get_element( This );
    if ( !element )
        return E_FAIL;

    xml_name = xmlChar_from_wchar(p);

    if(!xmlValidateNameValue(xml_name))
    {
        heap_free(xml_name);
        return E_FAIL;
    }

    attr = xmlHasProp(element, xml_name);
    if(attr) {
        unk = create_attribute((xmlNodePtr)attr);
        hr = IUnknown_QueryInterface(unk, &IID_IXMLDOMAttribute, (void**)attributeNode);
        IUnknown_Release(unk);
    }

    heap_free(xml_name);

    return hr;
}

static HRESULT WINAPI domelem_setAttributeNode(
    IXMLDOMElement *iface,
    IXMLDOMAttribute* domAttribute,
    IXMLDOMAttribute** attributeNode)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_removeAttributeNode(
    IXMLDOMElement *iface,
    IXMLDOMAttribute* domAttribute,
    IXMLDOMAttribute** attributeNode)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_getElementsByTagName(
    IXMLDOMElement *iface,
    BSTR bstrName, IXMLDOMNodeList** resultList)
{
    static const WCHAR xpathformat[] =
            { '.','/','/','*','[','l','o','c','a','l','-','n','a','m','e','(',')','=','\'','%','s','\'',']',0 };
    domelem *This = impl_from_IXMLDOMElement( iface );
    LPWSTR szPattern;
    xmlNodePtr element;
    HRESULT hr;

    TRACE("(%p)->(%s,%p)\n", This, debugstr_w(bstrName), resultList);

    if (bstrName[0] == '*' && bstrName[1] == 0)
    {
        szPattern = heap_alloc(sizeof(WCHAR)*5);
        szPattern[0] = '.';
        szPattern[1] = szPattern[2] = '/';
        szPattern[3] = '*';
        szPattern[4] = 0;
    }
    else
    {
        szPattern = heap_alloc(sizeof(WCHAR)*(21+lstrlenW(bstrName)+1));
        wsprintfW(szPattern, xpathformat, bstrName);
    }
    TRACE("%s\n", debugstr_w(szPattern));

    element = get_element(This);
    if (!element)
        hr = E_FAIL;
    else
        hr = queryresult_create(element, szPattern, resultList);
    heap_free(szPattern);

    return hr;
}

static HRESULT WINAPI domelem_normalize(
    IXMLDOMElement *iface )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static const struct IXMLDOMElementVtbl domelem_vtbl =
{
    domelem_QueryInterface,
    domelem_AddRef,
    domelem_Release,
    domelem_GetTypeInfoCount,
    domelem_GetTypeInfo,
    domelem_GetIDsOfNames,
    domelem_Invoke,
    domelem_get_nodeName,
    domelem_get_nodeValue,
    domelem_put_nodeValue,
    domelem_get_nodeType,
    domelem_get_parentNode,
    domelem_get_childNodes,
    domelem_get_firstChild,
    domelem_get_lastChild,
    domelem_get_previousSibling,
    domelem_get_nextSibling,
    domelem_get_attributes,
    domelem_insertBefore,
    domelem_replaceChild,
    domelem_removeChild,
    domelem_appendChild,
    domelem_hasChildNodes,
    domelem_get_ownerDocument,
    domelem_cloneNode,
    domelem_get_nodeTypeString,
    domelem_get_text,
    domelem_put_text,
    domelem_get_specified,
    domelem_get_definition,
    domelem_get_nodeTypedValue,
    domelem_put_nodeTypedValue,
    domelem_get_dataType,
    domelem_put_dataType,
    domelem_get_xml,
    domelem_transformNode,
    domelem_selectNodes,
    domelem_selectSingleNode,
    domelem_get_parsed,
    domelem_get_namespaceURI,
    domelem_get_prefix,
    domelem_get_baseName,
    domelem_transformNodeToObject,
    domelem_get_tagName,
    domelem_getAttribute,
    domelem_setAttribute,
    domelem_removeAttribute,
    domelem_getAttributeNode,
    domelem_setAttributeNode,
    domelem_removeAttributeNode,
    domelem_getElementsByTagName,
    domelem_normalize,
};

static const tid_t domelem_iface_tids[] = {
    IXMLDOMElement_tid,
    0
};

static dispex_static_data_t domelem_dispex = {
    NULL,
    IXMLDOMElement_tid,
    NULL,
    domelem_iface_tids
};

IUnknown* create_element( xmlNodePtr element )
{
    domelem *This;

    This = heap_alloc( sizeof *This );
    if ( !This )
        return NULL;

    This->lpVtbl = &domelem_vtbl;
    This->ref = 1;

    init_xmlnode(&This->node, element, (IUnknown*)&This->lpVtbl, &domelem_dispex);

    return (IUnknown*) &This->lpVtbl;
}

#endif
