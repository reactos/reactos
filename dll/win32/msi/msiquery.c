/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2002-2005 Mike McCormack for CodeWeavers
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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wine/debug.h"
#include "wine/exception.h"
#include "msi.h"
#include "msiquery.h"
#include "objbase.h"
#include "objidl.h"
#include "winnls.h"

#include "msipriv.h"
#include "query.h"
#include "winemsi.h"

#include "initguid.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

static void MSI_CloseView( MSIOBJECTHDR *arg )
{
    MSIQUERY *query = (MSIQUERY*) arg;
    struct list *ptr, *t;

    if( query->view && query->view->ops->delete )
        query->view->ops->delete( query->view );
    msiobj_release( &query->db->hdr );

    LIST_FOR_EACH_SAFE( ptr, t, &query->mem )
    {
        free( ptr );
    }
}

UINT VIEW_find_column( MSIVIEW *table, LPCWSTR name, LPCWSTR table_name, UINT *n )
{
    LPCWSTR col_name, haystack_table_name;
    UINT i, count, r;

    r = table->ops->get_dimensions( table, NULL, &count );
    if( r != ERROR_SUCCESS )
        return r;

    for( i=1; i<=count; i++ )
    {
        INT x;

        r = table->ops->get_column_info( table, i, &col_name, NULL,
                                         NULL, &haystack_table_name );
        if( r != ERROR_SUCCESS )
            return r;
        x = wcscmp( name, col_name );
        if( table_name )
            x |= wcscmp( table_name, haystack_table_name );
        if( !x )
        {
            *n = i;
            return ERROR_SUCCESS;
        }
    }
    return ERROR_INVALID_PARAMETER;
}

UINT WINAPI MsiDatabaseOpenViewA( MSIHANDLE hdb, const char *szQuery, MSIHANDLE *phView )
{
    UINT r;
    WCHAR *szwQuery;

    TRACE( "%lu, %s, %p\n", hdb, debugstr_a(szQuery), phView );

    if( szQuery )
    {
        szwQuery = strdupAtoW( szQuery );
        if( !szwQuery )
            return ERROR_FUNCTION_FAILED;
    }
    else
        szwQuery = NULL;

    r = MsiDatabaseOpenViewW( hdb, szwQuery, phView);

    free( szwQuery );
    return r;
}

UINT MSI_DatabaseOpenViewW( MSIDATABASE *db, const WCHAR *szQuery, MSIQUERY **pView )
{
    MSIQUERY *query;
    UINT r;

    TRACE( "%s, %p\n", debugstr_w(szQuery), pView );

    /* pre allocate a handle to hold a pointer to the view */
    query = alloc_msiobject( MSIHANDLETYPE_VIEW, sizeof (MSIQUERY),
                              MSI_CloseView );
    if( !query )
        return ERROR_FUNCTION_FAILED;

    msiobj_addref( &db->hdr );
    query->db = db;
    list_init( &query->mem );

    r = MSI_ParseSQL( db, szQuery, &query->view, &query->mem );
    if( r == ERROR_SUCCESS )
    {
        msiobj_addref( &query->hdr );
        *pView = query;
    }

    msiobj_release( &query->hdr );
    return r;
}

UINT WINAPIV MSI_OpenQuery( MSIDATABASE *db, MSIQUERY **view, LPCWSTR fmt, ... )
{
    UINT r;
    int size = 100, res;
    LPWSTR query;

    /* construct the string */
    for (;;)
    {
        va_list va;
        query = malloc(size * sizeof(WCHAR));
        va_start(va, fmt);
        res = vswprintf(query, size, fmt, va);
        va_end(va);
        if (res == -1) size *= 2;
        else if (res >= size) size = res + 1;
        else break;
        free(query);
    }
    /* perform the query */
    r = MSI_DatabaseOpenViewW(db, query, view);
    free(query);
    return r;
}

UINT MSI_IterateRecords( MSIQUERY *view, LPDWORD count,
                         record_func func, LPVOID param )
{
    MSIRECORD *rec = NULL;
    UINT r, n = 0, max = 0;

    r = MSI_ViewExecute( view, NULL );
    if( r != ERROR_SUCCESS )
        return r;

    if( count )
        max = *count;

    /* iterate a query */
    for( n = 0; (max == 0) || (n < max); n++ )
    {
        r = MSI_ViewFetch( view, &rec );
        if( r != ERROR_SUCCESS )
            break;
        if (func)
            r = func( rec, param );
        msiobj_release( &rec->hdr );
        if( r != ERROR_SUCCESS )
            break;
    }

    MSI_ViewClose( view );

    if( count )
        *count = n;

    if( r == ERROR_NO_MORE_ITEMS )
        r = ERROR_SUCCESS;

    return r;
}

