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

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(vbscript);

static DISPID propput_dispid = DISPID_PROPERTYPUT;

typedef struct {
    vbscode_t *code;
    instr_t *instr;
    script_ctx_t *script;
    function_t *func;
    vbdisp_t *vbthis;

    VARIANT *args;
    VARIANT *vars;
    SAFEARRAY **arrays;

    dynamic_var_t *dynamic_vars;
    heap_pool_t heap;

    BOOL resume_next;

    unsigned stack_size;
    unsigned top;
    VARIANT *stack;

    VARIANT ret_val;
} exec_ctx_t;

typedef HRESULT (*instr_func_t)(exec_ctx_t*);

typedef enum {
    REF_NONE,
    REF_DISP,
    REF_VAR,
    REF_OBJ,
    REF_CONST,
    REF_FUNC
} ref_type_t;

typedef struct {
    ref_type_t type;
    union {
        struct {
            IDispatch *disp;
            DISPID id;
        } d;
        VARIANT *v;
        function_t *f;
        IDispatch *obj;
    } u;
} ref_t;

typedef struct {
    VARIANT *v;
    VARIANT store;
    BOOL owned;
} variant_val_t;

static BOOL lookup_dynamic_vars(dynamic_var_t *var, const WCHAR *name, ref_t *ref)
{
    while(var) {
        if(!wcsicmp(var->name, name)) {
            ref->type = var->is_const ? REF_CONST : REF_VAR;
            ref->u.v = &var->v;
            return TRUE;
        }

        var = var->next;
    }

    return FALSE;
}

static BOOL lookup_global_vars(ScriptDisp *script, const WCHAR *name, ref_t *ref)
{
    dynamic_var_t **vars = script->global_vars;
    size_t i, cnt = script->global_vars_cnt;

    for(i = 0; i < cnt; i++) {
        if(!wcsicmp(vars[i]->name, name)) {
            ref->type = vars[i]->is_const ? REF_CONST : REF_VAR;
            ref->u.v = &vars[i]->v;
            return TRUE;
        }
    }

    return FALSE;
}

static BOOL lookup_global_funcs(ScriptDisp *script, const WCHAR *name, ref_t *ref)
{
    function_t **funcs = script->global_funcs;
    size_t i, cnt = script->global_funcs_cnt;

    for(i = 0; i < cnt; i++) {
        if(!wcsicmp(funcs[i]->name, name)) {
            ref->type = REF_FUNC;
            ref->u.f = funcs[i];
            return TRUE;
        }
    }

    return FALSE;
}

static HRESULT lookup_identifier(exec_ctx_t *ctx, BSTR name, vbdisp_invoke_type_t invoke_type, ref_t *ref)
{
    ScriptDisp *script_obj = ctx->script->script_obj;
    named_item_t *item;
    unsigned i;
    DISPID id;
    HRESULT hres;

    if(invoke_type != VBDISP_CALLGET
       && (ctx->func->type == FUNC_FUNCTION || ctx->func->type == FUNC_PROPGET)
       && !wcsicmp(name, ctx->func->name)) {
        ref->type = REF_VAR;
        ref->u.v = &ctx->ret_val;
        return S_OK;
    }

    if(ctx->func->type != FUNC_GLOBAL) {
        for(i=0; i < ctx->func->var_cnt; i++) {
            if(!wcsicmp(ctx->func->vars[i].name, name)) {
                ref->type = REF_VAR;
                ref->u.v = ctx->vars+i;
                return S_OK;
            }
        }

        for(i=0; i < ctx->func->arg_cnt; i++) {
            if(!wcsicmp(ctx->func->args[i].name, name)) {
                ref->type = REF_VAR;
                ref->u.v = ctx->args+i;
                return S_OK;
            }
        }

        if(lookup_dynamic_vars(ctx->dynamic_vars, name, ref))
            return S_OK;

        if(ctx->vbthis) {
            /* FIXME: Bind such identifier while generating bytecode. */
            for(i=0; i < ctx->vbthis->desc->prop_cnt; i++) {
                if(!wcsicmp(ctx->vbthis->desc->props[i].name, name)) {
                    ref->type = REF_VAR;
                    ref->u.v = ctx->vbthis->props+i;
                    return S_OK;
                }
            }

            hres = vbdisp_get_id(ctx->vbthis, name, invoke_type, TRUE, &id);
            if(SUCCEEDED(hres)) {
                ref->type = REF_DISP;
                ref->u.d.disp = (IDispatch*)&ctx->vbthis->IDispatchEx_iface;
                ref->u.d.id = id;
                return S_OK;
            }
        }
    }

    if(ctx->code->named_item) {
        if(lookup_global_vars(ctx->code->named_item->script_obj, name, ref))
            return S_OK;
        if(lookup_global_funcs(ctx->code->named_item->script_obj, name, ref))
            return S_OK;
    }

    if(ctx->func->code_ctx->named_item && ctx->func->code_ctx->named_item->disp &&
       !(ctx->func->code_ctx->named_item->flags & SCRIPTITEM_CODEONLY))
    {
        hres = disp_get_id(ctx->func->code_ctx->named_item->disp, name, invoke_type, TRUE, &id);
        if(SUCCEEDED(hres)) {
            ref->type = REF_DISP;
            ref->u.d.disp = ctx->func->code_ctx->named_item->disp;
            ref->u.d.id = id;
            return S_OK;
        }
    }

    if(lookup_global_vars(script_obj, name, ref))
        return S_OK;
    if(lookup_global_funcs(script_obj, name, ref))
        return S_OK;

    hres = get_builtin_id(ctx->script->global_obj, name, &id);
    if(SUCCEEDED(hres)) {
        ref->type = REF_DISP;
        ref->u.d.disp = &ctx->script->global_obj->IDispatch_iface;
        ref->u.d.id = id;
        return S_OK;
    }

    item = lookup_named_item(ctx->script, name, SCRIPTITEM_ISVISIBLE);
    if(item && item->disp) {
        ref->type = REF_OBJ;
        ref->u.obj = item->disp;
        return S_OK;
    }

    LIST_FOR_EACH_ENTRY(item, &ctx->script->named_items, named_item_t, entry) {
        if((item->flags & SCRIPTITEM_GLOBALMEMBERS)) {
            hres = disp_get_id(item->disp, name, invoke_type, FALSE, &id);
            if(SUCCEEDED(hres)) {
                ref->type = REF_DISP;
                ref->u.d.disp = item->disp;
                ref->u.d.id = id;
                return S_OK;
            }
        }
    }

    ref->type = REF_NONE;
    return S_OK;
}

static HRESULT add_dynamic_var(exec_ctx_t *ctx, const WCHAR *name,
        BOOL is_const, VARIANT **out_var)
{
    ScriptDisp *script_obj = ctx->code->named_item ? ctx->code->named_item->script_obj : ctx->script->script_obj;
    dynamic_var_t *new_var;
    heap_pool_t *heap;
    WCHAR *str;
    unsigned size;

    heap = ctx->func->type == FUNC_GLOBAL ? &script_obj->heap : &ctx->heap;

    new_var = heap_pool_alloc(heap, sizeof(*new_var));
    if(!new_var)
        return E_OUTOFMEMORY;

    size = (lstrlenW(name)+1)*sizeof(WCHAR);
    str = heap_pool_alloc(heap, size);
    if(!str)
        return E_OUTOFMEMORY;
    memcpy(str, name, size);
    new_var->name = str;
    new_var->is_const = is_const;
    new_var->array = NULL;
    V_VT(&new_var->v) = VT_EMPTY;

    if(ctx->func->type == FUNC_GLOBAL) {
        size_t cnt = script_obj->global_vars_cnt + 1;
        if(cnt > script_obj->global_vars_size) {
            dynamic_var_t **new_vars;
            if(script_obj->global_vars)
                new_vars = realloc(script_obj->global_vars, cnt * 2 * sizeof(*new_vars));
            else
                new_vars = malloc(cnt * 2 * sizeof(*new_vars));
            if(!new_vars)
                return E_OUTOFMEMORY;
            script_obj->global_vars = new_vars;
            script_obj->global_vars_size = cnt * 2;
        }
        script_obj->global_vars[script_obj->global_vars_cnt++] = new_var;
    }else {
        new_var->next = ctx->dynamic_vars;
        ctx->dynamic_vars = new_var;
    }

    *out_var = &new_var->v;
    return S_OK;
}

void clear_ei(EXCEPINFO *ei)
{
    SysFreeString(ei->bstrSource);
    SysFreeString(ei->bstrDescription);
    SysFreeString(ei->bstrHelpFile);
    memset(ei, 0, sizeof(*ei));
}

static void clear_error_loc(script_ctx_t *ctx)
{
    if(ctx->error_loc_code) {
        release_vbscode(ctx->error_loc_code);
        ctx->error_loc_code = NULL;
    }
}

static inline VARIANT *stack_pop(exec_ctx_t *ctx)
{
    assert(ctx->top);
    return ctx->stack + --ctx->top;
}

static inline VARIANT *stack_top(exec_ctx_t *ctx, unsigned n)
{
    assert(ctx->top >= n);
    return ctx->stack + (ctx->top-n-1);
}

