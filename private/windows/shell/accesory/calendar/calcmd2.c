/*
 *   Windows Calendar
 *   Copyright (c) 1985 by Microsoft Corporation, all rights reserved.
 *   Written by Mark L. Chamberlin, consultant to Microsoft.
 *
 ****** calcmd2.c
 *
 */

/* Get rid of more stuff from windows.h */
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOSYSCOMMANDS
#define NOMEMMGR
#define NOKANJI
#define NOSCROLL
#define NODRAWTEXT
#define NOVIRTUALKEYCODES

#define ATTRDIR   0xC010        /* added for the revised SaveAs dialog */

#include "cal.h"
#include <ctype.h>
#include <fcntl.h>       /* Get O_RDONLY definition for open call. */
CHAR rgch[256];    /* moved out of FnSaveAs because of 512 byte limit of local variables */
CHAR szFullPathName[120];

INT CheckMarginNums(HWND hWnd);

BOOL APIENTRY CallSaveAsDialog ()
{
     CHAR szSaveFileSpec [CCHFILESPECMAX] = "";
     CHAR szAnsiFileSpec [CCHFILESPECMAX];
     extern CHAR szLastDir[];

     /* default selection in edit window */
     lstrcpy ((LPSTR)szSaveFileSpec, OFStruct[IDFILEORIGINAL].szPathName);

     /* set up the variable fields of the OPENFILENAME struct. (the constant
      * fields have been sel in CalInit()
      */
     vOFN.lpstrFile	    = szSaveFileSpec;
     vOFN.lpstrInitialDir   = szLastDir;
     vOFN.lpstrTitle	    = vszSaveasCaption;
     vOFN.Flags 	    = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;	   /* always 0 for SaveAs */

     /* All long pointers should be defined immediately before the call.
      * L.Raman - 2/12/91
      */
     vOFN.lpstrFilter	    = (LPSTR)vszFilterSpec;
     vOFN.lpstrCustomFilter = (LPSTR)vszCustFilterSpec;
     vOFN.lpstrDefExt	    = vszFileExtension + 1;   /* point to "CAL" */

     if ( GetSaveFileName ((LPOPENFILENAME)&vOFN))
     {
// on NT, don't do the oemtoansi, just copy the file into new buffer.
// OemToAnsi(vOFN.lpstrFile, (LPSTR)szAnsiFileSpec);

     strcpy(szAnsiFileSpec, vOFN.lpstrFile);
	 return FSaveFile (szAnsiFileSpec, FALSE);
     }
     return FALSE;
}

/****************************************************************************
 *
 * BOOL FAR PASCAL FnPageSetup (hwnd, message, wParam, lParam)
 *
 * purpose : Dialog function for page setup dialog. Accepts header/footer
 *	     formatting instructions and margin lengths.
 *
 * params  : same as for all dialog functions
 *
 *
 ***************************************************************************/

BOOL FAR APIENTRY FnPageSetup (
    HWND hwnd,
    WORD message,
    WPARAM wParam,
    LONG lParam)
{
    SHORT    id;
    WORD2DWORD          iSelFirst;
    WORD2DWORD          iSelLast;

    switch (message)
    {
        case WM_INITDIALOG :
            EnableWindow (GetDlgItem (hwnd, IDOK), TRUE);

            /* Set the Dialog Items to what they were before.  Also handles case
             * when Page Setup is chosen for the first time. */

            for (id=IDCN_EDITHEADER; id < IDCN_EDITHEADER+6; id++)
            {
                SendDlgItemMessage(hwnd, id, EM_LIMITTEXT, PT_LEN-1, 0L);
                SetDlgItemText(hwnd, id, (LPSTR)chPageText[id-IDCN_EDITHEADER]);
            }

            iSelLast = PT_LEN-1;
            iSelFirst = 0;
	    MSendMsgEM_GETSEL(GetDlgItem(hwnd, IDCN_EDITHEADER),
				&iSelFirst, &iSelLast);
            CalSetFocus (GetDlgItem (hwnd, IDCN_EDITHEADER));
            break;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam)) {
                case IDOK:
                    /* Store the changes made only if margins are valid nums. */
                    id = CheckMarginNums(hwnd);
                    if (id <= 0)       /* invalid margins */
                    {
                        MessageBox(hwnd, vszIncorrectSyntax, vszCalendar, MB_OK | MB_ICONEXCLAMATION);
                        if (id == 0)   /* can't guess invalid margin */
                            return TRUE;    /* continue the dialog */
                        else
                        {
                            CalSetFocus(GetDlgItem (hwnd, -id));
                            return FALSE;
                        }
                    }
                    /* store the margin values */

                    for (id = IDCN_EDITHEADER; id <= IDCN_EDITHEADER+6; id++)
                        GetDlgItemText(hwnd, id, chPageText[id-IDCN_EDITHEADER], PT_LEN-1);

                    EndDialog (hwnd, TRUE);
                    break;

              case IDCANCEL :
                 EndDialog (hwnd, FALSE);
                 break;
            }
            return TRUE;
    }

    return FALSE;
}

