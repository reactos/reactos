/*++

Copyright (c) 1994-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    date.c

Abstract:

    This module implements the textual representations of
    Day Month Year : Hours Min Seconds for the Date/Time applet.

Revision History:

--*/



//
//  Include Files.
//

#include "timedate.h"
#include <commctrl.h>
#include <prsht.h>
#include <regstr.h>
#include <imm.h>
#include "clock.h"
#include "mapctl.h"
#include "rc.h"
#include <help.h>




//
//  Constant Declarations.
//

#define TZNAME_SIZE          128
#define TZDISPLAYZ           128

#define BIAS_ONE_HOUR        (-60L)

#define ZONE_IMAGE_SCALE     (356)
#define ZONE_BIAS_SCALE      (-1440)
#define ZONE_IMAGE_LEFT      (120)
#define ZONE_IMAGE_WIDTH     (80)

#define BIAS_PLUS_12         (12L * BIAS_ONE_HOUR)
#define BIAS_MINUS_12        (- BIAS_PLUS_12)

#define TZ_HIT_NONE          (0)
#define TZ_HIT_BASE          (1)
#define TZ_HIT_PARTIAL       (2)
#define TZ_HIT_EXACT         (3)


//
//  Global Variables.
//

TCHAR const szIntl[] = TEXT("intl");

//
//  Default used if none could be found.
//
INTLSTRUCT IntlDef =
{
    TEXT("Other Country"),
    1, 0, 0, 0, 0, 2, 0, 1, 2, 1,
    TEXT("AM"),
    TEXT("PM"),
    TEXT("$"),
    TEXT(","),
    TEXT("."),
    TEXT("/"),
    TEXT(":"),
    TEXT(","),
    TEXT("dddd, MMMM dd, yyyy"),
    TEXT("M/d/yyyy"),
    TEXT("USA"),
    1, 0, 1, 0, 0x0409,
    TEXT("hh:mm:ss tt"),
    0, 1,
    TEXT(","),
    TEXT(".")
};

BOOL g_bFirstBoot = FALSE;   // for first boot during setup

int g_Time[3];               // time the user currently has set
int g_LastTime[3];           // last displayed time - to stop flicker

short wDateTime[7];                 // values for first 7 date/time items
short wPrevDateTime[7];             // only repaint fields if necessary
BOOL  fDateDirty;

//
//  Formatting strings for AM and PM
//
TCHAR sz1159[12];
TCHAR sz2359[12];

//
//  Are we in 24 hour time.  If not, is it AM or PM.
//
BOOL g_b24HR;
BOOL g_bPM;

//
//  This flag indicates if the user has tried to change the time.
//  If so, then we stop providing the system time and use the
//  time that we store internally. We send the clock control our
//  TimeProvider function.
//
WORD g_Modified = 0;
WORD g_WasModified = 0;

//
//  Which of the HMS MDY have leading zeros.
//
BOOL g_bLZero[6] = {FALSE, TRUE, TRUE, FALSE, FALSE, FALSE};

//
//  Ranges of HMS MDY
//
struct
{
    int nMax;
    int nMin;
} g_sDateInfo[] =
{
    23, 0,
    59, 0,
    59, 0,
    12, 1,
    31, 1,
    2099, 1980,
};

//
//  Time Zone info globals.
//
int g_nTimeZones = 0;
TIME_ZONE_INFORMATION g_tziCurrent, *g_ptziCurrent = NULL;

//
//  Registry location for Time Zone information.
//
#ifdef WINNT
  TCHAR c_szTimeZones[] = TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones");
