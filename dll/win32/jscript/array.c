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

#ifdef __REACTOS__
#include <wine/config.h>
#include <wine/port.h>
#endif

#include <math.h>
#include <assert.h>

#include "jscript.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

typedef struct {
    jsdisp_t dispex;

    DWORD length;
} ArrayInstance;

static inline ArrayInstance *array_from_jsdisp(jsdisp_t *jsdisp)
{
    return CONTAINING_RECORD(jsdisp, ArrayInstance, dispex);
}

static inline ArrayInstance *array_this(jsval_t vthis)
{
    jsdisp_t *jsdisp = is_object_instance(vthis) ? to_jsdisp(get_object(vthis)) : NULL;
    return (jsdisp && is_class(jsdisp, JSCLASS_ARRAY)) ? array_from_jsdisp(jsdisp) : NULL;
}

unsigned array_get_length(jsdisp_t *array)
{
    assert(is_class(array, JSCLASS_ARRAY));
    return array_from_jsdisp(array)->length;
}

static HRESULT get_length(script_ctx_t *ctx, jsval_t vthis, jsdisp_t **jsthis, UINT32 *ret)
{
    jsdisp_t *jsdisp;
    IDispatch *disp;
    jsval_t val;
    HRESULT hres;

    hres = to_object(ctx, vthis, &disp);
    if(FAILED(hres))
        return hres;

    jsdisp = iface_to_jsdisp(disp);
    IDispatch_Release(disp);
    if(!jsdisp)
        return JS_E_JSCRIPT_EXPECTED;
    *jsthis = jsdisp;

    if(is_class(jsdisp, JSCLASS_ARRAY)) {
        *ret = array_from_jsdisp(jsdisp)->length;
        return S_OK;
    }

    hres = jsdisp_propget_name(jsdisp, L"length", &val);
    if(SUCCEEDED(hres)) {
        hres = to_uint32(ctx, val, ret);
        jsval_release(val);
        if(SUCCEEDED(hres))
            return hres;
    }

    jsdisp_release(jsdisp);
    return hres;
}

static HRESULT set_length(jsdisp_t *obj, DWORD length)
{
    if(is_class(obj, JSCLASS_ARRAY)) {
        array_from_jsdisp(obj)->length = length;
        return S_OK;
    }

    return jsdisp_propput_name(obj, L"length", jsval_number(length));
}

WCHAR *idx_to_str(DWORD idx, WCHAR *ptr)
{
    if(!idx) {
        *ptr = '0';
        return ptr;
    }

    while(idx) {
        *ptr-- = '0' + (idx%10);
        idx /= 10;
    }

    return ptr+1;
}

static HRESULT Array_get_length(script_ctx_t *ctx, jsdisp_t *jsthis, jsval_t *r)
{
    TRACE("%p\n", jsthis);

    *r = jsval_number(array_from_jsdisp(jsthis)->length);
    return S_OK;
}

static HRESULT Array_set_length(script_ctx_t *ctx, jsdisp_t *jsthis, jsval_t value)
{
    ArrayInstance *This = array_from_jsdisp(jsthis);
    DOUBLE len = -1;
    DWORD i;
    HRESULT hres;

    TRACE("%p %ld\n", This, This->length);

    hres = to_number(ctx, value, &len);
    if(FAILED(hres))
        return hres;

    len = floor(len);
    if(len!=(DWORD)len)
        return JS_E_INVALID_LENGTH;

    for(i=len; i < This->length; i++) {
        hres = jsdisp_delete_idx(&This->dispex, i);
        if(FAILED(hres))
            return hres;
    }

    This->length = len;
    return S_OK;
}

static HRESULT concat_array(jsdisp_t *array, ArrayInstance *obj, DWORD *len)
{
    jsval_t val;
    DWORD i;
    HRESULT hres;

    for(i=0; i < obj->length; i++) {
        hres = jsdisp_get_idx(&obj->dispex, i, &val);
        if(hres == DISP_E_UNKNOWNNAME)
            continue;
        if(FAILED(hres))
            return hres;

        hres = jsdisp_propput_idx(array, *len+i, val);
        jsval_release(val);
        if(FAILED(hres))
            return hres;
    }

    *len += obj->length;
    return S_OK;
}

static HRESULT concat_obj(jsdisp_t *array, IDispatch *obj, DWORD *len)
{
    jsdisp_t *jsobj;
    HRESULT hres;

    jsobj = iface_to_jsdisp(obj);
    if(jsobj) {
        if(is_class(jsobj, JSCLASS_ARRAY)) {
            hres = concat_array(array, array_from_jsdisp(jsobj), len);
            jsdisp_release(jsobj);
            return hres;
        }
        jsdisp_release(jsobj);
    }

    return jsdisp_propput_idx(array, (*len)++, jsval_disp(obj));
}

static HRESULT Array_concat(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    IDispatch *jsthis;
    jsdisp_t *ret;
    DWORD len = 0;
    HRESULT hres;

    TRACE("\n");

    hres = to_object(ctx, vthis, &jsthis);
    if(FAILED(hres))
        return hres;

    hres = create_array(ctx, 0, &ret);
    if(FAILED(hres))
        goto done;

    hres = concat_obj(ret, jsthis, &len);
    if(SUCCEEDED(hres)) {
        DWORD i;

        for(i=0; i < argc; i++) {
            if(is_object_instance(argv[i]))
                hres = concat_obj(ret, get_object(argv[i]), &len);
            else
                hres = jsdisp_propput_idx(ret, len++, argv[i]);
            if(FAILED(hres))
                break;
        }
    }

    if(FAILED(hres))
        goto done;

    if(r)
        *r = jsval_obj(ret);
    else
        jsdisp_release(ret);
done:
    IDispatch_Release(jsthis);
    return S_OK;
}

