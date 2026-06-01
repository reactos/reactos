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

#ifdef __REACTOS__
#include <wine/config.h>
#include <wine/port.h>
#endif

#include <math.h>
#include <limits.h>

#include "jscript.h"
#include "engine.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

static int uri_char_table[] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 00-0f */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 10-1f */
    0,2,0,0,1,0,1,2,2,2,2,1,1,2,2,1, /* 20-2f */
    2,2,2,2,2,2,2,2,2,2,1,1,0,1,0,1, /* 30-3f */
    1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, /* 40-4f */
    2,2,2,2,2,2,2,2,2,2,2,0,0,0,0,2, /* 50-5f */
    0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, /* 60-6f */
    2,2,2,2,2,2,2,2,2,2,2,0,0,0,2,0, /* 70-7f */
};

/* 1 - reserved */
/* 2 - unescaped */

static inline BOOL is_uri_reserved(WCHAR c)
{
    return c < 128 && uri_char_table[c] == 1;
}

static inline BOOL is_uri_unescaped(WCHAR c)
{
    return c < 128 && uri_char_table[c] == 2;
}

/* Check that the character is one of the 69 non-blank characters as defined by ECMA-262 B.2.1 */
static inline BOOL is_ecma_nonblank(const WCHAR c)
{
    return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
        c == '@' || c == '*' || c == '_' || c == '+' || c == '-' || c == '.' || c == '/');
}

static WCHAR int_to_char(int i)
{
    if(i < 10)
        return '0'+i;
    return 'A'+i-10;
}

static HRESULT JSGlobal_escape(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsstr_t *ret_str, *str;
    const WCHAR *ptr, *buf;
    DWORD len = 0;
    WCHAR *ret;
    HRESULT hres;

    TRACE("\n");

    if(!argc) {
        if(r)
            *r = jsval_string(jsstr_undefined());
        return S_OK;
    }

    hres = to_flat_string(ctx, argv[0], &str, &buf);
    if(FAILED(hres))
        return hres;

    for(ptr = buf; *ptr; ptr++) {
        if(*ptr > 0xff)
            len += 6;
        else if(is_ecma_nonblank(*ptr))
            len++;
        else
            len += 3;
    }

    ret_str = jsstr_alloc_buf(len, &ret);
    if(!ret_str) {
        jsstr_release(str);
        return E_OUTOFMEMORY;
    }

    len = 0;
    for(ptr = buf; *ptr; ptr++) {
        if(*ptr > 0xff) {
            ret[len++] = '%';
            ret[len++] = 'u';
            ret[len++] = int_to_char(*ptr >> 12);
            ret[len++] = int_to_char((*ptr >> 8) & 0xf);
            ret[len++] = int_to_char((*ptr >> 4) & 0xf);
            ret[len++] = int_to_char(*ptr & 0xf);
        }
        else if(is_ecma_nonblank(*ptr))
            ret[len++] = *ptr;
        else {
            ret[len++] = '%';
            ret[len++] = int_to_char(*ptr >> 4);
            ret[len++] = int_to_char(*ptr & 0xf);
        }
    }

    jsstr_release(str);

    if(r)
        *r = jsval_string(ret_str);
    else
        jsstr_release(ret_str);
    return S_OK;
}

/* ECMA-262 3rd Edition    15.1.2.1 */
HRESULT builtin_eval(script_ctx_t *ctx, call_frame_t *frame, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    DWORD exec_flags = EXEC_EVAL;
    bytecode_t *code;
    const WCHAR *src;
    HRESULT hres;

    TRACE("\n");

    if(!argc) {
        if(r)
            *r = jsval_undefined();
        return S_OK;
    }

    if(!is_string(argv[0])) {
        if(r)
            return jsval_copy(argv[0], r);
        return S_OK;
    }

    src = jsstr_flatten(get_string(argv[0]));
    if(!src)
        return E_OUTOFMEMORY;

    TRACE("parsing %s\n", debugstr_jsval(argv[0]));
    hres = compile_script(ctx, src, 0, 0, NULL, NULL, TRUE, FALSE, frame ? frame->bytecode->named_item : NULL, &code);
    if(FAILED(hres)) {
        WARN("parse (%s) failed: %08lx\n", debugstr_jsval(argv[0]), hres);
        return hres;
    }

    if(!frame || (frame->flags & EXEC_GLOBAL))
        exec_flags |= EXEC_GLOBAL;
    if(flags & DISPATCH_JSCRIPT_CALLEREXECSSOURCE)
        exec_flags |= EXEC_RETURN_TO_INTERP;
    hres = exec_source(ctx, exec_flags, code, &code->global_code, frame ? frame->scope : NULL,
            frame ? frame->this_obj : NULL, NULL, 0, NULL, r);
    release_bytecode(code);
    return hres;
}

