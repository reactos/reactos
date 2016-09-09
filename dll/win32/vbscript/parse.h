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

typedef enum {
    EXPR_ADD,
    EXPR_AND,
    EXPR_BOOL,
    EXPR_BRACKETS,
    EXPR_CONCAT,
    EXPR_DIV,
    EXPR_DOUBLE,
    EXPR_EMPTY,
    EXPR_EQUAL,
    EXPR_EQV,
    EXPR_EXP,
    EXPR_GT,
    EXPR_GTEQ,
    EXPR_IDIV,
    EXPR_IMP,
    EXPR_IS,
    EXPR_LT,
    EXPR_LTEQ,
    EXPR_ME,
    EXPR_MEMBER,
    EXPR_MOD,
    EXPR_MUL,
    EXPR_NEG,
    EXPR_NEQUAL,
    EXPR_NEW,
    EXPR_NOARG, /* not a real expression */
    EXPR_NOT,
    EXPR_NOTHING,
    EXPR_NULL,
    EXPR_OR,
    EXPR_STRING,
    EXPR_SUB,
    EXPR_ULONG,
    EXPR_USHORT,
    EXPR_XOR
} expression_type_t;

typedef struct _expression_t {
    expression_type_t type;
    struct _expression_t *next;
} expression_t;

typedef struct {
    expression_t expr;
    VARIANT_BOOL value;
} bool_expression_t;

typedef struct {
    expression_t expr;
    LONG value;
} int_expression_t;

typedef struct {
    expression_t expr;
    double value;
} double_expression_t;

typedef struct {
    expression_t expr;
    const WCHAR *value;
} string_expression_t;

typedef struct {
    expression_t expr;
    expression_t *subexpr;
} unary_expression_t;

typedef struct {
    expression_t expr;
    expression_t *left;
    expression_t *right;
} binary_expression_t;

typedef struct {
    expression_t expr;
    expression_t *obj_expr;
    const WCHAR *identifier;
    expression_t *args;
} member_expression_t;

typedef enum {
    STAT_ASSIGN,
    STAT_CALL,
    STAT_CONST,
    STAT_DIM,
    STAT_DOUNTIL,
    STAT_DOWHILE,
    STAT_EXITDO,
    STAT_EXITFOR,
    STAT_EXITFUNC,
    STAT_EXITPROP,
    STAT_EXITSUB,
    STAT_FOREACH,
    STAT_FORTO,
    STAT_FUNC,
    STAT_IF,
    STAT_ONERROR,
    STAT_SELECT,
    STAT_SET,
    STAT_STOP,
    STAT_UNTIL,
    STAT_WHILE,
    STAT_WHILELOOP
} statement_type_t;

typedef struct _statement_t {
    statement_type_t type;
    struct _statement_t *next;
} statement_t;

typedef struct {
    statement_t stat;
    member_expression_t *expr;
    BOOL is_strict;
} call_statement_t;

typedef struct {
    statement_t stat;
    member_expression_t *member_expr;
    expression_t *value_expr;
} assign_statement_t;

typedef struct _dim_list_t {
    unsigned val;
    struct _dim_list_t *next;
} dim_list_t;

typedef struct _dim_decl_t {
    const WCHAR *name;
    BOOL is_array;
    BOOL is_public; /* Used only for class members. */
    dim_list_t *dims;
    struct _dim_decl_t *next;
} dim_decl_t;

typedef struct _dim_statement_t {
    statement_t stat;
    dim_decl_t *dim_decls;
} dim_statement_t;

typedef struct _arg_decl_t {
    const WCHAR *name;
    BOOL by_ref;
    struct _arg_decl_t *next;
} arg_decl_t;

typedef struct _function_decl_t {
    const WCHAR *name;
    function_type_t type;
    BOOL is_public;
    arg_decl_t *args;
    statement_t *body;
    struct _function_decl_t *next;
    struct _function_decl_t *next_prop_func;
} function_decl_t;

typedef struct {
    statement_t stat;
    function_decl_t *func_decl;
} function_statement_t;

typedef struct _class_decl_t {
    const WCHAR *name;
    function_decl_t *funcs;
    dim_decl_t *props;
    struct _class_decl_t *next;
} class_decl_t;

typedef struct _elseif_decl_t {
    expression_t *expr;
    statement_t *stat;
    struct _elseif_decl_t *next;
} elseif_decl_t;

typedef struct {
    statement_t stat;
    expression_t *expr;
    statement_t *if_stat;
    elseif_decl_t *elseifs;
    statement_t *else_stat;
} if_statement_t;

typedef struct {
    statement_t stat;
    expression_t *expr;
    statement_t *body;
} while_statement_t;

typedef struct {
    statement_t stat;
    const WCHAR *identifier;
    expression_t *from_expr;
    expression_t *to_expr;
    expression_t *step_expr;
    statement_t *body;
} forto_statement_t;

typedef struct {
    statement_t stat;
    const WCHAR *identifier;
    expression_t *group_expr;
    statement_t *body;
} foreach_statement_t;

typedef struct {
    statement_t stat;
    BOOL resume_next;
} onerror_statement_t;

typedef struct _const_decl_t {
    const WCHAR *name;
    expression_t *value_expr;
    struct _const_decl_t *next;
} const_decl_t;

typedef struct {
    statement_t stat;
    const_decl_t *decls;
} const_statement_t;

typedef struct _case_clausule_t {
    expression_t *expr;
    statement_t *stat;
    struct _case_clausule_t *next;
} case_clausule_t;

typedef struct {
    statement_t stat;
    expression_t *expr;
    case_clausule_t *case_clausules;
} select_statement_t;

typedef struct {
    const WCHAR *code;
    const WCHAR *ptr;
    const WCHAR *end;

    BOOL option_explicit;
    BOOL parse_complete;
    BOOL is_html;
    HRESULT hres;

    int last_token;
    unsigned last_nl;

    statement_t *stats;
    statement_t *stats_tail;
    class_decl_t *class_decls;

    heap_pool_t heap;
} parser_ctx_t;

HRESULT parse_script(parser_ctx_t*,const WCHAR*,const WCHAR*) DECLSPEC_HIDDEN;
void parser_release(parser_ctx_t*) DECLSPEC_HIDDEN;
int parser_lex(void*,parser_ctx_t*) DECLSPEC_HIDDEN;
void *parser_alloc(parser_ctx_t*,size_t) DECLSPEC_HIDDEN;
