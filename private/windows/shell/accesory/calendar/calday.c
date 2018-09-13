/*
 *   Windows Calendar
 *   Copyright (c) 1985 by Microsoft Corporation, all rights reserved.
 *   Written by Mark L. Chamberlin, consultant to Microsoft.
 *
*/

/*
 *****
 ***** calday.c
 *****
*/

/* Get rid of more stuff from windows.h */
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOSYSCOMMANDS
#define NOMEMMGR
#define NOCLIPBOARD

#include "cal.h"

#define FIXEDFONTWIDTH 0

WORD wID;

BOOL vfUpdate = TRUE;   /* flag to disable setqdec() update fix. 27-Oct-1987 */

/**** DayMode - Switch to day mode. */

VOID APIENTRY DayMode (D3 *pd3)
     {

     RECT rect;
     HDC  hDC;

     if (!vfDayMode)
	  {
	  /* Say we are in day mode. */
	  vfDayMode = TRUE;

	  /* Disable focus for now.  If in notes area, leave it there,
	     otherwise set up to give focus to appointment description.
	  */
	  CalSetFocus ((HWND)NULL);
	  if (vhwndFocus != vhwnd2C)
	       vhwndFocus = vhwnd3;

	  /* Clear the window so we don't get a blank region appearing
	     in the middle of the monthly calendar when we ShowWindow
	     the appointment description edit control.
	  */
	  GetClientRect (vhwnd2B, (LPRECT)&rect);
	  rect.bottom -= vcyBorder;

	  hDC = CalGetDC (vhwnd2B);
	  FillRect (hDC, (LPRECT)&rect, vhbrBackSub);
	  ReleaseDC (vhwnd2B, hDC);
	  /* Make the appointment description edit control visible. */
	  SetEcText(vhwnd3, "");

	  SetScrollRange (vhwnd2B, SB_HORZ, 0, 0, TRUE);
      SetScrollPos (vhwnd2B, SB_HORZ, 0,TRUE);

          /*InvalidateRect (vhwnd2B, (LPRECT)NULL, FALSE);*/
		  UpdateWindow (vhwnd2B);
          ShowWindow (vhwnd3, SHOW_OPENWINDOW);



	  }

     /* Switch to the specified date.  Note that this gets done even if
	we were already in day mode.  This means that the View Day command
	can be used to get back to the starting time of the currently
	displayed day.	It also is necessary because there callers who
	want the day redisplayed even if already in day mode (like New).
     */
     SwitchToDate (pd3);
     }


/**** SwitchToDate - the ONLY routine that changes the selected day in
      day mode.
*/

VOID APIENTRY SwitchToDate ( D3   *pd3 )
     {
     RECT rect;

     register BOOL fNewMonth;

     if (FGetDateDr (DtFromPd3 (pd3)))
	  {
	  fNewMonth = vd3Sel.wMonth != pd3 -> wMonth
	   || vd3Sel.wYear != pd3 -> wYear;
	  vd3Sel = *pd3;
	  if (fNewMonth)
	       SetUpMonth ();
	  }

     FillTld (vtmStart);
     SetQdEc (0);

     /* If focus is on notes area put it there.  Otherwise it has already
	been set up by SetQdEc.
     */
     if (vhwndFocus == vhwnd2C)
	  CalSetFocus (vhwnd2C);

     /* Set the scroll bar range and thumb position.  (The scroll bar range
	depends on the number of TM in the day, so it must be set up each
	time the day is changed.)
     */

     SetDayScrollRange ();
     SetDayScrollPos (-1);

    /* Repaint Wnd2A to display "Schedule for: ..." message. */
     InvalidateRect (vhwnd2A, (LPRECT)NULL, TRUE);
     UpdateWindow (vhwnd2A);

    /* Redraw the appointments. */
     GetClientRect (vhwnd1, (LPRECT)&rect);
     rect.bottom = vycoWnd2C;
     rect.top = vcyWnd2A;

     InvalidateRect (vhwnd1, (LPRECT)&rect, TRUE);
/*   UpdateWindow (vhwnd1);   */

     /* Set up the notes area. */
     SetNotesEc ();
     }


