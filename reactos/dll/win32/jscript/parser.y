/*
 * Copyright 2008 Jacek Caban for CodeWeavers
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

%{

#include "jscript.h"

static int parser_error(parser_ctx_t*,const char*);
static void set_error(parser_ctx_t*,UINT);
static BOOL explicit_error(parser_ctx_t*,void*,WCHAR);
static BOOL allow_auto_semicolon(parser_ctx_t*);
static void program_parsed(parser_ctx_t*,source_elements_t*);

typedef struct _statement_list_t {
    statement_t *head;
    statement_t *tail;
} statement_list_t;

static literal_t *new_string_literal(parser_ctx_t*,const WCHAR*);
static literal_t *new_null_literal(parser_ctx_t*);

typedef struct _property_list_t {
    prop_val_t *head;
    prop_val_t *tail;
} property_list_t;

static property_list_t *new_property_list(parser_ctx_t*,literal_t*,expression_t*);
static property_list_t *property_list_add(parser_ctx_t*,property_list_t*,literal_t*,expression_t*);

typedef struct _element_list_t {
    array_element_t *head;
    array_element_t *tail;
} element_list_t;

static element_list_t *new_element_list(parser_ctx_t*,int,expression_t*);
static element_list_t *element_list_add(parser_ctx_t*,element_list_t*,int,expression_t*);

typedef struct _argument_list_t {
    argument_t *head;
    argument_t *tail;
} argument_list_t;

static argument_list_t *new_argument_list(parser_ctx_t*,expression_t*);
static argument_list_t *argument_list_add(parser_ctx_t*,argument_list_t*,expression_t*);

typedef struct _case_list_t {
    case_clausule_t *head;
    case_clausule_t *tail;
} case_list_t;

static catch_block_t *new_catch_block(parser_ctx_t*,const WCHAR*,statement_t*);
static case_clausule_t *new_case_clausule(parser_ctx_t*,expression_t*,statement_list_t*);
static case_list_t *new_case_list(parser_ctx_t*,case_clausule_t*);
static case_list_t *case_list_add(parser_ctx_t*,case_list_t*,case_clausule_t*);
static case_clausule_t *new_case_block(parser_ctx_t*,case_list_t*,case_clausule_t*,case_list_t*);

typedef struct _variable_list_t {
    variable_declaration_t *head;
    variable_declaration_t *tail;
} variable_list_t;

static variable_declaration_t *new_variable_declaration(parser_ctx_t*,const WCHAR*,expression_t*);
static variable_list_t *new_variable_list(parser_ctx_t*,variable_declaration_t*);
static variable_list_t *variable_list_add(parser_ctx_t*,variable_list_t*,variable_declaration_t*);

static void *new_statement(parser_ctx_t*,statement_type_t,size_t);
static statement_t *new_block_statement(parser_ctx_t*,statement_list_t*);
static statement_t *new_var_statement(parser_ctx_t*,variable_list_t*);
static statement_t *new_expression_statement(parser_ctx_t*,expression_t*);
static statement_t *new_if_statement(parser_ctx_t*,expression_t*,statement_t*,statement_t*);
static statement_t *new_while_statement(parser_ctx_t*,BOOL,expression_t*,statement_t*);
static statement_t *new_for_statement(parser_ctx_t*,variable_list_t*,expression_t*,expression_t*,
        expression_t*,statement_t*);
static statement_t *new_forin_statement(parser_ctx_t*,variable_declaration_t*,expression_t*,expression_t*,statement_t*);
static statement_t *new_continue_statement(parser_ctx_t*,const WCHAR*);
static statement_t *new_break_statement(parser_ctx_t*,const WCHAR*);
static statement_t *new_return_statement(parser_ctx_t*,expression_t*);
static statement_t *new_with_statement(parser_ctx_t*,expression_t*,statement_t*);
static statement_t *new_labelled_statement(parser_ctx_t*,const WCHAR*,statement_t*);
static statement_t *new_switch_statement(parser_ctx_t*,expression_t*,case_clausule_t*);
static statement_t *new_throw_statement(parser_ctx_t*,expression_t*);
static statement_t *new_try_statement(parser_ctx_t*,statement_t*,catch_block_t*,statement_t*);

struct statement_list_t {
   statement_t *head;
   statement_t *tail;
};

static statement_list_t *new_statement_list(parser_ctx_t*,statement_t*);
static statement_list_t *statement_list_add(statement_list_t*,statement_t*);

typedef struct _parameter_list_t {
    parameter_t *head;
    parameter_t *tail;
} parameter_list_t;

static parameter_list_t *new_parameter_list(parser_ctx_t*,const WCHAR*);
static parameter_list_t *parameter_list_add(parser_ctx_t*,parameter_list_t*,const WCHAR*);

static void *new_expression(parser_ctx_t *ctx,expression_type_t,size_t);
static expression_t *new_function_expression(parser_ctx_t*,const WCHAR*,parameter_list_t*,
        source_elements_t*,const WCHAR*,DWORD);
static expression_t *new_binary_expression(parser_ctx_t*,expression_type_t,expression_t*,expression_t*);
static expression_t *new_unary_expression(parser_ctx_t*,expression_type_t,expression_t*);
static expression_t *new_conditional_expression(parser_ctx_t*,expression_t*,expression_t*,expression_t*);
static expression_t *new_member_expression(parser_ctx_t*,expression_t*,const WCHAR*);
static expression_t *new_new_expression(parser_ctx_t*,expression_t*,argument_list_t*);
static expression_t *new_call_expression(parser_ctx_t*,expression_t*,argument_list_t*);
static expression_t *new_identifier_expression(parser_ctx_t*,const WCHAR*);
static expression_t *new_literal_expression(parser_ctx_t*,literal_t*);
static expression_t *new_array_literal_expression(parser_ctx_t*,element_list_t*,int);
static expression_t *new_prop_and_value_expression(parser_ctx_t*,property_list_t*);

static source_elements_t *new_source_elements(parser_ctx_t*);
static source_elements_t *source_elements_add_statement(source_elements_t*,statement_t*);

%}

%lex-param { parser_ctx_t *ctx }
%parse-param { parser_ctx_t *ctx }
%pure-parser
%start Program

%union {
    int                     ival;
    const WCHAR             *srcptr;
    LPCWSTR                 wstr;
    literal_t               *literal;
    struct _argument_list_t *argument_list;
    case_clausule_t         *case_clausule;
    struct _case_list_t     *case_list;
    catch_block_t           *catch_block;
    struct _element_list_t  *element_list;
    expression_t            *expr;
    const WCHAR            *identifier;
    struct _parameter_list_t *parameter_list;
    struct _property_list_t *property_list;
    source_elements_t       *source_elements;
    statement_t             *statement;
    struct _statement_list_t *statement_list;
    struct _variable_list_t *variable_list;
    variable_declaration_t  *variable_declaration;
}

/* keywords */
%token kBREAK kCASE kCATCH kCONTINUE kDEFAULT kDELETE kDO kELSE kIF kFINALLY kFOR kIN
%token kINSTANCEOF kNEW kNULL kRETURN kSWITCH kTHIS kTHROW kTRUE kFALSE kTRY kTYPEOF kVAR kVOID kWHILE kWITH
%token tANDAND tOROR tINC tDEC tHTMLCOMMENT kDIVEQ

