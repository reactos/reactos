/*
 * Unit test suite for time functions
 *
 * Copyright 2004 Uwe Bonnes
 * Copyright 2007 Dmitry Timoshkov
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
#include "winternl.h"

static BOOL (WINAPI *pTzSpecificLocalTimeToSystemTime)(LPTIME_ZONE_INFORMATION, LPSYSTEMTIME, LPSYSTEMTIME);
static BOOL (WINAPI *pSystemTimeToTzSpecificLocalTime)(LPTIME_ZONE_INFORMATION, LPSYSTEMTIME, LPSYSTEMTIME);
static BOOL (WINAPI *pGetSystemTimes)(LPFILETIME, LPFILETIME, LPFILETIME);
static int (WINAPI *pGetCalendarInfoA)(LCID,CALID,CALTYPE,LPSTR,int,LPDWORD);
static int (WINAPI *pGetCalendarInfoW)(LCID,CALID,CALTYPE,LPWSTR,int,LPDWORD);
static DWORD (WINAPI *pGetDynamicTimeZoneInformation)(DYNAMIC_TIME_ZONE_INFORMATION*);
static void (WINAPI *pGetSystemTimePreciseAsFileTime)(LPFILETIME);
static BOOL (WINAPI *pGetTimeZoneInformationForYear)(USHORT, PDYNAMIC_TIME_ZONE_INFORMATION, LPTIME_ZONE_INFORMATION);
static ULONG (WINAPI *pNtGetTickCount)(void);

#define SECSPERMIN         60
#define SECSPERDAY        86400
/* 1601 to 1970 is 369 years plus 89 leap days */
#define SECS_1601_TO_1970  ((369 * 365 + 89) * (ULONGLONG)SECSPERDAY)
#define TICKSPERSEC       10000000
#define TICKSPERMSEC      10000
#define TICKS_1601_TO_1970 (SECS_1601_TO_1970 * TICKSPERSEC)


#define NEWYEAR_1980_HI 0x01a8e79f
#define NEWYEAR_1980_LO 0xe1d58000

#define MAYDAY_2002_HI 0x01c1f107
#define MAYDAY_2002_LO 0xb82b6000

#define ATIME_HI  0x1c2349b
#define ATIME_LOW 0x580716b0

#define LOCAL_ATIME_HI  0x01c23471
#define LOCAL_ATIME_LOW 0x6f310eb0

#define DOS_DATE(y,m,d) ( (((y)-1980)<<9) | ((m)<<5) | (d) )
#define DOS_TIME(h,m,s) ( ((h)<<11) | ((m)<<5) | ((s)>>1) )


#define SETUP_1980(st) \
    (st).wYear = 1980; \
    (st).wMonth = 1; \
    (st).wDay = 1; \
    (st).wHour = 0; \
    (st).wMinute = 0; \
    (st).wSecond = 0; \
    (st).wMilliseconds = 0;

#define SETUP_2002(st) \
    (st).wYear = 2002; \
    (st).wMonth = 5; \
    (st).wDay = 1; \
    (st).wHour = 12; \
    (st).wMinute = 0; \
    (st).wSecond = 0; \
    (st).wMilliseconds = 0;

#define SETUP_ATIME(st) \
    (st).wYear = 2002; \
    (st).wMonth = 7; \
    (st).wDay = 26; \
    (st).wHour = 11; \
    (st).wMinute = 55; \
    (st).wSecond = 32; \
    (st).wMilliseconds = 123;

#define SETUP_ZEROTIME(st) \
    (st).wYear = 1601; \
    (st).wMonth = 1; \
    (st).wDay = 1; \
    (st).wHour = 0; \
    (st).wMinute = 0; \
    (st).wSecond = 0; \
    (st).wMilliseconds = 0;

#define SETUP_EARLY(st) \
    (st).wYear = 1600; \
    (st).wMonth = 12; \
    (st).wDay = 31; \
    (st).wHour = 23; \
    (st).wMinute = 59; \
    (st).wSecond = 59; \
    (st).wMilliseconds = 999;


static void test_conversions(void)
{
    FILETIME ft;
    SYSTEMTIME st;

    memset(&ft,0,sizeof ft);

    SetLastError(0xdeadbeef);
    SETUP_EARLY(st)
    ok (!SystemTimeToFileTime(&st, &ft), "Conversion succeeded EARLY\n");
    ok (GetLastError() == ERROR_INVALID_PARAMETER ||
        GetLastError() == 0xdeadbeef, /* win9x */
        "EARLY should be INVALID\n");

    SETUP_ZEROTIME(st)
    ok (SystemTimeToFileTime(&st, &ft), "Conversion failed ZERO_TIME\n");
    ok( (!((ft.dwHighDateTime != 0) || (ft.dwLowDateTime != 0))),
        "Wrong time for ATIME: %08lx %08lx (correct %08x %08x)\n",
        ft.dwLowDateTime, ft.dwHighDateTime, 0, 0);


    SETUP_ATIME(st)
    ok (SystemTimeToFileTime(&st,&ft), "Conversion Failed ATIME\n");
    ok( (!((ft.dwHighDateTime != ATIME_HI) || (ft.dwLowDateTime!=ATIME_LOW))),
        "Wrong time for ATIME: %08lx %08lx (correct %08x %08x)\n",
        ft.dwLowDateTime, ft.dwHighDateTime, ATIME_LOW, ATIME_HI);


    SETUP_2002(st)
    ok (SystemTimeToFileTime(&st, &ft), "Conversion failed 2002\n");

    ok( (!((ft.dwHighDateTime != MAYDAY_2002_HI) ||
         (ft.dwLowDateTime!=MAYDAY_2002_LO))),
        "Wrong time for 2002 %08lx %08lx (correct %08x %08x)\n", ft.dwLowDateTime,
        ft.dwHighDateTime, MAYDAY_2002_LO, MAYDAY_2002_HI);


    SETUP_1980(st)
    ok((SystemTimeToFileTime(&st, &ft)), "Conversion failed 1980\n");

    ok( (!((ft.dwHighDateTime!=NEWYEAR_1980_HI) ||
        (ft.dwLowDateTime!=NEWYEAR_1980_LO))) ,
        "Wrong time for 1980 %08lx %08lx (correct %08x %08x)\n", ft.dwLowDateTime,
         ft.dwHighDateTime, NEWYEAR_1980_LO,NEWYEAR_1980_HI  );

    ok(DosDateTimeToFileTime(DOS_DATE(1980,1,1),DOS_TIME(0,0,0),&ft),
        "DosDateTimeToFileTime() failed\n");

    ok( (!((ft.dwHighDateTime!=NEWYEAR_1980_HI) ||
         (ft.dwLowDateTime!=NEWYEAR_1980_LO))),
        "Wrong time DosDateTimeToFileTime %08lx %08lx (correct %08x %08x)\n",
        ft.dwHighDateTime, ft.dwLowDateTime, NEWYEAR_1980_HI, NEWYEAR_1980_LO);

}

static void test_invalid_arg(void)
{
    FILETIME ft;
    SYSTEMTIME st;


    /* Invalid argument checks */

    memset(&ft,0,sizeof ft);
    ok( DosDateTimeToFileTime(DOS_DATE(1980,1,1),DOS_TIME(0,0,0),&ft), /* this is 1 Jan 1980 00:00:00 */
        "DosDateTimeToFileTime() failed\n");

    ok( (ft.dwHighDateTime==NEWYEAR_1980_HI) && (ft.dwLowDateTime==NEWYEAR_1980_LO),
        "filetime for 1/1/80 00:00:00 was %08lx %08lx\n", ft.dwHighDateTime, ft.dwLowDateTime);

    /* now check SystemTimeToFileTime */
    memset(&ft,0,sizeof ft);


    /* try with a bad month */
    SETUP_1980(st)
    st.wMonth = 0;

    ok( !SystemTimeToFileTime(&st, &ft), "bad month\n");

    /* with a bad hour */
    SETUP_1980(st)
    st.wHour = 24;

    ok( !SystemTimeToFileTime(&st, &ft), "bad hour\n");

    /* with a bad minute */
    SETUP_1980(st)
    st.wMinute = 60;

    ok( !SystemTimeToFileTime(&st, &ft), "bad minute\n");
}

