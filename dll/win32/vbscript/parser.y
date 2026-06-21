/*
 * Copyright 2011 Jacek Caban for CodeWeavers
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

#include "vbscript.h"
#include "parse.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(vbscript);

static int parser_error(unsigned*,parser_ctx_t*,const char*);

static void handle_isexpression_script(parser_ctx_t *ctx, expression_t *expr);

static void source_add_statement(parser_ctx_t*,statement_t*);
static void source_add_class(parser_ctx_t*,class_decl_t*);

static void *new_expression(parser_ctx_t*,expression_type_t,size_t);
static expression_t *new_bool_expression(parser_ctx_t*,VARIANT_BOOL);
static expression_t *new_date_expression(parser_ctx_t*,DATE);
static expression_t *new_string_expression(parser_ctx_t*,const WCHAR*);
static expression_t *new_long_expression(parser_ctx_t*,expression_type_t,LONG);
static expression_t *new_double_expression(parser_ctx_t*,double);
static expression_t *new_unary_expression(parser_ctx_t*,expression_type_t,expression_t*);
static expression_t *new_binary_expression(parser_ctx_t*,expression_type_t,expression_t*,expression_t*);
static expression_t *new_new_expression(parser_ctx_t*,const WCHAR*);

static member_expression_t *new_member_expression(parser_ctx_t*,expression_t*,const WCHAR*);
static call_expression_t *new_call_expression(parser_ctx_t*,expression_t*,expression_t*);
static call_expression_t *make_call_expression(parser_ctx_t*,expression_t*,expression_t*);

static void *new_statement(parser_ctx_t*,statement_type_t,size_t,unsigned);
static statement_t *new_call_statement(parser_ctx_t*,unsigned,expression_t*);
static statement_t *new_assign_statement(parser_ctx_t*,unsigned,expression_t*,expression_t*);
static statement_t *new_set_statement(parser_ctx_t*,unsigned,expression_t*,expression_t*);
static statement_t *new_dim_statement(parser_ctx_t*,unsigned,dim_decl_t*);
static statement_t *new_redim_statement(parser_ctx_t*,unsigned,BOOL,redim_decl_t*);
static statement_t *new_while_statement(parser_ctx_t*,unsigned,statement_type_t,expression_t*,statement_t*);
static statement_t *new_forto_statement(parser_ctx_t*,unsigned,const WCHAR*,expression_t*,expression_t*,expression_t*,statement_t*);
static statement_t *new_foreach_statement(parser_ctx_t*,unsigned,const WCHAR*,expression_t*,statement_t*);
static statement_t *new_if_statement(parser_ctx_t*,unsigned,expression_t*,statement_t*,elseif_decl_t*,statement_t*);
static statement_t *new_function_statement(parser_ctx_t*,unsigned,function_decl_t*);
static statement_t *new_onerror_statement(parser_ctx_t*,unsigned,BOOL);
static statement_t *new_const_statement(parser_ctx_t*,unsigned,const_decl_t*);
static statement_t *new_select_statement(parser_ctx_t*,unsigned,expression_t*,case_clausule_t*);
static statement_t *new_with_statement(parser_ctx_t*,unsigned,expression_t*,statement_t*);

static dim_decl_t *new_dim_decl(parser_ctx_t*,const WCHAR*,BOOL,dim_list_t*);
static dim_list_t *new_dim(parser_ctx_t*,unsigned,dim_list_t*);
static redim_decl_t *new_redim_decl(parser_ctx_t*,const WCHAR*,expression_t*);
static elseif_decl_t *new_elseif_decl(parser_ctx_t*,unsigned,expression_t*,statement_t*);
static function_decl_t *new_function_decl(parser_ctx_t*,const WCHAR*,function_type_t,unsigned,arg_decl_t*,statement_t*);
static arg_decl_t *new_argument_decl(parser_ctx_t*,const WCHAR*,BOOL);
static const_decl_t *new_const_decl(parser_ctx_t*,const WCHAR*,expression_t*);
static case_clausule_t *new_case_clausule(parser_ctx_t*,expression_t*,statement_t*,case_clausule_t*);

static class_decl_t *new_class_decl(parser_ctx_t*);
static class_decl_t *add_class_function(parser_ctx_t*,class_decl_t*,function_decl_t*);
static class_decl_t *add_dim_prop(parser_ctx_t*,class_decl_t*,dim_decl_t*,unsigned);

static statement_t *link_statements(statement_t*,statement_t*);

#define STORAGE_IS_PRIVATE    1
#define STORAGE_IS_DEFAULT    2

#define CHECK_ERROR if(((parser_ctx_t*)ctx)->hres != S_OK) YYABORT

#define PARSER_LTYPE unsigned
#define YYLLOC_DEFAULT(Cur, Rhs, N) Cur = YYRHSLOC((Rhs), (N) ? 1 : 0)

%}

%lex-param { parser_ctx_t *ctx }
%parse-param { parser_ctx_t *ctx }
%define api.prefix {parser_}
%define api.pure
%start Program

%union {
    const WCHAR *string;
    statement_t *statement;
    expression_t *expression;
    member_expression_t *member;
    elseif_decl_t *elseif;
    dim_decl_t *dim_decl;
    dim_list_t *dim_list;
    redim_decl_t *redim_decl;
    function_decl_t *func_decl;
    arg_decl_t *arg_decl;
    class_decl_t *class_decl;
    const_decl_t *const_decl;
    case_clausule_t *case_clausule;
    unsigned uint;
    LONG integer;
    BOOL boolean;
    double dbl;
    DATE date;
}

%token tEXPRESSION tNL tEMPTYBRACKETS tEXPRLBRACKET
%token tLTEQ tGTEQ tNEQ
%token tSTOP tME tREM tDOT
%token <string> tTRUE tFALSE
%token <string> tNOT tAND tOR tXOR tEQV tIMP
%token <string> tIS tMOD
%token <string> tCALL tSUB tFUNCTION tGET tLET tCONST
%token <string> tDIM tREDIM tPRESERVE
%token <string> tIF tELSE tELSEIF tEND tTHEN tEXIT
%token <string> tWHILE tWEND tDO tLOOP tUNTIL tFOR tTO tEACH tIN
%token <string> tSELECT tCASE tWITH
%token <string> tBYREF tBYVAL
%token <string> tOPTION
%token <string> tNOTHING tEMPTY tNULL
%token <string> tCLASS tSET tNEW tPUBLIC tPRIVATE
%token <string> tNEXT tON tRESUME tGOTO
%token <string> tIdentifier tString
%token <string> tDEFAULT tERROR tEXPLICIT tPROPERTY tSTEP
%token <integer> tInt
%token <dbl> tDouble
%token <date> tDate

%type <statement> Statement SimpleStatement StatementNl StatementsNl StatementsNl_opt BodyStatements IfStatement Else_opt
%type <statement> GlobalDimDeclaration
%type <expression> Expression LiteralExpression PrimaryExpression EqualityExpression CallExpression ExpressionNl_opt
%type <expression> ConcatExpression AdditiveExpression ModExpression IntdivExpression MultiplicativeExpression ExpExpression
%type <expression> NotExpression UnaryExpression AndExpression OrExpression XorExpression EqvExpression SignExpression
%type <expression> ConstExpression NumericLiteralExpression
%type <member> MemberExpression
%type <expression> Arguments ArgumentList ArgumentList_opt Step_opt ExpressionList
%type <boolean> DoType Preserve_opt
%type <arg_decl> ArgumentsDecl ArgumentsDecl_opt ArgumentDeclList ArgumentDecl
%type <func_decl> FunctionDecl PropertyDecl
%type <elseif> ElseIfs_opt ElseIfs ElseIf
%type <class_decl> ClassDeclaration ClassBody
%type <uint> Storage Storage_opt IntegerValue
%type <dim_decl> DimDeclList DimDecl
%type <dim_list> DimList
%type <redim_decl> ReDimDeclList ReDimDecl
%type <const_decl> ConstDecl ConstDeclList
%type <string> Identifier
%type <case_clausule> CaseClausules

%%

Program
    : OptionExplicit_opt SourceElements
    | tEXPRESSION ExpressionNl_opt          { handle_isexpression_script(ctx, $2); }

OptionExplicit_opt
    : /* empty */
    | tOPTION tEXPLICIT StSep               { ctx->option_explicit = TRUE; }

