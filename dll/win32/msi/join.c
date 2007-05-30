/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2006 Mike McCormack for CodeWeavers
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
#include "query.h"

WINE_DEFAULT_DEBUG_CHANNEL(msidb);

typedef struct tagMSIJOINVIEW
{
    MSIVIEW        view;
    MSIDATABASE   *db;
    MSIVIEW       *left, *right;
    UINT           left_count, right_count;
    UINT           left_rows, right_rows;
} MSIJOINVIEW;

static UINT JOIN_fetch_int( struct tagMSIVIEW *view, UINT row, UINT col, UINT *val )
{
    MSIJOINVIEW *jv = (MSIJOINVIEW*)view;
    MSIVIEW *table;

    TRACE("%p %d %d %p\n", jv, row, col, val );

    if( !jv->left || !jv->right )
         return ERROR_FUNCTION_FAILED;

    if( (col==0) || (col>(jv->left_count + jv->right_count)) )
         return ERROR_FUNCTION_FAILED;

    if( row >= (jv->left_rows * jv->right_rows) )
         return ERROR_FUNCTION_FAILED;

    if( col <= jv->left_count )
    {
        table = jv->left;
        row = (row/jv->right_rows);
    }
    else
    {
        table = jv->right;
        row = (row % jv->right_rows);
        col -= jv->left_count;
    }

    return table->ops->fetch_int( table, row, col, val );
}

static UINT JOIN_fetch_stream( struct tagMSIVIEW *view, UINT row, UINT col, IStream **stm)
{
    MSIJOINVIEW *jv = (MSIJOINVIEW*)view;
    MSIVIEW *table;

    TRACE("%p %d %d %p\n", jv, row, col, stm );

    if( !jv->left || !jv->right )
         return ERROR_FUNCTION_FAILED;

    if( (col==0) || (col>(jv->left_count + jv->right_count)) )
         return ERROR_FUNCTION_FAILED;

    if( row >= jv->left_rows * jv->right_rows )
         return ERROR_FUNCTION_FAILED;

    if( row <= jv->left_count )
    {
        table = jv->left;
        row = (row/jv->right_rows);
    }
    else
    {
        table = jv->right;
        row = (row % jv->right_rows);
        col -= jv->left_count;
    }

    return table->ops->fetch_stream( table, row, col, stm );
}

static UINT JOIN_execute( struct tagMSIVIEW *view, MSIRECORD *record )
{
    MSIJOINVIEW *jv = (MSIJOINVIEW*)view;
    UINT r, *ldata = NULL, *rdata = NULL;

    TRACE("%p %p\n", jv, record);

    if( !jv->left || !jv->right )
         return ERROR_FUNCTION_FAILED;

    r = jv->left->ops->execute( jv->left, NULL );
    if (r != ERROR_SUCCESS)
        return r;

    r = jv->right->ops->execute( jv->right, NULL );
    if (r != ERROR_SUCCESS)
        return r;

    /* get the number of rows in each table */
    r = jv->left->ops->get_dimensions( jv->left, &jv->left_rows, NULL );
    if( r != ERROR_SUCCESS )
    {
        ERR("can't get left table dimensions\n");
        goto end;
    }

    r = jv->right->ops->get_dimensions( jv->right, &jv->right_rows, NULL );
    if( r != ERROR_SUCCESS )
    {
        ERR("can't get right table dimensions\n");
        goto end;
    }

end:
    msi_free( ldata );
    msi_free( rdata );

    return r;
}

static UINT JOIN_close( struct tagMSIVIEW *view )
{
    MSIJOINVIEW *jv = (MSIJOINVIEW*)view;

    TRACE("%p\n", jv );

    if( !jv->left || !jv->right )
        return ERROR_FUNCTION_FAILED;

    jv->left->ops->close( jv->left );
    jv->right->ops->close( jv->right );

    return ERROR_SUCCESS;
}

