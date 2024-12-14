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

#include "widl.h"
#include "utils.h"
#include "parser.h"
#include "header.h"
#include "typelib.h"
#include "typegen.h"
#include "expr.h"
#include "typetree.h"

static unsigned char pointer_default = FC_UP;

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

static void fix_incomplete(void);
static void fix_incomplete_types(type_t *complete_type);

static str_list_t *append_str(str_list_t *list, char *str);
static attr_list_t *append_attr(attr_list_t *list, attr_t *attr);
static attr_list_t *append_attr_list(attr_list_t *new_list, attr_list_t *old_list);
static decl_spec_t *make_decl_spec(type_t *type, decl_spec_t *left, decl_spec_t *right, attr_t *attr, enum storage_class stgclass);
static attr_t *make_attr(enum attr_type type);
static attr_t *make_attrv(enum attr_type type, unsigned int val);
static attr_t *make_attrp(enum attr_type type, void *val);
static expr_list_t *append_expr(expr_list_t *list, expr_t *expr);
static type_t *append_array(type_t *chain, expr_t *expr);
static var_t *declare_var(attr_list_t *attrs, decl_spec_t *decl_spec, const declarator_t *decl, int top);
static var_list_t *set_var_types(attr_list_t *attrs, decl_spec_t *decl_spec, declarator_list_t *decls);
static ifref_list_t *append_ifref(ifref_list_t *list, ifref_t *iface);
static ifref_t *make_ifref(type_t *iface);
static var_list_t *append_var_list(var_list_t *list, var_list_t *vars);
static declarator_list_t *append_declarator(declarator_list_t *list, declarator_t *p);
static declarator_t *make_declarator(var_t *var);
static type_t *make_safearray(type_t *type);
static typelib_t *make_library(const char *name, const attr_list_t *attrs);
static type_t *append_chain_type(type_t *chain, type_t *type);
static warning_list_t *append_warning(warning_list_t *, int);

static type_t *reg_typedefs(decl_spec_t *decl_spec, var_list_t *names, attr_list_t *attrs);
static type_t *find_type_or_error(const char *name, int t);
static type_t *find_type_or_error2(char *name, int t);

static var_t *reg_const(var_t *var);

static void push_namespace(const char *name);
static void pop_namespace(const char *name);

static char *gen_name(void);
static void check_arg_attrs(const var_t *arg);
static void check_statements(const statement_list_t *stmts, int is_inside_library);
static void check_all_user_types(const statement_list_t *stmts);
static attr_list_t *check_iface_attrs(const char *name, attr_list_t *attrs);
static attr_list_t *check_function_attrs(const char *name, attr_list_t *attrs);
static attr_list_t *check_typedef_attrs(attr_list_t *attrs);
static attr_list_t *check_enum_attrs(attr_list_t *attrs);
static attr_list_t *check_struct_attrs(attr_list_t *attrs);
static attr_list_t *check_union_attrs(attr_list_t *attrs);
static attr_list_t *check_field_attrs(const char *name, attr_list_t *attrs);
static attr_list_t *check_library_attrs(const char *name, attr_list_t *attrs);
static attr_list_t *check_dispiface_attrs(const char *name, attr_list_t *attrs);
static attr_list_t *check_module_attrs(const char *name, attr_list_t *attrs);
static attr_list_t *check_coclass_attrs(const char *name, attr_list_t *attrs);
const char *get_attr_display_name(enum attr_type type);
static void add_explicit_handle_if_necessary(const type_t *iface, var_t *func);
static void check_def(const type_t *t);

static void check_async_uuid(type_t *iface);

static statement_t *make_statement(enum statement_type type);
static statement_t *make_statement_type_decl(type_t *type);
static statement_t *make_statement_reference(type_t *type);
static statement_t *make_statement_declaration(var_t *var);
static statement_t *make_statement_library(typelib_t *typelib);
static statement_t *make_statement_pragma(const char *str);
static statement_t *make_statement_cppquote(const char *str);
static statement_t *make_statement_importlib(const char *str);
static statement_t *make_statement_module(type_t *type);
static statement_t *make_statement_typedef(var_list_t *names);
static statement_t *make_statement_import(const char *str);
static statement_list_t *append_statement(statement_list_t *list, statement_t *stmt);
static statement_list_t *append_statements(statement_list_t *, statement_list_t *);
static attr_list_t *append_attribs(attr_list_t *, attr_list_t *);

static struct namespace global_namespace = {
    NULL, NULL, LIST_INIT(global_namespace.entry), LIST_INIT(global_namespace.children)
};

static struct namespace *current_namespace = &global_namespace;

static typelib_t *current_typelib;

%}
%union {
	attr_t *attr;
	attr_list_t *attr_list;
	str_list_t *str_list;
	expr_t *expr;
	expr_list_t *expr_list;
	type_t *type;
	var_t *var;
	var_list_t *var_list;
	declarator_t *declarator;
	declarator_list_t *declarator_list;
	statement_t *statement;
	statement_list_t *stmt_list;
	warning_t *warning;
	warning_list_t *warning_list;
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

%token <str> aIDENTIFIER aPRAGMA
%token <str> aKNOWNTYPE
%token <num> aNUM aHEXNUM
%token <dbl> aDOUBLE
%token <str> aSTRING aWSTRING aSQSTRING
%token <uuid> aUUID
%token aEOF aACF
%token SHL SHR
%token MEMBERPTR
%token EQUALITY INEQUALITY
%token GREATEREQUAL LESSEQUAL
%token LOGICALOR LOGICALAND
%token ELLIPSIS
%token tAGGREGATABLE tALLNODES tALLOCATE tANNOTATION tAPPOBJECT tASYNC tASYNCUUID
%token tAUTOHANDLE tBINDABLE tBOOLEAN tBROADCAST tBYTE tBYTECOUNT
%token tCALLAS tCALLBACK tCASE tCDECL tCHAR tCOCLASS tCODE tCOMMSTATUS
%token tCONST tCONTEXTHANDLE tCONTEXTHANDLENOSERIALIZE
%token tCONTEXTHANDLESERIALIZE tCONTROL tCPPQUOTE
%token tDECODE tDEFAULT tDEFAULTBIND
%token tDEFAULTCOLLELEM
%token tDEFAULTVALUE
%token tDEFAULTVTABLE
%token tDISABLECONSISTENCYCHECK tDISPLAYBIND
%token tDISPINTERFACE
%token tDLLNAME tDONTFREE tDOUBLE tDUAL
%token tENABLEALLOCATE tENCODE tENDPOINT
%token tENTRY tENUM tERRORSTATUST
%token tEXPLICITHANDLE tEXTERN
%token tFALSE
%token tFASTCALL tFAULTSTATUS
%token tFLOAT tFORCEALLOCATE
%token tHANDLE
%token tHANDLET
%token tHELPCONTEXT tHELPFILE
%token tHELPSTRING tHELPSTRINGCONTEXT tHELPSTRINGDLL
%token tHIDDEN
%token tHYPER tID tIDEMPOTENT
%token tIGNORE tIIDIS
%token tIMMEDIATEBIND
%token tIMPLICITHANDLE
%token tIMPORT tIMPORTLIB
%token tIN tIN_LINE tINLINE
%token tINPUTSYNC
%token tINT tINT32 tINT3264 tINT64
%token tINTERFACE
%token tLCID
%token tLENGTHIS tLIBRARY
%token tLICENSED tLOCAL
%token tLONG
%token tMAYBE tMESSAGE
%token tMETHODS
%token tMODULE
%token tNAMESPACE
%token tNOCODE tNONBROWSABLE
%token tNONCREATABLE
%token tNONEXTENSIBLE
%token tNOTIFY tNOTIFYFLAG
%token tNULL
%token tOBJECT tODL tOLEAUTOMATION
%token tOPTIMIZE tOPTIONAL
%token tOUT
%token tPARTIALIGNORE tPASCAL
%token tPOINTERDEFAULT
%token tPRAGMA_WARNING
%token tPROGID tPROPERTIES
%token tPROPGET tPROPPUT tPROPPUTREF
%token tPROXY tPTR
%token tPUBLIC
%token tRANGE
%token tREADONLY tREF
%token tREGISTER tREPRESENTAS
%token tREQUESTEDIT
%token tRESTRICTED
%token tRETVAL
%token tSAFEARRAY
%token tSHORT
%token tSIGNED tSINGLENODE
%token tSIZEIS tSIZEOF
%token tSMALL
%token tSOURCE
%token tSTATIC
%token tSTDCALL
%token tSTRICTCONTEXTHANDLE
%token tSTRING tSTRUCT
%token tSWITCH tSWITCHIS tSWITCHTYPE
%token tTHREADING tTRANSMITAS
%token tTRUE
%token tTYPEDEF
%token tUIDEFAULT tUNION
%token tUNIQUE
%token tUNSIGNED
%token tUSESGETLASTERROR tUSERMARSHAL tUUID
%token tV1ENUM
%token tVARARG
%token tVERSION tVIPROGID
%token tVOID
%token tWCHAR tWIREMARSHAL
%token tAPARTMENT tNEUTRAL tSINGLE tFREE tBOTH

%type <attr> attribute type_qualifier function_specifier acf_attribute
%type <attr_list> m_attributes attributes attrib_list m_type_qual_list
%type <attr_list> acf_attributes acf_attribute_list
%type <str_list> str_list
%type <expr> m_expr expr expr_const expr_int_const array m_bitfield
%type <expr_list> m_exprs /* exprs expr_list */ expr_list_int_const
%type <ifinfo> interfacehdr
%type <stgclass> storage_cls_spec
%type <declspec> decl_spec decl_spec_no_type m_decl_spec_no_type
%type <type> inherit interface interfacedef interfacedec
%type <type> dispinterface dispinterfacehdr dispinterfacedef
%type <type> module modulehdr moduledef
%type <str> namespacedef
%type <type> base_type int_std
%type <type> enumdef structdef uniondef typedecl
%type <type> type
%type <ifref> coclass_int
%type <ifref_list> coclass_ints
%type <var> arg ne_union_field union_field s_field case enum declaration
%type <var> funcdef
%type <var_list> m_args arg_list args dispint_meths
%type <var_list> fields ne_union_fields cases enums enum_list dispint_props field
%type <var> m_ident ident
%type <declarator> declarator direct_declarator init_declarator struct_declarator
%type <declarator> m_any_declarator any_declarator any_declarator_no_direct any_direct_declarator
%type <declarator> m_abstract_declarator abstract_declarator abstract_declarator_no_direct abstract_direct_declarator
%type <declarator_list> declarator_list struct_declarator_list
%type <type> coclass coclasshdr coclassdef
%type <num> pointer_type threading_type version
%type <str> libraryhdr callconv cppquote importlib import t_ident
%type <uuid> uuid_string
%type <import> import_start
%type <typelib> library_start librarydef
%type <statement> statement typedef pragma_warning
%type <stmt_list> gbl_statements imp_statements int_statements
%type <warning_list> warnings
%type <num> allocate_option_list allocate_option

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

%define parse.error verbose

%%

input: gbl_statements m_acf			{ fix_incomplete();
						  check_statements($1, FALSE);
						  check_all_user_types($1);
						  write_header($1);
						  write_id_data($1);
						  write_proxies($1);
						  write_client($1);
						  write_server($1);
						  write_regscript($1);
#ifndef __REACTOS__
						  write_typelib_regscript($1);
#endif
						  write_dlldata($1);
						  write_local_stubs($1);
						}
	;

m_acf: /* empty */ | aACF acf_statements

gbl_statements:					{ $$ = NULL; }
	| gbl_statements namespacedef '{' { push_namespace($2); } gbl_statements '}'
						{ pop_namespace($2); $$ = append_statements($1, $5); }
	| gbl_statements interfacedec		{ $$ = append_statement($1, make_statement_reference($2)); }
	| gbl_statements interfacedef		{ $$ = append_statement($1, make_statement_type_decl($2)); }
	| gbl_statements coclass ';'		{ $$ = $1;
						  reg_type($2, $2->name, current_namespace, 0);
						}
	| gbl_statements coclassdef		{ $$ = append_statement($1, make_statement_type_decl($2));
						  reg_type($2, $2->name, current_namespace, 0);
						}
	| gbl_statements moduledef		{ $$ = append_statement($1, make_statement_module($2)); }
	| gbl_statements librarydef		{ $$ = append_statement($1, make_statement_library($2)); }
	| gbl_statements statement		{ $$ = append_statement($1, $2); }
	;

imp_statements:					{ $$ = NULL; }
	| imp_statements interfacedec		{ $$ = append_statement($1, make_statement_reference($2)); }
	| imp_statements namespacedef '{' { push_namespace($2); } imp_statements '}'
						{ pop_namespace($2); $$ = append_statements($1, $5); }
	| imp_statements interfacedef		{ $$ = append_statement($1, make_statement_type_decl($2)); }
	| imp_statements coclass ';'		{ $$ = $1; reg_type($2, $2->name, current_namespace, 0); }
	| imp_statements coclassdef		{ $$ = append_statement($1, make_statement_type_decl($2));
						  reg_type($2, $2->name, current_namespace, 0);
						}
	| imp_statements moduledef		{ $$ = append_statement($1, make_statement_module($2)); }
	| imp_statements statement		{ $$ = append_statement($1, $2); }
	| imp_statements importlib		{ $$ = append_statement($1, make_statement_importlib($2)); }
	| imp_statements librarydef		{ $$ = append_statement($1, make_statement_library($2)); }
	;

int_statements:					{ $$ = NULL; }
	| int_statements statement		{ $$ = append_statement($1, $2); }
	;

semicolon_opt:
	| ';'
	;

statement:
	  cppquote				{ $$ = make_statement_cppquote($1); }
	| typedecl ';'				{ $$ = make_statement_type_decl($1); }
	| declaration ';'			{ $$ = make_statement_declaration($1); }
	| import				{ $$ = make_statement_import($1); }
	| typedef ';'				{ $$ = $1; }
	| aPRAGMA				{ $$ = make_statement_pragma($1); }
	| pragma_warning { $$ = NULL; }
	;

pragma_warning: tPRAGMA_WARNING '(' aIDENTIFIER ':' warnings ')'
                  {
                      int result;
                      $$ = NULL;
                      result = do_warning($3, $5);
                      if(!result)
                          error_loc("expected \"disable\" or \"enable\"\n");
                  }
	;

warnings:
	  aNUM { $$ = append_warning(NULL, $1); }
	| warnings aNUM { $$ = append_warning($1, $2); }
	;

typedecl:
	  enumdef
	| tENUM aIDENTIFIER                     { $$ = type_new_enum($2, current_namespace, FALSE, NULL); }
	| structdef
	| tSTRUCT aIDENTIFIER                   { $$ = type_new_struct($2, current_namespace, FALSE, NULL); }
	| uniondef
	| tUNION aIDENTIFIER                    { $$ = type_new_nonencapsulated_union($2, FALSE, NULL); }
	| attributes enumdef                    { $$ = $2; $$->attrs = check_enum_attrs($1); }
	| attributes structdef                  { $$ = $2; $$->attrs = check_struct_attrs($1); }
	| attributes uniondef                   { $$ = $2; $$->attrs = check_union_attrs($1); }
	;

cppquote: tCPPQUOTE '(' aSTRING ')'		{ $$ = $3; }
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
						}
	;

