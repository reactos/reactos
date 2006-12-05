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

typedef struct tagMSIINSERTVIEW
{
    MSIVIEW          view;
    MSIDATABASE     *db;
    BOOL             bIsTemp;
    MSIVIEW         *sv;
    column_info     *vals;
} MSIINSERTVIEW;

static UINT INSERT_fetch_int( struct tagMSIVIEW *view, UINT row, UINT col, UINT *val )
{
    MSIINSERTVIEW *iv = (MSIINSERTVIEW*)view;

    TRACE("%p %d %d %p\n", iv, row, col, val );

    return ERROR_FUNCTION_FAILED;
}

/*
 * msi_query_merge_record
 *
 * Merge a value_list and a record to create a second record.
 * Replace wildcard entries in the valuelist with values from the record
 */
MSIRECORD *msi_query_merge_record( UINT fields, column_info *vl, MSIRECORD *rec )
{
    MSIRECORD *merged;
    DWORD wildcard_count = 1, i;

    merged = MSI_CreateRecord( fields );
    for( i=1; i <= fields; i++ )
    {
        if( !vl )
        {
            TRACE("Not enough elements in the list to insert\n");
            goto err;
        }
        switch( vl->val->type )
        {
        case EXPR_SVAL:
            TRACE("field %d -> %s\n", i, debugstr_w(vl->val->u.sval));
            MSI_RecordSetStringW( merged, i, vl->val->u.sval );
            break;
        case EXPR_IVAL:
            MSI_RecordSetInteger( merged, i, vl->val->u.ival );
            break;
        case EXPR_WILDCARD:
            if( !rec )
                goto err;
            MSI_RecordCopyField( rec, wildcard_count, merged, i );
            wildcard_count++;
            break;
        default:
            ERR("Unknown expression type %d\n", vl->val->type);
        }
        vl = vl->next;
    }

    return merged;
err:
    msiobj_release( &merged->hdr );
    return NULL;
}

static UINT INSERT_execute( struct tagMSIVIEW *view, MSIRECORD *record )
{
    MSIINSERTVIEW *iv = (MSIINSERTVIEW*)view;
    UINT r, col_count = 0;
    MSIVIEW *sv;
    MSIRECORD *values = NULL;

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

    /*
     * Merge the wildcard values into the list of values provided
     * in the query, and create a record containing both.
     */
    values = msi_query_merge_record( col_count, iv->vals, record );
    if( !values )
        goto err;

    r = sv->ops->insert_row( sv, values );

err:
    if( values )
        msiobj_release( &values->hdr );

    return r;
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

static UINT INSERT_modify( struct tagMSIVIEW *view, MSIMODIFY eModifyMode, MSIRECORD *rec)
{
    MSIINSERTVIEW *iv = (MSIINSERTVIEW*)view;

    TRACE("%p %d %p\n", iv, eModifyMode, rec );

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
    msiobj_release( &iv->db->hdr );
    msi_free( iv );

    return ERROR_SUCCESS;
}

static UINT INSERT_find_matching_rows( struct tagMSIVIEW *view, UINT col,
    UINT val, UINT *row, MSIITERHANDLE *handle )
{
    TRACE("%p, %d, %u, %p\n", view, col, val, *handle);

    return ERROR_FUNCTION_FAILED;
}


static const MSIVIEWOPS insert_ops =
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
    INSERT_delete,
    INSERT_find_matching_rows
};

static UINT count_column_info( column_info *ci )
{
    UINT n = 0;
    for ( ; ci; ci = ci->next )
        n++;
    return n;
}

UINT INSERT_CreateView( MSIDATABASE *db, MSIVIEW **view, LPWSTR table,
                        column_info *columns, column_info *values, BOOL temp )
{
    MSIINSERTVIEW *iv = NULL;
    UINT r;
    MSIVIEW *tv = NULL, *sv = NULL;

    TRACE("%p\n", iv );

    /* there should be one value for each column */
    if ( count_column_info( columns ) != count_column_info(values) )
        return ERROR_BAD_QUERY_SYNTAX;

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

    iv = msi_alloc_zero( sizeof *iv );
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