%token <srcptr> kFUNCTION '}'

/* tokens */
%token <identifier> tIdentifier
%token <ival> tAssignOper tEqOper tShiftOper tRelOper
%token <literal> tNumericLiteral tBooleanLiteral
%token <wstr> tStringLiteral
%token tEOF

%type <source_elements> SourceElements
%type <source_elements> FunctionBody
%type <statement> Statement
%type <statement> Block
%type <statement> VariableStatement
%type <statement> EmptyStatement
%type <statement> ExpressionStatement
%type <statement> IfStatement
%type <statement> IterationStatement
%type <statement> ContinueStatement
%type <statement> BreakStatement
%type <statement> ReturnStatement
%type <statement> WithStatement
%type <statement> LabelledStatement
%type <statement> SwitchStatement
%type <statement> ThrowStatement
%type <statement> TryStatement
%type <statement> Finally
%type <statement_list> StatementList StatementList_opt
%type <parameter_list> FormalParameterList FormalParameterList_opt
%type <expr> Expression Expression_opt Expression_err
%type <expr> ExpressionNoIn ExpressionNoIn_opt
%type <expr> FunctionExpression
%type <expr> AssignmentExpression AssignmentExpressionNoIn
%type <expr> ConditionalExpression ConditionalExpressionNoIn
%type <expr> LeftHandSideExpression
%type <expr> LogicalORExpression LogicalORExpressionNoIn
%type <expr> LogicalANDExpression LogicalANDExpressionNoIn
%type <expr> BitwiseORExpression BitwiseORExpressionNoIn
%type <expr> BitwiseXORExpression BitwiseXORExpressionNoIn
%type <expr> BitwiseANDExpression BitwiseANDExpressionNoIn
%type <expr> EqualityExpression EqualityExpressionNoIn
%type <expr> RelationalExpression RelationalExpressionNoIn
%type <expr> ShiftExpression
%type <expr> AdditiveExpression
%type <expr> MultiplicativeExpression
%type <expr> Initialiser_opt Initialiser
%type <expr> InitialiserNoIn_opt InitialiserNoIn
%type <expr> UnaryExpression
%type <expr> PostfixExpression
%type <expr> NewExpression
%type <expr> CallExpression
%type <expr> MemberExpression
%type <expr> PrimaryExpression
%type <identifier> Identifier_opt
%type <variable_list> VariableDeclarationList
%type <variable_list> VariableDeclarationListNoIn
%type <variable_declaration> VariableDeclaration
%type <variable_declaration> VariableDeclarationNoIn
%type <case_list> CaseClausules CaseClausules_opt
%type <case_clausule> CaseClausule DefaultClausule CaseBlock
%type <catch_block> Catch
%type <argument_list> Arguments
%type <argument_list> ArgumentList
%type <literal> Literal
%type <expr> ArrayLiteral
%type <expr> ObjectLiteral
%type <ival> Elision Elision_opt
%type <element_list> ElementList
%type <property_list> PropertyNameAndValueList
%type <literal> PropertyName
%type <literal> BooleanLiteral
%type <srcptr> KFunction
%type <ival> AssignOper

%nonassoc LOWER_THAN_ELSE
%nonassoc kELSE

%%

/* ECMA-262 3rd Edition    14 */
Program
       : SourceElements HtmlComment tEOF
                                { program_parsed(ctx, $1); }

HtmlComment
        : tHTMLCOMMENT          {}
        | /* empty */           {}

/* ECMA-262 3rd Edition    14 */
SourceElements
        : /* empty */           { $$ = new_source_elements(ctx); }
        | SourceElements Statement
                                { $$ = source_elements_add_statement($1, $2); }

/* ECMA-262 3rd Edition    13 */
FunctionExpression
        : KFunction Identifier_opt left_bracket FormalParameterList_opt right_bracket '{' FunctionBody '}'
                                { $$ = new_function_expression(ctx, $2, $4, $7, $1, $8-$1+1); }

KFunction
        : kFUNCTION             { $$ = $1; }

/* ECMA-262 3rd Edition    13 */
FunctionBody
        : SourceElements        { $$ = $1; }

/* ECMA-262 3rd Edition    13 */
FormalParameterList
        : tIdentifier           { $$ = new_parameter_list(ctx, $1); }
        | FormalParameterList ',' tIdentifier
                                { $$ = parameter_list_add(ctx, $1, $3); }

/* ECMA-262 3rd Edition    13 */
FormalParameterList_opt
        : /* empty */           { $$ = NULL; }
        | FormalParameterList   { $$ = $1; }

/* ECMA-262 3rd Edition    12 */
Statement
        : Block                 { $$ = $1; }
        | VariableStatement     { $$ = $1; }
        | EmptyStatement        { $$ = $1; }
        | FunctionExpression    { $$ = new_expression_statement(ctx, $1); }
        | ExpressionStatement   { $$ = $1; }
        | IfStatement           { $$ = $1; }
        | IterationStatement    { $$ = $1; }
        | ContinueStatement     { $$ = $1; }
        | BreakStatement        { $$ = $1; }
        | ReturnStatement       { $$ = $1; }
        | WithStatement         { $$ = $1; }
        | LabelledStatement     { $$ = $1; }
        | SwitchStatement       { $$ = $1; }
        | ThrowStatement        { $$ = $1; }
        | TryStatement          { $$ = $1; }

/* ECMA-262 3rd Edition    12.2 */
StatementList
        : Statement             { $$ = new_statement_list(ctx, $1); }
        | StatementList Statement
                                { $$ = statement_list_add($1, $2); }

/* ECMA-262 3rd Edition    12.2 */
StatementList_opt
        : /* empty */           { $$ = NULL; }
        | StatementList         { $$ = $1; }

/* ECMA-262 3rd Edition    12.1 */
Block
        : '{' StatementList '}' { $$ = new_block_statement(ctx, $2); }
        | '{' '}'               { $$ = new_block_statement(ctx, NULL); }

/* ECMA-262 3rd Edition    12.2 */
VariableStatement
        : kVAR VariableDeclarationList semicolon_opt
                                { $$ = new_var_statement(ctx, $2); }

/* ECMA-262 3rd Edition    12.2 */
VariableDeclarationList
        : VariableDeclaration   { $$ = new_variable_list(ctx, $1); }
        | VariableDeclarationList ',' VariableDeclaration
                                { $$ = variable_list_add(ctx, $1, $3); }

/* ECMA-262 3rd Edition    12.2 */
VariableDeclarationListNoIn
        : VariableDeclarationNoIn
                                { $$ = new_variable_list(ctx, $1); }
        | VariableDeclarationListNoIn ',' VariableDeclarationNoIn
                                { $$ = variable_list_add(ctx, $1, $3); }

/* ECMA-262 3rd Edition    12.2 */
VariableDeclaration
        : tIdentifier Initialiser_opt
                                { $$ = new_variable_declaration(ctx, $1, $2); }