static HRESULT array_join(script_ctx_t *ctx, jsdisp_t *array, DWORD length, const WCHAR *sep,
        unsigned seplen, HRESULT (*to_string)(script_ctx_t*,jsval_t,jsstr_t**), jsval_t *r)
{
    jsstr_t **str_tab, *ret = NULL;
    jsval_t val;
    DWORD i;
    HRESULT hres = E_FAIL;

    if(!length) {
        if(r)
            *r = jsval_string(jsstr_empty());
        return S_OK;
    }

    str_tab = calloc(length, sizeof(*str_tab));
    if(!str_tab)
        return E_OUTOFMEMORY;

    for(i=0; i < length; i++) {
        hres = jsdisp_get_idx(array, i, &val);
        if(hres == DISP_E_UNKNOWNNAME) {
            hres = S_OK;
            continue;
        } else if(FAILED(hres))
            break;

        if(!is_undefined(val) && !is_null(val)) {
            hres = to_string(ctx, val, str_tab+i);
            jsval_release(val);
            if(FAILED(hres))
                break;
        }
    }

    if(SUCCEEDED(hres)) {
        DWORD len = 0;

        if(str_tab[0])
            len = jsstr_length(str_tab[0]);
        for(i=1; i < length; i++) {
            len += seplen;
            if(str_tab[i])
                len += jsstr_length(str_tab[i]);
            if(len > JSSTR_MAX_LENGTH) {
                hres = E_OUTOFMEMORY;
                break;
            }
        }

        if(SUCCEEDED(hres)) {
            WCHAR *ptr = NULL;

            ret = jsstr_alloc_buf(len, &ptr);
            if(ret) {
                if(str_tab[0])
                    ptr += jsstr_flush(str_tab[0], ptr);

                for(i=1; i < length; i++) {
                    if(seplen) {
                        memcpy(ptr, sep, seplen*sizeof(WCHAR));
                        ptr += seplen;
                    }

                    if(str_tab[i])
                        ptr += jsstr_flush(str_tab[i], ptr);
                }
            }else {
                hres = E_OUTOFMEMORY;
            }
        }
    }

    for(i=0; i < length; i++) {
        if(str_tab[i])
            jsstr_release(str_tab[i]);
    }
    free(str_tab);
    if(FAILED(hres))
        return hres;

    TRACE("= %s\n", debugstr_jsstr(ret));

    if(r)
        *r = jsval_string(ret);
    else
        jsstr_release(ret);
    return S_OK;
}

/* ECMA-262 3rd Edition    15.4.4.5 */
static HRESULT Array_join(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsdisp_t *jsthis;
    UINT32 length;
    HRESULT hres;

    TRACE("\n");

    hres = get_length(ctx, vthis, &jsthis, &length);
    if(FAILED(hres))
        return hres;

    if(argc) {
        const WCHAR *sep;
        jsstr_t *sep_str;

        hres = to_flat_string(ctx, argv[0], &sep_str, &sep);
        if(FAILED(hres))
            goto done;

        hres = array_join(ctx, jsthis, length, sep, jsstr_length(sep_str), to_string, r);

        jsstr_release(sep_str);
    }else {
        hres = array_join(ctx, jsthis, length, L",", 1, to_string, r);
    }

done:
    jsdisp_release(jsthis);
    return hres;
}

static HRESULT Array_pop(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsdisp_t *jsthis;
    jsval_t val;
    UINT32 length;
    HRESULT hres;

    TRACE("\n");

    hres = get_length(ctx, vthis, &jsthis, &length);
    if(FAILED(hres))
        return hres;

    if(!length) {
        hres = set_length(jsthis, 0);
        if(FAILED(hres))
            goto done;

        if(r)
            *r = jsval_undefined();
        goto done;
    }

    length--;
    hres = jsdisp_get_idx(jsthis, length, &val);
    if(SUCCEEDED(hres))
        hres = jsdisp_delete_idx(jsthis, length);
    else if(hres == DISP_E_UNKNOWNNAME) {
        val = jsval_undefined();
        hres = S_OK;
    }else
        goto done;

    if(SUCCEEDED(hres))
        hres = set_length(jsthis, length);

    if(FAILED(hres)) {
        jsval_release(val);
        goto done;
    }

    if(r)
        *r = val;
    else
        jsval_release(val);
done:
    jsdisp_release(jsthis);
    return hres;
}

/* ECMA-262 3rd Edition    15.4.4.7 */
static HRESULT Array_push(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsdisp_t *jsthis;
    UINT32 length = 0;
    unsigned i;
    HRESULT hres;

    TRACE("\n");

    hres = get_length(ctx, vthis, &jsthis, &length);
    if(FAILED(hres))
        return hres;

    for(i=0; i < argc; i++) {
        hres = jsdisp_propput_idx(jsthis, length+i, argv[i]);
        if(FAILED(hres))
            goto done;
    }

    hres = set_length(jsthis, length+argc);
    if(FAILED(hres))
        goto done;

    if(r)
        *r = jsval_number(length+argc);
done:
    jsdisp_release(jsthis);
    return hres;
}

static HRESULT Array_reverse(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsdisp_t *jsthis;
    UINT32 length, k, l;
    jsval_t v1, v2;
    HRESULT hres1, hres2;

    TRACE("\n");

    hres1 = get_length(ctx, vthis, &jsthis, &length);
    if(FAILED(hres1))
        return hres1;

    for(k=0; k<length/2; k++) {
        l = length-k-1;

        hres1 = jsdisp_get_idx(jsthis, k, &v1);
        if(FAILED(hres1) && hres1!=DISP_E_UNKNOWNNAME)
            goto done;

        hres2 = jsdisp_get_idx(jsthis, l, &v2);
        if(FAILED(hres2) && hres2!=DISP_E_UNKNOWNNAME) {
            jsval_release(v1);
            hres1 = hres2;
            goto done;
        }

        if(hres1 == DISP_E_UNKNOWNNAME)
            hres1 = jsdisp_delete_idx(jsthis, l);
        else
            hres1 = jsdisp_propput_idx(jsthis, l, v1);

        if(FAILED(hres1)) {
            jsval_release(v1);
            jsval_release(v2);
            goto done;
        }

        if(hres2 == DISP_E_UNKNOWNNAME)
            hres2 = jsdisp_delete_idx(jsthis, k);
        else
            hres2 = jsdisp_propput_idx(jsthis, k, v2);

        if(FAILED(hres2)) {
            jsval_release(v2);
            hres1 = hres2;
            goto done;
        }
    }

    if(r)
        *r = jsval_obj(jsdisp_addref(jsthis));
done:
    jsdisp_release(jsthis);
    return hres1;
}

