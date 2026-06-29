/*
 * Unit test suite for time functions.
 *
 * Copyright 2004 Uwe Bonnes
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

#include "wine/test.h"
#include "winbase.h"
#include "winnls.h"
#include "time.h"

#include <stdlib.h> /*setenv*/
#include <stdio.h> /*printf*/
#include <locale.h>
#include <errno.h>

#define _MAX__TIME64_T     (((__time64_t)0x00000007 << 32) | 0x93406FFF)

#define SECSPERDAY         86400
#define SECSPERHOUR        3600
#define SECSPERMIN         60
#define MINSPERHOUR        60
#define HOURSPERDAY        24

typedef struct {
    const char *short_wday[7];
    const char *wday[7];
    const char *short_mon[12];
    const char *mon[12];
    const char *am;
    const char *pm;
    const char *short_date;
    const char *date;
    const char *time;
    LCID lcid;
    int unk;
    int refcount;
} __lc_time_data;

static __time32_t (__cdecl *p_mkgmtime32)(struct tm*);
static struct tm* (__cdecl *p_gmtime32)(__time32_t*);
static struct tm* (__cdecl *p_gmtime)(time_t*);
static errno_t    (__cdecl *p_gmtime32_s)(struct tm*, __time32_t*);
static struct tm* (__cdecl *p_gmtime64)(__time64_t*);
static errno_t    (__cdecl *p_gmtime64_s)(struct tm*, __time64_t*);
static errno_t    (__cdecl *p_strtime_s)(char*,size_t);
static errno_t    (__cdecl *p_strdate_s)(char*,size_t);
static errno_t    (__cdecl *p_localtime32_s)(struct tm*, __time32_t*);
static errno_t    (__cdecl *p_localtime64_s)(struct tm*, __time64_t*);
static int*       (__cdecl *p__daylight)(void);
static int*       (__cdecl *p___p__daylight)(void);
static long*      (__cdecl *p___p__dstbias)(void);
static long*      (__cdecl *p__dstbias)(void);
static long*      (__cdecl *p___p__timezone)(void);
static size_t     (__cdecl *p_strftime)(char *, size_t, const char *, const struct tm *);
static size_t (__cdecl *p__Strftime)(char*, size_t, const char*, const struct tm*, void*);
static size_t     (__cdecl *p_wcsftime)(wchar_t *, size_t, const wchar_t *, const struct tm *);
static char*      (__cdecl *p_asctime)(const struct tm *);

static void init(void)
{
    HMODULE hmod = LoadLibraryA("msvcrt.dll");

    p_gmtime32 = (void*)GetProcAddress(hmod, "_gmtime32");
    p_gmtime = (void*)GetProcAddress(hmod, "gmtime");
    p_gmtime32_s = (void*)GetProcAddress(hmod, "_gmtime32_s");
    p_gmtime64 = (void*)GetProcAddress(hmod, "_gmtime64");
    p_gmtime64_s = (void*)GetProcAddress(hmod, "_gmtime64_s");
    p_mkgmtime32 = (void*)GetProcAddress(hmod, "_mkgmtime32");
    p_strtime_s = (void*)GetProcAddress(hmod, "_strtime_s");
    p_strdate_s = (void*)GetProcAddress(hmod, "_strdate_s");
    p_localtime32_s = (void*)GetProcAddress(hmod, "_localtime32_s");
    p_localtime64_s = (void*)GetProcAddress(hmod, "_localtime64_s");
    p__daylight = (void*)GetProcAddress(hmod, "__daylight");
    p___p__daylight = (void*)GetProcAddress(hmod, "__p__daylight");
    p___p__dstbias = (void*)GetProcAddress(hmod, "__p__dstbias");
    p__dstbias = (void*)GetProcAddress(hmod, "__dstbias");
    p___p__timezone = (void*)GetProcAddress(hmod, "__p__timezone");
    p_strftime = (void*)GetProcAddress(hmod, "strftime");
    p__Strftime = (void*)GetProcAddress(hmod, "_Strftime");
    p_wcsftime = (void*)GetProcAddress(hmod, "wcsftime");
    p_asctime = (void*)GetProcAddress(hmod, "asctime");
}

static int get_test_year(time_t *start)
{
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);

    /* compute start of year in seconds */
    *start = SECSPERDAY * ((tm->tm_year - 70) * 365 +
                           (tm->tm_year - 69) / 4 -
                           (tm->tm_year - 1) / 100 +
                           (tm->tm_year + 299) / 400);
    return tm->tm_year;
}

