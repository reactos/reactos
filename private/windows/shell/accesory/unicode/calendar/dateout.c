/* Time and date stuff                                              */
/* NOTE: Date & time input routines work only if SS == DS */

#include "windows.h"
#include "date.h"


INT FAR APIENTRY GetTimeString(PDOSTIME pdt, TCHAR *pch)
{
    BOOL  isAM;
    INT   h;
    extern BOOL vfHour24;

    if (vfHour24)
//dee         || Time.iTime)
    {
       wsprintf (pch, Time.iLZero ? TEXT("%02d%c%02d") : TEXT("%d%c%02d"),
                 pdt->hour, Time.chSep, pdt->minutes);
    }
    else
    {
       h = pdt->hour;
       if (h >= 12)
       {
          if (h > 12)
             h -= 12;
          isAM = FALSE;
       }
       wsprintf (pch, Time.iLZero ? TEXT("%02d%c%02d %s") : TEXT("%d%c%02d %s"),
                 h, Time.chSep, pdt->minutes, isAM ? Time.sz1159 : Time.sz2359);
    }

    return (lstrlen(pch));
}


INT FAR APIENTRY GetDateString(PDOSDATE pdd, TCHAR *pch, WORD format)
{
    BOOL    bLead;
    TCHAR   chSep;
    WORD    count;
    LPTSTR  szFormat;
    WORD    Size;
    register INT i = 0, j = 0;


    if (format & GDS_SHORT)
    {
        szFormat = Date.szShortFmt;
        Size = MAX_SHORTFMT;
    }
    else if (format & GDS_LONG)
    {
        szFormat = Date.szLongFmt;
        Size = MAX_LONGFMT;
    }
    else
        return (0);

    while (szFormat[i] && (i < Size - 1))
    {
        bLead = FALSE;
        count = 1;

        switch (chSep = szFormat[i++])
        {
            case TEXT('d'):
                while (szFormat[i] == TEXT('d'))
                {
                    i++;
                    count++;
                }
                bLead = count % 2;
                if (count <= 2 && !(format & GDS_NODAY))
                {
                    if (bLead || (pdd->day / 10))
                        pch[j++] = TEXT('0') + pdd->day / 10;
                    pch[j++] = TEXT('0') + pdd->day % 10;
                }
                else
                {
                    if (format & GDS_DAYOFWEEK)
                    {
                        if (pdd->dayofweek == 0xff && ValidateDosDate(pdd) < 0)
                            return (0);
                        if (bLead)
                            j += LoadString(hinstTimeDate, IDS_DAYABBREVS + pdd->dayofweek, pch + j, CCHDAY);
                        else
                           j += LoadString(hinstTimeDate, IDS_DAYSOFWEEK + pdd->dayofweek, pch + j, CCHDAY);
                    }
                    else
                    {
                        if (szFormat[i++] == TEXT('\''))
                        {
                            while (szFormat[i++] != TEXT('\''))
                                ;
                            i++;
                        }
                    }
                }
                break;

            case TEXT('M'):
                while (szFormat[i] == TEXT('M'))
                {
                    i++;
                    count++;
                }
                bLead = count % 2;
                if (count <= 2)
                {
                    if (bLead || (pdd->month / 10))
                        pch[j++] = TEXT('0') + pdd->month / 10;
                    pch[j++] = TEXT('0') + pdd->month % 10;
                }
                else
                {
                    if (bLead)
                        j += LoadString(hinstTimeDate, IDS_MONTHABBREVS + pdd->month - 1, pch + j, CCHMONTH);
                    else
                        j += LoadString(hinstTimeDate, IDS_MONTHS + pdd->month - 1, pch + j, CCHMONTH);
                }
                break;

            case TEXT('y'):
                i++;
                if (szFormat[i] == TEXT('y'))
                {
                    bLead = TRUE;
                    i += 2;
                }
                if (bLead)
                {
                    pch[j++] = (pdd->year < 2000 ? TEXT('1') : TEXT('2'));
                    pch[j++] = (pdd->year < 2000 ? TEXT('9') : TEXT('0'));
                }
                 pch[j++] = TEXT('0') + (pdd->year % 100) / 10;
                 pch[j++] = TEXT('0') + (pdd->year % 100) % 10;
                 break;

            case TEXT('\''):
                 break;

            default:
                 /* copy the current character into the formatted string - it
                  * is a separator. BUT: don't copy a separator into the
                  * very first position (could happen if the year comes first,
                  * but we're not using the year)
                  */
                 if (i)
                     pch[j++] = chSep;
                 break;
        }
    }
    while ((pch[j-1] < TEXT('0')) || (pch[j-1] > TEXT('9')))
        j--;
    pch[j] = TEXT('\0');

    return (j);
}


INT FAR APIENTRY GetMonthYear(PDOSDATE pdd, TCHAR *pch)
{
    register INT i = 0, j = 0;

    j += LoadString(hinstTimeDate, IDS_MONTHS + pdd->month - 1, pch, CCHMONTH);

    pch[j++] = TEXT(' ');

    pch[j++] = (pdd->year < 2000 ? TEXT('1') : TEXT('2'));
    pch[j++] = (pdd->year < 2000 ? TEXT('9') : TEXT('0'));
    pch[j++] = TEXT('0') + (pdd->year % 100) / 10;
    pch[j++] = TEXT('0') + (pdd->year % 100) % 10;

    pch[j] = TEXT('\0');

    return (j);
}

