/****************************Module*Header******************************\
* Copyright (c) 1987 - 1991  Microsoft Corporation                      *
\***********************************************************************/
/********************************************************
*                                                       *
*   file:   CalcWnds.c                                  *
*   system: PC Paintbrush for MS-Windows                *
*   descr:   calculates dimensions of child windows     *
*   date:   04/02/87 @ 11:30                            *
********************************************************/

#include "onlypbr.h"
#undef NOMINMAX
#undef NOWINOFFSETS
#undef NOWINSTYLES
#undef NOSYSMETRICS

#include <windows.h>
#include "port1632.h"
#include "pbrush.h"

#define NOT ~

#define recalc(source,multip) ((int)((source*(long)multip) >> 10))

/* minimum screen dimensions (used by GetRect()) */
#define MINGAPWID   2
#define MINGAPHGT   2
#define MINPAINTWID 16
#define MINPAINTHGT 16
#define MINTOOLWID  8
#define MINTOOLHGT  40
#define MINSIZEWID  8
#define MINSIZEHGT  22
#define MINCOLORWID 72
#define MINCOLORHGT 10

/* screen proportions in 1024th's (used by CalcWnds()) */
#define GAPWID64   3
#define GAPHGT64   2
#define PAINTWID64 900
#define PAINTHGT64 900
#define TOOLWID64  100
#define TOOLHGT64  770
#define SIZEWID64  100
#define SIZEHGT64  230
#define COLORWID64 900
#define COLORHGT64 100

extern RECT pbrushRct[];
extern HWND pbrushWnd[];
extern HBITMAP hToolbox;
extern int imageWid, imageHgt;
extern BOOL inMagnify;

