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

#include "config.h"
#include "wine/port.h"

#include <math.h>

#include "jscript.h"
#include "engine.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

#define EXPR_NOVAL   0x0001
#define EXPR_NEWREF  0x0002
#define EXPR_STRREF  0x0004

static inline HRESULT stat_eval(exec_ctx_t *ctx, statement_t *stat, return_type_t *rt, VARIANT *ret)
{
    return stat->eval(ctx, stat, rt, ret);
}

static inline HRESULT expr_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    return _expr->eval(ctx, _expr, flags, ei, ret);
}

static void exprval_release(exprval_t *val)
{
    switch(val->type) {
    case EXPRVAL_VARIANT:
        if(V_VT(&val->u.var) != VT_EMPTY)
            VariantClear(&val->u.var);
        return;
    case EXPRVAL_IDREF:
        if(val->u.idref.disp)
            IDispatch_Release(val->u.idref.disp);
        return;
    case EXPRVAL_NAMEREF:
        if(val->u.nameref.disp)
            IDispatch_Release(val->u.nameref.disp);
        SysFreeString(val->u.nameref.name);
    }
}

/* ECMA-262 3rd Edition    8.7.1 */
static HRESULT exprval_value(script_ctx_t *ctx, exprval_t *val, jsexcept_t *ei, VARIANT *ret)
{
    V_VT(ret) = VT_EMPTY;

    switch(val->type) {
    case EXPRVAL_VARIANT:
        return VariantCopy(ret, &val->u.var);
    case EXPRVAL_IDREF:
        if(!val->u.idref.disp) {
            FIXME("throw ReferenceError\n");
            return E_FAIL;
        }

        return disp_propget(val->u.idref.disp, val->u.idref.id, ctx->lcid, ret, ei, NULL/*FIXME*/);
    default:
        ERR("type %d\n", val->type);
        return E_FAIL;
    }
}

static HRESULT exprval_to_value(script_ctx_t *ctx, exprval_t *val, jsexcept_t *ei, VARIANT *ret)
{
    if(val->type == EXPRVAL_VARIANT) {
        *ret = val->u.var;
        V_VT(&val->u.var) = VT_EMPTY;
        return S_OK;
    }

    return exprval_value(ctx, val, ei, ret);
}

static HRESULT exprval_to_boolean(script_ctx_t *ctx, exprval_t *exprval, jsexcept_t *ei, VARIANT_BOOL *b)
{
    if(exprval->type != EXPRVAL_VARIANT) {
        VARIANT val;
        HRESULT hres;

        hres = exprval_to_value(ctx, exprval, ei, &val);
        if(FAILED(hres))
            return hres;

        hres = to_boolean(&val, b);
        VariantClear(&val);
        return hres;
    }

    return to_boolean(&exprval->u.var, b);
}

static void exprval_init(exprval_t *val)
{
    val->type = EXPRVAL_VARIANT;
    V_VT(&val->u.var) = VT_EMPTY;
}

static void exprval_set_idref(exprval_t *val, IDispatch *disp, DISPID id)
{
    val->type = EXPRVAL_IDREF;
    val->u.idref.disp = disp;
    val->u.idref.id = id;

    if(disp)
        IDispatch_AddRef(disp);
}

HRESULT scope_push(scope_chain_t *scope, DispatchEx *obj, scope_chain_t **ret)
{
    scope_chain_t *new_scope;

    new_scope = heap_alloc(sizeof(scope_chain_t));
    if(!new_scope)
        return E_OUTOFMEMORY;

    new_scope->ref = 1;

    IDispatchEx_AddRef(_IDispatchEx_(obj));
    new_scope->obj = obj;

    if(scope) {
        scope_addref(scope);
        new_scope->next = scope;
    }else {
        new_scope->next = NULL;
    }

    *ret = new_scope;
    return S_OK;
}

static void scope_pop(scope_chain_t **scope)
{
    scope_chain_t *tmp;

    tmp = *scope;
    *scope = tmp->next;
    scope_release(tmp);
}

void scope_release(scope_chain_t *scope)
{
    if(--scope->ref)
        return;

    if(scope->next)
        scope_release(scope->next);

    IDispatchEx_Release(_IDispatchEx_(scope->obj));
    heap_free(scope);
}

HRESULT create_exec_ctx(IDispatch *this_obj, DispatchEx *var_disp, scope_chain_t *scope, exec_ctx_t **ret)
{
    exec_ctx_t *ctx;

    ctx = heap_alloc_zero(sizeof(exec_ctx_t));
    if(!ctx)
        return E_OUTOFMEMORY;

    IDispatch_AddRef(this_obj);
    ctx->this_obj = this_obj;

    IDispatchEx_AddRef(_IDispatchEx_(var_disp));
    ctx->var_disp = var_disp;

    if(scope) {
        scope_addref(scope);
        ctx->scope_chain = scope;
    }

    *ret = ctx;
    return S_OK;
}

void exec_release(exec_ctx_t *ctx)
{
    if(--ctx->ref)
        return;

    if(ctx->scope_chain)
        scope_release(ctx->scope_chain);
    if(ctx->var_disp)
        IDispatchEx_Release(_IDispatchEx_(ctx->var_disp));
    if(ctx->this_obj)
        IDispatch_Release(ctx->this_obj);
    heap_free(ctx);
}

static HRESULT disp_get_id(IDispatch *disp, BSTR name, DWORD flags, DISPID *id)
{
    IDispatchEx *dispex;
    HRESULT hres;

    hres = IDispatch_QueryInterface(disp, &IID_IDispatchEx, (void**)&dispex);
    if(FAILED(hres)) {
        TRACE("unsing IDispatch\n");

        *id = 0;
        return IDispatch_GetIDsOfNames(disp, &IID_NULL, &name, 1, 0, id);
    }

    *id = 0;
    hres = IDispatchEx_GetDispID(dispex, name, flags|fdexNameCaseSensitive, id);
    IDispatchEx_Release(dispex);
    return hres;
}

/* ECMA-262 3rd Edition    8.7.2 */
static HRESULT put_value(script_ctx_t *ctx, exprval_t *ref, VARIANT *v, jsexcept_t *ei)
{
    if(ref->type != EXPRVAL_IDREF) {
        FIXME("throw ReferemceError\n");
        return E_FAIL;
    }

    return disp_propput(ref->u.idref.disp, ref->u.idref.id, ctx->lcid, v, ei, NULL/*FIXME*/);
}

static HRESULT disp_cmp(IDispatch *disp1, IDispatch *disp2, BOOL *ret)
{
    IObjectIdentity *identity;
    IUnknown *unk1, *unk2;
    HRESULT hres;

    if(disp1 == disp2) {
        *ret = TRUE;
        return S_OK;
    }

    hres = IDispatch_QueryInterface(disp1, &IID_IUnknown, (void**)&unk1);
    if(FAILED(hres))
        return hres;

    hres = IDispatch_QueryInterface(disp2, &IID_IUnknown, (void**)&unk2);
    if(FAILED(hres)) {
        IUnknown_Release(unk1);
        return hres;
    }

    if(unk1 == unk2) {
        *ret = TRUE;
    }else {
        hres = IUnknown_QueryInterface(unk1, &IID_IObjectIdentity, (void**)&identity);
        if(SUCCEEDED(hres)) {
            hres = IObjectIdentity_IsEqualObject(identity, unk2);
            IObjectIdentity_Release(identity);
            *ret = hres == S_OK;
        }else {
            *ret = FALSE;
        }
    }

    IUnknown_Release(unk1);
    IUnknown_Release(unk2);
    return S_OK;
}