/* Check valididity of margin values specified.
 *  return TRUE if margins are valid.
 *
 *  returns  -IDCN_EDITMARGINLEFT if Left margin is invalid,
 *           -IDCN_EDITMARGINRIGHT if Right margin is invalid
 *           -IDCN_EDITMARGINTOP   if Top margin is invalid
 *           -IDCN_EDITMARGINBOTTOM if Bottom margin is invalid
 *           0/FALSE if it cannot guess the invalid margin
 */

INT CheckMarginNums(HWND hWnd)
    {
    SHORT   n;
    CHAR    *pStr;
    CHAR    szStr[PT_LEN];
    WORD    Left, Right, Top, Bottom;
    HANDLE  hPrintDC;
    WORD    xPixInch, yPixInch, xPrintRes, yPrintRes;

    for (n = IDCN_EDITHEADER+2; n < IDCN_EDITHEADER+6; n++)
        {
        GetDlgItemText(hWnd, n, szStr, PT_LEN);
        pStr = szStr;

        while (*pStr)
            if (isdigit(*pStr) || *pStr == szDec[0])
                pStr = AnsiNext(pStr);
            else
                return (-n);
        }

    if (((INT)(hPrintDC = (HANDLE)GetPrinterDC())) < 0)
        return FALSE;

    xPrintRes = GetDeviceCaps(hPrintDC, HORZRES);
    yPrintRes = GetDeviceCaps(hPrintDC, VERTRES);
    xPixInch  = GetDeviceCaps(hPrintDC, LOGPIXELSX);
    yPixInch  = GetDeviceCaps(hPrintDC, LOGPIXELSY);

    DeleteDC(hPrintDC);

    /* margin values have int/float values. Do range check */
    GetDlgItemText(hWnd, IDCN_EDITMARGINLEFT, szStr, PT_LEN);
    Left     = atopix(szStr,xPixInch);

    GetDlgItemText(hWnd, IDCN_EDITMARGINRIGHT, szStr, PT_LEN);
    Right    = atopix(szStr, xPixInch);

    GetDlgItemText(hWnd, IDCN_EDITMARGINTOP, szStr, PT_LEN);
    Top      = atopix(szStr, yPixInch);

    GetDlgItemText(hWnd, IDCN_EDITMARGINBOT, szStr, PT_LEN);
    Bottom   = atopix(szStr, yPixInch);

    /* try to guess the invalid margin */

    if (Left >= xPrintRes)
        return -IDCN_EDITMARGINLEFT;        /* Left margin is invalid */
    else if (Right >= xPrintRes)
        return -IDCN_EDITMARGINRIGHT;       /* Right margin is invalid */
    else if (Top >= yPrintRes)
        return -IDCN_EDITMARGINTOP;         /* Top margin is invalid */
    else if (Bottom >= yPrintRes)
        return -IDCN_EDITMARGINBOT;         /* Bottom margin is invalid */
    else if (Left >= (xPrintRes-Right))
        return FALSE;                   /* can't guess, return FALSE */
    else if (Top >= (yPrintRes-Bottom))
        return FALSE;                   /* can't guess, return FALSE */

    return TRUE;
    }

/**** FnDate */

