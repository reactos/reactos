/*
 *    DOM text node implementation
 *
 * Copyright 2006 Huw Davies
 * Copyright 2007-2008 Alistair Leslie-Hughes
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

typedef struct _domtext
{
    const struct IXMLDOMTextVtbl *lpVtbl;
    LONG ref;
    IUnknown *node_unk;
    IXMLDOMNode *node;
} domtext;

static inline domtext *impl_from_IXMLDOMText( IXMLDOMText *iface )
{
    return (domtext *)((char*)iface - FIELD_OFFSET(domtext, lpVtbl));
}

static HRESULT WINAPI domtext_QueryInterface(
    IXMLDOMText *iface,
    REFIID riid,
    void** ppvObject )
{
    domtext *This = impl_from_IXMLDOMText( iface );
    TRACE("%p %s %p\n", This, debugstr_guid(riid), ppvObject);

    if ( IsEqualGUID( riid, &IID_IXMLDOMText ) ||
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
    else if ( IsEqualGUID( riid, &IID_IXMLDOMElement ) ||
              IsEqualGUID( riid, &IID_IXMLDOMCDATASection ) )
    {
        /* IXMLDOMText is known to be correct in not implementing these */
        TRACE("Unsupported interface\n");
        return E_NOINTERFACE;
    }
    else
    {
        FIXME("Unsupported interface %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IXMLDOMText_AddRef( iface );

    return S_OK;
}

static ULONG WINAPI domtext_AddRef(
    IXMLDOMText *iface )
{
    domtext *This = impl_from_IXMLDOMText( iface );
    return InterlockedIncrement( &This->ref );
}

static ULONG WINAPI domtext_Release(
    IXMLDOMText *iface )
{
    domtext *This = impl_from_IXMLDOMText( iface );
    ULONG ref;

    ref = InterlockedDecrement( &This->ref );
    if ( ref == 0 )
    {
        IUnknown_Release( This->node_unk );
        HeapFree( GetProcessHeap(), 0, This );
    }

    return ref;
}

static HRESULT WINAPI domtext_GetTypeInfoCount(
    IXMLDOMText *iface,
    UINT* pctinfo )
{
    domtext *This = impl_from_IXMLDOMText( iface );

    TRACE("(%p)->(%p)\n", This, pctinfo);

    *pctinfo = 1;

    return S_OK;
}

static HRESULT WINAPI domtext_GetTypeInfo(
    IXMLDOMText *iface,
    UINT iTInfo, LCID lcid,
    ITypeInfo** ppTInfo )
{
    domtext *This = impl_from_IXMLDOMText( iface );
    HRESULT hr;

    TRACE("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);

    hr = get_typeinfo(IXMLDOMText_tid, ppTInfo);

    return hr;
}

static HRESULT WINAPI domtext_GetIDsOfNames(
    IXMLDOMText *iface,
    REFIID riid, LPOLESTR* rgszNames,
    UINT cNames, LCID lcid, DISPID* rgDispId )
{
    domtext *This = impl_from_IXMLDOMText( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);

    if(!rgszNames || cNames == 0 || !rgDispId)
        return E_INVALIDARG;

    hr = get_typeinfo(IXMLDOMText_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI domtext_Invoke(
    IXMLDOMText *iface,
    DISPID dispIdMember, REFIID riid, LCID lcid,
    WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult,
    EXCEPINFO* pExcepInfo, UINT* puArgErr )
{
    domtext *This = impl_from_IXMLDOMText( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IXMLDOMText_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, &(This->lpVtbl), dispIdMember, wFlags, pDispParams,
                pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI domtext_get_nodeName(
    IXMLDOMText *iface,
    BSTR* p )
{
    domtext *This = impl_from_IXMLDOMText( iface );
    return IXMLDOMNode_get_nodeName( This->node, p );
}

static HRESULT WINAPI domtext_get_nodeValue(
    IXMLDOMText *iface,
    VARIANT* var1 )
{
    domtext *This = impl_from_IXMLDOMText( iface );
    return IXMLDOMNode_get_nodeValue( This->node, var1 );
}

static HRESULT WINAPI domtext_put_nodeValue(
    IXMLDOMText *iface,
    VARIANT var1 )
{
    domtext *This = impl_from_IXMLDOMText( iface );
    return IXMLDOMNode_put_nodeValue( This->node, var1 );
}

static HRESULT WINAPI domtext_get_nodeType(
    IXMLDOMText *iface,
    DOMNodeType* domNodeType )
{
    domtext *This = impl_from_IXMLDOMText( iface );
    return IXMLDOMNode_get_nodeType( This->node, domNodeType );
}

static HRESULT WINAPI domtext_get_parentNode(
    IXMLDOMText *iface,
    IXMLDOMNode** parent )
{
    domtext *This = impl_from_IXMLDOMText( iface );
    return IXMLDOMNode_get_parentNode( This->node, parent );
}

static HRESULT WINAPI domtext_get_childNodes(
    IXMLDOMText *iface,
    IXMLDOMNodeList** outList)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    return IXMLDOMNode_get_childNodes( This->node, outList );
}

static HRESULT WINAPI domtext_get_firstChild(
    IXMLDOMText *iface,
    IXMLDOMNode** domNode)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    return IXMLDOMNode_get_firstChild( This->node, domNode );
}

static HRESULT WINAPI domtext_get_lastChild(
    IXMLDOMText *iface,
    IXMLDOMNode** domNode)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    return IXMLDOMNode_get_lastChild( This->node, domNode );
}

static HRESULT WINAPI domtext_get_previousSibling(
    IXMLDOMText *iface,
    IXMLDOMNode** domNode)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    return IXMLDOMNode_get_previousSibling( This->node, domNode );
}

static HRESULT WINAPI domtext_get_nextSibling(
    IXMLDOMText *iface,
    IXMLDOMNode** domNode)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    return IXMLDOMNode_get_nextSibling( This->node, domNode );
}

static HRESULT WINAPI domtext_get_attributes(
    IXMLDOMText *iface,
    IXMLDOMNamedNodeMap** attributeMap)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    return IXMLDOMNode_get_attributes( This->node, attributeMap );
}