SourceElements
    : /* empty */
    | SourceElements GlobalDimDeclaration StSep
                                            { source_add_statement(ctx, $2); }
    | SourceElements StatementNl            { source_add_statement(ctx, $2); }
    | SourceElements ClassDeclaration       { source_add_class(ctx, $2); }

GlobalDimDeclaration
    : tPRIVATE tCONST ConstDeclList         { $$ = new_const_statement(ctx, @$, $3); CHECK_ERROR; }
    | tPUBLIC  tCONST ConstDeclList         { $$ = new_const_statement(ctx, @$, $3); CHECK_ERROR; }
    | tPRIVATE DimDeclList                  { $$ = new_dim_statement(ctx, @$, $2); CHECK_ERROR; }
    | tPUBLIC  DimDeclList                  { $$ = new_dim_statement(ctx, @$, $2);
                                              (void)parser_nerrs; /* avoid unused variable warning */
                                              CHECK_ERROR; }

ExpressionNl_opt
    : /* empty */                           { $$ = NULL; }
    | Expression tNL                        { $$ = $1; }

BodyStatements
    : /* empty */                           { $$ = NULL; }
    | Statement                             { $$ = $1; }
    | StatementNl BodyStatements            { $$ = link_statements($1, $2); }

StatementsNl_opt
    : /* empty */                           { $$ = NULL; }
    | StatementsNl                          { $$ = $1; }

StatementsNl
    : SimpleStatement StSep                 { $$ = $1; }
    | SimpleStatement StSep StatementsNl    { $$ = link_statements($1, $3); }

StatementNl
    : Statement tNL                         { $$ = $1; }

Statement
    : ':'                                   { $$ = NULL; }
    | ':' Statement                         { $$ = $2; }
    | SimpleStatement                       { $$ = $1; }
    | SimpleStatement ':' Statement         { $1->next = $3; $$ = $1; }
    | SimpleStatement ':'                   { $$ = $1; }

SimpleStatement
    : CallExpression ArgumentList_opt       { call_expression_t *call_expr = make_call_expression(ctx, $1, $2); CHECK_ERROR;
                                              $$ = new_call_statement(ctx, @$, &call_expr->expr); CHECK_ERROR; }
    | tCALL UnaryExpression                 { $$ = new_call_statement(ctx, @$, $2); CHECK_ERROR; }
    | CallExpression '=' Expression
                                            { $$ = new_assign_statement(ctx, @$, $1, $3); CHECK_ERROR; }
    | tDIM DimDeclList                      { $$ = new_dim_statement(ctx, @$, $2); CHECK_ERROR; }
    | tREDIM Preserve_opt ReDimDeclList     { $$ = new_redim_statement(ctx, @$, $2, $3); CHECK_ERROR; }
    | IfStatement                           { $$ = $1; }
    | tWHILE Expression StSep StatementsNl_opt tWEND
                                            { $$ = new_while_statement(ctx, @$, STAT_WHILE, $2, $4); CHECK_ERROR; }
    | tDO DoType Expression StSep StatementsNl_opt tLOOP
                                            { $$ = new_while_statement(ctx, @$, $2 ? STAT_WHILELOOP : STAT_UNTIL, $3, $5);
                                              CHECK_ERROR; }
    | tDO StSep StatementsNl_opt tLOOP DoType Expression
                                            { $$ = new_while_statement(ctx, @4, $5 ? STAT_DOWHILE : STAT_DOUNTIL, $6, $3);
                                              CHECK_ERROR; }
    | tDO StSep StatementsNl_opt tLOOP      { $$ = new_while_statement(ctx, @$, STAT_DOWHILE, NULL, $3); CHECK_ERROR; }
    | FunctionDecl                          { $$ = new_function_statement(ctx, @$, $1); CHECK_ERROR; }
    | tEXIT tDO                             { $$ = new_statement(ctx, STAT_EXITDO, 0, @$); CHECK_ERROR; }
    | tEXIT tFOR                            { $$ = new_statement(ctx, STAT_EXITFOR, 0, @$); CHECK_ERROR; }
    | tEXIT tFUNCTION                       { $$ = new_statement(ctx, STAT_EXITFUNC, 0, @$); CHECK_ERROR; }
    | tEXIT tPROPERTY                       { $$ = new_statement(ctx, STAT_EXITPROP, 0, @$); CHECK_ERROR; }
    | tEXIT tSUB                            { $$ = new_statement(ctx, STAT_EXITSUB, 0, @$); CHECK_ERROR; }
    | tSET CallExpression '=' Expression    { $$ = new_set_statement(ctx, @$, $2, $4); CHECK_ERROR; }
    | tSTOP                                 { $$ = new_statement(ctx, STAT_STOP, 0, @$); CHECK_ERROR; }
    | tON tERROR tRESUME tNEXT              { $$ = new_onerror_statement(ctx, @$, TRUE); CHECK_ERROR; }
    | tON tERROR tGOTO '0'                  { $$ = new_onerror_statement(ctx, @$, FALSE); CHECK_ERROR; }
    | tCONST ConstDeclList                  { $$ = new_const_statement(ctx, @$, $2); CHECK_ERROR; }
    | tFOR Identifier '=' Expression tTO Expression Step_opt StSep StatementsNl_opt tNEXT
                                            { $$ = new_forto_statement(ctx, @$, $2, $4, $6, $7, $9); CHECK_ERROR; }
    | tFOR tEACH Identifier tIN Expression StSep StatementsNl_opt tNEXT
                                            { $$ = new_foreach_statement(ctx, @$, $3, $5, $7); }
    | tSELECT tCASE Expression StSep CaseClausules tEND tSELECT
                                            { $$ = new_select_statement(ctx, @$, $3, $5); }
    | tWITH Expression StSep StatementsNl_opt tEND tWITH
                                            { $$ = new_with_statement(ctx, @$, $2, $4); }

