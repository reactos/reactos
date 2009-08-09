/*
 *    DOM CDATA node implementation
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

#include "config.h"

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "msxml2.h"

#include "msxml_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

#ifdef HAVE_LIBXML2

typedef struct _domcdata
{
    const struct IXMLDOMCDATASectionVtbl *lpVtbl;
    LONG ref;
    IUnknown *node_unk;
    IXMLDOMNode *node;
} domcdata;

static inline domcdata *impl_from_IXMLDOMCDATASection( IXMLDOMCDATASection *iface )
{
    return (domcdata *)((char*)iface - FIELD_OFFSET(domcdata, lpVtbl));
}

static HRESULT WINAPI domcdata_QueryInterface(
    IXMLDOMCDATASection *iface,
    REFIID riid,
    void** ppvObject )
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    TRACE("%p %s %p\n", This, debugstr_guid(riid), ppvObject);

    if ( IsEqualGUID( riid, &IID_IXMLDOMCDATASection ) ||
         IsEqualGUID( riid, &IID_IXMLDOMCharacterData) ||
         IsEqualGUID( riid, &IID_IDispatch ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = iface;
    }
    else if ( IsEqualGUID( riid, &IID_IXMLDOMNode ) )
    {
        return IUnknown_QueryInterface(This->node_unk, riid, ppvObject);
    }
    else if ( IsEqualGUID( riid, &IID_IXMLDOMText ) ||
              IsEqualGUID( riid, &IID_IXMLDOMElement ) )
    {
        TRACE("Unsupported interface\n");
        return E_NOINTERFACE;
    }
    else
    {
        FIXME("Unsupported interface %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IXMLDOMCDATASection_AddRef( iface );

    return S_OK;
}

static ULONG WINAPI domcdata_AddRef(
    IXMLDOMCDATASection *iface )
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    return InterlockedIncrement( &This->ref );
}

static ULONG WINAPI domcdata_Release(
    IXMLDOMCDATASection *iface )
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    ULONG ref;

    ref = InterlockedDecrement( &This->ref );
    if ( ref == 0 )
    {
        IUnknown_Release( This->node_unk );
        HeapFree( GetProcessHeap(), 0, This );
    }

    return ref;
}

static HRESULT WINAPI domcdata_GetTypeInfoCount(
    IXMLDOMCDATASection *iface,
    UINT* pctinfo )
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );

    TRACE("(%p)->(%p)\n", This, pctinfo);

    *pctinfo = 1;

    return S_OK;
}

static HRESULT WINAPI domcdata_GetTypeInfo(
    IXMLDOMCDATASection *iface,
    UINT iTInfo, LCID lcid,
    ITypeInfo** ppTInfo )
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    HRESULT hr;

    TRACE("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);

    hr = get_typeinfo(IXMLDOMCDATASection_tid, ppTInfo);

    return hr;
}

static HRESULT WINAPI domcdata_GetIDsOfNames(
    IXMLDOMCDATASection *iface,
    REFIID riid, LPOLESTR* rgszNames,
    UINT cNames, LCID lcid, DISPID* rgDispId )
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);

    if(!rgszNames || cNames == 0 || !rgDispId)
        return E_INVALIDARG;

    hr = get_typeinfo(IXMLDOMCDATASection_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI domcdata_Invoke(
    IXMLDOMCDATASection *iface,
    DISPID dispIdMember, REFIID riid, LCID lcid,
    WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult,
    EXCEPINFO* pExcepInfo, UINT* puArgErr )
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IXMLDOMCDATASection_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, &(This->lpVtbl), dispIdMember, wFlags, pDispParams,
                pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI domcdata_get_nodeName(
    IXMLDOMCDATASection *iface,
    BSTR* p )
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    return IXMLDOMNode_get_nodeName( This->node, p );
}

static HRESULT WINAPI domcdata_get_nodeValue(
    IXMLDOMCDATASection *iface,
    VARIANT* var1 )
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    return IXMLDOMNode_get_nodeValue( This->node, var1 );
}

static HRESULT WINAPI domcdata_put_nodeValue(
    IXMLDOMCDATASection *iface,
    VARIANT var1 )
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    return IXMLDOMNode_put_nodeValue( This->node, var1 );
}

static HRESULT WINAPI domcdata_get_nodeType(
    IXMLDOMCDATASection *iface,
    DOMNodeType* domNodeType )
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    return IXMLDOMNode_get_nodeType( This->node, domNodeType );
}

static HRESULT WINAPI domcdata_get_parentNode(
    IXMLDOMCDATASection *iface,
    IXMLDOMNode** parent )
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    return IXMLDOMNode_get_parentNode( This->node, parent );
}

static HRESULT WINAPI domcdata_get_childNodes(
    IXMLDOMCDATASection *iface,
    IXMLDOMNodeList** outList)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    return IXMLDOMNode_get_childNodes( This->node, outList );
}

static HRESULT WINAPI domcdata_get_firstChild(
    IXMLDOMCDATASection *iface,
    IXMLDOMNode** domNode)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    return IXMLDOMNode_get_firstChild( This->node, domNode );
}

static HRESULT WINAPI domcdata_get_lastChild(
    IXMLDOMCDATASection *iface,
    IXMLDOMNode** domNode)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    return IXMLDOMNode_get_lastChild( This->node, domNode );
}

static HRESULT WINAPI domcdata_get_previousSibling(
    IXMLDOMCDATASection *iface,
    IXMLDOMNode** domNode)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    return IXMLDOMNode_get_previousSibling( This->node, domNode );
}

static HRESULT WINAPI domcdata_get_nextSibling(
    IXMLDOMCDATASection *iface,
    IXMLDOMNode** domNode)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    return IXMLDOMNode_get_nextSibling( This->node, domNode );
}

static HRESULT WINAPI domcdata_get_attributes(
    IXMLDOMCDATASection *iface,
    IXMLDOMNamedNodeMap** attributeMap)
{
	domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    return IXMLDOMNode_get_attributes( This->node, attributeMap );
}

static HRESULT WINAPI domcdata_insertBefore(
    IXMLDOMCDATASection *iface,
    IXMLDOMNode* newNode, VARIANT var1,
    IXMLDOMNode** outOldNode)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    return IXMLDOMNode_insertBefore( This->node, newNode, var1, outOldNode );
}

static HRESULT WINAPI domcdata_replaceChild(
    IXMLDOMCDATASection *iface,
    IXMLDOMNode* newNode,
    IXMLDOMNode* oldNode,
    IXMLDOMNode** outOldNode)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    return IXMLDOMNode_replaceChild( This->node, newNode, oldNode, outOldNode );
}

static HRESULT WINAPI domcdata_removeChild(
    IXMLDOMCDATASection *iface,
    IXMLDOMNode* domNode, IXMLDOMNode** oldNode)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    return IXMLDOMNode_removeChild( This->node, domNode, oldNode );
}

static HRESULT WINAPI domcdata_appendChild(
    IXMLDOMCDATASection *iface,
    IXMLDOMNode* newNode, IXMLDOMNode** outNewNode)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    return IXMLDOMNode_appendChild( This->node, newNode, outNewNode );
}

static HRESULT WINAPI domcdata_hasChildNodes(
    IXMLDOMCDATASection *iface,
    VARIANT_BOOL* pbool)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    return IXMLDOMNode_hasChildNodes( This->node, pbool );
}

static HRESULT WINAPI domcdata_get_ownerDocument(
    IXMLDOMCDATASection *iface,
    IXMLDOMDocument** domDocument)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    return IXMLDOMNode_get_ownerDocument( This->node, domDocument );
}

static HRESULT WINAPI domcdata_cloneNode(
    IXMLDOMCDATASection *iface,
    VARIANT_BOOL pbool, IXMLDOMNode** outNode)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    return IXMLDOMNode_cloneNode( This->node, pbool, outNode );
}

static HRESULT WINAPI domcdata_get_nodeTypeString(
    IXMLDOMCDATASection *iface,
    BSTR* p)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    return IXMLDOMNode_get_nodeTypeString( This->node, p );
}

static HRESULT WINAPI domcdata_get_text(
    IXMLDOMCDATASection *iface,
    BSTR* p)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    return IXMLDOMNode_get_text( This->node, p );
}

static HRESULT WINAPI domcdata_put_text(
    IXMLDOMCDATASection *iface,
    BSTR p)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    return IXMLDOMNode_put_text( This->node, p );
}

static HRESULT WINAPI domcdata_get_specified(
    IXMLDOMCDATASection *iface,
    VARIANT_BOOL* pbool)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    return IXMLDOMNode_get_specified( This->node, pbool );
}

static HRESULT WINAPI domcdata_get_definition(
    IXMLDOMCDATASection *iface,
    IXMLDOMNode** domNode)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    return IXMLDOMNode_get_definition( This->node, domNode );
}

static HRESULT WINAPI domcdata_get_nodeTypedValue(
    IXMLDOMCDATASection *iface,
    VARIANT* var1)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    return IXMLDOMNode_get_nodeTypedValue( This->node, var1 );
}

static HRESULT WINAPI domcdata_put_nodeTypedValue(
    IXMLDOMCDATASection *iface,
    VARIANT var1)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    return IXMLDOMNode_put_nodeTypedValue( This->node, var1 );
}

static HRESULT WINAPI domcdata_get_dataType(
    IXMLDOMCDATASection *iface,
    VARIANT* var1)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    return IXMLDOMNode_get_dataType( This->node, var1 );
}

static HRESULT WINAPI domcdata_put_dataType(
    IXMLDOMCDATASection *iface,
    BSTR p)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    return IXMLDOMNode_put_dataType( This->node, p );
}

static HRESULT WINAPI domcdata_get_xml(
    IXMLDOMCDATASection *iface,
    BSTR* p)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    return IXMLDOMNode_get_xml( This->node, p );
}

static HRESULT WINAPI domcdata_transformNode(
    IXMLDOMCDATASection *iface,
    IXMLDOMNode* domNode, BSTR* p)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    return IXMLDOMNode_transformNode( This->node, domNode, p );
}

static HRESULT WINAPI domcdata_selectNodes(
    IXMLDOMCDATASection *iface,
    BSTR p, IXMLDOMNodeList** outList)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    return IXMLDOMNode_selectNodes( This->node, p, outList );
}

static HRESULT WINAPI domcdata_selectSingleNode(
    IXMLDOMCDATASection *iface,
    BSTR p, IXMLDOMNode** outNode)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    return IXMLDOMNode_selectSingleNode( This->node, p, outNode );
}

static HRESULT WINAPI domcdata_get_parsed(
    IXMLDOMCDATASection *iface,
    VARIANT_BOOL* pbool)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    return IXMLDOMNode_get_parsed( This->node, pbool );
}

static HRESULT WINAPI domcdata_get_namespaceURI(
    IXMLDOMCDATASection *iface,
    BSTR* p)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    return IXMLDOMNode_get_namespaceURI( This->node, p );
}

static HRESULT WINAPI domcdata_get_prefix(
    IXMLDOMCDATASection *iface,
    BSTR* p)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    return IXMLDOMNode_get_prefix( This->node, p );
}

static HRESULT WINAPI domcdata_get_baseName(
    IXMLDOMCDATASection *iface,
    BSTR* p)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    return IXMLDOMNode_get_baseName( This->node, p );
}

static HRESULT WINAPI domcdata_transformNodeToObject(
    IXMLDOMCDATASection *iface,
    IXMLDOMNode* domNode, VARIANT var1)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    return IXMLDOMNode_transformNodeToObject( This->node, domNode, var1 );
}

static HRESULT WINAPI domcdata_get_data(
    IXMLDOMCDATASection *iface,
    BSTR *p)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    HRESULT hr = E_FAIL;
    VARIANT vRet;

    if(!p)
        return E_INVALIDARG;

    hr = IXMLDOMNode_get_nodeValue( This->node, &vRet );
    if(hr == S_OK)
    {
        *p = V_BSTR(&vRet);
    }

    return hr;
}

static HRESULT WINAPI domcdata_put_data(
    IXMLDOMCDATASection *iface,
    BSTR data)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    HRESULT hr = E_FAIL;
    VARIANT val;

    TRACE("%p %s\n", This, debugstr_w(data) );

    V_VT(&val) = VT_BSTR;
    V_BSTR(&val) = data;

    hr = IXMLDOMNode_put_nodeValue( This->node, val );

    return hr;
}

static HRESULT WINAPI domcdata_get_length(
    IXMLDOMCDATASection *iface,
    long *len)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    xmlnode *pDOMNode = impl_from_IXMLDOMNode( This->node );
    xmlChar *pContent;
    long nLength = 0;

    TRACE("%p\n", iface);

    if(!len)
        return E_INVALIDARG;

    pContent = xmlNodeGetContent(pDOMNode->node);
    if(pContent)
    {
        nLength = xmlStrlen(pContent);
        xmlFree(pContent);
    }

    *len = nLength;

    return S_OK;
}

static HRESULT WINAPI domcdata_substringData(
    IXMLDOMCDATASection *iface,
    long offset, long count, BSTR *p)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    xmlnode *pDOMNode = impl_from_IXMLDOMNode( This->node );
    xmlChar *pContent;
    long nLength = 0;
    HRESULT hr = S_FALSE;

    TRACE("%p\n", iface);

    if(!p)
        return E_INVALIDARG;

    *p = NULL;
    if(offset < 0 || count < 0)
        return E_INVALIDARG;

    if(count == 0)
        return hr;

    pContent = xmlNodeGetContent(pDOMNode->node);
    if(pContent)
    {
        nLength = xmlStrlen(pContent);

        if( offset < nLength)
        {
            BSTR sContent = bstr_from_xmlChar(pContent);
            if(offset + count > nLength)
                *p = SysAllocString(&sContent[offset]);
            else
                *p = SysAllocStringLen(&sContent[offset], count);

            SysFreeString(sContent);
            hr = S_OK;
        }

        xmlFree(pContent);
    }

    return hr;
}

static HRESULT WINAPI domcdata_appendData(
    IXMLDOMCDATASection *iface,
    BSTR p)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    xmlnode *pDOMNode = impl_from_IXMLDOMNode( This->node );
    xmlChar *pContent;
    HRESULT hr = S_FALSE;

    TRACE("%p\n", iface);

    /* Nothing to do if NULL or an Empty string passed in. */
    if(p == NULL || SysStringLen(p) == 0)
        return S_OK;

    pContent = xmlChar_from_wchar( p );
    if(pContent)
    {
        if(xmlTextConcat(pDOMNode->node, pContent, SysStringLen(p) ) == 0)
            hr = S_OK;
        else
            hr = E_FAIL;
    }
    else
        hr = E_FAIL;
    HeapFree(GetProcessHeap(), 0, pContent);

    return hr;
}

