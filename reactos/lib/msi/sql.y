%{

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


#include "config.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "query.h"
#include "wine/debug.h"
#include "wine/unicode.h"

#define YYLEX_PARAM info
#define YYPARSE_PARAM info

extern int SQL_error(const char *str);

WINE_DEFAULT_DEBUG_CHANNEL(msi);

typedef struct tag_SQL_input
{
    MSIDATABASE *db;
    LPCWSTR command;
    DWORD n, len;
    MSIVIEW **view;  /* view structure for the resulting query */
} SQL_input;

static LPWSTR SQL_getstring( struct sql_str *str );
static INT SQL_getint( SQL_input *sql );
static int SQL_lex( void *SQL_lval, SQL_input *info);

static MSIVIEW *do_one_select( MSIDATABASE *db, MSIVIEW *in, 
                               string_list *columns );
static MSIVIEW *do_order_by( MSIDATABASE *db, MSIVIEW *in, 
                             string_list *columns );

static BOOL SQL_MarkPrimaryKeys( create_col_info *cols,
                                 string_list *keys);

static struct expr * EXPR_complex( struct expr *l, UINT op, struct expr *r );
static struct expr * EXPR_column( LPWSTR );
static struct expr * EXPR_ival( struct sql_str *, int sign);
static struct expr * EXPR_sval( struct sql_str *);
static struct expr * EXPR_wildcard();

%}

%pure-parser

%union
{
    struct sql_str str;
    LPWSTR string;
    string_list *column_list;
    value_list *val_list;
    MSIVIEW *query;
    struct expr *expr;
    USHORT column_type;
    create_col_info *column_info;
    column_assignment update_col_info;
}

%token TK_ABORT TK_AFTER TK_AGG_FUNCTION TK_ALL TK_AND TK_AS TK_ASC
%token TK_BEFORE TK_BEGIN TK_BETWEEN TK_BITAND TK_BITNOT TK_BITOR TK_BY
%token TK_CASCADE TK_CASE TK_CHAR TK_CHECK TK_CLUSTER TK_COLLATE TK_COLUMN
%token TK_COMMA TK_COMMENT TK_COMMIT TK_CONCAT TK_CONFLICT 
%token TK_CONSTRAINT TK_COPY TK_CREATE
%token TK_DEFAULT TK_DEFERRABLE TK_DEFERRED TK_DELETE TK_DELIMITERS TK_DESC
%token TK_DISTINCT TK_DOT TK_DROP TK_EACH
%token TK_ELSE TK_END TK_END_OF_FILE TK_EQ TK_EXCEPT TK_EXPLAIN
%token TK_FAIL TK_FLOAT TK_FOR TK_FOREIGN TK_FROM TK_FUNCTION
%token TK_GE TK_GLOB TK_GROUP TK_GT
%token TK_HAVING TK_HOLD
%token TK_IGNORE TK_ILLEGAL TK_IMMEDIATE TK_IN TK_INDEX TK_INITIALLY
%token <str> TK_ID 
%token TK_INSERT TK_INSTEAD TK_INT 
%token <str> TK_INTEGER
%token TK_INTERSECT TK_INTO TK_IS
%token TK_ISNULL
%token TK_JOIN TK_JOIN_KW
%token TK_KEY
%token TK_LE TK_LIKE TK_LIMIT TK_LONG TK_LONGCHAR TK_LP TK_LSHIFT TK_LT
%token TK_LOCALIZABLE
%token TK_MATCH TK_MINUS
%token TK_NE TK_NOT TK_NOTNULL TK_NULL
%token TK_OBJECT TK_OF TK_OFFSET TK_ON TK_OR TK_ORACLE_OUTER_JOIN TK_ORDER
%token TK_PLUS TK_PRAGMA TK_PRIMARY
%token TK_RAISE TK_REFERENCES TK_REM TK_REPLACE TK_RESTRICT TK_ROLLBACK
%token TK_ROW TK_RP TK_RSHIFT
%token TK_SELECT TK_SEMI TK_SET TK_SHORT TK_SLASH TK_SPACE TK_STAR TK_STATEMENT 
%token <str> TK_STRING
%token TK_TABLE TK_TEMP TK_THEN TK_TRANSACTION TK_TRIGGER
%token TK_UMINUS TK_UNCLOSED_STRING TK_UNION TK_UNIQUE
%token TK_UPDATE TK_UPLUS TK_USING
%token TK_VACUUM TK_VALUES TK_VIEW
%token TK_WHEN TK_WHERE TK_WILDCARD

