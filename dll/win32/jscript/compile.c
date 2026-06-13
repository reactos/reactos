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

#include <math.h>
#include <assert.h>

#include "jscript.h"
#include "engine.h"
#include "parser.h"

#include "wine/rbtree.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);
WINE_DECLARE_DEBUG_CHANNEL(jscript_disas);

typedef struct _statement_ctx_t {
    unsigned stack_use;
    BOOL using_scope;
    BOOL using_except;

    unsigned break_label;
    unsigned continue_label;

    const labelled_statement_t *labelled_stat;

    unsigned int scope_index;
    BOOL block_scope;
    BOOL scope_has_functions;
    struct _statement_ctx_t *next;
} statement_ctx_t;

typedef struct {
    struct wine_rb_entry entry;
    BSTR name;
    int ref;
} function_local_t;

typedef struct _compiler_ctx_t {
    parser_ctx_t *parser;
    bytecode_t *code;

    BOOL from_eval;

    unsigned code_off;
    unsigned code_size;

    unsigned *labels;
    unsigned labels_size;
    unsigned labels_cnt;

    struct
    {
        struct wine_rb_tree locals;
        unsigned int locals_cnt;
        unsigned int *ref_index;
    }
    *local_scopes;
    unsigned local_scope_count;
    unsigned local_scope_size;

    statement_ctx_t *stat_ctx;
    function_code_t *func;

    unsigned loc;

    function_expression_t *func_head;
    function_expression_t *func_tail;
    function_expression_t *current_function_expr;

    heap_pool_t heap;
} compiler_ctx_t;

static const struct {
    const char *op_str;
    instr_arg_type_t arg1_type;
    instr_arg_type_t arg2_type;
} instr_info[] = {
#define X(n,a,b,c) {#n,b,c},
OP_LIST
#undef X
};

static void dump_instr_arg(instr_arg_type_t type, instr_arg_t *arg)
{
    switch(type) {
    case ARG_STR:
        TRACE_(jscript_disas)("\t%s", debugstr_jsstr(arg->str));
        break;
    case ARG_BSTR:
        TRACE_(jscript_disas)("\t%s", debugstr_wn(arg->bstr, SysStringLen(arg->bstr)));
        break;
    case ARG_INT:
        TRACE_(jscript_disas)("\t%d", arg->uint);
        break;
    case ARG_UINT:
    case ARG_ADDR:
        TRACE_(jscript_disas)("\t%u", arg->uint);
        break;
    case ARG_FUNC:
    case ARG_NONE:
        break;
    DEFAULT_UNREACHABLE;
    }
}

static void dump_code(compiler_ctx_t *ctx, unsigned off)
{
    instr_t *instr;

    for(instr = ctx->code->instrs+off; instr < ctx->code->instrs+ctx->code_off; instr++) {
        TRACE_(jscript_disas)("%d:\t%s", (int)(instr-ctx->code->instrs), instr_info[instr->op].op_str);
        if(instr_info[instr->op].arg1_type == ARG_DBL) {
            TRACE_(jscript_disas)("\t%lf", instr->u.dbl);
        }else {
            dump_instr_arg(instr_info[instr->op].arg1_type, instr->u.arg);
            dump_instr_arg(instr_info[instr->op].arg2_type, instr->u.arg+1);
        }
        TRACE_(jscript_disas)("\n");
    }
}

static HRESULT compile_expression(compiler_ctx_t*,expression_t*,BOOL);
static HRESULT compile_statement(compiler_ctx_t*,statement_ctx_t*,statement_t*);

static int function_local_cmp(const void *key, const struct wine_rb_entry *entry)
{
    function_local_t *local = WINE_RB_ENTRY_VALUE(entry, function_local_t, entry);
    return wcscmp(key, local->name);
}

static BOOL alloc_local_scope(compiler_ctx_t *ctx, unsigned int *scope_index)
{
    unsigned int scope, new_size;
    void *new_alloc;

    scope = ctx->local_scope_count++;
    if (scope == ctx->local_scope_size)
    {
        new_size = max(1, ctx->local_scope_size * 2);
        if (!(new_alloc = realloc(ctx->local_scopes, new_size * sizeof(*ctx->local_scopes))))
            return FALSE;
        ctx->local_scopes = new_alloc;
        ctx->local_scope_size = new_size;
    }

    ctx->local_scopes[scope].locals_cnt = 0;
    ctx->local_scopes[scope].ref_index = scope_index;
    wine_rb_init(&ctx->local_scopes[scope].locals, function_local_cmp);
    *scope_index = scope;

    return TRUE;
}

static void remove_local_scope(compiler_ctx_t *ctx, unsigned int scope_index)
{
    unsigned int i;

    assert(scope_index < ctx->local_scope_count);
    --ctx->local_scope_count;
    assert(scope_index == *ctx->local_scopes[scope_index].ref_index);
    *ctx->local_scopes[scope_index].ref_index = 0;
    memmove(&ctx->local_scopes[scope_index], &ctx->local_scopes[scope_index + 1],
            sizeof(*ctx->local_scopes) * (ctx->local_scope_count - scope_index));
    for (i = scope_index; i < ctx->local_scope_count; ++i)
        --*ctx->local_scopes[i].ref_index;
}

static inline void *compiler_alloc(bytecode_t *code, size_t size)
{
    return heap_pool_alloc(&code->heap, size);
}

jsstr_t *compiler_alloc_string_len(compiler_ctx_t *ctx, const WCHAR *str, unsigned len)
{
    jsstr_t *new_str;

    if(!ctx->code->str_pool_size) {
        ctx->code->str_pool = malloc(8 * sizeof(jsstr_t*));
        if(!ctx->code->str_pool)
            return NULL;
        ctx->code->str_pool_size = 8;
    }else if(ctx->code->str_pool_size == ctx->code->str_cnt) {
        jsstr_t **new_pool;

        new_pool = realloc(ctx->code->str_pool, ctx->code->str_pool_size*2*sizeof(jsstr_t*));
        if(!new_pool)
            return NULL;

        ctx->code->str_pool = new_pool;
        ctx->code->str_pool_size *= 2;
    }

    new_str = jsstr_alloc_len(str, len);
    if(!new_str)
        return NULL;

    ctx->code->str_pool[ctx->code->str_cnt++] = new_str;
    return new_str;
}

static jsstr_t *compiler_alloc_string(compiler_ctx_t *ctx, const WCHAR *str)
{
    return compiler_alloc_string_len(ctx, str, lstrlenW(str));
}

static BOOL ensure_bstr_slot(compiler_ctx_t *ctx)
{
    if(!ctx->code->bstr_pool_size) {
        ctx->code->bstr_pool = malloc(8 * sizeof(BSTR));
        if(!ctx->code->bstr_pool)
            return FALSE;
        ctx->code->bstr_pool_size = 8;
    }else if(ctx->code->bstr_pool_size == ctx->code->bstr_cnt) {
        BSTR *new_pool;

        new_pool = realloc(ctx->code->bstr_pool, ctx->code->bstr_pool_size*2*sizeof(BSTR));
        if(!new_pool)
            return FALSE;

        ctx->code->bstr_pool = new_pool;
        ctx->code->bstr_pool_size *= 2;
    }

    return TRUE;
}

static BSTR compiler_alloc_bstr(compiler_ctx_t *ctx, const WCHAR *str)
{
    if(!ensure_bstr_slot(ctx))
        return NULL;

    ctx->code->bstr_pool[ctx->code->bstr_cnt] = SysAllocString(str);
    if(!ctx->code->bstr_pool[ctx->code->bstr_cnt])
        return NULL;

    return ctx->code->bstr_pool[ctx->code->bstr_cnt++];
}

static BSTR compiler_alloc_bstr_len(compiler_ctx_t *ctx, const WCHAR *str, size_t len)
{
    if(!ensure_bstr_slot(ctx))
        return NULL;

    ctx->code->bstr_pool[ctx->code->bstr_cnt] = SysAllocStringLen(str, len);
    if(!ctx->code->bstr_pool[ctx->code->bstr_cnt])
        return NULL;

    return ctx->code->bstr_pool[ctx->code->bstr_cnt++];
}

void set_compiler_loc(compiler_ctx_t *ctx, unsigned loc)
{
    ctx->loc = loc;
}

static unsigned push_instr(compiler_ctx_t *ctx, jsop_t op)
{
    assert(ctx->code_size >= ctx->code_off);

    if(ctx->code_size == ctx->code_off) {
        instr_t *new_instrs;

        new_instrs = realloc(ctx->code->instrs, ctx->code_size*2*sizeof(instr_t));
        if(!new_instrs)
            return 0;

        ctx->code->instrs = new_instrs;
        ctx->code_size *= 2;
    }

    ctx->code->instrs[ctx->code_off].op = op;
    ctx->code->instrs[ctx->code_off].loc = ctx->loc;
    return ctx->code_off++;
}

static inline instr_t *instr_ptr(compiler_ctx_t *ctx, unsigned off)
{
    assert(off < ctx->code_off);
    return ctx->code->instrs + off;
}

static HRESULT push_instr_int(compiler_ctx_t *ctx, jsop_t op, LONG arg)
{
    unsigned instr;

    instr = push_instr(ctx, op);
    if(!instr)
        return E_OUTOFMEMORY;

    instr_ptr(ctx, instr)->u.arg->lng = arg;
    return S_OK;
}

static HRESULT push_instr_str(compiler_ctx_t *ctx, jsop_t op, jsstr_t *str)
{
    unsigned instr;

    instr = push_instr(ctx, op);
    if(!instr)
        return E_OUTOFMEMORY;

    instr_ptr(ctx, instr)->u.arg->str = str;
    return S_OK;
}

static HRESULT push_instr_str_uint(compiler_ctx_t *ctx, jsop_t op, jsstr_t *str, unsigned arg2)
{
    unsigned instr;

    instr = push_instr(ctx, op);
    if(!instr)
        return E_OUTOFMEMORY;

    instr_ptr(ctx, instr)->u.arg[0].str = str;
    instr_ptr(ctx, instr)->u.arg[1].uint = arg2;
    return S_OK;
}

static HRESULT push_instr_bstr(compiler_ctx_t *ctx, jsop_t op, const WCHAR *arg)
{
    unsigned instr;
    WCHAR *str;

    str = compiler_alloc_bstr(ctx, arg);
    if(!str)
        return E_OUTOFMEMORY;

    instr = push_instr(ctx, op);
    if(!instr)
        return E_OUTOFMEMORY;

    instr_ptr(ctx, instr)->u.arg->bstr = str;
    return S_OK;
}

static HRESULT push_instr_bstr_uint(compiler_ctx_t *ctx, jsop_t op, const WCHAR *arg1, unsigned arg2)
{
    unsigned instr;
    WCHAR *str;

    str = compiler_alloc_bstr(ctx, arg1);
    if(!str)
        return E_OUTOFMEMORY;

    instr = push_instr(ctx, op);
    if(!instr)
        return E_OUTOFMEMORY;

    instr_ptr(ctx, instr)->u.arg[0].bstr = str;
    instr_ptr(ctx, instr)->u.arg[1].uint = arg2;
    return S_OK;
}

