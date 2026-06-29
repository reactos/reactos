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
    jsstr_t *str;
} StringInstance;

static inline StringInstance *string_from_jsdisp(jsdisp_t *jsdisp)
{
    return CONTAINING_RECORD(jsdisp, StringInstance, dispex);
}

static inline StringInstance *string_this(jsval_t vthis)
{
    jsdisp_t *jsdisp = is_object_instance(vthis) ? to_jsdisp(get_object(vthis)) : NULL;
    return (jsdisp && is_class(jsdisp, JSCLASS_STRING)) ? string_from_jsdisp(jsdisp) : NULL;
}

static HRESULT get_string_val(script_ctx_t *ctx, jsval_t vthis, jsstr_t **val)
{
    StringInstance *string;

    if(ctx->version >= SCRIPTLANGUAGEVERSION_ES5 && (is_undefined(vthis) || is_null(vthis)))
        return JS_E_OBJECT_EXPECTED;

    if((string = string_this(vthis))) {
        *val = jsstr_addref(string->str);
        return S_OK;
    }

    return to_string(ctx, vthis, val);
}

static HRESULT get_string_flat_val(script_ctx_t *ctx, jsval_t vthis, jsstr_t **jsval, const WCHAR **val)
{
    HRESULT hres;

    hres = get_string_val(ctx, vthis, jsval);
    if(FAILED(hres))
        return hres;

    *val = jsstr_flatten(*jsval);
    if(*val)
        return S_OK;

    jsstr_release(*jsval);
    return E_OUTOFMEMORY;
}

static HRESULT String_get_length(script_ctx_t *ctx, jsdisp_t *jsthis, jsval_t *r)
{
    StringInstance *string = string_from_jsdisp(jsthis);

    TRACE("%p\n", jsthis);

    *r = jsval_number(jsstr_length(string->str));
    return S_OK;
}

static HRESULT stringobj_to_string(jsval_t vthis, jsval_t *r)
{
    StringInstance *string;

    if(!(string = string_this(vthis))) {
        WARN("this is not a string object\n");
        return E_FAIL;
    }

    if(r)
        *r = jsval_string(jsstr_addref(string->str));
    return S_OK;
}

/* ECMA-262 3rd Edition    15.5.4.2 */
static HRESULT String_toString(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    TRACE("\n");

    return stringobj_to_string(vthis, r);
}

/* ECMA-262 3rd Edition    15.5.4.2 */
static HRESULT String_valueOf(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    TRACE("\n");

    return stringobj_to_string(vthis, r);
}

static HRESULT do_attributeless_tag_format(script_ctx_t *ctx, jsval_t vthis, jsval_t *r, const WCHAR *tagname)
{
    unsigned tagname_len;
    jsstr_t *str, *ret;
    WCHAR *ptr;
    HRESULT hres;

    hres = get_string_val(ctx, vthis, &str);
    if(FAILED(hres))
        return hres;

    if(!r) {
        jsstr_release(str);
        return S_OK;
    }

    tagname_len = lstrlenW(tagname);

    ret = jsstr_alloc_buf(jsstr_length(str) + 2*tagname_len + 5, &ptr);
    if(!ret) {
        jsstr_release(str);
        return E_OUTOFMEMORY;
    }

    *ptr++ = '<';
    memcpy(ptr, tagname, tagname_len*sizeof(WCHAR));
    ptr += tagname_len;
    *ptr++ = '>';

    ptr += jsstr_flush(str, ptr);
    jsstr_release(str);

    *ptr++ = '<';
    *ptr++ = '/';
    memcpy(ptr, tagname, tagname_len*sizeof(WCHAR));
    ptr += tagname_len;
    *ptr = '>';

    *r = jsval_string(ret);
    return S_OK;
}

static HRESULT do_attribute_tag_format(script_ctx_t *ctx, jsval_t vthis, unsigned argc, jsval_t *argv, jsval_t *r,
        const WCHAR *tagname, const WCHAR *attrname)
{
    jsstr_t *str, *attr_value = NULL;
    HRESULT hres;

    hres = get_string_val(ctx, vthis, &str);
    if(FAILED(hres))
        return hres;

    if(argc) {
        hres = to_string(ctx, argv[0], &attr_value);
        if(FAILED(hres)) {
            jsstr_release(str);
            return hres;
        }
    }else {
        attr_value = jsstr_undefined();
    }

    if(r) {
        unsigned attrname_len = lstrlenW(attrname);
        unsigned tagname_len = lstrlenW(tagname);
        jsstr_t *ret;
        WCHAR *ptr;

        ret = jsstr_alloc_buf(2*tagname_len + attrname_len + jsstr_length(attr_value) + jsstr_length(str) + 9, &ptr);
        if(ret) {
            *ptr++ = '<';
            memcpy(ptr, tagname, tagname_len*sizeof(WCHAR));
            ptr += tagname_len;
            *ptr++ = ' ';
            memcpy(ptr, attrname, attrname_len*sizeof(WCHAR));
            ptr += attrname_len;
            *ptr++ = '=';
            *ptr++ = '"';
            ptr += jsstr_flush(attr_value, ptr);
            *ptr++ = '"';
            *ptr++ = '>';
            ptr += jsstr_flush(str, ptr);

            *ptr++ = '<';
            *ptr++ = '/';
            memcpy(ptr, tagname, tagname_len*sizeof(WCHAR));
            ptr += tagname_len;
            *ptr = '>';

            *r = jsval_string(ret);
        }else {
            hres = E_OUTOFMEMORY;
        }
    }

    jsstr_release(attr_value);
    jsstr_release(str);
    return hres;
}

static HRESULT String_anchor(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    return do_attribute_tag_format(ctx, vthis, argc, argv, r, L"A", L"NAME");
}

static HRESULT String_big(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    return do_attributeless_tag_format(ctx, vthis, r, L"BIG");
}

