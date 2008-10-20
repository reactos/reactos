%{
/*
 * IDL Compiler
 *
 * Copyright 2002 Ove Kaaven
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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif

#include "widl.h"
#include "utils.h"
#include "parser.h"
#include "header.h"
#include "typelib.h"
#include "typegen.h"

#if defined(YYBYACC)
	/* Berkeley yacc (byacc) doesn't seem to know about these */
	/* Some *BSD supplied versions do define these though */
# ifndef YYEMPTY
#  define YYEMPTY	(-1)	/* Empty lookahead value of yychar */
# endif
# ifndef YYLEX
#  define YYLEX		yylex()
# endif

#elif defined(YYBISON)
	/* Bison was used for original development */
	/* #define YYEMPTY -2 */
	/* #define YYLEX   yylex() */

#else
	/* No yacc we know yet */
# if !defined(YYEMPTY) || !defined(YYLEX)
#  error Yacc version/type unknown. This version needs to be verified for settings of YYEMPTY and YYLEX.
# elif defined(__GNUC__)	/* gcc defines the #warning directive */
#  warning Yacc version/type unknown. It defines YYEMPTY and YYLEX, but is not tested
  /* #else we just take a chance that it works... */
# endif
#endif

unsigned char pointer_default = RPC_FC_UP;

typedef struct list typelist_t;
struct typenode {
  type_t *type;
  struct list entry;
};

typelist_t incomplete_types = LIST_INIT(incomplete_types);

static void add_incomplete(type_t *t);
static void fix_incomplete(void);

static str_list_t *append_str(str_list_t *list, char *str);
static attr_list_t *append_attr(attr_list_t *list, attr_t *attr);
static attr_t *make_attr(enum attr_type type);
static attr_t *make_attrv(enum attr_type type, unsigned long val);
static attr_t *make_attrp(enum attr_type type, void *val);
static expr_t *make_expr(enum expr_type type);
static expr_t *make_exprl(enum expr_type type, long val);
static expr_t *make_exprd(enum expr_type type, double val);
static expr_t *make_exprs(enum expr_type type, char *val);
static expr_t *make_exprt(enum expr_type type, type_t *tref, expr_t *expr);
static expr_t *make_expr1(enum expr_type type, expr_t *expr);
static expr_t *make_expr2(enum expr_type type, expr_t *exp1, expr_t *exp2);
static expr_t *make_expr3(enum expr_type type, expr_t *expr1, expr_t *expr2, expr_t *expr3);
static type_t *make_type(unsigned char type, type_t *ref);
static expr_list_t *append_expr(expr_list_t *list, expr_t *expr);
static array_dims_t *append_array(array_dims_t *list, expr_t *expr);
static void set_type(var_t *v, type_t *type, int ptr_level, array_dims_t *arr, int top);
static ifref_list_t *append_ifref(ifref_list_t *list, ifref_t *iface);
static ifref_t *make_ifref(type_t *iface);
static var_list_t *append_var(var_list_t *list, var_t *var);
static var_t *make_var(char *name);
static pident_list_t *append_pident(pident_list_t *list, pident_t *p);
static pident_t *make_pident(var_t *var);
static func_list_t *append_func(func_list_t *list, func_t *func);
static func_t *make_func(var_t *def, var_list_t *args);
static type_t *make_class(char *name);
static type_t *make_safearray(type_t *type);
static type_t *make_builtin(char *name);
static type_t *make_int(int sign);

static type_t *reg_type(type_t *type, const char *name, int t);
static type_t *reg_typedefs(type_t *type, var_list_t *names, attr_list_t *attrs);
static type_t *find_type(const char *name, int t);
static type_t *find_type2(char *name, int t);
static type_t *get_type(unsigned char type, char *name, int t);
static type_t *get_typev(unsigned char type, var_t *name, int t);
static int get_struct_type(var_list_t *fields);

static var_t *reg_const(var_t *var);
static var_t *find_const(char *name, int f);

static void write_libid(const char *name, const attr_list_t *attr);
static void write_clsid(type_t *cls);
static void write_diid(type_t *iface);
static void write_iid(type_t *iface);

static int compute_method_indexes(type_t *iface);
static char *gen_name(void);
static void process_typedefs(var_list_t *names);
static void check_arg(var_t *arg);
static void check_all_user_types(ifref_list_t *ifaces);

#define tsENUM   1
#define tsSTRUCT 2
#define tsUNION  3

%}
%union {
	attr_t *attr;
	attr_list_t *attr_list;
	str_list_t *str_list;
	expr_t *expr;
	expr_list_t *expr_list;
	array_dims_t *array_dims;
	type_t *type;
	var_t *var;
	var_list_t *var_list;
	pident_t *pident;
	pident_list_t *pident_list;
	func_t *func;
	func_list_t *func_list;
	ifref_t *ifref;
	ifref_list_t *ifref_list;
	char *str;
	UUID *uuid;
	unsigned int num;
	double dbl;
	interface_info_t ifinfo;
}

%token <str> aIDENTIFIER
%token <str> aKNOWNTYPE
%token <num> aNUM aHEXNUM
%token <dbl> aDOUBLE
%token <str> aSTRING
%token <uuid> aUUID
%token aEOF
%token SHL SHR
%token tAGGREGATABLE tALLOCATE tAPPOBJECT tASYNC tASYNCUUID
%token tAUTOHANDLE tBINDABLE tBOOLEAN tBROADCAST tBYTE tBYTECOUNT
%token tCALLAS tCALLBACK tCASE tCDECL tCHAR tCOCLASS tCODE tCOMMSTATUS
%token tCONST tCONTEXTHANDLE tCONTEXTHANDLENOSERIALIZE
%token tCONTEXTHANDLESERIALIZE tCONTROL tCPPQUOTE
%token tDEFAULT
%token tDEFAULTCOLLELEM
%token tDEFAULTVALUE
%token tDEFAULTVTABLE
%token tDISPLAYBIND
%token tDISPINTERFACE
%token tDLLNAME tDOUBLE tDUAL
%token tENDPOINT
%token tENTRY tENUM tERRORSTATUST
%token tEXPLICITHANDLE tEXTERN
%token tFALSE
%token tFLOAT
%token tHANDLE
%token tHANDLET
%token tHELPCONTEXT tHELPFILE
%token tHELPSTRING tHELPSTRINGCONTEXT tHELPSTRINGDLL
%token tHIDDEN
%token tHYPER tID tIDEMPOTENT
%token tIIDIS
%token tIMMEDIATEBIND
%token tIMPLICITHANDLE
%token tIMPORT tIMPORTLIB
%token tIN tINLINE
%token tINPUTSYNC
%token tINT tINT64
%token tINTERFACE
%token tLCID
%token tLENGTHIS tLIBRARY
%token tLOCAL
%token tLONG
%token tMETHODS
%token tMODULE
%token tNONBROWSABLE
%token tNONCREATABLE
%token tNONEXTENSIBLE
%token tOBJECT tODL tOLEAUTOMATION
%token tOPTIONAL
%token tOUT
%token tPOINTERDEFAULT
%token tPROPERTIES
%token tPROPGET tPROPPUT tPROPPUTREF
%token tPTR
%token tPUBLIC
%token tRANGE
%token tREADONLY tREF
%token tREQUESTEDIT
%token tRESTRICTED
%token tRETVAL
%token tSAFEARRAY
%token tSHORT
%token tSIGNED
%token tSINGLE
%token tSIZEIS tSIZEOF
%token tSMALL
%token tSOURCE
%token tSTDCALL
%token tSTRICTCONTEXTHANDLE
%token tSTRING tSTRUCT
%token tSWITCH tSWITCHIS tSWITCHTYPE
%token tTRANSMITAS
%token tTRUE
%token tTYPEDEF
%token tUNION
%token tUNIQUE
%token tUNSIGNED
%token tUUID
%token tV1ENUM
%token tVARARG
%token tVERSION
%token tVOID
%token tWCHAR tWIREMARSHAL

%type <attr> attribute
%type <attr_list> m_attributes attributes attrib_list
%type <str_list> str_list
%type <expr> m_expr expr expr_const
%type <expr_list> m_exprs /* exprs expr_list */ expr_list_const
%type <array_dims> array array_list
%type <ifinfo> interfacehdr
%type <type> inherit interface interfacedef interfacedec
%type <type> dispinterface dispinterfacehdr dispinterfacedef
%type <type> module modulehdr moduledef
%type <type> base_type int_std
%type <type> enumdef structdef uniondef
%type <type> type
%type <ifref> coclass_int
%type <ifref_list> gbl_statements coclass_ints
%type <var> arg field s_field case enum constdef externdef
%type <var_list> m_args no_args args fields cases enums enum_list dispint_props
%type <var> m_ident t_ident ident
%type <pident> p_ident pident
%type <pident_list> pident_list
%type <func> funcdef
%type <func_list> int_statements dispint_meths
%type <type> coclass coclasshdr coclassdef
%type <num> pointer_type version
%type <str> libraryhdr
%type <uuid> uuid_string

