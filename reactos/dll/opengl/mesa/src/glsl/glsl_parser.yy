%{
/*
 * Copyright © 2008, 2009 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
    
#include "ast.h"
#include "glsl_parser_extras.h"
#include "glsl_types.h"

#define YYLEX_PARAM state->scanner

#undef yyerror

static void yyerror(YYLTYPE *loc, _mesa_glsl_parse_state *st, const char *msg)
{
   _mesa_glsl_error(loc, st, "%s", msg);
}
%}

%pure-parser
%error-verbose

%locations
%initial-action {
   @$.first_line = 1;
   @$.first_column = 1;
   @$.last_line = 1;
   @$.last_column = 1;
   @$.source = 0;
}

%lex-param   {void *scanner}
%parse-param {struct _mesa_glsl_parse_state *state}

%union {
   int n;
   float real;
   char *identifier;

   struct ast_type_qualifier type_qualifier;

   ast_node *node;
   ast_type_specifier *type_specifier;
   ast_fully_specified_type *fully_specified_type;
   ast_function *function;
   ast_parameter_declarator *parameter_declarator;
   ast_function_definition *function_definition;
   ast_compound_statement *compound_statement;
   ast_expression *expression;
   ast_declarator_list *declarator_list;
   ast_struct_specifier *struct_specifier;
   ast_declaration *declaration;
   ast_switch_body *switch_body;
   ast_case_label *case_label;
   ast_case_label_list *case_label_list;
   ast_case_statement *case_statement;
   ast_case_statement_list *case_statement_list;

   struct {
      ast_node *cond;
      ast_expression *rest;
   } for_rest_statement;

   struct {
      ast_node *then_statement;
      ast_node *else_statement;
   } selection_rest_statement;
}

%token ATTRIBUTE CONST_TOK BOOL_TOK FLOAT_TOK INT_TOK UINT_TOK
%token BREAK CONTINUE DO ELSE FOR IF DISCARD RETURN SWITCH CASE DEFAULT
%token BVEC2 BVEC3 BVEC4 IVEC2 IVEC3 IVEC4 UVEC2 UVEC3 UVEC4 VEC2 VEC3 VEC4
%token CENTROID IN_TOK OUT_TOK INOUT_TOK UNIFORM VARYING
%token NOPERSPECTIVE FLAT SMOOTH
%token MAT2X2 MAT2X3 MAT2X4
%token MAT3X2 MAT3X3 MAT3X4
%token MAT4X2 MAT4X3 MAT4X4
%token SAMPLER1D SAMPLER2D SAMPLER3D SAMPLERCUBE SAMPLER1DSHADOW SAMPLER2DSHADOW
%token SAMPLERCUBESHADOW SAMPLER1DARRAY SAMPLER2DARRAY SAMPLER1DARRAYSHADOW
%token SAMPLER2DARRAYSHADOW ISAMPLER1D ISAMPLER2D ISAMPLER3D ISAMPLERCUBE
%token ISAMPLER1DARRAY ISAMPLER2DARRAY USAMPLER1D USAMPLER2D USAMPLER3D
%token USAMPLERCUBE USAMPLER1DARRAY USAMPLER2DARRAY
%token SAMPLEREXTERNALOES
%token STRUCT VOID_TOK WHILE
%token <identifier> IDENTIFIER TYPE_IDENTIFIER NEW_IDENTIFIER
%type <identifier> any_identifier
%token <real> FLOATCONSTANT
%token <n> INTCONSTANT UINTCONSTANT BOOLCONSTANT
%token <identifier> FIELD_SELECTION
%token LEFT_OP RIGHT_OP
%token INC_OP DEC_OP LE_OP GE_OP EQ_OP NE_OP
%token AND_OP OR_OP XOR_OP MUL_ASSIGN DIV_ASSIGN ADD_ASSIGN
%token MOD_ASSIGN LEFT_ASSIGN RIGHT_ASSIGN AND_ASSIGN XOR_ASSIGN OR_ASSIGN
%token SUB_ASSIGN
%token INVARIANT
%token LOWP MEDIUMP HIGHP SUPERP PRECISION

%token VERSION_TOK EXTENSION LINE COLON EOL INTERFACE OUTPUT
%token PRAGMA_DEBUG_ON PRAGMA_DEBUG_OFF
%token PRAGMA_OPTIMIZE_ON PRAGMA_OPTIMIZE_OFF
%token PRAGMA_INVARIANT_ALL
%token LAYOUT_TOK

   /* Reserved words that are not actually used in the grammar.
    */
%token ASM CLASS UNION ENUM TYPEDEF TEMPLATE THIS PACKED_TOK GOTO
%token INLINE_TOK NOINLINE VOLATILE PUBLIC_TOK STATIC EXTERN EXTERNAL
%token LONG_TOK SHORT_TOK DOUBLE_TOK HALF FIXED_TOK UNSIGNED INPUT_TOK OUPTUT
%token HVEC2 HVEC3 HVEC4 DVEC2 DVEC3 DVEC4 FVEC2 FVEC3 FVEC4
%token SAMPLER2DRECT SAMPLER3DRECT SAMPLER2DRECTSHADOW
%token SIZEOF CAST NAMESPACE USING

%token ERROR_TOK

%token COMMON PARTITION ACTIVE SAMPLERBUFFER FILTER
%token  IMAGE1D  IMAGE2D  IMAGE3D  IMAGECUBE  IMAGE1DARRAY  IMAGE2DARRAY
%token IIMAGE1D IIMAGE2D IIMAGE3D IIMAGECUBE IIMAGE1DARRAY IIMAGE2DARRAY
%token UIMAGE1D UIMAGE2D UIMAGE3D UIMAGECUBE UIMAGE1DARRAY UIMAGE2DARRAY
%token IMAGE1DSHADOW IMAGE2DSHADOW IMAGEBUFFER IIMAGEBUFFER UIMAGEBUFFER
%token IMAGE1DARRAYSHADOW IMAGE2DARRAYSHADOW
%token ROW_MAJOR

%type <identifier> variable_identifier
%type <node> statement
%type <node> statement_list
%type <node> simple_statement
%type <n> precision_qualifier
%type <type_qualifier> type_qualifier
%type <type_qualifier> storage_qualifier
%type <type_qualifier> interpolation_qualifier
%type <type_qualifier> layout_qualifier
%type <type_qualifier> layout_qualifier_id_list layout_qualifier_id
%type <type_specifier> type_specifier
%type <type_specifier> type_specifier_no_prec
%type <type_specifier> type_specifier_nonarray
%type <n> basic_type_specifier_nonarray
%type <fully_specified_type> fully_specified_type
%type <function> function_prototype
%type <function> function_header
%type <function> function_header_with_parameters
%type <function> function_declarator
%type <parameter_declarator> parameter_declarator
%type <parameter_declarator> parameter_declaration
%type <type_qualifier> parameter_qualifier
%type <type_qualifier> parameter_type_qualifier
%type <type_specifier> parameter_type_specifier
%type <function_definition> function_definition
%type <compound_statement> compound_statement_no_new_scope
%type <compound_statement> compound_statement
%type <node> statement_no_new_scope
%type <node> expression_statement
%type <expression> expression
%type <expression> primary_expression
%type <expression> assignment_expression
%type <expression> conditional_expression
%type <expression> logical_or_expression
%type <expression> logical_xor_expression
%type <expression> logical_and_expression
%type <expression> inclusive_or_expression
%type <expression> exclusive_or_expression
%type <expression> and_expression
%type <expression> equality_expression
%type <expression> relational_expression
%type <expression> shift_expression
%type <expression> additive_expression
%type <expression> multiplicative_expression
%type <expression> unary_expression
%type <expression> constant_expression
%type <expression> integer_expression
%type <expression> postfix_expression
%type <expression> function_call_header_with_parameters
%type <expression> function_call_header_no_parameters
%type <expression> function_call_header
%type <expression> function_call_generic
%type <expression> function_call_or_method
%type <expression> function_call
%type <expression> method_call_generic
%type <expression> method_call_header_with_parameters
%type <expression> method_call_header_no_parameters
%type <expression> method_call_header
%type <n> assignment_operator
%type <n> unary_operator
%type <expression> function_identifier
%type <node> external_declaration
%type <declarator_list> init_declarator_list
%type <declarator_list> single_declaration
%type <expression> initializer
%type <node> declaration
%type <node> declaration_statement
%type <node> jump_statement
%type <struct_specifier> struct_specifier
%type <node> struct_declaration_list
%type <declarator_list> struct_declaration
%type <declaration> struct_declarator
%type <declaration> struct_declarator_list
%type <node> selection_statement
%type <selection_rest_statement> selection_rest_statement
%type <node> switch_statement
%type <switch_body> switch_body
%type <case_label_list> case_label_list
%type <case_label> case_label
%type <case_statement> case_statement
%type <case_statement_list> case_statement_list
%type <node> iteration_statement
%type <node> condition
%type <node> conditionopt
%type <node> for_init_statement
%type <for_rest_statement> for_rest_statement
%%

translation_unit: 
	version_statement extension_statement_list
	{
	   _mesa_glsl_initialize_types(state);
	}
	external_declaration_list
	{
	   delete state->symbols;
	   state->symbols = new(ralloc_parent(state)) glsl_symbol_table;
	   _mesa_glsl_initialize_types(state);
	}
	;

version_statement:
	/* blank - no #version specified: defaults are already set */
	| VERSION_TOK INTCONSTANT EOL
	{
	   bool supported = false;

	   switch ($2) {
	   case 100:
	      state->es_shader = true;
	      supported = state->Const.GLSL_100ES;
	      break;
	   case 110:
	      supported = state->Const.GLSL_110;
	      break;
	   case 120:
	      supported = state->Const.GLSL_120;
	      break;
	   case 130:
	      supported = state->Const.GLSL_130;
	      break;
	   default:
	      supported = false;
	      break;
	   }

	   state->language_version = $2;
	   state->version_string =
	      ralloc_asprintf(state, "GLSL%s %d.%02d",
			      state->es_shader ? " ES" : "",
			      state->language_version / 100,
			      state->language_version % 100);

	   if (!supported) {
	      _mesa_glsl_error(& @2, state, "%s is not supported. "
			       "Supported versions are: %s\n",
			       state->version_string,
			       state->supported_version_string);
	   }
	}
	;

pragma_statement:
	PRAGMA_DEBUG_ON EOL
	| PRAGMA_DEBUG_OFF EOL
	| PRAGMA_OPTIMIZE_ON EOL
	| PRAGMA_OPTIMIZE_OFF EOL
	| PRAGMA_INVARIANT_ALL EOL
	{
	   if (state->language_version < 120) {
	      _mesa_glsl_warning(& @1, state,
				 "pragma `invariant(all)' not supported in %s",
				 state->version_string);
	   } else {
	      state->all_invariant = true;
	   }
	}
	;

extension_statement_list:

	| extension_statement_list extension_statement
	;

any_identifier:
	IDENTIFIER
	| TYPE_IDENTIFIER
	| NEW_IDENTIFIER
	;

extension_statement:
	EXTENSION any_identifier COLON any_identifier EOL
	{
	   if (!_mesa_glsl_process_extension($2, & @2, $4, & @4, state)) {
	      YYERROR;
	   }
	}
	;

external_declaration_list:
	external_declaration
	{
	   /* FINISHME: The NULL test is required because pragmas are set to
	    * FINISHME: NULL. (See production rule for external_declaration.)
	    */
	   if ($1 != NULL)
	      state->translation_unit.push_tail(& $1->link);
	}
	| external_declaration_list external_declaration
	{
	   /* FINISHME: The NULL test is required because pragmas are set to
	    * FINISHME: NULL. (See production rule for external_declaration.)
	    */
	   if ($2 != NULL)
	      state->translation_unit.push_tail(& $2->link);
	}
	;

variable_identifier:
	IDENTIFIER
	| NEW_IDENTIFIER
	;

primary_expression:
	variable_identifier
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_expression(ast_identifier, NULL, NULL, NULL);
	   $$->set_location(yylloc);
	   $$->primary_expression.identifier = $1;
	}
	| INTCONSTANT
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_expression(ast_int_constant, NULL, NULL, NULL);
	   $$->set_location(yylloc);
	   $$->primary_expression.int_constant = $1;
	}
	| UINTCONSTANT
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_expression(ast_uint_constant, NULL, NULL, NULL);
	   $$->set_location(yylloc);
	   $$->primary_expression.uint_constant = $1;
	}
	| FLOATCONSTANT
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_expression(ast_float_constant, NULL, NULL, NULL);
	   $$->set_location(yylloc);
	   $$->primary_expression.float_constant = $1;
	}
	| BOOLCONSTANT
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_expression(ast_bool_constant, NULL, NULL, NULL);
	   $$->set_location(yylloc);
	   $$->primary_expression.bool_constant = $1;
	}
	| '(' expression ')'
	{
	   $$ = $2;
	}
	;

