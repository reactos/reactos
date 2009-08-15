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
static const WCHAR hasOwnPropertyW[] = {'h','a','s','O','w','n','P','r','o','p','e','r','t','y',0};
static const WCHAR propertyIsEnumerableW[] =
    {'p','r','o','p','e','r','t','y','I','s','E','n','u','m','e','r','a','b','l','e',0};
static const WCHAR isPrototypeOfW[] = {'i','s','P','r','o','t','o','t','y','p','e','O','f',0};

#define NUMBER_TOSTRING_BUF_SIZE 64
/* ECMA-262 3rd Edition    15.7.4.2 */
static HRESULT Number_toString(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    NumberInstance *number;
    INT radix = 10;
    DOUBLE val;
    BSTR str;
    HRESULT hres;

    TRACE("\n");

    if(!is_class(dispex, JSCLASS_NUMBER))
        return throw_type_error(dispex->ctx, ei, IDS_NOT_NUM, NULL);

    number = (NumberInstance*)dispex;

    if(arg_cnt(dp)) {
        hres = to_int32(dispex->ctx, get_arg(dp, 0), ei, &radix);
        if(FAILED(hres))
            return hres;

        if(radix<2 || radix>36)
            return throw_type_error(dispex->ctx, ei, IDS_INVALID_CALL_ARG, NULL);
    }

    if(V_VT(&number->num) == VT_I4)
        val = V_I4(&number->num);
    else
        val = V_R8(&number->num);

    if(radix==10 || isnan(val) || isinf(val)) {
        hres = to_string(dispex->ctx, &number->num, ei, &str);
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

static HRESULT Number_toLocaleString(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Number_toFixed(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Number_toExponential(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Number_toPrecision(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Number_valueOf(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    TRACE("\n");

    if(!is_class(dispex, JSCLASS_NUMBER))
        return throw_type_error(dispex->ctx, ei, IDS_NOT_NUM, NULL);

    if(retv) {
        NumberInstance *number = (NumberInstance*)dispex;
        *retv = number->num;
    }
    return S_OK;
}

static HRESULT Number_hasOwnProperty(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Number_propertyIsEnumerable(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Number_isPrototypeOf(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Number_value(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    NumberInstance *number = (NumberInstance*)dispex;

    switch(flags) {
    case INVOKE_FUNC:
        return throw_type_error(dispex->ctx, ei, IDS_NOT_FUNC, NULL);
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
    {hasOwnPropertyW,        Number_hasOwnProperty,        PROPF_METHOD},
    {isPrototypeOfW,         Number_isPrototypeOf,         PROPF_METHOD},
    {propertyIsEnumerableW,  Number_propertyIsEnumerable,  PROPF_METHOD},
    {toExponentialW,         Number_toExponential,         PROPF_METHOD},
    {toFixedW,               Number_toFixed,               PROPF_METHOD},
    {toLocaleStringW,        Number_toLocaleString,        PROPF_METHOD},
    {toPrecisionW,           Number_toPrecision,           PROPF_METHOD},
    {toStringW,              Number_toString,              PROPF_METHOD},
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

static HRESULT NumberConstr_value(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
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

        hres = to_number(dispex->ctx, get_arg(dp, 0), ei, &num);
        if(FAILED(hres))
            return hres;

        if(retv)
            *retv = num;
        break;

    case DISPATCH_CONSTRUCT: {
        DispatchEx *obj;

        if(arg_cnt(dp)) {
            hres = to_number(dispex->ctx, get_arg(dp, 0), ei, &num);
            if(FAILED(hres))
                return hres;
        }else {
            V_VT(&num) = VT_I4;
            V_I4(&num) = 0;
        }

        hres = create_number(dispex->ctx, &num, &obj);
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

static HRESULT alloc_number(script_ctx_t *ctx, BOOL use_constr, NumberInstance **ret)
{
    NumberInstance *number;
    HRESULT hres;

    number = heap_alloc_zero(sizeof(NumberInstance));
    if(!number)
        return E_OUTOFMEMORY;

    if(use_constr)
        hres = init_dispex_from_constr(&number->dispex, ctx, &Number_info, ctx->number_constr);
    else
        hres = init_dispex(&number->dispex, ctx, &Number_info, NULL);
    if(FAILED(hres))
        return hres;

    *ret = number;
    return S_OK;
}

HRESULT create_number_constr(script_ctx_t *ctx, DispatchEx **ret)
{
    NumberInstance *number;
    HRESULT hres;

    hres = alloc_number(ctx, FALSE, &number);
    if(FAILED(hres))
        return hres;

    V_VT(&number->num) = VT_I4;
    hres = create_builtin_function(ctx, NumberConstr_value, NULL, PROPF_CONSTR, &number->dispex, ret);

    jsdisp_release(&number->dispex);
    return hres;
}

HRESULT create_number(script_ctx_t *ctx, VARIANT *num, DispatchEx **ret)
{
    NumberInstance *number;
    HRESULT hres;

    hres = alloc_number(ctx, TRUE, &number);
    if(FAILED(hres))
        return hres;

    number->num = *num;

    *ret = &number->dispex;
    return S_OK;
}
