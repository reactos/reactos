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

static const WCHAR dimensionsW[] = {'d','i','m','e','n','s','i','o','n','s',0};
static const WCHAR getItemW[] = {'g','e','t','I','t','e','m',0};
static const WCHAR lboundW[] = {'l','b','o','u','n','d',0};
static const WCHAR toArrayW[] = {'t','o','A','r','r','a','y',0};
static const WCHAR uboundW[] = {'u','b','o','u','n','d',0};

static inline VBArrayInstance *vbarray_from_jsdisp(jsdisp_t *jsdisp)
{
    return CONTAINING_RECORD(jsdisp, VBArrayInstance, dispex);
}

static inline VBArrayInstance *vbarray_from_vdisp(vdisp_t *vdisp)
{
    return vbarray_from_jsdisp(vdisp->u.jsdisp);
}

static inline VBArrayInstance *vbarray_this(vdisp_t *jsthis)
{
    return is_vclass(jsthis, JSCLASS_VBARRAY) ? vbarray_from_vdisp(jsthis) : NULL;
}

static HRESULT VBArray_dimensions(script_ctx_t *ctx, vdisp_t *vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    VBArrayInstance *vbarray;

    TRACE("\n");

    vbarray = vbarray_this(vthis);
    if(!vbarray)
        return throw_type_error(ctx, JS_E_VBARRAY_EXPECTED, NULL);

    if(r)
        *r = jsval_number(SafeArrayGetDim(vbarray->safearray));
    return S_OK;
}

static HRESULT VBArray_getItem(script_ctx_t *ctx, vdisp_t *vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    VBArrayInstance *vbarray;
    int i, *indexes;
    VARIANT out;
    HRESULT hres;

    TRACE("\n");

    vbarray = vbarray_this(vthis);
    if(!vbarray)
        return throw_type_error(ctx, JS_E_VBARRAY_EXPECTED, NULL);

    if(argc < SafeArrayGetDim(vbarray->safearray))
        return throw_range_error(ctx, JS_E_SUBSCRIPT_OUT_OF_RANGE, NULL);

    indexes = heap_alloc(sizeof(int)*argc);
    if(!indexes)
        return E_OUTOFMEMORY;

    for(i=0; i<argc; i++) {
        hres = to_int32(ctx, argv[i], indexes+i);
        if(FAILED(hres)) {
            heap_free(indexes);
            return hres;
        }
    }

    hres = SafeArrayGetElement(vbarray->safearray, indexes, (void*)&out);
    heap_free(indexes);
    if(hres == DISP_E_BADINDEX)
        return throw_range_error(ctx, JS_E_SUBSCRIPT_OUT_OF_RANGE, NULL);
    else if(FAILED(hres))
        return hres;

    if(r) {
        hres = variant_to_jsval(&out, r);
        VariantClear(&out);
    }
    return hres;
}

static HRESULT VBArray_lbound(script_ctx_t *ctx, vdisp_t *vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    VBArrayInstance *vbarray;
    int dim;
    HRESULT hres;

    TRACE("\n");

    vbarray = vbarray_this(vthis);
    if(!vbarray)
        return throw_type_error(ctx, JS_E_VBARRAY_EXPECTED, NULL);

    if(argc) {
        hres = to_int32(ctx, argv[0], &dim);
        if(FAILED(hres))
            return hres;
    } else
        dim = 1;

    hres = SafeArrayGetLBound(vbarray->safearray, dim, &dim);
    if(hres == DISP_E_BADINDEX)
        return throw_range_error(ctx, JS_E_SUBSCRIPT_OUT_OF_RANGE, NULL);
    else if(FAILED(hres))
        return hres;

    if(r)
        *r = jsval_number(dim);
    return S_OK;
}