static void test_ctime(void)
{
    time_t badtime = -1;
    char* ret;
    ret = ctime(&badtime);
    ok(ret == NULL, "expected ctime to return NULL, got %s\n", ret);
}
static void test_gmtime(void)
{
    __time32_t valid, gmt;
    struct tm* gmt_tm, gmt_tm_s;
    errno_t err;

    if(!p_gmtime32) {
        win_skip("Skipping _gmtime32 tests\n");
        return;
    }

    gmt_tm = p_gmtime32(NULL);
    ok(gmt_tm == NULL, "gmt_tm != NULL\n");

    gmt = -1;
    gmt_tm = p_gmtime32(&gmt);
    ok(gmt_tm==NULL || broken(gmt_tm->tm_year==70 && gmt_tm->tm_sec<0), "gmt_tm != NULL\n");

    gmt = valid = 0;
    gmt_tm = p_gmtime32(&gmt);
    if(!gmt_tm) {
        ok(0, "_gmtime32() failed\n");
        return;
    }

    ok(((gmt_tm->tm_year == 70) && (gmt_tm->tm_mon  == 0) && (gmt_tm->tm_yday  == 0) &&
                (gmt_tm->tm_mday ==  1) && (gmt_tm->tm_wday == 4) && (gmt_tm->tm_hour  == 0) &&
                (gmt_tm->tm_min  ==  0) && (gmt_tm->tm_sec  == 0) && (gmt_tm->tm_isdst == 0)),
            "Wrong date:Year %4d mon %2d yday %3d mday %2d wday %1d hour%2d min %2d sec %2d dst %2d\n",
            gmt_tm->tm_year, gmt_tm->tm_mon, gmt_tm->tm_yday, gmt_tm->tm_mday, gmt_tm->tm_wday,
            gmt_tm->tm_hour, gmt_tm->tm_min, gmt_tm->tm_sec, gmt_tm->tm_isdst);

    if(!p_mkgmtime32) {
        win_skip("Skipping _mkgmtime32 tests\n");
        return;
    }

    gmt_tm->tm_wday = gmt_tm->tm_yday = 0;
    gmt = p_mkgmtime32(gmt_tm);
    ok(gmt == valid, "gmt = %lu\n", gmt);
    ok(gmt_tm->tm_wday == 4, "gmt_tm->tm_wday = %d\n", gmt_tm->tm_wday);
    ok(gmt_tm->tm_yday == 0, "gmt_tm->tm_yday = %d\n", gmt_tm->tm_yday);

    gmt_tm->tm_wday = gmt_tm->tm_yday = 0;
    gmt_tm->tm_isdst = -1;
    gmt = p_mkgmtime32(gmt_tm);
    ok(gmt == valid, "gmt = %lu\n", gmt);
    ok(gmt_tm->tm_wday == 4, "gmt_tm->tm_wday = %d\n", gmt_tm->tm_wday);
    ok(gmt_tm->tm_yday == 0, "gmt_tm->tm_yday = %d\n", gmt_tm->tm_yday);

    gmt_tm->tm_wday = gmt_tm->tm_yday = 0;
    gmt_tm->tm_isdst = 1;
    gmt = p_mkgmtime32(gmt_tm);
    ok(gmt == valid, "gmt = %lu\n", gmt);
    ok(gmt_tm->tm_wday == 4, "gmt_tm->tm_wday = %d\n", gmt_tm->tm_wday);
    ok(gmt_tm->tm_yday == 0, "gmt_tm->tm_yday = %d\n", gmt_tm->tm_yday);

    gmt = valid = 173921;
    gmt_tm = p_gmtime32(&gmt);
    if(!gmt_tm) {
        ok(0, "_gmtime32() failed\n");
        return;
    }

    gmt_tm->tm_isdst = -1;
    gmt = p_mkgmtime32(gmt_tm);
    ok(gmt == valid, "gmt = %lu\n", gmt);
    ok(gmt_tm->tm_wday == 6, "gmt_tm->tm_wday = %d\n", gmt_tm->tm_wday);
    ok(gmt_tm->tm_yday == 2, "gmt_tm->tm_yday = %d\n", gmt_tm->tm_yday);

    gmt_tm->tm_isdst = 1;
    gmt = p_mkgmtime32(gmt_tm);
    ok(gmt == valid, "gmt = %lu\n", gmt);

    if(!p_gmtime32_s) {
        win_skip("Skipping _gmtime32_s tests\n");
        return;
    }

    errno = 0;
    gmt = 0;
    err = p_gmtime32_s(NULL, &gmt);
    ok(err == EINVAL, "err = %d\n", err);
    ok(errno == EINVAL, "errno = %d\n", errno);

    errno = 0;
    gmt = -1;
    err = p_gmtime32_s(&gmt_tm_s, &gmt);
    ok(gmt_tm_s.tm_year == -1 || broken(gmt_tm_s.tm_year == 70 && gmt_tm_s.tm_sec < 0),
       "tm_year = %d, tm_sec = %d\n", gmt_tm_s.tm_year, gmt_tm_s.tm_sec);
    if(gmt_tm_s.tm_year == -1) {
        ok(err==EINVAL, "err = %d\n", err);
        ok(errno==EINVAL, "errno = %d\n", errno);
    }
}

static void test_gmtime64(void)
{
    struct tm *ptm, tm;
    __time64_t t;
    int ret;

    t = -1;
    memset(&tm, 0xcc, sizeof(tm));
    ptm = p_gmtime64(&t);
    if (!ptm)
    {
        skip("Old gmtime64 limits, skipping tests.\n");
        return;
    }
    ok(!!ptm, "got NULL.\n");
    ret = p_gmtime64_s(&tm, &t);
    ok(!ret, "got %d.\n", ret);
    ok(tm.tm_year == 69 && tm.tm_hour == 23 && tm.tm_min == 59 && tm.tm_sec == 59, "got %d, %d, %d, %d.\n",
            tm.tm_year, tm.tm_hour, tm.tm_min, tm.tm_sec);

    t = -43200;
    memset(&tm, 0xcc, sizeof(tm));
    ptm = p_gmtime64(&t);
    ok(!!ptm, "got NULL.\n");
    ret = p_gmtime64_s(&tm, &t);
    ok(!ret, "got %d.\n", ret);
    ok(tm.tm_year == 69 && tm.tm_hour == 12 && tm.tm_min == 0 && tm.tm_sec == 0, "got %d, %d, %d, %d.\n",
            tm.tm_year, tm.tm_hour, tm.tm_min, tm.tm_sec);
    ptm = p_gmtime32((__time32_t *)&t);
    ok(!!ptm, "got NULL.\n");
    memset(&tm, 0xcc, sizeof(tm));
    ret = p_gmtime32_s(&tm, (__time32_t *)&t);
    ok(!ret, "got %d.\n", ret);
    todo_wine_if(tm.tm_year == 69 && tm.tm_hour == 12)
    ok(tm.tm_year == 70 && tm.tm_hour == -12 && tm.tm_min == 0 && tm.tm_sec == 0, "got %d, %d, %d, %d.\n",
            tm.tm_year, tm.tm_hour, tm.tm_min, tm.tm_sec);

    t = -43201;
    ptm = p_gmtime64(&t);
    ok(!ptm, "got non-NULL.\n");
    memset(&tm, 0xcc, sizeof(tm));
    ret = p_gmtime64_s(&tm, &t);
    ok(ret == EINVAL, "got %d.\n", ret);
    ok(tm.tm_year == -1 && tm.tm_hour == -1 && tm.tm_min == -1 && tm.tm_sec == -1, "got %d, %d, %d, %d.\n",
            tm.tm_year, tm.tm_hour, tm.tm_min, tm.tm_sec);
    ptm = p_gmtime32((__time32_t *)&t);
    ok(!ptm, "got NULL.\n");
    memset(&tm, 0xcc, sizeof(tm));
    ret = p_gmtime32_s(&tm, (__time32_t *)&t);
    ok(ret == EINVAL, "got %d.\n", ret);
    ok(tm.tm_year == -1 && tm.tm_hour == -1 && tm.tm_min == -1 && tm.tm_sec == -1, "got %d, %d, %d, %d.\n",
            tm.tm_year, tm.tm_hour, tm.tm_min, tm.tm_sec);

    t = _MAX__TIME64_T + 46800;
    memset(&tm, 0xcc, sizeof(tm));
    ptm = p_gmtime64(&t);
    ok(!!ptm, "got NULL.\n");
    ret = p_gmtime64_s(&tm, &t);
    ok(!ret, "got %d.\n", ret);
    ok(tm.tm_year == 1101 && tm.tm_hour == 20 && tm.tm_min == 59 && tm.tm_sec == 59, "got %d, %d, %d, %d.\n",
            tm.tm_year, tm.tm_hour, tm.tm_min, tm.tm_sec);

    t = _MAX__TIME64_T + 46801;
    ptm = p_gmtime64(&t);
    ok(!ptm, "got non-NULL.\n");
    memset(&tm, 0xcc, sizeof(tm));
    ret = p_gmtime64_s(&tm, &t);
    ok(ret == EINVAL, "got %d.\n", ret);
    ok(tm.tm_year == -1 && tm.tm_hour == -1 && tm.tm_min == -1 && tm.tm_sec == -1, "got %d, %d, %d, %d.\n",
            tm.tm_year, tm.tm_hour, tm.tm_min, tm.tm_sec);
}

