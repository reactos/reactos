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

    double value;
} NumberInstance;

static const WCHAR toStringW[] = {'t','o','S','t','r','i','n','g',0};
static const WCHAR toLocaleStringW[] = {'t','o','L','o','c','a','l','e','S','t','r','i','n','g',0};
static const WCHAR toFixedW[] = {'t','o','F','i','x','e','d',0};
static const WCHAR toExponentialW[] = {'t','o','E','x','p','o','n','e','n','t','i','a','l',0};
static const WCHAR toPrecisionW[] = {'t','o','P','r','e','c','i','s','i','o','n',0};
static const WCHAR valueOfW[] = {'v','a','l','u','e','O','f',0};

#define NUMBER_TOSTRING_BUF_SIZE 64
#define NUMBER_DTOA_SIZE 18

static inline NumberInstance *number_from_jsdisp(jsdisp_t *jsdisp)
{
    return CONTAINING_RECORD(jsdisp, NumberInstance, dispex);
}

static inline NumberInstance *number_from_vdisp(vdisp_t *vdisp)
{
    return number_from_jsdisp(vdisp->u.jsdisp);
}

static inline NumberInstance *number_this(vdisp_t *jsthis)
{
    return is_vclass(jsthis, JSCLASS_NUMBER) ? number_from_vdisp(jsthis) : NULL;
}

static inline void number_to_str(double d, WCHAR *buf, int size, int *dec_point)
{
    ULONGLONG l;
    int i;

    /* TODO: this function should print doubles with bigger precision */
    assert(size>=2 && size<=NUMBER_DTOA_SIZE && d>=0);

    if(d == 0)
        *dec_point = 0;
    else
        *dec_point = floor(log10(d));
    l = d*pow(10, size-*dec_point-1);

    if(l%10 >= 5)
        l = l/10+1;
    else
        l /= 10;

    buf[size-1] = 0;
    for(i=size-2; i>=0; i--) {
        buf[i] = '0'+l%10;
        l /= 10;
    }

    /* log10 was wrong by 1 or rounding changed number of digits */
    if(l) {
        (*dec_point)++;
        memmove(buf+1, buf, size-2);
        buf[0] = '0'+l;
    }else if(buf[0]=='0' && buf[1]>='1' && buf[1]<='9') {
        (*dec_point)--;
        memmove(buf, buf+1, size-2);
        buf[size-2] = '0';
    }
}

static inline jsstr_t *number_to_fixed(double val, int prec)
{
    WCHAR buf[NUMBER_DTOA_SIZE];
    int dec_point, size, buf_size, buf_pos;
    BOOL neg = FALSE;
    jsstr_t *ret;
    WCHAR *str;

    TRACE("%lf %d\n", val, prec);

    if(val < 0) {
        neg = TRUE;
        val = -val;
    }

    if(val >= 1)
        buf_size = log10(val)+prec+2;
    else
        buf_size = prec ? prec+1 : 2;
    if(buf_size > NUMBER_DTOA_SIZE)
        buf_size = NUMBER_DTOA_SIZE;

    number_to_str(val, buf, buf_size, &dec_point);
    dec_point++;
    size = 0;
    if(neg)
        size++;
    if(dec_point > 0)
        size += dec_point;
    else
        size++;
    if(prec)
        size += prec+1;

    str = jsstr_alloc_buf(size, &ret);
    if(!ret)
        return NULL;

    size = buf_pos = 0;
    if(neg)
        str[size++] = '-';
    if(dec_point > 0) {
        for(;buf_pos<buf_size-1 && dec_point; dec_point--)
            str[size++] = buf[buf_pos++];
    }else {
        str[size++] = '0';
    }
    for(; dec_point>0; dec_point--)
        str[size++] = '0';
    if(prec) {
        str[size++] = '.';

        for(; dec_point<0 && prec; dec_point++, prec--)
            str[size++] = '0';
        for(; buf_pos<buf_size-1 && prec; prec--)
            str[size++] = buf[buf_pos++];
        for(; prec; prec--) {
            str[size++] = '0';
        }
    }
    str[size++] = 0;
    return ret;
}