%left ','
%right '?' ':'
%left '|'
%left '&'
%left '-' '+'
%left '*' '/'
%left SHL SHR
%right '~'
%right CAST
%right PPTR
%right NEG
%right ADDRESSOF

%%

input:   gbl_statements				{ fix_incomplete();
						  check_all_user_types($1);
						  write_proxies($1);
						  write_client($1);
						  write_server($1);
						  write_dlldata($1);
						}
	;

gbl_statements:					{ $$ = NULL; }
	| gbl_statements interfacedec		{ $$ = $1; }
	| gbl_statements interfacedef		{ $$ = append_ifref( $1, make_ifref($2) ); }
	| gbl_statements coclass ';'		{ $$ = $1;
						  reg_type($2, $2->name, 0);
						  if (!parse_only && do_header) write_coclass_forward($2);
						}
	| gbl_statements coclassdef		{ $$ = $1;
						  add_typelib_entry($2);
						  reg_type($2, $2->name, 0);
						  if (!parse_only && do_header) write_coclass_forward($2);
						}
	| gbl_statements moduledef		{ $$ = $1; add_typelib_entry($2); }
	| gbl_statements librarydef		{ $$ = $1; }
	| gbl_statements statement		{ $$ = $1; }
	;

imp_statements:					{}
	| imp_statements interfacedec		{ if (!parse_only) add_typelib_entry($2); }
	| imp_statements interfacedef		{ if (!parse_only) add_typelib_entry($2); }
	| imp_statements coclass ';'		{ reg_type($2, $2->name, 0); if (!parse_only && do_header) write_coclass_forward($2); }
	| imp_statements coclassdef		{ if (!parse_only) add_typelib_entry($2);
						  reg_type($2, $2->name, 0);
						  if (!parse_only && do_header) write_coclass_forward($2);
						}
	| imp_statements moduledef		{ if (!parse_only) add_typelib_entry($2); }
	| imp_statements statement		{}
	| imp_statements importlib		{}
	| imp_statements librarydef		{}
	;

int_statements:					{ $$ = NULL; }
	| int_statements funcdef ';'		{ $$ = append_func( $1, $2 ); }
	| int_statements statement		{ $$ = $1; }
	;

statement: ';'					{}
	| constdef ';'				{ if (!parse_only && do_header) { write_constdef($1); } }
	| cppquote				{}
	| enumdef ';'				{ if (!parse_only && do_header) {
						    write_type_def_or_decl(header, $1, FALSE, NULL);
						    fprintf(header, ";\n\n");
						  }
						}
	| externdef ';'				{ if (!parse_only && do_header) { write_externdef($1); } }
	| import				{}
	| structdef ';'				{ if (!parse_only && do_header) {
						    write_type_def_or_decl(header, $1, FALSE, NULL);
						    fprintf(header, ";\n\n");
						  }
						}
	| typedef ';'				{}
	| uniondef ';'				{ if (!parse_only && do_header) {
						    write_type_def_or_decl(header, $1, FALSE, NULL);
						    fprintf(header, ";\n\n");
						  }
						}
	;

cppquote: tCPPQUOTE '(' aSTRING ')'		{ if (!parse_only && do_header) fprintf(header, "%s\n", $3); }
	;
import_start: tIMPORT aSTRING ';'		{ assert(yychar == YYEMPTY);
						  if (!do_import($2)) yychar = aEOF; }
	;
import:   import_start imp_statements aEOF	{}
	;

importlib: tIMPORTLIB '(' aSTRING ')'		{ if(!parse_only) add_importlib($3); }
	;

libraryhdr: tLIBRARY aIDENTIFIER		{ $$ = $2; }
	;
library_start: attributes libraryhdr '{'	{ if (!parse_only) start_typelib($2, $1);
						  if (!parse_only && do_header) write_library($2, $1);
						  if (!parse_only && do_idfile) write_libid($2, $1);
						}
	;
librarydef: library_start imp_statements '}'	{ if (!parse_only) end_typelib(); }
	;

m_args:						{ $$ = NULL; }
	| args
	;

no_args:  tVOID					{ $$ = NULL; }
	;

args:	  arg					{ check_arg($1); $$ = append_var( NULL, $1 ); }
	| args ',' arg				{ check_arg($3); $$ = append_var( $1, $3); }
	| no_args
	;

/* split into two rules to get bison to resolve a tVOID conflict */
arg:	  attributes type pident array		{ $$ = $3->var;
						  $$->attrs = $1;
						  set_type($$, $2, $3->ptr_level, $4, TRUE);
						  free($3);
						}
	| type pident array			{ $$ = $2->var;
						  set_type($$, $1, $2->ptr_level, $3, TRUE);
						  free($2);
						}
	| attributes type pident '(' m_args ')'	{ $$ = $3->var;
						  $$->attrs = $1;
						  set_type($$, $2, $3->ptr_level - 1, NULL, TRUE);
						  free($3);
						  $$->args = $5;
						}
	| type pident '(' m_args ')'		{ $$ = $2->var;
						  set_type($$, $1, $2->ptr_level - 1, NULL, TRUE);
						  free($2);
						  $$->args = $4;
						}
	;

array:						{ $$ = NULL; }
	| '[' array_list ']'			{ $$ = $2; }
	| '[' '*' ']'				{ $$ = append_array( NULL, make_expr(EXPR_VOID) ); }
	;

array_list: m_expr /* size of first dimension is optional */ { $$ = append_array( NULL, $1 ); }
	| array_list ',' expr                   { $$ = append_array( $1, $3 ); }
	| array_list ']' '[' expr               { $$ = append_array( $1, $4 ); }
	;

m_attributes:					{ $$ = NULL; }
	| attributes
	;

attributes:
	  '[' attrib_list ']'			{ $$ = $2;
						  if (!$$)
						    error_loc("empty attribute lists unsupported\n");
						}
	;

attrib_list: attribute                          { $$ = append_attr( NULL, $1 ); }
	| attrib_list ',' attribute             { $$ = append_attr( $1, $3 ); }
	| attrib_list ']' '[' attribute         { $$ = append_attr( $1, $4 ); }
	;

str_list: aSTRING                               { $$ = append_str( NULL, $1 ); }
	| str_list ',' aSTRING                  { $$ = append_str( $1, $3 ); }
	;