static HRESULT push_instr_uint_str(compiler_ctx_t *ctx, jsop_t op, unsigned arg1, const WCHAR *arg2)
{
    unsigned instr;
    jsstr_t *str;

    str = compiler_alloc_string(ctx, arg2);
    if(!str)
        return E_OUTOFMEMORY;

    instr = push_instr(ctx, op);
    if(!instr)
        return E_OUTOFMEMORY;

    instr_ptr(ctx, instr)->u.arg[0].uint = arg1;
    instr_ptr(ctx, instr)->u.arg[1].str = str;
    return S_OK;
}

static HRESULT push_instr_double(compiler_ctx_t *ctx, jsop_t op, double arg)
{
    unsigned instr;

    instr = push_instr(ctx, op);
    if(!instr)
        return E_OUTOFMEMORY;

    instr_ptr(ctx, instr)->u.dbl = arg;
    return S_OK;
}

static inline void set_arg_uint(compiler_ctx_t *ctx, unsigned instr, unsigned arg)
{
    instr_ptr(ctx, instr)->u.arg->uint = arg;
}

static HRESULT push_instr_uint(compiler_ctx_t *ctx, jsop_t op, unsigned arg)
{
    unsigned instr;

    instr = push_instr(ctx, op);
    if(!instr)
        return E_OUTOFMEMORY;

    set_arg_uint(ctx, instr, arg);
    return S_OK;
}

static HRESULT compile_binary_expression(compiler_ctx_t *ctx, binary_expression_t *expr, jsop_t op)
{
    HRESULT hres;

    hres = compile_expression(ctx, expr->expression1, TRUE);
    if(FAILED(hres))
        return hres;

    hres = compile_expression(ctx, expr->expression2, TRUE);
    if(FAILED(hres))
        return hres;

    return push_instr(ctx, op) ? S_OK : E_OUTOFMEMORY;
}

static HRESULT compile_unary_expression(compiler_ctx_t *ctx, unary_expression_t *expr, jsop_t op)
{
    HRESULT hres;

    hres = compile_expression(ctx, expr->expression, TRUE);
    if(FAILED(hres))
        return hres;

    return push_instr(ctx, op) ? S_OK : E_OUTOFMEMORY;
}

/* ECMA-262 3rd Edition    11.2.1 */
static HRESULT compile_member_expression(compiler_ctx_t *ctx, member_expression_t *expr)
{
    HRESULT hres;

    hres = compile_expression(ctx, expr->expression, TRUE);
    if(FAILED(hres))
        return hres;

    return push_instr_bstr(ctx, OP_member, expr->identifier);
}

#define LABEL_FLAG 0x80000000

static unsigned alloc_label(compiler_ctx_t *ctx)
{
    if(!ctx->labels_size) {
        ctx->labels = malloc(8 * sizeof(*ctx->labels));
        if(!ctx->labels)
            return 0;
        ctx->labels_size = 8;
    }else if(ctx->labels_size == ctx->labels_cnt) {
        unsigned *new_labels;

        new_labels = realloc(ctx->labels, 2*ctx->labels_size*sizeof(*ctx->labels));
        if(!new_labels)
            return 0;

        ctx->labels = new_labels;
        ctx->labels_size *= 2;
    }

    return ctx->labels_cnt++ | LABEL_FLAG;
}

static void label_set_addr(compiler_ctx_t *ctx, unsigned label)
{
    assert(label & LABEL_FLAG);
    ctx->labels[label & ~LABEL_FLAG] = ctx->code_off;
}

static inline BOOL is_memberid_expr(expression_type_t type)
{
    return type == EXPR_IDENT || type == EXPR_MEMBER || type == EXPR_ARRAY;
}

static BOOL bind_local(compiler_ctx_t *ctx, const WCHAR *identifier, int *ret_ref)
{
    statement_ctx_t *iter;
    local_ref_t *ref;

    for(iter = ctx->stat_ctx; iter; iter = iter->next) {
        if(iter->using_scope)
        {
            if (!iter->block_scope)
                return FALSE;

            if ((ref = lookup_local(ctx->func, identifier, iter->scope_index)))
            {
                *ret_ref = ref->ref;
                return TRUE;
            }
        }
    }

    ref = lookup_local(ctx->func, identifier, 0);
    if(!ref)
        return FALSE;

    *ret_ref = ref->ref;
    return TRUE;
}

static HRESULT emit_identifier_ref(compiler_ctx_t *ctx, const WCHAR *identifier, unsigned flags)
{
    int local_ref;
    if(bind_local(ctx, identifier, &local_ref))
        return push_instr_int(ctx, OP_local_ref, local_ref);
    return push_instr_bstr_uint(ctx, OP_identid, identifier, flags);
}

static HRESULT emit_identifier(compiler_ctx_t *ctx, const WCHAR *identifier)
{
    int local_ref;
    if(bind_local(ctx, identifier, &local_ref))
        return push_instr_int(ctx, OP_local, local_ref);
    return push_instr_bstr(ctx, OP_ident, identifier);
}

static HRESULT emit_member_expression(compiler_ctx_t *ctx, expression_t *expr)
{
    HRESULT hres;

    if(expr->type == EXPR_ARRAY) {
        binary_expression_t *array_expr = (binary_expression_t*)expr;

        hres = compile_expression(ctx, array_expr->expression1, TRUE);
        if(FAILED(hres))
            return hres;

        hres = compile_expression(ctx, array_expr->expression2, TRUE);
        if(FAILED(hres))
            return hres;

        if(!push_instr(ctx, OP_to_string))
            return E_OUTOFMEMORY;
    }else {
        member_expression_t *member_expr = (member_expression_t*)expr;
        jsstr_t *jsstr;

        assert(expr->type == EXPR_MEMBER);

        hres = compile_expression(ctx, member_expr->expression, TRUE);
        if(FAILED(hres))
            return hres;

        jsstr = compiler_alloc_string(ctx, member_expr->identifier);
        if(!jsstr)
            return E_OUTOFMEMORY;

        hres = push_instr_str(ctx, OP_str, jsstr);
        if(FAILED(hres))
            return hres;
    }

    return S_OK;
}

static void push_compiler_statement_ctx(compiler_ctx_t *ctx, statement_ctx_t *stat_ctx)
{
    if (stat_ctx)
    {
        stat_ctx->next = ctx->stat_ctx;
        ctx->stat_ctx = stat_ctx;
    }
}

static void pop_compiler_statement_ctx(compiler_ctx_t *ctx, statement_ctx_t *stat_ctx)
{
    if (stat_ctx)
    {
        assert(ctx->stat_ctx == stat_ctx);
        ctx->stat_ctx = stat_ctx->next;
    }
}

static HRESULT compile_memberid_expression(compiler_ctx_t *ctx, expression_t *expr, unsigned flags)
{
    HRESULT hres;

    if(expr->type == EXPR_IDENT) {
        identifier_expression_t *ident_expr = (identifier_expression_t*)expr;
        return emit_identifier_ref(ctx, ident_expr->identifier, flags);
    }

    hres = emit_member_expression(ctx, expr);
    if(FAILED(hres))
        return hres;

    return push_instr_uint(ctx, OP_memberid, flags);
}

static HRESULT compile_increment_expression(compiler_ctx_t *ctx, unary_expression_t *expr, jsop_t op, int n)
{
    HRESULT hres;

    if(!is_memberid_expr(expr->expression->type)) {
        hres = compile_expression(ctx, expr->expression, TRUE);
        if(FAILED(hres))
            return hres;

        return push_instr_uint(ctx, OP_throw_ref, JS_E_ILLEGAL_ASSIGN);
    }

    hres = compile_memberid_expression(ctx, expr->expression, fdexNameEnsure);
    if(FAILED(hres))
        return hres;

    return push_instr_int(ctx, op, n);
}

/* ECMA-262 3rd Edition    11.14 */
static HRESULT compile_comma_expression(compiler_ctx_t *ctx, binary_expression_t *expr, BOOL emit_ret)
{
    HRESULT hres;

    hres = compile_expression(ctx, expr->expression1, FALSE);
    if(FAILED(hres))
        return hres;

    return compile_expression(ctx, expr->expression2, emit_ret);
}

/* ECMA-262 3rd Edition    11.11 */
static HRESULT compile_logical_expression(compiler_ctx_t *ctx, binary_expression_t *expr, jsop_t op)
{
    unsigned instr;
    HRESULT hres;

    hres = compile_expression(ctx, expr->expression1, TRUE);
    if(FAILED(hres))
        return hres;

    instr = push_instr(ctx, op);
    if(!instr)
        return E_OUTOFMEMORY;

    hres = compile_expression(ctx, expr->expression2, TRUE);
    if(FAILED(hres))
        return hres;

    set_arg_uint(ctx, instr, ctx->code_off);
    return S_OK;
}

/* ECMA-262 3rd Edition    11.12 */
static HRESULT compile_conditional_expression(compiler_ctx_t *ctx, conditional_expression_t *expr)
{
    unsigned jmp_false, jmp_end;
    HRESULT hres;

    hres = compile_expression(ctx, expr->expression, TRUE);
    if(FAILED(hres))
        return hres;

    jmp_false = push_instr(ctx, OP_cnd_z);
    if(!jmp_false)
        return E_OUTOFMEMORY;

    hres = compile_expression(ctx, expr->true_expression, TRUE);
    if(FAILED(hres))
        return hres;

    jmp_end = push_instr(ctx, OP_jmp);
    if(!jmp_end)
        return E_OUTOFMEMORY;

    set_arg_uint(ctx, jmp_false, ctx->code_off);
    hres = push_instr_uint(ctx, OP_pop, 1);
    if(FAILED(hres))
        return hres;

    hres = compile_expression(ctx, expr->false_expression, TRUE);
    if(FAILED(hres))
        return hres;

    set_arg_uint(ctx, jmp_end, ctx->code_off);
    return S_OK;
}

static HRESULT compile_new_expression(compiler_ctx_t *ctx, call_expression_t *expr)
{
    unsigned arg_cnt = 0;
    argument_t *arg;
    HRESULT hres;

    hres = compile_expression(ctx, expr->expression, TRUE);
    if(FAILED(hres))
        return hres;

    for(arg = expr->argument_list; arg; arg = arg->next) {
        hres = compile_expression(ctx, arg->expr, TRUE);
        if(FAILED(hres))
            return hres;
        arg_cnt++;
    }

    hres = push_instr_uint(ctx, OP_new, arg_cnt);
    if(FAILED(hres))
        return hres;

    hres = push_instr_uint(ctx, OP_pop, arg_cnt+1);
    if(FAILED(hres))
        return hres;

    return push_instr(ctx, OP_push_acc) ? S_OK : E_OUTOFMEMORY;
}

static HRESULT compile_call_expression(compiler_ctx_t *ctx, call_expression_t *expr, BOOL emit_ret)
{
    unsigned arg_cnt = 0, extra_args = 0;
    HRESULT hres = S_OK;
    argument_t *arg;
    unsigned instr;
    jsop_t op;

    if(is_memberid_expr(expr->expression->type)) {
        if(expr->expression->type == EXPR_IDENT && !wcscmp(((identifier_expression_t*)expr->expression)->identifier, L"eval"))
            op = OP_call_eval;
        else {
            op = OP_call_member;
            extra_args = 2;
            hres = compile_memberid_expression(ctx, expr->expression, 0);
        }
    }else {
        op = OP_call;
        extra_args = 1;
        hres = compile_expression(ctx, expr->expression, TRUE);
    }

    if(FAILED(hres))
        return hres;

    for(arg = expr->argument_list; arg; arg = arg->next) {
        hres = compile_expression(ctx, arg->expr, TRUE);
        if(FAILED(hres))
            return hres;
        arg_cnt++;
    }

    instr = push_instr(ctx, op);
    if(!instr)
        return E_OUTOFMEMORY;

    instr_ptr(ctx, instr)->u.arg[0].uint = arg_cnt;
    instr_ptr(ctx, instr)->u.arg[1].lng = emit_ret;

    hres = push_instr_uint(ctx, OP_pop, arg_cnt + extra_args);
    if(FAILED(hres))
        return hres;

    return !emit_ret || push_instr(ctx, OP_push_acc) ? S_OK : E_OUTOFMEMORY;
}

