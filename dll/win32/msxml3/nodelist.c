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
#ifdef HAVE_LIBXML2
# include <libxml/parser.h>
# include <libxml/xmlerror.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "msxml6.h"
#include "msxml2did.h"

#include "msxml_private.h"

#include "wine/debug.h"

/* This file implements the object returned by childNodes property. Note that this is
 * not the IXMLDOMNodeList returned by XPath queries - it's implemented in selection.c.
 * They are different because the list returned by childNodes:
 *  - is "live" - changes to the XML tree are automatically reflected in the list
 *  - doesn't supports IXMLDOMSelection
 *  - note that an attribute node have a text child in DOM but not in the XPath data model
 *    thus the child is inaccessible by an XPath query
 */

#ifdef HAVE_LIBXML2

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

typedef struct
{
    DispatchEx dispex;
    IXMLDOMNodeList IXMLDOMNodeList_iface;
    LONG ref;
    xmlNodePtr parent;
    xmlNodePtr current;
    IEnumVARIANT *enumvariant;
} xmlnodelist;

static HRESULT nodelist_get_item(IUnknown *iface, LONG index, VARIANT *item)
{
    V_VT(item) = VT_DISPATCH;
    return IXMLDOMNodeList_get_item((IXMLDOMNodeList*)iface, index, (IXMLDOMNode**)&V_DISPATCH(item));
}

static const struct enumvariant_funcs nodelist_enumvariant = {
    nodelist_get_item,
    NULL
};

static inline xmlnodelist *impl_from_IXMLDOMNodeList( IXMLDOMNodeList *iface )
{
    return CONTAINING_RECORD(iface, xmlnodelist, IXMLDOMNodeList_iface);
}

static HRESULT WINAPI xmlnodelist_QueryInterface(
    IXMLDOMNodeList *iface,
    REFIID riid,
    void** ppvObject )
{
    xmlnodelist *This = impl_from_IXMLDOMNodeList( iface );

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppvObject);

#ifdef __REACTOS__
    if (!ppvObject)
    {
        /* NOTE: Interface documentation for IUnknown explicitly states
         * this case should return E_POINTER. Empirical data proves
         * MS violates this contract and instead return E_INVALIDARG.
         */
        return E_INVALIDARG;
    }
