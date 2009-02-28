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

#include "config.h"
#include "wine/port.h"

#include <math.h>

#include "jscript.h"
#include "engine.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

const char *debugstr_variant(const VARIANT *v)
{
    switch(V_VT(v)) {
    case VT_EMPTY:
        return wine_dbg_sprintf("{VT_EMPTY}");
    case VT_NULL:
        return wine_dbg_sprintf("{VT_NULL}");
    case VT_I4:
        return wine_dbg_sprintf("{VT_I4: %d}", V_I4(v));
    case VT_R8:
        return wine_dbg_sprintf("{VT_R8: %lf}", V_R8(v));
    case VT_BSTR:
        return wine_dbg_sprintf("{VT_BSTR: %s}", debugstr_w(V_BSTR(v)));
    case VT_DISPATCH:
        return wine_dbg_sprintf("{VT_DISPATCH: %p}", V_DISPATCH(v));
    case VT_BOOL:
        return wine_dbg_sprintf("{VT_BOOL: %x}", V_BOOL(v));
    default:
        return wine_dbg_sprintf("{vt %d}", V_VT(v));
    }
}

#define MIN_BLOCK_SIZE  128

static inline DWORD block_size(DWORD block)
{
    return MIN_BLOCK_SIZE << block;
}

void jsheap_init(jsheap_t *heap)
{
    memset(heap, 0, sizeof(*heap));
    list_init(&heap->custom_blocks);
}

