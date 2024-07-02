/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2002 Mike McCormack for CodeWeavers
 * Copyright 2011 Bernhard Loos
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
#include <assert.h>

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
struct row_entry
{
    struct tagMSIWHEREVIEW *wv; /* used during sorting */
    UINT values[1];
};

struct join_table
{
    struct join_table *next;
    MSIVIEW *view;
    UINT col_count;
    UINT row_count;
    UINT table_index;
};

typedef struct tagMSIORDERINFO
{
    UINT col_count;
    UINT error;
    union ext_column columns[1];
} MSIORDERINFO;

typedef struct tagMSIWHEREVIEW
{
    MSIVIEW        view;
    MSIDATABASE   *db;
    struct join_table *tables;
    UINT           row_count;
    UINT           col_count;
    UINT           table_count;
    struct row_entry **reorder;
    UINT           reorder_size; /* number of entries available in reorder */
    struct expr   *cond;
    UINT           rec_index;
    MSIORDERINFO  *order_info;
} MSIWHEREVIEW;

static UINT WHERE_evaluate( MSIWHEREVIEW *wv, const UINT rows[],
                            struct expr *cond, INT *val, MSIRECORD *record );

#define INITIAL_REORDER_SIZE 16

#define INVALID_ROW_INDEX (-1)

static void free_reorder(MSIWHEREVIEW *wv)
{
    UINT i;

    if (!wv->reorder)
        return;

    for (i = 0; i < wv->row_count; i++)
        free(wv->reorder[i]);

    free(wv->reorder);
    wv->reorder = NULL;
    wv->reorder_size = 0;
    wv->row_count = 0;
}

static UINT init_reorder(MSIWHEREVIEW *wv)
{
    struct row_entry **new = calloc(INITIAL_REORDER_SIZE, sizeof(*new));
    if (!new)
        return ERROR_OUTOFMEMORY;

    free_reorder(wv);

    wv->reorder = new;
    wv->reorder_size = INITIAL_REORDER_SIZE;

    return ERROR_SUCCESS;
}

static inline UINT find_row(MSIWHEREVIEW *wv, UINT row, UINT *(values[]))
{
    if (row >= wv->row_count)
        return ERROR_NO_MORE_ITEMS;

    *values = wv->reorder[row]->values;

    return ERROR_SUCCESS;
}

static UINT add_row(MSIWHEREVIEW *wv, UINT vals[])
{
    struct row_entry *new;

    if (wv->reorder_size <= wv->row_count)
    {
        struct row_entry **new_reorder;
        UINT newsize = wv->reorder_size * 2;

        new_reorder = realloc(wv->reorder, newsize * sizeof(*new_reorder));
        if (!new_reorder)
            return ERROR_OUTOFMEMORY;
        memset(new_reorder + wv->reorder_size, 0, (newsize - wv->reorder_size) * sizeof(*new_reorder));

        wv->reorder = new_reorder;
        wv->reorder_size = newsize;
    }

    new = malloc(offsetof(struct row_entry, values[wv->table_count]));

    if (!new)
        return ERROR_OUTOFMEMORY;

    wv->reorder[wv->row_count++] = new;

    memcpy(new->values, vals, wv->table_count * sizeof(UINT));
    new->wv = wv;

    return ERROR_SUCCESS;
}

static struct join_table *find_table(MSIWHEREVIEW *wv, UINT col, UINT *table_col)
{
    struct join_table *table = wv->tables;

    if(col == 0 || col > wv->col_count)
         return NULL;

    while (col > table->col_count)
    {
        col -= table->col_count;
        table = table->next;
        assert(table);
    }

    *table_col = col;
    return table;
}

