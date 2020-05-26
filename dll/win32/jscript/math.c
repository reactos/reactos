/*
 * Copyright 2008 Jacek Caban for CodeWeavers
 * Copyright 2009 Piotr Caban
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
#include "ntsecapi.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

static const WCHAR EW[] = {'E',0};
static const WCHAR LOG2EW[] = {'L','O','G','2','E',0};
static const WCHAR LOG10EW[] = {'L','O','G','1','0','E',0};
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

/* ECMA-262 3rd Edition    15.8.2.12 */
static HRESULT Math_abs(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    double d;
    HRESULT hres;

    TRACE("\n");

    if(!argc) {
        if(r)
            *r = jsval_number(NAN);
        return S_OK;
    }

    hres = to_number(ctx, argv[0], &d);
    if(FAILED(hres))
        return hres;

    if(r)
        *r = jsval_number(d < 0.0 ? -d : d);
    return S_OK;
}

static HRESULT Math_acos(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    double x;
    HRESULT hres;

    TRACE("\n");

    if(!argc) {
        if(r)
            *r = jsval_number(NAN);
        return S_OK;
    }

    hres = to_number(ctx, argv[0], &x);
    if(FAILED(hres))
        return hres;

    if(r)
        *r = jsval_number(x < -1.0 || x > 1.0 ? NAN : acos(x));
    return S_OK;
}

static HRESULT Math_asin(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    double x;
    HRESULT hres;

    TRACE("\n");

    if(!argc) {
        if(r)
            *r = jsval_number(NAN);
        return S_OK;
    }

    hres = to_number(ctx, argv[0], &x);
    if(FAILED(hres))
        return hres;

    if(r)
        *r = jsval_number(x < -1.0 || x > 1.0 ? NAN : asin(x));
    return S_OK;
}

static HRESULT Math_atan(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    double x;
    HRESULT hres;

    TRACE("\n");

    if(!argc) {
        if(r)
            *r = jsval_number(NAN);
        return S_OK;
    }

    hres = to_number(ctx, argv[0], &x);
    if(FAILED(hres))
        return hres;

    if(r)
        *r = jsval_number(atan(x));
    return S_OK;
}

static HRESULT Math_atan2(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    double x, y;
    HRESULT hres;

    TRACE("\n");

    if(argc<2) {
        if(r)
            *r = jsval_number(NAN);
        return S_OK;
    }

    hres = to_number(ctx, argv[0], &y);
    if(FAILED(hres))
        return hres;

    hres = to_number(ctx, argv[1], &x);
    if(FAILED(hres))
        return hres;

    if(r)
        *r = jsval_number(atan2(y, x));
    return S_OK;
}

/* ECMA-262 3rd Edition    15.8.2.6 */
static HRESULT Math_ceil(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    double x;
    HRESULT hres;

    TRACE("\n");

    if(!argc) {
        if(r)
            *r = jsval_number(NAN);
        return S_OK;
    }

    hres = to_number(ctx, argv[0], &x);
    if(FAILED(hres))
        return hres;

    if(r)
        *r = jsval_number(ceil(x));
    return S_OK;
}

static HRESULT Math_cos(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    double x;
    HRESULT hres;

    TRACE("\n");

    if(!argc) {
        if(r)
            *r = jsval_number(NAN);
        return S_OK;
    }

    hres = to_number(ctx, argv[0], &x);
    if(FAILED(hres))
        return hres;

    if(r)
        *r = jsval_number(cos(x));
    return S_OK;
}

static HRESULT Math_exp(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    double x;
    HRESULT hres;

    TRACE("\n");

    if(!argc) {
        if(r)
            *r = jsval_number(NAN);
        return S_OK;
    }

    hres = to_number(ctx, argv[0], &x);
    if(FAILED(hres))
        return hres;

    if(r)
        *r = jsval_number(exp(x));
    return S_OK;
}

