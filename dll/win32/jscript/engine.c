/*
 * Copyright 2008,2011 Jacek Caban for CodeWeavers
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

static const WCHAR booleanW[] = {'b','o','o','l','e','a','n',0};
static const WCHAR functionW[] = {'f','u','n','c','t','i','o','n',0};
static const WCHAR numberW[] = {'n','u','m','b','e','r',0};
static const WCHAR objectW[] = {'o','b','j','e','c','t',0};
static const WCHAR stringW[] = {'s','t','r','i','n','g',0};
static const WCHAR undefinedW[] = {'u','n','d','e','f','i','n','e','d',0};
static const WCHAR unknownW[] = {'u','n','k','n','o','w','n',0};

struct _except_frame_t {
    unsigned stack_top;
    scope_chain_t *scope;
    unsigned catch_off;
    BSTR ident;

    except_frame_t *next;
};

typedef struct {
    enum {
        EXPRVAL_JSVAL,
        EXPRVAL_IDREF,
        EXPRVAL_INVALID
    } type;
    union {
        jsval_t val;
        struct {
            IDispatch *disp;
            DISPID id;
        } idref;
    } u;
} exprval_t;

static HRESULT stack_push(exec_ctx_t *ctx, jsval_t v)
{
    if(!ctx->stack_size) {
        ctx->stack = heap_alloc(16*sizeof(*ctx->stack));
        if(!ctx->stack)
            return E_OUTOFMEMORY;
        ctx->stack_size = 16;
    }else if(ctx->stack_size == ctx->top) {
        jsval_t *new_stack;

        new_stack = heap_realloc(ctx->stack, ctx->stack_size*2*sizeof(*new_stack));
        if(!new_stack) {
            jsval_release(v);
            return E_OUTOFMEMORY;
        }

        ctx->stack = new_stack;
        ctx->stack_size *= 2;
    }

    ctx->stack[ctx->top++] = v;
    return S_OK;
}

static inline HRESULT stack_push_string(exec_ctx_t *ctx, const WCHAR *str)
{
    jsstr_t *v;

    v = jsstr_alloc(str);
    if(!v)
        return E_OUTOFMEMORY;

    return stack_push(ctx, jsval_string(v));
}

static HRESULT stack_push_objid(exec_ctx_t *ctx, IDispatch *disp, DISPID id)
{
    HRESULT hres;

    hres = stack_push(ctx, jsval_disp(disp));
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, jsval_number(id));
}

static inline jsval_t stack_top(exec_ctx_t *ctx)
{
    assert(ctx->top);
    return ctx->stack[ctx->top-1];
}

static inline jsval_t stack_topn(exec_ctx_t *ctx, unsigned n)
{
    assert(ctx->top > n);
    return ctx->stack[ctx->top-1-n];
}

static inline jsval_t *stack_args(exec_ctx_t *ctx, unsigned n)
{
    if(!n)
        return NULL;
    assert(ctx->top > n-1);
    return ctx->stack + ctx->top-n;
}

static inline jsval_t stack_pop(exec_ctx_t *ctx)
{
    assert(ctx->top);
    return ctx->stack[--ctx->top];
}

static void stack_popn(exec_ctx_t *ctx, unsigned n)
{
    while(n--)
        jsval_release(stack_pop(ctx));
}

static HRESULT stack_pop_number(exec_ctx_t *ctx, double *r)
{
    jsval_t v;
    HRESULT hres;

    v = stack_pop(ctx);
    hres = to_number(ctx->script, v, r);
    jsval_release(v);
    return hres;
}

static HRESULT stack_pop_object(exec_ctx_t *ctx, IDispatch **r)
{
    jsval_t v;
    HRESULT hres;

    v = stack_pop(ctx);
    if(is_object_instance(v)) {
        if(!get_object(v))
            return throw_type_error(ctx->script, JS_E_OBJECT_REQUIRED, NULL);
        *r = get_object(v);
        return S_OK;
    }

    hres = to_object(ctx->script, v, r);
    jsval_release(v);
    return hres;
}

static inline HRESULT stack_pop_int(exec_ctx_t *ctx, INT *r)
{
    return to_int32(ctx->script, stack_pop(ctx), r);
}

static inline HRESULT stack_pop_uint(exec_ctx_t *ctx, DWORD *r)
{
    return to_uint32(ctx->script, stack_pop(ctx), r);
}

static inline IDispatch *stack_pop_objid(exec_ctx_t *ctx, DISPID *id)
{
    assert(is_number(stack_top(ctx)) && is_object_instance(stack_topn(ctx, 1)));

    *id = get_number(stack_pop(ctx));
    return get_object(stack_pop(ctx));
}

static inline IDispatch *stack_topn_objid(exec_ctx_t *ctx, unsigned n, DISPID *id)
{
    assert(is_number(stack_topn(ctx, n)) && is_object_instance(stack_topn(ctx, n+1)));

    *id = get_number(stack_topn(ctx, n));
    return get_object(stack_topn(ctx, n+1));
}

static void exprval_release(exprval_t *val)
{
    switch(val->type) {
    case EXPRVAL_JSVAL:
        jsval_release(val->u.val);
        return;
    case EXPRVAL_IDREF:
        if(val->u.idref.disp)
            IDispatch_Release(val->u.idref.disp);
        return;
    case EXPRVAL_INVALID:
        return;
    }
}

/* ECMA-262 3rd Edition    8.7.1 */
static HRESULT exprval_to_value(script_ctx_t *ctx, exprval_t *val, jsval_t *ret)
{
    switch(val->type) {
    case EXPRVAL_JSVAL:
        *ret = val->u.val;
        val->u.val = jsval_undefined();
        return S_OK;
    case EXPRVAL_IDREF:
        if(!val->u.idref.disp) {
            FIXME("throw ReferenceError\n");
            return E_FAIL;
        }

        return disp_propget(ctx, val->u.idref.disp, val->u.idref.id, ret);
    case EXPRVAL_INVALID:
        assert(0);
    }

    ERR("type %d\n", val->type);
    return E_FAIL;
}

static void exprval_set_idref(exprval_t *val, IDispatch *disp, DISPID id)
{
    val->type = EXPRVAL_IDREF;
    val->u.idref.disp = disp;
    val->u.idref.id = id;

    if(disp)
        IDispatch_AddRef(disp);
}