/* ECMA-262 3rd Edition    15.4.4.9 */
static HRESULT Array_shift(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsdisp_t *jsthis;
    UINT32 length = 0, i;
    jsval_t v, ret;
    HRESULT hres;

    TRACE("\n");

    hres = get_length(ctx, vthis, &jsthis, &length);
    if(FAILED(hres))
        return hres;

    if(!length) {
        hres = set_length(jsthis, 0);
        if(FAILED(hres))
            goto done;

        if(r)
            *r = jsval_undefined();
        goto done;
    }

    hres = jsdisp_get_idx(jsthis, 0, &ret);
    if(hres == DISP_E_UNKNOWNNAME) {
        ret = jsval_undefined();
        hres = S_OK;
    }

    for(i=1; SUCCEEDED(hres) && i<length; i++) {
        hres = jsdisp_get_idx(jsthis, i, &v);
        if(hres == DISP_E_UNKNOWNNAME)
            hres = jsdisp_delete_idx(jsthis, i-1);
        else if(SUCCEEDED(hres)) {
            hres = jsdisp_propput_idx(jsthis, i-1, v);
            jsval_release(v);
        }
    }

    if(SUCCEEDED(hres)) {
        hres = jsdisp_delete_idx(jsthis, length-1);
        if(SUCCEEDED(hres))
            hres = set_length(jsthis, length-1);
    }

    if(FAILED(hres))
        goto done;

    if(r)
        *r = ret;
    else
        jsval_release(ret);
done:
    jsdisp_release(jsthis);
    return hres;
}

/* ECMA-262 3rd Edition    15.4.4.10 */
static HRESULT Array_slice(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv, jsval_t *r)
{
    jsdisp_t *arr, *jsthis;
    DOUBLE range;
    UINT32 length, start, end, idx;
    HRESULT hres;

    TRACE("\n");

    hres = get_length(ctx, vthis, &jsthis, &length);
    if(FAILED(hres))
        return hres;

    if(argc) {
        hres = to_number(ctx, argv[0], &range);
        if(FAILED(hres))
            goto done;

        range = floor(range);
        if(-range>length || isnan(range)) start = 0;
        else if(range < 0) start = range+length;
        else if(range <= length) start = range;
        else start = length;
    }
    else start = 0;

    if(argc > 1) {
        hres = to_number(ctx, argv[1], &range);
        if(FAILED(hres))
            goto done;

        range = floor(range);
        if(-range>length) end = 0;
        else if(range < 0) end = range+length;
        else if(range <= length) end = range;
        else end = length;
    }
    else end = length;

    hres = create_array(ctx, (end>start)?end-start:0, &arr);
    if(FAILED(hres))
        goto done;

    for(idx=start; idx<end; idx++) {
        jsval_t v;

        hres = jsdisp_get_idx(jsthis, idx, &v);
        if(hres == DISP_E_UNKNOWNNAME)
            continue;

        if(SUCCEEDED(hres)) {
            hres = jsdisp_propput_idx(arr, idx-start, v);
            jsval_release(v);
        }

        if(FAILED(hres)) {
            jsdisp_release(arr);
            goto done;
        }
    }

    if(r)
        *r = jsval_obj(arr);
    else
        jsdisp_release(arr);
    hres = S_OK;

done:
    jsdisp_release(jsthis);
    return hres;
}

static HRESULT sort_cmp(script_ctx_t *ctx, jsdisp_t *cmp_func, jsval_t v1, jsval_t v2, INT *cmp)
{
    HRESULT hres;

    if(cmp_func) {
        jsval_t args[2] = {v1, v2};
        jsval_t res;
        double n;

        hres = jsdisp_call_value(cmp_func, jsval_undefined(), DISPATCH_METHOD, 2, args, &res);
        if(FAILED(hres))
            return hres;

        hres = to_number(ctx, res, &n);
        jsval_release(res);
        if(FAILED(hres))
            return hres;

        if(n == 0)
            *cmp = 0;
        *cmp = n > 0.0 ? 1 : -1;
    }else if(is_undefined(v1)) {
        *cmp = is_undefined(v2) ? 0 : 1;
    }else if(is_undefined(v2)) {
        *cmp = -1;
    }else if(is_number(v1) && is_number(v2)) {
        double d = get_number(v1)-get_number(v2);
        if(d > 0.0)
            *cmp = 1;
        else
            *cmp = d < -0.0 ? -1 : 0;
    }else {
        jsstr_t *x, *y;

        hres = to_string(ctx, v1, &x);
        if(FAILED(hres))
            return hres;

        hres = to_string(ctx, v2, &y);
        if(SUCCEEDED(hres)) {
            *cmp = jsstr_cmp(x, y);
            jsstr_release(y);
        }
        jsstr_release(x);
        if(FAILED(hres))
            return hres;
    }

    return S_OK;
}

