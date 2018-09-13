/** FILE: date.c *********** Module Header ********************************
 *
 *  Control panel applet for Date/Time configuration.  This file holds
 *  everything to do with the "DateTime" dialog box in the Control
 *  Panel.
 *
 * History:
 *  12:30 on Tues  23 Apr 1991    -by-  Steve Cathcart   [stevecat]
 *        Took base code from Win 3.1 source
 *  10:30 on Tues  04 Feb 1992    -by-  Steve Cathcart   [stevecat]
 *        Updated code to latest Win 3.1 sources
 *  12:30 on Tues  27 Oct 1992    -by-  Steve Cathcart   [stevecat]
 *        Added TimeZone extensions
 *  22:00 on Wed   17 Nov 1993    -by-  Steve Cathcart   [stevecat]
 *        Time Zone resources in registry
 *
 *  Copyright (C) 1990-1993 Microsoft Corporation
 *
 *************************************************************************/
//==========================================================================
//                             Include files
//==========================================================================
// C Runtime
#include <stddef.h>
#include <stdlib.h>

// Application specific
#include "main.h"

//==========================================================================
//                            Local Definitions
//==========================================================================

// #define TZMAP

//==========================================================================
//                            External Declarations
//==========================================================================


//==========================================================================
//                            Local Data Declarations
//==========================================================================
short wDeltaDateTime[6];         /* Amount of time change */

#ifdef JAPAN    /* V-KeijiY  June.29.1992 */
// need more space to hold strings
// At least "GoZen" and "GoGo"
TCHAR  sz1159[TIMESUF_LEN];
TCHAR  sz2359[TIMESUF_LEN];
#else
TCHAR  sz1159[4];
TCHAR  sz2359[4];
#endif

BOOL  bAMPM;
BOOL  bPM;
BOOL  bPrefix;
BOOL  bDisplayMessage = FALSE;

WORD wTimerOn;
BOOL bLZero[6] = {FALSE, TRUE, TRUE, FALSE, FALSE, FALSE};


ARROWVSCROLL avs[6] = { { 1, -1, 5, -5, 23, 0, 12, 12 },
                        { 1, -1, 5, -5, 59, 0, 30, 30 },
                        { 1, -1, 5, -5, 59, 0, 30, 30 },
                        { 1, -1, 4, -4, 12, 1, 0, 0 },
                        { 1, -1, 5, -5, 31, 1, 0, 0 },
                        { 1, -1, 10, -10, 2099, 1980, 1990, 1990 }
                      };

#ifdef TZMAP
// "World" memory bitmap variables

HPALETTE hpalTZmap = NULL;

HBITMAP hbmTZmap = NULL;
HBITMAP hbmTZDefault = NULL;
HBITMAP hbmBitmaps = NULL;
HBITMAP hbmDefault = NULL;
HDC hdcMem   = NULL;
HDC hdcTZmap = NULL;
int iWidth, iHeight;        // World bitmap width and height
#endif  //  TZMAP

LONG  NumTimeZones = 0;

PAPPLET_TIME_ZONE_INFORMATION Tzi;
LPTIME_ZONE_INFORMATION SelectedTimeZone = NULL;
TIME_ZONE_INFORMATION   TimeZone;

PAPPLET_TIME_ZONE_INFORMATION ptziOriginal;
int iOriginalTimeZone;
int iOrigButtonChecked;

//  Registry location for Time Zone information
TCHAR *pszTimezones = TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones");

// Time Zone data value keys
TCHAR *pszTZDisplayName    = TEXT("Display");  
TCHAR *pszTZDaylightName   = TEXT("Dlt");  
TCHAR *pszTZI              = TEXT("TZI");


//==========================================================================
//                            Local Function Prototypes
//==========================================================================
void DateTimeInit(HWND   hDlg,
                  WORD   nBaseID,
                  WORD   nSepID,
                  TCHAR *pszSep,
                  int    nMaxDigitWidth,
                  BOOL   bDate);

VOID   CentreWindow(HWND hwnd);
LPVOID GetResPtr (int iResType);
BOOL   InitTimeZone (HWND hDlg);
LONG   PaintWorld (HWND hwndWorld, HDC hdcWorld);
BOOL   SetupWorld (HWND hWnd);
VOID   SetTheVirtualTimezone (HWND hDlg, int DaylightOption, PAPPLET_TIME_ZONE_INFORMATION ptzi);
void   UpdateItem (HWND hDlg, short i);

//==========================================================================
//                                Functions
//==========================================================================


/* ParseDateElement assumes that the character pointed to by pszElement
                    is a 'M', 'd', or 'y', and checks if the string
                    indicates a leading zero or century.  The return
                    value is a pointer to the next character, which
                    should be a separator or NULL.  A return value of
                    NULL indicates an error.
*/

TCHAR *ParseDateElement (TCHAR *pszElement, BOOL *pbLZero)
{
    switch (*pszElement)  /* Check for valid character */
    {
       case TEXT('y'):
       case TEXT('M'):
       case TEXT('d'):
/*
         case TEXT('h'):
         case TEXT('m'):
         case TEXT('s'):
*/
           break;
       default:
           return (NULL);
    }

    ++pszElement;
    if (*pszElement != *(pszElement - 1))
        *pbLZero = 0;
    else
    {
        *pbLZero = 1;
        if (*pszElement++ == TEXT('y'))
        {
            if (!(*pszElement == TEXT('y')))
                *pbLZero = 0;
            else if (!(*++pszElement == TEXT('y')))
                return (NULL);  /* Found 3 y's, invalid format */
            else
                ++pszElement;
        }
    }
    return (pszElement);
}


/* AdjustDelta() alters the variables in wDeltaDateTime, allowing a
                 CANCEL button to perform its job by resetting the
                 time as if it had never been touched.  GetTime() &
                 GetDate() should already have been called.
*/

void AdjustDelta (HWND hDlg, short nIndex)
{
    short    nDelta;
    BOOL bOK;

    nDelta = (short) GetDlgItemInt (hDlg, DATETIME_HOUR + nIndex, (BOOL * ) &bOK, FALSE);
    GetDateTime ();

    if (nIndex == HOUR)
    {
        if (!bAMPM)
        {
            if (nDelta == 12)
            {
                if (!bPM)
                    nDelta = 0;
            }
            else if (bPM)
                nDelta += 12;
        }
    }
    else if ((nIndex == YEAR) && !bLZero[YEAR])
    {
        if (nDelta < 80)
            nDelta += 2000;
        else
            nDelta += 1900;
//            nDelta += wDateTime[YEAR] - wDateTime[YEAR] % 100;
    }
    else if ((nIndex == MONTH) || (nIndex == DAY))
    {
        //
        //  Check for invalid DAY or MONTH value
        //

        if (nDelta == 0)
        {
            //
            //  Set it to current date value
            //

            UpdateItem (hDlg, nIndex);
            nDelta = wDateTime[nIndex];
        }
    }

    if (wDateTime[nIndex] != nDelta)
    {
        wDeltaDateTime[nIndex] += nDelta - wDateTime[nIndex];
        wPrevDateTime[nIndex] = wDateTime[nIndex] = nDelta;
        if (nIndex < 3)
            SetTime ();
        else
            SetDate ();
    }
}


