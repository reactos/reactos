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
#include "wine/unicode.h"
#include "msi.h"
#include "msiquery.h"
#include "objbase.h"
#include "objidl.h"
#include "msipriv.h"
#include "winnls.h"

#include "query.h"
#include "msiserver.h"

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
        msi_free( ptr );
    }
}

UINT VIEW_find_column( MSIVIEW *table, LPCWSTR name, UINT *n )
{
    LPWSTR col_name;
    UINT i, count, r;

    r = table->ops->get_dimensions( table, NULL, &count );
    if( r != ERROR_SUCCESS )
        return r;

    for( i=1; i<=count; i++ )
    {
        INT x;

        col_name = NULL;
        r = table->ops->get_column_info( table, i, &col_name, NULL, NULL );
        if( r != ERROR_SUCCESS )
            return r;
        x = lstrcmpW( name, col_name );
        msi_free( col_name );
        if( !x )
        {
            *n = i;
            return ERROR_SUCCESS;
        }
    }

    return ERROR_INVALID_PARAMETER;
}

UINT WINAPI MsiDatabaseOpenViewA(MSIHANDLE hdb,
              LPCSTR szQuery, MSIHANDLE *phView)
{
    UINT r;
    LPWSTR szwQuery;

    TRACE("%d %s %p\n", hdb, debugstr_a(szQuery), phView);

    if( szQuery )
    {
        szwQuery = strdupAtoW( szQuery );
        if( !szwQuery )
            return ERROR_FUNCTION_FAILED;
    }
    else
        szwQuery = NULL;

    r = MsiDatabaseOpenViewW( hdb, szwQuery, phView);

    msi_free( szwQuery );
    return r;
}