static HRESULT Math_floor(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    double x;
    HRESULT hres;

    TRACE("\n");

    if(!argc) {
        if(r)
            *r = jsval_number(NAN);
        return S_OK;
    }

    hres = to_number(ctx, argv[0], &x);
    if(FAILED(hres))
        return hres;

    if(r)
        *r = jsval_number(floor(x));
    return S_OK;
}

static HRESULT Math_log(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    double x;
    HRESULT hres;

    TRACE("\n");

    if(!argc) {
        if(r)
            *r = jsval_number(NAN);
        return S_OK;
    }

    hres = to_number(ctx, argv[0], &x);
    if(FAILED(hres))
        return hres;

    if(r)
        *r = jsval_number(x < -0.0 ? NAN : log(x));
    return S_OK;
}

/* ECMA-262 3rd Edition    15.8.2.11 */
static HRESULT Math_max(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    DOUBLE max, d;
    DWORD i;
    HRESULT hres;

    TRACE("\n");

    if(!argc) {
        if(r)
            *r = jsval_number(-INFINITY);
        return S_OK;
    }

    hres = to_number(ctx, argv[0], &max);
    if(FAILED(hres))
        return hres;

    for(i=1; i < argc; i++) {
        hres = to_number(ctx, argv[i], &d);
        if(FAILED(hres))
            return hres;

        if(d > max || isnan(d))
            max = d;
    }

    if(r)
        *r = jsval_number(max);
    return S_OK;
}

/* ECMA-262 3rd Edition    15.8.2.12 */
static HRESULT Math_min(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    DOUBLE min, d;
    DWORD i;
    HRESULT hres;

    TRACE("\n");

    if(!argc) {
        if(r)
            *r = jsval_number(INFINITY);
        return S_OK;
    }

    hres = to_number(ctx, argv[0], &min);
    if(FAILED(hres))
        return hres;

    for(i=1; i < argc; i++) {
        hres = to_number(ctx, argv[i], &d);
        if(FAILED(hres))
            return hres;

        if(d < min || isnan(d))
            min = d;
    }

    if(r)
        *r = jsval_number(min);
    return S_OK;
}

/* ECMA-262 3rd Edition    15.8.2.13 */
static HRESULT Math_pow(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    double x, y;
    HRESULT hres;

    TRACE("\n");

    if(argc < 2) {
        if(r)
            *r = jsval_number(NAN);
        return S_OK;
    }

    hres = to_number(ctx, argv[0], &x);
    if(FAILED(hres))
        return hres;

    hres = to_number(ctx, argv[1], &y);
    if(FAILED(hres))
        return hres;

    if(r)
        *r = jsval_number(pow(x, y));
    return S_OK;
}

/* ECMA-262 3rd Edition    15.8.2.14 */
static HRESULT Math_random(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    UINT x;

    TRACE("\n");

    if(!RtlGenRandom(&x, sizeof(x)))
        return E_UNEXPECTED;

    if(r)
        *r = jsval_number((double)x/(double)UINT_MAX);
    return S_OK;
}

/* ECMA-262 3rd Edition    15.8.2.15 */
static HRESULT Math_round(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    double x;
    HRESULT hres;

    TRACE("\n");

    if(!argc) {
        if(r)
            *r = jsval_number(NAN);
        return S_OK;
    }

    hres = to_number(ctx, argv[0], &x);
    if(FAILED(hres))
        return hres;

    if(r)
        *r = jsval_number(floor(x+0.5));
    return S_OK;
}

static HRESULT Math_sin(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    double x;
    HRESULT hres;

    TRACE("\n");

    if(!argc) {
        if(r)
            *r = jsval_number(NAN);
        return S_OK;
    }

    hres = to_number(ctx, argv[0], &x);
    if(FAILED(hres))
        return hres;

    if(r)
        *r = jsval_number(sin(x));
    return S_OK;
}

