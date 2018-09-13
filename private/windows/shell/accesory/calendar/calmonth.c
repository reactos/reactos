/*
 *   Windows Calendar
 *   Copyright (c) 1985 by Microsoft Corporation, all rights reserved.
 *   Written by Mark L. Chamberlin, consultant to Microsoft.
 *
 ****** calmonth.c
 *
 */

/* Get rid of more stuff from windows.h */
#define NOWINMESSAGES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOSYSCOMMANDS
#define NOCLIPBOARD

#include "cal.h"


/**** GetWeekday - calculate day of week from D3. */

WORD APIENTRY GetWeekday (D3*pd3)
     {
     /* Add 2 since January 1, 1980 was a Tuesday. */
     return ((DtFromPd3 (pd3) + 2) % 7);
     }




/**** CDaysMonth - return the number of days in the month specified
      by the month and year of the D3 argument.
****/

INT APIENTRY CDaysMonth (D3*pd3)
     {
     register INT cDays;

     /* Calculate the number of days in the current month (adding in
        one if this is a leap year and the month is past February.
      */
     cDays = vrgcDaysMonth [pd3 -> wMonth];
     if (pd3 -> wYear % 4 == 0 && pd3 -> wMonth == MONTHFEB)
          cDays++;

     return (cDays);
     }



/**** SetUpMonth - Based on vd3Sel, set up the following:
      - vcDaysMonth - number of days in the month being displayed.
      - vcWeeksMonth - the number of weeks needed to display the month
        (4, 5, or 6).
      - vrgbMonth - the array of days.  0's indicate unused entries.

****/

VOID APIENTRY SetUpMonth ()
     {
     WORD wDay;
     INT  *pb;		      /* changed from BYTE to int */
     D3   d3Temp;
     INT i;

     for (i=0; i< CBMONTHARRAY;i++)
	vrgbMonth[i]=0;

    /* FillBuf (vrgbMonth, CBMONTHARRAY*sizeof(int), 0);  */

     /* Set up the count of days in the month. */
     vcDaysMonth = CDaysMonth (&vd3Sel);

     /* Get the weekday of the the first day of the month. */
     d3Temp = vd3Sel;
     d3Temp.wDay = 0;
     vwWeekdayFirst = GetWeekday (&d3Temp);

     /* Calculate the number of weeks we will need to display. */
     vcWeeksMonth = (vwWeekdayFirst + 6 + vcDaysMonth) / 7;

     /* Fill in the days. */
     pb = vrgbMonth + vwWeekdayFirst;

     for (wDay = 1; (WORD)wDay <= (WORD)vcDaysMonth; wDay++)
       /* *pb++ = (BYTE)wDay;	 */
          *pb++ = wDay;

     /* Set the TODAY bit of the appropriate day if today
        is in this month.
      */
     if (vd3Cur.wMonth == vd3Sel.wMonth && vd3Cur.wYear == vd3Sel.wYear)
          vrgbMonth [vwWeekdayFirst + vd3Cur.wDay] |= TODAY;

     /* Set the marked bits for the marked days in this month. */
     GetMarkedDays ();
     }



/**** BuildMonthGrid */

VOID APIENTRY BuildMonthGrid ()
     {
     INT  dx;
     INT  dy;
     INT  xco;
     INT  yco;
     INT  cLines;

     /* Calculate the x coordinates if vertical scrollbar is absent */
     if (vmScrollMax == 0)
         {
	 dx = (vcxWnd2B + vcxBorder + vcxVScrollBar)/ 7;
	 vrgxcoGrid [7] = vcxWnd2B + vcxHScrollBar;
         }
     else
         {
         /* Calculate the x coordinates if vertical scrollbar is present */
         dx = (vcxWnd2B + vcxBorder) / 7;
         vrgxcoGrid [7] = vcxWnd2B ;
         }

     xco = - vcxBorder;

     for (cLines = 0; cLines < 7; cLines++)
          {
          vrgxcoGrid [cLines] = xco;
          xco += dx;
	  }

     /* Calculate the y coordinates if horiz. scrollbar is absent. */
     if (hmScrollMax == 0)
         {
         dy = (vcyWnd2BBot - vcyBorder + vcyHScrollBar)/vcWeeksMonth;
         vrgycoGrid [vcWeeksMonth] = vcyWnd2B;
         }
     /* Calculate the y coordinates if horiz. scrollbar is present. */
     else
         {
         dy = (vcyWnd2BBot - vcyBorder) / vcWeeksMonth;
         vrgycoGrid [vcWeeksMonth] = vcyWnd2B - vcyHScrollBar;
         }

     yco = vcyWnd2BTop;

     for (cLines = 0; cLines < vcWeeksMonth; cLines++)
          {
          vrgycoGrid [cLines] = yco;
          yco += dy;
          }
     }