static UINT parse_column(MSIWHEREVIEW *wv, union ext_column *column,
                         UINT *column_type)
{
    struct join_table *table = wv->tables;
    UINT i, r;

    do
    {
        LPCWSTR table_name;

        if (column->unparsed.table)
        {
            r = table->view->ops->get_column_info(table->view, 1, NULL, NULL,
                                                  NULL, &table_name);
            if (r != ERROR_SUCCESS)
                return r;
            if (wcscmp(table_name, column->unparsed.table) != 0)
                continue;
        }

        for(i = 1; i <= table->col_count; i++)
        {
            LPCWSTR col_name;

            r = table->view->ops->get_column_info(table->view, i, &col_name, column_type,
                                                  NULL, NULL);
            if(r != ERROR_SUCCESS )
                return r;

            if(wcscmp(col_name, column->unparsed.column))
                continue;
            column->parsed.column = i;
            column->parsed.table = table;
            return ERROR_SUCCESS;
        }
    }
    while ((table = table->next));

    WARN("Couldn't find column %s.%s\n", debugstr_w( column->unparsed.table ), debugstr_w( column->unparsed.column ) );
    return ERROR_BAD_QUERY_SYNTAX;
}

static UINT WHERE_fetch_int( struct tagMSIVIEW *view, UINT row, UINT col, UINT *val )
{
    MSIWHEREVIEW *wv = (MSIWHEREVIEW*)view;
    struct join_table *table;
    UINT *rows;
    UINT r;

    TRACE("%p %d %d %p\n", wv, row, col, val );

    if( !wv->tables )
        return ERROR_FUNCTION_FAILED;

    r = find_row(wv, row, &rows);
    if (r != ERROR_SUCCESS)
        return r;

    table = find_table(wv, col, &col);
    if (!table)
        return ERROR_FUNCTION_FAILED;

    return table->view->ops->fetch_int(table->view, rows[table->table_index], col, val);
}

static UINT WHERE_fetch_stream( struct tagMSIVIEW *view, UINT row, UINT col, IStream **stm )
{
    MSIWHEREVIEW *wv = (MSIWHEREVIEW*)view;
    struct join_table *table;
    UINT *rows;
    UINT r;

    TRACE("%p %d %d %p\n", wv, row, col, stm );

    if( !wv->tables )
        return ERROR_FUNCTION_FAILED;

    r = find_row(wv, row, &rows);
    if (r != ERROR_SUCCESS)
        return r;

    table = find_table(wv, col, &col);
    if (!table)
        return ERROR_FUNCTION_FAILED;

    return table->view->ops->fetch_stream( table->view, rows[table->table_index], col, stm );
}

static UINT WHERE_set_int(struct tagMSIVIEW *view, UINT row, UINT col, int val)
{
    MSIWHEREVIEW *wv = (MSIWHEREVIEW*)view;
    struct join_table *table;
    UINT *rows;
    UINT r;

    TRACE("view %p, row %u, col %u, val %d.\n", wv, row, col, val );

    r = find_row(wv, row, &rows);
    if (r != ERROR_SUCCESS)
        return r;

    table = find_table(wv, col, &col);
    if (!table)
        return ERROR_FUNCTION_FAILED;

    return table->view->ops->set_int(table->view, rows[table->table_index], col, val);
}

static UINT WHERE_set_string(struct tagMSIVIEW *view, UINT row, UINT col, const WCHAR *val, int len)
{
    MSIWHEREVIEW *wv = (MSIWHEREVIEW*)view;
    struct join_table *table;
    UINT *rows;
    UINT r;

    TRACE("view %p, row %u, col %u, val %s.\n", wv, row, col, debugstr_wn(val, len));

    r = find_row(wv, row, &rows);
    if (r != ERROR_SUCCESS)
        return r;

    table = find_table(wv, col, &col);
    if (!table)
        return ERROR_FUNCTION_FAILED;

    return table->view->ops->set_string(table->view, rows[table->table_index], col, val, len);
}

static UINT WHERE_set_stream(MSIVIEW *view, UINT row, UINT col, IStream *stream)
{
    MSIWHEREVIEW *wv = (MSIWHEREVIEW*)view;
    struct join_table *table;
    UINT *rows;
    UINT r;

    TRACE("view %p, row %u, col %u, stream %p.\n", wv, row, col, stream);

    r = find_row(wv, row, &rows);
    if (r != ERROR_SUCCESS)
        return r;

    table = find_table(wv, col, &col);
    if (!table)
        return ERROR_FUNCTION_FAILED;

    return table->view->ops->set_stream(table->view, rows[table->table_index], col, stream);
}