/* ECMA-262 3rd Edition    11.9.6 */
static HRESULT equal2_values(VARIANT *lval, VARIANT *rval, BOOL *ret)
{
    TRACE("\n");

    if(V_VT(lval) != V_VT(rval)) {
        if(is_num_vt(V_VT(lval)) && is_num_vt(V_VT(rval))) {
            *ret = num_val(lval) == num_val(rval);
            return S_OK;
        }

        *ret = FALSE;
        return S_OK;
    }

    switch(V_VT(lval)) {
    case VT_EMPTY:
    case VT_NULL:
        *ret = VARIANT_TRUE;
        break;
    case VT_I4:
        *ret = V_I4(lval) == V_I4(rval);
        break;
    case VT_R8:
        *ret = V_R8(lval) == V_R8(rval);
        break;
    case VT_BSTR:
        *ret = !strcmpW(V_BSTR(lval), V_BSTR(rval));
        break;
    case VT_DISPATCH:
        return disp_cmp(V_DISPATCH(lval), V_DISPATCH(rval), ret);
    case VT_BOOL:
        *ret = !V_BOOL(lval) == !V_BOOL(rval);
        break;
    default:
        FIXME("unimplemented vt %d\n", V_VT(lval));
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT literal_to_var(literal_t *literal, VARIANT *v)
{
    V_VT(v) = literal->vt;

    switch(V_VT(v)) {
    case VT_EMPTY:
    case VT_NULL:
        break;
    case VT_I4:
        V_I4(v) = literal->u.lval;
        break;
    case VT_R8:
        V_R8(v) = literal->u.dval;
        break;
    case VT_BSTR:
        V_BSTR(v) = SysAllocString(literal->u.wstr);
        break;
    case VT_BOOL:
        V_BOOL(v) = literal->u.bval;
        break;
    case VT_DISPATCH:
        IDispatch_AddRef(literal->u.disp);
        V_DISPATCH(v) = literal->u.disp;
        break;
    default:
        ERR("wrong type %d\n", V_VT(v));
        return E_NOTIMPL;
    }

    return S_OK;
}

HRESULT exec_source(exec_ctx_t *ctx, parser_ctx_t *parser, source_elements_t *source, jsexcept_t *ei, VARIANT *retv)
{
    script_ctx_t *script = parser->script;
    function_declaration_t *func;
    parser_ctx_t *prev_parser;
    var_list_t *var;
    VARIANT val, tmp;
    statement_t *stat;
    exec_ctx_t *prev_ctx;
    return_type_t rt;
    HRESULT hres = S_OK;

    for(func = source->functions; func; func = func->next) {
        DispatchEx *func_obj;
        VARIANT var;

        hres = create_source_function(parser, func->expr->parameter_list, func->expr->source_elements,
                ctx->scope_chain, func->expr->src_str, func->expr->src_len, &func_obj);
        if(FAILED(hres))
            return hres;

        V_VT(&var) = VT_DISPATCH;
        V_DISPATCH(&var) = (IDispatch*)_IDispatchEx_(func_obj);
        hres = jsdisp_propput_name(ctx->var_disp, func->expr->identifier, script->lcid, &var, ei, NULL);
        jsdisp_release(func_obj);
        if(FAILED(hres))
            return hres;
    }

    for(var = source->variables; var; var = var->next) {
        DISPID id = 0;

        hres = jsdisp_get_id(ctx->var_disp, var->identifier, fdexNameEnsure, &id);
        if(FAILED(hres))
            return hres;
    }

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

/* ECMA-262 3rd Edition    10.1.4 */
static HRESULT identifier_eval(exec_ctx_t *ctx, BSTR identifier, DWORD flags, exprval_t *ret)
{
    scope_chain_t *scope;
    named_item_t *item;
    DISPID id = 0;
    HRESULT hres;

    TRACE("%s\n", debugstr_w(identifier));

    for(scope = ctx->scope_chain; scope; scope = scope->next) {
        hres = jsdisp_get_id(scope->obj, identifier, 0, &id);
        if(SUCCEEDED(hres))
            break;
    }

    if(scope) {
        exprval_set_idref(ret, (IDispatch*)_IDispatchEx_(scope->obj), id);
        return S_OK;
    }

    hres = jsdisp_get_id(ctx->parser->script->global, identifier, 0, &id);
    if(SUCCEEDED(hres)) {
        exprval_set_idref(ret, (IDispatch*)_IDispatchEx_(ctx->parser->script->global), id);
        return S_OK;
    }

    for(item = ctx->parser->script->named_items; item; item = item->next) {
        if((item->flags & SCRIPTITEM_ISVISIBLE) && !strcmpW(item->name, identifier)) {
            if(!item->disp) {
                IUnknown *unk;

                if(!ctx->parser->script->site)
                    break;

                hres = IActiveScriptSite_GetItemInfo(ctx->parser->script->site, identifier,
                                                     SCRIPTINFO_IUNKNOWN, &unk, NULL);
                if(FAILED(hres)) {
                    WARN("GetItemInfo failed: %08x\n", hres);
                    break;
                }

                hres = IUnknown_QueryInterface(unk, &IID_IDispatch, (void**)&item->disp);
                IUnknown_Release(unk);
                if(FAILED(hres)) {
                    WARN("object does not implement IDispatch\n");
                    break;
                }
            }

            ret->type = EXPRVAL_VARIANT;
            V_VT(&ret->u.var) = VT_DISPATCH;
            V_DISPATCH(&ret->u.var) = item->disp;
            IDispatch_AddRef(item->disp);
            return S_OK;
        }
    }

    for(item = ctx->parser->script->named_items; item; item = item->next) {
        if(item->flags & SCRIPTITEM_GLOBALMEMBERS) {
            hres = disp_get_id(item->disp, identifier, 0, &id);
            if(SUCCEEDED(hres))
                break;
        }
    }

    if(item) {
        exprval_set_idref(ret, item->disp, id);
        return S_OK;
    }

    hres = jsdisp_get_id(ctx->parser->script->script_disp, identifier, 0, &id);
    if(SUCCEEDED(hres)) {
        exprval_set_idref(ret, (IDispatch*)_IDispatchEx_(ctx->parser->script->script_disp), id);
        return S_OK;
    }

    if(flags & EXPR_NEWREF) {
        hres = jsdisp_get_id(ctx->var_disp, identifier, fdexNameEnsure, &id);
        if(FAILED(hres))
            return hres;

        exprval_set_idref(ret, (IDispatch*)_IDispatchEx_(ctx->var_disp), id);
        return S_OK;
    }

    WARN("Could not find identifier %s\n", debugstr_w(identifier));
    return E_FAIL;
}

/* ECMA-262 3rd Edition    12.1 */
HRESULT block_statement_eval(exec_ctx_t *ctx, statement_t *_stat, return_type_t *rt, VARIANT *ret)
{
    block_statement_t *stat = (block_statement_t*)_stat;
    VARIANT val, tmp;
    statement_t *iter;
    HRESULT hres = S_OK;

    TRACE("\n");

    V_VT(&val) = VT_EMPTY;
    for(iter = stat->stat_list; iter; iter = iter->next) {
        hres = stat_eval(ctx, iter, rt, &tmp);
        if(FAILED(hres))
            break;

        VariantClear(&val);
        val = tmp;
        if(rt->type != RT_NORMAL)
            break;
    }

    if(FAILED(hres)) {
        VariantClear(&val);
        return hres;
    }

    *ret = val;
    return S_OK;
}

/* ECMA-262 3rd Edition    12.2 */
static HRESULT variable_list_eval(exec_ctx_t *ctx, variable_declaration_t *var_list, jsexcept_t *ei)
{
    variable_declaration_t *iter;
    HRESULT hres = S_OK;

    for(iter = var_list; iter; iter = iter->next) {
        exprval_t exprval;
        VARIANT val;

        if(!iter->expr)
            continue;

        hres = expr_eval(ctx, iter->expr, 0, ei, &exprval);
        if(FAILED(hres))
            break;

        hres = exprval_to_value(ctx->parser->script, &exprval, ei, &val);
        exprval_release(&exprval);
        if(FAILED(hres))
            break;

        hres = jsdisp_propput_name(ctx->var_disp, iter->identifier, ctx->parser->script->lcid, &val, ei, NULL/*FIXME*/);
        VariantClear(&val);
        if(FAILED(hres))
            break;
    }

    return hres;
}

/* ECMA-262 3rd Edition    12.2 */
HRESULT var_statement_eval(exec_ctx_t *ctx, statement_t *_stat, return_type_t *rt, VARIANT *ret)
{
    var_statement_t *stat = (var_statement_t*)_stat;
    HRESULT hres;

    TRACE("\n");

    hres = variable_list_eval(ctx, stat->variable_list, &rt->ei);
    if(FAILED(hres))
        return hres;

    V_VT(ret) = VT_EMPTY;
    return S_OK;
}

/* ECMA-262 3rd Edition    12.3 */
HRESULT empty_statement_eval(exec_ctx_t *ctx, statement_t *stat, return_type_t *rt, VARIANT *ret)
{
    TRACE("\n");

    V_VT(ret) = VT_EMPTY;
    return S_OK;
}

/* ECMA-262 3rd Edition    12.4 */
HRESULT expression_statement_eval(exec_ctx_t *ctx, statement_t *_stat, return_type_t *rt, VARIANT *ret)
{
    expression_statement_t *stat = (expression_statement_t*)_stat;
    exprval_t exprval;
    VARIANT val;
    HRESULT hres;

    TRACE("\n");

    hres = expr_eval(ctx, stat->expr, EXPR_NOVAL, &rt->ei, &exprval);
    if(FAILED(hres))
        return hres;

    hres = exprval_to_value(ctx->parser->script, &exprval, &rt->ei, &val);
    exprval_release(&exprval);
    if(FAILED(hres))
        return hres;

    *ret = val;
    TRACE("= %s\n", debugstr_variant(ret));
    return S_OK;
}

/* ECMA-262 3rd Edition    12.5 */
HRESULT if_statement_eval(exec_ctx_t *ctx, statement_t *_stat, return_type_t *rt, VARIANT *ret)
{
    if_statement_t *stat = (if_statement_t*)_stat;
    exprval_t exprval;
    VARIANT_BOOL b;
    HRESULT hres;

    TRACE("\n");

    hres = expr_eval(ctx, stat->expr, 0, &rt->ei, &exprval);
    if(FAILED(hres))
        return hres;

    hres = exprval_to_boolean(ctx->parser->script, &exprval, &rt->ei, &b);
    exprval_release(&exprval);
    if(FAILED(hres))
        return hres;

    if(b)
        hres = stat_eval(ctx, stat->if_stat, rt, ret);
    else if(stat->else_stat)
        hres = stat_eval(ctx, stat->else_stat, rt, ret);
    else
        V_VT(ret) = VT_EMPTY;

    return hres;
}

/* ECMA-262 3rd Edition    12.6.2 */
HRESULT while_statement_eval(exec_ctx_t *ctx, statement_t *_stat, return_type_t *rt, VARIANT *ret)
{
    while_statement_t *stat = (while_statement_t*)_stat;
    exprval_t exprval;
    VARIANT val, tmp;
    VARIANT_BOOL b;
    BOOL test_expr;
    HRESULT hres;

    TRACE("\n");

    V_VT(&val) = VT_EMPTY;
    test_expr = !stat->do_while;

    while(1) {
        if(test_expr) {
            hres = expr_eval(ctx, stat->expr, 0, &rt->ei, &exprval);
            if(FAILED(hres))
                break;

            hres = exprval_to_boolean(ctx->parser->script, &exprval, &rt->ei, &b);
            exprval_release(&exprval);
            if(FAILED(hres) || !b)
                break;
        }else {
            test_expr = TRUE;
        }

        hres = stat_eval(ctx, stat->statement, rt, &tmp);
        if(FAILED(hres))
            break;

        VariantClear(&val);
        val = tmp;

        if(rt->type == RT_CONTINUE)
            rt->type = RT_NORMAL;
        if(rt->type != RT_NORMAL)
            break;
    }

    if(FAILED(hres)) {
        VariantClear(&val);
        return hres;
    }

    if(rt->type == RT_BREAK)
        rt->type = RT_NORMAL;

    *ret = val;
    return S_OK;
}

/* ECMA-262 3rd Edition    12.6.3 */
HRESULT for_statement_eval(exec_ctx_t *ctx, statement_t *_stat, return_type_t *rt, VARIANT *ret)
{
    for_statement_t *stat = (for_statement_t*)_stat;
    VARIANT val, tmp, retv;
    exprval_t exprval;
    VARIANT_BOOL b;
    HRESULT hres;

    TRACE("\n");

    if(stat->variable_list) {
        hres = variable_list_eval(ctx, stat->variable_list, &rt->ei);
        if(FAILED(hres))
            return hres;
    }else if(stat->begin_expr) {
        hres = expr_eval(ctx, stat->begin_expr, EXPR_NEWREF, &rt->ei, &exprval);
        if(FAILED(hres))
            return hres;

        hres = exprval_to_value(ctx->parser->script, &exprval, &rt->ei, &val);
        exprval_release(&exprval);
        if(FAILED(hres))
            return hres;

        VariantClear(&val);
    }

    V_VT(&retv) = VT_EMPTY;

    while(1) {
        if(stat->expr) {
            hres = expr_eval(ctx, stat->expr, 0, &rt->ei, &exprval);
            if(FAILED(hres))
                break;

            hres = exprval_to_boolean(ctx->parser->script, &exprval, &rt->ei, &b);
            exprval_release(&exprval);
            if(FAILED(hres) || !b)
                break;
        }

        hres = stat_eval(ctx, stat->statement, rt, &tmp);
        if(FAILED(hres))
            break;

        VariantClear(&retv);
        retv = tmp;

        if(rt->type == RT_CONTINUE)
            rt->type = RT_NORMAL;
        else if(rt->type != RT_NORMAL)
            break;

        if(stat->end_expr) {
            hres = expr_eval(ctx, stat->end_expr, 0, &rt->ei, &exprval);
            if(FAILED(hres))
                break;

            hres = exprval_to_value(ctx->parser->script, &exprval, &rt->ei, &val);
            exprval_release(&exprval);
            if(FAILED(hres))
                break;

            VariantClear(&val);
        }
    }

    if(FAILED(hres)) {
        VariantClear(&retv);
        return hres;
    }

    if(rt->type == RT_BREAK)
        rt->type = RT_NORMAL;

    *ret = retv;
    return S_OK;
}

/* ECMA-262 3rd Edition    12.6.4 */
HRESULT forin_statement_eval(exec_ctx_t *ctx, statement_t *_stat, return_type_t *rt, VARIANT *ret)
{
    forin_statement_t *stat = (forin_statement_t*)_stat;
    VARIANT val, name, retv, tmp;
    DISPID id = DISPID_STARTENUM;
    BSTR str, identifier = NULL;
    IDispatchEx *in_obj;
    exprval_t exprval;
    HRESULT hres;

    TRACE("\n");

    if(stat->variable) {
        hres = variable_list_eval(ctx, stat->variable, &rt->ei);
        if(FAILED(hres))
            return hres;
    }

    hres = expr_eval(ctx, stat->in_expr, EXPR_NEWREF, &rt->ei, &exprval);
    if(FAILED(hres))
        return hres;

    hres = exprval_to_value(ctx->parser->script, &exprval, &rt->ei, &val);
    exprval_release(&exprval);
    if(FAILED(hres))
        return hres;

    if(V_VT(&val) != VT_DISPATCH) {
        TRACE("in vt %d\n", V_VT(&val));
        VariantClear(&val);
        V_VT(ret) = VT_EMPTY;
        return S_OK;
    }

    hres = IDispatch_QueryInterface(V_DISPATCH(&val), &IID_IDispatchEx, (void**)&in_obj);
    IDispatch_Release(V_DISPATCH(&val));
    if(FAILED(hres)) {
        FIXME("Object doesn't support IDispatchEx\n");
        return E_NOTIMPL;
    }

    V_VT(&retv) = VT_EMPTY;

    if(stat->variable)
        identifier = SysAllocString(stat->variable->identifier);

    while(1) {
        hres = IDispatchEx_GetNextDispID(in_obj, fdexEnumDefault, id, &id);
        if(FAILED(hres) || hres == S_FALSE)
            break;

        hres = IDispatchEx_GetMemberName(in_obj, id, &str);
        if(FAILED(hres))
            break;

        TRACE("iter %s\n", debugstr_w(str));

        if(stat->variable)
            hres = identifier_eval(ctx, identifier, 0, &exprval);
        else
            hres = expr_eval(ctx, stat->expr, EXPR_NEWREF, &rt->ei, &exprval);
        if(SUCCEEDED(hres)) {
            V_VT(&name) = VT_BSTR;
            V_BSTR(&name) = str;
            hres = put_value(ctx->parser->script, &exprval, &name, &rt->ei);
            exprval_release(&exprval);
        }
        SysFreeString(str);
        if(FAILED(hres))
            break;

        hres = stat_eval(ctx, stat->statement, rt, &tmp);
        if(FAILED(hres))
            break;

        VariantClear(&retv);
        retv = tmp;

        if(rt->type == RT_CONTINUE)
            rt->type = RT_NORMAL;
        else if(rt->type != RT_NORMAL)
            break;
    }

    SysFreeString(identifier);
    IDispatchEx_Release(in_obj);
    if(FAILED(hres)) {
        VariantClear(&retv);
        return hres;
    }

    if(rt->type == RT_BREAK)
        rt->type = RT_NORMAL;

    *ret = retv;
    return S_OK;
}

/* ECMA-262 3rd Edition    12.7 */
HRESULT continue_statement_eval(exec_ctx_t *ctx, statement_t *_stat, return_type_t *rt, VARIANT *ret)
{
    branch_statement_t *stat = (branch_statement_t*)_stat;

    TRACE("\n");

    if(stat->identifier) {
        FIXME("indentifier not implemented\n");
        return E_NOTIMPL;
    }

    rt->type = RT_CONTINUE;
    V_VT(ret) = VT_EMPTY;
    return S_OK;
}

/* ECMA-262 3rd Edition    12.8 */
HRESULT break_statement_eval(exec_ctx_t *ctx, statement_t *_stat, return_type_t *rt, VARIANT *ret)
{
    branch_statement_t *stat = (branch_statement_t*)_stat;

    TRACE("\n");

    if(stat->identifier) {
        FIXME("indentifier not implemented\n");
        return E_NOTIMPL;
    }

    rt->type = RT_BREAK;
    V_VT(ret) = VT_EMPTY;
    return S_OK;
}

/* ECMA-262 3rd Edition    12.9 */
HRESULT return_statement_eval(exec_ctx_t *ctx, statement_t *_stat, return_type_t *rt, VARIANT *ret)
{
    expression_statement_t *stat = (expression_statement_t*)_stat;
    HRESULT hres;

    TRACE("\n");

    if(stat->expr) {
        exprval_t exprval;

        hres = expr_eval(ctx, stat->expr, 0, &rt->ei, &exprval);
        if(FAILED(hres))
            return hres;

        hres = exprval_to_value(ctx->parser->script, &exprval, &rt->ei, ret);
        exprval_release(&exprval);
        if(FAILED(hres))
            return hres;
    }else {
        V_VT(ret) = VT_EMPTY;
    }

    TRACE("= %s\n", debugstr_variant(ret));
    rt->type = RT_RETURN;
    return S_OK;
}

/* ECMA-262 3rd Edition    12.10 */
HRESULT with_statement_eval(exec_ctx_t *ctx, statement_t *_stat, return_type_t *rt, VARIANT *ret)
{
    with_statement_t *stat = (with_statement_t*)_stat;
    exprval_t exprval;
    IDispatch *disp;
    DispatchEx *obj;
    VARIANT val;
    HRESULT hres;

    TRACE("\n");

    hres = expr_eval(ctx, stat->expr, 0, &rt->ei, &exprval);
    if(FAILED(hres))
        return hres;

    hres = exprval_to_value(ctx->parser->script, &exprval, &rt->ei, &val);
    exprval_release(&exprval);
    if(FAILED(hres))
        return hres;

    hres = to_object(ctx, &val, &disp);
    VariantClear(&val);
    if(FAILED(hres))
        return hres;

    obj = iface_to_jsdisp((IUnknown*)disp);
    IDispatch_Release(disp);
    if(!obj) {
        FIXME("disp id not jsdisp\n");
        return E_NOTIMPL;
    }

    hres = scope_push(ctx->scope_chain, obj, &ctx->scope_chain);
    jsdisp_release(obj);
    if(FAILED(hres))
        return hres;

    hres = stat_eval(ctx, stat->statement, rt, ret);

    scope_pop(&ctx->scope_chain);
    return hres;
}

/* ECMA-262 3rd Edition    12.12 */
HRESULT labelled_statement_eval(exec_ctx_t *ctx, statement_t *stat, return_type_t *rt, VARIANT *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

/* ECMA-262 3rd Edition    12.13 */
HRESULT switch_statement_eval(exec_ctx_t *ctx, statement_t *_stat, return_type_t *rt, VARIANT *ret)
{
    switch_statement_t *stat = (switch_statement_t*)_stat;
    case_clausule_t *iter, *default_clausule = NULL;
    statement_t *stat_iter;
    VARIANT val, cval;
    exprval_t exprval;
    BOOL b;
    HRESULT hres;

    TRACE("\n");

    hres = expr_eval(ctx, stat->expr, 0, &rt->ei, &exprval);
    if(FAILED(hres))
        return hres;

    hres = exprval_to_value(ctx->parser->script, &exprval, &rt->ei, &val);
    exprval_release(&exprval);
    if(FAILED(hres))
        return hres;

    for(iter = stat->case_list; iter; iter = iter->next) {
        if(!iter->expr) {
            default_clausule = iter;
            continue;
        }

        hres = expr_eval(ctx, iter->expr, 0, &rt->ei, &exprval);
        if(FAILED(hres))
            break;

        hres = exprval_to_value(ctx->parser->script, &exprval, &rt->ei, &cval);
        exprval_release(&exprval);
        if(FAILED(hres))
            break;

        hres = equal2_values(&val, &cval, &b);
        VariantClear(&cval);
        if(FAILED(hres) || b)
            break;
    }

    VariantClear(&val);
    if(FAILED(hres))
        return hres;

    if(!iter)
        iter = default_clausule;

    V_VT(&val) = VT_EMPTY;
    if(iter) {
        VARIANT tmp;

        for(stat_iter = iter->stat; stat_iter; stat_iter = stat_iter->next) {
            hres = stat_eval(ctx, stat_iter, rt, &tmp);
            if(FAILED(hres))
                break;

            VariantClear(&val);
            val = tmp;

            if(rt->type != RT_NORMAL)
                break;
        }
    }

    if(FAILED(hres)) {
        VariantClear(&val);
        return hres;
    }

    if(rt->type == RT_BREAK)
        rt->type = RT_NORMAL;

    *ret = val;
    return S_OK;
}

/* ECMA-262 3rd Edition    12.13 */
HRESULT throw_statement_eval(exec_ctx_t *ctx, statement_t *_stat, return_type_t *rt, VARIANT *ret)
{
    expression_statement_t *stat = (expression_statement_t*)_stat;
    exprval_t exprval;
    VARIANT val;
    HRESULT hres;

    TRACE("\n");

    hres = expr_eval(ctx, stat->expr, 0, &rt->ei, &exprval);
    if(FAILED(hres))
        return hres;

    hres = exprval_to_value(ctx->parser->script, &exprval, &rt->ei, &val);
    exprval_release(&exprval);
    if(FAILED(hres))
        return hres;

    rt->ei.var = val;
    return DISP_E_EXCEPTION;
}

/* ECMA-262 3rd Edition    12.14 */
static HRESULT catch_eval(exec_ctx_t *ctx, catch_block_t *block, return_type_t *rt, VARIANT *ret)
{
    DispatchEx *var_disp;
    VARIANT ex, val;
    HRESULT hres;

    ex = rt->ei.var;
    memset(&rt->ei, 0, sizeof(jsexcept_t));

    hres = create_dispex(ctx->parser->script, NULL, NULL, &var_disp);
    if(SUCCEEDED(hres)) {
        hres = jsdisp_propput_name(var_disp, block->identifier, ctx->parser->script->lcid,
                &ex, &rt->ei, NULL/*FIXME*/);
        if(SUCCEEDED(hres)) {
            hres = scope_push(ctx->scope_chain, var_disp, &ctx->scope_chain);
            if(SUCCEEDED(hres)) {
                hres = stat_eval(ctx, block->statement, rt, &val);
                scope_pop(&ctx->scope_chain);
            }
        }

        jsdisp_release(var_disp);
    }

    VariantClear(&ex);
    if(FAILED(hres))
        return hres;

    *ret = val;
    return S_OK;
}

/* ECMA-262 3rd Edition    12.14 */
HRESULT try_statement_eval(exec_ctx_t *ctx, statement_t *_stat, return_type_t *rt, VARIANT *ret)
{
    try_statement_t *stat = (try_statement_t*)_stat;
    VARIANT val;
    HRESULT hres;

    TRACE("\n");

    hres = stat_eval(ctx, stat->try_statement, rt, &val);
    if(FAILED(hres)) {
        TRACE("EXCEPTION\n");
        if(!stat->catch_block)
            return hres;

        hres = catch_eval(ctx, stat->catch_block, rt, &val);
        if(FAILED(hres))
            return hres;
    }

    if(stat->finally_statement) {
        VariantClear(&val);
        hres = stat_eval(ctx, stat->finally_statement, rt, &val);
        if(FAILED(hres))
            return hres;
    }

    *ret = val;
    return S_OK;
}

static HRESULT return_bool(exprval_t *ret, DWORD b)
{
    ret->type = EXPRVAL_VARIANT;
    V_VT(&ret->u.var) = VT_BOOL;
    V_BOOL(&ret->u.var) = b ? VARIANT_TRUE : VARIANT_FALSE;

    return S_OK;
}

static HRESULT get_binary_expr_values(exec_ctx_t *ctx, binary_expression_t *expr, jsexcept_t *ei, VARIANT *lval, VARIANT *rval)
{
    exprval_t exprval;
    HRESULT hres;

    hres = expr_eval(ctx, expr->expression1, 0, ei, &exprval);
    if(FAILED(hres))
        return hres;

    hres = exprval_to_value(ctx->parser->script, &exprval, ei, lval);
    exprval_release(&exprval);
    if(FAILED(hres))
        return hres;

    hres = expr_eval(ctx, expr->expression2, 0, ei, &exprval);
    if(SUCCEEDED(hres)) {
        hres = exprval_to_value(ctx->parser->script, &exprval, ei, rval);
        exprval_release(&exprval);
    }

    if(FAILED(hres)) {
        VariantClear(lval);
        return hres;
    }

    return S_OK;
}

typedef HRESULT (*oper_t)(exec_ctx_t*,VARIANT*,VARIANT*,jsexcept_t*,VARIANT*);

static HRESULT binary_expr_eval(exec_ctx_t *ctx, binary_expression_t *expr, oper_t oper, jsexcept_t *ei,
        exprval_t *ret)
{
    VARIANT lval, rval, retv;
    HRESULT hres;

    hres = get_binary_expr_values(ctx, expr, ei, &lval, &rval);
    if(FAILED(hres))
        return hres;

    hres = oper(ctx, &lval, &rval, ei, &retv);
    VariantClear(&lval);
    VariantClear(&rval);
    if(FAILED(hres))
        return hres;

    ret->type = EXPRVAL_VARIANT;
    ret->u.var = retv;
    return S_OK;
}

/* ECMA-262 3rd Edition    11.13.2 */
static HRESULT assign_oper_eval(exec_ctx_t *ctx, expression_t *lexpr, expression_t *rexpr, oper_t oper,
                                jsexcept_t *ei, exprval_t *ret)
{
    VARIANT retv, lval, rval;
    exprval_t exprval, exprvalr;
    HRESULT hres;

    hres = expr_eval(ctx, lexpr, EXPR_NEWREF, ei, &exprval);
    if(FAILED(hres))
        return hres;

    hres = exprval_value(ctx->parser->script, &exprval, ei, &lval);
    if(SUCCEEDED(hres)) {
        hres = expr_eval(ctx, rexpr, 0, ei, &exprvalr);
        if(SUCCEEDED(hres)) {
            hres = exprval_value(ctx->parser->script, &exprvalr, ei, &rval);
            exprval_release(&exprvalr);
        }
        if(SUCCEEDED(hres)) {
            hres = oper(ctx, &lval, &rval, ei, &retv);
            VariantClear(&rval);
        }
        VariantClear(&lval);
    }

    if(SUCCEEDED(hres)) {
        hres = put_value(ctx->parser->script, &exprval, &retv, ei);
        if(FAILED(hres))
            VariantClear(&retv);
    }
    exprval_release(&exprval);

    if(FAILED(hres))
        return hres;

    ret->type = EXPRVAL_VARIANT;
    ret->u.var = retv;
    return S_OK;
}

/* ECMA-262 3rd Edition    13 */
HRESULT function_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    function_expression_t *expr = (function_expression_t*)_expr;
    VARIANT var;
    HRESULT hres;

    TRACE("\n");

    if(expr->identifier) {
        hres = jsdisp_propget_name(ctx->var_disp, expr->identifier, ctx->parser->script->lcid, &var, ei, NULL/*FIXME*/);
        if(FAILED(hres))
            return hres;
    }else {
        DispatchEx *dispex;

        hres = create_source_function(ctx->parser, expr->parameter_list, expr->source_elements, ctx->scope_chain,
                expr->src_str, expr->src_len, &dispex);
        if(FAILED(hres))
            return hres;

        V_VT(&var) = VT_DISPATCH;
        V_DISPATCH(&var) = (IDispatch*)_IDispatchEx_(dispex);
    }

    ret->type = EXPRVAL_VARIANT;
    ret->u.var = var;
    return S_OK;
}

/* ECMA-262 3rd Edition    11.12 */
HRESULT conditional_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    conditional_expression_t *expr = (conditional_expression_t*)_expr;
    exprval_t exprval;
    VARIANT_BOOL b;
    HRESULT hres;

    TRACE("\n");

    hres = expr_eval(ctx, expr->expression, 0, ei, &exprval);
    if(FAILED(hres))
        return hres;

    hres = exprval_to_boolean(ctx->parser->script, &exprval, ei, &b);
    exprval_release(&exprval);
    if(FAILED(hres))
        return hres;

    return expr_eval(ctx, b ? expr->true_expression : expr->false_expression, flags, ei, ret);
}

/* ECMA-262 3rd Edition    11.2.1 */
HRESULT array_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    array_expression_t *expr = (array_expression_t*)_expr;
    exprval_t exprval;
    VARIANT member, val;
    DISPID id;
    BSTR str;
    IDispatch *obj = NULL;
    HRESULT hres;

    TRACE("\n");

    hres = expr_eval(ctx, expr->member_expr, EXPR_NEWREF, ei, &exprval);
    if(FAILED(hres))
        return hres;

    hres = exprval_to_value(ctx->parser->script, &exprval, ei, &member);
    exprval_release(&exprval);
    if(FAILED(hres))
        return hres;

    hres = expr_eval(ctx, expr->expression, EXPR_NEWREF, ei, &exprval);
    if(SUCCEEDED(hres)) {
        hres = exprval_to_value(ctx->parser->script, &exprval, ei, &val);
        exprval_release(&exprval);
    }

    if(SUCCEEDED(hres))
        hres = to_object(ctx, &member, &obj);
    VariantClear(&member);
    if(SUCCEEDED(hres)) {
        hres = to_string(ctx->parser->script, &val, ei, &str);
        if(SUCCEEDED(hres)) {
            if(flags & EXPR_STRREF) {
                ret->type = EXPRVAL_NAMEREF;
                ret->u.nameref.disp = obj;
                ret->u.nameref.name = str;
                return S_OK;
            }

            hres = disp_get_id(obj, str, flags & EXPR_NEWREF ? fdexNameEnsure : 0, &id);
        }

        if(SUCCEEDED(hres)) {
            exprval_set_idref(ret, obj, id);
        }else if(!(flags & EXPR_NEWREF) && hres == DISP_E_UNKNOWNNAME) {
            exprval_init(ret);
            hres = S_OK;
        }

        IDispatch_Release(obj);
    }

    return hres;
}

/* ECMA-262 3rd Edition    11.2.1 */
HRESULT member_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    member_expression_t *expr = (member_expression_t*)_expr;
    IDispatch *obj = NULL;
    exprval_t exprval;
    VARIANT member;
    DISPID id;
    BSTR str;
    HRESULT hres;

    TRACE("\n");

    hres = expr_eval(ctx, expr->expression, 0, ei, &exprval);
    if(FAILED(hres))
        return hres;

    hres = exprval_to_value(ctx->parser->script, &exprval, ei, &member);
    exprval_release(&exprval);
    if(FAILED(hres))
        return hres;

    hres = to_object(ctx, &member, &obj);
    VariantClear(&member);
    if(FAILED(hres))
        return hres;

    str = SysAllocString(expr->identifier);
    if(flags & EXPR_STRREF) {
        ret->type = EXPRVAL_NAMEREF;
        ret->u.nameref.disp = obj;
        ret->u.nameref.name = str;
        return S_OK;
    }

    hres = disp_get_id(obj, str, flags & EXPR_NEWREF ? fdexNameEnsure : 0, &id);
    SysFreeString(str);
    if(SUCCEEDED(hres)) {
        exprval_set_idref(ret, obj, id);
    }else if(!(flags & EXPR_NEWREF) && hres == DISP_E_UNKNOWNNAME) {
        exprval_init(ret);
        hres = S_OK;
    }

    IDispatch_Release(obj);
    return hres;
}

