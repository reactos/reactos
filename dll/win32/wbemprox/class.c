/*
 * Copyright 2012 Hans Leidekker for CodeWeavers
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

struct enum_class_object
{
    IEnumWbemClassObject IEnumWbemClassObject_iface;
    LONG refs;
    struct query *query;
    UINT index;
    enum wbm_namespace ns;
};

static inline struct enum_class_object *impl_from_IEnumWbemClassObject(
    IEnumWbemClassObject *iface )
{
    return CONTAINING_RECORD(iface, struct enum_class_object, IEnumWbemClassObject_iface);
}

static ULONG WINAPI enum_class_object_AddRef(
    IEnumWbemClassObject *iface )
{
    struct enum_class_object *ec = impl_from_IEnumWbemClassObject( iface );
    return InterlockedIncrement( &ec->refs );
}

static ULONG WINAPI enum_class_object_Release(
    IEnumWbemClassObject *iface )
{
    struct enum_class_object *ec = impl_from_IEnumWbemClassObject( iface );
    LONG refs = InterlockedDecrement( &ec->refs );
    if (!refs)
    {
        TRACE("destroying %p\n", ec);
        release_query( ec->query );
        free( ec );
    }
    return refs;
}

static HRESULT WINAPI enum_class_object_QueryInterface(
    IEnumWbemClassObject *iface,
    REFIID riid,
    void **ppvObject )
{
    struct enum_class_object *ec = impl_from_IEnumWbemClassObject( iface );

    TRACE("%p, %s, %p\n", ec, debugstr_guid( riid ), ppvObject );

    if ( IsEqualGUID( riid, &IID_IEnumWbemClassObject ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = &ec->IEnumWbemClassObject_iface;
    }
    else if ( IsEqualGUID( riid, &IID_IClientSecurity ) )
    {
        *ppvObject = &client_security;
        return S_OK;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }
    IEnumWbemClassObject_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI enum_class_object_Reset(
    IEnumWbemClassObject *iface )
{
    struct enum_class_object *ec = impl_from_IEnumWbemClassObject( iface );

    TRACE("%p\n", iface);

    ec->index = 0;
    return WBEM_S_NO_ERROR;
}

static HRESULT WINAPI enum_class_object_Next(
    IEnumWbemClassObject *iface,
    LONG lTimeout,
    ULONG uCount,
    IWbemClassObject **apObjects,
    ULONG *puReturned )
{
    struct enum_class_object *ec = impl_from_IEnumWbemClassObject( iface );
    struct view *view = ec->query->view;
    struct table *table;
    static int once = 0;
    HRESULT hr;
    ULONG i, j;

    TRACE( "%p, %ld, %lu, %p, %p\n", iface, lTimeout, uCount, apObjects, puReturned );

    if (!apObjects || !puReturned) return WBEM_E_INVALID_PARAMETER;
    if (lTimeout != WBEM_INFINITE && !once++) FIXME("timeout not supported\n");

    *puReturned = 0;

    for (i = 0; i < uCount; i++)
    {
        if (ec->index >= view->result_count) return WBEM_S_FALSE;
        table = get_view_table( view, ec->index );
        hr = create_class_object( ec->ns, table->name, iface, ec->index, NULL, &apObjects[i] );
        if (hr != S_OK)
        {
            for (j = 0; j < i; j++) IWbemClassObject_Release( apObjects[j] );
            return hr;
        }
        ec->index++;
        (*puReturned)++;
    }

    return WBEM_S_NO_ERROR;
}

static HRESULT WINAPI enum_class_object_NextAsync(
    IEnumWbemClassObject *iface,
    ULONG uCount,
    IWbemObjectSink *pSink )
{
    FIXME( "%p, %lu, %p\n", iface, uCount, pSink );
    return E_NOTIMPL;
}

static HRESULT WINAPI enum_class_object_Clone(
    IEnumWbemClassObject *iface,
    IEnumWbemClassObject **ppEnum )
{
    struct enum_class_object *ec = impl_from_IEnumWbemClassObject( iface );

    TRACE("%p, %p\n", iface, ppEnum);

    return EnumWbemClassObject_create( ec->query, (void **)ppEnum );
}

static HRESULT WINAPI enum_class_object_Skip(
    IEnumWbemClassObject *iface,
    LONG lTimeout,
    ULONG nCount )
{
    struct enum_class_object *ec = impl_from_IEnumWbemClassObject( iface );
    struct view *view = ec->query->view;
    static int once = 0;

    TRACE( "%p, %ld, %lu\n", iface, lTimeout, nCount );

    if (lTimeout != WBEM_INFINITE && !once++) FIXME("timeout not supported\n");

    if (!view->result_count) return WBEM_S_FALSE;

    if (nCount > view->result_count - ec->index)
    {
        ec->index = view->result_count - 1;
        return WBEM_S_FALSE;
    }
    ec->index += nCount;
    return WBEM_S_NO_ERROR;
}

static const IEnumWbemClassObjectVtbl enum_class_object_vtbl =
{
    enum_class_object_QueryInterface,
    enum_class_object_AddRef,
    enum_class_object_Release,
    enum_class_object_Reset,
    enum_class_object_Next,
    enum_class_object_NextAsync,
    enum_class_object_Clone,
    enum_class_object_Skip
};

HRESULT EnumWbemClassObject_create( struct query *query, LPVOID *ppObj )
{
    struct enum_class_object *ec;

    TRACE("%p\n", ppObj);

    if (!(ec = malloc( sizeof(*ec) ))) return E_OUTOFMEMORY;

    ec->IEnumWbemClassObject_iface.lpVtbl = &enum_class_object_vtbl;
    ec->refs  = 1;
    ec->query = addref_query( query );
    ec->index = 0;
    ec->ns = query->ns;

    *ppObj = &ec->IEnumWbemClassObject_iface;

    TRACE("returning iface %p\n", *ppObj);
    return S_OK;
}

static struct record *create_record( struct table *table )
{
    UINT i;
    struct record *record;

    if (!(record = malloc( sizeof(struct record) ))) return NULL;
    if (!(record->fields = malloc( table->num_cols * sizeof(struct field) )))
    {
        free( record );
        return NULL;
    }
    for (i = 0; i < table->num_cols; i++)
    {
        record->fields[i].type    = table->columns[i].type;
        record->fields[i].u.ival  = 0;
    }
    record->count = table->num_cols;
    record->table = grab_table( table );
    return record;
}

void destroy_array( struct array *array, CIMTYPE type )
{
    UINT i;
    if (!array) return;
    if (type == CIM_STRING || type == CIM_DATETIME || type == CIM_REFERENCE)
    {
        for (i = 0; i < array->count; i++) free( *(WCHAR **)((char *)array->ptr + i * array->elem_size) );
    }
    free( array->ptr );
    free( array );
}

static void destroy_record( struct record *record )
{
    UINT i;

    if (!record) return;
    release_table( record->table );
    for (i = 0; i < record->count; i++)
    {
        if (record->fields[i].type == CIM_STRING ||
            record->fields[i].type == CIM_DATETIME ||
            record->fields[i].type == CIM_REFERENCE) free( record->fields[i].u.sval );
        else if (record->fields[i].type & CIM_FLAG_ARRAY)
            destroy_array( record->fields[i].u.aval, record->fields[i].type & CIM_TYPE_MASK );
    }
    free( record->fields );
    free( record );
}

struct class_object
{
    IWbemClassObject IWbemClassObject_iface;
    LONG refs;
    WCHAR *name;
    IEnumWbemClassObject *iter;
    UINT index;
    UINT index_method;
    UINT index_property;
    enum wbm_namespace ns;
    struct record *record; /* uncommitted instance */
};

