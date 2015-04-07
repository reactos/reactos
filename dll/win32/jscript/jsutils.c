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

WINE_DECLARE_DEBUG_CHANNEL(heap);

const char *debugstr_jsval(const jsval_t v)
{
    switch(jsval_type(v)) {
    case JSV_UNDEFINED:
        return "undefined";
    case JSV_NULL:
        return "null";
    case JSV_OBJECT:
        return wine_dbg_sprintf("obj(%p)", get_object(v));
    case JSV_STRING:
        return wine_dbg_sprintf("str(%s)", debugstr_jsstr(get_string(v)));
    case JSV_NUMBER:
        return wine_dbg_sprintf("%lf", get_number(v));
    case JSV_BOOL:
        return get_bool(v) ? "true" : "false";
    case JSV_VARIANT:
        return debugstr_variant(get_variant(v));
    }

    assert(0);
    return NULL;
}

#define MIN_BLOCK_SIZE  128
#define ARENA_FREE_FILLER  0xaa

static inline DWORD block_size(DWORD block)
{
    return MIN_BLOCK_SIZE << block;
}

void heap_pool_init(heap_pool_t *heap)
{
    memset(heap, 0, sizeof(*heap));
    list_init(&heap->custom_blocks);
}

void *heap_pool_alloc(heap_pool_t *heap, DWORD size)
{
    struct list *list;
    void *tmp;

    if(!heap->block_cnt) {
        if(!heap->blocks) {
            heap->blocks = heap_alloc(sizeof(void*));
            if(!heap->blocks)
                return NULL;
        }

        tmp = heap_alloc(block_size(0));
        if(!tmp)
            return NULL;

        heap->blocks[0] = tmp;
        heap->block_cnt = 1;
    }

    if(heap->offset + size <= block_size(heap->last_block)) {
        tmp = ((BYTE*)heap->blocks[heap->last_block])+heap->offset;
        heap->offset += size;
        return tmp;
    }

    if(size <= block_size(heap->last_block+1)) {
        if(heap->last_block+1 == heap->block_cnt) {
            tmp = heap_realloc(heap->blocks, (heap->block_cnt+1)*sizeof(void*));
            if(!tmp)
                return NULL;

            heap->blocks = tmp;
            heap->blocks[heap->block_cnt] = heap_alloc(block_size(heap->block_cnt));
            if(!heap->blocks[heap->block_cnt])
                return NULL;

            heap->block_cnt++;
        }

        heap->last_block++;
        heap->offset = size;
        return heap->blocks[heap->last_block];
    }

    list = heap_alloc(size + sizeof(struct list));
    if(!list)
        return NULL;

    list_add_head(&heap->custom_blocks, list);
    return list+1;
}

void *heap_pool_grow(heap_pool_t *heap, void *mem, DWORD size, DWORD inc)
{
    void *ret;

    if(mem == (BYTE*)heap->blocks[heap->last_block] + heap->offset-size
       && heap->offset+inc < block_size(heap->last_block)) {
        heap->offset += inc;
        return mem;
    }

    ret = heap_pool_alloc(heap, size+inc);
    if(ret) /* FIXME: avoid copying for custom blocks */
        memcpy(ret, mem, size);
    return ret;
}

void heap_pool_clear(heap_pool_t *heap)
{
    struct list *tmp;

    if(!heap)
        return;

    while((tmp = list_next(&heap->custom_blocks, &heap->custom_blocks))) {
        list_remove(tmp);
        heap_free(tmp);
    }

    if(WARN_ON(heap)) {
        DWORD i;

        for(i=0; i < heap->block_cnt; i++)
            memset(heap->blocks[i], ARENA_FREE_FILLER, block_size(i));
    }

    heap->last_block = heap->offset = 0;
    heap->mark = FALSE;
}

