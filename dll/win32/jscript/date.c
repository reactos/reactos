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

#include <limits.h>
#include <math.h>
#include <assert.h>

#include "jscript.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

typedef struct {
    jsdisp_t dispex;

    /* ECMA-262 3rd Edition    15.9.1.1 */
    DOUBLE time;

    LONG bias;
    SYSTEMTIME standardDate;
    LONG standardBias;
    SYSTEMTIME daylightDate;
    LONG daylightBias;
} DateInstance;

static inline DateInstance *date_from_jsdisp(jsdisp_t *jsdisp)
{
    return CONTAINING_RECORD(jsdisp, DateInstance, dispex);
}

static inline DateInstance *date_this(jsval_t vthis)
{
    jsdisp_t *jsdisp = is_object_instance(vthis) ? to_jsdisp(get_object(vthis)) : NULL;
    return (jsdisp && is_class(jsdisp, JSCLASS_DATE)) ? date_from_jsdisp(jsdisp) : NULL;
}

static inline double file_time_to_date_time(const FILETIME *ftime)
{
    /* 1601 to 1970 is 369 years plus 89 leap days */
    const LONGLONG time_epoch = (LONGLONG)(369 * 365 + 89) * 86400 * 1000;

    LONGLONG time = (((ULONGLONG)ftime->dwHighDateTime << 32) | ftime->dwLowDateTime) / 10000;

    return time - time_epoch;
}

/*ECMA-262 3rd Edition    15.9.1.2 */
#define MS_PER_DAY 86400000
#define MS_PER_HOUR 3600000
#define MS_PER_MINUTE 60000

/* ECMA-262 3rd Edition    15.9.1.2 */
static inline DOUBLE day(DOUBLE time)
{
    return floor(time / MS_PER_DAY);
}

/* ECMA-262 3rd Edition    15.9.1.2 */
static inline DOUBLE time_within_day(DOUBLE time)
{
    DOUBLE ret;

    ret = fmod(time, MS_PER_DAY);
    if(ret < 0)
        ret += MS_PER_DAY;

    return ret;
}

/* ECMA-262 3rd Edition    15.9.1.3 */
static inline DOUBLE days_in_year(DOUBLE year)
{
    int y;

    if(year != (int)year)
        return NAN;

    y = year;
    if(y%4 != 0) return 365;
    if(y%100 != 0) return 366;
    if(y%400 != 0) return 365;
    return 366;
}

/* ECMA-262 3rd Edition    15.9.1.3 */
static inline DOUBLE day_from_year(DOUBLE year)
{
    if(year != (int)year)
        return NAN;

    return floor(365.0*(year-1970) + floor((year-1969)/4)
        - floor((year-1901)/100) + floor((year-1601)/400));
}

static inline int day_from_month(int month, int in_leap_year)
{
    switch(month)
    {
        case 0:
            return 0;
        case 1:
            return 31;
        case 2:
            return 59+in_leap_year;
        case 3:
            return 90+in_leap_year;
        case 4:
            return 120+in_leap_year;
        case 5:
            return 151+in_leap_year;
        case 6:
            return 181+in_leap_year;
        case 7:
            return 212+in_leap_year;
        case 8:
            return 243+in_leap_year;
        case 9:
            return 273+in_leap_year;
        case 10:
            return 304+in_leap_year;
        default:
            return 334+in_leap_year;
    }
}

/* ECMA-262 3rd Edition    15.9.1.3 */
static inline DOUBLE time_from_year(DOUBLE year)
{
    return MS_PER_DAY*day_from_year(year);
}

/* ECMA-262 3rd Edition    15.9.1.3 */
static inline DOUBLE year_from_time(DOUBLE time)
{
    int y;

    if(isnan(time))
        return NAN;

    y = 1970 + time/365.25/MS_PER_DAY;

    if(time_from_year(y) > time)
        while(time_from_year(y) > time) y--;
    else
        while(time_from_year(y+1)<=time) y++;

    return y;
}

/* ECMA-262 3rd Edition    15.9.1.3 */
static inline int in_leap_year(DOUBLE time)
{
    if(days_in_year(year_from_time(time))==366)
        return 1;
    return 0;
}

/* ECMA-262 3rd Edition    15.9.1.4 */
static inline int day_within_year(DOUBLE time)
{
    return day(time) - day_from_year(year_from_time(time));
}

/* ECMA-262 3rd Edition    15.9.1.4 */
static inline DOUBLE month_from_time(DOUBLE time)
{
    int ily = in_leap_year(time);
    int dwy = day_within_year(time);

    if(isnan(time))
        return NAN;

    if(0<=dwy && dwy<31) return 0;
    if(dwy < 59+ily) return 1;
    if(dwy < 90+ily) return 2;
    if(dwy < 120+ily) return 3;
    if(dwy < 151+ily) return 4;
    if(dwy < 181+ily) return 5;
    if(dwy < 212+ily) return 6;
    if(dwy < 243+ily) return 7;
    if(dwy < 273+ily) return 8;
    if(dwy < 304+ily) return  9;
    if(dwy < 334+ily) return  10;
    return  11;
}

/* ECMA-262 3rd Edition    15.9.1.5 */
static inline DOUBLE date_from_time(DOUBLE time)
{
    int dwy = day_within_year(time);
    int ily = in_leap_year(time);
    int mft = month_from_time(time);

    if(isnan(time))
        return NAN;

    if(mft==0) return dwy+1;
    if(mft==1) return dwy-30;
    if(mft==2) return dwy-58-ily;
    if(mft==3) return dwy-89-ily;
    if(mft==4) return dwy-119-ily;
    if(mft==5) return dwy-150-ily;
    if(mft==6) return dwy-180-ily;
    if(mft==7) return dwy-211-ily;
    if(mft==8) return dwy-242-ily;
    if(mft==9) return dwy-272-ily;
    if(mft==10) return dwy-303-ily;
    return dwy-333-ily;
}

/* ECMA-262 3rd Edition    15.9.1.6 */
static inline DOUBLE week_day(DOUBLE time)
{
    DOUBLE ret;

    if(isnan(time))
        return NAN;

    ret = fmod(day(time)+4, 7);
    if(ret<0) ret += 7;

    return ret;
}

static inline DOUBLE convert_time(int year, SYSTEMTIME st)
{
    DOUBLE time;
    int set_week_day;

    if(st.wMonth == 0)
        return NAN;

    if(st.wYear != 0)
        year = st.wYear;

    time = time_from_year(year);
    time += (DOUBLE)day_from_month(st.wMonth-1, in_leap_year(time)) * MS_PER_DAY;

    if(st.wYear == 0) {
        set_week_day = st.wDayOfWeek-week_day(time);
        if(set_week_day < 0)
            set_week_day += 7;
        time += set_week_day * MS_PER_DAY;

        time += (DOUBLE)(st.wDay-1) * 7 * MS_PER_DAY;
        if(month_from_time(time) != st.wMonth-1)
            time -= 7 * MS_PER_DAY;
    }
    else
        time += st.wDay * MS_PER_DAY;

    time += st.wHour * MS_PER_HOUR;
    time += st.wMinute * MS_PER_MINUTE;

    return time;
}

/* ECMA-262 3rd Edition    15.9.1.9 */
static inline DOUBLE daylight_saving_ta(DOUBLE time, DateInstance *date)
{
    int year = year_from_time(time);
    DOUBLE standardTime, daylightTime;

    if(isnan(time))
        return 0;

    standardTime = convert_time(year, date->standardDate);
    daylightTime = convert_time(year, date->daylightDate);

    if(isnan(standardTime) || isnan(daylightTime))
        return 0;
    else if(standardTime > daylightTime) {
        if(daylightTime <= time && time < standardTime)
            return date->daylightBias;

        return date->standardBias;
    }
    else {
        if(standardTime <= time && time < daylightTime)
            return date->standardBias;

        return date->daylightBias;
    }
}

/* ECMA-262 3rd Edition    15.9.1.9 */
static inline DOUBLE local_time(DOUBLE time, DateInstance *date)
{
    return time - (daylight_saving_ta(time, date)+date->bias)*MS_PER_MINUTE;
}

/* ECMA-262 3rd Edition    15.9.1.9 */
static inline DOUBLE utc(DOUBLE time, DateInstance *date)
{
    time += date->bias * MS_PER_MINUTE;
    return time + daylight_saving_ta(time, date)*MS_PER_MINUTE;
}