/**** DayPaint */

VOID APIENTRY DayPaint (HDC  hDC)
     {

     CHAR sz [CCHTIMESZ];
     register INT  ycoText;
     register INT  ln;
     INT  cch;
     TM   tm;
     CHAR *pchQd;
     RECT rectQd;
     BYTE *pbTqr;
     DWORD        iSelFirst;
     DWORD        iSelLast;
#ifdef DISABLE
     DWORD        iSelFirstT;
     DWORD        iSelLastT;
#endif

     pbTqr = PbTqrLock ();
     rectQd.right = vxcoQdMax ;
     rectQd.left = vxcoQdFirst;

     for (ln = 0; ln < vcln; ln++)
	  {
	  ycoText = YcoFromLn (ln);

	  if (FAlarm (ln))
	       DrawAlarmBell (hDC, ycoText);

	  cch = GetTimeSz (tm = vtld [ln].tm, sz);

	  /* Display am or pm only for the first appointment in the window
	     and for noon.
	  */
	  if (ln != 0 && tm != TMNOON)
	      cch = 5;

	  TextOut (hDC, vxcoApptTime, ycoText, (LPSTR)sz, cch);

	  rectQd.top = YcoFromLn (ln);
	  rectQd.bottom = rectQd.top + vcyFont;
	  pchQd = "";
	  if (vtld [ln].otqr != OTQRNIL)
	       pchQd = (CHAR *)(pbTqr + vtld [ln].otqr + CBQRHEAD);

	  DrawText (hDC, (LPSTR)pchQd, -1, (LPRECT)&rectQd,
		    DT_NOPREFIX | DT_SINGLELINE | DT_LEFT | DT_TOP);
	  if (ln == vlnCur)
	       {
	       /* We have just painted the appointment that has the
		  edit control.  In order to keep "flashing" to a minimum,
		  we want to prevent the edit control from repainting,
		  but we do need to get the highlight back up.
		  So:
		  1) Validate the edit control to prevent repainting.
		  2) Disable redraw.
		  3) Fetch and save the current selection.
		  4) Set the selection to null (redraw is off, so this
		     will not affect the highlight).
		  5) Enable redraw.
		  6) Set the selection back to the saved value.  Since redraw
		     is enabled this will highlight the selected characters.
	       */
#ifdef DISABLE
	       ValidateRect (vhwnd3, (LPRECT)NULL);
	       SendMessage (vhwnd3, WM_SETREDRAW, FALSE, 0L);
	       MSendMsgEM_GETSEL(vhwnd3, &iSelFirst, &iSelLast);
	       iselFirstT = iSelFirst;
	       iselLastT = iSelLast;
	       iselFirst = iSelLast = 0;
	       SendMessage(vhwnd3, EM_SETSEL, iSelFirst, (LONG)iSelLast);
	       SendMessage(vhwnd3, WM_SETREDRAW, TRUE, 0L);
	       SendMessage(vhwnd3, EM_SETSEL, iSelFirstT, (LONG)iSelLastT);
#else
               /* don't try to be fancy.  If only part of hilight in update
                * region, there was bug with selection being half inverted,
                * half normal.  This solved it.  10-Jun-1987.
                */
	       MSendMsgEM_GETSEL(vhwnd3, &iSelFirst, &iSelLast);
	       SendMessage(vhwnd3, EM_SETSEL, iSelFirst, (LONG)iSelLast);
#endif
	       }
	  }
     DrUnlockCur ();
     }


/**** FillTld */