/* return a single record from a query */
MSIRECORD * WINAPIV MSI_QueryGetRecord( MSIDATABASE *db, LPCWSTR fmt, ... )
{
    MSIRECORD *rec = NULL;
    MSIQUERY *view = NULL;
    UINT r;
    int size = 100, res;
    LPWSTR query;

    /* construct the string */
    for (;;)
    {
        va_list va;
        query = malloc(size * sizeof(WCHAR));
        va_start(va, fmt);
        res = vswprintf(query, size, fmt, va);
        va_end(va);
        if (res == -1) size *= 2;
        else if (res >= size) size = res + 1;
        else break;
        free(query);
    }
    /* perform the query */
    r = MSI_DatabaseOpenViewW(db, query, &view);
    free(query);

    if( r == ERROR_SUCCESS )
    {
        MSI_ViewExecute( view, NULL );
        MSI_ViewFetch( view, &rec );
        MSI_ViewClose( view );
        msiobj_release( &view->hdr );
    }
    return rec;
}

UINT WINAPI MsiDatabaseOpenViewW(MSIHANDLE hdb,
              LPCWSTR szQuery, MSIHANDLE *phView)
{
    MSIDATABASE *db;
    MSIQUERY *query = NULL;
    UINT ret;

    TRACE("%s %p\n", debugstr_w(szQuery), phView);

    if (!phView)
        return ERROR_INVALID_PARAMETER;

    if (!szQuery)
        return ERROR_BAD_QUERY_SYNTAX;

    db = msihandle2msiinfo( hdb, MSIHANDLETYPE_DATABASE );
    if( !db )
    {
        MSIHANDLE remote, remote_view;

        if (!(remote = msi_get_remote(hdb)))
            return ERROR_INVALID_HANDLE;

        __TRY
        {
            ret = remote_DatabaseOpenView(remote, szQuery, &remote_view);
        }
        __EXCEPT(rpc_filter)
        {
            ret = GetExceptionCode();
        }
        __ENDTRY

        if (!ret)
            *phView = alloc_msi_remote_handle(remote_view);
        return ret;
    }

    ret = MSI_DatabaseOpenViewW( db, szQuery, &query );
    if( ret == ERROR_SUCCESS )
    {
        *phView = alloc_msihandle( &query->hdr );
        if (! *phView)
           ret = ERROR_NOT_ENOUGH_MEMORY;
        msiobj_release( &query->hdr );
    }
    msiobj_release( &db->hdr );

    return ret;
}

UINT msi_view_refresh_row(MSIDATABASE *db, MSIVIEW *view, UINT row, MSIRECORD *rec)
{
    UINT row_count = 0, col_count = 0, i, ival, ret, type;

    ret = view->ops->get_dimensions(view, &row_count, &col_count);
    if (ret)
        return ret;

    if (!col_count)
        return ERROR_INVALID_PARAMETER;

    for (i = 1; i <= col_count; i++)
    {
        ret = view->ops->get_column_info(view, i, NULL, &type, NULL, NULL);
        if (ret)
        {
            ERR("Error getting column type for %d\n", i);
            continue;
        }

        if (MSITYPE_IS_BINARY(type))
        {
            IStream *stm = NULL;

            ret = view->ops->fetch_stream(view, row, i, &stm);
            if ((ret == ERROR_SUCCESS) && stm)
            {
                MSI_RecordSetIStream(rec, i, stm);
                IStream_Release(stm);
            }
            else
                WARN("failed to get stream\n");

            continue;
        }

        ret = view->ops->fetch_int(view, row, i, &ival);
        if (ret)
        {
            ERR("Error fetching data for %d\n", i);
            continue;
        }

        if (! (type & MSITYPE_VALID))
            ERR("Invalid type!\n");

        if (type & MSITYPE_STRING)
        {
            int len;
            const WCHAR *sval = msi_string_lookup(db->strings, ival, &len);
            msi_record_set_string(rec, i, sval, len);
        }
        else
        {
            if ((type & MSI_DATASIZEMASK) == 2)
                MSI_RecordSetInteger(rec, i, ival ? ival - (1<<15) : MSI_NULL_INTEGER);
            else
                MSI_RecordSetInteger(rec, i, ival - (1u<<31));
        }
    }

    return ERROR_SUCCESS;
}