importlib: tIMPORTLIB '(' aSTRING ')'
/* ifdef __REACTOS__ */
	   semicolon_opt			{ $$ = $3; if(!parse_only) add_importlib($3); }
/* else
	   semicolon_opt			{ $$ = $3; if(!parse_only) add_importlib($3, current_typelib); }
*/
	;

libraryhdr: tLIBRARY aIDENTIFIER		{ $$ = $2; }
	|   tLIBRARY aKNOWNTYPE			{ $$ = $2; }
	;
library_start: attributes libraryhdr '{'	{ $$ = make_library($2, check_library_attrs($2, $1));
/* ifdef __REACTOS__ */
						  if (!parse_only) start_typelib($$);
/* else
						  if (!parse_only && do_typelib) current_typelib = $$;
*/
						}
	;
librarydef: library_start imp_statements '}'
/* ifdef __REACTOS__ */
	    semicolon_opt			{ $$ = $1;
						  $$->stmts = $2;
						  if (!parse_only) end_typelib();
						}
/* else
	    semicolon_opt			{ $$ = $1; $$->stmts = $2; }
*/
	;

m_args:						{ $$ = NULL; }
	| args
	;

arg_list: arg					{ check_arg_attrs($1); $$ = append_var( NULL, $1 ); }
	| arg_list ',' arg			{ check_arg_attrs($3); $$ = append_var( $1, $3 ); }
	;

args:	  arg_list
	| arg_list ',' ELLIPSIS			{ $$ = append_var( $1, make_var(strdup("...")) ); }
	;

/* split into two rules to get bison to resolve a tVOID conflict */
arg:	  attributes decl_spec m_any_declarator	{ if ($2->stgclass != STG_NONE && $2->stgclass != STG_REGISTER)
						    error_loc("invalid storage class for function parameter\n");
						  $$ = declare_var($1, $2, $3, TRUE);
						  free($2); free($3);
						}
	| decl_spec m_any_declarator		{ if ($1->stgclass != STG_NONE && $1->stgclass != STG_REGISTER)
						    error_loc("invalid storage class for function parameter\n");
						  $$ = declare_var(NULL, $1, $2, TRUE);
						  free($1); free($2);
						}
	;

array:	  '[' expr ']'				{ $$ = $2;
						  if (!$$->is_const || $$->cval <= 0)
						      error_loc("array dimension is not a positive integer constant\n");
						}
	| '[' '*' ']'				{ $$ = make_expr(EXPR_VOID); }
	| '[' ']'				{ $$ = make_expr(EXPR_VOID); }
	;

m_attributes:					{ $$ = NULL; }
	| attributes
	;

attributes:
	  '[' attrib_list ']'			{ $$ = $2; }
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
	| tANNOTATION '(' aSTRING ')'		{ $$ = make_attrp(ATTR_ANNOTATION, $3); }
	| tAPPOBJECT				{ $$ = make_attr(ATTR_APPOBJECT); }
	| tASYNC				{ $$ = make_attr(ATTR_ASYNC); }
	| tAUTOHANDLE				{ $$ = make_attr(ATTR_AUTO_HANDLE); }
	| tBINDABLE				{ $$ = make_attr(ATTR_BINDABLE); }
	| tBROADCAST				{ $$ = make_attr(ATTR_BROADCAST); }
	| tCALLAS '(' ident ')'			{ $$ = make_attrp(ATTR_CALLAS, $3); }
	| tCASE '(' expr_list_int_const ')'	{ $$ = make_attrp(ATTR_CASE, $3); }
	| tCODE					{ $$ = make_attr(ATTR_CODE); }
	| tCOMMSTATUS				{ $$ = make_attr(ATTR_COMMSTATUS); }
	| tCONTEXTHANDLE			{ $$ = make_attrv(ATTR_CONTEXTHANDLE, 0); }
	| tCONTEXTHANDLENOSERIALIZE		{ $$ = make_attrv(ATTR_CONTEXTHANDLE, 0); /* RPC_CONTEXT_HANDLE_DONT_SERIALIZE */ }
	| tCONTEXTHANDLESERIALIZE		{ $$ = make_attrv(ATTR_CONTEXTHANDLE, 0); /* RPC_CONTEXT_HANDLE_SERIALIZE */ }
	| tCONTROL				{ $$ = make_attr(ATTR_CONTROL); }
	| tDECODE				{ $$ = make_attr(ATTR_DECODE); }
	| tDEFAULT				{ $$ = make_attr(ATTR_DEFAULT); }
	| tDEFAULTBIND				{ $$ = make_attr(ATTR_DEFAULTBIND); }
	| tDEFAULTCOLLELEM			{ $$ = make_attr(ATTR_DEFAULTCOLLELEM); }
	| tDEFAULTVALUE '(' expr_const ')'	{ $$ = make_attrp(ATTR_DEFAULTVALUE, $3); }
	| tDEFAULTVTABLE			{ $$ = make_attr(ATTR_DEFAULTVTABLE); }
	| tDISABLECONSISTENCYCHECK		{ $$ = make_attr(ATTR_DISABLECONSISTENCYCHECK); }
	| tDISPLAYBIND				{ $$ = make_attr(ATTR_DISPLAYBIND); }
	| tDLLNAME '(' aSTRING ')'		{ $$ = make_attrp(ATTR_DLLNAME, $3); }
	| tDUAL					{ $$ = make_attr(ATTR_DUAL); }
	| tENABLEALLOCATE			{ $$ = make_attr(ATTR_ENABLEALLOCATE); }
	| tENCODE				{ $$ = make_attr(ATTR_ENCODE); }
	| tENDPOINT '(' str_list ')'		{ $$ = make_attrp(ATTR_ENDPOINT, $3); }
	| tENTRY '(' expr_const ')'		{ $$ = make_attrp(ATTR_ENTRY, $3); }
	| tEXPLICITHANDLE			{ $$ = make_attr(ATTR_EXPLICIT_HANDLE); }
	| tFAULTSTATUS				{ $$ = make_attr(ATTR_FAULTSTATUS); }
	| tFORCEALLOCATE			{ $$ = make_attr(ATTR_FORCEALLOCATE); }
	| tHANDLE				{ $$ = make_attr(ATTR_HANDLE); }
	| tHELPCONTEXT '(' expr_int_const ')'	{ $$ = make_attrp(ATTR_HELPCONTEXT, $3); }
	| tHELPFILE '(' aSTRING ')'		{ $$ = make_attrp(ATTR_HELPFILE, $3); }
	| tHELPSTRING '(' aSTRING ')'		{ $$ = make_attrp(ATTR_HELPSTRING, $3); }
	| tHELPSTRINGCONTEXT '(' expr_int_const ')'	{ $$ = make_attrp(ATTR_HELPSTRINGCONTEXT, $3); }
	| tHELPSTRINGDLL '(' aSTRING ')'	{ $$ = make_attrp(ATTR_HELPSTRINGDLL, $3); }
	| tHIDDEN				{ $$ = make_attr(ATTR_HIDDEN); }
	| tID '(' expr_int_const ')'		{ $$ = make_attrp(ATTR_ID, $3); }
	| tIDEMPOTENT				{ $$ = make_attr(ATTR_IDEMPOTENT); }
	| tIGNORE				{ $$ = make_attr(ATTR_IGNORE); }
	| tIIDIS '(' expr ')'			{ $$ = make_attrp(ATTR_IIDIS, $3); }
	| tIMMEDIATEBIND			{ $$ = make_attr(ATTR_IMMEDIATEBIND); }
	| tIMPLICITHANDLE '(' arg ')'		{ $$ = make_attrp(ATTR_IMPLICIT_HANDLE, $3); }
	| tIN					{ $$ = make_attr(ATTR_IN); }
	| tINPUTSYNC				{ $$ = make_attr(ATTR_INPUTSYNC); }
	| tLENGTHIS '(' m_exprs ')'		{ $$ = make_attrp(ATTR_LENGTHIS, $3); }
	| tLCID	'(' expr_int_const ')'		{ $$ = make_attrp(ATTR_LIBLCID, $3); }
	| tLCID					{ $$ = make_attr(ATTR_PARAMLCID); }
	| tLICENSED				{ $$ = make_attr(ATTR_LICENSED); }
	| tLOCAL				{ $$ = make_attr(ATTR_LOCAL); }
	| tMAYBE				{ $$ = make_attr(ATTR_MAYBE); }
	| tMESSAGE				{ $$ = make_attr(ATTR_MESSAGE); }
	| tNOCODE				{ $$ = make_attr(ATTR_NOCODE); }
	| tNONBROWSABLE				{ $$ = make_attr(ATTR_NONBROWSABLE); }
	| tNONCREATABLE				{ $$ = make_attr(ATTR_NONCREATABLE); }
	| tNONEXTENSIBLE			{ $$ = make_attr(ATTR_NONEXTENSIBLE); }
	| tNOTIFY				{ $$ = make_attr(ATTR_NOTIFY); }
	| tNOTIFYFLAG				{ $$ = make_attr(ATTR_NOTIFYFLAG); }
	| tOBJECT				{ $$ = make_attr(ATTR_OBJECT); }
	| tODL					{ $$ = make_attr(ATTR_ODL); }
	| tOLEAUTOMATION			{ $$ = make_attr(ATTR_OLEAUTOMATION); }
	| tOPTIMIZE '(' aSTRING ')'		{ $$ = make_attrp(ATTR_OPTIMIZE, $3); }
	| tOPTIONAL                             { $$ = make_attr(ATTR_OPTIONAL); }
	| tOUT					{ $$ = make_attr(ATTR_OUT); }
	| tPARTIALIGNORE			{ $$ = make_attr(ATTR_PARTIALIGNORE); }
	| tPOINTERDEFAULT '(' pointer_type ')'	{ $$ = make_attrv(ATTR_POINTERDEFAULT, $3); }
	| tPROGID '(' aSTRING ')'		{ $$ = make_attrp(ATTR_PROGID, $3); }
	| tPROPGET				{ $$ = make_attr(ATTR_PROPGET); }
	| tPROPPUT				{ $$ = make_attr(ATTR_PROPPUT); }
	| tPROPPUTREF				{ $$ = make_attr(ATTR_PROPPUTREF); }
	| tPROXY				{ $$ = make_attr(ATTR_PROXY); }
	| tPUBLIC				{ $$ = make_attr(ATTR_PUBLIC); }
	| tRANGE '(' expr_int_const ',' expr_int_const ')'
						{ expr_list_t *list = append_expr( NULL, $3 );
						  list = append_expr( list, $5 );
						  $$ = make_attrp(ATTR_RANGE, list); }
	| tREADONLY				{ $$ = make_attr(ATTR_READONLY); }
	| tREPRESENTAS '(' type ')'		{ $$ = make_attrp(ATTR_REPRESENTAS, $3); }
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
	| tTHREADING '(' threading_type ')'	{ $$ = make_attrv(ATTR_THREADING, $3); }
	| tUIDEFAULT				{ $$ = make_attr(ATTR_UIDEFAULT); }
	| tUSESGETLASTERROR			{ $$ = make_attr(ATTR_USESGETLASTERROR); }
	| tUSERMARSHAL '(' type ')'		{ $$ = make_attrp(ATTR_USERMARSHAL, $3); }
	| tUUID '(' uuid_string ')'		{ $$ = make_attrp(ATTR_UUID, $3); }
	| tASYNCUUID '(' uuid_string ')'	{ $$ = make_attrp(ATTR_ASYNCUUID, $3); }
	| tV1ENUM				{ $$ = make_attr(ATTR_V1ENUM); }
	| tVARARG				{ $$ = make_attr(ATTR_VARARG); }
	| tVERSION '(' version ')'		{ $$ = make_attrv(ATTR_VERSION, $3); }
	| tVIPROGID '(' aSTRING ')'		{ $$ = make_attrp(ATTR_VIPROGID, $3); }
	| tWIREMARSHAL '(' type ')'		{ $$ = make_attrp(ATTR_WIREMARSHAL, $3); }
	| pointer_type				{ $$ = make_attrv(ATTR_POINTERTYPE, $1); }
	;