static inline jsstr_t *number_to_exponential(double val, int prec)
{
    WCHAR buf[NUMBER_DTOA_SIZE], *pbuf;
    int dec_point, size, buf_size, exp_size = 1;
    BOOL neg = FALSE;
    jsstr_t *ret;
    WCHAR *str;

    if(val < 0) {
        neg = TRUE;
        val = -val;
    }

    buf_size = prec+2;
    if(buf_size<2 || buf_size>NUMBER_DTOA_SIZE)
        buf_size = NUMBER_DTOA_SIZE;
    number_to_str(val, buf, buf_size, &dec_point);
    buf_size--;
    if(prec == -1)
        for(; buf_size>1 && buf[buf_size-1]=='0'; buf_size--)
            buf[buf_size-1] = 0;

    size = 10;
    while(dec_point>=size || dec_point<=-size) {
        size *= 10;
        exp_size++;
    }

    if(buf_size == 1)
        size = buf_size+2+exp_size; /* 2 = strlen(e+) */
    else if(prec == -1)
        size = buf_size+3+exp_size; /* 3 = strlen(.e+) */
    else
        size = prec+4+exp_size; /* 4 = strlen(0.e+) */
    if(neg)
        size++;

    str = jsstr_alloc_buf(size, &ret);
    if(!ret)
        return NULL;

    size = 0;
    pbuf = buf;
    if(neg)
        str[size++] = '-';
    str[size++] = *pbuf++;
    if(buf_size != 1) {
        str[size++] = '.';
        while(*pbuf)
            str[size++] = *pbuf++;
        for(; prec>buf_size-1; prec--)
            str[size++] = '0';
    }
    str[size++] = 'e';
    if(dec_point >= 0) {
        str[size++] = '+';
    }else {
        str[size++] = '-';
        dec_point = -dec_point;
    }
    size += exp_size;
    do {
        str[--size] = '0'+dec_point%10;
        dec_point /= 10;
    }while(dec_point>0);
    size += exp_size;
    str[size] = 0;

    return ret;
}

/* ECMA-262 3rd Edition    15.7.4.2 */
static HRESULT Number_toString(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    NumberInstance *number;
    INT radix = 10;
    DOUBLE val;
    jsstr_t *str;
    HRESULT hres;

    TRACE("\n");

    if(!(number = number_this(jsthis)))
        return throw_type_error(ctx, JS_E_NUMBER_EXPECTED, NULL);

    if(argc) {
        hres = to_int32(ctx, argv[0], &radix);
        if(FAILED(hres))
            return hres;

        if(radix<2 || radix>36)
            return throw_type_error(ctx, JS_E_INVALIDARG, NULL);
    }

    val = number->value;

    if(radix==10 || isnan(val) || isinf(val)) {
        hres = to_string(ctx, jsval_number(val), &str);
        if(FAILED(hres))
            return hres;
    }else {
        INT idx = 0;
        DOUBLE integ, frac, log_radix = 0;
        WCHAR buf[NUMBER_TOSTRING_BUF_SIZE+16];
        BOOL exp = FALSE;

        if(val<0) {
            val = -val;
            buf[idx++] = '-';
        }

        while(1) {
            integ = floor(val);
            frac = val-integ;

            if(integ == 0)
                buf[idx++] = '0';
            while(integ>=1 && idx<NUMBER_TOSTRING_BUF_SIZE) {
                buf[idx] = fmod(integ, radix);
                if(buf[idx]<10) buf[idx] += '0';
                else buf[idx] += 'a'-10;
                integ /= radix;
                idx++;
            }

            if(idx<NUMBER_TOSTRING_BUF_SIZE) {
                INT beg = buf[0]=='-'?1:0;
                INT end = idx-1;
                WCHAR wch;

                while(end > beg) {
                    wch = buf[beg];
                    buf[beg++] = buf[end];
                    buf[end--] = wch;
                }
            }

            if(idx != NUMBER_TOSTRING_BUF_SIZE) buf[idx++] = '.';

            while(frac>0 && idx<NUMBER_TOSTRING_BUF_SIZE) {
                frac *= radix;
                buf[idx] = fmod(frac, radix);
                frac -= buf[idx];
                if(buf[idx]<10) buf[idx] += '0';
                else buf[idx] += 'a'-10;
                idx++;
            }

            if(idx==NUMBER_TOSTRING_BUF_SIZE && !exp) {
                exp = TRUE;
                idx = (buf[0]=='-') ? 1 : 0;
                log_radix = floor(log(val)/log(radix));
                val *= pow(radix, -log_radix);
                continue;
            }

            break;
        }

        while(buf[idx-1] == '0') idx--;
        if(buf[idx-1] == '.') idx--;

        if(exp) {
            if(log_radix==0)
                buf[idx] = 0;
            else {
                static const WCHAR formatW[] = {'(','e','%','c','%','d',')',0};
                WCHAR ch;

                if(log_radix<0) {
                    log_radix = -log_radix;
                    ch = '-';
                }
                else ch = '+';
                sprintfW(&buf[idx], formatW, ch, (int)log_radix);
            }
        }
        else buf[idx] = '\0';

        str = jsstr_alloc(buf);
        if(!str)
            return E_OUTOFMEMORY;
    }

    if(r)
        *r = jsval_string(str);
    else
        jsstr_release(str);
    return S_OK;
}

