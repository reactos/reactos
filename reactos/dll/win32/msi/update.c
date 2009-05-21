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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
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

WINE_DEFAULT_DEBUG_CHANNEL(msidb);


/* below is the query interface to a table */

typedef struct tagMSIUPDATEVIEW
{
    MSIVIEW          view;
    MSIDATABASE     *db;
    MSIVIEW         *wv;
    column_info     *vals;
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
    UINT i, r, col_count = 0, row_count = 0;
    MSIRECORD *values = NULL;
    MSIRECORD *where = NULL;
    MSIVIEW *wv;
    UINT cols_count, where_count;
    column_info *col = uv->vals;

    TRACE("%p %p\n", uv, record );

    /* extract the where markers from the record */
    if (record)
    {
        r = MSI_RecordGetFieldCount(record);

        for (i = 0; col; col = col->next)
            i++;

        cols_count = i;
        where_count = r - i;

        if (where_count > 0)
        {
            where = MSI_CreateRecord(where_count);

            if (where)
                for (i = 1; i <= where_count; i++)
                    MSI_RecordCopyField(record, cols_count + i, where, i);
        }
    }

    wv = uv->wv;
    if( !wv )
    {
        r = ERROR_FUNCTION_FAILED;
        goto done;
    }

    r = wv->ops->execute( wv, where );
    TRACE("tv execute returned %x\n", r);
    if( r )
        goto done;

    r = wv->ops->get_dimensions( wv, &row_count, &col_count );
    if( r )
        goto done;

    values = msi_query_merge_record( col_count, uv->vals, record );
    if (!values)
    {
        r = ERROR_FUNCTION_FAILED;
        goto done;
    }

    for ( i=0; i<row_count; i++ )
    {
        r = wv->ops->set_row( wv, i, values, (1 << col_count) - 1 );
        if (r != ERROR_SUCCESS)
            break;
    }

done:
    if ( where ) msiobj_release( &where->hdr );
    if ( values ) msiobj_release( &values->hdr );

    return r;
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
                UINT n, LPWSTR *name, UINT *type, BOOL *temporary )
{
    MSIUPDATEVIEW *uv = (MSIUPDATEVIEW*)view;
    MSIVIEW *wv;

    TRACE("%p %d %p %p %p\n", uv, n, name, type, temporary );

    wv = uv->wv;
    if( !wv )
        return ERROR_FUNCTION_FAILED;

    return wv->ops->get_column_info( wv, n, name, type, temporary );
}

static UINT UPDATE_modify( struct tagMSIVIEW *view, MSIMODIFY eModifyMode,
                           MSIRECORD *rec, UINT row )
{
    MSIUPDATEVIEW *uv = (MSIUPDATEVIEW*)view;

    TRACE("%p %d %p\n", uv, eModifyMode, rec );

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
    msiobj_release( &uv->db->hdr );
    msi_free( uv );

    return ERROR_SUCCESS;
}

static UINT UPDATE_find_matching_rows( struct tagMSIVIEW *view, UINT col, UINT val, UINT *row, MSIITERHANDLE *handle )
{
    TRACE("%p %d %d %p\n", view, col, val, *handle );

    return ERROR_FUNCTION_FAILED;
}


static const MSIVIEWOPS update_ops =
{
    UPDATE_fetch_int,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    UPDATE_execute,
    UPDATE_close,
    UPDATE_get_dimensions,
    UPDATE_get_column_info,
    UPDATE_modify,
    UPDATE_delete,
    UPDATE_find_matching_rows,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};

UINT UPDATE_CreateView( MSIDATABASE *db, MSIVIEW **view, LPCWSTR table,
                        column_info *columns, struct expr *expr )
{
    MSIUPDATEVIEW *uv = NULL;
    UINT r;
    MSIVIEW *tv = NULL, *sv = NULL, *wv = NULL;

    TRACE("%p\n", uv );

    r = TABLE_CreateView( db, table, &tv );
    if( r != ERROR_SUCCESS )
        return r;

    if (expr)
    {
        /* add conditions first */
        r = WHERE_CreateView( db, &wv, tv, expr );
        if( r != ERROR_SUCCESS )
        {
            tv->ops->delete( tv );
            return r;
        }
    }
    else
       wv = tv;

    /* then select the columns we want */
    r = SELECT_CreateView( db, &sv, wv, columns );
    if( r != ERROR_SUCCESS )
    {
        wv->ops->delete( wv );
        return r;
    }

    uv = msi_alloc_zero( sizeof *uv );
    if( !uv )
        return ERROR_FUNCTION_FAILED;

    /* fill the structure */
    uv->view.ops = &update_ops;
    msiobj_addref( &db->hdr );
    uv->db = db;
    uv->vals = columns;
    uv->wv = sv;
    *view = (MSIVIEW*) uv;

    return ERROR_SUCCESS;
}