short ReadShortDate (TCHAR *pszDate, BOOL *pbMonth, BOOL *pbDay, BOOL *pbYear)
{
    short  i, nOrder;
    BOOL  *pbOrder[3];
    TCHAR   cHope[3];

    switch (cHope[0] = *pszDate)
    {
       case TEXT('M'):
           nOrder = 0;
           pbOrder[0] = pbMonth;
           break;

       case TEXT('d'):
           nOrder = 1;
           pbOrder[0] = pbDay;
           break;

       case TEXT('y'):
           nOrder = 2;
           pbOrder[0] = pbYear;
           break;

       default:
           return (FALSE);
    }
    if (nOrder)
    {
        cHope[1] = TEXT('M');
        pbOrder[1] = pbMonth;
    }
    else
    {
        cHope[1] = TEXT('d');
        pbOrder[1] = pbDay;
    }
    if (nOrder == 2)
    {
        cHope[2] = TEXT('d');
        pbOrder[2] = pbDay;
    }
    else
    {
        cHope[2] = TEXT('y');
        pbOrder[2] = pbYear;
    }

    for (i = 0; i < 3; i++, pszDate++)
    {
        if (*pszDate != cHope[i])
            return ((short) (-1 - nOrder));
        if (!(pszDate = ParseDateElement (pszDate, pbOrder[i])))
            return ((short) (-1 - nOrder));
    }

    return (nOrder);  /* Success.  Return MDY, DMY or YMD index */
}


/* Determine the widest digit (safety against variable pitch fonts) */
int GetMaxCharWidth (HDC hDC)
{
    int    *pNumWidth;
    int    nNumWidth[10];
    int    nMaxNumWidth;

    GetCharWidth (hDC, (DWORD)TEXT('0'), (DWORD)TEXT('9'), (LPINT) nNumWidth);

    pNumWidth = nNumWidth + 1;
    for (nMaxNumWidth = nNumWidth[0]; pNumWidth < (int *) (nNumWidth + 10);
            pNumWidth++)
    {
        if (*pNumWidth > nMaxNumWidth)
            nMaxNumWidth = *pNumWidth;
    }
    return (nMaxNumWidth);
}


void DateTimeInit (hDlg, nBaseID, nSepID, pszSep, nMaxDigitWidth, bDate)
HWND  hDlg;
WORD  nBaseID;
WORD  nSepID;
TCHAR *pszSep;
int   nMaxDigitWidth;
BOOL  bDate;
{
    HWND  hAMPMList, hwndTemp;
    HWND  hDay, hMonth, hYear;        // also used as hHour, hMinute, & hSecond
    HWND  hOrder[5];
    HDC   hDC;
    short nWidth, nHeight, X;
    RECT  Rect;
    int   i;
    short nAMPMlength;
    BOOL  bSuccess;
    SIZE  TxtSize;
    TCHAR szMessage[256];
    TCHAR szTemp[128];
    short   nSaveXpos;


    hMonth    = GetDlgItem (hDlg, nBaseID);
    hDay      = GetDlgItem (hDlg, nBaseID + 1);
    hYear     = GetDlgItem (hDlg, nBaseID + 2);
    hOrder[1] = GetDlgItem (hDlg, nSepID);
    hOrder[3] = GetDlgItem (hDlg, nSepID + 1);

    if (bDate)
    {
        i = GetLocaleValue (0,
                            LOCALE_IDATE,
                            szTemp,
                            CharSizeOf(szTemp),
                            TEXT("0"));
    }
    else
    {
        if (!(bAMPM = (BOOL) GetLocaleValue (0,
                                             LOCALE_ITIME,
                                             szTemp,
                                             CharSizeOf(szTemp),
                                             TEXT("0"))))
        {
            bPrefix = GetLocaleValue (0,
                                      LOCALE_ITIMEMARKPOSN,
                                      szTemp,
                                      CharSizeOf(szTemp),
                                      TEXT("0"));
            GetLocaleValue (0,
                            LOCALE_S1159,
                            sz1159,
                            CharSizeOf(sz1159),
                            IntlDef.s1159);
            GetLocaleValue (0,
                            LOCALE_S2359,
                            sz2359,
                            CharSizeOf(sz2359),
                            IntlDef.s2359);
        }
        i = 0;
    }

    switch (i)
    {
       case 1:
          hOrder[0] = hDay;
          hOrder[2] = hMonth;
          hOrder[4] = hYear;
          break;

       case 2:
          hOrder[0] = hYear;
          hOrder[2] = hMonth;
          hOrder[4] = hDay;
          break;

       case 0:
       default:
          hOrder[0] = hMonth;
          hOrder[2] = hDay;
          hOrder[4] = hYear;
          break;
    }

    //  Find the correct window for proper order for date controls and
    //  for window positioning

    switch (nBaseID)
    {
        case DATETIME_HOUR:
            hwndTemp = GetDlgItem (hDlg, IDD_TZ_TIME);
            break;

        case DATETIME_MONTH:
            hwndTemp = GetDlgItem (hDlg, IDD_TZ_DATE);
            break;

        case IDD_TZ_SD_MONTH:
            hwndTemp = GetDlgItem (hDlg, IDD_TZ_SDATE);
            break;

        case IDD_TZ_ED_MONTH:
            hwndTemp = GetDlgItem (hDlg, IDD_TZ_EDATE);
            break;

        default:
            hwndTemp = hDlg;
            break;
    }

    if (bDate)
    {
        SetWindowPos (hOrder[0], hwndTemp, 0, 0, 0, 0,
                      SWP_NOMOVE | SWP_NOSIZE | SWP_NOREDRAW);
        SetWindowPos (hOrder[2], hOrder[0], 0, 0, 0, 0,
                      SWP_NOMOVE | SWP_NOSIZE | SWP_NOREDRAW);
        SetWindowPos (hOrder[4], hOrder[2], 0, 0, 0, 0,
                      SWP_NOMOVE | SWP_NOSIZE | SWP_NOREDRAW);
    }

    hDC = GetDC (hDlg);
    if (!bDate)
    {
        bSuccess = GetTextExtentPoint (hDC, sz2359, lstrlen (sz2359), &TxtSize);
        nAMPMlength = (short) TxtSize.cx;
        bSuccess = GetTextExtentPoint (hDC, sz1159, lstrlen (sz1159), &TxtSize);

        if (nAMPMlength < (short) TxtSize.cx)
            nAMPMlength = (short) TxtSize.cx;
    }

#ifdef JAPAN
        //We need to add the borders of ListBox.
                //            Bug#231                  1992.10.27   by yutakas
    nAMPMlength += 2;
#endif

    bSuccess = GetTextExtentPoint (hDC, pszSep, lstrlen (pszSep), &TxtSize);
    ReleaseDC (hDlg, hDC);

    GetWindowRect (hYear, (LPRECT) &Rect);
    ScreenToClient (hDlg, (LPPOINT) &Rect.left);
    ScreenToClient (hDlg, (LPPOINT) &Rect.right);
    nHeight = (short) (Rect.bottom - Rect.top);
    nWidth = (short) Rect.top;

    GetWindowRect (hwndTemp, (LPRECT) &Rect);

    ScreenToClient (hDlg, (LPPOINT) &Rect.left);
    ScreenToClient (hDlg, (LPPOINT) &Rect.right);
    Rect.top = (long) nWidth;
    X = (short) ((Rect.left + Rect.right - 6 * nMaxDigitWidth - 2 * TxtSize.cx) / 2);
    if (bDate)
    {
        if (bLZero[YEAR])
            X -= (short) nMaxDigitWidth;
    }
    else if (!bAMPM)
    {
        if (bPrefix)
        {
            nSaveXpos = X - nAMPMlength / 2;
            X = nSaveXpos + nAMPMlength;
        }
        else
            X -= nAMPMlength / 2;
    }

    for (i = 0; i < 5; i++)
    {
        nWidth = (short) ((i % 2) ? TxtSize.cx : 2 * nMaxDigitWidth);
        if ((hOrder[i] == hYear) && bDate && bLZero[YEAR])
            nWidth *= 2;
        nWidth += 2;                     /* allow for centering in edit control*/
        MoveWindow (hOrder[i], X, Rect.top, nWidth, nHeight, FALSE);
        X += nWidth;
    }

    hAMPMList = GetDlgItem (hDlg, DATETIME_AMPM);
    if (!bDate && !bAMPM)
    {
        if (bPrefix)
            X = nSaveXpos;

        MoveWindow (hAMPMList, X, Rect.top, nAMPMlength, nHeight, FALSE);
        SendMessage (hAMPMList, LB_INSERTSTRING, (WPARAM)(LONG) -1, (LPARAM)sz1159);
        SendMessage (hAMPMList, LB_INSERTSTRING, (WPARAM)(LONG)-1, (LPARAM)sz2359);
        SendMessage (hAMPMList, LB_SETCURSEL, (WPARAM)(LONG)-1, 0);
    }
    EnableWindow (hAMPMList, !bAMPM);

    SendMessage (hYear, EM_LIMITTEXT, (bDate && bLZero[YEAR]) ? 4 : 2, 0L);
    SendMessage (hMonth, EM_LIMITTEXT, 2, 0L);
    SendMessage (hDay, EM_LIMITTEXT, 2, 0L);
    SetDlgItemText (hDlg, nSepID, pszSep);
    SetDlgItemText (hDlg, nSepID + 1, pszSep);

    //  If running under "Setup" display helpful message
    //  and disable "Cancel" 
    if (bDisplayMessage)
    {
        if (LoadString (hModule, DATE+9, szMessage, CharSizeOf(szMessage)-2))
        {
            SetDlgItemText (hDlg, DATETIME_MSG, szMessage);
        }
        EnableWindow (GetDlgItem (hDlg, PUSH_CANCEL), FALSE);
    }
}