void *jsheap_alloc(jsheap_t *heap, DWORD size)
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

    if(heap->offset + size < block_size(heap->last_block)) {
        tmp = ((BYTE*)heap->blocks[heap->last_block])+heap->offset;
        heap->offset += size;
        return tmp;
    }

    if(size < block_size(heap->last_block+1)) {
        if(heap->last_block+1 == heap->block_cnt) {
            tmp = heap_realloc(heap->blocks, (heap->block_cnt+1)*sizeof(void*));
            if(!tmp)
                return NULL;
            heap->blocks = tmp;
        }

        tmp = heap_alloc(block_size(heap->block_cnt+1));
        if(!tmp)
            return NULL;

        heap->blocks[heap->block_cnt++] = tmp;

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

void *jsheap_grow(jsheap_t *heap, void *mem, DWORD size, DWORD inc)
{
    if(mem == (BYTE*)heap->blocks[heap->last_block] + heap->offset-size
       && heap->offset+inc < block_size(heap->last_block)) {
        heap->offset += inc;
        return mem;
    }

    return jsheap_alloc(heap, size+inc);
}

void jsheap_clear(jsheap_t *heap)
{
    struct list *tmp;

    if(!heap)
        return;

    while((tmp = list_next(&heap->custom_blocks, &heap->custom_blocks))) {
        list_remove(tmp);
        heap_free(tmp);
    }

    heap->last_block = heap->offset = 0;
}

void jsheap_free(jsheap_t *heap)
{
    DWORD i;

    jsheap_clear(heap);

    for(i=0; i < heap->block_cnt; i++)
        heap_free(heap->blocks[i]);
    heap_free(heap->blocks);

    jsheap_init(heap);
}

jsheap_t *jsheap_mark(jsheap_t *heap)
{
    if(heap->mark)
        return NULL;

    heap->mark = TRUE;
    return heap;
}

/* ECMA-262 3rd Edition    9.1 */
HRESULT to_primitive(script_ctx_t *ctx, VARIANT *v, jsexcept_t *ei, VARIANT *ret)
{
    switch(V_VT(v)) {
    case VT_EMPTY:
    case VT_NULL:
    case VT_BOOL:
    case VT_I4:
    case VT_R8:
        *ret = *v;
        break;
    case VT_BSTR:
        V_VT(ret) = VT_BSTR;
        V_BSTR(ret) = SysAllocString(V_BSTR(v));
        break;
    case VT_DISPATCH:
        return disp_propget(V_DISPATCH(v), DISPID_VALUE, ctx->lcid, ret, ei, NULL /*FIXME*/);
    default:
        FIXME("Unimplemented for vt %d\n", V_VT(v));
        return E_NOTIMPL;
    }

    return S_OK;
}

/* ECMA-262 3rd Edition    9.2 */
HRESULT to_boolean(VARIANT *v, VARIANT_BOOL *b)
{
    switch(V_VT(v)) {
    case VT_EMPTY:
    case VT_NULL:
        *b = VARIANT_FALSE;
        break;
    case VT_I4:
        *b = V_I4(v) ? VARIANT_TRUE : VARIANT_FALSE;
        break;
    case VT_R8:
        *b = V_R8(v) ? VARIANT_TRUE : VARIANT_FALSE;
        break;
    case VT_BSTR:
        *b = V_BSTR(v) && *V_BSTR(v) ? VARIANT_TRUE : VARIANT_FALSE;
        break;
    case VT_DISPATCH:
        *b = V_DISPATCH(v) ? VARIANT_TRUE : VARIANT_FALSE;
        break;
    case VT_BOOL:
        *b = V_BOOL(v);
        break;
    default:
        FIXME("unimplemented for vt %d\n", V_VT(v));
        return E_NOTIMPL;
    }

    return S_OK;
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
static HRESULT str_to_number(BSTR str, VARIANT *ret)
{
    const WCHAR *ptr = str;
    BOOL neg = FALSE;
    DOUBLE d = 0.0;

    static const WCHAR infinityW[] = {'I','n','f','i','n','i','t','y'};

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
            num_set_nan(ret);
        else
            num_set_inf(ret, !neg);
        return S_OK;
    }

    if(*ptr == '0' && ptr[1] == 'x') {
        DWORD l = 0;

        ptr += 2;
        while((l = hex_to_int(*ptr)) != -1) {
            d = d*16 + l;
            ptr++;
        }

        num_set_val(ret, d);
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
        num_set_nan(ret);
        return S_OK;
    }

    if(neg)
        d = -d;

    num_set_val(ret, d);
    return S_OK;
}

/* ECMA-262 3rd Edition    9.3 */
HRESULT to_number(script_ctx_t *ctx, VARIANT *v, jsexcept_t *ei, VARIANT *ret)
{
    switch(V_VT(v)) {
    case VT_EMPTY:
        num_set_nan(ret);
        break;
    case VT_NULL:
        V_VT(ret) = VT_I4;
        V_I4(ret) = 0;
        break;
    case VT_I4:
    case VT_R8:
        *ret = *v;
        break;
    case VT_BSTR:
        return str_to_number(V_BSTR(v), ret);
    case VT_DISPATCH: {
        VARIANT prim;
        HRESULT hres;

        hres = to_primitive(ctx, v, ei, &prim);
        if(FAILED(hres))
            return hres;

        hres = to_number(ctx, &prim, ei, ret);
        VariantClear(&prim);
        return hres;
    }
    case VT_BOOL:
        V_VT(ret) = VT_I4;
        V_I4(ret) = V_BOOL(v) ? 1 : 0;
        break;
    default:
        FIXME("unimplemented for vt %d\n", V_VT(v));
        return E_NOTIMPL;
    }

    return S_OK;
}

/* ECMA-262 3rd Edition    9.4 */
HRESULT to_integer(script_ctx_t *ctx, VARIANT *v, jsexcept_t *ei, VARIANT *ret)
{
    VARIANT num;
    HRESULT hres;

    hres = to_number(ctx, v, ei, &num);
    if(FAILED(hres))
        return hres;

    if(V_VT(&num) == VT_I4)
        *ret = num;
    else
        num_set_val(ret, V_R8(&num) >= 0.0 ? floor(V_R8(&num)) : -floor(-V_R8(&num)));

    return S_OK;
}

/* ECMA-262 3rd Edition    9.5 */
HRESULT to_int32(script_ctx_t *ctx, VARIANT *v, jsexcept_t *ei, INT *ret)
{
    VARIANT num;
    HRESULT hres;

    hres = to_number(ctx, v, ei, &num);
    if(FAILED(hres))
        return hres;

    *ret = V_VT(&num) == VT_I4 ? V_I4(&num) : (INT)V_R8(&num);
    return S_OK;
}

/* ECMA-262 3rd Edition    9.6 */
HRESULT to_uint32(script_ctx_t *ctx, VARIANT *v, jsexcept_t *ei, DWORD *ret)
{
    VARIANT num;
    HRESULT hres;

    hres = to_number(ctx, v, ei, &num);
    if(FAILED(hres))
        return hres;

    *ret = V_VT(&num) == VT_I4 ? V_I4(&num) : (DWORD)V_R8(&num);
    return S_OK;
}

static BSTR int_to_bstr(INT i)
{
    WCHAR buf[12], *p;
    BOOL neg = FALSE;

    if(!i) {
        static const WCHAR zeroW[] = {'0',0};
        return SysAllocString(zeroW);
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

    return SysAllocString(p);
}

/* ECMA-262 3rd Edition    9.8 */
HRESULT to_string(script_ctx_t *ctx, VARIANT *v, jsexcept_t *ei, BSTR *str)
{
    const WCHAR undefinedW[] = {'u','n','d','e','f','i','n','e','d',0};
    const WCHAR nullW[] = {'n','u','l','l',0};
    const WCHAR trueW[] = {'t','r','u','e',0};
    const WCHAR falseW[] = {'f','a','l','s','e',0};

    switch(V_VT(v)) {
    case VT_EMPTY:
        *str = SysAllocString(undefinedW);
        break;
    case VT_NULL:
        *str = SysAllocString(nullW);
        break;
    case VT_I4:
        *str = int_to_bstr(V_I4(v));
        break;
    case VT_R8: {
        VARIANT strv;
        HRESULT hres;

        V_VT(&strv) = VT_EMPTY;
        hres = VariantChangeTypeEx(&strv, v, MAKELCID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),SORT_DEFAULT), 0, VT_BSTR);
        if(FAILED(hres))
            return hres;

        *str = V_BSTR(&strv);
        return S_OK;
    }
    case VT_BSTR:
        *str = SysAllocString(V_BSTR(v));
        break;
    case VT_DISPATCH: {
        VARIANT prim;
        HRESULT hres;

        hres = to_primitive(ctx, v, ei, &prim);
        if(FAILED(hres))
            return hres;

        hres = to_string(ctx, &prim, ei, str);
        VariantClear(&prim);
        return hres;
    }
    case VT_BOOL:
        *str = SysAllocString(V_BOOL(v) ? trueW : falseW);
        break;
    default:
        FIXME("unsupported vt %d\n", V_VT(v));
        return E_NOTIMPL;
    }

    return *str ? S_OK : E_OUTOFMEMORY;
}

/* ECMA-262 3rd Edition    9.9 */
HRESULT to_object(exec_ctx_t *ctx, VARIANT *v, IDispatch **disp)
{
    DispatchEx *dispex;
    HRESULT hres;

    switch(V_VT(v)) {
    case VT_BSTR:
        hres = create_string(ctx->parser->script, V_BSTR(v), SysStringLen(V_BSTR(v)), &dispex);
        if(FAILED(hres))
            return hres;

        *disp = (IDispatch*)_IDispatchEx_(dispex);
        break;
    case VT_I4:
    case VT_R8:
        hres = create_number(ctx->parser->script, v, &dispex);
        if(FAILED(hres))
            return hres;

        *disp = (IDispatch*)_IDispatchEx_(dispex);
        break;
    case VT_DISPATCH:
        IDispatch_AddRef(V_DISPATCH(v));
        *disp = V_DISPATCH(v);
        break;
    case VT_BOOL:
        hres = create_bool(ctx->parser->script, V_BOOL(v), &dispex);
        if(FAILED(hres))
            return hres;

        *disp = (IDispatch*)_IDispatchEx_(dispex);
        break;
    default:
        FIXME("unsupported vt %d\n", V_VT(v));
        return E_NOTIMPL;
    }

    return S_OK;
}