static UINT WHERE_set_row( struct tagMSIVIEW *view, UINT row, MSIRECORD *rec, UINT mask )
{
    MSIWHEREVIEW *wv = (MSIWHEREVIEW*)view;
    UINT i, r, offset = 0;
    struct join_table *table = wv->tables;
    UINT *rows;
    UINT mask_copy = mask;

    TRACE("%p %d %p %08x\n", wv, row, rec, mask );

    if( !wv->tables )
         return ERROR_FUNCTION_FAILED;

    r = find_row(wv, row, &rows);
    if (r != ERROR_SUCCESS)
        return r;

    if (mask >= 1 << wv->col_count)
        return ERROR_INVALID_PARAMETER;

    do
    {
        for (i = 0; i < table->col_count; i++) {
            UINT type;

            if (!(mask_copy & (1 << i)))
                continue;
            r = table->view->ops->get_column_info(table->view, i + 1, NULL,
                                            &type, NULL, NULL );
            if (r != ERROR_SUCCESS)
                return r;
            if (type & MSITYPE_KEY)
                return ERROR_FUNCTION_FAILED;
        }
        mask_copy >>= table->col_count;
    }
    while (mask_copy && (table = table->next));

    table = wv->tables;

    do
    {
        const UINT col_count = table->col_count;
        UINT i;
        MSIRECORD *reduced;
        UINT reduced_mask = (mask >> offset) & ((1 << col_count) - 1);

        if (!reduced_mask)
        {
            offset += col_count;
            continue;
        }

        reduced = MSI_CreateRecord(col_count);
        if (!reduced)
            return ERROR_FUNCTION_FAILED;

        for (i = 1; i <= col_count; i++)
        {
            r = MSI_RecordCopyField(rec, i + offset, reduced, i);
            if (r != ERROR_SUCCESS)
                break;
        }

        offset += col_count;

        if (r == ERROR_SUCCESS)
            r = table->view->ops->set_row(table->view, rows[table->table_index], reduced, reduced_mask);

        msiobj_release(&reduced->hdr);
    }
    while ((table = table->next));
    return r;
}

static UINT WHERE_delete_row(struct tagMSIVIEW *view, UINT row)
{
    MSIWHEREVIEW *wv = (MSIWHEREVIEW *)view;
    UINT r;
    UINT *rows;

    TRACE("(%p %d)\n", view, row);

    if (!wv->tables)
        return ERROR_FUNCTION_FAILED;

    r = find_row(wv, row, &rows);
    if ( r != ERROR_SUCCESS )
        return r;

    if (wv->table_count > 1)
        return ERROR_CALL_NOT_IMPLEMENTED;

    return wv->tables->view->ops->delete_row(wv->tables->view, rows[0]);
}

static INT INT_evaluate_binary( MSIWHEREVIEW *wv, const UINT rows[],
                                const struct complex_expr *expr, INT *val, MSIRECORD *record )
{
    UINT rl, rr;
    INT lval, rval;

    rl = WHERE_evaluate(wv, rows, expr->left, &lval, record);
    if (rl != ERROR_SUCCESS && rl != ERROR_CONTINUE)
        return rl;
    rr = WHERE_evaluate(wv, rows, expr->right, &rval, record);
    if (rr != ERROR_SUCCESS && rr != ERROR_CONTINUE)
        return rr;

    if (rl == ERROR_CONTINUE || rr == ERROR_CONTINUE)
    {
        if (rl == rr)
        {
            *val = TRUE;
            return ERROR_CONTINUE;
        }

        if (expr->op == OP_AND)
        {
            if ((rl == ERROR_CONTINUE && !rval) || (rr == ERROR_CONTINUE && !lval))
            {
                *val = FALSE;
                return ERROR_SUCCESS;
            }
        }
        else if (expr->op == OP_OR)
        {
            if ((rl == ERROR_CONTINUE && rval) || (rr == ERROR_CONTINUE && lval))
            {
                *val = TRUE;
                return ERROR_SUCCESS;
            }
        }

        *val = TRUE;
        return ERROR_CONTINUE;
    }

    switch( expr->op )
    {
    case OP_EQ:
        *val = ( lval == rval );
        break;
    case OP_AND:
        *val = ( lval && rval );
        break;
    case OP_OR:
        *val = ( lval || rval );
        break;
    case OP_GT:
        *val = ( lval > rval );
        break;
    case OP_LT:
        *val = ( lval < rval );
        break;
    case OP_LE:
        *val = ( lval <= rval );
        break;
    case OP_GE:
        *val = ( lval >= rval );
        break;
    case OP_NE:
        *val = ( lval != rval );
        break;
    default:
        ERR("Unknown operator %d\n", expr->op );
        return ERROR_FUNCTION_FAILED;
    }

    return ERROR_SUCCESS;
}

