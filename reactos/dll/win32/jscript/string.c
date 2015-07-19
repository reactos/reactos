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

#define UINT32_MAX 0xffffffff

typedef struct {
    jsdisp_t dispex;
    jsstr_t *str;
} StringInstance;

static const WCHAR lengthW[] = {'l','e','n','g','t','h',0};
static const WCHAR toStringW[] = {'t','o','S','t','r','i','n','g',0};
static const WCHAR valueOfW[] = {'v','a','l','u','e','O','f',0};
static const WCHAR anchorW[] = {'a','n','c','h','o','r',0};
static const WCHAR bigW[] = {'b','i','g',0};
static const WCHAR blinkW[] = {'b','l','i','n','k',0};
static const WCHAR boldW[] = {'b','o','l','d',0};
static const WCHAR charAtW[] = {'c','h','a','r','A','t',0};
static const WCHAR charCodeAtW[] = {'c','h','a','r','C','o','d','e','A','t',0};
static const WCHAR concatW[] = {'c','o','n','c','a','t',0};
static const WCHAR fixedW[] = {'f','i','x','e','d',0};
static const WCHAR fontcolorW[] = {'f','o','n','t','c','o','l','o','r',0};
static const WCHAR fontsizeW[] = {'f','o','n','t','s','i','z','e',0};
static const WCHAR indexOfW[] = {'i','n','d','e','x','O','f',0};
static const WCHAR italicsW[] = {'i','t','a','l','i','c','s',0};
static const WCHAR lastIndexOfW[] = {'l','a','s','t','I','n','d','e','x','O','f',0};
static const WCHAR linkW[] = {'l','i','n','k',0};
static const WCHAR matchW[] = {'m','a','t','c','h',0};
static const WCHAR replaceW[] = {'r','e','p','l','a','c','e',0};
static const WCHAR searchW[] = {'s','e','a','r','c','h',0};
static const WCHAR sliceW[] = {'s','l','i','c','e',0};
static const WCHAR smallW[] = {'s','m','a','l','l',0};
static const WCHAR splitW[] = {'s','p','l','i','t',0};
static const WCHAR strikeW[] = {'s','t','r','i','k','e',0};
static const WCHAR subW[] = {'s','u','b',0};
static const WCHAR substringW[] = {'s','u','b','s','t','r','i','n','g',0};
static const WCHAR substrW[] = {'s','u','b','s','t','r',0};
static const WCHAR supW[] = {'s','u','p',0};
static const WCHAR toLowerCaseW[] = {'t','o','L','o','w','e','r','C','a','s','e',0};
static const WCHAR toUpperCaseW[] = {'t','o','U','p','p','e','r','C','a','s','e',0};
static const WCHAR toLocaleLowerCaseW[] = {'t','o','L','o','c','a','l','e','L','o','w','e','r','C','a','s','e',0};
static const WCHAR toLocaleUpperCaseW[] = {'t','o','L','o','c','a','l','e','U','p','p','e','r','C','a','s','e',0};
static const WCHAR localeCompareW[] = {'l','o','c','a','l','e','C','o','m','p','a','r','e',0};
static const WCHAR fromCharCodeW[] = {'f','r','o','m','C','h','a','r','C','o','d','e',0};

static inline StringInstance *string_from_jsdisp(jsdisp_t *jsdisp)
{
    return CONTAINING_RECORD(jsdisp, StringInstance, dispex);
}

static inline StringInstance *string_from_vdisp(vdisp_t *vdisp)
{
    return string_from_jsdisp(vdisp->u.jsdisp);
}

static inline StringInstance *string_this(vdisp_t *jsthis)
{
    return is_vclass(jsthis, JSCLASS_STRING) ? string_from_vdisp(jsthis) : NULL;
}

static HRESULT get_string_val(script_ctx_t *ctx, vdisp_t *jsthis, jsstr_t **val)
{
    StringInstance *string;

    if((string = string_this(jsthis))) {
        *val = jsstr_addref(string->str);
        return S_OK;
    }

    return to_string(ctx, jsval_disp(jsthis->u.disp), val);
}