HRESULT JSGlobal_eval(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    return builtin_eval(ctx, ctx->version < SCRIPTLANGUAGEVERSION_ES5 ? ctx->call_ctx : NULL, flags, argc, argv, r);
}

static HRESULT JSGlobal_isNaN(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    BOOL ret = TRUE;
    double n;
    HRESULT hres;

    TRACE("\n");

    if(argc) {
        hres = to_number(ctx, argv[0], &n);
        if(FAILED(hres))
            return hres;

        if(!isnan(n))
            ret = FALSE;
    }

    if(r)
        *r = jsval_bool(ret);
    return S_OK;
}

static HRESULT JSGlobal_isFinite(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    BOOL ret = FALSE;
    HRESULT hres;

    TRACE("\n");

    if(argc) {
        double n;

        hres = to_number(ctx, argv[0], &n);
        if(FAILED(hres))
            return hres;

        ret = isfinite(n);
    }

    if(r)
        *r = jsval_bool(ret);
    return S_OK;
}

static INT char_to_int(WCHAR c)
{
    if('0' <= c && c <= '9')
        return c - '0';
    if('a' <= c && c <= 'z')
        return c - 'a' + 10;
    if('A' <= c && c <= 'Z')
        return c - 'A' + 10;
    return 100;
}

static HRESULT JSGlobal_parseInt(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    BOOL neg = FALSE, empty = TRUE;
    const WCHAR *ptr;
    DOUBLE ret = 0.0;
    INT radix=0, i;
    jsstr_t *str;
    HRESULT hres;

    if(!argc) {
        if(r)
            *r = jsval_number(NAN);
        return S_OK;
    }

    if(argc >= 2) {
        hres = to_int32(ctx, argv[1], &radix);
        if(FAILED(hres))
            return hres;

        if(radix && (radix < 2 || radix > 36)) {
            WARN("radix %d out of range\n", radix);
            if(r)
                *r = jsval_number(NAN);
            return S_OK;
        }
    }

    hres = to_flat_string(ctx, argv[0], &str, &ptr);
    if(FAILED(hres))
        return hres;

    while(iswspace(*ptr))
        ptr++;

    switch(*ptr) {
    case '+':
        ptr++;
        break;
    case '-':
        neg = TRUE;
        ptr++;
        break;
    }

    if(!radix) {
        if(*ptr == '0') {
            if(ptr[1] == 'x' || ptr[1] == 'X') {
                radix = 16;
                ptr += 2;
            }else {
                radix = 8;
                ptr++;
                empty = FALSE;
            }
        }else {
            radix = 10;
        }
    }else if(radix == 16 && *ptr == '0' && (ptr[1] == 'x' || ptr[1] == 'X')) {
        ptr += 2;
    }

    i = char_to_int(*ptr++);
    if(i < radix) {
        do {
            ret = ret*radix + i;
            i = char_to_int(*ptr++);
        }while(i < radix);
    }else if(empty) {
        ret = NAN;
    }

    jsstr_release(str);

    if(neg)
        ret = -ret;

    if(r)
        *r = jsval_number(ret);
    return S_OK;
}

