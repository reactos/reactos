/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2008 James Hawkins
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

struct drop_view
{
    MSIVIEW view;
    MSIDATABASE *db;
    MSIVIEW *table;
    column_info *colinfo;
    INT hold;
};

static UINT DROP_execute(struct tagMSIVIEW *view, MSIRECORD *record)
{
    struct drop_view *dv = (struct drop_view *)view;
    UINT r;

    TRACE("%p %p\n", dv, record);

    if( !dv->table )
         return ERROR_FUNCTION_FAILED;

    r = dv->table->ops->execute(dv->table, record);
    if (r != ERROR_SUCCESS)
        return r;

    return dv->table->ops->drop(dv->table);
}

static UINT DROP_close(struct tagMSIVIEW *view)
{
    struct drop_view *dv = (struct drop_view *)view;

    TRACE("%p\n", dv);

    return ERROR_SUCCESS;
}

static UINT DROP_get_dimensions(struct tagMSIVIEW *view, UINT *rows, UINT *cols)
{
    struct drop_view *dv = (struct drop_view *)view;

    TRACE("%p %p %p\n", dv, rows, cols);

    return ERROR_FUNCTION_FAILED;
}

static UINT DROP_delete( struct tagMSIVIEW *view )
{
    struct drop_view *dv = (struct drop_view *)view;

    TRACE("%p\n", dv );

    if( dv->table )
        dv->table->ops->delete( dv->table );

    free( dv );

    return ERROR_SUCCESS;
}

static const MSIVIEWOPS drop_ops =
{
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    DROP_execute,
    DROP_close,
    DROP_get_dimensions,
    NULL,
    NULL,
    DROP_delete,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};

UINT DROP_CreateView(MSIDATABASE *db, MSIVIEW **view, LPCWSTR name)
{
    struct drop_view *dv;
    UINT r;

    TRACE("%p %s\n", view, debugstr_w(name));

    dv = calloc(1, sizeof *dv);
    if(!dv)
        return ERROR_FUNCTION_FAILED;

    r = TABLE_CreateView(db, name, &dv->table);
    if (r != ERROR_SUCCESS)
    {
        free( dv );
        return r;
    }

    dv->view.ops = &drop_ops;
    dv->db = db;

    *view = (MSIVIEW *)dv;

    return ERROR_SUCCESS;
}
