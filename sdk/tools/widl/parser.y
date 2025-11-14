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

struct _import_t
{
  char *name;
  int import_performed;
};

static str_list_t *append_str(str_list_t *list, char *str);
static decl_spec_t *make_decl_spec(type_t *type, decl_spec_t *left, decl_spec_t *right,
        enum storage_class stgclass, enum type_qualifier qual, enum function_specifier func_specifier);
static expr_list_t *append_expr(expr_list_t *list, expr_t *expr);
static var_t *declare_var(attr_list_t *attrs, decl_spec_t *decl_spec, declarator_t *decl, int top);
static var_list_t *set_var_types(attr_list_t *attrs, decl_spec_t *decl_spec, declarator_list_t *decls);
static var_list_t *append_var_list(var_list_t *list, var_list_t *vars);
static declarator_list_t *append_declarator(declarator_list_t *list, declarator_t *p);
static declarator_t *make_declarator(var_t *var);
static type_t *make_safearray(type_t *type);
static typelib_t *make_library(const char *name, const attr_list_t *attrs);
static void append_array(declarator_t *decl, expr_t *expr);
static void append_chain_type(declarator_t *decl, type_t *type, enum type_qualifier qual);
static void append_chain_callconv( struct location where, type_t *chain, char *callconv );
static warning_list_t *append_warning(warning_list_t *, int);

static type_t *reg_typedefs( struct location where, decl_spec_t *decl_spec, var_list_t *names, attr_list_t *attrs );
static type_t *find_type_or_error(struct namespace *parent, const char *name);
static struct namespace *find_namespace_or_error(struct namespace *namespace, const char *name);

static var_t *reg_const(var_t *var);

static void push_namespaces(str_list_t *names);
static void pop_namespaces(str_list_t *names);
static void push_parameters_namespace(const char *name);
static void pop_parameters_namespace(const char *name);

static statement_list_t *append_parameterized_type_stmts(statement_list_t *stmts);
static void check_statements(const statement_list_t *stmts, int is_inside_library);
static void check_all_user_types(const statement_list_t *stmts);
static void add_explicit_handle_if_necessary(const type_t *iface, var_t *func);

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
static statement_t *make_statement_typedef(var_list_t *names, bool is_defined);
static statement_t *make_statement_import(const char *str);
static statement_t *make_statement_parameterized_type(type_t *type, typeref_list_t *params);
static statement_t *make_statement_delegate(type_t *ret, var_list_t *args);
static statement_list_t *append_statement(statement_list_t *list, statement_t *stmt);
static statement_list_t *append_statements(statement_list_t *, statement_list_t *);

static struct namespace global_namespace = {
    NULL, NULL, LIST_INIT(global_namespace.entry), LIST_INIT(global_namespace.children)
};

static struct namespace *current_namespace = &global_namespace;
static struct namespace *parameters_namespace = NULL;
static statement_list_t *parameterized_type_stmts = NULL;

static typelib_t *current_typelib;

%}

%code requires
{

#define PARSER_LTYPE struct location

}

%code provides
{

int parser_lex( PARSER_STYPE *yylval, PARSER_LTYPE *yylloc );
void push_import( const char *fname, PARSER_LTYPE *yylloc );
PARSER_LTYPE pop_import(void);

# define YYLLOC_DEFAULT( cur, rhs, n ) \
        do { if (n) init_location( &(cur), &YYRHSLOC( rhs, 1 ), &YYRHSLOC( rhs, n ) ); \
             else init_location( &(cur), &YYRHSLOC( rhs, 0 ), NULL ); } while(0)

}

%define api.prefix {parser_}
%define api.pure full
%define parse.error verbose
%locations

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
	typeref_t *typeref;
	typeref_list_t *typeref_list;
	char *str;
	struct uuid *uuid;
	unsigned int num;
	struct integer integer;
	double dbl;
	typelib_t *typelib;
	struct _import_t *import;
	struct _decl_spec_t *declspec;
	enum storage_class stgclass;
	enum type_qualifier type_qualifier;
	enum function_specifier function_specifier;
	struct namespace *namespace;
}

%token <str> aIDENTIFIER aPRAGMA
%token <str> aKNOWNTYPE
%token <integer> aNUM aHEXNUM
%token <dbl> aDOUBLE
%token <str> aSTRING aWSTRING aSQSTRING
%token <str> tCDECL
%token <str> tFASTCALL
%token <str> tPASCAL
%token <str> tSTDCALL
%token <uuid> aUUID
%token aEOF aACF
%token SHL SHR
%token MEMBERPTR
%token EQUALITY INEQUALITY
%token GREATEREQUAL LESSEQUAL
%token LOGICALOR LOGICALAND
%token ELLIPSIS
%token tACTIVATABLE
%token tAGGREGATABLE
%token tAGILE
%token tALLNODES tALLOCATE tANNOTATION
%token tAPICONTRACT
%token tAPPOBJECT tASYNC tASYNCUUID
%token tAUTOHANDLE tBINDABLE tBOOLEAN tBROADCAST tBYTE tBYTECOUNT
%token tCALLAS tCALLBACK tCASE tCHAR tCOCLASS tCODE tCOMMSTATUS
%token tCOMPOSABLE
%token tCONST tCONTEXTHANDLE tCONTEXTHANDLENOSERIALIZE
%token tCONTEXTHANDLESERIALIZE
%token tCONTRACT
%token tCONTRACTVERSION
%token tCONTROL tCPPQUOTE
%token tCUSTOM
%token tDECLARE
%token tDECODE tDEFAULT tDEFAULTBIND
%token tDELEGATE
%token tDEFAULT_OVERLOAD
%token tDEFAULTCOLLELEM
%token tDEFAULTVALUE
%token tDEFAULTVTABLE
%token tDEPRECATED
%token tDISABLECONSISTENCYCHECK tDISPLAYBIND
%token tDISPINTERFACE
%token tDLLNAME tDONTFREE tDOUBLE tDUAL
%token tENABLEALLOCATE tENCODE tENDPOINT
%token tENTRY tENUM tERRORSTATUST
%token tEVENTADD tEVENTREMOVE
%token tEXCLUSIVETO
%token tEXPLICITHANDLE tEXTERN
%token tFALSE
%token tFAULTSTATUS
%token tFLAGS
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
%token tMARSHALINGBEHAVIOR
%token tMAYBE tMESSAGE
%token tMETHODS
%token tMODULE
%token tMTA
%token tNAMESPACE
%token tNOCODE tNONBROWSABLE
%token tNONCREATABLE
%token tNONE
%token tNONEXTENSIBLE
%token tNOTIFY tNOTIFYFLAG
%token tNULL
%token tOBJECT tODL tOLEAUTOMATION
%token tOPTIMIZE tOPTIONAL
%token tOUT
%token tOVERLOAD
%token tPARTIALIGNORE
%token tPOINTERDEFAULT
%token tPRAGMA_WARNING
%token tPROGID tPROPERTIES
%token tPROPGET tPROPPUT tPROPPUTREF
%token tPROTECTED
%token tPROXY tPTR
%token tPUBLIC
%token tRANGE
%token tREADONLY tREF
%token tREGISTER tREPRESENTAS
%token tREQUESTEDIT
%token tREQUIRES
%token tRESTRICTED
%token tRETVAL
%token tRUNTIMECLASS
%token tSAFEARRAY
%token tSHORT
%token tSIGNED tSINGLENODE
%token tSIZEIS tSIZEOF
%token tSMALL
%token tSOURCE
%token tSTANDARD
%token tSTATIC
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

%type <attr> access_attr
%type <attr> attribute acf_attribute
%type <attr_list> m_attributes attributes attrib_list
%type <attr_list> acf_attributes acf_attribute_list
%type <attr_list> dispattributes
%type <str_list> str_list
%type <expr> m_expr expr expr_const expr_int_const array m_bitfield
%type <expr_list> m_exprs /* exprs expr_list */ expr_list_int_const
%type <expr> contract_req
%type <expr> static_attr
%type <expr> activatable_attr
%type <expr> composable_attr
%type <expr> deprecated_attr
%type <type> delegatedef
%type <stgclass> storage_cls_spec
%type <type_qualifier> type_qualifier m_type_qual_list
%type <function_specifier> function_specifier
%type <declspec> decl_spec unqualified_decl_spec decl_spec_no_type m_decl_spec_no_type
%type <type> inherit interface interfacedef
%type <type> interfaceref
%type <type> dispinterfaceref
%type <type> dispinterface dispinterfacedef
%type <type> module moduledef
%type <str_list> namespacedef
%type <type> base_type int_std
%type <type> enumdef structdef uniondef typedecl
%type <type> type unqualified_type qualified_type
%type <type> type_parameter
%type <typeref_list> type_parameters
%type <type> parameterized_type
%type <type> parameterized_type_arg
%type <typeref_list> parameterized_type_args
%type <typeref> class_interface
%type <typeref_list> class_interfaces
%type <typeref_list> requires required_types
%type <var> arg ne_union_field union_field s_field case enum enum_member declaration
%type <var> funcdef
%type <var_list> m_args arg_list args dispint_meths
%type <var_list> fields ne_union_fields cases enums enum_list dispint_props field
%type <var> m_ident ident
%type <declarator> declarator direct_declarator init_declarator struct_declarator
%type <declarator> m_any_declarator any_declarator any_declarator_no_direct any_direct_declarator
%type <declarator> m_abstract_declarator abstract_declarator abstract_declarator_no_direct abstract_direct_declarator
%type <declarator_list> declarator_list struct_declarator_list
%type <type> coclass coclassdef
%type <type> runtimeclass runtimeclass_def
%type <type> apicontract apicontract_def
%type <num> contract_ver
%type <num> pointer_type threading_type marshaling_behavior version
%type <str> libraryhdr callconv cppquote importlib import
%type <str> typename m_typename
%type <str> import_start
%type <typelib> library_start librarydef
%type <statement> statement typedef pragma_warning
%type <stmt_list> gbl_statements imp_statements int_statements
%type <stmt_list> decl_block decl_statements
%type <stmt_list> imp_decl_block imp_decl_statements
%type <warning_list> warnings
%type <num> allocate_option_list allocate_option
%type <namespace> namespace_pfx

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

input: gbl_statements m_acf			{ $1 = append_parameterized_type_stmts($1);
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
                                                  (void)parser_nerrs;  /* avoid unused variable warning */
						}
	;

m_acf
	: %empty
	| aACF acf_statements
	;

decl_statements
	: %empty				{ $$ = NULL; }
	| decl_statements tINTERFACE qualified_type '<' parameterized_type_args '>' ';'
						{ parameterized_type_stmts = append_statement(parameterized_type_stmts, make_statement_parameterized_type($3, $5));
						  $$ = append_statement($1, make_statement_reference(type_parameterized_type_specialize_declare($3, $5)));
						}
	;

