/* Time and date stuff                                              */
/* NOTE: Date & time input routines work only if SS == DS */

#include "windows.h"
#include <port1632.h>
#include "date.h"

CHAR    chSepDate = '/';        /* Separator character for date string */
CHAR    chSepTime = ':';        /* Separator character for time string */
CHAR    sz1159[9] = "AM";       /* Time string suffix for morning hours */
CHAR    sz2359[9] = "PM";       /* Time string suffix for afternoon */
INT     iYearOffset = 0;        /* Japanese year offset */
INT     iDate = 0;              /* short date format code */
BOOL    f24Time = FALSE;        /* True if military time, else False */
BOOL    fLZero = FALSE;         /* True iff date values get leading zeros */
HANDLE  hinstTimeDate;
INT     cchTimeMax;             /* Size of time string */
INT     cchLongDateMax;

CHAR * FAR APIENTRY Int2Ascii();
CHAR * APIENTRY Ascii2Int();
CHAR * APIENTRY SkipSep();

INT FAR APIENTRY GetTimeString(PDOSTIME pdt, CHAR *pch, WORD format)
{
    CHAR *pchSave = pch;
    CHAR *szAMPM;
    INT h;

    szAMPM = NULL;
    h = pdt->hour;
    if (!f24Time) {       /* want 12 hour clock */
        if (h >= 12) {     /* PM */
            h -= 12;
            szAMPM = sz2359;
        } else {
            szAMPM = sz1159;
        }
        if (h == 0)
            h = 12;
    }
    pch = Int2Ascii(h, pch, fLZero || (format & GTS_LEADINGZEROS));
    *pch++ = chSepTime;
    pch = Int2Ascii(pdt->minutes, pch, TRUE);
#ifdef DISABLE
    if (format & (GTS_SECONDS | GTS_HUNDREDTHS)) {
        *pch++ = chSepTime;
        pch = Int2Ascii(pdt->seconds, pch, TRUE);
    }
    if (format & GTS_HUNDREDTHS) {
        *pch++ = chSepTime;
        pch = Int2Ascii(pdt->hundredths, pch, TRUE);
    }
#endif
    if (szAMPM) {
        *pch++ = ' ';
    while (*szAMPM != 0)
            *pch++ = *szAMPM++;
    }
    if ((format & GTS_LEADINGSPACE) && *pchSave == '0')
        *pchSave = ' ';

    *pch = 0;
    return(pch - pchSave);
}

INT FAR APIENTRY GetDateString(PDOSDATE pdd, CHAR *pch, WORD format)
{
    CHAR *pchSave = pch;
    CHAR *szMonth;
    INT i1, i2, i3;
    BOOL fLZeroSave;
    LANGID PrimaryLangID = PRIMARYLANGID(GetSystemDefaultLangID());

    if ((PrimaryLangID == LANG_JAPANESE) || (PrimaryLangID == LANG_KOREAN))
    {
        pdd->year = pdd->year - iYearOffset;
        if (format != GDS_LONG)
            pdd->year %= 100;
    }
    else
    {
        pdd->year = (pdd->year - iYearOffset) % 100;
    }
    i1 = pdd->month;            /* assume mdy */
    i2 = pdd->day;
    i3 = pdd->year;
    if (iDate != 0) {
        i1 = pdd->day;          /* dmy or ymd */
        i2 = pdd->month;
        if (iDate == 2) {  /* ymd */
            i1 = pdd->year;
            i3 = pdd->day;
        }
    }

    if ((iDate == 2) && (format == GDS_LONG) &&
        ((PrimaryLangID == LANG_JAPANESE) || (PrimaryLangID == LANG_KOREAN)))
    {
        pch = Int2Ascii(i1/100, pch, fLZero);
        pch = Int2Ascii(i1%100, pch, TRUE);
    }
    else
    {
        pch = Int2Ascii(i1, pch, fLZero);
    }
    *pch++ = chSepDate;
    pch = Int2Ascii(i2, pch, fLZero);
    *pch++ = chSepDate;

    if ((iDate != 2) && (format == GDS_LONG) &&
        ((PrimaryLangID == LANG_JAPANESE) || (PrimaryLangID == LANG_KOREAN)))
    {
        pch = Int2Ascii(i3/100, pch, fLZero);
        pch = Int2Ascii(i3%100, pch, TRUE);
    }
    else
    {
        pch = Int2Ascii(i3, pch, fLZero);
    }
    *pch = 0;

    return(pch - pchSave);
}

CHAR * FAR APIENTRY Int2Ascii(val, pch, fLeadingZeros)
register INT val;
register CHAR *pch;
BOOL fLeadingZeros;
{
    INT tens;

    if ((tens = val / 10) != 0 || fLeadingZeros) {
        *pch++ = tens + '0';
    }
    *pch++ = (val % 10) + '0';
    return(pch);
}
