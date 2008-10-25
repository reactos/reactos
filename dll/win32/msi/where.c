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

#define MSI_HASH_TABLE_SIZE 37

typedef struct tagMSIHASHENTRY
{
    struct tagMSIHASHENTRY *next;
    UINT value;
    UINT row;
} MSIHASHENTRY;

/* below is the query interface to a table */

typedef struct tagMSIWHEREVIEW
{
    MSIVIEW        view;
    MSIDATABASE   *db;
    MSIVIEW       *table;
    UINT           row_count;
    MSIHASHENTRY **reorder;
    struct expr   *cond;
    UINT           rec_index;
} MSIWHEREVIEW;

static void free_hash_table(MSIHASHENTRY **table)
{
    MSIHASHENTRY *new, *old;
    int i;

    if (!table)
        return;

    for (i = 0; i < MSI_HASH_TABLE_SIZE; i++)
    {
        new = table[i];

        while (new)
        {
            old = new;
            new = old->next;
            msi_free(old);
        }

        table[i] = NULL;
    }

    msi_free(table);
}

static UINT find_entry_in_hash(MSIHASHENTRY **table, UINT row, UINT *val)
{
    MSIHASHENTRY *entry;

    if (!table)
        return ERROR_SUCCESS;

    if (!(entry = table[row % MSI_HASH_TABLE_SIZE]))
    {
        WARN("Row not found in hash table!\n");
        return ERROR_FUNCTION_FAILED;
    }

    while (entry && entry->row != row)
        entry = entry->next;

    if (entry) *val = entry->value;
    return ERROR_SUCCESS;
}

static UINT add_entry_to_hash(MSIHASHENTRY **table, UINT row, UINT val)
{
    MSIHASHENTRY *new = msi_alloc(sizeof(MSIHASHENTRY));
    MSIHASHENTRY *prev;

    if (!new)
        return ERROR_OUTOFMEMORY;

    new->next = NULL;
    new->value = val;
    new->row = row;

    prev = table[row % MSI_HASH_TABLE_SIZE];
    if (prev)
        new->next = prev;

    table[row % MSI_HASH_TABLE_SIZE] = new;

    return ERROR_SUCCESS;
}

static UINT WHERE_fetch_int( struct tagMSIVIEW *view, UINT row, UINT col, UINT *val )
{
    MSIWHEREVIEW *wv = (MSIWHEREVIEW*)view;
    UINT r;

    TRACE("%p %d %d %p\n", wv, row, col, val );

    if( !wv->table )
        return ERROR_FUNCTION_FAILED;

    if( row > wv->row_count )
        return ERROR_NO_MORE_ITEMS;

    r = find_entry_in_hash(wv->reorder, row, &row);
    if (r != ERROR_SUCCESS)
        return r;

    return wv->table->ops->fetch_int( wv->table, row, col, val );
}

static UINT WHERE_fetch_stream( struct tagMSIVIEW *view, UINT row, UINT col, IStream **stm )
{
    MSIWHEREVIEW *wv = (MSIWHEREVIEW*)view;
    UINT r;

    TRACE("%p %d %d %p\n", wv, row, col, stm );

    if( !wv->table )
        return ERROR_FUNCTION_FAILED;

    if( row > wv->row_count )
        return ERROR_NO_MORE_ITEMS;

    r = find_entry_in_hash(wv->reorder, row, &row);
    if (r != ERROR_SUCCESS)
        return r;

    return wv->table->ops->fetch_stream( wv->table, row, col, stm );
}

static UINT WHERE_get_row( struct tagMSIVIEW *view, UINT row, MSIRECORD **rec )
{
    MSIWHEREVIEW *wv = (MSIWHEREVIEW *)view;
    UINT r;

    TRACE("%p %d %p\n", wv, row, rec );

    if (!wv->table)
        return ERROR_FUNCTION_FAILED;

    if (row > wv->row_count)
        return ERROR_NO_MORE_ITEMS;

    r = find_entry_in_hash(wv->reorder, row, &row);
    if (r != ERROR_SUCCESS)
        return r;

    return wv->table->ops->get_row(view, row, rec);
}

