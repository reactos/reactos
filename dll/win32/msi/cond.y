%{

/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2003 Mike McCormack for CodeWeavers
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

#define COBJMACROS

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "msi.h"
#include "msiquery.h"
#include "objbase.h"
#include "oleauto.h"

#include "msipriv.h"
#include "winemsi_s.h"
#include "wine/debug.h"
#include "wine/exception.h"
#include "wine/unicode.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

typedef struct tag_yyinput
{
    MSIPACKAGE *package;
    LPCWSTR str;
    INT    n;
    MSICONDITION result;
    struct list mem;
} COND_input;

struct cond_str {
    LPCWSTR data;
    INT len;
};

struct value {
    enum value_type {
        VALUE_INTEGER,
        VALUE_LITERAL,
        VALUE_SYMBOL
    } type;
    union {
        INT integer;
        WCHAR *string;
    } u;
};

static LPWSTR COND_GetString( COND_input *info, const struct cond_str *str );
static LPWSTR COND_GetLiteral( COND_input *info, const struct cond_str *str );
static int cond_lex( void *COND_lval, COND_input *info);
static int cond_error( COND_input *info, const char *str);

static void *cond_alloc( COND_input *cond, unsigned int sz );
static void *cond_track_mem( COND_input *cond, void *ptr, unsigned int sz );
static void cond_free( void *ptr );

static INT compare_int( INT a, INT operator, INT b );
static INT compare_string( LPCWSTR a, INT operator, LPCWSTR b, BOOL convert );

static BOOL num_from_prop( LPCWSTR p, INT *val )
{
    INT ret = 0, sign = 1;

    if (!p)
        return FALSE;
    if (*p == '-')
    {
        sign = -1;
        p++;
    }
    if (!*p)
        return FALSE;
    while (*p)
    {
        if( *p < '0' || *p > '9' )
            return FALSE;
        ret = ret*10 + (*p - '0');
        p++;
    }
    *val = ret*sign;
    return TRUE;
}

static void value_free( struct value val )
{
    if (val.type != VALUE_INTEGER)
        cond_free( val.u.string );
}

%}

%lex-param { COND_input *info }
%parse-param { COND_input *info }
%define api.prefix {cond_}
%define api.pure

%union
{
    struct cond_str str;
    struct value value;
    LPWSTR identifier;
    INT operator;
    BOOL bool;
}

%token COND_SPACE
%token COND_OR COND_AND COND_NOT COND_XOR COND_IMP COND_EQV
%token COND_LT COND_GT COND_EQ COND_NE COND_GE COND_LE
%token COND_ILT COND_IGT COND_IEQ COND_INE COND_IGE COND_ILE
%token COND_LPAR COND_RPAR COND_TILDA COND_SS COND_ISS
%token COND_ILHS COND_IRHS COND_LHS COND_RHS
%token COND_PERCENT COND_DOLLARS COND_QUESTION COND_AMPER COND_EXCLAM
%token <str> COND_IDENT <str> COND_NUMBER <str> COND_LITER

%nonassoc COND_ERROR

%type <bool> expression boolean_term boolean_factor
%type <value> value
%type <identifier> identifier
%type <operator> operator

%%

condition:
    expression
        {
            COND_input* cond = (COND_input*) info;
            cond->result = $1;
        }
  | /* empty */
        {
            COND_input* cond = (COND_input*) info;
            cond->result = MSICONDITION_NONE;
        }
    ;

expression:
    boolean_term
        {
            $$ = $1;
        }
  | expression COND_OR boolean_term
        {
            $$ = $1 || $3;
        }
  | expression COND_IMP boolean_term
        {
            $$ = !$1 || $3;
        }
  | expression COND_XOR boolean_term
        {
            $$ = ( $1 || $3 ) && !( $1 && $3 );
        }
  | expression COND_EQV boolean_term
        {
            $$ = ( $1 && $3 ) || ( !$1 && !$3 );
        }
    ;

