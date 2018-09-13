/*
 *   Windows Calendar
 *   Copyright (c) 1985 by Microsoft Corporation, all rights reserved.
 *   Written by Mark L. Chamberlin, consultant to Microsoft.
 *
*/

/*
 *****
 ***** calalarm.c
 *****
*/

/* Get rid of more stuff from windows.h */
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOSYSCOMMANDS
#define NOSCROLL
#define NODRAWTEXT
#define NOVIRTUALKEYCODES
#define NOCLIPBOARD

#include "cal.h"


/**** FAlarm - return TRUE if the ln has an alarm set. */

BOOL APIENTRY FAlarm (INT  ln)

     {

     BOOL fAlarm;
     WORD otqr;

     fAlarm = FALSE;
     if ((otqr = vtld [ln].otqr) != OTQRNIL)
          {
          fAlarm = ((PQR )(PbTqrLock () + otqr)) -> fAlarm;
          DrUnlockCur ();
          }
     return (fAlarm);
     }


/**** AlarmToggle - Here on Alarm Set command.
      Since the command is only enabled
      when the focus in on an ln, we know that all we need to do is
      toggle the alarm state for the ln that has the focus.
*/

VOID APIENTRY AlarmToggle ()

     {

     QR   qrNew;
     WORD otqr;
     register PQR pqr;
     TM   tm;
     RECT rect;
     BOOL fAlarm;
     BOOL fEmpty;
     register INT ln;
     DT   dt;
     INT  itdd;
     FT   ftTemp;
     DR   *pdr;

     if ((otqr = vtld [ln = vlnCur].otqr) == OTQRNIL)
          {
          /* There is no QR for this ln, so we know that it can't
             have an alarm set.  We create a QR for it with the
             alarm flag set.
          */
          qrNew.cb = CBQRHEAD + 1;
          fAlarm = qrNew.fAlarm = TRUE;
          qrNew.fSpecial = FALSE;
          qrNew.tm = tm = vtld [ln].tm;
          qrNew.qd [0] = TEXT('\0');

          /* Since we know there
             was no old QR, we know FSearchTqr will not find a
             match - so we ignore it's return value.
             We call it to set up the insertion point in votqrNext.
          */
          FSearchTqr (tm);

          /* If there's not enough room to insert the new QR,
             then the alarm cannot get set, so we don't want to
             alter cAlarm (or any of the
             other stuff that gets altered if the QR is inserted).
             Note that FinsertQr puts up the alert.
          */
          if (!FInsertQr (votqrNext, &qrNew))
               return;

          vtld [ln].otqr = votqrNext;

          /* Adjust up the otqrs in the tld beyond the current ln. */
          AdjustOtqr (ln, CBQRHEAD + 1);
          }

     else

          {
          /* There is a QR for this ln.  Toggle its alarm flag. */
          pqr = (PQR )(PbTqrLock () + otqr);
          fAlarm = pqr -> fAlarm = !pqr -> fAlarm;
          fEmpty = !fAlarm && !pqr -> fSpecial && pqr -> cb == CBQRHEAD + 1;
          DrUnlockCur ();

          if (fEmpty)
               {
               /* We can get rid of this QR now since it has no flags
                  set and it has a null appointment description.
               */
               DeleteQr (otqr);
               vtld [ln].otqr = OTQRNIL;

               /* Adjust down the otqrs in the tld beyond the current ln. */
               AdjustOtqr (ln, -(CBQRHEAD + 1));
               }
          }

     /* Get rid of or display the alarm bell icon. */
     rect.top = YcoFromLn (ln);
     rect.bottom = rect.top + vcyLineToLine;
     rect.right = (rect.left = vxcoBell) + vcxBell;
     InvalidateRect (vhwnd2B, (LPRECT)&rect, TRUE);
     UpdateWindow (vhwnd2B);

     /* Set the dirty flags, and adjust the count of alarms for this date. */
     (pdr = PdrLockCur ()) -> fDirty = vfDirty = TRUE;
     dt = pdr -> dt;
     DrUnlockCur ();
     FSearchTdd (dt, &itdd);
     (TddLock () + itdd) -> cAlarms += fAlarm ? 1 : -1;
     TddUnlock ();

     ftTemp.dt = dt;
     ftTemp.tm = vtld [ln].tm;
     if (fAlarm)
          {
          /* Setting an alarm. */
          if (CompareFt (&ftTemp, &vftAlarmNext) == -1
           && CompareFt (&ftTemp, &vftCur) > -1)
               {
               /* The alarm being set is less than the next armed alarm
                  and it is greater than or equal to the current time.
                  Make it the next alarm, and see if it needs to go off
                  right now.  (Waiting for it to go off "naturally" could
                  result in its being a minutue too late since the alarms
                  are only checked when the minute changes.)
               */
               vftAlarmNext = ftTemp;
               AlarmCheck ();
               }
          }
     else
          {
          /* Cancelling an alarm. */
          if (CompareFt (&ftTemp, &vftAlarmNext) == 0)
               {
               /* Cancelling the next armed alarm.  Need to arm the one
                  after it.  Since the one we are cancelling has not yet
                  gone off, it can't be time for the one after it to
                  go off either, so there is no need to call AlarmCheck -
                  just let it go off naturally.
               */
               GetNextAlarm (&vftCur, &vftCur, TRUE, (HWND)NULL);
               }
          }
     }