attribute:					{ $$ = NULL; }
	| tAGGREGATABLE				{ $$ = make_attr(ATTR_AGGREGATABLE); }
	| tAPPOBJECT				{ $$ = make_attr(ATTR_APPOBJECT); }
	| tASYNC				{ $$ = make_attr(ATTR_ASYNC); }
	| tAUTOHANDLE				{ $$ = make_attr(ATTR_AUTO_HANDLE); }
	| tBINDABLE				{ $$ = make_attr(ATTR_BINDABLE); }
	| tCALLAS '(' ident ')'			{ $$ = make_attrp(ATTR_CALLAS, $3); }
	| tCASE '(' expr_list_const ')'		{ $$ = make_attrp(ATTR_CASE, $3); }
	| tCONTEXTHANDLE			{ $$ = make_attrv(ATTR_CONTEXTHANDLE, 0); }
	| tCONTEXTHANDLENOSERIALIZE		{ $$ = make_attrv(ATTR_CONTEXTHANDLE, 0); /* RPC_CONTEXT_HANDLE_DONT_SERIALIZE */ }
	| tCONTEXTHANDLESERIALIZE		{ $$ = make_attrv(ATTR_CONTEXTHANDLE, 0); /* RPC_CONTEXT_HANDLE_SERIALIZE */ }
	| tCONTROL				{ $$ = make_attr(ATTR_CONTROL); }
	| tDEFAULT				{ $$ = make_attr(ATTR_DEFAULT); }
	| tDEFAULTCOLLELEM			{ $$ = make_attr(ATTR_DEFAULTCOLLELEM); }
	| tDEFAULTVALUE '(' expr_const ')'	{ $$ = make_attrp(ATTR_DEFAULTVALUE_EXPR, $3); }
	| tDEFAULTVALUE '(' aSTRING ')'		{ $$ = make_attrp(ATTR_DEFAULTVALUE_STRING, $3); }
	| tDEFAULTVTABLE			{ $$ = make_attr(ATTR_DEFAULTVTABLE); }
	| tDISPLAYBIND				{ $$ = make_attr(ATTR_DISPLAYBIND); }
	| tDLLNAME '(' aSTRING ')'		{ $$ = make_attrp(ATTR_DLLNAME, $3); }
	| tDUAL					{ $$ = make_attr(ATTR_DUAL); }
	| tENDPOINT '(' str_list ')'		{ $$ = make_attrp(ATTR_ENDPOINT, $3); }
	| tENTRY '(' aSTRING ')'		{ $$ = make_attrp(ATTR_ENTRY_STRING, $3); }
	| tENTRY '(' expr_const ')'		{ $$ = make_attrp(ATTR_ENTRY_ORDINAL, $3); }
	| tEXPLICITHANDLE			{ $$ = make_attr(ATTR_EXPLICIT_HANDLE); }
	| tHANDLE				{ $$ = make_attr(ATTR_HANDLE); }
	| tHELPCONTEXT '(' expr_const ')'	{ $$ = make_attrp(ATTR_HELPCONTEXT, $3); }
	| tHELPFILE '(' aSTRING ')'		{ $$ = make_attrp(ATTR_HELPFILE, $3); }
	| tHELPSTRING '(' aSTRING ')'		{ $$ = make_attrp(ATTR_HELPSTRING, $3); }
	| tHELPSTRINGCONTEXT '(' expr_const ')'	{ $$ = make_attrp(ATTR_HELPSTRINGCONTEXT, $3); }
	| tHELPSTRINGDLL '(' aSTRING ')'	{ $$ = make_attrp(ATTR_HELPSTRINGDLL, $3); }
	| tHIDDEN				{ $$ = make_attr(ATTR_HIDDEN); }
	| tID '(' expr_const ')'		{ $$ = make_attrp(ATTR_ID, $3); }
	| tIDEMPOTENT				{ $$ = make_attr(ATTR_IDEMPOTENT); }
	| tIIDIS '(' expr ')'			{ $$ = make_attrp(ATTR_IIDIS, $3); }
	| tIMMEDIATEBIND			{ $$ = make_attr(ATTR_IMMEDIATEBIND); }
	| tIMPLICITHANDLE '(' tHANDLET aIDENTIFIER ')'	{ $$ = make_attrp(ATTR_IMPLICIT_HANDLE, $4); }
	| tIN					{ $$ = make_attr(ATTR_IN); }
	| tINPUTSYNC				{ $$ = make_attr(ATTR_INPUTSYNC); }
	| tLENGTHIS '(' m_exprs ')'		{ $$ = make_attrp(ATTR_LENGTHIS, $3); }
	| tLOCAL				{ $$ = make_attr(ATTR_LOCAL); }
	| tNONBROWSABLE				{ $$ = make_attr(ATTR_NONBROWSABLE); }
	| tNONCREATABLE				{ $$ = make_attr(ATTR_NONCREATABLE); }
	| tNONEXTENSIBLE			{ $$ = make_attr(ATTR_NONEXTENSIBLE); }
	| tOBJECT				{ $$ = make_attr(ATTR_OBJECT); }
	| tODL					{ $$ = make_attr(ATTR_ODL); }
	| tOLEAUTOMATION			{ $$ = make_attr(ATTR_OLEAUTOMATION); }
	| tOPTIONAL                             { $$ = make_attr(ATTR_OPTIONAL); }
	| tOUT					{ $$ = make_attr(ATTR_OUT); }
	| tPOINTERDEFAULT '(' pointer_type ')'	{ $$ = make_attrv(ATTR_POINTERDEFAULT, $3); }
	| tPROPGET				{ $$ = make_attr(ATTR_PROPGET); }
	| tPROPPUT				{ $$ = make_attr(ATTR_PROPPUT); }
	| tPROPPUTREF				{ $$ = make_attr(ATTR_PROPPUTREF); }
	| tPUBLIC				{ $$ = make_attr(ATTR_PUBLIC); }
	| tRANGE '(' expr_const ',' expr_const ')' { expr_list_t *list = append_expr( NULL, $3 );
                                                     list = append_expr( list, $5 );
                                                     $$ = make_attrp(ATTR_RANGE, list); }
	| tREADONLY				{ $$ = make_attr(ATTR_READONLY); }
	| tREQUESTEDIT				{ $$ = make_attr(ATTR_REQUESTEDIT); }
	| tRESTRICTED				{ $$ = make_attr(ATTR_RESTRICTED); }
	| tRETVAL				{ $$ = make_attr(ATTR_RETVAL); }
	| tSIZEIS '(' m_exprs ')'		{ $$ = make_attrp(ATTR_SIZEIS, $3); }
	| tSOURCE				{ $$ = make_attr(ATTR_SOURCE); }
	| tSTRICTCONTEXTHANDLE                  { $$ = make_attr(ATTR_STRICTCONTEXTHANDLE); }
	| tSTRING				{ $$ = make_attr(ATTR_STRING); }
	| tSWITCHIS '(' expr ')'		{ $$ = make_attrp(ATTR_SWITCHIS, $3); }
	| tSWITCHTYPE '(' type ')'		{ $$ = make_attrp(ATTR_SWITCHTYPE, $3); }
	| tTRANSMITAS '(' type ')'		{ $$ = make_attrp(ATTR_TRANSMITAS, $3); }
	| tUUID '(' uuid_string ')'		{ $$ = make_attrp(ATTR_UUID, $3); }
	| tV1ENUM				{ $$ = make_attr(ATTR_V1ENUM); }
	| tVARARG				{ $$ = make_attr(ATTR_VARARG); }
	| tVERSION '(' version ')'		{ $$ = make_attrv(ATTR_VERSION, $3); }
	| tWIREMARSHAL '(' type ')'		{ $$ = make_attrp(ATTR_WIREMARSHAL, $3); }
	| pointer_type				{ $$ = make_attrv(ATTR_POINTERTYPE, $1); }
	;

uuid_string:
	  aUUID
	| aSTRING				{ if (!is_valid_uuid($1))
						    error_loc("invalid UUID: %s\n", $1);
						  $$ = parse_uuid($1); }
        ;

callconv:
	| tSTDCALL
	;

cases:						{ $$ = NULL; }
	| cases case				{ $$ = append_var( $1, $2 ); }
	;

case:	  tCASE expr ':' field			{ attr_t *a = make_attrp(ATTR_CASE, append_expr( NULL, $2 ));
						  $$ = $4; if (!$$) $$ = make_var(NULL);
						  $$->attrs = append_attr( $$->attrs, a );
						}
	| tDEFAULT ':' field			{ attr_t *a = make_attr(ATTR_DEFAULT);
						  $$ = $3; if (!$$) $$ = make_var(NULL);
						  $$->attrs = append_attr( $$->attrs, a );
						}
	;

constdef: tCONST type ident '=' expr_const	{ $$ = reg_const($3);
						  set_type($$, $2, 0, NULL, FALSE);
						  $$->eval = $5;
						}
	;

enums:						{ $$ = NULL; }
	| enum_list ','				{ $$ = $1; }
	| enum_list
	;

enum_list: enum					{ if (!$1->eval)
						    $1->eval = make_exprl(EXPR_NUM, 0 /* default for first enum entry */);
                                                  $$ = append_var( NULL, $1 );
						}
	| enum_list ',' enum			{ if (!$3->eval)
                                                  {
                                                    var_t *last = LIST_ENTRY( list_tail($$), var_t, entry );
                                                    $3->eval = make_exprl(EXPR_NUM, last->eval->cval + 1);
                                                  }
                                                  $$ = append_var( $1, $3 );
						}
	;

enum:	  ident '=' expr_const			{ $$ = reg_const($1);
						  $$->eval = $3;
                                                  $$->type = make_int(0);
						}
	| ident					{ $$ = reg_const($1);
                                                  $$->type = make_int(0);
						}
	;

enumdef: tENUM t_ident '{' enums '}'		{ $$ = get_typev(RPC_FC_ENUM16, $2, tsENUM);
						  $$->kind = TKIND_ENUM;
						  $$->fields = $4;
						  $$->defined = TRUE;
                                                  if(in_typelib)
                                                      add_typelib_entry($$);
						}
	;

m_exprs:  m_expr                                { $$ = append_expr( NULL, $1 ); }
	| m_exprs ',' m_expr                    { $$ = append_expr( $1, $3 ); }
	;

/*
exprs:						{ $$ = make_expr(EXPR_VOID); }
	| expr_list
	;

expr_list: expr
	| expr_list ',' expr			{ LINK($3, $1); $$ = $3; }
	;
*/

m_expr:						{ $$ = make_expr(EXPR_VOID); }
	| expr
	;

expr:	  aNUM					{ $$ = make_exprl(EXPR_NUM, $1); }
	| aHEXNUM				{ $$ = make_exprl(EXPR_HEXNUM, $1); }
	| aDOUBLE				{ $$ = make_exprd(EXPR_DOUBLE, $1); }
	| tFALSE				{ $$ = make_exprl(EXPR_TRUEFALSE, 0); }
	| tTRUE					{ $$ = make_exprl(EXPR_TRUEFALSE, 1); }
	| aIDENTIFIER				{ $$ = make_exprs(EXPR_IDENTIFIER, $1); }
	| expr '?' expr ':' expr		{ $$ = make_expr3(EXPR_COND, $1, $3, $5); }
	| expr '|' expr				{ $$ = make_expr2(EXPR_OR , $1, $3); }
	| expr '&' expr				{ $$ = make_expr2(EXPR_AND, $1, $3); }
	| expr '+' expr				{ $$ = make_expr2(EXPR_ADD, $1, $3); }
	| expr '-' expr				{ $$ = make_expr2(EXPR_SUB, $1, $3); }
	| expr '*' expr				{ $$ = make_expr2(EXPR_MUL, $1, $3); }
	| expr '/' expr				{ $$ = make_expr2(EXPR_DIV, $1, $3); }
	| expr SHL expr				{ $$ = make_expr2(EXPR_SHL, $1, $3); }
	| expr SHR expr				{ $$ = make_expr2(EXPR_SHR, $1, $3); }
	| '~' expr				{ $$ = make_expr1(EXPR_NOT, $2); }
	| '-' expr %prec NEG			{ $$ = make_expr1(EXPR_NEG, $2); }
	| '&' expr %prec ADDRESSOF      { $$ = make_expr1(EXPR_ADDRESSOF, $2); }
	| '*' expr %prec PPTR			{ $$ = make_expr1(EXPR_PPTR, $2); }
	| '(' type ')' expr %prec CAST		{ $$ = make_exprt(EXPR_CAST, $2, $4); }
	| tSIZEOF '(' type ')'			{ $$ = make_exprt(EXPR_SIZEOF, $3, NULL); }
	| '(' expr ')'				{ $$ = $2; }
	;