postfix_expression:
	primary_expression
	| postfix_expression '[' integer_expression ']'
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_expression(ast_array_index, $1, $3, NULL);
	   $$->set_location(yylloc);
	}
	| function_call
	{
	   $$ = $1;
	}
	| postfix_expression '.' any_identifier
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_expression(ast_field_selection, $1, NULL, NULL);
	   $$->set_location(yylloc);
	   $$->primary_expression.identifier = $3;
	}
	| postfix_expression INC_OP
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_expression(ast_post_inc, $1, NULL, NULL);
	   $$->set_location(yylloc);
	}
	| postfix_expression DEC_OP
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_expression(ast_post_dec, $1, NULL, NULL);
	   $$->set_location(yylloc);
	}
	;

integer_expression:
	expression
	;

function_call:
	function_call_or_method
	;

function_call_or_method:
	function_call_generic
	| postfix_expression '.' method_call_generic
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_expression(ast_field_selection, $1, $3, NULL);
	   $$->set_location(yylloc);
	}
	;

function_call_generic:
	function_call_header_with_parameters ')'
	| function_call_header_no_parameters ')'
	;

function_call_header_no_parameters:
	function_call_header VOID_TOK
	| function_call_header
	;

function_call_header_with_parameters:
	function_call_header assignment_expression
	{
	   $$ = $1;
	   $$->set_location(yylloc);
	   $$->expressions.push_tail(& $2->link);
	}
	| function_call_header_with_parameters ',' assignment_expression
	{
	   $$ = $1;
	   $$->set_location(yylloc);
	   $$->expressions.push_tail(& $3->link);
	}
	;

	// Grammar Note: Constructors look like functions, but lexical 
	// analysis recognized most of them as keywords. They are now
	// recognized through "type_specifier".