/* ECMA-262 3rd Edition    12.2 */
VariableDeclarationNoIn
        : tIdentifier InitialiserNoIn_opt
                                { $$ = new_variable_declaration(ctx, $1, $2); }

/* ECMA-262 3rd Edition    12.2 */
Initialiser_opt
        : /* empty */           { $$ = NULL; }
        | Initialiser           { $$ = $1; }

/* ECMA-262 3rd Edition    12.2 */
Initialiser
        : '=' AssignmentExpression
                                { $$ = $2; }

/* ECMA-262 3rd Edition    12.2 */
InitialiserNoIn_opt
        : /* empty */           { $$ = NULL; }
        | InitialiserNoIn       { $$ = $1; }

/* ECMA-262 3rd Edition    12.2 */
InitialiserNoIn
        : '=' AssignmentExpressionNoIn
                                { $$ = $2; }

/* ECMA-262 3rd Edition    12.3 */
EmptyStatement
        : ';'                   { $$ = new_statement(ctx, STAT_EMPTY, 0); }

/* ECMA-262 3rd Edition    12.4 */
ExpressionStatement
        : Expression semicolon_opt
                                { $$ = new_expression_statement(ctx, $1); }

/* ECMA-262 3rd Edition    12.5 */
IfStatement
        : kIF left_bracket Expression_err right_bracket Statement kELSE Statement
                                { $$ = new_if_statement(ctx, $3, $5, $7); }
        | kIF left_bracket Expression_err right_bracket Statement %prec LOWER_THAN_ELSE
                                { $$ = new_if_statement(ctx, $3, $5, NULL); }

/* ECMA-262 3rd Edition    12.6 */
IterationStatement
        : kDO Statement kWHILE left_bracket Expression_err right_bracket semicolon_opt
                                { $$ = new_while_statement(ctx, TRUE, $5, $2); }
        | kWHILE left_bracket Expression_err right_bracket Statement
                                { $$ = new_while_statement(ctx, FALSE, $3, $5); }
        | kFOR left_bracket ExpressionNoIn_opt
                                { if(!explicit_error(ctx, $3, ';')) YYABORT; }
        semicolon  Expression_opt
                                { if(!explicit_error(ctx, $6, ';')) YYABORT; }
        semicolon Expression_opt right_bracket Statement
                                { $$ = new_for_statement(ctx, NULL, $3, $6, $9, $11); }
        | kFOR left_bracket kVAR VariableDeclarationListNoIn
                                { if(!explicit_error(ctx, $4, ';')) YYABORT; }
        semicolon Expression_opt
                                { if(!explicit_error(ctx, $7, ';')) YYABORT; }
        semicolon Expression_opt right_bracket Statement
                                { $$ = new_for_statement(ctx, $4, NULL, $7, $10, $12); }
        | kFOR left_bracket LeftHandSideExpression kIN Expression_err right_bracket Statement
                                { $$ = new_forin_statement(ctx, NULL, $3, $5, $7); }
        | kFOR left_bracket kVAR VariableDeclarationNoIn kIN Expression_err right_bracket Statement
                                { $$ = new_forin_statement(ctx, $4, NULL, $6, $8); }

/* ECMA-262 3rd Edition    12.7 */
ContinueStatement
        : kCONTINUE /* NONL */ Identifier_opt semicolon_opt
                                { $$ = new_continue_statement(ctx, $2); }

/* ECMA-262 3rd Edition    12.8 */
BreakStatement
        : kBREAK /* NONL */ Identifier_opt semicolon_opt
                                { $$ = new_break_statement(ctx, $2); }

/* ECMA-262 3rd Edition    12.9 */
ReturnStatement
        : kRETURN /* NONL */ Expression_opt semicolon_opt
                                { $$ = new_return_statement(ctx, $2); }

/* ECMA-262 3rd Edition    12.10 */
WithStatement
        : kWITH left_bracket Expression right_bracket Statement
                                { $$ = new_with_statement(ctx, $3, $5); }

/* ECMA-262 3rd Edition    12.12 */
LabelledStatement
        : tIdentifier ':' Statement
                                { $$ = new_labelled_statement(ctx, $1, $3); }

/* ECMA-262 3rd Edition    12.11 */
SwitchStatement
        : kSWITCH left_bracket Expression right_bracket CaseBlock
                                { $$ = new_switch_statement(ctx, $3, $5); }

/* ECMA-262 3rd Edition    12.11 */
CaseBlock
        : '{' CaseClausules_opt '}'
                                 { $$ = new_case_block(ctx, $2, NULL, NULL); }
        | '{' CaseClausules_opt DefaultClausule CaseClausules_opt '}'
                                 { $$ = new_case_block(ctx, $2, $3, $4); }

/* ECMA-262 3rd Edition    12.11 */
CaseClausules_opt
        : /* empty */            { $$ = NULL; }
        | CaseClausules          { $$ = $1; }

/* ECMA-262 3rd Edition    12.11 */
CaseClausules
        : CaseClausule           { $$ = new_case_list(ctx, $1); }
        | CaseClausules CaseClausule
                                 { $$ = case_list_add(ctx, $1, $2); }

/* ECMA-262 3rd Edition    12.11 */
CaseClausule
        : kCASE Expression ':' StatementList_opt
                                 { $$ = new_case_clausule(ctx, $2, $4); }

/* ECMA-262 3rd Edition    12.11 */
DefaultClausule
        : kDEFAULT ':' StatementList_opt
                                 { $$ = new_case_clausule(ctx, NULL, $3); }

/* ECMA-262 3rd Edition    12.13 */
ThrowStatement
        : kTHROW /* NONL */ Expression semicolon_opt
                                { $$ = new_throw_statement(ctx, $2); }

/* ECMA-262 3rd Edition    12.14 */
TryStatement
        : kTRY Block Catch      { $$ = new_try_statement(ctx, $2, $3, NULL); }
        | kTRY Block Finally    { $$ = new_try_statement(ctx, $2, NULL, $3); }
        | kTRY Block Catch Finally
                                { $$ = new_try_statement(ctx, $2, $3, $4); }

/* ECMA-262 3rd Edition    12.14 */
Catch
        : kCATCH left_bracket tIdentifier right_bracket Block
                                { $$ = new_catch_block(ctx, $3, $5); }

/* ECMA-262 3rd Edition    12.14 */
Finally
        : kFINALLY Block        { $$ = $2; }

/* ECMA-262 3rd Edition    11.14 */
Expression_opt
        : /* empty */           { $$ = NULL; }
        | Expression            { $$ = $1; }

Expression_err
        : Expression            { $$ = $1; }
        | error                 { set_error(ctx, JS_E_SYNTAX); YYABORT; }

/* ECMA-262 3rd Edition    11.14 */
Expression
        : AssignmentExpression  { $$ = $1; }
        | Expression ',' AssignmentExpression
                                { $$ = new_binary_expression(ctx, EXPR_COMMA, $1, $3); }

/* ECMA-262 3rd Edition    11.14 */
ExpressionNoIn_opt
        : /* empty */           { $$ = NULL; }
        | ExpressionNoIn        { $$ = $1; }

