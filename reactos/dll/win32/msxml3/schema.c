/*
 * Schema cache implementation
 *
 * Copyright 2007 Huw Davies
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

#include "wine/debug.h"

#include "msxml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);


typedef struct
{
    const struct IXMLDOMSchemaCollectionVtbl *lpVtbl;
    LONG ref;
} schema_t;

static inline schema_t *impl_from_IXMLDOMSchemaCollection( IXMLDOMSchemaCollection *iface )
{
    return (schema_t *)((char*)iface - FIELD_OFFSET(schema_t, lpVtbl));
}

static HRESULT WINAPI schema_cache_QueryInterface( IXMLDOMSchemaCollection *iface, REFIID riid, void** ppvObject )
{
    schema_t *This = impl_from_IXMLDOMSchemaCollection( iface );

    TRACE("%p %s %p\n", This, debugstr_guid( riid ), ppvObject );

    if ( IsEqualIID( riid, &IID_IUnknown ) ||
         IsEqualIID( riid, &IID_IDispatch ) ||
         IsEqualIID( riid, &IID_IXMLDOMSchemaCollection ) )
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IXMLDOMSchemaCollection_AddRef( iface );

    return S_OK;
}

static ULONG WINAPI schema_cache_AddRef( IXMLDOMSchemaCollection *iface )
{
    schema_t *This = impl_from_IXMLDOMSchemaCollection( iface );
    LONG ref = InterlockedIncrement( &This->ref );
    TRACE("%p new ref %d\n", This, ref);
    return ref;
}

static ULONG WINAPI schema_cache_Release( IXMLDOMSchemaCollection *iface )
{
    schema_t *This = impl_from_IXMLDOMSchemaCollection( iface );
    LONG ref = InterlockedDecrement( &This->ref );
    TRACE("%p new ref %d\n", This, ref);

    if ( ref == 0 )
    {
        HeapFree( GetProcessHeap(), 0, This );
    }

    return ref;
}

static HRESULT WINAPI schema_cache_GetTypeInfoCount( IXMLDOMSchemaCollection *iface, UINT* pctinfo )
{
    schema_t *This = impl_from_IXMLDOMSchemaCollection( iface );

    TRACE("(%p)->(%p)\n", This, pctinfo);

    *pctinfo = 1;

    return S_OK;
}

static HRESULT WINAPI schema_cache_GetTypeInfo( IXMLDOMSchemaCollection *iface,
                                                UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo )
{
    schema_t *This = impl_from_IXMLDOMSchemaCollection( iface );
    HRESULT hr;

    TRACE("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);

    hr = get_typeinfo(IXMLDOMSchemaCollection_tid, ppTInfo);

    return hr;
}

static HRESULT WINAPI schema_cache_GetIDsOfNames( IXMLDOMSchemaCollection *iface,
                                                  REFIID riid,
                                                  LPOLESTR* rgszNames,
                                                  UINT cNames,
                                                  LCID lcid,
                                                  DISPID* rgDispId )
{
    schema_t *This = impl_from_IXMLDOMSchemaCollection( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);

    if(!rgszNames || cNames == 0 || !rgDispId)
        return E_INVALIDARG;

    hr = get_typeinfo(IXMLDOMSchemaCollection_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI schema_cache_Invoke( IXMLDOMSchemaCollection *iface,
                                           DISPID dispIdMember,
                                           REFIID riid,
                                           LCID lcid,
                                           WORD wFlags,
                                           DISPPARAMS* pDispParams,
                                           VARIANT* pVarResult,
                                           EXCEPINFO* pExcepInfo,
                                           UINT* puArgErr )
{
    schema_t *This = impl_from_IXMLDOMSchemaCollection( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IXMLDOMSchemaCollection_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, &(This->lpVtbl), dispIdMember, wFlags, pDispParams,
                pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI schema_cache_add( IXMLDOMSchemaCollection *iface, BSTR uri, VARIANT var )
{
    FIXME("(%p)->(%s, var(vt %x)): stub\n", iface, debugstr_w(uri), V_VT(&var));
    return S_OK;
}

static HRESULT WINAPI schema_cache_get( IXMLDOMSchemaCollection *iface, BSTR uri, IXMLDOMNode **node )
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI schema_cache_remove( IXMLDOMSchemaCollection *iface, BSTR uri )
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI schema_cache_get_length( IXMLDOMSchemaCollection *iface, LONG *length )
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI schema_cache_get_namespaceURI( IXMLDOMSchemaCollection *iface, LONG index, BSTR *len )
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI schema_cache_addCollection( IXMLDOMSchemaCollection *iface, IXMLDOMSchemaCollection *otherCollection )
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI schema_cache_get__newEnum( IXMLDOMSchemaCollection *iface, IUnknown **ppUnk )
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static const struct IXMLDOMSchemaCollectionVtbl schema_vtbl =
{
    schema_cache_QueryInterface,
    schema_cache_AddRef,
    schema_cache_Release,
    schema_cache_GetTypeInfoCount,
    schema_cache_GetTypeInfo,
    schema_cache_GetIDsOfNames,
    schema_cache_Invoke,
    schema_cache_add,
    schema_cache_get,
    schema_cache_remove,
    schema_cache_get_length,
    schema_cache_get_namespaceURI,
    schema_cache_addCollection,
    schema_cache_get__newEnum
};

HRESULT SchemaCache_create(IUnknown *pUnkOuter, LPVOID *ppObj)
{
    schema_t *schema = HeapAlloc( GetProcessHeap(), 0, sizeof (*schema) );
    if( !schema )
        return E_OUTOFMEMORY;

    schema->lpVtbl = &schema_vtbl;
    schema->ref = 1;

    *ppObj = &schema->lpVtbl;
    return S_OK;
}