static LONGLONG system_time_to_minutes(const SYSTEMTIME *st)
{
    BOOL ret;
    FILETIME ft;
    LONGLONG minutes;

    SetLastError(0xdeadbeef);
    ret = SystemTimeToFileTime(st, &ft);
    ok(ret, "SystemTimeToFileTime error %lu\n", GetLastError());

    minutes = ((LONGLONG)ft.dwHighDateTime << 32) + ft.dwLowDateTime;
    minutes /= (LONGLONG)600000000; /* convert to minutes */
    return minutes;
}

static LONG get_tz_bias(const TIME_ZONE_INFORMATION *tzinfo, DWORD tz_id)
{
    switch (tz_id)
    {
    case TIME_ZONE_ID_DAYLIGHT:
        if (memcmp(&tzinfo->StandardDate, &tzinfo->DaylightDate, sizeof(tzinfo->DaylightDate)) != 0)
            return tzinfo->DaylightBias;
        /* fall through */

    case TIME_ZONE_ID_STANDARD:
        return tzinfo->StandardBias;

    default:
        trace("unknown time zone id %ld\n", tz_id);
        /* fall through */
    case TIME_ZONE_ID_UNKNOWN:
        return 0;
    }
}
 
static void test_GetTimeZoneInformation(void)
{
    char std_name[32], dlt_name[32];
    TIME_ZONE_INFORMATION tzinfo, tzinfo1;
    BOOL res;
    DWORD tz_id;
    SYSTEMTIME st, current, utc, local;
    FILETIME l_ft, s_ft;
    LONGLONG l_time, s_time;
    LONG diff;

    GetSystemTime(&st);
    s_time = system_time_to_minutes(&st);

    SetLastError(0xdeadbeef);
    res = SystemTimeToFileTime(&st, &s_ft);
    ok(res, "SystemTimeToFileTime error %lu\n", GetLastError());
    SetLastError(0xdeadbeef);
    res = FileTimeToLocalFileTime(&s_ft, &l_ft);
    ok(res, "FileTimeToLocalFileTime error %lu\n", GetLastError());
    SetLastError(0xdeadbeef);
    res = FileTimeToSystemTime(&l_ft, &local);
    ok(res, "FileTimeToSystemTime error %lu\n", GetLastError());
    l_time = system_time_to_minutes(&local);

    tz_id = GetTimeZoneInformation(&tzinfo);
    ok(tz_id != TIME_ZONE_ID_INVALID, "GetTimeZoneInformation failed\n");

    trace("tz_id %lu (%s)\n", tz_id,
          tz_id == TIME_ZONE_ID_DAYLIGHT ? "TIME_ZONE_ID_DAYLIGHT" :
          (tz_id == TIME_ZONE_ID_STANDARD ? "TIME_ZONE_ID_STANDARD" :
          (tz_id == TIME_ZONE_ID_UNKNOWN ? "TIME_ZONE_ID_UNKNOWN" :
          "TIME_ZONE_ID_INVALID")));

    WideCharToMultiByte(CP_ACP, 0, tzinfo.StandardName, -1, std_name, sizeof(std_name), NULL, NULL);
    WideCharToMultiByte(CP_ACP, 0, tzinfo.DaylightName, -1, dlt_name, sizeof(dlt_name), NULL, NULL);
    trace("bias %ld, %s - %s\n", tzinfo.Bias, std_name, dlt_name);
    trace("standard (d/m/y): %u/%02u/%04u day of week %u %u:%02u:%02u.%03u bias %ld\n",
        tzinfo.StandardDate.wDay, tzinfo.StandardDate.wMonth,
        tzinfo.StandardDate.wYear, tzinfo.StandardDate.wDayOfWeek,
        tzinfo.StandardDate.wHour, tzinfo.StandardDate.wMinute,
        tzinfo.StandardDate.wSecond, tzinfo.StandardDate.wMilliseconds,
        tzinfo.StandardBias);
    trace("daylight (d/m/y): %u/%02u/%04u day of week %u %u:%02u:%02u.%03u bias %ld\n",
        tzinfo.DaylightDate.wDay, tzinfo.DaylightDate.wMonth,
        tzinfo.DaylightDate.wYear, tzinfo.DaylightDate.wDayOfWeek,
        tzinfo.DaylightDate.wHour, tzinfo.DaylightDate.wMinute,
        tzinfo.DaylightDate.wSecond, tzinfo.DaylightDate.wMilliseconds,
        tzinfo.DaylightBias);

    diff = (LONG)(s_time - l_time);
    ok(diff == tzinfo.Bias + get_tz_bias(&tzinfo, tz_id),
       "system/local diff %ld != tz bias %ld\n",
       diff, tzinfo.Bias + get_tz_bias(&tzinfo, tz_id));

    ok(SetEnvironmentVariableA("TZ","GMT0") != 0,
       "SetEnvironmentVariableA failed\n");
    res =  GetTimeZoneInformation(&tzinfo1);
    ok(res != TIME_ZONE_ID_INVALID, "GetTimeZoneInformation failed\n");

    ok(((tzinfo.Bias == tzinfo1.Bias) && 
	(tzinfo.StandardBias == tzinfo1.StandardBias) &&
	(tzinfo.DaylightBias == tzinfo1.DaylightBias)),
       "Bias influenced by TZ variable\n"); 
    ok(SetEnvironmentVariableA("TZ",NULL) != 0,
       "SetEnvironmentVariableA failed\n");

    if (!pSystemTimeToTzSpecificLocalTime)
    {
        win_skip("SystemTimeToTzSpecificLocalTime not available\n");
        return;
    }

    diff = get_tz_bias(&tzinfo, tz_id);

    utc = st;
    SetLastError(0xdeadbeef);
    res = pSystemTimeToTzSpecificLocalTime(&tzinfo, &utc, &current);
    if (!res && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("SystemTimeToTzSpecificLocalTime is not implemented\n");
        return;
    }

    ok(res, "SystemTimeToTzSpecificLocalTime error %lu\n", GetLastError());
    s_time = system_time_to_minutes(&current);

    tzinfo.StandardBias -= 123;
    tzinfo.DaylightBias += 456;

    res = pSystemTimeToTzSpecificLocalTime(&tzinfo, &utc, &local);
    ok(res, "SystemTimeToTzSpecificLocalTime error %lu\n", GetLastError());
    l_time = system_time_to_minutes(&local);
    ok(l_time - s_time == diff - get_tz_bias(&tzinfo, tz_id), "got %ld, expected %ld\n",
       (LONG)(l_time - s_time), diff - get_tz_bias(&tzinfo, tz_id));

    /* pretend that there is no transition dates */
    tzinfo.DaylightDate.wDay = 0;
    tzinfo.DaylightDate.wMonth = 0;
    tzinfo.DaylightDate.wYear = 0;
    tzinfo.StandardDate.wDay = 0;
    tzinfo.StandardDate.wMonth = 0;
    tzinfo.StandardDate.wYear = 0;

    res = pSystemTimeToTzSpecificLocalTime(&tzinfo, &utc, &local);
    ok(res, "SystemTimeToTzSpecificLocalTime error %lu\n", GetLastError());
    l_time = system_time_to_minutes(&local);
    ok(l_time - s_time == diff, "got %ld, expected %ld\n",
       (LONG)(l_time - s_time), diff);

    /* test 23:01, 31st of December date */
    memset(&tzinfo, 0, sizeof(tzinfo));
    tzinfo.StandardDate.wMonth = 10;
    tzinfo.StandardDate.wDay = 5;
    tzinfo.StandardDate.wHour = 2;
    tzinfo.StandardDate.wMinute = 0;
    tzinfo.DaylightDate.wMonth = 4;
    tzinfo.DaylightDate.wDay = 1;
    tzinfo.DaylightDate.wHour = 2;
    tzinfo.Bias = 0;
    tzinfo.StandardBias = 0;
    tzinfo.DaylightBias = -60;
    utc.wYear = 2012;
    utc.wMonth = 12;
    utc.wDay = 31;
    utc.wHour = 23;
    utc.wMinute = 1;
    res = pSystemTimeToTzSpecificLocalTime(&tzinfo, &utc, &local);
    ok(res, "SystemTimeToTzSpecificLocalTime error %lu\n", GetLastError());
    ok(local.wYear==2012 && local.wMonth==12 && local.wDay==31 && local.wHour==23 && local.wMinute==1,
            "got (%d-%d-%d %02d:%02d), expected (2012-12-31 23:01)\n",
            local.wYear, local.wMonth, local.wDay, local.wHour, local.wMinute);
}