static HRESULT String_blink(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    return do_attributeless_tag_format(ctx, vthis, r, L"BLINK");
}

static HRESULT String_bold(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    return do_attributeless_tag_format(ctx, vthis, r, L"B");
}

/* ECMA-262 3rd Edition    15.5.4.5 */
static HRESULT String_charAt(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsstr_t *str, *ret;
    INT pos = 0;
    HRESULT hres;

    TRACE("\n");

    hres = get_string_val(ctx, vthis, &str);
    if(FAILED(hres))
        return hres;

    if(argc) {
        double d;

        hres = to_integer(ctx, argv[0], &d);
        if(FAILED(hres)) {
            jsstr_release(str);
            return hres;
        }
        pos = is_int32(d) ? d : -1;
    }

    if(!r) {
        jsstr_release(str);
        return S_OK;
    }

    if(0 <= pos && pos < jsstr_length(str)) {
        ret = jsstr_substr(str, pos, 1);
        if(!ret)
            return E_OUTOFMEMORY;
    }else {
        ret = jsstr_empty();
    }

    *r = jsval_string(ret);
    return S_OK;
}

/* ECMA-262 3rd Edition    15.5.4.5 */
static HRESULT String_charCodeAt(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsstr_t *str;
    DWORD idx = 0;
    HRESULT hres;

    TRACE("\n");

    hres = get_string_val(ctx, vthis, &str);
    if(FAILED(hres))
        return hres;

    if(argc > 0) {
        double d;

        hres = to_integer(ctx, argv[0], &d);
        if(FAILED(hres)) {
            jsstr_release(str);
            return hres;
        }

        if(!is_int32(d) || d < 0 || d >= jsstr_length(str)) {
            jsstr_release(str);
            if(r)
                *r = jsval_number(NAN);
            return S_OK;
        }

        idx = d;
    }

    if(r) {
        WCHAR c;
        jsstr_extract(str, idx, 1, &c);
        *r = jsval_number(c);
    }

    jsstr_release(str);
    return S_OK;
}

/* ECMA-262 3rd Edition    15.5.4.6 */
static HRESULT String_concat(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsstr_t *ret = NULL, *str;
    HRESULT hres;

    TRACE("\n");

    hres = get_string_val(ctx, vthis, &str);
    if(FAILED(hres))
        return hres;

    switch(argc) {
    case 0:
        ret = str;
        break;
    case 1: {
        jsstr_t *arg_str;

        hres = to_string(ctx, argv[0], &arg_str);
        if(FAILED(hres)) {
            jsstr_release(str);
            return hres;
        }

        ret = jsstr_concat(str, arg_str);
        jsstr_release(str);
        if(!ret)
            return E_OUTOFMEMORY;
        break;
    }
    default: {
        const unsigned str_cnt = argc+1;
        unsigned len = 0, i;
        jsstr_t **strs;
        WCHAR *ptr;

        strs = calloc(str_cnt, sizeof(*strs));
        if(!strs) {
            jsstr_release(str);
            return E_OUTOFMEMORY;
        }

        strs[0] = str;
        for(i=0; i < argc; i++) {
            hres = to_string(ctx, argv[i], strs+i+1);
            if(FAILED(hres))
                break;
        }

        if(SUCCEEDED(hres)) {
            for(i=0; i < str_cnt; i++) {
                len += jsstr_length(strs[i]);
                if(len > JSSTR_MAX_LENGTH) {
                    hres = E_OUTOFMEMORY;
                    break;
                }
            }

            if(SUCCEEDED(hres)) {
                ret = jsstr_alloc_buf(len, &ptr);
                if(ret) {
                    for(i=0; i < str_cnt; i++)
                        ptr += jsstr_flush(strs[i], ptr);
                }else {
                    hres = E_OUTOFMEMORY;
                }
            }
        }

        while(i--)
            jsstr_release(strs[i]);
        free(strs);
        if(FAILED(hres))
            return hres;
    }
    }

    if(r)
        *r = jsval_string(ret);
    else
        jsstr_release(ret);
    return S_OK;
}

static HRESULT String_fixed(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    return do_attributeless_tag_format(ctx, vthis, r, L"TT");
}

static HRESULT String_fontcolor(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    return do_attribute_tag_format(ctx, vthis, argc, argv, r, L"FONT", L"COLOR");
}

static HRESULT String_fontsize(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    return do_attribute_tag_format(ctx, vthis, argc, argv, r, L"FONT", L"SIZE");
}

static HRESULT String_indexOf(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    unsigned pos = 0, search_len, length;
    jsstr_t *search_jsstr, *jsstr;
    const WCHAR *search_str, *str;
    INT ret = -1;
    HRESULT hres;

    TRACE("\n");

    hres = get_string_flat_val(ctx, vthis, &jsstr, &str);
    if(FAILED(hres))
        return hres;

    if(!argc) {
        if(r)
            *r = jsval_number(-1);
        jsstr_release(jsstr);
        return S_OK;
    }

    hres = to_flat_string(ctx, argv[0], &search_jsstr, &search_str);
    if(FAILED(hres)) {
        jsstr_release(jsstr);
        return hres;
    }

    search_len = jsstr_length(search_jsstr);
    length = jsstr_length(jsstr);

    if(argc >= 2) {
        double d;

        hres = to_integer(ctx, argv[1], &d);
        if(SUCCEEDED(hres) && d > 0.0)
            pos = is_int32(d) ? min(length, d) : length;
    }

    if(SUCCEEDED(hres) && length >= search_len) {
        const WCHAR *end = str+length-search_len;
        const WCHAR *ptr;

        for(ptr = str+pos; ptr <= end; ptr++) {
            if(!memcmp(ptr, search_str, search_len*sizeof(WCHAR))) {
                ret = ptr-str;
                break;
            }
        }
    }

    jsstr_release(search_jsstr);
    jsstr_release(jsstr);
    if(FAILED(hres))
        return hres;

    if(r)
        *r = jsval_number(ret);
    return S_OK;
}

