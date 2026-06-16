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

#include <math.h>

#include "jscript.h"
#include "regexp.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

typedef struct {
    jsdisp_t dispex;

    regexp_t *jsregexp;
    jsstr_t *str;
    INT last_index;
    jsval_t last_index_val;
} RegExpInstance;

static inline RegExpInstance *regexp_from_jsdisp(jsdisp_t *jsdisp)
{
    return CONTAINING_RECORD(jsdisp, RegExpInstance, dispex);
}

static inline RegExpInstance *regexp_this(jsval_t vthis)
{
    jsdisp_t *jsdisp = is_object_instance(vthis) ? to_jsdisp(get_object(vthis)) : NULL;
    return (jsdisp && is_class(jsdisp, JSCLASS_REGEXP)) ? regexp_from_jsdisp(jsdisp) : NULL;
}

static void set_last_index(RegExpInstance *This, DWORD last_index)
{
    This->last_index = last_index;
    jsval_release(This->last_index_val);
    This->last_index_val = jsval_number(last_index);
}

static HRESULT do_regexp_match_next(script_ctx_t *ctx, RegExpInstance *regexp,
        DWORD rem_flags, jsstr_t *jsstr, const WCHAR *str, match_state_t *ret)
{
    HRESULT hres;

    hres = regexp_execute(regexp->jsregexp, ctx, &ctx->tmp_heap,
            str, jsstr_length(jsstr), ret);
    if(FAILED(hres))
        return hres;
    if(hres == S_FALSE) {
        if(rem_flags & REM_RESET_INDEX)
            set_last_index(regexp, 0);
        return S_FALSE;
    }

    if(!(rem_flags & REM_NO_CTX_UPDATE) && ctx->last_match != jsstr) {
        jsstr_release(ctx->last_match);
        ctx->last_match = jsstr_addref(jsstr);
    }

    if(!(rem_flags & REM_NO_CTX_UPDATE)) {
        DWORD i, n = min(ARRAY_SIZE(ctx->match_parens), ret->paren_count);

        for(i=0; i < n; i++) {
            if(ret->parens[i].index == -1) {
                ctx->match_parens[i].index = 0;
                ctx->match_parens[i].length = 0;
            }else {
                ctx->match_parens[i].index = ret->parens[i].index;
                ctx->match_parens[i].length = ret->parens[i].length;
            }
        }

        if(n < ARRAY_SIZE(ctx->match_parens))
            memset(ctx->match_parens+n, 0, sizeof(ctx->match_parens) - n*sizeof(ctx->match_parens[0]));
    }

    set_last_index(regexp, ret->cp-str);

    if(!(rem_flags & REM_NO_CTX_UPDATE)) {
        ctx->last_match_index = ret->cp-str-ret->match_len;
        ctx->last_match_length = ret->match_len;
    }

    return S_OK;
}

HRESULT regexp_match_next(script_ctx_t *ctx, jsdisp_t *dispex,
        DWORD rem_flags, jsstr_t *jsstr, match_state_t **ret)
{
    RegExpInstance *regexp = regexp_from_jsdisp(dispex);
    match_state_t *match;
    heap_pool_t *mark;
    const WCHAR *str;
    HRESULT hres;

    if((rem_flags & REM_CHECK_GLOBAL) && !(regexp->jsregexp->flags & REG_GLOB)) {
        if(rem_flags & REM_ALLOC_RESULT)
            *ret = NULL;
        return S_FALSE;
    }

    str = jsstr_flatten(jsstr);
    if(!str)
        return E_OUTOFMEMORY;

    if(rem_flags & REM_ALLOC_RESULT) {
        match = alloc_match_state(regexp->jsregexp, NULL, str);
        if(!match)
            return E_OUTOFMEMORY;
        *ret = match;
    }

    mark = heap_pool_mark(&ctx->tmp_heap);

    if(rem_flags & REM_NO_PARENS) {
        match = alloc_match_state(regexp->jsregexp, &ctx->tmp_heap, NULL);
        if(!match) {
            heap_pool_clear(mark);
            return E_OUTOFMEMORY;
        }
        match->cp = (*ret)->cp;
        match->match_len = (*ret)->match_len;
    }else {
        match = *ret;
    }

    hres = do_regexp_match_next(ctx, regexp, rem_flags, jsstr, str, match);

    if(rem_flags & REM_NO_PARENS) {
        (*ret)->cp = match->cp;
        (*ret)->match_len = match->match_len;
    }

    heap_pool_clear(mark);

    if(hres != S_OK && (rem_flags & REM_ALLOC_RESULT)) {
        free(match);
        *ret = NULL;
    }

    return hres;
}

