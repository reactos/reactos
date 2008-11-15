%{
/*
 * IDL Compiler
 *
 * Copyright 2002 Ove Kaaven
 * Copyright 2006-2008 Robert Shearman
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
#include "expr.h"

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

#define YYERROR_VERBOSE

unsigned char pointer_default = RPC_FC_UP;
static int is_in_interface = FALSE;
static int is_object_interface = FALSE;
/* are we inside a library block? */
static int is_inside_library = FALSE;

typedef struct list typelist_t;
struct typenode {
  type_t *type;
  struct list entry;
};

struct _import_t
{
  char *name;
  int import_performed;
};

typedef struct _decl_spec_t
{
  type_t *type;
  attr_list_t *attrs;
  enum storage_class stgclass;
} decl_spec_t;

typelist_t incomplete_types = LIST_INIT(incomplete_types);

static void add_incomplete(type_t *t);
static void fix_incomplete(void);

static str_list_t *append_str(str_list_t *list, char *str);
static attr_list_t *append_attr(attr_list_t *list, attr_t *attr);
static attr_list_t *append_attr_list(attr_list_t *new_list, attr_list_t *old_list);
static decl_spec_t *make_decl_spec(type_t *type, decl_spec_t *left, decl_spec_t *right, attr_t *attr, enum storage_class stgclass);
static attr_t *make_attr(enum attr_type type);
static attr_t *make_attrv(enum attr_type type, unsigned long val);
static attr_t *make_attrp(enum attr_type type, void *val);
static expr_list_t *append_expr(expr_list_t *list, expr_t *expr);
static array_dims_t *append_array(array_dims_t *list, expr_t *expr);
static void set_type(var_t *v, decl_spec_t *decl_spec, const declarator_t *decl, int top);
static var_list_t *set_var_types(attr_list_t *attrs, decl_spec_t *decl_spec, declarator_list_t *decls);
static ifref_list_t *append_ifref(ifref_list_t *list, ifref_t *iface);
static ifref_t *make_ifref(type_t *iface);
static var_list_t *append_var(var_list_t *list, var_t *var);
static var_list_t *append_var_list(var_list_t *list, var_list_t *vars);
static var_t *make_var(char *name);
static declarator_list_t *append_declarator(declarator_list_t *list, declarator_t *p);
static declarator_t *make_declarator(var_t *var);
static func_list_t *append_func(func_list_t *list, func_t *func);
static func_t *make_func(var_t *def);
static type_t *make_class(char *name);
static type_t *make_safearray(type_t *type);
static type_t *make_builtin(char *name);
static type_t *make_int(int sign);
static typelib_t *make_library(const char *name, const attr_list_t *attrs);
static type_t *make_func_type(var_list_t *args);
static type_t *make_pointer_type(type_t *ref, attr_list_t *attrs);
static type_t *append_ptrchain_type(type_t *ptrchain, type_t *type);

static type_t *reg_type(type_t *type, const char *name, int t);
static type_t *reg_typedefs(decl_spec_t *decl_spec, var_list_t *names, attr_list_t *attrs);
static type_t *find_type2(char *name, int t);
static type_t *get_type(unsigned char type, char *name, int t);
static type_t *get_typev(unsigned char type, var_t *name, int t);
static int get_struct_type(var_list_t *fields);

static var_t *reg_const(var_t *var);

static void write_libid(const typelib_t *typelib);
static void write_clsid(type_t *cls);
static void write_diid(type_t *iface);
static void write_iid(type_t *iface);

static int compute_method_indexes(type_t *iface);
static char *gen_name(void);
static statement_t *process_typedefs(var_list_t *names);
static void check_arg(var_t *arg);
static void check_functions(const type_t *iface);
static void check_all_user_types(const statement_list_t *stmts);
static attr_list_t *check_iface_attrs(const char *name, attr_list_t *attrs);
static attr_list_t *check_function_attrs(const char *name, attr_list_t *attrs);
static attr_list_t *check_typedef_attrs(attr_list_t *attrs);
static attr_list_t *check_field_attrs(const char *name, attr_list_t *attrs);
static attr_list_t *check_library_attrs(const char *name, attr_list_t *attrs);
static attr_list_t *check_dispiface_attrs(const char *name, attr_list_t *attrs);
static attr_list_t *check_module_attrs(const char *name, attr_list_t *attrs);
static attr_list_t *check_coclass_attrs(const char *name, attr_list_t *attrs);
const char *get_attr_display_name(enum attr_type type);
static void add_explicit_handle_if_necessary(func_t *func);

static statement_t *make_statement(enum statement_type type);
static statement_t *make_statement_type_decl(type_t *type);
static statement_t *make_statement_reference(type_t *type);
static statement_t *make_statement_declaration(var_t *var);
static statement_t *make_statement_library(typelib_t *typelib);
static statement_t *make_statement_cppquote(const char *str);
static statement_t *make_statement_importlib(const char *str);
static statement_t *make_statement_module(type_t *type);
static statement_t *make_statement_import(const char *str);
static statement_list_t *append_statement(statement_list_t *list, statement_t *stmt);
static func_list_t *append_func_from_statement(func_list_t *list, statement_t *stmt);

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
	declarator_t *declarator;
	declarator_list_t *declarator_list;
	func_t *func;
	func_list_t *func_list;
	statement_t *statement;
	statement_list_t *stmt_list;
	ifref_t *ifref;
	ifref_list_t *ifref_list;
	char *str;
	UUID *uuid;
	unsigned int num;
	double dbl;
	interface_info_t ifinfo;
	typelib_t *typelib;
	struct _import_t *import;
	struct _decl_spec_t *declspec;
	enum storage_class stgclass;
}

%token <str> aIDENTIFIER
%token <str> aKNOWNTYPE
%token <num> aNUM aHEXNUM
%token <dbl> aDOUBLE
%token <str> aSTRING aWSTRING
%token <uuid> aUUID
%token aEOF
%token SHL SHR
%token MEMBERPTR
%token EQUALITY INEQUALITY
%token GREATEREQUAL LESSEQUAL
%token LOGICALOR LOGICALAND
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
%token tFASTCALL
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
%token tIN tIN_LINE tINLINE
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
%token tNULL
%token tOBJECT tODL tOLEAUTOMATION
%token tOPTIONAL
%token tOUT
%token tPASCAL
%token tPOINTERDEFAULT
%token tPROPERTIES
%token tPROPGET tPROPPUT tPROPPUTREF
%token tPTR
%token tPUBLIC
%token tRANGE
%token tREADONLY tREF
%token tREGISTER
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
%token tSTATIC
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

%type <attr> attribute type_qualifier function_specifier
%type <attr_list> m_attributes attributes attrib_list m_type_qual_list
%type <str_list> str_list
%type <expr> m_expr expr expr_const expr_int_const array
%type <expr_list> m_exprs /* exprs expr_list */ expr_list_int_const
%type <ifinfo> interfacehdr
%type <stgclass> storage_cls_spec
%type <declspec> decl_spec decl_spec_no_type m_decl_spec_no_type
%type <type> inherit interface interfacedef interfacedec
%type <type> dispinterface dispinterfacehdr dispinterfacedef
%type <type> module modulehdr moduledef
%type <type> base_type int_std
%type <type> enumdef structdef uniondef
%type <type> type
%type <ifref> coclass_int
%type <ifref_list> coclass_ints
%type <var> arg ne_union_field union_field s_field case enum declaration
%type <var_list> m_args no_args args fields ne_union_fields cases enums enum_list dispint_props field
%type <var> m_ident t_ident ident
%type <declarator> declarator direct_declarator init_declarator
%type <declarator_list> declarator_list
%type <func> funcdef
%type <func_list> int_statements dispint_meths
%type <type> coclass coclasshdr coclassdef
%type <num> pointer_type version
%type <str> libraryhdr callconv cppquote importlib import
%type <uuid> uuid_string
%type <import> import_start
%type <typelib> library_start librarydef
%type <statement> statement typedef
%type <stmt_list> gbl_statements imp_statements

%left ','
%right '?' ':'
%left LOGICALOR
%left LOGICALAND
%left '|'
%left '^'
%left '&'
%left EQUALITY INEQUALITY
%left '<' '>' LESSEQUAL GREATEREQUAL
%left SHL SHR
%left '-' '+'
%left '*' '/' '%'
%right '!' '~' CAST PPTR POS NEG ADDRESSOF tSIZEOF
%left '.' MEMBERPTR '[' ']'

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
	| gbl_statements interfacedef		{ $$ = append_statement($1, make_statement_type_decl($2)); }
	| gbl_statements coclass ';'		{ $$ = $1;
						  reg_type($2, $2->name, 0);
						  if (!parse_only && do_header) write_coclass_forward($2);
						}
	| gbl_statements coclassdef		{ $$ = append_statement($1, make_statement_type_decl($2));
						  add_typelib_entry($2);
						  reg_type($2, $2->name, 0);
						  if (!parse_only && do_header) write_coclass_forward($2);
						}
	| gbl_statements moduledef		{ $$ = append_statement($1, make_statement_module($2));
						  add_typelib_entry($2);
						}
	| gbl_statements librarydef		{ $$ = append_statement($1, make_statement_library($2)); }
	| gbl_statements statement		{ $$ = append_statement($1, $2); }
	;