static void test_mktime(void)
{
    TIME_ZONE_INFORMATION tzinfo;
    DWORD res =  GetTimeZoneInformation(&tzinfo);
    struct tm my_tm, sav_tm;
    time_t nulltime, local_time;
    char TZ_env[256];
    char buffer[64];
    int year;
    time_t ref, secs;

    year = get_test_year( &ref );
    ref += SECSPERDAY;

    ok (res != TIME_ZONE_ID_INVALID, "GetTimeZoneInformation failed\n");
    WideCharToMultiByte( CP_ACP, 0, tzinfo.StandardName, -1, buffer, sizeof(buffer), NULL, NULL );
    trace( "bias %ld std %ld dst %ld zone %s\n",
           tzinfo.Bias, tzinfo.StandardBias, tzinfo.DaylightBias, buffer );
    /* Bias may be positive or negative, to use offset of one day */
    my_tm = *localtime(&ref);  /* retrieve current dst flag */
    secs = SECSPERDAY - tzinfo.Bias * SECSPERMIN;
    secs -= (my_tm.tm_isdst ? tzinfo.DaylightBias : tzinfo.StandardBias) * SECSPERMIN;
    my_tm.tm_mday = 1 + secs/SECSPERDAY;
    secs = secs % SECSPERDAY;
    my_tm.tm_hour = secs / SECSPERHOUR;
    secs = secs % SECSPERHOUR;
    my_tm.tm_min = secs / SECSPERMIN;
    secs = secs % SECSPERMIN;
    my_tm.tm_sec = secs;

    my_tm.tm_year = year;
    my_tm.tm_mon  =  0;

    sav_tm = my_tm;

    local_time = mktime(&my_tm);
    ok(local_time == ref, "mktime returned %lu, expected %lu\n",
       (DWORD)local_time, (DWORD)ref);
    /* now test some unnormalized struct tm's */
    my_tm = sav_tm;
    my_tm.tm_sec += 60;
    my_tm.tm_min -= 1;
    local_time = mktime(&my_tm);
    ok(local_time == ref, "Unnormalized mktime returned %lu, expected %lu\n",
        (DWORD)local_time, (DWORD)ref);
    ok( my_tm.tm_year == sav_tm.tm_year && my_tm.tm_mon == sav_tm.tm_mon &&
        my_tm.tm_mday == sav_tm.tm_mday && my_tm.tm_hour == sav_tm.tm_hour &&
        my_tm.tm_sec == sav_tm.tm_sec,
            "mktime returned %2d-%02d-%02d %02d:%02d expected %2d-%02d-%02d %02d:%02d\n",
            my_tm.tm_year,my_tm.tm_mon,my_tm.tm_mday,
            my_tm.tm_hour,my_tm.tm_sec,
            sav_tm.tm_year,sav_tm.tm_mon,sav_tm.tm_mday,
            sav_tm.tm_hour,sav_tm.tm_sec);
    my_tm = sav_tm;
    my_tm.tm_min -= 60;
    my_tm.tm_hour += 1;
    local_time = mktime(&my_tm);
    ok(local_time == ref, "Unnormalized mktime returned %lu, expected %lu\n",
       (DWORD)local_time, (DWORD)ref);
    ok( my_tm.tm_year == sav_tm.tm_year && my_tm.tm_mon == sav_tm.tm_mon &&
        my_tm.tm_mday == sav_tm.tm_mday && my_tm.tm_hour == sav_tm.tm_hour &&
        my_tm.tm_sec == sav_tm.tm_sec,
            "mktime returned %2d-%02d-%02d %02d:%02d expected %2d-%02d-%02d %02d:%02d\n",
            my_tm.tm_year,my_tm.tm_mon,my_tm.tm_mday,
            my_tm.tm_hour,my_tm.tm_sec,
            sav_tm.tm_year,sav_tm.tm_mon,sav_tm.tm_mday,
            sav_tm.tm_hour,sav_tm.tm_sec);
    my_tm = sav_tm;
    my_tm.tm_mon -= 12;
    my_tm.tm_year += 1;
    local_time = mktime(&my_tm);
    ok(local_time == ref, "Unnormalized mktime returned %lu, expected %lu\n",
       (DWORD)local_time, (DWORD)ref);
    ok( my_tm.tm_year == sav_tm.tm_year && my_tm.tm_mon == sav_tm.tm_mon &&
        my_tm.tm_mday == sav_tm.tm_mday && my_tm.tm_hour == sav_tm.tm_hour &&
        my_tm.tm_sec == sav_tm.tm_sec,
            "mktime returned %2d-%02d-%02d %02d:%02d expected %2d-%02d-%02d %02d:%02d\n",
            my_tm.tm_year,my_tm.tm_mon,my_tm.tm_mday,
            my_tm.tm_hour,my_tm.tm_sec,
            sav_tm.tm_year,sav_tm.tm_mon,sav_tm.tm_mday,
            sav_tm.tm_hour,sav_tm.tm_sec);
    my_tm = sav_tm;
    my_tm.tm_mon += 12;
    my_tm.tm_year -= 1;
    local_time = mktime(&my_tm);
    ok(local_time == ref, "Unnormalized mktime returned %lu, expected %lu\n",
       (DWORD)local_time, (DWORD)ref);
    ok( my_tm.tm_year == sav_tm.tm_year && my_tm.tm_mon == sav_tm.tm_mon &&
        my_tm.tm_mday == sav_tm.tm_mday && my_tm.tm_hour == sav_tm.tm_hour &&
        my_tm.tm_sec == sav_tm.tm_sec,
            "mktime returned %2d-%02d-%02d %02d:%02d expected %2d-%02d-%02d %02d:%02d\n",
            my_tm.tm_year,my_tm.tm_mon,my_tm.tm_mday,
            my_tm.tm_hour,my_tm.tm_sec,
            sav_tm.tm_year,sav_tm.tm_mon,sav_tm.tm_mday,
            sav_tm.tm_hour,sav_tm.tm_sec);
    /* now a bad time example */
    my_tm = sav_tm;
    my_tm.tm_year = 69;
    local_time = mktime(&my_tm);
    ok((local_time == -1), "(bad time) mktime returned %d, expected -1\n", (int)local_time);

    my_tm = sav_tm;
    /* TEST that we are independent from the TZ variable */
    /*Argh, msvcrt doesn't have setenv() */
    _snprintf(TZ_env,255,"TZ=%s",(getenv("TZ")?getenv("TZ"):""));
    putenv("TZ=GMT");
    nulltime = mktime(&my_tm);
    ok(nulltime == ref,"mktime returned 0x%08lx\n",(DWORD)nulltime);
    putenv(TZ_env);
}

