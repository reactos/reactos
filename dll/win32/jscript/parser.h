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

#pragma once

typedef struct _source_elements_t source_elements_t;
typedef struct _expression_t expression_t;
typedef struct _statement_t statement_t;

typedef struct {
    BOOL is_num;
    union {
        BOOL b;
        double n;
    } u;
} ccval_t;

typedef struct _parser_ctx_t {
    const WCHAR *begin;
    const WCHAR *end;
    const WCHAR *ptr;

    script_ctx_t *script;
    source_elements_t *source;
    BOOL nl;
    BOOL implicit_nl_semicolon;
    BOOL is_html;
    BOOL lexer_error;
    HRESULT hres;

    ccval_t ccval;
    unsigned cc_if_depth;

    heap_pool_t heap;
} parser_ctx_t;

HRESULT script_parse(script_ctx_t*,const WCHAR*,const WCHAR*,BOOL,parser_ctx_t**) DECLSPEC_HIDDEN;
void parser_release(parser_ctx_t*) DECLSPEC_HIDDEN;

int parser_lex(void*,parser_ctx_t*) DECLSPEC_HIDDEN;

static inline void *parser_alloc(parser_ctx_t *ctx, DWORD size)
{
    return heap_pool_alloc(&ctx->heap, size);
}

static inline void *parser_alloc_tmp(parser_ctx_t *ctx, DWORD size)
{
    return heap_pool_alloc(&ctx->script->tmp_heap, size);
}

typedef enum {
    LT_DOUBLE,
    LT_STRING,
    LT_BOOL,
    LT_NULL,
    LT_REGEXP
}literal_type_t;

typedef struct {
    literal_type_t type;
    union {
        double dval;
        const WCHAR *wstr;
        BOOL bval;
        struct {
            const WCHAR *str;
            DWORD str_len;
            DWORD flags;
        } regexp;
    } u;
} literal_t;

literal_t *parse_regexp(parser_ctx_t*) DECLSPEC_HIDDEN;
literal_t *new_boolean_literal(parser_ctx_t*,BOOL) DECLSPEC_HIDDEN;

typedef struct _variable_declaration_t {
    const WCHAR *identifier;
    expression_t *expr;

    struct _variable_declaration_t *next;
    struct _variable_declaration_t *global_next; /* for compiler */
} variable_declaration_t;

typedef enum {
    STAT_BLOCK,
    STAT_BREAK,
    STAT_CONTINUE,
    STAT_EMPTY,
    STAT_EXPR,
    STAT_FOR,
    STAT_FORIN,
    STAT_IF,
    STAT_LABEL,
    STAT_RETURN,
    STAT_SWITCH,
    STAT_THROW,
    STAT_TRY,
    STAT_VAR,
    STAT_WHILE,
    STAT_WITH
} statement_type_t;

struct _statement_t {
    statement_type_t type;
    statement_t *next;
};

typedef struct {
    statement_t stat;
    statement_t *stat_list;
} block_statement_t;

typedef struct {
    statement_t stat;
    variable_declaration_t *variable_list;
} var_statement_t;

typedef struct {
    statement_t stat;
    expression_t *expr;
} expression_statement_t;

typedef struct {
    statement_t stat;
    expression_t *expr;
    statement_t *if_stat;
    statement_t *else_stat;
} if_statement_t;

typedef struct {
    statement_t stat;
    BOOL do_while;
    expression_t *expr;
    statement_t *statement;
} while_statement_t;

typedef struct {
    statement_t stat;
    variable_declaration_t *variable_list;
    expression_t *begin_expr;
    expression_t *expr;
    expression_t *end_expr;
    statement_t *statement;
} for_statement_t;

typedef struct {
    statement_t stat;
    variable_declaration_t *variable;
    expression_t *expr;
    expression_t *in_expr;
    statement_t *statement;
} forin_statement_t;

typedef struct {
    statement_t stat;
    const WCHAR *identifier;
} branch_statement_t;

typedef struct {
    statement_t stat;
    expression_t *expr;
    statement_t *statement;
} with_statement_t;

typedef struct {
    statement_t stat;
    const WCHAR *identifier;
    statement_t *statement;
} labelled_statement_t;

typedef struct _case_clausule_t {
    expression_t *expr;
    statement_t *stat;

    struct _case_clausule_t *next;
} case_clausule_t;

typedef struct {
    statement_t stat;
    expression_t *expr;
    case_clausule_t *case_list;
} switch_statement_t;

typedef struct {
    const WCHAR *identifier;
    statement_t *statement;
} catch_block_t;

typedef struct {
    statement_t stat;
    statement_t *try_statement;
    catch_block_t *catch_block;
    statement_t *finally_statement;
} try_statement_t;

