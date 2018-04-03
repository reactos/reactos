/*
 * Unit test suite for ntdll time functions
 *
 * Copyright 2004 Rein Klazes
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

#ifndef __REACTOS__
#define NONAMELESSUNION
#endif
#include "ntdll_test.h"
#ifndef __REACTOS__
#include "ddk/wdm.h"
#else /* FIXME: Inspect */

typedef struct _KSYSTEM_TIME {
    ULONG LowPart;
    LONG High1Time;
    LONG High2Time;
} KSYSTEM_TIME, *PKSYSTEM_TIME;

typedef enum _NT_PRODUCT_TYPE {
    NtProductWinNt = 1,
    NtProductLanManNt,
    NtProductServer
} NT_PRODUCT_TYPE, *PNT_PRODUCT_TYPE;

#define PROCESSOR_FEATURE_MAX 64

typedef enum _ALTERNATIVE_ARCHITECTURE_TYPE
{
   StandardDesign,
   NEC98x86,
   EndAlternatives
} ALTERNATIVE_ARCHITECTURE_TYPE;

#define MAX_WOW64_SHARED_ENTRIES 16

typedef struct _KUSER_SHARED_DATA {
    ULONG TickCountLowDeprecated;
    ULONG TickCountMultiplier;
    volatile KSYSTEM_TIME InterruptTime;
    volatile KSYSTEM_TIME SystemTime;
    volatile KSYSTEM_TIME TimeZoneBias;
    USHORT ImageNumberLow;
    USHORT ImageNumberHigh;
    WCHAR NtSystemRoot[260];
    ULONG MaxStackTraceDepth;
    ULONG CryptoExponent;
    ULONG TimeZoneId;
    ULONG LargePageMinimum;
    ULONG Reserved2[7];
    NT_PRODUCT_TYPE NtProductType;
    BOOLEAN ProductTypeIsValid;
    ULONG NtMajorVersion;
    ULONG NtMinorVersion;
    BOOLEAN ProcessorFeatures[PROCESSOR_FEATURE_MAX];
    ULONG Reserved1;
    ULONG Reserved3;
    volatile ULONG TimeSlip;
    ALTERNATIVE_ARCHITECTURE_TYPE AlternativeArchitecture;
    LARGE_INTEGER SystemExpirationDate;
    ULONG SuiteMask;
    BOOLEAN KdDebuggerEnabled;
    UCHAR NXSupportPolicy;
    volatile ULONG ActiveConsoleId;
    volatile ULONG DismountCount;
    ULONG ComPlusPackage;
    ULONG LastSystemRITEventTickCount;
    ULONG NumberOfPhysicalPages;
    BOOLEAN SafeBootMode;
    ULONG TraceLogging;
    ULONGLONG TestRetInstruction;
    ULONG SystemCall;
    ULONG SystemCallReturn;
    ULONGLONG SystemCallPad[3];
    union {
        volatile KSYSTEM_TIME TickCount;
        volatile ULONG64 TickCountQuad;
    } DUMMYUNIONNAME;
    ULONG Cookie;
    ULONG Wow64SharedInformation[MAX_WOW64_SHARED_ENTRIES];
} KSHARED_USER_DATA, *PKSHARED_USER_DATA;

#endif /* !__REACTOS__ */

#define TICKSPERSEC        10000000
#define TICKSPERMSEC       10000
#define SECSPERDAY         86400

static VOID (WINAPI *pRtlTimeToTimeFields)( const LARGE_INTEGER *liTime, PTIME_FIELDS TimeFields) ;
static VOID (WINAPI *pRtlTimeFieldsToTime)(  PTIME_FIELDS TimeFields,  PLARGE_INTEGER Time) ;
static NTSTATUS (WINAPI *pNtQueryPerformanceCounter)( LARGE_INTEGER *counter, LARGE_INTEGER *frequency );
static NTSTATUS (WINAPI *pRtlQueryTimeZoneInformation)( RTL_TIME_ZONE_INFORMATION *);
static NTSTATUS (WINAPI *pRtlQueryDynamicTimeZoneInformation)( RTL_DYNAMIC_TIME_ZONE_INFORMATION *);
static ULONG (WINAPI *pNtGetTickCount)(void);

