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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"
#include "wine/unicode.h"

#include "msi.h"
#include "msiquery.h"
#include "msipriv.h"

#define YYLEX_PARAM info
#define YYPARSE_PARAM info

static int COND_error(char *str);

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
static int COND_lex( void *COND_lval, COND_input *info);

typedef INT (*comp_int)(INT a, INT b);
typedef INT (*comp_str)(LPWSTR a, LPWSTR b, BOOL caseless);
typedef INT (*comp_m1)(LPWSTR a,int b);
typedef INT (*comp_m2)(int a,LPWSTR b);

static INT comp_lt_i(INT a, INT b);
static INT comp_gt_i(INT a, INT b);
static INT comp_le_i(INT a, INT b);
static INT comp_ge_i(INT a, INT b);
static INT comp_eq_i(INT a, INT b);
static INT comp_ne_i(INT a, INT b);
static INT comp_bitand(INT a, INT b);
static INT comp_highcomp(INT a, INT b);
static INT comp_lowcomp(INT a, INT b);

static INT comp_eq_s(LPWSTR a, LPWSTR b, BOOL casless);
static INT comp_ne_s(LPWSTR a, LPWSTR b, BOOL casless);
static INT comp_lt_s(LPWSTR a, LPWSTR b, BOOL casless);
static INT comp_gt_s(LPWSTR a, LPWSTR b, BOOL casless);
static INT comp_le_s(LPWSTR a, LPWSTR b, BOOL casless);
static INT comp_ge_s(LPWSTR a, LPWSTR b, BOOL casless);
static INT comp_substring(LPWSTR a, LPWSTR b, BOOL casless);
static INT comp_start(LPWSTR a, LPWSTR b, BOOL casless);
static INT comp_end(LPWSTR a, LPWSTR b, BOOL casless);

static INT comp_eq_m1(LPWSTR a, INT b);
static INT comp_ne_m1(LPWSTR a, INT b);
static INT comp_lt_m1(LPWSTR a, INT b);
static INT comp_gt_m1(LPWSTR a, INT b);
static INT comp_le_m1(LPWSTR a, INT b);
static INT comp_ge_m1(LPWSTR a, INT b);

static INT comp_eq_m2(INT a, LPWSTR b);
static INT comp_ne_m2(INT a, LPWSTR b);
static INT comp_lt_m2(INT a, LPWSTR b);
static INT comp_gt_m2(INT a, LPWSTR b);
static INT comp_le_m2(INT a, LPWSTR b);
static INT comp_ge_m2(INT a, LPWSTR b);

%}

%pure-parser

%union
{
    struct cond_str str;
    LPWSTR    string;
    INT       value;
    comp_int  fn_comp_int;
    comp_str  fn_comp_str;
    comp_m1   fn_comp_m1;
    comp_m2   fn_comp_m2;
}

%token COND_SPACE COND_EOF COND_SPACE
%token COND_OR COND_AND COND_NOT
%token COND_LT COND_GT COND_EQ 
%token COND_LPAR COND_RPAR COND_TILDA
%token COND_PERCENT COND_DOLLARS COND_QUESTION COND_AMPER COND_EXCLAM
%token <str> COND_IDENT <str> COND_NUMBER <str> COND_LITER

%nonassoc COND_EOF COND_ERROR

%type <value> expression boolean_term boolean_factor 
%type <value> term value_i symbol_i integer
%type <string> identifier value_s symbol_s literal
%type <fn_comp_int> comp_op_i
%type <fn_comp_str> comp_op_s 
%type <fn_comp_m1>  comp_op_m1 
%type <fn_comp_m2>  comp_op_m2

%%

condition:
    expression
        {
            COND_input* cond = (COND_input*) info;
            cond->result = $1;
        }
    ;

