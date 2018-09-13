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

#include "cal.h"
#include <time.h>


HDC vhDCTemp;            /* Save code by not having to pass the HDC on each
                            call to DrawArrow and DrawArrowBorder. */


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
     CHAR sz [CCHTIMESZ + 2];   /* added 2 for spaces.  19 Sep 89 Clark Cyr */

     /* Convert the time into an ASCII string. */
     GetTimeSz (vftCur.tm, sz);
     lstrcat((LPSTR)sz, (LPSTR)"  ");

     /* Output the time. */
     TextOut (hDC, vcxFont, vcyExtLead, (LPSTR)sz,  lstrlen((LPSTR)sz));
     }




/**** GetTimeSz - convert the time into a zero terminated ASCII string. ****/
INT APIENTRY GetTimeSz (
     TM   tm,            /* The time to convert. */
     CHAR *sz)           /* pointer to the buffer to receive the string -
                            the caller should allocate CCHTIMESZ chars. */
     {
#ifndef NOCOMMON
    DOSTIME dt;

    dt.minutes = tm % 60;
    dt.hour = tm / 60;
    return(GetTimeString(&dt, sz, GTS_LEADINGZEROS | GTS_LEADINGSPACE));
#else
     WORD wHour;

     /* Put in the boiler plate. */
     lstrcpy (sz + 2, ":    ");

     wHour = tm / 60;

     if (!vfHour24)
          {
          lstrcpy (sz + 5, "am");
          if (wHour > 11)
               {
               /* Change to pm, and adjust down the hour. */
               *(sz + 5) = 'p';
               wHour -= 12;
               }
          /* Convert the 0 hour (midnight) to 12 (am is already selected). */
          if (wHour == 0)
               wHour = 12;
          }

     /* Convert the hours to ASCII. */
     ByteTo2Digs ((BYTE)wHour, sz);

     /* Change leading 0 to space if in 12 hour mode. */
     if (!vfHour24 && *sz == '0')
          *sz = ' ';

     /* Convert the minutes to ASCII. */
     ByteTo2Digs ((BYTE)(tm % 60), sz + 3);
#endif
     }



/****  ByteTo2Digs - convert byte to 2 decimal ASCII digits. ****/

VOID APIENTRY ByteTo2Digs (
     BYTE b,             /* The byte to convert from binary to ASCII. */
     CHAR *pch)          /* Pointer to output buffer (must be at least 2
                            chars long.
                          */
     {
     *pch++ = b / 10 + '0';
     *pch = b % 10 + '0';
     }



/**** DispDate - Display date in Wnd2A. */
VOID APIENTRY FAR DispDate (
     HDC  hDC,
     D3   *pd3)
     {
     RECT   rc;
     HBRUSH hbr;

     CHAR sz [CCHDATEDISP];

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

     SetBkMode(hDC,TRANSPARENT);
     /*	Output the date. Use transparent mode so we don't erase the background
      * color.
      */
     TextOut (hDC, vxcoDate+2, vcyExtLead, (LPSTR)sz, lstrlen ((LPSTR)sz));
     SetBkMode(hDC,OPAQUE);
     }




/**** GetDateDisp - convert the date to an ASCII string of the form:
      weekday, month, day, year.  For example, "Sunday, March 30, 1985".
****/

VOID APIENTRY GetDateDisp (
    D3   *pd3,
    CHAR *sz)
    {
    DOSDATE dd;

    dd.dayofweek = 0xff;        /* so it'll calculate day of week */
    dd.year = pd3->wYear + 1980;
    dd.month = pd3->wMonth + 1;
    dd.day = pd3->wDay + 1;

    GetLongDateString(&dd, sz, GDS_LONG | GDS_DAYOFWEEK);
    }


/**** FillBuf - fill buffer with specified count of specified byte.
      Return a pointer to the buffer position following the filled bytes.
****/

BYTE * APIENTRY FillBuf (
     BYTE *pb,
     INT  cb,
     BYTE b)
     {
     while (cb--)
          *pb++ = b;

     return (pb);
     }


#ifndef NOCOMMON
/**** WordToASCII - convert word to ASCII digits - return a pointer
      to the first character following the generated digits.
****/

CHAR * APIENTRY WordToASCII (
     WORD  w,                      /* Word to convert. */
     CHAR *pch,                    /* Pointer to output buffer. */
     BOOL  fLeadZero)              /* TRUE for leading zeroes,
                                      FALSE to suppress leading zeroes.
                                    */
     {
     WORD wPlace;
     WORD wDig;

     for (wPlace = 10000; wPlace > 0; wPlace /= 10)
          {
          wDig = w / wPlace;
          w %= wPlace;
          if (wDig != 0 || fLeadZero || wPlace == 1)
               {
               *pch++ = wDig + '0';

               /* After the first digit gets put down, we're no longer
                  going to see leading zeros.  Prevent additional zeroes
                  from being suppressed by setting fLeadZero to TRUE.
               */
               fLeadZero = TRUE;
               }
          }

     return (pch);
     }