static HRESULT regexp_match(script_ctx_t *ctx, jsdisp_t *dispex, jsstr_t *jsstr, BOOL gflag,
        match_result_t **match_result, DWORD *result_cnt)
{
    RegExpInstance *This = regexp_from_jsdisp(dispex);
    match_result_t *ret = NULL;
    match_state_t *result;
    DWORD i=0, ret_size = 0;
    heap_pool_t *mark;
    const WCHAR *str;
    HRESULT hres;

    mark = heap_pool_mark(&ctx->tmp_heap);

    str = jsstr_flatten(jsstr);
    if(!str)
        return E_OUTOFMEMORY;

    result = alloc_match_state(This->jsregexp, &ctx->tmp_heap, str);
    if(!result) {
        heap_pool_clear(mark);
        return E_OUTOFMEMORY;
    }

    while(1) {
        hres = do_regexp_match_next(ctx, This, 0, jsstr, str, result);
        if(hres == S_FALSE) {
            hres = S_OK;
            break;
        }

        if(FAILED(hres))
            break;

        if(ret_size == i) {
            if(ret) {
                match_result_t *old_ret = ret;

                ret = realloc(old_ret, (ret_size <<= 1) * sizeof(match_result_t));
                if(!ret)
                    free(old_ret);
            }else {
                ret = malloc((ret_size=4) * sizeof(match_result_t));
            }
            if(!ret) {
                hres = E_OUTOFMEMORY;
                break;
            }
        }

        ret[i].index = result->cp - str - result->match_len;
        ret[i++].length = result->match_len;

        if (result->match_len == 0)
	    result->cp++;

        if(!gflag && !(This->jsregexp->flags & REG_GLOB)) {
            hres = S_OK;
            break;
        }
    }

    heap_pool_clear(mark);
    if(FAILED(hres)) {
        free(ret);
        return hres;
    }

    *match_result = ret;
    *result_cnt = i;
    return S_OK;
}

static HRESULT RegExp_get_source(script_ctx_t *ctx, jsdisp_t *jsthis, jsval_t *r)
{
    TRACE("\n");

    *r = jsval_string(jsstr_addref(regexp_from_jsdisp(jsthis)->str));
    return S_OK;
}

static HRESULT RegExp_get_global(script_ctx_t *ctx, jsdisp_t *jsthis, jsval_t *r)
{
    TRACE("\n");

    *r = jsval_bool(!!(regexp_from_jsdisp(jsthis)->jsregexp->flags & REG_GLOB));
    return S_OK;
}

static HRESULT RegExp_get_ignoreCase(script_ctx_t *ctx, jsdisp_t *jsthis, jsval_t *r)
{
    TRACE("\n");

    *r = jsval_bool(!!(regexp_from_jsdisp(jsthis)->jsregexp->flags & REG_FOLD));
    return S_OK;
}

static HRESULT RegExp_get_multiline(script_ctx_t *ctx, jsdisp_t *jsthis, jsval_t *r)
{
    TRACE("\n");

    *r = jsval_bool(!!(regexp_from_jsdisp(jsthis)->jsregexp->flags & REG_MULTILINE));
    return S_OK;
}

static INT index_from_val(script_ctx_t *ctx, jsval_t v)
{
    double n;
    HRESULT hres;

    hres = to_number(ctx, v, &n);
    if(FAILED(hres))
        return 0;

    n = floor(n);
    return is_int32(n) ? n : 0;
}

static HRESULT RegExp_get_lastIndex(script_ctx_t *ctx, jsdisp_t *jsthis, jsval_t *r)
{
    RegExpInstance *regexp = regexp_from_jsdisp(jsthis);

    TRACE("\n");

    return jsval_copy(regexp->last_index_val, r);
}

static HRESULT RegExp_set_lastIndex(script_ctx_t *ctx, jsdisp_t *jsthis, jsval_t value)
{
    RegExpInstance *regexp = regexp_from_jsdisp(jsthis);
    HRESULT hres;

    TRACE("\n");

    jsval_release(regexp->last_index_val);
    hres = jsval_copy(value, &regexp->last_index_val);
    if(FAILED(hres))
        return hres;

    regexp->last_index = index_from_val(ctx, value);
    return S_OK;
}

