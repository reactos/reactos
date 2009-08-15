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
static const WCHAR toLocaleStringW[] = {'t','o','L','o','c','a','l','e','S','t','r','i','n','g',0};
static const WCHAR valueOfW[] = {'v','a','l','u','e','O','f',0};
static const WCHAR hasOwnPropertyW[] = {'h','a','s','O','w','n','P','r','o','p','e','r','t','y',0};
static const WCHAR propertyIsEnumerableW[] =
    {'p','r','o','p','e','r','t','y','I','s','E','n','u','m','e','r','a','b','l','e',0};
static const WCHAR isPrototypeOfW[] = {'i','s','P','r','o','t','o','t','y','p','e','O','f',0};

/* ECMA-262 3rd Edition    15.6.4.2 */
static HRESULT Bool_toString(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    static const WCHAR trueW[] = {'t','r','u','e',0};
    static const WCHAR falseW[] = {'f','a','l','s','e',0};

    TRACE("\n");

    if(!is_class(dispex, JSCLASS_BOOLEAN))
        return throw_type_error(dispex->ctx, ei, IDS_NOT_BOOL, NULL);

    if(retv) {
        BoolInstance *bool = (BoolInstance*)dispex;
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

static HRESULT Bool_toLocaleString(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    TRACE("\n");
    return Bool_toString(dispex, lcid, flags, dp, retv, ei, sp);
}

/* ECMA-262 3rd Edition    15.6.4.3 */
static HRESULT Bool_valueOf(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    TRACE("\n");

    if(!is_class(dispex, JSCLASS_BOOLEAN))
        return throw_type_error(dispex->ctx, ei, IDS_NOT_BOOL, NULL);

    if(retv) {
        BoolInstance *bool = (BoolInstance*)dispex;

        V_VT(retv) = VT_BOOL;
        V_BOOL(retv) = bool->val;
    }

    return S_OK;
}

static HRESULT Bool_hasOwnProperty(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Bool_propertyIsEnumerable(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Bool_isPrototypeOf(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Bool_value(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
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

static const builtin_prop_t Bool_props[] = {
    {hasOwnPropertyW,        Bool_hasOwnProperty,       PROPF_METHOD},
    {isPrototypeOfW,         Bool_isPrototypeOf,        PROPF_METHOD},
    {propertyIsEnumerableW,  Bool_propertyIsEnumerable, PROPF_METHOD},
    {toLocaleStringW,        Bool_toLocaleString,       PROPF_METHOD},
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

static HRESULT BoolConstr_value(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
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

        hres = create_bool(dispex->ctx, value, &bool);
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

static HRESULT alloc_bool(script_ctx_t *ctx, BOOL use_constr, BoolInstance **ret)
{
    BoolInstance *bool;
    HRESULT hres;

    bool = heap_alloc_zero(sizeof(BoolInstance));
    if(!bool)
        return E_OUTOFMEMORY;

    if(use_constr)
        hres = init_dispex_from_constr(&bool->dispex, ctx, &Bool_info, ctx->bool_constr);
    else
        hres = init_dispex(&bool->dispex, ctx, &Bool_info, NULL);

    if(FAILED(hres)) {
        heap_free(bool);
        return hres;
    }

    *ret = bool;
    return S_OK;
}

HRESULT create_bool_constr(script_ctx_t *ctx, DispatchEx **ret)
{
    BoolInstance *bool;
    HRESULT hres;

    hres = alloc_bool(ctx, FALSE, &bool);
    if(FAILED(hres))
        return hres;

    hres = create_builtin_function(ctx, BoolConstr_value, NULL, PROPF_CONSTR, &bool->dispex, ret);

    jsdisp_release(&bool->dispex);
    return hres;
}

HRESULT create_bool(script_ctx_t *ctx, VARIANT_BOOL b, DispatchEx **ret)
{
    BoolInstance *bool;
    HRESULT hres;

    hres = alloc_bool(ctx, TRUE, &bool);
    if(FAILED(hres))
        return hres;

    bool->val = b;

    *ret = &bool->dispex;
    return S_OK;
}