decl_block: tDECLARE '{' decl_statements '}' { $$ = $3; }
	;

imp_decl_statements
	: %empty				{ $$ = NULL; }
	| imp_decl_statements tINTERFACE qualified_type '<' parameterized_type_args '>' ';'
						{ $$ = append_statement($1, make_statement_reference(type_parameterized_type_specialize_declare($3, $5))); }
	;

imp_decl_block
	: tDECLARE '{' imp_decl_statements '}'	{ $$ = $3; }
	;

gbl_statements
	: %empty				{ $$ = NULL; }
	| gbl_statements namespacedef '{' { push_namespaces($2); } gbl_statements '}'
						{ pop_namespaces($2); $$ = append_statements($1, $5); }
	| gbl_statements interface ';'		{ $$ = append_statement($1, make_statement_reference($2)); }
	| gbl_statements dispinterface ';'	{ $$ = append_statement($1, make_statement_reference($2)); }
	| gbl_statements interfacedef		{ $$ = append_statement($1, make_statement_type_decl($2)); }
	| gbl_statements delegatedef		{ $$ = append_statement($1, make_statement_type_decl($2)); }
	| gbl_statements coclass ';'		{ $$ = $1;
						  reg_type($2, $2->name, current_namespace, 0);
						}
	| gbl_statements coclassdef		{ $$ = append_statement($1, make_statement_type_decl($2));
						  reg_type($2, $2->name, current_namespace, 0);
						}
	| gbl_statements apicontract ';'	{ $$ = $1; reg_type($2, $2->name, current_namespace, 0); }
	| gbl_statements apicontract_def	{ $$ = append_statement($1, make_statement_type_decl($2));
						  reg_type($2, $2->name, current_namespace, 0); }
	| gbl_statements runtimeclass ';'       { $$ = $1; reg_type($2, $2->name, current_namespace, 0); }
	| gbl_statements runtimeclass_def       { $$ = append_statement($1, make_statement_type_decl($2));
	                                          reg_type($2, $2->name, current_namespace, 0); }
	| gbl_statements moduledef		{ $$ = append_statement($1, make_statement_module($2)); }
	| gbl_statements librarydef		{ $$ = append_statement($1, make_statement_library($2)); }
	| gbl_statements statement		{ $$ = append_statement($1, $2); }
	| gbl_statements decl_block		{ $$ = append_statements($1, $2); }
	;

imp_statements
	: %empty				{ $$ = NULL; }
	| imp_statements interface ';'		{ $$ = append_statement($1, make_statement_reference($2)); }
	| imp_statements dispinterface ';'	{ $$ = append_statement($1, make_statement_reference($2)); }
	| imp_statements namespacedef '{' { push_namespaces($2); } imp_statements '}'
						{ pop_namespaces($2); $$ = append_statements($1, $5); }
	| imp_statements interfacedef		{ $$ = append_statement($1, make_statement_type_decl($2)); }
	| imp_statements delegatedef		{ $$ = append_statement($1, make_statement_type_decl($2)); }
	| imp_statements coclass ';'		{ $$ = $1; reg_type($2, $2->name, current_namespace, 0); }
	| imp_statements coclassdef		{ $$ = append_statement($1, make_statement_type_decl($2));
						  reg_type($2, $2->name, current_namespace, 0);
						}
	| imp_statements apicontract ';'	{ $$ = $1; reg_type($2, $2->name, current_namespace, 0); }
	| imp_statements apicontract_def	{ $$ = append_statement($1, make_statement_type_decl($2));
						  reg_type($2, $2->name, current_namespace, 0); }
	| imp_statements runtimeclass ';'       { $$ = $1; reg_type($2, $2->name, current_namespace, 0); }
	| imp_statements runtimeclass_def       { $$ = append_statement($1, make_statement_type_decl($2));
	                                          reg_type($2, $2->name, current_namespace, 0); }
	| imp_statements moduledef		{ $$ = append_statement($1, make_statement_module($2)); }
	| imp_statements statement		{ $$ = append_statement($1, $2); }
	| imp_statements importlib		{ $$ = append_statement($1, make_statement_importlib($2)); }
	| imp_statements librarydef		{ $$ = append_statement($1, make_statement_library($2)); }
	| imp_statements imp_decl_block		{ $$ = append_statements($1, $2); }
	;

int_statements
	: %empty				{ $$ = NULL; }
	| int_statements statement		{ $$ = append_statement($1, $2); }
	;

semicolon_opt
	: %empty
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
                          error_loc("expected \"disable\", \"enable\" or \"default\"\n");
                  }
              | tPRAGMA_WARNING '(' tDEFAULT ':' warnings ')'
                  {
                      $$ = NULL;
                      do_warning("default", $5);
                  }
	;

warnings:
	  aNUM { $$ = append_warning(NULL, $1.value); }
	| warnings aNUM { $$ = append_warning($1, $2.value); }
	;

typedecl:
	  enumdef
	| tENUM typename                        { $$ = type_new_enum($2, current_namespace, FALSE, NULL, &@$); }
	| structdef
	| tSTRUCT typename                      { $$ = type_new_struct($2, current_namespace, FALSE, NULL, &@$); }
	| uniondef
	| tUNION typename                       { $$ = type_new_nonencapsulated_union($2, current_namespace, FALSE, NULL, &@$); }
	| attributes enumdef                    { $$ = $2; $$->attrs = check_enum_attrs($1); }
	| attributes structdef                  { $$ = $2; $$->attrs = check_struct_attrs($1); }
	| attributes uniondef                   { $$ = $2; $$->attrs = check_union_attrs($1); }
	;

cppquote: tCPPQUOTE '(' aSTRING ')'		{ $$ = $3; }
	;

import_start: tIMPORT aSTRING ';'		{ $$ = $2; push_import( $2, &yylloc ); }
	;
import: import_start imp_statements aEOF	{ yylloc = pop_import(); }
	;

importlib: tIMPORTLIB '(' aSTRING ')'
/* ifdef __REACTOS__ */
	   semicolon_opt			{ $$ = $3; if(!parse_only) add_importlib($3); }
/* else
	   semicolon_opt			{ $$ = $3; if(!parse_only) add_importlib($3, current_typelib); }
*/
	;

libraryhdr: tLIBRARY typename			{ $$ = $2; }
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

m_args
	: %empty				{ $$ = NULL; }
	| args
	;

arg_list: arg					{ check_arg_attrs($1); $$ = append_var( NULL, $1 ); }
	| arg_list ',' arg			{ check_arg_attrs($3); $$ = append_var( $1, $3 ); }
	;

args:	  arg_list
	| arg_list ',' ELLIPSIS			{ $$ = append_var( $1, make_var(xstrdup("...")) ); }
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

m_attributes
	: %empty				{ $$ = NULL; }
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

marshaling_behavior:
	  tAGILE				{ $$ = MARSHALING_AGILE; }
	| tNONE					{ $$ = MARSHALING_NONE; }
	| tSTANDARD				{ $$ = MARSHALING_STANDARD; }
	;

contract_ver:
	  aNUM					{ $$ = MAKEVERSION(0, $1.value); }
	| aNUM '.' aNUM				{ $$ = MAKEVERSION($3.value, $1.value); }
	;

contract_req
        : decl_spec ',' contract_ver            {
                                                  struct integer integer = {.value = $3};
                                                  if ($1->type->type_type != TYPE_APICONTRACT)
                                                    error_loc("type %s is not an apicontract\n", $1->type->name);
                                                  $$ = make_exprl(EXPR_NUM, &integer);
                                                  $$ = make_exprt(EXPR_GTREQL, declare_var(NULL, $1, make_declarator(NULL), 0), $$);
                                                }
        ;

static_attr
	: decl_spec ',' contract_req		{ if ($1->type->type_type != TYPE_INTERFACE)
						      error_loc("type %s is not an interface\n", $1->type->name);
						  $$ = make_exprt(EXPR_MEMBER, declare_var(NULL, $1, make_declarator(NULL), 0), $3);
						}
	;

activatable_attr:
	  decl_spec ',' contract_req		{ if ($1->type->type_type != TYPE_INTERFACE)
						      error_loc("type %s is not an interface\n", $1->type->name);
						  $$ = make_exprt(EXPR_MEMBER, declare_var(NULL, $1, make_declarator(NULL), 0), $3);
						}
	| contract_req				{ $$ = $1; } /* activatable on the default activation factory */
	;

access_attr
        : tPUBLIC                               { $$ = attr_int( @$, ATTR_PUBLIC, 0 ); }
        | tPROTECTED                            { $$ = attr_int( @$, ATTR_PROTECTED, 0 ); }
        ;

composable_attr
        : decl_spec ',' access_attr ',' contract_req
                                                { if ($1->type->type_type != TYPE_INTERFACE)
                                                      error_loc( "type %s is not an interface\n", $1->type->name );
                                                  $$ = make_exprt( EXPR_MEMBER, declare_var( append_attr( NULL, $3 ), $1, make_declarator( NULL ), 0 ), $5 );
                                                }
        ;

deprecated_attr
        : aSTRING ',' aIDENTIFIER ',' contract_req
                                                { $$ = make_expr3( EXPR_MEMBER, make_exprs( EXPR_STRLIT, $1 ), make_exprs( EXPR_IDENTIFIER, $3 ), $5 ); }
        ;