static HRESULT String_italics(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    return do_attributeless_tag_format(ctx, vthis, r, L"I");
}

/* ECMA-262 3rd Edition    15.5.4.8 */
static HRESULT String_lastIndexOf(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    unsigned pos = 0, search_len, length;
    jsstr_t *search_jsstr, *jsstr;
    const WCHAR *search_str, *str;
    INT ret = -1;
    HRESULT hres;

    TRACE("\n");

    hres = get_string_flat_val(ctx, vthis, &jsstr, &str);
    if(FAILED(hres))
        return hres;

    if(!argc) {
        if(r)
            *r = jsval_number(-1);
        jsstr_release(jsstr);
        return S_OK;
    }

    hres = to_flat_string(ctx, argv[0], &search_jsstr, &search_str);
    if(FAILED(hres)) {
        jsstr_release(jsstr);
        return hres;
    }

    search_len = jsstr_length(search_jsstr);
    length = jsstr_length(jsstr);

    if(argc >= 2) {
        double d;

        hres = to_integer(ctx, argv[1], &d);
        if(SUCCEEDED(hres) && d > 0)
            pos = is_int32(d) ? min(length, d) : length;
    }else {
        pos = length;
    }

    if(SUCCEEDED(hres) && length >= search_len) {
        const WCHAR *ptr;

        for(ptr = str+min(pos, length-search_len); ptr >= str; ptr--) {
            if(!memcmp(ptr, search_str, search_len*sizeof(WCHAR))) {
                ret = ptr-str;
                break;
            }
        }
    }

    jsstr_release(search_jsstr);
    jsstr_release(jsstr);
    if(FAILED(hres))
        return hres;

    if(r)
        *r = jsval_number(ret);
    return S_OK;
}

static HRESULT String_link(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    return do_attribute_tag_format(ctx, vthis, argc, argv, r, L"A", L"HREF");
}

/* ECMA-262 3rd Edition    15.5.4.10 */
static HRESULT String_match(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsdisp_t *regexp = NULL;
    jsstr_t *str;
    HRESULT hres;

    TRACE("\n");

    if(!argc) {
        if(r)
            *r = jsval_null();
        return S_OK;
    }

    if(is_object_instance(argv[0])) {
        regexp = iface_to_jsdisp(get_object(argv[0]));
        if(regexp && !is_class(regexp, JSCLASS_REGEXP)) {
            jsdisp_release(regexp);
            regexp = NULL;
        }
    }

    if(!regexp) {
        jsstr_t *match_str;

        hres = to_string(ctx, argv[0], &match_str);
        if(FAILED(hres))
            return hres;

        hres = create_regexp(ctx, match_str, 0, &regexp);
        jsstr_release(match_str);
        if(FAILED(hres))
            return hres;
    }

    hres = get_string_val(ctx, vthis, &str);
    if(SUCCEEDED(hres))
        hres = regexp_string_match(ctx, regexp, str, r);

    jsdisp_release(regexp);
    jsstr_release(str);
    return hres;
}

typedef struct {
    WCHAR *buf;
    DWORD size;
    DWORD len;
} strbuf_t;

static BOOL strbuf_ensure_size(strbuf_t *buf, unsigned len)
{
    WCHAR *new_buf;
    DWORD new_size;

    if(len <= buf->size)
        return TRUE;

    new_size = buf->size ? buf->size<<1 : 16;
    if(new_size < len)
        new_size = len;
    new_buf = realloc(buf->buf, new_size * sizeof(WCHAR));
    if(!new_buf)
        return FALSE;

    buf->buf = new_buf;
    buf->size = new_size;
    return TRUE;
}

static HRESULT strbuf_append(strbuf_t *buf, const WCHAR *str, DWORD len)
{
    if(!len)
        return S_OK;

    if(!strbuf_ensure_size(buf, buf->len+len))
        return E_OUTOFMEMORY;

    memcpy(buf->buf+buf->len, str, len*sizeof(WCHAR));
    buf->len += len;
    return S_OK;
}

static HRESULT strbuf_append_jsstr(strbuf_t *buf, jsstr_t *str)
{
    if(!strbuf_ensure_size(buf, buf->len+jsstr_length(str)))
        return E_OUTOFMEMORY;

    jsstr_flush(str, buf->buf+buf->len);
    buf->len += jsstr_length(str);
    return S_OK;
}

static HRESULT rep_call(script_ctx_t *ctx, jsdisp_t *func,
        jsstr_t *jsstr, const WCHAR *str, match_state_t *match, jsstr_t **ret)
{
    jsval_t *argv;
    unsigned argc;
    jsval_t val;
    jsstr_t *tmp_str;
    DWORD i;
    HRESULT hres = S_OK;

    argc = match->paren_count+3;
    argv = calloc(argc, sizeof(*argv));
    if(!argv)
        return E_OUTOFMEMORY;

    tmp_str = jsstr_alloc_len(match->cp-match->match_len, match->match_len);
    if(!tmp_str)
        hres = E_OUTOFMEMORY;
    argv[0] = jsval_string(tmp_str);

    if(SUCCEEDED(hres)) {
        for(i=0; i < match->paren_count; i++) {
            if(match->parens[i].index != -1)
                tmp_str = jsstr_substr(jsstr, match->parens[i].index, match->parens[i].length);
            else
                tmp_str = jsstr_empty();
            if(!tmp_str) {
               hres = E_OUTOFMEMORY;
               break;
            }
            argv[i+1] = jsval_string(tmp_str);
        }
    }

    if(SUCCEEDED(hres)) {
        argv[match->paren_count+1] = jsval_number(match->cp-str - match->match_len);
        argv[match->paren_count+2] = jsval_string(jsstr);
    }

    if(SUCCEEDED(hres))
        hres = jsdisp_call_value(func, jsval_undefined(), DISPATCH_METHOD, argc, argv, &val);

    for(i=0; i <= match->paren_count; i++)
        jsstr_release(get_string(argv[i]));
    free(argv);

    if(FAILED(hres))
        return hres;

    hres = to_string(ctx, val, ret);
    jsval_release(val);
    return hres;
}

