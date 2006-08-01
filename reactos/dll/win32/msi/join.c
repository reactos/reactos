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
    UINT           left_key, right_key;
    UINT          *pairs;
    UINT           pair_count;
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

    if( row >= jv->pair_count )
         return ERROR_FUNCTION_FAILED;

    if( col <= jv->left_count )
    {
        table = jv->left;
        row = jv->pairs[ row*2 ];
    }
    else
    {
        table = jv->right;
        row = jv->pairs[ row*2 + 1 ];
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

    if( row <= jv->left_count )
    {
        table = jv->left;
        row = jv->pairs[ row*2 ];
    }
    else
    {
        table = jv->right;
        row = jv->pairs[ row*2 + 1 ];
        col -= jv->left_count;
    }

    return table->ops->fetch_stream( table, row, col, stm );
}

static UINT JOIN_set_int( struct tagMSIVIEW *view, UINT row, UINT col, UINT val )
{
    MSIJOINVIEW *jv = (MSIJOINVIEW*)view;

    TRACE("%p %d %d %04x\n", jv, row, col, val );

    return ERROR_FUNCTION_FAILED;
}

static UINT JOIN_insert_row( struct tagMSIVIEW *view, MSIRECORD *record )
{
    MSIJOINVIEW *jv = (MSIJOINVIEW*)view;

    TRACE("%p %p\n", jv, record );

    return ERROR_FUNCTION_FAILED;
}

static int join_key_compare(const void *l, const void *r)
{
    const UINT *left = l, *right = r;
    if (left[1] < right[1])
        return -1;
    if (left[1] == right[1])
        return 0;
    return 1;
}

static UINT join_load_key_column( MSIJOINVIEW *jv, MSIVIEW *table, UINT column,
                                  UINT **pdata, UINT *pcount )
{
    UINT r, i, count = 0, *data = NULL;

    r = table->ops->get_dimensions( table, &count, NULL );
    if( r != ERROR_SUCCESS )
        return r;

    if (!count)
        goto end;

    data = msi_alloc( count * 2 * sizeof (UINT) );
    if (!data)
        return ERROR_SUCCESS;

    for (i=0; i<count; i++)
    {
        data[i*2] = i;
        r = table->ops->fetch_int( table, i, column, &data[i*2+1] );
        if (r != ERROR_SUCCESS)
            ERR("fetch data (%u,%u) failed\n", i, column);
    }

    qsort( data, count, 2 * sizeof (UINT), join_key_compare );

end:
    *pdata = data;
    *pcount = count;

    return ERROR_SUCCESS;
}

static UINT join_match( UINT *ldata, UINT lcount,
                        UINT *rdata, UINT rcount,
                        UINT **ppairs, UINT *ppair_count )
{
    UINT *pairs;
    UINT n, i, j;

    TRACE("left %u right %u\n", rcount, lcount);

    /* there can be at most max(lcount, rcount) matches */
    if (lcount > rcount)
        n = lcount;
    else
        n = rcount;

    pairs = msi_alloc( n * 2 * sizeof(UINT) );
    if (!pairs)
        return ERROR_OUTOFMEMORY;

    for (n=0, i=0, j=0; i<lcount && j<rcount; )
    {
        /* values match... store the row numbers */
        if (ldata[i*2+1] == rdata[j*2+1])
        {
            pairs[n*2] = ldata[i*2];
            pairs[n*2+1] = rdata[j*2];
            i++;  /* FIXME: assumes primary key on the right */
            n++;
            continue;
        }

        /* values differ... move along */
        if (ldata[i*2+1] < rdata[j*2+1])
            i++;
        else
            j++;
    }

    *ppairs = pairs;
    *ppair_count = n;

    return ERROR_SUCCESS;
}

static UINT JOIN_execute( struct tagMSIVIEW *view, MSIRECORD *record )
{
    MSIJOINVIEW *jv = (MSIJOINVIEW*)view;
    UINT r, *ldata = NULL, *rdata = NULL, lcount = 0, rcount = 0;

    TRACE("%p %p\n", jv, record);

    if( !jv->left || !jv->right )
         return ERROR_FUNCTION_FAILED;

    r = jv->left->ops->execute( jv->left, NULL );
    if (r != ERROR_SUCCESS)
        return r;

    r = jv->right->ops->execute( jv->right, NULL );
    if (r != ERROR_SUCCESS)
        return r;

    r = join_load_key_column( jv, jv->left, jv->left_key, &ldata, &lcount );
    if (r != ERROR_SUCCESS)
        return r;

    r = join_load_key_column( jv, jv->right, jv->right_key, &rdata, &rcount );
    if (r != ERROR_SUCCESS)
        goto end;

    r = join_match( ldata, lcount, rdata, rcount, &jv->pairs, &jv->pair_count );

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

        *rows = jv->pair_count;
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

    msi_free( jv->pairs );
    jv->pairs = NULL;

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
    JOIN_set_int,
    JOIN_insert_row,
    JOIN_execute,
    JOIN_close,
    JOIN_get_dimensions,
    JOIN_get_column_info,
    JOIN_modify,
    JOIN_delete,
    JOIN_find_matching_rows
};

/*
 * join_check_condition
 *
 * This is probably overly strict about what kind of condition we need
 *  for a join query.
 */
static UINT join_check_condition(MSIJOINVIEW *jv, struct expr *cond)
{
    UINT r, type = 0;

    /* assume that we have  `KeyColumn` = `SubkeyColumn` */
    if ( cond->type != EXPR_COMPLEX )
        return ERROR_FUNCTION_FAILED;

    if ( cond->u.expr.op != OP_EQ )
        return ERROR_FUNCTION_FAILED;

    if ( cond->u.expr.left->type != EXPR_COLUMN )
        return ERROR_FUNCTION_FAILED;

    if ( cond->u.expr.right->type != EXPR_COLUMN )
        return ERROR_FUNCTION_FAILED;

    /* make sure both columns exist */
    r = VIEW_find_column( jv->left, cond->u.expr.left->u.column, &jv->left_key );
    if (r != ERROR_SUCCESS)
        return ERROR_FUNCTION_FAILED;

    r = VIEW_find_column( jv->right, cond->u.expr.right->u.column, &jv->right_key );
    if (r != ERROR_SUCCESS)
        return ERROR_FUNCTION_FAILED;

    /* make sure both columns are keys */
    r = jv->left->ops->get_column_info( jv->left, jv->left_key, NULL, &type );
    if (r != ERROR_SUCCESS)
        return ERROR_FUNCTION_FAILED;

    if (!(type & MSITYPE_KEY))
        return ERROR_FUNCTION_FAILED;

    r = jv->right->ops->get_column_info( jv->right, jv->right_key, NULL, &type );
    if (r != ERROR_SUCCESS)
        return ERROR_FUNCTION_FAILED;

    if (!(type & MSITYPE_KEY))
        return ERROR_FUNCTION_FAILED;

    TRACE("left %s (%u) right %s (%u)\n",
        debugstr_w(cond->u.expr.left->u.column), jv->left_key,
        debugstr_w(cond->u.expr.right->u.column), jv->right_key);

    return ERROR_SUCCESS;
}

UINT JOIN_CreateView( MSIDATABASE *db, MSIVIEW **view,
                      LPCWSTR left, LPCWSTR right,
                      struct expr *cond )
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

    r = join_check_condition( jv, cond );
    if( r != ERROR_SUCCESS )
    {
        ERR("can't get join condition\n");
        goto end;
    }

    *view = &jv->view;
    return ERROR_SUCCESS;

end:
    jv->view.ops->delete( &jv->view );

    return r;
}
