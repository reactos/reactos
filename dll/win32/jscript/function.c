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

typedef struct {
    DispatchEx dispex;
    builtin_invoke_t value_proc;
    DWORD flags;
    source_elements_t *source;
    parameter_t *parameters;
    scope_chain_t *scope_chain;
    parser_ctx_t *parser;
    const WCHAR *src_str;
    DWORD src_len;
    DWORD length;
} FunctionInstance;

static const WCHAR prototypeW[] = {'p','r','o','t','o','t', 'y', 'p','e',0};

static const WCHAR lengthW[] = {'l','e','n','g','t','h',0};
static const WCHAR toStringW[] = {'t','o','S','t','r','i','n','g',0};
static const WCHAR toLocaleStringW[] = {'t','o','L','o','c','a','l','e','S','t','r','i','n','g',0};
static const WCHAR valueOfW[] = {'v','a','l','u','e','O','f',0};
static const WCHAR applyW[] = {'a','p','p','l','y',0};
static const WCHAR callW[] = {'c','a','l','l',0};
static const WCHAR hasOwnPropertyW[] = {'h','a','s','O','w','n','P','r','o','p','e','r','t','y',0};
static const WCHAR propertyIsEnumerableW[] = {'p','r','o','p','e','r','t','y','I','s','E','n','u','m','e','r','a','b','l','e',0};
static const WCHAR isPrototypeOfW[] = {'i','s','P','r','o','t','o','t','y','p','e','O','f',0};

static IDispatch *get_this(DISPPARAMS *dp)
{
    DWORD i;

    for(i=0; i < dp->cNamedArgs; i++) {
        if(dp->rgdispidNamedArgs[i] == DISPID_THIS) {
            if(V_VT(dp->rgvarg+i) == VT_DISPATCH)
                return V_DISPATCH(dp->rgvarg+i);

            WARN("This is not VT_DISPATCH\n");
            return NULL;
        }
    }

    TRACE("no this passed\n");
    return NULL;
}

static HRESULT init_parameters(DispatchEx *var_disp, FunctionInstance *function, LCID lcid, DISPPARAMS *dp,
        jsexcept_t *ei, IServiceProvider *caller)
{
    parameter_t *param;
    VARIANT var_empty;
    DWORD cargs, i=0;
    HRESULT hres;

    V_VT(&var_empty) = VT_EMPTY;
    cargs = dp->cArgs - dp->cNamedArgs;

    for(param = function->parameters; param; param = param->next) {
        hres = jsdisp_propput_name(var_disp, param->identifier, lcid,
                i < cargs ? dp->rgvarg + dp->cArgs-1 - i : &var_empty,
                ei, caller);
        if(FAILED(hres))
            return hres;

        i++;
    }

    return S_OK;
}

static HRESULT init_arguments(DispatchEx *arg_disp, FunctionInstance *function, LCID lcid, DISPPARAMS *dp,
        jsexcept_t *ei, IServiceProvider *caller)
{
    VARIANT var;
    DWORD i;
    HRESULT hres;

    for(i=0; i < dp->cArgs-dp->cNamedArgs; i++) {
        hres = jsdisp_propput_idx(arg_disp, i, lcid, dp->rgvarg+dp->cArgs-1-i, ei, caller);
        if(FAILED(hres))
            return hres;
    }

    V_VT(&var) = VT_I4;
    V_I4(&var) = dp->cArgs - dp->cNamedArgs;
    return jsdisp_propput_name(arg_disp, lengthW, lcid, &var, ei, caller);
}

static HRESULT create_var_disp(FunctionInstance *function, LCID lcid, DISPPARAMS *dp, jsexcept_t *ei,
                               IServiceProvider *caller, DispatchEx **ret)
{
    DispatchEx *var_disp, *arg_disp;
    HRESULT hres;

    static const WCHAR argumentsW[] = {'a','r','g','u','m','e','n','t','s',0};

    hres = create_dispex(function->dispex.ctx, NULL, NULL, &var_disp);
    if(FAILED(hres))
        return hres;

    hres = create_dispex(function->dispex.ctx, NULL, NULL, &arg_disp);
    if(SUCCEEDED(hres)) {
        hres = init_arguments(arg_disp, function, lcid, dp, ei, caller);
        if(SUCCEEDED(hres)) {
            VARIANT var;

            V_VT(&var) = VT_DISPATCH;
            V_DISPATCH(&var) = (IDispatch*)_IDispatchEx_(arg_disp);
            hres = jsdisp_propput_name(var_disp, argumentsW, lcid, &var, ei, caller);
        }

        jsdisp_release(arg_disp);
    }

    if(SUCCEEDED(hres))
        hres = init_parameters(var_disp, function, lcid, dp, ei, caller);
    if(FAILED(hres)) {
        jsdisp_release(var_disp);
        return hres;
    }

    *ret = var_disp;
    return S_OK;
}