static HRESULT Math_sqrt(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    double x;
    HRESULT hres;

    TRACE("\n");

    if(!argc) {
        if(r)
            *r = jsval_number(NAN);
        return S_OK;
    }

    hres = to_number(ctx, argv[0], &x);
    if(FAILED(hres))
        return hres;

    if(r)
        *r = jsval_number(sqrt(x));
    return S_OK;
}

static HRESULT Math_tan(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    double x;
    HRESULT hres;

    TRACE("\n");

    if(!argc) {
        if(r)
            *r = jsval_number(NAN);
        return S_OK;
    }

    hres = to_number(ctx, argv[0], &x);
    if(FAILED(hres))
        return hres;

    if(r)
        *r = jsval_number(tan(x));
    return S_OK;
}

static const builtin_prop_t Math_props[] = {
    {absW,      Math_abs,      PROPF_METHOD|1},
    {acosW,     Math_acos,     PROPF_METHOD|1},
    {asinW,     Math_asin,     PROPF_METHOD|1},
    {atanW,     Math_atan,     PROPF_METHOD|1},
    {atan2W,    Math_atan2,    PROPF_METHOD|2},
    {ceilW,     Math_ceil,     PROPF_METHOD|1},
    {cosW,      Math_cos,      PROPF_METHOD|1},
    {expW,      Math_exp,      PROPF_METHOD|1},
    {floorW,    Math_floor,    PROPF_METHOD|1},
    {logW,      Math_log,      PROPF_METHOD|1},
    {maxW,      Math_max,      PROPF_METHOD|2},
    {minW,      Math_min,      PROPF_METHOD|2},
    {powW,      Math_pow,      PROPF_METHOD|2},
    {randomW,   Math_random,   PROPF_METHOD},
    {roundW,    Math_round,    PROPF_METHOD|1},
    {sinW,      Math_sin,      PROPF_METHOD|1},
    {sqrtW,     Math_sqrt,     PROPF_METHOD|1},
    {tanW,      Math_tan,      PROPF_METHOD|1}
};

static const builtin_info_t Math_info = {
    JSCLASS_MATH,
    {NULL, NULL, 0},
    ARRAY_SIZE(Math_props),
    Math_props,
    NULL,
    NULL
};

HRESULT create_math(script_ctx_t *ctx, jsdisp_t **ret)
{
    jsdisp_t *math;
    unsigned i;
    HRESULT hres;

    struct {
        const WCHAR *name;
        DOUBLE val;
    }constants[] = {
        {EW,        M_E},        /* ECMA-262 3rd Edition    15.8.1.1 */
        {LN10W,     M_LN10},     /* ECMA-262 3rd Edition    15.8.1.2 */
        {LN2W,      M_LN2},      /* ECMA-262 3rd Edition    15.8.1.3 */
        {LOG2EW,    M_LOG2E},    /* ECMA-262 3rd Edition    15.8.1.4 */
        {LOG10EW,   M_LOG10E},   /* ECMA-262 3rd Edition    15.8.1.5 */
        {PIW,       M_PI},       /* ECMA-262 3rd Edition    15.8.1.6 */
        {SQRT1_2W,  M_SQRT1_2},  /* ECMA-262 3rd Edition    15.8.1.7 */
        {SQRT2W,    M_SQRT2},    /* ECMA-262 3rd Edition    15.8.1.8 */
    };

    math = heap_alloc_zero(sizeof(jsdisp_t));
    if(!math)
        return E_OUTOFMEMORY;

    hres = init_dispex_from_constr(math, ctx, &Math_info, ctx->object_constr);
    if(FAILED(hres)) {
        heap_free(math);
        return hres;
    }

    for(i=0; i < ARRAY_SIZE(constants); i++) {
        hres = jsdisp_define_data_property(math, constants[i].name, 0,
                                           jsval_number(constants[i].val));
        if(FAILED(hres)) {
            jsdisp_release(math);
            return hres;
        }
    }

    *ret = math;
    return S_OK;
}
