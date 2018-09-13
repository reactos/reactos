/*
 *   Windows Calendar
 *   Copyright (c) 1985 by Microsoft Corporation, all rights reserved.
 *   Written by Mark L. Chamberlin, consultant to Microsoft.
 *
*/

/*
 *****
 ***** calspecl.c
 *****
*/

/* Get rid of more stuff from windows.h */
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOSYSCOMMANDS
#define NOMEMMGR
#define NOCLIPBOARD
#define NOVIRTUALKEYCODES

#include "cal.h"

/* Notes about special times - 9/13/85 - MLC:

   I want to explain some things about special times because the design
   is not as clean as it should be and can lead to some confusion.
   As I originally planned it, a special time was defined to be an
   appointment time that had been inserted with the Options Special Time
   Insert command.  An appointment time inserted this way had the fSpecial
   bit set in its associated QR.  This is still the case, but from the
   user's point of view the definition of a special time is different.
   The problem was that user confusion could result from the original
   definition.  For example, suppose that with the interval set to 15
   minutes, the user types text into a 9:15 appointment.  At some
   point he switches the interval to 60 minutes.  The 9:15 appointment
   is still present because it has data associated with it, and given
   the 60 minute interval, it sure looks like a special time to the
   user (particularly if he is looking at it days later and doesn't
   remember that it was inserted when he had the interval set to 15
   minutes).  He tries to deleted it with the Options Special Time
   Delete command, but since its fSpecial bit is not set, he is
   told that it is not a special time, which totally confuses him,
   and he sues Tandy Trower.  The only way to get rid of the 9:15
   appointment is to make it "empty" (no text, no alarm).
   So to avoid user confusion I ended up saying that for purposes
   of the Options Special Time Delete command, a special time is
   any time that does not fall on a regular interval given the
   current interval setting.
   The fSpecial bit now serves only one semi-rediculous purpose:
   With the interval set to 60, the user inserts the special time
   of 10:30.  He types no text into that time, and there is no
   alarm set.  He switches to 30 minute interval, and then back
   to 60 minute interval.  Were it not for the fSpecial bit, the
   appointment would have disappeared because in 30 minute mode
   it would not look like a special time.  But the fSpecial bit
   keeps the appointment from going away.
   I now consider the whole business of the fSpecial
   bit to be unnecessary baggage, and I am tempted to get rid of it.
   I am short on time, and it's not really worth making such a change
   at this late stage, so I won't, but if I had it to do
   over, I would not have the fSpecial bit.
   My apologies in advance to anyone who has to work on special times -
   I hope this explanation helps.
*/


/**** InsertSpecial - insert a special time. */

VOID APIENTRY InsertSpecial ()

     {

     QR   qrNew;

     /* Record the current edits and prevent them from later using an
        invalid tld (which we are about to invalidate by fooling around
        with the tqr).
     */
     CalSetFocus ((HWND)NULL);

     /* Insert a new QR for the special time with the special time bit set.
        The special time bit prevents keeps the QR in the tqr even though
        it has no appointment text and the alarm is not set.
        Note that FSearchTqr was called by the Special Time dialog so
        votqrNext is already set up.
     */
     qrNew.cb = CBQRHEAD + 1;
     qrNew.fAlarm = FALSE;
     qrNew.fSpecial = TRUE;
     qrNew.tm = vtmSpecial;
     qrNew.qd [0] = '\0';

     if (FInsertQr (votqrNext, &qrNew))
          {
          /* Adjust up the scroll bar range. */
          AdjustDayScrollRange (1);

          /* Fix up the display. */
          SpecialTimeFin ();
          }
     }


/**** DeleteSpecial - delete a special time. */

VOID APIENTRY DeleteSpecial ()

     {

     register BOOL fAlarm;
     register DR   *pdr;
     INT  itdd;
     FT   ftTemp;

     /* Note that FSearchTqr was
        called by the special time dialog code, and the result was
        TRUE.  Therefore, votqrCur has been set up.
     */

     /* Record the current edits and prevent them from later using an
        invalid tld (which we are about to invalidate by fooling around
        with the tqr).
     */
     CalSetFocus ((HWND)NULL);

     /* Before deleting the QR, see if it had an alarm. */
     fAlarm = ((PQR )(PbTqrFromPdr (pdr = PdrLockCur ()) + votqrCur))
      -> fAlarm;
     ftTemp.dt = pdr -> dt;
     DrUnlockCur ();

     /* Delete the QR for the special time. */
     DeleteQr (votqrCur);

     if (fAlarm)
          {
          /* There is an alarm for the special time we're deleting.
             Decrement the count of alarms for this date.
          */
          FSearchTdd (ftTemp.dt, &itdd);
          (TddLock () + itdd) -> cAlarms--;
          TddUnlock ();

          ftTemp.tm = vtmSpecial;
          if (CompareFt (&ftTemp, &vftAlarmNext) == 0)
               {
               /* Cancelling the next armed alarm.  Need to arm the one
                  after it.  Since the one we are cancelling has not yet
                  gone off, it can't be time for the one after it to
                  go off either, so there is no need to call AlarmCheck -
                  just let it go off naturally.
                  Note this be done AFTER the QR has been deleted and
                  the count has been decremented so
                  GetNextAlarm doesn't find the same alarm again.
               */
               GetNextAlarm (&vftCur, &vftCur, TRUE, (HWND)NULL);
               }
          }

     /* Adjust down the range of the scroll bar. */
     AdjustDayScrollRange (-1);

     /* Fix up the display. */
     SpecialTimeFin ();

     }


/**** SpecialTimeFin */

VOID APIENTRY SpecialTimeFin ()

     {

     register INT ln;

     /* Mark the DR and the file as dirty. */
     PdrLockCur () -> fDirty = vfDirty = TRUE;
     DrUnlockCur ();

     /* Fill the tld, using the special time as the starting time.
        In the insert case, FillTld will use the new special time.
        In the delete case, FillTld will use the first appointment
        time greater than the special time that was just deleted.
     */
     FillTld (vtmSpecial);

     /* Find the ln of the new special time (insert) or of the appointment
        time following the old special time (delete).  It's not
        necessarily on the top line since there may not have been
        enough appointment times following it to fill up the screen.
     */
     for (ln = 0; ln < vlnLast && vtld [ln].tm < vtmSpecial; ln++)
          ;

     /* Give it the focus.  (Need to set vhwndFocus before calling
        SetQdEc because we want the focus to go to the appointment
        even if it's currently in the notes area.
     */
     vhwndFocus = vhwnd3;
     SetQdEc (ln);

     /* Set the thumb, and repaint the appointments. */
     SetDayScrollPos (-1);
     InvalidateRect (vhwnd2B, (LPRECT)NULL, TRUE);

     }