static HRESULT compile_delete_expression(compiler_ctx_t *ctx, unary_expression_t *expr)
{
    HRESULT hres;

    switch(expr->expression->type) {
    case EXPR_ARRAY: {
        binary_expression_t *array_expr = (binary_expression_t*)expr->expression;

        hres = compile_expression(ctx, array_expr->expression1, TRUE);
        if(FAILED(hres))
            return hres;

        hres = compile_expression(ctx, array_expr->expression2, TRUE);
        if(FAILED(hres))
            return hres;

        if(!push_instr(ctx, OP_delete))
            return E_OUTOFMEMORY;
        break;
    }
    case EXPR_MEMBER: {
        member_expression_t *member_expr = (member_expression_t*)expr->expression;
        jsstr_t *jsstr;

        hres = compile_expression(ctx, member_expr->expression, TRUE);
        if(FAILED(hres))
            return hres;

        /* FIXME: Potential optimization */
        jsstr = compiler_alloc_string(ctx, member_expr->identifier);
        if(!jsstr)
            return E_OUTOFMEMORY;

        hres = push_instr_str(ctx, OP_str, jsstr);
        if(FAILED(hres))
            return hres;

        if(!push_instr(ctx, OP_delete))
            return E_OUTOFMEMORY;
        break;
    }
    case EXPR_IDENT:
        return push_instr_bstr(ctx, OP_delete_ident, ((identifier_expression_t*)expr->expression)->identifier);
    default: {
        WARN("invalid delete, unimplemented exception message\n");

        hres = compile_expression(ctx, expr->expression, TRUE);
        if(FAILED(hres))
            return hres;

        return push_instr_uint_str(ctx, OP_throw_type, JS_E_INVALID_DELETE, L"FIXME");
    }
    }

    return S_OK;
}

static HRESULT compile_assign_expression(compiler_ctx_t *ctx, binary_expression_t *expr, jsop_t op)
{
    jsop_t assign_op = OP_throw_ref;
    unsigned arg_cnt = 0;
    HRESULT hres;

    if(expr->expression1->type == EXPR_CALL) {
        call_expression_t *call_expr = (call_expression_t*)expr->expression1;
        argument_t *arg;

        if(is_memberid_expr(call_expr->expression->type) && call_expr->argument_list) {
            hres = compile_memberid_expression(ctx, call_expr->expression, fdexNameEnsure);
            if(FAILED(hres))
                return hres;

            for(arg = call_expr->argument_list; arg; arg = arg->next) {
                hres = compile_expression(ctx, arg->expr, TRUE);
                if(FAILED(hres))
                    return hres;
                arg_cnt++;
            }

            if(op != OP_LAST) {
                unsigned instr;

                /* We need to call the functions twice: to get the value and to set it.
                 * JavaScript interpreted functions may to modify value on the stack,
                 * but assignment calls are allowed only on external functions, so we
                 * may reuse the stack here. */
                instr = push_instr(ctx, OP_call_member);
                if(!instr)
                    return E_OUTOFMEMORY;
                instr_ptr(ctx, instr)->u.arg[0].uint = arg_cnt;
                instr_ptr(ctx, instr)->u.arg[1].lng = 1;

                if(!push_instr(ctx, OP_push_acc))
                    return E_OUTOFMEMORY;
            }
            assign_op = OP_assign_call;
        }
    }else if(is_memberid_expr(expr->expression1->type)) {
        if(op != OP_LAST || expr->expression1->type == EXPR_IDENT) {
            hres = compile_memberid_expression(ctx, expr->expression1, fdexNameEnsure);
            if(FAILED(hres))
                return hres;
            if(op != OP_LAST && !push_instr(ctx, OP_refval))
                return E_OUTOFMEMORY;
            assign_op = OP_assign;
        }else {
            hres = emit_member_expression(ctx, expr->expression1);
            if(FAILED(hres))
                return hres;
            assign_op = OP_set_member;
        }
    }

    if(assign_op == OP_throw_ref) {
        /* Illegal assignment: evaluate and throw */
        hres = compile_expression(ctx, expr->expression1, TRUE);
        if(FAILED(hres))
            return hres;
        arg_cnt = JS_E_ILLEGAL_ASSIGN;
    }

    hres = compile_expression(ctx, expr->expression2, TRUE);
    if(FAILED(hres))
        return hres;

    if(op != OP_LAST && !push_instr(ctx, op))
        return E_OUTOFMEMORY;

    return push_instr_uint(ctx, assign_op, arg_cnt);
}

static HRESULT compile_typeof_expression(compiler_ctx_t *ctx, unary_expression_t *expr)
{
    jsop_t op;
    HRESULT hres;

    if(is_memberid_expr(expr->expression->type)) {
        if(expr->expression->type == EXPR_IDENT)
            return push_instr_bstr(ctx, OP_typeofident, ((identifier_expression_t*)expr->expression)->identifier);

        op = OP_typeofid;
        hres = compile_memberid_expression(ctx, expr->expression, 0);
    }else {
        op = OP_typeof;
        hres = compile_expression(ctx, expr->expression, TRUE);
    }
    if(FAILED(hres))
        return hres;

    return push_instr(ctx, op) ? S_OK : E_OUTOFMEMORY;
}

static HRESULT compile_literal(compiler_ctx_t *ctx, literal_t *literal)
{
    switch(literal->type) {
    case LT_BOOL:
        return push_instr_int(ctx, OP_bool, literal->u.bval);
    case LT_DOUBLE:
        return push_instr_double(ctx, OP_double, literal->u.dval);
    case LT_NULL:
        return push_instr(ctx, OP_null) ? S_OK : E_OUTOFMEMORY;
    case LT_STRING:
        return push_instr_str(ctx, OP_str, literal->u.str);
    case LT_REGEXP:
        return push_instr_str_uint(ctx, OP_regexp, literal->u.regexp.str, literal->u.regexp.flags);
    DEFAULT_UNREACHABLE;
    }
    return E_FAIL;
}

static HRESULT literal_as_string(compiler_ctx_t *ctx, literal_t *literal, jsstr_t **str)
{
    switch(literal->type) {
    case LT_STRING:
        *str = literal->u.str;
        break;
    case LT_DOUBLE:
        return double_to_string(literal->u.dval, str);
    DEFAULT_UNREACHABLE;
    }

    return *str ? S_OK : E_OUTOFMEMORY;
}

static HRESULT compile_array_literal(compiler_ctx_t *ctx, array_literal_expression_t *expr)
{
    unsigned length = 0;
    array_element_t *iter;
    unsigned array_instr;
    HRESULT hres;

    array_instr = push_instr(ctx, OP_carray);

    for(iter = expr->element_list; iter; iter = iter->next) {
        length += iter->elision;

        hres = compile_expression(ctx, iter->expr, TRUE);
        if(FAILED(hres))
            return hres;

        hres = push_instr_uint(ctx, OP_carray_set, length);
        if(FAILED(hres))
            return hres;

        length++;
    }

    instr_ptr(ctx, array_instr)->u.arg[0].uint = length + expr->length;
    return S_OK;
}

static HRESULT compile_object_literal(compiler_ctx_t *ctx, property_value_expression_t *expr)
{
    property_definition_t *iter;
    jsstr_t *name;
    HRESULT hres;

    if(!push_instr(ctx, OP_new_obj))
        return E_OUTOFMEMORY;

    for(iter = expr->property_list; iter; iter = iter->next) {
        hres = literal_as_string(ctx, iter->name, &name);
        if(FAILED(hres))
            return hres;

        hres = compile_expression(ctx, iter->value, TRUE);
        if(FAILED(hres))
            return hres;

        hres = push_instr_str_uint(ctx, OP_obj_prop, name, iter->type);
        if(FAILED(hres))
            return hres;
    }

    return S_OK;
}

static HRESULT compile_function_expression(compiler_ctx_t *ctx, function_expression_t *expr, BOOL emit_ret)
{
    statement_ctx_t *stat_ctx;

    assert(ctx->current_function_expr);

    for(stat_ctx = ctx->stat_ctx; stat_ctx; stat_ctx = stat_ctx->next)
    {
        if(stat_ctx->block_scope)
            break;
    }
    ctx->current_function_expr->scope_index = stat_ctx ? stat_ctx->scope_index : 0;
    ctx->current_function_expr = ctx->current_function_expr->next;

    return emit_ret ? push_instr_uint(ctx, OP_func, expr->func_id) : S_OK;
}

