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

#include <assert.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wbemcli.h"

#include "wine/debug.h"
#include "wbemprox_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(wbemprox);

static CRITICAL_SECTION table_list_cs;
static CRITICAL_SECTION_DEBUG table_debug =
{
    0, 0, &table_list_cs,
    { &table_debug.ProcessLocksList, &table_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": table_list_cs") }
};
static CRITICAL_SECTION table_list_cs = { &table_debug, -1, 0, 0, 0, 0 };

HRESULT get_column_index( const struct table *table, const WCHAR *name, UINT *column )
{
    UINT i;
    for (i = 0; i < table->num_cols; i++)
    {
        if (!wcsicmp( table->columns[i].name, name ))
        {
            *column = i;
            return S_OK;
        }
    }
    return WBEM_E_INVALID_QUERY;
}

UINT get_type_size( CIMTYPE type )
{
    if (type & CIM_FLAG_ARRAY) return sizeof(void *);

    switch (type)
    {
    case CIM_BOOLEAN:
        return sizeof(int);
    case CIM_SINT8:
    case CIM_UINT8:
        return sizeof(INT8);
    case CIM_SINT16:
    case CIM_UINT16:
        return sizeof(INT16);
    case CIM_SINT32:
    case CIM_UINT32:
        return sizeof(INT32);
    case CIM_SINT64:
    case CIM_UINT64:
        return sizeof(INT64);
    case CIM_DATETIME:
    case CIM_REFERENCE:
    case CIM_STRING:
        return sizeof(WCHAR *);
    case CIM_REAL32:
        return sizeof(FLOAT);
    default:
        ERR( "unhandled type %lu\n", type );
        break;
    }
    return sizeof(LONGLONG);
}

static UINT get_column_size( const struct table *table, UINT column )
{
    return get_type_size( table->columns[column].type & COL_TYPE_MASK );
}

static UINT get_column_offset( const struct table *table, UINT column )
{
    UINT i, offset = 0;
    for (i = 0; i < column; i++) offset += get_column_size( table, i );
    return offset;
}

static UINT get_row_size( const struct table *table )
{
    return get_column_offset( table, table->num_cols - 1 ) + get_column_size( table, table->num_cols - 1 );
}

HRESULT get_value( const struct table *table, UINT row, UINT column, LONGLONG *val )
{
    UINT col_offset, row_size;
    const BYTE *ptr;

    col_offset = get_column_offset( table, column );
    row_size = get_row_size( table );
    ptr = table->data + row * row_size + col_offset;

    if (table->columns[column].type & CIM_FLAG_ARRAY)
    {
        *val = (INT_PTR)*(const void **)ptr;
        return S_OK;
    }
    switch (table->columns[column].type & COL_TYPE_MASK)
    {
    case CIM_BOOLEAN:
        *val = *(const int *)ptr;
        break;
    case CIM_DATETIME:
    case CIM_REFERENCE:
    case CIM_STRING:
        *val = (INT_PTR)*(const WCHAR **)ptr;
        break;
    case CIM_SINT8:
        *val = *(const INT8 *)ptr;
        break;
    case CIM_UINT8:
        *val = *(const UINT8 *)ptr;
        break;
    case CIM_SINT16:
        *val = *(const INT16 *)ptr;
        break;
    case CIM_UINT16:
        *val = *(const UINT16 *)ptr;
        break;
    case CIM_SINT32:
        *val = *(const INT32 *)ptr;
        break;
    case CIM_UINT32:
        *val = *(const UINT32 *)ptr;
        break;
    case CIM_SINT64:
        *val = *(const INT64 *)ptr;
        break;
    case CIM_UINT64:
        *val = *(const UINT64 *)ptr;
        break;
    case CIM_REAL32:
        memcpy( val, ptr, sizeof(FLOAT) );
        break;
    default:
        ERR("invalid column type %u\n", table->columns[column].type & COL_TYPE_MASK);
        *val = 0;
        break;
    }
    return S_OK;
}