/* ECMA-262 3rd Edition    15.9.1.10 */
static inline DOUBLE hour_from_time(DOUBLE time)
{
    DOUBLE ret;

    if(isnan(time))
        return NAN;

    ret = fmod(floor(time/MS_PER_HOUR), 24);
    if(ret<0) ret += 24;

    return ret;
}

/* ECMA-262 3rd Edition    15.9.1.10 */
static inline DOUBLE min_from_time(DOUBLE time)
{
    DOUBLE ret;

    if(isnan(time))
        return NAN;

    ret = fmod(floor(time/MS_PER_MINUTE), 60);
    if(ret<0) ret += 60;

    return ret;
}

/* ECMA-262 3rd Edition    15.9.1.10 */
static inline DOUBLE sec_from_time(DOUBLE time)
{
    DOUBLE ret;

    if(isnan(time))
        return NAN;

    ret = fmod(floor(time/1000), 60);
    if(ret<0) ret += 60;

    return ret;
}

/* ECMA-262 3rd Edition    15.9.1.10 */
static inline DOUBLE ms_from_time(DOUBLE time)
{
    DOUBLE ret;

    if(isnan(time))
        return NAN;

    ret = fmod(time, 1000);
    if(ret<0) ret += 1000;

    return ret;
}

/* ECMA-262 3rd Edition    15.9.1.11 */
static inline DOUBLE make_time(DOUBLE hour, DOUBLE min, DOUBLE sec, DOUBLE ms)
{
    return hour*MS_PER_HOUR + min*MS_PER_MINUTE + sec*1000 + ms;
}

/* ECMA-262 3rd Edition    15.9.1.12 */
static inline DOUBLE make_day(DOUBLE year, DOUBLE month, DOUBLE day)
{
    DOUBLE time;

    year += floor(month/12);

    month = fmod(month, 12);
    if(month<0) month += 12;

    time = time_from_year(year);

    day += floor(time / MS_PER_DAY);
    day += day_from_month(month, in_leap_year(time));

    return day-1;
}

/* ECMA-262 3rd Edition    15.9.1.13 */
static inline DOUBLE make_date(DOUBLE day, DOUBLE time)
{
    return day*MS_PER_DAY + time;
}

/* ECMA-262 3rd Edition    15.9.1.14 */
static inline DOUBLE time_clip(DOUBLE time)
{
    if(8.64e15 < time || time < -8.64e15) {
        return NAN;
    }

    return floor(time);
}

static double date_now(void)
{
    FILETIME ftime;

    GetSystemTimeAsFileTime(&ftime);
    return file_time_to_date_time(&ftime);
}

static SYSTEMTIME create_systemtime(DOUBLE time)
{
    SYSTEMTIME st;

    st.wYear = year_from_time(time);
    st.wMonth = month_from_time(time) + 1;
    st.wDayOfWeek = week_day(time);
    st.wDay = date_from_time(time);
    st.wHour = hour_from_time(time);
    st.wMinute = min_from_time(time);
    st.wSecond = sec_from_time(time);
    st.wMilliseconds = ms_from_time(time);

    return st;
}

static inline HRESULT date_to_string(DOUBLE time, BOOL show_offset, int offset, jsval_t *r)
{
    static const DWORD week_ids[] = { LOCALE_SABBREVDAYNAME7, LOCALE_SABBREVDAYNAME1,
        LOCALE_SABBREVDAYNAME2, LOCALE_SABBREVDAYNAME3, LOCALE_SABBREVDAYNAME4,
        LOCALE_SABBREVDAYNAME5, LOCALE_SABBREVDAYNAME6 };
    static const DWORD month_ids[] = { LOCALE_SABBREVMONTHNAME1, LOCALE_SABBREVMONTHNAME2,
        LOCALE_SABBREVMONTHNAME3, LOCALE_SABBREVMONTHNAME4,
        LOCALE_SABBREVMONTHNAME5, LOCALE_SABBREVMONTHNAME6,
        LOCALE_SABBREVMONTHNAME7, LOCALE_SABBREVMONTHNAME8,
        LOCALE_SABBREVMONTHNAME9, LOCALE_SABBREVMONTHNAME10,
        LOCALE_SABBREVMONTHNAME11, LOCALE_SABBREVMONTHNAME12 };

    const WCHAR *formatEra = L"";
    WCHAR week[64], month[64];
    WCHAR buf[192];
    jsstr_t *date_jsstr;
    int year, day;
    DWORD lcid_en;
    WCHAR sign = '-';

    if(isnan(time)) {
        if(r)
            *r = jsval_string(jsstr_nan());
        return S_OK;
    }

    if(r) {
        lcid_en = MAKELCID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),SORT_DEFAULT);

        week[0] = 0;
        GetLocaleInfoW(lcid_en, week_ids[(int)week_day(time)], week, ARRAY_SIZE(week));

        month[0] = 0;
        GetLocaleInfoW(lcid_en, month_ids[(int)month_from_time(time)], month, ARRAY_SIZE(month));

        year = year_from_time(time);
        if(year<0) {
            formatEra = L" B.C.";
            year = -year+1;
        }

        day = date_from_time(time);

        if(offset < 0) {
            sign = '+';
            offset = -offset;
        }

        if(!show_offset)
            swprintf(buf, ARRAY_SIZE(buf), L"%s %s %d %02d:%02d:%02d %d%s", week, month, day,
                    (int)hour_from_time(time), (int)min_from_time(time),
                    (int)sec_from_time(time), year, formatEra);
        else if(offset)
            swprintf(buf, ARRAY_SIZE(buf), L"%s %s %d %02d:%02d:%02d UTC%c%02d%02d %d%s", week, month, day,
                    (int)hour_from_time(time), (int)min_from_time(time),
                    (int)sec_from_time(time), sign, offset/60, offset%60,
                    year, formatEra);
        else
            swprintf(buf, ARRAY_SIZE(buf), L"%s %s %d %02d:%02d:%02d UTC %d%s", week, month, day,
                    (int)hour_from_time(time), (int)min_from_time(time),
                    (int)sec_from_time(time), year, formatEra);

        date_jsstr = jsstr_alloc(buf);
        if(!date_jsstr)
            return E_OUTOFMEMORY;

        *r = jsval_string(date_jsstr);
    }
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.1.2 */
static HRESULT dateobj_to_string(DateInstance *date, jsval_t *r)
{
    DOUBLE time;
    int offset;

    time = local_time(date->time, date);
    offset = date->bias +
        daylight_saving_ta(time, date);

    return date_to_string(time, TRUE, offset, r);
}

static HRESULT Date_toString(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv, jsval_t *r)
{
    DateInstance *date;

    TRACE("\n");

    if(!(date = date_this(vthis)))
        return JS_E_DATE_EXPECTED;

    return dateobj_to_string(date, r);
}

/* ECMA-262 3rd Edition    15.9.1.5 */
static HRESULT Date_toLocaleString(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    SYSTEMTIME st;
    DateInstance *date;
    jsstr_t *date_str;
    int date_len, time_len;

    TRACE("\n");

    if(!(date = date_this(vthis)))
        return JS_E_DATE_EXPECTED;

    if(isnan(date->time)) {
        if(r)
            *r = jsval_string(jsstr_nan());
        return S_OK;
    }

    st = create_systemtime(local_time(date->time, date));

    if(st.wYear<1601 || st.wYear>9999)
        return dateobj_to_string(date, r);

    if(r) {
        WCHAR *ptr;

        date_len = GetDateFormatW(ctx->lcid, DATE_LONGDATE, &st, NULL, NULL, 0);
        time_len = GetTimeFormatW(ctx->lcid, 0, &st, NULL, NULL, 0);

        date_str = jsstr_alloc_buf(date_len+time_len-1, &ptr);
        if(!date_str)
            return E_OUTOFMEMORY;

        GetDateFormatW(ctx->lcid, DATE_LONGDATE, &st, NULL, ptr, date_len);
        GetTimeFormatW(ctx->lcid, 0, &st, NULL, ptr+date_len, time_len);
        ptr[date_len-1] = ' ';

        *r = jsval_string(date_str);
    }
    return S_OK;
}