/* ECMA-262 3rd Edition    15.4.4.11 */
static HRESULT Array_sort(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsdisp_t *jsthis, *cmp_func = NULL;
    jsval_t *vtab, **sorttab = NULL;
    UINT32 length;
    DWORD i;
    HRESULT hres = S_OK;

    TRACE("\n");

    hres = get_length(ctx, vthis, &jsthis, &length);
    if(FAILED(hres))
        return hres;

    if(argc >= 1) {
        if(is_object_instance(argv[0])) {
            if(argc > 1 && ctx->version < SCRIPTLANGUAGEVERSION_ES5) {
                WARN("invalid arg_cnt %d\n", argc);
                hres = JS_E_JSCRIPT_EXPECTED;
                goto done;
            }
            cmp_func = iface_to_jsdisp(get_object(argv[0]));
            if(!cmp_func || !is_class(cmp_func, JSCLASS_FUNCTION)) {
                WARN("cmp_func is not a function\n");
                if(cmp_func)
                    jsdisp_release(cmp_func);
                hres = JS_E_JSCRIPT_EXPECTED;
                goto done;
            }
        }else if(ctx->version >= SCRIPTLANGUAGEVERSION_ES5 ? !is_undefined(argv[0]) :
                 (!is_null(argv[0]) || is_null_disp(argv[0]))) {
            WARN("invalid arg %s\n", debugstr_jsval(argv[0]));
            hres = JS_E_JSCRIPT_EXPECTED;
            goto done;
        }
    }

    if(!length) {
        if(cmp_func)
            jsdisp_release(cmp_func);
        if(r)
            *r = jsval_obj(jsdisp_addref(jsthis));
        goto done;
    }

    vtab = calloc(length, sizeof(*vtab));
    if(vtab) {
        for(i=0; i<length; i++) {
            hres = jsdisp_get_idx(jsthis, i, vtab+i);
            if(hres == DISP_E_UNKNOWNNAME) {
                vtab[i] = jsval_undefined();
                hres = S_OK;
            } else if(FAILED(hres)) {
                WARN("Could not get elem %ld: %08lx\n", i, hres);
                break;
            }
        }
    }else {
        hres = E_OUTOFMEMORY;
    }

    if(SUCCEEDED(hres)) {
        sorttab = malloc(length*2*sizeof(*sorttab));
        if(!sorttab)
            hres = E_OUTOFMEMORY;
    }

    /* merge-sort */
    if(SUCCEEDED(hres)) {
        jsval_t *tmpv, **tmpbuf;
        INT cmp;

        tmpbuf = sorttab + length;
        for(i=0; i < length; i++)
            sorttab[i] = vtab+i;

        for(i=0; i < length/2; i++) {
            hres = sort_cmp(ctx, cmp_func, *sorttab[2*i+1], *sorttab[2*i], &cmp);
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

                    memcpy(tmpbuf, sorttab+i, k*sizeof(jsval_t*));

                    while(a < k && b < bend) {
                        hres = sort_cmp(ctx, cmp_func, *tmpbuf[a], *sorttab[i+k+b], &cmp);
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
                        memcpy(sorttab+i+a+b, tmpbuf+a, (k-a)*sizeof(jsval_t*));
                }

                if(FAILED(hres))
                    break;
            }
        }

        for(i=0; SUCCEEDED(hres) && i < length; i++)
            hres = jsdisp_propput_idx(jsthis, i, *sorttab[i]);
    }

    if(vtab) {
        for(i=0; i < length; i++)
            jsval_release(vtab[i]);
        free(vtab);
    }
    free(sorttab);
    if(cmp_func)
        jsdisp_release(cmp_func);

    if(FAILED(hres))
        goto done;

    if(r)
        *r = jsval_obj(jsdisp_addref(jsthis));
done:
    jsdisp_release(jsthis);
    return hres;
}

/* ECMA-262 3rd Edition    15.4.4.12 */
static HRESULT Array_splice(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    UINT32 length, start=0, delete_cnt=0, i, add_args = 0;
    jsdisp_t *ret_array = NULL, *jsthis;
    jsval_t val;
    double d;
    int n;
    HRESULT hres = S_OK;

    TRACE("\n");

    hres = get_length(ctx, vthis, &jsthis, &length);
    if(FAILED(hres))
        return hres;

    if(argc) {
        hres = to_integer(ctx, argv[0], &d);
        if(FAILED(hres))
            goto done;

        if(is_int32(d)) {
            if((n = d) >= 0)
                start = min(n, length);
            else
                start = -n > length ? 0 : length + n;
        }else {
            start = d < 0.0 ? 0 : length;
        }
    }

    if(argc >= 2) {
        hres = to_integer(ctx, argv[1], &d);
        if(FAILED(hres))
            goto done;

        if(is_int32(d)) {
            if((n = d) > 0)
                delete_cnt = min(n, length-start);
        }else if(d > 0.0) {
            delete_cnt = length-start;
        }

        add_args = argc-2;
    } else if (argc && ctx->version >= SCRIPTLANGUAGEVERSION_ES5) {
        delete_cnt = length-start;
    }

    if(r) {
        hres = create_array(ctx, 0, &ret_array);
        if(FAILED(hres))
            goto done;

        for(i=0; SUCCEEDED(hres) && i < delete_cnt; i++) {
            hres = jsdisp_get_idx(jsthis, start+i, &val);
            if(hres == DISP_E_UNKNOWNNAME) {
                hres = S_OK;
            }else if(SUCCEEDED(hres)) {
                hres = jsdisp_propput_idx(ret_array, i, val);
                jsval_release(val);
            }
        }

        if(SUCCEEDED(hres))
            hres = jsdisp_propput_name(ret_array, L"length", jsval_number(delete_cnt));
    }

    if(add_args < delete_cnt) {
        for(i = start; SUCCEEDED(hres) && i < length-delete_cnt; i++) {
            hres = jsdisp_get_idx(jsthis, i+delete_cnt, &val);
            if(hres == DISP_E_UNKNOWNNAME) {
                hres = jsdisp_delete_idx(jsthis, i+add_args);
            }else if(SUCCEEDED(hres)) {
                hres = jsdisp_propput_idx(jsthis, i+add_args, val);
                jsval_release(val);
            }
        }

        for(i=length; SUCCEEDED(hres) && i != length-delete_cnt+add_args; i--)
            hres = jsdisp_delete_idx(jsthis, i-1);
    }else if(add_args > delete_cnt) {
        for(i=length-delete_cnt; SUCCEEDED(hres) && i != start; i--) {
            hres = jsdisp_get_idx(jsthis, i+delete_cnt-1, &val);
            if(hres == DISP_E_UNKNOWNNAME) {
                hres = jsdisp_delete_idx(jsthis, i+add_args-1);
            }else if(SUCCEEDED(hres)) {
                hres = jsdisp_propput_idx(jsthis, i+add_args-1, val);
                jsval_release(val);
            }
        }
    }

    for(i=0; SUCCEEDED(hres) && i < add_args; i++)
        hres = jsdisp_propput_idx(jsthis, start+i, argv[i+2]);

    if(SUCCEEDED(hres))
        hres = jsdisp_propput_name(jsthis, L"length", jsval_number(length-delete_cnt+add_args));

    if(FAILED(hres)) {
        if(ret_array)
            jsdisp_release(ret_array);
        goto done;
    }

    if(r)
        *r = jsval_obj(ret_array);
done:
    jsdisp_release(jsthis);
    return hres;
}