static HRESULT stack_push(exec_ctx_t *ctx, VARIANT *v)
{
    if(ctx->stack_size == ctx->top) {
        VARIANT *new_stack;

        new_stack = realloc(ctx->stack, ctx->stack_size*2*sizeof(*ctx->stack));
        if(!new_stack) {
            VariantClear(v);
            return E_OUTOFMEMORY;
        }

        ctx->stack = new_stack;
        ctx->stack_size *= 2;
    }

    ctx->stack[ctx->top++] = *v;
    return S_OK;
}

static inline HRESULT stack_push_null(exec_ctx_t *ctx)
{
    VARIANT v;
    V_VT(&v) = VT_NULL;
    return stack_push(ctx, &v);
}

static void stack_popn(exec_ctx_t *ctx, unsigned n)
{
    while(n--)
        VariantClear(stack_pop(ctx));
}

static void stack_pop_deref(exec_ctx_t *ctx, variant_val_t *r)
{
    VARIANT *v;

    v = stack_pop(ctx);
    if(V_VT(v) == (VT_BYREF|VT_VARIANT)) {
        r->owned = FALSE;
        r->v = V_VARIANTREF(v);
    }else {
        r->owned = TRUE;
        r->v = v;
    }
}

static inline void release_val(variant_val_t *v)
{
    if(v->owned)
        VariantClear(v->v);
}

static HRESULT stack_pop_val(exec_ctx_t *ctx, variant_val_t *r)
{
    stack_pop_deref(ctx, r);

    if(V_VT(r->v) == VT_DISPATCH) {
        HRESULT hres;

        hres = get_disp_value(ctx->script, V_DISPATCH(r->v), &r->store);
        if(r->owned && V_DISPATCH(r->v))
            IDispatch_Release(V_DISPATCH(r->v));
        if(FAILED(hres))
            return hres;

        r->owned = TRUE;
        r->v = &r->store;
    }

    return S_OK;
}

static HRESULT stack_assume_val(exec_ctx_t *ctx, unsigned n)
{
    VARIANT *v = stack_top(ctx, n);
    HRESULT hres;

    if(V_VT(v) == (VT_BYREF|VT_VARIANT)) {
        VARIANT *ref = V_VARIANTREF(v);

        V_VT(v) = VT_EMPTY;
        hres = VariantCopy(v, ref);
        if(FAILED(hres))
            return hres;
    }

    if(V_VT(v) == VT_DISPATCH) {
        IDispatch *disp;

        disp = V_DISPATCH(v);
        hres = get_disp_value(ctx->script, disp, v);
        if(disp)
            IDispatch_Release(disp);
        if(FAILED(hres))
            return hres;
    }

    return S_OK;
}

static HRESULT stack_pop_bool(exec_ctx_t *ctx, BOOL *b)
{
    variant_val_t val;
    HRESULT hres;
    VARIANT v;

    hres = stack_pop_val(ctx, &val);
    if(FAILED(hres))
        return hres;

    if (V_VT(val.v) == VT_NULL)
    {
        *b = FALSE;
    }
    else
    {
        V_VT(&v) = VT_EMPTY;
        if (SUCCEEDED(hres = VariantChangeType(&v, val.v, VARIANT_LOCALBOOL, VT_BOOL)))
            *b = !!V_BOOL(&v);
    }

    release_val(&val);

    return hres;
}

static HRESULT stack_pop_disp(exec_ctx_t *ctx, IDispatch **ret)
{
    VARIANT *v = stack_pop(ctx);

    if(V_VT(v) == VT_DISPATCH) {
        *ret = V_DISPATCH(v);
        return S_OK;
    }

    if(V_VT(v) != (VT_VARIANT|VT_BYREF)) {
        FIXME("not supported type: %s\n", debugstr_variant(v));
        VariantClear(v);
        return E_FAIL;
    }

    v = V_BYREF(v);
    if(V_VT(v) != VT_DISPATCH) {
        FIXME("not disp %s\n", debugstr_variant(v));
        return E_FAIL;
    }

    if(V_DISPATCH(v))
        IDispatch_AddRef(V_DISPATCH(v));
    *ret = V_DISPATCH(v);
    return S_OK;
}

static HRESULT stack_assume_disp(exec_ctx_t *ctx, unsigned n, IDispatch **disp)
{
    VARIANT *v = stack_top(ctx, n), *ref;

    if(V_VT(v) != VT_DISPATCH && (disp || V_VT(v) != VT_UNKNOWN)) {
        if(V_VT(v) != (VT_VARIANT|VT_BYREF)) {
            FIXME("not supported type: %s\n", debugstr_variant(v));
            return E_FAIL;
        }

        ref = V_VARIANTREF(v);
        if(V_VT(ref) != VT_DISPATCH && (disp || V_VT(ref) != VT_UNKNOWN)) {
            FIXME("not disp %s\n", debugstr_variant(ref));
            return E_FAIL;
        }

        V_VT(v) = V_VT(ref);
        V_UNKNOWN(v) = V_UNKNOWN(ref);
        if(V_UNKNOWN(v))
            IUnknown_AddRef(V_UNKNOWN(v));
    }

    if(disp)
        *disp = V_DISPATCH(v);
    return S_OK;
}

static inline void instr_jmp(exec_ctx_t *ctx, unsigned addr)
{
    ctx->instr = ctx->code->instrs + addr;
}

static void vbstack_to_dp(exec_ctx_t *ctx, unsigned arg_cnt, BOOL is_propput, DISPPARAMS *dp)
{
    dp->cNamedArgs = is_propput ? 1 : 0;
    dp->cArgs = arg_cnt + dp->cNamedArgs;
    dp->rgdispidNamedArgs = is_propput ? &propput_dispid : NULL;

    if(arg_cnt) {
        VARIANT tmp;
        unsigned i;

        assert(ctx->top >= arg_cnt);

        for(i=1; i*2 <= arg_cnt; i++) {
            tmp = ctx->stack[ctx->top-i];
            ctx->stack[ctx->top-i] = ctx->stack[ctx->top-arg_cnt+i-1];
            ctx->stack[ctx->top-arg_cnt+i-1] = tmp;
        }

        dp->rgvarg = ctx->stack + ctx->top-dp->cArgs;
    }else {
        dp->rgvarg = is_propput ? ctx->stack+ctx->top-1 : NULL;
    }
}

HRESULT array_access(SAFEARRAY *array, DISPPARAMS *dp, VARIANT **ret)
{
    unsigned i, argc = arg_cnt(dp);
    LONG *indices;
    HRESULT hres;

    if(!array) {
        FIXME("NULL array\n");
        return E_FAIL;
    }

    hres = SafeArrayLock(array);
    if(FAILED(hres))
        return hres;

    if(array->cDims != argc) {
        FIXME("argc %d does not match cDims %d\n", dp->cArgs, array->cDims);
        SafeArrayUnlock(array);
        return E_FAIL;
    }

    indices = malloc(sizeof(*indices) * argc);
    if(!indices) {
        SafeArrayUnlock(array);
        return E_OUTOFMEMORY;
    }

    for(i=0; i<argc; i++) {
        hres = to_int(get_arg(dp, i), (int *)(indices+i));
        if(FAILED(hres)) {
            free(indices);
            SafeArrayUnlock(array);
            return hres;
        }
    }

    hres = SafeArrayPtrOfIndex(array, indices, (void**)ret);
    SafeArrayUnlock(array);
    free(indices);
    return hres;
}

static HRESULT variant_call(exec_ctx_t *ctx, VARIANT *v, unsigned arg_cnt, VARIANT *res)
{
    SAFEARRAY *array = NULL;
    DISPPARAMS dp;
    HRESULT hres;

    TRACE("%s\n", debugstr_variant(v));

    if(V_VT(v) == (VT_VARIANT|VT_BYREF))
        v = V_VARIANTREF(v);

    switch(V_VT(v)) {
    case VT_ARRAY|VT_BYREF|VT_VARIANT:
        array = *V_ARRAYREF(v);
        break;
    case VT_ARRAY|VT_VARIANT:
        array = V_ARRAY(v);
        break;
    case VT_DISPATCH:
        vbstack_to_dp(ctx, arg_cnt, FALSE, &dp);
        hres = disp_call(ctx->script, V_DISPATCH(v), DISPID_VALUE, &dp, res);
        break;
    default:
        FIXME("unsupported on %s\n", debugstr_variant(v));
        return E_NOTIMPL;
    }

    if(array) {
        if(!res) {
            FIXME("no res\n");
            return E_NOTIMPL;
        }

        vbstack_to_dp(ctx, arg_cnt, FALSE, &dp);
        hres = array_access(array, &dp, &v);
        if(FAILED(hres))
            return hres;

        V_VT(res) = VT_BYREF|VT_VARIANT;
        V_BYREF(res) = v;
    }

    stack_popn(ctx, arg_cnt);
    return S_OK;
}