BOOL APIENTRY FnDate (
     HWND hwnd,
     WORD message,
     WPARAM wParam,
     LONG lParam)
     {
     CHAR szDate [CCHDASHDATE];
     INT    iErr;

     switch (message)
	  {
	  case WM_INITDIALOG:
	       /* Remember the window handle of the dialog for AlertBox. */
	       vhwndDialog = hwnd;

	       SetDlgItemText (hwnd, IDCN_TODATE, (LPSTR)"");
	       return (TRUE);

	  case WM_COMMAND:
		switch (GET_WM_COMMAND_ID(wParam, lParam)) {
		   case IDOK:
			 GetDlgItemText (hwnd, IDCN_TODATE, (LPSTR)szDate, CCHDASHDATE);

			 if ((iErr = FD3FromDateSz (szDate, &vd3To)) == 0)
			      EndDialog (hwnd, TRUE);
			 else
			      {
			      /* Error in date - put up message box. */
			      DateTimeAlert(TRUE, iErr);
			      /* line added to fix keyboard hang problem
				 when running under 1.11 */
			      CalSetFocus (GetDlgItem(hwnd, IDCN_TODATE));
                              /* Select entire contents of edit control. */
                              /*  16 July 1989  Clark Cyr                */
			      SendDlgItemMessage(hwnd, IDCN_TODATE, EM_SETSEL,
                                      0, (DWORD)(-1L));
			      }
			 break;

		    case IDCANCEL:
			 EndDialog (hwnd, FALSE);
			 break;
		    }
	       return (TRUE);
	  }

     /* Tell Windows we did not process the message. */
     return (FALSE);
     }



/**** FnControls */

BOOL APIENTRY FnControls (
     HWND hwnd,
     WORD message,
     WPARAM wParam,
     LONG lParam)
     {
     static BOOL fSound;
     static WORD cMinEarlyRing;

     BOOL fOk;

     switch (message)
	  {
	  case WM_INITDIALOG:
	       /* Remember the window handle of the dialog for AlertBox. */
	       vhwndDialog = hwnd;

	       CheckDlgButton (hwnd, IDCN_SOUND, fSound = vfSound);
	       SetDlgItemInt (hwnd, IDCN_EARLYRING,
		cMinEarlyRing = vcMinEarlyRing, FALSE);
	       return (TRUE);

	  case WM_COMMAND:
		switch (GET_WM_COMMAND_ID(wParam, lParam)) {
		    case IDOK:
			 cMinEarlyRing = GetDlgItemInt (hwnd, IDCN_EARLYRING,
			      &fOk, FALSE);

			 if (!fOk || cMinEarlyRing > 10)
			      {
			      /* The value didn't parse or it's not within
				 range.  Put up an error message.
			      */
			      AlertBox (vszBadEarlyRing,
			       (CHAR *)NULL,
			       MB_APPLMODAL | MB_OK | MB_ICONASTERISK);

			      /* Don't end the dialog - they will have to
				 enter a proper time or cancel.
			      */
			      /* line added to fix keyboard hanging problem
				 in ver 1.11 */
			      CalSetFocus (GetDlgItem( hwnd, IDCN_EARLYRING));
			      break;
			      }

			 /* Assign the new values. */
			 vfSound = fSound;
			 vcMinEarlyRing = cMinEarlyRing;

			 EndDialog (hwnd, TRUE);
			 break;

		    case IDCANCEL:
			 EndDialog (hwnd, FALSE);
			 break;

		    case IDCN_SOUND:
			 CheckDlgButton (hwnd, IDCN_SOUND,
			  !IsDlgButtonChecked (hwnd, IDCN_SOUND));
			 fSound = !fSound;
			 break;
		    }
	       return (TRUE);
	  }

     /* Tell Windows we did not process the message. */
     return (FALSE);
     }




/**** FnSpecialTime */

