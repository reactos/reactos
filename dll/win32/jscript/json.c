/*
 * Copyright 2016 Jacek Caban for CodeWeavers
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
#include <assert.h>

#include "jscript.h"
#include "parser.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

static const WCHAR parseW[] = {'p','a','r','s','e',0};
static const WCHAR stringifyW[] = {'s','t','r','i','n','g','i','f','y',0};

static const WCHAR nullW[] = {'n','u','l','l',0};
static const WCHAR trueW[] = {'t','r','u','e',0};
static const WCHAR falseW[] = {'f','a','l','s','e',0};

static const WCHAR toJSONW[] = {'t','o','J','S','O','N',0};

typedef struct {
    const WCHAR *ptr;
    const WCHAR *end;
    script_ctx_t *ctx;
} json_parse_ctx_t;

static BOOL is_json_space(WCHAR c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static WCHAR skip_spaces(json_parse_ctx_t *ctx)
{
    while(is_json_space(*ctx->ptr))
        ctx->ptr++;
    return *ctx->ptr;
}

static BOOL is_keyword(json_parse_ctx_t *ctx, const WCHAR *keyword)
{
    unsigned i;
    for(i=0; keyword[i]; i++) {
        if(!ctx->ptr[i] || keyword[i] != ctx->ptr[i])
            return FALSE;
    }
    if(is_identifier_char(ctx->ptr[i]))
        return FALSE;
    ctx->ptr += i;
    return TRUE;
}

/* ECMA-262 5.1 Edition    15.12.1.1 */
static HRESULT parse_json_string(json_parse_ctx_t *ctx, WCHAR **r)
{
    const WCHAR *ptr = ++ctx->ptr;
    size_t len;
    WCHAR *buf;

    while(*ctx->ptr && *ctx->ptr != '"') {
        if(*ctx->ptr++ == '\\')
            ctx->ptr++;
    }
    if(!*ctx->ptr) {
        FIXME("unterminated string\n");
        return E_FAIL;
    }

    len = ctx->ptr-ptr;
    buf = heap_alloc((len+1)*sizeof(WCHAR));
    if(!buf)
        return E_OUTOFMEMORY;
    if(len)
        memcpy(buf, ptr, len*sizeof(WCHAR));

    if(!unescape(buf, &len)) {
        FIXME("unescape failed\n");
        heap_free(buf);
        return E_FAIL;
    }

    buf[len] = 0;
    ctx->ptr++;
    *r = buf;
    return S_OK;
}

