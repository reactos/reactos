/*
 *   Windows Calendar
 *   Copyright (c) 1985 by Microsoft Corporation, all rights reserved.
 *   Written by Mark L. Chamberlin, consultant to Microsoft.
 *
*/

/*
 *****
 ***** calrem.c
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


/**** FnRemove */

BOOL APIENTRY FnRemove (
    HWND hwnd,
    WORD message,
    WPARAM wParam,
    LONG lParam)
     {
     DOSDATE dd;
     CHAR   sz[CCHDASHDATE];

     switch (message)
          {
          case WM_INITDIALOG:
               /* Remember the window handle of the dialog for AlertBox. */
               vhwndDialog = hwnd;

#ifdef OLDWAY
               /*
                * NT Bug 9019 says this should start from today's date,
                * but I personally think the function is more useful this way,
                * so I will leave the code here and just ifdef it out incase
                * I can convince others to let me put it back in.
                * 30-Jan-1993 JonPa
                */

               /* Get date string for 1-1-1980 */
               dd.month = 1;
               dd.day = 1;
               dd.year = 1980;
               GetDateString(&dd, sz, GDS_SHORT);
#else
               /* Get date string for currently selected day */
               GetDashDateSel (sz);
#endif
               SetDlgItemText (hwnd, IDCN_FROMDATE, (LPSTR)sz);
               return (TRUE);

	  case WM_COMMAND:
		switch (GET_WM_COMMAND_ID(wParam, lParam)) {
                    case IDOK:
			 GetRangeOfDates (hwnd);
			 /* line added to fix keyboard hanging problem when
			    Calendar is run under ver 3.0 rel 1.11 */
			 CalSetFocus (GetDlgItem (hwnd, IDCN_FROMDATE));
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


/**** GetRangeOfDates */

VOID APIENTRY GetRangeOfDates (HWND hwnd)
     {

     CHAR szDateFrom [CCHDASHDATE];
     CHAR szDateTo [CCHDASHDATE];
     D3   d3From;

     GetDlgItemText (hwnd, IDCN_FROMDATE, (LPSTR)szDateFrom, CCHDASHDATE);
     if (GetDlgItemText (hwnd, IDCN_TODATE, (LPSTR)szDateTo, CCHDASHDATE) == 0)
          lstrcpy (szDateTo, szDateFrom);

     if (FD3FromDateSz (szDateFrom, &d3From) == 0
      && FD3FromDateSz (szDateTo, &vd3To) == 0
      && (vdtFrom = DtFromPd3 (&d3From)) <= (vdtTo = DtFromPd3 (&vd3To)))
          {
          /* Get the index of the first date in the range.  Note that if the
             date doesn't exist in the tdd, the index of the first date
             higher than it is returned.
          */
          FSearchTdd (vdtFrom, &vitddFirst);

          /* Get the index of the last date in the range + 1.  If an exact
             match is found, increment to get the one beyond the range.
           */
          if (FSearchTdd (vdtTo, &vitddMax))
               vitddMax++;

          EndDialog (hwnd, TRUE);
          }
     else
          {
          /* Error in date - put up message box. */
          AlertBox (vszBadDateRange,
           (CHAR *)NULL, MB_APPLMODAL | MB_OK | MB_ICONASTERISK);
          }
     }


/**** Remove  - remove the dates within the range vd3From through vd3To.
      Call with vdtFrom <= vdtTo since Remove depends on this.
      Also, vitddFirst <= vitddLast.
*/

VOID APIENTRY Remove ()

     {

     register WORD idr;
     register DR *pdr;

     /* Show the hour glass cursor. */
     HourGlassOn ();

     /* Record edits and disable focus so edits don't get recorded later
        into a DR that has been removed.
     */
     CalSetFocus ((HWND)NULL);

     /* Free up any DRs within the range of dates to be removed. */
     for (idr = 0; idr < CDR; idr++)
          {
          /* Get a pointer to the DR. */
          pdr = PdrLock (idr);

          if (pdr -> dt >= vdtFrom && pdr -> dt <= vdtTo)
               pdr -> dt = DTNIL;

          /* Unlock the DR. */
          DrUnlock (idr);
          }

     /* Get rid of the dates. */
     ShrinkTdd (vitddFirst, vitddMax - vitddFirst);

     /* It's possible that the armed alarm has just been wiped out.  If so,
        we need to arm a higher one (if there is one).  By calling GetNextAlarm
        with vftAlarmNext, the current alarm will be kept if it hasn't been
        removed.  If it has been removed, the next highest alarm will get
        armed.  Note - there is no need to call AlarmCheck here - the
        new alarm is greater than or equal to the old one, and the old
        one had not gone off yet.
     */
     GetNextAlarm (&vftAlarmNext, &vftAlarmNext, TRUE, (HWND)NULL);

     /* The date being displayed may have been removed, so update the
        display accordingly.  In day mode, only removing the day being
        displayed matters, but in month mode, removing any marked days
        in the current month requires a redisplay.  It doesn't seem worth
        the trouble to check for these cases and only update the display
        for them, so the display is updated in all cases.
     */
     if (vfDayMode)
          {
          /* This can't fail since the selected date is either still in
             memory (it wasn't removed) or it was removed and won't require
             disk I/O to create (since it has no longer has any data).
             Note that SwitchToDate will set the focus (which we turned
             off above).
          */
          SwitchToDate (&vd3Sel);
          }
     else
          {
          /* Reset the focus.
             Call UpdateMonth since marked days in the current month may
             have been removed.  UpdateMonth will also call SetNotesEc,
             which is necessary since the notes for the selected date may
             have been removed.  Note that vwDaySticky has not changed,
             so the correct day will get selected by UpdateMonth.
          */
          CalSetFocus (vhwndFocus);
          vd3To = vd3Sel;
          UpdateMonth ();
          }

     /* Remove makes the file dirty. */
     vfDirty = TRUE;

     /* The waiting is over. */
     HourGlassOff ();

     }


/* Calls to HourGlassOn and HourGlassOff are always balanced, but they
   may be nested.  For example, LoadCal calls HourGlassOn, then it
   calls CleanSlate.  Cleanslate calls HourGlassOn, does its stuff,
   calls HourGlassOff, and returns to LoadCal.  LoadCal does the
   rest of its stuff and calls HourGlassOff.  To handle this nesting
   cleanly and without flicker, a count of calls to HourGlassOn
   is kept.  When HourGlassOn increments this count from 0 to 1,
   the handle of the current cursor is saved away.  When HourGlassOff
   decrements this count from 1 to 0, the cursor is restored to the
   saved one.
*/

INT  vcHourGlassOn = 0;
HCURSOR vhcsrPrev;

/**** HourGlassOn - put up the hour glass cursor and remember what
      the cursor was before.
*/

VOID APIENTRY HourGlassOn ()

     {

     register HCURSOR hcsrTemp;

     hcsrTemp = SetCursor (vhcsrWait);
     if (vcHourGlassOn++ == 0)
          vhcsrPrev = hcsrTemp;

     }


/**** HourGlassOff - restore the cursor to what it was before we put
      up the hour glass.
*/

VOID APIENTRY HourGlassOff ()

     {

     if (--vcHourGlassOn == 0)
          SetCursor (vhcsrPrev);

     }
