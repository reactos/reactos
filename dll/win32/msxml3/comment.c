/*
 *    DOM comment node implementation
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
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "msxml2.h"

#include "msxml_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

#ifdef HAVE_LIBXML2

typedef struct _domcomment
{
    const struct IXMLDOMCommentVtbl *lpVtbl;
    LONG ref;
    IUnknown *node_unk;
    IXMLDOMNode *node;
} domcomment;

static inline domcomment *impl_from_IXMLDOMComment( IXMLDOMComment *iface )
{
    return (domcomment *)((char*)iface - FIELD_OFFSET(domcomment, lpVtbl));
}

static HRESULT WINAPI domcomment_QueryInterface(
    IXMLDOMComment *iface,
    REFIID riid,
    void** ppvObject )
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    TRACE("%p %s %p\n", This, debugstr_guid(riid), ppvObject);

    if ( IsEqualGUID( riid, &IID_IXMLDOMComment ) ||
         IsEqualGUID( riid, &IID_IDispatch ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = iface;
    }
    else if ( IsEqualGUID( riid, &IID_IXMLDOMNode ) )
    {
        return IUnknown_QueryInterface(This->node_unk, riid, ppvObject);
    }
    else
    {
        FIXME("Unsupported interface %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IXMLDOMComment_AddRef( iface );

    return S_OK;
}

static ULONG WINAPI domcomment_AddRef(
    IXMLDOMComment *iface )
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return InterlockedIncrement( &This->ref );
}

static ULONG WINAPI domcomment_Release(
    IXMLDOMComment *iface )
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    ULONG ref;

    ref = InterlockedDecrement( &This->ref );
    if ( ref == 0 )
    {
        IUnknown_Release( This->node_unk );
        HeapFree( GetProcessHeap(), 0, This );
    }

    return ref;
}

static HRESULT WINAPI domcomment_GetTypeInfoCount(
    IXMLDOMComment *iface,
    UINT* pctinfo )
{
    domcomment *This = impl_from_IXMLDOMComment( iface );

    TRACE("(%p)->(%p)\n", This, pctinfo);

    *pctinfo = 1;

    return S_OK;
}

static HRESULT WINAPI domcomment_GetTypeInfo(
    IXMLDOMComment *iface,
    UINT iTInfo, LCID lcid,
    ITypeInfo** ppTInfo )
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    HRESULT hr;

    TRACE("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);

    hr = get_typeinfo(IXMLDOMComment_tid, ppTInfo);

    return hr;
}

static HRESULT WINAPI domcomment_GetIDsOfNames(
    IXMLDOMComment *iface,
    REFIID riid, LPOLESTR* rgszNames,
    UINT cNames, LCID lcid, DISPID* rgDispId )
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);

    if(!rgszNames || cNames == 0 || !rgDispId)
        return E_INVALIDARG;

    hr = get_typeinfo(IXMLDOMComment_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI domcomment_Invoke(
    IXMLDOMComment *iface,
    DISPID dispIdMember, REFIID riid, LCID lcid,
    WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult,
    EXCEPINFO* pExcepInfo, UINT* puArgErr )
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IXMLDOMComment_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, &(This->lpVtbl), dispIdMember, wFlags, pDispParams,
                pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI domcomment_get_nodeName(
    IXMLDOMComment *iface,
    BSTR* p )
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_get_nodeName( This->node, p );
}

static HRESULT WINAPI domcomment_get_nodeValue(
    IXMLDOMComment *iface,
    VARIANT* var1 )
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_get_nodeValue( This->node, var1 );
}

static HRESULT WINAPI domcomment_put_nodeValue(
    IXMLDOMComment *iface,
    VARIANT var1 )
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_put_nodeValue( This->node, var1 );
}

static HRESULT WINAPI domcomment_get_nodeType(
    IXMLDOMComment *iface,
    DOMNodeType* domNodeType )
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_get_nodeType( This->node, domNodeType );
}

static HRESULT WINAPI domcomment_get_parentNode(
    IXMLDOMComment *iface,
    IXMLDOMNode** parent )
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_get_parentNode( This->node, parent );
}

static HRESULT WINAPI domcomment_get_childNodes(
    IXMLDOMComment *iface,
    IXMLDOMNodeList** outList)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_get_childNodes( This->node, outList );
}

static HRESULT WINAPI domcomment_get_firstChild(
    IXMLDOMComment *iface,
    IXMLDOMNode** domNode)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_get_firstChild( This->node, domNode );
}

static HRESULT WINAPI domcomment_get_lastChild(
    IXMLDOMComment *iface,
    IXMLDOMNode** domNode)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_get_lastChild( This->node, domNode );
}

static HRESULT WINAPI domcomment_get_previousSibling(
    IXMLDOMComment *iface,
    IXMLDOMNode** domNode)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_get_previousSibling( This->node, domNode );
}

static HRESULT WINAPI domcomment_get_nextSibling(
    IXMLDOMComment *iface,
    IXMLDOMNode** domNode)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_get_nextSibling( This->node, domNode );
}

static HRESULT WINAPI domcomment_get_attributes(
    IXMLDOMComment *iface,
    IXMLDOMNamedNodeMap** attributeMap)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_get_attributes( This->node, attributeMap );
}

static HRESULT WINAPI domcomment_insertBefore(
    IXMLDOMComment *iface,
    IXMLDOMNode* newNode, VARIANT var1,
    IXMLDOMNode** outOldNode)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_insertBefore( This->node, newNode, var1, outOldNode );
}

static HRESULT WINAPI domcomment_replaceChild(
    IXMLDOMComment *iface,
    IXMLDOMNode* newNode,
    IXMLDOMNode* oldNode,
    IXMLDOMNode** outOldNode)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_replaceChild( This->node, newNode, oldNode, outOldNode );
}

static HRESULT WINAPI domcomment_removeChild(
    IXMLDOMComment *iface,
    IXMLDOMNode* domNode, IXMLDOMNode** oldNode)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_removeChild( This->node, domNode, oldNode );
}

static HRESULT WINAPI domcomment_appendChild(
    IXMLDOMComment *iface,
    IXMLDOMNode* newNode, IXMLDOMNode** outNewNode)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_appendChild( This->node, newNode, outNewNode );
}

static HRESULT WINAPI domcomment_hasChildNodes(
    IXMLDOMComment *iface,
    VARIANT_BOOL* pbool)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_hasChildNodes( This->node, pbool );
}

static HRESULT WINAPI domcomment_get_ownerDocument(
    IXMLDOMComment *iface,
    IXMLDOMDocument** domDocument)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_get_ownerDocument( This->node, domDocument );
}

static HRESULT WINAPI domcomment_cloneNode(
    IXMLDOMComment *iface,
    VARIANT_BOOL pbool, IXMLDOMNode** outNode)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_cloneNode( This->node, pbool, outNode );
}

static HRESULT WINAPI domcomment_get_nodeTypeString(
    IXMLDOMComment *iface,
    BSTR* p)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_get_nodeTypeString( This->node, p );
}

static HRESULT WINAPI domcomment_get_text(
    IXMLDOMComment *iface,
    BSTR* p)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_get_text( This->node, p );
}

static HRESULT WINAPI domcomment_put_text(
    IXMLDOMComment *iface,
    BSTR p)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_put_text( This->node, p );
}

static HRESULT WINAPI domcomment_get_specified(
    IXMLDOMComment *iface,
    VARIANT_BOOL* pbool)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_get_specified( This->node, pbool );
}

static HRESULT WINAPI domcomment_get_definition(
    IXMLDOMComment *iface,
    IXMLDOMNode** domNode)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_get_definition( This->node, domNode );
}

static HRESULT WINAPI domcomment_get_nodeTypedValue(
    IXMLDOMComment *iface,
    VARIANT* var1)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_get_nodeTypedValue( This->node, var1 );
}

static HRESULT WINAPI domcomment_put_nodeTypedValue(
    IXMLDOMComment *iface,
    VARIANT var1)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_put_nodeTypedValue( This->node, var1 );
}

static HRESULT WINAPI domcomment_get_dataType(
    IXMLDOMComment *iface,
    VARIANT* var1)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_get_dataType( This->node, var1 );
}

static HRESULT WINAPI domcomment_put_dataType(
    IXMLDOMComment *iface,
    BSTR p)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_put_dataType( This->node, p );
}

static HRESULT WINAPI domcomment_get_xml(
    IXMLDOMComment *iface,
    BSTR* p)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_get_xml( This->node, p );
}

static HRESULT WINAPI domcomment_transformNode(
    IXMLDOMComment *iface,
    IXMLDOMNode* domNode, BSTR* p)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_transformNode( This->node, domNode, p );
}

static HRESULT WINAPI domcomment_selectNodes(
    IXMLDOMComment *iface,
    BSTR p, IXMLDOMNodeList** outList)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_selectNodes( This->node, p, outList );
}

static HRESULT WINAPI domcomment_selectSingleNode(
    IXMLDOMComment *iface,
    BSTR p, IXMLDOMNode** outNode)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_selectSingleNode( This->node, p, outNode );
}

static HRESULT WINAPI domcomment_get_parsed(
    IXMLDOMComment *iface,
    VARIANT_BOOL* pbool)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_get_parsed( This->node, pbool );
}

static HRESULT WINAPI domcomment_get_namespaceURI(
    IXMLDOMComment *iface,
    BSTR* p)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_get_namespaceURI( This->node, p );
}

static HRESULT WINAPI domcomment_get_prefix(
    IXMLDOMComment *iface,
    BSTR* p)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_get_prefix( This->node, p );
}

static HRESULT WINAPI domcomment_get_baseName(
    IXMLDOMComment *iface,
    BSTR* p)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_get_baseName( This->node, p );
}

static HRESULT WINAPI domcomment_transformNodeToObject(
    IXMLDOMComment *iface,
    IXMLDOMNode* domNode, VARIANT var1)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_transformNodeToObject( This->node, domNode, var1 );
}

static HRESULT WINAPI domcomment_get_data(
    IXMLDOMComment *iface,
    BSTR *p)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
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

static HRESULT WINAPI domcomment_put_data(
    IXMLDOMComment *iface,
    BSTR data)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    HRESULT hr = E_FAIL;
    VARIANT val;

    TRACE("%p %s\n", This, debugstr_w(data) );

    V_VT(&val) = VT_BSTR;
    V_BSTR(&val) = data;

    hr = IXMLDOMNode_put_nodeValue( This->node, val );

    return hr;
}

static HRESULT WINAPI domcomment_get_length(
    IXMLDOMComment *iface,
    long *len)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
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

static HRESULT WINAPI domcomment_substringData(
    IXMLDOMComment *iface,
    long offset, long count, BSTR *p)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
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

static HRESULT WINAPI domcomment_appendData(
    IXMLDOMComment *iface,
    BSTR p)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    xmlnode *pDOMNode = impl_from_IXMLDOMNode( This->node );
    xmlChar *pContent;
    HRESULT hr = S_FALSE;

    TRACE("%p\n", iface);

    /* Nothing to do if NULL or an Empty string passed in. */
    if(p == NULL || SysStringLen(p) == 0)
        return S_OK;

    pContent = xmlChar_from_wchar( (WCHAR*)p );
    if(pContent)
    {
        /* Older versions of libxml < 2.6.27 didn't correctly support
           xmlTextConcat on Comment nodes. Fallback to setting the
           contents directly if xmlTextConcat fails.

           NOTE: if xmlTextConcat fails, pContent is destroyed.
         */
        if(xmlTextConcat(pDOMNode->node, pContent, SysStringLen(p) ) == 0)
            hr = S_OK;
        else
        {
            xmlChar *pNew;
            pContent = xmlChar_from_wchar( (WCHAR*)p );
            if(pContent)
            {
                pNew = xmlStrcat(xmlNodeGetContent(pDOMNode->node), pContent);
                if(pNew)
                {
                    xmlNodeSetContent(pDOMNode->node, pNew);
                    hr = S_OK;
                }
                else
                    hr = E_FAIL;
            }
            else
                hr = E_FAIL;
        }
    }
    else
        hr = E_FAIL;

    return hr;
}