#endif

    if ( IsEqualGUID( riid, &IID_IUnknown ) ||
         IsEqualGUID( riid, &IID_IDispatch ) ||
         IsEqualGUID( riid, &IID_IXMLDOMNodeList ) )
    {
        *ppvObject = iface;
    }
    else if (IsEqualGUID( riid, &IID_IEnumVARIANT ))
    {
        if (!This->enumvariant)
        {
            HRESULT hr = create_enumvariant((IUnknown*)iface, FALSE, &nodelist_enumvariant, &This->enumvariant);
            if (FAILED(hr)) return hr;
        }

        return IEnumVARIANT_QueryInterface(This->enumvariant, &IID_IEnumVARIANT, ppvObject);
    }
    else if (dispex_query_interface(&This->dispex, riid, ppvObject))
    {
        return *ppvObject ? S_OK : E_NOINTERFACE;
    }
    else
    {
        TRACE("interface %s not implemented\n", debugstr_guid(riid));
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
    ULONG ref = InterlockedIncrement( &This->ref );
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI xmlnodelist_Release(
    IXMLDOMNodeList *iface )
{
    xmlnodelist *This = impl_from_IXMLDOMNodeList( iface );
    ULONG ref = InterlockedDecrement( &This->ref );

    TRACE("(%p)->(%d)\n", This, ref);
    if ( ref == 0 )
    {
        xmldoc_release( This->parent->doc );
        if (This->enumvariant) IEnumVARIANT_Release(This->enumvariant);
        heap_free( This );
    }

    return ref;
}

static HRESULT WINAPI xmlnodelist_GetTypeInfoCount(
    IXMLDOMNodeList *iface,
    UINT* pctinfo )
{
    xmlnodelist *This = impl_from_IXMLDOMNodeList( iface );
    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI xmlnodelist_GetTypeInfo(
    IXMLDOMNodeList *iface,
    UINT iTInfo,
    LCID lcid,
    ITypeInfo** ppTInfo )
{
    xmlnodelist *This = impl_from_IXMLDOMNodeList( iface );
    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface,
        iTInfo, lcid, ppTInfo);
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
    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface,
        riid, rgszNames, cNames, lcid, rgDispId);
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
    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface,
        dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI xmlnodelist_get_item(
        IXMLDOMNodeList* iface,
        LONG index,
        IXMLDOMNode** listItem)
{
    xmlnodelist *This = impl_from_IXMLDOMNodeList( iface );
    xmlNodePtr curr;
    LONG nodeIndex = 0;

    TRACE("(%p)->(%d %p)\n", This, index, listItem);

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
        LONG* listLength)
{

    xmlNodePtr curr;
    LONG nodeCount = 0;

    xmlnodelist *This = impl_from_IXMLDOMNodeList( iface );

    TRACE("(%p)->(%p)\n", This, listLength);

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

    TRACE("(%p)->(%p)\n", This, nextItem );

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
        IUnknown** enumv)
{
    xmlnodelist *This = impl_from_IXMLDOMNodeList( iface );
    TRACE("(%p)->(%p)\n", This, enumv);
    return create_enumvariant((IUnknown*)iface, TRUE, &nodelist_enumvariant, (IEnumVARIANT**)enumv);
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

static HRESULT xmlnodelist_get_dispid(IUnknown *iface, BSTR name, DWORD flags, DISPID *dispid)
{
    WCHAR *ptr;
    int idx = 0;

    for(ptr = name; *ptr && isdigitW(*ptr); ptr++)
        idx = idx*10 + (*ptr-'0');
    if(*ptr)
        return DISP_E_UNKNOWNNAME;

    *dispid = DISPID_DOM_COLLECTION_BASE + idx;
    TRACE("ret %x\n", *dispid);
    return S_OK;
}

static HRESULT xmlnodelist_invoke(IUnknown *iface, DISPID id, LCID lcid, WORD flags, DISPPARAMS *params,
        VARIANT *res, EXCEPINFO *ei)
{
    xmlnodelist *This = impl_from_IXMLDOMNodeList( (IXMLDOMNodeList*)iface );

    TRACE("(%p)->(%x %x %x %p %p %p)\n", This, id, lcid, flags, params, res, ei);

    if (id >= DISPID_DOM_COLLECTION_BASE && id <= DISPID_DOM_COLLECTION_MAX)
    {
        switch(flags)
        {
            case DISPATCH_PROPERTYGET:
            {
                IXMLDOMNode *disp = NULL;

                V_VT(res) = VT_DISPATCH;
                IXMLDOMNodeList_get_item(&This->IXMLDOMNodeList_iface, id - DISPID_DOM_COLLECTION_BASE, &disp);
                V_DISPATCH(res) = (IDispatch*)disp;
                break;
            }
            default:
            {
                FIXME("unimplemented flags %x\n", flags);
                break;
            }
        }
    }
    else if (id == DISPID_VALUE)
    {
        switch(flags)
        {
            case DISPATCH_METHOD|DISPATCH_PROPERTYGET:
            case DISPATCH_PROPERTYGET:
            case DISPATCH_METHOD:
            {
                IXMLDOMNode *item;
                VARIANT index;
                HRESULT hr;

                if (params->cArgs - params->cNamedArgs != 1) return DISP_E_BADPARAMCOUNT;

                VariantInit(&index);
                hr = VariantChangeType(&index, params->rgvarg, 0, VT_I4);
                if(FAILED(hr))
                {
                    FIXME("failed to convert arg, %s\n", debugstr_variant(params->rgvarg));
                    return hr;
                }

                IXMLDOMNodeList_get_item(&This->IXMLDOMNodeList_iface, V_I4(&index), &item);
                V_VT(res) = VT_DISPATCH;
                V_DISPATCH(res) = (IDispatch*)item;
                break;
            }
            default:
            {
                FIXME("DISPID_VALUE: unimplemented flags %x\n", flags);
                break;
            }
        }
    }
    else
        return DISP_E_UNKNOWNNAME;

    TRACE("ret %p\n", V_DISPATCH(res));

    return S_OK;
}

static const dispex_static_data_vtbl_t xmlnodelist_dispex_vtbl = {
    xmlnodelist_get_dispid,
    xmlnodelist_invoke
};

static const tid_t xmlnodelist_iface_tids[] = {
    IXMLDOMNodeList_tid,
    0
};
static dispex_static_data_t xmlnodelist_dispex = {
    &xmlnodelist_dispex_vtbl,
    IXMLDOMNodeList_tid,
    NULL,
    xmlnodelist_iface_tids
};

IXMLDOMNodeList* create_children_nodelist( xmlNodePtr node )
{
    xmlnodelist *This;

    This = heap_alloc( sizeof *This );
    if ( !This )
        return NULL;

    This->IXMLDOMNodeList_iface.lpVtbl = &xmlnodelist_vtbl;
    This->ref = 1;
    This->parent = node;
    This->current = node->children;
    This->enumvariant = NULL;
    xmldoc_add_ref( node->doc );

    init_dispex(&This->dispex, (IUnknown*)&This->IXMLDOMNodeList_iface, &xmlnodelist_dispex);

    return &This->IXMLDOMNodeList_iface;
}

#endif