imp_statements:					{ $$ = NULL; }
	| imp_statements interfacedec		{ $$ = append_statement($1, make_statement_reference($2)); if (!parse_only) add_typelib_entry($2); }
	| imp_statements interfacedef		{ $$ = append_statement($1, make_statement_type_decl($2)); if (!parse_only) add_typelib_entry($2); }
	| imp_statements coclass ';'		{ $$ = $1; reg_type($2, $2->name, 0); if (!parse_only && do_header) write_coclass_forward($2); }
	| imp_statements coclassdef		{ $$ = append_statement($1, make_statement_type_decl($2));
						  if (!parse_only) add_typelib_entry($2);
						  reg_type($2, $2->name, 0);
						  if (!parse_only && do_header) write_coclass_forward($2);
						}
	| imp_statements moduledef		{ $$ = append_statement($1, make_statement_module($2)); if (!parse_only) add_typelib_entry($2); }
	| imp_statements statement		{ $$ = append_statement($1, $2); }
	| imp_statements importlib		{ $$ = append_statement($1, make_statement_importlib($2)); }
	| imp_statements librarydef		{ $$ = append_statement($1, make_statement_library($2)); }
	;

int_statements:					{ $$ = NULL; }
	| int_statements statement		{ $$ = append_func_from_statement( $1, $2 ); }
	;

semicolon_opt:
	| ';'
	;

statement:
	  cppquote				{ $$ = make_statement_cppquote($1); }
	| enumdef ';'				{ $$ = make_statement_type_decl($1);
						  if (!parse_only && do_header) {
						    write_type_def_or_decl(header, $1, FALSE, NULL);
						    fprintf(header, ";\n\n");
						  }
						}
	| declaration ';'			{ $$ = make_statement_declaration($1);
						  if (!parse_only && do_header) write_declaration($1, is_in_interface);
						}
	| import				{ $$ = make_statement_import($1); }
	| structdef ';'				{ $$ = make_statement_type_decl($1);
						  if (!parse_only && do_header) {
						    write_type_def_or_decl(header, $1, FALSE, NULL);
						    fprintf(header, ";\n\n");
						  }
						}
	| typedef ';'				{ $$ = $1; }
	| uniondef ';'				{ $$ = make_statement_type_decl($1);
						  if (!parse_only && do_header) {
						    write_type_def_or_decl(header, $1, FALSE, NULL);
						    fprintf(header, ";\n\n");
						  }
						}
	;

cppquote: tCPPQUOTE '(' aSTRING ')'		{ $$ = $3; if (!parse_only && do_header) fprintf(header, "%s\n", $3); }
	;
import_start: tIMPORT aSTRING ';'		{ assert(yychar == YYEMPTY);
						  $$ = xmalloc(sizeof(struct _import_t));
						  $$->name = $2;
						  $$->import_performed = do_import($2);
						  if (!$$->import_performed) yychar = aEOF;
						}
	;

import: import_start imp_statements aEOF	{ $$ = $1->name;
						  if ($1->import_performed) pop_import();
						  free($1);
						  if (!parse_only && do_header) write_import($$);
						}
	;

importlib: tIMPORTLIB '(' aSTRING ')'
	   semicolon_opt			{ $$ = $3; if(!parse_only) add_importlib($3); }
	;

libraryhdr: tLIBRARY aIDENTIFIER		{ $$ = $2; }
	;
library_start: attributes libraryhdr '{'	{ $$ = make_library($2, check_library_attrs($2, $1));
						  if (!parse_only) start_typelib($$);
						  if (!parse_only && do_header) write_library($$);
						  if (!parse_only && do_idfile) write_libid($$);
						  is_inside_library = TRUE;
						}
	;
librarydef: library_start imp_statements '}'
	    semicolon_opt			{ $$ = $1;
						  $$->stmts = $2;
						  if (!parse_only) end_typelib();
						  is_inside_library = FALSE;
						}
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
arg:	  attributes decl_spec declarator	{ $$ = $3->var;
						  $$->attrs = $1;
						  if ($2->stgclass != STG_NONE && $2->stgclass != STG_REGISTER)
						    error_loc("invalid storage class for function parameter\n");
						  set_type($$, $2, $3, TRUE);
						  free($3);
						}
	| decl_spec declarator			{ $$ = $2->var;
						  if ($1->stgclass != STG_NONE && $1->stgclass != STG_REGISTER)
						    error_loc("invalid storage class for function parameter\n");
						  set_type($$, $1, $2, TRUE);
						  free($2);
						}
	;