/* ECMA-262 3rd Edition    15.4.4.2 */
static HRESULT Array_toString(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    ArrayInstance *array;

    TRACE("\n");

    array = array_this(vthis);
    if(!array)
        return JS_E_ARRAY_EXPECTED;

    return array_join(ctx, &array->dispex, array->length, L",", 1, to_string, r);
}

static HRESULT to_locale_string(script_ctx_t *ctx, jsval_t val, jsstr_t **str)
{
    jsdisp_t *jsdisp;
    IDispatch *obj;
    HRESULT hres;

    switch(jsval_type(val)) {
    case JSV_OBJECT:
        hres = disp_call_name(ctx, get_object(val), L"toLocaleString", DISPATCH_METHOD, 0, NULL, &val);
        if(FAILED(hres)) {
            if(hres == JS_E_INVALID_PROPERTY && ctx->version >= SCRIPTLANGUAGEVERSION_ES5)
                hres = JS_E_FUNCTION_EXPECTED;
            return hres;
        }
        break;
    case JSV_NUMBER:
        if(ctx->version >= SCRIPTLANGUAGEVERSION_ES5)
            return localize_number(ctx, get_number(val), FALSE, str);
        /* fall through */
    default:
        if(ctx->version >= SCRIPTLANGUAGEVERSION_ES5)
            break;

        hres = to_object(ctx, val, &obj);
        if(FAILED(hres))
            return hres;

        jsdisp = as_jsdisp(obj);
        hres = jsdisp_call_name(jsdisp, L"toLocaleString", DISPATCH_METHOD, 0, NULL, &val);
        jsdisp_release(jsdisp);
        if(FAILED(hres))
            return hres;
        break;
    }

    return to_string(ctx, val, str);
}

static HRESULT Array_toLocaleString(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsdisp_t *jsthis;
    UINT32 length;
    HRESULT hres;
    WCHAR buf[5];
    int len;

    TRACE("\n");

    if(ctx->version < SCRIPTLANGUAGEVERSION_ES5) {
        ArrayInstance *array = array_this(vthis);
        if(!array)
            return JS_E_ARRAY_EXPECTED;
        jsthis = jsdisp_addref(&array->dispex);
        length = array->length;
    }else {
        hres = get_length(ctx, vthis, &jsthis, &length);
        if(FAILED(hres))
            return hres;
    }

    if(!(len = GetLocaleInfoW(ctx->lcid, LOCALE_SLIST, buf, ARRAY_SIZE(buf) - 1))) {
        buf[len++] = ',';
        len++;
    }
    buf[len - 1] = ' ';
    buf[len] = '\0';

    hres = array_join(ctx, jsthis, length, buf, len, to_locale_string, r);
    jsdisp_release(jsthis);
    return hres;
}

static HRESULT Array_every(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsval_t context_this = jsval_undefined();
    jsval_t value, args[3], res;
    BOOL boolval, ret = TRUE;
    IDispatch *callback;
    unsigned length, i;
    jsdisp_t *jsthis;
    HRESULT hres;

    TRACE("\n");

    hres = get_length(ctx, vthis, &jsthis, &length);
    if(FAILED(hres))
        return hres;

    /* FIXME: check IsCallable */
    if(!argc || !is_object_instance(argv[0])) {
        FIXME("Invalid arg %s\n", debugstr_jsval(argc ? argv[0] : jsval_undefined()));
        hres = E_INVALIDARG;
        goto done;
    }
    callback = get_object(argv[0]);

    if(argc > 1)
        context_this = argv[1];

    for(i = 0; i < length; i++) {
        hres = jsdisp_get_idx(jsthis, i, &value);
        if(FAILED(hres)) {
            if(hres == DISP_E_UNKNOWNNAME)
                continue;
            goto done;
        }
        args[0] = value;
        args[1] = jsval_number(i);
        args[2] = jsval_obj(jsthis);
        hres = disp_call_value(ctx, callback, context_this, DISPATCH_METHOD, ARRAY_SIZE(args), args, &res);
        jsval_release(value);
        if(FAILED(hres))
            goto done;

        hres = to_boolean(res, &boolval);
        jsval_release(res);
        if(FAILED(hres))
            goto done;
        if(!boolval) {
            ret = FALSE;
            break;
        }
    }

    if(r)
        *r = jsval_bool(ret);
    hres = S_OK;
done:
    jsdisp_release(jsthis);
    return hres;
}

static HRESULT Array_filter(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsval_t context_this = jsval_undefined();
    jsval_t value, args[3], res;
    unsigned length, i, j = 0;
    jsdisp_t *jsthis, *arr;
    IDispatch *callback;
    HRESULT hres;
    BOOL boolval;

    TRACE("\n");

    hres = get_length(ctx, vthis, &jsthis, &length);
    if(FAILED(hres))
        return hres;

    /* FIXME: check IsCallable */
    if(!argc || !is_object_instance(argv[0])) {
        FIXME("Invalid arg %s\n", debugstr_jsval(argc ? argv[0] : jsval_undefined()));
        hres = E_INVALIDARG;
        goto done;
    }
    callback = get_object(argv[0]);

    if(argc > 1)
        context_this = argv[1];

    hres = create_array(ctx, 0, &arr);
    if(FAILED(hres))
        goto done;

    for(i = 0; i < length; i++) {
        hres = jsdisp_get_idx(jsthis, i, &value);
        if(FAILED(hres)) {
            if(hres == DISP_E_UNKNOWNNAME) {
                hres = S_OK;
                continue;
            }
            break;
        }
        args[0] = value;
        args[1] = jsval_number(i);
        args[2] = jsval_obj(jsthis);
        hres = disp_call_value(ctx, callback, context_this, DISPATCH_METHOD, ARRAY_SIZE(args), args, &res);
        if(SUCCEEDED(hres)) {
            hres = to_boolean(res, &boolval);
            jsval_release(res);
            if(SUCCEEDED(hres) && boolval)
                hres = jsdisp_propput_idx(arr, j++, value);
        }
        jsval_release(value);
        if(FAILED(hres))
            break;
    }

    if(FAILED(hres)) {
        jsdisp_release(arr);
        goto done;
    }
    set_length(arr, j);

    if(r)
        *r = jsval_obj(arr);
    else
        jsdisp_release(arr);
done:
    jsdisp_release(jsthis);
    return hres;
}

