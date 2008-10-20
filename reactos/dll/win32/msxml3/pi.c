/*
 *    DOM processing instruction node implementation
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

typedef struct _dom_pi
{
    const struct IXMLDOMProcessingInstructionVtbl *lpVtbl;
    LONG ref;
    IUnknown *node_unk;
    IXMLDOMNode *node;
} dom_pi;

static inline dom_pi *impl_from_IXMLDOMProcessingInstruction( IXMLDOMProcessingInstruction *iface )
{
    return (dom_pi *)((char*)iface - FIELD_OFFSET(dom_pi, lpVtbl));
}

static HRESULT WINAPI dom_pi_QueryInterface(
    IXMLDOMProcessingInstruction *iface,
    REFIID riid,
    void** ppvObject )
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    TRACE("%p %s %p\n", This, debugstr_guid(riid), ppvObject);

    if ( IsEqualGUID( riid, &IID_IXMLDOMProcessingInstruction ) ||
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

    IXMLDOMProcessingInstruction_AddRef( iface );

    return S_OK;
}

static ULONG WINAPI dom_pi_AddRef(
    IXMLDOMProcessingInstruction *iface )
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    return InterlockedIncrement( &This->ref );
}

static ULONG WINAPI dom_pi_Release(
    IXMLDOMProcessingInstruction *iface )
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    ULONG ref;

    ref = InterlockedDecrement( &This->ref );
    if ( ref == 0 )
    {
        IUnknown_Release( This->node_unk );
        HeapFree( GetProcessHeap(), 0, This );
    }

    return ref;
}

static HRESULT WINAPI dom_pi_GetTypeInfoCount(
    IXMLDOMProcessingInstruction *iface,
    UINT* pctinfo )
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );

    TRACE("(%p)->(%p)\n", This, pctinfo);

    *pctinfo = 1;

    return S_OK;
}

static HRESULT WINAPI dom_pi_GetTypeInfo(
    IXMLDOMProcessingInstruction *iface,
    UINT iTInfo, LCID lcid,
    ITypeInfo** ppTInfo )
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    HRESULT hr;

    TRACE("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);

    hr = get_typeinfo(IXMLDOMProcessingInstruction_tid, ppTInfo);

    return hr;
}

static HRESULT WINAPI dom_pi_GetIDsOfNames(
    IXMLDOMProcessingInstruction *iface,
    REFIID riid, LPOLESTR* rgszNames,
    UINT cNames, LCID lcid, DISPID* rgDispId )
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);

    if(!rgszNames || cNames == 0 || !rgDispId)
        return E_INVALIDARG;

    hr = get_typeinfo(IXMLDOMProcessingInstruction_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI dom_pi_Invoke(
    IXMLDOMProcessingInstruction *iface,
    DISPID dispIdMember, REFIID riid, LCID lcid,
    WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult,
    EXCEPINFO* pExcepInfo, UINT* puArgErr )
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IXMLDOMProcessingInstruction_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
       hr = ITypeInfo_Invoke(typeinfo, &(This->lpVtbl), dispIdMember, wFlags, pDispParams,
                pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI dom_pi_get_nodeName(
    IXMLDOMProcessingInstruction *iface,
    BSTR* p )
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    return IXMLDOMNode_get_nodeName( This->node, p );
}

static HRESULT WINAPI dom_pi_get_nodeValue(
    IXMLDOMProcessingInstruction *iface,
    VARIANT* var1 )
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    return IXMLDOMNode_get_nodeValue( This->node, var1 );
}

static HRESULT WINAPI dom_pi_put_nodeValue(
    IXMLDOMProcessingInstruction *iface,
    VARIANT var1 )
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    BSTR sTarget;
    static const WCHAR szXML[] = {'x','m','l',0};
    HRESULT hr;

    TRACE("%p\n", This );

    /* Cannot set data to a PI node whose target is 'xml' */
    hr = dom_pi_get_nodeName(iface, &sTarget);
    if(hr == S_OK)
    {
        if(lstrcmpW( sTarget, szXML) == 0)
        {
            SysFreeString(sTarget);
            return E_FAIL;
        }

        SysFreeString(sTarget);
    }

    return IXMLDOMNode_put_nodeValue( This->node, var1 );
}

