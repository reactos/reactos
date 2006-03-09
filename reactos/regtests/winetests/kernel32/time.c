/*
 * Unit test suite for time functions
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "wine/test.h"
#include "winbase.h"

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



static void test_conversions(void)
{
    FILETIME ft;
    SYSTEMTIME st;

    memset(&ft,0,sizeof ft);

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
 
static void test_GetTimeZoneInformation(void)
{
    TIME_ZONE_INFORMATION tzinfo, tzinfo1;
    DWORD res =  GetTimeZoneInformation(&tzinfo);
    ok(res != TIME_ZONE_ID_INVALID, "GetTimeZoneInformation failed\n");
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
       "FileTimeToSystemTime() failed with Error 0x%08lx\n",GetLastError());
    ok(((st.wYear == 1601) && (st.wMonth  == 1) && (st.wDay    == 1) &&
	(st.wHour ==    0) && (st.wMinute == 0) && (st.wSecond == 0) &&
	(st.wMilliseconds == 0)),
	"Got Year %4d Month %2d Day %2d\n",  st.wYear, st.wMonth, st.wDay);

    ft.dwHighDateTime = (UINT)(time >> 32);
    ft.dwLowDateTime  = (UINT)time;
    ret = FileTimeToSystemTime(&ft, &st);
    ok( ret,
       "FileTimeToSystemTime() failed with Error 0x%08lx\n",GetLastError());
    ok(((st.wYear == 1970) && (st.wMonth == 1) && (st.wDay == 1) &&
	(st.wHour ==    0) && (st.wMinute == 0) && (st.wSecond == 1) &&
	(st.wMilliseconds == 0)),
       "Got Year %4d Month %2d Day %2d Hour %2d Min %2d Sec %2d mSec %3d\n",
       st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond,
       st.wMilliseconds);
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
       "FileTimeToLocalFileTime() failed with Error 0x%08lx\n",GetLastError());
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
       "FileTimeToLocalFileTime() failed with Error 0x%08lx\n",GetLastError());
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


/* test TzSpecificLocalTimeToSystemTime and SystemTimeToTzSpecificLocalTime
 * these are in winXP and later */
typedef HANDLE (WINAPI *fnTzSpecificLocalTimeToSystemTime)( LPTIME_ZONE_INFORMATION, LPSYSTEMTIME, LPSYSTEMTIME);
typedef HANDLE (WINAPI *fnSystemTimeToTzSpecificLocalTime)( LPTIME_ZONE_INFORMATION, LPSYSTEMTIME, LPSYSTEMTIME);

typedef struct {
    int nr;             /* test case number for easier lookup */
    TIME_ZONE_INFORMATION *ptz; /* ptr to timezone */
    SYSTEMTIME slt;     /* system/local time to convert */
    WORD ehour;        /* expected hour */
} TZLT2ST_case;

static void test_TzSpecificLocalTimeToSystemTime(void)
{    
    HMODULE hKernel = GetModuleHandle("kernel32");
    fnTzSpecificLocalTimeToSystemTime pTzSpecificLocalTimeToSystemTime;
    fnSystemTimeToTzSpecificLocalTime pSystemTimeToTzSpecificLocalTime = NULL;
    TIME_ZONE_INFORMATION tzE, tzW, tzS;
    SYSTEMTIME result;
    int i, j, year;
    pTzSpecificLocalTimeToSystemTime = (fnTzSpecificLocalTimeToSystemTime) GetProcAddress( hKernel, "TzSpecificLocalTimeToSystemTime");
    if(pTzSpecificLocalTimeToSystemTime)
        pSystemTimeToTzSpecificLocalTime = (fnTzSpecificLocalTimeToSystemTime) GetProcAddress( hKernel, "SystemTimeToTzSpecificLocalTime");
    if( !pSystemTimeToTzSpecificLocalTime)
        return;
    ZeroMemory( &tzE, sizeof(tzE));
    ZeroMemory( &tzW, sizeof(tzW));
    ZeroMemory( &tzS, sizeof(tzS));
    /* timezone Eastern hemisphere */
    tzE.Bias=-600;
    tzE.StandardBias=0;
    tzE.DaylightBias=-60;
    tzE.StandardDate.wMonth=10;
    tzE.StandardDate.wDayOfWeek=0; /*sunday */
    tzE.StandardDate.wDay=5;       /* last (sunday) of the month */
    tzE.StandardDate.wHour=3;
    tzE.DaylightDate.wMonth=3;
    tzE.DaylightDate.wDay=5;
    tzE.DaylightDate.wHour=2;
    /* timezone Western hemisphere */
    tzW.Bias=240;
    tzW.StandardBias=0;
    tzW.DaylightBias=-60;
    tzW.StandardDate.wMonth=10;
    tzW.StandardDate.wDayOfWeek=0; /*sunday */
    tzW.StandardDate.wDay=4;       /* 4th (sunday) of the month */
    tzW.StandardDate.wHour=2;
    tzW.DaylightDate.wMonth=4;
    tzW.DaylightDate.wDay=1;
    tzW.DaylightDate.wHour=2;
    /* timezone Eastern hemisphere */
    tzS.Bias=240;
    tzS.StandardBias=0;
    tzS.DaylightBias=-60;
    tzS.StandardDate.wMonth=4;
    tzS.StandardDate.wDayOfWeek=0; /*sunday */
    tzS.StandardDate.wDay=1;       /* 1st  (sunday) of the month */
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
            /* and now south */
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
            /* and now south */
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

START_TEST(time)
{
    test_conversions();
    test_invalid_arg();
    test_GetTimeZoneInformation();
    test_FileTimeToSystemTime();
    test_FileTimeToLocalFileTime();
    test_TzSpecificLocalTimeToSystemTime();
}
