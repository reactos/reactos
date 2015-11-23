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

static __time32_t (__cdecl *p_mkgmtime32)(struct tm*);
static struct tm* (__cdecl *p_gmtime32)(__time32_t*);
static struct tm* (__cdecl *p_gmtime)(time_t*);
static errno_t    (__cdecl *p_gmtime32_s)(struct tm*, __time32_t*);
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
static size_t     (__cdecl *p_wcsftime)(wchar_t *, size_t, const wchar_t *, const struct tm *);
static char*      (__cdecl *p_asctime)(const struct tm *);

static void init(void)
{
    HMODULE hmod = LoadLibraryA("msvcrt.dll");

    p_gmtime32 = (void*)GetProcAddress(hmod, "_gmtime32");
    p_gmtime = (void*)GetProcAddress(hmod, "gmtime");
    p_gmtime32_s = (void*)GetProcAddress(hmod, "_gmtime32_s");
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
    ok(gmt == valid, "gmt = %u\n", gmt);
    ok(gmt_tm->tm_wday == 4, "gmt_tm->tm_wday = %d\n", gmt_tm->tm_wday);
    ok(gmt_tm->tm_yday == 0, "gmt_tm->tm_yday = %d\n", gmt_tm->tm_yday);

    gmt_tm->tm_wday = gmt_tm->tm_yday = 0;
    gmt_tm->tm_isdst = -1;
    gmt = p_mkgmtime32(gmt_tm);
    ok(gmt == valid, "gmt = %u\n", gmt);
    ok(gmt_tm->tm_wday == 4, "gmt_tm->tm_wday = %d\n", gmt_tm->tm_wday);
    ok(gmt_tm->tm_yday == 0, "gmt_tm->tm_yday = %d\n", gmt_tm->tm_yday);

    gmt_tm->tm_wday = gmt_tm->tm_yday = 0;
    gmt_tm->tm_isdst = 1;
    gmt = p_mkgmtime32(gmt_tm);
    ok(gmt == valid, "gmt = %u\n", gmt);
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
    ok(gmt == valid, "gmt = %u\n", gmt);
    ok(gmt_tm->tm_wday == 6, "gmt_tm->tm_wday = %d\n", gmt_tm->tm_wday);
    ok(gmt_tm->tm_yday == 2, "gmt_tm->tm_yday = %d\n", gmt_tm->tm_yday);

    gmt_tm->tm_isdst = 1;
    gmt = p_mkgmtime32(gmt_tm);
    ok(gmt == valid, "gmt = %u\n", gmt);

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
    trace( "bias %d std %d dst %d zone %s\n",
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
    ok(local_time == ref, "mktime returned %u, expected %u\n",
       (DWORD)local_time, (DWORD)ref);
    /* now test some unnormalized struct tm's */
    my_tm = sav_tm;
    my_tm.tm_sec += 60;
    my_tm.tm_min -= 1;
    local_time = mktime(&my_tm);
    ok(local_time == ref, "Unnormalized mktime returned %u, expected %u\n",
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
    ok(local_time == ref, "Unnormalized mktime returned %u, expected %u\n",
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
    ok(local_time == ref, "Unnormalized mktime returned %u, expected %u\n",
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
    ok(local_time == ref, "Unnormalized mktime returned %u, expected %u\n",
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
    ok(nulltime == ref,"mktime returned 0x%08x\n",(DWORD)nulltime);
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
    wchar_t format[] = { '%','0','2','d','/','%','0','2','d','/','%','0','2','d',0 };

    result = _wstrdate(date);
    ok(result == date, "Wrong return value\n");
    len = wcslen(date);
    ok(len == 8, "Wrong length: returned %d, should be 8\n", len);
    count = swscanf(date, format, &month, &day, &year);
    ok(count == 3, "Wrong format: count = %d, should be 3\n", count);
}

static void test_wstrtime(void)
{
    wchar_t time[16], * result;
    int hour, minute, second, count, len;
    wchar_t format[] = { '%','0','2','d',':','%','0','2','d',':','%','0','2','d',0 };

    result = _wstrtime(time);
    ok(result == time, "Wrong return value\n");
    len = wcslen(time);
    ok(len == 8, "Wrong length: returned %d, should be 8\n", len);
    count = swscanf(time, format, &hour, &minute, &second);
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

    if (!p___p__daylight)
    {
        win_skip("__p__daylight not available\n");
        return;
    }

    ret1 = p__daylight();
    ret2 = p___p__daylight();
    ok(ret1 && ret1 == ret2, "got %p\n", ret1);
}

static void test_strftime(void)
{
    static const wchar_t cW[] = { '%','c',0 };
    static const char expected[] = "01/01/70 00:00:00";
    time_t gmt;
    struct tm* gmt_tm;
    char buf[256], bufA[256];
    WCHAR bufW[256];
    long retA, retW;

    if (!p_strftime || !p_wcsftime || !p_gmtime)
    {
        win_skip("strftime, wcsftime or gmtime is not available\n");
        return;
    }

    setlocale(LC_TIME, "C");

    gmt = 0;
    gmt_tm = p_gmtime(&gmt);
    ok(gmt_tm != NULL, "gmtime failed\n");

    errno = 0xdeadbeef;
    retA = strftime(NULL, 0, "copy", gmt_tm);
    ok(retA == 0, "expected 0, got %ld\n", retA);
    ok(errno==EINVAL || broken(errno==0xdeadbeef), "errno = %d\n", errno);

    retA = strftime(bufA, 256, "copy", NULL);
    ok(retA == 4, "expected 4, got %ld\n", retA);
    ok(!strcmp(bufA, "copy"), "got %s\n", bufA);

    retA = strftime(bufA, 256, "copy it", gmt_tm);
    ok(retA == 7, "expected 7, got %ld\n", retA);
    ok(!strcmp(bufA, "copy it"), "got %s\n", bufA);

    errno = 0xdeadbeef;
    retA = strftime(bufA, 2, "copy", gmt_tm);
    ok(retA == 0, "expected 0, got %ld\n", retA);
    ok(!strcmp(bufA, "") || broken(!strcmp(bufA, "copy it")), "got %s\n", bufA);
    ok(errno==ERANGE || errno==0xdeadbeef, "errno = %d\n", errno);

    errno = 0xdeadbeef;
    retA = strftime(bufA, 256, "a%e", gmt_tm);
    ok(retA==0 || broken(retA==1), "expected 0, got %ld\n", retA);
    ok(!strcmp(bufA, "") || broken(!strcmp(bufA, "a")), "got %s\n", bufA);
    ok(errno==EINVAL || broken(errno==0xdeadbeef), "errno = %d\n", errno);

    if(0) { /* crashes on Win2k */
        errno = 0xdeadbeef;
        retA = strftime(bufA, 256, "%c", NULL);
        ok(retA == 0, "expected 0, got %ld\n", retA);
        ok(!strcmp(bufA, ""), "got %s\n", bufA);
        ok(errno == EINVAL, "errno = %d\n", errno);
    }

    retA = strftime(bufA, 256, "e%#%e", gmt_tm);
    ok(retA == 3, "expected 3, got %ld\n", retA);
    ok(!strcmp(bufA, "e%e"), "got %s\n", bufA);

    retA = strftime(bufA, 256, "%c", gmt_tm);
    ok(retA == 17, "expected 17, got %ld\n", retA);
    ok(strcmp(bufA, expected) == 0, "expected %s, got %s\n", expected, bufA);

    retW = wcsftime(bufW, 256, cW, gmt_tm);
    ok(retW == 17, "expected 17, got %ld\n", retW);
    ok(retA == retW, "expected %ld, got %ld\n", retA, retW);
    buf[0] = 0;
    retA = WideCharToMultiByte(CP_ACP, 0, bufW, retW, buf, 256, NULL, NULL);
    buf[retA] = 0;
    ok(strcmp(bufA, buf) == 0, "expected %s, got %s\n", bufA, buf);

    retA = strftime(bufA, 256, "%x", gmt_tm);
    ok(retA == 8, "expected 8, got %ld\n", retA);
    ok(!strcmp(bufA, "01/01/70"), "got %s\n", bufA);

    retA = strftime(bufA, 256, "%X", gmt_tm);
    ok(retA == 8, "expected 8, got %ld\n", retA);
    ok(!strcmp(bufA, "00:00:00"), "got %s\n", bufA);

    retA = strftime(bufA, 256, "%a", gmt_tm);
    ok(retA == 3, "expected 3, got %ld\n", retA);
    ok(!strcmp(bufA, "Thu"), "got %s\n", bufA);

    retA = strftime(bufA, 256, "%A", gmt_tm);
    ok(retA == 8, "expected 8, got %ld\n", retA);
    ok(!strcmp(bufA, "Thursday"), "got %s\n", bufA);

    retA = strftime(bufA, 256, "%b", gmt_tm);
    ok(retA == 3, "expected 3, got %ld\n", retA);
    ok(!strcmp(bufA, "Jan"), "got %s\n", bufA);

    retA = strftime(bufA, 256, "%B", gmt_tm);
    ok(retA == 7, "expected 7, got %ld\n", retA);
    ok(!strcmp(bufA, "January"), "got %s\n", bufA);

    retA = strftime(bufA, 256, "%d", gmt_tm);
    ok(retA == 2, "expected 2, got %ld\n", retA);
    ok(!strcmp(bufA, "01"), "got %s\n", bufA);

    retA = strftime(bufA, 256, "%#d", gmt_tm);
    ok(retA == 1, "expected 1, got %ld\n", retA);
    ok(!strcmp(bufA, "1"), "got %s\n", bufA);

    retA = strftime(bufA, 256, "%H", gmt_tm);
    ok(retA == 2, "expected 2, got %ld\n", retA);
    ok(!strcmp(bufA, "00"), "got %s\n", bufA);

    retA = strftime(bufA, 256, "%I", gmt_tm);
    ok(retA == 2, "expected 2, got %ld\n", retA);
    ok(!strcmp(bufA, "12"), "got %s\n", bufA);

    retA = strftime(bufA, 256, "%j", gmt_tm);
    ok(retA == 3, "expected 3, got %ld\n", retA);
    ok(!strcmp(bufA, "001"), "got %s\n", bufA);

    retA = strftime(bufA, 256, "%m", gmt_tm);
    ok(retA == 2, "expected 2, got %ld\n", retA);
    ok(!strcmp(bufA, "01"), "got %s\n", bufA);

    retA = strftime(bufA, 256, "%#M", gmt_tm);
    ok(retA == 1, "expected 1, got %ld\n", retA);
    ok(!strcmp(bufA, "0"), "got %s\n", bufA);

    retA = strftime(bufA, 256, "%p", gmt_tm);
    ok(retA == 2, "expected 2, got %ld\n", retA);
    ok(!strcmp(bufA, "AM"), "got %s\n", bufA);

    retA = strftime(bufA, 256, "%U", gmt_tm);
    ok(retA == 2, "expected 2, got %ld\n", retA);
    ok(!strcmp(bufA, "00"), "got %s\n", bufA);

    retA = strftime(bufA, 256, "%W", gmt_tm);
    ok(retA == 2, "expected 2, got %ld\n", retA);
    ok(!strcmp(bufA, "00"), "got %s\n", bufA);

    gmt_tm->tm_wday = 0;
    retA = strftime(bufA, 256, "%U", gmt_tm);
    ok(retA == 2, "expected 2, got %ld\n", retA);
    ok(!strcmp(bufA, "01"), "got %s\n", bufA);

    retA = strftime(bufA, 256, "%W", gmt_tm);
    ok(retA == 2, "expected 2, got %ld\n", retA);
    ok(!strcmp(bufA, "00"), "got %s\n", bufA);

    gmt_tm->tm_yday = 365;
    retA = strftime(bufA, 256, "%U", gmt_tm);
    ok(retA == 2, "expected 2, got %ld\n", retA);
    ok(!strcmp(bufA, "53"), "got %s\n", bufA);

    retA = strftime(bufA, 256, "%W", gmt_tm);
    ok(retA == 2, "expected 2, got %ld\n", retA);
    ok(!strcmp(bufA, "52"), "got %s\n", bufA);

    gmt_tm->tm_mon = 1;
    gmt_tm->tm_mday = 30;
    retA = strftime(bufA, 256, "%c", gmt_tm);
    todo_wine {
        ok(retA == 17, "expected 17, got %ld\n", retA);
        ok(!strcmp(bufA, "02/30/70 00:00:00"), "got %s\n", bufA);
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

    if(!p___p__daylight || !p___p__timezone || !p___p__dstbias) {
        win_skip("__p__daylight, __p__timezone or __p__dstbias is not available\n");
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

static void test_clock(void)
{
    static const int THRESH = 50;
    clock_t s, e;
    int i;

    for (i = 0; i < 10; i++)
    {
        s = clock();
        Sleep(1000);
        e = clock();

        ok(abs((e-s) - 1000) < THRESH, "clock off on loop %i: %i\n", i, e-s);
    }
}

START_TEST(time)
{
    init();

    test__tzset();
    test_strftime();
    test_ctime();
    test_gmtime();
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
    test_clock();
}
