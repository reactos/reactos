/****************************Module*Header******************************\
* Copyright (c) 1987 - 1991  Microsoft Corporation                      *
\***********************************************************************/
/********************************************************
*							*
*	file:	PolyDP.c				*
*	system: PC Paintbrush for MS-Windows		*
*	descr:	rect draw proc				*
*	date:	03/16/87 @ 15:15			*
********************************************************/

#include "onlypbr.h"
#undef NOWINMESSAGES
#undef NORASTEROPS
#undef NOKEYSTATES

#include <windows.h>
#include "port1632.h"
//#define NOEXTERN
#include "pbrush.h"
#include "pbserver.h"

#define POLYSLOP 2
static int nCount;

LONG APIENTRY DrawPoly(HDC dstDC, LPRECT lprBounds, WPARAM wParam)
{
   int dx, dy;

   if(wParam & MK_SHIFT) {
      dx = abs(lprBounds->right - lprBounds->left);
      dy = abs(lprBounds->bottom - lprBounds->top);

      if((long)horzDotsMM * dy < (long)vertDotsMM * dx/2) {
         dx = lprBounds->right - lprBounds->left;
         dy = 0;
      } else if((long)vertDotsMM * dx < (long)horzDotsMM * dy/2) {
         dy = lprBounds->bottom - lprBounds->top;
         dx = 0;
      } else if((long)horzDotsMM * dy < (long)vertDotsMM * dx) {
         dy = (int)((long)(lprBounds->bottom<lprBounds->top ? -dx : dx)
               *vertDotsMM/horzDotsMM);
         dx = lprBounds->right - lprBounds->left;
      } else {
         dx = (int)((long)(lprBounds->right<lprBounds->left ? -dy : dy)
               *horzDotsMM/vertDotsMM);
         dy = lprBounds->bottom - lprBounds->top;
      }
   } else {
      dx = lprBounds->right - lprBounds->left;
      dy = lprBounds->bottom - lprBounds->top;
   }

   polyPts[nCount].x = lprBounds->left + dx;
   polyPts[nCount].y = lprBounds->top + dy;

   MMoveTo(dstDC, lprBounds->left, lprBounds->top);
   LineTo(dstDC, lprBounds->left + dx, lprBounds->top + dy);

   return(TRUE);
}