HRESULT scope_push(scope_chain_t *scope, jsdisp_t *jsobj, IDispatch *obj, scope_chain_t **ret)
{
    scope_chain_t *new_scope;

    new_scope = heap_alloc(sizeof(scope_chain_t));
    if(!new_scope)
        return E_OUTOFMEMORY;

    new_scope->ref = 1;

    IDispatch_AddRef(obj);
    new_scope->jsobj = jsobj;
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

void clear_ei(script_ctx_t *ctx)
{
    memset(&ctx->ei.ei, 0, sizeof(ctx->ei.ei));
    jsval_release(ctx->ei.val);
    ctx->ei.val = jsval_undefined();
}

void scope_release(scope_chain_t *scope)
{
    if(--scope->ref)
        return;

    if(scope->next)
        scope_release(scope->next);

    IDispatch_Release(scope->obj);
    heap_free(scope);
}

HRESULT create_exec_ctx(script_ctx_t *script_ctx, IDispatch *this_obj, jsdisp_t *var_disp,
        scope_chain_t *scope, BOOL is_global, exec_ctx_t **ret)
{
    exec_ctx_t *ctx;

    ctx = heap_alloc_zero(sizeof(exec_ctx_t));
    if(!ctx)
        return E_OUTOFMEMORY;

    ctx->ref = 1;
    ctx->is_global = is_global;
    ctx->ret = jsval_undefined();

    /* ECMA-262 3rd Edition    11.2.3.7 */
    if(this_obj) {
        jsdisp_t *jsthis;

        jsthis = iface_to_jsdisp((IUnknown*)this_obj);
        if(jsthis) {
            if(jsthis->builtin_info->class == JSCLASS_GLOBAL || jsthis->builtin_info->class == JSCLASS_NONE)
                this_obj = NULL;
            jsdisp_release(jsthis);
        }
    }

    if(this_obj)
        ctx->this_obj = this_obj;
    else if(script_ctx->host_global)
        ctx->this_obj = script_ctx->host_global;
    else
        ctx->this_obj = to_disp(script_ctx->global);
    IDispatch_AddRef(ctx->this_obj);

    jsdisp_addref(var_disp);
    ctx->var_disp = var_disp;

    script_addref(script_ctx);
    ctx->script = script_ctx;

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
        jsdisp_release(ctx->var_disp);
    if(ctx->this_obj)
        IDispatch_Release(ctx->this_obj);
    if(ctx->script)
        script_release(ctx->script);
    jsval_release(ctx->ret);
    heap_free(ctx->stack);
    heap_free(ctx);
}

static HRESULT disp_get_id(script_ctx_t *ctx, IDispatch *disp, const WCHAR *name, BSTR name_bstr, DWORD flags, DISPID *id)
{
    IDispatchEx *dispex;
    jsdisp_t *jsdisp;
    BSTR bstr;
    HRESULT hres;

    jsdisp = iface_to_jsdisp((IUnknown*)disp);
    if(jsdisp) {
        hres = jsdisp_get_id(jsdisp, name, flags, id);
        jsdisp_release(jsdisp);
        return hres;
    }

    if(name_bstr) {
        bstr = name_bstr;
    }else {
        bstr = SysAllocString(name);
        if(!bstr)
            return E_OUTOFMEMORY;
    }

    *id = 0;
    hres = IDispatch_QueryInterface(disp, &IID_IDispatchEx, (void**)&dispex);
    if(SUCCEEDED(hres)) {
        hres = IDispatchEx_GetDispID(dispex, bstr, make_grfdex(ctx, flags|fdexNameCaseSensitive), id);
        IDispatchEx_Release(dispex);
    }else {
        TRACE("using IDispatch\n");
        hres = IDispatch_GetIDsOfNames(disp, &IID_NULL, &bstr, 1, 0, id);
    }

    if(name_bstr != bstr)
        SysFreeString(bstr);
    return hres;
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

    if(!disp1 || !disp2) {
        *ret = FALSE;
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
static HRESULT equal2_values(jsval_t lval, jsval_t rval, BOOL *ret)
{
    jsval_type_t type = jsval_type(lval);

    TRACE("\n");

    if(type != jsval_type(rval)) {
        if(is_null_instance(lval))
            *ret = is_null_instance(rval);
        else
            *ret = FALSE;
        return S_OK;
    }

    switch(type) {
    case JSV_UNDEFINED:
    case JSV_NULL:
        *ret = TRUE;
        break;
    case JSV_OBJECT:
        return disp_cmp(get_object(lval), get_object(rval), ret);
    case JSV_STRING:
        *ret = jsstr_eq(get_string(lval), get_string(rval));
        break;
    case JSV_NUMBER:
        *ret = get_number(lval) == get_number(rval);
        break;
    case JSV_BOOL:
        *ret = !get_bool(lval) == !get_bool(rval);
        break;
    case JSV_VARIANT:
        FIXME("VARIANT not implemented\n");
        return E_NOTIMPL;
    }

    return S_OK;
}

static BOOL lookup_global_members(script_ctx_t *ctx, BSTR identifier, exprval_t *ret)
{
    named_item_t *item;
    DISPID id;
    HRESULT hres;

    for(item = ctx->named_items; item; item = item->next) {
        if(item->flags & SCRIPTITEM_GLOBALMEMBERS) {
            hres = disp_get_id(ctx, item->disp, identifier, identifier, 0, &id);
            if(SUCCEEDED(hres)) {
                if(ret)
                    exprval_set_idref(ret, item->disp, id);
                return TRUE;
            }
        }
    }

    return FALSE;
}

/* ECMA-262 3rd Edition    10.1.4 */
static HRESULT identifier_eval(script_ctx_t *ctx, BSTR identifier, exprval_t *ret)
{
    scope_chain_t *scope;
    named_item_t *item;
    DISPID id = 0;
    HRESULT hres;

    TRACE("%s\n", debugstr_w(identifier));

    if(ctx->exec_ctx) {
        for(scope = ctx->exec_ctx->scope_chain; scope; scope = scope->next) {
            if(scope->jsobj)
                hres = jsdisp_get_id(scope->jsobj, identifier, fdexNameImplicit, &id);
            else
                hres = disp_get_id(ctx, scope->obj, identifier, identifier, fdexNameImplicit, &id);
            if(SUCCEEDED(hres)) {
                exprval_set_idref(ret, scope->obj, id);
                return S_OK;
            }
        }
    }

    hres = jsdisp_get_id(ctx->global, identifier, 0, &id);
    if(SUCCEEDED(hres)) {
        exprval_set_idref(ret, to_disp(ctx->global), id);
        return S_OK;
    }

    for(item = ctx->named_items; item; item = item->next) {
        if((item->flags & SCRIPTITEM_ISVISIBLE) && !strcmpW(item->name, identifier)) {
            if(!item->disp) {
                IUnknown *unk;

                if(!ctx->site)
                    break;

                hres = IActiveScriptSite_GetItemInfo(ctx->site, identifier,
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

            IDispatch_AddRef(item->disp);
            ret->type = EXPRVAL_JSVAL;
            ret->u.val = jsval_disp(item->disp);
            return S_OK;
        }
    }

    if(lookup_global_members(ctx, identifier, ret))
        return S_OK;

    ret->type = EXPRVAL_INVALID;
    return S_OK;
}

static inline BSTR get_op_bstr(exec_ctx_t *ctx, int i){
    return ctx->code->instrs[ctx->ip].u.arg[i].bstr;
}

static inline unsigned get_op_uint(exec_ctx_t *ctx, int i){
    return ctx->code->instrs[ctx->ip].u.arg[i].uint;
}

static inline unsigned get_op_int(exec_ctx_t *ctx, int i){
    return ctx->code->instrs[ctx->ip].u.arg[i].lng;
}

static inline jsstr_t *get_op_str(exec_ctx_t *ctx, int i){
    return ctx->code->instrs[ctx->ip].u.arg[i].str;
}

static inline double get_op_double(exec_ctx_t *ctx){
    return ctx->code->instrs[ctx->ip].u.dbl;
}

/* ECMA-262 3rd Edition    12.2 */
static HRESULT interp_var_set(exec_ctx_t *ctx)
{
    const BSTR name = get_op_bstr(ctx, 0);
    jsval_t val;
    HRESULT hres;

    TRACE("%s\n", debugstr_w(name));

    val = stack_pop(ctx);
    hres = jsdisp_propput_name(ctx->var_disp, name, val);
    jsval_release(val);
    return hres;
}

/* ECMA-262 3rd Edition    12.6.4 */
static HRESULT interp_forin(exec_ctx_t *ctx)
{
    const HRESULT arg = get_op_uint(ctx, 0);
    IDispatch *var_obj, *obj = NULL;
    IDispatchEx *dispex;
    DISPID id, var_id;
    BSTR name = NULL;
    HRESULT hres;

    TRACE("\n");

    assert(is_number(stack_top(ctx)));
    id = get_number(stack_top(ctx));

    var_obj = stack_topn_objid(ctx, 1, &var_id);
    if(!var_obj) {
        FIXME("invalid ref\n");
        return E_FAIL;
    }

    if(is_object_instance(stack_topn(ctx, 3)))
        obj = get_object(stack_topn(ctx, 3));

    if(obj) {
        hres = IDispatch_QueryInterface(obj, &IID_IDispatchEx, (void**)&dispex);
        if(SUCCEEDED(hres)) {
            hres = IDispatchEx_GetNextDispID(dispex, fdexEnumDefault, id, &id);
            if(hres == S_OK)
                hres = IDispatchEx_GetMemberName(dispex, id, &name);
            IDispatchEx_Release(dispex);
            if(FAILED(hres))
                return hres;
        }else {
            TRACE("No IDispatchEx\n");
        }
    }

    if(name) {
        jsstr_t *str;

        str = jsstr_alloc_len(name, SysStringLen(name));
        SysFreeString(name);
        if(!str)
            return E_OUTOFMEMORY;

        stack_pop(ctx);
        stack_push(ctx, jsval_number(id)); /* safe, just after pop() */

        hres = disp_propput(ctx->script, var_obj, var_id, jsval_string(str));
        jsstr_release(str);
        if(FAILED(hres))
            return hres;

        ctx->ip++;
    }else {
        stack_popn(ctx, 4);
        ctx->ip = arg;
    }
    return S_OK;
}

/* ECMA-262 3rd Edition    12.10 */
static HRESULT interp_push_scope(exec_ctx_t *ctx)
{
    IDispatch *disp;
    jsval_t v;
    HRESULT hres;

    TRACE("\n");

    v = stack_pop(ctx);
    hres = to_object(ctx->script, v, &disp);
    jsval_release(v);
    if(FAILED(hres))
        return hres;

    hres = scope_push(ctx->scope_chain, to_jsdisp(disp), disp, &ctx->scope_chain);
    IDispatch_Release(disp);
    return hres;
}

/* ECMA-262 3rd Edition    12.10 */
static HRESULT interp_pop_scope(exec_ctx_t *ctx)
{
    TRACE("\n");

    scope_pop(&ctx->scope_chain);
    return S_OK;
}

/* ECMA-262 3rd Edition    12.13 */
static HRESULT interp_case(exec_ctx_t *ctx)
{
    const unsigned arg = get_op_uint(ctx, 0);
    jsval_t v;
    BOOL b;
    HRESULT hres;

    TRACE("\n");

    v = stack_pop(ctx);
    hres = equal2_values(stack_top(ctx), v, &b);
    jsval_release(v);
    if(FAILED(hres))
        return hres;

    if(b) {
        stack_popn(ctx, 1);
        ctx->ip = arg;
    }else {
        ctx->ip++;
    }
    return S_OK;
}

/* ECMA-262 3rd Edition    12.13 */
static HRESULT interp_throw(exec_ctx_t *ctx)
{
    TRACE("\n");

    jsval_release(ctx->script->ei.val);
    ctx->script->ei.val = stack_pop(ctx);
    return DISP_E_EXCEPTION;
}

static HRESULT interp_throw_ref(exec_ctx_t *ctx)
{
    const HRESULT arg = get_op_uint(ctx, 0);

    TRACE("%08x\n", arg);

    return throw_reference_error(ctx->script, arg, NULL);
}

static HRESULT interp_throw_type(exec_ctx_t *ctx)
{
    const HRESULT hres = get_op_uint(ctx, 0);
    jsstr_t *str = get_op_str(ctx, 1);
    const WCHAR *ptr;

    TRACE("%08x %s\n", hres, debugstr_jsstr(str));

    ptr = jsstr_flatten(str);
    return ptr ? throw_type_error(ctx->script, hres, ptr) : E_OUTOFMEMORY;
}

/* ECMA-262 3rd Edition    12.14 */
static HRESULT interp_push_except(exec_ctx_t *ctx)
{
    const unsigned arg1 = get_op_uint(ctx, 0);
    const BSTR arg2 = get_op_bstr(ctx, 1);
    except_frame_t *except;
    unsigned stack_top;

    TRACE("\n");

    stack_top = ctx->top;

    if(!arg2) {
        HRESULT hres;

        hres = stack_push(ctx, jsval_bool(TRUE));
        if(FAILED(hres))
            return hres;
        hres = stack_push(ctx, jsval_bool(TRUE));
        if(FAILED(hres))
            return hres;
    }

    except = heap_alloc(sizeof(*except));
    if(!except)
        return E_OUTOFMEMORY;

    except->stack_top = stack_top;
    except->scope = ctx->scope_chain;
    except->catch_off = arg1;
    except->ident = arg2;
    except->next = ctx->except_frame;
    ctx->except_frame = except;
    return S_OK;
}

/* ECMA-262 3rd Edition    12.14 */
static HRESULT interp_pop_except(exec_ctx_t *ctx)
{
    except_frame_t *except;

    TRACE("\n");

    except = ctx->except_frame;
    assert(except != NULL);

    ctx->except_frame = except->next;
    heap_free(except);
    return S_OK;
}

/* ECMA-262 3rd Edition    12.14 */
static HRESULT interp_end_finally(exec_ctx_t *ctx)
{
    jsval_t v;

    TRACE("\n");

    v = stack_pop(ctx);
    assert(is_bool(v));

    if(!get_bool(v)) {
        TRACE("passing exception\n");

        ctx->script->ei.val = stack_pop(ctx);
        return DISP_E_EXCEPTION;
    }

    stack_pop(ctx);
    return S_OK;
}

/* ECMA-262 3rd Edition    13 */
static HRESULT interp_func(exec_ctx_t *ctx)
{
    unsigned func_idx = get_op_uint(ctx, 0);
    jsdisp_t *dispex;
    HRESULT hres;

    TRACE("%d\n", func_idx);

    hres = create_source_function(ctx->script, ctx->code, ctx->func_code->funcs+func_idx,
            ctx->scope_chain, &dispex);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, jsval_obj(dispex));
}

/* ECMA-262 3rd Edition    11.2.1 */
static HRESULT interp_array(exec_ctx_t *ctx)
{
    jsstr_t *name_str;
    const WCHAR *name;
    jsval_t v, namev;
    IDispatch *obj;
    DISPID id;
    HRESULT hres;

    TRACE("\n");

    namev = stack_pop(ctx);

    hres = stack_pop_object(ctx, &obj);
    if(FAILED(hres)) {
        jsval_release(namev);
        return hres;
    }

    hres = to_flat_string(ctx->script, namev, &name_str, &name);
    jsval_release(namev);
    if(FAILED(hres)) {
        IDispatch_Release(obj);
        return hres;
    }

    hres = disp_get_id(ctx->script, obj, name, NULL, 0, &id);
    jsstr_release(name_str);
    if(SUCCEEDED(hres)) {
        hres = disp_propget(ctx->script, obj, id, &v);
    }else if(hres == DISP_E_UNKNOWNNAME) {
        v = jsval_undefined();
        hres = S_OK;
    }
    IDispatch_Release(obj);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, v);
}

/* ECMA-262 3rd Edition    11.2.1 */
static HRESULT interp_member(exec_ctx_t *ctx)
{
    const BSTR arg = get_op_bstr(ctx, 0);
    IDispatch *obj;
    jsval_t v;
    DISPID id;
    HRESULT hres;

    TRACE("\n");

    hres = stack_pop_object(ctx, &obj);
    if(FAILED(hres))
        return hres;

    hres = disp_get_id(ctx->script, obj, arg, arg, 0, &id);
    if(SUCCEEDED(hres)) {
        hres = disp_propget(ctx->script, obj, id, &v);
    }else if(hres == DISP_E_UNKNOWNNAME) {
        v = jsval_undefined();
        hres = S_OK;
    }
    IDispatch_Release(obj);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, v);
}

/* ECMA-262 3rd Edition    11.2.1 */
static HRESULT interp_memberid(exec_ctx_t *ctx)
{
    const unsigned arg = get_op_uint(ctx, 0);
    jsval_t objv, namev;
    const WCHAR *name;
    jsstr_t *name_str;
    IDispatch *obj;
    DISPID id;
    HRESULT hres;

    TRACE("%x\n", arg);

    namev = stack_pop(ctx);
    objv = stack_pop(ctx);

    hres = to_object(ctx->script, objv, &obj);
    jsval_release(objv);
    if(SUCCEEDED(hres)) {
        hres = to_flat_string(ctx->script, namev, &name_str, &name);
        if(FAILED(hres))
            IDispatch_Release(obj);
    }
    jsval_release(namev);
    if(FAILED(hres))
        return hres;

    hres = disp_get_id(ctx->script, obj, name, NULL, arg, &id);
    jsstr_release(name_str);
    if(FAILED(hres)) {
        IDispatch_Release(obj);
        if(hres == DISP_E_UNKNOWNNAME && !(arg & fdexNameEnsure)) {
            obj = NULL;
            id = JS_E_INVALID_PROPERTY;
        }else {
            ERR("failed %08x\n", hres);
            return hres;
        }
    }

    return stack_push_objid(ctx, obj, id);
}

/* ECMA-262 3rd Edition    11.2.1 */
static HRESULT interp_refval(exec_ctx_t *ctx)
{
    IDispatch *disp;
    jsval_t v;
    DISPID id;
    HRESULT hres;

    TRACE("\n");

    disp = stack_topn_objid(ctx, 0, &id);
    if(!disp)
        return throw_reference_error(ctx->script, JS_E_ILLEGAL_ASSIGN, NULL);

    hres = disp_propget(ctx->script, disp, id, &v);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, v);
}

/* ECMA-262 3rd Edition    11.2.2 */
static HRESULT interp_new(exec_ctx_t *ctx)
{
    const unsigned argc = get_op_uint(ctx, 0);
    jsval_t r, constr;
    HRESULT hres;

    TRACE("%d\n", argc);

    constr = stack_topn(ctx, argc);

    /* NOTE: Should use to_object here */

    if(is_null(constr))
        return throw_type_error(ctx->script, JS_E_OBJECT_EXPECTED, NULL);
    else if(!is_object_instance(constr))
        return throw_type_error(ctx->script, JS_E_INVALID_ACTION, NULL);
    else if(!get_object(constr))
        return throw_type_error(ctx->script, JS_E_INVALID_PROPERTY, NULL);

    hres = disp_call_value(ctx->script, get_object(constr), NULL, DISPATCH_CONSTRUCT, argc, stack_args(ctx, argc), &r);
    if(FAILED(hres))
        return hres;

    stack_popn(ctx, argc+1);
    return stack_push(ctx, r);
}

/* ECMA-262 3rd Edition    11.2.3 */
static HRESULT interp_call(exec_ctx_t *ctx)
{
    const unsigned argn = get_op_uint(ctx, 0);
    const int do_ret = get_op_int(ctx, 1);
    jsval_t r, obj;
    HRESULT hres;

    TRACE("%d %d\n", argn, do_ret);

    obj = stack_topn(ctx, argn);
    if(!is_object_instance(obj))
        return throw_type_error(ctx->script, JS_E_INVALID_PROPERTY, NULL);

    hres = disp_call_value(ctx->script, get_object(obj), NULL, DISPATCH_METHOD, argn, stack_args(ctx, argn),
            do_ret ? &r : NULL);
    if(FAILED(hres))
        return hres;

    stack_popn(ctx, argn+1);
    return do_ret ? stack_push(ctx, r) : S_OK;
}

/* ECMA-262 3rd Edition    11.2.3 */
static HRESULT interp_call_member(exec_ctx_t *ctx)
{
    const unsigned argn = get_op_uint(ctx, 0);
    const int do_ret = get_op_int(ctx, 1);
    IDispatch *obj;
    jsval_t r;
    DISPID id;
    HRESULT hres;

    TRACE("%d %d\n", argn, do_ret);

    obj = stack_topn_objid(ctx, argn, &id);
    if(!obj)
        return throw_type_error(ctx->script, id, NULL);

    hres = disp_call(ctx->script, obj, id, DISPATCH_METHOD, argn, stack_args(ctx, argn), do_ret ? &r : NULL);
    if(FAILED(hres))
        return hres;

    stack_popn(ctx, argn+2);
    return do_ret ? stack_push(ctx, r) : S_OK;

}

/* ECMA-262 3rd Edition    11.1.1 */
static HRESULT interp_this(exec_ctx_t *ctx)
{
    TRACE("\n");

    IDispatch_AddRef(ctx->this_obj);
    return stack_push(ctx, jsval_disp(ctx->this_obj));
}

/* ECMA-262 3rd Edition    10.1.4 */
static HRESULT interp_ident(exec_ctx_t *ctx)
{
    const BSTR arg = get_op_bstr(ctx, 0);
    exprval_t exprval;
    jsval_t v;
    HRESULT hres;

    TRACE("%s\n", debugstr_w(arg));

    hres = identifier_eval(ctx->script, arg, &exprval);
    if(FAILED(hres))
        return hres;

    if(exprval.type == EXPRVAL_INVALID)
        return throw_type_error(ctx->script, JS_E_UNDEFINED_VARIABLE, arg);

    hres = exprval_to_value(ctx->script, &exprval, &v);
    exprval_release(&exprval);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, v);
}

