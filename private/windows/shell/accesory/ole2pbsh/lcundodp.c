/****************************Module*Header******************************\
* Copyright (c) 1987 - 1991  Microsoft Corporation                      *
\***********************************************************************/
/********************************************************
*                                                       *
*   file:   LcUndoDP.c                                  *
*   system: PC Paintbrush for MS-Windows                *
*   descr:  local undo draw proc                        *
*   date:   04/01/87 @ 10:10                            *
*                                                       *
********************************************************/

#include <windows.h>
#include <port1632.h>

#include "oleglue.h"
#include "pbrush.h"


#define oldPt (*((LPPOINT)lprBounds  ))
#define newPt (*((LPPOINT)lprBounds+1))

extern RECT theBounds;
extern POINT csrPt;
extern POINT polyPts[];
extern int cursTool;
extern int paintWid, paintHgt;
extern RECT imageView;
extern int theTool, theSize;
extern DPPROC DrawProc;
extern DPPROC dpArray[];
extern struct	csstat CursorStat;
extern LPTSTR DrawCursor;

static int cntr, dir, wid, hgt, halfWid, halfHgt;

extern BOOL bExchanged;
extern RECT rDirty;

LONG APIENTRY DrawLcUndo(HDC dstDC, LPRECT lprBounds, WPARAM wParam)
{
   int bitmapWid, bitmapHgt;
//   HRGN hRgn,hRgn1;
   RECT r;

   if(01&cntr++)
      return(FALSE);

   XorCsr(dstDC, csrPt, BOXXcsr);
   csrPt = newPt;

   newPt.x -= halfWid;
   newPt.y -= halfHgt;

   ConstrainBrush(lprBounds, wParam, &dir);

   if(newPt.x >= oldPt.x) {
      r.left = oldPt.x;
      r.right = newPt.x + wid;
   } else {
      r.left = newPt.x;
      r.right = oldPt.x + wid;
   }
   if(newPt.y >= oldPt.y) {
      r.top = oldPt.y;
      r.bottom = newPt.y + hgt;
   } else {
      r.top = newPt.y;
      r.bottom = oldPt.y + hgt;
   }
   bitmapWid = r.right - r.left;
   bitmapHgt = r.bottom - r.top;

   PolyTo(oldPt, newPt, wid, hgt);
//   if(!(hRgn = CreatePolygonRgn(polyPts, 6, WINDING)))
//	goto Error1;
//   if(!(hRgn1 = CreateRectRgn(0, 0, paintWid, paintHgt)))
//	goto Error2;
//   GetClipRgn(hdcWork,&hRgn1);
//   SelectClipRgn(hdcWork, hRgn);
//   DeleteObject(hRgn);

   BitBlt(hdcWork, r.left + imageView.left, r.top + imageView.top, bitmapWid, bitmapHgt,
	  hdcImage,r.left + imageView.left, r.top + imageView.top, SRCCOPY);

   BitBlt(dstDC, r.left, r.top, bitmapWid, bitmapHgt,
	  hdcWork,r.left + imageView.left, r.top + imageView.top, SRCCOPY);

//   SelectClipRgn(hdcWork, hRgn1);
//   DeleteObject(hRgn1);

//Error2:
//Error1:
   XorCsr(dstDC, csrPt, BOXXcsr);
   oldPt = newPt;

   return(TRUE);
}

void LcUndoDP(HWND hWnd, UINT code, WPARAM wParam, LONG lParam)
{
   RECT rcReturn;
   HDC paintDC;
   LPPAINTSTRUCT lpps;
   POINT thePt;

   LONG2POINT(lParam,thePt);

   switch (code) {
   case WM_PAINT:
      lpps = (LPPAINTSTRUCT) lParam;
      XorCsr(lpps->hdc, csrPt, BOXXcsr);
      break;

   case WM_HIDECURSOR:
      HideCsr(NULL, hWnd, BOXXcsr);
      csrPt.x = csrPt.y = -1;
      SetCursorOn();
      break;

   case WM_SHOWCURSOR:
      csrPt.x = csrPt.y = -1;
      SetCursorOff();
      break;

   case WM_LBUTTONDOWN:
      if(bExchanged)
      {
    	PasteDownRect(rDirty.left, rDirty.top,
	       rDirty.right-rDirty.left, rDirty.bottom-rDirty.top);
      }

      GetAspct(-((theSize + 1) << 2), &wid, &hgt);
      halfWid = wid >> 1;
      halfHgt = hgt >> 1;

      cntr = 0;
      dir = 0;
      rcReturn.left = thePt.x - halfWid;
      rcReturn.top = thePt.y - halfHgt;
      rcReturn.right = thePt.x;
      rcReturn.bottom = thePt.y;

      code = TrackTool(hWnd, DrawLcUndo, &rcReturn, &wParam, NULL);

      SendMessage(hWnd, WM_HIDECURSOR, 0, 0L);
      theBounds.left   += imageView.left - (wid>>1);
      theBounds.top    += imageView.top  - (hgt>>1);
      theBounds.right  += imageView.left + (wid>>1);
      theBounds.bottom += imageView.top  + (hgt>>1);

      UnionWithRect(&rDirty, &theBounds);

      AdviseDataChange();

   case WM_TERMINATE:
      if(code == WM_TERMINATE)
         SendMessage(hWnd, WM_HIDECURSOR, 0, 0L);
      DrawProc = dpArray[theTool];
      cursTool = theTool;
      CursorStat.noted = FALSE;
      PbSetCursor(DrawCursor = szPbCursor(theTool));
      SendMessage(hWnd, WM_SHOWCURSOR, wParam, lParam);
      SetCursor(LoadCursor(NULL, IDC_ARROW));
      break;

   case WM_MOUSEMOVE:
      if(!(paintDC = GetDisplayDC(hWnd)))
         return;
      XorCsr(paintDC, csrPt, BOXXcsr);
      XorCsr(paintDC, csrPt = thePt, BOXXcsr);
      ReleaseDC(hWnd, paintDC);
      break;
   }
}