array:	  '[' m_expr ']'			{ $$ = $2; }
	| '[' '*' ']'				{ $$ = make_expr(EXPR_VOID); }
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
	| tBROADCAST				{ $$ = make_attr(ATTR_BROADCAST); }
	| tCALLAS '(' ident ')'			{ $$ = make_attrp(ATTR_CALLAS, $3); }
	| tCASE '(' expr_list_int_const ')'	{ $$ = make_attrp(ATTR_CASE, $3); }
	| tCONTEXTHANDLE			{ $$ = make_attrv(ATTR_CONTEXTHANDLE, 0); }
	| tCONTEXTHANDLENOSERIALIZE		{ $$ = make_attrv(ATTR_CONTEXTHANDLE, 0); /* RPC_CONTEXT_HANDLE_DONT_SERIALIZE */ }
	| tCONTEXTHANDLESERIALIZE		{ $$ = make_attrv(ATTR_CONTEXTHANDLE, 0); /* RPC_CONTEXT_HANDLE_SERIALIZE */ }
	| tCONTROL				{ $$ = make_attr(ATTR_CONTROL); }
	| tDEFAULT				{ $$ = make_attr(ATTR_DEFAULT); }
	| tDEFAULTCOLLELEM			{ $$ = make_attr(ATTR_DEFAULTCOLLELEM); }
	| tDEFAULTVALUE '(' expr_const ')'	{ $$ = make_attrp(ATTR_DEFAULTVALUE, $3); }
	| tDEFAULTVTABLE			{ $$ = make_attr(ATTR_DEFAULTVTABLE); }
	| tDISPLAYBIND				{ $$ = make_attr(ATTR_DISPLAYBIND); }
	| tDLLNAME '(' aSTRING ')'		{ $$ = make_attrp(ATTR_DLLNAME, $3); }
	| tDUAL					{ $$ = make_attr(ATTR_DUAL); }
	| tENDPOINT '(' str_list ')'		{ $$ = make_attrp(ATTR_ENDPOINT, $3); }
	| tENTRY '(' expr_const ')'		{ $$ = make_attrp(ATTR_ENTRY, $3); }
	| tEXPLICITHANDLE			{ $$ = make_attr(ATTR_EXPLICIT_HANDLE); }
	| tHANDLE				{ $$ = make_attr(ATTR_HANDLE); }
	| tHELPCONTEXT '(' expr_int_const ')'	{ $$ = make_attrp(ATTR_HELPCONTEXT, $3); }
	| tHELPFILE '(' aSTRING ')'		{ $$ = make_attrp(ATTR_HELPFILE, $3); }
	| tHELPSTRING '(' aSTRING ')'		{ $$ = make_attrp(ATTR_HELPSTRING, $3); }
	| tHELPSTRINGCONTEXT '(' expr_int_const ')'	{ $$ = make_attrp(ATTR_HELPSTRINGCONTEXT, $3); }
	| tHELPSTRINGDLL '(' aSTRING ')'	{ $$ = make_attrp(ATTR_HELPSTRINGDLL, $3); }
	| tHIDDEN				{ $$ = make_attr(ATTR_HIDDEN); }
	| tID '(' expr_int_const ')'		{ $$ = make_attrp(ATTR_ID, $3); }
	| tIDEMPOTENT				{ $$ = make_attr(ATTR_IDEMPOTENT); }
	| tIIDIS '(' expr ')'			{ $$ = make_attrp(ATTR_IIDIS, $3); }
	| tIMMEDIATEBIND			{ $$ = make_attr(ATTR_IMMEDIATEBIND); }
	| tIMPLICITHANDLE '(' tHANDLET aIDENTIFIER ')'	{ $$ = make_attrp(ATTR_IMPLICIT_HANDLE, $4); }
	| tIN					{ $$ = make_attr(ATTR_IN); }
	| tINPUTSYNC				{ $$ = make_attr(ATTR_INPUTSYNC); }
	| tLENGTHIS '(' m_exprs ')'		{ $$ = make_attrp(ATTR_LENGTHIS, $3); }
	| tLCID	'(' expr_int_const ')'		{ $$ = make_attrp(ATTR_LIBLCID, $3); }
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
	| tRANGE '(' expr_int_const ',' expr_int_const ')'
						{ expr_list_t *list = append_expr( NULL, $3 );
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

callconv: tCDECL				{ $$ = $<str>1; }
	| tFASTCALL				{ $$ = $<str>1; }
	| tPASCAL				{ $$ = $<str>1; }
	| tSTDCALL				{ $$ = $<str>1; }
	;

cases:						{ $$ = NULL; }
	| cases case				{ $$ = append_var( $1, $2 ); }
	;

case:	  tCASE expr_int_const ':' union_field	{ attr_t *a = make_attrp(ATTR_CASE, append_expr( NULL, $2 ));
						  $$ = $4; if (!$$) $$ = make_var(NULL);
						  $$->attrs = append_attr( $$->attrs, a );
						}
	| tDEFAULT ':' union_field		{ attr_t *a = make_attr(ATTR_DEFAULT);
						  $$ = $3; if (!$$) $$ = make_var(NULL);
						  $$->attrs = append_attr( $$->attrs, a );
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

enum:	  ident '=' expr_int_const		{ $$ = reg_const($1);
						  $$->eval = $3;
                                                  $$->type = make_int(0);
						}
	| ident					{ $$ = reg_const($1);
                                                  $$->type = make_int(0);
						}
	;

enumdef: tENUM t_ident '{' enums '}'		{ $$ = get_typev(RPC_FC_ENUM16, $2, tsENUM);
						  $$->kind = TKIND_ENUM;
						  $$->fields_or_args = $4;
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
	| tNULL					{ $$ = make_exprl(EXPR_NUM, 0); }
	| tTRUE					{ $$ = make_exprl(EXPR_TRUEFALSE, 1); }
	| aSTRING				{ $$ = make_exprs(EXPR_STRLIT, $1); }
	| aWSTRING				{ $$ = make_exprs(EXPR_WSTRLIT, $1); }
	| aIDENTIFIER				{ $$ = make_exprs(EXPR_IDENTIFIER, $1); }
	| expr '?' expr ':' expr		{ $$ = make_expr3(EXPR_COND, $1, $3, $5); }
	| expr LOGICALOR expr			{ $$ = make_expr2(EXPR_LOGOR, $1, $3); }
	| expr LOGICALAND expr			{ $$ = make_expr2(EXPR_LOGAND, $1, $3); }
	| expr '|' expr				{ $$ = make_expr2(EXPR_OR , $1, $3); }
	| expr '^' expr				{ $$ = make_expr2(EXPR_XOR, $1, $3); }
	| expr '&' expr				{ $$ = make_expr2(EXPR_AND, $1, $3); }
	| expr EQUALITY expr			{ $$ = make_expr2(EXPR_EQUALITY, $1, $3); }
	| expr INEQUALITY expr			{ $$ = make_expr2(EXPR_INEQUALITY, $1, $3); }
	| expr '>' expr				{ $$ = make_expr2(EXPR_GTR, $1, $3); }
	| expr '<' expr				{ $$ = make_expr2(EXPR_LESS, $1, $3); }
	| expr GREATEREQUAL expr		{ $$ = make_expr2(EXPR_GTREQL, $1, $3); }
	| expr LESSEQUAL expr			{ $$ = make_expr2(EXPR_LESSEQL, $1, $3); }
	| expr SHL expr				{ $$ = make_expr2(EXPR_SHL, $1, $3); }
	| expr SHR expr				{ $$ = make_expr2(EXPR_SHR, $1, $3); }
	| expr '+' expr				{ $$ = make_expr2(EXPR_ADD, $1, $3); }
	| expr '-' expr				{ $$ = make_expr2(EXPR_SUB, $1, $3); }
	| expr '%' expr				{ $$ = make_expr2(EXPR_MOD, $1, $3); }
	| expr '*' expr				{ $$ = make_expr2(EXPR_MUL, $1, $3); }
	| expr '/' expr				{ $$ = make_expr2(EXPR_DIV, $1, $3); }
	| '!' expr				{ $$ = make_expr1(EXPR_LOGNOT, $2); }
	| '~' expr				{ $$ = make_expr1(EXPR_NOT, $2); }
	| '+' expr %prec POS			{ $$ = make_expr1(EXPR_POS, $2); }
	| '-' expr %prec NEG			{ $$ = make_expr1(EXPR_NEG, $2); }
	| '&' expr %prec ADDRESSOF		{ $$ = make_expr1(EXPR_ADDRESSOF, $2); }
	| '*' expr %prec PPTR			{ $$ = make_expr1(EXPR_PPTR, $2); }
	| expr MEMBERPTR aIDENTIFIER		{ $$ = make_expr2(EXPR_MEMBER, make_expr1(EXPR_PPTR, $1), make_exprs(EXPR_IDENTIFIER, $3)); }
	| expr '.' aIDENTIFIER			{ $$ = make_expr2(EXPR_MEMBER, $1, make_exprs(EXPR_IDENTIFIER, $3)); }
	| '(' type ')' expr %prec CAST		{ $$ = make_exprt(EXPR_CAST, $2, $4); }
	| tSIZEOF '(' type ')'			{ $$ = make_exprt(EXPR_SIZEOF, $3, NULL); }
	| expr '[' expr ']'			{ $$ = make_expr2(EXPR_ARRAY, $1, $3); }
	| '(' expr ')'				{ $$ = $2; }
	;

expr_list_int_const: expr_int_const		{ $$ = append_expr( NULL, $1 ); }
	| expr_list_int_const ',' expr_int_const	{ $$ = append_expr( $1, $3 ); }
	;

expr_int_const: expr				{ $$ = $1;
						  if (!$$->is_const)
						      error_loc("expression is not an integer constant\n");
						}
	;

expr_const: expr				{ $$ = $1;
						  if (!$$->is_const && $$->type != EXPR_STRLIT && $$->type != EXPR_WSTRLIT)
						      error_loc("expression is not constant\n");
						}
	;

fields:						{ $$ = NULL; }
	| fields field				{ $$ = append_var_list($1, $2); }
	;

field:	  m_attributes decl_spec declarator_list ';'
						{ const char *first = LIST_ENTRY(list_head($3), declarator_t, entry)->var->name;
						  check_field_attrs(first, $1);
						  $$ = set_var_types($1, $2, $3);
						}
	| m_attributes uniondef ';'		{ var_t *v = make_var(NULL);
						  v->type = $2; v->attrs = $1;
						  $$ = append_var(NULL, v);
						}
	;

ne_union_field:
	  s_field ';'				{ $$ = $1; }
	| attributes ';'			{ $$ = make_var(NULL); $$->attrs = $1; }
	;

ne_union_fields:				{ $$ = NULL; }
	| ne_union_fields ne_union_field	{ $$ = append_var( $1, $2 ); }
	;

union_field:
	  s_field ';'				{ $$ = $1; }
	| ';'					{ $$ = NULL; }
	;

s_field:  m_attributes decl_spec declarator	{ $$ = $3->var;
						  $$->attrs = check_field_attrs($$->name, $1);
						  set_type($$, $2, $3, FALSE);
						  free($3);
						}
	;

funcdef:
	  m_attributes decl_spec declarator	{ var_t *v = $3->var;
						  v->attrs = check_function_attrs(v->name, $1);
						  set_type(v, $2, $3, FALSE);
						  free($3);
						  $$ = make_func(v);
						}
	;

declaration:
	  attributes decl_spec init_declarator
						{ $$ = $3->var;
						  $$->attrs = $1;
						  set_type($$, $2, $3, FALSE);
						  free($3);
						}
	| decl_spec init_declarator		{ $$ = $2->var;
						  set_type($$, $1, $2, FALSE);
						  free($2);
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
						  $$->attrs = check_coclass_attrs($2->name, $1);
						  if (!parse_only && do_header)
						    write_coclass($$);
						  if (!parse_only && do_idfile)
						    write_clsid($$);
						}
	;

coclassdef: coclasshdr '{' coclass_ints '}' semicolon_opt
						{ $$ = $1;
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
						  is_in_interface = TRUE;
						  is_object_interface = TRUE;
						  $$ = $2;
						  if ($$->defined) error_loc("multiple definition error\n");
						  attrs = make_attr(ATTR_DISPINTERFACE);
						  $$->attrs = append_attr( check_dispiface_attrs($2->name, $1), attrs );
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
						  $$->fields_or_args = $3;
						  $$->funcs = $4;
						  if (!parse_only && do_header) write_interface($$);
						  if (!parse_only && do_idfile) write_diid($$);
						  is_in_interface = FALSE;
						}
	| dispinterfacehdr
	 '{' interface ';' '}' 			{ $$ = $1;
						  $$->fields_or_args = $3->fields_or_args;
						  $$->funcs = $3->funcs;
						  if (!parse_only && do_header) write_interface($$);
						  if (!parse_only && do_idfile) write_diid($$);
						  is_in_interface = FALSE;
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
						  is_object_interface = is_object($1);
						  is_in_interface = TRUE;
						  if ($2->defined) error_loc("multiple definition error\n");
						  $2->attrs = check_iface_attrs($2->name, $1);
						  $2->defined = TRUE;
						  if (!parse_only && do_header) write_forward($2);
						}
	;

interfacedef: interfacehdr inherit
	  '{' int_statements '}' semicolon_opt	{ $$ = $1.interface;
						  $$->ref = $2;
						  $$->funcs = $4;
						  check_functions($$);
						  compute_method_indexes($$);
						  if (!parse_only && do_header) write_interface($$);
						  if (!parse_only && local_stubs) write_locals(local_stubs, $$, TRUE);
						  if (!parse_only && do_idfile) write_iid($$);
						  pointer_default = $1.old_pointer_default;
						  is_in_interface = FALSE;
						}
/* MIDL is able to import the definition of a base class from inside the
 * definition of a derived class, I'll try to support it with this rule */
	| interfacehdr ':' aIDENTIFIER
	  '{' import int_statements '}'
	   semicolon_opt			{ $$ = $1.interface;
						  $$->ref = find_type2($3, 0);
						  if (!$$->ref) error_loc("base class '%s' not found in import\n", $3);
						  $$->funcs = $6;
						  compute_method_indexes($$);
						  if (!parse_only && do_header) write_interface($$);
						  if (!parse_only && local_stubs) write_locals(local_stubs, $$, TRUE);
						  if (!parse_only && do_idfile) write_iid($$);
						  pointer_default = $1.old_pointer_default;
						  is_in_interface = FALSE;
						}
	| dispinterfacedef semicolon_opt	{ $$ = $1; }
	;

interfacedec:
	  interface ';'				{ $$ = $1; if (!parse_only && do_header) write_forward($$); }
	| dispinterface ';'			{ $$ = $1; if (!parse_only && do_header) write_forward($$); }
	;

module:   tMODULE aIDENTIFIER			{ $$ = make_type(0, NULL); $$->name = $2; $$->kind = TKIND_MODULE; }
	| tMODULE aKNOWNTYPE			{ $$ = make_type(0, NULL); $$->name = $2; $$->kind = TKIND_MODULE; }
	;

modulehdr: attributes module			{ $$ = $2;
						  $$->attrs = check_module_attrs($2->name, $1);
						}
	;

moduledef: modulehdr '{' int_statements '}'
	   semicolon_opt			{ $$ = $1;
						  $$->funcs = $3;
						  /* FIXME: if (!parse_only && do_header) write_module($$); */
						}
	;

storage_cls_spec:
	  tEXTERN				{ $$ = STG_EXTERN; }
	| tSTATIC				{ $$ = STG_STATIC; }
	| tREGISTER				{ $$ = STG_REGISTER; }
	;

function_specifier:
	  tINLINE				{ $$ = make_attr(ATTR_INLINE); }
	;

type_qualifier:
	  tCONST				{ $$ = make_attr(ATTR_CONST); }
	;

m_type_qual_list:				{ $$ = NULL; }
	| m_type_qual_list type_qualifier	{ $$ = append_attr($1, $2); }
	;

decl_spec: type m_decl_spec_no_type		{ $$ = make_decl_spec($1, $2, NULL, NULL, STG_NONE); }
	| decl_spec_no_type type m_decl_spec_no_type
						{ $$ = make_decl_spec($2, $1, $3, NULL, STG_NONE); }
	;

m_decl_spec_no_type:				{ $$ = NULL; }
	| decl_spec_no_type
	;

decl_spec_no_type:
	  type_qualifier m_decl_spec_no_type	{ $$ = make_decl_spec(NULL, $2, NULL, $1, STG_NONE); }
	| function_specifier m_decl_spec_no_type  { $$ = make_decl_spec(NULL, $2, NULL, $1, STG_NONE); }
	| storage_cls_spec m_decl_spec_no_type  { $$ = make_decl_spec(NULL, $2, NULL, NULL, $1); }
	;

declarator:
	  '*' m_type_qual_list declarator %prec PPTR
						{ $$ = $3; $$->type = append_ptrchain_type($$->type, make_pointer_type(NULL, $2)); }
	| callconv declarator			{ $$ = $2; $$->type->attrs = append_attr($$->type->attrs, make_attrp(ATTR_CALLCONV, $1)); }
	| direct_declarator
	;

direct_declarator:
	  ident					{ $$ = make_declarator($1); }
	| '(' declarator ')'			{ $$ = $2; }
	| direct_declarator array		{ $$ = $1; $$->array = append_array($$->array, $2); }
	| direct_declarator '(' m_args ')'	{ $$ = $1;
						  $$->func_type = append_ptrchain_type($$->type, make_func_type($3));
						  $$->type = NULL;
						}
	;

declarator_list:
	  declarator				{ $$ = append_declarator( NULL, $1 ); }
	| declarator_list ',' declarator	{ $$ = append_declarator( $1, $3 ); }
	;

init_declarator:
	  declarator				{ $$ = $1; }
	| declarator '=' expr_const		{ $$ = $1; $1->var->eval = $3; }
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
						  $$->fields_or_args = $4;
						  $$->defined = TRUE;
                                                  if(in_typelib)
                                                      add_typelib_entry($$);
                                                }
	;

type:	  tVOID					{ $$ = duptype(find_type("void", 0), 1); }
	| aKNOWNTYPE				{ $$ = find_type($1, 0); }
	| base_type				{ $$ = $1; }
	| enumdef				{ $$ = $1; }
	| tENUM aIDENTIFIER			{ $$ = find_type2($2, tsENUM); }
	| structdef				{ $$ = $1; }
	| tSTRUCT aIDENTIFIER			{ $$ = get_type(RPC_FC_STRUCT, $2, tsSTRUCT); }
	| uniondef				{ $$ = $1; }
	| tUNION aIDENTIFIER			{ $$ = find_type2($2, tsUNION); }
	| tSAFEARRAY '(' type ')'		{ $$ = make_safearray($3); }
	;

typedef: tTYPEDEF m_attributes decl_spec declarator_list
						{ reg_typedefs($3, $4, check_typedef_attrs($2));
						  $$ = process_typedefs($4);
						}
	;

uniondef: tUNION t_ident '{' ne_union_fields '}'
						{ $$ = get_typev(RPC_FC_NON_ENCAPSULATED_UNION, $2, tsUNION);
						  $$->kind = TKIND_UNION;
						  $$->fields_or_args = $4;
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
						  u->type->fields_or_args = $9;
						  u->type->defined = TRUE;
						  $$->fields_or_args = append_var( $$->fields_or_args, $5 );
						  $$->fields_or_args = append_var( $$->fields_or_args, u );
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
    attr_t *attr_existing;
    if (!attr) return list;
    if (!list)
    {
        list = xmalloc( sizeof(*list) );
        list_init( list );
    }
    LIST_FOR_EACH_ENTRY(attr_existing, list, attr_t, entry)
        if (attr_existing->type == attr->type)
        {
            parser_warning("duplicate attribute %s\n", get_attr_display_name(attr->type));
            /* use the last attribute, like MIDL does */
            list_remove(&attr_existing->entry);
            break;
        }
    list_add_tail( list, &attr->entry );
    return list;
}

static attr_list_t *move_attr(attr_list_t *dst, attr_list_t *src, enum attr_type type)
{
  attr_t *attr;
  if (!src) return dst;
  LIST_FOR_EACH_ENTRY(attr, src, attr_t, entry)
    if (attr->type == type)
    {
      list_remove(&attr->entry);
      return append_attr(dst, attr);
    }
  return dst;
}

static attr_list_t *append_attr_list(attr_list_t *new_list, attr_list_t *old_list)
{
  struct list *entry;

  if (!old_list) return new_list;

  while ((entry = list_head(old_list)))
  {
    attr_t *attr = LIST_ENTRY(entry, attr_t, entry);
    list_remove(entry);
    new_list = append_attr(new_list, attr);
  }
  return new_list;
}

static attr_list_t *dupattrs(const attr_list_t *list)
{
  attr_list_t *new_list;
  const attr_t *attr;

  if (!list) return NULL;

  new_list = xmalloc( sizeof(*list) );
  list_init( new_list );
  LIST_FOR_EACH_ENTRY(attr, list, const attr_t, entry)
  {
    attr_t *new_attr = xmalloc(sizeof(*new_attr));
    *new_attr = *attr;
    list_add_tail(new_list, &new_attr->entry);
  }
  return new_list;
}

static decl_spec_t *make_decl_spec(type_t *type, decl_spec_t *left, decl_spec_t *right, attr_t *attr, enum storage_class stgclass)
{
  decl_spec_t *declspec = left ? left : right;
  if (!declspec)
  {
    declspec = xmalloc(sizeof(*declspec));
    declspec->type = NULL;
    declspec->attrs = NULL;
    declspec->stgclass = STG_NONE;
  }
  declspec->type = type;
  if (left && declspec != left)
  {
    declspec->attrs = append_attr_list(declspec->attrs, left->attrs);
    if (declspec->stgclass == STG_NONE)
      declspec->stgclass = left->stgclass;
    else if (left->stgclass != STG_NONE)
      error_loc("only one storage class can be specified\n");
    assert(!left->type);
    free(left);
  }
  if (right && declspec != right)
  {
    declspec->attrs = append_attr_list(declspec->attrs, right->attrs);
    if (declspec->stgclass == STG_NONE)
      declspec->stgclass = right->stgclass;
    else if (right->stgclass != STG_NONE)
      error_loc("only one storage class can be specified\n");
    assert(!right->type);
    free(right);
  }

  declspec->attrs = append_attr(declspec->attrs, attr);
  if (declspec->stgclass == STG_NONE)
    declspec->stgclass = stgclass;
  else if (stgclass != STG_NONE)
    error_loc("only one storage class can be specified\n");

  /* apply attributes to type */
  if (type && declspec->attrs)
  {
    attr_list_t *attrs;
    declspec->type = duptype(type, 1);
    attrs = dupattrs(type->attrs);
    declspec->type->attrs = append_attr_list(attrs, declspec->attrs);
    declspec->attrs = NULL;
  }

  return declspec;
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

type_t *make_type(unsigned char type, type_t *ref)
{
  type_t *t = alloc_type();
  t->name = NULL;
  t->kind = TKIND_PRIMITIVE;
  t->type = type;
  t->ref = ref;
  t->attrs = NULL;
  t->orig = NULL;
  t->funcs = NULL;
  t->fields_or_args = NULL;
  t->ifaces = NULL;
  t->dim = 0;
  t->size_is = NULL;
  t->length_is = NULL;
  t->typestring_offset = 0;
  t->ptrdesc = 0;
  t->declarray = FALSE;
  t->ignore = (parse_only != 0);
  t->sign = 0;
  t->defined = FALSE;
  t->written = FALSE;
  t->user_types_registered = FALSE;
  t->tfswrite = FALSE;
  t->checked = FALSE;
  t->typelib_idx = -1;
  return t;
}

static type_t *make_func_type(var_list_t *args)
{
  type_t *t = make_type(RPC_FC_FUNCTION, NULL);
  t->fields_or_args = args;
  return t;
}

static type_t *make_pointer_type(type_t *ref, attr_list_t *attrs)
{
    type_t *t = make_type(pointer_default, ref);
    t->attrs = attrs;
    return t;
}

static type_t *append_ptrchain_type(type_t *ptrchain, type_t *type)
{
  type_t *ptrchain_type;
  if (!ptrchain)
    return type;
  for (ptrchain_type = ptrchain; ptrchain_type->ref; ptrchain_type = ptrchain_type->ref)
    ;
  ptrchain_type->ref = type;
  return ptrchain;
}

static void set_type(var_t *v, decl_spec_t *decl_spec, const declarator_t *decl,
                     int top)
{
  expr_list_t *sizes = get_attrp(v->attrs, ATTR_SIZEIS);
  expr_list_t *lengs = get_attrp(v->attrs, ATTR_LENGTHIS);
  int ptr_attr = get_attrv(v->attrs, ATTR_POINTERTYPE);
  int sizeless, has_varconf;
  expr_t *dim;
  type_t *atype, **ptype;
  array_dims_t *arr = decl ? decl->array : NULL;
  type_t *func_type = decl ? decl->func_type : NULL;
  type_t *type = decl_spec->type;

  if (is_attr(type->attrs, ATTR_INLINE))
  {
    if (!func_type)
      error_loc("inline attribute applied to non-function type\n");
    else
    {
      type_t *t;
      /* move inline attribute from return type node to function node */
      for (t = func_type; is_ptr(t); t = t->ref)
        ;
      t->attrs = move_attr(t->attrs, type->attrs, ATTR_INLINE);
    }
  }

  /* add type onto the end of the pointers in pident->type */
  v->type = append_ptrchain_type(decl ? decl->type : NULL, type);
  v->stgclass = decl_spec->stgclass;

  /* the highest level of pointer specified should default to the var's ptr attr
   * or (RPC_FC_RP if not specified and it's a top level ptr), not
   * pointer_default so we need to fix that up here */
  if (!arr)
  {
    const type_t *ptr = NULL;
    /* pointer attributes on the left side of the type belong to the function
     * pointer, if one is being declared */
    type_t **pt = func_type ? &func_type : &v->type;
    for (ptr = *pt; ptr; )
    {
      if (ptr->kind == TKIND_ALIAS)
        ptr = ptr->orig;
      else
        break;
    }
    if (ptr && is_ptr(ptr) && (ptr_attr || top))
    {
      /* duplicate type to avoid changing original type */
      *pt = duptype(*pt, 1);
      (*pt)->type = ptr_attr ? ptr_attr : RPC_FC_RP;
    }
    else if (ptr_attr)
       error_loc("%s: pointer attribute applied to non-pointer type\n", v->name);
  }

  if (is_attr(v->attrs, ATTR_STRING) && !is_ptr(v->type) && !arr)
    error_loc("'%s': [string] attribute applied to non-pointer, non-array type\n",
              v->name);

  sizeless = FALSE;
  if (arr) LIST_FOR_EACH_ENTRY_REV(dim, arr, expr_t, entry)
  {
    if (sizeless)
      error_loc("%s: only the first array dimension can be unspecified\n", v->name);

    if (dim->is_const)
    {
      unsigned int align = 0;
      size_t size = type_memsize(v->type, &align);

      if (dim->cval <= 0)
        error_loc("%s: array dimension must be positive\n", v->name);

      if (0xffffffffuL / size < (unsigned long) dim->cval)
        error_loc("%s: total array size is too large\n", v->name);
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
        error_loc("%s: cannot specify size_is for a fixed sized array\n", v->name);

      if (atype->type != RPC_FC_CARRAY && !is_ptr(atype))
        error_loc("%s: size_is attribute applied to illegal type\n", v->name);

      atype->type = RPC_FC_CARRAY;
      atype->size_is = dim;
    }

    ptype = &(*ptype)->ref;
    if (*ptype == NULL)
      error_loc("%s: too many expressions in size_is attribute\n", v->name);
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
        error_loc("%s: length_is attribute applied to illegal type\n", v->name);

      atype->length_is = dim;
    }

    ptype = &(*ptype)->ref;
    if (*ptype == NULL)
      error_loc("%s: too many expressions in length_is attribute\n", v->name);
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

  /* v->type is currently pointing to the type on the left-side of the
   * declaration, so we need to fix this up so that it is the return type of the
   * function and make v->type point to the function side of the declaration */
  if (func_type)
  {
    type_t *ft, *t;
    type_t *return_type = v->type;
    v->type = func_type;
    for (ft = v->type; is_ptr(ft); ft = ft->ref)
      ;
    assert(ft->type == RPC_FC_FUNCTION);
    ft->ref = return_type;
    /* move calling convention attribute, if present, from pointer nodes to
     * function node */
    for (t = v->type; is_ptr(t); t = t->ref)
      ft->attrs = move_attr(ft->attrs, t->attrs, ATTR_CALLCONV);
    if (is_object_interface && !is_attr(ft->attrs, ATTR_CALLCONV))
    {
      static char *stdmethodcalltype;
      if (!stdmethodcalltype) stdmethodcalltype = strdup("STDMETHODCALLTYPE");
      ft->attrs = append_attr(NULL, make_attrp(ATTR_CALLCONV, stdmethodcalltype));
    }
  }
  else
  {
    type_t *t;
    for (t = v->type; is_ptr(t); t = t->ref)
      if (is_attr(t->attrs, ATTR_CALLCONV))
        error_loc("calling convention applied to non-function-pointer type\n");
  }
}

static var_list_t *set_var_types(attr_list_t *attrs, decl_spec_t *decl_spec, declarator_list_t *decls)
{
  declarator_t *decl, *next;
  var_list_t *var_list = NULL;

  LIST_FOR_EACH_ENTRY_SAFE( decl, next, decls, declarator_t, entry )
  {
    var_t *var = decl->var;

    var->attrs = attrs;
    set_type(var, decl_spec, decl, 0);
    var_list = append_var(var_list, var);
    free(decl);
  }
  return var_list;
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

static var_list_t *append_var_list(var_list_t *list, var_list_t *vars)
{
    if (!vars) return list;
    if (!list)
    {
        list = xmalloc( sizeof(*list) );
        list_init( list );
    }
    list_move_tail( list, vars );
    return list;
}

static var_t *make_var(char *name)
{
  var_t *v = xmalloc(sizeof(var_t));
  v->name = name;
  v->type = NULL;
  v->attrs = NULL;
  v->eval = NULL;
  v->stgclass = STG_NONE;
  v->loc_info.input_name = input_name ? input_name : "stdin";
  v->loc_info.line_number = line_number;
  v->loc_info.near_text = parser_text;
  return v;
}

static declarator_list_t *append_declarator(declarator_list_t *list, declarator_t *d)
{
  if (!d) return list;
  if (!list) {
    list = xmalloc(sizeof(*list));
    list_init(list);
  }
  list_add_tail(list, &d->entry);
  return list;
}

static declarator_t *make_declarator(var_t *var)
{
  declarator_t *d = xmalloc(sizeof(*d));
  d->var = var;
  d->type = NULL;
  d->func_type = NULL;
  d->array = NULL;
  return d;
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

static func_t *make_func(var_t *def)
{
  func_t *f = xmalloc(sizeof(func_t));
  f->def = def;
  f->args = def->type->fields_or_args;
  f->ignore = parse_only;
  f->idx = -1;
  return f;
}

static type_t *make_class(char *name)
{
  type_t *c = make_type(RPC_FC_COCLASS, NULL);
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

static typelib_t *make_library(const char *name, const attr_list_t *attrs)
{
    typelib_t *typelib = xmalloc(sizeof(*typelib));
    typelib->name = xstrdup(name);
    typelib->filename = NULL;
    typelib->attrs = attrs;
    list_init( &typelib->entries );
    list_init( &typelib->importlibs );
    return typelib;
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
    t->fields_or_args = ot->fields_or_args;
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

static type_t *reg_typedefs(decl_spec_t *decl_spec, declarator_list_t *decls, attr_list_t *attrs)
{
  const declarator_t *decl;
  int is_str = is_attr(attrs, ATTR_STRING);
  type_t *type = decl_spec->type;

  if (is_str)
  {
    type_t *t = decl_spec->type;
    unsigned char c;

    while (is_ptr(t))
      t = t->ref;

    c = t->type;
    if (c != RPC_FC_CHAR && c != RPC_FC_BYTE && c != RPC_FC_WCHAR)
    {
      decl = LIST_ENTRY( list_head( decls ), const declarator_t, entry );
      error_loc("'%s': [string] attribute is only valid on 'char', 'byte', or 'wchar_t' pointers and arrays\n",
              decl->var->name);
    }
  }

  /* We must generate names for tagless enum, struct or union.
     Typedef-ing a tagless enum, struct or union means we want the typedef
     to be included in a library hence the public attribute.  */
  if ((type->kind == TKIND_ENUM || type->kind == TKIND_RECORD
       || type->kind == TKIND_UNION) && ! type->name && ! parse_only)
  {
    if (! is_attr(attrs, ATTR_PUBLIC))
      attrs = append_attr( attrs, make_attr(ATTR_PUBLIC) );
    type->name = gen_name();
  }
  else if (is_attr(attrs, ATTR_UUID) && !is_attr(attrs, ATTR_PUBLIC))
    attrs = append_attr( attrs, make_attr(ATTR_PUBLIC) );

  LIST_FOR_EACH_ENTRY( decl, decls, const declarator_t, entry )
  {
    var_t *name = decl->var;

    if (name->name) {
      type_t *cur;

      /* set the attributes to allow set_type to do some checks on them */
      name->attrs = attrs;
      set_type(name, decl_spec, decl, 0);
      cur = alias(name->type, name->name);
      cur->attrs = attrs;

      if (is_incomplete(cur))
        add_incomplete(cur);
      reg_type(cur, cur->name, 0);
    }
  }
  return type;
}

type_t *find_type(const char *name, int t)
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

var_t *find_const(const char *name, int f)
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

static void write_libid(const typelib_t *typelib)
{
  const UUID *uuid = get_attrp(typelib->attrs, ATTR_UUID);
  write_guid(idfile, "LIBID", typelib->name, uuid);
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

struct allowed_attr
{
    unsigned int dce_compatible : 1;
    unsigned int acf : 1;
    unsigned int on_interface : 1;
    unsigned int on_function : 1;
    unsigned int on_arg : 1;
    unsigned int on_type : 1;
    unsigned int on_field : 1;
    unsigned int on_library : 1;
    unsigned int on_dispinterface : 1;
    unsigned int on_module : 1;
    unsigned int on_coclass : 1;
    const char *display_name;
};

struct allowed_attr allowed_attr[] =
{
    /* attr                     { D ACF I Fn ARG T Fi  L  DI M  C  <display name> } */
    /* ATTR_AGGREGATABLE */     { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, "aggregatable" },
    /* ATTR_APPOBJECT */        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, "appobject" },
    /* ATTR_ASYNC */            { 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, "async" },
    /* ATTR_AUTO_HANDLE */      { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, "auto_handle" },
    /* ATTR_BINDABLE */         { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, "bindable" },
    /* ATTR_BROADCAST */        { 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, "broadcast" },
    /* ATTR_CALLAS */           { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, "call_as" },
    /* ATTR_CALLCONV */         { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, NULL },
    /* ATTR_CASE */             { 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, "case" },
    /* ATTR_CONST */            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "const" },
    /* ATTR_CONTEXTHANDLE */    { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, "context_handle" },
    /* ATTR_CONTROL */          { 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, "control" },
    /* ATTR_DEFAULT */          { 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, "default" },
    /* ATTR_DEFAULTCOLLELEM */  { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, "defaultcollelem" },
    /* ATTR_DEFAULTVALUE */     { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, "defaultvalue" },
    /* ATTR_DEFAULTVTABLE */    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, "defaultvtable" },
    /* ATTR_DISPINTERFACE */    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL },
    /* ATTR_DISPLAYBIND */      { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, "displaybind" },
    /* ATTR_DLLNAME */          { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, "dllname" },
    /* ATTR_DUAL */             { 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, "dual" },
    /* ATTR_ENDPOINT */         { 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, "endpoint" },
    /* ATTR_ENTRY */            { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, "entry" },
    /* ATTR_EXPLICIT_HANDLE */  { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, "explicit_handle" },
    /* ATTR_HANDLE */           { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, "handle" },
    /* ATTR_HELPCONTEXT */      { 0, 0, 1, 1, 0, 1, 0, 1, 1, 1, 1, "helpcontext" },
    /* ATTR_HELPFILE */         { 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, "helpfile" },
    /* ATTR_HELPSTRING */       { 0, 0, 1, 1, 0, 1, 0, 1, 1, 1, 1, "helpstring" },
    /* ATTR_HELPSTRINGCONTEXT */ { 0, 0, 1, 1, 0, 1, 0, 1, 1, 1, 1, "helpstringcontext" },
    /* ATTR_HELPSTRINGDLL */    { 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, "helpstringdll" },
    /* ATTR_HIDDEN */           { 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 1, "hidden" },
    /* ATTR_ID */               { 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, "id" },
    /* ATTR_IDEMPOTENT */       { 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, "idempotent" },
    /* ATTR_IIDIS */            { 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, "iid_is" },
    /* ATTR_IMMEDIATEBIND */    { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, "immediatebind" },
    /* ATTR_IMPLICIT_HANDLE */  { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, "implicit_handle" },
    /* ATTR_IN */               { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, "in" },
    /* ATTR_INLINE */           { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "inline" },
    /* ATTR_INPUTSYNC */        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "inputsync" },
    /* ATTR_LENGTHIS */         { 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, "length_is" },
    /* ATTR_LIBLCID */          { 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, "lcid" },
    /* ATTR_LOCAL */            { 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, "local" },
    /* ATTR_NONBROWSABLE */     { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, "nonbrowsable" },
    /* ATTR_NONCREATABLE */     { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, "noncreatable" },
    /* ATTR_NONEXTENSIBLE */    { 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, "nonextensible" },
    /* ATTR_OBJECT */           { 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, "object" },
    /* ATTR_ODL */              { 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, "odl" },
    /* ATTR_OLEAUTOMATION */    { 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, "oleautomation" },
    /* ATTR_OPTIONAL */         { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, "optional" },
    /* ATTR_OUT */              { 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, "out" },
    /* ATTR_POINTERDEFAULT */   { 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, "pointer_default" },
    /* ATTR_POINTERTYPE */      { 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, "ref, unique or ptr" },
    /* ATTR_PROPGET */          { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, "propget" },
    /* ATTR_PROPPUT */          { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, "propput" },
    /* ATTR_PROPPUTREF */       { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, "propputref" },
    /* ATTR_PUBLIC */           { 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, "public" },
    /* ATTR_RANGE */            { 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, "range" },
    /* ATTR_READONLY */         { 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, "readonly" },
    /* ATTR_REQUESTEDIT */      { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, "requestedit" },
    /* ATTR_RESTRICTED */       { 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 0, "restricted" },
    /* ATTR_RETVAL */           { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, "retval" },
    /* ATTR_SIZEIS */           { 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, "size_is" },
    /* ATTR_SOURCE */           { 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, "source" },
    /* ATTR_STRICTCONTEXTHANDLE */ { 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, "strict_context_handle" },
    /* ATTR_STRING */           { 1, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, "string" },
    /* ATTR_SWITCHIS */         { 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, "switch_is" },
    /* ATTR_SWITCHTYPE */       { 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, "switch_type" },
    /* ATTR_TRANSMITAS */       { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, "transmit_as" },
    /* ATTR_UUID */             { 1, 0, 1, 0, 0, 1, 0, 1, 1, 1, 1, "uuid" },
    /* ATTR_V1ENUM */           { 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, "v1_enum" },
    /* ATTR_VARARG */           { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, "vararg" },
    /* ATTR_VERSION */          { 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, "version" },
    /* ATTR_WIREMARSHAL */      { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, "wire_marshal" },
};

const char *get_attr_display_name(enum attr_type type)
{
    return allowed_attr[type].display_name;
}

static attr_list_t *check_iface_attrs(const char *name, attr_list_t *attrs)
{
  const attr_t *attr;
  if (!attrs) return attrs;
  LIST_FOR_EACH_ENTRY(attr, attrs, const attr_t, entry)
  {
    if (!allowed_attr[attr->type].on_interface)
      error_loc("inapplicable attribute %s for interface %s\n",
                allowed_attr[attr->type].display_name, name);
  }
  return attrs;
}

static attr_list_t *check_function_attrs(const char *name, attr_list_t *attrs)
{
  const attr_t *attr;
  if (!attrs) return attrs;
  LIST_FOR_EACH_ENTRY(attr, attrs, const attr_t, entry)
  {
    if (!allowed_attr[attr->type].on_function)
      error_loc("inapplicable attribute %s for function %s\n",
                allowed_attr[attr->type].display_name, name);
  }
  return attrs;
}

static void check_arg(var_t *arg)
{
  const type_t *t = arg->type;
  const attr_t *attr;

  if (t->type == 0 && ! is_var_ptr(arg))
    error_loc("argument '%s' has void type\n", arg->name);

  if (arg->attrs)
  {
    LIST_FOR_EACH_ENTRY(attr, arg->attrs, const attr_t, entry)
    {
      if (!allowed_attr[attr->type].on_arg)
        error_loc("inapplicable attribute %s for argument %s\n",
                  allowed_attr[attr->type].display_name, arg->name);
    }
  }
}

static attr_list_t *check_typedef_attrs(attr_list_t *attrs)
{
  const attr_t *attr;
  if (!attrs) return attrs;
  LIST_FOR_EACH_ENTRY(attr, attrs, const attr_t, entry)
  {
    if (!allowed_attr[attr->type].on_type)
      error_loc("inapplicable attribute %s for typedef\n",
                allowed_attr[attr->type].display_name);
  }
  return attrs;
}

static attr_list_t *check_field_attrs(const char *name, attr_list_t *attrs)
{
  const attr_t *attr;
  if (!attrs) return attrs;
  LIST_FOR_EACH_ENTRY(attr, attrs, const attr_t, entry)
  {
    if (!allowed_attr[attr->type].on_field)
      error_loc("inapplicable attribute %s for field %s\n",
                allowed_attr[attr->type].display_name, name);
  }
  return attrs;
}

static attr_list_t *check_library_attrs(const char *name, attr_list_t *attrs)
{
  const attr_t *attr;
  if (!attrs) return attrs;
  LIST_FOR_EACH_ENTRY(attr, attrs, const attr_t, entry)
  {
    if (!allowed_attr[attr->type].on_library)
      error_loc("inapplicable attribute %s for library %s\n",
                allowed_attr[attr->type].display_name, name);
  }
  return attrs;
}

static attr_list_t *check_dispiface_attrs(const char *name, attr_list_t *attrs)
{
  const attr_t *attr;
  if (!attrs) return attrs;
  LIST_FOR_EACH_ENTRY(attr, attrs, const attr_t, entry)
  {
    if (!allowed_attr[attr->type].on_dispinterface)
      error_loc("inapplicable attribute %s for dispinterface %s\n",
                allowed_attr[attr->type].display_name, name);
  }
  return attrs;
}

static attr_list_t *check_module_attrs(const char *name, attr_list_t *attrs)
{
  const attr_t *attr;
  if (!attrs) return attrs;
  LIST_FOR_EACH_ENTRY(attr, attrs, const attr_t, entry)
  {
    if (!allowed_attr[attr->type].on_module)
      error_loc("inapplicable attribute %s for module %s\n",
                allowed_attr[attr->type].display_name, name);
  }
  return attrs;
}

static attr_list_t *check_coclass_attrs(const char *name, attr_list_t *attrs)
{
  const attr_t *attr;
  if (!attrs) return attrs;
  LIST_FOR_EACH_ENTRY(attr, attrs, const attr_t, entry)
  {
    if (!allowed_attr[attr->type].on_coclass)
      error_loc("inapplicable attribute %s for coclass %s\n",
                allowed_attr[attr->type].display_name, name);
  }
  return attrs;
}

static int is_allowed_conf_type(const type_t *type)
{
    switch (type->type)
    {
    case RPC_FC_CHAR:
    case RPC_FC_SMALL:
    case RPC_FC_BYTE:
    case RPC_FC_USMALL:
    case RPC_FC_WCHAR:
    case RPC_FC_SHORT:
    case RPC_FC_ENUM16:
    case RPC_FC_USHORT:
    case RPC_FC_LONG:
    case RPC_FC_ENUM32:
    case RPC_FC_ULONG:
        return TRUE;
    default:
        return FALSE;
    }
}

static int is_ptr_guid_type(const type_t *type)
{
    unsigned int align = 0;
    for (;;)
    {
        if (type->kind == TKIND_ALIAS)
            type = type->orig;
        else if (is_ptr(type))
        {
            type = type->ref;
            break;
        }
        else
            return FALSE;
    }
    return (type_memsize(type, &align) == 16);
}

static void check_conformance_expr_list(const char *attr_name, const var_t *arg, const type_t *container_type, expr_list_t *expr_list)
{
    expr_t *dim;
    struct expr_loc expr_loc;
    expr_loc.v = arg;
    expr_loc.attr = attr_name;
    if (expr_list) LIST_FOR_EACH_ENTRY(dim, expr_list, expr_t, entry)
    {
        if (dim->type != EXPR_VOID)
        {
            const type_t *expr_type = expr_resolve_type(&expr_loc, container_type, dim);
            if (!is_allowed_conf_type(expr_type))
                error_loc_info(&arg->loc_info, "expression must resolve to integral type <= 32bits for attribute %s\n",
                               attr_name);
        }
    }
}

static void check_remoting_fields(const var_t *var, type_t *type);

/* checks that properties common to fields and arguments are consistent */
static void check_field_common(const type_t *container_type,
                               const char *container_name, const var_t *arg)
{
    type_t *type = arg->type;
    int is_wire_marshal = 0;
    int is_context_handle = 0;
    const char *container_type_name = NULL;

    if (is_struct(container_type->type))
        container_type_name = "struct";
    else if (is_union(container_type->type))
        container_type_name = "union";
    else if (container_type->type == RPC_FC_FUNCTION)
        container_type_name = "function";

    if (is_attr(arg->attrs, ATTR_LENGTHIS) &&
        (is_attr(arg->attrs, ATTR_STRING) || is_aliaschain_attr(arg->type, ATTR_STRING)))
        error_loc_info(&arg->loc_info,
                       "string and length_is specified for argument %s are mutually exclusive attributes\n",
                       arg->name);

    if (is_attr(arg->attrs, ATTR_SIZEIS))
    {
        expr_list_t *size_is_exprs = get_attrp(arg->attrs, ATTR_SIZEIS);
        check_conformance_expr_list("size_is", arg, container_type, size_is_exprs);
    }
    if (is_attr(arg->attrs, ATTR_LENGTHIS))
    {
        expr_list_t *length_is_exprs = get_attrp(arg->attrs, ATTR_LENGTHIS);
        check_conformance_expr_list("length_is", arg, container_type, length_is_exprs);
    }
    if (is_attr(arg->attrs, ATTR_IIDIS))
    {
        struct expr_loc expr_loc;
        expr_t *expr = get_attrp(arg->attrs, ATTR_IIDIS);
        if (expr->type != EXPR_VOID)
        {
            const type_t *expr_type;
            expr_loc.v = arg;
            expr_loc.attr = "iid_is";
            expr_type = expr_resolve_type(&expr_loc, container_type, expr);
            if (!expr_type || !is_ptr_guid_type(expr_type))
                error_loc_info(&arg->loc_info, "expression must resolve to pointer to GUID type for attribute iid_is\n");
        }
    }
    if (is_attr(arg->attrs, ATTR_SWITCHIS))
    {
        struct expr_loc expr_loc;
        expr_t *expr = get_attrp(arg->attrs, ATTR_SWITCHIS);
        if (expr->type != EXPR_VOID)
        {
            const type_t *expr_type;
            expr_loc.v = arg;
            expr_loc.attr = "switch_is";
            expr_type = expr_resolve_type(&expr_loc, container_type, expr);
            if (!expr_type || !is_allowed_conf_type(expr_type))
                error_loc_info(&arg->loc_info, "expression must resolve to integral type <= 32bits for attribute %s\n",
                               expr_loc.attr);
        }
    }

    /* get fundamental type for the argument */
    for (;;)
    {
        if (is_attr(type->attrs, ATTR_WIREMARSHAL))
        {
            is_wire_marshal = 1;
            break;
        }
        if (is_attr(type->attrs, ATTR_CONTEXTHANDLE))
        {
            is_context_handle = 1;
            break;
        }
        if (type->kind == TKIND_ALIAS)
            type = type->orig;
        else if (is_ptr(type) || is_array(type))
            type = type->ref;
        else
            break;
    }

    if (type->type == 0 && !is_attr(arg->attrs, ATTR_IIDIS) && !is_wire_marshal && !is_context_handle)
        error_loc_info(&arg->loc_info, "parameter \'%s\' of %s \'%s\' cannot derive from void *\n", arg->name, container_type_name, container_name);
    else if (type->type == RPC_FC_FUNCTION)
        error_loc_info(&arg->loc_info, "parameter \'%s\' of %s \'%s\' cannot be a function pointer\n", arg->name, container_type_name, container_name);
    else if (!is_wire_marshal && (is_struct(type->type) || is_union(type->type)))
        check_remoting_fields(arg, type);
}

static void check_remoting_fields(const var_t *var, type_t *type)
{
    const var_t *field;
    const var_list_t *fields = NULL;

    if (type->checked)
        return;

    type->checked = TRUE;

    if (is_struct(type->type))
        fields = type->fields_or_args;
    else if (is_union(type->type))
    {
        if (type->type == RPC_FC_ENCAPSULATED_UNION)
        {
            const var_t *uv = LIST_ENTRY(list_tail(type->fields_or_args), const var_t, entry);
            fields = uv->type->fields_or_args;
        }
        else
            fields = type->fields_or_args;
    }

    if (fields) LIST_FOR_EACH_ENTRY( field, type->fields_or_args, const var_t, entry )
        if (field->type) check_field_common(type, type->name, field);
}

/* checks that arguments for a function make sense for marshalling and unmarshalling */
static void check_remoting_args(const func_t *func)
{
    const char *funcname = func->def->name;
    const var_t *arg;

    if (func->args) LIST_FOR_EACH_ENTRY( arg, func->args, const var_t, entry )
    {
        int ptr_level = 0;
        const type_t *type = arg->type;

        /* get pointer level and fundamental type for the argument */
        for (;;)
        {
            if (is_attr(type->attrs, ATTR_WIREMARSHAL))
                break;
            if (is_attr(type->attrs, ATTR_CONTEXTHANDLE))
                break;
            if (type->kind == TKIND_ALIAS)
                type = type->orig;
            else if (is_ptr(type))
            {
                ptr_level++;
                type = type->ref;
            }
            else
                break;
        }

        /* check that [out] parameters have enough pointer levels */
        if (is_attr(arg->attrs, ATTR_OUT))
        {
            if (!is_array(type))
            {
                if (!ptr_level)
                    error_loc_info(&arg->loc_info, "out parameter \'%s\' of function \'%s\' is not a pointer\n", arg->name, funcname);
                if (type->type == RPC_FC_IP && ptr_level == 1)
                    error_loc_info(&arg->loc_info, "out interface pointer \'%s\' of function \'%s\' is not a double pointer\n", arg->name, funcname);
            }
        }

        check_field_common(func->def->type, funcname, arg);
    }
}

static void add_explicit_handle_if_necessary(func_t *func)
{
    const var_t* explicit_handle_var;
    const var_t* explicit_generic_handle_var = NULL;
    const var_t* context_handle_var = NULL;

    /* check for a defined binding handle */
    explicit_handle_var = get_explicit_handle_var(func);
    if (!explicit_handle_var)
    {
        explicit_generic_handle_var = get_explicit_generic_handle_var(func);
        if (!explicit_generic_handle_var)
        {
            context_handle_var = get_context_handle_var(func);
            if (!context_handle_var)
            {
                /* no explicit handle specified so add
                 * "[in] handle_t IDL_handle" as the first parameter to the
                 * function */
                var_t *idl_handle = make_var(xstrdup("IDL_handle"));
                idl_handle->attrs = append_attr(NULL, make_attr(ATTR_IN));
                idl_handle->type = find_type("handle_t", 0);
                if (!func->def->type->fields_or_args)
                {
                    func->def->type->fields_or_args = xmalloc( sizeof(*func->def->type->fields_or_args) );
                    list_init( func->def->type->fields_or_args );
                }
                list_add_head( func->def->type->fields_or_args, &idl_handle->entry );
                func->args = func->def->type->fields_or_args;
            }
        }
    }
}

static void check_functions(const type_t *iface)
{
    if (is_attr(iface->attrs, ATTR_EXPLICIT_HANDLE) && iface->funcs)
    {
        func_t *func;
        LIST_FOR_EACH_ENTRY( func, iface->funcs, func_t, entry )
            add_explicit_handle_if_necessary(func);
    }
    if (!is_inside_library && !is_attr(iface->attrs, ATTR_LOCAL))
    {
        const func_t *func;
        if (iface->funcs) LIST_FOR_EACH_ENTRY( func, iface->funcs, const func_t, entry )
        {
            if (!is_attr(func->def->attrs, ATTR_LOCAL))
                check_remoting_args(func);
        }
    }
}

static void check_all_user_types(const statement_list_t *stmts)
{
  const statement_t *stmt;

  if (stmts) LIST_FOR_EACH_ENTRY(stmt, stmts, const statement_t, entry)
  {
    if (stmt->type == STMT_LIBRARY)
      check_all_user_types(stmt->u.lib->stmts);
    else if (stmt->type == STMT_TYPE && stmt->u.type->type == RPC_FC_IP)
    {
      const func_t *f;
      const func_list_t *fs = stmt->u.type->funcs;
      if (fs) LIST_FOR_EACH_ENTRY(f, fs, const func_t, entry)
        check_for_additional_prototype_types(f->args);
    }
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

static statement_t *make_statement(enum statement_type type)
{
    statement_t *stmt = xmalloc(sizeof(*stmt));
    stmt->type = type;
    return stmt;
}

static statement_t *make_statement_type_decl(type_t *type)
{
    statement_t *stmt = make_statement(STMT_TYPE);
    stmt->u.type = type;
    return stmt;
}

static statement_t *make_statement_reference(type_t *type)
{
    statement_t *stmt = make_statement(STMT_TYPEREF);
    stmt->u.type = type;
    return stmt;
}

static statement_t *make_statement_declaration(var_t *var)
{
    statement_t *stmt = make_statement(STMT_DECLARATION);
    stmt->u.var = var;
    if (var->stgclass == STG_EXTERN && var->eval)
        warning("'%s' initialised and declared extern\n", var->name);
    if (is_const_decl(var))
    {
        if (var->eval)
            reg_const(var);
    }
    else if ((var->stgclass == STG_NONE || var->stgclass == STG_REGISTER) &&
             var->type->type != RPC_FC_FUNCTION)
        error_loc("instantiation of data is illegal\n");
    return stmt;
}

static statement_t *make_statement_library(typelib_t *typelib)
{
    statement_t *stmt = make_statement(STMT_LIBRARY);
    stmt->u.lib = typelib;
    return stmt;
}

static statement_t *make_statement_cppquote(const char *str)
{
    statement_t *stmt = make_statement(STMT_CPPQUOTE);
    stmt->u.str = str;
    return stmt;
}

static statement_t *make_statement_importlib(const char *str)
{
    statement_t *stmt = make_statement(STMT_IMPORTLIB);
    stmt->u.str = str;
    return stmt;
}

static statement_t *make_statement_import(const char *str)
{
    statement_t *stmt = make_statement(STMT_IMPORT);
    stmt->u.str = str;
    return stmt;
}

static statement_t *make_statement_module(type_t *type)
{
    statement_t *stmt = make_statement(STMT_MODULE);
    stmt->u.type = type;
    return stmt;
}

static statement_t *process_typedefs(declarator_list_t *decls)
{
    declarator_t *decl, *next;
    statement_t *stmt;
    type_list_t **type_list;

    if (!decls) return NULL;

    stmt = make_statement(STMT_TYPEDEF);
    stmt->u.type_list = NULL;
    type_list = &stmt->u.type_list;

    LIST_FOR_EACH_ENTRY_SAFE( decl, next, decls, declarator_t, entry )
    {
        var_t *var = decl->var;
        type_t *type = find_type(var->name, 0);
        *type_list = xmalloc(sizeof(type_list_t));
        (*type_list)->type = type;
        (*type_list)->next = NULL;

        if (! parse_only && do_header)
            write_typedef(type);
        if (in_typelib && is_attr(type->attrs, ATTR_PUBLIC))
            add_typelib_entry(type);

        type_list = &(*type_list)->next;
        free(decl);
        free(var);
    }

    return stmt;
}

static statement_list_t *append_statement(statement_list_t *list, statement_t *stmt)
{
    if (!stmt) return list;
    if (!list)
    {
        list = xmalloc( sizeof(*list) );
        list_init( list );
    }
    list_add_tail( list, &stmt->entry );
    return list;
}

static func_list_t *append_func_from_statement(func_list_t *list, statement_t *stmt)
{
    if (stmt->type == STMT_DECLARATION)
    {
        var_t *var = stmt->u.var;
        if (var->stgclass == STG_NONE && var->type->type == RPC_FC_FUNCTION)
        {
            check_function_attrs(var->name, var->type->attrs);
            return append_func(list, make_func(stmt->u.var));
        }
    }
    return list;
}
