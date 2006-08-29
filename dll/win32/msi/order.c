/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2002 Mike McCormack for CodeWeavers
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

typedef struct tagMSIORDERVIEW
{
    MSIVIEW        view;
    MSIDATABASE   *db;
    MSIVIEW       *table;
    UINT          *reorder;
    UINT           num_cols;
    UINT           cols[1];
} MSIORDERVIEW;

static UINT ORDER_compare( MSIORDERVIEW *ov, UINT a, UINT b, UINT *swap )
{
    UINT r, i, a_val = 0, b_val = 0;

    *swap = 0;
    for( i=0; i<ov->num_cols; i++ )
    {
        r = ov->table->ops->fetch_int( ov->table, a, ov->cols[i], &a_val );
        if( r != ERROR_SUCCESS )
            return r;

        r = ov->table->ops->fetch_int( ov->table, b, ov->cols[i], &b_val );
        if( r != ERROR_SUCCESS )
            return r;

        if( a_val != b_val )
        {
            if( a_val > b_val )
                *swap = 1;
            break;
        }
    }

    return ERROR_SUCCESS;
}

static UINT ORDER_mergesort( MSIORDERVIEW *ov, UINT left, UINT right )
{
    UINT r, centre = (left + right)/2, temp, swap = 0, i, j;
    UINT *array = ov->reorder;

    if( left == right )
        return ERROR_SUCCESS;

    /* sort the left half */
    r = ORDER_mergesort( ov, left, centre );
    if( r != ERROR_SUCCESS )
        return r;

    /* sort the right half */
    r = ORDER_mergesort( ov, centre+1, right );
    if( r != ERROR_SUCCESS )
        return r;

    for( i=left, j=centre+1; (i<=centre) && (j<=right); i++ )
    {
        r = ORDER_compare( ov, array[i], array[j], &swap );
        if( r != ERROR_SUCCESS )
            return r;
        if( swap )
        { 
            temp = array[j];
            memmove( &array[i+1], &array[i], (j-i)*sizeof (UINT) );
            array[i] = temp;
            j++;
            centre++;
        }
    }

    return ERROR_SUCCESS;
}

static UINT ORDER_verify( MSIORDERVIEW *ov, UINT num_rows )
{
    UINT i, swap, r;

    for( i=1; i<num_rows; i++ )
    {
        r = ORDER_compare( ov, ov->reorder[i-1], ov->reorder[i], &swap );
        if( r != ERROR_SUCCESS )
            return r;
        if( !swap )
            continue;
        ERR("Bad order! %d\n", i);
        return ERROR_FUNCTION_FAILED;
    }

    return ERROR_SUCCESS;
}

static UINT ORDER_fetch_int( struct tagMSIVIEW *view, UINT row, UINT col, UINT *val )
{
    MSIORDERVIEW *ov = (MSIORDERVIEW*)view;

    TRACE("%p %d %d %p\n", ov, row, col, val );

    if( !ov->table )
         return ERROR_FUNCTION_FAILED;

    row = ov->reorder[ row ];

    return ov->table->ops->fetch_int( ov->table, row, col, val );
}

static UINT ORDER_execute( struct tagMSIVIEW *view, MSIRECORD *record )
{
    MSIORDERVIEW *ov = (MSIORDERVIEW*)view;
    UINT r, num_rows = 0, i;

    TRACE("%p %p\n", ov, record);

    if( !ov->table )
         return ERROR_FUNCTION_FAILED;

    r = ov->table->ops->execute( ov->table, record );
    if( r != ERROR_SUCCESS )
        return r;

    r = ov->table->ops->get_dimensions( ov->table, &num_rows, NULL );
    if( r != ERROR_SUCCESS )
        return r;

    ov->reorder = msi_alloc( num_rows*sizeof(UINT) );
    if( !ov->reorder )
        return ERROR_FUNCTION_FAILED;

    for( i=0; i<num_rows; i++ )
        ov->reorder[i] = i;

    r = ORDER_mergesort( ov, 0, num_rows - 1 );
    if( r != ERROR_SUCCESS )
        return r;

    r = ORDER_verify( ov, num_rows );
    if( r != ERROR_SUCCESS )
        return r;

    return ERROR_SUCCESS;
}

static UINT ORDER_close( struct tagMSIVIEW *view )
{
    MSIORDERVIEW *ov = (MSIORDERVIEW*)view;

    TRACE("%p\n", ov );

    if( !ov->table )
         return ERROR_FUNCTION_FAILED;

    msi_free( ov->reorder );
    ov->reorder = NULL;

    return ov->table->ops->close( ov->table );
}

