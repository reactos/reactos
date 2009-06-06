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

#include <limits.h>
#include <math.h>

#include "jscript.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

/* 1601 to 1970 is 369 years plus 89 leap days */
#define TIME_EPOCH  ((ULONGLONG)(369 * 365 + 89) * 86400 * 1000)

typedef struct {
    DispatchEx dispex;

    /* ECMA-262 3rd Edition    15.9.1.1 */
    DOUBLE time;

    LONG bias;
} DateInstance;

static const WCHAR toStringW[] = {'t','o','S','t','r','i','n','g',0};
static const WCHAR toLocaleStringW[] = {'t','o','L','o','c','a','l','e','S','t','r','i','n','g',0};
static const WCHAR hasOwnPropertyW[] = {'h','a','s','O','w','n','P','r','o','p','e','r','t','y',0};
static const WCHAR propertyIsEnumerableW[] =
    {'p','r','o','p','e','r','t','y','I','s','E','n','u','m','e','r','a','b','l','e',0};
static const WCHAR isPrototypeOfW[] = {'i','s','P','r','o','t','o','t','y','p','e','O','f',0};
static const WCHAR valueOfW[] = {'v','a','l','u','e','O','f',0};
static const WCHAR toUTCStringW[] = {'t','o','U','T','C','S','t','r','i','n','g',0};
static const WCHAR toDateStringW[] = {'t','o','D','a','t','e','S','t','r','i','n','g',0};
static const WCHAR toTimeStringW[] = {'t','o','T','i','m','e','S','t','r','i','n','g',0};
static const WCHAR toLocaleDateStringW[] = {'t','o','L','o','c','a','l','e','D','a','t','e','S','t','r','i','n','g',0};
static const WCHAR toLocaleTimeStringW[] = {'t','o','L','o','c','a','l','e','T','i','m','e','S','t','r','i','n','g',0};
static const WCHAR getTimeW[] = {'g','e','t','T','i','m','e',0};
static const WCHAR getFullYearW[] = {'g','e','t','F','u','l','l','Y','e','a','r',0};
static const WCHAR getUTCFullYearW[] = {'g','e','t','U','T','C','F','u','l','l','Y','e','a','r',0};
static const WCHAR getMonthW[] = {'g','e','t','M','o','n','t','h',0};
static const WCHAR getUTCMonthW[] = {'g','e','t','U','T','C','M','o','n','t','h',0};
static const WCHAR getDateW[] = {'g','e','t','D','a','t','e',0};
static const WCHAR getUTCDateW[] = {'g','e','t','U','T','C','D','a','t','e',0};
static const WCHAR getDayW[] = {'g','e','t','D','a','y',0};
static const WCHAR getUTCDayW[] = {'g','e','t','U','T','C','D','a','y',0};
static const WCHAR getHoursW[] = {'g','e','t','H','o','u','r','s',0};
static const WCHAR getUTCHoursW[] = {'g','e','t','U','T','C','H','o','u','r','s',0};
static const WCHAR getMinutesW[] = {'g','e','t','M','i','n','u','t','e','s',0};
static const WCHAR getUTCMinutesW[] = {'g','e','t','U','T','C','M','i','n','u','t','e','s',0};
static const WCHAR getSecondsW[] = {'g','e','t','S','e','c','o','n','d','s',0};
static const WCHAR getUTCSecondsW[] = {'g','e','t','U','T','C','S','e','c','o','n','d','s',0};
static const WCHAR getMilisecondsW[] = {'g','e','t','M','i','l','i','s','e','c','o','n','d','s',0};
static const WCHAR getUTCMilisecondsW[] = {'g','e','t','U','T','C','M','i','l','i','s','e','c','o','n','d','s',0};
static const WCHAR getTimezoneOffsetW[] = {'g','e','t','T','i','m','e','z','o','n','e','O','f','f','s','e','t',0};
static const WCHAR setTimeW[] = {'s','e','t','T','i','m','e',0};
static const WCHAR setMilisecondsW[] = {'s','e','t','M','i','l','i','s','e','c','o','n','d','s',0};
static const WCHAR setUTCMilisecondsW[] = {'s','e','t','U','T','C','M','i','l','i','s','e','c','o','n','d','s',0};
static const WCHAR setSecondsW[] = {'s','e','t','S','e','c','o','n','d','s',0};
static const WCHAR setUTCSecondsW[] = {'s','e','t','U','T','C','S','e','c','o','n','d','s',0};
static const WCHAR setMinutesW[] = {'s','e','t','M','i','n','u','t','e','s',0};
static const WCHAR setUTCMinutesW[] = {'s','e','t','U','T','C','M','i','n','u','t','e','s',0};
static const WCHAR setHoursW[] = {'s','e','t','H','o','u','r','s',0};
static const WCHAR setUTCHoursW[] = {'s','e','t','H','o','u','r','s',0};
static const WCHAR setDateW[] = {'s','e','t','D','a','t','e',0};
static const WCHAR setUTCDateW[] = {'s','e','t','U','T','C','D','a','t','e',0};
static const WCHAR setMonthW[] = {'s','e','t','M','o','n','t','h',0};
static const WCHAR setUTCMonthW[] = {'s','e','t','U','T','C','M','o','n','t','h',0};
static const WCHAR setFullYearW[] = {'s','e','t','F','u','l','l','Y','e','a','r',0};
static const WCHAR setUTCFullYearW[] = {'s','e','t','U','T','C','F','u','l','l','Y','e','a','r',0};