/**** ProcessAlarms */

VOID APIENTRY FAR ProcessAlarms ()
     {
     static BOOL vfLocked = FALSE;


     /* This routine is locked to prevent reentry.  This is done to prevent
        an alarm dialog from getting put up on top of another one.  In
        addition to being less confusing for the user, this avoids the nasty
        problems of running out of resources (such as stack and heap).
        The reason that reentry may occur is that we arm the next alarm
        in the process of putting up the dialog, and while the dialog is
        waiting for input, timer messages can come in and the next alarm
        can be triggered.  This is a desirable feature
        since it means the user still hears the audible alarm and we flash
        the window if the next alarm goes off before the OK button is pressed
        for the current alarm dialog (a likely scenario if the user is
        not at the machine but left it with the focus on Calendar).
        As soon as the user pushes the OK button for the current dialog,
        another dialog will be put up to show the new alarms.  This is
        done by looping here until there are no new alarms.
     */

     /* Only enter if the routine is not locked. */
     if (!vfLocked)
          {
          /* Lock this routine to prevent reentry. */
          vfLocked = TRUE;

          /* vftAlarmFirst.dt will get set to DTNIL when the next alarm
             gets armed (during the ACKALARMS dialog).  If that alarm
             gets triggered while the dialog box is up, vdtAlarmFirst
             will get set to that alarm time by AlarmCheck.  We continue
             putting up dialog boxes as long as alarms go off while
             the previous one is up.
          */
          while (vftAlarmFirst.dt != DTNIL)
               {
               /* Quit flashing. */
               StartStopFlash (FALSE);

               if (vfMustSyncAlarm)
                  {
                  /* The system clock was changed so we need to
                     resynchronize the alarms.  Tell the user about it.
                  */
                  AlertBox (vszAlarmSync, (TCHAR *)NULL,
                   MB_SYSTEMMODAL | MB_OK | MB_ICONEXCLAMATION);

                  /* Now that the user has had his last chance to mess
                     with the clock (could have changed it again
                     during the application modal alert), reset the
                     flag.  We will resync to the latest time that's
                     been read by CalTimer.
                  */
                  vfMustSyncAlarm = FALSE;

                  /* Arm the first alarm >= the current time. */
                  GetNextAlarm (&vftCur, &vftCur, TRUE, (HWND)NULL);

                  /* Say there are no unacknowledged alarms. */
                  vftAlarmFirst.dt = DTNIL;

                  /* See if the alarm must go off immediately.  If so,
                     AlarmCheck will make vftAlarmFirst something other
                     than DTNIL, so this loop will continue.
                  */
                  AlarmCheck();
                  }
               else
                  {
                   /* Show the alarms that have been triggered, returning
                      here after the user presses the OK button.
                   */
                   FDoDialog (IDD_ACKALARMS);
                  }
               }

          /* Unlock this routine. */
          vfLocked = FALSE;
          }
     }




