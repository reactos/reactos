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

#include <assert.h>

#include "vbscript.h"
#include "parse.h"
#include "parser.tab.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(vbscript);
WINE_DECLARE_DEBUG_CHANNEL(vbscript_disas);

typedef struct _statement_ctx_t {
    unsigned stack_use;

    unsigned while_end_label;
    unsigned for_end_label;
    unsigned with_stack_offset;

    struct _statement_ctx_t *next;
} statement_ctx_t;

typedef struct {
    parser_ctx_t parser;

    unsigned instr_cnt;
    unsigned instr_size;
    vbscode_t *code;

    unsigned loc;
    statement_ctx_t *stat_ctx;

    unsigned *labels;
    unsigned labels_size;
    unsigned labels_cnt;

    unsigned sub_end_label;
    unsigned func_end_label;
    unsigned prop_end_label;

    dim_decl_t *dim_decls;
    dim_decl_t *dim_decls_tail;

    const_decl_t *const_decls;
    const_decl_t *global_consts;

    function_t *func;
    function_decl_t *func_decls;
} compile_ctx_t;

static HRESULT compile_expression(compile_ctx_t*,expression_t*);
static HRESULT compile_statement(compile_ctx_t*,statement_ctx_t*,statement_t*);

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
    case ARG_BSTR:
        TRACE_(vbscript_disas)("\t%s", debugstr_w(arg->str));
        break;
    case ARG_INT:
        TRACE_(vbscript_disas)("\t%d", arg->uint);
        break;
    case ARG_UINT:
    case ARG_ADDR:
        TRACE_(vbscript_disas)("\t%u", arg->uint);
        break;
    case ARG_DATE:
    case ARG_DOUBLE:
        TRACE_(vbscript_disas)("\t%lf", *arg->dbl);
        break;
    case ARG_NONE:
        break;
    DEFAULT_UNREACHABLE;
    }
}

static void dump_code(compile_ctx_t *ctx)
{
    instr_t *instr;

    for(instr = ctx->code->instrs+1; instr < ctx->code->instrs+ctx->instr_cnt; instr++) {
        assert(instr->op < OP_LAST);
        TRACE_(vbscript_disas)("%d:\t%s", (int)(instr-ctx->code->instrs), instr_info[instr->op].op_str);
        dump_instr_arg(instr_info[instr->op].arg1_type, &instr->arg1);
        dump_instr_arg(instr_info[instr->op].arg2_type, &instr->arg2);
        TRACE_(vbscript_disas)("\n");
    }
}

static inline void *compiler_alloc(vbscode_t *vbscode, size_t size)
{
    return heap_pool_alloc(&vbscode->heap, size);
}

static inline void *compiler_alloc_zero(vbscode_t *vbscode, size_t size)
{
    void *ret;

    ret = heap_pool_alloc(&vbscode->heap, size);
    if(ret)
        memset(ret, 0, size);
    return ret;
}

static WCHAR *compiler_alloc_string(vbscode_t *vbscode, const WCHAR *str)
{
    size_t size;
    WCHAR *ret;

    size = (lstrlenW(str)+1)*sizeof(WCHAR);
    ret = compiler_alloc(vbscode, size);
    if(ret)
        memcpy(ret, str, size);
    return ret;
}

static inline instr_t *instr_ptr(compile_ctx_t *ctx, unsigned id)
{
    assert(id < ctx->instr_cnt);
    return ctx->code->instrs + id;
}

static unsigned push_instr(compile_ctx_t *ctx, vbsop_t op)
{
    assert(ctx->instr_size && ctx->instr_size >= ctx->instr_cnt);

    if(ctx->instr_size == ctx->instr_cnt) {
        instr_t *new_instr;

        new_instr = realloc(ctx->code->instrs, ctx->instr_size*2*sizeof(instr_t));
        if(!new_instr)
            return 0;

        ctx->code->instrs = new_instr;
        ctx->instr_size *= 2;
    }

    ctx->code->instrs[ctx->instr_cnt].op = op;
    ctx->code->instrs[ctx->instr_cnt].loc = ctx->loc;
    return ctx->instr_cnt++;
}

static HRESULT push_instr_int(compile_ctx_t *ctx, vbsop_t op, LONG arg)
{
    unsigned ret;

    ret = push_instr(ctx, op);
    if(!ret)
        return E_OUTOFMEMORY;

    instr_ptr(ctx, ret)->arg1.lng = arg;
    return S_OK;
}

static HRESULT push_instr_uint(compile_ctx_t *ctx, vbsop_t op, unsigned arg)
{
    unsigned ret;

    ret = push_instr(ctx, op);
    if(!ret)
        return E_OUTOFMEMORY;

    instr_ptr(ctx, ret)->arg1.uint = arg;
    return S_OK;
}

static HRESULT push_instr_addr(compile_ctx_t *ctx, vbsop_t op, unsigned arg)
{
    unsigned ret;

    ret = push_instr(ctx, op);
    if(!ret)
        return E_OUTOFMEMORY;

    instr_ptr(ctx, ret)->arg1.uint = arg;
    return S_OK;
}

static HRESULT push_instr_str(compile_ctx_t *ctx, vbsop_t op, const WCHAR *arg)
{
    unsigned instr;
    WCHAR *str;

    str = compiler_alloc_string(ctx->code, arg);
    if(!str)
        return E_OUTOFMEMORY;

    instr = push_instr(ctx, op);
    if(!instr)
        return E_OUTOFMEMORY;

    instr_ptr(ctx, instr)->arg1.str = str;
    return S_OK;
}

static HRESULT push_instr_double(compile_ctx_t *ctx, vbsop_t op, double arg)
{
    unsigned instr;
    double *d;

    d = compiler_alloc(ctx->code, sizeof(double));
    if(!d)
        return E_OUTOFMEMORY;

    instr = push_instr(ctx, op);
    if(!instr)
        return E_OUTOFMEMORY;

    *d = arg;
    instr_ptr(ctx, instr)->arg1.dbl = d;
    return S_OK;
}

static HRESULT push_instr_date(compile_ctx_t *ctx, vbsop_t op, DATE arg)
{
    unsigned instr;
    DATE *d;

    d = compiler_alloc(ctx->code, sizeof(DATE));
    if(!d)
        return E_OUTOFMEMORY;

    instr = push_instr(ctx, op);
    if(!instr)
        return E_OUTOFMEMORY;

    *d = arg;
    instr_ptr(ctx, instr)->arg1.date = d;
    return S_OK;
}

static BSTR alloc_bstr_arg(compile_ctx_t *ctx, const WCHAR *str)
{
    if(!ctx->code->bstr_pool_size) {
        ctx->code->bstr_pool = malloc(8 * sizeof(BSTR));
        if(!ctx->code->bstr_pool)
            return NULL;
        ctx->code->bstr_pool_size = 8;
    }else if(ctx->code->bstr_pool_size == ctx->code->bstr_cnt) {
        BSTR *new_pool;

        new_pool = realloc(ctx->code->bstr_pool, ctx->code->bstr_pool_size*2*sizeof(BSTR));
        if(!new_pool)
            return NULL;

        ctx->code->bstr_pool = new_pool;
        ctx->code->bstr_pool_size *= 2;
    }

    ctx->code->bstr_pool[ctx->code->bstr_cnt] = SysAllocString(str);
    if(!ctx->code->bstr_pool[ctx->code->bstr_cnt])
        return NULL;

    return ctx->code->bstr_pool[ctx->code->bstr_cnt++];
}

static HRESULT push_instr_bstr(compile_ctx_t *ctx, vbsop_t op, const WCHAR *arg)
{
    unsigned instr;
    BSTR bstr;

    bstr = alloc_bstr_arg(ctx, arg);
    if(!bstr)
        return E_OUTOFMEMORY;

    instr = push_instr(ctx, op);
    if(!instr)
        return E_OUTOFMEMORY;

    instr_ptr(ctx, instr)->arg1.bstr = bstr;
    return S_OK;
}

