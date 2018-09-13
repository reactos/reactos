/****************************Module*Header******************************\
* Copyright (c) 1987 - 1991  Microsoft Corporation                      *
\***********************************************************************/
/********************************************************
*                                                       *
*   file:   BrushDP.c                                   *
*   system: PC Paintbrush for MS-Windows                *
*   descr:  brush draw proc                             *
*   date:   04/07/87 @ 14:10                            *
********************************************************/

#include <windows.h>
#include <port1632.h>

#include "oleglue.h"
#include "pbrush.h"


#define oldPt (*((LPPOINT)lprBounds  ))
#define newPt (*((LPPOINT)lprBounds+1))

extern RECT theBounds;
extern RECT imageView;
static int cntr, dir, wid, hgt;
extern int theBrush, theSize, theForeg;
extern DWORD *rgbColor;
extern POINT csrPt;
extern POINT polyPts[];

extern BOOL bExchanged;
extern RECT rDirty;

LONG APIENTRY DrawBrush(HDC dstDC, LPRECT lprBounds, WPARAM wParam)
{
   int dx, dy, i;
   int octant;
   RECT tan;
   int width;
   int height;
   RECT rcRect;

   if(01 & cntr++)
      return(FALSE);

   XorCsr(dstDC, csrPt, theBrush);
   csrPt = newPt;

   ConstrainBrush(lprBounds, wParam, &dir);

   dx = abs(newPt.x - oldPt.x);
   dy = abs(newPt.y - oldPt.y);

   if(newPt.x > oldPt.x) {
      if (newPt.y < oldPt.y)
         octant = (dx > dy) ? 0 : 1;
      else
         octant = (dx > dy) ? 7 : 6;
   } else {
      if (newPt.y < oldPt.y)
         octant = (dx > dy) ? 3 : 2;
      else
         octant = (dx > dy) ? 4 : 5;
   }

   switch (theBrush) {
   case RECTbrush:
   case VERTbrush:
   case HORZbrush:
      PolyTo(oldPt, newPt, wid, hgt);
      for (i=0;i<6;i++) {
    	polyPts[i].x += imageView.left;
    	polyPts[i].y += imageView.top;
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
	     hdcWork, rcRect.left, rcRect.top,
	     SRCCOPY);

      break;

   case OVALbrush:
      if(newPt.x!=oldPt.x || newPt.y!=oldPt.y) {
         GetTanPt(wid, hgt, newPt.x-oldPt.x, newPt.y-oldPt.y, &tan);

    	 polyPts[0].x = oldPt.x + tan.left + imageView.left;
    	 polyPts[1].x = oldPt.x + tan.right + imageView.left;
    	 polyPts[2].x = newPt.x + tan.right + imageView.left;
    	 polyPts[3].x = newPt.x + tan.left + imageView.left;

    	 polyPts[0].y = oldPt.y + tan.top + imageView.top;
    	 polyPts[1].y = oldPt.y + tan.bottom + imageView.top;
    	 polyPts[2].y = newPt.y + tan.bottom + imageView.top;
    	 polyPts[3].y = newPt.y + tan.top + imageView.top;

    	 /* get bounding rect of polygon (ignoring pen width) */
    	 PolyRect(polyPts, 4, &rcRect);

    	 /* Inflate bounding box to surround the thick pen */
    	 CompensateForPen(hdcWork, &rcRect);

    	 width = rcRect.right - rcRect.left;
    	 height = rcRect.bottom - rcRect.top;

    	 Polygon(hdcWork, polyPts, 4);

    	 BitBlt(dstDC, rcRect.left - imageView.left,
    		       rcRect.top - imageView.top,
    		       width, height,
		hdcWork, rcRect.left, rcRect.top,
		SRCCOPY);

      }
      Ellipse(hdcWork, newPt.x + imageView.left,
		       newPt.y + imageView.top,
		       newPt.x + wid + imageView.left,
		       newPt.y + hgt + imageView.top);

      BitBlt(dstDC, newPt.x, newPt.y, wid, hgt,
	     hdcWork, newPt.x + imageView.left,
		      newPt.y + imageView.top,
	     SRCCOPY);


      break;

   case SLANTLbrush:
      switch(octant) {
      case 0 :
    	 polyPts[0].x = oldPt.x + imageView.left;
    	 polyPts[1].x = oldPt.x + 1 + imageView.left;
    	 polyPts[2].x = newPt.x + 1 + imageView.left;
    	 polyPts[3].x = newPt.x + hgt + 1 + imageView.left;
    	 polyPts[4].x = newPt.x + hgt + imageView.left;
    	 polyPts[5].x = oldPt.x + hgt + imageView.left;
    	 polyPts[0].y = polyPts[1].y = oldPt.y + hgt + imageView.top;
    	 polyPts[2].y = newPt.y + hgt + imageView.top;
    	 polyPts[3].y = polyPts[4].y = newPt.y + imageView.top;
    	 polyPts[5].y = oldPt.y + imageView.top;
         break;

      case 1 :
    	 polyPts[0].x = oldPt.x + imageView.left;
    	 polyPts[1].x = oldPt.x + 1 + imageView.left;
    	 polyPts[2].x = oldPt.x + hgt + 1 + imageView.left;
    	 polyPts[3].x = newPt.x + hgt + 1 + imageView.left;
    	 polyPts[4].x = newPt.x + hgt + imageView.left;
    	 polyPts[5].x = newPt.x + imageView.left;
    	 polyPts[0].y = polyPts[1].y = oldPt.y + hgt + imageView.top;
    	 polyPts[2].y = oldPt.y + imageView.top;
    	 polyPts[3].y = polyPts[4].y = newPt.y + imageView.top;
    	 polyPts[5].y = newPt.y + hgt + imageView.top;
         break;

      case 2 :
      case 3 :
    	 polyPts[0].x = oldPt.x + 1 + imageView.left;
    	 polyPts[1].x = oldPt.x + imageView.left;
    	 polyPts[2].x = newPt.x + imageView.left;
    	 polyPts[3].x = newPt.x + hgt + imageView.left;
    	 polyPts[4].x = newPt.x + hgt + 1 + imageView.left;
    	 polyPts[5].x = oldPt.x + hgt + 1 + imageView.left;
    	 polyPts[0].y = polyPts[1].y = oldPt.y + hgt + imageView.top;
    	 polyPts[2].y = newPt.y + hgt + imageView.top;
    	 polyPts[3].y = polyPts[4].y = newPt.y + imageView.top;
    	 polyPts[5].y = oldPt.y + imageView.top;
         break;

      case 4 :
    	 polyPts[0].x = oldPt.x + hgt + 1 + imageView.left;
    	 polyPts[1].x = oldPt.x + hgt + imageView.left;
    	 polyPts[2].x = newPt.x + hgt + imageView.left;
    	 polyPts[3].x = newPt.x + imageView.left;
    	 polyPts[4].x = newPt.x + 1 + imageView.left;
    	 polyPts[5].x = oldPt.x + 1 + imageView.left;
    	 polyPts[0].y = polyPts[1].y = oldPt.y + imageView.top;
    	 polyPts[2].y = newPt.y + imageView.top;
    	 polyPts[3].y = polyPts[4].y = newPt.y + hgt + imageView.top;
    	 polyPts[5].y = oldPt.y + hgt + imageView.top;
         break;

      case 5 :
    	 polyPts[0].x = oldPt.x + hgt + 1 + imageView.left;
    	 polyPts[1].x = oldPt.x + hgt + imageView.left;
    	 polyPts[2].x = oldPt.x + imageView.left;
    	 polyPts[3].x = newPt.x + imageView.left;
    	 polyPts[4].x = newPt.x + 1 + imageView.left;
    	 polyPts[5].x = newPt.x + hgt + 1 + imageView.left;
    	 polyPts[0].y = polyPts[1].y = oldPt.y + imageView.top;
    	 polyPts[2].y = oldPt.y + hgt + imageView.top;
    	 polyPts[3].y = polyPts[4].y = newPt.y + hgt + imageView.top;
    	 polyPts[5].y = newPt.y + imageView.top;
         break;

      default :
    	 polyPts[0].x = oldPt.x + hgt + imageView.left;
    	 polyPts[1].x = oldPt.x + hgt + 1 + imageView.left;
    	 polyPts[2].x = newPt.x + hgt + 1 + imageView.left;
    	 polyPts[3].x = newPt.x + 1 + imageView.left;
    	 polyPts[4].x = newPt.x + imageView.left;
    	 polyPts[5].x = oldPt.x + imageView.left;
    	 polyPts[0].y = polyPts[1].y = oldPt.y + imageView.top;
    	 polyPts[2].y = newPt.y + imageView.top;
    	 polyPts[3].y = polyPts[4].y = newPt.y + hgt + imageView.top;
    	 polyPts[5].y = oldPt.y + hgt + imageView.top;
         break;
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
		    rcRect.right - rcRect.left,
		    rcRect.bottom - rcRect.top,
	     hdcWork, rcRect.left,rcRect.top,
	     SRCCOPY);

      break;

   case SLANTRbrush:
      switch(octant) {
      case 0 :
      case 1 :
    	 polyPts[0].x = oldPt.x + hgt + imageView.left;
    	 polyPts[1].x = oldPt.x + hgt + 1 + imageView.left;
    	 polyPts[2].x = newPt.x + hgt + 1 + imageView.left;
    	 polyPts[3].x = newPt.x + 1 + imageView.left;
    	 polyPts[4].x = newPt.x + imageView.left;
    	 polyPts[5].x = oldPt.x + imageView.left;
    	 polyPts[0].y = polyPts[1].y = oldPt.y + hgt + imageView.top;
    	 polyPts[2].y = newPt.y + hgt + imageView.top;
    	 polyPts[3].y = polyPts[4].y = newPt.y + imageView.top;
    	 polyPts[5].y = oldPt.y + imageView.top;
         break;

      case 2 :
    	 polyPts[0].x = oldPt.x + hgt + 1 + imageView.left;
    	 polyPts[1].x = oldPt.x + hgt + imageView.left;
    	 polyPts[2].x = oldPt.x + imageView.left;
    	 polyPts[3].x = newPt.x + imageView.left;
    	 polyPts[4].x = newPt.x + 1 + imageView.left;
    	 polyPts[5].x = newPt.x + hgt + 1 + imageView.left;
    	 polyPts[0].y = polyPts[1].y = oldPt.y + hgt + imageView.top;
    	 polyPts[2].y = oldPt.y + imageView.top;
    	 polyPts[3].y = polyPts[4].y = newPt.y + imageView.top;
    	 polyPts[5].y = newPt.y + hgt + imageView.top;
         break;

      case 3 :
    	 polyPts[0].x = oldPt.x + hgt + 1 + imageView.left;
    	 polyPts[1].x = oldPt.x + hgt + imageView.left;
    	 polyPts[2].x = newPt.x + hgt + imageView.left;
    	 polyPts[3].x = newPt.x + imageView.left;
    	 polyPts[4].x = newPt.x + 1 + imageView.left;
    	 polyPts[5].x = oldPt.x + 1 + imageView.left;
    	 polyPts[0].y = polyPts[1].y = oldPt.y + hgt + imageView.top;
    	 polyPts[2].y = newPt.y + hgt + imageView.top;
    	 polyPts[3].y = polyPts[4].y = newPt.y + imageView.top;
    	 polyPts[5].y = oldPt.y + imageView.top;
         break;

      case 4 :
      case 5 :
    	 polyPts[0].x = oldPt.x + 1 + imageView.left;
    	 polyPts[1].x = oldPt.x + imageView.left;
    	 polyPts[2].x = newPt.x + imageView.left;
    	 polyPts[3].x = newPt.x + hgt + imageView.left;
    	 polyPts[4].x = newPt.x + hgt + 1 + imageView.left;
    	 polyPts[5].x = oldPt.x + hgt + 1 + imageView.left;
    	 polyPts[0].y = polyPts[1].y = oldPt.y + imageView.top;
    	 polyPts[2].y = newPt.y + imageView.top;
    	 polyPts[3].y = polyPts[4].y = newPt.y + hgt + imageView.top;
    	 polyPts[5].y = oldPt.y + hgt + imageView.top;
         break;

      case 6 :
    	 polyPts[0].x = oldPt.x + imageView.left;
    	 polyPts[1].x = oldPt.x + 1 + imageView.left;
    	 polyPts[2].x = oldPt.x + hgt + 1 + imageView.left;
    	 polyPts[3].x = newPt.x + hgt + 1 + imageView.left;
    	 polyPts[4].x = newPt.x + hgt + imageView.left;
    	 polyPts[5].x = newPt.x + imageView.left;
    	 polyPts[0].y = polyPts[1].y = oldPt.y + imageView.top;
    	 polyPts[2].y = oldPt.y + hgt + imageView.top;
    	 polyPts[3].y = polyPts[4].y = newPt.y + hgt + imageView.top;
    	 polyPts[5].y = newPt.y + imageView.top;
         break;

      default :
    	 polyPts[0].x = oldPt.x + imageView.left;
    	 polyPts[1].x = oldPt.x + 1 + imageView.left;
    	 polyPts[2].x = newPt.x + 1 + imageView.left;
    	 polyPts[3].x = newPt.x + hgt + 1 + imageView.left;
    	 polyPts[4].x = newPt.x + hgt + imageView.left;
    	 polyPts[5].x = oldPt.x + hgt + imageView.left;
    	 polyPts[0].y = polyPts[1].y = oldPt.y + imageView.top;
    	 polyPts[2].y = newPt.y + imageView.top;
    	 polyPts[3].y = polyPts[4].y = newPt.y + hgt + imageView.top;
    	 polyPts[5].y = oldPt.y + hgt + imageView.top;
         break;
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
		    rcRect.right - rcRect.left,
		    rcRect.bottom - rcRect.top,
	     hdcWork, rcRect.left,rcRect.top,
	     SRCCOPY);

      break;
   }

   oldPt = newPt;
   XorCsr(dstDC, csrPt, theBrush);

   return(TRUE);
}