MemberExpression
    : Identifier                            { $$ = new_member_expression(ctx, NULL, $1); CHECK_ERROR; }
    | CallExpression '.' tIdentifier        { $$ = new_member_expression(ctx, $1, $3); CHECK_ERROR; }
    | tDOT tIdentifier                      { expression_t *dot_expr = new_expression(ctx, EXPR_DOT, sizeof(*dot_expr)); CHECK_ERROR;
                                              $$ = new_member_expression(ctx, dot_expr, $2); CHECK_ERROR; }

Preserve_opt
    : /* empty */                           { $$ = FALSE; }
    | tPRESERVE                             { $$ = TRUE; }

ReDimDecl
    : tIdentifier '(' ArgumentList ')'      { $$ = new_redim_decl(ctx, $1, $3); CHECK_ERROR; }

ReDimDeclList
    : ReDimDecl                             { $$ = $1; }
    | ReDimDecl ',' ReDimDeclList           { $1->next = $3; $$ = $1; }

DimDeclList
    : DimDecl                               { $$ = $1; }
    | DimDecl ',' DimDeclList               { $1->next = $3; $$ = $1; }

DimDecl
    : Identifier                            { $$ = new_dim_decl(ctx, $1, FALSE, NULL); CHECK_ERROR; }
    | Identifier '(' DimList ')'            { $$ = new_dim_decl(ctx, $1, TRUE, $3); CHECK_ERROR; }
    | Identifier tEMPTYBRACKETS             { $$ = new_dim_decl(ctx, $1, TRUE, NULL); CHECK_ERROR; }

DimList
    : IntegerValue                          { $$ = new_dim(ctx, $1, NULL); }
    | IntegerValue ',' DimList              { $$ = new_dim(ctx, $1, $3); }

ConstDeclList
    : ConstDecl                             { $$ = $1; }
    | ConstDecl ',' ConstDeclList           { $1->next = $3; $$ = $1; }

ConstDecl
    : Identifier '=' ConstExpression        { $$ = new_const_decl(ctx, $1, $3); CHECK_ERROR; }

ConstExpression
    : LiteralExpression                     { $$ = $1; }
    | '-' NumericLiteralExpression          { $$ = new_unary_expression(ctx, EXPR_NEG, $2); CHECK_ERROR; }

DoType
    : tWHILE        { $$ = TRUE; }
    | tUNTIL        { $$ = FALSE; }

Step_opt
    : /* empty */                           { $$ = NULL;}
    | tSTEP Expression                      { $$ = $2; }

IfStatement
    : tIF Expression tTHEN tNL StatementsNl_opt ElseIfs_opt Else_opt tEND tIF
                                               { $$ = new_if_statement(ctx, @$, $2, $5, $6, $7); CHECK_ERROR; }
    | tIF Expression tTHEN Statement EndIf_opt { $$ = new_if_statement(ctx, @$, $2, $4, NULL, NULL); CHECK_ERROR; }
    | tIF Expression tTHEN Statement tELSE Statement EndIf_opt
                                               { $$ = new_if_statement(ctx, @$, $2, $4, NULL, $6); CHECK_ERROR; }

EndIf_opt
    : /* empty */
    | tEND tIF

ElseIfs_opt
    : /* empty */                           { $$ = NULL; }
    | ElseIfs                               { $$ = $1; }

ElseIfs
    : ElseIf                                { $$ = $1; }
    | ElseIf ElseIfs                        { $1->next = $2; $$ = $1; }

ElseIf
    : tELSEIF Expression tTHEN StSep_opt StatementsNl_opt
                                            { $$ = new_elseif_decl(ctx, @$, $2, $5); }

Else_opt
    : /* empty */                           { $$ = NULL; }
    | tELSE StSep_opt StatementsNl_opt      { $$ = $3; }

CaseClausules
    : /* empty */                                                      { $$ = NULL; }
    | tCASE tELSE StSep_opt StatementsNl_opt                           { $$ = new_case_clausule(ctx, NULL, $4, NULL); }
    | tCASE ExpressionList StSep_opt StatementsNl_opt CaseClausules    { $$ = new_case_clausule(ctx, $2, $4, $5); }

Arguments
    : tEMPTYBRACKETS                { $$ = NULL; }
    | '(' ArgumentList ')'          { $$ = $2; }

ArgumentList_opt
    : /* empty */                   { $$ = NULL; }
    | ArgumentList                  { $$ = $1; }

ArgumentList
    : Expression                    { $$ = $1; }
    | Expression ',' ArgumentList   { $1->next = $3; $$ = $1; }
    | ',' ArgumentList              { $$ = new_expression(ctx, EXPR_NOARG, 0); CHECK_ERROR; $$->next = $2; }

EmptyBrackets_opt
    : /* empty */
    | tEMPTYBRACKETS