boolean_term:
    boolean_factor
        {
            $$ = $1;
        }
  | boolean_term COND_AND boolean_factor
        {
            $$ = $1 && $3;
        }
    ;

boolean_factor:
    COND_NOT boolean_factor
        {
            $$ = !$2;
        }
  | value
        {
            if ($1.type == VALUE_INTEGER)
                $$ = $1.u.integer ? 1 : 0;
            else
                $$ = $1.u.string && $1.u.string[0];
            value_free( $1 );
        }
  | value operator value
        {
            if ($1.type == VALUE_INTEGER && $3.type == VALUE_INTEGER)
            {
                $$ = compare_int($1.u.integer, $2, $3.u.integer);
            }
            else if ($1.type != VALUE_INTEGER && $3.type != VALUE_INTEGER)
            {
                $$ = compare_string($1.u.string, $2, $3.u.string,
                        $1.type == VALUE_SYMBOL || $3.type == VALUE_SYMBOL);
            }
            else if ($1.type == VALUE_LITERAL || $3.type == VALUE_LITERAL)
            {
                $$ = ($2 == COND_NE || $2 == COND_INE );
            }
            else if ($1.type == VALUE_SYMBOL) /* symbol operator integer */
            {
                int num;
                if (num_from_prop( $1.u.string, &num ))
                    $$ = compare_int( num, $2, $3.u.integer );
                else
                    $$ = ($2 == COND_NE || $2 == COND_INE );
            }
            else /* integer operator symbol */
            {
                int num;
                if (num_from_prop( $3.u.string, &num ))
                    $$ = compare_int( $1.u.integer, $2, num );
                else
                    $$ = ($2 == COND_NE || $2 == COND_INE );
            }

            value_free( $1 );
            value_free( $3 );
        }
  | COND_LPAR expression COND_RPAR
        {
            $$ = $2;
        }
    ;

operator:
    /* common functions */
    COND_EQ { $$ = COND_EQ; }
  | COND_NE { $$ = COND_NE; }
  | COND_LT { $$ = COND_LT; }
  | COND_GT { $$ = COND_GT; }
  | COND_LE { $$ = COND_LE; }
  | COND_GE { $$ = COND_GE; }
  | COND_SS { $$ = COND_SS; }
  | COND_IEQ { $$ = COND_IEQ; }
  | COND_INE { $$ = COND_INE; }
  | COND_ILT { $$ = COND_ILT; }
  | COND_IGT { $$ = COND_IGT; }
  | COND_ILE { $$ = COND_ILE; }
  | COND_IGE { $$ = COND_IGE; }
  | COND_ISS { $$ = COND_ISS; }
  | COND_LHS { $$ = COND_LHS; }
  | COND_RHS { $$ = COND_RHS; }
  | COND_ILHS { $$ = COND_ILHS; }
  | COND_IRHS { $$ = COND_IRHS; }
    ;