expr_list_const: expr_const                     { $$ = append_expr( NULL, $1 ); }
	| expr_list_const ',' expr_const        { $$ = append_expr( $1, $3 ); }
	;

expr_const: expr				{ $$ = $1;
						  if (!$$->is_const)
						      error_loc("expression is not constant\n");
						}
	;

externdef: tEXTERN tCONST type ident		{ $$ = $4;
						  set_type($$, $3, 0, NULL, FALSE);
						}
	;

fields:						{ $$ = NULL; }
	| fields field				{ $$ = append_var( $1, $2 ); }
	;

field:	  s_field ';'				{ $$ = $1; }
	| m_attributes uniondef ';'		{ $$ = make_var(NULL); $$->type = $2; $$->attrs = $1; }
	| attributes ';'			{ $$ = make_var(NULL); $$->attrs = $1; }
	| ';'					{ $$ = NULL; }
	;

s_field:  m_attributes type pident array	{ $$ = $3->var;
						  $$->attrs = $1;
						  set_type($$, $2, $3->ptr_level, $4, FALSE);
						  free($3);
						}
	;

funcdef:
	  m_attributes type callconv pident
	  '(' m_args ')'			{ var_t *v = $4->var;
						  v->attrs = $1;
						  set_type(v, $2, $4->ptr_level, NULL, FALSE);
						  free($4);
						  $$ = make_func(v, $6);
						  if (is_attr(v->attrs, ATTR_IN)) {
						    error_loc("inapplicable attribute [in] for function '%s'\n",$$->def->name);
						  }
						}
	;

m_ident:					{ $$ = NULL; }
	| ident
	;

t_ident:					{ $$ = NULL; }
	| aIDENTIFIER				{ $$ = make_var($1); }
	| aKNOWNTYPE				{ $$ = make_var($1); }
	;

ident:	  aIDENTIFIER				{ $$ = make_var($1); }
/* some "reserved words" used in attributes are also used as field names in some MS IDL files */
	| aKNOWNTYPE				{ $$ = make_var($<str>1); }
	;

base_type: tBYTE				{ $$ = make_builtin($<str>1); }
	| tWCHAR				{ $$ = make_builtin($<str>1); }
	| int_std
	| tSIGNED int_std			{ $$ = $2; $$->sign = 1; }
	| tUNSIGNED int_std			{ $$ = $2; $$->sign = -1;
						  switch ($$->type) {
						  case RPC_FC_CHAR:  break;
						  case RPC_FC_SMALL: $$->type = RPC_FC_USMALL; break;
						  case RPC_FC_SHORT: $$->type = RPC_FC_USHORT; break;
						  case RPC_FC_LONG:  $$->type = RPC_FC_ULONG;  break;
						  case RPC_FC_HYPER:
						    if ($$->name[0] == 'h') /* hyper, as opposed to __int64 */
                                                    {
                                                      $$ = alias($$, "MIDL_uhyper");
                                                      $$->sign = 0;
                                                    }
						    break;
						  default: break;
						  }
						}
	| tUNSIGNED				{ $$ = make_int(-1); }
	| tFLOAT				{ $$ = make_builtin($<str>1); }
	| tSINGLE				{ $$ = duptype(find_type("float", 0), 1); }
	| tDOUBLE				{ $$ = make_builtin($<str>1); }
	| tBOOLEAN				{ $$ = make_builtin($<str>1); }
	| tERRORSTATUST				{ $$ = make_builtin($<str>1); }
	| tHANDLET				{ $$ = make_builtin($<str>1); }
	;

m_int:
	| tINT
	;

int_std:  tINT					{ $$ = make_builtin($<str>1); }
	| tSHORT m_int				{ $$ = make_builtin($<str>1); }
	| tSMALL				{ $$ = make_builtin($<str>1); }
	| tLONG m_int				{ $$ = make_builtin($<str>1); }
	| tHYPER m_int				{ $$ = make_builtin($<str>1); }
	| tINT64				{ $$ = make_builtin($<str>1); }
	| tCHAR					{ $$ = make_builtin($<str>1); }
	;

coclass:  tCOCLASS aIDENTIFIER			{ $$ = make_class($2); }
	| tCOCLASS aKNOWNTYPE			{ $$ = find_type($2, 0);
						  if ($$->defined) error_loc("multiple definition error\n");
						  if ($$->kind != TKIND_COCLASS) error_loc("%s was not declared a coclass\n", $2);
						}
	;

coclasshdr: attributes coclass			{ $$ = $2;
						  $$->attrs = $1;
						  if (!parse_only && do_header)
						    write_coclass($$);
						  if (!parse_only && do_idfile)
						    write_clsid($$);
						}
	;

coclassdef: coclasshdr '{' coclass_ints '}'	{ $$ = $1;
						  $$->ifaces = $3;
						  $$->defined = TRUE;
						}
	;

coclass_ints:					{ $$ = NULL; }
	| coclass_ints coclass_int		{ $$ = append_ifref( $1, $2 ); }
	;

coclass_int:
	  m_attributes interfacedec		{ $$ = make_ifref($2); $$->attrs = $1; }
	;

dispinterface: tDISPINTERFACE aIDENTIFIER	{ $$ = get_type(0, $2, 0); $$->kind = TKIND_DISPATCH; }
	|      tDISPINTERFACE aKNOWNTYPE	{ $$ = get_type(0, $2, 0); $$->kind = TKIND_DISPATCH; }
	;

dispinterfacehdr: attributes dispinterface	{ attr_t *attrs;
						  $$ = $2;
						  if ($$->defined) error_loc("multiple definition error\n");
						  attrs = make_attr(ATTR_DISPINTERFACE);
						  $$->attrs = append_attr( $1, attrs );
						  $$->ref = find_type("IDispatch", 0);
						  if (!$$->ref) error_loc("IDispatch is undefined\n");
						  $$->defined = TRUE;
						  if (!parse_only && do_header) write_forward($$);
						}
	;

dispint_props: tPROPERTIES ':'			{ $$ = NULL; }
	| dispint_props s_field ';'		{ $$ = append_var( $1, $2 ); }
	;

dispint_meths: tMETHODS ':'			{ $$ = NULL; }
	| dispint_meths funcdef ';'		{ $$ = append_func( $1, $2 ); }
	;

dispinterfacedef: dispinterfacehdr '{'
	  dispint_props
	  dispint_meths
	  '}'					{ $$ = $1;
						  $$->fields = $3;
						  $$->funcs = $4;
						  if (!parse_only && do_header) write_dispinterface($$);
						  if (!parse_only && do_idfile) write_diid($$);
						}
	| dispinterfacehdr
	 '{' interface ';' '}'			{ $$ = $1;
						  $$->fields = $3->fields;
						  $$->funcs = $3->funcs;
						  if (!parse_only && do_header) write_dispinterface($$);
						  if (!parse_only && do_idfile) write_diid($$);
						}
	;

inherit:					{ $$ = NULL; }
	| ':' aKNOWNTYPE			{ $$ = find_type2($2, 0); }
	;

interface: tINTERFACE aIDENTIFIER		{ $$ = get_type(RPC_FC_IP, $2, 0); $$->kind = TKIND_INTERFACE; }
	|  tINTERFACE aKNOWNTYPE		{ $$ = get_type(RPC_FC_IP, $2, 0); $$->kind = TKIND_INTERFACE; }
	;

interfacehdr: attributes interface		{ $$.interface = $2;
						  $$.old_pointer_default = pointer_default;
						  if (is_attr($1, ATTR_POINTERDEFAULT))
						    pointer_default = get_attrv($1, ATTR_POINTERDEFAULT);
						  if ($2->defined) error_loc("multiple definition error\n");
						  $2->attrs = $1;
						  $2->defined = TRUE;
						  if (!parse_only && do_header) write_forward($2);
						}
	;

interfacedef: interfacehdr inherit
	  '{' int_statements '}'		{ $$ = $1.interface;
						  $$->ref = $2;
						  $$->funcs = $4;
						  compute_method_indexes($$);
						  if (!parse_only && do_header) write_interface($$);
						  if (!parse_only && local_stubs) write_locals(local_stubs, $$, TRUE);
						  if (!parse_only && do_idfile) write_iid($$);
						  pointer_default = $1.old_pointer_default;
						}
