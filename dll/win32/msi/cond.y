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

#include "config.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wine/debug.h"
#include "wine/unicode.h"

#include "msi.h"
#include "msiquery.h"
#include "msipriv.h"

#define YYLEX_PARAM info
#define YYPARSE_PARAM info

static int cond_error(const char *str);

WINE_DEFAULT_DEBUG_CHANNEL(msi);

typedef struct tag_yyinput
{
    MSIPACKAGE *package;
    LPCWSTR str;
    INT    n;
    MSICONDITION result;
} COND_input;

struct cond_str {
    LPCWSTR data;
    INT len;
};

static LPWSTR COND_GetString( struct cond_str *str );
static LPWSTR COND_GetLiteral( struct cond_str *str );
static int cond_lex( void *COND_lval, COND_input *info);
static const WCHAR szEmpty[] = { 0 };

static INT compare_int( INT a, INT operator, INT b );
static INT compare_string( LPCWSTR a, INT operator, LPCWSTR b );

static INT compare_and_free_strings( LPWSTR a, INT op, LPWSTR b )
{
    INT r;

    r = compare_string( a, op, b );
    msi_free( a );
    msi_free( b );
    return r;
}

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

%}

%pure-parser

%union
{
    struct cond_str str;
    LPWSTR    string;
    INT       value;
}

%token COND_SPACE COND_EOF
%token COND_OR COND_AND COND_NOT COND_XOR COND_IMP COND_EQV
%token COND_LT COND_GT COND_EQ COND_NE COND_GE COND_LE
%token COND_ILT COND_IGT COND_IEQ COND_INE COND_IGE COND_ILE
%token COND_LPAR COND_RPAR COND_TILDA COND_SS COND_ISS
%token COND_ILHS COND_IRHS COND_LHS COND_RHS
%token COND_PERCENT COND_DOLLARS COND_QUESTION COND_AMPER COND_EXCLAM
%token <str> COND_IDENT <str> COND_NUMBER <str> COND_LITER

%nonassoc COND_ERROR COND_EOF