/*
 * These are extra tokens used by the lexer but never seen by the
 * parser.  We put them in a rule so that the parser generator will
 * add them to the parse.h output file.
 *
 */
%nonassoc END_OF_FILE ILLEGAL SPACE UNCLOSED_STRING COMMENT FUNCTION
          COLUMN AGG_FUNCTION.

%type <string> column table string_or_id
%type <column_list> selcollist
%type <query> from unorderedsel oneselect onequery onecreate oneinsert oneupdate
%type <expr> expr val column_val const_val
%type <column_type> column_type data_type data_type_l data_count
%type <column_info> column_def table_def
%type <val_list> constlist
%type <update_col_info> column_assignment update_assign_list

%%

onequery:
    oneselect
    {
        SQL_input* sql = (SQL_input*) info;
        *sql->view = $1;
    }
  | onecreate
    {
        SQL_input* sql = (SQL_input*) info;
        *sql->view = $1;
    }
  | oneinsert
    {
        SQL_input* sql = (SQL_input*) info;
        *sql->view = $1;
    }
  | oneupdate
    {
        SQL_input* sql = (SQL_input*) info;
        *sql->view = $1;
    }
    ;

oneinsert:
    TK_INSERT TK_INTO table TK_LP selcollist TK_RP TK_VALUES TK_LP constlist TK_RP
    {
        SQL_input *sql = (SQL_input*) info;
        MSIVIEW *insert = NULL; 

        INSERT_CreateView( sql->db, &insert, $3, $5, $9, FALSE ); 
        $$ = insert;
    }
  | TK_INSERT TK_INTO table TK_LP selcollist TK_RP TK_VALUES TK_LP constlist TK_RP TK_TEMP
    {
        SQL_input *sql = (SQL_input*) info;
        MSIVIEW *insert = NULL; 

        INSERT_CreateView( sql->db, &insert, $3, $5, $9, TRUE ); 
        $$ = insert;
    }
    ;

onecreate:
    TK_CREATE TK_TABLE table TK_LP table_def TK_RP
        {
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW *create = NULL; 

            if( !$5 )
                YYABORT;
            CREATE_CreateView( sql->db, &create, $3, $5, FALSE );
            $$ = create;
        }
  | TK_CREATE TK_TABLE table TK_LP table_def TK_RP TK_HOLD
        {
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW *create = NULL; 

            if( !$5 )
                YYABORT;
            CREATE_CreateView( sql->db, &create, $3, $5, TRUE );
            $$ = create;
        }
    ;

oneupdate:
    TK_UPDATE table TK_SET update_assign_list TK_WHERE expr
        {
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW *update = NULL; 

            UPDATE_CreateView( sql->db, &update, $2, &$4, $6 );
            $$ = update;
        }
    ;

table_def:
    column_def TK_PRIMARY TK_KEY selcollist
        {
            if( SQL_MarkPrimaryKeys( $1, $4 ) )
                $$ = $1;
            else
                $$ = NULL;
        }
    ;

column_def:
    column_def TK_COMMA column column_type
        {
            create_col_info *ci;

            for( ci = $1; ci->next; ci = ci->next )
                ;

            ci->next = HeapAlloc( GetProcessHeap(), 0, sizeof *$$ );
            if( !ci->next )
            {
                /* FIXME: free $1 */
                YYABORT;
            }
            ci->next->colname = $3;
            ci->next->type = $4;
            ci->next->next = NULL;

            $$ = $1;
        }
  | column column_type
        {
            $$ = HeapAlloc( GetProcessHeap(), 0, sizeof *$$ );
            if( ! $$ )
                YYABORT;
            $$->colname = $1;
            $$->type = $2;
            $$->next = NULL;
        }
    ;

column_type:
    data_type_l
        {
            $$ = $1 | MSITYPE_VALID;
        }
  | data_type_l TK_LOCALIZABLE
        {
            FIXME("LOCALIZABLE ignored\n");
            $$ = $1 | MSITYPE_VALID;
        }
    ;

data_type_l:
    data_type
        {
            $$ |= MSITYPE_NULLABLE;
        }
  | data_type TK_NOT TK_NULL
        {
            $$ = $1;
        }
    ;