static inline UINT expr_fetch_value(const union ext_column *expr, const UINT rows[], UINT *val)
{
    struct join_table *table = expr->parsed.table;

    if( rows[table->table_index] == INVALID_ROW_INDEX )
    {
        *val = 1;
        return ERROR_CONTINUE;
    }
    return table->view->ops->fetch_int(table->view, rows[table->table_index],
                                        expr->parsed.column, val);
}


static UINT INT_evaluate_unary( MSIWHEREVIEW *wv, const UINT rows[],
                                const struct complex_expr *expr, INT *val, MSIRECORD *record )
{
    UINT r;
    UINT lval;

    r = expr_fetch_value(&expr->left->u.column, rows, &lval);
    if(r != ERROR_SUCCESS)
        return r;

    switch( expr->op )
    {
    case OP_ISNULL:
        *val = !lval;
        break;
    case OP_NOTNULL:
        *val = lval;
        break;
    default:
        ERR("Unknown operator %d\n", expr->op );
        return ERROR_FUNCTION_FAILED;
    }
    return ERROR_SUCCESS;
}

static UINT STRING_evaluate( MSIWHEREVIEW *wv, const UINT rows[],
                                     const struct expr *expr,
                                     const MSIRECORD *record,
                                     const WCHAR **str )
{
    UINT val = 0, r = ERROR_SUCCESS;

    switch( expr->type )
    {
    case EXPR_COL_NUMBER_STRING:
        r = expr_fetch_value(&expr->u.column, rows, &val);
        if (r == ERROR_SUCCESS)
            *str =  msi_string_lookup(wv->db->strings, val, NULL);
        else
            *str = NULL;
        break;

    case EXPR_SVAL:
        *str = expr->u.sval;
        break;

    case EXPR_WILDCARD:
        *str = MSI_RecordGetString(record, ++wv->rec_index);
        break;

    default:
        ERR("Invalid expression type\n");
        r = ERROR_FUNCTION_FAILED;
        *str = NULL;
        break;
    }
    return r;
}

static UINT STRCMP_Evaluate( MSIWHEREVIEW *wv, const UINT rows[], const struct complex_expr *expr,
                             INT *val, const MSIRECORD *record )
{
    int sr;
    const WCHAR *l_str, *r_str;
    UINT r;

    *val = TRUE;
    r = STRING_evaluate(wv, rows, expr->left, record, &l_str);
    if (r == ERROR_CONTINUE)
        return r;
    r = STRING_evaluate(wv, rows, expr->right, record, &r_str);
    if (r == ERROR_CONTINUE)
        return r;

    if( l_str == r_str ||
        ((!l_str || !*l_str) && (!r_str || !*r_str)) )
        sr = 0;
    else if( l_str && ! r_str )
        sr = 1;
    else if( r_str && ! l_str )
        sr = -1;
    else
        sr = wcscmp( l_str, r_str );

    *val = ( expr->op == OP_EQ && ( sr == 0 ) ) ||
           ( expr->op == OP_NE && ( sr != 0 ) );

    return ERROR_SUCCESS;
}

static UINT WHERE_evaluate( MSIWHEREVIEW *wv, const UINT rows[],
                            struct expr *cond, INT *val, MSIRECORD *record )
{
    UINT r, tval;

    if( !cond )
    {
        *val = TRUE;
        return ERROR_SUCCESS;
    }

