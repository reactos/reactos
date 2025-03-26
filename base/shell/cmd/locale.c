/*
 *  LOCALE.C - locale handling.
 *
 *
 *  History:
 *
 *    09-Jan-1999 (Eric Kohl)
 *        Started.
 *
 *    20-Jan-1999 (Eric Kohl)
 *        Unicode safe!
 */

#include "precomp.h"

TCHAR cDateSeparator;
TCHAR cTimeSeparator;
TCHAR cThousandSeparator;
TCHAR cDecimalSeparator;
INT   nDateFormat;
INT   nTimeFormat;
INT   nNumberGroups;


VOID InitLocale(VOID)
{
    TCHAR szBuffer[256];

    /* date settings */
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDATE, szBuffer, ARRAYSIZE(szBuffer));
    cDateSeparator = szBuffer[0];
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_IDATE | LOCALE_RETURN_NUMBER, (LPTSTR)&nDateFormat, sizeof(nDateFormat) / sizeof(TCHAR));

    /* time settings */
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STIME, szBuffer, ARRAYSIZE(szBuffer));
    cTimeSeparator = szBuffer[0];
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_ITIME | LOCALE_RETURN_NUMBER, (LPTSTR)&nTimeFormat, sizeof(nTimeFormat) / sizeof(TCHAR));

    /* number settings */
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, szBuffer, ARRAYSIZE(szBuffer));
    cThousandSeparator = szBuffer[0];
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, szBuffer, ARRAYSIZE(szBuffer));
    cDecimalSeparator  = szBuffer[0];
        GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SGROUPING, szBuffer, ARRAYSIZE(szBuffer));
        nNumberGroups = _ttoi(szBuffer);
#if 0
    /* days of week */
    for (i = 0; i < 7; i++)
    {
        GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SABBREVDAYNAME1 + i, szBuffer, ARRAYSIZE(szBuffer));
        _tcscpy(aszDayNames[(i+1)%7], szBuffer); /* little hack */
    }
#endif
}

/* Return date string without weekday. Used for $D in prompt and %DATE% */
LPTSTR
GetDateString(VOID)
{
    static TCHAR szDate[32];
    SYSTEMTIME t;

    GetLocalTime(&t);
    GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &t, NULL, szDate, ARRAYSIZE(szDate));
    return szDate;
}

/* Return time in H:mm:ss.SS format. Used for $T in prompt and %TIME% */
LPTSTR
GetTimeString(VOID)
{
    static TCHAR szTime[12];
    SYSTEMTIME t;
    GetLocalTime(&t);
    _stprintf(szTime, _T("%2d%c%02d%c%02d%c%02d"),
        t.wHour, cTimeSeparator, t.wMinute, cTimeSeparator,
        t.wSecond, cDecimalSeparator, t.wMilliseconds / 10);
    return szTime;
}
