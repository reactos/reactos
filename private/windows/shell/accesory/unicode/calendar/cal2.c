/*
 *   Windows Calendar
 *   Copyright (c) 1985 by Microsoft Corporation, all rights reserved.
 *   Written by Mark L. Chamberlin, consultant to Microsoft.
 *
 ****** cal2.c
 *
 */



/* Get rid of more stuff from windows.h */
#define NOWINSTYLES
#define NOVIRTUALKEYCODES
#define NOSYSMETRICS
#define NOMENUS
#define NOKEYSTATE
#define NOSYSCOMMANDS
#define NOMEMMGR
#define NOSOUND
#define NOCOMM

#include <time.h>
#include "cal.h"
//dee#include "date.h"


HDC vhDCTemp;            /* Save code by not having to pass the HDC on each
                            call to DrawArrow and DrawArrowBorder. */

#if defined (JAPAN)||defined(KOREA) //  added 01 Jun. 1992  by Hiraisi, JeeP 10/13/92
INT CalParseTimeString(DOSTIME *, TCHAR *);
VOID ReplaceTimeString(TCHAR *, TCHAR *);
extern BOOL fTimePrefix;
#endif


/**** CalPaint ****/

VOID APIENTRY CalPaint (
     HWND hwnd,
     HDC  hDC)
{
     register D3  *pd3;

     if (hwnd == vhwnd1)
          PatBlt (hDC, 0, vycoNotesBox, vcxWnd2A , vcyBorder, PATCOPY);

     if (hwnd == vhwnd2A)
     {
          DispTime (hDC);

          /* Assume we are in month mode so we will display the
             current date.
           */
          pd3 = &vd3Cur;

          /* Set up global for DrawArrow and DrawArrowBorder. */
          vhDCTemp = hDC;

#ifdef BUG_8560
          /* We want the arrows to have the border color. */
          SetTextColor (hDC, GetSysColor (COLOR_WINDOWFRAME));

          /* Draw left arrow. */
          DrawArrow (vhbmLeftArrow, vxcoLeftArrowFirst);

          /* Draw border at left end of left arrow. */
          DrawArrowBorder (vxcoLeftArrowFirst);

          /* Draw border between left and right arrows. */
          DrawArrowBorder (vxcoLeftArrowMax);

          /* Draw right arrow. */
          DrawArrow (vhbmRightArrow, vxcoRightArrowFirst);

          /* Draw border to right of right arrow. */
          DrawArrowBorder (vxcoRightArrowMax - vcxBorder);
#endif

          /* Set colors back to defaults. */
          SetDefaultColors (hDC);

          /* Want to display the page date, not the current date. */
          pd3 = &vd3Sel;

          DispDate (hDC, pd3);
          return;
     }

     if (hwnd == vhwnd2B)
     {
          PatBlt (hDC, 0, 0, vcxWnd2B+ vcxVScrollBar, vcyBorder, PATCOPY);

          if (vfDayMode)
               DayPaint (hDC);
          else
               {
               PaintMonthGrid (hDC);
               PaintMonth (hDC);
               }
     }
}



#ifdef  BUG_8560
******************* The following are no longer required ********************

/**** DrawArrow ****/

VOID APIENTRY DrawArrow (
     HBITMAP hbm,
     INT     xco)
     {
     SelectObject (vhDCMemory, hbm);
     BitBlt (vhDCTemp, xco, vcyBorder, vcxHScrollBar, vcyHScrollBar,
             vhDCMemory, 0, 0, SRCCOPY);
     }




/**** DrawArrowBorder ****/

VOID APIENTRY DrawArrowBorder (INT  xco)
     {
     PatBlt (vhDCTemp, xco, 0, vcxBorder, vcyWnd2A, PATCOPY);
     }
************************ No longer required ********************************
#endif




/**** DispTime - Display the current time. ****/

VOID APIENTRY FAR DispTime (HDC  hDC)
{
     TCHAR sz [CCHTIMESZ + 2];   /* added 2 for spaces.  19 Sep 89 Clark Cyr */

     /* Erase background */
//     TextOut (hDC, vcxFont, vcyExtLead, TEXT("              "), 12);

     /* Convert the time into an ASCII string. */
     GetTimeSz (vftCur.tm, sz);
     lstrcat(sz, TEXT("  "));

     /* Output the time. */
     TextOut (hDC, vcxFont, vcyExtLead, sz, lstrlen(sz));
}