UINT msi_view_get_row(MSIDATABASE *db, MSIVIEW *view, UINT row, MSIRECORD **rec)
{
    UINT row_count = 0, col_count = 0, r;
    MSIRECORD *object;

    if ((r = view->ops->get_dimensions(view, &row_count, &col_count)))
        return r;

    if (row >= row_count)
        return ERROR_NO_MORE_ITEMS;

    if (!(object = MSI_CreateRecord( col_count )))
        return ERROR_OUTOFMEMORY;

    if ((r = msi_view_refresh_row(db, view, row, object)))
        msiobj_release( &object->hdr );
    else
        *rec = object;

    return r;
}

UINT MSI_ViewFetch(MSIQUERY *query, MSIRECORD **prec)
{
    MSIVIEW *view;
    UINT r;

    TRACE("%p %p\n", query, prec );

    view = query->view;
    if( !view )
        return ERROR_FUNCTION_FAILED;

    r = msi_view_get_row(query->db, view, query->row, prec);
    if (r == ERROR_SUCCESS)
    {
        query->row ++;
        (*prec)->cookie = (UINT64)(ULONG_PTR)query;
        MSI_RecordSetInteger(*prec, 0, 1);
    }
    else if (r == ERROR_NO_MORE_ITEMS)
    {
        /* end of view; reset cursor to first row */
        query->row = 0;
    }

    return r;
}

UINT WINAPI MsiViewFetch( MSIHANDLE hView, MSIHANDLE *record )
{
    MSIQUERY *query;
    MSIRECORD *rec = NULL;
    UINT ret;

    TRACE( "%lu, %p\n", hView, record );

    if( !record )
        return ERROR_INVALID_PARAMETER;
    *record = 0;

    query = msihandle2msiinfo( hView, MSIHANDLETYPE_VIEW );
    if (!query)
    {
        struct wire_record *wire_rec = NULL;
        MSIHANDLE remote;

        if (!(remote = msi_get_remote(hView)))
            return ERROR_INVALID_HANDLE;

        __TRY
        {
            ret = remote_ViewFetch(remote, &wire_rec);
        }
        __EXCEPT(rpc_filter)
        {
            ret = GetExceptionCode();
        }
        __ENDTRY

        if (!ret)
        {
            ret = unmarshal_record(wire_rec, record);
            free_remote_record(wire_rec);
        }
        return ret;
    }
    ret = MSI_ViewFetch( query, &rec );
    if( ret == ERROR_SUCCESS )
    {
        *record = alloc_msihandle( &rec->hdr );
        if (! *record)
           ret = ERROR_NOT_ENOUGH_MEMORY;
        msiobj_release( &rec->hdr );
    }
    msiobj_release( &query->hdr );
    return ret;
}

UINT MSI_ViewClose(MSIQUERY *query)
{
    MSIVIEW *view;

    TRACE("%p\n", query );

    view = query->view;
    if( !view )
        return ERROR_FUNCTION_FAILED;
    if( !view->ops->close )
        return ERROR_FUNCTION_FAILED;

    return view->ops->close( view );
}

UINT WINAPI MsiViewClose( MSIHANDLE hView )
{
    MSIQUERY *query;
    UINT ret;

    TRACE( "%lu\n", hView );

    query = msihandle2msiinfo( hView, MSIHANDLETYPE_VIEW );
    if (!query)
    {
        MSIHANDLE remote;

        if (!(remote = msi_get_remote(hView)))
            return ERROR_INVALID_HANDLE;

        __TRY
        {
            ret = remote_ViewClose(remote);
        }
        __EXCEPT(rpc_filter)
        {
            ret = GetExceptionCode();
        }
        __ENDTRY

        return ret;
    }

    ret = MSI_ViewClose( query );
    msiobj_release( &query->hdr );
    return ret;
}

UINT MSI_ViewExecute(MSIQUERY *query, MSIRECORD *rec )
{
    MSIVIEW *view;

    TRACE("%p %p\n", query, rec);

    view = query->view;
    if( !view )
        return ERROR_FUNCTION_FAILED;
    if( !view->ops->execute )
        return ERROR_FUNCTION_FAILED;
    query->row = 0;

    return view->ops->execute( view, rec );
}

