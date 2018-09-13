/*
 *   Windows Calendar
 *   Copyright (c) 1985 by Microsoft Corporation, all rights reserved.
 *   Written by Mark L. Chamberlin, consultant to Microsoft.
 *
 *   cal.c
 *
 */

/* Get rid of more stuff from windows.h */
#define NOWINSTYLES
#define NOSYSMETRICS
#define NODRAWTEXT

#include "cal.h"

BOOL FAR APIENTRY IsDefaultPrinterStillValid(LPSTR);

/**** FCalSize ****/

BOOL APIENTRY FCalSize (
     HWND hwnd,
     INT x,
     INT y,
     INT code)
     {
     INT     cy, ytop, cx, xleft,
             cyUseable, vclnOld, dln = 0,
             tmp, dx, dy;


     if (hwnd==vhwnd0)
	  {
	  /* Store appointment currently being edited so it gets repainted
           *  by DayPaint.
           */

	  if (GetFocus () == vhwnd3)
	  {
	       StoreQd ();
	  }

	  switch (code)
	       {
	       case SIZEFULLSCREEN:
	       case SIZENORMAL:
                    MoveWindow(vhwnd1, xleft=XcoWnd1(),
                                ytop=YcoWnd1(), vcxWnd1,
                                vcyWnd1, TRUE);

                    cy = vcyWnd2B;

                    if (cy > y-ytop-vcyWnd2A)
                        cy=y-ytop-vcyWnd2A;

                    cx = vcxWnd1 - vcxBorder;

                    if (cx > x-xleft)
                        cx=x-xleft;

                    MoveWindow(vhwnd2B, 0, vcyWnd2A, cx, cy, FALSE);

		    /* Reset global variables according to new window size. */
                    cyUseable = cy - 2 * vcyBorder - vcyExtLead ;
		    vclnOld = vcln;
		    vcln = cyUseable/vcyLineToLine;

		    /* Always display at least one line.  In addition to
		       avoiding div by 0 errors, this shows user that there
		       is something that is "trying" to be displayed even if
		       there is not enough space. */
			//- FCalSize: Fixed to handle vcln < 0.
		    if (vcln <= 0)
				vcln = 1;
		    vlnLast = vcln-1;

		    /* If we're in day mode, reset the scroll range so user
		       can scroll 11:00 pm to bottom of window.  (If in month
		       mode, smallest unit of scrolling is one month.) */

		    /* foll. lines set up vertical scroll globals  for month view
		       Set vmScrollMax so that if less than a certain fraction of
		       the bottom line is visible, scrolling should be possible.
                       This has been determined by trial and error
                     */

		    dy = (vcyWnd2BBot - vcyBorder)/ vcWeeksMonth;
                    tmp = y/dy;

		    if ((y%dy) < (5*dy/6))
                       tmp--;

		    vmScrollMax =max ( 0, vcWeeksMonth + 1 - tmp);
		    vmScrollPos = 0;

		    /* foll lines set up horizontal scroll globals for month view
		       Set hmScrollMax so that if less than a certain fraction of
		       the rightmost column is visible, scrolling should be possible.
		       This has been determined by trial and error */
		    dx = (vcxWnd2B + vcxBorder)/7;
                    tmp = x/dx;

		    if ((x%dx) < (5*dx/6))
                       tmp-- ;

		    hmScrollMax = max (0, 6 - tmp);
		    hmScrollPos = 0;

                    if (vfDayMode)
                        {
			SetDayScrollRange();

			/* If our resizing made window bigger, pick up extra
			   ld records that need to be put in tld.  */
                        if ((dln = vcln - vclnOld) > 0)
                            {
                            while (dln && FGetNextLd(vtld[vlnLast-dln].tm, &vtld[vlnLast-dln+1]))
				dln--;

			    /* If there is going to be extra space at bottom
			       of window, scroll to fill that space. */
			    ScrollDownDay(dln, TRUE, TRUE);

                            }

                        }
                    else
                       {
		       /* set up horiz. and vertical scroll bars in monthmode */

		       SetScrollPos (vhwnd2B, SB_VERT, vmScrollPos, TRUE);
		       SetScrollRange (vhwnd2B, SB_VERT, 0, vmScrollMax,TRUE);

		       SetScrollPos (vhwnd2B, SB_HORZ, hmScrollPos, TRUE);
		       SetScrollRange (vhwnd2B, SB_HORZ, 0, hmScrollMax, TRUE);
                        }
               return (TRUE);
	       }
	  }
     return (FALSE);
     }