/* ECMA-262 3rd Edition    15.9.1.14 */
static inline DOUBLE time_clip(DOUBLE time)
{
    if(8.64e15 < time || time < -8.64e15) {
        return ret_nan();
    }

    return floor(time);
}

static HRESULT Date_toString(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Date_toLocaleString(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Date_hasOwnProperty(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Date_propertyIsEnumerable(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Date_isPrototypeOf(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Date_valueOf(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Date_toUTCString(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Date_toDateString(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Date_toTimeString(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Date_toLocaleDateString(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Date_toLocaleTimeString(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    FIXME("\n");
    return E_NOTIMPL;
}

/* ECMA-262 3rd Edition    15.9.5.9 */
static HRESULT Date_getTime(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    TRACE("\n");

    if(!is_class(dispex, JSCLASS_DATE)) {
        FIXME("throw TypeError\n");
        return E_FAIL;
    }

    if(retv) {
        DateInstance *date = (DateInstance*)dispex;
        num_set_val(retv, date->time);
    }
    return S_OK;
}

static HRESULT Date_getFullYear(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Date_getUTCFullYear(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Date_getMonth(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Date_getUTCMonth(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Date_getDate(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Date_getUTCDate(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Date_getDay(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Date_getUTCDay(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Date_getHours(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Date_getUTCHours(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Date_getMinutes(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Date_getUTCMinutes(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Date_getSeconds(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Date_getUTCSeconds(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Date_getMiliseconds(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Date_getUTCMiliseconds(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Date_getTimezoneOffset(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Date_setTime(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    VARIANT v;
    HRESULT hres;
    DateInstance *date;

    TRACE("\n");

    if(!is_class(dispex, JSCLASS_DATE)) {
        FIXME("throw TypeError\n");
        return E_FAIL;
    }

    if(!arg_cnt(dp)) {
        if(retv) num_set_nan(retv);
        return S_OK;
    }

    hres = to_number(dispex->ctx, get_arg(dp, 0), ei, &v);
    if(FAILED(hres))
        return hres;

    date = (DateInstance*)dispex;
    date->time = time_clip(num_val(&v));

    if(retv)
        num_set_val(retv, date->time);

    return S_OK;
}

static HRESULT Date_setMiliseconds(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Date_setUTCMiliseconds(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Date_setSeconds(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Date_setUTCSeconds(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Date_setMinutes(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Date_setUTCMinutes(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Date_setHours(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Date_setUTCHours(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Date_setDate(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Date_setUTCDate(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Date_setMonth(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Date_setUTCMonth(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Date_setFullYear(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Date_setUTCFullYear(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Date_value(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static const builtin_prop_t Date_props[] = {
    {getDateW,               Date_getDate,               PROPF_METHOD},
    {getDayW,                Date_getDay,                PROPF_METHOD},
    {getFullYearW,           Date_getFullYear,           PROPF_METHOD},
    {getHoursW,              Date_getHours,              PROPF_METHOD},
    {getMilisecondsW,        Date_getMiliseconds,        PROPF_METHOD},
    {getMinutesW,            Date_getMinutes,            PROPF_METHOD},
    {getMonthW,              Date_getMonth,              PROPF_METHOD},
    {getSecondsW,            Date_getSeconds,            PROPF_METHOD},
    {getTimeW,               Date_getTime,               PROPF_METHOD},
    {getTimezoneOffsetW,     Date_getTimezoneOffset,     PROPF_METHOD},
    {getUTCDateW,            Date_getUTCDate,            PROPF_METHOD},
    {getUTCDayW,             Date_getUTCDay,             PROPF_METHOD},
    {getUTCFullYearW,        Date_getUTCFullYear,        PROPF_METHOD},
    {getUTCHoursW,           Date_getUTCHours,           PROPF_METHOD},
    {getUTCMilisecondsW,     Date_getUTCMiliseconds,     PROPF_METHOD},
    {getUTCMinutesW,         Date_getUTCMinutes,         PROPF_METHOD},
    {getUTCMonthW,           Date_getUTCMonth,           PROPF_METHOD},
    {getUTCSecondsW,         Date_getUTCSeconds,         PROPF_METHOD},
    {hasOwnPropertyW,        Date_hasOwnProperty,        PROPF_METHOD},
    {isPrototypeOfW,         Date_isPrototypeOf,         PROPF_METHOD},
    {propertyIsEnumerableW,  Date_propertyIsEnumerable,  PROPF_METHOD},
    {setDateW,               Date_setDate,               PROPF_METHOD},
    {setFullYearW,           Date_setFullYear,           PROPF_METHOD},
    {setHoursW,              Date_setHours,              PROPF_METHOD},
    {setMilisecondsW,        Date_setMiliseconds,        PROPF_METHOD},
    {setMinutesW,            Date_setMinutes,            PROPF_METHOD},
    {setMonthW,              Date_setMonth,              PROPF_METHOD},
    {setSecondsW,            Date_setSeconds,            PROPF_METHOD},
    {setTimeW,               Date_setTime,               PROPF_METHOD},
    {setUTCDateW,            Date_setUTCDate,            PROPF_METHOD},
    {setUTCFullYearW,        Date_setUTCFullYear,        PROPF_METHOD},
    {setUTCHoursW,           Date_setUTCHours,           PROPF_METHOD},
    {setUTCMilisecondsW,     Date_setUTCMiliseconds,     PROPF_METHOD},
    {setUTCMinutesW,         Date_setUTCMinutes,         PROPF_METHOD},
    {setUTCMonthW,           Date_setUTCMonth,           PROPF_METHOD},
    {setUTCSecondsW,         Date_setUTCSeconds,         PROPF_METHOD},
    {toDateStringW,          Date_toDateString,          PROPF_METHOD},
    {toLocaleDateStringW,    Date_toLocaleDateString,    PROPF_METHOD},
    {toLocaleStringW,        Date_toLocaleString,        PROPF_METHOD},
    {toLocaleTimeStringW,    Date_toLocaleTimeString,    PROPF_METHOD},
    {toStringW,              Date_toString,              PROPF_METHOD},
    {toTimeStringW,          Date_toTimeString,          PROPF_METHOD},
    {toUTCStringW,           Date_toUTCString,           PROPF_METHOD},
    {valueOfW,               Date_valueOf,               PROPF_METHOD},
};

static const builtin_info_t Date_info = {
    JSCLASS_DATE,
    {NULL, Date_value, 0},
    sizeof(Date_props)/sizeof(*Date_props),
    Date_props,
    NULL,
    NULL
};

static HRESULT create_date(script_ctx_t *ctx, BOOL use_constr, DOUBLE time, DispatchEx **ret)
{
    DateInstance *date;
    HRESULT hres;
    TIME_ZONE_INFORMATION tzi;

    GetTimeZoneInformation(&tzi);

    date = heap_alloc_zero(sizeof(DateInstance));
    if(!date)
        return E_OUTOFMEMORY;

    if(use_constr)
        hres = init_dispex_from_constr(&date->dispex, ctx, &Date_info, ctx->date_constr);
    else
        hres = init_dispex(&date->dispex, ctx, &Date_info, NULL);
    if(FAILED(hres)) {
        heap_free(date);
        return hres;
    }

    date->time = time;
    date->bias = tzi.Bias;

    *ret = &date->dispex;
    return S_OK;
}

static HRESULT DateConstr_value(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    DispatchEx *date;
    HRESULT hres;

    TRACE("\n");

    switch(flags) {
    case DISPATCH_CONSTRUCT:
        switch(arg_cnt(dp)) {
        /* ECMA-262 3rd Edition    15.9.3.3 */
        case 0: {
            FILETIME time;
            LONGLONG lltime;

            GetSystemTimeAsFileTime(&time);
            lltime = ((LONGLONG)time.dwHighDateTime<<32)
                + time.dwLowDateTime;

            hres = create_date(dispex->ctx, TRUE, lltime/10000-TIME_EPOCH, &date);
            if(FAILED(hres))
                return hres;
            break;
        }

        /* ECMA-262 3rd Edition    15.9.3.2 */
        case 1: {
            VARIANT prim, num;

            hres = to_primitive(dispex->ctx, get_arg(dp,0), ei, &prim);
            if(FAILED(hres))
                return hres;

            if(V_VT(&prim) == VT_BSTR) {
                FIXME("VT_BSTR not supported\n");
                return E_NOTIMPL;
            }

            hres = to_number(dispex->ctx, &prim, ei, &num);
            VariantClear(&prim);
            if(FAILED(hres))
                return hres;

            hres = create_date(dispex->ctx, TRUE, time_clip(num_val(&num)), &date);
            if(FAILED(hres))
                return hres;
            break;
        }

        default:
            FIXME("unimplemented argcnt %d\n", arg_cnt(dp));
            return E_NOTIMPL;
        }

        V_VT(retv) = VT_DISPATCH;
        V_DISPATCH(retv) = (IDispatch*)_IDispatchEx_(date);
        return S_OK;

    default:
        FIXME("unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

HRESULT create_date_constr(script_ctx_t *ctx, DispatchEx **ret)
{
    DispatchEx *date;
    HRESULT hres;

    hres = create_date(ctx, FALSE, 0.0, &date);
    if(FAILED(hres))
        return hres;

    hres = create_builtin_function(ctx, DateConstr_value, PROPF_CONSTR, date, ret);

    jsdisp_release(date);
    return hres;
}