value:
    identifier
        {
            COND_input* cond = (COND_input*) info;
            UINT len;

            $$.type = VALUE_SYMBOL;
            $$.u.string = msi_dup_property( cond->package->db, $1 );
            if ($$.u.string)
            {
                len = (lstrlenW($$.u.string) + 1) * sizeof (WCHAR);
                $$.u.string = cond_track_mem( cond, $$.u.string, len );
            }
            cond_free( $1 );
        }
  | COND_PERCENT identifier
        {
            COND_input* cond = (COND_input*) info;
            UINT len = GetEnvironmentVariableW( $2, NULL, 0 );
            $$.type = VALUE_SYMBOL;
            $$.u.string = NULL;
            if (len++)
            {
                $$.u.string = cond_alloc( cond, len*sizeof (WCHAR) );
                if( !$$.u.string )
                    YYABORT;
                GetEnvironmentVariableW( $2, $$.u.string, len );
            }
            cond_free( $2 );
        }
  | COND_LITER
        {
            COND_input* cond = (COND_input*) info;
            $$.type = VALUE_LITERAL;
            $$.u.string = COND_GetLiteral( cond, &$1 );
            if( !$$.u.string )
                YYABORT;
        }
  | COND_NUMBER
        {
            COND_input* cond = (COND_input*) info;
            LPWSTR szNum = COND_GetString( cond, &$1 );
            if( !szNum )
                YYABORT;
            $$.type = VALUE_INTEGER;
            $$.u.integer = wcstol( szNum, NULL, 10 );
            cond_free( szNum );
        }
  | COND_DOLLARS identifier
        {
            COND_input* cond = (COND_input*) info;
            INSTALLSTATE install = INSTALLSTATE_UNKNOWN, action = INSTALLSTATE_UNKNOWN;

            if(MSI_GetComponentStateW(cond->package, $2, &install, &action ) != ERROR_SUCCESS)
            {
                $$.type = VALUE_LITERAL;
                $$.u.string = NULL;
            }
            else
            {
                $$.type = VALUE_INTEGER;
                $$.u.integer = action;
            }
            cond_free( $2 );
        }
  | COND_QUESTION identifier
        {
            COND_input* cond = (COND_input*) info;
            INSTALLSTATE install = INSTALLSTATE_UNKNOWN, action = INSTALLSTATE_UNKNOWN;

            if(MSI_GetComponentStateW(cond->package, $2, &install, &action ) != ERROR_SUCCESS)
            {
                $$.type = VALUE_LITERAL;
                $$.u.string = NULL;
            }
            else
            {
                $$.type = VALUE_INTEGER;
                $$.u.integer = install;
            }
            cond_free( $2 );
        }
  | COND_AMPER identifier
        {
            COND_input* cond = (COND_input*) info;
            INSTALLSTATE install, action;

            if (MSI_GetFeatureStateW(cond->package, $2, &install, &action ) != ERROR_SUCCESS)
            {
                $$.type = VALUE_LITERAL;
                $$.u.string = NULL;
            }
            else
            {
                $$.type = VALUE_INTEGER;
                $$.u.integer = action;
            }
            cond_free( $2 );
        }
  | COND_EXCLAM identifier
        {
            COND_input* cond = (COND_input*) info;
            INSTALLSTATE install = INSTALLSTATE_UNKNOWN, action = INSTALLSTATE_UNKNOWN;

            if(MSI_GetFeatureStateW(cond->package, $2, &install, &action ) != ERROR_SUCCESS)
            {
                $$.type = VALUE_LITERAL;
                $$.u.string = NULL;
            }
            else
            {
                $$.type = VALUE_INTEGER;
                $$.u.integer = install;
            }
            cond_free( $2 );
        }
    ;

identifier:
    COND_IDENT
        {
            COND_input* cond = (COND_input*) info;
            $$ = COND_GetString( cond, &$1 );
            if( !$$ )
                YYABORT;
        }
    ;

%%


static int COND_IsAlpha( WCHAR x )
{
    return( ( ( x >= 'A' ) && ( x <= 'Z' ) ) ||
            ( ( x >= 'a' ) && ( x <= 'z' ) ) ||
            ( ( x == '_' ) ) );
}

static int COND_IsNumber( WCHAR x )
{
    return( (( x >= '0' ) && ( x <= '9' ))  || (x =='-') || (x =='.') );
}

static WCHAR *strstriW( const WCHAR *str, const WCHAR *sub )
{
    LPWSTR strlower, sublower, r;
    strlower = CharLowerW( wcsdup( str ) );
    sublower = CharLowerW( wcsdup( sub ) );
    r = wcsstr( strlower, sublower );
    if (r)
        r = (LPWSTR)str + (r - strlower);
    free( strlower );
    free( sublower );
    return r;
}

static BOOL str_is_number( LPCWSTR str )
{
    int i;

    if (!*str)
        return FALSE;

    for (i = 0; i < lstrlenW( str ); i++)
        if (!iswdigit(str[i]))
            return FALSE;

    return TRUE;
}

