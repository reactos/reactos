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

#include "wbemprox_private.h"

struct enum_class_object
{
    IEnumWbemClassObject IEnumWbemClassObject_iface;
    LONG refs;
    struct query *query;
    UINT index;
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
        heap_free( ec );
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
        *ppvObject = ec;
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
    HRESULT hr;

    TRACE("%p, %d, %u, %p, %p\n", iface, lTimeout, uCount, apObjects, puReturned);

    if (!uCount) return WBEM_S_FALSE;
    if (!apObjects || !puReturned) return WBEM_E_INVALID_PARAMETER;
    if (lTimeout != WBEM_INFINITE)
    {
        static int once;
        if (!once++) FIXME("timeout not supported\n");
    }

    *puReturned = 0;
    if (ec->index >= view->count) return WBEM_S_FALSE;

    hr = create_class_object( view->table->name, iface, ec->index, NULL, apObjects );
    if (hr != S_OK) return hr;

    ec->index++;
    *puReturned = 1;
    if (ec->index == view->count && uCount > 1) return WBEM_S_FALSE;
    if (uCount > 1) return WBEM_S_TIMEDOUT;
    return WBEM_S_NO_ERROR;
}

static HRESULT WINAPI enum_class_object_NextAsync(
    IEnumWbemClassObject *iface,
    ULONG uCount,
    IWbemObjectSink *pSink )
{
    FIXME("%p, %u, %p\n", iface, uCount, pSink);
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

    TRACE("%p, %d, %u\n", iface, lTimeout, nCount);

    if (lTimeout != WBEM_INFINITE) FIXME("timeout not supported\n");

    if (!view->count) return WBEM_S_FALSE;

    if (nCount > view->count - ec->index)
    {
        ec->index = view->count - 1;
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

    ec = heap_alloc( sizeof(*ec) );
    if (!ec) return E_OUTOFMEMORY;

    ec->IEnumWbemClassObject_iface.lpVtbl = &enum_class_object_vtbl;
    ec->refs  = 1;
    ec->query = addref_query( query );
    ec->index = 0;

    *ppObj = &ec->IEnumWbemClassObject_iface;

    TRACE("returning iface %p\n", *ppObj);
    return S_OK;
}

static struct record *create_record( struct table *table )
{
    UINT i;
    struct record *record;

    if (!(record = heap_alloc( sizeof(struct record) ))) return NULL;
    if (!(record->fields = heap_alloc( table->num_cols * sizeof(struct field) )))
    {
        heap_free( record );
        return NULL;
    }
    for (i = 0; i < table->num_cols; i++)
    {
        record->fields[i].type    = table->columns[i].type;
        record->fields[i].vartype = table->columns[i].vartype;
        record->fields[i].u.ival  = 0;
    }
    record->count = table->num_cols;
    record->table = addref_table( table );
    return record;
}

void destroy_array( struct array *array, CIMTYPE type )
{
    UINT i, size;

    if (!array) return;
    if (type == CIM_STRING || type == CIM_DATETIME)
    {
        size = get_type_size( type );
        for (i = 0; i < array->count; i++) heap_free( *(WCHAR **)((char *)array->ptr + i * size) );
    }
    heap_free( array->ptr );
    heap_free( array );
}

static void destroy_record( struct record *record )
{
    UINT i;

    if (!record) return;
    release_table( record->table );
    for (i = 0; i < record->count; i++)
    {
        if (record->fields[i].type == CIM_STRING || record->fields[i].type == CIM_DATETIME)
            heap_free( record->fields[i].u.sval );
        else if (record->fields[i].type & CIM_FLAG_ARRAY)
            destroy_array( record->fields[i].u.aval, record->fields[i].type & CIM_TYPE_MASK );
    }
    heap_free( record->fields );
    heap_free( record );
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
        heap_free( co->name );
        heap_free( co );
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
        *ppvObject = co;
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
    FIXME("%p, %p\n", iface, ppQualSet);
    return E_NOTIMPL;
}

static HRESULT record_get_value( const struct record *record, UINT index, VARIANT *var, CIMTYPE *type )
{
    VARTYPE vartype = record->fields[index].vartype;

    if (type) *type = record->fields[index].type;

    if (record->fields[index].type & CIM_FLAG_ARRAY)
    {
        V_VT( var ) = vartype ? vartype : to_vartype( record->fields[index].type & CIM_TYPE_MASK ) | VT_ARRAY;
        V_ARRAY( var ) = to_safearray( record->fields[index].u.aval, record->fields[index].type & CIM_TYPE_MASK );
        return S_OK;
    }
    switch (record->fields[index].type)
    {
    case CIM_STRING:
    case CIM_DATETIME:
        if (!vartype) vartype = VT_BSTR;
        V_BSTR( var ) = SysAllocString( record->fields[index].u.sval );
        break;
    case CIM_SINT32:
        if (!vartype) vartype = VT_I4;
        V_I4( var ) = record->fields[index].u.ival;
        break;
    case CIM_UINT32:
        if (!vartype) vartype = VT_UI4;
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

    TRACE("%p, %s, %08x, %p, %p, %p\n", iface, debugstr_w(wszName), lFlags, pVal, pType, plFlavor);

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
        record->fields[index].u.sval = (WCHAR *)(INT_PTR)val;
        return S_OK;
    case CIM_SINT16:
    case CIM_UINT16:
    case CIM_SINT32:
    case CIM_UINT32:
        record->fields[index].u.ival = val;
        return S_OK;
    default:
        FIXME("unhandled type %u\n", type);
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

    TRACE("%p, %s, %08x, %p, %u\n", iface, debugstr_w(wszName), lFlags, pVal, Type);

    if (co->record)
    {
        UINT index;
        HRESULT hr;

        if ((hr = get_column_index( co->record->table, wszName, &index )) != S_OK) return hr;
        return record_set_value( co->record, index, pVal );
    }
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

    TRACE("%p, %s, %08x, %s, %p\n", iface, debugstr_w(wszQualifierName), lFlags,
          debugstr_variant(pQualifierVal), pNames);

    if (lFlags != WBEM_FLAG_ALWAYS &&
        lFlags != WBEM_FLAG_NONSYSTEM_ONLY &&
        lFlags != WBEM_FLAG_SYSTEM_ONLY)
    {
        FIXME("flags %08x not supported\n", lFlags);
        return E_NOTIMPL;
    }
    if (wszQualifierName || pQualifierVal)
        FIXME("qualifier not supported\n");

    return get_properties( ec->query->view, lFlags, pNames );
}

static HRESULT WINAPI class_object_BeginEnumeration(
    IWbemClassObject *iface,
    LONG lEnumFlags )
{
    struct class_object *co = impl_from_IWbemClassObject( iface );

    TRACE("%p, %08x\n", iface, lEnumFlags);

    if (lEnumFlags) FIXME("flags 0x%08x not supported\n", lEnumFlags);

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
    BSTR prop;
    HRESULT hr;
    UINT i;

    TRACE("%p, %08x, %p, %p, %p, %p\n", iface, lFlags, strName, pVal, pType, plFlavor);

    for (i = obj->index_property; i < view->table->num_cols; i++)
    {
        if (is_method( view->table, i )) continue;
        if (!is_selected_prop( view, view->table->columns[i].name )) continue;
        if (!(prop = SysAllocString( view->table->columns[i].name ))) return E_OUTOFMEMORY;
        if ((hr = get_propval( view, obj->index, prop, pVal, pType, plFlavor )) != S_OK)
        {
            SysFreeString( prop );
            return hr;
        }
        obj->index_property = i + 1;
        *strName = prop;
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

    return WbemQualifierSet_create( co->name, wszProperty, (void **)ppQualSet );
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
    static const WCHAR fmtW[] = {'\n','\t','%','s',' ','=',' ','%','s',';',0};
    BSTR value, ret;
    WCHAR *p;
    UINT i;

    *len = 0;
    for (i = 0; i < table->num_cols; i++)
    {
        if ((value = get_value_bstr( table, row, i )))
        {
            *len += sizeof(fmtW) / sizeof(fmtW[0]);
            *len += strlenW( table->columns[i].name );
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
            p += sprintfW( p, fmtW, table->columns[i].name, value );
            SysFreeString( value );
        }
    }
    return ret;
}

static BSTR get_object_text( const struct view *view, UINT index )
{
    static const WCHAR fmtW[] =
        {'\n','i','n','s','t','a','n','c','e',' ','o','f',' ','%','s','\n','{','%','s','\n','}',';',0};
    UINT len, len_body, row = view->result[index];
    BSTR ret, body;

    len = sizeof(fmtW) / sizeof(fmtW[0]);
    len += strlenW( view->table->name );
    if (!(body = get_body_text( view->table, row, &len_body ))) return NULL;
    len += len_body;

    if (!(ret = SysAllocStringLen( NULL, len ))) return NULL;
    sprintfW( ret, fmtW, view->table->name, body );
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

    TRACE("%p, %08x, %p\n", iface, lFlags, pstrObjectText);

    if (lFlags) FIXME("flags %08x not implemented\n", lFlags);

    if (!(text = get_object_text( view, co->index ))) return E_OUTOFMEMORY;
    *pstrObjectText = text;
    return S_OK;
}

static HRESULT WINAPI class_object_SpawnDerivedClass(
    IWbemClassObject *iface,
    LONG lFlags,
    IWbemClassObject **ppNewClass )
{
    FIXME("%p, %08x, %p\n", iface, lFlags, ppNewClass);
    return E_NOTIMPL;
}

static HRESULT WINAPI class_object_SpawnInstance(
    IWbemClassObject *iface,
    LONG lFlags,
    IWbemClassObject **ppNewInstance )
{
    struct class_object *co = impl_from_IWbemClassObject( iface );
    struct enum_class_object *ec = impl_from_IEnumWbemClassObject( co->iter );
    struct view *view = ec->query->view;
    struct record *record;

    TRACE("%p, %08x, %p\n", iface, lFlags, ppNewInstance);

    if (!(record = create_record( view->table ))) return E_OUTOFMEMORY;

    return create_class_object( co->name, NULL, 0, record, ppNewInstance );
}

static HRESULT WINAPI class_object_CompareTo(
    IWbemClassObject *iface,
    LONG lFlags,
    IWbemClassObject *pCompareTo )
{
    FIXME("%p, %08x, %p\n", iface, lFlags, pCompareTo);
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
        FIXME("unhandled type %u\n", type);
        break;
    }
}

static HRESULT create_signature_columns_and_data( IEnumWbemClassObject *iter, UINT *num_cols,
                                           struct column **cols, BYTE **data )
{
    static const WCHAR parameterW[] = {'P','a','r','a','m','e','t','e','r',0};
    static const WCHAR typeW[] = {'T','y','p','e',0};
    static const WCHAR varianttypeW[] = {'V','a','r','i','a','n','t','T','y','p','e',0};
    static const WCHAR defaultvalueW[] = {'D','e','f','a','u','l','t','V','a','l','u','e',0};
    struct column *columns;
    BYTE *row;
    IWbemClassObject *param;
    VARIANT val;
    HRESULT hr = E_OUTOFMEMORY;
    UINT offset = 0;
    ULONG count;
    int i = 0;

    count = count_instances( iter );
    if (!(columns = heap_alloc( count * sizeof(struct column) ))) return E_OUTOFMEMORY;
    if (!(row = heap_alloc_zero( count * sizeof(LONGLONG) ))) goto error;

    for (;;)
    {
        IEnumWbemClassObject_Next( iter, WBEM_INFINITE, 1, &param, &count );
        if (!count) break;

        hr = IWbemClassObject_Get( param, parameterW, 0, &val, NULL, NULL );
        if (hr != S_OK) goto error;
        columns[i].name = heap_strdupW( V_BSTR( &val ) );
        VariantClear( &val );

        hr = IWbemClassObject_Get( param, typeW, 0, &val, NULL, NULL );
        if (hr != S_OK) goto error;
        columns[i].type = V_UI4( &val );

        hr = IWbemClassObject_Get( param, varianttypeW, 0, &val, NULL, NULL );
        if (hr != S_OK) goto error;
        columns[i].vartype = V_UI4( &val );

        hr = IWbemClassObject_Get( param, defaultvalueW, 0, &val, NULL, NULL );
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
    for (; i >= 0; i--) heap_free( (WCHAR *)columns[i].name );
    heap_free( columns );
    heap_free( row );
    return hr;
}

static HRESULT create_signature_table( IEnumWbemClassObject *iter, WCHAR *name )
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
        heap_free( row );
        return E_OUTOFMEMORY;
    }
    if (!add_table( table )) free_table( table ); /* already exists */
    return S_OK;
}

static WCHAR *build_signature_table_name( const WCHAR *class, const WCHAR *method, enum param_direction dir )
{
    static const WCHAR fmtW[] = {'_','_','%','s','_','%','s','_','%','s',0};
    static const WCHAR outW[] = {'O','U','T',0};
    static const WCHAR inW[] = {'I','N',0};
    UINT len = SIZEOF(fmtW) + SIZEOF(outW) + strlenW( class ) + strlenW( method );
    WCHAR *ret;

    if (!(ret = heap_alloc( len * sizeof(WCHAR) ))) return NULL;
    sprintfW( ret, fmtW, class, method, dir == PARAM_IN ? inW : outW );
    return struprW( ret );
}

HRESULT create_signature( const WCHAR *class, const WCHAR *method, enum param_direction dir,
                          IWbemClassObject **sig )
{
    static const WCHAR selectW[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
         '_','_','P','A','R','A','M','E','T','E','R','S',' ','W','H','E','R','E',' ',
         'C','l','a','s','s','=','\'','%','s','\'',' ','A','N','D',' ',
         'M','e','t','h','o','d','=','\'','%','s','\'',' ','A','N','D',' ',
         'D','i','r','e','c','t','i','o','n','%','s',0};
    static const WCHAR geW[] = {'>','=','0',0};
    static const WCHAR leW[] = {'<','=','0',0};
    UINT len = SIZEOF(selectW) + SIZEOF(geW);
    IEnumWbemClassObject *iter;
    WCHAR *query, *name;
    HRESULT hr;

    len += strlenW( class ) + strlenW( method );
    if (!(query = heap_alloc( len * sizeof(WCHAR) ))) return E_OUTOFMEMORY;
    sprintfW( query, selectW, class, method, dir >= 0 ? geW : leW );

    hr = exec_query( query, &iter );
    heap_free( query );
    if (hr != S_OK) return hr;

    if (!(name = build_signature_table_name( class, method, dir )))
    {
        IEnumWbemClassObject_Release( iter );
        return E_OUTOFMEMORY;
    }
    hr = create_signature_table( iter, name );
    IEnumWbemClassObject_Release( iter );
    if (hr == S_OK)
        hr = get_object( name, sig );

    heap_free( name );
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
    HRESULT hr;

    TRACE("%p, %s, %08x, %p, %p\n", iface, debugstr_w(wszName), lFlags, ppInSignature, ppOutSignature);

    hr = create_signature( co->name, wszName, PARAM_IN, &in );
    if (hr != S_OK) return hr;

    hr = create_signature( co->name, wszName, PARAM_OUT, &out );
    if (hr == S_OK)
    {
        if (ppInSignature) *ppInSignature = in;
        else IWbemClassObject_Release( in );
        if (ppOutSignature) *ppOutSignature = out;
        else IWbemClassObject_Release( out );
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
    FIXME("%p, %s, %08x, %p, %p\n", iface, debugstr_w(wszName), lFlags, pInSignature, pOutSignature);
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

    TRACE("%p, %08x\n", iface, lEnumFlags);

    if (lEnumFlags) FIXME("flags 0x%08x not supported\n", lEnumFlags);

    if (co->iter)
    {
        WARN("not allowed on instance\n");
        return WBEM_E_ILLEGAL_OPERATION;
    }
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

    TRACE("%p, %08x, %p, %p, %p\n", iface, lFlags, pstrName, ppInSignature, ppOutSignature);

    if (!(method = get_method_name( co->name, co->index_method ))) return WBEM_S_NO_MORE_DATA;

    hr = create_signature( co->name, method, PARAM_IN, ppInSignature );
    if (hr != S_OK)
    {
        SysFreeString( method );
        return hr;
    }
    hr = create_signature( co->name, method, PARAM_OUT, ppOutSignature );
    if (hr != S_OK)
    {
        SysFreeString( method );
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
    FIXME("%p, %s, %p\n", iface, debugstr_w(wszMethod), ppQualSet);
    return E_NOTIMPL;
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

HRESULT create_class_object( const WCHAR *name, IEnumWbemClassObject *iter, UINT index,
                             struct record *record, IWbemClassObject **obj )
{
    struct class_object *co;

    TRACE("%s, %p\n", debugstr_w(name), obj);

    co = heap_alloc( sizeof(*co) );
    if (!co) return E_OUTOFMEMORY;

    co->IWbemClassObject_iface.lpVtbl = &class_object_vtbl;
    co->refs  = 1;
    if (!name) co->name = NULL;
    else if (!(co->name = heap_strdupW( name )))
    {
        heap_free( co );
        return E_OUTOFMEMORY;
    }
    co->iter           = iter;
    co->index          = index;
    co->index_method   = 0;
    co->index_property = 0;
    co->record         = record;
    if (iter) IEnumWbemClassObject_AddRef( iter );

    *obj = &co->IWbemClassObject_iface;

    TRACE("returning iface %p\n", *obj);
    return S_OK;
}