BOOL CheckNum (HWND hDlg, WORD nID)
{
    short    i;
    TCHAR    szNum[5];
    BOOL    bReturn;
    INT     iVal;

    bReturn = TRUE;

    GetDlgItemText (hDlg, nID, szNum, CharSizeOf(szNum));

    for (i = 0; szNum[i]; i++)
        if (!_istdigit (szNum[i]))
            return (FALSE);

    iVal = MyAtoi(szNum);

    switch (nID)
    {
       case DATETIME_HOUR:
          if (iVal > 23)
             bReturn = FALSE;
          break;

       case DATETIME_MINUTE:
       case DATETIME_SECOND:
          if (iVal > 59)
             bReturn = FALSE;
          break;

       case DATETIME_MONTH:
          if (iVal > 12)
             bReturn = FALSE;
          break;

       case DATETIME_DAY:
          if (iVal > 31)
             bReturn = FALSE;
          break;
    }
    return (bReturn);
}


short MonthUpperBound (short nMonth, short nYear)
{
    switch (nMonth)
    {
       case 2:
           /* The following line accounts for leap years every 400 years and
            * every 4 years, except every 100 years.
            */
           return ((short) (!(nYear % 400) || ((nYear % 100) && !(nYear % 4)) ? 29 : 28));
           break;

       case 4:
       case 6:
       case 9:
       case 11:
           return (30);
           break;
    }
    return (31);
}


void UpdateItem (HWND hDlg, short i)
{
    TCHAR    szNum[5];
    short    nNum = wDateTime[i];

    if (i == YEAR)
    {
        if (!bLZero[i])
            nNum %= 100;
    }
    else if ((i == HOUR) && !bAMPM)
    {
        bPM = (nNum >= 12) ? 1 : 0;
        SendDlgItemMessage (hDlg, DATETIME_AMPM, LB_SETTOPINDEX, bPM, 0L);
        if (bPM)
            nNum -= 12;
        if (!nNum)
            nNum = 12;
    }
    if ((nNum < 10) && (bLZero[i] || (i == YEAR)))
    {
        szNum[0] = TEXT('0');
        szNum[1] = (TCHAR)(TEXT('0') + nNum);
        szNum[2] = TEXT('\0');
    }
    else
        MyItoa (nNum, szNum, 10);
    SetDlgItemText (hDlg, DATETIME_HOUR + i, szNum);
    SendDlgItemMessage (hDlg, DATETIME_HOUR + i, EM_SETSEL, 0, 32767);
    if (i == MONTH || i == YEAR)
        avs[DAY].top = MonthUpperBound (wDateTime[MONTH], wDateTime[YEAR]);
}