/**** PaintMonthGrid - Paint the grid for the monthly calendar display. */

VOID APIENTRY PaintMonthGrid (HDC  hDC)
     {
     INT  *pcoCur;
     INT  *pcoMax;

     BuildMonthGrid ();

     /* Draw the horizontal lines. */

     pcoCur = vrgycoGrid;
     for (pcoMax = pcoCur+vcWeeksMonth; pcoCur < pcoMax; pcoCur++)
	  PatBlt (hDC, 0, *pcoCur, vcxWnd2B + vcxVScrollBar, vcyBorder, PATCOPY);

     /* Draw the vertical lines. */
     pcoCur = vrgxcoGrid + 1;
     for (pcoMax = pcoCur+6; pcoCur < pcoMax; pcoCur++)
         PatBlt(hDC, *pcoCur, vcyWnd2BTop, vcxBorder, vcyWnd2BBot + vcyHScrollBar, PATCOPY);

     }



/**** PaintMonth */

VOID APIENTRY PaintMonth (HDC hDC)
     {
     INT  xcoBox;
     INT  ycoBox;
     INT  dx;
     INT  dy;
     INT  xcoText;
     INT  ycoText;
     INT  cDay;
     INT  cWeek;
     INT  cch;
     CHAR *pch;
     INT  *pb;	    /* changed from BYTE to int */
     INT  *pxcoCur;
     INT  *pycoCur;
     CHAR rgchDayAbbrevs[4];
     CHAR rgch[CCHDATEDISP];
     DOSDATE dd;
     extern HANDLE hinstTimeDate;
     INT iWeekStart;	  /* week no. of month which will appear on top row
			    in monthview */
     INT iWeekEnd;	  /* week no. which will appear on last row of
			    month view	*/
     INT iDayStart;	  /* weekday which will appear on leftmost column */
     INT iDayEnd;	  /* weekday which will appear on rightmost column */
     INT MarkedBits;	  /* the marked bits extracted from a day */

     dd.year  = vd3Sel.wYear + 1980;
     dd.month = vd3Sel.wMonth + 1;

     cch = GetLongDateString(&dd, rgch, GDS_LONG | GDS_NODAY);

     xcoText = (vcxWnd2B - cch * vcxFont) / 2;
     ycoText = 2;

     TextOut(hDC, xcoText, ycoText, (LPSTR)rgch, cch );

     iDayEnd = 7;
     iWeekEnd = vcWeeksMonth + 2;

     /* for an unscrolled window */
     if ((vmScrollPos == 0) && (hmScrollPos == 0))
         {
	 iWeekStart = 0;
	 iDayStart = 0;
	 pb =  vrgbMonth;
	 pycoCur = vrgycoGrid;
         }
     else       /* scrolled window */
         {
	 iWeekStart = vmScrollPos +1;
	 iDayStart  = hmScrollPos;
	 pycoCur   = vrgycoGrid + iWeekStart -(vmScrollPos + 1);
	 pb	   = vrgbMonth + (iWeekStart -1)*7 + iDayStart;
         }

     ycoBox = *pycoCur;
     dy = *pycoCur -ycoBox;
     pxcoCur = vrgxcoGrid;
     pb -= iDayStart - hmScrollPos ;

     /* display "S M T W ..." above month grid */
     for (cDay = iDayStart; cDay < iDayEnd; cDay++ )
         {
         INT cchT;
         xcoBox = *pxcoCur++;
         dx = *pxcoCur -xcoBox;
               cchT = LoadString(hinstTimeDate, IDS_DAYABBREVS+cDay,
                                 (LPSTR)rgchDayAbbrevs, 4);

         xcoText = xcoBox + (dx - cchT*vcxFont)/2;
         ycoText = vcyWnd2BTop -vcyLineToLine;
         TextOut (hDC, xcoText, ycoText, (LPSTR)rgchDayAbbrevs, cchT);
         }

     /* draw the month grid and fill in the dates */
     for (cWeek = iWeekStart ; cWeek < iWeekEnd ; cWeek++,pb+=iDayStart)
	  {
	  ycoBox = *pycoCur++;
          dy = *pycoCur - ycoBox;
	  pxcoCur = vrgxcoGrid;

	  for (cDay = iDayStart; cDay < iDayEnd ; cDay++, pb++)
               {
               xcoBox = *pxcoCur++;
               dx = *pxcoCur - xcoBox;

	       if  (*pb !=0)
                    {
                    cch = 2;
		    ByteTo2Digs (*pb & ~(MARK_BOX| TODAY), pch = rgch);

		    if (*pch == '0')
                         {
                         cch--;
			 pch++;
                         }
                    xcoText = xcoBox + (dx - cch * vcxFont) / 2;
		    ycoText = ycoBox + vcyBorder + (dy	- vcyFont) / 2;

		    /* draw mark symbols, if any */
		    if ((MarkedBits = ((~CLEARMARKEDBITS) & (*pb))) != 0)
			DrawMark (hDC, xcoBox, ycoText, dx, MarkedBits);

		    if (*pb & TODAY)
			 ShowToday (hDC, xcoBox, ycoText, dx);

                    TextOut (hDC, xcoText, ycoText, (LPSTR)pch, cch);
                    }
               }
	  }

     InvertDay (hDC, vd3Sel.wDay);
     /* Don't mess with the caret position if the notes have the focus. */
     if (vhwndFocus == vhwnd2B)
	  PositionCaret ();
     }