static HRESULT do_icall(exec_ctx_t *ctx, VARIANT *res, BSTR identifier, unsigned arg_cnt)
{
    DISPPARAMS dp;
    ref_t ref;
    HRESULT hres;

    TRACE("%s %u\n", debugstr_w(identifier), arg_cnt);

    hres = lookup_identifier(ctx, identifier, VBDISP_CALLGET, &ref);
    if(FAILED(hres))
        return hres;

    switch(ref.type) {
    case REF_VAR:
    case REF_CONST:
        if(arg_cnt)
            return variant_call(ctx, ref.u.v, arg_cnt, res);

        if(!res) {
            FIXME("REF_VAR no res\n");
            return E_NOTIMPL;
        }

        V_VT(res) = VT_BYREF|VT_VARIANT;
        V_BYREF(res) = V_VT(ref.u.v) == (VT_VARIANT|VT_BYREF) ? V_VARIANTREF(ref.u.v) : ref.u.v;
        break;
    case REF_DISP:
        vbstack_to_dp(ctx, arg_cnt, FALSE, &dp);
        hres = disp_call(ctx->script, ref.u.d.disp, ref.u.d.id, &dp, res);
        if(FAILED(hres))
            return hres;
        break;
    case REF_FUNC:
        vbstack_to_dp(ctx, arg_cnt, FALSE, &dp);
        hres = exec_script(ctx->script, FALSE, ref.u.f, NULL, &dp, res);
        if(FAILED(hres))
            return hres;
        break;
    case REF_OBJ:
        if(arg_cnt) {
            FIXME("arguments on object\n");
            return E_NOTIMPL;
        }

        if(res) {
            IDispatch_AddRef(ref.u.obj);
            V_VT(res) = VT_DISPATCH;
            V_DISPATCH(res) = ref.u.obj;
        }
        break;
    case REF_NONE:
        if(res && !ctx->func->code_ctx->option_explicit && arg_cnt == 0) {
            VARIANT *new;
            hres = add_dynamic_var(ctx, identifier, FALSE, &new);
            if(FAILED(hres))
                return hres;
            V_VT(res) = VT_BYREF|VT_VARIANT;
            V_BYREF(res) = new;
            break;
        }
        FIXME("%s not found\n", debugstr_w(identifier));
        return DISP_E_UNKNOWNNAME;
    }

    stack_popn(ctx, arg_cnt);
    return S_OK;
}

static HRESULT interp_icall(exec_ctx_t *ctx)
{
    BSTR identifier = ctx->instr->arg1.bstr;
    const unsigned arg_cnt = ctx->instr->arg2.uint;
    VARIANT v;
    HRESULT hres;

    TRACE("\n");

    hres = do_icall(ctx, &v, identifier, arg_cnt);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, &v);
}

static HRESULT interp_icallv(exec_ctx_t *ctx)
{
    BSTR identifier = ctx->instr->arg1.bstr;
    const unsigned arg_cnt = ctx->instr->arg2.uint;

    TRACE("\n");

    return do_icall(ctx, NULL, identifier, arg_cnt);
}

static HRESULT interp_vcall(exec_ctx_t *ctx)
{
    const unsigned arg_cnt = ctx->instr->arg1.uint;
    VARIANT res, *v;
    HRESULT hres;

    TRACE("\n");

    v = stack_pop(ctx);
    hres = variant_call(ctx, v, arg_cnt, &res);
    VariantClear(v);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, &res);
}

static HRESULT interp_vcallv(exec_ctx_t *ctx)
{
    const unsigned arg_cnt = ctx->instr->arg1.uint;
    VARIANT *v;
    HRESULT hres;

    TRACE("\n");

    v = stack_pop(ctx);
    hres = variant_call(ctx, v, arg_cnt, NULL);
    VariantClear(v);
    return hres;
}

static HRESULT do_mcall(exec_ctx_t *ctx, VARIANT *res)
{
    const BSTR identifier = ctx->instr->arg1.bstr;
    const unsigned arg_cnt = ctx->instr->arg2.uint;
    IDispatch *obj;
    DISPPARAMS dp;
    DISPID id;
    HRESULT hres;

    hres = stack_pop_disp(ctx, &obj);
    if(FAILED(hres))
        return hres;

    if(!obj) {
        FIXME("NULL obj\n");
        return E_FAIL;
    }

    vbstack_to_dp(ctx, arg_cnt, FALSE, &dp);

    hres = disp_get_id(obj, identifier, VBDISP_CALLGET, FALSE, &id);
    if(SUCCEEDED(hres))
        hres = disp_call(ctx->script, obj, id, &dp, res);
    IDispatch_Release(obj);
    if(FAILED(hres))
        return hres;

    stack_popn(ctx, arg_cnt);
    return S_OK;
}

static HRESULT interp_mcall(exec_ctx_t *ctx)
{
    VARIANT res;
    HRESULT hres;

    TRACE("\n");

    hres = do_mcall(ctx, &res);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, &res);
}

static HRESULT interp_mcallv(exec_ctx_t *ctx)
{
    TRACE("\n");

    return do_mcall(ctx, NULL);
}

static HRESULT interp_ident(exec_ctx_t *ctx)
{
    BSTR identifier = ctx->instr->arg1.bstr;
    VARIANT v;
    HRESULT hres;

    TRACE("%s\n", debugstr_w(identifier));

    if((ctx->func->type == FUNC_FUNCTION || ctx->func->type == FUNC_PROPGET)
       && !wcsicmp(identifier, ctx->func->name)) {
        V_VT(&v) = VT_BYREF|VT_VARIANT;
        V_BYREF(&v) = &ctx->ret_val;
        return stack_push(ctx, &v);
    }

    hres = do_icall(ctx, &v, identifier, 0);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, &v);
}

static HRESULT assign_value(exec_ctx_t *ctx, VARIANT *dst, VARIANT *src, WORD flags)
{
    VARIANT value;
    HRESULT hres;

    V_VT(&value) = VT_EMPTY;
    hres = VariantCopyInd(&value, src);
    if(FAILED(hres))
        return hres;

    if(V_VT(&value) == VT_DISPATCH && !(flags & DISPATCH_PROPERTYPUTREF)) {
        IDispatch *disp = V_DISPATCH(&value);

        V_VT(&value) = VT_EMPTY;
        hres = get_disp_value(ctx->script, disp, &value);
        if(disp)
            IDispatch_Release(disp);
        if(FAILED(hres))
            return hres;
    }

    VariantClear(dst);
    *dst = value;
    return S_OK;
}

static HRESULT assign_ident(exec_ctx_t *ctx, BSTR name, WORD flags, DISPPARAMS *dp)
{
    ref_t ref;
    HRESULT hres;

    hres = lookup_identifier(ctx, name, VBDISP_LET, &ref);
    if(FAILED(hres))
        return hres;

    switch(ref.type) {
    case REF_VAR: {
        VARIANT *v = ref.u.v;

        if(V_VT(v) == (VT_VARIANT|VT_BYREF))
            v = V_VARIANTREF(v);

        if(arg_cnt(dp)) {
            SAFEARRAY *array;

            if(V_VT(v) == VT_DISPATCH) {
                hres = disp_propput(ctx->script, V_DISPATCH(v), DISPID_VALUE, flags, dp);
                break;
            }

            if(!(V_VT(v) & VT_ARRAY)) {
                FIXME("array assign on type %d\n", V_VT(v));
                return E_FAIL;
            }

            switch(V_VT(v)) {
            case VT_ARRAY|VT_BYREF|VT_VARIANT:
                array = *V_ARRAYREF(v);
                break;
            case VT_ARRAY|VT_VARIANT:
                array = V_ARRAY(v);
                break;
            default:
                FIXME("Unsupported array type %x\n", V_VT(v));
                return E_NOTIMPL;
            }

            if(!array) {
                FIXME("null array\n");
                return E_FAIL;
            }

            hres = array_access(array, dp, &v);
            if(FAILED(hres))
                return hres;
        }else if(V_VT(v) == (VT_ARRAY|VT_BYREF|VT_VARIANT)) {
            FIXME("non-array assign\n");
            return E_NOTIMPL;
        }

        hres = assign_value(ctx, v, dp->rgvarg, flags);
        break;
    }
    case REF_DISP:
        hres = disp_propput(ctx->script, ref.u.d.disp, ref.u.d.id, flags, dp);
        break;
    case REF_FUNC:
        FIXME("functions not implemented\n");
        return E_NOTIMPL;
    case REF_OBJ:
        FIXME("REF_OBJ\n");
        return E_NOTIMPL;
    case REF_CONST:
        FIXME("REF_CONST\n");
        return E_NOTIMPL;
    case REF_NONE:
        if(ctx->func->code_ctx->option_explicit) {
            FIXME("throw exception\n");
            hres = E_FAIL;
        }else {
            VARIANT *new_var;

            if(arg_cnt(dp)) {
                FIXME("arg_cnt %d not supported\n", arg_cnt(dp));
                return E_NOTIMPL;
            }

            TRACE("creating variable %s\n", debugstr_w(name));
            hres = add_dynamic_var(ctx, name, FALSE, &new_var);
            if(SUCCEEDED(hres))
                hres = assign_value(ctx, new_var, dp->rgvarg, flags);
        }
    }

    return hres;
}

static HRESULT interp_assign_ident(exec_ctx_t *ctx)
{
    const BSTR arg = ctx->instr->arg1.bstr;
    const unsigned arg_cnt = ctx->instr->arg2.uint;
    DISPPARAMS dp;
    HRESULT hres;

    TRACE("%s\n", debugstr_w(arg));

    vbstack_to_dp(ctx, arg_cnt, TRUE, &dp);
    hres = assign_ident(ctx, arg, DISPATCH_PROPERTYPUT, &dp);
    if(FAILED(hres))
        return hres;

    stack_popn(ctx, arg_cnt+1);
    return S_OK;
}