#endif




/**** GetDashDateSel - convert the selected date to an ASCII string
      of the form: mm-dd-yyyy.  For example, "4-21-1985".
****/

VOID APIENTRY GetDashDateSel (CHAR *sz)
     {
#ifndef NOCOMMON
    DOSDATE dd;

    dd.month = vd3Sel.wMonth + 1;
    dd.day = vd3Sel.wDay + 1;
    dd.year = vd3Sel.wYear + 1980;
    GetDateString(&dd, sz, GDS_SHORT);
#else
     sz = WordToASCII ((WORD)(vd3Sel.wMonth + 1), sz, FALSE);
     *sz++ = '-';
     sz = WordToASCII ((WORD)(vd3Sel.wDay + 1), sz, FALSE);
     *sz++ = '-';
     *(WordToASCII ((WORD)(vd3Sel.wYear + 1980), sz, FALSE)) = '\0';
#endif
     }



/**** FGetTmFromTimeSz
      The format for inputting the time is: [h]h[:mm][a|am|p|pm]
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
    CHAR *sz,           /* INPUT - ASCII time string. */
    TM   *ptm)          /* OUTPUT - converted time - unchanged if we
                           return FALSE.
                         */
    {
    DOSTIME dt;
    INT     iErr;

    if ((iErr = ParseTimeString(&dt, sz)) == 0)
    {
        *ptm = dt.hour * 60 + dt.minutes;
        return(TRUE);
    }

    return(iErr);
    }



/**** SkipSpace - skip spaces in a sz. ****/

VOID APIENTRY SkipSpace (CHAR **psz)
     {
     while (**psz == ' ')
          (*psz)++;
     }

#ifdef NOCOMMON

/**** FGetWord - convert ASCII digits into a word
      in the range 0 to 65535 inclusive.
****/

BOOL APIENTRY FGetWord (
     CHAR **ppch,
     WORD *pw)
     {
     LONG l;
     CHAR ch;

     l = 0;

     /* Must see at least one digit. */
     if (!isdigit (**ppch))
          return (FALSE);

     while (isdigit (ch = **ppch))
          {
          l = l * 10 + (ch - '0');
          (*ppch)++;
          if (l > 65535)
               return (FALSE);
          }

     *pw = (WORD)l;
     return (TRUE);
     }

#endif



/**** ChUpperCase - convert from lower to upper case. ****/

#ifdef DISABLE
CHAR APIENTRY ChUpperCase (CHAR ch)
     {
     return (ch >= 'a' && ch <= 'z' ? ch - 'a' + 'A' : ch);
     }
#endif



/**** FD3FromDateSz
      Format supported: mm-dd-yyyy
      (Slashes (/) may be used instead of dashes.)
      If the year is in the range 0 through 99, it is assumed that the
      low order digits of 19yy have been specified.
****/

BOOL APIENTRY FD3FromDateSz (
    CHAR *sz,           /* INPUT - ASCII date string. */
    D3   *pd3)          /* OUTPUT - converted date.  Unchanged if
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
     pd3 -> wYear = 4 * (dt / 1461);
     dt = dt % 1461;

     /* Account for the individual years.  Again, the first year is
       a leap year, the next two are normal (only two since we already
       divided by groups of 4 years).
     */
     cDaysYear = 366;
     while ((INT)dt >= cDaysYear)
          {
          dt -= cDaysYear;
          pd3 -> wYear++;
          cDaysYear = 365;
          }

     /* Subtract out days of each month.  Note that we add one
        to the count of days in the month for February in a leap year.
     */
     for (i = MONTHJAN; (INT)dt >= (cDaysMonth = vrgcDaysMonth [i] +
      (cDaysYear == 366 && i == MONTHFEB ? 1 : 0)); i++)
          dt -= cDaysMonth;

     pd3 -> wMonth = i;

     /* Whatever's left is the offset into the month. */
     pd3 -> wDay = dt;
     }



/**** Set text of an edit ctl and then place selection at end. */
VOID APIENTRY SetEcText(
     HWND    hwnd,
     CHAR *  sz)
     {
     WPARAM        iSelFirst;
     WPARAM        iSelLast;

     SendMessage (hwnd, WM_SETTEXT, 0, (LONG)(LPSTR)sz);
     iSelFirst = iSelLast = -1;
     SendMessage(hwnd, EM_SETSEL, iSelFirst, (LONG)iSelLast);
     }