/**** FnAckAlarms */

BOOL APIENTRY FnAckAlarms (
    HWND hwnd,
    WORD message,
    WPARAM wParam,
    LONG lParam)

     {

     FT   ftTemp;

     switch (message)
          {
          case WM_INITDIALOG:
               /* Remember the window handle of the dialog for AlertBox. */
               vhwndDialog = hwnd;

               /* Fill the list box, and arm the next alarm.  The first
                  alarm to put in the list box is the first unacknowledged
                  one (which is in vftAlarmFirst).  The next alarm to arm
                  (the one following the last one to go into the list
                  box), is the first one > the current time + the early
                  ring period.  Since GetNextAlarm works with >=, add
                  one more than the early ring period.
               */
               ftTemp = vftCur;
               AddMinsToFt (&ftTemp, (WORD)(vcMinEarlyRing + 1));
               GetNextAlarm (&vftAlarmFirst, &ftTemp, TRUE, hwnd);

               /* Say there are no unacknowledged alarms.  Note that
                  there has not been an opportunity for the alarm just
                  armed by GetNextAlarm to be triggered, since the last
                  thing that routine does is arm the alarm so no error
                  dialogs could have occurred after arming, and consequently
                  we could not have yielded and processed a timer message.
               */
               vftAlarmFirst.dt = DTNIL;
               return (TRUE);

          case WM_COMMAND:
               if (GET_WM_COMMAND_ID(wParam, lParam) == IDOK)
                    {
                    EndDialog (hwnd, TRUE);
                    return (TRUE);
                    }
               /* Fall into default case if WM_COMMAND is not from IDOK. */
          default:
               /* Tell Windows we did not process the message. */
               return (FALSE);
          }
     }


/**** GetNextAlarm */