static inline struct class_object *impl_from_IWbemClassObject(
    IWbemClassObject *iface )
{
    return CONTAINING_RECORD(iface, struct class_object, IWbemClassObject_iface);
}

static ULONG WINAPI class_object_AddRef(
    IWbemClassObject *iface )
{
    struct class_object *co = impl_from_IWbemClassObject( iface );
    return InterlockedIncrement( &co->refs );
}

static ULONG WINAPI class_object_Release(
    IWbemClassObject *iface )
{
    struct class_object *co = impl_from_IWbemClassObject( iface );
    LONG refs = InterlockedDecrement( &co->refs );
    if (!refs)
    {
        TRACE("destroying %p\n", co);
        if (co->iter) IEnumWbemClassObject_Release( co->iter );
        destroy_record( co->record );
        free( co->name );
        free( co );
    }
    return refs;
}

static HRESULT WINAPI class_object_QueryInterface(
    IWbemClassObject *iface,
    REFIID riid,
    void **ppvObject )
{
    struct class_object *co = impl_from_IWbemClassObject( iface );

    TRACE("%p, %s, %p\n", co, debugstr_guid( riid ), ppvObject );

    if ( IsEqualGUID( riid, &IID_IWbemClassObject ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = &co->IWbemClassObject_iface;
    }
    else if (IsEqualGUID( riid, &IID_IClientSecurity ))
    {
        *ppvObject = &client_security;
        return S_OK;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }
    IWbemClassObject_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI class_object_GetQualifierSet(
    IWbemClassObject *iface,
    IWbemQualifierSet **ppQualSet )
{
    struct class_object *co = impl_from_IWbemClassObject( iface );

    TRACE("%p, %p\n", iface, ppQualSet);

    return WbemQualifierSet_create( co->ns, co->name, NULL, (void **)ppQualSet );
}

static HRESULT record_get_value( const struct record *record, UINT index, VARIANT *var, CIMTYPE *type )
{
    VARTYPE vartype = to_vartype( record->fields[index].type & CIM_TYPE_MASK );

    if (type) *type = record->fields[index].type;
    if (!var) return S_OK;

    if (record->fields[index].type & CIM_FLAG_ARRAY)
    {
        V_VT( var ) = vartype | VT_ARRAY;
        V_ARRAY( var ) = to_safearray( record->fields[index].u.aval, record->fields[index].type & CIM_TYPE_MASK );
        return S_OK;
    }
    switch (record->fields[index].type)
    {
    case CIM_STRING:
    case CIM_DATETIME:
    case CIM_REFERENCE:
        V_BSTR( var ) = SysAllocString( record->fields[index].u.sval );
        break;
    case CIM_SINT32:
        V_I4( var ) = record->fields[index].u.ival;
        break;
    case CIM_UINT32:
        V_UI4( var ) = record->fields[index].u.ival;
        break;
    default:
        FIXME("unhandled type %u\n", record->fields[index].type);
        return WBEM_E_INVALID_PARAMETER;
    }
    V_VT( var ) = vartype;
    return S_OK;
}

static HRESULT WINAPI class_object_Get(
    IWbemClassObject *iface,
    LPCWSTR wszName,
    LONG lFlags,
    VARIANT *pVal,
    CIMTYPE *pType,
    LONG *plFlavor )
{
    struct class_object *co = impl_from_IWbemClassObject( iface );
    struct enum_class_object *ec = impl_from_IEnumWbemClassObject( co->iter );

    TRACE( "%p, %s, %#lx, %p, %p, %p\n", iface, debugstr_w(wszName), lFlags, pVal, pType, plFlavor );

    if (co->record)
    {
        UINT index;
        HRESULT hr;

        if ((hr = get_column_index( co->record->table, wszName, &index )) != S_OK) return hr;
        return record_get_value( co->record, index, pVal, pType );
    }
    return get_propval( ec->query->view, co->index, wszName, pVal, pType, plFlavor );
}

static HRESULT record_set_value( struct record *record, UINT index, VARIANT *var )
{
    LONGLONG val;
    CIMTYPE type;
    HRESULT hr;

    if ((hr = to_longlong( var, &val, &type )) != S_OK) return hr;
    if (type != record->fields[index].type) return WBEM_E_TYPE_MISMATCH;

    if (type & CIM_FLAG_ARRAY)
    {
        record->fields[index].u.aval = (struct array *)(INT_PTR)val;
        return S_OK;
    }
    switch (type)
    {
    case CIM_STRING:
    case CIM_DATETIME:
    case CIM_REFERENCE:
        record->fields[index].u.sval = (WCHAR *)(INT_PTR)val;
        return S_OK;
    case CIM_SINT16:
    case CIM_UINT16:
    case CIM_SINT32:
    case CIM_UINT32:
        record->fields[index].u.ival = val;
        return S_OK;
    default:
        FIXME( "unhandled type %lu\n", type );
        break;
    }
    return WBEM_E_INVALID_PARAMETER;
}

static HRESULT WINAPI class_object_Put(
    IWbemClassObject *iface,
    LPCWSTR wszName,
    LONG lFlags,
    VARIANT *pVal,
    CIMTYPE Type )
{
    struct class_object *co = impl_from_IWbemClassObject( iface );
    struct enum_class_object *ec = impl_from_IEnumWbemClassObject( co->iter );

    TRACE( "%p, %s, %#lx, %p, %lu\n", iface, debugstr_w(wszName), lFlags, pVal, Type );

    if (co->record)
    {
        UINT index;
        HRESULT hr;

        if ((hr = get_column_index( co->record->table, wszName, &index )) != S_OK) return hr;
        return record_set_value( co->record, index, pVal );
    }

    if (!ec) return S_OK;

    return put_propval( ec->query->view, co->index, wszName, pVal, Type );
}

static HRESULT WINAPI class_object_Delete(
    IWbemClassObject *iface,
    LPCWSTR wszName )
{
    FIXME("%p, %s\n", iface, debugstr_w(wszName));
    return E_NOTIMPL;
}

static HRESULT WINAPI class_object_GetNames(
    IWbemClassObject *iface,
    LPCWSTR wszQualifierName,
    LONG lFlags,
    VARIANT *pQualifierVal,
    SAFEARRAY **pNames )
{
    struct class_object *co = impl_from_IWbemClassObject( iface );
    struct enum_class_object *ec = impl_from_IEnumWbemClassObject( co->iter );

    TRACE( "%p, %s, %#lx, %s, %p\n", iface, debugstr_w(wszQualifierName), lFlags,
          debugstr_variant(pQualifierVal), pNames);

    if (!pNames)
        return WBEM_E_INVALID_PARAMETER;

    /* Combination used in a handful of broken apps */
    if (lFlags == (WBEM_FLAG_ALWAYS | WBEM_MASK_CONDITION_ORIGIN))
        lFlags = WBEM_FLAG_ALWAYS;

    if (lFlags && (lFlags != WBEM_FLAG_ALWAYS &&
        lFlags != WBEM_FLAG_NONSYSTEM_ONLY &&
        lFlags != WBEM_FLAG_SYSTEM_ONLY))
    {
        FIXME( "flags %#lx not supported\n", lFlags );
        return E_NOTIMPL;
    }

    if (wszQualifierName || pQualifierVal)
        FIXME("qualifier not supported\n");

    return get_properties( ec->query->view, co->index, lFlags, pNames );
}

static HRESULT WINAPI class_object_BeginEnumeration(
    IWbemClassObject *iface,
    LONG lEnumFlags )
{
    struct class_object *co = impl_from_IWbemClassObject( iface );

    TRACE( "%p, %#lx\n", iface, lEnumFlags );

    if (lEnumFlags) FIXME( "flags %#lx not supported\n", lEnumFlags );

    co->index_property = 0;
    return S_OK;
}

static HRESULT WINAPI class_object_Next(
    IWbemClassObject *iface,
    LONG lFlags,
    BSTR *strName,
    VARIANT *pVal,
    CIMTYPE *pType,
    LONG *plFlavor )
{
    struct class_object *obj = impl_from_IWbemClassObject( iface );
    struct enum_class_object *iter = impl_from_IEnumWbemClassObject( obj->iter );
    struct view *view = iter->query->view;
    struct table *table = get_view_table( view, obj->index );
    BSTR prop;
    HRESULT hr;
    UINT i;

    TRACE( "%p, %#lx, %p, %p, %p, %p\n", iface, lFlags, strName, pVal, pType, plFlavor );

    for (i = obj->index_property; i < table->num_cols; i++)
    {
        if (is_method( table, i )) continue;
        if (!is_result_prop( view, table->columns[i].name )) continue;
        if (!(prop = SysAllocString( table->columns[i].name ))) return E_OUTOFMEMORY;
        if (obj->record)
        {
            UINT index;

            if ((hr = get_column_index( table, table->columns[i].name, &index )) == S_OK)
                hr = record_get_value( obj->record, index, pVal, pType );
        }
        else
            hr = get_propval( view, obj->index, prop, pVal, pType, plFlavor );

        if (FAILED(hr))
        {
            SysFreeString( prop );
            return hr;
        }

        obj->index_property = i + 1;
        if (strName) *strName = prop;
        else SysFreeString( prop );

        return S_OK;
    }
    return WBEM_S_NO_MORE_DATA;
}

static HRESULT WINAPI class_object_EndEnumeration(
    IWbemClassObject *iface )
{
    struct class_object *co = impl_from_IWbemClassObject( iface );

    TRACE("%p\n", iface);

    co->index_property = 0;
    return S_OK;
}

static HRESULT WINAPI class_object_GetPropertyQualifierSet(
    IWbemClassObject *iface,
    LPCWSTR wszProperty,
    IWbemQualifierSet **ppQualSet )
{
    struct class_object *co = impl_from_IWbemClassObject( iface );

    TRACE("%p, %s, %p\n", iface, debugstr_w(wszProperty), ppQualSet);

    return WbemQualifierSet_create( co->ns, co->name, wszProperty, (void **)ppQualSet );
}

static HRESULT WINAPI class_object_Clone(
    IWbemClassObject *iface,
    IWbemClassObject **ppCopy )
{
    FIXME("%p, %p\n", iface, ppCopy);
    return E_NOTIMPL;
}

static BSTR get_body_text( const struct table *table, UINT row, UINT *len )
{
    BSTR value, ret;
    WCHAR *p;
    UINT i;

    *len = 0;
    for (i = 0; i < table->num_cols; i++)
    {
        if ((value = get_value_bstr( table, row, i )))
        {
            *len += ARRAY_SIZE( L"\n\t%s = %s;" );
            *len += lstrlenW( table->columns[i].name );
            *len += SysStringLen( value );
            SysFreeString( value );
        }
    }
    if (!(ret = SysAllocStringLen( NULL, *len ))) return NULL;
    p = ret;
    for (i = 0; i < table->num_cols; i++)
    {
        if ((value = get_value_bstr( table, row, i )))
        {
            p += swprintf( p, *len - (p - ret), L"\n\t%s = %s;", table->columns[i].name, value );
            SysFreeString( value );
        }
    }
    return ret;
}

static BSTR get_object_text( const struct view *view, UINT index )
{
    UINT len, len_body, row = view->result[index];
    struct table *table = get_view_table( view, index );
    BSTR ret, body;

    len = ARRAY_SIZE( L"\ninstance of %s\n{%s\n};" );
    len += lstrlenW( table->name );
    if (!(body = get_body_text( table, row, &len_body ))) return NULL;
    len += len_body;

    if (!(ret = SysAllocStringLen( NULL, len ))) return NULL;
    swprintf( ret, len, L"\ninstance of %s\n{%s\n};", table->name, body );
    SysFreeString( body );
    return ret;
}

static HRESULT WINAPI class_object_GetObjectText(
    IWbemClassObject *iface,
    LONG lFlags,
    BSTR *pstrObjectText )
{
    struct class_object *co = impl_from_IWbemClassObject( iface );
    struct enum_class_object *ec = impl_from_IEnumWbemClassObject( co->iter );
    struct view *view = ec->query->view;
    BSTR text;

    TRACE( "%p, %#lx, %p\n", iface, lFlags, pstrObjectText );

    if (lFlags) FIXME( "flags %#lx not implemented\n", lFlags );

    if (!(text = get_object_text( view, co->index ))) return E_OUTOFMEMORY;
    *pstrObjectText = text;
    return S_OK;
}

static HRESULT WINAPI class_object_SpawnDerivedClass(
    IWbemClassObject *iface,
    LONG lFlags,
    IWbemClassObject **ppNewClass )
{
    FIXME( "%p, %#lx, %p\n", iface, lFlags, ppNewClass );
    return E_NOTIMPL;
}

static HRESULT WINAPI class_object_SpawnInstance(
    IWbemClassObject *iface,
    LONG lFlags,
    IWbemClassObject **ppNewInstance )
{
    struct class_object *co = impl_from_IWbemClassObject( iface );
    struct enum_class_object *ec = impl_from_IEnumWbemClassObject( co->iter );
    struct table *table = get_view_table( ec->query->view, co->index );
    IEnumWbemClassObject *iter;
    struct record *record;
    HRESULT hr;

    TRACE( "%p, %#lx, %p\n", iface, lFlags, ppNewInstance );

    if (!(record = create_record( table ))) return E_OUTOFMEMORY;
    if (FAILED(hr = IEnumWbemClassObject_Clone( co->iter, &iter )))
    {
        destroy_record( record );
        return hr;
    }
    hr = create_class_object( co->ns, co->name, iter, 0, record, ppNewInstance );
    IEnumWbemClassObject_Release( iter );
    return hr;
}

static HRESULT WINAPI class_object_CompareTo(
    IWbemClassObject *iface,
    LONG lFlags,
    IWbemClassObject *pCompareTo )
{
    FIXME( "%p, %#lx, %p\n", iface, lFlags, pCompareTo );
    return E_NOTIMPL;
}

static HRESULT WINAPI class_object_GetPropertyOrigin(
    IWbemClassObject *iface,
    LPCWSTR wszName,
    BSTR *pstrClassName )
{
    FIXME("%p, %s, %p\n", iface, debugstr_w(wszName), pstrClassName);
    return E_NOTIMPL;
}

static HRESULT WINAPI class_object_InheritsFrom(
    IWbemClassObject *iface,
    LPCWSTR strAncestor )
{
    FIXME("%p, %s\n", iface, debugstr_w(strAncestor));
    return E_NOTIMPL;
}

static UINT count_instances( IEnumWbemClassObject *iter )
{
    UINT count = 0;
    while (!IEnumWbemClassObject_Skip( iter, WBEM_INFINITE, 1 )) count++;
    IEnumWbemClassObject_Reset( iter );
    return count;
}

static void set_default_value( CIMTYPE type, UINT val, BYTE *ptr )
{
    switch (type)
    {
    case CIM_SINT16:
        *(INT16 *)ptr = val;
        break;
    case CIM_UINT16:
        *(UINT16 *)ptr = val;
        break;
    case CIM_SINT32:
        *(INT32 *)ptr = val;
        break;
    case CIM_UINT32:
        *(UINT32 *)ptr = val;
        break;
    default:
        FIXME( "unhandled type %lu\n", type );
        break;
    }
}

static HRESULT create_signature_columns_and_data( IEnumWbemClassObject *iter, UINT *num_cols,
                                           struct column **cols, BYTE **data )
{
    struct column *columns;
    BYTE *row;
    IWbemClassObject *param;
    VARIANT val;
    HRESULT hr = E_OUTOFMEMORY;
    UINT offset = 0;
    ULONG count;
    int i = 0;

    count = count_instances( iter );
    if (!(columns = malloc( count * sizeof(struct column) ))) return E_OUTOFMEMORY;
    if (!(row = calloc( count, sizeof(LONGLONG) ))) goto error;

    for (;;)
    {
        IEnumWbemClassObject_Next( iter, WBEM_INFINITE, 1, &param, &count );
        if (!count) break;

        hr = IWbemClassObject_Get( param, L"Parameter", 0, &val, NULL, NULL );
        if (hr != S_OK) goto error;
        columns[i].name = wcsdup( V_BSTR( &val ) );
        VariantClear( &val );

        hr = IWbemClassObject_Get( param, L"Type", 0, &val, NULL, NULL );
        if (hr != S_OK) goto error;
        columns[i].type = V_UI4( &val );

        hr = IWbemClassObject_Get( param, L"DefaultValue", 0, &val, NULL, NULL );
        if (hr != S_OK) goto error;
        if (V_UI4( &val )) set_default_value( columns[i].type, V_UI4( &val ), row + offset );
        offset += get_type_size( columns[i].type );

        IWbemClassObject_Release( param );
        i++;
    }
    *num_cols = i;
    *cols = columns;
    *data = row;
    return S_OK;

error:
    for (; i >= 0; i--) free( (WCHAR *)columns[i].name );
    free( columns );
    free( row );
    return hr;
}

static HRESULT create_signature_table( IEnumWbemClassObject *iter, enum wbm_namespace ns, WCHAR *name )
{
    HRESULT hr;
    struct table *table;
    struct column *columns;
    UINT num_cols;
    BYTE *row;

    hr = create_signature_columns_and_data( iter, &num_cols, &columns, &row );
    if (hr != S_OK) return hr;

    if (!(table = create_table( name, num_cols, columns, 1, 1, row, NULL )))
    {
        free_columns( columns, num_cols );
        free( row );
        return E_OUTOFMEMORY;
    }
    if (!add_table( ns, table )) free_table( table ); /* already exists */
    return S_OK;
}

static WCHAR *build_signature_table_name( const WCHAR *class, const WCHAR *method, enum param_direction dir )
{
    UINT len = ARRAY_SIZE(L"__%s_%s_%s") + ARRAY_SIZE(L"OUT") + lstrlenW( class ) + lstrlenW( method );
    WCHAR *ret;

    if (!(ret = malloc( len * sizeof(WCHAR) ))) return NULL;
    swprintf( ret, len, L"__%s_%s_%s", class, method, dir == PARAM_IN ? L"IN" : L"OUT" );
    return wcsupr( ret );
}

HRESULT create_signature( enum wbm_namespace ns, const WCHAR *class, const WCHAR *method, enum param_direction dir,
                          IWbemClassObject **sig )
{
    static const WCHAR selectW[] = L"SELECT * FROM __PARAMETERS WHERE Class='%s' AND Method='%s' AND Direction%s";
    UINT len = ARRAY_SIZE(selectW) + ARRAY_SIZE(L">=0");
    IEnumWbemClassObject *iter;
    WCHAR *query, *name;
    HRESULT hr;

    len += lstrlenW( class ) + lstrlenW( method );
    if (!(query = malloc( len * sizeof(WCHAR) ))) return E_OUTOFMEMORY;
    swprintf( query, len, selectW, class, method, dir >= 0 ? L">=0" : L"<=0" );

    hr = exec_query( ns, query, &iter );
    free( query );
    if (hr != S_OK) return hr;

    if (!count_instances( iter ))
    {
        *sig = NULL;
        IEnumWbemClassObject_Release( iter );
        return S_OK;
    }

    if (!(name = build_signature_table_name( class, method, dir )))
    {
        IEnumWbemClassObject_Release( iter );
        return E_OUTOFMEMORY;
    }
    hr = create_signature_table( iter, ns, name );
    IEnumWbemClassObject_Release( iter );
    if (hr == S_OK)
        hr = get_object( ns, name, sig );

    free( name );
    return hr;
}

static HRESULT WINAPI class_object_GetMethod(
    IWbemClassObject *iface,
    LPCWSTR wszName,
    LONG lFlags,
    IWbemClassObject **ppInSignature,
    IWbemClassObject **ppOutSignature )
{
    struct class_object *co = impl_from_IWbemClassObject( iface );
    IWbemClassObject *in, *out;
    struct table *table;
    unsigned int i;
    HRESULT hr;

    TRACE( "%p, %s, %#lx, %p, %p\n", iface, debugstr_w(wszName), lFlags, ppInSignature, ppOutSignature );

    if (ppInSignature) *ppInSignature = NULL;
    if (ppOutSignature) *ppOutSignature = NULL;

    table = get_view_table( impl_from_IEnumWbemClassObject( co->iter )->query->view, co->index );

    for (i = 0; i < table->num_cols; ++i)
    {
        if (is_method( table, i ) && !lstrcmpiW( table->columns[i].name, wszName )) break;
    }
    if (i == table->num_cols)
    {
        FIXME("Method %s not found in class %s.\n", debugstr_w(wszName), debugstr_w(co->name));
        return WBEM_E_NOT_FOUND;
    }

    hr = create_signature( co->ns, co->name, wszName, PARAM_IN, &in );
    if (hr != S_OK) return hr;

    hr = create_signature( co->ns, co->name, wszName, PARAM_OUT, &out );
    if (hr == S_OK)
    {
        if (ppInSignature) *ppInSignature = in;
        else if (in) IWbemClassObject_Release( in );
        if (ppOutSignature) *ppOutSignature = out;
        else if (out) IWbemClassObject_Release( out );
    }
    else IWbemClassObject_Release( in );
    return hr;
}

static HRESULT WINAPI class_object_PutMethod(
    IWbemClassObject *iface,
    LPCWSTR wszName,
    LONG lFlags,
    IWbemClassObject *pInSignature,
    IWbemClassObject *pOutSignature )
{
    FIXME( "%p, %s, %#lx, %p, %p\n", iface, debugstr_w(wszName), lFlags, pInSignature, pOutSignature );
    return E_NOTIMPL;
}

static HRESULT WINAPI class_object_DeleteMethod(
    IWbemClassObject *iface,
    LPCWSTR wszName )
{
    FIXME("%p, %s\n", iface, debugstr_w(wszName));
    return E_NOTIMPL;
}

static HRESULT WINAPI class_object_BeginMethodEnumeration(
    IWbemClassObject *iface,
    LONG lEnumFlags)
{
    struct class_object *co = impl_from_IWbemClassObject( iface );

    TRACE( "%p, %#lx\n", iface, lEnumFlags );

    if (lEnumFlags) FIXME( "flags %#lx not supported\n", lEnumFlags );

    co->index_method = 0;
    return S_OK;
}

static HRESULT WINAPI class_object_NextMethod(
    IWbemClassObject *iface,
    LONG lFlags,
    BSTR *pstrName,
    IWbemClassObject **ppInSignature,
    IWbemClassObject **ppOutSignature)
{
    struct class_object *co = impl_from_IWbemClassObject( iface );
    BSTR method;
    HRESULT hr;

    TRACE( "%p, %#lx, %p, %p, %p\n", iface, lFlags, pstrName, ppInSignature, ppOutSignature );

    if (!(method = get_method_name( co->ns, co->name, co->index_method ))) return WBEM_S_NO_MORE_DATA;

    hr = create_signature( co->ns, co->name, method, PARAM_IN, ppInSignature );
    if (hr != S_OK)
    {
        SysFreeString( method );
        return hr;
    }
    hr = create_signature( co->ns, co->name, method, PARAM_OUT, ppOutSignature );
    if (hr != S_OK)
    {
        SysFreeString( method );
        if (*ppInSignature)
            IWbemClassObject_Release( *ppInSignature );
    }
    else
    {
        *pstrName = method;
        co->index_method++;
    }
    return hr;
}

static HRESULT WINAPI class_object_EndMethodEnumeration(
    IWbemClassObject *iface )
{
    struct class_object *co = impl_from_IWbemClassObject( iface );

    TRACE("%p\n", iface);

    co->index_method = 0;
    return S_OK;
}

static HRESULT WINAPI class_object_GetMethodQualifierSet(
    IWbemClassObject *iface,
    LPCWSTR wszMethod,
    IWbemQualifierSet **ppQualSet)
{
    struct class_object *co = impl_from_IWbemClassObject( iface );

    TRACE("%p, %s, %p\n", iface, debugstr_w(wszMethod), ppQualSet);

    return WbemQualifierSet_create( co->ns, co->name, wszMethod, (void **)ppQualSet );
}

static HRESULT WINAPI class_object_GetMethodOrigin(
    IWbemClassObject *iface,
    LPCWSTR wszMethodName,
    BSTR *pstrClassName)
{
    FIXME("%p, %s, %p\n", iface, debugstr_w(wszMethodName), pstrClassName);
    return E_NOTIMPL;
}

static const IWbemClassObjectVtbl class_object_vtbl =
{
    class_object_QueryInterface,
    class_object_AddRef,
    class_object_Release,
    class_object_GetQualifierSet,
    class_object_Get,
    class_object_Put,
    class_object_Delete,
    class_object_GetNames,
    class_object_BeginEnumeration,
    class_object_Next,
    class_object_EndEnumeration,
    class_object_GetPropertyQualifierSet,
    class_object_Clone,
    class_object_GetObjectText,
    class_object_SpawnDerivedClass,
    class_object_SpawnInstance,
    class_object_CompareTo,
    class_object_GetPropertyOrigin,
    class_object_InheritsFrom,
    class_object_GetMethod,
    class_object_PutMethod,
    class_object_DeleteMethod,
    class_object_BeginMethodEnumeration,
    class_object_NextMethod,
    class_object_EndMethodEnumeration,
    class_object_GetMethodQualifierSet,
    class_object_GetMethodOrigin
};

HRESULT create_class_object( enum wbm_namespace ns, const WCHAR *name, IEnumWbemClassObject *iter, UINT index,
                             struct record *record, IWbemClassObject **obj )
{
    struct class_object *co;

    TRACE("%s, %p\n", debugstr_w(name), obj);

    if (!(co = malloc( sizeof(*co) ))) return E_OUTOFMEMORY;

    co->IWbemClassObject_iface.lpVtbl = &class_object_vtbl;
    co->refs  = 1;
    if (!name) co->name = NULL;
    else if (!(co->name = wcsdup( name )))
    {
        free( co );
        return E_OUTOFMEMORY;
    }
    co->iter           = iter;
    co->index          = index;
    co->index_method   = 0;
    co->index_property = 0;
    co->record         = record;
    co->ns             = ns;
    if (iter) IEnumWbemClassObject_AddRef( iter );

    *obj = &co->IWbemClassObject_iface;

    TRACE("returning iface %p\n", *obj);
    return S_OK;
}