attribute
        : %empty                                { $$ = NULL; }
        | tACTIVATABLE '(' activatable_attr ')' { $$ = attr_ptr( @$, ATTR_ACTIVATABLE, $3 ); }
        | tAGGREGATABLE                         { $$ = attr_int( @$, ATTR_AGGREGATABLE, 0 ); }
        | tANNOTATION '(' aSTRING ')'           { $$ = attr_ptr( @$, ATTR_ANNOTATION, $3 ); }
        | tAPPOBJECT                            { $$ = attr_int( @$, ATTR_APPOBJECT, 0 ); }
        | tASYNC                                { $$ = attr_int( @$, ATTR_ASYNC, 0 ); }
        | tAUTOHANDLE                           { $$ = attr_int( @$, ATTR_AUTO_HANDLE, 0 ); }
        | tBINDABLE                             { $$ = attr_int( @$, ATTR_BINDABLE, 0 ); }
        | tBROADCAST                            { $$ = attr_int( @$, ATTR_BROADCAST, 0 ); }
        | tCALLAS '(' ident ')'                 { $$ = attr_ptr( @$, ATTR_CALLAS, $3 ); }
        | tCASE '(' expr_list_int_const ')'     { $$ = attr_ptr( @$, ATTR_CASE, $3 ); }
        | tCODE                                 { $$ = attr_int( @$, ATTR_CODE, 0 ); }
        | tCOMPOSABLE '(' composable_attr ')'   { $$ = attr_ptr( @$, ATTR_COMPOSABLE, $3 ); }
        | tCOMMSTATUS                           { $$ = attr_int( @$, ATTR_COMMSTATUS, 0 ); }
        | tCONTEXTHANDLE                        { $$ = attr_int( @$, ATTR_CONTEXTHANDLE, 0 ); }
        | tCONTEXTHANDLENOSERIALIZE             { $$ = attr_int( @$, ATTR_CONTEXTHANDLE, 0 ); /* RPC_CONTEXT_HANDLE_DONT_SERIALIZE */ }
        | tCONTEXTHANDLESERIALIZE               { $$ = attr_int( @$, ATTR_CONTEXTHANDLE, 0 ); /* RPC_CONTEXT_HANDLE_SERIALIZE */ }
        | tCONTRACT '(' contract_req ')'        { $$ = attr_ptr( @$, ATTR_CONTRACT, $3 ); }
        | tCONTRACTVERSION '(' contract_ver ')' { $$ = attr_int( @$, ATTR_CONTRACTVERSION, $3 ); }
        | tCONTROL                              { $$ = attr_int( @$, ATTR_CONTROL, 0 ); }
        | tCUSTOM '(' aUUID ',' expr_const ')'  { attr_custdata_t *data = xmalloc( sizeof(*data) );
                                                  data->id = *$3; data->pval = $5;
                                                  $$ = attr_ptr( @$, ATTR_CUSTOM, data );
                                                }
        | tDECODE                               { $$ = attr_int( @$, ATTR_DECODE, 0 ); }
        | tDEFAULT                              { $$ = attr_int( @$, ATTR_DEFAULT, 0 ); }
        | tDEFAULT_OVERLOAD                     { $$ = attr_int( @$, ATTR_DEFAULT_OVERLOAD, 0 ); }
        | tDEFAULTBIND                          { $$ = attr_int( @$, ATTR_DEFAULTBIND, 0 ); }
        | tDEFAULTCOLLELEM                      { $$ = attr_int( @$, ATTR_DEFAULTCOLLELEM, 0 ); }
        | tDEFAULTVALUE '(' expr_const ')'      { $$ = attr_ptr( @$, ATTR_DEFAULTVALUE, $3 ); }
        | tDEFAULTVTABLE                        { $$ = attr_int( @$, ATTR_DEFAULTVTABLE, 0 ); }
        | tDEPRECATED '(' deprecated_attr ')'   { $$ = attr_ptr( @$, ATTR_DEPRECATED, $3 ); }
        | tDISABLECONSISTENCYCHECK              { $$ = attr_int( @$, ATTR_DISABLECONSISTENCYCHECK, 0 ); }
        | tDISPLAYBIND                          { $$ = attr_int( @$, ATTR_DISPLAYBIND, 0 ); }
        | tDLLNAME '(' aSTRING ')'              { $$ = attr_ptr( @$, ATTR_DLLNAME, $3 ); }
        | tDUAL                                 { $$ = attr_int( @$, ATTR_DUAL, 0 ); }
        | tENABLEALLOCATE                       { $$ = attr_int( @$, ATTR_ENABLEALLOCATE, 0 ); }
        | tENCODE                               { $$ = attr_int( @$, ATTR_ENCODE, 0 ); }
        | tENDPOINT '(' str_list ')'            { $$ = attr_ptr( @$, ATTR_ENDPOINT, $3 ); }
        | tENTRY '(' expr_const ')'             { $$ = attr_ptr( @$, ATTR_ENTRY, $3 ); }
        | tEVENTADD                             { $$ = attr_int( @$, ATTR_EVENTADD, 0 ); }
        | tEVENTREMOVE                          { $$ = attr_int( @$, ATTR_EVENTREMOVE, 0 ); }
        | tEXCLUSIVETO '(' decl_spec ')'        { if ($3->type->type_type != TYPE_RUNTIMECLASS)
                                                      error_loc( "type %s is not a runtimeclass\n", $3->type->name );
                                                  $$ = attr_ptr( @$, ATTR_EXCLUSIVETO, $3->type );
                                                }
        | tEXPLICITHANDLE                       { $$ = attr_int( @$, ATTR_EXPLICIT_HANDLE, 0 ); }
        | tFAULTSTATUS                          { $$ = attr_int( @$, ATTR_FAULTSTATUS, 0 ); }
        | tFLAGS                                { $$ = attr_int( @$, ATTR_FLAGS, 0 ); }
        | tFORCEALLOCATE                        { $$ = attr_int( @$, ATTR_FORCEALLOCATE, 0 ); }
        | tHANDLE                               { $$ = attr_int( @$, ATTR_HANDLE, 0 ); }
        | tHELPCONTEXT '(' expr_int_const ')'   { $$ = attr_ptr( @$, ATTR_HELPCONTEXT, $3 ); }
        | tHELPFILE '(' aSTRING ')'             { $$ = attr_ptr( @$, ATTR_HELPFILE, $3 ); }
        | tHELPSTRING '(' aSTRING ')'           { $$ = attr_ptr( @$, ATTR_HELPSTRING, $3 ); }
        | tHELPSTRINGCONTEXT '(' expr_int_const ')'
                                                { $$ = attr_ptr( @$, ATTR_HELPSTRINGCONTEXT, $3 ); }
        | tHELPSTRINGDLL '(' aSTRING ')'        { $$ = attr_ptr( @$, ATTR_HELPSTRINGDLL, $3 ); }
        | tHIDDEN                               { $$ = attr_int( @$, ATTR_HIDDEN, 0 ); }
        | tID '(' expr_int_const ')'            { $$ = attr_ptr( @$, ATTR_ID, $3 ); }
        | tIDEMPOTENT                           { $$ = attr_int( @$, ATTR_IDEMPOTENT, 0 ); }
        | tIGNORE                               { $$ = attr_int( @$, ATTR_IGNORE, 0 ); }
        | tIIDIS '(' expr ')'                   { $$ = attr_ptr( @$, ATTR_IIDIS, $3 ); }
        | tIMMEDIATEBIND                        { $$ = attr_int( @$, ATTR_IMMEDIATEBIND, 0 ); }
        | tIMPLICITHANDLE '(' arg ')'           { $$ = attr_ptr( @$, ATTR_IMPLICIT_HANDLE, $3 ); }
        | tIN                                   { $$ = attr_int( @$, ATTR_IN, 0 ); }
        | tINPUTSYNC                            { $$ = attr_int( @$, ATTR_INPUTSYNC, 0 ); }
        | tLENGTHIS '(' m_exprs ')'             { $$ = attr_ptr( @$, ATTR_LENGTHIS, $3 ); }
        | tLCID '(' expr_int_const ')'          { $$ = attr_ptr( @$, ATTR_LIBLCID, $3 ); }
        | tLCID                                 { $$ = attr_int( @$, ATTR_PARAMLCID, 0 ); }
        | tLICENSED                             { $$ = attr_int( @$, ATTR_LICENSED, 0 ); }
        | tLOCAL                                { $$ = attr_int( @$, ATTR_LOCAL, 0 ); }
        | tMARSHALINGBEHAVIOR '(' marshaling_behavior ')'
                                                { $$ = attr_int( @$, ATTR_MARSHALING_BEHAVIOR, $3 ); }
        | tMAYBE                                { $$ = attr_int( @$, ATTR_MAYBE, 0 ); }
        | tMESSAGE                              { $$ = attr_int( @$, ATTR_MESSAGE, 0 ); }
        | tNOCODE                               { $$ = attr_int( @$, ATTR_NOCODE, 0 ); }
        | tNONBROWSABLE                         { $$ = attr_int( @$, ATTR_NONBROWSABLE, 0 ); }
        | tNONCREATABLE                         { $$ = attr_int( @$, ATTR_NONCREATABLE, 0 ); }
        | tNONEXTENSIBLE                        { $$ = attr_int( @$, ATTR_NONEXTENSIBLE, 0 ); }
        | tNOTIFY                               { $$ = attr_int( @$, ATTR_NOTIFY, 0 ); }
        | tNOTIFYFLAG                           { $$ = attr_int( @$, ATTR_NOTIFYFLAG, 0 ); }
        | tOBJECT                               { $$ = attr_int( @$, ATTR_OBJECT, 0 ); }
        | tODL                                  { $$ = attr_int( @$, ATTR_ODL, 0 ); }
        | tOLEAUTOMATION                        { $$ = attr_int( @$, ATTR_OLEAUTOMATION, 0 ); }
        | tOPTIMIZE '(' aSTRING ')'             { $$ = attr_ptr( @$, ATTR_OPTIMIZE, $3 ); }
        | tOPTIONAL                             { $$ = attr_int( @$, ATTR_OPTIONAL, 0 ); }
        | tOUT                                  { $$ = attr_int( @$, ATTR_OUT, 0 ); }
        | tOVERLOAD '(' aSTRING ')'             { $$ = attr_ptr( @$, ATTR_OVERLOAD, $3 ); }
        | tPARTIALIGNORE                        { $$ = attr_int( @$, ATTR_PARTIALIGNORE, 0 ); }
        | tPOINTERDEFAULT '(' pointer_type ')'  { $$ = attr_int( @$, ATTR_POINTERDEFAULT, $3 ); }
        | tPROGID '(' aSTRING ')'               { $$ = attr_ptr( @$, ATTR_PROGID, $3 ); }
        | tPROPGET                              { $$ = attr_int( @$, ATTR_PROPGET, 0 ); }
        | tPROPPUT                              { $$ = attr_int( @$, ATTR_PROPPUT, 0 ); }
        | tPROPPUTREF                           { $$ = attr_int( @$, ATTR_PROPPUTREF, 0 ); }
        | tPROTECTED                            { $$ = attr_int( @$, ATTR_PROTECTED, 0 ); }
        | tPROXY                                { $$ = attr_int( @$, ATTR_PROXY, 0 ); }
        | tPUBLIC                               { $$ = attr_int( @$, ATTR_PUBLIC, 0 ); }
        | tRANGE '(' expr_int_const ',' expr_int_const ')'
                                                { expr_list_t *list = append_expr( NULL, $3 );
                                                  list = append_expr( list, $5 );
                                                  $$ = attr_ptr( @$, ATTR_RANGE, list );
                                                }
        | tREADONLY                             { $$ = attr_int( @$, ATTR_READONLY, 0 ); }
        | tREPRESENTAS '(' type ')'             { $$ = attr_ptr( @$, ATTR_REPRESENTAS, $3 ); }
        | tREQUESTEDIT                          { $$ = attr_int( @$, ATTR_REQUESTEDIT, 0 ); }
        | tRESTRICTED                           { $$ = attr_int( @$, ATTR_RESTRICTED, 0 ); }
        | tRETVAL                               { $$ = attr_int( @$, ATTR_RETVAL, 0 ); }
        | tSIZEIS '(' m_exprs ')'               { $$ = attr_ptr( @$, ATTR_SIZEIS, $3 ); }
        | tSOURCE                               { $$ = attr_int( @$, ATTR_SOURCE, 0 ); }
        | tSTATIC '(' static_attr ')'           { $$ = attr_ptr( @$, ATTR_STATIC, $3 ); }
        | tSTRICTCONTEXTHANDLE                  { $$ = attr_int( @$, ATTR_STRICTCONTEXTHANDLE, 0 ); }
        | tSTRING                               { $$ = attr_int( @$, ATTR_STRING, 0 ); }
        | tSWITCHIS '(' expr ')'                { $$ = attr_ptr( @$, ATTR_SWITCHIS, $3 ); }
        | tSWITCHTYPE '(' type ')'              { $$ = attr_ptr( @$, ATTR_SWITCHTYPE, $3 ); }
        | tTRANSMITAS '(' type ')'              { $$ = attr_ptr( @$, ATTR_TRANSMITAS, $3 ); }
        | tTHREADING '(' threading_type ')'     { $$ = attr_int( @$, ATTR_THREADING, $3 ); }
        | tUIDEFAULT                            { $$ = attr_int( @$, ATTR_UIDEFAULT, 0 ); }
        | tUSESGETLASTERROR                     { $$ = attr_int( @$, ATTR_USESGETLASTERROR, 0 ); }
        | tUSERMARSHAL '(' type ')'             { $$ = attr_ptr( @$, ATTR_USERMARSHAL, $3 ); }
        | tUUID '(' aUUID ')'                   { $$ = attr_ptr( @$, ATTR_UUID, $3 ); }
        | tASYNCUUID '(' aUUID ')'              { $$ = attr_ptr( @$, ATTR_ASYNCUUID, $3 ); }
        | tV1ENUM                               { $$ = attr_int( @$, ATTR_V1ENUM, 0 ); }
        | tVARARG                               { $$ = attr_int( @$, ATTR_VARARG, 0 ); }
        | tVERSION '(' version ')'              { $$ = attr_int( @$, ATTR_VERSION, $3 ); }
        | tVIPROGID '(' aSTRING ')'             { $$ = attr_ptr( @$, ATTR_VIPROGID, $3 ); }
        | tWIREMARSHAL '(' type ')'             { $$ = attr_ptr( @$, ATTR_WIREMARSHAL, $3 ); }
        | pointer_type                          { $$ = attr_int( @$, ATTR_POINTERTYPE, $1 ); }
        ;