void heap_pool_free(heap_pool_t *heap)
{
    DWORD i;

    heap_pool_clear(heap);

    for(i=0; i < heap->block_cnt; i++)
        heap_free(heap->blocks[i]);
    heap_free(heap->blocks);

    heap_pool_init(heap);
}

heap_pool_t *heap_pool_mark(heap_pool_t *heap)
{
    if(heap->mark)
        return NULL;

    heap->mark = TRUE;
    return heap;
}

void jsval_release(jsval_t val)
{
    switch(jsval_type(val)) {
    case JSV_OBJECT:
        if(get_object(val))
            IDispatch_Release(get_object(val));
        break;
    case JSV_STRING:
        jsstr_release(get_string(val));
        break;
    case JSV_VARIANT:
        VariantClear(get_variant(val));
        heap_free(get_variant(val));
        break;
    default:
        break;
    }
}

static HRESULT jsval_variant(jsval_t *val, VARIANT *var)
{
    VARIANT *v;
    HRESULT hres;

    __JSVAL_TYPE(*val) = JSV_VARIANT;
    __JSVAL_VAR(*val) = v = heap_alloc(sizeof(VARIANT));
    if(!v)
        return E_OUTOFMEMORY;

    V_VT(v) = VT_EMPTY;
    hres = VariantCopy(v, var);
    if(FAILED(hres))
        heap_free(v);
    return hres;
}

HRESULT jsval_copy(jsval_t v, jsval_t *r)
{
    switch(jsval_type(v)) {
    case JSV_UNDEFINED:
    case JSV_NULL:
    case JSV_NUMBER:
    case JSV_BOOL:
        *r = v;
        return S_OK;
    case JSV_OBJECT:
        if(get_object(v))
            IDispatch_AddRef(get_object(v));
        *r = v;
        return S_OK;
    case JSV_STRING: {
        jsstr_addref(get_string(v));
        *r = v;
        return S_OK;
    }
    case JSV_VARIANT:
        return jsval_variant(r, get_variant(v));
    }

    assert(0);
    return E_FAIL;
}

HRESULT variant_to_jsval(VARIANT *var, jsval_t *r)
{
    if(V_VT(var) == (VT_VARIANT|VT_BYREF))
        var = V_VARIANTREF(var);

    switch(V_VT(var)) {
    case VT_EMPTY:
        *r = jsval_undefined();
        return S_OK;
    case VT_NULL:
        *r = jsval_null();
        return S_OK;
    case VT_BOOL:
        *r = jsval_bool(V_BOOL(var));
        return S_OK;
    case VT_I4:
        *r = jsval_number(V_I4(var));
        return S_OK;
    case VT_R8:
        *r = jsval_number(V_R8(var));
        return S_OK;
    case VT_BSTR: {
        jsstr_t *str;

        if(V_BSTR(var)) {
            str = jsstr_alloc_len(V_BSTR(var), SysStringLen(V_BSTR(var)));
            if(!str)
                return E_OUTOFMEMORY;
        }else {
            str = jsstr_null_bstr();
        }

        *r = jsval_string(str);
        return S_OK;
    }
    case VT_DISPATCH: {
        if(V_DISPATCH(var))
            IDispatch_AddRef(V_DISPATCH(var));
        *r = jsval_disp(V_DISPATCH(var));
        return S_OK;
    }
    case VT_I2:
        *r = jsval_number(V_I2(var));
        return S_OK;
    case VT_INT:
        *r = jsval_number(V_INT(var));
        return S_OK;
    case VT_UI4:
        *r = jsval_number(V_UI4(var));
        return S_OK;
    case VT_UNKNOWN:
        if(V_UNKNOWN(var)) {
            IDispatch *disp;
            HRESULT hres;

            hres = IUnknown_QueryInterface(V_UNKNOWN(var), &IID_IDispatch, (void**)&disp);
            if(SUCCEEDED(hres)) {
                *r = jsval_disp(disp);
                return S_OK;
            }
        }else {
            *r = jsval_disp(NULL);
            return S_OK;
        }
        /* fall through */
    default:
        return jsval_variant(r, var);
    }
}