static void free_dp(DISPPARAMS *dp)
{
    DWORD i;

    for(i=0; i < dp->cArgs; i++)
        VariantClear(dp->rgvarg+i);
    heap_free(dp->rgvarg);
}

static HRESULT args_to_param(exec_ctx_t *ctx, argument_t *args, jsexcept_t *ei, DISPPARAMS *dp)
{
    VARIANTARG *vargs;
    exprval_t exprval;
    argument_t *iter;
    DWORD cnt = 0, i;
    HRESULT hres = S_OK;

    memset(dp, 0, sizeof(*dp));
    if(!args)
        return S_OK;

    for(iter = args; iter; iter = iter->next)
        cnt++;

    vargs = heap_alloc_zero(cnt * sizeof(*vargs));
    if(!vargs)
        return E_OUTOFMEMORY;

    for(i = cnt, iter = args; iter; iter = iter->next) {
        hres = expr_eval(ctx, iter->expr, 0, ei, &exprval);
        if(FAILED(hres))
            break;

        hres = exprval_to_value(ctx->parser->script, &exprval, ei, vargs + (--i));
        exprval_release(&exprval);
        if(FAILED(hres))
            break;
    }

    if(FAILED(hres)) {
        free_dp(dp);
        return hres;
    }

    dp->rgvarg = vargs;
    dp->cArgs = cnt;
    return S_OK;
}