    switch( cond->type )
    {
    case EXPR_COL_NUMBER:
        r = expr_fetch_value(&cond->u.column, rows, &tval);
        if( r != ERROR_SUCCESS )
            return r;
        *val = tval - 0x8000;
        return ERROR_SUCCESS;

    case EXPR_COL_NUMBER32:
        r = expr_fetch_value(&cond->u.column, rows, &tval);
        if( r != ERROR_SUCCESS )
            return r;
        *val = tval - 0x80000000;
        return r;

    case EXPR_UVAL:
        *val = cond->u.uval;
        return ERROR_SUCCESS;

    case EXPR_COMPLEX:
        return INT_evaluate_binary(wv, rows, &cond->u.expr, val, record);

    case EXPR_UNARY:
        return INT_evaluate_unary( wv, rows, &cond->u.expr, val, record );

    case EXPR_STRCMP:
        return STRCMP_Evaluate( wv, rows, &cond->u.expr, val, record );

    case EXPR_WILDCARD:
        *val = MSI_RecordGetInteger( record, ++wv->rec_index );
        return ERROR_SUCCESS;

    default:
        ERR("Invalid expression type\n");
        return ERROR_FUNCTION_FAILED;
    }

    return ERROR_SUCCESS;
}

static UINT check_condition( MSIWHEREVIEW *wv, MSIRECORD *record, struct join_table **tables,
                             UINT table_rows[] )
{
    UINT r = ERROR_FUNCTION_FAILED;
    INT val;

    for (table_rows[(*tables)->table_index] = 0;
         table_rows[(*tables)->table_index] < (*tables)->row_count;
         table_rows[(*tables)->table_index]++)
    {
        val = 0;
        wv->rec_index = 0;
        r = WHERE_evaluate( wv, table_rows, wv->cond, &val, record );
        if (r != ERROR_SUCCESS && r != ERROR_CONTINUE)
            break;
        if (val)
        {
            if (*(tables + 1))
            {
                r = check_condition(wv, record, tables + 1, table_rows);
                if (r != ERROR_SUCCESS)
                    break;
            }
            else
            {
                if (r != ERROR_SUCCESS)
                    break;
                add_row (wv, table_rows);
            }
        }
    }
    table_rows[(*tables)->table_index] = INVALID_ROW_INDEX;
    return r;
}

static int __cdecl compare_entry( const void *left, const void *right )
{
    const struct row_entry *le = *(const struct row_entry **)left;
    const struct row_entry *re = *(const struct row_entry **)right;
    const MSIWHEREVIEW *wv = le->wv;
    MSIORDERINFO *order = wv->order_info;
    UINT i, j, r, l_val, r_val;

    assert(le->wv == re->wv);

    if (order)
    {
        for (i = 0; i < order->col_count; i++)
        {
            const union ext_column *column = &order->columns[i];

            r = column->parsed.table->view->ops->fetch_int(column->parsed.table->view,
                          le->values[column->parsed.table->table_index],
                          column->parsed.column, &l_val);
            if (r != ERROR_SUCCESS)
            {
                order->error = r;
                return 0;
            }

            r = column->parsed.table->view->ops->fetch_int(column->parsed.table->view,
                          re->values[column->parsed.table->table_index],
                          column->parsed.column, &r_val);
            if (r != ERROR_SUCCESS)
            {
                order->error = r;
                return 0;
            }

            if (l_val != r_val)
                return l_val < r_val ? -1 : 1;
        }
    }

    for (j = 0; j < wv->table_count; j++)
    {
        if (le->values[j] != re->values[j])
            return le->values[j] < re->values[j] ? -1 : 1;
    }
    return 0;
}

static void add_to_array( struct join_table **array, struct join_table *elem )
{
    while (*array && *array != elem)
        array++;
    if (!*array)
        *array = elem;
}

static BOOL in_array( struct join_table **array, struct join_table *elem )
{
    while (*array && *array != elem)
        array++;
    return *array != NULL;
}

#define CONST_EXPR 1 /* comparison to a constant value */
#define JOIN_TO_CONST_EXPR 0x10000 /* comparison to a table involved with
                                      a CONST_EXPR comaprison */