BOOL DateTimeDlg (HWND hDlg, UINT message, DWORD wParam, LONG lParam)
{
    int   nNewNum;  /* Temporary */
    int   nMaxDigitWidth;
    int   j;
    short i;
    short nDelta;
    TCHAR  szNum[5];
    TCHAR  szShortDate[12];
    HDC   hDC;
    HFONT hFont;
    BOOL  bOK;
    HWND  hwndCB;
    int   iTimeZone, iButtonChecked;
    PAPPLET_TIME_ZONE_INFORMATION ptzi;
    TCHAR szTemp[128];

    switch (message)
    {
        case WM_INITDIALOG:
            HourGlass (TRUE);
        
            //  Center dialog on screen, both since it is large and for setup
            CentreWindow(hDlg);

            //
            //  TEMPORARY FIX FOR BUG 10170 - Time screw-up if User attempts
            //  to change Timezone and Date/Time at same time.  This forces
            //  applet to work the same way as it does under Setup, except it
            //  does not display message.
            //

            bDisplayMessage = bSetup;
            bSetup = TRUE;

//
// BUG 10170 - When user tries to set tz and date/time at same time in applet
// the date/time will get fouled up.  This is because when the tz is changed
// we set the global "SelectedTimezone" to point to a TZINFO struct.  That is
// used in "SystemTimeToTimezoneSpecificLocalTime" api call to get what the
// local time would be in that timezone if it were selected.
//
// If a user then tries to change the date or time, a call is made to SetTime
// which uses the displayed time in a call to SetLocalTime().  That call in
// turn changes the SystemTime.  Then the next call to GetDate/Time will use
// the now erroneous SystemTime in its next call to SystemTimeToTZSpecific
// LocalTime" api and get a bogus time.
//
//
// We want to preserve the behavoir of the Date/Time applet to show the current
// local time for the user's new TZ selection, while still allowing them to
// change the date or time.
//
// The temporary fix for this is to force applet to act the same way it does
// under Setup, where current date and time are preserved across a TZ selection
// and the user must explicitly set the desired Date and Time.
//
            SelectedTimeZone = NULL;

            InitTimeZone (hDlg);

            //  Setup date and time fields in correct order
            bLZero[HOUR] = bLZero[MONTH] = bLZero[DAY] = bLZero[YEAR] = FALSE;
            bLZero[MINUTE] = bLZero[SECOND] = TRUE;
        
            hDC = GetDC (hDlg);
            if (hFont = (HFONT) SendMessage (hDlg, WM_GETFONT, 0, 0L))
                hFont = SelectObject (hDC, hFont);
            nMaxDigitWidth = GetMaxCharWidth (hDC);
            if (hFont)
                SelectObject (hDC, hFont);
            ReleaseDC (hDlg, hDC);
        
            bLZero[HOUR] = GetLocaleValue (0,
                                           LOCALE_ITLZERO,
                                           szTemp,
                                           CharSizeOf(szTemp),
                                           TEXT("0"));
            GetLocaleValue (0,
                            LOCALE_SSHORTDATE,
                            szShortDate,
                            CharSizeOf(szShortDate),
                            IntlDef.sShortDate);
            ReadShortDate (szShortDate, bLZero + MONTH, bLZero + DAY, bLZero + YEAR);
        
            GetLocaleValue (0,
                            LOCALE_SDATE,
                            szNum,
                            CharSizeOf(szNum),
                            IntlDef.sDateSep);
            DateTimeInit (hDlg, DATETIME_MONTH, DATETIME_DSEP1, szNum,
                          nMaxDigitWidth, TRUE);
#ifdef LATER
            //  Setup timezone date fields
            DateTimeInit (hDlg, IDD_TZ_SD_MONTH, IDD_TZ_SD_SEP1, szNum,
                                                            nMaxDigitWidth, TRUE);
        
            DateTimeInit (hDlg, IDD_TZ_ED_MONTH, IDD_TZ_ED_SEP1, szNum,
                                                            nMaxDigitWidth, TRUE);
#endif   // LATER
        
            GetLocaleValue (0,
                            LOCALE_STIME,
                            szNum,
                            CharSizeOf(szNum),
                            IntlDef.sTime);
            DateTimeInit (hDlg, DATETIME_HOUR, DATETIME_TSEP1, szNum,
                                                            nMaxDigitWidth, FALSE);
            OddArrowWindow (GetDlgItem (hDlg, DATETIME_DARROW));
            OddArrowWindow (GetDlgItem (hDlg, DATETIME_TARROW));
        
            GetDateTime ();
            avs[DAY].top = MonthUpperBound (wDateTime[MONTH], wDateTime[YEAR]);
            if (!bLZero[YEAR])
            {
                wDateTime[YEAR] %= 100;
                avs[YEAR].top = 99;
                avs[YEAR].bottom = 0;
                avs[YEAR].thumbpos = avs[YEAR].thumbtrack = 93;
            }
            else
            {
                avs[YEAR].top = 9999;
                avs[YEAR].bottom = 1980;
                avs[YEAR].thumbpos = avs[YEAR].thumbtrack = 1993;
            }
            for (i = 0; i < 6; i++)
            {
                wPrevDateTime[i] = -1;
                wDeltaDateTime[i] = 0;
            }
        
            wTimerOn = TRUE;
            SendMessage (hDlg, WM_TIMER, SECOND, 0L);
            wTimerOn = (WORD) SetTimer (hDlg, SECOND, 950, (WNDPROC) NULL);
        
            HourGlass (FALSE);
            break;
        
        case WM_VSCROLL:
            switch (nDelta = (short) GetWindowLong(GetFocus(), GWL_ID))
            {
                case DATETIME_AMPM:
                case DATETIME_HOUR:
                case DATETIME_MINUTE:
                case DATETIME_SECOND:
                    if (HIWORD(wParam) != DATETIME_TARROW)
                        return (FALSE);
                    break;
                
                case DATETIME_MONTH:
                case DATETIME_DAY:
                case DATETIME_YEAR:
                    if (HIWORD(wParam) != DATETIME_DARROW)
                        return (FALSE);
                    break;
                
                default:
                    return (FALSE);
            }
        
            i = (short) (nDelta - DATETIME_HOUR);
            GetDateTime ();
        
            switch (LOWORD(wParam))
            {
                case SB_THUMBTRACK:
                case SB_ENDSCROLL:
                    return (TRUE);
                    break;
                
                default:
                    if (nDelta == DATETIME_AMPM)
                    {
                        HWND hWndAMPM;
                
                        hWndAMPM = GetFocus ();
                        SendMessage (hWndAMPM, LB_SETCURSEL,
                                  1 - (WORD)SendMessage (hWndAMPM, LB_GETCURSEL, 0, 0L),
                                  0L);
                         return (TRUE);
                     }
                     else
                     {
                        nNewNum = GetDlgItemInt (hDlg, nDelta, &bOK, FALSE);
                        if ((i == HOUR) && !bAMPM)
                        {
                            if (bPM)
                                nNewNum += 12;
                            if (!(nNewNum % 12))
                                nNewNum -= 12;
                        }
                        wDateTime[i] = ArrowVScrollProc (LOWORD(wParam),
                                                   (short)nNewNum,
                                                   (LPARROWVSCROLL) (avs + i));
                
                        /* Wrap around if exceeded limit */
                        if (avs[i].flags & UNDERFLOW)
                            wDateTime[i] = avs[i].top;
                        else if (avs[i].flags & OVERFLOW)
                            wDateTime[i] = avs[i].bottom;
                        
                        if (i == HOUR)
                        {
                            bPM = (wDateTime[i] >= 12);
                        }
                    }
                 break;
            }
        
            /* set system var */
            UpdateItem (hDlg, i);
        
            break;
        
        case WM_TIMER:
            if (wTimerOn && (wParam == SECOND))
            {
                GetDateTime ();
        
                if (!bLZero[YEAR])
                    wDateTime[YEAR] %= 100;

                for (i = 0; i < 6; i++)
                {
                    if ((wDateTime[i] != wPrevDateTime[i]) &&
                        (GetFocus () != GetDlgItem (hDlg, DATETIME_HOUR + i)))
                    {
                        //
                        //  Update prev date-time
                        //

                        wPrevDateTime[i] = wDateTime[i];
                        UpdateItem (hDlg, i);
                    }
                }
            }
            break;
        
#ifdef  TZMAP
//        case WM_PAINT:
//            return PaintWorld (hDlg);
        
        case WM_CTLCOLORSTATIC:
            if ((HWND) lParam == GetDlgItem (hDlg, IDD_TZ_WORLD))
            {
                return PaintWorld ((HWND) lParam, (HDC) wParam);
            }
            break;
#endif   //  TZMAP
        
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDD_HELP:
                    goto DoHelp;
                
                case DATETIME_AMPM:
                {
                    switch (HIWORD(wParam))
                    {
                        case LBN_SETFOCUS:
                            SendMessage ((HWND) lParam, LB_SETCURSEL,
                               (WORD)SendMessage ((HWND) lParam, LB_GETTOPINDEX,
                               0, 0L), 0L);
                            break;
                
                        case LBN_KILLFOCUS:
                            SendMessage ((HWND)lParam, LB_SETCURSEL, (WPARAM)(LONG)-1, 0L);
                            bPM = (BOOL)SendMessage ((HWND)lParam, LB_GETTOPINDEX,
                                                    0, 0L);
                            AdjustDelta (hDlg, HOUR);
                            break;
                    }
                    break;
                }
                
                case DATETIME_HOUR:
                case DATETIME_MINUTE:
                case DATETIME_SECOND:
                case DATETIME_YEAR:
                case DATETIME_MONTH:
                case DATETIME_DAY:
                    if (HIWORD(wParam) == EN_UPDATE)
                    {
                        if (!CheckNum (hDlg, LOWORD(wParam)))
                            SendMessage ((HWND) lParam, EM_UNDO, 0, 0L);
                    }
                    else if (HIWORD(wParam) == EN_KILLFOCUS)
                    {
                        AdjustDelta (hDlg, (short) (LOWORD(wParam) - DATETIME_HOUR));
                    }
                    break;

                
#ifdef SHOW_NEW_TIME
                case IDD_TZ_DAYLIGHT:
                {
                    //  If we get a BN_CLICKED message on Checkbox, treat it as
                    //  though User made another Timezone selection - this forces
                    //  the system to recalc date/time
                
                    if (HIWORD(wParam) != BN_CLICKED)
                        break;
                
                    hwndCB = GetDlgItem (hDlg, IDD_TZ_TIMEZONES);
                
                    iTimeZone = SendMessage (hwndCB, CB_GETCURSEL, 0, 0L);
                
                    // Get DST selection
                    iButtonChecked = IsDlgButtonChecked (hDlg, IDD_TZ_DAYLIGHT);
                
                    // A disabled checkbox is same as "No DST" selection
                    if (iButtonChecked != 1)
                        iButtonChecked = 0;

                    if (iTimeZone != CB_ERR)
                    {
                        ptzi = (PAPPLET_TIME_ZONE_INFORMATION) SendMessage (
                                                              hwndCB,
                                                              CB_GETITEMDATA,
                                                              (WPARAM)iTimeZone,
                                                              0L);
                
                        if (bSetup)
                            SetTheTimezone (hDlg, iButtonChecked, ptzi);
                        else 
                            SetTheVirtualTimezone (hDlg, iButtonChecked, ptzi);
                    }
                    break;
                }
#endif  // SHOW_NEW_TIME
                
                case IDD_TZ_TIMEZONES:
                {
                    if (HIWORD(wParam) != CBN_SELCHANGE)
                        break;
                
                    hwndCB = GetDlgItem (hDlg, IDD_TZ_TIMEZONES);
                
                    iTimeZone = SendMessage (hwndCB, CB_GETCURSEL, 0, 0L);
                
                    if (iTimeZone != CB_ERR)
                    {
                        ptzi = (PAPPLET_TIME_ZONE_INFORMATION) SendMessage (
                                                              hwndCB,
                                                              CB_GETITEMDATA,
                                                              (WPARAM)iTimeZone,
                                                              0L);
                
                        hwndCB = GetDlgItem (hDlg, IDD_TZ_DAYLIGHT);
                        
                        //  Gray Checkbox if this TimeZone doesn't allow
                        //  Daylight Saving Time
                        
                        if (ptzi->StandardDate.wMonth == 0)
                        {
                            CheckDlgButton (hDlg, IDD_TZ_DAYLIGHT, 0);
                            EnableWindow (hwndCB, FALSE);
                            iButtonChecked = 0;
                        }
                        else
                        {
                            EnableWindow (hwndCB, TRUE);
                            CheckDlgButton (hDlg, IDD_TZ_DAYLIGHT, 1);
                            iButtonChecked = 1;
                        }

#ifdef SHOW_NEW_TIME
                        if (bSetup)
                            SetTheTimezone (hDlg, iButtonChecked, ptzi);
                        else 
                            SetTheVirtualTimezone (hDlg, iButtonChecked, ptzi);
#endif  // SHOW_NEW_TIME
                    }
                    break;
                }
                
                case PUSH_CANCEL:
                    SelectedTimeZone = NULL;

                    SetFocus (GetDlgItem (hDlg, LOWORD(wParam)));

                    hwndCB = GetDlgItem (hDlg, IDD_TZ_TIMEZONES);

                    GetDateTime ();
                
                    for (i = 0; i < 6; i++)
                        wDateTime[i] -= wDeltaDateTime[i];
                    SetDateTime ();

#ifdef SHOW_NEW_TIME
                    //
                    //  Restore the original TimeZone and DST state.
                    //

                    SetTheTimezone (hDlg, iOrigButtonChecked, ptziOriginal);
#endif  // SHOW_NEW_TIME

                    goto KillTheTimer;


                case PUSH_OK:
                    HourGlass (TRUE);

                    SelectedTimeZone = NULL;

                    hwndCB = GetDlgItem (hDlg, IDD_TZ_TIMEZONES);
                
                    // Get DST selection
                    iButtonChecked = IsDlgButtonChecked (hDlg, IDD_TZ_DAYLIGHT);
                
                    // A disabled checkbox is same as "No DST" selection
                    if (iButtonChecked != 1)
                        iButtonChecked = 0;
                
                    // Find out which listbox item was selected
                    iTimeZone = SendMessage (hwndCB, CB_GETCURSEL, 0, 0L);

                    if (iTimeZone != CB_ERR)
                    {
                        ptzi = (PAPPLET_TIME_ZONE_INFORMATION) SendMessage (
                                                              hwndCB,
                                                              CB_GETITEMDATA,
                                                              (WPARAM)iTimeZone,
                                                              0L);
                    }
                    else
                    {
                        ptzi = NULL;
                    }

                    SetTheTimezone (hDlg, iButtonChecked, ptzi);

                    SendMessage ((HWND) -1, WM_TIMECHANGE, 0L, 0L);
                
KillTheTimer:   
                    //  No more updates to local time - use what we have
                    KillTimer (hDlg, SECOND);
                
                    //////////////////////////////////////////////////////////////////
                    //  Free TimeZone structs and all strings associated with them
                    //////////////////////////////////////////////////////////////////
                
                    for (j = 0; j < NumTimeZones; j++)
                    {
                        ptzi = (PAPPLET_TIME_ZONE_INFORMATION) SendMessage (
                                                                  hwndCB,
                                                                  CB_GETITEMDATA,
                                                                  (WPARAM)j,
                                                                  0L);
                
                        if (ptzi)
                            FreeMem (ptzi, sizeof(APPLET_TIME_ZONE_INFORMATION));
                    }
                
#ifdef  TZMAP  
                    if (hpalTZmap)
                        DeleteObject (hpalTZmap);
#endif  //  TZMAP
                
                    HourGlass (FALSE);
                    EndDialog (hDlg, 0L);
                    break;
            }
            break;
        
        default:
            if (message == wHelpMessage)
            {
DoHelp: 
                CPHelp (hDlg);
                return TRUE;
            }
            else
                return FALSE;
            break;
    }

    return (TRUE);
}

