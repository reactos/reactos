#include "windows.h"
#include "date.h"
#include "uniconv.h"


TIME    Time;
DATE    Date;
WORD    iYearOffset;
HANDLE  hinstTimeDate;
WORD    cchTimeMax;             /* Maximum size of time string */
WORD    cchLongDateMax;         /* Maximum size of long date string */


VOID CalcCchDateMax(idFirst, idLast)
WORD idFirst;
WORD idLast;
{
    TCHAR rgch[CCHMONTH];
    register INT cch, cchT;

    cch = 0;
    while (idFirst <= idLast)
    {
        cchT = LoadString(hinstTimeDate, idFirst++, rgch, CCHMONTH);
        if (cchT > cch)
            cch = cchT;
    }
    cchLongDateMax += cch;
}


VOID LockStrings(id)
WORD id;
{
    HANDLE h;

    if ((h = FindResource (hinstTimeDate, (LPTSTR) MAKEINTRESOURCE(IDS_DATESTRINGS >> 4), RT_STRING))
        && (h = LoadResource(hinstTimeDate, h)))
    {
        GlobalLock(h);
    }
}


VOID FAR APIENTRY InitTimeDate (HANDLE hInstance)
{
  LCID   lcid;
  int    i, id;
  TCHAR  szBuf[3];

  extern TCHAR    szDec[5];

    hinstTimeDate = hInstance;

    /* get current locale */
    lcid = GetUserDefaultLCID ();

    /* Get time related info */
    GetLocaleInfoW (lcid, LOCALE_ITIME, (LPWSTR) szBuf, CharSizeOf(szBuf));
    Time.iTime = MyAtoi (szBuf);

    GetLocaleInfoW (lcid, LOCALE_ITLZERO, (LPWSTR) szBuf, CharSizeOf(szBuf));
    Time.iLZero = MyAtoi (szBuf);

    GetLocaleInfoW (lcid, LOCALE_S1159, (LPWSTR) Time.sz1159, CharSizeOf(Time.sz1159));
    GetLocaleInfoW (lcid, LOCALE_S2359, (LPWSTR) Time.sz2359, CharSizeOf(Time.sz2359));
    GetLocaleInfoW (lcid, LOCALE_STIME, (LPWSTR) szBuf, CharSizeOf(szBuf));
    Time.chSep = *szBuf;

    /* Calc max size of time string */
    if (!Time.iTime)
       cchTimeMax = max (lstrlen(Time.sz2359), lstrlen(Time.sz1159));
    cchTimeMax += 6;

    /* Get date related info */
    iYearOffset  = GetProfileInt(TEXT("intl"), TEXT("iYearOffset"), 0);

    GetLocaleInfoW (lcid, LOCALE_IDATE, (LPWSTR) szBuf, CharSizeOf(szBuf));
    Date.iDate = MyAtoi (szBuf);

    GetLocaleInfoW (lcid, LOCALE_ILZERO, (LPWSTR) szBuf, CharSizeOf(szBuf));
    Date.iLZero = MyAtoi (szBuf);

    /* Get short date format */
    GetLocaleInfoW (lcid, LOCALE_SSHORTDATE, (LPWSTR) Date.szShortFmt, MAX_SHORTFMT);

    /* Get long date format */
    GetLocaleInfoW (lcid, LOCALE_SLONGDATE, (LPWSTR) Date.szLongFmt, CCHDATEDISP);

    /* Get date/time strings into memory & lock down for all time */
    LockStrings(((WORD)IDS_DATESTRINGS >> 4) + 0);
    LockStrings(((WORD)IDS_DATESTRINGS >> 4) + 1);

    /* Now calculate worst case size of long date string */
    /* this is sum of separator strings, plus max of months, plus max of
       weekdays, plus 2 digits, plus 4 digits, plus zero terminator */
    cchLongDateMax = 2 + 4;

    CalcCchDateMax(IDS_MONTHS, IDS_MONTHS + 11);

    CalcCchDateMax(IDS_DAYSOFWEEK, IDS_DAYSOFWEEK + 6);

    cchLongDateMax += 5;    /* room for spaces and commas */

    /* Get the intl decimal character for use in Page Setup Box. */
    GetLocaleInfoW(lcid, LOCALE_SDECIMAL, szDec, CharSizeOf(szDec));
}