typedef enum {
     EXPR_COMMA,
     EXPR_OR,
     EXPR_AND,
     EXPR_BOR,
     EXPR_BXOR,
     EXPR_BAND,
     EXPR_INSTANCEOF,
     EXPR_IN,
     EXPR_ADD,
     EXPR_SUB,
     EXPR_MUL,
     EXPR_DIV,
     EXPR_MOD,
     EXPR_DELETE,
     EXPR_VOID,
     EXPR_TYPEOF,
     EXPR_MINUS,
     EXPR_PLUS,
     EXPR_POSTINC,
     EXPR_POSTDEC,
     EXPR_PREINC,
     EXPR_PREDEC,
     EXPR_EQ,
     EXPR_EQEQ,
     EXPR_NOTEQ,
     EXPR_NOTEQEQ,
     EXPR_LESS,
     EXPR_LESSEQ,
     EXPR_GREATER,
     EXPR_GREATEREQ,
     EXPR_BITNEG,
     EXPR_LOGNEG,
     EXPR_LSHIFT,
     EXPR_RSHIFT,
     EXPR_RRSHIFT,
     EXPR_ASSIGN,
     EXPR_ASSIGNLSHIFT,
     EXPR_ASSIGNRSHIFT,
     EXPR_ASSIGNRRSHIFT,
     EXPR_ASSIGNADD,
     EXPR_ASSIGNSUB,
     EXPR_ASSIGNMUL,
     EXPR_ASSIGNDIV,
     EXPR_ASSIGNMOD,
     EXPR_ASSIGNAND,
     EXPR_ASSIGNOR,
     EXPR_ASSIGNXOR,
     EXPR_COND,
     EXPR_ARRAY,
     EXPR_MEMBER,
     EXPR_NEW,
     EXPR_CALL,
     EXPR_THIS,
     EXPR_FUNC,
     EXPR_IDENT,
     EXPR_ARRAYLIT,
     EXPR_PROPVAL,
     EXPR_LITERAL
} expression_type_t;

struct _expression_t {
    expression_type_t type;
};

typedef struct _parameter_t {
    const WCHAR *identifier;
    struct _parameter_t *next;
} parameter_t;

struct _source_elements_t {
    statement_t *statement;
    statement_t *statement_tail;
};

typedef struct _function_expression_t {
    expression_t expr;
    const WCHAR *identifier;
    const WCHAR *event_target;
    parameter_t *parameter_list;
    source_elements_t *source_elements;
    const WCHAR *src_str;
    DWORD src_len;

    struct _function_expression_t *next; /* for compiler */
} function_expression_t;

typedef struct {
    expression_t expr;
    expression_t *expression1;
    expression_t *expression2;
} binary_expression_t;

typedef struct {
    expression_t expr;
    expression_t *expression;
} unary_expression_t;

typedef struct {
    expression_t expr;
    expression_t *expression;
    expression_t *true_expression;
    expression_t *false_expression;
} conditional_expression_t;

typedef struct {
    expression_t expr;
    expression_t *expression;
    const WCHAR *identifier;
} member_expression_t;

typedef struct _argument_t {
    expression_t *expr;

    struct _argument_t *next;
} argument_t;

typedef struct {
    expression_t expr;
    expression_t *expression;
    argument_t *argument_list;
} call_expression_t;

typedef struct {
    expression_t expr;
    const WCHAR *identifier;
} identifier_expression_t;

typedef struct {
    expression_t expr;
    literal_t *literal;
} literal_expression_t;

typedef struct _array_element_t {
    int elision;
    expression_t *expr;

    struct _array_element_t *next;
} array_element_t;

typedef struct {
    expression_t expr;
    array_element_t *element_list;
    int length;
} array_literal_expression_t;

typedef struct _prop_val_t {
    literal_t *name;
    expression_t *value;

    struct _prop_val_t *next;
} prop_val_t;

typedef struct {
    expression_t expr;
    prop_val_t *property_list;
} property_value_expression_t;

BOOL try_parse_ccval(parser_ctx_t*,ccval_t*) DECLSPEC_HIDDEN;
BOOL parse_cc_expr(parser_ctx_t*) DECLSPEC_HIDDEN;

static inline ccval_t ccval_num(double n)
{
    ccval_t r;
    r.is_num = TRUE;
    r.u.n = n;
    return r;
}

static inline ccval_t ccval_bool(BOOL b)
{
    ccval_t r;
    r.is_num = FALSE;
    r.u.b = b;
    return r;
}

static inline BOOL get_ccbool(ccval_t v)
{
    return v.is_num ? v.u.n != 0 : v.u.b;
}

static inline double get_ccnum(ccval_t v)
{
    return v.is_num ? v.u.n : v.u.b;
}