callconv: tCDECL
	| tFASTCALL
	| tPASCAL
	| tSTDCALL
	;

cases
	: %empty				{ $$ = NULL; }
	| cases case				{ $$ = append_var( $1, $2 ); }
	;

case    : tCASE expr_int_const ':' union_field  { attr_t *a = attr_ptr( @$, ATTR_CASE, append_expr( NULL, $2 ) );
                                                  $$ = $4; if (!$$) $$ = make_var( NULL );
                                                  $$->attrs = append_attr( $$->attrs, a );
                                                }
        | tDEFAULT ':' union_field              { attr_t *a = attr_int( @$, ATTR_DEFAULT, 0 );
                                                  $$ = $3; if (!$$) $$ = make_var( NULL );
                                                  $$->attrs = append_attr( $$->attrs, a );
                                                }
        ;

enums
	: %empty				{ $$ = NULL; }
	| enum_list ','				{ $$ = $1; }
	| enum_list
	;

enum_list: enum                                 {
                                                  struct integer integer = {.value = 0};
                                                  if (!$1->eval)
                                                    $1->eval = make_exprl(EXPR_NUM, &integer);
                                                  $$ = append_var( NULL, $1 );
                                                }
        | enum_list ',' enum                    {
                                                  if (!$3->eval)
                                                  {
                                                    var_t *last = LIST_ENTRY( list_tail($$), var_t, entry );
                                                    struct integer integer;

                                                    if (last->eval->type == EXPR_NUM)
                                                      integer.is_hex = last->eval->u.integer.is_hex;
                                                    integer.value = last->eval->cval + 1;
                                                    if (integer.value < 0)
                                                      integer.is_hex = TRUE;
                                                    $3->eval = make_exprl(EXPR_NUM, &integer);
                                                  }
                                                  $$ = append_var( $1, $3 );
                                                }
        ;

enum_member: m_attributes ident 		{ $$ = $2;
						  $$->attrs = check_enum_member_attrs($1);
						}
	;

enum:	  enum_member '=' expr_int_const	{ $$ = reg_const($1);
						  $$->eval = $3;
                                                  $$->declspec.type = type_new_int(TYPE_BASIC_INT, 0);
						}
	| enum_member				{ $$ = reg_const($1);
                                                  $$->declspec.type = type_new_int(TYPE_BASIC_INT, 0);
						}
	;

enumdef: tENUM m_typename '{' enums '}'		{ $$ = type_new_enum($2, current_namespace, TRUE, $4, &@2); }
	;

m_exprs:  m_expr                                { $$ = append_expr( NULL, $1 ); }
	| m_exprs ',' m_expr                    { $$ = append_expr( $1, $3 ); }
	;

m_expr
	: %empty				{ $$ = make_expr(EXPR_VOID); }
	| expr
	;

expr:     aNUM                                  { $$ = make_exprl(EXPR_NUM, &$1); }
        | aHEXNUM                               { $$ = make_exprl(EXPR_NUM, &$1); }
	| aDOUBLE				{ $$ = make_exprd(EXPR_DOUBLE, $1); }
        | tFALSE                                { struct integer integer = {.value = 0};
                                                  $$ = make_exprl(EXPR_TRUEFALSE, &integer); }
        | tNULL                                 { struct integer integer = {.value = 0};
                                                  $$ = make_exprl(EXPR_NUM, &integer); }
        | tTRUE                                 { struct integer integer = {.value = 1};
                                                  $$ = make_exprl(EXPR_TRUEFALSE, &integer); }
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
	| '(' unqualified_decl_spec m_abstract_declarator ')' expr %prec CAST
						{ $$ = make_exprt(EXPR_CAST, declare_var(NULL, $2, $3, 0), $5); free($2); free($3); }
	| tSIZEOF '(' unqualified_decl_spec m_abstract_declarator ')'
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

fields
	: %empty				{ $$ = NULL; }
	| fields field				{ $$ = append_var_list($1, $2); }
	;

field:	  m_attributes decl_spec struct_declarator_list ';'
						{ const char *first = LIST_ENTRY(list_head($3), declarator_t, entry)->var->name;
						  check_field_attrs(first, $1);
						  $$ = set_var_types($1, $2, $3);
						}
	| m_attributes uniondef ';'		{ var_t *v = make_var(NULL);
						  v->declspec.type = $2; v->attrs = $1;
						  $$ = append_var(NULL, v);
						}
	;

ne_union_field:
	  s_field ';'				{ $$ = $1; }
	| attributes ';'			{ $$ = make_var(NULL); $$->attrs = $1; }
        ;

ne_union_fields
	: %empty				{ $$ = NULL; }
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
						  v->declspec.type = $2; v->attrs = $1;
						  $$ = v;
						}
	;