static HRESULT JSGlobal_parseFloat(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    LONGLONG d = 0, hlp;
    jsstr_t *val_str;
    int exp = 0;
    const WCHAR *str;
    BOOL ret_nan = TRUE, positive = TRUE;
    HRESULT hres;

    if(!argc) {
        if(r)
            *r = jsval_number(NAN);
        return S_OK;
    }

    TRACE("%s\n", debugstr_jsval(argv[0]));

    hres = to_flat_string(ctx, argv[0], &val_str, &str);
    if(FAILED(hres))
        return hres;

    while(iswspace(*str)) str++;

    if(*str == '+')
        str++;
    else if(*str == '-') {
        positive = FALSE;
        str++;
    }

    if(is_digit(*str))
        ret_nan = FALSE;

    while(is_digit(*str)) {
        hlp = d*10 + *(str++) - '0';
        if(d>MAXLONGLONG/10 || hlp<0) {
            exp++;
            break;
        }
        else
            d = hlp;
    }
    while(is_digit(*str)) {
        exp++;
        str++;
    }

    if(*str == '.') str++;

    if(is_digit(*str))
        ret_nan = FALSE;

    while(is_digit(*str)) {
        hlp = d*10 + *(str++) - '0';
        if(d>MAXLONGLONG/10 || hlp<0)
            break;

        d = hlp;
        exp--;
    }
    while(is_digit(*str))
        str++;

    if(*str && !ret_nan && (*str=='e' || *str=='E')) {
        int sign = 1, e = 0;

        str++;
        if(*str == '+')
            str++;
        else if(*str == '-') {
            sign = -1;
            str++;
        }

        while(is_digit(*str)) {
            if(e>INT_MAX/10 || (e = e*10 + *str++ - '0')<0)
                e = INT_MAX;
        }
        e *= sign;

        if(exp<0 && e<0 && exp+e>0) exp = INT_MIN;
        else if(exp>0 && e>0 && exp+e<0) exp = INT_MAX;
        else exp += e;
    }

    jsstr_release(val_str);

    if(ret_nan) {
        if(r)
            *r = jsval_number(NAN);
        return S_OK;
    }

    if(!positive)
        d = -d;
    if(r)
        *r = jsval_number(exp>0 ? d*pow(10, exp) : d/pow(10, -exp));
    return S_OK;
}

