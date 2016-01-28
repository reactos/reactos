%{

/*
 * Copyright 2012 Hans Leidekker for CodeWeavers
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

#include "config.h"
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wbemcli.h"
#include "wbemprox_private.h"

#include "wine/list.h"
#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(wbemprox);

struct parser
{
    const WCHAR *cmd;
    UINT idx;
    UINT len;
    HRESULT error;
    struct view **view;
    struct list *mem;
};

struct string
{
    const WCHAR *data;
    int len;
};

static void *alloc_mem( struct parser *parser, UINT size )
{
    struct list *mem = heap_alloc( sizeof(struct list) + size );
    list_add_tail( parser->mem, mem );
    return &mem[1];
}

static struct property *alloc_property( struct parser *parser, const WCHAR *class, const WCHAR *name )
{
    struct property *prop = alloc_mem( parser, sizeof(*prop) );
    if (prop)
    {
        prop->name  = name;
        prop->class = class;
        prop->next  = NULL;
    }
    return prop;
}

static WCHAR *get_string( struct parser *parser, const struct string *str )
{
    const WCHAR *p = str->data;
    int len = str->len;
    WCHAR *ret;

    if ((p[0] == '\"' && p[len - 1] != '\"') ||
        (p[0] == '\'' && p[len - 1] != '\'')) return NULL;
    if ((p[0] == '\"' && p[len - 1] == '\"') ||
        (p[0] == '\'' && p[len - 1] == '\''))
    {
        p++;
        len -= 2;
    }
    if (!(ret = alloc_mem( parser, (len + 1) * sizeof(WCHAR) ))) return NULL;
    memcpy( ret, p, len * sizeof(WCHAR) );
    ret[len] = 0;
    return ret;
}

static int get_int( struct parser *parser )
{
    const WCHAR *p = &parser->cmd[parser->idx];
    int i, ret = 0;

    for (i = 0; i < parser->len; i++)
    {
        if (p[i] < '0' || p[i] > '9')
        {
            ERR("should only be numbers here!\n");
            break;
        }
        ret = (p[i] - '0') + ret * 10;
    }
    return ret;
}

static struct expr *expr_complex( struct parser *parser, struct expr *l, UINT op, struct expr *r )
{
    struct expr *e = alloc_mem( parser, sizeof(*e) );
    if (e)
    {
        e->type = EXPR_COMPLEX;
        e->u.expr.left = l;
        e->u.expr.op = op;
        e->u.expr.right = r;
    }
    return e;
}

static struct expr *expr_unary( struct parser *parser, struct expr *l, UINT op )
{
    struct expr *e = alloc_mem( parser, sizeof(*e) );
    if (e)
    {
        e->type = EXPR_UNARY;
        e->u.expr.left = l;
        e->u.expr.op = op;
        e->u.expr.right = NULL;
    }
    return e;
}

static struct expr *expr_ival( struct parser *parser, int val )
{
    struct expr *e = alloc_mem( parser, sizeof *e );
    if (e)
    {
        e->type = EXPR_IVAL;
        e->u.ival = val;
    }
    return e;
}

static struct expr *expr_sval( struct parser *parser, const struct string *str )
{
    struct expr *e = alloc_mem( parser, sizeof *e );
    if (e)
    {
        e->type = EXPR_SVAL;
        e->u.sval = get_string( parser, str );
        if (!e->u.sval)
            return NULL; /* e will be freed by query destructor */
    }
    return e;
}

static struct expr *expr_bval( struct parser *parser, int val )
{
    struct expr *e = alloc_mem( parser, sizeof *e );
    if (e)
    {
        e->type = EXPR_BVAL;
        e->u.ival = val;
    }
    return e;
}

static struct expr *expr_propval( struct parser *parser, const struct property *prop )
{
    struct expr *e = alloc_mem( parser, sizeof *e );
    if (e)
    {
        e->type = EXPR_PROPVAL;
        e->u.propval = prop;
    }
    return e;
}

static int wql_error( struct parser *parser, const char *str );
static int wql_lex( void *val, struct parser *parser );

#define PARSER_BUBBLE_UP_VIEW( parser, result, current_view ) \
    *parser->view = current_view; \
    result = current_view