UINT WINAPI MsiViewExecute( MSIHANDLE hView, MSIHANDLE hRec )
{
    MSIQUERY *query;
    MSIRECORD *rec = NULL;
    UINT ret;

    TRACE( "%lu, %lu\n", hView, hRec );

    if( hRec )
    {
        rec = msihandle2msiinfo( hRec, MSIHANDLETYPE_RECORD );
        if( !rec )
            return ERROR_INVALID_HANDLE;
    }

    query = msihandle2msiinfo( hView, MSIHANDLETYPE_VIEW );
    if( !query )
    {
        MSIHANDLE remote;

        if (!(remote = msi_get_remote(hView)))
            return ERROR_INVALID_HANDLE;

        __TRY
        {
            ret = remote_ViewExecute(remote, rec ? (struct wire_record *)&rec->count : NULL);
        }
        __EXCEPT(rpc_filter)
        {
            ret = GetExceptionCode();
        }
        __ENDTRY

        if (rec)
            msiobj_release(&rec->hdr);
        return ret;
    }

    msiobj_lock( &rec->hdr );
    ret = MSI_ViewExecute( query, rec );
    msiobj_unlock( &rec->hdr );

    msiobj_release( &query->hdr );
    if( rec )
        msiobj_release( &rec->hdr );

    return ret;
}

static UINT set_record_type_string( MSIRECORD *rec, UINT field, UINT type, BOOL temporary )
{
    WCHAR szType[0x10];

    if (MSITYPE_IS_BINARY(type))
        szType[0] = 'v';
    else if (type & MSITYPE_LOCALIZABLE)
        szType[0] = 'l';
    else if (type & MSITYPE_UNKNOWN)
        szType[0] = 'f';
    else if (type & MSITYPE_STRING)
    {
        if (temporary)
            szType[0] = 'g';
        else
          szType[0] = 's';
    }
    else
    {
        if (temporary)
            szType[0] = 'j';
        else
            szType[0] = 'i';
    }

    if (type & MSITYPE_NULLABLE)
        szType[0] &= ~0x20;

    swprintf( &szType[1], ARRAY_SIZE(szType) - 1, L"%d", (type&0xff) );

    TRACE("type %04x -> %s\n", type, debugstr_w(szType) );

    return MSI_RecordSetStringW( rec, field, szType );
}

UINT MSI_ViewGetColumnInfo( MSIQUERY *query, MSICOLINFO info, MSIRECORD **prec )
{
    UINT r = ERROR_FUNCTION_FAILED, i, count = 0, type;
    MSIRECORD *rec;
    MSIVIEW *view = query->view;
    LPCWSTR name;
    BOOL temporary;

    if( !view )
        return ERROR_FUNCTION_FAILED;

    if( !view->ops->get_dimensions )
        return ERROR_FUNCTION_FAILED;

    r = view->ops->get_dimensions( view, NULL, &count );
    if( r != ERROR_SUCCESS )
        return r;
    if( !count )
        return ERROR_INVALID_PARAMETER;

    rec = MSI_CreateRecord( count );
    if( !rec )
        return ERROR_FUNCTION_FAILED;

    for( i=0; i<count; i++ )
    {
        name = NULL;
        r = view->ops->get_column_info( view, i+1, &name, &type, &temporary, NULL );
        if( r != ERROR_SUCCESS )
            continue;
        if (info == MSICOLINFO_NAMES)
            MSI_RecordSetStringW( rec, i+1, name );
        else
            set_record_type_string( rec, i+1, type, temporary );
    }
    *prec = rec;
    return ERROR_SUCCESS;
}

UINT WINAPI MsiViewGetColumnInfo( MSIHANDLE hView, MSICOLINFO info, MSIHANDLE *hRec )
{
    MSIQUERY *query = NULL;
    MSIRECORD *rec = NULL;
    UINT r;

    TRACE( "%lu, %d, %p\n", hView, info, hRec );

    if( !hRec )
        return ERROR_INVALID_PARAMETER;

    if( info != MSICOLINFO_NAMES && info != MSICOLINFO_TYPES )
        return ERROR_INVALID_PARAMETER;

    query = msihandle2msiinfo( hView, MSIHANDLETYPE_VIEW );
    if (!query)
    {
        struct wire_record *wire_rec = NULL;
        MSIHANDLE remote;

        if (!(remote = msi_get_remote(hView)))
            return ERROR_INVALID_HANDLE;

        __TRY
        {
            r = remote_ViewGetColumnInfo(remote, info, &wire_rec);
        }
        __EXCEPT(rpc_filter)
        {
            r = GetExceptionCode();
        }
        __ENDTRY

        if (!r)
        {
            r = unmarshal_record(wire_rec, hRec);
            free_remote_record(wire_rec);
        }

        return r;
    }

    r = MSI_ViewGetColumnInfo( query, info, &rec );
    if ( r == ERROR_SUCCESS )
    {
        *hRec = alloc_msihandle( &rec->hdr );
        if ( !*hRec )
            r = ERROR_NOT_ENOUGH_MEMORY;
        msiobj_release( &rec->hdr );
    }

    msiobj_release( &query->hdr );

    return r;
}

