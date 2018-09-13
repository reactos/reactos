/*
 *   Windows Calendar
 *   Copyright (c) 1985 by Microsoft Corporation, all rights reserved.
 *   Written by Mark L. Chamberlin, consultant to Microsoft.
 *
 *   ***** calmon2.c
 *
 */

/* Get rid of more stuff from windows.h */
#define NOWINMESSAGES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOSYSCOMMANDS
#define NOCLIPBOARD

#include "cal.h"



/**** FMonthPrev ****/
BOOL APIENTRY FMonthPrev ()
     {
     if (vd3To.wMonth != MONTHJAN)
          vd3To.wMonth--;
     else
          {
          if (vd3To.wYear == YEAR1980)
               return (FALSE);

          vd3To.wYear--;
          vd3To.wMonth = MONTHDEC;
          }

     return (TRUE);
     }



/**** FMonthNext ****/
BOOL APIENTRY FMonthNext ()
     {
     if (vd3To.wMonth != MONTHDEC)
          vd3To.wMonth++;
     else
          {
          if (vd3To.wYear == YEAR2099)
               return (FALSE);

          vd3To.wYear++;
          vd3To.wMonth = MONTHJAN;
          }

     return (TRUE);
     }




/**** ShowMonthPrevNext ****/
VOID APIENTRY ShowMonthPrevNext (BOOL fNext)
     {
     /* First see if able to move to previous or next month.  (Can't
        move back if already January 1980, can't move forward if already
        December 2099.)
     */
     if (fNext ? FMonthNext () : FMonthPrev ())
          UpdateMonth ();
     }




/**** UpdateMonth - update the month display after the month has been
      changed, or when switching from day mode to month mode.
      Set the selected day to the minimum of the sticky day or the
      last day of the month.
****/

VOID APIENTRY FAR UpdateMonth ()
     {
     HDC    hDC;

     /* See if the data can be fetched for the target date.  If not,
        don't switch to that date.  If so, switch.
     */
     vd3To.wDay = (WORD)min ((INT)(CDaysMonth (&vd3To) - 1), (INT)vwDaySticky);
     if (FFetchTargetDate ())
          {
          vd3Sel=vd3To;
          SetScrollPos(vhwnd2B, SB_VERT, vd3Sel.wYear*12+vd3Sel.wMonth, TRUE);

          SetUpMonth ();
          InvalidateMonth ();
	  UpdateWindow (vhwnd0);   /* changed from vhwnd2B to fix scrollbar
				      repaint problems */
          SetNotesEc ();

          hDC=GetDC(vhwnd2A);
          DispDate (hDC, &vd3To);
          ReleaseDC(vhwnd2A, hDC);
          }
     }




/**** MouseSelectDay  - we have a click in the monthly calendar.  Map
      it to a day and move the selection.
*****/
VOID APIENTRY MouseSelectDay (
     MPOINT     point,             /* Screen coordinates of mouse where click
                                     occurred.
                                    */
     BOOL      fDblClk)            /* TRUE for double click. */
     {
     INT  ixco, iyco, irgb;

     if (FMapCoToIGrid (point.x, vrgxcoGrid, 7, &ixco) &&
          FMapCoToIGrid (point.y, vrgycoGrid, vcWeeksMonth, &iyco) &&
          vrgbMonth [irgb = 7 * iyco + ixco +7*vmScrollPos + hmScrollPos]
          != 0)
	  {
	  vd3To = vd3Sel;
	  MoveSelCurMonth (irgb - vwWeekdayFirst );

          /* Give the focus to the calendar if it doesn't already have it. */
          if (GetFocus () != vhwnd2B)
               CalSetFocus (vhwnd2B);

          /* Switch to day mode for the day the user double clicked on. */
	  if (fDblClk)
               DayMode (&vd3Sel);
          }

     }



/**** FScrollMonth */
/************************************************************************
 *
 * VOID PASCAL FScrollMonth (code, posNew)
 *
 * purpose : calculates sone vertical scroll globals and scrolls
 *	     the month display vertically.
 *
 ***********************************************************************/
