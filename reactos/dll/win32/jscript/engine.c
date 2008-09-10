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

#include "jscript.h"
#include "engine.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

static inline HRESULT stat_eval(exec_ctx_t *ctx, statement_t *stat, return_type_t *rt, VARIANT *ret)
{
    return stat->eval(ctx, stat, rt, ret);
}

HRESULT create_exec_ctx(exec_ctx_t **ret)
{
    exec_ctx_t *ctx;

    ctx = heap_alloc_zero(sizeof(exec_ctx_t));
    if(!ctx)
        return E_OUTOFMEMORY;

    *ret = ctx;
    return S_OK;
}

void exec_release(exec_ctx_t *ctx)
{
    if(--ctx->ref)
        return;

    heap_free(ctx);
}

HRESULT exec_source(exec_ctx_t *ctx, parser_ctx_t *parser, source_elements_t *source, jsexcept_t *ei, VARIANT *retv)
{
    script_ctx_t *script = parser->script;
    parser_ctx_t *prev_parser;
    VARIANT val, tmp;
    statement_t *stat;
    exec_ctx_t *prev_ctx;
    return_type_t rt;
    HRESULT hres = S_OK;

    prev_ctx = script->exec_ctx;
    script->exec_ctx = ctx;

    prev_parser = ctx->parser;
    ctx->parser = parser;

    V_VT(&val) = VT_EMPTY;
    memset(&rt, 0, sizeof(rt));
    rt.type = RT_NORMAL;

    for(stat = source->statement; stat; stat = stat->next) {
        hres = stat_eval(ctx, stat, &rt, &tmp);
        if(FAILED(hres))
            break;

        VariantClear(&val);
        val = tmp;
        if(rt.type != RT_NORMAL)
            break;
    }

    script->exec_ctx = prev_ctx;
    ctx->parser = prev_parser;

    if(rt.type != RT_NORMAL && rt.type != RT_RETURN) {
        FIXME("wrong rt %d\n", rt.type);
        hres = E_FAIL;
    }

    *ei = rt.ei;
    if(FAILED(hres)) {
        VariantClear(&val);
        return hres;
    }

    if(retv)
        *retv = val;
    else
        VariantClear(&val);
    return S_OK;
}

HRESULT block_statement_eval(exec_ctx_t *ctx, statement_t *stat, return_type_t *rt, VARIANT *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT var_statement_eval(exec_ctx_t *ctx, statement_t *stat, return_type_t *rt, VARIANT *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT empty_statement_eval(exec_ctx_t *ctx, statement_t *stat, return_type_t *rt, VARIANT *ret)
{
    TRACE("\n");

    V_VT(ret) = VT_EMPTY;
    return S_OK;
}

HRESULT expression_statement_eval(exec_ctx_t *ctx, statement_t *stat, return_type_t *rt, VARIANT *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT if_statement_eval(exec_ctx_t *ctx, statement_t *stat, return_type_t *rt, VARIANT *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT dowhile_statement_eval(exec_ctx_t *ctx, statement_t *stat, return_type_t *rt, VARIANT *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT while_statement_eval(exec_ctx_t *ctx, statement_t *stat, return_type_t *rt, VARIANT *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT for_statement_eval(exec_ctx_t *ctx, statement_t *stat, return_type_t *rt, VARIANT *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT forin_statement_eval(exec_ctx_t *ctx, statement_t *stat, return_type_t *rt, VARIANT *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT continue_statement_eval(exec_ctx_t *ctx, statement_t *stat, return_type_t *rt, VARIANT *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT break_statement_eval(exec_ctx_t *ctx, statement_t *stat, return_type_t *rt, VARIANT *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT return_statement_eval(exec_ctx_t *ctx, statement_t *stat, return_type_t *rt, VARIANT *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT with_statement_eval(exec_ctx_t *ctx, statement_t *stat, return_type_t *rt, VARIANT *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT labelled_statement_eval(exec_ctx_t *ctx, statement_t *stat, return_type_t *rt, VARIANT *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT switch_statement_eval(exec_ctx_t *ctx, statement_t *stat, return_type_t *rt, VARIANT *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT throw_statement_eval(exec_ctx_t *ctx, statement_t *stat, return_type_t *rt, VARIANT *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT try_statement_eval(exec_ctx_t *ctx, statement_t *stat, return_type_t *rt, VARIANT *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT function_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT conditional_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT array_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT member_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT member_new_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT call_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT this_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT identifier_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT literal_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT array_literal_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT property_value_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT comma_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT logical_or_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT logical_and_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT binary_or_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT binary_xor_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT binary_and_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT instanceof_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT in_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT add_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT sub_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT mul_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT div_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT mod_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT delete_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT void_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT typeof_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT minus_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT plus_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT post_increment_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT post_decrement_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT pre_increment_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT pre_decrement_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT new_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT equal_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT equal2_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT not_equal_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{

    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT not_equal2_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT less_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT lesseq_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT greater_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT greatereq_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT binary_negation_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT logical_negation_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT left_shift_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT right_shift_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT right2_shift_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT assign_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT assign_lshift_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT assign_rshift_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT assign_rrshift_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT assign_add_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT assign_sub_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT assign_mul_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT assign_div_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT assign_mod_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT assign_and_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT assign_or_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

HRESULT assign_xor_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}