data_type:
    TK_CHAR
        {
            $$ = MSITYPE_STRING | 1;
        }
  | TK_CHAR TK_LP data_count TK_RP
        {
            $$ = MSITYPE_STRING | 0x400 | $3;
        }
  | TK_LONGCHAR
        {
            $$ = 2;
        }
  | TK_SHORT
        {
            $$ = 2;
        }
  | TK_INT
        {
            $$ = 2;
        }
  | TK_LONG
        {
            $$ = 4;
        }
  | TK_OBJECT
        {
            $$ = 0;
        }
    ;

data_count:
    TK_INTEGER
        {
            SQL_input* sql = (SQL_input*) info;
            int val = SQL_getint(sql);
            if( ( val > 255 ) || ( val < 0 ) )
                YYABORT;
            $$ = val;
        }
    ;

oneselect:
    unorderedsel TK_ORDER TK_BY selcollist
        {
            SQL_input* sql = (SQL_input*) info;

            if( !$1 )
                YYABORT;
            if( $4 )
                $$ = do_order_by( sql->db, $1, $4 );
            else
                $$ = $1;
        }
  | unorderedsel
    ;

unorderedsel:
    TK_SELECT selcollist from 
        {
            SQL_input* sql = (SQL_input*) info;
            if( !$3 )
                YYABORT;
            if( $2 )
            {
                $$ = do_one_select( sql->db, $3, $2 );
                if( !$$ )
                    YYABORT;
            }
            else
                $$ = $3;
        }
  | TK_SELECT TK_DISTINCT selcollist from 
        {
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW *view = $4;

            if( !view )
                YYABORT;
            if( $3 )
            {
                view = do_one_select( sql->db, view, $3 );
                if( !view )
                    YYABORT;
            }
            DISTINCT_CreateView( sql->db, & $$, view );
        }
    ;

selcollist:
    column 
        { 
            string_list *list;

            list = HeapAlloc( GetProcessHeap(), 0, sizeof *list );
            if( !list )
                YYABORT;
            list->string = $1;
            list->next = NULL;

            $$ = list;
            TRACE("Collist %s\n",debugstr_w($$->string));
        }
  | column TK_COMMA selcollist
        { 
            string_list *list;

            list = HeapAlloc( GetProcessHeap(), 0, sizeof *list );
            if( !list )
                YYABORT;
            list->string = $1;
            list->next = $3;

            $$ = list;
            TRACE("From table: %s\n",debugstr_w($$->string));
        }
  | TK_STAR
        {
            $$ = NULL;
        }
    ;

from:
    TK_FROM table
        { 
            SQL_input* sql = (SQL_input*) info;
            UINT r;

            $$ = NULL;
            TRACE("From table: %s\n",debugstr_w($2));
            r = TABLE_CreateView( sql->db, $2, & $$ );
            if( r != ERROR_SUCCESS )
                YYABORT;
        }
  | TK_FROM table TK_WHERE expr
        { 
            SQL_input* sql = (SQL_input*) info;
            MSIVIEW *view = NULL;
            UINT r;

            $$ = NULL;
            TRACE("From table: %s\n",debugstr_w($2));
            r = TABLE_CreateView( sql->db, $2, &view );
            if( r != ERROR_SUCCESS )
                YYABORT;
            r = WHERE_CreateView( sql->db, &view, view, $4 );
            if( r != ERROR_SUCCESS )
                YYABORT;
            $$ = view;
        }
    ;

expr:
    TK_LP expr TK_RP
        {
            $$ = $2;
        }
  | column_val TK_EQ column_val
        {
            $$ = EXPR_complex( $1, OP_EQ, $3 );
        }
  | expr TK_AND expr
        {
            $$ = EXPR_complex( $1, OP_AND, $3 );
        }
  | expr TK_OR expr
        {
            $$ = EXPR_complex( $1, OP_OR, $3 );
        }
  | column_val TK_EQ val
        {
            $$ = EXPR_complex( $1, OP_EQ, $3 );
        }
  | column_val TK_GT val
        {
            $$ = EXPR_complex( $1, OP_GT, $3 );
        }
  | column_val TK_LT val
        {
            $$ = EXPR_complex( $1, OP_LT, $3 );
        }
  | column_val TK_LE val
        {
            $$ = EXPR_complex( $1, OP_LE, $3 );
        }
  | column_val TK_GE val
        {
            $$ = EXPR_complex( $1, OP_GE, $3 );
        }
  | column_val TK_NE val
        {
            $$ = EXPR_complex( $1, OP_NE, $3 );
        }
  | column_val TK_IS TK_NULL
        {
            $$ = EXPR_complex( $1, OP_ISNULL, NULL );
        }
  | column_val TK_IS TK_NOT TK_NULL
        {
            $$ = EXPR_complex( $1, OP_NOTNULL, NULL );
        }
    ;