static HRESULT RegExp_toString(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    RegExpInstance *regexp;
    unsigned len, f;
    jsstr_t *ret;
    WCHAR *ptr;

    TRACE("\n");

    if(!(regexp = regexp_this(vthis))) {
        WARN("Not a RegExp\n");
        return JS_E_REGEXP_EXPECTED;
    }


    if(!r)
        return S_OK;

    len = jsstr_length(regexp->str) + 2;

    f = regexp->jsregexp->flags;
    if(f & REG_FOLD)
        len++;
    if(f & REG_GLOB)
        len++;
    if(f & REG_MULTILINE)
        len++;

    ret = jsstr_alloc_buf(len, &ptr);
    if(!ret)
        return E_OUTOFMEMORY;

    *ptr++ = '/';
    ptr += jsstr_flush(regexp->str, ptr);
    *ptr++ = '/';

    if(f & REG_FOLD)
        *ptr++ = 'i';
    if(f & REG_GLOB)
        *ptr++ = 'g';
    if(f & REG_MULTILINE)
        *ptr++ = 'm';

    *r = jsval_string(ret);
    return S_OK;
}

static HRESULT create_match_array(script_ctx_t *ctx, jsstr_t *input_str,
        const match_state_t *result, IDispatch **ret)
{
    const WCHAR *input;
    jsdisp_t *array;
    jsstr_t *str;
    DWORD i;
    HRESULT hres = S_OK;

    input = jsstr_flatten(input_str);
    if(!input)
        return E_OUTOFMEMORY;

    hres = create_array(ctx, result->paren_count+1, &array);
    if(FAILED(hres))
        return hres;

    for(i=0; i < result->paren_count; i++) {
        jsval_t val;

        if(result->parens[i].index != -1) {
            if(!(str = jsstr_substr(input_str, result->parens[i].index, result->parens[i].length))) {
                hres = E_OUTOFMEMORY;
                break;
            }
            val = jsval_string(str);
        }else if(ctx->version < SCRIPTLANGUAGEVERSION_ES5) {
            val = jsval_string(jsstr_empty());
        }else {
            val = jsval_undefined();
        }

        hres = jsdisp_propput_idx(array, i+1, val);
        jsval_release(val);
        if(FAILED(hres))
            break;
    }

    while(SUCCEEDED(hres)) {
        hres = jsdisp_propput_name(array, L"index", jsval_number(result->cp-input-result->match_len));
        if(FAILED(hres))
            break;

        hres = jsdisp_propput_name(array, L"lastIndex", jsval_number(result->cp-input));
        if(FAILED(hres))
            break;

        hres = jsdisp_propput_name(array, L"input", jsval_string(input_str));
        if(FAILED(hres))
            break;

        str = jsstr_alloc_len(result->cp-result->match_len, result->match_len);
        if(!str) {
            hres = E_OUTOFMEMORY;
            break;
        }
        hres = jsdisp_propput_name(array, L"0", jsval_string(str));
        jsstr_release(str);
        break;
    }

    if(FAILED(hres)) {
        jsdisp_release(array);
        return hres;
    }

    *ret = to_disp(array);
    return S_OK;
}

static HRESULT run_exec(script_ctx_t *ctx, jsval_t vthis, jsval_t arg,
        jsstr_t **input, match_state_t **result, BOOL *ret)
{
    RegExpInstance *regexp;
    match_state_t *match;
    DWORD last_index = 0;
    const WCHAR *string;
    jsstr_t *jsstr;
    HRESULT hres;

    if(!(regexp = regexp_this(vthis))) {
        WARN("Not a RegExp\n");
        return JS_E_REGEXP_EXPECTED;
    }

    hres = to_flat_string(ctx, arg, &jsstr, &string);
    if(FAILED(hres))
        return hres;

    if(regexp->jsregexp->flags & REG_GLOB) {
        if(regexp->last_index < 0) {
            jsstr_release(jsstr);
            set_last_index(regexp, 0);
            *ret = FALSE;
            if(input)
                *input = jsstr_empty();
            return S_OK;
        }

        last_index = regexp->last_index;
    }

    match = alloc_match_state(regexp->jsregexp, &ctx->tmp_heap, string+last_index);
    if(!match) {
        jsstr_release(jsstr);
        return E_OUTOFMEMORY;
    }

    hres = regexp_match_next(ctx, &regexp->dispex, REM_RESET_INDEX, jsstr, &match);
    if(FAILED(hres)) {
        jsstr_release(jsstr);
        return hres;
    }

    *result = match;
    *ret = hres == S_OK;
    if(input)
        *input = jsstr;
    else
        jsstr_release(jsstr);
    return S_OK;
}

