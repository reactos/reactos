/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2002-2004 Mike McCormack for CodeWeavers
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

WINE_DEFAULT_DEBUG_CHANNEL(msi);

#if 0
typedef struct tagMSIQUERY
{
    MSIOBJECTHDR hdr;
    MSIVIEW *view;
    UINT row;
    MSIDATABASE *db;
} MSIQUERY;
#endif

UINT WINAPI MsiDatabaseIsTablePersistentA(
              MSIHANDLE hDatabase, LPSTR szTableName)
{
    FIXME("%lx %s\n", hDatabase, debugstr_a(szTableName));
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiDatabaseIsTablePersistentW(
              MSIHANDLE hDatabase, LPWSTR szTableName)
{
    FIXME("%lx %s\n", hDatabase, debugstr_w(szTableName));
    return ERROR_CALL_NOT_IMPLEMENTED;
}

void MSI_CloseView( MSIOBJECTHDR *arg )
{
    MSIQUERY *query = (MSIQUERY*) arg;

    if( query->view && query->view->ops->delete )
        query->view->ops->delete( query->view );
    msiobj_release( &query->db->hdr );
}

UINT VIEW_find_column( MSIVIEW *table, LPWSTR name, UINT *n )
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
        r = table->ops->get_column_info( table, i, &col_name, NULL );
        if( r != ERROR_SUCCESS )
            return r;
        x = lstrcmpW( name, col_name );
        HeapFree( GetProcessHeap(), 0, col_name );
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

    TRACE("%ld %s %p\n", hdb, debugstr_a(szQuery), phView);

    if( szQuery )
    {
        UINT len = MultiByteToWideChar( CP_ACP, 0, szQuery, -1, NULL, 0 );
        szwQuery = HeapAlloc( GetProcessHeap(), 0, len*sizeof(WCHAR) );
        if( !szwQuery )
            return ERROR_FUNCTION_FAILED;
        MultiByteToWideChar( CP_ACP, 0, szQuery, -1, szwQuery, len );
    }
    else
        szwQuery = NULL;

    r = MsiDatabaseOpenViewW( hdb, szwQuery, phView);

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

    r = MSI_ParseSQL( db, szQuery, &query->view );
    if( r == ERROR_SUCCESS )
    {
        msiobj_addref( &query->hdr );
        *pView = query;
    }

    msiobj_release( &query->hdr );
    return r;
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
        return ERROR_INVALID_HANDLE;

    ret = MSI_DatabaseOpenViewW( db, szQuery, &query );
    if( ret == ERROR_SUCCESS )
    {
        *phView = alloc_msihandle( &query->hdr );
        msiobj_release( &query->hdr );
    }
    msiobj_release( &db->hdr );

    return ret;
}

UINT MSI_ViewFetch(MSIQUERY *query, MSIRECORD **prec)
{
    MSIVIEW *view;
    MSIRECORD *rec;
    UINT row_count = 0, col_count = 0, i, ival, ret, type;

    TRACE("%p %p\n", query, prec );

    view = query->view;
    if( !view )
        return ERROR_FUNCTION_FAILED;

    ret = view->ops->get_dimensions( view, &row_count, &col_count );
    if( ret )
        return ret;
    if( !col_count )
        return ERROR_INVALID_PARAMETER;

    if( query->row >= row_count )
        return ERROR_NO_MORE_ITEMS;

    rec = MSI_CreateRecord( col_count );
    if( !rec )
        return ERROR_FUNCTION_FAILED;

    for( i=1; i<=col_count; i++ )
    {
        ret = view->ops->get_column_info( view, i, NULL, &type );
        if( ret )
        {
            ERR("Error getting column type for %d\n", i );
            continue;
        }
        if (( type != MSITYPE_BINARY) && (type != (MSITYPE_BINARY |
                                                   MSITYPE_NULLABLE)))
        {
            ret = view->ops->fetch_int( view, query->row, i, &ival );
            if( ret )
            {
                ERR("Error fetching data for %d\n", i );
                continue;
            }
            if( ! (type & MSITYPE_VALID ) )
                ERR("Invalid type!\n");

            /* check if it's nul (0) - if so, don't set anything */
            if( !ival )
                continue;

            if( type & MSITYPE_STRING )
            {
                LPWSTR sval;

                sval = MSI_makestring( query->db, ival );
                MSI_RecordSetStringW( rec, i, sval );
                HeapFree( GetProcessHeap(), 0, sval );
            }
            else
            {
                if( (type & MSI_DATASIZEMASK) == 2 )
                    MSI_RecordSetInteger( rec, i, ival - (1<<15) );
                else
                    MSI_RecordSetInteger( rec, i, ival - (1<<31) );
            }
        }
        else
        {
            IStream *stm = NULL;

            ret = view->ops->fetch_stream( view, query->row, i, &stm );
            if( ( ret == ERROR_SUCCESS ) && stm )
            {
                MSI_RecordSetIStream( rec, i, stm );
                IStream_Release( stm );
            }
            else
                ERR("failed to get stream\n");
        }
    }
    query->row ++;

    *prec = rec;

    return ERROR_SUCCESS;
}