/* ECMA-262 3rd Edition    11.14 */
ExpressionNoIn
        : AssignmentExpressionNoIn
                                { $$ = $1; }
        | ExpressionNoIn ',' AssignmentExpressionNoIn
                                { $$ = new_binary_expression(ctx, EXPR_COMMA, $1, $3); }

AssignOper
        : tAssignOper           { $$ = $1; }
        | kDIVEQ                { $$ = EXPR_ASSIGNDIV; }

/* ECMA-262 3rd Edition    11.13 */
AssignmentExpression
        : ConditionalExpression { $$ = $1; }
        | LeftHandSideExpression '=' AssignmentExpression
                                { $$ = new_binary_expression(ctx, EXPR_ASSIGN, $1, $3); }
        | LeftHandSideExpression AssignOper AssignmentExpression
                                { $$ = new_binary_expression(ctx, $2, $1, $3); }

/* ECMA-262 3rd Edition    11.13 */
AssignmentExpressionNoIn
        : ConditionalExpressionNoIn
                                { $$ = $1; }
        | LeftHandSideExpression '=' AssignmentExpressionNoIn
                                { $$ = new_binary_expression(ctx, EXPR_ASSIGN, $1, $3); }
        | LeftHandSideExpression AssignOper AssignmentExpressionNoIn
                                { $$ = new_binary_expression(ctx, $2, $1, $3); }

/* ECMA-262 3rd Edition    11.12 */
ConditionalExpression
        : LogicalORExpression   { $$ = $1; }
        | LogicalORExpression '?' AssignmentExpression ':' AssignmentExpression
                                { $$ = new_conditional_expression(ctx, $1, $3, $5); }

/* ECMA-262 3rd Edition    11.12 */
ConditionalExpressionNoIn
        : LogicalORExpressionNoIn
                                { $$ = $1; }
        | LogicalORExpressionNoIn '?' AssignmentExpressionNoIn ':' AssignmentExpressionNoIn
                                { $$ = new_conditional_expression(ctx, $1, $3, $5); }

/* ECMA-262 3rd Edition    11.11 */
LogicalORExpression
        : LogicalANDExpression  { $$ = $1; }
        | LogicalORExpression tOROR LogicalANDExpression
                                { $$ = new_binary_expression(ctx, EXPR_OR, $1, $3); }

/* ECMA-262 3rd Edition    11.11 */
LogicalORExpressionNoIn
        : LogicalANDExpressionNoIn
                                { $$ = $1; }
        | LogicalORExpressionNoIn tOROR LogicalANDExpressionNoIn
                                { $$ = new_binary_expression(ctx, EXPR_OR, $1, $3); }

/* ECMA-262 3rd Edition    11.11 */
LogicalANDExpression
        : BitwiseORExpression   { $$ = $1; }
        | LogicalANDExpression tANDAND BitwiseORExpression
                                { $$ = new_binary_expression(ctx, EXPR_AND, $1, $3); }

/* ECMA-262 3rd Edition    11.11 */
LogicalANDExpressionNoIn
        : BitwiseORExpressionNoIn
                                { $$ = $1; }
        | LogicalANDExpressionNoIn tANDAND BitwiseORExpressionNoIn
                                { $$ = new_binary_expression(ctx, EXPR_AND, $1, $3); }

/* ECMA-262 3rd Edition    11.10 */
BitwiseORExpression
        : BitwiseXORExpression   { $$ = $1; }
        | BitwiseORExpression '|' BitwiseXORExpression
                                { $$ = new_binary_expression(ctx, EXPR_BOR, $1, $3); }

/* ECMA-262 3rd Edition    11.10 */
BitwiseORExpressionNoIn
        : BitwiseXORExpressionNoIn
                                { $$ = $1; }
        | BitwiseORExpressionNoIn '|' BitwiseXORExpressionNoIn
                                { $$ = new_binary_expression(ctx, EXPR_BOR, $1, $3); }

/* ECMA-262 3rd Edition    11.10 */
BitwiseXORExpression
        : BitwiseANDExpression  { $$ = $1; }
        | BitwiseXORExpression '^' BitwiseANDExpression
                                { $$ = new_binary_expression(ctx, EXPR_BXOR, $1, $3); }

/* ECMA-262 3rd Edition    11.10 */
BitwiseXORExpressionNoIn
        : BitwiseANDExpressionNoIn
                                { $$ = $1; }
        | BitwiseXORExpressionNoIn '^' BitwiseANDExpressionNoIn
                                { $$ = new_binary_expression(ctx, EXPR_BXOR, $1, $3); }

/* ECMA-262 3rd Edition    11.10 */
BitwiseANDExpression
        : EqualityExpression    { $$ = $1; }
        | BitwiseANDExpression '&' EqualityExpression
                                { $$ = new_binary_expression(ctx, EXPR_BAND, $1, $3); }

/* ECMA-262 3rd Edition    11.10 */
BitwiseANDExpressionNoIn
        : EqualityExpressionNoIn
                                { $$ = $1; }
        | BitwiseANDExpressionNoIn '&' EqualityExpressionNoIn
                                { $$ = new_binary_expression(ctx, EXPR_BAND, $1, $3); }

/* ECMA-262 3rd Edition    11.9 */
EqualityExpression
        : RelationalExpression  { $$ = $1; }
        | EqualityExpression tEqOper RelationalExpression
                                { $$ = new_binary_expression(ctx, $2, $1, $3); }

/* ECMA-262 3rd Edition    11.9 */
EqualityExpressionNoIn
        : RelationalExpressionNoIn  { $$ = $1; }
        | EqualityExpressionNoIn tEqOper RelationalExpressionNoIn
                                { $$ = new_binary_expression(ctx, $2, $1, $3); }

/* ECMA-262 3rd Edition    11.8 */
RelationalExpression
        : ShiftExpression       { $$ = $1; }
        | RelationalExpression tRelOper ShiftExpression
                                { $$ = new_binary_expression(ctx, $2, $1, $3); }
        | RelationalExpression kINSTANCEOF ShiftExpression
                                { $$ = new_binary_expression(ctx, EXPR_INSTANCEOF, $1, $3); }
        | RelationalExpression kIN ShiftExpression
                                { $$ = new_binary_expression(ctx, EXPR_IN, $1, $3); }

/* ECMA-262 3rd Edition    11.8 */
RelationalExpressionNoIn
        : ShiftExpression       { $$ = $1; }
        | RelationalExpressionNoIn tRelOper ShiftExpression
                                { $$ = new_binary_expression(ctx, $2, $1, $3); }
        | RelationalExpressionNoIn kINSTANCEOF ShiftExpression
                                { $$ = new_binary_expression(ctx, EXPR_INSTANCEOF, $1, $3); }

/* ECMA-262 3rd Edition    11.7 */
ShiftExpression
        : AdditiveExpression    { $$ = $1; }
        | ShiftExpression tShiftOper AdditiveExpression
                                { $$ = new_binary_expression(ctx, $2, $1, $3); }

