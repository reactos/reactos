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

    DWORD length;
} ArrayInstance;

static const WCHAR lengthW[] = {'l','e','n','g','t','h',0};
static const WCHAR concatW[] = {'c','o','n','c','a','t',0};
static const WCHAR joinW[] = {'j','o','i','n',0};
static const WCHAR popW[] = {'p','o','p',0};
static const WCHAR pushW[] = {'p','u','s','h',0};
static const WCHAR reverseW[] = {'r','e','v','e','r','s','e',0};
static const WCHAR shiftW[] = {'s','h','i','f','t',0};
static const WCHAR sliceW[] = {'s','l','i','c','e',0};
static const WCHAR sortW[] = {'s','o','r','t',0};
static const WCHAR spliceW[] = {'s','p','l','i','c','e',0};
static const WCHAR toStringW[] = {'t','o','S','t','r','i','n','g',0};
static const WCHAR toLocaleStringW[] = {'t','o','L','o','c','a','l','e','S','t','r','i','n','g',0};
static const WCHAR valueOfW[] = {'v','a','l','u','e','O','f',0};
static const WCHAR unshiftW[] = {'u','n','s','h','i','f','t',0};
static const WCHAR hasOwnPropertyW[] = {'h','a','s','O','w','n','P','r','o','p','e','r','t','y',0};
static const WCHAR propertyIsEnumerableW[] =
    {'p','r','o','p','e','r','t','y','I','s','E','n','u','m','e','r','a','b','l','e',0};
static const WCHAR isPrototypeOfW[] = {'i','s','P','r','o','t','o','t','y','p','e','O','f',0};

static const WCHAR default_separatorW[] = {',',0};