#if defined(__REACTOS__) && defined(__clang__)
static inline __attribute__((always_inline)) int hex_to_int(WCHAR wch) {
#else
static inline int hex_to_int(const WCHAR wch) {
#endif
    if(towupper(wch)>='A' && towupper(wch)<='F') return towupper(wch)-'A'+10;
    if(is_digit(wch)) return wch-'0';
    return -1;
}

static HRESULT JSGlobal_unescape(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsstr_t *ret_str, *str;
    const WCHAR *ptr, *buf;
    DWORD len = 0;
    WCHAR *ret;
    HRESULT hres;

    TRACE("\n");

    if(!argc) {
        if(r)
            *r = jsval_string(jsstr_undefined());
        return S_OK;
    }

    hres = to_flat_string(ctx, argv[0], &str, &buf);
    if(FAILED(hres))
        return hres;

    for(ptr = buf; *ptr; ptr++) {
        if(*ptr == '%') {
            if(hex_to_int(*(ptr+1))!=-1 && hex_to_int(*(ptr+2))!=-1)
                ptr += 2;
            else if(*(ptr+1)=='u' && hex_to_int(*(ptr+2))!=-1 && hex_to_int(*(ptr+3))!=-1
                    && hex_to_int(*(ptr+4))!=-1 && hex_to_int(*(ptr+5))!=-1)
                ptr += 5;
        }

        len++;
    }

    ret_str = jsstr_alloc_buf(len, &ret);
    if(!ret_str) {
        jsstr_release(str);
        return E_OUTOFMEMORY;
    }

    len = 0;
    for(ptr = buf; *ptr; ptr++) {
        if(*ptr == '%') {
            if(hex_to_int(*(ptr+1))!=-1 && hex_to_int(*(ptr+2))!=-1) {
                ret[len] = (hex_to_int(*(ptr+1))<<4) + hex_to_int(*(ptr+2));
                ptr += 2;
            }
            else if(*(ptr+1)=='u' && hex_to_int(*(ptr+2))!=-1 && hex_to_int(*(ptr+3))!=-1
                    && hex_to_int(*(ptr+4))!=-1 && hex_to_int(*(ptr+5))!=-1) {
                ret[len] = (hex_to_int(*(ptr+2))<<12) + (hex_to_int(*(ptr+3))<<8)
                    + (hex_to_int(*(ptr+4))<<4) + hex_to_int(*(ptr+5));
                ptr += 5;
            }
            else
                ret[len] = *ptr;
        }
        else
            ret[len] = *ptr;

        len++;
    }

    jsstr_release(str);

    if(r)
        *r = jsval_string(ret_str);
    else
        jsstr_release(ret_str);
    return S_OK;
}

static HRESULT JSGlobal_GetObject(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT JSGlobal_ScriptEngine(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    TRACE("\n");

    if(r) {
        jsstr_t *ret;

        ret = jsstr_alloc(L"JScript");
        if(!ret)
            return E_OUTOFMEMORY;

        *r = jsval_string(ret);
    }

    return S_OK;
}

static HRESULT JSGlobal_ScriptEngineMajorVersion(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    TRACE("\n");

    if(r)
        *r = jsval_number(JSCRIPT_MAJOR_VERSION);
    return S_OK;
}

static HRESULT JSGlobal_ScriptEngineMinorVersion(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    TRACE("\n");

    if(r)
        *r = jsval_number(JSCRIPT_MINOR_VERSION);
    return S_OK;
}

static HRESULT JSGlobal_ScriptEngineBuildVersion(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    TRACE("\n");

    if(r)
        *r = jsval_number(JSCRIPT_BUILD_VERSION);
    return S_OK;
}

static HRESULT JSGlobal_CollectGarbage(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    return gc_run(ctx);
}

static HRESULT JSGlobal_encodeURI(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    const WCHAR *ptr, *uri;
    jsstr_t *str, *ret;
    DWORD len = 0, i;
    char buf[4];
    WCHAR *rptr;
    HRESULT hres;

    TRACE("\n");

    if(!argc) {
        if(r)
            *r = jsval_string(jsstr_undefined());
        return S_OK;
    }

    hres = to_flat_string(ctx, argv[0], &str, &uri);
    if(FAILED(hres))
        return hres;

    for(ptr = uri; *ptr; ptr++) {
        if(is_uri_unescaped(*ptr) || is_uri_reserved(*ptr) || *ptr == '#') {
            len++;
        }else {
            i = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, ptr, 1, NULL, 0, NULL, NULL)*3;
            if(!i) {
                jsstr_release(str);
                return JS_E_INVALID_URI_CHAR;
            }

            len += i;
        }
    }

    ret = jsstr_alloc_buf(len, &rptr);
    if(!ret) {
        jsstr_release(str);
        return E_OUTOFMEMORY;
    }

    for(ptr = uri; *ptr; ptr++) {
        if(is_uri_unescaped(*ptr) || is_uri_reserved(*ptr) || *ptr == '#') {
            *rptr++ = *ptr;
        }else {
            len = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, ptr, 1, buf, sizeof(buf), NULL, NULL);
            for(i=0; i<len; i++) {
                *rptr++ = '%';
                *rptr++ = int_to_char((BYTE)buf[i] >> 4);
                *rptr++ = int_to_char(buf[i] & 0x0f);
            }
        }
    }

    TRACE("%s -> %s\n", debugstr_jsstr(str), debugstr_jsstr(ret));
    jsstr_release(str);

    if(r)
        *r = jsval_string(ret);
    else
        jsstr_release(ret);
    return S_OK;
}

static HRESULT JSGlobal_decodeURI(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    const WCHAR *ptr, *uri;
    jsstr_t *str, *ret_str;
    unsigned len = 0;
    int i, val, res;
    WCHAR *ret;
    char buf[4];
    WCHAR out;
    HRESULT hres;

    TRACE("\n");

    if(!argc) {
        if(r)
            *r = jsval_string(jsstr_undefined());
        return S_OK;
    }

    hres = to_flat_string(ctx, argv[0], &str, &uri);
    if(FAILED(hres))
        return hres;

    for(ptr = uri; *ptr; ptr++) {
        if(*ptr != '%') {
            len++;
        }else {
            res = 0;
            for(i=0; i<4; i++) {
                if(ptr[i*3]!='%' || hex_to_int(ptr[i*3+1])==-1 || (val=hex_to_int(ptr[i*3+2]))==-1)
                    break;
                val += hex_to_int(ptr[i*3+1])<<4;
                buf[i] = val;

                res = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, buf, i+1, &out, 1);
                if(res)
                    break;
            }

            if(!res) {
                jsstr_release(str);
                return JS_E_INVALID_URI_CODING;
            }

            ptr += i*3+2;
            len++;
        }
    }

    ret_str = jsstr_alloc_buf(len, &ret);
    if(!ret_str) {
        jsstr_release(str);
        return E_OUTOFMEMORY;
    }

    for(ptr = uri; *ptr; ptr++) {
        if(*ptr != '%') {
            *ret++ = *ptr;
        }else {
            for(i=0; i<4; i++) {
                if(ptr[i*3]!='%' || hex_to_int(ptr[i*3+1])==-1 || (val=hex_to_int(ptr[i*3+2]))==-1)
                    break;
                val += hex_to_int(ptr[i*3+1])<<4;
                buf[i] = val;

                res = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, buf, i+1, ret, 1);
                if(res)
                    break;
            }

            ptr += i*3+2;
            ret++;
        }
    }

    TRACE("%s -> %s\n", debugstr_jsstr(str), debugstr_jsstr(ret_str));
    jsstr_release(str);

    if(r)
        *r = jsval_string(ret_str);
    else
        jsstr_release(ret_str);
    return S_OK;
}