static HRESULT Date_toISOString(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    DateInstance *date;
    WCHAR buf[64], *p = buf;
    double year;

    TRACE("\n");

    if(!(date = date_this(vthis)))
        return JS_E_DATE_EXPECTED;

    year = year_from_time(date->time);
    if(isnan(year) || year > 999999 || year < -999999) {
        FIXME("year %lf should throw an exception\n", year);
        return E_FAIL;
    }

    if(year < 0) {
        *p++ = '-';
        p += swprintf(p, ARRAY_SIZE(buf) - 1, L"%06d", -(int)year);
    }else if(year > 9999) {
        *p++ = '+';
        p += swprintf(p, ARRAY_SIZE(buf) - 1, L"%06d", (int)year);
    }else {
        p += swprintf(p, ARRAY_SIZE(buf), L"%04d", (int)year);
    }

    swprintf(p, ARRAY_SIZE(buf) - (p - buf), L"-%02d-%02dT%02d:%02d:%02d.%03dZ",
             (int)month_from_time(date->time) + 1, (int)date_from_time(date->time),
             (int)hour_from_time(date->time), (int)min_from_time(date->time),
             (int)sec_from_time(date->time), (int)ms_from_time(date->time));

    if(r) {
        jsstr_t *ret;
        if(!(ret = jsstr_alloc(buf)))
            return E_OUTOFMEMORY;
        *r = jsval_string(ret);
    }
    return S_OK;
}

static HRESULT Date_valueOf(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    DateInstance *date;

    TRACE("\n");

    if(!(date = date_this(vthis)))
        return JS_E_DATE_EXPECTED;

    if(r)
        *r = jsval_number(date->time);
    return S_OK;
}

static inline HRESULT create_utc_string(script_ctx_t *ctx, jsval_t vthis, jsval_t *r)
{
    static const DWORD week_ids[] = { LOCALE_SABBREVDAYNAME7, LOCALE_SABBREVDAYNAME1,
        LOCALE_SABBREVDAYNAME2, LOCALE_SABBREVDAYNAME3, LOCALE_SABBREVDAYNAME4,
        LOCALE_SABBREVDAYNAME5, LOCALE_SABBREVDAYNAME6 };
    static const DWORD month_ids[] = { LOCALE_SABBREVMONTHNAME1, LOCALE_SABBREVMONTHNAME2,
        LOCALE_SABBREVMONTHNAME3, LOCALE_SABBREVMONTHNAME4,
        LOCALE_SABBREVMONTHNAME5, LOCALE_SABBREVMONTHNAME6,
        LOCALE_SABBREVMONTHNAME7, LOCALE_SABBREVMONTHNAME8,
        LOCALE_SABBREVMONTHNAME9, LOCALE_SABBREVMONTHNAME10,
        LOCALE_SABBREVMONTHNAME11, LOCALE_SABBREVMONTHNAME12 };

    const WCHAR *formatEra = L"";
    WCHAR week[64], month[64];
    WCHAR buf[192];
    DateInstance *date;
    jsstr_t *date_str;
    int year, day;
    DWORD lcid_en;

    if(!(date = date_this(vthis)))
        return JS_E_DATE_EXPECTED;

    if(isnan(date->time)) {
        if(r)
            *r = jsval_string(jsstr_nan());
        return S_OK;
    }

    if(r) {
        lcid_en = MAKELCID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),SORT_DEFAULT);

        week[0] = 0;
        GetLocaleInfoW(lcid_en, week_ids[(int)week_day(date->time)], week, ARRAY_SIZE(week));

        month[0] = 0;
        GetLocaleInfoW(lcid_en, month_ids[(int)month_from_time(date->time)], month, ARRAY_SIZE(month));

        year = year_from_time(date->time);
        if(year<0) {
            formatEra = L" B.C.";
            year = -year+1;
        }

        day = date_from_time(date->time);

        swprintf(buf, ARRAY_SIZE(buf),
                L"%s, %d %s %d%s %02d:%02d:%02d UTC", week, day, month, year, formatEra,
                (int)hour_from_time(date->time), (int)min_from_time(date->time),
                (int)sec_from_time(date->time));

        date_str = jsstr_alloc(buf);
        if(!date_str)
            return E_OUTOFMEMORY;

        *r = jsval_string(date_str);
    }
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.42 */
static HRESULT Date_toUTCString(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    TRACE("\n");
    return create_utc_string(ctx, vthis, r);
}

static HRESULT Date_toGMTString(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    TRACE("\n");
    return create_utc_string(ctx, vthis, r);
}

/* ECMA-262 3rd Edition    15.9.5.3 */
static HRESULT dateobj_to_date_string(DateInstance *date, jsval_t *r)
{
    static const DWORD week_ids[] = { LOCALE_SABBREVDAYNAME7, LOCALE_SABBREVDAYNAME1,
        LOCALE_SABBREVDAYNAME2, LOCALE_SABBREVDAYNAME3, LOCALE_SABBREVDAYNAME4,
        LOCALE_SABBREVDAYNAME5, LOCALE_SABBREVDAYNAME6 };
    static const DWORD month_ids[] = { LOCALE_SABBREVMONTHNAME1, LOCALE_SABBREVMONTHNAME2,
        LOCALE_SABBREVMONTHNAME3, LOCALE_SABBREVMONTHNAME4,
        LOCALE_SABBREVMONTHNAME5, LOCALE_SABBREVMONTHNAME6,
        LOCALE_SABBREVMONTHNAME7, LOCALE_SABBREVMONTHNAME8,
        LOCALE_SABBREVMONTHNAME9, LOCALE_SABBREVMONTHNAME10,
        LOCALE_SABBREVMONTHNAME11, LOCALE_SABBREVMONTHNAME12 };

    const WCHAR *formatEra = L"";
    WCHAR week[64], month[64];
    WCHAR buf[192];
    jsstr_t *date_str;
    DOUBLE time;
    int year, day;
    DWORD lcid_en;

    if(isnan(date->time)) {
        if(r)
            *r = jsval_string(jsstr_nan());
        return S_OK;
    }

    time = local_time(date->time, date);

    if(r) {
        lcid_en = MAKELCID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),SORT_DEFAULT);

        week[0] = 0;
        GetLocaleInfoW(lcid_en, week_ids[(int)week_day(time)], week, ARRAY_SIZE(week));

        month[0] = 0;
        GetLocaleInfoW(lcid_en, month_ids[(int)month_from_time(time)], month, ARRAY_SIZE(month));

        year = year_from_time(time);
        if(year<0) {
            formatEra = L" B.C.";
            year = -year+1;
        }

        day = date_from_time(time);

        swprintf(buf, ARRAY_SIZE(buf), L"%s %s %d %d%s", week, month, day, year, formatEra);

        date_str = jsstr_alloc(buf);
        if(!date_str)
            return E_OUTOFMEMORY;

        *r = jsval_string(date_str);
    }
    return S_OK;
}

static HRESULT Date_toDateString(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    DateInstance *date;

    if(!(date = date_this(vthis)))
        return JS_E_DATE_EXPECTED;

    return dateobj_to_date_string(date, r);
}