static HRESULT WINAPI dom_pi_get_nodeType(
    IXMLDOMProcessingInstruction *iface,
    DOMNodeType* domNodeType )
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    return IXMLDOMNode_get_nodeType( This->node, domNodeType );
}

static HRESULT WINAPI dom_pi_get_parentNode(
    IXMLDOMProcessingInstruction *iface,
    IXMLDOMNode** parent )
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    return IXMLDOMNode_get_parentNode( This->node, parent );
}

static HRESULT WINAPI dom_pi_get_childNodes(
    IXMLDOMProcessingInstruction *iface,
    IXMLDOMNodeList** outList)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    return IXMLDOMNode_get_childNodes( This->node, outList );
}

static HRESULT WINAPI dom_pi_get_firstChild(
    IXMLDOMProcessingInstruction *iface,
    IXMLDOMNode** domNode)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    return IXMLDOMNode_get_firstChild( This->node, domNode );
}

static HRESULT WINAPI dom_pi_get_lastChild(
    IXMLDOMProcessingInstruction *iface,
    IXMLDOMNode** domNode)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    return IXMLDOMNode_get_lastChild( This->node, domNode );
}

static HRESULT WINAPI dom_pi_get_previousSibling(
    IXMLDOMProcessingInstruction *iface,
    IXMLDOMNode** domNode)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    return IXMLDOMNode_get_previousSibling( This->node, domNode );
}

static HRESULT WINAPI dom_pi_get_nextSibling(
    IXMLDOMProcessingInstruction *iface,
    IXMLDOMNode** domNode)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    return IXMLDOMNode_get_nextSibling( This->node, domNode );
}

static HRESULT WINAPI dom_pi_get_attributes(
    IXMLDOMProcessingInstruction *iface,
    IXMLDOMNamedNodeMap** attributeMap)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    return IXMLDOMNode_get_attributes( This->node, attributeMap );
}

static HRESULT WINAPI dom_pi_insertBefore(
    IXMLDOMProcessingInstruction *iface,
    IXMLDOMNode* newNode, VARIANT var1,
    IXMLDOMNode** outOldNode)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    return IXMLDOMNode_insertBefore( This->node, newNode, var1, outOldNode );
}

static HRESULT WINAPI dom_pi_replaceChild(
    IXMLDOMProcessingInstruction *iface,
    IXMLDOMNode* newNode,
    IXMLDOMNode* oldNode,
    IXMLDOMNode** outOldNode)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    return IXMLDOMNode_replaceChild( This->node, newNode, oldNode, outOldNode );
}

static HRESULT WINAPI dom_pi_removeChild(
    IXMLDOMProcessingInstruction *iface,
    IXMLDOMNode* domNode, IXMLDOMNode** oldNode)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    return IXMLDOMNode_removeChild( This->node, domNode, oldNode );
}

static HRESULT WINAPI dom_pi_appendChild(
    IXMLDOMProcessingInstruction *iface,
    IXMLDOMNode* newNode, IXMLDOMNode** outNewNode)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    return IXMLDOMNode_appendChild( This->node, newNode, outNewNode );
}

static HRESULT WINAPI dom_pi_hasChildNodes(
    IXMLDOMProcessingInstruction *iface,
    VARIANT_BOOL* pbool)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    return IXMLDOMNode_hasChildNodes( This->node, pbool );
}

static HRESULT WINAPI dom_pi_get_ownerDocument(
    IXMLDOMProcessingInstruction *iface,
    IXMLDOMDocument** domDocument)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    return IXMLDOMNode_get_ownerDocument( This->node, domDocument );
}