/* ECMA-262 3rd Edition    10.1.4 */
static HRESULT interp_identid(exec_ctx_t *ctx)
{
    const BSTR arg = get_op_bstr(ctx, 0);
    const unsigned flags = get_op_uint(ctx, 1);
    exprval_t exprval;
    HRESULT hres;

    TRACE("%s %x\n", debugstr_w(arg), flags);

    hres = identifier_eval(ctx->script, arg, &exprval);
    if(FAILED(hres))
        return hres;

    if(exprval.type == EXPRVAL_INVALID && (flags & fdexNameEnsure)) {
        DISPID id;

        hres = jsdisp_get_id(ctx->script->global, arg, fdexNameEnsure, &id);
        if(FAILED(hres))
            return hres;

        exprval_set_idref(&exprval, to_disp(ctx->script->global), id);
    }

    if(exprval.type != EXPRVAL_IDREF) {
        WARN("invalid ref\n");
        exprval_release(&exprval);
        return stack_push_objid(ctx, NULL, JS_E_OBJECT_EXPECTED);
    }

    return stack_push_objid(ctx, exprval.u.idref.disp, exprval.u.idref.id);
}

/* ECMA-262 3rd Edition    7.8.1 */
static HRESULT interp_null(exec_ctx_t *ctx)
{
    TRACE("\n");

    return stack_push(ctx, jsval_null());
}

