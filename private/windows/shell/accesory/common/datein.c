#include "windows.h"
#include <port1632.h>
#include "date.h"

extern CHAR    chSepDate;
extern CHAR    chSepTime;
extern CHAR    sz1159[];
extern CHAR    sz2359[];
extern INT     iDate;
extern INT     iYearOffset;
extern BOOL    f24Time;
extern BOOL    fLZero;
extern HANDLE  hinstTimeDate;

CHAR * APIENTRY Int2Ascii();
CHAR * FAR APIENTRY Ascii2Int();
CHAR * APIENTRY SkipDateSep();
CHAR * APIENTRY GetMonthString();
CHAR * APIENTRY GetWeekString();

INT  FAR APIENTRY ParseTimeString(pdt, pch)
DOSTIME *pdt;
register CHAR *pch;
{
    INT h, m;
    CHAR *pchT;
    CHAR ch;
    BOOL fPM;

    if ((pch = Ascii2Int(pch, &h)) == NULL)
        return(PD_ERRFORMAT);
    if (*pch++ != chSepTime)
        return(PD_ERRFORMAT);
    if ((pch = Ascii2Int(pch, &m)) == NULL)
        return(PD_ERRFORMAT);

    /* Now look for match against AM or PM string */
    fPM = FALSE;
    if (*pch != 0) {
        /* Upper case the string in PLACE */
        AnsiUpper((LPSTR)pch);
        ch = *pch;
        if (ch == sz1159[0]) {
            pchT = sz1159;
        } else if (ch == sz2359[0]) {
            fPM = TRUE;
            pchT = sz2359;
        } else {
            return(PD_ERRFORMAT);
        }
        /* The following is just a case-insensitive, kanji-sensitive
           string equality check */
        while (*pchT != 0) {
            if (*pch == 0 || *pch++ != *pchT++)
                return(PD_ERRFORMAT);
        }
    }

    if (!f24Time) {
        if (h > 12)
            return PD_ERRSUBRANGE;

        if (!fPM) {
            /* Convert 12:xx am to 0:xx */
            if (h == 12)
                h = 0;
        } else {
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
register CHAR *pch;
{
    INT m, d, y;
    register INT t;
    INT cDays;

    if ((pch = Ascii2Int(pch, &m)) == NULL)
        return(PD_ERRFORMAT);
    if ((pch = SkipDateSep(pch, chSepDate)) == NULL)
        return(PD_ERRFORMAT);
    if ((pch = Ascii2Int(pch, &d)) == NULL)
        return(PD_ERRFORMAT);
    if ((pch = SkipDateSep(pch, chSepDate)) == NULL)
        return(PD_ERRFORMAT);
    if ((pch = Ascii2Int(pch, &y)) == NULL)
        return(PD_ERRFORMAT);

    if (*pch != 0)
        return(PD_ERRFORMAT);

    switch (iDate) {
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

CHAR * APIENTRY SkipDateSep(pch, ch)
CHAR *pch;
CHAR ch;
{
    if (*pch == ch || *pch == '-' || *pch == '/')
        return(++pch);

    return(NULL);
}

CHAR * FAR APIENTRY Ascii2Int(pch, pw)
register CHAR *pch;
DWORD *pw;     //- Changed from WORD to DWORD for 32 bit ints. 7/11/91 t-davema
{
    register INT ch;
    DWORD n;
    CHAR *pchStart;

    /* skip leading spaces */
    while (*pch == ' ')
        pch++;

    pchStart = pch;
    n = 0;
    /* OLD: while (n < 3000 && !IsTwoByteCharPrefix(ch = *pch) && *pch >= '0' && ch <= '9') { */
    while (n < 3000 && /*!IsTwoByteCharPrefix(ch = *pch) &&*/ (ch = *pch) >= '0' && ch <= '9') {
        n = n * 10 + ch - '0';
        pch++;
    }
    if (pch == pchStart)        /* return NULL if nothing parsed */
	return (NULL);

    /* skip trailing spaces */
    while (*pch == ' ')
        pch++;

    *pw = n;

    return(pch);
}

INT FAR APIENTRY ValidateDosDate(pdd)
PDOSDATE pdd;
{
    register WORD d;
    register WORD y;
    static INT cDaysAccum[12] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
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
    if ((y & (4-1)) == 0 && pdd->month == 2)
        d++;

    if ((WORD)(pdd->day) > (WORD)d || (pdd->day == 0))
        return(PD_ERRSUBRANGE);

    /* We have a legal date.  Now calculate day of week and store in pdd->dayofweek */

    /* calc no. days in previous years plus total up to beginning of month */
    d = y * 365 + cDaysAccum[pdd->month - 1];

    /* Add in the days for the preceding leap years. */
    if (y != 0)
        d += 1 + (y - 1) / 4;

    /* if this is a leap year and we're past feb, add in extra day. */
    if ((y & (4-1)) == 0 && pdd->month > 2)
        d++;

    /* add 2 since jan 1 1980 was a tuesday */
    pdd->dayofweek = ((d + pdd->day - 1 + 2) % 7);
    return(0);
}
