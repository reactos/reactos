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


void test_GetTimeZoneInformation()
{
    TIME_ZONE_INFORMATION tzinfo, tzinfo1;
    DWORD res =  GetTimeZoneInformation(&tzinfo);
    ok(res != 0, "GetTimeZoneInformation failed\n");
    ok(SetEnvironmentVariableA("TZ","GMT0") != 0,
       "SetEnvironmentVariableA failed\n");
    res =  GetTimeZoneInformation(&tzinfo1);
    ok(res != 0, "GetTimeZoneInformation failed\n");

    ok(((tzinfo.Bias == tzinfo1.Bias) && 
	(tzinfo.StandardBias == tzinfo1.StandardBias) &&
	(tzinfo.DaylightBias == tzinfo1.DaylightBias)),
       "Bias influenced by TZ variable\n"); 
    ok(SetEnvironmentVariableA("TZ",NULL) != 0,
       "SetEnvironmentVariableA failed\n");
        
}

void test_FileTimeToSystemTime()
{
    FILETIME ft;
    SYSTEMTIME st;
    ULONGLONG time = (ULONGLONG)TICKSPERSEC + TICKS_1601_TO_1970;

    ft.dwHighDateTime = 0;
    ft.dwLowDateTime  = 0;
    ok(FileTimeToSystemTime(&ft, &st),
       "FileTimeToSystemTime() failed with Error 0x%08lx\n",GetLastError());
    ok(((st.wYear == 1601) && (st.wMonth  == 1) && (st.wDay    == 1) &&
	(st.wHour ==    0) && (st.wMinute == 0) && (st.wSecond == 0) &&
	(st.wMilliseconds == 0)),
	"Got Year %4d Month %2d Day %2d\n",  st.wYear, st.wMonth, st.wDay);

    ft.dwHighDateTime = (UINT)(time >> 32);
    ft.dwLowDateTime  = (UINT)time;
    ok(FileTimeToSystemTime(&ft, &st),
       "FileTimeToSystemTime() failed with Error 0x%08lx\n",GetLastError());
    ok(((st.wYear == 1970) && (st.wMonth == 1) && (st.wDay == 1) &&
	(st.wHour ==    0) && (st.wMinute == 0) && (st.wSecond == 1) &&
	(st.wMilliseconds == 0)),
       "Got Year %4d Month %2d Day %2d Hour %2d Min %2d Sec %2d mSec %3d\n",
       st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond,
       st.wMilliseconds);
}

void test_FileTimeToLocalFileTime()
{
    FILETIME ft, lft;
    SYSTEMTIME st;
    TIME_ZONE_INFORMATION tzinfo;
    DWORD res =  GetTimeZoneInformation(&tzinfo);
    ULONGLONG time = (ULONGLONG)TICKSPERSEC + TICKS_1601_TO_1970 
	+ (ULONGLONG)tzinfo.Bias*SECSPERMIN *TICKSPERSEC;

    ok( res != 0, "GetTimeZoneInformation failed\n");
    ft.dwHighDateTime = (UINT)(time >> 32);
    ft.dwLowDateTime  = (UINT)time;
    ok(FileTimeToLocalFileTime(&ft, &lft) !=0 ,
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
    ok(res != 0, "GetTimeZoneInformation failed\n");
    ok(FileTimeToLocalFileTime(&ft, &lft) !=0 ,
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

START_TEST(time)
{
    test_GetTimeZoneInformation();
    test_FileTimeToSystemTime();
    test_FileTimeToLocalFileTime();
    
}