BOOL APIENTRY FnSpecialTime (
     HWND hwnd,
     WORD message,
     WPARAM wParam,
     LONG lParam)
     {
     static INT idcheck;
     CHAR szTime [CCHTIMESZ];
     register CHAR *szError;
     register TM tm;
     INT	iErr;

     switch (message)
	  {
	  case WM_INITDIALOG:
	       /* Remember the window handle of the dialog for AlertBox. */
	       vhwndDialog = hwnd;

	       /* If the currently selected appointment is not for a regular
		  time, put the time into the edit control as the default.
		  This makes it easy for the user to delete the special time.
	       */
	       if ((tm = vtld [vlnCur].tm) % vcMinInterval != 0)
		    {
		    GetTimeSz (tm, szTime);
		    SetDlgItemText (hwnd, IDCN_EDIT, (LPSTR)szTime);
		    }

	       /* Enable or disable the Insert and Delete buttons. */
	       CheckButtonEnable (hwnd, IDCN_INSERT, EN_CHANGE);
	       CheckButtonEnable (hwnd, IDCN_DELETE, EN_CHANGE);

	       /* Check if the display is in 24Hr format; If so, disable the
	        * AM/PM radio buttons;
		* Fix for Bug #5447 --SANKAR-- 10-19-89;
		*/
	       if(vfHour24)
	         {
		   EnableWindow(GetDlgItem(hwnd, IDCN_AM), FALSE);
		   EnableWindow(GetDlgItem(hwnd, IDCN_PM), FALSE);
		 }
	       else
	           CheckRadioButton (hwnd, IDCN_AM, IDCN_PM, viAMorPM);
	       return (TRUE);


	  case WM_COMMAND:
		switch (GET_WM_COMMAND_ID(wParam, lParam)) {
		    case IDCN_EDIT:
			 CheckButtonEnable (hwnd, IDCN_INSERT, EN_CHANGE);
				//- FnSpecialTime: Changed GetParam to EN_CHANGE.
				//- Not sure why the old one ever worked.
				//- GET_WM_COMMAND_ID(wParam, lParam));
			 CheckButtonEnable (hwnd, IDCN_DELETE, EN_CHANGE);
				//- FnSpecialTime: Changed GetParam to EN_CHANGE.
				//- GET_WM_COMMAND_ID(wParam, lParam));
			 break;

		    case IDCANCEL:
			 EndDialog (hwnd, FALSE);
			 break;

		    case IDCN_AM:
		    case IDCN_PM:
			 viAMorPM = GET_WM_COMMAND_ID(wParam, lParam);
			 CheckRadioButton (hwnd, IDCN_AM, IDCN_PM, viAMorPM);
			 break;

		    case IDOK:
		    case IDCN_INSERT:
			 vfInsert = TRUE;
			 szError = vszTimeAlreadyInUse;
			 goto InsDel;

		    case IDCN_DELETE:
			 vfInsert = FALSE;
			 szError = vszNoSuchTime;
InsDel:

			 GetDlgItemText (hwnd, IDCN_EDIT,
			  (LPSTR)szTime, CCHTIMESZ);
			 if ((iErr = FGetTmFromTimeSz (szTime, &vtmSpecial)) < 0)
			      {
			      /* Error in time - put up message box. */
			      DateTimeAlert(FALSE, iErr);
			      /* Don't end the dialog - they will have to
				 enter a proper time or cancel.
			      */
			      CalSetFocus (GetDlgItem (hwnd, IDCN_EDIT));

			      break;
			      }

			 /* If we are in 24Hr format, then we should not allow
			  * the AM/PM radiobuttons to override;
			  * Fix for Bug #5447 --SANKAR-- 10-19-89
			  */
			 if(!vfHour24)
			   {
			     /* AM or PM value on radiobutton overrides
			        text text in edit window */

                             if (vtmSpecial < TWELVEHOURS)
                               {
                                 if (viAMorPM == IDCN_PM)
                                     vtmSpecial += TWELVEHOURS;
                               }
                             else if (vtmSpecial > TWELVEHOURS)
                               {
                                 if (viAMorPM == IDCN_AM)
                                     vtmSpecial -= TWELVEHOURS;
                               }
			   }

			 /* Don't allow a regular time to be inserted or
			    deleted.  Note
			    that it's possible for the special time bit for
			    a regular time to be set, but it still can't
			    be deleted.  For example, while the interval
			    in 60, insert a special time of 8:30.  Then
			    switch the interval to 30.	While the interval
			    is 30 (or 15), the 8:30 time cannot be deleted.
			 */
			 if (vtmSpecial % vcMinInterval == 0)
			      {
			      /* Not a special time. */
			      AlertBox (vszNotSpecialTime,
			       (CHAR *)NULL, MB_APPLMODAL | MB_OK
			       | MB_ICONASTERISK);
			      break;
			      }

			 /* Don't allow insert if time already in tqr.
			    Don't allow delete if time not in tqr.
			    OK to use bitwise xor since TRUE == 1 and
			    FALSE == 0.
			 */
			 if (!(FSearchTqr (vtmSpecial) ^ vfInsert))
			      {
			      AlertBox (szError, (CHAR *)NULL,
			       MB_APPLMODAL | MB_OK| MB_ICONASTERISK);
			      break;
			      }

			 EndDialog (hwnd, TRUE);
			 break;
		    }
	       return (TRUE);
	  }

     /* Tell Windows we did not process the message. */
     return (FALSE);
     }