UINT MSI_ViewModify( MSIQUERY *query, MSIMODIFY mode, MSIRECORD *rec )
{
    MSIVIEW *view = NULL;
    UINT r;

    if ( !query || !rec )
        return ERROR_INVALID_HANDLE;

    view = query->view;
    if ( !view  || !view->ops->modify)
        return ERROR_FUNCTION_FAILED;

    if ( mode == MSIMODIFY_UPDATE && rec->cookie != (UINT64)(ULONG_PTR)query )
        return ERROR_FUNCTION_FAILED;

    r = view->ops->modify( view, mode, rec, query->row - 1 );
    if (mode == MSIMODIFY_DELETE && r == ERROR_SUCCESS)
        query->row--;

    return r;
}

UINT WINAPI MsiViewModify( MSIHANDLE hView, MSIMODIFY eModifyMode, MSIHANDLE hRecord )
{
    MSIQUERY *query = NULL;
    MSIRECORD *rec = NULL;
    UINT r = ERROR_FUNCTION_FAILED;

    TRACE( "%lu, %#x, %lu\n", hView, eModifyMode, hRecord );

    rec = msihandle2msiinfo( hRecord, MSIHANDLETYPE_RECORD );

    if (!rec)
        return ERROR_INVALID_HANDLE;

    query = msihandle2msiinfo( hView, MSIHANDLETYPE_VIEW );
    if (!query)
    {
        struct wire_record *wire_refreshed = NULL;
        MSIHANDLE remote;

        if (!(remote = msi_get_remote(hView)))
            return ERROR_INVALID_HANDLE;

        __TRY
        {
            r = remote_ViewModify(remote, eModifyMode,
                (struct wire_record *)&rec->count, &wire_refreshed);
        }
        __EXCEPT(rpc_filter)
        {
            r = GetExceptionCode();
        }
        __ENDTRY

        if (!r && (eModifyMode == MSIMODIFY_REFRESH || eModifyMode == MSIMODIFY_SEEK))
        {
            r = copy_remote_record(wire_refreshed, hRecord);
            free_remote_record(wire_refreshed);
        }

        msiobj_release(&rec->hdr);
        return r;
    }

    r = MSI_ViewModify( query, eModifyMode, rec );

    msiobj_release( &query->hdr );
    msiobj_release(&rec->hdr);
    return r;
}

MSIDBERROR WINAPI MsiViewGetErrorW( MSIHANDLE handle, WCHAR *buffer, DWORD *buflen )
{
    MSIQUERY *query;
    const WCHAR *column;
    MSIDBERROR r;

    TRACE( "%lu, %p, %p\n", handle, buffer, buflen );

    if (!buflen)
        return MSIDBERROR_INVALIDARG;

    if (!(query = msihandle2msiinfo(handle, MSIHANDLETYPE_VIEW)))
    {
        WCHAR *remote_column = NULL;
        MSIHANDLE remote;

        if (!(remote = msi_get_remote(handle)))
            return MSIDBERROR_INVALIDARG;

        if (!*buflen)
            return MSIDBERROR_FUNCTIONERROR;

        __TRY
        {
            r = remote_ViewGetError(remote, &remote_column);
        }
        __EXCEPT(rpc_filter)
        {
            r = GetExceptionCode();
        }
        __ENDTRY;

        if (msi_strncpyW(remote_column ? remote_column : L"", -1, buffer, buflen) == ERROR_MORE_DATA)
            r = MSIDBERROR_MOREDATA;

        if (remote_column)
            midl_user_free(remote_column);

        return r;
    }

    if ((r = query->view->error)) column = query->view->error_column;
    else column = L"";

    if (msi_strncpyW(column, -1, buffer, buflen) == ERROR_MORE_DATA)
        r = MSIDBERROR_MOREDATA;

    msiobj_release( &query->hdr );
    return r;
}