static void test_localtime(void)
{
    TIME_ZONE_INFORMATION tzinfo;
    DWORD res =  GetTimeZoneInformation(&tzinfo);
    time_t gmt, ref;

    char TZ_env[256];
    struct tm* lt;
    int year = get_test_year( &ref );
    int is_leap = !(year % 4) && ((year % 100) || !((year + 300) % 400));

    gmt = ref + SECSPERDAY + tzinfo.Bias * SECSPERMIN;
    ok (res != TIME_ZONE_ID_INVALID, "GetTimeZoneInformation failed\n");
    lt = localtime(&gmt);
    gmt += (lt->tm_isdst ? tzinfo.DaylightBias : tzinfo.StandardBias) * SECSPERMIN;
    lt = localtime(&gmt);
    ok(((lt->tm_year == year) && (lt->tm_mon  == 0) && (lt->tm_yday  == 1) &&
	(lt->tm_mday ==  2) && (lt->tm_hour  == 0) &&
	(lt->tm_min  ==  0) && (lt->tm_sec  == 0)),
       "Wrong date:Year %d mon %d yday %d mday %d wday %d hour %d min %d sec %d dst %d\n",
       lt->tm_year, lt->tm_mon, lt->tm_yday, lt->tm_mday, lt->tm_wday, lt->tm_hour, 
       lt->tm_min, lt->tm_sec, lt->tm_isdst); 

    _snprintf(TZ_env,255,"TZ=%s",(getenv("TZ")?getenv("TZ"):""));
    putenv("TZ=GMT");
    lt = localtime(&gmt);
    ok(((lt->tm_year == year) && (lt->tm_mon  == 0) && (lt->tm_yday  == 1) &&
	(lt->tm_mday ==  2) && (lt->tm_hour  == 0) &&
	(lt->tm_min  ==  0) && (lt->tm_sec  == 0)),
       "Wrong date:Year %d mon %d yday %d mday %d wday %d hour %d min %d sec %d dst %d\n",
       lt->tm_year, lt->tm_mon, lt->tm_yday, lt->tm_mday, lt->tm_wday, lt->tm_hour, 
       lt->tm_min, lt->tm_sec, lt->tm_isdst); 
    putenv(TZ_env);

    /* June 22 */
    gmt = ref + 202 * SECSPERDAY + tzinfo.Bias * SECSPERMIN;
    lt = localtime(&gmt);
    gmt += (lt->tm_isdst ? tzinfo.DaylightBias : tzinfo.StandardBias) * SECSPERMIN;
    lt = localtime(&gmt);
    ok(((lt->tm_year == year) && (lt->tm_mon  == 6) && (lt->tm_yday  == 202) &&
	(lt->tm_mday == 22 - is_leap) && (lt->tm_hour  == 0) &&
	(lt->tm_min  ==  0) && (lt->tm_sec  == 0)),
       "Wrong date:Year %d mon %d yday %d mday %d wday %d hour %d min %d sec %d dst %d\n",
       lt->tm_year, lt->tm_mon, lt->tm_yday, lt->tm_mday, lt->tm_wday, lt->tm_hour, 
       lt->tm_min, lt->tm_sec, lt->tm_isdst); 
}

static void test_strdate(void)
{
    char date[16], * result;
    int month, day, year, count, len;
    errno_t err;

    result = _strdate(date);
    ok(result == date, "Wrong return value\n");
    len = strlen(date);
    ok(len == 8, "Wrong length: returned %d, should be 8\n", len);
    count = sscanf(date, "%02d/%02d/%02d", &month, &day, &year);
    ok(count == 3, "Wrong format: count = %d, should be 3\n", count);

    if(!p_strdate_s) {
        win_skip("Skipping _strdate_s tests\n");
        return;
    }

    errno = 0;
    err = p_strdate_s(NULL, 1);
    ok(err == EINVAL, "err = %d\n", err);
    ok(errno == EINVAL, "errno = %d\n", errno);

    date[0] = 'x';
    date[1] = 'x';
    err = p_strdate_s(date, 8);
    ok(err == ERANGE, "err = %d\n", err);
    ok(errno == ERANGE, "errno = %d\n", errno);
    ok(date[0] == '\0', "date[0] != '\\0'\n");
    ok(date[1] == 'x', "date[1] != 'x'\n");

    err = p_strdate_s(date, 9);
    ok(err == 0, "err = %x\n", err);
}