/* ECMA-262 3rd Edition    11.2.2 */
HRESULT new_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    call_expression_t *expr = (call_expression_t*)_expr;
    exprval_t exprval;
    VARIANT constr, var;
    DISPPARAMS dp;
    HRESULT hres;

    TRACE("\n");

    hres = expr_eval(ctx, expr->expression, 0, ei, &exprval);
    if(FAILED(hres))
        return hres;

    hres = args_to_param(ctx, expr->argument_list, ei, &dp);
    if(SUCCEEDED(hres))
        hres = exprval_to_value(ctx->parser->script, &exprval, ei, &constr);
    exprval_release(&exprval);
    if(FAILED(hres))
        return hres;

    if(V_VT(&constr) != VT_DISPATCH) {
        FIXME("throw TypeError\n");
        VariantClear(&constr);
        return E_FAIL;
    }

    hres = disp_call(V_DISPATCH(&constr), DISPID_VALUE, ctx->parser->script->lcid,
                     DISPATCH_CONSTRUCT, &dp, &var, ei, NULL/*FIXME*/);
    IDispatch_Release(V_DISPATCH(&constr));
    if(FAILED(hres))
        return hres;

    ret->type = EXPRVAL_VARIANT;
    ret->u.var = var;
    return S_OK;
}

/* ECMA-262 3rd Edition    11.2.3 */
HRESULT call_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    call_expression_t *expr = (call_expression_t*)_expr;
    VARIANT var;
    exprval_t exprval;
    DISPPARAMS dp;
    HRESULT hres;

    TRACE("\n");

    hres = expr_eval(ctx, expr->expression, 0, ei, &exprval);
    if(FAILED(hres))
        return hres;

    hres = args_to_param(ctx, expr->argument_list, ei, &dp);
    if(SUCCEEDED(hres)) {
        switch(exprval.type) {
        case EXPRVAL_IDREF:
            hres = disp_call(exprval.u.idref.disp, exprval.u.idref.id, ctx->parser->script->lcid, DISPATCH_METHOD,
                    &dp, flags & EXPR_NOVAL ? NULL : &var, ei, NULL/*FIXME*/);
            if(flags & EXPR_NOVAL)
                V_VT(&var) = VT_EMPTY;
            break;
        default:
            FIXME("unimplemented type %d\n", exprval.type);
            hres = E_NOTIMPL;
        }

        free_dp(&dp);
    }

    exprval_release(&exprval);
    if(FAILED(hres))
        return hres;

    TRACE("= %s\n", debugstr_variant(&var));
    ret->type = EXPRVAL_VARIANT;
    ret->u.var = var;
    return S_OK;
}

