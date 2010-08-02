/*
 * Copyright 2008 Jacek Caban for CodeWeavers
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

#include "jscript.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

typedef struct {
    DispatchEx dispex;

    VARIANT_BOOL val;
} BoolInstance;

static const WCHAR toStringW[] = {'t','o','S','t','r','i','n','g',0};
static const WCHAR valueOfW[] = {'v','a','l','u','e','O','f',0};

static inline BoolInstance *bool_this(vdisp_t *jsthis)
{
    return is_vclass(jsthis, JSCLASS_BOOLEAN) ? (BoolInstance*)jsthis->u.jsdisp : NULL;
}

/* ECMA-262 3rd Edition    15.6.4.2 */
static HRESULT Bool_toString(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    BoolInstance *bool;

    static const WCHAR trueW[] = {'t','r','u','e',0};
    static const WCHAR falseW[] = {'f','a','l','s','e',0};

    TRACE("\n");

    if(!(bool = bool_this(jsthis)))
        return throw_type_error(ctx, ei, IDS_NOT_BOOL, NULL);

    if(retv) {
        BSTR val;

        if(bool->val) val = SysAllocString(trueW);
        else val = SysAllocString(falseW);

        if(!val)
            return E_OUTOFMEMORY;

        V_VT(retv) = VT_BSTR;
        V_BSTR(retv) = val;
    }

    return S_OK;
}

/* ECMA-262 3rd Edition    15.6.4.3 */
static HRESULT Bool_valueOf(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    BoolInstance *bool;

    TRACE("\n");

    if(!(bool = bool_this(jsthis)))
        return throw_type_error(ctx, ei, IDS_NOT_BOOL, NULL);

    if(retv) {
        V_VT(retv) = VT_BOOL;
        V_BOOL(retv) = bool->val;
    }

    return S_OK;
}

static HRESULT Bool_value(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    TRACE("\n");

    switch(flags) {
    case INVOKE_FUNC:
        return throw_type_error(ctx, ei, IDS_NOT_FUNC, NULL);
    default:
        FIXME("unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;

}

static const builtin_prop_t Bool_props[] = {
    {toStringW,              Bool_toString,             PROPF_METHOD},
    {valueOfW,               Bool_valueOf,              PROPF_METHOD}
};

static const builtin_info_t Bool_info = {
    JSCLASS_BOOLEAN,
    {NULL, Bool_value, 0},
    sizeof(Bool_props)/sizeof(*Bool_props),
    Bool_props,
    NULL,
    NULL
};

static HRESULT BoolConstr_value(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    HRESULT hres;
    VARIANT_BOOL value = VARIANT_FALSE;

    if(arg_cnt(dp)) {
        hres = to_boolean(get_arg(dp,0), &value);
        if(FAILED(hres))
            return hres;
    }

    switch(flags) {
    case DISPATCH_CONSTRUCT: {
        DispatchEx *bool;

        hres = create_bool(ctx, value, &bool);
        if(FAILED(hres))
            return hres;

        V_VT(retv) = VT_DISPATCH;
        V_DISPATCH(retv) = (IDispatch*)_IDispatchEx_(bool);
        return S_OK;
    }

    case INVOKE_FUNC:
        if(retv) {
            V_VT(retv) = VT_BOOL;
            V_BOOL(retv) = value;
        }
        return S_OK;

    default:
        FIXME("unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT alloc_bool(script_ctx_t *ctx, DispatchEx *object_prototype, BoolInstance **ret)
{
    BoolInstance *bool;
    HRESULT hres;

    bool = heap_alloc_zero(sizeof(BoolInstance));
    if(!bool)
        return E_OUTOFMEMORY;

    if(object_prototype)
        hres = init_dispex(&bool->dispex, ctx, &Bool_info, object_prototype);
    else
        hres = init_dispex_from_constr(&bool->dispex, ctx, &Bool_info, ctx->bool_constr);

    if(FAILED(hres)) {
        heap_free(bool);
        return hres;
    }

    *ret = bool;
    return S_OK;
}

HRESULT create_bool_constr(script_ctx_t *ctx, DispatchEx *object_prototype, DispatchEx **ret)
{
    BoolInstance *bool;
    HRESULT hres;

    static const WCHAR BooleanW[] = {'B','o','o','l','e','a','n',0};

    hres = alloc_bool(ctx, object_prototype, &bool);
    if(FAILED(hres))
        return hres;

    hres = create_builtin_function(ctx, BoolConstr_value, BooleanW, NULL,
            PROPF_CONSTR|1, &bool->dispex, ret);

    jsdisp_release(&bool->dispex);
    return hres;
}

HRESULT create_bool(script_ctx_t *ctx, VARIANT_BOOL b, DispatchEx **ret)
{
    BoolInstance *bool;
    HRESULT hres;

    hres = alloc_bool(ctx, NULL, &bool);
    if(FAILED(hres))
        return hres;

    bool->val = b;

    *ret = &bool->dispex;
    return S_OK;
}