/* ECMA-262 5.1 Edition    15.12.1.2 */
static HRESULT parse_json_value(json_parse_ctx_t *ctx, jsval_t *r)
{
    HRESULT hres;

    switch(skip_spaces(ctx)) {

    /* JSONNullLiteral */
    case 'n':
        if(!is_keyword(ctx, nullW))
            break;
        *r = jsval_null();
        return S_OK;

    /* JSONBooleanLiteral */
    case 't':
        if(!is_keyword(ctx, trueW))
            break;
        *r = jsval_bool(TRUE);
        return S_OK;
    case 'f':
        if(!is_keyword(ctx, falseW))
            break;
        *r = jsval_bool(FALSE);
        return S_OK;

    /* JSONObject */
    case '{': {
        WCHAR *prop_name;
        jsdisp_t *obj;
        jsval_t val;

        hres = create_object(ctx->ctx, NULL, &obj);
        if(FAILED(hres))
            return hres;

        ctx->ptr++;
        if(skip_spaces(ctx) == '}') {
            ctx->ptr++;
            *r = jsval_obj(obj);
            return S_OK;
        }

        while(1) {
            if(*ctx->ptr != '"')
                break;
            hres = parse_json_string(ctx, &prop_name);
            if(FAILED(hres))
                break;

            if(skip_spaces(ctx) != ':') {
                FIXME("missing ':'\n");
                heap_free(prop_name);
                break;
            }

            ctx->ptr++;
            hres = parse_json_value(ctx, &val);
            if(SUCCEEDED(hres)) {
                hres = jsdisp_propput_name(obj, prop_name, val);
                jsval_release(val);
            }
            heap_free(prop_name);
            if(FAILED(hres))
                break;

            if(skip_spaces(ctx) == '}') {
                ctx->ptr++;
                *r = jsval_obj(obj);
                return S_OK;
            }

            if(*ctx->ptr++ != ',') {
                FIXME("expected ','\n");
                break;
            }
            skip_spaces(ctx);
        }

        jsdisp_release(obj);
        break;
    }

    /* JSONString */
    case '"': {
        WCHAR *string;
        jsstr_t *str;

        hres = parse_json_string(ctx, &string);
        if(FAILED(hres))
            return hres;

        /* FIXME: avoid reallocation */
        str = jsstr_alloc(string);
        heap_free(string);
        if(!str)
            return E_OUTOFMEMORY;

        *r = jsval_string(str);
        return S_OK;
    }

    /* JSONArray */
    case '[': {
        jsdisp_t *array;
        unsigned i = 0;
        jsval_t val;

        hres = create_array(ctx->ctx, 0, &array);
        if(FAILED(hres))
            return hres;

        ctx->ptr++;
        if(skip_spaces(ctx) == ']') {
            ctx->ptr++;
            *r = jsval_obj(array);
            return S_OK;
        }

        while(1) {
            hres = parse_json_value(ctx, &val);
            if(FAILED(hres))
                break;

            hres = jsdisp_propput_idx(array, i, val);
            jsval_release(val);
            if(FAILED(hres))
                break;

            if(skip_spaces(ctx) == ']') {
                ctx->ptr++;
                *r = jsval_obj(array);
                return S_OK;
            }

            if(*ctx->ptr != ',') {
                FIXME("expected ','\n");
                break;
            }

            ctx->ptr++;
            i++;
        }

        jsdisp_release(array);
        break;
    }

    /* JSONNumber */
    default: {
        int sign = 1;
        double n;

        if(*ctx->ptr == '-') {
            sign = -1;
            ctx->ptr++;
            skip_spaces(ctx);
        }

        if(*ctx->ptr == '0' && ctx->ptr + 1 < ctx->end && iswdigit(ctx->ptr[1]))
            break;

        hres = parse_decimal(&ctx->ptr, ctx->end, &n);
        if(FAILED(hres))
            break;

        *r = jsval_number(sign*n);
        return S_OK;
    }
    }

    FIXME("Syntax error at %s\n", debugstr_w(ctx->ptr));
    return E_FAIL;
}

/* ECMA-262 5.1 Edition    15.12.2 */
static HRESULT JSON_parse(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv, jsval_t *r)
{
    json_parse_ctx_t parse_ctx;
    const WCHAR *buf;
    jsstr_t *str;
    jsval_t ret;
    HRESULT hres;

    if(argc != 1) {
        FIXME("Unsupported args\n");
        return E_INVALIDARG;
    }

    hres = to_flat_string(ctx, argv[0], &str, &buf);
    if(FAILED(hres))
        return hres;

    TRACE("%s\n", debugstr_w(buf));

    parse_ctx.ptr = buf;
    parse_ctx.end = buf + jsstr_length(str);
    parse_ctx.ctx = ctx;
    hres = parse_json_value(&parse_ctx, &ret);
    jsstr_release(str);
    if(FAILED(hres))
        return hres;

    if(skip_spaces(&parse_ctx)) {
        FIXME("syntax error\n");
        jsval_release(ret);
        return E_FAIL;
    }

    if(r)
        *r = ret;
    else
        jsval_release(ret);
    return S_OK;
}

typedef struct {
    script_ctx_t *ctx;

    WCHAR *buf;
    size_t buf_size;
    size_t buf_len;

    jsdisp_t **stack;
    size_t stack_top;
    size_t stack_size;

    WCHAR gap[11]; /* according to the spec, it's no longer than 10 chars */
} stringify_ctx_t;