VOID APIENTRY FScrollMonth (
     INT  code,
     WORD posNew)
     {
     INT dy;
     RECT rect;

     /* calculate the step size for the scroll. The step size is
        approximately the height of the month grid  */

     if (hmScrollMax == 0)
         dy = (vcyWnd2BBot - vcyBorder +vcyHScrollBar)/ vcWeeksMonth;
     else
         dy = (vcyWnd2BBot - vcyBorder)/ vcWeeksMonth;


     switch (code)
         {
         case SB_LINEUP :
             vmScrollInc = -1;
             break;

         case SB_LINEDOWN:
             vmScrollInc = 1;
             break;

         case SB_TOP:
             vmScrollInc = -vmScrollPos;
             break;

         case SB_BOTTOM :
             vmScrollInc = vmScrollMax - vmScrollPos;
             break;

         case SB_PAGEUP :
         case SB_PAGEDOWN :
             break;

         case SB_THUMBTRACK :
             vmScrollInc = posNew -vmScrollPos;
             break;

         default:
              vmScrollInc = 0;
         }

     if ((vmScrollInc = max(-vmScrollPos,
                            min (vmScrollInc, vmScrollMax-vmScrollPos))) != 0)
         {
         GetClientRect (vhwnd2B, (LPRECT)&rect);
         rect.top = vcyWnd2BTop;
         rect.bottom = vcyWnd2BBot + 2*dy;

         if (vmScrollMax ==1)
             rect.bottom += dy;

         vmScrollPos +=vmScrollInc;
         ScrollWindow (vhwnd2B, 0, -dy * vmScrollInc, &rect, &rect);
         SetScrollPos (vhwnd2B, SB_VERT, vmScrollPos, TRUE);

         /* refresh screen to get rid of caret droppings */
         rect.bottom = vcyWnd2BBot;
         rect.top -= 3*vcyLineToLine;
         InvalidateRect(vhwnd2B, (LPRECT)&rect, TRUE);
         UpdateWindow (vhwnd2B);
         }
    }




/************************************************************************
 *
 * VOID PASCAL FHorizScrollMonth (code, posNew)
 *
 * purpose : calculates some horizontal scroll globals and scrolls the
 *	     month display vertically.
 *
 ***********************************************************************/
VOID APIENTRY FHorizScrollMonth (
    INT code,
    WORD posNew)
    {
    RECT rect;
    INT dx;
    /* calculate the step size for the scroll. The step size
       is approximately the width of the month grid */

    if (vmScrollMax == 0)
	dx = (vcxWnd2B + vcxBorder+ vcxVScrollBar)/7;
    else
	dx = (vcxWnd2B + vcxBorder)/7;

    switch (code)
        {
	case SB_LINEUP:
	    hmScrollInc = -1;
            break;

	case SB_LINEDOWN:
	    hmScrollInc = 1;
            break;

	case SB_TOP :
	    hmScrollInc = -hmScrollPos;
            break;

	case SB_BOTTOM:
	    hmScrollInc = hmScrollMax -hmScrollPos;
            break;

	case SB_PAGEUP:
	case SB_PAGEDOWN:
            break;

	case SB_THUMBTRACK:
	    hmScrollInc = posNew - hmScrollPos;
            break;

	default:
	    hmScrollInc = 0;
        }

    if ((hmScrollInc = max(-hmScrollPos,
			   min(hmScrollInc, hmScrollMax - hmScrollPos))) != 0)
	{
	GetClientRect (vhwnd2B, (LPRECT)&rect);
	rect.top = vcyWnd2BTop - vcyLineToLine;
	rect.bottom = vycoQdMax;
	hmScrollPos += hmScrollInc;
        ScrollWindow ( vhwnd2B , -dx*hmScrollInc, 0, &rect, &rect);
	SetScrollPos ( vhwnd2B, SB_HORZ, hmScrollPos, TRUE );

	/* refresh screen to get rid of caret droppings */
	rect.bottom += vcyHScrollBar;
	rect.top -= 2* vcyLineToLine;
	InvalidateRect(vhwnd2B, (LPRECT)&rect, TRUE);
	UpdateWindow ( vhwnd2B );
        }
    }