static HRESULT WINAPI domtext_insertBefore(
    IXMLDOMText *iface,
    IXMLDOMNode* newNode, VARIANT var1,
    IXMLDOMNode** outOldNode)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    return IXMLDOMNode_insertBefore( This->node, newNode, var1, outOldNode );
}

static HRESULT WINAPI domtext_replaceChild(
    IXMLDOMText *iface,
    IXMLDOMNode* newNode,
    IXMLDOMNode* oldNode,
    IXMLDOMNode** outOldNode)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    return IXMLDOMNode_replaceChild( This->node, newNode, oldNode, outOldNode );
}

static HRESULT WINAPI domtext_removeChild(
    IXMLDOMText *iface,
    IXMLDOMNode* domNode, IXMLDOMNode** oldNode)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    return IXMLDOMNode_removeChild( This->node, domNode, oldNode );
}

static HRESULT WINAPI domtext_appendChild(
    IXMLDOMText *iface,
    IXMLDOMNode* newNode, IXMLDOMNode** outNewNode)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    return IXMLDOMNode_appendChild( This->node, newNode, outNewNode );
}

static HRESULT WINAPI domtext_hasChildNodes(
    IXMLDOMText *iface,
    VARIANT_BOOL* pbool)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    return IXMLDOMNode_hasChildNodes( This->node, pbool );
}

static HRESULT WINAPI domtext_get_ownerDocument(
    IXMLDOMText *iface,
    IXMLDOMDocument** domDocument)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    return IXMLDOMNode_get_ownerDocument( This->node, domDocument );
}

static HRESULT WINAPI domtext_cloneNode(
    IXMLDOMText *iface,
    VARIANT_BOOL pbool, IXMLDOMNode** outNode)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    return IXMLDOMNode_cloneNode( This->node, pbool, outNode );
}

static HRESULT WINAPI domtext_get_nodeTypeString(
    IXMLDOMText *iface,
    BSTR* p)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    return IXMLDOMNode_get_nodeTypeString( This->node, p );
}

static HRESULT WINAPI domtext_get_text(
    IXMLDOMText *iface,
    BSTR* p)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    return IXMLDOMNode_get_text( This->node, p );
}

static HRESULT WINAPI domtext_put_text(
    IXMLDOMText *iface,
    BSTR p)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    return IXMLDOMNode_put_text( This->node, p );
}

static HRESULT WINAPI domtext_get_specified(
    IXMLDOMText *iface,
    VARIANT_BOOL* pbool)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    return IXMLDOMNode_get_specified( This->node, pbool );
}

static HRESULT WINAPI domtext_get_definition(
    IXMLDOMText *iface,
    IXMLDOMNode** domNode)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    return IXMLDOMNode_get_definition( This->node, domNode );
}

static HRESULT WINAPI domtext_get_nodeTypedValue(
    IXMLDOMText *iface,
    VARIANT* var1)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    return IXMLDOMNode_get_nodeTypedValue( This->node, var1 );
}

static HRESULT WINAPI domtext_put_nodeTypedValue(
    IXMLDOMText *iface,
    VARIANT var1)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    return IXMLDOMNode_put_nodeTypedValue( This->node, var1 );
}

static HRESULT WINAPI domtext_get_dataType(
    IXMLDOMText *iface,
    VARIANT* var1)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    return IXMLDOMNode_get_dataType( This->node, var1 );
}

static HRESULT WINAPI domtext_put_dataType(
    IXMLDOMText *iface,
    BSTR p)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    return IXMLDOMNode_put_dataType( This->node, p );
}

static HRESULT WINAPI domtext_get_xml(
    IXMLDOMText *iface,
    BSTR* p)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    return IXMLDOMNode_get_xml( This->node, p );
}

static HRESULT WINAPI domtext_transformNode(
    IXMLDOMText *iface,
    IXMLDOMNode* domNode, BSTR* p)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    return IXMLDOMNode_transformNode( This->node, domNode, p );
}

static HRESULT WINAPI domtext_selectNodes(
    IXMLDOMText *iface,
    BSTR p, IXMLDOMNodeList** outList)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    return IXMLDOMNode_selectNodes( This->node, p, outList );
}