%type <value> expression boolean_term boolean_factor 
%type <value> value_i integer operator
%type <string> identifier symbol_s value_s literal

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
            $$ = $2 ? 0 : 1;
        }
  | value_i
        {
            $$ = $1 ? 1 : 0;
        }
  | value_s
        {
            $$ = ($1 && $1[0]) ? 1 : 0;
            msi_free($1);
        }
  | value_i operator value_i
        {
            $$ = compare_int( $1, $2, $3 );
        }
  | symbol_s operator value_i
        {
            int num;
            if (num_from_prop( $1, &num ))
                $$ = compare_int( num, $2, $3 );
            else 
                $$ = ($2 == COND_NE || $2 == COND_INE );
            msi_free($1);
        }
  | value_i operator symbol_s
        {
            int num;
            if (num_from_prop( $3, &num ))
                $$ = compare_int( $1, $2, num );
            else 
                $$ = ($2 == COND_NE || $2 == COND_INE );
            msi_free($3);
        }
  | symbol_s operator symbol_s
        {
            $$ = compare_and_free_strings( $1, $2, $3 );
        }
  | symbol_s operator literal
        {
            $$ = compare_and_free_strings( $1, $2, $3 );
        }
  | literal operator symbol_s
        {
            $$ = compare_and_free_strings( $1, $2, $3 );
        }
  | literal operator literal
        {
            $$ = compare_and_free_strings( $1, $2, $3 );
        }
  | literal operator value_i
        {
            $$ = 0;
            msi_free($1);
        }
  | value_i operator literal
        {
            $$ = 0;
            msi_free($3);
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

value_s:
    symbol_s
    {
        $$ = $1;
    } 
  | literal
    {
        $$ = $1;
    }
    ;

literal:
    COND_LITER
        {
            $$ = COND_GetLiteral(&$1);
            if( !$$ )
                YYABORT;
        }
    ;

value_i:
    integer
        {
            $$ = $1;
        }
  | COND_DOLLARS identifier
        {
            COND_input* cond = (COND_input*) info;
            INSTALLSTATE install = INSTALLSTATE_UNKNOWN, action = INSTALLSTATE_UNKNOWN;
      
            MSI_GetComponentStateW(cond->package, $2, &install, &action );
            $$ = action;
            msi_free( $2 );
        }
  | COND_QUESTION identifier
        {
            COND_input* cond = (COND_input*) info;
            INSTALLSTATE install = INSTALLSTATE_UNKNOWN, action = INSTALLSTATE_UNKNOWN;
      
            MSI_GetComponentStateW(cond->package, $2, &install, &action );
            $$ = install;
            msi_free( $2 );
        }
  | COND_AMPER identifier
        {
            COND_input* cond = (COND_input*) info;
            INSTALLSTATE install = INSTALLSTATE_UNKNOWN, action = INSTALLSTATE_UNKNOWN;
      
            MSI_GetFeatureStateW(cond->package, $2, &install, &action );
            $$ = action;
            msi_free( $2 );
        }
  | COND_EXCLAM identifier
        {
            COND_input* cond = (COND_input*) info;
            INSTALLSTATE install = INSTALLSTATE_UNKNOWN, action = INSTALLSTATE_UNKNOWN;
      
            MSI_GetFeatureStateW(cond->package, $2, &install, &action );
            $$ = install;
            msi_free( $2 );
        }
    ;

symbol_s:
    identifier
        {
            COND_input* cond = (COND_input*) info;

            $$ = msi_dup_property( cond->package, $1 );
            msi_free( $1 );
        }
    | COND_PERCENT identifier
        {
            UINT len = GetEnvironmentVariableW( $2, NULL, 0 );
            $$ = NULL;
            if (len++)
            {
                $$ = msi_alloc( len*sizeof (WCHAR) );
                GetEnvironmentVariableW( $2, $$, len );
            }
            msi_free( $2 );
        }
    ;

identifier:
    COND_IDENT
        {
            $$ = COND_GetString(&$1);
            if( !$$ )
                YYABORT;
        }
    ;

integer:
    COND_NUMBER
        {
            LPWSTR szNum = COND_GetString(&$1);
            if( !szNum )
                YYABORT;
            $$ = atoiW( szNum );
            msi_free( szNum );
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
    strlower = CharLowerW( strdupW( str ) );
    sublower = CharLowerW( strdupW( sub ) );
    r = strstrW( strlower, sublower );
    if (r)
        r = (LPWSTR)str + (r - strlower);
    msi_free( strlower );
    msi_free( sublower );
    return r;
}

static BOOL str_is_number( LPCWSTR str )
{
    int i;

    for (i = 0; i < lstrlenW( str ); i++)
        if (!isdigitW(str[i]))
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
    lhs = atoiW(a);
    rhs = atoiW(b);
    if (str_is_number(a) && str_is_number(b))
        return compare_int( lhs, operator, rhs );

    switch (operator)
    {
    case COND_SS:
        return strstrW( a, b ) ? 1 : 0;
    case COND_ISS:
        return strstriW( a, b ) ? 1 : 0;
    case COND_LHS:
    	return 0 == strncmpW( a, b, lstrlenW( b ) );
    case COND_RHS:
    	return 0 == lstrcmpW( a + (lstrlenW( a ) - lstrlenW( b )), b );
    case COND_ILHS:
    	return 0 == strncmpiW( a, b, lstrlenW( b ) );
    case COND_IRHS:
        return 0 == lstrcmpiW( a + (lstrlenW( a ) - lstrlenW( b )), b );
    default:
    	ERR("invalid substring operator\n");
        return 0;
    }
    return 0;
}

static INT compare_string( LPCWSTR a, INT operator, LPCWSTR b )
{
    if (operator >= COND_SS && operator <= COND_RHS)
        return compare_substring( a, operator, b );
	
    /* null and empty string are equivalent */
    if (!a) a = szEmpty;
    if (!b) b = szEmpty;

    /* a or b may be NULL */
    switch (operator)
    {
    case COND_LT:
        return -1 == lstrcmpW( a, b );
    case COND_GT:
        return  1 == lstrcmpW( a, b );
    case COND_EQ:
        return  0 == lstrcmpW( a, b );
    case COND_NE:
        return  0 != lstrcmpW( a, b );
    case COND_GE:
        return -1 != lstrcmpW( a, b );
    case COND_LE:
        return  1 != lstrcmpW( a, b );
    case COND_ILT:
        return -1 == lstrcmpiW( a, b );
    case COND_IGT:
        return  1 == lstrcmpiW( a, b );
    case COND_IEQ:
        return  0 == lstrcmpiW( a, b );
    case COND_INE:
        return  0 != lstrcmpiW( a, b );
    case COND_IGE:
        return -1 != lstrcmpiW( a, b );
    case COND_ILE:
        return  1 != lstrcmpiW( a, b );
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
        { {'~','=',0},     COND_IEQ },
        { {'~','<','=',0}, COND_ILE },
        { {'~','>','<',0}, COND_ISS },
        { {'~','>','>',0}, COND_IRHS },
        { {'~','<','>',0}, COND_INE },
        { {'~','<',0},     COND_ILT },
        { {'~','>','=',0}, COND_IGE },
        { {'~','<','<',0}, COND_ILHS },
        { {'~','>',0},     COND_IGT },
        { {'>','=',0},     COND_GE  },
        { {'>','<',0},     COND_SS  },
        { {'<','<',0},     COND_LHS },
        { {'<','>',0},     COND_NE  },
        { {'<','=',0},     COND_LE  },
        { {'>','>',0},     COND_RHS },
        { {'>',0},         COND_GT  },
        { {'<',0},         COND_LT  },
        { {0},             0        }
    };
    LPCWSTR p = &cond->str[cond->n];
    int i = 0, len;

    while ( 1 )
    {
        len = lstrlenW( table[i].str );
        if ( !len || 0 == strncmpW( table[i].str, p, len ) )
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
        break;

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
        LPCWSTR p = strchrW( str->data + 1, '"' );
	if (!p)
            return COND_ERROR;
        len = p - str->data + 1;
        rc = COND_LITER;
    }
    else if( COND_IsAlpha( ch ) )
    {
        static const WCHAR szNot[] = {'N','O','T',0};
        static const WCHAR szAnd[] = {'A','N','D',0};
        static const WCHAR szXor[] = {'X','O','R',0};
        static const WCHAR szEqv[] = {'E','Q','V',0};
        static const WCHAR szImp[] = {'I','M','P',0};
        static const WCHAR szOr[] = {'O','R',0};

        while( COND_IsIdent( str->data[len] ) )
            len++;
        rc = COND_IDENT;

        if ( len == 3 )
        {
            if ( !strncmpiW( str->data, szNot, len ) )
                rc = COND_NOT;
            else if( !strncmpiW( str->data, szAnd, len ) )
                rc = COND_AND;
            else if( !strncmpiW( str->data, szXor, len ) )
                rc = COND_XOR;
            else if( !strncmpiW( str->data, szEqv, len ) )
                rc = COND_EQV;
            else if( !strncmpiW( str->data, szImp, len ) )
                rc = COND_IMP;
        }
        else if( (len == 2) && !strncmpiW( str->data, szOr, len ) )
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

static LPWSTR COND_GetString( struct cond_str *str )
{
    LPWSTR ret;

    ret = msi_alloc( (str->len+1) * sizeof (WCHAR) );
    if( ret )
    {
        memcpy( ret, str->data, str->len * sizeof(WCHAR));
        ret[str->len]=0;
    }
    TRACE("Got identifier %s\n",debugstr_w(ret));
    return ret;
}

static LPWSTR COND_GetLiteral( struct cond_str *str )
{
    LPWSTR ret;

    ret = msi_alloc( (str->len-1) * sizeof (WCHAR) );
    if( ret )
    {
        memcpy( ret, str->data+1, (str->len-2) * sizeof(WCHAR) );
        ret[str->len - 2]=0;
    }
    TRACE("Got literal %s\n",debugstr_w(ret));
    return ret;
}

static int cond_error(const char *str)
{
    TRACE("%s\n", str );
    return 0;
}

MSICONDITION MSI_EvaluateConditionW( MSIPACKAGE *package, LPCWSTR szCondition )
{
    COND_input cond;
    MSICONDITION r;

    TRACE("%s\n", debugstr_w( szCondition ) );

    if ( szCondition == NULL )
    	return MSICONDITION_NONE;

    cond.package = package;
    cond.str   = szCondition;
    cond.n     = 0;
    cond.result = MSICONDITION_ERROR;
    
    if ( !cond_parse( &cond ) )
        r = cond.result;
    else
        r = MSICONDITION_ERROR;

    TRACE("%i <- %s\n", r, debugstr_w(szCondition));
    return r;
}

MSICONDITION WINAPI MsiEvaluateConditionW( MSIHANDLE hInstall, LPCWSTR szCondition )
{
    MSIPACKAGE *package;
    UINT ret;

    package = msihandle2msiinfo( hInstall, MSIHANDLETYPE_PACKAGE);
    if( !package)
        return MSICONDITION_ERROR;
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
    msi_free( szwCond );
    return r;
}