/* ECMA-262 3rd Edition    15.9.5.4 */
static HRESULT Date_toTimeString(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    DateInstance *date;
    jsstr_t *date_str;
    WCHAR buf[32];
    DOUBLE time;
    WCHAR sign;
    int offset;

    TRACE("\n");

    if(!(date = date_this(vthis)))
        return JS_E_DATE_EXPECTED;

    if(isnan(date->time)) {
        if(r)
            *r = jsval_string(jsstr_nan());
        return S_OK;
    }

    time = local_time(date->time, date);

    if(r) {
        offset = date->bias +
            daylight_saving_ta(time, date);

        if(offset < 0) {
            sign = '+';
            offset = -offset;
        }
        else sign = '-';

        if(offset)
            swprintf(buf, ARRAY_SIZE(buf), L"%02d:%02d:%02d UTC%c%02d%02d", (int)hour_from_time(time),
                    (int)min_from_time(time), (int)sec_from_time(time),
                    sign, offset/60, offset%60);
        else
            swprintf(buf, ARRAY_SIZE(buf), L"%02d:%02d:%02d UTC", (int)hour_from_time(time),
                    (int)min_from_time(time), (int)sec_from_time(time));

        date_str = jsstr_alloc(buf);
        if(!date_str)
            return E_OUTOFMEMORY;

        *r = jsval_string(date_str);
    }
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.6 */
static HRESULT Date_toLocaleDateString(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    SYSTEMTIME st;
    DateInstance *date;
    jsstr_t *date_str;
    int len;

    TRACE("\n");

    if(!(date = date_this(vthis)))
        return JS_E_DATE_EXPECTED;

    if(isnan(date->time)) {
        if(r)
            *r = jsval_string(jsstr_nan());
        return S_OK;
    }

    st = create_systemtime(local_time(date->time, date));

    if(st.wYear<1601 || st.wYear>9999)
        return dateobj_to_date_string(date, r);

    if(r) {
        WCHAR *ptr;

        len = GetDateFormatW(ctx->lcid, DATE_LONGDATE, &st, NULL, NULL, 0);
        date_str = jsstr_alloc_buf(len-1, &ptr);
        if(!date_str)
            return E_OUTOFMEMORY;
        GetDateFormatW(ctx->lcid, DATE_LONGDATE, &st, NULL, ptr, len);

        *r = jsval_string(date_str);
    }
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.7 */
static HRESULT Date_toLocaleTimeString(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    SYSTEMTIME st;
    DateInstance *date;
    jsstr_t *date_str;
    int len;

    TRACE("\n");

    if(!(date = date_this(vthis)))
        return JS_E_DATE_EXPECTED;

    if(isnan(date->time)) {
        if(r)
            *r = jsval_string(jsstr_nan());
        return S_OK;
    }

    st = create_systemtime(local_time(date->time, date));

    if(st.wYear<1601 || st.wYear>9999)
        return Date_toTimeString(ctx, vthis, flags, argc, argv, r);

    if(r) {
        WCHAR *ptr;

        len = GetTimeFormatW(ctx->lcid, 0, &st, NULL, NULL, 0);
        date_str = jsstr_alloc_buf(len-1, &ptr);
        if(!date_str)
            return E_OUTOFMEMORY;
        GetTimeFormatW(ctx->lcid, 0, &st, NULL, ptr, len);

        *r = jsval_string(date_str);
    }
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.9 */
static HRESULT Date_getTime(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    DateInstance *date;

    TRACE("\n");

    if(!(date = date_this(vthis)))
        return JS_E_DATE_EXPECTED;

    if(r)
        *r = jsval_number(date->time);
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.10 */
static HRESULT Date_getFullYear(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    DateInstance *date;

    TRACE("\n");

    if(!(date = date_this(vthis)))
        return JS_E_DATE_EXPECTED;

    if(r) {
        DOUBLE time = local_time(date->time, date);

        *r = jsval_number(year_from_time(time));
    }
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.11 */
static HRESULT Date_getUTCFullYear(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    DateInstance *date;

    TRACE("\n");

    if(!(date = date_this(vthis)))
        return JS_E_DATE_EXPECTED;

    if(r)
        *r = jsval_number(year_from_time(date->time));
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.12 */
static HRESULT Date_getMonth(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv, jsval_t *r)
{
    DateInstance *date;

    TRACE("\n");

    if(!(date = date_this(vthis)))
        return JS_E_DATE_EXPECTED;

    if(r)
        *r = jsval_number(month_from_time(local_time(date->time, date)));
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.13 */
static HRESULT Date_getUTCMonth(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    DateInstance *date;

    TRACE("\n");

    if(!(date = date_this(vthis)))
        return JS_E_DATE_EXPECTED;

    if(r)
        *r = jsval_number(month_from_time(date->time));
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.14 */
static HRESULT Date_getDate(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv, jsval_t *r)
{
    DateInstance *date;

    TRACE("\n");

    if(!(date = date_this(vthis)))
        return JS_E_DATE_EXPECTED;

    if(r)
        *r = jsval_number(date_from_time(local_time(date->time, date)));
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.15 */
static HRESULT Date_getUTCDate(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    DateInstance *date;

    TRACE("\n");

    if(!(date = date_this(vthis)))
        return JS_E_DATE_EXPECTED;

    if(r)
        *r = jsval_number(date_from_time(date->time));
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.16 */
static HRESULT Date_getDay(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    DateInstance *date;

    TRACE("\n");

    if(!(date = date_this(vthis)))
        return JS_E_DATE_EXPECTED;

    if(r)
        *r = jsval_number(week_day(local_time(date->time, date)));
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.17 */
static HRESULT Date_getUTCDay(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    DateInstance *date;

    TRACE("\n");

    if(!(date = date_this(vthis)))
        return JS_E_DATE_EXPECTED;

    if(r)
        *r = jsval_number(week_day(date->time));
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.18 */
static HRESULT Date_getHours(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    DateInstance *date;

    TRACE("\n");

    if(!(date = date_this(vthis)))
        return JS_E_DATE_EXPECTED;

    if(r)
        *r = jsval_number(hour_from_time(local_time(date->time, date)));
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.19 */
static HRESULT Date_getUTCHours(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    DateInstance *date;

    TRACE("\n");

    if(!(date = date_this(vthis)))
        return JS_E_DATE_EXPECTED;

    if(r)
        *r = jsval_number(hour_from_time(date->time));
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.20 */
static HRESULT Date_getMinutes(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    DateInstance *date;

    TRACE("\n");

    if(!(date = date_this(vthis)))
        return JS_E_DATE_EXPECTED;

    if(r)
        *r = jsval_number(min_from_time(local_time(date->time, date)));
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.21 */
static HRESULT Date_getUTCMinutes(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    DateInstance *date;

    TRACE("\n");

    if(!(date = date_this(vthis)))
        return JS_E_DATE_EXPECTED;

    if(r)
        *r = jsval_number(min_from_time(date->time));
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.22 */
static HRESULT Date_getSeconds(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv, jsval_t *r)
{
    DateInstance *date;

    TRACE("\n");

    if(!(date = date_this(vthis)))
        return JS_E_DATE_EXPECTED;

    if(r)
        *r = jsval_number(sec_from_time(local_time(date->time, date)));
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.23 */
static HRESULT Date_getUTCSeconds(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    DateInstance *date;

    TRACE("\n");

    if(!(date = date_this(vthis)))
        return JS_E_DATE_EXPECTED;

    if(r)
        *r = jsval_number(sec_from_time(date->time));
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.24 */
static HRESULT Date_getMilliseconds(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    DateInstance *date;

    TRACE("\n");

    if(!(date = date_this(vthis)))
        return JS_E_DATE_EXPECTED;

    if(r)
        *r = jsval_number(ms_from_time(local_time(date->time, date)));
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.25 */
static HRESULT Date_getUTCMilliseconds(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    DateInstance *date;

    TRACE("\n");

    if(!(date = date_this(vthis)))
        return JS_E_DATE_EXPECTED;

    if(r)
        *r = jsval_number(ms_from_time(date->time));
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.26 */
static HRESULT Date_getTimezoneOffset(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    DateInstance *date;

    TRACE("\n");

    if(!(date = date_this(vthis)))
        return JS_E_DATE_EXPECTED;

    if(r)
        *r = jsval_number(floor((date->time-local_time(date->time, date))/MS_PER_MINUTE));
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.27 */
static HRESULT Date_setTime(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    double n;
    HRESULT hres;
    DateInstance *date;

    TRACE("\n");

    if(!(date = date_this(vthis)))
        return JS_E_DATE_EXPECTED;

    if(!argc)
        return JS_E_MISSING_ARG;

    hres = to_number(ctx, argv[0], &n);
    if(FAILED(hres))
        return hres;

    date->time = time_clip(n);

    if(r)
        *r = jsval_number(date->time);
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.28 */
static HRESULT Date_setMilliseconds(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    DateInstance *date;
    double n, t;
    HRESULT hres;

    TRACE("\n");

    if(!(date = date_this(vthis)))
        return JS_E_DATE_EXPECTED;

    if(!argc)
        return JS_E_MISSING_ARG;

    hres = to_number(ctx, argv[0], &n);
    if(FAILED(hres))
        return hres;

    t = local_time(date->time, date);
    t = make_date(day(t), make_time(hour_from_time(t), min_from_time(t),
                sec_from_time(t), n));
    date->time = time_clip(utc(t, date));

    if(r)
        *r = jsval_number(date->time);
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.29 */
static HRESULT Date_setUTCMilliseconds(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    DateInstance *date;
    double n, t;
    HRESULT hres;

    TRACE("\n");

    if(!(date = date_this(vthis)))
        return JS_E_DATE_EXPECTED;

    if(!argc)
        return JS_E_MISSING_ARG;

    hres = to_number(ctx, argv[0], &n);
    if(FAILED(hres))
        return hres;

    t = date->time;
    t = make_date(day(t), make_time(hour_from_time(t), min_from_time(t),
                sec_from_time(t), n));
    date->time = time_clip(t);

    if(r)
        *r = jsval_number(date->time);
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.30 */
static HRESULT Date_setSeconds(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    DateInstance *date;
    double t, sec, ms;
    HRESULT hres;

    TRACE("\n");

    if(!(date = date_this(vthis)))
        return JS_E_DATE_EXPECTED;

    if(!argc)
        return JS_E_MISSING_ARG;

    t = local_time(date->time, date);

    hres = to_number(ctx, argv[0], &sec);
    if(FAILED(hres))
        return hres;

    if(argc > 1) {
        hres = to_number(ctx, argv[1], &ms);
        if(FAILED(hres))
            return hres;
    }else {
        ms = ms_from_time(t);
    }

    t = make_date(day(t), make_time(hour_from_time(t),
                min_from_time(t), sec, ms));
    date->time = time_clip(utc(t, date));

    if(r)
        *r = jsval_number(date->time);
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.31 */
static HRESULT Date_setUTCSeconds(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    DateInstance *date;
    double t, sec, ms;
    HRESULT hres;

    TRACE("\n");

    if(!(date = date_this(vthis)))
        return JS_E_DATE_EXPECTED;

    if(!argc)
        return JS_E_MISSING_ARG;

    t = date->time;

    hres = to_number(ctx, argv[0], &sec);
    if(FAILED(hres))
        return hres;

    if(argc > 1) {
        hres = to_number(ctx, argv[1], &ms);
        if(FAILED(hres))
            return hres;
    }else {
        ms = ms_from_time(t);
    }

    t = make_date(day(t), make_time(hour_from_time(t),
                min_from_time(t), sec, ms));
    date->time = time_clip(t);

    if(r)
        *r = jsval_number(date->time);
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.33 */
static HRESULT Date_setMinutes(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    DateInstance *date;
    double t, min, sec, ms;
    HRESULT hres;

    TRACE("\n");

    if(!(date = date_this(vthis)))
        return JS_E_DATE_EXPECTED;

    if(!argc)
        return JS_E_MISSING_ARG;

    t = local_time(date->time, date);

    hres = to_number(ctx, argv[0], &min);
    if(FAILED(hres))
        return hres;

    if(argc > 1) {
        hres = to_number(ctx, argv[1], &sec);
        if(FAILED(hres))
            return hres;
    }else {
        sec = sec_from_time(t);
    }

    if(argc > 2) {
        hres = to_number(ctx, argv[2], &ms);
        if(FAILED(hres))
            return hres;
    }else {
        ms = ms_from_time(t);
    }

    t = make_date(day(t), make_time(hour_from_time(t),
                min, sec, ms));
    date->time = time_clip(utc(t, date));

    if(r)
        *r = jsval_number(date->time);
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.34 */
static HRESULT Date_setUTCMinutes(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    DateInstance *date;
    double t, min, sec, ms;
    HRESULT hres;

    TRACE("\n");

    if(!(date = date_this(vthis)))
        return JS_E_DATE_EXPECTED;

    if(!argc)
        return JS_E_MISSING_ARG;

    t = date->time;

    hres = to_number(ctx, argv[0], &min);
    if(FAILED(hres))
        return hres;

    if(argc > 1) {
        hres = to_number(ctx, argv[1], &sec);
        if(FAILED(hres))
            return hres;
    }else {
        sec = sec_from_time(t);
    }

    if(argc > 2) {
        hres = to_number(ctx, argv[2], &ms);
        if(FAILED(hres))
            return hres;
    }else {
        ms = ms_from_time(t);
    }

    t = make_date(day(t), make_time(hour_from_time(t),
                min, sec, ms));
    date->time = time_clip(t);

    if(r)
        *r = jsval_number(date->time);
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.35 */
static HRESULT Date_setHours(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    DateInstance *date;
    double t, hour, min, sec, ms;
    HRESULT hres;

    TRACE("\n");

    if(!(date = date_this(vthis)))
        return JS_E_DATE_EXPECTED;

    if(!argc)
        return JS_E_MISSING_ARG;

    t = local_time(date->time, date);

    hres = to_number(ctx, argv[0], &hour);
    if(FAILED(hres))
        return hres;

    if(argc > 1) {
        hres = to_number(ctx, argv[1], &min);
        if(FAILED(hres))
            return hres;
    }else {
        min = min_from_time(t);
    }

    if(argc > 2) {
        hres = to_number(ctx, argv[2], &sec);
        if(FAILED(hres))
            return hres;
    }else {
        sec = sec_from_time(t);
    }

    if(argc > 3) {
        hres = to_number(ctx, argv[3], &ms);
        if(FAILED(hres))
            return hres;
    }else {
        ms = ms_from_time(t);
    }

    t = make_date(day(t), make_time(hour, min, sec, ms));
    date->time = time_clip(utc(t, date));

    if(r)
        *r = jsval_number(date->time);
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.36 */
static HRESULT Date_setUTCHours(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    DateInstance *date;
    double t, hour, min, sec, ms;
    HRESULT hres;

    TRACE("\n");

    if(!(date = date_this(vthis)))
        return JS_E_DATE_EXPECTED;

    if(!argc)
        return JS_E_MISSING_ARG;

    t = date->time;

    hres = to_number(ctx, argv[0], &hour);
    if(FAILED(hres))
        return hres;

    if(argc > 1) {
        hres = to_number(ctx, argv[1], &min);
        if(FAILED(hres))
            return hres;
    }else {
        min = min_from_time(t);
    }

    if(argc > 2) {
        hres = to_number(ctx, argv[2], &sec);
        if(FAILED(hres))
            return hres;
    }else {
        sec = sec_from_time(t);
    }

    if(argc > 3) {
        hres = to_number(ctx, argv[3], &ms);
        if(FAILED(hres))
            return hres;
    }else {
        ms = ms_from_time(t);
    }

    t = make_date(day(t), make_time(hour, min, sec, ms));
    date->time = time_clip(t);

    if(r)
        *r = jsval_number(date->time);
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.36 */
static HRESULT Date_setDate(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    DateInstance *date;
    double t, n;
    HRESULT hres;

    TRACE("\n");

    if(!(date = date_this(vthis)))
        return JS_E_DATE_EXPECTED;

    if(!argc)
        return JS_E_MISSING_ARG;

    hres = to_number(ctx, argv[0], &n);
    if(FAILED(hres))
        return hres;

    t = local_time(date->time, date);
    t = make_date(make_day(year_from_time(t), month_from_time(t), n), time_within_day(t));
    date->time = time_clip(utc(t, date));

    if(r)
        *r = jsval_number(date->time);
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.37 */
static HRESULT Date_setUTCDate(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    DateInstance *date;
    double t, n;
    HRESULT hres;

    TRACE("\n");

    if(!(date = date_this(vthis)))
        return JS_E_DATE_EXPECTED;

    if(!argc)
        return JS_E_MISSING_ARG;

    hres = to_number(ctx, argv[0], &n);
    if(FAILED(hres))
        return hres;

    t = date->time;
    t = make_date(make_day(year_from_time(t), month_from_time(t), n), time_within_day(t));
    date->time = time_clip(t);

    if(r)
        *r = jsval_number(date->time);
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.38 */
static HRESULT Date_setMonth(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    DateInstance *date;
    DOUBLE t, month, ddate;
    HRESULT hres;

    TRACE("\n");

    if(!(date = date_this(vthis)))
        return JS_E_DATE_EXPECTED;

    if(!argc)
        return JS_E_MISSING_ARG;

    t = local_time(date->time, date);

    hres = to_number(ctx, argv[0], &month);
    if(FAILED(hres))
        return hres;

    if(argc > 1) {
        hres = to_number(ctx, argv[1], &ddate);
        if(FAILED(hres))
            return hres;
    }else {
        ddate = date_from_time(t);
    }

    t = make_date(make_day(year_from_time(t), month, ddate),
            time_within_day(t));
    date->time = time_clip(utc(t, date));

    if(r)
        *r = jsval_number(date->time);
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.39 */
static HRESULT Date_setUTCMonth(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    DateInstance *date;
    double t, month, ddate;
    HRESULT hres;

    TRACE("\n");

    if(!(date = date_this(vthis)))
        return JS_E_DATE_EXPECTED;

    if(!argc)
        return JS_E_MISSING_ARG;

    t = date->time;

    hres = to_number(ctx, argv[0], &month);
    if(FAILED(hres))
        return hres;

    if(argc > 1) {
        hres = to_number(ctx, argv[1], &ddate);
        if(FAILED(hres))
            return hres;
    }else {
        ddate = date_from_time(t);
    }

    t = make_date(make_day(year_from_time(t), month, ddate),
            time_within_day(t));
    date->time = time_clip(t);

    if(r)
        *r = jsval_number(date->time);
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.40 */
static HRESULT Date_setFullYear(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    DateInstance *date;
    double t, year, month, ddate;
    HRESULT hres;

    TRACE("\n");

    if(!(date = date_this(vthis)))
        return JS_E_DATE_EXPECTED;

    if(!argc)
        return JS_E_MISSING_ARG;

    t = local_time(date->time, date);

    hres = to_number(ctx, argv[0], &year);
    if(FAILED(hres))
        return hres;

    if(argc > 1) {
        hres = to_number(ctx, argv[1], &month);
        if(FAILED(hres))
            return hres;
    }else {
        month = month_from_time(t);
    }

    if(argc > 2) {
        hres = to_number(ctx, argv[2], &ddate);
        if(FAILED(hres))
            return hres;
    }else {
        ddate = date_from_time(t);
    }

    t = make_date(make_day(year, month, ddate), time_within_day(t));
    date->time = time_clip(utc(t, date));

    if(r)
        *r = jsval_number(date->time);
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.41 */
static HRESULT Date_setUTCFullYear(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    DateInstance *date;
    double t, year, month, ddate;
    HRESULT hres;

    TRACE("\n");

    if(!(date = date_this(vthis)))
        return JS_E_DATE_EXPECTED;

    if(!argc)
        return JS_E_MISSING_ARG;

    t = date->time;

    hres = to_number(ctx, argv[0], &year);
    if(FAILED(hres))
        return hres;

    if(argc > 1) {
        hres = to_number(ctx, argv[1], &month);
        if(FAILED(hres))
            return hres;
    }else {
        month = month_from_time(t);
    }

    if(argc > 2) {
        hres = to_number(ctx, argv[2], &ddate);
        if(FAILED(hres))
            return hres;
    }else {
        ddate = date_from_time(t);
    }

    t = make_date(make_day(year, month, ddate), time_within_day(t));
    date->time = time_clip(t);

    if(r)
        *r = jsval_number(date->time);
    return S_OK;
}

/* ECMA-262 3rd Edition    B2.4 */
static HRESULT Date_getYear(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    DateInstance *date;
    DOUBLE t, year;

    TRACE("\n");

    if(!(date = date_this(vthis)))
        return JS_E_DATE_EXPECTED;

    t = local_time(date->time, date);
    if(isnan(t)) {
        if(r)
            *r = jsval_number(NAN);
        return S_OK;
    }

    year = year_from_time(t);
    if(r)
        *r = jsval_number((1900<=year && year<2000)?year-1900:year);
    return S_OK;
}

/* ECMA-262 3rd Edition    B2.5 */
static HRESULT Date_setYear(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    DateInstance *date;
    DOUBLE t, year;
    HRESULT hres;

    TRACE("\n");

    if(!(date = date_this(vthis)))
        return JS_E_DATE_EXPECTED;

    if(!argc)
        return JS_E_MISSING_ARG;

    t = local_time(date->time, date);

    hres = to_number(ctx, argv[0], &year);
    if(FAILED(hres))
        return hres;

    if(isnan(year)) {
        date->time = year;
        if(r)
            *r = jsval_number(NAN);
        return S_OK;
    }

    year = year >= 0.0 ? floor(year) : -floor(-year);
    if(-1.0 < year && year < 100.0)
        year += 1900.0;

    date->time = time_clip(utc(make_date(make_day(year, month_from_time(t), date_from_time(t)), time_within_day(t)), date));

    if(r)
        *r = jsval_number(date->time);
    return S_OK;
}

static const builtin_prop_t Date_props[] = {
    {L"getDate",             Date_getDate,               PROPF_METHOD},
    {L"getDay",              Date_getDay,                PROPF_METHOD},
    {L"getFullYear",         Date_getFullYear,           PROPF_METHOD},
    {L"getHours",            Date_getHours,              PROPF_METHOD},
    {L"getMilliseconds",     Date_getMilliseconds,       PROPF_METHOD},
    {L"getMinutes",          Date_getMinutes,            PROPF_METHOD},
    {L"getMonth",            Date_getMonth,              PROPF_METHOD},
    {L"getSeconds",          Date_getSeconds,            PROPF_METHOD},
    {L"getTime",             Date_getTime,               PROPF_METHOD},
    {L"getTimezoneOffset",   Date_getTimezoneOffset,     PROPF_METHOD},
    {L"getUTCDate",          Date_getUTCDate,            PROPF_METHOD},
    {L"getUTCDay",           Date_getUTCDay,             PROPF_METHOD},
    {L"getUTCFullYear",      Date_getUTCFullYear,        PROPF_METHOD},
    {L"getUTCHours",         Date_getUTCHours,           PROPF_METHOD},
    {L"getUTCMilliseconds",  Date_getUTCMilliseconds,    PROPF_METHOD},
    {L"getUTCMinutes",       Date_getUTCMinutes,         PROPF_METHOD},
    {L"getUTCMonth",         Date_getUTCMonth,           PROPF_METHOD},
    {L"getUTCSeconds",       Date_getUTCSeconds,         PROPF_METHOD},
    {L"getYear",             Date_getYear,               PROPF_METHOD},
    {L"setDate",             Date_setDate,               PROPF_METHOD|1},
    {L"setFullYear",         Date_setFullYear,           PROPF_METHOD|3},
    {L"setHours",            Date_setHours,              PROPF_METHOD|4},
    {L"setMilliseconds",     Date_setMilliseconds,       PROPF_METHOD|1},
    {L"setMinutes",          Date_setMinutes,            PROPF_METHOD|3},
    {L"setMonth",            Date_setMonth,              PROPF_METHOD|2},
    {L"setSeconds",          Date_setSeconds,            PROPF_METHOD|2},
    {L"setTime",             Date_setTime,               PROPF_METHOD|1},
    {L"setUTCDate",          Date_setUTCDate,            PROPF_METHOD|1},
    {L"setUTCFullYear",      Date_setUTCFullYear,        PROPF_METHOD|3},
    {L"setUTCHours",         Date_setUTCHours,           PROPF_METHOD|4},
    {L"setUTCMilliseconds",  Date_setUTCMilliseconds,    PROPF_METHOD|1},
    {L"setUTCMinutes",       Date_setUTCMinutes,         PROPF_METHOD|3},
    {L"setUTCMonth",         Date_setUTCMonth,           PROPF_METHOD|2},
    {L"setUTCSeconds",       Date_setUTCSeconds,         PROPF_METHOD|2},
    {L"setYear",             Date_setYear,               PROPF_METHOD|1},
    {L"toDateString",        Date_toDateString,          PROPF_METHOD},
    {L"toGMTString",         Date_toGMTString,           PROPF_METHOD},
    {L"toISOString",         Date_toISOString,           PROPF_METHOD|PROPF_ES5},
    {L"toLocaleDateString",  Date_toLocaleDateString,    PROPF_METHOD},
    {L"toLocaleString",      Date_toLocaleString,        PROPF_METHOD},
    {L"toLocaleTimeString",  Date_toLocaleTimeString,    PROPF_METHOD},
    {L"toString",            Date_toString,              PROPF_METHOD},
    {L"toTimeString",        Date_toTimeString,          PROPF_METHOD},
    {L"toUTCString",         Date_toUTCString,           PROPF_METHOD},
    {L"valueOf",             Date_valueOf,               PROPF_METHOD},
};

static const builtin_info_t Date_info = {
    .class     = JSCLASS_DATE,
    .props_cnt = ARRAY_SIZE(Date_props),
    .props     = Date_props,
};

static const builtin_info_t DateInst_info = {
    .class = JSCLASS_DATE,
};

static HRESULT create_date(script_ctx_t *ctx, jsdisp_t *object_prototype, DOUBLE time, DateInstance **ret)
{
    DateInstance *date;
    HRESULT hres;
    TIME_ZONE_INFORMATION tzi;

    GetTimeZoneInformation(&tzi);

    date = calloc(1, sizeof(DateInstance));
    if(!date)
        return E_OUTOFMEMORY;

    if(object_prototype)
        hres = init_dispex(&date->dispex, ctx, &Date_info, object_prototype);
    else
        hres = init_dispex_from_constr(&date->dispex, ctx, &DateInst_info, ctx->date_constr);
    if(FAILED(hres)) {
        free(date);
        return hres;
    }

    date->time = time;
    date->bias = tzi.Bias;
    date->standardDate = tzi.StandardDate;
    date->standardBias = tzi.StandardBias;
    date->daylightDate = tzi.DaylightDate;
    date->daylightBias = tzi.DaylightBias;

    *ret = date;
    return S_OK;
}

static inline HRESULT date_parse(jsstr_t *input_str, double *ret) {
    static const DWORD string_ids[] = { LOCALE_SMONTHNAME12, LOCALE_SMONTHNAME11,
        LOCALE_SMONTHNAME10, LOCALE_SMONTHNAME9, LOCALE_SMONTHNAME8,
        LOCALE_SMONTHNAME7, LOCALE_SMONTHNAME6, LOCALE_SMONTHNAME5,
        LOCALE_SMONTHNAME4, LOCALE_SMONTHNAME3, LOCALE_SMONTHNAME2,
        LOCALE_SMONTHNAME1, LOCALE_SDAYNAME7, LOCALE_SDAYNAME1,
        LOCALE_SDAYNAME2, LOCALE_SDAYNAME3, LOCALE_SDAYNAME4,
        LOCALE_SDAYNAME5, LOCALE_SDAYNAME6 };
    WCHAR *strings[ARRAY_SIZE(string_ids)];
    WCHAR *parse;
    int input_len, parse_len = 0, nest_level = 0, i, size;
    int year = 0, month = 0, day = 0, hour = 0, min = 0, sec = 0;
    int ms = 0, offset = 0, hour_adjust = 0;
    BOOL set_year = FALSE, set_month = FALSE, set_day = FALSE, set_hour = FALSE;
    BOOL set_offset = FALSE, set_era = FALSE, ad = TRUE, set_am = FALSE, am = TRUE;
    BOOL set_hour_adjust = TRUE;
    TIME_ZONE_INFORMATION tzi;
    const WCHAR *input;
    DateInstance di;
    DWORD lcid_en;

    input_len = jsstr_length(input_str);
    input = jsstr_flatten(input_str);
    if(!input)
        return E_OUTOFMEMORY;

    for(i=0; i<input_len; i++) {
        if(input[i] == '(') nest_level++;
        else if(input[i] == ')') {
            nest_level--;
            if(nest_level<0) {
                *ret = NAN;
                return S_OK;
            }
        }
        else if(!nest_level) parse_len++;
    }

    parse = malloc((parse_len+1)*sizeof(WCHAR));
    if(!parse)
        return E_OUTOFMEMORY;
    nest_level = 0;
    parse_len = 0;
    for(i=0; i<input_len; i++) {
        if(input[i] == '(') nest_level++;
        else if(input[i] == ')') nest_level--;
        else if(!nest_level) parse[parse_len++] = towupper(input[i]);
    }
    parse[parse_len] = 0;

    GetTimeZoneInformation(&tzi);
    di.bias = tzi.Bias;
    di.standardDate = tzi.StandardDate;
    di.standardBias = tzi.StandardBias;
    di.daylightDate = tzi.DaylightDate;
    di.daylightBias = tzi.DaylightBias;

    /* FIXME: Cache strings */
    lcid_en = MAKELCID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),SORT_DEFAULT);
    for(i=0; i<ARRAY_SIZE(string_ids); i++) {
        size = GetLocaleInfoW(lcid_en, string_ids[i], NULL, 0);
        strings[i] = malloc((size+1)*sizeof(WCHAR));
        if(!strings[i]) {
            i--;
            while(i-- >= 0)
                free(strings[i]);
            free(parse);
            return E_OUTOFMEMORY;
        }
        GetLocaleInfoW(lcid_en, string_ids[i], strings[i], size);
    }

    for(i=0; i<parse_len;) {
        while(iswspace(parse[i])) i++;
        if(parse[i] == ',') {
            while(parse[i] == ',') i++;
            continue;
        }

        if(parse[i]>='0' && parse[i]<='9') {
            int tmp = wcstol(&parse[i], NULL, 10);
            while(parse[i]>='0' && parse[i]<='9') i++;
            while(iswspace(parse[i])) i++;

            if(parse[i] == ':') {
                /* Time */
                if(set_hour) break;
                set_hour = TRUE;

                hour = tmp;

                while(parse[i] == ':') i++;
                while(iswspace(parse[i])) i++;
                if(parse[i]>='0' && parse[i]<='9') {
                    min = wcstol(&parse[i], NULL, 10);
                    while(parse[i]>='0' && parse[i]<='9') i++;
                }

                while(iswspace(parse[i])) i++;
                while(parse[i] == ':') i++;
                while(iswspace(parse[i])) i++;
                if(parse[i]>='0' && parse[i]<='9') {
                    sec = wcstol(&parse[i], NULL, 10);
                    while(parse[i]>='0' && parse[i]<='9') i++;
                }
            }
            else if(parse[i]=='-' || parse[i]=='/') {
                /* Short or long date */
                if(set_day || set_month || set_year) break;
                set_day = TRUE;
                set_month = TRUE;
                set_year = TRUE;

                month = tmp-1;

                while(iswspace(parse[i])) i++;
                while(parse[i]=='-' || parse[i]=='/') i++;
                while(iswspace(parse[i])) i++;
                if(parse[i]<'0' || parse[i]>'9') break;
                day = wcstol(&parse[i], NULL, 10);
                while(parse[i]>='0' && parse[i]<='9') i++;

                while(parse[i]=='-' || parse[i]=='/') i++;
                while(iswspace(parse[i])) i++;
                if(parse[i]<'0' || parse[i]>'9') break;
                year = wcstol(&parse[i], NULL, 10);
                while(parse[i]>='0' && parse[i]<='9') i++;

                if(tmp >= 70){
                        /* long date */
                        month = day - 1;
                        day = year;
                        year = tmp;
		}
            }
            else if(tmp<0) break;
            else if(tmp<70) {
                /* Day */
                if(set_day) break;
                set_day = TRUE;
                day = tmp;
            }
            else {
                /* Year */
                if(set_year) break;
                set_year = TRUE;
                year = tmp;
            }
        }
        else if(parse[i]=='+' || parse[i]=='-') {
            /* Timezone offset */
            BOOL positive = TRUE;

            if(set_offset && set_hour_adjust) break;
            set_offset = TRUE;
            set_hour_adjust = FALSE;

            if(parse[i] == '-')  positive = FALSE;

            i++;
            while(iswspace(parse[i])) i++;
            if(parse[i]<'0' || parse[i]>'9') break;
            offset = wcstol(&parse[i], NULL, 10);
            while(parse[i]>='0' && parse[i]<='9') i++;

            if(offset<24) offset *= 60;
            else offset = (offset/100)*60 + offset%100;

            if(positive) offset = -offset;

        }
        else {
            if(parse[i]<'A' || parse[i]>'Z') break;
            else if(parse[i]=='B' && (parse[i+1]=='C' ||
                        (parse[i+1]=='.' && parse[i+2]=='C'))) {
                /* AD/BC */
                if(set_era) break;
                set_era = TRUE;
                ad = FALSE;

                i++;
                if(parse[i] == '.') i++;
                i++;
                if(parse[i] == '.') i++;
            }
            else if(parse[i]=='A' && (parse[i+1]=='D' ||
                        (parse[i+1]=='.' && parse[i+2]=='D'))) {
                /* AD/BC */
                if(set_era) break;
                set_era = TRUE;

                i++;
                if(parse[i] == '.') i++;
                i++;
                if(parse[i] == '.') i++;
            }
            else if(parse[i+1]<'A' || parse[i+1]>'Z') {
                /* Timezone */
                if(set_offset) break;
                set_offset = TRUE;

                if(parse[i] <= 'I') hour_adjust = parse[i]-'A'+2;
                else if(parse[i] == 'J') break;
                else if(parse[i] <= 'M') hour_adjust = parse[i]-'K'+11;
                else if(parse[i] <= 'Y') hour_adjust = parse[i]-'N';
                else hour_adjust = 1;

                i++;
                if(parse[i] == '.') i++;
            }
            else if(parse[i]=='A' && parse[i+1]=='M') {
                /* AM/PM */
                if(set_am) break;
                set_am = TRUE;
                am = TRUE;
                i += 2;
            }
            else if(parse[i]=='P' && parse[i+1]=='M') {
                /* AM/PM */
                if(set_am) break;
                set_am = TRUE;
                am = FALSE;
                i += 2;
            }
            else if((parse[i]=='U' && parse[i+1]=='T' && parse[i+2]=='C')
                    || (parse[i]=='G' && parse[i+1]=='M' && parse[i+2]=='T')) {
                /* Timezone */
                if(set_offset) break;
                set_offset = TRUE;
                set_hour_adjust = FALSE;

                i += 3;
            }
            else {
                /* Month or garbage */
                unsigned int j;

                for(size=i; parse[size]>='A' && parse[size]<='Z'; size++);
                size -= i;

                for(j=0; j<ARRAY_SIZE(string_ids); j++)
                    if(!wcsnicmp(&parse[i], strings[j], size)) break;

                if(j < 12) {
                    if(set_month) break;
                    set_month = TRUE;
                    month = 11-j;
                }
                else if(j == ARRAY_SIZE(string_ids)) break;

                i += size;
            }
        }
    }

    if(i == parse_len && set_year && set_month && set_day && (!set_am || hour<13)) {
        if(set_am) {
            if(hour == 12) hour = 0;
            if(!am) hour += 12;
        }

        if(!ad) year = -year+1;
        else if(year<100) year += 1900;

        *ret = time_clip(make_date(make_day(year, month, day),
                    make_time(hour+hour_adjust, min, sec, ms)) + offset*MS_PER_MINUTE);

        if(set_hour_adjust)
            *ret = utc(*ret, &di);
    }else {
        *ret = NAN;
    }

    for(i=0; i<ARRAY_SIZE(string_ids); i++)
        free(strings[i]);
    free(parse);

    return S_OK;
}

static HRESULT DateConstr_parse(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsstr_t *parse_str;
    double n;
    HRESULT hres;

    TRACE("\n");

    if(!argc) {
        if(r)
            *r = jsval_number(NAN);
        return S_OK;
    }

    hres = to_string(ctx, argv[0], &parse_str);
    if(FAILED(hres))
        return hres;

    hres = date_parse(parse_str, &n);
    jsstr_release(parse_str);
    if(FAILED(hres))
        return hres;

    if(r)
        *r = jsval_number(n);
    return S_OK;
}

static HRESULT date_utc(script_ctx_t *ctx, unsigned argc, jsval_t *argv, double *ret)
{
    double year, month, vdate, hours, minutes, seconds, ms;
    HRESULT hres;

    TRACE("\n");

    if(argc) {
        hres = to_number(ctx, argv[0], &year);
        if(FAILED(hres))
            return hres;
        if(0 <= year && year <= 99)
            year += 1900;
    }else {
        year = 1900;
    }

    if(argc>1) {
        hres = to_number(ctx, argv[1], &month);
        if(FAILED(hres))
            return hres;
    }else {
        month = 0;
    }

    if(argc>2) {
        hres = to_number(ctx, argv[2], &vdate);
        if(FAILED(hres))
            return hres;
    }else {
        vdate = 1;
    }

    if(argc>3) {
        hres = to_number(ctx, argv[3], &hours);
        if(FAILED(hres))
            return hres;
    }else {
        hours = 0;
    }

    if(argc>4) {
        hres = to_number(ctx, argv[4], &minutes);
        if(FAILED(hres))
            return hres;
    }else {
        minutes = 0;
    }

    if(argc>5) {
        hres = to_number(ctx, argv[5], &seconds);
        if(FAILED(hres))
            return hres;
    }else {
        seconds = 0;
    }

    if(argc>6) {
        hres = to_number(ctx, argv[6], &ms);
        if(FAILED(hres))
            return hres;
    } else {
        ms = 0;
    }

    *ret = time_clip(make_date(make_day(year, month, vdate),
            make_time(hours, minutes,seconds, ms)));
    return S_OK;
}

static HRESULT DateConstr_UTC(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    double n;
    HRESULT hres;

    TRACE("\n");

    hres = date_utc(ctx, argc, argv, &n);
    if(SUCCEEDED(hres) && r)
        *r = jsval_number(n);
    return hres;
}

/* ECMA-262 5.1 Edition    15.9.4.4 */
static HRESULT DateConstr_now(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv, jsval_t *r)
{
    TRACE("\n");

    if(r) *r = jsval_number(date_now());
    return S_OK;
}

static HRESULT DateConstr_value(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    DateInstance *date;
    HRESULT hres;

    TRACE("\n");

    switch(flags) {
    case DISPATCH_CONSTRUCT:
        switch(argc) {
        /* ECMA-262 3rd Edition    15.9.3.3 */
        case 0:
            hres = create_date(ctx, NULL, date_now(), &date);
            if(FAILED(hres))
                return hres;
            break;

        /* ECMA-262 3rd Edition    15.9.3.2 */
        case 1: {
            jsval_t prim;
            double n;

            hres = to_primitive(ctx, argv[0], &prim, NO_HINT);
            if(FAILED(hres))
                return hres;

            if(is_string(prim))
                hres = date_parse(get_string(prim), &n);
            else
                hres = to_number(ctx, prim, &n);

            jsval_release(prim);
            if(FAILED(hres))
                return hres;

            hres = create_date(ctx, NULL, time_clip(n), &date);
            if(FAILED(hres))
                return hres;
            break;
        }

        /* ECMA-262 3rd Edition    15.9.3.1 */
        default: {
            double ret_date;

            hres = date_utc(ctx, argc, argv, &ret_date);
            if(FAILED(hres))
                return hres;

            hres = create_date(ctx, NULL, ret_date, &date);
            if(FAILED(hres))
                return hres;

            date->time = utc(date->time, date);
        }
        }

        if(r) *r = jsval_obj(&date->dispex);
        else  jsdisp_release(&date->dispex);
        return S_OK;

    case INVOKE_FUNC: {
        FILETIME system_time, local_time;

        GetSystemTimeAsFileTime(&system_time);
        FileTimeToLocalFileTime(&system_time, &local_time);

        return date_to_string(file_time_to_date_time(&local_time), FALSE, 0, r);
    }

    default:
        FIXME("unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

static const builtin_prop_t DateConstr_props[] = {
    {L"UTC",    DateConstr_UTC,    PROPF_METHOD},
    {L"now",    DateConstr_now,    PROPF_HTML|PROPF_METHOD},
    {L"parse",  DateConstr_parse,  PROPF_METHOD}
};

static const builtin_info_t DateConstr_info = {
    .class     = JSCLASS_FUNCTION,
    .call      = Function_value,
    .props_cnt = ARRAY_SIZE(DateConstr_props),
    .props     = DateConstr_props,
};

HRESULT create_date_constr(script_ctx_t *ctx, jsdisp_t *object_prototype, jsdisp_t **ret)
{
    DateInstance *date;
    HRESULT hres;

    hres = create_date(ctx, object_prototype, 0.0, &date);
    if(FAILED(hres))
        return hres;

    hres = create_builtin_constructor(ctx, DateConstr_value, L"Date", &DateConstr_info,
            PROPF_CONSTR|7, &date->dispex, ret);

    jsdisp_release(&date->dispex);
    return hres;
}

HRESULT variant_date_to_number(double date, double *ret)
{
    SYSTEMTIME st;
    UDATE udate;
    HRESULT hres;

    hres = VarUdateFromDate(date, 0, &udate);
    if(FAILED(hres))
        return hres;

    if(!TzSpecificLocalTimeToSystemTime(NULL, &udate.st, &st))
        return E_FAIL;

    TRACE("%uy %um %u %ud %uh %um %u.%us\n", st.wYear, st.wMonth, st.wDayOfWeek, st.wDay, st.wHour, st.wMinute,
        st.wSecond, st.wMilliseconds);

    *ret = make_date(make_day(st.wYear, st.wMonth - 1, st.wDay),
                     make_time(st.wHour, st.wMinute, st.wSecond, st.wMilliseconds));
    return S_OK;
}

HRESULT variant_date_to_string(script_ctx_t *ctx, double date, jsstr_t **r)
{
    DateInstance *date_obj;
    jsval_t val;
    double time;
    HRESULT hres;

    hres = variant_date_to_number(date, &time);
    if(FAILED(hres))
        return hres;

    hres = create_date(ctx, NULL, time, &date_obj);
    if(FAILED(hres))
        return hres;

    hres = dateobj_to_string(date_obj, &val);
    jsdisp_release(&date_obj->dispex);
    if(FAILED(hres))
        return hres;

    assert(is_string(val));
    *r = get_string(val);
    return hres;
}