funcdef: declaration				{ $$ = $1;
						  if (type_get_type($$->declspec.type) != TYPE_FUNCTION)
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

m_ident
	: %empty				{ $$ = NULL; }
	| ident
	;

m_typename
	: %empty				{ $$ = NULL; }
	| typename
	;

typename: aIDENTIFIER
	| aKNOWNTYPE
	;

ident:	  typename				{ $$ = make_var($1); }
	;

base_type: tBYTE				{ $$ = find_type_or_error( NULL, "byte" ); }
	| tWCHAR				{ $$ = find_type_or_error( NULL, "wchar_t" ); }
	| int_std
	| tSIGNED int_std			{ $$ = type_new_int(type_basic_get_type($2), -1); }
	| tUNSIGNED int_std			{ $$ = type_new_int(type_basic_get_type($2), 1); }
	| tUNSIGNED				{ $$ = type_new_int(TYPE_BASIC_INT, 1); }
	| tFLOAT				{ $$ = find_type_or_error( NULL, "float" ); }
	| tDOUBLE				{ $$ = find_type_or_error( NULL, "double" ); }
	| tBOOLEAN				{ $$ = find_type_or_error( NULL, "boolean" ); }
	| tERRORSTATUST				{ $$ = find_type_or_error( NULL, "error_status_t" ); }
	| tHANDLET				{ $$ = find_type_or_error( NULL, "handle_t" ); }
	;

m_int
	: %empty
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

namespace_pfx:
	  aIDENTIFIER '.'			{ $$ = find_namespace_or_error(&global_namespace, $1); }
	| namespace_pfx aIDENTIFIER '.'		{ $$ = find_namespace_or_error($1, $2); }
	;

qualified_type:
	  typename				{ $$ = find_type_or_error(current_namespace, $1); }
	| namespace_pfx typename		{ $$ = find_type_or_error($1, $2); }
	;

parameterized_type: qualified_type '<' parameterized_type_args '>'
						{ $$ = find_parameterized_type($1, $3); }
	;

parameterized_type_arg:
	  base_type				{ $$ = $1; }
	| qualified_type			{ $$ = $1; }
	| qualified_type '*'			{ $$ = type_new_pointer($1); }
	| parameterized_type			{ $$ = $1; }
	| parameterized_type '*'		{ $$ = type_new_pointer($1); }
	;

parameterized_type_args:
	  parameterized_type_arg		{ $$ = append_typeref(NULL, make_typeref($1)); }
	| parameterized_type_args ',' parameterized_type_arg
						{ $$ = append_typeref($1, make_typeref($3)); }
	;

coclass:  tCOCLASS typename			{ $$ = type_coclass_declare($2); }
	;

coclassdef: attributes coclass '{' class_interfaces '}' semicolon_opt
						{ $$ = type_coclass_define($2, $1, $4, &@2); }
	;

runtimeclass: tRUNTIMECLASS typename		{ $$ = type_runtimeclass_declare($2, current_namespace); }
	;

runtimeclass_def: attributes runtimeclass inherit '{' class_interfaces '}' semicolon_opt
						{ if ($3 && type_get_type($3) != TYPE_RUNTIMECLASS) error_loc("%s is not a runtimeclass\n", $3->name);
						  $$ = type_runtimeclass_define($2, $1, $5, &@2); }
	;

apicontract: tAPICONTRACT typename		{ $$ = type_apicontract_declare($2, current_namespace); }
	;

apicontract_def: attributes apicontract '{' '}' semicolon_opt
						{ $$ = type_apicontract_define($2, $1, &@2); }
	;

namespacedef: tNAMESPACE aIDENTIFIER		{ $$ = append_str( NULL, $2 ); }
	| namespacedef '.' aIDENTIFIER		{ $$ = append_str( $1, $3 ); }
	;

class_interfaces
	: %empty				{ $$ = NULL; }
	| class_interfaces class_interface	{ $$ = append_typeref( $1, $2 ); }
	;

class_interface:
	  m_attributes interfaceref ';'		{ $$ = make_typeref($2); $$->attrs = $1; }
	| m_attributes dispinterfaceref ';'	{ $$ = make_typeref($2); $$->attrs = $1; }
	;

dispinterface: tDISPINTERFACE typename		{ $$ = type_dispinterface_declare($2); }
	;

dispattributes: attributes                      { $$ = append_attr( $1, attr_int( @$, ATTR_DISPINTERFACE, 0 ) ); }
        ;

dispint_props: tPROPERTIES ':'			{ $$ = NULL; }
	| dispint_props s_field ';'		{ $$ = append_var( $1, $2 ); }
	;

dispint_meths: tMETHODS ':'			{ $$ = NULL; }
	| dispint_meths funcdef ';'		{ $$ = append_var( $1, $2 ); }
	;

dispinterfacedef:
	  dispattributes dispinterface '{' dispint_props dispint_meths '}'
						{ $$ = type_dispinterface_define($2, $1, $4, $5, &@2); }
	| dispattributes dispinterface '{' interface ';' '}'
						{ $$ = type_dispinterface_define_from_iface($2, $1, $4, &@2); }
	;

inherit
	: %empty				{ $$ = NULL; }
	| ':' qualified_type                    { $$ = $2; }
	| ':' parameterized_type		{ $$ = $2; }
	;

type_parameter: typename			{ $$ = get_type(TYPE_PARAMETER, $1, parameters_namespace, 0); }
	;

type_parameters:
	  type_parameter			{ $$ = append_typeref(NULL, make_typeref($1)); }
	| type_parameters ',' type_parameter	{ $$ = append_typeref($1, make_typeref($3)); }
	;

interface:
	  tINTERFACE typename			{ $$ = type_interface_declare($2, current_namespace); }
	| tINTERFACE typename '<' { push_parameters_namespace($2); } type_parameters { pop_parameters_namespace($2); } '>'
						{ $$ = type_parameterized_interface_declare($2, current_namespace, $5); }
	;

delegatedef: m_attributes tDELEGATE type ident '(' m_args ')' semicolon_opt
						{ $$ = type_delegate_declare($4->name, current_namespace);
						  $$ = type_delegate_define($$, $1, append_statement(NULL, make_statement_delegate($3, $6)), &@4);
						}
	| m_attributes tDELEGATE type ident
	  '<' { push_parameters_namespace($4->name); } type_parameters '>'
	  '(' m_args ')' { pop_parameters_namespace($4->name); } semicolon_opt
						{ $$ = type_parameterized_delegate_declare($4->name, current_namespace, $7);
						  $$ = type_parameterized_delegate_define($$, $1, append_statement(NULL, make_statement_delegate($3, $10)), &@4);
						}
	;

required_types:
	  qualified_type			{ $$ = append_typeref(NULL, make_typeref($1)); }
	| parameterized_type			{ $$ = append_typeref(NULL, make_typeref($1)); }
	| required_types ',' qualified_type	{ $$ = append_typeref($1, make_typeref($3)); }
	| required_types ',' parameterized_type	{ $$ = append_typeref($1, make_typeref($3)); }
	;

requires
	: %empty				{ $$ = NULL; }
	| tREQUIRES required_types		{ $$ = $2; }
	;

interfacedef: attributes interface		{ if ($2->type_type == TYPE_PARAMETERIZED_TYPE) push_parameters_namespace($2->name); }
	  inherit requires '{' int_statements '}' semicolon_opt
						{ if ($2->type_type == TYPE_PARAMETERIZED_TYPE)
						  {
						      $$ = type_parameterized_interface_define($2, $1, $4, $7, $5, &@2);
						      pop_parameters_namespace($2->name);
						  }
						  else
						  {
						      $$ = type_interface_define($2, $1, $4, $7, $5, &@2);
						      check_async_uuid($$);
						  }
						}
	| dispinterfacedef semicolon_opt	{ $$ = $1; }
	;

interfaceref:
	  tINTERFACE typename			{ $$ = get_type(TYPE_INTERFACE, $2, current_namespace, 0); }
	| tINTERFACE namespace_pfx typename	{ $$ = get_type(TYPE_INTERFACE, $3, $2, 0); }
	| tINTERFACE parameterized_type		{ if (type_get_type(($$ = $2)) != TYPE_INTERFACE) error_loc("%s is not an interface\n", $$->name); }
	;

dispinterfaceref:
	  tDISPINTERFACE typename		{ $$ = get_type(TYPE_INTERFACE, $2, current_namespace, 0); }
	;

module:   tMODULE typename			{ $$ = type_module_declare($2); }
	;

moduledef: m_attributes module '{' int_statements '}' semicolon_opt
						{ $$ = type_module_define($2, $1, $4, &@2); }
	;

storage_cls_spec:
	  tEXTERN				{ $$ = STG_EXTERN; }
	| tSTATIC				{ $$ = STG_STATIC; }
	| tREGISTER				{ $$ = STG_REGISTER; }
	;

function_specifier:
	  tINLINE				{ $$ = FUNCTION_SPECIFIER_INLINE; }
	;

type_qualifier:
	  tCONST				{ $$ = TYPE_QUALIFIER_CONST; }
	;

m_type_qual_list
	: %empty				{ $$ = 0; }
	| m_type_qual_list type_qualifier	{ $$ = $1 | $2; }
	;

decl_spec: type m_decl_spec_no_type		{ $$ = make_decl_spec($1, $2, NULL, STG_NONE, 0, 0); }
	| decl_spec_no_type type m_decl_spec_no_type
						{ $$ = make_decl_spec($2, $1, $3, STG_NONE, 0, 0); }
	;

unqualified_decl_spec: unqualified_type m_decl_spec_no_type
						{ $$ = make_decl_spec($1, $2, NULL, STG_NONE, 0, 0); }
	| decl_spec_no_type unqualified_type m_decl_spec_no_type
						{ $$ = make_decl_spec($2, $1, $3, STG_NONE, 0, 0); }
	;

m_decl_spec_no_type
	: %empty				{ $$ = NULL; }
	| decl_spec_no_type
	;

decl_spec_no_type:
	  type_qualifier m_decl_spec_no_type	{ $$ = make_decl_spec(NULL, $2, NULL, STG_NONE, $1, 0); }
	| function_specifier m_decl_spec_no_type  { $$ = make_decl_spec(NULL, $2, NULL, STG_NONE, 0, $1); }
	| storage_cls_spec m_decl_spec_no_type  { $$ = make_decl_spec(NULL, $2, NULL, $1, 0, 0); }
	;

declarator:
	  '*' m_type_qual_list declarator %prec PPTR
						{ $$ = $3; append_chain_type($$, type_new_pointer(NULL), $2); }
	| callconv declarator			{ $$ = $2; append_chain_callconv( @$, $$->type, $1 ); }
	| direct_declarator
	;

direct_declarator:
	  ident					{ $$ = make_declarator($1); }
	| '(' declarator ')'			{ $$ = $2; }
	| direct_declarator array		{ $$ = $1; append_array($$, $2); }
	| direct_declarator '(' m_args ')'	{ $$ = $1; append_chain_type($$, type_new_function($3), 0); }
	;

/* abstract declarator */
abstract_declarator:
	  '*' m_type_qual_list m_abstract_declarator %prec PPTR
						{ $$ = $3; append_chain_type($$, type_new_pointer(NULL), $2); }
	| callconv m_abstract_declarator	{ $$ = $2; append_chain_callconv( @$, $$->type, $1 ); }
	| abstract_direct_declarator
	;

/* abstract declarator without accepting direct declarator */
abstract_declarator_no_direct:
	  '*' m_type_qual_list m_any_declarator %prec PPTR
						{ $$ = $3; append_chain_type($$, type_new_pointer(NULL), $2); }
	| callconv m_any_declarator		{ $$ = $2; append_chain_callconv( @$, $$->type, $1 ); }
	;

/* abstract declarator or empty */
m_abstract_declarator
	: %empty 				{ $$ = make_declarator(NULL); }
	| abstract_declarator
	;

/* abstract direct declarator */
abstract_direct_declarator:
	  '(' abstract_declarator_no_direct ')'	{ $$ = $2; }
	| abstract_direct_declarator array	{ $$ = $1; append_array($$, $2); }
	| array					{ $$ = make_declarator(NULL); append_array($$, $1); }
	| '(' m_args ')'
						{ $$ = make_declarator(NULL);
						  append_chain_type($$, type_new_function($2), 0);
						}
	| abstract_direct_declarator '(' m_args ')'
						{ $$ = $1;
						  append_chain_type($$, type_new_function($3), 0);
						}
	;

/* abstract or non-abstract declarator */
any_declarator:
	  '*' m_type_qual_list m_any_declarator %prec PPTR
						{ $$ = $3; append_chain_type($$, type_new_pointer(NULL), $2); }
	| callconv m_any_declarator		{ $$ = $2; append_chain_callconv( @$, $$->type, $1 ); }
	| any_direct_declarator
	;

/* abstract or non-abstract declarator without accepting direct declarator */
any_declarator_no_direct:
	  '*' m_type_qual_list m_any_declarator %prec PPTR
						{ $$ = $3; append_chain_type($$, type_new_pointer(NULL), $2); }
	| callconv m_any_declarator		{ $$ = $2; append_chain_callconv( @$, $$->type, $1 ); }
	;

/* abstract or non-abstract declarator or empty */
m_any_declarator
	: %empty 				{ $$ = make_declarator(NULL); }
	| any_declarator
	;

/* abstract or non-abstract direct declarator. note: direct declarators
 * aren't accepted inside brackets to avoid ambiguity with the rule for
 * function arguments */
any_direct_declarator:
	  ident					{ $$ = make_declarator($1); }
	| '(' any_declarator_no_direct ')'	{ $$ = $2; }
	| any_direct_declarator array		{ $$ = $1; append_array($$, $2); }
	| array					{ $$ = make_declarator(NULL); append_array($$, $1); }
	| '(' m_args ')'
						{ $$ = make_declarator(NULL);
						  append_chain_type($$, type_new_function($2), 0);
						}
	| any_direct_declarator '(' m_args ')'
						{ $$ = $1;
						  append_chain_type($$, type_new_function($3), 0);
						}
	;

declarator_list:
	  declarator				{ $$ = append_declarator( NULL, $1 ); }
	| declarator_list ',' declarator	{ $$ = append_declarator( $1, $3 ); }
	;

m_bitfield
	: %empty				{ $$ = NULL; }
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
	| tMTA					{ $$ = THREADING_FREE; }
	;

pointer_type:
	  tREF					{ $$ = FC_RP; }
	| tUNIQUE				{ $$ = FC_UP; }
	| tPTR					{ $$ = FC_FP; }
	;

structdef: tSTRUCT m_typename '{' fields '}'	{ $$ = type_new_struct($2, current_namespace, TRUE, $4, &@2); }
	;

unqualified_type:
          tVOID                                 { $$ = type_new_void(); }
        | base_type                             { $$ = $1; }
        | enumdef                               { $$ = $1; }
        | tENUM typename                        { $$ = type_new_enum($2, current_namespace, FALSE, NULL, &@$); }
        | structdef                             { $$ = $1; }
        | tSTRUCT typename                      { $$ = type_new_struct($2, current_namespace, FALSE, NULL, &@$); }
        | uniondef                              { $$ = $1; }
        | tUNION typename                       { $$ = type_new_nonencapsulated_union($2, current_namespace, FALSE, NULL, &@$); }
        | tSAFEARRAY '(' type ')'               { $$ = make_safearray($3); }
        | aKNOWNTYPE                            { $$ = find_type_or_error(current_namespace, $1); }
        ;

type:
	  unqualified_type
	| namespace_pfx typename		{ $$ = find_type_or_error($1, $2); }
	| parameterized_type			{ $$ = $1; }
	;

typedef: m_attributes tTYPEDEF m_attributes decl_spec declarator_list
						{ $1 = append_attribs($1, $3);
						  reg_typedefs( @$, $4, $5, check_typedef_attrs( $1 ) );
						  $$ = make_statement_typedef($5, $4->type->defined && !$4->type->defined_in_import);
						}
	;

uniondef: tUNION m_typename '{' ne_union_fields '}'
						{ $$ = type_new_nonencapsulated_union($2, current_namespace, TRUE, $4, &@2); }
	| tUNION m_typename
	  tSWITCH '(' s_field ')'
	  m_ident '{' cases '}'			{ $$ = type_new_encapsulated_union($2, $5, $7, $9, &@2); }
	;

version:
	  aNUM					{ $$ = MAKEVERSION($1.value, 0); }
	| aNUM '.' aNUM				{ $$ = MAKEVERSION($1.value, $3.value); }
	| aHEXNUM				{ $$ = $1.value; }
	;

acf_statements
        : %empty
        | acf_interface acf_statements
	;

acf_int_statements
        : %empty
        | acf_int_statement acf_int_statements
	;

acf_int_statement
        : tTYPEDEF acf_attributes aKNOWNTYPE ';'
                                                { type_t *type = find_type_or_error(current_namespace, $3);
                                                  type->attrs = append_attr_list(type->attrs, $2);
                                                }
	;

acf_interface
        : acf_attributes tINTERFACE aKNOWNTYPE '{' acf_int_statements '}'
                                                {  type_t *iface = find_type_or_error(current_namespace, $3);
                                                   if (type_get_type(iface) != TYPE_INTERFACE)
                                                       error_loc("%s is not an interface\n", iface->name);
                                                   iface->attrs = append_attr_list(iface->attrs, $1);
                                                }
	;

acf_attributes
        : %empty                                { $$ = NULL; }
        | '[' acf_attribute_list ']'            { $$ = $2; }
	;

acf_attribute_list
        : acf_attribute                         { $$ = append_attr(NULL, $1); }
        | acf_attribute_list ',' acf_attribute  { $$ = append_attr($1, $3); }
	;

acf_attribute
        : tALLOCATE '(' allocate_option_list ')'
                                                { $$ = attr_int( @$, ATTR_ALLOCATE, $3 ); }
        | tENCODE                               { $$ = attr_int( @$, ATTR_ENCODE, 0 ); }
        | tDECODE                               { $$ = attr_int( @$, ATTR_DECODE, 0 ); }
        | tEXPLICITHANDLE                       { $$ = attr_int( @$, ATTR_EXPLICIT_HANDLE, 0 ); }
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
    const decl_spec_t ds = {.type = t};
    reg_type(type_new_alias(&ds, name), name, NULL, 0);
}