static UINT JOIN_get_dimensions( struct tagMSIVIEW *view, UINT *rows, UINT *cols )
{
    MSIJOINVIEW *jv = (MSIJOINVIEW*)view;

    TRACE("%p %p %p\n", jv, rows, cols );

    if( cols )
        *cols = jv->left_count + jv->right_count;

    if( rows )
    {
        if( !jv->left || !jv->right )
            return ERROR_FUNCTION_FAILED;

        *rows = jv->left_rows * jv->right_rows;
    }

    return ERROR_SUCCESS;
}

static UINT JOIN_get_column_info( struct tagMSIVIEW *view,
                UINT n, LPWSTR *name, UINT *type )
{
    MSIJOINVIEW *jv = (MSIJOINVIEW*)view;

    TRACE("%p %d %p %p\n", jv, n, name, type );

    if( !jv->left || !jv->right )
        return ERROR_FUNCTION_FAILED;

    if( (n==0) || (n>(jv->left_count + jv->right_count)) )
        return ERROR_FUNCTION_FAILED;

    if( n <= jv->left_count )
        return jv->left->ops->get_column_info( jv->left, n, name, type );

    n = n - jv->left_count;

    return jv->right->ops->get_column_info( jv->right, n, name, type );
}

static UINT JOIN_modify( struct tagMSIVIEW *view, MSIMODIFY eModifyMode,
                MSIRECORD *rec )
{
    MSIJOINVIEW *jv = (MSIJOINVIEW*)view;

    TRACE("%p %d %p\n", jv, eModifyMode, rec );

    return ERROR_FUNCTION_FAILED;
}

static UINT JOIN_delete( struct tagMSIVIEW *view )
{
    MSIJOINVIEW *jv = (MSIJOINVIEW*)view;

    TRACE("%p\n", jv );

    if( jv->left )
        jv->left->ops->delete( jv->left );
    jv->left = NULL;

    if( jv->right )
        jv->right->ops->delete( jv->right );
    jv->right = NULL;

    msi_free( jv );

    return ERROR_SUCCESS;
}

static UINT JOIN_find_matching_rows( struct tagMSIVIEW *view, UINT col,
    UINT val, UINT *row, MSIITERHANDLE *handle )
{
    MSIJOINVIEW *jv = (MSIJOINVIEW*)view;

    FIXME("%p, %d, %u, %p\n", jv, col, val, *handle);

    return ERROR_FUNCTION_FAILED;
}

static const MSIVIEWOPS join_ops =
{
    JOIN_fetch_int,
    JOIN_fetch_stream,
    NULL,
    NULL,
    JOIN_execute,
    JOIN_close,
    JOIN_get_dimensions,
    JOIN_get_column_info,
    JOIN_modify,
    JOIN_delete,
    JOIN_find_matching_rows
};

UINT JOIN_CreateView( MSIDATABASE *db, MSIVIEW **view,
                      LPCWSTR left, LPCWSTR right )
{
    MSIJOINVIEW *jv = NULL;
    UINT r = ERROR_SUCCESS;

    TRACE("%p (%s,%s)\n", jv, debugstr_w(left), debugstr_w(right) );

    jv = msi_alloc_zero( sizeof *jv );
    if( !jv )
        return ERROR_FUNCTION_FAILED;

    /* fill the structure */
    jv->view.ops = &join_ops;
    jv->db = db;

    /* create the tables to join */
    r = TABLE_CreateView( db, left, &jv->left );
    if( r != ERROR_SUCCESS )
    {
        ERR("can't create left table\n");
        goto end;
    }

    r = TABLE_CreateView( db, right, &jv->right );
    if( r != ERROR_SUCCESS )
    {
        ERR("can't create right table\n");
        goto end;
    }

    /* get the number of columns in each table */
    r = jv->left->ops->get_dimensions( jv->left, NULL, &jv->left_count );
    if( r != ERROR_SUCCESS )
    {
        ERR("can't get left table dimensions\n");
        goto end;
    }

    r = jv->right->ops->get_dimensions( jv->right, NULL, &jv->right_count );
    if( r != ERROR_SUCCESS )
    {
        ERR("can't get right table dimensions\n");
        goto end;
    }

    *view = &jv->view;
    return ERROR_SUCCESS;

end:
    jv->view.ops->delete( &jv->view );

    return r;
}