static UINT WHERE_set_row( struct tagMSIVIEW *view, UINT row, MSIRECORD *rec, UINT mask )
{
    MSIWHEREVIEW *wv = (MSIWHEREVIEW*)view;
    UINT r;

    TRACE("%p %d %p %08x\n", wv, row, rec, mask );

    if( !wv->table )
         return ERROR_FUNCTION_FAILED;

    if( row > wv->row_count )
        return ERROR_NO_MORE_ITEMS;

    r = find_entry_in_hash(wv->reorder, row, &row);
    if (r != ERROR_SUCCESS)
        return r;

    return wv->table->ops->set_row( wv->table, row, rec, mask );
}

static UINT WHERE_delete_row(struct tagMSIVIEW *view, UINT row)
{
    MSIWHEREVIEW *wv = (MSIWHEREVIEW *)view;
    UINT r;

    TRACE("(%p %d)\n", view, row);

    if ( !wv->table )
        return ERROR_FUNCTION_FAILED;

    if ( row > wv->row_count )
        return ERROR_NO_MORE_ITEMS;

    r = find_entry_in_hash( wv->reorder, row, &row );
    if ( r != ERROR_SUCCESS )
        return r;

    return wv->table->ops->delete_row( wv->table, row );
}

static INT INT_evaluate_binary( INT lval, UINT op, INT rval )
{
    switch( op )
    {
    case OP_EQ:
        return ( lval == rval );
    case OP_AND:
        return ( lval && rval );
    case OP_OR:
        return ( lval || rval );
    case OP_GT:
        return ( lval > rval );
    case OP_LT:
        return ( lval < rval );
    case OP_LE:
        return ( lval <= rval );
    case OP_GE:
        return ( lval >= rval );
    case OP_NE:
        return ( lval != rval );
    default:
        ERR("Unknown operator %d\n", op );
    }
    return 0;
}

static INT INT_evaluate_unary( INT lval, UINT op )
{
    switch( op )
    {
    case OP_ISNULL:
        return ( !lval );
    case OP_NOTNULL:
        return ( lval );
    default:
        ERR("Unknown operator %d\n", op );
    }
    return 0;
}

static const WCHAR *STRING_evaluate( MSIWHEREVIEW *wv, UINT row,
                                     const struct expr *expr,
                                     const MSIRECORD *record )
{
    UINT val = 0, r;

    switch( expr->type )
    {
    case EXPR_COL_NUMBER_STRING:
        r = wv->table->ops->fetch_int( wv->table, row, expr->u.col_number, &val );
        if( r != ERROR_SUCCESS )
            return NULL;
        return msi_string_lookup_id( wv->db->strings, val );

    case EXPR_SVAL:
        return expr->u.sval;

    case EXPR_WILDCARD:
        return MSI_RecordGetString( record, ++wv->rec_index );

    default:
        ERR("Invalid expression type\n");
        break;
    }
    return NULL;
}

static UINT STRCMP_Evaluate( MSIWHEREVIEW *wv, UINT row, const struct expr *cond,
                             INT *val, const MSIRECORD *record )
{
    int sr;
    const WCHAR *l_str, *r_str;

    l_str = STRING_evaluate( wv, row, cond->u.expr.left, record );
    r_str = STRING_evaluate( wv, row, cond->u.expr.right, record );
    if( l_str == r_str ||
        ((!l_str || !*l_str) && (!r_str || !*r_str)) )
        sr = 0;
    else if( l_str && ! r_str )
        sr = 1;
    else if( r_str && ! l_str )
        sr = -1;
    else
        sr = lstrcmpW( l_str, r_str );

    *val = ( cond->u.expr.op == OP_EQ && ( sr == 0 ) ) ||
           ( cond->u.expr.op == OP_NE && ( sr != 0 ) ) ||
           ( cond->u.expr.op == OP_LT && ( sr < 0 ) ) ||
           ( cond->u.expr.op == OP_GT && ( sr > 0 ) );

    return ERROR_SUCCESS;
}

static UINT WHERE_evaluate( MSIWHEREVIEW *wv, UINT row,
                            struct expr *cond, INT *val, MSIRECORD *record )
{
    UINT r, tval;
    INT lval, rval;