static HRESULT compile_expression(compiler_ctx_t *ctx, expression_t *expr, BOOL emit_ret)
{
    HRESULT hres;

    switch(expr->type) {
    case EXPR_ADD:
        hres = compile_binary_expression(ctx, (binary_expression_t*)expr, OP_add);
        break;
    case EXPR_AND:
        hres = compile_logical_expression(ctx, (binary_expression_t*)expr, OP_cnd_z);
        break;
    case EXPR_ARRAY:
        hres = compile_binary_expression(ctx, (binary_expression_t*)expr, OP_array);
        break;
    case EXPR_ARRAYLIT:
        hres = compile_array_literal(ctx, (array_literal_expression_t*)expr);
        break;
    case EXPR_ASSIGN:
        hres = compile_assign_expression(ctx, (binary_expression_t*)expr, OP_LAST);
        break;
    case EXPR_ASSIGNADD:
        hres = compile_assign_expression(ctx, (binary_expression_t*)expr, OP_add);
        break;
    case EXPR_ASSIGNAND:
        hres = compile_assign_expression(ctx, (binary_expression_t*)expr, OP_and);
        break;
    case EXPR_ASSIGNSUB:
        hres = compile_assign_expression(ctx, (binary_expression_t*)expr, OP_sub);
        break;
    case EXPR_ASSIGNMUL:
        hres = compile_assign_expression(ctx, (binary_expression_t*)expr, OP_mul);
        break;
    case EXPR_ASSIGNDIV:
        hres = compile_assign_expression(ctx, (binary_expression_t*)expr, OP_div);
        break;
    case EXPR_ASSIGNMOD:
        hres = compile_assign_expression(ctx, (binary_expression_t*)expr, OP_mod);
        break;
    case EXPR_ASSIGNOR:
        hres = compile_assign_expression(ctx, (binary_expression_t*)expr, OP_or);
        break;
    case EXPR_ASSIGNLSHIFT:
        hres = compile_assign_expression(ctx, (binary_expression_t*)expr, OP_lshift);
        break;
    case EXPR_ASSIGNRSHIFT:
        hres = compile_assign_expression(ctx, (binary_expression_t*)expr, OP_rshift);
        break;
    case EXPR_ASSIGNRRSHIFT:
        hres = compile_assign_expression(ctx, (binary_expression_t*)expr, OP_rshift2);
        break;
    case EXPR_ASSIGNXOR:
        hres = compile_assign_expression(ctx, (binary_expression_t*)expr, OP_xor);
        break;
    case EXPR_BAND:
        hres = compile_binary_expression(ctx, (binary_expression_t*)expr, OP_and);
        break;
    case EXPR_BITNEG:
        hres = compile_unary_expression(ctx, (unary_expression_t*)expr, OP_bneg);
        break;
    case EXPR_BOR:
        hres = compile_binary_expression(ctx, (binary_expression_t*)expr, OP_or);
        break;
    case EXPR_CALL:
        return compile_call_expression(ctx, (call_expression_t*)expr, emit_ret);
    case EXPR_COMMA:
        return compile_comma_expression(ctx, (binary_expression_t*)expr, emit_ret);
    case EXPR_COND:
        hres = compile_conditional_expression(ctx, (conditional_expression_t*)expr);
        break;
    case EXPR_DELETE:
        hres = compile_delete_expression(ctx, (unary_expression_t*)expr);
        break;
    case EXPR_DIV:
        hres = compile_binary_expression(ctx, (binary_expression_t*)expr, OP_div);
        break;
    case EXPR_EQ:
        hres = compile_binary_expression(ctx, (binary_expression_t*)expr, OP_eq);
        break;
    case EXPR_EQEQ:
        hres = compile_binary_expression(ctx, (binary_expression_t*)expr, OP_eq2);
        break;
    case EXPR_FUNC:
        return compile_function_expression(ctx, (function_expression_t*)expr, emit_ret);
    case EXPR_GREATER:
        hres = compile_binary_expression(ctx, (binary_expression_t*)expr, OP_gt);
        break;
    case EXPR_GREATEREQ:
        hres = compile_binary_expression(ctx, (binary_expression_t*)expr, OP_gteq);
        break;
    case EXPR_IDENT:
        hres = emit_identifier(ctx, ((identifier_expression_t*)expr)->identifier);
        break;
    case EXPR_IN:
        hres = compile_binary_expression(ctx, (binary_expression_t*)expr, OP_in);
        break;
    case EXPR_INSTANCEOF:
        hres = compile_binary_expression(ctx, (binary_expression_t*)expr, OP_instanceof);
        break;
    case EXPR_LESS:
        hres = compile_binary_expression(ctx, (binary_expression_t*)expr, OP_lt);
        break;
    case EXPR_LESSEQ:
        hres = compile_binary_expression(ctx, (binary_expression_t*)expr, OP_lteq);
        break;
    case EXPR_LITERAL:
        hres = compile_literal(ctx, ((literal_expression_t*)expr)->literal);
        break;
    case EXPR_LOGNEG:
        hres = compile_unary_expression(ctx, (unary_expression_t*)expr, OP_neg);
        break;
    case EXPR_LSHIFT:
        hres = compile_binary_expression(ctx, (binary_expression_t*)expr, OP_lshift);
        break;
    case EXPR_MEMBER:
        hres = compile_member_expression(ctx, (member_expression_t*)expr);
        break;
    case EXPR_MINUS:
        hres = compile_unary_expression(ctx, (unary_expression_t*)expr, OP_minus);
        break;
    case EXPR_MOD:
        hres = compile_binary_expression(ctx, (binary_expression_t*)expr, OP_mod);
        break;
    case EXPR_MUL:
        hres = compile_binary_expression(ctx, (binary_expression_t*)expr, OP_mul);
        break;
    case EXPR_NEW:
        hres = compile_new_expression(ctx, (call_expression_t*)expr);
        break;
    case EXPR_NOTEQ:
        hres = compile_binary_expression(ctx, (binary_expression_t*)expr, OP_neq);
        break;
    case EXPR_NOTEQEQ:
        hres = compile_binary_expression(ctx, (binary_expression_t*)expr, OP_neq2);
        break;
    case EXPR_OR:
        hres = compile_logical_expression(ctx, (binary_expression_t*)expr, OP_cnd_nz);
        break;
    case EXPR_PLUS:
        hres = compile_unary_expression(ctx, (unary_expression_t*)expr, OP_tonum);
        break;
    case EXPR_POSTDEC:
        hres = compile_increment_expression(ctx, (unary_expression_t*)expr, OP_postinc, -1);
        break;
    case EXPR_POSTINC:
        hres = compile_increment_expression(ctx, (unary_expression_t*)expr, OP_postinc, 1);
        break;
    case EXPR_PREDEC:
        hres = compile_increment_expression(ctx, (unary_expression_t*)expr, OP_preinc, -1);
        break;
    case EXPR_PREINC:
        hres = compile_increment_expression(ctx, (unary_expression_t*)expr, OP_preinc, 1);
        break;
    case EXPR_PROPVAL:
        hres = compile_object_literal(ctx, (property_value_expression_t*)expr);
        break;
    case EXPR_RSHIFT:
        hres = compile_binary_expression(ctx, (binary_expression_t*)expr, OP_rshift);
        break;
    case EXPR_RRSHIFT:
        hres = compile_binary_expression(ctx, (binary_expression_t*)expr, OP_rshift2);
        break;
    case EXPR_SUB:
        hres = compile_binary_expression(ctx, (binary_expression_t*)expr, OP_sub);
        break;
    case EXPR_THIS:
        return !emit_ret || push_instr(ctx, OP_this) ? S_OK : E_OUTOFMEMORY;
    case EXPR_TYPEOF:
        hres = compile_typeof_expression(ctx, (unary_expression_t*)expr);
        break;
    case EXPR_VOID:
        hres = compile_unary_expression(ctx, (unary_expression_t*)expr, OP_void);
        break;
    case EXPR_BXOR:
        hres = compile_binary_expression(ctx, (binary_expression_t*)expr, OP_xor);
        break;
    DEFAULT_UNREACHABLE;
    }

    if(FAILED(hres))
        return hres;

    return emit_ret ? S_OK : push_instr_uint(ctx, OP_pop, 1);
}

static inline BOOL is_loop_statement(statement_type_t type)
{
    return type == STAT_FOR || type == STAT_FORIN || type == STAT_WHILE;
}

/* ECMA-262 3rd Edition    12.1 */
static HRESULT compile_block_statement(compiler_ctx_t *ctx, block_statement_t *block, statement_t *iter)
{
    statement_ctx_t stat_ctx = {0, TRUE};
    BOOL needs_scope;
    HRESULT hres;

    needs_scope = block && block->scope_index;
    if (needs_scope)
    {
        if(FAILED(hres = push_instr_uint(ctx, OP_push_block_scope, block->scope_index)))
            return hres;

        stat_ctx.scope_index = block->scope_index;
        stat_ctx.block_scope = TRUE;
    }

    while(iter) {
        hres = compile_statement(ctx, needs_scope ? &stat_ctx : NULL, iter);
        if(FAILED(hres))
            return hres;

        iter = iter->next;
    }

    if(needs_scope && !push_instr(ctx, OP_pop_scope))
        return E_OUTOFMEMORY;

    return S_OK;
}

/* ECMA-262 3rd Edition    12.2 */
static HRESULT compile_variable_list(compiler_ctx_t *ctx, variable_declaration_t *list)
{
    variable_declaration_t *iter;
    HRESULT hres;

    assert(list != NULL);

    for(iter = list; iter; iter = iter->next) {
        if(!iter->expr)
            continue;

        if (iter->constant)
            FIXME("Constant variables are not supported.\n");

        hres = emit_identifier_ref(ctx, iter->identifier, 0);
        if(FAILED(hres))
            return hres;

        hres = compile_expression(ctx, iter->expr, TRUE);
        if(FAILED(hres))
            return hres;

        if(!push_instr(ctx, OP_assign))
            return E_OUTOFMEMORY;

        hres = push_instr_uint(ctx, OP_pop, 1);
        if(FAILED(hres))
            return hres;
    }

    return S_OK;
}

/* ECMA-262 3rd Edition    12.2 */
static HRESULT compile_var_statement(compiler_ctx_t *ctx, var_statement_t *stat)
{
    return compile_variable_list(ctx, stat->variable_list);
}

/* ECMA-262 3rd Edition    12.4 */
static HRESULT compile_expression_statement(compiler_ctx_t *ctx, expression_statement_t *stat)
{
    HRESULT hres;

    hres = compile_expression(ctx, stat->expr, ctx->from_eval);
    if(FAILED(hres))
        return hres;

    return !ctx->from_eval || push_instr(ctx, OP_setret) ? S_OK : E_OUTOFMEMORY;
}

/* ECMA-262 3rd Edition    12.5 */
static HRESULT compile_if_statement(compiler_ctx_t *ctx, if_statement_t *stat)
{
    unsigned jmp_else;
    HRESULT hres;

    hres = compile_expression(ctx, stat->expr, TRUE);
    if(FAILED(hres))
        return hres;

    jmp_else = push_instr(ctx, OP_jmp_z);
    if(!jmp_else)
        return E_OUTOFMEMORY;

    hres = compile_statement(ctx, NULL, stat->if_stat);
    if(FAILED(hres))
        return hres;

    if(stat->else_stat) {
        unsigned jmp_end;

        jmp_end = push_instr(ctx, OP_jmp);
        if(!jmp_end)
            return E_OUTOFMEMORY;

        set_arg_uint(ctx, jmp_else, ctx->code_off);

        hres = compile_statement(ctx, NULL, stat->else_stat);
        if(FAILED(hres))
            return hres;

        set_arg_uint(ctx, jmp_end, ctx->code_off);
    }else {
        set_arg_uint(ctx, jmp_else, ctx->code_off);
    }

    return S_OK;
}

/* ECMA-262 3rd Edition    12.6.2 */
static HRESULT compile_while_statement(compiler_ctx_t *ctx, while_statement_t *stat)
{
    statement_ctx_t stat_ctx = {0, FALSE, FALSE};
    unsigned jmp_off;
    HRESULT hres;

    stat_ctx.break_label = alloc_label(ctx);
    if(!stat_ctx.break_label)
        return E_OUTOFMEMORY;

    stat_ctx.continue_label = alloc_label(ctx);
    if(!stat_ctx.continue_label)
        return E_OUTOFMEMORY;

    jmp_off = ctx->code_off;

    if(!stat->do_while) {
        label_set_addr(ctx, stat_ctx.continue_label);
        hres = compile_expression(ctx, stat->expr, TRUE);
        if(FAILED(hres))
            return hres;

        hres = push_instr_uint(ctx, OP_jmp_z, stat_ctx.break_label);
        if(FAILED(hres))
            return hres;
    }

    hres = compile_statement(ctx, &stat_ctx, stat->statement);
    if(FAILED(hres))
        return hres;

    set_compiler_loc(ctx, stat->stat.loc);
    if(stat->do_while) {
        label_set_addr(ctx, stat_ctx.continue_label);
        hres = compile_expression(ctx, stat->expr, TRUE);
        if(FAILED(hres))
            return hres;

        hres = push_instr_uint(ctx, OP_jmp_z, stat_ctx.break_label);
        if(FAILED(hres))
            return hres;
    }

    hres = push_instr_uint(ctx, OP_jmp, jmp_off);
    if(FAILED(hres))
        return hres;

    label_set_addr(ctx, stat_ctx.break_label);
    return S_OK;
}