static UINT reorder_check( const struct expr *expr, struct join_table **ordered_tables,
                           BOOL process_joins, struct join_table **lastused )
{
    UINT res = 0;

    switch (expr->type)
    {
        case EXPR_WILDCARD:
        case EXPR_SVAL:
        case EXPR_UVAL:
            return 0;
        case EXPR_COL_NUMBER:
        case EXPR_COL_NUMBER32:
        case EXPR_COL_NUMBER_STRING:
            if (in_array(ordered_tables, expr->u.column.parsed.table))
                return JOIN_TO_CONST_EXPR;
            *lastused = expr->u.column.parsed.table;
            return CONST_EXPR;
        case EXPR_STRCMP:
        case EXPR_COMPLEX:
            res = reorder_check(expr->u.expr.right, ordered_tables, process_joins, lastused);
            /* fall through */
        case EXPR_UNARY:
            res += reorder_check(expr->u.expr.left, ordered_tables, process_joins, lastused);
            if (res == 0)
                return 0;
            if (res == CONST_EXPR)
                add_to_array(ordered_tables, *lastused);
            if (process_joins && res == JOIN_TO_CONST_EXPR + CONST_EXPR)
                add_to_array(ordered_tables, *lastused);
            return res;
        default:
            ERR("Unknown expr type: %i\n", expr->type);
            assert(0);
            return 0x1000000;
    }
}

/* reorders the tablelist in a way to evaluate the condition as fast as possible */
static struct join_table **ordertables( MSIWHEREVIEW *wv )
{
    struct join_table *table, **tables;

    tables = calloc(wv->table_count + 1, sizeof(*tables));

    if (wv->cond)
    {
        table = NULL;
        reorder_check(wv->cond, tables, FALSE, &table);
        table = NULL;
        reorder_check(wv->cond, tables, TRUE, &table);
    }

    table = wv->tables;
    while (table)
    {
        add_to_array(tables, table);
        table = table->next;
    }
    return tables;
}

static UINT WHERE_execute( struct tagMSIVIEW *view, MSIRECORD *record )
{
    MSIWHEREVIEW *wv = (MSIWHEREVIEW*)view;
    UINT r;
    struct join_table *table = wv->tables;
    UINT *rows;
    struct join_table **ordered_tables;
    UINT i = 0;

    TRACE("%p %p\n", wv, record);

    if( !table )
         return ERROR_FUNCTION_FAILED;

    r = init_reorder(wv);
    if (r != ERROR_SUCCESS)
        return r;

    do
    {
        table->view->ops->execute(table->view, NULL);

        r = table->view->ops->get_dimensions(table->view, &table->row_count, NULL);
        if (r != ERROR_SUCCESS)
        {
            ERR("failed to get table dimensions\n");
            return r;
        }

        /* each table must have at least one row */
        if (table->row_count == 0)
            return ERROR_SUCCESS;
    }
    while ((table = table->next));

    ordered_tables = ordertables( wv );

    rows = malloc(wv->table_count * sizeof(*rows));
    for (i = 0; i < wv->table_count; i++)
        rows[i] = INVALID_ROW_INDEX;

    r =  check_condition(wv, record, ordered_tables, rows);

    if (wv->order_info)
        wv->order_info->error = ERROR_SUCCESS;

    qsort(wv->reorder, wv->row_count, sizeof(struct row_entry *), compare_entry);

    if (wv->order_info)
        r = wv->order_info->error;

    free(rows);
    free(ordered_tables);
    return r;
}

static UINT WHERE_close( struct tagMSIVIEW *view )
{
    MSIWHEREVIEW *wv = (MSIWHEREVIEW*)view;
    struct join_table *table = wv->tables;

    TRACE("%p\n", wv );

    if (!table)
        return ERROR_FUNCTION_FAILED;

    do
        table->view->ops->close(table->view);
    while ((table = table->next));

    return ERROR_SUCCESS;
}

static UINT WHERE_get_dimensions( struct tagMSIVIEW *view, UINT *rows, UINT *cols )
{
    MSIWHEREVIEW *wv = (MSIWHEREVIEW*)view;

    TRACE("%p %p %p\n", wv, rows, cols );

    if(!wv->tables)
         return ERROR_FUNCTION_FAILED;

    if (rows)
    {
        if (!wv->reorder)
            return ERROR_FUNCTION_FAILED;
        *rows = wv->row_count;
    }

    if (cols)
        *cols = wv->col_count;

    return ERROR_SUCCESS;
}