/* ECMA-262 3rd Edition    15.5.4.11 */
static HRESULT String_replace(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    const WCHAR *str, *match_str = NULL, *rep_str = NULL;
    jsstr_t *rep_jsstr, *match_jsstr, *jsstr;
    jsdisp_t *rep_func = NULL, *regexp = NULL;
    match_state_t *match = NULL, last_match = {0};
    strbuf_t ret = {NULL,0,0};
    DWORD re_flags = REM_NO_CTX_UPDATE|REM_ALLOC_RESULT;
    DWORD rep_len=0;
    HRESULT hres = S_OK;

    TRACE("\n");

    hres = get_string_flat_val(ctx, vthis, &jsstr, &str);
    if(FAILED(hres))
        return hres;

    if(!argc) {
        if(r)
            *r = jsval_string(jsstr);
        else
            jsstr_release(jsstr);
        return S_OK;
    }

    if(is_object_instance(argv[0])) {
        regexp = iface_to_jsdisp(get_object(argv[0]));
        if(regexp && !is_class(regexp, JSCLASS_REGEXP)) {
            jsdisp_release(regexp);
            regexp = NULL;
        }
    }

    if(!regexp) {
        hres = to_flat_string(ctx, argv[0], &match_jsstr, &match_str);
        if(FAILED(hres)) {
            jsstr_release(jsstr);
            return hres;
        }
    }

    if(argc >= 2) {
        if(is_object_instance(argv[1])) {
            rep_func = iface_to_jsdisp(get_object(argv[1]));
            if(rep_func && !is_class(rep_func, JSCLASS_FUNCTION)) {
                jsdisp_release(rep_func);
                rep_func = NULL;
            }
        }

        if(!rep_func) {
            hres = to_flat_string(ctx, argv[1], &rep_jsstr, &rep_str);
            if(SUCCEEDED(hres))
                rep_len = jsstr_length(rep_jsstr);
        }
    }

    if(SUCCEEDED(hres)) {
        const WCHAR *ecp = str;

        while(1) {
            if(regexp) {
                hres = regexp_match_next(ctx, regexp, re_flags, jsstr, &match);
                re_flags = (re_flags | REM_CHECK_GLOBAL) & (~REM_ALLOC_RESULT);

                if(hres == S_FALSE) {
                    hres = S_OK;
                    break;
                }
                if(FAILED(hres))
                    break;

                last_match.cp = match->cp;
                last_match.match_len = match->match_len;
            }else {
                if(re_flags & REM_ALLOC_RESULT) {
                    re_flags &= ~REM_ALLOC_RESULT;
                    match = &last_match;
                    match->cp = str;
                }

                match->cp = wcsstr(match->cp, match_str);
                if(!match->cp)
                    break;
                match->match_len = jsstr_length(match_jsstr);
                match->cp += match->match_len;
            }

            hres = strbuf_append(&ret, ecp, match->cp-ecp-match->match_len);
            ecp = match->cp;
            if(FAILED(hres))
                break;

            if(rep_func) {
                jsstr_t *cstr;

                hres = rep_call(ctx, rep_func, jsstr, str, match, &cstr);
                if(FAILED(hres))
                    break;

                hres = strbuf_append_jsstr(&ret, cstr);
                jsstr_release(cstr);
                if(FAILED(hres))
                    break;
            }else if(rep_str && regexp) {
                const WCHAR *ptr = rep_str, *ptr2;

                while((ptr2 = wcschr(ptr, '$'))) {
                    hres = strbuf_append(&ret, ptr, ptr2-ptr);
                    if(FAILED(hres))
                        break;

                    switch(ptr2[1]) {
                    case '$':
                        hres = strbuf_append(&ret, ptr2, 1);
                        ptr = ptr2+2;
                        break;
                    case '&':
                        hres = strbuf_append(&ret, match->cp-match->match_len, match->match_len);
                        ptr = ptr2+2;
                        break;
                    case '`':
                        hres = strbuf_append(&ret, str, match->cp-str-match->match_len);
                        ptr = ptr2+2;
                        break;
                    case '\'':
                        hres = strbuf_append(&ret, ecp, (str+jsstr_length(jsstr))-ecp);
                        ptr = ptr2+2;
                        break;
                    default: {
                        DWORD idx;

                        if(!is_digit(ptr2[1])) {
                            hres = strbuf_append(&ret, ptr2, 1);
                            ptr = ptr2+1;
                            break;
                        }

                        idx = ptr2[1] - '0';
                        if(is_digit(ptr2[2]) && idx*10 + (ptr2[2]-'0') <= match->paren_count) {
                            idx = idx*10 + (ptr[2]-'0');
                            ptr = ptr2+3;
                        }else if(idx && idx <= match->paren_count) {
                            ptr = ptr2+2;
                        }else {
                            hres = strbuf_append(&ret, ptr2, 1);
                            ptr = ptr2+1;
                            break;
                        }

                        if(match->parens[idx-1].index != -1)
                            hres = strbuf_append(&ret, str+match->parens[idx-1].index,
                                    match->parens[idx-1].length);
                    }
                    }

                    if(FAILED(hres))
                        break;
                }

                if(SUCCEEDED(hres))
                    hres = strbuf_append(&ret, ptr, (rep_str+rep_len)-ptr);
                if(FAILED(hres))
                    break;
            }else if(rep_str) {
                hres = strbuf_append(&ret, rep_str, rep_len);
                if(FAILED(hres))
                    break;
            }else {
                hres = strbuf_append(&ret, L"undefined", ARRAY_SIZE(L"undefined")-1);
                if(FAILED(hres))
                    break;
            }

            if(!regexp)
                break;
            else if(!match->match_len)
                match->cp++;
        }

        if(SUCCEEDED(hres))
            hres = strbuf_append(&ret, ecp, str+jsstr_length(jsstr)-ecp);
    }

    if(rep_func)
        jsdisp_release(rep_func);
    if(rep_str)
        jsstr_release(rep_jsstr);
    if(match_str)
        jsstr_release(match_jsstr);
    if(regexp)
        free(match);

    if(SUCCEEDED(hres) && last_match.cp && regexp) {
        jsstr_release(ctx->last_match);
        ctx->last_match = jsstr_addref(jsstr);
        ctx->last_match_index = last_match.cp-str-last_match.match_len;
        ctx->last_match_length = last_match.match_len;
    }

    if(regexp)
        jsdisp_release(regexp);
    jsstr_release(jsstr);

    if(SUCCEEDED(hres) && r) {
        jsstr_t *ret_str;

        ret_str = jsstr_alloc_len(ret.buf, ret.len);
        if(!ret_str) {
            free(ret.buf);
            return E_OUTOFMEMORY;
        }

        TRACE("= %s\n", debugstr_jsstr(ret_str));
        *r = jsval_string(ret_str);
    }

    free(ret.buf);
    return hres;
}