ExpressionList
    : Expression                    { $$ = $1; }
    | Expression ',' ExpressionList { $1->next = $3; $$ = $1; }

Expression
    : EqvExpression                             { $$ = $1; }
    | Expression tIMP EqvExpression             { $$ = new_binary_expression(ctx, EXPR_IMP, $1, $3); CHECK_ERROR; }

EqvExpression
    : XorExpression                             { $$ = $1; }
    | EqvExpression tEQV XorExpression          { $$ = new_binary_expression(ctx, EXPR_EQV, $1, $3); CHECK_ERROR; }

XorExpression
    : OrExpression                              { $$ = $1; }
    | XorExpression tXOR OrExpression           { $$ = new_binary_expression(ctx, EXPR_XOR, $1, $3); CHECK_ERROR; }

OrExpression
    : AndExpression                             { $$ = $1; }
    | OrExpression tOR AndExpression            { $$ = new_binary_expression(ctx, EXPR_OR, $1, $3); CHECK_ERROR; }

AndExpression
    : NotExpression                             { $$ = $1; }
    | AndExpression tAND NotExpression          { $$ = new_binary_expression(ctx, EXPR_AND, $1, $3); CHECK_ERROR; }

NotExpression
    : EqualityExpression            { $$ = $1; }
    | tNOT NotExpression            { $$ = new_unary_expression(ctx, EXPR_NOT, $2); CHECK_ERROR; }

EqualityExpression
    : ConcatExpression                          { $$ = $1; }
    | EqualityExpression '=' ConcatExpression   { $$ = new_binary_expression(ctx, EXPR_EQUAL, $1, $3); CHECK_ERROR; }
    | EqualityExpression tNEQ ConcatExpression  { $$ = new_binary_expression(ctx, EXPR_NEQUAL, $1, $3); CHECK_ERROR; }
    | EqualityExpression '>' ConcatExpression   { $$ = new_binary_expression(ctx, EXPR_GT, $1, $3); CHECK_ERROR; }
    | EqualityExpression '<' ConcatExpression   { $$ = new_binary_expression(ctx, EXPR_LT, $1, $3); CHECK_ERROR; }
    | EqualityExpression tGTEQ ConcatExpression { $$ = new_binary_expression(ctx, EXPR_GTEQ, $1, $3); CHECK_ERROR; }
    | EqualityExpression tLTEQ ConcatExpression { $$ = new_binary_expression(ctx, EXPR_LTEQ, $1, $3); CHECK_ERROR; }
    | EqualityExpression tIS ConcatExpression   { $$ = new_binary_expression(ctx, EXPR_IS, $1, $3); CHECK_ERROR; }

ConcatExpression
    : AdditiveExpression                        { $$ = $1; }
    | ConcatExpression '&' AdditiveExpression   { $$ = new_binary_expression(ctx, EXPR_CONCAT, $1, $3); CHECK_ERROR; }

AdditiveExpression
    : ModExpression                             { $$ = $1; }
    | AdditiveExpression '+' ModExpression      { $$ = new_binary_expression(ctx, EXPR_ADD, $1, $3); CHECK_ERROR; }
    | AdditiveExpression '-' ModExpression      { $$ = new_binary_expression(ctx, EXPR_SUB, $1, $3); CHECK_ERROR; }

ModExpression
    : IntdivExpression                          { $$ = $1; }
    | ModExpression tMOD IntdivExpression       { $$ = new_binary_expression(ctx, EXPR_MOD, $1, $3); CHECK_ERROR; }

IntdivExpression
    : MultiplicativeExpression                  { $$ = $1; }
    | IntdivExpression '\\' MultiplicativeExpression
                                                { $$ = new_binary_expression(ctx, EXPR_IDIV, $1, $3); CHECK_ERROR; }

MultiplicativeExpression
    : ExpExpression                             { $$ = $1; }
    | MultiplicativeExpression '*' ExpExpression
                                                { $$ = new_binary_expression(ctx, EXPR_MUL, $1, $3); CHECK_ERROR; }
    | MultiplicativeExpression '/' ExpExpression
                                                { $$ = new_binary_expression(ctx, EXPR_DIV, $1, $3); CHECK_ERROR; }

ExpExpression
    : SignExpression                            { $$ = $1; }
    | ExpExpression '^' SignExpression          { $$ = new_binary_expression(ctx, EXPR_EXP, $1, $3); CHECK_ERROR; }

SignExpression
    : UnaryExpression               { $$ = $1; }
    | '-' SignExpression            { $$ = new_unary_expression(ctx, EXPR_NEG, $2); CHECK_ERROR; }
    | '+' SignExpression            { $$ = $2; }

UnaryExpression
    : LiteralExpression             { $$ = $1; }
    | CallExpression                { $$ = $1; }
    | tNEW Identifier               { $$ = new_new_expression(ctx, $2); CHECK_ERROR; }

CallExpression
    : PrimaryExpression             { $$ = $1; }
    | MemberExpression              { $$ = &$1->expr; }
    | CallExpression Arguments      { call_expression_t *expr = new_call_expression(ctx, $1, $2); CHECK_ERROR;
                                      $$ = &expr->expr; }

LiteralExpression
    : tTRUE                         { $$ = new_bool_expression(ctx, VARIANT_TRUE); CHECK_ERROR; }
    | tFALSE                        { $$ = new_bool_expression(ctx, VARIANT_FALSE); CHECK_ERROR; }
    | tString                       { $$ = new_string_expression(ctx, $1); CHECK_ERROR; }
    | tDate                         { $$ = new_date_expression(ctx, $1); CHECK_ERROR; }
    | NumericLiteralExpression      { $$ = $1; }
    | tEMPTY                        { $$ = new_expression(ctx, EXPR_EMPTY, 0); CHECK_ERROR; }
    | tNULL                         { $$ = new_expression(ctx, EXPR_NULL, 0); CHECK_ERROR; }
    | tNOTHING                      { $$ = new_expression(ctx, EXPR_NOTHING, 0); CHECK_ERROR; }

NumericLiteralExpression
    : '0'                           { $$ = new_long_expression(ctx, EXPR_INT, 0); CHECK_ERROR; }
    | tInt                          { $$ = new_long_expression(ctx, EXPR_INT, $1); CHECK_ERROR; }
    | tDouble                       { $$ = new_double_expression(ctx, $1); CHECK_ERROR; }

