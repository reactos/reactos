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

typedef struct tagMSIINSERTVIEW
{
    MSIVIEW          view;
    MSIDATABASE     *db;
    BOOL             bIsTemp;
    MSIVIEW         *sv;
    value_list      *vals;   /* looks like these may be ignored... */
} MSIINSERTVIEW;

static UINT INSERT_fetch_int( struct tagMSIVIEW *view, UINT row, UINT col, UINT *val )
{
    MSIINSERTVIEW *iv = (MSIINSERTVIEW*)view;

    TRACE("%p %d %d %p\n", iv, row, col, val );

    return ERROR_FUNCTION_FAILED;
}

static UINT INSERT_execute( struct tagMSIVIEW *view, MSIRECORD *record )
{
    MSIINSERTVIEW *iv = (MSIINSERTVIEW*)view;
    UINT n, type, val, r, row, col_count = 0;
    MSIVIEW *sv;

    TRACE("%p %p\n", iv, record );

    sv = iv->sv;
    if( !sv )
        return ERROR_FUNCTION_FAILED;

    r = sv->ops->execute( sv, 0 );
    TRACE("tv execute returned %x\n", r);
    if( r )
        return r;

    r = sv->ops->get_dimensions( sv, NULL, &col_count );
    if( r )
        goto err;

    n = MSI_RecordGetFieldCount( record );
    if( n != col_count )
    {
        ERR("Number of fields do not match\n");
        goto err;
    }

    row = -1;
    r = sv->ops->insert_row( sv, &row );
    TRACE("insert_row returned %x\n", r);
    if( r )
        goto err;

    for( n = 1; n <= col_count; n++ )
    {
        r = sv->ops->get_column_info( sv, n, NULL, &type );
        if( r )
            break;

        if( type & MSITYPE_STRING )
        {
            const WCHAR *str = MSI_RecordGetString( record, n );
            val = msi_addstringW( iv->db->strings, 0, str, -1, 1 );
        }
        else
        {
            val = MSI_RecordGetInteger( record, n );
            val |= 0x8000;
        }
        r = sv->ops->set_int( sv, row, n, val );
        if( r )
            break;
    }

err:
    return ERROR_SUCCESS;
}


static UINT INSERT_close( struct tagMSIVIEW *view )
{
    MSIINSERTVIEW *iv = (MSIINSERTVIEW*)view;
    MSIVIEW *sv;

    TRACE("%p\n", iv);

    sv = iv->sv;
    if( !sv )
        return ERROR_FUNCTION_FAILED;

    return sv->ops->close( sv );
}

static UINT INSERT_get_dimensions( struct tagMSIVIEW *view, UINT *rows, UINT *cols )
{
    MSIINSERTVIEW *iv = (MSIINSERTVIEW*)view;
    MSIVIEW *sv;

    TRACE("%p %p %p\n", iv, rows, cols );

    sv = iv->sv;
    if( !sv )
        return ERROR_FUNCTION_FAILED;

    return sv->ops->get_dimensions( sv, rows, cols );
}

static UINT INSERT_get_column_info( struct tagMSIVIEW *view,
                UINT n, LPWSTR *name, UINT *type )
{
    MSIINSERTVIEW *iv = (MSIINSERTVIEW*)view;
    MSIVIEW *sv;

    TRACE("%p %d %p %p\n", iv, n, name, type );

    sv = iv->sv;
    if( !sv )
        return ERROR_FUNCTION_FAILED;

    return sv->ops->get_column_info( sv, n, name, type );
}

static UINT INSERT_modify( struct tagMSIVIEW *view, MSIMODIFY eModifyMode, MSIHANDLE hrec)
{
    MSIINSERTVIEW *iv = (MSIINSERTVIEW*)view;

    TRACE("%p %d %ld\n", iv, eModifyMode, hrec );

    return ERROR_FUNCTION_FAILED;
}

static UINT INSERT_delete( struct tagMSIVIEW *view )
{
    MSIINSERTVIEW *iv = (MSIINSERTVIEW*)view;
    MSIVIEW *sv;

    TRACE("%p\n", iv );

    sv = iv->sv;
    if( sv )
        sv->ops->delete( sv );
    delete_value_list( iv->vals );
    msiobj_release( &iv->db->hdr );
    HeapFree( GetProcessHeap(), 0, iv );

    return ERROR_SUCCESS;
}


MSIVIEWOPS insert_ops =
{
    INSERT_fetch_int,
    NULL,
    NULL,
    NULL,
    INSERT_execute,
    INSERT_close,
    INSERT_get_dimensions,
    INSERT_get_column_info,
    INSERT_modify,
    INSERT_delete
};

UINT INSERT_CreateView( MSIDATABASE *db, MSIVIEW **view, LPWSTR table,
                        string_list *columns, value_list *values, BOOL temp )
{
    MSIINSERTVIEW *iv = NULL;
    UINT r;
    MSIVIEW *tv = NULL, *sv = NULL;

    TRACE("%p\n", iv );

    r = TABLE_CreateView( db, table, &tv );
    if( r != ERROR_SUCCESS )
        return r;

    r = SELECT_CreateView( db, &sv, tv, columns );
    if( r != ERROR_SUCCESS )
    {
        if( tv )
            tv->ops->delete( tv );
        return r;
    }
    
    iv = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof *iv );
    if( !iv )
        return ERROR_FUNCTION_FAILED;

    /* fill the structure */
    iv->view.ops = &insert_ops;
    msiobj_addref( &db->hdr );
    iv->db = db;
    iv->vals = values;
    iv->bIsTemp = temp;
    iv->sv = sv;
    *view = (MSIVIEW*) iv;

    return ERROR_SUCCESS;
}