static BOOL stringify_push_obj(stringify_ctx_t *ctx, jsdisp_t *obj)
{
    if(!ctx->stack_size) {
        ctx->stack = heap_alloc(4*sizeof(*ctx->stack));
        if(!ctx->stack)
            return FALSE;
        ctx->stack_size = 4;
    }else if(ctx->stack_top == ctx->stack_size) {
        jsdisp_t **new_stack;

        new_stack = heap_realloc(ctx->stack, ctx->stack_size*2*sizeof(*ctx->stack));
        if(!new_stack)
            return FALSE;
        ctx->stack = new_stack;
        ctx->stack_size *= 2;
    }

    ctx->stack[ctx->stack_top++] = obj;
    return TRUE;
}

static void stringify_pop_obj(stringify_ctx_t *ctx)
{
    ctx->stack_top--;
}

static BOOL is_on_stack(stringify_ctx_t *ctx, jsdisp_t *obj)
{
    size_t i = ctx->stack_top;
    while(i--) {
        if(ctx->stack[i] == obj)
            return TRUE;
    }
    return FALSE;
}

static BOOL append_string_len(stringify_ctx_t *ctx, const WCHAR *str, size_t len)
{
    if(!ctx->buf_size) {
        ctx->buf = heap_alloc(len*2*sizeof(WCHAR));
        if(!ctx->buf)
            return FALSE;
        ctx->buf_size = len*2;
    }else if(ctx->buf_len + len > ctx->buf_size) {
        WCHAR *new_buf;
        size_t new_size;

        new_size = ctx->buf_size * 2 + len;
        new_buf = heap_realloc(ctx->buf, new_size*sizeof(WCHAR));
        if(!new_buf)
            return FALSE;
        ctx->buf = new_buf;
        ctx->buf_size = new_size;
    }

    if(len)
        memcpy(ctx->buf + ctx->buf_len, str, len*sizeof(WCHAR));
    ctx->buf_len += len;
    return TRUE;
}

static inline BOOL append_string(stringify_ctx_t *ctx, const WCHAR *str)
{
    return append_string_len(ctx, str, lstrlenW(str));
}

static inline BOOL append_char(stringify_ctx_t *ctx, WCHAR c)
{
    return append_string_len(ctx, &c, 1);
}

static inline BOOL append_simple_quote(stringify_ctx_t *ctx, WCHAR c)
{
    WCHAR str[] = {'\\',c};
    return append_string_len(ctx, str, 2);
}

static HRESULT maybe_to_primitive(script_ctx_t *ctx, jsval_t val, jsval_t *r)
{
    jsdisp_t *obj;
    HRESULT hres;

    if(!is_object_instance(val) || !get_object(val) || !(obj = iface_to_jsdisp(get_object(val))))
        return jsval_copy(val, r);

    if(is_class(obj, JSCLASS_NUMBER)) {
        double n;
        hres = to_number(ctx, val, &n);
        jsdisp_release(obj);
        if(SUCCEEDED(hres))
            *r = jsval_number(n);
        return hres;
    }

    if(is_class(obj, JSCLASS_STRING)) {
        jsstr_t *str;
        hres = to_string(ctx, val, &str);
        jsdisp_release(obj);
        if(SUCCEEDED(hres))
            *r = jsval_string(str);
        return hres;
    }

    if(is_class(obj, JSCLASS_BOOLEAN)) {
        *r = jsval_bool(bool_obj_value(obj));
        jsdisp_release(obj);
        return S_OK;
    }

    *r = jsval_obj(obj);
    return S_OK;
}