static HRESULT JSGlobal_encodeURIComponent(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsstr_t *str, *ret_str;
    char buf[4];
    const WCHAR *ptr, *uri;
    DWORD len = 0, size, i;
    WCHAR *ret;
    HRESULT hres;

    TRACE("\n");

    if(!argc) {
        if(r)
            *r = jsval_string(jsstr_undefined());
        return S_OK;
    }

    hres = to_flat_string(ctx, argv[0], &str, &uri);
    if(FAILED(hres))
        return hres;

    for(ptr = uri; *ptr; ptr++) {
        if(is_uri_unescaped(*ptr))
            len++;
        else {
            size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, ptr, 1, NULL, 0, NULL, NULL);
            if(!size) {
                jsstr_release(str);
                return JS_E_INVALID_URI_CHAR;
            }
            len += size*3;
        }
    }

    ret_str = jsstr_alloc_buf(len, &ret);
    if(!ret_str) {
        jsstr_release(str);
        return E_OUTOFMEMORY;
    }

    for(ptr = uri; *ptr; ptr++) {
        if(is_uri_unescaped(*ptr)) {
            *ret++ = *ptr;
        }else {
            size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, ptr, 1, buf, sizeof(buf), NULL, NULL);
            for(i=0; i<size; i++) {
                *ret++ = '%';
                *ret++ = int_to_char((BYTE)buf[i] >> 4);
                *ret++ = int_to_char(buf[i] & 0x0f);
            }
        }
    }

    jsstr_release(str);

    if(r)
        *r = jsval_string(ret_str);
    else
        jsstr_release(ret_str);
    return S_OK;
}

/* ECMA-262 3rd Edition    15.1.3.2 */
static HRESULT JSGlobal_decodeURIComponent(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    const WCHAR *ptr, *uri;
    jsstr_t *str, *ret;
    WCHAR *out_ptr;
    DWORD len = 0;
    HRESULT hres;

    TRACE("\n");

    if(!argc) {
        if(r)
            *r = jsval_string(jsstr_undefined());
        return S_OK;
    }

    hres = to_flat_string(ctx, argv[0], &str, &uri);
    if(FAILED(hres))
        return hres;

    ptr = uri;
    while(*ptr) {
        if(*ptr == '%') {
            char octets[4];
            unsigned char mask = 0x80;
            int i, size, num_bytes = 0;
            if(hex_to_int(*(ptr+1)) < 0 || hex_to_int(*(ptr+2)) < 0) {
                FIXME("Throw URIError: Invalid hex sequence\n");
                jsstr_release(str);
                return E_FAIL;
            }
            octets[0] = (hex_to_int(*(ptr+1)) << 4) + hex_to_int(*(ptr+2));
            ptr += 3;
            while(octets[0] & mask) {
                mask = mask >> 1;
                ++num_bytes;
            }
            if(num_bytes == 1 || num_bytes > 4) {
                FIXME("Throw URIError: Invalid initial UTF character\n");
                jsstr_release(str);
                return E_FAIL;
            }
            for(i = 1; i < num_bytes; ++i) {
                if(*ptr != '%'){
                    FIXME("Throw URIError: Incomplete UTF sequence\n");
                    jsstr_release(str);
                    return E_FAIL;
                }
                if(hex_to_int(*(ptr+1)) < 0 || hex_to_int(*(ptr+2)) < 0) {
                    FIXME("Throw URIError: Invalid hex sequence\n");
                    jsstr_release(str);
                    return E_FAIL;
                }
                octets[i] = (hex_to_int(*(ptr+1)) << 4) + hex_to_int(*(ptr+2));
                ptr += 3;
            }
            size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, octets,
                    num_bytes ? num_bytes : 1, NULL, 0);
            if(size == 0) {
                FIXME("Throw URIError: Invalid UTF sequence\n");
                jsstr_release(str);
                return E_FAIL;
            }
            len += size;
        }else {
            ++ptr;
            ++len;
        }
    }

    ret = jsstr_alloc_buf(len, &out_ptr);
    if(!ret) {
        jsstr_release(str);
        return E_OUTOFMEMORY;
    }

    ptr = uri;
    while(*ptr) {
        if(*ptr == '%') {
            char octets[4];
            unsigned char mask = 0x80;
            int i, size, num_bytes = 0;
            octets[0] = (hex_to_int(*(ptr+1)) << 4) + hex_to_int(*(ptr+2));
            ptr += 3;
            while(octets[0] & mask) {
                mask = mask >> 1;
                ++num_bytes;
            }
            for(i = 1; i < num_bytes; ++i) {
                octets[i] = (hex_to_int(*(ptr+1)) << 4) + hex_to_int(*(ptr+2));
                ptr += 3;
            }
            size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, octets,
                    num_bytes ? num_bytes : 1, out_ptr, len);
            len -= size;
            out_ptr += size;
        }else {
            *out_ptr++ = *ptr++;
            --len;
        }
    }

    jsstr_release(str);

    if(r)
        *r = jsval_string(ret);
    else
        jsstr_release(ret);
    return S_OK;
}

