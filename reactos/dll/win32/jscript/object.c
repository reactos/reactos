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

static const WCHAR toStringW[] = {'t','o','S','t','r','i','n','g',0};
static const WCHAR toLocaleStringW[] = {'t','o','L','o','c','a','l','e','S','t','r','i','n','g',0};
static const WCHAR valueOfW[] = {'v','a','l','u','e','O','f',0};
static const WCHAR hasOwnPropertyW[] = {'h','a','s','O','w','n','P','r','o','p','e','r','t','y',0};
static const WCHAR propertyIsEnumerableW[] =
    {'p','r','o','p','e','r','t','y','I','s','E','n','u','m','e','r','a','b','l','e',0};
static const WCHAR isPrototypeOfW[] = {'i','s','P','r','o','t','o','t','y','p','e','O','f',0};

static const WCHAR default_valueW[] = {'[','o','b','j','e','c','t',' ','O','b','j','e','c','t',']',0};

static HRESULT Object_toString(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsdisp_t *jsdisp;
    const WCHAR *str;

    static const WCHAR formatW[] = {'[','o','b','j','e','c','t',' ','%','s',']',0};

    static const WCHAR arrayW[] = {'A','r','r','a','y',0};
    static const WCHAR booleanW[] = {'B','o','o','l','e','a','n',0};
    static const WCHAR dateW[] = {'D','a','t','e',0};
    static const WCHAR errorW[] = {'E','r','r','o','r',0};
    static const WCHAR functionW[] = {'F','u','n','c','t','i','o','n',0};
    static const WCHAR mathW[] = {'M','a','t','h',0};
    static const WCHAR numberW[] = {'N','u','m','b','e','r',0};
    static const WCHAR objectW[] = {'O','b','j','e','c','t',0};
    static const WCHAR regexpW[] = {'R','e','g','E','x','p',0};
    static const WCHAR stringW[] = {'S','t','r','i','n','g',0};
    /* Keep in sync with jsclass_t enum */
    static const WCHAR *names[] = {NULL, arrayW, booleanW, dateW, errorW,
        functionW, NULL, mathW, numberW, objectW, regexpW, stringW, objectW, objectW};

    TRACE("\n");

    jsdisp = get_jsdisp(jsthis);
    if(!jsdisp) {
        str = objectW;
    }else if(names[jsdisp->builtin_info->class]) {
        str = names[jsdisp->builtin_info->class];
    }else {
        assert(jsdisp->builtin_info->class != JSCLASS_NONE);
        FIXME("jdisp->builtin_info->class = %d\n", jsdisp->builtin_info->class);
        return E_FAIL;
    }

    if(r) {
        jsstr_t *ret;
        WCHAR *ptr;

        ptr = jsstr_alloc_buf(9+strlenW(str), &ret);
        if(!ptr)
            return E_OUTOFMEMORY;

        sprintfW(ptr, formatW, str);
        *r = jsval_string(ret);
    }

    return S_OK;
}

static HRESULT Object_toLocaleString(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    TRACE("\n");

    if(!is_jsdisp(jsthis)) {
        FIXME("Host object this\n");
        return E_FAIL;
    }

    return jsdisp_call_name(jsthis->u.jsdisp, toStringW, DISPATCH_METHOD, 0, NULL, r);
}

static HRESULT Object_valueOf(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    TRACE("\n");

    if(r) {
        IDispatch_AddRef(jsthis->u.disp);
        *r = jsval_disp(jsthis->u.disp);
    }
    return S_OK;
}

static HRESULT Object_hasOwnProperty(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsstr_t *name;
    DISPID id;
    BSTR bstr;
    HRESULT hres;

    TRACE("\n");

    if(!argc) {
        if(r)
            *r = jsval_bool(FALSE);
        return S_OK;
    }

    hres = to_string(ctx, argv[0], &name);
    if(FAILED(hres))
        return hres;

    if(is_jsdisp(jsthis)) {
        const WCHAR *name_str;
        BOOL result;

        name_str = jsstr_flatten(name);
        if(name_str)
            hres = jsdisp_is_own_prop(jsthis->u.jsdisp, name_str, &result);
        else
            hres = E_OUTOFMEMORY;
        jsstr_release(name);
        if(FAILED(hres))
            return hres;

        if(r)
            *r = jsval_bool(result);
        return S_OK;
    }


    bstr = SysAllocStringLen(NULL, jsstr_length(name));
    if(bstr)
        jsstr_flush(name, bstr);
    jsstr_release(name);
    if(!bstr)
        return E_OUTOFMEMORY;

    if(is_dispex(jsthis))
        hres = IDispatchEx_GetDispID(jsthis->u.dispex, bstr, make_grfdex(ctx, fdexNameCaseSensitive), &id);
    else
        hres = IDispatch_GetIDsOfNames(jsthis->u.disp, &IID_NULL, &bstr, 1, ctx->lcid, &id);

    SysFreeString(bstr);
    if(r)
        *r = jsval_bool(SUCCEEDED(hres));
    return S_OK;
}