/* ECMA-262 5.1 Edition    15.12.3 (abstract operation Quote) */
static HRESULT json_quote(stringify_ctx_t *ctx, const WCHAR *ptr, size_t len)
{
    if(!ptr || !append_char(ctx, '"'))
        return E_OUTOFMEMORY;

    while(len--) {
        switch(*ptr) {
        case '"':
        case '\\':
            if(!append_simple_quote(ctx, *ptr))
                return E_OUTOFMEMORY;
            break;
        case '\b':
            if(!append_simple_quote(ctx, 'b'))
                return E_OUTOFMEMORY;
            break;
        case '\f':
            if(!append_simple_quote(ctx, 'f'))
                return E_OUTOFMEMORY;
            break;
        case '\n':
            if(!append_simple_quote(ctx, 'n'))
                return E_OUTOFMEMORY;
            break;
        case '\r':
            if(!append_simple_quote(ctx, 'r'))
                return E_OUTOFMEMORY;
            break;
        case '\t':
            if(!append_simple_quote(ctx, 't'))
                return E_OUTOFMEMORY;
            break;
        default:
            if(*ptr < ' ') {
                static const WCHAR formatW[] = {'\\','u','%','0','4','x',0};
                WCHAR buf[7];
                swprintf(buf, formatW, *ptr);
                if(!append_string(ctx, buf))
                    return E_OUTOFMEMORY;
            }else {
                if(!append_char(ctx, *ptr))
                    return E_OUTOFMEMORY;
            }
        }
        ptr++;
    }

    return append_char(ctx, '"') ? S_OK : E_OUTOFMEMORY;
}

static inline BOOL is_callable(jsdisp_t *obj)
{
    return is_class(obj, JSCLASS_FUNCTION);
}

static HRESULT stringify(stringify_ctx_t *ctx, jsval_t val);

/* ECMA-262 5.1 Edition    15.12.3 (abstract operation JA) */
static HRESULT stringify_array(stringify_ctx_t *ctx, jsdisp_t *obj)
{
    unsigned length, i, j;
    jsval_t val;
    HRESULT hres;

    if(is_on_stack(ctx, obj)) {
        FIXME("Found a cycle\n");
        return E_FAIL;
    }

    if(!stringify_push_obj(ctx, obj))
        return E_OUTOFMEMORY;

    if(!append_char(ctx, '['))
        return E_OUTOFMEMORY;

    length = array_get_length(obj);

    for(i=0; i < length; i++) {
        if(i && !append_char(ctx, ','))
            return E_OUTOFMEMORY;

        if(*ctx->gap) {
            if(!append_char(ctx, '\n'))
                return E_OUTOFMEMORY;

            for(j=0; j < ctx->stack_top; j++) {
                if(!append_string(ctx, ctx->gap))
                    return E_OUTOFMEMORY;
            }
        }

        hres = jsdisp_get_idx(obj, i, &val);
        if(SUCCEEDED(hres)) {
            hres = stringify(ctx, val);
            if(FAILED(hres))
                return hres;
            if(hres == S_FALSE && !append_string(ctx, nullW))
                return E_OUTOFMEMORY;
        }else if(hres == DISP_E_UNKNOWNNAME) {
            if(!append_string(ctx, nullW))
                return E_OUTOFMEMORY;
        }else {
            return hres;
        }
    }

    if((length && *ctx->gap && !append_char(ctx, '\n')) || !append_char(ctx, ']'))
        return E_OUTOFMEMORY;

    stringify_pop_obj(ctx);
    return S_OK;
}