static UINT WHERE_get_column_info( struct tagMSIVIEW *view, UINT n, LPCWSTR *name,
                                   UINT *type, BOOL *temporary, LPCWSTR *table_name )
{
    MSIWHEREVIEW *wv = (MSIWHEREVIEW*)view;
    struct join_table *table;

    TRACE("%p %d %p %p %p %p\n", wv, n, name, type, temporary, table_name );

    if(!wv->tables)
         return ERROR_FUNCTION_FAILED;

    table = find_table(wv, n, &n);
    if (!table)
        return ERROR_FUNCTION_FAILED;

    return table->view->ops->get_column_info(table->view, n, name,
                                            type, temporary, table_name);
}

static UINT join_find_row( MSIWHEREVIEW *wv, MSIRECORD *rec, UINT *row )
{
    LPCWSTR str;
    UINT r, i, id, data;

    str = MSI_RecordGetString( rec, 1 );
    r = msi_string2id( wv->db->strings, str, -1, &id );
    if (r != ERROR_SUCCESS)
        return r;

    for (i = 0; i < wv->row_count; i++)
    {
        WHERE_fetch_int( &wv->view, i, 1, &data );

        if (data == id)
        {
            *row = i;
            return ERROR_SUCCESS;
        }
    }

    return ERROR_FUNCTION_FAILED;
}

static UINT join_modify_update( struct tagMSIVIEW *view, MSIRECORD *rec )
{
    MSIWHEREVIEW *wv = (MSIWHEREVIEW *)view;
    UINT r, row, i, mask = 0;
    MSIRECORD *current;


    r = join_find_row( wv, rec, &row );
    if (r != ERROR_SUCCESS)
        return r;

    r = msi_view_get_row( wv->db, view, row, &current );
    if (r != ERROR_SUCCESS)
        return r;

    assert(MSI_RecordGetFieldCount(rec) == MSI_RecordGetFieldCount(current));

    for (i = MSI_RecordGetFieldCount(rec); i > 0; i--)
    {
        if (!MSI_RecordsAreFieldsEqual(rec, current, i))
            mask |= 1 << (i - 1);
    }
     msiobj_release(&current->hdr);

    return WHERE_set_row( view, row, rec, mask );
}

static UINT WHERE_modify( struct tagMSIVIEW *view, MSIMODIFY eModifyMode,
                          MSIRECORD *rec, UINT row )
{
    MSIWHEREVIEW *wv = (MSIWHEREVIEW*)view;
    struct join_table *table = wv->tables;
    UINT r;

    TRACE("%p %d %p\n", wv, eModifyMode, rec);

    if (!table)
        return ERROR_FUNCTION_FAILED;

    if (!table->next)
    {
        UINT *rows;

        if (find_row(wv, row, &rows) == ERROR_SUCCESS)
            row = rows[0];
        else
            row = -1;

        return table->view->ops->modify(table->view, eModifyMode, rec, row);
    }

    switch (eModifyMode)
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
        WARN("%p %d %p %u - unknown mode\n", view, eModifyMode, rec, row );
        r = ERROR_INVALID_PARAMETER;
        break;
    }

    return r;
}

static UINT WHERE_delete( struct tagMSIVIEW *view )
{
    MSIWHEREVIEW *wv = (MSIWHEREVIEW*)view;
    struct join_table *table = wv->tables;

    TRACE("%p\n", wv );

    while(table)
    {
        struct join_table *next;

        table->view->ops->delete(table->view);
        table->view = NULL;
        next = table->next;
        free(table);
        table = next;
    }
    wv->tables = NULL;
    wv->table_count = 0;

    free_reorder(wv);

    free(wv->order_info);
    wv->order_info = NULL;

    msiobj_release( &wv->db->hdr );
    free(wv);

    return ERROR_SUCCESS;
}

static UINT WHERE_sort(struct tagMSIVIEW *view, column_info *columns)
{
    MSIWHEREVIEW *wv = (MSIWHEREVIEW *)view;
    struct join_table *table = wv->tables;
    column_info *column = columns;
    MSIORDERINFO *orderinfo;
    UINT r, count = 0;
    UINT i;

    TRACE("%p %p\n", view, columns);

    if (!table)
        return ERROR_FUNCTION_FAILED;

    while (column)
    {
        count++;
        column = column->next;
    }

    if (count == 0)
        return ERROR_SUCCESS;

    orderinfo = malloc(offsetof(MSIORDERINFO, columns[count]));
    if (!orderinfo)
        return ERROR_OUTOFMEMORY;

    orderinfo->col_count = count;

    column = columns;

    for (i = 0; i < count; i++)
    {
        orderinfo->columns[i].unparsed.column = column->column;
        orderinfo->columns[i].unparsed.table = column->table;

        r = parse_column(wv, &orderinfo->columns[i], NULL);
        if (r != ERROR_SUCCESS)
            goto error;
    }

    wv->order_info = orderinfo;

    return ERROR_SUCCESS;
error:
    free(orderinfo);
    return r;
}