uuid_string:
	  aUUID
	| aSTRING				{ if (!is_valid_uuid($1))
						    error_loc("invalid UUID: %s\n", $1);
						  $$ = parse_uuid($1); }
        ;

callconv: tCDECL				{ $$ = xstrdup("__cdecl"); }
	| tFASTCALL				{ $$ = xstrdup("__fastcall"); }
	| tPASCAL				{ $$ = xstrdup("__pascal"); }
	| tSTDCALL				{ $$ = xstrdup("__stdcall"); }
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
                                                    enum expr_type type = EXPR_NUM;
                                                    if (last->eval->type == EXPR_HEXNUM) type = EXPR_HEXNUM;
                                                    if (last->eval->cval + 1 < 0) type = EXPR_HEXNUM;
                                                    $3->eval = make_exprl(type, last->eval->cval + 1);
                                                  }
                                                  $$ = append_var( $1, $3 );
						}
	;

enum:	  ident '=' expr_int_const		{ $$ = reg_const($1);
						  $$->eval = $3;
                                                  $$->type = type_new_int(TYPE_BASIC_INT, 0);
						}
	| ident					{ $$ = reg_const($1);
                                                  $$->type = type_new_int(TYPE_BASIC_INT, 0);
						}
	;

enumdef: tENUM t_ident '{' enums '}'		{ $$ = type_new_enum($2, current_namespace, TRUE, $4); }
	;

m_exprs:  m_expr                                { $$ = append_expr( NULL, $1 ); }
	| m_exprs ',' m_expr                    { $$ = append_expr( $1, $3 ); }
	;

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
	| aSQSTRING				{ $$ = make_exprs(EXPR_CHARCONST, $1); }
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
	| '(' decl_spec m_abstract_declarator ')' expr %prec CAST
						{ $$ = make_exprt(EXPR_CAST, declare_var(NULL, $2, $3, 0), $5); free($2); free($3); }
	| tSIZEOF '(' decl_spec m_abstract_declarator ')'
						{ $$ = make_exprt(EXPR_SIZEOF, declare_var(NULL, $3, $4, 0), NULL); free($3); free($4); }
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

field:	  m_attributes decl_spec struct_declarator_list ';'
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

s_field:  m_attributes decl_spec declarator	{ $$ = declare_var(check_field_attrs($3->var->name, $1),
						                $2, $3, FALSE);
						  free($3);
						}
	| m_attributes structdef		{ var_t *v = make_var(NULL);
						  v->type = $2; v->attrs = $1;
						  $$ = v;
						}
	;

funcdef: declaration				{ $$ = $1;
						  if (type_get_type($$->type) != TYPE_FUNCTION)
						    error_loc("only methods may be declared inside the methods section of a dispinterface\n");
						  check_function_attrs($$->name, $$->attrs);
						}
	;

declaration:
	  attributes decl_spec init_declarator
						{ $$ = declare_var($1, $2, $3, FALSE);
						  free($3);
						}
	| decl_spec init_declarator		{ $$ = declare_var(NULL, $1, $2, FALSE);
						  free($2);
						}
	;

m_ident:					{ $$ = NULL; }
	| ident
	;

t_ident:					{ $$ = NULL; }
	| aIDENTIFIER				{ $$ = $1; }
	| aKNOWNTYPE				{ $$ = $1; }
	;

ident:	  aIDENTIFIER				{ $$ = make_var($1); }
/* some "reserved words" used in attributes are also used as field names in some MS IDL files */
	| aKNOWNTYPE				{ $$ = make_var($<str>1); }
	;

base_type: tBYTE				{ $$ = find_type_or_error($<str>1, 0); }
	| tWCHAR				{ $$ = find_type_or_error($<str>1, 0); }
	| int_std
	| tSIGNED int_std			{ $$ = type_new_int(type_basic_get_type($2), -1); }
	| tUNSIGNED int_std			{ $$ = type_new_int(type_basic_get_type($2), 1); }
	| tUNSIGNED				{ $$ = type_new_int(TYPE_BASIC_INT, 1); }
	| tFLOAT				{ $$ = find_type_or_error($<str>1, 0); }
	| tDOUBLE				{ $$ = find_type_or_error($<str>1, 0); }
	| tBOOLEAN				{ $$ = find_type_or_error($<str>1, 0); }
	| tERRORSTATUST				{ $$ = find_type_or_error($<str>1, 0); }
	| tHANDLET				{ $$ = find_type_or_error($<str>1, 0); }
	;

m_int:
	| tINT
	;

int_std:  tINT					{ $$ = type_new_int(TYPE_BASIC_INT, 0); }
	| tSHORT m_int				{ $$ = type_new_int(TYPE_BASIC_INT16, 0); }
	| tSMALL				{ $$ = type_new_int(TYPE_BASIC_INT8, 0); }
	| tLONG m_int				{ $$ = type_new_int(TYPE_BASIC_LONG, 0); }
	| tHYPER m_int				{ $$ = type_new_int(TYPE_BASIC_HYPER, 0); }
	| tINT64				{ $$ = type_new_int(TYPE_BASIC_INT64, 0); }
	| tCHAR					{ $$ = type_new_int(TYPE_BASIC_CHAR, 0); }
	| tINT32				{ $$ = type_new_int(TYPE_BASIC_INT32, 0); }
	| tINT3264				{ $$ = type_new_int(TYPE_BASIC_INT3264, 0); }
	;

coclass:  tCOCLASS aIDENTIFIER			{ $$ = type_new_coclass($2); }
	| tCOCLASS aKNOWNTYPE			{ $$ = find_type($2, NULL, 0);
						  if (type_get_type_detect_alias($$) != TYPE_COCLASS)
						    error_loc("%s was not declared a coclass at %s:%d\n",
							      $2, $$->loc_info.input_name,
							      $$->loc_info.line_number);
						}
	;

coclasshdr: attributes coclass			{ $$ = $2;
						  check_def($$);
						  $$->attrs = check_coclass_attrs($2->name, $1);
						}
	;

coclassdef: coclasshdr '{' coclass_ints '}' semicolon_opt
						{ $$ = type_coclass_define($1, $3); }
	;

namespacedef: tNAMESPACE aIDENTIFIER		{ $$ = $2; }
	;

coclass_ints:					{ $$ = NULL; }
	| coclass_ints coclass_int		{ $$ = append_ifref( $1, $2 ); }
	;

coclass_int:
	  m_attributes interfacedec		{ $$ = make_ifref($2); $$->attrs = $1; }
	;

dispinterface: tDISPINTERFACE aIDENTIFIER	{ $$ = get_type(TYPE_INTERFACE, $2, current_namespace, 0); }
	|      tDISPINTERFACE aKNOWNTYPE	{ $$ = get_type(TYPE_INTERFACE, $2, current_namespace, 0); }
	;

dispinterfacehdr: attributes dispinterface	{ attr_t *attrs;
						  $$ = $2;
						  check_def($$);
						  attrs = make_attr(ATTR_DISPINTERFACE);
						  $$->attrs = append_attr( check_dispiface_attrs($2->name, $1), attrs );
						  $$->defined = TRUE;
						}
	;

dispint_props: tPROPERTIES ':'			{ $$ = NULL; }
	| dispint_props s_field ';'		{ $$ = append_var( $1, $2 ); }
	;

dispint_meths: tMETHODS ':'			{ $$ = NULL; }
	| dispint_meths funcdef ';'		{ $$ = append_var( $1, $2 ); }
	;

