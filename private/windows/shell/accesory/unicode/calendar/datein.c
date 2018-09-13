#include "windows.h"
#include "date.h"


INT  FAR APIENTRY ParseTimeString(pdt, pch)
DOSTIME *pdt;
register TCHAR *pch;
{
    INT h, m;
    TCHAR *pchT;
    BOOL fPM = FALSE;

    if ((pch = Ascii2Int(pch, &h)) == NULL)
        return(PD_ERRFORMAT);
    if (*pch++ != Time.chSep)
        return(PD_ERRFORMAT);
    if ((pch = Ascii2Int(pch, &m)) == NULL)
        return(PD_ERRFORMAT);

    /* Now look for match against AM or PM string */
    if (*pch != (TCHAR) 0)
    {
        /* Upper case the string in PLACE */
        CharUpper(pch);
        if (*pch == Time.sz1159[0])
            pchT = Time.sz1159;
        else if (*pch == Time.sz2359[0])
        {
            fPM = TRUE;
            pchT = Time.sz2359;
        }
        else
            return(PD_ERRFORMAT);

        /* The following is just a case-insensitive, kanji-sensitive
           string equality check */
        while (*pchT != (TCHAR) 0)
        {
            if (*pch == (TCHAR) 0 || *pch++ != *pchT++)
                return(PD_ERRFORMAT);
        }
    }

    if (!Time.iTime)
    {
        if (h > 12)
            return PD_ERRSUBRANGE;

        if (!fPM)
        {
            /* Convert 12:xx am to 0:xx */
            if (h == 12)
                h = 0;
        }
        else
        {
            if (h == 0)
                return(PD_ERRSUBRANGE);
            /* convert 0..11 to 12..23 */
            if (h < 12)
                h += 12;
        }
    }
    if (h >= 24 || m >= 60)
        return(PD_ERRSUBRANGE);

    pdt->hour = h;
    pdt->minutes = m;
    pdt->seconds = 0;
    pdt->hundredths = 0;

    return(0);
}

BOOL FAR APIENTRY ParseDateString(pdd, pch)
DOSDATE *pdd;
register TCHAR *pch;
{
    INT m, d, y;
    register INT t;
    INT cDays;

    if ((pch = Ascii2Int(pch, &m)) == NULL)
        return(PD_ERRFORMAT);
    if ((pch = SkipDateSep(pch, Date.chSep)) == NULL)
        return(PD_ERRFORMAT);
    if ((pch = Ascii2Int(pch, &d)) == NULL)
        return(PD_ERRFORMAT);
    if ((pch = SkipDateSep(pch, Date.chSep)) == NULL)
        return(PD_ERRFORMAT);
    if ((pch = Ascii2Int(pch, &y)) == NULL)
        return(PD_ERRFORMAT);

    if (*pch != (TCHAR) 0)
        return(PD_ERRFORMAT);

    switch (Date.iDate)
    {
       case 1:                 /* mdy->dmy */
           t = m; m = d; d = t;
           break;
       case 2:                 /* mdy->ymd */
           t = y; y = m; m = d; d = t;
           break;
    }

    /* if y < 100, assume he's specifying the last two digits of 19xx */
    y += iYearOffset;

    if (y < 100)
        y += 1900;

    pdd->month = m;
    pdd->year  = y;
    pdd->day   = d;

    return(ValidateDosDate(pdd));   /* validate the date */
}

TCHAR * APIENTRY SkipDateSep(pch, ch)
TCHAR *pch;
TCHAR ch;
{
    if (*pch == ch || *pch == TEXT('-') || *pch == TEXT('/'))
        return(++pch);

    return(NULL);
}

TCHAR * FAR APIENTRY Ascii2Int(pch, pw)
register TCHAR *pch;
INT *pw;
{
    register INT ch;
    INT n;
    TCHAR *pchStart;

    /* skip leading spaces */
    while (*pch == TEXT(' '))
        pch++;

    pchStart = pch;
    n = 0;

    while (n < 3000 && (ch = *pch) >= TEXT('0') && ch <= TEXT('9'))
    {
        n = n * 10 + ch - TEXT('0');
        pch++;
    }
    if (pch == pchStart)        /* return NULL if nothing parsed */
        return (NULL);

    /* skip trailing spaces */
    while (*pch == TEXT(' '))
        pch++;

    *pw = n;

    return(pch);
}

INT FAR APIENTRY ValidateDosDate(pdd)
PDOSDATE pdd;
{
    register INT d;
    register BYTE y;
    static WORD cDaysAccum[12] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
    static BYTE rgbDaysMonth[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    /* Make sure this is a valid date:
       - The year must be in the range 1980 through 2099 inclusive.
       - The month must be in the range 1 through 12 inclusive.
       - The day must be in the range 1 through the number of days
         of the specified month (which may need adjustment for leap year).
    */
    /* months are from 1..12 */
    if (pdd->month == 0 || pdd->month > 12)
        return(PD_ERRSUBRANGE);

    y = pdd->year - 1980;
    if (y > 119)
        return(PD_ERRRANGE);

    d = rgbDaysMonth[pdd->month - 1];
    if ((y & (4 - 1)) == 0 && pdd->month == 2)
        d++;

    if (pdd->day > d || pdd->day == 0)
        return(PD_ERRSUBRANGE);

    /* We have a legal date.  Now calculate day of week and store in pdd->dayofweek */

    /* calc no. days in previous years plus total up to beginning of month */
    d = y * 365 + cDaysAccum[pdd->month - 1];

    /* Add in the days for the preceding leap years. */
    if (y != 0)
        d += 1 + (y - 1) / 4;

    /* if this is a leap year and we're past feb, add in extra day. */
    if ((y & (4 - 1)) == 0 && pdd->month > 2)
        d++;

    /* add 2 since jan 1 1980 was a tuesday */
    pdd->dayofweek = ((d + pdd->day - 1 + 2) % 7);

    return(0);
}