static HRESULT Array_length(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    ArrayInstance *This = (ArrayInstance*)dispex;

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

static HRESULT concat_array(DispatchEx *array, ArrayInstance *obj, DWORD *len, LCID lcid,
        jsexcept_t *ei, IServiceProvider *caller)
{
    VARIANT var;
    DWORD i;
    HRESULT hres;

    for(i=0; i < obj->length; i++) {
        hres = jsdisp_propget_idx(&obj->dispex, i, lcid, &var, ei, caller);
        if(hres == DISP_E_UNKNOWNNAME)
            continue;
        if(FAILED(hres))
            return hres;

        hres = jsdisp_propput_idx(array, *len+i, lcid, &var, ei, caller);
        VariantClear(&var);
        if(FAILED(hres))
            return hres;
    }

    *len += obj->length;
    return S_OK;
}

static HRESULT concat_obj(DispatchEx *array, IDispatch *obj, DWORD *len, LCID lcid, jsexcept_t *ei, IServiceProvider *caller)
{
    DispatchEx *jsobj;
    VARIANT var;
    HRESULT hres;

    jsobj = iface_to_jsdisp((IUnknown*)obj);
    if(jsobj) {
        if(is_class(jsobj, JSCLASS_ARRAY)) {
            hres = concat_array(array, (ArrayInstance*)jsobj, len, lcid, ei, caller);
            jsdisp_release(jsobj);
            return hres;
        }
        jsdisp_release(jsobj);
    }

    V_VT(&var) = VT_DISPATCH;
    V_DISPATCH(&var) = obj;
    return jsdisp_propput_idx(array, (*len)++, lcid, &var, ei, caller);
}

static HRESULT Array_concat(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    DispatchEx *ret;
    DWORD len = 0;
    HRESULT hres;

    TRACE("\n");

    hres = create_array(dispex->ctx, 0, &ret);
    if(FAILED(hres))
        return hres;

    hres = concat_obj(ret, (IDispatch*)_IDispatchEx_(dispex), &len, lcid, ei, caller);
    if(SUCCEEDED(hres)) {
        VARIANT *arg;
        DWORD i;

        for(i=0; i < arg_cnt(dp); i++) {
            arg = get_arg(dp, i);
            if(V_VT(arg) == VT_DISPATCH)
                hres = concat_obj(ret, V_DISPATCH(arg), &len, lcid, ei, caller);
            else
                hres = jsdisp_propput_idx(ret, len++, lcid, arg, ei, caller);
            if(FAILED(hres))
                break;
        }
    }

    if(FAILED(hres))
        return hres;

    if(retv) {
        V_VT(retv) = VT_DISPATCH;
        V_DISPATCH(retv) = (IDispatch*)_IDispatchEx_(ret);
    }else {
        jsdisp_release(ret);
    }
    return S_OK;
}

static HRESULT array_join(DispatchEx *array, LCID lcid, DWORD length, const WCHAR *sep, VARIANT *retv,
        jsexcept_t *ei, IServiceProvider *caller)
{
    BSTR *str_tab, ret = NULL;
    VARIANT var;
    DWORD i;
    HRESULT hres = E_FAIL;

    if(!length) {
        if(retv) {
            V_VT(retv) = VT_BSTR;
            V_BSTR(retv) = SysAllocStringLen(NULL, 0);
            if(!V_BSTR(retv))
                return E_OUTOFMEMORY;
        }
        return S_OK;
    }

    str_tab = heap_alloc_zero(length * sizeof(BSTR));
    if(!str_tab)
        return E_OUTOFMEMORY;

    for(i=0; i < length; i++) {
        hres = jsdisp_propget_idx(array, i, lcid, &var, ei, caller);
        if(FAILED(hres))
            break;

        if(V_VT(&var) != VT_EMPTY && V_VT(&var) != VT_NULL)
            hres = to_string(array->ctx, &var, ei, str_tab+i);
        VariantClear(&var);
        if(FAILED(hres))
            break;
    }

    if(SUCCEEDED(hres)) {
        DWORD seplen = 0, len = 0;
        WCHAR *ptr;

        seplen = strlenW(sep);

        if(str_tab[0])
            len = SysStringLen(str_tab[0]);
        for(i=1; i < length; i++)
            len += seplen + SysStringLen(str_tab[i]);

        ret = SysAllocStringLen(NULL, len);
        if(ret) {
            DWORD tmplen = 0;

            if(str_tab[0]) {
                tmplen = SysStringLen(str_tab[0]);
                memcpy(ret, str_tab[0], tmplen*sizeof(WCHAR));
            }

            ptr = ret + tmplen;
            for(i=1; i < length; i++) {
                if(seplen) {
                    memcpy(ptr, sep, seplen*sizeof(WCHAR));
                    ptr += seplen;
                }

                if(str_tab[i]) {
                    tmplen = SysStringLen(str_tab[i]);
                    memcpy(ptr, str_tab[i], tmplen*sizeof(WCHAR));
                    ptr += tmplen;
                }
            }
            *ptr=0;
        }else {
            hres = E_OUTOFMEMORY;
        }
    }

    for(i=0; i < length; i++)
        SysFreeString(str_tab[i]);
    heap_free(str_tab);
    if(FAILED(hres))
        return hres;

    TRACE("= %s\n", debugstr_w(ret));

    if(retv) {
        if(!ret) {
            ret = SysAllocStringLen(NULL, 0);
            if(!ret)
                return E_OUTOFMEMORY;
        }

        V_VT(retv) = VT_BSTR;
        V_BSTR(retv) = ret;
    }else {
        SysFreeString(ret);
    }

    return S_OK;
}

/* ECMA-262 3rd Edition    15.4.4.5 */
static HRESULT Array_join(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    DWORD length;
    HRESULT hres;

    TRACE("\n");

    if(is_class(dispex, JSCLASS_ARRAY)) {
        length = ((ArrayInstance*)dispex)->length;
    }else {
        FIXME("dispid is not Array\n");
        return E_NOTIMPL;
    }

    if(arg_cnt(dp)) {
        BSTR sep;

        hres = to_string(dispex->ctx, dp->rgvarg + dp->cArgs-1, ei, &sep);
        if(FAILED(hres))
            return hres;

        hres = array_join(dispex, lcid, length, sep, retv, ei, caller);

        SysFreeString(sep);
    }else {
        hres = array_join(dispex, lcid, length, default_separatorW, retv, ei, caller);
    }

    return hres;
}

static HRESULT Array_pop(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    VARIANT val;
    DWORD length;
    WCHAR buf[14];
    DISPID id;
    HRESULT hres;

    static const WCHAR formatW[] = {'%','d',0};

    TRACE("\n");

    if(is_class(dispex, JSCLASS_ARRAY)) {
        ArrayInstance *array = (ArrayInstance*)dispex;
        length = array->length;
    }else {
        FIXME("not Array this\n");
        return E_NOTIMPL;
    }

    if(!length) {
        if(retv)
            V_VT(retv) = VT_EMPTY;
        return S_OK;
    }

    sprintfW(buf, formatW, --length);
    hres = jsdisp_get_id(dispex, buf, 0, &id);
    if(SUCCEEDED(hres)) {
        hres = jsdisp_propget(dispex, id, lcid, &val, ei, caller);
        if(FAILED(hres))
            return hres;

        hres = IDispatchEx_DeleteMemberByDispID(_IDispatchEx_(dispex), id);
    }else if(hres == DISP_E_UNKNOWNNAME) {
        V_VT(&val) = VT_EMPTY;
        hres = S_OK;
    }else {
        return hres;
    }

    if(SUCCEEDED(hres)) {
        if(is_class(dispex, JSCLASS_ARRAY)) {
            ArrayInstance *array = (ArrayInstance*)dispex;
            array->length = length;
        }
    }

    if(FAILED(hres)) {
        VariantClear(&val);
        return hres;
    }

    if(retv)
        *retv = val;
    else
        VariantClear(&val);
    return S_OK;
}

/* ECMA-262 3rd Edition    15.4.4.7 */
static HRESULT Array_push(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    DWORD length = 0;
    int i, n;
    HRESULT hres;

    TRACE("\n");

    if(dispex->builtin_info->class == JSCLASS_ARRAY) {
        length = ((ArrayInstance*)dispex)->length;
    }else {
        FIXME("not Array this\n");
        return E_NOTIMPL;
    }

    n = dp->cArgs - dp->cNamedArgs;
    for(i=0; i < n; i++) {
        hres = jsdisp_propput_idx(dispex, length+i, lcid, get_arg(dp, i), ei, sp);
        if(FAILED(hres))
            return hres;
    }

    if(retv) {
        V_VT(retv) = VT_I4;
        V_I4(retv) = length+n;
    }
    return S_OK;
}

static HRESULT Array_reverse(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Array_shift(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Array_slice(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT sort_cmp(script_ctx_t *ctx, DispatchEx *cmp_func, VARIANT *v1, VARIANT *v2, jsexcept_t *ei,
        IServiceProvider *caller, INT *cmp)
{
    HRESULT hres;

    if(cmp_func) {
        VARIANTARG args[2];
        DISPPARAMS dp = {args, NULL, 2, 0};
        VARIANT tmp;
        VARIANT res;

        args[0] = *v2;
        args[1] = *v1;

        hres = jsdisp_call_value(cmp_func, ctx->lcid, DISPATCH_METHOD, &dp, &res, ei, caller);
        if(FAILED(hres))
            return hres;

        hres = to_number(ctx, &res, ei, &tmp);
        VariantClear(&res);
        if(FAILED(hres))
            return hres;

        if(V_VT(&tmp) == VT_I4)
            *cmp = V_I4(&tmp);
        else
            *cmp = V_R8(&tmp) > 0.0 ? 1 : -1;
    }else if(is_num_vt(V_VT(v1))) {
        if(is_num_vt(V_VT(v2))) {
            DOUBLE d = num_val(v1)-num_val(v2);
            if(d > 0.0)
                *cmp = 1;
            else if(d < -0.0)
                *cmp = -1;
            else
                *cmp = 0;
        }else {
            *cmp = -1;
        }
    }else if(is_num_vt(V_VT(v2))) {
        *cmp = 1;
    }else if(V_VT(v1) == VT_BSTR) {
        if(V_VT(v2) == VT_BSTR)
            *cmp = strcmpW(V_BSTR(v1), V_BSTR(v2));
        else
            *cmp = -1;
    }else if(V_VT(v2) == VT_BSTR) {
        *cmp = 1;
    }else {
        *cmp = 0;
    }

    return S_OK;
}

/* ECMA-262 3rd Edition    15.4.4.11 */
static HRESULT Array_sort(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    DispatchEx *cmp_func = NULL;
    VARIANT *vtab, **sorttab = NULL;
    DWORD length;
    DWORD i;
    HRESULT hres = S_OK;

    TRACE("\n");

    if(is_class(dispex, JSCLASS_ARRAY)) {
        length = ((ArrayInstance*)dispex)->length;
    }else {
        FIXME("unsupported this not array\n");
        return E_NOTIMPL;
    }

    if(arg_cnt(dp) > 1) {
        WARN("invalid arg_cnt %d\n", arg_cnt(dp));
        return E_FAIL;
    }

    if(arg_cnt(dp) == 1) {
        VARIANT *arg = get_arg(dp, 0);

        if(V_VT(arg) != VT_DISPATCH) {
            WARN("arg is not dispatch\n");
            return E_FAIL;
        }


        cmp_func = iface_to_jsdisp((IUnknown*)V_DISPATCH(arg));
        if(!cmp_func || !is_class(cmp_func, JSCLASS_FUNCTION)) {
            WARN("cmp_func is not a function\n");
            if(cmp_func)
                jsdisp_release(cmp_func);
            return E_FAIL;
        }
    }

    if(!length) {
        if(cmp_func)
            jsdisp_release(cmp_func);
        if(retv) {
            V_VT(retv) = VT_DISPATCH;
            V_DISPATCH(retv) = (IDispatch*)_IDispatchEx_(dispex);
	    IDispatchEx_AddRef(_IDispatchEx_(dispex));
        }
        return S_OK;
    }

    vtab = heap_alloc_zero(length * sizeof(VARIANT));
    if(vtab) {
        for(i=0; i<length; i++) {
            hres = jsdisp_propget_idx(dispex, i, lcid, vtab+i, ei, caller);
            if(FAILED(hres) && hres != DISP_E_UNKNOWNNAME) {
                WARN("Could not get elem %d: %08x\n", i, hres);
                break;
            }
        }
    }else {
        hres = E_OUTOFMEMORY;
    }

    if(SUCCEEDED(hres)) {
        sorttab = heap_alloc(length*2*sizeof(VARIANT*));
        if(!sorttab)
            hres = E_OUTOFMEMORY;
    }

    /* merge-sort */
    if(SUCCEEDED(hres)) {
        VARIANT *tmpv, **tmpbuf;
        INT cmp;

        tmpbuf = sorttab + length;
        for(i=0; i < length; i++)
            sorttab[i] = vtab+i;

        for(i=0; i < length/2; i++) {
            hres = sort_cmp(dispex->ctx, cmp_func, sorttab[2*i+1], sorttab[2*i], ei, caller, &cmp);
            if(FAILED(hres))
                break;

            if(cmp < 0) {
                tmpv = sorttab[2*i];
                sorttab[2*i] = sorttab[2*i+1];
                sorttab[2*i+1] = tmpv;
            }
        }

        if(SUCCEEDED(hres)) {
            DWORD k, a, b, bend;

            for(k=2; k < length; k *= 2) {
                for(i=0; i+k < length; i += 2*k) {
                    a = b = 0;
                    if(i+2*k <= length)
                        bend = k;
                    else
                        bend = length - (i+k);

                    memcpy(tmpbuf, sorttab+i, k*sizeof(VARIANT*));

                    while(a < k && b < bend) {
                        hres = sort_cmp(dispex->ctx, cmp_func, tmpbuf[a], sorttab[i+k+b], ei, caller, &cmp);
                        if(FAILED(hres))
                            break;

                        if(cmp < 0) {
                            sorttab[i+a+b] = tmpbuf[a];
                            a++;
                        }else {
                            sorttab[i+a+b] = sorttab[i+k+b];
                            b++;
                        }
                    }

                    if(FAILED(hres))
                        break;

                    if(a < k)
                        memcpy(sorttab+i+a+b, tmpbuf+a, (k-a)*sizeof(VARIANT*));
                }

                if(FAILED(hres))
                    break;
            }
        }

        for(i=0; SUCCEEDED(hres) && i < length; i++)
            hres = jsdisp_propput_idx(dispex, i, lcid, sorttab[i], ei, caller);
    }

    if(vtab) {
        for(i=0; i < length; i++)
            VariantClear(vtab+i);
        heap_free(vtab);
    }
    heap_free(sorttab);
    if(cmp_func)
        jsdisp_release(cmp_func);

    if(FAILED(hres))
        return hres;

    if(retv) {
        V_VT(retv) = VT_DISPATCH;
        V_DISPATCH(retv) = (IDispatch*)_IDispatchEx_(dispex);
        IDispatch_AddRef(_IDispatchEx_(dispex));
    }

    return S_OK;
}

static HRESULT Array_splice(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

/* ECMA-262 3rd Edition    15.4.4.2 */
static HRESULT Array_toString(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    TRACE("\n");

    if(!is_class(dispex, JSCLASS_ARRAY)) {
        WARN("not Array object\n");
        return E_FAIL;
    }

    return array_join(dispex, lcid, ((ArrayInstance*)dispex)->length, default_separatorW, retv, ei, sp);
}

static HRESULT Array_toLocaleString(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Array_valueOf(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Array_unshift(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Array_hasOwnProperty(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Array_propertyIsEnumerable(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Array_isPrototypeOf(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Array_value(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    TRACE("\n");

    switch(flags) {
    case INVOKE_PROPERTYGET:
        return array_join(dispex, lcid, ((ArrayInstance*)dispex)->length, default_separatorW, retv, ei, sp);
    default:
        FIXME("unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

static void Array_destructor(DispatchEx *dispex)
{
    heap_free(dispex);
}

static void Array_on_put(DispatchEx *dispex, const WCHAR *name)
{
    ArrayInstance *array = (ArrayInstance*)dispex;
    const WCHAR *ptr = name;
    DWORD id = 0;

    if(!isdigitW(*ptr))
        return;

    while(*ptr && isdigitW(*ptr)) {
        id = id*10 + (*ptr-'0');
        ptr++;
    }

    if(*ptr)
        return;

    if(id >= array->length)
        array->length = id+1;
}

static const builtin_prop_t Array_props[] = {
    {concatW,                Array_concat,               PROPF_METHOD},
    {hasOwnPropertyW,        Array_hasOwnProperty,       PROPF_METHOD},
    {isPrototypeOfW,         Array_isPrototypeOf,        PROPF_METHOD},
    {joinW,                  Array_join,                 PROPF_METHOD},
    {lengthW,                Array_length,               0},
    {popW,                   Array_pop,                  PROPF_METHOD},
    {propertyIsEnumerableW,  Array_propertyIsEnumerable, PROPF_METHOD},
    {pushW,                  Array_push,                 PROPF_METHOD},
    {reverseW,               Array_reverse,              PROPF_METHOD},
    {shiftW,                 Array_shift,                PROPF_METHOD},
    {sliceW,                 Array_slice,                PROPF_METHOD},
    {sortW,                  Array_sort,                 PROPF_METHOD},
    {spliceW,                Array_splice,               PROPF_METHOD},
    {toLocaleStringW,        Array_toLocaleString,       PROPF_METHOD},
    {toStringW,              Array_toString,             PROPF_METHOD},
    {unshiftW,               Array_unshift,              PROPF_METHOD},
    {valueOfW,               Array_valueOf,              PROPF_METHOD}
};

static const builtin_info_t Array_info = {
    JSCLASS_ARRAY,
    {NULL, Array_value, 0},
    sizeof(Array_props)/sizeof(*Array_props),
    Array_props,
    Array_destructor,
    Array_on_put
};

static HRESULT ArrayConstr_value(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    DispatchEx *obj;
    VARIANT *arg_var;
    DWORD i;
    HRESULT hres;

    TRACE("\n");

    switch(flags) {
    case DISPATCH_CONSTRUCT: {
        if(arg_cnt(dp) == 1 && V_VT((arg_var = get_arg(dp, 0))) == VT_I4) {
            if(V_I4(arg_var) < 0) {
                FIXME("throw RangeError\n");
                return E_FAIL;
            }

            hres = create_array(dispex->ctx, V_I4(arg_var), &obj);
            if(FAILED(hres))
                return hres;

            V_VT(retv) = VT_DISPATCH;
            V_DISPATCH(retv) = (IDispatch*)_IDispatchEx_(obj);
            return S_OK;
        }

        hres = create_array(dispex->ctx, arg_cnt(dp), &obj);
        if(FAILED(hres))
            return hres;

        for(i=0; i < arg_cnt(dp); i++) {
            hres = jsdisp_propput_idx(obj, i, lcid, get_arg(dp, i), ei, caller);
            if(FAILED(hres))
                break;
        }
        if(FAILED(hres)) {
            jsdisp_release(obj);
            return hres;
        }

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

static HRESULT alloc_array(script_ctx_t *ctx, BOOL use_constr, ArrayInstance **ret)
{
    ArrayInstance *array;
    HRESULT hres;

    array = heap_alloc_zero(sizeof(ArrayInstance));
    if(!array)
        return E_OUTOFMEMORY;

    if(use_constr)
        hres = init_dispex_from_constr(&array->dispex, ctx, &Array_info, ctx->array_constr);
    else
        hres = init_dispex(&array->dispex, ctx, &Array_info, NULL);

    if(FAILED(hres)) {
        heap_free(array);
        return hres;
    }

    *ret = array;
    return S_OK;
}

HRESULT create_array_constr(script_ctx_t *ctx, DispatchEx **ret)
{
    ArrayInstance *array;
    HRESULT hres;

    hres = alloc_array(ctx, FALSE, &array);
    if(FAILED(hres))
        return hres;

    hres = create_builtin_function(ctx, ArrayConstr_value, PROPF_CONSTR, &array->dispex, ret);

    IDispatchEx_Release(_IDispatchEx_(&array->dispex));
    return hres;
}

HRESULT create_array(script_ctx_t *ctx, DWORD length, DispatchEx **ret)
{
    ArrayInstance *array;
    HRESULT hres;

    hres = alloc_array(ctx, TRUE, &array);
    if(FAILED(hres))
        return hres;

    array->length = length;

    *ret = &array->dispex;
    return S_OK;
}