val:
    column_val
  | const_val
    ;

constlist:
    const_val
        {
            value_list *vals;

            vals = HeapAlloc( GetProcessHeap(), 0, sizeof *vals );
            if( vals )
            {
                vals->val = $1;
                vals->next = NULL;
            }
            $$ = vals;
        }
  | constlist TK_COMMA const_val
        {
            value_list *vals;

            vals = HeapAlloc( GetProcessHeap(), 0, sizeof *vals );
            if( vals )
            {
                vals->val = $3;
                vals->next = NULL;
            }
            $1->next = vals;
            $$ = $1;
        }
    ;

update_assign_list:
    column_assignment
  | column_assignment TK_COMMA update_assign_list
        {
            $1.col_list->next = $3.col_list;
            $1.val_list->next = $3.val_list;
            $$ = $1;
        }
    ;

column_assignment:
    column TK_EQ const_val
        {
            $$.col_list = HeapAlloc( GetProcessHeap(), 0, sizeof *$$.col_list );
            if( !$$.col_list )
                YYABORT;
            $$.col_list->string = $1;
            $$.col_list->next = NULL;
            $$.val_list = HeapAlloc( GetProcessHeap(), 0, sizeof *$$.val_list );
            if( !$$.val_list )
                YYABORT;
            $$.val_list->val = $3;
            $$.val_list->next = 0;
        }
    ;

const_val:
    TK_INTEGER
        {
            $$ = EXPR_ival( &$1, 1 );
        }
  | TK_MINUS  TK_INTEGER
        {
            $$ = EXPR_ival( &$2, -1 );
        }
  | TK_STRING
        {
            $$ = EXPR_sval( &$1 );
        }
  | TK_WILDCARD
        {
            $$ = EXPR_wildcard();
        }
    ;

column_val:
    column 
        {
            $$ = EXPR_column( $1 );
        }
    ;

column:
    table TK_DOT string_or_id
        {
            $$ = $3;  /* FIXME */
        }
  | string_or_id
        {
            $$ = $1;
        }
    ;

table:
    string_or_id
        {
            $$ = $1;
        }
    ;

string_or_id:
    TK_ID
        {
            $$ = SQL_getstring( &$1 );
        }
  | TK_STRING
        {
            $$ = SQL_getstring( &$1 );
        }
    ;

%%

int SQL_lex( void *SQL_lval, SQL_input *sql)
{
    int token;
    struct sql_str * str = SQL_lval;

    do
    {
        sql->n += sql->len;
        if( ! sql->command[sql->n] )
            return 0;  /* end of input */

        TRACE("string : %s\n", debugstr_w(&sql->command[sql->n]));
        sql->len = sqliteGetToken( &sql->command[sql->n], &token );
        if( sql->len==0 )
            break;
        str->data = &sql->command[sql->n];
        str->len = sql->len;
    }
    while( token == TK_SPACE );

    TRACE("token : %d (%s)\n", token, debugstr_wn(&sql->command[sql->n], sql->len));
    
    return token;
}

LPWSTR SQL_getstring( struct sql_str *strdata)
{
    LPCWSTR p = strdata->data;
    UINT len = strdata->len;
    LPWSTR str;

    /* if there's quotes, remove them */
    if( ( (p[0]=='`') && (p[len-1]=='`') ) || 
        ( (p[0]=='\'') && (p[len-1]=='\'') ) )
    {
        p++;
        len -= 2;
    }
    str = HeapAlloc( GetProcessHeap(), 0, (len + 1)*sizeof(WCHAR));
    if(!str )
        return str;
    memcpy(str, p, len*sizeof(WCHAR) );
    str[len]=0;

    return str;
}

INT SQL_getint( SQL_input *sql )
{
    LPCWSTR p = &sql->command[sql->n];

    return atoiW( p );
}

int SQL_error(const char *str)
{
    return 0;
}