/**** GetTimeSz - convert the time into a zero terminated ASCII string. ****/
INT APIENTRY GetTimeSz (
     TM   tm,            /* The time to convert. */
     TCHAR *sz)          /* pointer to the buffer to receive the string -
                            the caller should allocate CCHTIMESZ chars. */
{
    DOSTIME dt;

    dt.minutes = tm % 60;
    dt.hour = tm / 60;

#if defined (JAPAN)||defined(KOREA) //  added 29 May 1992  by Hiraisi, JeeP 10/12/92
    if (fTimePrefix)
    {
        TCHAR sz2 [CCHTIMESZ + 6 + 2];

        GetTimeString (&dt, sz);
        ReplaceTimeString (sz, sz2);
        lstrcpy (sz, sz2);
    }
    return (lstrlen(sz));
#else
    return (GetTimeString(&dt, sz));
#endif
}



/****  ByteTo2Digs - convert byte to 2 decimal ASCII digits. ****/

VOID APIENTRY ByteTo2Digs (
     BYTE  b,             /* The byte to convert from binary to ASCII. */
     TCHAR *pch)          /* Pointer to output buffer (must be at least 2
                            chars long.
                          */
{
     *pch++ = b / 10 + TEXT('0');
     *pch = b % 10 + TEXT('0');
}



/**** DispDate - Display date in Wnd2A. */
VOID APIENTRY FAR DispDate (
     HDC  hDC,
     D3   *pd3)
{
     RECT   rc;
     HBRUSH hbr;

     TCHAR sz [CCHDATEDISP];

     /* Convert the current date into an ASCII string. */
     GetDateDisp (pd3, sz);

     /* Erase the background */
     GetClientRect(vhwnd2A, (LPRECT)&rc);
     rc.left = vxcoDate;
     if (hbr = CreateSolidBrush(GetSysColor(COLOR_WINDOW)))
     {
         FillRect(hDC, (LPRECT)&rc, hbr);
         DeleteObject(hbr);
     }
     else
         FillRect(hDC, (LPRECT)&rc, GetStockObject(WHITE_BRUSH));

     /* Output the date. Use transparent mode so we don't erase the background
      * color.
      */
     SetBkMode(hDC,TRANSPARENT);
     TextOut (hDC, vxcoDate+2, vcyExtLead, sz, lstrlen (sz));
     SetBkMode(hDC,OPAQUE);
}




/**** GetDateDisp - convert the date to an string of the form dependent on
      the current locale.
****/

VOID APIENTRY GetDateDisp (
    D3   *pd3,
    TCHAR *sz)
{
    DOSDATE dd;

    dd.dayofweek = 0xff;        /* so it'll calculate day of week */
    dd.month = pd3->wMonth + 1;
    dd.day = pd3->wDay + 1;
    dd.year = (WORD) pd3->wYear + 1980;

    GetDateString(&dd, sz, GDS_LONG | GDS_DAYOFWEEK);
}


/**** FillBuf - fill buffer with specified count of specified byte.
      Return a pointer to the buffer position following the filled bytes.
****/

TCHAR * APIENTRY FillBuf (
     TCHAR *pb,
     INT  cb,
     TCHAR b)
{
     while (cb--)
          *pb++ = b;

     return (pb);
}


/**** GetDashDateSel - convert the selected date to an ASCII string
      of the form: mm-dd-yyyy.  For example, "4-21-1985".
****/

VOID APIENTRY GetDashDateSel (TCHAR *sz)
{
    DOSDATE dd;

    dd.month = vd3Sel.wMonth + 1;
    dd.day = vd3Sel.wDay + 1;
    dd.year = (WORD) vd3Sel.wYear + 1980;
    GetDateString(&dd, sz, GDS_SHORT);
}



/**** FGetTmFromTimeSz
      The format for inputting the time is: [h]h[:mm][a|am|p|pm]
      And in Japan it may be [a|am|p|pm][h]h[:mm]
      In other words:
      - at least one digit must be used to specify the hour (even if it's 0)
      - the minutes are optional but if specified must be two digits preceded
        by a colon
      - the am/pm designation is optional and can be abbreviated by just a or p
      - The am/pm designation can use any combination of upper and lower case
      - If hours > 12 then OK if pm specified, but error if am specified.
      - if hours == 0 then OK if am specified, but error if pm specified.
      - If 1 <= hour <= 12 then default to am if no am/pm specification.
****/

INT  APIENTRY FGetTmFromTimeSz (
    TCHAR *sz,          /* INPUT - ASCII time string. */
    TM    *ptm)         /* OUTPUT - converted time - unchanged if we
                           return FALSE.
                         */
{
    DOSTIME dt;
    INT     iErr;

#if defined (JAPAN)||defined(KOREA) //  added 01 Jun. 1992  by Hiraisi, JeeP 10/12/92
    if (fTimePrefix)
    {
        if ((iErr = CalParseTimeString(&dt, sz)) == 0)
        {
            *ptm = dt.hour * 60 + dt.minutes;
            return(TRUE);
        }
    }
    else
    {
        if ((iErr = ParseTimeString(&dt, sz)) == 0)
        {
            *ptm = dt.hour * 60 + dt.minutes;
            return(TRUE);
        }
    }
#else
    if ((iErr = ParseTimeString(&dt, sz)) == 0)
    {
        *ptm = dt.hour * 60 + dt.minutes;
        return(TRUE);
    }
#endif

    return(iErr);
}



