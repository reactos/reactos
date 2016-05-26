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

typedef struct {
    jsdisp_t dispex;

    regexp_t *jsregexp;
    jsstr_t *str;
    INT last_index;
    jsval_t last_index_val;
} RegExpInstance;

static const WCHAR sourceW[] = {'s','o','u','r','c','e',0};
static const WCHAR globalW[] = {'g','l','o','b','a','l',0};
static const WCHAR ignoreCaseW[] = {'i','g','n','o','r','e','C','a','s','e',0};
static const WCHAR multilineW[] = {'m','u','l','t','i','l','i','n','e',0};
static const WCHAR lastIndexW[] = {'l','a','s','t','I','n','d','e','x',0};
static const WCHAR toStringW[] = {'t','o','S','t','r','i','n','g',0};
static const WCHAR execW[] = {'e','x','e','c',0};
static const WCHAR testW[] = {'t','e','s','t',0};

static const WCHAR leftContextW[] =
    {'l','e','f','t','C','o','n','t','e','x','t',0};
static const WCHAR rightContextW[] =
    {'r','i','g','h','t','C','o','n','t','e','x','t',0};

static const WCHAR idx1W[] = {'$','1',0};
static const WCHAR idx2W[] = {'$','2',0};
static const WCHAR idx3W[] = {'$','3',0};
static const WCHAR idx4W[] = {'$','4',0};
static const WCHAR idx5W[] = {'$','5',0};
static const WCHAR idx6W[] = {'$','6',0};
static const WCHAR idx7W[] = {'$','7',0};
static const WCHAR idx8W[] = {'$','8',0};
static const WCHAR idx9W[] = {'$','9',0};

static inline RegExpInstance *regexp_from_jsdisp(jsdisp_t *jsdisp)
{
    return CONTAINING_RECORD(jsdisp, RegExpInstance, dispex);
}

static inline RegExpInstance *regexp_from_vdisp(vdisp_t *vdisp)
{
    return regexp_from_jsdisp(vdisp->u.jsdisp);
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
        DWORD i, n = min(sizeof(ctx->match_parens)/sizeof(ctx->match_parens[0]), ret->paren_count);

        for(i=0; i < n; i++) {
            if(ret->parens[i].index == -1) {
                ctx->match_parens[i].index = 0;
                ctx->match_parens[i].length = 0;
            }else {
                ctx->match_parens[i].index = ret->parens[i].index;
                ctx->match_parens[i].length = ret->parens[i].length;
            }
        }

        if(n < sizeof(ctx->match_parens)/sizeof(ctx->match_parens[0]))
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
    RegExpInstance *regexp = (RegExpInstance*)dispex;
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
        heap_free(match);
        *ret = NULL;
    }

    return hres;
}