/* ECMA-262 3rd Edition    11.1.1 */
HRESULT this_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    TRACE("\n");

    ret->type = EXPRVAL_VARIANT;
    V_VT(&ret->u.var) = VT_DISPATCH;
    V_DISPATCH(&ret->u.var) = ctx->this_obj;
    IDispatch_AddRef(ctx->this_obj);
    return S_OK;
}

/* ECMA-262 3rd Edition    10.1.4 */
HRESULT identifier_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    identifier_expression_t *expr = (identifier_expression_t*)_expr;
    BSTR identifier;
    HRESULT hres;

    TRACE("\n");

    identifier = SysAllocString(expr->identifier);
    if(!identifier)
        return E_OUTOFMEMORY;

    hres = identifier_eval(ctx, identifier, flags, ret);

    SysFreeString(identifier);
    return hres;
}

/* ECMA-262 3rd Edition    7.8 */
HRESULT literal_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    literal_expression_t *expr = (literal_expression_t*)_expr;
    VARIANT var;
    HRESULT hres;

    TRACE("\n");

    hres = literal_to_var(expr->literal, &var);
    if(FAILED(hres))
        return hres;

    ret->type = EXPRVAL_VARIANT;
    ret->u.var = var;
    return S_OK;
}

/* ECMA-262 3rd Edition    11.1.4 */
HRESULT array_literal_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    array_literal_expression_t *expr = (array_literal_expression_t*)_expr;
    DWORD length = 0, i = 0;
    array_element_t *elem;
    DispatchEx *array;
    exprval_t exprval;
    VARIANT val;
    HRESULT hres;

    TRACE("\n");

    for(elem = expr->element_list; elem; elem = elem->next)
        length += elem->elision+1;
    length += expr->length;

    hres = create_array(ctx->parser->script, length, &array);
    if(FAILED(hres))
        return hres;

    for(elem = expr->element_list; elem; elem = elem->next) {
        i += elem->elision;

        hres = expr_eval(ctx, elem->expr, 0, ei, &exprval);
        if(FAILED(hres))
            break;

        hres = exprval_to_value(ctx->parser->script, &exprval, ei, &val);
        exprval_release(&exprval);
        if(FAILED(hres))
            break;

        hres = jsdisp_propput_idx(array, i, ctx->parser->script->lcid, &val, ei, NULL/*FIXME*/);
        VariantClear(&val);
        if(FAILED(hres))
            break;

        i++;
    }

    if(FAILED(hres)) {
        jsdisp_release(array);
        return hres;
    }

    ret->type = EXPRVAL_VARIANT;
    V_VT(&ret->u.var) = VT_DISPATCH;
    V_DISPATCH(&ret->u.var) = (IDispatch*)_IDispatchEx_(array);
    return S_OK;
}

/* ECMA-262 3rd Edition    11.1.5 */
HRESULT property_value_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    property_value_expression_t *expr = (property_value_expression_t*)_expr;
    VARIANT val, tmp;
    DispatchEx *obj;
    prop_val_t *iter;
    exprval_t exprval;
    BSTR name;
    HRESULT hres;

    TRACE("\n");

    hres = create_object(ctx->parser->script, NULL, &obj);
    if(FAILED(hres))
        return hres;

    for(iter = expr->property_list; iter; iter = iter->next) {
        hres = literal_to_var(iter->name, &tmp);
        if(FAILED(hres))
            break;

        hres = to_string(ctx->parser->script, &tmp, ei, &name);
        VariantClear(&tmp);
        if(FAILED(hres))
            break;

        hres = expr_eval(ctx, iter->value, 0, ei, &exprval);
        if(SUCCEEDED(hres)) {
            hres = exprval_to_value(ctx->parser->script, &exprval, ei, &val);
            exprval_release(&exprval);
            if(SUCCEEDED(hres)) {
                hres = jsdisp_propput_name(obj, name, ctx->parser->script->lcid, &val, ei, NULL/*FIXME*/);
                VariantClear(&val);
            }
        }

        SysFreeString(name);
        if(FAILED(hres))
            break;
    }

    if(FAILED(hres)) {
        jsdisp_release(obj);
        return hres;
    }

    ret->type = EXPRVAL_VARIANT;
    V_VT(&ret->u.var) = VT_DISPATCH;
    V_DISPATCH(&ret->u.var) = (IDispatch*)_IDispatchEx_(obj);
    return S_OK;
}

/* ECMA-262 3rd Edition    11.14 */
HRESULT comma_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    binary_expression_t *expr = (binary_expression_t*)_expr;
    VARIANT lval, rval;
    HRESULT hres;

    TRACE("\n");

    hres = get_binary_expr_values(ctx, expr, ei, &lval, &rval);
    if(FAILED(hres))
        return hres;

    VariantClear(&lval);

    ret->type = EXPRVAL_VARIANT;
    ret->u.var = rval;
    return S_OK;
}

/* ECMA-262 3rd Edition    11.11 */
HRESULT logical_or_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    binary_expression_t *expr = (binary_expression_t*)_expr;
    exprval_t exprval;
    VARIANT_BOOL b;
    VARIANT val;
    HRESULT hres;

    TRACE("\n");

    hres = expr_eval(ctx, expr->expression1, 0, ei, &exprval);
    if(FAILED(hres))
        return hres;

    hres = exprval_to_value(ctx->parser->script, &exprval, ei, &val);
    exprval_release(&exprval);
    if(FAILED(hres))
        return hres;

    hres = to_boolean(&val, &b);
    if(SUCCEEDED(hres) && b) {
        ret->type = EXPRVAL_VARIANT;
        ret->u.var = val;
        return S_OK;
    }

    VariantClear(&val);
    if(FAILED(hres))
        return hres;

    hres = expr_eval(ctx, expr->expression2, 0, ei, &exprval);
    if(FAILED(hres))
        return hres;

    hres = exprval_to_value(ctx->parser->script, &exprval, ei, &val);
    exprval_release(&exprval);
    if(FAILED(hres))
        return hres;

    ret->type = EXPRVAL_VARIANT;
    ret->u.var = val;
    return S_OK;
}

/* ECMA-262 3rd Edition    11.11 */
HRESULT logical_and_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    binary_expression_t *expr = (binary_expression_t*)_expr;
    exprval_t exprval;
    VARIANT_BOOL b;
    VARIANT val;
    HRESULT hres;

    TRACE("\n");

    hres = expr_eval(ctx, expr->expression1, 0, ei, &exprval);
    if(FAILED(hres))
        return hres;

    hres = exprval_to_value(ctx->parser->script, &exprval, ei, &val);
    exprval_release(&exprval);
    if(FAILED(hres))
        return hres;

    hres = to_boolean(&val, &b);
    if(SUCCEEDED(hres) && !b) {
        ret->type = EXPRVAL_VARIANT;
        ret->u.var = val;
        return S_OK;
    }

    VariantClear(&val);
    if(FAILED(hres))
        return hres;

    hres = expr_eval(ctx, expr->expression2, 0, ei, &exprval);
    if(FAILED(hres))
        return hres;

    hres = exprval_to_value(ctx->parser->script, &exprval, ei, &val);
    exprval_release(&exprval);
    if(FAILED(hres))
        return hres;

    ret->type = EXPRVAL_VARIANT;
    ret->u.var = val;
    return S_OK;
}

/* ECMA-262 3rd Edition    11.10 */
static HRESULT bitor_eval(exec_ctx_t *ctx, VARIANT *lval, VARIANT *rval, jsexcept_t *ei, VARIANT *retv)
{
    INT li, ri;
    HRESULT hres;

    hres = to_int32(ctx->parser->script, lval, ei, &li);
    if(FAILED(hres))
        return hres;

    hres = to_int32(ctx->parser->script, rval, ei, &ri);
    if(FAILED(hres))
        return hres;

    V_VT(retv) = VT_I4;
    V_I4(retv) = li|ri;
    return S_OK;
}

/* ECMA-262 3rd Edition    11.10 */
HRESULT binary_or_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    binary_expression_t *expr = (binary_expression_t*)_expr;

    TRACE("\n");

    return binary_expr_eval(ctx, expr, bitor_eval, ei, ret);
}

/* ECMA-262 3rd Edition    11.10 */
static HRESULT xor_eval(exec_ctx_t *ctx, VARIANT *lval, VARIANT *rval, jsexcept_t *ei, VARIANT *retv)
{
    INT li, ri;
    HRESULT hres;

    hres = to_int32(ctx->parser->script, lval, ei, &li);
    if(FAILED(hres))
        return hres;

    hres = to_int32(ctx->parser->script, rval, ei, &ri);
    if(FAILED(hres))
        return hres;

    V_VT(retv) = VT_I4;
    V_I4(retv) = li^ri;
    return S_OK;
}

/* ECMA-262 3rd Edition    11.10 */
HRESULT binary_xor_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    binary_expression_t *expr = (binary_expression_t*)_expr;

    TRACE("\n");

    return binary_expr_eval(ctx, expr, xor_eval, ei, ret);
}

/* ECMA-262 3rd Edition    11.10 */
static HRESULT bitand_eval(exec_ctx_t *ctx, VARIANT *lval, VARIANT *rval, jsexcept_t *ei, VARIANT *retv)
{
    INT li, ri;
    HRESULT hres;

    hres = to_int32(ctx->parser->script, lval, ei, &li);
    if(FAILED(hres))
        return hres;

    hres = to_int32(ctx->parser->script, rval, ei, &ri);
    if(FAILED(hres))
        return hres;

    V_VT(retv) = VT_I4;
    V_I4(retv) = li&ri;
    return S_OK;
}

/* ECMA-262 3rd Edition    11.10 */
HRESULT binary_and_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    binary_expression_t *expr = (binary_expression_t*)_expr;

    TRACE("\n");

    return binary_expr_eval(ctx, expr, bitand_eval, ei, ret);
}

/* ECMA-262 3rd Edition    11.8.6 */
HRESULT instanceof_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

/* ECMA-262 3rd Edition    11.8.7 */
HRESULT in_expression_eval(exec_ctx_t *ctx, expression_t *expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    FIXME("\n");
    return E_NOTIMPL;
}