    if( !cond )
        return ERROR_SUCCESS;

    switch( cond->type )
    {
    case EXPR_COL_NUMBER:
        r = wv->table->ops->fetch_int( wv->table, row, cond->u.col_number, &tval );
        *val = tval - 0x8000;
        return ERROR_SUCCESS;

    case EXPR_COL_NUMBER32:
        r = wv->table->ops->fetch_int( wv->table, row, cond->u.col_number, &tval );
        *val = tval - 0x80000000;
        return r;

    case EXPR_UVAL:
        *val = cond->u.uval;
        return ERROR_SUCCESS;

    case EXPR_COMPLEX:
        r = WHERE_evaluate( wv, row, cond->u.expr.left, &lval, record );
        if( r != ERROR_SUCCESS )
            return r;
        r = WHERE_evaluate( wv, row, cond->u.expr.right, &rval, record );
        if( r != ERROR_SUCCESS )
            return r;
        *val = INT_evaluate_binary( lval, cond->u.expr.op, rval );
        return ERROR_SUCCESS;

    case EXPR_UNARY:
        r = wv->table->ops->fetch_int( wv->table, row, cond->u.expr.left->u.col_number, &tval );
        if( r != ERROR_SUCCESS )
            return r;
        *val = INT_evaluate_unary( tval, cond->u.expr.op );
        return ERROR_SUCCESS;

    case EXPR_STRCMP:
        return STRCMP_Evaluate( wv, row, cond, val, record );

    case EXPR_WILDCARD:
        *val = MSI_RecordGetInteger( record, ++wv->rec_index );
        return ERROR_SUCCESS;

    default:
        ERR("Invalid expression type\n");
        break;
    }

    return ERROR_SUCCESS;
}

static UINT WHERE_execute( struct tagMSIVIEW *view, MSIRECORD *record )
{
    MSIWHEREVIEW *wv = (MSIWHEREVIEW*)view;
    UINT count = 0, r, i;
    INT val;
    MSIVIEW *table = wv->table;

    TRACE("%p %p\n", wv, record);

    if( !table )
         return ERROR_FUNCTION_FAILED;

    r = table->ops->execute( table, record );
    if( r != ERROR_SUCCESS )
        return r;

    r = table->ops->get_dimensions( table, &count, NULL );
    if( r != ERROR_SUCCESS )
        return r;

    free_hash_table(wv->reorder);
    wv->reorder = msi_alloc_zero(MSI_HASH_TABLE_SIZE * sizeof(MSIHASHENTRY *));
    if( !wv->reorder )
        return ERROR_OUTOFMEMORY;

    wv->row_count = 0;
    if (wv->cond->type == EXPR_STRCMP)
    {
        MSIITERHANDLE handle = NULL;
        UINT row, value, col;
        struct expr *col_cond = wv->cond->u.expr.left;
        struct expr *val_cond = wv->cond->u.expr.right;

        /* swap conditionals */
        if (col_cond->type != EXPR_COL_NUMBER_STRING)
        {
            val_cond = wv->cond->u.expr.left;
            col_cond = wv->cond->u.expr.right;
        }

        if ((col_cond->type == EXPR_COL_NUMBER_STRING) && (val_cond->type == EXPR_SVAL))
        {
            col = col_cond->u.col_number;
            /* special case for "" - translate it into nil */
            if (!val_cond->u.sval[0])
                value = 0;
            else
            {
                r = msi_string2idW(wv->db->strings, val_cond->u.sval, &value);
                if (r != ERROR_SUCCESS)
                {
                    TRACE("no id for %s, assuming it doesn't exist in the table\n", debugstr_w(wv->cond->u.expr.right->u.sval));
                    return ERROR_SUCCESS;
                }
            }

            do
            {
                r = table->ops->find_matching_rows(table, col, value, &row, &handle);
                if (r == ERROR_SUCCESS)
                    add_entry_to_hash(wv->reorder, wv->row_count++, row);
            } while (r == ERROR_SUCCESS);

            if (r == ERROR_NO_MORE_ITEMS)
                return ERROR_SUCCESS;
            else
                return r;
        }
        /* else fallback to slow case */
    }

    for( i=0; i<count; i++ )
    {
        val = 0;
        wv->rec_index = 0;
        r = WHERE_evaluate( wv, i, wv->cond, &val, record );
        if( r != ERROR_SUCCESS )
            return r;
        if( val )
            add_entry_to_hash( wv->reorder, wv->row_count++, i );
    }

    return ERROR_SUCCESS;
}