static HRESULT WINAPI domtext_selectSingleNode(
    IXMLDOMText *iface,
    BSTR p, IXMLDOMNode** outNode)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    return IXMLDOMNode_selectSingleNode( This->node, p, outNode );
}

static HRESULT WINAPI domtext_get_parsed(
    IXMLDOMText *iface,
    VARIANT_BOOL* pbool)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    return IXMLDOMNode_get_parsed( This->node, pbool );
}

static HRESULT WINAPI domtext_get_namespaceURI(
    IXMLDOMText *iface,
    BSTR* p)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    return IXMLDOMNode_get_namespaceURI( This->node, p );
}

static HRESULT WINAPI domtext_get_prefix(
    IXMLDOMText *iface,
    BSTR* p)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    return IXMLDOMNode_get_prefix( This->node, p );
}

static HRESULT WINAPI domtext_get_baseName(
    IXMLDOMText *iface,
    BSTR* p)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    return IXMLDOMNode_get_baseName( This->node, p );
}

static HRESULT WINAPI domtext_transformNodeToObject(
    IXMLDOMText *iface,
    IXMLDOMNode* domNode, VARIANT var1)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    return IXMLDOMNode_transformNodeToObject( This->node, domNode, var1 );
}

static HRESULT WINAPI domtext_get_data(
    IXMLDOMText *iface,
    BSTR *p)
{
    domtext *This = impl_from_IXMLDOMText( iface );
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

static HRESULT WINAPI domtext_put_data(
    IXMLDOMText *iface,
    BSTR data)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    HRESULT hr = E_FAIL;
    VARIANT val;

    TRACE("%p %s\n", This, debugstr_w(data) );

    V_VT(&val) = VT_BSTR;
    V_BSTR(&val) = data;

    hr = IXMLDOMNode_put_nodeValue( This->node, val );

    return hr;
}

static HRESULT WINAPI domtext_get_length(
    IXMLDOMText *iface,
    long *len)
{
    domtext *This = impl_from_IXMLDOMText( iface );
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

static HRESULT WINAPI domtext_substringData(
    IXMLDOMText *iface,
    long offset, long count, BSTR *p)
{
    domtext *This = impl_from_IXMLDOMText( iface );
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

static HRESULT WINAPI domtext_appendData(
    IXMLDOMText *iface,
    BSTR p)
{
    domtext *This = impl_from_IXMLDOMText( iface );
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
        HeapFree( GetProcessHeap(), 0, pContent );
    }
    else
        hr = E_FAIL;

    return hr;
}

static HRESULT WINAPI domtext_insertData(
    IXMLDOMText *iface,
    long offset, BSTR p)
{
    domtext *This = impl_from_IXMLDOMText( iface );
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

static HRESULT WINAPI domtext_deleteData(
    IXMLDOMText *iface,
    long offset, long count)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domtext_replaceData(
    IXMLDOMText *iface,
    long offset, long count, BSTR p)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domtext_splitText(
    IXMLDOMText *iface,
    long offset, IXMLDOMText **txtNode)
{
    FIXME("\n");
    return E_NOTIMPL;
}


static const struct IXMLDOMTextVtbl domtext_vtbl =
{
    domtext_QueryInterface,
    domtext_AddRef,
    domtext_Release,
    domtext_GetTypeInfoCount,
    domtext_GetTypeInfo,
    domtext_GetIDsOfNames,
    domtext_Invoke,
    domtext_get_nodeName,
    domtext_get_nodeValue,
    domtext_put_nodeValue,
    domtext_get_nodeType,
    domtext_get_parentNode,
    domtext_get_childNodes,
    domtext_get_firstChild,
    domtext_get_lastChild,
    domtext_get_previousSibling,
    domtext_get_nextSibling,
    domtext_get_attributes,
    domtext_insertBefore,
    domtext_replaceChild,
    domtext_removeChild,
    domtext_appendChild,
    domtext_hasChildNodes,
    domtext_get_ownerDocument,
    domtext_cloneNode,
    domtext_get_nodeTypeString,
    domtext_get_text,
    domtext_put_text,
    domtext_get_specified,
    domtext_get_definition,
    domtext_get_nodeTypedValue,
    domtext_put_nodeTypedValue,
    domtext_get_dataType,
    domtext_put_dataType,
    domtext_get_xml,
    domtext_transformNode,
    domtext_selectNodes,
    domtext_selectSingleNode,
    domtext_get_parsed,
    domtext_get_namespaceURI,
    domtext_get_prefix,
    domtext_get_baseName,
    domtext_transformNodeToObject,
    domtext_get_data,
    domtext_put_data,
    domtext_get_length,
    domtext_substringData,
    domtext_appendData,
    domtext_insertData,
    domtext_deleteData,
    domtext_replaceData,
    domtext_splitText
};

IUnknown* create_text( xmlNodePtr text )
{
    domtext *This;
    HRESULT hr;

    This = HeapAlloc( GetProcessHeap(), 0, sizeof *This );
    if ( !This )
        return NULL;

    This->lpVtbl = &domtext_vtbl;
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