static void test_FileTimeToSystemTime(void)
{
    FILETIME ft;
    SYSTEMTIME st;
    ULONGLONG time = (ULONGLONG)TICKSPERSEC + TICKS_1601_TO_1970;
    BOOL ret;

    ft.dwHighDateTime = 0;
    ft.dwLowDateTime  = 0;
    ret = FileTimeToSystemTime(&ft, &st);
    ok( ret,
       "FileTimeToSystemTime() failed with Error %ld\n",GetLastError());
    ok(((st.wYear == 1601) && (st.wMonth  == 1) && (st.wDay    == 1) &&
	(st.wHour ==    0) && (st.wMinute == 0) && (st.wSecond == 0) &&
	(st.wMilliseconds == 0)),
	"Got Year %4d Month %2d Day %2d\n",  st.wYear, st.wMonth, st.wDay);

    ft.dwHighDateTime = (UINT)(time >> 32);
    ft.dwLowDateTime  = (UINT)time;
    ret = FileTimeToSystemTime(&ft, &st);
    ok( ret,
       "FileTimeToSystemTime() failed with Error %ld\n",GetLastError());
    ok(((st.wYear == 1970) && (st.wMonth == 1) && (st.wDay == 1) &&
	(st.wHour ==    0) && (st.wMinute == 0) && (st.wSecond == 1) &&
	(st.wMilliseconds == 0)),
       "Got Year %4d Month %2d Day %2d Hour %2d Min %2d Sec %2d mSec %3d\n",
       st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond,
       st.wMilliseconds);

    ft.dwHighDateTime = -1;
    ft.dwLowDateTime  = -1;
    SetLastError(0xdeadbeef);
    ret = FileTimeToSystemTime(&ft, &st);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());
}

static void test_FileTimeToLocalFileTime(void)
{
    FILETIME ft, lft;
    SYSTEMTIME st;
    TIME_ZONE_INFORMATION tzinfo;
    DWORD res =  GetTimeZoneInformation(&tzinfo);
    ULONGLONG time = (ULONGLONG)TICKSPERSEC + TICKS_1601_TO_1970 +
        (LONGLONG)(tzinfo.Bias + 
            ( res == TIME_ZONE_ID_STANDARD ? tzinfo.StandardBias :
            ( res == TIME_ZONE_ID_DAYLIGHT ? tzinfo.DaylightBias : 0 ))) *
             SECSPERMIN *TICKSPERSEC;
    BOOL ret;

    ok( res != TIME_ZONE_ID_INVALID , "GetTimeZoneInformation failed\n");
    ft.dwHighDateTime = (UINT)(time >> 32);
    ft.dwLowDateTime  = (UINT)time;
    ret = FileTimeToLocalFileTime(&ft, &lft);
    ok( ret,
       "FileTimeToLocalFileTime() failed with Error %ld\n",GetLastError());
    FileTimeToSystemTime(&lft, &st);
    ok(((st.wYear == 1970) && (st.wMonth == 1) && (st.wDay == 1) &&
	(st.wHour ==    0) && (st.wMinute == 0) && (st.wSecond == 1) &&
	(st.wMilliseconds == 0)),
       "Got Year %4d Month %2d Day %2d Hour %2d Min %2d Sec %2d mSec %3d\n",
       st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond,
       st.wMilliseconds);

    ok(SetEnvironmentVariableA("TZ","GMT") != 0,
       "SetEnvironmentVariableA failed\n");
    ok(res != TIME_ZONE_ID_INVALID, "GetTimeZoneInformation failed\n");
    ret = FileTimeToLocalFileTime(&ft, &lft);
    ok( ret,
       "FileTimeToLocalFileTime() failed with Error %ld\n",GetLastError());
    FileTimeToSystemTime(&lft, &st);
    ok(((st.wYear == 1970) && (st.wMonth == 1) && (st.wDay == 1) &&
	(st.wHour ==    0) && (st.wMinute == 0) && (st.wSecond == 1) &&
	(st.wMilliseconds == 0)),
       "Got Year %4d Month %2d Day %2d Hour %2d Min %2d Sec %2d mSec %3d\n",
       st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond,
       st.wMilliseconds);
    ok(SetEnvironmentVariableA("TZ",NULL) != 0,
       "SetEnvironmentVariableA failed\n");
}

typedef struct {
    int nr;             /* test case number for easier lookup */
    TIME_ZONE_INFORMATION *ptz; /* ptr to timezone */
    SYSTEMTIME slt;     /* system/local time to convert */
    WORD ehour;        /* expected hour */
} TZLT2ST_case;