static HRESULT interp_set_ident(exec_ctx_t *ctx)
{
    const BSTR arg = ctx->instr->arg1.bstr;
    const unsigned arg_cnt = ctx->instr->arg2.uint;
    DISPPARAMS dp;
    HRESULT hres;

    TRACE("%s %u\n", debugstr_w(arg), arg_cnt);

    hres = stack_assume_disp(ctx, arg_cnt, NULL);
    if(FAILED(hres))
        return hres;

    vbstack_to_dp(ctx, arg_cnt, TRUE, &dp);
    hres = assign_ident(ctx, arg, DISPATCH_PROPERTYPUTREF, &dp);
    if(FAILED(hres))
        return hres;

    stack_popn(ctx, arg_cnt + 1);
    return S_OK;
}

static HRESULT interp_assign_member(exec_ctx_t *ctx)
{
    BSTR identifier = ctx->instr->arg1.bstr;
    const unsigned arg_cnt = ctx->instr->arg2.uint;
    IDispatch *obj;
    DISPPARAMS dp;
    DISPID id;
    HRESULT hres;

    TRACE("%s\n", debugstr_w(identifier));

    hres = stack_assume_disp(ctx, arg_cnt+1, &obj);
    if(FAILED(hres))
        return hres;

    if(!obj) {
        FIXME("NULL obj\n");
        return E_FAIL;
    }

    hres = disp_get_id(obj, identifier, VBDISP_LET, FALSE, &id);
    if(SUCCEEDED(hres)) {
        vbstack_to_dp(ctx, arg_cnt, TRUE, &dp);
        hres = disp_propput(ctx->script, obj, id, DISPATCH_PROPERTYPUT, &dp);
    }
    if(FAILED(hres))
        return hres;

    stack_popn(ctx, arg_cnt+2);
    return S_OK;
}

static HRESULT interp_set_member(exec_ctx_t *ctx)
{
    BSTR identifier = ctx->instr->arg1.bstr;
    const unsigned arg_cnt = ctx->instr->arg2.uint;
    IDispatch *obj;
    DISPPARAMS dp;
    DISPID id;
    HRESULT hres;

    TRACE("%s\n", debugstr_w(identifier));

    hres = stack_assume_disp(ctx, arg_cnt+1, &obj);
    if(FAILED(hres))
        return hres;

    if(!obj) {
        FIXME("NULL obj\n");
        return E_FAIL;
    }

    hres = stack_assume_disp(ctx, arg_cnt, NULL);
    if(FAILED(hres))
        return hres;

    hres = disp_get_id(obj, identifier, VBDISP_SET, FALSE, &id);
    if(SUCCEEDED(hres)) {
        vbstack_to_dp(ctx, arg_cnt, TRUE, &dp);
        hres = disp_propput(ctx->script, obj, id, DISPATCH_PROPERTYPUTREF, &dp);
    }
    if(FAILED(hres))
        return hres;

    stack_popn(ctx, arg_cnt+2);
    return S_OK;
}

static HRESULT interp_const(exec_ctx_t *ctx)
{
    BSTR arg = ctx->instr->arg1.bstr;
    VARIANT *v;
    ref_t ref;
    HRESULT hres;

    TRACE("%s\n", debugstr_w(arg));

    assert(ctx->func->type == FUNC_GLOBAL);

    hres = lookup_identifier(ctx, arg, VBDISP_CALLGET, &ref);
    if(FAILED(hres))
        return hres;

    if(ref.type != REF_NONE) {
        FIXME("%s already defined\n", debugstr_w(arg));
        return E_FAIL;
    }

    hres = stack_assume_val(ctx, 0);
    if(FAILED(hres))
        return hres;

    hres = add_dynamic_var(ctx, arg, TRUE, &v);
    if(FAILED(hres))
        return hres;

    *v = *stack_pop(ctx);
    return S_OK;
}

static HRESULT interp_val(exec_ctx_t *ctx)
{
    variant_val_t val;
    VARIANT v;
    HRESULT hres;

    TRACE("\n");

    hres = stack_pop_val(ctx, &val);
    if(FAILED(hres))
        return hres;

    if(!val.owned) {
        V_VT(&v) = VT_EMPTY;
        hres = VariantCopy(&v, val.v);
        if(FAILED(hres))
            return hres;
    }

    return stack_push(ctx, val.owned ? val.v : &v);
}

static HRESULT interp_numval(exec_ctx_t *ctx)
{
    variant_val_t val;
    VARIANT v;
    HRESULT hres;

    TRACE("\n");

    hres = stack_pop_val(ctx, &val);
    if(FAILED(hres))
        return hres;

    if (V_VT(val.v) == VT_BSTR) {
        V_VT(&v) = VT_EMPTY;
        hres = VariantChangeType(&v, val.v, 0, VT_R8);
        if(FAILED(hres))
            return hres;
        release_val(&val);
        return stack_push(ctx, &v);
    }

    if(!val.owned) {
        V_VT(&v) = VT_EMPTY;
        hres = VariantCopy(&v, val.v);
        if(FAILED(hres))
            return hres;
    }

    return stack_push(ctx, val.owned ? val.v : &v);
}

static HRESULT interp_pop(exec_ctx_t *ctx)
{
    const unsigned n = ctx->instr->arg1.uint;

    TRACE("%u\n", n);

    stack_popn(ctx, n);
    return S_OK;
}

static HRESULT interp_stack(exec_ctx_t *ctx)
{
    const unsigned n = ctx->instr->arg1.uint;
    VARIANT v;
    HRESULT hres;

    TRACE("%#x\n", n);

    if(n == ~0)
        return MAKE_VBSERROR(505);
    assert(n < ctx->top);

    V_VT(&v) = VT_EMPTY;
    hres = VariantCopy(&v, ctx->stack + n);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, &v);
}

static HRESULT interp_deref(exec_ctx_t *ctx)
{
    VARIANT copy, *v = stack_top(ctx, 0);
    HRESULT hres;

    TRACE("%s\n", debugstr_variant(v));

    if(V_VT(v) != (VT_BYREF|VT_VARIANT))
        return S_OK;

    V_VT(&copy) = VT_EMPTY;
    hres = VariantCopy(&copy, V_VARIANTREF(v));
    if(SUCCEEDED(hres))
        *v = copy;
    return hres;
}

static HRESULT interp_new(exec_ctx_t *ctx)
{
    const WCHAR *arg = ctx->instr->arg1.bstr;
    class_desc_t *class_desc = NULL;
    vbdisp_t *obj;
    VARIANT v;
    HRESULT hres;

    TRACE("%s\n", debugstr_w(arg));

    if(!wcsicmp(arg, L"regexp")) {
        V_VT(&v) = VT_DISPATCH;
        hres = create_regexp(&V_DISPATCH(&v));
        if(FAILED(hres))
            return hres;

        return stack_push(ctx, &v);
    }

    if(ctx->code->named_item)
        for(class_desc = ctx->code->named_item->script_obj->classes; class_desc; class_desc = class_desc->next)
            if(!wcsicmp(class_desc->name, arg))
                break;
    if(!class_desc)
        for(class_desc = ctx->script->script_obj->classes; class_desc; class_desc = class_desc->next)
            if(!wcsicmp(class_desc->name, arg))
                break;
    if(!class_desc) {
        FIXME("Class %s not found\n", debugstr_w(arg));
        return E_FAIL;
    }

    hres = create_vbdisp(class_desc, &obj);
    if(FAILED(hres))
        return hres;

    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = (IDispatch*)&obj->IDispatchEx_iface;
    return stack_push(ctx, &v);
}

static HRESULT interp_dim(exec_ctx_t *ctx)
{
    ScriptDisp *script_obj = ctx->code->named_item ? ctx->code->named_item->script_obj : ctx->script->script_obj;
    const BSTR ident = ctx->instr->arg1.bstr;
    const unsigned array_id = ctx->instr->arg2.uint;
    const array_desc_t *array_desc;
    SAFEARRAY **array_ref;
    VARIANT *v;
    HRESULT hres;

    TRACE("%s\n", debugstr_w(ident));

    assert(array_id < ctx->func->array_cnt);

    if(ctx->func->type == FUNC_GLOBAL) {
        unsigned i;
        for(i = 0; i < script_obj->global_vars_cnt; i++) {
            if(!wcsicmp(script_obj->global_vars[i]->name, ident))
                break;
        }
        assert(i < script_obj->global_vars_cnt);
        v = &script_obj->global_vars[i]->v;
        array_ref = &script_obj->global_vars[i]->array;
    }else {
        ref_t ref;

        if(!ctx->arrays) {
            ctx->arrays = calloc(ctx->func->array_cnt, sizeof(SAFEARRAY*));
            if(!ctx->arrays)
                return E_OUTOFMEMORY;
        }

        hres = lookup_identifier(ctx, ident, VBDISP_LET, &ref);
        if(FAILED(hres)) {
            FIXME("lookup %s failed: %08lx\n", debugstr_w(ident), hres);
            return hres;
        }

        if(ref.type != REF_VAR) {
            FIXME("got ref.type = %d\n", ref.type);
            return E_FAIL;
        }

        v = ref.u.v;
        array_ref = ctx->arrays + array_id;
    }

    if(*array_ref) {
        FIXME("Array already initialized\n");
        return E_FAIL;
    }

    array_desc = ctx->func->array_descs + array_id;
    if(array_desc->dim_cnt) {
        *array_ref = SafeArrayCreate(VT_VARIANT, array_desc->dim_cnt, array_desc->bounds);
        if(!*array_ref)
            return E_OUTOFMEMORY;
        (*array_ref)->fFeatures |= (FADF_FIXEDSIZE | FADF_STATIC);
    }

    V_VT(v) = VT_ARRAY|VT_BYREF|VT_VARIANT;
    V_ARRAYREF(v) = array_ref;
    return S_OK;
}