BSTR get_value_bstr( const struct table *table, UINT row, UINT column )
{
    LONGLONG val;
    BSTR ret;
    WCHAR number[22];
    UINT len;

    if (table->columns[column].type & CIM_FLAG_ARRAY)
    {
        FIXME("array to string conversion not handled\n");
        return NULL;
    }
    if (get_value( table, row, column, &val ) != S_OK) return NULL;

    switch (table->columns[column].type & COL_TYPE_MASK)
    {
    case CIM_BOOLEAN:
        if (val) return SysAllocString( L"TRUE" );
        else return SysAllocString( L"FALSE" );

    case CIM_DATETIME:
    case CIM_REFERENCE:
    case CIM_STRING:
        if (!val) return NULL;
        len = lstrlenW( (const WCHAR *)(INT_PTR)val ) + 2;
        if (!(ret = SysAllocStringLen( NULL, len ))) return NULL;
        swprintf( ret, len + 1, L"\"%s\"", (const WCHAR *)(INT_PTR)val );
        return ret;

    case CIM_SINT16:
    case CIM_SINT32:
#ifdef __REACTOS__
        swprintf( number, ARRAY_SIZE( number ), L"%lld", val );
#else
        swprintf( number, ARRAY_SIZE( number ), L"%d", val );
#endif
        return SysAllocString( number );

    case CIM_UINT16:
    case CIM_UINT32:
#ifdef __REACTOS__
        swprintf( number, ARRAY_SIZE( number ), L"%llu", val );
#else
        swprintf( number, ARRAY_SIZE( number ), L"%u", val );
#endif
        return SysAllocString( number );

    case CIM_SINT64:
        wsprintfW( number, L"%I64d", val );
        return SysAllocString( number );

    case CIM_UINT64:
        wsprintfW( number, L"%I64u", val );
        return SysAllocString( number );

    default:
        FIXME("unhandled column type %u\n", table->columns[column].type & COL_TYPE_MASK);
        break;
    }
    return NULL;
}

HRESULT set_value( const struct table *table, UINT row, UINT column, LONGLONG val,
                   CIMTYPE type )
{
    UINT col_offset, row_size;
    BYTE *ptr;

    if ((table->columns[column].type & COL_TYPE_MASK) != type) return WBEM_E_TYPE_MISMATCH;

    col_offset = get_column_offset( table, column );
    row_size = get_row_size( table );
    ptr = table->data + row * row_size + col_offset;

    switch (table->columns[column].type & COL_TYPE_MASK)
    {
    case CIM_DATETIME:
    case CIM_REFERENCE:
    case CIM_STRING:
        *(WCHAR **)ptr = (WCHAR *)(INT_PTR)val;
        break;
    case CIM_SINT8:
        *(INT8 *)ptr = val;
        break;
    case CIM_UINT8:
        *(UINT8 *)ptr = val;
        break;
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
    case CIM_SINT64:
        *(INT64 *)ptr = val;
        break;
    case CIM_UINT64:
        *(UINT64 *)ptr = val;
        break;
    default:
        FIXME( "unhandled column type %lu\n", type );
        return WBEM_E_FAILED;
    }
    return S_OK;
}

HRESULT get_method( const struct table *table, const WCHAR *name, class_method **func )
{
    UINT i, j;

    for (i = 0; i < table->num_rows; i++)
    {
        for (j = 0; j < table->num_cols; j++)
        {
            if (table->columns[j].type & COL_FLAG_METHOD && !wcscmp( table->columns[j].name, name ))
            {
                HRESULT hr;
                LONGLONG val;

                if ((hr = get_value( table, i, j, &val )) != S_OK) return hr;
                *func = (class_method *)(INT_PTR)val;
                return S_OK;
            }
        }
    }
    return WBEM_E_INVALID_METHOD;

}

void free_row_values( const struct table *table, UINT row )
{
    UINT i, type;
    LONGLONG val;

    for (i = 0; i < table->num_cols; i++)
    {
        if (!(table->columns[i].type & COL_FLAG_DYNAMIC)) continue;

        type = table->columns[i].type & COL_TYPE_MASK;
        if (type == CIM_STRING || type == CIM_DATETIME || type == CIM_REFERENCE)
        {
            if (get_value( table, row, i, &val ) == S_OK) free( (void *)(INT_PTR)val );
        }
        else if (type & CIM_FLAG_ARRAY)
        {
            if (get_value( table, row, i, &val ) == S_OK)
                destroy_array( (void *)(INT_PTR)val, type & CIM_TYPE_MASK );
        }
    }
}

void clear_table( struct table *table )
{
    UINT i;

    if (!table->data) return;

    for (i = 0; i < table->num_rows; i++) free_row_values( table, i );
    if (table->fill)
    {
        table->num_rows = 0;
        table->num_rows_allocated = 0;
        free( table->data );
        table->data = NULL;
    }
}