/* ECMA-262 3rd Edition    7.8.2 */
static HRESULT interp_bool(exec_ctx_t *ctx)
{
    const int arg = get_op_int(ctx, 0);

    TRACE("%s\n", arg ? "true" : "false");

    return stack_push(ctx, jsval_bool(arg));
}

/* ECMA-262 3rd Edition    7.8.3 */
static HRESULT interp_int(exec_ctx_t *ctx)
{
    const int arg = get_op_int(ctx, 0);

    TRACE("%d\n", arg);

    return stack_push(ctx, jsval_number(arg));
}

/* ECMA-262 3rd Edition    7.8.3 */
static HRESULT interp_double(exec_ctx_t *ctx)
{
    const double arg = get_op_double(ctx);

    TRACE("%lf\n", arg);

    return stack_push(ctx, jsval_number(arg));
}

/* ECMA-262 3rd Edition    7.8.4 */
static HRESULT interp_str(exec_ctx_t *ctx)
{
    jsstr_t *str = get_op_str(ctx, 0);

    TRACE("%s\n", debugstr_jsstr(str));

    return stack_push(ctx, jsval_string(jsstr_addref(str)));
}

/* ECMA-262 3rd Edition    7.8 */
static HRESULT interp_regexp(exec_ctx_t *ctx)
{
    jsstr_t *source = get_op_str(ctx, 0);
    const unsigned flags = get_op_uint(ctx, 1);
    jsdisp_t *regexp;
    HRESULT hres;

    TRACE("%s %x\n", debugstr_jsstr(source), flags);

    hres = create_regexp(ctx->script, source, flags, &regexp);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, jsval_obj(regexp));
}

/* ECMA-262 3rd Edition    11.1.4 */
static HRESULT interp_carray(exec_ctx_t *ctx)
{
    const unsigned arg = get_op_uint(ctx, 0);
    jsdisp_t *array;
    jsval_t val;
    unsigned i;
    HRESULT hres;

    TRACE("%u\n", arg);

    hres = create_array(ctx->script, arg, &array);
    if(FAILED(hres))
        return hres;

    i = arg;
    while(i--) {
        val = stack_pop(ctx);
        hres = jsdisp_propput_idx(array, i, val);
        jsval_release(val);
        if(FAILED(hres)) {
            jsdisp_release(array);
            return hres;
        }
    }

    return stack_push(ctx, jsval_obj(array));
}

/* ECMA-262 3rd Edition    11.1.5 */
static HRESULT interp_new_obj(exec_ctx_t *ctx)
{
    jsdisp_t *obj;
    HRESULT hres;

    TRACE("\n");

    hres = create_object(ctx->script, NULL, &obj);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, jsval_obj(obj));
}

/* ECMA-262 3rd Edition    11.1.5 */
static HRESULT interp_obj_prop(exec_ctx_t *ctx)
{
    const BSTR name = get_op_bstr(ctx, 0);
    jsdisp_t *obj;
    jsval_t val;
    HRESULT hres;

    TRACE("%s\n", debugstr_w(name));

    val = stack_pop(ctx);

    assert(is_object_instance(stack_top(ctx)));
    obj = as_jsdisp(get_object(stack_top(ctx)));

    hres = jsdisp_propput_name(obj, name, val);
    jsval_release(val);
    return hres;
}

/* ECMA-262 3rd Edition    11.11 */
static HRESULT interp_cnd_nz(exec_ctx_t *ctx)
{
    const unsigned arg = get_op_uint(ctx, 0);
    BOOL b;
    HRESULT hres;

    TRACE("\n");

    hres = to_boolean(stack_top(ctx), &b);
    if(FAILED(hres))
        return hres;

    if(b) {
        ctx->ip = arg;
    }else {
        stack_popn(ctx, 1);
        ctx->ip++;
    }
    return S_OK;
}

/* ECMA-262 3rd Edition    11.11 */
static HRESULT interp_cnd_z(exec_ctx_t *ctx)
{
    const unsigned arg = get_op_uint(ctx, 0);
    BOOL b;
    HRESULT hres;

    TRACE("\n");

    hres = to_boolean(stack_top(ctx), &b);
    if(FAILED(hres))
        return hres;

    if(b) {
        stack_popn(ctx, 1);
        ctx->ip++;
    }else {
        ctx->ip = arg;
    }
    return S_OK;
}

