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

typedef struct tagMSISELECTVIEW
{
    MSIVIEW        view;
    MSIDATABASE   *db;
    MSIVIEW       *table;
    UINT           num_cols;
    UINT           max_cols;
    UINT           cols[1];
} MSISELECTVIEW;

static UINT SELECT_fetch_int( struct tagMSIVIEW *view, UINT row, UINT col, UINT *val )
{
    MSISELECTVIEW *sv = (MSISELECTVIEW*)view;

    TRACE("%p %d %d %p\n", sv, row, col, val );

    if( !sv->table )
         return ERROR_FUNCTION_FAILED;

    if( (col==0) || (col>sv->num_cols) )
         return ERROR_FUNCTION_FAILED;

    col = sv->cols[ col - 1 ];

    return sv->table->ops->fetch_int( sv->table, row, col, val );
}

static UINT SELECT_fetch_stream( struct tagMSIVIEW *view, UINT row, UINT col, IStream **stm)
{
    MSISELECTVIEW *sv = (MSISELECTVIEW*)view;

    TRACE("%p %d %d %p\n", sv, row, col, stm );

    if( !sv->table )
         return ERROR_FUNCTION_FAILED;

    if( (col==0) || (col>sv->num_cols) )
         return ERROR_FUNCTION_FAILED;

    col = sv->cols[ col - 1 ];

    return sv->table->ops->fetch_stream( sv->table, row, col, stm );
}

static UINT SELECT_set_int( struct tagMSIVIEW *view, UINT row, UINT col, UINT val )
{
    MSISELECTVIEW *sv = (MSISELECTVIEW*)view;

    TRACE("%p %d %d %04x\n", sv, row, col, val );

    if( !sv->table )
         return ERROR_FUNCTION_FAILED;

    if( (col==0) || (col>sv->num_cols) )
         return ERROR_FUNCTION_FAILED;

    col = sv->cols[ col - 1 ];

    return sv->table->ops->set_int( sv->table, row, col, val );
}

static UINT SELECT_insert_row( struct tagMSIVIEW *view, UINT *num )  
{
    MSISELECTVIEW *sv = (MSISELECTVIEW*)view;

    TRACE("%p %p\n", sv, num );

    if( !sv->table )
         return ERROR_FUNCTION_FAILED;

    return sv->table->ops->insert_row( sv->table, num );
}

static UINT SELECT_execute( struct tagMSIVIEW *view, MSIRECORD *record )
{
    MSISELECTVIEW *sv = (MSISELECTVIEW*)view;

    TRACE("%p %p\n", sv, record);

    if( !sv->table )
         return ERROR_FUNCTION_FAILED;

    return sv->table->ops->execute( sv->table, record );
}

static UINT SELECT_close( struct tagMSIVIEW *view )
{
    MSISELECTVIEW *sv = (MSISELECTVIEW*)view;

    TRACE("%p\n", sv );

    if( !sv->table )
         return ERROR_FUNCTION_FAILED;

    return sv->table->ops->close( sv->table );
}

static UINT SELECT_get_dimensions( struct tagMSIVIEW *view, UINT *rows, UINT *cols )
{
    MSISELECTVIEW *sv = (MSISELECTVIEW*)view;

    TRACE("%p %p %p\n", sv, rows, cols );

    if( !sv->table )
         return ERROR_FUNCTION_FAILED;

    if( cols )
        *cols = sv->num_cols;

    return sv->table->ops->get_dimensions( sv->table, rows, NULL );
}

static UINT SELECT_get_column_info( struct tagMSIVIEW *view,
                UINT n, LPWSTR *name, UINT *type )
{
    MSISELECTVIEW *sv = (MSISELECTVIEW*)view;

    TRACE("%p %d %p %p\n", sv, n, name, type );

    if( !sv->table )
         return ERROR_FUNCTION_FAILED;

    if( (n==0) || (n>sv->num_cols) )
         return ERROR_FUNCTION_FAILED;

    n = sv->cols[ n - 1 ];

    return sv->table->ops->get_column_info( sv->table, n, name, type );
}

static UINT SELECT_modify( struct tagMSIVIEW *view, MSIMODIFY eModifyMode, MSIHANDLE hrec)
{
    MSISELECTVIEW *sv = (MSISELECTVIEW*)view;

    TRACE("%p %d %ld\n", sv, eModifyMode, hrec );

    if( !sv->table )
         return ERROR_FUNCTION_FAILED;

    return sv->table->ops->modify( sv->table, eModifyMode, hrec );
}

static UINT SELECT_delete( struct tagMSIVIEW *view )
{
    MSISELECTVIEW *sv = (MSISELECTVIEW*)view;

    TRACE("%p\n", sv );

    if( sv->table )
        sv->table->ops->delete( sv->table );

    HeapFree( GetProcessHeap(), 0, sv );

    return ERROR_SUCCESS;
}


MSIVIEWOPS select_ops =
{
    SELECT_fetch_int,
    SELECT_fetch_stream,
    SELECT_set_int,
    SELECT_insert_row,
    SELECT_execute,
    SELECT_close,
    SELECT_get_dimensions,
    SELECT_get_column_info,
    SELECT_modify,
    SELECT_delete
};

static UINT SELECT_AddColumn( MSISELECTVIEW *sv, LPWSTR name )
{
    UINT r, n=0;
    MSIVIEW *table;

    TRACE("%p adding %s\n", sv, debugstr_w( name ) );

    if( sv->view.ops != &select_ops )
        return ERROR_FUNCTION_FAILED;

    table = sv->table;
    if( !table )
        return ERROR_FUNCTION_FAILED;
    if( !table->ops->get_dimensions )
        return ERROR_FUNCTION_FAILED;
    if( !table->ops->get_column_info )
        return ERROR_FUNCTION_FAILED;

    if( sv->num_cols >= sv->max_cols )
        return ERROR_FUNCTION_FAILED;

    r = VIEW_find_column( table, name, &n );
    if( r != ERROR_SUCCESS )
        return r;

    sv->cols[sv->num_cols] = n;
    TRACE("Translating column %s from %d -> %d\n", 
          debugstr_w( name ), sv->num_cols, n);

    sv->num_cols++;

    return ERROR_SUCCESS;
}

UINT SELECT_CreateView( MSIDATABASE *db, MSIVIEW **view, MSIVIEW *table,
                        string_list *columns )
{
    MSISELECTVIEW *sv = NULL;
    UINT count = 0, r;

    TRACE("%p\n", sv );

    r = table->ops->get_dimensions( table, NULL, &count );
    if( r != ERROR_SUCCESS )
    {
        ERR("can't get table dimensions\n");
        return r;
    }

    sv = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, 
                    sizeof *sv + count*sizeof (UINT) );
    if( !sv )
        return ERROR_FUNCTION_FAILED;
    
    /* fill the structure */
    sv->view.ops = &select_ops;
    sv->db = db;
    sv->table = table;
    sv->num_cols = 0;
    sv->max_cols = count;

    while( columns )
    {
        r = SELECT_AddColumn( sv, columns->string );
        if( r )
            break;
        columns = columns->next;
    }

    if( r != ERROR_SUCCESS )
    {
        sv->view.ops->delete( &sv->view );
        sv = NULL;
    }

    *view = &sv->view;

    return r;
}
