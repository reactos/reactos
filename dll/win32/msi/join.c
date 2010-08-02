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
#include "msi.h"
#include "msiquery.h"
#include "objbase.h"
#include "objidl.h"
#include "msipriv.h"
#include "query.h"

#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(msidb);

typedef struct tagJOINTABLE
{
    struct list entry;
    MSIVIEW *view;
    UINT columns;
    UINT rows;
    UINT next_rows;
} JOINTABLE;

typedef struct tagMSIJOINVIEW
{
    MSIVIEW        view;
    MSIDATABASE   *db;
    struct list    tables;
    UINT           columns;
    UINT           rows;
} MSIJOINVIEW;

static UINT JOIN_fetch_int( struct tagMSIVIEW *view, UINT row, UINT col, UINT *val )
{
    MSIJOINVIEW *jv = (MSIJOINVIEW*)view;
    JOINTABLE *table;
    UINT cols = 0;
    UINT prev_rows = 1;

    TRACE("%d, %d\n", row, col);

    if (col == 0 || col > jv->columns)
         return ERROR_FUNCTION_FAILED;

    if (row >= jv->rows)
         return ERROR_FUNCTION_FAILED;

    LIST_FOR_EACH_ENTRY(table, &jv->tables, JOINTABLE, entry)
    {
        if (col <= cols + table->columns)
        {
            row = (row % (jv->rows / table->next_rows)) / prev_rows;
            col -= cols;
            break;
        }

        prev_rows *= table->rows;
        cols += table->columns;
    }

    return table->view->ops->fetch_int( table->view, row, col, val );
}

static UINT JOIN_fetch_stream( struct tagMSIVIEW *view, UINT row, UINT col, IStream **stm)
{
    MSIJOINVIEW *jv = (MSIJOINVIEW*)view;
    JOINTABLE *table;
    UINT cols = 0;
    UINT prev_rows = 1;

    TRACE("%p %d %d %p\n", jv, row, col, stm );

    if (col == 0 || col > jv->columns)
         return ERROR_FUNCTION_FAILED;

    if (row >= jv->rows)
         return ERROR_FUNCTION_FAILED;

    LIST_FOR_EACH_ENTRY(table, &jv->tables, JOINTABLE, entry)
    {
        if (col <= cols + table->columns)
        {
            row = (row % (jv->rows / table->next_rows)) / prev_rows;
            col -= cols;
            break;
        }

        prev_rows *= table->rows;
        cols += table->columns;
    }

    return table->view->ops->fetch_stream( table->view, row, col, stm );
}

static UINT JOIN_get_row( struct tagMSIVIEW *view, UINT row, MSIRECORD **rec )
{
    MSIJOINVIEW *jv = (MSIJOINVIEW*)view;

    TRACE("%p %d %p\n", jv, row, rec);

    return msi_view_get_row( jv->db, view, row, rec );
}

static UINT JOIN_execute( struct tagMSIVIEW *view, MSIRECORD *record )
{
    MSIJOINVIEW *jv = (MSIJOINVIEW*)view;
    JOINTABLE *table;
    UINT r, rows;

    TRACE("%p %p\n", jv, record);

    LIST_FOR_EACH_ENTRY(table, &jv->tables, JOINTABLE, entry)
    {
        table->view->ops->execute(table->view, NULL);

        r = table->view->ops->get_dimensions(table->view, &table->rows, NULL);
        if (r != ERROR_SUCCESS)
        {
            ERR("failed to get table dimensions\n");
            return r;
        }

        /* each table must have at least one row */
        if (table->rows == 0)
        {
            jv->rows = 0;
            return ERROR_SUCCESS;
        }

        if (jv->rows == 0)
            jv->rows = table->rows;
        else
            jv->rows *= table->rows;
    }

    rows = jv->rows;
    LIST_FOR_EACH_ENTRY(table, &jv->tables, JOINTABLE, entry)
    {
        rows /= table->rows;
        table->next_rows = rows;
    }

    return ERROR_SUCCESS;
}

static UINT JOIN_close( struct tagMSIVIEW *view )
{
    MSIJOINVIEW *jv = (MSIJOINVIEW*)view;
    JOINTABLE *table;

    TRACE("%p\n", jv );

    LIST_FOR_EACH_ENTRY(table, &jv->tables, JOINTABLE, entry)
    {
        table->view->ops->close(table->view);
    }

    return ERROR_SUCCESS;
}