dispinterfacedef: dispinterfacehdr '{'
	  dispint_props
	  dispint_meths
	  '}'					{ $$ = $1;
						  type_dispinterface_define($$, $3, $4);
						}
	| dispinterfacehdr
	 '{' interface ';' '}' 			{ $$ = $1;
						  type_dispinterface_define_from_iface($$, $3);
						}
	;

inherit:					{ $$ = NULL; }
	| ':' aKNOWNTYPE			{ $$ = find_type_or_error2($2, 0); }
	;

interface: tINTERFACE aIDENTIFIER		{ $$ = get_type(TYPE_INTERFACE, $2, current_namespace, 0); }
	|  tINTERFACE aKNOWNTYPE		{ $$ = get_type(TYPE_INTERFACE, $2, current_namespace, 0); }
	;

interfacehdr: attributes interface		{ $$.interface = $2;
						  $$.old_pointer_default = pointer_default;
						  if (is_attr($1, ATTR_POINTERDEFAULT))
						    pointer_default = get_attrv($1, ATTR_POINTERDEFAULT);
						  check_def($2);
						  $2->attrs = check_iface_attrs($2->name, $1);
						  $2->defined = TRUE;
						}
	;

interfacedef: interfacehdr inherit
	  '{' int_statements '}' semicolon_opt	{ $$ = $1.interface;
						  if($$ == $2)
						    error_loc("Interface can't inherit from itself\n");
						  type_interface_define($$, $2, $4);
						  check_async_uuid($$);
						  pointer_default = $1.old_pointer_default;
						}
/* MIDL is able to import the definition of a base class from inside the
 * definition of a derived class, I'll try to support it with this rule */
	| interfacehdr ':' aIDENTIFIER
	  '{' import int_statements '}'
	   semicolon_opt			{ $$ = $1.interface;
						  type_interface_define($$, find_type_or_error2($3, 0), $6);
						  pointer_default = $1.old_pointer_default;
						}
	| dispinterfacedef semicolon_opt	{ $$ = $1; }
	;

interfacedec:
	  interface ';'				{ $$ = $1; }
	| dispinterface ';'			{ $$ = $1; }
	;

module:   tMODULE aIDENTIFIER			{ $$ = type_new_module($2); }
	| tMODULE aKNOWNTYPE			{ $$ = type_new_module($2); }
	;

modulehdr: attributes module			{ $$ = $2;
						  $$->attrs = check_module_attrs($2->name, $1);
						}
	;