/* ECMA-262 3rd Edition    11.6 */
AdditiveExpression
        : MultiplicativeExpression
                                { $$ = $1; }
        | AdditiveExpression '+' MultiplicativeExpression
                                { $$ = new_binary_expression(ctx, EXPR_ADD, $1, $3); }
        | AdditiveExpression '-' MultiplicativeExpression
                                { $$ = new_binary_expression(ctx, EXPR_SUB, $1, $3); }

/* ECMA-262 3rd Edition    11.5 */
MultiplicativeExpression
        : UnaryExpression       { $$ = $1; }
        | MultiplicativeExpression '*' UnaryExpression
                                { $$ = new_binary_expression(ctx, EXPR_MUL, $1, $3); }
        | MultiplicativeExpression '/' UnaryExpression
                                { $$ = new_binary_expression(ctx, EXPR_DIV, $1, $3); }
        | MultiplicativeExpression '%' UnaryExpression
                                { $$ = new_binary_expression(ctx, EXPR_MOD, $1, $3); }

/* ECMA-262 3rd Edition    11.4 */
UnaryExpression
        : PostfixExpression     { $$ = $1; }
        | kDELETE UnaryExpression
                                { $$ = new_unary_expression(ctx, EXPR_DELETE, $2); }
        | kVOID UnaryExpression { $$ = new_unary_expression(ctx, EXPR_VOID, $2); }
        | kTYPEOF UnaryExpression
                                { $$ = new_unary_expression(ctx, EXPR_TYPEOF, $2); }
        | tINC UnaryExpression  { $$ = new_unary_expression(ctx, EXPR_PREINC, $2); }
        | tDEC UnaryExpression  { $$ = new_unary_expression(ctx, EXPR_PREDEC, $2); }
        | '+' UnaryExpression   { $$ = new_unary_expression(ctx, EXPR_PLUS, $2); }
        | '-' UnaryExpression   { $$ = new_unary_expression(ctx, EXPR_MINUS, $2); }
        | '~' UnaryExpression   { $$ = new_unary_expression(ctx, EXPR_BITNEG, $2); }
        | '!' UnaryExpression   { $$ = new_unary_expression(ctx, EXPR_LOGNEG, $2); }

/* ECMA-262 3rd Edition    11.2 */
PostfixExpression
        : LeftHandSideExpression
                                { $$ = $1; }
        | LeftHandSideExpression /* NONL */ tINC
                                { $$ = new_unary_expression(ctx, EXPR_POSTINC, $1); }
        | LeftHandSideExpression /* NONL */ tDEC
                                { $$ = new_unary_expression(ctx, EXPR_POSTDEC, $1); }


/* ECMA-262 3rd Edition    11.2 */
LeftHandSideExpression
        : NewExpression         { $$ = $1; }
        | CallExpression        { $$ = $1; }

/* ECMA-262 3rd Edition    11.2 */
NewExpression
        : MemberExpression      { $$ = $1; }
        | kNEW NewExpression    { $$ = new_new_expression(ctx, $2, NULL); }

/* ECMA-262 3rd Edition    11.2 */
MemberExpression
        : PrimaryExpression     { $$ = $1; }
        | FunctionExpression    { $$ = $1; }
        | MemberExpression '[' Expression ']'
                                { $$ = new_binary_expression(ctx, EXPR_ARRAY, $1, $3); }
        | MemberExpression '.' tIdentifier
                                { $$ = new_member_expression(ctx, $1, $3); }
        | kNEW MemberExpression Arguments
                                { $$ = new_new_expression(ctx, $2, $3); }

/* ECMA-262 3rd Edition    11.2 */
CallExpression
        : MemberExpression Arguments
                                { $$ = new_call_expression(ctx, $1, $2); }
        | CallExpression Arguments
                                { $$ = new_call_expression(ctx, $1, $2); }
        | CallExpression '[' Expression ']'
                                { $$ = new_binary_expression(ctx, EXPR_ARRAY, $1, $3); }
        | CallExpression '.' tIdentifier
                                { $$ = new_member_expression(ctx, $1, $3); }

/* ECMA-262 3rd Edition    11.2 */
Arguments
        : '(' ')'               { $$ = NULL; }
        | '(' ArgumentList ')'  { $$ = $2; }

/* ECMA-262 3rd Edition    11.2 */
ArgumentList
        : AssignmentExpression  { $$ = new_argument_list(ctx, $1); }
        | ArgumentList ',' AssignmentExpression
                                { $$ = argument_list_add(ctx, $1, $3); }

/* ECMA-262 3rd Edition    11.1 */
PrimaryExpression
        : kTHIS                 { $$ = new_expression(ctx, EXPR_THIS, 0); }
        | tIdentifier           { $$ = new_identifier_expression(ctx, $1); }
        | Literal               { $$ = new_literal_expression(ctx, $1); }
        | ArrayLiteral          { $$ = $1; }
        | ObjectLiteral         { $$ = $1; }
        | '(' Expression ')'    { $$ = $2; }

/* ECMA-262 3rd Edition    11.1.4 */
ArrayLiteral
        : '[' ']'               { $$ = new_array_literal_expression(ctx, NULL, 0); }
        | '[' Elision ']'       { $$ = new_array_literal_expression(ctx, NULL, $2+1); }
        | '[' ElementList ']'   { $$ = new_array_literal_expression(ctx, $2, 0); }
        | '[' ElementList ',' Elision_opt ']'
                                { $$ = new_array_literal_expression(ctx, $2, $4+1); }

/* ECMA-262 3rd Edition    11.1.4 */
ElementList
        : Elision_opt AssignmentExpression
                                { $$ = new_element_list(ctx, $1, $2); }
        | ElementList ',' Elision_opt AssignmentExpression
                                { $$ = element_list_add(ctx, $1, $3, $4); }

/* ECMA-262 3rd Edition    11.1.4 */
Elision
        : ','                   { $$ = 1; }
        | Elision ','           { $$ = $1 + 1; }

/* ECMA-262 3rd Edition    11.1.4 */
Elision_opt
        : /* empty */           { $$ = 0; }
        | Elision               { $$ = $1; }

/* ECMA-262 3rd Edition    11.1.5 */
ObjectLiteral
        : '{' '}'               { $$ = new_prop_and_value_expression(ctx, NULL); }
        | '{' PropertyNameAndValueList '}'
                                { $$ = new_prop_and_value_expression(ctx, $2); }

/* ECMA-262 3rd Edition    11.1.5 */
PropertyNameAndValueList
        : PropertyName ':' AssignmentExpression
                                { $$ = new_property_list(ctx, $1, $3); }
        | PropertyNameAndValueList ',' PropertyName ':' AssignmentExpression
                                { $$ = property_list_add(ctx, $1, $3, $5); }

/* ECMA-262 3rd Edition    11.1.5 */
PropertyName
        : tIdentifier           { $$ = new_string_literal(ctx, $1); }
        | tStringLiteral        { $$ = new_string_literal(ctx, $1); }
        | tNumericLiteral       { $$ = $1; }