static HRESULT invoke_source(FunctionInstance *function, IDispatch *this_obj, LCID lcid, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    DispatchEx *var_disp;
    exec_ctx_t *exec_ctx;
    scope_chain_t *scope;
    HRESULT hres;

    if(!function->source) {
        FIXME("no source\n");
        return E_FAIL;
    }

    hres = create_var_disp(function, lcid, dp, ei, caller, &var_disp);
    if(FAILED(hres))
        return hres;

    hres = scope_push(function->scope_chain, var_disp, &scope);
    if(SUCCEEDED(hres)) {
        hres = create_exec_ctx(this_obj, var_disp, scope, &exec_ctx);
        scope_release(scope);
    }
    if(FAILED(hres))
        return hres;

    hres = exec_source(exec_ctx, function->parser, function->source, ei, retv);
    exec_release(exec_ctx);

    return hres;
}

static HRESULT invoke_function(FunctionInstance *function, LCID lcid, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    IDispatch *this_obj;

    if(!(this_obj = get_this(dp)))
        this_obj = (IDispatch*)_IDispatchEx_(function->dispex.ctx->script_disp);

    return invoke_source(function, this_obj, lcid, dp, retv, ei, caller);
}

static HRESULT invoke_constructor(FunctionInstance *function, LCID lcid, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    DispatchEx *this_obj;
    HRESULT hres;

    hres = create_object(function->dispex.ctx, &function->dispex, &this_obj);
    if(FAILED(hres))
        return hres;

    hres = invoke_source(function, (IDispatch*)_IDispatchEx_(this_obj), lcid, dp, retv, ei, caller);
    jsdisp_release(this_obj);
    if(FAILED(hres))
        return hres;

    V_VT(retv) = VT_DISPATCH;
    V_DISPATCH(retv) = (IDispatch*)_IDispatchEx_(this_obj);
    return S_OK;
}

static HRESULT invoke_value_proc(FunctionInstance *function, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    DispatchEx *this_obj = NULL;
    IDispatch *this_disp;
    HRESULT hres;

    this_disp = get_this(dp);
    if(this_disp)
        this_obj = iface_to_jsdisp((IUnknown*)this_disp);

    hres = function->value_proc(this_obj ? this_obj : function->dispex.ctx->script_disp, lcid,
                                flags, dp, retv, ei, caller);

    if(this_obj)
        jsdisp_release(this_obj);
    return hres;
}

static HRESULT function_to_string(FunctionInstance *function, BSTR *ret)
{
    BSTR str;

    if(function->value_proc) {
        FIXME("Builtin functions not implemented\n");
        return E_NOTIMPL;
    }

    str = SysAllocStringLen(function->src_str, function->src_len);
    if(!str)
        return E_OUTOFMEMORY;

    *ret = str;
    return S_OK;
}