static UINT WHERE_close( struct tagMSIVIEW *view )
{
    MSIWHEREVIEW *wv = (MSIWHEREVIEW*)view;

    TRACE("%p\n", wv );

    if( !wv->table )
        return ERROR_FUNCTION_FAILED;

    return wv->table->ops->close( wv->table );
}

static UINT WHERE_get_dimensions( struct tagMSIVIEW *view, UINT *rows, UINT *cols )
{
    MSIWHEREVIEW *wv = (MSIWHEREVIEW*)view;

    TRACE("%p %p %p\n", wv, rows, cols );

    if( !wv->table )
         return ERROR_FUNCTION_FAILED;

    if( rows )
    {
        if( !wv->reorder )
            return ERROR_FUNCTION_FAILED;
        *rows = wv->row_count;
    }

    return wv->table->ops->get_dimensions( wv->table, NULL, cols );
}

static UINT WHERE_get_column_info( struct tagMSIVIEW *view,
                UINT n, LPWSTR *name, UINT *type )
{
    MSIWHEREVIEW *wv = (MSIWHEREVIEW*)view;

    TRACE("%p %d %p %p\n", wv, n, name, type );

    if( !wv->table )
         return ERROR_FUNCTION_FAILED;

    return wv->table->ops->get_column_info( wv->table, n, name, type );
}

static UINT WHERE_modify( struct tagMSIVIEW *view, MSIMODIFY eModifyMode,
                          MSIRECORD *rec, UINT row )
{
    MSIWHEREVIEW *wv = (MSIWHEREVIEW*)view;

    TRACE("%p %d %p\n", wv, eModifyMode, rec);

    find_entry_in_hash(wv->reorder, row - 1, &row);
    row++;

    return wv->table->ops->modify( wv->table, eModifyMode, rec, row );
}

static UINT WHERE_delete( struct tagMSIVIEW *view )
{
    MSIWHEREVIEW *wv = (MSIWHEREVIEW*)view;

    TRACE("%p\n", wv );

    if( wv->table )
        wv->table->ops->delete( wv->table );
    wv->table = 0;

    free_hash_table(wv->reorder);
    wv->reorder = NULL;
    wv->row_count = 0;

    msiobj_release( &wv->db->hdr );
    msi_free( wv );

    return ERROR_SUCCESS;
}

static UINT WHERE_find_matching_rows( struct tagMSIVIEW *view, UINT col,
    UINT val, UINT *row, MSIITERHANDLE *handle )
{
    MSIWHEREVIEW *wv = (MSIWHEREVIEW*)view;
    UINT r;

    TRACE("%p, %d, %u, %p\n", view, col, val, *handle);

    if( !wv->table )
         return ERROR_FUNCTION_FAILED;

    r = wv->table->ops->find_matching_rows( wv->table, col, val, row, handle );
    if (r != ERROR_SUCCESS)
        return r;

    if( *row > wv->row_count )
        return ERROR_NO_MORE_ITEMS;

    return find_entry_in_hash(wv->reorder, *row, row);
}

static UINT WHERE_sort(struct tagMSIVIEW *view, column_info *columns)
{
    MSIWHEREVIEW *wv = (MSIWHEREVIEW *)view;

    TRACE("%p %p\n", view, columns);

    return wv->table->ops->sort(wv->table, columns);
}

static const MSIVIEWOPS where_ops =
{
    WHERE_fetch_int,
    WHERE_fetch_stream,
    WHERE_get_row,
    WHERE_set_row,
    NULL,
    WHERE_delete_row,
    WHERE_execute,
    WHERE_close,
    WHERE_get_dimensions,
    WHERE_get_column_info,
    WHERE_modify,
    WHERE_delete,
    WHERE_find_matching_rows,
    NULL,
    NULL,
    NULL,
    NULL,
    WHERE_sort,
};