static UINT ORDER_get_dimensions( struct tagMSIVIEW *view, UINT *rows, UINT *cols )
{
    MSIORDERVIEW *ov = (MSIORDERVIEW*)view;

    TRACE("%p %p %p\n", ov, rows, cols );

    if( !ov->table )
         return ERROR_FUNCTION_FAILED;

    return ov->table->ops->get_dimensions( ov->table, rows, cols );
}

static UINT ORDER_get_column_info( struct tagMSIVIEW *view,
                UINT n, LPWSTR *name, UINT *type )
{
    MSIORDERVIEW *ov = (MSIORDERVIEW*)view;

    TRACE("%p %d %p %p\n", ov, n, name, type );

    if( !ov->table )
         return ERROR_FUNCTION_FAILED;

    return ov->table->ops->get_column_info( ov->table, n, name, type );
}

static UINT ORDER_modify( struct tagMSIVIEW *view, MSIMODIFY eModifyMode,
                MSIRECORD *rec )
{
    MSIORDERVIEW *ov = (MSIORDERVIEW*)view;

    TRACE("%p %d %p\n", ov, eModifyMode, rec );

    if( !ov->table )
         return ERROR_FUNCTION_FAILED;

    return ov->table->ops->modify( ov->table, eModifyMode, rec );
}

static UINT ORDER_delete( struct tagMSIVIEW *view )
{
    MSIORDERVIEW *ov = (MSIORDERVIEW*)view;

    TRACE("%p\n", ov );

    if( ov->table )
        ov->table->ops->delete( ov->table );

    msi_free( ov->reorder );
    ov->reorder = NULL;

    msiobj_release( &ov->db->hdr );
    msi_free( ov );

    return ERROR_SUCCESS;
}

static UINT ORDER_find_matching_rows( struct tagMSIVIEW *view, UINT col,
    UINT val, UINT *row, MSIITERHANDLE *handle )
{
    MSIORDERVIEW *ov = (MSIORDERVIEW*)view;
    UINT r;

    TRACE("%p, %d, %u, %p\n", ov, col, val, *handle);

    if( !ov->table )
         return ERROR_FUNCTION_FAILED;

    r = ov->table->ops->find_matching_rows( ov->table, col, val, row, handle );

    *row = ov->reorder[ *row ];

    return r;
}


static const MSIVIEWOPS order_ops =
{
    ORDER_fetch_int,
    NULL,
    NULL,
    NULL,
    ORDER_execute,
    ORDER_close,
    ORDER_get_dimensions,
    ORDER_get_column_info,
    ORDER_modify,
    ORDER_delete,
    ORDER_find_matching_rows
};

static UINT ORDER_AddColumn( MSIORDERVIEW *ov, LPCWSTR name )
{
    UINT n, count, r;
    MSIVIEW *table;

    TRACE("%p adding %s\n", ov, debugstr_w( name ) );

    if( ov->view.ops != &order_ops )
        return ERROR_FUNCTION_FAILED;

    table = ov->table;
    if( !table )
        return ERROR_FUNCTION_FAILED;
    if( !table->ops->get_dimensions )
        return ERROR_FUNCTION_FAILED;
    if( !table->ops->get_column_info )
        return ERROR_FUNCTION_FAILED;

    r = table->ops->get_dimensions( table, NULL, &count );
    if( r != ERROR_SUCCESS )
        return r;

    if( ov->num_cols >= count )
        return ERROR_FUNCTION_FAILED;

    r = VIEW_find_column( table, name, &n );
    if( r != ERROR_SUCCESS )
        return r;

    ov->cols[ov->num_cols] = n;
    TRACE("Ordering by column %s (%d)\n", debugstr_w( name ), n);

    ov->num_cols++;

    return ERROR_SUCCESS;
}

UINT ORDER_CreateView( MSIDATABASE *db, MSIVIEW **view, MSIVIEW *table,
                       column_info *columns )
{
    MSIORDERVIEW *ov = NULL;
    UINT count = 0, r;
    column_info *x;

    TRACE("%p\n", ov );

    r = table->ops->get_dimensions( table, NULL, &count );
    if( r != ERROR_SUCCESS )
    {
        ERR("can't get table dimensions\n");
        return r;
    }

    ov = msi_alloc_zero( sizeof *ov + sizeof (UINT) * count );
    if( !ov )
        return ERROR_FUNCTION_FAILED;
    
    /* fill the structure */
    ov->view.ops = &order_ops;
    msiobj_addref( &db->hdr );
    ov->db = db;
    ov->table = table;
    ov->reorder = NULL;
    ov->num_cols = 0;
    *view = (MSIVIEW*) ov;

    for( x = columns; x ; x = x->next )
        ORDER_AddColumn( ov, x->column );

    return ERROR_SUCCESS;
}