static HRESULT Array_forEach(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsval_t context_this = jsval_undefined();
    jsval_t value, args[3], res;
    IDispatch *callback;
    jsdisp_t *jsthis;
    unsigned length, i;
    HRESULT hres;

    TRACE("\n");

    hres = get_length(ctx, vthis, &jsthis, &length);
    if(FAILED(hres))
        return hres;

    /* Fixme check IsCallable */
    if(!argc || !is_object_instance(argv[0])) {
        FIXME("Invalid arg %s\n", debugstr_jsval(argc ? argv[0] : jsval_undefined()));
        hres = E_INVALIDARG;
        goto done;
    }
    callback = get_object(argv[0]);

    if(argc > 1)
        context_this = argv[1];

    for(i = 0; i < length; i++) {
        hres = jsdisp_get_idx(jsthis, i, &value);
        if(hres == DISP_E_UNKNOWNNAME)
            continue;
        if(FAILED(hres))
            goto done;

        args[0] = value;
        args[1] = jsval_number(i);
        args[2] = jsval_obj(jsthis);
        hres = disp_call_value(ctx, callback, context_this, DISPATCH_METHOD, ARRAY_SIZE(args), args, &res);
        jsval_release(value);
        if(FAILED(hres))
            goto done;
        jsval_release(res);
    }

    if(r) *r = jsval_undefined();
    hres = S_OK;
done:
    jsdisp_release(jsthis);
    return hres;
}

static HRESULT Array_indexOf(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsdisp_t *jsthis;
    unsigned length, i, from = 0;
    jsval_t search, value;
    BOOL eq;
    HRESULT hres;

    TRACE("\n");

    hres = get_length(ctx, vthis, &jsthis, &length);
    if(FAILED(hres))
        return hres;
    if(!length) {
        if(r) *r = jsval_number(-1);
        goto done;
    }

    search = argc ? argv[0] : jsval_undefined();

    if(argc > 1) {
        double from_arg;

        hres = to_integer(ctx, argv[1], &from_arg);
        if(FAILED(hres))
            goto done;

        if(from_arg >= 0)
            from = min(from_arg, length);
        else
            from = max(from_arg + length, 0);
    }

    for(i = from; i < length; i++) {
        hres = jsdisp_get_idx(jsthis, i, &value);
        if(hres == DISP_E_UNKNOWNNAME)
            continue;
        if(FAILED(hres))
            goto done;

        hres = jsval_strict_equal(value, search, &eq);
        jsval_release(value);
        if(FAILED(hres))
            goto done;
        if(eq) {
            if(r) *r = jsval_number(i);
            goto done;
        }
    }

    if(r) *r = jsval_number(-1);
    hres = S_OK;
done:
    jsdisp_release(jsthis);
    return hres;
}

static HRESULT Array_lastIndexOf(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsval_t search, value;
    unsigned i, length;
    jsdisp_t *jsthis;
    HRESULT hres;
    BOOL eq;

    TRACE("\n");

    hres = get_length(ctx, vthis, &jsthis, &length);
    if(FAILED(hres))
        return hres;
    if(!length)
        goto notfound;

    search = argc ? argv[0] : jsval_undefined();

    i = length - 1;
    if(argc > 1) {
        double from_arg;

        hres = to_integer(ctx, argv[1], &from_arg);
        if(FAILED(hres))
            goto done;

        if(from_arg >= 0.0)
            i = min(from_arg, i);
        else {
            from_arg += length;
            if(from_arg < 0.0)
                goto notfound;
            i = from_arg;
        }
    }

    do {
        hres = jsdisp_get_idx(jsthis, i, &value);
        if(hres == DISP_E_UNKNOWNNAME)
            continue;
        if(FAILED(hres))
            goto done;

        hres = jsval_strict_equal(value, search, &eq);
        jsval_release(value);
        if(FAILED(hres))
            goto done;
        if(eq) {
            if(r) *r = jsval_number(i);
            goto done;
        }
    } while(i--);

notfound:
    if(r) *r = jsval_number(-1);
    hres = S_OK;
done:
    jsdisp_release(jsthis);
    return hres;
}

static HRESULT Array_map(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv, jsval_t *r)
{
    jsval_t context_this = jsval_undefined();
    jsval_t callback_args[3], mapped_value;
    jsdisp_t *jsthis, *array;
    IDispatch *callback;
    UINT32 length, k;
    HRESULT hres;

    TRACE("\n");

    hres = get_length(ctx, vthis, &jsthis, &length);
    if(FAILED(hres)) {
        FIXME("Could not get length\n");
        return hres;
    }

    /* FIXME: check IsCallable */
    if(!argc || !is_object_instance(argv[0])) {
        FIXME("Invalid arg %s\n", debugstr_jsval(argc ? argv[0] : jsval_undefined()));
        hres = E_INVALIDARG;
        goto done;
    }
    callback = get_object(argv[0]);

    if(argc > 1)
        context_this = argv[1];

    hres = create_array(ctx, 0, &array);
    if(FAILED(hres))
        goto done;

    for(k = 0; k < length; k++) {
        hres = jsdisp_get_idx(jsthis, k, &callback_args[0]);
        if(hres == DISP_E_UNKNOWNNAME) {
            hres = S_OK;
            continue;
        }
        if(FAILED(hres))
            break;

        callback_args[1] = jsval_number(k);
        callback_args[2] = jsval_obj(jsthis);
        hres = disp_call_value(ctx, callback, context_this, DISPATCH_METHOD, 3, callback_args, &mapped_value);
        jsval_release(callback_args[0]);
        if(FAILED(hres))
            break;

        hres = jsdisp_propput_idx(array, k, mapped_value);
        if(FAILED(hres))
            break;
    }

    if(SUCCEEDED(hres) && r)
        *r = jsval_obj(array);
    else
        jsdisp_release(array);
done:
    jsdisp_release(jsthis);
    return hres;
}