/**** DrawMark ***/
/***********************************************************************
 *
 * VOID PASCAL DrawMark (hDC, xcoBox, ycoText, dx, MarkedBits)
 *
 * purpose : To extract the marked bits one by one and draw the corresp.
 *	     mark symbols next to the selected day in month mode.
 *
 * paramteters : hDC	    - the display context
 *		 xcoBox     - the x-coordinate of the left vertical edge
 *			      of box containing selected day
 *		 ycoText    - y-coordinate of text to be written in box
 *		 dx	    - width of box
 *		 MarkedBits - variable with bits set corresp. to marks on
 *			      selected day
 *
 * returns     : none
 *
 * called by   : PaintMonth
 *
 ***********************************************************************/


VOID APIENTRY DrawMark (
     HDC  hDC,
     INT  xcoBox,
     INT  ycoText,
     INT  dx,
     INT  MarkedBits)
     {
     INT  xcoLeft;	  /* left x coordinate of box mark */
     INT  xcoRight;	  /* right x coordinate of box mark */
     INT  ycoTop;	  /* top y coordinate of box mark */
     INT  ycoBottom;	  /* bottom y coordinate of box mark */

     /* Note - the assumption is that numeric digits will not have
        descenders.  Therefore, we subtract out the descent when
        calculating the space below the date digits in the monthly
        calendar display.
     */
     xcoLeft = xcoBox + (dx - 2 * vcxFont) / 2 - 2 * vcxBorder;
     xcoRight = xcoLeft + 2 * vcxFont + 6 * vcxBorder;
     ycoTop = ycoText - 2 * vcyBorder;
     ycoBottom = ycoText + vcyFont + max (vcyBorder - vcyDescent, 0);

     if (MarkedBits & MARK_BOX)   /* a box-type mark */
         {
         PatBlt (hDC, xcoLeft, ycoTop, xcoRight - xcoLeft + 1, vcyBorder, PATCOPY);
         PatBlt (hDC, xcoLeft, ycoBottom, xcoRight - xcoLeft + 1, vcyBorder, PATCOPY);

         PatBlt (hDC, xcoLeft, ycoTop, vcxBorder, ycoBottom - ycoTop + 1, PATCOPY);
         PatBlt (hDC, xcoRight, ycoTop, vcxBorder, ycoBottom - ycoTop + 1, PATCOPY);
         }

     if (MarkedBits & MARK_CIRCLE)	 /* a "o" -type mark */
	    Ellipse (hDC, xcoLeft - 7*vcxBorder, ycoBottom +2*vcyBorder,
		     xcoLeft - 3*vcxBorder, ycoBottom + 6*vcyBorder);

     if (MarkedBits & MARK_PARENTHESES)  /* a parentheses-type mark */
         {
         TextOut ( hDC, xcoLeft - 4*vcxFont/3,ycoText, vszMarkLeftParen, 1);
         TextOut ( hDC, xcoRight + 2*vcxBorder,ycoText, vszMarkRightParen,1);
         }

     if (MarkedBits & MARK_CROSS)     /* an "x" mark */
         {
         (void)MMoveTo (hDC, xcoLeft - 2*vcxBorder, ycoTop - 2*vcxBorder);
         LineTo (hDC, xcoLeft - 7*vcxBorder, ycoTop -6*vcxBorder);
         (void)MMoveTo (hDC, xcoLeft - 6*vcxBorder, ycoTop - 2*vcxBorder);
         LineTo (hDC, xcoLeft - vcxBorder, ycoTop -6*vcxBorder);
         }

     if (MarkedBits & MARK_UNDERSCORE)  /* a "_" mark */
         {
/*
         ycoBottom = ycoText + 14 * vcyBorder;
*/
         PatBlt (hDC, xcoLeft, ycoBottom, xcoRight - xcoLeft + 1, vcyBorder, PATCOPY);
         }
     }