static const builtin_prop_t JSGlobal_props[] = {
    {L"CollectGarbage",            JSGlobal_CollectGarbage,            PROPF_METHOD},
    {L"GetObject",                 JSGlobal_GetObject,                 PROPF_METHOD|2},
    {L"ScriptEngine",              JSGlobal_ScriptEngine,              PROPF_METHOD},
    {L"ScriptEngineBuildVersion",  JSGlobal_ScriptEngineBuildVersion,  PROPF_METHOD},
    {L"ScriptEngineMajorVersion",  JSGlobal_ScriptEngineMajorVersion,  PROPF_METHOD},
    {L"ScriptEngineMinorVersion",  JSGlobal_ScriptEngineMinorVersion,  PROPF_METHOD},
    {L"decodeURI",                 JSGlobal_decodeURI,                 PROPF_METHOD|1},
    {L"decodeURIComponent",        JSGlobal_decodeURIComponent,        PROPF_METHOD|1},
    {L"encodeURI",                 JSGlobal_encodeURI,                 PROPF_METHOD|1},
    {L"encodeURIComponent",        JSGlobal_encodeURIComponent,        PROPF_METHOD|1},
    {L"escape",                    JSGlobal_escape,                    PROPF_METHOD|1},
    {L"eval",                      JSGlobal_eval,                      PROPF_METHOD|1},
    {L"isFinite",                  JSGlobal_isFinite,                  PROPF_METHOD|1},
    {L"isNaN",                     JSGlobal_isNaN,                     PROPF_METHOD|1},
    {L"parseFloat",                JSGlobal_parseFloat,                PROPF_METHOD|1},
    {L"parseInt",                  JSGlobal_parseInt,                  PROPF_METHOD|2},
    {L"unescape",                  JSGlobal_unescape,                  PROPF_METHOD|1}
};

static const builtin_info_t JSGlobal_info = {
    .class     = JSCLASS_GLOBAL,
    .props_cnt = ARRAY_SIZE(JSGlobal_props),
    .props     = JSGlobal_props,
};

static HRESULT init_object_prototype_accessors(script_ctx_t *ctx, jsdisp_t *object_prototype)
{
    property_desc_t desc;
    HRESULT hres = S_OK;

    /* __proto__ is an actual accessor on native, despite being a builtin */
    if(ctx->version >= SCRIPTLANGUAGEVERSION_ES6) {
        desc.flags = PROPF_CONFIGURABLE;
        desc.mask  = PROPF_CONFIGURABLE | PROPF_ENUMERABLE;
        desc.explicit_getter = desc.explicit_setter = TRUE;
        desc.explicit_value = FALSE;

        hres = create_builtin_function(ctx, Object_get_proto_, NULL, NULL, PROPF_METHOD, NULL, &desc.getter);
        if(SUCCEEDED(hres)) {
            hres = create_builtin_function(ctx, Object_set_proto_, NULL, NULL, PROPF_METHOD|1, NULL, &desc.setter);
            if(SUCCEEDED(hres)) {
                hres = jsdisp_define_property(object_prototype, L"__proto__", &desc);
                jsdisp_release(desc.setter);
            }
            jsdisp_release(desc.getter);
        }
    }
    return hres;
}