#else
  TCHAR c_szTimeZones[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Time Zones");
#endif

//
//  Time Zone data value keys.
//
TCHAR c_szTZDisplayName[]  = TEXT("Display");
TCHAR c_szTZStandardName[] = TEXT("Std");
TCHAR c_szTZDaylightName[] = TEXT("Dlt");
TCHAR c_szTZI[]            = TEXT("TZI");
TCHAR c_szTZMapInfo[]      = TEXT("MapID");

//
//  IME globals.
//
HIMC g_PrevIMCForDateField;




//
//  Context Help Ids.
//

const DWORD aDateTimeHelpIds[] =
{
    IDD_GROUPBOX1,      IDH_DATETIME_DATE_GROUP,
    IDD_GROUPBOX2,      IDH_DATETIME_TIME,
    DATETIME_CURTZ,     IDH_DATETIME_CURRENT_TIME_ZONE,
    DATETIME_CALENDAR,  IDH_DATETIME_DATE,
    DATETIME_CLOCK,     IDH_DATETIME_TIME,
    DATETIME_TBORDER,   IDH_DATETIME_TIME,
    DATETIME_HOUR,      IDH_DATETIME_TIME,
    DATETIME_TSEP1,     IDH_DATETIME_TIME,
    DATETIME_MINUTE,    IDH_DATETIME_TIME,
    DATETIME_TSEP2,     IDH_DATETIME_TIME,
    DATETIME_SECOND,    IDH_DATETIME_TIME,
    DATETIME_AMPM,      IDH_DATETIME_TIME,
    DATETIME_TARROW,    IDH_DATETIME_TIME,
    DATETIME_MONTHNAME, IDH_DATETIME_MONTH,
    DATETIME_YEAR,      IDH_DATETIME_YEAR,
    DATETIME_YARROW,    IDH_DATETIME_YEAR,
    IDD_TIMEZONES,      IDH_DATETIME_TIMEZONE,
//  IDD_TIMEMAP,        IDH_DATETIME_BITMAP,
    IDD_TIMEMAP,        NO_HELP,
    IDD_AUTOMAGIC,      IDH_DATETIME_DAYLIGHT_SAVE,

    0, 0
};




//
//  Typedef Declarations.
//

//
//  Registry info goes in this structure.
//
typedef struct tagTZINFO
{
    struct tagTZINFO *next;
    TCHAR            szDisplayName[TZDISPLAYZ];
    TCHAR            szStandardName[TZNAME_SIZE];
    TCHAR            szDaylightName[TZNAME_SIZE];
    int              ComboIndex;
    int              SeaIndex;
    int              LandIndex;
    int              MapLeft;
    int              MapWidth;
    LONG             Bias;
    LONG             StandardBias;
    LONG             DaylightBias;
    SYSTEMTIME       StandardDate;
    SYSTEMTIME       DaylightDate;

} TZINFO, *PTZINFO;

//
//  State info for the time zone page.
//
typedef struct
{
    PTZINFO zone;
    BOOL initializing;
    PTZINFO lookup[MAPCTL_MAX_INDICES];

} TZPAGE_STATE;



DWORD GetTextExtent(
    HDC hdc,
    LPCTSTR lpsz,
    int cb);


#ifndef UNICODE

////////////////////////////////////////////////////////////////////////////
//
//  WideStrToStr
//
////////////////////////////////////////////////////////////////////////////

int WINAPI WideStrToStr(
    LPSTR psz,
    LPWSTR pwsz)
{
    return WideCharToMultiByte(CP_ACP, 0, pwsz, -1, psz, MAX_PATH, NULL, NULL);
}


////////////////////////////////////////////////////////////////////////////
//
//  StrToWideStr
//
////////////////////////////////////////////////////////////////////////////

int WINAPI StrToWideStr(
    LPWSTR pwsz,
    LPCSTR psz)
{
    return MultiByteToWideChar(CP_ACP, 0, psz, -1, pwsz, MAX_PATH);
}


////////////////////////////////////////////////////////////////////////////
//
//  AnsiWideStrCmpI
//
////////////////////////////////////////////////////////////////////////////

int AnsiWideStrCmpI(
    LPCSTR pszAnsi,
    WCHAR *pwszWide)
{
    char szAnsi[64];

    WideStrToStr(szAnsi, pwszWide);
    return (lstrcmpi(pszAnsi, szAnsi));
}

#endif



////////////////////////////////////////////////////////////////////////////
//
//  ParseDateElement
//
//  Assumes that the character pointed to by pszElement is a
//  'M', 'd', or 'y', and checks if the string indicates a leading zero
//  or century.  The return value is a pointer to the next character,
//  which should be a separator or NULL.  A return value of NULL indicates
//  an error.
//
////////////////////////////////////////////////////////////////////////////

LPTSTR ParseDateElement(
    LPTSTR pszElement,
    BOOL *pbLZero)
{
    //
    //  Check for valid character.
    //
    switch (*pszElement)
    {
        case ( TEXT('y') ) :
        case ( TEXT('M') ) :
        case ( TEXT('d') ) :
        {
            break;
        }
        default:
        {
            return (NULL);
        }
    }

    ++pszElement;

    if (*pszElement != *(pszElement - 1))
    {
        *pbLZero = 0;
    }
    else
    {
        *pbLZero = 1;

        if (*pszElement++ == TEXT('y'))
        {
            if (!(*pszElement == TEXT('y')))
            {
                *pbLZero = 0;
            }
            else
            {
                if (!(*++pszElement == TEXT('y')))
                {
                    //
                    //  Found 3 y's, invalid format.
                    //
                    return (NULL);
                }
                else
                {
                    ++pszElement;
                }
            }
        }
    }

    return (pszElement);
}


int rgMoveTimeControls [] = 
{
    DATETIME_HOUR,
    DATETIME_MINUTE,
    DATETIME_SECOND,
    DATETIME_TSEP1,
    DATETIME_TSEP2
};


////////////////////////////////////////////////////////////////////////////
//
//  AdjustAMPMPosition
//
////////////////////////////////////////////////////////////////////////////
void AdjustAMPMPosition(HWND hwnd)
{
   TCHAR    szTimePrefix[5];
   static BOOL fMoved = FALSE;

   GetLocaleInfo(LOCALE_USER_DEFAULT, 
                 LOCALE_ITIMEMARKPOSN,
                 szTimePrefix,
                 ARRAYSIZE(szTimePrefix));

   if (!fMoved && szTimePrefix[0] == TEXT('1'))
   {
        RECT rLeftCtl, rAMPMCtl, rCurrCtl;
        HWND hwndAMPM, hwndCurr;
        int i, width;
        POINT pt;

        fMoved = TRUE;
        
        //Get rect of left most control (hours)
        GetWindowRect(GetDlgItem(hwnd, DATETIME_HOUR), &rLeftCtl);
        
        //Get rect of AM PM control
        hwndAMPM = GetDlgItem(hwnd, DATETIME_AMPM);
        GetWindowRect(hwndAMPM, &rAMPMCtl);
        width = rAMPMCtl.right - rAMPMCtl.left;
        
        //Shift all controls right by the AM PM control width
        for (i = 0; i < ARRAYSIZE(rgMoveTimeControls); i++)
        {
            hwndCurr = GetDlgItem(hwnd, rgMoveTimeControls[i]);
            GetWindowRect(hwndCurr, &rCurrCtl);
            pt.x = rCurrCtl.left;
            pt.y = rCurrCtl.top;
            ScreenToClient(hwnd, &pt);

            MoveWindow(hwndCurr, pt.x + width, 
                        pt.y,
                        rCurrCtl.right - rCurrCtl.left,
                        rCurrCtl.bottom - rCurrCtl.top,
                        TRUE);
        }
        
        //Move AM PM control left to where the hours were.
        pt.x = rLeftCtl.left;
        pt.y = rAMPMCtl.top;
        ScreenToClient(hwnd, &pt);
        MoveWindow(hwndAMPM, pt.x, 
                    pt.y,
                    rAMPMCtl.right - rAMPMCtl.left,
                    rAMPMCtl.bottom - rAMPMCtl.top,
                    TRUE);
        
   }
}


////////////////////////////////////////////////////////////////////////////
//
//  MonthUpperBound
//
////////////////////////////////////////////////////////////////////////////

int _fastcall MonthUpperBound(
    int nMonth,
    int nYear)
{
    switch (nMonth)
    {
        case ( 2 ) :
        {
            //
            //  A year is a leap year if it is divisible by 4 and is not
            //  a century year (multiple of 100) or if it is divisible by
            //  400.
            //
            return ( ((nYear % 4 == 0) &&
                      ((nYear % 100 != 0) || (nYear % 400 == 0))) ? 29 : 28 );
        }
        case ( 4 ) :
        case ( 6 ) :
        case ( 9 ) :
        case ( 11 ) :
        {
            return (30);
        }
    }

    return (31);
}

int WINAPI StrToInt(LPCTSTR lpSrc)   // atoi()
{

#define ISDIGIT(c)  ((c) >= '0' && (c) <= '9')

    int n = 0;
    BOOL bNeg = FALSE;

    if (*lpSrc == '-') {
        bNeg = TRUE;
    lpSrc++;
    }

    while (ISDIGIT(*lpSrc)) {
    n *= 10;
    n += *lpSrc - '0';
    lpSrc++;
    }
    return bNeg ? -n : n;
}

////////////////////////////////////////////////////////////////////////////
//
//  IsAMPM
//
//  True if PM.
//
////////////////////////////////////////////////////////////////////////////

BOOL IsAMPM(
    int iHour)
{
    return ((iHour >= 12) ? 1 : 0);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetDateTime
//
////////////////////////////////////////////////////////////////////////////

void GetDateTime()
{
    SYSTEMTIME SystemTime;

#if 0
    //
    //  BUGBUG: this API needs to be implemented.
    //
    SYSTEMTIME UniversalSystemTime;
    GetSystemTime(&UniversalSystemTime);

    if (!g_ptziCurrent ||
        !SystemTimeToTzSpecificLocalTime( g_ptziCurrent,
                                          &UniversalSystemTime,
                                          &SystemTime ))
#endif
    {
        GetLocalTime(&SystemTime);
    }

    wDateTime[HOUR]    = SystemTime.wHour;
    wDateTime[MINUTE]  = SystemTime.wMinute;
    wDateTime[SECOND]  = SystemTime.wSecond;
    wDateTime[MONTH]   = SystemTime.wMonth;
    wDateTime[DAY]     = SystemTime.wDay;
    wDateTime[YEAR]    = SystemTime.wYear;
    wDateTime[WEEKDAY] = SystemTime.wDayOfWeek;
}


////////////////////////////////////////////////////////////////////////////
//
//  GetTime
//
////////////////////////////////////////////////////////////////////////////

void GetTime()
{
    SYSTEMTIME SystemTime;

    GetLocalTime(&SystemTime);

    wDateTime[HOUR]   = SystemTime.wHour;
    wDateTime[MINUTE] = SystemTime.wMinute;
    wDateTime[SECOND] = SystemTime.wSecond;
}


////////////////////////////////////////////////////////////////////////////
//
//  GetDate
//
////////////////////////////////////////////////////////////////////////////

void GetDate()
{
    SYSTEMTIME SystemTime;

    GetLocalTime(&SystemTime);

    wDateTime[MONTH]   = SystemTime.wMonth;
    wDateTime[DAY]     = SystemTime.wDay;
    wDateTime[YEAR]    = SystemTime.wYear;
    wDateTime[WEEKDAY] = SystemTime.wDayOfWeek;
    fDateDirty = FALSE;
}


////////////////////////////////////////////////////////////////////////////
//
//  SetTime
//
////////////////////////////////////////////////////////////////////////////

void SetTime()
{
    SYSTEMTIME SystemTime;

    SystemTime.wHour   = wDateTime[HOUR];
    SystemTime.wMinute = wDateTime[MINUTE];
    SystemTime.wSecond = wDateTime[SECOND];

    SystemTime.wMilliseconds = 0;

    SystemTime.wMonth  = wDateTime[MONTH];
    SystemTime.wDay    = wDateTime[DAY];
    SystemTime.wYear   = wDateTime[YEAR];

    SetLocalTime(&SystemTime);
    SetLocalTime(&SystemTime);
    fDateDirty = FALSE;
}


////////////////////////////////////////////////////////////////////////////
//
//  SetDate
//
////////////////////////////////////////////////////////////////////////////

void SetDate()
{
    SYSTEMTIME SystemTime;

    SystemTime.wHour   = wDateTime[HOUR];
    SystemTime.wMinute = wDateTime[MINUTE];
    SystemTime.wSecond = wDateTime[SECOND];

    SystemTime.wMilliseconds = 0;

    SystemTime.wMonth  = wDateTime[MONTH];
    SystemTime.wDay    = wDateTime[DAY];
    SystemTime.wYear   = wDateTime[YEAR];

    SetLocalTime(&SystemTime);
    SetLocalTime(&SystemTime);
    fDateDirty = FALSE;
}


////////////////////////////////////////////////////////////////////////////
//
//  AdjustDelta
//
//  Alters the variables in wDeltaDateTime, allowing a CANCEL button
//  to perform its job by resetting the time as if it had never been
//  touched.  GetTime() & GetDate() should already have been called.
//
////////////////////////////////////////////////////////////////////////////

void AdjustDelta(
    HWND hDlg,
    int nIndex)
{
    int nDelta;

    //
    //  We dont do time this way any more.
    //
    if (nIndex <= SECOND && nIndex >= HOUR)
    {
        return;
    }

    //
    //  Get position of the buddy from either the date or the time.
    //
    nDelta = (int)SendDlgItemMessage( hDlg,
                                      nIndex <= SECOND
                                        ? DATETIME_TARROW
                                        : DATETIME_YARROW,
                                      UDM_GETPOS,
                                      0,
                                      0L );

    if ((nIndex == YEAR) && !g_bLZero[YEAR])
    {
        //
        //  Years before 80 are 2080.
        //  Range is 1980...2079.
        //
        if (nDelta < 80)
        {
            nDelta += 2000;
        }
        else
        {
            nDelta += 1900;
        }
    }

    //
    //  If our current recording of the time/date is not what we have
    //  now, do deltas.
    //
    if (wDateTime[nIndex] != nDelta)
    {
        //
        //  Previous value is current user's settings.
        //
        wPrevDateTime[nIndex] = wDateTime[nIndex] = (WORD)nDelta;
        fDateDirty = TRUE;
        
        //
        // If we are changing HMS, update the time.
        //
        if (nIndex <= SECOND)
        {
            nIndex = 0;
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  AdjustDeltaMonth
//
//  Change the month part of wDateTime
//
////////////////////////////////////////////////////////////////////////////

extern int GetDaysOfTheMonth(int iMonth);

void AdjustDeltaMonth(
    int iMonth)
{
    GetTime();

    if (wDateTime[MONTH] != iMonth)
    {
        //
        //  Make sure the current day is valid in the new month.
        //
        if (wDateTime[DAY] > (WORD)GetDaysOfTheMonth(iMonth))
        {
            wDateTime[DAY] = (WORD)GetDaysOfTheMonth(iMonth);
        }

        wPrevDateTime[MONTH] = wDateTime[MONTH] = (WORD)iMonth;
        fDateDirty = TRUE;

        g_sDateInfo[DAY].nMax = MonthUpperBound( wDateTime[MONTH],
                                                 wDateTime[YEAR] );
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  ReadShortDate
//
//  Verify that pszDate is one of MDY, DMY, or YMD.
//
////////////////////////////////////////////////////////////////////////////

int ReadShortDate(
    LPTSTR pszDate,
    BOOL *pbMonth,
    BOOL *pbDay,
    BOOL *pbYear)
{
    int i, nOrder;
    BOOL *pbOrder[3];
    TCHAR cHope[3];

    //
    //  nOrder :  0 = MDY
    //            1 = DMY
    //            2 = YMD
    //
    switch (cHope[0] = *pszDate)
    {
        case ( TEXT('M') ) :
        {
            nOrder = 0;
            pbOrder[0] = pbMonth;
            break;
        }
        case ( TEXT('d') ) :
        {
            nOrder = 1;
            pbOrder[0] = pbDay;
            break;
        }
        case ( TEXT('y') ) :
        {
            nOrder = 2;
            pbOrder[0] = pbYear;
            break;
        }
        default :
        {
            return (FALSE);
        }
    }

    //
    //  Sets element 1.
    //
    if (nOrder)         // 1 2
    {
        cHope[1] = TEXT('M');
        pbOrder[1] = pbMonth;
    }
    else                // 0
    {
        cHope[1] = TEXT('d');
        pbOrder[1] = pbDay;
    }

    //
    //  Sets element 2.
    //
    if (nOrder == 2)    // 2
    {
        cHope[2] = TEXT('d');
        pbOrder[2] = pbDay;
    }
    else                // 0 1
    {
        cHope[2] = TEXT('y');
        pbOrder[2] = pbYear;
    }

    //
    //  Verifies that pszDate is of the form MDY DMY YMD.
    //
    for (i = 0; i < 3; i++, pszDate++)
    {
        if (*pszDate != cHope[i])
        {
            return (-1 - nOrder);
        }

        if (!(pszDate = ParseDateElement(pszDate, pbOrder[i])))
        {
            return (-1 - nOrder);
        }
    }

    //
    //  Success.  Return MDY, DMY or YMD index.
    //
    return (nOrder);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetMaxCharWidth
//
//  Determine the widest digit (safety against variable pitch fonts).
//
////////////////////////////////////////////////////////////////////////////

int GetMaxCharWidth(
    HDC hDC)
{
    UINT nNumWidth[10];
    UINT i, nMaxNumWidth;

    GetCharWidth32(hDC, TEXT('0'), TEXT('9'), nNumWidth);

    for (nMaxNumWidth = 0, i = 0; i < 10; i++)
    {
        if (nNumWidth[i] > nMaxNumWidth)
        {
            nMaxNumWidth = nNumWidth[i];
        }
    }

    return (nMaxNumWidth);
}
////////////////////////////////////////////////////////////////////////////
//
//  GetMaxSubstitutedCharWidth
//
//  Determine the widest digit (safety against variable pitch fonts), but
//  do it using strings so that if number substitution is on, we will get
//  the width of the number based on what will actually be displayed
//
////////////////////////////////////////////////////////////////////////////

int GetMaxSubstitutedCharWidth(
    HDC hDC)
{
    char sz[2] = "0";
    TCHAR szAMPM[12];
    LONG i, nMaxNumWidth;
    DWORD dwWidth;
    SIZE size;
    
    for (nMaxNumWidth = 0, i = 0; i < 10; (*sz)++, i++)
    {
        if (GetTextExtentPoint32A(hDC, sz, 1, &size) && size.cx > nMaxNumWidth)
            nMaxNumWidth = size.cx;
        
    }

    if (nMaxNumWidth <= 8)
    {
        GetProfileString(szIntl, TEXT("s1159"), IntlDef.s1159, szAMPM, ARRAYSIZE(szAMPM));
        dwWidth = LOWORD(GetTextExtent(hDC, szAMPM, lstrlen(szAMPM)));
        if (dwWidth > 22)
            nMaxNumWidth = 10;
        GetProfileString(szIntl, TEXT("s2359"), IntlDef.s2359, szAMPM, ARRAYSIZE(szAMPM));
        dwWidth = LOWORD(GetTextExtent(hDC, szAMPM, lstrlen(szAMPM)));
        if (dwWidth > 22)
            nMaxNumWidth = 10;
    }
    return (nMaxNumWidth);
}


////////////////////////////////////////////////////////////////////////////
//
//  ReflectAMPM
//
//  Sets the global g_bPM and updates the control to display AM or PM.
//
////////////////////////////////////////////////////////////////////////////

void ReflectAMPM(
    HWND hDlg,
    int nNum)
{
    HWND hCtl = GetDlgItem(hDlg, DATETIME_AMPM);

    ListBox_SetTopIndex(hCtl, g_bPM);
    ListBox_SetCurSel(hCtl, (GetFocus() == hCtl) ? g_bPM : -1);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetTextExtent
//
////////////////////////////////////////////////////////////////////////////

#ifdef WIN32
DWORD GetTextExtent(
    HDC hdc,
    LPCTSTR lpsz,
    int cb)
{
    SIZE sz;

    GetTextExtentPoint(hdc, lpsz, cb, &sz);
    return ( MAKELONG((WORD)sz.cx, (WORD)sz.cy) );
}
#endif


////////////////////////////////////////////////////////////////////////////
//
//  DateTimeInit
//
//  Determine the widest digit (safety against variable pitch fonts).
//
////////////////////////////////////////////////////////////////////////////

void DateTimeInit(
    HWND hDlg,
    WORD nBaseID,
    WORD nSepID,
    LPTSTR pszSep,
    int nMaxDigitWidth,
    BOOL bDate)
{
    HWND hAMPMList;
    HWND hDay, hMonth, hYear;     // also used as hHour, hMinute, & hSecond
    HWND hOrder[5];
    HDC hDC;
    int nWidth, nHeight, X;
    DWORD dwSepExt;
    RECT Rect;
    int i;
    int nAMPMlength;

    hMonth    = GetDlgItem(hDlg, nBaseID);
    hDay      = GetDlgItem(hDlg, nBaseID + 1);
    hYear     = GetDlgItem(hDlg, nBaseID + 2);
    hOrder[1] = GetDlgItem(hDlg, nSepID);
    hOrder[3] = GetDlgItem(hDlg, nSepID + 1);

    if (bDate)
    {
        i = GetProfileInt(szIntl, TEXT("iDate"), 0);
    }
    else
    {
        if (g_b24HR = (BOOL)GetProfileInt(szIntl, TEXT("iTime"), 0))
        {
            g_sDateInfo[HOUR].nMin = 0;
            g_sDateInfo[HOUR].nMax = 23;
        }
        else
        {
            g_sDateInfo[HOUR].nMin = 1;
            g_sDateInfo[HOUR].nMax = 12;

            GetProfileString(szIntl, TEXT("s1159"), IntlDef.s1159, sz1159, ARRAYSIZE(sz1159));
            GetProfileString(szIntl, TEXT("s2359"), IntlDef.s2359, sz2359, ARRAYSIZE(sz2359));
        }
        i = 0;
    }

    switch (i)
    {
        case ( 1 ) :
        {
            hOrder[0] = hDay;
            hOrder[2] = hMonth;
            hOrder[4] = hYear;
            break;
        }
        case ( 2 ) :
        {
            hOrder[0] = hYear;
            hOrder[2] = hMonth;
            hOrder[4] = hDay;
            break;
        }
        case ( 0 ) :
        default :
        {
            hOrder[0] = hMonth;
            hOrder[2] = hDay;
            hOrder[4] = hYear;
            break;
        }
    }

    hDC = GetDC(hDlg);

    if (!bDate)
    {
        dwSepExt = GetTextExtent(hDC, sz1159, lstrlen(sz1159));
        nAMPMlength = LOWORD(GetTextExtent(hDC, sz2359, lstrlen(sz2359)));
        if (nAMPMlength < (int)LOWORD(dwSepExt))
        {
            nAMPMlength = (int)LOWORD(dwSepExt);
        }
    }

    dwSepExt = GetTextExtent(hDC, pszSep, lstrlen(pszSep));
    ReleaseDC(hDlg, hDC);

    GetWindowRect(hYear, (LPRECT)&Rect);
    ScreenToClient(hDlg, (LPPOINT)&Rect.left);
    ScreenToClient(hDlg, (LPPOINT)&Rect.right);

    nHeight = Rect.bottom - Rect.top;
    nWidth = Rect.top;

    GetWindowRect( GetDlgItem( hDlg,
                               bDate ? DATETIME_CALENDAR : DATETIME_CLOCK ),
                   (LPRECT)&Rect );
    ScreenToClient(hDlg, (LPPOINT)&Rect.left);
    ScreenToClient(hDlg, (LPPOINT)&Rect.right);

    Rect.top = nWidth;
    X = (Rect.left + Rect.right - (6 * nMaxDigitWidth) - (2 * LOWORD(dwSepExt))) / 2;

    if (bDate)
    {
        if (g_bLZero[YEAR])
        {
            X -= nMaxDigitWidth;
        }
    }
    else if (!g_b24HR)
    {
        X -= nAMPMlength / 2;
    }

    for (i = 0; i < 5; i++)
    {
        nWidth = (i % 2) ? LOWORD(dwSepExt) : 2 * nMaxDigitWidth;

        if ((hOrder[i] == hYear) && bDate && g_bLZero[YEAR])
        {
            nWidth *= 2;
        }

        //
        //  Allow for centering in edit control.
        //
        nWidth += 2;

    //  MoveWindow(hOrder[i], X, Rect.top, nWidth, nHeight, FALSE);
        X += nWidth;
    }

    hAMPMList = GetDlgItem(hDlg, DATETIME_AMPM);
    ListBox_ResetContent(hAMPMList);

    if (!bDate && !g_b24HR)
    {
        ListBox_InsertString(hAMPMList, 0, sz1159);
        ListBox_InsertString(hAMPMList, 1, sz2359);
    }

    EnableWindow(hAMPMList, !g_b24HR);

    Edit_LimitText(hYear, (bDate && g_bLZero[YEAR]) ? 4 : 2);
    Edit_LimitText(hMonth, 2);
    Edit_LimitText(hDay, 2);

    SetDlgItemText(hDlg, nSepID, pszSep);
    SetDlgItemText(hDlg, nSepID + 1, pszSep);
}


////////////////////////////////////////////////////////////////////////////
//
//  myitoa
//
////////////////////////////////////////////////////////////////////////////

void myitoa(
    int intValue,
    LPTSTR lpStr)
{
    LPTSTR lpString;
    TCHAR c;

    //
    //  lpString points to 1st char.
    //
    lpString = lpStr;

    do
    {
        *lpStr++ = (TCHAR)(intValue % 10 + TEXT('0'));
    } while ((intValue /= 10) > 0);

    //
    //  lpStr points to last char.
    //
    *lpStr-- = TEXT('\0');

    //
    //  Now reverse the string.
    //
    while (lpString < lpStr)
    {
      c = *lpString;
      *(lpString++) = *lpStr;
      *(lpStr--) = c;
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  UpdateItem
//
//  This displays the information in the control from the array
//  of global values. Also selects the control. Also adds leading 0's
//  as well as rounding years to 2 digits and 24 or AM/PM hours.
//
////////////////////////////////////////////////////////////////////////////

void UpdateItem(
    HWND hDlg,
    int i)
{
    TCHAR szNum[5];
    int nNum = g_Modified ? wPrevDateTime[i] : wDateTime[i];

    //
    //  Use internal time.
    //
    if (i <= SECOND && i >= HOUR)
    {
        nNum = g_Time[i];

        //
        //  Do not paint un-necessarily.
        //
        if ((nNum == g_LastTime[i]) && (nNum >= 10))
        {
            return;
        }

        g_LastTime[i] = nNum;

        if (i == HOUR)
        {
            if (IsAMPM(nNum))
            {
                g_bPM = TRUE;
            }
            ReflectAMPM(hDlg, nNum);
        }
    }

    if (i == YEAR)
    {
        //
        //  Round the years to last 2 digits.
        //
        if (!g_bLZero[i])
        {
            nNum %= 100;
        }
    }
    else if ((i == HOUR) && !g_b24HR)
    {
        //
        //  nNum came from our internal date time.
        //  Remove 12 hours if not 24hour.
        //
        if (g_bPM)
        {
            nNum %= 12;
        }

        //
        //  00 hours is actually 12AM.
        //
        if (!nNum)
        {
            nNum = 12;
        }
    }

    //
    //  See if we need leading zeros.
    //  We only deal with 2 character numbers MAX.
    //
    if ((nNum < 10) && (g_bLZero[i] || (i == YEAR)))
    {
        szNum[0] = TEXT('0');
        szNum[1] = (TCHAR)(TEXT('0') + nNum);
        szNum[2] = TEXT('\0');
    }
    else
    {
        myitoa(nNum, szNum);
    }

    //
    //  Reflect the value in the appropriate control.
    //
    SetDlgItemText(hDlg, DATETIME_HOUR + i, szNum);

    //
    //  Select the field too.
    //
    SendDlgItemMessage(hDlg, DATETIME_HOUR + i, EM_SETSEL, 0, MAKELONG(0, 32767));

    //
    //  If we changed year or month, then we may have altered the leap year
    //  state.
    //
    if (i == MONTH || i == YEAR)
    {
        g_sDateInfo[DAY].nMax = MonthUpperBound( wDateTime[MONTH],
                                                 wDateTime[YEAR] );
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  _ShowTZ
//
////////////////////////////////////////////////////////////////////////////

TCHAR c_szFirstBootTZ[] = TEXT("!!!First Boot!!!");

void _ShowTZ(
    HWND hDlg)
{
    HWND ctl = GetDlgItem(hDlg, DATETIME_CURTZ);
    TIME_ZONE_INFORMATION info;
    TCHAR final[64 + TZNAME_SIZE];
    TCHAR name[TZNAME_SIZE];
    DWORD TimeZoneId;

    if (g_bFirstBoot)
    {
        ShowWindow(ctl, SW_HIDE);
    }
    else
    {
        TimeZoneId = GetTimeZoneInformation(&info);
#ifdef UNICODE
        lstrcpy( name,
                 (TimeZoneId == TIME_ZONE_ID_STANDARD)
                   ? info.StandardName
                   : info.DaylightName );
#else
        WideStrToStr(name,  (TimeZoneId == TIME_ZONE_ID_STANDARD)
                   ? info.StandardName
                   : info.DaylightName );
#endif //UNICODE

        //
        //  Display nothing if it is our special 1st boot marker.
        //
        if (*name && (lstrcmpi(name, c_szFirstBootTZ) != 0))
        {
            static TCHAR format[128] = TEXT("");

            if (!*format)
            {
                GetWindowText( ctl,
                               format,
                               ARRAYSIZE(format) );
            }
            wsprintf(final, format, name);
        }
        else
        {
            *final = 0;
        }

        SetWindowText(ctl, final);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  UnhookTime
//
//  To stop the clock calling us back all the time (around exit).
//
////////////////////////////////////////////////////////////////////////////

void UnhookTimer(
    HWND hDlg)
{
    SendDlgItemMessage(hDlg, DATETIME_CLOCK, CLM_TIMEHWND, CLF_SETHWND, 0);
}


////////////////////////////////////////////////////////////////////////////
//
//  TimeProvider
//
//  Called by the clock to find out what time it is.
//
////////////////////////////////////////////////////////////////////////////

void TimeProvider(
    LPSYSTEMTIME lpSystemTime,
    HWND hDlg)
{
    short wTemp[7];

    //
    //  If the user has modified the time, the clock should
    //  display the edit controls, otherwise its just the SystemTime.
    //
    if (g_Modified)
    {
        lpSystemTime->wHour   = (WORD)g_Time[HOUR];
        lpSystemTime->wMinute = (WORD)g_Time[MINUTE];
        lpSystemTime->wSecond = (WORD)g_Time[SECOND];
    }
    else
    {
#ifdef WIN32
        GetLocalTime(lpSystemTime);
#else
        GetTime();
        if (wDateTime[HOUR] >= 0 && wDateTime[HOUR] <= 24)
        {
            lpSystemTime->wHour = wDateTime[HOUR];
        }
        lpSystemTime->wMinute = wDateTime[MINUTE];
        lpSystemTime->wSecond = wDateTime[SECOND];

#endif
        //
        //  Copy the time and display it for us too.
        //
        g_bPM = IsAMPM(lpSystemTime->wHour);
        g_Time[HOUR]   = lpSystemTime->wHour;
        g_Time[MINUTE] = lpSystemTime->wMinute;
        g_Time[SECOND] = lpSystemTime->wSecond;

        //
        //  Check for date rollover.
        //
        if (!fDateDirty)
        {
            wTemp[DAY]   = wDateTime[DAY];
            wTemp[MONTH] = wDateTime[MONTH];
            wTemp[YEAR]  = wDateTime[YEAR];

            GetDate();

            if ((wDateTime[DAY]   != wTemp[DAY])   ||
                (wDateTime[MONTH] != wTemp[MONTH]) ||
                (wDateTime[YEAR]  != wTemp[YEAR]))
            {
                InvalidateRect(GetDlgItem(hDlg, DATETIME_CALENDAR), NULL, TRUE);

                if (wDateTime[MONTH] != wTemp[MONTH])
                {
                    ComboBox_SetCurSel( GetDlgItem(hDlg, DATETIME_MONTHNAME),
                                        wDateTime[MONTH] - 1 );
                }

                if (wDateTime[YEAR] != wTemp[YEAR])
                {
                    UpdateItem(hDlg, YEAR);
                }

                _ShowTZ(hDlg);
            }
        }
        
        UpdateItem(hDlg, HOUR);
        UpdateItem(hDlg, MINUTE);
        UpdateItem(hDlg, SECOND);
        ReflectAMPM(hDlg, g_Time[HOUR]);
    }
}

////////////////////////////////////////////////////////////////////////////
//
//  bSupportedCalendar
//
//  Returns True if the current calendar is not Hijri nor Hebrew
//
//  Otherwise it returns FALSE.
//
////////////////////////////////////////////////////////////////////////////

BOOL bSupportedCalendar()
{
    TCHAR tchCalendar[32];
    CALTYPE defCalendar = CAL_GREGORIAN;

    if (GetLocaleInfo(LOCALE_USER_DEFAULT,
                      LOCALE_ICALENDARTYPE,
                      tchCalendar,
                      ARRAYSIZE(tchCalendar)))
    {
        defCalendar = StrToInt(tchCalendar);
    }

    return (!(defCalendar == CAL_HIJRI || defCalendar == CAL_HEBREW));
}

////////////////////////////////////////////////////////////////////////////
//
//  InitDateTimeDlg
//
//  Called to init the dialog.
//
////////////////////////////////////////////////////////////////////////////

void InitDateTimeDlg(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    int nMaxDigitWidth;
    int i;
    TCHAR szNum[5];
    TCHAR szMonth[64];
    TCHAR szShortDate[12];
    HDC hDC;
    HFONT hFont;
    HWND hwndCB;
    CALID calId;
    static int nInc[] = { 1, 5, 5, 1, 1, 5 };

    HWND hwndScroll;
    UDACCEL udAccel[2];
    HWND hwndTBorder;

    HCURSOR oldcursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

    LCID lcid = LOCALE_USER_DEFAULT;

    InitCommonControls();

    //
    //  Sets the Leading zero status of the 6 controls.
    //
    g_bLZero[HOUR]   = g_bLZero[MONTH]  = g_bLZero[DAY]  = FALSE;
    g_bLZero[MINUTE] = g_bLZero[SECOND] = g_bLZero[YEAR] = TRUE;

    hDC = GetDC(hDlg);

    if (hFont = (HFONT)SendMessage(hDlg, WM_GETFONT, 0, 0L))
    {
        hFont = SelectObject( hDC, hFont );
    }

    if (hFont)
    {
        SelectObject(hDC, hFont);
    }

    AdjustAMPMPosition(hDlg);

    nMaxDigitWidth = GetMaxCharWidth(hDC);
    ReleaseDC(hDlg, hDC);

    g_bLZero[HOUR] = GetProfileInt(szIntl, TEXT("iTLZero"), 0);
    GetProfileString(szIntl, TEXT("sShortDate"), IntlDef.sShortDate, szShortDate, ARRAYSIZE(szShortDate));
    ReadShortDate(szShortDate, g_bLZero + MONTH, g_bLZero + DAY, g_bLZero + YEAR);

    g_bLZero[YEAR] = TRUE;      //we always want the year to be 4 digits (this will suck in late 9999)
    
    //
    //  Setup the TIME stuff.
    //
    GetTime();

    g_Time[HOUR]   = wDateTime[HOUR];
    g_Time[MINUTE] = wDateTime[MINUTE];
    g_Time[SECOND] = wDateTime[SECOND];

    GetProfileString(szIntl, TEXT("sTime"), IntlDef.sTime, szNum, 3);
    DateTimeInit(hDlg, DATETIME_HOUR, DATETIME_TSEP1, szNum, nMaxDigitWidth, FALSE);

    //
    //  Force all entries to be re-drawn,
    //
    g_LastTime[HOUR] = g_LastTime[MINUTE] = g_LastTime[SECOND] = -1;
    UpdateItem(hDlg, HOUR);
    UpdateItem(hDlg, MINUTE);
    UpdateItem(hDlg, SECOND);
    ReflectAMPM(hDlg, wDateTime[HOUR]);

    //
    //  Setup the Date stuff.
    //
    GetDate();

    g_sDateInfo[DAY].nMax = MonthUpperBound(wDateTime[MONTH], wDateTime[YEAR]);

    if (!g_bLZero[YEAR])
    {
        wDateTime[YEAR] %= 100;
        g_sDateInfo[YEAR].nMax = 99;
        g_sDateInfo[YEAR].nMin = 0;
    }
    else
    {
        g_sDateInfo[YEAR].nMax = 2099;
        g_sDateInfo[YEAR].nMin = 1980;
    }

    for (i = MONTH; i <= YEAR; i++)
    {
        wPrevDateTime[i] = -1;
    }

    //
    //  Get the month names. And select this month.
    //
    hwndCB = GetDlgItem(hDlg, DATETIME_MONTHNAME);
    ComboBox_ResetContent(hwndCB);
    //
    // If the current calendar is Hijri or Hebrew then use the Gregorian one.
    //
    if (!bSupportedCalendar())
        lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);

    GetLocaleInfo(lcid, LOCALE_ICALENDARTYPE, szMonth, ARRAYSIZE(szMonth));
    calId = (CALID)StrToInt(szMonth);

    for (i = 0; i < 12; i++)
    {
#ifdef WINNT
        GetCalendarInfo(lcid, calId, CAL_SMONTHNAME1 + i, szMonth, ARRAYSIZE(szMonth), NULL);
#else
        GetLocaleInfo( LOCALE_USER_DEFAULT,
                       LOCALE_SMONTHNAME1 + i,
                       szMonth,
                       sizeof(szMonth) );

        if (*szMonth && !IsDBCSLeadByte(*szMonth))
        {
            *szMonth = (TCHAR)CharUpper((LPTSTR)(DWORD)(TBYTE)*szMonth);
        }

#endif
        ComboBox_AddString(hwndCB, szMonth);
    }

    ComboBox_SetCurSel(hwndCB, wDateTime[MONTH] - 1);

    //
    //  Set the default modifier for the Year Updown arrows.
    //
    wParam -= DATETIME_HOUR;
    hwndScroll = GetDlgItem(hDlg, DATETIME_YARROW);
    SendMessage( hwndScroll,
                 UDM_SETRANGE,
                 0,
                 MAKELPARAM(g_sDateInfo[YEAR].nMax, g_sDateInfo[YEAR].nMin) );

    udAccel[0].nSec = 0;
    udAccel[0].nInc = 1;
    udAccel[1].nSec = 2;
    udAccel[1].nInc = nInc[YEAR];

    SendMessage(hwndScroll, UDM_SETACCEL, 2, (LPARAM)(LPUDACCEL)udAccel);
    SendMessage(hwndScroll, UDM_SETBUDDY, (WPARAM)GetDlgItem(hDlg, DATETIME_YEAR), 0L);

    //
    //  Make the 'well' for the digits appear.
    //
    hwndTBorder = GetDlgItem(hDlg, DATETIME_TBORDER);
    SetWindowLong( hwndTBorder,
                   GWL_EXSTYLE,
                   GetWindowLong(hwndTBorder, GWL_EXSTYLE) | WS_EX_CLIENTEDGE );

    //
    //  Display the border right now.
    //
    SetWindowPos( hwndTBorder,
                  NULL,
                  0, 0, 0, 0,
                  SWP_NOMOVE | SWP_NOSIZE | SWP_DRAWFRAME | SWP_SHOWWINDOW );

    //
    //  Display month->year.
    //
    for (i = MONTH; i <= YEAR; i++)
    {
        if ((wDateTime[i] != wPrevDateTime[i]) &&
            (GetFocus() != GetDlgItem(hDlg, DATETIME_HOUR + i)))
        {
            //
            //  Update previous date-time.
            //
            wPrevDateTime[i] = wDateTime[i];

            if (i == YEAR)
            {
                UpdateItem(hDlg, i);
            }
        }
    }

    g_Modified = FALSE;

    //
    //  Tell the clock that we have a time provider - must be done last.
    //
    SendDlgItemMessage( hDlg,
                        DATETIME_CLOCK,
                        CLM_TIMEHWND,
                        CLF_SETHWND,
                        (LPARAM)(LPINT)hDlg );

    SetCursor(oldcursor);
}


////////////////////////////////////////////////////////////////////////////
//
//  CheckNum
//
////////////////////////////////////////////////////////////////////////////

LRESULT CheckNum(
    HWND hDlg,
    UINT nScrollID,
    HWND hCtl)
{
    static int cReenter = 0;

    LRESULT lRet;

    //
    //  If this is an illegal value, (but not blank), then kill the last char
    //  that was entered.
    //
    lRet = SendDlgItemMessage(hDlg, nScrollID, UDM_GETPOS, 0, 0L);

    //
    //  Guard against re-entrance.
    //
    ++cReenter;

    if (cReenter <= 4)
    {
        SendMessage( hCtl,
                     HIWORD(lRet) && GetWindowTextLength(hCtl)
                         ? EM_UNDO
                         : EM_EMPTYUNDOBUFFER,
                     0,
                     0L );
    }

    --cReenter;

    return (lRet);
}


////////////////////////////////////////////////////////////////////////////
//
//  DateTimeDlgProc
//
//  Main dialog proc.
//
////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK DateTimeDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    int i;

    switch (uMsg)
    {
        case ( WM_INITDIALOG ) :
        {
            InitDateTimeDlg(hDlg, uMsg, wParam, lParam);
            g_PrevIMCForDateField =
                ImmAssociateContext( GetDlgItem(hDlg, DATETIME_YEAR),
                                     0 );
            break;
        }
        case ( WM_DESTROY ) :
        {
            if (g_PrevIMCForDateField)
            {
                ImmAssociateContext( GetDlgItem(hDlg, DATETIME_YEAR),
                                     g_PrevIMCForDateField );
            }
            UnhookTimer(hDlg);
            break;
        }
#ifdef WIN32
        case ( WM_CTLCOLORSTATIC ) :
#endif
        case ( WM_CTLCOLOR ) :
        {
            //
            //  Set the background color of the time controls to the the
            //  color of the edit controls.
            //
            if ((GET_WM_CTLCOLOR_HWND(wParam, lParam, uMsg) ==
                 GetDlgItem(hDlg, DATETIME_TSEP1)) ||
                (GET_WM_CTLCOLOR_HWND(wParam, lParam, uMsg) ==
                 GetDlgItem(hDlg, DATETIME_TSEP2)) ||
                (GET_WM_CTLCOLOR_HWND(wParam, lParam, uMsg) ==
                 GetDlgItem(hDlg, DATETIME_TBORDER)))
            {
#ifndef WIN32
                //
                //  Make the statics the color of the edits.
                //
                lParam = GET_WM_CTLCOLOR_MPS(
                            GET_WM_CTLCOLOR_HDC(wParam, lParam, uMsg),
                            GET_WM_CTLCOLOR_HWND(wParam, lParam, uMsg),
                            CTLCOLOR_EDIT );

                return ((INT_PTR)DefWindowProc(hDlg, uMsg, wParam, lParam));
#else
                return ((INT_PTR)DefWindowProc(hDlg, WM_CTLCOLOREDIT, wParam, lParam));
#endif
            }
            return (0);
            break;
        }
        case ( WM_NOTIFY ) :
        {
            //
            //  Property sheet handler stuff.
            //
            switch (((NMHDR *)lParam)->code)
            {
                case ( PSN_SETACTIVE ) :
                {
                    _ShowTZ(hDlg);
                    break;
                }
                case ( PSN_RESET ) :
                {
                    UnhookTimer(hDlg);

                    SetFocus(GetDlgItem(hDlg, (int)wParam));

                    GetDate();
                    GetTime();
                    break;
                }
                case ( PSN_APPLY ) :
                {
                    wDateTime[MINUTE] = (WORD)g_Time[MINUTE];
                    wDateTime[SECOND] = (WORD)g_Time[SECOND];

                    if (g_b24HR)
                    {
                        wDateTime[HOUR] = (WORD)g_Time[HOUR];
                    }
                    else
                    {
                        wDateTime[HOUR] = g_Time[HOUR] % 12;

                        if (g_bPM)
                        {
                            wDateTime[HOUR] += 12;
                        }
                    }
                    
                    g_WasModified = g_Modified;
                    SetTime();

                    g_LastTime[HOUR] = g_LastTime[MINUTE] = g_LastTime[SECOND] = -1;

                    for (i = MONTH; i <= YEAR; i++)
                    {
                        wPrevDateTime[i] = -1;
                    }

                    g_Modified = FALSE;

                    wPrevDateTime[HOUR]    = wDateTime[HOUR];
                    wPrevDateTime[MINUTE]  = wDateTime[MINUTE];
                    wPrevDateTime[SECOND]  = wDateTime[SECOND];
                    wPrevDateTime[MONTH]   = wDateTime[MONTH];
                    wPrevDateTime[DAY]     = wDateTime[DAY];
                    wPrevDateTime[YEAR]    = wDateTime[YEAR];
                    wPrevDateTime[WEEKDAY] = wDateTime[WEEKDAY];

                    //
                    //  We handled it - no repaint.
                    //
                    return (TRUE);
                }
            }
            break;
        }
        case ( WM_VSCROLL ) :
        {
            switch (GET_WM_VSCROLL_CODE(wParam, lParam))
            {
                case ( SB_THUMBPOSITION ) :
                {
                    SYSTEMTIME SystemTime;
                    HWND hBuddy = (HWND)SendMessage(
                                           GET_WM_VSCROLL_HWND(wParam, lParam),
                                           UDM_GETBUDDY,
                                           0,
                                           0L );

                    if (hBuddy == GetDlgItem(hDlg, DATETIME_HOUR))
                    {
                        g_Time[HOUR] = GET_WM_VSCROLL_POS(wParam, lParam);
                    }
                    else if (hBuddy == GetDlgItem(hDlg, DATETIME_MINUTE))
                    {
                        g_Time[MINUTE] = GET_WM_VSCROLL_POS(wParam, lParam);
                    }
                    else if (hBuddy == GetDlgItem(hDlg, DATETIME_SECOND))
                    {
                        g_Time[SECOND] = GET_WM_VSCROLL_POS(wParam, lParam);
                    }
                //  else if (hBuddy == GetDlgItem(hDlg, DATETIME_AMPM))

                    if (hBuddy != GetDlgItem(hDlg, DATETIME_YEAR))  
                        g_Modified = TRUE;

                    //
                    //  Light the apply now button.
                    //
                    PropSheet_Changed(GetParent(hDlg), hDlg);

                    //
                    //  Force the clock to reflect this setting.
                    //
                    TimeProvider(&SystemTime, hDlg);

                    SendDlgItemMessage( hDlg,
                                        DATETIME_CLOCK,
                                        CLM_UPDATETIME,
                                        CLF_SETTIME,
                                        (LPARAM)(LPSYSTEMTIME)&SystemTime );

                    //
                    //  Fall thru to update the year...
                    //
                }
                case ( SB_ENDSCROLL ) :
                {
                    //
                    //  If this is the year, have the calendar repaint.
                    //
                    if ((HWND)SendMessage( GET_WM_VSCROLL_HWND(wParam, lParam),
                                           UDM_GETBUDDY,
                                           0,
                                           0L ) == GetDlgItem(hDlg, DATETIME_YEAR))
                    {
                        //
                        //  Have it update the information.
                        //
                        GetTime();
                        AdjustDelta(hDlg, YEAR);
                        UpdateItem(hDlg, YEAR);

                        InvalidateRect( GetDlgItem(hDlg, DATETIME_CALENDAR),
                                        NULL,
                                        TRUE );
                    }

                    break;
                }
            }
            break;
        }
        case ( CLM_UPDATETIME ) :
        {
            //
            //  The clock updating/reflecting the time.
            //
            switch (wParam)
            {
                case ( CLF_SETTIME ) :
                {
                    //
                    //  Clock telling us what the time is.
                    //
                    g_Modified = TRUE;
                    g_Time[HOUR] = ((LPSYSTEMTIME)lParam)->wHour;
                    g_Time[MINUTE] = ((LPSYSTEMTIME)lParam)->wMinute;
                    g_Time[SECOND] = ((LPSYSTEMTIME)lParam)->wSecond;
                    g_bPM = IsAMPM(g_Time[HOUR]);
                    break;
                }
                case ( CLF_GETTIME ) :
                {
                    //
                    //  We tell the clock what time it is.
                    //
                    TimeProvider((LPSYSTEMTIME)lParam, hDlg);
                    break;
                }
            }
            break;
        }
        case ( WM_COMMAND ) :
        {
            //
            //  Command processing.
            //
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
                case ( DATETIME_AMPM ) :
                {
                    //
                    //  Deals with the AMPM control.
                    //
                    UDACCEL udAccel;
                    HWND hwndScroll = GetDlgItem(hDlg, DATETIME_TARROW);
                    HWND hwndThisCtl = GET_WM_COMMAND_HWND(wParam, lParam);

                    //
                    //  We only care if we get/loose the focus.
                    //
                    switch (GET_WM_COMMAND_CMD(wParam, lParam))
                    {
                        case ( LBN_SETFOCUS ) :
                        {
                            //
                            //  If we get the focus, then the UD control
                            //  should deal with the AMPM.
                            //
                            //  Select the visible entry.
                            //
                            ReflectAMPM(hDlg, wDateTime[HOUR]);

                            //
                            //  Tell the UD control how to manipulate AM/PM.
                            //
                            SendMessage( hwndScroll,
                                         UDM_SETRANGE,
                                         0,
                                         MAKELPARAM(1, 0) );
                            udAccel.nSec = 0;
                            udAccel.nInc = 1;
                            SendMessage( hwndScroll,
                                         UDM_SETACCEL,
                                         1,
                                         (LPARAM)(LPUDACCEL)&udAccel );
                            SendMessage( hwndScroll,
                                         UDM_SETBUDDY,
                                         (WPARAM)hwndThisCtl,
                                         0 );
                            break;
                        }
                        case ( LBN_KILLFOCUS ) :
                        {
                            //
                            //  When we loose focus, the g_bPM flag is updated.
                            //
                            //  Remove selection from the AM/PM.
                            //
                            ListBox_SetCurSel(hwndThisCtl, -1);

                            if ((HWND)SendMessage( hwndScroll,
                                                   UDM_GETBUDDY,
                                                   0,
                                                   0 ) == hwndThisCtl)
                            {
                                SendMessage(hwndScroll, UDM_SETBUDDY, 0, 0);
                            }

                            break;
                        }
                        case ( LBN_SELCHANGE ) :
                        {
                            if ((g_Modified == FALSE) &&
                                (g_bPM == (BOOL)ListBox_GetTopIndex(hwndThisCtl)))
                            {
                                break;
                            }

                            //
                            //  Find the visible entry.
                            //
                            g_Modified = TRUE;

                            //
                            //  Light the apply now button.
                            //
                            PropSheet_Changed(GetParent(hDlg), hDlg);
                            g_bPM = (BOOL)ListBox_GetTopIndex(hwndThisCtl);
                            break;
                        }
                    }
                    break;
                }
                case ( DATETIME_HOUR ) :
                case ( DATETIME_MINUTE ) :
                case ( DATETIME_SECOND ) :
                {
                    switch (GET_WM_COMMAND_CMD(wParam, lParam))
                    {
                        case ( EN_CHANGE ) :
                        {
                            SYSTEMTIME SystemTime;

                            g_Modified = TRUE;

                            //
                            //  Light the apply now button.
                            //
                            PropSheet_Changed(GetParent(hDlg), hDlg);

                            //
                            //  Work out what the change was too.
                            //
                            g_Time[GET_WM_COMMAND_ID(wParam, lParam) -
                                   DATETIME_HOUR] =
                              (int)SendDlgItemMessage( hDlg,
                                                       DATETIME_TARROW,
                                                       UDM_GETPOS,
                                                       0,
                                                       0 );

                            //
                            //  Force the clock to reflect this setting.
                            //
                            TimeProvider(&SystemTime, hDlg);
                            SendDlgItemMessage( hDlg,
                                                DATETIME_CLOCK,
                                                CLM_UPDATETIME,
                                                0,
                                                (LPARAM)(LPSYSTEMTIME)&SystemTime );
                            break;
                        }
                    }

                    //  fall thru...
                }
                case ( DATETIME_MONTH ) :
                case ( DATETIME_YEAR ) :
                case ( DATETIME_DAY ) :
                {
                    switch (GET_WM_COMMAND_CMD(wParam, lParam))
                    {
                        case ( EN_CHANGE ) :
                        {
                            CheckNum( hDlg,
                                      GET_WM_COMMAND_ID(wParam, lParam) <= DATETIME_SECOND
                                          ? DATETIME_TARROW
                                          : DATETIME_YARROW,
                                      GET_WM_COMMAND_HWND(wParam, lParam) );

                            // Changing the year may alter the number of days in February.
                            // Yes this is a hack, but this entire applet is a giant
                            // broken hack and I want to change it as little as possible.
                            if (GET_WM_COMMAND_ID(wParam, lParam) == DATETIME_YEAR && wDateTime[MONTH] == 2)
                            {
                                g_sDateInfo[DAY].nMax = MonthUpperBound( wDateTime[MONTH],
                                                                         wDateTime[YEAR] );
                                if (wDateTime[DAY] > g_sDateInfo[DAY].nMax)
                                {
                                    wDateTime[DAY] = (WORD)g_sDateInfo[DAY].nMax;
                                    fDateDirty = TRUE;
                                }
                                InvalidateRect( GetDlgItem(hDlg, DATETIME_CALENDAR),
                                                NULL,
                                                TRUE );
                            }
                            break;
                        }
                        case ( EN_SETFOCUS ) :
                        {
                            UINT id = GET_WM_COMMAND_ID(wParam, lParam) - DATETIME_HOUR;

                            if (id <= SECOND)
                            {
                                UDACCEL udAccel[2];
                                static int nInc[] = { 1, 5, 5, 1, 1, 5 };
                                HWND hwndScroll = GetDlgItem(hDlg, DATETIME_TARROW);

                                SendMessage( hwndScroll,
                                             UDM_SETRANGE,
                                             0,
                                             MAKELPARAM( g_sDateInfo[id].nMax,
                                                         g_sDateInfo[id].nMin) );
                                udAccel[0].nSec = 0;
                                udAccel[0].nInc = 1;
                                udAccel[1].nSec = 2;
                                udAccel[1].nInc = nInc[id];
                                SendMessage( hwndScroll,
                                             UDM_SETACCEL,
                                             2,
                                             (LPARAM)(LPUDACCEL)udAccel );

                                //
                                //  Set the UD to update this control.
                                //
                                SendMessage( hwndScroll,
                                             UDM_SETBUDDY,
                                             (WPARAM)GET_WM_COMMAND_HWND(wParam,
                                                                         lParam),
                                             0 );
                            }
                            break;
                        }
                        case ( EN_KILLFOCUS ) :
                        {
                            HWND hwndScroll;

                            //
                            //  Gets in range HMS MDY.
                            //
                            UINT id = GET_WM_COMMAND_ID(wParam, lParam) - DATETIME_HOUR;

                            AdjustDelta(hDlg, id);
                            UpdateItem(hDlg, id);

                            hwndScroll = GetDlgItem(hDlg, id <= SECOND ? DATETIME_TARROW : DATETIME_YARROW);

                            //
                            //  Remove the buddy from this control.
                            //  Zero happens to be HOUR.
                            //
                            if ((id <= SECOND) &&
                                (HWND)SendMessage( hwndScroll,
                                                   UDM_GETBUDDY,
                                                   0,
                                                   0 ) == GET_WM_COMMAND_HWND(wParam, lParam))
                            {
                                SendMessage(hwndScroll, UDM_SETBUDDY, 0, 0);
                            }

                            //
                            //  If control is YEAR.
                            //
                            if (id == (DATETIME_YEAR - DATETIME_HOUR))
                            {
                                InvalidateRect( GetDlgItem(hDlg, DATETIME_CALENDAR),
                                                NULL,
                                                TRUE );
                            }

                            break;
                        }
                        default :
                        {
                            break;
                        }
                    }
                    break;
                }
                case ( DATETIME_MONTHNAME ) :
                {
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == CBN_SELCHANGE)
                    {
                        int nIndex = 1 + (int)ComboBox_GetCurSel(
                                              GetDlgItem( hDlg,
                                                          DATETIME_MONTHNAME ));

                        if (wDateTime[MONTH] != nIndex)
                        {
                            AdjustDeltaMonth(nIndex);
                            InvalidateRect( GetDlgItem(hDlg, DATETIME_CALENDAR),
                                            NULL,
                                            TRUE );
                            PropSheet_Changed(GetParent(hDlg), hDlg);
                        }
                    }
                    break;
                }
                case ( DATETIME_CALENDAR ) :
                {
                    //
                    //  If the calendar sent us a change, we will assume
                    //  that it is to allow the apply now to work.
                    //
                    PropSheet_Changed(GetParent(hDlg), hDlg);
                    break;
                }
            }
            break;
        }
        case ( WM_WININICHANGE ) :
        {
            //
            //  Reinitialize if there is a time format change.
            //
            InitDateTimeDlg(hDlg, uMsg, wParam, lParam);
            InvalidateRect(GetDlgItem(hDlg, DATETIME_CALENDAR), NULL, TRUE);
            break;
        }
        case ( WM_TIMECHANGE ) :
        {
            //
            //  Forward time change messages to the clock control.
            //
            SendDlgItemMessage( hDlg,
                                DATETIME_CLOCK,
                                WM_TIMECHANGE,
                                wParam,
                                lParam );

            break;
        }
        case ( WM_HELP ) :             // F1
        {
            WinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle,
                     NULL,
                     HELP_WM_HELP,
                     (DWORD_PTR)(LPTSTR)aDateTimeHelpIds );
            break;
        }
        case ( WM_CONTEXTMENU ) :      // right mouse click
        {
            WinHelp( (HWND)wParam,
                     NULL,
                     HELP_CONTEXTMENU,
                     (DWORD_PTR)(LPTSTR)aDateTimeHelpIds );
            break;
        }
        default :
        {
            return (FALSE);
        }
    }
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  SetZoneState
//
//  Sets the display state of a time zone in the map control.
//
////////////////////////////////////////////////////////////////////////////

void SetZoneState(
    HWND map,
    PTZINFO zone,
    BOOL highlight)
{
    if (zone)
    {
        if (zone->SeaIndex >= 0)
        {
            MapControlSetSeaRegionHighlight( map,
                                             zone->SeaIndex,
                                             highlight,
                                             zone->MapLeft,
                                             zone->MapWidth );
        }

        if (zone->LandIndex >= 0)
        {
            MapControlSetLandRegionHighlight( map,
                                              zone->LandIndex,
                                              highlight,
                                              zone->MapLeft,
                                              zone->MapWidth );
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  SetZoneFamilyState
//
//  Sets the display state of a time zone family in the map control.
//
////////////////////////////////////////////////////////////////////////////

void SetZoneFamilyState(
    HWND map,
    PTZINFO family,
    BOOL highlight)
{
    if (family)
    {
        PTZINFO zone = family;

        do
        {
            SetZoneState(map, zone, highlight);
            zone = zone->next;
        }
        while(zone && (zone != family));
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  ParseMapInfo
//
//  Parses the color table information about the world bitmap we display.
//
//  Expected format: "sea,land"
//    where sea and land are color table indices or -1.
//
////////////////////////////////////////////////////////////////////////////

void ParseMapInfo(
    PTZINFO zone,
    const TCHAR *text)
{
    const TCHAR *p = text;

    zone->SeaIndex = zone->LandIndex = -1;

    if (*p)
    {
        if (*p != TEXT('-'))
        {
            zone->SeaIndex = 0;

            while (*p && (*p != TEXT(',')))
            {
                zone->SeaIndex = (10 * zone->SeaIndex) + (*p - TEXT('0'));
                p++;
            }
        }
        else
        {
            do
            {
                p++;
            } while (*p && (*p != TEXT(',')));
        }

        if (*p == TEXT(','))
        {
            p++;
        }

        if (*p)
        {
            if (*p != TEXT('-'))
            {
                zone->LandIndex = 0;

                while (*p)
                {
                    zone->LandIndex = (10 * zone->LandIndex) + (*p - TEXT('0'));
                    p++;
                }
            }
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  ReadZoneData
//
//  Reads the data for a time zone from the registry.
//
////////////////////////////////////////////////////////////////////////////

BOOL ReadZoneData(
    PTZINFO zone,
    HKEY key,
    LPCTSTR keyname)
{
    TCHAR mapinfo[16];
    DWORD len;

    len = sizeof(zone->szDisplayName);

    if (RegQueryValueEx( key,
                         c_szTZDisplayName,
                         0,
                         NULL,
                         (LPBYTE)zone->szDisplayName,
                         &len ) != ERROR_SUCCESS)
    {
        return (FALSE);
    }

    //
    //  Under NT, the keyname is the "Standard" name.  Values stored
    //  under the keyname contain the other strings and binary info
    //  related to the time zone.  Every time zone must have a standard
    //  name, therefore, we save registry space by using the Standard
    //  name as the subkey name under the "Time Zones" key.
    //
    len = sizeof(zone->szStandardName);

    if (RegQueryValueEx( key,
                         c_szTZStandardName,
                         0,
                         NULL,
                         (LPBYTE)zone->szStandardName,
                         &len ) != ERROR_SUCCESS)
    {
        //
        //  Use keyname if can't get StandardName value.
        //
        lstrcpyn( zone->szStandardName,
                  keyname,
                  sizeof(zone->szStandardName) );
    }

    len = sizeof(zone->szDaylightName);

    if (RegQueryValueEx( key,
                         c_szTZDaylightName,
                         0,
                         NULL,
                         (LPBYTE)zone->szDaylightName,
                         &len ) != ERROR_SUCCESS)
    {
        return (FALSE);
    }

    len = sizeof(zone->Bias) +
          sizeof(zone->StandardBias) +
          sizeof(zone->DaylightBias) +
          sizeof(zone->StandardDate) +
          sizeof(zone->DaylightDate);

    if (RegQueryValueEx( key,
                         c_szTZI,
                         0,
                         NULL,
                         (LPBYTE)&zone->Bias,
                         &len ) != ERROR_SUCCESS)
    {
        return (FALSE);
    }

    len = sizeof(mapinfo);

    if (RegQueryValueEx( key,
                         c_szTZMapInfo,
                         0,
                         NULL,
                         (LPBYTE)mapinfo,
                         &len ) != ERROR_SUCCESS)
    {
        *mapinfo = TEXT('\0');
    }

    ParseMapInfo(zone, mapinfo);

    //
    //  Generate phony MapLeft and MapRight until they show up in the
    //  registry.
    //
    zone->MapLeft = ((zone->Bias * ZONE_IMAGE_SCALE) / ZONE_BIAS_SCALE) +
                    ZONE_IMAGE_LEFT;

    zone->MapWidth = ZONE_IMAGE_WIDTH;

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  AddZoneToList
//
//  Inserts a new time zone into a list, sorted by bias and then name.
//
////////////////////////////////////////////////////////////////////////////

void AddZoneToList(
    PTZINFO *list,
    PTZINFO zone)
{
    if (*list)
    {
        PTZINFO curr = NULL;
        PTZINFO next = *list;

        while (next && zone->Bias <= next->Bias)
        {
            if (zone->Bias == next->Bias)
            {
                if (CompareString( GetUserDefaultLCID(),
                                   0,
                                   zone->szDisplayName,
                                   -1,
                                   next->szDisplayName,
                                   -1 ) == CSTR_LESS_THAN)
                {
                    break;
                }
            }
            curr = next;
            next = curr->next;
        }

        zone->next = next;

        if (curr)
        {
            curr->next = zone;
        }
        else
        {
            *list = zone;
        }
    }
    else
    {
        *list = zone;
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  FreeTimezoneList
//
//  Frees all time zones in the passed list, setting the head to NULL.
//
////////////////////////////////////////////////////////////////////////////

void FreeTimezoneList(
    PTZINFO *list)
{
    while (*list)
    {
        PTZINFO next = (*list)->next;

        LocalFree((HANDLE)*list);

        *list = next;
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  ReadTimezones
//
//  Reads the time zone information from the registry.
//  Returns num read, -1 on failure.
//
////////////////////////////////////////////////////////////////////////////

int ReadTimezones(
    PTZINFO *list)
{
    HKEY key = NULL;
    int count = -1;

    *list = NULL;

    if (RegOpenKey( HKEY_LOCAL_MACHINE,
                    c_szTimeZones,
                    &key ) == ERROR_SUCCESS)
    {
        TCHAR name[TZNAME_SIZE];
        PTZINFO zone = NULL;
        int i;

        count = 0;

        for (i = 0;
             RegEnumKey(key, i, name, TZNAME_SIZE) == ERROR_SUCCESS;
             i++)
        {
            HKEY subkey = NULL;

            if (!zone &&
                ((zone = (PTZINFO)LocalAlloc(LPTR, sizeof(TZINFO))) == NULL))
            {
                zone = *list;
                *list = NULL;
                count = -1;
                break;
            }

            zone->next = NULL;

            if (RegOpenKey(key, name, &subkey) == ERROR_SUCCESS)
            {
                //
                //  Each sub key name under the Time Zones key is the
                //  "Standard" name for the Time Zone.
                //
                lstrcpyn(zone->szStandardName, name, TZNAME_SIZE);

                if (ReadZoneData(zone, subkey, name))
                {
                    AddZoneToList(list, zone);
                    zone = NULL;
                    count++;
                }

                RegCloseKey(subkey);
            }
        }

        FreeTimezoneList(&zone);
        RegCloseKey(key);
    }

    return (count);
}


////////////////////////////////////////////////////////////////////////////
//
//  InitZoneMapping
//
//  Initializes map and map lookup for a specific time zone.
//
////////////////////////////////////////////////////////////////////////////

void InitZoneMapping(
    PTZINFO *lookup,
    PTZINFO list,
    HWND map)
{
    PTZINFO zone = list;    // not needed but more readable

    while (zone)
    {
        if (zone->SeaIndex >= 0)
        {
            lookup[zone->SeaIndex] = zone;
        }

        if (zone->LandIndex >= 0)
        {
            lookup[zone->LandIndex] = zone;
        }

        SetZoneState(map, zone, FALSE);
        zone = zone->next;
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  BreakZonesIntoFamilies
//
//  Breaks the passed list into many circular lists.
//  Each list consists of all time zones with a particular bias.
//  Assumes the passed list is sorted by bias.
//
////////////////////////////////////////////////////////////////////////////

void BreakZonesIntoFamilies(
    PTZINFO head)
{
    PTZINFO subhead = NULL;
    PTZINFO last = NULL;
    PTZINFO zone = head;

    while (zone)
    {
        subhead = zone;

        do
        {
            last = zone;
            zone = zone->next;
        }
        while (zone && (zone->Bias == subhead->Bias));

        last->next = subhead;
    }

    //
    //  Merge -12 and +12 zones into a single group.
    //  Assumes populated registry and depends on sort order.
    //
    if ((subhead) &&
        (subhead->Bias == BIAS_PLUS_12) &&
        (head->Bias == BIAS_MINUS_12))
    {
        PTZINFO next = head;

        do
        {
            zone = next;
            next = zone->next;
        }
        while (next != head);

        zone->next = subhead;
        last->next = head;
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  InitTimezones
//
//  Initializes time zone stuff, UI and otherwise.
//
////////////////////////////////////////////////////////////////////////////

BOOL InitTimezones(
    HWND page,
    PTZINFO *lookup)
{
    PTZINFO list = NULL;

    if ((g_nTimeZones = ReadTimezones(&list)) >= 0)
    {
        HWND combo = GetDlgItem(page, IDD_TIMEZONES);
        PTZINFO zone = list;

        SetWindowRedraw(combo, FALSE);

        while (zone)
        {
            int index = ComboBox_AddString(combo, zone->szDisplayName);

            if (index < 0)
            {
                break;
            }
            zone->ComboIndex = index;
            ComboBox_SetItemData(combo, index, (LPARAM)zone);
            zone = zone->next;
        }

        SetWindowRedraw(combo, TRUE);

        if (!zone)
        {
            InitZoneMapping(lookup, list, GetDlgItem(page, IDD_TIMEMAP));
            BreakZonesIntoFamilies(list);
            return (TRUE);
        }

        FreeTimezoneList(&list);
        ComboBox_ResetContent(combo);
    }

    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  ChangeZone
//
//  Updates the current zone, making sure new zone's family is highlighted.
//
////////////////////////////////////////////////////////////////////////////

void ChangeZone(
    HWND page,
    TZPAGE_STATE *state,
    PTZINFO zone)
{
    if (zone || state->zone)
    {
        BOOL newfamily = (!zone || !state->zone ||
                          (zone->Bias != state->zone->Bias));
        HWND map = GetDlgItem(page, IDD_TIMEMAP);
        BOOL dayval = (zone && (zone->StandardDate.wMonth != 0));

        if (newfamily && state->zone)
        {
            SetZoneFamilyState(map, state->zone, FALSE);
        }

        state->zone = zone;

        if (newfamily && state->zone)
        {
            SetZoneFamilyState(map, state->zone, TRUE);
        }

        if (newfamily)
        {
            MapControlInvalidateDirtyRegions(map);
        }

        ShowWindow(GetDlgItem(page, IDD_AUTOMAGIC), (dayval != 0 ? SW_SHOW : SW_HIDE));

        if (!state->initializing)
        {
            PropSheet_Changed(GetParent(page), page);
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  HotTrackZone
//
//  Updates the map highlighting and combo selection for a given map index.
//  Expects to be called with dups.
//
////////////////////////////////////////////////////////////////////////////

void HotTrackZone(
    HWND page,
    TZPAGE_STATE *state,
    int index)
{
    PTZINFO zone = state->lookup[index];

    if (zone && (zone != state->zone))
    {
        ComboBox_SetCurSel( GetDlgItem(page, IDD_TIMEZONES),
                            (zone ? zone->ComboIndex : -1) );
        ChangeZone(page, state, zone);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  CenterZone
//
//  Updates the map highlighting and combo selection for a given map index.
//  Expects to be called with dups.
//
////////////////////////////////////////////////////////////////////////////

void CenterZone(
    HWND page,
    TZPAGE_STATE *state,
    BOOL animate)
{
    PTZINFO zone = state->zone;

    if (zone)
    {
        HWND map = GetDlgItem(page, IDD_TIMEMAP);

        MapControlRotateTo(map, zone->MapLeft + zone->MapWidth / 2, animate);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  GetPTZ
//
//  Returns the pointer for the iItem time zone.
//  If iItem is -1 on entry, use the currently selected time zone.
//
////////////////////////////////////////////////////////////////////////////

PTZINFO GetPTZ(
    HWND hDlg,
    int iItem)
{
    HWND hCtl = GetDlgItem(hDlg, IDD_TIMEZONES);

    if (iItem == -1)
    {
        iItem = (int)ComboBox_GetCurSel(hCtl);
    }

    if (iItem < 0)
    {
        return (NULL);
    }

    return ((PTZINFO)ComboBox_GetItemData(hCtl, iItem));
}


////////////////////////////////////////////////////////////////////////////
//
//  GetAllowLocalTimeChange
//
////////////////////////////////////////////////////////////////////////////

TCHAR c_szRegPathTZControl[] = REGSTR_PATH_TIMEZONE;
TCHAR c_szRegValDisableTZUpdate[] = REGSTR_VAL_TZNOAUTOTIME;

BOOL GetAllowLocalTimeChange()
{
    //
    //  Assume allowed until we see a disallow flag.
    //
    BOOL result = TRUE;
    HKEY key;

    if (RegOpenKey( HKEY_LOCAL_MACHINE,
                    c_szRegPathTZControl,
                    &key ) == ERROR_SUCCESS)
    {
        //
        //  Assume no disallow flag until we see one.
        //
        DWORD value = 0;
        long len = sizeof(value);
        DWORD type;

        if ((RegQueryValueEx( key,
                              c_szRegValDisableTZUpdate,
                              NULL,
                              &type,
                              (LPBYTE)&value,
                              &len ) == ERROR_SUCCESS) &&
            ((type == REG_DWORD) || (type == REG_BINARY)) &&
            (len == sizeof(value)) && value)
        {
            //
            //  Okay, we have a nonzero value, it is either:
            //
            //  1) 0xFFFFFFFF
            //      this is set in an inf file for first boot to prevent
            //      the base from performing any cutovers during setup.
            //
            //  2) some other value
            //      this signifies that the user actualy disabled cutovers
            //     *return that local time changes are disabled
            //
            if (value != 0xFFFFFFFF)
            {
                result = FALSE;
            }
        }

        RegCloseKey(key);
    }

    return (result);
}


////////////////////////////////////////////////////////////////////////////
//
//  SetAllowLocalTimeChange
//
////////////////////////////////////////////////////////////////////////////

void SetAllowLocalTimeChange(
    BOOL fAllow)
{
    HKEY key = NULL;

    if (fAllow)
    {
        //
        //  Remove the disallow flag from the registry if it exists.
        //
        if (RegOpenKey( HKEY_LOCAL_MACHINE,
                        c_szRegPathTZControl,
                        &key ) == ERROR_SUCCESS)
        {
            RegDeleteValue(key, c_szRegValDisableTZUpdate);
        }
    }
    else
    {
        //
        //  Add/set the nonzero disallow flag.
        //
        if (RegCreateKey( HKEY_LOCAL_MACHINE,
                          c_szRegPathTZControl,
                          &key ) == ERROR_SUCCESS)
        {
            DWORD value = 1;

            RegSetValueEx( key,
                           (LPCTSTR)c_szRegValDisableTZUpdate,
                           0UL,
                           REG_DWORD,
                           (LPBYTE)&value,
                           sizeof(value) );
        }
    }

    if (key)
    {
        RegCloseKey(key);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  InitTimeZonePage
//
//  This function initializes everything to do with the Time Zones.
//
////////////////////////////////////////////////////////////////////////////

BOOL InitTimeZonePage(
    HWND hDlg,
    TZPAGE_STATE *state)
{
    TIME_ZONE_INFORMATION tziCurrent;
    DWORD dwTZID;
    PTZINFO ptzi;
    int j ,iCurrentTZ;
    BOOL fForceSelection = FALSE;
    TCHAR temp[TZNAME_SIZE];
    TCHAR oldTzMapName[TZNAME_SIZE], newTzMapName[TZNAME_SIZE];

    //
    //  Get the current time zone information.
    //
    dwTZID = GetTimeZoneInformation(&tziCurrent);

    LoadString(g_hInst, IDS_ISRAELTIMEZONE, oldTzMapName, TZNAME_SIZE);
    LoadString(g_hInst, IDS_JERUSALEMTIMEZONE, newTzMapName, TZNAME_SIZE);

    // this is a hack for WIn95 and NT4 to Win98/Win2k migration.  "Israel" became "Jerusalem"
#ifdef UNICODE
    if (!lstrcmpi(oldTzMapName, tziCurrent.StandardName))
    {
        lstrcpy(tziCurrent.StandardName, newTzMapName);
        fForceSelection = TRUE;
    }
#endif

    //
    //  Check for bogus time zone info.
    //
    if (dwTZID != TIME_ZONE_ID_INVALID)
    {
        //
        //  Copy the name out so we can check for first boot.
        //
#ifdef UNICODE
        lstrcpy(temp, tziCurrent.StandardName);
#else
        WideStrToStr(temp, tziCurrent.StandardName);
#endif
    }
    else
    {
        //
        //  Treat bogus time zones like first boot.
        //
        lstrcpy(temp, c_szFirstBootTZ);
    }

    if (lstrcmpi(temp, c_szFirstBootTZ) == 0)
    {
        //
        //  The 'default' value of the time zone key specifies the
        //  default zone.
        //
        TCHAR szDefaultName[TZNAME_SIZE];
        LONG len = sizeof(szDefaultName);

        if (RegQueryValue( HKEY_LOCAL_MACHINE,
                           c_szTimeZones,
                           szDefaultName,
                           &len ) == ERROR_SUCCESS)
        {
#ifdef UNICODE
            lstrcpy(tziCurrent.StandardName, szDefaultName);
#else
            StrToWideStr(tziCurrent.StandardName, szDefaultName);
#endif
        }
        else
        {
            tziCurrent.StandardName[0] = 0;
        }

        //
        //  If we can't find it by name, use GMT.
        //
        tziCurrent.StandardBias = tziCurrent.DaylightBias = tziCurrent.Bias = 0;

        //
        //  Force the user to make a valid choice before quitting.
        //
        fForceSelection = TRUE;
    }

    //
    //  Get the Time Zones from the registry.
    //
    InitTimezones(hDlg, state->lookup);

    //
    //  Try to select the 'current' one or some lame equivalent.
    //

    //
    //  Start with an invalid index.
    //
    iCurrentTZ = g_nTimeZones;

    //
    //  Try to find by name.
    //
    for (j = 0; j < g_nTimeZones; j++)
    {
        ptzi = GetPTZ(hDlg, j);

#ifdef UNICODE
        if (!lstrcmpi(ptzi->szStandardName, tziCurrent.StandardName))
#else
        if (!AnsiWideStrCmpI(ptzi->szStandardName, tziCurrent.StandardName))
#endif
        {
            iCurrentTZ = j;
            break;
        }
    }

    //
    //  If it hasn't been found yet, try to find a nearby zone using biases.
    //
    if (iCurrentTZ == g_nTimeZones)
    {
        int nBestHitCount = TZ_HIT_NONE;

        for (j = 0; j < g_nTimeZones; j++)
        {
            ptzi = GetPTZ(hDlg, j);

            if (ptzi->Bias == tziCurrent.Bias)
            {
                int nHitCount = TZ_HIT_BASE +
                            ((ptzi->StandardBias == tziCurrent.StandardBias) +
                             (ptzi->DaylightBias == tziCurrent.DaylightBias));

                if (nHitCount > nBestHitCount)
                {
                    nBestHitCount = nHitCount;
                    iCurrentTZ = j;

                    if (nHitCount >= TZ_HIT_EXACT)
                    {
                        break;
                    }
                }
            }
        }
    }

    //
    //  Still didn't find it?
    //
    if (iCurrentTZ == g_nTimeZones)
    {
        //
        //  Punt.
        //
        iCurrentTZ = 0;

        fForceSelection = TRUE;
    }

    //
    //  Set up the dialog using this time zone's info.
    //

    //
    //  Always use our rules for the allow-daylight muck.
    //
#ifndef WINNT
    if ((ptzi = GetPTZ(hDlg, iCurrentTZ)) != NULL)
    {
        tziCurrent.StandardDate = ptzi->StandardDate;
        tziCurrent.DaylightDate = ptzi->DaylightDate;
    }
#endif

    //
    //  If wMonth is 0, then this Time Zone does not support DST.
    //
    if ((tziCurrent.StandardDate.wMonth == 0) ||
        (tziCurrent.DaylightDate.wMonth == 0))
    {
        ShowWindow(GetDlgItem(hDlg, IDD_AUTOMAGIC), SW_HIDE);
    }

    //
    //  Always get "allow DLT" flag even if this zone is disabled.
    //
    CheckDlgButton(hDlg, IDD_AUTOMAGIC, GetAllowLocalTimeChange());

    ComboBox_SetCurSel(GetDlgItem(hDlg, IDD_TIMEZONES), iCurrentTZ);

    ChangeZone(hDlg, state, GetPTZ(hDlg, -1));
    CenterZone(hDlg, state, FALSE);

    if (fForceSelection || g_bFirstBoot)
    {
        PropSheet_Changed(GetParent(hDlg), hDlg);
        PropSheet_CancelToClose(GetParent(hDlg));
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  SetTheTimezone
//
//  Apply the User's time zone selection.
//
////////////////////////////////////////////////////////////////////////////

void SetTheTimezone(
    BOOL bAutoMagicTimeChange,
    BOOL bAutoMagicEnabled,
    PTZINFO ptzi)
{
    TIME_ZONE_INFORMATION tzi;
    HCURSOR hCurOld;

    if (!ptzi)
    {
        return;
    }

    tzi.Bias = ptzi->Bias;

#ifdef WINNT
    if ((bAutoMagicTimeChange == 0) ||
        (ptzi->StandardDate.wMonth == 0))
    {
        //
        //  Standard Only.
        //
        tzi.StandardBias = ptzi->StandardBias;
        tzi.DaylightBias = ptzi->StandardBias;
        tzi.StandardDate = ptzi->StandardDate;
        tzi.DaylightDate = ptzi->StandardDate;

#ifdef UNICODE
        lstrcpy(tzi.StandardName, ptzi->szStandardName);
        lstrcpy(tzi.DaylightName, ptzi->szStandardName);
#else
        StrToWideStr(tzi.StandardName, ptzi->szStandardName);
        StrToWideStr(tzi.DaylightName, ptzi->szStandardName);
#endif
    }
    else
#endif
    {
        //
        //  Automatically adjust for Daylight Saving Time.
        //
        tzi.StandardBias = ptzi->StandardBias;
        tzi.DaylightBias = ptzi->DaylightBias;
        tzi.StandardDate = ptzi->StandardDate;
        tzi.DaylightDate = ptzi->DaylightDate;

#ifdef UNICODE
        lstrcpy(tzi.StandardName, ptzi->szStandardName);
        lstrcpy(tzi.DaylightName, ptzi->szDaylightName);
#else
        StrToWideStr(tzi.StandardName, ptzi->szStandardName);
        StrToWideStr(tzi.DaylightName, ptzi->szDaylightName);
#endif
    }

    SetAllowLocalTimeChange(bAutoMagicTimeChange);

    SetTimeZoneInformation(&tzi);

    hCurOld = SetCursor(LoadCursor(NULL, IDC_WAIT));

    SetCursor(hCurOld);
}


////////////////////////////////////////////////////////////////////////////
//
//  TimeZoneDlgProc
//
////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK TimeZoneDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    TZPAGE_STATE *state = (TZPAGE_STATE *)GetWindowLongPtr(hDlg, DWLP_USER);

    int  i;

    switch (uMsg)
    {
        case ( WM_INITDIALOG ) :
        {
            state = (TZPAGE_STATE *)LocalAlloc(LPTR, sizeof(TZPAGE_STATE));

            SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)state);

            if (!state)
            {
                EndDialog(hDlg, -1);
                break;
            }

            state->initializing = TRUE;
            InitTimeZonePage(hDlg, state);
            state->initializing = FALSE;

            break;
        }
        case ( WM_DESTROY ) :
        {
            for (i = 0; i < g_nTimeZones; i++)
            {
                LocalFree((HLOCAL)GetPTZ(hDlg, i));
            }

            if (state)
            {
                LocalFree((HANDLE)state);
                SetWindowLongPtr(hDlg, DWLP_USER, 0L);
            }
            break;
        }
        case ( WM_NOTIFY ) :
        {
            switch (((NMHDR *)lParam)->idFrom)
            {
                case ( 0 ) :
                {
                    switch (((NMHDR *)lParam)->code)
                    {
                        case ( PSN_APPLY ) :
                        {
                            g_ptziCurrent = NULL;

                            //
                            //  Find out which listbox item was selected.
                            //
                            SetTheTimezone(
                                IsDlgButtonChecked(hDlg, IDD_AUTOMAGIC),
                                IsWindowVisible(GetDlgItem(hDlg, IDD_AUTOMAGIC)),
                                GetPTZ(hDlg, -1) );

                            //
                            // if the user had modified the time as well as the timezone,
                            // then we should honor the time that they gave us since they
                            // explicitly said this was the time.  If we don't then the
                            // time they entered will be offset by the timezone change
                            //
                       
                            if (g_WasModified)
                            {
                                g_WasModified = FALSE;
                                SetTime();
                            }
                            break;
                        }
                    }
                    break;
                }
                case ( IDD_TIMEMAP ) :
                {
                    NFYMAPEVENT *event = (NFYMAPEVENT *)lParam;

                    switch (event->hdr.code)
                    {
                        case ( MAPN_TOUCH ) :
                        {
                            HotTrackZone(hDlg, state, event->index);
                            break;
                        }
                        case ( MAPN_SELECT ) :
                        {
                            CenterZone(hDlg, state, TRUE);
                            break;
                        }
                    }
                    break;
                }
            }
            break;
        }
        case ( WM_COMMAND ) :
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
                case ( IDD_TIMEZONES ) :    // combo box
                {
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == CBN_SELCHANGE)
                    {
                        ChangeZone(hDlg, state, GetPTZ(hDlg, -1));
                        CenterZone(hDlg, state, TRUE);
                    }
                    break;
                }
                case ( IDD_AUTOMAGIC ) :    // check box
                {
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == BN_CLICKED)
                    {
                        PropSheet_Changed(GetParent(hDlg), hDlg);
                    }
                    break;
                }
            }
            break;
        }
        case ( WM_HELP ) :             // F1
        {
            WinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle,
                     NULL,
                     HELP_WM_HELP,
                     (DWORD_PTR)(LPTSTR)aDateTimeHelpIds );
            break;
        }
        case ( WM_CONTEXTMENU ) :      // right mouse click
        {
            WinHelp( (HWND)wParam,
                     NULL,
                     HELP_CONTEXTMENU,
                     (DWORD_PTR)(LPTSTR)aDateTimeHelpIds );
            break;
        }
        default :
        {
            return (FALSE);
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetClInt
//
//  Steal an int from the command line.
//
////////////////////////////////////////////////////////////////////////////

static int GetClInt(
    const TCHAR *p)
{
    BOOL neg = FALSE;
    int v = 0;

    //
    //  Skip spaces.
    //
    while (*p == TEXT(' '))
    {
        p++;
    }

    //
    //  See if it's negative.
    //
    if (*p == TEXT('-'))
    {
        //
        //  It's negative.  Remember that it's negative and skip the
        //  '-' char.
        //
        neg = TRUE;
        p++;
    }

    //
    //  Parse the absolute portion.  Digits only.
    //
    while ((*p >= TEXT('0')) && (*p <= TEXT('9')))
    {
        //
        //  Accumulate the value.
        //
        v = v * 10 + *p++ - TEXT('0');
    }

    //
    //  Return the result.
    //
    return (neg ? -v : v);
}


////////////////////////////////////////////////////////////////////////////
//
//  SelectZoneByName
//
////////////////////////////////////////////////////////////////////////////

BOOL SelectZoneByName(
    LPCTSTR cmdline)
{
    BOOL result = FALSE;
    HKEY key = NULL;

    while (*cmdline == TEXT(' '))
    {
        cmdline++;
    }

    if (!*cmdline)
    {
        return (FALSE);
    }

    if (RegOpenKey( HKEY_LOCAL_MACHINE,
                    c_szTimeZones,
                    &key ) == ERROR_SUCCESS)
    {
        TCHAR name[TZNAME_SIZE];
        HKEY subkey = NULL;
        TZINFO zone;

        //
        //  User can pass key name.
        //
        if (RegOpenKey(key, cmdline, &subkey) == ERROR_SUCCESS)
        {
            if (ReadZoneData(&zone, subkey, cmdline))
            {
                result = TRUE;
            }
        }
        else
        {
            //
            //  User can also pass display name.
            //
            int i;
            int CmdLen = lstrlen(cmdline);

            for (i = 0;
                 RegEnumKey(key, i, name, TZNAME_SIZE) == ERROR_SUCCESS;
                 i++)
            {
                if (RegOpenKey(key, name, &subkey) == ERROR_SUCCESS)
                {
                    LONG len = sizeof(zone.szDisplayName);

                    if ((RegQueryValueEx( subkey,
                                          c_szTZDisplayName,
                                          0,
                                          NULL,
                                          (LPBYTE)&zone.szDisplayName,
                                          &len ) == ERROR_SUCCESS) &&
                        (CompareString( GetUserDefaultLCID(),
                                        NORM_IGNORECASE  | NORM_IGNOREKANATYPE |
                                        NORM_IGNOREWIDTH | NORM_IGNORENONSPACE,
                                        zone.szDisplayName,
                                        (CmdLen < 15)
                                            ? -1
                                            : min(lstrlen(zone.szDisplayName),
                                                  CmdLen),
                                        cmdline,
                                        -1 ) == CSTR_EQUAL))
                    {
                        if (ReadZoneData(&zone, subkey, name))
                        {
                            result = TRUE;
                        }
                    }

                    RegCloseKey(subkey);
                }

                if (result)
                {
                    break;
                }
            }
        }

        RegCloseKey(key);

        if (result)
        {
            SetTheTimezone(1, 1, &zone);
        }
    }

    return (result);
}


////////////////////////////////////////////////////////////////////////////
//
//  OpenDateTimePropertySheet
//
//  Opens a DateTime property sheet.
//  Set the page for the property sheet.
//
////////////////////////////////////////////////////////////////////////////

BOOL OpenDateTimePropertySheet(
    HWND hwnd,
    LPCTSTR cmdline)
{
    //
    //  Make this an array for multiple pages.
    //
    PROPSHEETPAGE apsp[2];
    PROPSHEETHEADER psh;
    HDC hDC;
    HFONT   hFont;
    int wMaxDigitWidth;
    
    hDC = GetDC(hwnd);

    wMaxDigitWidth = GetMaxSubstitutedCharWidth(hDC);
    ReleaseDC(hwnd, hDC);

    psh.nStartPage = (UINT)-1;

    if (cmdline && *cmdline)
    {
        if (*cmdline == TEXT('/'))
        {
            BOOL fAutoSet = FALSE;

            //
            //  Legend:
            //    zZ: first boot batch mode setup "/z pacific" etc
            //    fF: regular first boot
            //    mM: time zone change forced local time change message
            //
            switch (*++cmdline)
            {
                case ( TEXT('z') ) :
                case ( TEXT('Z') ) :
                {
                    fAutoSet = TRUE;

                    //
                    //  Fall thru...
                    //
                }
                case ( TEXT('f') ) :
                case ( TEXT('F') ) :
                {
                    g_bFirstBoot = TRUE;

                    if (fAutoSet && SelectZoneByName(cmdline + 1))
                    {
                        return (TRUE);
                    }

                    //
                    //  Start on time zone page.
                    //
                    psh.nStartPage = 1;
                    break;
                }
                case ( TEXT('m') ) :
                case ( TEXT('M') ) :
                {
                    MSGBOXPARAMS params =
                                  {
                                    sizeof(params),
                                    hwnd,
                                    g_hInst,
                                    MAKEINTRESOURCE(IDS_WARNAUTOTIMECHANGE),
                                    MAKEINTRESOURCE(IDS_WATC_CAPTION),
                                    MB_OK | MB_USERICON,
                                    MAKEINTRESOURCE(IDI_TIMEDATE),
                                    0,
                                    NULL,
                                    0
                                  };

                    MessageBoxIndirect(&params);

                    //
                    //  Show time/date page for user to verify.
                    //
                    psh.nStartPage = 0;

                    break;
                }
                default :
                {
                    //
                    //  Fall out, maybe it's a number...
                    //
                    break;
                }
            }
        }
    }

    if (psh.nStartPage == (UINT)-1)
    {
        if (cmdline && (*cmdline >= TEXT('0')) && (*cmdline <= TEXT('9')))
        {
            psh.nStartPage = GetClInt(cmdline);
        }
        else
        {
            psh.nStartPage = 0;
        }
    }

    //
    //  Register our classes.
    //
    ClockInit(g_hInst);
    CalendarInit(g_hInst);
    RegisterMapControlStuff(g_hInst);

    psh.dwSize = sizeof(psh);
    if (g_bFirstBoot)
    {
        //
        //  Disable Apply button for first boot.
        //
        psh.dwFlags = PSH_PROPTITLE | PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
    }
    else
    {
        psh.dwFlags = PSH_PROPTITLE | PSH_PROPSHEETPAGE;
    }
    psh.hwndParent = hwnd;
    psh.hInstance = g_hInst;
    psh.pszIcon = NULL;

    //
    //  psh.nStartPage is set above.
    //
    psh.pszCaption = MAKEINTRESOURCE(IDS_TIMEDATE);
    psh.nPages = 2;
    psh.ppsp = apsp;

    apsp[0].dwSize = sizeof(PROPSHEETPAGE);
    apsp[0].dwFlags = PSP_DEFAULT;
    apsp[0].hInstance = g_hInst;
    apsp[0].pszTemplate = wMaxDigitWidth > 8 ? MAKEINTRESOURCE(DLG_DATETIMEWIDE) : MAKEINTRESOURCE(DLG_DATETIME);
    apsp[0].pfnDlgProc = DateTimeDlgProc;
    apsp[0].lParam = 0;

    apsp[1].dwSize = sizeof(PROPSHEETPAGE);
    apsp[1].dwFlags = PSP_DEFAULT;
    apsp[1].hInstance = g_hInst;
    apsp[1].pszTemplate = MAKEINTRESOURCE(DLG_TIMEZONE);
    apsp[1].pfnDlgProc = TimeZoneDlgProc;
    apsp[1].lParam = 0;

    if (psh.nStartPage >= psh.nPages)
    {
        psh.nStartPage = 0;
    }

    return ( (int)PropertySheet(&psh) );
}