VOID APIENTRY FAR GetNextAlarm (

    FT   *pftStart,     /* Start looking at alarms >= this. */
    FT   *pftStop,      /* Stop when find alarm >= this.  If an alarm >=
                           this is found, arm it.
                        */
    BOOL fDisk,         /* Ok to read from disk if this is TRUE.  If FALSE,
                           give up if the next alarm is not in memory.
                        */
    HWND hwnd)          /* If not null, use this handle to send the triggered
                           alarms to the list box in the alarm acknowledgement
                           dialog box.
                        */
     {

     /* Need enough space for a time sz (we overwrite the terminating 0
        with a space), a maximum length appointment description, and a
        terminating 0.
     */
     TCHAR rgchAlarm [CCHTIMESZ + CCHQDMAX + 1];
     INT  itdd;
     INT  cch;
     DD   *pdd;
     FT   ftTemp;
     FT   ftStart;
     FT   ftStop;
     INT  cAlarms;
     DL   dl;
     register WORD  idr;
     register PQR       pqr;
     WORD  idrFree;

     /* This could take some time if we hit the disk. */
     HourGlassOn();

     /* Make local copies of the FTs we were passed pointers to so we
        don't overwrite them (suppose we are passed a pointer to vftAlarmNext
        for example).
     */
     ftStart = *pftStart;
     ftStop = *pftStop;

     /* Say there is no next alarm. */
     vftAlarmNext.dt = DTNIL;

     /* Find a free DR in case we need to read in from disk. */
     idrFree = IdrFree ();

     for (FSearchTdd (ftStart.dt, &itdd); itdd < vcddUsed; itdd++)
          {
          pdd = TddLock () + itdd;
          ftTemp.dt = pdd -> dt;
          cAlarms = pdd -> cAlarms;
          dl = pdd -> dl;
          idr = pdd -> idr;
          TddUnlock ();

          if (cAlarms == 0)
               continue;

          if (idr == IDRNIL)
               {
               /* The next alarm is not in memory.  If we're not supposed
                  to hit the disk, we've done all we can do.
               */
               if (!fDisk)
                    goto Exit0;

               /* Note - since cAlarms was not zero, there must be some
                  data for this date somewhere.  It isn't in memory, so
                  it must be on disk.  Therefore, dl cannot be DLNIL, so
                  we don't need check to see if it is.
               */
               ReadTempDr (idr = idrFree, dl);
               }

          pqr = (PQR)PbTqrFromPdr(PdrLock(idr));

          for ( ; cAlarms; pqr = (PQR )((BYTE *)pqr + pqr -> cb))
               {
               if (pqr -> fAlarm)
                    {
                    cAlarms--;

                    /* Remember the time of the alarm. */
                    ftTemp.tm = pqr -> tm;

                    if (CompareFt (&ftTemp, &ftStart) != -1)
                         {
                         /* This ft is greater than or equal to ftStart. */
                         if (CompareFt (&ftTemp, &ftStop) != -1)
                              {
                              /* This ft is greater than or equal
                                 to ftStop, so this is the next alarm.
                              */
                              vftAlarmNext = ftTemp;
                              DrUnlock (idr);
                              goto Exit0;
                              }

                         /* This is a triggered alarm, so put it into the
                            list box of the alarm acknowledgement dialog box.
                         */
                         if (hwnd != (HWND)NULL)
                              {
                              cch = GetTimeSz(pqr -> tm, rgchAlarm);
                              rgchAlarm[cch] = TEXT(' ');
                              lstrcpy(&rgchAlarm[cch + 1], pqr -> qd);
                              if (SendDlgItemMessage (hwnd, IDCN_LISTBOX,
                               LB_ADDSTRING, (WORD)0, (LONG)(LPTSTR)rgchAlarm)
                               == LB_ERRSPACE)
                                   {
                                   /* ??? Not enough memory.  Be sure to unlock
                                      if bail out due to this error.
                                   */
                                   }
                              }
                         }
                    }
               }

          DrUnlock (idr);

          }

     /* End of tdd reached - there is no next alarm. */
Exit0:
     HourGlassOff ();

     }


/**** IdrFree - find a free DR. */

WORD  APIENTRY IdrFree ()

     {

     register WORD   idr;
     register DT    dt;

     /* Find a free DR to read the date into.  There is guaranteed
        to be at least one free one since there are 3 and we keep at
        most 2 dates in memory at one time, so finding a free one
        will terminate this loop with idr containing the index of the
        free DR.
     */
     idr = CDR;
     do
          {
          idr--;
          dt = PdrLock(idr)->dt;
          DrUnlock (idr);
          }
     while (dt != DTNIL);

     return (idr);

     }


/**** ReadTempDr - read date into DR for temporary use. */

VOID APIENTRY ReadTempDr (
    WORD  idr,
    DL   dl)
     {

     register DR *pdr;

     pdr = PdrLock (idr);

     if (!FReadDrFromFile (TRUE, pdr, dl))
          {
          /* ??? Error trying to read date - what now? */
          }

     /* Make the DR still look free since we are only using it
        temporarily.
     */
     pdr -> dt = DTNIL;
     DrUnlock (idr);

     }


/**** StartStopFlash */

VOID APIENTRY StartStopFlash (BOOL fStart)
    /* FALSE means stop flashing, TRUE means start flashing. */
     {
     if (fStart != vfFlashing)
          FlashWindow (vhwnd0, vfFlashing = fStart);
     }