HRESULT jsval_to_variant(jsval_t val, VARIANT *retv)
{
    switch(jsval_type(val)) {
    case JSV_UNDEFINED:
        V_VT(retv) = VT_EMPTY;
        return S_OK;
    case JSV_NULL:
        V_VT(retv) = VT_NULL;
        return S_OK;
    case JSV_OBJECT:
        V_VT(retv) = VT_DISPATCH;
        if(get_object(val))
            IDispatch_AddRef(get_object(val));
        V_DISPATCH(retv) = get_object(val);
        return S_OK;
    case JSV_STRING: {
        jsstr_t *str = get_string(val);

        V_VT(retv) = VT_BSTR;
        if(is_null_bstr(str)) {
            V_BSTR(retv) = NULL;
        }else {
            V_BSTR(retv) = SysAllocStringLen(NULL, jsstr_length(str));
            if(V_BSTR(retv))
                jsstr_flush(str, V_BSTR(retv));
            else
                return E_OUTOFMEMORY;
        }
        return S_OK;
    }
    case JSV_NUMBER: {
        double n = get_number(val);

        if(is_int32(n)) {
            V_VT(retv) = VT_I4;
            V_I4(retv) = n;
        }else {
            V_VT(retv) = VT_R8;
            V_R8(retv) = n;
        }

        return S_OK;
    }
    case JSV_BOOL:
        V_VT(retv) = VT_BOOL;
        V_BOOL(retv) = get_bool(val) ? VARIANT_TRUE : VARIANT_FALSE;
        return S_OK;
    case JSV_VARIANT:
        V_VT(retv) = VT_EMPTY;
        return VariantCopy(retv, get_variant(val));
    }

    assert(0);
    return E_FAIL;
}

/* ECMA-262 3rd Edition    9.1 */
HRESULT to_primitive(script_ctx_t *ctx, jsval_t val, jsval_t *ret, hint_t hint)
{
    if(is_object_instance(val)) {
        jsdisp_t *jsdisp;
        jsval_t prim;
        DISPID id;
        HRESULT hres;

        static const WCHAR toStringW[] = {'t','o','S','t','r','i','n','g',0};
        static const WCHAR valueOfW[] = {'v','a','l','u','e','O','f',0};

        if(!get_object(val)) {
            *ret = jsval_null();
            return S_OK;
        }

        jsdisp = iface_to_jsdisp((IUnknown*)get_object(val));
        if(!jsdisp)
            return disp_propget(ctx, get_object(val), DISPID_VALUE, ret);

        if(hint == NO_HINT)
            hint = is_class(jsdisp, JSCLASS_DATE) ? HINT_STRING : HINT_NUMBER;

        /* Native implementation doesn't throw TypeErrors, returns strange values */

        hres = jsdisp_get_id(jsdisp, hint == HINT_STRING ? toStringW : valueOfW, 0, &id);
        if(SUCCEEDED(hres)) {
            hres = jsdisp_call(jsdisp, id, DISPATCH_METHOD, 0, NULL, &prim);
            if(FAILED(hres)) {
                WARN("call error - forwarding exception\n");
                jsdisp_release(jsdisp);
                return hres;
            }else if(!is_object_instance(prim)) {
                jsdisp_release(jsdisp);
                *ret = prim;
                return S_OK;
            }else {
                IDispatch_Release(get_object(prim));
            }
        }

        hres = jsdisp_get_id(jsdisp, hint == HINT_STRING ? valueOfW : toStringW, 0, &id);
        if(SUCCEEDED(hres)) {
            hres = jsdisp_call(jsdisp, id, DISPATCH_METHOD, 0, NULL, &prim);
            if(FAILED(hres)) {
                WARN("call error - forwarding exception\n");
                jsdisp_release(jsdisp);
                return hres;
            }else if(!is_object_instance(prim)) {
                jsdisp_release(jsdisp);
                *ret = prim;
                return S_OK;
            }else {
                IDispatch_Release(get_object(prim));
            }
        }

        jsdisp_release(jsdisp);

        WARN("failed\n");
        return throw_type_error(ctx, JS_E_TO_PRIMITIVE, NULL);
    }

    return jsval_copy(val, ret);

}

