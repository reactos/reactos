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

#include "windef.h"

#include "widl.h"
#include "utils.h"
#include "parser.h"
#include "header.h"
#include "typelib.h"

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

static attr_t *make_attr(enum attr_type type);
static attr_t *make_attrv(enum attr_type type, unsigned long val);
static attr_t *make_attrp(enum attr_type type, void *val);
static expr_t *make_expr(enum expr_type type);
static expr_t *make_exprl(enum expr_type type, long val);
static expr_t *make_exprs(enum expr_type type, char *val);
static expr_t *make_exprt(enum expr_type type, typeref_t *tref, expr_t *expr);
static expr_t *make_expr1(enum expr_type type, expr_t *expr);
static expr_t *make_expr2(enum expr_type type, expr_t *exp1, expr_t *exp2);
static expr_t *make_expr3(enum expr_type type, expr_t *expr1, expr_t *expr2, expr_t *expr3);
static type_t *make_type(unsigned char type, type_t *ref);
static typeref_t *make_tref(char *name, type_t *ref);
static typeref_t *uniq_tref(typeref_t *ref);
static type_t *type_ref(typeref_t *ref);
static void set_type(var_t *v, typeref_t *ref, expr_t *arr);
static ifref_t *make_ifref(type_t *iface);
static var_t *make_var(char *name);
static func_t *make_func(var_t *def, var_t *args);
static type_t *make_class(char *name);
static type_t *make_safearray(void);

static type_t *reg_type(type_t *type, const char *name, int t);
static type_t *reg_types(type_t *type, var_t *names, int t);
static type_t *find_type(const char *name, int t);
static type_t *find_type2(char *name, int t);
static type_t *get_type(unsigned char type, char *name, int t);
static type_t *get_typev(unsigned char type, var_t *name, int t);
static int get_struct_type(var_t *fields);

static var_t *reg_const(var_t *var);
static var_t *find_const(char *name, int f);

#define tsENUM   1
#define tsSTRUCT 2
#define tsUNION  3

static type_t std_bool = { "boolean" };
static type_t std_int = { "int" };
static type_t std_int64 = { "__int64" };
static type_t std_uhyper = { "MIDL_uhyper" };

%}
%union {
	attr_t *attr;
	expr_t *expr;
	type_t *type;
	typeref_t *tref;
	var_t *var;
	func_t *func;
	ifref_t *ifref;
	char *str;
	UUID *uuid;
	unsigned int num;
}

%token <str> aIDENTIFIER
%token <str> aKNOWNTYPE
%token <num> aNUM aHEXNUM
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

%type <attr> m_attributes attributes attrib_list attribute
%type <expr> m_exprs /* exprs expr_list */ m_expr expr expr_list_const expr_const
%type <expr> array array_list
%type <type> inherit interface interfacehdr interfacedef interfacedec
%type <type> dispinterface dispinterfacehdr dispinterfacedef
%type <type> module modulehdr moduledef
%type <type> base_type int_std
%type <type> enumdef structdef typedef uniondef
%type <ifref> gbl_statements coclass_ints coclass_int
%type <tref> type
%type <var> m_args no_args args arg
%type <var> fields field s_field cases case enums enum_list enum constdef externdef
%type <var> m_ident t_ident ident p_ident pident pident_list
%type <var> dispint_props
%type <func> funcdef int_statements
%type <func> dispint_meths
%type <type> coclass coclasshdr coclassdef
%type <num> pointer_type version
%type <str> libraryhdr

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

%%

input:   gbl_statements                        { write_proxies($1); write_client($1); write_server($1); }
	;

gbl_statements:					{ $$ = NULL; }
	| gbl_statements interfacedec		{ $$ = $1; }
	| gbl_statements interfacedef		{ $$ = make_ifref($2); LINK($$, $1); }
	| gbl_statements coclass ';'		{ $$ = $1;
						  reg_type($2, $2->name, 0);
						  if (!parse_only && do_header) write_coclass_forward($2);
						}
	| gbl_statements coclassdef		{ $$ = $1;
						  add_coclass($2);
						  reg_type($2, $2->name, 0);
						}
	| gbl_statements moduledef		{ $$ = $1; add_module($2); }
	| gbl_statements librarydef		{ $$ = $1; }
	| gbl_statements statement		{ $$ = $1; }
	;