static HRESULT array_bounds_from_stack(exec_ctx_t *ctx, unsigned dim_cnt, SAFEARRAYBOUND **ret)
{
    SAFEARRAYBOUND *bounds;
    unsigned i;
    int dim;
    HRESULT hres;

    if(!(bounds = malloc(dim_cnt * sizeof(*bounds))))
        return E_OUTOFMEMORY;

    for(i = 0; i < dim_cnt; i++) {
        hres = to_int(stack_top(ctx, dim_cnt - i - 1), &dim);
        if(FAILED(hres)) {
            free(bounds);
            return hres;
        }

        bounds[i].cElements = dim + 1;
        bounds[i].lLbound = 0;
    }

    stack_popn(ctx, dim_cnt);
    *ret = bounds;
    return S_OK;
}

static HRESULT interp_redim(exec_ctx_t *ctx)
{
    BSTR identifier = ctx->instr->arg1.bstr;
    const unsigned dim_cnt = ctx->instr->arg2.uint;
    VARIANT *v;
    SAFEARRAYBOUND *bounds;
    SAFEARRAY *array;
    ref_t ref;
    HRESULT hres;

    TRACE("%s %u\n", debugstr_w(identifier), dim_cnt);

    hres = lookup_identifier(ctx, identifier, VBDISP_LET, &ref);
    if(FAILED(hres)) {
        FIXME("lookup %s failed: %08lx\n", debugstr_w(identifier), hres);
        return hres;
    }

    if(ref.type != REF_VAR) {
        FIXME("got ref.type = %d\n", ref.type);
        return E_FAIL;
    }

    v = ref.u.v;

    if(V_VT(v) == (VT_VARIANT|VT_BYREF)) {
        v = V_VARIANTREF(v);
    }

    if(V_ISARRAY(v)) {
        SAFEARRAY *sa = V_ISBYREF(v) ? *V_ARRAYREF(v) : V_ARRAY(v);
        if(sa->fFeatures & FADF_FIXEDSIZE)
            return MAKE_VBSERROR(VBSE_ARRAY_LOCKED);
    }

    hres = array_bounds_from_stack(ctx, dim_cnt, &bounds);
    if(FAILED(hres))
        return hres;

    array = SafeArrayCreate(VT_VARIANT, dim_cnt, bounds);
    free(bounds);
    if(!array)
        return E_OUTOFMEMORY;

    VariantClear(v);
    V_VT(v) = VT_ARRAY|VT_VARIANT;
    V_ARRAY(v) = array;

    return S_OK;
}

static HRESULT interp_redim_preserve(exec_ctx_t *ctx)
{
    BSTR identifier = ctx->instr->arg1.bstr;
    const unsigned dim_cnt = ctx->instr->arg2.uint;
    unsigned i;
    VARIANT *v;
    SAFEARRAYBOUND *bounds;
    SAFEARRAY *array;
    ref_t ref;
    HRESULT hres;

    TRACE("%s %u\n", debugstr_w(identifier), dim_cnt);

    hres = lookup_identifier(ctx, identifier, VBDISP_LET, &ref);
    if(FAILED(hres)) {
        FIXME("lookup %s failed: %08lx\n", debugstr_w(identifier), hres);
        return hres;
    }

    if(ref.type != REF_VAR) {
        FIXME("got ref.type = %d\n", ref.type);
        return E_FAIL;
    }

    v = ref.u.v;

    if(V_VT(v) == (VT_VARIANT|VT_BYREF)) {
        v = V_VARIANTREF(v);
    }

    if(!(V_VT(v) & VT_ARRAY)) {
        FIXME("ReDim Preserve not valid on type %d\n", V_VT(v));
        return E_FAIL;
    }

    array = V_ISBYREF(v) ? *V_ARRAYREF(v) : V_ARRAY(v);

    hres = array_bounds_from_stack(ctx, dim_cnt, &bounds);
    if(FAILED(hres))
        return hres;

    if(array == NULL || array->cDims == 0) {
        /* can initially allocate the array */
        array = SafeArrayCreate(VT_VARIANT, dim_cnt, bounds);
        if(!array)
            hres = E_OUTOFMEMORY;
	else {
            VariantClear(v);
            V_VT(v) = VT_ARRAY|VT_VARIANT;
            V_ARRAY(v) = array;
        }
    } else if(array->cDims != dim_cnt) {
        /* can't otherwise change the number of dimensions */
        TRACE("Can't resize %s, cDims %d != %d\n", debugstr_w(identifier), array->cDims, dim_cnt);
        hres = MAKE_VBSERROR(VBSE_OUT_OF_BOUNDS);
    } else {
        /* can resize the last dimensions (if others match */
        for(i = 0; i+1 < dim_cnt; ++i) {
            if(array->rgsabound[array->cDims - 1 - i].cElements != bounds[i].cElements) {
                TRACE("Can't resize %s, bound[%d] %ld != %ld\n", debugstr_w(identifier), i, array->rgsabound[i].cElements, bounds[i].cElements);
                hres = MAKE_VBSERROR(VBSE_OUT_OF_BOUNDS);
                break;
            }
        }
        if(SUCCEEDED(hres))
            hres = SafeArrayRedim(array, &bounds[dim_cnt-1]);
    }
    free(bounds);
    return hres;
}

static HRESULT interp_step(exec_ctx_t *ctx)
{
    const BSTR ident = ctx->instr->arg2.bstr;
    BOOL gteq_zero;
    VARIANT zero;
    ref_t ref;
    HRESULT hres;

    TRACE("%s\n", debugstr_w(ident));

    V_VT(&zero) = VT_I2;
    V_I2(&zero) = 0;
    hres = VarCmp(stack_top(ctx, 0), &zero, ctx->script->lcid, 0);
    if(FAILED(hres))
        return hres;

    gteq_zero = hres == VARCMP_GT || hres == VARCMP_EQ;

    hres = lookup_identifier(ctx, ident, VBDISP_ANY, &ref);
    if(FAILED(hres))
        return hres;

    if(ref.type != REF_VAR) {
        FIXME("%s is not REF_VAR\n", debugstr_w(ident));
        return E_FAIL;
    }

    hres = VarCmp(ref.u.v, stack_top(ctx, 1), ctx->script->lcid, 0);
    if(FAILED(hres))
        return hres;

    if(hres == VARCMP_EQ || hres == (gteq_zero ? VARCMP_LT : VARCMP_GT)) {
        ctx->instr++;
    }else {
        stack_popn(ctx, 2);
        instr_jmp(ctx, ctx->instr->arg1.uint);
    }
    return S_OK;
}

