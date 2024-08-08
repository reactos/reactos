/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2006 Mike McCormack
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

struct alter_view
{
    MSIVIEW        view;
    MSIDATABASE   *db;
    MSIVIEW       *table;
    column_info   *colinfo;
    INT hold;
};

static UINT ALTER_fetch_int( struct tagMSIVIEW *view, UINT row, UINT col, UINT *val )
{
    struct alter_view *av = (struct alter_view *)view;

    TRACE("%p %d %d %p\n", av, row, col, val );

    return ERROR_FUNCTION_FAILED;
}

static UINT ALTER_fetch_stream( struct tagMSIVIEW *view, UINT row, UINT col, IStream **stm)
{
    struct alter_view *av = (struct alter_view *)view;

    TRACE("%p %d %d %p\n", av, row, col, stm );

    return ERROR_FUNCTION_FAILED;
}

static UINT ALTER_execute( struct tagMSIVIEW *view, MSIRECORD *record )
{
    struct alter_view *av = (struct alter_view *)view;
    UINT ref;

    TRACE("%p %p\n", av, record);

    if (av->colinfo)
        return av->table->ops->add_column(av->table, av->colinfo->column,
                av->colinfo->type, av->hold == 1);

    if (av->hold == 1)
        av->table->ops->add_ref(av->table);
    else if (av->hold == -1)
    {
        ref = av->table->ops->release(av->table);
        if (ref == 0)
            av->table = NULL;
    }

    return ERROR_SUCCESS;
}

static UINT ALTER_close( struct tagMSIVIEW *view )
{
    struct alter_view *av = (struct alter_view *)view;

    TRACE("%p\n", av );

    return ERROR_SUCCESS;
}

static UINT ALTER_get_dimensions( struct tagMSIVIEW *view, UINT *rows, UINT *cols )
{
    struct alter_view *av = (struct alter_view *)view;

    TRACE("%p %p %p\n", av, rows, cols );

    return ERROR_FUNCTION_FAILED;
}

static UINT ALTER_get_column_info( struct tagMSIVIEW *view, UINT n, LPCWSTR *name,
                                   UINT *type, BOOL *temporary, LPCWSTR *table_name )
{
    struct alter_view *av = (struct alter_view *)view;

    TRACE("%p %d %p %p %p %p\n", av, n, name, type, temporary, table_name );

    return ERROR_FUNCTION_FAILED;
}

static UINT ALTER_modify( struct tagMSIVIEW *view, MSIMODIFY eModifyMode,
                          MSIRECORD *rec, UINT row )
{
    struct alter_view *av = (struct alter_view *)view;

    TRACE("%p %d %p\n", av, eModifyMode, rec );

    return ERROR_FUNCTION_FAILED;
}

static UINT ALTER_delete( struct tagMSIVIEW *view )
{
    struct alter_view *av = (struct alter_view *)view;

    TRACE("%p\n", av );
    if (av->table)
        av->table->ops->delete( av->table );
    free( av );

    return ERROR_SUCCESS;
}

static const MSIVIEWOPS alter_ops =
{
    ALTER_fetch_int,
    ALTER_fetch_stream,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    ALTER_execute,
    ALTER_close,
    ALTER_get_dimensions,
    ALTER_get_column_info,
    ALTER_modify,
    ALTER_delete,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};

UINT ALTER_CreateView( MSIDATABASE *db, MSIVIEW **view, LPCWSTR name, column_info *colinfo, int hold )
{
    struct alter_view *av;
    UINT r;

    TRACE("%p %p %s %d\n", view, colinfo, debugstr_w(name), hold );

    av = calloc( 1, sizeof *av );
    if( !av )
        return ERROR_FUNCTION_FAILED;

    r = TABLE_CreateView( db, name, &av->table );
    if (r != ERROR_SUCCESS)
    {
        free( av );
        return r;
    }

    if (colinfo)
        colinfo->table = name;

    /* fill the structure */
    av->view.ops = &alter_ops;
    av->db = db;
    av->hold = hold;
    av->colinfo = colinfo;

    *view = &av->view;

    return ERROR_SUCCESS;
}
