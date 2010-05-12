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

    WCHAR *str;
    DWORD length;
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

static inline StringInstance *string_from_vdisp(vdisp_t *vdisp)
{
    return (StringInstance*)vdisp->u.jsdisp;
}

static inline StringInstance *string_this(vdisp_t *jsthis)
{
    return is_vclass(jsthis, JSCLASS_STRING) ? string_from_vdisp(jsthis) : NULL;
}

static HRESULT get_string_val(script_ctx_t *ctx, vdisp_t *jsthis, jsexcept_t *ei,
        const WCHAR **str, DWORD *len, BSTR *val_str)
{
    StringInstance *string;
    VARIANT this_var;
    HRESULT hres;

    if((string = string_this(jsthis))) {
        *str = string->str;
        *len = string->length;
        *val_str = NULL;
        return S_OK;
    }

    V_VT(&this_var) = VT_DISPATCH;
    V_DISPATCH(&this_var) = jsthis->u.disp;
    hres = to_string(ctx, &this_var, ei, val_str);
    if(FAILED(hres))
        return hres;

    *str = *val_str;
    *len = SysStringLen(*val_str);
    return S_OK;
}

static HRESULT String_length(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    TRACE("%p\n", jsthis);

    switch(flags) {
    case DISPATCH_PROPERTYGET: {
        StringInstance *string = string_from_vdisp(jsthis);

        V_VT(retv) = VT_I4;
        V_I4(retv) = string->length;
        break;
    }
    default:
        FIXME("unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT stringobj_to_string(vdisp_t *jsthis, VARIANT *retv)
{
    StringInstance *string;

    if(!(string = string_this(jsthis))) {
        WARN("this is not a string object\n");
        return E_FAIL;
    }

    if(retv) {
        BSTR str = SysAllocString(string->str);
        if(!str)
            return E_OUTOFMEMORY;

        V_VT(retv) = VT_BSTR;
        V_BSTR(retv) = str;
    }
    return S_OK;
}

/* ECMA-262 3rd Edition    15.5.4.2 */
static HRESULT String_toString(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    TRACE("\n");

    return stringobj_to_string(jsthis, retv);
}

/* ECMA-262 3rd Edition    15.5.4.2 */
static HRESULT String_valueOf(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    TRACE("\n");

    return stringobj_to_string(jsthis, retv);
}

static HRESULT do_attributeless_tag_format(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp, const WCHAR *tagname)
{
    const WCHAR *str;
    DWORD length;
    BSTR val_str = NULL;
    HRESULT hres;

    static const WCHAR tagfmt[] = {'<','%','s','>','%','s','<','/','%','s','>',0};

    hres = get_string_val(ctx, jsthis, ei, &str, &length, &val_str);
    if(FAILED(hres))
        return hres;

    if(retv) {
        BSTR ret = SysAllocStringLen(NULL, length + 2*strlenW(tagname) + 5);
        if(!ret) {
            SysFreeString(val_str);
            return E_OUTOFMEMORY;
        }

        sprintfW(ret, tagfmt, tagname, str, tagname);

        V_VT(retv) = VT_BSTR;
        V_BSTR(retv) = ret;
    }

    SysFreeString(val_str);
    return S_OK;
}

static HRESULT do_attribute_tag_format(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags,
        DISPPARAMS *dp, VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp,
        const WCHAR *tagname, const WCHAR *attr)
{
    static const WCHAR tagfmtW[]
        = {'<','%','s',' ','%','s','=','\"','%','s','\"','>','%','s','<','/','%','s','>',0};
    static const WCHAR undefinedW[] = {'u','n','d','e','f','i','n','e','d',0};

    StringInstance *string;
    const WCHAR *str;
    DWORD length;
    BSTR attr_value, val_str = NULL;
    HRESULT hres;

    if(!(string = string_this(jsthis))) {
        VARIANT this;

        V_VT(&this) = VT_DISPATCH;
        V_DISPATCH(&this) = jsthis->u.disp;

        hres = to_string(ctx, &this, ei, &val_str);
        if(FAILED(hres))
            return hres;

        str = val_str;
        length = SysStringLen(val_str);
    }
    else {
        str = string->str;
        length = string->length;
    }

    if(arg_cnt(dp)) {
        hres = to_string(ctx, get_arg(dp, 0), ei, &attr_value);
        if(FAILED(hres)) {
            SysFreeString(val_str);
            return hres;
        }
    }
    else {
        attr_value = SysAllocString(undefinedW);
        if(!attr_value) {
            SysFreeString(val_str);
            return E_OUTOFMEMORY;
        }
    }

    if(retv) {
        BSTR ret = SysAllocStringLen(NULL, length + 2*strlenW(tagname)
                + strlenW(attr) + SysStringLen(attr_value) + 9);
        if(!ret) {
            SysFreeString(attr_value);
            SysFreeString(val_str);
            return E_OUTOFMEMORY;
        }

        sprintfW(ret, tagfmtW, tagname, attr, attr_value, str, tagname);

        V_VT(retv) = VT_BSTR;
        V_BSTR(retv) = ret;
    }

    SysFreeString(attr_value);
    SysFreeString(val_str);
    return S_OK;
}

static HRESULT String_anchor(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    static const WCHAR fontW[] = {'A',0};
    static const WCHAR colorW[] = {'N','A','M','E',0};

    return do_attribute_tag_format(ctx, jsthis, flags, dp, retv, ei, sp, fontW, colorW);
}

static HRESULT String_big(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    static const WCHAR bigtagW[] = {'B','I','G',0};
    return do_attributeless_tag_format(ctx, jsthis, flags, dp, retv, ei, sp, bigtagW);
}

static HRESULT String_blink(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    static const WCHAR blinktagW[] = {'B','L','I','N','K',0};
    return do_attributeless_tag_format(ctx, jsthis, flags, dp, retv, ei, sp, blinktagW);
}

static HRESULT String_bold(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    static const WCHAR boldtagW[] = {'B',0};
    return do_attributeless_tag_format(ctx, jsthis, flags, dp, retv, ei, sp, boldtagW);
}

/* ECMA-262 3rd Edition    15.5.4.5 */
static HRESULT String_charAt(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    const WCHAR *str;
    DWORD length;
    BSTR ret, val_str;
    INT pos = 0;
    HRESULT hres;

    TRACE("\n");

    hres = get_string_val(ctx, jsthis, ei, &str, &length, &val_str);
    if(FAILED(hres))
        return hres;

    if(arg_cnt(dp)) {
        VARIANT num;

        hres = to_integer(ctx, get_arg(dp, 0), ei, &num);
        if(FAILED(hres)) {
            SysFreeString(val_str);
            return hres;
        }

        if(V_VT(&num) == VT_I4) {
            pos = V_I4(&num);
        }else {
            WARN("pos = %lf\n", V_R8(&num));
            pos = -1;
        }
    }

    if(!retv) {
        SysFreeString(val_str);
        return S_OK;
    }

    if(0 <= pos && pos < length)
        ret = SysAllocStringLen(str+pos, 1);
    else
        ret = SysAllocStringLen(NULL, 0);
    SysFreeString(val_str);
    if(!ret) {
        return E_OUTOFMEMORY;
    }

    V_VT(retv) = VT_BSTR;
    V_BSTR(retv) = ret;
    return S_OK;
}

/* ECMA-262 3rd Edition    15.5.4.5 */
static HRESULT String_charCodeAt(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    const WCHAR *str;
    BSTR val_str;
    DWORD length, idx = 0;
    HRESULT hres;

    TRACE("\n");

    hres = get_string_val(ctx, jsthis, ei, &str, &length, &val_str);
    if(FAILED(hres))
        return hres;

    if(arg_cnt(dp) > 0) {
        VARIANT v;

        hres = to_integer(ctx, get_arg(dp, 0), ei, &v);
        if(FAILED(hres)) {
            SysFreeString(val_str);
            return hres;
        }

        if(V_VT(&v) != VT_I4 || V_I4(&v) < 0 || V_I4(&v) >= length) {
            if(retv) num_set_nan(&v);
            SysFreeString(val_str);
            return S_OK;
        }

        idx = V_I4(&v);
    }

    if(retv) {
        V_VT(retv) = VT_I4;
        V_I4(retv) = str[idx];
    }

    SysFreeString(val_str);
    return S_OK;
}

/* ECMA-262 3rd Edition    15.5.4.6 */
static HRESULT String_concat(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    BSTR *strs = NULL, ret = NULL;
    DWORD len = 0, i, l, str_cnt;
    VARIANT var;
    WCHAR *ptr;
    HRESULT hres;

    TRACE("\n");

    str_cnt = arg_cnt(dp)+1;
    strs = heap_alloc_zero(str_cnt * sizeof(BSTR));
    if(!strs)
        return E_OUTOFMEMORY;

    V_VT(&var) = VT_DISPATCH;
    V_DISPATCH(&var) = jsthis->u.disp;

    hres = to_string(ctx, &var, ei, strs);
    if(SUCCEEDED(hres)) {
        for(i=0; i < arg_cnt(dp); i++) {
            hres = to_string(ctx, get_arg(dp, i), ei, strs+i+1);
            if(FAILED(hres))
                break;
        }
    }

    if(SUCCEEDED(hres)) {
        for(i=0; i < str_cnt; i++)
            len += SysStringLen(strs[i]);

        ptr = ret = SysAllocStringLen(NULL, len);

        for(i=0; i < str_cnt; i++) {
            l = SysStringLen(strs[i]);
            memcpy(ptr, strs[i], l*sizeof(WCHAR));
            ptr += l;
        }
    }

    for(i=0; i < str_cnt; i++)
        SysFreeString(strs[i]);
    heap_free(strs);

    if(FAILED(hres))
        return hres;

    if(retv) {
        V_VT(retv) = VT_BSTR;
        V_BSTR(retv) = ret;
    }else {
        SysFreeString(ret);
    }
    return S_OK;
}

static HRESULT String_fixed(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    static const WCHAR fixedtagW[] = {'T','T',0};
    return do_attributeless_tag_format(ctx, jsthis, flags, dp, retv, ei, sp, fixedtagW);
}

static HRESULT String_fontcolor(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    static const WCHAR fontW[] = {'F','O','N','T',0};
    static const WCHAR colorW[] = {'C','O','L','O','R',0};

    return do_attribute_tag_format(ctx, jsthis, flags, dp, retv, ei, sp, fontW, colorW);
}

static HRESULT String_fontsize(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    static const WCHAR fontW[] = {'F','O','N','T',0};
    static const WCHAR colorW[] = {'S','I','Z','E',0};

    return do_attribute_tag_format(ctx, jsthis, flags, dp, retv, ei, sp, fontW, colorW);
}

static HRESULT String_indexOf(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    DWORD length, pos = 0;
    const WCHAR *str;
    BSTR search_str, val_str;
    INT ret = -1;
    HRESULT hres;

    TRACE("\n");

    hres = get_string_val(ctx, jsthis, ei, &str, &length, &val_str);
    if(FAILED(hres))
        return hres;

    if(!arg_cnt(dp)) {
        if(retv) {
            V_VT(retv) = VT_I4;
            V_I4(retv) = -1;
        }
        SysFreeString(val_str);
        return S_OK;
    }

    hres = to_string(ctx, get_arg(dp,0), ei, &search_str);
    if(FAILED(hres)) {
        SysFreeString(val_str);
        return hres;
    }

    if(arg_cnt(dp) >= 2) {
        VARIANT ival;

        hres = to_integer(ctx, get_arg(dp,1), ei, &ival);
        if(SUCCEEDED(hres)) {
            if(V_VT(&ival) == VT_I4)
                pos = V_VT(&ival) > 0 ? V_I4(&ival) : 0;
            else
                pos = V_R8(&ival) > 0.0 ? length : 0;
            if(pos > length)
                pos = length;
        }
    }

    if(SUCCEEDED(hres)) {
        const WCHAR *ptr;

        ptr = strstrW(str+pos, search_str);
        if(ptr)
            ret = ptr - str;
        else
            ret = -1;
    }

    SysFreeString(search_str);
    SysFreeString(val_str);
    if(FAILED(hres))
        return hres;

    if(retv) {
        V_VT(retv) = VT_I4;
        V_I4(retv) = ret;
    }
    return S_OK;
}

static HRESULT String_italics(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    static const WCHAR italicstagW[] = {'I',0};
    return do_attributeless_tag_format(ctx, jsthis, flags, dp, retv, ei, sp, italicstagW);
}

/* ECMA-262 3rd Edition    15.5.4.8 */
static HRESULT String_lastIndexOf(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    BSTR search_str, val_str;
    DWORD length, pos, search_len;
    const WCHAR *str;
    INT ret = -1;
    HRESULT hres;

    TRACE("\n");

    hres = get_string_val(ctx, jsthis, ei, &str, &length, &val_str);
    if(FAILED(hres))
        return hres;

    if(!arg_cnt(dp)) {
        if(retv) {
            V_VT(retv) = VT_I4;
            V_I4(retv) = -1;
        }
        SysFreeString(val_str);
        return S_OK;
    }

    hres = to_string(ctx, get_arg(dp,0), ei, &search_str);
    if(FAILED(hres)) {
        SysFreeString(val_str);
        return hres;
    }

    search_len = SysStringLen(search_str);

    if(arg_cnt(dp) >= 2) {
        VARIANT ival;

        hres = to_integer(ctx, get_arg(dp,1), ei, &ival);
        if(SUCCEEDED(hres)) {
            if(V_VT(&ival) == VT_I4)
                pos = V_VT(&ival) > 0 ? V_I4(&ival) : 0;
            else
                pos = V_R8(&ival) > 0.0 ? length : 0;
            if(pos > length)
                pos = length;
        }
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

    SysFreeString(search_str);
    SysFreeString(val_str);
    if(FAILED(hres))
        return hres;

    if(retv) {
        V_VT(retv) = VT_I4;
        V_I4(retv) = ret;
    }
    return S_OK;
}

static HRESULT String_link(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    static const WCHAR fontW[] = {'A',0};
    static const WCHAR colorW[] = {'H','R','E','F',0};

    return do_attribute_tag_format(ctx, jsthis, flags, dp, retv, ei, sp, fontW, colorW);
}

/* ECMA-262 3rd Edition    15.5.4.10 */
static HRESULT String_match(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    const WCHAR *str;
    DispatchEx *regexp;
    VARIANT *arg_var;
    DWORD length;
    BSTR val_str = NULL;
    HRESULT hres = S_OK;

    TRACE("\n");

    if(!arg_cnt(dp)) {
        if(retv) {
            V_VT(retv) = VT_NULL;
        }

        return S_OK;
    }

    arg_var = get_arg(dp, 0);
    switch(V_VT(arg_var)) {
    case VT_DISPATCH:
        regexp = iface_to_jsdisp((IUnknown*)V_DISPATCH(arg_var));
        if(regexp) {
            if(is_class(regexp, JSCLASS_REGEXP))
                break;
            jsdisp_release(regexp);
        }
    default: {
        BSTR match_str;

        hres = to_string(ctx, arg_var, ei, &match_str);
        if(FAILED(hres))
            return hres;

        hres = create_regexp(ctx, match_str, SysStringLen(match_str), 0, &regexp);
        SysFreeString(match_str);
        if(FAILED(hres))
            return hres;
    }
    }

    hres = get_string_val(ctx, jsthis, ei, &str, &length, &val_str);
    if(SUCCEEDED(hres)) {
        if(!val_str)
            val_str = SysAllocStringLen(str, length);
        if(val_str)
            hres = regexp_string_match(ctx, regexp, val_str, retv, ei);
        else
            hres = E_OUTOFMEMORY;
    }

    jsdisp_release(regexp);
    SysFreeString(val_str);
    return hres;
}

typedef struct {
    WCHAR *buf;
    DWORD size;
    DWORD len;
} strbuf_t;

static HRESULT strbuf_append(strbuf_t *buf, const WCHAR *str, DWORD len)
{
    if(!len)
        return S_OK;

    if(len + buf->len > buf->size) {
        WCHAR *new_buf;
        DWORD new_size;

        new_size = buf->size ? buf->size<<1 : 16;
        if(new_size < buf->len+len)
            new_size = buf->len+len;
        if(buf->buf)
            new_buf = heap_realloc(buf->buf, new_size*sizeof(WCHAR));
        else
            new_buf = heap_alloc(new_size*sizeof(WCHAR));
        if(!new_buf)
            return E_OUTOFMEMORY;

        buf->buf = new_buf;
        buf->size = new_size;
    }

    memcpy(buf->buf+buf->len, str, len*sizeof(WCHAR));
    buf->len += len;
    return S_OK;
}

static HRESULT rep_call(script_ctx_t *ctx, DispatchEx *func, const WCHAR *str, match_result_t *match,
        match_result_t *parens, DWORD parens_cnt, BSTR *ret, jsexcept_t *ei, IServiceProvider *caller)
{
    DISPPARAMS dp = {NULL, NULL, 0, 0};
    VARIANTARG *args, *arg;
    VARIANT var;
    DWORD i;
    HRESULT hres = S_OK;

    dp.cArgs = parens_cnt+3;
    dp.rgvarg = args = heap_alloc_zero(sizeof(VARIANT)*dp.cArgs);
    if(!args)
        return E_OUTOFMEMORY;

    arg = get_arg(&dp,0);
    V_VT(arg) = VT_BSTR;
    V_BSTR(arg) = SysAllocStringLen(match->str, match->len);
    if(!V_BSTR(arg))
        hres = E_OUTOFMEMORY;

    if(SUCCEEDED(hres)) {
        for(i=0; i < parens_cnt; i++) {
            arg = get_arg(&dp,i+1);
            V_VT(arg) = VT_BSTR;
            V_BSTR(arg) = SysAllocStringLen(parens[i].str, parens[i].len);
            if(!V_BSTR(arg)) {
               hres = E_OUTOFMEMORY;
               break;
            }
        }
    }

    if(SUCCEEDED(hres)) {
        arg = get_arg(&dp,parens_cnt+1);
        V_VT(arg) = VT_I4;
        V_I4(arg) = match->str - str;

        arg = get_arg(&dp,parens_cnt+2);
        V_VT(arg) = VT_BSTR;
        V_BSTR(arg) = SysAllocString(str);
        if(!V_BSTR(arg))
            hres = E_OUTOFMEMORY;
    }

    if(SUCCEEDED(hres))
        hres = jsdisp_call_value(func, DISPATCH_METHOD, &dp, &var, ei, caller);

    for(i=0; i < parens_cnt+3; i++) {
        if(i != parens_cnt+1)
            SysFreeString(V_BSTR(get_arg(&dp,i)));
    }
    heap_free(args);

    if(FAILED(hres))
        return hres;

    hres = to_string(ctx, &var, ei, ret);
    VariantClear(&var);
    return hres;
}

/* ECMA-262 3rd Edition    15.5.4.11 */
static HRESULT String_replace(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    const WCHAR *str;
    DWORD parens_cnt = 0, parens_size=0, rep_len=0, length;
    BSTR rep_str = NULL, match_str = NULL, ret_str, val_str;
    DispatchEx *rep_func = NULL, *regexp = NULL;
    match_result_t *parens = NULL, match, **parens_ptr = &parens;
    strbuf_t ret = {NULL,0,0};
    DWORD re_flags = 0;
    VARIANT *arg_var;
    HRESULT hres = S_OK;

    TRACE("\n");

    hres = get_string_val(ctx, jsthis, ei, &str, &length, &val_str);
    if(FAILED(hres))
        return hres;

    if(!arg_cnt(dp)) {
        if(retv) {
            if(!val_str) {
                val_str = SysAllocStringLen(str, length);
                if(!val_str)
                    return E_OUTOFMEMORY;
            }

            V_VT(retv) = VT_BSTR;
            V_BSTR(retv) = val_str;
        }
        return S_OK;
    }

    arg_var = get_arg(dp, 0);
    switch(V_VT(arg_var)) {
    case VT_DISPATCH:
        regexp = iface_to_jsdisp((IUnknown*)V_DISPATCH(arg_var));
        if(regexp) {
            if(is_class(regexp, JSCLASS_REGEXP)) {
                break;
            }else {
                jsdisp_release(regexp);
                regexp = NULL;
            }
        }

    default:
        hres = to_string(ctx, arg_var, ei, &match_str);
        if(FAILED(hres)) {
            SysFreeString(val_str);
            return hres;
        }
    }

    if(arg_cnt(dp) >= 2) {
        arg_var = get_arg(dp,1);
        switch(V_VT(arg_var)) {
        case VT_DISPATCH:
            rep_func = iface_to_jsdisp((IUnknown*)V_DISPATCH(arg_var));
            if(rep_func) {
                if(is_class(rep_func, JSCLASS_FUNCTION)) {
                    break;
                }else {
                    jsdisp_release(rep_func);
                    rep_func = NULL;
                }
            }

        default:
            hres = to_string(ctx, arg_var, ei, &rep_str);
            if(FAILED(hres))
                break;

            rep_len = SysStringLen(rep_str);
            if(!strchrW(rep_str, '$'))
                parens_ptr = NULL;
        }
    }

    if(SUCCEEDED(hres)) {
        const WCHAR *cp, *ecp;

        cp = ecp = str;

        while(1) {
            if(regexp) {
                hres = regexp_match_next(ctx, regexp, re_flags, str, length, &cp, parens_ptr,
                        &parens_size, &parens_cnt, &match);
                re_flags = REM_CHECK_GLOBAL;

                if(hres == S_FALSE) {
                    hres = S_OK;
                    break;
                }
                if(FAILED(hres))
                    break;
            }else {
                match.str = strstrW(cp, match_str);
                if(!match.str)
                    break;
                match.len = SysStringLen(match_str);
                cp = match.str+match.len;
            }

            hres = strbuf_append(&ret, ecp, match.str-ecp);
            ecp = match.str+match.len;
            if(FAILED(hres))
                break;

            if(rep_func) {
                BSTR cstr;

                hres = rep_call(ctx, rep_func, str, &match, parens, parens_cnt, &cstr, ei, caller);
                if(FAILED(hres))
                    break;

                hres = strbuf_append(&ret, cstr, SysStringLen(cstr));
                SysFreeString(cstr);
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
                        hres = strbuf_append(&ret, match.str, match.len);
                        ptr = ptr2+2;
                        break;
                    case '`':
                        hres = strbuf_append(&ret, str, match.str-str);
                        ptr = ptr2+2;
                        break;
                    case '\'':
                        hres = strbuf_append(&ret, ecp, (str+length)-ecp);
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
                        if(isdigitW(ptr2[2]) && idx*10 + (ptr2[2]-'0') <= parens_cnt) {
                            idx = idx*10 + (ptr[2]-'0');
                            ptr = ptr2+3;
                        }else if(idx && idx <= parens_cnt) {
                            ptr = ptr2+2;
                        }else {
                            hres = strbuf_append(&ret, ptr2, 1);
                            ptr = ptr2+1;
                            break;
                        }

                        hres = strbuf_append(&ret, parens[idx-1].str, parens[idx-1].len);
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
        }

        if(SUCCEEDED(hres))
            hres = strbuf_append(&ret, ecp, (str+length)-ecp);
    }

    if(rep_func)
        jsdisp_release(rep_func);
    if(regexp)
        jsdisp_release(regexp);
    SysFreeString(val_str);
    SysFreeString(rep_str);
    SysFreeString(match_str);
    heap_free(parens);

    if(SUCCEEDED(hres) && retv) {
        ret_str = SysAllocStringLen(ret.buf, ret.len);
        if(!ret_str)
            return E_OUTOFMEMORY;

        V_VT(retv) = VT_BSTR;
        V_BSTR(retv) = ret_str;
        TRACE("= %s\n", debugstr_w(ret_str));
    }

    heap_free(ret.buf);
    return hres;
}

static HRESULT String_search(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    DispatchEx *regexp = NULL;
    const WCHAR *str, *cp;
    match_result_t match;
    VARIANT *arg;
    DWORD length;
    BSTR val_str;
    HRESULT hres;

    TRACE("\n");

    hres = get_string_val(ctx, jsthis, ei, &str, &length, &val_str);
    if(FAILED(hres))
        return hres;

    if(!arg_cnt(dp)) {
        if(retv)
            V_VT(retv) = VT_NULL;
        SysFreeString(val_str);
        return S_OK;
    }

    arg = get_arg(dp,0);
    if(V_VT(arg) == VT_DISPATCH) {
        regexp = iface_to_jsdisp((IUnknown*)V_DISPATCH(arg));
        if(regexp) {
            if(!is_class(regexp, JSCLASS_REGEXP)) {
                jsdisp_release(regexp);
                regexp = NULL;
            }
        }
    }

    if(!regexp) {
        hres = create_regexp_var(ctx, arg, NULL, &regexp);
        if(FAILED(hres)) {
            SysFreeString(val_str);
            return hres;
        }
    }

    cp = str;
    hres = regexp_match_next(ctx, regexp, REM_RESET_INDEX, str, length, &cp, NULL, NULL, NULL, &match);
    SysFreeString(val_str);
    jsdisp_release(regexp);
    if(FAILED(hres))
        return hres;

    if(retv) {
        V_VT(retv) = VT_I4;
        V_I4(retv) = hres == S_OK ? match.str-str : -1;
    }
    return S_OK;
}

/* ECMA-262 3rd Edition    15.5.4.13 */
static HRESULT String_slice(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    const WCHAR *str;
    BSTR val_str;
    DWORD length;
    INT start=0, end;
    VARIANT v;
    HRESULT hres;

    TRACE("\n");

    hres = get_string_val(ctx, jsthis, ei, &str, &length, &val_str);
    if(FAILED(hres))
        return hres;

    if(arg_cnt(dp)) {
        hres = to_integer(ctx, get_arg(dp,0), ei, &v);
        if(FAILED(hres)) {
            SysFreeString(val_str);
            return hres;
        }

        if(V_VT(&v) == VT_I4) {
            start = V_I4(&v);
            if(start < 0) {
                start = length + start;
                if(start < 0)
                    start = 0;
            }else if(start > length) {
                start = length;
            }
        }else {
            start = V_R8(&v) < 0.0 ? 0 : length;
        }
    }else {
        start = 0;
    }

    if(arg_cnt(dp) >= 2) {
        hres = to_integer(ctx, get_arg(dp,1), ei, &v);
        if(FAILED(hres)) {
            SysFreeString(val_str);
            return hres;
        }

        if(V_VT(&v) == VT_I4) {
            end = V_I4(&v);
            if(end < 0) {
                end = length + end;
                if(end < 0)
                    end = 0;
            }else if(end > length) {
                end = length;
            }
        }else {
            end = V_R8(&v) < 0.0 ? 0 : length;
        }
    }else {
        end = length;
    }

    if(end < start)
        end = start;

    if(retv) {
        BSTR retstr = SysAllocStringLen(str+start, end-start);
        if(!retstr) {
            SysFreeString(val_str);
            return E_OUTOFMEMORY;
        }

        V_VT(retv) = VT_BSTR;
        V_BSTR(retv) = retstr;
    }

    SysFreeString(val_str);
    return S_OK;
}

static HRESULT String_small(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    static const WCHAR smalltagW[] = {'S','M','A','L','L',0};
    return do_attributeless_tag_format(ctx, jsthis, flags, dp, retv, ei, sp, smalltagW);
}

static HRESULT String_split(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    match_result_t *match_result = NULL;
    DWORD length, match_cnt, i, match_len = 0;
    const WCHAR *str, *ptr, *ptr2;
    BOOL use_regexp = FALSE;
    VARIANT *arg, var;
    DispatchEx *array;
    BSTR val_str, match_str = NULL;
    HRESULT hres;

    TRACE("\n");

    if(arg_cnt(dp) != 1) {
        FIXME("unsupported args\n");
        return E_NOTIMPL;
    }

    hres = get_string_val(ctx, jsthis, ei, &str, &length, &val_str);
    if(FAILED(hres))
        return hres;

    arg = get_arg(dp, 0);
    switch(V_VT(arg)) {
    case VT_DISPATCH: {
        DispatchEx *regexp;

        regexp = iface_to_jsdisp((IUnknown*)V_DISPATCH(arg));
        if(regexp) {
            if(is_class(regexp, JSCLASS_REGEXP)) {
                use_regexp = TRUE;
                hres = regexp_match(ctx, regexp, str, length, TRUE, &match_result, &match_cnt);
                jsdisp_release(regexp);
                if(FAILED(hres)) {
                    SysFreeString(val_str);
                    return hres;
                }
                break;
            }
            jsdisp_release(regexp);
        }
    }
    default:
        hres = to_string(ctx, arg, ei, &match_str);
        if(FAILED(hres)) {
            SysFreeString(val_str);
            return hres;
        }

        match_len = SysStringLen(match_str);
        if(!match_len) {
            SysFreeString(match_str);
            match_str = NULL;
        }
    }

    hres = create_array(ctx, 0, &array);

    if(SUCCEEDED(hres)) {
        ptr = str;
        for(i=0;; i++) {
            if(use_regexp) {
                if(i == match_cnt)
                    break;
                ptr2 = match_result[i].str;
            }else if(match_str) {
                ptr2 = strstrW(ptr, match_str);
                if(!ptr2)
                    break;
            }else {
                if(!*ptr)
                    break;
                ptr2 = ptr+1;
            }

            V_VT(&var) = VT_BSTR;
            V_BSTR(&var) = SysAllocStringLen(ptr, ptr2-ptr);
            if(!V_BSTR(&var)) {
                hres = E_OUTOFMEMORY;
                break;
            }

            hres = jsdisp_propput_idx(array, i, &var, ei, sp);
            SysFreeString(V_BSTR(&var));
            if(FAILED(hres))
                break;

            if(use_regexp)
                ptr = match_result[i].str + match_result[i].len;
            else if(match_str)
                ptr = ptr2 + match_len;
            else
                ptr++;
        }
    }

    if(SUCCEEDED(hres) && (match_str || use_regexp)) {
        DWORD len = (str+length) - ptr;

        if(len || match_str) {
            V_VT(&var) = VT_BSTR;
            V_BSTR(&var) = SysAllocStringLen(ptr, len);

            if(V_BSTR(&var)) {
                hres = jsdisp_propput_idx(array, i, &var, ei, sp);
                SysFreeString(V_BSTR(&var));
            }else {
                hres = E_OUTOFMEMORY;
            }
        }
    }

    SysFreeString(match_str);
    SysFreeString(val_str);
    heap_free(match_result);

    if(SUCCEEDED(hres) && retv) {
        V_VT(retv) = VT_DISPATCH;
        V_DISPATCH(retv) = (IDispatch*)_IDispatchEx_(array);
    }else {
        jsdisp_release(array);
    }

    return hres;
}

static HRESULT String_strike(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    static const WCHAR striketagW[] = {'S','T','R','I','K','E',0};
    return do_attributeless_tag_format(ctx, jsthis, flags, dp, retv, ei, sp, striketagW);
}

static HRESULT String_sub(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    static const WCHAR subtagW[] = {'S','U','B',0};
    return do_attributeless_tag_format(ctx, jsthis, flags, dp, retv, ei, sp, subtagW);
}

/* ECMA-262 3rd Edition    15.5.4.15 */
static HRESULT String_substring(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    const WCHAR *str;
    BSTR val_str;
    INT start=0, end;
    DWORD length;
    VARIANT v;
    HRESULT hres;

    TRACE("\n");

    hres = get_string_val(ctx, jsthis, ei, &str, &length, &val_str);
    if(FAILED(hres))
        return hres;

    if(arg_cnt(dp) >= 1) {
        hres = to_integer(ctx, get_arg(dp,0), ei, &v);
        if(FAILED(hres)) {
            SysFreeString(val_str);
            return hres;
        }

        if(V_VT(&v) == VT_I4) {
            start = V_I4(&v);
            if(start < 0)
                start = 0;
            else if(start >= length)
                start = length;
        }else {
            start = V_R8(&v) < 0.0 ? 0 : length;
        }
    }

    if(arg_cnt(dp) >= 2) {
        hres = to_integer(ctx, get_arg(dp,1), ei, &v);
        if(FAILED(hres)) {
            SysFreeString(val_str);
            return hres;
        }

        if(V_VT(&v) == VT_I4) {
            end = V_I4(&v);
            if(end < 0)
                end = 0;
            else if(end > length)
                end = length;
        }else {
            end = V_R8(&v) < 0.0 ? 0 : length;
        }
    }else {
        end = length;
    }

    if(start > end) {
        INT tmp = start;
        start = end;
        end = tmp;
    }

    if(retv) {
        V_VT(retv) = VT_BSTR;
        V_BSTR(retv) = SysAllocStringLen(str+start, end-start);
        if(!V_BSTR(retv)) {
            SysFreeString(val_str);
            return E_OUTOFMEMORY;
        }
    }
    SysFreeString(val_str);
    return S_OK;
}

/* ECMA-262 3rd Edition    B.2.3 */
static HRESULT String_substr(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    BSTR val_str;
    const WCHAR *str;
    INT start=0, len;
    DWORD length;
    VARIANT v;
    HRESULT hres;

    TRACE("\n");

    hres = get_string_val(ctx, jsthis, ei, &str, &length, &val_str);
    if(FAILED(hres))
        return hres;

    if(arg_cnt(dp) >= 1) {
        hres = to_integer(ctx, get_arg(dp,0), ei, &v);
        if(FAILED(hres)) {
            SysFreeString(val_str);
            return hres;
        }

        if(V_VT(&v) == VT_I4) {
            start = V_I4(&v);
            if(start < 0)
                start = 0;
            else if(start >= length)
                start = length;
        }else {
            start = V_R8(&v) < 0.0 ? 0 : length;
        }
    }

    if(arg_cnt(dp) >= 2) {
        hres = to_integer(ctx, get_arg(dp,1), ei, &v);
        if(FAILED(hres)) {
            SysFreeString(val_str);
            return hres;
        }

        if(V_VT(&v) == VT_I4) {
            len = V_I4(&v);
            if(len < 0)
                len = 0;
            else if(len > length-start)
                len = length-start;
        }else {
            len = V_R8(&v) < 0.0 ? 0 : length-start;
        }
    }else {
        len = length-start;
    }

    hres = S_OK;
    if(retv) {
        V_VT(retv) = VT_BSTR;
        V_BSTR(retv) = SysAllocStringLen(str+start, len);
        if(!V_BSTR(retv))
            hres = E_OUTOFMEMORY;
    }

    SysFreeString(val_str);
    return hres;
}

static HRESULT String_sup(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    static const WCHAR suptagW[] = {'S','U','P',0};
    return do_attributeless_tag_format(ctx, jsthis, flags, dp, retv, ei, sp, suptagW);
}

static HRESULT String_toLowerCase(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    const WCHAR* str;
    DWORD length;
    BSTR val_str;
    HRESULT  hres;

    TRACE("\n");

    hres = get_string_val(ctx, jsthis, ei, &str, &length, &val_str);
    if(FAILED(hres))
        return hres;

    if(retv) {
        if(!val_str) {
            val_str = SysAllocStringLen(str, length);
            if(!val_str)
                return E_OUTOFMEMORY;
        }

        strlwrW(val_str);

        V_VT(retv) = VT_BSTR;
        V_BSTR(retv) = val_str;
    }
    else SysFreeString(val_str);
    return S_OK;
}

static HRESULT String_toUpperCase(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    const WCHAR* str;
    DWORD length;
    BSTR val_str;
    HRESULT hres;

    TRACE("\n");

    hres = get_string_val(ctx, jsthis, ei, &str, &length, &val_str);
    if(FAILED(hres))
        return hres;

    if(retv) {
        if(!val_str) {
            val_str = SysAllocStringLen(str, length);
            if(!val_str)
                return E_OUTOFMEMORY;
        }

        struprW(val_str);

        V_VT(retv) = VT_BSTR;
        V_BSTR(retv) = val_str;
    }
    else SysFreeString(val_str);
    return S_OK;
}

static HRESULT String_toLocaleLowerCase(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT String_toLocaleUpperCase(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT String_localeCompare(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT String_value(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    StringInstance *This = string_from_vdisp(jsthis);

    TRACE("\n");

    switch(flags) {
    case INVOKE_FUNC:
        return throw_type_error(ctx, ei, IDS_NOT_FUNC, NULL);
    case DISPATCH_PROPERTYGET: {
        BSTR str = SysAllocString(This->str);
        if(!str)
            return E_OUTOFMEMORY;

        V_VT(retv) = VT_BSTR;
        V_BSTR(retv) = str;
        break;
    }
    default:
        FIXME("flags %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

static void String_destructor(DispatchEx *dispex)
{
    StringInstance *This = (StringInstance*)dispex;

    heap_free(This->str);
    heap_free(This);
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
    {lengthW,                String_length,                0},
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
    {NULL, String_value, 0},
    sizeof(String_props)/sizeof(*String_props),
    String_props,
    String_destructor,
    NULL
};

/* ECMA-262 3rd Edition    15.5.3.2 */
static HRESULT StringConstr_fromCharCode(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags,
        DISPPARAMS *dp, VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    DWORD i, code;
    BSTR ret;
    HRESULT hres;

    ret = SysAllocStringLen(NULL, arg_cnt(dp));
    if(!ret)
        return E_OUTOFMEMORY;

    for(i=0; i<arg_cnt(dp); i++) {
        hres = to_uint32(ctx, get_arg(dp, i), ei, &code);
        if(FAILED(hres)) {
            SysFreeString(ret);
            return hres;
        }

        ret[i] = code;
    }

    if(retv) {
        V_VT(retv) = VT_BSTR;
        V_BSTR(retv) = ret;
    }
    else SysFreeString(ret);

    return S_OK;
}

static HRESULT StringConstr_value(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    HRESULT hres;

    TRACE("\n");

    switch(flags) {
    case INVOKE_FUNC: {
        BSTR str;

        if(arg_cnt(dp)) {
            hres = to_string(ctx, get_arg(dp, 0), ei, &str);
            if(FAILED(hres))
                return hres;
        }else {
            str = SysAllocStringLen(NULL, 0);
            if(!str)
                return E_OUTOFMEMORY;
        }

        V_VT(retv) = VT_BSTR;
        V_BSTR(retv) = str;
        break;
    }
    case DISPATCH_CONSTRUCT: {
        DispatchEx *ret;

        if(arg_cnt(dp)) {
            BSTR str;

            hres = to_string(ctx, get_arg(dp, 0), ei, &str);
            if(FAILED(hres))
                return hres;

            hres = create_string(ctx, str, SysStringLen(str), &ret);
            SysFreeString(str);
        }else {
            hres = create_string(ctx, NULL, 0, &ret);
        }

        if(FAILED(hres))
            return hres;

        V_VT(retv) = VT_DISPATCH;
        V_DISPATCH(retv) = (IDispatch*)_IDispatchEx_(ret);
        break;
    }

    default:
        FIXME("unimplemented flags: %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT string_alloc(script_ctx_t *ctx, DispatchEx *object_prototype, StringInstance **ret)
{
    StringInstance *string;
    HRESULT hres;

    string = heap_alloc_zero(sizeof(StringInstance));
    if(!string)
        return E_OUTOFMEMORY;

    if(object_prototype)
        hres = init_dispex(&string->dispex, ctx, &String_info, object_prototype);
    else
        hres = init_dispex_from_constr(&string->dispex, ctx, &String_info, ctx->string_constr);
    if(FAILED(hres)) {
        heap_free(string);
        return hres;
    }

    *ret = string;
    return S_OK;
}

static const builtin_prop_t StringConstr_props[] = {
    {fromCharCodeW,    StringConstr_fromCharCode,    PROPF_METHOD},
};

static const builtin_info_t StringConstr_info = {
    JSCLASS_FUNCTION,
    {NULL, Function_value, 0},
    sizeof(StringConstr_props)/sizeof(*StringConstr_props),
    StringConstr_props,
    NULL,
    NULL
};

HRESULT create_string_constr(script_ctx_t *ctx, DispatchEx *object_prototype, DispatchEx **ret)
{
    StringInstance *string;
    HRESULT hres;

    static const WCHAR StringW[] = {'S','t','r','i','n','g',0};

    hres = string_alloc(ctx, object_prototype, &string);
    if(FAILED(hres))
        return hres;

    hres = create_builtin_function(ctx, StringConstr_value, StringW, &StringConstr_info,
            PROPF_CONSTR|1, &string->dispex, ret);

    jsdisp_release(&string->dispex);
    return hres;
}

HRESULT create_string(script_ctx_t *ctx, const WCHAR *str, DWORD len, DispatchEx **ret)
{
    StringInstance *string;
    HRESULT hres;

    hres = string_alloc(ctx, NULL, &string);
    if(FAILED(hres))
        return hres;

    if(len == -1)
        len = strlenW(str);

    string->length = len;
    string->str = heap_alloc((len+1)*sizeof(WCHAR));
    if(!string->str) {
        jsdisp_release(&string->dispex);
        return E_OUTOFMEMORY;
    }

    memcpy(string->str, str, len*sizeof(WCHAR));
    string->str[len] = 0;

    *ret = &string->dispex;
    return S_OK;

}