MSIDBERROR WINAPI MsiViewGetErrorA( MSIHANDLE handle, char *buffer, DWORD *buflen )
{
    MSIQUERY *query;
    const WCHAR *column;
    MSIDBERROR r;

    TRACE( "%lu, %p, %p\n", handle, buffer, buflen );

    if (!buflen)
        return MSIDBERROR_INVALIDARG;

    if (!(query = msihandle2msiinfo(handle, MSIHANDLETYPE_VIEW)))
    {
        WCHAR *remote_column = NULL;
        MSIHANDLE remote;

        if (!(remote = msi_get_remote(handle)))
            return MSIDBERROR_INVALIDARG;

        if (!*buflen)
            return MSIDBERROR_FUNCTIONERROR;

        __TRY
        {
            r = remote_ViewGetError(remote, &remote_column);
        }
        __EXCEPT(rpc_filter)
        {
            r = GetExceptionCode();
        }
        __ENDTRY;

        if (msi_strncpyWtoA(remote_column ? remote_column : L"", -1, buffer, buflen, FALSE) == ERROR_MORE_DATA)
            r = MSIDBERROR_MOREDATA;

        if (remote_column)
            midl_user_free(remote_column);

        return r;
    }

    if ((r = query->view->error)) column = query->view->error_column;
    else column = L"";

    if (msi_strncpyWtoA(column, -1, buffer, buflen, FALSE) == ERROR_MORE_DATA)
        r = MSIDBERROR_MOREDATA;

    msiobj_release( &query->hdr );
    return r;
}

MSIHANDLE WINAPI MsiGetLastErrorRecord( void )
{
    FIXME("\n");
    return 0;
}

UINT MSI_DatabaseApplyTransformW( MSIDATABASE *db, const WCHAR *transform, int error_cond )
{
    HRESULT hr;
    UINT ret = ERROR_FUNCTION_FAILED;
    IStorage *stg;
    STATSTG stat;

    TRACE( "%p %s %08x\n", db, debugstr_w(transform), error_cond );

    if (*transform == ':')
    {
        hr = IStorage_OpenStorage( db->storage, transform + 1, NULL, STGM_SHARE_EXCLUSIVE, NULL, 0, &stg );
        if (FAILED( hr ))
        {
            WARN( "failed to open substorage transform %#lx\n", hr );
            return ERROR_FUNCTION_FAILED;
        }
    }
    else
    {
        hr = StgOpenStorage( transform, NULL, STGM_DIRECT|STGM_READ|STGM_SHARE_DENY_WRITE, NULL, 0, &stg );
        if (FAILED( hr ))
        {
            WARN( "failed to open file transform %#lx\n", hr );
            return ERROR_FUNCTION_FAILED;
        }
    }

    hr = IStorage_Stat( stg, &stat, STATFLAG_NONAME );
    if (FAILED( hr )) goto end;
    if (!IsEqualGUID( &stat.clsid, &CLSID_MsiTransform )) goto end;
    if (TRACE_ON( msi )) enum_stream_names( stg );

    ret = msi_table_apply_transform( db, stg, error_cond );

end:
    IStorage_Release( stg );
    return ret;
}

UINT WINAPI MsiDatabaseApplyTransformW( MSIHANDLE hdb, const WCHAR *transform, int error_cond )
{
    MSIDATABASE *db;
    UINT r;

    if (error_cond & ~MSITRANSFORM_ERROR_VIEWTRANSFORM) FIXME( "ignoring error conditions\n" );

    if (!(db = msihandle2msiinfo(hdb, MSIHANDLETYPE_DATABASE)))
        return ERROR_INVALID_HANDLE;

    r = MSI_DatabaseApplyTransformW( db, transform, error_cond );
    msiobj_release( &db->hdr );
    return r;
}

UINT WINAPI MsiDatabaseApplyTransformA( MSIHANDLE hdb, const char *transform, int error_cond )
{
    WCHAR *wstr;
    UINT ret;

    TRACE( "%lu, %s, %#x\n", hdb, debugstr_a(transform), error_cond );

    wstr = strdupAtoW( transform );
    if (transform && !wstr)
        return ERROR_NOT_ENOUGH_MEMORY;

    ret = MsiDatabaseApplyTransformW( hdb, wstr, error_cond );
    free( wstr );
    return ret;
}