%}

%lex-param { struct parser *ctx }
%parse-param { struct parser *ctx }
%error-verbose
%pure-parser

%union
{
    struct string str;
    WCHAR *string;
    struct property *proplist;
    struct view *view;
    struct expr *expr;
    int integer;
}

%token TK_SELECT TK_FROM TK_STAR TK_COMMA TK_DOT TK_IS TK_LP TK_RP TK_NULL TK_FALSE TK_TRUE
%token TK_INTEGER TK_WHERE TK_SPACE TK_MINUS TK_ILLEGAL TK_BY
%token <str> TK_STRING TK_ID

%type <string> id
%type <proplist> prop proplist
%type <view> select
%type <expr> expr prop_val const_val string_val
%type <integer> number

%left TK_OR
%left TK_AND
%left TK_NOT
%left TK_EQ TK_NE TK_LT TK_GT TK_LE TK_GE TK_LIKE

%%

select:
    TK_SELECT TK_FROM id
        {
            HRESULT hr;
            struct parser *parser = ctx;
            struct view *view;

            hr = create_view( NULL, $3, NULL, &view );
            if (hr != S_OK)
                YYABORT;

            PARSER_BUBBLE_UP_VIEW( parser, $$, view );
        }
  | TK_SELECT proplist TK_FROM id
        {
            HRESULT hr;
            struct parser *parser = ctx;
            struct view *view;

            hr = create_view( $2, $4, NULL, &view );
            if (hr != S_OK)
                YYABORT;

            PARSER_BUBBLE_UP_VIEW( parser, $$, view );
        }
  | TK_SELECT proplist TK_FROM id TK_WHERE expr
        {
            HRESULT hr;
            struct parser *parser = ctx;
            struct view *view;

            hr = create_view( $2, $4, $6, &view );
            if (hr != S_OK)
                YYABORT;

            PARSER_BUBBLE_UP_VIEW( parser, $$, view );
        }
    ;

proplist:
    prop
  | prop TK_COMMA proplist
        {
            $1->next = $3;
        }
  | TK_STAR
        {
            $$ = NULL;
        }
    ;

prop:
    id TK_DOT id
        {
            $$ = alloc_property( ctx, $1, $3 );
            if (!$$)
                YYABORT;
        }
  | id
        {
            $$ = alloc_property( ctx, NULL, $1 );
            if (!$$)
                YYABORT;
        }
    ;

id:
    TK_ID
        {
            $$ = get_string( ctx, &$1 );
            if (!$$)
                YYABORT;
        }
    ;

number:
    TK_INTEGER
        {
            $$ = get_int( ctx );
        }
    ;