VOID APIENTRY FillTld (TM tmFirst)
     {

     LD   *pldCur;
     LD   *pldLast;
     INT  cldEmpty;

     /* Find the first appointment less than or equal to the specified
	one.  Note that since tmFirst must be greater than or equal to
	0 (midnight), calling FGetPrevLd with tmFirst + 1 is guaranteed
	to find something, so there is no need to check the return value.
     */
     FGetPrevLd (tmFirst + 1, vtld);

     /* Work forward filling in the tld.  Stop when the end of the
	table is reached or the end of the day is reached.
     */
     for (pldLast = (pldCur = vtld) + vlnLast; pldCur < pldLast
      && FGetNextLd (pldCur -> tm, pldCur + 1); pldCur++)
			;


     /* If we stopped going forward because we reached the end of the day
	instead of the end of the tld, there are empty entries at the end
	of the tld.  In this case, we scroll the tld down to put the
	empty space at the top, and then we fill in the empty space by
	getting the earlier appointment times.	There will always be
	enough appointment times to fill the tld since the maximum interval
	(1 hour) gives 24 appointment times, and we don't have that many
	lines for displaying appointments.  So there is no need to check
	the return value of FGetPrevLd below, and we can rest assured that
	the tld will get completely filled.
     */
     if ((cldEmpty = pldLast - pldCur) > 0)
	  {
	  ScrollDownTld (cldEmpty);
	  for (pldCur = vtld + cldEmpty; pldCur > vtld; pldCur--)
	       FGetPrevLd (pldCur -> tm, pldCur - 1);
	  }
     }


/**** ScrollDownTld - Scroll the tld down (towards the bottom of the screen,
      but higher in memory) the specified number of ld,
      making room for new lds at the top of the tld.
*/

VOID APIENTRY ScrollDownTld (INT cld)
     {
     BltByte ((BYTE *)vtld, (BYTE *)(vtld + cld),
      (WORD)((vcln - cld) * sizeof (LD)));
     }


/**** ScrollUpTld - Scroll the tld up (towards the top of the screen,
      but lower in memory) the specified number of ld,
      making room for new lds at the bottom of the tld.
*/

VOID APIENTRY ScrollUpTld (INT  cld)
     {
     BltByte ((BYTE *)(vtld + cld), (BYTE *)vtld,
      (WORD)((vcln - cld) * sizeof (LD)));
     }


/**** FGetNextLd */

BOOL APIENTRY FGetNextLd (
    TM   tm,
    LD   *pld)
     {

     TM   tmFromTqr;
     DR   *pdr;


     FSearchTqr (tm);
     tmFromTqr = TMNILHIGH;
     if (votqrNext != (pdr = PdrLockCur ()) -> cbTqr)
	  tmFromTqr = ((PQR )(PbTqrFromPdr (pdr) + votqrNext)) -> tm;
     DrUnlockCur ();

     if ((tm = min (tmFromTqr, TmNextRegular (tm))) == TMNILHIGH)
	  return (FALSE);

     pld -> tm = tm;
     pld -> otqr = tm == tmFromTqr ? votqrNext : OTQRNIL;
     return (TRUE);

     }


/**** FGetPrevLd */

BOOL APIENTRY FGetPrevLd (
    TM   tm,
    LD   *pld)
     {

     TM   tmFromTqr;
     TM   tmInterval;

     FSearchTqr (tm);
     tmFromTqr = TMNILLOW;
     if ((WORD)votqrPrev != OTQRNIL)
	  {
	  tmFromTqr = ((PQR )(PbTqrLock () + votqrPrev)) -> tm;
	  DrUnlockCur ();
	  }

     /* Calculate the previous regular appointment time. */
     tmInterval = tm - 1;
     if (tm == 0 || (tmInterval -= tmInterval % vcMinInterval) < 0)
	  tmInterval = TMNILLOW;

     if ((tm = max (tmFromTqr, tmInterval)) == TMNILLOW)
	  return (FALSE);

     pld -> tm = tm;
     pld -> otqr = tm == tmFromTqr ? votqrPrev : OTQRNIL;
     return (TRUE);

     }


/**** FScrollDay */