expression:
    boolean_term 
        {
            $$ = $1;
        }
  | boolean_term COND_OR expression
        {
            $$ = $1 || $3;
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
    term
        {
            $$ = $1;
        }
  | COND_NOT term
        {
            $$ = ! $2;
        }
    ;


term:
    value_i
        {
            $$ = $1;
        }
  | value_s
        {
            $$ = $1[0] ? MSICONDITION_TRUE : MSICONDITION_FALSE;
        }
  | value_i comp_op_i value_i
        {
            $$ = $2( $1, $3 );
        }
  | value_s comp_op_s value_s
        {
            $$ = $2( $1, $3, FALSE );
        }
  | value_s COND_TILDA comp_op_s value_s
        {
            $$ = $3( $1, $4, TRUE );
        }
  | value_s comp_op_m1 value_i
        {
            $$ = $2( $1, $3 );
        }
  | value_i comp_op_m2 value_s
        {
            $$ = $2( $1, $3 );
        }
  | COND_LPAR expression COND_RPAR
        {
            $$ = $2;
        }
    ;

comp_op_i:
    /* common functions */
   COND_EQ
        {
            $$ = comp_eq_i;
        }
  | COND_LT COND_GT
        {
            $$ = comp_ne_i;
        }
  | COND_LT
        {
            $$ = comp_lt_i;
        }
  | COND_GT
        {
            $$ = comp_gt_i;
        }
  | COND_LT COND_EQ
        {
            $$ = comp_le_i;
        }
  | COND_GT COND_EQ
        {
            $$ = comp_ge_i;
        }
  /*Int only*/
  | COND_GT COND_LT
        {
            $$ = comp_bitand;
        }
  | COND_LT COND_LT
        {
            $$ = comp_highcomp;
        }
  | COND_GT COND_GT
        {
            $$ = comp_lowcomp;
        }
    ;

comp_op_s:
    /* common functions */
   COND_EQ
        {
            $$ = comp_eq_s;
        }
  | COND_LT COND_GT
        {
            $$ = comp_ne_s;
        }
  | COND_LT
        {
            $$ = comp_lt_s;
        }
  | COND_GT
        {
            $$ = comp_gt_s;
        }
  | COND_LT COND_EQ
        {
            $$ = comp_le_s;
        }
  | COND_GT COND_EQ
        {
            $$ = comp_ge_s;
        }
  /*string only*/
  | COND_GT COND_LT
        {
            $$ = comp_substring;
        }
  | COND_LT COND_LT
        {
            $$ = comp_start;
        }
  | COND_GT COND_GT
        {
            $$ = comp_end;
        }
    ;

comp_op_m1:
    /* common functions */
   COND_EQ
        {
            $$ = comp_eq_m1;
        }
  | COND_LT COND_GT
        {
            $$ = comp_ne_m1;
        }
  | COND_LT
        {
            $$ = comp_lt_m1;
        }
  | COND_GT
        {
            $$ = comp_gt_m1;
        }
  | COND_LT COND_EQ
        {
            $$ = comp_le_m1;
        }
  | COND_GT COND_EQ
        {
            $$ = comp_ge_m1;
        }
  /*Not valid for mixed compares*/
  | COND_GT COND_LT
        {
            $$ = 0;
        }
  | COND_LT COND_LT
        {
            $$ = 0;
        }
  | COND_GT COND_GT
        {
            $$ = 0;
        }
    ;

comp_op_m2:
    /* common functions */
   COND_EQ
        {
            $$ = comp_eq_m2;
        }
  | COND_LT COND_GT
        {
            $$ = comp_ne_m2;
        }
  | COND_LT
        {
            $$ = comp_lt_m2;
        }
  | COND_GT
        {
            $$ = comp_gt_m2;
        }
  | COND_LT COND_EQ
        {
            $$ = comp_le_m2;
        }
  | COND_GT COND_EQ
        {
            $$ = comp_ge_m2;
        }
  /*Not valid for mixed compares*/
  | COND_GT COND_LT
        {
            $$ = 0;
        }
  | COND_LT COND_LT
        {
            $$ = 0;
        }
  | COND_GT COND_GT
        {
            $$ = 0;
        }
    ;

value_i:
    symbol_i
        {
            $$ = $1;
        }
  | integer
        {
            $$ = $1;
        }
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

symbol_i:
    COND_DOLLARS identifier
        {
            COND_input* cond = (COND_input*) info;
            INSTALLSTATE install = INSTALLSTATE_UNKNOWN, action = INSTALLSTATE_UNKNOWN;
      
            MSI_GetComponentStateW(cond->package, $2, &install, &action );
            $$ = action;
            HeapFree( GetProcessHeap(), 0, $2 );
        }
  | COND_QUESTION identifier
        {
            COND_input* cond = (COND_input*) info;
            INSTALLSTATE install = INSTALLSTATE_UNKNOWN, action = INSTALLSTATE_UNKNOWN;
      
            MSI_GetComponentStateW(cond->package, $2, &install, &action );
            $$ = install;
            HeapFree( GetProcessHeap(), 0, $2 );
        }
  | COND_AMPER identifier
        {
            COND_input* cond = (COND_input*) info;
            INSTALLSTATE install = INSTALLSTATE_UNKNOWN, action = INSTALLSTATE_UNKNOWN;
      
            MSI_GetFeatureStateW(cond->package, $2, &install, &action );
            $$ = action;
            HeapFree( GetProcessHeap(), 0, $2 );
        }
  | COND_EXCLAM identifier
        {
            COND_input* cond = (COND_input*) info;
            INSTALLSTATE install = INSTALLSTATE_UNKNOWN, action = INSTALLSTATE_UNKNOWN;
      
            MSI_GetFeatureStateW(cond->package, $2, &install, &action );
            $$ = install;
            HeapFree( GetProcessHeap(), 0, $2 );
        }
    ;

symbol_s:
    identifier
        {
            DWORD sz;
            COND_input* cond = (COND_input*) info;

            sz = 0;
            MSI_GetPropertyW(cond->package, $1, NULL, &sz);
            if (sz == 0)
            {
                $$ = HeapAlloc( GetProcessHeap(), 0 ,sizeof(WCHAR));
                $$[0] = 0;
            }
            else
            {
                sz ++;
                $$ = HeapAlloc( GetProcessHeap(), 0, sz*sizeof (WCHAR) );

                /* Lookup the identifier */

                MSI_GetPropertyW(cond->package,$1,$$,&sz);
            }
            HeapFree( GetProcessHeap(), 0, $1 );
        }
    | COND_PERCENT identifier
        {
            UINT len = GetEnvironmentVariableW( $2, NULL, 0 );
            if( len++ )
            {
                $$ = HeapAlloc( GetProcessHeap(), 0, len*sizeof (WCHAR) );
                if( $$ )
                    GetEnvironmentVariableW( $2, $$, len );
            }
            HeapFree( GetProcessHeap(), 0, $2 );
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
            HeapFree( GetProcessHeap(), 0, szNum );
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


/* the mess of comparison functions */

static INT comp_lt_i(INT a, INT b)
{ return (a < b); }
static INT comp_gt_i(INT a, INT b)
{ return (a > b); }
static INT comp_le_i(INT a, INT b)
{ return (a <= b); }
static INT comp_ge_i(INT a, INT b)
{ return (a >= b); }
static INT comp_eq_i(INT a, INT b)
{ return (a == b); }
static INT comp_ne_i(INT a, INT b)
{ return (a != b); }
static INT comp_bitand(INT a, INT b)
{ return a & b;}
static INT comp_highcomp(INT a, INT b)
{ return HIWORD(a)==b; }
static INT comp_lowcomp(INT a, INT b)
{ return LOWORD(a)==b; }

static INT comp_eq_s(LPWSTR a, LPWSTR b, BOOL casless)
{ if (casless) return !strcmpiW(a,b); else return !strcmpW(a,b);}
static INT comp_ne_s(LPWSTR a, LPWSTR b, BOOL casless)
{ if (casless) return strcmpiW(a,b); else  return strcmpW(a,b);}
static INT comp_lt_s(LPWSTR a, LPWSTR b, BOOL casless)
{ if (casless) return strcmpiW(a,b)<0; else return strcmpW(a,b)<0;}
static INT comp_gt_s(LPWSTR a, LPWSTR b, BOOL casless)
{ if (casless) return strcmpiW(a,b)>0; else return strcmpW(a,b)>0;}
static INT comp_le_s(LPWSTR a, LPWSTR b, BOOL casless)
{ if (casless) return strcmpiW(a,b)<=0; else return strcmpW(a,b)<=0;}
static INT comp_ge_s(LPWSTR a, LPWSTR b, BOOL casless)
{ if (casless) return strcmpiW(a,b)>=0; else return  strcmpW(a,b)>=0;}
static INT comp_substring(LPWSTR a, LPWSTR b, BOOL casless)
/* ERROR NOT WORKING REWRITE */
{ if (casless) return strstrW(a,b)!=NULL; else return strstrW(a,b)!=NULL;}
static INT comp_start(LPWSTR a, LPWSTR b, BOOL casless)
{ if (casless) return strncmpiW(a,b,strlenW(b))==0; 
  else return strncmpW(a,b,strlenW(b))==0;}
static INT comp_end(LPWSTR a, LPWSTR b, BOOL casless)
{ 
    int i = strlenW(a); 
    int j = strlenW(b); 
    if (j>i)
        return 0;
    if (casless) return (!strcmpiW(&a[i-j-1],b));
    else  return (!strcmpW(&a[i-j-1],b));
}


static INT comp_eq_m1(LPWSTR a, INT b)
{ if (COND_IsNumber(a[0])) return atoiW(a)==b; else return 0;}
static INT comp_ne_m1(LPWSTR a, INT b)
{ if (COND_IsNumber(a[0])) return atoiW(a)!=b; else return 1;}
static INT comp_lt_m1(LPWSTR a, INT b)
{ if (COND_IsNumber(a[0])) return atoiW(a)<b; else return 0;}
static INT comp_gt_m1(LPWSTR a, INT b)
{ if (COND_IsNumber(a[0])) return atoiW(a)>b; else return 0;}
static INT comp_le_m1(LPWSTR a, INT b)
{ if (COND_IsNumber(a[0])) return atoiW(a)<=b; else return 0;}
static INT comp_ge_m1(LPWSTR a, INT b)
{ if (COND_IsNumber(a[0])) return atoiW(a)>=b; else return 0;}

static INT comp_eq_m2(INT a, LPWSTR b)
{ if (COND_IsNumber(b[0])) return a == atoiW(b); else return 0;}
static INT comp_ne_m2(INT a, LPWSTR b)
{ if (COND_IsNumber(b[0])) return a != atoiW(b); else return 1;}
static INT comp_lt_m2(INT a, LPWSTR b)
{ if (COND_IsNumber(b[0])) return a < atoiW(b); else return 0;}
static INT comp_gt_m2(INT a, LPWSTR b)
{ if (COND_IsNumber(b[0])) return a > atoiW(b); else return 0;}
static INT comp_le_m2(INT a, LPWSTR b)
{ if (COND_IsNumber(b[0])) return a <= atoiW(b); else return 0;}
static INT comp_ge_m2(INT a, LPWSTR b)
{ if (COND_IsNumber(b[0])) return a >= atoiW(b); else return 0;}



static int COND_IsIdent( WCHAR x )
{
    return( COND_IsAlpha( x ) || COND_IsNumber( x ) || ( x == '_' ) 
            || ( x == '#' ) || (x == '.') );
}

static int COND_GetOne( struct cond_str *str, COND_input *cond )
{
    static const WCHAR szNot[] = {'N','O','T',0};
    static const WCHAR szAnd[] = {'A','N','D',0};
    static const WCHAR szOr[] = {'O','R',0};
    WCHAR ch;
    int rc, len = 1;

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
    case '~': rc = COND_TILDA; break;
    case '<': rc = COND_LT; break;
    case '>': rc = COND_GT; break;
    case '"':
	{
	    const WCHAR *ch2 = str->data + 1;


	    while ( *ch2 && *ch2 != '"' )
	    	++ch2;
	    if (*ch2 == '"')
	    {
	        len = ch2 - str->data + 1;
		rc = COND_LITER;
		break;
	    }
	}
	ERR("Unterminated string\n");
	rc = COND_ERROR;
    	break;
    default: 
        if( COND_IsAlpha( ch ) )
        {
            while( COND_IsIdent( str->data[len] ) )
                len++;
            rc = COND_IDENT;
            break;
        }

        if( COND_IsNumber( ch ) )
        {
            while( COND_IsNumber( str->data[len] ) )
                len++;
            rc = COND_NUMBER;
            break;
        }

        ERR("Got unknown character %c(%x)\n",ch,ch);
        rc = COND_ERROR;
        break;
    }

    /* keyword identifiers */
    if( rc == COND_IDENT )
    {
        if( (len==3) && (strncmpiW(str->data,szNot,len)==0) )
            rc = COND_NOT;
        else if( (len==3) && (strncmpiW(str->data,szAnd,len)==0) )
            rc = COND_AND;
        else if( (len==2) && (strncmpiW(str->data,szOr,len)==0) )
            rc = COND_OR;
    }

    cond->n += len;
    str->len = len;

    return rc;
}

static int COND_lex( void *COND_lval, COND_input *cond )
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

    ret = HeapAlloc( GetProcessHeap(), 0, (str->len+1) * sizeof (WCHAR) );
    if( ret )
    {
        strncpyW( ret, str->data, str->len );
        ret[str->len]=0;
    }
    TRACE("Got identifier %s\n",debugstr_w(ret));
    return ret;
}