/* ECMA-262 5.1 Edition    15.12.3 (abstract operation JO) */
static HRESULT stringify_object(stringify_ctx_t *ctx, jsdisp_t *obj)
{
    DISPID dispid = DISPID_STARTENUM;
    jsval_t val = jsval_undefined();
    unsigned prop_cnt = 0, i;
    size_t stepback;
    BSTR prop_name;
    HRESULT hres;

    if(is_on_stack(ctx, obj)) {
        FIXME("Found a cycle\n");
        return E_FAIL;
    }

    if(!stringify_push_obj(ctx, obj))
        return E_OUTOFMEMORY;

    if(!append_char(ctx, '{'))
        return E_OUTOFMEMORY;

    while((hres = IDispatchEx_GetNextDispID(&obj->IDispatchEx_iface, fdexEnumDefault, dispid, &dispid)) == S_OK) {
        jsval_release(val);
        hres = jsdisp_propget(obj, dispid, &val);
        if(FAILED(hres))
            return hres;

        if(is_undefined(val))
            continue;

        stepback = ctx->buf_len;

        if(prop_cnt && !append_char(ctx, ',')) {
            hres = E_OUTOFMEMORY;
            break;
        }

        if(*ctx->gap) {
            if(!append_char(ctx, '\n')) {
                hres = E_OUTOFMEMORY;
                break;
            }

            for(i=0; i < ctx->stack_top; i++) {
                if(!append_string(ctx, ctx->gap)) {
                    hres = E_OUTOFMEMORY;
                    break;
                }
            }
        }

        hres = IDispatchEx_GetMemberName(&obj->IDispatchEx_iface, dispid, &prop_name);
        if(FAILED(hres))
            break;

        hres = json_quote(ctx, prop_name, SysStringLen(prop_name));
        SysFreeString(prop_name);
        if(FAILED(hres))
            break;

        if(!append_char(ctx, ':') || (*ctx->gap && !append_char(ctx, ' '))) {
            hres = E_OUTOFMEMORY;
            break;
        }

        hres = stringify(ctx, val);
        if(FAILED(hres))
            break;

        if(hres == S_FALSE) {
            ctx->buf_len = stepback;
            continue;
        }

        prop_cnt++;
    }
    jsval_release(val);
    if(FAILED(hres))
        return hres;

    if(prop_cnt && *ctx->gap) {
        if(!append_char(ctx, '\n'))
            return E_OUTOFMEMORY;

        for(i=1; i < ctx->stack_top; i++) {
            if(!append_string(ctx, ctx->gap)) {
                hres = E_OUTOFMEMORY;
                break;
            }
        }
    }

    if(!append_char(ctx, '}'))
        return E_OUTOFMEMORY;

    stringify_pop_obj(ctx);
    return S_OK;
}

/* ECMA-262 5.1 Edition    15.12.3 (abstract operation Str) */
static HRESULT stringify(stringify_ctx_t *ctx, jsval_t val)
{
    jsval_t value;
    HRESULT hres;

    if(is_object_instance(val) && get_object(val)) {
        jsdisp_t *obj;
        DISPID id;

        obj = iface_to_jsdisp(get_object(val));
        if(!obj)
            return S_FALSE;

        hres = jsdisp_get_id(obj, toJSONW, 0, &id);
        jsdisp_release(obj);
        if(hres == S_OK)
            FIXME("Use toJSON.\n");
    }

    /* FIXME: Support replacer replacer. */

    hres = maybe_to_primitive(ctx->ctx, val, &value);
    if(FAILED(hres))
        return hres;

    switch(jsval_type(value)) {
    case JSV_NULL:
        if(!append_string(ctx, nullW))
            hres = E_OUTOFMEMORY;
        break;
    case JSV_BOOL:
        if(!append_string(ctx, get_bool(value) ? trueW : falseW))
            hres = E_OUTOFMEMORY;
        break;
    case JSV_STRING: {
        jsstr_t *str = get_string(value);
        const WCHAR *ptr = jsstr_flatten(str);
        if(ptr)
            hres = json_quote(ctx, ptr, jsstr_length(str));
        else
            hres = E_OUTOFMEMORY;
        break;
    }
    case JSV_NUMBER: {
        double n = get_number(value);
        if(is_finite(n)) {
            const WCHAR *ptr;
            jsstr_t *str;

            /* FIXME: Optimize. There is no need for jsstr_t here. */
            hres = double_to_string(n, &str);
            if(FAILED(hres))
                break;

            ptr = jsstr_flatten(str);
            assert(ptr != NULL);
            hres = ptr && !append_string_len(ctx, ptr, jsstr_length(str)) ? E_OUTOFMEMORY : S_OK;
            jsstr_release(str);
        }else {
            if(!append_string(ctx, nullW))
                hres = E_OUTOFMEMORY;
        }
        break;
    }
    case JSV_OBJECT: {
        jsdisp_t *obj;

        obj = iface_to_jsdisp(get_object(value));
        if(!obj) {
            hres = S_FALSE;
            break;
        }

        if(!is_callable(obj))
            hres = is_class(obj, JSCLASS_ARRAY) ? stringify_array(ctx, obj) : stringify_object(ctx, obj);
        else
            hres = S_FALSE;

        jsdisp_release(obj);
        break;
    }
    case JSV_UNDEFINED:
        hres = S_FALSE;
        break;
    case JSV_VARIANT:
        FIXME("VARIANT\n");
        hres = E_NOTIMPL;
        break;
    }

    jsval_release(value);
    return hres;
}