static HRESULT regexp_match(script_ctx_t *ctx, jsdisp_t *dispex, jsstr_t *jsstr, BOOL gflag,
        match_result_t **match_result, DWORD *result_cnt)
{
    RegExpInstance *This = (RegExpInstance*)dispex;
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

                ret = heap_realloc(old_ret, (ret_size <<= 1) * sizeof(match_result_t));
                if(!ret)
                    heap_free(old_ret);
            }else {
                ret = heap_alloc((ret_size=4) * sizeof(match_result_t));
            }
            if(!ret) {
                hres = E_OUTOFMEMORY;
                break;
            }
        }

        ret[i].index = result->cp - str - result->match_len;
        ret[i++].length = result->match_len;

        if(!gflag && !(This->jsregexp->flags & REG_GLOB)) {
            hres = S_OK;
            break;
        }
    }

    heap_pool_clear(mark);
    if(FAILED(hres)) {
        heap_free(ret);
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

static HRESULT RegExp_set_source(script_ctx_t *ctx, jsdisp_t *jsthis, jsval_t value)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT RegExp_get_global(script_ctx_t *ctx, jsdisp_t *jsthis, jsval_t *r)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT RegExp_set_global(script_ctx_t *ctx, jsdisp_t *jsthis, jsval_t value)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT RegExp_get_ignoreCase(script_ctx_t *ctx, jsdisp_t *jsthis, jsval_t *r)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT RegExp_set_ignoreCase(script_ctx_t *ctx, jsdisp_t *jsthis, jsval_t value)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT RegExp_get_multiline(script_ctx_t *ctx, jsdisp_t *jsthis, jsval_t *r)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT RegExp_set_multiline(script_ctx_t *ctx, jsdisp_t *jsthis, jsval_t value)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static INT index_from_val(script_ctx_t *ctx, jsval_t v)
{
    double n;
    HRESULT hres;

    hres = to_number(ctx, v, &n);
    if(FAILED(hres)) {
        clear_ei(ctx); /* FIXME: Move ignoring exceptions to to_primitive */
        return 0;
    }

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

    hres = jsval_copy(value, &regexp->last_index_val);
    if(FAILED(hres))
        return hres;

    regexp->last_index = index_from_val(ctx, value);
    return S_OK;
}

static HRESULT RegExp_toString(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    RegExpInstance *regexp;
    unsigned len, f;
    jsstr_t *ret;
    WCHAR *ptr;

    TRACE("\n");

    if(!is_vclass(jsthis, JSCLASS_REGEXP)) {
        FIXME("Not a RegExp\n");
        return E_NOTIMPL;
    }

    regexp = regexp_from_vdisp(jsthis);

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

    ptr = jsstr_alloc_buf(len, &ret);
    if(!ptr)
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

    static const WCHAR indexW[] = {'i','n','d','e','x',0};
    static const WCHAR inputW[] = {'i','n','p','u','t',0};
    static const WCHAR lastIndexW[] = {'l','a','s','t','I','n','d','e','x',0};
    static const WCHAR zeroW[] = {'0',0};

    input = jsstr_flatten(input_str);
    if(!input)
        return E_OUTOFMEMORY;

    hres = create_array(ctx, result->paren_count+1, &array);
    if(FAILED(hres))
        return hres;

    for(i=0; i < result->paren_count; i++) {
        if(result->parens[i].index != -1)
            str = jsstr_substr(input_str, result->parens[i].index, result->parens[i].length);
        else
            str = jsstr_empty();
        if(!str) {
            hres = E_OUTOFMEMORY;
            break;
        }

        hres = jsdisp_propput_idx(array, i+1, jsval_string(str));
        jsstr_release(str);
        if(FAILED(hres))
            break;
    }

    while(SUCCEEDED(hres)) {
        hres = jsdisp_propput_name(array, indexW, jsval_number(result->cp-input-result->match_len));
        if(FAILED(hres))
            break;

        hres = jsdisp_propput_name(array, lastIndexW, jsval_number(result->cp-input));
        if(FAILED(hres))
            break;

        hres = jsdisp_propput_name(array, inputW, jsval_string(jsstr_addref(input_str)));
        if(FAILED(hres))
            break;

        str = jsstr_alloc_len(result->cp-result->match_len, result->match_len);
        if(!str) {
            hres = E_OUTOFMEMORY;
            break;
        }
        hres = jsdisp_propput_name(array, zeroW, jsval_string(str));
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

static HRESULT run_exec(script_ctx_t *ctx, vdisp_t *jsthis, jsval_t arg,
        jsstr_t **input, match_state_t **result, BOOL *ret)
{
    RegExpInstance *regexp;
    match_state_t *match;
    DWORD last_index = 0;
    const WCHAR *string;
    jsstr_t *jsstr;
    HRESULT hres;

    if(!is_vclass(jsthis, JSCLASS_REGEXP)) {
        FIXME("Not a RegExp\n");
        return E_NOTIMPL;
    }

    regexp = regexp_from_vdisp(jsthis);

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

static HRESULT RegExp_exec(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    match_state_t *match;
    heap_pool_t *mark;
    BOOL b;
    jsstr_t *string;
    HRESULT hres;

    TRACE("\n");

    mark = heap_pool_mark(&ctx->tmp_heap);

    hres = run_exec(ctx, jsthis, argc ? argv[0] : jsval_string(jsstr_empty()), &string, &match, &b);
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

static HRESULT RegExp_test(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    match_state_t *match;
    jsstr_t *undef_str;
    heap_pool_t *mark;
    BOOL b;
    HRESULT hres;

    TRACE("\n");

    mark = heap_pool_mark(&ctx->tmp_heap);
    hres = run_exec(ctx, jsthis, argc ? argv[0] : jsval_string(undef_str = jsstr_undefined()), NULL, &match, &b);
    heap_pool_clear(mark);
    if(!argc)
        jsstr_release(undef_str);
    if(FAILED(hres))
        return hres;

    if(r)
        *r = jsval_bool(b);
    return S_OK;
}

static HRESULT RegExp_value(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    TRACE("\n");

    switch(flags) {
    case INVOKE_FUNC:
        return throw_type_error(ctx, JS_E_FUNCTION_EXPECTED, NULL);
    default:
        FIXME("unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

static void RegExp_destructor(jsdisp_t *dispex)
{
    RegExpInstance *This = (RegExpInstance*)dispex;

    if(This->jsregexp)
        regexp_destroy(This->jsregexp);
    jsval_release(This->last_index_val);
    jsstr_release(This->str);
    heap_free(This);
}

static const builtin_prop_t RegExp_props[] = {
    {execW,                  RegExp_exec,                  PROPF_METHOD|1},
    {globalW,                NULL,0,                       RegExp_get_global,     RegExp_set_global},
    {ignoreCaseW,            NULL,0,                       RegExp_get_ignoreCase, RegExp_set_ignoreCase},
    {lastIndexW,             NULL,0,                       RegExp_get_lastIndex,  RegExp_set_lastIndex},
    {multilineW,             NULL,0,                       RegExp_get_multiline,  RegExp_set_multiline},
    {sourceW,                NULL,0,                       RegExp_get_source,     RegExp_set_source},
    {testW,                  RegExp_test,                  PROPF_METHOD|1},
    {toStringW,              RegExp_toString,              PROPF_METHOD}
};

static const builtin_info_t RegExp_info = {
    JSCLASS_REGEXP,
    {NULL, RegExp_value, 0},
    sizeof(RegExp_props)/sizeof(*RegExp_props),
    RegExp_props,
    RegExp_destructor,
    NULL
};

static const builtin_prop_t RegExpInst_props[] = {
    {globalW,                NULL,0,                       RegExp_get_global,     RegExp_set_global},
    {ignoreCaseW,            NULL,0,                       RegExp_get_ignoreCase, RegExp_set_ignoreCase},
    {lastIndexW,             NULL,0,                       RegExp_get_lastIndex,  RegExp_set_lastIndex},
    {multilineW,             NULL,0,                       RegExp_get_multiline,  RegExp_set_multiline},
    {sourceW,                NULL,0,                       RegExp_get_source,     RegExp_set_source}
};

static const builtin_info_t RegExpInst_info = {
    JSCLASS_REGEXP,
    {NULL, RegExp_value, 0},
    sizeof(RegExpInst_props)/sizeof(*RegExpInst_props),
    RegExpInst_props,
    RegExp_destructor,
    NULL
};

static HRESULT alloc_regexp(script_ctx_t *ctx, jsdisp_t *object_prototype, RegExpInstance **ret)
{
    RegExpInstance *regexp;
    HRESULT hres;

    regexp = heap_alloc_zero(sizeof(RegExpInstance));
    if(!regexp)
        return E_OUTOFMEMORY;

    if(object_prototype)
        hres = init_dispex(&regexp->dispex, ctx, &RegExp_info, object_prototype);
    else
        hres = init_dispex_from_constr(&regexp->dispex, ctx, &RegExpInst_info, ctx->regexp_constr);

    if(FAILED(hres)) {
        heap_free(regexp);
        return hres;
    }

    *ret = regexp;
    return S_OK;
}

HRESULT create_regexp(script_ctx_t *ctx, jsstr_t *src, DWORD flags, jsdisp_t **ret)
{
    RegExpInstance *regexp;
    const WCHAR *str;
    HRESULT hres;

    TRACE("%s %x\n", debugstr_jsstr(src), flags);

    str = jsstr_flatten(src);
    if(!str)
        return E_OUTOFMEMORY;

    hres = alloc_regexp(ctx, NULL, &regexp);
    if(FAILED(hres))
        return hres;

    regexp->str = jsstr_addref(src);
    regexp->last_index_val = jsval_number(0);

    regexp->jsregexp = regexp_new(ctx, &ctx->tmp_heap, str, jsstr_length(regexp->str), flags, FALSE);
    if(!regexp->jsregexp) {
        WARN("regexp_new failed\n");
        jsdisp_release(&regexp->dispex);
        return E_FAIL;
    }

    *ret = &regexp->dispex;
    return S_OK;
}

HRESULT create_regexp_var(script_ctx_t *ctx, jsval_t src_arg, jsval_t *flags_arg, jsdisp_t **ret)
{
    unsigned flags, opt_len = 0;
    const WCHAR *opt = NULL;
    jsstr_t *src;
    HRESULT hres;

    if(is_object_instance(src_arg)) {
        jsdisp_t *obj;

        obj = iface_to_jsdisp((IUnknown*)get_object(src_arg));
        if(obj) {
            if(is_class(obj, JSCLASS_REGEXP)) {
                RegExpInstance *regexp = (RegExpInstance*)obj;

                hres = create_regexp(ctx, regexp->str, regexp->jsregexp->flags, ret);
                jsdisp_release(obj);
                return hres;
            }

            jsdisp_release(obj);
        }
    }

    if(!is_string(src_arg)) {
        FIXME("src_arg = %s\n", debugstr_jsval(src_arg));
        return E_NOTIMPL;
    }

    src = get_string(src_arg);

    if(flags_arg) {
        jsstr_t *opt_str;

        if(!is_string(*flags_arg)) {
            FIXME("unimplemented for %s\n", debugstr_jsval(*flags_arg));
            return E_NOTIMPL;
        }

        opt_str = get_string(*flags_arg);
        opt = jsstr_flatten(opt_str);
        if(!opt)
            return E_OUTOFMEMORY;
        opt_len = jsstr_length(opt_str);
    }

    hres = parse_regexp_flags(opt, opt_len, &flags);
    if(FAILED(hres))
        return hres;

    return create_regexp(ctx, src, flags, ret);
}

HRESULT regexp_string_match(script_ctx_t *ctx, jsdisp_t *re, jsstr_t *jsstr, jsval_t *r)
{
    static const WCHAR indexW[] = {'i','n','d','e','x',0};
    static const WCHAR inputW[] = {'i','n','p','u','t',0};
    static const WCHAR lastIndexW[] = {'l','a','s','t','I','n','d','e','x',0};

    RegExpInstance *regexp = (RegExpInstance*)re;
    match_result_t *match_result;
    unsigned match_cnt, i;
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
    if(FAILED(hres))
        return hres;

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
        hres = jsdisp_propput_name(array, indexW, jsval_number(match_result[match_cnt-1].index));
        if(FAILED(hres))
            break;

        hres = jsdisp_propput_name(array, lastIndexW,
                jsval_number(match_result[match_cnt-1].index + match_result[match_cnt-1].length));
        if(FAILED(hres))
            break;

        hres = jsdisp_propput_name(array, inputW, jsval_string(jsstr));
        break;
    }

    heap_free(match_result);

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

static HRESULT RegExpConstr_value(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    TRACE("\n");

    switch(flags) {
    case DISPATCH_METHOD:
        if(argc) {
            if(is_object_instance(argv[0])) {
                jsdisp_t *jsdisp = iface_to_jsdisp((IUnknown*)get_object(argv[0]));
                if(jsdisp) {
                    if(is_class(jsdisp, JSCLASS_REGEXP)) {
                        if(argc > 1 && !is_undefined(argv[1])) {
                            jsdisp_release(jsdisp);
                            return throw_regexp_error(ctx, JS_E_REGEXP_SYNTAX, NULL);
                        }

                        if(r)
                            *r = jsval_obj(jsdisp);
                        else
                            jsdisp_release(jsdisp);
                        return S_OK;
                    }
                    jsdisp_release(jsdisp);
                }
            }
        }
        /* fall through */
    case DISPATCH_CONSTRUCT: {
        jsdisp_t *ret;
        HRESULT hres;

        if(!argc) {
            FIXME("no args\n");
            return E_NOTIMPL;
        }

        hres = create_regexp_var(ctx, argv[0], argc > 1 ? argv+1 : NULL, &ret);
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
    {idx1W,           NULL,0,  RegExpConstr_get_idx1,         builtin_set_const},
    {idx2W,           NULL,0,  RegExpConstr_get_idx2,         builtin_set_const},
    {idx3W,           NULL,0,  RegExpConstr_get_idx3,         builtin_set_const},
    {idx4W,           NULL,0,  RegExpConstr_get_idx4,         builtin_set_const},
    {idx5W,           NULL,0,  RegExpConstr_get_idx5,         builtin_set_const},
    {idx6W,           NULL,0,  RegExpConstr_get_idx6,         builtin_set_const},
    {idx7W,           NULL,0,  RegExpConstr_get_idx7,         builtin_set_const},
    {idx8W,           NULL,0,  RegExpConstr_get_idx8,         builtin_set_const},
    {idx9W,           NULL,0,  RegExpConstr_get_idx9,         builtin_set_const},
    {leftContextW,    NULL,0,  RegExpConstr_get_leftContext,  builtin_set_const},
    {rightContextW,   NULL,0,  RegExpConstr_get_rightContext, builtin_set_const}
};

static const builtin_info_t RegExpConstr_info = {
    JSCLASS_FUNCTION,
    DEFAULT_FUNCTION_VALUE,
    sizeof(RegExpConstr_props)/sizeof(*RegExpConstr_props),
    RegExpConstr_props,
    NULL,
    NULL
};

HRESULT create_regexp_constr(script_ctx_t *ctx, jsdisp_t *object_prototype, jsdisp_t **ret)
{
    RegExpInstance *regexp;
    HRESULT hres;

    static const WCHAR RegExpW[] = {'R','e','g','E','x','p',0};

    hres = alloc_regexp(ctx, object_prototype, &regexp);
    if(FAILED(hres))
        return hres;

    hres = create_builtin_constructor(ctx, RegExpConstr_value, RegExpW, &RegExpConstr_info,
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