BOOL APIENTRY FScrollDay (
    INT  code,
    WORD posNew)
     {

     wID=code;

     switch (code)
	  {
	  case SB_LINEUP:
	       ScrollDownDay (1, TRUE, FALSE);
	       break;

          case SB_LINEDOWN:
               ScrollUpDay (1, TRUE);
	       break;

	  case SB_PAGEUP:
	       ScrollDownDay (vlnLast, TRUE, FALSE);
	       break;

	  case SB_PAGEDOWN:
               ScrollUpDay (vlnLast, TRUE);
	       break;

	  case SB_THUMBPOSITION:
	       /* Record current edits (before changing the tld). */
	       if (vhwndFocus == vhwnd3)
		    CalSetFocus ((HWND)NULL);
	       FillTld (TmFromItm (posNew));

/* Move the call to SetQdEc() after the call to SetDayScrollPos(), and
   use GetScrollPos() to find out if the location passed was beyond the
   end of the scrollbar.  If not, 0 will be passed as before.  If true,
   the call to SetQdEc() will step down to the appropriate location on
   the display.  Tracked down to solve Bug #2502.
                          16 July 1989     Clark Cyr                   */

#if DISABLE
	       SetQdEc (0);
#endif
	       SetDayScrollPos (posNew);
               SetQdEc(posNew - GetScrollPos(vhwnd2B, SB_VERT));
	       InvalidateRect (vhwnd2B, (LPRECT)NULL, TRUE);
	       break;

	  default:
	       return (FALSE);
	  }

     return (TRUE);
     }


/**** ScrollDownDay
	  ctNew is number of lines to scroll.  fScrollBar is true if we
	  are not being scrolled by cursor movement.  fSizing is true iff
	  we are scrolling as a result of resizing. */
VOID APIENTRY ScrollDownDay (
    INT  ctmNew,
    BOOL fScrollBar,
    BOOL fSizing)

     {

     register INT ctm;
     register INT ln;
     LD   ldTemp;
     RECT rect;
     HDC  hDC;
     TM   tmFirstOld;
     extern INT cchTimeMax;
     CHAR sz[CCHTIMESZ];
     INT cch;
     INT iHeight;
     INT iWidth;

     /* Register current changes and hide the caret. */
     if (vhwndFocus == vhwnd3)
	  CalSetFocus ((HWND)NULL);

     tmFirstOld = vtld [0].tm;


     for (ctm = 0; ctm <(ctmNew) && FGetPrevLd (vtld [0].tm, &ldTemp) ; ctm++)
	  {
	  ScrollDownTld (1);
	  vtld [0] = ldTemp;
     }

     if (ctm != 0)
	  {
	  /* Get rid of am or pm on top line of window if it's not noon.
	     Note - it's OK to execute this code even if in 24 hour mode
	     since we will just be putting spaces over spaces.
	  */
          if (tmFirstOld != TMNOON)
	       {
	       hDC = CalGetDC (vhwnd2B);
	       cch = GetTimeSz (tmFirstOld, sz);
               MGetTextExtent(hDC, sz, 5, &iHeight, &iWidth); /* width of time string
							       + the blank following it */
               //- KLUDGE: TextOut (hDC, vxcoApptTime + iWidth, vycoQdFirst,
			   //- KLUDGE:		(LPSTR)vszBlankString,cchTimeMax+3);
			   //- For some reason, the above code no longer blanks out the
			   //- correct area.  It puts a space in the center of the time.
               TextOut (hDC, vxcoApptTime + iWidth + 19, vycoQdFirst,
                                 (LPSTR)"              ",12);
	       ReleaseDC (vhwnd2B, hDC);
	       }

	  GetClientRect (vhwnd2B, (LPRECT)&rect);
	  rect.top = vycoQdFirst;
	  rect.bottom = vycoQdMax;
          ScrollWindow (vhwnd2B, 0, ctm * vcyLineToLine, &rect,&rect);
          }

     /* Need to reset focus even if nothing has scrolled since
	the focus got turned off above.
     */
     ln = 0;
     if (fScrollBar)
          ln = min (vlnCur + ctm , vlnLast);

     vfUpdate = FALSE;
     SetQdEc (ln);
     vfUpdate = TRUE;

     /* When SetQdEc validates the appointment edit control, the
	corresponding rectangle of its parent (wnd2B) gets validated
	too.  If this is in the portion of wnd2B that was invalidated
	by the scroll, we must invalidate it now so it gets painted
	by DayPaint.
	We brought in ctm new lines at the top of wnd2B, so if the
	new ln (the position of the appointment edit control) is
	less than ctm, it's in the invalidated portion of wnd2B.
	Note that if ctm == 0, ln can't be less, so this case is OK.
	If we are resizing, whole window will be repainted.
     */
     if (ln < ctm || fSizing)
          InvalidateParentQdEc (ln);

     if (ctm != 0)
          {
	  UpdateWindow (vhwnd2B);
	  AdjustDayScrollPos (-ctm);

	  /* Need to update edit ctl window incase obscurred by popup. */
          if (AnyPopup() && vhwnd3)
	      {
	      InvalidateRect(vhwnd3, (LPRECT)NULL, TRUE);
	      UpdateWindow(vhwnd3);
              }
	  }
     }