static MSIVIEW *do_one_select( MSIDATABASE *db, MSIVIEW *in, 
                               string_list *columns )
{
    MSIVIEW *view = NULL;

    SELECT_CreateView( db, &view, in, columns );
    delete_string_list( columns );
    if( !view )
        ERR("Error creating select query\n");
    return view;
}

static MSIVIEW *do_order_by( MSIDATABASE *db, MSIVIEW *in, 
                             string_list *columns )
{
    MSIVIEW *view = NULL;

    ORDER_CreateView( db, &view, in );
    if( view )
    {
        string_list *x = columns;

        for( x = columns; x ; x = x->next )
            ORDER_AddColumn( view, x->string );
    }
    else
        ERR("Error creating select query\n");
    delete_string_list( columns );
    return view;
}

static struct expr * EXPR_wildcard()
{
    struct expr *e = HeapAlloc( GetProcessHeap(), 0, sizeof *e );
    if( e )
    {
        e->type = EXPR_WILDCARD;
    }
    return e;
}

static struct expr * EXPR_complex( struct expr *l, UINT op, struct expr *r )
{
    struct expr *e = HeapAlloc( GetProcessHeap(), 0, sizeof *e );
    if( e )
    {
        e->type = EXPR_COMPLEX;
        e->u.expr.left = l;
        e->u.expr.op = op;
        e->u.expr.right = r;
    }
    return e;
}

static struct expr * EXPR_column( LPWSTR str )
{
    struct expr *e = HeapAlloc( GetProcessHeap(), 0, sizeof *e );
    if( e )
    {
        e->type = EXPR_COLUMN;
        e->u.sval = str;
    }
    return e;
}

static struct expr * EXPR_ival( struct sql_str *str , int sign)
{
    struct expr *e = HeapAlloc( GetProcessHeap(), 0, sizeof *e );
    if( e )
    {
        e->type = EXPR_IVAL;
        e->u.ival = atoiW( str->data ) * sign;
    }
    return e;
}

static struct expr * EXPR_sval( struct sql_str *str )
{
    struct expr *e = HeapAlloc( GetProcessHeap(), 0, sizeof *e );
    if( e )
    {
        e->type = EXPR_SVAL;
        e->u.sval = SQL_getstring( str );
    }
    return e;
}

void delete_expr( struct expr *e )
{
    if( !e )
        return;
    if( e->type == EXPR_COMPLEX )
    {
        delete_expr( e->u.expr.left );
        delete_expr( e->u.expr.right );
    }
    else if( e->type == EXPR_UTF8 )
        HeapFree( GetProcessHeap(), 0, e->u.utf8 );
    else if( e->type == EXPR_SVAL )
        HeapFree( GetProcessHeap(), 0, e->u.sval );
    HeapFree( GetProcessHeap(), 0, e );
}

void delete_string_list( string_list *sl )
{
    while( sl )
    {
        string_list *t = sl->next;
        HeapFree( GetProcessHeap(), 0, sl->string );
        HeapFree( GetProcessHeap(), 0, sl );
        sl = t;
    }
}

void delete_value_list( value_list *vl )
{
    while( vl )
    {
        value_list *t = vl->next;
        delete_expr( vl->val );
        HeapFree( GetProcessHeap(), 0, vl );
        vl = t;
    }
}

static BOOL SQL_MarkPrimaryKeys( create_col_info *cols,
                                 string_list *keys )
{
    string_list *k;
    BOOL found = TRUE;

    for( k = keys; k && found; k = k->next )
    {
        create_col_info *c;

        found = FALSE;
        for( c = cols; c && !found; c = c->next )
        {
             if( lstrcmpW( k->string, c->colname ) )
                 continue;
             c->type |= MSITYPE_KEY;
             found = TRUE;
        }
    }

    return found;
}

UINT MSI_ParseSQL( MSIDATABASE *db, LPCWSTR command, MSIVIEW **phview )
{
    SQL_input sql;
    int r;

    *phview = NULL;

    sql.db = db;
    sql.command = command;
    sql.n = 0;
    sql.len = 0;
    sql.view = phview;

    r = SQL_parse(&sql);

    TRACE("Parse returned %d\n", r);
    if( r )
    {
        if( *sql.view )
            (*sql.view)->ops->delete( *sql.view );
        *sql.view = NULL;
        return ERROR_BAD_QUERY_SYNTAX;
    }

    return ERROR_SUCCESS;
}