static const int MonthLengths[2][12] =
{
	{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
	{ 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
};

static inline BOOL IsLeapYear(int Year)
{
    return Year % 4 == 0 && (Year % 100 != 0 || Year % 400 == 0);
}

/* start time of the tests */
static TIME_FIELDS tftest = {1889,12,31,23,59,59,0,0};

static void test_pRtlTimeToTimeFields(void)
{
    LARGE_INTEGER litime , liresult;
    TIME_FIELDS tfresult;
    int i=0;
    litime.QuadPart = ((ULONGLONG)0x0144017a << 32) | 0xf0b0a980;
    while( tftest.Year < 2110 ) {
        /* test at the last second of the month */
        pRtlTimeToTimeFields( &litime, &tfresult);
        ok( tfresult.Year == tftest.Year && tfresult.Month == tftest.Month &&
            tfresult.Day == tftest.Day && tfresult.Hour == tftest.Hour && 
            tfresult.Minute == tftest.Minute && tfresult.Second == tftest.Second,
            "#%d expected: %d-%d-%d %d:%d:%d  got:  %d-%d-%d %d:%d:%d\n", ++i,
            tftest.Year, tftest.Month, tftest.Day,
            tftest.Hour, tftest.Minute,tftest.Second,
            tfresult.Year, tfresult.Month, tfresult.Day,
            tfresult.Hour, tfresult.Minute, tfresult.Second);
        /* test the inverse */
        pRtlTimeFieldsToTime( &tfresult, &liresult);
        ok( liresult.QuadPart == litime.QuadPart," TimeFieldsToTime failed on %d-%d-%d %d:%d:%d. Error is %d ticks\n",
            tfresult.Year, tfresult.Month, tfresult.Day,
            tfresult.Hour, tfresult.Minute, tfresult.Second,
            (int) (liresult.QuadPart - litime.QuadPart) );
        /*  one second later is beginning of next month */
        litime.QuadPart +=  TICKSPERSEC ;
        pRtlTimeToTimeFields( &litime, &tfresult);
        ok( tfresult.Year == tftest.Year + (tftest.Month ==12) &&
            tfresult.Month == tftest.Month % 12 + 1 &&
            tfresult.Day == 1 && tfresult.Hour == 0 && 
            tfresult.Minute == 0 && tfresult.Second == 0,
            "#%d expected: %d-%d-%d %d:%d:%d  got:  %d-%d-%d %d:%d:%d\n", ++i,
            tftest.Year + (tftest.Month ==12),
            tftest.Month % 12 + 1, 1, 0, 0, 0,
            tfresult.Year, tfresult.Month, tfresult.Day,
            tfresult.Hour, tfresult.Minute, tfresult.Second);
        /* test the inverse */
        pRtlTimeFieldsToTime( &tfresult, &liresult);
        ok( liresult.QuadPart == litime.QuadPart," TimeFieldsToTime failed on %d-%d-%d %d:%d:%d. Error is %d ticks\n",
            tfresult.Year, tfresult.Month, tfresult.Day,
            tfresult.Hour, tfresult.Minute, tfresult.Second,
            (int) (liresult.QuadPart - litime.QuadPart) );
        /* advance to the end of the month */
        litime.QuadPart -=  TICKSPERSEC ;
        if( tftest.Month == 12) {
            tftest.Month = 1;
            tftest.Year += 1;
        } else 
            tftest.Month += 1;
        tftest.Day = MonthLengths[IsLeapYear(tftest.Year)][tftest.Month - 1];
        litime.QuadPart +=  (LONGLONG) tftest.Day * TICKSPERSEC * SECSPERDAY;
    }
}

static void test_NtQueryPerformanceCounter(void)
{
    LARGE_INTEGER counter, frequency;
    NTSTATUS status;

    status = pNtQueryPerformanceCounter(NULL, NULL);
    ok(status == STATUS_ACCESS_VIOLATION, "expected STATUS_ACCESS_VIOLATION, got %08x\n", status);
    status = pNtQueryPerformanceCounter(NULL, &frequency);
    ok(status == STATUS_ACCESS_VIOLATION, "expected STATUS_ACCESS_VIOLATION, got %08x\n", status);
    status = pNtQueryPerformanceCounter(&counter, (void *)0xdeadbee0);
    ok(status == STATUS_ACCESS_VIOLATION, "expected STATUS_ACCESS_VIOLATION, got %08x\n", status);
    status = pNtQueryPerformanceCounter((void *)0xdeadbee0, &frequency);
    ok(status == STATUS_ACCESS_VIOLATION, "expected STATUS_ACCESS_VIOLATION, got %08x\n", status);

    status = pNtQueryPerformanceCounter(&counter, NULL);
    ok(status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %08x\n", status);
    status = pNtQueryPerformanceCounter(&counter, &frequency);
    ok(status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %08x\n", status);
}

static void test_RtlQueryTimeZoneInformation(void)
{
    RTL_DYNAMIC_TIME_ZONE_INFORMATION tzinfo;
    NTSTATUS status;

    /* test RtlQueryTimeZoneInformation returns an indirect string,
       e.g. @tzres.dll,-32 (Vista or later) */
    if (!pRtlQueryTimeZoneInformation || !pRtlQueryDynamicTimeZoneInformation)
    {
        win_skip("Time zone name tests requires Vista or later\n");
        return;
    }

    memset(&tzinfo, 0, sizeof(tzinfo));
    status = pRtlQueryDynamicTimeZoneInformation(&tzinfo);
    ok(status == STATUS_SUCCESS,
       "RtlQueryDynamicTimeZoneInformation failed, got %08x\n", status);
    todo_wine ok(tzinfo.StandardName[0] == '@',
       "standard time zone name isn't an indirect string, got %s\n",
       wine_dbgstr_w(tzinfo.StandardName));
    todo_wine ok(tzinfo.DaylightName[0] == '@',
       "daylight time zone name isn't an indirect string, got %s\n",
       wine_dbgstr_w(tzinfo.DaylightName));

    memset(&tzinfo, 0, sizeof(tzinfo));
    status = pRtlQueryTimeZoneInformation((RTL_TIME_ZONE_INFORMATION *)&tzinfo);
    ok(status == STATUS_SUCCESS,
       "RtlQueryTimeZoneInformation failed, got %08x\n", status);
    todo_wine ok(tzinfo.StandardName[0] == '@',
       "standard time zone name isn't an indirect string, got %s\n",
       wine_dbgstr_w(tzinfo.StandardName));
    todo_wine ok(tzinfo.DaylightName[0] == '@',
       "daylight time zone name isn't an indirect string, got %s\n",
       wine_dbgstr_w(tzinfo.DaylightName));
}

static void test_NtGetTickCount(void)
{
#ifndef _WIN64
    KSHARED_USER_DATA *user_shared_data = (void *)0x7ffe0000;
    LONG diff;
    int i;

    if (!pNtGetTickCount)
    {
        win_skip("NtGetTickCount is not available\n");
        return;
    }

    for (i = 0; i < 5; ++i)
    {
        diff = (user_shared_data->TickCountQuad * user_shared_data->TickCountMultiplier) >> 24;
        diff = pNtGetTickCount() - diff;
        ok(diff < 32, "NtGetTickCount - TickCountQuad too high, expected < 32 got %d\n", diff);
        Sleep(50);
    }
#endif
}

START_TEST(time)
{
    HMODULE mod = GetModuleHandleA("ntdll.dll");
    pRtlTimeToTimeFields = (void *)GetProcAddress(mod,"RtlTimeToTimeFields");
    pRtlTimeFieldsToTime = (void *)GetProcAddress(mod,"RtlTimeFieldsToTime");
    pNtQueryPerformanceCounter = (void *)GetProcAddress(mod, "NtQueryPerformanceCounter");
    pNtGetTickCount = (void *)GetProcAddress(mod,"NtGetTickCount");
    pRtlQueryTimeZoneInformation =
        (void *)GetProcAddress(mod, "RtlQueryTimeZoneInformation");
    pRtlQueryDynamicTimeZoneInformation =
        (void *)GetProcAddress(mod, "RtlQueryDynamicTimeZoneInformation");

    if (pRtlTimeToTimeFields && pRtlTimeFieldsToTime)
        test_pRtlTimeToTimeFields();
    else
        win_skip("Required time conversion functions are not available\n");
    test_NtQueryPerformanceCounter();
    test_NtGetTickCount();
    test_RtlQueryTimeZoneInformation();
}