static HRESULT String_search(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsdisp_t *regexp = NULL;
    const WCHAR *str;
    jsstr_t *jsstr;
    match_state_t match, *match_ptr = &match;
    HRESULT hres;

    TRACE("\n");

    hres = get_string_flat_val(ctx, vthis, &jsstr, &str);
    if(FAILED(hres))
        return hres;

    if(!argc) {
        if(r)
            *r = jsval_null();
        jsstr_release(jsstr);
        return S_OK;
    }

    if(is_object_instance(argv[0])) {
        regexp = iface_to_jsdisp(get_object(argv[0]));
        if(regexp && !is_class(regexp, JSCLASS_REGEXP)) {
            jsdisp_release(regexp);
            regexp = NULL;
        }
    }

    if(!regexp) {
        hres = create_regexp_var(ctx, argv[0], NULL, &regexp);
        if(FAILED(hres)) {
            jsstr_release(jsstr);
            return hres;
        }
    }

    match.cp = str;
    hres = regexp_match_next(ctx, regexp, REM_RESET_INDEX|REM_NO_PARENS, jsstr, &match_ptr);
    jsstr_release(jsstr);
    jsdisp_release(regexp);
    if(FAILED(hres))
        return hres;

    if(r)
        *r = jsval_number(hres == S_OK ? match.cp-match.match_len-str : -1);
    return S_OK;
}

/* ECMA-262 3rd Edition    15.5.4.13 */
static HRESULT String_slice(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    int start=0, end, length;
    jsstr_t *str;
    double d;
    HRESULT hres;

    TRACE("\n");

    hres = get_string_val(ctx, vthis, &str);
    if(FAILED(hres))
        return hres;

    length = jsstr_length(str);
    if(argc) {
        hres = to_integer(ctx, argv[0], &d);
        if(FAILED(hres)) {
            jsstr_release(str);
            return hres;
        }

        if(is_int32(d)) {
            start = d;
            if(start < 0) {
                start = length + start;
                if(start < 0)
                    start = 0;
            }else if(start > length) {
                start = length;
            }
        }else if(d > 0) {
            start = length;
        }
    }

    if(argc >= 2) {
        hres = to_integer(ctx, argv[1], &d);
        if(FAILED(hres)) {
            jsstr_release(str);
            return hres;
        }

        if(is_int32(d)) {
            end = d;
            if(end < 0) {
                end = length + end;
                if(end < 0)
                    end = 0;
            }else if(end > length) {
                end = length;
            }
        }else {
            end = d < 0.0 ? 0 : length;
        }
    }else {
        end = length;
    }

    if(end < start)
        end = start;

    if(r) {
        jsstr_t *retstr = jsstr_substr(str, start, end-start);
        if(!retstr) {
            jsstr_release(str);
            return E_OUTOFMEMORY;
        }

        *r = jsval_string(retstr);
    }

    jsstr_release(str);
    return S_OK;
}

static HRESULT String_small(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    return do_attributeless_tag_format(ctx, vthis, r, L"SMALL");
}