/* MIDL is able to import the definition of a base class from inside the
 * definition of a derived class, I'll try to support it with this rule */
	| interfacehdr ':' aIDENTIFIER
	  '{' import int_statements '}'		{ $$ = $1.interface;
						  $$->ref = find_type2($3, 0);
						  if (!$$->ref) error_loc("base class '%s' not found in import\n", $3);
						  $$->funcs = $6;
						  compute_method_indexes($$);
						  if (!parse_only && do_header) write_interface($$);
						  if (!parse_only && local_stubs) write_locals(local_stubs, $$, TRUE);
						  if (!parse_only && do_idfile) write_iid($$);
						  pointer_default = $1.old_pointer_default;
						}
	| dispinterfacedef			{ $$ = $1; }
	;

interfacedec:
	  interface ';'				{ $$ = $1; if (!parse_only && do_header) write_forward($$); }
	| dispinterface ';'			{ $$ = $1; if (!parse_only && do_header) write_forward($$); }
	;

module:   tMODULE aIDENTIFIER			{ $$ = make_type(0, NULL); $$->name = $2; $$->kind = TKIND_MODULE; }
	| tMODULE aKNOWNTYPE			{ $$ = make_type(0, NULL); $$->name = $2; $$->kind = TKIND_MODULE; }
	;

modulehdr: attributes module			{ $$ = $2;
						  $$->attrs = $1;
						}
	;

moduledef: modulehdr '{' int_statements '}'	{ $$ = $1;
						  $$->funcs = $3;
						  /* FIXME: if (!parse_only && do_header) write_module($$); */
						}
	;

p_ident:  '*' pident %prec PPTR			{ $$ = $2; $$->ptr_level++; }
	| tCONST p_ident			{ $$ = $2; /* FIXME */ }
	;

pident:	  ident					{ $$ = make_pident($1); }
	| p_ident
	| '(' pident ')'			{ $$ = $2; }
	;

pident_list:
	pident                                  { $$ = append_pident( NULL, $1 ); }
	| pident_list ',' pident                { $$ = append_pident( $1, $3 ); }
	;

pointer_type:
	  tREF					{ $$ = RPC_FC_RP; }
	| tUNIQUE				{ $$ = RPC_FC_UP; }
	| tPTR					{ $$ = RPC_FC_FP; }
	;

structdef: tSTRUCT t_ident '{' fields '}'	{ $$ = get_typev(RPC_FC_STRUCT, $2, tsSTRUCT);
                                                  /* overwrite RPC_FC_STRUCT with a more exact type */
						  $$->type = get_struct_type( $4 );
						  $$->kind = TKIND_RECORD;
						  $$->fields = $4;
						  $$->defined = TRUE;
                                                  if(in_typelib)
                                                      add_typelib_entry($$);
                                                }
	;

type:	  tVOID					{ $$ = duptype(find_type("void", 0), 1); }
	| aKNOWNTYPE				{ $$ = find_type($1, 0); }
	| base_type				{ $$ = $1; }
	| tCONST type				{ $$ = duptype($2, 1); $$->is_const = TRUE; }
	| enumdef				{ $$ = $1; }
	| tENUM aIDENTIFIER			{ $$ = find_type2($2, tsENUM); }
	| structdef				{ $$ = $1; }
	| tSTRUCT aIDENTIFIER			{ $$ = get_type(RPC_FC_STRUCT, $2, tsSTRUCT); }
	| uniondef				{ $$ = $1; }
	| tUNION aIDENTIFIER			{ $$ = find_type2($2, tsUNION); }
	| tSAFEARRAY '(' type ')'		{ $$ = make_safearray($3); }
	;

typedef: tTYPEDEF m_attributes type pident_list	{ reg_typedefs($3, $4, $2);
						  process_typedefs($4);
						}
	;

uniondef: tUNION t_ident '{' fields '}'		{ $$ = get_typev(RPC_FC_NON_ENCAPSULATED_UNION, $2, tsUNION);
						  $$->kind = TKIND_UNION;
						  $$->fields = $4;
						  $$->defined = TRUE;
						}
	| tUNION t_ident
	  tSWITCH '(' s_field ')'
	  m_ident '{' cases '}'			{ var_t *u = $7;
						  $$ = get_typev(RPC_FC_ENCAPSULATED_UNION, $2, tsUNION);
						  $$->kind = TKIND_UNION;
						  if (!u) u = make_var( xstrdup("tagged_union") );
						  u->type = make_type(RPC_FC_NON_ENCAPSULATED_UNION, NULL);
						  u->type->kind = TKIND_UNION;
						  u->type->fields = $9;
						  u->type->defined = TRUE;
						  $$->fields = append_var( $$->fields, $5 );
						  $$->fields = append_var( $$->fields, u );
						  $$->defined = TRUE;
						}
	;

version:
	  aNUM					{ $$ = MAKEVERSION($1, 0); }
	| aNUM '.' aNUM				{ $$ = MAKEVERSION($1, $3); }
	;

%%

static void decl_builtin(const char *name, unsigned char type)
{
  type_t *t = make_type(type, NULL);
  t->name = xstrdup(name);
  reg_type(t, name, 0);
}

static type_t *make_builtin(char *name)
{
  /* NAME is strdup'd in the lexer */
  type_t *t = duptype(find_type(name, 0), 0);
  t->name = name;
  return t;
}

static type_t *make_int(int sign)
{
  type_t *t = duptype(find_type("int", 0), 1);

  t->sign = sign;
  if (sign < 0)
    t->type = t->type == RPC_FC_LONG ? RPC_FC_ULONG : RPC_FC_USHORT;

  return t;
}

void init_types(void)
{
  decl_builtin("void", 0);
  decl_builtin("byte", RPC_FC_BYTE);
  decl_builtin("wchar_t", RPC_FC_WCHAR);
  decl_builtin("int", RPC_FC_LONG);     /* win32 */
  decl_builtin("short", RPC_FC_SHORT);
  decl_builtin("small", RPC_FC_SMALL);
  decl_builtin("long", RPC_FC_LONG);
  decl_builtin("hyper", RPC_FC_HYPER);
  decl_builtin("__int64", RPC_FC_HYPER);
  decl_builtin("char", RPC_FC_CHAR);
  decl_builtin("float", RPC_FC_FLOAT);
  decl_builtin("double", RPC_FC_DOUBLE);
  decl_builtin("boolean", RPC_FC_BYTE);
  decl_builtin("error_status_t", RPC_FC_ERROR_STATUS_T);
  decl_builtin("handle_t", RPC_FC_BIND_PRIMITIVE);
}

static str_list_t *append_str(str_list_t *list, char *str)
{
    struct str_list_entry_t *entry;

    if (!str) return list;
    if (!list)
    {
        list = xmalloc( sizeof(*list) );
        list_init( list );
    }
    entry = xmalloc( sizeof(*entry) );
    entry->str = str;
    list_add_tail( list, &entry->entry );
    return list;
}

static attr_list_t *append_attr(attr_list_t *list, attr_t *attr)
{
    if (!attr) return list;
    if (!list)
    {
        list = xmalloc( sizeof(*list) );
        list_init( list );
    }
    list_add_tail( list, &attr->entry );
    return list;
}

static attr_t *make_attr(enum attr_type type)
{
  attr_t *a = xmalloc(sizeof(attr_t));
  a->type = type;
  a->u.ival = 0;
  return a;
}

static attr_t *make_attrv(enum attr_type type, unsigned long val)
{
  attr_t *a = xmalloc(sizeof(attr_t));
  a->type = type;
  a->u.ival = val;
  return a;
}

static attr_t *make_attrp(enum attr_type type, void *val)
{
  attr_t *a = xmalloc(sizeof(attr_t));
  a->type = type;
  a->u.pval = val;
  return a;
}

static expr_t *make_expr(enum expr_type type)
{
  expr_t *e = xmalloc(sizeof(expr_t));
  e->type = type;
  e->ref = NULL;
  e->u.lval = 0;
  e->is_const = FALSE;
  e->cval = 0;
  return e;
}

static expr_t *make_exprl(enum expr_type type, long val)
{
  expr_t *e = xmalloc(sizeof(expr_t));
  e->type = type;
  e->ref = NULL;
  e->u.lval = val;
  e->is_const = FALSE;
  /* check for numeric constant */
  if (type == EXPR_NUM || type == EXPR_HEXNUM || type == EXPR_TRUEFALSE) {
    /* make sure true/false value is valid */
    assert(type != EXPR_TRUEFALSE || val == 0 || val == 1);
    e->is_const = TRUE;
    e->cval = val;
  }
  return e;
}

static expr_t *make_exprd(enum expr_type type, double val)
{
  expr_t *e = xmalloc(sizeof(expr_t));
  e->type = type;
  e->ref = NULL;
  e->u.dval = val;
  e->is_const = TRUE;
  e->cval = val;
  return e;
}

static expr_t *make_exprs(enum expr_type type, char *val)
{
  expr_t *e;
  e = xmalloc(sizeof(expr_t));
  e->type = type;
  e->ref = NULL;
  e->u.sval = val;
  e->is_const = FALSE;
  /* check for predefined constants */
  if (type == EXPR_IDENTIFIER) {
    var_t *c = find_const(val, 0);
    if (c) {
      e->u.sval = c->name;
      free(val);
      e->is_const = TRUE;
      e->cval = c->eval->cval;
    }
  }
  return e;
}

