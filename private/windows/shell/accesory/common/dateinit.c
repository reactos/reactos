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

BOOL FAR APIENTRY InitTimeDate (HANDLE hInstance, WORD format)
{
    static CHAR szIntl[] = "intl";
    CHAR rgch[2];

    hinstTimeDate = hInstance;

    iDate = GetProfileInt((LPSTR)szIntl, (LPSTR)"iDate", 0);

    if (format & GTS_24HOUR)
        f24Time = TRUE;
    else if (format & GTS_12HOUR)
        f24Time = FALSE;
    else
        f24Time = GetProfileInt((LPSTR)szIntl, (LPSTR)"iTime", 0);

    if (format & GTS_LEADINGZEROS)
        fLZero = TRUE;
    else
        fLZero  = GetProfileInt((LPSTR)szIntl, (LPSTR)"iLzero", 0);

    iYearOffset  = GetProfileInt((LPSTR)szIntl, (LPSTR)"iYearOffset", 0);

    GetProfileString((LPSTR)szIntl, (LPSTR)"s1159", (LPSTR)sz1159,
                           (LPSTR)sz1159, 9);

    GetProfileString((LPSTR)szIntl, (LPSTR)"s2359", (LPSTR)sz2359,
                           (LPSTR)sz2359, 9);

    GetProfileString((LPSTR)szIntl, (LPSTR)"sDate", (LPSTR)"/", (LPSTR)rgch, 2);
    chSepDate = rgch[0];

    GetProfileString((LPSTR)szIntl, (LPSTR)"sTime", (LPSTR)":", (LPSTR)rgch, 2);
    chSepTime = rgch[0];
    return TRUE;
}
