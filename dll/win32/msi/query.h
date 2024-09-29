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

#ifndef __WINE_MSI_QUERY_H
#define __WINE_MSI_QUERY_H

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "objidl.h"
#include "msi.h"
#include "msiquery.h"
#include "msipriv.h"
#include "wine/list.h"


#define OP_EQ       1
#define OP_AND      2
#define OP_OR       3
#define OP_GT       4
#define OP_LT       5
#define OP_LE       6
#define OP_GE       7
#define OP_NE       8
#define OP_ISNULL   9
#define OP_NOTNULL  10

#define EXPR_COMPLEX  1
#define EXPR_COLUMN   2
#define EXPR_COL_NUMBER 3
#define EXPR_IVAL     4
#define EXPR_SVAL     5
#define EXPR_UVAL     6
#define EXPR_STRCMP   7
#define EXPR_WILDCARD 9
#define EXPR_COL_NUMBER_STRING 10
#define EXPR_COL_NUMBER32 11
#define EXPR_UNARY    12

struct sql_str {
    LPCWSTR data;
    INT len;
};

struct complex_expr
{
    UINT op;
    struct expr *left;
    struct expr *right;
};

union ext_column
{
    struct
    {
        LPCWSTR column;
        LPCWSTR table;
    } unparsed;
    struct
    {
        UINT column;
        struct join_table *table;
    } parsed;
};

struct expr
{
    int type;
    union
    {
        struct complex_expr expr;
        INT   ival;
        UINT  uval;
        LPCWSTR sval;
        union ext_column column;
    } u;
};

typedef struct
{
    MSIDATABASE *db;
    LPCWSTR command;
    DWORD n, len;
    UINT r;
    MSIVIEW **view;  /* View structure for the resulting query.  This value
                      * tracks the view currently being created so we can free
                      * this view on syntax error.
                      */
    struct list *mem;
} SQL_input;

UINT MSI_ParseSQL( MSIDATABASE *db, LPCWSTR command, MSIVIEW **phview, struct list *mem );

UINT TABLE_CreateView( MSIDATABASE *db, LPCWSTR name, MSIVIEW **view );

UINT SELECT_CreateView( MSIDATABASE *db, MSIVIEW **view, MSIVIEW *table, const column_info *columns );

UINT DISTINCT_CreateView( MSIDATABASE *db, MSIVIEW **view, MSIVIEW *table );

UINT ORDER_CreateView( MSIDATABASE *db, MSIVIEW **view, MSIVIEW *table, column_info *columns );

UINT WHERE_CreateView( MSIDATABASE *db, MSIVIEW **view, LPWSTR tables, struct expr *cond );

UINT CREATE_CreateView( MSIDATABASE *db, MSIVIEW **view, LPCWSTR table,
                        column_info *col_info, BOOL hold );

UINT INSERT_CreateView( MSIDATABASE *db, MSIVIEW **view, LPCWSTR table,
                        column_info *columns, column_info *values, BOOL temp );

UINT UPDATE_CreateView( MSIDATABASE *db, MSIVIEW **view, LPWSTR table,
                        column_info *list, struct expr *expr );

UINT DELETE_CreateView( MSIDATABASE *db, MSIVIEW **view, MSIVIEW *table );

UINT ALTER_CreateView( MSIDATABASE *db, MSIVIEW **view, LPCWSTR name, column_info *colinfo, int hold );

UINT STREAMS_CreateView( MSIDATABASE *db, MSIVIEW **view );

UINT STORAGES_CreateView( MSIDATABASE *db, MSIVIEW **view );

UINT DROP_CreateView( MSIDATABASE *db, MSIVIEW **view, LPCWSTR name );

int sqliteGetToken(const WCHAR *z, int *tokenType, int *skip);

MSIRECORD *msi_query_merge_record( UINT fields, const column_info *vl, MSIRECORD *rec );

UINT msi_create_table( MSIDATABASE *db, LPCWSTR name, column_info *col_info,
                       MSICONDITION persistent, BOOL hold );

UINT msi_select_update( MSIVIEW *view, MSIRECORD *rec, UINT row );

UINT msi_view_refresh_row( MSIDATABASE *db, MSIVIEW *view, UINT row, MSIRECORD *rec );

#endif /* __WINE_MSI_QUERY_H */
