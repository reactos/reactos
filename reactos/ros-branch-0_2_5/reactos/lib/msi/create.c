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

typedef struct tagMSICREATEVIEW
{
    MSIVIEW          view;
    MSIDATABASE     *db;
    LPWSTR           name;
    BOOL             bIsTemp;
    create_col_info *col_info;
} MSICREATEVIEW;

static UINT CREATE_fetch_int( struct tagMSIVIEW *view, UINT row, UINT col, UINT *val )
{
    MSICREATEVIEW *cv = (MSICREATEVIEW*)view;

    TRACE("%p %d %d %p\n", cv, row, col, val );

    return ERROR_FUNCTION_FAILED;
}

static UINT CREATE_execute( struct tagMSIVIEW *view, MSIRECORD *record )
{
    MSICREATEVIEW *cv = (MSICREATEVIEW*)view;
    create_col_info *col;
    UINT r, nField, row, table_val, column_val;
    static const WCHAR szTables[] =  { '_','T','a','b','l','e','s',0 };
    static const WCHAR szColumns[] = { '_','C','o','l','u','m','n','s',0 };
    MSIVIEW *tv = NULL;

    TRACE("%p Table %s (%s)\n", cv, debugstr_w(cv->name), 
          cv->bIsTemp?"temporary":"permanent");

    /* only add tables that don't exist already */
    if( TABLE_Exists(cv->db, cv->name ) )
        return ERROR_BAD_QUERY_SYNTAX;

    /* add the name to the _Tables table */
    table_val = msi_addstringW( cv->db->strings, 0, cv->name, -1, 1 );
    TRACE("New string %s -> %d\n", debugstr_w( cv->name ), table_val );
    if( table_val < 0 )
        return ERROR_FUNCTION_FAILED;

    r = TABLE_CreateView( cv->db, szTables, &tv );
    TRACE("CreateView returned %x\n", r);
    if( r )
        return r;

    r = tv->ops->execute( tv, 0 );
    TRACE("tv execute returned %x\n", r);
    if( r )
        return r;

    row = -1;
    r = tv->ops->insert_row( tv, &row );
    TRACE("insert_row returned %x\n", r);
    if( r )
        goto err;

    r = tv->ops->set_int( tv, row, 1, table_val );
    if( r )
        goto err;
    tv->ops->delete( tv );
    tv = NULL;

    /* add each column to the _Columns table */
    r = TABLE_CreateView( cv->db, szColumns, &tv );
    if( r )
        return r;

    r = tv->ops->execute( tv, 0 );
    TRACE("tv execute returned %x\n", r);
    if( r )
        return r;

    /*
     * need to set the table, column number, col name and type
     * for each column we enter in the table
     */
    nField = 1;
    for( col = cv->col_info; col; col = col->next )
    {
        row = -1;
        r = tv->ops->insert_row( tv, &row );
        if( r )
            goto err;

        column_val = msi_addstringW( cv->db->strings, 0, col->colname, -1, 1 );
        TRACE("New string %s -> %d\n", debugstr_w( col->colname ), column_val );
        if( column_val < 0 )
            break;

        /* add the string again here so we increase the reference count */
        table_val = msi_addstringW( cv->db->strings, 0, cv->name, -1, 1 );
        if( table_val < 0 )
            break;

        r = tv->ops->set_int( tv, row, 1, table_val );
        if( r )
            break;

        r = tv->ops->set_int( tv, row, 2, 0x8000|nField );
        if( r )
            break;

        r = tv->ops->set_int( tv, row, 3, column_val );
        if( r )
            break;

        r = tv->ops->set_int( tv, row, 4, 0x8000|col->type );
        if( r )
            break;

        nField++;
    }
    if( !col )
        r = ERROR_SUCCESS;

err:
    /* FIXME: remove values from the string table on error */
    if( tv )
        tv->ops->delete( tv );
    return r;
}

static UINT CREATE_close( struct tagMSIVIEW *view )
{
    MSICREATEVIEW *cv = (MSICREATEVIEW*)view;

    TRACE("%p\n", cv);

    return ERROR_SUCCESS;
}

static UINT CREATE_get_dimensions( struct tagMSIVIEW *view, UINT *rows, UINT *cols )
{
    MSICREATEVIEW *cv = (MSICREATEVIEW*)view;

    TRACE("%p %p %p\n", cv, rows, cols );

    return ERROR_FUNCTION_FAILED;
}

static UINT CREATE_get_column_info( struct tagMSIVIEW *view,
                UINT n, LPWSTR *name, UINT *type )
{
    MSICREATEVIEW *cv = (MSICREATEVIEW*)view;

    TRACE("%p %d %p %p\n", cv, n, name, type );

    return ERROR_FUNCTION_FAILED;
}

static UINT CREATE_modify( struct tagMSIVIEW *view, MSIMODIFY eModifyMode, MSIHANDLE hrec)
{
    MSICREATEVIEW *cv = (MSICREATEVIEW*)view;

    TRACE("%p %d %ld\n", cv, eModifyMode, hrec );

    return ERROR_FUNCTION_FAILED;
}

static UINT CREATE_delete( struct tagMSIVIEW *view )
{
    MSICREATEVIEW *cv = (MSICREATEVIEW*)view;
    create_col_info *col;

    TRACE("%p\n", cv );

    col = cv->col_info; 
    while( col )
    {
        create_col_info *t = col;
        col = col->next;
        HeapFree( GetProcessHeap(), 0, t->colname );
        HeapFree( GetProcessHeap(), 0, t );
    }
    msiobj_release( &cv->db->hdr );
    HeapFree( GetProcessHeap(), 0, cv->name );
    HeapFree( GetProcessHeap(), 0, cv );

    return ERROR_SUCCESS;
}


MSIVIEWOPS create_ops =
{
    CREATE_fetch_int,
    NULL,
    NULL,
    NULL,
    CREATE_execute,
    CREATE_close,
    CREATE_get_dimensions,
    CREATE_get_column_info,
    CREATE_modify,
    CREATE_delete
};

UINT CREATE_CreateView( MSIDATABASE *db, MSIVIEW **view, LPWSTR table,
                        create_col_info *col_info, BOOL temp )
{
    MSICREATEVIEW *cv = NULL;

    TRACE("%p\n", cv );

    cv = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof *cv );
    if( !cv )
        return ERROR_FUNCTION_FAILED;
    
    /* fill the structure */
    cv->view.ops = &create_ops;
    msiobj_addref( &db->hdr );
    cv->db = db;
    cv->name = table;  /* FIXME: strdupW it? */
    cv->col_info = col_info;
    cv->bIsTemp = temp;
    *view = (MSIVIEW*) cv;

    return ERROR_SUCCESS;
}