static HRESULT interp_newenum(exec_ctx_t *ctx)
{
    variant_val_t v;
    VARIANT *r;
    HRESULT hres;

    TRACE("\n");

    stack_pop_deref(ctx, &v);
    assert(V_VT(stack_top(ctx, 0)) == VT_EMPTY);
    r = stack_top(ctx, 0);

    switch(V_VT(v.v)) {
    case VT_DISPATCH|VT_BYREF:
    case VT_DISPATCH: {
        IEnumVARIANT *iter;
        DISPPARAMS dp = {0};
        VARIANT iterv;

        hres = disp_call(ctx->script, V_ISBYREF(v.v) ? *V_DISPATCHREF(v.v) : V_DISPATCH(v.v), DISPID_NEWENUM, &dp, &iterv);
        release_val(&v);
        if(FAILED(hres))
            return hres;

        if(V_VT(&iterv) != VT_UNKNOWN && V_VT(&iterv) != VT_DISPATCH) {
            FIXME("Unsupported iterv %s\n", debugstr_variant(&iterv));
            VariantClear(&iterv);
            return hres;
        }

        hres = IUnknown_QueryInterface(V_UNKNOWN(&iterv), &IID_IEnumVARIANT, (void**)&iter);
        IUnknown_Release(V_UNKNOWN(&iterv));
        if(FAILED(hres)) {
            FIXME("Could not get IEnumVARIANT iface: %08lx\n", hres);
            return hres;
        }

        V_VT(r) = VT_UNKNOWN;
        V_UNKNOWN(r) = (IUnknown*)iter;
        break;
    }
    case VT_VARIANT|VT_ARRAY:
    case VT_VARIANT|VT_ARRAY|VT_BYREF: {
        IEnumVARIANT *iter;

        hres = create_safearray_iter(V_ISBYREF(v.v) ? *V_ARRAYREF(v.v) : V_ARRAY(v.v), v.owned && !V_ISBYREF(v.v), &iter);
        if(FAILED(hres))
            return hres;

        V_VT(r) = VT_UNKNOWN;
        V_UNKNOWN(r) = (IUnknown*)iter;
        break;
    }
    default:
        FIXME("Unsupported for %s\n", debugstr_variant(v.v));
        release_val(&v);
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT interp_enumnext(exec_ctx_t *ctx)
{
    const unsigned loop_end = ctx->instr->arg1.uint;
    const BSTR ident = ctx->instr->arg2.bstr;
    VARIANT v;
    DISPPARAMS dp = {&v, &propput_dispid, 1, 1};
    IEnumVARIANT *iter;
    BOOL do_continue;
    HRESULT hres;

    TRACE("\n");

    if(V_VT(stack_top(ctx, 0)) == VT_EMPTY) {
        FIXME("uninitialized\n");
        return E_FAIL;
    }

    assert(V_VT(stack_top(ctx, 0)) == VT_UNKNOWN);
    iter = (IEnumVARIANT*)V_UNKNOWN(stack_top(ctx, 0));

    V_VT(&v) = VT_EMPTY;
    hres = IEnumVARIANT_Next(iter, 1, &v, NULL);
    if(FAILED(hres))
        return hres;

    do_continue = hres == S_OK;
    hres = assign_ident(ctx, ident, DISPATCH_PROPERTYPUT|DISPATCH_PROPERTYPUTREF, &dp);
    VariantClear(&v);
    if(FAILED(hres))
        return hres;

    if(do_continue) {
        ctx->instr++;
    }else {
        stack_popn(ctx, 1);
        instr_jmp(ctx, loop_end);
    }
    return S_OK;
}

static HRESULT interp_jmp(exec_ctx_t *ctx)
{
    const unsigned arg = ctx->instr->arg1.uint;

    TRACE("%u\n", arg);

    instr_jmp(ctx, arg);
    return S_OK;
}

static HRESULT interp_jmp_false(exec_ctx_t *ctx)
{
    const unsigned arg = ctx->instr->arg1.uint;
    HRESULT hres;
    BOOL b;

    TRACE("%u\n", arg);

    hres = stack_pop_bool(ctx, &b);
    if(FAILED(hres))
        return hres;

    if(b)
        ctx->instr++;
    else
        instr_jmp(ctx, ctx->instr->arg1.uint);
    return S_OK;
}

static HRESULT interp_jmp_true(exec_ctx_t *ctx)
{
    const unsigned arg = ctx->instr->arg1.uint;
    HRESULT hres;
    BOOL b;

    TRACE("%u\n", arg);

    hres = stack_pop_bool(ctx, &b);
    if(FAILED(hres))
        return hres;

    if(b)
        instr_jmp(ctx, ctx->instr->arg1.uint);
    else
        ctx->instr++;
    return S_OK;
}

static HRESULT interp_ret(exec_ctx_t *ctx)
{
    TRACE("\n");

    ctx->instr = NULL;
    return S_OK;
}

static HRESULT interp_retval(exec_ctx_t *ctx)
{
    variant_val_t val;
    HRESULT hres;

    TRACE("\n");

    stack_pop_deref(ctx, &val);

    if(val.owned) {
        VariantClear(&ctx->ret_val);
        ctx->ret_val = *val.v;
    }
    else {
        hres = VariantCopy(&ctx->ret_val, val.v);
        if(FAILED(hres))
            return hres;
    }

    return S_OK;
}

static HRESULT interp_stop(exec_ctx_t *ctx)
{
    WARN("\n");

    /* NOTE: this should have effect in debugging mode (that we don't support yet) */
    return S_OK;
}

static HRESULT interp_me(exec_ctx_t *ctx)
{
    IDispatch *disp;
    VARIANT v;

    TRACE("\n");

    if(ctx->vbthis) {
        disp = (IDispatch*)&ctx->vbthis->IDispatchEx_iface;
    }else if(ctx->code->named_item) {
        disp = (ctx->code->named_item->flags & SCRIPTITEM_CODEONLY)
               ? (IDispatch*)&ctx->code->named_item->script_obj->IDispatchEx_iface
               : ctx->code->named_item->disp;
    }else {
        named_item_t *item;
        disp = NULL;
        LIST_FOR_EACH_ENTRY(item, &ctx->script->named_items, named_item_t, entry) {
            if(!(item->flags & SCRIPTITEM_GLOBALMEMBERS)) continue;
            disp = item->disp;
            break;
        }
        if(!disp)
            disp = (IDispatch*)&ctx->script->script_obj->IDispatchEx_iface;
    }

    IDispatch_AddRef(disp);
    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = disp;
    return stack_push(ctx, &v);
}

static HRESULT interp_bool(exec_ctx_t *ctx)
{
    const VARIANT_BOOL arg = ctx->instr->arg1.lng;
    VARIANT v;

    TRACE("%s\n", arg ? "true" : "false");

    V_VT(&v) = VT_BOOL;
    V_BOOL(&v) = arg;
    return stack_push(ctx, &v);
}

static HRESULT interp_errmode(exec_ctx_t *ctx)
{
    const int err_mode = ctx->instr->arg1.uint;

    TRACE("%d\n", err_mode);

    ctx->resume_next = err_mode;
    clear_ei(&ctx->script->ei);
    return S_OK;
}

static HRESULT interp_string(exec_ctx_t *ctx)
{
    VARIANT v;

    TRACE("\n");

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = SysAllocString(ctx->instr->arg1.str);
    if(!V_BSTR(&v))
        return E_OUTOFMEMORY;

    return stack_push(ctx, &v);
}

static HRESULT interp_date(exec_ctx_t *ctx)
{
    const DATE *d = ctx->instr->arg1.date;
    VARIANT v;

    TRACE("%lf\n",*d);

    V_VT(&v) = VT_DATE;
    V_DATE(&v) = *d;

    return stack_push(ctx, &v);
}

static HRESULT interp_int(exec_ctx_t *ctx)
{
    const LONG arg = ctx->instr->arg1.lng;
    VARIANT v;

    TRACE("%ld\n", arg);

    if(arg == (INT16)arg) {
        V_VT(&v) = VT_I2;
        V_I2(&v) = arg;
    }else {
        V_VT(&v) = VT_I4;
        V_I4(&v) = arg;
    }
    return stack_push(ctx, &v);
}

static HRESULT interp_double(exec_ctx_t *ctx)
{
    const DOUBLE *arg = ctx->instr->arg1.dbl;
    VARIANT v;

    TRACE("%lf\n", *arg);

    V_VT(&v) = VT_R8;
    V_R8(&v) = *arg;
    return stack_push(ctx, &v);
}

static HRESULT interp_empty(exec_ctx_t *ctx)
{
    VARIANT v;

    TRACE("\n");

    V_VT(&v) = VT_EMPTY;
    return stack_push(ctx, &v);
}

static HRESULT interp_null(exec_ctx_t *ctx)
{
    TRACE("\n");
    return stack_push_null(ctx);
}

static HRESULT interp_nothing(exec_ctx_t *ctx)
{
    VARIANT v;

    TRACE("\n");

    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = NULL;
    return stack_push(ctx, &v);
}

static HRESULT interp_hres(exec_ctx_t *ctx)
{
    const unsigned arg = ctx->instr->arg1.uint;
    VARIANT v;

    TRACE("%d\n", arg);

    V_VT(&v) = VT_ERROR;
    V_ERROR(&v) = arg;
    return stack_push(ctx, &v);
}

static HRESULT interp_not(exec_ctx_t *ctx)
{
    variant_val_t val;
    VARIANT v;
    HRESULT hres;

    TRACE("\n");

    hres = stack_pop_val(ctx, &val);
    if(FAILED(hres))
        return hres;

    hres = VarNot(val.v, &v);
    release_val(&val);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, &v);
}

static HRESULT interp_and(exec_ctx_t *ctx)
{
    variant_val_t r, l;
    VARIANT v;
    HRESULT hres;

    TRACE("\n");

    hres = stack_pop_val(ctx, &r);
    if(FAILED(hres))
        return hres;

    hres = stack_pop_val(ctx, &l);
    if(SUCCEEDED(hres)) {
        hres = VarAnd(l.v, r.v, &v);
        release_val(&l);
    }
    release_val(&r);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, &v);
}

static HRESULT interp_or(exec_ctx_t *ctx)
{
    variant_val_t r, l;
    VARIANT v;
    HRESULT hres;

    TRACE("\n");

    hres = stack_pop_val(ctx, &r);
    if(FAILED(hres))
        return hres;

    hres = stack_pop_val(ctx, &l);
    if(SUCCEEDED(hres)) {
        hres = VarOr(l.v, r.v, &v);
        release_val(&l);
    }
    release_val(&r);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, &v);
}

static HRESULT interp_xor(exec_ctx_t *ctx)
{
    variant_val_t r, l;
    VARIANT v;
    HRESULT hres;

    TRACE("\n");

    hres = stack_pop_val(ctx, &r);
    if(FAILED(hres))
        return hres;

    hres = stack_pop_val(ctx, &l);
    if(SUCCEEDED(hres)) {
        hres = VarXor(l.v, r.v, &v);
        release_val(&l);
    }
    release_val(&r);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, &v);
}

static HRESULT interp_eqv(exec_ctx_t *ctx)
{
    variant_val_t r, l;
    VARIANT v;
    HRESULT hres;

    TRACE("\n");

    hres = stack_pop_val(ctx, &r);
    if(FAILED(hres))
        return hres;

    hres = stack_pop_val(ctx, &l);
    if(SUCCEEDED(hres)) {
        hres = VarEqv(l.v, r.v, &v);
        release_val(&l);
    }
    release_val(&r);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, &v);
}