IntegerValue
    : '0'                           { $$ = 0; }
    | tInt                          { $$ = $1; }

PrimaryExpression
    : tEXPRLBRACKET Expression ')'            { $$ = new_unary_expression(ctx, EXPR_BRACKETS, $2); }
    | tME                           { $$ = new_expression(ctx, EXPR_ME, 0); CHECK_ERROR; }

ClassDeclaration
    : tCLASS Identifier StSep ClassBody tEND tCLASS StSep       { $4->name = $2; $$ = $4; }

ClassBody
    : /* empty */                                 { $$ = new_class_decl(ctx); }
    | FunctionDecl                                { $$ = add_class_function(ctx, new_class_decl(ctx), $1); CHECK_ERROR; }
    | FunctionDecl StSep ClassBody                { $$ = add_class_function(ctx, $3, $1); CHECK_ERROR; }
    /* FIXME: We should use DimDecl here to support arrays, but that conflicts with PropertyDecl. */
    | Storage tIdentifier                         { dim_decl_t *dim_decl = new_dim_decl(ctx, $2, FALSE, NULL); CHECK_ERROR;
                                                  $$ = add_dim_prop(ctx, new_class_decl(ctx), dim_decl, $1); CHECK_ERROR; }
    | Storage tIdentifier StSep ClassBody         { dim_decl_t *dim_decl = new_dim_decl(ctx, $2, FALSE, NULL); CHECK_ERROR;
                                                  $$ = add_dim_prop(ctx, $4, dim_decl, $1); CHECK_ERROR; }
    | tDIM DimDecl                                { $$ = add_dim_prop(ctx, new_class_decl(ctx), $2, 0); CHECK_ERROR; }
    | tDIM DimDecl StSep ClassBody                { $$ = add_dim_prop(ctx, $4, $2, 0); CHECK_ERROR; }
    | PropertyDecl                                { $$ = add_class_function(ctx, new_class_decl(ctx), $1); CHECK_ERROR; }
    | PropertyDecl StSep ClassBody                { $$ = add_class_function(ctx, $3, $1); CHECK_ERROR; }

PropertyDecl
    : Storage_opt tPROPERTY tGET Identifier ArgumentsDecl_opt StSep BodyStatements tEND tPROPERTY
                                    { $$ = new_function_decl(ctx, $4, FUNC_PROPGET, $1, $5, $7); CHECK_ERROR; }
    | Storage_opt tPROPERTY tLET Identifier '(' ArgumentDeclList ')' StSep BodyStatements tEND tPROPERTY
                                    { $$ = new_function_decl(ctx, $4, FUNC_PROPLET, $1, $6, $9); CHECK_ERROR; }
    | Storage_opt tPROPERTY tSET Identifier '(' ArgumentDeclList ')' StSep BodyStatements tEND tPROPERTY
                                    { $$ = new_function_decl(ctx, $4, FUNC_PROPSET, $1, $6, $9); CHECK_ERROR; }

FunctionDecl
    : Storage_opt tSUB Identifier StSep BodyStatements tEND tSUB
                                    { $$ = new_function_decl(ctx, $3, FUNC_SUB, $1, NULL, $5); CHECK_ERROR; }
    | Storage_opt tSUB Identifier ArgumentsDecl Nl_opt BodyStatements tEND tSUB
                                    { $$ = new_function_decl(ctx, $3, FUNC_SUB, $1, $4, $6); CHECK_ERROR; }
    | Storage_opt tFUNCTION Identifier StSep BodyStatements tEND tFUNCTION
                                    { $$ = new_function_decl(ctx, $3, FUNC_FUNCTION, $1, NULL, $5); CHECK_ERROR; }
    | Storage_opt tFUNCTION Identifier ArgumentsDecl Nl_opt BodyStatements tEND tFUNCTION
                                    { $$ = new_function_decl(ctx, $3, FUNC_FUNCTION, $1, $4, $6); CHECK_ERROR; }

Storage_opt
    : /* empty*/                    { $$ = 0; }
    | Storage                       { $$ = $1; }

Storage
    : tPUBLIC tDEFAULT              { $$ = STORAGE_IS_DEFAULT; }
    | tPUBLIC                       { $$ = 0; }
    | tPRIVATE                      { $$ = STORAGE_IS_PRIVATE; }

ArgumentsDecl_opt
    : /* empty*/                                { $$ = 0; }
    | ArgumentsDecl                             { $$ = $1; }

ArgumentsDecl
    : tEMPTYBRACKETS                            { $$ = NULL; }
    | '(' ArgumentDeclList ')'                  { $$ = $2; }

ArgumentDeclList
    : ArgumentDecl                              { $$ = $1; }
    | ArgumentDecl ',' ArgumentDeclList         { $1->next = $3; $$ = $1; }

ArgumentDecl
    : Identifier EmptyBrackets_opt              { $$ = new_argument_decl(ctx, $1, TRUE); }
    | tBYREF Identifier EmptyBrackets_opt       { $$ = new_argument_decl(ctx, $2, TRUE); }
    | tBYVAL Identifier EmptyBrackets_opt       { $$ = new_argument_decl(ctx, $2, FALSE); }

/* these keywords may also be an identifier, depending on context */
Identifier
    : tIdentifier    { $$ = $1; }
    | tDEFAULT       { ctx->last_token = tIdentifier; $$ = $1; }
    | tERROR         { ctx->last_token = tIdentifier; $$ = $1; }
    | tEXPLICIT      { ctx->last_token = tIdentifier; $$ = $1; }
    | tPROPERTY      { ctx->last_token = tIdentifier; $$ = $1; }
    | tSTEP          { ctx->last_token = tIdentifier; $$ = $1; }

StSep_opt
    : /* empty */
    | StSep

Nl_opt
    : /* empty */
    | tNL Nl_opt

/* Most statements accept both new line and ':' as separators */
StSep
    : tNL
    | ':'
    | tNL StSep
    | ':' StSep

%%

static int parser_error(unsigned *loc, parser_ctx_t *ctx, const char *str)
{
    if(ctx->error_loc == -1)
        ctx->error_loc = *loc;
    if(ctx->hres == S_OK) {
        FIXME("%s: %s\n", debugstr_w(ctx->code + *loc), debugstr_a(str));
        ctx->hres = E_FAIL;
    }else {
        WARN("%s: %08lx\n", debugstr_w(ctx->code + *loc), ctx->hres);
    }
    return 0;
}

