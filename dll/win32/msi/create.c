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

struct create_view
{
    MSIVIEW          view;
    MSIDATABASE     *db;
    LPCWSTR          name;
    BOOL             bIsTemp;
    BOOL             hold;
    column_info     *col_info;
};

static UINT CREATE_fetch_int( struct tagMSIVIEW *view, UINT row, UINT col, UINT *val )
{
    struct create_view *cv = (struct create_view *)view;

    TRACE("%p %d %d %p\n", cv, row, col, val );

    return ERROR_FUNCTION_FAILED;
}

static UINT CREATE_execute( struct tagMSIVIEW *view, MSIRECORD *record )
{
    struct create_view *cv = (struct create_view *)view;
    BOOL persist = (cv->bIsTemp) ? MSICONDITION_FALSE : MSICONDITION_TRUE;

    TRACE("%p Table %s (%s)\n", cv, debugstr_w(cv->name),
          cv->bIsTemp?"temporary":"permanent");

    if (cv->bIsTemp && !cv->hold)
        return ERROR_SUCCESS;

    return msi_create_table( cv->db, cv->name, cv->col_info, persist, cv->hold );
}

static UINT CREATE_close( struct tagMSIVIEW *view )
{
    struct create_view *cv = (struct create_view *)view;

    TRACE("%p\n", cv);

    return ERROR_SUCCESS;
}

static UINT CREATE_get_dimensions( struct tagMSIVIEW *view, UINT *rows, UINT *cols )
{
    struct create_view *cv = (struct create_view *)view;

    TRACE("%p %p %p\n", cv, rows, cols );

    return ERROR_FUNCTION_FAILED;
}

static UINT CREATE_get_column_info( struct tagMSIVIEW *view, UINT n, LPCWSTR *name,
                                    UINT *type, BOOL *temporary, LPCWSTR *table_name )
{
    struct create_view *cv = (struct create_view *)view;

    TRACE("%p %d %p %p %p %p\n", cv, n, name, type, temporary, table_name );

    return ERROR_FUNCTION_FAILED;
}

static UINT CREATE_modify( struct tagMSIVIEW *view, MSIMODIFY eModifyMode,
                           MSIRECORD *rec, UINT row)
{
    struct create_view *cv = (struct create_view *)view;

    TRACE("%p %d %p\n", cv, eModifyMode, rec );

    return ERROR_FUNCTION_FAILED;
}

static UINT CREATE_delete( struct tagMSIVIEW *view )
{
    struct create_view *cv = (struct create_view *)view;

    TRACE("%p\n", cv );

    msiobj_release( &cv->db->hdr );
    free( cv );

    return ERROR_SUCCESS;
}

static const MSIVIEWOPS create_ops =
{
    CREATE_fetch_int,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    CREATE_execute,
    CREATE_close,
    CREATE_get_dimensions,
    CREATE_get_column_info,
    CREATE_modify,
    CREATE_delete,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};

static UINT check_columns( const column_info *col_info )
{
    const column_info *c1, *c2;

    /* check for two columns with the same name */
    for( c1 = col_info; c1; c1 = c1->next )
        for( c2 = c1->next; c2; c2 = c2->next )
            if (!wcscmp( c1->column, c2->column ))
                return ERROR_BAD_QUERY_SYNTAX;

    return ERROR_SUCCESS;
}

UINT CREATE_CreateView( MSIDATABASE *db, MSIVIEW **view, LPCWSTR table,
                        column_info *col_info, BOOL hold )
{
    struct create_view *cv = NULL;
    UINT r;
    column_info *col;
    BOOL temp = TRUE;
    BOOL tempprim = FALSE;

    TRACE("%p\n", cv );

    r = check_columns( col_info );
    if( r != ERROR_SUCCESS )
        return r;

    cv = calloc( 1, sizeof *cv );
    if( !cv )
        return ERROR_FUNCTION_FAILED;

    for( col = col_info; col; col = col->next )
    {
        if (!col->table)
            col->table = table;

        if( !(col->type & MSITYPE_TEMPORARY) )
            temp = FALSE;
        else if ( col->type & MSITYPE_KEY )
            tempprim = TRUE;
    }

    if ( !temp && tempprim )
    {
        free( cv );
        return ERROR_FUNCTION_FAILED;
    }

    /* fill the structure */
    cv->view.ops = &create_ops;
    msiobj_addref( &db->hdr );
    cv->db = db;
    cv->name = table;
    cv->col_info = col_info;
    cv->bIsTemp = temp;
    cv->hold = hold;
    *view = (MSIVIEW*) cv;

    return ERROR_SUCCESS;
}