/**** FCalKey ****/
BOOL APIENTRY FCalKey (
     HWND hwnd,
     WPARAM kc)       /* The virtual key code. */
     {
     register INT  iTemp;

     if (!vfDayMode && hwnd == vhwnd2B)
          {
          vd3To = vd3Sel;
          switch (kc)
               {
               case VK_LEFT:
                    if (vd3To.wDay != 0)
                         MoveSelCurMonth (vd3To.wDay - 1);
                    else
                        if (FMonthPrev ())
                            MoveSelNewMonth (CDaysMonth (&vd3To) - 1);
                    break;

               case VK_RIGHT:
                    if ((iTemp = vd3To.wDay + 1) < vcDaysMonth)
                         MoveSelCurMonth ((WORD)iTemp);
                    else
                        if (FMonthNext ())
                            MoveSelNewMonth (0);
                    break;

               case VK_UP:
                    if ((iTemp = vd3To.wDay - 7) >= 0)
                         MoveSelCurMonth ((WORD)iTemp);
                    else
                        if (FMonthPrev ())
                            MoveSelNewMonth (CDaysMonth (&vd3To) + iTemp);
                    break;


               case VK_DOWN:
                    if ((iTemp = vd3To.wDay + 7) < vcDaysMonth)
                         MoveSelCurMonth ((WORD)iTemp);
                    else
                         {
                         iTemp -= vcDaysMonth;
                         if (FMonthNext ())
                              MoveSelNewMonth (iTemp);
                         }
                    break;

               case VK_PRIOR:
                    ShowMonthPrevNext (FALSE);
                    break;

               case VK_NEXT:
                    ShowMonthPrevNext (TRUE);
                    break;

               case VK_TAB:
                    /* Switch to the notes area. */
                    CalSetFocus (vhwnd2C);
                    break;

               case VK_RETURN:
		    /* Switch to day mode for the selected day. */
                    DayMode (&vd3Sel);
                    break;

               default:
                    return (FALSE);
               }
          return (TRUE);
          }
     return (FALSE);
     }




/**** MoveSelCurMonth  ****/
VOID APIENTRY MoveSelCurMonth (WORD wDaySel)
     {
     HDC  hDC;

     /* Only switch if we can fetch the data for the target date. */
     vd3To.wDay = wDaySel;
     if (FFetchTargetDate ())
          {
          hDC = CalGetDC (vhwnd2B);
          InvertDay (hDC, vd3Sel.wDay);
          InvertDay (hDC, vwDaySticky = vd3Sel.wDay = wDaySel);
          ReleaseDC (vhwnd2B, hDC);

          hDC=GetDC(vhwnd2A);
          DispDate (hDC, &vd3Sel);
          ReleaseDC(vhwnd2A, hDC);


          PositionCaret ();
          SetNotesEc ();
          }
     }



/**** MoveSelNewMonth ****/
VOID APIENTRY MoveSelNewMonth (WORD wDaySel)
     {

     vd3To.wDay = wDaySel;

     /* Don't change vwDaySticky unless we can successfully switch to the
        target date.
      */

     if (FFetchTargetDate ())
          {
          vwDaySticky = wDaySel;
          UpdateMonth ();
          }
     }



/**** JumpDate - Jump to specified date in month mode. ****/
VOID APIENTRY JumpDate (D3*pd3)
     {
     register WORD wDay;

     wDay = pd3 -> wDay;

     if (pd3 -> wMonth == vd3Sel.wMonth && pd3 -> wYear == vd3Sel.wYear)
          {
          /* The target date is in the same month as the one we are
             currently displaying - just move the highlight.
          */
          MoveSelCurMonth (wDay);
          }
     else
          {
          /* The target date is not in the current month.  Need to
             update the entire month display.
          */
          vd3To = *pd3;
          MoveSelNewMonth (wDay);
          }
     }



/**** InvalidateMonth ****/
VOID APIENTRY InvalidateMonth ()
     {
     InvalidateRect (vhwnd2B, NULL, TRUE);
     }



/**** FFetchTargetDate ****/
BOOL APIENTRY FFetchTargetDate ()
     {
     register HWND hwndFocus;
     register BOOL fOk;

     /* FGetDateDr leaves the focus NULL if it succeeds, so remember who
        has it now, and set it back when finished.
     */
     hwndFocus = GetFocus ();
     fOk = FGetDateDr (DtFromPd3 (&vd3To));
     CalSetFocus (hwndFocus);
     return (fOk);
     }