/* ECMA-262 10th Edition   13.7.4 */
static HRESULT compile_for_statement(compiler_ctx_t *ctx, for_statement_t *stat)
{
    statement_ctx_t stat_ctx = {0, FALSE, FALSE};
    statement_ctx_t scope_stat_ctx = {0, TRUE};
    unsigned expr_off;
    HRESULT hres;

    if (stat->scope_index)
    {
        if(FAILED(hres = push_instr_uint(ctx, OP_push_block_scope, stat->scope_index)))
            return hres;

        scope_stat_ctx.scope_index = stat->scope_index;
        scope_stat_ctx.block_scope = TRUE;
        push_compiler_statement_ctx(ctx, &scope_stat_ctx);
    }

    if(stat->variable_list) {
        hres = compile_variable_list(ctx, stat->variable_list);
        if(FAILED(hres))
            goto done;
    }else if(stat->begin_expr) {
        hres = compile_expression(ctx, stat->begin_expr, FALSE);
        if(FAILED(hres))
            goto done;
    }

    stat_ctx.break_label = alloc_label(ctx);
    if(!stat_ctx.break_label)
    {
        hres = E_OUTOFMEMORY;
        goto done;
    }

    stat_ctx.continue_label = alloc_label(ctx);
    if(!stat_ctx.continue_label)
    {
        hres = E_OUTOFMEMORY;
        goto done;
    }
    expr_off = ctx->code_off;

    if(stat->expr) {
        set_compiler_loc(ctx, stat->expr_loc);
        hres = compile_expression(ctx, stat->expr, TRUE);
        if(FAILED(hres))
            goto done;

        hres = push_instr_uint(ctx, OP_jmp_z, stat_ctx.break_label);
        if(FAILED(hres))
            goto done;
    }

    hres = compile_statement(ctx, &stat_ctx, stat->statement);
    if(FAILED(hres))
        goto done;

    label_set_addr(ctx, stat_ctx.continue_label);

    if(stat->end_expr) {
        set_compiler_loc(ctx, stat->end_loc);
        hres = compile_expression(ctx, stat->end_expr, FALSE);
        if(FAILED(hres))
            goto done;
    }

    hres = push_instr_uint(ctx, OP_jmp, expr_off);
    if(FAILED(hres))
        goto done;

    label_set_addr(ctx, stat_ctx.break_label);
    hres = S_OK;
done:
    if (stat->scope_index)
    {
        pop_compiler_statement_ctx(ctx, &scope_stat_ctx);
        if(SUCCEEDED(hres) && !push_instr(ctx, OP_pop_scope))
            return E_OUTOFMEMORY;
    }
    return hres;
}

/* ECMA-262 3rd Edition    12.6.4 */
static HRESULT compile_forin_statement(compiler_ctx_t *ctx, forin_statement_t *stat)
{
    statement_ctx_t stat_ctx = {4, FALSE, FALSE};
    HRESULT hres;

    if(stat->variable) {
        hres = compile_variable_list(ctx, stat->variable);
        if(FAILED(hres))
            return hres;
    }

    stat_ctx.break_label = alloc_label(ctx);
    if(!stat_ctx.break_label)
        return E_OUTOFMEMORY;

    stat_ctx.continue_label = alloc_label(ctx);
    if(!stat_ctx.continue_label)
        return E_OUTOFMEMORY;

    hres = compile_expression(ctx, stat->in_expr, TRUE);
    if(FAILED(hres))
        return hres;

    if(stat->variable) {
        hres = emit_identifier_ref(ctx, stat->variable->identifier, fdexNameEnsure);
        if(FAILED(hres))
            return hres;
    }else if(is_memberid_expr(stat->expr->type)) {
        hres = compile_memberid_expression(ctx, stat->expr, fdexNameEnsure);
        if(FAILED(hres))
            return hres;
    }else {
        hres = push_instr_uint(ctx, OP_throw_ref, JS_E_ILLEGAL_ASSIGN);
        if(FAILED(hres))
            return hres;

        /* FIXME: compile statement anyways when we depend on compiler to check errors */
        return S_OK;
    }

    hres = push_instr_int(ctx, OP_int, DISPID_STARTENUM);
    if(FAILED(hres))
        return hres;

    label_set_addr(ctx, stat_ctx.continue_label);
    hres = push_instr_uint(ctx, OP_forin, stat_ctx.break_label);
    if(FAILED(hres))
        return E_OUTOFMEMORY;

    hres = compile_statement(ctx, &stat_ctx, stat->statement);
    if(FAILED(hres))
        return hres;

    hres = push_instr_uint(ctx, OP_jmp, stat_ctx.continue_label);
    if(FAILED(hres))
        return hres;

    label_set_addr(ctx, stat_ctx.break_label);
    return S_OK;
}

static HRESULT pop_to_stat(compiler_ctx_t *ctx, statement_ctx_t *stat_ctx)
{
    unsigned stack_pop = 0;
    statement_ctx_t *iter;
    HRESULT hres;

    for(iter = ctx->stat_ctx; iter != stat_ctx; iter = iter->next) {
        if(iter->using_scope && !push_instr(ctx, OP_pop_scope))
            return E_OUTOFMEMORY;
        if(iter->using_except) {
            if(stack_pop) {
                hres = push_instr_uint(ctx, OP_pop, stack_pop);
                if(FAILED(hres))
                    return hres;
                stack_pop = 0;
            }
            hres = push_instr_uint(ctx, OP_pop_except, ctx->code_off+1);
            if(FAILED(hres))
                return hres;
        }
        stack_pop += iter->stack_use;
    }

    if(stack_pop) {
        hres = push_instr_uint(ctx, OP_pop, stack_pop);
        if(FAILED(hres))
            return hres;
    }

    return S_OK;
}

/* ECMA-262 3rd Edition    12.7 */
static HRESULT compile_continue_statement(compiler_ctx_t *ctx, branch_statement_t *stat)
{
    statement_ctx_t *pop_ctx;
    HRESULT hres;

    if(stat->identifier) {
        statement_t *label_stat;
        statement_ctx_t *iter;

        pop_ctx = NULL;

        for(iter = ctx->stat_ctx; iter; iter = iter->next) {
            if(iter->continue_label)
                pop_ctx = iter;
            if(iter->labelled_stat && !wcscmp(iter->labelled_stat->identifier, stat->identifier))
                break;
        }

        if(!iter) {
            WARN("Label not found\n");
            return JS_E_LABEL_NOT_FOUND;
        }

        /* Labelled continue are allowed only on loops */
        for(label_stat = iter->labelled_stat->statement;
            label_stat->type == STAT_LABEL;
            label_stat = ((labelled_statement_t*)label_stat)->statement);
        if(!is_loop_statement(label_stat->type)) {
            WARN("Label is not a loop\n");
            return JS_E_INVALID_CONTINUE;
        }

        assert(pop_ctx != NULL);
    }else {
        for(pop_ctx = ctx->stat_ctx; pop_ctx; pop_ctx = pop_ctx->next) {
            if(pop_ctx->continue_label)
                break;
        }

        if(!pop_ctx) {
            WARN("continue outside loop\n");
            return JS_E_INVALID_CONTINUE;
        }
    }

    hres = pop_to_stat(ctx, pop_ctx);
    if(FAILED(hres))
        return hres;

    return push_instr_uint(ctx, OP_jmp, pop_ctx->continue_label);
}

/* ECMA-262 3rd Edition    12.8 */
static HRESULT compile_break_statement(compiler_ctx_t *ctx, branch_statement_t *stat)
{
    statement_ctx_t *pop_ctx;
    HRESULT hres;

    if(stat->identifier) {
        for(pop_ctx = ctx->stat_ctx; pop_ctx; pop_ctx = pop_ctx->next) {
            if(pop_ctx->labelled_stat && !wcscmp(pop_ctx->labelled_stat->identifier, stat->identifier)) {
                assert(pop_ctx->break_label);
                break;
            }
        }

        if(!pop_ctx) {
            WARN("Label not found\n");
            return JS_E_LABEL_NOT_FOUND;
        }
    }else {
        for(pop_ctx = ctx->stat_ctx; pop_ctx; pop_ctx = pop_ctx->next) {
            if(pop_ctx->break_label && !pop_ctx->labelled_stat)
                break;
        }

        if(!pop_ctx) {
            WARN("Break outside loop\n");
            return JS_E_INVALID_BREAK;
        }
    }

    hres = pop_to_stat(ctx, pop_ctx->next);
    if(FAILED(hres))
        return hres;

    return push_instr_uint(ctx, OP_jmp, pop_ctx->break_label);
}

/* ECMA-262 3rd Edition    12.9 */
static HRESULT compile_return_statement(compiler_ctx_t *ctx, expression_statement_t *stat)
{
    HRESULT hres;

    if(ctx->from_eval) {
        WARN("misplaced return statement\n");
        return JS_E_MISPLACED_RETURN;
    }

    if(stat->expr) {
        hres = compile_expression(ctx, stat->expr, TRUE);
        if(FAILED(hres))
            return hres;
        if(!push_instr(ctx, OP_setret))
            return E_OUTOFMEMORY;
    }

    hres = pop_to_stat(ctx, NULL);
    if(FAILED(hres))
        return hres;

    return push_instr_uint(ctx, OP_ret, !stat->expr);
}

/* ECMA-262 3rd Edition    12.10 */
static HRESULT compile_with_statement(compiler_ctx_t *ctx, with_statement_t *stat)
{
    statement_ctx_t stat_ctx = {0, TRUE, FALSE};
    HRESULT hres;

    hres = compile_expression(ctx, stat->expr, TRUE);
    if(FAILED(hres))
        return hres;

    if(!push_instr(ctx, OP_push_with_scope))
        return E_OUTOFMEMORY;

    hres = compile_statement(ctx, &stat_ctx, stat->statement);
    if(FAILED(hres))
        return hres;

    if(!push_instr(ctx, OP_pop_scope))
        return E_OUTOFMEMORY;

    return S_OK;
}

/* ECMA-262 3rd Edition    12.10 */
static HRESULT compile_labelled_statement(compiler_ctx_t *ctx, labelled_statement_t *stat)
{
    statement_ctx_t stat_ctx = {0, FALSE, FALSE, 0, 0, stat}, *iter;
    HRESULT hres;

    for(iter = ctx->stat_ctx; iter; iter = iter->next) {
        if(iter->labelled_stat && !wcscmp(iter->labelled_stat->identifier, stat->identifier)) {
            WARN("Label %s redefined\n", debugstr_w(stat->identifier));
            return JS_E_LABEL_REDEFINED;
        }
    }

    /* Labelled breaks are allowed for any labelled statements, not only loops (violating spec) */
    stat_ctx.break_label = alloc_label(ctx);
    if(!stat_ctx.break_label)
        return E_OUTOFMEMORY;

    hres = compile_statement(ctx, &stat_ctx, stat->statement);
    if(FAILED(hres))
        return hres;

    label_set_addr(ctx, stat_ctx.break_label);
    return S_OK;
}