function_call_header:
	function_identifier '('
	;

function_identifier:
	type_specifier
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_function_expression($1);
	   $$->set_location(yylloc);
   	}
	| variable_identifier
	{
	   void *ctx = state;
	   ast_expression *callee = new(ctx) ast_expression($1);
	   $$ = new(ctx) ast_function_expression(callee);
	   $$->set_location(yylloc);
   	}
	| FIELD_SELECTION
	{
	   void *ctx = state;
	   ast_expression *callee = new(ctx) ast_expression($1);
	   $$ = new(ctx) ast_function_expression(callee);
	   $$->set_location(yylloc);
   	}
	;

method_call_generic:
	method_call_header_with_parameters ')'
	| method_call_header_no_parameters ')'
	;

method_call_header_no_parameters:
	method_call_header VOID_TOK
	| method_call_header
	;

method_call_header_with_parameters:
	method_call_header assignment_expression
	{
	   $$ = $1;
	   $$->set_location(yylloc);
	   $$->expressions.push_tail(& $2->link);
	}
	| method_call_header_with_parameters ',' assignment_expression
	{
	   $$ = $1;
	   $$->set_location(yylloc);
	   $$->expressions.push_tail(& $3->link);
	}
	;

	// Grammar Note: Constructors look like methods, but lexical 
	// analysis recognized most of them as keywords. They are now
	// recognized through "type_specifier".
method_call_header:
	variable_identifier '('
	{
	   void *ctx = state;
	   ast_expression *callee = new(ctx) ast_expression($1);
	   $$ = new(ctx) ast_function_expression(callee);
	   $$->set_location(yylloc);
   	}
	;

	// Grammar Note: No traditional style type casts.
unary_expression:
	postfix_expression
	| INC_OP unary_expression
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_expression(ast_pre_inc, $2, NULL, NULL);
	   $$->set_location(yylloc);
	}
	| DEC_OP unary_expression
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_expression(ast_pre_dec, $2, NULL, NULL);
	   $$->set_location(yylloc);
	}
	| unary_operator unary_expression
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_expression($1, $2, NULL, NULL);
	   $$->set_location(yylloc);
	}
	;

	// Grammar Note: No '*' or '&' unary ops. Pointers are not supported.
unary_operator:
	'+'	{ $$ = ast_plus; }
	| '-'	{ $$ = ast_neg; }
	| '!'	{ $$ = ast_logic_not; }
	| '~'	{ $$ = ast_bit_not; }
	;

multiplicative_expression:
	unary_expression
	| multiplicative_expression '*' unary_expression
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_expression_bin(ast_mul, $1, $3);
	   $$->set_location(yylloc);
	}
	| multiplicative_expression '/' unary_expression
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_expression_bin(ast_div, $1, $3);
	   $$->set_location(yylloc);
	}
	| multiplicative_expression '%' unary_expression
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_expression_bin(ast_mod, $1, $3);
	   $$->set_location(yylloc);
	}
	;

additive_expression:
	multiplicative_expression
	| additive_expression '+' multiplicative_expression
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_expression_bin(ast_add, $1, $3);
	   $$->set_location(yylloc);
	}
	| additive_expression '-' multiplicative_expression
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_expression_bin(ast_sub, $1, $3);
	   $$->set_location(yylloc);
	}
	;

shift_expression:
	additive_expression
	| shift_expression LEFT_OP additive_expression
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_expression_bin(ast_lshift, $1, $3);
	   $$->set_location(yylloc);
	}
	| shift_expression RIGHT_OP additive_expression
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_expression_bin(ast_rshift, $1, $3);
	   $$->set_location(yylloc);
	}
	;

relational_expression:
	shift_expression
	| relational_expression '<' shift_expression
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_expression_bin(ast_less, $1, $3);
	   $$->set_location(yylloc);
	}
	| relational_expression '>' shift_expression
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_expression_bin(ast_greater, $1, $3);
	   $$->set_location(yylloc);
	}
	| relational_expression LE_OP shift_expression
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_expression_bin(ast_lequal, $1, $3);
	   $$->set_location(yylloc);
	}
	| relational_expression GE_OP shift_expression
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_expression_bin(ast_gequal, $1, $3);
	   $$->set_location(yylloc);
	}
	;

equality_expression:
	relational_expression
	| equality_expression EQ_OP relational_expression
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_expression_bin(ast_equal, $1, $3);
	   $$->set_location(yylloc);
	}
	| equality_expression NE_OP relational_expression
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_expression_bin(ast_nequal, $1, $3);
	   $$->set_location(yylloc);
	}
	;

