/*
 * PROJECT:     ntdll_apitest
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Tests for RtlQueryTimeZoneInformation
 * COPYRIGHT:   Copyright 2018 Doug Lyons
 */

#include "precomp.h"

#if 0
/*
 * RTL_SYSTEM_TIME is almost the same as the SYSTEMTIME structure defined
 * in winbase.h, however we need to define it differently here.
 * This is used by RtlQueryTimeZoneInformation and RtlSetTimeZoneInformation.
 * See: https://social.msdn.microsoft.com/Forums/en-US/home?forum=en-US and
 * Search: Reading TimeZone binary data from registry by Patrick
 *    and then look at the last post showing typedef struct SYSTEMTIME_TZI.
 */
typedef struct _RTL_SYSTEM_TIME {
    WORD wYear;
    WORD wMonth;
    WORD wDay;                  /* wDayOfWeek was here normally */
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;
    WORD wDayOfWeek;            /* wDayOfWeek relocated to here */
} RTL_SYSTEM_TIME;

/*
 * RTL_TIME_ZONE_INFORMATION is the same as the TIME_ZONE_INFORMATION structure
 * defined in winbase.h, however we need to define RTL_TIME_ZONE_INFORMATION
 * seperately here so we don't depend on winbase.h.
 * This is used by RtlQueryTimeZoneInformation and RtlSetTimeZoneInformation.
 */
typedef struct _RTL_TIME_ZONE_INFORMATION {
    LONG Bias;
    WCHAR StandardName[32];
    RTL_SYSTEM_TIME StandardDate;
    LONG StandardBias;
    WCHAR DaylightName[32];
    RTL_SYSTEM_TIME DaylightDate;
    LONG DaylightBias;
} RTL_TIME_ZONE_INFORMATION;
#endif

static NTSTATUS (WINAPI *pRtlQueryTimeZoneInformation)( RTL_TIME_ZONE_INFORMATION *);

