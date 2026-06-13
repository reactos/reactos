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

#ifdef __REACTOS__
#include <wine/config.h>
#include <wine/port.h>
#endif

#include <math.h>
#include <assert.h>

#include "jscript.h"
#include "parser.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

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

static BOOL unescape_json_string(WCHAR *str, size_t *len)
{
    WCHAR *pd, *p, c, *end = str + *len;
    int i;

    pd = p = str;
    while(p < end) {
        if(*p != '\\') {
            *pd++ = *p++;
            continue;
        }

        if(++p == end)
            return FALSE;

        switch(*p) {
        case '\"':
        case '\\':
        case '/':
            c = *p;
            break;
        case 'b':
            c = '\b';
            break;
        case 't':
            c = '\t';
            break;
        case 'n':
            c = '\n';
            break;
        case 'f':
            c = '\f';
            break;
        case 'r':
            c = '\r';
            break;
        case 'u':
            if(p + 4 >= end)
                return FALSE;
            i = hex_to_int(*++p);
            if(i == -1)
                return FALSE;
            c = i << 12;

            i = hex_to_int(*++p);
            if(i == -1)
                return FALSE;
            c += i << 8;

            i = hex_to_int(*++p);
            if(i == -1)
                return FALSE;
            c += i << 4;

            i = hex_to_int(*++p);
            if(i == -1)
                return FALSE;
            c += i;
            break;
        default:
            return FALSE;
        }

        *pd++ = c;
        p++;
    }

    *len = pd - str;
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
    buf = malloc((len+1)*sizeof(WCHAR));
    if(!buf)
        return E_OUTOFMEMORY;
    if(len)
        memcpy(buf, ptr, len*sizeof(WCHAR));

    if(!(ctx->ctx->html_mode ? unescape_json_string(buf, &len) : unescape(buf, &len))) {
        WARN("unescape failed\n");
        free(buf);
        return JS_E_INVALID_CHAR;
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
        if(!is_keyword(ctx, L"null"))
            break;
        *r = jsval_null();
        return S_OK;

    /* JSONBooleanLiteral */
    case 't':
        if(!is_keyword(ctx, L"true"))
            break;
        *r = jsval_bool(TRUE);
        return S_OK;
    case 'f':
        if(!is_keyword(ctx, L"false"))
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
                free(prop_name);
                break;
            }

            ctx->ptr++;
            hres = parse_json_value(ctx, &val);
            if(SUCCEEDED(hres)) {
                hres = jsdisp_propput_name(obj, prop_name, val);
                jsval_release(val);
            }
            free(prop_name);
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
        free(string);
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

        if(*ctx->ptr == '0' && ctx->ptr + 1 < ctx->end && is_digit(ctx->ptr[1]))
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

struct transform_json_object_ctx
{
    script_ctx_t *ctx;
    IDispatch *reviver;
    HRESULT hres;
};

static jsval_t transform_json_object(struct transform_json_object_ctx *proc_ctx, jsdisp_t *holder, jsstr_t *name)
{
    jsval_t res, args[2];
    const WCHAR *str;

    if(!(str = jsstr_flatten(name)))
        proc_ctx->hres = E_OUTOFMEMORY;
    else
        proc_ctx->hres = jsdisp_propget_name(holder, str, &args[1]);
    if(FAILED(proc_ctx->hres))
        return jsval_undefined();

    if(is_object_instance(args[1])) {
        jsdisp_t *obj = to_jsdisp(get_object(args[1]));
        jsstr_t *jsstr;
        DISPID id;
        BOOL b;

        if(!obj) {
            FIXME("non-JS obj in JSON object: %p\n", get_object(args[1]));
            proc_ctx->hres = E_NOTIMPL;
            goto ret;
        }else if(is_class(obj, JSCLASS_ARRAY)) {
            unsigned i, length = array_get_length(obj);
            WCHAR buf[14], *buf_end;

            buf_end = buf + ARRAY_SIZE(buf) - 1;
            *buf_end-- = 0;
            for(i = 0; i < length; i++) {
                str = idx_to_str(i, buf_end);
                if(!(jsstr = jsstr_alloc(str))) {
                    proc_ctx->hres = E_OUTOFMEMORY;
                    goto ret;
                }
                res = transform_json_object(proc_ctx, obj, jsstr);
                jsstr_release(jsstr);
                if(is_undefined(res)) {
                    if(FAILED(proc_ctx->hres))
                        goto ret;
                    if(FAILED(jsdisp_get_id(obj, str, 0, &id)))
                        continue;
                    proc_ctx->hres = disp_delete(to_disp(obj), id, &b);
                }else {
                    proc_ctx->hres = jsdisp_define_data_property(obj, str, PROPF_WRITABLE | PROPF_ENUMERABLE | PROPF_CONFIGURABLE, res);
                    jsval_release(res);
                }
                if(FAILED(proc_ctx->hres))
                    goto ret;
            }
        }else {
            id = DISPID_STARTENUM;
            for(;;) {
                proc_ctx->hres = jsdisp_next_prop(obj, id, JSDISP_ENUM_OWN_ENUMERABLE, &id);
                if(proc_ctx->hres == S_FALSE)
                    break;
                if(FAILED(proc_ctx->hres) || FAILED(proc_ctx->hres = jsdisp_get_prop_name(obj, id, &jsstr)))
                    goto ret;
                res = transform_json_object(proc_ctx, obj, jsstr);
                if(is_undefined(res)) {
                    if(SUCCEEDED(proc_ctx->hres))
                        proc_ctx->hres = disp_delete(to_disp(obj), id, &b);
                }else {
                    if(!(str = jsstr_flatten(jsstr)))
                        proc_ctx->hres = E_OUTOFMEMORY;
                    else
                        proc_ctx->hres = jsdisp_define_data_property(obj, str, PROPF_WRITABLE | PROPF_ENUMERABLE | PROPF_CONFIGURABLE, res);
                    jsval_release(res);
                }
                jsstr_release(jsstr);
                if(FAILED(proc_ctx->hres))
                    goto ret;
            }
        }
    }

    args[0] = jsval_string(name);
    proc_ctx->hres = disp_call_value(proc_ctx->ctx, proc_ctx->reviver, jsval_obj(holder),
                                     DISPATCH_METHOD, ARRAY_SIZE(args), args, &res);
ret:
    jsval_release(args[1]);
    return FAILED(proc_ctx->hres) ? jsval_undefined() : res;
}

/* ECMA-262 5.1 Edition    15.12.2 */
static HRESULT JSON_parse(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv, jsval_t *r)
{
    json_parse_ctx_t parse_ctx;
    const WCHAR *buf;
    jsdisp_t *root;
    jsstr_t *str;
    jsval_t ret;
    HRESULT hres;

    hres = to_flat_string(ctx, argv[0], &str, &buf);
    if(FAILED(hres))
        return hres;

    TRACE("%s\n", debugstr_w(buf));

    parse_ctx.ptr = buf;
    parse_ctx.end = buf + jsstr_length(str);
    parse_ctx.ctx = ctx;
    hres = parse_json_value(&parse_ctx, &ret);
    if(SUCCEEDED(hres) && skip_spaces(&parse_ctx)) {
        FIXME("syntax error\n");
        jsval_release(ret);
        hres = E_FAIL;
    }
    jsstr_release(str);
    if(FAILED(hres))
        return hres;

    /* FIXME: check IsCallable */
    if(argc > 1 && is_object_instance(argv[1])) {
        hres = create_object(ctx, NULL, &root);
        if(FAILED(hres)) {
            jsval_release(ret);
            return hres;
        }
        hres = jsdisp_define_data_property(root, L"", PROPF_WRITABLE | PROPF_ENUMERABLE | PROPF_CONFIGURABLE, ret);
        jsval_release(ret);

        if(SUCCEEDED(hres)) {
            struct transform_json_object_ctx proc_ctx = { ctx, get_object(argv[1]), S_OK };

            str = jsstr_empty();
            ret = transform_json_object(&proc_ctx, root, str);
            jsstr_release(str);
            hres = proc_ctx.hres;
        }
        jsdisp_release(root);
        if(FAILED(hres))
            return hres;
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

    jsdisp_t *replacer;
} stringify_ctx_t;

static BOOL stringify_push_obj(stringify_ctx_t *ctx, jsdisp_t *obj)
{
    if(!ctx->stack_size) {
        ctx->stack = malloc(4*sizeof(*ctx->stack));
        if(!ctx->stack)
            return FALSE;
        ctx->stack_size = 4;
    }else if(ctx->stack_top == ctx->stack_size) {
        jsdisp_t **new_stack;

        new_stack = realloc(ctx->stack, ctx->stack_size*2*sizeof(*ctx->stack));
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
        ctx->buf = malloc(len*2*sizeof(WCHAR));
        if(!ctx->buf)
            return FALSE;
        ctx->buf_size = len*2;
    }else if(ctx->buf_len + len > ctx->buf_size) {
        WCHAR *new_buf;
        size_t new_size;

        new_size = ctx->buf_size * 2 + len;
        new_buf = realloc(ctx->buf, new_size*sizeof(WCHAR));
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

    if(!is_object_instance(val) || !(obj = iface_to_jsdisp(get_object(val))))
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
                WCHAR buf[7];
                swprintf(buf, ARRAY_SIZE(buf), L"\\u%04x", *ptr);
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

static HRESULT stringify(stringify_ctx_t *ctx, jsdisp_t *object, const WCHAR *name);

/* ECMA-262 5.1 Edition    15.12.3 (abstract operation JA) */
static HRESULT stringify_array(stringify_ctx_t *ctx, jsdisp_t *obj)
{
    unsigned length, i, j;
    WCHAR name[16];
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

        _itow(i, name, ARRAY_SIZE(name));
        hres = stringify(ctx, obj, name);
        if(FAILED(hres))
            return hres;
        if(hres == S_FALSE && !append_string(ctx, L"null"))
            return E_OUTOFMEMORY;
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

    while((hres = IDispatchEx_GetNextDispID(to_dispex(obj), fdexEnumDefault, dispid, &dispid)) == S_OK) {
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

        hres = IDispatchEx_GetMemberName(to_dispex(obj), dispid, &prop_name);
        if(FAILED(hres))
            return hres;

        hres = json_quote(ctx, prop_name, SysStringLen(prop_name));
        if(FAILED(hres)) {
            SysFreeString(prop_name);
            return hres;
        }

        if(!append_char(ctx, ':') || (*ctx->gap && !append_char(ctx, ' '))) {
            SysFreeString(prop_name);
            return E_OUTOFMEMORY;
        }

        hres = stringify(ctx, obj, prop_name);
        SysFreeString(prop_name);
        if(FAILED(hres))
            return hres;

        if(hres == S_FALSE) {
            ctx->buf_len = stepback;
            continue;
        }

        prop_cnt++;
    }

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
static HRESULT stringify(stringify_ctx_t *ctx, jsdisp_t *object, const WCHAR *name)
{
    jsval_t value, v;
    HRESULT hres;

    hres = jsdisp_propget_name(object, name, &value);
    if(FAILED(hres))
        return hres == DISP_E_UNKNOWNNAME ? S_FALSE : hres;

    if(is_object_instance(value)) {
        jsdisp_t *obj;
        DISPID id;

        obj = to_jsdisp(get_object(value));
        if(!obj) {
            jsval_release(value);
            return S_FALSE;
        }

        hres = jsdisp_get_id(obj, L"toJSON", 0, &id);
        if(hres == S_OK)
            FIXME("Use toJSON.\n");
    }

    if(ctx->replacer) {
        jsstr_t *name_str;
        jsval_t args[2];
        if(!(name_str = jsstr_alloc(name))) {
            jsval_release(value);
            return E_OUTOFMEMORY;
        }
        args[0] = jsval_string(name_str);
        args[1] = value;
        hres = jsdisp_call_value(ctx->replacer, jsval_obj(object), DISPATCH_METHOD, ARRAY_SIZE(args), args, &v);
        jsstr_release(name_str);
        jsval_release(value);
        if(FAILED(hres))
            return hres;
        value = v;
    }

    v = value;
    hres = maybe_to_primitive(ctx->ctx, v, &value);
    jsval_release(v);
    if(FAILED(hres))
        return hres;

    switch(jsval_type(value)) {
    case JSV_NULL:
        if(is_null_disp(value))
            hres = S_FALSE;
        else if(!append_string(ctx, L"null"))
            hres = E_OUTOFMEMORY;
        break;
    case JSV_BOOL:
        if(!append_string(ctx, get_bool(value) ? L"true" : L"false"))
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
        if(isfinite(n)) {
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
            if(!append_string(ctx, L"null"))
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
static HRESULT JSON_stringify(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv, jsval_t *r)
{
    stringify_ctx_t stringify_ctx = { ctx };
    jsdisp_t *obj = NULL, *replacer;
    HRESULT hres;

    TRACE("\n");

    if(!argc) {
        if(r)
            *r = jsval_undefined();
        return S_OK;
    }

    if(argc >= 2 && is_object_instance(argv[1]) && (replacer = to_jsdisp(get_object(argv[1])))) {
        if(is_callable(replacer)) {
            stringify_ctx.replacer = jsdisp_addref(replacer);
        }else if(is_class(replacer, JSCLASS_ARRAY)) {
            FIXME("Array replacer not yet supported\n");
            return E_NOTIMPL;
        }
    }

    if(argc >= 3) {
        jsval_t space_val;

        hres = maybe_to_primitive(ctx, argv[2], &space_val);
        if(FAILED(hres))
            goto fail;

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

    if(FAILED(hres = create_object(ctx, NULL, &obj)))
        goto fail;
    if(FAILED(hres = jsdisp_propput_name(obj, L"", argv[0])))
        goto fail;

    hres = stringify(&stringify_ctx, obj, L"");
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

fail:
    if(obj)
        jsdisp_release(obj);
    if(stringify_ctx.replacer)
        jsdisp_release(stringify_ctx.replacer);
    free(stringify_ctx.buf);
    free(stringify_ctx.stack);
    return hres;
}

static const builtin_prop_t JSON_props[] = {
    {L"parse",     JSON_parse,     PROPF_METHOD|2},
    {L"stringify", JSON_stringify, PROPF_METHOD|3}
};

static const builtin_info_t JSON_info = {
    .class     = JSCLASS_JSON,
    .props_cnt = ARRAY_SIZE(JSON_props),
    .props     = JSON_props,
};

HRESULT create_json(script_ctx_t *ctx, jsdisp_t **ret)
{
    jsdisp_t *json;
    HRESULT hres;

    json = calloc(1, sizeof(*json));
    if(!json)
        return E_OUTOFMEMORY;

    hres = init_dispex_from_constr(json, ctx, &JSON_info, ctx->object_constr);
    if(FAILED(hres)) {
        free(json);
        return hres;
    }

    *ret = json;
    return S_OK;
}