static HRESULT WINAPI domcomment_insertData(
    IXMLDOMComment *iface,
    long offset, BSTR p)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
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

            str = xmlChar_from_wchar((WCHAR*)sNewString);
            if(str)
            {
                xmlNodeSetContent(pDOMNode->node, str);
                hr = S_OK;
            }

            SysFreeString(sNewString);
        }

        SysFreeString(sContent);

        xmlFree(pXmlContent);
    }

    return hr;
}

static HRESULT WINAPI domcomment_deleteData(
    IXMLDOMComment *iface,
    long offset, long count)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI domcomment_replaceData(
    IXMLDOMComment *iface,
    long offset, long count, BSTR p)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static const struct IXMLDOMCommentVtbl domcomment_vtbl =
{
    domcomment_QueryInterface,
    domcomment_AddRef,
    domcomment_Release,
    domcomment_GetTypeInfoCount,
    domcomment_GetTypeInfo,
    domcomment_GetIDsOfNames,
    domcomment_Invoke,
    domcomment_get_nodeName,
    domcomment_get_nodeValue,
    domcomment_put_nodeValue,
    domcomment_get_nodeType,
    domcomment_get_parentNode,
    domcomment_get_childNodes,
    domcomment_get_firstChild,
    domcomment_get_lastChild,
    domcomment_get_previousSibling,
    domcomment_get_nextSibling,
    domcomment_get_attributes,
    domcomment_insertBefore,
    domcomment_replaceChild,
    domcomment_removeChild,
    domcomment_appendChild,
    domcomment_hasChildNodes,
    domcomment_get_ownerDocument,
    domcomment_cloneNode,
    domcomment_get_nodeTypeString,
    domcomment_get_text,
    domcomment_put_text,
    domcomment_get_specified,
    domcomment_get_definition,
    domcomment_get_nodeTypedValue,
    domcomment_put_nodeTypedValue,
    domcomment_get_dataType,
    domcomment_put_dataType,
    domcomment_get_xml,
    domcomment_transformNode,
    domcomment_selectNodes,
    domcomment_selectSingleNode,
    domcomment_get_parsed,
    domcomment_get_namespaceURI,
    domcomment_get_prefix,
    domcomment_get_baseName,
    domcomment_transformNodeToObject,
    domcomment_get_data,
    domcomment_put_data,
    domcomment_get_length,
    domcomment_substringData,
    domcomment_appendData,
    domcomment_insertData,
    domcomment_deleteData,
    domcomment_replaceData
};

IUnknown* create_comment( xmlNodePtr comment )
{
    domcomment *This;
    HRESULT hr;

    This = HeapAlloc( GetProcessHeap(), 0, sizeof *This );
    if ( !This )
        return NULL;

    This->lpVtbl = &domcomment_vtbl;
    This->ref = 1;

    This->node_unk = create_basic_node( comment, (IUnknown*)&This->lpVtbl );
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
