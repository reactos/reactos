/*
 * Copyright 2010 Piotr Caban for CodeWeavers
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
    jsdisp_t dispex;

    SAFEARRAY *safearray;
} VBArrayInstance;

static inline VBArrayInstance *vbarray_from_jsdisp(jsdisp_t *jsdisp)
{
    return CONTAINING_RECORD(jsdisp, VBArrayInstance, dispex);
}

static inline VBArrayInstance *vbarray_this(jsval_t vthis)
{
    jsdisp_t *jsdisp = is_object_instance(vthis) ? to_jsdisp(get_object(vthis)) : NULL;
    return (jsdisp && is_class(jsdisp, JSCLASS_VBARRAY)) ? vbarray_from_jsdisp(jsdisp) : NULL;
}

static HRESULT VBArray_dimensions(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    VBArrayInstance *vbarray;

    TRACE("\n");

    vbarray = vbarray_this(vthis);
    if(!vbarray)
        return JS_E_VBARRAY_EXPECTED;

    if(r)
        *r = jsval_number(SafeArrayGetDim(vbarray->safearray));
    return S_OK;
}

static HRESULT VBArray_getItem(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    VBArrayInstance *vbarray;
    unsigned i;
    LONG *indexes;
    VARIANT out;
    HRESULT hres;

    TRACE("\n");

    vbarray = vbarray_this(vthis);
    if(!vbarray)
        return JS_E_VBARRAY_EXPECTED;

    if(argc < SafeArrayGetDim(vbarray->safearray))
        return JS_E_SUBSCRIPT_OUT_OF_RANGE;

    indexes = malloc(sizeof(indexes[0])*argc);
    if(!indexes)
        return E_OUTOFMEMORY;

    for(i=0; i<argc; i++) {
        hres = to_long(ctx, argv[i], indexes + i);
        if(FAILED(hres)) {
            free(indexes);
            return hres;
        }
    }

    hres = SafeArrayGetElement(vbarray->safearray, indexes, (void*)&out);
    free(indexes);
    if(hres == DISP_E_BADINDEX)
        return JS_E_SUBSCRIPT_OUT_OF_RANGE;
    else if(FAILED(hres))
        return hres;

    if(r) {
        hres = variant_to_jsval(ctx, &out, r);
        VariantClear(&out);
    }
    return hres;
}

static HRESULT VBArray_lbound(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    VBArrayInstance *vbarray;
    LONG dim;
    HRESULT hres;

    TRACE("\n");

    vbarray = vbarray_this(vthis);
    if(!vbarray)
        return JS_E_VBARRAY_EXPECTED;

    if(argc) {
        hres = to_long(ctx, argv[0], &dim);
        if(FAILED(hres))
            return hres;
    } else
        dim = 1;

    hres = SafeArrayGetLBound(vbarray->safearray, dim, &dim);
    if(hres == DISP_E_BADINDEX)
        return JS_E_SUBSCRIPT_OUT_OF_RANGE;
    else if(FAILED(hres))
        return hres;

    if(r)
        *r = jsval_number(dim);
    return S_OK;
}

static HRESULT VBArray_toArray(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    VBArrayInstance *vbarray;
    jsdisp_t *array;
    jsval_t val;
    VARIANT *v;
    LONG i, size = 1, ubound, lbound;
    HRESULT hres;

    TRACE("\n");

    vbarray = vbarray_this(vthis);
    if(!vbarray)
        return JS_E_VBARRAY_EXPECTED;

    for(i=1; i<=SafeArrayGetDim(vbarray->safearray); i++) {
        SafeArrayGetLBound(vbarray->safearray, i, &lbound);
        SafeArrayGetUBound(vbarray->safearray, i, &ubound);
        size *= ubound-lbound+1;
    }

    hres = SafeArrayAccessData(vbarray->safearray, (void**)&v);
    if(FAILED(hres))
        return hres;

    hres = create_array(ctx, 0, &array);
    if(FAILED(hres)) {
        SafeArrayUnaccessData(vbarray->safearray);
        return hres;
    }

    for(i=0; i<size; i++) {
        hres = variant_to_jsval(ctx, v, &val);
        if(SUCCEEDED(hres)) {
            hres = jsdisp_propput_idx(array, i, val);
            jsval_release(val);
        }
        if(FAILED(hres)) {
            SafeArrayUnaccessData(vbarray->safearray);
            jsdisp_release(array);
            return hres;
        }
        v++;
    }

    SafeArrayUnaccessData(vbarray->safearray);

    if(r)
        *r = jsval_obj(array);
    else
        jsdisp_release(array);
    return S_OK;
}

static HRESULT VBArray_ubound(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    VBArrayInstance *vbarray;
    LONG dim;
    HRESULT hres;

    TRACE("\n");

    vbarray = vbarray_this(vthis);
    if(!vbarray)
        return JS_E_VBARRAY_EXPECTED;

    if(argc) {
        hres = to_long(ctx, argv[0], &dim);
        if(FAILED(hres))
            return hres;
    } else
        dim = 1;

    hres = SafeArrayGetUBound(vbarray->safearray, dim, &dim);
    if(hres == DISP_E_BADINDEX)
        return JS_E_SUBSCRIPT_OUT_OF_RANGE;
    else if(FAILED(hres))
        return hres;

    if(r)
        *r = jsval_number(dim);
    return S_OK;
}

static HRESULT VBArray_value(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    FIXME("\n");

    switch(flags) {
        default:
            FIXME("unimplemented flags %x\n", flags);
            return E_NOTIMPL;
    }

    return S_OK;
}

static void VBArray_destructor(jsdisp_t *dispex)
{
    VBArrayInstance *vbarray = vbarray_from_jsdisp(dispex);

    SafeArrayDestroy(vbarray->safearray);
}

static const builtin_prop_t VBArray_props[] = {
    {L"dimensions",         VBArray_dimensions,         PROPF_METHOD},
    {L"getItem",            VBArray_getItem,            PROPF_METHOD|1},
    {L"lbound",             VBArray_lbound,             PROPF_METHOD},
    {L"toArray",            VBArray_toArray,            PROPF_METHOD},
    {L"ubound",             VBArray_ubound,             PROPF_METHOD}
};

static const builtin_info_t VBArray_info = {
    .class      = JSCLASS_VBARRAY,
    .call       = VBArray_value,
    .props_cnt  = ARRAY_SIZE(VBArray_props),
    .props      = VBArray_props,
    .destructor = VBArray_destructor,
};

static HRESULT alloc_vbarray(script_ctx_t *ctx, jsdisp_t *object_prototype, VBArrayInstance **ret)
{
    VBArrayInstance *vbarray;
    HRESULT hres;

    vbarray = calloc(1, sizeof(VBArrayInstance));
    if(!vbarray)
        return E_OUTOFMEMORY;

    if(object_prototype)
        hres = init_dispex(&vbarray->dispex, ctx, &VBArray_info, object_prototype);
    else
        hres = init_dispex_from_constr(&vbarray->dispex, ctx, &VBArray_info, ctx->vbarray_constr);

    if(FAILED(hres)) {
        free(vbarray);
        return hres;
    }

    *ret = vbarray;
    return S_OK;
}

static HRESULT VBArrayConstr_value(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    VBArrayInstance *vbarray;
    HRESULT hres;

    TRACE("\n");

    switch(flags) {
    case DISPATCH_METHOD:
        if(argc<1 || !is_variant(argv[0]) || V_VT(get_variant(argv[0])) != (VT_ARRAY|VT_VARIANT))
            return JS_E_VBARRAY_EXPECTED;

        return r ? jsval_copy(argv[0], r) : S_OK;

    case DISPATCH_CONSTRUCT:
        if(argc<1 || !is_variant(argv[0]) || V_VT(get_variant(argv[0])) != (VT_ARRAY|VT_VARIANT))
            return JS_E_VBARRAY_EXPECTED;
        if(!r)
            return S_OK;

        hres = alloc_vbarray(ctx, NULL, &vbarray);
        if(FAILED(hres))
            return hres;

        hres = SafeArrayCopy(V_ARRAY(get_variant(argv[0])), &vbarray->safearray);
        if(FAILED(hres)) {
            jsdisp_release(&vbarray->dispex);
            return hres;
        }

        *r = jsval_obj(&vbarray->dispex);
        break;

    default:
        FIXME("unimplemented flags: %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

HRESULT create_vbarray_constr(script_ctx_t *ctx, jsdisp_t *object_prototype, jsdisp_t **ret)
{
    VBArrayInstance *vbarray;
    HRESULT hres;

    hres = alloc_vbarray(ctx, object_prototype, &vbarray);
    if(FAILED(hres))
        return hres;

    hres = create_builtin_constructor(ctx, VBArrayConstr_value, L"VBArray", NULL, PROPF_CONSTR|1, &vbarray->dispex, ret);

    jsdisp_release(&vbarray->dispex);
    return hres;
}

HRESULT create_vbarray(script_ctx_t *ctx, SAFEARRAY *sa, jsdisp_t **ret)
{
    VBArrayInstance *vbarray;
    HRESULT hres;

    hres = alloc_vbarray(ctx, NULL, &vbarray);
    if(FAILED(hres))
        return hres;

    hres = SafeArrayCopy(sa, &vbarray->safearray);
    if(FAILED(hres)) {
        jsdisp_release(&vbarray->dispex);
        return hres;
    }

    *ret = &vbarray->dispex;
    return S_OK;
}
