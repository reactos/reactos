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
extern INT     cchTimeMax;
extern INT     cchLongDateMax;
extern CHAR    *rgszDayAbbrevs[];

void
LockStrings(id)
WORD id;
{
    HANDLE h;

    if ((h = FindResource(hinstTimeDate, MAKEINTRESOURCE(IDS_DATESTRINGS >> 4), RT_STRING))
            && (h = LoadResource(hinstTimeDate, h))) {
        GlobalLock(h);
    }
}

void
CalcCchDateMax(idFirst, idLast)
WORD idFirst;
WORD idLast;
{
    CHAR rgch[30];
    register INT cch, cchT;

    cch = 0;
    while (idFirst <= idLast) {
        cchT = LoadString(hinstTimeDate, idFirst++, (LPSTR)rgch, 30);
        if (cchT > cch)
            cch = cchT;
    }
    cchLongDateMax += cch;
}

BOOL FAR APIENTRY InitLongTimeDate(WORD format)
{
    INT cchT;
    INT i;
    CHAR rgch[30];
    INT FAR lstrlen();
    LANGID PrimaryLangID = PRIMARYLANGID(GetSystemDefaultLangID());

    /* Get date/time strings into memory & lock down for all time */
    LockStrings(((WORD)IDS_DATESTRINGS >> 4) + 0);
    LockStrings(((WORD)IDS_DATESTRINGS >> 4) + 1);

    /* Now calculate worst case size of long date string */
    /* this is sum of separator strings, plus max of months, plus max of
       weekdays, plus 2 digits, plus 4 digits, plus zero terminator */
    cchLongDateMax = 2 + 4;
    CalcCchDateMax(IDS_MONTHS,     IDS_MONTHS+11);
#ifdef LEPPARD
    for (i = IDS_SEPSTRINGS; i <= IDS_SEPSTRINGS+4; i++) {
        cchLongDateMax += LoadString(hinstTimeDate, i, (LPSTR)rgch, 30);
    }
#else
    cchLongDateMax += 5;    /* room for spaces and commas */
#endif
    CalcCchDateMax(IDS_DAYSOFWEEK, IDS_DAYSOFWEEK+6);

    //
    //  See if it's Japanese or Korean.
    //
    if ((PrimaryLangID == LANG_JAPANESE) || (PrimaryLangID == LANG_KOREAN))
    {
        cchLongDateMax += LoadString(hinstTimeDate, IDS_SEPSTRINGS+5, (LPSTR)rgch, 30);
        cchLongDateMax += LoadString(hinstTimeDate, IDS_SEPSTRINGS+6, (LPSTR)rgch, 30);
    }

    /* Calc max size of time string */
    cchT = 0;
    if (!f24Time) {
        cchT = lstrlen((LPSTR)sz2359);
        cchTimeMax = lstrlen((LPSTR)sz1159);
        if (cchT > cchTimeMax)
            cchTimeMax = cchT;
    }
    cchTimeMax += 6;
    return TRUE;
}