static UINT JOIN_get_dimensions( struct tagMSIVIEW *view, UINT *rows, UINT *cols )
{
    MSIJOINVIEW *jv = (MSIJOINVIEW*)view;

    TRACE("%p %p %p\n", jv, rows, cols );

    if (cols)
        *cols = jv->columns;

    if (rows)
        *rows = jv->rows;

    return ERROR_SUCCESS;
}

static UINT JOIN_get_column_info( struct tagMSIVIEW *view,
                UINT n, LPWSTR *name, UINT *type, BOOL *temporary,
                LPWSTR *table_name )
{
    MSIJOINVIEW *jv = (MSIJOINVIEW*)view;
    JOINTABLE *table;
    UINT cols = 0;

    TRACE("%p %d %p %p %p %p\n", jv, n, name, type, temporary, table_name );

    if (n == 0 || n > jv->columns)
        return ERROR_FUNCTION_FAILED;

    LIST_FOR_EACH_ENTRY(table, &jv->tables, JOINTABLE, entry)
    {
        if (n <= cols + table->columns)
            return table->view->ops->get_column_info(table->view, n - cols,
                                                     name, type, temporary,
                                                     table_name);

        cols += table->columns;
    }

    return ERROR_FUNCTION_FAILED;
}

static UINT join_find_row( MSIJOINVIEW *jv, MSIRECORD *rec, UINT *row )
{
    LPCWSTR str;
    UINT i, id, data;

    str = MSI_RecordGetString( rec, 1 );
    msi_string2idW( jv->db->strings, str, &id );

    for (i = 0; i < jv->rows; i++)
    {
        JOIN_fetch_int( &jv->view, i, 1, &data );

        if (data == id)
        {
            *row = i;
            return ERROR_SUCCESS;
        }
    }

    return ERROR_FUNCTION_FAILED;
}

static UINT JOIN_set_row( struct tagMSIVIEW *view, UINT row, MSIRECORD *rec, UINT mask )
{
    MSIJOINVIEW *jv = (MSIJOINVIEW*)view;
    JOINTABLE *table;
    UINT i, reduced_mask = 0, r = ERROR_SUCCESS, offset = 0, col_count;
    MSIRECORD *reduced;

    TRACE("%p %d %p %u %08x\n", jv, row, rec, rec->count, mask );

    if (mask >= 1 << jv->columns)
        return ERROR_INVALID_PARAMETER;

    LIST_FOR_EACH_ENTRY(table, &jv->tables, JOINTABLE, entry)
    {
        r = table->view->ops->get_dimensions( table->view, NULL, &col_count );
        if (r != ERROR_SUCCESS)
            return r;

        reduced = MSI_CreateRecord( col_count );
        if (!reduced)
            return ERROR_FUNCTION_FAILED;

        for (i = 0; i < col_count; i++)
        {
            r = MSI_RecordCopyField( rec, i + offset + 1, reduced, i + 1 );
            if (r != ERROR_SUCCESS)
                break;
        }

        offset += col_count;
        reduced_mask = mask >> (jv->columns - offset) & ((1 << col_count) - 1);

        if (r == ERROR_SUCCESS)
            r = table->view->ops->set_row( table->view, row, reduced, reduced_mask );

        msiobj_release( &reduced->hdr );
    }

    return r;
}

static UINT join_modify_update( struct tagMSIVIEW *view, MSIRECORD *rec )
{
    MSIJOINVIEW *jv = (MSIJOINVIEW *)view;
    UINT r, row;

    r = join_find_row( jv, rec, &row );
    if (r != ERROR_SUCCESS)
        return r;

    return JOIN_set_row( view, row, rec, (1 << jv->columns) - 1 );
}

static UINT JOIN_modify( struct tagMSIVIEW *view, MSIMODIFY mode, MSIRECORD *rec, UINT row )
{
    UINT r;

    TRACE("%p %d %p %u\n", view, mode, rec, row);

    switch (mode)
    {
    case MSIMODIFY_UPDATE:
        return join_modify_update( view, rec );

    case MSIMODIFY_ASSIGN:
    case MSIMODIFY_DELETE:
    case MSIMODIFY_INSERT:
    case MSIMODIFY_INSERT_TEMPORARY:
    case MSIMODIFY_MERGE:
    case MSIMODIFY_REPLACE:
    case MSIMODIFY_SEEK:
    case MSIMODIFY_VALIDATE:
    case MSIMODIFY_VALIDATE_DELETE:
    case MSIMODIFY_VALIDATE_FIELD:
    case MSIMODIFY_VALIDATE_NEW:
        r = ERROR_FUNCTION_FAILED;
        break;

    case MSIMODIFY_REFRESH:
        r = ERROR_CALL_NOT_IMPLEMENTED;
        break;

    default:
        WARN("%p %d %p %u - unknown mode\n", view, mode, rec, row );
        r = ERROR_INVALID_PARAMETER;
        break;
    }

    return r;
}

