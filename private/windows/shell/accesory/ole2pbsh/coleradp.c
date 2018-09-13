/****************************Module*Header******************************\
* Copyright (c) 1987 - 1991  Microsoft Corporation                      *
\***********************************************************************/
/********************************************************
*   file:   ColEraDP.c                                  *
*   system: PC Paintbrush for MS-Windows                *
*   descr:   color eraser draw proc                     *
*   date:   04/01/87 @ 10:10                            *
********************************************************/

#include <windows.h>
#include <port1632.h>

#include "oleglue.h"
#include "pbrush.h"


#define oldPt (*((LPPOINT)lprBounds  ))
#define newPt (*((LPPOINT)lprBounds+1))

extern BOOL bExchanged;
extern RECT rDirty;

static HBRUSH fBrush;

static int cntr, dir, wid, hgt, halfWid, halfHgt;

LONG APIENTRY DrawColEra(HDC dstDC, LPRECT lprBounds, WPARAM wParam)
{
   int monoWid, monoHgt;
   HDC colorDC, monoDC;
   HBITMAP colorBM, monoBM, hOldCBM, hOldMBM;
   RECT r;

   if(01&cntr++)
      return(FALSE);

   XorCsr(dstDC, csrPt, BOXCROSScsr);
   csrPt=newPt;

   ConstrainBrush(lprBounds, wParam, &dir);

   if(lprBounds->right >= lprBounds->left) {
      r.left = lprBounds->left - halfWid;
      r.right = lprBounds->right + halfWid;
   } else {
      r.left = lprBounds->right - halfWid;
      r.right = lprBounds->left + halfWid;
   }
   if(lprBounds->bottom >= lprBounds->top) {
      r.top = lprBounds->top - halfHgt;
      r.bottom = lprBounds->bottom + halfHgt;
   } else {
      r.top = lprBounds->bottom - halfHgt;
      r.bottom = lprBounds->top + halfHgt;
   }

   monoWid = r.right - r.left;
   monoHgt = r.bottom - r.top;

   if(!(colorDC = CreateCompatibleDC(hdcWork)))
      goto Error1;

   if(!(colorBM = CreateBitmap(monoWid, monoHgt,
         (BYTE)imagePlanes, (BYTE)imagePixels, 0L)))
   {
      goto Error2;
   }
   hOldCBM = SelectObject(colorDC, colorBM);

   if (hPalette) {
       SelectPalette(colorDC, hPalette, 0);
       RealizePalette(colorDC);
   }

   MUnrealizeObject(fBrush);
   SelectObject(colorDC, fBrush);

   if(!(monoDC = CreateCompatibleDC(hdcWork)))
      goto Error3;
   if(!(monoBM = CreateBitmap(monoWid, monoHgt, 1, 1, 0L)))
      goto Error4;
   hOldMBM = SelectObject(monoDC, monoBM);

   if (hPalette) {
       SelectPalette(monoDC, hPalette, 0);
       RealizePalette(monoDC);
   }

   SelectObject(monoDC, GetStockObject(WHITE_BRUSH));
   SelectObject(monoDC, GetStockObject(NULL_PEN));

   PatBlt(monoDC, 0, 0, monoWid, monoHgt, BLACKNESS);
   MSetWindowOrg(monoDC, r.left + halfWid, r.top + halfHgt);
   PolyTo(oldPt, newPt, wid, hgt);

   Polygon(monoDC, polyPts, 6);

   MSetWindowOrg(monoDC, 0, 0);

   BitBlt(colorDC, 0, 0, monoWid, monoHgt,
	  hdcWork, r.left + imageView.left,
		   r.top + imageView.top, ROP_SPxn);
   BitBlt(monoDC, 0, 0, monoWid, monoHgt,
	  colorDC, 0, 0, ROP_DSa);
   BitBlt(hdcWork, r.left + imageView.left,
		   r.top + imageView.top,
		   monoWid, monoHgt,
    	  monoDC, 0, 0, ROP_DSPDxax);
   BitBlt(dstDC, r.left, r.top, monoWid, monoHgt,
	  hdcWork, r.left + imageView.left, r.top + imageView.top, SRCCOPY);

   if (hOldMBM)
       SelectObject(monoDC, hOldMBM);
   DeleteObject(monoBM);
Error4:
   DeleteDC(monoDC);
Error3:
   if (hOldCBM)
       SelectObject(colorDC, hOldCBM);
   DeleteObject(colorBM);
Error2:
   DeleteDC(colorDC);
Error1:

   XorCsr(dstDC, csrPt, BOXCROSScsr);

   oldPt = newPt;

   return(TRUE);
}

