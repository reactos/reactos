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

typedef struct _queryresult
{
    DispatchEx dispex;
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

#define XMLQUERYRES(x)  ((IXMLDOMNodeList*)&(x)->lpVtbl)

static HRESULT WINAPI queryresult_QueryInterface(
    IXMLDOMNodeList *iface,
    REFIID riid,
    void** ppvObject )
{
    queryresult *This = impl_from_IXMLDOMNodeList( iface );

    TRACE("%p %s %p\n", iface, debugstr_guid(riid), ppvObject);

    if(!ppvObject)
        return E_INVALIDARG;

    if ( IsEqualGUID( riid, &IID_IUnknown ) ||
         IsEqualGUID( riid, &IID_IXMLDOMNodeList ) )
    {
        *ppvObject = iface;
    }
    else if(dispex_query_interface(&This->dispex, riid, ppvObject))
    {
        return *ppvObject ? S_OK : E_NOINTERFACE;
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
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI queryresult_GetTypeInfoCount(
    IXMLDOMNodeList *iface,
    UINT* pctinfo )
{
    queryresult *This = impl_from_IXMLDOMNodeList( iface );

    TRACE("(%p)->(%p)\n", This, pctinfo);

    *pctinfo = 1;

    return S_OK;
}

static HRESULT WINAPI queryresult_GetTypeInfo(
    IXMLDOMNodeList *iface,
    UINT iTInfo,
    LCID lcid,
    ITypeInfo** ppTInfo )
{
    queryresult *This = impl_from_IXMLDOMNodeList( iface );
    HRESULT hr;

    TRACE("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);

    hr = get_typeinfo(IXMLDOMNodeList_tid, ppTInfo);

    return hr;
}

static HRESULT WINAPI queryresult_GetIDsOfNames(
    IXMLDOMNodeList *iface,
    REFIID riid,
    LPOLESTR* rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID* rgDispId )
{
    queryresult *This = impl_from_IXMLDOMNodeList( iface );
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
    queryresult *This = impl_from_IXMLDOMNodeList( iface );
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

static HRESULT WINAPI queryresult_get_item(
        IXMLDOMNodeList* iface,
        long index,
        IXMLDOMNode** listItem)
{
    queryresult *This = impl_from_IXMLDOMNodeList( iface );

    TRACE("%p %ld\n", This, index);

    if(!listItem)
        return E_INVALIDARG;

    *listItem = NULL;

    if (index < 0 || index >= xmlXPathNodeSetGetLength(This->result->nodesetval))
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

    if(!listLength)
        return E_INVALIDARG;

    *listLength = xmlXPathNodeSetGetLength(This->result->nodesetval);
    return S_OK;
}

static HRESULT WINAPI queryresult_nextNode(
        IXMLDOMNodeList* iface,
        IXMLDOMNode** nextItem)
{
    queryresult *This = impl_from_IXMLDOMNodeList( iface );

    TRACE("%p %p\n", This, nextItem );

    if(!nextItem)
        return E_INVALIDARG;

    *nextItem = NULL;

    if (This->resultPos >= xmlXPathNodeSetGetLength(This->result->nodesetval))
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

static HRESULT queryresult_get_dispid(IUnknown *iface, BSTR name, DWORD flags, DISPID *dispid)
{
    queryresult *This = impl_from_IXMLDOMNodeList( (IXMLDOMNodeList*)iface );
    WCHAR *ptr;
    DWORD idx=0;

    for(ptr = name; *ptr && isdigitW(*ptr); ptr++)
        idx = idx*10 + (*ptr-'0');
    if(*ptr)
        return DISP_E_UNKNOWNNAME;

    if(idx >= xmlXPathNodeSetGetLength(This->result->nodesetval))
        return DISP_E_UNKNOWNNAME;

    *dispid = MSXML_DISPID_CUSTOM_MIN + idx;
    TRACE("ret %x\n", *dispid);
    return S_OK;
}

static HRESULT queryresult_invoke(IUnknown *iface, DISPID id, LCID lcid, WORD flags, DISPPARAMS *params,
        VARIANT *res, EXCEPINFO *ei)
{
    queryresult *This = impl_from_IXMLDOMNodeList( (IXMLDOMNodeList*)iface );

    TRACE("(%p)->(%x %x %x %p %p %p)\n", This, id, lcid, flags, params, res, ei);

    V_VT(res) = VT_DISPATCH;
    V_DISPATCH(res) = NULL;

    switch(flags)
    {
        case INVOKE_PROPERTYGET:
        {
            IXMLDOMNode *disp = NULL;

            queryresult_get_item(XMLQUERYRES(This), id - MSXML_DISPID_CUSTOM_MIN, &disp);
            V_DISPATCH(res) = (IDispatch*)&disp;
            break;
        }
        default:
        {
            FIXME("unimplemented flags %x\n", flags);
            break;
        }
    }

    return S_OK;
}

static const dispex_static_data_vtbl_t queryresult_dispex_vtbl = {
    queryresult_get_dispid,
    queryresult_invoke
};

static const tid_t queryresult_iface_tids[] = {
    IXMLDOMNodeList_tid,
    0
};
static dispex_static_data_t queryresult_dispex = {
    &queryresult_dispex_vtbl,
    IXMLDOMNodeList_tid,
    NULL,
    queryresult_iface_tids
};

HRESULT queryresult_create(xmlNodePtr node, LPWSTR szQuery, IXMLDOMNodeList **out)
{
    queryresult *This = heap_alloc_zero(sizeof(queryresult));
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

    init_dispex(&This->dispex, (IUnknown*)&This->lpVtbl, &queryresult_dispex);

    *out = (IXMLDOMNodeList *) &This->lpVtbl;
    hr = S_OK;
    TRACE("found %d matches\n", xmlXPathNodeSetGetLength(This->result->nodesetval));

cleanup:
    if (This != NULL && FAILED(hr))
        IXMLDOMNodeList_Release( (IXMLDOMNodeList*) &This->lpVtbl );
    if (ctxt != NULL)
        xmlXPathFreeContext(ctxt);
    HeapFree(GetProcessHeap(), 0, str);
    return hr;
}

#endif