and_expression:
	equality_expression
	| and_expression '&' equality_expression
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_expression_bin(ast_bit_and, $1, $3);
	   $$->set_location(yylloc);
	}
	;

exclusive_or_expression:
	and_expression
	| exclusive_or_expression '^' and_expression
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_expression_bin(ast_bit_xor, $1, $3);
	   $$->set_location(yylloc);
	}
	;

inclusive_or_expression:
	exclusive_or_expression
	| inclusive_or_expression '|' exclusive_or_expression
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_expression_bin(ast_bit_or, $1, $3);
	   $$->set_location(yylloc);
	}
	;

logical_and_expression:
	inclusive_or_expression
	| logical_and_expression AND_OP inclusive_or_expression
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_expression_bin(ast_logic_and, $1, $3);
	   $$->set_location(yylloc);
	}
	;

logical_xor_expression:
	logical_and_expression
	| logical_xor_expression XOR_OP logical_and_expression
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_expression_bin(ast_logic_xor, $1, $3);
	   $$->set_location(yylloc);
	}
	;

logical_or_expression:
	logical_xor_expression
	| logical_or_expression OR_OP logical_xor_expression
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_expression_bin(ast_logic_or, $1, $3);
	   $$->set_location(yylloc);
	}
	;

conditional_expression:
	logical_or_expression
	| logical_or_expression '?' expression ':' assignment_expression
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_expression(ast_conditional, $1, $3, $5);
	   $$->set_location(yylloc);
	}
	;

assignment_expression:
	conditional_expression
	| unary_expression assignment_operator assignment_expression
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_expression($2, $1, $3, NULL);
	   $$->set_location(yylloc);
	}
	;

assignment_operator:
	'='		{ $$ = ast_assign; }
	| MUL_ASSIGN	{ $$ = ast_mul_assign; }
	| DIV_ASSIGN	{ $$ = ast_div_assign; }
	| MOD_ASSIGN	{ $$ = ast_mod_assign; }
	| ADD_ASSIGN	{ $$ = ast_add_assign; }
	| SUB_ASSIGN	{ $$ = ast_sub_assign; }
	| LEFT_ASSIGN	{ $$ = ast_ls_assign; }
	| RIGHT_ASSIGN	{ $$ = ast_rs_assign; }
	| AND_ASSIGN	{ $$ = ast_and_assign; }
	| XOR_ASSIGN	{ $$ = ast_xor_assign; }
	| OR_ASSIGN	{ $$ = ast_or_assign; }
	;

expression:
	assignment_expression
	{
	   $$ = $1;
	}
	| expression ',' assignment_expression
	{
	   void *ctx = state;
	   if ($1->oper != ast_sequence) {
	      $$ = new(ctx) ast_expression(ast_sequence, NULL, NULL, NULL);
	      $$->set_location(yylloc);
	      $$->expressions.push_tail(& $1->link);
	   } else {
	      $$ = $1;
	   }

	   $$->expressions.push_tail(& $3->link);
	}
	;

constant_expression:
	conditional_expression
	;

declaration:
	function_prototype ';'
	{
	   state->symbols->pop_scope();
	   $$ = $1;
	}
	| init_declarator_list ';'
	{
	   $$ = $1;
	}
	| PRECISION precision_qualifier type_specifier_no_prec ';'
	{
	   $3->precision = $2;
	   $3->is_precision_statement = true;
	   $$ = $3;
	}
	;

function_prototype:
	function_declarator ')'
	;

function_declarator:
	function_header
	| function_header_with_parameters
	;

function_header_with_parameters:
	function_header parameter_declaration
	{
	   $$ = $1;
	   $$->parameters.push_tail(& $2->link);
	}
	| function_header_with_parameters ',' parameter_declaration
	{
	   $$ = $1;
	   $$->parameters.push_tail(& $3->link);
	}
	;

function_header:
	fully_specified_type variable_identifier '('
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_function();
	   $$->set_location(yylloc);
	   $$->return_type = $1;
	   $$->identifier = $2;

	   state->symbols->add_function(new(state) ir_function($2));
	   state->symbols->push_scope();
	}
	;

parameter_declarator:
	type_specifier any_identifier
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_parameter_declarator();
	   $$->set_location(yylloc);
	   $$->type = new(ctx) ast_fully_specified_type();
	   $$->type->set_location(yylloc);
	   $$->type->specifier = $1;
	   $$->identifier = $2;
	}
	| type_specifier any_identifier '[' constant_expression ']'
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_parameter_declarator();
	   $$->set_location(yylloc);
	   $$->type = new(ctx) ast_fully_specified_type();
	   $$->type->set_location(yylloc);
	   $$->type->specifier = $1;
	   $$->identifier = $2;
	   $$->is_array = true;
	   $$->array_size = $4;
	}
	;

parameter_declaration:
	parameter_type_qualifier parameter_qualifier parameter_declarator
	{
	   $1.flags.i |= $2.flags.i;

	   $$ = $3;
	   $$->type->qualifier = $1;
	}
	| parameter_qualifier parameter_declarator
	{
	   $$ = $2;
	   $$->type->qualifier = $1;
	}
	| parameter_type_qualifier parameter_qualifier parameter_type_specifier
	{
	   void *ctx = state;
	   $1.flags.i |= $2.flags.i;

	   $$ = new(ctx) ast_parameter_declarator();
	   $$->set_location(yylloc);
	   $$->type = new(ctx) ast_fully_specified_type();
	   $$->type->qualifier = $1;
	   $$->type->specifier = $3;
	}
	| parameter_qualifier parameter_type_specifier
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_parameter_declarator();
	   $$->set_location(yylloc);
	   $$->type = new(ctx) ast_fully_specified_type();
	   $$->type->qualifier = $1;
	   $$->type->specifier = $2;
	}
	;

parameter_qualifier:
	/* empty */
	{
	   memset(& $$, 0, sizeof($$));
	}
	| IN_TOK
	{
	   memset(& $$, 0, sizeof($$));
	   $$.flags.q.in = 1;
	}
	| OUT_TOK
	{
	   memset(& $$, 0, sizeof($$));
	   $$.flags.q.out = 1;
	}
	| INOUT_TOK
	{
	   memset(& $$, 0, sizeof($$));
	   $$.flags.q.in = 1;
	   $$.flags.q.out = 1;
	}
	;

parameter_type_specifier:
	type_specifier
	;