static void source_add_statement(parser_ctx_t *ctx, statement_t *stat)
{
    if(!stat)
        return;

    /* concatenate both linked lists */
    if(ctx->stats) {
        ctx->stats_tail->next = stat;
        ctx->stats_tail = stat;
    }else {
        ctx->stats = ctx->stats_tail = stat;
    }
    /* find new tail */
    while(ctx->stats_tail->next) {
        ctx->stats_tail=ctx->stats_tail->next;
    }
}

static void source_add_class(parser_ctx_t *ctx, class_decl_t *class_decl)
{
    class_decl->next = ctx->class_decls;
    ctx->class_decls = class_decl;
}

static void handle_isexpression_script(parser_ctx_t *ctx, expression_t *expr)
{
    retval_statement_t *stat;

    if(!expr)
        return;

    stat = new_statement(ctx, STAT_RETVAL, sizeof(*stat), 0);
    if(!stat)
        return;

    stat->expr = expr;
    ctx->stats = &stat->stat;
}

static void *new_expression(parser_ctx_t *ctx, expression_type_t type, size_t size)
{
    expression_t *expr;

    expr = parser_alloc(ctx, size ? size : sizeof(*expr));
    if(expr) {
        expr->type = type;
        expr->next = NULL;
    }

    return expr;
}

static expression_t *new_bool_expression(parser_ctx_t *ctx, VARIANT_BOOL value)
{
    bool_expression_t *expr;

    expr = new_expression(ctx, EXPR_BOOL, sizeof(*expr));
    if(!expr)
        return NULL;

    expr->value = value;
    return &expr->expr;
}

static expression_t *new_string_expression(parser_ctx_t *ctx, const WCHAR *value)
{
    string_expression_t *expr;

    expr = new_expression(ctx, EXPR_STRING, sizeof(*expr));
    if(!expr)
        return NULL;

    expr->value = value;
    return &expr->expr;
}

static expression_t *new_date_expression(parser_ctx_t *ctx, DATE value)
{
    date_expression_t *expr;

    expr = new_expression(ctx, EXPR_DATE, sizeof(*expr));
    if(!expr)
        return NULL;

    expr->value = value;
    return &expr->expr;
}

static expression_t *new_long_expression(parser_ctx_t *ctx, expression_type_t type, LONG value)
{
    int_expression_t *expr;

    expr = new_expression(ctx, type, sizeof(*expr));
    if(!expr)
        return NULL;

    expr->value = value;
    return &expr->expr;
}

static expression_t *new_double_expression(parser_ctx_t *ctx, double value)
{
    double_expression_t *expr;

    expr = new_expression(ctx, EXPR_DOUBLE, sizeof(*expr));
    if(!expr)
        return NULL;

    expr->value = value;
    return &expr->expr;
}

static expression_t *new_unary_expression(parser_ctx_t *ctx, expression_type_t type, expression_t *subexpr)
{
    unary_expression_t *expr;

    expr = new_expression(ctx, type, sizeof(*expr));
    if(!expr)
        return NULL;

    expr->subexpr = subexpr;
    return &expr->expr;
}

static expression_t *new_binary_expression(parser_ctx_t *ctx, expression_type_t type, expression_t *left, expression_t *right)
{
    binary_expression_t *expr;

    expr = new_expression(ctx, type, sizeof(*expr));
    if(!expr)
        return NULL;

    expr->left = left;
    expr->right = right;
    return &expr->expr;
}

static member_expression_t *new_member_expression(parser_ctx_t *ctx, expression_t *obj_expr, const WCHAR *identifier)
{
    member_expression_t *expr;

    expr = new_expression(ctx, EXPR_MEMBER, sizeof(*expr));
    if(!expr)
        return NULL;

    expr->obj_expr = obj_expr;
    expr->identifier = identifier;
    return expr;
}

static call_expression_t *new_call_expression(parser_ctx_t *ctx, expression_t *expr, expression_t *arguments)
{
    call_expression_t *call_expr;

    call_expr = new_expression(ctx, EXPR_CALL, sizeof(*call_expr));
    if(!call_expr)
        return NULL;

    call_expr->call_expr = expr;
    call_expr->args = arguments;
    return call_expr;
}

static call_expression_t *make_call_expression(parser_ctx_t *ctx, expression_t *callee_expr, expression_t *arguments)
{
    call_expression_t *call_expr;

    if(callee_expr->type == EXPR_MEMBER)
        return new_call_expression(ctx, callee_expr, arguments);
    if(callee_expr->type != EXPR_CALL) {
        FIXME("Unhandled for expr type %u\n", callee_expr->type);
        ctx->hres = E_FAIL;
        return NULL;
    }
    call_expr = (call_expression_t*)callee_expr;
    if(!call_expr->args) {
        call_expr->args = arguments;
        return call_expr;
    }

    if(call_expr->args->next) {
        FIXME("Invalid syntax: invalid use of parentheses for arguments\n");
        ctx->hres = E_FAIL;
        return NULL;
    }

    call_expr->args = new_unary_expression(ctx, EXPR_BRACKETS, call_expr->args);
    if(!call_expr->args)
        return NULL;
    if(!arguments)
        return call_expr;

    if(arguments->type != EXPR_NOARG) {
        FIXME("Invalid syntax: missing comma\n");
        ctx->hres = E_FAIL;
        return NULL;
    }

    call_expr->args->next = arguments->next;
    return call_expr;
}

static expression_t *new_new_expression(parser_ctx_t *ctx, const WCHAR *identifier)
{
    string_expression_t *expr;

    expr = new_expression(ctx, EXPR_NEW, sizeof(*expr));
    if(!expr)
        return NULL;

    expr->value = identifier;
    return &expr->expr;
}

static void *new_statement(parser_ctx_t *ctx, statement_type_t type, size_t size, unsigned loc)
{
    statement_t *stat;

    stat = parser_alloc(ctx, size ? size : sizeof(*stat));
    if(stat) {
        stat->type = type;
        stat->loc = loc;
        stat->next = NULL;
    }

    return stat;
}