/**** ScrollUpDay */

VOID APIENTRY ScrollUpDay (
    INT  ctmNew,
    BOOL fScrollBar)
     {

     register INT ctm;
     register INT ln;
     LD   ldTemp;
     RECT rect;
     HDC  hDC;


     /* Register current edits and hide the caret. */
     if (vhwndFocus == vhwnd3)
	  CalSetFocus ((HWND)NULL);

     for (ctm = 0; ctm < (ctmNew) && FGetNextLd (vtld [vlnLast].tm, &ldTemp);
      ctm++)
	  {
	  ScrollUpTld (1);
	  vtld [vlnLast] = ldTemp;
	  }

     if (ctm != 0)
	  {
	  GetClientRect (vhwnd2B, (LPRECT)&rect);
	  rect.top = vycoQdFirst;
	  rect.bottom = vycoQdMax;
          ScrollWindow (vhwnd2B, 0, -ctm * vcyLineToLine, &rect, &rect);

          if (wID==SB_PAGEDOWN)
              {
              /* Fix the problem of not repainting some times when scrolling */
              rect.top=vycoQdFirst;
              rect.bottom =vycoQdMax-(ctm*vcyLineToLine);
              rect.right=vxcoQdMax;
              rect.left=0;
              InvalidateRect(vhwnd2B, &rect, TRUE);
              }

	  /* If in 12 hour mode, put am/pm on first appointment in the window. */
	  if (!vfHour24)
	       {
#if FIXEDFONTWIDTH
               CHAR *sz;
	       extern CHAR sz1159[];
               extern CHAR sz2359[];
#else
               CHAR sz[CCHTIMESZ];
               INT cch;
#endif

               hDC = CalGetDC (vhwnd2B);

/* This has the problem that spaces do not have the same width as numbers
   in the new system fonts.  Depending on vxcoAmPm as the constant position
   for where AM and PM should be offset is no longer safe.  This is just a
   bandaid and should be written correctly later.  17 July 1989  Clark Cyr */

#if FIXEDFONTWIDTH
               sz=(vtld[0].tm < TMNOON ? sz1159 : sz2359);
               TextOut(hDC, vxcoAmPm, vycoQdFirst, sz , lstrlen(sz));
#else
	       cch = GetTimeSz (vtld[0].tm, sz);
               TextOut (hDC, vxcoApptTime, vycoQdFirst, (LPSTR)sz, cch);
#endif
	       ReleaseDC (vhwnd2B, hDC);
               }

         }
     /* Need to reset focus even if nothing has scrolled since
	the focus got turned off above.
     */
     ln = vlnLast;
     if (fScrollBar)
          ln = max (vlnCur - ctm, 0);

     vfUpdate = FALSE;
     SetQdEc (ln);
     vfUpdate = TRUE;

     /* When SetQdEc validates the appointment edit control, the
	corresponding rectangle of its parent (wnd2B) gets validated
	too.  If this is in the portion of wnd2B that was invalidated
	by the scroll, we must invalidate it now so it gets painted
	by DayPaint.
	We brought in ctm new lines at the bottom of wnd2B, so if the
	new ln (the position of the appointment edit control) is
	greater than vlnLast - ctm, it's in the invalidated portion of wnd2B.
	Note that if ctm == 0, ln can't be greater, so this case is OK.
     */
     if (ln > vlnLast - ctm)
	  InvalidateParentQdEc (ln);

     if (ctm != 0)
	  {
	  rect.top = YcoFromLn(vlnLast);
	  rect.bottom = rect.top + vcyLineToLine;
	  InvalidateRect(vhwnd2B, (LPRECT)&rect, TRUE);
	  UpdateWindow (vhwnd2B);
	  AdjustDayScrollPos (ctm);

	  /* Need to update edit ctl window incase obscurred by popup. */
	  if (AnyPopup() && vhwnd3)
	      {
	      InvalidateRect(vhwnd3, (LPRECT)NULL, TRUE);
	      UpdateWindow(vhwnd3);
	      }
	  }
     }