/**** FnDaySettings */

BOOL APIENTRY FnDaySettings (
     HWND hwnd,
     WORD message,
     WPARAM wParam,
     LONG lParam)
     {
     static INT idcnInterval;
     static INT idcnHourFormat;
     INT    vfHour24New;
     INT    iErr;
     CHAR szTime [CCHTIMESZ];

     switch (message)
	  {
	  case WM_INITDIALOG:
	       /* Remember the window handle of the dialog for AlertBox. */
	       vhwndDialog = hwnd;

	       CheckRadioButton (hwnd, IDCN_HOUR12, IDCN_HOUR24,
		idcnHourFormat = IDCN_HOUR12 + vfHour24);
	       GetTimeSz (vtmStart, szTime);
	       SetDlgItemText (hwnd, IDCN_STARTINGTIME, (LPSTR)szTime);

	       /* We want the current setting of interval to be checked,
		  and we want the interval group of radio buttons to have
		  the focus.  It doesn't work to call CheckRadioButton
		  here and then let Windows set the focus to the first
		  TABGRP in the dialog box, since when it sets the focus,
		  it also sends us a message to check the first button in
		  the group (so we would always get 15 checked instead
		  of the current setting).  So we do our own SetFocus
		  to the interval button we want to check.  We then return
		  FALSE to tell Windows that we have done our own SetFocus.
	       */
	       CalSetFocus (GetDlgItem (hwnd, idcnInterval = IDCN_MIN15 +
		vmdInterval));
	       return (FALSE);

	  case WM_COMMAND:
		switch (GET_WM_COMMAND_ID(wParam, lParam)) {
		    case IDOK:
			 GetDlgItemText (hwnd, IDCN_STARTINGTIME,
                                         (LPSTR)szTime, CCHTIMESZ);
			 /* Note - FGetTmFromTimeSz does not affect
			    vtmStart if it returns FALSE (i.e., if it detects
			    an error in the time string).
			 */
			 if ((iErr = FGetTmFromTimeSz (szTime, &vtmStart)) < 0)
			      {
			      /* Error in time - put up message box. */
			      DateTimeAlert(FALSE, iErr);

			      /* Don't end the dialog - they will have to
				 enter a proper time or cancel.
			      */
			      /* line added to fix keyboard hang problem
				 while running under 3.0 ver 1.11 */
			      CalSetFocus (GetDlgItem (hwnd, IDCN_STARTINGTIME));
			      break;
			      }

			 vfHour24New = idcnHourFormat - IDCN_HOUR12;
                         if (vfHour24 != vfHour24New)
                            {
			    vfHour24 = vfHour24New;
			    InitTimeDate(vhInstance, vfHour24 ? GTS_24HOUR : GTS_12HOUR);
                             }

			 switch (vmdInterval = idcnInterval - IDCN_MIN15)
			      {
			      case MDINTERVAL15:
				   vcMinInterval = 15;
				   break;

			      case MDINTERVAL30:
				   vcMinInterval = 30;
				   break;

			      case MDINTERVAL60:
				   vcMinInterval = 60;
				   break;
			      }
			 EndDialog (hwnd, TRUE);
			 break;

		    case IDCANCEL:
			 EndDialog (hwnd, FALSE);
			 break;

		    case IDCN_MIN15:
		    case IDCN_MIN30:
		    case IDCN_MIN60:
			 CheckRadioButton (hwnd, IDCN_MIN15, IDCN_MIN60,
			    idcnInterval = GET_WM_COMMAND_ID(wParam, lParam));
			 break;

		    case IDCN_HOUR12:
		    case IDCN_HOUR24:
			 CheckRadioButton (hwnd, IDCN_HOUR12, IDCN_HOUR24,
			    idcnHourFormat = GET_WM_COMMAND_ID(wParam, lParam));
			 break;
		    }
	       return (TRUE);
	  }

     /* Tell Windows we did not process the message. */
     return (FALSE);
     }


/****************************************************************
 *
 * BOOL FAR PASCAL FnMarkDay ( hwnd, message, wParam, lParam)
 *
 * purpose : puts up Options Mark dialog box and receives the
 *	     marks that are to be put up against the current day.
 *
 ***************************************************************/