/**** ShowToday */

VOID APIENTRY ShowToday (
     HDC  hDC,
     INT  xcoBox,
     INT  ycoText,
     INT  dx)
     {
     TextOut (hDC, xcoBox + 1, ycoText, (LPSTR)">", 1);
     TextOut (hDC, xcoBox + dx - vcxFont - 2, ycoText, (LPSTR)"<", 1);
     }



/**** InvertDay - Invert the specified day. */

VOID APIENTRY InvertDay (
     HDC  hDC,
     WORD wDay)
     {
     RECT rect;

     MapDayToRect (wDay, &rect);
     InvertRect (hDC, (LPRECT)&rect);
     }




/**** PositionCaret */

VOID APIENTRY PositionCaret ()
     {
     RECT rect;
     INT xcoCaret;
     INT ycoCaret;

     MapDayToRect (vd3Sel.wDay, &rect);

     /* Center the caret horizontally (it is 2 * vcxFont wide),
        and put it just above the bottom of the date box.
      */
     xcoCaret = (rect.left + rect.right) / 2 - vcxFont;
     ycoCaret = rect.bottom - vcyBorder;
     SetCaretPos (xcoCaret, ycoCaret);
     }



/****  MapDayToRect */

VOID APIENTRY MapDayToRect (
     WORD wDay,
     RECT *prect)
     {
     INT  ixco;
     INT  iyco;
     INT  irgb;

     ixco = (irgb = vwWeekdayFirst + wDay - vmScrollPos*7 - hmScrollPos)%7 ;
     iyco = irgb / 7;
     if ((ixco < 0)||(ixco > 6)||(iyco < 0) ||(iyco > vcWeeksMonth))
         {
         prect->left = prect->right = 0;
         prect->top = prect->bottom = 0;
         }
     else
         {
         prect -> left = vrgxcoGrid [ixco] + vcxBorder;
         prect -> right = vrgxcoGrid [ixco + 1 ];
         prect -> top = vrgycoGrid [iyco] + vcyBorder;
         prect -> bottom = vrgycoGrid [iyco + 1];
         }
     }




/**** FMapCoToIGrid */

BOOL APIENTRY FMapCoToIGrid (
     INT  co,            /* INPUT - the coordinate to map. */
     INT  *pco,          /* INPUT - Coordinates of the grid. */
     INT  cco,           /* INPUT - Count of coordinates in the grid. */
     INT  *pico)         /* OUTPUT - the index of the grid coordinate. */
     {
     /* Note - by using co >= *pco in the loop control, we map the
        the leftmost pixel of the calendar to the first column and
        the topmost pixel to the first row.  Then, because inside
        the loop we use co <= *++pco, the rightmost and bottommost
        pixels get mapped to the last column and bottom row.
        A click on the border between two adjacent date boxes is
        mapped to the one to the right (for vertical borders),
        or bottom (for horizontal borders).
     */

     for (*pico = 0; *pico < cco && co >= *pco; (*pico)++)
          {
          if (co <= *++pco)
               return (TRUE);
          }

     return (FALSE);

     }