void init_types(void)
{
  decl_builtin_basic("byte", TYPE_BASIC_BYTE);
  decl_builtin_basic("wchar_t", TYPE_BASIC_WCHAR);
  decl_builtin_basic("float", TYPE_BASIC_FLOAT);
  decl_builtin_basic("double", TYPE_BASIC_DOUBLE);
  decl_builtin_basic("error_status_t", TYPE_BASIC_ERROR_STATUS_T);
  decl_builtin_basic("handle_t", TYPE_BASIC_HANDLE);
  decl_builtin_alias("boolean", type_new_basic(TYPE_BASIC_CHAR));
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


static decl_spec_t *make_decl_spec(type_t *type, decl_spec_t *left, decl_spec_t *right,
        enum storage_class stgclass, enum type_qualifier qual, enum function_specifier func_specifier)
{
  decl_spec_t *declspec = left ? left : right;
  if (!declspec)
  {
    declspec = xmalloc(sizeof(*declspec));
    declspec->type = NULL;
    declspec->stgclass = STG_NONE;
    declspec->qualifier = 0;
    declspec->func_specifier = 0;
  }
  declspec->type = type;
  if (left && declspec != left)
  {
    if (declspec->stgclass == STG_NONE)
      declspec->stgclass = left->stgclass;
    else if (left->stgclass != STG_NONE)
      error_loc("only one storage class can be specified\n");
    declspec->qualifier |= left->qualifier;
    declspec->func_specifier |= left->func_specifier;
    assert(!left->type);
    free(left);
  }
  if (right && declspec != right)
  {
    if (declspec->stgclass == STG_NONE)
      declspec->stgclass = right->stgclass;
    else if (right->stgclass != STG_NONE)
      error_loc("only one storage class can be specified\n");
    declspec->qualifier |= right->qualifier;
    declspec->func_specifier |= right->func_specifier;
    assert(!right->type);
    free(right);
  }

  if (declspec->stgclass == STG_NONE)
    declspec->stgclass = stgclass;
  else if (stgclass != STG_NONE)
    error_loc("only one storage class can be specified\n");
  declspec->qualifier |= qual;
  declspec->func_specifier |= func_specifier;

  return declspec;
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

static void append_array(declarator_t *decl, expr_t *expr)
{
    type_t *array;

    if (!expr)
        return;

    /* An array is always a reference pointer unless explicitly marked otherwise
     * (regardless of what the default pointer attribute is). */
    array = type_new_array(NULL, NULL, FALSE, expr->is_const ? expr->cval : 0,
            expr->is_const ? NULL : expr, NULL);

    append_chain_type(decl, array, 0);
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

static type_t *get_chain_ref(type_t *type)
{
    if (is_ptr(type))
        return type_pointer_get_ref_type(type);
    else if (is_array(type))
        return type_array_get_element_type(type);
    else if (is_func(type))
        return type_function_get_rettype(type);
    return NULL;
}

static type_t *get_chain_end(type_t *type)
{
    type_t *inner;
    while ((inner = get_chain_ref(type)))
        type = inner;
    return type;
}

static void append_chain_type(declarator_t *decl, type_t *type, enum type_qualifier qual)
{
    type_t *chain_type;

    if (!decl->type)
    {
        decl->type = type;
        decl->qualifier = qual;
        return;
    }
    chain_type = get_chain_end(decl->type);

    if (is_ptr(chain_type))
    {
        chain_type->details.pointer.ref.type = type;
        chain_type->details.pointer.ref.qualifier = qual;
    }
    else if (is_array(chain_type))
    {
        chain_type->details.array.elem.type = type;
        chain_type->details.array.elem.qualifier = qual;
    }
    else if (is_func(chain_type))
    {
        chain_type->details.function->retval->declspec.type = type;
        chain_type->details.function->retval->declspec.qualifier = qual;
    }
    else
        assert(0);

    if (!is_func(chain_type))
        type->attrs = move_attr(type->attrs, chain_type->attrs, ATTR_CALLCONV);
}

static void append_chain_callconv( struct location where, type_t *chain, char *callconv )
{
    type_t *chain_end;

    if (chain && (chain_end = get_chain_end(chain)))
        chain_end->attrs = append_attr( chain_end->attrs, attr_ptr( where, ATTR_CALLCONV, callconv ) );
    else
        error_loc("calling convention applied to non-function type\n");
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

static var_t *declare_var(attr_list_t *attrs, decl_spec_t *decl_spec, declarator_t *decl,
                       int top)
{
  var_t *v = decl->var;
  expr_list_t *sizes = get_attrp(attrs, ATTR_SIZEIS);
  expr_list_t *lengs = get_attrp(attrs, ATTR_LENGTHIS);
  expr_t *dim;
  type_t **ptype;
  type_t *type = decl_spec->type;

  if (decl_spec->func_specifier & FUNCTION_SPECIFIER_INLINE)
  {
    if (!decl || !is_func(decl->type))
      error_loc("inline attribute applied to non-function type\n");
  }

  /* add type onto the end of the pointers in pident->type */
  append_chain_type(decl, type, decl_spec->qualifier);
  v->declspec = *decl_spec;
  v->declspec.type = decl->type;
  v->declspec.qualifier = decl->qualifier;
  v->attrs = attrs;
  v->is_defined = type->defined && !type->defined_in_import;

  if (is_attr(type->attrs, ATTR_CALLCONV) && !is_func(type))
    error_loc("calling convention applied to non-function type\n");

  /* check for pointer attribute being applied to non-pointer, non-array
   * type */
  if (!is_array(v->declspec.type))
  {
    int ptr_attr = get_attrv(v->attrs, ATTR_POINTERTYPE);
    const type_t *ptr = NULL;
    for (ptr = v->declspec.type; ptr && !ptr_attr; )
    {
      ptr_attr = get_attrv(ptr->attrs, ATTR_POINTERTYPE);
      if (!ptr_attr && type_is_alias(ptr))
        ptr = type_alias_get_aliasee_type(ptr);
      else
        break;
    }
    if (is_ptr(ptr))
    {
      if (ptr_attr && ptr_attr != FC_UP &&
          type_get_type(type_pointer_get_ref_type(ptr)) == TYPE_INTERFACE)
          warning_at( &v->where, "%s: pointer attribute applied to interface pointer type has no effect\n", v->name );
      if (!ptr_attr && top)
      {
        /* FIXME: this is a horrible hack to cope with the issue that we
         * store an offset to the typeformat string in the type object, but
         * two typeformat strings may be written depending on whether the
         * pointer is a toplevel parameter or not */
        v->declspec.type = duptype(v->declspec.type, 1);
      }
    }
    else if (ptr_attr)
       error_loc("%s: pointer attribute applied to non-pointer type\n", v->name);
  }

  if (is_attr(v->attrs, ATTR_STRING))
  {
    type_t *t = type;

    if (!is_ptr(v->declspec.type) && !is_array(v->declspec.type))
      error_loc("'%s': [string] attribute applied to non-pointer, non-array type\n",
                v->name);

    for (;;)
    {
        if (is_ptr(t))
            t = type_pointer_get_ref_type(t);
        else if (is_array(t))
            t = type_array_get_element_type(t);
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
    if (type_get_type_detect_alias(v->declspec.type) != TYPE_ENUM)
      error_loc("'%s': [v1_enum] attribute applied to non-enum type\n", v->name);
  }

  if (is_attr(v->attrs, ATTR_RANGE) && !is_allowed_range_type(v->declspec.type))
    error_loc("'%s': [range] attribute applied to non-integer type\n",
              v->name);

  ptype = &v->declspec.type;
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
                                  0, dim, NULL);
      }
      else if (is_ptr(*ptype))
        *ptype = type_new_array((*ptype)->name, type_pointer_get_ref(*ptype), TRUE,
                                0, dim, NULL);
      else
        error_loc("%s: size_is attribute applied to illegal type\n", v->name);
    }

    if (is_ptr(*ptype))
      ptype = &(*ptype)->details.pointer.ref.type;
    else if (is_array(*ptype))
      ptype = &(*ptype)->details.array.elem.type;
    else
      error_loc("%s: too many expressions in size_is attribute\n", v->name);
  }

  ptype = &v->declspec.type;
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
                                type_array_get_conformance(*ptype), dim);
      }
      else
        error_loc("%s: length_is attribute applied to illegal type\n", v->name);
    }

    if (is_ptr(*ptype))
      ptype = &(*ptype)->details.pointer.ref.type;
    else if (is_array(*ptype))
      ptype = &(*ptype)->details.array.elem.type;
    else
      error_loc("%s: too many expressions in length_is attribute\n", v->name);
  }

  if (decl->bits)
    v->declspec.type = type_new_bitfield(v->declspec.type, decl->bits);

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