static void test_TzSpecificLocalTimeToSystemTime(void)
{    
    TIME_ZONE_INFORMATION tzE, tzW, tzS;
    SYSTEMTIME result;
    int i, j, year;

    if (!pTzSpecificLocalTimeToSystemTime || !pSystemTimeToTzSpecificLocalTime)
    {
        win_skip("TzSpecificLocalTimeToSystemTime or SystemTimeToTzSpecificLocalTime not available\n");
        return;
    }

    ZeroMemory( &tzE, sizeof(tzE));
    ZeroMemory( &tzW, sizeof(tzW));
    ZeroMemory( &tzS, sizeof(tzS));
    /* timezone Eastern hemisphere */
    tzE.Bias=-600;
    tzE.StandardBias=0;
    tzE.DaylightBias=-60;
    tzE.StandardDate.wMonth=10;
    tzE.StandardDate.wDayOfWeek=0; /* Sunday */
    tzE.StandardDate.wDay=5;       /* last (Sunday) of the month */
    tzE.StandardDate.wHour=3;
    tzE.DaylightDate.wMonth=3;
    tzE.DaylightDate.wDay=5;
    tzE.DaylightDate.wHour=2;
    /* timezone Western hemisphere */
    tzW.Bias=240;
    tzW.StandardBias=0;
    tzW.DaylightBias=-60;
    tzW.StandardDate.wMonth=10;
    tzW.StandardDate.wDayOfWeek=0; /* Sunday */
    tzW.StandardDate.wDay=4;       /* 4th (Sunday) of the month */
    tzW.StandardDate.wHour=2;
    tzW.DaylightDate.wMonth=4;
    tzW.DaylightDate.wDay=1;
    tzW.DaylightDate.wHour=2;
    /* timezone Southern hemisphere */
    tzS.Bias=240;
    tzS.StandardBias=0;
    tzS.DaylightBias=-60;
    tzS.StandardDate.wMonth=4;
    tzS.StandardDate.wDayOfWeek=0; /*Sunday */
    tzS.StandardDate.wDay=1;       /* 1st (Sunday) of the month */
    tzS.StandardDate.wHour=2;
    tzS.DaylightDate.wMonth=10;
    tzS.DaylightDate.wDay=4;
    tzS.DaylightDate.wHour=2;
    /*tests*/
        /* TzSpecificLocalTimeToSystemTime */
    {   TZLT2ST_case cases[] = {
            /* standard->daylight transition */
            { 1, &tzE, {2004,3,-1,28,1,0,0,0}, 15 },
            { 2, &tzE, {2004,3,-1,28,1,59,59,999}, 15},
            { 3, &tzE, {2004,3,-1,28,2,0,0,0}, 15},
            /* daylight->standard transition */
            { 4, &tzE, {2004,10,-1,31,2,0,0,0} , 15 },
            { 5, &tzE, {2004,10,-1,31,2,59,59,999}, 15 },
            { 6, &tzE, {2004,10,-1,31,3,0,0,0}, 17 },
            /* West and with fixed weekday of the month */
            { 7, &tzW, {2004,4,-1,4,1,0,0,0}, 5},
            { 8, &tzW, {2004,4,-1,4,1,59,59,999}, 5},
            { 9, &tzW, {2004,4,-1,4,2,0,0,0}, 5},
            { 10, &tzW, {2004,10,-1,24,1,0,0,0}, 4},
            { 11, &tzW, {2004,10,-1,24,1,59,59,999}, 4},
            { 12, &tzW, {2004,10,-1,24,2,0,0,0 }, 6},
            /* and now South */
            { 13, &tzS, {2004,4,-1,4,1,0,0,0}, 4},
            { 14, &tzS, {2004,4,-1,4,1,59,59,999}, 4},
            { 15, &tzS, {2004,4,-1,4,2,0,0,0}, 6},
            { 16, &tzS, {2004,10,-1,24,1,0,0,0}, 5},
            { 17, &tzS, {2004,10,-1,24,1,59,59,999}, 5},
            { 18, &tzS, {2004,10,-1,24,2,0,0,0}, 5},
            {0}
        };
    /*  days of transitions to put into the cases array */
        int yeardays[][6]=
        {
              {28,31,4,24,4,24}  /* 1999 */
            , {26,29,2,22,2,22}  /* 2000 */
            , {25,28,1,28,1,28}  /* 2001 */
            , {31,27,7,27,7,27}  /* 2002 */
            , {30,26,6,26,6,26}  /* 2003 */
            , {28,31,4,24,4,24}  /* 2004 */
            , {27,30,3,23,3,23}  /* 2005 */
            , {26,29,2,22,2,22}  /* 2006 */
            , {25,28,1,28,1,28}  /* 2007 */
            , {30,26,6,26,6,26}  /* 2008 */
            , {29,25,5,25,5,25}  /* 2009 */
            , {28,31,4,24,4,24}  /* 2010 */
            , {27,30,3,23,3,23}  /* 2011 */
            , {25,28,1,28,1,28}  /* 2012 */
            , {31,27,7,27,7,27}  /* 2013 */
            , {30,26,6,26,6,26}  /* 2014 */
            , {29,25,5,25,5,25}  /* 2015 */
            , {27,30,3,23,3,23}  /* 2016 */
            , {26,29,2,22,2,22}  /* 2017 */
            , {25,28,1,28,1,28}  /* 2018 */
            , {31,27,7,27,7,27}  /* 2019 */
            ,{0}
        };
        for( j=0 , year = 1999; yeardays[j][0] ; j++, year++) {
            for (i=0; cases[i].nr; i++) {
                if(i) cases[i].nr += 18;
                cases[i].slt.wYear = year;
                cases[i].slt.wDay = yeardays[j][i/3];
                pTzSpecificLocalTimeToSystemTime( cases[i].ptz, &(cases[i].slt), &result);
                ok( result.wHour == cases[i].ehour,
                        "Test TzSpecificLocalTimeToSystemTime #%d. Got %4d-%02d-%02d %02d:%02d. Expect hour =  %02d\n", 
                        cases[i].nr, result.wYear, result.wMonth, result.wDay,
                        result.wHour, result.wMinute, cases[i].ehour);
            }
        }
    }
        /* SystemTimeToTzSpecificLocalTime */
    {   TZLT2ST_case cases[] = {
            /* standard->daylight transition */
            { 1, &tzE, {2004,3,-1,27,15,0,0,0}, 1 },
            { 2, &tzE, {2004,3,-1,27,15,59,59,999}, 1},
            { 3, &tzE, {2004,3,-1,27,16,0,0,0}, 3},
            /* daylight->standard transition */
            { 4,  &tzE, {2004,10,-1,30,15,0,0,0}, 2 },
            { 5, &tzE, {2004,10,-1,30,15,59,59,999}, 2 },
            { 6, &tzE, {2004,10,-1,30,16,0,0,0}, 2 },
            /* West and with fixed weekday of the month */
            { 7, &tzW, {2004,4,-1,4,5,0,0,0}, 1},
            { 8, &tzW, {2004,4,-1,4,5,59,59,999}, 1},
            { 9, &tzW, {2004,4,-1,4,6,0,0,0}, 3},
            { 10, &tzW, {2004,10,-1,24,4,0,0,0}, 1},
            { 11, &tzW, {2004,10,-1,24,4,59,59,999}, 1},
            { 12, &tzW, {2004,10,-1,24,5,0,0,0 }, 1},
            /* and now South */
            { 13, &tzS, {2004,4,-1,4,4,0,0,0}, 1},
            { 14, &tzS, {2004,4,-1,4,4,59,59,999}, 1},
            { 15, &tzS, {2004,4,-1,4,5,0,0,0}, 1},
            { 16, &tzS, {2004,10,-1,24,5,0,0,0}, 1},
            { 17, &tzS, {2004,10,-1,24,5,59,59,999}, 1},
            { 18, &tzS, {2004,10,-1,24,6,0,0,0}, 3},

            {0}
        }; 
    /*  days of transitions to put into the cases array */
        int yeardays[][6]=
        {
              {27,30,4,24,4,24}  /* 1999 */
            , {25,28,2,22,2,22}  /* 2000 */
            , {24,27,1,28,1,28}  /* 2001 */
            , {30,26,7,27,7,27}  /* 2002 */
            , {29,25,6,26,6,26}  /* 2003 */
            , {27,30,4,24,4,24}  /* 2004 */
            , {26,29,3,23,3,23}  /* 2005 */
            , {25,28,2,22,2,22}  /* 2006 */
            , {24,27,1,28,1,28}  /* 2007 */
            , {29,25,6,26,6,26}  /* 2008 */
            , {28,24,5,25,5,25}  /* 2009 */
            , {27,30,4,24,4,24}  /* 2010 */
            , {26,29,3,23,3,23}  /* 2011 */
            , {24,27,1,28,1,28}  /* 2012 */
            , {30,26,7,27,7,27}  /* 2013 */
            , {29,25,6,26,6,26}  /* 2014 */
            , {28,24,5,25,5,25}  /* 2015 */
            , {26,29,3,23,3,23}  /* 2016 */
            , {25,28,2,22,2,22}  /* 2017 */
            , {24,27,1,28,1,28}  /* 2018 */
            , {30,26,7,27,7,27}  /* 2019 */
            , {0}
        };
        for( j=0 , year = 1999; yeardays[j][0] ; j++, year++) {
            for (i=0; cases[i].nr; i++) {
                if(i) cases[i].nr += 18;
                cases[i].slt.wYear = year;
                cases[i].slt.wDay = yeardays[j][i/3];
                pSystemTimeToTzSpecificLocalTime( cases[i].ptz, &(cases[i].slt), &result);
                ok( result.wHour == cases[i].ehour,
                        "Test SystemTimeToTzSpecificLocalTime #%d. Got %4d-%02d-%02d %02d:%02d. Expect hour = %02d\n", 
                        cases[i].nr, result.wYear, result.wMonth, result.wDay,
                        result.wHour, result.wMinute, cases[i].ehour);
            }
        }

    }        
}

static void test_FileTimeToDosDateTime(void)
{
    FILETIME ft = { 0 };
    WORD fatdate, fattime;
    BOOL ret;

    if (0)
    {
        /* Crashes */
        FileTimeToDosDateTime(NULL, NULL, NULL);
    }
    /* Parameter checking */
    SetLastError(0xdeadbeef);
    ret = FileTimeToDosDateTime(&ft, NULL, NULL);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = FileTimeToDosDateTime(&ft, &fatdate, NULL);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = FileTimeToDosDateTime(&ft, NULL, &fattime);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = FileTimeToDosDateTime(&ft, &fatdate, &fattime);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());
}

