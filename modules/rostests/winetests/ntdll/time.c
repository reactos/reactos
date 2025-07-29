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

#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "ddk/wdm.h"
#include "intrin.h"
#include "wine/test.h"

#define TICKSPERSEC        10000000
#define TICKSPERMSEC       10000
#define SECSPERDAY         86400

static VOID (WINAPI *pRtlTimeToTimeFields)( const LARGE_INTEGER *liTime, PTIME_FIELDS TimeFields) ;
static VOID (WINAPI *pRtlTimeFieldsToTime)(  PTIME_FIELDS TimeFields,  PLARGE_INTEGER Time) ;
static NTSTATUS (WINAPI *pNtQueryPerformanceCounter)( LARGE_INTEGER *counter, LARGE_INTEGER *frequency );
static NTSTATUS (WINAPI *pNtQuerySystemInformation)( SYSTEM_INFORMATION_CLASS class,
                                                     void *info, ULONG size, ULONG *ret_size );
static NTSTATUS (WINAPI *pRtlQueryTimeZoneInformation)( RTL_TIME_ZONE_INFORMATION *);
static NTSTATUS (WINAPI *pRtlQueryDynamicTimeZoneInformation)( RTL_DYNAMIC_TIME_ZONE_INFORMATION *);
static BOOL     (WINAPI *pRtlQueryUnbiasedInterruptTime)( ULONGLONG *time );

