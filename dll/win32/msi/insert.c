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

struct insert_view
{
    MSIVIEW          view;
    MSIVIEW         *table;
    MSIDATABASE     *db;
    BOOL             bIsTemp;
    MSIVIEW         *sv;
    column_info     *vals;
};

static UINT INSERT_fetch_int( struct tagMSIVIEW *view, UINT row, UINT col, UINT *val )
{
    struct insert_view *iv = (struct insert_view *)view;

    TRACE("%p %d %d %p\n", iv, row, col, val );

    return ERROR_FUNCTION_FAILED;
}

/*
 * msi_query_merge_record
 *
 * Merge a value_list and a record to create a second record.
 * Replace wildcard entries in the valuelist with values from the record
 */
MSIRECORD *msi_query_merge_record( UINT fields, const column_info *vl, MSIRECORD *rec )
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
            TRACE( "field %lu -> %s\n", i, debugstr_w(vl->val->u.sval) );
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

/* checks to see if the column order specified in the INSERT query
 * matches the column order of the table
 */
static BOOL columns_in_order(struct insert_view *iv, UINT col_count)
{
    LPCWSTR a, b;
    UINT i;

    for (i = 1; i <= col_count; i++)
    {
        iv->sv->ops->get_column_info(iv->sv, i, &a, NULL, NULL, NULL);
        iv->table->ops->get_column_info(iv->table, i, &b, NULL, NULL, NULL);

        if (wcscmp( a, b )) return FALSE;
    }
    return TRUE;
}

/* rearranges the data in the record to be inserted based on column order,
 * and pads the record for any missing columns in the INSERT query
 */
static UINT arrange_record(struct insert_view *iv, MSIRECORD **values)
{
    MSIRECORD *padded;
    UINT col_count, val_count;
    UINT r, i, colidx;
    LPCWSTR a, b;

    r = iv->table->ops->get_dimensions(iv->table, NULL, &col_count);
    if (r != ERROR_SUCCESS)
        return r;

    val_count = MSI_RecordGetFieldCount(*values);

    /* check to see if the columns are arranged already
     * to avoid unnecessary copying
     */
    if (col_count == val_count && columns_in_order(iv, col_count))
        return ERROR_SUCCESS;

    padded = MSI_CreateRecord(col_count);
    if (!padded)
        return ERROR_OUTOFMEMORY;

    for (colidx = 1; colidx <= val_count; colidx++)
    {
        r = iv->sv->ops->get_column_info(iv->sv, colidx, &a, NULL, NULL, NULL);
        if (r != ERROR_SUCCESS)
            goto err;

        for (i = 1; i <= col_count; i++)
        {
            r = iv->table->ops->get_column_info(iv->table, i, &b, NULL,
                                                NULL, NULL);
            if (r != ERROR_SUCCESS)
                goto err;

            if (!wcscmp( a, b ))
            {
                MSI_RecordCopyField(*values, colidx, padded, i);
                break;
            }
        }
    }
    msiobj_release(&(*values)->hdr);
    *values = padded;
    return ERROR_SUCCESS;

err:
    msiobj_release(&padded->hdr);
    return r;
}

static BOOL row_has_null_primary_keys(struct insert_view *iv, MSIRECORD *row)
{
    UINT r, i, col_count, type;

    r = iv->table->ops->get_dimensions( iv->table, NULL, &col_count );
    if (r != ERROR_SUCCESS)
        return FALSE;

    for (i = 1; i <= col_count; i++)
    {
        r = iv->table->ops->get_column_info(iv->table, i, NULL, &type,
                                            NULL, NULL);
        if (r != ERROR_SUCCESS)
            return FALSE;

        if (!(type & MSITYPE_KEY))
            continue;

        if (MSI_RecordIsNull(row, i))
            return TRUE;
    }

    return FALSE;
}