init_declarator_list:
	single_declaration
	| init_declarator_list ',' any_identifier
	{
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration($3, false, NULL, NULL);
	   decl->set_location(yylloc);

	   $$ = $1;
	   $$->declarations.push_tail(&decl->link);
	   state->symbols->add_variable(new(state) ir_variable(NULL, $3, ir_var_auto));
	}
	| init_declarator_list ',' any_identifier '[' ']'
	{
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration($3, true, NULL, NULL);
	   decl->set_location(yylloc);

	   $$ = $1;
	   $$->declarations.push_tail(&decl->link);
	   state->symbols->add_variable(new(state) ir_variable(NULL, $3, ir_var_auto));
	}
	| init_declarator_list ',' any_identifier '[' constant_expression ']'
	{
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration($3, true, $5, NULL);
	   decl->set_location(yylloc);

	   $$ = $1;
	   $$->declarations.push_tail(&decl->link);
	   state->symbols->add_variable(new(state) ir_variable(NULL, $3, ir_var_auto));
	}
	| init_declarator_list ',' any_identifier '[' ']' '=' initializer
	{
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration($3, true, NULL, $7);
	   decl->set_location(yylloc);

	   $$ = $1;
	   $$->declarations.push_tail(&decl->link);
	   state->symbols->add_variable(new(state) ir_variable(NULL, $3, ir_var_auto));
	}
	| init_declarator_list ',' any_identifier '[' constant_expression ']' '=' initializer
	{
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration($3, true, $5, $8);
	   decl->set_location(yylloc);

	   $$ = $1;
	   $$->declarations.push_tail(&decl->link);
	   state->symbols->add_variable(new(state) ir_variable(NULL, $3, ir_var_auto));
	}
	| init_declarator_list ',' any_identifier '=' initializer
	{
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration($3, false, NULL, $5);
	   decl->set_location(yylloc);

	   $$ = $1;
	   $$->declarations.push_tail(&decl->link);
	   state->symbols->add_variable(new(state) ir_variable(NULL, $3, ir_var_auto));
	}
	;

	// Grammar Note: No 'enum', or 'typedef'.
single_declaration:
	fully_specified_type
	{
	   void *ctx = state;
	   /* Empty declaration list is valid. */
	   $$ = new(ctx) ast_declarator_list($1);
	   $$->set_location(yylloc);
	}
	| fully_specified_type any_identifier
	{
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration($2, false, NULL, NULL);

	   $$ = new(ctx) ast_declarator_list($1);
	   $$->set_location(yylloc);
	   $$->declarations.push_tail(&decl->link);
	}
	| fully_specified_type any_identifier '[' ']'
	{
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration($2, true, NULL, NULL);

	   $$ = new(ctx) ast_declarator_list($1);
	   $$->set_location(yylloc);
	   $$->declarations.push_tail(&decl->link);
	}
	| fully_specified_type any_identifier '[' constant_expression ']'
	{
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration($2, true, $4, NULL);

	   $$ = new(ctx) ast_declarator_list($1);
	   $$->set_location(yylloc);
	   $$->declarations.push_tail(&decl->link);
	}
	| fully_specified_type any_identifier '[' ']' '=' initializer
	{
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration($2, true, NULL, $6);

	   $$ = new(ctx) ast_declarator_list($1);
	   $$->set_location(yylloc);
	   $$->declarations.push_tail(&decl->link);
	}
	| fully_specified_type any_identifier '[' constant_expression ']' '=' initializer
	{
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration($2, true, $4, $7);

	   $$ = new(ctx) ast_declarator_list($1);
	   $$->set_location(yylloc);
	   $$->declarations.push_tail(&decl->link);
	}
	| fully_specified_type any_identifier '=' initializer
	{
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration($2, false, NULL, $4);

	   $$ = new(ctx) ast_declarator_list($1);
	   $$->set_location(yylloc);
	   $$->declarations.push_tail(&decl->link);
	}
	| INVARIANT variable_identifier // Vertex only.
	{
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration($2, false, NULL, NULL);

	   $$ = new(ctx) ast_declarator_list(NULL);
	   $$->set_location(yylloc);
	   $$->invariant = true;

	   $$->declarations.push_tail(&decl->link);
	}
	;

fully_specified_type:
	type_specifier
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_fully_specified_type();
	   $$->set_location(yylloc);
	   $$->specifier = $1;
	}
	| type_qualifier type_specifier
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_fully_specified_type();
	   $$->set_location(yylloc);
	   $$->qualifier = $1;
	   $$->specifier = $2;
	}
	;

layout_qualifier:
	LAYOUT_TOK '(' layout_qualifier_id_list ')'
	{
	  $$ = $3;
	}
	;

layout_qualifier_id_list:
	layout_qualifier_id
	| layout_qualifier_id_list ',' layout_qualifier_id
	{
	   if (($1.flags.i & $3.flags.i) != 0) {
	      _mesa_glsl_error(& @3, state,
			       "duplicate layout qualifiers used\n");
	      YYERROR;
	   }

	   $$.flags.i = $1.flags.i | $3.flags.i;

	   if ($1.flags.q.explicit_location)
	      $$.location = $1.location;

	   if ($3.flags.q.explicit_location)
	      $$.location = $3.location;
	}
	;

