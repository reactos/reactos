/*
 *    Node list implementation
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
#include "ole2.h"
#include "msxml2.h"

#include "msxml_private.h"

#include "wine/debug.h"

/* This file implements the object returned by childNodes property. Note that this is
 * not the IXMLDOMNodeList returned by XPath querites - it's implemented in queryresult.c.
 * They are different because the list returned by childNodes:
 *  - is "live" - changes to the XML tree are automatically reflected in the list
 *  - doesn't supports IXMLDOMSelection
 *  - note that an attribute node have a text child in DOM but not in the XPath data model
 *    thus the child is inaccessible by an XPath query
 */

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

#ifdef HAVE_LIBXML2

typedef struct _xmlnodelist
{
    const struct IXMLDOMNodeListVtbl *lpVtbl;
    LONG ref;
    xmlNodePtr parent;
    xmlNodePtr current;
} xmlnodelist;

static inline xmlnodelist *impl_from_IXMLDOMNodeList( IXMLDOMNodeList *iface )
{
    return (xmlnodelist *)((char*)iface - FIELD_OFFSET(xmlnodelist, lpVtbl));
}

static HRESULT WINAPI xmlnodelist_QueryInterface(
    IXMLDOMNodeList *iface,
    REFIID riid,
    void** ppvObject )
{
    TRACE("%p %s %p\n", iface, debugstr_guid(riid), ppvObject);

    if(!ppvObject)
        return E_INVALIDARG;

    if ( IsEqualGUID( riid, &IID_IUnknown ) ||
         IsEqualGUID( riid, &IID_IDispatch ) ||
         IsEqualGUID( riid, &IID_IXMLDOMNodeList ) )
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    IXMLDOMNodeList_AddRef( iface );

    return S_OK;
}

static ULONG WINAPI xmlnodelist_AddRef(
    IXMLDOMNodeList *iface )
{
    xmlnodelist *This = impl_from_IXMLDOMNodeList( iface );
    return InterlockedIncrement( &This->ref );
}

static ULONG WINAPI xmlnodelist_Release(
    IXMLDOMNodeList *iface )
{
    xmlnodelist *This = impl_from_IXMLDOMNodeList( iface );
    ULONG ref;

    ref = InterlockedDecrement( &This->ref );
    if ( ref == 0 )
    {
        xmldoc_release( This->parent->doc );
        HeapFree( GetProcessHeap(), 0, This );
    }

    return ref;
}

static HRESULT WINAPI xmlnodelist_GetTypeInfoCount(
    IXMLDOMNodeList *iface,
    UINT* pctinfo )
{
    xmlnodelist *This = impl_from_IXMLDOMNodeList( iface );

    TRACE("(%p)->(%p)\n", This, pctinfo);

    *pctinfo = 1;

    return S_OK;
}

static HRESULT WINAPI xmlnodelist_GetTypeInfo(
    IXMLDOMNodeList *iface,
    UINT iTInfo,
    LCID lcid,
    ITypeInfo** ppTInfo )
{
    xmlnodelist *This = impl_from_IXMLDOMNodeList( iface );
    HRESULT hr;

    TRACE("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);

    hr = get_typeinfo(IXMLDOMNodeList_tid, ppTInfo);

    return hr;
}

static HRESULT WINAPI xmlnodelist_GetIDsOfNames(
    IXMLDOMNodeList *iface,
    REFIID riid,
    LPOLESTR* rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID* rgDispId )
{
    xmlnodelist *This = impl_from_IXMLDOMNodeList( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);

    if(!rgszNames || cNames == 0 || !rgDispId)
        return E_INVALIDARG;

    hr = get_typeinfo(IXMLDOMNodeList_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI xmlnodelist_Invoke(
    IXMLDOMNodeList *iface,
    DISPID dispIdMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS* pDispParams,
    VARIANT* pVarResult,
    EXCEPINFO* pExcepInfo,
    UINT* puArgErr )
{
    xmlnodelist *This = impl_from_IXMLDOMNodeList( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IXMLDOMNodeList_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, &(This->lpVtbl), dispIdMember, wFlags, pDispParams,
                pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI xmlnodelist_get_item(
        IXMLDOMNodeList* iface,
        long index,
        IXMLDOMNode** listItem)
{
    xmlnodelist *This = impl_from_IXMLDOMNodeList( iface );
    xmlNodePtr curr;
    long nodeIndex = 0;

    TRACE("%p %ld\n", This, index);

    if(!listItem)
        return E_INVALIDARG;

    *listItem = NULL;

    if (index < 0)
        return S_FALSE;

    curr = This->parent->children;
    while(curr)
    {
        if(nodeIndex++ == index) break;
        curr = curr->next;
    }
    if(!curr) return S_FALSE;

    *listItem = create_node( curr );

    return S_OK;
}

static HRESULT WINAPI xmlnodelist_get_length(
        IXMLDOMNodeList* iface,
        long* listLength)
{

    xmlNodePtr curr;
    long nodeCount = 0;

    xmlnodelist *This = impl_from_IXMLDOMNodeList( iface );

    TRACE("%p\n", This);

    if(!listLength)
        return E_INVALIDARG;

    curr = This->parent->children;
    while (curr)
    {
        nodeCount++;
        curr = curr->next;
    }

    *listLength = nodeCount;
    return S_OK;
}

static HRESULT WINAPI xmlnodelist_nextNode(
        IXMLDOMNodeList* iface,
        IXMLDOMNode** nextItem)
{
    xmlnodelist *This = impl_from_IXMLDOMNodeList( iface );

    TRACE("%p %p\n", This, nextItem );

    if(!nextItem)
        return E_INVALIDARG;

    *nextItem = NULL;

    if (!This->current)
        return S_FALSE;

    *nextItem = create_node( This->current );
    This->current = This->current->next;
    return S_OK;
}

static HRESULT WINAPI xmlnodelist_reset(
        IXMLDOMNodeList* iface)
{
    xmlnodelist *This = impl_from_IXMLDOMNodeList( iface );

    TRACE("%p\n", This);
    This->current = This->parent->children;
    return S_OK;
}

static HRESULT WINAPI xmlnodelist__newEnum(
        IXMLDOMNodeList* iface,
        IUnknown** ppUnk)
{
    FIXME("\n");
    return E_NOTIMPL;
}


static const struct IXMLDOMNodeListVtbl xmlnodelist_vtbl =
{
    xmlnodelist_QueryInterface,
    xmlnodelist_AddRef,
    xmlnodelist_Release,
    xmlnodelist_GetTypeInfoCount,
    xmlnodelist_GetTypeInfo,
    xmlnodelist_GetIDsOfNames,
    xmlnodelist_Invoke,
    xmlnodelist_get_item,
    xmlnodelist_get_length,
    xmlnodelist_nextNode,
    xmlnodelist_reset,
    xmlnodelist__newEnum,
};

IXMLDOMNodeList* create_children_nodelist( xmlNodePtr node )
{
    xmlnodelist *nodelist;

    nodelist = HeapAlloc( GetProcessHeap(), 0, sizeof *nodelist );
    if ( !nodelist )
        return NULL;

    nodelist->lpVtbl = &xmlnodelist_vtbl;
    nodelist->ref = 1;
    nodelist->parent = node;
    nodelist->current = node->children;

    xmldoc_add_ref( node->doc );

    return (IXMLDOMNodeList*) &nodelist->lpVtbl;
}

#endif
