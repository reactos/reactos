/*
 *    XPath query result node list implementation (TODO: XSLPattern support)
 *
 * Copyright 2005 Mike McCormack
 * Copyright 2007 Mikolaj Zalewski
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

/* This file implements the object returned by a XPath query. Note that this is
 * not the IXMLDOMNodeList returned by childNodes - it's implemented in nodelist.c.
 * They are different because the list returned by XPath queries:
 *  - is static - gives the results for the XML tree as it existed during the
 *    execution of the query
 *  - supports IXMLDOMSelection (TODO)
 *
 * TODO: XSLPattern support
 */

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

#ifdef HAVE_LIBXML2

#include <libxml/xpath.h>

static const struct IXMLDOMNodeListVtbl queryresult_vtbl;

typedef struct _queryresult
{
    const struct IXMLDOMNodeListVtbl *lpVtbl;
    LONG ref;
    xmlNodePtr node;
    xmlXPathObjectPtr result;
    int resultPos;
} queryresult;

static inline queryresult *impl_from_IXMLDOMNodeList( IXMLDOMNodeList *iface )
{
    return (queryresult *)((char*)iface - FIELD_OFFSET(queryresult, lpVtbl));
}

HRESULT queryresult_create(xmlNodePtr node, LPWSTR szQuery, IXMLDOMNodeList **out)
{
    queryresult *This = CoTaskMemAlloc(sizeof(queryresult));
    xmlXPathContextPtr ctxt = xmlXPathNewContext(node->doc);
    xmlChar *str = xmlChar_from_wchar(szQuery);
    HRESULT hr;


    TRACE("(%p, %s, %p)\n", node, wine_dbgstr_w(szQuery), out);

    *out = NULL;
    if (This == NULL || ctxt == NULL || str == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    This->lpVtbl = &queryresult_vtbl;
    This->ref = 1;
    This->resultPos = 0;
    This->node = node;
    xmldoc_add_ref(This->node->doc);

    ctxt->node = node;
    This->result = xmlXPathEval(str, ctxt);
    if (!This->result || This->result->type != XPATH_NODESET)
    {
        hr = E_FAIL;
        goto cleanup;
    }

    *out = (IXMLDOMNodeList *)This;
    hr = S_OK;
    TRACE("found %d matches\n", This->result->nodesetval->nodeNr);

cleanup:
    if (This != NULL && FAILED(hr))
        IXMLDOMNodeList_Release( (IXMLDOMNodeList*) &This->lpVtbl );
    if (ctxt != NULL)
        xmlXPathFreeContext(ctxt);
    HeapFree(GetProcessHeap(), 0, str);
    return hr;
}


static HRESULT WINAPI queryresult_QueryInterface(
    IXMLDOMNodeList *iface,
    REFIID riid,
    void** ppvObject )
{
    TRACE("%p %s %p\n", iface, debugstr_guid(riid), ppvObject);

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

static ULONG WINAPI queryresult_AddRef(
    IXMLDOMNodeList *iface )
{
    queryresult *This = impl_from_IXMLDOMNodeList( iface );
    return InterlockedIncrement( &This->ref );
}

static ULONG WINAPI queryresult_Release(
    IXMLDOMNodeList *iface )
{
    queryresult *This = impl_from_IXMLDOMNodeList( iface );
    ULONG ref;

    ref = InterlockedDecrement(&This->ref);
    if ( ref == 0 )
    {
        xmlXPathFreeObject(This->result);
        xmldoc_release(This->node->doc);
        CoTaskMemFree(This);
    }

    return ref;
}

static HRESULT WINAPI queryresult_GetTypeInfoCount(
    IXMLDOMNodeList *iface,
    UINT* pctinfo )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI queryresult_GetTypeInfo(
    IXMLDOMNodeList *iface,
    UINT iTInfo,
    LCID lcid,
    ITypeInfo** ppTInfo )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI queryresult_GetIDsOfNames(
    IXMLDOMNodeList *iface,
    REFIID riid,
    LPOLESTR* rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID* rgDispId )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI queryresult_Invoke(
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
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI queryresult_get_item(
        IXMLDOMNodeList* iface,
        long index,
        IXMLDOMNode** listItem)
{
    queryresult *This = impl_from_IXMLDOMNodeList( iface );

    TRACE("%p %ld\n", This, index);

    *listItem = NULL;

    if (index < 0 || index >= This->result->nodesetval->nodeNr)
        return S_FALSE;

    *listItem = create_node(This->result->nodesetval->nodeTab[index]);
    This->resultPos = index + 1;

    return S_OK;
}

static HRESULT WINAPI queryresult_get_length(
        IXMLDOMNodeList* iface,
        long* listLength)
{
    queryresult *This = impl_from_IXMLDOMNodeList( iface );

    TRACE("%p\n", This);

    *listLength = This->result->nodesetval->nodeNr;
    return S_OK;
}

static HRESULT WINAPI queryresult_nextNode(
        IXMLDOMNodeList* iface,
        IXMLDOMNode** nextItem)
{
    queryresult *This = impl_from_IXMLDOMNodeList( iface );

    TRACE("%p %p\n", This, nextItem );

    *nextItem = NULL;

    if (This->resultPos >= This->result->nodesetval->nodeNr)
        return S_FALSE;

    *nextItem = create_node(This->result->nodesetval->nodeTab[This->resultPos]);
    This->resultPos++;
    return S_OK;
}

static HRESULT WINAPI queryresult_reset(
        IXMLDOMNodeList* iface)
{
    queryresult *This = impl_from_IXMLDOMNodeList( iface );

    TRACE("%p\n", This);
    This->resultPos = 0;
    return S_OK;
}

static HRESULT WINAPI queryresult__newEnum(
        IXMLDOMNodeList* iface,
        IUnknown** ppUnk)
{
    FIXME("\n");
    return E_NOTIMPL;
}


static const struct IXMLDOMNodeListVtbl queryresult_vtbl =
{
    queryresult_QueryInterface,
    queryresult_AddRef,
    queryresult_Release,
    queryresult_GetTypeInfoCount,
    queryresult_GetTypeInfo,
    queryresult_GetIDsOfNames,
    queryresult_Invoke,
    queryresult_get_item,
    queryresult_get_length,
    queryresult_nextNode,
    queryresult_reset,
    queryresult__newEnum,
};

#endif