static void test_RtlQueryTimeZoneInformation(void)
{
    RTL_TIME_ZONE_INFORMATION tzinfo;
    NTSTATUS status;
    TIME_ZONE_INFORMATION tziOld, tziNew, tziTest;
    DWORD dwRet;

    // Retrieve the current time zone information

    dwRet = GetTimeZoneInformation(&tziOld);

    ok(dwRet == TIME_ZONE_ID_STANDARD || dwRet == TIME_ZONE_ID_UNKNOWN || dwRet == TIME_ZONE_ID_DAYLIGHT,
        "Get Time Zone Name failed with error = %ld.\n", GetLastError());

    // Adjust the time zone information

    ZeroMemory(&tziNew, sizeof(tziNew));
    tziNew.Bias = 360;
    wcscpy(tziNew.StandardName, L"Test Standard Zone");
    tziNew.StandardDate.wMonth = 11;
    tziNew.StandardDate.wDayOfWeek = 5;
    tziNew.StandardDate.wDay = 3;
    tziNew.StandardDate.wHour = 2;
    tziNew.StandardBias = 120;

    wcscpy(tziNew.DaylightName, L"Test Daylight Zone");
    tziNew.DaylightDate.wMonth = 4;
    tziNew.DaylightDate.wDayOfWeek = 6;
    tziNew.DaylightDate.wDay = 2;
    tziNew.DaylightDate.wHour = 2;
    tziNew.DaylightBias = -60;

    // Set up SetLastError with known value for later testing

    SetLastError(0xDEADBEEF);

    ok(SetTimeZoneInformation(&tziNew) ,
        "Set Time Zone Information failed with error = %ld.\n", GetLastError());

    // if we got an error the function failed, so there is not much else we can do

    if(GetLastError() != 0xDEADBEEF)
    {
        win_skip("SetTimeZoneInformation() is not available, so tests cannot be run.\n");
        return;
    }

    // Retrieve and display the newly set time zone information

    dwRet = GetTimeZoneInformation(&tziTest);

    ok(dwRet == TIME_ZONE_ID_STANDARD || dwRet == TIME_ZONE_ID_UNKNOWN || dwRet == TIME_ZONE_ID_DAYLIGHT,
        "Get Time Zone Information Returned failed with error = %ld.\n", GetLastError());

    ok(!wcscmp(tziTest.StandardName, tziNew.StandardName),
        "Standard Time Zone Name = %ls, expected %ls.\n", tziTest.StandardName, tziNew.StandardName);

    ok(!wcscmp(tziTest.DaylightName, tziNew.DaylightName),
        "Standard Time Zone Name = %ls, expected %ls.\n", tziTest.DaylightName, tziNew.DaylightName);

    /* test RtlQueryTimeZoneInformation returns a TIME_ZONE_INFORMATION structure */

    if (!pRtlQueryTimeZoneInformation)
    {
        win_skip("pRtlQueryTimeZoneInformation() fails, so tests cannot be run.\n");
        return;
    }

    /* Clear Time Zone Info field */
    memset(&tzinfo, 0, sizeof(tzinfo));

    /* Read Time Zone Info */
    status = pRtlQueryTimeZoneInformation(&tzinfo);
    ok(status == STATUS_SUCCESS, "pRtlQueryTimeZoneInformation failed, got %08lx\n", status);

    /* Check for the Daylight Date Info */
    ok(tzinfo.DaylightDate.Month == 4, "tzinfo.DaylightDate.wMonth expected '4', got '%d'.\n", tzinfo.DaylightDate.Month);
    ok(tzinfo.DaylightDate.Day == 2, "tzinfo.DaylightDate.wDay expected '2', got '%d'.\n", tzinfo.DaylightDate.Day);
    ok(tzinfo.DaylightDate.Weekday == 6, "tzinfo.DaylightDate.wDayOfWeek expected '6', got '%d'.\n", tzinfo.DaylightDate.Weekday);
    ok(tzinfo.DaylightDate.Year == 0, "tzinfo.DaylightDate.wYear expected '0', got '%d'.\n", tzinfo.DaylightDate.Year);

    /* Check for the Standard Data Info */
    ok(tzinfo.StandardDate.Month == 11, "tzinfo.StandardDate.wMonth expected '11', got '%d'.\n", tzinfo.StandardDate.Month);
    ok(tzinfo.StandardDate.Day == 3, "tzinfo.StandardDate.wDay expected '3', got '%d'.\n", tzinfo.StandardDate.Day);
    ok(tzinfo.StandardDate.Weekday == 5, "tzinfo.StandardDate.wDayOfWeek expected '5', got '%d'.\n", tzinfo.StandardDate.Weekday);
    ok(tzinfo.StandardDate.Year == 0, "tzinfo.StandardDate.wYear expected '0', got '%d'.\n", tzinfo.StandardDate.Year);

    /* Check for the Bias Info */
    ok(tzinfo.Bias == 360, "tzinfo.Bias expected '360', got '%ld'.\n", tzinfo.Bias);
    ok(tzinfo.DaylightBias == -60, "tzinfo.DaylightBias expected '-60', got '%ld'.\n", tzinfo.DaylightBias);
    ok(tzinfo.StandardBias == 120, "tzinfo.StandardBias expected '120', got '%ld'.\n", tzinfo.StandardBias);

    // Restore the original time zone information and put things back like we found them originally
    ok(SetTimeZoneInformation(&tziOld),
        "Set Time Zone Information failed with error = %ld.\n", GetLastError());

}


START_TEST(RtlQueryTimeZoneInformation)
{
    /* Test modeled after reactos\modules\rostests\winetests\ntdll\time.c for Vista+ */
    HMODULE mod = GetModuleHandleA("ntdll.dll");
    pRtlQueryTimeZoneInformation = (void *)GetProcAddress(mod, "RtlQueryTimeZoneInformation");

    test_RtlQueryTimeZoneInformation();
}