static statement_t *new_call_statement(parser_ctx_t *ctx, unsigned loc, expression_t *expr)
{
    call_expression_t *call_expr = NULL;
    call_statement_t *stat;

    stat = new_statement(ctx, STAT_CALL, sizeof(*stat), loc);
    if(!stat)
        return NULL;

    switch(expr->type) {
    case EXPR_MEMBER:
        call_expr = new_call_expression(ctx, expr, NULL);
        break;
    case EXPR_CALL:
        call_expr = (call_expression_t*)expr;
        break;
    default:
        FIXME("Unsupported expr type %u\n", expr->type);
        ctx->hres = E_NOTIMPL;
    }
    if(!call_expr)
        return NULL;

    stat->expr = call_expr;
    return &stat->stat;
}

static statement_t *new_assign_statement(parser_ctx_t *ctx, unsigned loc, expression_t *left, expression_t *right)
{
    assign_statement_t *stat;

    stat = new_statement(ctx, STAT_ASSIGN, sizeof(*stat), loc);
    if(!stat)
        return NULL;

    stat->left_expr = left;
    stat->value_expr = right;

    return &stat->stat;
}

static statement_t *new_set_statement(parser_ctx_t *ctx, unsigned loc, expression_t *left, expression_t *right)
{
    assign_statement_t *stat;

    stat = new_statement(ctx, STAT_SET, sizeof(*stat), loc);
    if(!stat)
        return NULL;

    stat->left_expr = left;
    stat->value_expr = right;

    return &stat->stat;
}

static dim_decl_t *new_dim_decl(parser_ctx_t *ctx, const WCHAR *name, BOOL is_array, dim_list_t *dims)
{
    dim_decl_t *decl;

    decl = parser_alloc(ctx, sizeof(*decl));
    if(!decl)
        return NULL;

    decl->name = name;
    decl->is_array = is_array;
    decl->dims = dims;
    decl->next = NULL;
    return decl;
}

static dim_list_t *new_dim(parser_ctx_t *ctx, unsigned val, dim_list_t *next)
{
    dim_list_t *ret;

    ret = parser_alloc(ctx, sizeof(*ret));
    if(!ret)
        return NULL;

    ret->val = val;
    ret->next = next;
    return ret;
}

static statement_t *new_dim_statement(parser_ctx_t *ctx, unsigned loc, dim_decl_t *decls)
{
    dim_statement_t *stat;

    stat = new_statement(ctx, STAT_DIM, sizeof(*stat), loc);
    if(!stat)
        return NULL;

    stat->dim_decls = decls;
    return &stat->stat;
}

static redim_decl_t *new_redim_decl(parser_ctx_t *ctx, const WCHAR *identifier, expression_t *dims)
{
    redim_decl_t *decl;

    decl = parser_alloc(ctx, sizeof(*decl));
    if(!decl)
        return NULL;

    decl->identifier = identifier;
    decl->dims = dims;
    decl->next = NULL;
    return decl;
}

static statement_t *new_redim_statement(parser_ctx_t *ctx, unsigned loc, BOOL preserve, redim_decl_t *decls)
{
    redim_statement_t *stat;

    stat = new_statement(ctx, STAT_REDIM, sizeof(*stat), loc);
    if(!stat)
        return NULL;

    stat->preserve = preserve;
    stat->redim_decls = decls;
    return &stat->stat;
}

static elseif_decl_t *new_elseif_decl(parser_ctx_t *ctx, unsigned loc, expression_t *expr, statement_t *stat)
{
    elseif_decl_t *decl;

    decl = parser_alloc(ctx, sizeof(*decl));
    if(!decl)
        return NULL;

    decl->expr = expr;
    decl->stat = stat;
    decl->loc = loc;
    decl->next = NULL;
    return decl;
}

static statement_t *new_while_statement(parser_ctx_t *ctx, unsigned loc, statement_type_t type, expression_t *expr, statement_t *body)
{
    while_statement_t *stat;

    stat = new_statement(ctx, type, sizeof(*stat), loc);
    if(!stat)
        return NULL;

    stat->expr = expr;
    stat->body = body;
    return &stat->stat;
}

static statement_t *new_forto_statement(parser_ctx_t *ctx, unsigned loc, const WCHAR *identifier, expression_t *from_expr,
        expression_t *to_expr, expression_t *step_expr, statement_t *body)
{
    forto_statement_t *stat;

    stat = new_statement(ctx, STAT_FORTO, sizeof(*stat), loc);
    if(!stat)
        return NULL;

    stat->identifier = identifier;
    stat->from_expr = from_expr;
    stat->to_expr = to_expr;
    stat->step_expr = step_expr;
    stat->body = body;
    return &stat->stat;
}

static statement_t *new_foreach_statement(parser_ctx_t *ctx, unsigned loc, const WCHAR *identifier, expression_t *group_expr,
        statement_t *body)
{
    foreach_statement_t *stat;

    stat = new_statement(ctx, STAT_FOREACH, sizeof(*stat), loc);
    if(!stat)
        return NULL;

    stat->identifier = identifier;
    stat->group_expr = group_expr;
    stat->body = body;
    return &stat->stat;
}

static statement_t *new_if_statement(parser_ctx_t *ctx, unsigned loc, expression_t *expr, statement_t *if_stat, elseif_decl_t *elseif_decl,
        statement_t *else_stat)
{
    if_statement_t *stat;

    stat = new_statement(ctx, STAT_IF, sizeof(*stat), loc);
    if(!stat)
        return NULL;

    stat->expr = expr;
    stat->if_stat = if_stat;
    stat->elseifs = elseif_decl;
    stat->else_stat = else_stat;
    return &stat->stat;
}

static statement_t *new_select_statement(parser_ctx_t *ctx, unsigned loc, expression_t *expr, case_clausule_t *case_clausules)
{
    select_statement_t *stat;

    stat = new_statement(ctx, STAT_SELECT, sizeof(*stat), loc);
    if(!stat)
        return NULL;

    stat->expr = expr;
    stat->case_clausules = case_clausules;
    return &stat->stat;
}

static statement_t *new_with_statement(parser_ctx_t *ctx, unsigned loc, expression_t *expr, statement_t *body)
{
    with_statement_t *stat;

    stat = new_statement(ctx, STAT_WITH, sizeof(*stat), loc);
    if(!stat)
        return NULL;

    stat->expr = expr;
    stat->body = body;
    return &stat->stat;
}

static case_clausule_t *new_case_clausule(parser_ctx_t *ctx, expression_t *expr, statement_t *stat, case_clausule_t *next)
{
    case_clausule_t *ret;

    ret = parser_alloc(ctx, sizeof(*ret));
    if(!ret)
        return NULL;

    ret->expr = expr;
    ret->stat = stat;
    ret->next = next;
    return ret;
}