void BrushDP(HWND hWnd, UINT code, WPARAM wParam, LONG lParam)
{
   POINT thePt;
   HDC paintDC;
   HBRUSH solidBrush, hOldBrush;
   LPPAINTSTRUCT lpps;
   RECT rcReturn;

   LONG2POINT(lParam,thePt);
   switch (code) {
   case WM_PAINT:
      lpps = (LPPAINTSTRUCT)lParam;
      XorCsr(lpps->hdc, csrPt, theBrush);
      break;

   case WM_HIDECURSOR:
   case WM_TERMINATE:
      HideCsr(NULL, hWnd, theBrush);
      csrPt.x = csrPt.y = -1;
      SetCursorOn();
      break;

   case WM_SHOWCURSOR:
      csrPt.x = csrPt.y = -1;
      SetCursorOff();
      break;

   /* setup drawing environment */
   case WM_LBUTTONDBLCLK:	// do this for fast pen drawing
   case WM_LBUTTONDOWN:
      cntr = 0;
      dir = 0;
      rcReturn.left = rcReturn.right = thePt.x;
      rcReturn.top = rcReturn.bottom = thePt.y;

      if(!(paintDC = GetDisplayDC(hWnd)))
         goto Error1;
      if(!(solidBrush = CreateSolidBrush(rgbColor[theForeg])))
	 goto Error2;

      hOldBrush = SelectObject(hdcWork, solidBrush);
      SelectObject(hdcWork, GetStockObject(NULL_PEN));

      GetAspct(theSize, &wid, &hgt);
      if(wid < 1)
         wid = 1;
      if(hgt < 1)
         hgt = 1;

      switch (theBrush) {
      case OVALbrush:
         ++wid;
         ++hgt;
         break;

      case HORZbrush:
         hgt = 1;
         break;

      case VERTbrush:
         wid = 1;
         break;

      default:
         break;
      }

      if(bExchanged)
      {
    	PasteDownRect(rDirty.left, rDirty.top,
    	       rDirty.right-rDirty.left, rDirty.bottom-rDirty.top);
      }

      code = TrackTool(hWnd, DrawBrush, &rcReturn, &wParam, paintDC);

      XorCsr(paintDC, csrPt, theBrush);
      theBounds.right  += wid;
      theBounds.bottom += hgt;

      theBounds.left += imageView.left;
      theBounds.right += imageView.left;
      theBounds.top += imageView.top;
      theBounds.bottom += imageView.top;

      UnionWithRect(&rDirty, &theBounds);

      AdviseDataChange();

      XorCsr(paintDC, csrPt, theBrush);

      if (hOldBrush)
	 SelectObject(hdcWork, hOldBrush);
      DeleteObject(solidBrush);
Error2:
      ReleaseDC(hWnd, paintDC);
Error1:
      break;

   case WM_MOUSEMOVE:
      if(!(paintDC = GetDisplayDC(hWnd)))
         return;
      XorCsr(paintDC, csrPt, theBrush);
      XorCsr(paintDC, csrPt = thePt, theBrush);
      ReleaseDC(hWnd, paintDC);
      break;
   }
   return;
}
