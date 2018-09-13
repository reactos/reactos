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

CHAR * FAR APIENTRY Int2Ascii();
CHAR * APIENTRY LoadDateString();

#define LDS_DAYOFWEEK   0
#define LDS_MONTH       1
#define LDS_DAY         2
#define LDS_YEAR        3

BYTE mpIFmt[3][4] = {
    { LDS_DAYOFWEEK, LDS_MONTH, LDS_DAY, LDS_YEAR },
    { LDS_DAYOFWEEK, LDS_DAY, LDS_MONTH, LDS_YEAR },
    { LDS_YEAR, LDS_MONTH, LDS_DAY, LDS_DAYOFWEEK }
};

INT FAR APIENTRY GetLongDateString(PDOSDATE pdd, CHAR *pch, WORD format)
{
    register INT i;
    CHAR *pchSave = pch;
    CHAR *szMonth;
    INT i1, i2, i3;
    INT y;
    LANGID PrimaryLangID = PRIMARYLANGID(GetSystemDefaultLangID());

    for (i = 1; i <= 4; i++) {
        switch (mpIFmt[iDate][i - 1]) {
        case LDS_DAYOFWEEK:
            if ((format & GDS_DAYOFWEEK)) {
                if (pdd->dayofweek == 0xff && ValidateDosDate(pdd) < 0)
                    return(0);
                pch = LoadDateString(pch, IDS_DAYSOFWEEK + pdd->dayofweek);
                /* If day of week is at start of string, stick in comma */
                if (i == 1)
                    *pch++ = ',';
                if (i != 4)
                    *pch++ = ' ';
            }
            break;
        case LDS_MONTH:
            pch = LoadDateString(pch, pdd->month - 1 + IDS_MONTHS);
            *pch++ = ' ';
            break;
        case LDS_DAY:
            if (!(format & GDS_NODAY)) {
                pch = Int2Ascii(pdd->day, pch, fLZero);
                //
                //  If it's Japanese or Korean, get native name for year
                //  from resource.
                //
                if ((PrimaryLangID == LANG_JAPANESE) ||
                    (PrimaryLangID == LANG_KOREAN))
                {
                    pch = LoadDateString(pch, IDS_SEPSTRINGS + 6);
                }
                if (iDate == 0) {
                    *pch++ = ',';
                }
                *pch++ = ' ';
            }
            break;
        case LDS_YEAR:
            y = pdd->year - iYearOffset;
            pch = Int2Ascii(y / 100, pch, TRUE);
            pch = Int2Ascii(y % 100, pch, TRUE);
            //
            //  If it's Japanese or Korean, get native name for month
            //  from resource.
            //
            if ((PrimaryLangID == LANG_JAPANESE) ||
                (PrimaryLangID == LANG_KOREAN))
            {
                pch = LoadDateString(pch, IDS_SEPSTRINGS + 5);
            }
            if (i != 4)
                *pch++ = ' ';
            break;
        }
    }
    *pch = 0;
    return(pch - pchSave);
}

CHAR * APIENTRY LoadDateString(pch, id)
register CHAR *pch;
INT id;
{
    return(pch + LoadString(hinstTimeDate, id, (LPSTR)pch, 30));
}
