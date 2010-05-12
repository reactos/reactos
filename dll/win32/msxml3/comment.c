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
    xmlnode node;
    const struct IXMLDOMCommentVtbl *lpVtbl;
    LONG ref;
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
    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppvObject);

    if ( IsEqualGUID( riid, &IID_IXMLDOMComment ) ||
         IsEqualGUID( riid, &IID_IXMLDOMCharacterData) ||
         IsEqualGUID( riid, &IID_IDispatch ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = iface;
    }
    else if ( IsEqualGUID( riid, &IID_IXMLDOMNode ) )
    {
        *ppvObject = IXMLDOMNode_from_impl(&This->node);
    }
    else
    {
        FIXME("Unsupported interface %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IXMLDOMText_AddRef((IUnknown*)*ppvObject);
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
        destroy_xmlnode(&This->node);
        heap_free( This );
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
    return IXMLDOMNode_get_nodeName( IXMLDOMNode_from_impl(&This->node), p );
}

static HRESULT WINAPI domcomment_get_nodeValue(
    IXMLDOMComment *iface,
    VARIANT* var1 )
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_get_nodeValue( IXMLDOMNode_from_impl(&This->node), var1 );
}

static HRESULT WINAPI domcomment_put_nodeValue(
    IXMLDOMComment *iface,
    VARIANT var1 )
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_put_nodeValue( IXMLDOMNode_from_impl(&This->node), var1 );
}

static HRESULT WINAPI domcomment_get_nodeType(
    IXMLDOMComment *iface,
    DOMNodeType* domNodeType )
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_get_nodeType( IXMLDOMNode_from_impl(&This->node), domNodeType );
}

static HRESULT WINAPI domcomment_get_parentNode(
    IXMLDOMComment *iface,
    IXMLDOMNode** parent )
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_get_parentNode( IXMLDOMNode_from_impl(&This->node), parent );
}

static HRESULT WINAPI domcomment_get_childNodes(
    IXMLDOMComment *iface,
    IXMLDOMNodeList** outList)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_get_childNodes( IXMLDOMNode_from_impl(&This->node), outList );
}

static HRESULT WINAPI domcomment_get_firstChild(
    IXMLDOMComment *iface,
    IXMLDOMNode** domNode)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_get_firstChild( IXMLDOMNode_from_impl(&This->node), domNode );
}

static HRESULT WINAPI domcomment_get_lastChild(
    IXMLDOMComment *iface,
    IXMLDOMNode** domNode)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_get_lastChild( IXMLDOMNode_from_impl(&This->node), domNode );
}

static HRESULT WINAPI domcomment_get_previousSibling(
    IXMLDOMComment *iface,
    IXMLDOMNode** domNode)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_get_previousSibling( IXMLDOMNode_from_impl(&This->node), domNode );
}

static HRESULT WINAPI domcomment_get_nextSibling(
    IXMLDOMComment *iface,
    IXMLDOMNode** domNode)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_get_nextSibling( IXMLDOMNode_from_impl(&This->node), domNode );
}

static HRESULT WINAPI domcomment_get_attributes(
    IXMLDOMComment *iface,
    IXMLDOMNamedNodeMap** attributeMap)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_get_attributes( IXMLDOMNode_from_impl(&This->node), attributeMap );
}

static HRESULT WINAPI domcomment_insertBefore(
    IXMLDOMComment *iface,
    IXMLDOMNode* newNode, VARIANT var1,
    IXMLDOMNode** outOldNode)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_insertBefore( IXMLDOMNode_from_impl(&This->node), newNode, var1, outOldNode );
}

static HRESULT WINAPI domcomment_replaceChild(
    IXMLDOMComment *iface,
    IXMLDOMNode* newNode,
    IXMLDOMNode* oldNode,
    IXMLDOMNode** outOldNode)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_replaceChild( IXMLDOMNode_from_impl(&This->node), newNode, oldNode, outOldNode );
}

static HRESULT WINAPI domcomment_removeChild(
    IXMLDOMComment *iface,
    IXMLDOMNode* domNode, IXMLDOMNode** oldNode)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_removeChild( IXMLDOMNode_from_impl(&This->node), domNode, oldNode );
}

static HRESULT WINAPI domcomment_appendChild(
    IXMLDOMComment *iface,
    IXMLDOMNode* newNode, IXMLDOMNode** outNewNode)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_appendChild( IXMLDOMNode_from_impl(&This->node), newNode, outNewNode );
}

static HRESULT WINAPI domcomment_hasChildNodes(
    IXMLDOMComment *iface,
    VARIANT_BOOL* pbool)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_hasChildNodes( IXMLDOMNode_from_impl(&This->node), pbool );
}

static HRESULT WINAPI domcomment_get_ownerDocument(
    IXMLDOMComment *iface,
    IXMLDOMDocument** domDocument)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_get_ownerDocument( IXMLDOMNode_from_impl(&This->node), domDocument );
}

static HRESULT WINAPI domcomment_cloneNode(
    IXMLDOMComment *iface,
    VARIANT_BOOL pbool, IXMLDOMNode** outNode)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_cloneNode( IXMLDOMNode_from_impl(&This->node), pbool, outNode );
}