static HRESULT push_instr_bstr_uint(compile_ctx_t *ctx, vbsop_t op, const WCHAR *arg1, unsigned arg2)
{
    unsigned instr;
    BSTR bstr;

    bstr = alloc_bstr_arg(ctx, arg1);
    if(!bstr)
        return E_OUTOFMEMORY;

    instr = push_instr(ctx, op);
    if(!instr)
        return E_OUTOFMEMORY;

    instr_ptr(ctx, instr)->arg1.bstr = bstr;
    instr_ptr(ctx, instr)->arg2.uint = arg2;
    return S_OK;
}

static HRESULT push_instr_uint_bstr(compile_ctx_t *ctx, vbsop_t op, unsigned arg1, const WCHAR *arg2)
{
    unsigned instr;
    BSTR bstr;

    bstr = alloc_bstr_arg(ctx, arg2);
    if(!bstr)
        return E_OUTOFMEMORY;

    instr = push_instr(ctx, op);
    if(!instr)
        return E_OUTOFMEMORY;

    instr_ptr(ctx, instr)->arg1.uint = arg1;
    instr_ptr(ctx, instr)->arg2.bstr = bstr;
    return S_OK;
}

#define LABEL_FLAG 0x80000000

static unsigned alloc_label(compile_ctx_t *ctx)
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

static inline void label_set_addr(compile_ctx_t *ctx, unsigned label)
{
    assert(label & LABEL_FLAG);
    ctx->labels[label & ~LABEL_FLAG] = ctx->instr_cnt;
}

static inline unsigned stack_offset(compile_ctx_t *ctx)
{
    statement_ctx_t *iter;
    unsigned ret = 0;

    for(iter = ctx->stat_ctx; iter; iter = iter->next)
        ret += iter->stack_use;

    return ret;
}

static BOOL emit_catch_jmp(compile_ctx_t *ctx, unsigned stack_off, unsigned code_off)
{
    unsigned code;

    code = push_instr(ctx, OP_catch);
    if(!code)
        return FALSE;

    instr_ptr(ctx, code)->arg1.uint = code_off;
    instr_ptr(ctx, code)->arg2.uint = stack_off + stack_offset(ctx);
    return TRUE;
}

static inline BOOL emit_catch(compile_ctx_t *ctx, unsigned off)
{
    return emit_catch_jmp(ctx, off, ctx->instr_cnt);
}

static HRESULT compile_error(script_ctx_t *ctx, compile_ctx_t *compiler, HRESULT error)
{
    if(error == SCRIPT_E_REPORTED)
        return error;

    clear_ei(&ctx->ei);
    ctx->ei.scode = error;
    ctx->ei.bstrSource = get_vbscript_string(VBS_COMPILE_ERROR);
    map_vbs_exception(&ctx->ei);
    return report_script_error(ctx, compiler->code, compiler->loc);
}

static expression_t *lookup_const_decls(compile_ctx_t *ctx, const WCHAR *name, BOOL lookup_global)
{
    const_decl_t *decl;

    for(decl = ctx->const_decls; decl; decl = decl->next) {
        if(!wcsicmp(decl->name, name))
            return decl->value_expr;
    }

    if(!lookup_global)
        return NULL;

    for(decl = ctx->global_consts; decl; decl = decl->next) {
        if(!wcsicmp(decl->name, name))
            return decl->value_expr;
    }

    return NULL;
}

static BOOL lookup_args_name(compile_ctx_t *ctx, const WCHAR *name)
{
    unsigned i;

    for(i = 0; i < ctx->func->arg_cnt; i++) {
        if(!wcsicmp(ctx->func->args[i].name, name))
            return TRUE;
    }

    return FALSE;
}

static BOOL lookup_dim_decls(compile_ctx_t *ctx, const WCHAR *name)
{
    dim_decl_t *dim_decl;

    for(dim_decl = ctx->dim_decls; dim_decl; dim_decl = dim_decl->next) {
        if(!wcsicmp(dim_decl->name, name))
            return TRUE;
    }

    return FALSE;
}

static HRESULT compile_args(compile_ctx_t *ctx, expression_t *args, unsigned *ret)
{
    unsigned arg_cnt = 0;
    HRESULT hres;

    while(args) {
        hres = compile_expression(ctx, args);
        if(FAILED(hres))
            return hres;

        if(args->type == EXPR_BRACKETS && !push_instr(ctx, OP_deref))
            return E_OUTOFMEMORY;

        arg_cnt++;
        args = args->next;
    }

    *ret = arg_cnt;
    return S_OK;
}

static HRESULT compile_member_call_expression(compile_ctx_t *ctx, member_expression_t *expr,
                                              unsigned arg_cnt, BOOL ret_val)
{
    HRESULT hres;

    if(ret_val && !arg_cnt) {
        expression_t *const_expr;

        const_expr = lookup_const_decls(ctx, expr->identifier, TRUE);
        if(const_expr)
            return compile_expression(ctx, const_expr);
    }

    if(expr->obj_expr) {
        hres = compile_expression(ctx, expr->obj_expr);
        if(FAILED(hres))
            return hres;

        hres = push_instr_bstr_uint(ctx, ret_val ? OP_mcall : OP_mcallv, expr->identifier, arg_cnt);
    }else {
        hres = push_instr_bstr_uint(ctx, ret_val ? OP_icall : OP_icallv, expr->identifier, arg_cnt);
    }

    return hres;
}

static HRESULT compile_member_expression(compile_ctx_t *ctx, member_expression_t *expr)
{
    expression_t *const_expr;

    if (expr->obj_expr) /* FIXME: we should probably have a dedicated opcode as well */
        return compile_member_call_expression(ctx, expr, 0, TRUE);

    if (!lookup_dim_decls(ctx, expr->identifier) && !lookup_args_name(ctx, expr->identifier)) {
        const_expr = lookup_const_decls(ctx, expr->identifier, TRUE);
        if(const_expr)
            return compile_expression(ctx, const_expr);
    }
    return push_instr_bstr(ctx, OP_ident, expr->identifier);
}

static HRESULT compile_call_expression(compile_ctx_t *ctx, call_expression_t *expr, BOOL ret_val)
{
    unsigned arg_cnt = 0;
    expression_t *call;
    HRESULT hres;

    hres = compile_args(ctx, expr->args, &arg_cnt);
    if(FAILED(hres))
        return hres;

    for(call = expr->call_expr; call->type == EXPR_BRACKETS; call = ((unary_expression_t*)call)->subexpr);

    if(call->type == EXPR_MEMBER)
        return compile_member_call_expression(ctx, (member_expression_t*)call, arg_cnt, ret_val);

    hres = compile_expression(ctx, call);
    if(FAILED(hres))
        return hres;

    return push_instr_uint(ctx, ret_val ? OP_vcall : OP_vcallv, arg_cnt);
}

static HRESULT compile_dot_expression(compile_ctx_t *ctx)
{
    statement_ctx_t *stat_ctx;

    for(stat_ctx = ctx->stat_ctx; stat_ctx; stat_ctx = stat_ctx->next) {
        if(!stat_ctx->with_stack_offset)
            continue;

        return push_instr_uint(ctx, OP_stack, stat_ctx->with_stack_offset - 1);
    }

    WARN("dot expression outside with statement\n");
    return push_instr_uint(ctx, OP_stack, ~0);
}

static HRESULT compile_unary_expression(compile_ctx_t *ctx, unary_expression_t *expr, vbsop_t op)
{
    HRESULT hres;

    hres = compile_expression(ctx, expr->subexpr);
    if(FAILED(hres))
        return hres;

    return push_instr(ctx, op) ? S_OK : E_OUTOFMEMORY;
}

static HRESULT compile_binary_expression(compile_ctx_t *ctx, binary_expression_t *expr, vbsop_t op)
{
    HRESULT hres;

    hres = compile_expression(ctx, expr->left);
    if(FAILED(hres))
        return hres;

    hres = compile_expression(ctx, expr->right);
    if(FAILED(hres))
        return hres;

    return push_instr(ctx, op) ? S_OK : E_OUTOFMEMORY;
}