static HRESULT get_string_flat_val(script_ctx_t *ctx, vdisp_t *jsthis, jsstr_t **jsval, const WCHAR **val)
{
    HRESULT hres;

    hres = get_string_val(ctx, jsthis, jsval);
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
    StringInstance *string = (StringInstance*)jsthis;

    TRACE("%p\n", jsthis);

    *r = jsval_number(jsstr_length(string->str));
    return S_OK;
}

static HRESULT String_set_length(script_ctx_t *ctx, jsdisp_t *jsthis, jsval_t value)
{
    FIXME("%p\n", jsthis);
    return E_NOTIMPL;
}

static HRESULT stringobj_to_string(vdisp_t *jsthis, jsval_t *r)
{
    StringInstance *string;

    if(!(string = string_this(jsthis))) {
        WARN("this is not a string object\n");
        return E_FAIL;
    }

    if(r)
        *r = jsval_string(jsstr_addref(string->str));
    return S_OK;
}

/* ECMA-262 3rd Edition    15.5.4.2 */
static HRESULT String_toString(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    TRACE("\n");

    return stringobj_to_string(jsthis, r);
}

/* ECMA-262 3rd Edition    15.5.4.2 */
static HRESULT String_valueOf(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    TRACE("\n");

    return stringobj_to_string(jsthis, r);
}