typeref_list_t *append_typeref(typeref_list_t *list, typeref_t *ref)
{
    if (!ref) return list;
    if (!list)
    {
        list = xmalloc( sizeof(*list) );
        list_init( list );
    }
    list_add_tail( list, &ref->entry );
    return list;
}

typeref_t *make_typeref(type_t *type)
{
    typeref_t *ref = xmalloc(sizeof(typeref_t));
    ref->type = type;
    ref->attrs = NULL;
    return ref;
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
  init_declspec(&v->declspec, NULL);
  v->attrs = NULL;
  v->eval = NULL;
  init_location( &v->where, NULL, NULL );
  v->is_defined = 1;
  return v;
}

static var_t *copy_var(var_t *src, char *name, map_attrs_filter_t attr_filter)
{
  var_t *v = xmalloc(sizeof(var_t));
  v->name = name;
  v->declspec = src->declspec;
  v->attrs = map_attrs(src->attrs, attr_filter);
  v->eval = src->eval;
  v->where = src->where;
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
  d->qualifier = 0;
  d->bits = NULL;
  return d;
}

static type_t *make_safearray(type_t *type)
{
    decl_spec_t ds = {.type = type};
    ds.type = type_new_alias(&ds, "SAFEARRAY");
    return type_new_array(NULL, &ds, TRUE, 0, NULL, NULL);
}

static typelib_t *make_library(const char *name, const attr_list_t *attrs)
{
    typelib_t *typelib = xmalloc(sizeof(*typelib));
    memset(typelib, 0, sizeof(*typelib));
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

static void push_namespaces(str_list_t *names)
{
  const struct str_list_entry_t *name;
  LIST_FOR_EACH_ENTRY(name, names, const struct str_list_entry_t, entry)
    push_namespace(name->str);
}

static void pop_namespaces(str_list_t *names)
{
  const struct str_list_entry_t *name;
  LIST_FOR_EACH_ENTRY_REV(name, names, const struct str_list_entry_t, entry)
    pop_namespace(name->str);
}

static void push_parameters_namespace(const char *name)
{
    struct namespace *namespace;

    if (!(namespace = find_sub_namespace(current_namespace, name)))
    {
        namespace = xmalloc(sizeof(*namespace));
        namespace->name = xstrdup(name);
        namespace->parent = current_namespace;
        list_add_tail(&current_namespace->children, &namespace->entry);
        list_init(&namespace->children);
        memset(namespace->type_hash, 0, sizeof(namespace->type_hash));
    }

    parameters_namespace = namespace;
}

static void pop_parameters_namespace(const char *name)
{
    assert(!strcmp(parameters_namespace->name, name) && parameters_namespace->parent);
    parameters_namespace = NULL;
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
  {
    type->c_name = name;
    type->qualified_name = name;
  }
  else
  {
    type->c_name = format_namespace(namespace, "__x_", "_C", name, use_abi_namespace ? "ABI" : NULL);
    type->qualified_name = format_namespace(namespace, "", "::", name, use_abi_namespace ? "ABI" : NULL);
  }
  nt->type = type;
  nt->t = t;
  nt->next = namespace->type_hash[hash];
  namespace->type_hash[hash] = nt;
  return type;
}

static type_t *reg_typedefs( struct location where, decl_spec_t *decl_spec, declarator_list_t *decls, attr_list_t *attrs )
{
  declarator_t *decl;
  type_t *type = decl_spec->type;

  if (is_attr(attrs, ATTR_UUID) && !is_attr(attrs, ATTR_PUBLIC))
    attrs = append_attr( attrs, attr_int( where, ATTR_PUBLIC, 0 ) );

  /* We must generate names for tagless enum, struct or union.
     Typedef-ing a tagless enum, struct or union means we want the typedef
     to be included in a library hence the public attribute.  */
  if (type_get_type_detect_alias(type) == TYPE_ENUM ||
      type_get_type_detect_alias(type) == TYPE_STRUCT ||
      type_get_type_detect_alias(type) == TYPE_UNION ||
      type_get_type_detect_alias(type) == TYPE_ENCAPSULATED_UNION)
  {
    if (!type->name)
    {
      type->name = gen_name();
      if (!is_attr(attrs, ATTR_PUBLIC))
        attrs = append_attr( attrs, attr_int( where, ATTR_PUBLIC, 0 ) );
    }

    /* replace existing attributes when generating a typelib */
    if (do_typelib)
        type->attrs = attrs;
  }

#ifdef __REACTOS__ /* r53187 / 5bf224e */
  /* Append the SWITCHTYPE attribute to a non-encapsulated union if it does not already have it.  */
  if (type_get_type_detect_alias(type) == TYPE_UNION &&
      is_attr(attrs, ATTR_SWITCHTYPE) &&
      !is_attr(type->attrs, ATTR_SWITCHTYPE))
    type->attrs = append_attr(type->attrs, attr_ptr( where, ATTR_SWITCHTYPE, get_attrp(attrs, ATTR_SWITCHTYPE) ));
#endif

  LIST_FOR_EACH_ENTRY( decl, decls, declarator_t, entry )
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
      if (cur && input_name == cur->where.input_name)
          error_loc( "%s: redefinition error; original definition was at %s:%d\n",
                     cur->name, cur->where.input_name, cur->where.first_line );

      name = declare_var(attrs, decl_spec, decl, 0);
      cur = type_new_alias(&name->declspec, name->name);
      cur->attrs = attrs;

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

static type_t *find_type_or_error(struct namespace *namespace, const char *name)
{
    type_t *type;
    if (!(type = find_type(name, namespace, 0)) &&
        !(type = find_type(name, parameters_namespace, 0)))
    {
        error_loc("type '%s' not found in %s namespace\n", name, namespace && namespace->name ? namespace->name : "global");
        return NULL;
    }
    return type;
}

static struct namespace *find_namespace_or_error(struct namespace *parent, const char *name)
{
    struct namespace *namespace = NULL;

    if (!winrt_mode)
        error_loc("namespaces are only supported in winrt mode.\n");
    else if (!(namespace = find_sub_namespace(parent, name)))
        error_loc("namespace '%s' not found in '%s'\n", name, parent->name);

    return namespace;
}

int is_type(const char *name)
{
    return find_type(name, current_namespace, 0) != NULL ||
           find_type(name, parameters_namespace, 0);
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

char *gen_name(void)
{
  static unsigned long n = 0;
  static const char *file_id;

  if (! file_id)
  {
    char *dst = replace_extension( idl_name, ".idl", "" );
    file_id = dst;

    for (; *dst; ++dst)
      if (! isalnum((unsigned char) *dst))
        *dst = '_';
  }
  return strmake("__WIDL_%s_generated_name_%08lX", file_id, n++);
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
    case TYPE_RUNTIMECLASS:
    case TYPE_DELEGATE:
        return FALSE;
    case TYPE_APICONTRACT:
    case TYPE_PARAMETERIZED_TYPE:
    case TYPE_PARAMETER:
        /* not supposed to be here */
        assert(0);
        break;
    }
    return FALSE;
}

static int is_ptr_guid_type(const type_t *type)
{
    /* first, make sure it is a pointer to something */
    if (!is_ptr(type)) return FALSE;

    /* second, make sure it is a pointer to something of size sizeof(GUID),
     * i.e. 16 bytes */
    return (type_memsize(type_pointer_get_ref_type(type)) == 16);
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
                error_at( &arg->where, "expression must resolve to integral type <= 32bits for attribute %s\n", attr_name );
        }
    }
}

static void check_remoting_fields(const var_t *var, type_t *type);