/* ECMA-262 3rd Edition    12.13 */
static HRESULT compile_switch_statement(compiler_ctx_t *ctx, switch_statement_t *stat)
{
    statement_ctx_t stat_ctx = {0, FALSE, FALSE};
    unsigned case_cnt = 0, *case_jmps, i, default_jmp;
    BOOL have_default = FALSE;
    statement_t *stat_iter;
    case_clausule_t *iter;
    HRESULT hres;

    hres = compile_expression(ctx, stat->expr, TRUE);
    if(FAILED(hres))
        return hres;

    stat_ctx.break_label = alloc_label(ctx);
    if(!stat_ctx.break_label)
        return E_OUTOFMEMORY;

    for(iter = stat->case_list; iter; iter = iter->next) {
        if(iter->expr)
            case_cnt++;
    }

    case_jmps = malloc(case_cnt * sizeof(*case_jmps));
    if(!case_jmps)
        return E_OUTOFMEMORY;

    i = 0;
    for(iter = stat->case_list; iter; iter = iter->next) {
        if(!iter->expr) {
            have_default = TRUE;
            continue;
        }

        set_compiler_loc(ctx, iter->loc);
        hres = compile_expression(ctx, iter->expr, TRUE);
        if(FAILED(hres))
            break;

        case_jmps[i] = push_instr(ctx, OP_case);
        if(!case_jmps[i]) {
            hres = E_OUTOFMEMORY;
            break;
        }
        i++;
    }

    if(SUCCEEDED(hres)) {
        hres = push_instr_uint(ctx, OP_pop, 1);
        if(SUCCEEDED(hres)) {
            default_jmp = push_instr(ctx, OP_jmp);
            if(!default_jmp)
                hres = E_OUTOFMEMORY;
        }
    }

    if(FAILED(hres)) {
        free(case_jmps);
        return hres;
    }

    i = 0;
    for(iter = stat->case_list; iter; iter = iter->next) {
        while(iter->next && iter->next->stat == iter->stat) {
            set_arg_uint(ctx, iter->expr ? case_jmps[i++] : default_jmp, ctx->code_off);
            iter = iter->next;
        }

        set_arg_uint(ctx, iter->expr ? case_jmps[i++] : default_jmp, ctx->code_off);

        for(stat_iter = iter->stat; stat_iter && (!iter->next || iter->next->stat != stat_iter);
            stat_iter = stat_iter->next) {
            hres = compile_statement(ctx, &stat_ctx, stat_iter);
            if(FAILED(hres))
                break;
        }
        if(FAILED(hres))
            break;
    }

    free(case_jmps);
    if(FAILED(hres))
        return hres;
    assert(i == case_cnt);

    if(!have_default) {
        hres = push_instr_uint(ctx, OP_jmp, stat_ctx.break_label);
        if(FAILED(hres))
            return hres;
        set_arg_uint(ctx, default_jmp, ctx->code_off);
    }

    label_set_addr(ctx, stat_ctx.break_label);
    return S_OK;
}

/* ECMA-262 3rd Edition    12.13 */
static HRESULT compile_throw_statement(compiler_ctx_t *ctx, expression_statement_t *stat)
{
    HRESULT hres;

    hres = compile_expression(ctx, stat->expr, TRUE);
    if(FAILED(hres))
        return hres;

    return push_instr(ctx, OP_throw) ? S_OK : E_OUTOFMEMORY;
}

/* ECMA-262 3rd Edition    12.14 */
static HRESULT compile_try_statement(compiler_ctx_t *ctx, try_statement_t *stat)
{
    statement_ctx_t try_ctx = {0, FALSE, TRUE}, finally_ctx = {2, FALSE, FALSE};
    unsigned push_except, finally_off = 0, catch_off = 0, pop_except, catch_pop_except = 0;
    BSTR ident;
    HRESULT hres;

    push_except = push_instr(ctx, OP_push_except);
    if(!push_except)
        return E_OUTOFMEMORY;

    if(stat->catch_block) {
        ident = compiler_alloc_bstr(ctx, stat->catch_block->identifier);
        if(!ident)
            return E_OUTOFMEMORY;
    }else {
        ident = NULL;
    }

    hres = compile_statement(ctx, &try_ctx, stat->try_statement);
    if(FAILED(hres))
        return hres;

    pop_except = push_instr(ctx, OP_pop_except);
    if(!pop_except)
        return E_OUTOFMEMORY;

    if(stat->catch_block) {
        statement_ctx_t catch_ctx = {0, TRUE, stat->finally_statement != NULL};

        if(stat->finally_statement)
            catch_ctx.using_except = TRUE;

        catch_off = ctx->code_off;

        hres = push_instr_bstr(ctx, OP_enter_catch, ident);
        if(FAILED(hres))
            return hres;

        hres = compile_statement(ctx, &catch_ctx, stat->catch_block->statement);
        if(FAILED(hres))
            return hres;

        if(!push_instr(ctx, OP_pop_scope))
            return E_OUTOFMEMORY;

        if(stat->finally_statement) {
            catch_pop_except = push_instr(ctx, OP_pop_except);
            if(!catch_pop_except)
                return E_OUTOFMEMORY;
        }
    }

    if(stat->finally_statement) {
        /*
         * finally block expects two elements on the stack, which may be:
         * - (true, return_addr) set by OP_pop_except, OP_end_finally jumps back to passed address
         * - (false, exception_value) set when unwinding an exception, which OP_end_finally rethrows
         */
        finally_off = ctx->code_off;
        hres = compile_statement(ctx, &finally_ctx, stat->finally_statement);
        if(FAILED(hres))
            return hres;

        set_compiler_loc(ctx, stat->finally_loc);
        if(!push_instr(ctx, OP_end_finally))
            return E_OUTOFMEMORY;
    }

    instr_ptr(ctx, pop_except)->u.arg[0].uint = ctx->code_off;
    if(catch_pop_except)
        instr_ptr(ctx, catch_pop_except)->u.arg[0].uint = ctx->code_off;
    instr_ptr(ctx, push_except)->u.arg[0].uint = catch_off;
    instr_ptr(ctx, push_except)->u.arg[1].uint = finally_off;
    return S_OK;
}

static HRESULT compile_statement(compiler_ctx_t *ctx, statement_ctx_t *stat_ctx, statement_t *stat)
{
    HRESULT hres;

    push_compiler_statement_ctx(ctx, stat_ctx);

    set_compiler_loc(ctx, stat->loc);

    switch(stat->type) {
    case STAT_BLOCK:
        hres = compile_block_statement(ctx, (block_statement_t*)stat, ((block_statement_t*)stat)->stat_list);
        break;
    case STAT_BREAK:
        hres = compile_break_statement(ctx, (branch_statement_t*)stat);
        break;
    case STAT_CONTINUE:
        hres = compile_continue_statement(ctx, (branch_statement_t*)stat);
        break;
    case STAT_EMPTY:
        /* nothing to do */
        hres = S_OK;
        break;
    case STAT_EXPR:
        hres = compile_expression_statement(ctx, (expression_statement_t*)stat);
        break;
    case STAT_FOR:
        hres = compile_for_statement(ctx, (for_statement_t*)stat);
        break;
    case STAT_FORIN:
        hres = compile_forin_statement(ctx, (forin_statement_t*)stat);
        break;
    case STAT_IF:
        hres = compile_if_statement(ctx, (if_statement_t*)stat);
        break;
    case STAT_LABEL:
        hres = compile_labelled_statement(ctx, (labelled_statement_t*)stat);
        break;
    case STAT_RETURN:
        hres = compile_return_statement(ctx, (expression_statement_t*)stat);
        break;
    case STAT_SWITCH:
        hres = compile_switch_statement(ctx, (switch_statement_t*)stat);
        break;
    case STAT_THROW:
        hres = compile_throw_statement(ctx, (expression_statement_t*)stat);
        break;
    case STAT_TRY:
        hres = compile_try_statement(ctx, (try_statement_t*)stat);
        break;
    case STAT_VAR:
        hres = compile_var_statement(ctx, (var_statement_t*)stat);
        break;
    case STAT_WHILE:
        hres = compile_while_statement(ctx, (while_statement_t*)stat);
        break;
    case STAT_WITH:
        hres = compile_with_statement(ctx, (with_statement_t*)stat);
        break;
    DEFAULT_UNREACHABLE;
    }

    pop_compiler_statement_ctx(ctx, stat_ctx);

    return hres;
}

static inline function_local_t *find_local(compiler_ctx_t *ctx, const WCHAR *name, unsigned int scope)
{
    struct wine_rb_entry *entry = wine_rb_get(&ctx->local_scopes[scope].locals, name);
    return entry ? WINE_RB_ENTRY_VALUE(entry, function_local_t, entry) : NULL;
}

static BOOL alloc_local(compiler_ctx_t *ctx, BSTR name, int ref, unsigned int scope)
{
    function_local_t *local;

    local = heap_pool_alloc(&ctx->heap, sizeof(*local));
    if(!local)
        return FALSE;

    local->name = name;
    local->ref = ref;
    wine_rb_put(&ctx->local_scopes[scope].locals, name, &local->entry);
    ctx->local_scopes[scope].locals_cnt++;
    return TRUE;
}

static BOOL alloc_variable(compiler_ctx_t *ctx, const WCHAR *name, unsigned int scope)
{
    BSTR ident;

    if(find_local(ctx, name, scope))
        return TRUE;

    ident = compiler_alloc_bstr(ctx, name);
    if(!ident)
        return FALSE;

    return alloc_local(ctx, ident, ctx->func->var_cnt++, scope);
}

static HRESULT visit_function_expression(compiler_ctx_t *ctx, function_expression_t *expr)
{
    statement_ctx_t *stat_ctx;

    expr->func_id = ctx->func->func_cnt++;
    ctx->func_tail = ctx->func_tail ? (ctx->func_tail->next = expr) : (ctx->func_head = expr);

    if(!expr->identifier || expr->event_target)
        return S_OK;

    for (stat_ctx = ctx->stat_ctx; stat_ctx; stat_ctx = stat_ctx->next)
    {
        if (stat_ctx->block_scope)
        {
            stat_ctx->scope_has_functions = TRUE;
            break;
        }
    }

    if(!expr->is_statement && ctx->parser->script->version >= SCRIPTLANGUAGEVERSION_ES5)
        return S_OK;

    return alloc_variable(ctx, expr->identifier, stat_ctx ? stat_ctx->scope_index : 0) ? S_OK : E_OUTOFMEMORY;
}

