/****************************Module*Header******************************\
* Copyright (c) 1987 - 1991  Microsoft Corporation                      *
\***********************************************************************/
/********************************************************
*							*
*	file:	OvalDP.c				*
*	system: PC Paintbrush for MS-Windows		*
*	descr:	line draw proc				*
*	date:	03/16/87 @ 15:15			*
********************************************************/

#include "onlypbr.h"
#undef NOWINMESSAGES
#undef NORASTEROPS

#include <windows.h>
#include "port1632.h"
//#define NOEXTERN
#include "pbrush.h"
#include "pbserver.h"

extern BOOL bExchanged;
extern RECT rDirty;

LONG APIENTRY DrawOval(HDC dstDC, LPRECT lprBounds, WPARAM wParam)
{
   RECT rcTemp;

   rcTemp = *lprBounds;
   ConstrainRect(&rcTemp, NULL, wParam);

   /* are we drawing the actual box or just the outline? */
   if(GetROP2(dstDC) == R2_COPYPEN) {

       if(bExchanged)
	  PasteDownRect(rDirty.left, rDirty.top,
	       rDirty.right-rDirty.left, rDirty.bottom-rDirty.top);

       rcTemp.left += imageView.left;
       rcTemp.right += imageView.left;
       rcTemp.top += imageView.top;
       rcTemp.bottom += imageView.top;

       Ellipse(hdcWork, rcTemp.left,
			rcTemp.top,
			rcTemp.right,
			rcTemp.bottom);

       BitBlt(dstDC, rcTemp.left - imageView.left,
		     rcTemp.top - imageView.top,
		     rcTemp.right - rcTemp.left,
		     rcTemp.bottom - rcTemp.top,
	      hdcWork, rcTemp.left,rcTemp.top,
	      SRCCOPY);

       UnionWithRect(&rDirty, &rcTemp);

      if (fOLE)
	   SendDocChangeMsg(vpdoc, OLE_CHANGED);

   } else
      Ellipse(dstDC, rcTemp.left, rcTemp.top, rcTemp.right, rcTemp.bottom);

   return(TRUE);
}

void OvalDP(HWND hWnd, UINT message, WPARAM wParam, LONG lParam)
{
   HDC dstDC;
   RECT rcReturn;
   HPEN pen, hOldPen;
   HBRUSH brush, hOldBrush;
   int nSavedDC;
   POINT newPt;
   int iDraw;

   LONG2POINT(lParam,newPt);
   if(message == WM_LBUTTONDOWN) {
      rcReturn.left = rcReturn.right = newPt.x;
      rcReturn.top = rcReturn.bottom = newPt.y;
      if(TrackTool(hWnd, DrawOval, &rcReturn, &wParam, NULL)
            != WM_RBUTTONDOWN) {
         if(!(dstDC = GetDisplayDC(hWnd)))
            goto Error1;
	 nSavedDC = SaveDC(hdcWork);

	 SetBkMode(hdcWork, OPAQUE);
	 SetROP2(hdcWork, R2_COPYPEN);
	 iDraw=SetROP2(dstDC, R2_COPYPEN);

         if(theTool == OVALFILLtool)
            brush = CreateSolidBrush(rgbColor[theForeg]);
         else
            brush = GetStockObject(NULL_BRUSH);
         if(!brush)
            goto Error2;
	 hOldBrush = SelectObject(hdcWork, brush);

         if(theSize > 0)
            pen = CreatePen(PS_INSIDEFRAME, theSize, 
               rgbColor[(theTool == OVALFRAMEtool) ? theForeg : theBackg]);
         else
            pen = GetStockObject(NULL_PEN);
         if(!pen)
            goto Error3;
	 hOldPen = SelectObject(hdcWork, pen);

	 DrawOval(dstDC, &rcReturn, wParam);

         if(hOldPen)
	    SelectObject(hdcWork, hOldPen);
         if(theSize > 0)
            DeleteObject(pen);
Error3:
         if(hOldBrush)
	    SelectObject(hdcWork, hOldBrush);
         if(theTool == OVALFILLtool)
            DeleteObject(brush);
Error2:
	 RestoreDC(hdcWork, nSavedDC);
	 SetROP2(dstDC, iDraw);
	 ReleaseDC(hWnd, dstDC);
Error1:
         ;
      }
   }
}