static UINT JOIN_delete( struct tagMSIVIEW *view )
{
    MSIJOINVIEW *jv = (MSIJOINVIEW*)view;
    struct list *item, *cursor;

    TRACE("%p\n", jv );

    LIST_FOR_EACH_SAFE(item, cursor, &jv->tables)
    {
        JOINTABLE* table = LIST_ENTRY(item, JOINTABLE, entry);

        list_remove(&table->entry);
        table->view->ops->delete(table->view);
        table->view = NULL;
        msi_free(table);
    }

    msi_free(jv);

    return ERROR_SUCCESS;
}

static UINT JOIN_find_matching_rows( struct tagMSIVIEW *view, UINT col,
    UINT val, UINT *row, MSIITERHANDLE *handle )
{
    MSIJOINVIEW *jv = (MSIJOINVIEW*)view;
    UINT i, row_value;

    TRACE("%p, %d, %u, %p\n", view, col, val, *handle);

    if (col == 0 || col > jv->columns)
        return ERROR_INVALID_PARAMETER;

    for (i = PtrToUlong(*handle); i < jv->rows; i++)
    {
        if (view->ops->fetch_int( view, i, col, &row_value ) != ERROR_SUCCESS)
            continue;

        if (row_value == val)
        {
            *row = i;
            (*(UINT *)handle) = i + 1;
            return ERROR_SUCCESS;
        }
    }

    return ERROR_NO_MORE_ITEMS;
}

static UINT JOIN_sort(struct tagMSIVIEW *view, column_info *columns)
{
    MSIJOINVIEW *jv = (MSIJOINVIEW *)view;
    JOINTABLE *table;
    UINT r;

    TRACE("%p %p\n", view, columns);

    LIST_FOR_EACH_ENTRY(table, &jv->tables, JOINTABLE, entry)
    {
        r = table->view->ops->sort(table->view, columns);
        if (r != ERROR_SUCCESS)
            return r;
    }

    return ERROR_SUCCESS;
}

static const MSIVIEWOPS join_ops =
{
    JOIN_fetch_int,
    JOIN_fetch_stream,
    JOIN_get_row,
    NULL,
    NULL,
    NULL,
    JOIN_execute,
    JOIN_close,
    JOIN_get_dimensions,
    JOIN_get_column_info,
    JOIN_modify,
    JOIN_delete,
    JOIN_find_matching_rows,
    NULL,
    NULL,
    NULL,
    NULL,
    JOIN_sort,
    NULL,
};

UINT JOIN_CreateView( MSIDATABASE *db, MSIVIEW **view, LPWSTR tables )
{
    MSIJOINVIEW *jv = NULL;
    UINT r = ERROR_SUCCESS;
    JOINTABLE *table;
    LPWSTR ptr;

    TRACE("%p (%s)\n", jv, debugstr_w(tables) );

    jv = msi_alloc_zero( sizeof *jv );
    if( !jv )
        return ERROR_FUNCTION_FAILED;

    /* fill the structure */
    jv->view.ops = &join_ops;
    jv->db = db;
    jv->columns = 0;
    jv->rows = 0;

    list_init(&jv->tables);

    while (*tables)
    {
        if ((ptr = strchrW(tables, ' ')))
            *ptr = '\0';

        table = msi_alloc(sizeof(JOINTABLE));
        if (!table)
        {
            r = ERROR_OUTOFMEMORY;
            goto end;
        }

        r = TABLE_CreateView( db, tables, &table->view );
        if( r != ERROR_SUCCESS )
        {
            WARN("can't create table: %s\n", debugstr_w(tables));
            msi_free(table);
            r = ERROR_BAD_QUERY_SYNTAX;
            goto end;
        }

        r = table->view->ops->get_dimensions( table->view, NULL,
                                              &table->columns );
        if( r != ERROR_SUCCESS )
        {
            ERR("can't get table dimensions\n");
            goto end;
        }

        jv->columns += table->columns;

        list_add_head( &jv->tables, &table->entry );

        if (!ptr)
            break;

        tables = ptr + 1;
    }

    *view = &jv->view;
    return ERROR_SUCCESS;

end:
    jv->view.ops->delete( &jv->view );

    return r;
}