static void test_strtime(void)
{
    char time[16], * result;
    int hour, minute, second, count, len;
    errno_t err;

    result = _strtime(time);
    ok(result == time, "Wrong return value\n");
    len = strlen(time);
    ok(len == 8, "Wrong length: returned %d, should be 8\n", len);
    count = sscanf(time, "%02d:%02d:%02d", &hour, &minute, &second);
    ok(count == 3, "Wrong format: count = %d, should be 3\n", count);

    if(!p_strtime_s) {
        win_skip("Skipping _strtime_s tests\n");
        return;
    }

    errno = 0;
    err = p_strtime_s(NULL, 0);
    ok(err == EINVAL, "err = %d\n", err);
    ok(errno == EINVAL, "errno = %d\n", errno);

    err = p_strtime_s(NULL, 1);
    ok(err == EINVAL, "err = %d\n", err);
    ok(errno == EINVAL, "errno = %d\n", errno);

    time[0] = 'x';
    err = p_strtime_s(time, 8);
    ok(err == ERANGE, "err = %d\n", err);
    ok(errno == ERANGE, "errno = %d\n", errno);
    ok(time[0] == '\0', "time[0] != '\\0'\n");

    err = p_strtime_s(time, 9);
    ok(err == 0, "err = %x\n", err);
}

static void test_wstrdate(void)
{
    wchar_t date[16], * result;
    int month, day, year, count, len;

    result = _wstrdate(date);
    ok(result == date, "Wrong return value\n");
    len = wcslen(date);
    ok(len == 8, "Wrong length: returned %d, should be 8\n", len);
    count = swscanf(date, L"%02d/%02d/%02d", &month, &day, &year);
    ok(count == 3, "Wrong format: count = %d, should be 3\n", count);
}

static void test_wstrtime(void)
{
    wchar_t time[16], * result;
    int hour, minute, second, count, len;

    result = _wstrtime(time);
    ok(result == time, "Wrong return value\n");
    len = wcslen(time);
    ok(len == 8, "Wrong length: returned %d, should be 8\n", len);
    count = swscanf(time, L"%02d:%02d:%02d", &hour, &minute, &second);
    ok(count == 3, "Wrong format: count = %d, should be 3\n", count);
}

static void test_localtime32_s(void)
{
    struct tm tm;
    __time32_t time;
    errno_t err;

    if (!p_localtime32_s)
    {
        win_skip("Skipping _localtime32_s tests\n");
        return;
    }

    errno = EBADF;
    err = p_localtime32_s(NULL, NULL);
    ok(err == EINVAL, "Expected _localtime32_s to return EINVAL, got %d\n", err);
    ok(errno == EINVAL, "Expected errno to be EINVAL, got %d\n", errno);

    errno = EBADF;
    time = 0x12345678;
    err = p_localtime32_s(NULL, &time);
    ok(err == EINVAL, "Expected _localtime32_s to return EINVAL, got %d\n", err);
    ok(errno == EINVAL, "Expected errno to be EINVAL, got %d\n", errno);

    memset(&tm, 0, sizeof(tm));
    errno = EBADF;
    err = p_localtime32_s(&tm, NULL);
    ok(err == EINVAL, "Expected _localtime32_s to return EINVAL, got %d\n", err);
    ok(errno == EINVAL, "Expected errno to be EINVAL, got %d\n", errno);
    ok(tm.tm_sec == -1 && tm.tm_min == -1 && tm.tm_hour == -1 &&
       tm.tm_mday == -1 && tm.tm_mon == -1 && tm.tm_year == -1 &&
       tm.tm_wday == -1 && tm.tm_yday == -1 && tm.tm_isdst == -1,
       "Expected tm structure members to be initialized to -1, got "
       "(%d, %d, %d, %d, %d, %d, %d, %d, %d)\n", tm.tm_sec, tm.tm_min,
       tm.tm_hour, tm.tm_mday, tm.tm_mon, tm.tm_year, tm.tm_wday, tm.tm_yday,
       tm.tm_isdst);

    memset(&tm, 0, sizeof(tm));
    time = -1;
    errno = EBADF;
    err = p_localtime32_s(&tm, &time);
    ok(err == EINVAL, "Expected _localtime32_s to return EINVAL, got %d\n", err);
    ok(errno == EINVAL, "Expected errno to be EINVAL, got %d\n", errno);
    ok(tm.tm_sec == -1 && tm.tm_min == -1 && tm.tm_hour == -1 &&
       tm.tm_mday == -1 && tm.tm_mon == -1 && tm.tm_year == -1 &&
       tm.tm_wday == -1 && tm.tm_yday == -1 && tm.tm_isdst == -1,
       "Expected tm structure members to be initialized to -1, got "
       "(%d, %d, %d, %d, %d, %d, %d, %d, %d)\n", tm.tm_sec, tm.tm_min,
       tm.tm_hour, tm.tm_mday, tm.tm_mon, tm.tm_year, tm.tm_wday, tm.tm_yday,
       tm.tm_isdst);
}