layout_qualifier_id:
	any_identifier
	{
	   bool got_one = false;

	   memset(& $$, 0, sizeof($$));

	   /* Layout qualifiers for ARB_fragment_coord_conventions. */
	   if (!got_one && state->ARB_fragment_coord_conventions_enable) {
	      if (strcmp($1, "origin_upper_left") == 0) {
		 got_one = true;
		 $$.flags.q.origin_upper_left = 1;
	      } else if (strcmp($1, "pixel_center_integer") == 0) {
		 got_one = true;
		 $$.flags.q.pixel_center_integer = 1;
	      }

	      if (got_one && state->ARB_fragment_coord_conventions_warn) {
		 _mesa_glsl_warning(& @1, state,
				    "GL_ARB_fragment_coord_conventions layout "
				    "identifier `%s' used\n", $1);
	      }
	   }

	   /* Layout qualifiers for AMD/ARB_conservative_depth. */
	   if (!got_one &&
	       (state->AMD_conservative_depth_enable ||
	        state->ARB_conservative_depth_enable)) {
	      if (strcmp($1, "depth_any") == 0) {
	         got_one = true;
	         $$.flags.q.depth_any = 1;
	      } else if (strcmp($1, "depth_greater") == 0) {
	         got_one = true;
	         $$.flags.q.depth_greater = 1;
	      } else if (strcmp($1, "depth_less") == 0) {
	         got_one = true;
	         $$.flags.q.depth_less = 1;
	      } else if (strcmp($1, "depth_unchanged") == 0) {
	         got_one = true;
	         $$.flags.q.depth_unchanged = 1;
	      }
	
	      if (got_one && state->AMD_conservative_depth_warn) {
	         _mesa_glsl_warning(& @1, state,
	                            "GL_AMD_conservative_depth "
	                            "layout qualifier `%s' is used\n", $1);
	      }
	      if (got_one && state->ARB_conservative_depth_warn) {
	         _mesa_glsl_warning(& @1, state,
	                            "GL_ARB_conservative_depth "
	                            "layout qualifier `%s' is used\n", $1);
	      }
	   }

	   if (!got_one) {
	      _mesa_glsl_error(& @1, state, "unrecognized layout identifier "
			       "`%s'\n", $1);
	      YYERROR;
	   }
	}
	| any_identifier '=' INTCONSTANT
	{
	   bool got_one = false;

	   memset(& $$, 0, sizeof($$));

	   if (state->ARB_explicit_attrib_location_enable) {
	      /* FINISHME: Handle 'index' once GL_ARB_blend_func_exteneded and
	       * FINISHME: GLSL 1.30 (or later) are supported.
	       */
	      if (strcmp("location", $1) == 0) {
		 got_one = true;

		 $$.flags.q.explicit_location = 1;

		 if ($3 >= 0) {
		    $$.location = $3;
		 } else {
		    _mesa_glsl_error(& @3, state,
				     "invalid location %d specified\n", $3);
		    YYERROR;
		 }
	      }
	   }

	   /* If the identifier didn't match any known layout identifiers,
	    * emit an error.
	    */
	   if (!got_one) {
	      _mesa_glsl_error(& @1, state, "unrecognized layout identifier "
			       "`%s'\n", $1);
	      YYERROR;
	   } else if (state->ARB_explicit_attrib_location_warn) {
	      _mesa_glsl_warning(& @1, state,
				 "GL_ARB_explicit_attrib_location layout "
				 "identifier `%s' used\n", $1);
	   }
	}
	;

interpolation_qualifier:
	SMOOTH
	{
	   memset(& $$, 0, sizeof($$));
	   $$.flags.q.smooth = 1;
	}
	| FLAT
	{
	   memset(& $$, 0, sizeof($$));
	   $$.flags.q.flat = 1;
	}
	| NOPERSPECTIVE
	{
	   memset(& $$, 0, sizeof($$));
	   $$.flags.q.noperspective = 1;
	}
	;

parameter_type_qualifier:
	CONST_TOK
	{
	   memset(& $$, 0, sizeof($$));
	   $$.flags.q.constant = 1;
	}
	;

type_qualifier:
	storage_qualifier
	| layout_qualifier
	| layout_qualifier storage_qualifier
	{
	   $$ = $1;
	   $$.flags.i |= $2.flags.i;
	}
	| interpolation_qualifier
	| interpolation_qualifier storage_qualifier
	{
	   $$ = $1;
	   $$.flags.i |= $2.flags.i;
	}
	| INVARIANT storage_qualifier
	{
	   $$ = $2;
	   $$.flags.q.invariant = 1;
	}
	| INVARIANT interpolation_qualifier storage_qualifier
	{
	   $$ = $2;
	   $$.flags.i |= $3.flags.i;
	   $$.flags.q.invariant = 1;
	}
	| INVARIANT
	{
	   memset(& $$, 0, sizeof($$));
	   $$.flags.q.invariant = 1;
	}
	;

storage_qualifier:
	CONST_TOK
	{
	   memset(& $$, 0, sizeof($$));
	   $$.flags.q.constant = 1;
	}
	| ATTRIBUTE
	{
	   memset(& $$, 0, sizeof($$));
	   $$.flags.q.attribute = 1;
	}
	| VARYING
	{
	   memset(& $$, 0, sizeof($$));
	   $$.flags.q.varying = 1;
	}
	| CENTROID VARYING
	{
	   memset(& $$, 0, sizeof($$));
	   $$.flags.q.centroid = 1;
	   $$.flags.q.varying = 1;
	}
	| IN_TOK
	{
	   memset(& $$, 0, sizeof($$));
	   $$.flags.q.in = 1;
	}
	| OUT_TOK
	{
	   memset(& $$, 0, sizeof($$));
	   $$.flags.q.out = 1;
	}
	| CENTROID IN_TOK
	{
	   memset(& $$, 0, sizeof($$));
	   $$.flags.q.centroid = 1; $$.flags.q.in = 1;
	}
	| CENTROID OUT_TOK
	{
	   memset(& $$, 0, sizeof($$));
	   $$.flags.q.centroid = 1; $$.flags.q.out = 1;
	}
	| UNIFORM
	{
	   memset(& $$, 0, sizeof($$));
	   $$.flags.q.uniform = 1;
	}
	;

type_specifier:
	type_specifier_no_prec
	{
	   $$ = $1;
	}
	| precision_qualifier type_specifier_no_prec
	{
	   $$ = $2;
	   $$->precision = $1;
	}
	;

type_specifier_no_prec:
	type_specifier_nonarray
	| type_specifier_nonarray '[' ']'
	{
	   $$ = $1;
	   $$->is_array = true;
	   $$->array_size = NULL;
	}
	| type_specifier_nonarray '[' constant_expression ']'
	{
	   $$ = $1;
	   $$->is_array = true;
	   $$->array_size = $3;
	}
	;

type_specifier_nonarray:
	basic_type_specifier_nonarray
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_type_specifier($1);
	   $$->set_location(yylloc);
	}
	| struct_specifier
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_type_specifier($1);
	   $$->set_location(yylloc);
	}
	| TYPE_IDENTIFIER
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_type_specifier($1);
	   $$->set_location(yylloc);
	}
	;