static HRESULT WINAPI domcomment_get_nodeTypeString(
    IXMLDOMComment *iface,
    BSTR* p)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_get_nodeTypeString( IXMLDOMNode_from_impl(&This->node), p );
}

static HRESULT WINAPI domcomment_get_text(
    IXMLDOMComment *iface,
    BSTR* p)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_get_text( IXMLDOMNode_from_impl(&This->node), p );
}

static HRESULT WINAPI domcomment_put_text(
    IXMLDOMComment *iface,
    BSTR p)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_put_text( IXMLDOMNode_from_impl(&This->node), p );
}

static HRESULT WINAPI domcomment_get_specified(
    IXMLDOMComment *iface,
    VARIANT_BOOL* pbool)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_get_specified( IXMLDOMNode_from_impl(&This->node), pbool );
}

static HRESULT WINAPI domcomment_get_definition(
    IXMLDOMComment *iface,
    IXMLDOMNode** domNode)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_get_definition( IXMLDOMNode_from_impl(&This->node), domNode );
}

static HRESULT WINAPI domcomment_get_nodeTypedValue(
    IXMLDOMComment *iface,
    VARIANT* var1)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_get_nodeTypedValue( IXMLDOMNode_from_impl(&This->node), var1 );
}

static HRESULT WINAPI domcomment_put_nodeTypedValue(
    IXMLDOMComment *iface,
    VARIANT var1)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_put_nodeTypedValue( IXMLDOMNode_from_impl(&This->node), var1 );
}

static HRESULT WINAPI domcomment_get_dataType(
    IXMLDOMComment *iface,
    VARIANT* var1)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_get_dataType( IXMLDOMNode_from_impl(&This->node), var1 );
}

static HRESULT WINAPI domcomment_put_dataType(
    IXMLDOMComment *iface,
    BSTR p)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_put_dataType( IXMLDOMNode_from_impl(&This->node), p );
}

static HRESULT WINAPI domcomment_get_xml(
    IXMLDOMComment *iface,
    BSTR* p)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_get_xml( IXMLDOMNode_from_impl(&This->node), p );
}

static HRESULT WINAPI domcomment_transformNode(
    IXMLDOMComment *iface,
    IXMLDOMNode* domNode, BSTR* p)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_transformNode( IXMLDOMNode_from_impl(&This->node), domNode, p );
}

static HRESULT WINAPI domcomment_selectNodes(
    IXMLDOMComment *iface,
    BSTR p, IXMLDOMNodeList** outList)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_selectNodes( IXMLDOMNode_from_impl(&This->node), p, outList );
}

static HRESULT WINAPI domcomment_selectSingleNode(
    IXMLDOMComment *iface,
    BSTR p, IXMLDOMNode** outNode)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_selectSingleNode( IXMLDOMNode_from_impl(&This->node), p, outNode );
}

static HRESULT WINAPI domcomment_get_parsed(
    IXMLDOMComment *iface,
    VARIANT_BOOL* pbool)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_get_parsed( IXMLDOMNode_from_impl(&This->node), pbool );
}

static HRESULT WINAPI domcomment_get_namespaceURI(
    IXMLDOMComment *iface,
    BSTR* p)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_get_namespaceURI( IXMLDOMNode_from_impl(&This->node), p );
}

static HRESULT WINAPI domcomment_get_prefix(
    IXMLDOMComment *iface,
    BSTR* p)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_get_prefix( IXMLDOMNode_from_impl(&This->node), p );
}

static HRESULT WINAPI domcomment_get_baseName(
    IXMLDOMComment *iface,
    BSTR* p)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_get_baseName( IXMLDOMNode_from_impl(&This->node), p );
}

static HRESULT WINAPI domcomment_transformNodeToObject(
    IXMLDOMComment *iface,
    IXMLDOMNode* domNode, VARIANT var1)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IXMLDOMNode_transformNodeToObject( IXMLDOMNode_from_impl(&This->node), domNode, var1 );
}

static HRESULT WINAPI domcomment_get_data(
    IXMLDOMComment *iface,
    BSTR *p)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    HRESULT hr;
    VARIANT vRet;

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_INVALIDARG;

    hr = IXMLDOMNode_get_nodeValue( IXMLDOMNode_from_impl(&This->node), &vRet );
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
    VARIANT val;

    TRACE("(%p)->(%s)\n", This, debugstr_w(data) );

    V_VT(&val) = VT_BSTR;
    V_BSTR(&val) = data;

    return IXMLDOMNode_put_nodeValue( IXMLDOMNode_from_impl(&This->node), val );
}

static HRESULT WINAPI domcomment_get_length(
    IXMLDOMComment *iface,
    LONG *len)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    HRESULT hr;
    BSTR data;

    TRACE("(%p)->(%p)\n", This, len);

    if(!len)
        return E_INVALIDARG;

    hr = IXMLDOMComment_get_data(iface, &data);
    if(hr == S_OK)
    {
        *len = SysStringLen(data);
        SysFreeString(data);
    }

    return hr;
}