void ColEraDP(HWND hWnd, UINT code, WPARAM wParam, LONG lParam)
{
   HDC paintDC;
   HBRUSH bBrush, hOldBrush;
   HBITMAP brushBM;
   LPPAINTSTRUCT lpps;
   RECT rcReturn;
   RECT rcWind;
   WORD reason;
   POINT thePt;

   LONG2POINT(lParam,thePt);

   switch (code) {
   case WM_PAINT:
      lpps = (LPPAINTSTRUCT)lParam;
      XorCsr(lpps->hdc, csrPt, BOXCROSScsr);
      break;

   case WM_HIDECURSOR:
   case WM_TERMINATE:
      HideCsr(NULL, hWnd, BOXCROSScsr);
      csrPt.x = csrPt.y = -1;
      SetCursorOn();
      break;

   case WM_SHOWCURSOR:
      csrPt.x = csrPt.y = -1;
      SetCursorOff();
      gfDirty = TRUE;
      break;

   case WM_WHOLE:
      UpdFlag(TRUE);

      wid = paintWid + 1;
      hgt = paintHgt + 1;
      thePt.x = paintWid/2;
      thePt.y = paintHgt/2;
      PostMessage(hWnd, WM_LBUTTONUP, wParam, lParam);

   /* setup drawing environment */
   case WM_LBUTTONDOWN:
      if(code == WM_LBUTTONDOWN)
         GetAspct(-((theSize + 1) << 2), &wid, &hgt);
      halfWid = wid >> 1;
      halfHgt = hgt >> 1;

      cntr = 0;
      dir = 0;
      rcReturn.left = rcReturn.right = thePt.x;
      rcReturn.top = rcReturn.bottom = thePt.y;

      GetWindowRect(hWnd, &rcWind);

      if(!(paintDC = GetDisplayDC(hWnd)))
         goto Error5;
      if(!(brushBM = CreatePatternBM(hdcWork, rgbColor[theBackg])))
         goto Error6;
      if(!(bBrush = CreatePatternBrush(brushBM)))
         goto Error7;
      if(!(fBrush = CreateSolidBrush(rgbColor[theForeg])))
         goto Error8;

      MUnrealizeObject(bBrush);
      hOldBrush = SelectObject(hdcWork, bBrush);

      if(bExchanged)
	  PasteDownRect(rDirty.left, rDirty.top,
	       rDirty.right-rDirty.left, rDirty.bottom-rDirty.top);

      reason = TrackTool(hWnd, DrawColEra, &rcReturn, &wParam, paintDC);

      XorCsr(paintDC, csrPt, BOXCROSScsr);

      theBounds.left   += imageView.left - halfWid;
      theBounds.top    += imageView.top - halfHgt;
      theBounds.right  += imageView.left + halfWid;
      theBounds.bottom += imageView.top + halfHgt;

      UnionWithRect(&rDirty, &theBounds);

      AdviseDataChange();

      if (code != WM_WHOLE)
         XorCsr(paintDC, csrPt, BOXCROSScsr);
      else
         csrPt.x = csrPt.y = -1;

      DeleteObject(fBrush);
Error8:
      if (hOldBrush)
	 SelectObject(hdcWork, hOldBrush);
      DeleteObject(bBrush);
Error7:
      DeleteObject(brushBM);
Error6:
      ReleaseDC(hWnd, paintDC);
Error5:
      break;

   case WM_MOUSEMOVE:
      if(!(paintDC = GetDisplayDC(hWnd)))
         return;
      XorCsr(paintDC, csrPt, BOXCROSScsr);
      XorCsr(paintDC, csrPt = thePt, BOXCROSScsr);
      ReleaseDC(hWnd, paintDC);
      break;

   default:
      break;
   }
}