//////////////////////////////////////////////////////////////////////////////
//
// InitTimeZone
//
//  This function initializes everything to do with the Timezones
//
//////////////////////////////////////////////////////////////////////////////

BOOL InitTimeZone (HWND hDlg)
{
    TIME_ZONE_INFORMATION CurrentTzi;

    int    j;
    DWORD  TimeZoneId;
    int    CurrentTziIndex;
    HWND   hwndCB;
    PAPPLET_TIME_ZONE_INFORMATION ptzi;


#ifdef TZMAP
    //  Initilize drawing of world bitmap and timezones
    SetupWorld (hDlg);
#endif  //  TZMAP

    //////////////////////////////////////////////////////////////////////////
    // Init "Auto Adjust for DST" checkbox
    //////////////////////////////////////////////////////////////////////////

    TimeZoneId = GetTimeZoneInformation (&CurrentTzi);

    //  Assume "Auto Adjust for DST"
    CheckDlgButton (hDlg, IDD_TZ_DAYLIGHT, 1);

    //  If wMonth is 0, then this TimeZone does not support DST
    //  else if all fields between StandardDate and DaylightDate are equal
    //  then we assume that there is no "Daylight Saving Time" selected

    if ((CurrentTzi.StandardDate.wMonth == 0) ||
        (CurrentTzi.DaylightDate.wMonth == 0))
    {
        CheckDlgButton (hDlg, IDD_TZ_DAYLIGHT, 0);
        EnableWindow (GetDlgItem (hDlg, IDD_TZ_DAYLIGHT), FALSE);
    }
    else if ((CurrentTzi.StandardDate.wYear == CurrentTzi.DaylightDate.wYear) &&
             (CurrentTzi.StandardDate.wMonth == CurrentTzi.DaylightDate.wMonth) &&
             (CurrentTzi.StandardDate.wDayOfWeek == CurrentTzi.DaylightDate.wDayOfWeek) &&
             (CurrentTzi.StandardDate.wDay == CurrentTzi.DaylightDate.wDay) &&
             (CurrentTzi.StandardDate.wHour == CurrentTzi.DaylightDate.wHour) &&
             (CurrentTzi.StandardDate.wMinute == CurrentTzi.DaylightDate.wMinute) &&
             (CurrentTzi.StandardDate.wSecond == CurrentTzi.DaylightDate.wSecond) &&
             (CurrentTzi.StandardDate.wMilliseconds == CurrentTzi.DaylightDate.wMilliseconds))
        CheckDlgButton (hDlg, IDD_TZ_DAYLIGHT, 0);

    //////////////////////////////////////////////////////////////////////////
    //  Get the TimeZones from registry
    //////////////////////////////////////////////////////////////////////////

    if (!GetTimeZoneRes (hDlg))
    {
        MyMessageBox(hDlg, DATE+12, INITS+1, MB_OK|MB_ICONINFORMATION);
        return FALSE;
    }

    //////////////////////////////////////////////////////////////////////////
    //  Find the current one
    //////////////////////////////////////////////////////////////////////////

    hwndCB = GetDlgItem(hDlg, IDD_TZ_TIMEZONES);

    if (TimeZoneId != 0xffffffff)
    {
        CurrentTziIndex = NumTimeZones;
    }
    else
    {
        CurrentTziIndex = 0;
        goto QuickOut;
    }

    for (j = 0; j < NumTimeZones; j++)
    {
        ptzi = (PAPPLET_TIME_ZONE_INFORMATION) SendMessage (hwndCB,
                                                            CB_GETITEMDATA,
                                                            (WPARAM)j,
                                                            0L);
    
        if (!wcscmp (ptzi->szStandardName, CurrentTzi.StandardName))
        {
            CurrentTziIndex = j;
            break;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // hilight the current one
    //////////////////////////////////////////////////////////////////////////

    if (CurrentTziIndex == NumTimeZones)
    {
        for (j = 0; j < NumTimeZones; j++)
        {
            ptzi = (PAPPLET_TIME_ZONE_INFORMATION) SendMessage (hwndCB,
                                                                CB_GETITEMDATA,
                                                                (WPARAM)j,
                                                                0L);
    
            if (!wcscmp(ptzi->szDaylightName, CurrentTzi.DaylightName))
            {
                CurrentTziIndex = j;
            }
        }
    }

    if (CurrentTziIndex == NumTimeZones)
    {
        CurrentTziIndex = 0;
    }

QuickOut:

    SendMessage (hwndCB, CB_SETCURSEL, CurrentTziIndex, (LPARAM)NULL);
    SetFocus (hwndCB);

    //
    //  Save the original values
    //

    iOriginalTimeZone = CurrentTziIndex;

    ptziOriginal = (PAPPLET_TIME_ZONE_INFORMATION) SendMessage (hwndCB,
                                                                CB_GETITEMDATA,
                                                                (WPARAM)iOriginalTimeZone,
                                                                0L);
    //
    // Save initial DST selection
    //

    iOrigButtonChecked = IsDlgButtonChecked (hDlg, IDD_TZ_DAYLIGHT);
    
    //
    //  A disabled checkbox is same as "No DST" selection
    //

    if (iOrigButtonChecked != 1)
        iOrigButtonChecked = 0;

    return TRUE;
}


//////////////////////////////////////////////////////////////////////////////
//
// GetTimeZoneRes - REGISTRY VERSION
//
//  Retrieve Time Zone information from registry
//
//////////////////////////////////////////////////////////////////////////////

BOOL GetTimeZoneRes (HWND hDlg)
{
    int    i, j, k, nEntries = 0;
    DWORD  dwSize, dwBufz;
    DWORD  dwType;
    HKEY   hkey, hkeySub;
    TCHAR  szTimeZone[80];
    DWORD  dwValue;

    FILETIME  ftReg;

    PAPPLET_TIME_ZONE_INFORMATION ptzi;

    
    Tzi = NULL;

    //////////////////////////////////////////////////////////////////////////
    //
    //  Enumerate the subkeys under the Time Zone key and read data from
    //  each subkey to fill in APPLET_TIME_ZONE_INFO structs
    //
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////
    //  Get Time Zones from registry
    //////////////////////////////////////////////////////////////////////

    hkey = NULL;

    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE,        // Root key
                      pszTimezones,              // Subkey to open
                      0L,                        // Reserved
                      KEY_READ,                  // SAM
                      &hkey)                     // return handle
            != ERROR_SUCCESS)
    {
        return FALSE;
    }


    //////////////////////////////////////////////////////////////////////
    //  Special Case for when this routine is called from DllInitialize
    //  to do and "/Install=SomeTimeZoneName"
    //
    //  In this case I return an array of Time Zone structs, which we
    //  allocate here.
    //////////////////////////////////////////////////////////////////////

    if (hDlg == NULL)
    {
        //  Number of subkeys is best estimate of number of Time Zones

        dwValue = 0;
        dwBufz = CharSizeOf(szTimeZone);

        //  Get number of subkeys
        if (RegQueryInfoKey (hkey,        // handle of key to query
                             szTimeZone,  // ptr class string
                             &dwBufz,     // ptr size class string buffer
                             NULL,        // reserved
                             &dwValue,    // ptr number of subkeys
                             &dwBufz,     // ptr longest subkey name length
                             &dwBufz,     // ptr longest class string length
                             &dwBufz,     // ptr number of value entries
                             &dwBufz,     // ptr longest value name length
                             &dwBufz,     // ptr longest value data length
                             &dwBufz,     // ptr security descriptor length
                             &ftReg)      // ptr last write time
                != ERROR_SUCCESS)
        {
            RegCloseKey (hkey);
            return FALSE;
        }

        k = dwValue * sizeof(APPLET_TIME_ZONE_INFORMATION);
    
        if (!(Tzi = (PAPPLET_TIME_ZONE_INFORMATION) AllocMem (k)))
            return FALSE;
    
        ptzi = Tzi;
    }
    else
    {
        k = sizeof(APPLET_TIME_ZONE_INFORMATION);
        ptzi = NULL;
    }

    //////////////////////////////////////////////////////////////////////
    //  Enumerate all keys under Time Zones
    //////////////////////////////////////////////////////////////////////

    i = 0;

    while (RegEnumKey (hkey, i++, szTimeZone, CharSizeOf(szTimeZone))
                                             != ERROR_NO_MORE_ITEMS)
    {
        hkeySub = NULL;

        if (RegOpenKeyEx (hkey,                     // Root key
                          szTimeZone,               // Subkey to open
                          0L,                       // Reserved
                          KEY_READ,                 // SAM
                          &hkeySub)                 // return handle
                == ERROR_SUCCESS)
        {
            ///////////////////////////////////////////////////////////
            //  Allocate memory for this Time Zone struct
            ///////////////////////////////////////////////////////////

            if (hDlg)
                ptzi = (APPLET_TIME_ZONE_INFORMATION *) AllocMem (k);

            lstrcpy(ptzi->szRegKey, szTimeZone);

            ///////////////////////////////////////////////////////////
            //  Get Time Zone strings
            ///////////////////////////////////////////////////////////

            //  Get Time Zone Display Name
            dwSize = TZDISPLAYZ * sizeof(TCHAR);
            
            if ((RegQueryValueEx (hkeySub, pszTZDisplayName, 0L, &dwType,
                                  (LPBYTE) ptzi->szDisplayName, &dwSize))
                    != ERROR_SUCCESS)
            {
                goto TryNextSubKey;
            }

            if (dwType != REG_SZ)
                goto TryNextSubKey;


            //  Get Time Zone Standard Name

            //  NOTE:  Each subkey under the "Time Zones" key is the
            //         Standard Time Zone name so just copy it


            _tcsncpy (ptzi->szStandardName, szTimeZone, TZNAME_SIZE);

            
            //  OPTIONAL - Get Time Zone Daylight Name
            dwSize = TZNAME_SIZE * sizeof(TCHAR);
            
            if ((RegQueryValueEx (hkeySub, pszTZDaylightName, 0L, &dwType,
                                  (LPBYTE) ptzi->szDaylightName, &dwSize))
                    != ERROR_SUCCESS)
            {
                //  Set it to null string
                ptzi->szDaylightName[0] = TEXT('\0');
            }

            ///////////////////////////////////////////////////////////
            //  Get Binary information - Bias values, SYSTEMTIME structs
            //  
            //  NOTE: The binary information is stored in the same
            //        format as the last 5 values in our structure
            //        APPLET_TIME_ZONE_INFORMATION.
            //  
            ///////////////////////////////////////////////////////////

            dwSize = 3 * sizeof(LONG) + 2 * sizeof(SYSTEMTIME);
            
            if ((RegQueryValueEx (hkeySub, pszTZI, 0L, &dwType,
                             (LPBYTE) &ptzi->Bias, &dwSize))
                    != ERROR_SUCCESS)
            {
                goto TryNextSubKey;
            }


            ///////////////////////////////////////////////////////////
            //  Put Time Zone into Combobox
            ///////////////////////////////////////////////////////////

            if (hDlg)
            {
                if ((j = (int)SendDlgItemMessage (hDlg,
                                                  IDD_TZ_TIMEZONES,
                                                  CB_ADDSTRING,
                                                  (WPARAM) 0,
                                                  (LPARAM) ptzi->szDisplayName)) >= 0)
                {
                    SendDlgItemMessage (hDlg,
                                        IDD_TZ_TIMEZONES,
                                        CB_SETITEMDATA,
                                        j,
                                        (LPARAM) ptzi);
                }
            }
            else
            {
                //  Simply create an array of Time Zones
                ptzi++;
            }

            nEntries++;

        }
        else
        {
TryNextSubKey:
            if (hDlg && ptzi)
                FreeMem (ptzi, sizeof(APPLET_TIME_ZONE_INFORMATION));
        }

        RegCloseKey (hkeySub);

    }

    RegCloseKey (hkey);

    NumTimeZones = nEntries;

    return TRUE;
}