moduledef: modulehdr '{' int_statements '}'
	   semicolon_opt			{ $$ = $1;
                                                  type_module_define($$, $3);
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
						{ $$ = $3; $$->type = append_chain_type($$->type, type_new_pointer(pointer_default, NULL, $2)); }
	| callconv declarator			{ $$ = $2; if ($$->func_type) $$->func_type->attrs = append_attr($$->func_type->attrs, make_attrp(ATTR_CALLCONV, $1));
						           else if ($$->type) $$->type->attrs = append_attr($$->type->attrs, make_attrp(ATTR_CALLCONV, $1)); }
	| direct_declarator
	;

direct_declarator:
	  ident					{ $$ = make_declarator($1); }
	| '(' declarator ')'			{ $$ = $2; }
	| direct_declarator array		{ $$ = $1; $$->type = append_array($$->type, $2); }
	| direct_declarator '(' m_args ')'	{ $$ = $1;
						  $$->func_type = append_chain_type($$->type, type_new_function($3));
						  $$->type = NULL;
						}
	;

/* abstract declarator */
abstract_declarator:
	  '*' m_type_qual_list m_abstract_declarator %prec PPTR
						{ $$ = $3; $$->type = append_chain_type($$->type, type_new_pointer(pointer_default, NULL, $2)); }
	| callconv m_abstract_declarator	{ $$ = $2; if ($$->func_type) $$->func_type->attrs = append_attr($$->func_type->attrs, make_attrp(ATTR_CALLCONV, $1));
						           else if ($$->type) $$->type->attrs = append_attr($$->type->attrs, make_attrp(ATTR_CALLCONV, $1)); }
	| abstract_direct_declarator
	;

/* abstract declarator without accepting direct declarator */
abstract_declarator_no_direct:
	  '*' m_type_qual_list m_any_declarator %prec PPTR
						{ $$ = $3; $$->type = append_chain_type($$->type, type_new_pointer(pointer_default, NULL, $2)); }
	| callconv m_any_declarator		{ $$ = $2; if ($$->func_type) $$->func_type->attrs = append_attr($$->func_type->attrs, make_attrp(ATTR_CALLCONV, $1));
						           else if ($$->type) $$->type->attrs = append_attr($$->type->attrs, make_attrp(ATTR_CALLCONV, $1)); }
	;

/* abstract declarator or empty */
m_abstract_declarator: 				{ $$ = make_declarator(NULL); }
	| abstract_declarator
	;

/* abstract direct declarator */
abstract_direct_declarator:
	  '(' abstract_declarator_no_direct ')'	{ $$ = $2; }
	| abstract_direct_declarator array	{ $$ = $1; $$->type = append_array($$->type, $2); }
	| array					{ $$ = make_declarator(NULL); $$->type = append_array($$->type, $1); }
	| '(' m_args ')'
						{ $$ = make_declarator(NULL);
						  $$->func_type = append_chain_type($$->type, type_new_function($2));
						  $$->type = NULL;
						}
	| abstract_direct_declarator '(' m_args ')'
						{ $$ = $1;
						  $$->func_type = append_chain_type($$->type, type_new_function($3));
						  $$->type = NULL;
						}
	;

/* abstract or non-abstract declarator */
any_declarator:
	  '*' m_type_qual_list m_any_declarator %prec PPTR
						{ $$ = $3; $$->type = append_chain_type($$->type, type_new_pointer(pointer_default, NULL, $2)); }
	| callconv m_any_declarator		{ $$ = $2; $$->type->attrs = append_attr($$->type->attrs, make_attrp(ATTR_CALLCONV, $1)); }
	| any_direct_declarator
	;

/* abstract or non-abstract declarator without accepting direct declarator */
any_declarator_no_direct:
	  '*' m_type_qual_list m_any_declarator %prec PPTR
						{ $$ = $3; $$->type = append_chain_type($$->type, type_new_pointer(pointer_default, NULL, $2)); }
	| callconv m_any_declarator		{ $$ = $2; $$->type->attrs = append_attr($$->type->attrs, make_attrp(ATTR_CALLCONV, $1)); }
	;

/* abstract or non-abstract declarator or empty */
m_any_declarator: 				{ $$ = make_declarator(NULL); }
	| any_declarator
	;

/* abstract or non-abstract direct declarator. note: direct declarators
 * aren't accepted inside brackets to avoid ambiguity with the rule for
 * function arguments */
any_direct_declarator:
	  ident					{ $$ = make_declarator($1); }
	| '(' any_declarator_no_direct ')'	{ $$ = $2; }
	| any_direct_declarator array		{ $$ = $1; $$->type = append_array($$->type, $2); }
	| array					{ $$ = make_declarator(NULL); $$->type = append_array($$->type, $1); }
	| '(' m_args ')'
						{ $$ = make_declarator(NULL);
						  $$->func_type = append_chain_type($$->type, type_new_function($2));
						  $$->type = NULL;
						}
	| any_direct_declarator '(' m_args ')'
						{ $$ = $1;
						  $$->func_type = append_chain_type($$->type, type_new_function($3));
						  $$->type = NULL;
						}
	;

declarator_list:
	  declarator				{ $$ = append_declarator( NULL, $1 ); }
	| declarator_list ',' declarator	{ $$ = append_declarator( $1, $3 ); }
	;

m_bitfield:					{ $$ = NULL; }
	| ':' expr_const			{ $$ = $2; }
	;

struct_declarator: any_declarator m_bitfield	{ $$ = $1; $$->bits = $2;
						  if (!$$->bits && !$$->var->name)
						    error_loc("unnamed fields are not allowed\n");
						}
	;

struct_declarator_list:
	  struct_declarator			{ $$ = append_declarator( NULL, $1 ); }
	| struct_declarator_list ',' struct_declarator
						{ $$ = append_declarator( $1, $3 ); }
	;

init_declarator:
	  declarator				{ $$ = $1; }
	| declarator '=' expr_const		{ $$ = $1; $1->var->eval = $3; }
	;

threading_type:
	  tAPARTMENT				{ $$ = THREADING_APARTMENT; }
	| tNEUTRAL				{ $$ = THREADING_NEUTRAL; }
	| tSINGLE				{ $$ = THREADING_SINGLE; }
	| tFREE					{ $$ = THREADING_FREE; }
	| tBOTH					{ $$ = THREADING_BOTH; }
	;

pointer_type:
	  tREF					{ $$ = FC_RP; }
	| tUNIQUE				{ $$ = FC_UP; }
	| tPTR					{ $$ = FC_FP; }
	;

structdef: tSTRUCT t_ident '{' fields '}'	{ $$ = type_new_struct($2, current_namespace, TRUE, $4); }
	;

type:	  tVOID					{ $$ = type_new_void(); }
	| aKNOWNTYPE				{ $$ = find_type_or_error($1, 0); }
	| base_type				{ $$ = $1; }
	| enumdef				{ $$ = $1; }
	| tENUM aIDENTIFIER			{ $$ = type_new_enum($2, current_namespace, FALSE, NULL); }
	| structdef				{ $$ = $1; }
	| tSTRUCT aIDENTIFIER			{ $$ = type_new_struct($2, current_namespace, FALSE, NULL); }
	| uniondef				{ $$ = $1; }
	| tUNION aIDENTIFIER			{ $$ = type_new_nonencapsulated_union($2, FALSE, NULL); }
	| tSAFEARRAY '(' type ')'		{ $$ = make_safearray($3); }
	;

typedef: m_attributes tTYPEDEF m_attributes decl_spec declarator_list
						{ $1 = append_attribs($1, $3);
						  reg_typedefs($4, $5, check_typedef_attrs($1));
						  $$ = make_statement_typedef($5);
						}
	;

uniondef: tUNION t_ident '{' ne_union_fields '}'
						{ $$ = type_new_nonencapsulated_union($2, TRUE, $4); }
	| tUNION t_ident
	  tSWITCH '(' s_field ')'
	  m_ident '{' cases '}'			{ $$ = type_new_encapsulated_union($2, $5, $7, $9); }
	;

version:
	  aNUM					{ $$ = MAKEVERSION($1, 0); }
	| aNUM '.' aNUM				{ $$ = MAKEVERSION($1, $3); }
	| aHEXNUM				{ $$ = $1; }
	;

acf_statements
        : /* empty */
        | acf_interface acf_statements
	;

acf_int_statements
        : /* empty */
        | acf_int_statement acf_int_statements
	;

acf_int_statement
        : tTYPEDEF acf_attributes aKNOWNTYPE ';'
                                                { type_t *type = find_type_or_error($3, 0);
                                                  type->attrs = append_attr_list(type->attrs, $2);
                                                }
	;

acf_interface
        : acf_attributes tINTERFACE aKNOWNTYPE '{' acf_int_statements '}'
                                                {  type_t *iface = find_type_or_error2($3, 0);
                                                   if (type_get_type(iface) != TYPE_INTERFACE)
                                                       error_loc("%s is not an interface\n", iface->name);
                                                   iface->attrs = append_attr_list(iface->attrs, $1);
                                                }
	;

acf_attributes
        : /* empty */                           { $$ = NULL; };
        | '[' acf_attribute_list ']'            { $$ = $2; };
	;

acf_attribute_list
        : acf_attribute                         { $$ = append_attr(NULL, $1); }
        | acf_attribute_list ',' acf_attribute  { $$ = append_attr($1, $3); }
	;

acf_attribute
	: tALLOCATE '(' allocate_option_list ')'
						{ $$ = make_attrv(ATTR_ALLOCATE, $3); }
        | tENCODE                               { $$ = make_attr(ATTR_ENCODE); }
        | tDECODE                               { $$ = make_attr(ATTR_DECODE); }
        | tEXPLICITHANDLE                       { $$ = make_attr(ATTR_EXPLICIT_HANDLE); }
	;

allocate_option_list
	: allocate_option			{ $$ = $1; }
	| allocate_option_list ',' allocate_option
						{ $$ = $1 | $3; }
	;

allocate_option
	: tDONTFREE				{ $$ = FC_DONT_FREE; }
	| tFREE					{ $$ = 0; }
	| tALLNODES				{ $$ = FC_ALLOCATE_ALL_NODES; }
	| tSINGLENODE				{ $$ = 0; }
	;
%%

static void decl_builtin_basic(const char *name, enum type_basic_type type)
{
  type_t *t = type_new_basic(type);
  reg_type(t, name, NULL, 0);
}

static void decl_builtin_alias(const char *name, type_t *t)
{
  reg_type(type_new_alias(t, name), name, NULL, 0);
}

void init_types(void)
{
  decl_builtin_basic("byte", TYPE_BASIC_BYTE);
  decl_builtin_basic("wchar_t", TYPE_BASIC_WCHAR);
  decl_builtin_basic("float", TYPE_BASIC_FLOAT);
  decl_builtin_basic("double", TYPE_BASIC_DOUBLE);
  decl_builtin_basic("error_status_t", TYPE_BASIC_ERROR_STATUS_T);
  decl_builtin_basic("handle_t", TYPE_BASIC_HANDLE);
  decl_builtin_alias("boolean", type_new_basic(TYPE_BASIC_BYTE));
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

typedef int (*map_attrs_filter_t)(attr_list_t*,const attr_t*);

static attr_list_t *map_attrs(const attr_list_t *list, map_attrs_filter_t filter)
{
  attr_list_t *new_list;
  const attr_t *attr;
  attr_t *new_attr;

  if (!list) return NULL;

  new_list = xmalloc( sizeof(*list) );
  list_init( new_list );
  LIST_FOR_EACH_ENTRY(attr, list, const attr_t, entry)
  {
    if (filter && !filter(new_list, attr)) continue;
    new_attr = xmalloc(sizeof(*new_attr));
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
    attrs = map_attrs(type->attrs, NULL);
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

static attr_t *make_attrv(enum attr_type type, unsigned int val)
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

static type_t *append_array(type_t *chain, expr_t *expr)
{
    type_t *array;

    if (!expr)
        return chain;

    /* An array is always a reference pointer unless explicitly marked otherwise
     * (regardless of what the default pointer attribute is). */
    array = type_new_array(NULL, NULL, FALSE, expr->is_const ? expr->cval : 0,
            expr->is_const ? NULL : expr, NULL, FC_RP);

    return append_chain_type(chain, array);
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

void clear_all_offsets(void)
{
  type_pool_node_t *node;
  LIST_FOR_EACH_ENTRY(node, &type_pool, type_pool_node_t, link)
    node->data.typestring_offset = node->data.ptrdesc = 0;
}

static void type_function_add_head_arg(type_t *type, var_t *arg)
{
    if (!type->details.function->args)
    {
        type->details.function->args = xmalloc( sizeof(*type->details.function->args) );
        list_init( type->details.function->args );
    }
    list_add_head( type->details.function->args, &arg->entry );
}

static int is_allowed_range_type(const type_t *type)
{
    switch (type_get_type(type))
    {
    case TYPE_ENUM:
        return TRUE;
    case TYPE_BASIC:
        switch (type_basic_get_type(type))
        {
        case TYPE_BASIC_INT8:
        case TYPE_BASIC_INT16:
        case TYPE_BASIC_INT32:
        case TYPE_BASIC_INT64:
        case TYPE_BASIC_INT:
        case TYPE_BASIC_INT3264:
        case TYPE_BASIC_LONG:
        case TYPE_BASIC_BYTE:
        case TYPE_BASIC_CHAR:
        case TYPE_BASIC_WCHAR:
        case TYPE_BASIC_HYPER:
            return TRUE;
        case TYPE_BASIC_FLOAT:
        case TYPE_BASIC_DOUBLE:
        case TYPE_BASIC_ERROR_STATUS_T:
        case TYPE_BASIC_HANDLE:
            return FALSE;
        }
        return FALSE;
    default:
        return FALSE;
    }
}

static type_t *get_array_or_ptr_ref(type_t *type)
{
    if (is_ptr(type))
        return type_pointer_get_ref(type);
    else if (is_array(type))
        return type_array_get_element(type);
    return NULL;
}

static type_t *append_chain_type(type_t *chain, type_t *type)
{
    type_t *chain_type;

    if (!chain)
        return type;
    for (chain_type = chain; get_array_or_ptr_ref(chain_type); chain_type = get_array_or_ptr_ref(chain_type))
        ;

    if (is_ptr(chain_type))
        chain_type->details.pointer.ref = type;
    else if (is_array(chain_type))
        chain_type->details.array.elem = type;
    else
        assert(0);

    return chain;
}

static warning_list_t *append_warning(warning_list_t *list, int num)
{
    warning_t *entry;

    if(!list)
    {
        list = xmalloc( sizeof(*list) );
        list_init( list );
    }
    entry = xmalloc( sizeof(*entry) );
    entry->num = num;
    list_add_tail( list, &entry->entry );
    return list;
}

static var_t *declare_var(attr_list_t *attrs, decl_spec_t *decl_spec, const declarator_t *decl,
                       int top)
{
  var_t *v = decl->var;
  expr_list_t *sizes = get_attrp(attrs, ATTR_SIZEIS);
  expr_list_t *lengs = get_attrp(attrs, ATTR_LENGTHIS);
  expr_t *dim;
  type_t **ptype;
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
      for (t = func_type; is_ptr(t); t = type_pointer_get_ref(t))
        ;
      t->attrs = move_attr(t->attrs, type->attrs, ATTR_INLINE);
    }
  }

  /* add type onto the end of the pointers in pident->type */
  v->type = append_chain_type(decl ? decl->type : NULL, type);
  v->stgclass = decl_spec->stgclass;
  v->attrs = attrs;

  /* check for pointer attribute being applied to non-pointer, non-array
   * type */
  if (!is_array(v->type))
  {
    int ptr_attr = get_attrv(v->attrs, ATTR_POINTERTYPE);
    const type_t *ptr = NULL;
    /* pointer attributes on the left side of the type belong to the function
     * pointer, if one is being declared */
    type_t **pt = func_type ? &func_type : &v->type;
    for (ptr = *pt; ptr && !ptr_attr; )
    {
      ptr_attr = get_attrv(ptr->attrs, ATTR_POINTERTYPE);
      if (!ptr_attr && type_is_alias(ptr))
        ptr = type_alias_get_aliasee(ptr);
      else
        break;
    }
    if (is_ptr(ptr))
    {
      if (ptr_attr && ptr_attr != FC_UP &&
          type_get_type(type_pointer_get_ref(ptr)) == TYPE_INTERFACE)
          warning_loc_info(&v->loc_info,
                           "%s: pointer attribute applied to interface "
                           "pointer type has no effect\n", v->name);
      if (!ptr_attr && top && (*pt)->details.pointer.def_fc != FC_RP)
      {
        /* FIXME: this is a horrible hack to cope with the issue that we
         * store an offset to the typeformat string in the type object, but
         * two typeformat strings may be written depending on whether the
         * pointer is a toplevel parameter or not */
        *pt = duptype(*pt, 1);
      }
    }
    else if (ptr_attr)
       error_loc("%s: pointer attribute applied to non-pointer type\n", v->name);
  }

  if (is_attr(v->attrs, ATTR_STRING))
  {
    type_t *t = type;

    if (!is_ptr(v->type) && !is_array(v->type))
      error_loc("'%s': [string] attribute applied to non-pointer, non-array type\n",
                v->name);

    for (;;)
    {
        if (is_ptr(t))
            t = type_pointer_get_ref(t);
        else if (is_array(t))
            t = type_array_get_element(t);
        else
            break;
    }

    if (type_get_type(t) != TYPE_BASIC &&
        (get_basic_fc(t) != FC_CHAR &&
         get_basic_fc(t) != FC_BYTE &&
         get_basic_fc(t) != FC_WCHAR))
    {
      error_loc("'%s': [string] attribute is only valid on 'char', 'byte', or 'wchar_t' pointers and arrays\n",
                v->name);
    }
  }

  if (is_attr(v->attrs, ATTR_V1ENUM))
  {
    if (type_get_type_detect_alias(v->type) != TYPE_ENUM)
      error_loc("'%s': [v1_enum] attribute applied to non-enum type\n", v->name);
  }

  if (is_attr(v->attrs, ATTR_RANGE) && !is_allowed_range_type(v->type))
    error_loc("'%s': [range] attribute applied to non-integer type\n",
              v->name);

  ptype = &v->type;
  if (sizes) LIST_FOR_EACH_ENTRY(dim, sizes, expr_t, entry)
  {
    if (dim->type != EXPR_VOID)
    {
      if (is_array(*ptype))
      {
        if (!type_array_get_conformance(*ptype) ||
            type_array_get_conformance(*ptype)->type != EXPR_VOID)
          error_loc("%s: cannot specify size_is for an already sized array\n", v->name);
        else
          *ptype = type_new_array((*ptype)->name,
                                  type_array_get_element(*ptype), FALSE,
                                  0, dim, NULL, 0);
      }
      else if (is_ptr(*ptype))
        *ptype = type_new_array((*ptype)->name, type_pointer_get_ref(*ptype), TRUE,
                                0, dim, NULL, pointer_default);
      else
        error_loc("%s: size_is attribute applied to illegal type\n", v->name);
    }

    if (is_ptr(*ptype))
      ptype = &(*ptype)->details.pointer.ref;
    else if (is_array(*ptype))
      ptype = &(*ptype)->details.array.elem;
    else
      error_loc("%s: too many expressions in size_is attribute\n", v->name);
  }

  ptype = &v->type;
  if (lengs) LIST_FOR_EACH_ENTRY(dim, lengs, expr_t, entry)
  {
    if (dim->type != EXPR_VOID)
    {
      if (is_array(*ptype))
      {
        *ptype = type_new_array((*ptype)->name,
                                type_array_get_element(*ptype),
                                type_array_is_decl_as_ptr(*ptype),
                                type_array_get_dim(*ptype),
                                type_array_get_conformance(*ptype),
                                dim, type_array_get_ptr_default_fc(*ptype));
      }
      else
        error_loc("%s: length_is attribute applied to illegal type\n", v->name);
    }

    if (is_ptr(*ptype))
      ptype = &(*ptype)->details.pointer.ref;
    else if (is_array(*ptype))
      ptype = &(*ptype)->details.array.elem;
    else
      error_loc("%s: too many expressions in length_is attribute\n", v->name);
  }

  /* v->type is currently pointing to the type on the left-side of the
   * declaration, so we need to fix this up so that it is the return type of the
   * function and make v->type point to the function side of the declaration */
  if (func_type)
  {
    type_t *ft, *t;
    type_t *return_type = v->type;
    v->type = func_type;
    for (ft = v->type; is_ptr(ft); ft = type_pointer_get_ref(ft))
      ;
    assert(type_get_type_detect_alias(ft) == TYPE_FUNCTION);
    ft->details.function->retval = make_var(xstrdup("_RetVal"));
    ft->details.function->retval->type = return_type;
    /* move calling convention attribute, if present, from pointer nodes to
     * function node */
    for (t = v->type; is_ptr(t); t = type_pointer_get_ref(t))
      ft->attrs = move_attr(ft->attrs, t->attrs, ATTR_CALLCONV);
  }
  else
  {
    type_t *t;
    for (t = v->type; is_ptr(t); t = type_pointer_get_ref(t))
      if (is_attr(t->attrs, ATTR_CALLCONV))
        error_loc("calling convention applied to non-function-pointer type\n");
  }

  if (decl->bits)
    v->type = type_new_bitfield(v->type, decl->bits);

  return v;
}

static var_list_t *set_var_types(attr_list_t *attrs, decl_spec_t *decl_spec, declarator_list_t *decls)
{
  declarator_t *decl, *next;
  var_list_t *var_list = NULL;

  LIST_FOR_EACH_ENTRY_SAFE( decl, next, decls, declarator_t, entry )
  {
    var_t *var = declare_var(attrs, decl_spec, decl, 0);
    var_list = append_var(var_list, var);
    free(decl);
  }
  free(decl_spec);
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

var_list_t *append_var(var_list_t *list, var_t *var)
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

var_t *make_var(char *name)
{
  var_t *v = xmalloc(sizeof(var_t));
  v->name = name;
  v->type = NULL;
  v->attrs = NULL;
  v->eval = NULL;
  v->stgclass = STG_NONE;
  init_loc_info(&v->loc_info);
  return v;
}

static var_t *copy_var(var_t *src, char *name, map_attrs_filter_t attr_filter)
{
  var_t *v = xmalloc(sizeof(var_t));
  v->name = name;
  v->type = src->type;
  v->attrs = map_attrs(src->attrs, attr_filter);
  v->eval = src->eval;
  v->stgclass = src->stgclass;
  v->loc_info = src->loc_info;
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
  d->var = var ? var : make_var(NULL);
  d->type = NULL;
  d->func_type = NULL;
  d->bits = NULL;
  return d;
}

static type_t *make_safearray(type_t *type)
{
  return type_new_array(NULL, type_new_alias(type, "SAFEARRAY"), TRUE, 0,
                        NULL, NULL, FC_RP);
}

static typelib_t *make_library(const char *name, const attr_list_t *attrs)
{
    typelib_t *typelib = xmalloc(sizeof(*typelib));
    typelib->name = xstrdup(name);
    typelib->attrs = attrs;
    list_init( &typelib->importlibs );
    return typelib;
}

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

static struct namespace *find_sub_namespace(struct namespace *namespace, const char *name)
{
  struct namespace *cur;

  LIST_FOR_EACH_ENTRY(cur, &namespace->children, struct namespace, entry) {
    if(!strcmp(cur->name, name))
      return cur;
  }

  return NULL;
}

static void push_namespace(const char *name)
{
  struct namespace *namespace;

  namespace = find_sub_namespace(current_namespace, name);
  if(!namespace) {
    namespace = xmalloc(sizeof(*namespace));
    namespace->name = xstrdup(name);
    namespace->parent = current_namespace;
    list_add_tail(&current_namespace->children, &namespace->entry);
    list_init(&namespace->children);
    memset(namespace->type_hash, 0, sizeof(namespace->type_hash));
  }

  current_namespace = namespace;
}

static void pop_namespace(const char *name)
{
  assert(!strcmp(current_namespace->name, name) && current_namespace->parent);
  current_namespace = current_namespace->parent;
}

struct rtype {
  const char *name;
  type_t *type;
  int t;
  struct rtype *next;
};

type_t *reg_type(type_t *type, const char *name, struct namespace *namespace, int t)
{
  struct rtype *nt;
  int hash;
  if (!name) {
    error_loc("registering named type without name\n");
    return type;
  }
  if (!namespace)
    namespace = &global_namespace;
  hash = hash_ident(name);
  nt = xmalloc(sizeof(struct rtype));
  nt->name = name;
  if (is_global_namespace(namespace))
    type->c_name = name;
  else
    type->c_name = format_namespace(namespace, "__x_", "_C", name);
  nt->type = type;
  nt->t = t;
  nt->next = namespace->type_hash[hash];
  namespace->type_hash[hash] = nt;
  if ((t == tsSTRUCT || t == tsUNION))
    fix_incomplete_types(type);
  return type;
}

static int is_incomplete(const type_t *t)
{
  return !t->defined &&
    (type_get_type_detect_alias(t) == TYPE_STRUCT ||
     type_get_type_detect_alias(t) == TYPE_UNION ||
     type_get_type_detect_alias(t) == TYPE_ENCAPSULATED_UNION);
}

void add_incomplete(type_t *t)
{
  struct typenode *tn = xmalloc(sizeof *tn);
  tn->type = t;
  list_add_tail(&incomplete_types, &tn->entry);
}

static void fix_type(type_t *t)
{
  if (type_is_alias(t) && is_incomplete(t)) {
    type_t *ot = type_alias_get_aliasee(t);
    fix_type(ot);
    if (type_get_type_detect_alias(ot) == TYPE_STRUCT ||
        type_get_type_detect_alias(ot) == TYPE_UNION ||
        type_get_type_detect_alias(ot) == TYPE_ENCAPSULATED_UNION)
      t->details.structure = ot->details.structure;
    t->defined = ot->defined;
  }
}

static void fix_incomplete(void)
{
  struct typenode *tn, *next;

  LIST_FOR_EACH_ENTRY_SAFE(tn, next, &incomplete_types, struct typenode, entry) {
    fix_type(tn->type);
    list_remove(&tn->entry);
    free(tn);
  }
}

static void fix_incomplete_types(type_t *complete_type)
{
  struct typenode *tn, *next;

  LIST_FOR_EACH_ENTRY_SAFE(tn, next, &incomplete_types, struct typenode, entry)
  {
    if (type_is_equal(complete_type, tn->type))
    {
      tn->type->details.structure = complete_type->details.structure;
      list_remove(&tn->entry);
      free(tn);
    }
  }
}

static type_t *reg_typedefs(decl_spec_t *decl_spec, declarator_list_t *decls, attr_list_t *attrs)
{
  const declarator_t *decl;
  type_t *type = decl_spec->type;

  if (is_attr(attrs, ATTR_UUID) && !is_attr(attrs, ATTR_PUBLIC))
    attrs = append_attr( attrs, make_attr(ATTR_PUBLIC) );

  /* We must generate names for tagless enum, struct or union.
     Typedef-ing a tagless enum, struct or union means we want the typedef
     to be included in a library hence the public attribute.  */
  if (type_get_type_detect_alias(type) == TYPE_ENUM ||
      type_get_type_detect_alias(type) == TYPE_STRUCT ||
      type_get_type_detect_alias(type) == TYPE_UNION ||
      type_get_type_detect_alias(type) == TYPE_ENCAPSULATED_UNION)
  {
    if (!type->name)
      type->name = gen_name();

    /* replace existing attributes when generating a typelib */
    if (do_typelib)
        type->attrs = attrs;
  }

#ifdef __REACTOS__ /* r53187 / 5bf224e */
  /* Append the SWITCHTYPE attribute to a non-encapsulated union if it does not already have it.  */
  if (type_get_type_detect_alias(type) == TYPE_UNION &&
      is_attr(attrs, ATTR_SWITCHTYPE) &&
      !is_attr(type->attrs, ATTR_SWITCHTYPE))
    type->attrs = append_attr(type->attrs, make_attrp(ATTR_SWITCHTYPE, get_attrp(attrs, ATTR_SWITCHTYPE)));
#endif

  LIST_FOR_EACH_ENTRY( decl, decls, const declarator_t, entry )
  {

    if (decl->var->name) {
      type_t *cur;
      var_t *name;

      cur = find_type(decl->var->name, current_namespace, 0);

      /*
       * MIDL allows shadowing types that are declared in imported files.
       * We don't throw an error in this case and instead add a new type
       * (which is earlier on the list in hash table, so it will be used
       * instead of shadowed type).
       *
       * FIXME: We may consider string separated type tables for each input
       *        for cleaner solution.
       */
      if (cur && input_name == cur->loc_info.input_name)
          error_loc("%s: redefinition error; original definition was at %s:%d\n",
                    cur->name, cur->loc_info.input_name,
                    cur->loc_info.line_number);

      name = declare_var(attrs, decl_spec, decl, 0);
      cur = type_new_alias(name->type, name->name);
      cur->attrs = attrs;

      if (is_incomplete(cur))
        add_incomplete(cur);
      reg_type(cur, cur->name, current_namespace, 0);
    }
  }
  return type;
}

type_t *find_type(const char *name, struct namespace *namespace, int t)
{
  struct rtype *cur;

  if(namespace && namespace != &global_namespace) {
    for(cur = namespace->type_hash[hash_ident(name)]; cur; cur = cur->next) {
      if(cur->t == t && !strcmp(cur->name, name))
        return cur->type;
    }
  }
  for(cur = global_namespace.type_hash[hash_ident(name)]; cur; cur = cur->next) {
    if(cur->t == t && !strcmp(cur->name, name))
      return cur->type;
  }
  return NULL;
}

static type_t *find_type_or_error(const char *name, int t)
{
  type_t *type = find_type(name, NULL, t);
  if (!type) {
    error_loc("type '%s' not found\n", name);
    return NULL;
  }
  return type;
}

static type_t *find_type_or_error2(char *name, int t)
{
  type_t *tp = find_type_or_error(name, t);
  free(name);
  return tp;
}

int is_type(const char *name)
{
  return find_type(name, current_namespace, 0) != NULL;
}

type_t *get_type(enum type_type type, char *name, struct namespace *namespace, int t)
{
  type_t *tp;
  if (!namespace)
    namespace = &global_namespace;
  if (name) {
    tp = find_type(name, namespace, t);
    if (tp) {
      free(name);
      return tp;
    }
  }
  tp = make_type(type);
  tp->name = name;
  tp->namespace = namespace;
  if (!name) return tp;
  return reg_type(tp, name, namespace, t);
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

static char *gen_name(void)
{
  static const char format[] = "__WIDL_%s_generated_name_%08lX";
  static unsigned long n = 0;
  static const char *file_id;
  static size_t size;
  char *name;

  if (! file_id)
  {
    char *dst = dup_basename(input_idl_name, ".idl");
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
    unsigned int on_enum : 1;
    unsigned int on_struct : 2;
    unsigned int on_union : 1;
    unsigned int on_field : 1;
    unsigned int on_library : 1;
    unsigned int on_dispinterface : 1;
    unsigned int on_module : 1;
    unsigned int on_coclass : 1;
    const char *display_name;
};

struct allowed_attr allowed_attr[] =
{
    /* attr                        { D ACF I Fn ARG T En St Un Fi  L  DI M  C  <display name> } */
    /* ATTR_AGGREGATABLE */        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, "aggregatable" },
    /* ATTR_ALLOCATE */            { 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, "allocate" },
    /* ATTR_ANNOTATION */          { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, "annotation" },
    /* ATTR_APPOBJECT */           { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, "appobject" },
    /* ATTR_ASYNC */               { 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "async" },
    /* ATTR_ASYNCUUID */           { 1, 0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, "async_uuid" },
    /* ATTR_AUTO_HANDLE */         { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "auto_handle" },
    /* ATTR_BINDABLE */            { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "bindable" },
    /* ATTR_BROADCAST */           { 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "broadcast" },
    /* ATTR_CALLAS */              { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "call_as" },
    /* ATTR_CALLCONV */            { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL },
    /* ATTR_CASE */                { 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, "case" },
    /* ATTR_CODE */                { 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "code" },
    /* ATTR_COMMSTATUS */          { 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, "comm_status" },
    /* ATTR_CONST */               { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "const" },
    /* ATTR_CONTEXTHANDLE */       { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, "context_handle" },
    /* ATTR_CONTROL */             { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, "control" },
    /* ATTR_DECODE */              { 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, "decode" },
    /* ATTR_DEFAULT */             { 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, "default" },
    /* ATTR_DEFAULTBIND */         { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "defaultbind" },
    /* ATTR_DEFAULTCOLLELEM */     { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "defaultcollelem" },
    /* ATTR_DEFAULTVALUE */        { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, "defaultvalue" },
    /* ATTR_DEFAULTVTABLE */       { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, "defaultvtable" },
 /* ATTR_DISABLECONSISTENCYCHECK */{ 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, "disable_consistency_check" },
    /* ATTR_DISPINTERFACE */       { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL },
    /* ATTR_DISPLAYBIND */         { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "displaybind" },
    /* ATTR_DLLNAME */             { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, "dllname" },
    /* ATTR_DUAL */                { 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "dual" },
    /* ATTR_ENABLEALLOCATE */      { 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "enable_allocate" },
    /* ATTR_ENCODE */              { 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, "encode" },
    /* ATTR_ENDPOINT */            { 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "endpoint" },
    /* ATTR_ENTRY */               { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "entry" },
    /* ATTR_EXPLICIT_HANDLE */     { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "explicit_handle" },
    /* ATTR_FAULTSTATUS */         { 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, "fault_status" },
    /* ATTR_FORCEALLOCATE */       { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, "force_allocate" },
    /* ATTR_HANDLE */              { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, "handle" },
    /* ATTR_HELPCONTEXT */         { 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, "helpcontext" },
    /* ATTR_HELPFILE */            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, "helpfile" },
    /* ATTR_HELPSTRING */          { 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, "helpstring" },
    /* ATTR_HELPSTRINGCONTEXT */   { 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, "helpstringcontext" },
    /* ATTR_HELPSTRINGDLL */       { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, "helpstringdll" },
    /* ATTR_HIDDEN */              { 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 1, 1, 0, 1, "hidden" },
    /* ATTR_ID */                  { 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, "id" },
    /* ATTR_IDEMPOTENT */          { 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "idempotent" },
    /* ATTR_IGNORE */              { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, "ignore" },
    /* ATTR_IIDIS */               { 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, "iid_is" },
    /* ATTR_IMMEDIATEBIND */       { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "immediatebind" },
    /* ATTR_IMPLICIT_HANDLE */     { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "implicit_handle" },
    /* ATTR_IN */                  { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, "in" },
    /* ATTR_INLINE */              { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "inline" },
    /* ATTR_INPUTSYNC */           { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "inputsync" },
    /* ATTR_LENGTHIS */            { 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, "length_is" },
    /* ATTR_LIBLCID */             { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, "lcid" },
    /* ATTR_LICENSED */            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, "licensed" },
    /* ATTR_LOCAL */               { 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "local" },
    /* ATTR_MAYBE */               { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "maybe" },
    /* ATTR_MESSAGE */             { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "message" },
    /* ATTR_NOCODE */              { 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "nocode" },
    /* ATTR_NONBROWSABLE */        { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "nonbrowsable" },
    /* ATTR_NONCREATABLE */        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, "noncreatable" },
    /* ATTR_NONEXTENSIBLE */       { 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "nonextensible" },
    /* ATTR_NOTIFY */              { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "notify" },
    /* ATTR_NOTIFYFLAG */          { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "notify_flag" },
    /* ATTR_OBJECT */              { 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "object" },
    /* ATTR_ODL */                 { 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, "odl" },
    /* ATTR_OLEAUTOMATION */       { 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "oleautomation" },
    /* ATTR_OPTIMIZE */            { 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "optimize" },
    /* ATTR_OPTIONAL */            { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, "optional" },
    /* ATTR_OUT */                 { 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, "out" },
    /* ATTR_PARAMLCID */           { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, "lcid" },
    /* ATTR_PARTIALIGNORE */       { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, "partial_ignore" },
    /* ATTR_POINTERDEFAULT */      { 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "pointer_default" },
    /* ATTR_POINTERTYPE */         { 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, "ref, unique or ptr" },
    /* ATTR_PROGID */              { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, "progid" },
    /* ATTR_PROPGET */             { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "propget" },
    /* ATTR_PROPPUT */             { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "propput" },
    /* ATTR_PROPPUTREF */          { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "propputref" },
    /* ATTR_PROXY */               { 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "proxy" },
    /* ATTR_PUBLIC */              { 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, "public" },
    /* ATTR_RANGE */               { 0, 0, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, "range" },
    /* ATTR_READONLY */            { 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, "readonly" },
    /* ATTR_REPRESENTAS */         { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, "represent_as" },
    /* ATTR_REQUESTEDIT */         { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "requestedit" },
    /* ATTR_RESTRICTED */          { 0, 0, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, "restricted" },
    /* ATTR_RETVAL */              { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, "retval" },
    /* ATTR_SIZEIS */              { 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, "size_is" },
    /* ATTR_SOURCE */              { 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, "source" },
    /* ATTR_STRICTCONTEXTHANDLE */ { 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "strict_context_handle" },
    /* ATTR_STRING */              { 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, "string" },
    /* ATTR_SWITCHIS */            { 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, "switch_is" },
    /* ATTR_SWITCHTYPE */          { 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, "switch_type" },
    /* ATTR_THREADING */           { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, "threading" },
    /* ATTR_TRANSMITAS */          { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, "transmit_as" },
    /* ATTR_UIDEFAULT */           { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "uidefault" },
    /* ATTR_USESGETLASTERROR */    { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "usesgetlasterror" },
    /* ATTR_USERMARSHAL */         { 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, "user_marshal" },
    /* ATTR_UUID */                { 1, 0, 1, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 1, "uuid" },
    /* ATTR_V1ENUM */              { 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, "v1_enum" },
    /* ATTR_VARARG */              { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "vararg" },
    /* ATTR_VERSION */             { 1, 0, 1, 0, 0, 1, 1, 2, 0, 0, 1, 0, 0, 1, "version" },
    /* ATTR_VIPROGID */            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, "vi_progid" },
    /* ATTR_WIREMARSHAL */         { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, "wire_marshal" },
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
    if (attr->type == ATTR_IMPLICIT_HANDLE)
    {
        const var_t *var = attr->u.pval;
        if (type_get_type( var->type) == TYPE_BASIC &&
            type_basic_get_type( var->type ) == TYPE_BASIC_HANDLE)
            continue;
        if (is_aliaschain_attr( var->type, ATTR_HANDLE ))
            continue;
      error_loc("attribute %s requires a handle type in interface %s\n",
                allowed_attr[attr->type].display_name, name);
    }
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

static void check_arg_attrs(const var_t *arg)
{
  const attr_t *attr;

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

static attr_list_t *check_enum_attrs(attr_list_t *attrs)
{
  const attr_t *attr;
  if (!attrs) return attrs;
  LIST_FOR_EACH_ENTRY(attr, attrs, const attr_t, entry)
  {
    if (!allowed_attr[attr->type].on_enum)
      error_loc("inapplicable attribute %s for enum\n",
                allowed_attr[attr->type].display_name);
  }
  return attrs;
}

static attr_list_t *check_struct_attrs(attr_list_t *attrs)
{
  int mask = winrt_mode ? 3 : 1;
  const attr_t *attr;
  if (!attrs) return attrs;
  LIST_FOR_EACH_ENTRY(attr, attrs, const attr_t, entry)
  {
    if (!(allowed_attr[attr->type].on_struct & mask))
      error_loc("inapplicable attribute %s for struct\n",
                allowed_attr[attr->type].display_name);
  }
  return attrs;
}

static attr_list_t *check_union_attrs(attr_list_t *attrs)
{
  const attr_t *attr;
  if (!attrs) return attrs;
  LIST_FOR_EACH_ENTRY(attr, attrs, const attr_t, entry)
  {
    if (!allowed_attr[attr->type].on_union)
      error_loc("inapplicable attribute %s for union\n",
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
    switch (type_get_type(type))
    {
    case TYPE_ENUM:
        return TRUE;
    case TYPE_BASIC:
        switch (type_basic_get_type(type))
        {
        case TYPE_BASIC_INT8:
        case TYPE_BASIC_INT16:
        case TYPE_BASIC_INT32:
        case TYPE_BASIC_INT64:
        case TYPE_BASIC_INT:
        case TYPE_BASIC_LONG:
        case TYPE_BASIC_CHAR:
        case TYPE_BASIC_HYPER:
        case TYPE_BASIC_BYTE:
        case TYPE_BASIC_WCHAR:
            return TRUE;
        default:
            return FALSE;
        }
    case TYPE_ALIAS:
        /* shouldn't get here because of type_get_type call above */
        assert(0);
        /* fall through */
    case TYPE_STRUCT:
    case TYPE_UNION:
    case TYPE_ENCAPSULATED_UNION:
    case TYPE_ARRAY:
    case TYPE_POINTER:
    case TYPE_VOID:
    case TYPE_MODULE:
    case TYPE_COCLASS:
    case TYPE_FUNCTION:
    case TYPE_INTERFACE:
    case TYPE_BITFIELD:
        return FALSE;
    }
    return FALSE;
}

static int is_ptr_guid_type(const type_t *type)
{
    /* first, make sure it is a pointer to something */
    if (!is_ptr(type)) return FALSE;

    /* second, make sure it is a pointer to something of size sizeof(GUID),
     * i.e. 16 bytes */
    return (type_memsize(type_pointer_get_ref(type)) == 16);
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
    int more_to_do;
    const char *container_type_name;
    const char *var_type;

    switch (type_get_type(container_type))
    {
    case TYPE_STRUCT:
        container_type_name = "struct";
        var_type = "field";
        break;
    case TYPE_UNION:
        container_type_name = "union";
        var_type = "arm";
        break;
    case TYPE_ENCAPSULATED_UNION:
        container_type_name = "encapsulated union";
        var_type = "arm";
        break;
    case TYPE_FUNCTION:
        container_type_name = "function";
        var_type = "parameter";
        break;
    default:
        /* should be no other container types */
        assert(0);
        return;
    }

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

    do
    {
        more_to_do = FALSE;

        switch (typegen_detect_type(type, arg->attrs, TDT_IGNORE_STRINGS))
        {
        case TGT_STRUCT:
        case TGT_UNION:
            check_remoting_fields(arg, type);
            break;
        case TGT_INVALID:
        {
            const char *reason = "is invalid";
            switch (type_get_type(type))
            {
            case TYPE_VOID:
                reason = "cannot derive from void *";
                break;
            case TYPE_FUNCTION:
                reason = "cannot be a function pointer";
                break;
            case TYPE_BITFIELD:
                reason = "cannot be a bit-field";
                break;
            case TYPE_COCLASS:
                reason = "cannot be a class";
                break;
            case TYPE_INTERFACE:
                reason = "cannot be a non-pointer to an interface";
                break;
            case TYPE_MODULE:
                reason = "cannot be a module";
                break;
            default:
                break;
            }
            error_loc_info(&arg->loc_info, "%s \'%s\' of %s \'%s\' %s\n",
                           var_type, arg->name, container_type_name, container_name, reason);
            break;
        }
        case TGT_CTXT_HANDLE:
        case TGT_CTXT_HANDLE_POINTER:
            if (type_get_type(container_type) != TYPE_FUNCTION)
                error_loc_info(&arg->loc_info,
                               "%s \'%s\' of %s \'%s\' cannot be a context handle\n",
                               var_type, arg->name, container_type_name,
                               container_name);
            break;
        case TGT_STRING:
        {
            const type_t *t = type;
            while (is_ptr(t))
                t = type_pointer_get_ref(t);
            if (is_aliaschain_attr(t, ATTR_RANGE))
                warning_loc_info(&arg->loc_info, "%s: range not verified for a string of ranged types\n", arg->name);
            break;
        }
        case TGT_POINTER:
            type = type_pointer_get_ref(type);
            more_to_do = TRUE;
            break;
        case TGT_ARRAY:
            type = type_array_get_element(type);
            more_to_do = TRUE;
            break;
        case TGT_USER_TYPE:
        case TGT_IFACE_POINTER:
        case TGT_BASIC:
        case TGT_ENUM:
        case TGT_RANGE:
            /* nothing to do */
            break;
        }
    } while (more_to_do);
}

static void check_remoting_fields(const var_t *var, type_t *type)
{
    const var_t *field;
    const var_list_t *fields = NULL;

    type = type_get_real_type(type);

    if (type->checked)
        return;

    type->checked = TRUE;

    if (type_get_type(type) == TYPE_STRUCT)
    {
        if (type_is_complete(type))
            fields = type_struct_get_fields(type);
        else
            error_loc_info(&var->loc_info, "undefined type declaration %s\n", type->name);
    }
    else if (type_get_type(type) == TYPE_UNION || type_get_type(type) == TYPE_ENCAPSULATED_UNION)
        fields = type_union_get_cases(type);

    if (fields) LIST_FOR_EACH_ENTRY( field, fields, const var_t, entry )
        if (field->type) check_field_common(type, type->name, field);
}

/* checks that arguments for a function make sense for marshalling and unmarshalling */
static void check_remoting_args(const var_t *func)
{
    const char *funcname = func->name;
    const var_t *arg;

    if (func->type->details.function->args) LIST_FOR_EACH_ENTRY( arg, func->type->details.function->args, const var_t, entry )
    {
        const type_t *type = arg->type;

        /* check that [out] parameters have enough pointer levels */
        if (is_attr(arg->attrs, ATTR_OUT))
        {
            switch (typegen_detect_type(type, arg->attrs, TDT_ALL_TYPES))
            {
            case TGT_BASIC:
            case TGT_ENUM:
            case TGT_RANGE:
            case TGT_STRUCT:
            case TGT_UNION:
            case TGT_CTXT_HANDLE:
            case TGT_USER_TYPE:
                error_loc_info(&arg->loc_info, "out parameter \'%s\' of function \'%s\' is not a pointer\n", arg->name, funcname);
                break;
            case TGT_IFACE_POINTER:
                error_loc_info(&arg->loc_info, "out interface pointer \'%s\' of function \'%s\' is not a double pointer\n", arg->name, funcname);
                break;
            case TGT_STRING:
                if (is_array(type))
                {
                    /* needs conformance or fixed dimension */
                    if (type_array_has_conformance(type) &&
                        type_array_get_conformance(type)->type != EXPR_VOID) break;
                    if (!type_array_has_conformance(type) && type_array_get_dim(type)) break;
                }
                if (is_attr( arg->attrs, ATTR_IN )) break;
                error_loc_info(&arg->loc_info, "out parameter \'%s\' of function \'%s\' cannot be an unsized string\n", arg->name, funcname);
                break;
            case TGT_INVALID:
                /* already error'd before we get here */
            case TGT_CTXT_HANDLE_POINTER:
            case TGT_POINTER:
            case TGT_ARRAY:
                /* OK */
                break;
            }
        }

        check_field_common(func->type, funcname, arg);
    }

    if (type_get_type(type_function_get_rettype(func->type)) != TYPE_VOID)
    {
        var_t var;
        var = *func;
        var.type = type_function_get_rettype(func->type);
        var.name = xstrdup("return value");
        check_field_common(func->type, funcname, &var);
        free(var.name);
    }
}

static void add_explicit_handle_if_necessary(const type_t *iface, var_t *func)
{
    unsigned char explicit_fc, implicit_fc;

    /* check for a defined binding handle */
    if (!get_func_handle_var( iface, func, &explicit_fc, &implicit_fc ) || !explicit_fc)
    {
        /* no explicit handle specified so add
         * "[in] handle_t IDL_handle" as the first parameter to the
         * function */
        var_t *idl_handle = make_var(xstrdup("IDL_handle"));
        idl_handle->attrs = append_attr(NULL, make_attr(ATTR_IN));
        idl_handle->type = find_type_or_error("handle_t", 0);
        type_function_add_head_arg(func->type, idl_handle);
    }
}

static void check_functions(const type_t *iface, int is_inside_library)
{
    const statement_t *stmt;
    if (is_attr(iface->attrs, ATTR_EXPLICIT_HANDLE))
    {
        STATEMENTS_FOR_EACH_FUNC( stmt, type_iface_get_stmts(iface) )
        {
            var_t *func = stmt->u.var;
            add_explicit_handle_if_necessary(iface, func);
        }
    }
    if (!is_inside_library && !is_attr(iface->attrs, ATTR_LOCAL))
    {
        STATEMENTS_FOR_EACH_FUNC( stmt, type_iface_get_stmts(iface) )
        {
            const var_t *func = stmt->u.var;
            if (!is_attr(func->attrs, ATTR_LOCAL))
                check_remoting_args(func);
        }
    }
}

static char *concat_str(const char *prefix, const char *str)
{
    char *ret = xmalloc(strlen(prefix) + strlen(str) + 1);
    strcpy(ret, prefix);
    strcat(ret, str);
    return ret;
}

static int async_iface_attrs(attr_list_t *attrs, const attr_t *attr)
{
    switch(attr->type)
    {
    case ATTR_UUID:
        return 0;
    case ATTR_ASYNCUUID:
        append_attr(attrs, make_attrp(ATTR_UUID, attr->u.pval));
        return 0;
    default:
        return 1;
    }
}

static int arg_in_attrs(attr_list_t *attrs, const attr_t *attr)
{
    return attr->type != ATTR_OUT && attr->type != ATTR_RETVAL;
}

static int arg_out_attrs(attr_list_t *attrs, const attr_t *attr)
{
    return attr->type != ATTR_IN;
}

static void check_async_uuid(type_t *iface)
{
    statement_list_t *stmts = NULL;
    statement_t *stmt;
    type_t *async_iface;
    type_t *inherit;

    if (!is_attr(iface->attrs, ATTR_ASYNCUUID)) return;

    inherit = iface->details.iface->inherit;
    if (inherit && strcmp(inherit->name, "IUnknown"))
        inherit = inherit->details.iface->async_iface;
    if (!inherit)
        error_loc("async_uuid applied to an interface with incompatible parent\n");

    async_iface = get_type(TYPE_INTERFACE, concat_str("Async", iface->name), iface->namespace, 0);
    async_iface->attrs = map_attrs(iface->attrs, async_iface_attrs);

    STATEMENTS_FOR_EACH_FUNC( stmt, type_iface_get_stmts(iface) )
    {
        var_t *begin_func, *finish_func, *func = stmt->u.var, *arg;
        var_list_t *begin_args = NULL, *finish_args = NULL, *args;

        args = func->type->details.function->args;
        if (args) LIST_FOR_EACH_ENTRY(arg, args, var_t, entry)
        {
            if (is_attr(arg->attrs, ATTR_IN) || !is_attr(arg->attrs, ATTR_OUT))
                begin_args = append_var(begin_args, copy_var(arg, strdup(arg->name), arg_in_attrs));
            if (is_attr(arg->attrs, ATTR_OUT))
                finish_args = append_var(finish_args, copy_var(arg, strdup(arg->name), arg_out_attrs));
        }

        begin_func = copy_var(func, concat_str("Begin_", func->name), NULL);
        begin_func->type = type_new_function(begin_args);
        begin_func->type->attrs = func->attrs;
        begin_func->type->details.function->retval = func->type->details.function->retval;
        stmts = append_statement(stmts, make_statement_declaration(begin_func));

        finish_func = copy_var(func, concat_str("Finish_", func->name), NULL);
        finish_func->type = type_new_function(finish_args);
        finish_func->type->attrs = func->attrs;
        finish_func->type->details.function->retval = func->type->details.function->retval;
        stmts = append_statement(stmts, make_statement_declaration(finish_func));
    }

    type_interface_define(async_iface, inherit, stmts);
    iface->details.iface->async_iface = async_iface->details.iface->async_iface = async_iface;
}

static void check_statements(const statement_list_t *stmts, int is_inside_library)
{
    const statement_t *stmt;

    if (stmts) LIST_FOR_EACH_ENTRY(stmt, stmts, const statement_t, entry)
    {
        switch(stmt->type) {
        case STMT_LIBRARY:
            check_statements(stmt->u.lib->stmts, TRUE);
            break;
        case STMT_TYPE:
            switch(type_get_type(stmt->u.type)) {
            case TYPE_INTERFACE:
                check_functions(stmt->u.type, is_inside_library);
                break;
            case TYPE_COCLASS:
                if(winrt_mode)
                    error_loc("coclass is not allowed in Windows Runtime mode\n");
                break;
            default:
                break;
            }
            break;
        default:
            break;
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
    else if (stmt->type == STMT_TYPE && type_get_type(stmt->u.type) == TYPE_INTERFACE &&
             !is_local(stmt->u.type->attrs))
    {
      const statement_t *stmt_func;
      STATEMENTS_FOR_EACH_FUNC(stmt_func, type_iface_get_stmts(stmt->u.type)) {
        const var_t *func = stmt_func->u.var;
        check_for_additional_prototype_types(func->type->details.function->args);
      }
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
    else if (type_get_type(var->type) == TYPE_FUNCTION)
        check_function_attrs(var->name, var->attrs);
    else if (var->stgclass == STG_NONE || var->stgclass == STG_REGISTER)
        error_loc("instantiation of data is illegal\n");
    return stmt;
}

static statement_t *make_statement_library(typelib_t *typelib)
{
    statement_t *stmt = make_statement(STMT_LIBRARY);
    stmt->u.lib = typelib;
    return stmt;
}

static statement_t *make_statement_pragma(const char *str)
{
    statement_t *stmt = make_statement(STMT_PRAGMA);
    stmt->u.str = str;
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

static statement_t *make_statement_typedef(declarator_list_t *decls)
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
        type_t *type = find_type_or_error(var->name, 0);
        *type_list = xmalloc(sizeof(type_list_t));
        (*type_list)->type = type;
        (*type_list)->next = NULL;

        type_list = &(*type_list)->next;
        free(decl);
        free(var);
    }

    return stmt;
}

static statement_list_t *append_statements(statement_list_t *l1, statement_list_t *l2)
{
    if (!l2) return l1;
    if (!l1 || l1 == l2) return l2;
    list_move_tail (l1, l2);
    return l1;
}

static attr_list_t *append_attribs(attr_list_t *l1, attr_list_t *l2)
{
    if (!l2) return l1;
    if (!l1 || l1 == l2) return l2;
    list_move_tail (l1, l2);
    return l1;
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

void init_loc_info(loc_info_t *i)
{
    i->input_name = input_name ? input_name : "stdin";
    i->line_number = line_number;
    i->near_text = parser_text;
}

static void check_def(const type_t *t)
{
    if (t->defined)
        error_loc("%s: redefinition error; original definition was at %s:%d\n",
                  t->name, t->loc_info.input_name, t->loc_info.line_number);
}