static HRESULT WINAPI dom_pi_cloneNode(
    IXMLDOMProcessingInstruction *iface,
    VARIANT_BOOL pbool, IXMLDOMNode** outNode)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    return IXMLDOMNode_cloneNode( This->node, pbool, outNode );
}

static HRESULT WINAPI dom_pi_get_nodeTypeString(
    IXMLDOMProcessingInstruction *iface,
    BSTR* p)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    return IXMLDOMNode_get_nodeTypeString( This->node, p );
}

static HRESULT WINAPI dom_pi_get_text(
    IXMLDOMProcessingInstruction *iface,
    BSTR* p)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    return IXMLDOMNode_get_text( This->node, p );
}

static HRESULT WINAPI dom_pi_put_text(
    IXMLDOMProcessingInstruction *iface,
    BSTR p)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    return IXMLDOMNode_put_text( This->node, p );
}

static HRESULT WINAPI dom_pi_get_specified(
    IXMLDOMProcessingInstruction *iface,
    VARIANT_BOOL* pbool)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    return IXMLDOMNode_get_specified( This->node, pbool );
}

static HRESULT WINAPI dom_pi_get_definition(
    IXMLDOMProcessingInstruction *iface,
    IXMLDOMNode** domNode)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    return IXMLDOMNode_get_definition( This->node, domNode );
}

static HRESULT WINAPI dom_pi_get_nodeTypedValue(
    IXMLDOMProcessingInstruction *iface,
    VARIANT* var1)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    return IXMLDOMNode_get_nodeTypedValue( This->node, var1 );
}

static HRESULT WINAPI dom_pi_put_nodeTypedValue(
    IXMLDOMProcessingInstruction *iface,
    VARIANT var1)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    return IXMLDOMNode_put_nodeTypedValue( This->node, var1 );
}

static HRESULT WINAPI dom_pi_get_dataType(
    IXMLDOMProcessingInstruction *iface,
    VARIANT* var1)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    return IXMLDOMNode_get_dataType( This->node, var1 );
}

static HRESULT WINAPI dom_pi_put_dataType(
    IXMLDOMProcessingInstruction *iface,
    BSTR p)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    return IXMLDOMNode_put_dataType( This->node, p );
}

static HRESULT WINAPI dom_pi_get_xml(
    IXMLDOMProcessingInstruction *iface,
    BSTR* p)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    return IXMLDOMNode_get_xml( This->node, p );
}

static HRESULT WINAPI dom_pi_transformNode(
    IXMLDOMProcessingInstruction *iface,
    IXMLDOMNode* domNode, BSTR* p)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    return IXMLDOMNode_transformNode( This->node, domNode, p );
}

static HRESULT WINAPI dom_pi_selectNodes(
    IXMLDOMProcessingInstruction *iface,
    BSTR p, IXMLDOMNodeList** outList)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    return IXMLDOMNode_selectNodes( This->node, p, outList );
}

static HRESULT WINAPI dom_pi_selectSingleNode(
    IXMLDOMProcessingInstruction *iface,
    BSTR p, IXMLDOMNode** outNode)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    return IXMLDOMNode_selectSingleNode( This->node, p, outNode );
}

static HRESULT WINAPI dom_pi_get_parsed(
    IXMLDOMProcessingInstruction *iface,
    VARIANT_BOOL* pbool)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    return IXMLDOMNode_get_parsed( This->node, pbool );
}

static HRESULT WINAPI dom_pi_get_namespaceURI(
    IXMLDOMProcessingInstruction *iface,
    BSTR* p)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    return IXMLDOMNode_get_namespaceURI( This->node, p );
}

static HRESULT WINAPI dom_pi_get_prefix(
    IXMLDOMProcessingInstruction *iface,
    BSTR* p)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    return IXMLDOMNode_get_prefix( This->node, p );
}

static HRESULT WINAPI dom_pi_get_baseName(
    IXMLDOMProcessingInstruction *iface,
    BSTR* p)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    return IXMLDOMNode_get_baseName( This->node, p );
}