/**** InvalidateParentQdEc */

VOID APIENTRY InvalidateParentQdEc (INT ln)
     {

     RECT rect;

     rect.top = YcoFromLn (ln);
     rect.bottom = rect.top + vcyFont;
     rect.left = vxcoQdFirst;
     rect.right = vxcoQdMax;

     InvalidateRect (vhwnd2B, &rect, TRUE);
     OffsetRect(&rect, 0, vcyWnd2A);
     InvalidateRect (vhwnd1, &rect, TRUE);

     }


/**** YcoFromLn - given line number, return yco within Wnd2B */

INT  APIENTRY YcoFromLn (INT ln)
     {
     return (vycoQdFirst + ln * vcyLineToLine);
     }


/**** LnFromYco - given yco within Wnd2B, return line number. */

INT  APIENTRY LnFromYco (INT yco)
     {
     return (min (max (yco - vycoQdFirst, 0) / vcyLineToLine, vlnLast));
     }


/**** SetQdEc - Position and set the text of the appointment description
      edit control.
*/

VOID APIENTRY SetQdEc (INT ln)
     {

     register BYTE *pbTqr;
     register CHAR *pchQd;
     RECT rc;

     /* Store edits for current appointment description. */
     if (vhwndFocus == vhwnd3)
	  CalSetFocus ((HWND)NULL);

     /* Set the new current ln. */
     vlnCur = ln;

     /* do this to fix bug when scrolling before window is
      * actually painted on screen.  the movewindow below
      * forces the parents update region to clip out where
      * the child window was, and it would not get erased.
      * if in the scrollup/dn code, don't do the update or
      * you get an unnecessary flash.  vfUpdate will be false.
      * 27-Oct-1987. davidhab.
      */
     if (vfUpdate && GetUpdateRect(vhwnd2B, (LPRECT)&rc, FALSE)) {
        GetWindowRect(vhwnd3, (LPRECT)&rc);
        UpdateWindow(vhwnd2B);
     }


     ShowWindow(vhwnd3, SW_HIDE);
     MoveWindow (vhwnd3, vxcoQdFirst, YcoFromLn (ln),
		vxcoQdMax - vxcoQdFirst , vcyFont, FALSE);

     pbTqr = PbTqrLock ();
     pchQd = "";
     if (vtld [ln].otqr != OTQRNIL)
	  pchQd = (CHAR *)(pbTqr + vtld [ln].otqr + CBQRHEAD);
     /*SendMessage (vhwnd3, WM_SETREDRAW, FALSE, 0L);*/
     SetEcText(vhwnd3, pchQd);
     ShowWindow(vhwnd3, SW_SHOW);
     /*SendMessage (vhwnd3, WM_SETREDRAW, TRUE, 0L);
     ValidateRect (vhwnd3, (LPRECT)NULL);*/
     DrUnlockCur ();

     /* If not in the notes area, give the focus to the appointment
	description edit control.
     */
     if (vhwndFocus == vhwnd3)
          CalSetFocus (vhwnd3);



     }