/* ECMA-262 3rd Edition    11.6.1 */
static HRESULT add_eval(exec_ctx_t *ctx, VARIANT *lval, VARIANT *rval, jsexcept_t *ei, VARIANT *retv)
{
    VARIANT r, l;
    HRESULT hres;

    hres = to_primitive(ctx->parser->script, lval, ei, &l);
    if(FAILED(hres))
        return hres;

    hres = to_primitive(ctx->parser->script, rval, ei, &r);
    if(FAILED(hres)) {
        VariantClear(&l);
        return hres;
    }

    if(V_VT(&l) == VT_BSTR || V_VT(&r) == VT_BSTR) {
        BSTR lstr = NULL, rstr = NULL;

        if(V_VT(&l) == VT_BSTR)
            lstr = V_BSTR(&l);
        else
            hres = to_string(ctx->parser->script, &l, ei, &lstr);

        if(SUCCEEDED(hres)) {
            if(V_VT(&r) == VT_BSTR)
                rstr = V_BSTR(&r);
            else
                hres = to_string(ctx->parser->script, &r, ei, &rstr);
        }

        if(SUCCEEDED(hres)) {
            int len1, len2;

            len1 = SysStringLen(lstr);
            len2 = SysStringLen(rstr);

            V_VT(retv) = VT_BSTR;
            V_BSTR(retv) = SysAllocStringLen(NULL, len1+len2);
            memcpy(V_BSTR(retv), lstr, len1*sizeof(WCHAR));
            memcpy(V_BSTR(retv)+len1, rstr, (len2+1)*sizeof(WCHAR));
        }

        if(V_VT(&l) != VT_BSTR)
            SysFreeString(lstr);
        if(V_VT(&r) != VT_BSTR)
            SysFreeString(rstr);
    }else {
        VARIANT nl, nr;

        hres = to_number(ctx->parser->script, &l, ei, &nl);
        if(SUCCEEDED(hres)) {
            hres = to_number(ctx->parser->script, &r, ei, &nr);
            if(SUCCEEDED(hres))
                num_set_val(retv, num_val(&nl) + num_val(&nr));
        }
    }

    VariantClear(&r);
    VariantClear(&l);
    return hres;
}

/* ECMA-262 3rd Edition    11.6.1 */
HRESULT add_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    binary_expression_t *expr = (binary_expression_t*)_expr;

    TRACE("\n");

    return binary_expr_eval(ctx, expr, add_eval, ei, ret);
}

/* ECMA-262 3rd Edition    11.6.2 */
static HRESULT sub_eval(exec_ctx_t *ctx, VARIANT *lval, VARIANT *rval, jsexcept_t *ei, VARIANT *retv)
{
    VARIANT lnum, rnum;
    HRESULT hres;

    hres = to_number(ctx->parser->script, lval, ei, &lnum);
    if(FAILED(hres))
        return hres;

    hres = to_number(ctx->parser->script, rval, ei, &rnum);
    if(FAILED(hres))
        return hres;

    num_set_val(retv, num_val(&lnum) - num_val(&rnum));
    return S_OK;
}

/* ECMA-262 3rd Edition    11.6.2 */
HRESULT sub_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    binary_expression_t *expr = (binary_expression_t*)_expr;

    TRACE("\n");

    return binary_expr_eval(ctx, expr, sub_eval, ei, ret);
}

/* ECMA-262 3rd Edition    11.5.1 */
static HRESULT mul_eval(exec_ctx_t *ctx, VARIANT *lval, VARIANT *rval, jsexcept_t *ei, VARIANT *retv)
{
    VARIANT lnum, rnum;
    HRESULT hres;

    hres = to_number(ctx->parser->script, lval, ei, &lnum);
    if(FAILED(hres))
        return hres;

    hres = to_number(ctx->parser->script, rval, ei, &rnum);
    if(FAILED(hres))
        return hres;

    num_set_val(retv, num_val(&lnum) * num_val(&rnum));
    return S_OK;
}

/* ECMA-262 3rd Edition    11.5.1 */
HRESULT mul_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    binary_expression_t *expr = (binary_expression_t*)_expr;

    TRACE("\n");

    return binary_expr_eval(ctx, expr, mul_eval, ei, ret);
}

/* ECMA-262 3rd Edition    11.5.2 */
static HRESULT div_eval(exec_ctx_t *ctx, VARIANT *lval, VARIANT *rval, jsexcept_t *ei, VARIANT *retv)
{
    VARIANT lnum, rnum;
    HRESULT hres;

    hres = to_number(ctx->parser->script, lval, ei, &lnum);
    if(FAILED(hres))
        return hres;

    hres = to_number(ctx->parser->script, rval, ei, &rnum);
    if(FAILED(hres))
        return hres;

    num_set_val(retv, num_val(&lnum) / num_val(&rnum));
    return S_OK;
}

/* ECMA-262 3rd Edition    11.5.2 */
HRESULT div_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    binary_expression_t *expr = (binary_expression_t*)_expr;

    TRACE("\n");

    return binary_expr_eval(ctx, expr, div_eval, ei, ret);
}

/* ECMA-262 3rd Edition    11.5.3 */
static HRESULT mod_eval(exec_ctx_t *ctx, VARIANT *lval, VARIANT *rval, jsexcept_t *ei, VARIANT *retv)
{
    VARIANT lnum, rnum;
    HRESULT hres;

    hres = to_number(ctx->parser->script, lval, ei, &lnum);
    if(FAILED(hres))
        return hres;

    hres = to_number(ctx->parser->script, rval, ei, &rnum);
    if(FAILED(hres))
        return hres;

    num_set_val(retv, fmod(num_val(&lnum), num_val(&rnum)));
    return S_OK;
}

/* ECMA-262 3rd Edition    11.5.3 */
HRESULT mod_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    binary_expression_t *expr = (binary_expression_t*)_expr;

    TRACE("\n");

    return binary_expr_eval(ctx, expr, mod_eval, ei, ret);
}

/* ECMA-262 3rd Edition    11.4.2 */
HRESULT delete_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    unary_expression_t *expr = (unary_expression_t*)_expr;
    VARIANT_BOOL b = VARIANT_FALSE;
    exprval_t exprval;
    HRESULT hres;

    TRACE("\n");

    hres = expr_eval(ctx, expr->expression, EXPR_STRREF, ei, &exprval);
    if(FAILED(hres))
        return hres;

    switch(exprval.type) {
    case EXPRVAL_IDREF: {
        IDispatchEx *dispex;

        hres = IDispatch_QueryInterface(exprval.u.nameref.disp, &IID_IDispatchEx, (void**)&dispex);
        if(SUCCEEDED(hres)) {
            hres = IDispatchEx_DeleteMemberByDispID(dispex, exprval.u.idref.id);
            b = VARIANT_TRUE;
            IDispatchEx_Release(dispex);
        }
        break;
    }
    case EXPRVAL_NAMEREF: {
        IDispatchEx *dispex;

        hres = IDispatch_QueryInterface(exprval.u.nameref.disp, &IID_IDispatchEx, (void**)&dispex);
        if(SUCCEEDED(hres)) {
            hres = IDispatchEx_DeleteMemberByName(dispex, exprval.u.nameref.name, fdexNameCaseSensitive);
            b = VARIANT_TRUE;
            IDispatchEx_Release(dispex);
        }
        break;
    }
    default:
        FIXME("unsupported type %d\n", exprval.type);
        hres = E_NOTIMPL;
    }

    exprval_release(&exprval);
    if(FAILED(hres))
        return hres;

    return return_bool(ret, b);
}

/* ECMA-262 3rd Edition    11.4.2 */
HRESULT void_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    unary_expression_t *expr = (unary_expression_t*)_expr;
    exprval_t exprval;
    VARIANT tmp;
    HRESULT hres;

    TRACE("\n");

    hres = expr_eval(ctx, expr->expression, 0, ei, &exprval);
    if(FAILED(hres))
        return hres;

    hres = exprval_to_value(ctx->parser->script, &exprval, ei, &tmp);
    exprval_release(&exprval);
    if(FAILED(hres))
        return hres;

    VariantClear(&tmp);

    ret->type = EXPRVAL_VARIANT;
    V_VT(&ret->u.var) = VT_EMPTY;
    return S_OK;
}

/* ECMA-262 3rd Edition    11.4.3 */
HRESULT typeof_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    unary_expression_t *expr = (unary_expression_t*)_expr;
    const WCHAR *str;
    exprval_t exprval;
    VARIANT val;
    HRESULT hres;

    static const WCHAR booleanW[] = {'b','o','o','l','e','a','n',0};
    static const WCHAR functionW[] = {'f','u','n','c','t','i','o','n',0};
    static const WCHAR numberW[] = {'n','u','m','b','e','r',0};
    static const WCHAR objectW[] = {'o','b','j','e','c','t',0};
    static const WCHAR stringW[] = {'s','t','r','i','n','g',0};
    static const WCHAR undefinedW[] = {'u','n','d','e','f','i','n','e','d',0};

    TRACE("\n");

    hres = expr_eval(ctx, expr->expression, 0, ei, &exprval);
    if(FAILED(hres))
        return hres;

    hres = exprval_to_value(ctx->parser->script, &exprval, ei, &val);
    exprval_release(&exprval);
    if(FAILED(hres))
        return hres;

    switch(V_VT(&val)) {
    case VT_EMPTY:
        str = undefinedW;
        break;
    case VT_NULL:
        str = objectW;
        break;
    case VT_BOOL:
        str = booleanW;
        break;
    case VT_I4:
    case VT_R8:
        str = numberW;
        break;
    case VT_BSTR:
        str = stringW;
        break;
    case VT_DISPATCH: {
        DispatchEx *dispex;

        dispex = iface_to_jsdisp((IUnknown*)V_DISPATCH(&val));
        if(dispex) {
            str = dispex->builtin_info->class == JSCLASS_FUNCTION ? functionW : objectW;
            IDispatchEx_Release(_IDispatchEx_(dispex));
        }else {
            str = objectW;
        }
        break;
    }
    default:
        FIXME("unhandled vt %d\n", V_VT(&val));
        VariantClear(&val);
        return E_NOTIMPL;
    }

    VariantClear(&val);

    ret->type = EXPRVAL_VARIANT;
    V_VT(&ret->u.var) = VT_BSTR;
    V_BSTR(&ret->u.var) = SysAllocString(str);
    return S_OK;
}

/* ECMA-262 3rd Edition    11.4.7 */
HRESULT minus_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    unary_expression_t *expr = (unary_expression_t*)_expr;
    exprval_t exprval;
    VARIANT val, num;
    HRESULT hres;

    TRACE("\n");

    hres = expr_eval(ctx, expr->expression, 0, ei, &exprval);
    if(FAILED(hres))
        return hres;

    hres = exprval_to_value(ctx->parser->script, &exprval, ei, &val);
    exprval_release(&exprval);
    if(FAILED(hres))
        return hres;

    hres = to_number(ctx->parser->script, &val, ei, &num);
    VariantClear(&val);
    if(FAILED(hres))
        return hres;

    ret->type = EXPRVAL_VARIANT;
    num_set_val(&ret->u.var, -num_val(&num));
    return S_OK;
}