static expr_t *make_exprt(enum expr_type type, type_t *tref, expr_t *expr)
{
  expr_t *e;
  e = xmalloc(sizeof(expr_t));
  e->type = type;
  e->ref = expr;
  e->u.tref = tref;
  e->is_const = FALSE;
  /* check for cast of constant expression */
  if (type == EXPR_SIZEOF) {
    switch (tref->type) {
      case RPC_FC_BYTE:
      case RPC_FC_CHAR:
      case RPC_FC_SMALL:
      case RPC_FC_USMALL:
        e->is_const = TRUE;
        e->cval = 1;
        break;
      case RPC_FC_WCHAR:
      case RPC_FC_USHORT:
      case RPC_FC_SHORT:
        e->is_const = TRUE;
        e->cval = 2;
        break;
      case RPC_FC_LONG:
      case RPC_FC_ULONG:
      case RPC_FC_FLOAT:
      case RPC_FC_ERROR_STATUS_T:
        e->is_const = TRUE;
        e->cval = 4;
        break;
      case RPC_FC_HYPER:
      case RPC_FC_DOUBLE:
        e->is_const = TRUE;
        e->cval = 8;
        break;
    }
  }
  if (type == EXPR_CAST && expr->is_const) {
    e->is_const = TRUE;
    e->cval = expr->cval;
  }
  return e;
}

static expr_t *make_expr1(enum expr_type type, expr_t *expr)
{
  expr_t *e;
  if (type == EXPR_ADDRESSOF && expr->type != EXPR_IDENTIFIER)
    error("address-of operator applied to invalid expression\n");
  e = xmalloc(sizeof(expr_t));
  e->type = type;
  e->ref = expr;
  e->u.lval = 0;
  e->is_const = FALSE;
  /* check for compile-time optimization */
  if (expr->is_const) {
    e->is_const = TRUE;
    switch (type) {
    case EXPR_NEG:
      e->cval = -expr->cval;
      break;
    case EXPR_NOT:
      e->cval = ~expr->cval;
      break;
    default:
      e->is_const = FALSE;
      break;
    }
  }
  return e;
}

static expr_t *make_expr2(enum expr_type type, expr_t *expr1, expr_t *expr2)
{
  expr_t *e;
  e = xmalloc(sizeof(expr_t));
  e->type = type;
  e->ref = expr1;
  e->u.ext = expr2;
  e->is_const = FALSE;
  /* check for compile-time optimization */
  if (expr1->is_const && expr2->is_const) {
    e->is_const = TRUE;
    switch (type) {
    case EXPR_ADD:
      e->cval = expr1->cval + expr2->cval;
      break;
    case EXPR_SUB:
      e->cval = expr1->cval - expr2->cval;
      break;
    case EXPR_MUL:
      e->cval = expr1->cval * expr2->cval;
      break;
    case EXPR_DIV:
      e->cval = expr1->cval / expr2->cval;
      break;
    case EXPR_OR:
      e->cval = expr1->cval | expr2->cval;
      break;
    case EXPR_AND:
      e->cval = expr1->cval & expr2->cval;
      break;
    case EXPR_SHL:
      e->cval = expr1->cval << expr2->cval;
      break;
    case EXPR_SHR:
      e->cval = expr1->cval >> expr2->cval;
      break;
    default:
      e->is_const = FALSE;
      break;
    }
  }
  return e;
}

static expr_t *make_expr3(enum expr_type type, expr_t *expr1, expr_t *expr2, expr_t *expr3)
{
  expr_t *e;
  e = xmalloc(sizeof(expr_t));
  e->type = type;
  e->ref = expr1;
  e->u.ext = expr2;
  e->ext2 = expr3;
  e->is_const = FALSE;
  /* check for compile-time optimization */
  if (expr1->is_const && expr2->is_const && expr3->is_const) {
    e->is_const = TRUE;
    switch (type) {
    case EXPR_COND:
      e->cval = expr1->cval ? expr2->cval : expr3->cval;
      break;
    default:
      e->is_const = FALSE;
      break;
    }
  }
  return e;
}

static expr_list_t *append_expr(expr_list_t *list, expr_t *expr)
{
    if (!expr) return list;
    if (!list)
    {
        list = xmalloc( sizeof(*list) );
        list_init( list );
    }
    list_add_tail( list, &expr->entry );
    return list;
}

static array_dims_t *append_array(array_dims_t *list, expr_t *expr)
{
    if (!expr) return list;
    if (!list)
    {
        list = xmalloc( sizeof(*list) );
        list_init( list );
    }
    list_add_tail( list, &expr->entry );
    return list;
}

static struct list type_pool = LIST_INIT(type_pool);
typedef struct
{
  type_t data;
  struct list link;
} type_pool_node_t;

type_t *alloc_type(void)
{
  type_pool_node_t *node = xmalloc(sizeof *node);
  list_add_tail(&type_pool, &node->link);
  return &node->data;
}

void set_all_tfswrite(int val)
{
  type_pool_node_t *node;
  LIST_FOR_EACH_ENTRY(node, &type_pool, type_pool_node_t, link)
    node->data.tfswrite = val;
}

static type_t *make_type(unsigned char type, type_t *ref)
{
  type_t *t = alloc_type();
  t->name = NULL;
  t->kind = TKIND_PRIMITIVE;
  t->type = type;
  t->ref = ref;
  t->attrs = NULL;
  t->orig = NULL;
  t->funcs = NULL;
  t->fields = NULL;
  t->ifaces = NULL;
  t->dim = 0;
  t->size_is = NULL;
  t->length_is = NULL;
  t->typestring_offset = 0;
  t->ptrdesc = 0;
  t->declarray = FALSE;
  t->ignore = (parse_only != 0);
  t->is_const = FALSE;
  t->sign = 0;
  t->defined = FALSE;
  t->written = FALSE;
  t->user_types_registered = FALSE;
  t->tfswrite = FALSE;
  t->typelib_idx = -1;
  return t;
}

static void set_type(var_t *v, type_t *type, int ptr_level, array_dims_t *arr,
                     int top)
{
  expr_list_t *sizes = get_attrp(v->attrs, ATTR_SIZEIS);
  expr_list_t *lengs = get_attrp(v->attrs, ATTR_LENGTHIS);
  int ptr_attr = get_attrv(v->attrs, ATTR_POINTERTYPE);
  int ptr_type = ptr_attr;
  int sizeless, has_varconf;
  expr_t *dim;
  type_t *atype, **ptype;

  v->type = type;

  if (!ptr_type && top)
    ptr_type = RPC_FC_RP;

  for ( ; 0 < ptr_level; --ptr_level)
  {
    v->type = make_type(pointer_default, v->type);
    if (ptr_level == 1 && ptr_type && !arr)
    {
      v->type->type = ptr_type;
      ptr_type = 0;
    }
  }

  if (ptr_type && !arr)
  {
    if (is_ptr(v->type))
    {
      if (v->type->type != ptr_type)
      {
        v->type = duptype(v->type, 1);
        v->type->type = ptr_type;
      }
    }
    else if (!arr && ptr_attr)
      error("%s: pointer attribute applied to non-pointer type\n", v->name);
  }

  sizeless = FALSE;
  if (arr) LIST_FOR_EACH_ENTRY_REV(dim, arr, expr_t, entry)
  {
    if (sizeless)
      error("%s: only the first array dimension can be unspecified\n", v->name);

    if (dim->is_const)
    {
      unsigned int align = 0;
      size_t size = type_memsize(v->type, &align);

      if (dim->cval <= 0)
        error("%s: array dimension must be positive\n", v->name);

      if (0xffffffffuL / size < (unsigned long) dim->cval)
        error("%s: total array size is too large\n", v->name);
      else if (0xffffuL < size * dim->cval)
        v->type = make_type(RPC_FC_LGFARRAY, v->type);
      else
        v->type = make_type(RPC_FC_SMFARRAY, v->type);
    }
    else
    {
      sizeless = TRUE;
      v->type = make_type(RPC_FC_CARRAY, v->type);
    }

    v->type->declarray = TRUE;
    v->type->dim = dim->cval;
  }

  ptype = &v->type;
  has_varconf = FALSE;
  if (sizes) LIST_FOR_EACH_ENTRY(dim, sizes, expr_t, entry)
  {
    if (dim->type != EXPR_VOID)
    {
      has_varconf = TRUE;
      atype = *ptype = duptype(*ptype, 0);

      if (atype->type == RPC_FC_SMFARRAY || atype->type == RPC_FC_LGFARRAY)
        error("%s: cannot specify size_is for a fixed sized array\n", v->name);

      if (atype->type != RPC_FC_CARRAY && !is_ptr(atype))
        error("%s: size_is attribute applied to illegal type\n", v->name);

      atype->type = RPC_FC_CARRAY;
      atype->size_is = dim;
    }

    ptype = &(*ptype)->ref;
    if (*ptype == NULL)
      error("%s: too many expressions in size_is attribute\n", v->name);
  }

  ptype = &v->type;
  if (lengs) LIST_FOR_EACH_ENTRY(dim, lengs, expr_t, entry)
  {
    if (dim->type != EXPR_VOID)
    {
      has_varconf = TRUE;
      atype = *ptype = duptype(*ptype, 0);

      if (atype->type == RPC_FC_SMFARRAY)
        atype->type = RPC_FC_SMVARRAY;
      else if (atype->type == RPC_FC_LGFARRAY)
        atype->type = RPC_FC_LGVARRAY;
      else if (atype->type == RPC_FC_CARRAY)
        atype->type = RPC_FC_CVARRAY;
      else
        error("%s: length_is attribute applied to illegal type\n", v->name);

      atype->length_is = dim;
    }

    ptype = &(*ptype)->ref;
    if (*ptype == NULL)
      error("%s: too many expressions in length_is attribute\n", v->name);
  }

  if (has_varconf && !last_array(v->type))
  {
    ptype = &v->type;
    for (ptype = &v->type; is_array(*ptype); ptype = &(*ptype)->ref)
    {
      *ptype = duptype(*ptype, 0);
      (*ptype)->type = RPC_FC_BOGUS_ARRAY;
    }
  }

  if (is_array(v->type))
  {
    const type_t *rt = v->type->ref;
    if (is_user_type(rt))
      v->type->type = RPC_FC_BOGUS_ARRAY;
    else
      switch (rt->type)
        {
        case RPC_FC_BOGUS_STRUCT:
        case RPC_FC_NON_ENCAPSULATED_UNION:
        case RPC_FC_ENCAPSULATED_UNION:
        case RPC_FC_ENUM16:
          v->type->type = RPC_FC_BOGUS_ARRAY;
          break;
          /* FC_RP should be above, but widl overuses these, and will break things.  */
        case RPC_FC_UP:
        case RPC_FC_RP:
          if (rt->ref->type == RPC_FC_IP)
            v->type->type = RPC_FC_BOGUS_ARRAY;
          break;
        }
  }
}