/* ECMA-262 3rd Edition    11.10 */
static HRESULT interp_or(exec_ctx_t *ctx)
{
    INT l, r;
    HRESULT hres;

    TRACE("\n");

    hres = stack_pop_int(ctx, &r);
    if(FAILED(hres))
        return hres;

    hres = stack_pop_int(ctx, &l);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, jsval_number(l|r));
}

/* ECMA-262 3rd Edition    11.10 */
static HRESULT interp_xor(exec_ctx_t *ctx)
{
    INT l, r;
    HRESULT hres;

    TRACE("\n");

    hres = stack_pop_int(ctx, &r);
    if(FAILED(hres))
        return hres;

    hres = stack_pop_int(ctx, &l);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, jsval_number(l^r));
}

/* ECMA-262 3rd Edition    11.10 */
static HRESULT interp_and(exec_ctx_t *ctx)
{
    INT l, r;
    HRESULT hres;

    TRACE("\n");

    hres = stack_pop_int(ctx, &r);
    if(FAILED(hres))
        return hres;

    hres = stack_pop_int(ctx, &l);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, jsval_number(l&r));
}

/* ECMA-262 3rd Edition    11.8.6 */
static HRESULT interp_instanceof(exec_ctx_t *ctx)
{
    jsdisp_t *obj, *iter, *tmp = NULL;
    jsval_t prot, v;
    BOOL ret = FALSE;
    HRESULT hres;

    static const WCHAR prototypeW[] = {'p','r','o','t','o','t', 'y', 'p','e',0};

    v = stack_pop(ctx);
    if(!is_object_instance(v) || !get_object(v)) {
        jsval_release(v);
        return throw_type_error(ctx->script, JS_E_FUNCTION_EXPECTED, NULL);
    }

    obj = iface_to_jsdisp((IUnknown*)get_object(v));
    IDispatch_Release(get_object(v));
    if(!obj) {
        FIXME("non-jsdisp objects not supported\n");
        return E_FAIL;
    }

    if(is_class(obj, JSCLASS_FUNCTION)) {
        hres = jsdisp_propget_name(obj, prototypeW, &prot);
    }else {
        hres = throw_type_error(ctx->script, JS_E_FUNCTION_EXPECTED, NULL);
    }
    jsdisp_release(obj);
    if(FAILED(hres))
        return hres;

    v = stack_pop(ctx);

    if(is_object_instance(prot)) {
        if(is_object_instance(v))
            tmp = iface_to_jsdisp((IUnknown*)get_object(v));
        for(iter = tmp; !ret && iter; iter = iter->prototype) {
            hres = disp_cmp(get_object(prot), to_disp(iter), &ret);
            if(FAILED(hres))
                break;
        }

        if(tmp)
            jsdisp_release(tmp);
    }else {
        FIXME("prototype is not an object\n");
        hres = E_FAIL;
    }

    jsval_release(prot);
    jsval_release(v);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, jsval_bool(ret));
}

/* ECMA-262 3rd Edition    11.8.7 */
static HRESULT interp_in(exec_ctx_t *ctx)
{
    const WCHAR *str;
    jsstr_t *jsstr;
    jsval_t obj, v;
    DISPID id = 0;
    BOOL ret;
    HRESULT hres;

    TRACE("\n");

    obj = stack_pop(ctx);
    if(!is_object_instance(obj) || !get_object(obj)) {
        jsval_release(obj);
        return throw_type_error(ctx->script, JS_E_OBJECT_EXPECTED, NULL);
    }

    v = stack_pop(ctx);
    hres = to_flat_string(ctx->script, v, &jsstr, &str);
    jsval_release(v);
    if(FAILED(hres)) {
        IDispatch_Release(get_object(obj));
        return hres;
    }

    hres = disp_get_id(ctx->script, get_object(obj), str, NULL, 0, &id);
    IDispatch_Release(get_object(obj));
    jsstr_release(jsstr);
    if(SUCCEEDED(hres))
        ret = TRUE;
    else if(hres == DISP_E_UNKNOWNNAME)
        ret = FALSE;
    else
        return hres;

    return stack_push(ctx, jsval_bool(ret));
}

/* ECMA-262 3rd Edition    11.6.1 */
static HRESULT add_eval(script_ctx_t *ctx, jsval_t lval, jsval_t rval, jsval_t *ret)
{
    jsval_t r, l;
    HRESULT hres;

    hres = to_primitive(ctx, lval, &l, NO_HINT);
    if(FAILED(hres))
        return hres;

    hres = to_primitive(ctx, rval, &r, NO_HINT);
    if(FAILED(hres)) {
        jsval_release(l);
        return hres;
    }

    if(is_string(l) || is_string(r)) {
        jsstr_t *lstr, *rstr = NULL;

        hres = to_string(ctx, l, &lstr);
        if(SUCCEEDED(hres))
            hres = to_string(ctx, r, &rstr);

        if(SUCCEEDED(hres)) {
            jsstr_t *ret_str;

            ret_str = jsstr_concat(lstr, rstr);
            if(ret_str)
                *ret = jsval_string(ret_str);
            else
                hres = E_OUTOFMEMORY;
        }

        jsstr_release(lstr);
        if(rstr)
            jsstr_release(rstr);
    }else {
        double nl, nr;

        hres = to_number(ctx, l, &nl);
        if(SUCCEEDED(hres)) {
            hres = to_number(ctx, r, &nr);
            if(SUCCEEDED(hres))
                *ret = jsval_number(nl+nr);
        }
    }

    jsval_release(r);
    jsval_release(l);
    return hres;
}

/* ECMA-262 3rd Edition    11.6.1 */
static HRESULT interp_add(exec_ctx_t *ctx)
{
    jsval_t l, r, ret;
    HRESULT hres;

    r = stack_pop(ctx);
    l = stack_pop(ctx);

    TRACE("%s + %s\n", debugstr_jsval(l), debugstr_jsval(r));

    hres = add_eval(ctx->script, l, r, &ret);
    jsval_release(l);
    jsval_release(r);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, ret);
}

/* ECMA-262 3rd Edition    11.6.2 */
static HRESULT interp_sub(exec_ctx_t *ctx)
{
    double l, r;
    HRESULT hres;

    TRACE("\n");

    hres = stack_pop_number(ctx, &r);
    if(FAILED(hres))
        return hres;

    hres = stack_pop_number(ctx, &l);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, jsval_number(l-r));
}

/* ECMA-262 3rd Edition    11.5.1 */
static HRESULT interp_mul(exec_ctx_t *ctx)
{
    double l, r;
    HRESULT hres;

    TRACE("\n");

    hres = stack_pop_number(ctx, &r);
    if(FAILED(hres))
        return hres;

    hres = stack_pop_number(ctx, &l);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, jsval_number(l*r));
}

/* ECMA-262 3rd Edition    11.5.2 */
static HRESULT interp_div(exec_ctx_t *ctx)
{
    double l, r;
    HRESULT hres;

    TRACE("\n");

    hres = stack_pop_number(ctx, &r);
    if(FAILED(hres))
        return hres;

    hres = stack_pop_number(ctx, &l);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, jsval_number(l/r));
}

/* ECMA-262 3rd Edition    11.5.3 */
static HRESULT interp_mod(exec_ctx_t *ctx)
{
    double l, r;
    HRESULT hres;

    TRACE("\n");

    hres = stack_pop_number(ctx, &r);
    if(FAILED(hres))
        return hres;

    hres = stack_pop_number(ctx, &l);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, jsval_number(fmod(l, r)));
}

/* ECMA-262 3rd Edition    11.4.2 */
static HRESULT interp_delete(exec_ctx_t *ctx)
{
    jsval_t objv, namev;
    IDispatch *obj;
    jsstr_t *name;
    BOOL ret;
    HRESULT hres;

    TRACE("\n");

    namev = stack_pop(ctx);
    objv = stack_pop(ctx);

    hres = to_object(ctx->script, objv, &obj);
    jsval_release(objv);
    if(FAILED(hres)) {
        jsval_release(namev);
        return hres;
    }

    hres = to_string(ctx->script, namev, &name);
    jsval_release(namev);
    if(FAILED(hres)) {
        IDispatch_Release(obj);
        return hres;
    }

    hres = disp_delete_name(ctx->script, obj, name, &ret);
    IDispatch_Release(obj);
    jsstr_release(name);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, jsval_bool(ret));
}