/* checks that properties common to fields and arguments are consistent */
static void check_field_common(const type_t *container_type,
                               const char *container_name, const var_t *arg)
{
    type_t *type = arg->declspec.type;
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
        (is_attr(arg->attrs, ATTR_STRING) || is_aliaschain_attr(arg->declspec.type, ATTR_STRING)))
        error_at( &arg->where, "string and length_is specified for argument %s are mutually exclusive attributes\n", arg->name );

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
                error_at( &arg->where, "expression must resolve to pointer to GUID type for attribute iid_is\n" );
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
                error_at( &arg->where, "expression must resolve to integral type <= 32bits for attribute %s\n", expr_loc.attr );
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
            error_at( &arg->where, "%s \'%s\' of %s \'%s\' %s\n", var_type, arg->name, container_type_name, container_name, reason );
            break;
        }
        case TGT_CTXT_HANDLE:
        case TGT_CTXT_HANDLE_POINTER:
            if (type_get_type(container_type) != TYPE_FUNCTION)
                error_at( &arg->where, "%s \'%s\' of %s \'%s\' cannot be a context handle\n",
                          var_type, arg->name, container_type_name, container_name );
            break;
        case TGT_STRING:
        {
            const type_t *t = type;
            while (is_ptr(t))
                t = type_pointer_get_ref_type(t);
            if (is_aliaschain_attr(t, ATTR_RANGE))
                warning_at( &arg->where, "%s: range not verified for a string of ranged types\n", arg->name );
            break;
        }
        case TGT_POINTER:
            if (type_get_type(type_pointer_get_ref_type(type)) != TYPE_VOID ||
                !type->name || strcmp(type->name, "HANDLE"))
            {
                type = type_pointer_get_ref_type(type);
                more_to_do = TRUE;
            }
            break;
        case TGT_ARRAY:
            type = type_array_get_element_type(type);
            more_to_do = TRUE;
            break;
        case TGT_ENUM:
            type = type_get_real_type(type);
            if (!type_is_complete(type))
                error_at( &arg->where, "undefined type declaration \"enum %s\"\n", type->name );
        case TGT_USER_TYPE:
        case TGT_IFACE_POINTER:
        case TGT_BASIC:
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
            error_at( &var->where, "undefined type declaration \"struct %s\"\n", type->name );
    }
    else if (type_get_type(type) == TYPE_UNION || type_get_type(type) == TYPE_ENCAPSULATED_UNION)
    {
        if (type_is_complete(type))
            fields = type_union_get_cases(type);
        else
            error_at( &var->where, "undefined type declaration \"union %s\"\n", type->name );
    }

    if (fields) LIST_FOR_EACH_ENTRY( field, fields, const var_t, entry )
        if (field->declspec.type) check_field_common(type, type->name, field);
}

/* checks that arguments for a function make sense for marshalling and unmarshalling */
static void check_remoting_args(const var_t *func)
{
    const char *funcname = func->name;
    const var_t *arg;

    if (!type_function_get_args(func->declspec.type))
        return;

    LIST_FOR_EACH_ENTRY( arg, type_function_get_args(func->declspec.type), const var_t, entry )
    {
        const type_t *type = arg->declspec.type;

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
                error_at( &arg->where, "out parameter \'%s\' of function \'%s\' is not a pointer\n", arg->name, funcname );
                break;
            case TGT_IFACE_POINTER:
                error_at( &arg->where, "out interface pointer \'%s\' of function \'%s\' is not a double pointer\n", arg->name, funcname );
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
                error_at( &arg->where, "out parameter \'%s\' of function \'%s\' cannot be an unsized string\n", arg->name, funcname );
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

        check_field_common(func->declspec.type, funcname, arg);
    }

    if (type_get_type(type_function_get_rettype(func->declspec.type)) != TYPE_VOID)
    {
        var_t var;
        var = *func;
        var.declspec.type = type_function_get_rettype(func->declspec.type);
        var.name = xstrdup("return value");
        check_field_common(func->declspec.type, funcname, &var);
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
        idl_handle->attrs = append_attr( NULL, attr_int( iface->where, ATTR_IN, 0 ) );
        idl_handle->declspec.type = find_type_or_error(NULL, "handle_t");
        type_function_add_head_arg(func->declspec.type, idl_handle);
    }
}

static void check_functions(const type_t *iface, int is_inside_library)
{
    const statement_t *stmt;
    /* check for duplicates */
    if (is_attr(iface->attrs, ATTR_DISPINTERFACE))
    {
        var_list_t *methods = type_dispiface_get_methods(iface);
        var_t *func, *func_iter;

        if (methods) LIST_FOR_EACH_ENTRY( func, methods, var_t, entry )
        {
            LIST_FOR_EACH_ENTRY( func_iter, methods, var_t, entry )
            {
                if (func == func_iter) break;
                if (strcmp(func->name, func_iter->name)) continue;
                if (is_attr(func->attrs, ATTR_EVENTADD) != is_attr(func_iter->attrs, ATTR_EVENTADD)) continue;
                if (is_attr(func->attrs, ATTR_EVENTREMOVE) != is_attr(func_iter->attrs, ATTR_EVENTREMOVE)) continue;
                if (is_attr(func->attrs, ATTR_PROPGET) != is_attr(func_iter->attrs, ATTR_PROPGET)) continue;
                if (is_attr(func->attrs, ATTR_PROPPUT) != is_attr(func_iter->attrs, ATTR_PROPPUT)) continue;
                if (is_attr(func->attrs, ATTR_PROPPUTREF) != is_attr(func_iter->attrs, ATTR_PROPPUTREF)) continue;
                error_at( &func->where, "duplicated function \'%s\'\n", func->name );
            }
        }
    }
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

static int async_iface_attrs(attr_list_t *attrs, const attr_t *attr)
{
    switch(attr->type)
    {
    case ATTR_UUID:
        return 0;
    case ATTR_ASYNCUUID:
        append_attr( attrs, attr_ptr( attr->where, ATTR_UUID, attr->u.pval ) );
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

    inherit = type_iface_get_inherit(iface);
    if (inherit && strcmp(inherit->name, "IUnknown"))
        inherit = type_iface_get_async_iface(inherit);
    if (!inherit)
        error_loc("async_uuid applied to an interface with incompatible parent\n");

    async_iface = type_interface_declare(strmake("Async%s", iface->name), iface->namespace);

    STATEMENTS_FOR_EACH_FUNC( stmt, type_iface_get_stmts(iface) )
    {
        var_t *begin_func, *finish_func, *func = stmt->u.var, *arg;
        var_list_t *begin_args = NULL, *finish_args = NULL, *args;

        if (is_attr(func->attrs, ATTR_CALLAS)) continue;

        args = type_function_get_args(func->declspec.type);
        if (args) LIST_FOR_EACH_ENTRY(arg, args, var_t, entry)
        {
            if (is_attr(arg->attrs, ATTR_IN) || !is_attr(arg->attrs, ATTR_OUT))
                begin_args = append_var(begin_args, copy_var(arg, xstrdup(arg->name), arg_in_attrs));
            if (is_attr(arg->attrs, ATTR_OUT))
                finish_args = append_var(finish_args, copy_var(arg, xstrdup(arg->name), arg_out_attrs));
        }

        begin_func = copy_var(func, strmake("Begin_%s", func->name), NULL);
        begin_func->declspec.type = type_new_function(begin_args);
        begin_func->declspec.type->attrs = func->attrs;
        begin_func->declspec.type->details.function->retval = func->declspec.type->details.function->retval;
        stmts = append_statement(stmts, make_statement_declaration(begin_func));

        finish_func = copy_var(func, strmake("Finish_%s", func->name), NULL);
        finish_func->declspec.type = type_new_function(finish_args);
        finish_func->declspec.type->attrs = func->attrs;
        finish_func->declspec.type->details.function->retval = func->declspec.type->details.function->retval;
        stmts = append_statement(stmts, make_statement_declaration(finish_func));
    }

    type_interface_define(async_iface, map_attrs(iface->attrs, async_iface_attrs), inherit, stmts, NULL, &iface->where);
    iface->details.iface->async_iface = async_iface->details.iface->async_iface = async_iface;
}

static statement_list_t *append_parameterized_type_stmts(statement_list_t *stmts)
{
    statement_t *stmt, *next;

    if (stmts && parameterized_type_stmts) LIST_FOR_EACH_ENTRY_SAFE(stmt, next, parameterized_type_stmts, statement_t, entry)
    {
        switch(stmt->type)
        {
        case STMT_TYPE:
            stmt->u.type = type_parameterized_type_specialize_define(stmt->u.type);
            stmt->is_defined = 1;
            list_remove(&stmt->entry);
            stmts = append_statement(stmts, stmt);
            break;
        default:
            assert(0); /* should not be there */
            break;
        }
    }

    return stmts;
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
  const var_t *v;

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
        if (type_function_get_args(func->declspec.type))
          LIST_FOR_EACH_ENTRY( v, type_function_get_args(func->declspec.type), const var_t, entry )
            check_for_additional_prototype_types(v->declspec.type);
        check_for_additional_prototype_types(type_function_get_rettype(func->declspec.type));
      }
    }
  }
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
    stmt->is_defined = type->defined && !type->defined_in_import;
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
    if (var->declspec.stgclass == STG_EXTERN && var->eval)
        warning("'%s' initialised and declared extern\n", var->name);
    if (is_const_decl(var))
    {
        if (var->eval)
            reg_const(var);
    }
    else if (type_get_type(var->declspec.type) == TYPE_FUNCTION)
        check_function_attrs(var->name, var->attrs);
    else if (var->declspec.stgclass == STG_NONE || var->declspec.stgclass == STG_REGISTER)
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

static statement_t *make_statement_typedef(declarator_list_t *decls, bool is_defined)
{
    declarator_t *decl, *next;
    statement_t *stmt;

    if (!decls) return NULL;

    stmt = make_statement(STMT_TYPEDEF);
    stmt->u.type_list = NULL;
    stmt->is_defined = is_defined;

    LIST_FOR_EACH_ENTRY_SAFE( decl, next, decls, declarator_t, entry )
    {
        var_t *var = decl->var;
        type_t *type = find_type_or_error(current_namespace, var->name);
        stmt->u.type_list = append_typeref(stmt->u.type_list, make_typeref(type));
        free(decl);
        free(var);
    }

    return stmt;
}

static statement_t *make_statement_parameterized_type(type_t *type, typeref_list_t *params)
{
    statement_t *stmt = make_statement(STMT_TYPE);
    stmt->u.type = type_parameterized_type_specialize_partial(type, params);
    return stmt;
}

static statement_t *make_statement_delegate(type_t *ret, var_list_t *args)
{
    declarator_t *decl = make_declarator(make_var(xstrdup("Invoke")));
    decl_spec_t *spec = make_decl_spec(ret, NULL, NULL, STG_NONE, 0, 0);
    append_chain_type(decl, type_new_function(args), 0);
    return make_statement_declaration(declare_var(NULL, spec, decl, FALSE));
}

static statement_list_t *append_statements(statement_list_t *l1, statement_list_t *l2)
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

type_t *find_parameterized_type(type_t *type, typeref_list_t *params)
{
    char *name = format_parameterized_type_name(type, params);

    if (parameters_namespace)
    {
        assert(type->type_type == TYPE_PARAMETERIZED_TYPE);
        type = type_parameterized_type_specialize_partial(type, params);
    }
    else if ((type = find_type(name, type->namespace, 0)))
        assert(type->type_type != TYPE_PARAMETERIZED_TYPE);
    else
        error_loc("parameterized type '%s' not declared\n", name);

    free(name);
    return type;
}
