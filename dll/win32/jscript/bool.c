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

#include <assert.h>

#include "jscript.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

typedef struct {
    jsdisp_t dispex;

    BOOL val;
} BoolInstance;

static inline BoolInstance *bool_from_jsdisp(jsdisp_t *jsdisp)
{
    return CONTAINING_RECORD(jsdisp, BoolInstance, dispex);
}

static inline HRESULT boolval_this(jsval_t vthis, BOOL *ret)
{
    jsdisp_t *jsdisp;
    if(is_bool(vthis))
        *ret = get_bool(vthis);
    else if(is_object_instance(vthis) && (jsdisp = to_jsdisp(get_object(vthis))) && is_class(jsdisp, JSCLASS_BOOLEAN))
        *ret = bool_from_jsdisp(jsdisp)->val;
    else
        return JS_E_BOOLEAN_EXPECTED;
    return S_OK;
}

BOOL bool_obj_value(jsdisp_t *obj)
{
    assert(is_class(obj, JSCLASS_BOOLEAN));
    return bool_from_jsdisp(obj)->val;
}

/* ECMA-262 3rd Edition    15.6.4.2 */
static HRESULT Bool_toString(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv, jsval_t *r)
{
    BOOL boolval;
    HRESULT hres;

    TRACE("\n");

    hres = boolval_this(vthis, &boolval);
    if(FAILED(hres))
        return hres;

    if(r) {
        jsstr_t *val;

        val = jsstr_alloc(boolval ? L"true" : L"false");
        if(!val)
            return E_OUTOFMEMORY;

        *r = jsval_string(val);
    }

    return S_OK;
}

/* ECMA-262 3rd Edition    15.6.4.3 */
static HRESULT Bool_valueOf(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv, jsval_t *r)
{
    BOOL boolval;
    HRESULT hres;

    TRACE("\n");

    hres = boolval_this(vthis, &boolval);
    if(FAILED(hres))
        return hres;

    if(r)
        *r = jsval_bool(boolval);
    return S_OK;
}

static HRESULT Bool_value(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    TRACE("\n");

    switch(flags) {
    case INVOKE_FUNC:
        return JS_E_FUNCTION_EXPECTED;
    default:
        FIXME("unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;

}

static const builtin_prop_t Bool_props[] = {
    {L"toString",            Bool_toString,             PROPF_METHOD},
    {L"valueOf",             Bool_valueOf,              PROPF_METHOD}
};

static const builtin_info_t Bool_info = {
    .class     = JSCLASS_BOOLEAN,
    .call      = Bool_value,
    .props_cnt = ARRAY_SIZE(Bool_props),
    .props     = Bool_props,
};

static const builtin_info_t BoolInst_info = {
    .class = JSCLASS_BOOLEAN,
    .call  = Bool_value,
};

static HRESULT BoolConstr_value(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    BOOL value = FALSE;
    HRESULT hres;

    if(argc) {
        hres = to_boolean(argv[0], &value);
        if(FAILED(hres))
            return hres;
    }

    switch(flags) {
    case DISPATCH_CONSTRUCT: {
        jsdisp_t *b;

        if(!r)
            return S_OK;

        hres = create_bool(ctx, value, &b);
        if(FAILED(hres))
            return hres;

        *r = jsval_obj(b);
        return S_OK;
    }

    case INVOKE_FUNC:
        if(r)
            *r = jsval_bool(value);
        return S_OK;

    default:
        FIXME("unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT alloc_bool(script_ctx_t *ctx, jsdisp_t *object_prototype, BoolInstance **ret)
{
    BoolInstance *b;
    HRESULT hres;

    b = calloc(1, sizeof(BoolInstance));
    if(!b)
        return E_OUTOFMEMORY;

    if(object_prototype)
        hres = init_dispex(&b->dispex, ctx, &Bool_info, object_prototype);
    else
        hres = init_dispex_from_constr(&b->dispex, ctx, &BoolInst_info, ctx->bool_constr);

    if(FAILED(hres)) {
        free(b);
        return hres;
    }

    *ret = b;
    return S_OK;
}

HRESULT create_bool_constr(script_ctx_t *ctx, jsdisp_t *object_prototype, jsdisp_t **ret)
{
    BoolInstance *b;
    HRESULT hres;

    hres = alloc_bool(ctx, object_prototype, &b);
    if(FAILED(hres))
        return hres;

    hres = create_builtin_constructor(ctx, BoolConstr_value, L"Boolean", NULL,
            PROPF_CONSTR|1, &b->dispex, ret);

    jsdisp_release(&b->dispex);
    return hres;
}

HRESULT create_bool(script_ctx_t *ctx, BOOL bval, jsdisp_t **ret)
{
    BoolInstance *b;
    HRESULT hres;

    hres = alloc_bool(ctx, NULL, &b);
    if(FAILED(hres))
        return hres;

    b->val = bval;

    *ret = &b->dispex;
    return S_OK;
}
