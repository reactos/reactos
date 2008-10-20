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

static HRESULT Bool_toString(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Bool_toLocaleString(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Bool_valueOf(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
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
    FIXME("\n");
    return E_NOTIMPL;
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
    FIXME("\n");
    return E_NOTIMPL;
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

    hres = create_builtin_function(ctx, BoolConstr_value, PROPF_CONSTR, &bool->dispex, ret);

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