/* ECMA-262 3rd Edition    9.2 */
HRESULT to_boolean(jsval_t val, BOOL *ret)
{
    switch(jsval_type(val)) {
    case JSV_UNDEFINED:
    case JSV_NULL:
        *ret = FALSE;
        return S_OK;
    case JSV_OBJECT:
        *ret = get_object(val) != NULL;
        return S_OK;
    case JSV_STRING:
        *ret = jsstr_length(get_string(val)) != 0;
        return S_OK;
    case JSV_NUMBER:
        *ret = !isnan(get_number(val)) && get_number(val);
        return S_OK;
    case JSV_BOOL:
        *ret = get_bool(val);
        return S_OK;
    case JSV_VARIANT:
        FIXME("unimplemented for variant %s\n", debugstr_variant(get_variant(val)));
        return E_NOTIMPL;
    }

    assert(0);
    return E_FAIL;
}

static int hex_to_int(WCHAR c)
{
    if('0' <= c && c <= '9')
        return c-'0';

    if('a' <= c && c <= 'f')
        return c-'a'+10;

    if('A' <= c && c <= 'F')
        return c-'A'+10;

    return -1;
}

/* ECMA-262 3rd Edition    9.3.1 */
static HRESULT str_to_number(jsstr_t *str, double *ret)
{
    const WCHAR *ptr;
    BOOL neg = FALSE;
    DOUBLE d = 0.0;

    static const WCHAR infinityW[] = {'I','n','f','i','n','i','t','y'};

    ptr = jsstr_flatten(str);
    if(!ptr)
        return E_OUTOFMEMORY;

    while(isspaceW(*ptr))
        ptr++;

    if(*ptr == '-') {
        neg = TRUE;
        ptr++;
    }else if(*ptr == '+') {
        ptr++;
    }

    if(!strncmpW(ptr, infinityW, sizeof(infinityW)/sizeof(WCHAR))) {
        ptr += sizeof(infinityW)/sizeof(WCHAR);
        while(*ptr && isspaceW(*ptr))
            ptr++;

        if(*ptr)
            *ret = NAN;
        else
            *ret = neg ? -INFINITY : INFINITY;
        return S_OK;
    }

    if(*ptr == '0' && ptr[1] == 'x') {
        DWORD l = 0;

        ptr += 2;
        while((l = hex_to_int(*ptr)) != -1) {
            d = d*16 + l;
            ptr++;
        }

        *ret = d;
        return S_OK;
    }

    while(isdigitW(*ptr))
        d = d*10 + (*ptr++ - '0');

    if(*ptr == 'e' || *ptr == 'E') {
        BOOL eneg = FALSE;
        LONG l = 0;

        ptr++;
        if(*ptr == '-') {
            ptr++;
            eneg = TRUE;
        }else if(*ptr == '+') {
            ptr++;
        }

        while(isdigitW(*ptr))
            l = l*10 + (*ptr++ - '0');
        if(eneg)
            l = -l;

        d *= pow(10, l);
    }else if(*ptr == '.') {
        DOUBLE dec = 0.1;

        ptr++;
        while(isdigitW(*ptr)) {
            d += dec * (*ptr++ - '0');
            dec *= 0.1;
        }
    }

    while(isspaceW(*ptr))
        ptr++;

    if(*ptr) {
        *ret = NAN;
        return S_OK;
    }

    if(neg)
        d = -d;

    *ret = d;
    return S_OK;
}