/* ECMA-262 3rd Edition    7.6 */
Identifier_opt
        : /* empty*/            { $$ = NULL; }
        | tIdentifier           { $$ = $1; }

/* ECMA-262 3rd Edition    7.8 */
Literal
        : kNULL                 { $$ = new_null_literal(ctx); }
        | BooleanLiteral        { $$ = $1; }
        | tNumericLiteral       { $$ = $1; }
        | tStringLiteral        { $$ = new_string_literal(ctx, $1); }
        | '/'                   { $$ = parse_regexp(ctx);
                                  if(!$$) YYABORT; }
        | kDIVEQ                { $$ = parse_regexp(ctx);
                                  if(!$$) YYABORT; }

/* ECMA-262 3rd Edition    7.8.2 */
BooleanLiteral
        : kTRUE                 { $$ = new_boolean_literal(ctx, VARIANT_TRUE); }
        | kFALSE                { $$ = new_boolean_literal(ctx, VARIANT_FALSE); }
        | tBooleanLiteral       { $$ = $1; }

semicolon_opt
        : ';'
        | error                 { if(!allow_auto_semicolon(ctx)) {YYABORT;} }

left_bracket
        : '('
        | error                 { set_error(ctx, JS_E_MISSING_LBRACKET); YYABORT; }

right_bracket
        : ')'
        | error                 { set_error(ctx, JS_E_MISSING_RBRACKET); YYABORT; }

semicolon
        : ';'
        | error                 { set_error(ctx, JS_E_MISSING_SEMICOLON); YYABORT; }

%%

static BOOL allow_auto_semicolon(parser_ctx_t *ctx)
{
    return ctx->nl || ctx->ptr == ctx->end || *(ctx->ptr-1) == '}';
}

static void *new_statement(parser_ctx_t *ctx, statement_type_t type, size_t size)
{
    statement_t *stat;

    stat = parser_alloc(ctx, size ? size : sizeof(*stat));
    if(!stat)
        return NULL;

    stat->type = type;
    stat->next = NULL;

    return stat;
}

static literal_t *new_string_literal(parser_ctx_t *ctx, const WCHAR *str)
{
    literal_t *ret = parser_alloc(ctx, sizeof(literal_t));

    ret->type = LT_STRING;
    ret->u.wstr = str;

    return ret;
}

static literal_t *new_null_literal(parser_ctx_t *ctx)
{
    literal_t *ret = parser_alloc(ctx, sizeof(literal_t));

    ret->type = LT_NULL;

    return ret;
}

static prop_val_t *new_prop_val(parser_ctx_t *ctx, literal_t *name, expression_t *value)
{
    prop_val_t *ret = parser_alloc(ctx, sizeof(prop_val_t));

    ret->name = name;
    ret->value = value;
    ret->next = NULL;

    return ret;
}

static property_list_t *new_property_list(parser_ctx_t *ctx, literal_t *name, expression_t *value)
{
    property_list_t *ret = parser_alloc_tmp(ctx, sizeof(property_list_t));

    ret->head = ret->tail = new_prop_val(ctx, name, value);

    return ret;
}

static property_list_t *property_list_add(parser_ctx_t *ctx, property_list_t *list, literal_t *name, expression_t *value)
{
    list->tail = list->tail->next = new_prop_val(ctx, name, value);

    return list;
}

static array_element_t *new_array_element(parser_ctx_t *ctx, int elision, expression_t *expr)
{
    array_element_t *ret = parser_alloc(ctx, sizeof(array_element_t));

    ret->elision = elision;
    ret->expr = expr;
    ret->next = NULL;

    return ret;
}

static element_list_t *new_element_list(parser_ctx_t *ctx, int elision, expression_t *expr)
{
    element_list_t *ret = parser_alloc_tmp(ctx, sizeof(element_list_t));

    ret->head = ret->tail = new_array_element(ctx, elision, expr);

    return ret;
}

static element_list_t *element_list_add(parser_ctx_t *ctx, element_list_t *list, int elision, expression_t *expr)
{
    list->tail = list->tail->next = new_array_element(ctx, elision, expr);

    return list;
}

static argument_t *new_argument(parser_ctx_t *ctx, expression_t *expr)
{
    argument_t *ret = parser_alloc(ctx, sizeof(argument_t));

    ret->expr = expr;
    ret->next = NULL;

    return ret;
}

static argument_list_t *new_argument_list(parser_ctx_t *ctx, expression_t *expr)
{
    argument_list_t *ret = parser_alloc_tmp(ctx, sizeof(argument_list_t));

    ret->head = ret->tail = new_argument(ctx, expr);

    return ret;
}

static argument_list_t *argument_list_add(parser_ctx_t *ctx, argument_list_t *list, expression_t *expr)
{
    list->tail = list->tail->next = new_argument(ctx, expr);

    return list;
}

static catch_block_t *new_catch_block(parser_ctx_t *ctx, const WCHAR *identifier, statement_t *statement)
{
    catch_block_t *ret = parser_alloc(ctx, sizeof(catch_block_t));

    ret->identifier = identifier;
    ret->statement = statement;

    return ret;
}

static case_clausule_t *new_case_clausule(parser_ctx_t *ctx, expression_t *expr, statement_list_t *stat_list)
{
    case_clausule_t *ret = parser_alloc(ctx, sizeof(case_clausule_t));

    ret->expr = expr;
    ret->stat = stat_list ? stat_list->head : NULL;
    ret->next = NULL;

    return ret;
}

static case_list_t *new_case_list(parser_ctx_t *ctx, case_clausule_t *case_clausule)
{
    case_list_t *ret = parser_alloc_tmp(ctx, sizeof(case_list_t));

    ret->head = ret->tail = case_clausule;

    return ret;
}

static case_list_t *case_list_add(parser_ctx_t *ctx, case_list_t *list, case_clausule_t *case_clausule)
{
    list->tail = list->tail->next = case_clausule;

    return list;
}

static case_clausule_t *new_case_block(parser_ctx_t *ctx, case_list_t *case_list1,
        case_clausule_t *default_clausule, case_list_t *case_list2)
{
    case_clausule_t *ret = NULL, *iter = NULL, *iter2;
    statement_t *stat = NULL;

    if(case_list1) {
        ret = case_list1->head;
        iter = case_list1->tail;
    }

    if(default_clausule) {
        if(ret)
            iter = iter->next = default_clausule;
        else
            ret = iter = default_clausule;
    }

    if(case_list2) {
        if(ret)
            iter->next = case_list2->head;
        else
            ret = case_list2->head;
    }

    if(!ret)
        return NULL;

    for(iter = ret; iter; iter = iter->next) {
        for(iter2 = iter; iter2 && !iter2->stat; iter2 = iter2->next);
        if(!iter2)
            break;

        while(iter != iter2) {
            iter->stat = iter2->stat;
            iter = iter->next;
        }

        if(stat) {
            while(stat->next)
                stat = stat->next;
            stat->next = iter->stat;
        }else {
            stat = iter->stat;
        }
    }

    return ret;
}