static statement_t *new_onerror_statement(parser_ctx_t *ctx, unsigned loc, BOOL resume_next)
{
    onerror_statement_t *stat;

    stat = new_statement(ctx, STAT_ONERROR, sizeof(*stat), loc);
    if(!stat)
        return NULL;

    stat->resume_next = resume_next;
    return &stat->stat;
}

static arg_decl_t *new_argument_decl(parser_ctx_t *ctx, const WCHAR *name, BOOL by_ref)
{
    arg_decl_t *arg_decl;

    arg_decl = parser_alloc(ctx, sizeof(*arg_decl));
    if(!arg_decl)
        return NULL;

    arg_decl->name = name;
    arg_decl->by_ref = by_ref;
    arg_decl->next = NULL;
    return arg_decl;
}

static function_decl_t *new_function_decl(parser_ctx_t *ctx, const WCHAR *name, function_type_t type,
        unsigned storage_flags, arg_decl_t *arg_decl, statement_t *body)
{
    function_decl_t *decl;
    BOOL is_default = FALSE;

    if(storage_flags & STORAGE_IS_DEFAULT) {
        if(type == FUNC_PROPGET || type == FUNC_FUNCTION || type == FUNC_SUB) {
            is_default = TRUE;
        }else {
            FIXME("Invalid default property\n");
            ctx->hres = E_FAIL;
            return NULL;
        }
    }

    decl = parser_alloc(ctx, sizeof(*decl));
    if(!decl)
        return NULL;

    decl->name = name;
    decl->type = type;
    decl->is_public = !(storage_flags & STORAGE_IS_PRIVATE);
    decl->is_default = is_default;
    decl->args = arg_decl;
    decl->body = body;
    decl->next = NULL;
    decl->next_prop_func = NULL;
    return decl;
}

static statement_t *new_function_statement(parser_ctx_t *ctx, unsigned loc, function_decl_t *decl)
{
    function_statement_t *stat;

    stat = new_statement(ctx, STAT_FUNC, sizeof(*stat), loc);
    if(!stat)
        return NULL;

    stat->func_decl = decl;
    return &stat->stat;
}

static class_decl_t *new_class_decl(parser_ctx_t *ctx)
{
    class_decl_t *class_decl;

    class_decl = parser_alloc(ctx, sizeof(*class_decl));
    if(!class_decl)
        return NULL;

    class_decl->funcs = NULL;
    class_decl->props = NULL;
    class_decl->next = NULL;
    return class_decl;
}

static class_decl_t *add_class_function(parser_ctx_t *ctx, class_decl_t *class_decl, function_decl_t *decl)
{
    function_decl_t *iter;

    for(iter = class_decl->funcs; iter; iter = iter->next) {
        if(!wcsicmp(iter->name, decl->name)) {
            if(decl->type == FUNC_SUB || decl->type == FUNC_FUNCTION) {
                FIXME("Redefinition of %s::%s\n", debugstr_w(class_decl->name), debugstr_w(decl->name));
                ctx->hres = E_FAIL;
                return NULL;
            }

            while(1) {
                if(iter->type == decl->type) {
                    FIXME("Redefinition of %s::%s\n", debugstr_w(class_decl->name), debugstr_w(decl->name));
                    ctx->hres = E_FAIL;
                    return NULL;
                }
                if(!iter->next_prop_func)
                    break;
                iter = iter->next_prop_func;
            }

            iter->next_prop_func = decl;
            return class_decl;
        }
    }

    decl->next = class_decl->funcs;
    class_decl->funcs = decl;
    return class_decl;
}

static class_decl_t *add_dim_prop(parser_ctx_t *ctx, class_decl_t *class_decl, dim_decl_t *dim_decl, unsigned storage_flags)
{
    if(storage_flags & STORAGE_IS_DEFAULT) {
        FIXME("variant prop can't be default value\n");
        ctx->hres = E_FAIL;
        return NULL;
    }

    dim_decl->is_public = !(storage_flags & STORAGE_IS_PRIVATE);
    dim_decl->next = class_decl->props;
    class_decl->props = dim_decl;
    return class_decl;
}

static const_decl_t *new_const_decl(parser_ctx_t *ctx, const WCHAR *name, expression_t *expr)
{
    const_decl_t *decl;

    decl = parser_alloc(ctx, sizeof(*decl));
    if(!decl)
        return NULL;

    decl->name = name;
    decl->value_expr = expr;
    decl->next = NULL;
    return decl;
}

static statement_t *new_const_statement(parser_ctx_t *ctx, unsigned loc, const_decl_t *decls)
{
    const_statement_t *stat;

    stat = new_statement(ctx, STAT_CONST, sizeof(*stat), loc);
    if(!stat)
        return NULL;

    stat->decls = decls;
    return &stat->stat;
}

static statement_t *link_statements(statement_t *head, statement_t *tail)
{
    statement_t *iter;

    if (!head) return tail;

    for(iter = head; iter->next; iter = iter->next);
    iter->next = tail;

    return head;
}

void *parser_alloc(parser_ctx_t *ctx, size_t size)
{
    void *ret;

    ret = heap_pool_alloc(&ctx->heap, size);
    if(!ret)
        ctx->hres = E_OUTOFMEMORY;
    return ret;
}

HRESULT parse_script(parser_ctx_t *ctx, const WCHAR *code, const WCHAR *delimiter, DWORD flags)
{
    ctx->code = ctx->ptr = code;
    ctx->end = ctx->code + lstrlenW(ctx->code);

    heap_pool_init(&ctx->heap);

    ctx->hres = S_OK;
    ctx->error_loc = -1;
    ctx->last_token = tNL;
    ctx->last_nl = 0;
    ctx->stats = ctx->stats_tail = NULL;
    ctx->class_decls = NULL;
    ctx->option_explicit = FALSE;
    ctx->is_html = delimiter && !wcsicmp(delimiter, L"</script>");

    if(flags & SCRIPTTEXT_ISEXPRESSION)
        ctx->last_token = tEXPRESSION;

    parser_parse(ctx);

    return ctx->hres;
}

void parser_release(parser_ctx_t *ctx)
{
    heap_pool_free(&ctx->heap);
}
