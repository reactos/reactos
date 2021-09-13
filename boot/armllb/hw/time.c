/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/armllb/hw/time.c
 * PURPOSE:         LLB Time Routines
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#include "precomp.h"

#define LEAPS_THRU_END_OF(y) ((y)/4 - (y)/100 + (y)/400)

UCHAR LlbDaysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

#ifndef _ZOOM2_
TIMEINFO LlbTime;
#else
extern TIMEINFO LlbTime;
#endif

BOOLEAN
NTAPI
LlbIsLeapYear(IN ULONG Year)
{
    /* Every 4, 100, or 400 years */
    return (!(Year % 4) && (Year % 100)) || !(Year % 400);
}

ULONG
NTAPI
LlbDayOfMonth(IN ULONG Month,
              IN ULONG Year)
{
    /* Check how many days a month has, accounting for leap yearS */
    return LlbDaysInMonth[Month] + (LlbIsLeapYear(Year) && Month == 1);
}

VOID
NTAPI
LlbConvertRtcTime(IN ULONG RtcTime,
                  OUT TIMEINFO* TimeInfo)
{
    ULONG Month, Year, Days, DaysLeft;

    /* Count the days, keep the minutes */
    Days = RtcTime / 86400;
    RtcTime -= Days * 86400;

    /* Get the year, based on days since 1970 */
    Year = 1970 + Days / 365;

    /* Account for leap years which changed the number of days/year */
    Days -= (Year - 1970) * 365 + LEAPS_THRU_END_OF(Year - 1) - LEAPS_THRU_END_OF(1970 - 1);
    if (Days < 0)
    {
        /* We hit a leap year, so fixup the math */
        Year--;
        Days += 365 + LlbIsLeapYear(Year);
    }

    /* Count months */
    for (Month = 0; Month < 11; Month++)
    {
        /* How many days in this month? */
        DaysLeft = Days - LlbDayOfMonth(Month, Year);
        if (DaysLeft < 0) break;

        /* How many days left total? */
        Days = DaysLeft;
    }

    /* Write the structure */
    TimeInfo->Year = Year;
    TimeInfo->Day = Days + 1;
    TimeInfo->Month = Month + 1;
    TimeInfo->Hour = RtcTime / 3600;
    RtcTime -= TimeInfo->Hour * 3600;
    TimeInfo->Minute = RtcTime / 60;
    TimeInfo->Second = RtcTime - TimeInfo->Minute * 60;
}

TIMEINFO*
NTAPI
LlbGetTime(VOID)
{
    ULONG RtcTime;

    /* Read RTC time */
    RtcTime = LlbHwRtcRead();
#ifndef _ZOOM2_
    /* Convert it */
    LlbConvertRtcTime(RtcTime, &LlbTime);
#endif
    return &LlbTime;
}

/* EOF */