/* ECMA-262 3rd Edition    11.4.2 */
static HRESULT interp_delete_ident(exec_ctx_t *ctx)
{
    const BSTR arg = get_op_bstr(ctx, 0);
    exprval_t exprval;
    BOOL ret;
    HRESULT hres;

    TRACE("%s\n", debugstr_w(arg));

    hres = identifier_eval(ctx->script, arg, &exprval);
    if(FAILED(hres))
        return hres;

    switch(exprval.type) {
    case EXPRVAL_IDREF:
        hres = disp_delete(exprval.u.idref.disp, exprval.u.idref.id, &ret);
        IDispatch_Release(exprval.u.idref.disp);
        if(FAILED(hres))
            return hres;
        break;
    case EXPRVAL_INVALID:
        ret = TRUE;
        break;
    default:
        FIXME("Unsupported exprval\n");
        exprval_release(&exprval);
        return E_NOTIMPL;
    }


    return stack_push(ctx, jsval_bool(ret));
}

/* ECMA-262 3rd Edition    11.4.2 */
static HRESULT interp_void(exec_ctx_t *ctx)
{
    TRACE("\n");

    stack_popn(ctx, 1);
    return stack_push(ctx, jsval_undefined());
}

/* ECMA-262 3rd Edition    11.4.3 */
static HRESULT typeof_string(jsval_t v, const WCHAR **ret)
{
    switch(jsval_type(v)) {
    case JSV_UNDEFINED:
        *ret = undefinedW;
        break;
    case JSV_NULL:
        *ret = objectW;
        break;
    case JSV_OBJECT: {
        jsdisp_t *dispex;

        if(get_object(v) && (dispex = iface_to_jsdisp((IUnknown*)get_object(v)))) {
            *ret = is_class(dispex, JSCLASS_FUNCTION) ? functionW : objectW;
            jsdisp_release(dispex);
        }else {
            *ret = objectW;
        }
        break;
    }
    case JSV_STRING:
        *ret = stringW;
        break;
    case JSV_NUMBER:
        *ret = numberW;
        break;
    case JSV_BOOL:
        *ret = booleanW;
        break;
    case JSV_VARIANT:
        FIXME("unhandled variant %s\n", debugstr_variant(get_variant(v)));
        return E_NOTIMPL;
    }

    return S_OK;
}

/* ECMA-262 3rd Edition    11.4.3 */
static HRESULT interp_typeofid(exec_ctx_t *ctx)
{
    const WCHAR *ret;
    IDispatch *obj;
    jsval_t v;
    DISPID id;
    HRESULT hres;

    TRACE("\n");

    obj = stack_pop_objid(ctx, &id);
    if(!obj)
        return stack_push(ctx, jsval_string(jsstr_undefined()));

    hres = disp_propget(ctx->script, obj, id, &v);
    IDispatch_Release(obj);
    if(FAILED(hres))
        return stack_push_string(ctx, unknownW);

    hres = typeof_string(v, &ret);
    jsval_release(v);
    if(FAILED(hres))
        return hres;

    return stack_push_string(ctx, ret);
}

/* ECMA-262 3rd Edition    11.4.3 */
static HRESULT interp_typeofident(exec_ctx_t *ctx)
{
    const BSTR arg = get_op_bstr(ctx, 0);
    exprval_t exprval;
    const WCHAR *ret;
    jsval_t v;
    HRESULT hres;

    TRACE("%s\n", debugstr_w(arg));

    hres = identifier_eval(ctx->script, arg, &exprval);
    if(FAILED(hres))
        return hres;

    if(exprval.type == EXPRVAL_INVALID) {
        hres = stack_push(ctx, jsval_string(jsstr_undefined()));
        exprval_release(&exprval);
        return hres;
    }

    hres = exprval_to_value(ctx->script, &exprval, &v);
    exprval_release(&exprval);
    if(FAILED(hres))
        return hres;

    hres = typeof_string(v, &ret);
    jsval_release(v);
    if(FAILED(hres))
        return hres;

    return stack_push_string(ctx, ret);
}

/* ECMA-262 3rd Edition    11.4.3 */
static HRESULT interp_typeof(exec_ctx_t *ctx)
{
    const WCHAR *ret;
    jsval_t v;
    HRESULT hres;

    TRACE("\n");

    v = stack_pop(ctx);
    hres = typeof_string(v, &ret);
    jsval_release(v);
    if(FAILED(hres))
        return hres;

    return stack_push_string(ctx, ret);
}

/* ECMA-262 3rd Edition    11.4.7 */
static HRESULT interp_minus(exec_ctx_t *ctx)
{
    double n;
    HRESULT hres;

    TRACE("\n");

    hres = stack_pop_number(ctx, &n);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, jsval_number(-n));
}

/* ECMA-262 3rd Edition    11.4.6 */
static HRESULT interp_tonum(exec_ctx_t *ctx)
{
    jsval_t v;
    double n;
    HRESULT hres;

    TRACE("\n");

    v = stack_pop(ctx);
    hres = to_number(ctx->script, v, &n);
    jsval_release(v);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, jsval_number(n));
}

/* ECMA-262 3rd Edition    11.3.1 */
static HRESULT interp_postinc(exec_ctx_t *ctx)
{
    const int arg = get_op_int(ctx, 0);
    IDispatch *obj;
    DISPID id;
    jsval_t v;
    HRESULT hres;

    TRACE("%d\n", arg);

    obj = stack_pop_objid(ctx, &id);
    if(!obj)
        return throw_type_error(ctx->script, JS_E_OBJECT_EXPECTED, NULL);

    hres = disp_propget(ctx->script, obj, id, &v);
    if(SUCCEEDED(hres)) {
        double n;

        hres = to_number(ctx->script, v, &n);
        if(SUCCEEDED(hres))
            hres = disp_propput(ctx->script, obj, id, jsval_number(n+(double)arg));
        if(FAILED(hres))
            jsval_release(v);
    }
    IDispatch_Release(obj);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, v);
}

/* ECMA-262 3rd Edition    11.4.4, 11.4.5 */
static HRESULT interp_preinc(exec_ctx_t *ctx)
{
    const int arg = get_op_int(ctx, 0);
    IDispatch *obj;
    double ret;
    DISPID id;
    jsval_t v;
    HRESULT hres;

    TRACE("%d\n", arg);

    obj = stack_pop_objid(ctx, &id);
    if(!obj)
        return throw_type_error(ctx->script, JS_E_OBJECT_EXPECTED, NULL);

    hres = disp_propget(ctx->script, obj, id, &v);
    if(SUCCEEDED(hres)) {
        double n;

        hres = to_number(ctx->script, v, &n);
        jsval_release(v);
        if(SUCCEEDED(hres)) {
            ret = n+(double)arg;
            hres = disp_propput(ctx->script, obj, id, jsval_number(ret));
        }
    }
    IDispatch_Release(obj);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, jsval_number(ret));
}

/* ECMA-262 3rd Edition    11.9.3 */
static HRESULT equal_values(script_ctx_t *ctx, jsval_t lval, jsval_t rval, BOOL *ret)
{
    if(jsval_type(lval) == jsval_type(rval) || (is_number(lval) && is_number(rval)))
       return equal2_values(lval, rval, ret);

    /* FIXME: NULL disps should be handled in more general way */
    if(is_object_instance(lval) && !get_object(lval))
        return equal_values(ctx, jsval_null(), rval, ret);
    if(is_object_instance(rval) && !get_object(rval))
        return equal_values(ctx, lval, jsval_null(), ret);

    if((is_null(lval) && is_undefined(rval)) || (is_undefined(lval) && is_null(rval))) {
        *ret = TRUE;
        return S_OK;
    }

    if(is_string(lval) && is_number(rval)) {
        double n;
        HRESULT hres;

        hres = to_number(ctx, lval, &n);
        if(FAILED(hres))
            return hres;

        /* FIXME: optimize */
        return equal_values(ctx, jsval_number(n), rval, ret);
    }

    if(is_string(rval) && is_number(lval)) {
        double n;
        HRESULT hres;

        hres = to_number(ctx, rval, &n);
        if(FAILED(hres))
            return hres;

        /* FIXME: optimize */
        return equal_values(ctx, lval, jsval_number(n), ret);
    }

    if(is_bool(rval))
        return equal_values(ctx, lval, jsval_number(get_bool(rval) ? 1 : 0), ret);

    if(is_bool(lval))
        return equal_values(ctx, jsval_number(get_bool(lval) ? 1 : 0), rval, ret);


    if(is_object_instance(rval) && (is_string(lval) || is_number(lval))) {
        jsval_t prim;
        HRESULT hres;

        hres = to_primitive(ctx, rval, &prim, NO_HINT);
        if(FAILED(hres))
            return hres;

        hres = equal_values(ctx, lval, prim, ret);
        jsval_release(prim);
        return hres;
    }


    if(is_object_instance(lval) && (is_string(rval) || is_number(rval))) {
        jsval_t prim;
        HRESULT hres;

        hres = to_primitive(ctx, lval, &prim, NO_HINT);
        if(FAILED(hres))
            return hres;

        hres = equal_values(ctx, prim, rval, ret);
        jsval_release(prim);
        return hres;
    }


    *ret = FALSE;
    return S_OK;
}