static HRESULT String_split(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    match_state_t match_result, *match_ptr = &match_result;
    size_t length, i = 0, match_len = 0;
    const WCHAR *ptr, *ptr2, *str, *match_str = NULL;
    unsigned limit = ~0u;
    jsdisp_t *array, *regexp = NULL;
    jsstr_t *jsstr, *match_jsstr, *tmp_str;
    HRESULT hres;

    hres = get_string_flat_val(ctx, vthis, &jsstr, &str);
    if(FAILED(hres))
        return hres;
    length = jsstr_length(jsstr);

    TRACE("%s\n", debugstr_wn(str, length));

    if(!argc || (is_undefined(argv[0]) && ctx->version >= SCRIPTLANGUAGEVERSION_ES5)) {
        if(!r)
            return S_OK;

        hres = create_array(ctx, 0, &array);
        if(FAILED(hres))
            return hres;

        /* NOTE: according to spec, we should respect limit argument here (if provided).
         * We have a test showing that it's broken in native IE. */
        hres = jsdisp_propput_idx(array, 0, jsval_string(jsstr));
        if(FAILED(hres)) {
            jsdisp_release(array);
            return hres;
        }

        *r = jsval_obj(array);
        return S_OK;
    }

    if(argc > 1 && !is_undefined(argv[1])) {
        hres = to_uint32(ctx, argv[1], &limit);
        if(FAILED(hres)) {
            jsstr_release(jsstr);
            return hres;
        }
    }

    if(is_object_instance(argv[0])) {
        regexp = iface_to_jsdisp(get_object(argv[0]));
        if(regexp) {
            if(!is_class(regexp, JSCLASS_REGEXP)) {
                jsdisp_release(regexp);
                regexp = NULL;
            }
        }
    }

    if(!regexp) {
        hres = to_flat_string(ctx, argv[0], &match_jsstr, &match_str);
        if(FAILED(hres)) {
            jsstr_release(jsstr);
            return hres;
        }

        match_len = jsstr_length(match_jsstr);
        if(!match_len) {
            jsstr_release(match_jsstr);
            match_str = NULL;
        }
    }

    hres = create_array(ctx, 0, &array);

    if(SUCCEEDED(hres)) {
        ptr = str;
        match_result.cp = str;
        while(i < limit) {
            if(regexp) {
                hres = regexp_match_next(ctx, regexp, REM_NO_PARENS, jsstr, &match_ptr);
                if(hres != S_OK)
                    break;
                TRACE("got match %d %ld\n", (int)(match_result.cp - match_result.match_len - str), match_result.match_len);
                if(!match_result.match_len) {
                    /* If an empty string is matched, prevent including any match in the result */
                    if(!length) {
                        limit = 0;
                        break;
                    }
                    if(match_result.cp == ptr) {
                        match_result.cp++;
                        hres = regexp_match_next(ctx, regexp, REM_NO_PARENS, jsstr, &match_ptr);
                        if(hres != S_OK)
                            break;
                        TRACE("retried, got match %d %ld\n", (int)(match_result.cp - match_result.match_len - str),
                              match_result.match_len);
                    }
                    if(!match_result.match_len && match_result.cp == str + length)
                        break;
                }
                ptr2 = match_result.cp - match_result.match_len;
            }else if(match_str) {
                ptr2 = wcsstr(ptr, match_str);
                if(!ptr2)
                    break;
            }else {
                if(!*ptr)
                    break;
                ptr2 = ptr+1;
            }

            if(!regexp || ptr2 > ptr || ctx->version >= SCRIPTLANGUAGEVERSION_ES5) {
                tmp_str = jsstr_alloc_len(ptr, ptr2-ptr);
                if(!tmp_str) {
                    hres = E_OUTOFMEMORY;
                    break;
                }

                hres = jsdisp_propput_idx(array, i++, jsval_string(tmp_str));
                jsstr_release(tmp_str);
                if(FAILED(hres))
                    break;
            }

            if(regexp)
                ptr = match_result.cp;
            else if(match_str)
                ptr = ptr2 + match_len;
            else
                ptr++;
        }
    }

    if(SUCCEEDED(hres) && (match_str || regexp) && i<limit) {
        DWORD len = (str+length) - ptr;

        if(len || match_str || !length || ctx->version >= SCRIPTLANGUAGEVERSION_ES5) {
            tmp_str = jsstr_alloc_len(ptr, len);

            if(tmp_str) {
                hres = jsdisp_propput_idx(array, i, jsval_string(tmp_str));
                jsstr_release(tmp_str);
            }else {
                hres = E_OUTOFMEMORY;
            }
        }
    }

    if(regexp)
        jsdisp_release(regexp);
    if(match_str)
        jsstr_release(match_jsstr);
    jsstr_release(jsstr);

    if(SUCCEEDED(hres) && r)
        *r = jsval_obj(array);
    else
        jsdisp_release(array);

    return hres;
}

static HRESULT String_strike(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    return do_attributeless_tag_format(ctx, vthis, r, L"STRIKE");
}

static HRESULT String_sub(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    return do_attributeless_tag_format(ctx, vthis, r, L"SUB");
}

/* ECMA-262 3rd Edition    15.5.4.15 */
static HRESULT String_substring(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    INT start=0, end, length;
    jsstr_t *str;
    double d;
    HRESULT hres;

    TRACE("\n");

    hres = get_string_val(ctx, vthis, &str);
    if(FAILED(hres))
        return hres;

    length = jsstr_length(str);
    if(argc >= 1) {
        hres = to_integer(ctx, argv[0], &d);
        if(FAILED(hres)) {
            jsstr_release(str);
            return hres;
        }

        if(d >= 0)
            start = is_int32(d) ? min(length, d) : length;
    }

    if(argc >= 2) {
        hres = to_integer(ctx, argv[1], &d);
        if(FAILED(hres)) {
            jsstr_release(str);
            return hres;
        }

        if(d >= 0)
            end = is_int32(d) ? min(length, d) : length;
        else
            end = 0;
    }else {
        end = length;
    }

    if(start > end) {
        INT tmp = start;
        start = end;
        end = tmp;
    }

    if(r) {
        jsstr_t *ret = jsstr_substr(str, start, end-start);
        if(ret)
            *r = jsval_string(ret);
        else
            hres = E_OUTOFMEMORY;
    }
    jsstr_release(str);
    return hres;
}

/* ECMA-262 3rd Edition    B.2.3 */
static HRESULT String_substr(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    int start=0, len, length;
    jsstr_t *str;
    double d;
    HRESULT hres;

    TRACE("\n");

    hres = get_string_val(ctx, vthis, &str);
    if(FAILED(hres))
        return hres;

    length = jsstr_length(str);
    if(argc >= 1) {
        hres = to_integer(ctx, argv[0], &d);
        if(FAILED(hres)) {
            jsstr_release(str);
            return hres;
        }

        if(d >= 0)
            start = is_int32(d) ? min(length, d) : length;
    }

    if(argc >= 2) {
        hres = to_integer(ctx, argv[1], &d);
        if(FAILED(hres)) {
            jsstr_release(str);
            return hres;
        }

        if(d >= 0.0)
            len = is_int32(d) ? min(length-start, d) : length-start;
        else
            len = 0;
    }else {
        len = length-start;
    }

    hres = S_OK;
    if(r) {
        jsstr_t *ret = jsstr_substr(str, start, len);
        if(ret)
            *r = jsval_string(ret);
        else
            hres = E_OUTOFMEMORY;
    }

    jsstr_release(str);
    return hres;
}