/* ECMA-262 3rd Edition    11.4.6 */
HRESULT plus_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    unary_expression_t *expr = (unary_expression_t*)_expr;
    exprval_t exprval;
    VARIANT val, num;
    HRESULT hres;

    TRACE("\n");

    hres = expr_eval(ctx, expr->expression, EXPR_NEWREF, ei, &exprval);
    if(FAILED(hres))
        return hres;

    hres = exprval_to_value(ctx->parser->script, &exprval, ei, &val);
    exprval_release(&exprval);
    if(FAILED(hres))
        return hres;

    hres = to_number(ctx->parser->script, &val, ei, &num);
    if(FAILED(hres))
        return hres;

    ret->type = EXPRVAL_VARIANT;
    ret->u.var = num;
    return S_OK;
}

/* ECMA-262 3rd Edition    11.3.1 */
HRESULT post_increment_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    unary_expression_t *expr = (unary_expression_t*)_expr;
    VARIANT val, num;
    exprval_t exprval;
    HRESULT hres;

    TRACE("\n");

    hres = expr_eval(ctx, expr->expression, EXPR_NEWREF, ei, &exprval);
    if(FAILED(hres))
        return hres;

    hres = exprval_value(ctx->parser->script, &exprval, ei, &val);
    if(SUCCEEDED(hres)) {
        hres = to_number(ctx->parser->script, &val, ei, &num);
        VariantClear(&val);
    }

    if(SUCCEEDED(hres)) {
        VARIANT inc;
        num_set_val(&inc, num_val(&num)+1.0);
        hres = put_value(ctx->parser->script, &exprval, &inc, ei);
    }

    exprval_release(&exprval);
    if(FAILED(hres))
        return hres;

    ret->type = EXPRVAL_VARIANT;
    ret->u.var = num;
    return S_OK;
}

/* ECMA-262 3rd Edition    11.3.2 */
HRESULT post_decrement_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    unary_expression_t *expr = (unary_expression_t*)_expr;
    VARIANT val, num;
    exprval_t exprval;
    HRESULT hres;

    TRACE("\n");

    hres = expr_eval(ctx, expr->expression, EXPR_NEWREF, ei, &exprval);
    if(FAILED(hres))
        return hres;

    hres = exprval_value(ctx->parser->script, &exprval, ei, &val);
    if(SUCCEEDED(hres)) {
        hres = to_number(ctx->parser->script, &val, ei, &num);
        VariantClear(&val);
    }

    if(SUCCEEDED(hres)) {
        VARIANT dec;
        num_set_val(&dec, num_val(&num)-1.0);
        hres = put_value(ctx->parser->script, &exprval, &dec, ei);
    }

    exprval_release(&exprval);
    if(FAILED(hres))
        return hres;

    ret->type = EXPRVAL_VARIANT;
    ret->u.var = num;
    return S_OK;
}

/* ECMA-262 3rd Edition    11.4.4 */
HRESULT pre_increment_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    unary_expression_t *expr = (unary_expression_t*)_expr;
    VARIANT val, num;
    exprval_t exprval;
    HRESULT hres;

    TRACE("\n");

    hres = expr_eval(ctx, expr->expression, EXPR_NEWREF, ei, &exprval);
    if(FAILED(hres))
        return hres;

    hres = exprval_value(ctx->parser->script, &exprval, ei, &val);
    if(SUCCEEDED(hres)) {
        hres = to_number(ctx->parser->script, &val, ei, &num);
        VariantClear(&val);
    }

    if(SUCCEEDED(hres)) {
        num_set_val(&val, num_val(&num)+1.0);
        hres = put_value(ctx->parser->script, &exprval, &val, ei);
    }

    exprval_release(&exprval);
    if(FAILED(hres))
        return hres;

    ret->type = EXPRVAL_VARIANT;
    ret->u.var = val;
    return S_OK;
}

/* ECMA-262 3rd Edition    11.4.5 */
HRESULT pre_decrement_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    unary_expression_t *expr = (unary_expression_t*)_expr;
    VARIANT val, num;
    exprval_t exprval;
    HRESULT hres;

    TRACE("\n");

    hres = expr_eval(ctx, expr->expression, EXPR_NEWREF, ei, &exprval);
    if(FAILED(hres))
        return hres;

    hres = exprval_value(ctx->parser->script, &exprval, ei, &val);
    if(SUCCEEDED(hres)) {
        hres = to_number(ctx->parser->script, &val, ei, &num);
        VariantClear(&val);
    }

    if(SUCCEEDED(hres)) {
        num_set_val(&val, num_val(&num)-1.0);
        hres = put_value(ctx->parser->script, &exprval, &val, ei);
    }

    exprval_release(&exprval);
    if(FAILED(hres))
        return hres;

    ret->type = EXPRVAL_VARIANT;
    ret->u.var = val;
    return S_OK;
}

/* ECMA-262 3rd Edition    11.9.3 */
static HRESULT equal_values(exec_ctx_t *ctx, VARIANT *lval, VARIANT *rval, jsexcept_t *ei, BOOL *ret)
{
    if(V_VT(lval) == V_VT(rval) || (is_num_vt(V_VT(lval)) && is_num_vt(V_VT(rval))))
       return equal2_values(lval, rval, ret);

    /* FIXME: NULL disps should be handled in more general way */
    if(V_VT(lval) == VT_DISPATCH && !V_DISPATCH(lval)) {
        VARIANT v;
        V_VT(&v) = VT_NULL;
        return equal_values(ctx, &v, rval, ei, ret);
    }

    if(V_VT(rval) == VT_DISPATCH && !V_DISPATCH(rval)) {
        VARIANT v;
        V_VT(&v) = VT_NULL;
        return equal_values(ctx, lval, &v, ei, ret);
    }

    if((V_VT(lval) == VT_NULL && V_VT(rval) == VT_EMPTY) ||
       (V_VT(lval) == VT_EMPTY && V_VT(rval) == VT_NULL)) {
        *ret = TRUE;
        return S_OK;
    }

    if(V_VT(lval) == VT_BSTR && is_num_vt(V_VT(rval))) {
        VARIANT v;
        HRESULT hres;

        hres = to_number(ctx->parser->script, lval, ei, &v);
        if(FAILED(hres))
            return hres;

        return equal_values(ctx, &v, rval, ei, ret);
    }

    if(V_VT(rval) == VT_BSTR && is_num_vt(V_VT(lval))) {
        VARIANT v;
        HRESULT hres;

        hres = to_number(ctx->parser->script, rval, ei, &v);
        if(FAILED(hres))
            return hres;

        return equal_values(ctx, lval, &v, ei, ret);
    }

    if(V_VT(rval) == VT_BOOL) {
        VARIANT v;

        V_VT(&v) = VT_I4;
        V_I4(&v) = V_BOOL(rval) ? 1 : 0;
        return equal_values(ctx, lval, &v, ei, ret);
    }

    if(V_VT(lval) == VT_BOOL) {
        VARIANT v;

        V_VT(&v) = VT_I4;
        V_I4(&v) = V_BOOL(lval) ? 1 : 0;
        return equal_values(ctx, &v, rval, ei, ret);
    }


    if(V_VT(rval) == VT_DISPATCH && (V_VT(lval) == VT_BSTR || is_num_vt(V_VT(lval)))) {
        VARIANT v;
        HRESULT hres;

        hres = to_primitive(ctx->parser->script, rval, ei, &v);
        if(FAILED(hres))
            return hres;

        hres = equal_values(ctx, lval, &v, ei, ret);

        VariantClear(&v);
        return hres;
    }


    if(V_VT(lval) == VT_DISPATCH && (V_VT(rval) == VT_BSTR || is_num_vt(V_VT(rval)))) {
        VARIANT v;
        HRESULT hres;

        hres = to_primitive(ctx->parser->script, lval, ei, &v);
        if(FAILED(hres))
            return hres;

        hres = equal_values(ctx, &v, rval, ei, ret);

        VariantClear(&v);
        return hres;
    }


    *ret = FALSE;
    return S_OK;
}

/* ECMA-262 3rd Edition    11.9.1 */
HRESULT equal_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    binary_expression_t *expr = (binary_expression_t*)_expr;
    VARIANT rval, lval;
    BOOL b;
    HRESULT hres;

    TRACE("\n");

    hres = get_binary_expr_values(ctx, expr, ei, &rval, &lval);
    if(FAILED(hres))
        return hres;

    hres = equal_values(ctx, &rval, &lval, ei, &b);
    if(FAILED(hres))
        return hres;

    return return_bool(ret, b);
}

/* ECMA-262 3rd Edition    11.9.4 */
HRESULT equal2_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    binary_expression_t *expr = (binary_expression_t*)_expr;
    VARIANT rval, lval;
    BOOL b;
    HRESULT hres;

    TRACE("\n");

    hres = get_binary_expr_values(ctx, expr, ei, &rval, &lval);
    if(FAILED(hres))
        return hres;

    hres = equal2_values(&rval, &lval, &b);
    if(FAILED(hres))
        return hres;

    return return_bool(ret, b);
}

/* ECMA-262 3rd Edition    11.9.2 */
HRESULT not_equal_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    binary_expression_t *expr = (binary_expression_t*)_expr;
    VARIANT rval, lval;
    BOOL b;
    HRESULT hres;

    TRACE("\n");

    hres = get_binary_expr_values(ctx, expr, ei, &lval, &rval);
    if(FAILED(hres))
        return hres;

    hres = equal_values(ctx, &lval, &rval, ei, &b);
    if(FAILED(hres))
        return hres;

    return return_bool(ret, !b);
}

/* ECMA-262 3rd Edition    11.9.5 */
HRESULT not_equal2_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    binary_expression_t *expr = (binary_expression_t*)_expr;
    VARIANT rval, lval;
    BOOL b;
    HRESULT hres;

    TRACE("\n");

    hres = get_binary_expr_values(ctx, expr, ei, &lval, &rval);
    if(FAILED(hres))
        return hres;

    hres = equal2_values(&lval, &rval, &b);
    if(FAILED(hres))
        return hres;

    return return_bool(ret, !b);
}

/* ECMA-262 3rd Edition    11.8.5 */
static HRESULT less_eval(exec_ctx_t *ctx, VARIANT *lval, VARIANT *rval, BOOL greater, jsexcept_t *ei, BOOL *ret)
{
    VARIANT l, r, ln, rn;
    HRESULT hres;

    hres = to_primitive(ctx->parser->script, lval, ei, &l);
    if(FAILED(hres))
        return hres;

    hres = to_primitive(ctx->parser->script, rval, ei, &r);
    if(FAILED(hres)) {
        VariantClear(&l);
        return hres;
    }

    if(V_VT(&l) == VT_BSTR && V_VT(&r) == VT_BSTR) {
        *ret = (strcmpW(V_BSTR(&l), V_BSTR(&r)) < 0) ^ greater;
        SysFreeString(V_BSTR(&l));
        SysFreeString(V_BSTR(&r));
        return S_OK;
    }

    hres = to_number(ctx->parser->script, &l, ei, &ln);
    VariantClear(&l);
    if(SUCCEEDED(hres))
        hres = to_number(ctx->parser->script, &r, ei, &rn);
    VariantClear(&r);
    if(FAILED(hres))
        return hres;

    if(V_VT(&ln) == VT_I4 && V_VT(&rn) == VT_I4) {
        *ret = (V_I4(&ln) < V_I4(&rn)) ^ greater;
    }else  {
        DOUBLE ld = num_val(&ln);
        DOUBLE rd = num_val(&rn);

        *ret = !isnan(ld) && !isnan(rd) && ((ld < rd) ^ greater);
    }

    return S_OK;
}

