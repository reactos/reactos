/*
 * Copyright 2014 Jacek Caban for CodeWeavers
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
#include "engine.h"
#include "parser.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

%}

%lex-param { parser_ctx_t *ctx }
%parse-param { parser_ctx_t *ctx }
%define api.prefix {cc_parser_}
%define api.pure
%start CCExpr

%union {
    ccval_t ccval;
}

%token tEQ tEQEQ tNEQ tNEQEQ tLSHIFT tRSHIFT tRRSHIFT tOR tAND tLEQ tGEQ
%token <ccval> tCCValue

%type <ccval> CCUnaryExpression CCLogicalORExpression CCLogicalANDExpression
%type <ccval> CCBitwiseORExpression CCBitwiseXORExpression CCBitwiseANDExpression
%type <ccval> CCEqualityExpression CCRelationalExpression CCShiftExpression CCAdditiveExpression CCMultiplicativeExpression

%{

static int cc_parser_error(parser_ctx_t *ctx, const char *str)
{
    if(SUCCEEDED(ctx->hres)) {
        WARN("%s\n", str);
        ctx->hres = JS_E_SYNTAX;
    }

    return 0;
}

static int cc_parser_lex(void *lval, parser_ctx_t *ctx)
{
    int r;

    r = try_parse_ccval(ctx, lval);
    if(r)
        return r > 0 ? tCCValue : -1;

    switch(*ctx->ptr) {
    case '(':
    case ')':
    case '+':
    case '-':
    case '*':
    case '/':
    case '~':
    case '%':
    case '^':
        return *ctx->ptr++;
    case '=':
         if(*++ctx->ptr == '=') {
             if(*++ctx->ptr == '=') {
                 ctx->ptr++;
                 return tEQEQ;
             }
             return tEQ;
         }
         break;
    case '!':
        if(*++ctx->ptr == '=') {
            if(*++ctx->ptr == '=') {
                ctx->ptr++;
                return tNEQEQ;
            }
            return tNEQ;
        }
        return '!';
    case '<':
        switch(*++ctx->ptr) {
        case '<':
            ctx->ptr++;
            return tLSHIFT;
        case '=':
            ctx->ptr++;
            return tLEQ;
        default:
            return '<';
        }
    case '>':
        switch(*++ctx->ptr) {
        case '>':
            if(*++ctx->ptr == '>') {
                ctx->ptr++;
                return tRRSHIFT;
            }
            return tRSHIFT;
        case '=':
            ctx->ptr++;
            return tGEQ;
        default:
            return '>';
        }
    case '|':
        if(*++ctx->ptr == '|') {
            ctx->ptr++;
            return tOR;
        }
        return '|';
    case '&':
        if(*++ctx->ptr == '&') {
            ctx->ptr++;
            return tAND;
        }
        return '&';
    }

    WARN("Failed to interpret %s\n", debugstr_w(ctx->ptr));
    return -1;
}

%}

%%

/* FIXME: Implement missing expressions. */

CCExpr
    : CCUnaryExpression { ctx->ccval = $1;
                          (void)cc_parser_nerrs; /* avoid unused variable warning */
                          YYACCEPT; }

CCUnaryExpression
    : tCCValue                      { $$ = $1; }
    | '(' CCLogicalORExpression ')' { $$ = $2; }
    | '!' CCUnaryExpression         { $$ = ccval_bool(!get_ccbool($2)); };
    | '~' CCUnaryExpression         { FIXME("'~' expression not implemented\n"); ctx->hres = E_NOTIMPL; YYABORT; }
    | '+' CCUnaryExpression         { FIXME("'+' expression not implemented\n"); ctx->hres = E_NOTIMPL; YYABORT; }
    | '-' CCUnaryExpression         { FIXME("'-' expression not implemented\n"); ctx->hres = E_NOTIMPL; YYABORT; }

CCLogicalORExpression
    : CCLogicalANDExpression        { $$ = $1; }
    | CCLogicalORExpression tOR CCLogicalANDExpression
                                    { FIXME("'||' expression not implemented\n"); ctx->hres = E_NOTIMPL; YYABORT; }

CCLogicalANDExpression
    : CCBitwiseORExpression         { $$ = $1; }
    | CCBitwiseANDExpression tAND CCBitwiseORExpression
                                    { $$ = ccval_bool(get_ccbool($1) && get_ccbool($3)); }