/* ECMA-262 3rd Edition    11.9.1 */
static HRESULT interp_eq(exec_ctx_t *ctx)
{
    jsval_t l, r;
    BOOL b;
    HRESULT hres;

    r = stack_pop(ctx);
    l = stack_pop(ctx);

    TRACE("%s == %s\n", debugstr_jsval(l), debugstr_jsval(r));

    hres = equal_values(ctx->script, l, r, &b);
    jsval_release(l);
    jsval_release(r);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, jsval_bool(b));
}

/* ECMA-262 3rd Edition    11.9.2 */
static HRESULT interp_neq(exec_ctx_t *ctx)
{
    jsval_t l, r;
    BOOL b;
    HRESULT hres;

    r = stack_pop(ctx);
    l = stack_pop(ctx);

    TRACE("%s != %s\n", debugstr_jsval(l), debugstr_jsval(r));

    hres = equal_values(ctx->script, l, r, &b);
    jsval_release(l);
    jsval_release(r);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, jsval_bool(!b));
}

/* ECMA-262 3rd Edition    11.9.4 */
static HRESULT interp_eq2(exec_ctx_t *ctx)
{
    jsval_t l, r;
    BOOL b;
    HRESULT hres;

    r = stack_pop(ctx);
    l = stack_pop(ctx);

    TRACE("%s === %s\n", debugstr_jsval(l), debugstr_jsval(r));

    hres = equal2_values(r, l, &b);
    jsval_release(l);
    jsval_release(r);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, jsval_bool(b));
}

/* ECMA-262 3rd Edition    11.9.5 */
static HRESULT interp_neq2(exec_ctx_t *ctx)
{
    jsval_t l, r;
    BOOL b;
    HRESULT hres;

    TRACE("\n");

    r = stack_pop(ctx);
    l = stack_pop(ctx);

    hres = equal2_values(r, l, &b);
    jsval_release(l);
    jsval_release(r);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, jsval_bool(!b));
}

/* ECMA-262 3rd Edition    11.8.5 */
static HRESULT less_eval(script_ctx_t *ctx, jsval_t lval, jsval_t rval, BOOL greater, BOOL *ret)
{
    double ln, rn;
    jsval_t l, r;
    HRESULT hres;

    hres = to_primitive(ctx, lval, &l, NO_HINT);
    if(FAILED(hres))
        return hres;

    hres = to_primitive(ctx, rval, &r, NO_HINT);
    if(FAILED(hres)) {
        jsval_release(l);
        return hres;
    }

    if(is_string(l) && is_string(r)) {
        *ret = (jsstr_cmp(get_string(l), get_string(r)) < 0) ^ greater;
        jsstr_release(get_string(l));
        jsstr_release(get_string(r));
        return S_OK;
    }

    hres = to_number(ctx, l, &ln);
    jsval_release(l);
    if(SUCCEEDED(hres))
        hres = to_number(ctx, r, &rn);
    jsval_release(r);
    if(FAILED(hres))
        return hres;

    *ret = !isnan(ln) && !isnan(rn) && ((ln < rn) ^ greater);
    return S_OK;
}

/* ECMA-262 3rd Edition    11.8.1 */
static HRESULT interp_lt(exec_ctx_t *ctx)
{
    jsval_t l, r;
    BOOL b;
    HRESULT hres;

    r = stack_pop(ctx);
    l = stack_pop(ctx);

    TRACE("%s < %s\n", debugstr_jsval(l), debugstr_jsval(r));

    hres = less_eval(ctx->script, l, r, FALSE, &b);
    jsval_release(l);
    jsval_release(r);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, jsval_bool(b));
}

/* ECMA-262 3rd Edition    11.8.1 */
static HRESULT interp_lteq(exec_ctx_t *ctx)
{
    jsval_t l, r;
    BOOL b;
    HRESULT hres;

    r = stack_pop(ctx);
    l = stack_pop(ctx);

    TRACE("%s <= %s\n", debugstr_jsval(l), debugstr_jsval(r));

    hres = less_eval(ctx->script, r, l, TRUE, &b);
    jsval_release(l);
    jsval_release(r);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, jsval_bool(b));
}

/* ECMA-262 3rd Edition    11.8.2 */
static HRESULT interp_gt(exec_ctx_t *ctx)
{
    jsval_t l, r;
    BOOL b;
    HRESULT hres;

    r = stack_pop(ctx);
    l = stack_pop(ctx);

    TRACE("%s > %s\n", debugstr_jsval(l), debugstr_jsval(r));

    hres = less_eval(ctx->script, r, l, FALSE, &b);
    jsval_release(l);
    jsval_release(r);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, jsval_bool(b));
}

/* ECMA-262 3rd Edition    11.8.4 */
static HRESULT interp_gteq(exec_ctx_t *ctx)
{
    jsval_t l, r;
    BOOL b;
    HRESULT hres;

    r = stack_pop(ctx);
    l = stack_pop(ctx);

    TRACE("%s >= %s\n", debugstr_jsval(l), debugstr_jsval(r));

    hres = less_eval(ctx->script, l, r, TRUE, &b);
    jsval_release(l);
    jsval_release(r);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, jsval_bool(b));
}

/* ECMA-262 3rd Edition    11.4.8 */
static HRESULT interp_bneg(exec_ctx_t *ctx)
{
    jsval_t v;
    INT i;
    HRESULT hres;

    TRACE("\n");

    v = stack_pop(ctx);
    hres = to_int32(ctx->script, v, &i);
    jsval_release(v);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, jsval_number(~i));
}

/* ECMA-262 3rd Edition    11.4.9 */
static HRESULT interp_neg(exec_ctx_t *ctx)
{
    jsval_t v;
    BOOL b;
    HRESULT hres;

    TRACE("\n");

    v = stack_pop(ctx);
    hres = to_boolean(v, &b);
    jsval_release(v);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, jsval_bool(!b));
}

/* ECMA-262 3rd Edition    11.7.1 */
static HRESULT interp_lshift(exec_ctx_t *ctx)
{
    DWORD r;
    INT l;
    HRESULT hres;

    hres = stack_pop_uint(ctx, &r);
    if(FAILED(hres))
        return hres;

    hres = stack_pop_int(ctx, &l);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, jsval_number(l << (r&0x1f)));
}

/* ECMA-262 3rd Edition    11.7.2 */
static HRESULT interp_rshift(exec_ctx_t *ctx)
{
    DWORD r;
    INT l;
    HRESULT hres;

    hres = stack_pop_uint(ctx, &r);
    if(FAILED(hres))
        return hres;

    hres = stack_pop_int(ctx, &l);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, jsval_number(l >> (r&0x1f)));
}

/* ECMA-262 3rd Edition    11.7.3 */
static HRESULT interp_rshift2(exec_ctx_t *ctx)
{
    DWORD r, l;
    HRESULT hres;

    hres = stack_pop_uint(ctx, &r);
    if(FAILED(hres))
        return hres;

    hres = stack_pop_uint(ctx, &l);
    if(FAILED(hres))
        return hres;

    return stack_push(ctx, jsval_number(l >> (r&0x1f)));
}

/* ECMA-262 3rd Edition    11.13.1 */
static HRESULT interp_assign(exec_ctx_t *ctx)
{
    IDispatch *disp;
    DISPID id;
    jsval_t v;
    HRESULT hres;

    TRACE("\n");

    v = stack_pop(ctx);

    disp = stack_pop_objid(ctx, &id);
    if(!disp) {
        jsval_release(v);
        return throw_reference_error(ctx->script, JS_E_ILLEGAL_ASSIGN, NULL);
    }

    hres = disp_propput(ctx->script, disp, id, v);
    IDispatch_Release(disp);
    if(FAILED(hres)) {
        jsval_release(v);
        return hres;
    }

    return stack_push(ctx, v);
}

/* JScript extension */
static HRESULT interp_assign_call(exec_ctx_t *ctx)
{
    const unsigned argc = get_op_uint(ctx, 0);
    IDispatch *disp;
    jsval_t v;
    DISPID id;
    HRESULT hres;

    TRACE("%u\n", argc);

    disp = stack_topn_objid(ctx, argc+1, &id);
    if(!disp)
        return throw_reference_error(ctx->script, JS_E_ILLEGAL_ASSIGN, NULL);

    hres = disp_call(ctx->script, disp, id, DISPATCH_PROPERTYPUT, argc+1, stack_args(ctx, argc+1), NULL);
    if(FAILED(hres))
        return hres;

    v = stack_pop(ctx);
    stack_popn(ctx, argc+2);
    return stack_push(ctx, v);
}