static HRESULT Object_propertyIsEnumerable(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    const WCHAR *name;
    jsstr_t *name_str;
    BOOL ret;
    HRESULT hres;

    TRACE("\n");

    if(argc != 1) {
        FIXME("argc %d not supported\n", argc);
        return E_NOTIMPL;
    }

    if(!is_jsdisp(jsthis)) {
        FIXME("Host object this\n");
        return E_FAIL;
    }

    hres = to_flat_string(ctx, argv[0], &name_str, &name);
    if(FAILED(hres))
        return hres;

    hres = jsdisp_is_enumerable(jsthis->u.jsdisp, name, &ret);
    jsstr_release(name_str);
    if(FAILED(hres))
        return hres;

    if(r)
        *r = jsval_bool(ret);
    return S_OK;
}

static HRESULT Object_isPrototypeOf(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Object_get_value(script_ctx_t *ctx, jsdisp_t *jsthis, jsval_t *r)
{
    jsstr_t *ret;

    TRACE("\n");

    ret = jsstr_alloc(default_valueW);
    if(!ret)
        return E_OUTOFMEMORY;

    *r = jsval_string(ret);
    return S_OK;
}

static void Object_destructor(jsdisp_t *dispex)
{
    heap_free(dispex);
}

static const builtin_prop_t Object_props[] = {
    {hasOwnPropertyW,        Object_hasOwnProperty,        PROPF_METHOD|1},
    {isPrototypeOfW,         Object_isPrototypeOf,         PROPF_METHOD|1},
    {propertyIsEnumerableW,  Object_propertyIsEnumerable,  PROPF_METHOD|1},
    {toLocaleStringW,        Object_toLocaleString,        PROPF_METHOD},
    {toStringW,              Object_toString,              PROPF_METHOD},
    {valueOfW,               Object_valueOf,               PROPF_METHOD}
};

static const builtin_info_t Object_info = {
    JSCLASS_OBJECT,
    {NULL, NULL,0, Object_get_value},
    sizeof(Object_props)/sizeof(*Object_props),
    Object_props,
    Object_destructor,
    NULL
};

static const builtin_info_t ObjectInst_info = {
    JSCLASS_OBJECT,
    {NULL, NULL,0, Object_get_value},
    0, NULL,
    Object_destructor,
    NULL
};

static HRESULT ObjectConstr_value(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    HRESULT hres;

    TRACE("\n");

    switch(flags) {
    case DISPATCH_METHOD:
        if(argc) {
            if(!is_undefined(argv[0]) && !is_null(argv[0]) && (!is_object_instance(argv[0]) || get_object(argv[0]))) {
                IDispatch *disp;

                hres = to_object(ctx, argv[0], &disp);
                if(FAILED(hres))
                    return hres;

                if(r)
                    *r = jsval_disp(disp);
                else
                    IDispatch_Release(disp);
                return S_OK;
            }
        }
        /* fall through */
    case DISPATCH_CONSTRUCT: {
        jsdisp_t *obj;

        hres = create_object(ctx, NULL, &obj);
        if(FAILED(hres))
            return hres;

        if(r)
            *r = jsval_obj(obj);
        else
            jsdisp_release(obj);
        break;
    }

    default:
        FIXME("unimplemented flags: %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

HRESULT create_object_constr(script_ctx_t *ctx, jsdisp_t *object_prototype, jsdisp_t **ret)
{
    static const WCHAR ObjectW[] = {'O','b','j','e','c','t',0};

    return create_builtin_constructor(ctx, ObjectConstr_value, ObjectW, NULL, PROPF_CONSTR,
            object_prototype, ret);
}

HRESULT create_object_prototype(script_ctx_t *ctx, jsdisp_t **ret)
{
    return create_dispex(ctx, &Object_info, NULL, ret);
}

HRESULT create_object(script_ctx_t *ctx, jsdisp_t *constr, jsdisp_t **ret)
{
    jsdisp_t *object;
    HRESULT hres;

    object = heap_alloc_zero(sizeof(jsdisp_t));
    if(!object)
        return E_OUTOFMEMORY;

    hres = init_dispex_from_constr(object, ctx, &ObjectInst_info, constr ? constr : ctx->object_constr);
    if(FAILED(hres)) {
        heap_free(object);
        return hres;
    }

    *ret = object;
    return S_OK;
}