expr:
    TK_LP expr TK_RP
        {
            $$ = $2;
            if (!$$)
                YYABORT;
        }
  | expr TK_AND expr
        {
            $$ = expr_complex( ctx, $1, OP_AND, $3 );
            if (!$$)
                YYABORT;
        }
  | expr TK_OR expr
        {
            $$ = expr_complex( ctx, $1, OP_OR, $3 );
            if (!$$)
                YYABORT;
        }
  | TK_NOT expr
        {
            $$ = expr_unary( ctx, $2, OP_NOT );
            if (!$$)
                YYABORT;
        }
  | prop_val TK_EQ const_val
        {
            $$ = expr_complex( ctx, $1, OP_EQ, $3 );
            if (!$$)
                YYABORT;
        }
  | prop_val TK_GT const_val
        {
            $$ = expr_complex( ctx, $1, OP_GT, $3 );
            if (!$$)
                YYABORT;
        }
  | prop_val TK_LT const_val
        {
            $$ = expr_complex( ctx, $1, OP_LT, $3 );
            if (!$$)
                YYABORT;
        }
  | prop_val TK_LE const_val
        {
            $$ = expr_complex( ctx, $1, OP_LE, $3 );
            if (!$$)
                YYABORT;
        }
  | prop_val TK_GE const_val
        {
            $$ = expr_complex( ctx, $1, OP_GE, $3 );
            if (!$$)
                YYABORT;
        }
  | prop_val TK_NE const_val
        {
            $$ = expr_complex( ctx, $1, OP_NE, $3 );
            if (!$$)
                YYABORT;
        }
  | const_val TK_EQ prop_val
        {
            $$ = expr_complex( ctx, $1, OP_EQ, $3 );
            if (!$$)
                YYABORT;
        }
  | const_val TK_GT prop_val
        {
            $$ = expr_complex( ctx, $1, OP_GT, $3 );
            if (!$$)
                YYABORT;
        }
  | const_val TK_LT prop_val
        {
            $$ = expr_complex( ctx, $1, OP_LT, $3 );
            if (!$$)
                YYABORT;
        }
  | const_val TK_LE prop_val
        {
            $$ = expr_complex( ctx, $1, OP_LE, $3 );
            if (!$$)
                YYABORT;
        }
  | const_val TK_GE prop_val
        {
            $$ = expr_complex( ctx, $1, OP_GE, $3 );
            if (!$$)
                YYABORT;
        }
  | const_val TK_NE prop_val
        {
            $$ = expr_complex( ctx, $1, OP_NE, $3 );
            if (!$$)
                YYABORT;
        }
  | prop_val TK_LIKE string_val
        {
            $$ = expr_complex( ctx, $1, OP_LIKE, $3 );
            if (!$$)
                YYABORT;
        }
  | prop_val TK_IS TK_NULL
        {
            $$ = expr_unary( ctx, $1, OP_ISNULL );
            if (!$$)
                YYABORT;
        }
  | prop_val TK_IS TK_NOT TK_NULL
        {
            $$ = expr_unary( ctx, $1, OP_NOTNULL );
            if (!$$)
                YYABORT;
        }
    ;

string_val:
    TK_STRING
        {
            $$ = expr_sval( ctx, &$1 );
            if (!$$)
                YYABORT;
        }
    ;

prop_val:
    prop
        {
            $$ = expr_propval( ctx, $1 );
            if (!$$)
                YYABORT;
        }
    ;

const_val:
    number
        {
            $$ = expr_ival( ctx, $1 );
            if (!$$)
                YYABORT;
        }
  | TK_STRING
        {
            $$ = expr_sval( ctx, &$1 );
            if (!$$)
                YYABORT;
        }
  | TK_TRUE
        {
            $$ = expr_bval( ctx, -1 );
            if (!$$)
                YYABORT;
        }
  | TK_FALSE
        {
            $$ = expr_bval( ctx, 0 );
            if (!$$)
                YYABORT;
        }
    ;

%%

HRESULT parse_query( const WCHAR *str, struct view **view, struct list *mem )
{
    struct parser parser;
    int ret;

    *view = NULL;

    parser.cmd   = str;
    parser.idx   = 0;
    parser.len   = 0;
    parser.error = WBEM_E_INVALID_QUERY;
    parser.view  = view;
    parser.mem   = mem;

    ret = wql_parse( &parser );
    TRACE("wql_parse returned %d\n", ret);
    if (ret)
    {
        if (*parser.view)
        {
            destroy_view( *parser.view );
            *parser.view = NULL;
        }
        return parser.error;
    }
    return S_OK;
}

static const char id_char[] =
{
    0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
};

struct keyword
{
    const WCHAR *name;
    unsigned int len;
    int type;
};

#define MAX_TOKEN_LEN 6

static const WCHAR andW[] = {'A','N','D'};
static const WCHAR byW[] = {'B','Y'};
static const WCHAR falseW[] = {'F','A','L','S','E'};
static const WCHAR fromW[] = {'F','R','O','M'};
static const WCHAR isW[] = {'I','S'};
static const WCHAR likeW[] = {'L','I','K','E'};
static const WCHAR notW[] = {'N','O','T'};
static const WCHAR nullW[] = {'N','U','L','L'};
static const WCHAR orW[] = {'O','R'};
static const WCHAR selectW[] = {'S','E','L','E','C','T'};
static const WCHAR trueW[] = {'T','R','U','E'};
static const WCHAR whereW[] = {'W','H','E','R','E'};