/**** SkipSpace - skip spaces in a sz. ****/

VOID APIENTRY SkipSpace (TCHAR **psz)
{
     while (**psz == TEXT(' '))
          (*psz)++;
}



/**** FD3FromDateSz
      Format supported: mm-dd-yyyy
      (Slashes (/) may be used instead of dashes.)
      If the year is in the range 0 through 99, it is assumed that the
      low order digits of 19yy have been specified.
****/

BOOL APIENTRY FD3FromDateSz (
    TCHAR *sz,          /* INPUT - ASCII date string. */
    D3    *pd3)         /* OUTPUT - converted date.  Unchanged if
                           we return FALSE.
                         */
{
    DOSDATE dd;
    INT     iErr;

    if ((iErr = ParseDateString(&dd, sz)) == 0)
        {
        pd3->wMonth = dd.month - 1;
        pd3->wDay = dd.day - 1;
        pd3->wYear = dd.year - 1980;
        }

    return(iErr);
}



/**** GetD3FromDt ****/

VOID APIENTRY GetD3FromDt (
     DT   dt,
     D3   *pd3)
{
     register INT  cDaysYear;
     register INT  i;
     INT  cDaysMonth;

     /* See how many 4 year periods are in it (366 for leap year, 3 * 365
        for the next 3 years.
     */
     pd3->wYear = 4 * (dt / 1461);
     dt = dt % 1461;

     /* Account for the individual years.  Again, the first year is
       a leap year, the next two are normal (only two since we already
       divided by groups of 4 years).
     */
     cDaysYear = 366;
     while ((INT)dt >= cDaysYear)
     {
          dt -= cDaysYear;
          pd3->wYear++;
          cDaysYear = 365;
     }

     /* Subtract out days of each month.  Note that we add one
        to the count of days in the month for February in a leap year.
     */
     for (i = MONTHJAN; (INT)dt >= (cDaysMonth = vrgcDaysMonth [i] +
      (cDaysYear == 366 && i == MONTHFEB ? 1 : 0)); i++)
          dt -= cDaysMonth;

     pd3->wMonth = i;

     /* Whatever's left is the offset into the month. */
     pd3->wDay = dt;
     }



/**** Set text of an edit ctl and then place selection at end. */
VOID APIENTRY SetEcText(
     HWND    hwnd,
     TCHAR *  sz)
     {
     WPARAM        iSelFirst;
     WPARAM        iSelLast;

     SendMessage (hwnd, WM_SETTEXT, 0, (LPARAM)sz);
     iSelFirst = iSelLast = (WPARAM) -1;
     SendMessage(hwnd, EM_SETSEL, iSelFirst, (LONG)iSelLast);
     }


#if defined (JAPAN)||defined(KOREA) // added 29 May 1992  by Hiraisi, JeeP 10/13/92
/*
 * This function replaces Time and TimePrefix.
 *   ex.) "10:00 AM" => "AM 10:00"
*/
VOID ReplaceTimeString(
        TCHAR *src,
        TCHAR *dst )
{
    INT  i;
    TCHAR *sz;

    sz = src;
    for (sz += 6; *sz != TEXT('\0'); sz++, dst++) {
        *dst = *sz;
    }
    *dst = TEXT(' ');
    dst++;
    sz = src;
    for (i = 0; i < 5; i++, dst++) {
        *dst = sz[i];
    }
    *dst = TEXT('\0');
}


/*
 * This function Parses TimeString.   ex.) "AM 10:00"
*/
INT CalParseTimeString(
        DOSTIME *pdt,
        TCHAR *pch )
{
    INT h, m;
    TCHAR *pchT;
    BOOL fPM = FALSE;

    /* skip leading spaces */
    while (*pch == TEXT(' '))
        pch++;

    /* Now look for match against AM or PM string */
    if (*pch == (TCHAR) 0)
        return(PD_ERRFORMAT);

     /* Upper case the string in PLACE */
    CharUpper (pch);
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

    pch += lstrlen(pchT);

    /* skip trailing spaces */
    while (*pch == TEXT(' '))
        pch++;

    if ((pch = Ascii2Int(pch, &h)) == NULL)
        return(PD_ERRFORMAT);
    if (*pch++ != Time.chSep)
        return(PD_ERRFORMAT);
    if ((pch = Ascii2Int(pch, &m)) == NULL)
        return(PD_ERRFORMAT);

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

#endif