BOOL APIENTRY FnMarkDay (
    HWND hwnd,
    WORD message,
    WPARAM wParam,
    LONG  lParam)
    {
    BOOL checkbutton = FALSE;

    switch (message)
	{
	case WM_INITDIALOG:

	    /* check buttons corresp. to active marks on selected day */

	    CheckDlgButton( hwnd, IDCN_MARKBOX,  (viMarkSymbol & MARK_BOX));
	    CheckDlgButton( hwnd, IDCN_MARKPARENTHESES, (viMarkSymbol & MARK_PARENTHESES));
	    CheckDlgButton( hwnd, IDCN_MARKCIRCLE, (viMarkSymbol & MARK_CIRCLE));
	    CheckDlgButton( hwnd, IDCN_MARKCROSS, (viMarkSymbol & MARK_CROSS));
	    CheckDlgButton( hwnd, IDCN_MARKUNDERSCORE, (viMarkSymbol & MARK_UNDERSCORE));

	    CalSetFocus (GetDlgItem (hwnd, IDCN_MARKBOX));
	    return FALSE;

	  case WM_COMMAND:
		switch (GET_WM_COMMAND_ID(wParam, lParam)) {
		case IDOK:

		    viMarkSymbol = 0; /* clear existing bit pattern. This
				      is done so that only the marks specified
				      in dialog will appear on current day */

		    /* bits corresp. to selected mark symbols are set in MarkSymbol */

		    if (IsDlgButtonChecked(hwnd, IDCN_MARKBOX))
			viMarkSymbol |= MARK_BOX;

		    if (IsDlgButtonChecked(hwnd, IDCN_MARKPARENTHESES))
			viMarkSymbol |= MARK_PARENTHESES;

		    if (IsDlgButtonChecked(hwnd, IDCN_MARKCIRCLE))
			viMarkSymbol |= MARK_CIRCLE;

		    if (IsDlgButtonChecked(hwnd, IDCN_MARKCROSS))
			viMarkSymbol |= MARK_CROSS;

		    if (IsDlgButtonChecked(hwnd, IDCN_MARKUNDERSCORE))
			viMarkSymbol |= MARK_UNDERSCORE;

		    EndDialog (hwnd, TRUE);

		    break;

		case IDCANCEL:
		    EndDialog (hwnd, FALSE);
		    break;

		default :
		    return FALSE;
		} /* switch wParam */
            break;

	default :
            return FALSE;

	} /* switch message */
    return TRUE;

    } /* FnMark */



/**** AlertBox */

INT  APIENTRY AlertBox (
     CHAR *szText1,
     CHAR *szText2,
     WORD wStyle)
     {
     CHAR szMessage [160+128];
     register CHAR *pch;
     register HWND hwnd;

     MergeStrings(szText1, szText2, szMessage);

     MessageBeep (wStyle);

     /* The parent window is the active dialog if there is one.  If no
	active dialog, the parent window is Calendar's tiled window.
     */
     if ((hwnd = vhwndDialog) == (HWND)NULL)
	  hwnd = vhwnd0;
     return (MessageBox (hwnd, (LPSTR)szMessage, (LPSTR)vszCalendar, wStyle));
     }



/**** Scan sz1 for merge spec.	If found, insert string sz2 at that point.
      Then append rest of sz1 NOTE! Merge spec guaranteed to be two chars.
      returns TRUE if it does a merge, false otherwise. */
BOOL  APIENTRY MergeStrings(
    CHAR    *szSrc,
    CHAR    *szMerge,
    CHAR    *szDst)
    {
    register	CHAR *pchSrc;
    register	CHAR *pchDst;

    pchSrc = szSrc;
    pchDst = szDst;

#ifdef DBCS
    /* Find merge spec if there is one. */
    //- Merge Bytes: Changed to string to avoid word boundry crossing.
    while (*pchSrc != *vszMergeStr || *(pchSrc + 1) != *(vszMergeStr + 1))
        {
        if( IsDBCSLeadByte( *pchSrc ) )
            *pchDst++ = *pchSrc++;
        *pchDst++ = *pchSrc;

	/* If we reach end of string before merge spec, just return. */
        if(!*pchSrc++)
            return FALSE;
    }
#else
    /* Find merge spec if there is one. */
    //- Merge Bytes: Changed to string to avoid word boundry crossing.
    while (*pchSrc != *vszMergeStr || *(pchSrc + 1) != *(vszMergeStr + 1))
        {
	*pchDst++ = *pchSrc;

	/* If we reach end of string before merge spec, just return. */
	if (!*pchSrc++)
	    return FALSE;

        }
#endif

    /* If merge spec found, insert sz2 there. (check for null merge string */
    if (szMerge)
	while (*szMerge)
	    *pchDst++ = *szMerge++;

    /* Jump over merge spec */
    pchSrc++,pchSrc++;

    /* Now append rest of Src String */
    while (*pchDst++ = *pchSrc++);
    return TRUE;
    }




