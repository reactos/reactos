%{
/*
 * Copyright © 2010 Intel Corporation
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
#include <inttypes.h>

#include "glcpp.h"
#include "main/core.h" /* for struct gl_extensions */
#include "main/mtypes.h" /* for gl_api enum */

static void
yyerror (YYLTYPE *locp, glcpp_parser_t *parser, const char *error);

static void
_define_object_macro (glcpp_parser_t *parser,
		      YYLTYPE *loc,
		      const char *macro,
		      token_list_t *replacements);

static void
_define_function_macro (glcpp_parser_t *parser,
			YYLTYPE *loc,
			const char *macro,
			string_list_t *parameters,
			token_list_t *replacements);

static string_list_t *
_string_list_create (void *ctx);

static void
_string_list_append_item (string_list_t *list, const char *str);

static int
_string_list_contains (string_list_t *list, const char *member, int *index);

static int
_string_list_length (string_list_t *list);

static int
_string_list_equal (string_list_t *a, string_list_t *b);

static argument_list_t *
_argument_list_create (void *ctx);

static void
_argument_list_append (argument_list_t *list, token_list_t *argument);

static int
_argument_list_length (argument_list_t *list);

static token_list_t *
_argument_list_member_at (argument_list_t *list, int index);

/* Note: This function ralloc_steal()s the str pointer. */
static token_t *
_token_create_str (void *ctx, int type, char *str);

static token_t *
_token_create_ival (void *ctx, int type, int ival);

static token_list_t *
_token_list_create (void *ctx);

static void
_token_list_append (token_list_t *list, token_t *token);

static void
_token_list_append_list (token_list_t *list, token_list_t *tail);

static int
_token_list_equal_ignoring_space (token_list_t *a, token_list_t *b);

static void
_parser_active_list_push (glcpp_parser_t *parser,
			  const char *identifier,
			  token_node_t *marker);

static void
_parser_active_list_pop (glcpp_parser_t *parser);

static int
_parser_active_list_contains (glcpp_parser_t *parser, const char *identifier);

static void
_glcpp_parser_expand_if (glcpp_parser_t *parser, int type, token_list_t *list);

static void
_glcpp_parser_expand_token_list (glcpp_parser_t *parser,
				 token_list_t *list);

static void
_glcpp_parser_print_expanded_token_list (glcpp_parser_t *parser,
					 token_list_t *list);

static void
_glcpp_parser_skip_stack_push_if (glcpp_parser_t *parser, YYLTYPE *loc,
				  int condition);

static void
_glcpp_parser_skip_stack_change_if (glcpp_parser_t *parser, YYLTYPE *loc,
				    const char *type, int condition);

static void
_glcpp_parser_skip_stack_pop (glcpp_parser_t *parser, YYLTYPE *loc);

#define yylex glcpp_parser_lex

static int
glcpp_parser_lex (YYSTYPE *yylval, YYLTYPE *yylloc, glcpp_parser_t *parser);

static void
glcpp_parser_lex_from (glcpp_parser_t *parser, token_list_t *list);

static void
add_builtin_define(glcpp_parser_t *parser, const char *name, int value);

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

%parse-param {glcpp_parser_t *parser}
%lex-param {glcpp_parser_t *parser}

%expect 0
%token COMMA_FINAL DEFINED ELIF_EXPANDED HASH HASH_DEFINE_FUNC HASH_DEFINE_OBJ HASH_ELIF HASH_ELSE HASH_ENDIF HASH_IF HASH_IFDEF HASH_IFNDEF HASH_UNDEF HASH_VERSION IDENTIFIER IF_EXPANDED INTEGER INTEGER_STRING NEWLINE OTHER PLACEHOLDER SPACE
%token PASTE
%type <ival> expression INTEGER operator SPACE integer_constant
%type <str> IDENTIFIER INTEGER_STRING OTHER
%type <string_list> identifier_list
%type <token> preprocessing_token conditional_token
%type <token_list> pp_tokens replacement_list text_line conditional_tokens
%left OR
%left AND
%left '|'
%left '^'
%left '&'
%left EQUAL NOT_EQUAL
%left '<' '>' LESS_OR_EQUAL GREATER_OR_EQUAL
%left LEFT_SHIFT RIGHT_SHIFT
%left '+' '-'
%left '*' '/' '%'
%right UNARY

%%

input:
	/* empty */
|	input line
;

line:
	control_line {
		ralloc_strcat (&parser->output, "\n");
	}
|	text_line {
		_glcpp_parser_print_expanded_token_list (parser, $1);
		ralloc_strcat (&parser->output, "\n");
		ralloc_free ($1);
	}
|	expanded_line
|	HASH non_directive
;

expanded_line:
	IF_EXPANDED expression NEWLINE {
		_glcpp_parser_skip_stack_push_if (parser, & @1, $2);
	}
|	ELIF_EXPANDED expression NEWLINE {
		_glcpp_parser_skip_stack_change_if (parser, & @1, "elif", $2);
	}
;

control_line:
	HASH_DEFINE_OBJ	IDENTIFIER replacement_list NEWLINE {
		_define_object_macro (parser, & @2, $2, $3);
	}
|	HASH_DEFINE_FUNC IDENTIFIER '(' ')' replacement_list NEWLINE {
		_define_function_macro (parser, & @2, $2, NULL, $5);
	}
|	HASH_DEFINE_FUNC IDENTIFIER '(' identifier_list ')' replacement_list NEWLINE {
		_define_function_macro (parser, & @2, $2, $4, $6);
	}
|	HASH_UNDEF IDENTIFIER NEWLINE {
		macro_t *macro = hash_table_find (parser->defines, $2);
		if (macro) {
			hash_table_remove (parser->defines, $2);
			ralloc_free (macro);
		}
		ralloc_free ($2);
	}
|	HASH_IF conditional_tokens NEWLINE {
		/* Be careful to only evaluate the 'if' expression if
		 * we are not skipping. When we are skipping, we
		 * simply push a new 0-valued 'if' onto the skip
		 * stack.
		 *
		 * This avoids generating diagnostics for invalid
		 * expressions that are being skipped. */
		if (parser->skip_stack == NULL ||
		    parser->skip_stack->type == SKIP_NO_SKIP)
		{
			_glcpp_parser_expand_if (parser, IF_EXPANDED, $2);
		}	
		else
		{
			_glcpp_parser_skip_stack_push_if (parser, & @1, 0);
			parser->skip_stack->type = SKIP_TO_ENDIF;
		}
	}
|	HASH_IF NEWLINE {
		/* #if without an expression is only an error if we
		 *  are not skipping */
		if (parser->skip_stack == NULL ||
		    parser->skip_stack->type == SKIP_NO_SKIP)
		{
			glcpp_error(& @1, parser, "#if with no expression");
		}	
		_glcpp_parser_skip_stack_push_if (parser, & @1, 0);
	}
|	HASH_IFDEF IDENTIFIER junk NEWLINE {
		macro_t *macro = hash_table_find (parser->defines, $2);
		ralloc_free ($2);
		_glcpp_parser_skip_stack_push_if (parser, & @1, macro != NULL);
	}
|	HASH_IFNDEF IDENTIFIER junk NEWLINE {
		macro_t *macro = hash_table_find (parser->defines, $2);
		ralloc_free ($2);
		_glcpp_parser_skip_stack_push_if (parser, & @1, macro == NULL);
	}
|	HASH_ELIF conditional_tokens NEWLINE {
		/* Be careful to only evaluate the 'elif' expression
		 * if we are not skipping. When we are skipping, we
		 * simply change to a 0-valued 'elif' on the skip
		 * stack.
		 *
		 * This avoids generating diagnostics for invalid
		 * expressions that are being skipped. */
		if (parser->skip_stack &&
		    parser->skip_stack->type == SKIP_TO_ELSE)
		{
			_glcpp_parser_expand_if (parser, ELIF_EXPANDED, $2);
		}
		else
		{
			_glcpp_parser_skip_stack_change_if (parser, & @1,
							    "elif", 0);
		}
	}
|	HASH_ELIF NEWLINE {
		/* #elif without an expression is an error unless we
		 * are skipping. */
		if (parser->skip_stack &&
		    parser->skip_stack->type == SKIP_TO_ELSE)
		{
			glcpp_error(& @1, parser, "#elif with no expression");
		}
		else
		{
			_glcpp_parser_skip_stack_change_if (parser, & @1,
							    "elif", 0);
			glcpp_warning(& @1, parser, "ignoring illegal #elif without expression");
		}
	}
|	HASH_ELSE NEWLINE {
		_glcpp_parser_skip_stack_change_if (parser, & @1, "else", 1);
	}
|	HASH_ENDIF NEWLINE {
		_glcpp_parser_skip_stack_pop (parser, & @1);
	}
|	HASH_VERSION integer_constant NEWLINE {
		macro_t *macro = hash_table_find (parser->defines, "__VERSION__");
		if (macro) {
			hash_table_remove (parser->defines, "__VERSION__");
			ralloc_free (macro);
		}
		add_builtin_define (parser, "__VERSION__", $2);

		if ($2 == 100)
			add_builtin_define (parser, "GL_ES", 1);

		/* Currently, all ES2 implementations support highp in the
		 * fragment shader, so we always define this macro in ES2.
		 * If we ever get a driver that doesn't support highp, we'll
		 * need to add a flag to the gl_context and check that here.
		 */
		if ($2 >= 130 || $2 == 100)
			add_builtin_define (parser, "GL_FRAGMENT_PRECISION_HIGH", 1);

		ralloc_asprintf_append (&parser->output, "#version %" PRIiMAX, $2);
	}
|	HASH NEWLINE
;

integer_constant:
	INTEGER_STRING {
		if (strlen ($1) >= 3 && strncmp ($1, "0x", 2) == 0) {
			$$ = strtoll ($1 + 2, NULL, 16);
		} else if ($1[0] == '0') {
			$$ = strtoll ($1, NULL, 8);
		} else {
			$$ = strtoll ($1, NULL, 10);
		}
	}
|	INTEGER {
		$$ = $1;
	}

expression:
	integer_constant
|	expression OR expression {
		$$ = $1 || $3;
	}
|	expression AND expression {
		$$ = $1 && $3;
	}
|	expression '|' expression {
		$$ = $1 | $3;
	}
|	expression '^' expression {
		$$ = $1 ^ $3;
	}
|	expression '&' expression {
		$$ = $1 & $3;
	}
|	expression NOT_EQUAL expression {
		$$ = $1 != $3;
	}
|	expression EQUAL expression {
		$$ = $1 == $3;
	}
|	expression GREATER_OR_EQUAL expression {
		$$ = $1 >= $3;
	}
|	expression LESS_OR_EQUAL expression {
		$$ = $1 <= $3;
	}
|	expression '>' expression {
		$$ = $1 > $3;
	}
|	expression '<' expression {
		$$ = $1 < $3;
	}
|	expression RIGHT_SHIFT expression {
		$$ = $1 >> $3;
	}
|	expression LEFT_SHIFT expression {
		$$ = $1 << $3;
	}
|	expression '-' expression {
		$$ = $1 - $3;
	}
|	expression '+' expression {
		$$ = $1 + $3;
	}
|	expression '%' expression {
		if ($3 == 0) {
			yyerror (& @1, parser,
				 "zero modulus in preprocessor directive");
		} else {
			$$ = $1 % $3;
		}
	}
|	expression '/' expression {
		if ($3 == 0) {
			yyerror (& @1, parser,
				 "division by 0 in preprocessor directive");
		} else {
			$$ = $1 / $3;
		}
	}
|	expression '*' expression {
		$$ = $1 * $3;
	}
|	'!' expression %prec UNARY {
		$$ = ! $2;
	}
|	'~' expression %prec UNARY {
		$$ = ~ $2;
	}
|	'-' expression %prec UNARY {
		$$ = - $2;
	}
|	'+' expression %prec UNARY {
		$$ = + $2;
	}
|	'(' expression ')' {
		$$ = $2;
	}
;

identifier_list:
	IDENTIFIER {
		$$ = _string_list_create (parser);
		_string_list_append_item ($$, $1);
		ralloc_steal ($$, $1);
	}
|	identifier_list ',' IDENTIFIER {
		$$ = $1;	
		_string_list_append_item ($$, $3);
		ralloc_steal ($$, $3);
	}
;

text_line:
	NEWLINE { $$ = NULL; }
|	pp_tokens NEWLINE
;

non_directive:
	pp_tokens NEWLINE {
		yyerror (& @1, parser, "Invalid tokens after #");
	}
;

replacement_list:
	/* empty */ { $$ = NULL; }
|	pp_tokens
;

junk:
	/* empty */
|	pp_tokens {
		glcpp_warning(&@1, parser, "extra tokens at end of directive");
	}
;

conditional_token:
	/* Handle "defined" operator */
	DEFINED IDENTIFIER {
		int v = hash_table_find (parser->defines, $2) ? 1 : 0;
		$$ = _token_create_ival (parser, INTEGER, v);
	}
|	DEFINED '(' IDENTIFIER ')' {
		int v = hash_table_find (parser->defines, $3) ? 1 : 0;
		$$ = _token_create_ival (parser, INTEGER, v);
	}
|	preprocessing_token
;

conditional_tokens:
	/* Exactly the same as pp_tokens, but using conditional_token */
	conditional_token {
		$$ = _token_list_create (parser);
		_token_list_append ($$, $1);
	}
|	conditional_tokens conditional_token {
		$$ = $1;
		_token_list_append ($$, $2);
	}
;

pp_tokens:
	preprocessing_token {
		parser->space_tokens = 1;
		$$ = _token_list_create (parser);
		_token_list_append ($$, $1);
	}
|	pp_tokens preprocessing_token {
		$$ = $1;
		_token_list_append ($$, $2);
	}
;

preprocessing_token:
	IDENTIFIER {
		$$ = _token_create_str (parser, IDENTIFIER, $1);
		$$->location = yylloc;
	}
|	INTEGER_STRING {
		$$ = _token_create_str (parser, INTEGER_STRING, $1);
		$$->location = yylloc;
	}
|	operator {
		$$ = _token_create_ival (parser, $1, $1);
		$$->location = yylloc;
	}
|	OTHER {
		$$ = _token_create_str (parser, OTHER, $1);
		$$->location = yylloc;
	}
|	SPACE {
		$$ = _token_create_ival (parser, SPACE, SPACE);
		$$->location = yylloc;
	}
;

operator:
	'['			{ $$ = '['; }
|	']'			{ $$ = ']'; }
|	'('			{ $$ = '('; }
|	')'			{ $$ = ')'; }
|	'{'			{ $$ = '{'; }
|	'}'			{ $$ = '}'; }
|	'.'			{ $$ = '.'; }
|	'&'			{ $$ = '&'; }
|	'*'			{ $$ = '*'; }
|	'+'			{ $$ = '+'; }
|	'-'			{ $$ = '-'; }
|	'~'			{ $$ = '~'; }
|	'!'			{ $$ = '!'; }
|	'/'			{ $$ = '/'; }
|	'%'			{ $$ = '%'; }
|	LEFT_SHIFT		{ $$ = LEFT_SHIFT; }
|	RIGHT_SHIFT		{ $$ = RIGHT_SHIFT; }
|	'<'			{ $$ = '<'; }
|	'>'			{ $$ = '>'; }
|	LESS_OR_EQUAL		{ $$ = LESS_OR_EQUAL; }
|	GREATER_OR_EQUAL	{ $$ = GREATER_OR_EQUAL; }
|	EQUAL			{ $$ = EQUAL; }
|	NOT_EQUAL		{ $$ = NOT_EQUAL; }
|	'^'			{ $$ = '^'; }
|	'|'			{ $$ = '|'; }
|	AND			{ $$ = AND; }
|	OR			{ $$ = OR; }
|	';'			{ $$ = ';'; }
|	','			{ $$ = ','; }
|	'='			{ $$ = '='; }
|	PASTE			{ $$ = PASTE; }
;

%%

string_list_t *
_string_list_create (void *ctx)
{
	string_list_t *list;

	list = ralloc (ctx, string_list_t);
	list->head = NULL;
	list->tail = NULL;

	return list;
}

void
_string_list_append_item (string_list_t *list, const char *str)
{
	string_node_t *node;

	node = ralloc (list, string_node_t);
	node->str = ralloc_strdup (node, str);

	node->next = NULL;

	if (list->head == NULL) {
		list->head = node;
	} else {
		list->tail->next = node;
	}

	list->tail = node;
}

int
_string_list_contains (string_list_t *list, const char *member, int *index)
{
	string_node_t *node;
	int i;

	if (list == NULL)
		return 0;

	for (i = 0, node = list->head; node; i++, node = node->next) {
		if (strcmp (node->str, member) == 0) {
			if (index)
				*index = i;
			return 1;
		}
	}

	return 0;
}

int
_string_list_length (string_list_t *list)
{
	int length = 0;
	string_node_t *node;

	if (list == NULL)
		return 0;

	for (node = list->head; node; node = node->next)
		length++;

	return length;
}

int
_string_list_equal (string_list_t *a, string_list_t *b)
{
	string_node_t *node_a, *node_b;

	if (a == NULL && b == NULL)
		return 1;

	if (a == NULL || b == NULL)
		return 0;

	for (node_a = a->head, node_b = b->head;
	     node_a && node_b;
	     node_a = node_a->next, node_b = node_b->next)
	{
		if (strcmp (node_a->str, node_b->str))
			return 0;
	}

	/* Catch the case of lists being different lengths, (which
	 * would cause the loop above to terminate after the shorter
	 * list). */
	return node_a == node_b;
}

argument_list_t *
_argument_list_create (void *ctx)
{
	argument_list_t *list;

	list = ralloc (ctx, argument_list_t);
	list->head = NULL;
	list->tail = NULL;

	return list;
}

void
_argument_list_append (argument_list_t *list, token_list_t *argument)
{
	argument_node_t *node;

	node = ralloc (list, argument_node_t);
	node->argument = argument;

	node->next = NULL;

	if (list->head == NULL) {
		list->head = node;
	} else {
		list->tail->next = node;
	}

	list->tail = node;
}

int
_argument_list_length (argument_list_t *list)
{
	int length = 0;
	argument_node_t *node;

	if (list == NULL)
		return 0;

	for (node = list->head; node; node = node->next)
		length++;

	return length;
}

token_list_t *
_argument_list_member_at (argument_list_t *list, int index)
{
	argument_node_t *node;
	int i;

	if (list == NULL)
		return NULL;

	node = list->head;
	for (i = 0; i < index; i++) {
		node = node->next;
		if (node == NULL)
			break;
	}

	if (node)
		return node->argument;

	return NULL;
}

/* Note: This function ralloc_steal()s the str pointer. */
token_t *
_token_create_str (void *ctx, int type, char *str)
{
	token_t *token;

	token = ralloc (ctx, token_t);
	token->type = type;
	token->value.str = str;

	ralloc_steal (token, str);

	return token;
}

token_t *
_token_create_ival (void *ctx, int type, int ival)
{
	token_t *token;

	token = ralloc (ctx, token_t);
	token->type = type;
	token->value.ival = ival;

	return token;
}

token_list_t *
_token_list_create (void *ctx)
{
	token_list_t *list;

	list = ralloc (ctx, token_list_t);
	list->head = NULL;
	list->tail = NULL;
	list->non_space_tail = NULL;

	return list;
}

void
_token_list_append (token_list_t *list, token_t *token)
{
	token_node_t *node;

	node = ralloc (list, token_node_t);
	node->token = token;
	node->next = NULL;

	if (list->head == NULL) {
		list->head = node;
	} else {
		list->tail->next = node;
	}

	list->tail = node;
	if (token->type != SPACE)
		list->non_space_tail = node;
}

void
_token_list_append_list (token_list_t *list, token_list_t *tail)
{
	if (tail == NULL || tail->head == NULL)
		return;

	if (list->head == NULL) {
		list->head = tail->head;
	} else {
		list->tail->next = tail->head;
	}

	list->tail = tail->tail;
	list->non_space_tail = tail->non_space_tail;
}

static token_list_t *
_token_list_copy (void *ctx, token_list_t *other)
{
	token_list_t *copy;
	token_node_t *node;

	if (other == NULL)
		return NULL;

	copy = _token_list_create (ctx);
	for (node = other->head; node; node = node->next) {
		token_t *new_token = ralloc (copy, token_t);
		*new_token = *node->token;
		_token_list_append (copy, new_token);
	}

	return copy;
}

static void
_token_list_trim_trailing_space (token_list_t *list)
{
	token_node_t *tail, *next;

	if (list->non_space_tail) {
		tail = list->non_space_tail->next;
		list->non_space_tail->next = NULL;
		list->tail = list->non_space_tail;

		while (tail) {
			next = tail->next;
			ralloc_free (tail);
			tail = next;
		}
	}
}

static int
_token_list_is_empty_ignoring_space (token_list_t *l)
{
	token_node_t *n;

	if (l == NULL)
		return 1;

	n = l->head;
	while (n != NULL && n->token->type == SPACE)
		n = n->next;

	return n == NULL;
}

int
_token_list_equal_ignoring_space (token_list_t *a, token_list_t *b)
{
	token_node_t *node_a, *node_b;

	if (a == NULL || b == NULL) {
		int a_empty = _token_list_is_empty_ignoring_space(a);
		int b_empty = _token_list_is_empty_ignoring_space(b);
		return a_empty == b_empty;
	}

	node_a = a->head;
	node_b = b->head;

	while (1)
	{
		if (node_a == NULL && node_b == NULL)
			break;

		if (node_a == NULL || node_b == NULL)
			return 0;

		if (node_a->token->type == SPACE) {
			node_a = node_a->next;
			continue;
		}

		if (node_b->token->type == SPACE) {
			node_b = node_b->next;
			continue;
		}

		if (node_a->token->type != node_b->token->type)
			return 0;

		switch (node_a->token->type) {
		case INTEGER:
			if (node_a->token->value.ival != 
			    node_b->token->value.ival)
			{
				return 0;
			}
			break;
		case IDENTIFIER:
		case INTEGER_STRING:
		case OTHER:
			if (strcmp (node_a->token->value.str,
				    node_b->token->value.str))
			{
				return 0;
			}
			break;
		}

		node_a = node_a->next;
		node_b = node_b->next;
	}

	return 1;
}

static void
_token_print (char **out, token_t *token)
{
	if (token->type < 256) {
		ralloc_asprintf_append (out, "%c", token->type);
		return;
	}

	switch (token->type) {
	case INTEGER:
		ralloc_asprintf_append (out, "%" PRIiMAX, token->value.ival);
		break;
	case IDENTIFIER:
	case INTEGER_STRING:
	case OTHER:
		ralloc_strcat (out, token->value.str);
		break;
	case SPACE:
		ralloc_strcat (out, " ");
		break;
	case LEFT_SHIFT:
		ralloc_strcat (out, "<<");
		break;
	case RIGHT_SHIFT:
		ralloc_strcat (out, ">>");
		break;
	case LESS_OR_EQUAL:
		ralloc_strcat (out, "<=");
		break;
	case GREATER_OR_EQUAL:
		ralloc_strcat (out, ">=");
		break;
	case EQUAL:
		ralloc_strcat (out, "==");
		break;
	case NOT_EQUAL:
		ralloc_strcat (out, "!=");
		break;
	case AND:
		ralloc_strcat (out, "&&");
		break;
	case OR:
		ralloc_strcat (out, "||");
		break;
	case PASTE:
		ralloc_strcat (out, "##");
		break;
	case COMMA_FINAL:
		ralloc_strcat (out, ",");
		break;
	case PLACEHOLDER:
		/* Nothing to print. */
		break;
	default:
		assert(!"Error: Don't know how to print token.");
		break;
	}
}

/* Return a new token (ralloc()ed off of 'token') formed by pasting
 * 'token' and 'other'. Note that this function may return 'token' or
 * 'other' directly rather than allocating anything new.
 *
 * Caution: Only very cursory error-checking is performed to see if
 * the final result is a valid single token. */
static token_t *
_token_paste (glcpp_parser_t *parser, token_t *token, token_t *other)
{
	token_t *combined = NULL;

	/* Pasting a placeholder onto anything makes no change. */
	if (other->type == PLACEHOLDER)
		return token;

	/* When 'token' is a placeholder, just return 'other'. */
	if (token->type == PLACEHOLDER)
		return other;

	/* A very few single-character punctuators can be combined
	 * with another to form a multi-character punctuator. */
	switch (token->type) {
	case '<':
		if (other->type == '<')
			combined = _token_create_ival (token, LEFT_SHIFT, LEFT_SHIFT);
		else if (other->type == '=')
			combined = _token_create_ival (token, LESS_OR_EQUAL, LESS_OR_EQUAL);
		break;
	case '>':
		if (other->type == '>')
			combined = _token_create_ival (token, RIGHT_SHIFT, RIGHT_SHIFT);
		else if (other->type == '=')
			combined = _token_create_ival (token, GREATER_OR_EQUAL, GREATER_OR_EQUAL);
		break;
	case '=':
		if (other->type == '=')
			combined = _token_create_ival (token, EQUAL, EQUAL);
		break;
	case '!':
		if (other->type == '=')
			combined = _token_create_ival (token, NOT_EQUAL, NOT_EQUAL);
		break;
	case '&':
		if (other->type == '&')
			combined = _token_create_ival (token, AND, AND);
		break;
	case '|':
		if (other->type == '|')
			combined = _token_create_ival (token, OR, OR);
		break;
	}

	if (combined != NULL) {
		/* Inherit the location from the first token */
		combined->location = token->location;
		return combined;
	}

	/* Two string-valued tokens can usually just be mashed
	 * together.
	 *
	 * XXX: This isn't actually legitimate. Several things here
	 * should result in a diagnostic since the result cannot be a
	 * valid, single pre-processing token. For example, pasting
	 * "123" and "abc" is not legal, but we don't catch that
	 * here. */
	if ((token->type == IDENTIFIER || token->type == OTHER || token->type == INTEGER_STRING) &&
	    (other->type == IDENTIFIER || other->type == OTHER || other->type == INTEGER_STRING))
	{
		char *str;

		str = ralloc_asprintf (token, "%s%s", token->value.str,
				       other->value.str);
		combined = _token_create_str (token, token->type, str);
		combined->location = token->location;
		return combined;
	}

	glcpp_error (&token->location, parser, "");
	ralloc_strcat (&parser->info_log, "Pasting \"");
	_token_print (&parser->info_log, token);
	ralloc_strcat (&parser->info_log, "\" and \"");
	_token_print (&parser->info_log, other);
	ralloc_strcat (&parser->info_log, "\" does not give a valid preprocessing token.\n");

	return token;
}

static void
_token_list_print (glcpp_parser_t *parser, token_list_t *list)
{
	token_node_t *node;

	if (list == NULL)
		return;

	for (node = list->head; node; node = node->next)
		_token_print (&parser->output, node->token);
}

void
yyerror (YYLTYPE *locp, glcpp_parser_t *parser, const char *error)
{
	glcpp_error(locp, parser, "%s", error);
}

static void add_builtin_define(glcpp_parser_t *parser,
			       const char *name, int value)
{
   token_t *tok;
   token_list_t *list;

   tok = _token_create_ival (parser, INTEGER, value);

   list = _token_list_create(parser);
   _token_list_append(list, tok);
   _define_object_macro(parser, NULL, name, list);
}

glcpp_parser_t *
glcpp_parser_create (const struct gl_extensions *extensions, int api)
{
	glcpp_parser_t *parser;
	int language_version;

	parser = ralloc (NULL, glcpp_parser_t);

	glcpp_lex_init_extra (parser, &parser->scanner);
	parser->defines = hash_table_ctor (32, hash_table_string_hash,
					   hash_table_string_compare);
	parser->active = NULL;
	parser->lexing_if = 0;
	parser->space_tokens = 1;
	parser->newline_as_space = 0;
	parser->in_control_line = 0;
	parser->paren_count = 0;

	parser->skip_stack = NULL;

	parser->lex_from_list = NULL;
	parser->lex_from_node = NULL;

	parser->output = ralloc_strdup(parser, "");
	parser->info_log = ralloc_strdup(parser, "");
	parser->error = 0;

	/* Add pre-defined macros. */
	add_builtin_define(parser, "GL_ARB_draw_buffers", 1);
	add_builtin_define(parser, "GL_ARB_texture_rectangle", 1);

	if (api == API_OPENGLES2)
		add_builtin_define(parser, "GL_ES", 1);

	if (extensions != NULL) {
	   if (extensions->EXT_texture_array) {
	      add_builtin_define(parser, "GL_EXT_texture_array", 1);
	   }

	   if (extensions->ARB_fragment_coord_conventions)
	      add_builtin_define(parser, "GL_ARB_fragment_coord_conventions",
				 1);

	   if (extensions->ARB_explicit_attrib_location)
	      add_builtin_define(parser, "GL_ARB_explicit_attrib_location", 1);

	   if (extensions->ARB_shader_texture_lod)
	      add_builtin_define(parser, "GL_ARB_shader_texture_lod", 1);

	   if (extensions->ARB_draw_instanced)
	      add_builtin_define(parser, "GL_ARB_draw_instanced", 1);

	   if (extensions->ARB_conservative_depth) {
	      add_builtin_define(parser, "GL_AMD_conservative_depth", 1);
	      add_builtin_define(parser, "GL_ARB_conservative_depth", 1);
	   }

	   if (extensions->OES_EGL_image_external)
	      add_builtin_define(parser, "GL_OES_EGL_image_external", 1);
	}

	language_version = 110;
	add_builtin_define(parser, "__VERSION__", language_version);

	return parser;
}

int
glcpp_parser_parse (glcpp_parser_t *parser)
{
	return yyparse (parser);
}

void
glcpp_parser_destroy (glcpp_parser_t *parser)
{
	glcpp_lex_destroy (parser->scanner);
	hash_table_dtor (parser->defines);
	ralloc_free (parser);
}

typedef enum function_status
{
	FUNCTION_STATUS_SUCCESS,
	FUNCTION_NOT_A_FUNCTION,
	FUNCTION_UNBALANCED_PARENTHESES
} function_status_t;

/* Find a set of function-like macro arguments by looking for a
 * balanced set of parentheses.
 *
 * When called, 'node' should be the opening-parenthesis token, (or
 * perhaps preceeding SPACE tokens). Upon successful return *last will
 * be the last consumed node, (corresponding to the closing right
 * parenthesis).
 *
 * Return values:
 *
 *   FUNCTION_STATUS_SUCCESS:
 *
 *	Successfully parsed a set of function arguments.	
 *
 *   FUNCTION_NOT_A_FUNCTION:
 *
 *	Macro name not followed by a '('. This is not an error, but
 *	simply that the macro name should be treated as a non-macro.
 *
 *   FUNCTION_UNBALANCED_PARENTHESES
 *
 *	Macro name is not followed by a balanced set of parentheses.
 */
static function_status_t
_arguments_parse (argument_list_t *arguments,
		  token_node_t *node,
		  token_node_t **last)
{
	token_list_t *argument;
	int paren_count;

	node = node->next;

	/* Ignore whitespace before first parenthesis. */
	while (node && node->token->type == SPACE)
		node = node->next;

	if (node == NULL || node->token->type != '(')
		return FUNCTION_NOT_A_FUNCTION;

	node = node->next;

	argument = _token_list_create (arguments);
	_argument_list_append (arguments, argument);

	for (paren_count = 1; node; node = node->next) {
		if (node->token->type == '(')
		{
			paren_count++;
		}
		else if (node->token->type == ')')
		{
			paren_count--;
			if (paren_count == 0)
				break;
		}

		if (node->token->type == ',' &&
			 paren_count == 1)
		{
			_token_list_trim_trailing_space (argument);
			argument = _token_list_create (arguments);
			_argument_list_append (arguments, argument);
		}
		else {
			if (argument->head == NULL) {
				/* Don't treat initial whitespace as
				 * part of the arguement. */
				if (node->token->type == SPACE)
					continue;
			}
			_token_list_append (argument, node->token);
		}
	}

	if (paren_count)
		return FUNCTION_UNBALANCED_PARENTHESES;

	*last = node;

	return FUNCTION_STATUS_SUCCESS;
}

static token_list_t *
_token_list_create_with_one_space (void *ctx)
{
	token_list_t *list;
	token_t *space;

	list = _token_list_create (ctx);
	space = _token_create_ival (list, SPACE, SPACE);
	_token_list_append (list, space);

	return list;
}

static void
_glcpp_parser_expand_if (glcpp_parser_t *parser, int type, token_list_t *list)
{
	token_list_t *expanded;
	token_t *token;

	expanded = _token_list_create (parser);
	token = _token_create_ival (parser, type, type);
	_token_list_append (expanded, token);
	_glcpp_parser_expand_token_list (parser, list);
	_token_list_append_list (expanded, list);
	glcpp_parser_lex_from (parser, expanded);
}

static void
_glcpp_parser_apply_pastes (glcpp_parser_t *parser, token_list_t *list)
{
	token_node_t *node;

	node = list->head;
	while (node)
	{
		token_node_t *next_non_space;

		/* Look ahead for a PASTE token, skipping space. */
		next_non_space = node->next;
		while (next_non_space && next_non_space->token->type == SPACE)
			next_non_space = next_non_space->next;

		if (next_non_space == NULL)
			break;

		if (next_non_space->token->type != PASTE) {
			node = next_non_space;
			continue;
		}

		/* Now find the next non-space token after the PASTE. */
		next_non_space = next_non_space->next;
		while (next_non_space && next_non_space->token->type == SPACE)
			next_non_space = next_non_space->next;

		if (next_non_space == NULL) {
			yyerror (&node->token->location, parser, "'##' cannot appear at either end of a macro expansion\n");
			return;
		}

		node->token = _token_paste (parser, node->token, next_non_space->token);
		node->next = next_non_space->next;
		if (next_non_space == list->tail)
			list->tail = node;
	}

	list->non_space_tail = list->tail;
}

/* This is a helper function that's essentially part of the
 * implementation of _glcpp_parser_expand_node. It shouldn't be called
 * except for by that function.
 *
 * Returns NULL if node is a simple token with no expansion, (that is,
 * although 'node' corresponds to an identifier defined as a
 * function-like macro, it is not followed with a parenthesized
 * argument list).
 *
 * Compute the complete expansion of node (which is a function-like
 * macro) and subsequent nodes which are arguments.
 *
 * Returns the token list that results from the expansion and sets
 * *last to the last node in the list that was consumed by the
 * expansion. Specifically, *last will be set as follows: as the
 * token of the closing right parenthesis.
 */
static token_list_t *
_glcpp_parser_expand_function (glcpp_parser_t *parser,
			       token_node_t *node,
			       token_node_t **last)
			       
{
	macro_t *macro;
	const char *identifier;
	argument_list_t *arguments;
	function_status_t status;
	token_list_t *substituted;
	int parameter_index;

	identifier = node->token->value.str;

	macro = hash_table_find (parser->defines, identifier);

	assert (macro->is_function);

	arguments = _argument_list_create (parser);
	status = _arguments_parse (arguments, node, last);

	switch (status) {
	case FUNCTION_STATUS_SUCCESS:
		break;
	case FUNCTION_NOT_A_FUNCTION:
		return NULL;
	case FUNCTION_UNBALANCED_PARENTHESES:
		glcpp_error (&node->token->location, parser, "Macro %s call has unbalanced parentheses\n", identifier);
		return NULL;
	}

	/* Replace a macro defined as empty with a SPACE token. */
	if (macro->replacements == NULL) {
		ralloc_free (arguments);
		return _token_list_create_with_one_space (parser);
	}

	if (! ((_argument_list_length (arguments) == 
		_string_list_length (macro->parameters)) ||
	       (_string_list_length (macro->parameters) == 0 &&
		_argument_list_length (arguments) == 1 &&
		arguments->head->argument->head == NULL)))
	{
		glcpp_error (&node->token->location, parser,
			      "Error: macro %s invoked with %d arguments (expected %d)\n",
			      identifier,
			      _argument_list_length (arguments),
			      _string_list_length (macro->parameters));
		return NULL;
	}

	/* Perform argument substitution on the replacement list. */
	substituted = _token_list_create (arguments);

	for (node = macro->replacements->head; node; node = node->next)
	{
		if (node->token->type == IDENTIFIER &&
		    _string_list_contains (macro->parameters,
					   node->token->value.str,
					   &parameter_index))
		{
			token_list_t *argument;
			argument = _argument_list_member_at (arguments,
							     parameter_index);
			/* Before substituting, we expand the argument
			 * tokens, or append a placeholder token for
			 * an empty argument. */
			if (argument->head) {
				token_list_t *expanded_argument;
				expanded_argument = _token_list_copy (parser,
								      argument);
				_glcpp_parser_expand_token_list (parser,
								 expanded_argument);
				_token_list_append_list (substituted,
							 expanded_argument);
			} else {
				token_t *new_token;

				new_token = _token_create_ival (substituted,
								PLACEHOLDER,
								PLACEHOLDER);
				_token_list_append (substituted, new_token);
			}
		} else {
			_token_list_append (substituted, node->token);
		}
	}

	/* After argument substitution, and before further expansion
	 * below, implement token pasting. */

	_token_list_trim_trailing_space (substituted);

	_glcpp_parser_apply_pastes (parser, substituted);

	return substituted;
}

/* Compute the complete expansion of node, (and subsequent nodes after
 * 'node' in the case that 'node' is a function-like macro and
 * subsequent nodes are arguments).
 *
 * Returns NULL if node is a simple token with no expansion.
 *
 * Otherwise, returns the token list that results from the expansion
 * and sets *last to the last node in the list that was consumed by
 * the expansion. Specifically, *last will be set as follows:
 *
 *	As 'node' in the case of object-like macro expansion.
 *
 *	As the token of the closing right parenthesis in the case of
 *	function-like macro expansion.
 */
static token_list_t *
_glcpp_parser_expand_node (glcpp_parser_t *parser,
			   token_node_t *node,
			   token_node_t **last)
{
	token_t *token = node->token;
	const char *identifier;
	macro_t *macro;

	/* We only expand identifiers */
	if (token->type != IDENTIFIER) {
		/* We change any COMMA into a COMMA_FINAL to prevent
		 * it being mistaken for an argument separator
		 * later. */
		if (token->type == ',') {
			token->type = COMMA_FINAL;
			token->value.ival = COMMA_FINAL;
		}

		return NULL;
	}

	/* Look up this identifier in the hash table. */
	identifier = token->value.str;
	macro = hash_table_find (parser->defines, identifier);

	/* Not a macro, so no expansion needed. */
	if (macro == NULL)
		return NULL;

	/* Finally, don't expand this macro if we're already actively
	 * expanding it, (to avoid infinite recursion). */
	if (_parser_active_list_contains (parser, identifier)) {
		/* We change the token type here from IDENTIFIER to
		 * OTHER to prevent any future expansion of this
		 * unexpanded token. */
		char *str;
		token_list_t *expansion;
		token_t *final;

		str = ralloc_strdup (parser, token->value.str);
		final = _token_create_str (parser, OTHER, str);
		expansion = _token_list_create (parser);
		_token_list_append (expansion, final);
		*last = node;
		return expansion;
	}

	if (! macro->is_function)
	{
		token_list_t *replacement;
		*last = node;

		/* Replace a macro defined as empty with a SPACE token. */
		if (macro->replacements == NULL)
			return _token_list_create_with_one_space (parser);

		replacement = _token_list_copy (parser, macro->replacements);
		_glcpp_parser_apply_pastes (parser, replacement);
		return replacement;
	}

	return _glcpp_parser_expand_function (parser, node, last);
}

/* Push a new identifier onto the parser's active list.
 *
 * Here, 'marker' is the token node that appears in the list after the
 * expansion of 'identifier'. That is, when the list iterator begins
 * examining 'marker', then it is time to pop this node from the
 * active stack.
 */
static void
_parser_active_list_push (glcpp_parser_t *parser,
			  const char *identifier,
			  token_node_t *marker)
{
	active_list_t *node;

	node = ralloc (parser->active, active_list_t);
	node->identifier = ralloc_strdup (node, identifier);
	node->marker = marker;
	node->next = parser->active;

	parser->active = node;
}

static void
_parser_active_list_pop (glcpp_parser_t *parser)
{
	active_list_t *node = parser->active;

	if (node == NULL) {
		parser->active = NULL;
		return;
	}

	node = parser->active->next;
	ralloc_free (parser->active);

	parser->active = node;
}

static int
_parser_active_list_contains (glcpp_parser_t *parser, const char *identifier)
{
	active_list_t *node;

	if (parser->active == NULL)
		return 0;

	for (node = parser->active; node; node = node->next)
		if (strcmp (node->identifier, identifier) == 0)
			return 1;

	return 0;
}

/* Walk over the token list replacing nodes with their expansion.
 * Whenever nodes are expanded the walking will walk over the new
 * nodes, continuing to expand as necessary. The results are placed in
 * 'list' itself;
 */
static void
_glcpp_parser_expand_token_list (glcpp_parser_t *parser,
				 token_list_t *list)
{
	token_node_t *node_prev;
	token_node_t *node, *last = NULL;
	token_list_t *expansion;
	active_list_t *active_initial = parser->active;

	if (list == NULL)
		return;

	_token_list_trim_trailing_space (list);

	node_prev = NULL;
	node = list->head;

	while (node) {

		while (parser->active && parser->active->marker == node)
			_parser_active_list_pop (parser);

		expansion = _glcpp_parser_expand_node (parser, node, &last);
		if (expansion) {
			token_node_t *n;

			for (n = node; n != last->next; n = n->next)
				while (parser->active &&
				       parser->active->marker == n)
				{
					_parser_active_list_pop (parser);
				}

			_parser_active_list_push (parser,
						  node->token->value.str,
						  last->next);
			
			/* Splice expansion into list, supporting a
			 * simple deletion if the expansion is
			 * empty. */
			if (expansion->head) {
				if (node_prev)
					node_prev->next = expansion->head;
				else
					list->head = expansion->head;
				expansion->tail->next = last->next;
				if (last == list->tail)
					list->tail = expansion->tail;
			} else {
				if (node_prev)
					node_prev->next = last->next;
				else
					list->head = last->next;
				if (last == list->tail)
					list->tail = NULL;
			}
		} else {
			node_prev = node;
		}
		node = node_prev ? node_prev->next : list->head;
	}

	/* Remove any lingering effects of this invocation on the
	 * active list. That is, pop until the list looks like it did
	 * at the beginning of this function. */
	while (parser->active && parser->active != active_initial)
		_parser_active_list_pop (parser);

	list->non_space_tail = list->tail;
}

void
_glcpp_parser_print_expanded_token_list (glcpp_parser_t *parser,
					 token_list_t *list)
{
	if (list == NULL)
		return;

	_glcpp_parser_expand_token_list (parser, list);

	_token_list_trim_trailing_space (list);

	_token_list_print (parser, list);
}

static void
_check_for_reserved_macro_name (glcpp_parser_t *parser, YYLTYPE *loc,
				const char *identifier)
{
	/* According to the GLSL specification, macro names starting with "__"
	 * or "GL_" are reserved for future use.  So, don't allow them.
	 */
	if (strstr(identifier, "__")) {
		glcpp_error (loc, parser, "Macro names containing \"__\" are reserved.\n");
	}
	if (strncmp(identifier, "GL_", 3) == 0) {
		glcpp_error (loc, parser, "Macro names starting with \"GL_\" are reserved.\n");
	}
}

static int
_macro_equal (macro_t *a, macro_t *b)
{
	if (a->is_function != b->is_function)
		return 0;

	if (a->is_function) {
		if (! _string_list_equal (a->parameters, b->parameters))
			return 0;
	}

	return _token_list_equal_ignoring_space (a->replacements,
						 b->replacements);
}

void
_define_object_macro (glcpp_parser_t *parser,
		      YYLTYPE *loc,
		      const char *identifier,
		      token_list_t *replacements)
{
	macro_t *macro, *previous;

	if (loc != NULL)
		_check_for_reserved_macro_name(parser, loc, identifier);

	macro = ralloc (parser, macro_t);

	macro->is_function = 0;
	macro->parameters = NULL;
	macro->identifier = ralloc_strdup (macro, identifier);
	macro->replacements = replacements;
	ralloc_steal (macro, replacements);

	previous = hash_table_find (parser->defines, identifier);
	if (previous) {
		if (_macro_equal (macro, previous)) {
			ralloc_free (macro);
			return;
		}
		glcpp_error (loc, parser, "Redefinition of macro %s\n",
			     identifier);
	}

	hash_table_insert (parser->defines, macro, identifier);
}

void
_define_function_macro (glcpp_parser_t *parser,
			YYLTYPE *loc,
			const char *identifier,
			string_list_t *parameters,
			token_list_t *replacements)
{
	macro_t *macro, *previous;

	_check_for_reserved_macro_name(parser, loc, identifier);

	macro = ralloc (parser, macro_t);
	ralloc_steal (macro, parameters);
	ralloc_steal (macro, replacements);

	macro->is_function = 1;
	macro->parameters = parameters;
	macro->identifier = ralloc_strdup (macro, identifier);
	macro->replacements = replacements;
	previous = hash_table_find (parser->defines, identifier);
	if (previous) {
		if (_macro_equal (macro, previous)) {
			ralloc_free (macro);
			return;
		}
		glcpp_error (loc, parser, "Redefinition of macro %s\n",
			     identifier);
	}

	hash_table_insert (parser->defines, macro, identifier);
}

static int
glcpp_parser_lex (YYSTYPE *yylval, YYLTYPE *yylloc, glcpp_parser_t *parser)
{
	token_node_t *node;
	int ret;

	if (parser->lex_from_list == NULL) {
		ret = glcpp_lex (yylval, yylloc, parser->scanner);

		/* XXX: This ugly block of code exists for the sole
		 * purpose of converting a NEWLINE token into a SPACE
		 * token, but only in the case where we have seen a
		 * function-like macro name, but have not yet seen its
		 * closing parenthesis.
		 *
		 * There's perhaps a more compact way to do this with
		 * mid-rule actions in the grammar.
		 *
		 * I'm definitely not pleased with the complexity of
		 * this code here.
		 */
		if (parser->newline_as_space)
		{
			if (ret == '(') {
				parser->paren_count++;
			} else if (ret == ')') {
				parser->paren_count--;
				if (parser->paren_count == 0)
					parser->newline_as_space = 0;
			} else if (ret == NEWLINE) {
				ret = SPACE;
			} else if (ret != SPACE) {
				if (parser->paren_count == 0)
					parser->newline_as_space = 0;
			}
		}
		else if (parser->in_control_line)
		{
			if (ret == NEWLINE)
				parser->in_control_line = 0;
		}
		else if (ret == HASH_DEFINE_OBJ || ret == HASH_DEFINE_FUNC ||
			   ret == HASH_UNDEF || ret == HASH_IF ||
			   ret == HASH_IFDEF || ret == HASH_IFNDEF ||
			   ret == HASH_ELIF || ret == HASH_ELSE ||
			   ret == HASH_ENDIF || ret == HASH)
		{
			parser->in_control_line = 1;
		}
		else if (ret == IDENTIFIER)
		{
			macro_t *macro;
			macro = hash_table_find (parser->defines,
						 yylval->str);
			if (macro && macro->is_function) {
				parser->newline_as_space = 1;
				parser->paren_count = 0;
			}
		}

		return ret;
	}

	node = parser->lex_from_node;

	if (node == NULL) {
		ralloc_free (parser->lex_from_list);
		parser->lex_from_list = NULL;
		return NEWLINE;
	}

	*yylval = node->token->value;
	ret = node->token->type;

	parser->lex_from_node = node->next;

	return ret;
}

static void
glcpp_parser_lex_from (glcpp_parser_t *parser, token_list_t *list)
{
	token_node_t *node;

	assert (parser->lex_from_list == NULL);

	/* Copy list, eliminating any space tokens. */
	parser->lex_from_list = _token_list_create (parser);

	for (node = list->head; node; node = node->next) {
		if (node->token->type == SPACE)
			continue;
		_token_list_append (parser->lex_from_list, node->token);
	}

	ralloc_free (list);

	parser->lex_from_node = parser->lex_from_list->head;

	/* It's possible the list consisted of nothing but whitespace. */
	if (parser->lex_from_node == NULL) {
		ralloc_free (parser->lex_from_list);
		parser->lex_from_list = NULL;
	}
}

static void
_glcpp_parser_skip_stack_push_if (glcpp_parser_t *parser, YYLTYPE *loc,
				  int condition)
{
	skip_type_t current = SKIP_NO_SKIP;
	skip_node_t *node;

	if (parser->skip_stack)
		current = parser->skip_stack->type;

	node = ralloc (parser, skip_node_t);
	node->loc = *loc;

	if (current == SKIP_NO_SKIP) {
		if (condition)
			node->type = SKIP_NO_SKIP;
		else
			node->type = SKIP_TO_ELSE;
	} else {
		node->type = SKIP_TO_ENDIF;
	}

	node->next = parser->skip_stack;
	parser->skip_stack = node;
}

static void
_glcpp_parser_skip_stack_change_if (glcpp_parser_t *parser, YYLTYPE *loc,
				    const char *type, int condition)
{
	if (parser->skip_stack == NULL) {
		glcpp_error (loc, parser, "%s without #if\n", type);
		return;
	}

	if (parser->skip_stack->type == SKIP_TO_ELSE) {
		if (condition)
			parser->skip_stack->type = SKIP_NO_SKIP;
	} else {
		parser->skip_stack->type = SKIP_TO_ENDIF;
	}
}

static void
_glcpp_parser_skip_stack_pop (glcpp_parser_t *parser, YYLTYPE *loc)
{
	skip_node_t *node;

	if (parser->skip_stack == NULL) {
		glcpp_error (loc, parser, "#endif without #if\n");
		return;
	}

	node = parser->skip_stack;
	parser->skip_stack = node->next;
	ralloc_free (node);
}