static void test_localtime64_s(void)
{
    struct tm tm;
    __time64_t time;
    errno_t err;

    if (!p_localtime64_s)
    {
        win_skip("Skipping _localtime64_s tests\n");
        return;
    }

    errno = EBADF;
    err = p_localtime64_s(NULL, NULL);
    ok(err == EINVAL, "Expected _localtime64_s to return EINVAL, got %d\n", err);
    ok(errno == EINVAL, "Expected errno to be EINVAL, got %d\n", errno);

    errno = EBADF;
    time = 0xdeadbeef;
    err = p_localtime64_s(NULL, &time);
    ok(err == EINVAL, "Expected _localtime64_s to return EINVAL, got %d\n", err);
    ok(errno == EINVAL, "Expected errno to be EINVAL, got %d\n", errno);

    memset(&tm, 0, sizeof(tm));
    errno = EBADF;
    err = p_localtime64_s(&tm, NULL);
    ok(err == EINVAL, "Expected _localtime64_s to return EINVAL, got %d\n", err);
    ok(errno == EINVAL, "Expected errno to be EINVAL, got %d\n", errno);
    ok(tm.tm_sec == -1 && tm.tm_min == -1 && tm.tm_hour == -1 &&
       tm.tm_mday == -1 && tm.tm_mon == -1 && tm.tm_year == -1 &&
       tm.tm_wday == -1 && tm.tm_yday == -1 && tm.tm_isdst == -1,
       "Expected tm structure members to be initialized to -1, got "
       "(%d, %d, %d, %d, %d, %d, %d, %d, %d)\n", tm.tm_sec, tm.tm_min,
       tm.tm_hour, tm.tm_mday, tm.tm_mon, tm.tm_year, tm.tm_wday, tm.tm_yday,
       tm.tm_isdst);

    memset(&tm, 0, sizeof(tm));
    time = -1;
    errno = EBADF;
    err = p_localtime64_s(&tm, &time);
    ok(err == EINVAL, "Expected _localtime64_s to return EINVAL, got %d\n", err);
    ok(errno == EINVAL, "Expected errno to be EINVAL, got %d\n", errno);
    ok(tm.tm_sec == -1 && tm.tm_min == -1 && tm.tm_hour == -1 &&
       tm.tm_mday == -1 && tm.tm_mon == -1 && tm.tm_year == -1 &&
       tm.tm_wday == -1 && tm.tm_yday == -1 && tm.tm_isdst == -1,
       "Expected tm structure members to be initialized to -1, got "
       "(%d, %d, %d, %d, %d, %d, %d, %d, %d)\n", tm.tm_sec, tm.tm_min,
       tm.tm_hour, tm.tm_mday, tm.tm_mon, tm.tm_year, tm.tm_wday, tm.tm_yday,
       tm.tm_isdst);

    memset(&tm, 0, sizeof(tm));
    time = _MAX__TIME64_T + 1;
    errno = EBADF;
    err = p_localtime64_s(&tm, &time);
    ok(err == EINVAL, "Expected _localtime64_s to return EINVAL, got %d\n", err);
    ok(errno == EINVAL, "Expected errno to be EINVAL, got %d\n", errno);
    ok(tm.tm_sec == -1 && tm.tm_min == -1 && tm.tm_hour == -1 &&
       tm.tm_mday == -1 && tm.tm_mon == -1 && tm.tm_year == -1 &&
       tm.tm_wday == -1 && tm.tm_yday == -1 && tm.tm_isdst == -1,
       "Expected tm structure members to be initialized to -1, got "
       "(%d, %d, %d, %d, %d, %d, %d, %d, %d)\n", tm.tm_sec, tm.tm_min,
       tm.tm_hour, tm.tm_mday, tm.tm_mon, tm.tm_year, tm.tm_wday, tm.tm_yday,
       tm.tm_isdst);
}

static void test_daylight(void)
{
    int *ret1, *ret2;

    if (!p__daylight)
    {
        win_skip("__daylight() not available\n");
        return;
    }

    /* Examine the returned pointer from __p__environ(), if available. */
    if (sizeof(void*) != sizeof(int))
        ok( !p___p__daylight, "___p__daylight() should be 32-bit only\n");
    else
    {
        ret1 = p__daylight();
        ret2 = p___p__daylight();
        ok(ret1 && ret1 == ret2, "got %p\n", ret1);
    }
}

