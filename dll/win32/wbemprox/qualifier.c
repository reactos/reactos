/*
 * Copyright 2013 Hans Leidekker for CodeWeavers
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "wbemcli.h"

#include "wine/debug.h"
#include "wbemprox_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(wbemprox);

struct qualifier_set
{
    IWbemQualifierSet IWbemQualifierSet_iface;
    LONG refs;
    enum wbm_namespace ns;
    WCHAR *class;
    WCHAR *member;
};

static inline struct qualifier_set *impl_from_IWbemQualifierSet(
    IWbemQualifierSet *iface )
{
    return CONTAINING_RECORD(iface, struct qualifier_set, IWbemQualifierSet_iface);
}

static ULONG WINAPI qualifier_set_AddRef(
    IWbemQualifierSet *iface )
{
    struct qualifier_set *set = impl_from_IWbemQualifierSet( iface );
    return InterlockedIncrement( &set->refs );
}

static ULONG WINAPI qualifier_set_Release(
    IWbemQualifierSet *iface )
{
    struct qualifier_set *set = impl_from_IWbemQualifierSet( iface );
    LONG refs = InterlockedDecrement( &set->refs );
    if (!refs)
    {
        TRACE("destroying %p\n", set);
        free( set->class );
        free( set->member );
        free( set );
    }
    return refs;
}

static HRESULT WINAPI qualifier_set_QueryInterface(
    IWbemQualifierSet *iface,
    REFIID riid,
    void **ppvObject )
{
    struct qualifier_set *set = impl_from_IWbemQualifierSet( iface );

    TRACE("%p, %s, %p\n", set, debugstr_guid( riid ), ppvObject );

    if ( IsEqualGUID( riid, &IID_IWbemQualifierSet ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = &set->IWbemQualifierSet_iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }
    IWbemQualifierSet_AddRef( iface );
    return S_OK;
}

static HRESULT create_qualifier_enum( enum wbm_namespace ns, const WCHAR *class, const WCHAR *member,
                                      const WCHAR *name, IEnumWbemClassObject **iter )
{
    static const WCHAR fmtW[] = L"SELECT * FROM __QUALIFIERS WHERE Class='%s' AND Member='%s' AND Name='%s'";
    static const WCHAR fmt2W[] = L"SELECT * FROM __QUALIFIERS WHERE Class='%s' AND Member='%s'";
    static const WCHAR fmt3W[] = L"SELECT * FROM __QUALIFIERS WHERE Class='%s'";
    WCHAR *query;
    HRESULT hr;
    int len;

    if (member && name)
    {
        len = lstrlenW( class ) + lstrlenW( member ) + lstrlenW( name ) + ARRAY_SIZE(fmtW);
        if (!(query = malloc( len * sizeof(WCHAR) ))) return E_OUTOFMEMORY;
        swprintf( query, len, fmtW, class, member, name );
    }
    else if (member)
    {
        len = lstrlenW( class ) + lstrlenW( member ) + ARRAY_SIZE(fmt2W);
        if (!(query = malloc( len * sizeof(WCHAR) ))) return E_OUTOFMEMORY;
        swprintf( query, len, fmt2W, class, member );
    }
    else
    {
        len = lstrlenW( class ) + ARRAY_SIZE(fmt3W);
        if (!(query = malloc( len * sizeof(WCHAR) ))) return E_OUTOFMEMORY;
        swprintf( query, len, fmt3W, class );
    }

    hr = exec_query( ns, query, iter );
    free( query );
    return hr;
}

static HRESULT get_qualifier_value( enum wbm_namespace ns, const WCHAR *class, const WCHAR *member, const WCHAR *name,
                                    VARIANT *val, LONG *flavor )
{
    IEnumWbemClassObject *iter;
    IWbemClassObject *obj;
    VARIANT var;
    HRESULT hr;

    hr = create_qualifier_enum( ns, class, member, name, &iter );
    if (FAILED( hr )) return hr;

    hr = create_class_object( ns, NULL, iter, 0, NULL, &obj );
    IEnumWbemClassObject_Release( iter );
    if (FAILED( hr )) return hr;

    if (flavor)
    {
        hr = IWbemClassObject_Get( obj, L"Flavor", 0, &var, NULL, NULL );
        if (hr != S_OK) goto done;
        *flavor = V_I4( &var );
    }
    hr = IWbemClassObject_Get( obj, L"Type", 0, &var, NULL, NULL );
    if (hr != S_OK) goto done;
    switch (V_UI4( &var ))
    {
    case CIM_STRING:
        hr = IWbemClassObject_Get( obj, L"StringValue", 0, val, NULL, NULL );
        break;
    case CIM_SINT32:
        hr = IWbemClassObject_Get( obj, L"IntegerValue", 0, val, NULL, NULL );
        break;
    case CIM_BOOLEAN:
        hr = IWbemClassObject_Get( obj, L"BoolValue", 0, val, NULL, NULL );
        break;
    default:
        ERR( "unhandled type %lu\n", V_UI4( &var ) );
        break;
    }

done:
    IWbemClassObject_Release( obj );
    return hr;
}

static HRESULT WINAPI qualifier_set_Get(
    IWbemQualifierSet *iface,
    LPCWSTR wszName,
    LONG lFlags,
    VARIANT *pVal,
    LONG *plFlavor )
{
    struct qualifier_set *set = impl_from_IWbemQualifierSet( iface );

    TRACE( "%p, %s, %#lx, %p, %p\n", iface, debugstr_w(wszName), lFlags, pVal, plFlavor );
    if (lFlags)
    {
        FIXME( "flags %#lx not supported\n", lFlags );
        return E_NOTIMPL;
    }
    return get_qualifier_value( set->ns, set->class, set->member, wszName, pVal, plFlavor );
}

static HRESULT WINAPI qualifier_set_Put(
    IWbemQualifierSet *iface,
    LPCWSTR wszName,
    VARIANT *pVal,
    LONG lFlavor )
{
    FIXME( "%p, %s, %p, %ld\n", iface, debugstr_w(wszName), pVal, lFlavor );
    return E_NOTIMPL;
}

static HRESULT WINAPI qualifier_set_Delete(
    IWbemQualifierSet *iface,
    LPCWSTR wszName )
{
    FIXME("%p, %s\n", iface, debugstr_w(wszName));
    return E_NOTIMPL;
}

static HRESULT WINAPI qualifier_set_GetNames(
    IWbemQualifierSet *iface,
    LONG lFlags,
    SAFEARRAY **pNames )
{
    struct qualifier_set *set = impl_from_IWbemQualifierSet( iface );
    IEnumWbemClassObject *iter;
    IWbemClassObject *obj;
    HRESULT hr;

    TRACE( "%p, %#lx, %p\n", iface, lFlags, pNames );
    if (lFlags)
    {
        FIXME( "flags %#lx not supported\n", lFlags );
        return E_NOTIMPL;
    }

    hr = create_qualifier_enum( set->ns, set->class, set->member, NULL, &iter );
    if (FAILED( hr )) return hr;

    hr = create_class_object( set->ns, NULL, iter, 0, NULL, &obj );
    IEnumWbemClassObject_Release( iter );
    if (FAILED( hr )) return hr;

    hr = IWbemClassObject_GetNames( obj, NULL, 0, NULL, pNames );
    IWbemClassObject_Release( obj );
    return hr;
}

static HRESULT WINAPI qualifier_set_BeginEnumeration(
    IWbemQualifierSet *iface,
    LONG lFlags )
{
    FIXME( "%p, %#lx\n", iface, lFlags );
    return E_NOTIMPL;
}

static HRESULT WINAPI qualifier_set_Next(
    IWbemQualifierSet *iface,
    LONG lFlags,
    BSTR *pstrName,
    VARIANT *pVal,
    LONG *plFlavor )
{
    FIXME( "%p, %#lx, %p, %p, %p\n", iface, lFlags, pstrName, pVal, plFlavor );
    return E_NOTIMPL;
}

static HRESULT WINAPI qualifier_set_EndEnumeration(
    IWbemQualifierSet *iface )
{
    FIXME("%p\n", iface);
    return E_NOTIMPL;
}

static const IWbemQualifierSetVtbl qualifier_set_vtbl =
{
    qualifier_set_QueryInterface,
    qualifier_set_AddRef,
    qualifier_set_Release,
    qualifier_set_Get,
    qualifier_set_Put,
    qualifier_set_Delete,
    qualifier_set_GetNames,
    qualifier_set_BeginEnumeration,
    qualifier_set_Next,
    qualifier_set_EndEnumeration
};

HRESULT WbemQualifierSet_create( enum wbm_namespace ns, const WCHAR *class, const WCHAR *member, LPVOID *ppObj )
{
    struct qualifier_set *set;

    TRACE("%p\n", ppObj);

    if (!(set = malloc( sizeof(*set) ))) return E_OUTOFMEMORY;

    set->IWbemQualifierSet_iface.lpVtbl = &qualifier_set_vtbl;
    if (!(set->class = wcsdup( class )))
    {
        free( set );
        return E_OUTOFMEMORY;
    }
    if (!member) set->member = NULL;
    else if (!(set->member = wcsdup( member )))
    {
        free( set->class );
        free( set );
        return E_OUTOFMEMORY;
    }
    set->ns = ns;
    set->refs = 1;

    *ppObj = &set->IWbemQualifierSet_iface;

    TRACE("returning iface %p\n", *ppObj);
    return S_OK;
}
