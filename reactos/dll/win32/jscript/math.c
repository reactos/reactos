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

static const WCHAR EW[] = {'E',0};
static const WCHAR LOG2EW[] = {'L','O','G','2','E',0};
static const WCHAR LOG10EW[] = {'L','O','G','1','0',0};
static const WCHAR LN2W[] = {'L','N','2',0};
static const WCHAR LN10W[] = {'L','N','1','0',0};
static const WCHAR PIW[] = {'P','I',0};
static const WCHAR SQRT2W[] = {'S','Q','R','T','2',0};
static const WCHAR SQRT1_2W[] = {'S','Q','R','T','1','_','2',0};
static const WCHAR absW[] = {'a','b','s',0};
static const WCHAR acosW[] = {'a','c','o','s',0};
static const WCHAR asinW[] = {'a','s','i','n',0};
static const WCHAR atanW[] = {'a','t','a','n',0};
static const WCHAR atan2W[] = {'a','t','a','n','2',0};
static const WCHAR ceilW[] = {'c','e','i','l',0};
static const WCHAR cosW[] = {'c','o','s',0};
static const WCHAR expW[] = {'e','x','p',0};
static const WCHAR floorW[] = {'f','l','o','o','r',0};
static const WCHAR logW[] = {'l','o','g',0};
static const WCHAR maxW[] = {'m','a','x',0};
static const WCHAR minW[] = {'m','i','n',0};
static const WCHAR powW[] = {'p','o','w',0};
static const WCHAR randomW[] = {'r','a','n','d','o','m',0};
static const WCHAR roundW[] = {'r','o','u','n','d',0};
static const WCHAR sinW[] = {'s','i','n',0};
static const WCHAR sqrtW[] = {'s','q','r','t',0};
static const WCHAR tanW[] = {'t','a','n',0};

static HRESULT Math_E(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Math_LOG2E(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Math_LOG10E(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Math_LN2(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Math_LN10(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Math_PI(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Math_SQRT2(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Math_SQRT1_2(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Math_abs(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Math_acos(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Math_asin(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Math_atan(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Math_atan2(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Math_ceil(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Math_cos(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Math_exp(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Math_floor(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Math_log(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Math_max(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Math_min(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Math_pow(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Math_random(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Math_round(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Math_sin(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Math_sqrt(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Math_tan(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static const builtin_prop_t Math_props[] = {
    {EW,        Math_E,        0},
    {LOG2EW,    Math_LOG2E,    0},
    {LOG10EW,   Math_LOG10E,   0},
    {LN2W,      Math_LN2,      0},
    {LN10W,     Math_LN10,     0},
    {PIW,       Math_PI,       0},
    {SQRT1_2W,  Math_SQRT1_2,  0},
    {SQRT2W,    Math_SQRT2,    0},
    {absW,      Math_abs,      PROPF_METHOD},
    {acosW,     Math_acos,     PROPF_METHOD},
    {asinW,     Math_asin,     PROPF_METHOD},
    {atanW,     Math_atan,     PROPF_METHOD},
    {atan2W,    Math_atan2,    PROPF_METHOD},
    {ceilW,     Math_ceil,     PROPF_METHOD},
    {cosW,      Math_cos,      PROPF_METHOD},
    {expW,      Math_exp,      PROPF_METHOD},
    {floorW,    Math_floor,    PROPF_METHOD},
    {logW,      Math_log,      PROPF_METHOD},
    {maxW,      Math_max,      PROPF_METHOD},
    {minW,      Math_min,      PROPF_METHOD},
    {powW,      Math_pow,      PROPF_METHOD},
    {randomW,   Math_random,   PROPF_METHOD},
    {roundW,    Math_round,    PROPF_METHOD},
    {sinW,      Math_sin,      PROPF_METHOD},
    {sqrtW,     Math_sqrt,     PROPF_METHOD},
    {tanW,      Math_tan,      PROPF_METHOD}
};

static const builtin_info_t Math_info = {
    JSCLASS_MATH,
    {NULL, NULL, 0},
    sizeof(Math_props)/sizeof(*Math_props),
    Math_props,
    NULL,
    NULL
};

HRESULT create_math(script_ctx_t *ctx, DispatchEx **ret)
{
    return create_dispex(ctx, &Math_info, NULL, ret);
}