static UINT WHERE_VerifyCondition( MSIDATABASE *db, MSIVIEW *table, struct expr *cond,
                                   UINT *valid )
{
    UINT r, val = 0;

    switch( cond->type )
    {
    case EXPR_COLUMN:
        r = VIEW_find_column( table, cond->u.column, &val );
        if( r == ERROR_SUCCESS )
        {
            UINT type = 0;
            r = table->ops->get_column_info( table, val, NULL, &type );
            if( r == ERROR_SUCCESS )
            {
                if (type&MSITYPE_STRING)
                    cond->type = EXPR_COL_NUMBER_STRING;
                else if ((type&0xff) == 4)
                    cond->type = EXPR_COL_NUMBER32;
                else
                    cond->type = EXPR_COL_NUMBER;
                cond->u.col_number = val;
                *valid = 1;
            }
            else
                *valid = 0;
        }
        else
        {
            *valid = 0;
            WARN("Couldn't find column %s\n", debugstr_w( cond->u.column ) );
        }
        break;
    case EXPR_COMPLEX:
        r = WHERE_VerifyCondition( db, table, cond->u.expr.left, valid );
        if( r != ERROR_SUCCESS )
            return r;
        if( !*valid )
            return ERROR_SUCCESS;
        r = WHERE_VerifyCondition( db, table, cond->u.expr.right, valid );
        if( r != ERROR_SUCCESS )
            return r;

        /* check the type of the comparison */
        if( ( cond->u.expr.left->type == EXPR_SVAL ) ||
            ( cond->u.expr.left->type == EXPR_COL_NUMBER_STRING ) ||
            ( cond->u.expr.right->type == EXPR_SVAL ) ||
            ( cond->u.expr.right->type == EXPR_COL_NUMBER_STRING ) )
        {
            switch( cond->u.expr.op )
            {
            case OP_EQ:
            case OP_GT:
            case OP_LT:
            case OP_NE:
                break;
            default:
                *valid = FALSE;
                return ERROR_INVALID_PARAMETER;
            }

            /* FIXME: check we're comparing a string to a column */

            cond->type = EXPR_STRCMP;
        }

        break;
    case EXPR_UNARY:
        if ( cond->u.expr.left->type != EXPR_COLUMN )
        {
            *valid = FALSE;
            return ERROR_INVALID_PARAMETER;
        }
        r = WHERE_VerifyCondition( db, table, cond->u.expr.left, valid );
        if( r != ERROR_SUCCESS )
            return r;
        break;
    case EXPR_IVAL:
        *valid = 1;
        cond->type = EXPR_UVAL;
        cond->u.uval = cond->u.ival;
        break;
    case EXPR_WILDCARD:
        *valid = 1;
        break;
    case EXPR_SVAL:
        *valid = 1;
        break;
    default:
        ERR("Invalid expression type\n");
        *valid = 0;
        break;
    }

    return ERROR_SUCCESS;
}

UINT WHERE_CreateView( MSIDATABASE *db, MSIVIEW **view, MSIVIEW *table,
                       struct expr *cond )
{
    MSIWHEREVIEW *wv = NULL;
    UINT count = 0, r, valid = 0;

    TRACE("%p\n", table );

    r = table->ops->get_dimensions( table, NULL, &count );
    if( r != ERROR_SUCCESS )
    {
        ERR("can't get table dimensions\n");
        return r;
    }

    if( cond )
    {
        r = WHERE_VerifyCondition( db, table, cond, &valid );
        if( r != ERROR_SUCCESS )
            return r;
        if( !valid )
            return ERROR_FUNCTION_FAILED;
    }

    wv = msi_alloc_zero( sizeof *wv );
    if( !wv )
        return ERROR_FUNCTION_FAILED;
    
    /* fill the structure */
    wv->view.ops = &where_ops;
    msiobj_addref( &db->hdr );
    wv->db = db;
    wv->table = table;
    wv->row_count = 0;
    wv->reorder = NULL;
    wv->cond = cond;
    wv->rec_index = 0;
    *view = (MSIVIEW*) wv;

    return ERROR_SUCCESS;
}
