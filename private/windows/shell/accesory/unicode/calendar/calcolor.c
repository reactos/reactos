/*
 *   Windows Calendar
 *   Copyright (c) 1985 by Microsoft Corporation, all rights reserved.
 *   Written by Mark L. Chamberlin, consultant to Microsoft.
 *
*/

/*
 *****
 ***** calcolor.c
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


/**** CreateBrushes */

BOOL APIENTRY CreateBrushes ()

     {

     if (!(vhbrBackMain = CreateSolidBrush(GetSysColor (COLOR_APPWORKSPACE))))
            return FALSE;
     if (!(vhbrBackSub = CreateSolidBrush(GetSysColor (COLOR_WINDOW))))
        return FALSE;
     if (!(vhbrBorder = CreateSolidBrush(GetSysColor (COLOR_WINDOWFRAME))))
        return FALSE;

     return TRUE;
     }


/**** DestroyBrushes */

VOID APIENTRY DestroyBrushes ()

     {
     if (vhbrBackMain)
         DeleteObject (vhbrBackMain);
     if (vhbrBackSub)
         DeleteObject (vhbrBackSub);
     if (vhbrBorder)
         DeleteObject (vhbrBorder);

     }


/**** PaintBack - paint the window background. */

VOID APIENTRY PaintBack (
    HWND hwnd,
    HDC  hDC)
     {

     RECT rect;
     register HBRUSH hbr;

     GetClientRect (hwnd, (LPRECT)&rect);
     hbr = vhbrBackMain;
     if (hwnd != vhwnd0)
          hbr = vhbrBackSub;
     MUnrealizeObject(hbr);
     FillRect (hDC, (LPRECT)&rect, hbr);

     }


/**** CalGetDC */

HDC  APIENTRY FAR CalGetDC (HWND hwnd)
     {

     register HDC hDC;

     hDC = GetDC (hwnd);
     SetDefaultColors (hDC);
     return (hDC);

     }


/**** SetDefaultColors */

VOID APIENTRY SetDefaultColors (HDC  hDC)
     {

     SetTextColor (hDC, GetSysColor (COLOR_WINDOWTEXT));
     SetBkColor (hDC, GetSysColor (COLOR_WINDOW));
     SelectObject (hDC, vhbrBorder);

     }


/**** DrawAlarmBell */

VOID APIENTRY DrawAlarmBell (
    HDC  hDC,
    INT  yco)
     {

     /* Note - the bell will be drawn the same color as the text, which
        is what we want.
     */
     SelectObject (vhDCMemory, vhbmBell);
     BitBlt (hDC, vxcoBell , yco, vcxBell,
             (vcyBell > vcyLineToLine ? vcyLineToLine : vcyBell),
             vhDCMemory, 0, 0, SRCCOPY);

     }