static HRESULT init_constructors(script_ctx_t *ctx, jsdisp_t *object_prototype)
{
    HRESULT hres;

    hres = init_function_constr(ctx, object_prototype);
    if(FAILED(hres))
        return hres;

    hres = jsdisp_define_data_property(ctx->global, L"Function", PROPF_CONFIGURABLE | PROPF_WRITABLE,
                                       jsval_obj(ctx->function_constr));
    if(FAILED(hres))
        return hres;

    hres = create_object_constr(ctx, object_prototype, &ctx->object_constr);
    if(FAILED(hres))
        return hres;

    hres = jsdisp_define_data_property(ctx->global, L"Object", PROPF_CONFIGURABLE | PROPF_WRITABLE,
                                       jsval_obj(ctx->object_constr));
    if(FAILED(hres))
        return hres;

    hres = create_array_constr(ctx, object_prototype, &ctx->array_constr);
    if(FAILED(hres))
        return hres;

    hres = jsdisp_define_data_property(ctx->global, L"Array", PROPF_CONFIGURABLE | PROPF_WRITABLE,
                                       jsval_obj(ctx->array_constr));
    if(FAILED(hres))
        return hres;

    hres = create_bool_constr(ctx, object_prototype, &ctx->bool_constr);
    if(FAILED(hres))
        return hres;

    hres = jsdisp_define_data_property(ctx->global, L"Boolean", PROPF_CONFIGURABLE | PROPF_WRITABLE,
                                       jsval_obj(ctx->bool_constr));
    if(FAILED(hres))
        return hres;

    hres = create_date_constr(ctx, object_prototype, &ctx->date_constr);
    if(FAILED(hres))
        return hres;

    hres = jsdisp_define_data_property(ctx->global, L"Date", PROPF_CONFIGURABLE | PROPF_WRITABLE,
                                       jsval_obj(ctx->date_constr));
    if(FAILED(hres))
        return hres;

    hres = create_enumerator_constr(ctx, object_prototype, &ctx->enumerator_constr);
    if(FAILED(hres))
        return hres;

    hres = jsdisp_define_data_property(ctx->global, L"Enumerator", PROPF_CONFIGURABLE | PROPF_WRITABLE,
                                       jsval_obj(ctx->enumerator_constr));
    if(FAILED(hres))
        return hres;

    hres = init_error_constr(ctx, object_prototype);
    if(FAILED(hres))
        return hres;

    hres = jsdisp_define_data_property(ctx->global, L"Error", PROPF_CONFIGURABLE | PROPF_WRITABLE,
                                       jsval_obj(ctx->error_constr));
    if(FAILED(hres))
        return hres;

    hres = jsdisp_define_data_property(ctx->global, L"EvalError", PROPF_CONFIGURABLE | PROPF_WRITABLE,
                                       jsval_obj(ctx->eval_error_constr));
    if(FAILED(hres))
        return hres;

    hres = jsdisp_define_data_property(ctx->global, L"RangeError", PROPF_CONFIGURABLE | PROPF_WRITABLE,
                                       jsval_obj(ctx->range_error_constr));
    if(FAILED(hres))
        return hres;

    hres = jsdisp_define_data_property(ctx->global, L"ReferenceError", PROPF_CONFIGURABLE | PROPF_WRITABLE,
                                       jsval_obj(ctx->reference_error_constr));
    if(FAILED(hres))
        return hres;

    hres = jsdisp_define_data_property(ctx->global, L"RegExpError", PROPF_CONFIGURABLE | PROPF_WRITABLE,
                                       jsval_obj(ctx->regexp_error_constr));
    if(FAILED(hres))
        return hres;

    hres = jsdisp_define_data_property(ctx->global, L"SyntaxError", PROPF_CONFIGURABLE | PROPF_WRITABLE,
                                       jsval_obj(ctx->syntax_error_constr));
    if(FAILED(hres))
        return hres;

    hres = jsdisp_define_data_property(ctx->global, L"TypeError", PROPF_CONFIGURABLE | PROPF_WRITABLE,
                                       jsval_obj(ctx->type_error_constr));
    if(FAILED(hres))
        return hres;

    hres = jsdisp_define_data_property(ctx->global, L"URIError", PROPF_CONFIGURABLE | PROPF_WRITABLE,
                                       jsval_obj(ctx->uri_error_constr));
    if(FAILED(hres))
        return hres;

    hres = create_number_constr(ctx, object_prototype, &ctx->number_constr);
    if(FAILED(hres))
        return hres;

    hres = jsdisp_define_data_property(ctx->global, L"Number", PROPF_CONFIGURABLE | PROPF_WRITABLE,
                                       jsval_obj(ctx->number_constr));
    if(FAILED(hres))
        return hres;

    hres = create_regexp_constr(ctx, object_prototype, &ctx->regexp_constr);
    if(FAILED(hres))
        return hres;

    hres = jsdisp_define_data_property(ctx->global, L"RegExp", PROPF_CONFIGURABLE | PROPF_WRITABLE,
                                       jsval_obj(ctx->regexp_constr));
    if(FAILED(hres))
        return hres;

    hres = create_string_constr(ctx, object_prototype, &ctx->string_constr);
    if(FAILED(hres))
        return hres;

    hres = jsdisp_define_data_property(ctx->global, L"String", PROPF_CONFIGURABLE | PROPF_WRITABLE,
                                       jsval_obj(ctx->string_constr));
    if(FAILED(hres))
        return hres;

    hres = create_vbarray_constr(ctx, object_prototype, &ctx->vbarray_constr);
    if(FAILED(hres))
        return hres;

    hres = jsdisp_define_data_property(ctx->global, L"VBArray", PROPF_CONFIGURABLE | PROPF_WRITABLE,
                                       jsval_obj(ctx->vbarray_constr));
    if(FAILED(hres))
        return hres;

    return S_OK;
}

