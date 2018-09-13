/*
 *   Windows Calendar
 *   Copyright (c) 1985 by Microsoft Corporation, all rights reserved.
 *   Written by Mark L. Chamberlin, consultant to Microsoft.
 *
 ***** calmain.c - small segment containing main loop and caltimer stuff
 *
 */

/* Get rid of more stuff from windows.h */
#define NOWINSTYLES
#define NOSYSMETRICS
#define NODRAWTEXT

#include "cal.h"
#include "uniconv.h"


/**** WinMain ****/

MMain(hInstance, hPrevInstance, lpAnsiCmdLine, cmdShow)
/* { */
    MSG  msg;
    LPTSTR lpCmdLine = GetCommandLine ();

    if (!CalInit (hInstance, hPrevInstance, SkipProgramName (lpCmdLine), cmdShow)) {
        return (FALSE);
    }

    // OK to process WM_ACTIVATE from now onwards
    fInitComplete = TRUE;

    while (GetMessage ((LPMSG)&msg, NULL, 0, 0))
        {
        /* Filter the special keys BEFORE calling translate message.
         * This way, the WM_KEYDOWN messages will get trapped before
         * the WM_CHAR messages are created so we need not worry about
         * trapping the latter messages.
         */
        if (!FKeyFiltered (&msg))
            {
            if (TranslateAccelerator (vhwnd0, vhAccel, (LPMSG)&msg) == 0)
                {
                TranslateMessage ((LPMSG)&msg);
                DispatchMessage ((LPMSG)&msg);
                }
            }
        }

    return msg.wParam;
    }


/**** FKeyFiltered - return TRUE if the key has been filtered. ****/

BOOL APIENTRY FKeyFiltered (MSG *pmsg)
    {
    register WPARAM wParam;

    wParam = pmsg -> wParam;

    /* Handle TIMER message here so we don't pull in another segment. */
    if (pmsg -> message == WM_TIMER)
       {
       CalTimer(FALSE);
       return TRUE;
       }

    /* Look for key down messages going to the edit controls.
       Karl Stock says there is no need to filter out the
       key up messages, and we will not call TranslateMessage
       for the filtered keys so there will be no WM_CHAR messages
       to filter.
    */
    if (pmsg -> message == WM_KEYDOWN)
        {
        if (pmsg -> hwnd == vhwnd2C)
            {
            /* In the notes area.  Tab means leave the notes area. */
            if (wParam == VK_TAB)
                {
                /* Leave the notes area. */
                if (!vfDayMode)
                    {
                    /* Give the focus to the monthly calendar. */
                    CalSetFocus (vhwnd2B);
                    }
                else
                    {
                    /* In day mode - give focus to the appointment
                       description edit control.
                    */
                    CalSetFocus (vhwnd3);
                    }
                return (TRUE);
                }
            return (FALSE);
            }

        else if (vfDayMode)
            {
            switch (wParam)
                {
                case VK_RETURN:
                case VK_DOWN:
                    /* If on last appointment, scroll up one appoinment.
                     * If not on last appointment in window, change
                     * focus to next appointment in window.
                     */
                    if (vlnCur == vlnLast)
                         ScrollUpDay (1, FALSE);
                    else
                         SetQdEc (vlnCur + 1);

                    break;

                case VK_UP:
                    /* If on first appointment in window, scroll down
                     * one appointment.
                     * If not on first appointment in window, change
                     * focus to previous appointment in window.
                     */
                    if (vlnCur == 0)
                        ScrollDownDay (1, FALSE, FALSE);
                    else
                        SetQdEc (vlnCur-1);

                    break;

                case VK_NEXT:
                case VK_PRIOR:
                    if (GetKeyState (VK_CONTROL) < 0)
                        {
                        /* Control Pg Up and Control Pg Dn are
                         * the accelerators for Show Previous and
                         * Show Next.  We want TranslateAccelerator
                         * to see them, so return FALSE.
                         */
                        return (FALSE);
                        }

                    /* Translate into a scroll command (as if area
                     * below or above thumb had been clicked).
                     */
                    SendMessage(vhwnd2B, WM_VSCROLL,
                                wParam==VK_NEXT ? SB_PAGEDOWN : SB_PAGEUP, 0L);
                    break;

                case VK_TAB:
                    /* Switch to the notes area. */
                    CalSetFocus (vhwnd2C);
                    break;

                default:
                    return (FALSE);
                }
            return (TRUE);
            }
        }

    return (FALSE);
    }