static HRESULT WINAPI dom_pi_transformNodeToObject(
    IXMLDOMProcessingInstruction *iface,
    IXMLDOMNode* domNode, VARIANT var1)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    return IXMLDOMNode_transformNodeToObject( This->node, domNode, var1 );
}

static HRESULT WINAPI dom_pi_get_target(
    IXMLDOMProcessingInstruction *iface,
    BSTR *p)
{
    /* target returns the same value as nodeName property */
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    return IXMLDOMNode_get_nodeName( This->node, p );
}

static HRESULT WINAPI dom_pi_get_data(
    IXMLDOMProcessingInstruction *iface,
    BSTR *p)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
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

static HRESULT WINAPI dom_pi_put_data(
    IXMLDOMProcessingInstruction *iface,
    BSTR data)
{
    dom_pi *This = impl_from_IXMLDOMProcessingInstruction( iface );
    HRESULT hr = E_FAIL;
    VARIANT val;
    BSTR sTarget;
    static const WCHAR szXML[] = {'x','m','l',0};

    TRACE("%p %s\n", This, debugstr_w(data) );

    /* Cannot set data to a PI node whose target is 'xml' */
    hr = dom_pi_get_nodeName(iface, &sTarget);
    if(hr == S_OK)
    {
        if(lstrcmpW( sTarget, szXML) == 0)
        {
            SysFreeString(sTarget);
            return E_FAIL;
        }

        SysFreeString(sTarget);
    }

    V_VT(&val) = VT_BSTR;
    V_BSTR(&val) = data;

    hr = IXMLDOMNode_put_nodeValue( This->node, val );

    return hr;
}

static const struct IXMLDOMProcessingInstructionVtbl dom_pi_vtbl =
{
    dom_pi_QueryInterface,
    dom_pi_AddRef,
    dom_pi_Release,
    dom_pi_GetTypeInfoCount,
    dom_pi_GetTypeInfo,
    dom_pi_GetIDsOfNames,
    dom_pi_Invoke,
    dom_pi_get_nodeName,
    dom_pi_get_nodeValue,
    dom_pi_put_nodeValue,
    dom_pi_get_nodeType,
    dom_pi_get_parentNode,
    dom_pi_get_childNodes,
    dom_pi_get_firstChild,
    dom_pi_get_lastChild,
    dom_pi_get_previousSibling,
    dom_pi_get_nextSibling,
    dom_pi_get_attributes,
    dom_pi_insertBefore,
    dom_pi_replaceChild,
    dom_pi_removeChild,
    dom_pi_appendChild,
    dom_pi_hasChildNodes,
    dom_pi_get_ownerDocument,
    dom_pi_cloneNode,
    dom_pi_get_nodeTypeString,
    dom_pi_get_text,
    dom_pi_put_text,
    dom_pi_get_specified,
    dom_pi_get_definition,
    dom_pi_get_nodeTypedValue,
    dom_pi_put_nodeTypedValue,
    dom_pi_get_dataType,
    dom_pi_put_dataType,
    dom_pi_get_xml,
    dom_pi_transformNode,
    dom_pi_selectNodes,
    dom_pi_selectSingleNode,
    dom_pi_get_parsed,
    dom_pi_get_namespaceURI,
    dom_pi_get_prefix,
    dom_pi_get_baseName,
    dom_pi_transformNodeToObject,

    dom_pi_get_target,
    dom_pi_get_data,
    dom_pi_put_data
};

IUnknown* create_pi( xmlNodePtr pi )
{
    dom_pi *This;
    HRESULT hr;

    This = HeapAlloc( GetProcessHeap(), 0, sizeof *This );
    if ( !This )
        return NULL;

    This->lpVtbl = &dom_pi_vtbl;
    This->ref = 1;

    This->node_unk = create_basic_node( pi, (IUnknown*)&This->lpVtbl );
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