#ifdef SHOW_NEW_TIME
//////////////////////////////////////////////////////////////////////////////
//
//  SetTheVirtualTimezone
//
//  Apply the User's timezone selection based on Daylight Saving option. Sets
//  up global TimeZone struct to be used by GetDateTime() routine.  This TZ is
//  used by the SystemTimeToTZspecificLocalTime() api to get what the time
//  would be, if the user actually hit OK in the Date/Time dialog box.  It
//  does not actually change the System TimeZone or time settings.
//
//////////////////////////////////////////////////////////////////////////////

VOID SetTheVirtualTimezone (HWND hDlg, int DaylightOption, PAPPLET_TIME_ZONE_INFORMATION tzi)
{
    if (tzi == NULL)
    {
        SelectedTimeZone = NULL;
        return;
    }

    TimeZone.Bias = tzi->Bias;

    //  Automatically turn off Daylight Option if this TimeZone doesn't allow it
    if (tzi->StandardDate.wMonth == 0)
        DaylightOption = 0;

    if (DaylightOption == 0)
    {
        //  STANDARDONLY:
        TimeZone.StandardBias = tzi->StandardBias;
        TimeZone.DaylightBias = tzi->StandardBias;
        TimeZone.StandardDate = tzi->StandardDate;
        TimeZone.DaylightDate = tzi->StandardDate;
        lstrcpy(TimeZone.StandardName, tzi->szStandardName);
        lstrcpy(TimeZone.DaylightName, tzi->szStandardName);
     }
     else
     {
        //  Automatically adjust for Daylight Saving Time
        TimeZone.StandardBias = tzi->StandardBias;
        TimeZone.DaylightBias = tzi->DaylightBias;
        TimeZone.StandardDate = tzi->StandardDate;
        TimeZone.DaylightDate = tzi->DaylightDate;
        lstrcpy(TimeZone.StandardName, tzi->szStandardName);
        lstrcpy(TimeZone.DaylightName, tzi->szDaylightName);
     }

    SelectedTimeZone = &TimeZone;
}
#endif  // SHOW_NEW_TIME