/**** CalWndProc ****/

LONG APIENTRY CalWndProc (
     HWND       hwnd,
     WORD   message,
     WPARAM     wParam,
     LONG       lParam)
     {
     PAINTSTRUCT    ps;
     register BOOL  fActed;         /* TRUE if we acted on the message. */
     HCURSOR        hcsr;
     register WORD  wlParamHi;
     INT            lnT;

     fActed=TRUE;
     wlParamHi=HIWORD(lParam);

     switch(message)
	  {
	  case WM_CLOSE:
	       if (FCheckSave (FALSE))
		    DestroyWindow (vhwnd0);
	       break;

	  case WM_QUERYENDSESSION:
	       return (FCheckSave (TRUE));

	  case WM_DESTROY:
	       if (hwnd == vhwnd0)
		    {
		    /* Time to say goodbye - only field this message
		       for our main window.
                    */

                



		    /* Get rid of the change file - ignore errors since
		       there is nothing to be done about it now and the
		       the user has either said to discard the changes
		       or they have already been saved.
		    */
		    DeleteChangeFile ();

            WinHelp(hwnd, vszHelpFile, HELP_QUIT, 0L);

		    /* Free all global objects */
		    CalTerminate(2);

		    /* Terminate with exit code 0, meaning no errors. */
		    PostQuitMessage (0);
		    }
	       else
                    fActed = FALSE;

	       break;

	  case WM_ENDSESSION:
	       /* If wParam is TRUE, we are never coming back again. */
               if (wParam)
                    DeleteChangeFile ();
		    /* Get rid of the change file - ignore errors since
		       there is nothing to be done about it now and the
		       the user has either said to discard the changes
		       or they have already been saved.
		    */


	       break;

	  case WM_SIZE:
               fActed=FCalSize(hwnd, (SHORT)LOWORD(lParam),
                                  (SHORT)HIWORD(lParam), wParam);
	       break;

	  case WM_PAINT:
	       /* Hiding the caret of the appointment description edit
		  control before painting wnd2B in day mode is necessary
		  to prevent leaving cursor droppings around.  So hide
		  the caret here, and show it after painting.  Note that
		  this is OK even if we're not painting wnd2B, so no
		  extra code is used here to only do it for the wnd2B
		  case.
	       */
	       HideCaret (vhwnd3);

	       BeginPaint (hwnd, (LPPAINTSTRUCT)&ps);
	       SetDefaultColors (ps.hdc);
	       CalPaint (hwnd, ps.hdc);
	       EndPaint (hwnd, (LPPAINTSTRUCT)&ps);

	       ShowCaret (vhwnd3);
	       break;

	  case WM_COMMAND:
	       /* HIWORD (lParam) == 0 means a command has been selected
		  via the menu.  1 means a command has been selected
		  via an accelerator.  Something other than 0 or 1 is
		  the window handle of a control sending a notification.
	       */
	       if (GET_WM_COMMAND_CMD (wParam, lParam) <= 1)
		    {
		    /* A menu item has been selected. */
		    CalCommand (hwnd, GET_WM_MENUSELECT_CMD (wParam, lParam));
		    }
	       else
		    {
		    /* Handle notifications from edit controls. */
		    //- EcNotification (wParam, GET_WM_COMMAND_ID(wParam, lParam));
		    EcNotification ((WORD)wParam, (WORD)HIWORD (wParam));

		    /* Even if we handled the message, say we didn't
		       in case something else needs to be done with it.
		    */
		    fActed = FALSE;
		    }
	       break;

          case WM_SYSCOMMAND:

            





		    fActed = FALSE;
	       break;

	  case WM_TIMER:
	       CalTimer (FALSE);
	       break;

	  case WM_TIMECHANGE:
	       CalTimer(TRUE);
	       break;

	  case WM_VSCROLL:
	       if (IsWindowEnabled(vhwnd0))
	       {
		   if (fActed == vfDayMode)
		   {
			FScrollDay (GET_WM_VSCROLL_CODE(wParam, lParam),
				GET_WM_VSCROLL_POS(wParam, lParam));
		   }
		   else
		   {
			FScrollMonth (GET_WM_VSCROLL_CODE(wParam, lParam),
				GET_WM_VSCROLL_POS(wParam, lParam));
		   }
	       }
	       break;

	  case WM_HSCROLL:   /* added  11/3/88 for horiz. scroll in month view */

   /* We have replaced the bitmap arrows with a scrollbar control; So, the
    * following code handles the scroll messages from it
    * Fix for Bug #8560 -- SANKAR -- 01-28-90
    */
#ifndef BUG_8560
	       /* Check if this is the Horizontal Scroll bar control */
	       if(GET_WM_HSCROLL_HWND(wParam, lParam) == vhScrollWnd)
	         {
		    switch(GET_WM_HSCROLL_CODE(wParam, lParam))
		      {
		        case SB_LINEUP:
                          CalCommand (vhwnd0, IDCM_PREVIOUS);
                          break;
			case SB_LINEDOWN:
                          CalCommand (vhwnd0, IDCM_NEXT);
                          break;
                      }
		   break;
		 }
#endif
               if (IsWindowEnabled(vhwnd0))
                  {
		  if (fActed != vfDayMode)
		    FHorizScrollMonth (GET_WM_HSCROLL_CODE(wParam, lParam),
					GET_WM_HSCROLL_POS(wParam, lParam));
		  }


	  case WM_MOUSEMOVE:
	       /* The mouse cursor is an arrow everywhere except when
		  in an appointment description or in the notes area, when
		  it must be the Ibeam.  There are two reasons we can't just
		  let the edit controls take care of this:
		  1) The appointment description edit control only covers
		     one appointment description, and we want the Ibeam
		     to appear on all the descriptions.
		  2) The notes edit control does not use up the entire
		     bottom box of the calendar, and we want the Ibeam in the
		     entire box.  Note that we make the Ibeam start if the
		     cursor is below the line we drew at vycoNotesBox.
	       */
	       hcsr = vhcsrArrow;
	       if (hwnd == vhwnd2B && vfDayMode
                   && (INT)LOWORD (lParam) >= vxcoQdFirst
                   || hwnd == vhwnd1 && (INT)HIWORD (lParam) > vycoNotesBox)

                   hcsr = vhcsrIbeam;

	       SetCursor (hcsr);
	       break;

	  case WM_LBUTTONDBLCLK:
	  case WM_LBUTTONDOWN:
	       if (hwnd == vhwnd1 && (INT)HIWORD (lParam) > vycoNotesBox)
		    {
		    /* Click in the bottom box (below the line at vycoNotesBox)
		       but not in the notes edit control.  Pass the click
		       to the notes edit control.
		       The mouse coordinates we were passed are
		       relative to the origin of wnd1.	Make
		       them relative to the origin of the notes
		       edit control.
		    */
		    ((POINTS*)&lParam)->x -= vxcoWnd2C;
		    ((POINTS*)&lParam)->y -= vycoWnd2C;
		    PostMessage (vhwnd2C, message, wParam, lParam);
		    break;
		    }

	       if (hwnd == vhwnd2A )
		    {

		    /* Double clicking in the date field is the same as
		       using the View Month command (i.e., switch to
		       month mode).
		    */
                    if (message == WM_LBUTTONDBLCLK &&
			   (INT)LOWORD (lParam) >= vxcoDate)
                        {
                        if (vfDayMode)
                            CalCommand (vhwnd0, IDCM_MONTH);
                        else
                            /* Switch back to today */
                            DayMode (&vd3Sel);
                        }

		    break;
		    }

	       if (hwnd == vhwnd2B)
		    {
		    if (vfDayMode)
			 {
			 /* If we just clicked on a new line, "move" edit ctl
			    window to new line.  Otherwise, just pass mouse
			    message to edit ctl. */
			 if ((lnT = LnFromYco (wlParamHi)) != vlnCur) {
			    /* Suppose that the appointment window is not clean.
			      In particular, we are concerned about the
			      rectangle that we are about to put the appointment
			      description edit control on top of.  SetQdEc
			      validates the edit control after moving it to
			      prevent it from repainting, and the ValidateRect
			      call in turn validates that portion of the parent.
			      This means that if it was dirty before calling
			      SetQdEc, it won't get repainted by DayPaint, which
			      it should.  In order to get around this problem,
			      make sure everything is clean before calling
			      SetQdEc.  An example of where this was a problem:
			      Zoom and immediately click on a new appointment.
			      The click was seen before all painting has been
			      done, so a hole was left where the edit control
			      was moved.
			      Force everthing to be clean by calling
			      UpdateWindow for our main window (using
			      wnd2B caused out-of-sequence painting and
			      really left a mess on the screen).
			    */
			    UpdateWindow (vhwnd0);

			    SetQdEc (lnT);
			 }

			 /* Let the edit control see the click too.
			    The mouse coordinates we were passed are
			    relative to the origin of wnd2B.  Make
			    them relative to the origin of the QD
			    edit control.
			 */
			 ((POINTS*)&lParam)->x -= vxcoQdFirst;
			 ((POINTS*)&lParam)->y -= YcoFromLn(vlnCur);
			 PostMessage (vhwnd3, message, wParam, lParam);
			 }
		    else
			 {
			 /* Note - if the mouse position is not on a box for a
			    valid day of the month, the click is ignored.
			    However, we still leave fActed == TRUE since the
			    mouse click has been acted on by us in the sense
			    that we do not expect Windows to do anything
			    further with it.
			 */
			 MouseSelectDay(MAKEMPOINT(lParam),
			      message == WM_LBUTTONDBLCLK);
			 }
		    }
	       else
		    {
		    fActed = FALSE;
		    }
	       break;

	  case WM_KEYDOWN:
	       fActed = FCalKey (hwnd, wParam);
	       break;

	  case WM_ACTIVATE:
	  		if (!fInitComplete)
			{
				fActed = FALSE;
				break;
			}	
				

	       if (GET_WM_ACTIVATE_STATE(wParam, lParam))
		    {
		    /* Becoming active. */

		    /* If not iconic, give the focus to the last one who
		       had it.
		    */
		    if (GET_WM_ACTIVATE_FMINIMIZED(wParam, lParam))
			 CalSetFocus (vhwndFocus);

		    /* Tell the user about any alarms that went off while
		       we were inactive.
		    */
		    PostMessage(hwnd, CM_PROCALARMS, 0, 0L);
		    }
	       else
		    {
		    /* Becoming inactive - pass this off to DefWindowProc. */
		    fActed = FALSE;
		    }
	       break;

	  case CM_PROCALARMS:
                uProcessAlarms ();
		break;

	  case WM_SETFOCUS:
	       /* If the monthly calendar is getting the focus, create,
		  position, and show its caret.  Otherwise, do not process
		  this message.
	       */
	       if (hwnd == vhwnd2B && !vfDayMode)
		    {
		    /* Create a caret for month mode.  Specifying NULL for the
		       second parameter gives a black caret.  The third
		       parameter is the width, and by making the fourth
		       parameter 0, we get a height of a horizontal border
		       (same as vcyBorder).
		    */
		    CreateCaret (vhwnd2B, (HBITMAP)NULL, 2 * vcxFont, 0);

		    /* Position the caret to the selected day. */
		    PositionCaret ();

		    /* Make the caret visible. */
		    ShowCaret (vhwnd2B);

		    /* Remember we last had the focus so we get it
		       back when re-activated.
		    */
		    vhwndFocus = vhwnd2B;
		    }
               else if (hwnd == vhwnd0)
                    /* 12-Mar-1987. to make sure focus set somewhere when
                     * parent gets focus.
                     */
                    CalSetFocus (vhwndFocus);
	       else
		    fActed = FALSE;
	       break;

	  case WM_KILLFOCUS:
	       /* If the monthly calendar is losing the focus,
		  destroy its caret.  Otherwise, do not process this message.
	       */
	       if (hwnd == vhwnd2B && !vfDayMode)
		    DestroyCaret ();
	       else
		    fActed = FALSE;
	       break;

	  case WM_SYSCOLORCHANGE:
	       /* The system colors have changed.  Destroy and recreate
		  the brushes.
	       */
	       DestroyBrushes ();
               CreateBrushes ();

               /* Repaint since AppWorkspace color may have changed */
               InvalidateRect(hwnd, NULL, TRUE);
	       break;

	  case WM_ERASEBKGND:
	       PaintBack (hwnd, (HDC)wParam);
	       break;

	  case WM_INITMENU:
	       /* Menu is being pulled down.  Enable/disable, check/uncheck
		  menu items.
	       */
	       InitMenuItems ();
	       break;

	 case WM_WININICHANGE:
	     CalWinIniChange();
	     break;

	 default:
	     fActed = FALSE;
	     break;
	 }

     return (fActed ? 0L : DefWindowProc (hwnd, message, wParam, lParam));
     }


