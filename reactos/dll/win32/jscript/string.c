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
static const WCHAR hasOwnPropertyW[] = {'h','a','s','O','w','n','P','r','o','p','e','r','t','y',0};
static const WCHAR propertyIsEnumerableW[] =
    {'p','r','o','p','e','r','t','y','I','s','E','n','u','m','e','r','a','b','l','e',0};
static const WCHAR isPrototypeOfW[] = {'i','s','P','r','o','t','o','t','y','p','e','O','f',0};

static HRESULT String_length(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    TRACE("%p\n", dispex);

    switch(flags) {
    case DISPATCH_PROPERTYGET: {
        StringInstance *jsthis = (StringInstance*)dispex;

        V_VT(retv) = VT_I4;
        V_I4(retv) = jsthis->length;
        break;
    }
    default:
        FIXME("unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

/* ECMA-262 3rd Edition    15.5.4.2 */
static HRESULT String_toString(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    StringInstance *string;

    TRACE("\n");

    if(!is_class(dispex, JSCLASS_STRING)) {
        WARN("this is not a string object\n");
        return E_FAIL;
    }

    string = (StringInstance*)dispex;

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
static HRESULT String_valueOf(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    TRACE("\n");

    return String_toString(dispex, lcid, flags, dp, retv, ei, sp);
}

static HRESULT do_attributeless_tag_format(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp, const WCHAR *tagname)
{
    static const WCHAR tagfmt[] = {'<','%','s','>','%','s','<','/','%','s','>',0};
    StringInstance *string;
    BSTR ret;

    if(!is_class(dispex, JSCLASS_STRING)) {
        WARN("this is not a string object\n");
        return E_NOTIMPL;
    }

    string = (StringInstance*)dispex;

    if(retv) {
        ret = SysAllocStringLen(NULL, string->length + 2*strlenW(tagname) + 5);
        if(!ret)
            return E_OUTOFMEMORY;

        sprintfW(ret, tagfmt, tagname, string->str, tagname);

        V_VT(retv) = VT_BSTR;
        V_BSTR(retv) = ret;
    }
    return S_OK;
}

static HRESULT String_anchor(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT String_big(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    static const WCHAR bigtagW[] = {'B','I','G',0};
    return do_attributeless_tag_format(dispex, lcid, flags, dp, retv, ei, sp, bigtagW);
}

static HRESULT String_blink(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    static const WCHAR blinktagW[] = {'B','L','I','N','K',0};
    return do_attributeless_tag_format(dispex, lcid, flags, dp, retv, ei, sp, blinktagW);
}

static HRESULT String_bold(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    static const WCHAR boldtagW[] = {'B',0};
    return do_attributeless_tag_format(dispex, lcid, flags, dp, retv, ei, sp, boldtagW);
}

/* ECMA-262 3rd Edition    15.5.4.5 */
static HRESULT String_charAt(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    StringInstance *strobj;
    BSTR str;
    INT pos = 0;
    HRESULT hres;

    TRACE("\n");

    if(dispex->builtin_info->class != JSCLASS_STRING) {
        FIXME("not string this not supported\n");
        return E_NOTIMPL;
    }

    strobj = (StringInstance*)dispex;

    if(arg_cnt(dp)) {
        VARIANT num;

        hres = to_integer(dispex->ctx, get_arg(dp, 0), ei, &num);
        if(FAILED(hres))
            return hres;

        if(V_VT(&num) == VT_I4) {
            pos = V_I4(&num);
        }else {
            WARN("pos = %lf\n", V_R8(&num));
            pos = -1;
        }
    }

    if(!retv)
        return S_OK;

    if(0 <= pos && pos < strobj->length)
        str = SysAllocStringLen(strobj->str+pos, 1);
    else
        str = SysAllocStringLen(NULL, 0);
    if(!str)
        return E_OUTOFMEMORY;

    V_VT(retv) = VT_BSTR;
    V_BSTR(retv) = str;
    return S_OK;
}

/* ECMA-262 3rd Edition    15.5.4.5 */
static HRESULT String_charCodeAt(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    const WCHAR *str;
    DWORD length, idx = 0;
    HRESULT hres;

    TRACE("\n");

    if(dispex->builtin_info->class == JSCLASS_STRING) {
        StringInstance *string = (StringInstance*)dispex;

        str = string->str;
        length = string->length;
    }else {
        FIXME("not string this not supported\n");
        return E_NOTIMPL;
    }

    if(arg_cnt(dp) > 0) {
        VARIANT v;

        hres = to_integer(dispex->ctx, get_arg(dp, 0), ei, &v);
        if(FAILED(hres))
            return hres;

        if(V_VT(&v) != VT_I4 || V_I4(&v) < 0 || V_I4(&v) >= length) {
            if(retv) num_set_nan(&v);
            return S_OK;
        }

        idx = V_I4(&v);
    }

    if(retv) {
        V_VT(retv) = VT_I4;
        V_I4(retv) = str[idx];
    }
    return S_OK;
}

/* ECMA-262 3rd Edition    15.5.4.6 */
static HRESULT String_concat(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
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
    V_DISPATCH(&var) = (IDispatch*)_IDispatchEx_(dispex);

    hres = to_string(dispex->ctx, &var, ei, strs);
    if(SUCCEEDED(hres)) {
        for(i=0; i < arg_cnt(dp); i++) {
            hres = to_string(dispex->ctx, get_arg(dp, i), ei, strs+i+1);
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

static HRESULT String_fixed(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    static const WCHAR fixedtagW[] = {'T','T',0};
    return do_attributeless_tag_format(dispex, lcid, flags, dp, retv, ei, sp, fixedtagW);
}

static HRESULT String_fontcolor(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT String_fontsize(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT String_indexOf(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    DWORD length, pos = 0;
    const WCHAR *str;
    BSTR search_str;
    INT ret = -1;
    HRESULT hres;

    TRACE("\n");

    if(is_class(dispex, JSCLASS_STRING)) {
        StringInstance *string = (StringInstance*)dispex;

        str = string->str;
        length = string->length;
    }else {
        FIXME("not String this\n");
        return E_NOTIMPL;
    }

    if(!arg_cnt(dp)) {
        if(retv) {
            V_VT(retv) = VT_I4;
            V_I4(retv) = -1;
        }
        return S_OK;
    }

    hres = to_string(dispex->ctx, get_arg(dp,0), ei, &search_str);
    if(FAILED(hres))
        return hres;

    if(arg_cnt(dp) >= 2) {
        VARIANT ival;

        hres = to_integer(dispex->ctx, get_arg(dp,1), ei, &ival);
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
    if(FAILED(hres))
        return hres;

    if(retv) {
        V_VT(retv) = VT_I4;
        V_I4(retv) = ret;
    }
    return S_OK;
}

static HRESULT String_italics(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    static const WCHAR italicstagW[] = {'I',0};
    return do_attributeless_tag_format(dispex, lcid, flags, dp, retv, ei, sp, italicstagW);
}

static HRESULT String_lastIndexOf(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT String_link(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

/* ECMA-262 3rd Edition    15.5.4.10 */
static HRESULT String_match(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    StringInstance *This = (StringInstance*)dispex;
    match_result_t *match_result;
    DispatchEx *regexp;
    DispatchEx *array;
    VARIANT var, *arg_var;
    DWORD match_cnt, i;
    HRESULT hres = S_OK;

    TRACE("\n");

    if(arg_cnt(dp) != 1) {
        FIXME("unsupported args\n");
        return E_NOTIMPL;
    }

    arg_var = get_arg(dp, 0);
    switch(V_VT(arg_var)) {
    case VT_DISPATCH:
        regexp = iface_to_jsdisp((IUnknown*)V_DISPATCH(arg_var));
        if(regexp) {
            if(regexp->builtin_info->class == JSCLASS_REGEXP)
                break;
            jsdisp_release(regexp);
        }
    default: {
        BSTR match_str;

        hres = to_string(dispex->ctx, arg_var, ei, &match_str);
        if(FAILED(hres))
            return hres;

        hres = create_regexp_str(dispex->ctx, match_str, SysStringLen(match_str), NULL, 0, &regexp);
        SysFreeString(match_str);
        if(FAILED(hres))
            return hres;
    }
    }

    hres = regexp_match(regexp, This->str, This->length, FALSE, &match_result, &match_cnt);
    jsdisp_release(regexp);
    if(FAILED(hres))
        return hres;

    if(!match_cnt) {
        TRACE("no match\n");

        if(retv)
            V_VT(retv) = VT_NULL;
        return S_OK;
    }

    hres = create_array(dispex->ctx, match_cnt, &array);
    if(FAILED(hres))
        return hres;

    V_VT(&var) = VT_BSTR;

    for(i=0; i < match_cnt; i++) {
        V_BSTR(&var) = SysAllocStringLen(match_result[i].str, match_result[i].len);
        if(!V_BSTR(&var)) {
            hres = E_OUTOFMEMORY;
            break;
        }

        hres = jsdisp_propput_idx(array, i, lcid, &var, ei, NULL/*FIXME*/);
        SysFreeString(V_BSTR(&var));
        if(FAILED(hres))
            break;
    }

    if(SUCCEEDED(hres) && retv) {
        V_VT(retv) = VT_DISPATCH;
        V_DISPATCH(retv) = (IDispatch*)_IDispatchEx_(array);
    }else {
        jsdisp_release(array);
    }
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

static HRESULT rep_call(DispatchEx *func, const WCHAR *str, match_result_t *match, match_result_t *parens,
        DWORD parens_cnt, LCID lcid, BSTR *ret, jsexcept_t *ei, IServiceProvider *caller)
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
        hres = jsdisp_call_value(func, lcid, DISPATCH_METHOD, &dp, &var, ei, caller);

    for(i=0; i < parens_cnt+1; i++) {
        if(i != parens_cnt+1)
            SysFreeString(V_BSTR(get_arg(&dp,i)));
    }
    heap_free(args);

    if(FAILED(hres))
        return hres;

    hres = to_string(func->ctx, &var, ei, ret);
    VariantClear(&var);
    return hres;
}

/* ECMA-262 3rd Edition    15.5.4.11 */
static HRESULT String_replace(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    DWORD parens_cnt = 0, parens_size=0, rep_len=0, length;
    BSTR rep_str = NULL, match_str = NULL, ret_str;
    DispatchEx *rep_func = NULL, *regexp = NULL;
    match_result_t *parens = NULL, match;
    const WCHAR *str;
    strbuf_t ret = {NULL,0,0};
    BOOL gcheck = FALSE;
    VARIANT *arg_var;
    HRESULT hres = S_OK;

    TRACE("\n");

    if(is_class(dispex, JSCLASS_STRING)) {
        StringInstance *string = (StringInstance*)dispex;
        str = string->str;
        length = string->length;
    }else {
        FIXME("not String this\n");
        return E_NOTIMPL;
    }

    if(!arg_cnt(dp)) {
        if(retv) {
            ret_str = SysAllocString(str);
            if(!ret_str)
                return E_OUTOFMEMORY;

            V_VT(retv) = VT_BSTR;
            V_BSTR(retv) = ret_str;
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
        hres = to_string(dispex->ctx, arg_var, ei, &match_str);
        if(FAILED(hres))
            return hres;
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
            hres = to_string(dispex->ctx, arg_var, ei, &rep_str);
            if(FAILED(hres))
                break;

            if(strchrW(rep_str, '$')) {
                FIXME("unsupported $ in replace string\n");
                hres = E_NOTIMPL;
            }

            rep_len = SysStringLen(rep_str);
        }
    }

    if(SUCCEEDED(hres)) {
        const WCHAR *cp, *ecp;

        cp = ecp = str;

        while(1) {
            if(regexp) {
                hres = regexp_match_next(regexp, gcheck, str, length, &cp, rep_func ? &parens : NULL,
                                         &parens_size, &parens_cnt, &match);
                gcheck = TRUE;

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

                hres = rep_call(rep_func, str, &match, parens, parens_cnt, lcid, &cstr, ei, caller);
                if(FAILED(hres))
                    break;

                hres = strbuf_append(&ret, cstr, SysStringLen(cstr));
                SysFreeString(cstr);
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

static HRESULT String_search(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

/* ECMA-262 3rd Edition    15.5.4.13 */
static HRESULT String_slice(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    const WCHAR *str;
    DWORD length;
    INT start=0, end;
    VARIANT v;
    HRESULT hres;

    TRACE("\n");

    if(is_class(dispex, JSCLASS_STRING)) {
        StringInstance *string = (StringInstance*)dispex;

        str = string->str;
        length = string->length;
    }else {
        FIXME("this is not a string class\n");
        return E_NOTIMPL;
    }

    if(arg_cnt(dp)) {
        hres = to_integer(dispex->ctx, dp->rgvarg + dp->cArgs-1, ei, &v);
        if(FAILED(hres))
            return hres;

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
        hres = to_integer(dispex->ctx, dp->rgvarg + dp->cArgs-2, ei, &v);
        if(FAILED(hres))
            return hres;

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
        if(!str)
            return E_OUTOFMEMORY;

        V_VT(retv) = VT_BSTR;
        V_BSTR(retv) = retstr;
    }
    return S_OK;
}

static HRESULT String_small(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    static const WCHAR smalltagW[] = {'S','M','A','L','L',0};
    return do_attributeless_tag_format(dispex, lcid, flags, dp, retv, ei, sp, smalltagW);
}

static HRESULT String_split(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    match_result_t *match_result = NULL;
    DWORD match_cnt, i, match_len = 0;
    StringInstance *string;
    const WCHAR *ptr, *ptr2;
    VARIANT *arg, var;
    DispatchEx *array;
    BSTR match_str = NULL;
    HRESULT hres;

    TRACE("\n");

    if(!is_class(dispex, JSCLASS_STRING)) {
        FIXME("not String this\n");
        return E_NOTIMPL;
    }

    string = (StringInstance*)dispex;

    if(arg_cnt(dp) != 1) {
        FIXME("unsupported args\n");
        return E_NOTIMPL;
    }

    arg = get_arg(dp, 0);
    switch(V_VT(arg)) {
    case VT_DISPATCH: {
        DispatchEx *regexp;

        regexp = iface_to_jsdisp((IUnknown*)V_DISPATCH(arg));
        if(regexp) {
            if(is_class(regexp, JSCLASS_REGEXP)) {
                hres = regexp_match(regexp, string->str, string->length, TRUE, &match_result, &match_cnt);
                jsdisp_release(regexp);
                if(FAILED(hres))
                    return hres;
                break;
            }
            jsdisp_release(regexp);
        }
    }
    default:
        hres = to_string(dispex->ctx, arg, ei, &match_str);
        if(FAILED(hres))
            return hres;

        match_len = SysStringLen(match_str);
        if(!match_len) {
            SysFreeString(match_str);
            match_str = NULL;
        }
    }

    hres = create_array(dispex->ctx, 0, &array);

    if(SUCCEEDED(hres)) {
        ptr = string->str;
        for(i=0;; i++) {
            if(match_result) {
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

            hres = jsdisp_propput_idx(array, i, lcid, &var, ei, sp);
            SysFreeString(V_BSTR(&var));
            if(FAILED(hres))
                break;

            if(match_result)
                ptr = match_result[i].str + match_result[i].len;
            else if(match_str)
                ptr = ptr2 + match_len;
            else
                ptr++;
        }
    }

    if(SUCCEEDED(hres) && (match_str || match_result)) {
        DWORD len = (string->str+string->length) - ptr;

        if(len || match_str) {
            V_VT(&var) = VT_BSTR;
            V_BSTR(&var) = SysAllocStringLen(ptr, len);

            if(V_BSTR(&var)) {
                hres = jsdisp_propput_idx(array, i, lcid, &var, ei, sp);
                SysFreeString(V_BSTR(&var));
            }else {
                hres = E_OUTOFMEMORY;
            }
        }
    }

    SysFreeString(match_str);
    heap_free(match_result);

    if(SUCCEEDED(hres) && retv) {
        V_VT(retv) = VT_DISPATCH;
        V_DISPATCH(retv) = (IDispatch*)_IDispatchEx_(array);
    }else {
        jsdisp_release(array);
    }

    return hres;
}

static HRESULT String_strike(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    static const WCHAR striketagW[] = {'S','T','R','I','K','E',0};
    return do_attributeless_tag_format(dispex, lcid, flags, dp, retv, ei, sp, striketagW);
}

static HRESULT String_sub(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    static const WCHAR subtagW[] = {'S','U','B',0};
    return do_attributeless_tag_format(dispex, lcid, flags, dp, retv, ei, sp, subtagW);
}

/* ECMA-262 3rd Edition    15.5.4.15 */
static HRESULT String_substring(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    const WCHAR *str;
    INT start=0, end;
    DWORD length;
    VARIANT v;
    HRESULT hres;

    TRACE("\n");

    if(is_class(dispex, JSCLASS_STRING)) {
        StringInstance *string = (StringInstance*)dispex;

        length = string->length;
        str = string->str;
    }else {
        FIXME("not string this not supported\n");
        return E_NOTIMPL;
    }

    if(arg_cnt(dp) >= 1) {
        hres = to_integer(dispex->ctx, dp->rgvarg + dp->cArgs-1, ei, &v);
        if(FAILED(hres))
            return hres;

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
        hres = to_integer(dispex->ctx, dp->rgvarg + dp->cArgs-2, ei, &v);
        if(FAILED(hres))
            return hres;

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
        if(!V_BSTR(retv))
            return E_OUTOFMEMORY;
    }
    return S_OK;
}

static HRESULT String_substr(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT String_sup(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    static const WCHAR suptagW[] = {'S','U','P',0};
    return do_attributeless_tag_format(dispex, lcid, flags, dp, retv, ei, sp, suptagW);
}

static HRESULT String_toLowerCase(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    StringInstance *string;
    const WCHAR* str;
    DWORD length;
    BSTR bstr;

    TRACE("\n");

    if(is_class(dispex, JSCLASS_STRING)) {
        string = (StringInstance*)dispex;

        length = string->length;
        str = string->str;
    }else {
        FIXME("not string this not supported\n");
        return E_NOTIMPL;
    }

    if(retv) {
        bstr = SysAllocStringLen(str, length);
        if (!bstr)
            return E_OUTOFMEMORY;

        strlwrW(bstr);

        V_VT(retv) = VT_BSTR;
        V_BSTR(retv) = bstr;
    }
    return S_OK;
}

static HRESULT String_toUpperCase(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    StringInstance *string;
    const WCHAR* str;
    DWORD length;
    BSTR bstr;

    TRACE("\n");

    if(is_class(dispex, JSCLASS_STRING)) {
        string = (StringInstance*)dispex;

        length = string->length;
        str = string->str;
    }else {
        FIXME("not string this not supported\n");
        return E_NOTIMPL;
    }

    if(retv) {
        bstr = SysAllocStringLen(str, length);
        if (!bstr)
            return E_OUTOFMEMORY;

        struprW(bstr);

        V_VT(retv) = VT_BSTR;
        V_BSTR(retv) = bstr;
    }
    return S_OK;
}

static HRESULT String_toLocaleLowerCase(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT String_toLocaleUpperCase(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT String_localeCompare(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT String_hasOwnProperty(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT String_propertyIsEnumerable(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT String_isPrototypeOf(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT String_value(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    StringInstance *This = (StringInstance*)dispex;

    TRACE("\n");

    switch(flags) {
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
    {anchorW,                String_anchor,                PROPF_METHOD},
    {bigW,                   String_big,                   PROPF_METHOD},
    {blinkW,                 String_blink,                 PROPF_METHOD},
    {boldW,                  String_bold,                  PROPF_METHOD},
    {charAtW,                String_charAt,                PROPF_METHOD},
    {charCodeAtW,            String_charCodeAt,            PROPF_METHOD},
    {concatW,                String_concat,                PROPF_METHOD},
    {fixedW,                 String_fixed,                 PROPF_METHOD},
    {fontcolorW,             String_fontcolor,             PROPF_METHOD},
    {fontsizeW,              String_fontsize,              PROPF_METHOD},
    {hasOwnPropertyW,        String_hasOwnProperty,        PROPF_METHOD},
    {indexOfW,               String_indexOf,               PROPF_METHOD},
    {isPrototypeOfW,         String_isPrototypeOf,         PROPF_METHOD},
    {italicsW,               String_italics,               PROPF_METHOD},
    {lastIndexOfW,           String_lastIndexOf,           PROPF_METHOD},
    {lengthW,                String_length,                0},
    {linkW,                  String_link,                  PROPF_METHOD},
    {localeCompareW,         String_localeCompare,         PROPF_METHOD},
    {matchW,                 String_match,                 PROPF_METHOD},
    {propertyIsEnumerableW,  String_propertyIsEnumerable,  PROPF_METHOD},
    {replaceW,               String_replace,               PROPF_METHOD},
    {searchW,                String_search,                PROPF_METHOD},
    {sliceW,                 String_slice,                 PROPF_METHOD},
    {smallW,                 String_small,                 PROPF_METHOD},
    {splitW,                 String_split,                 PROPF_METHOD},
    {strikeW,                String_strike,                PROPF_METHOD},
    {subW,                   String_sub,                   PROPF_METHOD},
    {substrW,                String_substr,                PROPF_METHOD},
    {substringW,             String_substring,             PROPF_METHOD},
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

static HRESULT StringConstr_value(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    HRESULT hres;

    TRACE("\n");

    switch(flags) {
    case INVOKE_FUNC: {
        BSTR str;

        if(arg_cnt(dp)) {
            hres = to_string(dispex->ctx, get_arg(dp, 0), ei, &str);
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

            hres = to_string(dispex->ctx, get_arg(dp, 0), ei, &str);
            if(FAILED(hres))
                return hres;

            hres = create_string(dispex->ctx, str, SysStringLen(str), &ret);
            SysFreeString(str);
        }else {
            hres = create_string(dispex->ctx, NULL, 0, &ret);
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

static HRESULT string_alloc(script_ctx_t *ctx, BOOL use_constr, StringInstance **ret)
{
    StringInstance *string;
    HRESULT hres;

    string = heap_alloc_zero(sizeof(StringInstance));
    if(!string)
        return E_OUTOFMEMORY;

    if(use_constr)
        hres = init_dispex_from_constr(&string->dispex, ctx, &String_info, ctx->string_constr);
    else
        hres = init_dispex(&string->dispex, ctx, &String_info, NULL);
    if(FAILED(hres)) {
        heap_free(string);
        return hres;
    }

    *ret = string;
    return S_OK;
}

HRESULT create_string_constr(script_ctx_t *ctx, DispatchEx **ret)
{
    StringInstance *string;
    HRESULT hres;

    hres = string_alloc(ctx, FALSE, &string);
    if(FAILED(hres))
        return hres;

    hres = create_builtin_function(ctx, StringConstr_value, PROPF_CONSTR, &string->dispex, ret);

    jsdisp_release(&string->dispex);
    return hres;
}

HRESULT create_string(script_ctx_t *ctx, const WCHAR *str, DWORD len, DispatchEx **ret)
{
    StringInstance *string;
    HRESULT hres;

    hres = string_alloc(ctx, TRUE, &string);
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
