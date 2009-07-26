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
    SYSTEMTIME standardDate;
    LONG standardBias;
    SYSTEMTIME daylightDate;
    LONG daylightBias;
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
static const WCHAR getMillisecondsW[] = {'g','e','t','M','i','l','l','i','s','e','c','o','n','d','s',0};
static const WCHAR getUTCMillisecondsW[] = {'g','e','t','U','T','C','M','i','l','l','i','s','e','c','o','n','d','s',0};
static const WCHAR getTimezoneOffsetW[] = {'g','e','t','T','i','m','e','z','o','n','e','O','f','f','s','e','t',0};
static const WCHAR setTimeW[] = {'s','e','t','T','i','m','e',0};
static const WCHAR setMillisecondsW[] = {'s','e','t','M','i','l','l','i','s','e','c','o','n','d','s',0};
static const WCHAR setUTCMillisecondsW[] = {'s','e','t','U','T','C','M','i','l','l','i','s','e','c','o','n','d','s',0};
static const WCHAR setSecondsW[] = {'s','e','t','S','e','c','o','n','d','s',0};
static const WCHAR setUTCSecondsW[] = {'s','e','t','U','T','C','S','e','c','o','n','d','s',0};
static const WCHAR setMinutesW[] = {'s','e','t','M','i','n','u','t','e','s',0};
static const WCHAR setUTCMinutesW[] = {'s','e','t','U','T','C','M','i','n','u','t','e','s',0};
static const WCHAR setHoursW[] = {'s','e','t','H','o','u','r','s',0};
static const WCHAR setUTCHoursW[] = {'s','e','t','U','T','C','H','o','u','r','s',0};
static const WCHAR setDateW[] = {'s','e','t','D','a','t','e',0};
static const WCHAR setUTCDateW[] = {'s','e','t','U','T','C','D','a','t','e',0};
static const WCHAR setMonthW[] = {'s','e','t','M','o','n','t','h',0};
static const WCHAR setUTCMonthW[] = {'s','e','t','U','T','C','M','o','n','t','h',0};
static const WCHAR setFullYearW[] = {'s','e','t','F','u','l','l','Y','e','a','r',0};
static const WCHAR setUTCFullYearW[] = {'s','e','t','U','T','C','F','u','l','l','Y','e','a','r',0};

static const WCHAR UTCW[] = {'U','T','C',0};
static const WCHAR parseW[] = {'p','a','r','s','e',0};

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
        return ret_nan();

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
        return ret_nan();

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
        return ret_nan();

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
        return ret_nan();

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
        return ret_nan();

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
        return ret_nan();

    ret = fmod(day(time)+4, 7);
    if(ret<0) ret += 7;

    return ret;
}

static inline DOUBLE convert_time(int year, SYSTEMTIME st)
{
    DOUBLE time;
    int set_week_day;

    if(st.wMonth == 0)
        return ret_nan();

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
        return ret_nan();

    ret = fmod(floor(time/MS_PER_HOUR), 24);
    if(ret<0) ret += 24;

    return ret;
}

/* ECMA-262 3rd Edition    15.9.1.10 */
static inline DOUBLE min_from_time(DOUBLE time)
{
    DOUBLE ret;

    if(isnan(time))
        return ret_nan();

    ret = fmod(floor(time/MS_PER_MINUTE), 60);
    if(ret<0) ret += 60;

    return ret;
}

/* ECMA-262 3rd Edition    15.9.1.10 */
static inline DOUBLE sec_from_time(DOUBLE time)
{
    DOUBLE ret;

    if(isnan(time))
        return ret_nan();

    ret = fmod(floor(time/1000), 60);
    if(ret<0) ret += 60;

    return ret;
}

/* ECMA-262 3rd Edition    15.9.1.10 */
static inline DOUBLE ms_from_time(DOUBLE time)
{
    DOUBLE ret;

    if(isnan(time))
        return ret_nan();

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
        return ret_nan();
    }

    return floor(time);
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