//////////////////////////////////////////////////////////////////////////////
//
//  SetTheTimezone
//
//  Apply the User's timezone selection based on Daylight Saving option
//
//////////////////////////////////////////////////////////////////////////////

VOID SetTheTimezone (HWND hDlg, int DaylightOption, PAPPLET_TIME_ZONE_INFORMATION ptzi)
{
    TIME_ZONE_INFORMATION Info;

    if (!ptzi)
        return;

    Info.Bias = ptzi->Bias;

    //  Automatically turn off Daylight Option if this TimeZone doesn't allow it
    if (ptzi->StandardDate.wMonth == 0)
        DaylightOption = 0;

    if (DaylightOption == 0)
    {
        //  STANDARDONLY:
        Info.StandardBias = ptzi->StandardBias;
        Info.DaylightBias = ptzi->StandardBias;
        Info.StandardDate = ptzi->StandardDate;
        Info.DaylightDate = ptzi->StandardDate;
        lstrcpy(Info.StandardName, ptzi->szStandardName);
        lstrcpy(Info.DaylightName, ptzi->szStandardName);
     }
     else
     {
        //  Automatically adjust for Daylight Saving Time
        Info.StandardBias = ptzi->StandardBias;
        Info.DaylightBias = ptzi->DaylightBias;
        Info.StandardDate = ptzi->StandardDate;
        Info.DaylightDate = ptzi->DaylightDate;
        lstrcpy(Info.StandardName, ptzi->szStandardName);
        lstrcpy(Info.DaylightName, ptzi->szDaylightName);
     }

    //  If running under "Setup", setting Timezone info is not allowed
    //  to change system date and time
    if (bSetup)
        GetDateTime();

    if (!SetTimeZoneInformation (&Info))
    {
        if (hDlg)
            MyMessageBox(hDlg, ERRORS+15, INITS+1, MB_OK|MB_ICONINFORMATION);
    }

    if (bSetup)
        SetDateTime();
}


//////////////////////////////////////////////////////////////////////////////
//
// CentreWindow
//
// Purpose : Positions a window so that it is centred in its parent
//
// History:
// 12-09-91 Davidc       Created.
//
//////////////////////////////////////////////////////////////////////////////