static BOOL     (WINAPI *pRtlQueryPerformanceCounter)(LARGE_INTEGER*);
static BOOL     (WINAPI *pRtlQueryPerformanceFrequency)(LARGE_INTEGER*);

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
    ok(status == STATUS_ACCESS_VIOLATION, "expected STATUS_ACCESS_VIOLATION, got %08lx\n", status);
    status = pNtQueryPerformanceCounter(NULL, &frequency);
    ok(status == STATUS_ACCESS_VIOLATION, "expected STATUS_ACCESS_VIOLATION, got %08lx\n", status);
    status = pNtQueryPerformanceCounter(&counter, (void *)0xdeadbee0);
    ok(status == STATUS_ACCESS_VIOLATION, "expected STATUS_ACCESS_VIOLATION, got %08lx\n", status);
    status = pNtQueryPerformanceCounter((void *)0xdeadbee0, &frequency);
    ok(status == STATUS_ACCESS_VIOLATION, "expected STATUS_ACCESS_VIOLATION, got %08lx\n", status);

    status = pNtQueryPerformanceCounter(&counter, NULL);
    ok(status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %08lx\n", status);
    status = pNtQueryPerformanceCounter(&counter, &frequency);
    ok(status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %08lx\n", status);
}

#if (defined(__i386__) || defined(__x86_64__)) && !defined(__arm64ec__)

struct hypervisor_shared_data
{
    UINT64 unknown;
    UINT64 QpcMultiplier;
    UINT64 QpcBias;
};

/* 128-bit multiply a by b and return the high 64 bits, same as __umulh */
static UINT64 multiply_tsc(UINT64 a, UINT64 b)
{
    UINT64 ah = a >> 32, al = (UINT32)a, bh = b >> 32, bl = (UINT32)b, m;
    m = (ah * bl) + (bh * al) + ((al * bl) >> 32);
    return (ah * bh) + (m >> 32);
}

static void test_RtlQueryPerformanceCounter(void)
{
    struct hypervisor_shared_data *hsd;
    KSHARED_USER_DATA *usd = (void *)0x7ffe0000;
    LARGE_INTEGER frequency, counter;
    NTSTATUS status;
    UINT64 tsc0, tsc1;
    ULONG len;
    BOOL ret;

    if (!pRtlQueryPerformanceCounter || !pRtlQueryPerformanceFrequency)
    {
        win_skip( "RtlQueryPerformanceCounter/Frequency not available, skipping tests\n" );
        return;
    }

    if (!(usd->QpcBypassEnabled & SHARED_GLOBAL_FLAGS_QPC_BYPASS_ENABLED))
    {
        todo_wine win_skip("QpcBypassEnabled is not set, skipping tests\n");
        return;
    }

    if ((usd->QpcBypassEnabled & SHARED_GLOBAL_FLAGS_QPC_BYPASS_USE_HV_PAGE))
    {
        ok( usd->QpcBypassEnabled == (SHARED_GLOBAL_FLAGS_QPC_BYPASS_ENABLED|SHARED_GLOBAL_FLAGS_QPC_BYPASS_USE_HV_PAGE|SHARED_GLOBAL_FLAGS_QPC_BYPASS_USE_RDTSCP),
            "unexpected QpcBypassEnabled %x, expected 0x83\n", usd->QpcBypassEnabled );
        ok( usd->QpcFrequency == 10000000, "unexpected QpcFrequency %I64d, expected 10000000\n", usd->QpcFrequency );
        ok( !usd->QpcShift, "unexpected QpcShift %d, expected 0\n", usd->QpcShift );

        hsd = NULL;
        status = pNtQuerySystemInformation( SystemHypervisorSharedPageInformation, &hsd, sizeof(void *), &len );
        ok( !status, "NtQuerySystemInformation returned %lx\n", status );
        ok( len == sizeof(void *), "unexpected SystemHypervisorSharedPageInformation length %lu\n", len );
        ok( !!hsd, "unexpected SystemHypervisorSharedPageInformation address %p\n", hsd );

        tsc0 = __rdtsc();
        ret = pRtlQueryPerformanceCounter( &counter );
        tsc1 = __rdtsc();
        ok( ret, "RtlQueryPerformanceCounter failed\n" );

        tsc0 = multiply_tsc(tsc0, hsd->QpcMultiplier) + hsd->QpcBias + usd->QpcBias;
        tsc1 = multiply_tsc(tsc1, hsd->QpcMultiplier) + hsd->QpcBias + usd->QpcBias;

        ok( tsc0 <= counter.QuadPart, "rdtscp %I64d and RtlQueryPerformanceCounter %I64d are out of order\n", tsc0, counter.QuadPart );
        ok( counter.QuadPart <= tsc1, "RtlQueryPerformanceCounter %I64d and rdtscp %I64d are out of order\n", counter.QuadPart, tsc1 );
    }
    else
    {
        ok( usd->QpcShift == 10, "unexpected QpcShift %d, expected 10\n", usd->QpcShift );

        tsc0 = __rdtsc();
        ret = pRtlQueryPerformanceCounter( &counter );
        tsc1 = __rdtsc();
        ok( ret, "RtlQueryPerformanceCounter failed\n" );

        tsc0 += usd->QpcBias;
        tsc0 >>= usd->QpcShift;
        tsc1 += usd->QpcBias;
        tsc1 >>= usd->QpcShift;

        ok( tsc0 <= counter.QuadPart, "rdtscp %I64d and RtlQueryPerformanceCounter %I64d are out of order\n", tsc0, counter.QuadPart );
        ok( counter.QuadPart <= tsc1, "RtlQueryPerformanceCounter %I64d and rdtscp %I64d are out of order\n", counter.QuadPart, tsc1 );
    }

    ret = pRtlQueryPerformanceFrequency( &frequency );
    ok( ret, "RtlQueryPerformanceFrequency failed\n" );
    ok( frequency.QuadPart == usd->QpcFrequency,
        "RtlQueryPerformanceFrequency returned %I64d, expected USD QpcFrequency %I64d\n",
        frequency.QuadPart, usd->QpcFrequency );
}
#endif

#define TIMER_LEEWAY 10
#define CHECK_CURRENT_TIMER(expected) \
    do { \
        ok(status == STATUS_SUCCESS, "NtSetTimerResolution failed %lx\n", status); \
        ok(cur2 == (expected) || broken(abs((int)((expected) - cur2)) <= TIMER_LEEWAY), "expected new timer resolution %lu, got %lu\n", (expected), cur2); \
        set = cur2; \
        min2 = min + 20000; \
        cur2 = min2 + 1; \
        max2 = cur2 + 1; \
        status = NtQueryTimerResolution(&min2, &max2, &cur2); \
        ok(status == STATUS_SUCCESS, "NtQueryTimerResolution() failed %lx\n", status); \
        ok(min2 == min, "NtQueryTimerResolution() expected min=%lu, got %lu\n", min, min2); \
        ok(max2 == max, "NtQueryTimerResolution() expected max=%lu, got %lu\n", max, max2); \
        ok(cur2 == set, "NtQueryTimerResolution() expected timer resolution %lu, got %lu\n", set, cur2); \
    } while (0)

static void test_TimerResolution(void)
{
    ULONG min, max, cur, min2, max2, cur2, set;
    NTSTATUS status;

    status = NtQueryTimerResolution(NULL, &max, &cur);
    ok(status == STATUS_ACCESS_VIOLATION, "NtQueryTimerResolution(NULL,,) success\n");

    status = NtQueryTimerResolution(&min, NULL, &cur);
    ok(status == STATUS_ACCESS_VIOLATION, "NtQueryTimerResolution(,NULL,) success\n");

    status = NtQueryTimerResolution(&min, &max, NULL);
    ok(status == STATUS_ACCESS_VIOLATION, "NtQueryTimerResolution(,,NULL) success\n");

    min = 212121;
    cur = min + 1;
    max = cur + 1;
    status = NtQueryTimerResolution(&min, &max, &cur);
    ok(status == STATUS_SUCCESS, "NtQueryTimerResolution() failed (%lx)\n", status);
    ok(min == 156250 /* 1/64s HPET */ || min == 156001 /* RTC */,
       "unexpected minimum timer resolution %lu\n", min);
    ok(0 < max, "invalid maximum timer resolution, should be 0 < %lu\n", max);
    ok(max <= cur || broken(max - TIMER_LEEWAY <= cur), "invalid timer resolutions, should be %lu <= %lu\n", max, cur);
    ok(cur <= min || broken(cur <= min + TIMER_LEEWAY), "invalid timer resolutions, should be %lu <= %lu\n", cur, min);

    status = NtSetTimerResolution(0, FALSE, NULL);
    ok(status == STATUS_ACCESS_VIOLATION, "NtSetTimerResolution(,,NULL) success\n");

    /* Nothing happens if that pointer is not good */
    status = NtSetTimerResolution(cur - 1, TRUE, NULL);
    ok(status == STATUS_ACCESS_VIOLATION, "NtSetTimerResolution() failed %lx\n", status);

    min2 = min + 10000;
    cur2 = min2 + 1;
    max2 = cur2 + 1;
    status = NtQueryTimerResolution(&min2, &max2, &cur2);
    ok(status == STATUS_SUCCESS, "NtQueryTimerResolution() failed (%lx)\n", status);
    ok(min2 == min, "NtQueryTimerResolution() expected min=%lu, got %lu\n", min, min2);
    ok(max2 == max, "NtQueryTimerResolution() expected max=%lu, got %lu\n", max, max2);
    ok(cur2 == cur, "NtQueryTimerResolution() expected timer resolution %lu, got %lu\n", cur, cur2);

    /* 'fails' until the first valid timer resolution request */
    cur2 = 7654321;
    status = NtSetTimerResolution(0, FALSE, &cur2);
    ok(status == STATUS_TIMER_RESOLUTION_NOT_SET, "NtSetTimerResolution() failed %lx\n", status);
    /* and returns the current timer resolution */
    ok(cur2 == cur, "expected requested timer resolution %lu, got %lu\n", cur, cur2);


    cur2 = 7654321;
    status = NtSetTimerResolution(max - 1, TRUE, &cur2);
    CHECK_CURRENT_TIMER(max);

    /* Rescinds our timer resolution request */
    cur2 = 7654321;
    status = NtSetTimerResolution(0, FALSE, &cur2);
    ok(status == STATUS_SUCCESS, "NtSetTimerResolution() failed %lx\n", status);
    /* -> the timer resolution was reset to its initial value */
    ok(cur2 == cur, "expected requested timer resolution %lu, got %lu\n", min, cur2);

    cur2 = 7654321;
    status = NtSetTimerResolution(0, FALSE, &cur2);
    ok(status == STATUS_TIMER_RESOLUTION_NOT_SET, "NtSetTimerResolution() failed %lx\n", status);
    ok(cur2 == cur, "expected requested timer resolution %lu, got %lu\n", cur, cur2);

    cur2 = 7654321;
    status = NtSetTimerResolution(min + 1, TRUE, &cur2);
    ok(status == STATUS_SUCCESS, "NtSetTimerResolution() failed %lx\n", status);
    /* This works because:
     * - Either cur is the minimum (15.6 ms) resolution already, i.e. the
     *   closest valid value 'set' is rounded to.
     * - Or some other application requested a higher timer resolution, cur,
     *   and any attempt to lower the resolution has no effect until that
     *   request is rescinded (hopefully after this test is done).
     */
    CHECK_CURRENT_TIMER(cur);

    /* The requested resolution may (win7) or may not be rounded */
    cur2 = 7654321;
    set = max < cur ? cur - 1 : max;
    status = NtSetTimerResolution(set, TRUE, &cur2);
    ok(status == STATUS_SUCCESS, "NtSetTimerResolution() failed %lx\n", status);
    ok(cur2 <= set || broken(cur2 <= set + TIMER_LEEWAY), "expected new timer resolution %lu <= %lu\n", cur2, set);
    trace("timer resolution: %lu(max) <= %lu(cur) <= %lu(prev) <= %lu(min)\n", max, cur2, cur, min);

    cur2 = 7654321;
    status = NtSetTimerResolution(cur + 1, TRUE, &cur2);
    CHECK_CURRENT_TIMER(cur); /* see min + 1 test */

    /* Cleanup by rescinding the last request */
    cur2 = 7654321;
    status = NtSetTimerResolution(0, FALSE, &cur2);
    ok(status == STATUS_SUCCESS, "NtSetTimerResolution() failed %lx\n", status);
    ok(cur2 == cur, "expected requested timer resolution %lu, got %lu\n", set, cur2);
}

static void test_RtlQueryTimeZoneInformation(void)
{
    RTL_DYNAMIC_TIME_ZONE_INFORMATION tzinfo, tzinfo2;
    NTSTATUS status;
    ULONG len;

    /* test RtlQueryTimeZoneInformation returns an indirect string,
       e.g. @tzres.dll,-32 (Vista or later) */
    if (!pRtlQueryTimeZoneInformation || !pRtlQueryDynamicTimeZoneInformation)
    {
        win_skip("Time zone name tests require Vista or later\n");
        return;
    }

    memset(&tzinfo, 0xcc, sizeof(tzinfo));
    status = pRtlQueryDynamicTimeZoneInformation(&tzinfo);
    ok(status == STATUS_SUCCESS,
       "RtlQueryDynamicTimeZoneInformation failed, got %08lx\n", status);
    ok(tzinfo.StandardName[0] == '@' ||
       broken(tzinfo.StandardName[0]), /* some win10 2004 */
       "standard time zone name isn't an indirect string, got %s\n",
       wine_dbgstr_w(tzinfo.StandardName));
    ok(tzinfo.DaylightName[0] == '@' ||
       broken(tzinfo.DaylightName[0]), /* some win10 2004 */
       "daylight time zone name isn't an indirect string, got %s\n",
       wine_dbgstr_w(tzinfo.DaylightName));

    memset(&tzinfo2, 0xcc, sizeof(tzinfo2));
    status = pNtQuerySystemInformation( SystemDynamicTimeZoneInformation, &tzinfo2, sizeof(tzinfo2), &len );
    ok( !status, "NtQuerySystemInformation failed %lx\n", status );
    ok( len == sizeof(tzinfo2), "wrong len %lu\n", len );
    ok( !memcmp( &tzinfo, &tzinfo2, sizeof(tzinfo2) ), "tz data is different\n" );

    memset(&tzinfo, 0xcc, sizeof(tzinfo));
    status = pRtlQueryTimeZoneInformation((RTL_TIME_ZONE_INFORMATION *)&tzinfo);
    ok(status == STATUS_SUCCESS,
       "RtlQueryTimeZoneInformation failed, got %08lx\n", status);
    ok(tzinfo.StandardName[0] == '@' ||
       broken(tzinfo.StandardName[0]), /* some win10 2004 */
       "standard time zone name isn't an indirect string, got %s\n",
       wine_dbgstr_w(tzinfo.StandardName));
    ok(tzinfo.DaylightName[0] == '@' ||
       broken(tzinfo.DaylightName[0]), /* some win10 2004 */
       "daylight time zone name isn't an indirect string, got %s\n",
       wine_dbgstr_w(tzinfo.DaylightName));

    memset(&tzinfo, 0xcc, sizeof(tzinfo));
    status = pRtlQueryTimeZoneInformation((RTL_TIME_ZONE_INFORMATION *)&tzinfo);
    ok(status == STATUS_SUCCESS,
       "RtlQueryTimeZoneInformation failed, got %08lx\n", status);
    ok(tzinfo.StandardName[0] == '@' ||
       broken(tzinfo.StandardName[0]), /* some win10 2004 */
       "standard time zone name isn't an indirect string, got %s\n",
       wine_dbgstr_w(tzinfo.StandardName));
    ok(tzinfo.DaylightName[0] == '@' ||
       broken(tzinfo.DaylightName[0]), /* some win10 2004 */
       "daylight time zone name isn't an indirect string, got %s\n",
       wine_dbgstr_w(tzinfo.DaylightName));

    memset(&tzinfo2, 0xcc, sizeof(tzinfo2));
    status = pNtQuerySystemInformation( SystemCurrentTimeZoneInformation, &tzinfo2,
                                        sizeof(RTL_TIME_ZONE_INFORMATION), &len );
    ok( !status, "NtQuerySystemInformation failed %lx\n", status );
    ok( len == sizeof(RTL_TIME_ZONE_INFORMATION), "wrong len %lu\n", len );
    ok( !memcmp( &tzinfo, &tzinfo2, sizeof(RTL_TIME_ZONE_INFORMATION) ), "tz data is different\n" );
}

static ULONGLONG read_ksystem_time(volatile KSYSTEM_TIME *time)
{
    ULONGLONG high, low;
    do
    {
        high = time->High1Time;
        low = time->LowPart;
    }
    while (high != time->High2Time);
    return high << 32 | low;
}

static void test_user_shared_data_time(void)
{
    KSHARED_USER_DATA *user_shared_data = (void *)0x7ffe0000;
    SYSTEM_TIMEOFDAY_INFORMATION timeofday;
    ULONGLONG t1, t2, t3;
    NTSTATUS status;
    int i = 0;

    i = 0;
    do
    {
        t1 = GetTickCount();
        if (user_shared_data->NtMajorVersion <= 5 && user_shared_data->NtMinorVersion <= 1)
            t2 = (DWORD)((*(volatile ULONG*)&user_shared_data->TickCountLowDeprecated * (ULONG64)user_shared_data->TickCountMultiplier) >> 24);
        else
            t2 = (DWORD)((read_ksystem_time(&user_shared_data->TickCount) * user_shared_data->TickCountMultiplier) >> 24);
        t3 = GetTickCount();
    } while(t3 < t1 && i++ < 1); /* allow for wrap, but only once */

    ok(t1 <= t2, "USD TickCount / GetTickCount are out of order: %s %s\n",
       wine_dbgstr_longlong(t1), wine_dbgstr_longlong(t2));
    ok(t2 <= t3, "USD TickCount / GetTickCount are out of order: %s %s\n",
       wine_dbgstr_longlong(t2), wine_dbgstr_longlong(t3));

    i = 0;
    do
    {
        LARGE_INTEGER system_time;
        NtQuerySystemTime(&system_time);
        t1 = system_time.QuadPart;
        t2 = read_ksystem_time(&user_shared_data->SystemTime);
        NtQuerySystemTime(&system_time);
        t3 = system_time.QuadPart;
    } while(t3 < t1 && i++ < 1); /* allow for wrap, but only once */

    /* FIXME: not always in order, but should be close */
    todo_wine_if(t1 > t2 && t1 - t2 < 50 * TICKSPERMSEC)
    ok(t1 <= t2, "USD SystemTime / NtQuerySystemTime are out of order %s %s\n",
       wine_dbgstr_longlong(t1), wine_dbgstr_longlong(t2));
    ok(t2 <= t3, "USD SystemTime / NtQuerySystemTime are out of order %s %s\n",
       wine_dbgstr_longlong(t2), wine_dbgstr_longlong(t3));

    if (!pRtlQueryUnbiasedInterruptTime)
        win_skip("skipping RtlQueryUnbiasedInterruptTime tests\n");
    else
    {
        i = 0;
        do
        {
            pRtlQueryUnbiasedInterruptTime(&t1);
            t2 = read_ksystem_time(&user_shared_data->InterruptTime) - user_shared_data->InterruptTimeBias;
            pRtlQueryUnbiasedInterruptTime(&t3);
        } while(t3 < t1 && i++ < 1); /* allow for wrap, but only once */

        ok(t1 <= t2, "USD InterruptTime / RtlQueryUnbiasedInterruptTime are out of order %s %s\n",
           wine_dbgstr_longlong(t1), wine_dbgstr_longlong(t2));
        ok(t2 <= t3, "USD InterruptTime / RtlQueryUnbiasedInterruptTime are out of order %s %s\n",
           wine_dbgstr_longlong(t2), wine_dbgstr_longlong(t3));
    }

    t1 = read_ksystem_time(&user_shared_data->TimeZoneBias);
    status = NtQuerySystemInformation(SystemTimeOfDayInformation, &timeofday, sizeof(timeofday), NULL);
    ok(!status, "failed to query time of day, status %#lx\n", status);
    ok(timeofday.TimeZoneBias.QuadPart == t1, "got USD bias %I64u, ntdll bias %I64u\n",
            t1, timeofday.TimeZoneBias.QuadPart);
}

START_TEST(time)
{
    HMODULE mod = GetModuleHandleA("ntdll.dll");
    pRtlTimeToTimeFields = (void *)GetProcAddress(mod,"RtlTimeToTimeFields");
    pRtlTimeFieldsToTime = (void *)GetProcAddress(mod,"RtlTimeFieldsToTime");
    pNtQueryPerformanceCounter = (void *)GetProcAddress(mod, "NtQueryPerformanceCounter");
    pNtQuerySystemInformation = (void *)GetProcAddress(mod, "NtQuerySystemInformation");
    pRtlQueryTimeZoneInformation =
        (void *)GetProcAddress(mod, "RtlQueryTimeZoneInformation");
    pRtlQueryDynamicTimeZoneInformation =
        (void *)GetProcAddress(mod, "RtlQueryDynamicTimeZoneInformation");
    pRtlQueryUnbiasedInterruptTime = (void *)GetProcAddress(mod, "RtlQueryUnbiasedInterruptTime");
    pRtlQueryPerformanceCounter = (void *)GetProcAddress(mod, "RtlQueryPerformanceCounter");
    pRtlQueryPerformanceFrequency = (void *)GetProcAddress(mod, "RtlQueryPerformanceFrequency");

    if (pRtlTimeToTimeFields && pRtlTimeFieldsToTime)
        test_pRtlTimeToTimeFields();
    else
        win_skip("Required time conversion functions are not available\n");
    test_NtQueryPerformanceCounter();
    test_RtlQueryTimeZoneInformation();
    test_user_shared_data_time();
#if (defined(__i386__) || defined(__x86_64__)) && !defined(__arm64ec__)
    test_RtlQueryPerformanceCounter();
#endif
    test_TimerResolution();
}