static statement_t *new_block_statement(parser_ctx_t *ctx, statement_list_t *list)
{
    block_statement_t *ret;

    ret = new_statement(ctx, STAT_BLOCK, sizeof(*ret));
    if(!ret)
        return NULL;

    ret->stat_list = list ? list->head : NULL;

    return &ret->stat;
}

static variable_declaration_t *new_variable_declaration(parser_ctx_t *ctx, const WCHAR *identifier, expression_t *expr)
{
    variable_declaration_t *ret = parser_alloc(ctx, sizeof(variable_declaration_t));

    ret->identifier = identifier;
    ret->expr = expr;
    ret->next = NULL;
    ret->global_next = NULL;

    return ret;
}

static variable_list_t *new_variable_list(parser_ctx_t *ctx, variable_declaration_t *decl)
{
    variable_list_t *ret = parser_alloc_tmp(ctx, sizeof(variable_list_t));

    ret->head = ret->tail = decl;

    return ret;
}

static variable_list_t *variable_list_add(parser_ctx_t *ctx, variable_list_t *list, variable_declaration_t *decl)
{
    list->tail = list->tail->next = decl;

    return list;
}

static statement_t *new_var_statement(parser_ctx_t *ctx, variable_list_t *variable_list)
{
    var_statement_t *ret;

    ret = new_statement(ctx, STAT_VAR, sizeof(*ret));
    if(!ret)
        return NULL;

    ret->variable_list = variable_list->head;

    return &ret->stat;
}

static statement_t *new_expression_statement(parser_ctx_t *ctx, expression_t *expr)
{
    expression_statement_t *ret;

    ret = new_statement(ctx, STAT_EXPR, sizeof(*ret));
    if(!ret)
        return NULL;

    ret->expr = expr;

    return &ret->stat;
}

static statement_t *new_if_statement(parser_ctx_t *ctx, expression_t *expr, statement_t *if_stat, statement_t *else_stat)
{
    if_statement_t *ret;

    ret = new_statement(ctx, STAT_IF, sizeof(*ret));
    if(!ret)
        return NULL;

    ret->expr = expr;
    ret->if_stat = if_stat;
    ret->else_stat = else_stat;

    return &ret->stat;
}

static statement_t *new_while_statement(parser_ctx_t *ctx, BOOL dowhile, expression_t *expr, statement_t *stat)
{
    while_statement_t *ret;

    ret = new_statement(ctx, STAT_WHILE, sizeof(*ret));
    if(!ret)
        return NULL;

    ret->do_while = dowhile;
    ret->expr = expr;
    ret->statement = stat;

    return &ret->stat;
}

static statement_t *new_for_statement(parser_ctx_t *ctx, variable_list_t *variable_list, expression_t *begin_expr,
        expression_t *expr, expression_t *end_expr, statement_t *statement)
{
    for_statement_t *ret;

    ret = new_statement(ctx, STAT_FOR, sizeof(*ret));
    if(!ret)
        return NULL;

    ret->variable_list = variable_list ? variable_list->head : NULL;
    ret->begin_expr = begin_expr;
    ret->expr = expr;
    ret->end_expr = end_expr;
    ret->statement = statement;

    return &ret->stat;
}

static statement_t *new_forin_statement(parser_ctx_t *ctx, variable_declaration_t *variable, expression_t *expr,
        expression_t *in_expr, statement_t *statement)
{
    forin_statement_t *ret;

    ret = new_statement(ctx, STAT_FORIN, sizeof(*ret));
    if(!ret)
        return NULL;

    ret->variable = variable;
    ret->expr = expr;
    ret->in_expr = in_expr;
    ret->statement = statement;

    return &ret->stat;
}

static statement_t *new_continue_statement(parser_ctx_t *ctx, const WCHAR *identifier)
{
    branch_statement_t *ret;

    ret = new_statement(ctx, STAT_CONTINUE, sizeof(*ret));
    if(!ret)
        return NULL;

    ret->identifier = identifier;

    return &ret->stat;
}

static statement_t *new_break_statement(parser_ctx_t *ctx, const WCHAR *identifier)
{
    branch_statement_t *ret;

    ret = new_statement(ctx, STAT_BREAK, sizeof(*ret));
    if(!ret)
        return NULL;

    ret->identifier = identifier;

    return &ret->stat;
}

static statement_t *new_return_statement(parser_ctx_t *ctx, expression_t *expr)
{
    expression_statement_t *ret;

    ret = new_statement(ctx, STAT_RETURN, sizeof(*ret));
    if(!ret)
        return NULL;

    ret->expr = expr;

    return &ret->stat;
}

static statement_t *new_with_statement(parser_ctx_t *ctx, expression_t *expr, statement_t *statement)
{
    with_statement_t *ret;

    ret = new_statement(ctx, STAT_WITH, sizeof(*ret));
    if(!ret)
        return NULL;

    ret->expr = expr;
    ret->statement = statement;

    return &ret->stat;
}

static statement_t *new_labelled_statement(parser_ctx_t *ctx, const WCHAR *identifier, statement_t *statement)
{
    labelled_statement_t *ret;

    ret = new_statement(ctx, STAT_LABEL, sizeof(*ret));
    if(!ret)
        return NULL;

    ret->identifier = identifier;
    ret->statement = statement;

    return &ret->stat;
}

static statement_t *new_switch_statement(parser_ctx_t *ctx, expression_t *expr, case_clausule_t *case_list)
{
    switch_statement_t *ret;

    ret = new_statement(ctx, STAT_SWITCH, sizeof(*ret));
    if(!ret)
        return NULL;

    ret->expr = expr;
    ret->case_list = case_list;

    return &ret->stat;
}

static statement_t *new_throw_statement(parser_ctx_t *ctx, expression_t *expr)
{
    expression_statement_t *ret;

    ret = new_statement(ctx, STAT_THROW, sizeof(*ret));
    if(!ret)
        return NULL;

    ret->expr = expr;

    return &ret->stat;
}

static statement_t *new_try_statement(parser_ctx_t *ctx, statement_t *try_statement,
       catch_block_t *catch_block, statement_t *finally_statement)
{
    try_statement_t *ret;

    ret = new_statement(ctx, STAT_TRY, sizeof(*ret));
    if(!ret)
        return NULL;

    ret->try_statement = try_statement;
    ret->catch_block = catch_block;
    ret->finally_statement = finally_statement;

    return &ret->stat;
}

static parameter_t *new_parameter(parser_ctx_t *ctx, const WCHAR *identifier)
{
    parameter_t *ret = parser_alloc(ctx, sizeof(parameter_t));

    ret->identifier = identifier;
    ret->next = NULL;

    return ret;
}

static parameter_list_t *new_parameter_list(parser_ctx_t *ctx, const WCHAR *identifier)
{
    parameter_list_t *ret = parser_alloc_tmp(ctx, sizeof(parameter_list_t));

    ret->head = ret->tail = new_parameter(ctx, identifier);

    return ret;
}

static parameter_list_t *parameter_list_add(parser_ctx_t *ctx, parameter_list_t *list, const WCHAR *identifier)
{
    list->tail = list->tail->next = new_parameter(ctx, identifier);

    return list;
}