static HRESULT String_sup(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    return do_attributeless_tag_format(ctx, vthis, r, L"SUP");
}

static HRESULT to_upper_case(script_ctx_t *ctx, jsval_t vthis, jsval_t *r)
{
    jsstr_t *str;
    HRESULT hres;


    hres = get_string_val(ctx, vthis, &str);
    if(FAILED(hres))
        return hres;

    if(r) {
        unsigned len = jsstr_length(str);
        jsstr_t *ret;
        WCHAR *buf;

        ret = jsstr_alloc_buf(len, &buf);
        if(!ret) {
            jsstr_release(str);
            return E_OUTOFMEMORY;
        }

        jsstr_flush(str, buf);
        for (; len--; buf++) *buf = towupper(*buf);

        *r = jsval_string(ret);
    }
    jsstr_release(str);
    return S_OK;
}

static HRESULT to_lower_case(script_ctx_t *ctx, jsval_t vthis, jsval_t *r)
{
    jsstr_t *str;
    HRESULT hres;


    hres = get_string_val(ctx, vthis, &str);
    if(FAILED(hres))
        return hres;

    if(r) {
        unsigned len = jsstr_length(str);
        jsstr_t *ret;
        WCHAR *buf;

        ret = jsstr_alloc_buf(len, &buf);
        if(!ret) {
            jsstr_release(str);
            return E_OUTOFMEMORY;
        }

        jsstr_flush(str, buf);
        for (; len--; buf++) *buf = towlower(*buf);

        *r = jsval_string(ret);
    }
    jsstr_release(str);
    return S_OK;
}

static HRESULT String_toLowerCase(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    TRACE("\n");
    return to_lower_case(ctx, vthis, r);
}

static HRESULT String_toUpperCase(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    TRACE("\n");
    return to_upper_case(ctx, vthis, r);
}

static HRESULT String_toLocaleLowerCase(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    TRACE("\n");
    return to_lower_case(ctx, vthis, r);
}

static HRESULT String_toLocaleUpperCase(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    TRACE("\n");
    return to_upper_case(ctx, vthis, r);
}

static HRESULT String_trim(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc,
        jsval_t *argv, jsval_t *r)
{
    const WCHAR *str, *begin, *end;
    jsstr_t *jsstr;
    unsigned len;
    HRESULT hres;

    if(is_undefined(vthis) || is_null(vthis))
        return JS_E_OBJECT_EXPECTED;

    hres = to_flat_string(ctx, vthis, &jsstr, &str);
    if(FAILED(hres)) {
        WARN("to_flat_string failed: %08lx\n", hres);
        return hres;
    }
    len = jsstr_length(jsstr);
    TRACE("%s\n", debugstr_wn(str, len));

    for(begin = str, end = str + len; begin < end && iswspace(*begin); begin++);
    while(end > begin + 1 && iswspace(*(end-1))) end--;

    if(r) {
        jsstr_t *ret;

        if(begin == str && end == str + len)
            ret = jsstr_addref(jsstr);
        else
            ret = jsstr_alloc_len(begin, end - begin);
        if(ret)
            *r = jsval_string(ret);
        else
            hres = E_OUTOFMEMORY;
    }
    jsstr_release(jsstr);
    return hres;
}

