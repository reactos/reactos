/*++

Copyright (c) 1994-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    timedate.h

Abstract:

    This module contains the header information for the Date/Time applet.

Revision History:

--*/



#ifndef STRICT
  #define STRICT
#endif



//
//  Include Files.
//

#include <windows.h>
#include <windowsx.h>




//
//  Constant Declarations.
//

#define CharSizeOf(x)   (sizeof(x) / sizeof(x[0]))

//
//  Index into wDateTime.
//
#define  HOUR       0
#define  MINUTE     1
#define  SECOND     2
#define  MONTH      3
#define  DAY        4
#define  YEAR       5
#define  WEEKDAY    6




//
//  Typedef Declarations.
//

#define TIMESUF_LEN   9         // time suffix length + null terminator

typedef struct
{
    TCHAR  sCountry[24];        // country name
    int    iCountry;            // country code (phone ID)
    int    iDate;               // date mode (0: MDY, 1: DMY, 2: YMD)
    int    iTime;               // time mode (0: 12 hour clock, 1: 24 hour clock)
    int    iTLZero;             // leading zeros for hour (0: no, 1: yes)
    int    iCurFmt;             // currency mode (0: prefix, no separation
                                //                1: suffix, no separation
                                //                2: prefix, 1 char separation
                                //                3: suffix, 1 char separation)
    int    iCurDec;             // currency decimal place
    int    iNegCur;             // negative currency pattern:
                                //     ($1.23), -$1.23, $-1.23, $1.23-, etc.
    int    iLzero;              // leading zeros of decimal (0: no, 1: yes)
    int    iDigits;             // significant decimal digits
    int    iMeasure;            // 0: metric, 1: US
    TCHAR  s1159[TIMESUF_LEN];  // trailing string from 0:00 to 11:59
    TCHAR  s2359[TIMESUF_LEN];  // trailing string from 12:00 to 23:59
    TCHAR  sCurrency[6];        // currency symbol string
    TCHAR  sThousand[4];        // thousand separator string
    TCHAR  sDecimal[4];         // decimal separator string
    TCHAR  sDateSep[4];         // date separator string
    TCHAR  sTime[4];            // time separator string
    TCHAR  sList[4];            // list separator string
    TCHAR  sLongDate[80];       // long date picture string
    TCHAR  sShortDate[80];      // short date picture string
    TCHAR  sLanguage[4];        // language name
    short  iDayLzero;           // day leading zero for short date format
    short  iMonLzero;           // month leading zero for short date format
    short  iCentury;            // display full century in short date format
    short  iLDate;              // long date mode (0: MDY, 1: DMY, 2: YMD)
    LCID   lcid;                // locale id
    TCHAR  sTimeFormat[80];     // time format picture string
    int    iTimeMarker;         // time marker position (0: suffix, 1: prefix)
    int    iNegNumber;          // negative number pattern:
                                //     (1.1), -1.1, - 1.1, 1.1-, 1.1 -
    TCHAR  sMonThousand[4];     // monetary thousand separator string
    TCHAR  sMonDecimal[4];      // monetary decimal separator string

} INTLSTRUCT, *LPINTL;




//
//  Global Variables.
//

extern short wDateTime[7];             // values for first 7 date/time items
extern short wPrevDateTime[7];         // only repaint fields if necessary
extern short wDeltaDateTime[6];        // amount of time change

extern HINSTANCE g_hInst;

extern INTLSTRUCT IntlDef;




//
//  Function Prototypes.
//

void
GetDateTime(void);

void
GetTime(void);

void
SetTime(void);

void
GetDate(void);

void
SetDate(void);