basic_type_specifier_nonarray:
	VOID_TOK		{ $$ = ast_void; }
	| FLOAT_TOK		{ $$ = ast_float; }
	| INT_TOK		{ $$ = ast_int; }
	| UINT_TOK		{ $$ = ast_uint; }
	| BOOL_TOK		{ $$ = ast_bool; }
	| VEC2			{ $$ = ast_vec2; }
	| VEC3			{ $$ = ast_vec3; }
	| VEC4			{ $$ = ast_vec4; }
	| BVEC2			{ $$ = ast_bvec2; }
	| BVEC3			{ $$ = ast_bvec3; }
	| BVEC4			{ $$ = ast_bvec4; }
	| IVEC2			{ $$ = ast_ivec2; }
	| IVEC3			{ $$ = ast_ivec3; }
	| IVEC4			{ $$ = ast_ivec4; }
	| UVEC2			{ $$ = ast_uvec2; }
	| UVEC3			{ $$ = ast_uvec3; }
	| UVEC4			{ $$ = ast_uvec4; }
	| MAT2X2		{ $$ = ast_mat2; }
	| MAT2X3		{ $$ = ast_mat2x3; }
	| MAT2X4		{ $$ = ast_mat2x4; }
	| MAT3X2		{ $$ = ast_mat3x2; }
	| MAT3X3		{ $$ = ast_mat3; }
	| MAT3X4		{ $$ = ast_mat3x4; }
	| MAT4X2		{ $$ = ast_mat4x2; }
	| MAT4X3		{ $$ = ast_mat4x3; }
	| MAT4X4		{ $$ = ast_mat4; }
	| SAMPLER1D		{ $$ = ast_sampler1d; }
	| SAMPLER2D		{ $$ = ast_sampler2d; }
	| SAMPLER2DRECT		{ $$ = ast_sampler2drect; }
	| SAMPLER3D		{ $$ = ast_sampler3d; }
	| SAMPLERCUBE		{ $$ = ast_samplercube; }
	| SAMPLEREXTERNALOES	{ $$ = ast_samplerexternaloes; }
	| SAMPLER1DSHADOW	{ $$ = ast_sampler1dshadow; }
	| SAMPLER2DSHADOW	{ $$ = ast_sampler2dshadow; }
	| SAMPLER2DRECTSHADOW	{ $$ = ast_sampler2drectshadow; }
	| SAMPLERCUBESHADOW	{ $$ = ast_samplercubeshadow; }
	| SAMPLER1DARRAY	{ $$ = ast_sampler1darray; }
	| SAMPLER2DARRAY	{ $$ = ast_sampler2darray; }
	| SAMPLER1DARRAYSHADOW	{ $$ = ast_sampler1darrayshadow; }
	| SAMPLER2DARRAYSHADOW	{ $$ = ast_sampler2darrayshadow; }
	| ISAMPLER1D		{ $$ = ast_isampler1d; }
	| ISAMPLER2D		{ $$ = ast_isampler2d; }
	| ISAMPLER3D		{ $$ = ast_isampler3d; }
	| ISAMPLERCUBE		{ $$ = ast_isamplercube; }
	| ISAMPLER1DARRAY	{ $$ = ast_isampler1darray; }
	| ISAMPLER2DARRAY	{ $$ = ast_isampler2darray; }
	| USAMPLER1D		{ $$ = ast_usampler1d; }
	| USAMPLER2D		{ $$ = ast_usampler2d; }
	| USAMPLER3D		{ $$ = ast_usampler3d; }
	| USAMPLERCUBE		{ $$ = ast_usamplercube; }
	| USAMPLER1DARRAY	{ $$ = ast_usampler1darray; }
	| USAMPLER2DARRAY	{ $$ = ast_usampler2darray; }
	;

precision_qualifier:
	HIGHP	  {
		     if (!state->es_shader && state->language_version < 130)
			_mesa_glsl_error(& @1, state,
				         "precision qualifier forbidden "
					 "in %s (1.30 or later "
					 "required)\n",
					 state->version_string);

		     $$ = ast_precision_high;
		  }
	| MEDIUMP {
		     if (!state->es_shader && state->language_version < 130)
			_mesa_glsl_error(& @1, state,
					 "precision qualifier forbidden "
					 "in %s (1.30 or later "
					 "required)\n",
					 state->version_string);

		     $$ = ast_precision_medium;
		  }
	| LOWP	  {
		     if (!state->es_shader && state->language_version < 130)
			_mesa_glsl_error(& @1, state,
					 "precision qualifier forbidden "
					 "in %s (1.30 or later "
					 "required)\n",
					 state->version_string);

		     $$ = ast_precision_low;
		  }
	;

struct_specifier:
	STRUCT any_identifier '{' struct_declaration_list '}'
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_struct_specifier($2, $4);
	   $$->set_location(yylloc);
	   state->symbols->add_type($2, glsl_type::void_type);
	}
	| STRUCT '{' struct_declaration_list '}'
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_struct_specifier(NULL, $3);
	   $$->set_location(yylloc);
	}
	;

struct_declaration_list:
	struct_declaration
	{
	   $$ = (ast_node *) $1;
	   $1->link.self_link();
	}
	| struct_declaration_list struct_declaration
	{
	   $$ = (ast_node *) $1;
	   $$->link.insert_before(& $2->link);
	}
	;

struct_declaration:
	type_specifier struct_declarator_list ';'
	{
	   void *ctx = state;
	   ast_fully_specified_type *type = new(ctx) ast_fully_specified_type();
	   type->set_location(yylloc);

	   type->specifier = $1;
	   $$ = new(ctx) ast_declarator_list(type);
	   $$->set_location(yylloc);

	   $$->declarations.push_degenerate_list_at_head(& $2->link);
	}
	;

struct_declarator_list:
	struct_declarator
	{
	   $$ = $1;
	   $1->link.self_link();
	}
	| struct_declarator_list ',' struct_declarator
	{
	   $$ = $1;
	   $$->link.insert_before(& $3->link);
	}
	;

struct_declarator:
	any_identifier
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_declaration($1, false, NULL, NULL);
	   $$->set_location(yylloc);
	   state->symbols->add_variable(new(state) ir_variable(NULL, $1, ir_var_auto));
	}
	| any_identifier '[' constant_expression ']'
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_declaration($1, true, $3, NULL);
	   $$->set_location(yylloc);
	}
	;

initializer:
	assignment_expression
	;

declaration_statement:
	declaration
	;

	// Grammar Note: labeled statements for SWITCH only; 'goto' is not
	// supported.
statement:
	compound_statement	{ $$ = (ast_node *) $1; }
	| simple_statement
	;

simple_statement:
	declaration_statement
	| expression_statement
	| selection_statement
	| switch_statement
	| iteration_statement
	| jump_statement
	;

compound_statement:
	'{' '}'
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_compound_statement(true, NULL);
	   $$->set_location(yylloc);
	}
	| '{'
	{
	   state->symbols->push_scope();
	}
	statement_list '}'
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_compound_statement(true, $3);
	   $$->set_location(yylloc);
	   state->symbols->pop_scope();
	}
	;