imp_statements:					{}
	| imp_statements interfacedec		{ if (!parse_only) add_interface($2); }
	| imp_statements interfacedef		{ if (!parse_only) add_interface($2); }
	| imp_statements coclass ';'		{ reg_type($2, $2->name, 0); if (!parse_only && do_header) write_coclass_forward($2); }
	| imp_statements coclassdef		{ if (!parse_only) add_coclass($2);
						  reg_type($2, $2->name, 0);
						}
	| imp_statements moduledef		{ if (!parse_only) add_module($2); }
	| imp_statements statement		{}
	| imp_statements importlib		{}
	;

int_statements:					{ $$ = NULL; }
	| int_statements funcdef ';'		{ $$ = $2; LINK($$, $1); }
	| int_statements statement		{ $$ = $1; }
	;

statement: ';'					{}
	| constdef ';'				{ if (!parse_only && do_header) { write_constdef($1); } }
	| cppquote				{}
	| enumdef ';'				{ if (!parse_only && do_header) { write_type(header, $1, NULL, NULL); fprintf(header, ";\n\n"); } }
	| externdef ';'				{ if (!parse_only && do_header) { write_externdef($1); } }
	| import				{}
	| structdef ';'				{ if (!parse_only && do_header) { write_type(header, $1, NULL, NULL); fprintf(header, ";\n\n"); } }
	| typedef ';'				{}
	| uniondef ';'				{ if (!parse_only && do_header) { write_type(header, $1, NULL, NULL); fprintf(header, ";\n\n"); } }
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
library_start: attributes libraryhdr '{'	{ start_typelib($2, $1);
						  if (!parse_only && do_header) write_library($2, $1); }
	;
librarydef: library_start imp_statements '}'	{ end_typelib(); }
	;

m_args:						{ $$ = NULL; }
	| args
	;

no_args:  tVOID					{ $$ = NULL; }
	;

args:	  arg
	| args ',' arg				{ LINK($3, $1); $$ = $3; }
	| no_args
	;

/* split into two rules to get bison to resolve a tVOID conflict */
arg:	  attributes type pident array		{ $$ = $3;
						  set_type($$, $2, $4);
						  $$->attrs = $1;
						}
	| type pident array			{ $$ = $2;
						  set_type($$, $1, $3);
						}
	| attributes type pident '(' m_args ')'	{ $$ = $3;
						  $$->ptr_level--;
						  set_type($$, $2, NULL);
						  $$->attrs = $1;
						  $$->args = $5;
						}
	| type pident '(' m_args ')'		{ $$ = $2;
						  $$->ptr_level--;
						  set_type($$, $1, NULL);
						  $$->args = $4;
						}
	;

array:						{ $$ = NULL; }
	| '[' array_list ']'			{ $$ = $2; }
	| '[' '*' ']'				{ $$ = make_expr(EXPR_VOID); }
	;

array_list: m_expr /* size of first dimension is optional */
	| array_list ',' expr			{ LINK($3, $1); $$ = $3; }
	| array_list ']' '[' expr		{ LINK($4, $1); $$ = $4; }
	;

m_attributes:					{ $$ = NULL; }
	| attributes
	;

attributes:
	  '[' attrib_list ']'			{ $$ = $2;
						  if (!$$)
						    yyerror("empty attribute lists unsupported");
						}
	;