static ifref_list_t *append_ifref(ifref_list_t *list, ifref_t *iface)
{
    if (!iface) return list;
    if (!list)
    {
        list = xmalloc( sizeof(*list) );
        list_init( list );
    }
    list_add_tail( list, &iface->entry );
    return list;
}

static ifref_t *make_ifref(type_t *iface)
{
  ifref_t *l = xmalloc(sizeof(ifref_t));
  l->iface = iface;
  l->attrs = NULL;
  return l;
}

static var_list_t *append_var(var_list_t *list, var_t *var)
{
    if (!var) return list;
    if (!list)
    {
        list = xmalloc( sizeof(*list) );
        list_init( list );
    }
    list_add_tail( list, &var->entry );
    return list;
}

static var_t *make_var(char *name)
{
  var_t *v = xmalloc(sizeof(var_t));
  v->name = name;
  v->type = NULL;
  v->args = NULL;
  v->attrs = NULL;
  v->eval = NULL;
  return v;
}

static pident_list_t *append_pident(pident_list_t *list, pident_t *p)
{
  if (!p) return list;
  if (!list) {
    list = xmalloc(sizeof(*list));
    list_init(list);
  }
  list_add_tail(list, &p->entry);
  return list;
}

static pident_t *make_pident(var_t *var)
{
  pident_t *p = xmalloc(sizeof(*p));
  p->var = var;
  p->ptr_level = 0;
  return p;
}

static func_list_t *append_func(func_list_t *list, func_t *func)
{
    if (!func) return list;
    if (!list)
    {
        list = xmalloc( sizeof(*list) );
        list_init( list );
    }
    list_add_tail( list, &func->entry );
    return list;
}

static func_t *make_func(var_t *def, var_list_t *args)
{
  func_t *f = xmalloc(sizeof(func_t));
  f->def = def;
  f->args = args;
  f->ignore = parse_only;
  f->idx = -1;
  return f;
}

static type_t *make_class(char *name)
{
  type_t *c = make_type(0, NULL);
  c->name = name;
  c->kind = TKIND_COCLASS;
  return c;
}

static type_t *make_safearray(type_t *type)
{
  type_t *sa = duptype(find_type("SAFEARRAY", 0), 1);
  sa->ref = type;
  return make_type(pointer_default, sa);
}

#define HASHMAX 64

static int hash_ident(const char *name)
{
  const char *p = name;
  int sum = 0;
  /* a simple sum hash is probably good enough */
  while (*p) {
    sum += *p;
    p++;
  }
  return sum & (HASHMAX-1);
}

/***** type repository *****/

struct rtype {
  const char *name;
  type_t *type;
  int t;
  struct rtype *next;
};

struct rtype *type_hash[HASHMAX];

static type_t *reg_type(type_t *type, const char *name, int t)
{
  struct rtype *nt;
  int hash;
  if (!name) {
    error_loc("registering named type without name\n");
    return type;
  }
  hash = hash_ident(name);
  nt = xmalloc(sizeof(struct rtype));
  nt->name = name;
  nt->type = type;
  nt->t = t;
  nt->next = type_hash[hash];
  type_hash[hash] = nt;
  return type;
}

static int is_incomplete(const type_t *t)
{
  return !t->defined && (is_struct(t->type) || is_union(t->type));
}

static void add_incomplete(type_t *t)
{
  struct typenode *tn = xmalloc(sizeof *tn);
  tn->type = t;
  list_add_tail(&incomplete_types, &tn->entry);
}

static void fix_type(type_t *t)
{
  if (t->kind == TKIND_ALIAS && is_incomplete(t)) {
    type_t *ot = t->orig;
    fix_type(ot);
    t->fields = ot->fields;
    t->defined = ot->defined;
  }
}

static void fix_incomplete(void)
{
  struct typenode *tn, *next;

  LIST_FOR_EACH_ENTRY_SAFE(tn, next, &incomplete_types, struct typenode, entry) {
    fix_type(tn->type);
    free(tn);
  }
}

static type_t *reg_typedefs(type_t *type, pident_list_t *pidents, attr_list_t *attrs)
{
  type_t *ptr = type;
  const pident_t *pident;
  int ptrc = 0;
  int is_str = is_attr(attrs, ATTR_STRING);
  unsigned char ptr_type = get_attrv(attrs, ATTR_POINTERTYPE);

  if (is_str)
  {
    type_t *t = type;
    unsigned char c;

    while (is_ptr(t))
      t = t->ref;

    c = t->type;
    if (c != RPC_FC_CHAR && c != RPC_FC_BYTE && c != RPC_FC_WCHAR)
    {
      pident = LIST_ENTRY( list_head( pidents ), const pident_t, entry );
      error_loc("'%s': [string] attribute is only valid on 'char', 'byte', or 'wchar_t' pointers and arrays\n",
              pident->var->name);
    }
  }

  /* We must generate names for tagless enum, struct or union.
     Typedef-ing a tagless enum, struct or union means we want the typedef
     to be included in a library whether it has other attributes or not,
     hence the public attribute.  */
  if ((type->kind == TKIND_ENUM || type->kind == TKIND_RECORD
       || type->kind == TKIND_UNION) && ! type->name && ! parse_only)
  {
    if (! is_attr(attrs, ATTR_PUBLIC))
      attrs = append_attr( attrs, make_attr(ATTR_PUBLIC) );
    type->name = gen_name();
  }

  LIST_FOR_EACH_ENTRY( pident, pidents, const pident_t, entry )
  {
    var_t *name = pident->var;

    if (name->name) {
      type_t *cur = ptr;
      int cptr = pident->ptr_level;
      if (cptr > ptrc) {
        while (cptr > ptrc) {
          cur = ptr = make_type(pointer_default, cur);
          ptrc++;
        }
      } else {
        while (cptr < ptrc) {
          cur = cur->ref;
          cptr++;
        }
      }
      cur = alias(cur, name->name);
      cur->attrs = attrs;
      if (ptr_type)
      {
        if (is_ptr(cur))
          cur->type = ptr_type;
        else
          error_loc("'%s': pointer attribute applied to non-pointer type\n",
                  cur->name);
      }
      else if (is_str && ! is_ptr(cur))
        error_loc("'%s': [string] attribute applied to non-pointer type\n",
                cur->name);

      if (is_incomplete(cur))
        add_incomplete(cur);
      reg_type(cur, cur->name, 0);
    }
  }
  return type;
}

static type_t *find_type(const char *name, int t)
{
  struct rtype *cur = type_hash[hash_ident(name)];
  while (cur && (cur->t != t || strcmp(cur->name, name)))
    cur = cur->next;
  if (!cur) {
    error_loc("type '%s' not found\n", name);
    return NULL;
  }
  return cur->type;
}

static type_t *find_type2(char *name, int t)
{
  type_t *tp = find_type(name, t);
  free(name);
  return tp;
}

int is_type(const char *name)
{
  struct rtype *cur = type_hash[hash_ident(name)];
  while (cur && (cur->t || strcmp(cur->name, name)))
    cur = cur->next;
  if (cur) return TRUE;
  return FALSE;
}