statement_no_new_scope:
	compound_statement_no_new_scope { $$ = (ast_node *) $1; }
	| simple_statement
	;

compound_statement_no_new_scope:
	'{' '}'
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_compound_statement(false, NULL);
	   $$->set_location(yylloc);
	}
	| '{' statement_list '}'
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_compound_statement(false, $2);
	   $$->set_location(yylloc);
	}
	;

statement_list:
	statement
	{
	   if ($1 == NULL) {
	      _mesa_glsl_error(& @1, state, "<nil> statement\n");
	      assert($1 != NULL);
	   }

	   $$ = $1;
	   $$->link.self_link();
	}
	| statement_list statement
	{
	   if ($2 == NULL) {
	      _mesa_glsl_error(& @2, state, "<nil> statement\n");
	      assert($2 != NULL);
	   }
	   $$ = $1;
	   $$->link.insert_before(& $2->link);
	}
	;

expression_statement:
	';'
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_expression_statement(NULL);
	   $$->set_location(yylloc);
	}
	| expression ';'
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_expression_statement($1);
	   $$->set_location(yylloc);
	}
	;

selection_statement:
	IF '(' expression ')' selection_rest_statement
	{
	   $$ = new(state) ast_selection_statement($3, $5.then_statement,
						   $5.else_statement);
	   $$->set_location(yylloc);
	}
	;

selection_rest_statement:
	statement ELSE statement
	{
	   $$.then_statement = $1;
	   $$.else_statement = $3;
	}
	| statement
	{
	   $$.then_statement = $1;
	   $$.else_statement = NULL;
	}
	;

condition:
	expression
	{
	   $$ = (ast_node *) $1;
	}
	| fully_specified_type any_identifier '=' initializer
	{
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration($2, false, NULL, $4);
	   ast_declarator_list *declarator = new(ctx) ast_declarator_list($1);
	   decl->set_location(yylloc);
	   declarator->set_location(yylloc);

	   declarator->declarations.push_tail(&decl->link);
	   $$ = declarator;
	}
	;

/*
 * siwtch_statement grammar is based on the syntax described in the body
 * of the GLSL spec, not in it's appendix!!!
 */
switch_statement:
	SWITCH '(' expression ')' switch_body
	{
	   $$ = new(state) ast_switch_statement($3, $5);
	   $$->set_location(yylloc);
	}
	;

switch_body:
	'{' '}'
	{
	   $$ = new(state) ast_switch_body(NULL);
	   $$->set_location(yylloc);
	}
	| '{' case_statement_list '}'
	{
	   $$ = new(state) ast_switch_body($2);
	   $$->set_location(yylloc);
	}
	;

case_label:
	CASE expression ':'
	{
	   $$ = new(state) ast_case_label($2);
	   $$->set_location(yylloc);
	}
	| DEFAULT ':'
	{
	   $$ = new(state) ast_case_label(NULL);
	   $$->set_location(yylloc);
	}
	;

case_label_list:
	case_label
	{
	   ast_case_label_list *labels = new(state) ast_case_label_list();

	   labels->labels.push_tail(& $1->link);
	   $$ = labels;
	   $$->set_location(yylloc);
	}
	| case_label_list case_label
	{
	   $$ = $1;
	   $$->labels.push_tail(& $2->link);
	}
	;

case_statement:
	case_label_list statement
	{
	   ast_case_statement *stmts = new(state) ast_case_statement($1);
	   stmts->set_location(yylloc);

	   stmts->stmts.push_tail(& $2->link);
	   $$ = stmts;
	}
	| case_statement statement
	{
	   $$ = $1;
	   $$->stmts.push_tail(& $2->link);
	}
	;

case_statement_list:
	case_statement
	{
	   ast_case_statement_list *cases= new(state) ast_case_statement_list();
	   cases->set_location(yylloc);

	   cases->cases.push_tail(& $1->link);
	   $$ = cases;
	}
	| case_statement_list case_statement
	{
	   $$ = $1;
	   $$->cases.push_tail(& $2->link);
	}
	;

iteration_statement:
	WHILE '(' condition ')' statement_no_new_scope
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_iteration_statement(ast_iteration_statement::ast_while,
	   					    NULL, $3, NULL, $5);
	   $$->set_location(yylloc);
	}
	| DO statement WHILE '(' expression ')' ';'
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_iteration_statement(ast_iteration_statement::ast_do_while,
						    NULL, $5, NULL, $2);
	   $$->set_location(yylloc);
	}
	| FOR '(' for_init_statement for_rest_statement ')' statement_no_new_scope
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_iteration_statement(ast_iteration_statement::ast_for,
						    $3, $4.cond, $4.rest, $6);
	   $$->set_location(yylloc);
	}
	;

for_init_statement:
	expression_statement
	| declaration_statement
	;

conditionopt:
	condition
	| /* empty */
	{
	   $$ = NULL;
	}
	;

for_rest_statement:
	conditionopt ';'
	{
	   $$.cond = $1;
	   $$.rest = NULL;
	}
	| conditionopt ';' expression
	{
	   $$.cond = $1;
	   $$.rest = $3;
	}
	;

	// Grammar Note: No 'goto'. Gotos are not supported.
jump_statement:
	CONTINUE ';' 
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_jump_statement(ast_jump_statement::ast_continue, NULL);
	   $$->set_location(yylloc);
	}
	| BREAK ';'
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_jump_statement(ast_jump_statement::ast_break, NULL);
	   $$->set_location(yylloc);
	}
	| RETURN ';'
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_jump_statement(ast_jump_statement::ast_return, NULL);
	   $$->set_location(yylloc);
	}
	| RETURN expression ';'
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_jump_statement(ast_jump_statement::ast_return, $2);
	   $$->set_location(yylloc);
	}
	| DISCARD ';' // Fragment shader only.
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_jump_statement(ast_jump_statement::ast_discard, NULL);
	   $$->set_location(yylloc);
	}
	;

external_declaration:
	function_definition	{ $$ = $1; }
	| declaration		{ $$ = $1; }
	| pragma_statement	{ $$ = NULL; }
	;

function_definition:
	function_prototype compound_statement_no_new_scope
	{
	   void *ctx = state;
	   $$ = new(ctx) ast_function_definition();
	   $$->set_location(yylloc);
	   $$->prototype = $1;
	   $$->body = $2;

	   state->symbols->pop_scope();
	}
	;