static HRESULT String_localeCompare(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static void String_destructor(jsdisp_t *dispex)
{
    StringInstance *This = string_from_jsdisp(dispex);

    jsstr_release(This->str);
}

static HRESULT String_lookup_prop(jsdisp_t *jsdisp, const WCHAR *name, unsigned flags, struct property_info *desc)
{
    StringInstance *string = string_from_jsdisp(jsdisp);

    /*
     * NOTE: For invoke version < 2, indexed array is not implemented at all.
     * Newer jscript.dll versions implement it on string type, not class,
     * which is not how it should work according to spec. IE9+ implements it
     * properly.
     */
    if(string->dispex.ctx->version < 2)
        return DISP_E_UNKNOWNNAME;

    return jsdisp_index_lookup(&string->dispex, name, jsstr_length(string->str), desc);
}

static HRESULT String_next_prop(jsdisp_t *jsdisp, unsigned id, struct property_info *desc)
{
    StringInstance *string = string_from_jsdisp(jsdisp);

    if(string->dispex.ctx->version < 2)
        return S_FALSE;

    return jsdisp_next_index(&string->dispex, jsstr_length(string->str), id, desc);
}

static HRESULT String_prop_get(jsdisp_t *jsdisp, unsigned idx, jsval_t *r)
{
    StringInstance *string = string_from_jsdisp(jsdisp);
    jsstr_t *ret;

    ret = jsstr_substr(string->str, idx, 1);
    if(!ret)
        return E_OUTOFMEMORY;

    TRACE("%p[%u] = %s\n", string, idx, debugstr_jsstr(ret));

    *r = jsval_string(ret);
    return S_OK;
}

static const builtin_prop_t String_props[] = {
    {L"anchor",                String_anchor,                PROPF_METHOD|1},
    {L"big",                   String_big,                   PROPF_METHOD},
    {L"blink",                 String_blink,                 PROPF_METHOD},
    {L"bold",                  String_bold,                  PROPF_METHOD},
    {L"charAt",                String_charAt,                PROPF_METHOD|1},
    {L"charCodeAt",            String_charCodeAt,            PROPF_METHOD|1},
    {L"concat",                String_concat,                PROPF_METHOD|1},
    {L"fixed",                 String_fixed,                 PROPF_METHOD},
    {L"fontcolor",             String_fontcolor,             PROPF_METHOD|1},
    {L"fontsize",              String_fontsize,              PROPF_METHOD|1},
    {L"indexOf",               String_indexOf,               PROPF_METHOD|2},
    {L"italics",               String_italics,               PROPF_METHOD},
    {L"lastIndexOf",           String_lastIndexOf,           PROPF_METHOD|2},
    {L"length",                NULL,0,                       String_get_length},
    {L"link",                  String_link,                  PROPF_METHOD|1},
    {L"localeCompare",         String_localeCompare,         PROPF_METHOD|1},
    {L"match",                 String_match,                 PROPF_METHOD|1},
    {L"replace",               String_replace,               PROPF_METHOD|1},
    {L"search",                String_search,                PROPF_METHOD},
    {L"slice",                 String_slice,                 PROPF_METHOD},
    {L"small",                 String_small,                 PROPF_METHOD},
    {L"split",                 String_split,                 PROPF_METHOD|2},
    {L"strike",                String_strike,                PROPF_METHOD},
    {L"sub",                   String_sub,                   PROPF_METHOD},
    {L"substr",                String_substr,                PROPF_METHOD|2},
    {L"substring",             String_substring,             PROPF_METHOD|2},
    {L"sup",                   String_sup,                   PROPF_METHOD},
    {L"toLocaleLowerCase",     String_toLocaleLowerCase,     PROPF_METHOD},
    {L"toLocaleUpperCase",     String_toLocaleUpperCase,     PROPF_METHOD},
    {L"toLowerCase",           String_toLowerCase,           PROPF_METHOD},
    {L"toString",              String_toString,              PROPF_METHOD},
    {L"toUpperCase",           String_toUpperCase,           PROPF_METHOD},
    {L"trim",                  String_trim,                  PROPF_ES5|PROPF_METHOD},
    {L"valueOf",               String_valueOf,               PROPF_METHOD}
};

static const builtin_info_t String_info = {
    .class      = JSCLASS_STRING,
    .props_cnt  = ARRAY_SIZE(String_props),
    .props      = String_props,
    .destructor = String_destructor,
};

static const builtin_prop_t StringInst_props[] = {
    {L"length",                NULL,0,                       String_get_length}
};

static const builtin_info_t StringInst_info = {
    .class       = JSCLASS_STRING,
    .props_cnt   = ARRAY_SIZE(StringInst_props),
    .props       = StringInst_props,
    .destructor  = String_destructor,
    .lookup_prop = String_lookup_prop,
    .next_prop   = String_next_prop,
    .prop_get    = String_prop_get,
};

/* ECMA-262 3rd Edition    15.5.3.2 */
static HRESULT StringConstr_fromCharCode(script_ctx_t *ctx, jsval_t vthis, WORD flags,
        unsigned argc, jsval_t *argv, jsval_t *r)
{
    WCHAR *ret_str;
    UINT32 i, code;
    jsstr_t *ret;
    HRESULT hres;

    TRACE("\n");

    ret = jsstr_alloc_buf(argc, &ret_str);
    if(!ret)
        return E_OUTOFMEMORY;

    for(i=0; i<argc; i++) {
        hres = to_uint32(ctx, argv[i], &code);
        if(FAILED(hres)) {
            jsstr_release(ret);
            return hres;
        }

        ret_str[i] = code;
    }

    if(r)
        *r = jsval_string(ret);
    else
        jsstr_release(ret);
    return S_OK;
}

static HRESULT StringConstr_value(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    HRESULT hres = S_OK;

    TRACE("\n");

    switch(flags) {
    case INVOKE_FUNC: {
        jsstr_t *str;

        if(argc) {
            hres = to_string(ctx, argv[0], &str);
            if(FAILED(hres))
                return hres;
        }else {
            str = jsstr_empty();
        }

        if(r) *r = jsval_string(str);
        else  jsstr_release(str);
        break;
    }
    case DISPATCH_CONSTRUCT: {
        jsstr_t *str;
        jsdisp_t *ret;

        if(argc) {
            hres = to_string(ctx, argv[0], &str);
            if(FAILED(hres))
                return hres;
        }else {
            str = jsstr_empty();
        }

        if(r) {
            hres = create_string(ctx, str, &ret);
            if(SUCCEEDED(hres)) *r = jsval_obj(ret);
        }
        jsstr_release(str);
        return hres;
    }

    default:
        FIXME("unimplemented flags: %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT string_alloc(script_ctx_t *ctx, jsdisp_t *object_prototype, jsstr_t *str, StringInstance **ret)
{
    StringInstance *string;
    HRESULT hres;

    string = calloc(1, sizeof(StringInstance));
    if(!string)
        return E_OUTOFMEMORY;

    if(object_prototype)
        hres = init_dispex(&string->dispex, ctx, &String_info, object_prototype);
    else
        hres = init_dispex_from_constr(&string->dispex, ctx, &StringInst_info, ctx->string_constr);
    if(FAILED(hres)) {
        free(string);
        return hres;
    }

    string->str = jsstr_addref(str);
    *ret = string;
    return S_OK;
}

static const builtin_prop_t StringConstr_props[] = {
    {L"fromCharCode",    StringConstr_fromCharCode,    PROPF_METHOD},
};

static const builtin_info_t StringConstr_info = {
    .class     = JSCLASS_FUNCTION,
    .call      = Function_value,
    .props_cnt = ARRAY_SIZE(StringConstr_props),
    .props     = StringConstr_props,
};

HRESULT create_string_constr(script_ctx_t *ctx, jsdisp_t *object_prototype, jsdisp_t **ret)
{
    StringInstance *string;
    HRESULT hres;

    hres = string_alloc(ctx, object_prototype, jsstr_empty(), &string);
    if(FAILED(hres))
        return hres;

    hres = create_builtin_constructor(ctx, StringConstr_value, L"String", &StringConstr_info,
            PROPF_CONSTR|1, &string->dispex, ret);

    jsdisp_release(&string->dispex);
    return hres;
}

HRESULT create_string(script_ctx_t *ctx, jsstr_t *str, jsdisp_t **ret)
{
    StringInstance *string;
    HRESULT hres;

    hres = string_alloc(ctx, NULL, str, &string);
    if(FAILED(hres))
        return hres;

    *ret = &string->dispex;
    return S_OK;

}