static INT compare_substring( LPCWSTR a, INT operator, LPCWSTR b )
{
    int lhs, rhs;

    /* substring operators return 0 if LHS is missing */
    if (!a || !*a)
        return 0;

    /* substring operators return 1 if RHS is missing */
    if (!b || !*b)
        return 1;

    /* if both strings contain only numbers, use integer comparison */
    lhs = wcstol(a, NULL, 10);
    rhs = wcstol(b, NULL, 10);
    if (str_is_number(a) && str_is_number(b))
        return compare_int( lhs, operator, rhs );

    switch (operator)
    {
    case COND_SS:
        return wcsstr( a, b ) != 0;
    case COND_ISS:
        return strstriW( a, b ) != 0;
    case COND_LHS:
    {
        int l = lstrlenW( a );
        int r = lstrlenW( b );
        if (r > l) return 0;
        return !wcsncmp( a, b, r );
    }
    case COND_RHS:
    {
        int l = lstrlenW( a );
        int r = lstrlenW( b );
        if (r > l) return 0;
        return !wcsncmp( a + (l - r), b, r );
    }
    case COND_ILHS:
    {
        int l = lstrlenW( a );
        int r = lstrlenW( b );
        if (r > l) return 0;
        return !wcsnicmp( a, b, r );
    }
    case COND_IRHS:
    {
        int l = lstrlenW( a );
        int r = lstrlenW( b );
        if (r > l) return 0;
        return !wcsnicmp( a + (l - r), b, r );
    }
    default:
        ERR("invalid substring operator\n");
        return 0;
    }
    return 0;
}

static INT compare_string( LPCWSTR a, INT operator, LPCWSTR b, BOOL convert )
{
    if (operator >= COND_SS && operator <= COND_RHS)
        return compare_substring( a, operator, b );

    /* null and empty string are equivalent */
    if (!a) a = L"";
    if (!b) b = L"";

    if (convert && str_is_number(a) && str_is_number(b))
        return compare_int( wcstol(a, NULL, 10), operator, wcstol(b, NULL, 10) );

    /* a or b may be NULL */
    switch (operator)
    {
    case COND_LT:
        return wcscmp( a, b ) < 0;
    case COND_GT:
        return wcscmp( a, b ) > 0;
    case COND_EQ:
        return wcscmp( a, b ) == 0;
    case COND_NE:
        return wcscmp( a, b ) != 0;
    case COND_GE:
        return wcscmp( a, b ) >= 0;
    case COND_LE:
        return wcscmp( a, b ) <= 0;
    case COND_ILT:
        return wcsicmp( a, b ) < 0;
    case COND_IGT:
        return wcsicmp( a, b ) > 0;
    case COND_IEQ:
        return wcsicmp( a, b ) == 0;
    case COND_INE:
        return wcsicmp( a, b ) != 0;
    case COND_IGE:
        return wcsicmp( a, b ) >= 0;
    case COND_ILE:
        return wcsicmp( a, b ) <= 0;
    default:
        ERR("invalid string operator\n");
        return 0;
    }
    return 0;
}


static INT compare_int( INT a, INT operator, INT b )
{
    switch (operator)
    {
    case COND_LT:
    case COND_ILT:
        return a < b;
    case COND_GT:
    case COND_IGT:
        return a > b;
    case COND_EQ:
    case COND_IEQ:
        return a == b;
    case COND_NE:
    case COND_INE:
        return a != b;
    case COND_GE:
    case COND_IGE:
        return a >= b;
    case COND_LE:
    case COND_ILE:
        return a <= b;
    case COND_SS:
    case COND_ISS:
        return ( a & b ) ? 1 : 0;
    case COND_RHS:
        return ( ( a & 0xffff ) == b ) ? 1 : 0;
    case COND_LHS:
        return ( ( (a>>16) & 0xffff ) == b ) ? 1 : 0;
    default:
        ERR("invalid integer operator\n");
        return 0;
    }
    return 0;
}


static int COND_IsIdent( WCHAR x )
{
    return( COND_IsAlpha( x ) || COND_IsNumber( x ) || ( x == '_' )
            || ( x == '#' ) || (x == '.') );
}

