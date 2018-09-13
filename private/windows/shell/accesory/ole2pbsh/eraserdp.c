/****************************Module*Header******************************\
* Copyright (c) 1987 - 1991  Microsoft Corporation                      *
\***********************************************************************/
/********************************************************
*                                                       *
*   file:   EraserDP.c                                  *
*   system: PC Paintbrush for MS-Windows                *
*   descr:  eraser draw proc                            *
*   date:   04/01/87 @ 10:10                            *
********************************************************/

#include <windows.h>
#include <port1632.h>

#include "oleglue.h"
#include "pbrush.h"

extern BOOL bExchanged;
extern RECT rDirty;

static int cntr, dir, wid, hgt;

LONG APIENTRY DrawEraser(HDC dstDC, LPRECT lprBounds, WPARAM wParam)
{
   int i,w2,h2;
   int width;
   int height;
   RECT rcRect;

   if(01&cntr++)
      return(FALSE);

   XorCsr(dstDC, csrPt, BOXcsr);
   csrPt.x = lprBounds->right;
   csrPt.y = lprBounds->bottom;

   ConstrainBrush(lprBounds, wParam, &dir);

//   MSetWindowOrg(dstDC, wid>>1, hgt>>1);

   w2 = wid >> 1;
   h2 = hgt >> 1;

   PolyTo(*((LPPOINT)lprBounds), *((LPPOINT)lprBounds+1), wid, hgt);

   for (i=0;i<6;i++) {
        polyPts[i].x += imageView.left - w2;
        polyPts[i].y += imageView.top - h2;
   }

   /* get bounding rect of polygon (ignoring pen width) */
   PolyRect(polyPts, 6, &rcRect);

   /* Inflate bounding box to surround the thick pen */
   CompensateForPen(hdcWork, &rcRect);

   width = rcRect.right - rcRect.left;
   height = rcRect.bottom - rcRect.top;

   Polygon(hdcWork, polyPts, 6);

   BitBlt(dstDC, rcRect.left - imageView.left,
		 rcRect.top - imageView.top,
		 width, height,
         hdcWork, rcRect.left, rcRect.top, SRCCOPY);



//   MSetWindowOrg(dstDC, 0, 0);

   lprBounds->left = lprBounds->right;
   lprBounds->top = lprBounds->bottom;

   XorCsr(dstDC, csrPt, BOXcsr);

   return(TRUE);
}

void EraserDP(HWND hWnd, UINT code, WPARAM wParam, LONG lParam)
{
   HBRUSH brush, hOldBrush;
   LPPAINTSTRUCT lpps;
   RECT rcReturn;
   HDC paintDC;
   POINT newPt;

   LONG2POINT(lParam,newPt);

   switch (code) {
   case WM_PAINT:
      lpps = (LPPAINTSTRUCT) lParam;
      XorCsr(lpps->hdc, csrPt, BOXcsr);
      break;

   case WM_HIDECURSOR:
   case WM_TERMINATE:
      HideCsr(NULL, hWnd, BOXcsr);
      csrPt.x = csrPt.y = -1;
      SetCursorOn();
      break;

   case WM_SHOWCURSOR:
      csrPt.x = csrPt.y = -1;
      SetCursorOff();
      break;

   /* setup drawing environment */
   case WM_LBUTTONDOWN:
      cntr = 0;
      dir = 0;
      rcReturn.left = rcReturn.right = newPt.x;
      rcReturn.top = rcReturn.bottom = newPt.y;
      GetAspct(-((theSize + 1) << 2), &wid, &hgt);

      if(!(paintDC = GetDisplayDC(hWnd)))
    	 goto Error1;

      if(!(brush = CreateSolidBrush(rgbColor[theBackg])))
         goto Error2;
      hOldBrush = SelectObject(hdcWork, brush);
      SelectObject(hdcWork, GetStockObject(NULL_PEN));

      if(bExchanged)
      {
    	PasteDownRect(rDirty.left, rDirty.top,
	       rDirty.right-rDirty.left, rDirty.bottom-rDirty.top);
      }

      code = TrackTool(hWnd, DrawEraser, &rcReturn, &wParam, paintDC);

      XorCsr(paintDC, csrPt, BOXcsr);
      theBounds.left   += imageView.left - (wid>>1);
      theBounds.top    += imageView.top - (hgt>>1);
      theBounds.right  += imageView.left + (wid>>1);
      theBounds.bottom += imageView.top + (hgt>>1);

      UnionWithRect(&rDirty, &theBounds);

      AdviseDataChange();

      XorCsr(paintDC, csrPt, BOXcsr);

      if (hOldBrush)
    	 SelectObject(hdcWork, hOldBrush);
      DeleteObject(brush);
Error2:
      ReleaseDC(hWnd, paintDC);
Error1:
      break;

   case WM_MOUSEMOVE:
      if(!(paintDC = GetDisplayDC(hWnd)))
	     return;
      XorCsr(paintDC, csrPt, BOXcsr);
      XorCsr(paintDC, csrPt = newPt, BOXcsr);
      ReleaseDC(hWnd, paintDC);
      break;

   default:
      break;
   }
}