/* ECMA-262 3rd Edition    9.3 */
HRESULT to_number(script_ctx_t *ctx, jsval_t val, double *ret)
{
    switch(jsval_type(val)) {
    case JSV_UNDEFINED:
        *ret = NAN;
        return S_OK;
    case JSV_NULL:
        *ret = 0;
        return S_OK;
    case JSV_NUMBER:
        *ret = get_number(val);
        return S_OK;
    case JSV_STRING:
        return str_to_number(get_string(val), ret);
    case JSV_OBJECT: {
        jsval_t prim;
        HRESULT hres;

        hres = to_primitive(ctx, val, &prim, HINT_NUMBER);
        if(FAILED(hres))
            return hres;

        hres = to_number(ctx, prim, ret);
        jsval_release(prim);
        return hres;
    }
    case JSV_BOOL:
        *ret = get_bool(val) ? 1 : 0;
        return S_OK;
    case JSV_VARIANT:
        FIXME("unimplemented for variant %s\n", debugstr_variant(get_variant(val)));
        return E_NOTIMPL;
    };

    assert(0);
    return E_FAIL;
}

/* ECMA-262 3rd Edition    9.4 */
HRESULT to_integer(script_ctx_t *ctx, jsval_t v, double *ret)
{
    double n;
    HRESULT hres;

    hres = to_number(ctx, v, &n);
    if(FAILED(hres))
        return hres;

    if(isnan(n))
        *ret = 0;
    else
        *ret = n >= 0.0 ? floor(n) : -floor(-n);
    return S_OK;
}

/* ECMA-262 3rd Edition    9.5 */
HRESULT to_int32(script_ctx_t *ctx, jsval_t v, INT *ret)
{
    double n;
    HRESULT hres;

    hres = to_number(ctx, v, &n);
    if(FAILED(hres))
        return hres;

    *ret = isnan(n) || isinf(n) ? 0 : n;
    return S_OK;
}

/* ECMA-262 3rd Edition    9.6 */
HRESULT to_uint32(script_ctx_t *ctx, jsval_t val, DWORD *ret)
{
    INT32 n;
    HRESULT hres;

    hres = to_int32(ctx, val, &n);
    if(SUCCEEDED(hres))
        *ret = n;
    return hres;
}

static jsstr_t *int_to_string(int i)
{
    WCHAR buf[12], *p;
    BOOL neg = FALSE;

    if(!i) {
        static const WCHAR zeroW[] = {'0',0};
        return jsstr_alloc(zeroW);
    }

    if(i < 0) {
        neg = TRUE;
        i = -i;
    }

    p = buf + sizeof(buf)/sizeof(*buf)-1;
    *p-- = 0;
    while(i) {
        *p-- = i%10 + '0';
        i /= 10;
    }

    if(neg)
        *p = '-';
    else
        p++;

    return jsstr_alloc(p);
}

HRESULT double_to_string(double n, jsstr_t **str)
{
    const WCHAR InfinityW[] = {'-','I','n','f','i','n','i','t','y',0};

    if(isnan(n)) {
        *str = jsstr_nan();
    }else if(isinf(n)) {
        *str = jsstr_alloc(n<0 ? InfinityW : InfinityW+1);
    }else if(is_int32(n)) {
        *str = int_to_string(n);
    }else {
        VARIANT strv, v;
        HRESULT hres;

        /* FIXME: Don't use VariantChangeTypeEx */
        V_VT(&v) = VT_R8;
        V_R8(&v) = n;
        V_VT(&strv) = VT_EMPTY;
        hres = VariantChangeTypeEx(&strv, &v, MAKELCID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),SORT_DEFAULT), 0, VT_BSTR);
        if(FAILED(hres))
            return hres;

        *str = jsstr_alloc(V_BSTR(&strv));
        SysFreeString(V_BSTR(&strv));
    }

    return *str ? S_OK : E_OUTOFMEMORY;
}