attrib_list: attribute
	| attrib_list ',' attribute		{ if ($3) { LINK($3, $1); $$ = $3; }
						  else { $$ = $1; }
						}
	| attrib_list ']' '[' attribute		{ if ($4) { LINK($4, $1); $$ = $4; }
						  else { $$ = $1; }
						}
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
	| tENDPOINT '(' aSTRING ')'		{ $$ = make_attrp(ATTR_ENDPOINT, $3); }
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
	| tIIDIS '(' ident ')'			{ $$ = make_attrp(ATTR_IIDIS, $3); }
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
	| tRANGE '(' expr_const ',' expr_const ')' { LINK($5, $3); $$ = make_attrp(ATTR_RANGE, $5); }
	| tREADONLY				{ $$ = make_attr(ATTR_READONLY); }
	| tREQUESTEDIT				{ $$ = make_attr(ATTR_REQUESTEDIT); }
	| tRESTRICTED				{ $$ = make_attr(ATTR_RESTRICTED); }
	| tRETVAL				{ $$ = make_attr(ATTR_RETVAL); }
	| tSIZEIS '(' m_exprs ')'		{ $$ = make_attrp(ATTR_SIZEIS, $3); }
	| tSOURCE				{ $$ = make_attr(ATTR_SOURCE); }
	| tSTRING				{ $$ = make_attr(ATTR_STRING); }
	| tSWITCHIS '(' expr ')'		{ $$ = make_attrp(ATTR_SWITCHIS, $3); }
	| tSWITCHTYPE '(' type ')'		{ $$ = make_attrp(ATTR_SWITCHTYPE, type_ref($3)); }
	| tTRANSMITAS '(' type ')'		{ $$ = make_attrp(ATTR_TRANSMITAS, type_ref($3)); }
	| tUUID '(' aUUID ')'			{ $$ = make_attrp(ATTR_UUID, $3); }
	| tV1ENUM				{ $$ = make_attr(ATTR_V1ENUM); }
	| tVARARG				{ $$ = make_attr(ATTR_VARARG); }
	| tVERSION '(' version ')'		{ $$ = make_attrv(ATTR_VERSION, $3); }
	| tWIREMARSHAL '(' type ')'		{ $$ = make_attrp(ATTR_WIREMARSHAL, type_ref($3)); }
	| pointer_type				{ $$ = make_attrv(ATTR_POINTERTYPE, $1); }
	;

callconv:
	| tSTDCALL
	;

cases:						{ $$ = NULL; }
	| cases case				{ if ($2) { LINK($2, $1); $$ = $2; }
						  else { $$ = $1; }
						}
	;

case:	  tCASE expr ':' field			{ attr_t *a = make_attrp(ATTR_CASE, $2);
						  $$ = $4; if (!$$) $$ = make_var(NULL);
						  LINK(a, $$->attrs); $$->attrs = a;
						}
	| tDEFAULT ':' field			{ attr_t *a = make_attr(ATTR_DEFAULT);
						  $$ = $3; if (!$$) $$ = make_var(NULL);
						  LINK(a, $$->attrs); $$->attrs = a;
						}
	;

constdef: tCONST type ident '=' expr_const	{ $$ = reg_const($3);
						  set_type($$, $2, NULL);
						  $$->eval = $5;
						}
	;

enums:						{ $$ = NULL; }
	| enum_list ','				{ $$ = $1; }
	| enum_list
	;

enum_list: enum					{ if (!$$->eval)
						    $$->eval = make_exprl(EXPR_NUM, 0 /* default for first enum entry */);
						}
	| enum_list ',' enum			{ LINK($3, $1); $$ = $3;
						  if (!$$->eval)
						    $$->eval = make_exprl(EXPR_NUM, $1->eval->cval + 1);
						}
	;

enum:	  ident '=' expr_const			{ $$ = reg_const($1);
						  $$->eval = $3;
                                                  $$->type = make_type(RPC_FC_LONG, &std_int);
						}
	| ident					{ $$ = reg_const($1);
                                                  $$->type = make_type(RPC_FC_LONG, &std_int);
						}
	;

enumdef: tENUM t_ident '{' enums '}'		{ $$ = get_typev(RPC_FC_ENUM16, $2, tsENUM);
						  $$->fields = $4;
						  $$->defined = TRUE;
                                                  if(in_typelib)
                                                      add_enum($$);
						}
	;

m_exprs:  m_expr
	| m_exprs ',' m_expr			{ LINK($3, $1); $$ = $3; }
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
	| '*' expr %prec PPTR			{ $$ = make_expr1(EXPR_PPTR, $2); }
	| '(' type ')' expr %prec CAST		{ $$ = make_exprt(EXPR_CAST, $2, $4); }
	| tSIZEOF '(' type ')'			{ $$ = make_exprt(EXPR_SIZEOF, $3, NULL); }
	| '(' expr ')'				{ $$ = $2; }
	;

expr_list_const: expr_const
	| expr_list_const ',' expr_const	{ LINK($3, $1); $$ = $3; }
	;

expr_const: expr				{ $$ = $1;
						  if (!$$->is_const)
						      yyerror("expression is not constant");
						}
	;