static HRESULT visit_expression(compiler_ctx_t *ctx, expression_t *expr)
{
    HRESULT hres = S_OK;

    switch(expr->type) {
    case EXPR_ADD:
    case EXPR_AND:
    case EXPR_ARRAY:
    case EXPR_ASSIGN:
    case EXPR_ASSIGNADD:
    case EXPR_ASSIGNAND:
    case EXPR_ASSIGNSUB:
    case EXPR_ASSIGNMUL:
    case EXPR_ASSIGNDIV:
    case EXPR_ASSIGNMOD:
    case EXPR_ASSIGNOR:
    case EXPR_ASSIGNLSHIFT:
    case EXPR_ASSIGNRSHIFT:
    case EXPR_ASSIGNRRSHIFT:
    case EXPR_ASSIGNXOR:
    case EXPR_BAND:
    case EXPR_BOR:
    case EXPR_COMMA:
    case EXPR_DIV:
    case EXPR_EQ:
    case EXPR_EQEQ:
    case EXPR_GREATER:
    case EXPR_GREATEREQ:
    case EXPR_IN:
    case EXPR_INSTANCEOF:
    case EXPR_LESS:
    case EXPR_LESSEQ:
    case EXPR_LSHIFT:
    case EXPR_MOD:
    case EXPR_MUL:
    case EXPR_NOTEQ:
    case EXPR_NOTEQEQ:
    case EXPR_OR:
    case EXPR_RSHIFT:
    case EXPR_RRSHIFT:
    case EXPR_SUB:
    case EXPR_BXOR: {
        binary_expression_t *binary_expr = (binary_expression_t*)expr;

        hres = visit_expression(ctx, binary_expr->expression1);
        if(FAILED(hres))
            return hres;

        hres = visit_expression(ctx, binary_expr->expression2);
        break;
    }
    case EXPR_BITNEG:
    case EXPR_DELETE:
    case EXPR_LOGNEG:
    case EXPR_MINUS:
    case EXPR_PLUS:
    case EXPR_POSTDEC:
    case EXPR_POSTINC:
    case EXPR_PREDEC:
    case EXPR_PREINC:
    case EXPR_TYPEOF:
    case EXPR_VOID:
        hres = visit_expression(ctx, ((unary_expression_t*)expr)->expression);
        break;
    case EXPR_IDENT:
    case EXPR_LITERAL:
    case EXPR_THIS:
        break;
    case EXPR_ARRAYLIT: {
        array_literal_expression_t *array_expr = (array_literal_expression_t*)expr;
        array_element_t *iter;

        for(iter = array_expr->element_list; iter; iter = iter->next) {
            hres = visit_expression(ctx, iter->expr);
            if(FAILED(hres))
                return hres;
        }
        break;
    }
    case EXPR_CALL:
    case EXPR_NEW: {
        call_expression_t *call_expr = (call_expression_t*)expr;
        argument_t *arg;

        hres = visit_expression(ctx, call_expr->expression);
        if(FAILED(hres))
            return hres;

        for(arg = call_expr->argument_list; arg; arg = arg->next) {
            hres = visit_expression(ctx, arg->expr);
            if(FAILED(hres))
                return hres;
        }
        break;
    }
    case EXPR_COND: {
        conditional_expression_t *cond_expr = (conditional_expression_t*)expr;

        hres = visit_expression(ctx, cond_expr->expression);
        if(FAILED(hres))
            return hres;

        hres = visit_expression(ctx, cond_expr->true_expression);
        if(FAILED(hres))
            return hres;

        hres = visit_expression(ctx, cond_expr->false_expression);
        break;
    }
    case EXPR_FUNC:
        hres = visit_function_expression(ctx, (function_expression_t*)expr);
        break;
    case EXPR_MEMBER:
        hres = visit_expression(ctx, ((member_expression_t*)expr)->expression);
        break;
    case EXPR_PROPVAL: {
        property_definition_t *iter;
        for(iter = ((property_value_expression_t*)expr)->property_list; iter; iter = iter->next) {
            hres = visit_expression(ctx, iter->value);
            if(FAILED(hres))
                return hres;
        }
        break;
    }
    DEFAULT_UNREACHABLE;
    }

    return hres;
}

static HRESULT visit_variable_list(compiler_ctx_t *ctx, variable_declaration_t *list)
{
    variable_declaration_t *iter;
    statement_ctx_t *stat_ctx;
    HRESULT hres;

    for(iter = list; iter; iter = iter->next) {
        for (stat_ctx = ctx->stat_ctx; stat_ctx; stat_ctx = stat_ctx->next)
        {
            if (stat_ctx->block_scope)
                break;
        }

        if(!alloc_variable(ctx, iter->identifier, iter->block_scope && stat_ctx ? stat_ctx->scope_index : 0))
                return E_OUTOFMEMORY;

        if(iter->expr) {
            hres = visit_expression(ctx, iter->expr);
            if(FAILED(hres))
                return hres;
        }
    }

    return S_OK;
}

static HRESULT visit_statement(compiler_ctx_t*,statement_ctx_t *,statement_t*);

static HRESULT visit_block_statement(compiler_ctx_t *ctx, block_statement_t *block, statement_t *iter)
{
    statement_ctx_t stat_ctx = {0, TRUE};
    BOOL needs_scope;
    HRESULT hres;

    needs_scope = block && ctx->parser->script->version >= SCRIPTLANGUAGEVERSION_ES5;
    if (needs_scope)
    {
        if (!alloc_local_scope(ctx, &block->scope_index))
            return E_OUTOFMEMORY;

        stat_ctx.scope_index = block->scope_index;
        stat_ctx.block_scope = TRUE;
    }

    while(iter) {
        hres = visit_statement(ctx, needs_scope ? &stat_ctx : NULL, iter);
        if(FAILED(hres))
            return hres;

        iter = iter->next;
    }

    if (needs_scope && !(ctx->local_scopes[stat_ctx.scope_index].locals_cnt || stat_ctx.scope_has_functions))
        remove_local_scope(ctx, block->scope_index);

    return S_OK;
}

static HRESULT visit_statement(compiler_ctx_t *ctx, statement_ctx_t *stat_ctx, statement_t *stat)
{
    HRESULT hres = S_OK;

    push_compiler_statement_ctx(ctx, stat_ctx);

    switch(stat->type) {
    case STAT_BLOCK:
        hres = visit_block_statement(ctx, (block_statement_t*)stat, ((block_statement_t*)stat)->stat_list);
        break;
    case STAT_BREAK:
    case STAT_CONTINUE:
    case STAT_EMPTY:
        break;
    case STAT_EXPR: {
        expression_statement_t *expr_stat = (expression_statement_t*)stat;
        if(expr_stat->expr) {
            if(expr_stat->expr->type == EXPR_FUNC)
                ((function_expression_t*)expr_stat->expr)->is_statement = TRUE;
            hres = visit_expression(ctx, expr_stat->expr);
        }
        break;
    }
    case STAT_RETURN:
    case STAT_THROW: {
        expression_statement_t *expr_stat = (expression_statement_t*)stat;
        if(expr_stat->expr)
            hres = visit_expression(ctx, expr_stat->expr);
        break;
    }
    case STAT_FOR: {
        statement_ctx_t stat_ctx_data = {0, TRUE}, *stat_ctx = NULL;
        for_statement_t *for_stat = (for_statement_t*)stat;

        if(for_stat->variable_list)
        {
            variable_declaration_t *var;

            for(var = for_stat->variable_list; var; var = var->next)
            {
                if (var->block_scope)
                {
                    stat_ctx = &stat_ctx_data;
                    break;
                }
            }

            if (stat_ctx)
            {
                if (!alloc_local_scope(ctx, &for_stat->scope_index))
                {
                    hres = E_OUTOFMEMORY;
                    break;
                }
                stat_ctx->scope_index = for_stat->scope_index;
                stat_ctx->block_scope = TRUE;
                push_compiler_statement_ctx(ctx, stat_ctx);
            }
            hres = visit_variable_list(ctx, for_stat->variable_list);
        }
        else if(for_stat->begin_expr)
            hres = visit_expression(ctx, for_stat->begin_expr);
        if(FAILED(hres))
        {
            pop_compiler_statement_ctx(ctx, stat_ctx);
            break;
        }

        if(for_stat->expr) {
            hres = visit_expression(ctx, for_stat->expr);
            if(FAILED(hres))
            {
                pop_compiler_statement_ctx(ctx, stat_ctx);
                break;
            }
        }

        hres = visit_statement(ctx, NULL, for_stat->statement);
        if(FAILED(hres))
        {
            pop_compiler_statement_ctx(ctx, stat_ctx);
            break;
        }
        if(for_stat->end_expr)
            hres = visit_expression(ctx, for_stat->end_expr);
        pop_compiler_statement_ctx(ctx, stat_ctx);
        break;
    }
    case STAT_FORIN:  {
        forin_statement_t *forin_stat = (forin_statement_t*)stat;

        if(forin_stat->variable) {
            hres = visit_variable_list(ctx, forin_stat->variable);
            if(FAILED(hres))
                break;
        }

        hres = visit_expression(ctx, forin_stat->in_expr);
        if(FAILED(hres))
            return hres;

        if(forin_stat->expr) {
            hres = visit_expression(ctx, forin_stat->expr);
            if(FAILED(hres))
                return hres;
        }

        hres = visit_statement(ctx, NULL, forin_stat->statement);
        break;
    }
    case STAT_IF: {
        if_statement_t *if_stat = (if_statement_t*)stat;

        hres = visit_expression(ctx, if_stat->expr);
        if(FAILED(hres))
            return hres;

        hres = visit_statement(ctx, NULL, if_stat->if_stat);
        if(FAILED(hres))
            return hres;

        if(if_stat->else_stat)
            hres = visit_statement(ctx, NULL, if_stat->else_stat);
        break;
    }
    case STAT_LABEL:
        hres = visit_statement(ctx, NULL, ((labelled_statement_t*)stat)->statement);
        break;
    case STAT_SWITCH: {
        switch_statement_t *switch_stat = (switch_statement_t*)stat;
        statement_t *stat_iter;
        case_clausule_t *iter;

        hres = visit_expression(ctx, switch_stat->expr);
        if(FAILED(hres))
            return hres;

        for(iter = switch_stat->case_list; iter; iter = iter->next) {
            if(!iter->expr)
                continue;
            hres = visit_expression(ctx, iter->expr);
            if(FAILED(hres))
                return hres;
        }

        for(iter = switch_stat->case_list; iter; iter = iter->next) {
            while(iter->next && iter->next->stat == iter->stat)
                iter = iter->next;
            for(stat_iter = iter->stat; stat_iter && (!iter->next || iter->next->stat != stat_iter);
                stat_iter = stat_iter->next) {
                hres = visit_statement(ctx, NULL, stat_iter);
                if(FAILED(hres))
                    return hres;
            }
        }
        break;
    }
    case STAT_TRY: {
        try_statement_t *try_stat = (try_statement_t*)stat;

        hres = visit_statement(ctx, NULL, try_stat->try_statement);
        if(FAILED(hres))
            return hres;

        if(try_stat->catch_block) {
            hres = visit_statement(ctx, NULL, try_stat->catch_block->statement);
            if(FAILED(hres))
                return hres;
        }

        if(try_stat->finally_statement)
            hres = visit_statement(ctx, NULL, try_stat->finally_statement);
        break;
    }
    case STAT_VAR:
        hres = visit_variable_list(ctx, ((var_statement_t*)stat)->variable_list);
        break;
    case STAT_WHILE: {
        while_statement_t *while_stat = (while_statement_t*)stat;

        hres = visit_expression(ctx, while_stat->expr);
        if(FAILED(hres))
            return hres;

        hres = visit_statement(ctx, NULL, while_stat->statement);
        break;
    }
    case STAT_WITH: {
        with_statement_t *with_stat = (with_statement_t*)stat;

        hres = visit_expression(ctx, with_stat->expr);
        if(FAILED(hres))
            return hres;

        hres = visit_statement(ctx, NULL, with_stat->statement);
        break;
    }
    DEFAULT_UNREACHABLE;
    }

    pop_compiler_statement_ctx(ctx, stat_ctx);

    return hres;
}

static void resolve_labels(compiler_ctx_t *ctx, unsigned off)
{
    instr_t *instr;

    for(instr = ctx->code->instrs+off; instr < ctx->code->instrs+ctx->code_off; instr++) {
        if(instr_info[instr->op].arg1_type == ARG_ADDR && (instr->u.arg->uint & LABEL_FLAG)) {
            assert((instr->u.arg->uint & ~LABEL_FLAG) < ctx->labels_cnt);
            instr->u.arg->uint = ctx->labels[instr->u.arg->uint & ~LABEL_FLAG];
        }
        assert(instr_info[instr->op].arg2_type != ARG_ADDR);
    }

    ctx->labels_cnt = 0;
}