CCBitwiseORExpression
    : CCBitwiseXORExpression        { $$ = $1; }
    | CCBitwiseORExpression '|' CCBitwiseXORExpression
                                    { FIXME("'|' expression not implemented\n"); ctx->hres = E_NOTIMPL; YYABORT; }

CCBitwiseXORExpression
    : CCBitwiseANDExpression        { $$ = $1; }
    | CCBitwiseXORExpression '^' CCBitwiseANDExpression
                                    { FIXME("'^' expression not implemented\n"); ctx->hres = E_NOTIMPL; YYABORT; }

CCBitwiseANDExpression
    : CCEqualityExpression          { $$ = $1; }
    | CCBitwiseANDExpression '&' CCEqualityExpression
                                    { FIXME("'&' expression not implemented\n"); ctx->hres = E_NOTIMPL; YYABORT; }

CCEqualityExpression
    : CCRelationalExpression        { $$ = $1; }
    | CCEqualityExpression tEQ CCRelationalExpression
                                    { $$ = ccval_bool(get_ccnum($1) == get_ccnum($3)); }
    | CCEqualityExpression tNEQ CCRelationalExpression
                                    { $$ = ccval_bool(get_ccnum($1) != get_ccnum($3)); }
    | CCEqualityExpression tEQEQ CCRelationalExpression
                                    { FIXME("'===' expression not implemented\n"); ctx->hres = E_NOTIMPL; YYABORT; }
    | CCEqualityExpression tNEQEQ CCRelationalExpression
                                    { FIXME("'!==' expression not implemented\n"); ctx->hres = E_NOTIMPL; YYABORT; }

CCRelationalExpression
    : CCShiftExpression             { $$ = $1; }
    | CCRelationalExpression '<' CCShiftExpression
                                    { $$ = ccval_bool(get_ccnum($1) < get_ccnum($3)); }
    | CCRelationalExpression tLEQ CCShiftExpression
                                    { $$ = ccval_bool(get_ccnum($1) <= get_ccnum($3)); }
    | CCRelationalExpression '>' CCShiftExpression
                                    { $$ = ccval_bool(get_ccnum($1) > get_ccnum($3)); }
    | CCRelationalExpression tGEQ CCShiftExpression
                                    { $$ = ccval_bool(get_ccnum($1) >= get_ccnum($3)); }

CCShiftExpression
    : CCAdditiveExpression          { $$ = $1; }
    | CCShiftExpression tLSHIFT CCAdditiveExpression
                                    { FIXME("'<<' expression not implemented\n"); ctx->hres = E_NOTIMPL; YYABORT; }
    | CCShiftExpression tRSHIFT CCAdditiveExpression
                                    { FIXME("'>>' expression not implemented\n"); ctx->hres = E_NOTIMPL; YYABORT; }
    | CCShiftExpression tRRSHIFT CCAdditiveExpression
                                    { FIXME("'>>>' expression not implemented\n"); ctx->hres = E_NOTIMPL; YYABORT; }

CCAdditiveExpression
    : CCMultiplicativeExpression    { $$ = $1; }
    | CCAdditiveExpression '+' CCMultiplicativeExpression
                                    { $$ = ccval_num(get_ccnum($1) + get_ccnum($3)); }
    | CCAdditiveExpression '-' CCMultiplicativeExpression
                                    { $$ = ccval_num(get_ccnum($1) - get_ccnum($3)); }

CCMultiplicativeExpression
    : CCUnaryExpression             { $$ = $1; }
    | CCMultiplicativeExpression '*' CCUnaryExpression
                                    { $$ = ccval_num(get_ccnum($1) * get_ccnum($3)); }
    | CCMultiplicativeExpression '/' CCUnaryExpression
                                    { $$ = ccval_num(get_ccnum($1) / get_ccnum($3)); }
    | CCMultiplicativeExpression '%' CCUnaryExpression
                                    { FIXME("'%%' expression not implemented\n"); ctx->hres = E_NOTIMPL; YYABORT; }

%%

BOOL parse_cc_expr(parser_ctx_t *ctx)
{
    ctx->hres = S_OK;
    cc_parser_parse(ctx);
    return SUCCEEDED(ctx->hres);
}