static expression_t *new_function_expression(parser_ctx_t *ctx, const WCHAR *identifier,
       parameter_list_t *parameter_list, source_elements_t *source_elements, const WCHAR *src_str, DWORD src_len)
{
    function_expression_t *ret = new_expression(ctx, EXPR_FUNC, sizeof(*ret));

    ret->identifier = identifier;
    ret->parameter_list = parameter_list ? parameter_list->head : NULL;
    ret->source_elements = source_elements;
    ret->src_str = src_str;
    ret->src_len = src_len;
    ret->next = NULL;

    return &ret->expr;
}

static void *new_expression(parser_ctx_t *ctx, expression_type_t type, size_t size)
{
    expression_t *ret = parser_alloc(ctx, size ? size : sizeof(*ret));

    ret->type = type;

    return ret;
}

static expression_t *new_binary_expression(parser_ctx_t *ctx, expression_type_t type,
       expression_t *expression1, expression_t *expression2)
{
    binary_expression_t *ret = new_expression(ctx, type, sizeof(*ret));

    ret->expression1 = expression1;
    ret->expression2 = expression2;

    return &ret->expr;
}

static expression_t *new_unary_expression(parser_ctx_t *ctx, expression_type_t type, expression_t *expression)
{
    unary_expression_t *ret = new_expression(ctx, type, sizeof(*ret));

    ret->expression = expression;

    return &ret->expr;
}

static expression_t *new_conditional_expression(parser_ctx_t *ctx, expression_t *expression,
       expression_t *true_expression, expression_t *false_expression)
{
    conditional_expression_t *ret = new_expression(ctx, EXPR_COND, sizeof(*ret));

    ret->expression = expression;
    ret->true_expression = true_expression;
    ret->false_expression = false_expression;

    return &ret->expr;
}

static expression_t *new_member_expression(parser_ctx_t *ctx, expression_t *expression, const WCHAR *identifier)
{
    member_expression_t *ret = new_expression(ctx, EXPR_MEMBER, sizeof(*ret));

    ret->expression = expression;
    ret->identifier = identifier;

    return &ret->expr;
}

static expression_t *new_new_expression(parser_ctx_t *ctx, expression_t *expression, argument_list_t *argument_list)
{
    call_expression_t *ret = new_expression(ctx, EXPR_NEW, sizeof(*ret));

    ret->expression = expression;
    ret->argument_list = argument_list ? argument_list->head : NULL;

    return &ret->expr;
}

static expression_t *new_call_expression(parser_ctx_t *ctx, expression_t *expression, argument_list_t *argument_list)
{
    call_expression_t *ret = new_expression(ctx, EXPR_CALL, sizeof(*ret));

    ret->expression = expression;
    ret->argument_list = argument_list ? argument_list->head : NULL;

    return &ret->expr;
}

static int parser_error(parser_ctx_t *ctx, const char *str)
{
    return 0;
}

static void set_error(parser_ctx_t *ctx, UINT error)
{
    ctx->hres = error;
}

static BOOL explicit_error(parser_ctx_t *ctx, void *obj, WCHAR next)
{
    if(obj || *(ctx->ptr-1)==next) return TRUE;

    set_error(ctx, JS_E_SYNTAX);
    return FALSE;
}


static expression_t *new_identifier_expression(parser_ctx_t *ctx, const WCHAR *identifier)
{
    identifier_expression_t *ret = new_expression(ctx, EXPR_IDENT, sizeof(*ret));

    ret->identifier = identifier;

    return &ret->expr;
}

static expression_t *new_array_literal_expression(parser_ctx_t *ctx, element_list_t *element_list, int length)
{
    array_literal_expression_t *ret = new_expression(ctx, EXPR_ARRAYLIT, sizeof(*ret));

    ret->element_list = element_list ? element_list->head : NULL;
    ret->length = length;

    return &ret->expr;
}

static expression_t *new_prop_and_value_expression(parser_ctx_t *ctx, property_list_t *property_list)
{
    property_value_expression_t *ret = new_expression(ctx, EXPR_PROPVAL, sizeof(*ret));

    ret->property_list = property_list ? property_list->head : NULL;

    return &ret->expr;
}

static expression_t *new_literal_expression(parser_ctx_t *ctx, literal_t *literal)
{
    literal_expression_t *ret = new_expression(ctx, EXPR_LITERAL, sizeof(*ret));

    ret->literal = literal;

    return &ret->expr;
}

static source_elements_t *new_source_elements(parser_ctx_t *ctx)
{
    source_elements_t *ret = parser_alloc(ctx, sizeof(source_elements_t));

    memset(ret, 0, sizeof(*ret));

    return ret;
}

static source_elements_t *source_elements_add_statement(source_elements_t *source_elements, statement_t *statement)
{
    if(source_elements->statement_tail)
        source_elements->statement_tail = source_elements->statement_tail->next = statement;
    else
        source_elements->statement = source_elements->statement_tail = statement;

    return source_elements;
}

static statement_list_t *new_statement_list(parser_ctx_t *ctx, statement_t *statement)
{
    statement_list_t *ret =  parser_alloc_tmp(ctx, sizeof(statement_list_t));

    ret->head = ret->tail = statement;

    return ret;
}

static statement_list_t *statement_list_add(statement_list_t *list, statement_t *statement)
{
    list->tail = list->tail->next = statement;

    return list;
}

static void program_parsed(parser_ctx_t *ctx, source_elements_t *source)
{
    ctx->source = source;
    if(!ctx->lexer_error)
        ctx->hres = S_OK;
}

void parser_release(parser_ctx_t *ctx)
{
    script_release(ctx->script);
    heap_pool_free(&ctx->heap);
    heap_free(ctx);
}

HRESULT script_parse(script_ctx_t *ctx, const WCHAR *code, const WCHAR *delimiter, BOOL from_eval,
        parser_ctx_t **ret)
{
    parser_ctx_t *parser_ctx;
    heap_pool_t *mark;
    HRESULT hres;

    const WCHAR html_tagW[] = {'<','/','s','c','r','i','p','t','>',0};

    parser_ctx = heap_alloc_zero(sizeof(parser_ctx_t));
    if(!parser_ctx)
        return E_OUTOFMEMORY;

    parser_ctx->hres = JS_E_SYNTAX;
    parser_ctx->is_html = delimiter && !strcmpiW(delimiter, html_tagW);

    parser_ctx->begin = parser_ctx->ptr = code;
    parser_ctx->end = parser_ctx->begin + strlenW(parser_ctx->begin);

    script_addref(ctx);
    parser_ctx->script = ctx;

    mark = heap_pool_mark(&ctx->tmp_heap);
    heap_pool_init(&parser_ctx->heap);

    parser_parse(parser_ctx);
    heap_pool_clear(mark);
    hres = parser_ctx->hres;
    if(FAILED(hres)) {
        WARN("parser failed around %s\n",
            debugstr_w(parser_ctx->begin+20 > parser_ctx->ptr ? parser_ctx->begin : parser_ctx->ptr-20));
        parser_release(parser_ctx);
        return hres;
    }

    *ret = parser_ctx;
    return S_OK;
}