static int COND_GetOperator( COND_input *cond )
{
    static const struct {
        const WCHAR str[4];
        int id;
    } table[] = {
        { L"~<=", COND_ILE },
        { L"~><", COND_ISS },
        { L"~>>", COND_IRHS },
        { L"~<>", COND_INE },
        { L"~>=", COND_IGE },
        { L"~<<", COND_ILHS },
        { L"~=",  COND_IEQ },
        { L"~<",  COND_ILT },
        { L"~>",  COND_IGT },
        { L">=",  COND_GE },
        { L"><",  COND_SS },
        { L"<<",  COND_LHS },
        { L"<>",  COND_NE },
        { L"<=",  COND_LE },
        { L">>",  COND_RHS },
        { L">",   COND_GT },
        { L"<",   COND_LT },
        { L"",    0 }
    };
    LPCWSTR p = &cond->str[cond->n];
    int i = 0, len;

    while ( 1 )
    {
        len = lstrlenW( table[i].str );
        if ( !len || 0 == wcsncmp( table[i].str, p, len ) )
            break;
        i++;
    }
    cond->n += len;
    return table[i].id;
}

static int COND_GetOne( struct cond_str *str, COND_input *cond )
{
    int rc, len = 1;
    WCHAR ch;

    str->data = &cond->str[cond->n];

    ch = str->data[0];

    switch( ch )
    {
    case 0: return 0;
    case '(': rc = COND_LPAR; break;
    case ')': rc = COND_RPAR; break;
    case '&': rc = COND_AMPER; break;
    case '!': rc = COND_EXCLAM; break;
    case '$': rc = COND_DOLLARS; break;
    case '?': rc = COND_QUESTION; break;
    case '%': rc = COND_PERCENT; break;
    case ' ': rc = COND_SPACE; break;
    case '=': rc = COND_EQ; break;

    case '~':
    case '<':
    case '>':
        rc = COND_GetOperator( cond );
        if (!rc)
            rc = COND_ERROR;
        return rc;
    default:
        rc = 0;
    }

    if ( rc )
    {
        cond->n += len;
        return rc;
    }

    if (ch == '"' )
    {
        LPCWSTR p = wcschr( str->data + 1, '"' );
        if (!p) return COND_ERROR;
        len = p - str->data + 1;
        rc = COND_LITER;
    }
    else if( COND_IsAlpha( ch ) )
    {
        while( COND_IsIdent( str->data[len] ) )
            len++;
        rc = COND_IDENT;

        if ( len == 3 )
        {
            if ( !wcsnicmp( str->data, L"NOT", len ) )
                rc = COND_NOT;
            else if( !wcsnicmp( str->data, L"AND", len ) )
                rc = COND_AND;
            else if( !wcsnicmp( str->data, L"XOR", len ) )
                rc = COND_XOR;
            else if( !wcsnicmp( str->data, L"EQV", len ) )
                rc = COND_EQV;
            else if( !wcsnicmp( str->data, L"IMP", len ) )
                rc = COND_IMP;
        }
        else if( (len == 2) && !wcsnicmp( str->data, L"OR", len ) )
            rc = COND_OR;
    }
    else if( COND_IsNumber( ch ) )
    {
        while( COND_IsNumber( str->data[len] ) )
            len++;
        rc = COND_NUMBER;
    }
    else
    {
        ERR("Got unknown character %c(%x)\n",ch,ch);
        return COND_ERROR;
    }

    cond->n += len;
    str->len = len;

    return rc;
}

static int cond_lex( void *COND_lval, COND_input *cond )
{
    int rc;
    struct cond_str *str = COND_lval;

    do {
        rc = COND_GetOne( str, cond );
    } while (rc == COND_SPACE);

    return rc;
}