/**** XcoWnd1 - return the xco of where to put Wnd1 */

INT APIENTRY XcoWnd1 ()
     {
     RECT  rect;
     SHORT cxDesired, cxAvailable, xcoLeft;

     GetClientRect (vhwnd0, (LPRECT)&rect);
     cxDesired = vcxWnd1;
     cxAvailable = rect.right - rect.left;
     xcoLeft = 0;
     if (cxAvailable > cxDesired)
          xcoLeft = (cxAvailable - cxDesired) / 2;

#ifdef DISABLE
     return (max (xcoLeft, vcxFont));
#endif
     return (xcoLeft);
     }




/**** YcoWnd1 - return the yco of where to put Wnd1 */

INT APIENTRY YcoWnd1 ()
     {
     RECT  rect;
     INT   cyAvailable;
     INT   ycoTop;

     GetClientRect (vhwnd0, (LPRECT)&rect);
     cyAvailable = rect.bottom - rect.top;
     ycoTop = 0;
     if (cyAvailable > vcyWnd1)
	  ycoTop = (cyAvailable - vcyWnd1) / 2;
#ifdef DISABLE
     return (max (ycoTop, vcyBorder));
#endif
     return (ycoTop);
     }




/**** CalSetFocus - Set the focus unless vfNoGrabFocus is set.	This
      is used to prevent Calendar from grabbing the focus if brought
      up iconic.
*/