static UINT INSERT_execute( struct tagMSIVIEW *view, MSIRECORD *record )
{
    struct insert_view *iv = (struct insert_view *)view;
    UINT r, row = -1, col_count = 0;
    MSIVIEW *sv;
    MSIRECORD *values = NULL;

    TRACE("%p %p\n", iv, record );

    sv = iv->sv;
    if( !sv )
        return ERROR_FUNCTION_FAILED;

    r = sv->ops->execute( sv, 0 );
    TRACE("sv execute returned %x\n", r);
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

    r = arrange_record( iv, &values );
    if( r != ERROR_SUCCESS )
        goto err;

    /* rows with NULL primary keys are inserted at the beginning of the table */
    if( row_has_null_primary_keys( iv, values ) )
        row = 0;

    r = iv->table->ops->insert_row( iv->table, values, row, iv->bIsTemp );

err:
    if( values )
        msiobj_release( &values->hdr );

    return r;
}


static UINT INSERT_close( struct tagMSIVIEW *view )
{
    struct insert_view *iv = (struct insert_view *)view;
    MSIVIEW *sv;

    TRACE("%p\n", iv);

    sv = iv->sv;
    if( !sv )
        return ERROR_FUNCTION_FAILED;

    return sv->ops->close( sv );
}

static UINT INSERT_get_dimensions( struct tagMSIVIEW *view, UINT *rows, UINT *cols )
{
    struct insert_view *iv = (struct insert_view *)view;
    MSIVIEW *sv;

    TRACE("%p %p %p\n", iv, rows, cols );

    sv = iv->sv;
    if( !sv )
        return ERROR_FUNCTION_FAILED;

    return sv->ops->get_dimensions( sv, rows, cols );
}

static UINT INSERT_get_column_info( struct tagMSIVIEW *view, UINT n, LPCWSTR *name,
                                    UINT *type, BOOL *temporary, LPCWSTR *table_name )
{
    struct insert_view *iv = (struct insert_view *)view;
    MSIVIEW *sv;

    TRACE("%p %d %p %p %p %p\n", iv, n, name, type, temporary, table_name );

    sv = iv->sv;
    if( !sv )
        return ERROR_FUNCTION_FAILED;

    return sv->ops->get_column_info( sv, n, name, type, temporary, table_name );
}

static UINT INSERT_modify( struct tagMSIVIEW *view, MSIMODIFY eModifyMode, MSIRECORD *rec, UINT row)
{
    struct insert_view *iv = (struct insert_view *)view;

    TRACE("%p %d %p\n", iv, eModifyMode, rec );

    return ERROR_FUNCTION_FAILED;
}

static UINT INSERT_delete( struct tagMSIVIEW *view )
{
    struct insert_view *iv = (struct insert_view *)view;
    MSIVIEW *sv;

    TRACE("%p\n", iv );

    sv = iv->sv;
    if( sv )
        sv->ops->delete( sv );
    msiobj_release( &iv->db->hdr );
    free( iv );

    return ERROR_SUCCESS;
}

static const MSIVIEWOPS insert_ops =
{
    INSERT_fetch_int,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    INSERT_execute,
    INSERT_close,
    INSERT_get_dimensions,
    INSERT_get_column_info,
    INSERT_modify,
    INSERT_delete,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};

static UINT count_column_info( const column_info *ci )
{
    UINT n = 0;
    for ( ; ci; ci = ci->next )
        n++;
    return n;
}

UINT INSERT_CreateView( MSIDATABASE *db, MSIVIEW **view, LPCWSTR table,
                        column_info *columns, column_info *values, BOOL temp )
{
    struct insert_view *iv = NULL;
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

    iv = calloc( 1, sizeof *iv );
    if( !iv )
        return ERROR_FUNCTION_FAILED;

    /* fill the structure */
    iv->view.ops = &insert_ops;
    msiobj_addref( &db->hdr );
    iv->table = tv;
    iv->db = db;
    iv->vals = values;
    iv->bIsTemp = temp;
    iv->sv = sv;
    *view = (MSIVIEW*) iv;

    return ERROR_SUCCESS;
}