/* ECMA-262 3rd Edition    11.8.1 */
HRESULT less_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    binary_expression_t *expr = (binary_expression_t*)_expr;
    VARIANT rval, lval;
    BOOL b;
    HRESULT hres;

    TRACE("\n");

    hres = get_binary_expr_values(ctx, expr, ei, &lval, &rval);
    if(FAILED(hres))
        return hres;

    hres = less_eval(ctx, &lval, &rval, FALSE, ei, &b);
    VariantClear(&lval);
    VariantClear(&rval);
    if(FAILED(hres))
        return hres;

    return return_bool(ret, b);
}

/* ECMA-262 3rd Edition    11.8.3 */
HRESULT lesseq_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    binary_expression_t *expr = (binary_expression_t*)_expr;
    VARIANT rval, lval;
    BOOL b;
    HRESULT hres;

    TRACE("\n");

    hres = get_binary_expr_values(ctx, expr, ei, &lval, &rval);
    if(FAILED(hres))
        return hres;

    hres = less_eval(ctx, &rval, &lval, TRUE, ei, &b);
    VariantClear(&lval);
    VariantClear(&rval);
    if(FAILED(hres))
        return hres;

    return return_bool(ret, b);
}

/* ECMA-262 3rd Edition    11.8.2 */
HRESULT greater_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    binary_expression_t *expr = (binary_expression_t*)_expr;
    VARIANT rval, lval;
    BOOL b;
    HRESULT hres;

    TRACE("\n");

    hres = get_binary_expr_values(ctx, expr, ei, &lval, &rval);
    if(FAILED(hres))
        return hres;

    hres = less_eval(ctx, &rval, &lval, FALSE, ei, &b);
    VariantClear(&lval);
    VariantClear(&rval);
    if(FAILED(hres))
        return hres;

    return return_bool(ret, b);
}

/* ECMA-262 3rd Edition    11.8.4 */
HRESULT greatereq_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    binary_expression_t *expr = (binary_expression_t*)_expr;
    VARIANT rval, lval;
    BOOL b;
    HRESULT hres;

    TRACE("\n");

    hres = get_binary_expr_values(ctx, expr, ei, &lval, &rval);
    if(FAILED(hres))
        return hres;

    hres = less_eval(ctx, &lval, &rval, TRUE, ei, &b);
    VariantClear(&lval);
    VariantClear(&rval);
    if(FAILED(hres))
        return hres;

    return return_bool(ret, b);
}

/* ECMA-262 3rd Edition    11.4.8 */
HRESULT binary_negation_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    unary_expression_t *expr = (unary_expression_t*)_expr;
    exprval_t exprval;
    VARIANT val;
    INT i;
    HRESULT hres;

    TRACE("\n");

    hres = expr_eval(ctx, expr->expression, EXPR_NEWREF, ei, &exprval);
    if(FAILED(hres))
        return hres;

    hres = exprval_to_value(ctx->parser->script, &exprval, ei, &val);
    exprval_release(&exprval);
    if(FAILED(hres))
        return hres;

    hres = to_int32(ctx->parser->script, &val, ei, &i);
    if(FAILED(hres))
        return hres;

    ret->type = EXPRVAL_VARIANT;
    V_VT(&ret->u.var) = VT_I4;
    V_I4(&ret->u.var) = ~i;
    return S_OK;
}

/* ECMA-262 3rd Edition    11.4.9 */
HRESULT logical_negation_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    unary_expression_t *expr = (unary_expression_t*)_expr;
    exprval_t exprval;
    VARIANT_BOOL b;
    HRESULT hres;

    TRACE("\n");

    hres = expr_eval(ctx, expr->expression, EXPR_NEWREF, ei, &exprval);
    if(FAILED(hres))
        return hres;

    hres = exprval_to_boolean(ctx->parser->script, &exprval, ei, &b);
    exprval_release(&exprval);
    if(FAILED(hres))
        return hres;

    return return_bool(ret, !b);
}

/* ECMA-262 3rd Edition    11.7.1 */
static HRESULT lshift_eval(exec_ctx_t *ctx, VARIANT *lval, VARIANT *rval, jsexcept_t *ei, VARIANT *retv)
{
    DWORD ri;
    INT li;
    HRESULT hres;

    hres = to_int32(ctx->parser->script, lval, ei, &li);
    if(FAILED(hres))
        return hres;

    hres = to_uint32(ctx->parser->script, rval, ei, &ri);
    if(FAILED(hres))
        return hres;

    V_VT(retv) = VT_I4;
    V_I4(retv) = li << (ri&0x1f);
    return S_OK;
}

/* ECMA-262 3rd Edition    11.7.1 */
HRESULT left_shift_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    binary_expression_t *expr = (binary_expression_t*)_expr;

    TRACE("\n");

    return binary_expr_eval(ctx, expr, lshift_eval, ei, ret);
}

/* ECMA-262 3rd Edition    11.7.2 */
static HRESULT rshift_eval(exec_ctx_t *ctx, VARIANT *lval, VARIANT *rval, jsexcept_t *ei, VARIANT *retv)
{
    DWORD ri;
    INT li;
    HRESULT hres;

    hres = to_int32(ctx->parser->script, lval, ei, &li);
    if(FAILED(hres))
        return hres;

    hres = to_uint32(ctx->parser->script, rval, ei, &ri);
    if(FAILED(hres))
        return hres;

    V_VT(retv) = VT_I4;
    V_I4(retv) = li >> (ri&0x1f);
    return S_OK;
}

/* ECMA-262 3rd Edition    11.7.2 */
HRESULT right_shift_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    binary_expression_t *expr = (binary_expression_t*)_expr;

    TRACE("\n");

    return binary_expr_eval(ctx, expr, rshift_eval, ei, ret);
}

/* ECMA-262 3rd Edition    11.7.3 */
static HRESULT rshift2_eval(exec_ctx_t *ctx, VARIANT *lval, VARIANT *rval, jsexcept_t *ei, VARIANT *retv)
{
    DWORD li, ri;
    HRESULT hres;

    hres = to_uint32(ctx->parser->script, lval, ei, &li);
    if(FAILED(hres))
        return hres;

    hres = to_uint32(ctx->parser->script, rval, ei, &ri);
    if(FAILED(hres))
        return hres;

    V_VT(retv) = VT_I4;
    V_I4(retv) = li >> (ri&0x1f);
    return S_OK;
}

/* ECMA-262 3rd Edition    11.7.3 */
HRESULT right2_shift_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    binary_expression_t *expr = (binary_expression_t*)_expr;

    TRACE("\n");

    return binary_expr_eval(ctx, expr, rshift2_eval, ei, ret);
}

/* ECMA-262 3rd Edition    11.13.1 */
HRESULT assign_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    binary_expression_t *expr = (binary_expression_t*)_expr;
    exprval_t exprval, exprvalr;
    VARIANT rval;
    HRESULT hres;

    TRACE("\n");

    hres = expr_eval(ctx, expr->expression1, EXPR_NEWREF, ei, &exprval);
    if(FAILED(hres))
        return hres;

    hres = expr_eval(ctx, expr->expression2, 0, ei, &exprvalr);
    if(SUCCEEDED(hres)) {
        hres = exprval_to_value(ctx->parser->script, &exprvalr, ei, &rval);
        exprval_release(&exprvalr);
    }

    if(SUCCEEDED(hres))
        hres = put_value(ctx->parser->script, &exprval, &rval, ei);

    exprval_release(&exprval);
    if(FAILED(hres))
        return hres;

    ret->type = EXPRVAL_VARIANT;
    ret->u.var = rval;
    return S_OK;
}

/* ECMA-262 3rd Edition    11.13.2 */
HRESULT assign_lshift_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    binary_expression_t *expr = (binary_expression_t*)_expr;

    TRACE("\n");

    return assign_oper_eval(ctx, expr->expression1, expr->expression2, lshift_eval, ei, ret);
}

/* ECMA-262 3rd Edition    11.13.2 */
HRESULT assign_rshift_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    binary_expression_t *expr = (binary_expression_t*)_expr;

    TRACE("\n");

    return assign_oper_eval(ctx, expr->expression1, expr->expression2, rshift_eval, ei, ret);
}

/* ECMA-262 3rd Edition    11.13.2 */
HRESULT assign_rrshift_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    binary_expression_t *expr = (binary_expression_t*)_expr;

    TRACE("\n");

    return assign_oper_eval(ctx, expr->expression1, expr->expression2, rshift2_eval, ei, ret);
}

/* ECMA-262 3rd Edition    11.13.2 */
HRESULT assign_add_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    binary_expression_t *expr = (binary_expression_t*)_expr;

    TRACE("\n");

    return assign_oper_eval(ctx, expr->expression1, expr->expression2, add_eval, ei, ret);
}

/* ECMA-262 3rd Edition    11.13.2 */
HRESULT assign_sub_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    binary_expression_t *expr = (binary_expression_t*)_expr;

    TRACE("\n");

    return assign_oper_eval(ctx, expr->expression1, expr->expression2, sub_eval, ei, ret);
}

/* ECMA-262 3rd Edition    11.13.2 */
HRESULT assign_mul_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    binary_expression_t *expr = (binary_expression_t*)_expr;

    TRACE("\n");

    return assign_oper_eval(ctx, expr->expression1, expr->expression2, mul_eval, ei, ret);
}

/* ECMA-262 3rd Edition    11.13.2 */
HRESULT assign_div_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    binary_expression_t *expr = (binary_expression_t*)_expr;

    TRACE("\n");

    return assign_oper_eval(ctx, expr->expression1, expr->expression2, div_eval, ei, ret);
}

/* ECMA-262 3rd Edition    11.13.2 */
HRESULT assign_mod_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    binary_expression_t *expr = (binary_expression_t*)_expr;

    TRACE("\n");

    return assign_oper_eval(ctx, expr->expression1, expr->expression2, mod_eval, ei, ret);
}

/* ECMA-262 3rd Edition    11.13.2 */
HRESULT assign_and_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    binary_expression_t *expr = (binary_expression_t*)_expr;

    TRACE("\n");

    return assign_oper_eval(ctx, expr->expression1, expr->expression2, bitand_eval, ei, ret);
}

/* ECMA-262 3rd Edition    11.13.2 */
HRESULT assign_or_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    binary_expression_t *expr = (binary_expression_t*)_expr;

    TRACE("\n");

    return assign_oper_eval(ctx, expr->expression1, expr->expression2, bitor_eval, ei, ret);
}

/* ECMA-262 3rd Edition    11.13.2 */
HRESULT assign_xor_expression_eval(exec_ctx_t *ctx, expression_t *_expr, DWORD flags, jsexcept_t *ei, exprval_t *ret)
{
    binary_expression_t *expr = (binary_expression_t*)_expr;

    TRACE("\n");

    return assign_oper_eval(ctx, expr->expression1, expr->expression2, xor_eval, ei, ret);
}