UINT MSI_DatabaseOpenViewW(MSIDATABASE *db,
              LPCWSTR szQuery, MSIQUERY **pView)
{
    MSIQUERY *query;
    UINT r;

    TRACE("%s %p\n", debugstr_w(szQuery), pView);

    if( !szQuery)
        return ERROR_INVALID_PARAMETER;

    /* pre allocate a handle to hold a pointer to the view */
    query = alloc_msiobject( MSIHANDLETYPE_VIEW, sizeof (MSIQUERY),
                              MSI_CloseView );
    if( !query )
        return ERROR_FUNCTION_FAILED;

    msiobj_addref( &db->hdr );
    query->row = 0;
    query->db = db;
    query->view = NULL;
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

UINT MSI_OpenQuery( MSIDATABASE *db, MSIQUERY **view, LPCWSTR fmt, ... )
{
    UINT r;
    int size = 100, res;
    LPWSTR query;

    /* construct the string */
    for (;;)
    {
        va_list va;
        query = msi_alloc( size*sizeof(WCHAR) );
        va_start(va, fmt);
        res = vsnprintfW(query, size, fmt, va);
        va_end(va);
        if (res == -1) size *= 2;
        else if (res >= size) size = res + 1;
        else break;
        msi_free( query );
    }
    /* perform the query */
    r = MSI_DatabaseOpenViewW(db, query, view);
    msi_free(query);
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
MSIRECORD *MSI_QueryGetRecord( MSIDATABASE *db, LPCWSTR fmt, ... )
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
        query = msi_alloc( size*sizeof(WCHAR) );
        va_start(va, fmt);
        res = vsnprintfW(query, size, fmt, va);
        va_end(va);
        if (res == -1) size *= 2;
        else if (res >= size) size = res + 1;
        else break;
        msi_free( query );
    }
    /* perform the query */
    r = MSI_DatabaseOpenViewW(db, query, &view);
    msi_free(query);

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

    db = msihandle2msiinfo( hdb, MSIHANDLETYPE_DATABASE );
    if( !db )
    {
        HRESULT hr;
        IWineMsiRemoteDatabase *remote_database;

        remote_database = (IWineMsiRemoteDatabase *)msi_get_remote( hdb );
        if ( !remote_database )
            return ERROR_INVALID_HANDLE;

        hr = IWineMsiRemoteDatabase_OpenView( remote_database, (BSTR)szQuery, phView );
        IWineMsiRemoteDatabase_Release( remote_database );

        if (FAILED(hr))
        {
            if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
                return HRESULT_CODE(hr);

            return ERROR_FUNCTION_FAILED;
        }

        return ERROR_SUCCESS;
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

UINT msi_view_get_row(MSIDATABASE *db, MSIVIEW *view, UINT row, MSIRECORD **rec)
{
    UINT row_count = 0, col_count = 0, i, ival, ret, type;

    TRACE("%p %p %d %p\n", db, view, row, rec);

    ret = view->ops->get_dimensions(view, &row_count, &col_count);
    if (ret)
        return ret;

    if (!col_count)
        return ERROR_INVALID_PARAMETER;

    if (row >= row_count)
        return ERROR_NO_MORE_ITEMS;

    *rec = MSI_CreateRecord(col_count);
    if (!*rec)
        return ERROR_FUNCTION_FAILED;

    for (i = 1; i <= col_count; i++)
    {
        ret = view->ops->get_column_info(view, i, NULL, &type, NULL);
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
                MSI_RecordSetIStream(*rec, i, stm);
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

        /* check if it's nul (0) - if so, don't set anything */
        if (!ival)
            continue;

        if (type & MSITYPE_STRING)
        {
            LPCWSTR sval;

            sval = msi_string_lookup_id(db->strings, ival);
            MSI_RecordSetStringW(*rec, i, sval);
        }
        else
        {
            if ((type & MSI_DATASIZEMASK) == 2)
                MSI_RecordSetInteger(*rec, i, ival - (1<<15));
            else
                MSI_RecordSetInteger(*rec, i, ival - (1<<31));
        }
    }

    return ERROR_SUCCESS;
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
        MSI_RecordSetInteger(*prec, 0, (int)query);
    }

    return r;
}

UINT WINAPI MsiViewFetch(MSIHANDLE hView, MSIHANDLE *record)
{
    MSIQUERY *query;
    MSIRECORD *rec = NULL;
    UINT ret;

    TRACE("%d %p\n", hView, record);

    if( !record )
        return ERROR_INVALID_PARAMETER;
    *record = 0;

    query = msihandle2msiinfo( hView, MSIHANDLETYPE_VIEW );
    if( !query )
        return ERROR_INVALID_HANDLE;
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

UINT WINAPI MsiViewClose(MSIHANDLE hView)
{
    MSIQUERY *query;
    UINT ret;

    TRACE("%d\n", hView );

    query = msihandle2msiinfo( hView, MSIHANDLETYPE_VIEW );
    if( !query )
        return ERROR_INVALID_HANDLE;

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

UINT WINAPI MsiViewExecute(MSIHANDLE hView, MSIHANDLE hRec)
{
    MSIQUERY *query;
    MSIRECORD *rec = NULL;
    UINT ret;
    
    TRACE("%d %d\n", hView, hRec);

    query = msihandle2msiinfo( hView, MSIHANDLETYPE_VIEW );
    if( !query )
        return ERROR_INVALID_HANDLE;

    if( hRec )
    {
        rec = msihandle2msiinfo( hRec, MSIHANDLETYPE_RECORD );
        if( !rec )
        {
            ret = ERROR_INVALID_HANDLE;
            goto out;
        }
    }

    msiobj_lock( &rec->hdr );
    ret = MSI_ViewExecute( query, rec );
    msiobj_unlock( &rec->hdr );

out:
    msiobj_release( &query->hdr );
    if( rec )
        msiobj_release( &rec->hdr );

    return ret;
}

static UINT msi_set_record_type_string( MSIRECORD *rec, UINT field,
                                        UINT type, BOOL temporary )
{
    static const WCHAR fmt[] = { '%','d',0 };
    WCHAR szType[0x10];

    if (MSITYPE_IS_BINARY(type))
        szType[0] = 'v';
    else if (type & MSITYPE_LOCALIZABLE)
        szType[0] = 'l';
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

    sprintfW( &szType[1], fmt, (type&0xff) );

    TRACE("type %04x -> %s\n", type, debugstr_w(szType) );

    return MSI_RecordSetStringW( rec, field, szType );
}

UINT MSI_ViewGetColumnInfo( MSIQUERY *query, MSICOLINFO info, MSIRECORD **prec )
{
    UINT r = ERROR_FUNCTION_FAILED, i, count = 0, type;
    MSIRECORD *rec;
    MSIVIEW *view = query->view;
    LPWSTR name;
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
        r = view->ops->get_column_info( view, i+1, &name, &type, &temporary );
        if( r != ERROR_SUCCESS )
            continue;
        if (info == MSICOLINFO_NAMES)
            MSI_RecordSetStringW( rec, i+1, name );
        else
            msi_set_record_type_string( rec, i+1, type, temporary );
        msi_free( name );
    }

    *prec = rec;
    return ERROR_SUCCESS;
}

UINT WINAPI MsiViewGetColumnInfo(MSIHANDLE hView, MSICOLINFO info, MSIHANDLE *hRec)
{
    MSIQUERY *query = NULL;
    MSIRECORD *rec = NULL;
    UINT r;

    TRACE("%d %d %p\n", hView, info, hRec);

    if( !hRec )
        return ERROR_INVALID_PARAMETER;

    if( info != MSICOLINFO_NAMES && info != MSICOLINFO_TYPES )
        return ERROR_INVALID_PARAMETER;

    query = msihandle2msiinfo( hView, MSIHANDLETYPE_VIEW );
    if( !query )
        return ERROR_INVALID_HANDLE;

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

    if ( mode == MSIMODIFY_UPDATE && MSI_RecordGetInteger( rec, 0 ) != (int)query )
        return ERROR_FUNCTION_FAILED;

    r = view->ops->modify( view, mode, rec, query->row );
    if (mode == MSIMODIFY_DELETE && r == ERROR_SUCCESS)
        query->row--;

    return r;
}

UINT WINAPI MsiViewModify( MSIHANDLE hView, MSIMODIFY eModifyMode,
                MSIHANDLE hRecord)
{
    MSIQUERY *query = NULL;
    MSIRECORD *rec = NULL;
    UINT r = ERROR_FUNCTION_FAILED;

    TRACE("%d %x %d\n", hView, eModifyMode, hRecord);

    query = msihandle2msiinfo( hView, MSIHANDLETYPE_VIEW );
    if( !query )
        return ERROR_INVALID_HANDLE;

    rec = msihandle2msiinfo( hRecord, MSIHANDLETYPE_RECORD );
    r = MSI_ViewModify( query, eModifyMode, rec );

    msiobj_release( &query->hdr );
    if( rec )
        msiobj_release( &rec->hdr );

    return r;
}

MSIDBERROR WINAPI MsiViewGetErrorW( MSIHANDLE handle, LPWSTR szColumnNameBuffer,
                              LPDWORD pcchBuf )
{
    MSIQUERY *query = NULL;
    static const WCHAR szError[] = { 0 };
    MSIDBERROR r = MSIDBERROR_NOERROR;
    DWORD len;

    FIXME("%d %p %p - returns empty error string\n",
          handle, szColumnNameBuffer, pcchBuf );

    if( !pcchBuf )
        return MSIDBERROR_INVALIDARG;

    query = msihandle2msiinfo( handle, MSIHANDLETYPE_VIEW );
    if( !query )
        return MSIDBERROR_INVALIDARG;

    len = strlenW( szError );
    if( szColumnNameBuffer )
    {
        if( *pcchBuf > len )
            lstrcpyW( szColumnNameBuffer, szError );
        else
            r = MSIDBERROR_MOREDATA;
    }
    *pcchBuf = len;

    msiobj_release( &query->hdr );
    return r;
}

MSIDBERROR WINAPI MsiViewGetErrorA( MSIHANDLE handle, LPSTR szColumnNameBuffer,
                              LPDWORD pcchBuf )
{
    static const CHAR szError[] = { 0 };
    MSIQUERY *query = NULL;
    MSIDBERROR r = MSIDBERROR_NOERROR;
    DWORD len;

    FIXME("%d %p %p - returns empty error string\n",
          handle, szColumnNameBuffer, pcchBuf );

    if( !pcchBuf )
        return MSIDBERROR_INVALIDARG;

    query = msihandle2msiinfo( handle, MSIHANDLETYPE_VIEW );
    if( !query )
        return MSIDBERROR_INVALIDARG;

    len = strlen( szError );
    if( szColumnNameBuffer )
    {
        if( *pcchBuf > len )
            lstrcpyA( szColumnNameBuffer, szError );
        else
            r = MSIDBERROR_MOREDATA;
    }
    *pcchBuf = len;

    msiobj_release( &query->hdr );
    return r;
}

MSIHANDLE WINAPI MsiGetLastErrorRecord( void )
{
    FIXME("\n");
    return 0;
}

DEFINE_GUID( CLSID_MsiTransform, 0x000c1082, 0x0000, 0x0000, 0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);

UINT MSI_DatabaseApplyTransformW( MSIDATABASE *db,
                 LPCWSTR szTransformFile, int iErrorCond )
{
    HRESULT r;
    UINT ret = ERROR_FUNCTION_FAILED;
    IStorage *stg = NULL;
    STATSTG stat;

    TRACE("%p %s %d\n", db, debugstr_w(szTransformFile), iErrorCond);

    r = StgOpenStorage( szTransformFile, NULL,
           STGM_DIRECT|STGM_READ|STGM_SHARE_DENY_WRITE, NULL, 0, &stg);
    if ( FAILED(r) )
        return ret;

    r = IStorage_Stat( stg, &stat, STATFLAG_NONAME );
    if ( FAILED( r ) )
        goto end;

    if ( !IsEqualGUID( &stat.clsid, &CLSID_MsiTransform ) )
        goto end;

    if( TRACE_ON( msi ) )
        enum_stream_names( stg );

    ret = msi_table_apply_transform( db, stg );

end:
    IStorage_Release( stg );

    return ret;
}

UINT WINAPI MsiDatabaseApplyTransformW( MSIHANDLE hdb,
                 LPCWSTR szTransformFile, int iErrorCond)
{
    MSIDATABASE *db;
    UINT r;

    db = msihandle2msiinfo( hdb, MSIHANDLETYPE_DATABASE );
    if( !db )
    {
        IWineMsiRemoteDatabase *remote_database;

        remote_database = (IWineMsiRemoteDatabase *)msi_get_remote( hdb );
        if ( !remote_database )
            return ERROR_INVALID_HANDLE;

        IWineMsiRemoteDatabase_Release( remote_database );
        WARN("MsiDatabaseApplyTransform not allowed during a custom action!\n");

        return ERROR_SUCCESS;
    }

    r = MSI_DatabaseApplyTransformW( db, szTransformFile, iErrorCond );
    msiobj_release( &db->hdr );
    return r;
}

UINT WINAPI MsiDatabaseApplyTransformA( MSIHANDLE hdb, 
                 LPCSTR szTransformFile, int iErrorCond)
{
    LPWSTR wstr;
    UINT ret;

    TRACE("%d %s %d\n", hdb, debugstr_a(szTransformFile), iErrorCond);

    wstr = strdupAtoW( szTransformFile );
    if( szTransformFile && !wstr )
        return ERROR_NOT_ENOUGH_MEMORY;

    ret = MsiDatabaseApplyTransformW( hdb, wstr, iErrorCond);

    msi_free( wstr );

    return ret;
}

UINT WINAPI MsiDatabaseGenerateTransformA( MSIHANDLE hdb, MSIHANDLE hdbref,
                 LPCSTR szTransformFile, int iReserved1, int iReserved2 )
{
    FIXME("%d %d %s %d %d\n", hdb, hdbref,
           debugstr_a(szTransformFile), iReserved1, iReserved2);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiDatabaseGenerateTransformW( MSIHANDLE hdb, MSIHANDLE hdbref,
                 LPCWSTR szTransformFile, int iReserved1, int iReserved2 )
{
    FIXME("%d %d %s %d %d\n", hdb, hdbref,
           debugstr_w(szTransformFile), iReserved1, iReserved2);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiDatabaseCommit( MSIHANDLE hdb )
{
    MSIDATABASE *db;
    UINT r;

    TRACE("%d\n", hdb);

    db = msihandle2msiinfo( hdb, MSIHANDLETYPE_DATABASE );
    if( !db )
    {
        IWineMsiRemoteDatabase *remote_database;

        remote_database = (IWineMsiRemoteDatabase *)msi_get_remote( hdb );
        if ( !remote_database )
            return ERROR_INVALID_HANDLE;

        IWineMsiRemoteDatabase_Release( remote_database );
        WARN("MsiDatabaseCommit not allowed during a custom action!\n");

        return ERROR_SUCCESS;
    }

    /* FIXME: lock the database */

    r = MSI_CommitTables( db );
    if (r != ERROR_SUCCESS) ERR("Failed to commit tables!\n");

    /* FIXME: unlock the database */

    msiobj_release( &db->hdr );

    if (r == ERROR_SUCCESS)
    {
        msi_free( db->deletefile );
        db->deletefile = NULL;
    }

    return r;
}

struct msi_primary_key_record_info
{
    DWORD n;
    MSIRECORD *rec;
};

static UINT msi_primary_key_iterator( MSIRECORD *rec, LPVOID param )
{
    struct msi_primary_key_record_info *info = param;
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

UINT MSI_DatabaseGetPrimaryKeys( MSIDATABASE *db,
                LPCWSTR table, MSIRECORD **prec )
{
    static const WCHAR sql[] = {
        's','e','l','e','c','t',' ','*',' ',
        'f','r','o','m',' ','`','_','C','o','l','u','m','n','s','`',' ',
        'w','h','e','r','e',' ',
        '`','T','a','b','l','e','`',' ','=',' ','\'','%','s','\'',0 };
    struct msi_primary_key_record_info info;
    MSIQUERY *query = NULL;
    UINT r;

    r = MSI_OpenQuery( db, &query, sql, table );
    if( r != ERROR_SUCCESS )
        return r;

    /* count the number of primary key records */
    info.n = 0;
    info.rec = 0;
    r = MSI_IterateRecords( query, 0, msi_primary_key_iterator, &info );
    if( r == ERROR_SUCCESS )
    {
        TRACE("Found %d primary keys\n", info.n );

        /* allocate a record and fill in the names of the tables */
        info.rec = MSI_CreateRecord( info.n );
        info.n = 0;
        r = MSI_IterateRecords( query, 0, msi_primary_key_iterator, &info );
        if( r == ERROR_SUCCESS )
            *prec = info.rec;
        else
            msiobj_release( &info.rec->hdr );
    }
    msiobj_release( &query->hdr );

    return r;
}

UINT WINAPI MsiDatabaseGetPrimaryKeysW( MSIHANDLE hdb,
                    LPCWSTR table, MSIHANDLE* phRec )
{
    MSIRECORD *rec = NULL;
    MSIDATABASE *db;
    UINT r;

    TRACE("%d %s %p\n", hdb, debugstr_w(table), phRec);

    db = msihandle2msiinfo( hdb, MSIHANDLETYPE_DATABASE );
    if( !db )
    {
        HRESULT hr;
        IWineMsiRemoteDatabase *remote_database;

        remote_database = (IWineMsiRemoteDatabase *)msi_get_remote( hdb );
        if ( !remote_database )
            return ERROR_INVALID_HANDLE;

        hr = IWineMsiRemoteDatabase_GetPrimaryKeys( remote_database, (BSTR)table, phRec );
        IWineMsiRemoteDatabase_Release( remote_database );

        if (FAILED(hr))
        {
            if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
                return HRESULT_CODE(hr);

            return ERROR_FUNCTION_FAILED;
        }

        return ERROR_SUCCESS;
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

UINT WINAPI MsiDatabaseGetPrimaryKeysA(MSIHANDLE hdb, 
                    LPCSTR table, MSIHANDLE* phRec)
{
    LPWSTR szwTable = NULL;
    UINT r;

    TRACE("%d %s %p\n", hdb, debugstr_a(table), phRec);

    if( table )
    {
        szwTable = strdupAtoW( table );
        if( !szwTable )
            return ERROR_OUTOFMEMORY;
    }
    r = MsiDatabaseGetPrimaryKeysW( hdb, szwTable, phRec );
    msi_free( szwTable );

    return r;
}

MSICONDITION WINAPI MsiDatabaseIsTablePersistentA(
              MSIHANDLE hDatabase, LPCSTR szTableName)
{
    LPWSTR szwTableName = NULL;
    MSICONDITION r;

    TRACE("%x %s\n", hDatabase, debugstr_a(szTableName));

    if( szTableName )
    {
        szwTableName = strdupAtoW( szTableName );
        if( !szwTableName )
            return MSICONDITION_ERROR;
    }
    r = MsiDatabaseIsTablePersistentW( hDatabase, szwTableName );
    msi_free( szwTableName );

    return r;
}

MSICONDITION WINAPI MsiDatabaseIsTablePersistentW(
              MSIHANDLE hDatabase, LPCWSTR szTableName)
{
    MSIDATABASE *db;
    MSICONDITION r;

    TRACE("%x %s\n", hDatabase, debugstr_w(szTableName));

    db = msihandle2msiinfo( hDatabase, MSIHANDLETYPE_DATABASE );
    if( !db )
    {
        HRESULT hr;
        MSICONDITION condition;
        IWineMsiRemoteDatabase *remote_database;

        remote_database = (IWineMsiRemoteDatabase *)msi_get_remote( hDatabase );
        if ( !remote_database )
            return MSICONDITION_ERROR;

        hr = IWineMsiRemoteDatabase_IsTablePersistent( remote_database,
                                                       (BSTR)szTableName, &condition );
        IWineMsiRemoteDatabase_Release( remote_database );

        if (FAILED(hr))
            return MSICONDITION_ERROR;

        return condition;
    }

    r = MSI_DatabaseIsTablePersistent( db, szTableName );

    msiobj_release( &db->hdr );

    return r;
}
