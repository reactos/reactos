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

#include "config.h"
#include "wine/port.h"

#include <math.h>

#include "jscript.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

typedef struct {
    DispatchEx dispex;

    VARIANT num;
} NumberInstance;

static const WCHAR toStringW[] = {'t','o','S','t','r','i','n','g',0};
static const WCHAR toLocaleStringW[] = {'t','o','L','o','c','a','l','e','S','t','r','i','n','g',0};
static const WCHAR toFixedW[] = {'t','o','F','i','x','e','d',0};
static const WCHAR toExponentialW[] = {'t','o','E','x','p','o','n','e','n','t','i','a','l',0};
static const WCHAR toPrecisionW[] = {'t','o','P','r','e','c','i','s','i','o','n',0};
static const WCHAR valueOfW[] = {'v','a','l','u','e','O','f',0};

#define NUMBER_TOSTRING_BUF_SIZE 64

static inline NumberInstance *number_from_vdisp(vdisp_t *vdisp)
{
    return (NumberInstance*)vdisp->u.jsdisp;
}

static inline NumberInstance *number_this(vdisp_t *jsthis)
{
    return is_vclass(jsthis, JSCLASS_NUMBER) ? number_from_vdisp(jsthis) : NULL;
}

/* ECMA-262 3rd Edition    15.7.4.2 */
static HRESULT Number_toString(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    NumberInstance *number;
    INT radix = 10;
    DOUBLE val;
    BSTR str;
    HRESULT hres;

    TRACE("\n");

    if(!(number = number_this(jsthis)))
        return throw_type_error(ctx, ei, IDS_NOT_NUM, NULL);

    if(arg_cnt(dp)) {
        hres = to_int32(ctx, get_arg(dp, 0), ei, &radix);
        if(FAILED(hres))
            return hres;

        if(radix<2 || radix>36)
            return throw_type_error(ctx, ei, IDS_INVALID_CALL_ARG, NULL);
    }

    if(V_VT(&number->num) == VT_I4)
        val = V_I4(&number->num);
    else
        val = V_R8(&number->num);

    if(radix==10 || isnan(val) || isinf(val)) {
        hres = to_string(ctx, &number->num, ei, &str);
        if(FAILED(hres))
            return hres;
    }
    else {
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
                buf[idx++] = '\0';
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

        str = SysAllocString(buf);
        if(!str)
            return E_OUTOFMEMORY;
    }

    if(retv) {
        V_VT(retv) = VT_BSTR;
        V_BSTR(retv) = str;
    }else {
        SysFreeString(str);
    }
    return S_OK;
}

static HRESULT Number_toLocaleString(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Number_toFixed(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Number_toExponential(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Number_toPrecision(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Number_valueOf(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    NumberInstance *number;

    TRACE("\n");

    if(!(number = number_this(jsthis)))
        return throw_type_error(ctx, ei, IDS_NOT_NUM, NULL);

    if(retv)
        *retv = number->num;
    return S_OK;
}

static HRESULT Number_value(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    NumberInstance *number = number_from_vdisp(jsthis);

    switch(flags) {
    case INVOKE_FUNC:
        return throw_type_error(ctx, ei, IDS_NOT_FUNC, NULL);
    case DISPATCH_PROPERTYGET:
        *retv = number->num;
        break;

    default:
        FIXME("flags %x\n", flags);
        return E_NOTIMPL;
    }

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
    {NULL, Number_value, 0},
    sizeof(Number_props)/sizeof(*Number_props),
    Number_props,
    NULL,
    NULL
};

static HRESULT NumberConstr_value(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    VARIANT num;
    HRESULT hres;

    TRACE("\n");

    switch(flags) {
    case INVOKE_FUNC:
        if(!arg_cnt(dp)) {
            if(retv) {
                V_VT(retv) = VT_I4;
                V_I4(retv) = 0;
            }
            return S_OK;
        }

        hres = to_number(ctx, get_arg(dp, 0), ei, &num);
        if(FAILED(hres))
            return hres;

        if(retv)
            *retv = num;
        break;

    case DISPATCH_CONSTRUCT: {
        DispatchEx *obj;

        if(arg_cnt(dp)) {
            hres = to_number(ctx, get_arg(dp, 0), ei, &num);
            if(FAILED(hres))
                return hres;
        }else {
            V_VT(&num) = VT_I4;
            V_I4(&num) = 0;
        }

        hres = create_number(ctx, &num, &obj);
        if(FAILED(hres))
            return hres;

        V_VT(retv) = VT_DISPATCH;
        V_DISPATCH(retv) = (IDispatch*)_IDispatchEx_(obj);
        break;
    }
    default:
        FIXME("unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT alloc_number(script_ctx_t *ctx, DispatchEx *object_prototype, NumberInstance **ret)
{
    NumberInstance *number;
    HRESULT hres;

    number = heap_alloc_zero(sizeof(NumberInstance));
    if(!number)
        return E_OUTOFMEMORY;

    if(object_prototype)
        hres = init_dispex(&number->dispex, ctx, &Number_info, object_prototype);
    else
        hres = init_dispex_from_constr(&number->dispex, ctx, &Number_info, ctx->number_constr);
    if(FAILED(hres))
        return hres;

    *ret = number;
    return S_OK;
}

HRESULT create_number_constr(script_ctx_t *ctx, DispatchEx *object_prototype, DispatchEx **ret)
{
    NumberInstance *number;
    HRESULT hres;

    static const WCHAR NumberW[] = {'N','u','m','b','e','r',0};

    hres = alloc_number(ctx, object_prototype, &number);
    if(FAILED(hres))
        return hres;

    V_VT(&number->num) = VT_I4;
    hres = create_builtin_function(ctx, NumberConstr_value, NumberW, NULL,
            PROPF_CONSTR|1, &number->dispex, ret);

    jsdisp_release(&number->dispex);
    return hres;
}

HRESULT create_number(script_ctx_t *ctx, VARIANT *num, DispatchEx **ret)
{
    NumberInstance *number;
    HRESULT hres;

    hres = alloc_number(ctx, NULL, &number);
    if(FAILED(hres))
        return hres;

    number->num = *num;

    *ret = &number->dispex;
    return S_OK;
}
