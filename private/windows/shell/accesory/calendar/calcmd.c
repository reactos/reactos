/*
 *   Windows Calendar
 *   Copyright (c) 1985 by Microsoft Corporation, all rights reserved.
 *   Written by Mark L. Chamberlin, consultant to Microsoft.
 *
 ****** calcmd.c
 *
*/

/* Get rid of more stuff from windows.h */
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOSYSCOMMANDS
#define NOMEMMGR
#define NOCLIPBOARD

#include "cal.h"
#include <shellapi.h>



/**** CalCommand - process menu command. */

VOID APIENTRY CalCommand (
     HWND hwnd,
     INT  idcm)
     {
     register HDC  hDC;
     register DT   dt;
     HWND          hwndFocus;
     DWORD        iSelFirst;
     DWORD        iSelLast;
     INT           tmStart12;
     INT           itdd;
     DD            *pdd;
     BOOL          fTemp;

     /* Make the target date the same as the selected date.  Several
        commands depend on this.
     */
     vd3To = vd3Sel;

     switch (idcm)
	  {
          case IDCM_ABOUT:
               ShellAbout(hwnd, vrgsz[IDS_CALENDAR], (LPSTR)"", 
                          LoadIcon((HWND)GetWindowLong(hwnd, GWL_HINSTANCE),
						  MAKEINTRESOURCE(1)));
               break;

          case IDCM_EXIT:
               PostMessage(hwnd, WM_CLOSE, 0, 0L);
               break;

	  case IDCM_NEW:

               if (FCheckSave (FALSE))
                   {
                   vfOpenFileReadOnly = FALSE;  /* in case earlier file was readonly */
                   CleanSlate (TRUE);
                   }
               break;

          case IDCM_OPEN:
               if (FCheckSave (FALSE))
                   {
                   vfOpenFileReadOnly = FALSE;  /* in case earlier file was readonly */
                   OpenCal ();
                   }
	       break;

	  case IDCM_SAVE:
	       if (vfOpenFileReadOnly)
		  {
		  AlertBox (vszFileReadOnly, (CHAR *)NULL,MB_APPLMODAL|MB_OK|
							  MB_ICONEXCLAMATION);
		  break;
                   }
               else
                   if (vfOriginalFile)
                       {
                       hwndFocus = GetFocus();
                       FSaveFile (vszFileSpec, TRUE);
                       SetFocus(hwndFocus);
                       break;
                       }

               /* There is no original file, which means we are still
                  untitled, so we can't do a Save without getting a file
                  name from the user.  Fall into the Save As to do so.
               */


	  case IDCM_SAVEAS:
	       CallSaveAsDialog();
               break;

          case IDCM_PAGESETUP:
               FDoDialog (IDD_PAGESETUP);
	       break;

          case IDCM_PRINT:
	       if (FDoDialog (IDD_PRINT))
                    Print ();
               break;

	  case IDCM_PRINTERSETUP:
	       vPD.Flags |= PD_PRINTSETUP;  /* invoke only the Setup dialog */
	       bPrinterSetupDone = PrintDlg ((LPPRINTDLG)&vPD);
               break;


          case IDCM_REMOVE:
               if (FDoDialog (IDD_REMOVE))
                    Remove ();
               break;

          /* Guy hit delete key - either shifted or unshifted. */
          case IDCM_DEL:

               /* If window with focus is not edit ctl, nop. */
               if ((hwndFocus = GetFocus()) == vhwnd2B)
                    break;

               /* Do something only if there is a non-null selection. */
	       MSendMsgEM_GETSEL(hwndFocus, &iSelFirst, &iSelLast);
               if (iSelFirst != iSelLast)
                    {
                    /* If shifted delete, do a cut (which is a menu function.) */
                    if ((GetKeyState(VK_SHIFT) < 0) || (GetKeyState(VK_DELETE) >= 0))
                        {
                        HiliteMenuItem(vhwnd0, GetMenu(vhwnd0), IDCM_CUT, MF_HILITE | MF_BYCOMMAND);
                        SendMessage(hwndFocus, WM_CUT, (WORD)0, 0L);
                        HiliteMenuItem(vhwnd0, GetMenu(vhwnd0), IDCM_CUT, MF_BYCOMMAND);

                    /* Otherwise, do a clear (which is not a menu function.) */
                        }
                    else
                        SendMessage(hwndFocus, WM_CLEAR, (WORD)0, 0L);

                   }
               break;

          /* Note - Cut, Copy, and Paste are only enabled when
             one of the edit controls has the focus, so we know
             it's OK to just send the command to the edit control.
          */
          case IDCM_CUT:
               SendMessage (GetFocus (), WM_CUT, (WORD)0, 0L);
               break;

          case IDCM_COPY:
               SendMessage (GetFocus (), WM_COPY, (WORD)0, 0L);
               break;

          case IDCM_PASTE:
               SendMessage (GetFocus (), WM_PASTE, (WORD)0, 0L);
               break;

          case IDCM_DAY:
               DayMode (&vd3Sel);
               break;

	  case IDCM_MONTH:

	       vmScrollPos = 0;
	       hmScrollPos = 0;
	       MonthMode();
               break;

          case IDCM_TODAY:
               if (vfDayMode)
                    SwitchToDate (&vd3Cur);
               else
                   {
                   /* added setfocus to fix bug where if in another month,
                    * and focus in notes area, today's notes would get
                    * overwritten with the current notes. why? who knows!
                    */
                   CalSetFocus(vhwnd2B);
                   JumpDate (&vd3Cur);
                   }
               break;

          case IDCM_PREVIOUS:
               if (vfDayMode)
                    {
                    if ((dt = DtFromPd3 (&vd3Sel)) != DTFIRST)
                         {
                         GetD3FromDt (--dt, &vd3To);
                         SwitchToDate (&vd3To);
                         }
                    }
               else
                    {
		    /* Show the previous month . */
		    /* This  causes the new month window to be displayed
		       from week 1 */
		    vmScrollPos = 0;
		    SetScrollPos (vhwnd2B, SB_VERT, vmScrollPos,TRUE);
                    ShowMonthPrevNext (FALSE);
                    }
               break;

          case IDCM_NEXT:
               if (vfDayMode)
                    {
                    if ((dt = DtFromPd3 (&vd3Sel)) != DTLAST)
                         {
                         GetD3FromDt (++dt, &vd3To);
                         SwitchToDate (&vd3To);
                         }
                    }
               else
                    {
		    /* Show the next month. */
		    vmScrollPos = 0;
		    SetScrollPos (vhwnd2B, SB_VERT, vmScrollPos, TRUE);
                    ShowMonthPrevNext (TRUE);
                    }
               break;

          case IDCM_DATE:
               if (FDoDialog (IDD_DATE))
                    {
                    if (vfDayMode)
                         SwitchToDate (&vd3To);
                    else {
                         /* added setfocus to fix bug where if in another month,
                          * and focus in notes area, today's notes would get
                          * overwritten with the current notes. why? who knows!
                          */
                         CalSetFocus(vhwnd2B);
                         JumpDate (&vd3To);
                         }
                    }
               break;

          case IDCM_SET:
               AlarmToggle ();
               break;

          case IDCM_CONTROLS:
               if (FDoDialog (IDD_CONTROLS))
                    {
                    /* It's possible that the user just increased the
                       Early Ring period.  If so, it may be time for the
                       next alarm to go off.  Call AlarmCheck directly since
                       CalTimer won't do it until the minute changes, and
                       that might not be soon enough.
                       Note that shortening the Early Ring period does not
                       cause alarms that have previously gone off (due to
                       the older, longer Early Ring period) to go off again.
                       For example if the Early Ring period was 10, at 9:20
                       the 9:30 alarm went off.  If at 9:23 the user changes
                       the Early Ring period to 5 minutes, the 9:30 alarm
                       will not go off again at 9:25.  The user has already
                       seen the alarm.
                    */
                    AlarmCheck ();

                    /* Changing the alarm controls makes the file dirty.
                       Note that the user may not have actually changed
                       the settings, but he did push the OK button, and
                       that's close enough.  It would be a waste of code
                       to only set the dirty flag when the settings are
                       actually different.
		    */
                    vfDirty = TRUE;
                    }
               break;



	  case IDCM_MARK:
	       dt = DtFromPd3(&vd3Sel);    /* fetch selected day */
	       FSearchTdd (dt, &itdd);
	       pdd = TddLock() + itdd;
	       TddUnlock();

	       viMarkSymbol = pdd->fMarked;   /* set viMarkSymbol with active
					       marks on selected day */
	       /* show mark dialog */
	       fTemp =	(BOOL)DialogBox((HANDLE)vhInstance,
			MAKEINTRESOURCE(IDD_MARK), (HWND)vhwnd0,
			(DLGPROC)(lpfnMark = (FARPROC)FnMarkDay));
               vhwndDialog = (HWND)NULL;

	       if (fTemp)
                  CmdMark ();

	       break;

          /* Go Do help.  Since menu items match numbers in help file,
           * no need to change them before calling help.
           */
          case IDCM_HELP:
               WinHelp(hwnd, vszHelpFile, HELP_INDEX, 0L);
               break;

          case IDCM_USINGHELP:
               WinHelp(hwnd, (LPSTR)NULL, HELP_HELPONHELP, 0L);
               break;

          case IDCM_SEARCH:
               WinHelp(hwnd, vszHelpFile, HELP_PARTIALKEY, (DWORD)(LPSTR)"");
               break;

          case IDCM_SPECIALTIME:
               if (FDoDialog (IDD_SPECIALTIME))
                    {
		    if (vfInsert)
                         InsertSpecial ();
                    else
                         DeleteSpecial ();
                    }
               break;

          case IDCM_DAYSETTINGS:
               if (FDoDialog (IDD_DAYSETTINGS))
                    {
                    /* Changing the day settings makes the file dirty.
                       Note that the user may not have actually changed
                       the settings, but he did push the OK button, and
                       that's close enough.  It would be a waste of code
                       to only set the dirty flag when the settings are
                       actually different.
                    */
                    vfDirty = TRUE;

                    /* Redisplay the time since the clock format
                       may have been changed.
                    */
                    hDC = CalGetDC (vhwnd2A);
                    DispTime (hDC);
                    ReleaseDC (vhwnd2A, hDC);

                    if (vfDayMode)
                         {
                         /* Since the clock format, interval, and starting
                            hour all affect the day mode display, just
                            call DayMode to redisplay the whole works.
                         */
                         DayMode (&vd3Sel);
                         }
                    }
               break;

    /* Added new key functionality.  Ctrl+Home scrolls appointment
     * window to StartTime.  Ctrl+End scrolls to StartTime + 12 hours.
     * 26-Mar-1987.
     */
        case IDCM_START12:
                tmStart12 = vtmStart + 12 * 60; /* 12 hours later */
                if (tmStart12 > TMLAST-61)
                    tmStart12 = TMLAST-61;
                /* fall thru... */

        case IDCM_START:
            if (vfDayMode && (GetFocus() == vhwnd3))
                FScrollDay(SB_THUMBPOSITION,
                           ItmFromTm(idcm==IDCM_START ? vtmStart : tmStart12));
            break;
          }
     }




/**** FDoDialog - Do modal dialog. */

BOOL APIENTRY FDoDialog (INT  idd)
     {
     register INT fTemp;

     fTemp = DialogBox((HANDLE)vhInstance, MAKEINTRESOURCE(idd),
			(HWND)vhwnd0, (DLGPROC)vrglpfnDialog[idd-1]);
     /* Tell AlertBox there is no longer a dialog active. */
     vhwndDialog = (HWND)NULL;

     return (fTemp);

     }