void CalcWnds(int disptools, int displine, int dispcolor, int disppaint)
{
   int i;
   REGISTER int clientwid, clienthgt;
   int gapwid, paintwid, toolwid, sizewid, colorwid;
   int gaphgt, painthgt, toolhgt, sizehgt, colorhgt;
   BITMAP hbits;
   RECT r;
   long style;

/* get client area dimensions */
   clientwid = pbrushRct[PARENTid].right;
   clienthgt = pbrushRct[PARENTid].bottom;

   gapwid = max(MINGAPWID, recalc(clientwid, GAPWID64));
   gaphgt = max(MINGAPHGT, recalc(clienthgt, GAPHGT64));

   if((disptools == SHOWWINDOW) ||
         (disptools == NOCHANGEWINDOW && IsWindowVisible(pbrushWnd[TOOLid]))) {
      paintwid = max(MINPAINTWID, recalc(clientwid, PAINTWID64));
   } else {
      paintwid = max(MINPAINTWID, recalc(clientwid, 1024 - (GAPWID64 << 1)));
   }

   if((dispcolor == SHOWWINDOW) ||
         (dispcolor == NOCHANGEWINDOW && IsWindowVisible(pbrushWnd[COLORid]))) {
      painthgt = max(MINPAINTHGT, recalc(clienthgt, PAINTHGT64));
   } else {
      painthgt = max(MINPAINTHGT, recalc(clienthgt, 1024 - (GAPHGT64 << 2)));
   }

   toolwid = max(MINTOOLWID, recalc(clientwid, TOOLWID64));
   toolhgt = max(MINTOOLHGT, recalc(clienthgt, TOOLHGT64));

   if(hToolbox) {
      GetObject(hToolbox, sizeof(hbits), (LPVOID) &hbits);     
      i = (50*toolwid) / hbits.bmWidth;
      if((45 < i) && (i < 55))
         toolwid = hbits.bmWidth;
      i = (50*toolhgt) / hbits.bmHeight;
      if((45 < i) && (i < 55))
         toolhgt = hbits.bmHeight;
   }

   sizewid = max(MINSIZEWID, recalc(clientwid, SIZEWID64));
   sizewid = toolwid;   /* always */
   sizehgt = max(MINSIZEHGT, recalc(clienthgt, SIZEHGT64));
   sizehgt = clienthgt - (toolhgt + 3 * gaphgt);
   sizehgt = max(3, sizehgt);

   colorwid = max(MINPAINTWID,recalc(clientwid, COLORWID64));
   colorhgt = max(MINCOLORHGT,recalc(clienthgt, COLORHGT64));

   /* set up a RECT for the paint window */
   r.left = r.top = 0;
   if (inMagnify)
   {
      r.right = paintwid
            + 2*GetSystemMetrics(SM_CXBORDER);
      if (r.right > (imageWid * zoomAmount))
        r.right = (imageWid * zoomAmount) + 2*GetSystemMetrics(SM_CXBORDER);
      r.bottom = painthgt
            + 2*GetSystemMetrics(SM_CYBORDER);
      if (r.bottom > (imageHgt * zoomAmount))
        r.bottom = (imageHgt * zoomAmount) + 2*GetSystemMetrics(SM_CYBORDER);
   }
   else
   {
      r.right = min(imageWid, paintwid)
            + 2*GetSystemMetrics(SM_CXBORDER);
      r.bottom = min(imageHgt, painthgt)
            + 2*GetSystemMetrics(SM_CYBORDER);
   }

   /* set up scroll bars for wm_size message */
   if(pbrushWnd[PAINTid])
      style = GetWindowLong(pbrushWnd[PAINTid], GWL_STYLE);

   if((inMagnify && r.right < (imageWid * zoomAmount)) ||
       (!inMagnify && r.right > paintwid)) {
      style |= WS_HSCROLL;
      r.bottom += GetSystemMetrics(SM_CYHSCROLL) -
            GetSystemMetrics(SM_CYBORDER);
   } else
      style &= NOT WS_HSCROLL;

   if((inMagnify && r.bottom < (imageHgt * zoomAmount)) ||
        (!inMagnify && r.bottom > painthgt)) {
      style |= WS_VSCROLL;
      r.right += GetSystemMetrics(SM_CXVSCROLL) -
            GetSystemMetrics(SM_CXBORDER);
   } else
      style &= NOT WS_VSCROLL;

   if(!(style & WS_HSCROLL) && (r.right > paintwid)) {
      style |= WS_HSCROLL;
      r.bottom += GetSystemMetrics(SM_CYHSCROLL) -
            GetSystemMetrics(SM_CYBORDER);
   }

   if(pbrushWnd[PAINTid])
      SetWindowLong(pbrushWnd[PAINTid], GWL_STYLE, style);

   /* bound by the size of our painting area */
   if(r.right > paintwid)
      r.right = paintwid;
   if(r.bottom > painthgt)
      r.bottom = painthgt;

   if((disptools == SHOWWINDOW) ||
         (disptools == NOCHANGEWINDOW && IsWindowVisible(pbrushWnd[TOOLid]))) {
      pbrushRct[PAINTid].left = toolwid + (gapwid << 1);
   } else {
      pbrushRct[PAINTid].left = gapwid;
   }
   pbrushRct[PAINTid].right = pbrushRct[PAINTid].left + r.right - r.left;
   pbrushRct[PAINTid].top = gaphgt;
   pbrushRct[PAINTid].bottom = pbrushRct[PAINTid].top + r.bottom - r.top;

   pbrushRct[TOOLid].left = gapwid;
   pbrushRct[TOOLid].right = pbrushRct[TOOLid].left + toolwid - 1;
   pbrushRct[TOOLid].top = gaphgt;
   pbrushRct[TOOLid].bottom = pbrushRct[TOOLid].top + toolhgt - 1;

   pbrushRct[SIZEid].left = gapwid;
   pbrushRct[SIZEid].right = pbrushRct[SIZEid].left + sizewid - 1;
   pbrushRct[SIZEid].top = toolhgt + (gaphgt << 1);
   pbrushRct[SIZEid].bottom = pbrushRct[SIZEid].top + sizehgt - 1;

   pbrushRct[COLORid].left = toolwid + (gapwid << 1);
   pbrushRct[COLORid].right = pbrushRct[COLORid].left + colorwid - 1;
   i = pbrushRct[PAINTid].bottom + gaphgt;
   i = max(i, painthgt + (gaphgt << 1));
   pbrushRct[COLORid].top = i;
   pbrushRct[COLORid].bottom = pbrushRct[COLORid].top + colorhgt - 1;
}
