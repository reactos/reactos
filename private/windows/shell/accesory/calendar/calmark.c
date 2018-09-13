/*
 *   Windows Calendar
 *   Copyright (c) 1985 by Microsoft Corporation, all rights reserved.
 *   Written by Mark L. Chamberlin, consultant to Microsoft.
 *
*/

/*
 *****
 ***** calmark.c
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


/**** CmdMark - mark or unmark the selected day. */

VOID APIENTRY CmdMark ()

     {

     register DT   dt;
     INT  itdd;
     RECT rect;
     register DD *pdd;

     /* Note that the Mark command only operates on the selected date,
        and this must be in the tdd, so there is no need to check the
        return result of FSearchTdd.
     */
     dt = DtFromPd3 (&vd3Sel);
     FSearchTdd (dt, &itdd);

     /* Mark or unmark the DD (toggle its state). */
     pdd = TddLock () + itdd;

     /* Update the month array by toggling the marked bit for the current
        day.
     */
     /* clear old marked bits on day */
     vrgbMonth [vwWeekdayFirst + vd3Sel.wDay] &= CLEARMARKEDBITS;

     /* set new marked bits */
     vrgbMonth [vwWeekdayFirst + vd3Sel.wDay] |= viMarkSymbol;
     pdd -> fMarked = viMarkSymbol;

     TddUnlock();
     /* If in month mode, cause the marking box to be drawn or erased. */
     if (!vfDayMode)
          {
          MapDayToRect (vd3Sel.wDay, &rect);
          InvalidateRect (vhwnd2B, (LPRECT)&rect, TRUE);
          }


     /* Marking a date makes the file dirty. */
     vfDirty = TRUE;
     }
