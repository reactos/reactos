/****************************Module*Header******************************\
* Copyright (c) 1987 - 1991  Microsoft Corporation                      *
\***********************************************************************/
/********************************************************
*                                                       *
*   file:   AirBruDP.c                                  *
*   system: PC Paintbrush for MS-Windows                *
*   descr:   air brush draw proc                        *
*   date:   04/01/87 @ 10:20                            *
********************************************************/

#include "onlypbr.h"
#undef NOKEYSTATES
#undef NORASTEROPS
#undef NOWINMESSAGES

#include <windows.h>
#include "port1632.h"
#include "pbrush.h"
#include "pbserver.h"

extern int theSize, theForeg;
extern RECT theBounds;
extern DWORD *rgbColor;

int cntr, dir, wid, hgt, halfWid, halfHgt;

static HDC mono1DC;

extern BOOL bExchanged;
extern RECT rDirty;

LONG APIENTRY DrawAirBru(HDC dstDC, LPRECT lprBounds, WPARAM wParam)
{
   RECT rcTemp;

   if(01&cntr++)
      return(FALSE);

   ConstrainBrush(lprBounds, wParam, &dir);

   rcTemp.left = lprBounds->right-halfWid + imageView.left;
   rcTemp.top = lprBounds->bottom-halfHgt + imageView.top;
   rcTemp.right = rcTemp.left + wid;
   rcTemp.bottom = rcTemp.top + hgt;

   BitBlt(hdcWork, rcTemp.left,
		   rcTemp.top,
		   wid, hgt,
	  mono1DC, 0, 0, ROP_DSPDxax);

   BitBlt(dstDC, rcTemp.left - imageView.left,
		 rcTemp.top - imageView.top,
		 wid, hgt,
	  hdcWork,rcTemp.left, rcTemp.top, SRCCOPY);

   lprBounds->left = lprBounds->right;
   lprBounds->top = lprBounds->bottom;

   return(TRUE);
}

void AirBruDP(HWND hWnd, UINT code, WPARAM wParam, LONG lParam)
{
   HBITMAP monoBM, hOldBM;
   HDC paintDC;
   HBRUSH brush, hOldBrush;
   HBITMAP brushBM;
   POINT newPt;

   RECT rcReturn;

   if(code == WM_LBUTTONDOWN ||
      code == WM_LBUTTONDBLCLK) {

      if(bExchanged)
	PasteDownRect(rDirty.left, rDirty.top,
	       rDirty.right-rDirty.left, rDirty.bottom-rDirty.top);

      LONG2POINT(lParam,newPt);
      cntr = dir = 0;
      rcReturn.left = rcReturn.right = newPt.x;
      rcReturn.top = rcReturn.bottom = newPt.y;

      GetAspct((theSize + 1) * 3, &wid, &hgt);
      halfWid = wid >> 1;
      halfHgt = hgt >> 1;

      if(!(paintDC = GetDisplayDC(hWnd)))
         goto Error1;
      if(!(mono1DC = CreateCompatibleDC(paintDC)))
         goto Error2;
      if(!(monoBM = CreateBitmap(wid, hgt, 1, 1, 0L)))
         goto Error3;
      hOldBM = SelectObject(mono1DC, monoBM);

      PatBlt(mono1DC, 0, 0, wid, hgt, BLACKNESS);
      SelectObject(mono1DC, GetStockObject(NULL_PEN));
      SelectObject(mono1DC, GetStockObject(DKGRAY_BRUSH));
      Ellipse(mono1DC, 0, 0, wid, hgt);

      if(!(brushBM = CreatePatternBM(paintDC, rgbColor[theForeg])))
         goto Error4;
      if(!(brush = CreatePatternBrush(brushBM)))
         goto Error5;

      hOldBrush = SelectObject(hdcWork, brush);

      code = TrackTool(hWnd, DrawAirBru, &rcReturn, &wParam, paintDC);

      theBounds.left   += imageView.left - halfWid;
      theBounds.top    += imageView.top - halfHgt;
      theBounds.right  += imageView.left + halfWid;
      theBounds.bottom += imageView.top + halfHgt;


      UnionWithRect(&rDirty, &theBounds);

      if (fOLE)
	   SendDocChangeMsg(vpdoc, OLE_CHANGED);

      if (hOldBrush)
	 SelectObject(hdcWork, hOldBrush);

      DeleteObject(brush);
Error5:
      DeleteObject(brushBM);
Error4:
      if (hOldBM)
	 SelectObject(mono1DC, hOldBM);
      DeleteObject(monoBM);
Error3:
      DeleteDC(mono1DC);
Error2:
      ReleaseDC(hWnd, paintDC);
Error1:
      ;
   }
   return;
}