static LPWSTR COND_GetString( COND_input *cond, const struct cond_str *str )
{
    LPWSTR ret;

    ret = cond_alloc( cond, (str->len+1) * sizeof (WCHAR) );
    if( ret )
    {
        memcpy( ret, str->data, str->len * sizeof(WCHAR));
        ret[str->len]=0;
    }
    TRACE("Got identifier %s\n",debugstr_w(ret));
    return ret;
}

static LPWSTR COND_GetLiteral( COND_input *cond, const struct cond_str *str )
{
    LPWSTR ret;

    ret = cond_alloc( cond, (str->len-1) * sizeof (WCHAR) );
    if( ret )
    {
        memcpy( ret, str->data+1, (str->len-2) * sizeof(WCHAR) );
        ret[str->len - 2]=0;
    }
    TRACE("Got literal %s\n",debugstr_w(ret));
    return ret;
}

static void *cond_alloc( COND_input *cond, unsigned int sz )
{
    struct list *mem;

    mem = malloc( sizeof (struct list) + sz );
    if( !mem )
        return NULL;

    list_add_head( &(cond->mem), mem );
    return mem + 1;
}

static void *cond_track_mem( COND_input *cond, void *ptr, unsigned int sz )
{
    void *new_ptr;

    if( !ptr )
        return ptr;

    new_ptr = cond_alloc( cond, sz );
    if( !new_ptr )
    {
        free( ptr );
        return NULL;
    }

    memcpy( new_ptr, ptr, sz );
    free( ptr );
    return new_ptr;
}

static void cond_free( void *ptr )
{
    struct list *mem = (struct list *)ptr - 1;

    if( ptr )
    {
        list_remove( mem );
        free( mem );
    }
}

static int cond_error( COND_input *info, const char *str )
{
    TRACE("%s\n", str );
    return 0;
}

MSICONDITION MSI_EvaluateConditionW( MSIPACKAGE *package, LPCWSTR szCondition )
{
    COND_input cond;
    MSICONDITION r;
    struct list *mem, *safety;

    TRACE("%s\n", debugstr_w( szCondition ) );

    if (szCondition == NULL) return MSICONDITION_NONE;

    cond.package = package;
    cond.str   = szCondition;
    cond.n     = 0;
    cond.result = MSICONDITION_ERROR;

    list_init( &cond.mem );

    if ( !cond_parse( &cond ) )
        r = cond.result;
    else
        r = MSICONDITION_ERROR;

    LIST_FOR_EACH_SAFE( mem, safety, &cond.mem )
    {
        /* The tracked memory lives directly after the list struct */
        void *ptr = mem + 1;
        if ( r != MSICONDITION_ERROR )
            WARN( "condition parser failed to free up some memory: %p\n", ptr );
        cond_free( ptr );
    }

    TRACE("%i <- %s\n", r, debugstr_w(szCondition));
    return r;
}

MSICONDITION WINAPI MsiEvaluateConditionW( MSIHANDLE hInstall, LPCWSTR szCondition )
{
    MSIPACKAGE *package;
    UINT ret;

    package = msihandle2msiinfo( hInstall, MSIHANDLETYPE_PACKAGE);
    if( !package )
    {
        MSIHANDLE remote;

        if (!(remote = msi_get_remote(hInstall)))
            return MSICONDITION_ERROR;

        if (!szCondition)
            return MSICONDITION_NONE;

        __TRY
        {
            ret = remote_EvaluateCondition(remote, szCondition);
        }
        __EXCEPT(rpc_filter)
        {
            ret = GetExceptionCode();
        }
        __ENDTRY

        return ret;
    }

    ret = MSI_EvaluateConditionW( package, szCondition );
    msiobj_release( &package->hdr );
    return ret;
}

MSICONDITION WINAPI MsiEvaluateConditionA( MSIHANDLE hInstall, LPCSTR szCondition )
{
    LPWSTR szwCond = NULL;
    MSICONDITION r;

    szwCond = strdupAtoW( szCondition );
    if( szCondition && !szwCond )
        return MSICONDITION_ERROR;

    r = MsiEvaluateConditionW( hInstall, szwCond );
    free( szwCond );
    return r;
}