UINT WINAPI MsiViewFetch(MSIHANDLE hView, MSIHANDLE *record)
{
    MSIQUERY *query;
    MSIRECORD *rec = NULL;
    UINT ret;

    TRACE("%ld %p\n", hView, record);

    query = msihandle2msiinfo( hView, MSIHANDLETYPE_VIEW );
    if( !query )
        return ERROR_INVALID_HANDLE;
    ret = MSI_ViewFetch( query, &rec );
    if( ret == ERROR_SUCCESS )
    {
        *record = alloc_msihandle( &rec->hdr );
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

    TRACE("%ld\n", hView );

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
    
    TRACE("%ld %ld\n", hView, hRec);

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

    ret = MSI_ViewExecute( query, rec );
out:
    if( query )
        msiobj_release( &query->hdr );
    if( rec )
        msiobj_release( &rec->hdr );

    return ret;
}

UINT WINAPI MsiViewGetColumnInfo(MSIHANDLE hView, MSICOLINFO info, MSIHANDLE *hRec)
{
    MSIVIEW *view;
    MSIQUERY *query;
    MSIHANDLE handle;
    UINT ret, i, count = 0, type;
    LPWSTR name;

    TRACE("%ld %d %p\n", hView, info, hRec);

    query = msihandle2msiinfo( hView, MSIHANDLETYPE_VIEW );
    if( !query )
        return ERROR_INVALID_HANDLE;

    view = query->view;
    if( !view )
        return ERROR_FUNCTION_FAILED;

    if( !view->ops->get_dimensions )
        return ERROR_FUNCTION_FAILED;

    ret = view->ops->get_dimensions( view, NULL, &count );
    if( ret )
        return ret;
    if( !count )
        return ERROR_INVALID_PARAMETER;

    handle = MsiCreateRecord( count );
    if( !handle )
        return ERROR_FUNCTION_FAILED;

    for( i=0; i<count; i++ )
    {
        name = NULL;
        ret = view->ops->get_column_info( view, i+1, &name, &type );
        if( ret != ERROR_SUCCESS )
            continue;
        MsiRecordSetStringW( handle, i+1, name );
        HeapFree( GetProcessHeap(), 0, name );
    }

    *hRec = handle;

    return ERROR_SUCCESS;
}

UINT WINAPI MsiDatabaseApplyTransformA( MSIHANDLE hdb, 
                 LPCSTR szTransformFile, int iErrorCond)
{
    FIXME("%ld %s %d\n", hdb, debugstr_a(szTransformFile), iErrorCond);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiDatabaseApplyTransformW( MSIHANDLE hdb, 
                 LPCWSTR szTransformFile, int iErrorCond)
{
    FIXME("%ld %s %d\n", hdb, debugstr_w(szTransformFile), iErrorCond);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiDatabaseGenerateTransformA( MSIHANDLE hdb, MSIHANDLE hdbref,
                 LPCSTR szTransformFile, int iReserved1, int iReserved2 )
{
    FIXME("%ld %ld %s %d %d\n", hdb, hdbref, 
           debugstr_a(szTransformFile), iReserved1, iReserved2);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiDatabaseGenerateTransformW( MSIHANDLE hdb, MSIHANDLE hdbref,
                 LPCWSTR szTransformFile, int iReserved1, int iReserved2 )
{
    FIXME("%ld %ld %s %d %d\n", hdb, hdbref, 
           debugstr_w(szTransformFile), iReserved1, iReserved2);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiDatabaseCommit( MSIHANDLE hdb )
{
    MSIDATABASE *db;
    UINT r;

    TRACE("%ld\n", hdb);

    db = msihandle2msiinfo( hdb, MSIHANDLETYPE_DATABASE );
    if( !db )
        return ERROR_INVALID_HANDLE;

    /* FIXME: lock the database */

    r = MSI_CommitTables( db );

    /* FIXME: unlock the database */

    return r;
}

UINT WINAPI MsiDatabaseGetPrimaryKeysA(MSIHANDLE hdb, 
                    LPCSTR table, MSIHANDLE* rec)
{
    FIXME("%ld %s %p\n", hdb, debugstr_a(table), rec);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiDatabaseGetPrimaryKeysW(MSIHANDLE hdb,
                    LPCWSTR table, MSIHANDLE* rec)
{
    FIXME("%ld %s %p\n", hdb, debugstr_w(table), rec);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiViewModify(MSIHANDLE hView, MSIMODIFY eModifyMode, MSIHANDLE
hRecord)
{
    FIXME("%ld %x %ld\n",hView, eModifyMode, hRecord);
    return ERROR_CALL_NOT_IMPLEMENTED;
}