static HRESULT compile_expression(compile_ctx_t *ctx, expression_t *expr)
{
    switch(expr->type) {
    case EXPR_ADD:
        return compile_binary_expression(ctx, (binary_expression_t*)expr, OP_add);
    case EXPR_AND:
        return compile_binary_expression(ctx, (binary_expression_t*)expr, OP_and);
    case EXPR_BOOL:
        return push_instr_int(ctx, OP_bool, ((bool_expression_t*)expr)->value);
    case EXPR_BRACKETS:
        return compile_expression(ctx, ((unary_expression_t*)expr)->subexpr);
    case EXPR_CALL:
        return compile_call_expression(ctx, (call_expression_t*)expr, TRUE);
    case EXPR_CONCAT:
        return compile_binary_expression(ctx, (binary_expression_t*)expr, OP_concat);
    case EXPR_DATE:
        return push_instr_date(ctx, OP_date, ((date_expression_t*)expr)->value);
    case EXPR_DIV:
        return compile_binary_expression(ctx, (binary_expression_t*)expr, OP_div);
    case EXPR_DOT:
        return compile_dot_expression(ctx);
    case EXPR_DOUBLE:
        return push_instr_double(ctx, OP_double, ((double_expression_t*)expr)->value);
    case EXPR_EMPTY:
        return push_instr(ctx, OP_empty) ? S_OK : E_OUTOFMEMORY;
    case EXPR_EQUAL:
        return compile_binary_expression(ctx, (binary_expression_t*)expr, OP_equal);
    case EXPR_EQV:
        return compile_binary_expression(ctx, (binary_expression_t*)expr, OP_eqv);
    case EXPR_EXP:
        return compile_binary_expression(ctx, (binary_expression_t*)expr, OP_exp);
    case EXPR_GT:
        return compile_binary_expression(ctx, (binary_expression_t*)expr, OP_gt);
    case EXPR_GTEQ:
        return compile_binary_expression(ctx, (binary_expression_t*)expr, OP_gteq);
    case EXPR_IDIV:
        return compile_binary_expression(ctx, (binary_expression_t*)expr, OP_idiv);
    case EXPR_IS:
        return compile_binary_expression(ctx, (binary_expression_t*)expr, OP_is);
    case EXPR_IMP:
        return compile_binary_expression(ctx, (binary_expression_t*)expr, OP_imp);
    case EXPR_LT:
        return compile_binary_expression(ctx, (binary_expression_t*)expr, OP_lt);
    case EXPR_LTEQ:
        return compile_binary_expression(ctx, (binary_expression_t*)expr, OP_lteq);
    case EXPR_ME:
        return push_instr(ctx, OP_me) ? S_OK : E_OUTOFMEMORY;
    case EXPR_MEMBER:
        return compile_member_expression(ctx, (member_expression_t*)expr);
    case EXPR_MOD:
        return compile_binary_expression(ctx, (binary_expression_t*)expr, OP_mod);
    case EXPR_MUL:
        return compile_binary_expression(ctx, (binary_expression_t*)expr, OP_mul);
    case EXPR_NEG:
        return compile_unary_expression(ctx, (unary_expression_t*)expr, OP_neg);
    case EXPR_NEQUAL:
        return compile_binary_expression(ctx, (binary_expression_t*)expr, OP_nequal);
    case EXPR_NEW:
        return push_instr_str(ctx, OP_new, ((string_expression_t*)expr)->value);
    case EXPR_NOARG:
        return push_instr_int(ctx, OP_hres, DISP_E_PARAMNOTFOUND);
    case EXPR_NOT:
        return compile_unary_expression(ctx, (unary_expression_t*)expr, OP_not);
    case EXPR_NOTHING:
        return push_instr(ctx, OP_nothing) ? S_OK : E_OUTOFMEMORY;
    case EXPR_NULL:
        return push_instr(ctx, OP_null) ? S_OK : E_OUTOFMEMORY;
    case EXPR_OR:
        return compile_binary_expression(ctx, (binary_expression_t*)expr, OP_or);
    case EXPR_STRING:
        return push_instr_str(ctx, OP_string, ((string_expression_t*)expr)->value);
    case EXPR_SUB:
        return compile_binary_expression(ctx, (binary_expression_t*)expr, OP_sub);
    case EXPR_INT:
        return push_instr_int(ctx, OP_int, ((int_expression_t*)expr)->value);
    case EXPR_XOR:
        return compile_binary_expression(ctx, (binary_expression_t*)expr, OP_xor);
    default:
        FIXME("Unimplemented expression type %d\n", expr->type);
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT compile_if_statement(compile_ctx_t *ctx, if_statement_t *stat)
{
    unsigned cnd_jmp, endif_label = 0;
    elseif_decl_t *elseif_decl;
    HRESULT hres;

    hres = compile_expression(ctx, stat->expr);
    if(FAILED(hres))
        return hres;

    cnd_jmp = push_instr(ctx, OP_jmp_false);
    if(!cnd_jmp)
        return E_OUTOFMEMORY;

    if(!emit_catch(ctx, 0))
        return E_OUTOFMEMORY;

    hres = compile_statement(ctx, NULL, stat->if_stat);
    if(FAILED(hres))
        return hres;

    if(stat->else_stat || stat->elseifs) {
        endif_label = alloc_label(ctx);
        if(!endif_label)
            return E_OUTOFMEMORY;

        hres = push_instr_addr(ctx, OP_jmp, endif_label);
        if(FAILED(hres))
            return hres;
    }

    for(elseif_decl = stat->elseifs; elseif_decl; elseif_decl = elseif_decl->next) {
        instr_ptr(ctx, cnd_jmp)->arg1.uint = ctx->instr_cnt;

        ctx->loc = elseif_decl->loc;

        hres = compile_expression(ctx, elseif_decl->expr);
        if(FAILED(hres))
            return hres;

        cnd_jmp = push_instr(ctx, OP_jmp_false);
        if(!cnd_jmp)
            return E_OUTOFMEMORY;

        if(!emit_catch(ctx, 0))
            return E_OUTOFMEMORY;

        hres = compile_statement(ctx, NULL, elseif_decl->stat);
        if(FAILED(hres))
            return hres;

        hres = push_instr_addr(ctx, OP_jmp, endif_label);
        if(FAILED(hres))
            return hres;
    }

    instr_ptr(ctx, cnd_jmp)->arg1.uint = ctx->instr_cnt;

    if(stat->else_stat) {
        hres = compile_statement(ctx, NULL, stat->else_stat);
        if(FAILED(hres))
            return hres;
    }

    if(endif_label)
        label_set_addr(ctx, endif_label);
    return S_OK;
}

static HRESULT compile_while_statement(compile_ctx_t *ctx, while_statement_t *stat)
{
    statement_ctx_t stat_ctx = {0}, *loop_ctx;
    unsigned start_addr;
    unsigned jmp_end;
    HRESULT hres;

    start_addr = ctx->instr_cnt;

    hres = compile_expression(ctx, stat->expr);
    if(FAILED(hres))
        return hres;

    jmp_end = push_instr(ctx, stat->stat.type == STAT_UNTIL ? OP_jmp_true : OP_jmp_false);
    if(!jmp_end)
        return E_OUTOFMEMORY;

    if(!emit_catch(ctx, 0))
        return E_OUTOFMEMORY;

    if(stat->stat.type == STAT_WHILE) {
        loop_ctx = NULL;
    }else {
        if(!(stat_ctx.while_end_label = alloc_label(ctx)))
            return E_OUTOFMEMORY;
        loop_ctx = &stat_ctx;
    }

    hres = compile_statement(ctx, loop_ctx, stat->body);
    if(FAILED(hres))
        return hres;

    hres = push_instr_addr(ctx, OP_jmp, start_addr);
    if(FAILED(hres))
        return hres;

    instr_ptr(ctx, jmp_end)->arg1.uint = ctx->instr_cnt;

    if(loop_ctx)
        label_set_addr(ctx, stat_ctx.while_end_label);

    return S_OK;
}

static HRESULT compile_dowhile_statement(compile_ctx_t *ctx, while_statement_t *stat)
{
    statement_ctx_t loop_ctx = {0};
    unsigned start_addr;
    vbsop_t jmp_op;
    HRESULT hres;

    start_addr = ctx->instr_cnt;

    if(!(loop_ctx.while_end_label = alloc_label(ctx)))
        return E_OUTOFMEMORY;

    hres = compile_statement(ctx, &loop_ctx, stat->body);
    if(FAILED(hres))
        return hres;

    ctx->loc = stat->stat.loc;
    if(stat->expr) {
        hres = compile_expression(ctx, stat->expr);
        if(FAILED(hres))
            return hres;

        jmp_op = stat->stat.type == STAT_DOUNTIL ? OP_jmp_false : OP_jmp_true;
    }else {
        jmp_op = OP_jmp;
    }

    hres = push_instr_addr(ctx, jmp_op, start_addr);
    if(FAILED(hres))
        return hres;

    label_set_addr(ctx, loop_ctx.while_end_label);

    if(!emit_catch(ctx, 0))
        return E_OUTOFMEMORY;

    return S_OK;
}

static HRESULT compile_foreach_statement(compile_ctx_t *ctx, foreach_statement_t *stat)
{
    statement_ctx_t loop_ctx = {1};
    unsigned loop_start;
    HRESULT hres;

    /* Preserve a place on the stack in case we throw before having proper enum collection. */
    if(!push_instr(ctx, OP_empty))
        return E_OUTOFMEMORY;

    hres = compile_expression(ctx, stat->group_expr);
    if(FAILED(hres))
        return hres;

    if(!push_instr(ctx, OP_newenum))
        return E_OUTOFMEMORY;

    if(!(loop_ctx.for_end_label = alloc_label(ctx)))
        return E_OUTOFMEMORY;

    hres = push_instr_uint_bstr(ctx, OP_enumnext, loop_ctx.for_end_label, stat->identifier);
    if(FAILED(hres))
        return hres;

    if(!emit_catch(ctx, 1))
        return E_OUTOFMEMORY;

    loop_start = ctx->instr_cnt;
    hres = compile_statement(ctx, &loop_ctx, stat->body);
    if(FAILED(hres))
        return hres;

    /* We need a separated enumnext here, because we need to jump out of the loop on exception. */
    ctx->loc = stat->stat.loc;
    hres = push_instr_uint_bstr(ctx, OP_enumnext, loop_ctx.for_end_label, stat->identifier);
    if(FAILED(hres))
        return hres;

    hres = push_instr_addr(ctx, OP_jmp, loop_start);
    if(FAILED(hres))
        return hres;

    label_set_addr(ctx, loop_ctx.for_end_label);

    if(!emit_catch(ctx, 0))
        return E_OUTOFMEMORY;

    return S_OK;
}

static HRESULT compile_forto_statement(compile_ctx_t *ctx, forto_statement_t *stat)
{
    statement_ctx_t loop_ctx = {2};
    unsigned step_instr, instr;
    BSTR identifier;
    HRESULT hres;

    identifier = alloc_bstr_arg(ctx, stat->identifier);
    if(!identifier)
        return E_OUTOFMEMORY;

    hres = compile_expression(ctx, stat->from_expr);
    if(FAILED(hres))
        return hres;
    if(!push_instr(ctx, OP_numval))
        return E_OUTOFMEMORY;

    /* FIXME: Assign should happen after both expressions evaluation. */
    instr = push_instr(ctx, OP_assign_ident);
    if(!instr)
        return E_OUTOFMEMORY;
    instr_ptr(ctx, instr)->arg1.bstr = identifier;
    instr_ptr(ctx, instr)->arg2.uint = 0;

    hres = compile_expression(ctx, stat->to_expr);
    if(FAILED(hres))
        return hres;

    if(!push_instr(ctx, OP_numval))
        return E_OUTOFMEMORY;

    if(stat->step_expr) {
        hres = compile_expression(ctx, stat->step_expr);
        if(FAILED(hres))
            return hres;

        if(!push_instr(ctx, OP_numval))
            return E_OUTOFMEMORY;
    }else {
        hres = push_instr_int(ctx, OP_int, 1);
        if(FAILED(hres))
            return hres;
    }

    loop_ctx.for_end_label = alloc_label(ctx);
    if(!loop_ctx.for_end_label)
        return E_OUTOFMEMORY;

    step_instr = push_instr(ctx, OP_step);
    if(!step_instr)
        return E_OUTOFMEMORY;
    instr_ptr(ctx, step_instr)->arg2.bstr = identifier;
    instr_ptr(ctx, step_instr)->arg1.uint = loop_ctx.for_end_label;

    if(!emit_catch(ctx, 2))
        return E_OUTOFMEMORY;

    hres = compile_statement(ctx, &loop_ctx, stat->body);
    if(FAILED(hres))
        return hres;

    /* FIXME: Error handling can't be done compatible with native using OP_incc here. */
    instr = push_instr(ctx, OP_incc);
    if(!instr)
        return E_OUTOFMEMORY;
    instr_ptr(ctx, instr)->arg1.bstr = identifier;

    hres = push_instr_addr(ctx, OP_jmp, step_instr);
    if(FAILED(hres))
        return hres;

    hres = push_instr_uint(ctx, OP_pop, 2);
    if(FAILED(hres))
        return hres;

    label_set_addr(ctx, loop_ctx.for_end_label);

    /* FIXME: reconsider after OP_incc fixup. */
    if(!emit_catch(ctx, 0))
        return E_OUTOFMEMORY;

    return S_OK;
}

static HRESULT compile_with_statement(compile_ctx_t *ctx, with_statement_t *stat)
{
    statement_ctx_t with_ctx = { 1 };
    HRESULT hres;

    hres = compile_expression(ctx, stat->expr);
    if(FAILED(hres))
        return hres;

    if(!emit_catch(ctx, 1))
        return E_OUTOFMEMORY;

    with_ctx.with_stack_offset = stack_offset(ctx) + 1;
    hres = compile_statement(ctx, &with_ctx, stat->body);
    if(FAILED(hres))
        return hres;

    return push_instr_uint(ctx, OP_pop, 1);
}

static HRESULT compile_select_statement(compile_ctx_t *ctx, select_statement_t *stat)
{
    unsigned end_label, case_cnt = 0, *case_labels = NULL, i;
    case_clausule_t *case_iter;
    expression_t *expr_iter;
    HRESULT hres;

    hres = compile_expression(ctx, stat->expr);
    if(FAILED(hres))
        return hres;

    if(!push_instr(ctx, OP_val))
        return E_OUTOFMEMORY;

    end_label = alloc_label(ctx);
    if(!end_label)
        return E_OUTOFMEMORY;

    if(!emit_catch_jmp(ctx, 0, end_label))
        return E_OUTOFMEMORY;

    for(case_iter = stat->case_clausules; case_iter; case_iter = case_iter->next)
        case_cnt++;

    if(case_cnt) {
        case_labels = malloc(case_cnt*sizeof(*case_labels));
        if(!case_labels)
            return E_OUTOFMEMORY;
    }

    for(case_iter = stat->case_clausules, i=0; case_iter; case_iter = case_iter->next, i++) {
        case_labels[i] = alloc_label(ctx);
        if(!case_labels[i]) {
            hres = E_OUTOFMEMORY;
            break;
        }

        if(!case_iter->expr)
            break;

        for(expr_iter = case_iter->expr; expr_iter; expr_iter = expr_iter->next) {
            hres = compile_expression(ctx, expr_iter);
            if(FAILED(hres))
                break;

            hres = push_instr_addr(ctx, OP_case, case_labels[i]);
            if(FAILED(hres))
                break;

            if(!emit_catch_jmp(ctx, 0, case_labels[i])) {
                hres = E_OUTOFMEMORY;
                break;
            }
        }
    }

    if(FAILED(hres)) {
        free(case_labels);
        return hres;
    }

    hres = push_instr_uint(ctx, OP_pop, 1);
    if(FAILED(hres)) {
        free(case_labels);
        return hres;
    }

    hres = push_instr_addr(ctx, OP_jmp, case_iter ? case_labels[i] : end_label);
    if(FAILED(hres)) {
        free(case_labels);
        return hres;
    }

    for(case_iter = stat->case_clausules, i=0; case_iter; case_iter = case_iter->next, i++) {
        label_set_addr(ctx, case_labels[i]);
        hres = compile_statement(ctx, NULL, case_iter->stat);
        if(FAILED(hres))
            break;

        if(!case_iter->next)
            break;

        hres = push_instr_addr(ctx, OP_jmp, end_label);
        if(FAILED(hres))
            break;
    }

    free(case_labels);
    if(FAILED(hres))
        return hres;

    label_set_addr(ctx, end_label);
    return S_OK;
}

static HRESULT compile_assignment(compile_ctx_t *ctx, expression_t *left, expression_t *value_expr, BOOL is_set)
{
    call_expression_t *call_expr = NULL;
    member_expression_t *member_expr;
    unsigned args_cnt = 0;
    vbsop_t op;
    HRESULT hres;

    switch(left->type) {
    case EXPR_MEMBER:
        member_expr = (member_expression_t*)left;
        break;
    case EXPR_CALL:
        call_expr = (call_expression_t*)left;
        assert(call_expr->call_expr->type == EXPR_MEMBER);
        member_expr = (member_expression_t*)call_expr->call_expr;
        break;
    default:
        assert(0);
        return E_FAIL;
    }

    if(member_expr->obj_expr) {
        hres = compile_expression(ctx, member_expr->obj_expr);
        if(FAILED(hres))
            return hres;

        op = is_set ? OP_set_member : OP_assign_member;
    }else {
        op = is_set ? OP_set_ident : OP_assign_ident;
    }

    hres = compile_expression(ctx, value_expr);
    if(FAILED(hres))
        return hres;

    if(call_expr) {
        hres = compile_args(ctx, call_expr->args, &args_cnt);
        if(FAILED(hres))
            return hres;
    }

    hres = push_instr_bstr_uint(ctx, op, member_expr->identifier, args_cnt);
    if(FAILED(hres))
        return hres;

    if(!emit_catch(ctx, 0))
        return E_OUTOFMEMORY;

    return S_OK;
}

static HRESULT compile_assign_statement(compile_ctx_t *ctx, assign_statement_t *stat, BOOL is_set)
{
    return compile_assignment(ctx, stat->left_expr, stat->value_expr, is_set);
}

static HRESULT compile_call_statement(compile_ctx_t *ctx, call_statement_t *stat)
{
    HRESULT hres;

    hres = compile_call_expression(ctx, stat->expr, FALSE);
    if(FAILED(hres))
        return hres;

    if(!emit_catch(ctx, 0))
        return E_OUTOFMEMORY;

    return S_OK;
}

static HRESULT compile_dim_statement(compile_ctx_t *ctx, dim_statement_t *stat)
{
    dim_decl_t *dim_decl = stat->dim_decls;

    while(1) {
        if(lookup_dim_decls(ctx, dim_decl->name) || lookup_args_name(ctx, dim_decl->name)
           || lookup_const_decls(ctx, dim_decl->name, FALSE)) {
            FIXME("dim %s name redefined\n", debugstr_w(dim_decl->name));
            return E_FAIL;
        }

        ctx->func->var_cnt++;

        if(dim_decl->is_array) {
            HRESULT hres = push_instr_bstr_uint(ctx, OP_dim, dim_decl->name, ctx->func->array_cnt++);
            if(FAILED(hres))
                return hres;

            if(!emit_catch(ctx, 0))
                return E_OUTOFMEMORY;
        }

        if(!dim_decl->next)
            break;
        dim_decl = dim_decl->next;
    }

    if(ctx->dim_decls_tail)
        ctx->dim_decls_tail->next = stat->dim_decls;
    else
        ctx->dim_decls = stat->dim_decls;
    ctx->dim_decls_tail = dim_decl;
    return S_OK;
}

static HRESULT compile_redim_statement(compile_ctx_t *ctx, redim_statement_t *stat)
{
    redim_decl_t *decl = stat->redim_decls;
    unsigned arg_cnt;
    HRESULT hres;

    while(1) {
        hres = compile_args(ctx, decl->dims, &arg_cnt);
        if(FAILED(hres))
            return hres;

        hres = push_instr_bstr_uint(ctx, stat->preserve ? OP_redim_preserve : OP_redim, decl->identifier, arg_cnt);
        if(FAILED(hres))
            return hres;

        if(!emit_catch(ctx, 0))
            return E_OUTOFMEMORY;

        if(!decl->next)
            break;
        decl = decl->next;
    }

    return S_OK;
}

static HRESULT compile_const_statement(compile_ctx_t *ctx, const_statement_t *stat)
{
    const_decl_t *decl, *next_decl = stat->decls;

    do {
        decl = next_decl;

        if(lookup_const_decls(ctx, decl->name, FALSE) || lookup_args_name(ctx, decl->name)
                || lookup_dim_decls(ctx, decl->name)) {
            FIXME("%s redefined\n", debugstr_w(decl->name));
            return E_FAIL;
        }

        if(ctx->func->type == FUNC_GLOBAL) {
            HRESULT hres;

            hres = compile_expression(ctx, decl->value_expr);
            if(FAILED(hres))
                return hres;

            hres = push_instr_bstr(ctx, OP_const, decl->name);
            if(FAILED(hres))
                return hres;

            if(!emit_catch(ctx, 0))
                return E_OUTOFMEMORY;
        }

        next_decl = decl->next;
        decl->next = ctx->const_decls;
        ctx->const_decls = decl;
    } while(next_decl);

    return S_OK;
}

static HRESULT compile_function_statement(compile_ctx_t *ctx, function_statement_t *stat)
{
    if(ctx->func != &ctx->code->main_code) {
        FIXME("Function is not in the global code\n");
        return E_FAIL;
    }

    stat->func_decl->next = ctx->func_decls;
    ctx->func_decls = stat->func_decl;
    return S_OK;
}

static HRESULT compile_exitdo_statement(compile_ctx_t *ctx)
{
    statement_ctx_t *iter;
    unsigned pop_cnt = 0;

    for(iter = ctx->stat_ctx; iter; iter = iter->next) {
        pop_cnt += iter->stack_use;
        if(iter->while_end_label)
            break;
    }
    if(!iter) {
        FIXME("Exit Do outside Do Loop\n");
        return E_FAIL;
    }

    if(pop_cnt) {
        HRESULT hres;

        hres = push_instr_uint(ctx, OP_pop, pop_cnt);
        if(FAILED(hres))
            return hres;
    }

    return push_instr_addr(ctx, OP_jmp, iter->while_end_label);
}

static HRESULT compile_exitfor_statement(compile_ctx_t *ctx)
{
    statement_ctx_t *iter;
    unsigned pop_cnt = 0;

    for(iter = ctx->stat_ctx; iter; iter = iter->next) {
        pop_cnt += iter->stack_use;
        if(iter->for_end_label)
            break;
    }
    if(!iter) {
        FIXME("Exit For outside For loop\n");
        return E_FAIL;
    }

    if(pop_cnt) {
        HRESULT hres;

        hres = push_instr_uint(ctx, OP_pop, pop_cnt);
        if(FAILED(hres))
            return hres;
    }

    return push_instr_addr(ctx, OP_jmp, iter->for_end_label);
}

static HRESULT exit_label(compile_ctx_t *ctx, unsigned jmp_label)
{
    unsigned pop_cnt = stack_offset(ctx);

    if(pop_cnt) {
        HRESULT hres;

        hres = push_instr_uint(ctx, OP_pop, pop_cnt);
        if(FAILED(hres))
            return hres;
    }

    return push_instr_addr(ctx, OP_jmp, jmp_label);
}

static HRESULT compile_exitsub_statement(compile_ctx_t *ctx)
{
    if(!ctx->sub_end_label) {
        FIXME("Exit Sub outside Sub?\n");
        return E_FAIL;
    }

    return exit_label(ctx, ctx->sub_end_label);
}

static HRESULT compile_exitfunc_statement(compile_ctx_t *ctx)
{
    if(!ctx->func_end_label) {
        FIXME("Exit Function outside Function?\n");
        return E_FAIL;
    }

    return exit_label(ctx, ctx->func_end_label);
}

static HRESULT compile_exitprop_statement(compile_ctx_t *ctx)
{
    if(!ctx->prop_end_label) {
        FIXME("Exit Property outside Property?\n");
        return E_FAIL;
    }

    return exit_label(ctx, ctx->prop_end_label);
}

static HRESULT compile_onerror_statement(compile_ctx_t *ctx, onerror_statement_t *stat)
{
    return push_instr_int(ctx, OP_errmode, stat->resume_next);
}

static HRESULT compile_retval_statement(compile_ctx_t *ctx, retval_statement_t *stat)
{
    HRESULT hres;

    hres = compile_expression(ctx, stat->expr);
    if(FAILED(hres))
        return hres;

    hres = push_instr(ctx, OP_retval);
    if(FAILED(hres))
        return hres;

    return S_OK;
}

static HRESULT compile_statement(compile_ctx_t *ctx, statement_ctx_t *stat_ctx, statement_t *stat)
{
    HRESULT hres;

    if(stat_ctx) {
        stat_ctx->next = ctx->stat_ctx;
        ctx->stat_ctx = stat_ctx;
    }

    while(stat) {
        ctx->loc = stat->loc;

        switch(stat->type) {
        case STAT_ASSIGN:
            hres = compile_assign_statement(ctx, (assign_statement_t*)stat, FALSE);
            break;
        case STAT_CALL:
            hres = compile_call_statement(ctx, (call_statement_t*)stat);
            break;
        case STAT_CONST:
            hres = compile_const_statement(ctx, (const_statement_t*)stat);
            break;
        case STAT_DIM:
            hres = compile_dim_statement(ctx, (dim_statement_t*)stat);
            break;
        case STAT_DOWHILE:
        case STAT_DOUNTIL:
            hres = compile_dowhile_statement(ctx, (while_statement_t*)stat);
            break;
        case STAT_EXITDO:
            hres = compile_exitdo_statement(ctx);
            break;
        case STAT_EXITFOR:
            hres = compile_exitfor_statement(ctx);
            break;
        case STAT_EXITFUNC:
            hres = compile_exitfunc_statement(ctx);
            break;
        case STAT_EXITPROP:
            hres = compile_exitprop_statement(ctx);
            break;
        case STAT_EXITSUB:
            hres = compile_exitsub_statement(ctx);
            break;
        case STAT_FOREACH:
            hres = compile_foreach_statement(ctx, (foreach_statement_t*)stat);
            break;
        case STAT_FORTO:
            hres = compile_forto_statement(ctx, (forto_statement_t*)stat);
            break;
        case STAT_FUNC:
            hres = compile_function_statement(ctx, (function_statement_t*)stat);
            break;
        case STAT_IF:
            hres = compile_if_statement(ctx, (if_statement_t*)stat);
            break;
        case STAT_ONERROR:
            hres = compile_onerror_statement(ctx, (onerror_statement_t*)stat);
            break;
        case STAT_REDIM:
            hres = compile_redim_statement(ctx, (redim_statement_t*)stat);
            break;
        case STAT_SELECT:
            hres = compile_select_statement(ctx, (select_statement_t*)stat);
            break;
        case STAT_SET:
            hres = compile_assign_statement(ctx, (assign_statement_t*)stat, TRUE);
            break;
        case STAT_STOP:
            hres = push_instr(ctx, OP_stop) ? S_OK : E_OUTOFMEMORY;
            break;
        case STAT_UNTIL:
        case STAT_WHILE:
        case STAT_WHILELOOP:
            hres = compile_while_statement(ctx, (while_statement_t*)stat);
            break;
        case STAT_WITH:
            hres = compile_with_statement(ctx, (with_statement_t*)stat);
            break;
        case STAT_RETVAL:
            hres = compile_retval_statement(ctx, (retval_statement_t*)stat);
            break;
        default:
            FIXME("Unimplemented statement type %d\n", stat->type);
            hres = E_NOTIMPL;
        }

        if(FAILED(hres))
            return hres;
        stat = stat->next;
    }

    if(stat_ctx) {
        assert(ctx->stat_ctx == stat_ctx);
        ctx->stat_ctx = stat_ctx->next;
    }

    return S_OK;
}

static void resolve_labels(compile_ctx_t *ctx, unsigned off)
{
    instr_t *instr;

    for(instr = ctx->code->instrs+off; instr < ctx->code->instrs+ctx->instr_cnt; instr++) {
        if(instr_info[instr->op].arg1_type == ARG_ADDR && (instr->arg1.uint & LABEL_FLAG)) {
            assert((instr->arg1.uint & ~LABEL_FLAG) < ctx->labels_cnt);
            instr->arg1.uint = ctx->labels[instr->arg1.uint & ~LABEL_FLAG];
        }
        assert(instr_info[instr->op].arg2_type != ARG_ADDR);
    }

    ctx->labels_cnt = 0;
}

static HRESULT fill_array_desc(compile_ctx_t *ctx, dim_decl_t *dim_decl, array_desc_t *array_desc)
{
    unsigned dim_cnt = 0, i;
    dim_list_t *iter;

    for(iter = dim_decl->dims; iter; iter = iter->next)
        dim_cnt++;

    array_desc->bounds = compiler_alloc(ctx->code, dim_cnt * sizeof(SAFEARRAYBOUND));
    if(!array_desc->bounds)
        return E_OUTOFMEMORY;

    array_desc->dim_cnt = dim_cnt;

    for(iter = dim_decl->dims, i=0; iter; iter = iter->next, i++) {
        array_desc->bounds[i].cElements = iter->val+1;
        array_desc->bounds[i].lLbound = 0;
    }

    return S_OK;
}

static HRESULT compile_func(compile_ctx_t *ctx, statement_t *stat, function_t *func)
{
    HRESULT hres;

    func->code_off = ctx->instr_cnt;

    ctx->sub_end_label = 0;
    ctx->func_end_label = 0;
    ctx->prop_end_label = 0;

    switch(func->type) {
    case FUNC_FUNCTION:
        ctx->func_end_label = alloc_label(ctx);
        if(!ctx->func_end_label)
            return E_OUTOFMEMORY;
        break;
    case FUNC_SUB:
        ctx->sub_end_label = alloc_label(ctx);
        if(!ctx->sub_end_label)
            return E_OUTOFMEMORY;
        break;
    case FUNC_PROPGET:
    case FUNC_PROPLET:
    case FUNC_PROPSET:
        ctx->prop_end_label = alloc_label(ctx);
        if(!ctx->prop_end_label)
            return E_OUTOFMEMORY;
        break;
    case FUNC_GLOBAL:
        break;
    }

    ctx->func = func;
    ctx->dim_decls = ctx->dim_decls_tail = NULL;
    ctx->const_decls = NULL;
    hres = compile_statement(ctx, NULL, stat);
    ctx->func = NULL;
    if(FAILED(hres))
        return hres;

    if(ctx->sub_end_label)
        label_set_addr(ctx, ctx->sub_end_label);
    if(ctx->func_end_label)
        label_set_addr(ctx, ctx->func_end_label);
    if(ctx->prop_end_label)
        label_set_addr(ctx, ctx->prop_end_label);

    if(!push_instr(ctx, OP_ret))
        return E_OUTOFMEMORY;

    resolve_labels(ctx, func->code_off);

    if(func->var_cnt) {
        dim_decl_t *dim_decl;
        unsigned i;

        func->vars = compiler_alloc(ctx->code, func->var_cnt * sizeof(var_desc_t));
        if(!func->vars)
            return E_OUTOFMEMORY;

        for(dim_decl = ctx->dim_decls, i=0; dim_decl; dim_decl = dim_decl->next, i++) {
            func->vars[i].name = compiler_alloc_string(ctx->code, dim_decl->name);
            if(!func->vars[i].name)
                return E_OUTOFMEMORY;
        }

        assert(i == func->var_cnt);
    }

    if(func->array_cnt) {
        unsigned array_id = 0;
        dim_decl_t *dim_decl;

        func->array_descs = compiler_alloc(ctx->code, func->array_cnt * sizeof(array_desc_t));
        if(!func->array_descs)
            return E_OUTOFMEMORY;

        for(dim_decl = ctx->dim_decls; dim_decl; dim_decl = dim_decl->next) {
            if(dim_decl->is_array) {
                hres = fill_array_desc(ctx, dim_decl, func->array_descs + array_id++);
                if(FAILED(hres))
                    return hres;
            }
        }

        assert(array_id == func->array_cnt);
    }

    return S_OK;
}

static BOOL lookup_funcs_name(compile_ctx_t *ctx, const WCHAR *name)
{
    function_t *iter;

    for(iter = ctx->code->funcs; iter; iter = iter->next) {
        if(!wcsicmp(iter->name, name))
            return TRUE;
    }

    return FALSE;
}

static HRESULT create_function(compile_ctx_t *ctx, function_decl_t *decl, function_t **ret)
{
    function_t *func;
    HRESULT hres;

    if(lookup_dim_decls(ctx, decl->name) || lookup_const_decls(ctx, decl->name, FALSE)) {
        FIXME("%s: redefinition\n", debugstr_w(decl->name));
        return E_FAIL;
    }

    func = compiler_alloc(ctx->code, sizeof(*func));
    if(!func)
        return E_OUTOFMEMORY;

    func->name = compiler_alloc_string(ctx->code, decl->name);
    if(!func->name)
        return E_OUTOFMEMORY;

    func->vars = NULL;
    func->var_cnt = 0;
    func->array_cnt = 0;
    func->code_ctx = ctx->code;
    func->type = decl->type;
    func->is_public = decl->is_public;

    func->arg_cnt = 0;
    if(decl->args) {
        arg_decl_t *arg;
        unsigned i;

        for(arg = decl->args; arg; arg = arg->next)
            func->arg_cnt++;

        func->args = compiler_alloc(ctx->code, func->arg_cnt * sizeof(arg_desc_t));
        if(!func->args)
            return E_OUTOFMEMORY;

        for(i = 0, arg = decl->args; arg; arg = arg->next, i++) {
            func->args[i].name = compiler_alloc_string(ctx->code, arg->name);
            if(!func->args[i].name)
                return E_OUTOFMEMORY;
            func->args[i].by_ref = arg->by_ref;
        }
    }else {
        func->args = NULL;
    }

    hres = compile_func(ctx, decl->body, func);
    if(FAILED(hres))
        return hres;

    *ret = func;
    return S_OK;
}

static BOOL lookup_class_name(compile_ctx_t *ctx, const WCHAR *name)
{
    class_desc_t *iter;

    for(iter = ctx->code->classes; iter; iter = iter->next) {
        if(!wcsicmp(iter->name, name))
            return TRUE;
    }

    return FALSE;
}

static HRESULT create_class_funcprop(compile_ctx_t *ctx, function_decl_t *func_decl, vbdisp_funcprop_desc_t *desc)
{
    vbdisp_invoke_type_t invoke_type;
    function_decl_t *funcprop_decl;
    HRESULT hres;

    desc->name = compiler_alloc_string(ctx->code, func_decl->name);
    if(!desc->name)
        return E_OUTOFMEMORY;

    for(funcprop_decl = func_decl; funcprop_decl; funcprop_decl = funcprop_decl->next_prop_func) {
        switch(funcprop_decl->type) {
        case FUNC_FUNCTION:
        case FUNC_SUB:
        case FUNC_PROPGET:
            invoke_type = VBDISP_CALLGET;
            break;
        case FUNC_PROPLET:
            invoke_type = VBDISP_LET;
            break;
        case FUNC_PROPSET:
            invoke_type = VBDISP_SET;
            break;
        DEFAULT_UNREACHABLE;
        }

        assert(!desc->entries[invoke_type]);

        if(funcprop_decl->is_public)
            desc->is_public = TRUE;

        hres = create_function(ctx, funcprop_decl, desc->entries+invoke_type);
        if(FAILED(hres))
            return hres;
    }

    return S_OK;
}

static BOOL lookup_class_funcs(class_desc_t *class_desc, const WCHAR *name)
{
    unsigned i;

    for(i=0; i < class_desc->func_cnt; i++) {
        if(class_desc->funcs[i].name && !wcsicmp(class_desc->funcs[i].name, name))
            return TRUE;
    }

    return FALSE;
}

static HRESULT compile_class(compile_ctx_t *ctx, class_decl_t *class_decl)
{
    function_decl_t *func_decl, *func_prop_decl;
    BOOL is_default, have_default = FALSE;
    class_desc_t *class_desc;
    dim_decl_t *prop_decl;
    unsigned i;
    HRESULT hres;

    if(lookup_dim_decls(ctx, class_decl->name) || lookup_funcs_name(ctx, class_decl->name)
            || lookup_const_decls(ctx, class_decl->name, FALSE) || lookup_class_name(ctx, class_decl->name)) {
        FIXME("%s: redefinition\n", debugstr_w(class_decl->name));
        return E_FAIL;
    }

    class_desc = compiler_alloc_zero(ctx->code, sizeof(*class_desc));
    if(!class_desc)
        return E_OUTOFMEMORY;

    class_desc->name = compiler_alloc_string(ctx->code, class_decl->name);
    if(!class_desc->name)
        return E_OUTOFMEMORY;

    class_desc->func_cnt = 1; /* always allocate slot for default getter or method */

    for(func_decl = class_decl->funcs; func_decl; func_decl = func_decl->next) {
        is_default = FALSE;
        for(func_prop_decl = func_decl; func_prop_decl; func_prop_decl = func_prop_decl->next_prop_func) {
            if(func_prop_decl->is_default) {
                if(have_default) {
                    FIXME("multiple default getters or methods\n");
                    return E_FAIL;
                }
                is_default = have_default = TRUE;
                break;
            }
        }
        if(!is_default)
            class_desc->func_cnt++;
    }

    class_desc->funcs = compiler_alloc(ctx->code, class_desc->func_cnt*sizeof(*class_desc->funcs));
    if(!class_desc->funcs)
        return E_OUTOFMEMORY;
    memset(class_desc->funcs, 0, class_desc->func_cnt*sizeof(*class_desc->funcs));

    for(func_decl = class_decl->funcs, i=1; func_decl; func_decl = func_decl->next, i++) {
        for(func_prop_decl = func_decl; func_prop_decl; func_prop_decl = func_prop_decl->next_prop_func) {
            if(func_prop_decl->is_default) {
                i--;
                break;
            }
        }

        if(!wcsicmp(L"class_initialize", func_decl->name)) {
            if(func_decl->type != FUNC_SUB) {
                FIXME("class initializer is not sub\n");
                return E_FAIL;
            }

            class_desc->class_initialize_id = i;
        }else  if(!wcsicmp(L"class_terminate", func_decl->name)) {
            if(func_decl->type != FUNC_SUB) {
                FIXME("class terminator is not sub\n");
                return E_FAIL;
            }

            class_desc->class_terminate_id = i;
        }

        hres = create_class_funcprop(ctx, func_decl, class_desc->funcs + (func_prop_decl ? 0 : i));
        if(FAILED(hres))
            return hres;
    }

    for(prop_decl = class_decl->props; prop_decl; prop_decl = prop_decl->next)
        class_desc->prop_cnt++;

    class_desc->props = compiler_alloc(ctx->code, class_desc->prop_cnt*sizeof(*class_desc->props));
    if(!class_desc->props)
        return E_OUTOFMEMORY;

    for(prop_decl = class_decl->props, i=0; prop_decl; prop_decl = prop_decl->next, i++) {
        if(lookup_class_funcs(class_desc, prop_decl->name)) {
            FIXME("Property %s redefined\n", debugstr_w(prop_decl->name));
            return E_FAIL;
        }

        class_desc->props[i].name = compiler_alloc_string(ctx->code, prop_decl->name);
        if(!class_desc->props[i].name)
            return E_OUTOFMEMORY;

        class_desc->props[i].is_public = prop_decl->is_public;
        class_desc->props[i].is_array = prop_decl->is_array;

        if(prop_decl->is_array)
            class_desc->array_cnt++;
    }

    if(class_desc->array_cnt) {
        class_desc->array_descs = compiler_alloc(ctx->code, class_desc->array_cnt*sizeof(*class_desc->array_descs));
        if(!class_desc->array_descs)
            return E_OUTOFMEMORY;

        for(prop_decl = class_decl->props, i=0; prop_decl; prop_decl = prop_decl->next) {
            if(prop_decl->is_array) {
                hres = fill_array_desc(ctx, prop_decl, class_desc->array_descs + i++);
                if(FAILED(hres))
                    return hres;
            }
        }
    }

    class_desc->next = ctx->code->classes;
    ctx->code->classes = class_desc;
    return S_OK;
}

static BOOL lookup_script_identifier(compile_ctx_t *ctx, script_ctx_t *script, const WCHAR *identifier)
{
    ScriptDisp *contexts[] = {
        ctx->code->named_item ? ctx->code->named_item->script_obj : NULL,
        script->script_obj
    };
    class_desc_t *class;
    vbscode_t *code;
    unsigned c, i;

    for(c = 0; c < ARRAY_SIZE(contexts); c++) {
        if(!contexts[c]) continue;

        for(i = 0; i < contexts[c]->global_vars_cnt; i++) {
            if(!wcsicmp(contexts[c]->global_vars[i]->name, identifier))
                return TRUE;
        }

        for(i = 0; i < contexts[c]->global_funcs_cnt; i++) {
            if(!wcsicmp(contexts[c]->global_funcs[i]->name, identifier))
                return TRUE;
        }

        for(class = contexts[c]->classes; class; class = class->next) {
            if(!wcsicmp(class->name, identifier))
                return TRUE;
        }
    }

    LIST_FOR_EACH_ENTRY(code, &script->code_list, vbscode_t, entry) {
        unsigned var_cnt = code->main_code.var_cnt;
        var_desc_t *vars = code->main_code.vars;
        function_t *func;

        if(!code->pending_exec || (code->named_item && code->named_item != ctx->code->named_item))
            continue;

        for(i = 0; i < var_cnt; i++) {
            if(!wcsicmp(vars[i].name, identifier))
                return TRUE;
        }

        for(func = code->funcs; func; func = func->next) {
            if(!wcsicmp(func->name, identifier))
                return TRUE;
        }

        for(class = code->classes; class; class = class->next) {
            if(!wcsicmp(class->name, identifier))
                return TRUE;
        }
    }

    return FALSE;
}

static HRESULT check_script_collisions(compile_ctx_t *ctx, script_ctx_t *script)
{
    unsigned i, var_cnt = ctx->code->main_code.var_cnt;
    var_desc_t *vars = ctx->code->main_code.vars;
    class_desc_t *class;

    for(i = 0; i < var_cnt; i++) {
        if(lookup_script_identifier(ctx, script, vars[i].name)) {
            FIXME("%s: redefined\n", debugstr_w(vars[i].name));
            return E_FAIL;
        }
    }

    for(class = ctx->code->classes; class; class = class->next) {
        if(lookup_script_identifier(ctx, script, class->name)) {
            FIXME("%s: redefined\n", debugstr_w(class->name));
            return E_FAIL;
        }
    }

    return S_OK;
}

void release_vbscode(vbscode_t *code)
{
    unsigned i;

    if(--code->ref)
        return;

    for(i=0; i < code->bstr_cnt; i++)
        SysFreeString(code->bstr_pool[i]);

    if(code->named_item)
        release_named_item(code->named_item);
    heap_pool_free(&code->heap);

    free(code->bstr_pool);
    free(code->source);
    free(code->instrs);
    free(code);
}

static vbscode_t *alloc_vbscode(compile_ctx_t *ctx, const WCHAR *source, DWORD_PTR cookie, unsigned start_line)
{
    vbscode_t *ret;
    size_t len;

    len = source ? lstrlenW(source) : 0;
    if(len > INT32_MAX)
        return NULL;

    ret = calloc(1, sizeof(*ret));
    if(!ret)
        return NULL;

    ret->source = malloc((len + 1) * sizeof(WCHAR));
    if(!ret->source) {
        free(ret);
        return NULL;
    }
    if(len)
        memcpy(ret->source, source, len * sizeof(WCHAR));
    ret->source[len] = 0;

    ret->ref = 1;
    ret->cookie = cookie;
    ret->start_line = start_line;

    ret->instrs = malloc(32*sizeof(instr_t));
    if(!ret->instrs) {
        release_vbscode(ret);
        return NULL;
    }

    ctx->instr_cnt = 1;
    ctx->instr_size = 32;
    heap_pool_init(&ret->heap);

    ret->main_code.type = FUNC_GLOBAL;
    ret->main_code.code_ctx = ret;

    list_init(&ret->entry);
    return ret;
}

static void release_compiler(compile_ctx_t *ctx)
{
    parser_release(&ctx->parser);
    free(ctx->labels);
    if(ctx->code)
        release_vbscode(ctx->code);
}

HRESULT compile_script(script_ctx_t *script, const WCHAR *src, const WCHAR *item_name, const WCHAR *delimiter,
                       DWORD_PTR cookie, unsigned start_line, DWORD flags, vbscode_t **ret)
{
    function_decl_t *func_decl;
    named_item_t *item = NULL;
    class_decl_t *class_decl;
    function_t *new_func;
    compile_ctx_t ctx;
    vbscode_t *code;
    HRESULT hres;

    if(item_name) {
        item = lookup_named_item(script, item_name, 0);
        if(!item) {
            WARN("Unknown context %s\n", debugstr_w(item_name));
            return E_INVALIDARG;
        }
        if(!item->script_obj) item = NULL;
    }

    memset(&ctx, 0, sizeof(ctx));
    code = ctx.code = alloc_vbscode(&ctx, src, cookie, start_line);
    if(!ctx.code)
        return E_OUTOFMEMORY;
    if(item) {
        code->named_item = item;
        item->ref++;
    }

    ctx.parser.lcid = script->lcid;
    hres = parse_script(&ctx.parser, code->source, delimiter, flags);
    if(FAILED(hres)) {
        if(ctx.parser.error_loc != -1)
            ctx.loc = ctx.parser.error_loc;
        hres = compile_error(script, &ctx, hres);
        release_vbscode(code);
        return hres;
    }

    hres = compile_func(&ctx, ctx.parser.stats, &ctx.code->main_code);
    if(FAILED(hres)) {
        hres = compile_error(script, &ctx, hres);
        release_compiler(&ctx);
        return hres;
    }

    code->option_explicit = ctx.parser.option_explicit;
    ctx.global_consts = ctx.const_decls;
    code->option_explicit = ctx.parser.option_explicit;


    for(func_decl = ctx.func_decls; func_decl; func_decl = func_decl->next) {
        hres = create_function(&ctx, func_decl, &new_func);
        if(FAILED(hres)) {
            hres = compile_error(script, &ctx, hres);
            release_compiler(&ctx);
            return hres;
        }

        new_func->next = ctx.code->funcs;
        ctx.code->funcs = new_func;
    }

    for(class_decl = ctx.parser.class_decls; class_decl; class_decl = class_decl->next) {
        hres = compile_class(&ctx, class_decl);
        if(FAILED(hres)) {
            hres = compile_error(script, &ctx, hres);
            release_compiler(&ctx);
            return hres;
        }
    }

    hres = check_script_collisions(&ctx, script);
    if(FAILED(hres)) {
        hres = compile_error(script, &ctx, hres);
        release_compiler(&ctx);
        return hres;
    }

    code->is_persistent = (flags & SCRIPTTEXT_ISPERSISTENT) != 0;

    if(TRACE_ON(vbscript_disas))
        dump_code(&ctx);

    ctx.code = NULL;
    release_compiler(&ctx);

    list_add_tail(&script->code_list, &code->entry);
    *ret = code;
    return S_OK;
}

HRESULT compile_procedure(script_ctx_t *script, const WCHAR *src, const WCHAR *item_name, const WCHAR *delimiter,
                          DWORD_PTR cookie, unsigned start_line, DWORD flags, class_desc_t **ret)
{
    class_desc_t *desc;
    vbscode_t *code;
    HRESULT hres;

    hres = compile_script(script, src, item_name, delimiter, cookie, start_line,
                          flags & ~SCRIPTTEXT_ISPERSISTENT, &code);
    if(FAILED(hres))
        return hres;

    if(!(desc = compiler_alloc_zero(code, sizeof(*desc))))
        return E_OUTOFMEMORY;
    if(!(desc->funcs = compiler_alloc_zero(code, sizeof(*desc->funcs))))
        return E_OUTOFMEMORY;

    desc->ctx = script;
    desc->func_cnt = 1;
    desc->funcs->entries[VBDISP_CALLGET] = &code->main_code;

    *ret = desc;
    return S_OK;
}