/**** DtFromPd3 - convert D3 to DT. */

DT   APIENTRY FAR DtFromPd3 (D3 *pd3)
     {
     static INT cDaysAccum [12] =
      {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

     DT   dt;

     dt = (pd3 -> wYear) * 365;

     /* Add in the days for the preceding leap years. */
     if (pd3 -> wYear != 0)
          dt += 1 + (pd3 -> wYear - 1) / 4;

     /* Add in the days for the full months before the current one. */
     dt += cDaysAccum [pd3 -> wMonth];

     /* If this is a leap year and the current month is beyond February,
        add in an extra day.
     */
     if (pd3 -> wYear % 4 == 0 && pd3 -> wMonth > MONTHFEB)
          dt++;

     /* Add in the days of the current month (prior to the current one). */
     dt += pd3 -> wDay;

     return (dt);
     }



/**** GetMarkedDays - set the marked bits in vrgbMonth for the days
      of the current month that are marked.
****/

VOID APIENTRY GetMarkedDays ()
     {
     D3   d3Temp;
     DT   dtFirst;       /* First day of month. */
     DT   dtMax;         /* Last day of month + 1. */
     DT   dtCur;         /* Day of month of current dd. */
     INT  itdd;          /* Index into the tdd. */
     DD   *pdd;          /* Pointer into the tdd. */

     /* Get the DTs of the first and (last + 1) days of the month. */
     d3Temp = vd3Sel;
     d3Temp.wDay = 0;
     dtMax = (dtFirst = DtFromPd3 (&d3Temp)) + vcDaysMonth;

     /* Look for the first day of the month.  If it's found, itdd will
        be its index.  If it's not found, itdd will be the index of the
        first entry in the tdd that is beyond the first day of the month.
        In either case, this is the place where we start looking for marked
        days within the current month.
     */
     FSearchTdd (dtFirst, &itdd);

     /* Lock the tdd and looking at all dates in the current month,
        set their marked bits in vrgbMonth if they are marked in the
        tdd.
     */
     for (pdd = TddLock () + itdd;
      itdd < vcddUsed && (dtCur = pdd -> dt) < dtMax; pdd++, itdd++)
       /* set marked bits on each day */
          if (pdd -> fMarked)
	      vrgbMonth [vwWeekdayFirst + (dtCur - dtFirst)] |= pdd -> fMarked;

     TddUnlock ();
     }




/**** MonthMode - Switch to month mode. */

VOID APIENTRY MonthMode ()
     {

     if (vfDayMode)
          {
          /* Record edits and disable focus for now.  (When UpdateMonth
             calls FFetchNewDate the focus should be NULL so it doesn't
             get set to the wrong thing (like Wnd3)).
          */
          CalSetFocus ((HWND)NULL);

          /* Say we are in Month mode. */
          vfDayMode = FALSE;

          SetScrollRange (vhwnd2B, SB_VERT, 0, SCROLLMONTHLAST, FALSE);
          SetScrollPos (vhwnd2B, SB_VERT, vd3Sel.wYear * 12 + vd3Sel.wMonth,
            TRUE);

          /* Hide the appointment description edit control. */
          ShowWindow (vhwnd3, HIDE_WINDOW);

          /* Repaint Wnd2A to display "Today is: ..." message. */
          InvalidateRect (vhwnd2A, (LPRECT)NULL, TRUE);

          /* Note - we are coming from View Month in CalCommand, so
             vd3To == vd3Sel.  This, along with setting vwDaySticky
             to the current selected day will insure that the selected
             date does not change when we call UpDateMonth.
          */
	  vwDaySticky = vd3Sel.wDay;
	  SetScrollPos (vhwnd2B, SB_HORZ, hmScrollPos, TRUE);
	  SetScrollPos (vhwnd2B, SB_VERT, vmScrollPos, TRUE);
	  SetScrollRange (vhwnd2B, SB_HORZ, 0, hmScrollMax, TRUE);
	  SetScrollRange (vhwnd2B, SB_VERT, 0, vmScrollMax, TRUE);

	  UpdateMonth ();

          /* If the focus was in the notes area in day mode, leave it in
             the notes area in month mode.  Otherwise put the focus on
             the calendar.
          */
          CalSetFocus (vhwndFocus == vhwnd2C ? vhwnd2C : vhwnd2B);
          }
     }