/**** CalTimer ****/

VOID APIENTRY CalTimer (BOOL fAdjust)
    {
    HDC  hDC;
    D3   d3New;
    DT   dtNew;
    TM   tmNew;
    FT   ftPrev;

    if (vfFlashing)
         FlashWindow (vhwnd0, TRUE);

    if (vcAlarmBeeps != 0)
         {
         MessageBeep (ALARMBEEP);
         vcAlarmBeeps--;
         }

    /* Fetch the date and time. */
    ReadClock (&d3New, &tmNew);

    /* See if the time or date has changed.  Note that it's necessary
     * to check all parts in order to immediartely detect all changes.
     * (I used to just check the time, but that meant a date change was
     * not detected until the minute changed.)
     */

    if (tmNew != vftCur.tm || d3New.wMonth != vd3Cur.wMonth
         || d3New.wDay != vd3Cur.wDay || d3New.wYear != vd3Cur.wYear)
         {
         /* Remember the old date and time */
         ftPrev = vftCur;
         vftCur.tm = tmNew;

         /* Show new date/time only if not iconic */
         if (!IsIconic(vhwnd0))
           {
           hDC = CalGetDC (vhwnd2A);
           DispTime (hDC);

           if ((dtNew = DtFromPd3 (&d3New)) != vftCur.dt)
                {
                vftCur.dt = dtNew;
                vd3Cur = d3New;
                if (!vfDayMode)
                     {
                     /* Display the new date. */
                     DispDate (hDC, &vd3Cur);

                     /* If the old or new date is in the
                        month currently being displayed, redisplay to get rid of
                        the >< on the old date.
                        Also, if the new date is in the month being displayed,
                        it will get marked with the >< as a result.
                     */
                     if ((vd3Cur.wMonth == vd3Sel.wMonth && vd3Cur.wYear ==
                      vd3Sel.wYear) || (d3New.wMonth == vd3Sel.wMonth
                      && d3New.wYear == vd3Sel.wYear))
                           {
                           /* Note - neither vcDaysMonth nor vwDaySticky
                              has changed, so UpdateMonth will end up selecting
                              the same day that's currently selected (which
                              is what we want).
                           */
                           vd3To = vd3Sel;
                           UpdateMonth ();
                           }
                     }
                }

           ReleaseDC (vhwnd2A, hDC);
           }

         /* If the new date/time is less than the previous one, or the
            new one is a day (1440 minutes) or more greater than the
            previous one, we want to resynchronize the next alarm.
            Obviously, if the date/time is less than the previouse one,
            the system clock has been adjusted (except in the case
            where it wraps on December 31, 2099, which I am not worried
            about).  However, it is not obvious when the clock has been
            set forward.  For example, if the user is running Calendar
            and then switches to an old application that grabs the
            whole machine (.g., Lotus 123), Calendar will not get
            timer messages while the olf app is running.  It is completely
            reasonable to expect that the user may not return to Windows
            for a long time (on the order of hours), so we only assume
            the clock has been set forward if it changes by a day or
            more (1440 minutes).  We don't want to make this period
            too great either since if we don't think the clock has been
            set ahead, we will put all the alarms that have passed into
            the alarm acknowledgement listbox.  In fact, avoiding this
            was the main reason for detecting clock adjustments.  Without
            setting the date/time on a machine without a hardware clock,
            the date/time would start out back in January, 1980.  If he
            then noticed the date was wrong and set it, all the alarms
            since January 1980 would be put into the listbox, which is
            not only rediculous, but could take a long time to read
            the disk for a bunch of old dates.  With the one day adjustment
            period, this is no longer a problem, because we ignore alarms
            that go off due to a forward clock adjustment.
            Note - do not set vfMustSyncAlarm FALSE in any case since
            it may already be TRUE and hasn't been serviced yet
            (because ProcessAlarms is locked).
         */

         /* If there is no NextAlarm present, then we dont have to resync
          * any alaram at all;
          * Fix for Bug #6196 --SANKAR-- 11-14-89
          */
         if (fAdjust && CompareFt (&vftCur, &ftPrev) != 0
              && vftAlarmNext.dt != DTNIL)
             {
             /* The clock has been adjusted - set flag to tell
                ProcessAlarms we want to resync, and force the
                call to AlarmCheck (below) to trigger an alarm
                immediately by setting the alarm time to the current
                time.
              */

             vfMustSyncAlarm=TRUE;
             vftAlarmNext=vftCur;
             }

         /* See if it's time to trigger the alarm (also handle
            resynchronization).
         */
         AlarmCheck ();
         }
    }


