/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2004 Mike McCormack for CodeWeavers
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
 * You should have receuved a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wine/debug.h"
#include "msi.h"
#include "msiquery.h"
#include "objbase.h"
#include "objidl.h"
#include "msipriv.h"
#include "winnls.h"

#include "query.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);


/* below is the query interface to a table */

typedef struct tagMSIUPDATEVIEW
{
    MSIVIEW          view;
    MSIDATABASE     *db;
    MSIVIEW         *wv;
    value_list      *vals;
} MSIUPDATEVIEW;

static UINT UPDATE_fetch_int( struct tagMSIVIEW *view, UINT row, UINT col, UINT *val )
{
    MSIUPDATEVIEW *uv = (MSIUPDATEVIEW*)view;

    TRACE("%p %d %d %p\n", uv, row, col, val );

    return ERROR_FUNCTION_FAILED;
}

static UINT UPDATE_execute( struct tagMSIVIEW *view, MSIRECORD *record )
{
    MSIUPDATEVIEW *uv = (MSIUPDATEVIEW*)view;
    UINT n, type, val, r, row, col_count = 0, row_count = 0;
    MSIVIEW *wv;

    TRACE("%p %p\n", uv, record );

    if( !record )
        return ERROR_FUNCTION_FAILED;

    wv = uv->wv;
    if( !wv )
        return ERROR_FUNCTION_FAILED;

    r = wv->ops->execute( wv, 0 );
    TRACE("tv execute returned %x\n", r);
    if( r )
        return r;

    r = wv->ops->get_dimensions( wv, &row_count, &col_count );
    if( r )
        goto err;

    for( row = 0; row < row_count; row++ )
    {
        for( n = 1; n <= col_count; n++ )
        {
            r = wv->ops->get_column_info( wv, n, NULL, &type );
            if( r )
                break;

            if( type & MSITYPE_STRING )
            {
                const WCHAR *str = MSI_RecordGetString( record, n );
                val = msi_addstringW( uv->db->strings, 0, str, -1, 1 );
            }
            else
            {
                val = MSI_RecordGetInteger( record, n );
                val |= 0x8000;
            }
            r = wv->ops->set_int( wv, row, n, val );
            if( r )
                break;
        }
    }

err:
    return ERROR_SUCCESS;
}


static UINT UPDATE_close( struct tagMSIVIEW *view )
{
    MSIUPDATEVIEW *uv = (MSIUPDATEVIEW*)view;
    MSIVIEW *wv;

    TRACE("%p\n", uv);

    wv = uv->wv;
    if( !wv )
        return ERROR_FUNCTION_FAILED;

    return wv->ops->close( wv );
}

static UINT UPDATE_get_dimensions( struct tagMSIVIEW *view, UINT *rows, UINT *cols )
{
    MSIUPDATEVIEW *uv = (MSIUPDATEVIEW*)view;
    MSIVIEW *wv;

    TRACE("%p %p %p\n", uv, rows, cols );

    wv = uv->wv;
    if( !wv )
        return ERROR_FUNCTION_FAILED;

    return wv->ops->get_dimensions( wv, rows, cols );
}

static UINT UPDATE_get_column_info( struct tagMSIVIEW *view,
                UINT n, LPWSTR *name, UINT *type )
{
    MSIUPDATEVIEW *uv = (MSIUPDATEVIEW*)view;
    MSIVIEW *wv;

    TRACE("%p %d %p %p\n", uv, n, name, type );

    wv = uv->wv;
    if( !wv )
        return ERROR_FUNCTION_FAILED;

    return wv->ops->get_column_info( wv, n, name, type );
}

static UINT UPDATE_modify( struct tagMSIVIEW *view, MSIMODIFY eModifyMode, MSIHANDLE hrec)
{
    MSIUPDATEVIEW *uv = (MSIUPDATEVIEW*)view;

    TRACE("%p %d %ld\n", uv, eModifyMode, hrec );

    return ERROR_FUNCTION_FAILED;
}

static UINT UPDATE_delete( struct tagMSIVIEW *view )
{
    MSIUPDATEVIEW *uv = (MSIUPDATEVIEW*)view;
    MSIVIEW *wv;

    TRACE("%p\n", uv );

    wv = uv->wv;
    if( wv )
        wv->ops->delete( wv );
    delete_value_list( uv->vals );
    msiobj_release( &uv->db->hdr );
    HeapFree( GetProcessHeap(), 0, uv );

    return ERROR_SUCCESS;
}


static MSIVIEWOPS update_ops =
{
    UPDATE_fetch_int,
    NULL,
    NULL,
    NULL,
    UPDATE_execute,
    UPDATE_close,
    UPDATE_get_dimensions,
    UPDATE_get_column_info,
    UPDATE_modify,
    UPDATE_delete
};

UINT UPDATE_CreateView( MSIDATABASE *db, MSIVIEW **view, LPWSTR table,
                        column_assignment *list, struct expr *expr )
{
    MSIUPDATEVIEW *uv = NULL;
    UINT r;
    MSIVIEW *tv = NULL, *sv = NULL, *wv = NULL;

    TRACE("%p\n", uv );

    r = TABLE_CreateView( db, table, &tv );
    if( r != ERROR_SUCCESS )
        return r;

    /* add conditions first */
    r = WHERE_CreateView( db, &wv, tv, expr );
    if( r != ERROR_SUCCESS )
    {
        if( sv )
            sv->ops->delete( tv );
        return r;
    }
    
    /* then select the columns we want */
    r = SELECT_CreateView( db, &sv, wv, list->col_list );
    if( r != ERROR_SUCCESS )
    {
        if( tv )
            tv->ops->delete( sv );
        return r;
    }

    uv = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof *uv );
    if( !uv )
        return ERROR_FUNCTION_FAILED;

    /* fill the structure */
    uv->view.ops = &update_ops;
    msiobj_addref( &db->hdr );
    uv->db = db;
    uv->vals = list->val_list;
    uv->wv = sv;
    *view = (MSIVIEW*) uv;

    return ERROR_SUCCESS;
}
