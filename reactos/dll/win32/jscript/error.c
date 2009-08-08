/*
 * Copyright 2009 Piotr Caban
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

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

typedef struct {
    DispatchEx dispex;

    VARIANT number;
    VARIANT description;
    VARIANT message;
} ErrorInstance;

static const WCHAR descriptionW[] = {'d','e','s','c','r','i','p','t','i','o','n',0};
static const WCHAR messageW[] = {'m','e','s','s','a','g','e',0};
static const WCHAR numberW[] = {'n','u','m','b','e','r',0};
static const WCHAR toStringW[] = {'t','o','S','t','r','i','n','g',0};
static const WCHAR hasOwnPropertyW[] = {'h','a','s','O','w','n','P','r','o','p','e','r','t','y',0};
static const WCHAR propertyIsEnumerableW[] =
    {'p','r','o','p','e','r','t','y','I','s','E','n','u','m','e','r','a','b','l','e',0};
static const WCHAR isPrototypeOfW[] = {'i','s','P','r','o','t','o','t','y','p','e','O','f',0};

static HRESULT Error_number(DispatchEx *dispex, LCID lcid, WORD flags,
        DISPPARAMS *dp, VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    ErrorInstance *This = (ErrorInstance*)dispex;

    TRACE("\n");

    switch(flags) {
    case DISPATCH_PROPERTYGET:
        return VariantCopy(retv, &This->number);
    case DISPATCH_PROPERTYPUT:
        return VariantCopy(&This->number, get_arg(dp, 0));
    default:
        FIXME("unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }
}

static HRESULT Error_description(DispatchEx *dispex, LCID lcid, WORD flags,
        DISPPARAMS *dp, VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    ErrorInstance *This = (ErrorInstance*)dispex;

    TRACE("\n");

    switch(flags) {
    case DISPATCH_PROPERTYGET:
        return VariantCopy(retv, &This->description);
    case DISPATCH_PROPERTYPUT:
        return VariantCopy(&This->description, get_arg(dp, 0));
    default:
        FIXME("unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }
}

/* ECMA-262 3rd Edition    15.11.4.3 */
static HRESULT Error_message(DispatchEx *dispex, LCID lcid, WORD flags,
        DISPPARAMS *dp, VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    ErrorInstance *This = (ErrorInstance*)dispex;

    TRACE("\n");

    switch(flags) {
    case DISPATCH_PROPERTYGET:
        return VariantCopy(retv, &This->message);
    case DISPATCH_PROPERTYPUT:
        return VariantCopy(&This->message, get_arg(dp, 0));
    default:
        FIXME("unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }
}

/* ECMA-262 3rd Edition    15.11.4.4 */
static HRESULT Error_toString(DispatchEx *dispex, LCID lcid, WORD flags,
        DISPPARAMS *dp, VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    static const WCHAR str[] = {'[','o','b','j','e','c','t',' ','E','r','r','o','r',']',0};

    TRACE("\n");

    if(retv) {
        V_VT(retv) = VT_BSTR;
        V_BSTR(retv) = SysAllocString(str);
        if(!V_BSTR(retv))
            return E_OUTOFMEMORY;
    }

    return S_OK;
}

static HRESULT Error_hasOwnProperty(DispatchEx *dispex, LCID lcid, WORD flags,
        DISPPARAMS *dp, VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Error_propertyIsEnumerable(DispatchEx *dispex, LCID lcid, WORD flags,
        DISPPARAMS *dp, VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}


static HRESULT Error_isPrototypeOf(DispatchEx *dispex, LCID lcid, WORD flags,
        DISPPARAMS *dp, VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Error_value(DispatchEx *dispex, LCID lcid, WORD flags,
        DISPPARAMS *dp, VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    TRACE("\n");

    switch(flags) {
    case INVOKE_FUNC:
        return throw_type_error(dispex->ctx, ei, IDS_NOT_FUNC, NULL);
    default:
        FIXME("unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

static void Error_destructor(DispatchEx *dispex)
{
    ErrorInstance *This = (ErrorInstance*)dispex;

    VariantClear(&This->number);
    VariantClear(&This->description);
    VariantClear(&This->message);
    heap_free(This);
}

static const builtin_prop_t Error_props[] = {
    {descriptionW,              Error_description,                  0},
    {hasOwnPropertyW,           Error_hasOwnProperty,               PROPF_METHOD},
    {isPrototypeOfW,            Error_isPrototypeOf,                PROPF_METHOD},
    {messageW,                  Error_message,                      0},
    {numberW,                   Error_number,                       0},
    {propertyIsEnumerableW,     Error_propertyIsEnumerable,         PROPF_METHOD},
    {toStringW,                 Error_toString,                     PROPF_METHOD}
};

static const builtin_info_t Error_info = {
    JSCLASS_ERROR,
    {NULL, Error_value, 0},
    sizeof(Error_props)/sizeof(*Error_props),
    Error_props,
    Error_destructor,
    NULL
};

static const builtin_prop_t ErrorInst_props[] = {
    {descriptionW,              Error_description,                  0},
    {hasOwnPropertyW,           Error_hasOwnProperty,               PROPF_METHOD},
    {isPrototypeOfW,            Error_isPrototypeOf,                PROPF_METHOD},
    {messageW,                  Error_message,                      0},
    {numberW,                   Error_number,                       0},
    {propertyIsEnumerableW,     Error_propertyIsEnumerable,         PROPF_METHOD}
};

static const builtin_info_t ErrorInst_info = {
    JSCLASS_ERROR,
    {NULL, Error_value, 0},
    sizeof(ErrorInst_props)/sizeof(*ErrorInst_props),
    ErrorInst_props,
    Error_destructor,
    NULL
};

static HRESULT alloc_error(script_ctx_t *ctx, BOOL error_prototype,
        DispatchEx *constr, ErrorInstance **ret)
{
    ErrorInstance *err;
    DispatchEx *inherit;
    HRESULT hres;

    err = heap_alloc_zero(sizeof(ErrorInstance));
    if(!err)
        return E_OUTOFMEMORY;

    inherit = error_prototype ? ctx->object_constr : ctx->error_constr;
    hres = init_dispex_from_constr(&err->dispex, ctx,
            error_prototype ? &Error_info : &ErrorInst_info,
            constr ? constr : inherit);
    if(FAILED(hres)) {
        heap_free(err);
        return hres;
    }

    *ret = err;
    return S_OK;
}

static HRESULT create_error(script_ctx_t *ctx, DispatchEx *constr,
        const UINT *number, const WCHAR *msg, DispatchEx **ret)
{
    ErrorInstance *err;
    HRESULT hres;

    hres = alloc_error(ctx, FALSE, constr, &err);
    if(FAILED(hres))
        return hres;

    if(number) {
        V_VT(&err->number) = VT_I4;
        V_I4(&err->number) = *number;
    }

    V_VT(&err->message) = VT_BSTR;
    if(msg) V_BSTR(&err->message) = SysAllocString(msg);
    else V_BSTR(&err->message) = SysAllocStringLen(NULL, 0);

    VariantCopy(&err->description, &err->message);

    if(!V_BSTR(&err->message)) {
        heap_free(err);
        return E_OUTOFMEMORY;
    }

    *ret = &err->dispex;
    return S_OK;
}

static HRESULT error_constr(DispatchEx *dispex, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, DispatchEx *constr) {
    DispatchEx *err;
    VARIANT numv;
    UINT num;
    BSTR msg = NULL;
    HRESULT hres;

    V_VT(&numv) = VT_NULL;

    if(arg_cnt(dp)) {
        hres = to_number(dispex->ctx, get_arg(dp, 0), ei, &numv);
        if(FAILED(hres) || (V_VT(&numv)==VT_R8 && isnan(V_R8(&numv))))
            hres = to_string(dispex->ctx, get_arg(dp, 0), ei, &msg);
        else if(V_VT(&numv) == VT_I4)
            num = V_I4(&numv);
        else
            num = V_R8(&numv);

        if(FAILED(hres))
            return hres;
    }

    if(arg_cnt(dp)>1 && !msg) {
        hres = to_string(dispex->ctx, get_arg(dp, 1), ei, &msg);
        if(FAILED(hres))
            return hres;
    }

    switch(flags) {
    case INVOKE_FUNC:
    case DISPATCH_CONSTRUCT:
        if(V_VT(&numv) == VT_NULL)
            hres = create_error(dispex->ctx, constr, NULL, msg, &err);
        else
            hres = create_error(dispex->ctx, constr, &num, msg, &err);

        if(FAILED(hres))
            return hres;

        if(retv) {
            V_VT(retv) = VT_DISPATCH;
            V_DISPATCH(retv) = (IDispatch*)_IDispatchEx_(err);
        }
        else
            IDispatchEx_Release(_IDispatchEx_(err));

        return S_OK;

    default:
        FIXME("unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }
}

static HRESULT ErrorConstr_value(DispatchEx *dispex, LCID lcid, WORD flags,
        DISPPARAMS *dp, VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    TRACE("\n");
    return error_constr(dispex, flags, dp, retv, ei,
            dispex->ctx->error_constr);
}

static HRESULT EvalErrorConstr_value(DispatchEx *dispex, LCID lcid, WORD flags,
        DISPPARAMS *dp, VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    TRACE("\n");
    return error_constr(dispex, flags, dp, retv, ei,
            dispex->ctx->eval_error_constr);
}

static HRESULT RangeErrorConstr_value(DispatchEx *dispex, LCID lcid, WORD flags,
        DISPPARAMS *dp, VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    TRACE("\n");
    return error_constr(dispex, flags, dp, retv, ei,
            dispex->ctx->range_error_constr);
}

static HRESULT ReferenceErrorConstr_value(DispatchEx *dispex, LCID lcid, WORD flags,
        DISPPARAMS *dp, VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    TRACE("\n");
    return error_constr(dispex, flags, dp, retv, ei,
            dispex->ctx->reference_error_constr);
}

static HRESULT SyntaxErrorConstr_value(DispatchEx *dispex, LCID lcid, WORD flags,
        DISPPARAMS *dp, VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    TRACE("\n");
    return error_constr(dispex, flags, dp, retv, ei,
            dispex->ctx->syntax_error_constr);
}

static HRESULT TypeErrorConstr_value(DispatchEx *dispex, LCID lcid, WORD flags,
        DISPPARAMS *dp, VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    TRACE("\n");
    return error_constr(dispex, flags, dp, retv, ei,
            dispex->ctx->type_error_constr);
}

static HRESULT URIErrorConstr_value(DispatchEx *dispex, LCID lcid, WORD flags,
        DISPPARAMS *dp, VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    TRACE("\n");
    return error_constr(dispex, flags, dp, retv, ei,
            dispex->ctx->uri_error_constr);
}

HRESULT init_error_constr(script_ctx_t *ctx)
{
    static const WCHAR nameW[] = {'n','a','m','e',0};
    static const WCHAR ErrorW[] = {'E','r','r','o','r',0};
    static const WCHAR EvalErrorW[] = {'E','v','a','l','E','r','r','o','r',0};
    static const WCHAR RangeErrorW[] = {'R','a','n','g','e','E','r','r','o','r',0};
    static const WCHAR ReferenceErrorW[] = {'R','e','f','e','r','e','n','c','e','E','r','r','o','r',0};
    static const WCHAR SyntaxErrorW[] = {'S','y','n','t','a','x','E','r','r','o','r',0};
    static const WCHAR TypeErrorW[] = {'T','y','p','e','E','r','r','o','r',0};
    static const WCHAR URIErrorW[] = {'U','R','I','E','r','r','o','r',0};
    static const WCHAR *names[] = {ErrorW, EvalErrorW, RangeErrorW,
        ReferenceErrorW, SyntaxErrorW, TypeErrorW, URIErrorW};
    DispatchEx **constr_addr[] = {&ctx->error_constr, &ctx->eval_error_constr,
        &ctx->range_error_constr, &ctx->reference_error_constr,
        &ctx->syntax_error_constr, &ctx->type_error_constr,
        &ctx->uri_error_constr};
    static builtin_invoke_t constr_val[] = {ErrorConstr_value, EvalErrorConstr_value,
        RangeErrorConstr_value, ReferenceErrorConstr_value, SyntaxErrorConstr_value,
        TypeErrorConstr_value, URIErrorConstr_value};

    ErrorInstance *err;
    INT i;
    VARIANT v;
    HRESULT hres;

    for(i=0; i<7; i++) {
        hres = alloc_error(ctx, i==0, NULL, &err);
        if(FAILED(hres))
            return hres;

        V_VT(&v) = VT_BSTR;
        V_BSTR(&v) = SysAllocString(names[i]);
        if(!V_BSTR(&v)) {
            IDispatchEx_Release(_IDispatchEx_(&err->dispex));
            return E_OUTOFMEMORY;
        }

        hres = jsdisp_propput_name(&err->dispex, nameW, ctx->lcid, &v, NULL/*FIXME*/, NULL/*FIXME*/);

        if(SUCCEEDED(hres))
            hres = create_builtin_function(ctx, constr_val[i], NULL,
                    PROPF_CONSTR, &err->dispex, constr_addr[i]);

        IDispatchEx_Release(_IDispatchEx_(&err->dispex));
        VariantClear(&v);
        if(FAILED(hres))
            return hres;
    }

    return S_OK;
}

static HRESULT throw_error(script_ctx_t *ctx, jsexcept_t *ei, UINT id, const WCHAR *str, DispatchEx *constr)
{
    WCHAR buf[1024], *pos = NULL;
    DispatchEx *err;
    HRESULT hres;

    buf[0] = '\0';
    LoadStringW(jscript_hinstance, id&0xFFFF,  buf, sizeof(buf)/sizeof(WCHAR));

    if(str) pos = strchrW(buf, '|');
    if(pos) {
        int len = strlenW(str);
        memmove(pos+len, pos+1, (strlenW(pos+1)+1)*sizeof(WCHAR));
        memcpy(pos, str, len*sizeof(WCHAR));
    }

    WARN("%s\n", debugstr_w(buf));

    id |= JSCRIPT_ERROR;
    hres = create_error(ctx, constr, &id, buf, &err);
    if(FAILED(hres))
        return hres;

    if(!ei)
        return id;

    V_VT(&ei->var) = VT_DISPATCH;
    V_DISPATCH(&ei->var) = (IDispatch*)_IDispatchEx_(err);

    return id;
}

HRESULT throw_eval_error(script_ctx_t *ctx, jsexcept_t *ei, UINT id, const WCHAR *str)
{
    return throw_error(ctx, ei, id, str, ctx->eval_error_constr);
}

HRESULT throw_range_error(script_ctx_t *ctx, jsexcept_t *ei, UINT id, const WCHAR *str)
{
    return throw_error(ctx, ei, id, str, ctx->range_error_constr);
}

HRESULT throw_reference_error(script_ctx_t *ctx, jsexcept_t *ei, UINT id, const WCHAR *str)
{
    return throw_error(ctx, ei, id, str, ctx->reference_error_constr);
}

HRESULT throw_syntax_error(script_ctx_t *ctx, jsexcept_t *ei, UINT id, const WCHAR *str)
{
    return throw_error(ctx, ei, id, str, ctx->syntax_error_constr);
}

HRESULT throw_type_error(script_ctx_t *ctx, jsexcept_t *ei, UINT id, const WCHAR *str)
{
    return throw_error(ctx, ei, id, str, ctx->type_error_constr);
}

HRESULT throw_uri_error(script_ctx_t *ctx, jsexcept_t *ei, UINT id, const WCHAR *str)
{
    return throw_error(ctx, ei, id, str, ctx->uri_error_constr);
}