/**** If sz does not have extension, append ".TXT" */
VOID APIENTRY AddDefExt (LPSTR sz)
    {
    LPSTR pch1;
    register INT ch;

    pch1 = sz + lstrlen(sz);

    while ((ch = *pch1) != '.' && ch != '\\' && ch != ':' && pch1 > sz)
	pch1 = AnsiPrev(sz, pch1);

    if (*pch1 != '.')
	lstrcat(sz, (LPSTR)vrgsz[IDS_FILEEXTENSION]);
    }


/**** CheckButtonEnable - Enable specified button if edit field not null,
      disable the button if the edit field is null.
*/

VOID APIENTRY CheckButtonEnable (
     HWND hwnd,
     INT  idButton,
     WORD message)
     {
     if (message == EN_CHANGE)
	  {
	  EnableWindow (GetDlgItem (hwnd, idButton),
	   (SendMessage (GetDlgItem (hwnd, IDCN_EDIT),
	   WM_GETTEXTLENGTH, 0, 0L)));
	  }
     }


/* FCheckSave - if the calendar is dirty see if the user wants to Save the
   the changes.
*/

BOOL APIENTRY FCheckSave (BOOL fSysModal)
     {
     /* Force current edits to be recorded right away since this may cause
	the file to be dirty.
      */

     RecordEdits ();

     if (vfDirty)
	  {
	  /* if file has been opened as read only, warn user and return */

          if (vfOpenFileReadOnly)
              {
              AlertBox (vszFileReadOnly, (CHAR *)NULL, MB_APPLMODAL|MB_OK|
                        MB_ICONEXCLAMATION);

              /* Force the Save As dialog letting user save under a
               * different name, thus not losing changes. */

	      CallSaveAsDialog();
	      return (TRUE);
              }

	  switch (AlertBox (vszSaveChanges, vszFileSpec,
                 (fSysModal ? MB_SYSTEMMODAL : MB_APPLMODAL) | MB_YESNOCANCEL
                 | MB_ICONEXCLAMATION))
	       {
	       case IDYES:
		    if (vfOriginalFile)
			 return (FSaveFile (vszFileSpec, TRUE));
		    else
			 return CallSaveAsDialog();

	       case IDCANCEL:
		    return (FALSE);

	       /* The IDNO case falls through to return TRUE. */
	       }
	  }
    return (TRUE);
    }




/**** RecordEdits - if the notes or the appointment description edit
      control has the focus, record the contents of the edit control
      without changing the focus.
*/

VOID APIENTRY RecordEdits ()
     {
     register HWND hwndFocus;

     if ((hwndFocus = GetFocus ()) == vhwnd2C)
	  StoreNotes ();
     else if (hwndFocus == vhwnd3)
	  StoreQd ();
     }



/**** Display error when user types date or time in wrong Format */
VOID APIENTRY DateTimeAlert(
    BOOL    fDate,
    INT     iErr)
    {
    CHAR    sz[256];
    CHAR    *pch1;
    CHAR    *pch2 = 0;
    DOSDATE dd;

    switch (iErr)
        {
	/* Range error only occurs for dates */
	case PD_ERRRANGE:
	    pch1 = vrgsz[IDS_DATERANGE];
	    break;

	case PD_ERRSUBRANGE:
	    pch1 = vrgsz[fDate ? IDS_DATESUBRANGE : IDS_TIMESUBRANGE];
	    break;

	case PD_ERRFORMAT:
	    pch1 = fDate ? vrgsz[IDS_BADDATE] : vrgsz[IDS_BADTIME];
	    pch2 = sz;
	    if (fDate)
		{
		dd.month = vd3Sel.wMonth + 1;
		dd.day = vd3Sel.wDay + 1;
		dd.year = vd3Sel.wYear + 1980;
		GetDateString(&dd, pch2, GDS_SHORT);
		}
	    else
		GetTimeSz(vftCur.tm, pch2);
	    break;
        }

    AlertBox (pch1, pch2, MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
    }