static LPWSTR COND_GetLiteral( struct cond_str *str )
{
    LPWSTR ret;

    ret = HeapAlloc( GetProcessHeap(), 0, (str->len-1) * sizeof (WCHAR) );
    if( ret )
    {
        memcpy( ret, str->data+1, (str->len-2) * sizeof(WCHAR) );
        ret[str->len - 2]=0;
    }
    TRACE("Got literal %s\n",debugstr_w(ret));
    return ret;
}

static int COND_error(char *str)
{
    return 0;
}

MSICONDITION MSI_EvaluateConditionW( MSIPACKAGE *package, LPCWSTR szCondition )
{
    COND_input cond;
    MSICONDITION r;

    cond.package = package;
    cond.str   = szCondition;
    cond.n     = 0;
    cond.result = -1;
    
    TRACE("Evaluating %s\n",debugstr_w(szCondition));    

    if( szCondition && !COND_parse( &cond ) )
        r = cond.result;
    else
        r = MSICONDITION_ERROR;

    TRACE("Evaluates to %i\n",r);
    return r;
}

MSICONDITION WINAPI MsiEvaluateConditionW( MSIHANDLE hInstall, LPCWSTR szCondition )
{
    MSIPACKAGE *package;
    UINT ret;

    package = msihandle2msiinfo( hInstall, MSIHANDLETYPE_PACKAGE);
    if( !package)
        return ERROR_INVALID_HANDLE;
    ret = MSI_EvaluateConditionW( package, szCondition );
    msiobj_release( &package->hdr );
    return ret;
}

MSICONDITION WINAPI MsiEvaluateConditionA( MSIHANDLE hInstall, LPCSTR szCondition )
{
    LPWSTR szwCond = NULL;
    MSICONDITION r;

    if( szCondition )
    {
        UINT len = MultiByteToWideChar( CP_ACP, 0, szCondition, -1, NULL, 0 );
        szwCond = HeapAlloc( GetProcessHeap(), 0, len * sizeof (WCHAR) );
        MultiByteToWideChar( CP_ACP, 0, szCondition, -1, szwCond, len );
    }

    r = MsiEvaluateConditionW( hInstall, szwCond );

    HeapFree( GetProcessHeap(), 0, szwCond );

    return r;
}