/**** AlarmCheck ****/

VOID APIENTRY AlarmCheck ()
     {

     FT   ftTemp;

     /* If the current time plus the early ring period is greater than or
        equal to the next alarm time, trigger the alarm.
     */
     ftTemp = vftCur;
     AddMinsToFt (&ftTemp, vcMinEarlyRing);
     if (CompareFt (&ftTemp, &vftAlarmNext) > -1)
          {
          /* Sound the alarm if sound is enabled.  Give the first beep
             right now and the rest at one second intervals in the timer
             message routine.
          */
          if (vfSound)
               {
               MessageBeep (ALARMBEEP);
               vcAlarmBeeps = CALARMBEEPS - 1;
               }

          if (vftAlarmFirst.dt == DTNIL)
               {
               /* This is the first unacknowledged alarm - remember it. */
               vftAlarmFirst = vftAlarmNext;

               if (GetActiveWindow () == vhwnd0)
                    {
                    /* We are the active window, so process the alarm now. */
                    ProcessAlarms ();
                    return;
                    }

               /* Not the active window, so fall through. */
               }

          /* Let the user know there are unacknowledged alarms. */
          StartStopFlash (TRUE);

          /* The next alarm is the first one > the one that just went off.
             GetNextAlarm looks for >=, so add one minute.
             Do not go to the disk - only arm the next alarm if it's in
             memory.  Note that this is absolutely necessary in the case
             where we don't have the focus (the user is doing something
             else, so it would be rude to start spinning the the disk and
             possibly asking for the correct floppy to be inserted).  In
             the case where we are active but the alarm acknowledgement
             dialog is already up, it would actually be OK to go to the disk,
             but I have decided it would be too confusing for the user if
             a disk I/O error were to occur at this point.
          */
          ftTemp = vftAlarmNext;
          AddMinsToFt (&ftTemp, 1);
          GetNextAlarm (&vftAlarmNext, &ftTemp, FALSE, (HWND)NULL);
          }
     }


/**** AddMinsToFt ****/

VOID APIENTRY AddMinsToFt (
    FT   *pft,
    WORD cMin)          /* Not to exceed the minutes in one day (1440). */
    {
    /* Add cMin to the time.  Note that the highest legitimate
       TM and the largest cMin cannot overflow a WORD, which
       is what a TM is, so we needn't worry about overflow here.
    */
    if ((pft -> tm += cMin) > TMLAST)
         {
         /* The time wrapped into the next day.  Adjust down the time,
            and increment the day.  If the date goes beyond DTLAST (that
            of December 31, 2099, the value should still be OK for the
            caller since it will only be used for comparison purposes.
            Anyway, I will be dead when that case comes up, so if it doesn't
            work correctly, it won't be my problem.
         */
         pft -> tm -= TMLAST + 1;
         pft -> dt++;
         }
    }



/**** CompareFt - compare the two FTs returning:
      -1 iff ft1 < ft2
      0  iff ft1 = ft2
      +1 iff ft1 > ft2
****/

INT  APIENTRY CompareFt (
    FT   *pft1,
    FT   *pft2)
    {
    register FT *pft1Temp;
    register FT *pft2Temp;

    if ((pft1Temp = pft1) -> dt < (pft2Temp = pft2) -> dt)
         return (-1);

    if (pft1Temp -> dt > pft2Temp -> dt)
         return (1);

    /* DTs are equal, compare the TMs. */
    if (pft1Temp -> tm < pft2Temp -> tm)
         return (-1);

    if (pft1Temp -> tm > pft2Temp -> tm)
         return (1);

    return (0);
    }