static HRESULT Array_reduce(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv, jsval_t *r)
{
    jsval_t callback_args[4], acc, new_acc;
    BOOL have_value = FALSE;
    IDispatch *callback;
    jsdisp_t *jsthis;
    UINT32 length, k;
    HRESULT hres;

    TRACE("\n");

    hres = get_length(ctx, vthis, &jsthis, &length);
    if(FAILED(hres)) {
        FIXME("Could not get length\n");
        return hres;
    }

    /* Fixme check IsCallable */
    if(!argc || !is_object_instance(argv[0])) {
        FIXME("Invalid arg %s\n", debugstr_jsval(argc ? argv[0] : jsval_undefined()));
        hres = E_INVALIDARG;
        goto done;
    }
    callback = get_object(argv[0]);

    if(argc > 1) {
        have_value = TRUE;
        hres = jsval_copy(argv[1], &acc);
        if(FAILED(hres))
            goto done;
    }

    for(k = 0; k < length; k++) {
        hres = jsdisp_get_idx(jsthis, k, &callback_args[1]);
        if(hres == DISP_E_UNKNOWNNAME) {
            hres = S_OK;
            continue;
        }
        if(FAILED(hres))
            break;

        if(!have_value) {
            have_value = TRUE;
            acc = callback_args[1];
            continue;
        }

        callback_args[0] = acc;
        callback_args[2] = jsval_number(k);
        callback_args[3] = jsval_obj(jsthis);
        hres = disp_call_value(ctx, callback, jsval_undefined(), DISPATCH_METHOD, ARRAY_SIZE(callback_args), callback_args, &new_acc);
        jsval_release(callback_args[1]);
        if(FAILED(hres))
            break;

        jsval_release(acc);
        acc = new_acc;
    }

    if(SUCCEEDED(hres) && !have_value) {
        WARN("No array element\n");
        hres = JS_E_INVALID_ACTION;
    }

    if(SUCCEEDED(hres) && r)
        *r = acc;
    else if(have_value)
        jsval_release(acc);
done:
    jsdisp_release(jsthis);
    return hres;
}

static HRESULT Array_some(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsval_t context_this = jsval_undefined();
    jsval_t value, args[3], res;
    BOOL boolval, ret = FALSE;
    IDispatch *callback;
    unsigned length, i;
    jsdisp_t *jsthis;
    HRESULT hres;

    TRACE("\n");

    hres = get_length(ctx, vthis, &jsthis, &length);
    if(FAILED(hres))
        return hres;

    /* FIXME: check IsCallable */
    if(!argc || !is_object_instance(argv[0])) {
        FIXME("Invalid arg %s\n", debugstr_jsval(argc ? argv[0] : jsval_undefined()));
        hres = E_INVALIDARG;
        goto done;
    }
    callback = get_object(argv[0]);

    if(argc > 1)
        context_this = argv[1];

    for(i = 0; i < length; i++) {
        hres = jsdisp_get_idx(jsthis, i, &value);
        if(FAILED(hres)) {
            if(hres == DISP_E_UNKNOWNNAME)
                continue;
            goto done;
        }
        args[0] = value;
        args[1] = jsval_number(i);
        args[2] = jsval_obj(jsthis);
        hres = disp_call_value(ctx, callback, context_this, DISPATCH_METHOD, ARRAY_SIZE(args), args, &res);
        jsval_release(value);
        if(FAILED(hres))
            goto done;

        hres = to_boolean(res, &boolval);
        jsval_release(res);
        if(FAILED(hres))
            goto done;
        if(boolval) {
            ret = TRUE;
            break;
        }
    }

    if(r)
        *r = jsval_bool(ret);
    hres = S_OK;
done:
    jsdisp_release(jsthis);
    return hres;
}

/* ECMA-262 3rd Edition    15.4.4.13 */
static HRESULT Array_unshift(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsdisp_t *jsthis;
    WCHAR buf[14], *buf_end, *str;
    UINT32 i, length;
    jsval_t val;
    DISPID id;
    HRESULT hres;

    TRACE("\n");

    hres = get_length(ctx, vthis, &jsthis, &length);
    if(FAILED(hres))
        return hres;

    if(argc) {
        buf_end = buf + ARRAY_SIZE(buf)-1;
        *buf_end-- = 0;
        i = length;

        while(i--) {
            str = idx_to_str(i, buf_end);

            hres = jsdisp_get_id(jsthis, str, 0, &id);
            if(SUCCEEDED(hres)) {
                hres = jsdisp_propget(jsthis, id, &val);
                if(FAILED(hres))
                    goto done;

                hres = jsdisp_propput_idx(jsthis, i+argc, val);
                jsval_release(val);
            }else if(hres == DISP_E_UNKNOWNNAME) {
                hres = IDispatchEx_DeleteMemberByDispID(to_dispex(jsthis), id);
            }
        }

        if(FAILED(hres))
            goto done;
    }

    for(i=0; i<argc; i++) {
        hres = jsdisp_propput_idx(jsthis, i, argv[i]);
        if(FAILED(hres))
            goto done;
    }

    if(argc) {
        length += argc;
        hres = set_length(jsthis, length);
        if(FAILED(hres))
            goto done;
    }

    if(r)
        *r = ctx->version < 2 ? jsval_undefined() : jsval_number(length);
    hres = S_OK;
done:
    jsdisp_release(jsthis);
    return hres;
}

static void Array_on_put(jsdisp_t *dispex, const WCHAR *name)
{
    ArrayInstance *array = array_from_jsdisp(dispex);
    const WCHAR *ptr = name;
    DWORD id = 0;

    if(!is_digit(*ptr))
        return;

    while(*ptr && is_digit(*ptr)) {
        id = id*10 + (*ptr-'0');
        ptr++;
    }

    if(*ptr)
        return;

    if(id >= array->length)
        array->length = id+1;
}