/* ECMA-262 3rd Edition    9.8 */
HRESULT to_string(script_ctx_t *ctx, jsval_t val, jsstr_t **str)
{
    const WCHAR nullW[] = {'n','u','l','l',0};
    const WCHAR trueW[] = {'t','r','u','e',0};
    const WCHAR falseW[] = {'f','a','l','s','e',0};

    switch(jsval_type(val)) {
    case JSV_UNDEFINED:
        *str = jsstr_undefined();
        return S_OK;
    case JSV_NULL:
        *str = jsstr_alloc(nullW);
        break;
    case JSV_NUMBER:
        return double_to_string(get_number(val), str);
    case JSV_STRING:
        *str = jsstr_addref(get_string(val));
        break;
    case JSV_OBJECT: {
        jsval_t prim;
        HRESULT hres;

        hres = to_primitive(ctx, val, &prim, HINT_STRING);
        if(FAILED(hres))
            return hres;

        hres = to_string(ctx, prim, str);
        jsval_release(prim);
        return hres;
    }
    case JSV_BOOL:
        *str = jsstr_alloc(get_bool(val) ? trueW : falseW);
        break;
    default:
        FIXME("unsupported %s\n", debugstr_jsval(val));
        return E_NOTIMPL;
    }

    return *str ? S_OK : E_OUTOFMEMORY;
}