static HRESULT WINAPI domcdata_insertData(
    IXMLDOMCDATASection *iface,
    long offset, BSTR p)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    xmlnode *pDOMNode = impl_from_IXMLDOMNode( This->node );
    xmlChar *pXmlContent;
    BSTR sNewString;
    HRESULT hr = S_FALSE;
    long nLength = 0, nLengthP = 0;
    xmlChar *str = NULL;

    TRACE("%p\n", This);

    /* If have a NULL or empty string, don't do anything. */
    if(SysStringLen(p) == 0)
        return S_OK;

    if(offset < 0)
    {
        return E_INVALIDARG;
    }

    pXmlContent = xmlNodeGetContent(pDOMNode->node);
    if(pXmlContent)
    {
        BSTR sContent = bstr_from_xmlChar( pXmlContent );
        nLength = SysStringLen(sContent);
        nLengthP = SysStringLen(p);

        if(nLength < offset)
        {
            SysFreeString(sContent);
            xmlFree(pXmlContent);

            return E_INVALIDARG;
        }

        sNewString = SysAllocStringLen(NULL, nLength + nLengthP + 1);
        if(sNewString)
        {
            if(offset > 0)
                memcpy(sNewString, sContent, offset * sizeof(WCHAR));

            memcpy(&sNewString[offset], p, nLengthP * sizeof(WCHAR));

            if(offset+nLengthP < nLength)
                memcpy(&sNewString[offset+nLengthP], &sContent[offset], (nLength-offset) * sizeof(WCHAR));

            sNewString[nLengthP + nLength] = 0;

            str = xmlChar_from_wchar(sNewString);
            if(str)
            {
                xmlNodeSetContent(pDOMNode->node, str);
                hr = S_OK;
            }
            HeapFree(GetProcessHeap(), 0, str);

            SysFreeString(sNewString);
        }

        SysFreeString(sContent);

        xmlFree(pXmlContent);
    }

    return hr;
}