static const struct keyword keyword_table[] =
{
  { andW,    SIZEOF(andW),    TK_AND },
  { byW,     SIZEOF(byW),     TK_BY },
  { falseW,  SIZEOF(falseW),  TK_FALSE },
  { fromW,   SIZEOF(fromW),   TK_FROM },
  { isW,     SIZEOF(isW),     TK_IS },
  { likeW,   SIZEOF(likeW),   TK_LIKE },
  { notW,    SIZEOF(notW),    TK_NOT },
  { nullW,   SIZEOF(nullW),   TK_NULL },
  { orW,     SIZEOF(orW),     TK_OR },
  { selectW, SIZEOF(selectW), TK_SELECT },
  { trueW,   SIZEOF(trueW),   TK_TRUE },
  { whereW,  SIZEOF(whereW),  TK_WHERE }
};

static int cmp_keyword( const void *arg1, const void *arg2 )
{
    const struct keyword *key1 = arg1, *key2 = arg2;
    int len = min( key1->len, key2->len );
    int ret;

    if ((ret = memicmpW( key1->name, key2->name, len ))) return ret;
    if (key1->len < key2->len) return -1;
    else if (key1->len > key2->len) return 1;
    return 0;
}

static int keyword_type( const WCHAR *str, unsigned int len )
{
    struct keyword key, *ret;

    if (len > MAX_TOKEN_LEN) return TK_ID;

    key.name = str;
    key.len  = len;
    key.type = 0;
    ret = bsearch( &key, keyword_table, SIZEOF(keyword_table), sizeof(struct keyword), cmp_keyword );
    if (ret) return ret->type;
    return TK_ID;
}

static int get_token( const WCHAR *s, int *token )
{
    int i;

    switch (*s)
    {
    case ' ':
    case '\t':
    case '\r':
    case '\n':
        for (i = 1; isspaceW( s[i] ); i++) {}
        *token = TK_SPACE;
        return i;
    case '-':
        if (!s[1]) return -1;
        *token = TK_MINUS;
        return 1;
    case '(':
        *token = TK_LP;
        return 1;
    case ')':
        *token = TK_RP;
        return 1;
    case '*':
        *token = TK_STAR;
        return 1;
    case '=':
        *token = TK_EQ;
        return 1;
    case '<':
        if (s[1] == '=' )
        {
            *token = TK_LE;
            return 2;
        }
        else if (s[1] == '>')
        {
            *token = TK_NE;
            return 2;
        }
        else
        {
            *token = TK_LT;
            return 1;
        }
    case '>':
        if (s[1] == '=')
        {
            *token = TK_GE;
            return 2;
        }
        else
        {
            *token = TK_GT;
            return 1;
        }
    case '!':
        if (s[1] != '=')
        {
            *token = TK_ILLEGAL;
            return 2;
        }
        else
        {
            *token = TK_NE;
            return 2;
        }
    case ',':
        *token = TK_COMMA;
        return 1;
    case '\"':
    case '\'':
        {
            for (i = 1; s[i]; i++)
            {
                if (s[i] == s[0]) break;
            }
            if (s[i]) i++;
            *token = TK_STRING;
            return i;
        }
    case '.':
        if (!isdigitW( s[1] ))
        {
            *token = TK_DOT;
            return 1;
        }
        /* fall through */
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
        *token = TK_INTEGER;
        for (i = 1; isdigitW( s[i] ); i++) {}
        return i;
    default:
        if (!id_char[*s]) break;

        for (i = 1; id_char[s[i]]; i++) {}
        *token = keyword_type( s, i );
        return i;
    }
    *token = TK_ILLEGAL;
    return 1;
}

static int wql_lex( void *p, struct parser *parser )
{
    struct string *str = p;
    int token = -1;
    do
    {
        parser->idx += parser->len;
        if (!parser->cmd[parser->idx]) return 0;
        parser->len = get_token( &parser->cmd[parser->idx], &token );
        if (!parser->len) break;

        str->data = &parser->cmd[parser->idx];
        str->len = parser->len;
    } while (token == TK_SPACE);
    return token;
}

static int wql_error( struct parser *parser, const char *str )
{
    ERR("%s\n", str);
    return 0;
}