static HRESULT interp_imp(exec_ctx_t *ctx)
{
    variant_val_t r, l;
    VARIANT v;
    HRESULT hres;

    TRACE("\n");

    hres = stack_pop_val(ctx, &r);
    if(FAILED(hres))
        return hres;

    hres = stack_pop_val(ctx, &l);
    if(SUCCEEDED(hres)) {
        hres = VarImp(l.v, r.v, &v);
        release_val(&l);
    }
    release_val(&r);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, &v);
}

static HRESULT var_cmp(exec_ctx_t *ctx, VARIANT *l, VARIANT *r)
{
    TRACE("%s %s\n", debugstr_variant(l), debugstr_variant(r));

    /* FIXME: Fix comparing string to number */

    return VarCmp(l, r, ctx->script->lcid, 0);
 }

static HRESULT cmp_oper(exec_ctx_t *ctx)
{
    variant_val_t l, r;
    HRESULT hres;

    hres = stack_pop_val(ctx, &r);
    if(FAILED(hres))
        return hres;

    hres = stack_pop_val(ctx, &l);
    if(SUCCEEDED(hres)) {
        hres = var_cmp(ctx, l.v, r.v);
        release_val(&l);
    }

    release_val(&r);
    return hres;
}

static HRESULT interp_equal(exec_ctx_t *ctx)
{
    VARIANT v;
    HRESULT hres;

    TRACE("\n");

    hres = cmp_oper(ctx);
    if(FAILED(hres))
        return hres;
    if(hres == VARCMP_NULL)
        return stack_push_null(ctx);

    V_VT(&v) = VT_BOOL;
    V_BOOL(&v) = hres == VARCMP_EQ ? VARIANT_TRUE : VARIANT_FALSE;
    return stack_push(ctx, &v);
}

static HRESULT interp_nequal(exec_ctx_t *ctx)
{
    VARIANT v;
    HRESULT hres;

    TRACE("\n");

    hres = cmp_oper(ctx);
    if(FAILED(hres))
        return hres;
    if(hres == VARCMP_NULL)
        return stack_push_null(ctx);

    V_VT(&v) = VT_BOOL;
    V_BOOL(&v) = hres != VARCMP_EQ ? VARIANT_TRUE : VARIANT_FALSE;
    return stack_push(ctx, &v);
}

static HRESULT interp_gt(exec_ctx_t *ctx)
{
    VARIANT v;
    HRESULT hres;

    TRACE("\n");

    hres = cmp_oper(ctx);
    if(FAILED(hres))
        return hres;
    if(hres == VARCMP_NULL)
        return stack_push_null(ctx);

    V_VT(&v) = VT_BOOL;
    V_BOOL(&v) = hres == VARCMP_GT ? VARIANT_TRUE : VARIANT_FALSE;
    return stack_push(ctx, &v);
}

static HRESULT interp_gteq(exec_ctx_t *ctx)
{
    VARIANT v;
    HRESULT hres;

    TRACE("\n");

    hres = cmp_oper(ctx);
    if(FAILED(hres))
        return hres;
    if(hres == VARCMP_NULL)
        return stack_push_null(ctx);

    V_VT(&v) = VT_BOOL;
    V_BOOL(&v) = hres == VARCMP_GT || hres == VARCMP_EQ ? VARIANT_TRUE : VARIANT_FALSE;
    return stack_push(ctx, &v);
}

static HRESULT interp_lt(exec_ctx_t *ctx)
{
    VARIANT v;
    HRESULT hres;

    TRACE("\n");

    hres = cmp_oper(ctx);
    if(FAILED(hres))
        return hres;
    if(hres == VARCMP_NULL)
        return stack_push_null(ctx);

    V_VT(&v) = VT_BOOL;
    V_BOOL(&v) = hres == VARCMP_LT ? VARIANT_TRUE : VARIANT_FALSE;
    return stack_push(ctx, &v);
}

static HRESULT interp_lteq(exec_ctx_t *ctx)
{
    VARIANT v;
    HRESULT hres;

    TRACE("\n");

    hres = cmp_oper(ctx);
    if(FAILED(hres))
        return hres;
    if(hres == VARCMP_NULL)
        return stack_push_null(ctx);

    V_VT(&v) = VT_BOOL;
    V_BOOL(&v) = hres == VARCMP_LT || hres == VARCMP_EQ ? VARIANT_TRUE : VARIANT_FALSE;
    return stack_push(ctx, &v);
}

static HRESULT interp_case(exec_ctx_t *ctx)
{
    const unsigned arg = ctx->instr->arg1.uint;
    variant_val_t v;
    HRESULT hres;

    TRACE("%d\n", arg);

    hres = stack_pop_val(ctx, &v);
    if(FAILED(hres))
        return hres;

    hres = var_cmp(ctx, stack_top(ctx, 0), v.v);
    release_val(&v);
    if(FAILED(hres))
        return hres;

    if(hres == VARCMP_EQ) {
        stack_popn(ctx, 1);
        instr_jmp(ctx, arg);
    }else {
        ctx->instr++;
    }

    return S_OK;
}

static HRESULT interp_is(exec_ctx_t *ctx)
{
    IUnknown *l = NULL, *r = NULL;
    variant_val_t v;
    HRESULT hres = S_OK;

    TRACE("\n");

    stack_pop_deref(ctx, &v);
    if(V_VT(v.v) != VT_DISPATCH && V_VT(v.v) != VT_UNKNOWN) {
        FIXME("Unhandled type %s\n", debugstr_variant(v.v));
        hres = E_NOTIMPL;
    }else if(V_UNKNOWN(v.v)) {
        hres = IUnknown_QueryInterface(V_UNKNOWN(v.v), &IID_IUnknown, (void**)&r);
    }
    if(v.owned) VariantClear(v.v);
    if(FAILED(hres))
        return hres;

    stack_pop_deref(ctx, &v);
    if(V_VT(v.v) != VT_DISPATCH && V_VT(v.v) != VT_UNKNOWN) {
        FIXME("Unhandled type %s\n", debugstr_variant(v.v));
        hres = E_NOTIMPL;
    }else if(V_UNKNOWN(v.v)) {
        hres = IUnknown_QueryInterface(V_UNKNOWN(v.v), &IID_IUnknown, (void**)&l);
    }
    if(v.owned) VariantClear(v.v);

    if(SUCCEEDED(hres)) {
        VARIANT res;
        V_VT(&res) = VT_BOOL;
        if(r == l)
            V_BOOL(&res) = VARIANT_TRUE;
        else if(!r || !l)
            V_BOOL(&res) = VARIANT_FALSE;
        else {
            IObjectIdentity *identity;
            hres = IUnknown_QueryInterface(l, &IID_IObjectIdentity, (void**)&identity);
            if(SUCCEEDED(hres)) {
                hres = IObjectIdentity_IsEqualObject(identity, r);
                IObjectIdentity_Release(identity);
            }
            V_BOOL(&res) = hres == S_OK ? VARIANT_TRUE : VARIANT_FALSE;
        }
        hres = stack_push(ctx, &res);
    }
    if(r)
        IUnknown_Release(r);
    if(l)
        IUnknown_Release(l);
    return hres;
}

static HRESULT interp_concat(exec_ctx_t *ctx)
{
    variant_val_t r, l;
    VARIANT v;
    HRESULT hres;

    TRACE("\n");

    hres = stack_pop_val(ctx, &r);
    if(FAILED(hres))
        return hres;

    hres = stack_pop_val(ctx, &l);
    if(SUCCEEDED(hres)) {
        hres = VarCat(l.v, r.v, &v);
        release_val(&l);
    }
    release_val(&r);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, &v);
}

static HRESULT interp_add(exec_ctx_t *ctx)
{
    variant_val_t r, l;
    VARIANT v;
    HRESULT hres;

    TRACE("\n");

    hres = stack_pop_val(ctx, &r);
    if(FAILED(hres))
        return hres;

    hres = stack_pop_val(ctx, &l);
    if(SUCCEEDED(hres)) {
        hres = VarAdd(l.v, r.v, &v);
        release_val(&l);
    }
    release_val(&r);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, &v);
}

static HRESULT interp_sub(exec_ctx_t *ctx)
{
    variant_val_t r, l;
    VARIANT v;
    HRESULT hres;

    TRACE("\n");

    hres = stack_pop_val(ctx, &r);
    if(FAILED(hres))
        return hres;

    hres = stack_pop_val(ctx, &l);
    if(SUCCEEDED(hres)) {
        hres = VarSub(l.v, r.v, &v);
        release_val(&l);
    }
    release_val(&r);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, &v);
}

static HRESULT interp_mod(exec_ctx_t *ctx)
{
    variant_val_t r, l;
    VARIANT v;
    HRESULT hres;

    TRACE("\n");

    hres = stack_pop_val(ctx, &r);
    if(FAILED(hres))
        return hres;

    hres = stack_pop_val(ctx, &l);
    if(SUCCEEDED(hres)) {
        hres = VarMod(l.v, r.v, &v);
        release_val(&l);
    }
    release_val(&r);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, &v);
}

