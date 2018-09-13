/****************************Module*Header******************************\
* Copyright (c) 1987 - 1991  Microsoft Corporation                      *
\***********************************************************************/
/********************************************************
*							*
*	file:	LineDP.c				*
*	system: PC Paintbrush for MS-Windows		*
*	descr:	line draw proc				*
*	date:	03/16/87 @ 15:15			*
********************************************************/

#include "onlypbr.h"
#undef NORASTEROPS
#undef NOWINMESSAGES
#undef NOKEYSTATES

#include <windows.h>
#include "port1632.h"
#include "pbrush.h"
#include "pbserver.h"

extern BOOL bExchanged;
extern RECT rDirty;

extern DWORD *rgbColor;
extern int horzDotsMM, vertDotsMM;
extern int theSize, theForeg;

LONG APIENTRY DrawLine(HDC dstDC, LPRECT lprBounds, WPARAM wParam)
{
   int dx, dy;
   BOOL bFinal;

   /* are we drawing the actual line or just the outline? */
   bFinal = (GetROP2(dstDC) == R2_COPYPEN);

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

      if (bFinal) {
	  if(bExchanged)
	     PasteDownRect(rDirty.left, rDirty.top,
	       rDirty.right-rDirty.left, rDirty.bottom-rDirty.top);

	  {
	  int temp;
	  RECT rcRect;

	/* get bounding rect of line's center */
	  rcRect.left = lprBounds->left + imageView.left;
	  rcRect.top = lprBounds->top + imageView.top;
	  rcRect.right = rcRect.left + dx;
	  rcRect.bottom = rcRect.top + dy;

	MMoveTo(hdcWork, lprBounds->left + imageView.left,
			 lprBounds->top + imageView.top);

	/* normalize rectangle width */
	  if (rcRect.right < rcRect.left) {
		temp = rcRect.left;
		rcRect.left = rcRect.right;
		rcRect.right = temp;
	  }

       /* normalize rectangle height */
	  if (rcRect.bottom < rcRect.top) {
	     temp = rcRect.top;
	     rcRect.top = rcRect.bottom;
	     rcRect.bottom = temp;
	  }

       /* Inflate bounding box to surround the thick pen */
	  CompensateForPen(hdcWork, &rcRect);

	  LineTo(hdcWork, lprBounds->left + dx + imageView.left,
			  lprBounds->top + dy + imageView.top);

	  BitBlt(dstDC, rcRect.left - imageView.left,
		     rcRect.top - imageView.top,
		     rcRect.right - rcRect.left,
		     rcRect.bottom - rcRect.top,
		 hdcWork, rcRect.left,rcRect.top,
		 SRCCOPY);

	  UnionWithRect(&rDirty, &rcRect);

	  if (fOLE)
	      SendDocChangeMsg(vpdoc, OLE_CHANGED);

	 }

      } else {
	 MMoveTo(dstDC, lprBounds->left, lprBounds->top);
         LineTo(dstDC, lprBounds->left + dx, lprBounds->top + dy);
      }

   return(TRUE);
}

void LineDP(HWND hWnd, UINT message, WPARAM wParam, LONG lParam)
{
   HDC dstDC;
   RECT rcReturn;
   HPEN pen, hOldPen;
   int nSavedDC;
   POINT newPt;

   LONG2POINT(lParam,newPt);
   if(message == WM_LBUTTONDOWN) {
      rcReturn.left = rcReturn.right = newPt.x;
      rcReturn.top = rcReturn.bottom = newPt.y;

      message = TrackTool(hWnd, DrawLine, &rcReturn, &wParam, NULL);

      if(message!=WM_RBUTTONDOWN && message!=WM_TERMINATE) {
         if(!(dstDC = GetDisplayDC(hWnd)))
            goto Error1;

	 nSavedDC = SaveDC(dstDC);

         SetROP2(dstDC, R2_COPYPEN);
         if(!(pen = CreatePen(PS_INSIDEFRAME, theSize, rgbColor[theForeg])))
	    goto Error2;
	 hOldPen = SelectObject(hdcWork, pen);

         DrawLine(dstDC, &rcReturn, wParam);

         if(hOldPen)
	    SelectObject(hdcWork, hOldPen);
         DeleteObject(pen);
Error2:
         RestoreDC(dstDC, nSavedDC);
         ReleaseDC(hWnd, dstDC);
Error1:
         ;
      }
   }
}