static type_t *get_type(unsigned char type, char *name, int t)
{
  struct rtype *cur = NULL;
  type_t *tp;
  if (name) {
    cur = type_hash[hash_ident(name)];
    while (cur && (cur->t != t || strcmp(cur->name, name)))
      cur = cur->next;
  }
  if (cur) {
    free(name);
    return cur->type;
  }
  tp = make_type(type, NULL);
  tp->name = name;
  if (!name) return tp;
  return reg_type(tp, name, t);
}

static type_t *get_typev(unsigned char type, var_t *name, int t)
{
  char *sname = NULL;
  if (name) {
    sname = name->name;
    free(name);
  }
  return get_type(type, sname, t);
}

static int get_struct_type(var_list_t *fields)
{
  int has_pointer = 0;
  int has_conformance = 0;
  int has_variance = 0;
  var_t *field;

  if (get_padding(fields))
    return RPC_FC_BOGUS_STRUCT;

  if (fields) LIST_FOR_EACH_ENTRY( field, fields, var_t, entry )
  {
    type_t *t = field->type;

    if (is_user_type(t))
      return RPC_FC_BOGUS_STRUCT;

    if (is_ptr(t))
    {
        do
            t = t->ref;
        while (is_ptr(t));

        switch (t->type)
        {
        case RPC_FC_IP:
        case RPC_FC_ENCAPSULATED_UNION:
        case RPC_FC_NON_ENCAPSULATED_UNION:
        case RPC_FC_BOGUS_STRUCT:
            return RPC_FC_BOGUS_STRUCT;
        }

        has_pointer = 1;
        continue;
    }

    if (field->type->declarray)
    {
        if (is_string_type(field->attrs, field->type))
        {
            if (is_conformant_array(field->type))
                has_conformance = 1;
            has_variance = 1;
            continue;
        }

        if (is_array(field->type->ref))
            return RPC_FC_BOGUS_STRUCT;

        if (is_conformant_array(field->type))
        {
            has_conformance = 1;
            if (field->type->declarray && list_next(fields, &field->entry))
                error_loc("field '%s' deriving from a conformant array must be the last field in the structure\n",
                        field->name);
        }
        if (field->type->length_is)
            has_variance = 1;

        t = field->type->ref;
    }

    switch (t->type)
    {
    /*
     * RPC_FC_BYTE, RPC_FC_STRUCT, etc
     *  Simple types don't effect the type of struct.
     *  A struct containing a simple struct is still a simple struct.
     *  So long as we can block copy the data, we return RPC_FC_STRUCT.
     */
    case 0: /* void pointer */
    case RPC_FC_BYTE:
    case RPC_FC_CHAR:
    case RPC_FC_SMALL:
    case RPC_FC_USMALL:
    case RPC_FC_WCHAR:
    case RPC_FC_SHORT:
    case RPC_FC_USHORT:
    case RPC_FC_LONG:
    case RPC_FC_ULONG:
    case RPC_FC_INT3264:
    case RPC_FC_UINT3264:
    case RPC_FC_HYPER:
    case RPC_FC_FLOAT:
    case RPC_FC_DOUBLE:
    case RPC_FC_STRUCT:
    case RPC_FC_ENUM32:
      break;

    case RPC_FC_RP:
    case RPC_FC_UP:
    case RPC_FC_FP:
    case RPC_FC_OP:
    case RPC_FC_CARRAY:
    case RPC_FC_CVARRAY:
    case RPC_FC_BOGUS_ARRAY:
      has_pointer = 1;
      break;

    /*
     * Propagate member attributes
     *  a struct should be at least as complex as its member
     */
    case RPC_FC_CVSTRUCT:
      has_conformance = 1;
      has_variance = 1;
      has_pointer = 1;
      break;

    case RPC_FC_CPSTRUCT:
      has_conformance = 1;
      if (list_next( fields, &field->entry ))
          error_loc("field '%s' deriving from a conformant array must be the last field in the structure\n",
                  field->name);
      has_pointer = 1;
      break;

    case RPC_FC_CSTRUCT:
      has_conformance = 1;
      if (list_next( fields, &field->entry ))
          error_loc("field '%s' deriving from a conformant array must be the last field in the structure\n",
                  field->name);
      break;

    case RPC_FC_PSTRUCT:
      has_pointer = 1;
      break;

    default:
      error_loc("Unknown struct member %s with type (0x%02x)\n", field->name, t->type);
      /* fallthru - treat it as complex */

    /* as soon as we see one of these these members, it's bogus... */
    case RPC_FC_ENCAPSULATED_UNION:
    case RPC_FC_NON_ENCAPSULATED_UNION:
    case RPC_FC_BOGUS_STRUCT:
    case RPC_FC_ENUM16:
      return RPC_FC_BOGUS_STRUCT;
    }
  }

  if( has_variance )
  {
    if ( has_conformance )
      return RPC_FC_CVSTRUCT;
    else
      return RPC_FC_BOGUS_STRUCT;
  }
  if( has_conformance && has_pointer )
    return RPC_FC_CPSTRUCT;
  if( has_conformance )
    return RPC_FC_CSTRUCT;
  if( has_pointer )
    return RPC_FC_PSTRUCT;
  return RPC_FC_STRUCT;
}

/***** constant repository *****/

struct rconst {
  char *name;
  var_t *var;
  struct rconst *next;
};

struct rconst *const_hash[HASHMAX];

static var_t *reg_const(var_t *var)
{
  struct rconst *nc;
  int hash;
  if (!var->name) {
    error_loc("registering constant without name\n");
    return var;
  }
  hash = hash_ident(var->name);
  nc = xmalloc(sizeof(struct rconst));
  nc->name = var->name;
  nc->var = var;
  nc->next = const_hash[hash];
  const_hash[hash] = nc;
  return var;
}

static var_t *find_const(char *name, int f)
{
  struct rconst *cur = const_hash[hash_ident(name)];
  while (cur && strcmp(cur->name, name))
    cur = cur->next;
  if (!cur) {
    if (f) error_loc("constant '%s' not found\n", name);
    return NULL;
  }
  return cur->var;
}

static void write_libid(const char *name, const attr_list_t *attr)
{
  const UUID *uuid = get_attrp(attr, ATTR_UUID);
  write_guid(idfile, "LIBID", name, uuid);
}

static void write_clsid(type_t *cls)
{
  const UUID *uuid = get_attrp(cls->attrs, ATTR_UUID);
  write_guid(idfile, "CLSID", cls->name, uuid);
}

static void write_diid(type_t *iface)
{
  const UUID *uuid = get_attrp(iface->attrs, ATTR_UUID);
  write_guid(idfile, "DIID", iface->name, uuid);
}

static void write_iid(type_t *iface)
{
  const UUID *uuid = get_attrp(iface->attrs, ATTR_UUID);
  write_guid(idfile, "IID", iface->name, uuid);
}

static int compute_method_indexes(type_t *iface)
{
  int idx;
  func_t *f;

  if (iface->ref)
    idx = compute_method_indexes(iface->ref);
  else
    idx = 0;

  if (!iface->funcs)
    return idx;

  LIST_FOR_EACH_ENTRY( f, iface->funcs, func_t, entry )
    if (! is_callas(f->def->attrs))
      f->idx = idx++;

  return idx;
}

static char *gen_name(void)
{
  static const char format[] = "__WIDL_%s_generated_name_%08lX";
  static unsigned long n = 0;
  static const char *file_id;
  static size_t size;
  char *name;

  if (! file_id)
  {
    char *dst = dup_basename(input_name, ".idl");
    file_id = dst;

    for (; *dst; ++dst)
      if (! isalnum((unsigned char) *dst))
        *dst = '_';

    size = sizeof format - 7 + strlen(file_id) + 8;
  }

  name = xmalloc(size);
  sprintf(name, format, file_id, n++);
  return name;
}

static void process_typedefs(pident_list_t *pidents)
{
  pident_t *pident, *next;

  if (!pidents) return;
  LIST_FOR_EACH_ENTRY_SAFE( pident, next, pidents, pident_t, entry )
  {
    var_t *var = pident->var;
    type_t *type = find_type(var->name, 0);

    if (! parse_only && do_header)
      write_typedef(type);
    if (in_typelib && type->attrs)
      add_typelib_entry(type);

    free(pident);
    free(var);
  }
}

static void check_arg(var_t *arg)
{
  type_t *t = arg->type;

  if (t->type == 0 && ! is_var_ptr(arg))
    error_loc("argument '%s' has void type\n", arg->name);
}

static void check_all_user_types(ifref_list_t *ifrefs)
{
  const ifref_t *ifref;
  const func_t *f;

  if (ifrefs) LIST_FOR_EACH_ENTRY(ifref, ifrefs, const ifref_t, entry)
  {
    const func_list_t *fs = ifref->iface->funcs;
    if (fs) LIST_FOR_EACH_ENTRY(f, fs, const func_t, entry)
      check_for_user_types_and_context_handles(f->args);
  }
}

int is_valid_uuid(const char *s)
{
  int i;

  for (i = 0; i < 36; ++i)
    if (i == 8 || i == 13 || i == 18 || i == 23)
    {
      if (s[i] != '-')
        return FALSE;
    }
    else
      if (!isxdigit(s[i]))
        return FALSE;

  return s[i] == '\0';
}