void free_columns( struct column *columns, UINT num_cols )
{
    UINT i;

    for (i = 0; i < num_cols; i++) { free( (WCHAR *)columns[i].name ); }
    free( columns );
}

void free_table( struct table *table )
{
    if (!table) return;
    assert( table->flags & TABLE_FLAG_DYNAMIC );

    TRACE("destroying %p\n", table);

    clear_table( table );
    free( (WCHAR *)table->name );
    free_columns( (struct column *)table->columns, table->num_cols );
    free( table->data );

    table->cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection( &table->cs );
    free( table );
}

void release_table( struct table *table )
{
    if (!--table->refs)
    {
        clear_table( table );
        if (table->flags & TABLE_FLAG_DYNAMIC)
        {
            EnterCriticalSection( &table_list_cs );
            list_remove( &table->entry );
            table->removed = TRUE;
            LeaveCriticalSection( &table_list_cs );

            LeaveCriticalSection( &table->cs );
            free_table( table );
            return;
        }
    }
    LeaveCriticalSection( &table->cs );
}

struct table *grab_table( struct table *table )
{
    EnterCriticalSection( &table->cs );
    if (table->removed)
    {
        LeaveCriticalSection( &table->cs );
        return NULL;
    }
    table->refs++;
    return table;
}

struct table *find_table( enum wbm_namespace ns, const WCHAR *name )
{
    struct table *table;

    if (ns == WBEMPROX_NAMESPACE_LAST) return NULL;

    LIST_FOR_EACH_ENTRY( table, table_list[ns], struct table, entry )
    {
        if (name && !wcsicmp( table->name, name ))
        {
            TRACE("returning %p\n", table);
            return grab_table( table );
        }
    }
    return NULL;
}

struct table *create_table( const WCHAR *name, UINT num_cols, const struct column *columns,
                            UINT num_rows, UINT num_allocated, BYTE *data,
                            enum fill_status (*fill)(struct table *, const struct expr *cond) )
{
    struct table *table;

    if (!(table = malloc( sizeof(*table) ))) return NULL;
    table->name               = wcsdup( name );
    table->num_cols           = num_cols;
    table->columns            = columns;
    table->num_rows           = num_rows;
    table->num_rows_allocated = num_allocated;
    table->data               = data;
    table->fill               = fill;
    table->flags              = TABLE_FLAG_DYNAMIC;
    table->refs               = 0;
    table->removed            = FALSE;
    list_init( &table->entry );
    InitializeCriticalSectionEx( &table->cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO );
    table->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": table.cs");
    return table;
}

BOOL add_table( enum wbm_namespace ns, struct table *table )
{
    struct table *iter;

    if (ns == WBEMPROX_NAMESPACE_LAST) return FALSE;

    EnterCriticalSection( &table_list_cs );
    LIST_FOR_EACH_ENTRY( iter, table_list[ns], struct table, entry )
    {
        if (!wcsicmp( iter->name, table->name ))
        {
            TRACE("table %s already exists\n", debugstr_w(table->name));
            LeaveCriticalSection( &table_list_cs );
            return FALSE;
        }
    }
    list_add_tail( table_list[ns], &table->entry );
    LeaveCriticalSection( &table_list_cs );

    TRACE("added %p\n", table);
    return TRUE;
}

BSTR get_method_name( enum wbm_namespace ns, const WCHAR *class, UINT index )
{
    struct table *table;
    UINT i, count = 0;
    BSTR ret;

    if (!(table = find_table( ns, class ))) return NULL;

    for (i = 0; i < table->num_cols; i++)
    {
        if (table->columns[i].type & COL_FLAG_METHOD)
        {
            if (index == count)
            {
                ret = SysAllocString( table->columns[i].name );
                release_table( table );
                return ret;
            }
            count++;
        }
    }
    release_table( table );
    return NULL;
}

WCHAR *get_first_key_property( enum wbm_namespace ns, const WCHAR *class )
{
    struct table *table;
    WCHAR *ret = NULL;
    UINT i;

    if (!(table = find_table( ns, class ))) return NULL;

    for (i = 0; i < table->num_cols; i++)
    {
        if (table->columns[i].type & COL_FLAG_KEY)
        {
            ret = wcsdup( table->columns[i].name );
            break;
        }
    }

    release_table( table );
    return ret;
}
