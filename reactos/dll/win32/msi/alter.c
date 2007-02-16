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

typedef struct tagMSIALTERVIEW
{
    MSIVIEW        view;
    MSIDATABASE   *db;
} MSIALTERVIEW;

static UINT ALTER_fetch_int( struct tagMSIVIEW *view, UINT row, UINT col, UINT *val )
{
    MSIALTERVIEW *av = (MSIALTERVIEW*)view;

    TRACE("%p %d %d %p\n", av, row, col, val );

    return ERROR_FUNCTION_FAILED;
}

static UINT ALTER_fetch_stream( struct tagMSIVIEW *view, UINT row, UINT col, IStream **stm)
{
    MSIALTERVIEW *av = (MSIALTERVIEW*)view;

    TRACE("%p %d %d %p\n", av, row, col, stm );

    return ERROR_FUNCTION_FAILED;
}

static UINT ALTER_execute( struct tagMSIVIEW *view, MSIRECORD *record )
{
    MSIALTERVIEW *av = (MSIALTERVIEW*)view;

    FIXME("%p %p\n", av, record);

    return ERROR_SUCCESS;
}

static UINT ALTER_close( struct tagMSIVIEW *view )
{
    MSIALTERVIEW *av = (MSIALTERVIEW*)view;

    TRACE("%p\n", av );

    return ERROR_SUCCESS;
}

static UINT ALTER_get_dimensions( struct tagMSIVIEW *view, UINT *rows, UINT *cols )
{
    MSIALTERVIEW *av = (MSIALTERVIEW*)view;

    TRACE("%p %p %p\n", av, rows, cols );

    return ERROR_FUNCTION_FAILED;
}

static UINT ALTER_get_column_info( struct tagMSIVIEW *view,
                UINT n, LPWSTR *name, UINT *type )
{
    MSIALTERVIEW *av = (MSIALTERVIEW*)view;

    TRACE("%p %d %p %p\n", av, n, name, type );

    return ERROR_FUNCTION_FAILED;
}

static UINT ALTER_modify( struct tagMSIVIEW *view, MSIMODIFY eModifyMode,
                MSIRECORD *rec )
{
    MSIALTERVIEW *av = (MSIALTERVIEW*)view;

    TRACE("%p %d %p\n", av, eModifyMode, rec );

    return ERROR_FUNCTION_FAILED;
}

static UINT ALTER_delete( struct tagMSIVIEW *view )
{
    MSIALTERVIEW *av = (MSIALTERVIEW*)view;

    TRACE("%p\n", av );
    msi_free( av );

    return ERROR_SUCCESS;
}

static UINT ALTER_find_matching_rows( struct tagMSIVIEW *view, UINT col,
    UINT val, UINT *row, MSIITERHANDLE *handle )
{
    TRACE("%p, %d, %u, %p\n", view, col, val, *handle);

    return ERROR_FUNCTION_FAILED;
}


static const MSIVIEWOPS alter_ops =
{
    ALTER_fetch_int,
    ALTER_fetch_stream,
    NULL,
    NULL,
    ALTER_execute,
    ALTER_close,
    ALTER_get_dimensions,
    ALTER_get_column_info,
    ALTER_modify,
    ALTER_delete,
    ALTER_find_matching_rows
};

UINT ALTER_CreateView( MSIDATABASE *db, MSIVIEW **view, LPCWSTR name, int hold )
{
    MSIALTERVIEW *av;

    TRACE("%p\n", view );

    av = msi_alloc_zero( sizeof *av );
    if( !av )
        return ERROR_FUNCTION_FAILED;

    /* fill the structure */
    av->view.ops = &alter_ops;
    av->db = db;

    *view = &av->view;

    return ERROR_SUCCESS;
}