void PolyDP(HWND hWnd, UINT message, WPARAM wParam, LONG lParam)
{
   static BOOL drawing = FALSE;
   static BOOL bTerminated = TRUE;
   static HDC dstDC = NULL;
   static int nSavedDC;
   static RECT rcReturn;

   HPEN pen, hOldPen;
   HBRUSH brush, hOldBrush;
   POINT newPt;
   int iBk,iROP,i;

   LONG2POINT(lParam,newPt);

   switch(message) {
   case WM_LBUTTONDOWN:
      if(!drawing) {
         drawing = TRUE;

         if(!(dstDC = GetDisplayDC(hWnd)))
            return;
         nSavedDC = SaveDC(dstDC);

         SelectObject(dstDC, GetStockObject(WHITE_PEN));
         SetROP2(dstDC, R2_XORPEN);

         rcReturn.right = polyPts[0].x = newPt.x;
         rcReturn.bottom = polyPts[0].y = newPt.y;
	 nCount = 1;
      }

      rcReturn.left = polyPts[nCount-1].x;
      rcReturn.top = polyPts[nCount-1].y;
      rcReturn.right = newPt.x;
      rcReturn.bottom = newPt.y;
      if(TrackTool(hWnd, DrawPoly, &rcReturn, &wParam, NULL)
            == WM_RBUTTONDOWN) {
	 PolyDP(hWnd, WM_TERMINATE, wParam, lParam);
         return;
      }
      DrawPoly(dstDC, &rcReturn, wParam);
      ++nCount;

      if(nCount < MAXpts) {
	 if(	  polyPts[nCount-1].x >= polyPts[0].x - POLYSLOP
	       && polyPts[nCount-1].x <= polyPts[0].x + POLYSLOP
	       && polyPts[nCount-1].y >= polyPts[0].y - POLYSLOP
	       && polyPts[nCount-1].y <= polyPts[0].y + POLYSLOP) {
	    if(nCount == 2) {
	       PolyDP(hWnd, WM_TERMINATE, wParam, lParam);
               break;
            } else {
               DrawPoly(dstDC, &rcReturn, wParam);
	       --nCount;
            }
         } else
            break;
      }

      /* notice that we fall through once we hit MAXpts (without warning)
         or we click close enough to the point of origin */
   case WM_LBUTTONDBLCLK:
      if(!drawing)
         break;
      Polyline(dstDC, polyPts, nCount);

      iBk=SetBkMode(hdcWork, OPAQUE);
      iROP=SetROP2(hdcWork, R2_COPYPEN);

      if(theTool == POLYFILLtool)
	 brush = CreateSolidBrush(rgbColor[theForeg]);
      else
         brush = GetStockObject(NULL_BRUSH);
      if(!brush)
         goto Error2;
      hOldBrush = SelectObject(hdcWork, brush);

      if(theSize > 0)
         pen = CreatePen(PS_INSIDEFRAME, theSize, 
            rgbColor[(theTool == POLYFRAMEtool) ? theForeg : theBackg]);
      else
         pen = GetStockObject(NULL_PEN);
      if(!pen)
         goto Error3;
      hOldPen = SelectObject(hdcWork, pen);

      {
      RECT rcRect;
      int fResult = TRUE;
      extern BOOL bExchanged;
      extern RECT rDirty;


   /* get bounding rect of polygon (ignoring pen width) */
      PolyRect(polyPts, nCount, &rcRect);

   /* Inflate bounding box to surround the thick pen */
      CompensateForPen(hdcWork, &rcRect);

       if(bExchanged)
	  PasteDownRect(rDirty.left, rDirty.top,
	       rDirty.right-rDirty.left, rDirty.bottom-rDirty.top);

       for (i=0;i<nCount;i++) {
	   polyPts[i].x += imageView.left;
	   polyPts[i].y += imageView.top;
       }
       fResult = Polygon(hdcWork, polyPts, nCount);

       rcRect.left += imageView.left;
       rcRect.right += imageView.left;
       rcRect.top += imageView.top;
       rcRect.bottom += imageView.top;

       BitBlt(dstDC, rcRect.left - imageView.left,
		     rcRect.top - imageView.top,
		     rcRect.right - rcRect.left,
		     rcRect.bottom - rcRect.top,
	      hdcWork, rcRect.left,rcRect.top,
	      SRCCOPY);

       UnionWithRect(&rDirty, &rcRect);

      if (fOLE)
	   SendDocChangeMsg(vpdoc, OLE_CHANGED);


//   if (!fResult)
//	SimpleMessage(IDSNotEnufMem, NULL, MB_OK);
      }




      if(hOldPen)
	 SelectObject(hdcWork, hOldPen);
      if(theSize > 0)
         DeleteObject(pen);
Error3:
      if(hOldBrush)
	 SelectObject(hdcWork, hOldBrush);
      if(theTool == POLYFILLtool)
         DeleteObject(brush);
Error2:

      drawing = FALSE;
      SetBkMode(hdcWork,iBk);
      SetROP2(hdcWork,iROP);
      PolyDP(hWnd, WM_TERMINATE, wParam, lParam);
      break;

   case WM_RBUTTONDOWN:
   case WM_TERMINATE:
      if(drawing) {
	 Polyline(dstDC, polyPts, nCount);
         drawing = FALSE;
      }

      if(dstDC) {
         RestoreDC(dstDC, nSavedDC);
         ReleaseDC(hWnd, dstDC);
         dstDC = NULL;
      }
      break;

   default:
      break;
   }
}