VOID APIENTRY FAR CalSetFocus (HWND hwnd)
     {
     if (!vfNoGrabFocus)
	  SetFocus (hwnd);
     }


/**** InitMenuItems */

VOID APIENTRY InitMenuItems ()
     {
     register WORD mf1;
     register WORD mf2;
     HMENU         hMenu;
     WORD2DWORD    iSelFirst;
     WORD2DWORD    iSelLast;
     WORD      wFmt;

     /* Get a handle to the menu. */
     hMenu = GetMenu (vhwnd0);

     /* Cut and Copy - enable iff edit control has focus and
	some text is selected.
	Paste - enable iff edit control has focus and the clipboard
	is not empty.
     */
     mf1 = mf2 = MF_GRAYED;
     if (vhwndFocus != vhwnd2B)
	  {
	  /* Focus is not on monthly calendar so it must be on either
	     the appointment edit control or the notes edit control.
	  */

	  /* Enable Cut and Copy if the selection isn't null. */
	  MSendMsgEM_GETSEL(vhwndFocus, &iSelFirst, &iSelLast);
	  if (iSelFirst != iSelLast)
	       mf1 = MF_ENABLED;

	  /* Enable Paste if the clipboard isn't empty. */
	  if (OpenClipboard (vhwnd0))
	       {
		wFmt = 0;
		/* If clipboard has any text data, enable paste item.  otherwise
		   leave it grayed. */
		while ((wFmt = EnumClipboardFormats(wFmt)) != 0)
		    {
		    if (wFmt == CF_TEXT)
			{
			mf2 = MF_ENABLED;
			break;
			}
		    }
	       CloseClipboard ();
	       }
	  }
     EnableMenuItem (hMenu, IDCM_CUT, mf1);
     EnableMenuItem (hMenu, IDCM_COPY, mf1);
     EnableMenuItem (hMenu, IDCM_PASTE, mf2);

     /* Check day if in day mode, check month if in month mode.
	Uncheck the other.
     */
     mf1 = MF_CHECKED;
     mf2 = MF_UNCHECKED;
     if (!vfDayMode)
	  {
	  mf1 = MF_UNCHECKED;
	  mf2 = MF_CHECKED;
	  }
     CheckMenuItem (hMenu, IDCM_DAY, mf1);
     CheckMenuItem (hMenu, IDCM_MONTH, mf2);

     /* Alarm Set - enable iff focus is on an appointment.
	If enabled, check iff appointment has alarm set.
     */
     mf1 = MF_GRAYED;
     mf2 = MF_UNCHECKED;
     if (vhwndFocus == vhwnd3)
	  {
	  mf1 = MF_ENABLED;
	  if (FAlarm (vlnCur))
	       mf2 = MF_CHECKED;
	  }
     EnableMenuItem (hMenu, IDCM_SET, mf1);
     CheckMenuItem (hMenu, IDCM_SET, mf2);

     /* Options special time - enable if in day mode. */
     mf1 = MF_GRAYED;
     if (vfDayMode)
	  mf1 = MF_ENABLED;
     EnableMenuItem (hMenu, IDCM_SPECIALTIME, mf1);

     }