static const MSIVIEWOPS where_ops =
{
    WHERE_fetch_int,
    WHERE_fetch_stream,
    WHERE_set_int,
    WHERE_set_string,
    WHERE_set_stream,
    WHERE_set_row,
    NULL,
    WHERE_delete_row,
    WHERE_execute,
    WHERE_close,
    WHERE_get_dimensions,
    WHERE_get_column_info,
    WHERE_modify,
    WHERE_delete,
    NULL,
    NULL,
    NULL,
    WHERE_sort,
    NULL,
};

static UINT WHERE_VerifyCondition( MSIWHEREVIEW *wv, struct expr *cond,
                                   UINT *valid )
{
    UINT r;

    switch( cond->type )
    {
    case EXPR_COLUMN:
    {
        UINT type;

        *valid = FALSE;

        r = parse_column(wv, &cond->u.column, &type);
        if (r != ERROR_SUCCESS)
            break;

        if (type&MSITYPE_STRING)
            cond->type = EXPR_COL_NUMBER_STRING;
        else if ((type&0xff) == 4)
            cond->type = EXPR_COL_NUMBER32;
        else
            cond->type = EXPR_COL_NUMBER;

        *valid = TRUE;
        break;
    }
    case EXPR_COMPLEX:
        r = WHERE_VerifyCondition( wv, cond->u.expr.left, valid );
        if( r != ERROR_SUCCESS )
            return r;
        if( !*valid )
            return ERROR_SUCCESS;
        r = WHERE_VerifyCondition( wv, cond->u.expr.right, valid );
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
        r = WHERE_VerifyCondition( wv, cond->u.expr.left, valid );
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

UINT WHERE_CreateView( MSIDATABASE *db, MSIVIEW **view, LPWSTR tables,
                       struct expr *cond )
{
    MSIWHEREVIEW *wv = NULL;
    UINT r, valid = 0;
    WCHAR *ptr;

    TRACE("(%s)\n", debugstr_w(tables) );

    wv = calloc(1, sizeof *wv);
    if( !wv )
        return ERROR_FUNCTION_FAILED;

    /* fill the structure */
    wv->view.ops = &where_ops;
    msiobj_addref( &db->hdr );
    wv->db = db;
    wv->cond = cond;

    while (*tables)
    {
        struct join_table *table;

        if ((ptr = wcschr(tables, ' ')))
            *ptr = '\0';

        table = malloc(sizeof(*table));
        if (!table)
        {
            r = ERROR_OUTOFMEMORY;
            goto end;
        }

        r = TABLE_CreateView(db, tables, &table->view);
        if (r != ERROR_SUCCESS)
        {
            WARN("can't create table: %s\n", debugstr_w(tables));
            free(table);
            r = ERROR_BAD_QUERY_SYNTAX;
            goto end;
        }

        r = table->view->ops->get_dimensions(table->view, NULL,
                                             &table->col_count);
        if (r != ERROR_SUCCESS)
        {
            ERR("can't get table dimensions\n");
            table->view->ops->delete(table->view);
            free(table);
            goto end;
        }

        wv->col_count += table->col_count;
        table->table_index = wv->table_count++;

        table->next = wv->tables;
        wv->tables = table;

        if (!ptr)
            break;

        tables = ptr + 1;
    }

    if( cond )
    {
        r = WHERE_VerifyCondition( wv, cond, &valid );
        if( r != ERROR_SUCCESS )
            goto end;
        if( !valid ) {
            r = ERROR_FUNCTION_FAILED;
            goto end;
        }
    }

    *view = (MSIVIEW*) wv;

    return ERROR_SUCCESS;
end:
    WHERE_delete(&wv->view);

    return r;
}