static HRESULT Number_toLocaleString(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Number_toFixed(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    NumberInstance *number;
    DOUBLE val;
    INT prec = 0;
    jsstr_t *str;
    HRESULT hres;

    TRACE("\n");

    if(!(number = number_this(jsthis)))
        return throw_type_error(ctx, JS_E_NUMBER_EXPECTED, NULL);

    if(argc) {
        hres = to_int32(ctx, argv[0], &prec);
        if(FAILED(hres))
            return hres;

        if(prec<0 || prec>20)
            return throw_range_error(ctx, JS_E_FRACTION_DIGITS_OUT_OF_RANGE, NULL);
    }

    val = number->value;
    if(isinf(val) || isnan(val)) {
        hres = to_string(ctx, jsval_number(val), &str);
        if(FAILED(hres))
            return hres;
    }else {
        str = number_to_fixed(val, prec);
        if(!str)
            return E_OUTOFMEMORY;
    }

    if(r)
        *r = jsval_string(str);
    else
        jsstr_release(str);
    return S_OK;
}

static HRESULT Number_toExponential(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    NumberInstance *number;
    DOUBLE val;
    INT prec = 0;
    jsstr_t *str;
    HRESULT hres;

    TRACE("\n");

    if(!(number = number_this(jsthis)))
        return throw_type_error(ctx, JS_E_NUMBER_EXPECTED, NULL);

    if(argc) {
        hres = to_int32(ctx, argv[0], &prec);
        if(FAILED(hres))
            return hres;

        if(prec<0 || prec>20)
            return throw_range_error(ctx, JS_E_FRACTION_DIGITS_OUT_OF_RANGE, NULL);
    }

    val = number->value;
    if(isinf(val) || isnan(val)) {
        hres = to_string(ctx, jsval_number(val), &str);
        if(FAILED(hres))
            return hres;
    }else {
        if(!prec)
            prec--;
        str = number_to_exponential(val, prec);
        if(!str)
            return E_OUTOFMEMORY;
    }

    if(r)
        *r = jsval_string(str);
    else
        jsstr_release(str);
    return S_OK;
}

static HRESULT Number_toPrecision(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    NumberInstance *number;
    INT prec = 0, size;
    jsstr_t *str;
    DOUBLE val;
    HRESULT hres;

    if(!(number = number_this(jsthis)))
        return throw_type_error(ctx, JS_E_NUMBER_EXPECTED, NULL);

    if(argc) {
        hres = to_int32(ctx, argv[0], &prec);
        if(FAILED(hres))
            return hres;

        if(prec<1 || prec>21)
            return throw_range_error(ctx, JS_E_PRECISION_OUT_OF_RANGE, NULL);
    }

    val = number->value;
    if(isinf(val) || isnan(val) || !prec) {
        hres = to_string(ctx, jsval_number(val), &str);
        if(FAILED(hres))
            return hres;
    }else {
        if(val != 0)
            size = floor(log10(val>0 ? val : -val)) + 1;
        else
            size = 1;

        if(size > prec)
            str = number_to_exponential(val, prec-1);
        else
            str = number_to_fixed(val, prec-size);
        if(!str)
            return E_OUTOFMEMORY;
    }

    if(r)
        *r = jsval_string(str);
    else
        jsstr_release(str);
    return S_OK;
}

static HRESULT Number_valueOf(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    NumberInstance *number;

    TRACE("\n");

    if(!(number = number_this(jsthis)))
        return throw_type_error(ctx, JS_E_NUMBER_EXPECTED, NULL);

    if(r)
        *r = jsval_number(number->value);
    return S_OK;
}

static HRESULT Number_get_value(script_ctx_t *ctx, jsdisp_t *jsthis, jsval_t *r)
{
    NumberInstance *number = number_from_jsdisp(jsthis);

    TRACE("(%p)\n", number);

    *r = jsval_number(number->value);
    return S_OK;
}

static const builtin_prop_t Number_props[] = {
    {toExponentialW,         Number_toExponential,         PROPF_METHOD|1},
    {toFixedW,               Number_toFixed,               PROPF_METHOD},
    {toLocaleStringW,        Number_toLocaleString,        PROPF_METHOD},
    {toPrecisionW,           Number_toPrecision,           PROPF_METHOD|1},
    {toStringW,              Number_toString,              PROPF_METHOD|1},
    {valueOfW,               Number_valueOf,               PROPF_METHOD}
};

static const builtin_info_t Number_info = {
    JSCLASS_NUMBER,
    {NULL, NULL,0, Number_get_value},
    sizeof(Number_props)/sizeof(*Number_props),
    Number_props,
    NULL,
    NULL
};

static const builtin_info_t NumberInst_info = {
    JSCLASS_NUMBER,
    {NULL, NULL,0, Number_get_value},
    0, NULL,
    NULL,
    NULL
};

static HRESULT NumberConstr_value(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    double n;
    HRESULT hres;

    TRACE("\n");

    switch(flags) {
    case INVOKE_FUNC:
        if(!argc) {
            if(r)
                *r = jsval_number(0);
            return S_OK;
        }

        hres = to_number(ctx, argv[0], &n);
        if(FAILED(hres))
            return hres;

        if(r)
            *r = jsval_number(n);
        break;

    case DISPATCH_CONSTRUCT: {
        jsdisp_t *obj;

        if(argc) {
            hres = to_number(ctx, argv[0], &n);
            if(FAILED(hres))
                return hres;
        }else {
            n = 0;
        }

        hres = create_number(ctx, n, &obj);
        if(FAILED(hres))
            return hres;

        *r = jsval_obj(obj);
        break;
    }
    default:
        FIXME("unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT alloc_number(script_ctx_t *ctx, jsdisp_t *object_prototype, NumberInstance **ret)
{
    NumberInstance *number;
    HRESULT hres;

    number = heap_alloc_zero(sizeof(NumberInstance));
    if(!number)
        return E_OUTOFMEMORY;

    if(object_prototype)
        hres = init_dispex(&number->dispex, ctx, &Number_info, object_prototype);
    else
        hres = init_dispex_from_constr(&number->dispex, ctx, &NumberInst_info, ctx->number_constr);
    if(FAILED(hres)) {
        heap_free(number);
        return hres;
    }

    *ret = number;
    return S_OK;
}

HRESULT create_number_constr(script_ctx_t *ctx, jsdisp_t *object_prototype, jsdisp_t **ret)
{
    NumberInstance *number;
    HRESULT hres;

    static const WCHAR NumberW[] = {'N','u','m','b','e','r',0};

    hres = alloc_number(ctx, object_prototype, &number);
    if(FAILED(hres))
        return hres;

    number->value = 0;
    hres = create_builtin_constructor(ctx, NumberConstr_value, NumberW, NULL,
            PROPF_CONSTR|1, &number->dispex, ret);

    jsdisp_release(&number->dispex);
    return hres;
}

HRESULT create_number(script_ctx_t *ctx, double value, jsdisp_t **ret)
{
    NumberInstance *number;
    HRESULT hres;

    hres = alloc_number(ctx, NULL, &number);
    if(FAILED(hres))
        return hres;

    number->value = value;

    *ret = &number->dispex;
    return S_OK;
}