UINT WINAPI MsiDatabaseGenerateTransformA( MSIHANDLE hdb, MSIHANDLE hdbref, const char *szTransformFile,
                                           int iReserved1, int iReserved2 )
{
    FIXME( "%lu, %lu, %s, %d, %d\n", hdb, hdbref, debugstr_a(szTransformFile), iReserved1, iReserved2 );
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiDatabaseGenerateTransformW( MSIHANDLE hdb, MSIHANDLE hdbref, const WCHAR *szTransformFile,
                                           int iReserved1, int iReserved2 )
{
    FIXME( "%lu, %lu, %s, %d, %d\n", hdb, hdbref, debugstr_w(szTransformFile), iReserved1, iReserved2 );
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiDatabaseCommit( MSIHANDLE hdb )
{
    MSIDATABASE *db;
    UINT r;

    TRACE( "%lu\n", hdb );

    db = msihandle2msiinfo( hdb, MSIHANDLETYPE_DATABASE );
    if( !db )
    {
        MSIHANDLE remote;

        if (!(remote = msi_get_remote(hdb)))
            return ERROR_INVALID_HANDLE;

        WARN("not allowed during a custom action!\n");

        return ERROR_SUCCESS;
    }

    if (db->mode == MSI_OPEN_READONLY)
    {
        msiobj_release( &db->hdr );
        return ERROR_SUCCESS;
    }

    /* FIXME: lock the database */

    r = msi_commit_streams( db );
    if (r != ERROR_SUCCESS) ERR("Failed to commit streams!\n");
    else
    {
        r = MSI_CommitTables( db );
        if (r != ERROR_SUCCESS) ERR("Failed to commit tables!\n");
    }

    /* FIXME: unlock the database */

    msiobj_release( &db->hdr );

    if (r == ERROR_SUCCESS)
    {
        free( db->deletefile );
        db->deletefile = NULL;
    }

    return r;
}

struct primary_key_record_info
{
    DWORD n;
    MSIRECORD *rec;
};

static UINT primary_key_iterator( MSIRECORD *rec, void *param )
{
    struct primary_key_record_info *info = param;
    LPCWSTR name, table;
    DWORD type;

    type = MSI_RecordGetInteger( rec, 4 );
    if( type & MSITYPE_KEY )
    {
        info->n++;
        if( info->rec )
        {
            if ( info->n == 1 )
            {
                table = MSI_RecordGetString( rec, 1 );
                MSI_RecordSetStringW( info->rec, 0, table);
            }

            name = MSI_RecordGetString( rec, 3 );
            MSI_RecordSetStringW( info->rec, info->n, name );
        }
    }

    return ERROR_SUCCESS;
}

UINT MSI_DatabaseGetPrimaryKeys( MSIDATABASE *db, const WCHAR *table, MSIRECORD **prec )
{
    struct primary_key_record_info info;
    MSIQUERY *query = NULL;
    UINT r;

    if (!TABLE_Exists( db, table ))
        return ERROR_INVALID_TABLE;

    r = MSI_OpenQuery( db, &query, L"SELECT * FROM `_Columns` WHERE `Table` = '%s'", table );
    if( r != ERROR_SUCCESS )
        return r;

    /* count the number of primary key records */
    info.n = 0;
    info.rec = 0;
    r = MSI_IterateRecords( query, 0, primary_key_iterator, &info );
    if( r == ERROR_SUCCESS )
    {
        TRACE( "found %lu primary keys\n", info.n );

        /* allocate a record and fill in the names of the tables */
        info.rec = MSI_CreateRecord( info.n );
        info.n = 0;
        r = MSI_IterateRecords( query, 0, primary_key_iterator, &info );
        if( r == ERROR_SUCCESS )
            *prec = info.rec;
        else
            msiobj_release( &info.rec->hdr );
    }
    msiobj_release( &query->hdr );

    return r;
}

UINT WINAPI MsiDatabaseGetPrimaryKeysW( MSIHANDLE hdb, const WCHAR *table, MSIHANDLE *phRec )
{
    MSIRECORD *rec = NULL;
    MSIDATABASE *db;
    UINT r;

    TRACE( "%lu, %s, %p\n", hdb, debugstr_w(table), phRec );

    db = msihandle2msiinfo( hdb, MSIHANDLETYPE_DATABASE );
    if( !db )
    {
        struct wire_record *wire_rec = NULL;
        MSIHANDLE remote;

        if (!(remote = msi_get_remote(hdb)))
            return ERROR_INVALID_HANDLE;

        __TRY
        {
            r = remote_DatabaseGetPrimaryKeys(remote, table, &wire_rec);
        }
        __EXCEPT(rpc_filter)
        {
            r = GetExceptionCode();
        }
        __ENDTRY

        if (!r)
        {
            r = unmarshal_record(wire_rec, phRec);
            free_remote_record(wire_rec);
        }

        return r;
    }

    r = MSI_DatabaseGetPrimaryKeys( db, table, &rec );
    if( r == ERROR_SUCCESS )
    {
        *phRec = alloc_msihandle( &rec->hdr );
        if (! *phRec)
           r = ERROR_NOT_ENOUGH_MEMORY;
        msiobj_release( &rec->hdr );
    }
    msiobj_release( &db->hdr );

    return r;
}

UINT WINAPI MsiDatabaseGetPrimaryKeysA( MSIHANDLE hdb, const char *table, MSIHANDLE *phRec )
{
    WCHAR *szwTable = NULL;
    UINT r;

    TRACE( "%lu, %s, %p\n", hdb, debugstr_a(table), phRec );

    if( table )
    {
        szwTable = strdupAtoW( table );
        if( !szwTable )
            return ERROR_OUTOFMEMORY;
    }
    r = MsiDatabaseGetPrimaryKeysW( hdb, szwTable, phRec );
    free( szwTable );

    return r;
}

MSICONDITION WINAPI MsiDatabaseIsTablePersistentA( MSIHANDLE hDatabase, const char *szTableName )
{
    WCHAR *szwTableName = NULL;
    MSICONDITION r;

    TRACE( "%lu, %s\n", hDatabase, debugstr_a(szTableName) );

    if( szTableName )
    {
        szwTableName = strdupAtoW( szTableName );
        if( !szwTableName )
            return MSICONDITION_ERROR;
    }
    r = MsiDatabaseIsTablePersistentW( hDatabase, szwTableName );
    free( szwTableName );

    return r;
}

MSICONDITION WINAPI MsiDatabaseIsTablePersistentW( MSIHANDLE hDatabase, const WCHAR *szTableName )
{
    MSIDATABASE *db;
    MSICONDITION r;

    TRACE( "%lu, %s\n", hDatabase, debugstr_w(szTableName) );

    db = msihandle2msiinfo( hDatabase, MSIHANDLETYPE_DATABASE );
    if( !db )
    {
        MSIHANDLE remote;

        if (!(remote = msi_get_remote(hDatabase)))
            return MSICONDITION_ERROR;

        __TRY
        {
            r = remote_DatabaseIsTablePersistent(remote, szTableName);
        }
        __EXCEPT(rpc_filter)
        {
            r = MSICONDITION_ERROR;
        }
        __ENDTRY

        return r;
    }

    r = MSI_DatabaseIsTablePersistent( db, szTableName );

    msiobj_release( &db->hdr );

    return r;
}

UINT __cdecl s_remote_ViewClose(MSIHANDLE view)
{
    return MsiViewClose(view);
}

UINT __cdecl s_remote_ViewExecute(MSIHANDLE view, struct wire_record *remote_rec)
{
    MSIHANDLE rec = 0;
    UINT r;

    if ((r = unmarshal_record(remote_rec, &rec)))
        return r;

    r = MsiViewExecute(view, rec);

    MsiCloseHandle(rec);
    return r;
}

UINT __cdecl s_remote_ViewFetch(MSIHANDLE view, struct wire_record **rec)
{
    MSIHANDLE handle;
    UINT r = MsiViewFetch(view, &handle);
    *rec = NULL;
    if (!r)
        *rec = marshal_record(handle);
    MsiCloseHandle(handle);
    return r;
}

UINT __cdecl s_remote_ViewGetColumnInfo(MSIHANDLE view, MSICOLINFO info, struct wire_record **rec)
{
    MSIHANDLE handle;
    UINT r = MsiViewGetColumnInfo(view, info, &handle);
    *rec = NULL;
    if (!r)
        *rec = marshal_record(handle);
    MsiCloseHandle(handle);
    return r;
}

MSIDBERROR __cdecl s_remote_ViewGetError(MSIHANDLE view, LPWSTR *column)
{
    WCHAR empty[1];
    DWORD size = 1;
    UINT r;

    r = MsiViewGetErrorW(view, empty, &size);
    if (r == MSIDBERROR_MOREDATA)
    {
        if (!(*column = midl_user_allocate(++size * sizeof(WCHAR))))
            return MSIDBERROR_FUNCTIONERROR;
        r = MsiViewGetErrorW(view, *column, &size);
    }
    return r;
}

UINT __cdecl s_remote_ViewModify(MSIHANDLE view, MSIMODIFY mode,
    struct wire_record *remote_rec, struct wire_record **remote_refreshed)
{
    MSIHANDLE handle = 0;
    UINT r;

    if ((r = unmarshal_record(remote_rec, &handle)))
        return r;

    r = MsiViewModify(view, mode, handle);
    *remote_refreshed = NULL;
    if (!r && (mode == MSIMODIFY_REFRESH || mode == MSIMODIFY_SEEK))
        *remote_refreshed = marshal_record(handle);

    MsiCloseHandle(handle);
    return r;
}