HRESULT to_flat_string(script_ctx_t *ctx, jsval_t val, jsstr_t **str, const WCHAR **ret_str)
{
    HRESULT hres;

    hres = to_string(ctx, val, str);
    if(FAILED(hres))
        return hres;

    *ret_str = jsstr_flatten(*str);
    if(!*ret_str) {
        jsstr_release(*str);
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

/* ECMA-262 3rd Edition    9.9 */
HRESULT to_object(script_ctx_t *ctx, jsval_t val, IDispatch **disp)
{
    jsdisp_t *dispex;
    HRESULT hres;

    switch(jsval_type(val)) {
    case JSV_STRING:
        hres = create_string(ctx, get_string(val), &dispex);
        if(FAILED(hres))
            return hres;

        *disp = to_disp(dispex);
        break;
    case JSV_NUMBER:
        hres = create_number(ctx, get_number(val), &dispex);
        if(FAILED(hres))
            return hres;

        *disp = to_disp(dispex);
        break;
    case JSV_OBJECT:
        if(get_object(val)) {
            *disp = get_object(val);
            IDispatch_AddRef(*disp);
        }else {
            jsdisp_t *obj;

            hres = create_object(ctx, NULL, &obj);
            if(FAILED(hres))
                return hres;

            *disp = to_disp(obj);
        }
        break;
    case JSV_BOOL:
        hres = create_bool(ctx, get_bool(val), &dispex);
        if(FAILED(hres))
            return hres;

        *disp = to_disp(dispex);
        break;
    case JSV_UNDEFINED:
    case JSV_NULL:
        WARN("object expected\n");
        return throw_type_error(ctx, JS_E_OBJECT_EXPECTED, NULL);
    case JSV_VARIANT:
        switch(V_VT(get_variant(val))) {
        case VT_ARRAY|VT_VARIANT:
            hres = create_vbarray(ctx, V_ARRAY(get_variant(val)), &dispex);
            if(FAILED(hres))
                return hres;

            *disp = to_disp(dispex);
            break;

        default:
            FIXME("Unsupported %s\n", debugstr_variant(get_variant(val)));
            return E_NOTIMPL;
        }
        break;
    }

    return S_OK;
}

HRESULT variant_change_type(script_ctx_t *ctx, VARIANT *dst, VARIANT *src, VARTYPE vt)
{
    jsval_t val;
    HRESULT hres;

    clear_ei(ctx);
    hres = variant_to_jsval(src, &val);
    if(FAILED(hres))
        return hres;

    switch(vt) {
    case VT_I2:
    case VT_I4: {
        INT i;

        hres = to_int32(ctx, val, &i);
        if(SUCCEEDED(hres)) {
            if(vt == VT_I4)
                V_I4(dst) = i;
            else
                V_I2(dst) = i;
        }
        break;
    }
    case VT_R8: {
        double n;
        hres = to_number(ctx, val, &n);
        if(SUCCEEDED(hres))
            V_R8(dst) = n;
        break;
    }
    case VT_R4: {
        double n;

        hres = to_number(ctx, val, &n);
        if(SUCCEEDED(hres))
            V_R4(dst) = n;
        break;
    }
    case VT_BOOL: {
        BOOL b;

        hres = to_boolean(val, &b);
        if(SUCCEEDED(hres))
            V_BOOL(dst) = b ? VARIANT_TRUE : VARIANT_FALSE;
        break;
    }
    case VT_BSTR: {
        jsstr_t *str;

        hres = to_string(ctx, val, &str);
        if(FAILED(hres))
            break;

        if(is_null_bstr(str)) {
            V_BSTR(dst) = NULL;
            break;
        }

        V_BSTR(dst) = SysAllocStringLen(NULL, jsstr_length(str));
        if(V_BSTR(dst))
            jsstr_flush(str, V_BSTR(dst));
        else
            hres = E_OUTOFMEMORY;
        break;
    }
    case VT_EMPTY:
        hres = V_VT(src) == VT_EMPTY ? S_OK : E_NOTIMPL;
        break;
    case VT_NULL:
        hres = V_VT(src) == VT_NULL ? S_OK : E_NOTIMPL;
        break;
    default:
        FIXME("vt %d not implemented\n", vt);
        hres = E_NOTIMPL;
    }

    jsval_release(val);
    if(FAILED(hres))
        return hres;

    V_VT(dst) = vt;
    return S_OK;
}

static inline JSCaller *impl_from_IServiceProvider(IServiceProvider *iface)
{
    return CONTAINING_RECORD(iface, JSCaller, IServiceProvider_iface);
}

static HRESULT WINAPI JSCaller_QueryInterface(IServiceProvider *iface, REFIID riid, void **ppv)
{
    JSCaller *This = impl_from_IServiceProvider(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IServiceProvider_iface;
    }else if(IsEqualGUID(&IID_IServiceProvider, riid)) {
        TRACE("(%p)->(IID_IServiceProvider %p)\n", This, ppv);
        *ppv = &This->IServiceProvider_iface;
    }else {
        WARN("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI JSCaller_AddRef(IServiceProvider *iface)
{
    JSCaller *This = impl_from_IServiceProvider(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI JSCaller_Release(IServiceProvider *iface)
{
    JSCaller *This = impl_from_IServiceProvider(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        assert(!This->ctx);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI JSCaller_QueryService(IServiceProvider *iface, REFGUID guidService,
        REFIID riid, void **ppv)
{
    JSCaller *This = impl_from_IServiceProvider(iface);

    if(IsEqualGUID(guidService, &SID_VariantConversion) && This->ctx && This->ctx->active_script) {
        TRACE("(%p)->(SID_VariantConversion)\n", This);
        return IActiveScript_QueryInterface(This->ctx->active_script, riid, ppv);
    }

    FIXME("(%p)->(%s %s %p)\n", This, debugstr_guid(guidService), debugstr_guid(riid), ppv);

    *ppv = NULL;
    return E_NOINTERFACE;
}

static const IServiceProviderVtbl ServiceProviderVtbl = {
    JSCaller_QueryInterface,
    JSCaller_AddRef,
    JSCaller_Release,
    JSCaller_QueryService
};

HRESULT create_jscaller(script_ctx_t *ctx)
{
    JSCaller *ret;

    ret = heap_alloc(sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IServiceProvider_iface.lpVtbl = &ServiceProviderVtbl;
    ret->ref = 1;
    ret->ctx = ctx;

    ctx->jscaller = ret;
    return S_OK;
}