static void test_GetCalendarInfo(void)
{
    char bufferA[20];
    WCHAR bufferW[20];
    DWORD val1, val2;
    int i, j, ret, ret2;

    if (!pGetCalendarInfoA || !pGetCalendarInfoW)
    {
        trace( "GetCalendarInfo missing\n" );
        return;
    }

    ret = pGetCalendarInfoA( 0x0409, CAL_GREGORIAN, CAL_ITWODIGITYEARMAX | CAL_RETURN_NUMBER,
                             NULL, 0, &val1 );
    ok( ret, "GetCalendarInfoA failed err %lu\n", GetLastError() );
    ok( ret == sizeof(val1), "wrong size %u\n", ret );
    ok( val1 >= 2000 && val1 < 2100, "wrong value %lu\n", val1 );

    ret = pGetCalendarInfoW( 0x0409, CAL_GREGORIAN, CAL_ITWODIGITYEARMAX | CAL_RETURN_NUMBER,
                             NULL, 0, &val2 );
    ok( ret, "GetCalendarInfoW failed err %lu\n", GetLastError() );
    ok( ret == sizeof(val2)/sizeof(WCHAR), "wrong size %u\n", ret );
    ok( val1 == val2, "A/W mismatch %lu/%lu\n", val1, val2 );

    ret = pGetCalendarInfoA( 0x0409, CAL_GREGORIAN, CAL_ITWODIGITYEARMAX, bufferA, sizeof(bufferA), NULL );
    ok( ret, "GetCalendarInfoA failed err %lu\n", GetLastError() );
    ok( ret == 5, "wrong size %u\n", ret );
    ok( atoi( bufferA ) == val1, "wrong value %s/%lu\n", bufferA, val1 );

    ret = pGetCalendarInfoW( 0x0409, CAL_GREGORIAN, CAL_ITWODIGITYEARMAX, bufferW, ARRAY_SIZE(bufferW), NULL );
    ok( ret, "GetCalendarInfoW failed err %lu\n", GetLastError() );
    ok( ret == 5, "wrong size %u\n", ret );
    memset( bufferA, 0x55, sizeof(bufferA) );
    WideCharToMultiByte( CP_ACP, 0, bufferW, -1, bufferA, sizeof(bufferA), NULL, NULL );
    ok( atoi( bufferA ) == val1, "wrong value %s/%lu\n", bufferA, val1 );

    ret = pGetCalendarInfoA( 0x0409, CAL_GREGORIAN, CAL_ITWODIGITYEARMAX | CAL_RETURN_NUMBER,
                             NULL, 0, NULL );
    ok( !ret, "GetCalendarInfoA succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %lu\n", GetLastError() );

    ret = pGetCalendarInfoA( 0x0409, CAL_GREGORIAN, CAL_ITWODIGITYEARMAX, NULL, 0, NULL );
    ok( ret, "GetCalendarInfoA failed err %lu\n", GetLastError() );
    ok( ret == 5, "wrong size %u\n", ret );

    ret = pGetCalendarInfoW( 0x0409, CAL_GREGORIAN, CAL_ITWODIGITYEARMAX | CAL_RETURN_NUMBER,
                             NULL, 0, NULL );
    ok( !ret, "GetCalendarInfoW succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %lu\n", GetLastError() );

    ret = pGetCalendarInfoW( 0x0409, CAL_GREGORIAN, CAL_ITWODIGITYEARMAX, NULL, 0, NULL );
    ok( ret, "GetCalendarInfoW failed err %lu\n", GetLastError() );
    ok( ret == 5, "wrong size %u\n", ret );

    ret = pGetCalendarInfoA( LANG_SYSTEM_DEFAULT, CAL_GREGORIAN, CAL_SDAYNAME1,
                             bufferA, sizeof(bufferA), NULL);
    ok( ret, "GetCalendarInfoA failed err %lu\n", GetLastError() );
    ret2 = pGetCalendarInfoA( LANG_SYSTEM_DEFAULT, CAL_GREGORIAN, CAL_SDAYNAME1,
                              bufferA, 0, NULL);
    ok( ret2, "GetCalendarInfoA failed err %lu\n", GetLastError() );
    ok( ret == ret2, "got %d, expected %d\n", ret2, ret );

    ret2 = pGetCalendarInfoW( LANG_SYSTEM_DEFAULT, CAL_GREGORIAN, CAL_SDAYNAME1,
                              bufferW, ARRAY_SIZE(bufferW), NULL);
    ok( ret2, "GetCalendarInfoW failed err %lu\n", GetLastError() );
    ret2 = WideCharToMultiByte( CP_ACP, 0, bufferW, -1, NULL, 0, NULL, NULL );
    ok( ret == ret2, "got %d, expected %d\n", ret, ret2 );

#ifdef __REACTOS__
    if (LOBYTE(LOWORD(GetVersion())) >= 6) {
#endif
    for (i = CAL_GREGORIAN; i <= CAL_UMALQURA; i++)
    {
        WCHAR name[80];
        ret = pGetCalendarInfoW( 0x0409, i, CAL_SCALNAME, name, ARRAY_SIZE(name), NULL);
        for (j = CAL_ICALINTVALUE; j <= CAL_SRELATIVELONGDATE; j++)
        {
            ret2 = pGetCalendarInfoW( 0x0409, i, j, bufferW, ARRAY_SIZE(bufferW), NULL);
            if (ret || j == CAL_ITWODIGITYEARMAX)
#ifdef __REACTOS__
                ok( ret2 || broken(j == CAL_SRELATIVELONGDATE) /* Win7 */ || broken(j == CAL_SMONTHDAY || j == CAL_SABBREVERASTRING), /* Vista */
#else
                ok( ret2 || broken(j == CAL_SRELATIVELONGDATE),  /* win7 doesn't have this */
#endif
                    "calendar %u %s value %02x failed\n", i, wine_dbgstr_w(name), j );
            else
                ok( !ret2, "calendar %u %s value %02x succeeded %s\n",
                    i, wine_dbgstr_w(name), j, wine_dbgstr_w(bufferW) );
        }
    }
#ifdef __REACTOS__
    }
#endif
}

static void test_GetDynamicTimeZoneInformation(void)
{
    DYNAMIC_TIME_ZONE_INFORMATION dyninfo;
    TIME_ZONE_INFORMATION tzinfo;
    DWORD ret, ret2;

    if (!pGetDynamicTimeZoneInformation)
    {
        win_skip("GetDynamicTimeZoneInformation() is not supported.\n");
        return;
    }

    ret = pGetDynamicTimeZoneInformation(&dyninfo);
    ret2 = GetTimeZoneInformation(&tzinfo);
    ok(ret == ret2, "got %ld, %ld\n", ret, ret2);

    ok(dyninfo.Bias == tzinfo.Bias, "got %ld, %ld\n", dyninfo.Bias, tzinfo.Bias);
    ok(!lstrcmpW(dyninfo.StandardName, tzinfo.StandardName), "got std name %s, %s\n",
        wine_dbgstr_w(dyninfo.StandardName), wine_dbgstr_w(tzinfo.StandardName));
    ok(!memcmp(&dyninfo.StandardDate, &tzinfo.StandardDate, sizeof(dyninfo.StandardDate)), "got different StandardDate\n");
    ok(dyninfo.StandardBias == tzinfo.StandardBias, "got %ld, %ld\n", dyninfo.StandardBias, tzinfo.StandardBias);
    ok(!lstrcmpW(dyninfo.DaylightName, tzinfo.DaylightName), "got daylight name %s, %s\n",
        wine_dbgstr_w(dyninfo.DaylightName), wine_dbgstr_w(tzinfo.DaylightName));
    ok(!memcmp(&dyninfo.DaylightDate, &tzinfo.DaylightDate, sizeof(dyninfo.DaylightDate)), "got different DaylightDate\n");
    ok(dyninfo.TimeZoneKeyName[0] != 0, "got empty tz keyname\n");
    trace("Dyn TimeZoneKeyName %s\n", wine_dbgstr_w(dyninfo.TimeZoneKeyName));
}

static ULONGLONG get_longlong_time(FILETIME *time)
{
    ULARGE_INTEGER uli;
    uli.u.LowPart = time->dwLowDateTime;
    uli.u.HighPart = time->dwHighDateTime;
    return uli.QuadPart;
}

static void test_GetSystemTimeAsFileTime(void)
{
    LARGE_INTEGER t1, t2, t3;
    FILETIME ft;

    NtQuerySystemTime( &t1 );
    GetSystemTimeAsFileTime( &ft );
    NtQuerySystemTime( &t3 );
    t2.QuadPart = get_longlong_time( &ft );

    ok(t1.QuadPart <= t2.QuadPart, "out of order %s %s\n", wine_dbgstr_longlong(t1.QuadPart), wine_dbgstr_longlong(t2.QuadPart));
    ok(t2.QuadPart <= t3.QuadPart, "out of order %s %s\n", wine_dbgstr_longlong(t2.QuadPart), wine_dbgstr_longlong(t3.QuadPart));
}

static void test_GetSystemTimePreciseAsFileTime(void)
{
    FILETIME ft;
    ULONGLONG time1, time2;
    LONGLONG diff;

    if (!pGetSystemTimePreciseAsFileTime)
    {
        win_skip("GetSystemTimePreciseAsFileTime() is not supported.\n");
        return;
    }

    GetSystemTimeAsFileTime(&ft);
    time1 = get_longlong_time(&ft);
    pGetSystemTimePreciseAsFileTime(&ft);
    time2 = get_longlong_time(&ft);
    diff = time2 - time1;
    if (diff < 0)
        diff = -diff;
    ok(diff < 1000000, "Difference between GetSystemTimeAsFileTime and GetSystemTimePreciseAsFileTime more than 100 ms\n");

    pGetSystemTimePreciseAsFileTime(&ft);
    time1 = get_longlong_time(&ft);
    do {
        pGetSystemTimePreciseAsFileTime(&ft);
        time2 = get_longlong_time(&ft);
    } while (time2 == time1);
    diff = time2 - time1;
    ok(diff < 10000 && diff > 0, "GetSystemTimePreciseAsFileTime incremented by more than 1 ms\n");
}

static void test_GetSystemTimes(void)
{

    FILETIME idletime, kerneltime, usertime;
    int i;
    ULARGE_INTEGER ul1, ul2, ul3;
    SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION *sppi;
    SYSTEM_BASIC_INFORMATION sbi;
    ULONG ReturnLength;
    ULARGE_INTEGER total_usertime, total_kerneltime, total_idletime;

    if (!pGetSystemTimes)
    {
        win_skip("GetSystemTimes not available\n");
        return;
    }

    ok( pGetSystemTimes(NULL, NULL, NULL), "GetSystemTimes failed unexpectedly\n" );

    total_usertime.QuadPart = 0;
    total_kerneltime.QuadPart = 0;
    total_idletime.QuadPart = 0;
    memset( &idletime, 0x11, sizeof(idletime) );
    memset( &kerneltime, 0x11, sizeof(kerneltime) );
    memset( &usertime, 0x11, sizeof(usertime) );
    ok( pGetSystemTimes(&idletime, &kerneltime , &usertime),
        "GetSystemTimes failed unexpectedly\n" );

    ul1.LowPart = idletime.dwLowDateTime;
    ul1.HighPart = idletime.dwHighDateTime;
    ul2.LowPart = kerneltime.dwLowDateTime;
    ul2.HighPart = kerneltime.dwHighDateTime;
    ul3.LowPart = usertime.dwLowDateTime;
    ul3.HighPart = usertime.dwHighDateTime;

    ok( !NtQuerySystemInformation(SystemBasicInformation, &sbi, sizeof(sbi), &ReturnLength),
                                  "NtQuerySystemInformation failed\n" );
    ok( sizeof(sbi) == ReturnLength, "Inconsistent length %ld\n", ReturnLength );

    /* Check if we have some return values */
    trace( "Number of Processors : %d\n", sbi.NumberOfProcessors );
    ok( sbi.NumberOfProcessors > 0, "Expected more than 0 processors, got %d\n",
        sbi.NumberOfProcessors );

    sppi = HeapAlloc( GetProcessHeap(), 0,
                      sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * sbi.NumberOfProcessors);

    ok( !NtQuerySystemInformation( SystemProcessorPerformanceInformation, sppi,
                                   sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * sbi.NumberOfProcessors,
                                   &ReturnLength),
                                   "NtQuerySystemInformation failed\n" );

    for (i = 0; i < sbi.NumberOfProcessors; i++)
    {
        total_usertime.QuadPart += sppi[i].UserTime.QuadPart;
        total_kerneltime.QuadPart += sppi[i].KernelTime.QuadPart;
        total_idletime.QuadPart += sppi[i].IdleTime.QuadPart;
    }

    ok( total_idletime.QuadPart - ul1.QuadPart < 10000000, "test idletime failed\n" );
    ok( total_kerneltime.QuadPart - ul2.QuadPart < 10000000, "test kerneltime failed\n" );
    ok( total_usertime.QuadPart - ul3.QuadPart < 10000000, "test usertime failed\n" );

    HeapFree(GetProcessHeap(), 0, sppi);
}

static WORD day_of_month(const SYSTEMTIME* systemtime, WORD year)
{
    SYSTEMTIME first_of_month = {0};
    FILETIME filetime;
    WORD result;

    if (systemtime->wYear != 0)
        return systemtime->wDay;

    if (systemtime->wMonth == 0)
        return 0;

    first_of_month.wYear = year;
    first_of_month.wMonth = systemtime->wMonth;
    first_of_month.wDay = 1;

    /* round-trip conversion sets day of week field */
    SystemTimeToFileTime(&first_of_month, &filetime);
    FileTimeToSystemTime(&filetime, &first_of_month);

    result = 1 + ((systemtime->wDayOfWeek - first_of_month.wDayOfWeek + 7) % 7) +
        (7 * (systemtime->wDay - 1));

    if (systemtime->wDay == 5)
    {
        /* make sure this isn't in the next month */
        SYSTEMTIME result_date;

        result_date = first_of_month;
        result_date.wDay = result;

        SystemTimeToFileTime(&result_date, &filetime);
        FileTimeToSystemTime(&filetime, &result_date);

        if (result_date.wDay != result)
            result = 1 + ((systemtime->wDayOfWeek - first_of_month.wDayOfWeek + 7) % 7) +
                (7 * (4 - 1));
    }

    return result;
}

static void test_GetTimeZoneInformationForYear(void)
{
    BOOL ret, broken_test = FALSE;
    SYSTEMTIME systemtime;
    TIME_ZONE_INFORMATION local_tzinfo, tzinfo, tzinfo2;
    DYNAMIC_TIME_ZONE_INFORMATION dyn_tzinfo;
    WORD std_day, dlt_day;
    unsigned i;
    static const struct
    {
        const WCHAR *tzname;
        USHORT year;
        LONG bias, std_bias, dlt_bias;
        WORD std_month, std_day, dlt_month, dlt_day;
    }
    test_data[] =
    {
        { L"Greenland Standard Time",     2015,  180, 0, -60, 10, 24, 3, 28 },
        { L"Greenland Standard Time",     2016,  180, 0, -60, 10, 29, 3, 26 },
        { L"Greenland Standard Time",     2017,  180, 0, -60, 10, 28, 3, 25 },
        { L"Easter Island Standard Time", 2014,  360, 0, -60,  4, 26, 9,  6 },
        { L"Easter Island Standard Time", 2015,  300, 0, -60,  0,  0, 0,  0 },
        { L"Easter Island Standard Time", 2016,  360, 0, -60,  5, 14, 8, 13 },
        { L"Egypt Standard Time",         2013, -120, 0, -60,  0,  0, 0,  0 },
        { L"Egypt Standard Time",         2014, -120, 0, -60,  9, 25, 5, 15 },
        { L"Egypt Standard Time",         2015, -120, 0, -60,  0,  0, 0,  0 },
        { L"Egypt Standard Time",         2016, -120, 0, -60,  0,  0, 0,  0 },
        { L"Altai Standard Time",         2016, -420, 0,  60,  3, 27, 1,  1 },
        { L"Altai Standard Time",         2017, -420, 0, -60,  0,  0, 0,  0 },
        { L"Altai Standard Time",         2018, -420, 0, -60,  0,  0, 0,  0 },
    };

    if (!pGetTimeZoneInformationForYear || !pGetDynamicTimeZoneInformation)
    {
        win_skip("GetTimeZoneInformationForYear not available\n");
        return;
    }

    GetLocalTime(&systemtime);

    GetTimeZoneInformation(&local_tzinfo);

    ret = pGetTimeZoneInformationForYear(systemtime.wYear, NULL, &tzinfo);
    ok(ret == TRUE, "GetTimeZoneInformationForYear failed, err %lu\n", GetLastError());
    ok(tzinfo.Bias == local_tzinfo.Bias, "Expected Bias %ld, got %ld\n", local_tzinfo.Bias, tzinfo.Bias);
    ok(!lstrcmpW(tzinfo.StandardName, local_tzinfo.StandardName),
        "Expected StandardName %s, got %s\n", wine_dbgstr_w(local_tzinfo.StandardName), wine_dbgstr_w(tzinfo.StandardName));
    ok(!memcmp(&tzinfo.StandardDate, &local_tzinfo.StandardDate, sizeof(SYSTEMTIME)), "StandardDate does not match\n");
    ok(tzinfo.StandardBias == local_tzinfo.StandardBias, "Expected StandardBias %ld, got %ld\n", local_tzinfo.StandardBias, tzinfo.StandardBias);
    ok(!lstrcmpW(tzinfo.DaylightName, local_tzinfo.DaylightName),
        "Expected DaylightName %s, got %s\n", wine_dbgstr_w(local_tzinfo.DaylightName), wine_dbgstr_w(tzinfo.DaylightName));
    ok(!memcmp(&tzinfo.DaylightDate, &local_tzinfo.DaylightDate, sizeof(SYSTEMTIME)), "DaylightDate does not match\n");
    ok(tzinfo.DaylightBias == local_tzinfo.DaylightBias, "Expected DaylightBias %ld, got %ld\n", local_tzinfo.DaylightBias, tzinfo.DaylightBias);

    pGetDynamicTimeZoneInformation(&dyn_tzinfo);

    ret = pGetTimeZoneInformationForYear(systemtime.wYear, &dyn_tzinfo, &tzinfo);
    ok(ret == TRUE, "GetTimeZoneInformationForYear failed, err %lu\n", GetLastError());
    ok(tzinfo.Bias == local_tzinfo.Bias, "Expected Bias %ld, got %ld\n", local_tzinfo.Bias, tzinfo.Bias);
    ok(!lstrcmpW(tzinfo.StandardName, local_tzinfo.StandardName),
        "Expected StandardName %s, got %s\n", wine_dbgstr_w(local_tzinfo.StandardName), wine_dbgstr_w(tzinfo.StandardName));
    ok(!memcmp(&tzinfo.StandardDate, &local_tzinfo.StandardDate, sizeof(SYSTEMTIME)), "StandardDate does not match\n");
    ok(tzinfo.StandardBias == local_tzinfo.StandardBias, "Expected StandardBias %ld, got %ld\n", local_tzinfo.StandardBias, tzinfo.StandardBias);
    ok(!lstrcmpW(tzinfo.DaylightName, local_tzinfo.DaylightName),
        "Expected DaylightName %s, got %s\n", wine_dbgstr_w(local_tzinfo.DaylightName), wine_dbgstr_w(tzinfo.DaylightName));
    ok(!memcmp(&tzinfo.DaylightDate, &local_tzinfo.DaylightDate, sizeof(SYSTEMTIME)), "DaylightDate does not match\n");
    ok(tzinfo.DaylightBias == local_tzinfo.DaylightBias, "Expected DaylightBias %ld, got %ld\n", local_tzinfo.DaylightBias, tzinfo.DaylightBias);

    memset(&dyn_tzinfo, 0xaa, sizeof(dyn_tzinfo));
    lstrcpyW(dyn_tzinfo.TimeZoneKeyName, L"Greenland Daylight Time");
    SetLastError(0xdeadbeef);
    ret = pGetTimeZoneInformationForYear(2015, &dyn_tzinfo, &tzinfo);
    if (ret)
        broken_test = TRUE;
    ok((ret == FALSE && GetLastError() == ERROR_FILE_NOT_FOUND) ||
        broken(broken_test) /* vista,7 */,
        "GetTimeZoneInformationForYear err %lu\n", GetLastError());

    memset(&dyn_tzinfo, 0xaa, sizeof(dyn_tzinfo));
    lstrcpyW(dyn_tzinfo.TimeZoneKeyName, L"Altai Standard Time");
    SetLastError(0xdeadbeef);
    ret = pGetTimeZoneInformationForYear(2015, &dyn_tzinfo, &tzinfo);
    if (!ret && GetLastError() == ERROR_FILE_NOT_FOUND)
        broken_test = TRUE;
    ok(ret == TRUE || broken(broken_test) /* before 10 1809 */,
        "GetTimeZoneInformationForYear err %lu\n", GetLastError());

    if (broken(broken_test))
    {
        win_skip("Old Windows version\n");
        return;
    }

    for (i = 0; i < ARRAY_SIZE(test_data); i++)
    {
        memset(&dyn_tzinfo, 0xaa, sizeof(dyn_tzinfo));
        memset(&tzinfo, 0xbb, sizeof(tzinfo));
        lstrcpyW(dyn_tzinfo.TimeZoneKeyName, test_data[i].tzname);
        dyn_tzinfo.DynamicDaylightTimeDisabled = FALSE;

        ret = pGetTimeZoneInformationForYear(test_data[i].year, &dyn_tzinfo, &tzinfo);
        ok(ret == TRUE, "GetTimeZoneInformationForYear failed, err %lu, for %s\n", GetLastError(), wine_dbgstr_w(test_data[i].tzname));
        if (!ret)
            continue;
        ok(tzinfo.Bias == test_data[i].bias, "Expected bias %ld, got %ld, for %s\n",
           test_data[i].bias, tzinfo.Bias, wine_dbgstr_w(test_data[i].tzname));
        ok(tzinfo.StandardDate.wMonth == test_data[i].std_month, "Expected standard month %d, got %d, for %s\n",
           test_data[i].std_month, tzinfo.StandardDate.wMonth, wine_dbgstr_w(test_data[i].tzname));
        std_day = day_of_month(&tzinfo.StandardDate, test_data[i].year);
        ok(std_day == test_data[i].std_day, "Expected standard day %d, got %d, for %s\n",
           test_data[i].std_day, std_day, wine_dbgstr_w(test_data[i].tzname));
        ok(tzinfo.StandardBias == test_data[i].std_bias, "Expected standard bias %ld, got %ld, for %s\n",
           test_data[i].std_bias, tzinfo.StandardBias, wine_dbgstr_w(test_data[i].tzname));
        ok(tzinfo.DaylightDate.wMonth == test_data[i].dlt_month, "Expected daylight month %d, got %d, for %s\n",
           test_data[i].dlt_month, tzinfo.DaylightDate.wMonth, wine_dbgstr_w(test_data[i].tzname));
        dlt_day = day_of_month(&tzinfo.DaylightDate, test_data[i].year);
        ok(dlt_day == test_data[i].dlt_day, "Expected daylight day %d, got %d, for %s\n",
           test_data[i].dlt_day, dlt_day, wine_dbgstr_w(test_data[i].tzname));
        ok(tzinfo.DaylightBias == test_data[i].dlt_bias, "Expected daylight bias %ld, got %ld, for %s\n",
           test_data[i].dlt_bias, tzinfo.DaylightBias, wine_dbgstr_w(test_data[i].tzname));

        if (i > 0 && test_data[i-1].tzname == test_data[i].tzname)
        {
            ok(!lstrcmpW(tzinfo.StandardName, tzinfo2.StandardName),
                "Got differing StandardName values %s and %s\n",
                wine_dbgstr_w(tzinfo.StandardName), wine_dbgstr_w(tzinfo2.StandardName));
            ok(!lstrcmpW(tzinfo.DaylightName, tzinfo2.DaylightName),
                "Got differing DaylightName values %s and %s\n",
                wine_dbgstr_w(tzinfo.DaylightName), wine_dbgstr_w(tzinfo2.DaylightName));
        }

        memcpy(&tzinfo2, &tzinfo, sizeof(tzinfo2));
    }
}

static void test_GetTickCount(void)
{
    DWORD t1, t2, t3;
    int i = 0;

    if (!pNtGetTickCount)
    {
        win_skip("NtGetTickCount not implemented\n");
        return;
    }

    do
    {
        t1 = pNtGetTickCount();
        t2 = GetTickCount();
        t3 = pNtGetTickCount();
    } while(t3 < t1 && i++ < 1); /* allow for wrap, but only once */

    ok(t1 <= t2, "out of order %ld %ld\n", t1, t2);
    ok(t2 <= t3, "out of order %ld %ld\n", t2, t3);
}

BOOL (WINAPI *pQueryUnbiasedInterruptTime)(ULONGLONG *time);
BOOL (WINAPI *pRtlQueryUnbiasedInterruptTime)(ULONGLONG *time);

static void test_QueryUnbiasedInterruptTime(void)
{
    ULONGLONG time;
    BOOL ret;

    if (pQueryUnbiasedInterruptTime)
    {
        SetLastError( 0xdeadbeef );
        ret = pQueryUnbiasedInterruptTime( &time );
        ok( ret, "QueryUnbiasedInterruptTime failed err %lu\n", GetLastError() );
        SetLastError( 0xdeadbeef );
        ret = pQueryUnbiasedInterruptTime( NULL );
        ok( !ret, "QueryUnbiasedInterruptTime succeeded\n" );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %lu\n", GetLastError() );
    }
    else win_skip( "QueryUnbiasedInterruptTime not supported\n" );
    if (pRtlQueryUnbiasedInterruptTime)
    {
        SetLastError( 0xdeadbeef );
        ret = pRtlQueryUnbiasedInterruptTime( &time );
        ok( ret, "RtlQueryUnbiasedInterruptTime failed err %lu\n", GetLastError() );
        SetLastError( 0xdeadbeef );
        ret = pRtlQueryUnbiasedInterruptTime( NULL );
        ok( !ret, "RtlQueryUnbiasedInterruptTime succeeded\n" );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %lu\n", GetLastError() );
    }
    else win_skip( "RtlQueryUnbiasedInterruptTime not supported\n" );
}

static void test_processor_idle_cycle_time(void)
{
#ifdef __REACTOS__
    skip("Cannot build test_processor_idle_cycle_time() until kernelbase is synced.\n");
#else
    unsigned int cpu_count = NtCurrentTeb()->Peb->NumberOfProcessors;
    ULONG64 buffer[64];
    ULONG size;
    DWORD err;
    BOOL bret;

    SetLastError( 0xdeadbeef );
    size = 0;
    bret = QueryIdleProcessorCycleTime( &size, NULL );
    err = GetLastError();
    ok( bret == TRUE && err == 0xdeadbeef, "got %d, %ld.\n", bret, err );
    ok( size == cpu_count * sizeof(ULONG64), "got %lu.\n", size );

    size = 4;
    memset( buffer, 0xcc, sizeof(buffer) );
    SetLastError( 0xdeadbeef );
    bret = QueryIdleProcessorCycleTime( &size, NULL );
    err = GetLastError();
    ok( bret == TRUE && err == 0xdeadbeef, "got %d, %ld.\n", bret, err );
    ok( size == 4, "got %lu.\n", size );
    ok( buffer[0] == 0xcccccccccccccccc, "got %#I64x.\n", buffer[0] );

    size = sizeof(buffer);
    SetLastError( 0xdeadbeef );
    bret = QueryIdleProcessorCycleTime( &size, NULL );
    err = GetLastError();
    ok( bret == TRUE && err == 0xdeadbeef, "got %d, %ld.\n", bret, err );
    ok( size == sizeof(buffer), "got %lu.\n", size );

    size = sizeof(buffer);
    SetLastError( 0xdeadbeef );
    bret = QueryIdleProcessorCycleTime( &size, buffer );
    err = GetLastError();
    ok( bret == TRUE && err == 0xdeadbeef, "got %d, %ld.\n", bret, err );
    ok( size == cpu_count * sizeof(ULONG64), "got %lu.\n", size );

    SetLastError( 0xdeadbeef );
    size = 0;
    bret = QueryIdleProcessorCycleTimeEx( 0, &size, NULL );
    err = GetLastError();
    ok( bret == TRUE && err == 0xdeadbeef, "got %d, %ld.\n", bret, err );
    ok( size == cpu_count * sizeof(ULONG64), "got %lu.\n", size );

    size = 4;
    SetLastError( 0xdeadbeef );
    bret = QueryIdleProcessorCycleTimeEx( 0, &size, NULL );
    err = GetLastError();
    ok( bret == TRUE && err == 0xdeadbeef, "got %d, %ld.\n", bret, err );
    ok( size == 4, "got %lu.\n", size );

    size = sizeof(buffer);
    SetLastError( 0xdeadbeef );
    bret = QueryIdleProcessorCycleTimeEx( 0, &size, NULL );
    err = GetLastError();
    ok( bret == TRUE && err == 0xdeadbeef, "got %d, %ld.\n", bret, err );
    ok( size == sizeof(buffer), "got %lu.\n", size );

    size = sizeof(buffer);
    SetLastError( 0xdeadbeef );
    bret = QueryIdleProcessorCycleTimeEx( 0, &size, buffer );
    err = GetLastError();
    ok( bret == TRUE && err == 0xdeadbeef, "got %d, %ld.\n", bret, err );
    ok( size == cpu_count * sizeof(ULONG64), "got %lu.\n", size );
#endif
}

START_TEST(time)
{
    HMODULE hKernel = GetModuleHandleA("kernel32");
    HMODULE hntdll = GetModuleHandleA("ntdll");
    pTzSpecificLocalTimeToSystemTime = (void *)GetProcAddress(hKernel, "TzSpecificLocalTimeToSystemTime");
    pSystemTimeToTzSpecificLocalTime = (void *)GetProcAddress( hKernel, "SystemTimeToTzSpecificLocalTime");
    pGetSystemTimes = (void *)GetProcAddress( hKernel, "GetSystemTimes");
    pGetCalendarInfoA = (void *)GetProcAddress(hKernel, "GetCalendarInfoA");
    pGetCalendarInfoW = (void *)GetProcAddress(hKernel, "GetCalendarInfoW");
    pGetDynamicTimeZoneInformation = (void *)GetProcAddress(hKernel, "GetDynamicTimeZoneInformation");
    pGetSystemTimePreciseAsFileTime = (void *)GetProcAddress(hKernel, "GetSystemTimePreciseAsFileTime");
    pGetTimeZoneInformationForYear = (void *)GetProcAddress(hKernel, "GetTimeZoneInformationForYear");
    pQueryUnbiasedInterruptTime = (void *)GetProcAddress(hKernel, "QueryUnbiasedInterruptTime");
    pNtGetTickCount = (void *)GetProcAddress(hntdll, "NtGetTickCount");
    pRtlQueryUnbiasedInterruptTime = (void *)GetProcAddress(hntdll, "RtlQueryUnbiasedInterruptTime");

    test_conversions();
    test_invalid_arg();
    test_GetTimeZoneInformation();
    test_FileTimeToSystemTime();
    test_FileTimeToLocalFileTime();
    test_TzSpecificLocalTimeToSystemTime();
    test_GetSystemTimes();
    test_FileTimeToDosDateTime();
    test_GetCalendarInfo();
    test_GetDynamicTimeZoneInformation();
    test_GetSystemTimeAsFileTime();
    test_GetSystemTimePreciseAsFileTime();
    test_GetTimeZoneInformationForYear();
    test_GetTickCount();
    test_QueryUnbiasedInterruptTime();
    test_processor_idle_cycle_time();
}