VOID APIENTRY CalWinIniChange()
    {
    HMENU hMenu;
    SHORT id, nx;
    CHAR  ch;
    static bszDecRead=FALSE;

    /* Set decimal to scan for */
    if (bszDecRead)
        ch=szDec[0]; /* If we already changed it. */
    else
        ch='.';  /* First time. */

    bszDecRead=TRUE;

    /* Get the intl decimal character for use in Page Setup Box. */
    GetProfileString("intl", "sDecimal", ".", szDec, 4);

    /* Scan for . and replace with intl decimal */
    for (id=2; id<6; id++)
        {

        for (nx=0; nx < lstrlen((LPSTR)chPageText[id]); nx++)
            if (chPageText[id][nx]==ch)
                chPageText[id][nx]=szDec[0];
        }


    hMenu = GetMenu(vhwnd0);
    /* Check if a default printer exists */
    /* Fix for Bug #5607 --SANKAR-- 10-30-89 */
    if(bPrinterSetupDone)
      {
        if(!IsDefaultPrinterStillValid((LPSTR)szPrinter))
	    bPrinterSetupDone = FALSE; /* No longer valid */
      }
#ifdef SLOWTHINGSDOWN
    if (((INT)(hdc = GetPrinterDC())) < 0)
	{
	EnableMenuItem(hMenu, IDCM_PRINT, MF_GRAYED);
	}
    else
	{
	DeleteDC(hdc);
	EnableMenuItem(hMenu, IDCM_PRINT, MF_ENABLED);
	}
#endif
    InitTimeDate (vhInstance, vfHour24 ? GTS_24HOUR : GTS_12HOUR);


    /* Force repainting of windows that contain date time strings. */
    InvalidateRect(vhwnd0,  NULL, TRUE);
    }