VOID CentreWindow(HWND hwnd)
{
    RECT    rect;
    RECT    rectParent;
    HWND    hwndParent;
    LONG    dx, dy;
    LONG    dxParent, dyParent;
    LONG    Style;

    // Get window rect
    GetWindowRect(hwnd, &rect);

    dx = rect.right - rect.left;
    dy = rect.bottom - rect.top;

    // Get parent rect
    Style = GetWindowLong(hwnd, GWL_STYLE);
    if ((Style & WS_CHILD) == 0) {
        hwndParent = GetDesktopWindow();
    } else {
        hwndParent = GetParent(hwnd);
        if (hwndParent == NULL) {
            hwndParent = GetDesktopWindow();
        }
    }
    GetWindowRect(hwndParent, &rectParent);

    dxParent = rectParent.right - rectParent.left;
    dyParent = rectParent.bottom - rectParent.top;

    // Centre the child in the parent
    rect.left = (dxParent - dx) / 2;
    rect.top  = (dyParent - dy) / 3;

    // Move the child into position
    SetWindowPos(hwnd, NULL, rect.left, rect.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

    SetForegroundWindow(hwnd);
}


#ifdef TZMAP
//////////////////////////////////////////////////////////////////////////////
//
// SetupWorld
//
//  This function retrieves the world bitmap from the resources, draws it
//  with stretchblt and halftoning into the dialog frame area, scales the
//  Timezone polygon region coordinates to match the bitmaps stretched size
//  and sets up the gross "hit test accelerator x-coordinate array" for use
//  in fast hit-testing to determine user selection.
//
//  NOTE: This function must also create an offscreen bitmap the same size
//        as the on-screen STATIC area that holds the world.  We should
//        just do the HALF-TONE of the source bitmap into it once at INIT
//        time, and then just BITBLT from this memory bitmap to the screen
//        and then highlight the selected region
//
//////////////////////////////////////////////////////////////////////////////

BOOL SetupWorld (HWND hWnd)
{
    HDC         hdc;
    HANDLE      h, hRes;
    LPBYTE      lpBits;
    int         i;
    RECT        rc;

    LPBITMAPINFOHEADER  lpBitmapInfo;

    h    = FindResource (hModule, MAKEINTRESOURCE(WORLD), RT_BITMAP);
    hRes = LoadResource (hModule, h);

    /* Lock the bitmap and get a pointer to the color table. */
    lpBitmapInfo = (LPBITMAPINFOHEADER)LockResource(hRes);

    if (!lpBitmapInfo)
        return FALSE;

//    UnlockResource(hRes);

    /* First skip over the header structure */
    lpBits = (LPBYTE)(lpBitmapInfo + 1);

    /* Skip the color table entries, if any */
    lpBits += (1 << (lpBitmapInfo->biBitCount)) * sizeof(RGBQUAD);

    /* Create a color bitmap compatible with the display device */
    hdc = GetDC(NULL);

///////////////////////////////////////////////////////////////////////////
// Create another hDC compatible with the screen and make a bitmap
// the size of the static control - WORLD
// Then halftone the WORLD bitmap into it
///////////////////////////////////////////////////////////////////////////

    //  Stretchblt with halftoning, the source bitmap from our resources
    //  into a local (display compatible) memory bitmap
    //
    //  This is a speed hack to allow faster redrawing of display

    GetClientRect (GetDlgItem (hWnd, IDD_TZ_WORLD), &rc);

    if (hdcTZmap = CreateCompatibleDC (hdc))
    {
#ifdef USE_HALFTONE
        //  Create a halftone palette for screen compatible memory dc
        hpalTZmap = CreateHalftonePalette (hdcTZmap);
        SelectPalette (hdcTZmap, hpalTZmap, FALSE);
        RealizePalette (hdcTZmap);
#endif  //  USE_HALFTONE

        if (hbmTZmap = CreateCompatibleBitmap (hdc, rc.right, rc.bottom))
        {
#ifdef  USE_HALFTONE

            SetStretchBltMode (hdcTZmap, HALFTONE);

#else   //  USE_HALFTONE

            SetStretchBltMode (hdcTZmap, COLORONCOLOR);

#endif  //  USE_HALFTONE

            hbmTZDefault = SelectObject (hdcTZmap, hbmTZmap);
            StretchDIBits (hdcTZmap, 0, 0, rc.right, rc.bottom,
                           0, 0, lpBitmapInfo->biWidth, lpBitmapInfo->biHeight,
                           lpBits, lpBitmapInfo, DIB_RGB_COLORS, SRCCOPY);
        }
        else
            return FALSE;
    }

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//  Discard bitmap and DC from resources - not needed now!!
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

    ReleaseDC (NULL, hdc);

    GlobalUnlock (hRes);
    FreeResource (hRes);

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
//
// PaintWorld
//
//  This function paints the World bitmap and any highlighted timezone
//  regions using stretchblt and halftoning into the dialog frame area
//  from the memory bitmap.
//
//////////////////////////////////////////////////////////////////////////////

LONG PaintWorld (HWND hwndWorld, HDC hdcWorld)
{
    RECT        rect;

    GetClientRect (hwndWorld, &rect);

//  Performance hack??? - Check the paint rect against our hwndWorld client
//  rect to see if we need to do any painting at all

#ifdef USE_HALFTONE
    SelectPalette (hdcWorld, hpalTZmap, FALSE);
    RealizePalette (hdcWorld);
#endif  //  USE_HALFTONE

    BitBlt (hdcWorld, 0, 0, rect.right, rect.bottom, hdcTZmap, 0, 0, SRCCOPY);

// FIX FIX FIX - determine which timezone REGION is selected based on current
// user TimeZone selection and invert it

    //  Clean up

    return 0L;
}

#ifdef WORLDPAINT

//////////////////////////////////////////////////////////////////////////////
//
// SetupWorld
//
//  This function retrieves the world bitmap from the resources, draws it
//  with stretchblt and halftoning into the dialog frame area, scales the
//  Timezone polygon region coordinates to match the bitmaps stretched size
//  and sets up the gross "hit test accelerator x-coordinate array" for use
//  in fast hit-testing to determine user selection.
//
//  NOTE: This function must also create an offscreen bitmap the same size
//        as the on-screen STATIC area that holds the world.  We should
//        just do the HALF-TONE of the source bitmap into it once at INIT
//        time, and then just BITBLT from this memory bitmap to the screen
//        and then highlight the selected region
//
//////////////////////////////////////////////////////////////////////////////

BOOL SetupWorld (HWND hWnd)
{
    HDC         hdc;
    HANDLE      h, hRes;
    LPBYTE      lpBits;
    int         i;
    RECT        rc;

    LPBITMAPINFOHEADER  lpBitmapInfo;

    h    = FindResource (hModule, (LPTSTR) MAKEINTRESOURCE(WORLD), RT_BITMAP);
    hRes = LoadResource (hModule, h);

    /* Lock the bitmap and get a pointer to the color table. */
    lpBitmapInfo = (LPBITMAPINFOHEADER)LockResource(hRes);

    if (!lpBitmapInfo)
        return FALSE;

//    UnlockResource(hRes);

    /* First skip over the header structure */
    lpBits = (LPBYTE)(lpBitmapInfo + 1);

    /* Skip the color table entries, if any */
    lpBits += (1 << (lpBitmapInfo->biBitCount)) * sizeof(RGBQUAD);

    /* Create a color bitmap compatible with the display device */
    hdc = GetDC(NULL);

    if (hdcMem = CreateCompatibleDC (hdc))
    {
        if (hbmBitmaps = CreateDIBitmap (hdc, lpBitmapInfo, (DWORD)CBM_INIT,
                         lpBits, (LPBITMAPINFO)lpBitmapInfo, DIB_RGB_COLORS))
            hbmDefault = SelectObject (hdcMem, hbmBitmaps);
    }

    //  Save the size of the source "World" bitmap

    iWidth  = lpBitmapInfo->biWidth;
    iHeight = lpBitmapInfo->biHeight;

///////////////////////////////////////////////////////////////////////////
// Create another hDC compatible with the screen and make a bitmap
// the size of the static control - WORLD
// Then halftone the WORLD bitmap into it
///////////////////////////////////////////////////////////////////////////

    //  Stretchblt with halftoning, the source bitmap from our resources
    //  into a local (display compatible) memory bitmap
    //
    //  This is a speed hack to allow faster redrawing of display

    GetClientRect (GetDlgItem (hWnd, IDD_TZ_WORLD), &rc);

    if (hdcTZmap = CreateCompatibleDC (hdc))
    {
        if (hbmTZmap = CreateCompatibleBitmap (hdc, rc.right, rc.bottom))
            hbmTZDefault = SelectObject (hdcTZmap, hbmTZmap);
    }

    SetStretchBltMode (hdcTZmap, HALFTONE);

    StretchBlt (hdcTZmap, 0, 0, rc.right, rc.bottom,
                hdcMem,   0, 0, iWidth,   iHeight,
                SRCCOPY);

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//  Discard bitmap and DC from resources - not needed now!!
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

    ReleaseDC (NULL, hdc);

    GlobalUnlock (hRes);
    FreeResource (hRes);

    return TRUE;
}

LONG PaintWorld (HWND hwndWorld, HDC hdcWorld)
{
    RECT        rect;

    GetClientRect (hwndWorld, &rect);

//  Performance hack??? - Check the paint rect against our hwndWorld client
//  rect to see if we need to do any painting at all

    SetStretchBltMode (hdcWorld, HALFTONE);

    StretchBlt (hdcWorld, 0, 0, rect.right, rect.bottom,
                hdcMem,   0, 0, iWidth,     iHeight,
                SRCCOPY);

// FIX FIX FIX - determine which timezone REGION is selected based on current
// user TimeZone selection and invert it

    //  Clean up

    return 0L;
}

LONG PaintWorld (HWND hWnd)
{
    HDC         hDC, hdcWorld;
    HWND        hwndWorld;
    int         i;
    PAINTSTRUCT ps;
    RECT        rect;


    hDC = BeginPaint (hWnd, &ps);

    hwndWorld = GetDlgItem (hWnd, IDD_TZ_WORLD);
    hdcWorld = GetDC (hwndWorld);

    GetClientRect (hwndWorld, &rect);

//  Performance hack??? - Check the paint rect against our hwndWorld client
//  rect to see if we need to do any painting at all

    SetStretchBltMode (hdcWorld, HALFTONE);

    StretchBlt (hdcWorld, 0, 0, rect.right, rect.bottom,
                hdcMem,   0, 0, iWidth,     iHeight,
                SRCCOPY);

// FIX FIX FIX - determine which timezone REGION is selected based on current
// user TimeZone selection and invert it

    ReleaseDC(hwndWorld, hdcWorld);

    //  Clean up

    return EndPaint(hWnd, &ps);
}
#endif  //  WORLDPAINT
#endif  //  TZMAP