unsigned get_location_line(bytecode_t *code, unsigned loc, unsigned *char_pos)
{
    unsigned line = code->start_line;
    const WCHAR *nl, *p;

    for(nl = p = code->source; p < code->source + loc; p++) {
        if(*p != '\n') continue;
        line++;
        nl = p + 1;
    }
    *char_pos = loc - (nl - code->source);
    return line;
}

void release_bytecode(bytecode_t *code)
{
    unsigned i;

    if(--code->ref)
        return;

    for(i=0; i < code->bstr_cnt; i++)
        SysFreeString(code->bstr_pool[i]);
    for(i=0; i < code->str_cnt; i++)
        jsstr_release(code->str_pool[i]);

    if(code->named_item)
        release_named_item(code->named_item);
    free(code->source);
    heap_pool_free(&code->heap);
    free(code->bstr_pool);
    free(code->str_pool);
    free(code->instrs);
    free(code);
}

static HRESULT init_code(compiler_ctx_t *compiler, const WCHAR *source, UINT64 source_context, unsigned start_line)
{
    size_t len = source ? lstrlenW(source) : 0;

    if(len > INT32_MAX)
        return E_OUTOFMEMORY;

    compiler->code = calloc(1, sizeof(bytecode_t));
    if(!compiler->code)
        return E_OUTOFMEMORY;

    compiler->code->ref = 1;
    compiler->code->source_context = source_context;
    compiler->code->start_line = start_line;
    heap_pool_init(&compiler->code->heap);

    compiler->code->source = malloc((len + 1) * sizeof(WCHAR));
    if(!compiler->code->source) {
        release_bytecode(compiler->code);
        return E_OUTOFMEMORY;
    }
    if(len)
        memcpy(compiler->code->source, source, len * sizeof(WCHAR));
    compiler->code->source[len] = 0;

    compiler->code->instrs = malloc(64 * sizeof(instr_t));
    if(!compiler->code->instrs) {
        release_bytecode(compiler->code);
        return E_OUTOFMEMORY;
    }

    compiler->code_size = 64;
    compiler->code_off = 1;
    return S_OK;
}

static HRESULT compile_function(compiler_ctx_t *ctx, statement_t *source, function_expression_t *func_expr,
        BOOL from_eval, function_code_t *func)
{
    function_expression_t *iter;
    function_local_t *local;
    unsigned off, i, scope;
    HRESULT hres;

    TRACE("\n");

    func->bytecode = ctx->code;
    func->local_ref = INVALID_LOCAL_REF;
    func->scope_index = 0;
    ctx->func_head = ctx->func_tail = NULL;
    ctx->from_eval = from_eval;
    ctx->func = func;
    ctx->local_scope_count = 0;
    if (!alloc_local_scope(ctx, &scope))
        return E_OUTOFMEMORY;
    assert(!scope);

    if(func_expr) {
        parameter_t *param_iter;

        if(func_expr->identifier) {
            func->name = compiler_alloc_bstr(ctx, func_expr->identifier);
            if(!func->name)
                return E_OUTOFMEMORY;
        }

        if(func_expr->event_target) {
            func->event_target = compiler_alloc_bstr(ctx, func_expr->event_target);
            if(!func->event_target)
                return E_OUTOFMEMORY;
        }

        func->source = func_expr->src_str;
        func->source_len = func_expr->src_len;

        for(param_iter = func_expr->parameter_list; param_iter; param_iter = param_iter->next)
            func->param_cnt++;

        func->params = compiler_alloc(ctx->code, func->param_cnt * sizeof(*func->params));
        if(!func->params)
            return E_OUTOFMEMORY;

        for(param_iter = func_expr->parameter_list, i=0; param_iter; param_iter = param_iter->next, i++) {
            func->params[i] = compiler_alloc_bstr(ctx, param_iter->identifier);
            if(!func->params[i])
                return E_OUTOFMEMORY;
        }
    }

    for(i = func->param_cnt; i--;) {
        if(!find_local(ctx, func->params[i], 0) && !alloc_local(ctx, func->params[i], -i-1, 0))
            return E_OUTOFMEMORY;
    }

    hres = visit_block_statement(ctx, NULL, source);
    if(FAILED(hres))
        return hres;

    func->local_scope_count = ctx->local_scope_count;
    func->local_scopes = compiler_alloc(ctx->code, func->local_scope_count * sizeof(*func->local_scopes));
    if(!func->local_scopes)
        return E_OUTOFMEMORY;

    func->variables = compiler_alloc(ctx->code, func->var_cnt * sizeof(*func->variables));
    if(!func->variables)
        return E_OUTOFMEMORY;

    for (scope = 0; scope < func->local_scope_count; ++scope)
    {
        func->local_scopes[scope].locals = compiler_alloc(ctx->code,
                ctx->local_scopes[scope].locals_cnt * sizeof(*func->local_scopes[scope].locals));
        if(!func->local_scopes[scope].locals)
            return E_OUTOFMEMORY;
        func->local_scopes[scope].locals_cnt = ctx->local_scopes[scope].locals_cnt;

        i = 0;
        WINE_RB_FOR_EACH_ENTRY(local, &ctx->local_scopes[scope].locals, function_local_t, entry) {
            func->local_scopes[scope].locals[i].name = local->name;
            func->local_scopes[scope].locals[i].ref = local->ref;
            if(local->ref >= 0) {
                func->variables[local->ref].name = local->name;
                func->variables[local->ref].func_id = -1;
            }
            i++;
        }
        assert(i == ctx->local_scopes[scope].locals_cnt);
    }

    func->funcs = compiler_alloc(ctx->code, func->func_cnt * sizeof(*func->funcs));
    if(!func->funcs)
        return E_OUTOFMEMORY;
    memset(func->funcs, 0, func->func_cnt * sizeof(*func->funcs));

    ctx->current_function_expr = ctx->func_head;
    off = ctx->code_off;
    hres = compile_block_statement(ctx, NULL, source);
    if(FAILED(hres))
        return hres;

    resolve_labels(ctx, off);

    hres = push_instr_uint(ctx, OP_ret, !from_eval);
    if(FAILED(hres))
        return hres;

    if(TRACE_ON(jscript_disas))
        dump_code(ctx, off);

    func->instr_off = off;

    for(iter = ctx->func_head, i=0; iter; iter = iter->next, i++) {
        hres = compile_function(ctx, iter->statement_list, iter, FALSE, func->funcs+i);
        if(FAILED(hres))
            return hres;

        func->funcs[i].scope_index = iter->scope_index;

        TRACE("[%d] func %s, scope_index %u\n", i, debugstr_w(func->funcs[i].name), iter->scope_index);
        if((ctx->parser->script->version < SCRIPTLANGUAGEVERSION_ES5 || iter->is_statement) &&
           func->funcs[i].name && !func->funcs[i].event_target) {
            local_ref_t *local_ref = lookup_local(func, func->funcs[i].name, func->funcs[i].scope_index);

            func->funcs[i].local_ref = local_ref->ref;
            TRACE("found ref %s %d for %s\n", debugstr_w(local_ref->name), local_ref->ref, debugstr_w(func->funcs[i].name));
            if(local_ref->ref >= 0)
                func->variables[local_ref->ref].func_id = i;
        }
    }

    assert(i == func->func_cnt);

    return S_OK;
}

static HRESULT parse_arguments(compiler_ctx_t *ctx, const WCHAR *args, BSTR *arg_array, unsigned *args_size)
{
    const WCHAR *ptr = args, *ptr2;
    unsigned arg_cnt = 0;

    while(iswspace(*ptr))
        ptr++;
    if(!*ptr) {
        if(args_size)
            *args_size = 0;
        return S_OK;
    }

    while(1) {
        if(!iswalpha(*ptr) && *ptr != '_') {
            FIXME("expected alpha or '_': %s\n", debugstr_w(ptr));
            return E_FAIL;
        }

        ptr2 = ptr;
        while(iswalnum(*ptr) || *ptr == '_')
            ptr++;

        if(*ptr && *ptr != ',' && !iswspace(*ptr)) {
            FIXME("unexpected har %s\n", debugstr_w(ptr));
            return E_FAIL;
        }

        if(arg_array) {
            arg_array[arg_cnt] = compiler_alloc_bstr_len(ctx, ptr2, ptr-ptr2);
            if(!arg_array[arg_cnt])
                return E_OUTOFMEMORY;
        }
        arg_cnt++;

        while(iswspace(*ptr))
            ptr++;
        if(!*ptr)
            break;
        if(*ptr != ',') {
            FIXME("expected ',': %s\n", debugstr_w(ptr));
            return E_FAIL;
        }

        ptr++;
        while(iswspace(*ptr))
            ptr++;
    }

    if(args_size)
        *args_size = arg_cnt;
    return S_OK;
}

static HRESULT compile_arguments(compiler_ctx_t *ctx, const WCHAR *args)
{
    HRESULT hres;

    hres = parse_arguments(ctx, args, NULL, &ctx->code->global_code.param_cnt);
    if(FAILED(hres))
        return hres;

    ctx->code->global_code.params = compiler_alloc(ctx->code,
            ctx->code->global_code.param_cnt * sizeof(*ctx->code->global_code.params));
    if(!ctx->code->global_code.params)
        return E_OUTOFMEMORY;

    return parse_arguments(ctx, args, ctx->code->global_code.params, NULL);
}

HRESULT compile_script(script_ctx_t *ctx, const WCHAR *code, UINT64 source_context, unsigned start_line,
                       const WCHAR *args, const WCHAR *delimiter, BOOL from_eval, BOOL use_decode,
                       named_item_t *named_item, bytecode_t **ret)
{
    compiler_ctx_t compiler = {0};
    HRESULT hres;

    hres = init_code(&compiler, code, source_context, start_line);
    if(FAILED(hres))
        return hres;

    if(args) {
        hres = compile_arguments(&compiler, args);
        if(FAILED(hres))
            return hres;
    }

    if(use_decode) {
        hres = decode_source(compiler.code->source);
        if(FAILED(hres)) {
            WARN("Decoding failed\n");
            return hres;
        }
    }

    hres = script_parse(ctx, &compiler, compiler.code, delimiter, from_eval, &compiler.parser);
    if(FAILED(hres)) {
        release_bytecode(compiler.code);
        return hres;
    }

    heap_pool_init(&compiler.heap);
    hres = compile_function(&compiler, compiler.parser->source, NULL, from_eval, &compiler.code->global_code);
    free(compiler.local_scopes);
    heap_pool_free(&compiler.heap);
    parser_release(compiler.parser);
    if(FAILED(hres)) {
        if(hres != DISP_E_EXCEPTION)
            throw_error(ctx, hres, NULL);
        set_error_location(ctx->ei, compiler.code, compiler.loc, IDS_COMPILATION_ERROR, NULL);
        release_bytecode(compiler.code);
        return DISP_E_EXCEPTION;
    }

    if(named_item) {
        compiler.code->named_item = named_item;
        named_item->ref++;
    }

    *ret = compiler.code;
    return S_OK;
}