static HRESULT Function_length(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FunctionInstance *This = (FunctionInstance*)dispex;

    TRACE("%p %d\n", This, This->length);

    switch(flags) {
    case DISPATCH_PROPERTYGET:
        V_VT(retv) = VT_I4;
        V_I4(retv) = This->length;
        break;
    default:
        FIXME("unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT Function_toString(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FunctionInstance *function;
    BSTR str;
    HRESULT hres;

    TRACE("\n");

    if(!is_class(dispex, JSCLASS_FUNCTION)) {
        FIXME("throw TypeError\n");
        return E_FAIL;
    }

    function = (FunctionInstance*)dispex;

    hres = function_to_string(function, &str);
    if(FAILED(hres))
        return hres;

    if(retv) {
        V_VT(retv) = VT_BSTR;
        V_BSTR(retv) = str;
    }else {
        SysFreeString(str);
    }
    return S_OK;
}

static HRESULT Function_toLocaleString(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Function_valueOf(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Function_apply(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Function_call(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Function_hasOwnProperty(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Function_propertyIsEnumerable(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Function_isPrototypeOf(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Function_value(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    FunctionInstance *function;

    TRACE("\n");

    if(dispex->builtin_info->class != JSCLASS_FUNCTION) {
        ERR("dispex is not a function\n");
        return E_FAIL;
    }

    function = (FunctionInstance*)dispex;

    switch(flags) {
    case DISPATCH_METHOD:
        if(function->value_proc)
            return invoke_value_proc(function, lcid, flags, dp, retv, ei, caller);

        return invoke_function(function, lcid, dp, retv, ei, caller);

    case DISPATCH_PROPERTYGET: {
        HRESULT hres;
        BSTR str;

        hres = function_to_string(function, &str);
        if(FAILED(hres))
            return hres;

        V_VT(retv) = VT_BSTR;
        V_BSTR(retv) = str;
        break;
    }

    case DISPATCH_CONSTRUCT:
        if(function->value_proc)
            return invoke_value_proc(function, lcid, flags, dp, retv, ei, caller);

        return invoke_constructor(function, lcid, dp, retv, ei, caller);

    default:
        FIXME("not implemented flags %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

static void Function_destructor(DispatchEx *dispex)
{
    FunctionInstance *This = (FunctionInstance*)dispex;

    if(This->parser)
        parser_release(This->parser);
    if(This->scope_chain)
        scope_release(This->scope_chain);
    heap_free(This);
}

static const builtin_prop_t Function_props[] = {
    {applyW,                 Function_apply,                 PROPF_METHOD},
    {callW,                  Function_call,                  PROPF_METHOD},
    {hasOwnPropertyW,        Function_hasOwnProperty,        PROPF_METHOD},
    {isPrototypeOfW,         Function_isPrototypeOf,         PROPF_METHOD},
    {lengthW,                Function_length,                0},
    {propertyIsEnumerableW,  Function_propertyIsEnumerable,  PROPF_METHOD},
    {toLocaleStringW,        Function_toLocaleString,        PROPF_METHOD},
    {toStringW,              Function_toString,              PROPF_METHOD},
    {valueOfW,               Function_valueOf,               PROPF_METHOD}
};

static const builtin_info_t Function_info = {
    JSCLASS_FUNCTION,
    {NULL, Function_value, 0},
    sizeof(Function_props)/sizeof(*Function_props),
    Function_props,
    Function_destructor,
    NULL
};

static HRESULT FunctionConstr_value(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT FunctionProt_value(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT create_function(script_ctx_t *ctx, DWORD flags, BOOL funcprot, DispatchEx *prototype, FunctionInstance **ret)
{
    FunctionInstance *function;
    HRESULT hres;

    function = heap_alloc_zero(sizeof(FunctionInstance));
    if(!function)
        return E_OUTOFMEMORY;

    if(funcprot)
        hres = init_dispex(&function->dispex, ctx, &Function_info, prototype);
    else
        hres = init_dispex_from_constr(&function->dispex, ctx, &Function_info, ctx->function_constr);
    if(FAILED(hres))
        return hres;

    function->flags = flags;
    function->length = flags & PROPF_ARGMASK;

    if(prototype) {
        jsexcept_t jsexcept;
        VARIANT var;

        V_VT(&var) = VT_DISPATCH;
        V_DISPATCH(&var) = (IDispatch*)_IDispatchEx_(prototype);
        memset(&jsexcept, 0, sizeof(jsexcept));

        hres = jsdisp_propput_name(&function->dispex, prototypeW, ctx->lcid, &var, &jsexcept, NULL/*FIXME*/);
        if(FAILED(hres)) {
            IDispatchEx_Release(_IDispatchEx_(&function->dispex));
            return hres;
        }
    }

    *ret = function;
    return S_OK;
}

HRESULT create_builtin_function(script_ctx_t *ctx, builtin_invoke_t value_proc, DWORD flags,
        DispatchEx *prototype, DispatchEx **ret)
{
    FunctionInstance *function;
    HRESULT hres;

    hres = create_function(ctx, flags, FALSE, prototype, &function);
    if(FAILED(hres))
        return hres;

    function->value_proc = value_proc;

    *ret = &function->dispex;
    return S_OK;
}

HRESULT create_source_function(parser_ctx_t *ctx, parameter_t *parameters, source_elements_t *source,
        scope_chain_t *scope_chain, const WCHAR *src_str, DWORD src_len, DispatchEx **ret)
{
    FunctionInstance *function;
    DispatchEx *prototype;
    parameter_t *iter;
    DWORD length = 0;
    HRESULT hres;

    hres = create_object(ctx->script, NULL, &prototype);
    if(FAILED(hres))
        return hres;

    hres = create_function(ctx->script, PROPF_CONSTR, FALSE, prototype, &function);
    jsdisp_release(prototype);
    if(FAILED(hres))
        return hres;

    function->source = source;
    function->parameters = parameters;

    if(scope_chain) {
        scope_addref(scope_chain);
        function->scope_chain = scope_chain;
    }

    parser_addref(ctx);
    function->parser = ctx;

    for(iter = parameters; iter; iter = iter->next)
        length++;
    function->length = length;

    function->src_str = src_str;
    function->src_len = src_len;

    *ret = &function->dispex;
    return S_OK;
}

HRESULT init_function_constr(script_ctx_t *ctx)
{
    FunctionInstance *prot, *constr;
    HRESULT hres;

    hres = create_function(ctx, PROPF_CONSTR, TRUE, NULL, &prot);
    if(FAILED(hres))
        return hres;

    prot->value_proc = FunctionProt_value;

    hres = create_function(ctx, PROPF_CONSTR, TRUE, &prot->dispex, &constr);
    jsdisp_release(&prot->dispex);
    if(FAILED(hres))
        return hres;

    constr->value_proc = FunctionConstr_value;
    ctx->function_constr = &constr->dispex;
    return hres;
}