static HRESULT interp_undefined(exec_ctx_t *ctx)
{
    TRACE("\n");

    return stack_push(ctx, jsval_undefined());
}

static HRESULT interp_jmp(exec_ctx_t *ctx)
{
    const unsigned arg = get_op_uint(ctx, 0);

    TRACE("%u\n", arg);

    ctx->ip = arg;
    return S_OK;
}

static HRESULT interp_jmp_z(exec_ctx_t *ctx)
{
    const unsigned arg = get_op_uint(ctx, 0);
    BOOL b;
    jsval_t v;
    HRESULT hres;

    TRACE("\n");

    v = stack_pop(ctx);
    hres = to_boolean(v, &b);
    jsval_release(v);
    if(FAILED(hres))
        return hres;

    if(b)
        ctx->ip++;
    else
        ctx->ip = arg;
    return S_OK;
}

static HRESULT interp_pop(exec_ctx_t *ctx)
{
    const unsigned arg = get_op_uint(ctx, 0);

    TRACE("%u\n", arg);

    stack_popn(ctx, arg);
    return S_OK;
}

static HRESULT interp_ret(exec_ctx_t *ctx)
{
    TRACE("\n");

    ctx->ip = -1;
    return S_OK;
}

static HRESULT interp_setret(exec_ctx_t *ctx)
{
    TRACE("\n");

    jsval_release(ctx->ret);
    ctx->ret = stack_pop(ctx);
    return S_OK;
}

typedef HRESULT (*op_func_t)(exec_ctx_t*);

static const op_func_t op_funcs[] = {
#define X(x,a,b,c) interp_##x,
OP_LIST
#undef X
};

static const unsigned op_move[] = {
#define X(a,x,b,c) x,
OP_LIST
#undef X
};

static HRESULT unwind_exception(exec_ctx_t *ctx)
{
    except_frame_t *except_frame;
    jsval_t except_val;
    BSTR ident;
    HRESULT hres;

    except_frame = ctx->except_frame;
    ctx->except_frame = except_frame->next;

    assert(except_frame->stack_top <= ctx->top);
    stack_popn(ctx, ctx->top - except_frame->stack_top);

    while(except_frame->scope != ctx->scope_chain)
        scope_pop(&ctx->scope_chain);

    ctx->ip = except_frame->catch_off;

    except_val = ctx->script->ei.val;
    ctx->script->ei.val = jsval_undefined();
    clear_ei(ctx->script);

    ident = except_frame->ident;
    heap_free(except_frame);

    if(ident) {
        jsdisp_t *scope_obj;

        hres = create_dispex(ctx->script, NULL, NULL, &scope_obj);
        if(SUCCEEDED(hres)) {
            hres = jsdisp_propput_name(scope_obj, ident, except_val);
            if(FAILED(hres))
                jsdisp_release(scope_obj);
        }
        jsval_release(except_val);
        if(FAILED(hres))
            return hres;

        hres = scope_push(ctx->scope_chain, scope_obj, to_disp(scope_obj), &ctx->scope_chain);
        jsdisp_release(scope_obj);
    }else {
        hres = stack_push(ctx, except_val);
        if(FAILED(hres))
            return hres;

        hres = stack_push(ctx, jsval_bool(FALSE));
    }

    return hres;
}

static HRESULT enter_bytecode(script_ctx_t *ctx, bytecode_t *code, function_code_t *func, jsval_t *ret)
{
    exec_ctx_t *exec_ctx = ctx->exec_ctx;
    except_frame_t *prev_except_frame;
    function_code_t *prev_func;
    unsigned prev_ip, prev_top;
    scope_chain_t *prev_scope;
    bytecode_t *prev_code;
    jsop_t op;
    HRESULT hres = S_OK;

    TRACE("\n");

    prev_top = exec_ctx->top;
    prev_scope = exec_ctx->scope_chain;
    prev_except_frame = exec_ctx->except_frame;
    prev_ip = exec_ctx->ip;
    prev_code = exec_ctx->code;
    prev_func = exec_ctx->func_code;
    exec_ctx->ip = func->instr_off;
    exec_ctx->except_frame = NULL;
    exec_ctx->code = code;
    exec_ctx->func_code = func;

    while(exec_ctx->ip != -1) {
        op = code->instrs[exec_ctx->ip].op;
        hres = op_funcs[op](exec_ctx);
        if(FAILED(hres)) {
            TRACE("EXCEPTION %08x\n", hres);

            if(!exec_ctx->except_frame)
                break;

            hres = unwind_exception(exec_ctx);
            if(FAILED(hres))
                break;
        }else {
            exec_ctx->ip += op_move[op];
        }
    }

    exec_ctx->ip = prev_ip;
    exec_ctx->except_frame = prev_except_frame;
    exec_ctx->code = prev_code;
    exec_ctx->func_code = prev_func;

    if(FAILED(hres)) {
        while(exec_ctx->scope_chain != prev_scope)
            scope_pop(&exec_ctx->scope_chain);
        stack_popn(exec_ctx, exec_ctx->top-prev_top);
        return hres;
    }

    assert(exec_ctx->top == prev_top+1 || exec_ctx->top == prev_top);
    assert(exec_ctx->scope_chain == prev_scope);
    assert(exec_ctx->top == prev_top);

    *ret = exec_ctx->ret;
    exec_ctx->ret = jsval_undefined();
    return S_OK;
}

static HRESULT bind_event_target(script_ctx_t *ctx, function_code_t *func, jsdisp_t *func_obj)
{
    IBindEventHandler *target;
    exprval_t exprval;
    IDispatch *disp;
    jsval_t v;
    HRESULT hres;

    hres = identifier_eval(ctx, func->event_target, &exprval);
    if(FAILED(hres))
        return hres;

    hres = exprval_to_value(ctx, &exprval, &v);
    exprval_release(&exprval);
    if(FAILED(hres))
        return hres;

    if(!is_object_instance(v)) {
        FIXME("Can't bind to %s\n", debugstr_jsval(v));
        jsval_release(v);
    }

    disp = get_object(v);
    hres = IDispatch_QueryInterface(disp, &IID_IBindEventHandler, (void**)&target);
    if(SUCCEEDED(hres)) {
        hres = IBindEventHandler_BindHandler(target, func->name, (IDispatch*)&func_obj->IDispatchEx_iface);
        IBindEventHandler_Release(target);
        if(FAILED(hres))
            WARN("BindEvent failed: %08x\n", hres);
    }else {
        FIXME("No IBindEventHandler, not yet supported binding\n");
    }

    IDispatch_Release(disp);
    return hres;
}

HRESULT exec_source(exec_ctx_t *ctx, bytecode_t *code, function_code_t *func, BOOL from_eval, jsval_t *ret)
{
    exec_ctx_t *prev_ctx;
    jsval_t val;
    unsigned i;
    HRESULT hres = S_OK;

    for(i = 0; i < func->func_cnt; i++) {
        jsdisp_t *func_obj;

        if(!func->funcs[i].name)
            continue;

        hres = create_source_function(ctx->script, code, func->funcs+i, ctx->scope_chain, &func_obj);
        if(FAILED(hres))
            return hres;

        if(func->funcs[i].event_target)
            hres = bind_event_target(ctx->script, func->funcs+i, func_obj);
        else
            hres = jsdisp_propput_name(ctx->var_disp, func->funcs[i].name, jsval_obj(func_obj));
        jsdisp_release(func_obj);
        if(FAILED(hres))
            return hres;
    }

    for(i=0; i < func->var_cnt; i++) {
        if(!ctx->is_global || !lookup_global_members(ctx->script, func->variables[i], NULL)) {
            DISPID id = 0;

            hres = jsdisp_get_id(ctx->var_disp, func->variables[i], fdexNameEnsure, &id);
            if(FAILED(hres))
                return hres;
        }
    }

    prev_ctx = ctx->script->exec_ctx;
    ctx->script->exec_ctx = ctx;

    hres = enter_bytecode(ctx->script, code, func, &val);
    assert(ctx->script->exec_ctx == ctx);
    ctx->script->exec_ctx = prev_ctx;
    if(FAILED(hres))
        return hres;

    if(ret)
        *ret = val;
    else
        jsval_release(val);
    return S_OK;
}