static HRESULT RegExp_exec(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    match_state_t *match;
    heap_pool_t *mark;
    BOOL b;
    jsstr_t *string;
    HRESULT hres;

    TRACE("\n");

    mark = heap_pool_mark(&ctx->tmp_heap);

    hres = run_exec(ctx, vthis, argc ? argv[0] : jsval_string(jsstr_empty()), &string, &match, &b);
    if(FAILED(hres)) {
        heap_pool_clear(mark);
        return hres;
    }

    if(r) {
        if(b) {
            IDispatch *ret;

            hres = create_match_array(ctx, string, match, &ret);
            if(SUCCEEDED(hres))
                *r = jsval_disp(ret);
        }else {
            *r = jsval_null();
        }
    }

    heap_pool_clear(mark);
    jsstr_release(string);
    return hres;
}

static HRESULT RegExp_test(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    match_state_t *match;
    jsstr_t *undef_str;
    heap_pool_t *mark;
    BOOL b;
    HRESULT hres;

    TRACE("\n");

    mark = heap_pool_mark(&ctx->tmp_heap);
    hres = run_exec(ctx, vthis, argc ? argv[0] : jsval_string(undef_str = jsstr_undefined()), NULL, &match, &b);
    heap_pool_clear(mark);
    if(!argc)
        jsstr_release(undef_str);
    if(FAILED(hres))
        return hres;

    if(r)
        *r = jsval_bool(b);
    return S_OK;
}