static const builtin_prop_t Array_props[] = {
    {L"concat",                Array_concat,               PROPF_METHOD|1},
    {L"every",                 Array_every,                PROPF_METHOD|PROPF_ES5|1},
    {L"filter",                Array_filter,               PROPF_METHOD|PROPF_ES5|1},
    {L"forEach",               Array_forEach,              PROPF_METHOD|PROPF_ES5|1},
    {L"indexOf",               Array_indexOf,              PROPF_METHOD|PROPF_ES5|1},
    {L"join",                  Array_join,                 PROPF_METHOD|1},
    {L"lastIndexOf",           Array_lastIndexOf,          PROPF_METHOD|PROPF_ES5|1},
    {L"length",                NULL,0,                     Array_get_length, Array_set_length},
    {L"map",                   Array_map,                  PROPF_METHOD|PROPF_ES5|1},
    {L"pop",                   Array_pop,                  PROPF_METHOD},
    {L"push",                  Array_push,                 PROPF_METHOD|1},
    {L"reduce",                Array_reduce,               PROPF_METHOD|PROPF_ES5|1},
    {L"reverse",               Array_reverse,              PROPF_METHOD},
    {L"shift",                 Array_shift,                PROPF_METHOD},
    {L"slice",                 Array_slice,                PROPF_METHOD|2},
    {L"some",                  Array_some,                 PROPF_METHOD|PROPF_ES5|1},
    {L"sort",                  Array_sort,                 PROPF_METHOD|1},
    {L"splice",                Array_splice,               PROPF_METHOD|2},
    {L"toLocaleString",        Array_toLocaleString,       PROPF_METHOD},
    {L"toString",              Array_toString,             PROPF_METHOD},
    {L"unshift",               Array_unshift,              PROPF_METHOD|1},
};

static const builtin_info_t Array_info = {
    .class      = JSCLASS_ARRAY,
    .props_cnt  = ARRAY_SIZE(Array_props),
    .props      = Array_props,
    .on_put     = Array_on_put,
};

static const builtin_prop_t ArrayInst_props[] = {
    {L"length",                NULL,0,                     Array_get_length, Array_set_length}
};

static const builtin_info_t ArrayInst_info = {
    .class      = JSCLASS_ARRAY,
    .props_cnt  = ARRAY_SIZE(ArrayInst_props),
    .props      = ArrayInst_props,
    .on_put     = Array_on_put,
};

/* ECMA-262 5.1 Edition    15.4.3.2 */
static HRESULT ArrayConstr_isArray(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv, jsval_t *r)
{
    jsdisp_t *obj;

    TRACE("\n");

    if(!argc || !is_object_instance(argv[0])) {
        if(r) *r = jsval_bool(FALSE);
        return S_OK;
    }

    obj = to_jsdisp(get_object(argv[0]));
    if(r) *r = jsval_bool(obj && is_class(obj, JSCLASS_ARRAY));
    return S_OK;
}

static HRESULT ArrayConstr_value(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsdisp_t *obj;
    DWORD i;
    HRESULT hres;

    TRACE("\n");

    switch(flags) {
    case DISPATCH_METHOD:
    case DISPATCH_CONSTRUCT: {
        if(argc == 1 && is_number(argv[0])) {
            double n = get_number(argv[0]);

            if(n < 0 || !is_int32(n))
                return JS_E_INVALID_LENGTH;
            if(!r)
                return S_OK;

            hres = create_array(ctx, n, &obj);
            if(FAILED(hres))
                return hres;

            *r = jsval_obj(obj);
            return S_OK;
        }

        if(!r)
            return S_OK;
        hres = create_array(ctx, argc, &obj);
        if(FAILED(hres))
            return hres;

        for(i=0; i < argc; i++) {
            hres = jsdisp_propput_idx(obj, i, argv[i]);
            if(FAILED(hres))
                break;
        }
        if(FAILED(hres)) {
            jsdisp_release(obj);
            return hres;
        }

        *r = jsval_obj(obj);
        break;
    }
    default:
        FIXME("unimplemented flags: %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT alloc_array(script_ctx_t *ctx, jsdisp_t *object_prototype, ArrayInstance **ret)
{
    ArrayInstance *array;
    HRESULT hres;

    array = calloc(1, sizeof(ArrayInstance));
    if(!array)
        return E_OUTOFMEMORY;

    if(object_prototype)
        hres = init_dispex(&array->dispex, ctx, &Array_info, object_prototype);
    else
        hres = init_dispex_from_constr(&array->dispex, ctx, &ArrayInst_info, ctx->array_constr);

    if(FAILED(hres)) {
        free(array);
        return hres;
    }

    *ret = array;
    return S_OK;
}

static const builtin_prop_t ArrayConstr_props[] = {
    {L"isArray",    ArrayConstr_isArray,    PROPF_ES5|PROPF_METHOD|1}
};

static const builtin_info_t ArrayConstr_info = {
    .class     = JSCLASS_FUNCTION,
    .call      = Function_value,
    .props_cnt = ARRAY_SIZE(ArrayConstr_props),
    .props     = ArrayConstr_props,
};

HRESULT create_array_constr(script_ctx_t *ctx, jsdisp_t *object_prototype, jsdisp_t **ret)
{
    ArrayInstance *array;
    HRESULT hres;

    hres = alloc_array(ctx, object_prototype, &array);
    if(FAILED(hres))
        return hres;

    hres = create_builtin_constructor(ctx, ArrayConstr_value, L"Array", &ArrayConstr_info, PROPF_CONSTR|1, &array->dispex, ret);

    jsdisp_release(&array->dispex);
    return hres;
}

HRESULT create_array(script_ctx_t *ctx, DWORD length, jsdisp_t **ret)
{
    ArrayInstance *array;
    HRESULT hres;

    hres = alloc_array(ctx, NULL, &array);
    if(FAILED(hres))
        return hres;

    array->length = length;

    *ret = &array->dispex;
    return S_OK;
}