externdef: tEXTERN tCONST type ident		{ $$ = $4;
						  set_type($$, $3, NULL);
						}
	;

fields:						{ $$ = NULL; }
	| fields field				{ if ($2) { LINK($2, $1); $$ = $2; }
						  else { $$ = $1; }
						}
	;

field:	  s_field ';'				{ $$ = $1; }
	| m_attributes uniondef ';'		{ $$ = make_var(NULL); $$->type = $2; $$->attrs = $1; }
	| attributes ';'			{ $$ = make_var(NULL); $$->attrs = $1; }
	| ';'					{ $$ = NULL; }
	;

s_field:  m_attributes type pident array	{ $$ = $3; set_type($$, $2, $4); $$->attrs = $1; }
	;

funcdef:
	  m_attributes type callconv pident
	  '(' m_args ')'			{ set_type($4, $2, NULL);
						  $4->attrs = $1;
						  $$ = make_func($4, $6);
						  if (is_attr($4->attrs, ATTR_IN)) {
						    yyerror("inapplicable attribute [in] for function '%s'",$$->def->name);
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
	| tASYNC				{ $$ = make_var($<str>1); }
	| tID					{ $$ = make_var($<str>1); }
	| tLCID					{ $$ = make_var($<str>1); }
	| tOBJECT				{ $$ = make_var($<str>1); }
	| tRANGE				{ $$ = make_var($<str>1); }
	| tRETVAL				{ $$ = make_var($<str>1); }
	| tUUID					{ $$ = make_var($<str>1); }
	| tVERSION				{ $$ = make_var($<str>1); }
	;

base_type: tBYTE				{ $$ = make_type(RPC_FC_BYTE, NULL); }
	| tWCHAR				{ $$ = make_type(RPC_FC_WCHAR, NULL); }
	| int_std
	| tSIGNED int_std			{ $$ = $2; $$->sign = 1; }
	| tUNSIGNED int_std			{ $$ = $2; $$->sign = -1;
						  switch ($$->type) {
						  case RPC_FC_CHAR:  break;
						  case RPC_FC_SMALL: $$->type = RPC_FC_USMALL; break;
						  case RPC_FC_SHORT: $$->type = RPC_FC_USHORT; break;
						  case RPC_FC_LONG:  $$->type = RPC_FC_ULONG;  break;
						  case RPC_FC_HYPER:
						    if (!$$->ref) { $$->ref = &std_uhyper; $$->sign = 0; }
						    break;
						  default: break;
						  }
						}
	| tUNSIGNED				{ $$ = make_type(RPC_FC_ULONG, &std_int); $$->sign = -1; }
	| tFLOAT				{ $$ = make_type(RPC_FC_FLOAT, NULL); }
	| tSINGLE				{ $$ = make_type(RPC_FC_FLOAT, NULL); }
	| tDOUBLE				{ $$ = make_type(RPC_FC_DOUBLE, NULL); }
	| tBOOLEAN				{ $$ = make_type(RPC_FC_BYTE, &std_bool); /* ? */ }
	| tERRORSTATUST				{ $$ = make_type(RPC_FC_ERROR_STATUS_T, NULL); }
	| tHANDLET				{ $$ = make_type(RPC_FC_BIND_PRIMITIVE, NULL); /* ? */ }
	;

m_int:
	| tINT
	;

int_std:  tINT					{ $$ = make_type(RPC_FC_LONG, &std_int); } /* win32 only */
	| tSHORT m_int				{ $$ = make_type(RPC_FC_SHORT, NULL); }
	| tSMALL				{ $$ = make_type(RPC_FC_SMALL, NULL); }
	| tLONG m_int				{ $$ = make_type(RPC_FC_LONG, NULL); }
	| tHYPER m_int				{ $$ = make_type(RPC_FC_HYPER, NULL); }
	| tINT64				{ $$ = make_type(RPC_FC_HYPER, &std_int64); }
	| tCHAR					{ $$ = make_type(RPC_FC_CHAR, NULL); }
	;

coclass:  tCOCLASS aIDENTIFIER			{ $$ = make_class($2); }
	| tCOCLASS aKNOWNTYPE			{ $$ = find_type($2, 0);
						  if ($$->defined) yyerror("multiple definition error");
						  if ($$->kind != TKIND_COCLASS) yyerror("%s was not declared a coclass", $2);
						}
	;

coclasshdr: attributes coclass			{ $$ = $2;
						  $$->attrs = $1;
						  if (!parse_only && do_header)
						    write_coclass($$);
						}
	;

coclassdef: coclasshdr '{' coclass_ints '}'	{ $$ = $1;
						  $$->ifaces = $3;
						}
	;

coclass_ints:					{ $$ = NULL; }
	| coclass_ints coclass_int		{ LINK($2, $1); $$ = $2; }
	;

coclass_int:
	  m_attributes interfacedec		{ $$ = make_ifref($2); $$->attrs = $1; }
	;

dispinterface: tDISPINTERFACE aIDENTIFIER	{ $$ = get_type(0, $2, 0); }
	|      tDISPINTERFACE aKNOWNTYPE	{ $$ = get_type(0, $2, 0); }
	;

dispinterfacehdr: attributes dispinterface	{ attr_t *attrs;
						  $$ = $2;
						  if ($$->defined) yyerror("multiple definition error");
						  attrs = make_attr(ATTR_DISPINTERFACE);
						  LINK(attrs, $1);
						  $$->attrs = attrs;
						  $$->ref = find_type("IDispatch", 0);
						  if (!$$->ref) yyerror("IDispatch is undefined");
						  $$->defined = TRUE;
						  if (!parse_only && do_header) write_forward($$);
						}
	;

dispint_props: tPROPERTIES ':'			{ $$ = NULL; }
	| dispint_props s_field ';'		{ LINK($2, $1); $$ = $2; }
	;

dispint_meths: tMETHODS ':'			{ $$ = NULL; }
	| dispint_meths funcdef ';'		{ LINK($2, $1); $$ = $2; }
	;

dispinterfacedef: dispinterfacehdr '{'
	  dispint_props
	  dispint_meths
	  '}'					{ $$ = $1;
						  $$->fields = $3;
						  $$->funcs = $4;
						  if (!parse_only && do_header) write_dispinterface($$);
						}
/* FIXME: not sure how to handle this yet
	| dispinterfacehdr '{' interface '}'	{ $$ = $1;
						  if (!parse_only && do_header) write_interface($$);
						}
*/
	;

inherit:					{ $$ = NULL; }
	| ':' aKNOWNTYPE			{ $$ = find_type2($2, 0); }
	;

interface: tINTERFACE aIDENTIFIER		{ $$ = get_type(RPC_FC_IP, $2, 0); }
	|  tINTERFACE aKNOWNTYPE		{ $$ = get_type(RPC_FC_IP, $2, 0); }
	;

interfacehdr: attributes interface		{ $$ = $2;
						  if ($$->defined) yyerror("multiple definition error");
						  $$->attrs = $1;
						  $$->defined = TRUE;
						  if (!parse_only && do_header) write_forward($$);
						}
	;

interfacedef: interfacehdr inherit
	  '{' int_statements '}'		{ $$ = $1;
						  $$->ref = $2;
						  $$->funcs = $4;
						  if (!parse_only && do_header) write_interface($$);
						}
/* MIDL is able to import the definition of a base class from inside the
 * definition of a derived class, I'll try to support it with this rule */
	| interfacehdr ':' aIDENTIFIER
	  '{' import int_statements '}'		{ $$ = $1;
						  $$->ref = find_type2($3, 0);
						  if (!$$->ref) yyerror("base class '%s' not found in import", $3);
						  $$->funcs = $6;
						  if (!parse_only && do_header) write_interface($$);
						}
	| dispinterfacedef			{ $$ = $1; }
	;

interfacedec:
	  interface ';'				{ $$ = $1; if (!parse_only && do_header) write_forward($$); }
	| dispinterface ';'			{ $$ = $1; if (!parse_only && do_header) write_forward($$); }
	;

module:   tMODULE aIDENTIFIER			{ $$ = make_type(0, NULL); $$->name = $2; }
	| tMODULE aKNOWNTYPE			{ $$ = make_type(0, NULL); $$->name = $2; }
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

pident:	  ident
	| p_ident
	| '(' pident ')'			{ $$ = $2; }
	;

pident_list:
	  pident
	| pident_list ',' pident		{ LINK($3, $1); $$ = $3; }
	;

pointer_type:
	  tREF					{ $$ = RPC_FC_RP; }
	| tUNIQUE				{ $$ = RPC_FC_UP; }
	| tPTR					{ $$ = RPC_FC_FP; }
	;

structdef: tSTRUCT t_ident '{' fields '}'	{ $$ = get_typev(RPC_FC_STRUCT, $2, tsSTRUCT);
                                                  /* overwrite RPC_FC_STRUCT with a more exact type */
						  $$->type = get_struct_type( $4 );
						  $$->fields = $4;
						  $$->defined = TRUE;
                                                  if(in_typelib)
                                                      add_struct($$);
                                                }
	;

type:	  tVOID					{ $$ = make_tref(NULL, make_type(0, NULL)); }
	| aKNOWNTYPE				{ $$ = make_tref($1, find_type($1, 0)); }
	| base_type				{ $$ = make_tref(NULL, $1); }
	| tCONST type				{ $$ = uniq_tref($2); $$->ref->is_const = TRUE; }
	| enumdef				{ $$ = make_tref(NULL, $1); }
	| tENUM aIDENTIFIER			{ $$ = make_tref(NULL, find_type2($2, tsENUM)); }
	| structdef				{ $$ = make_tref(NULL, $1); }
	| tSTRUCT aIDENTIFIER			{ $$ = make_tref(NULL, get_type(RPC_FC_STRUCT, $2, tsSTRUCT)); }
	| uniondef				{ $$ = make_tref(NULL, $1); }
	| tUNION aIDENTIFIER			{ $$ = make_tref(NULL, find_type2($2, tsUNION)); }
	| tSAFEARRAY '(' type ')'		{ $$ = make_tref(NULL, make_safearray()); }
	;

typedef: tTYPEDEF m_attributes type pident_list	{ typeref_t *tref = uniq_tref($3);
						  $4->tname = tref->name;
						  tref->name = NULL;
						  $$ = type_ref(tref);
						  $$->attrs = $2;
						  if (!parse_only && do_header)
						    write_typedef($$, $4);
						  if (in_typelib && $$->attrs)
						    add_typedef($$, $4);
						  reg_types($$, $4, 0);
						}
	;

uniondef: tUNION t_ident '{' fields '}'		{ $$ = get_typev(RPC_FC_NON_ENCAPSULATED_UNION, $2, tsUNION);
						  $$->fields = $4;
						  $$->defined = TRUE;
						}
	| tUNION t_ident
	  tSWITCH '(' s_field ')'
	  m_ident '{' cases '}'			{ var_t *u = $7;
						  $$ = get_typev(RPC_FC_ENCAPSULATED_UNION, $2, tsUNION);
						  if (!u) u = make_var("tagged_union");
						  u->type = make_type(RPC_FC_NON_ENCAPSULATED_UNION, NULL);
						  u->type->fields = $9;
						  u->type->defined = TRUE;
						  LINK(u, $5); $$->fields = u;
						  $$->defined = TRUE;
						}
	;

version:
	  aNUM					{ $$ = MAKELONG($1, 0); }
	| aNUM '.' aNUM				{ $$ = MAKELONG($1, $3); }
	;

%%

static attr_t *make_attr(enum attr_type type)
{
  attr_t *a = xmalloc(sizeof(attr_t));
  a->type = type;
  a->u.ival = 0;
  INIT_LINK(a);
  return a;
}

static attr_t *make_attrv(enum attr_type type, unsigned long val)
{
  attr_t *a = xmalloc(sizeof(attr_t));
  a->type = type;
  a->u.ival = val;
  INIT_LINK(a);
  return a;
}

static attr_t *make_attrp(enum attr_type type, void *val)
{
  attr_t *a = xmalloc(sizeof(attr_t));
  a->type = type;
  a->u.pval = val;
  INIT_LINK(a);
  return a;
}

static expr_t *make_expr(enum expr_type type)
{
  expr_t *e = xmalloc(sizeof(expr_t));
  e->type = type;
  e->ref = NULL;
  e->u.lval = 0;
  e->is_const = FALSE;
  INIT_LINK(e);
  return e;
}

static expr_t *make_exprl(enum expr_type type, long val)
{
  expr_t *e = xmalloc(sizeof(expr_t));
  e->type = type;
  e->ref = NULL;
  e->u.lval = val;
  e->is_const = FALSE;
  INIT_LINK(e);
  /* check for numeric constant */
  if (type == EXPR_NUM || type == EXPR_HEXNUM || type == EXPR_TRUEFALSE) {
    /* make sure true/false value is valid */
    assert(type != EXPR_TRUEFALSE || val == 0 || val == 1);
    e->is_const = TRUE;
    e->cval = val;
  }
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
  INIT_LINK(e);
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

static expr_t *make_exprt(enum expr_type type, typeref_t *tref, expr_t *expr)
{
  expr_t *e;
  e = xmalloc(sizeof(expr_t));
  e->type = type;
  e->ref = expr;
  e->u.tref = tref;
  e->is_const = FALSE;
  INIT_LINK(e);
  /* check for cast of constant expression */
  if (type == EXPR_SIZEOF) {
    switch (tref->ref->type) {
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
  e = xmalloc(sizeof(expr_t));
  e->type = type;
  e->ref = expr;
  e->u.lval = 0;
  e->is_const = FALSE;
  INIT_LINK(e);
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
  INIT_LINK(e);
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
  INIT_LINK(e);
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

static type_t *make_type(unsigned char type, type_t *ref)
{
  type_t *t = xmalloc(sizeof(type_t));
  t->name = NULL;
  t->kind = TKIND_PRIMITIVE;
  t->type = type;
  t->ref = ref;
  t->attrs = NULL;
  t->funcs = NULL;
  t->fields = NULL;
  t->ifaces = NULL;
  t->ignore = parse_only;
  t->is_const = FALSE;
  t->sign = 0;
  t->defined = FALSE;
  t->written = FALSE;
  t->typelib_idx = -1;
  INIT_LINK(t);
  return t;
}

static typeref_t *make_tref(char *name, type_t *ref)
{
  typeref_t *t = xmalloc(sizeof(typeref_t));
  t->name = name;
  t->ref = ref;
  t->uniq = ref ? 0 : 1;
  return t;
}

static typeref_t *uniq_tref(typeref_t *ref)
{
  typeref_t *t = ref;
  type_t *tp;
  if (t->uniq) return t;
  tp = make_type(0, t->ref);
  tp->name = t->name;
  t->name = NULL;
  t->ref = tp;
  t->uniq = 1;
  return t;
}

static type_t *type_ref(typeref_t *ref)
{
  type_t *t = ref->ref;
  if (ref->name) free(ref->name);
  free(ref);
  return t;
}

static void set_type(var_t *v, typeref_t *ref, expr_t *arr)
{
  v->type = ref->ref;
  v->tname = ref->name;
  ref->name = NULL;
  free(ref);
  v->array = arr;
}

static ifref_t *make_ifref(type_t *iface)
{
  ifref_t *l = xmalloc(sizeof(ifref_t));
  l->iface = iface;
  l->attrs = NULL;
  INIT_LINK(l);
  return l;
}

static var_t *make_var(char *name)
{
  var_t *v = xmalloc(sizeof(var_t));
  v->name = name;
  v->ptr_level = 0;
  v->type = NULL;
  v->tname = NULL;
  v->attrs = NULL;
  v->array = NULL;
  v->eval = NULL;
  INIT_LINK(v);
  return v;
}

static func_t *make_func(var_t *def, var_t *args)
{
  func_t *f = xmalloc(sizeof(func_t));
  f->def = def;
  f->args = args;
  f->ignore = parse_only;
  f->idx = -1;
  INIT_LINK(f);
  return f;
}

static type_t *make_class(char *name)
{
  type_t *c = make_type(0, NULL);
  c->name = name;
  c->kind = TKIND_COCLASS;
  INIT_LINK(c);
  return c;
}

static type_t *make_safearray(void)
{
  return make_type(RPC_FC_FP, find_type("SAFEARRAY", 0));
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
    yyerror("registering named type without name");
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

/* determine pointer type from attrs */
static unsigned char get_pointer_type( type_t *type )
{
  int t;
  if (is_attr( type->attrs, ATTR_STRING ))
  {
    type_t *t = type;
    while( t->type == 0 && t->ref )
      t = t->ref;
    switch( t->type )
    {
    case RPC_FC_CHAR:
      return RPC_FC_C_CSTRING;
    case RPC_FC_WCHAR:
      return RPC_FC_C_WSTRING;
    }
  }
  t = get_attrv( type->attrs, ATTR_POINTERTYPE );
  if (t) return t;
  return RPC_FC_FP;
}

static type_t *reg_types(type_t *type, var_t *names, int t)
{
  type_t *ptr = type;
  int ptrc = 0;

  while (names) {
    var_t *next = NEXT_LINK(names);
    if (names->name) {
      type_t *cur = ptr;
      int cptr = names->ptr_level;
      if (cptr > ptrc) {
        while (cptr > ptrc) {
          int t = get_pointer_type( cur );
          cur = ptr = make_type(t, cur);
          ptrc++;
        }
      } else {
        while (cptr < ptrc) {
          cur = cur->ref;
          cptr++;
        }
      }
      reg_type(cur, names->name, t);
    }
    free(names);
    names = next;
  }
  return type;
}

static type_t *find_type(const char *name, int t)
{
  struct rtype *cur = type_hash[hash_ident(name)];
  while (cur && (cur->t != t || strcmp(cur->name, name)))
    cur = cur->next;
  if (!cur) {
    yyerror("type '%s' not found", name);
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

static int get_struct_type(var_t *field)
{
  int has_pointer = 0;
  int has_conformance = 0;
  int has_variance = 0;

  for (; field; field = NEXT_LINK(field))
  {
    type_t *t = field->type;

    /* get the base type */
    while( (t->type == 0) && t->ref )
      t = t->ref;

    if (field->ptr_level > 0)
    {
        has_pointer = 1;
        continue;
    }

    if (is_string_type(field->attrs, 0, field->array))
    {
        has_conformance = 1;
        has_variance = 1;
        continue;
    }

    if (is_array_type(field->attrs, 0, field->array))
    {
        if (field->array && !field->array->is_const)
        {
            has_conformance = 1;
            if (PREV_LINK(field))
                yyerror("field '%s' deriving from a conformant array must be the last field in the structure",
                        field->name);
        }
        if (is_attr(field->attrs, ATTR_LENGTHIS))
            has_variance = 1;
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
    case RPC_FC_ENUM16:
    case RPC_FC_ENUM32:
      break;

    case RPC_FC_UP:
    case RPC_FC_FP:
      has_pointer = 1;
      break;
    case RPC_FC_CARRAY:
      has_conformance = 1;
      if (PREV_LINK(field))
          yyerror("field '%s' deriving from a conformant array must be the last field in the structure",
                  field->name);
      break;
    case RPC_FC_C_CSTRING:
    case RPC_FC_C_WSTRING:
      has_conformance = 1;
      has_variance = 1;
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
      if (PREV_LINK(field))
          yyerror("field '%s' deriving from a conformant array must be the last field in the structure",
                  field->name);
      has_pointer = 1;
      break;

    case RPC_FC_CSTRUCT:
      has_conformance = 1;
      if (PREV_LINK(field))
          yyerror("field '%s' deriving from a conformant array must be the last field in the structure",
                  field->name);
      break;

    case RPC_FC_PSTRUCT:
      has_pointer = 1;
      break;

    default:
      fprintf(stderr,"Unknown struct member %s with type (0x%02x)\n",
              field->name, t->type);
      /* fallthru - treat it as complex */

    /* as soon as we see one of these these members, it's bogus... */
    case RPC_FC_IP:
    case RPC_FC_ENCAPSULATED_UNION:
    case RPC_FC_NON_ENCAPSULATED_UNION:
    case RPC_FC_TRANSMIT_AS:
    case RPC_FC_REPRESENT_AS:
    case RPC_FC_PAD:
    case RPC_FC_EMBEDDED_COMPLEX:
    case RPC_FC_BOGUS_STRUCT:
      return RPC_FC_BOGUS_STRUCT;
    }
  }

  if( has_variance )
    return RPC_FC_CVSTRUCT;
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
    yyerror("registering constant without name");
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
    if (f) yyerror("constant '%s' not found", name);
    return NULL;
  }
  return cur->var;
}