static HRESULT interp_idiv(exec_ctx_t *ctx)
{
    variant_val_t r, l;
    VARIANT v;
    HRESULT hres;

    TRACE("\n");

    hres = stack_pop_val(ctx, &r);
    if(FAILED(hres))
        return hres;

    hres = stack_pop_val(ctx, &l);
    if(SUCCEEDED(hres)) {
        hres = VarIdiv(l.v, r.v, &v);
        release_val(&l);
    }
    release_val(&r);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, &v);
}

static HRESULT interp_div(exec_ctx_t *ctx)
{
    variant_val_t r, l;
    VARIANT v;
    HRESULT hres;

    TRACE("\n");

    hres = stack_pop_val(ctx, &r);
    if(FAILED(hres))
        return hres;

    hres = stack_pop_val(ctx, &l);
    if(SUCCEEDED(hres)) {
        hres = VarDiv(l.v, r.v, &v);
        release_val(&l);
    }
    release_val(&r);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, &v);
}

static HRESULT interp_mul(exec_ctx_t *ctx)
{
    variant_val_t r, l;
    VARIANT v;
    HRESULT hres;

    TRACE("\n");

    hres = stack_pop_val(ctx, &r);
    if(FAILED(hres))
        return hres;

    hres = stack_pop_val(ctx, &l);
    if(SUCCEEDED(hres)) {
        hres = VarMul(l.v, r.v, &v);
        release_val(&l);
    }
    release_val(&r);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, &v);
}

static HRESULT interp_exp(exec_ctx_t *ctx)
{
    variant_val_t r, l;
    VARIANT v;
    HRESULT hres;

    TRACE("\n");

    hres = stack_pop_val(ctx, &r);
    if(FAILED(hres))
        return hres;

    hres = stack_pop_val(ctx, &l);
    if(SUCCEEDED(hres)) {
        hres = VarPow(l.v, r.v, &v);
        release_val(&l);
    }
    release_val(&r);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, &v);
}

static HRESULT interp_neg(exec_ctx_t *ctx)
{
    variant_val_t val;
    VARIANT v;
    HRESULT hres;

    hres = stack_pop_val(ctx, &val);
    if(FAILED(hres))
        return hres;

    hres = VarNeg(val.v, &v);
    release_val(&val);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, &v);
}

static HRESULT interp_incc(exec_ctx_t *ctx)
{
    const BSTR ident = ctx->instr->arg1.bstr;
    VARIANT v;
    ref_t ref;
    HRESULT hres;

    TRACE("\n");

    hres = lookup_identifier(ctx, ident, VBDISP_LET, &ref);
    if(FAILED(hres))
        return hres;

    if(ref.type != REF_VAR) {
        FIXME("ref.type is not REF_VAR\n");
        return E_FAIL;
    }

    hres = VarAdd(stack_top(ctx, 0), ref.u.v, &v);
    if(FAILED(hres))
        return hres;

    VariantClear(ref.u.v);
    *ref.u.v = v;
    return S_OK;
}

static HRESULT interp_catch(exec_ctx_t *ctx)
{
    /* Nothing to do here, the OP is for unwinding only. */
    return S_OK;
}

static const instr_func_t op_funcs[] = {
#define X(x,n,a,b) interp_ ## x,
OP_LIST
#undef X
};

static const unsigned op_move[] = {
#define X(x,n,a,b) n,
OP_LIST
#undef X
};

void release_dynamic_var(dynamic_var_t *var)
{
    VariantClear(&var->v);
    if(var->array)
        SafeArrayDestroy(var->array);
}

static void release_exec(exec_ctx_t *ctx)
{
    dynamic_var_t *var;
    unsigned i;

    VariantClear(&ctx->ret_val);

    for(var = ctx->dynamic_vars; var; var = var->next)
        release_dynamic_var(var);

    if(ctx->vbthis)
        IDispatchEx_Release(&ctx->vbthis->IDispatchEx_iface);

    if(ctx->args) {
        for(i=0; i < ctx->func->arg_cnt; i++)
            VariantClear(ctx->args+i);
    }

    if(ctx->vars) {
        for(i=0; i < ctx->func->var_cnt; i++)
            VariantClear(ctx->vars+i);
    }

    if(ctx->arrays) {
        for(i=0; i < ctx->func->array_cnt; i++) {
            if(ctx->arrays[i])
                SafeArrayDestroy(ctx->arrays[i]);
        }
        free(ctx->arrays);
    }

    heap_pool_free(&ctx->heap);
    free(ctx->args);
    free(ctx->vars);
    free(ctx->stack);
}

HRESULT exec_script(script_ctx_t *ctx, BOOL extern_caller, function_t *func, vbdisp_t *vbthis, DISPPARAMS *dp, VARIANT *res)
{
    exec_ctx_t exec = {func->code_ctx};
    vbsop_t op;
    HRESULT hres = S_OK;

    exec.code = func->code_ctx;

    if(dp ? func->arg_cnt != arg_cnt(dp) : func->arg_cnt) {
        FIXME("wrong arg_cnt %d, expected %d\n", dp ? arg_cnt(dp) : 0, func->arg_cnt);
        return E_FAIL;
    }

    heap_pool_init(&exec.heap);

    TRACE("%s args=%u\n", debugstr_w(func->name),func->arg_cnt);
    if(func->arg_cnt) {
        VARIANT *v;
        unsigned i;

        exec.args = calloc(func->arg_cnt, sizeof(VARIANT));
        if(!exec.args) {
            release_exec(&exec);
            return E_OUTOFMEMORY;
        }

        for(i=0; i < func->arg_cnt; i++) {
            v = get_arg(dp, i);
            TRACE("  [%d] %s\n", i, debugstr_variant(v));
            if(V_VT(v) == (VT_VARIANT|VT_BYREF)) {
                if(func->args[i].by_ref)
                    exec.args[i] = *v;
                else
                    hres = VariantCopyInd(exec.args+i, V_VARIANTREF(v));
            }else {
                hres = VariantCopyInd(exec.args+i, v);
            }
            if(FAILED(hres)) {
                release_exec(&exec);
                return hres;
            }
        }
    }else {
        exec.args = NULL;
    }

    if(func->var_cnt) {
        exec.vars = calloc(func->var_cnt, sizeof(VARIANT));
        if(!exec.vars) {
            release_exec(&exec);
            return E_OUTOFMEMORY;
        }
    }else {
        exec.vars = NULL;
    }

    exec.stack_size = 16;
    exec.top = 0;
    exec.stack = malloc(exec.stack_size * sizeof(VARIANT));
    if(!exec.stack) {
        release_exec(&exec);
        return E_OUTOFMEMORY;
    }

    if(extern_caller)
        IActiveScriptSite_OnEnterScript(ctx->site);

    if(vbthis) {
        IDispatchEx_AddRef(&vbthis->IDispatchEx_iface);
        exec.vbthis = vbthis;
    }

    exec.instr = exec.code->instrs + func->code_off;
    exec.script = ctx;
    exec.func = func;

    while(exec.instr) {
        op = exec.instr->op;
        hres = op_funcs[op](&exec);
        if(FAILED(hres)) {
            if(hres != SCRIPT_E_RECORDED) {
                /* SCRIPT_E_RECORDED means ctx->ei is already populated */
                clear_ei(&ctx->ei);
                ctx->ei.scode = hres;
            }

            if(!ctx->ei.bstrDescription)
                map_vbs_exception(&ctx->ei);

            if(exec.resume_next) {
                unsigned stack_off;

                WARN("Failed %08lx in resume next mode\n", ctx->ei.scode);

                /*
                 * Unwinding here is simple. We need to find the next OP_catch, which contains
                 * information about expected stack size and jump offset on error. Generated
                 * bytecode needs to guarantee, that simple jump and stack adjustment will
                 * guarantee proper execution continuation.
                 */
                while((++exec.instr)->op != OP_catch);

                TRACE("unwind jmp %d stack_off %d\n", exec.instr->arg1.uint, exec.instr->arg2.uint);

                clear_error_loc(ctx);
                stack_off = exec.instr->arg2.uint;
                instr_jmp(&exec, exec.instr->arg1.uint);

                if(exec.top > stack_off) {
                    stack_popn(&exec, exec.top-stack_off);
                }else if(exec.top < stack_off) {
                    VARIANT v;

                    V_VT(&v) = VT_EMPTY;
                    while(exec.top < stack_off) {
                        hres = stack_push(&exec, &v);
                        if(FAILED(hres))
                            break;
                    }
                }

                continue;
            }else {
                if(!ctx->error_loc_code) {
                    grab_vbscode(exec.code);
                    ctx->error_loc_code = exec.code;
                    ctx->error_loc_offset = exec.instr->loc;
                }
                stack_popn(&exec, exec.top);
                break;
            }
        }

        exec.instr += op_move[op];
    }

    assert(!exec.top);

    if(extern_caller) {
        if(FAILED(hres)) {
            if(!ctx->ei.scode)
                ctx->ei.scode = hres;
            hres = report_script_error(ctx, ctx->error_loc_code, ctx->error_loc_offset);
            clear_error_loc(ctx);
        }
        IActiveScriptSite_OnLeaveScript(ctx->site);
    }

    if(SUCCEEDED(hres) && res) {
        *res = exec.ret_val;
        V_VT(&exec.ret_val) = VT_EMPTY;
    }

    release_exec(&exec);
    return hres;
}