static HRESULT RegExp_value(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
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

static void RegExp_destructor(jsdisp_t *dispex)
{
    RegExpInstance *This = regexp_from_jsdisp(dispex);

    if(This->jsregexp)
        regexp_destroy(This->jsregexp);
    jsval_release(This->last_index_val);
    jsstr_release(This->str);
}

static HRESULT RegExp_gc_traverse(struct gc_ctx *gc_ctx, enum gc_traverse_op op, jsdisp_t *dispex)
{
    return gc_process_linked_val(gc_ctx, op, dispex, &regexp_from_jsdisp(dispex)->last_index_val);
}

static const builtin_prop_t RegExp_props[] = {
    {L"exec",                RegExp_exec,                  PROPF_METHOD|1},
    {L"global",              NULL,0,                       RegExp_get_global},
    {L"ignoreCase",          NULL,0,                       RegExp_get_ignoreCase},
    {L"lastIndex",           NULL,0,                       RegExp_get_lastIndex,  RegExp_set_lastIndex},
    {L"multiline",           NULL,0,                       RegExp_get_multiline},
    {L"source",              NULL,0,                       RegExp_get_source},
    {L"test",                RegExp_test,                  PROPF_METHOD|1},
    {L"toString",            RegExp_toString,              PROPF_METHOD}
};

static const builtin_info_t RegExp_info = {
    .class       = JSCLASS_REGEXP,
    .call        = RegExp_value,
    .props_cnt   = ARRAY_SIZE(RegExp_props),
    .props       = RegExp_props,
    .destructor  = RegExp_destructor,
    .gc_traverse = RegExp_gc_traverse
};

static const builtin_prop_t RegExpInst_props[] = {
    {L"global",              NULL,0,                       RegExp_get_global},
    {L"ignoreCase",          NULL,0,                       RegExp_get_ignoreCase},
    {L"lastIndex",           NULL,0,                       RegExp_get_lastIndex,  RegExp_set_lastIndex},
    {L"multiline",           NULL,0,                       RegExp_get_multiline},
    {L"source",              NULL,0,                       RegExp_get_source}
};

static const builtin_info_t RegExpInst_info = {
    .class       = JSCLASS_REGEXP,
    .call        = RegExp_value,
    .props_cnt   = ARRAY_SIZE(RegExpInst_props),
    .props       = RegExpInst_props,
    .destructor  = RegExp_destructor,
    .gc_traverse = RegExp_gc_traverse
};

static HRESULT alloc_regexp(script_ctx_t *ctx, jsstr_t *str, jsdisp_t *object_prototype, RegExpInstance **ret)
{
    RegExpInstance *regexp;
    HRESULT hres;

    regexp = calloc(1, sizeof(RegExpInstance));
    if(!regexp)
        return E_OUTOFMEMORY;

    if(object_prototype)
        hres = init_dispex(&regexp->dispex, ctx, &RegExp_info, object_prototype);
    else
        hres = init_dispex_from_constr(&regexp->dispex, ctx, &RegExpInst_info, ctx->regexp_constr);

    if(FAILED(hres)) {
        free(regexp);
        return hres;
    }

    regexp->str = jsstr_addref(str);
    regexp->last_index_val = jsval_number(0);

    *ret = regexp;
    return S_OK;
}

HRESULT create_regexp(script_ctx_t *ctx, jsstr_t *src, DWORD flags, jsdisp_t **ret)
{
    RegExpInstance *regexp;
    const WCHAR *str;
    HRESULT hres;

    str = jsstr_flatten(src);
    if(!str)
        return E_OUTOFMEMORY;

    TRACE("%s %lx\n", debugstr_wn(str, jsstr_length(src)), flags);

    hres = alloc_regexp(ctx, src, NULL, &regexp);
    if(FAILED(hres))
        return hres;

    regexp->jsregexp = regexp_new(ctx, &ctx->tmp_heap, str, jsstr_length(regexp->str), flags, FALSE);
    if(!regexp->jsregexp) {
        WARN("regexp_new failed\n");
        jsdisp_release(&regexp->dispex);
        return DISP_E_EXCEPTION;
    }

    *ret = &regexp->dispex;
    return S_OK;
}

HRESULT create_regexp_var(script_ctx_t *ctx, jsval_t src_arg, jsval_t *flags_arg, jsdisp_t **ret)
{
    DWORD flags = 0;
    const WCHAR *opt = NULL;
    jsstr_t *src;
    HRESULT hres = S_OK;

    if(is_object_instance(src_arg)) {
        jsdisp_t *obj;

        obj = to_jsdisp(get_object(src_arg));
        if(obj) {
            if(is_class(obj, JSCLASS_REGEXP)) {
                RegExpInstance *regexp = regexp_from_jsdisp(obj);

                hres = create_regexp(ctx, regexp->str, regexp->jsregexp->flags, ret);
                return hres;
            }
        }
    }

    if(is_undefined(src_arg))
        src = jsstr_empty();
    else
        hres = to_string(ctx, src_arg, &src);
    if(FAILED(hres))
        return hres;

    if(flags_arg && !is_undefined(*flags_arg)) {
        jsstr_t *opt_str;

        hres = to_string(ctx, *flags_arg, &opt_str);
        if(SUCCEEDED(hres)) {
            opt = jsstr_flatten(opt_str);
            if(opt)
                hres = parse_regexp_flags(opt, jsstr_length(opt_str), &flags);
            else
                hres = E_OUTOFMEMORY;
            jsstr_release(opt_str);
        }
    }

    if(SUCCEEDED(hres))
        hres = create_regexp(ctx, src, flags, ret);
    jsstr_release(src);
    return hres;
}

HRESULT regexp_string_match(script_ctx_t *ctx, jsdisp_t *re, jsstr_t *jsstr, jsval_t *r)
{
    RegExpInstance *regexp = regexp_from_jsdisp(re);
    match_result_t *match_result;
    DWORD match_cnt, i;
    const WCHAR *str;
    jsdisp_t *array;
    HRESULT hres;

    str = jsstr_flatten(jsstr);
    if(!str)
        return E_OUTOFMEMORY;

    if(!(regexp->jsregexp->flags & REG_GLOB)) {
        match_state_t *match;
        heap_pool_t *mark;

        mark = heap_pool_mark(&ctx->tmp_heap);
        match = alloc_match_state(regexp->jsregexp, &ctx->tmp_heap, str);
        if(!match) {
            heap_pool_clear(mark);
            return E_OUTOFMEMORY;
        }

        hres = regexp_match_next(ctx, &regexp->dispex, 0, jsstr, &match);
        if(FAILED(hres)) {
            heap_pool_clear(mark);
            return hres;
        }

        if(r) {
            if(hres == S_OK) {
                IDispatch *ret;

                hres = create_match_array(ctx, jsstr, match, &ret);
                if(SUCCEEDED(hres))
                    *r = jsval_disp(ret);
            }else {
                *r = jsval_null();
            }
        }

        heap_pool_clear(mark);
        return S_OK;
    }

    hres = regexp_match(ctx, &regexp->dispex, jsstr, FALSE, &match_result, &match_cnt);
    if(FAILED(hres))
        return hres;

    if(!match_cnt) {
        TRACE("no match\n");

        if(r)
            *r = jsval_null();
        return S_OK;
    }

    hres = create_array(ctx, match_cnt, &array);
    if(FAILED(hres)) {
        free(match_result);
        return hres;
    }

    for(i=0; i < match_cnt; i++) {
        jsstr_t *tmp_str;

        tmp_str = jsstr_substr(jsstr, match_result[i].index, match_result[i].length);
        if(!tmp_str) {
            hres = E_OUTOFMEMORY;
            break;
        }

        hres = jsdisp_propput_idx(array, i, jsval_string(tmp_str));
        jsstr_release(tmp_str);
        if(FAILED(hres))
            break;
    }

    while(SUCCEEDED(hres)) {
        hres = jsdisp_propput_name(array, L"index", jsval_number(match_result[match_cnt-1].index));
        if(FAILED(hres))
            break;

        hres = jsdisp_propput_name(array, L"lastIndex",
                jsval_number(match_result[match_cnt-1].index + match_result[match_cnt-1].length));
        if(FAILED(hres))
            break;

        hres = jsdisp_propput_name(array, L"input", jsval_string(jsstr));
        break;
    }

    free(match_result);

    if(SUCCEEDED(hres) && r)
        *r = jsval_obj(array);
    else
        jsdisp_release(array);
    return hres;
}

static HRESULT global_idx(script_ctx_t *ctx, DWORD idx, jsval_t *r)
{
    jsstr_t *ret;

    ret = jsstr_substr(ctx->last_match, ctx->match_parens[idx].index, ctx->match_parens[idx].length);
    if(!ret)
        return E_OUTOFMEMORY;

    *r = jsval_string(ret);
    return S_OK;
}

static HRESULT RegExpConstr_get_idx1(script_ctx_t *ctx, jsdisp_t *jsthis, jsval_t *r)
{
    TRACE("\n");
    return global_idx(ctx, 0, r);
}

static HRESULT RegExpConstr_get_idx2(script_ctx_t *ctx, jsdisp_t *jsthis, jsval_t *r)
{
    TRACE("\n");
    return global_idx(ctx, 1, r);
}

static HRESULT RegExpConstr_get_idx3(script_ctx_t *ctx, jsdisp_t *jsthis, jsval_t *r)
{
    TRACE("\n");
    return global_idx(ctx, 2, r);
}

static HRESULT RegExpConstr_get_idx4(script_ctx_t *ctx, jsdisp_t *jsthis, jsval_t *r)
{
    TRACE("\n");
    return global_idx(ctx, 3, r);
}

static HRESULT RegExpConstr_get_idx5(script_ctx_t *ctx, jsdisp_t *jsthis, jsval_t *r)
{
    TRACE("\n");
    return global_idx(ctx, 4, r);
}

static HRESULT RegExpConstr_get_idx6(script_ctx_t *ctx, jsdisp_t *jsthis, jsval_t *r)
{
    TRACE("\n");
    return global_idx(ctx, 5, r);
}

static HRESULT RegExpConstr_get_idx7(script_ctx_t *ctx, jsdisp_t *jsthis, jsval_t *r)
{
    TRACE("\n");
    return global_idx(ctx, 6, r);
}

static HRESULT RegExpConstr_get_idx8(script_ctx_t *ctx, jsdisp_t *jsthis, jsval_t *r)
{
    TRACE("\n");
    return global_idx(ctx, 7, r);
}

static HRESULT RegExpConstr_get_idx9(script_ctx_t *ctx, jsdisp_t *jsthis, jsval_t *r)
{
    TRACE("\n");
    return global_idx(ctx, 8, r);
}

static HRESULT RegExpConstr_get_leftContext(script_ctx_t *ctx, jsdisp_t *jsthis, jsval_t *r)
{
    jsstr_t *ret;

    TRACE("\n");

    ret = jsstr_substr(ctx->last_match, 0, ctx->last_match_index);
    if(!ret)
        return E_OUTOFMEMORY;

    *r = jsval_string(ret);
    return S_OK;
}

static HRESULT RegExpConstr_get_rightContext(script_ctx_t *ctx, jsdisp_t *jsthis, jsval_t *r)
{
    jsstr_t *ret;

    TRACE("\n");

    ret = jsstr_substr(ctx->last_match, ctx->last_match_index+ctx->last_match_length,
            jsstr_length(ctx->last_match) - ctx->last_match_index - ctx->last_match_length);
    if(!ret)
        return E_OUTOFMEMORY;

    *r = jsval_string(ret);
    return S_OK;
}

static HRESULT RegExpConstr_value(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    TRACE("\n");

    switch(flags) {
    case DISPATCH_METHOD:
        if(argc) {
            if(is_object_instance(argv[0])) {
                jsdisp_t *jsdisp = to_jsdisp(get_object(argv[0]));
                if(jsdisp) {
                    if(is_class(jsdisp, JSCLASS_REGEXP)) {
                        if(argc > 1 && !is_undefined(argv[1]))
                            return JS_E_REGEXP_SYNTAX;

                        if(r)
                            *r = jsval_obj(jsdisp_addref(jsdisp));
                        return S_OK;
                    }
                }
            }
        }
        /* fall through */
    case DISPATCH_CONSTRUCT: {
        jsdisp_t *ret;
        HRESULT hres;

        hres = create_regexp_var(ctx, argc ? argv[0] : jsval_undefined(), argc > 1 ? argv+1 : NULL, &ret);
        if(FAILED(hres))
            return hres;

        if(r)
            *r = jsval_obj(ret);
        else
            jsdisp_release(ret);
        return S_OK;
    }
    default:
        FIXME("unimplemented flags: %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

static const builtin_prop_t RegExpConstr_props[] = {
    {L"$1",           NULL,0,  RegExpConstr_get_idx1,         builtin_set_const},
    {L"$2",           NULL,0,  RegExpConstr_get_idx2,         builtin_set_const},
    {L"$3",           NULL,0,  RegExpConstr_get_idx3,         builtin_set_const},
    {L"$4",           NULL,0,  RegExpConstr_get_idx4,         builtin_set_const},
    {L"$5",           NULL,0,  RegExpConstr_get_idx5,         builtin_set_const},
    {L"$6",           NULL,0,  RegExpConstr_get_idx6,         builtin_set_const},
    {L"$7",           NULL,0,  RegExpConstr_get_idx7,         builtin_set_const},
    {L"$8",           NULL,0,  RegExpConstr_get_idx8,         builtin_set_const},
    {L"$9",           NULL,0,  RegExpConstr_get_idx9,         builtin_set_const},
    {L"leftContext",  NULL,0,  RegExpConstr_get_leftContext,  builtin_set_const},
    {L"rightContext", NULL,0,  RegExpConstr_get_rightContext, builtin_set_const}
};

static const builtin_info_t RegExpConstr_info = {
    .class     = JSCLASS_FUNCTION,
    .call      = Function_value,
    .props_cnt = ARRAY_SIZE(RegExpConstr_props),
    .props     = RegExpConstr_props,
};

HRESULT create_regexp_constr(script_ctx_t *ctx, jsdisp_t *object_prototype, jsdisp_t **ret)
{
    jsstr_t *str = jsstr_empty();
    RegExpInstance *regexp;
    HRESULT hres;

    hres = alloc_regexp(ctx, str, object_prototype, &regexp);
    jsstr_release(str);
    if(FAILED(hres))
        return hres;

    hres = create_builtin_constructor(ctx, RegExpConstr_value, L"RegExp", &RegExpConstr_info,
            PROPF_CONSTR|2, &regexp->dispex, ret);

    jsdisp_release(&regexp->dispex);
    return hres;
}

HRESULT parse_regexp_flags(const WCHAR *str, DWORD str_len, DWORD *ret)
{
    const WCHAR *p;
    DWORD flags = 0;

    for (p = str; p < str+str_len; p++) {
        switch (*p) {
        case 'g':
            flags |= REG_GLOB;
            break;
        case 'i':
            flags |= REG_FOLD;
            break;
        case 'm':
            flags |= REG_MULTILINE;
            break;
        case 'y':
            flags |= REG_STICKY;
            break;
        default:
            WARN("wrong flag %c\n", *p);
            return E_FAIL;
        }
    }

    *ret = flags;
    return S_OK;
}