/* ECMA-262 3rd Edition    15.9.1.2 */
static HRESULT Date_toString(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    static const WCHAR NaNW[] = { 'N','a','N',0 };
    static const WCHAR formatW[] = { '%','s',' ','%','s',' ','%','d',' ',
        '%','0','2','d',':','%','0','2','d',':','%','0','2','d',' ',
        'U','T','C','%','c','%','0','2','d','%','0','2','d',' ','%','d','%','s',0 };
    static const WCHAR formatUTCW[] = { '%','s',' ','%','s',' ','%','d',' ',
        '%','0','2','d',':','%','0','2','d',':','%','0','2','d',' ',
        'U','T','C',' ','%','d','%','s',0 };
    static const WCHAR ADW[] = { 0 };
    static const WCHAR BCW[] = { ' ','B','.','C','.',0 };

    static const DWORD week_ids[] = { LOCALE_SABBREVDAYNAME7, LOCALE_SABBREVDAYNAME1,
        LOCALE_SABBREVDAYNAME2, LOCALE_SABBREVDAYNAME3, LOCALE_SABBREVDAYNAME4,
        LOCALE_SABBREVDAYNAME5, LOCALE_SABBREVDAYNAME6 };
    static const DWORD month_ids[] = { LOCALE_SABBREVMONTHNAME1, LOCALE_SABBREVMONTHNAME2,
        LOCALE_SABBREVMONTHNAME3, LOCALE_SABBREVMONTHNAME4,
        LOCALE_SABBREVMONTHNAME5, LOCALE_SABBREVMONTHNAME6,
        LOCALE_SABBREVMONTHNAME7, LOCALE_SABBREVMONTHNAME8,
        LOCALE_SABBREVMONTHNAME9, LOCALE_SABBREVMONTHNAME10,
        LOCALE_SABBREVMONTHNAME11, LOCALE_SABBREVMONTHNAME12 };

    BOOL formatAD = TRUE;
    BSTR week, month;
    DateInstance *date;
    BSTR date_str;
    DOUBLE time;
    int len, size, year, day, offset;
    DWORD lcid_en, week_id, month_id;
    WCHAR sign = '-';

    TRACE("\n");

    if(!is_class(dispex, JSCLASS_DATE)) {
        FIXME("throw TypeError\n");
        return E_FAIL;
    }

    date = (DateInstance*)dispex;

    if(isnan(date->time)) {
        if(retv) {
            V_VT(retv) = VT_BSTR;
            V_BSTR(retv) = SysAllocString(NaNW);
            if(!V_BSTR(retv))
                return E_OUTOFMEMORY;
        }
        return S_OK;
    }

    time = local_time(date->time, date);

    if(retv) {
        len = 21;

        lcid_en = MAKELCID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),SORT_DEFAULT);

        week_id = week_ids[(int)week_day(time)];
        size = GetLocaleInfoW(lcid_en, week_id, NULL, 0);
        week = SysAllocStringLen(NULL, size);
        if(!week)
            return E_OUTOFMEMORY;
        GetLocaleInfoW(lcid_en, week_id, week, size);
        len += size-1;

        month_id = month_ids[(int)month_from_time(time)];
        size = GetLocaleInfoW(lcid_en, month_id, NULL, 0);
        month = SysAllocStringLen(NULL, size);
        if(!month) {
            SysFreeString(week);
            return E_OUTOFMEMORY;
        }
        GetLocaleInfoW(lcid_en, month_id, month, size);
        len += size-1;

        year = year_from_time(time);
        if(year<0)
            year = -year+1;
        do {
            year /= 10;
            len++;
        } while(year);

        year = year_from_time(time);
        if(year<0) {
            formatAD = FALSE;
            year = -year+1;
            len += 5;
        }

        day = date_from_time(time);
        do {
            day /= 10;
            len++;
        } while(day);
        day = date_from_time(time);

        offset = date->bias +
            daylight_saving_ta(time, date);

        if(offset == 0) len -= 5;
        else if(offset < 0) {
            sign = '+';
            offset = -offset;
        }

        date_str = SysAllocStringLen(NULL, len);
        if(!date_str) {
            SysFreeString(week);
            SysFreeString(month);
            return E_OUTOFMEMORY;
        }

        if(offset)
            sprintfW(date_str, formatW, week, month, day,
                    (int)hour_from_time(time), (int)min_from_time(time),
                    (int)sec_from_time(time), sign, offset/60, offset%60,
                    year, formatAD?ADW:BCW);
        else
            sprintfW(date_str, formatUTCW, week, month, day,
            (int)hour_from_time(time), (int)min_from_time(time),
            (int)sec_from_time(time), year, formatAD?ADW:BCW);

        SysFreeString(week);
        SysFreeString(month);

        V_VT(retv) = VT_BSTR;
        V_BSTR(retv) = date_str;
    }
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.1.5 */
static HRESULT Date_toLocaleString(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    static const WCHAR NaNW[] = { 'N','a','N',0 };
    SYSTEMTIME st;
    DateInstance *date;
    BSTR date_str;
    int date_len, time_len;

    TRACE("\n");

    if(!is_class(dispex, JSCLASS_DATE)) {
        FIXME("throw TypeError\n");
        return E_FAIL;
    }

    date = (DateInstance*)dispex;

    if(isnan(date->time)) {
        if(retv) {
            V_VT(retv) = VT_BSTR;
            V_BSTR(retv) = SysAllocString(NaNW);
            if(!V_BSTR(retv))
                return E_OUTOFMEMORY;
        }
        return S_OK;
    }

    st = create_systemtime(local_time(date->time, date));

    if(st.wYear<1601 || st.wYear>9999)
        return Date_toString(dispex, lcid, flags, dp, retv, ei, caller);

    if(retv) {
        date_len = GetDateFormatW(lcid, DATE_LONGDATE, &st, NULL, NULL, 0);
        time_len = GetTimeFormatW(lcid, 0, &st, NULL, NULL, 0);
        date_str = SysAllocStringLen(NULL, date_len+time_len-1);
        if(!date_str)
            return E_OUTOFMEMORY;
        GetDateFormatW(lcid, DATE_LONGDATE, &st, NULL, date_str, date_len);
        GetTimeFormatW(lcid, 0, &st, NULL, &date_str[date_len], time_len);
        date_str[date_len-1] = ' ';

        V_VT(retv) = VT_BSTR;
        V_BSTR(retv) = date_str;
    }
    return S_OK;
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

/* ECMA-262 3rd Edition    15.9.5.42 */
static HRESULT Date_toUTCString(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    static const WCHAR NaNW[] = { 'N','a','N',0 };
    static const WCHAR formatADW[] = { '%','s',',',' ','%','d',' ','%','s',' ','%','d',' ',
        '%','0','2','d',':','%','0','2','d',':','%','0','2','d',' ','U','T','C',0 };
    static const WCHAR formatBCW[] = { '%','s',',',' ','%','d',' ','%','s',' ','%','d',' ','B','.','C','.',' ',
        '%','0','2','d',':','%','0','2','d',':','%','0','2','d',' ','U','T','C',0 };

    static const DWORD week_ids[] = { LOCALE_SABBREVDAYNAME7, LOCALE_SABBREVDAYNAME1,
        LOCALE_SABBREVDAYNAME2, LOCALE_SABBREVDAYNAME3, LOCALE_SABBREVDAYNAME4,
        LOCALE_SABBREVDAYNAME5, LOCALE_SABBREVDAYNAME6 };
    static const DWORD month_ids[] = { LOCALE_SABBREVMONTHNAME1, LOCALE_SABBREVMONTHNAME2,
        LOCALE_SABBREVMONTHNAME3, LOCALE_SABBREVMONTHNAME4,
        LOCALE_SABBREVMONTHNAME5, LOCALE_SABBREVMONTHNAME6,
        LOCALE_SABBREVMONTHNAME7, LOCALE_SABBREVMONTHNAME8,
        LOCALE_SABBREVMONTHNAME9, LOCALE_SABBREVMONTHNAME10,
        LOCALE_SABBREVMONTHNAME11, LOCALE_SABBREVMONTHNAME12 };

    BOOL formatAD = TRUE;
    BSTR week, month;
    DateInstance *date;
    BSTR date_str;
    int len, size, year, day;
    DWORD lcid_en, week_id, month_id;

    TRACE("\n");

    if(!is_class(dispex, JSCLASS_DATE)) {
        FIXME("throw TypeError\n");
        return E_FAIL;
    }

    date = (DateInstance*)dispex;

    if(isnan(date->time)) {
        if(retv) {
            V_VT(retv) = VT_BSTR;
            V_BSTR(retv) = SysAllocString(NaNW);
            if(!V_BSTR(retv))
                return E_OUTOFMEMORY;
        }
        return S_OK;
    }

    if(retv) {
        len = 17;

        lcid_en = MAKELCID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),SORT_DEFAULT);

        week_id = week_ids[(int)week_day(date->time)];
        size = GetLocaleInfoW(lcid_en, week_id, NULL, 0);
        week = SysAllocStringLen(NULL, size);
        if(!week)
            return E_OUTOFMEMORY;
        GetLocaleInfoW(lcid_en, week_id, week, size);
        len += size-1;

        month_id = month_ids[(int)month_from_time(date->time)];
        size = GetLocaleInfoW(lcid_en, month_id, NULL, 0);
        month = SysAllocStringLen(NULL, size);
        if(!month) {
            SysFreeString(week);
            return E_OUTOFMEMORY;
        }
        GetLocaleInfoW(lcid_en, month_id, month, size);
        len += size-1;

        year = year_from_time(date->time);
        if(year<0)
            year = -year+1;
        do {
            year /= 10;
            len++;
        } while(year);

        year = year_from_time(date->time);
        if(year<0) {
            formatAD = FALSE;
            year = -year+1;
            len += 5;
        }

        day = date_from_time(date->time);
        do {
            day /= 10;
            len++;
        } while(day);
        day = date_from_time(date->time);

        date_str = SysAllocStringLen(NULL, len);
        if(!date_str) {
            SysFreeString(week);
            SysFreeString(month);
            return E_OUTOFMEMORY;
        }
        sprintfW(date_str, formatAD?formatADW:formatBCW, week, day, month, year,
                (int)hour_from_time(date->time), (int)min_from_time(date->time),
                (int)sec_from_time(date->time));

        SysFreeString(week);
        SysFreeString(month);

        V_VT(retv) = VT_BSTR;
        V_BSTR(retv) = date_str;
    }
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.3 */
static HRESULT Date_toDateString(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    static const WCHAR NaNW[] = { 'N','a','N',0 };
    static const WCHAR formatADW[] = { '%','s',' ','%','s',' ','%','d',' ','%','d',0 };
    static const WCHAR formatBCW[] = { '%','s',' ','%','s',' ','%','d',' ','%','d',' ','B','.','C','.',0 };

    static const DWORD week_ids[] = { LOCALE_SABBREVDAYNAME7, LOCALE_SABBREVDAYNAME1,
        LOCALE_SABBREVDAYNAME2, LOCALE_SABBREVDAYNAME3, LOCALE_SABBREVDAYNAME4,
        LOCALE_SABBREVDAYNAME5, LOCALE_SABBREVDAYNAME6 };
    static const DWORD month_ids[] = { LOCALE_SABBREVMONTHNAME1, LOCALE_SABBREVMONTHNAME2,
        LOCALE_SABBREVMONTHNAME3, LOCALE_SABBREVMONTHNAME4,
        LOCALE_SABBREVMONTHNAME5, LOCALE_SABBREVMONTHNAME6,
        LOCALE_SABBREVMONTHNAME7, LOCALE_SABBREVMONTHNAME8,
        LOCALE_SABBREVMONTHNAME9, LOCALE_SABBREVMONTHNAME10,
        LOCALE_SABBREVMONTHNAME11, LOCALE_SABBREVMONTHNAME12 };

    BOOL formatAD = TRUE;
    BSTR week, month;
    DateInstance *date;
    BSTR date_str;
    DOUBLE time;
    int len, size, year, day;
    DWORD lcid_en, week_id, month_id;

    TRACE("\n");

    if(!is_class(dispex, JSCLASS_DATE)) {
        FIXME("throw TypeError\n");
        return E_FAIL;
    }

    date = (DateInstance*)dispex;

    if(isnan(date->time)) {
        if(retv) {
            V_VT(retv) = VT_BSTR;
            V_BSTR(retv) = SysAllocString(NaNW);
            if(!V_BSTR(retv))
                return E_OUTOFMEMORY;
        }
        return S_OK;
    }

    time = local_time(date->time, date);

    if(retv) {
        len = 5;

        lcid_en = MAKELCID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),SORT_DEFAULT);

        week_id = week_ids[(int)week_day(time)];
        size = GetLocaleInfoW(lcid_en, week_id, NULL, 0);
        week = SysAllocStringLen(NULL, size);
        if(!week)
            return E_OUTOFMEMORY;
        GetLocaleInfoW(lcid_en, week_id, week, size);
        len += size-1;

        month_id = month_ids[(int)month_from_time(time)];
        size = GetLocaleInfoW(lcid_en, month_id, NULL, 0);
        month = SysAllocStringLen(NULL, size);
        if(!month) {
            SysFreeString(week);
            return E_OUTOFMEMORY;
        }
        GetLocaleInfoW(lcid_en, month_id, month, size);
        len += size-1;

        year = year_from_time(time);
        if(year<0)
            year = -year+1;
        do {
            year /= 10;
            len++;
        } while(year);

        year = year_from_time(time);
        if(year<0) {
            formatAD = FALSE;
            year = -year+1;
            len += 5;
        }

        day = date_from_time(time);
        do {
            day /= 10;
            len++;
        } while(day);
        day = date_from_time(time);

        date_str = SysAllocStringLen(NULL, len);
        if(!date_str) {
            SysFreeString(week);
            SysFreeString(month);
            return E_OUTOFMEMORY;
        }
        sprintfW(date_str, formatAD?formatADW:formatBCW, week, month, day, year);

        SysFreeString(week);
        SysFreeString(month);

        V_VT(retv) = VT_BSTR;
        V_BSTR(retv) = date_str;
    }
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.4 */
static HRESULT Date_toTimeString(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    static const WCHAR NaNW[] = { 'N','a','N',0 };
    static const WCHAR formatW[] = { '%','0','2','d',':','%','0','2','d',':','%','0','2','d',
        ' ','U','T','C','%','c','%','0','2','d','%','0','2','d',0 };
    static const WCHAR formatUTCW[] = { '%','0','2','d',':','%','0','2','d',
        ':','%','0','2','d',' ','U','T','C',0 };
    DateInstance *date;
    BSTR date_str;
    DOUBLE time;
    WCHAR sign;
    int offset;

    TRACE("\n");

    if(!is_class(dispex, JSCLASS_DATE)) {
        FIXME("throw TypeError\n");
        return E_FAIL;
    }

    date = (DateInstance*)dispex;

    if(isnan(date->time)) {
        if(retv) {
            V_VT(retv) = VT_BSTR;
            V_BSTR(retv) = SysAllocString(NaNW);
            if(!V_BSTR(retv))
                return E_OUTOFMEMORY;
        }
        return S_OK;
    }

    time = local_time(date->time, date);

    if(retv) {
        date_str = SysAllocStringLen(NULL, 17);
        if(!date_str)
            return E_OUTOFMEMORY;

        offset = date->bias +
            daylight_saving_ta(time, date);

        if(offset < 0) {
            sign = '+';
            offset = -offset;
        }
        else sign = '-';

        if(offset)
            sprintfW(date_str, formatW, (int)hour_from_time(time),
                    (int)min_from_time(time), (int)sec_from_time(time),
                    sign, offset/60, offset%60);
        else
            sprintfW(date_str, formatUTCW, (int)hour_from_time(time),
                    (int)min_from_time(time), (int)sec_from_time(time));

        V_VT(retv) = VT_BSTR;
        V_BSTR(retv) = date_str;
    }
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.6 */
static HRESULT Date_toLocaleDateString(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    static const WCHAR NaNW[] = { 'N','a','N',0 };
    SYSTEMTIME st;
    DateInstance *date;
    BSTR date_str;
    int len;

    TRACE("\n");

    if(!is_class(dispex, JSCLASS_DATE)) {
        FIXME("throw TypeError\n");
        return E_FAIL;
    }

    date = (DateInstance*)dispex;

    if(isnan(date->time)) {
        if(retv) {
            V_VT(retv) = VT_BSTR;
            V_BSTR(retv) = SysAllocString(NaNW);
            if(!V_BSTR(retv))
                return E_OUTOFMEMORY;
        }
        return S_OK;
    }

    st = create_systemtime(local_time(date->time, date));

    if(st.wYear<1601 || st.wYear>9999)
        return Date_toDateString(dispex, lcid, flags, dp, retv, ei, caller);

    if(retv) {
        len = GetDateFormatW(lcid, DATE_LONGDATE, &st, NULL, NULL, 0);
        date_str = SysAllocStringLen(NULL, len);
        if(!date_str)
            return E_OUTOFMEMORY;
        GetDateFormatW(lcid, DATE_LONGDATE, &st, NULL, date_str, len);

        V_VT(retv) = VT_BSTR;
        V_BSTR(retv) = date_str;
    }
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.7 */
static HRESULT Date_toLocaleTimeString(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    static const WCHAR NaNW[] = { 'N','a','N',0 };
    SYSTEMTIME st;
    DateInstance *date;
    BSTR date_str;
    int len;

    TRACE("\n");

    if(!is_class(dispex, JSCLASS_DATE)) {
        FIXME("throw TypeError\n");
        return E_FAIL;
    }

    date = (DateInstance*)dispex;

    if(isnan(date->time)) {
        if(retv) {
            V_VT(retv) = VT_BSTR;
            V_BSTR(retv) = SysAllocString(NaNW);
            if(!V_BSTR(retv))
                return E_OUTOFMEMORY;
        }
        return S_OK;
    }

    st = create_systemtime(local_time(date->time, date));

    if(st.wYear<1601 || st.wYear>9999)
        return Date_toTimeString(dispex, lcid, flags, dp, retv, ei, caller);

    if(retv) {
        len = GetTimeFormatW(lcid, 0, &st, NULL, NULL, 0);
        date_str = SysAllocStringLen(NULL, len);
        if(!date_str)
            return E_OUTOFMEMORY;
        GetTimeFormatW(lcid, 0, &st, NULL, date_str, len);

        V_VT(retv) = VT_BSTR;
        V_BSTR(retv) = date_str;
    }
    return S_OK;
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

/* ECMA-262 3rd Edition    15.9.5.10 */
static HRESULT Date_getFullYear(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    TRACE("\n");

    if(!is_class(dispex, JSCLASS_DATE)) {
        FIXME("throw TypeError\n");
        return E_FAIL;
    }

    if(retv) {
        DateInstance *date = (DateInstance*)dispex;
        DOUBLE time = local_time(date->time, date);

        num_set_val(retv, year_from_time(time));
    }
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.11 */
static HRESULT Date_getUTCFullYear(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    TRACE("\n");

    if(!is_class(dispex, JSCLASS_DATE)) {
        FIXME("throw TypeError\n");
        return E_FAIL;
    }

    if(retv) {
        DateInstance *date = (DateInstance*)dispex;
        num_set_val(retv, year_from_time(date->time));
    }
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.12 */
static HRESULT Date_getMonth(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    TRACE("\n");

    if(!is_class(dispex, JSCLASS_DATE)) {
        FIXME("throw TypeError\n");
        return E_FAIL;
    }

    if(retv) {
        DateInstance *date = (DateInstance*)dispex;
        DOUBLE time = local_time(date->time, date);

        num_set_val(retv, month_from_time(time));
    }
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.13 */
static HRESULT Date_getUTCMonth(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    TRACE("\n");

    if(!is_class(dispex, JSCLASS_DATE)) {
        FIXME("throw TypeError\n");
        return E_FAIL;
    }

    if(retv) {
        DateInstance *date = (DateInstance*)dispex;
        num_set_val(retv, month_from_time(date->time));
    }
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.14 */
static HRESULT Date_getDate(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    TRACE("\n");

    if(!is_class(dispex, JSCLASS_DATE)) {
        FIXME("throw TypeError\n");
        return E_FAIL;
    }

    if(retv) {
        DateInstance *date = (DateInstance*)dispex;
        DOUBLE time = local_time(date->time, date);

        num_set_val(retv, date_from_time(time));
    }
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.15 */
static HRESULT Date_getUTCDate(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    TRACE("\n");

    if(!is_class(dispex, JSCLASS_DATE)) {
        FIXME("throw TypeError\n");
        return E_FAIL;
    }

    if(retv) {
        DateInstance *date = (DateInstance*)dispex;
        num_set_val(retv, date_from_time(date->time));
    }
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.16 */
static HRESULT Date_getDay(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    TRACE("\n");

    if(!is_class(dispex, JSCLASS_DATE)) {
        FIXME("throw TypeError\n");
        return E_FAIL;
    }

    if(retv) {
        DateInstance *date = (DateInstance*)dispex;
        DOUBLE time = local_time(date->time, date);

        num_set_val(retv, week_day(time));
    }
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.17 */
static HRESULT Date_getUTCDay(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    TRACE("\n");

    if(!is_class(dispex, JSCLASS_DATE)) {
        FIXME("throw TypeError\n");
        return E_FAIL;
    }

    if(retv) {
        DateInstance *date = (DateInstance*)dispex;
        num_set_val(retv, week_day(date->time));
    }
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.18 */
static HRESULT Date_getHours(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    TRACE("\n");

    if(!is_class(dispex, JSCLASS_DATE)) {
        FIXME("throw TypeError\n");
        return E_FAIL;
    }

    if(retv) {
        DateInstance *date = (DateInstance*)dispex;
        DOUBLE time = local_time(date->time, date);

        num_set_val(retv, hour_from_time(time));
    }
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.19 */
static HRESULT Date_getUTCHours(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    TRACE("\n");

    if(!is_class(dispex, JSCLASS_DATE)) {
        FIXME("throw TypeError\n");
        return E_FAIL;
    }

    if(retv) {
        DateInstance *date = (DateInstance*)dispex;
        num_set_val(retv, hour_from_time(date->time));
    }
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.20 */
static HRESULT Date_getMinutes(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    TRACE("\n");

    if(!is_class(dispex, JSCLASS_DATE)) {
        FIXME("throw TypeError\n");
        return E_FAIL;
    }

    if(retv) {
        DateInstance *date = (DateInstance*)dispex;
        DOUBLE time = local_time(date->time, date);

        num_set_val(retv, min_from_time(time));
    }
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.21 */
static HRESULT Date_getUTCMinutes(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    TRACE("\n");

    if(!is_class(dispex, JSCLASS_DATE)) {
        FIXME("throw TypeError\n");
        return E_FAIL;
    }

    if(retv) {
        DateInstance *date = (DateInstance*)dispex;
        num_set_val(retv, min_from_time(date->time));
    }
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.22 */
static HRESULT Date_getSeconds(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    TRACE("\n");

    if(!is_class(dispex, JSCLASS_DATE)) {
        FIXME("throw TypeError\n");
        return E_FAIL;
    }

    if(retv) {
        DateInstance *date = (DateInstance*)dispex;
        DOUBLE time = local_time(date->time, date);

        num_set_val(retv, sec_from_time(time));
    }
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.23 */
static HRESULT Date_getUTCSeconds(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    TRACE("\n");

    if(!is_class(dispex, JSCLASS_DATE)) {
        FIXME("throw TypeError\n");
        return E_FAIL;
    }

    if(retv) {
        DateInstance *date = (DateInstance*)dispex;
        num_set_val(retv, sec_from_time(date->time));
    }
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.24 */
static HRESULT Date_getMilliseconds(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    TRACE("\n");

    if(!is_class(dispex, JSCLASS_DATE)) {
        FIXME("throw TypeError\n");
        return E_FAIL;
    }

    if(retv) {
        DateInstance *date = (DateInstance*)dispex;
        DOUBLE time = local_time(date->time, date);

        num_set_val(retv, ms_from_time(time));
    }
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.25 */
static HRESULT Date_getUTCMilliseconds(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    TRACE("\n");

    if(!is_class(dispex, JSCLASS_DATE)) {
        FIXME("throw TypeError\n");
        return E_FAIL;
    }

    if(retv) {
        DateInstance *date = (DateInstance*)dispex;
        num_set_val(retv, ms_from_time(date->time));
    }
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.26 */
static HRESULT Date_getTimezoneOffset(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    TRACE("\n");

    if(!is_class(dispex, JSCLASS_DATE)) {
        FIXME("throw TypeError\n");
        return E_FAIL;
    }

    if(retv) {
        DateInstance *date = (DateInstance*)dispex;
        num_set_val(retv, floor(
                    (date->time-local_time(date->time, date))/MS_PER_MINUTE));
    }
    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.27 */
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
        FIXME("throw ArgumentNotOptional\n");
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

/* ECMA-262 3rd Edition    15.9.5.28 */
static HRESULT Date_setMilliseconds(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    VARIANT v;
    HRESULT hres;
    DateInstance *date;
    DOUBLE t;

    TRACE("\n");

    if(!is_class(dispex, JSCLASS_DATE)) {
        FIXME("throw TypeError\n");
        return E_FAIL;
    }

    if(!arg_cnt(dp)) {
        FIXME("throw ArgumentNotOptional\n");
        if(retv) num_set_nan(retv);
        return S_OK;
    }

    hres = to_number(dispex->ctx, get_arg(dp, 0), ei, &v);
    if(FAILED(hres))
        return hres;

    date = (DateInstance*)dispex;
    t = local_time(date->time, date);
    t = make_date(day(t), make_time(hour_from_time(t), min_from_time(t),
                sec_from_time(t), num_val(&v)));
    date->time = time_clip(utc(t, date));

    if(retv)
        num_set_val(retv, date->time);

    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.29 */
static HRESULT Date_setUTCMilliseconds(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    VARIANT v;
    HRESULT hres;
    DateInstance *date;
    DOUBLE t;

    TRACE("\n");

    if(!is_class(dispex, JSCLASS_DATE)) {
        FIXME("throw TypeError\n");
        return E_FAIL;
    }

    if(!arg_cnt(dp)) {
        FIXME("throw ArgumentNotOptional\n");
        if(retv) num_set_nan(retv);
        return S_OK;
    }

    hres = to_number(dispex->ctx, get_arg(dp, 0), ei, &v);
    if(FAILED(hres))
        return hres;

    date = (DateInstance*)dispex;
    t = date->time;
    t = make_date(day(t), make_time(hour_from_time(t), min_from_time(t),
                sec_from_time(t), num_val(&v)));
    date->time = time_clip(t);

    if(retv)
        num_set_val(retv, date->time);

    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.30 */
static HRESULT Date_setSeconds(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    VARIANT v;
    HRESULT hres;
    DateInstance *date;
    DOUBLE t, sec, ms;

    TRACE("\n");

    if(!is_class(dispex, JSCLASS_DATE)) {
        FIXME("throw TypeError\n");
        return E_FAIL;
    }

    if(!arg_cnt(dp)) {
        FIXME("throw ArgumentNotOptional\n");
        if(retv) num_set_nan(retv);
        return S_OK;
    }

    date = (DateInstance*)dispex;
    t = local_time(date->time, date);

    hres = to_number(dispex->ctx, get_arg(dp, 0), ei, &v);
    if(FAILED(hres))
        return hres;
    sec = num_val(&v);

    if(arg_cnt(dp) > 1) {
        hres = to_number(dispex->ctx, get_arg(dp, 1), ei, &v);
        if(FAILED(hres))
            return hres;
        ms = num_val(&v);
    }
    else ms = ms_from_time(t);

    t = make_date(day(t), make_time(hour_from_time(t),
                min_from_time(t), sec, ms));
    date->time = time_clip(utc(t, date));

    if(retv)
        num_set_val(retv, date->time);

    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.31 */
static HRESULT Date_setUTCSeconds(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    VARIANT v;
    HRESULT hres;
    DateInstance *date;
    DOUBLE t, sec, ms;

    TRACE("\n");

    if(!is_class(dispex, JSCLASS_DATE)) {
        FIXME("throw TypeError\n");
        return E_FAIL;
    }

    if(!arg_cnt(dp)) {
        FIXME("throw ArgumentNotOptional\n");
        if(retv) num_set_nan(retv);
        return S_OK;
    }

    date = (DateInstance*)dispex;
    t = date->time;

    hres = to_number(dispex->ctx, get_arg(dp, 0), ei, &v);
    if(FAILED(hres))
        return hres;
    sec = num_val(&v);

    if(arg_cnt(dp) > 1) {
        hres = to_number(dispex->ctx, get_arg(dp, 1), ei, &v);
        if(FAILED(hres))
            return hres;
        ms = num_val(&v);
    }
    else ms = ms_from_time(t);

    t = make_date(day(t), make_time(hour_from_time(t),
                min_from_time(t), sec, ms));
    date->time = time_clip(t);

    if(retv)
        num_set_val(retv, date->time);

    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.33 */
static HRESULT Date_setMinutes(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    VARIANT v;
    HRESULT hres;
    DateInstance *date;
    DOUBLE t, min, sec, ms;

    TRACE("\n");

    if(!is_class(dispex, JSCLASS_DATE)) {
        FIXME("throw TypeError\n");
        return E_FAIL;
    }

    if(!arg_cnt(dp)) {
        FIXME("throw ArgumentNotOptional\n");
        if(retv) num_set_nan(retv);
        return S_OK;
    }

    date = (DateInstance*)dispex;
    t = local_time(date->time, date);

    hres = to_number(dispex->ctx, get_arg(dp, 0), ei, &v);
    if(FAILED(hres))
        return hres;
    min = num_val(&v);

    if(arg_cnt(dp) > 1) {
        hres = to_number(dispex->ctx, get_arg(dp, 1), ei, &v);
        if(FAILED(hres))
            return hres;
        sec = num_val(&v);
    }
    else sec = sec_from_time(t);

    if(arg_cnt(dp) > 2) {
        hres = to_number(dispex->ctx, get_arg(dp, 2), ei, &v);
        if(FAILED(hres))
            return hres;
        ms = num_val(&v);
    }
    else ms = ms_from_time(t);

    t = make_date(day(t), make_time(hour_from_time(t),
                min, sec, ms));
    date->time = time_clip(utc(t, date));

    if(retv)
        num_set_val(retv, date->time);

    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.34 */
static HRESULT Date_setUTCMinutes(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    VARIANT v;
    HRESULT hres;
    DateInstance *date;
    DOUBLE t, min, sec, ms;

    TRACE("\n");

    if(!is_class(dispex, JSCLASS_DATE)) {
        FIXME("throw TypeError\n");
        return E_FAIL;
    }

    if(!arg_cnt(dp)) {
        FIXME("throw ArgumentNotOptional\n");
        if(retv) num_set_nan(retv);
        return S_OK;
    }

    date = (DateInstance*)dispex;
    t = date->time;

    hres = to_number(dispex->ctx, get_arg(dp, 0), ei, &v);
    if(FAILED(hres))
        return hres;
    min = num_val(&v);

    if(arg_cnt(dp) > 1) {
        hres = to_number(dispex->ctx, get_arg(dp, 1), ei, &v);
        if(FAILED(hres))
            return hres;
        sec = num_val(&v);
    }
    else sec = sec_from_time(t);

    if(arg_cnt(dp) > 2) {
        hres = to_number(dispex->ctx, get_arg(dp, 2), ei, &v);
        if(FAILED(hres))
            return hres;
        ms = num_val(&v);
    }
    else ms = ms_from_time(t);

    t = make_date(day(t), make_time(hour_from_time(t),
                min, sec, ms));
    date->time = time_clip(t);

    if(retv)
        num_set_val(retv, date->time);

    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.35 */
static HRESULT Date_setHours(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    VARIANT v;
    HRESULT hres;
    DateInstance *date;
    DOUBLE t, hour, min, sec, ms;

    TRACE("\n");

    if(!is_class(dispex, JSCLASS_DATE)) {
        FIXME("throw TypeError\n");
        return E_FAIL;
    }

    if(!arg_cnt(dp)) {
        FIXME("throw ArgumentNotOptional\n");
        if(retv) num_set_nan(retv);
        return S_OK;
    }

    date = (DateInstance*)dispex;
    t = local_time(date->time, date);

    hres = to_number(dispex->ctx, get_arg(dp, 0), ei, &v);
    if(FAILED(hres))
        return hres;
    hour = num_val(&v);

    if(arg_cnt(dp) > 1) {
        hres = to_number(dispex->ctx, get_arg(dp, 1), ei, &v);
        if(FAILED(hres))
            return hres;
        min = num_val(&v);
    }
    else min = min_from_time(t);

    if(arg_cnt(dp) > 2) {
        hres = to_number(dispex->ctx, get_arg(dp, 2), ei, &v);
        if(FAILED(hres))
            return hres;
        sec = num_val(&v);
    }
    else sec = sec_from_time(t);

    if(arg_cnt(dp) > 3) {
        hres = to_number(dispex->ctx, get_arg(dp, 3), ei, &v);
        if(FAILED(hres))
            return hres;
        ms = num_val(&v);
    }
    else ms = ms_from_time(t);

    t = make_date(day(t), make_time(hour, min, sec, ms));
    date->time = time_clip(utc(t, date));

    if(retv)
        num_set_val(retv, date->time);

    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.36 */
static HRESULT Date_setUTCHours(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    VARIANT v;
    HRESULT hres;
    DateInstance *date;
    DOUBLE t, hour, min, sec, ms;

    TRACE("\n");

    if(!is_class(dispex, JSCLASS_DATE)) {
        FIXME("throw TypeError\n");
        return E_FAIL;
    }

    if(!arg_cnt(dp)) {
        FIXME("throw ArgumentNotOptional\n");
        if(retv) num_set_nan(retv);
        return S_OK;
    }

    date = (DateInstance*)dispex;
    t = date->time;

    hres = to_number(dispex->ctx, get_arg(dp, 0), ei, &v);
    if(FAILED(hres))
        return hres;
    hour = num_val(&v);

    if(arg_cnt(dp) > 1) {
        hres = to_number(dispex->ctx, get_arg(dp, 1), ei, &v);
        if(FAILED(hres))
            return hres;
        min = num_val(&v);
    }
    else min = min_from_time(t);

    if(arg_cnt(dp) > 2) {
        hres = to_number(dispex->ctx, get_arg(dp, 2), ei, &v);
        if(FAILED(hres))
            return hres;
        sec = num_val(&v);
    }
    else sec = sec_from_time(t);

    if(arg_cnt(dp) > 3) {
        hres = to_number(dispex->ctx, get_arg(dp, 3), ei, &v);
        if(FAILED(hres))
            return hres;
        ms = num_val(&v);
    }
    else ms = ms_from_time(t);

    t = make_date(day(t), make_time(hour, min, sec, ms));
    date->time = time_clip(t);

    if(retv)
        num_set_val(retv, date->time);

    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.36 */
static HRESULT Date_setDate(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    VARIANT v;
    HRESULT hres;
    DateInstance *date;
    DOUBLE t;

    TRACE("\n");

    if(!is_class(dispex, JSCLASS_DATE)) {
        FIXME("throw TypeError\n");
        return E_FAIL;
    }

    if(!arg_cnt(dp)) {
        FIXME("throw ArgumentNotOptional\n");
        if(retv) num_set_nan(retv);
        return S_OK;
    }

    hres = to_number(dispex->ctx, get_arg(dp, 0), ei, &v);
    if(FAILED(hres))
        return hres;

    date = (DateInstance*)dispex;
    t = local_time(date->time, date);
    t = make_date(make_day(year_from_time(t), month_from_time(t),
                num_val(&v)), time_within_day(t));
    date->time = time_clip(utc(t, date));

    if(retv)
        num_set_val(retv, date->time);

    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.37 */
static HRESULT Date_setUTCDate(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    VARIANT v;
    HRESULT hres;
    DateInstance *date;
    DOUBLE t;

    TRACE("\n");

    if(!is_class(dispex, JSCLASS_DATE)) {
        FIXME("throw TypeError\n");
        return E_FAIL;
    }

    if(!arg_cnt(dp)) {
        FIXME("throw ArgumentNotOptional\n");
        if(retv) num_set_nan(retv);
        return S_OK;
    }

    hres = to_number(dispex->ctx, get_arg(dp, 0), ei, &v);
    if(FAILED(hres))
        return hres;

    date = (DateInstance*)dispex;
    t = date->time;
    t = make_date(make_day(year_from_time(t), month_from_time(t),
                num_val(&v)), time_within_day(t));
    date->time = time_clip(t);

    if(retv)
        num_set_val(retv, date->time);

    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.38 */
static HRESULT Date_setMonth(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    VARIANT v;
    HRESULT hres;
    DateInstance *date;
    DOUBLE t, month, ddate;

    TRACE("\n");

    if(!is_class(dispex, JSCLASS_DATE)) {
        FIXME("throw TypeError\n");
        return E_FAIL;
    }

    if(!arg_cnt(dp)) {
        FIXME("throw ArgumentNotOptional\n");
        if(retv) num_set_nan(retv);
        return S_OK;
    }

    date = (DateInstance*)dispex;
    t = local_time(date->time, date);

    hres = to_number(dispex->ctx, get_arg(dp, 0), ei, &v);
    if(FAILED(hres))
        return hres;
    month = num_val(&v);

    if(arg_cnt(dp) > 1) {
        hres = to_number(dispex->ctx, get_arg(dp, 1), ei, &v);
        if(FAILED(hres))
            return hres;
        ddate = num_val(&v);
    }
    else ddate = date_from_time(t);

    t = make_date(make_day(year_from_time(t), month, ddate),
            time_within_day(t));
    date->time = time_clip(utc(t, date));

    if(retv)
        num_set_val(retv, date->time);

    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.39 */
static HRESULT Date_setUTCMonth(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    VARIANT v;
    HRESULT hres;
    DateInstance *date;
    DOUBLE t, month, ddate;

    TRACE("\n");

    if(!is_class(dispex, JSCLASS_DATE)) {
        FIXME("throw TypeError\n");
        return E_FAIL;
    }

    if(!arg_cnt(dp)) {
        FIXME("throw ArgumentNotOptional\n");
        if(retv) num_set_nan(retv);
        return S_OK;
    }

    date = (DateInstance*)dispex;
    t = date->time;

    hres = to_number(dispex->ctx, get_arg(dp, 0), ei, &v);
    if(FAILED(hres))
        return hres;
    month = num_val(&v);

    if(arg_cnt(dp) > 1) {
        hres = to_number(dispex->ctx, get_arg(dp, 1), ei, &v);
        if(FAILED(hres))
            return hres;
        ddate = num_val(&v);
    }
    else ddate = date_from_time(t);

    t = make_date(make_day(year_from_time(t), month, ddate),
            time_within_day(t));
    date->time = time_clip(t);

    if(retv)
        num_set_val(retv, date->time);

    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.40 */
static HRESULT Date_setFullYear(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    VARIANT v;
    HRESULT hres;
    DateInstance *date;
    DOUBLE t, year, month, ddate;

    TRACE("\n");

    if(!is_class(dispex, JSCLASS_DATE)) {
        FIXME("throw TypeError\n");
        return E_FAIL;
    }

    if(!arg_cnt(dp)) {
        FIXME("throw ArgumentNotOptional\n");
        if(retv) num_set_nan(retv);
        return S_OK;
    }

    date = (DateInstance*)dispex;
    t = local_time(date->time, date);

    hres = to_number(dispex->ctx, get_arg(dp, 0), ei, &v);
    if(FAILED(hres))
        return hres;
    year = num_val(&v);

    if(arg_cnt(dp) > 1) {
        hres = to_number(dispex->ctx, get_arg(dp, 1), ei, &v);
        if(FAILED(hres))
            return hres;
        month = num_val(&v);
    }
    else month = month_from_time(t);

    if(arg_cnt(dp) > 2) {
        hres = to_number(dispex->ctx, get_arg(dp, 2), ei, &v);
        if(FAILED(hres))
            return hres;
        ddate = num_val(&v);
    }
    else ddate = date_from_time(t);

    t = make_date(make_day(year, month, ddate), time_within_day(t));
    date->time = time_clip(utc(t, date));

    if(retv)
        num_set_val(retv, date->time);

    return S_OK;
}

/* ECMA-262 3rd Edition    15.9.5.41 */
static HRESULT Date_setUTCFullYear(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *caller)
{
    VARIANT v;
    HRESULT hres;
    DateInstance *date;
    DOUBLE t, year, month, ddate;

    TRACE("\n");

    if(!is_class(dispex, JSCLASS_DATE)) {
        FIXME("throw TypeError\n");
        return E_FAIL;
    }

    if(!arg_cnt(dp)) {
        FIXME("throw ArgumentNotOptional\n");
        if(retv) num_set_nan(retv);
        return S_OK;
    }

    date = (DateInstance*)dispex;
    t = date->time;

    hres = to_number(dispex->ctx, get_arg(dp, 0), ei, &v);
    if(FAILED(hres))
        return hres;
    year = num_val(&v);

    if(arg_cnt(dp) > 1) {
        hres = to_number(dispex->ctx, get_arg(dp, 1), ei, &v);
        if(FAILED(hres))
            return hres;
        month = num_val(&v);
    }
    else month = month_from_time(t);

    if(arg_cnt(dp) > 2) {
        hres = to_number(dispex->ctx, get_arg(dp, 2), ei, &v);
        if(FAILED(hres))
            return hres;
        ddate = num_val(&v);
    }
    else ddate = date_from_time(t);

    t = make_date(make_day(year, month, ddate), time_within_day(t));
    date->time = time_clip(t);

    if(retv)
        num_set_val(retv, date->time);

    return S_OK;
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
    {getMillisecondsW,       Date_getMilliseconds,       PROPF_METHOD},
    {getMinutesW,            Date_getMinutes,            PROPF_METHOD},
    {getMonthW,              Date_getMonth,              PROPF_METHOD},
    {getSecondsW,            Date_getSeconds,            PROPF_METHOD},
    {getTimeW,               Date_getTime,               PROPF_METHOD},
    {getTimezoneOffsetW,     Date_getTimezoneOffset,     PROPF_METHOD},
    {getUTCDateW,            Date_getUTCDate,            PROPF_METHOD},
    {getUTCDayW,             Date_getUTCDay,             PROPF_METHOD},
    {getUTCFullYearW,        Date_getUTCFullYear,        PROPF_METHOD},
    {getUTCHoursW,           Date_getUTCHours,           PROPF_METHOD},
    {getUTCMillisecondsW,    Date_getUTCMilliseconds,    PROPF_METHOD},
    {getUTCMinutesW,         Date_getUTCMinutes,         PROPF_METHOD},
    {getUTCMonthW,           Date_getUTCMonth,           PROPF_METHOD},
    {getUTCSecondsW,         Date_getUTCSeconds,         PROPF_METHOD},
    {hasOwnPropertyW,        Date_hasOwnProperty,        PROPF_METHOD},
    {isPrototypeOfW,         Date_isPrototypeOf,         PROPF_METHOD},
    {propertyIsEnumerableW,  Date_propertyIsEnumerable,  PROPF_METHOD},
    {setDateW,               Date_setDate,               PROPF_METHOD},
    {setFullYearW,           Date_setFullYear,           PROPF_METHOD},
    {setHoursW,              Date_setHours,              PROPF_METHOD},
    {setMillisecondsW,       Date_setMilliseconds,       PROPF_METHOD},
    {setMinutesW,            Date_setMinutes,            PROPF_METHOD},
    {setMonthW,              Date_setMonth,              PROPF_METHOD},
    {setSecondsW,            Date_setSeconds,            PROPF_METHOD},
    {setTimeW,               Date_setTime,               PROPF_METHOD},
    {setUTCDateW,            Date_setUTCDate,            PROPF_METHOD},
    {setUTCFullYearW,        Date_setUTCFullYear,        PROPF_METHOD},
    {setUTCHoursW,           Date_setUTCHours,           PROPF_METHOD},
    {setUTCMillisecondsW,    Date_setUTCMilliseconds,    PROPF_METHOD},
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
    date->standardDate = tzi.StandardDate;
    date->standardBias = tzi.StandardBias;
    date->daylightDate = tzi.DaylightDate;
    date->daylightBias = tzi.DaylightBias;

    *ret = &date->dispex;
    return S_OK;
}

static HRESULT DateConstr_parse(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT DateConstr_UTC(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    VARIANT year, month, vdate, hours, minutes, seconds, ms;
    DOUBLE y;
    int arg_no = arg_cnt(dp);
    HRESULT hres;

    TRACE("\n");

    if(arg_no>0) {
        hres = to_number(dispex->ctx, get_arg(dp, 0), ei, &year);
        if(FAILED(hres))
            return hres;
        y = num_val(&year);
        if(0<=y && y<=99)
            y += 1900;
    }
    else y = 1900;

    if(arg_no>1) {
        hres = to_number(dispex->ctx, get_arg(dp, 1), ei, &month);
        if(FAILED(hres))
            return hres;
    }
    else {
        V_VT(&month) = VT_R8;
        V_R8(&month) = 0;
    }

    if(arg_no>2) {
        hres = to_number(dispex->ctx, get_arg(dp, 2), ei, &vdate);
        if(FAILED(hres))
            return hres;
    }
    else {
        V_VT(&vdate) = VT_R8;
        V_R8(&vdate) = 1;
    }

    if(arg_no>3) {
        hres = to_number(dispex->ctx, get_arg(dp, 3), ei, &hours);
        if(FAILED(hres))
            return hres;
    }
    else {
        V_VT(&hours) = VT_R8;
        V_R8(&hours) = 0;
    }

    if(arg_no>4) {
        hres = to_number(dispex->ctx, get_arg(dp, 4), ei, &minutes);
        if(FAILED(hres))
            return hres;
    }
    else {
        V_VT(&minutes) = VT_R8;
        V_R8(&minutes) = 0;
    }

    if(arg_no>5) {
        hres = to_number(dispex->ctx, get_arg(dp, 5), ei, &seconds);
        if(FAILED(hres))
            return hres;
    }
    else {
        V_VT(&seconds) = VT_R8;
        V_R8(&seconds) = 0;
    }

    if(arg_no>6) {
        hres = to_number(dispex->ctx, get_arg(dp, 6), ei, &ms);
        if(FAILED(hres))
            return hres;
    }
    else {
        V_VT(&ms) = VT_R8;
        V_R8(&ms) = 0;
    }

    if(retv) {
        V_VT(retv) = VT_R8;
        V_R8(retv) = time_clip(make_date(
                    make_day(y, num_val(&month), num_val(&vdate)),
                    make_time(num_val(&hours), num_val(&minutes),
                    num_val(&seconds), num_val(&ms))));
    }

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

        /* ECMA-262 3rd Edition    15.9.3.1 */
        default: {
            VARIANT ret_date;
            DateInstance *di;

            DateConstr_UTC(dispex, lcid, flags, dp, &ret_date, ei, sp);

            hres = create_date(dispex->ctx, TRUE, num_val(&ret_date), &date);
            if(FAILED(hres))
                return hres;

            di = (DateInstance*)date;
            di->time = utc(di->time, di);
        }
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

static const builtin_prop_t DateConstr_props[] = {
    {UTCW,    DateConstr_UTC,    PROPF_METHOD},
    {parseW,  DateConstr_parse,  PROPF_METHOD}
};

static const builtin_info_t DateConstr_info = {
    JSCLASS_FUNCTION,
    {NULL, Function_value, 0},
    sizeof(DateConstr_props)/sizeof(*DateConstr_props),
    DateConstr_props,
    NULL,
    NULL
};

HRESULT create_date_constr(script_ctx_t *ctx, DispatchEx **ret)
{
    DispatchEx *date;
    HRESULT hres;

    hres = create_date(ctx, FALSE, 0.0, &date);
    if(FAILED(hres))
        return hres;

    hres = create_builtin_function(ctx, DateConstr_value, &DateConstr_info, PROPF_CONSTR, date, ret);

    jsdisp_release(date);
    return hres;
}