static HRESULT do_attributeless_tag_format(script_ctx_t *ctx, vdisp_t *jsthis, jsval_t *r, const WCHAR *tagname)
{
    unsigned tagname_len;
    jsstr_t *str, *ret;
    WCHAR *ptr;
    HRESULT hres;

    hres = get_string_val(ctx, jsthis, &str);
    if(FAILED(hres))
        return hres;

    if(!r) {
        jsstr_release(str);
        return S_OK;
    }

    tagname_len = strlenW(tagname);

    ptr = jsstr_alloc_buf(jsstr_length(str) + 2*tagname_len + 5, &ret);
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

static HRESULT do_attribute_tag_format(script_ctx_t *ctx, vdisp_t *jsthis, unsigned argc, jsval_t *argv, jsval_t *r,
        const WCHAR *tagname, const WCHAR *attrname)
{
    jsstr_t *str, *attr_value = NULL;
    HRESULT hres;

    hres = get_string_val(ctx, jsthis, &str);
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
        unsigned attrname_len = strlenW(attrname);
        unsigned tagname_len = strlenW(tagname);
        jsstr_t *ret;
        WCHAR *ptr;

        ptr = jsstr_alloc_buf(2*tagname_len + attrname_len + jsstr_length(attr_value) + jsstr_length(str) + 9, &ret);
        if(ptr) {
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

static HRESULT String_anchor(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    static const WCHAR fontW[] = {'A',0};
    static const WCHAR colorW[] = {'N','A','M','E',0};

    return do_attribute_tag_format(ctx, jsthis, argc, argv, r, fontW, colorW);
}

static HRESULT String_big(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    static const WCHAR bigtagW[] = {'B','I','G',0};
    return do_attributeless_tag_format(ctx, jsthis, r, bigtagW);
}

static HRESULT String_blink(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    static const WCHAR blinktagW[] = {'B','L','I','N','K',0};
    return do_attributeless_tag_format(ctx, jsthis, r, blinktagW);
}

static HRESULT String_bold(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    static const WCHAR boldtagW[] = {'B',0};
    return do_attributeless_tag_format(ctx, jsthis, r, boldtagW);
}

/* ECMA-262 3rd Edition    15.5.4.5 */
static HRESULT String_charAt(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsstr_t *str, *ret;
    INT pos = 0;
    HRESULT hres;

    TRACE("\n");

    hres = get_string_val(ctx, jsthis, &str);
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
static HRESULT String_charCodeAt(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsstr_t *str;
    DWORD idx = 0;
    HRESULT hres;

    TRACE("\n");

    hres = get_string_val(ctx, jsthis, &str);
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
static HRESULT String_concat(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsstr_t *ret, *str;
    HRESULT hres;

    TRACE("\n");

    hres = get_string_val(ctx, jsthis, &str);
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

        strs = heap_alloc_zero(str_cnt * sizeof(*strs));
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
                ptr = jsstr_alloc_buf(len, &ret);
                if(ptr) {
                    for(i=0; i < str_cnt; i++)
                        ptr += jsstr_flush(strs[i], ptr);
                }else {
                    hres = E_OUTOFMEMORY;
                }
            }
        }

        while(i--)
            jsstr_release(strs[i]);
        heap_free(strs);
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

static HRESULT String_fixed(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    static const WCHAR fixedtagW[] = {'T','T',0};
    return do_attributeless_tag_format(ctx, jsthis, r, fixedtagW);
}

static HRESULT String_fontcolor(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    static const WCHAR fontW[] = {'F','O','N','T',0};
    static const WCHAR colorW[] = {'C','O','L','O','R',0};

    return do_attribute_tag_format(ctx, jsthis, argc, argv, r, fontW, colorW);
}

static HRESULT String_fontsize(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    static const WCHAR fontW[] = {'F','O','N','T',0};
    static const WCHAR colorW[] = {'S','I','Z','E',0};

    return do_attribute_tag_format(ctx, jsthis, argc, argv, r, fontW, colorW);
}

static HRESULT String_indexOf(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsstr_t *search_jsstr, *jsstr;
    const WCHAR *search_str, *str;
    int length, pos = 0;
    INT ret = -1;
    HRESULT hres;

    TRACE("\n");

    hres = get_string_flat_val(ctx, jsthis, &jsstr, &str);
    if(FAILED(hres))
        return hres;

    length = jsstr_length(jsstr);
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

    if(argc >= 2) {
        double d;

        hres = to_integer(ctx, argv[1], &d);
        if(SUCCEEDED(hres) && d > 0.0)
            pos = is_int32(d) ? min(length, d) : length;
    }

    if(SUCCEEDED(hres)) {
        const WCHAR *ptr;

        ptr = strstrW(str+pos, search_str);
        if(ptr)
            ret = ptr - str;
        else
            ret = -1;
    }

    jsstr_release(search_jsstr);
    jsstr_release(jsstr);
    if(FAILED(hres))
        return hres;

    if(r)
        *r = jsval_number(ret);
    return S_OK;
}

static HRESULT String_italics(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    static const WCHAR italicstagW[] = {'I',0};
    return do_attributeless_tag_format(ctx, jsthis, r, italicstagW);
}

/* ECMA-262 3rd Edition    15.5.4.8 */
static HRESULT String_lastIndexOf(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    unsigned pos = 0, search_len, length;
    jsstr_t *search_jsstr, *jsstr;
    const WCHAR *search_str, *str;
    INT ret = -1;
    HRESULT hres;

    TRACE("\n");

    hres = get_string_flat_val(ctx, jsthis, &jsstr, &str);
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

static HRESULT String_link(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    static const WCHAR fontW[] = {'A',0};
    static const WCHAR colorW[] = {'H','R','E','F',0};

    return do_attribute_tag_format(ctx, jsthis, argc, argv, r, fontW, colorW);
}

/* ECMA-262 3rd Edition    15.5.4.10 */
static HRESULT String_match(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
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
        regexp = iface_to_jsdisp((IUnknown*)get_object(argv[0]));
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

    hres = get_string_val(ctx, jsthis, &str);
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
    if(buf->buf)
        new_buf = heap_realloc(buf->buf, new_size*sizeof(WCHAR));
    else
        new_buf = heap_alloc(new_size*sizeof(WCHAR));
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
    argv = heap_alloc_zero(sizeof(*argv)*argc);
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
        hres = jsdisp_call_value(func, NULL, DISPATCH_METHOD, argc, argv, &val);

    for(i=0; i <= match->paren_count; i++)
        jsstr_release(get_string(argv[i]));
    heap_free(argv);

    if(FAILED(hres))
        return hres;

    hres = to_string(ctx, val, ret);
    jsval_release(val);
    return hres;
}

/* ECMA-262 3rd Edition    15.5.4.11 */
static HRESULT String_replace(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
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

    hres = get_string_flat_val(ctx, jsthis, &jsstr, &str);
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
        regexp = iface_to_jsdisp((IUnknown*)get_object(argv[0]));
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
            rep_func = iface_to_jsdisp((IUnknown*)get_object(argv[1]));
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

                match->cp = strstrW(match->cp, match_str);
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

                while((ptr2 = strchrW(ptr, '$'))) {
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

                        if(!isdigitW(ptr2[1])) {
                            hres = strbuf_append(&ret, ptr2, 1);
                            ptr = ptr2+1;
                            break;
                        }

                        idx = ptr2[1] - '0';
                        if(isdigitW(ptr2[2]) && idx*10 + (ptr2[2]-'0') <= match->paren_count) {
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
                static const WCHAR undefinedW[] = {'u','n','d','e','f','i','n','e','d'};

                hres = strbuf_append(&ret, undefinedW, sizeof(undefinedW)/sizeof(WCHAR));
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
        heap_free(match);

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
        if(!ret_str)
            return E_OUTOFMEMORY;

        TRACE("= %s\n", debugstr_jsstr(ret_str));
        *r = jsval_string(ret_str);
    }

    heap_free(ret.buf);
    return hres;
}

static HRESULT String_search(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsdisp_t *regexp = NULL;
    const WCHAR *str;
    jsstr_t *jsstr;
    match_state_t match, *match_ptr = &match;
    HRESULT hres;

    TRACE("\n");

    hres = get_string_flat_val(ctx, jsthis, &jsstr, &str);
    if(FAILED(hres))
        return hres;

    if(!argc) {
        if(r)
            *r = jsval_null();
        jsstr_release(jsstr);
        return S_OK;
    }

    if(is_object_instance(argv[0])) {
        regexp = iface_to_jsdisp((IUnknown*)get_object(argv[0]));
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
static HRESULT String_slice(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    int start=0, end, length;
    jsstr_t *str;
    double d;
    HRESULT hres;

    TRACE("\n");

    hres = get_string_val(ctx, jsthis, &str);
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

static HRESULT String_small(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    static const WCHAR smalltagW[] = {'S','M','A','L','L',0};
    return do_attributeless_tag_format(ctx, jsthis, r, smalltagW);
}

static HRESULT String_split(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    match_state_t match_result, *match_ptr = &match_result;
    DWORD length, i, match_len = 0;
    const WCHAR *ptr, *ptr2, *str, *match_str = NULL;
    unsigned limit = ~0u;
    jsdisp_t *array, *regexp = NULL;
    jsstr_t *jsstr, *match_jsstr, *tmp_str;
    HRESULT hres;

    TRACE("\n");

    if(argc != 1 && argc != 2) {
        FIXME("unsupported argc %u\n", argc);
        return E_NOTIMPL;
    }

    hres = get_string_flat_val(ctx, jsthis, &jsstr, &str);
    if(FAILED(hres))
        return hres;

    length = jsstr_length(jsstr);

    if(argc > 1 && !is_undefined(argv[1])) {
        hres = to_uint32(ctx, argv[1], &limit);
        if(FAILED(hres)) {
            jsstr_release(jsstr);
            return hres;
        }
    }

    if(is_object_instance(argv[0])) {
        regexp = iface_to_jsdisp((IUnknown*)get_object(argv[0]));
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
        for(i=0; i<limit; i++) {
            if(regexp) {
                hres = regexp_match_next(ctx, regexp, REM_NO_PARENS, jsstr, &match_ptr);
                if(hres != S_OK)
                    break;
                ptr2 = match_result.cp - match_result.match_len;
            }else if(match_str) {
                ptr2 = strstrW(ptr, match_str);
                if(!ptr2)
                    break;
            }else {
                if(!*ptr)
                    break;
                ptr2 = ptr+1;
            }

            tmp_str = jsstr_alloc_len(ptr, ptr2-ptr);
            if(!tmp_str) {
                hres = E_OUTOFMEMORY;
                break;
            }

            hres = jsdisp_propput_idx(array, i, jsval_string(tmp_str));
            jsstr_release(tmp_str);
            if(FAILED(hres))
                break;

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

        if(len || match_str) {
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

static HRESULT String_strike(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    static const WCHAR striketagW[] = {'S','T','R','I','K','E',0};
    return do_attributeless_tag_format(ctx, jsthis, r, striketagW);
}

static HRESULT String_sub(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    static const WCHAR subtagW[] = {'S','U','B',0};
    return do_attributeless_tag_format(ctx, jsthis, r, subtagW);
}

/* ECMA-262 3rd Edition    15.5.4.15 */
static HRESULT String_substring(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    INT start=0, end, length;
    jsstr_t *str;
    double d;
    HRESULT hres;

    TRACE("\n");

    hres = get_string_val(ctx, jsthis, &str);
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
static HRESULT String_substr(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    int start=0, len, length;
    jsstr_t *str;
    double d;
    HRESULT hres;

    TRACE("\n");

    hres = get_string_val(ctx, jsthis, &str);
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

static HRESULT String_sup(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    static const WCHAR suptagW[] = {'S','U','P',0};
    return do_attributeless_tag_format(ctx, jsthis, r, suptagW);
}

static HRESULT String_toLowerCase(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsstr_t *str;
    HRESULT  hres;

    TRACE("\n");

    hres = get_string_val(ctx, jsthis, &str);
    if(FAILED(hres))
        return hres;

    if(r) {
        jsstr_t *ret;
        WCHAR *buf;

        buf = jsstr_alloc_buf(jsstr_length(str), &ret);
        if(!buf) {
            jsstr_release(str);
            return E_OUTOFMEMORY;
        }

        jsstr_flush(str, buf);
        strlwrW(buf);
        *r = jsval_string(ret);
    }
    jsstr_release(str);
    return S_OK;
}

static HRESULT String_toUpperCase(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsstr_t *str;
    HRESULT hres;

    TRACE("\n");

    hres = get_string_val(ctx, jsthis, &str);
    if(FAILED(hres))
        return hres;

    if(r) {
        jsstr_t *ret;
        WCHAR *buf;

        buf = jsstr_alloc_buf(jsstr_length(str), &ret);
        if(!buf) {
            jsstr_release(str);
            return E_OUTOFMEMORY;
        }

        jsstr_flush(str, buf);
        struprW(buf);
        *r = jsval_string(ret);
    }
    jsstr_release(str);
    return S_OK;
}

static HRESULT String_toLocaleLowerCase(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT String_toLocaleUpperCase(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT String_localeCompare(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT String_get_value(script_ctx_t *ctx, jsdisp_t *jsthis, jsval_t *r)
{
    StringInstance *This = (StringInstance*)jsthis;

    TRACE("\n");

    *r = jsval_string(jsstr_addref(This->str));
    return S_OK;
}

static void String_destructor(jsdisp_t *dispex)
{
    StringInstance *This = (StringInstance*)dispex;

    jsstr_release(This->str);
    heap_free(This);
}

static unsigned String_idx_length(jsdisp_t *jsdisp)
{
    StringInstance *string = (StringInstance*)jsdisp;

    /*
     * NOTE: For invoke version < 2, indexed array is not implemented at all.
     * Newer jscript.dll versions implement it on string type, not class,
     * which is not how it should work according to spec. IE9 implements it
     * properly, but it uses its own JavaScript engine inside MSHTML. We
     * implement it here, but in the way IE9 and spec work.
     */
    return string->dispex.ctx->version < 2 ? 0 : jsstr_length(string->str);
}

static HRESULT String_idx_get(jsdisp_t *jsdisp, unsigned idx, jsval_t *r)
{
    StringInstance *string = (StringInstance*)jsdisp;
    jsstr_t *ret;

    ret = jsstr_substr(string->str, idx, 1);
    if(!ret)
        return E_OUTOFMEMORY;

    TRACE("%p[%u] = %s\n", string, idx, debugstr_jsstr(ret));

    *r = jsval_string(ret);
    return S_OK;
}

static const builtin_prop_t String_props[] = {
    {anchorW,                String_anchor,                PROPF_METHOD|1},
    {bigW,                   String_big,                   PROPF_METHOD},
    {blinkW,                 String_blink,                 PROPF_METHOD},
    {boldW,                  String_bold,                  PROPF_METHOD},
    {charAtW,                String_charAt,                PROPF_METHOD|1},
    {charCodeAtW,            String_charCodeAt,            PROPF_METHOD|1},
    {concatW,                String_concat,                PROPF_METHOD|1},
    {fixedW,                 String_fixed,                 PROPF_METHOD},
    {fontcolorW,             String_fontcolor,             PROPF_METHOD|1},
    {fontsizeW,              String_fontsize,              PROPF_METHOD|1},
    {indexOfW,               String_indexOf,               PROPF_METHOD|2},
    {italicsW,               String_italics,               PROPF_METHOD},
    {lastIndexOfW,           String_lastIndexOf,           PROPF_METHOD|2},
    {lengthW,                NULL,0,                       String_get_length, String_set_length},
    {linkW,                  String_link,                  PROPF_METHOD|1},
    {localeCompareW,         String_localeCompare,         PROPF_METHOD|1},
    {matchW,                 String_match,                 PROPF_METHOD|1},
    {replaceW,               String_replace,               PROPF_METHOD|1},
    {searchW,                String_search,                PROPF_METHOD},
    {sliceW,                 String_slice,                 PROPF_METHOD},
    {smallW,                 String_small,                 PROPF_METHOD},
    {splitW,                 String_split,                 PROPF_METHOD|2},
    {strikeW,                String_strike,                PROPF_METHOD},
    {subW,                   String_sub,                   PROPF_METHOD},
    {substrW,                String_substr,                PROPF_METHOD|2},
    {substringW,             String_substring,             PROPF_METHOD|2},
    {supW,                   String_sup,                   PROPF_METHOD},
    {toLocaleLowerCaseW,     String_toLocaleLowerCase,     PROPF_METHOD},
    {toLocaleUpperCaseW,     String_toLocaleUpperCase,     PROPF_METHOD},
    {toLowerCaseW,           String_toLowerCase,           PROPF_METHOD},
    {toStringW,              String_toString,              PROPF_METHOD},
    {toUpperCaseW,           String_toUpperCase,           PROPF_METHOD},
    {valueOfW,               String_valueOf,               PROPF_METHOD}
};

static const builtin_info_t String_info = {
    JSCLASS_STRING,
    {NULL, NULL,0, String_get_value},
    sizeof(String_props)/sizeof(*String_props),
    String_props,
    String_destructor,
    NULL
};

static const builtin_prop_t StringInst_props[] = {
    {lengthW,                NULL,0,                       String_get_length, String_set_length}
};

static const builtin_info_t StringInst_info = {
    JSCLASS_STRING,
    {NULL, NULL,0, String_get_value},
    sizeof(StringInst_props)/sizeof(*StringInst_props),
    StringInst_props,
    String_destructor,
    NULL,
    String_idx_length,
    String_idx_get
};

/* ECMA-262 3rd Edition    15.5.3.2 */
static HRESULT StringConstr_fromCharCode(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags,
        unsigned argc, jsval_t *argv, jsval_t *r)
{
    WCHAR *ret_str;
    DWORD i, code;
    jsstr_t *ret;
    HRESULT hres;

    TRACE("\n");

    ret_str = jsstr_alloc_buf(argc, &ret);
    if(!ret_str)
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

static HRESULT StringConstr_value(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    HRESULT hres;

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

        *r = jsval_string(str);
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

        hres = create_string(ctx, str, &ret);
        if (SUCCEEDED(hres)) *r = jsval_obj(ret);
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

    string = heap_alloc_zero(sizeof(StringInstance));
    if(!string)
        return E_OUTOFMEMORY;

    if(object_prototype)
        hres = init_dispex(&string->dispex, ctx, &String_info, object_prototype);
    else
        hres = init_dispex_from_constr(&string->dispex, ctx, &StringInst_info, ctx->string_constr);
    if(FAILED(hres)) {
        heap_free(string);
        return hres;
    }

    string->str = jsstr_addref(str);
    *ret = string;
    return S_OK;
}

static const builtin_prop_t StringConstr_props[] = {
    {fromCharCodeW,    StringConstr_fromCharCode,    PROPF_METHOD},
};

static const builtin_info_t StringConstr_info = {
    JSCLASS_FUNCTION,
    DEFAULT_FUNCTION_VALUE,
    sizeof(StringConstr_props)/sizeof(*StringConstr_props),
    StringConstr_props,
    NULL,
    NULL
};

HRESULT create_string_constr(script_ctx_t *ctx, jsdisp_t *object_prototype, jsdisp_t **ret)
{
    StringInstance *string;
    HRESULT hres;

    static const WCHAR StringW[] = {'S','t','r','i','n','g',0};

    hres = string_alloc(ctx, object_prototype, jsstr_empty(), &string);
    if(FAILED(hres))
        return hres;

    hres = create_builtin_constructor(ctx, StringConstr_value, StringW, &StringConstr_info,
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
