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

static const WCHAR toStringW[] = {'t','o','S','t','r','i','n','g',0};
static const WCHAR toLocaleStringW[] = {'t','o','L','o','c','a','l','e','S','t','r','i','n','g',0};
static const WCHAR valueOfW[] = {'v','a','l','u','e','O','f',0};
static const WCHAR hasOwnPropertyW[] = {'h','a','s','O','w','n','P','r','o','p','e','r','t','y',0};
static const WCHAR propertyIsEnumerableW[] =
    {'p','r','o','p','e','r','t','y','I','s','E','n','u','m','e','r','a','b','l','e',0};
static const WCHAR isPrototypeOfW[] = {'i','s','P','r','o','t','o','t','y','p','e','O','f',0};

static const WCHAR default_valueW[] = {'[','o','b','j','e','c','t',' ','O','b','j','e','c','t',']',0};

static HRESULT Object_toString(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
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
        functionW, NULL, mathW, numberW, objectW, regexpW, stringW};

    TRACE("\n");

    if(names[dispex->builtin_info->class] == NULL) {
        ERR("dispex->builtin_info->class = %d\n",
                dispex->builtin_info->class);
        return E_FAIL;
    }

    if(retv) {
        V_VT(retv) = VT_BSTR;
        V_BSTR(retv) = SysAllocStringLen(NULL, 9+strlenW(names[dispex->builtin_info->class]));
        if(!V_BSTR(retv))
            return E_OUTOFMEMORY;

        sprintfW(V_BSTR(retv), formatW, names[dispex->builtin_info->class]);
    }

    return S_OK;
}

static HRESULT Object_toLocaleString(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    TRACE("\n");
    return Object_toString(dispex, lcid, flags, dp, retv, ei, sp);
}

static HRESULT Object_valueOf(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    TRACE("\n");

    if(retv) {
        IDispatchEx_AddRef(_IDispatchEx_(dispex));

        V_VT(retv) = VT_DISPATCH;
        V_DISPATCH(retv) = (IDispatch*)_IDispatchEx_(dispex);
    }

    return S_OK;
}

static HRESULT Object_hasOwnProperty(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Object_propertyIsEnumerable(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Object_isPrototypeOf(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Object_value(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    TRACE("\n");

    switch(flags) {
    case INVOKE_FUNC:
        return throw_type_error(dispex->ctx, ei, IDS_NOT_FUNC, NULL);
    case DISPATCH_PROPERTYGET:
        V_VT(retv) = VT_BSTR;
        V_BSTR(retv) = SysAllocString(default_valueW);
        if(!V_BSTR(retv))
            return E_OUTOFMEMORY;
        break;
    default:
        FIXME("unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

static void Object_destructor(DispatchEx *dispex)
{
    heap_free(dispex);
}

static const builtin_prop_t Object_props[] = {
    {hasOwnPropertyW,        Object_hasOwnProperty,        PROPF_METHOD},
    {isPrototypeOfW,         Object_isPrototypeOf,         PROPF_METHOD},
    {propertyIsEnumerableW,  Object_propertyIsEnumerable,  PROPF_METHOD},
    {toLocaleStringW,        Object_toLocaleString,        PROPF_METHOD},
    {toStringW,              Object_toString,              PROPF_METHOD},
    {valueOfW,               Object_valueOf,               PROPF_METHOD}
};

static const builtin_info_t Object_info = {
    JSCLASS_OBJECT,
    {NULL, Object_value, 0},
    sizeof(Object_props)/sizeof(*Object_props),
    Object_props,
    Object_destructor,
    NULL
};

static HRESULT ObjectConstr_value(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    HRESULT hres;

    TRACE("\n");

    switch(flags) {
    case DISPATCH_CONSTRUCT: {
        DispatchEx *obj;

        hres = create_object(dispex->ctx, NULL, &obj);
        if(FAILED(hres))
            return hres;

        V_VT(retv) = VT_DISPATCH;
        V_DISPATCH(retv) = (IDispatch*)_IDispatchEx_(obj);
        break;
    }

    default:
        FIXME("unimplemented flags: %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

HRESULT create_object_constr(script_ctx_t *ctx, DispatchEx *object_prototype, DispatchEx **ret)
{
    return create_builtin_function(ctx, ObjectConstr_value, NULL, PROPF_CONSTR,
            object_prototype, ret);
}

HRESULT create_object_prototype(script_ctx_t *ctx, DispatchEx **ret)
{
    return create_dispex(ctx, &Object_info, NULL, ret);
}

HRESULT create_object(script_ctx_t *ctx, DispatchEx *constr, DispatchEx **ret)
{
    DispatchEx *object;
    HRESULT hres;

    object = heap_alloc_zero(sizeof(DispatchEx));
    if(!object)
        return E_OUTOFMEMORY;

    hres = init_dispex_from_constr(object, ctx, &Object_info, constr ? constr : ctx->object_constr);
    if(FAILED(hres)) {
        heap_free(object);
        return hres;
    }

    *ret = object;
    return S_OK;
}