static HRESULT WINAPI domcomment_substringData(
    IXMLDOMComment *iface,
    LONG offset, LONG count, BSTR *p)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    HRESULT hr;
    BSTR data;

    TRACE("(%p)->(%d %d %p)\n", This, offset, count, p);

    if(!p)
        return E_INVALIDARG;

    *p = NULL;
    if(offset < 0 || count < 0)
        return E_INVALIDARG;

    if(count == 0)
        return S_FALSE;

    hr = IXMLDOMComment_get_data(iface, &data);
    if(hr == S_OK)
    {
        LONG len = SysStringLen(data);

        if(offset < len)
        {
            if(offset + count > len)
                *p = SysAllocString(&data[offset]);
            else
                *p = SysAllocStringLen(&data[offset], count);
        }
        else
            hr = S_FALSE;

        SysFreeString(data);
    }

    return hr;
}

static HRESULT WINAPI domcomment_appendData(
    IXMLDOMComment *iface,
    BSTR p)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    HRESULT hr;
    BSTR data;
    LONG p_len;

    TRACE("(%p)->(%s)\n", This, debugstr_w(p));

    /* Nothing to do if NULL or an Empty string passed in. */
    if((p_len = SysStringLen(p)) == 0) return S_OK;

    hr = IXMLDOMComment_get_data(iface, &data);
    if(hr == S_OK)
    {
        LONG len = SysStringLen(data);
        BSTR str = SysAllocStringLen(NULL, p_len + len);

        memcpy(str, data, len*sizeof(WCHAR));
        memcpy(&str[len], p, p_len*sizeof(WCHAR));
        str[len+p_len] = 0;

        hr = IXMLDOMComment_put_data(iface, str);

        SysFreeString(str);
        SysFreeString(data);
    }

    return hr;
}

static HRESULT WINAPI domcomment_insertData(
    IXMLDOMComment *iface,
    LONG offset, BSTR p)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    HRESULT hr;
    BSTR data;
    LONG p_len;

    TRACE("(%p)->(%d %s)\n", This, offset, debugstr_w(p));

    /* If have a NULL or empty string, don't do anything. */
    if((p_len = SysStringLen(p)) == 0)
        return S_OK;

    if(offset < 0)
    {
        return E_INVALIDARG;
    }

    hr = IXMLDOMComment_get_data(iface, &data);
    if(hr == S_OK)
    {
        LONG len = SysStringLen(data);
        BSTR str;

        if(len < offset)
        {
            SysFreeString(data);
            return E_INVALIDARG;
        }

        str = SysAllocStringLen(NULL, len + p_len);
        /* start part, supplied string and end part */
        memcpy(str, data, offset*sizeof(WCHAR));
        memcpy(&str[offset], p, p_len*sizeof(WCHAR));
        memcpy(&str[offset+p_len], &data[offset], (len-offset)*sizeof(WCHAR));
        str[len+p_len] = 0;

        hr = IXMLDOMComment_put_data(iface, str);

        SysFreeString(str);
        SysFreeString(data);
    }

    return hr;
}

static HRESULT WINAPI domcomment_deleteData(
    IXMLDOMComment *iface,
    LONG offset, LONG count)
{
    HRESULT hr;
    LONG len = -1;
    BSTR str;

    TRACE("(%p)->(%d %d)\n", iface, offset, count);

    hr = IXMLDOMComment_get_length(iface, &len);
    if(hr != S_OK) return hr;

    if((offset < 0) || (offset > len) || (count < 0))
        return E_INVALIDARG;

    if(len == 0) return S_OK;

    /* cutting start or end */
    if((offset == 0) || ((count + offset) >= len))
    {
        if(offset == 0)
            IXMLDOMComment_substringData(iface, count, len - count, &str);
        else
            IXMLDOMComment_substringData(iface, 0, offset, &str);
        hr = IXMLDOMComment_put_data(iface, str);
    }
    else
    /* cutting from the inside */
    {
        BSTR str_end;

        IXMLDOMComment_substringData(iface, 0, offset, &str);
        IXMLDOMComment_substringData(iface, offset + count, len - count, &str_end);

        hr = IXMLDOMComment_put_data(iface, str);
        if(hr == S_OK)
            hr = IXMLDOMComment_appendData(iface, str_end);

        SysFreeString(str_end);
    }

    SysFreeString(str);

    return hr;
}

static HRESULT WINAPI domcomment_replaceData(
    IXMLDOMComment *iface,
    LONG offset, LONG count, BSTR p)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    HRESULT hr;

    TRACE("(%p)->(%d %d %s)\n", This, offset, count, debugstr_w(p));

    hr = IXMLDOMComment_deleteData(iface, offset, count);

    if (hr == S_OK)
       hr = IXMLDOMComment_insertData(iface, offset, p);

    return hr;
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

    This = heap_alloc( sizeof *This );
    if ( !This )
        return NULL;

    This->lpVtbl = &domcomment_vtbl;
    This->ref = 1;

    init_xmlnode(&This->node, comment, (IUnknown*)&This->lpVtbl, NULL);

    return (IUnknown*) &This->lpVtbl;
}

#endif