/* ECMA-262 5.1 Edition    15.12.3 */
static HRESULT JSON_stringify(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv, jsval_t *r)
{
    stringify_ctx_t stringify_ctx = {ctx, NULL,0,0, NULL,0,0, {0}};
    HRESULT hres;

    TRACE("\n");

    if(!argc) {
        if(r)
            *r = jsval_undefined();
        return S_OK;
    }

    if(argc >= 2 && is_object_instance(argv[1])) {
        FIXME("Replacer %s not yet supported\n", debugstr_jsval(argv[1]));
        return E_NOTIMPL;
    }

    if(argc >= 3) {
        jsval_t space_val;

        hres = maybe_to_primitive(ctx, argv[2], &space_val);
        if(FAILED(hres))
            return hres;

        if(is_number(space_val)) {
            double n = get_number(space_val);
            if(n >= 1) {
                int i, len;
                if(n > 10)
                    n = 10;
                len = floor(n);
                for(i=0; i < len; i++)
                    stringify_ctx.gap[i] = ' ';
                stringify_ctx.gap[len] = 0;
            }
        }else if(is_string(space_val)) {
            jsstr_t *space_str = get_string(space_val);
            size_t len = jsstr_length(space_str);
            if(len > 10)
                len = 10;
            jsstr_extract(space_str, 0, len, stringify_ctx.gap);
        }

        jsval_release(space_val);
    }

    hres = stringify(&stringify_ctx, argv[0]);
    if(SUCCEEDED(hres) && r) {
        assert(!stringify_ctx.stack_top);

        if(hres == S_OK) {
            jsstr_t *ret = jsstr_alloc_len(stringify_ctx.buf, stringify_ctx.buf_len);
            if(ret)
                *r = jsval_string(ret);
            else
                hres = E_OUTOFMEMORY;
        }else {
            *r = jsval_undefined();
        }
    }

    heap_free(stringify_ctx.buf);
    heap_free(stringify_ctx.stack);
    return hres;
}

static const builtin_prop_t JSON_props[] = {
    {parseW,     JSON_parse,     PROPF_METHOD|2},
    {stringifyW, JSON_stringify, PROPF_METHOD|3}
};

static const builtin_info_t JSON_info = {
    JSCLASS_JSON,
    {NULL, NULL, 0},
    ARRAY_SIZE(JSON_props),
    JSON_props,
    NULL,
    NULL
};

HRESULT create_json(script_ctx_t *ctx, jsdisp_t **ret)
{
    jsdisp_t *json;
    HRESULT hres;

    json = heap_alloc_zero(sizeof(*json));
    if(!json)
        return E_OUTOFMEMORY;

    hres = init_dispex_from_constr(json, ctx, &JSON_info, ctx->object_constr);
    if(FAILED(hres)) {
        heap_free(json);
        return hres;
    }

    *ret = json;
    return S_OK;
}