static void test_strftime(void)
{
    const struct {
        const char *format;
    } tests_einval[] = {
        {"%C"},
        {"%D"},
        {"%e"},
        {"%F"},
        {"%h"},
        {"%n"},
        {"%R"},
        {"%t"},
        {"%T"},
        {"%u"},
    };

    const struct {
        const char *format;
        const char *ret;
        struct tm tm;
        BOOL todo;
    } tests[] = {
        {"e%#%e", "e%e", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%c", "01/01/70 00:00:00", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%c", "02/30/70 00:00:00", { 0, 0, 0, 30, 1, 70, 4, 0, 0 }},
        {"%#c", "Thursday, January 01, 1970 00:00:00", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%#c", "Thursday, February 30, 1970 00:00:00", { 0, 0, 0, 30, 1, 70, 4, 0, 0 }},
        {"%x", "01/01/70", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%x", "02/30/70", { 0, 0, 0, 30, 1, 70, 4, 0, 0 }},
        {"%#x", "Thursday, January 01, 1970", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%#x", "Thursday, February 30, 1970", { 0, 0, 0, 30, 1, 70, 4, 0, 0 }},
        {"%X", "00:00:00", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%X", "14:00:00", { 0, 0, 14, 1, 0, 70, 4, 0, 0 }},
        {"%a", "Thu", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%A", "Thursday", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%b", "Jan", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%B", "January", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%d", "01", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%#d", "1", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%H", "00", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%I", "12", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%j", "001", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%m", "01", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%#M", "0", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%p", "AM", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%U", "00", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%W", "00", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%U", "01", { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        {"%W", "00", { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        {"%U", "53", { 0, 0, 0, 1, 0, 70, 0, 365, 0 }},
        {"%W", "52", { 0, 0, 0, 1, 0, 70, 0, 365, 0 }},
    };

    const struct {
        const char *format;
        const char *ret;
        const char *short_date;
        const char *date;
        const char *time;
        struct tm tm;
        BOOL todo;
    } tests_td[] = {
        { "%c", "x z", "x", "y", "z", { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%#c", "y z", "x", "y", "z", { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%X", "m1", 0, 0, "MMM", { 0, 0, 1, 1, 0, 70, 0, 0, 0 }},
        { "%X", "1", 0, 0, "h", { 0, 0, 1, 1, 0, 70, 0, 0, 0 }},
        { "%X", "01", 0, 0, "hh", { 0, 0, 1, 1, 0, 70, 0, 0, 0 }},
        { "%X", "h01", 0, 0, "hhh", { 0, 0, 1, 1, 0, 70, 0, 0, 0 }},
        { "%X", "hh01", 0, 0, "hhhh", { 0, 0, 1, 1, 0, 70, 0, 0, 0 }},
        { "%X", "1", 0, 0, "H", { 0, 0, 1, 1, 0, 70, 0, 0, 0 }},
        { "%X", "01", 0, 0, "HH", { 0, 0, 1, 1, 0, 70, 0, 0, 0 }},
        { "%X", "H13", 0, 0, "HHH", { 0, 0, 13, 1, 0, 70, 0, 0, 0 }},
        { "%X", "0", 0, 0, "m", { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%X", "00", 0, 0, "mm", { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%X", "0", 0, 0, "s", { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%X", "00", 0, 0, "ss", { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%X", "s00", 0, 0, "sss", { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%X", "t", 0, 0, "t", { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%X", "tam", 0, 0, "tt", { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%X", "tam", 0, 0, "ttttttttt", { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%X", "tam", 0, 0, "a", { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%X", "tam", 0, 0, "aaaaa", { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%X", "tam", 0, 0, "A", { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%X", "tam", 0, 0, "AAAAA", { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%x", "1", "d", 0, 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%x", "01", "dd", 0, 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%x", "d1", "ddd", 0, 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%x", "day1", "dddd", 0, 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%x", "dday1", "ddddd", 0, 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%x", "1", "M", 0, 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%x", "01", "MM", 0, 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%x", "m1", "MMM", 0, 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%x", "mon1", "MMMM", 0, 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%x", "Mmon1", "MMMMM", 0, 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%x", "y", "y", 0, 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%x", "70", "yy", 0, 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%x", "y70", "yyy", 0, 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%x", "1970", "yyyy", 0, 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%x", "y1970", "yyyyy", 0, 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%x", "ggggggggggg", "ggggggggggg", 0, 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%#x", "1", 0, "d", 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%#x", "01", 0, "dd", 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%#x", "d1", 0, "ddd", 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%#x", "day1", 0, "dddd", 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%#x", "dday1", 0, "ddddd", 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%#x", "1", 0, "M", 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%#x", "01", 0, "MM", 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%#x", "m1", 0, "MMM", 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%#x", "mon1", 0, "MMMM", 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%#x", "Mmon1", 0, "MMMMM", 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%#x", "y", 0, "y", 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%#x", "70", 0, "yy", 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%#x", "y70", 0, "yyy", 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%#x", "1970", 0, "yyyy", 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%#x", "y1970", 0, "yyyyy", 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
    };

    __lc_time_data time_data = {
        { "d1", "d2", "d3", "d4", "d5", "d6", "d7" },
        { "day1", "day2", "day3", "day4", "day5", "day6", "day7" },
        { "m1", "m2", "m3", "m4", "m5", "m6", "m7", "m8", "m9", "m10", "m11", "m12" },
        { "mon1", "mon2", "mon3", "mon4", "mon5", "mon6", "mon7", "mon8", "mon9", "mon10", "mon11", "mon12" },
        "tam", "tpm"
    };
    time_t gmt;
    struct tm* gmt_tm;
    char buf[256], bufA[256];
    WCHAR bufW[256];
    long retA, retW;
    int i;

    if (!p_strftime || !p_wcsftime || !p_gmtime)
    {
        win_skip("strftime, wcsftime or gmtime is not available\n");
        return;
    }

    setlocale(LC_TIME, "C");

    gmt = 0;
    gmt_tm = p_gmtime(&gmt);
    ok(gmt_tm != NULL, "gmtime failed\n");

    for (i=0; i<ARRAY_SIZE(tests_einval); i++)
    {
        errno = 0xdeadbeef;
        retA = p_strftime(bufA, 256, tests_einval[i].format, gmt_tm);
        ok(retA == 0, "%d) ret = %ld\n", i, retA);
        ok(errno==EINVAL || broken(errno==0xdeadbeef), "%d) errno = %d\n", i, errno);
    }

    errno = 0xdeadbeef;
    retA = p_strftime(NULL, 0, "copy", gmt_tm);
    ok(retA == 0, "expected 0, got %ld\n", retA);
    ok(errno==EINVAL || broken(errno==0xdeadbeef), "errno = %d\n", errno);

    retA = p_strftime(bufA, 256, "copy", NULL);
    ok(retA == 4, "expected 4, got %ld\n", retA);
    ok(!strcmp(bufA, "copy"), "got %s\n", bufA);

    retA = p_strftime(bufA, 256, "copy it", gmt_tm);
    ok(retA == 7, "expected 7, got %ld\n", retA);
    ok(!strcmp(bufA, "copy it"), "got %s\n", bufA);

    errno = 0xdeadbeef;
    retA = p_strftime(bufA, 2, "copy", gmt_tm);
    ok(retA == 0, "expected 0, got %ld\n", retA);
    ok(!strcmp(bufA, "") || broken(!strcmp(bufA, "copy it")), "got %s\n", bufA);
    ok(errno==ERANGE || errno==0xdeadbeef, "errno = %d\n", errno);

    errno = 0xdeadbeef;
    retA = p_strftime(bufA, 256, "a%e", gmt_tm);
    ok(retA==0 || broken(retA==1), "expected 0, got %ld\n", retA);
    ok(!strcmp(bufA, "") || broken(!strcmp(bufA, "a")), "got %s\n", bufA);
    ok(errno==EINVAL || broken(errno==0xdeadbeef), "errno = %d\n", errno);

    if(0) { /* crashes on Win2k */
        errno = 0xdeadbeef;
        retA = p_strftime(bufA, 256, "%c", NULL);
        ok(retA == 0, "expected 0, got %ld\n", retA);
        ok(!strcmp(bufA, ""), "got %s\n", bufA);
        ok(errno == EINVAL, "errno = %d\n", errno);
    }

    for (i=0; i<ARRAY_SIZE(tests); i++)
    {
        retA = p_strftime(bufA, 256, tests[i].format, &tests[i].tm);
        todo_wine_if(tests[i].todo) {
            ok(retA == strlen(tests[i].ret), "%d) ret = %ld\n", i, retA);
            ok(!strcmp(bufA, tests[i].ret), "%d) buf = \"%s\", expected \"%s\"\n",
                    i, bufA, tests[i].ret);
        }
    }

    retA = p_strftime(bufA, 256, "%c", gmt_tm);
    retW = p_wcsftime(bufW, 256, L"%c", gmt_tm);
    ok(retW == 17, "expected 17, got %ld\n", retW);
    ok(retA == retW, "expected %ld, got %ld\n", retA, retW);
    buf[0] = 0;
    retA = WideCharToMultiByte(CP_ACP, 0, bufW, retW, buf, 256, NULL, NULL);
    buf[retA] = 0;
    ok(strcmp(bufA, buf) == 0, "expected %s, got %s\n", bufA, buf);

    if(!setlocale(LC_ALL, "Japanese_Japan.932")) {
        win_skip("Japanese_Japan.932 locale not available\n");
        return;
    }

    /* test with multibyte character */
    retA = p_strftime(bufA, 256, "\x82%c", gmt_tm);
    ok(retA == 3, "expected 3, got %ld\n", retA);
    ok(!strcmp(bufA, "\x82%c"), "got %s\n", bufA);

    setlocale(LC_ALL, "C");
    if(!p__Strftime) {
        win_skip("_Strftime is not available\n");
        return;
    }

    /* TODO: find meaning of unk */
    time_data.unk = 1;
    for (i=0; i<ARRAY_SIZE(tests_td); i++)
    {
        time_data.short_date = tests_td[i].short_date;
        time_data.date = tests_td[i].date;
        time_data.time = tests_td[i].time;
        retA = p__Strftime(buf, sizeof(buf), tests_td[i].format, &tests_td[i].tm, &time_data);
        ok(retA == strlen(buf), "%d) ret = %ld\n", i, retA);
        todo_wine_if(tests_td[i].todo) {
            ok(!strcmp(buf, tests_td[i].ret), "%d) buf = \"%s\", expected \"%s\"\n",
                    i, buf, tests_td[i].ret);
        }
    }
}

static void test_asctime(void)
{
    struct tm* gmt_tm;
    time_t gmt;
    char *ret;

    if(!p_asctime || !p_gmtime)
    {
        win_skip("asctime or gmtime is not available\n");
        return;
    }

    gmt = 0;
    gmt_tm = p_gmtime(&gmt);
    ret = p_asctime(gmt_tm);
    ok(!strcmp(ret, "Thu Jan 01 00:00:00 1970\n"), "asctime returned %s\n", ret);

    gmt = 312433121;
    gmt_tm = p_gmtime(&gmt);
    ret = p_asctime(gmt_tm);
    ok(!strcmp(ret, "Mon Nov 26 02:58:41 1979\n"), "asctime returned %s\n", ret);

    /* Week day is only checked if it's in 0..6 range */
    gmt_tm->tm_wday = 3;
    ret = p_asctime(gmt_tm);
    ok(!strcmp(ret, "Wed Nov 26 02:58:41 1979\n"), "asctime returned %s\n", ret);

    errno = 0xdeadbeef;
    gmt_tm->tm_wday = 7;
    ret = p_asctime(gmt_tm);
    ok(!ret || broken(!ret[0]), "asctime returned %s\n", ret);
    ok(errno==EINVAL || broken(errno==0xdeadbeef), "errno = %d\n", errno);

    /* Year day is ignored */
    gmt_tm->tm_wday = 3;
    gmt_tm->tm_yday = 1300;
    ret = p_asctime(gmt_tm);
    ok(!strcmp(ret, "Wed Nov 26 02:58:41 1979\n"), "asctime returned %s\n", ret);

    /* Dates that can't be displayed using 26 characters are broken */
    gmt_tm->tm_mday = 28;
    gmt_tm->tm_year = 8100;
    ret = p_asctime(gmt_tm);
    ok(!strcmp(ret, "Wed Nov 28 02:58:41 :000\n"), "asctime returned %s\n", ret);

    gmt_tm->tm_year = 264100;
    ret = p_asctime(gmt_tm);
    ok(!strcmp(ret, "Wed Nov 28 02:58:41 :000\n"), "asctime returned %s\n", ret);

    /* asctime works from year 1900 */
    errno = 0xdeadbeef;
    gmt_tm->tm_year = -1;
    ret = p_asctime(gmt_tm);
    ok(!ret || broken(!strcmp(ret, "Wed Nov 28 02:58:41 190/\n")), "asctime returned %s\n", ret);
    ok(errno==EINVAL || broken(errno == 0xdeadbeef), "errno = %d\n", errno);

    errno = 0xdeadbeef;
    gmt_tm->tm_mon = 1;
    gmt_tm->tm_mday = 30;
    gmt_tm->tm_year = 79;
    ret = p_asctime(gmt_tm);
    ok(!ret || broken(!strcmp(ret, "Wed Feb 30 02:58:41 1979\n")), "asctime returned %s\n", ret);
    ok(errno==EINVAL || broken(errno==0xdeadbeef), "errno = %d\n", errno);
}

static void test__tzset(void)
{
    char TZ_env[256];
    int ret;

    if (sizeof(void*) != sizeof(int))
    {
        ok(!p___p__daylight, "___p__daylight() should be 32-bit only\n");
        ok(!p___p__timezone, "___p__timezone() should be 32-bit only\n");
        ok(!p___p__dstbias, "___p__dstbias() should be 32-bit only\n");
        return;
    }

    if (p__dstbias) {
        ret = *p__dstbias();
        ok(ret == -3600, "*__dstbias() = %d\n", ret);
        ret = *p___p__dstbias();
        ok(ret == -3600, "*__p__dstbias() = %d\n", ret);
    }
    else
        win_skip("__dstbias() is not available.\n");

    _snprintf(TZ_env,255,"TZ=%s",(getenv("TZ")?getenv("TZ"):""));

    ret = *p___p__daylight();
    ok(ret == 1, "*__p__daylight() = %d\n", ret);
    ret = *p___p__timezone();
    ok(ret == 28800, "*__p__timezone() = %d\n", ret);
    ret = *p___p__dstbias();
    ok(ret == -3600, "*__p__dstbias() = %d\n", ret);

    _putenv("TZ=xxx+1yyy");
    _tzset();
    ret = *p___p__daylight();
    ok(ret == 121, "*__p__daylight() = %d\n", ret);
    ret = *p___p__timezone();
    ok(ret == 3600, "*__p__timezone() = %d\n", ret);
    ret = *p___p__dstbias();
    ok(ret == -3600, "*__p__dstbias() = %d\n", ret);

    *p___p__dstbias() = 0;
    _putenv("TZ=xxx+1:3:5zzz");
    _tzset();
    ret = *p___p__daylight();
    ok(ret == 122, "*__p__daylight() = %d\n", ret);
    ret = *p___p__timezone();
    ok(ret == 3785, "*__p__timezone() = %d\n", ret);
    ret = *p___p__dstbias();
    ok(ret == 0, "*__p__dstbias() = %d\n", ret);

    _putenv(TZ_env);
}

START_TEST(time)
{
    init();

    test__tzset();
    test_strftime();
    test_ctime();
    test_gmtime();
    test_gmtime64();
    test_mktime();
    test_localtime();
    test_strdate();
    test_strtime();
    test_wstrdate();
    test_wstrtime();
    test_localtime32_s();
    test_localtime64_s();
    test_daylight();
    test_asctime();
}