HRESULT init_global(script_ctx_t *ctx)
{
    unsigned const_flags = ctx->version >= SCRIPTLANGUAGEVERSION_ES5 ? 0 : PROPF_WRITABLE;
    jsdisp_t *math, *constr;
    HRESULT hres;

    if(ctx->global)
        return S_OK;

    hres = create_dispex(ctx, &JSGlobal_info, NULL, &ctx->global);
    if(FAILED(hres))
        return hres;

    hres = create_object_prototype(ctx, &ctx->object_prototype);
    if(FAILED(hres))
        return hres;

    hres = init_constructors(ctx, ctx->object_prototype);
    if(FAILED(hres))
        return hres;

    hres = init_object_prototype_accessors(ctx, ctx->object_prototype);
    if(FAILED(hres))
        return hres;

    hres = create_math(ctx, &math);
    if(FAILED(hres))
        return hres;

    hres = jsdisp_define_data_property(ctx->global, L"Math", PROPF_CONFIGURABLE | PROPF_WRITABLE, jsval_obj(math));
    jsdisp_release(math);
    if(FAILED(hres))
        return hres;

    if(ctx->version >= 2) {
        jsdisp_t *json;

        hres = create_json(ctx, &json);
        if(FAILED(hres))
            return hres;

        hres = jsdisp_define_data_property(ctx->global, L"JSON", PROPF_CONFIGURABLE | PROPF_WRITABLE, jsval_obj(json));
        jsdisp_release(json);
        if(FAILED(hres))
            return hres;
    }

    hres = create_activex_constr(ctx, &constr);
    if(FAILED(hres))
        return hres;

    hres = jsdisp_define_data_property(ctx->global, L"ActiveXObject", PROPF_CONFIGURABLE | PROPF_WRITABLE,
                                       jsval_obj(constr));
    jsdisp_release(constr);
    if(FAILED(hres))
        return hres;

    hres = jsdisp_define_data_property(ctx->global, L"undefined", const_flags, jsval_undefined());
    if(FAILED(hres))
        return hres;

    hres = jsdisp_define_data_property(ctx->global, L"NaN", const_flags, jsval_number(NAN));
    if(FAILED(hres))
        return hres;

    hres = jsdisp_define_data_property(ctx->global, L"Infinity", const_flags, jsval_number(INFINITY));
    if(FAILED(hres))
        return hres;

    hres = init_arraybuf_constructors(ctx);
    if(FAILED(hres))
        return hres;

    return init_set_constructor(ctx);
}