static HRESULT VBArray_toArray(script_ctx_t *ctx, vdisp_t *vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    VBArrayInstance *vbarray;
    jsdisp_t *array;
    jsval_t val;
    VARIANT *v;
    int i, size = 1, ubound, lbound;
    HRESULT hres;

    TRACE("\n");

    vbarray = vbarray_this(vthis);
    if(!vbarray)
        return throw_type_error(ctx, JS_E_VBARRAY_EXPECTED, NULL);

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
        hres = variant_to_jsval(v, &val);
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

static HRESULT VBArray_ubound(script_ctx_t *ctx, vdisp_t *vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    VBArrayInstance *vbarray;
    int dim;
    HRESULT hres;

    TRACE("\n");

    vbarray = vbarray_this(vthis);
    if(!vbarray)
        return throw_type_error(ctx, JS_E_VBARRAY_EXPECTED, NULL);

    if(argc) {
        hres = to_int32(ctx, argv[0], &dim);
        if(FAILED(hres))
            return hres;
    } else
        dim = 1;

    hres = SafeArrayGetUBound(vbarray->safearray, dim, &dim);
    if(hres == DISP_E_BADINDEX)
        return throw_range_error(ctx, JS_E_SUBSCRIPT_OUT_OF_RANGE, NULL);
    else if(FAILED(hres))
        return hres;

    if(r)
        *r = jsval_number(dim);
    return S_OK;
}

static HRESULT VBArray_value(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
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
    heap_free(vbarray);
}

static const builtin_prop_t VBArray_props[] = {
    {dimensionsW,           VBArray_dimensions,         PROPF_METHOD},
    {getItemW,              VBArray_getItem,            PROPF_METHOD|1},
    {lboundW,               VBArray_lbound,             PROPF_METHOD},
    {toArrayW,              VBArray_toArray,            PROPF_METHOD},
    {uboundW,               VBArray_ubound,             PROPF_METHOD}
};

static const builtin_info_t VBArray_info = {
    JSCLASS_VBARRAY,
    {NULL, VBArray_value, 0},
    ARRAY_SIZE(VBArray_props),
    VBArray_props,
    VBArray_destructor,
    NULL
};

static HRESULT alloc_vbarray(script_ctx_t *ctx, jsdisp_t *object_prototype, VBArrayInstance **ret)
{
    VBArrayInstance *vbarray;
    HRESULT hres;

    vbarray = heap_alloc_zero(sizeof(VBArrayInstance));
    if(!vbarray)
        return E_OUTOFMEMORY;

    if(object_prototype)
        hres = init_dispex(&vbarray->dispex, ctx, &VBArray_info, object_prototype);
    else
        hres = init_dispex_from_constr(&vbarray->dispex, ctx, &VBArray_info, ctx->vbarray_constr);

    if(FAILED(hres)) {
        heap_free(vbarray);
        return hres;
    }

    *ret = vbarray;
    return S_OK;
}

static HRESULT VBArrayConstr_value(script_ctx_t *ctx, vdisp_t *vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    VBArrayInstance *vbarray;
    HRESULT hres;

    TRACE("\n");

    switch(flags) {
    case DISPATCH_METHOD:
        if(argc<1 || !is_variant(argv[0]) || V_VT(get_variant(argv[0])) != (VT_ARRAY|VT_VARIANT))
            return throw_type_error(ctx, JS_E_VBARRAY_EXPECTED, NULL);

        return jsval_copy(argv[0], r);

    case DISPATCH_CONSTRUCT:
        if(argc<1 || !is_variant(argv[0]) || V_VT(get_variant(argv[0])) != (VT_ARRAY|VT_VARIANT))
            return throw_type_error(ctx, JS_E_VBARRAY_EXPECTED, NULL);

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

    static const WCHAR VBArrayW[] = {'V','B','A','r','r','a','y',0};

    hres = alloc_vbarray(ctx, object_prototype, &vbarray);
    if(FAILED(hres))
        return hres;

    hres = create_builtin_constructor(ctx, VBArrayConstr_value, VBArrayW, NULL, PROPF_CONSTR|1, &vbarray->dispex, ret);

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