static HRESULT WINAPI domcdata_deleteData(
    IXMLDOMCDATASection *iface,
    long offset, long count)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domcdata_replaceData(
    IXMLDOMCDATASection *iface,
    long offset, long count, BSTR p)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domcdata_splitText(
    IXMLDOMCDATASection *iface,
    long offset, IXMLDOMText **txtNode)
{
    FIXME("\n");
    return E_NOTIMPL;
}


static const struct IXMLDOMCDATASectionVtbl domcdata_vtbl =
{
    domcdata_QueryInterface,
    domcdata_AddRef,
    domcdata_Release,
    domcdata_GetTypeInfoCount,
    domcdata_GetTypeInfo,
    domcdata_GetIDsOfNames,
    domcdata_Invoke,
    domcdata_get_nodeName,
    domcdata_get_nodeValue,
    domcdata_put_nodeValue,
    domcdata_get_nodeType,
    domcdata_get_parentNode,
    domcdata_get_childNodes,
    domcdata_get_firstChild,
    domcdata_get_lastChild,
    domcdata_get_previousSibling,
    domcdata_get_nextSibling,
    domcdata_get_attributes,
    domcdata_insertBefore,
    domcdata_replaceChild,
    domcdata_removeChild,
    domcdata_appendChild,
    domcdata_hasChildNodes,
    domcdata_get_ownerDocument,
    domcdata_cloneNode,
    domcdata_get_nodeTypeString,
    domcdata_get_text,
    domcdata_put_text,
    domcdata_get_specified,
    domcdata_get_definition,
    domcdata_get_nodeTypedValue,
    domcdata_put_nodeTypedValue,
    domcdata_get_dataType,
    domcdata_put_dataType,
    domcdata_get_xml,
    domcdata_transformNode,
    domcdata_selectNodes,
    domcdata_selectSingleNode,
    domcdata_get_parsed,
    domcdata_get_namespaceURI,
    domcdata_get_prefix,
    domcdata_get_baseName,
    domcdata_transformNodeToObject,
    domcdata_get_data,
    domcdata_put_data,
    domcdata_get_length,
    domcdata_substringData,
    domcdata_appendData,
    domcdata_insertData,
    domcdata_deleteData,
    domcdata_replaceData,
    domcdata_splitText
};

IUnknown* create_cdata( xmlNodePtr text )
{
    domcdata *This;
    HRESULT hr;

    This = HeapAlloc( GetProcessHeap(), 0, sizeof *This );
    if ( !This )
        return NULL;

    This->lpVtbl = &domcdata_vtbl;
    This->ref = 1;

    This->node_unk = create_basic_node( text, (IUnknown*)&This->lpVtbl );
    if(!This->node_unk)
    {
        HeapFree(GetProcessHeap(), 0, This);
        return NULL;
    }

    hr = IUnknown_QueryInterface(This->node_unk, &IID_IXMLDOMNode, (LPVOID*)&This->node);
    if(FAILED(hr))
    {
        IUnknown_Release(This->node_unk);
        HeapFree( GetProcessHeap(), 0, This );
        return NULL;
    }
    /* The ref on This->node is actually looped back into this object, so release it */
    IXMLDOMNode_Release(This->node);

    return (IUnknown*) &This->lpVtbl;
}

#endif
